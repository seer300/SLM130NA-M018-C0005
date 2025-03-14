// /**
//  * @file    i2c_slave_poll_demo.c
//  * @brief   I2C从机机工作模式，轮询收发Demo
//  * @note    在这个Demo中，MCU做为I2C从机，接收92字节数据，超时时间内接收到指定数据长度，则将收到的92字节数据回写至I2C主机。
//  * 			如果超时时间未接收到指定长度数据，将实际接收长度的数据回写至I2C主机。
//  ***********************************************************************************/
// #include "hal_gpio.h"
// #include "hal_i2c.h"

// //用户收发一次数据的长度，不得超过MAX_ONCE_DATA_LEN
// #define USER_ONCE_DATA_LEN    92

// //数据缓存大小，用户可自定义，最大65536字节
// #define MAX_ONCE_DATA_LEN     128     //BYTES

// uint8_t rxdata[MAX_ONCE_DATA_LEN] = {0x00};

// HAL_I2C_HandleTypeDef i2c1_slave = {0};

// HAL_StatusTypeDef ret = HAL_OK;

// /**
//  * @brief I2C从机初始化函数，主要完成以下配置：
//  * 		  1. I2C的SCL、SDA引脚的GPIO初始化配置
//  * 		  2. I2C的主从模式、速率、自身地址、寻址模式初始化配置
//  */
// void I2C_Slave_Init(void)
// {
// 	//i2c gpio初始化
// 	HAL_GPIO_InitTypeDef GPIO_InitStruct = {0};

// 	GPIO_InitStruct.Mode = GPIO_MODE_HW_PER;
// 	GPIO_InitStruct.PinRemap = GPIO_I2C1_SCL;
// 	GPIO_InitStruct.Pin = GPIO_PAD_NUM_6;
// 	HAL_GPIO_Init(&GPIO_InitStruct);

//     GPIO_InitStruct.Mode = GPIO_MODE_HW_PER;
// 	GPIO_InitStruct.PinRemap = GPIO_I2C1_SDA;
// 	GPIO_InitStruct.Pin = GPIO_PAD_NUM_5;
// 	HAL_GPIO_Init(&GPIO_InitStruct);

// 	//i2c初始化
// 	i2c1_slave.Instance = HAL_I2C1;
// 	i2c1_slave.Init.Mode = HAL_I2C_MODE_SLAVE;
// 	i2c1_slave.Init.ClockSpeed = HAL_I2C_SPEED_400K;
// 	i2c1_slave.Init.OwnAddress = 0x38; //作为从机时的自身地址
// 	i2c1_slave.Init.AddressingMode = HAL_I2C_ADDRESS_7BITS;
// 	HAL_I2C_Init(&i2c1_slave);
// }

// /**
//  * @brief   在这个Demo中，MCU做为I2C从机接收92字节数据，超时时间内接收到指定数据长度，则将收到的92字节数据回写至I2C主机。
//  * 			如果超时时间未接收到指定长度数据，将实际接收长度的数据回写至I2C主机。
//  */
// __RAM_FUNC int main(void)
// {
// 	SystemInit();

// 	I2C_Slave_Init();

// 	while (1)
// 	{
//         //阻塞接收92字节，自身地址为0x38，设置超时时间为500ms
// 		ret = HAL_I2C_Slave_Receive(&i2c1_slave, rxdata, USER_ONCE_DATA_LEN, HAL_MAX_DELAY);
//         if(ret == HAL_OK)
//         {
// 			//成功接收指定字节数
// 			//用户进行数据处理，此处demo仅将接收到的数据回写
// 		    HAL_I2C_Slave_Transmit(&i2c1_slave, rxdata, USER_ONCE_DATA_LEN, HAL_MAX_DELAY);
//         }
//         else if(ret == HAL_TIMEOUT)
//         {
//             //超时未成功接收指定字节数，i2c1_slave.XferCount为实际接收字节数
//             if(i2c1_slave.XferCount > 0)
//             {
//                 //用户进行数据处理，此处demo仅将接收到的数据回写
//                 HAL_I2C_Slave_Transmit(&i2c1_slave, rxdata, i2c1_slave.XferCount, HAL_MAX_DELAY);
//             }
//         }
// 	}

// 	return 0;
// }