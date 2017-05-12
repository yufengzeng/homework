#include "iotgo_sled.h"


#if 0
#define IOTGO_SLED_GPIO         SL_GPIO_12
#else
#define IOTGO_SLED_GPIO         SL_GPIO_0
#endif
#define IOTGO_SLED_ON_LEVEL     SL_PIN_HIGH
#define IOTGO_SLED_OFF_LEVEL    SL_PIN_LOW

static uint8 sled_indicator = IOTGO_SLED_OFF_LEVEL;

static void  sledOn(void)
{
    SL_GpioWrite(IOTGO_SLED_GPIO, IOTGO_SLED_ON_LEVEL);
    sled_indicator = IOTGO_SLED_ON_LEVEL;
}

static void  sledOff(void)
{
    SL_GpioWrite(IOTGO_SLED_GPIO, IOTGO_SLED_OFF_LEVEL);
    sled_indicator = IOTGO_SLED_OFF_LEVEL;
}

static void sledToggle(void)
{
    if (IOTGO_SLED_OFF_LEVEL == sled_indicator)
    {
        sledOn();
    }
    else
    {
        sledOff();
    }
}

static void initSled(void)
{
    SL_GpioSetDir(IOTGO_SLED_GPIO, SL_GPIO_OUT);
    sledOff();
}

/*
TIME :  1  2  3  4  5  6  7  8  9  10  11  12  13  14  15  16  17  18  19  20
AP XX:  111111100000000000000000000000000000000000000000000000000000000000000    
AP OK:  111111100000011111111100000000000000000000000000000000000000000000000
INIT :  111111111111111111111111111111100000000000000000000000000000000000000
OK   :  111111111111111111111111111111111111111111111111111111111111111111111
SET  :  111111111111111100000000000000111111111111111111100000000000000000000
*/

void  iotgoSledScanCb(void)
{
    static uint32 status_100ms_counter = 0;   
    static ZZDeviceState old_mode = ZZ_DEVICE_STATE_INVALID;
    static ZZDeviceState mode = ZZ_DEVICE_STATE_INVALID;

    status_100ms_counter++;
    mode = iotgoGetDeviceState();
    if (old_mode != mode)
    {
        old_mode = mode;
        status_100ms_counter = 0;
    }

    if (status_100ms_counter >= 20)
    {
        status_100ms_counter = 0;
    }
    
    if (ZZ_DEVICE_STATE_LOGINED == mode)
    {
        if (IOTGO_SLED_ON_LEVEL != sled_indicator)        
        {
            sledOn();
        }
    }
    else if (ZZ_DEVICE_STATE_LOGIN == mode)
    {
        if ((status_100ms_counter % 10) == 0)
        {
            if (IOTGO_SLED_OFF_LEVEL == sled_indicator)
            {
                sledOn();
            }
            else
            {
                sledOff();
            }
        }
    }
    else if (ZZ_DEVICE_STATE_CONN == mode) 
    {
        if (1 == status_100ms_counter)
        {
            sledOn();
        }
        else if (2 == status_100ms_counter)
        {
            sledOff();
        }
        else if (3 == status_100ms_counter)
        {
            sledOn();
        }
        else
        {
            sledOff();
        }
    }
    else if (ZZ_DEVICE_STATE_GPRS == mode)
    {
        if (1 == status_100ms_counter)
        {
            sledOn();
        }
        else
        {
            sledOff();
        }
    }
    else if (ZZ_DEVICE_STATE_START == mode)
    {
        sledOff();
    }
    else if (ZZ_DEVICE_STATE_DISABLED == mode)
    {
        sledToggle();
    }
#if 0    
    else if (ZZ_DEVICE_STATE_UPGRADE == mode)
    {
        if (1 == status_100ms_counter)
        {
            sledOn();
        }
        else if (2 == status_100ms_counter)
        {
            sledOff();
        }
        else if (3 == status_100ms_counter)
        {
            sledOn();
        }
        else if (4 == status_100ms_counter)
        {
            sledOff();
        }
        else if (5 == status_100ms_counter)
        {
            sledOn();
        }
        else
        {
            sledOff();
        } 
    }
#endif    
    else
    {
        sledOff();
    }
}

void  iotgoSledInit(void)
{
    initSled();
}

