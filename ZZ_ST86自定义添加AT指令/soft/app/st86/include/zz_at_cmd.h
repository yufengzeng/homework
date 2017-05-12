#ifndef _AT_CMD_
#define _AT_CMD_

#include "zz_type.h"
#include "sl_stdlib.h"
#include "sl_memory.h"






#define FiFOLEN  255  			 //fifo缓存长度


//#define my_malloc(s)  os_malloc(s)				//根据环境的内存函数定义
#define my_malloc(s)         SL_GetMemory(s)					   
#define my_free(s)           SL_FreeMemory(s)
#define mystrstr(s1,s2)      SL_Strstr(s1,s2)
//#define my_strlen        SL_Strlen

typedef enum {
    ctError =0,
     ctTEST  ,
     ctQUERY   ,
     ctSETUP,
     ctEXE
} CmdType;

typedef struct AT_OrderCB
{ 
    uint8 *at_cmdName;
    void (*at_testCmd)(void);
    void (*at_queryCmd)(void);
    void (*at_setupCmd)(uint8 *pPara);
    void (*at_exeCmd)(void);
   
}AT_OCB;



void PrivAT_init(AT_OCB *ptr, uint16 ptrlen);


void RecvDataDeal(uint8* pdata, uint16 datalen);


#endif

