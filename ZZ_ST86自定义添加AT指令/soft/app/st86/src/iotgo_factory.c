#include "iotgo_factory.h"

#define IOTGO_BUTTON_GPIO  (SL_GPIO_4)

#define IOTOG_DEVICEIDINFO_FILE_NAME    "deviceidinfo.json"
#define IOTGO_FACTORY_SLED_TIMER        (1)

#define IOTGO_EVENT_BASE            ZZ_EVT_BASE
#define IOTGO_EVENT_FACTORY_CMD     (IOTGO_EVENT_BASE + 1)

#define IOTGO_FACOTRY_CMD_RET_ERROR_FLASHED "\r\nFLASH FAILED: FLASHED ALREADY\r\nOK\r\n"
#define IOTGO_FACOTRY_CMD_RET_ERROR_FORMAT  "\r\nERROR FORMAT:fields lost\r\nOK\r\n"
#define IOTGO_FACOTRY_CMD_RET_OK            "\r\nOK\r\n"

#define UART1_RX_BUFFER_SIZE (1024 + 1)

typedef struct {
    uint8 buffer[UART1_RX_BUFFER_SIZE];
    uint16 head;
    uint16 tail;
} SerialRingBuffer;

static SL_TASK task_sled;
static SL_TASK task_uart;
static U8 uart = SL_UART_1;
static BOOL is_recv_json = FALSE;
static U32 json_len = 0;
static SerialRingBuffer m_rx_buffer;
static bool flag_factory_data_is_flashed = false;

/*Added by zyf*/

static BOOL is_mode_atcmd = FALSE;
void atcmd_at_exe(void);
void atcmd_start_exe(void);
void atcmd_gpiohigh_exe(void);
void atcmd_gpiolow_exe(void);
void atcmd_rssi_exe(void);
void atcmd_end_exe(void);


AT_OCB at_cmd_lib[] ={
    {"AT",                           NULL,     NULL,      NULL,     atcmd_at_exe   },
    {"AT+START",            NULL,     NULL,      NULL,     atcmd_start_exe   },
    {"AT+GPIOHIGH",   NULL,      NULL,     NULL,     atcmd_gpiohigh_exe},
    {"AT+GPIOLOW",     NULL,     NULL,      NULL,     atcmd_gpiolow_exe},
    {"AT+RSSI",                NULL,     NULL,      NULL,     atcmd_rssi_exe},
    {"AT+END",                NULL,     NULL,      NULL,     atcmd_end_exe}
};

void setFactIODirOut(void)
{
    SL_GpioSetDir(SL_GPIO_0,SL_GPIO_OUT);
    SL_GpioSetDir(SL_GPIO_1,SL_GPIO_OUT);
    SL_GpioSetDir(SL_GPIO_2,SL_GPIO_OUT);
    SL_GpioSetDir(SL_GPIO_3,SL_GPIO_OUT);
    SL_GpioSetDir(SL_GPIO_4,SL_GPIO_OUT);
    SL_GpioSetDir(SL_GPIO_5,SL_GPIO_OUT);
    SL_GpioSetDir(SL_GPIO_6,SL_GPIO_OUT);
}


void setFactIOHigh(void)
{
    SL_GpioWrite(SL_GPIO_0,SL_PIN_HIGH);
    SL_GpioWrite(SL_GPIO_1,SL_PIN_HIGH);
    SL_GpioWrite(SL_GPIO_2,SL_PIN_HIGH);
    SL_GpioWrite(SL_GPIO_3,SL_PIN_HIGH);
    SL_GpioWrite(SL_GPIO_4,SL_PIN_HIGH);
    SL_GpioWrite(SL_GPIO_5,SL_PIN_HIGH);
    SL_GpioWrite(SL_GPIO_6,SL_PIN_HIGH);
}


void setFactIOLow(void)
{
    SL_GpioWrite(SL_GPIO_0,SL_PIN_LOW);
    SL_GpioWrite(SL_GPIO_1,SL_PIN_LOW);
    SL_GpioWrite(SL_GPIO_2,SL_PIN_LOW);
    SL_GpioWrite(SL_GPIO_3,SL_PIN_LOW);
    SL_GpioWrite(SL_GPIO_4,SL_PIN_LOW);
    SL_GpioWrite(SL_GPIO_5,SL_PIN_LOW);
    SL_GpioWrite(SL_GPIO_6,SL_PIN_LOW);
}



