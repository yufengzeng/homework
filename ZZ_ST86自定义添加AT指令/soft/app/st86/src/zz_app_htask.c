#include "zz_app_htask.h"
#include "iotgo_sled.h"
#include "zz_key.h"
#include "zz_global.h"
#include "zz_timer.h"

#define ZZ_APP_HTASK_SLED_TIMER     (ZZ_APP_HTASK_TIMERID_BEGIN)
#define ZZ_APP_HTASK_KEY_TIMER      (ZZ_APP_HTASK_TIMERID_BEGIN + 1)
#if 0
#define IOTGO_RELAY_GPIO            (SL_GPO_1)    
#else
#define IOTGO_RELAY_GPIO            (SL_GPIO_2)
#endif

#if 0
#define IOTGO_BUTTON_GPIO           (SL_GPIO_1)
#else
#define IOTGO_BUTTON_GPIO           (SL_GPIO_4)
#endif

/*
 * RELAY CONF FILE FORMAT:
 * @code
 *  {
 *      "def":0/1/2,
 *      "switch":"on or off",
 *  }
 * @endcode
 */
#define ZZ_RELAY_CONF_FILE          "relayconf.json"

#define ZZ_DEVICE_ENABLE_FILE         "deviceenable.flag"


/* 
 * After started, relay will be default. 
 */
typedef enum 
{
    ZZ_RLY_DEF_OFF  = 0,    /* off (default when exception occurs) */
    ZZ_RLY_DEF_ON   = 1,    /* on */
    ZZ_RLY_DEF_STAY = 2     /* last state (on or off) */
} ZZRelayDefault;

SL_TASK zz_app_htask = {{0}};

static bool relay_on = false;
static ZZRelayDefault relay_def = ZZ_RLY_DEF_OFF;
static ZZDeviceState device_state = ZZ_DEVICE_STATE_INVALID;

bool zzRelayIsOn(void)
{
    return relay_on;
}


/*
 * Save relay_def and relay_on to file. 
 */
static void zzRelaySave(void)
{
    int32 ret = 0;
    int32 fd = 0;
    cJSON *json_root = NULL;
    char *raw = NULL;
    int32 len = 0;
    
    fd = SL_FileOpen(ZZ_RELAY_CONF_FILE, SL_FS_RDWR | SL_FS_CREAT);
    if (fd < 0)
    {
        zzLogE("SL_FileOpen err");
        return;
    }

    json_root = cJSON_CreateObject();
    if (!json_root)
    {
        zzLogE("create json err");
        goto ExitErr1;
    }

    cJSON_AddNumberToObject(json_root, "def", relay_def);
    cJSON_AddStringToObject(json_root, "switch", relay_on ? "on" : "off");

    raw = cJSON_Print(json_root);
    len = SL_Strlen(raw) + 1;
    zzLogP("save len = %u, [%s]", len, raw);

    ret = SL_FileWrite(fd, raw, len);
    if (ret != len)
    {
        zzLogE("SL_FileWrite err");
    }

    SL_FreeMemory(raw);
    cJSON_Delete(json_root);
    if (SL_FileClose(fd))
    {
        zzLogE("SL_FileClose err");
    }

    return;

ExitErr1:
    
    if (SL_FileClose(fd))
    {
        zzLogE("SL_FileClose err");
    }
    return;
}

/*
 * Load relay_def and relay_on from file. 
 */
static void zzRelayLoad(void)
{
    int32 ret = 0;
    int32 fd = 0;
    int32 len = 0;
    ZZData data;
    
    cJSON *json_root = NULL;
    cJSON *json_def = NULL;
    cJSON *json_switch = NULL;

    fd = SL_FileOpen(ZZ_RELAY_CONF_FILE, SL_FS_RDWR | SL_FS_CREAT);
    if (fd < 0)
    {
        zzLogE("SL_FileOpen failed");
        return;
    }

    len = SL_FileGetSize(fd);
    if (len <= 0)
    {
        zzLogE("get size err");
        goto ExitErr1;
    }

    data = zzDataCreate(len);
    ret = SL_FileRead(fd, data.start, data.length);
    if (ret != data.length)
    {
        zzLogE("read err");
    }

    zzLogP("read len=%u, [%s]", data.length, data.start);
    
    json_root = cJSON_Parse(data.start);
    if (!json_root)
    {
        zzLogE("parse json err");
        goto ExitErr2;
    }

    json_switch = cJSON_GetObjectItem(json_root, "switch");
    json_def = cJSON_GetObjectItem(json_root, "def");

    if (json_switch)
    {
        if (0 == SL_Strcmp(json_switch->valuestring, "on"))
        {
            relay_on = true;
        }
        else
        {
            relay_on = false;
        }
    }

    if (json_def)
    {
        if (ZZ_RLY_DEF_ON == json_def->valueint)
        {
            relay_def = ZZ_RLY_DEF_ON;
        }
        else if (ZZ_RLY_DEF_STAY == json_def->valueint)
        {
            relay_def = ZZ_RLY_DEF_STAY;
        }
        else
        {
            relay_def = ZZ_RLY_DEF_OFF;
        }
    }
    
    cJSON_Delete(json_root);
    zzDataFree(&data);
    if (SL_FileClose(fd))
    {
        zzLogE("SL_FileClose err");
    }

    return;


ExitErr2:
    zzDataFree(&data);

ExitErr1:
    
    if (SL_FileClose(fd))
    {
        zzLogE("SL_FileClose err");
    }
    return;
}

