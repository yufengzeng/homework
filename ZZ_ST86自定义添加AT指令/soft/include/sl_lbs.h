/*********************************************************************************
** File Name:     sl_lbs.h                                                *
** Author:         Wang Ya Feng                                                  *
** DATE:           2015/05/06                                                   *
** Copyright:      2013 by SimpLight Nanoelectronics,Ltd. All Rights Reserved.  *
*********************************************************************************

*********************************************************************************
** Description:    Implement for common function of API layer                   *
** Note:           None                                                         *
*********************************************************************************

*********************************************************************************
**                        Edit History                                          *
** -------------------------------------------------------------------------    *
** DATE                   NAME             DESCRIPTION                                  *
** 2015/05/06     Wang Ya Feng       Create
*********************************************************************************/
#ifndef _SL_LBS_H_
#define _SL_LBS_H_

#include "sl_type.h"

/****************************************************
                    macro define

****************************************************/
#define LBS_ADDR_LENTH_MAX     80
#define LBS_ROAD_LENTH_MAX     60
#define LBS_COUNTY_CODE_LENTH_MAX  20
#define LBS_TOWN_CODE_LENTH_MAX  20

typedef enum
{
    SL_LBS_MAP_GPS = 1,
	SL_LBS_MAP_GOOGLE = 2,
	SL_LBS_MAP_BAIDU = 4,
	SL_LBS_MAP_GAODE = 8,
	SL_LBS_MAP_TOTAL
}SL_LBS_MAP_INFO_TYPE;

typedef struct
{
    U32 ulLongi; //����gps����
	U32 ulLati; //����gpsγ��
	BOOL blLongLatValidFlag; // ����gps��γ����Ч��־
	SL_LBS_MAP_INFO_TYPE enMapType;  //���ص�ͼ�������� 1=GPS 2=Google 4=Baidu 8=QQ(�ߵ�) ��ѡ��� ����Ĭ��Ϊ1
    BOOL bAddr;  //�Ƿ񷵻�λ����Ϣ
	BOOL bRoad;  //�Ƿ񷵻ص�·��Ϣ
	BOOL bRadius;  //�Ƿ񷵻ظ��Ƕ���Ϣ
	BOOL bContyDivCode; // �Ƿ񷵻��ؼ�������������
	BOOL bTownDivCode;  // �Ƿ񷵻�����������������
}SL_LBS_REQ_INFO_STRUCT;

typedef struct
{
    U32 ulLongi; //����gps����
	U32 ulLati; //����gpsγ��
    U32 ulLongiGoogle; // ����google��ͼ����
	U32 ulLatiGoogle; // ����google��ͼγ��
    U32 ulLongiBaidu; // ����baidu��ͼ����
	U32 ulLatiBaidu; // ����baidu��ͼγ��
    U32 ulLongiGaode; // ���ظߵµ�ͼ����
	U32 ulLatiGaode; // ���ظߵµ�ͼγ��
	U32 ulRadius; // ���ظ��Ƕ���Ϣ
    U8 aucAddr[LBS_ADDR_LENTH_MAX]; // ���ص�ַ��Ϣ�ַ���
	U8 aucRoad[LBS_ROAD_LENTH_MAX]; // ���ص�·��Ϣ�ַ���
	U8 aucCountyDivCode[LBS_COUNTY_CODE_LENTH_MAX]; // �����ؼ��������������ַ���
	U8 aucTownDivCode[LBS_TOWN_CODE_LENTH_MAX]; // ���������������������ַ���
}SL_LBS_RSP_INFO_STRUCT;

/****************************************************
                                   function

****************************************************/
VOID SL_LbsInit(VOID);
S32 SL_LbsSendReq(SL_LBS_REQ_INFO_STRUCT* pstLbsReqInfo);
S32 SL_LbsRspInfoParse(S8* scRcvInfo, SL_LBS_RSP_INFO_STRUCT* pstRspInfo);

#endif
