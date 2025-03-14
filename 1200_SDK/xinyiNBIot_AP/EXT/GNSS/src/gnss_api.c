/** 
* @file        
* @brief   该源文件为GNSS操作的API接口，以供应用用户通过API接口进行GNSS硬件的操控。该接口可供用户在AP核上进行GNSS相关OPENCPU的二次开发
* @warning     
*/
#include "xy_system.h"
#include "xy_printf.h"
#include "urc_process.h"
#include "xy_cp.h"
#include "xy_lpm.h"
#include "xy_at_api.h"
#include "xy_memmap.h"
#include "gnss_api.h"
#include "at_uart.h"



/*GNSS不连续工作的周期秒数。值为0表示连续工作;值为有效值时，收到符合条件的定位信息后，会关闭GNSS该时长，以节省功耗*/
uint32_t g_period_sec = GNSS_PERIOD_SEC;


void gnss_write_hex_stream(char *hex_str)
{
	uint8_t *stream;
	uint32_t len;
	
	len = strlen(hex_str);
	stream = xy_malloc(len/2 + 1);
	
	hexstr2bytes(hex_str,len,(char*)stream,len/2);
	
	write_to_gnss(stream,len/2);

	xy_free(stream);
}

/*GNSS模块上电初始化。*/
void gnss_on()
{
    LPM_LOCK(STANDBY_GNSS_LOCK);
	LPM_LOCK(DSLEEP_GNSS_LOCK);
	gnss_pin_reset();
	GNSS_UART_Init();
}

// GNSS模块上电 但不初始化芯翼UART串口 (便于使用外接USB串口调试)
void gnss_on2()
{
    LPM_LOCK(STANDBY_GNSS_LOCK);
	LPM_LOCK(DSLEEP_GNSS_LOCK);
	gnss_pin_reset();
}


/*GNSS模块下电*/
void gnss_off()
{
	//发送下电命令
	gnss_write_hex_stream("F1D90641050000000000034F64");
	GNSS_UART_DeInit();
	// 等待500毫秒 GNSS手册要求
	HAL_Delay(500);

	// 操作GPIO2断电  输出低电平
	HAL_GPIO_WritePin(GPIO_PAD_NUM_2, GPIO_PIN_RESET);

    LPM_UNLOCK(STANDBY_GNSS_LOCK);
	LPM_UNLOCK(DSLEEP_GNSS_LOCK);
}

/*恢复出厂设置*/
void gnss_Restore_fac_mode()
{
	gnss_write_hex_stream("F1D90609080002000000FFFFFFFF1501");
}

/*复位*/
void gnss_reset()
{
	gnss_write_hex_stream("F1D906400100004721");
}

/*冷启动*/
void gnss_Cold_start()
{
	gnss_write_hex_stream("F1D906400100014822");
}

/*热启动*/
void gnss_Hot_start()
{
	gnss_write_hex_stream("F1D906400100034A24");
}

/*温启动*/
void gnss_Warm_start()
{
	gnss_write_hex_stream("F1D906400100024923");
}

/*NMEA 上报频率1/5/10. 最终的NMEA的log输出需要调用xy_printf_NMEA*/
void gnss_set_NMEA_freq(int hz)
{
	if(hz == 1)
	{
		gnss_write_hex_stream("F1D90642140000013835E8030000000000000000000000000000B56B");
	}
	else if(hz == 5)
	{
		gnss_write_hex_stream("F1D90642140000053800C80000000000000000000000000000006105");
	}
	else if(hz == 10)
	{
		gnss_write_hex_stream("F1D906421400000A3800640000000000000000000000000000000224");
	}
}

/*NMEA上报的子事件的开关*/
void gnss_GGA_enable(int enable)
{
	if(enable == 0)
		gnss_write_hex_stream("F1D906010300F00000FA0F");
	else
		gnss_write_hex_stream("F1D906010300F00001FB10");
}

/*NMEA上报的子事件的开关*/
void gnss_GLL_enable(int enable)
{
	if(enable == 0)
		gnss_write_hex_stream("F1D906010300F00100FB11");
	else
		gnss_write_hex_stream("F1D906010300F00101FC12");
}

/*NMEA上报的子事件的开关*/
void gnss_GSA_enable(int enable)
{
	if(enable == 0)
		gnss_write_hex_stream("F1D906010300F00200FC13");
	else
		gnss_write_hex_stream("F1D906010300F00201FD14");
}

/*NMEA上报的子事件的开关*/
void gnss_GSV_enable(int enable)
{
	if(enable == 0)
		gnss_write_hex_stream("F1D906010300F00400FE17");
	else
		gnss_write_hex_stream("F1D906010300F00401FF18");
}

/*NMEA上报的子事件的开关*/
void gnss_RMC_enable(int enable)
{
	if(enable == 0)
		gnss_write_hex_stream("F1D906010300F00500FF19");
	else
		gnss_write_hex_stream("F1D906010300F00501001A");
}

/*NMEA上报的子事件的开关*/
void gnss_VTG_enable(int enable)
{
	if(enable == 0)
		gnss_write_hex_stream("F1D906010300F00600001B");
	else
		gnss_write_hex_stream("F1D906010300F00601011C");
}

/*NMEA上报的子事件的开关*/
void gnss_ZDA_enable(int enable)
{
	if(enable == 0)
		gnss_write_hex_stream("F1D906010300F00700011D");
	else
		gnss_write_hex_stream("F1D906010300F00701021E");
}

/*NMEA上报的子事件的开关*/
void gnss_GST_enable(int enable)
{
	if(enable == 0)
		gnss_write_hex_stream("F1D906010300F00800021F");
	else
		gnss_write_hex_stream("F1D906010300F008010320");
}

/*NMEA上报的子事件的开关*/
void gnss_GNTXT_enable(int enable)
{
	if(enable == 0)
		gnss_write_hex_stream("F1D906010300F020001A4F");
	else
		gnss_write_hex_stream("F1D906010300F020011B50");
}

/*开关星历数据保存*/
void gnss_SVEPH_enable(int enable)
{
	if(enable == 0)
		gnss_write_hex_stream("F1D906100100001761");
	else
		gnss_write_hex_stream("F1D906100100011862");
}

/*制式模式，0:gps;1:beidou;2:gps+beidou*/
void gnss_set_mode(int mode)
{
	/*gps*/
	if(mode == 0)
	{
		gnss_write_hex_stream("F1D9060C04000100000017A0");
	}
	/*beidou*/
	else if(mode == 1)
	{
		gnss_write_hex_stream("F1D9060C0400040000001AAC");
	}
	/*gps + beidou*/
	else if(mode == 2)
	{
		gnss_write_hex_stream("F1D9060C0400050000001BB0");
	}
}


/*GNSS对端波特率设置，115200/9600*/
void gnss_set_baudrate(int rate)
{
	if(rate == 115200)
	{
		gnss_write_hex_stream("F1D9060008000000000000C20100D1E0");
	}
	else if(rate == 9600)
	{
		gnss_write_hex_stream("F1D9060008000000000080250000B307");
	}
}


/*保存NV配置，仅限FLASH版本*/
void gnss_save_nvm()
{
	gnss_write_hex_stream("F1D906090800000000002F00000046B7");
}


/*查询固件/硬件版本信息*/
void gnss_query_version()
{
	gnss_write_hex_stream("F1D90A0400000E34");
}

/*低功耗模式，休眠电流达到3.3V@8.4uA。通过拉低PRRSTX信号后释放唤醒*/
void gnss_set_lpm()
{
	gnss_write_hex_stream("F1D906410500FFFFFF7F03CB56");
}

