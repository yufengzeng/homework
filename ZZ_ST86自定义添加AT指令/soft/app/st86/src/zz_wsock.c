#include "zz_wsock.h"

#define ZZ_WSOCK_STREAM                     (ZZ_STREAM_0)
#define ZZ_WSOCK_STREAM_RECV_BUFFER_SIZE    (2048 + 1)

#define ZZ_WSOCK_PING_TIMER                 (ZZ_WSOCK_TASK_TIMERID_BEGIN)

typedef enum 
{
    ZZ_WSOCK_PH_IDLE         = 0,   /* default after wsock created */
    ZZ_WSOCK_PH_GET_IP       = 1,   /* getting ip by domain name */
    ZZ_WSOCK_PH_GOT_IP       = 2,   /* got ip */
    ZZ_WSOCK_PH_OPEN         = 3,   /* open stream */
    ZZ_WSOCK_PH_OPENED       = 4,   /* stream opened */
    ZZ_WSOCK_PH_START        = 5,   /* opening handshake begin */
    ZZ_WSOCK_PH_STARTED      = 6,   /* opening handshake success */
    ZZ_WSOCK_PH_WRITE        = 7,   /* write frame to stream */
    ZZ_WSOCK_PH_WRITTEN      = 8,   /* written */
    ZZ_WSOCK_PH_READING      = 9,   /* reading frame from stream */
    ZZ_WSOCK_PH_READ         = 10,  /* read finished */
    ZZ_WSOCK_PH_STOP         = 11,  /* closing handshake begin */
    ZZ_WSOCK_PH_STOPPED      = 12,  /* closing handshake success */
    ZZ_WSOCK_PH_CLOSE        = 13,  /* closing stream */
    ZZ_WSOCK_PH_CLOSED       = 14   /* stream closed */
} ZZWsockPhase;


typedef struct
{
    /* 
     * public 
     */
     
    char hostname[ZZ_SIZE_HOSTNAME];            /* server ip or domain */
    uint16 port;                                /* server port */
    ZZWsockEventCb event_cb;
    ZZWsockFrame read_frame;                    
    ZZData read_raw;                            /* serialized frame from stream */
    
    /*
     * private 
     */
     
    ZZStream stream;
    ZZWsockPhase phase;
    bool flag_handshaked;                               /* denote websocket upgrade or not */
    bool flag_stopped;                               /* denote stream closed or not */
    uint32 write_raw_length;                    /* serialized frame to write to stream */
    uint32 written_raw_length;                  /* the length written to stream */

    ZZRinterval heartbeat;
    bool enable_hb;
} ZZWsock;

static ZZWsock wsock;
static SL_TASK wsock_task = {{0}};

static void stopPingTimer(void)
{
    SL_StopTimer(wsock_task, ZZ_WSOCK_PING_TIMER);
}

static void restartPingTimer(void)
{
    stopPingTimer();
    SL_StartTimer(wsock_task, 
        ZZ_WSOCK_PING_TIMER, 
        1, 
        SL_SecondToTicks(zzRintVal(&wsock.heartbeat)));
}