void atcmd_at_exe (void)
{
    SL_UartSendData(uart, "OK\r\n", 4);
}

void atcmd_start_exe(void)
{
    is_mode_atcmd = TRUE;
    SL_StopTimer(task_sled,IOTGO_FACTORY_SLED_TIMER);
    setFactIODirOut();
    setFactIOHigh();
    SL_UartSendData(uart, "OK\r\n", 4);
}

void atcmd_gpiohigh_exe(void)
{   
    if(!is_mode_atcmd)
   {
   	SL_UartSendData(uart, "ERROR\r\n", 7);
   }
   else
   {
      setFactIOHigh();
      SL_UartSendData(uart, "OK\r\n", 4);
   }
}


void atcmd_gpiolow_exe(void)
{
    if(!is_mode_atcmd)
    {
   	 SL_UartSendData(uart, "ERROR\r\n", 7);
    }
   else
   {
       setFactIOLow();
       SL_UartSendData(uart, "OK\r\n", 4);
   }
}

void atcmd_rssi_exe(void)
{
    int8 rssi = 0;
    int8 *str = NULL;
    uint16 strlen = 0;
    if(!is_mode_atcmd)
   {
   	SL_UartSendData(uart, "ERROR\r\n", 7);
	return;
   }
    str = SL_GetMemory(10);
    rssi =zzNetGetRssi();
    SL_itoa(rssi, str, 10);  
    strlen = (uint16)SL_Strlen((const char*)str);
    SL_UartSendData(uart, "RSSI:", 5);
    SL_UartSendData(uart, str, strlen);
    SL_UartSendData(uart, "\r\n", 2);
    SL_UartSendData(uart, "OK\r\n", 4);
    SL_FreeMemory(str);
}

void atcmd_end_exe(void)
{
     if(!is_mode_atcmd)
     {
   	  SL_UartSendData(uart, "ERROR\r\n", 7);
	  return;
     }
     is_mode_atcmd =false;
     SL_StartTimer(task_sled, IOTGO_FACTORY_SLED_TIMER, SL_TIMER_MODE_PERIODIC, 
     SL_MilliSecondToTicks(100));  //more times used,more fast the frq 
     SL_UartSendData(uart, "OK\r\n", 4);
}


void atcmd_error_respon (void)
{
    SL_UartSendData(uart, "ERROR\r\n", 7);
}


/*End zyf*/






/* Added by wpf */
static uint16 uart1_rx_available(void)
{
    return (UART1_RX_BUFFER_SIZE + m_rx_buffer.head - m_rx_buffer.tail) % UART1_RX_BUFFER_SIZE;
}
/* Added by wpf */
static void uart1_rx_flush(void)
{
    //os_bzero(&m_rx_buffer, sizeof(m_rx_buffer));
    m_rx_buffer.head = m_rx_buffer.tail;
}
/* Added by wpf */
static uint8 uart1_rx_read(void)
{
    // if the head isn't ahead of the tail, we don't have any characters
    if (m_rx_buffer.head == m_rx_buffer.tail) {
        return 0xFF;
    } else {
        uint8 c = m_rx_buffer.buffer[m_rx_buffer.tail];
        m_rx_buffer.tail = (uint16)(m_rx_buffer.tail + 1) % UART1_RX_BUFFER_SIZE;
        return c;
    }
}
/* Added by wpf */
static void uart1_rx_write(uint8 data)
{
    uint16 next_write_index = (uint16)(m_rx_buffer.head + 1) % UART1_RX_BUFFER_SIZE;
    // if we should be storing the received character into the location
    // just before the tail (meaning that the head would advance to the
    // current location of the tail), we're about to overflow the buffer
    // and so we don't write the character or advance the head.
    if (next_write_index != m_rx_buffer.tail) {
        m_rx_buffer.buffer[m_rx_buffer.head] = data;
        m_rx_buffer.head = next_write_index;
    }
}

static void iotgoEmitEvent(SL_TASK stTask, U32 ulMsgId)
{
    SL_EVENT stEvnet;
    SL_Memset(&stEvnet, 0, sizeof(SL_EVENT));
    stEvnet.nEventId = ulMsgId;
    SL_SendEvents(stTask, &stEvnet);
}

