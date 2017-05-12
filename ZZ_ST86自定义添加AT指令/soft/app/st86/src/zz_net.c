#include "zz_net.h"

extern void zzStreamOpenCb(U8 cid, S32 stream, BOOL result, S32 error_code);
extern void zzStreamCloseCb(U8 cid, S32 stream, BOOL result, S32 error_code);
extern void zzStreamWriteCb(U8 cid, S32 stream, BOOL result, S32 error_code);
extern void zzStreamReadCb(U8 cid, S32 stream, BOOL result, S32 error_code);

static char zz_net_apn[]        = {"CMNET"};
static char zz_net_username[]   = {"S80"};
static char zz_net_password[]   = {"S80"};

void zzNetActive(void)
{    
    int32 ret = 0;
    
    zzNetPrintState();
    
    SL_TcpipGprsApnSet(zz_net_apn, zz_net_username, zz_net_password);
    ret = SL_TcpipGprsNetActive();
    zzLogIWF(ret);
}

static void zzNetActiveCb(U8 cid, S32 error_code)
{
    zzLogI("zzNetActiveCb cid = %u, error_code = %d", cid, error_code);
    if (!error_code)
    {
        zzNetPrintIp();
        zzNetPrintState();
    }

    zzEmitEvt2Main(ZZ_EVT_MAIN_TASK_ACTIVE_DONE);
}

void zzNetDeactive(void)
{
    int32 ret = 0;

    zzNetPrintState();
    
    ret = SL_TcpipGprsNetDeactive();
    zzLogIWF(ret);
}

static void zzNetDeactiveCb(U8 cid, S32 error_code)
{
    zzLogI("zzNetDeactiveCb cid = %u, error_code = %d", cid, error_code);
    zzNetPrintState();
    
    zzEmitEvt2Main(ZZ_EVT_MAIN_TASK_DEACTIVE_DONE);
}

void zzNetPrintIp(void)
{
    int32 ret = 0;
    char local_ip[20] = {0};
    uint8 cid = 0;
    
    SL_Memset(local_ip, 0, sizeof(local_ip));
    cid = SL_TcpipGetCid();
    ret = SL_TcpipGetLocalIpAddr(cid, local_ip);
    if (ret)
    {
        zzLogW("SL_TcpipGetLocalIpAddr err(%d)", ret);
    }
    
    zzLogI("cid = %d, local ip = [%s]", cid, local_ip);
}


/**
 * @note 
 *  There is 5 types of state in GSM/GPRS system
 *  1. sim card state - SL_SIM_STATUS
 *  2. net reg state - SL_NW_REG_STATUS
 *  3. gprs net reg state - SL_NW_REG_STATUS
 *  4. gprs attach state - SL_GPRS_ATTACH_STATE
 *  5. gprs tpcip cid state - SL_GPRS_CID_STATE
 */
void zzNetPrintState(void)
{
    int32 sim = 0;
    int32 net_reg = 0;
    int32 gprs_net_reg = 0;
    uint8 gprs_attach = 0;
    uint8 gprs_tcpip_cid = 0;
    uint8 gprs_net_reg2 = 0;
    int8 rssi = 0;
    uint8 ber = 0;
    int32 ret = 0;

    
    SL_GetDeviceCurrentRunState(&sim, &net_reg, &gprs_net_reg, &rssi, &ber);
 
    ret = SL_GprsGetAttState(&gprs_attach);
    if (ret)
    {
        zzLogE("SL_GprsGetAttState err(%d)", ret);
    }
    
    ret = SL_TcpipGprsNetGetState(&gprs_tcpip_cid, &gprs_net_reg2);
    if (ret)
    {
        zzLogE("SL_TcpipGprsNetGetState err(%d)", ret);
    }

    zzLogP("{%d %u %u %u %u %d %u}",
        sim, net_reg, gprs_net_reg, gprs_attach, gprs_tcpip_cid, rssi, ber);
}

bool zzNetGprsIsActived(void)
{
    uint8 gprs_tcpip_cid = 0;
    uint8 gprs_net_reg2 = 0;
    int32 ret = 0;    
    
    ret = SL_TcpipGprsNetGetState(&gprs_tcpip_cid, &gprs_net_reg2);
    if (ret)
    {
        zzLogE("SL_TcpipGprsNetGetState err(%d)", ret);
        return false;
    }

    if (SL_GPRS_ACTIVED == gprs_tcpip_cid)
    {
        return true;
    }
    else
    {
        return false;
    }
}

int8 zzNetGetRssi(void)
{
    int32 sim = 0;
    int32 net_reg = 0;
    int32 gprs_net_reg = 0;
    int8 rssi = 0;
    uint8 ber = 0;

    SL_GetDeviceCurrentRunState(&sim, &net_reg, &gprs_net_reg, &rssi, &ber);
    
    return rssi;     
}

void zzNetInit(void)
{
    int32 ret = 0;
    
    SL_TCPIP_CALLBACK tcpip_cbs;

    SL_Memset(&tcpip_cbs, 0, sizeof(tcpip_cbs));
    
    tcpip_cbs.pstSlnetAct = zzNetActiveCb;
    tcpip_cbs.pstSlnetDea = zzNetDeactiveCb;
    tcpip_cbs.pstSlsockConn = zzStreamOpenCb;
    tcpip_cbs.pstSlsockClose = zzStreamCloseCb;
    tcpip_cbs.pstSlsockAcpt = NULL;
    tcpip_cbs.pstSlsockRecv = zzStreamReadCb;
    tcpip_cbs.pstSlsockSend = zzStreamWriteCb;
    tcpip_cbs.pstSlGetHostName = NULL;
 
    ret = SL_TcpipGprsNetInit(0, &tcpip_cbs);
    zzLogIEF(ret);
}

