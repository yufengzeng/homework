#include "zz_stream.h"

typedef struct 
{
    bool is_taken;
    ZZStreamState state;
    ZZStreamOpenCb open_cb;
    ZZStreamCloseCb close_cb;
    ZZStreamWriteCb write_cb;
    ZZStreamReadCb read_cb;
} ZZStreamObj;

static ZZLock zz_streams_lock;
static ZZStreamObj zz_streams[ZZ_STREAM_MAX] = {{0}};

#define lockStreams() \
    do \
    { \
        /*zzLogP("%s,%d lo %d", __FUNCTION__, __LINE__, zz_streams_lock.mutex);*/  \
        zzLock(&zz_streams_lock); \
        /*zzLogP("%s,%d ld by %d", __FUNCTION__, __LINE__, zz_streams_lock.user);*/  \
    } while(0)

#define unlockStreams() \
    do \
    { \
        /*zzLogP("%s,%d ul %d,%d", __FUNCTION__, __LINE__, zz_streams_lock.mutex, zz_streams_lock.user);*/ \
        zzUnlock(zz_streams_lock); \
    } while(0)

static bool isStream(ZZStream stream)
{
    if (stream >= 0 && stream < ZZ_STREAM_MAX)
    {
        return true;
    }
    else
    {
        return false;
    }
}

int32 zzStreamGetState(ZZStream stream)
{
    ZZStreamState state = ZZ_STREAM_IDLE;

    if (!isStream(stream))
    {
        return ZZ_ERR_STREAM_INVALID;
    }
    
    state = zz_streams[stream].state;
    
    return state;
}



int32 zzStreamCreate(ZZStream stream)
{
    if (!isStream(stream))
    {
         return ZZ_ERR_STREAM_INVALID;
    }

    lockStreams();
    if (zz_streams[stream].is_taken)
    {
        unlockStreams();
        return ZZ_ERR_STREAM_TAKEN;
    }

    zz_streams[stream].is_taken = true;
    zz_streams[stream].state = ZZ_STREAM_IDLE;
    zz_streams[stream].open_cb = NULL;
    zz_streams[stream].close_cb = NULL;
    zz_streams[stream].write_cb = NULL;
    zz_streams[stream].read_cb = NULL;
    unlockStreams();
    
    return 0;
}

/**
 * @warning
 *  Before calling this, be sure that stream has been closed totally. 
 */
int32 zzStreamRelease(ZZStream stream)
{
    if (!isStream(stream))
    {
         return ZZ_ERR_STREAM_INVALID;
    }

    lockStreams();

    if (!zz_streams[stream].is_taken)
    {
        unlockStreams();
        return ZZ_ERR_STREAM_UNTAKEN;
    }

    zz_streams[stream].is_taken = false;
    zz_streams[stream].state = ZZ_STREAM_IDLE;
    zz_streams[stream].open_cb = NULL;
    zz_streams[stream].close_cb = NULL;
    zz_streams[stream].write_cb = NULL;
    zz_streams[stream].read_cb = NULL;

    unlockStreams();
    return 0;
}

int32 zzStreamSetCallback(ZZStream stream, 
    ZZStreamOpenCb open_cb,
    ZZStreamCloseCb close_cb,
    ZZStreamWriteCb write_cb,
    ZZStreamReadCb read_cb)
{
    if (!isStream(stream))
    {
         return ZZ_ERR_STREAM_INVALID;
    }
    
    lockStreams();

    if (!zz_streams[stream].is_taken)
    {
        unlockStreams();
        return ZZ_ERR_STREAM_UNTAKEN;
    }

    zz_streams[stream].open_cb = open_cb;
    zz_streams[stream].close_cb = close_cb;
    zz_streams[stream].write_cb = write_cb;
    zz_streams[stream].read_cb = read_cb;
    unlockStreams();
    
    return 0;
}

