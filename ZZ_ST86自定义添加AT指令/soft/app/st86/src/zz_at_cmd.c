#include "zz_at_cmd.h"



/**
 *Record a fifo buffer's info.
 *
 *
 */
typedef struct _fifobuffer_{
         uint16	         FifoBuffSize;
         uint8           pFifoBuff[FiFOLEN +1];  
         uint16         InPos;  //next In position
         uint16         OutPos; //next out position
}FIFOBUFF;


AT_OCB *GetArrayPtr;
uint16      GetCmdNum;
FIFOBUFF FifoBuffer;



const uint8*CommdStartFlag ={"AT"};		    //
const uint8*CommdEndFlag   ={"\r\n"};   //自定义
//const uint8*CommdEndFlag   ={"\n"};




AT_OCB* FindInCmdLib(uint8 str[],uint16 strlen);
uint16 GetStrLen(uint8*str);



static uint16 Get_Fifo_Usedblock(void)
{
    return (FifoBuffer.FifoBuffSize + FifoBuffer.InPos - FifoBuffer.OutPos) % FifoBuffer.FifoBuffSize;
}


//+1 means the block before Outpos is not allowed to write
static uint16 Get_Fifo_Freeblock(void)
{
    return (FifoBuffer.FifoBuffSize - Get_Fifo_Usedblock()-1);
}


static uint8 Read_Fifo_Usedblock(uint16 outpos)
{
    uint8 value =0;
    if(outpos> FifoBuffer.FifoBuffSize-1)return 0xff; //error
    if(FifoBuffer.InPos == outpos)
    {
      return 0xff; //stand for fifo empty;
    }
    else {     
      value = FifoBuffer.pFifoBuff[outpos];
      //FifoBuffer.OutPos = (uint16)(FifoBuffer.OutPos+1)%FifoBuffer.FifoBuffSize
      //标准fifo应该用FifoBuffer.OutPos代替上述outpos，本项目FifoBuffer.OutPos不变
      return value;
    }
      
      
    
}



//to avoid the head is equal to the tail,the block before OutPos is not allowed 
// write;
static void Write_Fifo_Freeblock(uint8 data)
{
	    uint16 next_write_index = (uint16)(FifoBuffer.InPos + 1) % FifoBuffer.FifoBuffSize;
	    if (next_write_index != FifoBuffer.OutPos) {
	        FifoBuffer.pFifoBuff[FifoBuffer.InPos] = data;
	        FifoBuffer.InPos = next_write_index;
	    } 
}





void PrivAT_init(AT_OCB *ptr, uint16 ptrlen)
{				
        FifoBuffer.FifoBuffSize = FiFOLEN; 
        FifoBuffer.InPos = 0;     //0~FiFOLEN-1
        FifoBuffer.OutPos = 0;

	 GetArrayPtr = ptr;
        GetCmdNum = ptrlen;
}




void FIFO_Buf_Clr(void)
{
   FifoBuffer.OutPos = FifoBuffer.InPos;
}



bool AddDataToFifo(FIFOBUFF *fifoptr, uint8* pdata, uint16 datalen)
{   
	   if(pdata ==(void*)0)      return false ;
	   if(datalen==0)            return false ;
	   
	   if(Get_Fifo_Freeblock()<datalen)return false;
	   
	   while(datalen--)
	   {
	  	  Write_Fifo_Freeblock(*(pdata++));
	   }
	   return true;  
}



 

void AtAnalysis(uint8 str[],uint16 strlen)
{
    uint16 cmdlen = 0,paramlen =0,cnt =0;
    uint8   *param =NULL;
   CmdType cmdtype =ctError;
   AT_OCB * ocbptr;
	//if((*str!='A')||(*(str+1)!='T')||(strlen<2))return;

	if(*(str+strlen-1)=='?')
		{
		    if(*(str+strlen-2)=='=')
		    	{
		        cmdtype = ctTEST;
			 cmdlen = strlen-2;	   
		    	}
			else 
			{
			   cmdtype = ctQUERY;
			   cmdlen = strlen-1;
			}
		}
	else
		{
		    for(cnt =0; cnt<strlen;cnt++)
		    	{
		    	    if(*(str+cnt) == '=')
		    	    {
		    	    	   cmdtype = ctSETUP;
				   cmdlen = cnt;
				   param = str+cnt+1;
				   paramlen = strlen -cnt-1+1;	 //原基础上加一放\0
				   if(paramlen<2){           //"AT+XXX=" must with param
                                  cmdtype = ctError;
                                }
				  *(param+paramlen-1) = '\0' ;
				   cnt = strlen+6;  //flag avoid  if(cnt ==strlen) run
			           break;
		    	    	}
		    	}
			if(cnt ==strlen)
			{
				cmdtype = ctEXE;
				cmdlen = strlen;
			}
		}
	ocbptr =FindInCmdLib(str,cmdlen);	
	if(ocbptr ==NULL)return;
	if(cmdtype ==ctTEST) {
	    if(ocbptr->at_testCmd!=(void*)0){
		(*ocbptr->at_testCmd)();
	    }
	}
	else  if(cmdtype ==ctQUERY)  {
	    if(ocbptr->at_queryCmd!=(void*)0){
	       (*ocbptr->at_queryCmd)();
	    }
	}
	else  if(cmdtype ==ctSETUP)  {
	    if(ocbptr->at_setupCmd!=(void*)0){
		(*ocbptr->at_setupCmd)(param);
	    }
	}
	else  if(cmdtype ==ctEXE) {       
	   if(ocbptr->at_exeCmd!=(void*)0){
			(*ocbptr->at_exeCmd)();
	   }
	}
	else {;}
}


