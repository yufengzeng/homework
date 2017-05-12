#ifndef __ZZ_LOG_H__
#define __ZZ_LOG_H__

#include "sl_debug.h"

#include "zz_app_config.h"

#ifdef ZZ_LOG
#define ZZ_LOG_PRINTF               (1)
#define ZZ_LOG_ERROR                (1)
#define ZZ_LOG_WARN                 (1)
#define ZZ_LOG_INFO                 (1)
#define ZZ_LOG_DEBUG                (1)
#else
#define ZZ_LOG_PRINTF               (0)
#define ZZ_LOG_ERROR                (0)
#define ZZ_LOG_WARN                 (0)
#define ZZ_LOG_INFO                 (0)
#define ZZ_LOG_DEBUG                (0)
#endif

#define ZZ_LOG_ERROR_PATH           (1)
#define ZZ_LOG_WARN_PATH            (1)
#define ZZ_LOG_INFO_PATH            (0)
#define ZZ_LOG_DEBUG_PATH           (0)

#define __zzLogPrint                  SL_ApiPrint

#define zzLogP(fmt, args...)\
    do {\
        if (ZZ_LOG_PRINTF)\
        {\
    		__zzLogPrint(fmt, ##args);\
        }\
    } while(0)

#define zzLogE(fmt, args...)\
    do {\
        if (ZZ_LOG_ERROR)\
        {\
            if (ZZ_LOG_ERROR_PATH)\
    		    __zzLogPrint("[ZZ E:%s,%d,%s] ",__FILE__,__LINE__,__FUNCTION__);\
            else\
                __zzLogPrint("[ZZ E] ");\
    		__zzLogPrint(fmt, ##args);\
    		__zzLogPrint("\n");\
        }\
    } while(0)

#define zzLogW(fmt, args...)\
    do {\
        if (ZZ_LOG_WARN)\
        {\
            if (ZZ_LOG_WARN_PATH)\
    		    __zzLogPrint("[ZZ W:%s,%d,%s] ",__FILE__,__LINE__,__FUNCTION__);\
            else\
                __zzLogPrint("[ZZ W] ");\
    		__zzLogPrint(fmt, ##args);\
    		__zzLogPrint("\n");\
        }\
    } while(0)

#define zzLogI(fmt, args...)\
    do {\
        if (ZZ_LOG_INFO)\
        {\
            if (ZZ_LOG_INFO_PATH)\
                __zzLogPrint("[ZZ I:%s,%d,%s] ",__FILE__,__LINE__,__FUNCTION__);\
            else\
                __zzLogPrint("[ZZ I] ");\
            __zzLogPrint(fmt, ##args);\
            __zzLogPrint("\n");\
        }\
    } while(0)
        
#define zzLogD(fmt, args...)\
    do {\
        if (ZZ_LOG_DEBUG)\
        {\
            if (ZZ_LOG_DEBUG_PATH)\
                __zzLogPrint("[ZZ D:%s,%d,%s] ",__FILE__,__LINE__,__FUNCTION__);\
            else\
                __zzLogPrint("[ZZ D] ");\
            __zzLogPrint(fmt, ##args);\
            __zzLogPrint("\n");\
        }\
    } while(0)


#define zzLogFB() zzLogP("%s begin", __FUNCTION__)
#define zzLogFE() zzLogP("%s   end", __FUNCTION__)

#define zzLogII(ret, str) \
    do {\
        ret = ret;\
        if (!(ret)) {\
            zzLogI("%s ok", str);\
        } else {\
            zzLogI("%s err(%d)", str, (ret));\
        }\
    } while(0)

#define zzLogIIF(ret) \
    do {\
        ret = ret;\
        if (!(ret)) {\
            zzLogI("%s ok", __FUNCTION__);\
        } else {\
            zzLogI("%s err(%d)", __FUNCTION__, (ret));\
        }\
    } while(0)

#define zzLogIW(ret, str) \
    do {\
        ret = ret;\
        if (!(ret)) {\
            zzLogI("%s ok", str);\
        } else {\
            zzLogW("%s err(%d)", str, (ret));\
        }\
    } while(0)
        
#define zzLogIWF(ret) \
    do {\
        ret = ret;\
        if (!(ret)) {\
            zzLogI("%s ok", __FUNCTION__);\
        } else {\
            zzLogW("%s err(%d)", __FUNCTION__, (ret));\
        }\
    } while(0)

#define zzLogIE(ret, str) \
    do {\
        ret = ret;\
        if (!(ret)) {\
            zzLogI("%s ok", str);\
        } else {\
            zzLogE("%s err(%d)", str, (ret));\
        }\
    } while(0)

#define zzLogIEF(ret) \
    do {\
        ret = ret;\
        if (!(ret)) {\
            zzLogI("%s ok", __FUNCTION__);\
        } else {\
            zzLogE("%s err(%d)", __FUNCTION__, (ret));\
        }\
    } while(0)


#endif /* __ZZ_LOG_H__ */

