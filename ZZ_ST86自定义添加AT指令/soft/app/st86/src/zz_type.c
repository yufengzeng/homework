#include "zz_type.h"

/**
 * Copy str in new memory. 
 * 
 * @return pointer to new str or NULL. 
 *
 * @note
 *  Free the returned pointer when out of use. 
 */
char * zzStringCopy(const char *str)
{
    uint16 len = 0;
    char *buf = NULL;

    if (!str || !SL_Strlen(str))
    {
        zzLogE(ZZ_ERR_STR_INP);
        return NULL;
    }

    len = SL_Strlen(str);
    len += 1;
    
    buf = SL_GetMemory(len);
    if (!buf)
    {
        zzLogE(ZZ_ERR_STR_OOM);
        return NULL;
    }

    SL_Memcpy(buf, str, len);

    return buf;
}

