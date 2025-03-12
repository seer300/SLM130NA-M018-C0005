
#include "gnss_msg.h"
#include "gnss_drv.h"
#include <stdint.h>
#include "gnss_api.h"
#include "gnss_main.h"

/* gnss主函数消息链表，含GNSS芯片上报事件和AP核本地发送消息*/
ListHeader_t g_gnss_msg_head = {0};



extern uint8_t check_gnss_cmd_rsp(uint8_t *buf);



void gnss_clear_event_msg(void)
{
	ListFreeAll(&g_gnss_msg_head);
}



__RAM_FUNC void send_msg_to_mainctl(uint8_t id,void *data,int len)
{
	gnss_msg_t *node = xy_malloc(sizeof(gnss_msg_t) + len + 1);
	

	node->id = id;
	node->length = len;
	memcpy(node->payload,data,len);
	*(node->payload+len) = '\0';

	node->next = NULL;

	ListInsert((List_t *)node,&g_gnss_msg_head);
}