#if 0 // for test
static void iotgoTestForFs(void)
{
    U32 freeSpaceByte = 0;
    U32 fullSpaceByte = 0;
    S32 file_open_ret = 0;
    
    if (SL_RET_OK == SL_FileSysGetSpaceInfo(SL_FS_DEV_TYPE_FLASH, &freeSpaceByte, &fullSpaceByte))
    {
        SL_ApiPrint("fs info: %uKB / %uKB", freeSpaceByte/1024, fullSpaceByte/1024);
    }
    else
    {
        SL_ApiPrint("read space info failed!");
    }

    SL_ApiPrint("fs free: %d/KB", SL_FileGetFreeSize()/1024);

    if (SL_FS_NO_MORE_FILES == SL_FileCheck(IOTOG_DEVICEIDINFO_FILE_NAME))
    {
        SL_ApiPrint("no more file");
    }

    file_open_ret = SL_FileOpen(IOTOG_DEVICEIDINFO_FILE_NAME, SL_FS_RDONLY);
    if (file_open_ret < 0)
    {
        SL_ApiPrint("open file failed! ret = %d", file_open_ret);
    }
    else
    {
        SL_ApiPrint("open file ok! ret = %d", file_open_ret);
    }

    
}
#endif

void iotgoFactoryDeleteDeviceIdInfoFile(void)
{
    S32 ret = 0;
    ret = SL_FileCheck(IOTOG_DEVICEIDINFO_FILE_NAME);
    if (SL_RET_OK == ret)
    {
        SL_FileDelete(IOTOG_DEVICEIDINFO_FILE_NAME);
        SL_ApiPrint("file exists already and delete it!");
    }
    else
    {
        SL_ApiPrint("file dosen't exist");
    }
}

static void iotgoFactorySaveDeviceIdInfoToFile(U8 *content)
{
    S32 ret = 0;
    S32 file = 0;
    U32 len = 0;
    
    iotgoFactoryDeleteDeviceIdInfoFile();
    
    ret = SL_FileCreate(IOTOG_DEVICEIDINFO_FILE_NAME);
    if (ret < 0)
    {
        SL_ApiPrint("SL_FileCreate failed ret = %d", ret);
        return;
    }
    file = SL_FileOpen(IOTOG_DEVICEIDINFO_FILE_NAME, SL_FS_RDWR);
    if (file < 0)
    {
        SL_ApiPrint("SL_FileOpen failed ret = %d", file);
    }

    len = SL_Strlen(content);
    ret = SL_FileWrite(file, content, len);
    if (ret == len)
    {
        SL_ApiPrint("SL_FileWrite ok");
    }
    else
    {
        SL_ApiPrint("SL_FileWrite err!");
    }
    
    SL_FileClose(file);
}

#if 1 // for test
void iotgoFactoryGenerateDeviceIdInfoFile(void)
{
    S32 ret = 0;
    S32 file = 0;
    U8 *content = 
        "{"
            "\"deviceid\":\"100007eea0\","
            "\"factory_apikey\":\"5f385a0a-417f-49d6-bed6-af67dd1771f9\","
            "\"sta_mac\":\"d0:27:00:0f:da:90\","
            "\"sap_mac\":\"d0:27:00:0f:da:91\","
            "\"device_model\":\"GSA-D01-GL\""
        "}"
    ;

    U32 len = 0;
    
    iotgoFactoryDeleteDeviceIdInfoFile();
    
    ret = SL_FileCreate(IOTOG_DEVICEIDINFO_FILE_NAME);
    if (ret < 0)
    {
        SL_ApiPrint("SL_FileCreate failed ret = %d", ret);
        return;
    }
    file = SL_FileOpen(IOTOG_DEVICEIDINFO_FILE_NAME, SL_FS_RDWR);
    if (file < 0)
    {
        SL_ApiPrint("SL_FileOpen failed ret = %d", file);
    }

    len = SL_Strlen(content);
    ret = SL_FileWrite(file, content, len);
    if (ret == len)
    {
        SL_ApiPrint("SL_FileWrite ok");
    }
    else
    {
        SL_ApiPrint("SL_FileWrite err!");
    }
    
    SL_FileClose(file);

}
#endif

