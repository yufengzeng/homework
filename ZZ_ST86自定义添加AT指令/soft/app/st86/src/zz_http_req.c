#include "zz_http_req.h"

#define ZZ_HTTP_REQ_TIMEOUT_DEFAULT     (30)
#define ZZ_HTTP_STREAM_RECV_BUFFER_SIZE (2048 + 1)

typedef struct ZZHttpResponse
{
    bool head_is_started;                       /* indicate response's head is started or not */
    bool head_is_complete;                      /* indicate response's head is complete or not */
    bool body_is_started;                       /* indicate response's body is started or not */
    ZZHttpResEndIndicator res_end_indicator;
    uint16 status_code;                         /* status code */
    uint32 content_length;                      /* Content-Length value */
    ZZData head;                                /* HTTP response head */
    ZZData body;                                /* HTTP response body */

    /* inner */
    uint32 body_counter;
} ZZHttpResponse; 

typedef enum 
{
    ZZ_HTTP_REQ_PH_IDLE         = 0,    /* default after request created */
    ZZ_HTTP_REQ_PH_SUBMITTED    = 1,    /* submit ok */
    ZZ_HTTP_REQ_PH_GET_IP       = 2,    /* getting ip by domain name */
    ZZ_HTTP_REQ_PH_GOT_IP       = 3,    /* got ip */
    ZZ_HTTP_REQ_PH_OPEN         = 4,    /* open stream */
    ZZ_HTTP_REQ_PH_OPENED       = 5,    /* stream opened */
    ZZ_HTTP_REQ_PH_WRITE        = 6,    /* write request to stream */
    ZZ_HTTP_REQ_PH_WRITTEN      = 7,    /* write ok */
    ZZ_HTTP_REQ_PH_RES_BEGIN    = 8,    /* response begin */
    ZZ_HTTP_REQ_PH_RES_HEAD     = 9,    /* got the entire head */
    ZZ_HTTP_REQ_PH_RES_BODY     = 10,   /* got a part of body */
    ZZ_HTTP_REQ_PH_RES_END      = 11,   /* response end */
    ZZ_HTTP_REQ_PH_CLOSE        = 12,   /* close stream */
    ZZ_HTTP_REQ_PH_CLOSED       = 13    /* stream closed */
} ZZHttpReqPhase;

typedef struct ZZHttpRequest
{
    /* 
     * public 
     */
     
    ZZStream stream;
    char *hostname;                             /* server ip or domain */
    uint16 port;                                /* server port */
    uint32 timeout;                             /* request timeout millisecond */
    ZZHttpReqEventCb event_cb;
    char *method;                               /* ALL HTTP method supported */                     
    char *path;
    ZZKeyStr args[ZZ_HTTP_REQ_ARG_MAX];
    ZZKeyStr headers[ZZ_HTTP_REQ_HEADER_MAX];
    ZZData body;                                /* HTTP request body */
    
    ZZHttpResponse response;

    /*
     * private 
     */
     
    char ip[ZZ_SIZE_IP];
    ZZHttpReqPhase phase;
    uint32 length;                              /* the entire request length */
    uint32 written_length;                      /* the length written to stream */
    bool res_is_end;                            /* indicate response is end or not */
    bool flag_finish;                           /* indicate the request finished or not */    


    /* Connection:keep-alive/close */
    bool flag_keep_alive;
    bool flag_reopen;
    char *current_hostname;
    uint16 current_port;
} ZZHttpRequest;

static SL_TASK http_req_task = {{0}};
static ZZLock http_req_lock = ZZ_LOCK_NULL;
static ZZHttpRequest *http_req_list[ZZ_HTTP_REQ_HANDLE_MAX] = {0};

#define lockReqList() \
    do \
    { \
        /*zzLogP("%s,%d lo %d", __FUNCTION__, __LINE__, http_req_lock.mutex);*/  \
        zzLock(&http_req_lock); \
        /*zzLogP("%s,%d ld by %d", __FUNCTION__, __LINE__, http_req_lock.user);*/  \
    } while(0)

#define unlockReqList() \
    do \
    { \
        /*zzLogP("%s,%d ul %d,%d", __FUNCTION__, __LINE__, http_req_lock.mutex, http_req_lock.user);*/ \
        zzUnlock(http_req_lock); \
    } while(0)


static bool isHandle(ZZHttpReqHandle req_handle)
{
    if (req_handle >= 0 && req_handle < ZZ_HTTP_REQ_HANDLE_MAX)
    {
        return true;
    }
    else
    {
        return false;
    }
}

static bool isArgIndex(uint8 index)
{
    if (index >=0 && index < ZZ_HTTP_REQ_ARG_MAX)
    {
        return true;
    }
    else
    {
        return false;
    }
}

static bool isHeaderIndex(uint8 index)
{
    if (index >=0 && index < ZZ_HTTP_REQ_HEADER_MAX)
    {
        return true;
    }
    else
    {
        return false;
    }
}

static bool checkConnectionField(ZZHttpRequest *request)
{
    uint32 i = 0;
    
    for (i = 0; i < ZZ_HTTP_REQ_HEADER_MAX; i++)
    {
        if (!request->headers[i].key || !request->headers[i].str)
        {
            continue;
        }
        
        if (0 == SL_Strcmp(request->headers[i].key, "Connection"))
        {
            if (0 == SL_Strcmp(request->headers[i].str, "keep-alive"))
            {
                request->flag_keep_alive = true;
                return true;
            }
            else if (0 == SL_Strcmp(request->headers[i].str, "close"))
            {
                request->flag_keep_alive = false;
                return true;
            }
            else if (0 == SL_Strcmp(request->headers[i].str, "upgrade"))
            {
                request->flag_keep_alive = true;
                return true;
            }
            else
            {
                zzLogE("unsupported Connection value [%s]", request->headers[i].str);
                return false;
            }

            break;
        }
    }

    return false;
}

/*
 * Check request's hostname, port, method, path, Connection field. 
 */
static bool initForSubmit(ZZHttpRequest *request)
{
    if (!request)
    {
        zzLogE(ZZ_ERR_STR_INP);
        return false;
    }

    if (!request->hostname)
    {
        zzLogE("bad hostname");
        return false;
    }

    if (!request->path)
    {
        zzLogE("bad path");
        return false;
    }

    if (!request->method)
    {
        zzLogE("bad method");
        return false;
    }

    if (!request->port)
    {
        zzLogE("bad port");
        return false;
    }

    if (!checkConnectionField(request))
    {
        zzLogE("bad Connection field");
        return false;
    }

    if (request->response.head.start)
    {
        zzDataFree(&request->response.head);
    }
    
    if (request->response.body.start)
    {
        zzDataFree(&request->response.body);
    }

    zzMemzero(&request->response, sizeof(request->response));

    zzMemzero(request->ip, sizeof(request->ip));
    request->length = 0;
    request->written_length = 0;
    request->flag_finish = false;
    request->res_is_end = false;
    request->phase = ZZ_HTTP_REQ_PH_SUBMITTED;
    //zzLogP("req phase to %u", request->phase);

    if (!request->timeout)
    {
        request->timeout = ZZ_HTTP_REQ_TIMEOUT_DEFAULT;
        //zzLogI("set timeout to default: %u", ZZ_HTTP_REQ_TIMEOUT_DEFAULT);
    }

    return true;
}