static void zzWsockProcessEvent(ZZWsockEvent ev)
{
    //zzLogP("process ev = %u", ev);
    
    /* fill wsock.phase when events occur */
    switch (ev)
    {
        case ZZ_WSOCK_EVT_OPENED:
            wsock.phase = ZZ_WSOCK_PH_OPENED;
            //zzLogP("wsock phase to %u", wsock.phase);
        break;
        case ZZ_WSOCK_EVT_STARTED:
            wsock.phase = ZZ_WSOCK_PH_STARTED;
            //zzLogP("wsock phase to %u", wsock.phase);
        break;
        case ZZ_WSOCK_EVT_WRITTEN:
            wsock.phase = ZZ_WSOCK_PH_WRITTEN;
            //zzLogP("wsock phase to %u", wsock.phase);
        break;
        case ZZ_WSOCK_EVT_READ:
            wsock.phase = ZZ_WSOCK_PH_READ;
            //zzLogP("wsock phase to %u", wsock.phase);
        break;
        case ZZ_WSOCK_EVT_STOPPED:
            wsock.phase = ZZ_WSOCK_PH_STOPPED;
            //zzLogP("wsock phase to %u", wsock.phase);
        break;
        case ZZ_WSOCK_EVT_CLOSED:
            wsock.phase = ZZ_WSOCK_PH_CLOSED;
            //zzLogP("wsock phase to %u", wsock.phase);
        break;
        default:
        break;
    }

    if (!wsock.flag_stopped
        /*|| (wsock.flag_stopped
            && ZZ_WSOCK_EVT_DNSERR != ev
            && ZZ_WSOCK_EVT_OPENERR != ev
            && ZZ_WSOCK_EVT_ABORTED != ev)*/)
    {
        if (wsock.event_cb)
        {
            //zzLogP("ev %u cb begin", ev);
            wsock.event_cb(ev);
            //zzLogP("ev %u cb   end", ev);
        }
    }
    else
    {
        //zzLogI("ev %u ignored(flag_stopped already)", ev);
    }

    if (ZZ_WSOCK_EVT_STARTED == ev)
    {
        if (wsock.enable_hb)
        {
            restartPingTimer();
        }
    }

    if (ZZ_WSOCK_EVT_STOPPED == ev
        || ZZ_WSOCK_EVT_CLOSED == ev)
    {
        stopPingTimer();
    }

    if (ZZ_WSOCK_EVT_STOPPED == ev)
    {
        //zzLogI("wsock close frame received and stop it!");
        if (!wsock.flag_stopped)
        {
            zzWsockStop();
        }
    }
    
    /* Handle Exception Event */
    if (ZZ_WSOCK_EVT_ABORTED == ev
        || ZZ_WSOCK_EVT_OPENERR == ev
        || ZZ_WSOCK_EVT_DNSERR == ev)
    {
        zzLogE("wsock exception %u occurs and stop it!", ev);
        if (!wsock.flag_stopped)
        {
            zzWsockStop();
        }
    }

}


/*
 * Verify frame is valid or not. 
 */
static bool isFrame(ZZWsockFrame frame)
{
    if (ZZ_WSOCK_FRAME_TEXT == frame.type)
    {
        if (!frame.payload.start || !frame.payload.length || frame.payload.length >= 65536)
        {
            zzLogE("payload is bad!");
            return false;
        }
        else
        {
            return true;   
        }        
    }
    else if (ZZ_WSOCK_FRAME_PING == frame.type)
    {
        return true;
    }
    else if (ZZ_WSOCK_FRAME_PONG == frame.type)
    {
        return true;
    }
    else if (ZZ_WSOCK_FRAME_CLOSE == frame.type)
    {
        return true;
    }
    else
    {
        zzLogE("unsupported frame.type = %u", frame.type);
        return false;
    }
}

/*
 * Serialize ZZWsockFrame to WebSocket Protocol Frame for transmission via stream. 
 *
 * @note
 *  Free the data when out of use. 
 */
