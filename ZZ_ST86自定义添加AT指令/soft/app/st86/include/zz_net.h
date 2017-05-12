#ifndef __ZZ_NET_H__
#define __ZZ_NET_H__

#include "sl_os.h"
#include "sl_stdlib.h"
#include "sl_type.h"
#include "sl_debug.h"
#include "sl_error.h"
#include "sl_sms.h"
#include "sl_call.h"
#include "sl_timer.h"
#include "sl_system.h"
#include "sl_memory.h"
#include "sl_uart.h"
#include "sl_tcpip.h"
#include "sl_audio.h"
#include "sl_gpio.h"
#include "sl_lowpower.h"
#include "sl_filesystem.h"
#include "sl_assistgps.h"

#include "zz_type.h"
#include "zz_log.h"
#include "zz_util.h"
#include "zz_event.h"

void zzNetInit(void);
void zzNetActive(void);
void zzNetDeactive(void);
void zzNetPrintState(void);
void zzNetPrintIp(void);
int8 zzNetGetRssi(void);
bool zzNetGprsIsActived(void);

#endif /* __ZZ_NET_H__ */

