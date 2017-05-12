#ifndef __ZZ_APP_MTASK_H__
#define __ZZ_APP_MTASK_H__

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

void zzAppMtaskInit(void);
void zzSetDeviceid(const char *deviceid);
const char * zzGetDeviceid(void);
const char * zzGetApikey(void);
const char * zzGetModel(void);
void zzSetApikey(const char *apikey);
void zzSetModel(const char *model);
void returnErrorCode2Ps(int32 err_code, const char *seq);

#endif /* __ZZ_APP_MTASK_H__ */

