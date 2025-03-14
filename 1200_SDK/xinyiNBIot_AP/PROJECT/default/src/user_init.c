#include "app_init.h"
#include "at_uart.h"
#include "hal_gpio.h"


#define GNSS_PRRSTX GPIO_PAD_NUM_20     
#define GNSS_PRTRG  GPIO_PAD_NUM_21

void test(void)
{


}


/*section方式定义用户的初始化函数，最终在main入口被依次执行*/
application_init(test);


/*section方式执行用户定义的初始化函数，即application_init()定义的函数*/
void User_Startup_Init()
{
    extern uint8_t *__appRegTable_start__; //定义在sections.ld文件
    extern uint8_t *__appRegTable_end__; //定义在sections.ld文件
    appRegItem_t *appRegTable_start = (appRegItem_t *)&__appRegTable_start__;
    appRegItem_t *appRegTable_end = (appRegItem_t *)&__appRegTable_end__;

    appRegItem_t *cur = appRegTable_start;
    while (cur < appRegTable_end)
    {
        cur->app_init_entry();
        cur += 1;
    }

    // 以下是GNSS调试代码 现在全部移植到xinyiNBIot_AP\EXT\GNSS\src\gnss_drv.c中
#if 0
    // 开机进入Boot流程
    Send_AT_to_Ext("\r\n GNSS Boot init3 \r\n");
    // UART通信口配置成浮空输入
    McuGpioModeSet(GPIO_PAD_NUM_10, 0x11);
    McuGpioModeSet(GPIO_PAD_NUM_9, 0x11);
    HAL_Delay(100);

    // GNSS供电
    McuGpioModeSet(GPIO_PAD_NUM_2, 0x00);
    McuGpioWrite(GPIO_PAD_NUM_2, 1);

    HAL_Delay(100);

    // 拉低 PRTRG 推挽输出模式 输出低电平
    McuGpioModeSet(GNSS_PRTRG, 0x00);
    McuGpioWrite(GNSS_PRTRG, 0);
    // 拉低 PRRSTX 推挽输出模式 输出低电平
    McuGpioModeSet(GNSS_PRRSTX, 0x00);
    McuGpioWrite(GNSS_PRRSTX, 0);
    
    HAL_Delay(100);
    // 释放 PRRSTX 变成高阻态
    McuGpioModeSet(GNSS_PRRSTX, 0x24);
    // 等待50毫秒以上
    HAL_Delay(100);
    // 释放 PRTRG 变成高阻态
    McuGpioModeSet(GNSS_PRTRG, 0x24);

    Send_AT_to_Ext("\r\n GNSS Boot end \r\n");


    // 正常开机流程
    Send_AT_to_Ext("\r\n GNSS On \r\n");
    McuGpioModeSet(GPIO_PAD_NUM_10, 0x11);
    McuGpioModeSet(GPIO_PAD_NUM_9, 0x11);
    HAL_Delay(100);

    // TRG设置高阻态：0x24
    McuGpioModeSet(GNSS_PRTRG, 0x24);
    HAL_Delay(100);

    // STX设置推挽输出 低电平
    McuGpioModeSet(GNSS_PRRSTX, 0x00);
    McuGpioWrite(GNSS_PRRSTX, 0);
    // GNSS上电
    HAL_Delay(100);
    McuGpioModeSet(GPIO_PAD_NUM_2, 0x00);
    McuGpioWrite(GPIO_PAD_NUM_2, 1);
    HAL_Delay(100);

    // STX设置高阻态
    // McuGpioModeSet(GNSS_PRRSTX, 0x24);
    // 输出高电平
    McuGpioWrite(GNSS_PRRSTX, 1);
    Send_AT_to_Ext("\r\n GNSS On end \r\n");
#endif
}