static void serialize(const ZZWsockFrame frame, ZZData *data)
{
    ZZData ret = ZZ_DATA_NULL;
    uint32 ws_raw_len = 0;
    uint32 index = 0;
    
    if (!data || !isFrame(frame))
    {
        zzLogE(ZZ_ERR_STR_INP);
        return;
    }
    
    if (ZZ_WSOCK_FRAME_TEXT == frame.type)
    {   
        zzLogI("payload len = %u", frame.payload.length);
        zzLogI("payload = [%s]", frame.payload.start);

        /* 
         * Calculate the length of serialized websocket frame 
         */
         
        ws_raw_len = 2; /* FIN, opcode, Payload len */
        
        if (frame.payload.length <= 125)
        {
            ws_raw_len += 0;
        }
        else if (frame.payload.length >= 126 && frame.payload.length <= 65535)
        {
            ws_raw_len += 2;
        }
        else
        {
            zzLogE("frame payload length is too large %u and discard it!", frame.payload.length);
            return;
        }
        
        ws_raw_len += 4; /* MASK-KEY 4 Bytes */
        ws_raw_len += frame.payload.length; /* payload length */


        /* 
         * Package serialized websocket frame 
         */
         
        ret = zzDataCreate(ws_raw_len);
      
        index = 0;
        ret.start[index++] = 0x81;  /* Final package and text data type */
        if (frame.payload.length <= 125)
        {
            ret.start[index++] = 0x80 + frame.payload.length; /* mask set 1 and payload length */    
        }
        else if (frame.payload.length >= 126 && frame.payload.length <= 65535)
        {
            ret.start[index++] = 0x80 + 126;
            ret.start[index++] = (frame.payload.length >> 8) & 0xFF;
            ret.start[index++] = (frame.payload.length) & 0xFF;
        }
        
        ret.start[index++] = 0x00;  /* masking key = 0 */
        ret.start[index++] = 0x00;  /* masking key = 0 */  
        ret.start[index++] = 0x00;  /* masking key = 0 */
        ret.start[index++] = 0x00;  /* masking key = 0 */
        
        SL_Memcpy(&ret.start[index], frame.payload.start, frame.payload.length);
        index += frame.payload.length;
        
        if (index == ws_raw_len)
        {
            //zzLogI("serialized len = %u", index);
        }
        else
        {
            zzLogE("Fatal error with ws_raw_len!");
        }
        
        data->length = ret.length;
        data->start = ret.start;
    }
    else if (ZZ_WSOCK_FRAME_PING == frame.type)
    {
        ret = zzDataCreate(6);
        ret.start[0] = 0x89;
        ret.start[1] = 0x80;
        
        data->length = ret.length;
        data->start = ret.start;
    }
    else if (ZZ_WSOCK_FRAME_PONG == frame.type)
    {
        ret = zzDataCreate(6);
        ret.start[0] = 0x8A;
        ret.start[1] = 0x80;
        
        data->length = ret.length;
        data->start = ret.start;
    }
    else if (ZZ_WSOCK_FRAME_CLOSE == frame.type)
    {
        ret = zzDataCreate(6);
        ret.start[0] = 0x88;
        ret.start[1] = 0x80;
        
        data->length = ret.length;
        data->start = ret.start;
    }
    else
    {
        zzLogE("unsupported frame.type = %u", frame.type);
    }
    
}

static void writePong2Ps(void)
{
    ZZWsockFrame frame;
    zzMemzero(&frame, sizeof(frame));
    frame.type = ZZ_WSOCK_FRAME_PONG;
    if (0 == zzWsockWrite(frame))
    {
        zzLogP("echo pong ok");
    }
    else
    {
        zzLogE("echo pong err");
    }
}

/*
 * Unserialize WebSocket Frame to ZZWsockFrame. 
 *
 * @note
 *  DO NOT free data's memory. 
 */
