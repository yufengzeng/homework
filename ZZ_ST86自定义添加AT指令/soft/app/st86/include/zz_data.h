#ifndef __ZZ_DATA_H__
#define __ZZ_DATA_H__

#include "sl_type.h"
#include "zz_log.h"
#include "zz_error.h"
#include "zz_mem.h"

typedef struct 
{
    uint8 *start;
    uint32 length;
} ZZData;

#define ZZ_DATA_NULL          {NULL, 0}

ZZData zzData(const uint8 *start, uint32 length);
ZZData zzDataCreate(uint32 length);
ZZData zzDataCopy(ZZData data);
ZZData zzDataMerge(ZZData data1, ZZData data2);
ZZData zzDataSub(ZZData data, uint32 begin, uint32 end);
#if 0
void zzDataFree(ZZData data);
#else
void zzDataFree(ZZData *data);
#endif

#endif /* __ZZ_DATA_H__ */

