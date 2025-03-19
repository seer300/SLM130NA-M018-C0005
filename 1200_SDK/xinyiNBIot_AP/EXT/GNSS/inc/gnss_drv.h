/** 
* @file        
* @brief   该头文件为GNSS芯片底层驱动接口，负责控制GNSS芯片，并收发数据
* @warning     
*/
#include "xy_system.h"
#include "xy_printf.h"
#include "urc_process.h"
#include "xy_cp.h"
#include "xy_lpm.h"
#include "xy_at_api.h"
#include "xy_memmap.h"
#include "hal_csp.h"




//外部MCU传输过程中最大的干扰时长，用户可根据产品特性自行修改
//单位MS，该参数是设定并使用CSP模块自身的硬件超时特性，最大超时时长为 1/baudrate*1000*65535，例如当波特率为9600时，最大超时时长约6826ms。
//该参数在CSP_UART非阻塞接收API中的作用：当用户指定长度的数据没有全部收完时，若出现一段数据接收间隔大于设定的超时时长后就会触发接收完成回调函数的调用。
#define GNSS_UART_MAX_TIMEOUT    5

//一次GNSS数据最大接收缓存，GNSS一次吐数据不能超过这个值
#define GNSS_RCV_MAX_LEN    1500


extern uint8_t *g_gnss_data;

void gnss_nv_read(void);
void gnss_pin_reset();
void gnss_bak_power_set(uint8_t value);
void gnss_power_clock_init();
void gnss_power_clock_deinit();
void gnss_main_power_init();
void gnss_main_power_deinit();
void GNSS_UART_Init();
void gnss_boot_mode();
void gnss_exit_lpm_mode();

int write_to_gnss(uint8_t*buf, int size);

void gnss_uartbaud_change(uint32_t baud);

