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
#include "zz_task.h"


#define ZZ_TEST_STREAM       (3)

//static char zz_server_name[50] = "shennongmin.iok.la";
static char zz_server_ip[ZZ_SIZE_IP] = "103.44.145.245";
static uint16 zz_server_port = 36080;
static SL_TASK zz_stream_test_task = {{0}};

#if 0

/* 
 * shennongmin.iok.la - 103.44.145.245
 */
static void zzGetIpByNameCb(U8 cid, S32 result, U8* ipaddr)
{
    zzLogI("zzGetIpByNameCb cid = %u, result = %d", cid, result);
    if (!result)
    {
        zzLogI("ipaddr = [%s]", ipaddr);
        SL_Memset(zz_server_ip, 0, sizeof(zz_server_ip));
        SL_Strcpy(zz_server_ip, ipaddr);
    }
}

static void testGetIpByName(void)
{
    int32 ret = 0;
    //ret = SL_TcpipGetHostIpbyName(SL_TcpipGetCid(), "103.44.145.245", zzGetIpByNameCb);
    ret = SL_TcpipGetHostIpbyName(SL_TcpipGetCid(), zz_server_name, zzGetIpByNameCb);
    zzLogIW(ret, "SL_TcpipGetHostIpbyName");
}

static void zzConn(void)
{
    int32 ret = 0;
    ret = SL_TcpipSocketCreate(ZZ_TEST_STREAM, SL_TCPIP_SOCK_TCP);
    if (ret < 0)
    {
        zzLogE("SL_TcpipSocketCreate err(%d)", ret);
        return;
    }
    else
    {
        zzLogI("SL_TcpipSocketCreate ok");
    }

    ret = SL_TcpipSocketBind(ZZ_TEST_STREAM);
    zzLogIE(ret, "SL_TcpipSocketBind");

    ret = SL_TcpipSocketConnect(ZZ_TEST_STREAM, zz_server_ip, zz_server_port);
    zzLogIE(ret, "SL_TcpipSocketConnect");

}
#endif

static void zzOpenCb(ZZStream stream, bool result, int32 error_code)
{
    zzLogI("%s stream = %d, result = %d, error_code = %d", 
        __FUNCTION__, stream, result, error_code);
    zzEmitEvt(zz_stream_test_task, ZZ_EVT_STREAMTEST_TASK_OPEN_DONE);
}

static void zzCloseCb(ZZStream stream, bool result, int32 error_code)
{
    zzLogI("%s stream = %d, result = %d, error_code = %d", 
        __FUNCTION__, stream, result, error_code);
    zzEmitEvt(zz_stream_test_task, ZZ_EVT_STREAMTEST_TASK_CLOSE_DONE);
}

static void zzWriteCb(ZZStream stream, bool result, int32 error_code)
{
    zzLogI("%s stream = %d, result = %d, error_code = %d", 
        __FUNCTION__, stream, result, error_code);
    zzEmitEvt(zz_stream_test_task, ZZ_EVT_STREAMTEST_TASK_WRITE_DONE);
}

static void zzReadCb(ZZStream stream, bool result, int32 error_code)
{
    int32 ret = 0;
    static uint8 zz_recv_buf[2048] = {0};
    uint8 *buf = NULL;
    
    zzLogI("%s stream = %d, result = %d, error_code = %d", 
        __FUNCTION__, stream, result, error_code);

    if (!result)
    {
        return;
    }
    
    SL_Memset(zz_recv_buf, 0, sizeof(zz_recv_buf));
    ret = zzStreamRead(stream, zz_recv_buf, sizeof(zz_recv_buf));
    if (ret > 0)
    {
        //zzLogI("len = %d, data = [%s]", ret, zz_recv_buf);
        
        if (0 == SL_Strcmp(zz_recv_buf, "end"))
        {
            zzEmitEvt(zz_stream_test_task, ZZ_EVT_STREAMTEST_TASK_CLOSE);
        }
        else
        {
            buf = SL_GetMemory(ret + 1);
            if (!buf)
            {
                zzLogE("SL_GetMemory failed");
                return;
            }
            SL_Memcpy(buf, zz_recv_buf, ret + 1);
            
            zzEmitEvtP2(zz_stream_test_task, ZZ_EVT_STREAMTEST_TASK_READ_DO, (uint32)buf, ret);
        }
    }
    else
    {
        zzLogE("err(%d)", ret);
    }    
}


static void zzStreamTestTask (void *pData)
{
    SL_EVENT ev = {0};
    int32 ret = 0;
    
    zzLogI("zzStreamTestTask begin");
    ret = zzStreamCreate(ZZ_TEST_STREAM);
    zzLogIE(ret, "zzStreamCreate");

    ret = zzStreamSetCallback(ZZ_TEST_STREAM, zzOpenCb, zzCloseCb, zzWriteCb, zzReadCb);
    zzLogIE(ret, "zzStreamSetCallback");

    zzEmitEvt(zz_stream_test_task, ZZ_EVT_STREAMTEST_TASK_OPEN);
    
    while (1)
    {
        SL_Memset(&ev, 0, sizeof(ev));
        SL_GetEvent(zz_stream_test_task, &ev);
        switch (ev.nEventId)
        {
            case ZZ_EVT_STREAMTEST_TASK_OPEN:
                zzLogI("ZZ_EVT_STREAMTEST_TASK_OPEN");
                ret = zzStreamOpen(ZZ_TEST_STREAM, zz_server_ip, zz_server_port);
                zzLogIE(ret, "zzStreamOpen");
                
            break;
            case ZZ_EVT_STREAMTEST_TASK_OPEN_DONE:
                zzLogI("ZZ_EVT_STREAMTEST_TASK_OPEN_DONE");
                ret = zzStreamWrite(ZZ_TEST_STREAM, "hello", 5);
                if (5 == ret)
                {
                    zzLogI("zzStreamWrite success");
                }
                else
                {
                    zzLogE("zzStreamWrite failed(%d)", ret);
                }
                
            break;
            case ZZ_EVT_STREAMTEST_TASK_CLOSE:
                zzLogI("ZZ_EVT_STREAMTEST_TASK_CLOSE");
                ret = zzStreamClose(ZZ_TEST_STREAM);
                zzLogIE(ret, "zzStreamClose");
                
            break;
            case ZZ_EVT_STREAMTEST_TASK_CLOSE_DONE:
                zzLogI("ZZ_EVT_STREAMTEST_TASK_CLOSE_DONE");
                
            break;
            case ZZ_EVT_STREAMTEST_TASK_WRITE_DONE:
                zzLogI("ZZ_EVT_STREAMTEST_TASK_WRITE_DONE");
                
            break;
            case ZZ_EVT_STREAMTEST_TASK_READ_DO:
                zzLogI("ZZ_EVT_STREAMTEST_TASK_READ_DO");
                zzLogI("len = %u, data = [%s]", ev.nParam2, ev.nParam1);
                
            break;
            default:
                zzLogE("unknown ev = %u", ev.nEventId);
            break;
        }
    }
}

void zzStreamTestInit(void)
{
    zz_stream_test_task.element[0] = SL_CreateTask(zzStreamTestTask, 
        2048, 
        ZZ_APP_TASK_PRIORITY_BEGIN, 
        "zzStreamTestTask");

    if (!zz_stream_test_task.element[0])
    {
        zzLogE("create tasks failed!");
    }
}