static ZZHttpRequest * getRequest(ZZHttpReqHandle req_handle)
{
    ZZHttpRequest *request = NULL;
    
    if (!isHandle(req_handle))
    {
        zzLogE(ZZ_ERR_STR_INP);
        return NULL;
    }
    
    request = http_req_list[req_handle];

    return request;
}

static ZZHttpRequest * registerRequest(ZZHttpReqHandle req_handle, ZZHttpRequest *request)
{
    if (http_req_list[req_handle])
    {
        zzLogE("register req failed");
        return NULL;
    }
    http_req_list[req_handle] = request;
    return request;
}

static void unregisterRequest(ZZHttpReqHandle req_handle)
{
    http_req_list[req_handle] = NULL;
}


static ZZHttpReqHandle getHandle(ZZStream stream)
{
    return stream;
}

ZZHttpResEndIndicator zzHttpResGetEndIndicator(ZZHttpReqHandle req_handle)
{
    ZZHttpRequest *request = NULL;
    ZZHttpResEndIndicator ret = 0;

    lockReqList();

    request = getRequest(req_handle);
    if (!request)
    {
        zzLogE(ZZ_ERR_STR_BADREQOHANDLE);
        unlockReqList();
        return ZZ_HTTP_RES_END_UNSUPPORTED;
    }

    ret = request->response.res_end_indicator;
    
    unlockReqList();

    return ret;
}

uint16 zzHttpResGetStatus(ZZHttpReqHandle req_handle)
{
    ZZHttpRequest *request = NULL;
    uint16 status = 0;

    lockReqList();

    request = getRequest(req_handle);
    if (!request)
    {
        zzLogE(ZZ_ERR_STR_BADREQOHANDLE);
        unlockReqList();
        return 0;
    }
    status = request->response.status_code;
    unlockReqList();

    return status;
}


uint32 zzHttpResGetContentLength(ZZHttpReqHandle req_handle)
{
    ZZHttpRequest *request = NULL;
    uint32 cl = 0;

    lockReqList();

    request = getRequest(req_handle);
    if (!request)
    {
        zzLogE(ZZ_ERR_STR_BADREQOHANDLE);
        unlockReqList();
        return 0;
    }
    cl = request->response.content_length;
    unlockReqList();
    
    return cl;

}

/**
 * Return the enitre head of response. 
 * 
 * @warning
 *  DO NOT free the memory of returned value. 
 */
ZZData zzHttpResGetHead(ZZHttpReqHandle req_handle)
{
    ZZHttpRequest *request = NULL;
    ZZData ret = ZZ_DATA_NULL;
    
    lockReqList();

    request = getRequest(req_handle);
    if (!request)
    {
        zzLogE(ZZ_ERR_STR_BADREQOHANDLE);
        unlockReqList();
        return ret;
    }

    ret = request->response.head;
    unlockReqList();
    return ret;
        
}

/**
 * Return a part of body received, NOT THE ENTIRE BODY. 
 *
 * @warning
 *  DO NOT free the memory of returned value. 
 */
ZZData zzHttpResGetBody(ZZHttpReqHandle req_handle)
{
    ZZHttpRequest *request = NULL;
    ZZData ret = ZZ_DATA_NULL;

    lockReqList();
    
    request = getRequest(req_handle);
    if (!request)
    {
        zzLogE(ZZ_ERR_STR_BADREQOHANDLE);
        unlockReqList();
        return ret;
    }
    ret = request->response.body;
    unlockReqList();
    return ret;
}


/*
 * simultaneously called in socket task and req task. 
 */
static void zzHttpReqProcessEvent(ZZHttpReqHandle req_handle, ZZHttpReqEvent ev)
{
    ZZHttpRequest *request = NULL;
    
    //zzLogP("process ev = %u", ev);
    
    request = getRequest(req_handle);
    if (!request)
    {
        zzLogE(ZZ_ERR_STR_BADREQOHANDLE);
        return;
    }

    /* fill request->phase when events occur */
    switch (ev)
    {
        case ZZ_HTTP_REQ_EVT_OPENED:
            request->phase = ZZ_HTTP_REQ_PH_OPENED;
            //zzLogP("req phase to %u", request->phase);
        break;
        case ZZ_HTTP_REQ_EVT_WRITTEN:
            request->phase = ZZ_HTTP_REQ_PH_WRITTEN;
            //zzLogP("req phase to %u", request->phase);
        break;
        case ZZ_HTTP_REQ_EVT_RES_BEGIN:
            request->phase = ZZ_HTTP_REQ_PH_RES_BEGIN;
            //zzLogP("req phase to %u", request->phase);
        break;
        case ZZ_HTTP_REQ_EVT_RES_HEAD:
            request->phase = ZZ_HTTP_REQ_PH_RES_HEAD;
            //zzLogP("req phase to %u", request->phase);
        break;
        case ZZ_HTTP_REQ_EVT_RES_BODY:
            request->phase = ZZ_HTTP_REQ_PH_RES_BODY;
            //zzLogP("req phase to %u", request->phase);
        break;
        case ZZ_HTTP_REQ_EVT_RES_END:
            request->phase = ZZ_HTTP_REQ_PH_RES_END;
            //zzLogP("req phase to %u", request->phase);
        break;
        case ZZ_HTTP_REQ_EVT_CLOSED:
            request->phase = ZZ_HTTP_REQ_PH_CLOSED;
            //zzLogP("req phase to %u", request->phase);
        break;
        default:
        break;
    }

    /* stop timer when some events occur */
    if (ZZ_HTTP_REQ_EVT_RES_END == ev
        || ZZ_HTTP_REQ_EVT_DNSERR == ev
        || ZZ_HTTP_REQ_EVT_OPENERR == ev
        || ZZ_HTTP_REQ_EVT_ABORTED == ev
        || ZZ_HTTP_REQ_EVT_CLOSED == ev)
    {
        SL_StopTimer(http_req_task, req_handle);
    }

    if (ZZ_HTTP_REQ_EVT_RES_END == ev)
    {
        request->res_is_end = true;
    }

    /* call request event callback */
    if (!request->flag_finish
        /*|| (request->flag_finish
            && ZZ_HTTP_REQ_EVT_DNSERR != ev
            && ZZ_HTTP_REQ_EVT_OPENERR != ev
            && ZZ_HTTP_REQ_EVT_ABORTED != ev
            && ZZ_HTTP_REQ_EVT_TIMEOUT != ev)*/)
    {
        if (request->event_cb)
        {
            //zzLogP("ev %u cb begin", ev);
            request->event_cb(req_handle, ev);
            //zzLogP("ev %u cb   end", ev);
        }
    }
    else
    {
        //zzLogI("ev %u ignored(finished already)", ev);
    }

    /* close stream after ZZ_HTTP_REQ_EVT_RES_END if flag_keep_alive is false */
    if (ZZ_HTTP_REQ_EVT_RES_END == ev)
    {
        if (!request->flag_keep_alive 
            && ZZ_HTTP_RES_END_CONN_CLOSE != request->response.res_end_indicator)
        {
            if (!request->flag_finish)
            {
                zzLogI("response end and finish it now!");
                zzHttpReqFinish(req_handle);
            }
        }
    }
    
    /* Handle Exception Event */
    if (ZZ_HTTP_REQ_EVT_ABORTED == ev
        || ZZ_HTTP_REQ_EVT_TIMEOUT == ev
        || ZZ_HTTP_REQ_EVT_OPENERR == ev
        || ZZ_HTTP_REQ_EVT_DNSERR == ev)
    {
        if (!request->flag_finish)
        {
            zzLogI("request exception %u occurs and finish it now!", ev);
            zzHttpReqFinish(req_handle);
        }
    }

}

