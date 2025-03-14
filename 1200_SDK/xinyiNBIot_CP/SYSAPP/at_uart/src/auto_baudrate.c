#include "cmsis_device.h"
#include "gpio.h"
#include "oss_nv.h"
#include "prcm.h"

// SYSAPP去除-w后编译不过，等待驱动适配好代码后删除
#include "tmp_aon_func.h"

/* ==== AT口自动波特率检测状态 ==== */
typedef enum at_autobaud_state
{
    AUTOBAUD_STATE_INIT,
    AUTOBAUD_STATE_CAPTURE_FIRST,
    AUTOBAUD_STATE_CAPTURE_SECOND,
    AUTOBAUD_STATE_CAPTURE_THIRD,
    AUTOBAUD_STATE_VERIFY,
    AUTOBAUD_STATE_DONE,
} at_autobaud_state_t;


#define TIME_DURATION          0.02 // 单位:秒
#define SYSTICK_LOAD_RELOAD    (uint32_t)(TIME_DURATION*(BBPLL_CLK_DEFAULT*2/4))

/**
 * @brief AT口自动波特率检测专用--超时捕获上升沿
 * @param rxd_pin_num: at_rxd引脚号
 * @return SysTick重装载次数
 * @note 调用前请确保SysTick已经初始化
 */
static uint32_t Capture_RisingEdge_withTimeout(uint8_t rxd_pin_num)
{
    volatile uint32_t systick_reload_flag = 0;
    volatile uint32_t gpio_status = 0;
    volatile uint8_t i = 0;

    //超时等待上升沿到来
    systick_reload_flag = SysTick->CTRL;
    systick_reload_flag = 0;
    do{
        gpio_status = GPIO_ReadPin(rxd_pin_num);
        if(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)
        {
            systick_reload_flag++;
        }
    }while(gpio_status == 0 && (systick_reload_flag < 2));

    for(i=0; i<10; i++);//消抖

    return systick_reload_flag;
}


/**
 * @brief AT口自动波特率检测专用--超时捕获下降沿并记录tick值
 * @param rxd_pin_num: at_rxd引脚号
 * @param autobaud_tick：指向记录tick的变量
 * @return SysTick重装载次数
 * @note 调用前请确保SysTick已经初始化
 */
static uint32_t Capture_FallingEdge_withTimeout(uint8_t rxd_pin_num,uint32_t *autobaud_tick)
{
    volatile uint32_t systick_reload_flag = 0;
    volatile uint32_t gpio_status = 0;
    volatile uint8_t i = 0;

    //超时等待下降沿到来
    systick_reload_flag = SysTick->CTRL;
    systick_reload_flag = 0;
    do{
        gpio_status = GPIO_ReadPin(rxd_pin_num);
        *autobaud_tick = SysTick->VAL;
        if(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)
        {
            systick_reload_flag++;
        }
    }while(gpio_status != 0 && (systick_reload_flag < 2));

    for(i=0; i<10; i++);//消抖

    return systick_reload_flag;
}


/**
 * @brief AT口自动波特率检测专用--计数下降沿个数并等待RXD变IDLE
 * @param rxd_pin_num: at_rxd引脚号
 * @return 1: 后续RXD下降沿计数个数大于等于9,且RXD处于IDLE, 0: 后续RXD下降沿计数个数小于9,验证失败
 * @note AT\r\n后面是10个下降沿，at\r\n后面是9个下降沿
 * @note 调用前请确保SysTick已经初始化
 */
