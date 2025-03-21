// 保证头文件只被编译一次
#pragma once


int at_IOTRWADY_req(char *at_buf, char **prsp_cmd);
int at_IOTSEND_req(char *at_buf, char **prsp_cmd);