int32 zzHttpReqSetCallback(ZZHttpReqHandle req_handle, ZZHttpReqEventCb event_cb)
{
    ZZHttpRequest *request = NULL;

    lockReqList();
    request = getRequest(req_handle);
    
    if (!request)
    {
        zzLogE(ZZ_ERR_STR_BADREQOHANDLE);
        unlockReqList();
        return ZZ_ERR_INVALID_PARAM;
    }

    request->event_cb = event_cb;
    unlockReqList();
    
    return 0;
}    


/**
 * @note
 *  parameter(s) are copied to new memory freed by zzHttpReqRelease
 */
int32 zzHttpReqSetHostname(ZZHttpReqHandle req_handle, const char *hostname)
{
    ZZHttpRequest *request = NULL;

    
    lockReqList();
    request = getRequest(req_handle);

    if (!request || !hostname)
    {
        zzLogE(ZZ_ERR_STR_INP);
        unlockReqList();
        return ZZ_ERR_INVALID_PARAM;
    }

    if (request->hostname)
    {
        SL_FreeMemory(request->hostname);
    }
    
    request->hostname = zzStringCopy(hostname);
    unlockReqList();

    

    return 0;
}

int32 zzHttpReqSetPort(ZZHttpReqHandle req_handle, uint16 port)
{
    ZZHttpRequest *request = NULL;

    
    lockReqList();
    request = getRequest(req_handle);

    if (!request || !port)
    {
        zzLogE(ZZ_ERR_STR_INP);
        unlockReqList();
        return ZZ_ERR_INVALID_PARAM;
    }

    request->port = port;
    unlockReqList();

    

    return 0;
}

int32 zzHttpReqSetTimeout(ZZHttpReqHandle req_handle, uint32 timeout)
{
    ZZHttpRequest *request = NULL;

    lockReqList();
    
    request = getRequest(req_handle);

    if (!request)
    {
        zzLogE(ZZ_ERR_STR_BADREQOHANDLE);
        unlockReqList();
        return ZZ_ERR_INVALID_PARAM;
    }

    request->timeout = timeout;

    unlockReqList();
    
    return 0;
}

/**
 * @note
 *  parameter(s) are copied to new memory freed by zzHttpReqRelease
 */
int32 zzHttpReqSetMethod(ZZHttpReqHandle req_handle, const char *method)
{
    ZZHttpRequest *request = NULL;

    lockReqList();
    
    request = getRequest(req_handle);

    if (!request || !method)
    {
        zzLogE(ZZ_ERR_STR_INP);
        unlockReqList();
        return ZZ_ERR_INVALID_PARAM;
    }

    if (request->method)
    {
        SL_FreeMemory(request->method);
    }
    
    request->method = zzStringCopy(method);
    
    unlockReqList();
    return 0;
}

/**
 * @note
 *  parameter(s) are copied to new memory freed by zzHttpReqRelease
 */
int32 zzHttpReqSetPath(ZZHttpReqHandle req_handle, const char *path)
{    
    ZZHttpRequest *request = NULL;
    
    
    lockReqList();
    request = getRequest(req_handle);

    if (!request || !path)
    {
        zzLogE(ZZ_ERR_STR_INP);
        unlockReqList();
        return ZZ_ERR_INVALID_PARAM;
    }

    if (request->path)
    {
        SL_FreeMemory(request->path);
    }
    
    request->path = zzStringCopy(path);

    unlockReqList();

    return 0;
}

/**
 * @note
 *  parameter(s) are copied to new memory freed by zzHttpReqRelease
 */
int32 zzHttpReqSetArg(ZZHttpReqHandle req_handle, uint8 index, const char *key, const char *str)
{
    ZZHttpRequest *request = NULL;

    lockReqList();
    request = getRequest(req_handle);

    if (!request 
        || !isArgIndex(index)
        || !key
        || !str)
    {
        zzLogE(ZZ_ERR_STR_INP);
        unlockReqList();
        return ZZ_ERR_INVALID_PARAM;
    }

    if (request->args[index].key)
    {
        SL_FreeMemory(request->args[index].key);
        SL_FreeMemory(request->args[index].str);
    }

    request->args[index].key = zzStringCopy(key);
    request->args[index].str = zzStringCopy(str);

    unlockReqList();
    
    return 0;
}

/**
 * @note
 *  parameter(s) are copied to new memory freed by zzHttpReqRelease
 */
int32 zzHttpReqSetHeader(ZZHttpReqHandle req_handle, uint8 index, const char *key, const char *str)
{
    ZZHttpRequest *request = NULL;

    
    lockReqList();
    request = getRequest(req_handle);

    if (!request 
        || !isHeaderIndex(index)
        || !key
        || !str)
    {
        zzLogE(ZZ_ERR_STR_INP);
        unlockReqList();
        return ZZ_ERR_INVALID_PARAM;
    }

    if (request->headers[index].key)
    {
        SL_FreeMemory(request->headers[index].key);
        SL_FreeMemory(request->headers[index].str);
    }

    request->headers[index].key = zzStringCopy(key);
    request->headers[index].str = zzStringCopy(str);

    unlockReqList();
    
    return 0;
}

/**
 * @note
 *  body are copied to new memory freed by zzHttpReqRelease
 */
