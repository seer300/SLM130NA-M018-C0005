#include "app_init.h"
#include "at_uart.h"
#include "hal_gpio.h"
#include "hal_timer.h"


#define GNSS_PRRSTX GPIO_PAD_NUM_20     
#define GNSS_PRTRG  GPIO_PAD_NUM_21

void test(void)
{


}


/*section方式定义用户的初始化函数，最终在main入口被依次执行*/
application_init(test);

/**
 * @brief 	TIMER 单PWM模式初始化函数
 * 			这个函数描述了timer初始化为双PWM模式需要的相关步骤.
 * 			在初始化函数内部需要设置单路PWM的输出引脚与GPIO引脚的对应关系、引脚复用方式
 * 			以及定时器的编号、工作模式、重载值、时钟分频、PWM比较值、时钟极性等
 * 			然后打开定时器.
 */
void GPIO6_PWM_Init(void)
{
	HAL_GPIO_InitTypeDef gpio_init = {0};
	gpio_init.Pin = GPIO_PAD_NUM_6;
	gpio_init.Mode = GPIO_MODE_HW_PER;
	gpio_init.PinRemap = GPIO_TMR2PWM_OUTP;
	HAL_GPIO_Init(&gpio_init);

	HAL_TIM_HandleTypeDef TimPWMSingleHandle = {0};
	//用户仅限使用HAL_TIM1和HAL_TIM2
	TimPWMSingleHandle.Instance = HAL_TIM2;
	//单输出脉冲宽度（PWM）调制模式
	TimPWMSingleHandle.Init.Mode = HAL_TIM_MODE_PWM_SINGLE;
	//设置时钟源分频
	TimPWMSingleHandle.Init.ClockDivision = HAL_TIM_CLK_DIV_128;
	//重载值，具体含义请参看本文件的@file相关注释
	TimPWMSingleHandle.Init.Reload = 15000;
	//PWM翻转比较值，具体含义请参看本文件的@file相关注释
	TimPWMSingleHandle.Init.PWMCmp = 5000;
	//Polarity_Low表示定时器关闭时输出为低电平，开启定时器Count寄存器计数值达到PWM比较值后翻转为高电平
	TimPWMSingleHandle.Init.Polarity = HAL_TIM_Polarity_Low;

	HAL_TIM_Init(&TimPWMSingleHandle);
	HAL_TIM_Start(&TimPWMSingleHandle);

    // Send_AT_to_Ext("\r\nGPIO6 PWM init\r\n");
}


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

    // GPIO6 输出PWM波形 硬件设计要求
    GPIO6_PWM_Init();

}