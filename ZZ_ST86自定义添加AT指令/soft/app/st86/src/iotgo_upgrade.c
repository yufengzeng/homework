#include "iotgo_upgrade.h"
#include "zz_task.h"
#include "zz_event.h"
#include "zz_app_mtask.h"
#include "zz_http_req.h"

#define IOTGO_UPGRADE_HTTP_DEF_PORT         (80)

#define IOTGO_UPGRADE_ERROR_OK              (0)
#define IOTGO_UPGRADE_ERROR_BADREQ          (400)
#define IOTGO_UPGRADE_ERROR_DOWNLOAD        (404)
#define IOTGO_UPGRADE_ERROR_MODEL           (411)
#define IOTGO_UPGRADE_ERROR_DIGEST          (409)
#define IOTGO_UPGRADE_ERROR_VERSION         (410)

#define IOTGO_UPGRADE_TIMER_DL              (ZZ_UPGRADE_TASK_TIMERID_BEGIN)
#define IOTGO_UPGRADE_TIMER_REBOOT          (ZZ_UPGRADE_TASK_TIMERID_BEGIN + 1)

#define IOTGO_UPGRADE_TIMEOUT               (300)   // seconds

#define IOTGO_UPGRADE_BIN_MAX_LEN           (400 * 1024)

#define IOTGO_UPGRADE_STORE_FLAG_START      (0x00)
#define IOTGO_UPGRADE_STORE_FLAG_PROCEED    (0x01)
#define IOTGO_UPGRADE_STORE_FLAG_FINISH     (0x02)

#define IOTGO_UPGRADE_STREAM                (ZZ_STREAM_2)

typedef struct  {
    char host[ZZ_SIZE_HOSTNAME];        /* domain or ip */
    uint16 port;                        /* port number */
    char path[100];                     /* bin file path */
    char sha256[65];                    /* digest of bin (lowercase) */
    char sequence[ZZ_SIZE_SEQUENCE];    /* sequence to server in response */
} IoTgoUpgradeInfo;

static IoTgoUpgradeInfo upgrade_info;
static SL_TASK upgrade_task;
static ZZHttpReqHandle upgrade_req = IOTGO_UPGRADE_STREAM;

static uint32 file_length = 0;
static uint8 *bin_buffer = NULL;
static uint32 bin_buffer_index = 0;

static void finish(uint32 error_code)
{
    zzEmitEvtP1(upgrade_task, ZZ_EVT_UPGRADE_TASK_STOP, error_code);
}

static void printIoTgoUpgradeInfo(const IoTgoUpgradeInfo *info)
{
    if (info)
    {
        zzLogP("--- upgrade info { ---");
        zzLogP("host:[%s]", info->host);
        zzLogP("port:[%u]", info->port);
        zzLogP("path:[%s]", info->path);
        zzLogP("sha256:[%s]", info->sha256);
        zzLogP("sequence:[%s]", info->sequence);
        zzLogP("--- upgrade info } ---");
    }
}


/* 
 * Parse host¡¢port¡¢path from dlurl and fill info
 * 
 * http://172.16.7.184:8088/ota/rom/123456abcde/user1.1024.new.bin
 * http://172.16.7.184/ota/rom/123456abcde/user1.1024.new.bin
 * https://172.16.7.184:8088/ota/rom/123456abcde/user1.1024.new.bin
 * https://172.16.7.184/ota/rom/123456abcde/user1.1024.new.bin    
 * http://172.16.7.184:8088/ota/rom/123456abcde/user1.1024.new.bin?deviceid=12&ts=1001&sign=xxx
 */
static void parseDownloadURLToUpgradeInfo(const char *dlurl, IoTgoUpgradeInfo *info)
{
    char *slash;
    char *colon;

    if (!dlurl || !info)
    {
        return;
    }
    
    char temp[10] = {0};
    SL_Memset(info->host, 0, sizeof(info->host));
    colon = (char *)SL_Strstr(&dlurl[7], ":");
    slash = (char *)SL_Strstr(&dlurl[7], "/");
    if (colon && slash)
    {
        SL_Strncpy(info->host, &dlurl[7], colon - &dlurl[7]);
        SL_Strncpy(temp, colon + 1, slash - colon + 1);
        info->port = SL_atoi(temp);
        SL_Strcpy(info->path, slash);
    }
    else if (slash)
    {
        SL_Strncpy(info->host, &dlurl[7], slash - &dlurl[7]);
        info->port = IOTGO_UPGRADE_HTTP_DEF_PORT;
        SL_Strcpy(info->path, slash);
    }
    else
    {
        zzLogP("bad downloadUrl!");
    }
}