int32 zzHttpReqSetBody(ZZHttpReqHandle req_handle, ZZData body)
{
    ZZHttpRequest *request = NULL;
    
    lockReqList();
    request = getRequest(req_handle);

    if (!request 
        || !body.start
        || !body.length)
    {
        zzLogE(ZZ_ERR_STR_INP);
        unlockReqList();
        return ZZ_ERR_INVALID_PARAM;
    }

    if (request->body.start)
    {
        zzDataFree(&request->body);
    }

    request->body = zzDataCopy(body);

    unlockReqList();
    
    return 0;
}


/**
 * Output http request from ZZHttpRequest. 
 * 
 * @note
 *  1. Free data->start memory when out of use.
 *  2. HTTP version is 1.1.
 */
static int32 serialize(ZZHttpRequest *request, ZZData *data)
{
    uint32 total_len = 0;
    uint32 head_len = 0;
    uint32 i = 0;
    ZZData output = ZZ_DATA_NULL;
    uint32 data_index = 0;
    bool is_first_arg = true;

    if (!request || !data)
    {
        zzLogE(ZZ_ERR_STR_INP);
        return ZZ_ERR_INVALID_PARAM;
    }

    /* 
     * calculate head length (method + path + args + version + headers) 
     */
    head_len = 0;

    head_len += SL_Strlen(request->method); /* method */
        
    head_len++; /* space */
    head_len += SL_Strlen(request->path); /* path */

    /* ?key1=str1&key2=str2 */
    for (i = 0; i < ZZ_HTTP_REQ_ARG_MAX; i++)
    {
        if (!request->args[i].key)
        {
            continue;
        }
        
        head_len++; /* ? or & */
        
        head_len += SL_Strlen(request->args[i].key); /* key */
        head_len++; /* = */
        head_len += SL_Strlen(request->args[i].str); /* str */
    }

    head_len++; /* space */
    head_len += 8; /* HTTP/1.1 */    
    head_len += 2; /* \r\n */  

    /* headers key: str\r\n */
    for (i = 0; i < ZZ_HTTP_REQ_HEADER_MAX; i++)
    {
        if (!request->headers[i].key)
        {
            continue;
        }
                
        head_len += SL_Strlen(request->headers[i].key); /* key */
        head_len += 2; /* :<space> */
        head_len += SL_Strlen(request->headers[i].str); /* str */
        head_len += 2; /* \r\n */
    }
    
    head_len += 2; /* \r\n */

    /* 
     * calculate total len of http req 
     */
    
    total_len = head_len + request->body.length;

    /*
     * allocate memory for http req
     */
    output.length = total_len;
    output.start = (uint8 *)SL_GetMemory(output.length + 1);
    if (!output.start)
    {
        zzLogE(ZZ_ERR_STR_OOM);
        return ZZ_ERR_OUT_OF_MEMORY;
    }

    zzMemzero(output.start, output.length + 1);

    /*
     * copy all fields to output
     */
    data_index = 0;

    SL_Memcpy(&output.start[data_index], request->method, SL_Strlen(request->method));
    data_index += SL_Strlen(request->method);  /* method */  
    
    SL_Memcpy(&output.start[data_index], " ", 1);
    data_index++; /* space */
    
    SL_Memcpy(&output.start[data_index], request->path, SL_Strlen(request->path));
    data_index += SL_Strlen(request->path); /* path */
    
    /* ?key1=str1&key2=str2 */
    for (i = 0; i < ZZ_HTTP_REQ_ARG_MAX; i++)
    {
        if (!request->args[i].key)
        {
            continue;
        }
        
        if (is_first_arg)
        {
            is_first_arg = false;
            SL_Memcpy(&output.start[data_index], "?", 1);
            data_index++; /* ? */
        }
        else
        {
            SL_Memcpy(&output.start[data_index], "&", 1);
            data_index++; /* & */
        }
        
        SL_Memcpy(&output.start[data_index], request->args[i].key, SL_Strlen(request->args[i].key));
        data_index += SL_Strlen(request->args[i].key); /* key */
        
        SL_Memcpy(&output.start[data_index], "=", 1);
        data_index++; /* = */
        
        SL_Memcpy(&output.start[data_index], request->args[i].str, SL_Strlen(request->args[i].str));
        data_index += SL_Strlen(request->args[i].str); /* str */
    }

    SL_Memcpy(&output.start[data_index], " HTTP/1.1\r\n", 11);
    data_index += 11; /* <space>HTTP/1.1\r\n */

    /* headers key: str\r\n */
    for (i = 0; i < ZZ_HTTP_REQ_HEADER_MAX; i++)
    {
        if (!request->headers[i].key)
        {
            continue;
        }

        SL_Memcpy(&output.start[data_index], request->headers[i].key, SL_Strlen(request->headers[i].key));
        data_index += SL_Strlen(request->headers[i].key); /* key */

        SL_Memcpy(&output.start[data_index], ": ", 2);
        data_index += 2; /* :<space> */

        SL_Memcpy(&output.start[data_index], request->headers[i].str, SL_Strlen(request->headers[i].str));
        data_index += SL_Strlen(request->headers[i].str); /* str */

        SL_Memcpy(&output.start[data_index], "\r\n", 2);
        data_index += 2; /* \r\n */
        
    }
    
    SL_Memcpy(&output.start[data_index], "\r\n", 2);
    data_index += 2; /* \r\n */

    zzLogI("http req head[%s]", output.start);

    SL_Memcpy(&output.start[data_index], request->body.start, request->body.length);
    data_index += request->body.length; /* body */

    if (data_index != output.length)
    {
        zzLogE("http req length err(%u != %u)", data_index, output.length);
    }

    data->length = output.length;
    data->start = output.start;

    return 0;
}

/**
 * HTTP/1.1 200 OK\r\n
 */
static int32 parseStatusCode(const ZZData head)
{
    char str[10] = {0};
    int32 ret = 0;
    
    if (!head.start || !head.length)
    {
        zzLogE(ZZ_ERR_STR_INP);
        return ZZ_ERR_INVALID_PARAM;
    }
        
    SL_Strncpy(str, head.start + 9, 3);
    
    ret = SL_atoi(str);
    
    return ret;
}


/**
 * Content-Length: 123\r\n
 */
static int32 parseContentLength(const ZZData head)
{
    char *clstr = NULL;
    char *crlf = NULL;
    char str[10] = {0};
    int32 ret = 0;
    
    if (!head.start || !head.length)
    {
        zzLogE(ZZ_ERR_STR_INP);
        return ZZ_ERR_INVALID_PARAM;
    }
    
    clstr = SL_Strstr(head.start, "Content-Length: ");
    if (!clstr)
    {
        zzLogE("no Content-Length found");
        return ZZ_ERR_HTTP_BAD_FORMAT;
    }

    crlf = SL_Strstr(clstr + 16, "\r\n");
    if (!crlf)
    {
        zzLogE("no <crlf> found");
        return ZZ_ERR_HTTP_BAD_FORMAT;
    }
    
    SL_Strncpy(str, clstr + 16, crlf - (clstr + 16));
    
    ret = SL_atoi(str);
    
    return ret;
}

