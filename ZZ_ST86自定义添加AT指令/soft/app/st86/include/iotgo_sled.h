#ifndef __IOTGO_SLED_H__
#define __IOTGO_SLED_H__

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

#include "zz_type.h"
#include "zz_app_htask.h"

void  iotgoSledInit(void);
void  iotgoSledScanCb(void);


#endif /* __IOTGO_SLED_H__ */