uint16 GetStrLen(uint8*str)
{
    uint16 cnt =0;
   while(1)
   	{
           if(*(str+cnt) =='\0')return cnt;
					 if(++cnt>1000)break;		  //最长1000个字符的串
   	}
   return 0;
   
}



AT_OCB* FindInCmdLib(uint8 str[],uint16 strlen)
{
  uint16 cnt, cntI;
  extern uint16 GetCmdNum;

cnt  = SL_Strlen(CommdStartFlag);
 if(strlen  <cnt) return NULL;
 
 for(cnt =0;cnt<GetCmdNum;cnt++)
    	{
    	if((GetArrayPtr+cnt)->at_cmdName == (void*)0) cntI = 0;
    	else cntI = SL_Strlen((GetArrayPtr+cnt)->at_cmdName);
	    if(cntI ==strlen)
	    	{
	    	    for(cntI =0;cntI<strlen;cntI++)
    	   	    	{
    	   	    	   if(*(str+cntI)!=*((GetArrayPtr+cnt)->at_cmdName+cntI))
    	   	    	   	{
    	   	    	   	  cntI = strlen +5;   // not match;
    	   	    	   	  break;
    	   	    	   	}
    	   	    	}
		      if(cntI ==strlen)    //match ok;
		      	{ 
		      	   return  GetArrayPtr+cnt;
		      	}
	    	}
 	}
      return (void*)0;
}




void RecvDataDeal( uint8* pdata, uint16 datalen)
{
   extern FIFOBUFF FifoBuffer;
   
   uint16 cnt =0;   
   uint16 fifoUsedNum = 0;  //used-fifo num;
   uint16 outpos_t          = 0;
   uint8 *findOutPtr    = NULL;
   uint8  *copyPtr         = NULL;
   uint8  *copyPtrOrgi= NULL;

   
   if(false ==AddDataToFifo(&FifoBuffer, pdata,datalen))return;

   fifoUsedNum = Get_Fifo_Usedblock()+1;       //one more space needed to set \0
   copyPtrOrgi =(uint8*)my_malloc(fifoUsedNum);
   copyPtr = copyPtrOrgi;
   outpos_t = FifoBuffer.OutPos;
   SL_Memset(copyPtr, 0, fifoUsedNum);   // 

   copyPtr[fifoUsedNum-1] = '\0';
   for(cnt=0;cnt<fifoUsedNum-1;cnt++)   //
   {
       *(copyPtr+cnt) = Read_Fifo_Usedblock(outpos_t);
       outpos_t = (uint16)(outpos_t+1)%FifoBuffer.FifoBuffSize;
   }
   
   while(1)
   {
      findOutPtr = (uint8 *)mystrstr((const char*)copyPtr,(const char*)CommdStartFlag);
      if(findOutPtr != (void*)0)
      {
        
        FifoBuffer.OutPos = (FifoBuffer.OutPos + (findOutPtr - copyPtr))
                             %FifoBuffer.FifoBuffSize;
      // copyPtr	jump to the pos find  StartFlag
	 copyPtr = findOutPtr;          
      }else{
         FIFO_Buf_Clr();break;  //fifo without StartFlag,stop search;
      }
      findOutPtr = (uint8 *)mystrstr((const char*)copyPtr,(const char*)CommdEndFlag);
      if(findOutPtr !=(void*)0)
      {
        FifoBuffer.OutPos = (FifoBuffer.OutPos + (findOutPtr - copyPtr))
                             %FifoBuffer.FifoBuffSize;         
      }else{
        break;     //can't clear data, for next use;
      }
	  
      cnt = findOutPtr - copyPtr;   //stop before CommdEndFlag
      AtAnalysis(copyPtr,cnt);      
      copyPtr = findOutPtr;
        
   	}
   my_free(copyPtrOrgi);	
}





