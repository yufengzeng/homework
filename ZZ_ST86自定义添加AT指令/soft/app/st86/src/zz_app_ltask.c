#include "zz_app_ltask.h"

static SL_TASK app_ltask = {{0}};


static void zzAppLtask (void *pdata)
{
    SL_EVENT ev = {0};
    
    zzLogFB();
    zzPrintCurrentTaskPrio();
    
    while (1)
    {
        SL_Memset(&ev, 0, sizeof(ev));
        SL_GetEvent(app_ltask, &ev);
        switch (ev.nEventId)
        {
            case ZZ_EVT_APP_LTASK_START:
                zzLogI("ZZ_EVT_APP_LTASK_START");
                
            break;
            case ZZ_EVT_APP_LTASK_TIMER:
                zzLogI("ZZ_EVT_APP_LTASK_TIMER");
                
            break;
            default:
                zzLogE("unknown ev = %u", ev.nEventId);
            break;
        }
    }
}




void zzAppLtaskInit(void)
{
    app_ltask.element[0] = SL_CreateTask(zzAppLtask, 
        ZZ_APP_LTASK_STACKSIZE, 
        ZZ_APP_LTASK_PRIORITY, 
        ZZ_APP_LTASK_NAME);

    if (!app_ltask.element[0])
    {
        zzLogE("create zzAppLtask failed!");
    }
}



