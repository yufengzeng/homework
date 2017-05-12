/**
 * Usage:
 * @code
 *  Take()
 *      Set/ClrCallback()
 *      Open() 
 *          GetState()
 *      OpenCb 
 *          GetState()
 *      Write()
 *          GetState() 
 *      WriteCb
 *          GetState() 
 *      ReadCb
 *          GetState() 
 *      Read()
 *          GetState() 
 *      Close
 *          GetState() 
 *      CloseCb
 *          GetState() 
 *  Give()
 * @endcode
 * 
 * @note
 *  1. Before calling Give, user MUST close the stream totally. 
 */

#ifndef __ZZ_STREAM_H__
#define __ZZ_STREAM_H__

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
#include "zz_lock.h"

/* Indicate the total number of streams available */
#define ZZ_STREAM_MAX  (4)   

typedef enum 
{
    ZZ_STREAM_0 = 0,
    ZZ_STREAM_1 = 1,
    ZZ_STREAM_2 = 2,
    ZZ_STREAM_3 = 3
} ZZStream;

typedef enum
{
    ZZ_STREAM_IDLE    = 0, 
    ZZ_STREAM_OPEN    = 1, 
    ZZ_STREAM_OPENED  = 2, 
    ZZ_STREAM_CLOSE   = 3, 
    ZZ_STREAM_CLOSED  = 4   
} ZZStreamState;

typedef void (*ZZStreamOpenCb)(ZZStream stream, bool result, int32 error_code);
typedef void (*ZZStreamCloseCb)(ZZStream stream, bool result, int32 error_code);
typedef void (*ZZStreamWriteCb)(ZZStream stream, bool result, int32 error_code);
typedef void (*ZZStreamReadCb)(ZZStream stream, bool result, int32 error_code);

void zzStreamInit(void);
int32 zzStreamCreate(ZZStream stream);
int32 zzStreamRelease(ZZStream stream);
int32 zzStreamSetCallback(ZZStream stream, 
    ZZStreamOpenCb open_cb,
    ZZStreamCloseCb close_cb,
    ZZStreamWriteCb write_cb,
    ZZStreamReadCb read_cb);

int32 zzStreamClrCallback(ZZStream stream);
    
int32 zzStreamOpen(ZZStream stream, const char *ip, uint16 port);
int32 zzStreamClose(ZZStream stream);
int32 zzStreamWrite(ZZStream stream, uint8 *pdata, uint16 len);
int32 zzStreamRead(ZZStream stream, uint8 *pdata, uint16 len);

int32 zzStreamGetState(ZZStream stream);


#endif /* __ZZ_STREAM_H__ */

