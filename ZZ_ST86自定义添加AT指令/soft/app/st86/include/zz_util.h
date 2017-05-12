#ifndef __ZZ_UTIL_H__
#define __ZZ_UTIL_H__

#include "sl_os.h"
#include "sl_stdlib.h"
#include "sl_type.h"
#include "sl_debug.h"
#include "sl_error.h"
#include "sl_timer.h"
#include "sl_system.h"
#include "sl_memory.h"

#include "zz_type.h"
#include "zz_log.h"

typedef struct
{
    uint32 base_inteval;        /* second */
    float32 rand_ratio_max;     /* 0.0 - 1.0 */
} ZZRinterval;

typedef struct
{
    uint32 base;
    uint32 power_max;
    uint32 counter;
} ZZPinterval;


void zzRintInit(ZZRinterval *ri, uint32 base_inteval, float32 rand_ratio_max);
uint32 zzRintVal(ZZRinterval *ri);

void zzPintInit(ZZPinterval *pi, uint32 base, uint32 power_max);
void zzPintReset(ZZPinterval *pi);
uint32 zzPintVal(ZZPinterval *pi);

void zzPrintVerInfo(void);
void zzPrintMemLeft(void);
void zzPrintCurrentTaskPrio(void);

uint32 zzRand(uint32 n1, uint32 n2);
uint32 zzPower(uint32 base, uint32 power);

#endif /* __ZZ_UTIL_H__ */

