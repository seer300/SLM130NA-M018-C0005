/** 
* @file        
* @brief   该源文件为GNSS相关的AT命令处理函数，在at_cmd_regist.c中注册回调函数；还负责URC的组装发送
* @warning     
*/
#include "xy_system.h"
#include "xy_printf.h"
#include "urc_process.h"
#include "xy_cp.h"
#include "xy_lpm.h"
#include "xy_at_api.h"
#include "xy_memmap.h"
#include "at_cmd_regist.h"
#include "at_uart.h"
#include "gnss_api.h"



/*值为1表示HEX码流测试模式，GNSS硬件上报的数据直接写LPUART发送给PC终端工具*/
int g_hex_test = 0;


void gnss_hex_test(char *at_buf)
{
	int len;
	char *hex_str = xy_malloc(strlen(at_buf));
	uint8_t *stream = xy_malloc(strlen(at_buf));

	at_parse_param(",%s,",at_buf,hex_str);
	len = strlen(hex_str);
	xy_assert(len%2 == 0);
	xy_printf("GNSS Recv AT+GNSS=HEX,%s",hex_str);
	
	hexstr2bytes(hex_str,len,(char *)stream,len/2);
	
	write_to_gnss(stream,len/2);

	xy_free(hex_str);
	xy_free(stream);
}

uint8_t g_gnss_log_enable = 0;
/*AT+GNSS=<NMEA上报子事件>,<enable>*/
int NMEA_control_req(char *at_buf,char *cmd,char **prsp_cmd)
{
	int enable = 0;
	(void)prsp_cmd;
	at_parse_param(",%d,",at_buf,&enable);

	if(enable!=0 && enable!=1)
		return XY_ERR;
	
	/*AT+GNSS=GGA,<enable>*/
	if (!strcmp(cmd, "GGA"))
	{	
		gnss_GGA_enable(enable);
	}
	/*AT+GNSS=GLL,<enable>*/
	else if (!strcmp(cmd, "GLL"))
	{	
		gnss_GLL_enable(enable);
	}
	/*AT+GNSS=GSA,<enable>*/
	else if (!strcmp(cmd, "GSA"))
	{	
		gnss_GSA_enable(enable);
	}
	/*AT+GNSS=GSV,<enable>*/
	else if (!strcmp(cmd, "GSV"))
	{	
		gnss_GSV_enable(enable);
	}
	/*AT+GNSS=RMC,<enable>*/
	else if (!strcmp(cmd, "RMC"))
	{	
		gnss_RMC_enable(enable);
	}
	/*AT+GNSS=VTG,<enable>*/
	else if (!strcmp(cmd, "VTG"))
	{	
		gnss_VTG_enable(enable);
	}
	/*AT+GNSS=ZDA,<enable>*/
	else if (!strcmp(cmd, "ZDA"))
	{	
		gnss_ZDA_enable(enable);
	}
	/*AT+GNSS=GST,<enable>*/
	else if (!strcmp(cmd, "GST"))
	{	
		gnss_GST_enable(enable);
	}
	/*AT+GNSS=GNTXT,<enable>*/
	else if (!strcmp(cmd, "GNTXT"))
	{	
		gnss_GNTXT_enable(enable);
	}
	/*AT+GNSS=SVEPH,<enable>   开关星历数据保存*/
	else if (!strcmp(cmd, "SVEPH"))
	{	
		gnss_SVEPH_enable(enable);
	}
	else
		return XY_ERR;
	return XY_OK;
}