static void unserialize(ZZData data)
{
    uint32 payload_len = 0;
    ZZData temp = ZZ_DATA_NULL;
    uint32 data_offset = 0;

    if (!data.start || !data.length)
    {
        zzLogE(ZZ_ERR_STR_INP);
        return;
    }
    
    /* Append data to read_raw */
    if (!wsock.read_raw.start)
    {
        wsock.read_raw = zzDataCopy(data);
    }
    else
    {
        temp = wsock.read_raw;
        wsock.read_raw = zzDataMerge(wsock.read_raw, data);
        zzDataFree(&temp);
    }


    while (wsock.read_raw.length)
    {
        if (wsock.read_raw.length < 2)
        {
            zzLogI("need more bytes (< 2)");
            break;
        }
        
        /* Certify the integrality of ws package */
        if (wsock.read_raw.start[0] == 0x81) /* a text frame */
        {
            if (wsock.read_raw.start[1] <= 126)
            {
                if (wsock.read_raw.start[1] <= 125)
                {
                    data_offset = 2;
                    payload_len = wsock.read_raw.start[1];
                }
                else
                {
                    if (wsock.read_raw.length < 4)
                    {
                        zzLogI("need more bytes (< 4)");
                        break;
                    }
                    
                    data_offset = 4;
                    payload_len = wsock.read_raw.start[2]*256 + wsock.read_raw.start[3];
                }

                if (wsock.read_raw.length >= (payload_len + data_offset))
                {
                    wsock.read_frame.type = ZZ_WSOCK_FRAME_TEXT;                
                    wsock.read_frame.payload = zzDataSub(wsock.read_raw, 
                        data_offset, 
                        data_offset + payload_len - 1);
                    
                    if (wsock.read_raw.length == (payload_len + data_offset)) /* no more data left */
                    {
                        zzLogI("the complete frame text");
                        zzDataFree(&wsock.read_raw);
                    }
                    else /* save data left */
                    {
                        temp = wsock.read_raw;
                        wsock.read_raw = zzDataSub(wsock.read_raw, 
                            data_offset + payload_len, 
                            wsock.read_raw.length - 1);
                        zzDataFree(&temp);
                        zzLogI("the complete frame text with next...(%02x)", wsock.read_raw.start[0]);
                    }

                    zzWsockProcessEvent(ZZ_WSOCK_EVT_READ);
                    
                    zzDataFree(&wsock.read_frame.payload);

                    continue;
                }
                else
                {
                    zzLogI("need more bytes text");
                    break;
                }
            }
            else
            {
                zzLogE("unsupported payload len = %u", wsock.read_raw.start[1]);
                zzDataFree(&wsock.read_raw);
                zzWsockProcessEvent(ZZ_WSOCK_EVT_ABORTED);
                break;
            }
            
        }
        else if (wsock.read_raw.start[0] == 0x8A && wsock.read_raw.start[1] == 0x00) /* pong */
        {
            if (wsock.read_raw.length == 2)
            {
                zzLogI("the complete frame pong");
                zzDataFree(&wsock.read_raw);
            }
            else
            {
                temp = wsock.read_raw;
                wsock.read_raw = zzDataSub(wsock.read_raw, 2, wsock.read_raw.length - 1);
                zzDataFree(&temp);
                zzLogI("the complete frame pong with next...(%02x)", wsock.read_raw.start[0]);
            }
            
            wsock.read_frame.type = ZZ_WSOCK_FRAME_PONG;
            //zzWsockProcessEvent(ZZ_WSOCK_EVT_READ);
            continue;
        }
        else if (wsock.read_raw.start[0] == 0x89 && wsock.read_raw.start[1] == 0x00) /* ping */
        {
            if (wsock.read_raw.length == 2)
            {
                zzLogI("the complete frame ping");
                zzDataFree(&wsock.read_raw);
            }
            else
            {
                temp = wsock.read_raw;
                wsock.read_raw = zzDataSub(wsock.read_raw, 2, wsock.read_raw.length - 1);
                zzDataFree(&temp);
                zzLogI("the complete frame ping with next...(%02x)", wsock.read_raw.start[0]);
            }
            
            writePong2Ps();
            wsock.read_frame.type = ZZ_WSOCK_FRAME_PING;
            //zzWsockProcessEvent(ZZ_WSOCK_EVT_READ);
            continue;
        }
        else if (wsock.read_raw.start[0] == 0x88 && wsock.read_raw.start[1] == 0x00) /* close */
        {
            zzLogI("the complete frame connection close");
            zzDataFree(&wsock.read_raw);
            
            wsock.read_frame.type = ZZ_WSOCK_FRAME_CLOSE;
            //zzWsockProcessEvent(ZZ_WSOCK_EVT_STOPPED);
            continue;
        }
        else /* unknown */
        {
            zzLogE("Unknown type of package and ignored![%02X %02X]", 
                wsock.read_raw.start[0], wsock.read_raw.start[1]);
            zzDataFree(&wsock.read_raw);
            
            zzWsockProcessEvent(ZZ_WSOCK_EVT_ABORTED);
            break;
        }
    }
}


int32 zzWsockSetCallback(ZZWsockEventCb event_cb)
{
    if (!event_cb)
    {
        zzLogE(ZZ_ERR_STR_INP);
        return ZZ_ERR_INVALID_PARAM;
    }

    wsock.event_cb = event_cb;
    
    return 0;
}

/**
 * @note
 *  param hostname MUST be ip string such as "54.231.23.99". 
 */
int32 zzWsockSetRemote(const char *hostname, uint16 port)
{
    if (!hostname || !port || SL_Strlen(hostname) >= ZZ_SIZE_HOSTNAME)
    {
        zzLogE(ZZ_ERR_STR_INP);
        return ZZ_ERR_INVALID_PARAM;
    }

    SL_Strcpy(wsock.hostname, hostname);
    wsock.port = port;

    return 0;
}


