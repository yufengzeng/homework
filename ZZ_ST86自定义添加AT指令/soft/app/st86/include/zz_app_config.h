#ifndef __ZZ_APP_CONFIG_H__
#define __ZZ_APP_CONFIG_H__



#define ZZ_COMPILE_FOR_DEBUG        /* test env with log */
//#define ZZ_COMPILE_FOR_RELEASE      /* real env without log */



#if defined(ZZ_COMPILE_FOR_DEBUG) && !defined(ZZ_COMPILE_FOR_RELEASE)

#define ZZ_LOG
#define ZZ_IOT_DS_DOMAIN_NAME     "cn-testapi.coolkit.cc"

#elif defined(ZZ_COMPILE_FOR_RELEASE) && !defined(ZZ_COMPILE_FOR_DEBUG)

//#define ZZ_LOG
#define ZZ_IOT_DS_DOMAIN_NAME     "cn-disp.coolkit.cc"

#else
    #error Sorry! You must select a right compile!
#endif


#endif /* __ZZ_APP_CONFIG_H__ */