/**
 * Parse upgrade request from PS. 
 * 
 * @return 0 for success, NON-0 for failure.
 */
int32 iotgoUpgradeParseUpgrade(const char *json_str)
{
    uint32 i = 0;
    cJSON *json_root = NULL;
    
    cJSON *action = NULL;
    cJSON *deviceid = NULL;
    cJSON *sequence = NULL;
    cJSON *params = NULL;

    /* params -> */
    cJSON *model = NULL;
    cJSON *version = NULL;
    cJSON *binList = NULL;

    /* binList -> */
    cJSON *downloadUrl = NULL;
    cJSON *digest = NULL;

    /* for array index */
    cJSON *array_item = NULL;

    bool flag_sequence = false;
    bool flag_dlurl = false;
    bool flag_digest = false;
    bool flag_model = false;
        
    json_root = cJSON_Parse(json_str);
    if (!json_root) 
    {
        zzLogP("cJSON_Parse err");
        return -1;
    }
    
    action = cJSON_GetObjectItem(json_root, "action");
    if (action)
    {
        zzLogP("action:[%s]", action->valuestring);
        if (0 != SL_Strcmp(action->valuestring, "upgrade"))
        {
            cJSON_Delete(json_root);
            return -1;
        }
    }
    else
    {
        cJSON_Delete(json_root);
        return -1;
    }

    SL_Memset(&upgrade_info, 0, sizeof(upgrade_info));

    
    deviceid = cJSON_GetObjectItem(json_root, "deviceid");
    sequence = cJSON_GetObjectItem(json_root, "sequence");

    if (deviceid)
        zzLogP("deviceid:[%s]", deviceid->valuestring);
    if (sequence)
    {
        SL_Strcpy(upgrade_info.sequence, sequence->valuestring);
        flag_sequence = true;
        zzLogP("sequence:[%s]", sequence->valuestring);
    }
    params = cJSON_GetObjectItem(json_root, "params");

    if (params)
    {
        model = cJSON_GetObjectItem(params, "model");
        version = cJSON_GetObjectItem(params, "version");
        
        if (model)
        {
            zzLogP("model:[%s]", model->valuestring);
            if (0 == SL_Strcmp(model->valuestring, zzGetModel()))
            {
                flag_model = true;
            }
        }
        
        if (version)
        {
            zzLogP("version:[%s]", version->valuestring);    
        }
        
        binList = cJSON_GetObjectItem(params, "binList");
    }
    
    if (binList)
    {
        for (i = 0; i < 1 /* cJSON_GetArraySize(binList) */; i++)
        {    
            zzLogP("---- index %u ----", i);
            array_item = cJSON_GetArrayItem(binList, i);
            
            downloadUrl = cJSON_GetObjectItem(array_item, "downloadUrl");
            digest = cJSON_GetObjectItem(array_item, "digest");
            
            if (downloadUrl)
            {
                zzLogP("downloadUrl:[%s]", downloadUrl->valuestring);
                parseDownloadURLToUpgradeInfo(downloadUrl->valuestring, &upgrade_info);
                flag_dlurl = true;
            }
            if (digest)
            {
                zzLogP("digest:[%s]", digest->valuestring);
                SL_Strcpy(upgrade_info.sha256, digest->valuestring);
                flag_digest = true;
            }
        }
    }
    
    cJSON_Delete(json_root);

    /* Verify upgrade info and then send event to upgrade task */
    if (flag_model && flag_digest && flag_dlurl && flag_sequence)
    {
        printIoTgoUpgradeInfo(&upgrade_info);
        zzEmitEvt(upgrade_task, ZZ_EVT_UPGRADE_TASK_START);
    }
    else /* Or ignore it! */
    {
        zzLogE("bad upgrade request!");
    }

    
    return 0;
}