void zzRelaySetDef(const char *startup)
{
    if (!startup)
    {
        zzLogE(ZZ_ERR_STR_INP);
        return;
    }

    if (0 == SL_Strcmp(startup, "on"))
    {
        relay_def = ZZ_RLY_DEF_ON;
    }
    else if (0 == SL_Strcmp(startup, "stay"))
    {
        relay_def = ZZ_RLY_DEF_STAY;
    }
    else
    {
        relay_def = ZZ_RLY_DEF_OFF;
    }   
    
    zzRelaySave();
}

const char * zzRelayGetDef(void)
{
    if (ZZ_RLY_DEF_ON == relay_def)
    {
        return "on";
    }
    else if (ZZ_RLY_DEF_STAY == relay_def)
    {
        return "stay";
    }
    else
    {
        return "off";
    }
}

static void zzRelayOn(void)
{
#if 0
    SL_GpoWrite(IOTGO_RELAY_GPIO, SL_PIN_HIGH);
#else
    SL_GpioWrite(IOTGO_RELAY_GPIO, SL_PIN_HIGH);
#endif    
    relay_on = true;
    zzRelaySave();
}

static void zzRelayOff(void)
{
#if 0
    SL_GpoWrite(IOTGO_RELAY_GPIO, SL_PIN_LOW);
#else
    SL_GpioWrite(IOTGO_RELAY_GPIO, SL_PIN_LOW);
#endif
    relay_on = false;
    zzRelaySave();
}

static void zzRelayInit(void)
{
#if 0
    SL_GpoWrite(IOTGO_RELAY_GPIO, SL_PIN_LOW);
#else
    SL_GpioSetDir(IOTGO_RELAY_GPIO, SL_GPIO_OUT);
    SL_GpioWrite(IOTGO_RELAY_GPIO, SL_PIN_LOW);
#endif    
    relay_on = false;
    
    zzRelayLoad();

    if (ZZ_RLY_DEF_ON == relay_def)
    {
        zzRelayOn();
    }
    else if (ZZ_RLY_DEF_STAY == relay_def)
    {
        if (relay_on)
        {
            zzRelayOn();
        }
        else
        {
            zzRelayOff();
        }                
    }
    else
    {
        zzRelayOff();
    }
}

static void initKey(void)
{
    SL_GpioSetDir(IOTGO_BUTTON_GPIO, SL_GPIO_IN);
}

ZZDeviceState iotgoGetDeviceState(void)
{
    return device_state;
}

void iotgoSetDeviceMode(ZZDeviceState state)
{
    device_state = state;
}

static bool readKeyState(void)
{
    return !SL_GpioRead(IOTGO_BUTTON_GPIO);
}
  

static void releaseKeyCb(void)
{
    if (zzRelayIsOn())
    {
        zzRelayOff();
    }
    else
    {
        zzRelayOn();
    }

    if (iotgoGetDeviceState() == ZZ_DEVICE_STATE_LOGINED)
    {
        zzEmitEvt(zz_app_mtask, ZZ_EVT_APP_MTASK_UPDATE_TO_PS);
    }
}


bool zzDeviceIsEnabled(void)
{
    int32 ret = 0;
    
    ret = SL_FileCheck(ZZ_DEVICE_ENABLE_FILE);
    if (ret < 0)
    {
        zzLogI("file %s does exist(%d)", ZZ_DEVICE_ENABLE_FILE, ret);
        return false;
    }
    else
    {
        zzLogI("enabled already");
        return true;
    }
}

void zzDeviceDisable(void)
{
    int32 ret = 0;
    ret = SL_FileDelete(ZZ_DEVICE_ENABLE_FILE);
    if (ret)
    {
        zzLogE("device disable err(%d)", ret);
    }
    else
    {
        zzLogI("device disabled ok");
    }
}

