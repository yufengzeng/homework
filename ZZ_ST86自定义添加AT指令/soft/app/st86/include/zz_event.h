#ifndef __ZZ_EVENT_H__
#define __ZZ_EVENT_H__

#include "sl_os.h"
#include "sl_stdlib.h"
#include "sl_type.h"
#include "sl_debug.h"
#include "sl_error.h"
#include "sl_timer.h"
#include "sl_system.h"
#include "sl_memory.h"

#include "zz_log.h"
#include "zz_util.h"


#define ZZ_EVT_BASE                         (SL_EV_MMI_EV_BASE)

/* main task event */
#define ZZ_EVT_MAIN_TASK_TIMER              (SL_EV_TIMER)
#define ZZ_EVT_MAIN_TASK_START              (ZZ_EVT_BASE + 0)
#define ZZ_EVT_MAIN_TASK_ATTACH             (ZZ_EVT_BASE + 1)
#define ZZ_EVT_MAIN_TASK_DETACH             (ZZ_EVT_BASE + 2)
#define ZZ_EVT_MAIN_TASK_ACTIVE             (ZZ_EVT_BASE + 3)
#define ZZ_EVT_MAIN_TASK_DEACTIVE           (ZZ_EVT_BASE + 4)
#define ZZ_EVT_MAIN_TASK_ACTIVE_DONE        (ZZ_EVT_BASE + 5)
#define ZZ_EVT_MAIN_TASK_DEACTIVE_DONE      (ZZ_EVT_BASE + 6)
#define ZZ_EVT_MAIN_TASK_DEVICE_ENABLED     (ZZ_EVT_BASE + 7)


/* http req task event */
#define ZZ_EVT_HTTP_REQ_TASK_TIMER          (SL_EV_TIMER)
#define ZZ_EVT_HTTP_REQ_TASK_START          (ZZ_EVT_BASE + 100)


/* app htask event */
#define ZZ_EVT_APP_HTASK_TIMER              (SL_EV_TIMER)
#define ZZ_EVT_APP_HTASK_START              (ZZ_EVT_BASE + 200)
#define ZZ_EVT_APP_HTASK_RELAY_ON           (ZZ_EVT_BASE + 201)
#define ZZ_EVT_APP_HTASK_RELAY_OFF          (ZZ_EVT_BASE + 202)
#define ZZ_EVT_APP_HTASK_USER_ENABLE        (ZZ_EVT_BASE + 203)



/* app mtask event */
#define ZZ_EVT_APP_MTASK_TIMER              (SL_EV_TIMER)
#define ZZ_EVT_APP_MTASK_START              (ZZ_EVT_BASE + 300)
#define ZZ_EVT_APP_MTASK_GET_PS             (ZZ_EVT_BASE + 301)
#define ZZ_EVT_APP_MTASK_GET_PS_OK          (ZZ_EVT_BASE + 302)
#define ZZ_EVT_APP_MTASK_WSOCK_START        (ZZ_EVT_BASE + 303)
#define ZZ_EVT_APP_MTASK_WSOCK_START_OK     (ZZ_EVT_BASE + 304)
#define ZZ_EVT_APP_MTASK_LOGIN              (ZZ_EVT_BASE + 305)
#define ZZ_EVT_APP_MTASK_LOGIN_OK           (ZZ_EVT_BASE + 306)
#define ZZ_EVT_APP_MTASK_UPDATE_TO_PS       (ZZ_EVT_BASE + 307)
#define ZZ_EVT_APP_MTASK_UPDATE_TO_PS_OK    (ZZ_EVT_BASE + 308)
#define ZZ_EVT_APP_MTASK_UPDATE_BY_PS       (ZZ_EVT_BASE + 309)
#define ZZ_EVT_APP_MTASK_UPDATE_BY_PS_OK    (ZZ_EVT_BASE + 310)
#define ZZ_EVT_APP_MTASK_QUERY_TO_PS        (ZZ_EVT_BASE + 311)
#define ZZ_EVT_APP_MTASK_QUERY_TO_PS_OK     (ZZ_EVT_BASE + 312)
#define ZZ_EVT_APP_MTASK_DATE_TO_PS         (ZZ_EVT_BASE + 313)
#define ZZ_EVT_APP_MTASK_DATE_TO_PS_OK      (ZZ_EVT_BASE + 314)
#define ZZ_EVT_APP_MTASK_UPGRADE            (ZZ_EVT_BASE + 315)
#define ZZ_EVT_APP_MTASK_UPGRADE_OK         (ZZ_EVT_BASE + 316)
#define ZZ_EVT_APP_MTASK_GET_PS_ERR         (ZZ_EVT_BASE + 317)
#define ZZ_EVT_APP_MTASK_WSOCK_ERR          (ZZ_EVT_BASE + 318)
#define ZZ_EVT_APP_MTASK_REDIRECT_OK        (ZZ_EVT_BASE + 319)
#define ZZ_EVT_APP_MTASK_LOGIN_ERR          (ZZ_EVT_BASE + 320)



