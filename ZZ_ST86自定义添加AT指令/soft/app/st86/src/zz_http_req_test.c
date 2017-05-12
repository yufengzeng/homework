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
#include "zz_task.h"


static SL_TASK zz_http_req_test_task = {{0}};

static void zzHttpReqEventCb(ZZHttpReqHandle req_handle, ZZHttpReqEvent req_event)
{
    static uint32 body_len = 0;
    uint16 status = 0;
    uint32 cl = 0;
    ZZData head = ZZ_DATA_NULL;
    ZZData body = ZZ_DATA_NULL;
    //int32 ret = 0;
    ZZData ping = zzDataCreate(6);
    
    switch (req_event)
    {
        case ZZ_HTTP_REQ_EVT_DNSERR: 
            zzLogI("ZZ_HTTP_REQ_EVT_DNSERR");
                        
        break;
        case ZZ_HTTP_REQ_EVT_OPENERR: 
            zzLogI("ZZ_HTTP_REQ_EVT_OPENERR");
                        
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
            //zzLogI("status = %u", status);
            cl = zzHttpResGetContentLength(req_handle);
            head = zzHttpResGetHead(req_handle);
            zzLogI("HEAD = {status=%u, cl=%u, headlen=%u, data=[%s]}", 
                status,
                cl,
                head.length, 
                head.start);
        break;
        case ZZ_HTTP_REQ_EVT_RES_BODY: 
            zzLogI("ZZ_HTTP_REQ_EVT_RES_BODY");
            cl = zzHttpResGetContentLength(req_handle);
            body = zzHttpResGetBody(req_handle);
            body_len += body.length;
            zzLogI("%u, %u, %u [%s]", cl, body_len, body.length, body.start);
            //zzLogI("[%02x %02x]", body.start[0], body.start[1]);
        break;
        case ZZ_HTTP_REQ_EVT_RES_END: 
            zzLogI("ZZ_HTTP_REQ_EVT_RES_END");
            ping.length = 6;
            ping.start[0] = 0x89;
            ping.start[1] = 0x80;
            ping.start[2] = 0x00;
            ping.start[3] = 0x00;
            ping.start[4] = 0x00;
            ping.start[5] = 0x00;
            
#if 0            
            ret = zzStreamWrite(req_handle, ping.start, ping.length);
            if (ret != 6)
            {
                zzLogE("send ping err(%d)", ret);
            }
            else
            {
                zzLogI("seng ping ok");
            }
#endif            
            
        break;
        case ZZ_HTTP_REQ_EVT_CLOSED: 
            zzLogI("ZZ_HTTP_REQ_EVT_CLOSED");
            
            //ret = zzHttpReqRelease(req_handle);
            //zzLogIE(ret, "zzHttpReqRelease");
            zzPrintMemLeft();
                        
        break;
        case ZZ_HTTP_REQ_EVT_ABORTED: 
            zzLogI("ZZ_HTTP_REQ_EVT_ABORTED");
                        
        break;
        case ZZ_HTTP_REQ_EVT_TIMEOUT: 
            zzLogI("ZZ_HTTP_REQ_EVT_TIMEOUT");
                        
        break;
        default:
            zzLogE("unknown event(%d)", req_event);
        break;
    }
}

