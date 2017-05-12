#ifndef __IOTGO_UPGRADE_H__
#define __IOTGO_UPGRADE_H__

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
#include "zz_log.h"

int32 iotgoUpgradeParseUpgrade(const char *json_str);
void iotgoUpgradeCreateUpgradeTask(void);

#if 0
void iotgoStartUpgrade(void);// for test
#endif

#endif