int32 zzWsockSetHeartbeat(uint32 base_interval)
{
    if (!base_interval)
    {
        zzLogE(ZZ_ERR_STR_INP);
        return ZZ_ERR_INVALID_PARAM;
    }

    zzRintInit(&wsock.heartbeat, base_interval, 0);
    
    if (wsock.enable_hb)
    {
        restartPingTimer();
    }
    
    return 0;
}

void zzWsockEnableHeartbeat(void)
{
    wsock.enable_hb = true;
    
    restartPingTimer();
    
}

void zzWsockDisableHeartbeat(void)
{
    wsock.enable_hb = false;
    stopPingTimer();
}

/**
 * @return 0 for success, negative for failure. 
 * @note
 *  DO NOT free the memory of frame. 
 */
int32 zzWsockWrite(ZZWsockFrame frame)
{
    int32 ret = 0;
    ZZData data = ZZ_DATA_NULL;
    
    if (!isFrame(frame))
    {
        zzLogE(ZZ_ERR_STR_INP);
        return ZZ_ERR_INVALID_PARAM;
    }

    serialize(frame, &data);

    wsock.written_raw_length = 0;
    wsock.write_raw_length = data.length;
    
    ret = zzStreamWrite(wsock.stream, data.start, data.length);
    zzDataFree(&data);
    
    if (ret != wsock.write_raw_length)
    {
        zzLogE("zzStreamWrite err(%d)", ret); 
        return ret;
    }
    
    wsock.phase = ZZ_WSOCK_PH_WRITE;
    //zzLogP("wsock phase to %u", wsock.phase);

    return 0;
}

/**
 * @note
 *  DO NOT free the memory of returned value. 
 */
ZZWsockFrame zzWsockRead(void)
{ 
    return wsock.read_frame;
}

/**
 * @return 0 for success, negative for failure. 
 */
int32 zzWsockWriteText(const char *text)
{
    ZZWsockFrame frame;
    int32 ret = 0;
    
    if (!text)
    {
        zzLogE(ZZ_ERR_STR_INP);
        return ZZ_ERR_INVALID_PARAM;
    }
    
    zzMemzero(&frame, sizeof(frame));
    frame.type = ZZ_WSOCK_FRAME_TEXT;
    frame.payload = zzData((uint8 *)text, SL_Strlen(text));
    
    ret = zzWsockWrite(frame);

    zzDataFree(&frame.payload);
    
    return ret;
}

/**
 * @note
 *  DO NOT free the memory of returned value. 
 */
const char * zzWsockReadText(void)
{
    ZZWsockFrame frame;
    
    zzMemzero(&frame, sizeof(frame));
    frame = zzWsockRead();
    
    if (ZZ_WSOCK_FRAME_TEXT != frame.type)
    {
        zzLogE("frame.type is not text");
        return NULL;
    }
    
    return frame.payload.start;
}

/**
 * Open stream, opening handshake
 */
int32 zzWsockStart(void)
{
    int32 ret = 0;
    
    //zzLogFB();
    
    wsock.flag_stopped = false;
    wsock.flag_handshaked = false;
    
    ret = zzStreamOpen(wsock.stream, wsock.hostname, wsock.port);
    if (ret)
    {
        zzLogE("zzStreamOpen err(%d)", ret);
        zzWsockProcessEvent(ZZ_WSOCK_EVT_OPENERR);
        return ret;
    }
    
    wsock.phase = ZZ_WSOCK_PH_OPEN;
    //zzLogP("wsock phase to %u", wsock.phase);
    
    //zzLogFE();
    
    return 0;
}


/**
 * Close the stream directly. 
 */