/* app ltask event */
#define ZZ_EVT_APP_LTASK_TIMER              (SL_EV_TIMER)
#define ZZ_EVT_APP_LTASK_START              (ZZ_EVT_BASE + 400)



/* factory task event */
#define ZZ_EVT_FTRY_TASK_START              (ZZ_EVT_BASE + 500)


/* timer task event */
#define ZZ_EVT_TIMER_TASK_TIMER             (SL_EV_TIMER)
#define ZZ_EVT_TIMER_TASK_START             (ZZ_EVT_BASE + 600)

/* upgrade task event */
#define ZZ_EVT_UPGRADE_TASK_TIMER           (SL_EV_TIMER)
#define ZZ_EVT_UPGRADE_TASK_START           (ZZ_EVT_BASE + 700)
#define ZZ_EVT_UPGRADE_TASK_STOP            (ZZ_EVT_BASE + 701) /* P1(error_code) */

/* wsock task event */
#define ZZ_EVT_WSOCK_TASK_TIMER           (SL_EV_TIMER)
#define ZZ_EVT_WSOCK_TASK_START           (ZZ_EVT_BASE + 800)


/* http req test task event */
#define ZZ_EVT_HTTPREQTEST_TASK_TIMER       (SL_EV_TIMER)
#define ZZ_EVT_HTTPREQTEST_TASK_START       (ZZ_EVT_BASE + 1000)


/* wsock test task event */
#define ZZ_EVT_WSOCKTEST_TASK_TIMER         (SL_EV_TIMER)
#define ZZ_EVT_WSOCKTEST_TASK_START         (ZZ_EVT_BASE + 1100)


/* stream test task event */
#define ZZ_EVT_STREAMTEST_TASK_OPEN         (ZZ_EVT_BASE + 1200)
#define ZZ_EVT_STREAMTEST_TASK_CLOSE        (ZZ_EVT_BASE + 1201)
#define ZZ_EVT_STREAMTEST_TASK_WRITE        (ZZ_EVT_BASE + 1202)
#define ZZ_EVT_STREAMTEST_TASK_READ         (ZZ_EVT_BASE + 1203)
#define ZZ_EVT_STREAMTEST_TASK_OPEN_DONE    (ZZ_EVT_BASE + 1204)
#define ZZ_EVT_STREAMTEST_TASK_CLOSE_DONE   (ZZ_EVT_BASE + 1205)
#define ZZ_EVT_STREAMTEST_TASK_WRITE_DONE   (ZZ_EVT_BASE + 1206)
#define ZZ_EVT_STREAMTEST_TASK_READ_DO      (ZZ_EVT_BASE + 1207) /* P2(uint8 *pdata, uint16 len) */



void zzEmitEvt(SL_TASK task, uint32 msg);
void zzEmitEvtP1(SL_TASK task, uint32 msg, uint32 param1);
void zzEmitEvtP2(SL_TASK task, uint32 msg, uint32 param1, uint32 param2);
void zzEmitEvtP3(SL_TASK task, uint32 msg, uint32 param1, uint32 param2, uint32 param3);
void zzEmitEvt2Main(uint32 msg);

#endif /* __ZZ_EVENT_H__ */