/*AT+GNSS=<cmd>,<val>*/
int at_GNSS_req(char *at_buf, char **prsp_cmd)
{
	(void)prsp_cmd;
	if (g_cmd_type == AT_CMD_REQ)
	{
		char cmd[10] = {0};

		at_parse_param("%s,",at_buf,cmd);

		/*后续废弃！AT+GNSS=HEX,<val>  与PC机进行GNSS码流的透传*/
		if (!strcmp(cmd, "HEX"))
		{
			/*进入HEX码流测试模式，后续GNSS硬件上报的数据直接写LPUART发送给PC终端工具*/
			g_hex_test = 1;
			gnss_hex_test(at_buf);
		}
		/*后续废弃！AT+GNSS=STOP*/
		else if (!strcmp(cmd, "STOPHEX"))
		{
			/*切换为普通AT命令模式，GNSS硬件上报的数据需要按照标准URC进行封装处理，不能以码流方式传递*/
			g_hex_test = 0;
		}
		/*后续废弃！调试cspuart串口波特率*/
		else if (!strcmp(cmd, "UARTBAUD"))
		{
			int baud = 9600;
			at_parse_param(",%d,",at_buf,&baud);
			gnss_uartbaud_change(baud);
		}
		/*AT+GNSS=ON[,<Interval period>]*/
		else if (!strcmp(cmd, "ON"))
		{
			int sec = -1;
			at_parse_param(",%d,",at_buf,&sec);

			if(sec != -1)
				g_period_sec = sec;

			gnss_on();
		}
		else if (!strcmp(cmd, "OFF"))
		{
			gnss_off();
		}
		/*AT+GNSS=RESTORE  恢复出厂设置*/
		else if (!strcmp(cmd, "RESTORE"))
		{
			gnss_Restore_fac_mode();
		}
		/*AT+GNSS=RESET  复位*/
		else if (!strcmp(cmd, "RESET"))
		{
			gnss_reset();
		}
		/*AT+GNSS=COLD  冷启动*/
		else if (!strcmp(cmd, "COLD"))
		{
			gnss_Cold_start();
		}
		/*AT+GNSS=HOT  热启动*/
		else if (!strcmp(cmd, "HOT"))
		{
			gnss_Hot_start();
		}
		/*AT+GNSS=WARM  温启动*/
		else if (!strcmp(cmd, "WARM"))
		{
			gnss_Warm_start();
		}

		/*AT+GNSS=NMEAFREQ,<frequency>    NMEA 上报频率1/5/10*/
		else if (!strcmp(cmd, "NMEAFREQ"))
		{
			int freq = -1;
			at_parse_param(",%d,",at_buf,&freq);

			if(freq==1 || freq==5 || freq==10)		
				gnss_set_NMEA_freq(freq);
			else
				return XY_ERR_PARAM_INVALID;
		}
		else if (NMEA_control_req(at_buf,cmd,prsp_cmd) == XY_OK)
		{
		}

		/*AT+GNSS=MODE,<gps/beidou mode>   制式模式，0:gps;1:beidou;2:gps+beidou*/
		else if (!strcmp(cmd, "MODE"))
		{
			int mode = -1;
			at_parse_param(",%d,",at_buf,&mode);

			if(mode==0 || mode==1 || mode==2)		
				gnss_set_mode(mode);
			else
				return XY_ERR_PARAM_INVALID;
		}

		/*AT+GNSS=RATE,<gnss baud rate>   GNSS对端波特率设置，115200/9600*/
		else if (!strcmp(cmd, "RATE"))
		{
			int rate = -1;
			at_parse_param(",%d,",at_buf,&rate);

			if(rate==115200 || rate==9600)		
				gnss_set_baudrate(rate);
			else
				return XY_ERR_PARAM_INVALID;
		}

		/*AT+GNSS=SAVE  保存NV配置，仅限FLASH版本*/
		else if (!strcmp(cmd, "SAVE"))
		{
			gnss_save_nvm();
		}
		/*AT+GNSS=LPM  低功耗模式，休眠电流达到3.3V@8.4uA。通过拉低PRRSTX信号后释放唤醒*/
		else if (!strcmp(cmd, "LPM"))
		{
			gnss_set_lpm();
		}
		/*AT+GNSS=BOOT  GNSS进入BOOT模式下载固件*/
		else if (!strcmp(cmd, "BOOT"))
		{
			gnss_boot_mode();
		}
		/*AT+GNSS=NMEALOG,<ON/OFF>    NMEA log输出开关0、1*/
		else if (!strcmp(cmd, "NMEALOG"))
		{
			int log_enable = -1;
			at_parse_param(",%d,",at_buf,&log_enable);

			if(log_enable == 0 || log_enable == 1)		
				g_gnss_log_enable = log_enable;
			else
				return XY_ERR_PARAM_INVALID;
		}
		else
		{
			return XY_ERR_PARAM_INVALID;
		}
		
    	return XY_OK;
	}

	else
	{
		return XY_ERR_PARAM_INVALID;
	}
}