int32 zzWsockStop(void)
{
    ZZStreamState state = ZZ_STREAM_IDLE;
    int32 ret = 0;
    
    //zzLogFB();

    if (wsock.flag_stopped)
    {
        zzLogW("wsock flag_stopped already!");
        return 0;
    }

    wsock.flag_handshaked = false;
    wsock.flag_stopped = true;

    state = zzStreamGetState(wsock.stream);
    
    if (ZZ_STREAM_IDLE != state && ZZ_STREAM_CLOSED != state)
    {
        zzLogI("stream %u state:%d and close it now!", wsock.stream, state);
        ret = zzStreamClose(wsock.stream);
        if (ret)
        {
            zzLogE("zzStreamClose err(%d)", ret);
            return ret;
        }
        
        wsock.phase = ZZ_WSOCK_PH_CLOSE;
        //zzLogP("wsock phase to %u", wsock.phase);
    }
    else
    {
        zzLogI("stream %u state:%d and do nothing!", wsock.stream, state);
    }

    //zzLogFE();
    
    return 0;
}

static void openingHandshake(void)
{
    ZZData data = ZZ_DATA_NULL;
    int32 ret = 0;

    wsock.flag_handshaked = false;
    
    data = zzDataCreate(512);
    
    SL_Strcpy(data.start, "GET /api/ws HTTP/1.1\r\n");
    SL_Strcat(data.start, "Host: iotgo.iteadstudio.com\r\n");
    SL_Strcat(data.start, "Connection: upgrade\r\n");
    SL_Strcat(data.start, "Upgrade: websocket\r\n");
    SL_Strcat(data.start, "Sec-WebSocket-Key: ITEADTmobiM0x1DabcEsnw==\r\n");
    SL_Strcat(data.start, "Sec-WebSocket-Version: 13\r\n");
    SL_Strcat(data.start, "\r\n");

    data.length = SL_Strlen(data.start);

    ret = zzStreamWrite(wsock.stream, data.start, data.length);
    if (ret == data.length)
    {
        //zzLogI("opening handshake ok");
        
        wsock.phase = ZZ_WSOCK_PH_START;
        //zzLogP("wsock phase to %u", wsock.phase);
        
    }
    else
    {
        zzLogE("opening handshake err(%d)", ret);
        zzWsockProcessEvent(ZZ_WSOCK_EVT_ABORTED);
    }
    
    zzDataFree(&data);
    
}


static void zzWsockStreamOpenCb(ZZStream stream, bool result, int32 error_code)
{
    //zzLogFB();

    if (!result)
    {
        zzLogE("%s stream = %d, result = %d, error_code = %d", 
            __FUNCTION__, stream, result, error_code);
            
        zzWsockProcessEvent(ZZ_WSOCK_EVT_ABORTED);
        return;
    }
    
    zzWsockProcessEvent(ZZ_WSOCK_EVT_OPENED);
    
    openingHandshake();

    //zzLogFE();
}

static void zzWsockStreamWriteCb(ZZStream stream, bool result, int32 error_code)
{
    //zzLogFB();
    
    if (!result)
    {
        zzLogE("%s stream = %d, result = %d, error_code = %d", 
            __FUNCTION__, stream, result, error_code);
        
        zzWsockProcessEvent(ZZ_WSOCK_EVT_ABORTED);
        
        return;
    }

    if (wsock.flag_handshaked)
    {
        wsock.written_raw_length += error_code;
        //zzLogI("written %d bytes", error_code);

        if (wsock.written_raw_length == wsock.write_raw_length)
        {
            zzWsockProcessEvent(ZZ_WSOCK_EVT_WRITTEN); 
            wsock.written_raw_length = 0;
            wsock.write_raw_length = 0;
        }
    }
    
    //zzLogFE();   
}

/*
 * @note
 *  The length of data received maybe larger than buffer size.
 */
