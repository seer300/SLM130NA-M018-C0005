/** 
* @file        
* @brief   该头文件为gnss的ringbuf相关文档
* @warning     
*/
#pragma once
#include <stdint.h>
#include "xy_list.h"


typedef struct
{
	struct List_t *next;
	uint8_t id;           /*消息类型，see @ref gnss_msg_E*/
	uint16_t length;       /*payload长度。*/
	uint8_t payload[0];   /**/
} gnss_msg_t;



/*gnss_main主函数的消息类型*/
typedef enum
{
	GNSS_MSG_BASE = 0,
	GNSS_STREAM,
} gnss_msg_E;



extern ListHeader_t g_gnss_msg_head;




void send_msg_to_mainctl(uint8_t id,void *data,int len);








void gnss_clear_event_msg(void);


