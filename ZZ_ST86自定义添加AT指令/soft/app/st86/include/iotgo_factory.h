#ifndef __IOTGO_FACTORY_H__
#define __IOTGO_FACTORY_H__

#include "sl_os.h"
#include "sl_stdlib.h"
#include "sl_type.h"
#include "sl_debug.h"
#include "sl_error.h"
#include "sl_sms.h"
#include "sl_timer.h"
#include "sl_system.h"
#include "sl_memory.h"
#include "sl_tcpip.h"
#include "sl_gpio.h"
#include "sl_filesystem.h"

#include "cJSON.h"
#include "sha256.h"
#include "zz_task.h"
#include "zz_event.h"
#include "zz_log.h"
#include "zz_app_mtask.h"
#include "zz_app_htask.h"

#include "zz_net.h"       //////added by zyf
#include "zz_at_cmd.h"    //////added by zyf



BOOL iotgoFactoryVerifyDeviceIdInfo(void);
void iotgoFactoryMainLoop(void);
BOOL iotgoIsBootForFactory(void);
void iotgoFactoryDeleteDeviceIdInfoFile(void);


#endif
