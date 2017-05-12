#include "zz_data.h"

/**
 * Create data in new memory by copying from start with length. 
 * 
 * @note
 *  Free the returned data when out of use.  
 *  CAUTION: This function DO NOT free the memory of start. 
 */
ZZData zzData(const uint8 *start, uint32 length)
{
    ZZData ret = ZZ_DATA_NULL;
    uint8 *buf = NULL;
    
    if (!start || !length)
    {
        zzLogE(ZZ_ERR_STR_INP);
        return ret;
    }
    
    buf = SL_GetMemory(length + 1);
    if (!buf)
    {
        zzLogE(ZZ_ERR_STR_OOM);
        return ret;
    }

    SL_Memcpy(buf, start, length);
    buf[length] = '\0';
    
    ret.length = length;
    ret.start = buf;
    
    return ret;
}

/**
 * Create data with filled zero. 
 *
 * @note
 *  Free the returned data when out of use. 
 */
ZZData zzDataCreate(uint32 length)
{
    ZZData ret = ZZ_DATA_NULL;
    uint8 *buf = NULL;

    if (!length)
    {
        zzLogE(ZZ_ERR_STR_INP);
        return ret;
    }

    buf = SL_GetMemory(length + 1);
    if (!buf)
    {
        zzLogE(ZZ_ERR_STR_OOM);
        return ret;
    }

    zzMemzero(buf, length);
    buf[length] = '\0';

    ret.length = length;
    ret.start = buf;

    return ret;
}


/**
 * Copy data in new memory. 
 * 
 * @return new data. 
 *
 * @note
 *  Free the returned data when out of use. 
 */
ZZData zzDataCopy(ZZData data)
{
    ZZData ret = ZZ_DATA_NULL;
    uint8 *buf = NULL;
    
    if (!data.start || !data.length)
    {
        zzLogE(ZZ_ERR_STR_INP);
        return ret;
    }

    buf = SL_GetMemory(data.length + 1);
    if (!buf)
    {
        zzLogE(ZZ_ERR_STR_OOM);
        return ret;
    }

    SL_Memcpy(buf, data.start, data.length);
    buf[data.length] = '\0';
    
    ret.length = data.length;
    ret.start = buf;
    
    return ret;
}


/**
 * Copy and merge in new memory. 
 * 
 * @return new merged data. 
 *
 * @note
 *  Free the returned data when out of use. 
 *  CAUTION: This function DO NOT free the memory of data1 and data2. 
 */
ZZData zzDataMerge(ZZData data1, ZZData data2)
{
    ZZData ret = ZZ_DATA_NULL;
    uint8 *buf = NULL;
    uint32 total = 0;
    
    if (!data1.start 
        || !data1.length
        || !data2.start 
        || !data2.length
        )
    {
        zzLogE(ZZ_ERR_STR_INP);
        return ret;
    }

    total = data1.length + data2.length;
    
    buf = SL_GetMemory(total + 1);
    if (!buf)
    {
        zzLogE(ZZ_ERR_STR_OOM);
        return ret;
    }

    SL_Memcpy(buf, data1.start, data1.length);
    SL_Memcpy(&buf[data1.length], data2.start, data2.length);
    buf[total] = '\0';
    
    ret.length = total;
    ret.start = buf;
    
    return ret;
}

/**
 * Copy sub data to new memory(including begin and end). 
 *
 * @note
 *  Free the returned data when out of use. 
 */
ZZData zzDataSub(ZZData data, uint32 begin, uint32 end)
{
    ZZData ret = ZZ_DATA_NULL;
    uint8 *buf = NULL;
    uint32 sub_len = 0;    
    
    if (!data.start || !data.length || (end >= data.length) || (end < begin))
    {
        zzLogE(ZZ_ERR_STR_INP);
        return ret;
    }

    sub_len = end - begin + 1;

    buf = SL_GetMemory(sub_len + 1);
    if (!buf)
    {
        zzLogE(ZZ_ERR_STR_OOM);
        return ret;
    }

    SL_Memcpy(buf, &data.start[begin], sub_len);
    buf[sub_len] = '\0';
    
    ret.length = sub_len;
    ret.start = buf;
    
    return ret;
}

#if 0
/**
 * Free data.start. 
 */
void zzDataFree(ZZData data)
{
    if (!data.start)
    {
        zzLogE("start is NULL");
        return;
    }

    if (!data.length)
    {
        zzLogW("length is 0");
    }
    
    SL_FreeMemory(data.start);
}
#else

/**
 * Free data->start and zerolize data's obj. 
 *
 * @note
 *  After calling this, data's obj is filled with zero. 
 */
void zzDataFree(ZZData *data)
{
    if (!data || !data->start)
    {
        zzLogE(ZZ_ERR_STR_INP);
        return;
    }

    if (!data->length)
    {
        zzLogW("length is 0");
    }
    
    SL_FreeMemory(data->start);
    zzMemzero(data, sizeof(ZZData));
}

#endif