/*
Í¼Æ¬: http://www.shennongmin.org/wp-content/uploads/2016/11/avro-logo.png
´óÐ¡: 4,777×Ö½Ú

Port: SSL 443, NON-SSL 8081

Dispatcher Server

    Public: cn-disp.coolkit.cc
    Test: 54.223.98.144

Persistent Server
    54.222.195.159

*/
static void testHttpReq(void)
{
    ZZHttpReqHandle req_handle = 0;
    int32 ret = 0;
    //ZZData body = ZZ_DATA_NULL;
    
    zzPrintMemLeft();
    
    //body = zzDataCreate(4096);
    
    zzPrintMemLeft();
    
    req_handle = zzHttpReqCreate(1); // stream 3
    if (req_handle < 0)
    {
        zzLogE("req_handle err");
    }
    else
    {
        zzLogI("req_handle ok");
    }

    //zzHttpReqSetHostname(req_handle, "103.44.145.245");
    //zzHttpReqSetHostname(req_handle, "shennongmin.iok.la");
    //zzHttpReqSetPort(req_handle, 36080);
    zzHttpReqSetCallback(req_handle, zzHttpReqEventCb);
    //zzHttpReqSetHostname(req_handle, "54.179.149.246"); /* www.shennongmin.org */
    //zzHttpReqSetHostname(req_handle, "www.shennongmin.org"); /* www.shennongmin.org */
    zzHttpReqSetHostname(req_handle, "54.222.195.159");
    
    zzHttpReqSetPort(req_handle, 8081);
    zzHttpReqSetTimeout(req_handle, 10);
    
    zzHttpReqSetMethod(req_handle, "GET");
    zzHttpReqSetPath(req_handle, "/api/ws");
    //zzHttpReqSetPath(req_handle, "/wp-content/uploads/2016/11/avro-logo.png");

    //zzHttpReqSetArg(req_handle, 4, "p", "261");
    //zzHttpReqSetArg(req_handle, 3, "password", "1234");
    
    //zzHttpReqSetHeader(req_handle, 0, "Host", "localhost");    
    //zzHttpReqSetHeader(req_handle, 3, "Connection", "close");
    //zzHttpReqSetHeader(req_handle, 3, "Connection", "keep-alive");
    
    zzHttpReqSetHeader(req_handle, 0, "Host", "iotgo.iteadstudio.com");
    zzHttpReqSetHeader(req_handle, 1, "Connection", "upgrade");    
    zzHttpReqSetHeader(req_handle, 2, "Upgrade", "websocket");    
    zzHttpReqSetHeader(req_handle, 3, "Sec-WebSocket-Key", "ITEADTmobiM0x1DabcEsnw==");    
    //zzHttpReqSetHeader(req_handle, 3, "Sec-WebSocket-Key", "dGhlIHNhbXBsZSBub25jZQ==");    
    zzHttpReqSetHeader(req_handle, 4, "Sec-WebSocket-Version", "13");    
    


    //zzHttpReqSetBody(req_handle, body);
    
    zzPrintMemLeft();
    
    ret = zzHttpReqSubmit(req_handle);
    zzLogIE(ret, "zzHttpReqSubmit");
    
    //ret = zzHttpReqRelease(req_handle);
    //zzLogIE(ret, "zzHttpReqRelease");
    //zzPrintMemLeft();
    
    //zzDataFree(body);
    
    zzPrintMemLeft();
}

static void zzHttpReqTestTask (void *pData)
{
    SL_EVENT ev = {0};
    
    zzLogI("zzHttpReqTestTask begin");

    zzEmitEvt(zz_http_req_test_task, ZZ_EVT_HTTPREQTEST_TASK_START);
    //SL_StartTimer(zz_http_req_test_task, 0, SL_TIMER_MODE_PERIODIC, SL_SecondToTicks(1));
    
    while (1)
    {
        SL_Memset(&ev, 0, sizeof(ev));
        SL_GetEvent(zz_http_req_test_task, &ev);
        switch (ev.nEventId)
        {
            case ZZ_EVT_HTTPREQTEST_TASK_START:
                zzLogI("ZZ_EVT_HTTPREQTEST_TASK_START");
                testHttpReq();
                
            break;
            case ZZ_EVT_HTTPREQTEST_TASK_TIMER:
                zzLogI("ZZ_EVT_HTTPREQTEST_TASK_TIMER");
                testHttpReq();
                
            break;
            default:
                zzLogE("unknown ev = %u", ev.nEventId);
            break;
        }
    }
}

void zzHttpTestInit(void)
{
    zz_http_req_test_task.element[0] = SL_CreateTask(zzHttpReqTestTask, 
        2048, 
        ZZ_APP_TASK_PRIORITY_BEGIN, 
        "zzHttpReqTestTask");

    if (!zz_http_req_test_task.element[0])
    {
        zzLogE("create tasks failed!");
    }
}