static bool verifyBinDigest(void)
{
    sha256_context sha256_ctx;
    uint8 digest[32];
    uint8 digest_hex2[65];
    uint16 i;
    
    sha256_starts(&sha256_ctx);
    sha256_update(&sha256_ctx, bin_buffer, file_length);
    sha256_finish(&sha256_ctx, digest);

    for (i = 0; i < 64; i += 2)
    {
        SL_Sprintf(&digest_hex2[i], "%02x", digest[i/2]);
    }
    digest_hex2[64] = 0;
    
    zzLogP("digest_flash = [%s]", digest_hex2);
    zzLogP("digest_origin = [%s]", upgrade_info.sha256);
    
    if (0 == SL_Strcmp(digest_hex2, upgrade_info.sha256))
    {
        return true;
    }
    else
    {
        return false;
    }
    return true;
}


/**
 * Store content into bin_buffer
 */
static void storeContentToBinBuffer(uint8 *pdata, uint32 len, uint32 flag)
{
    if (IOTGO_UPGRADE_STORE_FLAG_START == flag)
    {
        bin_buffer = SL_GetMemory(file_length);
        if (!bin_buffer)        
        {
            zzLogP("SL_GetMemory(file_length) failed");
        }
        else
        {
            zzLogP("SL_GetMemory(file_length) ok");
        }
        
        SL_Memset(bin_buffer, 0, file_length);
        SL_Memcpy(bin_buffer, pdata, len);
        bin_buffer_index = len;
    }
    else if (IOTGO_UPGRADE_STORE_FLAG_PROCEED == flag)
    {
        SL_Memcpy(bin_buffer + bin_buffer_index, pdata, len);
        bin_buffer_index += len;
    }
    else if (IOTGO_UPGRADE_STORE_FLAG_FINISH == flag)
    {
        SL_Memcpy(bin_buffer + bin_buffer_index, pdata, len);
        bin_buffer_index += len;
        if (file_length == bin_buffer_index)
        {
            zzLogP("store bin ok");
        }
        else
        {
            zzLogP("store bin err");
        }
        bin_buffer_index = 0;
    }
}


static void zzUpgradeHttpReqEventCb(ZZHttpReqHandle req_handle, ZZHttpReqEvent req_event)
{
    static uint32 body_len = 0;
    static bool flag_first_body = true;
    uint16 status = 0;
    uint32 cl = 0;
    ZZData head = ZZ_DATA_NULL;
    ZZData body = ZZ_DATA_NULL;
    //int32 ret = 0;
    
    switch (req_event)
    {
        case ZZ_HTTP_REQ_EVT_DNSERR: 
            zzLogI("ZZ_HTTP_REQ_EVT_DNSERR");
            finish(IOTGO_UPGRADE_ERROR_DOWNLOAD);
            
        break;
        case ZZ_HTTP_REQ_EVT_OPENERR: 
            zzLogI("ZZ_HTTP_REQ_EVT_OPENERR");
            finish(IOTGO_UPGRADE_ERROR_DOWNLOAD);
            
        break;
        case ZZ_HTTP_REQ_EVT_OPENED: 
            zzLogI("ZZ_HTTP_REQ_EVT_OPENED");
        break;
        case ZZ_HTTP_REQ_EVT_WRITTEN: 
            zzLogI("ZZ_HTTP_REQ_EVT_WRITTEN");
        break;
        case ZZ_HTTP_REQ_EVT_RES_BEGIN: 
            zzLogI("ZZ_HTTP_REQ_EVT_RES_BEGIN");
        break;
        case ZZ_HTTP_REQ_EVT_RES_HEAD: 
            zzLogI("ZZ_HTTP_REQ_EVT_RES_HEAD");
            status = zzHttpResGetStatus(req_handle);
            cl = zzHttpResGetContentLength(req_handle);
            head = zzHttpResGetHead(req_handle);
            zzLogI("HEAD = {status=%u, cl=%u, headlen=%u, data=[%s]}", 
                status,
                cl,
                head.length, 
                head.start);
            file_length = cl;
            flag_first_body = true;
            body_len = 0;

            if (file_length > IOTGO_UPGRADE_BIN_MAX_LEN)
            {
                zzLogE("file too large!");
                finish(IOTGO_UPGRADE_ERROR_DOWNLOAD);
            }
            
        break;
        case ZZ_HTTP_REQ_EVT_RES_BODY: 
            zzLogI("ZZ_HTTP_REQ_EVT_RES_BODY");
            body = zzHttpResGetBody(req_handle);
            
            body_len += body.length;
            zzLogI("%u, %u, %u (%u%%)", file_length, body_len, body.length, 
                (uint32)(100 * ((body_len * 1.0)/(file_length * 1.0))));
            
            if (flag_first_body)
            {
                flag_first_body = false;
                storeContentToBinBuffer(body.start, body.length, IOTGO_UPGRADE_STORE_FLAG_START);
            }
            else if (body_len == file_length)
            {
                storeContentToBinBuffer(body.start, body.length, IOTGO_UPGRADE_STORE_FLAG_FINISH);
            }
            else
            {
                storeContentToBinBuffer(body.start, body.length, IOTGO_UPGRADE_STORE_FLAG_PROCEED);
            }
            
        break;
        case ZZ_HTTP_REQ_EVT_RES_END: 
            zzLogI("ZZ_HTTP_REQ_EVT_RES_END");
            zzLogI("Download and store in flash successfully!");
            if (verifyBinDigest())
            {
                zzLogI("new bin verify ok");
                finish(IOTGO_UPGRADE_ERROR_OK);
            }
            else
            {
                zzLogI("new bin verify failed");
                finish(IOTGO_UPGRADE_ERROR_DIGEST);
            }
            
        break;
        case ZZ_HTTP_REQ_EVT_CLOSED: 
            zzLogI("ZZ_HTTP_REQ_EVT_CLOSED");
            finish(IOTGO_UPGRADE_ERROR_DOWNLOAD);
            
        break;
        case ZZ_HTTP_REQ_EVT_ABORTED: 
            zzLogI("ZZ_HTTP_REQ_EVT_ABORTED");
            finish(IOTGO_UPGRADE_ERROR_DOWNLOAD);
            
        break;
        case ZZ_HTTP_REQ_EVT_TIMEOUT: 
            zzLogI("ZZ_HTTP_REQ_EVT_TIMEOUT");
            finish(IOTGO_UPGRADE_ERROR_DOWNLOAD);
            
        break;
        default:
            zzLogE("unknown event(%d)", req_event);
        break;
    }
}


