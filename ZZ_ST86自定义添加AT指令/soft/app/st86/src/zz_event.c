#include "zz_event.h"

void zzEmitEvtP3(SL_TASK task, uint32 msg, uint32 param1, uint32 param2, uint32 param3)
{
    SL_EVENT ev = {0};
    int32 ret = 0;
    
    ev.nEventId = msg;
    ev.nParam1 = param1;
    ev.nParam2 = param2;
    ev.nParam3 = param3;
    
    ret = SL_SendEvents(task, &ev);
    if (ret)
    {
        zzLogE("SL_SendEvents err(%d)", ret);
    }
}

void zzEmitEvtP2(SL_TASK task, uint32 msg, uint32 param1, uint32 param2)
{
    zzEmitEvtP3(task, msg, param1, param2, 0);
}

void zzEmitEvtP1(SL_TASK task, uint32 msg, uint32 param1)
{
    zzEmitEvtP3(task, msg, param1, 0, 0);
}

void zzEmitEvt(SL_TASK task, uint32 msg)
{
    zzEmitEvtP3(task, msg, 0, 0, 0);
}

void zzEmitEvt2Main(uint32 msg)
{
    SL_TASK task = {{0}};
    task.element[0] = SL_GetAppTaskHandle();
    zzEmitEvtP3(task, msg, 0, 0, 0);
}

