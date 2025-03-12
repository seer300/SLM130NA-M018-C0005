/** 
* @file        
* @brief   该头文件为GNSS相关的AT命令处理函数，在at_cmd_regist.c中注册回调函数；还负责URC的组装发送
* @warning     
*/
#include "xy_system.h"
#include "xy_printf.h"
#include "urc_process.h"
#include "xy_cp.h"
#include "xy_lpm.h"
#include "xy_at_api.h"
#include "xy_memmap.h"





int at_GNSS_req(char *at_buf, char **prsp_cmd);