/**
 * Parse response end indicator
 */
static ZZHttpResEndIndicator parseResEndIndicator(const char *method, const ZZData head)
{
    ZZHttpResEndIndicator ret = ZZ_HTTP_RES_END_UNSUPPORTED;
    int32 cl = 0;
    int32 status_code = 0;
    char *conn_close = NULL;
    
    if (!head.start || !head.length)
    {
        zzLogE(ZZ_ERR_STR_INP);
        return ret;
    }
        
    status_code = parseStatusCode(head);
    if ((0 == SL_Strcmp(method, "HEAD"))
        || (status_code >= 100 && status_code <= 199)
        || 204 == status_code 
        || 304 == status_code
        )
    {
        ret = ZZ_HTTP_RES_END_NO_BODY;
        return ret;
    }
    
    cl = parseContentLength(head);
    if (cl >= 0)
    {
        ret = ZZ_HTTP_RES_END_CL;
        return ret;
    }
    

    conn_close = SL_Strstr(head.start, "Connection: close\r\n");
    if (conn_close)
    {
        ret = ZZ_HTTP_RES_END_CONN_CLOSE;
        return ret;
    }
    
    return ret;
}



/**
 * Parse response (fill fields and copy data) from stream data. 
 *
 * @param res - pointer of ZZHttpResponse. 
 * @param data - data from underlying stream. 
 * @return 0 for success, NON-0 for failure.
 *
 * @note
 *  CAUTION: DO NOT free data's memory. 
 */
static void unserialize(ZZHttpRequest *request, const ZZData data)
{
    ZZHttpResponse *res = NULL;
    ZZHttpReqHandle req_handle = 0;
    ZZData temp = ZZ_DATA_NULL;
    ZZData head = ZZ_DATA_NULL;
    ZZData body = ZZ_DATA_NULL;
    uint8 *border = NULL;
    uint32 body_offset = 0;

    if (!request)
    {
        zzLogE(ZZ_ERR_STR_BADREQOHANDLE);
        return;
    }

    if (!data.start || !data.length)
    {
        zzLogE(ZZ_ERR_STR_INP);
        return;
    }

    req_handle = getHandle(request->stream);
    
    res = &request->response;

    if (!res->head_is_complete)
    {
        if (!res->head.start)
        {
            res->head = zzDataCopy(data);
            //zzLogI("new response head");
        }
        else
        {
            temp = res->head;
            res->head = zzDataMerge(res->head, data);
            zzDataFree(&temp);
            //zzLogI("head continue...");
        }

        border = SL_Strstr(res->head.start, "\r\n\r\n");
        if (border) 
        {
            //zzLogI("head complete!");
            
            res->head_is_complete = true;
            body_offset = border - res->head.start + 4;
            
            /* http head is full and maybe followed by a part of body  */
            if (body_offset < res->head.length)
            {
                head = zzDataSub(res->head, 0, body_offset - 1);
                body = zzDataSub(res->head, body_offset, res->head.length - 1);
                zzDataFree(&res->head);

                res->head = head;             
                //zzLogI("head:{len=%u, data=[%s]}", res->head.length, res->head.start);
                
                res->body = body;
                res->body_is_started = true;
                
                //zzLogI("body:{len=%u, data=[%s]}", res->body.length, res->body.start);
            }
            else
            {
                //zzLogI("head:{len=%u, data=[%s]}", res->head.length, res->head.start);
            }

            res->status_code = parseStatusCode(res->head);
            //zzLogI("response status = %u", res->status_code);

            res->res_end_indicator = parseResEndIndicator(request->method, res->head);

            if (ZZ_HTTP_RES_END_CL == res->res_end_indicator)
            {
                res->content_length = parseContentLength(res->head);
                //zzLogI("res end with cl (%u)", res->content_length);
                zzHttpReqProcessEvent(req_handle, ZZ_HTTP_REQ_EVT_RES_HEAD);
            }
            else if (ZZ_HTTP_RES_END_NO_BODY == res->res_end_indicator)
            {  
                //zzLogI("res end with no body");
                zzHttpReqProcessEvent(req_handle, ZZ_HTTP_REQ_EVT_RES_HEAD);
                zzHttpReqProcessEvent(req_handle, ZZ_HTTP_REQ_EVT_RES_END);
            }
            else if (ZZ_HTTP_RES_END_CONN_CLOSE == res->res_end_indicator)
            {
                //zzLogI("res end with conn close");
                zzHttpReqProcessEvent(req_handle, ZZ_HTTP_REQ_EVT_RES_HEAD);
            }
            else /* ZZ_HTTP_RES_END_UNSUPPORTED */
            {
                zzLogE("res end with unsupport");
                zzHttpReqProcessEvent(req_handle, ZZ_HTTP_REQ_EVT_ABORTED);
            }
        }
        else
        {
            //zzLogI("head is not full yet");
        }
    }
    else
    {
        if (!res->body_is_started)
        {
            res->body_is_started = true;
        }
        
        if (res->body.start)
        {
            zzDataFree(&res->body);
        }
        
        res->body = zzDataCopy(data);
        
        //zzLogI("body:{len=%u, data=[%s]}", res->body.length, res->body.start);
    }
    
    if (res->body_is_started)
    {
        res->body_counter += res->body.length;
        zzHttpReqProcessEvent(req_handle, ZZ_HTTP_REQ_EVT_RES_BODY);

        if (res->body_counter == res->content_length
            && ZZ_HTTP_RES_END_CL == res->res_end_indicator)
        {
            zzHttpReqProcessEvent(req_handle, ZZ_HTTP_REQ_EVT_RES_END);
        }
        else if (res->body_counter > res->content_length
            && ZZ_HTTP_RES_END_CL == res->res_end_indicator)
        {
            zzHttpReqProcessEvent(req_handle, ZZ_HTTP_REQ_EVT_ABORTED);
        }
    }
}


static void openStreamByIp(ZZHttpReqHandle req_handle, 
    ZZStream stream, 
    const char *ip, 
    uint16 port)
{
    int32 ret = 0;

    ZZHttpRequest *request = NULL;

    request = getRequest(req_handle);
    if (!request)
    {
        zzLogE(ZZ_ERR_STR_BADREQOHANDLE);        
        return;
    }

    ret = zzStreamOpen(stream, ip, port);
    if (ret)
    {
        zzLogE("zzStreamOpen err(%d)", ret);
        zzHttpReqProcessEvent(req_handle, ZZ_HTTP_REQ_EVT_OPENERR);
    }
    request->phase = ZZ_HTTP_REQ_PH_OPEN;
    //zzLogP("req phase to %u", request->phase);
}