static uint8_t Count_FallingEdge_Wait_Rxd_Idle(uint8_t rxd_pin_num)
{
    volatile uint32_t systick_reload_flag = 0;
    volatile uint32_t gpio_status_last = 0;
    volatile uint32_t gpio_status_current = 0;
    volatile uint32_t falling_edge_cnt = 0;

    systick_reload_flag = SysTick->CTRL;
    systick_reload_flag = 0;

    gpio_status_last = GPIO_ReadPin(rxd_pin_num);

    do{
        gpio_status_current = GPIO_ReadPin(rxd_pin_num);
        if(gpio_status_last >0 && gpio_status_current ==0)
        {
            falling_edge_cnt++;
            systick_reload_flag = 0;
        }

        if(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)
        {
            systick_reload_flag++;
        }

        gpio_status_last = gpio_status_current;

        for(volatile uint8_t i=0; i<10; i++);//消抖
    }while(/*falling_edge_cnt < 10 && */systick_reload_flag < 2);//falling_edge_cnt < 10 屏蔽就只支持大写AT

    //后续RXD下降沿计数个数大于等于9则等待rxd变为IDLE，否则表明验证失败
    if(falling_edge_cnt >= 9) {
        while( GPIO_ReadPin(rxd_pin_num) == 0);
        return 1;
    }
    else {
        return 0;
    }
}


/**
 * @brief 自动波特率检测，并重写NV高7位为波特率结果，低9位为0。
 * @note 支持AT\r\n和at\r\n检测
 * @note AT\r\n --> A=0x41,T=0x54,\r=0x0d,\n=0x0a
 * @note at\r\n --> A=0x61,T=0x74,\r=0x0d,\n=0x0a
 * @return auto_baudrate:检测出波特率的倍数，单位2400bps
 */
