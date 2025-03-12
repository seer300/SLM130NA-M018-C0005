
该文件夹里内容是GNSS相关，选用的芯片为华大。

NB系列为芯翼的1200 B1系列，AP核可用内存RAM总计128K，FLASH空间约400K左右。

实现GNSS的AT命令操控能力，以供外部MCU通过AT命令使用GNSS。



码流测试GNSS的说明：

拿1200系列，通过uart与华大的GNSS调试板相连；
基于1200的LPUART的AT通道，使用芯翼的AT调试工具，发送“AT+GNSS=HEX,<val>”命令；
val参数值就是HEX码流，搜索T_gnss_performance.py脚本中8120对应的码流，如果不清楚，咨询测试部的王健；
AP主控核收到该命令后，会解析出码流，直接调用GNSS底层驱动的写接口，发送码流给GNSS芯片；
GNSS芯片的上报数据，会再以ASCII码方式直接经LPUART发送给PC机的AT工具，如“GPRMC”"RMC"等

