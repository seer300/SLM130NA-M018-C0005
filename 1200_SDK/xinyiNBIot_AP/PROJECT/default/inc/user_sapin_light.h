#pragma once


#include "mcu_adapt.h"


/******************************************************************************
                                定义GPIO
******************************************************************************/

// 电源控制、检测
#define POWER_KEY_PIN                            MCU_GPIO14
#define POWER_HOLD_PIN                           MCU_GPIO7
#define LIGHT_CTRL_PIN                           MCU_GPIO24

// ------------------------------
// EVB
// #define POWER_KEY_PIN                            MCU_GPIO21
// #define POWER_HOLD_PIN                           MCU_GPIO14
// #define LIGHT_CTRL_PIN                           MCU_GPIO5


/******************************************************************************
                                 外部功能函数
******************************************************************************/

extern void SapinLight_Start(int t);
extern int SapinLight_Type_Get(void);
extern void SapinLight_Init(void);