BOOL iotgoFactoryVerifyDeviceIdInfo(void)
{
    S32 ret = 0;
    S32 file = 0;
    S32 size = 0;
    U8 *buffer = NULL;
    cJSON *cjson_root = NULL;
    U8 *deviceid = NULL;
    U8 *factory_apikey = NULL;
    U8 *device_model = NULL;
    
    ret = SL_FileCheck(IOTOG_DEVICEIDINFO_FILE_NAME);
    if (SL_RET_OK == ret)
    {
        file = SL_FileOpen(IOTOG_DEVICEIDINFO_FILE_NAME, SL_FS_RDONLY);
        if (file < 0)
        {
            SL_ApiPrint("SL_FileOpen failed ret = %d", file);
            return FALSE;
        }
        
        size = SL_FileGetSize(file);
        buffer = (U8 *)SL_GetMemory(size + 1);
        SL_Memset(buffer, 0, size + 1);
        ret = SL_FileRead(file, buffer, size);
        if (ret == size)
        {
            //SL_ApiPrint("SL_FileRead ok and content is [%s]", buffer);
            cjson_root = cJSON_Parse((char *)buffer);
            
            deviceid = cJSON_GetObjectItem(cjson_root, "deviceid")->valuestring;
            factory_apikey = cJSON_GetObjectItem(cjson_root, "factory_apikey")->valuestring;
            device_model = cJSON_GetObjectItem(cjson_root, "device_model")->valuestring;
            
            zzLogP("device_model:[%s]", device_model);
            zzLogP("deviceid:[%s]", deviceid);
            zzLogP("factory_apikey:[%s]", factory_apikey);
            
            zzSetDeviceid(deviceid);
            zzSetApikey(factory_apikey);
            zzSetModel(device_model);

            
            cJSON_Delete(cjson_root);
            SL_FileClose(file);
            SL_FreeMemory(buffer);
            return TRUE;
        }
        else
        {
            SL_ApiPrint("SL_FileRead err");
        }
        
        SL_FileClose(file);
        SL_FreeMemory(buffer);
    }

    return FALSE;
}

static void turnOnSled(void)
{
    SL_GpioWrite(SL_GPIO_0,SL_PIN_HIGH);
}

static void turnOffSled(void)
{
    SL_GpioWrite(SL_GPIO_0,SL_PIN_LOW);
}


static void taskSled (void *pdata)
{
    SL_EVENT ev;
    U8 cnt = 0;
    
    while (1)
    {
        SL_Memset(&ev, 0, sizeof(ev));
        SL_GetEvent(task_sled, &ev);
        switch (ev.nEventId)
        {
            case SL_EV_TIMER:
                cnt++;
                if (20 == cnt)
                {
                    cnt = 0;
                }
                
                if (0 == cnt
                    || 2 == cnt
                    || 4 == cnt
                    || 6 == cnt
                    || 8 == cnt)
                {
                    turnOnSled();
                }
                else
                {
                    turnOffSled();
                }
                
                break;
            default:
                SL_ApiPrint("taskSled: unknown event %u", ev.nEventId);
                break;
        }
    }
}

static void initSledForFactoryMode(void)
{
    SL_GpioSetDir(SL_GPIO_0,SL_GPIO_OUT);
    turnOffSled();
    task_sled.element[0] = SL_CreateTask(taskSled, 2048, SL_APP_TASK_PRIORITY_BEGIN + 5, "taskSled");
    SL_StartTimer(task_sled, IOTGO_FACTORY_SLED_TIMER, SL_TIMER_MODE_PERIODIC, 
        SL_MilliSecondToTicks(100));
}

static void procATSEND(void)
{
    U32 i;
    U32 buf_size = uart1_rx_available() + 1;
    U8 *at_send = NULL;
    U8 *str_len = NULL;
    U32 len = 0;
    U8 *end = NULL;
    U8 *buf = (U8 *)SL_GetMemory(buf_size);
    SL_Memset(buf, 0, buf_size);

    for (i = 0; i < buf_size - 1; i++)
    {
        buf[i] = uart1_rx_read();
    }
    
    at_send = SL_Strstr(buf, "AT+SEND=");
    end = SL_Strstr(buf, "\r\n");
    if (at_send && (at_send < end))
    {
        end[0] = 0;
        str_len = at_send + SL_Strlen("AT+SEND=");
        len = SL_atoi(str_len);
        if (len > 0 && len <= 512)
        {
            json_len = len;
            is_recv_json = TRUE;
            SL_ApiPrint("len = %d", len);
            SL_UartSendData(uart, ">\r\n", 3);
        }
    }
    uart1_rx_flush();
    
    SL_FreeMemory((PVOID)buf);
}