static void dnsCb(ZZStream stream, U8 cid, S32 result, U8* ipaddr)
{
    ZZHttpReqHandle req_handle = 0;
    ZZHttpRequest *request = NULL;

    lockReqList();
    
    //zzLogFB();
    
    req_handle = getHandle(stream);
    request = getRequest(req_handle);
    
    if (!request)
    {
        zzLogE(ZZ_ERR_STR_BADREQOHANDLE);
        unlockReqList();
        return;
    }

    if (!result)
    {
        zzLogI("got [%s] ipaddr = [%s]", request->hostname, ipaddr);
        SL_Strcpy(request->ip, ipaddr);
        request->phase = ZZ_HTTP_REQ_PH_GOT_IP;
        //zzLogP("req phase to %u", request->phase);
        openStreamByIp(req_handle, request->stream, request->ip, request->port);
    }
    else
    {
        zzLogE("dns [%s] err(%d)", request->hostname, result);
        zzHttpReqProcessEvent(req_handle, ZZ_HTTP_REQ_EVT_DNSERR);
    }
    
    //zzLogFE();
    unlockReqList();
}

static void zzHttpReqStream0GetIpByNameCb(U8 cid, S32 result, U8* ipaddr)
{
    dnsCb(ZZ_STREAM_0, cid, result, ipaddr);
}

static void zzHttpReqStream1GetIpByNameCb(U8 cid, S32 result, U8* ipaddr)
{
    dnsCb(ZZ_STREAM_1, cid, result, ipaddr);
}

static void zzHttpReqStream2GetIpByNameCb(U8 cid, S32 result, U8* ipaddr)
{
    dnsCb(ZZ_STREAM_2, cid, result, ipaddr);
}

static void zzHttpReqStream3GetIpByNameCb(U8 cid, S32 result, U8* ipaddr)
{
    dnsCb(ZZ_STREAM_3, cid, result, ipaddr);
}

static int32 openStream(ZZHttpReqHandle req_handle, 
    ZZStream stream, 
    const char *hostname, 
    uint16 port)
{
    int32 ret = 0;
    uint32 ip = 0;
    SL_TCPIP_GET_HOSTIP_BY_NAME stream_get_ip_by_name_cb = NULL;
    ZZHttpRequest *request = NULL;
    
    request = getRequest(req_handle);
    if (!request)
    {
        zzLogE(ZZ_ERR_STR_BADREQOHANDLE);
        return ZZ_ERR_INVALID_PARAM;
    }
    
    request->flag_reopen = false;

    if (request->current_hostname)
    {
        SL_FreeMemory(request->current_hostname);
    }
    
    request->current_hostname = zzStringCopy(request->hostname);
    request->current_port = request->port;


    if (!SL_TcpipSocketCheckIp((uint8 *)hostname, &ip))
    {
        openStreamByIp(req_handle, stream, hostname, port);
    }
    else
    {
        switch (stream)
        {
            case ZZ_STREAM_0:
                stream_get_ip_by_name_cb = zzHttpReqStream0GetIpByNameCb;
            break;
            case ZZ_STREAM_1:
                stream_get_ip_by_name_cb = zzHttpReqStream1GetIpByNameCb;
            break;
            case ZZ_STREAM_2:
                stream_get_ip_by_name_cb = zzHttpReqStream2GetIpByNameCb;
            break;
            case ZZ_STREAM_3:
                stream_get_ip_by_name_cb = zzHttpReqStream3GetIpByNameCb;
            break;
            default:
                zzLogE("bad stream = %d", stream);
                return ZZ_ERR_INVALID_PARAM;
            break;
        }
        
        ret = SL_TcpipGetHostIpbyName(SL_TcpipGetCid(), (uint8 *)hostname, stream_get_ip_by_name_cb);
        if (ret)
        {
            zzLogE("SL_TcpipGetHostIpbyName err(%d)", ret);
            return ret;
        }

        request->phase = ZZ_HTTP_REQ_PH_GET_IP;
        //zzLogP("req phase to %u", request->phase);
    }

    return 0;
}


static void writeStream(ZZHttpReqHandle req_handle, ZZStream stream)
{
    int32 ret = 0;
    ZZData output = ZZ_DATA_NULL;
    ZZHttpRequest *request = NULL;
    
    request = getRequest(req_handle);
    if (!request)
    {
        zzLogE(ZZ_ERR_STR_INP);
        return;
    }

    ret = serialize(request, &output);
    if (ret)
    {
        zzLogE("serialize err(%d)", ret);
        zzHttpReqProcessEvent(req_handle, ZZ_HTTP_REQ_EVT_ABORTED);
        return;
    }

    request->length = output.length;
    request->written_length = 0;
    //zzLogI("req length = %u", request->length);
    
    ret = zzStreamWrite(stream, output.start, output.length);
    zzDataFree(&output);
    
    if (ret == request->length)
    {
        //zzLogI("http stream write ok(%u)", ret);
        request->phase = ZZ_HTTP_REQ_PH_WRITE;
        //zzLogP("req phase to %u", request->phase);
    }
    else
    {
        zzLogE("http stream write err(%d)", ret);
        zzHttpReqProcessEvent(req_handle, ZZ_HTTP_REQ_EVT_ABORTED);
    }
}

static void zzHttpReqStreamOpenCb(ZZStream stream, bool result, int32 error_code)
{
    ZZHttpReqHandle req_handle = 0;
    ZZHttpRequest *request = NULL;

    lockReqList();
    //zzLogFB();
    
    req_handle = getHandle(stream);
    request = getRequest(req_handle);
    if (!request)
    {
        zzLogE(ZZ_ERR_STR_BADREQOHANDLE);
        unlockReqList();
        return;
    }

    if (!result)
    {
        zzLogE("%s stream = %d, result = %d, error_code = %d", 
            __FUNCTION__, stream, result, error_code);
            
        zzHttpReqProcessEvent(req_handle, ZZ_HTTP_REQ_EVT_ABORTED);
        unlockReqList();
        return;
    }
    
    zzHttpReqProcessEvent(req_handle, ZZ_HTTP_REQ_EVT_OPENED);
    
    writeStream(req_handle, stream);

    //zzLogFE();
    unlockReqList();

}

static void zzHttpReqStreamCloseCb(ZZStream stream, bool result, int32 error_code)
{
    ZZHttpReqHandle req_handle = 0;
    ZZHttpRequest *request = NULL;

    lockReqList();

    //zzLogFB();
    
    req_handle = getHandle(stream);
    request = getRequest(req_handle);
    
    if (!request)
    {
        zzLogE(ZZ_ERR_STR_BADREQOHANDLE);
        unlockReqList();
        return;
    }
    
    if (!result)
    {
        zzLogE("%s stream = %d, result = %d, error_code = %d", 
            __FUNCTION__, stream, result, error_code);
            
        zzHttpReqProcessEvent(req_handle, ZZ_HTTP_REQ_EVT_ABORTED);
        unlockReqList();
        return;
    }


    if (ZZ_HTTP_RES_END_CONN_CLOSE == request->response.res_end_indicator
        && SL_TCPIP_SOCKET_SERVER_DISCONNECT == error_code)
    {
        zzHttpReqProcessEvent(req_handle, ZZ_HTTP_REQ_EVT_RES_END);
    }

    zzHttpReqProcessEvent(req_handle, ZZ_HTTP_REQ_EVT_CLOSED);
    
    if (request->flag_reopen)
    {
        request->flag_reopen = false;
        openStream(req_handle, stream, request->hostname, request->port);
    }

    //zzLogFE();
    unlockReqList();
}

