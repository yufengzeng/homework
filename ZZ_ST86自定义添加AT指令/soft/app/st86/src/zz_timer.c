#include "zz_timer.h"
#include "zz_log.h"
#include "sl_os.h"
#include "sl_system.h"
#include "sl_stdlib.h"
#include "zz_type.h"
#include "zz_task.h"
#include "zz_event.h"


/*
 *timer defined for cjson parse
 */
#define ZZ_STRING_TIMER       "timers" 
//#define ZZ_STRING_ID        "id"
#define ZZ_STRING_ENABLED    "enabled"
#define ZZ_STRING_TYPE       "type"
#define ZZ_STRING_ONCE       "once"
#define ZZ_STRING_REPEAT     "repeat"
#define ZZ_STRING_DURATION    "duration"
#define ZZ_STRING_AT         "at"
#define ZZ_STRING_DO         "do"
#define ZZ_STRING_START_DO    "startDo"
#define ZZ_STRING_END_DO      "endDo"


#define ZZ_TIMER_SCAN              (1000)
#define ZZ_GMT_STRING_MIN_LENGTH (24)
#define ZZ_TIMER_MAX_NUM    (8)


static SL_TASK timer_task = {{0}};
static zzTimerCallback zz_timer_action = NULL;
static ZZTimers zz_timers;

static bool  isLeapYear(int32 year)
{
    if (year < 0)
        return false;
    return ((year%4 == 0 && year%100 != 0) || year%400 == 0) ? true : false;
}

static int8  daysOfMonth(int32 year, int8 month)
{
    int8 days = 0;;
    switch(month) 
    {
        case 1:
        case 3:
        case 5:
        case 7:
        case 8:
        case 10:
        case 12:
            days = 31;
            break;
        case 4:
        case 6:
        case 9:
        case 11:
            days = 30;
            break;
        case 2:
            if (isLeapYear(year))
                days = 29;
            else
                days = 28;
            break;
        default:
            zzLogE("Invalid month = %d", month);
    }
    return days;
}

static bool  gmtCompareWithNow(ZZGMTTime gmt)
{
    SL_SYSTEMTIME cur_gmt;
    if(SL_GetLocalTime(&cur_gmt))
    {
        if(gmt.year == cur_gmt.uYear && gmt.month == cur_gmt.uMonth
            && gmt.day == cur_gmt.uDay && gmt.hour == cur_gmt.uHour
            && gmt.minute == cur_gmt.uMinute && cur_gmt.uSecond == 0)
        {
            return true;
        }
    }
    return false;
}
static bool  cronCompareWitchNow(ZZCron cron)
{
    SL_SYSTEMTIME cur_gmt;
    if(SL_GetLocalTime(&cur_gmt))
    {
        if(cron.weeks[cur_gmt.uDayOfWeek] == true
        && (cron.hour == cur_gmt.uHour || cron.hour == 24)
        && cron.minute == cur_gmt.uMinute 
        && cur_gmt.uSecond == 0)
        {
            return true;
        }
    }
    return false;
}
static uint32 compareTimeToKnowDate(ZZGMTTime gmt)
{
    uint32 year = 0;
    uint32 count = 0;
    uint32 day_sum = 0;
    uint32 second_sum = 0;
    ZZGMTTime know_date ={2016,1,1,0,0,0};

    if(gmt.year < know_date.year)
    {
        return second_sum;
    }
    else if(gmt.year > know_date.year)
    {
        for(year = know_date.year ; year < gmt.year ; year++)
        {
            for(count=1; count<=12; count++)
            {
                day_sum += daysOfMonth(year,count);
            }
        }

        for(count=1; count < gmt.month; count++)
        {
            day_sum += daysOfMonth(year,count);
        }
    }
    else
    {
        for(count=1; count < gmt.month; count++)
        {
            day_sum += daysOfMonth(year,count);
        }
    }
    day_sum += (gmt.day-1);
    second_sum = (day_sum * 24 * 60 + gmt.hour * 60 + gmt.minute) * 60 + gmt.second;
    return second_sum;
}
static int8  durationCompareWithNow(ZZGMTTime gmt,
                                                    uint32 time_interval,uint32 time_duration)
{
    uint32 cur_diff_second = 0;
    uint32 time_diff_second = 0;
    uint32 diff_val = 0;
    uint32 diff_second = 0;
    uint32 diff_minute = 0;
    ZZGMTTime cur_gmt;
    SL_SYSTEMTIME cur_sltimer;
    if(SL_GetLocalTime(&cur_sltimer))
    {
        cur_gmt.year = cur_sltimer.uYear;
        cur_gmt.month = cur_sltimer.uMonth;
        cur_gmt.day = cur_sltimer.uDay;
        cur_gmt.hour = cur_sltimer.uHour;
        cur_gmt.minute = cur_sltimer.uMinute;
        cur_gmt.second = cur_sltimer.uSecond;
        cur_diff_second = compareTimeToKnowDate(cur_gmt);
        time_diff_second = compareTimeToKnowDate(gmt);
        if(0 == cur_diff_second || 0 == time_diff_second || (time_diff_second > cur_diff_second))
        {
            return -1;
        }
        diff_val = cur_diff_second - time_diff_second;
        diff_minute = diff_val / 60;
        diff_second = diff_val % 60;
        if(time_interval != 0 && (diff_minute % time_interval) == 0 && (diff_second == 0))
        {
            return  0;
        }
        else if(time_duration != 0 && ((time_duration == (diff_minute % time_interval)) && (diff_second == 0)))
        {
            return 1;
        }
        else
        {
            return -1;
        }
    }
    return -1;
}

