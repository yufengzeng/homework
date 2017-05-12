#include "zz_app_htask.h"
#include "zz_app_mtask.h"
#include "zz_http_req.h"
#include "zz_wsock.h"
#include "cJSON.h"
#include "zz_global.h"
#include "zz_timer.h"
#include "iotgo_upgrade.h"

#define ZZ_APP_MTASK_DS_STREAM              (ZZ_STREAM_1)
#define IOTGO_IOT_PROTOCOL_VERSION          (2)
#define IOTGO_DISTRUBUTOR_PROTOCOL_VERSION  (2)
#define IOTGO_FM_VERSION                    "1.1.0"

#define ZZ_DS_TIMER             (ZZ_APP_MTASK_TIMERID_BEGIN)
#define ZZ_WSOCK_TIMER          (ZZ_APP_MTASK_TIMERID_BEGIN + 1)
#define ZZ_LOGIN_TIMER          (ZZ_APP_MTASK_TIMERID_BEGIN + 2)

static ZZPinterval ds_timer_pi;
static ZZPinterval wsock_timer_pi;
static ZZPinterval login_timer_pi;



SL_TASK zz_app_mtask = {{0}};

static ZZHttpReqHandle req = 0;
static ZZDevice zz_device;
static char zz_seq[ZZ_SIZE_SEQUENCE] = {0};

void zzSetDeviceid(const char *deviceid)
{
    SL_Strcpy(zz_device.deviceid, deviceid);
}

const char * zzGetDeviceid(void)
{
    return zz_device.deviceid;
}

const char * zzGetApikey(void)
{
    return zz_device.apikey;
}

const char * zzGetModel(void)
{
    return zz_device.model;
}


void zzSetApikey(const char *apikey)
{
    SL_Strcpy(zz_device.apikey, apikey);
}

void zzSetModel(const char *model)
{
    SL_Strcpy(zz_device.model, model);
}


/*
{"error":0,"reason":"ok","IP":"54.223.229.90","port":444}
*/
static bool parsePsIpAndPort(const char *json_str)
{
    cJSON *json_root = NULL;
    cJSON *json_error = NULL;
    cJSON *json_ip = NULL;
    cJSON *json_port = NULL;
    
    json_root = cJSON_Parse(json_str);
    if (!json_root)
    {
        zzLogE("parse json failed");
        return false;
    }

    json_error = cJSON_GetObjectItem(json_root, "error");
    json_ip = cJSON_GetObjectItem(json_root, "IP");
    json_port = cJSON_GetObjectItem(json_root, "port");

    if (!json_error || json_error->valueint || !json_ip || !json_port)
    {
        zzLogE("fields lost");
        goto ExitErr1;
    }

    SL_Strcpy(zz_device.ps_ip, json_ip->valuestring);
    zz_device.ps_port = json_port->valueint;

    zzLogI("got PS [%s/%u]", zz_device.ps_ip, zz_device.ps_port);

    cJSON_Delete(json_root);
    return true;

    
ExitErr1:
    cJSON_Delete(json_root);
    return false;
}

static void zzDsHttpReqEventCb(ZZHttpReqHandle req_handle, ZZHttpReqEvent req_event)
{
    static uint32 body_len = 0;
    uint16 status = 0;
    uint32 cl = 0;
    ZZData head = ZZ_DATA_NULL;
    ZZData body = ZZ_DATA_NULL;
    
    switch (req_event)
    {
        case ZZ_HTTP_REQ_EVT_DNSERR: 
            zzLogI("ZZ_HTTP_REQ_EVT_DNSERR");
            zzEmitEvt(zz_app_mtask, ZZ_EVT_APP_MTASK_GET_PS_ERR);
            
        break;
        case ZZ_HTTP_REQ_EVT_OPENERR: 
            zzLogI("ZZ_HTTP_REQ_EVT_OPENERR");
            zzEmitEvt(zz_app_mtask, ZZ_EVT_APP_MTASK_GET_PS_ERR);
            
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
            body_len = 0;
        break;
        case ZZ_HTTP_REQ_EVT_RES_BODY: 
            zzLogI("ZZ_HTTP_REQ_EVT_RES_BODY");
            cl = zzHttpResGetContentLength(req_handle);
            body = zzHttpResGetBody(req_handle);
            body_len += body.length;
            zzLogI("%u, %u, %u [%s]", cl, body_len, body.length, body.start);
            if (cl == body_len)
            {
                if(parsePsIpAndPort(body.start))
                {
                    zzEmitEvt(zz_app_mtask, ZZ_EVT_APP_MTASK_GET_PS_OK);
                }
                else
                {
                    zzEmitEvt(zz_app_mtask, ZZ_EVT_APP_MTASK_GET_PS_ERR);
                }
            }
        break;
        case ZZ_HTTP_REQ_EVT_RES_END: 
            zzLogI("ZZ_HTTP_REQ_EVT_RES_END");            
        break;
        case ZZ_HTTP_REQ_EVT_CLOSED: 
            zzLogI("ZZ_HTTP_REQ_EVT_CLOSED");
            zzEmitEvt(zz_app_mtask, ZZ_EVT_APP_MTASK_GET_PS_ERR);
            
        break;
        case ZZ_HTTP_REQ_EVT_ABORTED: 
            zzLogI("ZZ_HTTP_REQ_EVT_ABORTED");
            zzEmitEvt(zz_app_mtask, ZZ_EVT_APP_MTASK_GET_PS_ERR);
            
        break;
        case ZZ_HTTP_REQ_EVT_TIMEOUT: 
            zzLogI("ZZ_HTTP_REQ_EVT_TIMEOUT");
            zzEmitEvt(zz_app_mtask, ZZ_EVT_APP_MTASK_GET_PS_ERR);
            
        break;
        default:
            zzLogE("unknown event(%d)", req_event);
        break;
    }
}