static void zzHttpReqStreamWriteCb(ZZStream stream, bool result, int32 error_code)
{
    ZZHttpReqHandle req_handle = 0;
    ZZHttpRequest *request = NULL;

    lockReqList();
    //zzLogFB();

    req_handle = getHandle(stream);
    request = getRequest(req_handle);
    if (!request)
    {
        zzLogE(ZZ_ERR_STR_BADREQOHANDLE);
        unlockReqList();
        return;
    }
    
    if (!result)
    {
        zzLogE("%s stream = %d, result = %d, error_code = %d", 
            __FUNCTION__, stream, result, error_code);
        
        zzHttpReqProcessEvent(req_handle, ZZ_HTTP_REQ_EVT_ABORTED);
        unlockReqList();
        return;
    }
    

    request->written_length += error_code;
    //zzLogI("written %d bytes", error_code);

    if (request->written_length == request->length)
    {
        zzHttpReqProcessEvent(req_handle, ZZ_HTTP_REQ_EVT_WRITTEN);
    }
    
    //zzLogFE();   
    unlockReqList();
}

/*
 * @note
 *  1. The length of data received maybe larger than ZZ_HTTP_STREAM_RECV_BUFFER_SIZE .
 */
static void zzHttpReqStreamReadCb(ZZStream stream, bool result, int32 error_code)
{
    static uint8 zz_recv_buf[ZZ_HTTP_STREAM_RECV_BUFFER_SIZE] = {0};
    ZZData data = ZZ_DATA_NULL;
    int32 ret = 0;
    ZZHttpReqHandle req_handle = 0;
    ZZHttpRequest *request = NULL;

    lockReqList();
    //zzLogFB();
        
    req_handle = getHandle(stream);
    request = getRequest(req_handle);
    
    if (!result)
    {
        zzLogE("%s stream = %d, result = %d, error_code = %d", 
            __FUNCTION__, stream, result, error_code);
            
        zzHttpReqProcessEvent(req_handle, ZZ_HTTP_REQ_EVT_ABORTED);
        unlockReqList();
        return;
    }

    if (!request)
    {
        zzLogE(ZZ_ERR_STR_BADREQOHANDLE);
        unlockReqList();
        return;
    }

    if (!request->response.head_is_started)
    {
        request->response.head_is_started = true;
        zzHttpReqProcessEvent(req_handle, ZZ_HTTP_REQ_EVT_RES_BEGIN);
    }

    while ((ret = zzStreamRead(stream, zz_recv_buf, sizeof(zz_recv_buf) - 1)) > 0)
    {
        data.length = ret;
        data.start = zz_recv_buf;
        data.start[ret] = '\0';
        
        //zzLogI("read len=%u", data.length);
        unserialize(request, data);
    }
    
    if (ret < 0)
    {
        if (!request->res_is_end && !request->flag_finish)
        {
            zzLogE("zzStreamRead err(%d)", ret);
            zzHttpReqProcessEvent(req_handle, ZZ_HTTP_REQ_EVT_ABORTED);
        }
    }
    
    //zzLogFE();    
    unlockReqList();
}

/**
 * Open stream, write http request, waiting for response and call callbacks. 
 */
int32 zzHttpReqSubmit(ZZHttpReqHandle req_handle)
{
    int32 ret = 0;
    ZZHttpRequest *request = NULL;
    ZZStreamState state = ZZ_STREAM_IDLE;

    lockReqList();

    request = getRequest(req_handle);

    if (!request)
    {
        zzLogE(ZZ_ERR_STR_BADREQOHANDLE);
        unlockReqList();
        return ZZ_ERR_INVALID_PARAM;
    }

    if (!request->flag_finish && !request->res_is_end)
    {
        zzLogE(ZZ_ERR_STR_BADOPR);
        unlockReqList();
        return ZZ_ERR_BAD_OPERATION;
    }

    if (!initForSubmit(request))
    {
        unlockReqList();
        return ZZ_ERR_HTTP_BAD_FORMAT;
    }

    state = zzStreamGetState(request->stream);
    if (ZZ_STREAM_OPENED == state)
    {
        if ((0 == SL_Strcmp(request->current_hostname, request->hostname))
            && (request->current_port == request->port))
        {
            /* write http request to stream directly */
            zzLogI("stream %u state:%d and write directly", request->stream, state);
            writeStream(req_handle, request->stream);
        }
        else /* close stream and reopen */
        {
            zzLogI("stream %u state:%d and reopen for new connection", request->stream, state);
            request->flag_reopen = true;
            ret = zzStreamClose(request->stream);
            if (ret)
            {
                zzLogE("zzStreamClose err(%d)", ret);
                request->flag_reopen = false;
                unlockReqList();
                return ret;
            }
            request->phase = ZZ_HTTP_REQ_PH_CLOSE;
            //zzLogP("req phase to %u", request->phase);
        }
    }
    else /* open stream */
    {
        zzLogI("stream %u state:%d and open it", request->stream, state);
        ret = openStream(req_handle, request->stream, request->hostname, request->port);
        if (ret)
        {
            zzLogE("openStream err(%d)", ret);
            unlockReqList();
            return ret;
        }
    }
    
    SL_StopTimer(http_req_task, req_handle);
    SL_StartTimer(http_req_task, 
        req_handle, 
        SL_TIMER_MODE_SINGLE, 
        SL_SecondToTicks(request->timeout));

    unlockReqList();        
    
    return 0;
}

/**
 * Finish the current request ahead of routine. 
 *
 * Called when Exception Event occurs.
 * 
 * @note
 *  1. For Exception Event, user maybe want to cancel the request(by seting flags and closing stream). 
 *  2. After calling this function, Exception Event will be ignored(Routine Event works normally). 
 *  3. Marks the request as aborting. Calling this will cause remaining data in the 
 *     response to be dropped and the socket to be destroyed.
 */