static void timerDeal(void)
{
    uint8 i = 0;
    SL_SYSTEMTIME cur_gmt;
    SL_GetLocalTime(&cur_gmt);
    zzLogP("[%04d/%02d/%02d %02d:%02d:%02d]",cur_gmt.uYear,cur_gmt.uMonth,cur_gmt.uDay,cur_gmt.uHour,cur_gmt.uMinute,cur_gmt.uSecond);
    for(i = 0; i < ZZ_TIMER_MAX_NUM;i++)
    {
        if(false == zz_timers.timer[i].enabled)
        {
            continue;
        }
        switch(zz_timers.timer[i].type)
        {
            case ZZ_TIME_TYPE_ONCE:
            {
                if(gmtCompareWithNow(zz_timers.timer[i].timing.gmt))
                {
                    if(NULL != zz_timer_action)
                    {
                        zz_timer_action(zz_timers.timer[i].time_action.action_do);
                    }
                    zz_timers.timer[i].enabled = false;
                }
            }break;
            case ZZ_TIME_TYPE_REPEAT:
            {
                if(cronCompareWitchNow(zz_timers.timer[i].timing.cron))
                {
                    if(NULL != zz_timer_action)
                    {
                        zz_timer_action(zz_timers.timer[i].time_action.action_do);
                    }
                }
            }break;
            case ZZ_TIME_TYPE_DURATION:
            {
                int8 ret = -1;
                ret = durationCompareWithNow(zz_timers.timer[i].timing.gmt,
                             zz_timers.timer[i].time_interval,zz_timers.timer[i].time_duration);
                if(0 == ret)
                {
                    if(0 != SL_Strlen(zz_timers.timer[i].time_action.action_do))
                    {
                        if(NULL != zz_timer_action)
                        {
                            zz_timer_action(zz_timers.timer[i].time_action.action_do);
                        }
                    }
                    else
                    {
                        if(NULL != zz_timer_action)
                        {
                            zz_timer_action(zz_timers.timer[i].time_action.duration_do.start_do);
                        }
                    }
                }
                else if(1 == ret)
                {
                    if(NULL != zz_timer_action)
                    {
                        zz_timer_action(zz_timers.timer[i].time_action.duration_do.end_do);
                    }
                }
                
            }break;
            default:
            {
                zzLogI("please check code,this type don't exit");
            };
        }
    }
}

static void timerTask (void *pdata)
{
    SL_EVENT ev = {0};
    
    zzLogFB();    
    zzPrintCurrentTaskPrio();
    
    while(1)
    {
        SL_Memset(&ev,0,sizeof(SL_EVENT));
        SL_GetEvent(timer_task, &ev);
        switch(ev.nEventId)
        {
            case ZZ_EVT_TIMER_TASK_START:
                zzLogI("ZZ_EVT_TIMER_TASK_START");
            break;
            case ZZ_EVT_TIMER_TASK_TIMER:
                //zzLogI("ZZ_EVT_TIMER_TASK_TIMER");
                timerDeal();
            break;
            default:
                zzLogE("unknown ev = %u", ev.nEventId);
            break;
        }
    }
}
static bool  verifyGMTTime(ZZGMTTime *gmt_time)
{
    if (!gmt_time)
    {
        return false;
    }
    
    if (gmt_time->year > 2000
        && (gmt_time->month >= 1 && gmt_time->month <= 12)
        && (gmt_time->day >= 1 && gmt_time->day <= 31)
        && (gmt_time->hour >= 0 && gmt_time->hour <= 23)
        && (gmt_time->minute >= 0 && gmt_time->minute <= 59)
        && (gmt_time->second >= 0 && gmt_time->second <= 59)
        )
    {
        return true;
    }
    return false;
}