int32 zzStreamClrCallback(ZZStream stream)
{
    if (!isStream(stream))
    {
         return ZZ_ERR_STREAM_INVALID;
    }

    lockStreams();

    if (!zz_streams[stream].is_taken)
    {
        unlockStreams();
        return ZZ_ERR_STREAM_UNTAKEN;
    }

    zz_streams[stream].open_cb = NULL;
    zz_streams[stream].close_cb = NULL;
    zz_streams[stream].write_cb = NULL;
    zz_streams[stream].read_cb = NULL;
    
    unlockStreams();

    return 0;
}

/**
 * Create socket, connect
 */
int32 zzStreamOpen(ZZStream stream, const char *ip, uint16 port)
{
    int32 ret = 0;

    if (!isStream(stream))
    {
         return ZZ_ERR_STREAM_INVALID;
    }
    
    if (!ip || !port)
    {
        return ZZ_ERR_INVALID_PARAM;
    }
    
    lockStreams();
    ret = SL_TcpipSocketCreate(stream, SL_TCPIP_SOCK_TCP);
    if (ret < 0)
    {
        unlockStreams();
        return ret;
    }

    ret = SL_TcpipSocketConnect(stream, (uint8 *)ip, port);
    if (ret)
    {
        unlockStreams();
        return ret;
    }

    zz_streams[stream].state = ZZ_STREAM_OPEN;
    unlockStreams();
    
    return 0;
}

int32 zzStreamClose(ZZStream stream)
{
    int32 ret = 0;
    
    if (!isStream(stream))
    {
         return ZZ_ERR_STREAM_INVALID;
    }

    lockStreams();
    ret = SL_TcpipSocketClose(stream);
    if (ret)
    {
        unlockStreams();
        return ret;
    }

    zz_streams[stream].state = ZZ_STREAM_CLOSE;
    unlockStreams();
    
    return 0;
}

int32 zzStreamWrite(ZZStream stream, uint8 *pdata, uint16 len)
{
    int32 ret = 0;
    
    if (!isStream(stream))
    {
         return ZZ_ERR_STREAM_INVALID;
    }

    if (!pdata || !len)
    {
        return ZZ_ERR_INVALID_PARAM;
    }

    ret = SL_TcpipSocketSend(stream, pdata, len);
    
    return ret;
}

/**
 * @note
 *  
 */
int32 zzStreamRead(ZZStream stream, uint8 *pdata, uint16 len)
{
    int32 ret = 0;
    
    if (!isStream(stream))
    {
         return ZZ_ERR_STREAM_INVALID;
    }

    if (!pdata || !len)
    {
        return ZZ_ERR_INVALID_PARAM;
    }

    ret = SL_TcpipSocketRecv(stream, pdata, len);
    
    return ret;
}

void zzStreamOpenCb(U8 cid, S32 stream, BOOL result, S32 error_code)
{
    lockStreams();
    zz_streams[stream].state = ZZ_STREAM_OPENED;
    
    if (zz_streams[stream].open_cb)
    {
        zz_streams[stream].open_cb(stream, result, error_code);
    }
    unlockStreams();
}

void zzStreamCloseCb(U8 cid, S32 stream, BOOL result, S32 error_code)
{
    lockStreams();

    if (zz_streams[stream].close_cb)
    {
        zz_streams[stream].close_cb(stream, result, error_code);
    }

    zz_streams[stream].state = ZZ_STREAM_CLOSED;
    
    unlockStreams();
}

void zzStreamWriteCb(U8 cid, S32 stream, BOOL result, S32 error_code)
{
    lockStreams();

    if (zz_streams[stream].write_cb)
    {
        zz_streams[stream].write_cb(stream, result, error_code);
    }
    
    unlockStreams();
}

void zzStreamReadCb(U8 cid, S32 stream, BOOL result, S32 error_code)
{
    lockStreams();

    if (zz_streams[stream].read_cb)
    {
        zz_streams[stream].read_cb(stream, result, error_code);
    }
    
    unlockStreams();
}

void zzStreamInit(void)
{
    zzMemzero(zz_streams, sizeof(zz_streams));
    zz_streams_lock = zzCreateLock();
}

