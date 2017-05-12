#ifndef __SL_WIFI_H__
#define __SL_WIFI_H__
#define SL_WIFI_AP_MAX_CNT		16
#define SL_IW_ESSID_MAX_SIZE	32
#define SL_ETH_ALEN				6
typedef void (*SL_WIFI_POWERON_CB)(S32 slResult, U8* ucpMacAddr, U32 ulLen);
typedef void (*SL_WIFI_POWEROFF_CB)(S32 slResult);
typedef void (*SL_WIFI_SCAN_CB)(U8* ucpDesc, U32 ulCnt);
typedef void (*SL_WIFI_CONNECTAP_CB)(S32 slResult, U32 ulErrNo);
typedef void (*SL_WIFI_DISCONNECTAP_CB)(void);
typedef void (*SL_WIFI_DHCP_CB)(S32 slResult, void* vpIpAddrInd);
typedef struct{
    SL_WIFI_POWERON_CB pstSlWifiPwrOn;
    SL_WIFI_POWEROFF_CB pstSlWifiPwrOff;
	SL_WIFI_SCAN_CB pstSlWifiScan;
	SL_WIFI_CONNECTAP_CB pstSlWifiConnectAp;
	SL_WIFI_DISCONNECTAP_CB pstSlWifiDisconnectAp;
	SL_WIFI_DHCP_CB pstSlWifiDhcp;
}SL_WIFI_CALLBACK;
typedef struct{
	U8 ssid[SL_IW_ESSID_MAX_SIZE + 1];
	U8 bss_type;
	U8 channel;
	U8 dot11i_info;
	U8 bssid[SL_ETH_ALEN];
	U8 rssi;
	U8 auth_info;
	U8 rsn_cap[2];
	U8 rsn_grp;
    U8 rsn_pairwise[3];
    U8 rsn_auth[3];
    U8 wpa_grp;
    U8 wpa_pairwise[3];
    U8 wpa_auth[3];
}SL_WIFI_BSS_DESC_t;
typedef struct
{
	U8 ref_count;
	U16 msg_len;  
	U8 cause;
	U8 ip_addr[4];
	U8 pri_dns_addr[4];
	U8 sec_dns_addr[4];
	U8 gateway[4];
	U8 netmask[4];
}SL_WIFI_IPADDR_IND_t;
typedef enum {
	SL_WIFI_PWRON_NORMAL,
	SL_WIFI_PWRON_TX_MACDATA,	
}SL_WIFI_PWRON_MODE_t;
#include "sl_app.h"
#define SL_WifiGetCb AA.SL_WifiGetCb_p
#define SL_WifiInit AA.SL_WifiInit_p
#define SL_wifi_power_on AA.SL_wifi_power_on_p
#define SL_WifiPowerOn AA.SL_WifiPowerOn_p
#define SL_WifiPowerOff AA.SL_WifiPowerOff_p
#define SL_WifiScan AA.SL_WifiScan_p
#define SL_WifiConnectAp AA.SL_WifiConnectAp_p
#define SL_WifiDisconnectAp AA.SL_WifiDisconnectAp_p
#define SL_WifiDhcp AA.SL_WifiDhcp_p
#define SL_WifiSendMacData AA.SL_WifiSendMacData_p
#define SL_WifiGetPwrOnMode AA.SL_WifiGetPwrOnMode_p
#define SL_WifiSetTxPower AA.SL_WifiSetTxPower_p
#endif