static bool  parseGMTTimeFromString(const char *str, ZZGMTTime *gmt_time)
{
    char temp[24] = {0};
    if (gmt_time && str)
    {
        if (SL_Strlen(str) == 24
            && str[10] == 'T'
            && str[23] == 'Z' )
        {
            SL_Memset(temp,0, sizeof(temp));
            SL_Strncpy(temp, str + 0, 4);
            gmt_time->year = SL_atoi(temp);
            
            SL_Memset(temp,0, sizeof(temp));
            SL_Strncpy(temp, str + 5, 2);
            gmt_time->month = SL_atoi(temp);
            
            SL_Memset(temp,0, sizeof(temp));
            SL_Strncpy(temp, str + 8, 2);
            gmt_time->day = SL_atoi(temp);
            
            SL_Memset(temp,0, sizeof(temp));
            SL_Strncpy(temp, str + 11, 2);
            gmt_time->hour = SL_atoi(temp);
            
            SL_Memset(temp,0, sizeof(temp));
            SL_Strncpy(temp, str + 14, 2);
            gmt_time->minute = SL_atoi(temp);
            
            SL_Memset(temp,0, sizeof(temp));
            SL_Strncpy(temp, str + 17, 2);
            gmt_time->second = SL_atoi(temp);
            
            if (verifyGMTTime(gmt_time))
            {
                return true;
            }
            else
            {
                zzLogI("Bad IoTgoGMTTime");
                return false;
            }
        }
    }
    
    return false;
}
static bool  parseCronFromString(const char *str, ZZCron *cron)
{
    uint8 str_len;
    int32 field_len;
    char *p = (char *)str;
    char *temp;
    char buffer[25];
    uint8 i;
    uint8 j;
    
    if (NULL == str || NULL == cron)
    {
        return false;
    }
    str_len = SL_Strlen(str);
    if (str_len < 9 || str_len > 23)
    {
        return false;
    }

    /* 解析分钟 */
    temp = (char *)SL_Strstr(p, " ");
    if (!temp || (temp - p <= 0))
    {
        return false;
    }
    field_len = temp - p;
    SL_Strncpy(buffer, p, field_len);
    buffer[field_len] = '\0';
    cron->minute = SL_atoi(buffer);
    if (cron->minute < 0 || cron->minute > 59)
    {
        return false;
    }
    
    temp++;
    p = temp;
    
    /* 解析小时 */
    temp = (char *)SL_Strstr(p, " ");
    if (!temp || (temp - p <= 0))
    {
        return false;
    }
    field_len = temp - p;
    SL_Strncpy(buffer, p, field_len);
    buffer[field_len] = '\0';
    if (0 == SL_Strcmp(buffer, "*"))
    {
        cron->hour = 24;
    }
    else
    {
        cron->hour = SL_atoi(buffer);    
        if (cron->hour < 0 || cron->hour > 23)
        {
            return false;
        }
    }

    /* 忽略月日 */
    p = temp + 5; 

    /* 解析周 */
    for (i = 0; i < 7; i++)
    {
        cron->weeks[i] = false;
    }

    /* 1 */
    /* 0,1,2,3,4,5,6 */
    SL_Strcpy(buffer, p);
    if (0 == SL_Strcmp(buffer, "*"))
    {
        for (i = 0; i < 7; i++)
        {
            cron->weeks[i] = true;
        }
    }
    else 
    {
        for (j = 0; j < SL_Strlen(buffer); j++)
        {
            if (buffer[j] != ',')
            {
                i = buffer[j] - 48;
                
                if (i < 0 || i > 6)
                {
                    return false;
                }
                cron->weeks[i] = true;
            }
        }
    }
    return true;
    
}
static bool  parseFromATString(const char*at_str,char*gmt,uint32* interval,uint32* duration)
{
    uint8 str_len;
    int32 field_len;
    char *p = (char*)at_str;
    char *temp ;
    char buffer[30];
    if(NULL == at_str || NULL == gmt)
    {
        return false;
    }
    
    str_len = SL_Strlen(at_str);
    if(str_len <= ZZ_GMT_STRING_MIN_LENGTH)
    {
        return false;
    }
    
    /* 解析第一个空格 */
    temp = (char*)SL_Strstr(p," ");
    if(!temp || (temp-p) <= 0)
    {
        return false;
    }
    field_len = temp -p;
    SL_Memset(buffer,0, sizeof(buffer));
    SL_Strncpy(buffer,p,field_len);
    buffer[field_len]='\0';
    SL_Strcpy(gmt,buffer);
    
    /* 解析interval */
    temp++;
    p = temp;
    temp = (char*)SL_Strstr(p," ");

    /*只有interval */
    if(temp == NULL)
    {
        field_len = str_len-(p -at_str);
        SL_Memset(buffer,0, sizeof(buffer));
        SL_Strncpy(buffer,p,field_len);
        buffer[field_len] = '\0';
        *interval = SL_atoi(buffer);
        if(*interval == 0)
        {
            return false;
        }
        *duration = 0;
        return true; ;
    }

    field_len = temp -p;
    SL_Memset(buffer,0, sizeof(buffer));
    SL_Strncpy(buffer,p,field_len);
    buffer[field_len] = '\0';
    *interval = SL_atoi(buffer);
    if(*interval == 0)
    {
        return false;
    }
    /*解析duartion*/
    temp++;
    p = temp;
    field_len = str_len-(p -at_str);
    SL_Memset(buffer,0, sizeof(buffer));
    SL_Strncpy(buffer,p,field_len);
    buffer[field_len] = '\0';
    *duration = SL_atoi(buffer);
    return true;
}
#if 0
static bool  getTimerId(ZZOneTimer *timer,cJSON *cjson_timer)
{
    cJSON *cjson_id = NULL;
    if(NULL == cjson_timer
        || NULL == (cjson_id = cJSON_GetObjectItem(cjson_timer,ZZ_STRING_ID))
        || cJSON_Number != cjson_id->type)
    {
        return false;
    }
    timer->id = cjson_id->valueint;
	return true;
}
#endif
static bool  getTimerEnabled(ZZOneTimer *timer,cJSON *cjson_timer)
{
    cJSON *cjson_enabled = NULL;
    if(NULL == cjson_timer
        || NULL == (cjson_enabled = cJSON_GetObjectItem(cjson_timer,ZZ_STRING_ENABLED))
        || cJSON_Number != cjson_enabled->type)
    {
        return false;
    }
    timer->enabled= cjson_enabled->valueint;
    zzLogI("enabled = %d",timer->enabled);
	return true;
}
static bool  getTimerType(ZZOneTimer *timer,cJSON *cjson_timer)
{
    cJSON *cjson_type = NULL;
    if(NULL == cjson_timer
        || NULL == (cjson_type = cJSON_GetObjectItem(cjson_timer,ZZ_STRING_TYPE))
        || cJSON_String != cjson_type->type)
    {
        return false;
    }
    if(0 == SL_Strcmp(cjson_type->valuestring,ZZ_STRING_ONCE))
    {
        timer->type = ZZ_TIME_TYPE_ONCE;
    }
    else if(0 == SL_Strcmp(cjson_type->valuestring,ZZ_STRING_REPEAT))
    {
        timer->type = ZZ_TIME_TYPE_REPEAT;
    }
    else if(0 == SL_Strcmp(cjson_type->valuestring,ZZ_STRING_DURATION))
    {
        timer->type = ZZ_TIME_TYPE_DURATION;
    }
    else
    {
        timer->type = ZZ_TIME_TYPE_INVALID;
        return false;
    }  
    zzLogI("type = %d",timer->type);
    return true;
}
static bool  getTImerAt(ZZOneTimer *timer,cJSON *cjson_timer)
{
    cJSON *cjson_at = NULL;
    bool ret = false;
    if(NULL == cjson_timer
        || NULL == (cjson_at = cJSON_GetObjectItem(cjson_timer,ZZ_STRING_AT))
        || cJSON_String != cjson_at->type)
    {
        return false;
    }
    SL_Memset(timer->at,0,sizeof(timer->at));
    SL_Strcpy(timer->at,cjson_at->valuestring);
    zzLogI("at = %s",timer->at);
    switch(timer->type)
    {
        case ZZ_TIME_TYPE_ONCE:
        {
            if (parseGMTTimeFromString(cjson_at->valuestring, &timer->timing.gmt))
            {
                ret = true;
            }
        }break;
        case ZZ_TIME_TYPE_REPEAT:
        {
            if (parseCronFromString(cjson_at->valuestring, &timer->timing.cron))
            {
                ret = true;
            }
        }break;
        case ZZ_TIME_TYPE_DURATION:
        {
            char timer_gmt[30] = {0};
            if(parseFromATString(cjson_at->valuestring,timer_gmt,&timer->time_interval,&timer->time_duration))
            {
                if (parseGMTTimeFromString(timer_gmt, &timer->timing.gmt))
                {
                    ret = true;
                }
            }
        }break;
        default:
        {
            ret = false;
        }
    }
    return ret;
}
static bool  getTimerAction(ZZOneTimer *timer,cJSON *cjson_timer)
{
    cJSON *cjson_do = NULL;
    cJSON *cjson_start_do = NULL;
    cJSON *cjson_end_do = NULL;
    char *action_do = NULL;
    char *action_start_do = NULL;
    char *action_end_do = NULL;
    if(NULL == cjson_timer)
    {
        return false;
    }
    if(NULL != (cjson_do = cJSON_GetObjectItem(cjson_timer,"do")))
	{
        if(cjson_do->type != cJSON_Object)
        {
            return false;
        }
        action_do = cJSON_PrintUnformatted(cjson_do);
        SL_Memset(timer->time_action.action_do,0,sizeof(timer->time_action.action_do));
        SL_Strcpy(timer->time_action.action_do,action_do);
        zzLogI("do = %s",timer->time_action.action_do);
        SL_FreeMemory(action_do);
	} 
	else if(NULL != (cjson_start_do = cJSON_GetObjectItem(cjson_timer,ZZ_STRING_START_DO)) 
	        && (NULL != (cjson_end_do = cJSON_GetObjectItem(cjson_timer,ZZ_STRING_END_DO))))
	{
        if(cjson_start_do->type != cJSON_Object || cjson_end_do->type != cJSON_Object)
        {
            return false;
        }
        action_start_do = cJSON_PrintUnformatted(cjson_start_do);
        action_end_do = cJSON_PrintUnformatted(cjson_end_do);
        SL_Memset(timer->time_action.duration_do.start_do,0,sizeof(timer->time_action.duration_do.start_do));
        SL_Strcpy(timer->time_action.duration_do.start_do,action_start_do);
        SL_FreeMemory(action_start_do);

        SL_Memset(timer->time_action.duration_do.end_do,0,sizeof(timer->time_action.duration_do.end_do));
        SL_Strcpy(timer->time_action.duration_do.end_do,action_end_do);
        SL_FreeMemory(action_end_do);
	}
	else
	{
        return false;
	}
	return true;
}