/*
host:[54.223.98.144]
port:[8088]
path:[/ota/rom/VMiYSCaBBVj0N9qwIAS2wyXMaDAqYKV2/user1.1024.new.2.bin]
sha256:[7d02a1f22fdcaa981f2e4fde3174405cff2f65ae651eb454043bbf356ad10d45]
sequence:[1488375893992]
*/

static void upgradeTask(void *pdata)
{
    SL_EVENT ev;
    uint32 error_code = 0;
    int32 ret = 0;
    char ts[20] = {0};
    uint8 digest[32] = {0};
    uint8 digest_hex[65] = {0};    
    sha256_context sha256_ctx;
    uint32 i = 0;
    SL_APP_UPDATE_STATUS update_status = SL_APP_UPDATE_OK;
    
    zzLogFB();    
    zzPrintCurrentTaskPrio();
    
    while (1)
    {
        SL_Memset(&ev, 0, sizeof(ev));
        SL_GetEvent(upgrade_task, &ev);
        switch (ev.nEventId)
        {
            case ZZ_EVT_UPGRADE_TASK_START: /* begin of upgrade procedure */
                zzLogI("ZZ_EVT_UPGRADE_TASK_START");

                file_length = 0;
                bin_buffer_index = 0;
                if (bin_buffer)
                {
                    SL_FreeMemory(bin_buffer);
                    bin_buffer = NULL;
                }
                
                upgrade_req = zzHttpReqCreate(IOTGO_UPGRADE_STREAM);
                zzHttpReqSetCallback(upgrade_req, zzUpgradeHttpReqEventCb);
                zzHttpReqSetMethod(upgrade_req, "GET");
                zzHttpReqSetHeader(upgrade_req, 0, "Host", "dl.itead.cn");
                zzHttpReqSetHeader(upgrade_req, 1, "User-Agent", "itead-device");
                zzHttpReqSetHeader(upgrade_req, 2, "Connection", "close");
                zzHttpReqSetArg(upgrade_req, 0, "deviceid", zzGetDeviceid());

                SL_Memset(digest, 0, sizeof(digest));
                SL_Memset(digest_hex, 0, sizeof(digest_hex));
                SL_Memset(ts, 0, sizeof(ts));
                
                SL_Sprintf(ts, "%u", SL_TmGetTick());
                
                sha256_starts(&sha256_ctx);
                sha256_update(&sha256_ctx, (uint8 *)zzGetDeviceid(), SL_Strlen(zzGetDeviceid()));
                sha256_update(&sha256_ctx, ts, SL_Strlen(ts));
                sha256_update(&sha256_ctx, (uint8 *)zzGetApikey(), SL_Strlen(zzGetApikey()));
                sha256_finish(&sha256_ctx, digest);
                
                for (i = 0; i < 64; i += 2)
                {
                    SL_Sprintf(&digest_hex[i], "%02x", digest[i/2]);
                }
                digest_hex[64] = 0;
                zzLogP("digest_hex = [%s]", digest_hex);

                zzHttpReqSetHostname(upgrade_req, upgrade_info.host);
                zzHttpReqSetPort(upgrade_req, upgrade_info.port);
                zzHttpReqSetPath(upgrade_req, upgrade_info.path);
                zzHttpReqSetArg(upgrade_req, 1, "ts", ts);
                zzHttpReqSetArg(upgrade_req, 2, "sign", digest_hex);

                ret = zzHttpReqSubmit(upgrade_req);
                zzLogIE(ret, "zzHttpReqSubmit upgrade req");
                
                SL_StartTimer(upgrade_task, 
                    IOTGO_UPGRADE_TIMER_DL, 
                    SL_TIMER_MODE_SINGLE, 
                    SL_SecondToTicks(IOTGO_UPGRADE_TIMEOUT));
                
            break;
            case ZZ_EVT_UPGRADE_TASK_STOP:  /* end of upgrade procedure */
                zzLogI("ZZ_EVT_UPGRADE_TASK_STOP");
                error_code = ev.nParam1;
                
                SL_StopTimer(upgrade_task, IOTGO_UPGRADE_TIMER_DL);                

                returnErrorCode2Ps(error_code, upgrade_info.sequence);
                
                if (IOTGO_UPGRADE_ERROR_OK == error_code)
                {
                    update_status = SL_AppUpdateInit(bin_buffer, file_length);
                    zzLogI("SL_AppUpdateInit update_status = %u", update_status);
                    if (SL_APP_UPDATE_OK == update_status)
                    {
                        zzLogW("system will powerdown after 5 seconds");
                        SL_StartTimer(upgrade_task, 
                            IOTGO_UPGRADE_TIMER_REBOOT, 
                            SL_TIMER_MODE_SINGLE, 
                            SL_SecondToTicks(5));
                    }
                }

                file_length = 0;
                bin_buffer_index = 0;
                if (bin_buffer)
                {
                    SL_FreeMemory(bin_buffer);
                    bin_buffer = NULL;
                }
                
                ret = zzHttpReqRelease(upgrade_req);
                zzLogIE(ret, "zzHttpReqRelease(upgrade_req)");

            break;
            case ZZ_EVT_UPGRADE_TASK_TIMER:
                zzLogI("ZZ_EVT_UPGRADE_TASK_TIMER");
                if (IOTGO_UPGRADE_TIMER_DL == ev.nParam1)
                {
                    zzLogI("download timeout");
                    finish(IOTGO_UPGRADE_ERROR_DOWNLOAD);
                }
                else if (IOTGO_UPGRADE_TIMER_REBOOT == ev.nParam1)
                {
                    zzLogP("now powerdown(0) for new bin");
                    SL_PowerDown(0); // reboot for new bin
                }
            break;
            default:
                zzLogE("unknown ev %u", ev.nEventId);
            break;
        }
    }
}

void iotgoUpgradeCreateUpgradeTask(void)
{
    upgrade_task.element[0] = SL_CreateTask(upgradeTask, 
        ZZ_UPGRADE_TASK_STACKSIZE, 
        ZZ_UPGRADE_TASK_PRIORITY,
        ZZ_UPGRADE_TASK_NAME);
        
    if (upgrade_task.element[0])
    {
        zzLogP("upgradeTask created ok");
    }
    else
    {
        zzLogP("upgradeTask created err");        
    }
    
    SL_Memset(&upgrade_info, 0, sizeof(upgrade_info));
}

