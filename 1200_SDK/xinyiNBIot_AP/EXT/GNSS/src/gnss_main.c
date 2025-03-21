/** 
* @file        
* @brief   
* @warning     
*/
#include "xy_system.h"
#include "xy_printf.h"
#include "urc_process.h"
#include "xy_cp.h"
#include "xy_lpm.h"
#include "xy_at_api.h"
#include "xy_memmap.h"
#include "hal_gpio.h"
#include "hal_csp.h"
#include "gnss_api.h"
#include "at_uart.h"

#include "gnss_parse.h"


uint8_t g_gnss_timeout = 0; /*GNSS周期性唤醒定位超时标记，需要执行上电定位*/
uint32_t g_sat_num = 0; /*参与定位的有效卫星数*/
int g_positioned = 0;   /*1表示已定位成功*/

GNSS_Info_TypeDef gnss_datas = { 0 };



void parase_GNGGA(uint8_t *str,uint32_t len)
{
	char *head;
	char *end;
	char sat_num[10] = {0};
	head = find_special_symbol((char *)str,',',7);
	end = find_special_symbol((char *)str,',',8);
	(void)len;

	if(head!=NULL && end!=NULL)
	{
		memcpy(sat_num,head,end-head-1);
		g_sat_num = atoi((const char *)str);
	}
}

void parase_GNRMC(uint8_t *str,uint32_t len)
{
	char *head = find_special_symbol((char*)str,',',2);
	(void)len;
	if(*head == 'A')
	{
		g_positioned = 1;
	}
	else
	{
		g_positioned = 0;
	}	
}

/*周期性到期后，通知main主函数执行热启动，不得在中断回调中直接执行，否则耗时过久*/
__RAM_FUNC void period_timeout_Cb(void)
{
	g_gnss_timeout = 1;
}

/*解析GNSS芯片上报的定位信息，NMEA的log输出需要调用xy_printf_NMEA*/
int gnss_stream_proc(uint8_t *str,uint32_t len)
{
	/*需要解析GNSS码流，然后按照标准AT命令进行URC封装*/
	if(at_prefix_strstr((char *)str,"GNGGA"))
		parase_GNGGA(str,len);
	
	else if(at_prefix_strstr((char *)str,"GNRMC"))
		parase_GNRMC(str,len);
	

	/*判断参与卫星数超过8个并定位质量标识可信,进入低功耗*/
	if(g_positioned==1 && g_sat_num>8) 
	{
		/*给GNSS下主电，进入低功耗模式*/
		gnss_off();

		Timer_AddEvent(GNSS_PERIOD_TIMER,g_period_sec*1000, period_timeout_Cb,0);
	}
	return 1;
}



extern int g_hex_test;
extern void at_lpuart_write(char *buf, int size);
/*HEX码流测试模式，GNSS硬件上报的数据直接写LPUART，以透传发送给PC工具*/
int  hex_stream_test_proc(uint8_t *stream,uint32_t len)
{
	if(g_hex_test == 1 && len > 0)
	{
		at_lpuart_write((char *)stream,len);
		return 1;
	}
	else
		return 0;
}


/**
 * @brief gnss串口接收到的数据处理，在main主函数中执行转发或本地处理
 * 		  
 */
extern uint8_t g_gnss_log_enable;
void gnss_recv_process()
{
	gnss_msg_t *msg_node = NULL;

	/*由main函数来轮询执行上电GNSS*/
	if (g_gnss_timeout)
	{
		g_gnss_timeout = 0;
		gnss_on();
	}

	while((msg_node = (gnss_msg_t *)ListRemove(&g_gnss_msg_head)) != NULL)
	{

		switch (msg_node->id)
		{
			case GNSS_STREAM:  /*NMEA的log输出需要调用xy_printf_NMEA*/
				
				if(hex_stream_test_proc(msg_node->payload,msg_node->length) == 0)
				{
					/*需要解析GNSS码流，然后按照标准AT命令进行URC封装*/
					//gnss_stream_proc(msg_node->payload,msg_node->length);

					if (GNSS_Analysis(&gnss_datas, (const char *)msg_node->payload, (uint32_t)msg_node->length) == 0)
					{
						if (gnss_datas.RMC.IsEffectivePositioning != 0)
						{
							// 有效定位
							xy_printf("[GNSS] (RMC) UTC: %d/%d/%d %02d:%02d:%02d, Longitude: %.5f, Latitude: %.5f",
								gnss_datas.RMC.Date.Date, gnss_datas.RMC.Date.Month, gnss_datas.RMC.Date.Year,
								gnss_datas.RMC.Time.Hour, gnss_datas.RMC.Time.Minute, gnss_datas.RMC.Time.Second,
								gnss_datas.RMC.Position.Longitude.Value, gnss_datas.RMC.Position.Latitude.Value);
						}
					}
				}
				
				if(g_gnss_log_enable)
				{
					uint8_t *stream = xy_malloc(msg_node->length + 1);
					memset(stream, 0x00, msg_node->length + 1);
					memcpy(stream, msg_node->payload, msg_node->length);
					xy_printf_NMEA("%s", stream);
					xy_free(stream);
				}
				break;

			default:
				break;
		}

		xy_free(msg_node);
		msg_node = NULL;
	}
}

