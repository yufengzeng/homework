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
#include "zz_wsock.h"
#include "zz_task.h"


static SL_TASK task = {{0}};
static ZZWsockFrame ping;

static void zzWsockEventCb(ZZWsockEvent ws_ev)
{  
    int32 ret = 0;
    
    switch (ws_ev)
    {
        case ZZ_WSOCK_EVT_DNSERR: 
            zzLogI("ZZ_WSOCK_EVT_DNSERR");
                        
        break;
        case ZZ_WSOCK_EVT_OPENERR: 
            zzLogI("ZZ_WSOCK_EVT_OPENERR");
                        
        break;
        case ZZ_WSOCK_EVT_OPENED: 
            zzLogI("ZZ_WSOCK_EVT_OPENED");
                        
        break;
        case ZZ_WSOCK_EVT_STARTED: 
            zzLogI("ZZ_WSOCK_EVT_STARTED");
            ping.type = ZZ_WSOCK_FRAME_PING;            
            ret = zzWsockWrite(ping);
            if (ret < 0)
            {
                zzLogE("zzWsockWrite err(%d)", ret);
            }
            
        break;
        case ZZ_WSOCK_EVT_WRITTEN: 
            zzLogI("ZZ_WSOCK_EVT_WRITTEN");
                        
        break;
        case ZZ_WSOCK_EVT_READ: 
            zzLogI("ZZ_WSOCK_EVT_READ");
            ZZWsockFrame frame = zzWsockRead();
            zzLogI("frame.type = %u, frame.payload.len = %u", frame.type, frame.payload.length);

#if 1
            if (ZZ_WSOCK_FRAME_PONG == frame.type)
            {
                frame.type = ZZ_WSOCK_FRAME_CLOSE;
                ret = zzWsockWrite(frame);
                if (ret < 0)
                {
                    zzLogE("zzWsockWrite err(%d)", ret);
                }
            }
#else
            zzWsockStop();
#endif

            
        break;
        case ZZ_WSOCK_EVT_STOPPED: 
            zzLogI("ZZ_WSOCK_EVT_STOPPED");
            
        break;
        case ZZ_WSOCK_EVT_CLOSED: 
            zzLogI("ZZ_WSOCK_EVT_CLOSED");                        
            
        break;
        case ZZ_WSOCK_EVT_ABORTED: 
            zzLogI("ZZ_WSOCK_EVT_ABORTED");
                        
        break;
        default:
            zzLogE("unknown event(%d)", ws_ev);
        break;
    }
}

/*
Port: SSL 443, NON-SSL 8081

Dispatcher Server

    Public: cn-disp.coolkit.cc
    Test: 54.223.98.144

Persistent Server
    54.222.195.159

*/
static void testWsock(void)
{
    int32 ret = 0;
    
    ret = zzWsockSetCallback(zzWsockEventCb);
    zzLogIE(ret, "zzWsockSetCallback");
    
    zzWsockSetRemote("54.222.195.159", 8081);   
    zzLogIE(ret, "zzWsockSetRemote");
    
    zzWsockStart();
    zzLogIE(ret, "zzWsockStart");
}

static void zzWsockTestTask (void *pData)
{
    SL_EVENT ev = {0};
    
    zzLogFB();
    
    zzEmitEvt(task, ZZ_EVT_WSOCKTEST_TASK_START);
    
    while (1)
    {
        SL_Memset(&ev, 0, sizeof(ev));
        SL_GetEvent(task, &ev);
        switch (ev.nEventId)
        {
            case ZZ_EVT_WSOCKTEST_TASK_START:
                zzLogI("ZZ_EVT_WSOCKTEST_TASK_START");
                testWsock();
                
            break;
            default:
                zzLogE("unknown ev = %u", ev.nEventId);
            break;
        }
    }
}

void zzWsockTestInit(void)
{
    task.element[0] = SL_CreateTask(zzWsockTestTask, 
        2048, 
        ZZ_APP_TASK_PRIORITY_BEGIN, 
        "zzWsockTestTask");

    if (!task.element[0])
    {
        zzLogE("create tasks failed!");
    }
}


