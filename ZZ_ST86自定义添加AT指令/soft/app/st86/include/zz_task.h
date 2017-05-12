/**
 * @note
 *  1. All zz system tasks' stacksize, priority and taskname should be defined here. 
 */
#ifndef __ZZ_TASK_H__
#define __ZZ_TASK_H__

#define ZZ_APP_TASK_PRIORITY_BEGIN      (233)   
#define ZZ_APP_TASK_PRIORITY_END        (240)   

#define ZZ_TASK_PRIORITY_BEGIN          (241)   
#define ZZ_TASK_PRIORITY_END            (244)   


#define ZZ_MAIN_TASK_TIMERID_BEGIN      (200)
#define ZZ_MAIN_TASK_TIMERID_END        (209)


#define ZZ_HTTP_REQ_TASK_NAME           "zzHttpReqTask"
#define ZZ_HTTP_REQ_TASK_STACKSIZE      (2048)
#define ZZ_HTTP_REQ_TASK_PRIORITY       (244)   /* lowest prio */
#define ZZ_HTTP_REQ_TASK_TIMERID_BEGIN  (0)
#define ZZ_HTTP_REQ_TASK_TIMERID_END    (9)


#define ZZ_APP_HTASK_NAME               "zzAppHtask"
#define ZZ_APP_HTASK_STACKSIZE          (2048)
#define ZZ_APP_HTASK_PRIORITY           (235)
#define ZZ_APP_HTASK_TIMERID_BEGIN      (10)
#define ZZ_APP_HTASK_TIMERID_END        (19)


#define ZZ_APP_MTASK_NAME               "zzAppMtask"
#define ZZ_APP_MTASK_STACKSIZE          (2048)
#define ZZ_APP_MTASK_PRIORITY           (236)
#define ZZ_APP_MTASK_TIMERID_BEGIN      (20)
#define ZZ_APP_MTASK_TIMERID_END        (29)


#define ZZ_APP_LTASK_NAME               "zzAppLtask"
#define ZZ_APP_LTASK_STACKSIZE          (2048)
#define ZZ_APP_LTASK_PRIORITY           (237)
#define ZZ_APP_LTASK_TIMERID_BEGIN      (30)
#define ZZ_APP_LTASK_TIMERID_END        (39)

/*
 * zzTimerTask has the highest prio. 
 */
#define ZZ_TIMER_TASK_NAME              "zzTimerTask"
#define ZZ_TIMER_TASK_STACKSIZE         (2048) 
#define ZZ_TIMER_TASK_PRIORITY          (233)
#define ZZ_TIMER_TASK_TIMERID_BEGIN     (40)
#define ZZ_TIMER_TASK_TIMERID_END       (49)

/*
 * zzUpgradeTask
 */
#define ZZ_UPGRADE_TASK_NAME              "zzUpgradeTask"
#define ZZ_UPGRADE_TASK_STACKSIZE         (2048) 
#define ZZ_UPGRADE_TASK_PRIORITY          (238)
#define ZZ_UPGRADE_TASK_TIMERID_BEGIN     (50)
#define ZZ_UPGRADE_TASK_TIMERID_END       (59)

/*
 * zzWsockTask
 */
#define ZZ_WSOCK_TASK_NAME              "zzWsockTask"
#define ZZ_WSOCK_TASK_STACKSIZE         (2048) 
#define ZZ_WSOCK_TASK_PRIORITY          (244)
#define ZZ_WSOCK_TASK_TIMERID_BEGIN     (60)
#define ZZ_WSOCK_TASK_TIMERID_END       (69)



#endif /* __ZZ_TASK_H__ */