static void recvJSON(void)
{   
    U32 i = 0;
    U8 *buf = NULL;
    cJSON *cjson_root = NULL;
    U8 *deviceid = NULL;
    U8 *factory_apikey = NULL;
    U8 *sta_mac = NULL;
    U8 *sap_mac = NULL;
    U8 *device_model = NULL;
    sha256_context sha256_ctx;
    uint8 digest[32];
    uint8 digest_hex[65];

    SL_ApiPrint("json_len = %u", json_len);
    
    if (json_len > uart1_rx_available())
    {
        return;
    }


    buf = (U8 *)SL_GetMemory(json_len + 1);
    SL_Memset(buf, 0, json_len + 1);
    
    for (i = 0; i < json_len; i++)
    {
        buf[i] = uart1_rx_read();
    }
    //SL_ApiPrint("buf = [%s]", buf);

    if (flag_factory_data_is_flashed)
    {
        SL_UartSendData(uart, 
            IOTGO_FACOTRY_CMD_RET_ERROR_FLASHED, 
            SL_Strlen(IOTGO_FACOTRY_CMD_RET_ERROR_FLASHED));
        zzLogI("flashed data already!");
        goto RetExit;
    }
    
    cjson_root = cJSON_Parse((char *)buf);
    if (cjson_root)
    {
        deviceid = cJSON_GetObjectItem(cjson_root, "deviceid")->valuestring;
        factory_apikey = cJSON_GetObjectItem(cjson_root, "factory_apikey")->valuestring;
        sta_mac = cJSON_GetObjectItem(cjson_root, "sta_mac")->valuestring;
        sap_mac = cJSON_GetObjectItem(cjson_root, "sap_mac")->valuestring;
        device_model = cJSON_GetObjectItem(cjson_root, "device_model")->valuestring;
        if (deviceid && 10 == SL_Strlen(deviceid)
            && factory_apikey && 36 == SL_Strlen(factory_apikey)
            && sta_mac && 17 == SL_Strlen(sta_mac) 
            && sap_mac && 17 == SL_Strlen(sap_mac) 
            && device_model && 10 == SL_Strlen(device_model)
            )
        {
            
            iotgoFactorySaveDeviceIdInfoToFile(buf);

            zzDeviceDisable(); /* New device born with disabled */
            
            //SL_ApiPrint("deviceid = [%s]", deviceid);
            //SL_ApiPrint("factory_apikey = [%s]", factory_apikey);
            //SL_ApiPrint("sta_mac = [%s]", sta_mac);
            //SL_ApiPrint("sap_mac = [%s]", sap_mac);
            //SL_ApiPrint("device_model = [%s]", device_model);

            SL_Memset(digest, 0, sizeof(digest));
            SL_Memset(digest_hex, 0, sizeof(digest_hex));
            
            sha256_starts(&sha256_ctx);
            sha256_update(&sha256_ctx, deviceid, SL_Strlen(deviceid));
            sha256_update(&sha256_ctx, factory_apikey, SL_Strlen(factory_apikey));
            sha256_update(&sha256_ctx, sta_mac, SL_Strlen(sta_mac));
            sha256_update(&sha256_ctx, sap_mac, SL_Strlen(sap_mac));
            sha256_update(&sha256_ctx, device_model, SL_Strlen(device_model));
            sha256_finish(&sha256_ctx, digest);
            for (i = 0; i < 64; i += 2)
            {
                SL_Sprintf(&digest_hex[i], "%02x", digest[i/2]);
            }
            digest_hex[64] = '\0';

            for (i = 0; i < 3; i++)
            {
                SL_UartSendData(uart, digest_hex, SL_Strlen(digest_hex));
                SL_UartSendData(uart, IOTGO_FACOTRY_CMD_RET_OK, SL_Strlen(IOTGO_FACOTRY_CMD_RET_OK));
            }

            flag_factory_data_is_flashed = true;

            SL_ApiPrint("flash data ok");
        }
        else
        {
            SL_UartSendData(uart, IOTGO_FACOTRY_CMD_RET_ERROR_FORMAT, 
                SL_Strlen(IOTGO_FACOTRY_CMD_RET_ERROR_FORMAT));
                
            SL_ApiPrint("bad flash data format");
        }

        cJSON_Delete(cjson_root);
    }
    else
    {
        SL_UartSendData(uart, IOTGO_FACOTRY_CMD_RET_ERROR_FORMAT, 
            SL_Strlen(IOTGO_FACOTRY_CMD_RET_ERROR_FORMAT));
        
        SL_ApiPrint("format err(not json)");
    }

RetExit:
    SL_FreeMemory((PVOID)buf);
    uart1_rx_flush();
    json_len = 0;
    is_recv_json = FALSE;
}

