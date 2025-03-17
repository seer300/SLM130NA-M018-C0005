#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "uart.h"
#include "at_uart.h"
#include "at_hardware_cmd.h"
#include "mcu_adapt.h"
#include "at_uart.h"
#include "auto_baudrate.h"

#include "gpio.h"
#include "hw_gpio.h"
#include "mcu_adapt.h"
#include "xy_event.h"
#include "xy_at_api.h"


#include "user_sapin_light.h"

/*
本文件为 西班牙-警示灯 项目使用
控制灯时序

         80ms             80ms            80ms
    +----------+     +----------+     +----------+
    |          |     |          |     |          |
    |          |     |          |     |          |
    +          +-----+          +-----+          +------------------------------
                 30ms             30ms                       940ms

    SLM130_EVB_BOARD_V2.0 + SLM130-NA 调试GPIO分配
    +--------+------------+
    | GPIOx  |   板载功能  |
    +--------+------------+
    | GPIO5  | 蓝灯(Pin16) |
    +--------+------------+
    | GPIO14 | 黄灯(Pin20) |
    +--------+------------+
    | GPIO21 | 悬空(Pin31) |
    +--------+------------+
    | GPIO25 | SSN0(Pin6) |
    +--------+------------+
*/


#define SLIGHT_TIMER_ID                          TIMER_LP_USER5

// 灯控制状态变量定义
static int sLightCtrlType = 0;
static int sLightTimeCount = 0, sLightState = 0, sLightHighCount = 0;

// 灯控制回调函数
__RAM_FUNC void SapinLight_TimeEventCallBack(void)
{
    sLightTimeCount++;
    switch (sLightState)
    {
    case 0:
        // 1. 80ms --- high
        if (sLightTimeCount > 7)
        {
            sLightTimeCount = 0;

            McuGpioWrite(LIGHT_CTRL_PIN, 0);  // to low
            sLightState = 1;
            sLightHighCount++;
        }
        break;

    case 1:
        // 2. 30ms --- low
        if (sLightTimeCount > 2)
        {
            sLightTimeCount = 0;

            if (sLightHighCount >= 3)
            {
                McuGpioWrite(LIGHT_CTRL_PIN, 0);  // to low
                sLightState = 2;
                sLightHighCount = 0;
            }
            else
            {
                McuGpioWrite(LIGHT_CTRL_PIN, 1);  // to high
                sLightState = 0;
            }
        }
        break;

    case 2:
        // 3. 940ms --- low
        if (sLightTimeCount > 90)
        {
            sLightTimeCount = 0;

            McuGpioWrite(LIGHT_CTRL_PIN, 1);  // to high
            sLightState = 0;
        }
        break;
    
    default:
        // 从头来
        sLightTimeCount = 0;
        sLightState = 0;
        sLightHighCount = 0;
        break;
    }
}

void SapinLight_Start(int t)
{
    sLightCtrlType = t;

    // 启动一个定时器进行控制
    if (sLightCtrlType != 0)
    {
        if (sLightCtrlType < 2)
        {
            McuGpioWrite(LIGHT_CTRL_PIN, 1);  // to high

            // 10ms 基准进行计数
            DisableInterrupt();  // 防止设置瞬间Time超时改变g_Rsp_Timeout的值
            sLightTimeCount = 0;
            sLightState = 0;
            sLightHighCount = 0;
            Timer_AddEvent(SLIGHT_TIMER_ID, 10, SapinLight_TimeEventCallBack, 1);
            EnableInterrupt();
        }
        else
        {
            McuGpioWrite(LIGHT_CTRL_PIN, sLightCtrlType == 2 ? 1 : 0);
        }
    }
    else
    {
        // 停止定时器
        Timer_DeleteEvent(SLIGHT_TIMER_ID);

        sLightTimeCount = 0;
        sLightState = 0;

        McuGpioWrite(LIGHT_CTRL_PIN, 0);  // to low
    }
}

int SapinLight_Type_Get(void)
{
    return sLightCtrlType;
}

void SapinLight_Init(void)
{
    sLightCtrlType = 0;

    // 初始化管脚
    McuGpioModeSet(LIGHT_CTRL_PIN, 0x00);

    McuGpioWrite(LIGHT_CTRL_PIN, 0);  // 默认关灯

    // 上电后，直接启动闪灯
    SapinLight_Start(1);
}