#ifndef __ZZ_TYPE_H__
#define __ZZ_TYPE_H__

#include "sl_type.h"
#include "zz_log.h"
#include "zz_error.h"
#include "zz_data.h"

#ifndef true
#define true    TRUE
#endif

#ifndef false
#define false   FALSE
#endif

#define zzArraySize(a)      (sizeof(a)/sizeof(a[0]))

/*
 * ZZ_SIZE_* denote the size of string (char *) including '\0'. 
 */

#define ZZ_SIZE_HOSTNAME    (50 + 1)
#define ZZ_SIZE_IP          (15 + 1)
#define ZZ_SIZE_MODEL       (10 + 1)
#define ZZ_SIZE_DEVICEID    (10 + 1)
#define ZZ_SIZE_ACTION      (20 + 1)
#define ZZ_SIZE_SEQUENCE    (20 + 1)
#define ZZ_SIZE_APIKEY      (36 + 1)
#define ZZ_SIZE_UUID        (ZZ_SIZE_APIKEY)


typedef struct 
{
    char *key;
    char *str;
} ZZKeyStr;


typedef enum 
{
    ZZ_DEVICE_STATE_START           = 0,    /* Starting */
    ZZ_DEVICE_STATE_GPRS            = 1,    /* GPRS attaching */
    ZZ_DEVICE_STATE_CONN            = 2,    /* Connecting */
    ZZ_DEVICE_STATE_LOGIN           = 3,    /* Login ... */
    ZZ_DEVICE_STATE_LOGINED         = 4,    /* Logined (online) */
    ZZ_DEVICE_STATE_UPGRADE         = 5,    /* upgrade */
    ZZ_DEVICE_STATE_DISABLED        = 6,    /* disabled (waiting for enable) */
    
    ZZ_DEVICE_STATE_INVALID        = 255,
} ZZDeviceState;

typedef struct
{
    char model[ZZ_SIZE_MODEL];
    char deviceid[ZZ_SIZE_DEVICEID];
    char apikey[ZZ_SIZE_APIKEY];
    char uuid[ZZ_SIZE_UUID];
    char ds_hostname[ZZ_SIZE_HOSTNAME];
    uint16 ds_port;                 /* 8081 */
    char ps_ip[ZZ_SIZE_IP];
    uint16 ps_port;                 /* 8081 */
} ZZDevice;


/*
 * Any IoT Package that can be accepted MUST be one of ZZIotPkgType. 
 */
typedef enum  
{
    ZZ_IOT_PKG_TYPE_RES_OF_LOGIN        = 1,
    ZZ_IOT_PKG_TYPE_RES_OF_UPDATE       = 2,
    ZZ_IOT_PKG_TYPE_RES_OF_QUERY        = 3,
    ZZ_IOT_PKG_TYPE_RES_OF_DATE         = 4,
    ZZ_IOT_PKG_TYPE_REQ_OF_UPDATE       = 5,
    ZZ_IOT_PKG_TYPE_REQ_OF_QUERY        = 6,
    ZZ_IOT_PKG_TYPE_REQ_OF_UPGRADE      = 7,
    ZZ_IOT_PKG_TYPE_REQ_OF_REDIRECT     = 8,
    ZZ_IOT_PKG_TYPE_REQ_OF_NOTIFY       = 9,
    ZZ_IOT_PKG_TYPE_REQ_OF_RESTART      = 10,
    
    ZZ_IOT_PKG_TYPE_BAD_RESPONSE        = 254,   /* error != 0 */
    ZZ_IOT_PKG_TYPE_INVALID             = 255    /* invalid pkg */   
} ZZIotPkgType;

typedef struct  
{
    uint16 year;
    uint8 month;
    uint8 day;
    uint8 hour;
    uint8 minute;
    uint8 second;
} IoTgoGMTTime;

char * zzStringCopy(const char *str);

#endif /* __ZZ_TYPE_H__ */