static void taskUart(void *pdata)
{    
    SL_EVENT ev;    
    U32 i = 0;
    U8 *uart_data = NULL;
    U32 uart_data_len = 0;
    
    json_len = 0;
    is_recv_json = FALSE;
    uart1_rx_flush();

    
    SL_ApiPrint("uart config begin");

    SL_UartSetBaudRate(uart, SL_UART_BAUD_RATE_19200);
    SL_UartSetDCBConfig(uart, SL_UART_8_DATA_BITS, SL_UART_1_STOP_BIT, SL_UART_NO_PARITY);
    SL_UartSetFlowCtrl(0, 0);
    SL_UartSetAppTaskHandle(uart, task_uart.element[0]);
    SL_UartClrRxBuffer(uart);
    SL_UartClrTxBuffer(uart);
    
    SL_ApiPrint("uart config end");
    
    if (SL_UartOpen(uart))
    {
        SL_ApiPrint("uart open ok");
    }


    while (1)
    {
        SL_Memset(&ev, 0, sizeof(ev));
        SL_GetEvent(task_uart, &ev);
        switch (ev.nEventId)
        {
            case SL_EV_UART_RECEIVE_DATA_IND:
                SL_ApiPrint("SL_EV_UART_RECEIVE_DATA_IND");
                uart_data_len = *((U32 *)ev.nParam1);
                uart_data = (U8 *)ev.nParam1 + 4;
                for (i = 0; i < uart_data_len; i++)
                {
                    uart1_rx_write(uart_data[i]);
                }

                RecvDataDeal(uart_data, (u16)uart_data_len);
                
                SL_FreeMemory((PVOID)ev.nParam1);
                iotgoEmitEvent(task_uart, IOTGO_EVENT_FACTORY_CMD);
                break;
            case IOTGO_EVENT_FACTORY_CMD:
                SL_ApiPrint("IOTGO_EVENT_FACTORY_CMD");
                if (!is_recv_json)
                {
                    procATSEND();
                }
                else
                {
                    recvJSON();
                }
                break;
            default:
                SL_ApiPrint("taskUart: unknown event %u", ev.nEventId);
                break;
        }
    }
}

static void initUart(void)
{
    task_uart.element[0] = SL_CreateTask(taskUart, 2048, SL_APP_TASK_PRIORITY_BEGIN + 2, "taskUart");
}

/**
 * @note:
 *   - Recive JSON from UART2
 *   - Save data to file
 *   - Return digest sha256 of values
 */
void iotgoFactoryMainLoop(void)
{
    SL_EVENT ev;
    SL_TASK main_task;
    main_task.element[0] = SL_GetAppTaskHandle();
    SL_ApiPrint("---------iotgoFactoryMainLoop---------");

    iotgoEmitEvent(main_task, ZZ_EVT_FTRY_TASK_START);

    while (1)
    {
        SL_Memset(&ev, 0, sizeof(ev));
        SL_GetEvent(main_task, &ev);
        switch (ev.nEventId)
        {
            case ZZ_EVT_FTRY_TASK_START:
                SL_ApiPrint("ZZ_EVT_FTRY_TASK_START");
                initSledForFactoryMode();
                initUart();
                PrivAT_init(at_cmd_lib, sizeof(at_cmd_lib)/sizeof(AT_OCB));  /////added by zyf
                break;
            default:
                SL_ApiPrint("main loop: unknown event %u", ev.nEventId);
                break;
        }
    }
}

/**
 * Press the key for factory mode
 */
BOOL iotgoIsBootForFactory(void)
{
    SL_GpioSetDir(IOTGO_BUTTON_GPIO, SL_GPIO_IN);
    if (SL_PIN_LOW == SL_GpioRead(IOTGO_BUTTON_GPIO))
    {
        SL_ApiPrint("boot for factory mode");
        return TRUE;
    }
    else
    {
        SL_ApiPrint("boot for work mode");
        return FALSE;
    }
}

