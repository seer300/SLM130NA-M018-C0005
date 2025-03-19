# factoryNV修订记录
此目录储存的factoryNV仅供西班牙客户130NA-S使用

# 客制化修改-`factoryNV.img`
## resetctl
130NA-S模组硬件设计RESET按键默认高电平状态, 按下时触发低电平  
故根据要求设置为`19`  
19 -- 引脚低电平大于 20ms 唤醒，大于 2.56s 复位（重启）。

## deepsleep_enable
设置为0，代表关闭深睡 (将不会进入POWERDOWN模式)  

## ucPsmEnableFlg
设置为0，默认关闭PSM模式  

## ucEdrxEnableFlg
设置为0，默认关闭EDRX模式  

## at_uart_rate
设置为48，代表默认AT口为115200波特率  
计算公式：2400*48=115200  