int32 zzHttpReqFinish(ZZHttpReqHandle req_handle)
{
    int32 ret = 0;
    ZZHttpRequest *request = NULL;
    ZZStreamState state = ZZ_STREAM_IDLE;

    lockReqList();

    request = getRequest(req_handle);
    
    if (!request)
    {
        zzLogE(ZZ_ERR_STR_BADREQOHANDLE);
        unlockReqList();
        return ZZ_ERR_INVALID_PARAM;
    }

    if (request->flag_finish)
    {
        zzLogI("req_handle finished already!");
        unlockReqList();
        return 0;
    }

    SL_StopTimer(http_req_task, req_handle);

    request->flag_finish = true;
    
    state = zzStreamGetState(request->stream);
    
    if (ZZ_STREAM_IDLE != state && ZZ_STREAM_CLOSED != state)
    {
        zzLogI("stream %u state:%d and close it now!", request->stream, state);
        ret = zzStreamClose(request->stream);
        if (ret)
        {
            zzLogE("zzStreamClose err(%d)", ret);
            unlockReqList();
            return ret;
        }
        request->phase = ZZ_HTTP_REQ_PH_CLOSE;
        //zzLogP("req phase to %u", request->phase);
    }
    else
    {
        zzLogI("stream %u state:%d and do nothing!", request->stream, state);
    }
    
    unlockReqList();

    return 0;
}

/**
 *
 * @return non-negative for success, negative for failure. 
 *
 * @note
 *  Call zzHttpReqRelease to free the memory allocated by this function. 
 */
ZZHttpReqHandle zzHttpReqCreate(ZZStream stream)
{
    ZZHttpRequest *request = NULL;
    ZZHttpReqHandle req_handle = 0;
    int32 ret = 0;

    lockReqList();

    req_handle = getHandle(stream);
    request = getRequest(req_handle);

    if (request)
    {
        zzLogE("Please release req_handle %d before create another", req_handle);
        unlockReqList();
        return ZZ_ERR_BAD_OPERATION;
    }
    
    ret = zzStreamCreate(stream);
    if (ret)
    {
        zzLogE("zzStreamCreate %d failed", stream);
        goto ExitErr1;
    }
    
    ret = zzStreamSetCallback(stream, 
        zzHttpReqStreamOpenCb, 
        zzHttpReqStreamCloseCb, 
        zzHttpReqStreamWriteCb, 
        zzHttpReqStreamReadCb);
    if (ret)
    {
        zzLogE("zzStreamSetCallback err(%d)", ret);
        goto ExitErr2;
    }
    
    request = (ZZHttpRequest *)SL_GetMemory(sizeof(ZZHttpRequest));
    if (!request)
    {
        zzLogE(ZZ_ERR_STR_OOM);
        ret = ZZ_ERR_OUT_OF_MEMORY;
        goto ExitErr3;
    }
    
    zzMemzero(request, sizeof(ZZHttpRequest));

    request->stream = stream;
    request->phase = ZZ_HTTP_REQ_PH_IDLE;
    request->res_is_end = true;
    request->flag_finish = true;
    
    //zzLogP("req phase to %u", request->phase);
    
    if (!registerRequest(req_handle, request))
    {
        ret = ZZ_ERR_HTTP_REG_FAILED;
        goto ExitErr4;
    }
    
    unlockReqList();
    return req_handle;

ExitErr4:
    SL_FreeMemory(request);
    
ExitErr3:
    zzStreamClrCallback(stream);

ExitErr2:
    zzStreamRelease(stream);

ExitErr1:
    unlockReqList();
    return ret;
    
}


/**
 * Release all resources about req_handle. 
 * 
 * @note
 *  1. Free request's inner pointers and itself. 
 *  2. Close the stream if it still opened. 
 */
int32 zzHttpReqRelease(ZZHttpReqHandle req_handle)
{
    ZZHttpRequest *request = NULL;
    int32 ret = 0;
    int32 i = 0;

    lockReqList();
    request = getRequest(req_handle);
    if (!request)
    {
        zzLogE("Please create req_handle %d before release it", req_handle);
        unlockReqList();
        return ZZ_ERR_BAD_OPERATION;
    }

    ret = zzStreamClrCallback(request->stream);
    if (ret)
    {
        zzLogE( "zzStreamClrCallback err(%d)", ret);
    }

    zzHttpReqFinish(req_handle);

    ret = zzStreamRelease(request->stream);
    if (ret)
    {
        zzLogE( "zzStreamRelease err(%d)", ret);
    }

    unregisterRequest(req_handle);

    /* free inner pointers of memory allocated */

    if (request->method)
    {
        SL_FreeMemory(request->method);
    }

    if (request->hostname)
    {
        SL_FreeMemory(request->hostname);
    }

    if (request->current_hostname)
    {
        SL_FreeMemory(request->current_hostname);
    }

    if (request->path)
    {
        SL_FreeMemory(request->path);
    }

    for (i = 0; i < ZZ_HTTP_REQ_ARG_MAX; i++)
    {
        if (request->args[i].key)
        {
            SL_FreeMemory(request->args[i].key);
        }

        if (request->args[i].str)
        {
            SL_FreeMemory(request->args[i].str);
        }
    }

    for (i = 0; i < ZZ_HTTP_REQ_HEADER_MAX; i++)
    {
        if (request->headers[i].key)
        {
            SL_FreeMemory(request->headers[i].key);
        }

        if (request->headers[i].str)
        {
            SL_FreeMemory(request->headers[i].str);
        }
    }

    if (request->body.start)
    {
        zzDataFree(&request->body);
    }
    
    if (request->response.head.start)
    {
        zzDataFree(&request->response.head);
    }

    if (request->response.body.start)
    {
        zzDataFree(&request->response.body);
    }
    
    zzMemzero(request, sizeof(ZZHttpRequest));
    
    /* free request */
    SL_FreeMemory(request);

    unlockReqList();
    
    return 0;
}

static void zzHttpReqTask (void *pdata)
{
    SL_EVENT ev = {0};
    ZZHttpReqHandle req_handle = 0;
    
    zzLogI("zzHttpReqTask begin");
    
    while (1)
    {
        SL_Memset(&ev, 0, sizeof(ev));
        SL_GetEvent(http_req_task, &ev);
        switch (ev.nEventId)
        {
            case ZZ_EVT_HTTP_REQ_TASK_TIMER:
                zzLogI("ZZ_EVT_HTTP_REQ_TASK_TIMER");
                req_handle = ev.nParam1;
                lockReqList();
                zzHttpReqProcessEvent(req_handle, ZZ_HTTP_REQ_EVT_TIMEOUT);
                unlockReqList();
                
            break;
            default:
                zzLogE("unknown ev = %u", ev.nEventId);
            break;
        }
    }
}


void zzHttpReqTaskInit(void)
{
    http_req_lock = zzCreateLock();

    http_req_task.element[0] = SL_CreateTask(zzHttpReqTask, 
        ZZ_HTTP_REQ_TASK_STACKSIZE, 
        ZZ_HTTP_REQ_TASK_PRIORITY, 
        ZZ_HTTP_REQ_TASK_NAME);

    if (!http_req_task.element[0])
    {
        zzLogE("create zzHttpReqTask failed!");
    }
}

