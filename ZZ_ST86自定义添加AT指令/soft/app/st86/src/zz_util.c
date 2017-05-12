#include "zz_util.h"

#define LIB_VER_LEN             50
#define APP_VER_LEN             50
#define IMEI_LEN                50
#define SN_LEN                  50
#define IMSI_LEN                50
#define ICCID_LEN               50
#define OPERATOR_LEN            50

static char libver[LIB_VER_LEN] = {0};
static char imei[IMEI_LEN] = {0};
static char sn [SN_LEN] = {0};
static char imsi[IMSI_LEN] = {0};
static char iccid[ICCID_LEN] = {0};
static char operator[OPERATOR_LEN] = {0};

/* 
 * Return a number between n1 and n2(included two ends). 
 * 
 * @param n1 - the smaller.
 * @param n2 - the bigger.
 * @retval random number between n1 and n2. 
 */
uint32 zzRand(uint32 n1, uint32 n2)
{  
    uint32 n_min = 0;
    uint32 n_max = 0;
    uint32 n_diff = 0;
    uint32 ret = 0;
    
    if (n2 > n1)
    {
        n_max = n2;
        n_min = n1;
    }
    else if (n2 < n1)
    {
        n_max = n1;
        n_min = n2;
    }
    else /* if (n1 == n2) */
    {
        return n2;
    }

    n_diff = n_max - n_min;
    ret = n_min + (SL_TmGetTick() % (n_diff + 1) + (n_diff + 1)) % (n_diff + 1);
    return ret;
}

void zzRintInit(ZZRinterval *ri, uint32 base_inteval, float32 rand_ratio_max)
{
    if (!ri)    
    {
        zzLogE(ZZ_ERR_STR_INP);
        return;
    }

    ri->base_inteval = base_inteval;
    ri->rand_ratio_max = rand_ratio_max;
}

/*
 * retval = base_inteval + rand(0, base_inteval * rand_ratio_max)
 */
uint32 zzRintVal(ZZRinterval *ri)
{
    if (!ri)    
    {
        zzLogE(ZZ_ERR_STR_INP);
        return 1;
    }

    return ri->base_inteval + zzRand(0, (uint32)(1.0f * ri->base_inteval * ri->rand_ratio_max + 0.5));
}

void zzPintInit(ZZPinterval *pi, uint32 base, uint32 power_max)
{
    if (!pi)    
    {
        zzLogE(ZZ_ERR_STR_INP);
        return;
    }

    pi->base = base;
    pi->power_max = power_max;
    pi->counter = 0;
}

void zzPintReset(ZZPinterval *pi)
{
    if (!pi)    
    {
        zzLogE(ZZ_ERR_STR_INP);
        return;
    }

    pi->counter = 0;
}

/*
 * if counter < power_max
 *  retval = base + rand(2^(counter), 2^(counter + 1))
 * else
 *  retval = base + rand(2^(power_max), 2^(power_max + 1))
 */
uint32 zzPintVal(ZZPinterval *pi)
{
    uint32 ret = 1;
    
    if (!pi)    
    {
        zzLogE(ZZ_ERR_STR_INP);
        return 1;
    }
    
    if (pi->counter < pi->power_max)
    {
        ret = pi->base + zzRand(zzPower(2, pi->counter), zzPower(2, pi->counter + 1));
    }
    else
    {
        ret = pi->base + zzRand(zzPower(2, pi->power_max), zzPower(2, pi->power_max + 1));    
    }

    pi->counter++;
    
    return ret;
}

uint32 zzPower(uint32 base, uint32 power)
{
    uint32 i = 0;
    uint32 ret = 1;
    for (i = 0; i < power; i++)
    {
        ret *= base;
    }
    return ret;
}


static void zzGetCCIDCb(S32 result, U8* ccid, U8 ccid_len)
{
    zzLogI("zzGetCCIDCb result = %d, len = %u", result, ccid_len);
    if (!result)
    {
        SL_Strncpy(iccid, ccid, ccid_len);
        zzLogI("iccid = [%s]", iccid);    
    }
    else
    {
        zzLogE("zzGetCCIDCb err");
    }
}

void zzPrintVerInfo(void)
{
    SL_GSM_GetIMEI(imei, sizeof(imei));
    SL_GSM_GetSN(sn, sizeof(sn));
    SL_GetCoreVer(libver, sizeof(libver));
    SL_SIM_GetIMSI(imsi, sizeof(imsi));
    SL_GetOperator(operator, sizeof(operator));
    
    zzLogI("imei = [%s]\n"
        "sn = [%s]\n"
        "imsi = [%s]\n"
        "libver = [%s]\n"
        "operator = [%s]\n",
        imei, sn, imsi, libver, operator
        );

    SL_SimGetCCID(zzGetCCIDCb);
    
}


void zzPrintMemLeft(void)
{
    zzLogI("{MemTotalLeftSize: %u bytes}", SL_MemTotalLeftSize());
}

void zzPrintCurrentTaskPrio(void)
{
    zzLogI("current task prio %u", SL_GetCurrentTaskPriority());
}

