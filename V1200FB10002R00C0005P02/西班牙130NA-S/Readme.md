# factoryNV修订记录
此目录储存的factoryNV仅供西班牙客户130NA-S使用

# 客制化修改-`factoryNV.img`
## resetctl
设置为`12`  
12 -- 引脚低电平大于 160ms 唤醒，大于 5.12s 复位。

## deepsleep_enable
设置为0，代表关闭深睡 (将不会进入POWERDOWN模式)  

## ucPsmEnableFlg
设置为0，默认关闭PSM模式  

## ucEdrxEnableFlg
设置为0，默认关闭EDRX模式  

## at_uart_rate
设置为48，代表默认AT口为115200波特率  
计算公式：2400*48=115200  