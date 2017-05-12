/**
 * HTTP Request
 * 
 * Usage:
 *      create()
 *          set()
 *          set()
 *          submit()
 *          finish()
 *      release() -> finish()
 * 
 * @author Wu Pengfei <pengfei.wu@itead.cc>
 */
#ifndef __ZZ_HTTP_REQ_H__
#define __ZZ_HTTP_REQ_H__

#include "sl_os.h"
#include "sl_stdlib.h"
#include "sl_type.h"
#include "sl_debug.h"
#include "sl_error.h"
#include "sl_timer.h"
#include "sl_system.h"
#include "sl_memory.h"

#include "zz_log.h"
#include "zz_mem.h"
#include "zz_util.h"
#include "zz_net.h"
#include "zz_event.h"
#include "zz_error.h"
#include "zz_stream.h"
#include "zz_task.h"
#include "zz_lock.h"

#define ZZ_HTTP_REQ_HANDLE_MAX          (ZZ_STREAM_MAX)
#define ZZ_HTTP_REQ_ARG_MAX             (5)
#define ZZ_HTTP_REQ_HEADER_MAX          (5)

typedef enum
{
    ZZ_HTTP_RES_END_CL          = 0,   /**< Content-Length */
    ZZ_HTTP_RES_END_NO_BODY     = 1,   /**< no body */
    ZZ_HTTP_RES_END_CONN_CLOSE  = 2,   /**< the server close connection */
    ZZ_HTTP_RES_END_UNSUPPORTED = 3    /**< unsupported(Transfer-Encoding or Media Type) */
} ZZHttpResEndIndicator;

typedef enum 
{
    /* Routine Event */
    ZZ_HTTP_REQ_EVT_OPENED      = 0,     /**< stream is open already */
    ZZ_HTTP_REQ_EVT_WRITTEN     = 1,     /**< request has been written to stream */
    ZZ_HTTP_REQ_EVT_RES_BEGIN   = 2,     /**< response has received to this request */
    ZZ_HTTP_REQ_EVT_RES_HEAD    = 3,     /**< received head */        
    ZZ_HTTP_REQ_EVT_RES_BODY    = 4,     /**< received body */        
    ZZ_HTTP_REQ_EVT_RES_END     = 5,     /**< body end */        
    ZZ_HTTP_REQ_EVT_CLOSED      = 6,     /**< the stream has been closed */

    /* Exception Event */
    ZZ_HTTP_REQ_EVT_DNSERR      = 7,     /**< dns error */
    ZZ_HTTP_REQ_EVT_OPENERR     = 8,     /**< open stream error */
    ZZ_HTTP_REQ_EVT_ABORTED     = 9,     /**< this request has been aborted */
    ZZ_HTTP_REQ_EVT_TIMEOUT     = 10     /**< this request timeout */
} ZZHttpReqEvent;


/**
 *  ZHttpReqHandle should be non negative. 
 */
typedef int32 ZZHttpReqHandle;

typedef void (*ZZHttpReqEventCb)(ZZHttpReqHandle req_handle, ZZHttpReqEvent req_event);

void zzHttpReqTaskInit(void);

ZZHttpReqHandle zzHttpReqCreate(ZZStream stream);
int32 zzHttpReqRelease(ZZHttpReqHandle req_handle);

int32 zzHttpReqSetCallback(ZZHttpReqHandle req_handle, ZZHttpReqEventCb event_cb);

int32 zzHttpReqSetHostname(ZZHttpReqHandle req_handle, const char *hostname);
int32 zzHttpReqSetPort(ZZHttpReqHandle req_handle, uint16 port);
int32 zzHttpReqSetTimeout(ZZHttpReqHandle req_handle, uint32 timeout);

int32 zzHttpReqSetMethod(ZZHttpReqHandle req_handle, const char *method);
int32 zzHttpReqSetPath(ZZHttpReqHandle req_handle, const char *path);
int32 zzHttpReqSetArg(ZZHttpReqHandle req_handle, uint8 index, const char *key, const char *str);
int32 zzHttpReqSetHeader(ZZHttpReqHandle req_handle, uint8 index, const char *key, const char *str);
int32 zzHttpReqSetBody(ZZHttpReqHandle req_handle, ZZData body);

#if 0 /* no implementation yet */
int32 zzHttpReqClrArgs(ZZHttpReqHandle req_handle);
int32 zzHttpReqClrHeaders(ZZHttpReqHandle req_handle);
int32 zzHttpReqClrBody(ZZHttpReqHandle req_handle);
#endif

int32 zzHttpReqSubmit(ZZHttpReqHandle req_handle);

int32 zzHttpReqFinish(ZZHttpReqHandle req_handle);


/* 
 * Response functions 
 */

ZZHttpResEndIndicator zzHttpResGetEndIndicator(ZZHttpReqHandle req_handle);
uint16 zzHttpResGetStatus(ZZHttpReqHandle req_handle);
uint32 zzHttpResGetContentLength(ZZHttpReqHandle req_handle);
ZZData zzHttpResGetHead(ZZHttpReqHandle req_handle);
ZZData zzHttpResGetBody(ZZHttpReqHandle req_handle);

#endif /* __ZZ_HTTP_REQ_H__ */

