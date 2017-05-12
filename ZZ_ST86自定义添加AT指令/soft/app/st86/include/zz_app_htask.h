#ifndef __ZZ_APP_HTASK_H__
#define __ZZ_APP_HTASK_H__

#include "sl_os.h"
#include "sl_stdlib.h"
#include "sl_type.h"
#include "sl_debug.h"
#include "sl_error.h"
#include "sl_timer.h"
#include "sl_system.h"
#include "sl_memory.h"

#include "zz_log.h"
#include "zz_mem.h"
#include "zz_util.h"
#include "zz_event.h"
#include "zz_error.h"
#include "zz_task.h"


void zzAppHtaskInit(void);
ZZDeviceState iotgoGetDeviceState(void);
void iotgoSetDeviceMode(ZZDeviceState state);
bool zzRelayIsOn(void);
void zzRelayExecuteTimer(void *arg);
void zzRelaySetDef(const char *startup);
const char * zzRelayGetDef(void);
bool zzDeviceIsEnabled(void);
void zzDeviceDisable(void);

#endif /* __ZZ_APP_HTASK_H__ */

