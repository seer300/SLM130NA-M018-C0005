#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "xinyi2100.h"
#include "xy_printf.h"
#include "hal_gpio.h"
#include "uart.h"
#include "hal_lpuart.h"
#include "at_uart.h"
#include "at_process.h"
#include "at_hardware_cmd.h"
#include "at_cmd_regist.h"
#include "xy_flash.h"
#include "mcu_adapt.h"
#include "at_uart.h"
#include "auto_baudrate.h"

#include "gpio.h"
#include "hw_gpio.h"
#include "mcu_adapt.h"
#include "xy_event.h"
#include "xy_at_api.h"

#include "user_sapin_light.h"


// at+sled=<type>
// at+sled=?
// at+sled?
int at_SLED_req(char *at_buf, char **prsp_cmd)
{
    int slType = 0;

	if (g_cmd_type == AT_CMD_REQ)
	{
		slType = 0;
		if (at_parse_param("%d", at_buf, &slType) != XY_OK || slType < 0 || slType > 3)
		{
            return (XY_ERR_PARAM_INVALID);
        }

        *prsp_cmd = xy_malloc(32);
        snprintf(*prsp_cmd, 32, "\r\n+SLED:%d\r\n\r\nOK\r\n", slType);

		SapinLight_Start(slType);
	}
    else if (g_cmd_type == AT_CMD_QUERY)
    {
        *prsp_cmd = xy_malloc(32);
        snprintf(*prsp_cmd, 32, "\r\n+SLED:%d\r\n\r\nOK\r\n", SapinLight_Type_Get());
    }
	else if (g_cmd_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_malloc(32);
		snprintf(*prsp_cmd, 32, "\r\n+SLED:(0 or 1, 2 or 3)\r\n\r\nOK\r\n");
	}
	else
	{
        return (XY_ERR_PARAM_INVALID);
    }

	return XY_OK;
}
