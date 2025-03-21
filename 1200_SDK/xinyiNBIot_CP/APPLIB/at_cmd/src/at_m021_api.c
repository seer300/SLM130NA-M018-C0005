#include "at_m021_api.h"
#include "cJSON.h"
#include "FreeRTOS.h"
#include "xy_system.h"
#include "xy_utils.h"
#include "xy_ps_api.h"
#include "xy_at_api.h"
#include "atc_ps_def.h"
#include "xy_net_api.h"


/**
 * 线程回调函数  负责发送iotDataReady类型报文
 * 报文示例JSON格式:
 * {
        "eventType": "iotDataReady",
        "subscription": {
            "iccid": "8934076400003813993",
            "imsi": "214074302591929",
            "msisdn": "345901012148557",
            "eid": "",
            "imei": "",
            "id": 30278235,
            "customerId": "EU_PRUEBA_VT174c015ffcbcOfSGcaKFf6",
            "commercialGroupId": 51468,
            "supervisionGroupName": "GS_Monitorizacion"
        },
        "data": {
            "timestamp": "2022-04-22T12:15:20.243Z",
            "payload": {
                "fabricante": "TelComm",
                "evento": "Baliza70"
            }
        }
    }
 */
void process_send_readydata_task(){
    // 创建JSON根节点
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        user_printf("M021-Error: Failed to create JSON root object\n");
        return;
    }

    // 收集相关数据 res保存返回状态
    int res = 0;
    // iccid 长度20位数字
    char *iccid = xy_malloc(21);
    res = xy_get_NCCID(iccid, 21);
    if( res != ATC_AP_TRUE ){
        // 数据获取失败
        user_printf("M021-Error: get iccid fail!\n");
        return;
    }

    user_printf("M021- get iccid: %s\n", iccid);
    cJSON_AddStringToObject(root, "iccid", iccid);
    // imsi

    // msisdn

    // imei

    user_printf("M021- print json string: %s\n", cJSON_Print(root));

    // 递归删除整个JSON树，避免内存泄漏
    cJSON_Delete(root);
    // 逐个释放获取的参数值
    xy_free(iccid);
}


/**
 * 西班牙警示灯UDP报文: iotDataReady类型报文发送
 * eg: AT+IOTRWADY  不需要携带参数
 */
int at_IOTRWADY_req(char *at_buf, char **prsp_cmd){
    if (g_req_type == AT_CMD_ACTIVE){
        process_send_readydata_task();
        // osThreadAttr_t task_attr = {0};

        //命令调用类型为 AT+XXX
        

        // 创建线程, 收集数据并发送
        // ping_para_t *ping_para = (ping_para_t*)xy_malloc(sizeof(ping_para_t));
        // memcpy(ping_para, para, sizeof(ping_para_t));
    
        // task_attr.name = PING_THREAD_NAME;
        // task_attr.priority = PING_THREAD_PRIO;
        // task_attr.stack_size = PING_THREAD_STACK_SIZE;
        // at_ping_thread_id = osThreadNew((osThreadFunc_t)(process_ping_task), ping_para, &task_attr);

        return AT_END;
    }else{
        // 命令格式不匹配
        return ATERR_PARAM_INVALID;
    }
}

/**
 * 西班牙警示灯UDP报文: Protocol A类型报文发送
 * eg: AT+IOTSEND=
 */
int at_IOTSEND_req(char *at_buf, char **prsp_cmd){
    if (g_req_type == AT_CMD_REQ){
        //命令调用类型为 AT+XXX=param
        int sock_id = 0;

        // if (at_parse_param("%d(0-)", at_buf, &sock_id) != AT_OK)
        // {
        //     return ATERR_PARAM_INVALID;
        // }

        return AT_END;
    }else{
        // 命令格式不匹配
        return ATERR_PARAM_INVALID;
    }
}