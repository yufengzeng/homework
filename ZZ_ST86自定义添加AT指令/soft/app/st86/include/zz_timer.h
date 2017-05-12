#ifndef  __ZZ_TIMER_H__
#define  __ZZ_TIMER_H__

#include "sl_type.h"
#include "cJSON.h"


#define ZZ_TIMER_MAX_NUM                  (8)

typedef enum 
{
    ZZ_TIME_TYPE_INVALID    = 0,
    ZZ_TIME_TYPE_ONCE        = 1,
    ZZ_TIME_TYPE_REPEAT       = 2,
    ZZ_TIME_TYPE_DURATION   = 3,          /*每隔一段时间执行一次*/
}ZZTimerType;

typedef struct  
{
    uint16 year;
    uint8 month;
    uint8 day;
    uint8 hour;
    uint8 minute;
    uint8 second;
} ZZGMTTime;

typedef struct 
{
    bool weeks[7];  /* weeks[0-6]: index = week, true or false */
    uint8 hour;     /* 0-23, 24=* */
    uint8 minute;   /* 0-59 */
} ZZCron;

typedef struct
{
    char start_do[100];
    char end_do[100];
}ZZDurationDo;

/*
 * defined one timer
 */
typedef struct
{
    uint32 time_interval;
    uint32 time_duration;
    bool enabled;
    ZZTimerType type;
    union
    {
        ZZGMTTime gmt;
        ZZCron cron;
    }timing;
    union
    {
        char action_do[100];
        ZZDurationDo duration_do;
    }time_action;
    char at[30];
}ZZOneTimer;

typedef struct
{
    //uint8 timer_num;
    ZZOneTimer timer[ZZ_TIMER_MAX_NUM];
}ZZTimers;

typedef void(*zzTimerCallback)(void *ptr);

/*
 * 定时器初始化函数
 *
 * timer_callback: 定时器处理动作回调函数
 */
bool zzTimerTaskInit(zzTimerCallback timer_callback);

/*
 * 定时器处理函数
 * 
 * cjson_params : 定时器定义JSON 对象,内存由调用者自己释放.
 */
bool zzTimerProc(cJSON *cjson_params);

#endif