static void zzWsockStreamReadCb(ZZStream stream, bool result, int32 error_code)
{
    static uint8 recv_buf[ZZ_WSOCK_STREAM_RECV_BUFFER_SIZE] = {0};
    ZZData data = ZZ_DATA_NULL;
    int32 ret = 0;
    
    //zzLogFB();
            
    if (!result)
    {
        zzLogE("%s stream = %d, result = %d, error_code = %d", 
            __FUNCTION__, stream, result, error_code);
            
        zzWsockProcessEvent(ZZ_WSOCK_EVT_ABORTED);
        
        return;
    }

    if (wsock.flag_handshaked)
    {
        wsock.phase = ZZ_WSOCK_PH_READING;
        //zzLogP("wsock phase to %u", wsock.phase);
    }
    
    while ((ret = zzStreamRead(stream, recv_buf, sizeof(recv_buf) - 1)) > 0)
    {
        data.length = ret;
        data.start = recv_buf;
        data.start[ret] = '\0';
        
        zzLogI("wsock read len=%u", data.length);

        if (wsock.flag_handshaked)
        {
            unserialize(data);    
        }
        else
        {
            if (0 == SL_Strncmp(data.start, "HTTP/1.1 101", 12))
            {
                wsock.flag_handshaked = true;
                zzWsockProcessEvent(ZZ_WSOCK_EVT_STARTED);
            }
            else
            {
                zzLogE("Fatal err!");
                zzWsockProcessEvent(ZZ_WSOCK_EVT_ABORTED);
            }
        }
            
    }
    
    if (ret < 0)
    {
        if (!wsock.flag_stopped)
        {
            zzLogE("zzStreamRead err(%d)", ret);
            zzWsockProcessEvent(ZZ_WSOCK_EVT_ABORTED);
        }
    }
    
    //zzLogFE();    
    
}

static void zzWsockStreamCloseCb(ZZStream stream, bool result, int32 error_code)
{
    //zzLogFB();
    
    
    if (!result)
    {
        zzLogE("%s stream = %d, result = %d, error_code = %d", 
            __FUNCTION__, stream, result, error_code);
            
        zzWsockProcessEvent(ZZ_WSOCK_EVT_ABORTED);
        
        return;
    }


    zzWsockProcessEvent(ZZ_WSOCK_EVT_CLOSED);
    
    wsock.flag_handshaked = false;
    wsock.flag_stopped = true;
    
    //zzLogFE();
    
}

static void zzWsockTask (void *pdata)
{
    SL_EVENT ev = {0};
    ZZWsockFrame ping;
    int32 ret = 0;
    
    zzLogFB();
    zzPrintCurrentTaskPrio();

    zzMemzero(&ping, sizeof(ping));
    ping.type = ZZ_WSOCK_FRAME_PING;
    
    while (1)
    {
        SL_Memset(&ev, 0, sizeof(ev));
        SL_GetEvent(wsock_task, &ev);
        switch (ev.nEventId)
        {
            case ZZ_EVT_WSOCK_TASK_START:
                zzLogI("ZZ_EVT_WSOCK_TASK_START");
                
            break;
            case ZZ_EVT_WSOCK_TASK_TIMER:
                //zzLogI("ZZ_EVT_WSOCK_TASK_TIMER");
                if (ZZ_WSOCK_PING_TIMER == ev.nParam1)
                {
                    ret = zzWsockWrite(ping);
                    zzLogIE(ret, "send ping");
                }
                
            break;
            default:
                zzLogE("unknown ev = %u", ev.nEventId);
            break;
        }
    }
}


void zzWsockTaskInit(void)
{
    wsock_task.element[0] = SL_CreateTask(zzWsockTask, 
        ZZ_WSOCK_TASK_STACKSIZE, 
        ZZ_WSOCK_TASK_PRIORITY, 
        ZZ_WSOCK_TASK_NAME);

    if (!wsock_task.element[0])
    {
        zzLogE("create zzWsockTask failed!");
    }
}



int32 zzWsockInit(void)
{
    int32 ret = 0;
    
    //zzLogFB();
    
    zzMemzero(&wsock, sizeof(wsock));
    
    wsock.stream = ZZ_WSOCK_STREAM;
    
    wsock.phase = ZZ_WSOCK_PH_IDLE;
    
    wsock.enable_hb = false;
    zzRintInit(&wsock.heartbeat, 20, 0);
    
    ret = zzStreamCreate(wsock.stream);
    zzLogIE(ret, "zzStreamCreate");
    
    ret = zzStreamSetCallback(wsock.stream, 
        zzWsockStreamOpenCb, 
        zzWsockStreamCloseCb, 
        zzWsockStreamWriteCb, 
        zzWsockStreamReadCb);
    zzLogIE(ret, "zzStreamSetCallback");

    
    zzWsockTaskInit();

    //zzLogFE();
    
    return 0;
}

