/**
 * Wsock is designed for singleton pattern. 
 *
 * Usage:
 *  @code
 *      init
 *      set
 *      enable/disable
 *      start
 *      write
 *      read
 *      stop
 *  @endcode
 * @auther Wu Pengfei <pengfei.wu@itead.cc>
 *
 */
#ifndef __ZZ_WSOCK_H__
#define __ZZ_WSOCK_H__

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

typedef enum 
{
    /* Routine Event */
    ZZ_WSOCK_EVT_OPENED      = 0,     /**< stream is open already */
    ZZ_WSOCK_EVT_STARTED     = 1,     /**< opening handshake success */
    ZZ_WSOCK_EVT_WRITTEN     = 2,     /**< frame written */
    ZZ_WSOCK_EVT_READ        = 3,     /**< frame is ready for read */
    ZZ_WSOCK_EVT_STOPPED     = 4,     /**< closing handshake success */
    ZZ_WSOCK_EVT_CLOSED      = 5,     /**< the stream has been closed */

    /* Exception Event */
    ZZ_WSOCK_EVT_DNSERR      = 6,     /**< dns error */
    ZZ_WSOCK_EVT_OPENERR     = 7,     /**< open stream error */
    ZZ_WSOCK_EVT_ABORTED     = 8,     /**< wsock has been aborted */
} ZZWsockEvent;

typedef void (*ZZWsockEventCb)(ZZWsockEvent ws_event);


typedef enum
{
    ZZ_WSOCK_FRAME_UNSUPPORT = 0,
    ZZ_WSOCK_FRAME_TEXT      = 1,
    ZZ_WSOCK_FRAME_PING      = 9,
    ZZ_WSOCK_FRAME_PONG      = 10,
    ZZ_WSOCK_FRAME_CLOSE     = 8
} ZZWsockFrameType;

typedef struct
{
    ZZWsockFrameType type;
    ZZData payload;
} ZZWsockFrame;

int32 zzWsockInit(void);

int32 zzWsockSetCallback(ZZWsockEventCb event_cb);
int32 zzWsockSetRemote(const char *hostname, uint16 port);

int32 zzWsockSetHeartbeat(uint32 base_inteval);
void zzWsockEnableHeartbeat(void);
void zzWsockDisableHeartbeat(void);

int32 zzWsockStart(void);
int32 zzWsockStop(void);

int32 zzWsockWrite(ZZWsockFrame data);
ZZWsockFrame zzWsockRead(void);

int32 zzWsockWriteText(const char *text);
const char * zzWsockReadText(void);


#endif /* __ZZ_WSOCK_H__ */