uint32_t AutoBaudDetection(void)
{
    at_autobaud_state_t autobaud_state = AUTOBAUD_STATE_INIT;

    uint8_t at_rxd_pin = 0;

    volatile uint32_t gpio_status = 0;
    uint32_t autobaud_tick[3] = {0};    
    uint32_t bits_len1 = 0,bits_len2 = 0;
    uint32_t checkvalue = 0;
    uint32_t auto_baudrate = 0;
    int8_t scan = 0;
    GPIO_InitTypeDef gpio_init = {0};
    uint32_t autobaud_table[] = {2400,4800,9600,19200,38400,57600,115200};
    
    //从NV获取at_rxd的引脚号
    at_rxd_pin = g_softap_fac_nv->at_rxd_pin;

    while(1)
    {
        /* 自动波特率检测状态机 */
        switch(autobaud_state)
        {
            /* 初始化状态：初始化RXD引脚为输入上拉，初始化SysTick最大倒计时为200ms */
            case AUTOBAUD_STATE_INIT:
            {
                //1:配置RXD所用引脚为：普通GPIO，输入上拉
                NVIC_DisableIRQ(GPIO_IRQn);
                set_lpuart1(DISABLE);//lpuart1使能时gpio4固定为rxd，若要配gpio4为输入上拉则必须关闭lpuart1
   	            gpio_init.Pin = at_rxd_pin;
	            gpio_init.Mode = GPIO_MODE_INPUT;
	            gpio_init.Pull = GPIO_PULL_UP;
	            GPIO_Init(&gpio_init);
                
                //2:初始化SysTick
                SysTick->LOAD  = SYSTICK_LOAD_RELOAD;
                SysTick->VAL  = 0UL;
                SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk| SysTick_CTRL_ENABLE_Msk;

                autobaud_state = AUTOBAUD_STATE_CAPTURE_FIRST;
                break;
            }
            /* 第一次捕获状态：捕获第一个下降沿，并记录最新tick值至autobaud_tick[0] */
            case AUTOBAUD_STATE_CAPTURE_FIRST:
            {
                do{
                    gpio_status = GPIO_ReadPin(at_rxd_pin);
                    autobaud_tick[0] = SysTick->VAL;
                }while(gpio_status != 0);

                for(volatile uint8_t i=0; i<10; i++);//消抖

                autobaud_state = AUTOBAUD_STATE_CAPTURE_SECOND;
                break;
            }
            /* 第二次捕获状态：捕获第二个下降沿，并记录最新tick值至autobaud_tick[1] */
            case AUTOBAUD_STATE_CAPTURE_SECOND:
            {
                //超时捕获上升沿，如果超时时长合理则捕获下降沿，否则切换至第一次捕获状态
                if(Capture_RisingEdge_withTimeout(at_rxd_pin) < 2)
                {
                    //超时捕获下降沿，如果超时时长合理则切换至第三次捕获状态，否则切换至第一次捕获状态
                    if(Capture_FallingEdge_withTimeout(at_rxd_pin,&autobaud_tick[1]) < 2)
                        autobaud_state = AUTOBAUD_STATE_CAPTURE_THIRD;
                    else
                        autobaud_state = AUTOBAUD_STATE_CAPTURE_FIRST;
                }
                else {
                    autobaud_state = AUTOBAUD_STATE_CAPTURE_FIRST;
                }
                break;
            }
            /* 第三次捕获状态：捕获第三个下降沿，并记录最新tick值至autobaud_tick[2] */
            case AUTOBAUD_STATE_CAPTURE_THIRD:
            {
                //超时捕获上升沿，如果超时时长合理则捕获下降沿，否则切换至第一次捕获状态
                if(Capture_RisingEdge_withTimeout(at_rxd_pin) < 2)
                {
                    //超时捕获下降沿，如果超时时长合理则切换至验证状态，否则切换至第一次捕获状态
                    if(Capture_FallingEdge_withTimeout(at_rxd_pin,&autobaud_tick[2]) < 2)
                        autobaud_state = AUTOBAUD_STATE_VERIFY;
                    else
                        autobaud_state = AUTOBAUD_STATE_CAPTURE_FIRST;
                }
                else {
                    autobaud_state = AUTOBAUD_STATE_CAPTURE_FIRST;
                }
                break;
            }
            /* 验证状态：计算并检验波特率,捕获后续下降沿个数等待rxd变为idle */
            case AUTOBAUD_STATE_VERIFY:
            {
                //计算tick_cnt比例
                bits_len1 = (autobaud_tick[0] - autobaud_tick[1] + SYSTICK_LOAD_RELOAD) % SYSTICK_LOAD_RELOAD;
                bits_len2 = (autobaud_tick[0] - autobaud_tick[2] + SYSTICK_LOAD_RELOAD) % SYSTICK_LOAD_RELOAD;
                checkvalue = (bits_len2*10)/bits_len1;
                
                //如果tick_cnt比例合理则计算波特率，否则切换至第一次捕获状态
                if( checkvalue>36 && checkvalue<44 )
                {
                    //将位的tick_cnt值转换成位时间，倒数为波特率
                    auto_baudrate = (BBPLL_CLK_DEFAULT*2/4)*8/bits_len2;

                    //找理论波特率
                    for (scan = sizeof(autobaud_table) / sizeof(autobaud_table[0]) - 1; scan >= 0; scan--)
                    {
                        if( (auto_baudrate>autobaud_table[scan]*8/10) && (auto_baudrate<autobaud_table[scan]*12/10) )
                        {
                            auto_baudrate = autobaud_table[scan];
                            break;
                        }
                    }
                    
                    //如果找到波特率则继续检测接下来的9~10位下降沿，如果没找到则切换至第一次捕获状态
                    if(scan >= 0)
                    {
                        if(Count_FallingEdge_Wait_Rxd_Idle(at_rxd_pin) == 1)
                        {
                            autobaud_state = AUTOBAUD_STATE_DONE;
                        }
                        else {
                            autobaud_state = AUTOBAUD_STATE_INIT;
                        }
                    }
                    else {
                        autobaud_state = AUTOBAUD_STATE_CAPTURE_FIRST;
                    }
                }
                else {
                    autobaud_state = AUTOBAUD_STATE_CAPTURE_FIRST;
                }
                break;
            }
            case AUTOBAUD_STATE_DONE: break;
            default: break;
        }

        /* 复位SysTick并退出while(1) */
        if(autobaud_state == AUTOBAUD_STATE_DONE)
        {
        	SysTick->LOAD = 0UL;
        	SysTick->VAL = 0UL;
        	SysTick->CTRL = 0UL;
        	break;
        }
    }
    
    //写工作态NV：写NV高7位为波特率结果
    g_softap_fac_nv->at_uart_rate = (auto_baudrate/2400)<<9;
    SAVE_FAC_PARAM(at_uart_rate);

    return (auto_baudrate/2400);
}
