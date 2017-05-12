#ifndef __ZZ_ERROR_H__
#define __ZZ_ERROR_H__

#define ZZ_ERR_STR_OOM                  "out of memroy"
#define ZZ_ERR_STR_INP                  "invalid param"
#define ZZ_ERR_STR_BADOPR               "bad operation"
#define ZZ_ERR_STR_BADREQOHANDLE        "bad request or req_handle"

typedef enum 
{
    ZZ_OK = 0,

    /* common */
    ZZ_ERR_INVALID_PARAM                = -90001000,
    ZZ_ERR_OUT_OF_MEMORY                = -90001001,
    ZZ_ERR_BAD_OPERATION                = -90001002,
    
    /* Stream */ 
    ZZ_ERR_STREAM_INVALID               = -90002000,
    ZZ_ERR_STREAM_TAKEN                 = -90002001,
    ZZ_ERR_STREAM_UNTAKEN               = -90002002,

    /* Http */
    ZZ_ERR_HTTP_BAD_FORMAT              = -90003000,
    ZZ_ERR_HTTP_REG_FAILED              = -90003001,
    
    
} ZZErrorCode;

#endif /* __ZZ_ERROR_H__ */

