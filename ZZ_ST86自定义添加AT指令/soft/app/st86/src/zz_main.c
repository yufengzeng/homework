#include "sl_os.h"
#include "sl_stdlib.h"
#include "sl_type.h"
#include "sl_debug.h"
#include "sl_error.h"
#include "sl_timer.h"
#include "sl_system.h"
#include "sl_memory.h"

#include "zz_log.h"
#include "zz_util.h"
#include "zz_net.h"
#include "zz_event.h"
#include "zz_stream.h"
#include "zz_http_req.h"
#include "zz_wsock.h"
#include "iotgo_factory.h"

#include "zz_app_htask.h"
#include "zz_app_mtask.h"
#include "zz_app_ltask.h"
#include "zz_global.h"
#include "zz_timer.h"
#include "iotgo_upgrade.h"


#define zzMain  SL_Entry
#define ZZ_NET_SHIELD_TIMER     (ZZ_MAIN_TASK_TIMERID_BEGIN)

#define IOTGO_TEST_PURPOSE_GPIO   (SL_GPIO_3)

static SL_TASK task = {{0}};
static bool flag_device_is_test_purpose = false;

static void zzAppCreateTask(void)
{
    if (zzTimerTaskInit(zzRelayExecuteTimer))
    {
        zzLogI("zzTimerTaskInit ok");
    }
    else
    {
        zzLogE("zzTimerTaskInit err");
    }

    zzAppHtaskInit();
    zzAppMtaskInit();
    zzAppLtaskInit();
    iotgoUpgradeCreateUpgradeTask();
}

static void zzGprsAttachCb(U8 state, S32 result)
{
    zzLogI("zzGprsAttachCb state = %u, result = %d", state, result);
    zzNetPrintState();
    if (state)
    {
        //zzEmitEvt2Main(ZZ_EVT_MAIN_TASK_DETACH);
    }
}

static void netShield(void)
{
    zzNetPrintState();
    if (!zzNetGprsIsActived())
    {
        zzLogW("GPRS is unavailable and active it now!");
        zzEmitEvt2Main(ZZ_EVT_MAIN_TASK_ACTIVE);
    }
}

static void startNetShield(void)
{
    SL_StartTimer(task, ZZ_NET_SHIELD_TIMER, 1, SL_SecondToTicks(60));
}


static bool zzDeviceIsTestPurpose(void)
{
    SL_GpioSetDir(IOTGO_TEST_PURPOSE_GPIO, SL_GPIO_IN);
    if (SL_PIN_HIGH == SL_GpioRead(IOTGO_TEST_PURPOSE_GPIO))
    {
        zzLogI("work for test purpose");
        return true;
    }
    else
    {
        zzLogI("work for user mode");
        return false;
    }
}


/**
 * Main Task (priority 232 > all user tasks)
 */
void APP_ENTRY_START zzMain(void)
{
    SL_EVENT ev = {0};
    int32 ret = 0; 
    
    task.element[0] = SL_GetAppTaskHandle();
    
    zzLogI("core init begin");
    SL_Sleep(100); /* 10 milliseconds waiting for log output */
    while(SL_CoreInitFinish() == FALSE)
    {
        SL_Sleep(100);
    }
    zzLogI("core init end");    

#if 0
    void iotgoFactoryGenerateDeviceIdInfoFile(void);
    iotgoFactoryGenerateDeviceIdInfoFile();
    iotgoFactoryVerifyDeviceIdInfo();
#else
    if (!iotgoFactoryVerifyDeviceIdInfo()
        || iotgoIsBootForFactory()
        )
    {
        iotgoFactoryMainLoop(); // NEVER COME BACK!!!
    }
#endif

    if (zzDeviceIsTestPurpose())
    {
        flag_device_is_test_purpose = true;
    }

    zzAppCreateTask();

    iotgoSetDeviceMode(ZZ_DEVICE_STATE_START);
    
    zzEmitEvt2Main(ZZ_EVT_MAIN_TASK_START);
    
    while (1)
    {
        SL_Memset(&ev, 0, sizeof(ev));
        SL_GetEvent(task, &ev);
        switch (ev.nEventId)
        {
            case ZZ_EVT_MAIN_TASK_START:
                zzLogI("ZZ_EVT_MAIN_TASK_START"); 
                zzNetPrintState();
                
                /* Waiting for gsm network reg ok */
                while(SL_GetNwStatus() != SL_RET_OK)
                {
                    zzLogP("net reg...");
                    zzNetPrintState();
                    SL_Sleep(500);
                }
                zzLogI("net reg ok");
                zzNetPrintState();

                //zzPrintVerInfo();
                
                zzNetInit();                
                zzStreamInit();
                zzHttpReqTaskInit();
                zzWsockInit();
                
                if (flag_device_is_test_purpose)
                {
                    zzEmitEvt2Main(ZZ_EVT_MAIN_TASK_DEVICE_ENABLED);
                }
                else if (zzDeviceIsEnabled())
                {
                    zzEmitEvt2Main(ZZ_EVT_MAIN_TASK_DEVICE_ENABLED);
                }
                else
                {
                    zzLogI("device is waiting for enabled...");
                    iotgoSetDeviceMode(ZZ_DEVICE_STATE_DISABLED);
                }
                
            break;
            case ZZ_EVT_MAIN_TASK_DEVICE_ENABLED:
                zzLogI("ZZ_EVT_MAIN_TASK_DEVICE_ENABLED");
                iotgoSetDeviceMode(ZZ_DEVICE_STATE_START);
                zzEmitEvt2Main(ZZ_EVT_MAIN_TASK_ACTIVE);
                startNetShield();
                
            break;            
            case ZZ_EVT_MAIN_TASK_ATTACH:
                zzLogI("ZZ_EVT_MAIN_TASK_ATTACH");
                ret = SL_GprsAtt(SL_GPRS_ATTACHED, zzGprsAttachCb);
                zzLogIW(ret, "SL_GprsAtt SL_GPRS_ATTACHED");
                
            break;            
            case ZZ_EVT_MAIN_TASK_DETACH:
                zzLogI("ZZ_EVT_MAIN_TASK_DETACH");
                ret = SL_GprsAtt(SL_GPRS_DETACHED, zzGprsAttachCb);
                zzLogIW(ret, "SL_GprsAtt SL_GPRS_DETACHED");
                
            break;
            case ZZ_EVT_MAIN_TASK_ACTIVE:
                zzLogI("ZZ_EVT_MAIN_TASK_ACTIVE");
                iotgoSetDeviceMode(ZZ_DEVICE_STATE_GPRS);
                zzNetActive();
                
            break;
            case ZZ_EVT_MAIN_TASK_ACTIVE_DONE:
                zzLogI("ZZ_EVT_MAIN_TASK_ACTIVE_DONE");
                zzEmitEvt(zz_app_mtask, ZZ_EVT_APP_MTASK_START);
                
            break;
            case ZZ_EVT_MAIN_TASK_DEACTIVE:
                zzLogI("ZZ_EVT_MAIN_TASK_DEACTIVE");
                zzNetDeactive();
                
            break;            
            case ZZ_EVT_MAIN_TASK_DEACTIVE_DONE:
                zzLogI("ZZ_EVT_MAIN_TASK_DEACTIVE_DONE");
                
            break;
            case ZZ_EVT_MAIN_TASK_TIMER:
                //zzLogI("ZZ_EVT_MAIN_TASK_TIMER");
                if (ZZ_NET_SHIELD_TIMER == ev.nParam1)
                {
                    netShield();
                }
                
            break;
            default:
                zzLogE("unknown ev = %u", ev.nEventId);
            break;
        }
    }
}