static void initDsReq(void)
{
    ZZData body = ZZ_DATA_NULL;
    char body_len_str[10] = {0};

    zzHttpReqSetCallback(req, zzDsHttpReqEventCb);
    zzHttpReqSetHostname(req, zz_device.ds_hostname);
    zzHttpReqSetPort(req, zz_device.ds_port);
    zzHttpReqSetMethod(req, "POST");
    zzHttpReqSetPath(req, "/dispatch/device");
    zzHttpReqSetHeader(req, 0, "Host", zz_device.ds_hostname);
    zzHttpReqSetHeader(req, 1, "Content-Type", "application/json");
    zzHttpReqSetHeader(req, 2, "Connection", "close");
    zzHttpReqSetTimeout(req, 15);
    
    body = zzDataCreate(512);
    SL_Sprintf(body.start,
        "{"
            "\"accept\":\"ws;%u\","
            "\"version\":%u,"
            "\"ts\":%u,"
            "\"deviceid\":\"%s\","
            "\"apikey\":\"%s\","
            "\"model\":\"%s\","
            "\"romVersion\":\"%s\""
        "}",
        IOTGO_IOT_PROTOCOL_VERSION,
        IOTGO_DISTRUBUTOR_PROTOCOL_VERSION,
        SL_TmGetTick() % 65536,
        zz_device.deviceid,
        zz_device.apikey,
        zz_device.model,
        IOTGO_FM_VERSION
        );
    body.length = SL_Strlen(body.start);

    SL_Sprintf(body_len_str, "%u", body.length);

    zzLogP("body = {%s}", body.start);

    zzHttpReqSetHeader(req, 3, "Content-Length", body_len_str);    
    zzHttpReqSetBody(req, body);

    zzDataFree(&body);
}

/*
 * @param uuid - apikey from iot pkg received. 
 */