static void zzDeviceEnable(void)
{   
    int32 ret = 0;
    int32 fd = 0;
        
    fd = SL_FileOpen(ZZ_DEVICE_ENABLE_FILE, SL_FS_RDWR | SL_FS_CREAT);
    if (fd < 0)
    {
        zzLogE("SL_FileOpen err");
        return;
    }

    ret = SL_FileWrite(fd, "enabled", 8);
    if (8 != ret)
    {
        zzLogE("SL_FileWrite enabled failed(%d)", ret);
    }
    
    ret = SL_FileClose(fd);
    zzLogIE(ret, "SL_FileClose");

    zzLogI("device enable ok");
}

static void pressLongKeyCb(void)
{
    if (iotgoGetDeviceState() == ZZ_DEVICE_STATE_DISABLED 
        && !zzDeviceIsEnabled())
    {
        zzEmitEvt(zz_app_htask, ZZ_EVT_APP_HTASK_USER_ENABLE);
    }
}

void zzRelayExecuteTimer(void *json_str)
{
    cJSON *json_root = NULL;
    cJSON *field_switch = NULL;
    
    json_root = cJSON_Parse(json_str);
    if (!json_root)
    {
        zzLogE("parse json failed");
        return;
    }
    
    field_switch = cJSON_GetObjectItem(json_root, "switch");
    if (!field_switch)
    {
        zzLogE("bad switch");
        goto ExitErr1;
    }

    if (0 == SL_Strcmp(field_switch->valuestring, "on"))
    {
        zzRelayOn();
    }
    else
    {
        zzRelayOff();
    }
    
    if (iotgoGetDeviceState() == ZZ_DEVICE_STATE_LOGINED)
    {
        zzEmitEvt(zz_app_mtask, ZZ_EVT_APP_MTASK_UPDATE_TO_PS);
    }
    
    cJSON_Delete(json_root);
    return;
    
ExitErr1:
    cJSON_Delete(json_root);
    return;

}

static void zzAppHtask (void *pdata)
{
    SL_EVENT ev = {0};
    
    zzLogFB();    
    zzPrintCurrentTaskPrio();
    
    zzRelayInit();

    initKey();
    keyRegisterSingle(readKeyState, 
        NULL,
        releaseKeyCb,
        pressLongKeyCb,
        NULL,
        NULL);

    SL_StartTimer(zz_app_htask, 
        ZZ_APP_HTASK_KEY_TIMER, 
        SL_TIMER_MODE_PERIODIC, 
        SL_MilliSecondToTicks(25));

    
    iotgoSledInit();
    SL_StartTimer(zz_app_htask, 
        ZZ_APP_HTASK_SLED_TIMER, 
        SL_TIMER_MODE_PERIODIC, 
        SL_MilliSecondToTicks(100));
    
    while (1)
    {
        SL_Memset(&ev, 0, sizeof(ev));
        SL_GetEvent(zz_app_htask, &ev);
        switch (ev.nEventId)
        {
            case ZZ_EVT_APP_HTASK_START:
                zzLogI("ZZ_EVT_APP_HTASK_START");
                
            break;
            case ZZ_EVT_APP_HTASK_RELAY_ON:
                zzLogI("ZZ_EVT_APP_HTASK_RELAY_ON");
                zzRelayOn();
                
            break;
            case ZZ_EVT_APP_HTASK_RELAY_OFF:
                zzLogI("ZZ_EVT_APP_HTASK_RELAY_OFF");
                zzRelayOff();
                
            break;
            case ZZ_EVT_APP_HTASK_USER_ENABLE:
                zzLogI("ZZ_EVT_APP_HTASK_USER_ENABLE");
                zzDeviceEnable();
                zzEmitEvt2Main(ZZ_EVT_MAIN_TASK_DEVICE_ENABLED);
                
            break;
            case ZZ_EVT_APP_HTASK_TIMER:
                if (ZZ_APP_HTASK_SLED_TIMER == ev.nParam1)
                {
                    iotgoSledScanCb();
                }
                else if (ZZ_APP_HTASK_KEY_TIMER == ev.nParam1)
                {
                    keyScan();
                }
                
            break;
            default:
                zzLogE("unknown ev = %u", ev.nEventId);
            break;
        }
    }
}


void zzAppHtaskInit(void)
{
    zz_app_htask.element[0] = SL_CreateTask(zzAppHtask, 
        ZZ_APP_HTASK_STACKSIZE, 
        ZZ_APP_HTASK_PRIORITY, 
        ZZ_APP_HTASK_NAME);

    if (!zz_app_htask.element[0])
    {
        zzLogE("create zzAppHtask failed!");
    }
}


