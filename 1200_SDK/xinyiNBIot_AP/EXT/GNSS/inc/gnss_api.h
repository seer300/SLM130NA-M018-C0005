/** 
* @file        
* @brief   该头文件为GNSS操作的API接口，以供应用用户通过API接口进行GNSS硬件的操控。该接口可供用户在AP核上进行GNSS相关OPENCPU的二次开发
* @warning     
*/
#include "xy_system.h"
#include "xy_printf.h"
#include "urc_process.h"
#include "xy_cp.h"
#include "xy_lpm.h"
#include "xy_at_api.h"
#include "xy_memmap.h"
#include "gnss_drv.h"
#include "at_utils.h"
#include "gnss_msg.h"


/*GNSS不连续工作的周期秒数。值为0表示连续工作;值为有效值时，收到符合条件的定位信息后，会关闭GNSS该时长，以节省功耗*/
#define  GNSS_PERIOD_SEC            0
#define  GNSS_PERIOD_TIMER       TIMER_LP_USER3



/*GNSS不连续工作的周期秒数。值为0表示连续工作;值为有效值时，收到符合条件的定位信息后，会关闭GNSS该时长，以节省功耗*/
extern uint32_t g_period_sec;



/*GNSS模块上电初始化。*/
void gnss_on();


/*GNSS模块下电*/
void gnss_off();



/*恢复出厂设置*/
void gnss_Restore_fac_mode();

/*复位*/
void gnss_reset();

/*冷启动*/
void gnss_Cold_start();

/*热启动*/
void gnss_Hot_start();

/*温启动*/
void gnss_Warm_start();

/*NMEA 上报频率1/5/10*/
void gnss_set_NMEA_freq(int hz);

/*NMEA上报的子事件的开关*/
void gnss_GGA_enable(int enable);

/*NMEA上报的子事件的开关*/
void gnss_GLL_enable(int enable);

/*NMEA上报的子事件的开关*/
void gnss_GSA_enable(int enable);

/*NMEA上报的子事件的开关*/
void gnss_GSV_enable(int enable);

/*NMEA上报的子事件的开关*/
void gnss_RMC_enable(int enable);

/*NMEA上报的子事件的开关*/
void gnss_VTG_enable(int enable);

/*NMEA上报的子事件的开关*/
void gnss_ZDA_enable(int enable);

/*NMEA上报的子事件的开关*/
void gnss_GST_enable(int enable);

/*NMEA上报的子事件的开关*/
void gnss_GNTXT_enable(int enable);

/*开关星历数据保存*/
void gnss_SVEPH_enable(int enable);

/*制式模式，0:gps;1:beidou;2:gps+beidou*/
void gnss_set_mode(int mode);

/*GNSS对端波特率设置，115200/9600*/
void gnss_set_baudrate(int rate);

/*保存NV配置，仅限FLASH版本*/
void gnss_save_nvm();

/*查询固件/硬件版本信息*/
void gnss_query_version();

/*低功耗模式，休眠电流达到3.3V@8.4uA。通过拉低PRRSTX信号后释放唤醒*/
void gnss_set_lpm();