static bool isLogined(void)
{
    if (SL_Strlen(zz_device.apikey) == (ZZ_SIZE_APIKEY - 1)
        && SL_Strlen(zz_device.uuid) == (ZZ_SIZE_UUID - 1)
        && SL_Strcmp(zz_device.apikey, zz_device.uuid) != 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/*
{
"action":"update",
"deviceid":"100007eea0",
"apikey":"e73d89fe-d418-4c62-b49c-545661653185",
"userAgent":"app",
"sequence":"1490932033623",
"ts":0,
"params":{"switch":"on"},
"from":"app"
}
*/

static ZZIotPkgType parseIotPkgType(const char *json_str)
{
    ZZIotPkgType pkg_type = ZZ_IOT_PKG_TYPE_INVALID;
    
    cJSON *json_root = NULL;
    cJSON *json_error     = NULL;
    cJSON *json_apikey    = NULL;
    cJSON *json_deviceid  = NULL;
    cJSON *json_action    = NULL;
    cJSON *json_params    = NULL;
    cJSON *json_date      = NULL;

    json_root = cJSON_Parse(json_str);
    if (!json_root)
    {
        zzLogE("parse json failed");
        return pkg_type;
    }

    json_error = cJSON_GetObjectItem(json_root, "error");
    json_action = cJSON_GetObjectItem(json_root, "action");
    json_apikey = cJSON_GetObjectItem(json_root, "apikey");
    json_deviceid = cJSON_GetObjectItem(json_root, "deviceid");
    json_params = cJSON_GetObjectItem(json_root, "params");
    json_date = cJSON_GetObjectItem(json_root, "date");

    if (!json_deviceid || 0 != SL_Strcmp(json_deviceid->valuestring, zz_device.deviceid))
    {
        zzLogE("Fatal error: bad deviceid");
        goto ExitErr1;
    }

    if (json_action) /* request from PS */
    {
        if (0 == SL_Strcmp(json_action->valuestring, "update")) /* update */
        {
            pkg_type = ZZ_IOT_PKG_TYPE_REQ_OF_UPDATE;
        }
        else if (0 == SL_Strcmp(json_action->valuestring, "upgrade")) /* upgrade */
        {
            pkg_type = ZZ_IOT_PKG_TYPE_REQ_OF_UPGRADE;
        }
        else if (0 == SL_Strcmp(json_action->valuestring, "restart")) /* restart */
        {
            pkg_type = ZZ_IOT_PKG_TYPE_REQ_OF_RESTART;
        }
        else if (0 == SL_Strcmp(json_action->valuestring, "redirect")) /* redirect */
        {
            pkg_type = ZZ_IOT_PKG_TYPE_REQ_OF_REDIRECT;        
        }
        else if (0 == SL_Strcmp(json_action->valuestring, "notify")) /* notify */
        {
            pkg_type = ZZ_IOT_PKG_TYPE_REQ_OF_NOTIFY;
        }
        else if (0 == SL_Strcmp(json_action->valuestring, "queryDev")) /* query */
        {
            pkg_type = ZZ_IOT_PKG_TYPE_REQ_OF_QUERY;
        }
    }
    else if (json_error && (0 == json_error->valueint)) /* response from PS */
    {
        if (!isLogined() && !json_params) /* res of login (register) */
        {
            pkg_type = ZZ_IOT_PKG_TYPE_RES_OF_LOGIN;
        }
        else if (!json_params && !json_date) /* res of update */
        {
            pkg_type = ZZ_IOT_PKG_TYPE_RES_OF_UPDATE;
        }
        else if (json_date) /* res of date */
        {
            pkg_type = ZZ_IOT_PKG_TYPE_RES_OF_DATE;
        }
        else if (json_params) /* res of query */
        {
            pkg_type = ZZ_IOT_PKG_TYPE_RES_OF_QUERY;
        }
    }
    else if (json_error && (0 != json_error->valueint)) /* bad response from PS */
    {
        /* relogin */
        zzLogI("bad res from PS(%d)", json_error->valueint);
        pkg_type = ZZ_IOT_PKG_TYPE_BAD_RESPONSE;
    }
    else /* invalid pkg from PS */
    {
        /* reconnect(wsock restart) */
        zzLogE("invalid pkg from PS");
        pkg_type = ZZ_IOT_PKG_TYPE_INVALID;
    }
    
    cJSON_Delete(json_root);
    zzLogI("pkg type = %u", pkg_type);
    return pkg_type;

ExitErr1:
    cJSON_Delete(json_root);
    return ZZ_IOT_PKG_TYPE_INVALID;
}


static bool login(void)
{
    cJSON *json_root = NULL;
    char *raw = NULL;
    int32 ret = 0;

    zzMemzero(zz_device.uuid, sizeof(zz_device.uuid));
    
    json_root = cJSON_CreateObject();

    cJSON_AddStringToObject(json_root, "userAgent","device");
    cJSON_AddNumberToObject(json_root, "version", IOTGO_IOT_PROTOCOL_VERSION);
    cJSON_AddStringToObject(json_root, "romVersion", IOTGO_FM_VERSION);
    cJSON_AddStringToObject(json_root, "action","register");
    cJSON_AddStringToObject(json_root, "model", zz_device.model);
    cJSON_AddStringToObject(json_root, "apikey", zz_device.apikey);
    cJSON_AddStringToObject(json_root, "deviceid", zz_device.deviceid);
    cJSON_AddNumberToObject(json_root, "ts", SL_TmGetTick() % 65536);    

    raw = cJSON_Print(json_root);
    cJSON_Delete(json_root);
    
    if (!raw)
    {
        zzLogE("cJSON_Print failed");
        return false;
    }

    ret = zzWsockWriteText(raw);
    SL_FreeMemory(raw);
    
    if (ret)
    {
        zzLogE("zzWsockWriteText failed(%d)", ret);
        return false;
    }
    
    return true;
}

/*
{
"error":0,
"deviceid":"100007eea0",
"apikey":"e73d89fe-d418-4c62-b49c-545661653185",
"config":{"hb":1,"hbInterval":145}
}
*/
static bool procResponseOfLogin(const char *json_str)
{
    cJSON *json_root = NULL;
    cJSON *json_error = NULL;
    cJSON *json_uuid = NULL;
    cJSON *json_config = NULL;
    cJSON *json_hb = NULL;
    cJSON *json_hbi = NULL;
    
    json_root = cJSON_Parse(json_str);
    if (!json_root)
    {
        zzLogE("parse json failed");
        return false;
    }

    json_error = cJSON_GetObjectItem(json_root, "error");
    json_uuid = cJSON_GetObjectItem(json_root, "apikey");
    json_config = cJSON_GetObjectItem(json_root, "config");
    
    json_hb = cJSON_GetObjectItem(json_config, "hb");
    json_hbi = cJSON_GetObjectItem(json_config, "hbInterval");
    
    if (!json_error || json_error->valueint || !json_uuid)
    {
        zzLogE("fields lost");
        goto ExitErr1;
    }

    SL_Strcpy(zz_device.uuid, json_uuid->valuestring);
    zzLogI("got uuid [%s]", zz_device.uuid);

    if (json_hb)
    {
        if (json_hb->valueint)
        {
            zzWsockEnableHeartbeat();
            zzLogP("hb enabled");
        }
        else
        {
            zzWsockDisableHeartbeat();
            zzLogP("hb disabled");
        }
    }

    if (json_hbi)
    {
        zzWsockSetHeartbeat(json_hbi->valueint);
        zzLogP("hbi = %u", json_hbi->valueint);
    }

    zzEmitEvt(zz_app_mtask, ZZ_EVT_APP_MTASK_LOGIN_OK);
    
    cJSON_Delete(json_root);
    return true;

    
ExitErr1:
    cJSON_Delete(json_root);
    return false;
}

static bool procRequestOfRedirect(const char *json_str)
{
    cJSON *json_root = NULL;
    cJSON *IP = NULL;
    cJSON *seq = NULL;
    
    json_root = cJSON_Parse(json_str);
    if (!json_root)
    {
        zzLogE("parse json failed");
        return false;
    }
    
    seq = cJSON_GetObjectItem(json_root, "sequence");
    if (seq)
    {
        SL_Strcpy(zz_seq, seq->valuestring);
    }

    IP = cJSON_GetObjectItem(json_root, "IP");
    if (!IP)
    {
        zzLogE("bad IP");
        goto ExitErr1;
    }

    SL_Strcpy(zz_device.ps_ip, IP->valuestring);
    zzLogI("redirect to [%s]", zz_device.ps_ip);
    
    zzEmitEvt(zz_app_mtask, ZZ_EVT_APP_MTASK_REDIRECT_OK);

    cJSON_Delete(json_root);
    return true;

    
ExitErr1:
    cJSON_Delete(json_root);
    return false;

}


static bool procRequestOfUpdate(const char *json_str)
{
    cJSON *json_root = NULL;
    cJSON *params = NULL;
    cJSON *seq = NULL;
    cJSON *field_switch = NULL;
    cJSON *field_startup = NULL;
    cJSON *field_timers = NULL;
    
    json_root = cJSON_Parse(json_str);
    if (!json_root)
    {
        zzLogE("parse json failed");
        return false;
    }

    params = cJSON_GetObjectItem(json_root, "params");
    if (!params)
    {
        zzLogE("bad params");
        goto ExitErr1;
    }

    seq = cJSON_GetObjectItem(json_root, "sequence");
    if (seq)
    {
        SL_Strcpy(zz_seq, seq->valuestring);
    }
    
    field_switch = cJSON_GetObjectItem(params, "switch");
    field_startup = cJSON_GetObjectItem(params, "startup");
    field_timers = cJSON_GetObjectItem(params, "timers");

    if (field_switch)
    {
        if (0 == SL_Strcmp(field_switch->valuestring, "on"))
        {
            zzEmitEvt(zz_app_htask, ZZ_EVT_APP_HTASK_RELAY_ON);
        }
        else
        {
            zzEmitEvt(zz_app_htask, ZZ_EVT_APP_HTASK_RELAY_OFF);
        }
    }

    if (field_startup)
    {
        zzRelaySetDef(field_startup->valuestring);
        zzLogI("proc startup ok");    
    }

    if (field_timers)
    {
        if (zzTimerProc(params))
        {
            zzLogI("proc timer ok");       
        }
        else
        {
            zzLogE("proc timer err");       
        }
    }

    zzEmitEvt(zz_app_mtask, ZZ_EVT_APP_MTASK_UPDATE_BY_PS_OK);

    cJSON_Delete(json_root);
    return true;

    
ExitErr1:
    cJSON_Delete(json_root);
    return false;
}

static bool verifyGMTTime(IoTgoGMTTime *gmt_time)
{
    if (!gmt_time)
    {
        return false;
    }
    
    if (gmt_time->year > 2000
        && (gmt_time->month >= 1 && gmt_time->month <= 12)
        && (gmt_time->day >= 1 && gmt_time->day <= 31)
        && (gmt_time->hour >= 0 && gmt_time->hour <= 23)
        && (gmt_time->minute >= 0 && gmt_time->minute <= 59)
        && (gmt_time->second >= 0 && gmt_time->second <= 59)
        )
    {
        return true;
    }
    return false;
}


/* 
2015-02-27T06:52:53.325Z

year:0-3
::4
month: 5-6
::7
day:8-9
T:10
hour:11-12
::13
minute:14-15
::16
second:17-18
Z:23
*/
static bool parseGMTTimeFromString(const char *str, IoTgoGMTTime *gmt_time)
{
    char temp[24] = {0};
    if (gmt_time && str)
    {
        if (SL_Strlen(str) == 24
            && str[10] == 'T'
            && str[23] == 'Z' )
        {
            zzMemzero(temp, sizeof(temp));
            SL_Strncpy(temp, str + 0, 4);
            gmt_time->year = SL_atoi(temp);
            
            zzMemzero(temp, sizeof(temp));
            SL_Strncpy(temp, str + 5, 2);
            gmt_time->month = SL_atoi(temp);
            
            zzMemzero(temp, sizeof(temp));
            SL_Strncpy(temp, str + 8, 2);
            gmt_time->day = SL_atoi(temp);
            
            zzMemzero(temp, sizeof(temp));
            SL_Strncpy(temp, str + 11, 2);
            gmt_time->hour = SL_atoi(temp);
            
            zzMemzero(temp, sizeof(temp));
            SL_Strncpy(temp, str + 14, 2);
            gmt_time->minute = SL_atoi(temp);
            
            zzMemzero(temp, sizeof(temp));
            SL_Strncpy(temp, str + 17, 2);
            gmt_time->second = SL_atoi(temp);
            
            if (verifyGMTTime(gmt_time))
            {
                return true;
            }
            else
            {
                zzLogE("Bad IoTgoGMTTime");
                return false;
            }
        }
    }
    
    return false;
}


/*
"date":"2017-03-31T09:07:06.287Z"

*/
static bool procResponseOfDate(const char *json_str)
{
    cJSON *json_root = NULL;
    cJSON *date = NULL;
    IoTgoGMTTime gmt = {0};
    SL_SYSTEMTIME sys_time = {0};
    
    json_root = cJSON_Parse(json_str);
    if (!json_root)
    {
        zzLogE("parse json failed");
        return false;
    }
    
    date = cJSON_GetObjectItem(json_root, "date");
    if (!date)
    {
        zzLogE("bad date");
        goto ExitErr1;
    }

    if (!parseGMTTimeFromString(date->valuestring, &gmt))
    {
        zzLogE("parseGMTTimeFromString failed!");
        goto ExitErr1;
    }

    sys_time.uYear = gmt.year;
    sys_time.uMonth = gmt.month;
    sys_time.uDay = gmt.day;
    sys_time.uHour = gmt.hour;
    sys_time.uMinute = gmt.minute;
    sys_time.uSecond = gmt.second;
    
    SL_SetTimeZone(0);
    SL_SetLocalTime(&sys_time);
    
    zzEmitEvt(zz_app_mtask, ZZ_EVT_APP_MTASK_DATE_TO_PS_OK);

    cJSON_Delete(json_root);
    return true;

    
ExitErr1:
    cJSON_Delete(json_root);
    return false;
}


static bool procResponseOfQueryTimerList(const char *json_str)
{
    cJSON *json_root = NULL;
    cJSON *params = NULL;
    cJSON *seq = NULL;
    cJSON *field_timers = NULL;
    
    json_root = cJSON_Parse(json_str);
    if (!json_root)
    {
        zzLogE("parse json failed");
        return false;
    }

    params = cJSON_GetObjectItem(json_root, "params");
    if (!params)
    {
        zzLogE("bad params");
        goto ExitErr1;
    }

    seq = cJSON_GetObjectItem(json_root, "sequence");
    if (seq)
    {
        SL_Strcpy(zz_seq, seq->valuestring);
    }
    
    field_timers = cJSON_GetObjectItem(params, "timers");

    if (field_timers)
    {
        if (zzTimerProc(params))
        {
            zzLogI("proc timer ok");       
        }
        else
        {
            zzLogE("proc timer err");       
        }
    }

    zzEmitEvt(zz_app_mtask, ZZ_EVT_APP_MTASK_QUERY_TO_PS_OK);

    cJSON_Delete(json_root);
    return true;

    
ExitErr1:
    cJSON_Delete(json_root);
    return false;

}

static void zzPsWsockEventCb(ZZWsockEvent ws_ev)
{  
    //int32 ret = 0;
    
    switch (ws_ev)
    {
        case ZZ_WSOCK_EVT_DNSERR: 
            zzLogI("ZZ_WSOCK_EVT_DNSERR");
            zzEmitEvt(zz_app_mtask, ZZ_EVT_APP_MTASK_WSOCK_ERR);
                        
        break;
        case ZZ_WSOCK_EVT_OPENERR: 
            zzLogI("ZZ_WSOCK_EVT_OPENERR");
            zzEmitEvt(zz_app_mtask, ZZ_EVT_APP_MTASK_WSOCK_ERR);
                        
        break;
        case ZZ_WSOCK_EVT_OPENED: 
            zzLogI("ZZ_WSOCK_EVT_OPENED");
                        
        break;
        case ZZ_WSOCK_EVT_STARTED: 
            zzLogI("ZZ_WSOCK_EVT_STARTED");
            zzEmitEvt(zz_app_mtask, ZZ_EVT_APP_MTASK_WSOCK_START_OK);
#if 0            
            ping.type = ZZ_WSOCK_FRAME_PING;            
            ret = zzWsockWrite(ping);
            if (ret < 0)
            {
                zzLogE("zzWsockWrite err(%d)", ret);
            }
#endif            
        break;
        case ZZ_WSOCK_EVT_WRITTEN: 
            zzLogI("ZZ_WSOCK_EVT_WRITTEN");
                        
        break;
        case ZZ_WSOCK_EVT_READ: 
            zzLogI("ZZ_WSOCK_EVT_READ");
            ZZWsockFrame frame = zzWsockRead();
            zzLogI("frame.type = %u, frame.payload.len = %u", frame.type, frame.payload.length);
            zzLogP("frame.payload.start = [%s]", frame.payload.start);

            ZZIotPkgType pkg_type = parseIotPkgType(frame.payload.start);
            if (ZZ_IOT_PKG_TYPE_RES_OF_LOGIN == pkg_type)
            {
                procResponseOfLogin(frame.payload.start);
            }
            else if (ZZ_IOT_PKG_TYPE_RES_OF_UPDATE == pkg_type)
            {
                zzEmitEvt(zz_app_mtask, ZZ_EVT_APP_MTASK_UPDATE_TO_PS_OK);
            }
            else if (ZZ_IOT_PKG_TYPE_RES_OF_QUERY == pkg_type)
            {
                procResponseOfQueryTimerList(frame.payload.start);
            }
            else if (ZZ_IOT_PKG_TYPE_RES_OF_DATE == pkg_type)
            {
                procResponseOfDate(frame.payload.start);
            }
            else if (ZZ_IOT_PKG_TYPE_REQ_OF_UPDATE == pkg_type)
            {
                procRequestOfUpdate(frame.payload.start);
            }
            else if (ZZ_IOT_PKG_TYPE_REQ_OF_QUERY == pkg_type)
            {
                //procRequestOfQuery(frame.payload.start);
            }
            else if (ZZ_IOT_PKG_TYPE_REQ_OF_UPGRADE == pkg_type)
            {
                iotgoUpgradeParseUpgrade(frame.payload.start);
            }
            else if (ZZ_IOT_PKG_TYPE_REQ_OF_REDIRECT == pkg_type)
            {
                procRequestOfRedirect(frame.payload.start);
            }
            else if (ZZ_IOT_PKG_TYPE_REQ_OF_NOTIFY == pkg_type)
            {
                //procRequestOfNotify(frame.payload.start);
            }
            else if (ZZ_IOT_PKG_TYPE_REQ_OF_RESTART == pkg_type)
            {
                //procRequestOfRestart(frame.payload.start);
            }
            else if (ZZ_IOT_PKG_TYPE_BAD_RESPONSE == pkg_type)
            {
                zzEmitEvt(zz_app_mtask, ZZ_EVT_APP_MTASK_LOGIN_ERR);
            }
            else if (ZZ_IOT_PKG_TYPE_INVALID == pkg_type)
            {
                zzEmitEvt(zz_app_mtask, ZZ_EVT_APP_MTASK_WSOCK_ERR);
            }
            
        break;
        case ZZ_WSOCK_EVT_STOPPED: 
            zzLogI("ZZ_WSOCK_EVT_STOPPED");
            zzEmitEvt(zz_app_mtask, ZZ_EVT_APP_MTASK_WSOCK_ERR);
            
        break;
        case ZZ_WSOCK_EVT_CLOSED: 
            zzLogI("ZZ_WSOCK_EVT_CLOSED");
            zzEmitEvt(zz_app_mtask, ZZ_EVT_APP_MTASK_WSOCK_ERR);
            
        break;
        case ZZ_WSOCK_EVT_ABORTED: 
            zzLogI("ZZ_WSOCK_EVT_ABORTED");
            zzEmitEvt(zz_app_mtask, ZZ_EVT_APP_MTASK_WSOCK_ERR);
                        
        break;
        default:
            zzLogE("unknown event(%d)", ws_ev);
        break;
    }
}


static void initDevice(void)
{
    SL_Strcpy(zz_device.ds_hostname, ZZ_IOT_DS_DOMAIN_NAME);    
    zz_device.ds_port = 8081;
}

static bool update2Ps(void)
{
    cJSON *json_root = NULL;
    cJSON *params = NULL;
    char *raw = NULL;
    int32 ret = 0;
    
    json_root = cJSON_CreateObject();
    params = cJSON_CreateObject();
    
    cJSON_AddStringToObject(json_root, "userAgent", "device");
    cJSON_AddStringToObject(json_root, "action", "update");
    cJSON_AddStringToObject(json_root, "apikey", zz_device.uuid);
    cJSON_AddStringToObject(json_root, "deviceid", zz_device.deviceid);
    cJSON_AddItemToObject(json_root, "params", params);
    
    cJSON_AddStringToObject(params, "fwVersion", IOTGO_FM_VERSION);
    cJSON_AddNumberToObject(params, "gsm_rssi", zzNetGetRssi());
    cJSON_AddStringToObject(params, "startup", zzRelayGetDef());
    
    if (zzRelayIsOn())
    {
        cJSON_AddStringToObject(params, "switch", "on");
    }
    else
    {
        cJSON_AddStringToObject(params, "switch", "off");
    }

    
    raw = cJSON_Print(json_root);
    cJSON_Delete(json_root);
    
    if (!raw)
    {
        zzLogE("cJSON_Print failed");
        return false;
    }

    ret = zzWsockWriteText(raw);
    SL_FreeMemory(raw);
    
    if (ret)
    {
        zzLogE("zzWsockWriteText failed(%d)", ret);
        return false;
    }
    
    return true;
    
}

void returnErrorCode2Ps(int32 err_code, const char *seq)
{
    cJSON *root = NULL;
    char *raw = NULL;
    int32 ret = 0;
   
    if (!seq)
    {
        zzLogE(ZZ_ERR_STR_INP);
        return;
    }
    
    root = cJSON_CreateObject();
    if(!root)
    {
        zzLogE("create cjson object is error");
        return;
    }
    
    cJSON_AddStringToObject(root, "userAgent", "device");
    cJSON_AddStringToObject(root, "apikey", zz_device.uuid);
    cJSON_AddStringToObject(root, "deviceid", zz_device.deviceid);
    cJSON_AddNumberToObject(root, "error", err_code);
    cJSON_AddStringToObject(root, "sequence", seq);
    
    raw = cJSON_Print(root);
    cJSON_Delete(root);
    
    ret = zzWsockWriteText(raw);
    SL_FreeMemory(raw);
    
    if (ret)
    {
        zzLogE("zzWsockWriteText failed(%d)", ret);
    }
}

static void getDateFromPs(void)
{
    cJSON *root = NULL;
    char *raw = NULL;
    int32 ret = 0;
    
    root = cJSON_CreateObject();
    if(!root)
    {
        zzLogE("create cjson object is error");
        return;
    }
    
    cJSON_AddStringToObject(root, "userAgent", "device");
    cJSON_AddStringToObject(root, "apikey", zz_device.uuid);
    cJSON_AddStringToObject(root, "deviceid", zz_device.deviceid);
    cJSON_AddStringToObject(root, "action", "date");
    
    raw = cJSON_Print(root);
    cJSON_Delete(root);
    
    ret = zzWsockWriteText(raw);
    SL_FreeMemory(raw);
    
    if (ret)
    {
        zzLogE("zzWsockWriteText failed(%d)", ret);
    }

}

static void getTimerListFromPs(void)
{
    cJSON *root = NULL;
    cJSON *params = NULL;
    char *raw = NULL;
    int32 ret = 0;
    
    root = cJSON_CreateObject();
    params = cJSON_CreateArray();
    if(!root || !params)
    {
        zzLogE("create cjson error");
        return;
    }

    cJSON_AddStringToObject(root, "userAgent", "device");
    cJSON_AddStringToObject(root, "apikey", zz_device.uuid);
    cJSON_AddStringToObject(root, "deviceid", zz_device.deviceid);
    cJSON_AddStringToObject(root, "action", "query");
    cJSON_AddItemToObject(root, "params", params);
    cJSON_AddItemToArray(params, cJSON_CreateString("timers"));
    
    raw = cJSON_Print(root);
    cJSON_Delete(root);
    
    ret = zzWsockWriteText(raw);
    SL_FreeMemory(raw);
    
    if (ret)
    {
        zzLogE("zzWsockWriteText failed(%d)", ret);
    }
}

static void stopRetryDsReqTimer(void)
{
    zzPintReset(&ds_timer_pi);
    SL_StopTimer(zz_app_mtask, ZZ_DS_TIMER);
}

static void startRetryDsReqTimer(void)
{
    uint32 delay = 0;
    delay = zzPintVal(&ds_timer_pi);
    zzLogP("ds req retry after %u seconds", delay);
    SL_StopTimer(zz_app_mtask, ZZ_DS_TIMER);
    SL_StartTimer(zz_app_mtask, 
        ZZ_DS_TIMER, 
        0, 
        SL_SecondToTicks(delay));
}

static void stopRetryWsockTimer(void)
{
    zzPintReset(&wsock_timer_pi);
    SL_StopTimer(zz_app_mtask, ZZ_WSOCK_TIMER);
}

static void startRetryWsockTimer(void)
{
    uint32 delay = 0;
    delay = zzPintVal(&wsock_timer_pi);
    zzLogP("wsock retry after %u seconds", delay);

    SL_StopTimer(zz_app_mtask, ZZ_WSOCK_TIMER);
    SL_StartTimer(zz_app_mtask, 
        ZZ_WSOCK_TIMER, 
        0, 
        SL_SecondToTicks(delay));
}

static void stopRetryLoginTimer(void)
{
    zzPintReset(&login_timer_pi);
    SL_StopTimer(zz_app_mtask, ZZ_LOGIN_TIMER);
}

static void startRetryLoginTimer(void)
{
    uint32 delay = 0;
    delay = zzPintVal(&login_timer_pi);
    zzLogP("login retry after %u seconds", delay);

    SL_StopTimer(zz_app_mtask, ZZ_LOGIN_TIMER);
    SL_StartTimer(zz_app_mtask, 
        ZZ_LOGIN_TIMER, 
        0, 
        SL_SecondToTicks(delay));
}


static void zzAppMtask (void *pdata)
{
    SL_EVENT ev = {0};
    int32 ret = 0;
    
    zzLogFB();    
    zzPrintCurrentTaskPrio();
    initDevice();
    
    while (1)
    {
        SL_Memset(&ev, 0, sizeof(ev));
        SL_GetEvent(zz_app_mtask, &ev);
        switch (ev.nEventId)
        {
            case ZZ_EVT_APP_MTASK_START:
                zzLogI("ZZ_EVT_APP_MTASK_START");                
                stopRetryDsReqTimer();
                zzEmitEvt(zz_app_mtask, ZZ_EVT_APP_MTASK_GET_PS);
            
            break;
            case ZZ_EVT_APP_MTASK_GET_PS:
                SL_Sleep(100);
                zzLogI("ZZ_EVT_APP_MTASK_GET_PS");    
                iotgoSetDeviceMode(ZZ_DEVICE_STATE_CONN);
                req = zzHttpReqCreate(ZZ_APP_MTASK_DS_STREAM);
                initDsReq();
                zzHttpReqSubmit(req);
                
            break;
            case ZZ_EVT_APP_MTASK_GET_PS_OK:
                SL_Sleep(100);
                zzLogI("ZZ_EVT_APP_MTASK_GET_PS_OK");
                stopRetryDsReqTimer();
                ret = zzHttpReqRelease(req);
                zzLogIE(ret, "zzHttpReqRelease");
                
                stopRetryWsockTimer();
                zzEmitEvt(zz_app_mtask, ZZ_EVT_APP_MTASK_WSOCK_START);
                
            break;
            case ZZ_EVT_APP_MTASK_GET_PS_ERR:
                SL_Sleep(100);
                zzLogI("ZZ_EVT_APP_MTASK_GET_PS_ERR");
                ret = zzHttpReqRelease(req);
                zzLogIE(ret, "zzHttpReqRelease");
                startRetryDsReqTimer();
                
            break;
            case ZZ_EVT_APP_MTASK_WSOCK_START:
                SL_Sleep(100);
                zzLogI("ZZ_EVT_APP_MTASK_WSOCK_START");
                iotgoSetDeviceMode(ZZ_DEVICE_STATE_CONN);
                zzWsockSetCallback(zzPsWsockEventCb);
                zzWsockSetRemote(zz_device.ps_ip, 8081);
                zzWsockStart();
                
            break;
            case ZZ_EVT_APP_MTASK_WSOCK_START_OK:
                SL_Sleep(100);
                zzLogI("ZZ_EVT_APP_MTASK_WSOCK_START_OK");
                stopRetryWsockTimer();
                
                stopRetryLoginTimer();
                zzEmitEvt(zz_app_mtask, ZZ_EVT_APP_MTASK_LOGIN);
                
            break;
            case ZZ_EVT_APP_MTASK_WSOCK_ERR:
                SL_Sleep(100);
                zzLogI("ZZ_EVT_APP_MTASK_WSOCK_ERR");
                zzWsockStop();
                startRetryWsockTimer();

            break;
            case ZZ_EVT_APP_MTASK_LOGIN:
                SL_Sleep(100);
                zzLogI("ZZ_EVT_APP_MTASK_LOGIN");
                iotgoSetDeviceMode(ZZ_DEVICE_STATE_LOGIN);
                login();
                
            break;
            case ZZ_EVT_APP_MTASK_LOGIN_OK:
                SL_Sleep(100);
                zzLogI("ZZ_EVT_APP_MTASK_LOGIN_OK");
                iotgoSetDeviceMode(ZZ_DEVICE_STATE_LOGINED);
                stopRetryLoginTimer();
                zzEmitEvt(zz_app_mtask, ZZ_EVT_APP_MTASK_DATE_TO_PS);
                
            break;
            case ZZ_EVT_APP_MTASK_LOGIN_ERR:
                SL_Sleep(100);
                zzLogI("ZZ_EVT_APP_MTASK_LOGIN_ERR");
                startRetryLoginTimer();
                
            break;            
            case ZZ_EVT_APP_MTASK_UPDATE_TO_PS:
                SL_Sleep(100);
                zzLogI("ZZ_EVT_APP_MTASK_UPDATE_TO_PS");
                update2Ps();
                
            break;
            case ZZ_EVT_APP_MTASK_UPDATE_TO_PS_OK:
                SL_Sleep(100);
                zzLogI("ZZ_EVT_APP_MTASK_UPDATE_TO_PS_OK");
                
            break;
            case ZZ_EVT_APP_MTASK_UPDATE_BY_PS:
                SL_Sleep(100);
                zzLogI("ZZ_EVT_APP_MTASK_UPDATE_BY_PS");
                
            break;
            case ZZ_EVT_APP_MTASK_UPDATE_BY_PS_OK:
                SL_Sleep(100);
                zzLogI("ZZ_EVT_APP_MTASK_UPDATE_BY_PS_OK");
                returnErrorCode2Ps(0, zz_seq);
                
            break;
            case ZZ_EVT_APP_MTASK_QUERY_TO_PS:
                SL_Sleep(100);
                zzLogI("ZZ_EVT_APP_MTASK_QUERY_TO_PS");
                getTimerListFromPs();
                
            break;
            case ZZ_EVT_APP_MTASK_QUERY_TO_PS_OK:
                SL_Sleep(100);
                zzLogI("ZZ_EVT_APP_MTASK_QUERY_TO_PS_OK");
                
            break;
            case ZZ_EVT_APP_MTASK_DATE_TO_PS:
                SL_Sleep(100);
                zzLogI("ZZ_EVT_APP_MTASK_DATE_TO_PS");
                getDateFromPs();
                
            break;
            case ZZ_EVT_APP_MTASK_DATE_TO_PS_OK:
                SL_Sleep(100);
                zzLogI("ZZ_EVT_APP_MTASK_DATE_TO_PS_OK");
                zzEmitEvt(zz_app_mtask, ZZ_EVT_APP_MTASK_UPDATE_TO_PS);
                zzEmitEvt(zz_app_mtask, ZZ_EVT_APP_MTASK_QUERY_TO_PS);
                
            break;
            case ZZ_EVT_APP_MTASK_REDIRECT_OK:
                SL_Sleep(100);
                zzLogI("ZZ_EVT_APP_MTASK_REDIRECT_OK");
                returnErrorCode2Ps(0, zz_seq);
                zzEmitEvt(zz_app_mtask, ZZ_EVT_APP_MTASK_WSOCK_ERR);
                
            break;
            case ZZ_EVT_APP_MTASK_TIMER:
                if (ZZ_DS_TIMER == ev.nParam1)
                {
                    zzLogI("ZZ_DS_TIMER");
                    zzEmitEvt(zz_app_mtask, ZZ_EVT_APP_MTASK_GET_PS);
                }
                else if (ZZ_WSOCK_TIMER == ev.nParam1)
                {
                    zzLogI("ZZ_WSOCK_TIMER");
                    zzEmitEvt(zz_app_mtask, ZZ_EVT_APP_MTASK_WSOCK_START);
                }
                else if (ZZ_LOGIN_TIMER == ev.nParam1)
                {
                    zzLogI("ZZ_LOGIN_TIMER");
                    zzEmitEvt(zz_app_mtask, ZZ_EVT_APP_MTASK_LOGIN);
                }
            break;
            default:
                zzLogE("unknown ev = %u", ev.nEventId);
            break;
        }
    }
}


void zzAppMtaskInit(void)
{
    zz_app_mtask.element[0] = SL_CreateTask(zzAppMtask, 
        ZZ_APP_MTASK_STACKSIZE, 
        ZZ_APP_MTASK_PRIORITY, 
        ZZ_APP_MTASK_NAME);

    if (!zz_app_mtask.element[0])
    {
        zzLogE("create zzAppMtask failed!");
    }

    zzPintInit(&ds_timer_pi, 5, 5);
    zzPintInit(&wsock_timer_pi, 5, 5);
    zzPintInit(&login_timer_pi, 1, 4);
}