static bool timerProcOneTimer(ZZOneTimer *one_timer,cJSON *cjson_one_timer)
{
/*
    if(!getTimerId(one_timer,cjson_one_timer))
    {
        return false;
    }
*/
    if(!getTimerEnabled(one_timer,cjson_one_timer))
    {
        return false;
    }
    if(!getTimerType(one_timer,cjson_one_timer))
    {
        return false;
    }
    if(!getTImerAt(one_timer,cjson_one_timer))
    {
        return false;
    }
    if(!getTimerAction(one_timer,cjson_one_timer))
    {
        return false;
    }
    return true;
}
static bool timerProc(cJSON *cjson_timer)
{
    cJSON *cjson_one_timer = NULL;
    uint8 timer_num = 0;
    uint8 i = 0;
    if(NULL == cjson_timer 
        || cJSON_Array != cjson_timer->type
        || 0 > (timer_num = cJSON_GetArraySize(cjson_timer))
        || ZZ_TIMER_MAX_NUM < timer_num)
    {
        return false;
    }
    for(i = 0; i < timer_num; i++)
    {
        cjson_one_timer = cJSON_GetArrayItem(cjson_timer,i);
        if(!timerProcOneTimer(&zz_timers.timer[i],cjson_one_timer))
        {
            SL_Memset(zz_timers.timer,0,sizeof(zz_timers.timer));
            return false;
        }
    } 
    return true;
}
bool zzTimerProc(cJSON *cjson_params)
{
    cJSON *cjson_timer = NULL;
    SL_Memset(zz_timers.timer,0,sizeof(zz_timers.timer));
    if(NULL == cjson_params 
        || NULL == (cjson_timer = cJSON_GetObjectItem(cjson_params,ZZ_STRING_TIMER)))
    {
        return false;
    }
    return timerProc(cjson_timer);
}
bool zzTimerTaskInit(zzTimerCallback timer_callback)
{
    if(NULL == timer_callback)
    {
        return false;
    }
    timer_task.element[0] = SL_CreateTask(timerTask, 
        ZZ_TIMER_TASK_STACKSIZE, 
        ZZ_TIMER_TASK_PRIORITY, 
        ZZ_TIMER_TASK_NAME);

    if (!timer_task.element[0])
    {
        zzLogE("create zzTimerTask failed!");
    }
    zz_timer_action = timer_callback;

    SL_StartTimer(timer_task, 
        ZZ_TIMER_TASK_TIMERID_BEGIN, 
        SL_TIMER_MODE_PERIODIC, 
        SL_MilliSecondToTicks(ZZ_TIMER_SCAN));

    return true;
}

