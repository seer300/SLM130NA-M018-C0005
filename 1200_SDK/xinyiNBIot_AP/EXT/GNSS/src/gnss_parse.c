#include "gnss_parse.h"


/**
  *****************************************************************************
  * @Name   : GNSS校验数据
  *
  * @Brief  : none
  *
  * @Input  : *src: 需要校验字符串
  *
  * @Output : none
  *
  * @Return : 0: 失败
  *           1: 成功
  *****************************************************************************
**/
int GNSS_CheckDataInvalid(const char *src)
{
	int err = 0;
	uint32_t len = 0, i = 0;
	uint8_t buf[2];
	unsigned char crc_cal = 0, crc_rcv = 0;
	char *p1, *p2;
	
	p1 = strstr((const char *)src, (const char *)"$");
	p2 = strstr((const char *)src, (const char *)"*");
	p1++;
	len = p2 - p1;
	for (i = 0; i < len; i++)
	{
		crc_cal ^= (unsigned char)*(p1 + i);
	}
	
	memset((char *)buf, 0, sizeof(buf));
	strncpy((char *)buf, p2 + 1, 2);
	sscanf((const char *)buf, "%02x", (unsigned int *)&err);
	crc_rcv = (unsigned char)(err & 0xFFU);
	
	if (crc_cal == crc_rcv)
	{
		err = 1;
	}
	
	return err;
	
//	//以下方法比上面少 4 byte空间
//	int i = 0, c = 0;
//	uint32_t len = 0;
//	uint8_t check_sum = 0, ck = 0;
//	uint8_t pBuf[128];
//	
//	if (src[0] == '$')
//	{
//		//拷贝需要校验的数据
//		len = strlen((const char *)src);
//		for (i = 0; i < len; i++)
//		{
//			if (src[i] != '*')  //没遇到尾符
//			{
//				pBuf[i] = src[i + 1];
//			}
//			else
//			{
//				c = i;
//				i = len;
//			}
//		}
//		
//		//异或校验
//		ck = ((src[c + 1] - '0') << 4) | (src[c + 2] - '0');
//		for (i = 0; i < c - 1; i++)  //*不参与计算
//		{
//			check_sum ^= pBuf[i];
//		}
//		
//		if (check_sum == ck)
//		{
//			return 1;
//		}
//	}
//	
//	return 0;
}

/**
  *****************************************************************************
  * @Name   : GNSS转换经纬度数值
  *
  * @Brief  : none
  *
  * @Input  : Value: 需要转换的角度值
  *
  * @Output : *pValue: 度分数据输出
  *           *Degree: 度
  *           *Cent:   分
  *           *Second: 秒
  *
  * @Return : none
  *****************************************************************************
**/
void GNSS_LatitudeLongitudeConversion(double Value, double *pValue, uint32_t *Degree, uint32_t *Cent, double *Second)
{
	double val = Value;
	double dd = 0.0, cc = 0.0, ss = 0.0, tt = 0.0;
	uint32_t tmp = 0;
	
	//4546.40891
	//dd = 4546.40891 / 100 = 45.4640891
	dd = val / (double)100.0;  //得到度
	
	//cc = 4546.40891 - (45 * 100) = 46.40891
	tmp = (uint32_t)dd;
	cc = val - ((double)tmp * (double)100.0);  //得到分
	
	//ss = (46.40891 - (46 * 100)) * 60 = 24.5346
	tmp = (uint32_t)cc;
	ss = (cc - (double)tmp) * (double)60.0;
	
	tmp = (uint32_t)dd;
	tmp *= 100;
	
	tt = val - (double)tmp;
	tt /= (double)60.0;
	tmp = (uint32_t)dd;
	
	*Degree = (uint32_t)dd;
	*Cent = (uint32_t)cc;
	*Second = ss;
	*pValue = (double)tmp + tt;
}

/**
  *****************************************************************************
  * @Name   : GNSS GGA数据帧解码
  *
  * @Brief  : none
  *
  * @Input  : *src:   GGA数据帧字符串
  *
  * @Output : *info:  数据输出结构指针
  *
  * @Return : 0:  解码正确
  *           -1: 非法数据帧
  *****************************************************************************
**/
int GNSS_ParseGGA(GNSS_Info_TypeDef * info, const char *src)
{
	/*!< 数据帧格式说明
	 * $GPGGA,<1>,<2>,<3>,<4>,<5>,<6>,<7>,<8>,<9>,M,<10>,M,<11>,<12>*xx<CR><LF>
	 * <1>: UTC 时间，格式为hhmmss.sss，时分秒格式
	 * <2>: 纬度，格式为ddmm.mmmm(第一位是零也将传送)
	 * <3>: 纬度半球，N 或S(北纬或南纬)
	 * <4>: 经度，格式为dddmm.mmmm(第一位零也将传送)
	 * <5>: 经度半球，E 或W(东经或西经)
	 * <6>: 定位质量指示，0=定位无效，1=定位有效；GPS状态，0=未定位，1=非差分定位，2=差分定位，3=无效PPS，6=正在估算
	 * <7>: 使用卫星数量，从00到12(第一个零也将传送)
	 * <8>: HDOP水平精度因子（0.5-99.9）
	 * <9>: 天线离海平面的高度，-9999.9到9999.9米M指单位米；海拔高度（-9999.9-99999.9）
	 * <10>: 大地水准面高度，-9999.9到9999.9米M指单位米；地球椭圆面相对大地水准面的高度
	 * <11>: 差分GPS数据期限(RTCMSC-104)，最后设立RTCM传送的秒数量；差分时间（从最近一次接收到差分信号开始的秒数，如果不是差分定位将为空）
	 * <12>: 差分参考基站标号，从0000到1023(首位0也将传送)；差分站ID 0000-1023
	 * xx:   校验值
	 *
	 * 例如: $GNGGA,101507.00,2232.65596,N,11403.98689,E,1,08,1.01,238.0,M,-2.0,M,,*57
	*/
	
	int err = -1;
	double val = 0.0, Second = 0.0, tt = 0.0;
	uint32_t i = 0;
	uint32_t Degree = 0, Cent = 0;
	uint8_t sbit = 0;  //遇到分隔符标志
	uint8_t index = 0;
	uint8_t pBuf[32];

	if (*src == NULL || *src != '$')
	{
		return err;
	}
	else
	{
		err = 0;
	}
	
	
	//循环解析数据帧
	while (*src != '\0')
	{
		if (sbit != 0)  //遇到分隔符了，取分隔符后面的数字串
		{
			if (*src == ',')  //遇到下一个分隔符了
			{
				pBuf[i] = '\0';  //添加结束符
				val = 0.0;
				
				switch (index)
				{
					case 0:  //UTC时间
						if (strlen((const char *)pBuf) != 0)
						{
							info->GGA.Time.Hour = (uint8_t)(((pBuf[0] - '0') * 10) + (pBuf[1] - '0'));
							info->GGA.Time.Minute = (uint8_t)(((pBuf[2] - '0') * 10) + (pBuf[3] - '0'));
							info->GGA.Time.Second = (uint8_t)(((pBuf[4] - '0') * 10) + (pBuf[5] - '0'));
							i = 0;
							while (pBuf[i + 7] != '\0')
							{
								pBuf[i] = pBuf[i + 7];
								i++;
							}
							pBuf[i] = '\0';  //添加结束符
							info->GGA.Time.mSecond = (uint16_t)atoi((const char *)pBuf);
							
							info->GGA.Time.Hour += 8;  //UTC ==> BTC
						}
						break;
					case 1:  //纬度
						if (strlen((const char *)pBuf) != 0)
						{
							val = get_float_value((const char *)pBuf);
							info->GGA.Position.LatitudeOriginal = val;
							GNSS_LatitudeLongitudeConversion(val, &tt, &Degree, &Cent, &Second);  //占用空间840 bytes
							info->GGA.Position.Latitude.Value = tt;
							info->GGA.Position.Latitude.Degree = Degree;
							info->GGA.Position.Latitude.Minute = Cent;
							info->GGA.Position.Latitude.Second = Second;
							
							//采用以下方式930 bytes
//							info->GGA.Position.Latitude_Degree = (uint32_t)(val / 100.0);
//							info->GGA.Position.Latitude_Cent = (uint32_t)(val - info->GGA.Position.Latitude_Degree * 100);
//							info->GGA.Position.Latitude_Second = (uint32_t)(((val - info->GGA.Position.Latitude_Degree * 100) - info->GGA.Position.Latitude_Cent) * 60);
//							info->GGA.Position.Latitude = ((double)info->GGA.Position.Latitude_Degree) + ((val - (double)info->GGA.Position.Latitude_Degree * 100.0) / 60.0);
						}
						else
						{
							info->GGA.Position.LatitudeOriginal = 0.0;
							info->GGA.Position.Latitude.Value = 0.0;
							info->GGA.Position.Latitude.Degree = 0;
							info->GGA.Position.Latitude.Minute = 0;
							info->GGA.Position.Latitude.Second = 0.0;
						}
						break;
					case 2:  //纬度半球，N 或S(北纬或南纬)
						info->GGA.Position.IsNorth = 0;
						if (pBuf[0] == 'N')
						{
							info->GGA.Position.IsNorth = 1;
						}
//						else
//						{
//							//南纬需要变负数
//							info->GGA.Position.Latitude = -info->GGA.Position.Latitude;
//						}
						break;
					case 3:  //经度
						if (strlen((const char *)pBuf) != 0)
						{
							val = get_float_value((const char *)pBuf);
							info->GGA.Position.LongitudeOriginal = val;
							GNSS_LatitudeLongitudeConversion(val, &tt, &Degree, &Cent, &Second);
							info->GGA.Position.Longitude.Value = tt;
							info->GGA.Position.Longitude.Degree = Degree;
							info->GGA.Position.Longitude.Minute = Cent;
							info->GGA.Position.Longitude.Second = Second;
							
//							info->GGA.Position.Longitude_Degree = (uint32_t)(val / 100);
//							info->GGA.Position.Longitude_Cent = (uint32_t)(val - info->GGA.Position.Longitude_Degree * 100);
//							info->GGA.Position.Longitude_Second = (uint32_t)(((val - info->GGA.Position.Longitude_Degree * 100) - info->GGA.Position.Longitude_Cent) * 60);
//							info->GGA.Position.Longitude = ((double)info->GGA.Position.Longitude_Degree) + ((val - (double)info->GGA.Position.Longitude_Degree * 100.0) / 60.0);
						}
						else
						{
							info->GGA.Position.LongitudeOriginal = 0.0;
							info->GGA.Position.Longitude.Value = 0.0;
							info->GGA.Position.Longitude.Degree = 0;
							info->GGA.Position.Longitude.Minute = 0;
							info->GGA.Position.Longitude.Second = 0.0;
						}
						break;
					case 4:  //经度半球，E 或W(东经或西经)
						info->GGA.Position.IsEast = 0;
						if (pBuf[0] == 'E')
						{
							info->GGA.Position.IsEast = 1;
						}
//						else
//						{
//							//西经需要变负数
//							info->GGA.Position.Longitude = -info->GGA.Position.Longitude;
//						}
						break;
					case 5:  //定位质量指示，0=定位无效，1=定位有效；GPS状态，0=未定位，1=非差分定位，2=差分定位，3=无效PPS，6=正在估算
						info->GGA.GPS_State = pBuf[0] - '0';
						break;
					case 6:  //使用卫星数量，从00到12(第一个零也将传送)
						info->GGA.GPS_Number = (uint8_t)(((pBuf[0] - '0') * 10) + (pBuf[1] - '0'));
						break;
					case 7:  //HDOP水平精度因子（0.5-99.9）
						if (strlen((const char *)pBuf) != 0)
						{
							val = get_float_value((const char *)pBuf);
						}
						info->GGA.PrecisionFactorHDOP = val;
						break;
					case 8:  //天线离海平面的高度，-9999.9到9999.9米M指单位米；海拔高度（-9999.9-99999.9）
						if (strlen((const char *)pBuf) != 0)
						{
							val = get_float_value((const char *)pBuf);
						}
						info->GGA.AltitudeHeight = val;
						break;
					case 9:  //天线离海平面的高度单位
						break;
					case 10:  //大地水准面高度，-9999.9到9999.9米M指单位米；地球椭圆面相对大地水准面的高度
						if (strlen((const char *)pBuf) != 0)
						{
							val = get_float_value((const char *)pBuf);
						}
						info->GGA.LevelHeight = val;
						break;
					case 11:  //大地水准面高度单位
						break;
					case 12:  //差分GPS数据期限(RTCMSC-104)，最后设立RTCM传送的秒数量；差分时间（从最近一次接收到差分信号开始的秒数，如果不是差分定位将为空）
						if (info->GGA.GPS_State == 2)
						{
							if (strlen((const char *)pBuf) != 0)
							{
								val = get_float_value((const char *)pBuf);
							}
							info->GGA.DifferentialTime = val;
						}
						break;
					case 13:  //差分参考基站标号，从0000到1023(首位0也将传送)；差分站ID 0000-1023
						//需要截取‘*’之前，‘，’分隔符之后的数值
						if (info->GGA.GPS_State == 2)
						{
							i = 0;
							while (pBuf[i] != '\0')
							{
								if (pBuf[i] == '*')
								{
									pBuf[i] = '\0';  //结束符代替
									break;
								}
								else
								{
									i++;
								}
							}
							i = 0;
							if (strlen((const char *)pBuf) != 0)
							{
								i = (uint16_t)get_int_value((const char *)pBuf);
							}
							info->GGA.DifferentialStationID = i;
						}
						break;
					default:
						break;
				}
				
				i = 0;
				index++;
			}
			else
			{
				pBuf[i] = *src;  //取分隔符之间的数字串
				i++;
			}
		}
		else  //没遇到分隔符
		{
			if (*src == ',')  //遇到下一个分隔符了
			{
				sbit = 1;
			}
		}
		src++;
	}
	
	if (info->GGA.GPS_State == 1 || info->GGA.GPS_State == 2)  //非差分定位或者差分定位
	{
		// return 0;
	}
	
	// return -1;  //非定位完成
	return err;
}

/**
  *****************************************************************************
  * @Name   : GNSS VTG数据帧解码
  *
  * @Brief  : none
  *
  * @Input  : *src:   VTG数据帧字符串
  *           Length: VTG数据帧长度
  *
  * @Output : *info:  数据输出结构指针
  *
  * @Return : 0:  解码正确
  *           -1: 非法数据帧
  *****************************************************************************
**/
int GNSS_ParseVTG(GNSS_Info_TypeDef * info, const char *src)
{
	/*!< 数据帧格式说明
	 * $GPVTG,<1>,T,<2>,M,<3>,N,<4>,K,<5>*hh<CR><LF>
	 * <1>: 以正北为参考基准的地面航向(000~359度，前面的0也将被传输)；运动角度，000-359
	 * <2>: 以磁北为参考基准的地面航向(000~359度，前面的0也将被传输)；运动角度，000-359
	 * <3>: 地面速率(000.0~999.9节，前面的0也将被传输)；水平运动速度（0.00），N=节，Knots
	 * <4>: 地面速率(0000.0~1851.8公里/小时，前面的0也将被传输)；水平运动速度（0.00），K=公里/时，km/h
	 * <5>: 模式指示(仅NMEA0183 3.00版本输出，A=自主定位，D=差分，E=估算，N=数据无效
	 * hh:   校验值
	 *
	 * 例如: $GNVTG,,T,,M,0.003,N,0.005,K,A*3B
	*/
	
	int err = -1;
	double val = 0.0;
	uint32_t i = 0;
	uint8_t sbit = 0;  //遇到分隔符标志
	uint8_t index = 0;
	uint8_t ch = 0;
	uint8_t pBuf[32];

	if (src == NULL || *src != '$')
	{
		return err;
	}
	else
	{
		err = 0;
	}
	
	
	//循环解析数据帧
	ch = ',';  //分隔符
	while (*src != '\0')
	{
		if (sbit != 0)  //遇到分隔符了，取分隔符后面的数字串
		{
			if (*src == ch)  //遇到下一个分隔符了
			{
				pBuf[i] = '\0';  //添加结束符
				val = 0.0;
				
				switch (index)
				{
					case 0:  //以正北为参考基准的地面航向(000~359度，前面的0也将被传输)；运动角度，000-359
						if (strlen((const char *)pBuf) != 0)
						{
							val = get_float_value((const char *)pBuf);
						}
						info->VTG.NorthAirlines = val;
						break;
					case 1:  //正北标志'T'
						break;
					case 2:  //以磁北为参考基准的地面航向(000~359度，前面的0也将被传输)；运动角度，000-359
						if (strlen((const char *)pBuf) != 0)
						{
							val = get_float_value((const char *)pBuf);
						}
						info->VTG.MagneticNorthAirlines = val;
						break;
					case 3:  //磁北标志‘M'
						break;
					case 4:  //地面速率(000.0~999.9节，前面的0也将被传输)；水平运动速度（0.00），N=节，Knots
						if (strlen((const char *)pBuf) != 0)
						{
							val = get_float_value((const char *)pBuf);
						}
						info->VTG.GroundRateKnots = val;
						break;
					case 5:  //地面速率单位'N'
						break;
					case 6:  //地面速率(0000.0~1851.8公里/小时，前面的0也将被传输)；水平运动速度（0.00），K=公里/时，km/h
						if (strlen((const char *)pBuf) != 0)
						{
							val = get_float_value((const char *)pBuf);
						}
						info->VTG.GroundRateKmh = val;
						break;
					case 7:  //地面速率单位'K'
						ch = '*';  //最后遇到*结束
						break;
					case 8:  //模式指示(仅NMEA0183 3.00版本输出，A=自主定位，D=差分，E=估算，N=数据无效
						//需要截取‘*’之前，‘，’分隔符之后的数值
						if (strlen((const char *)pBuf) != 0)
						{
							ch = pBuf[0];
							switch (ch)
							{
								case 'A':
									info->VTG.Mode = 1;
									break;
								case 'D':
									info->VTG.Mode = 2;
									break;
								case 'E':
									info->VTG.Mode = 3;
									break;
//								case 'N':
//									info->VTG.Mode = 0;
//									break;
								default:
									info->VTG.Mode = 0;
									break;
							}
						}
						break;
					default:
						break;
				}
				
				i = 0;
				index++;
			}
			else
			{
				pBuf[i] = *src;  //取分隔符之间的数字串
				i++;
			}
		}
		else  //没遇到分隔符
		{
			if (*src == ',')  //遇到下一个分隔符了
			{
				sbit = 1;
			}
		}
		src++;
	}
	
	if (info->VTG.Mode != 0)  //定位有效
	{
		// return 0;
	}
	
	// return -1;
	return err;
}

/**
  *****************************************************************************
  * @Name   : GNSS RMC数据帧解码
  *
  * @Brief  : none
  *
  * @Input  : *src:   RMC数据帧字符串
  *           Length: RMC数据帧长度
  *
  * @Output : *info:  数据输出结构指针
  *
  * @Return : 0:  解码正确
  *           -1: 非法数据帧
  *****************************************************************************
**/
int GNSS_ParseRMC(GNSS_Info_TypeDef * info, const char *src)
{
	/*!< 数据帧格式说明
	 * $GPRMC,<1>,<2>,<3>,<4>,<5>,<6>,<7>,<8>,<9>,<10>,<11>,<12>,<13>*hh<CR><LF>
	 * <1>: UTC 时间，hhmmss(时分秒)格式
	 * <2>: 定位状态，A=有效定位，V=无效定位
	 * <3>: 纬度ddmm.mmmm(度分)格式(前面的0也将被传输)
	 * <4>: 纬度半球N(北半球)或S(南半球)
	 * <5>: 经度dddmm.mmmm(度分)格式(前面的0也将被传输)
	 * <6>: 经度半球E(东经)或W(西经)
	 * <7>: 地面速率(000.0~999.9节，前面的0也将被传输)
	 * <8>: 地面航向(000.0~359.9度，以真北为参考基准，前面的0也将被传输)
	 * <9>: UTC 日期，ddmmyy(日月年)格式
	 * <10>: 磁偏角(000.0~180.0度，前面的0也将被传输)
	 * <11>: 磁偏角方向，E(东)或W(西)
	 * <12>: 模式指示(仅NMEA01833.00版本输出，A=自主定位，D=差分，E=估算，N=数据无效)
	 * <13>: 定位状态，A=有效定位，V=无效定位
	 * hh:   校验值
	 *
	 * 例如: $GNRMC,060932.00,V,,,,,,,310519,,,N,V*18
	*/
	
	int err = -1;
	double val = 0.0, Second = 0.0, tt = 0.0;
	uint32_t i = 0;
	uint32_t Degree = 0, Cent = 0;
	uint8_t sbit = 0;  //遇到分隔符标志
	uint8_t index = 0;
	uint8_t ch = 0;
	uint8_t pBuf[32];

	if (src == NULL || *src != '$')
	{
		return err;
	}
	else
	{
		err = 0;
	}
	
	//循环解析数据帧
	ch = ',';  //分隔符
	while (*src != '\0')
	{
		if (sbit != 0)  //遇到分隔符了，取分隔符后面的数字串
		{
			if (*src == ',')  //遇到下一个分隔符了
			{
				pBuf[i] = '\0';  //添加结束符
				val = 0.0;
				
				switch (index)
				{
					case 0:  //UTC时间
						if (strlen((const char *)pBuf) != 0)
						{
							info->RMC.Time.Hour = (uint8_t)(((pBuf[0] - '0') * 10) + (pBuf[1] - '0'));
							info->RMC.Time.Minute = (uint8_t)(((pBuf[2] - '0') * 10) + (pBuf[3] - '0'));
							info->RMC.Time.Second = (uint8_t)(((pBuf[4] - '0') * 10) + (pBuf[5] - '0'));
							i = 0;
							while (pBuf[i + 7] != '\0')
							{
								pBuf[i] = pBuf[i + 7];
								i++;
							}
							pBuf[i] = '\0';  //添加结束符
							info->RMC.Time.mSecond = (uint16_t)atoi((const char *)pBuf);
							
							info->RMC.Time.Hour += 8;  //UTC ==> BTC
						}
						break;
					case 1:  //定位状态，A=有效定位，V=无效定位
						if (pBuf[0] == 'A')  //定位有效
						{
							info->RMC.IsEffectivePositioning = 1;
						}
						else if (pBuf[0] == 'V')
						{
							info->RMC.IsEffectivePositioning = 0;
						}
						break;
					case 2:  //纬度
						if (strlen((const char *)pBuf) != 0)
						{
							val = get_float_value((const char *)pBuf);
							info->RMC.Position.LatitudeOriginal = val;
							GNSS_LatitudeLongitudeConversion(val, &tt, &Degree, &Cent, &Second);
							info->RMC.Position.Latitude.Value = tt;
							info->RMC.Position.Latitude.Degree = Degree;
							info->RMC.Position.Latitude.Minute = Cent;
							info->RMC.Position.Latitude.Second = Second;
						}
						else
						{
							info->RMC.Position.LatitudeOriginal = 0.0;
							info->RMC.Position.Latitude.Value = 0.0;
							info->RMC.Position.Latitude.Degree = 0;
							info->RMC.Position.Latitude.Minute = 0;
							info->RMC.Position.Latitude.Second = 0.0;
						}
						break;
					case 3:  //纬度半球，N 或S(北纬或南纬)
						info->RMC.Position.IsNorth = 0;
						if (pBuf[0] == 'N')
						{
							info->RMC.Position.IsNorth = 1;
						}
//						else
//						{
//							//南纬需要变负数
//							info->RMC.Position.Latitude = -info->RMC.Position.Latitude;
//						}
						break;
					case 4:  //经度
						if (strlen((const char *)pBuf) != 0)
						{
							val = get_float_value((const char *)pBuf);
							info->RMC.Position.LongitudeOriginal = val;
							GNSS_LatitudeLongitudeConversion(val, &tt, &Degree, &Cent, &Second);
							info->RMC.Position.Longitude.Value = tt;
							info->RMC.Position.Longitude.Degree = Degree;
							info->RMC.Position.Longitude.Minute = Cent;
							info->RMC.Position.Longitude.Second = Second;
						}
						else
						{
							info->RMC.Position.LongitudeOriginal = 0.0;
							info->RMC.Position.Longitude.Value = 0.0;
							info->RMC.Position.Longitude.Degree = 0;
							info->RMC.Position.Longitude.Minute = 0;
							info->RMC.Position.Longitude.Second = 0.0;
						}
						break;
					case 5:  //经度半球，E 或W(东经或西经)
						info->RMC.Position.IsEast = 0;
						if (pBuf[0] == 'E')
						{
							info->RMC.Position.IsEast = 1;
						}
//						else
//						{
//							//西经需要变负数
//							info->RMC.Position.Longitude = -info->RMC.Position.Longitude;
//						}
						break;
					case 6:  //地面速率(000.0~999.9节，前面的0也将被传输)
						if (strlen((const char *)pBuf) != 0)
						{
							val = get_float_value((const char *)pBuf);
						}
						info->RMC.GroundRateKnots = val;
						//转换得到km/h
						info->RMC.GroundRateKmh = val * ((double)1.8522);
						break;
					case 7:  //地面航向(000.0~359.9度，以真北为参考基准，前面的0也将被传输)
						if (strlen((const char *)pBuf) != 0)
						{
							val = get_float_value((const char *)pBuf);
						}
						info->RMC.NorthAirlines = val;
						break;
					case 8:  //UTC 日期，ddmmyy(日月年)格式
						info->RMC.Date.Date = (uint8_t)(((pBuf[0] - '0') * 10) + (pBuf[1] - '0'));
						info->RMC.Date.Month = (uint8_t)(((pBuf[2] - '0') * 10) + (pBuf[3] - '0'));
						info->RMC.Date.Year = (uint16_t)(((pBuf[4] - '0') * 10) + (pBuf[5] - '0'));
						info->RMC.Date.Year += 2000;  ////从2000年开始算起
						break;
					case 9:  //磁偏角(000.0~180.0度，前面的0也将被传输)
						if (strlen((const char *)pBuf) != 0)
						{
							val = get_float_value((const char *)pBuf);
						}
						info->RMC.MagneticDeclination = val;
						break;
					case 10:  //磁偏角方向，E(东)或W(西)
						info->RMC.IsMDEast = 0;
						if (strlen((const char *)pBuf) != 0)
						{
							if (pBuf[0] == 'E')
							{
								info->RMC.IsMDEast = 1;
							}
						}
						break;
					case 11:  //模式指示(仅NMEA01833.00版本输出，A=自主定位，D=差分，E=估算，N=数据无效)
						ch = pBuf[0];
						switch (ch)
						{
							case 'A':  //自主定位
								info->RMC.Mode = 1;
								break;
							case 'D':  //差分定位
								info->RMC.Mode = 2;
								break;
							case 'E':  //估算
								info->RMC.Mode = 3;
								break;
//							case 'N':  //数据无效
//								info->RMC.Mode = 0;
//								break;
							default:
								info->RMC.Mode = 0;
								break;
						}
						ch = '*';  //最后遇到*结束
						break;
					case 12:  //定位状态，A=有效定位，V=无效定位
						//需要截取‘*’之前，‘，’分隔符之后的数值
						if (strlen((const char *)pBuf) != 0)
						{
							if (pBuf[0] == 'A')  //定位有效
							{
								info->RMC.IsEffectivePositioning = 1;
							}
							else if (pBuf[0] == 'V')
							{
								info->RMC.IsEffectivePositioning = 0;
							}
						}
						break;
					default:
						break;
				}
				
				i = 0;
				index++;
			}
			else
			{
				pBuf[i] = *src;  //取分隔符之间的数字串
				i++;
			}
		}
		else  //没遇到分隔符
		{
			if (*src == ',')  //遇到下一个分隔符了
			{
				sbit = 1;
			}
		}
		src++;
	}
	
	if (info->RMC.IsEffectivePositioning != 0)  //定位有效
	{
		// return 0;
	}
	
	// return -1;
	return err;
}

/**
  *****************************************************************************
  * @Name   : GNSS GSA数据帧解码
  *
  * @Brief  : none
  *
  * @Input  : *src:   GSA数据帧字符串
  *           Length: GSA数据帧长度
  *
  * @Output : *info:  数据输出结构指针
  *
  * @Return : 0:  解码正确
  *           -1: 非法数据帧
  *****************************************************************************
**/
int GNSS_ParseGSA(GNSS_Info_TypeDef * info, const char *src)
{
	/*!< 数据帧格式说明
	 * $GPGSA,<1>,<2>,<3>,<4>,<5>,<6>,<7>,<8>,<9>,<10>,<11>,<12>,<13>,<14>,<15>,<16>,<17>,<18>*hh<CR><LF>
	 * <1>: 定位模式，A=自动手动2D/3D，M=手动2D/3D
	 * <2>: 定位类型，1=未定位，2=2D定位，3=3D定位
	 * <3>: PRN码（伪随机噪声码），第1信道正在使用的卫星PRN码编号（00）（前导位数不足则补0）
	 * <4>: PRN码（伪随机噪声码），第2信道正在使用的卫星PRN码编号（00）（前导位数不足则补0）
	 * <5>: PRN码（伪随机噪声码），第3信道正在使用的卫星PRN码编号（00）（前导位数不足则补0）
	 * <6>: PRN码（伪随机噪声码），第4信道正在使用的卫星PRN码编号（00）（前导位数不足则补0）
	 * <7>: PRN码（伪随机噪声码），第5信道正在使用的卫星PRN码编号（00）（前导位数不足则补0）
	 * <8>: PRN码（伪随机噪声码），第6信道正在使用的卫星PRN码编号（00）（前导位数不足则补0）
	 * <9>: PRN码（伪随机噪声码），第7信道正在使用的卫星PRN码编号（00）（前导位数不足则补0）
	 * <10>: PRN码（伪随机噪声码），第8信道正在使用的卫星PRN码编号（00）（前导位数不足则补0）
	 * <11>: PRN码（伪随机噪声码），第9信道正在使用的卫星PRN码编号（00）（前导位数不足则补0）
	 * <12>: PRN码（伪随机噪声码），第10信道正在使用的卫星PRN码编号（00）（前导位数不足则补0）
	 * <13>: PRN码（伪随机噪声码），第11信道正在使用的卫星PRN码编号（00）（前导位数不足则补0）
	 * <14>: PRN码（伪随机噪声码），第12信道正在使用的卫星PRN码编号（00）（前导位数不足则补0）
	 * <15>: PDOP综合位置精度因子（0.5 - 99.9）
	 * <16>: HDOP水平精度因子（0.5 - 99.9）
	 * <17>: VDOP垂直精度因子（0.5 - 99.9）
	 * <18>: 
	 * hh:   校验值
	 *
	 * 例如: $GNGSA,A,1,,,,,,,,,,,,,10.6,10.5,1.00,1*01
	*/
	
	int err = -1;
	double val = 0.0;
	uint32_t i = 0;
	uint8_t sbit = 0;  //遇到分隔符标志
	uint8_t index = 0, ch_cnt = 0;
	uint8_t ch = 0;
	uint8_t pBuf[32];

	if (src == NULL || *src != '$')
	{
		return err;
	}
	else
	{
		err = 0;
	}
	
	//循环解析数据帧
	ch = ',';  //分隔符
	while (*src != '\0')
	{
		if (sbit != 0)  //遇到分隔符了，取分隔符后面的数字串
		{
			if (*src == ch)  //遇到下一个分隔符了
			{
				pBuf[i] = '\0';  //添加结束符
				val = 0.0;
				
				switch (index)
				{
					case 0:  //定位模式，A=自动手动2D/3D，M=手动2D/3D
						info->GSA.Mode = 0;
						if (strlen((const char *)pBuf) != 0)
						{
							if (pBuf[0] == 'M')
							{
								info->GSA.Mode = 1;
							}
						}
						index++;
						break;
					case 1:  //定位类型，1=未定位，2=2D定位，3=3D定位
						info->GSA.Type = 0;
						if (strlen((const char *)pBuf) != 0)
						{
							info->GSA.Type = pBuf[0] - '0';
						}
						index++;
						break;
					case 2:  //PRN码（伪随机噪声码），第1信道正在使用的卫星PRN码编号（00）（前导位数不足则补0）
						if (info->GSA.Type > 1)
						{
							if (strlen((const char *)pBuf) != 0)
							{
								info->GSA.PRNCode[ch_cnt] = (uint8_t)get_int_value((const char *)pBuf);
							}
						}
						ch_cnt++;
						if (ch_cnt > 11)
						{
							index++;
						}
						break;
					case 3:  //PDOP综合位置精度因子（0.5 - 99.9）
						if (strlen((const char *)pBuf) != 0)
						{
							val = get_float_value((const char *)pBuf);
						}
						info->GSA.PrecisionFactorPDOP = val;
						index++;
						break;
					case 4:  //HDOP水平精度因子（0.5 - 99.9）
						if (strlen((const char *)pBuf) != 0)
						{
							val = get_float_value((const char *)pBuf);
						}
						info->GSA.PrecisionFactorHDOP = val;
						index++;
						break;
					case 5:  //VDOP垂直精度因子（0.5 - 99.9）
						if (strlen((const char *)pBuf) != 0)
						{
							val = get_float_value((const char *)pBuf);
						}
						info->GSA.PrecisionFactorVDOP = val;
						index++;
						ch = '*';  //最后遇到*结束
						break;
					case 6:  //不知道定义，可能是定位类型
						break;
					default:
						break;
				}
				
				i = 0;
//				index++;
			}
			else
			{
				pBuf[i] = *src;  //取分隔符之间的数字串
				i++;
			}
		}
		else  //没遇到分隔符
		{
			if (*src == ',')  //遇到下一个分隔符了
			{
				sbit = 1;
			}
		}
		src++;
	}
	
	if (info->GSA.Type > 1)  //定位有效
	{
		// return 0;
	}
	
	// return -1;
	return err;
}

/**
  *****************************************************************************
  * @Name   : GNSS GSV数据帧解码
  *
  * @Brief  : none
  *
  * @Input  : *src:   GSV数据帧字符串
  *           Length: GSV数据帧长度
  *
  * @Output : *info:  数据输出结构指针
  *
  * @Return : 0:  解码正确
  *           -1: 非法数据帧
  *****************************************************************************
**/
int gsv_count = 0;
int GNSS_ParseGSV(GNSS_Info_TypeDef * info, const char *src)
{
	/*!< 数据帧格式说明
	 * $GPGSV,<1>,<2>,<3>,<1-4>,<1-5>,<1-6>,<1-7>,<2-4>,<2-5>,<2-6>,<2-7>,<3-4>,<3-5>,<3-6>,<3-7>,<4-4>,<4-5>,<4-6>,<4-7>,<20>*hh<CR><LF>
	 * <1>: 本次总的GSV语句电文数（01-03）
	 * <2>: 当前GSV语句号（01-03）
	 * <3>: 当前可视卫星总数（00-12）
	 * <x-4>: PRN码（伪随机噪声码）　也可以认为是卫星编号（01-32）
	 * <x-5>: 卫星仰角(00～90度)
	 * <x-6>: 卫星方位角(000～359度)
	 * <x-7>: 信噪比(00～99dB Hz)
	 * <20>: 
	 * hh:   校验值
	 *
	 * 例如: $GPGSV,4,1,14,01,72,142,25,03,11,155,,07,74,255,,08,35,031,,0*69
             $GPGSV,4,2,14,09,14,216,25,11,80,010,,16,07,101,,17,14,253,,0*66
             $GPGSV,4,3,14,18,63,066,26,22,18,134,,23,04,182,,27,06,051,,0*69
             $GPGSV,4,4,14,28,18,313,,30,48,308,,0*66
	*/
	
	int total = 0, cnt = 0, ts = 0, i = 0, err = -1;
	uint8_t sbit = 0;  //遇到分隔符标志
	uint8_t index = 0;
	uint8_t pBuf[32];

	if (src == NULL || *src != '$')
	{
		return err;
	}
	else
	{
		err = 0;
	}
	
	//循环解析数据帧
	while (*src != '\0')
	{
		if (sbit != 0)  //遇到分隔符了，取分隔符后面的数字串
		{
			if (*src == ',')  //遇到下一个分隔符了
			{
				pBuf[i] = '\0';  //添加结束符
				total = 0;
				ts = 0;
				
				switch (index)
				{
					case 0:  //本次总的GSV语句电文数（01-03）
						if (strlen((const char *)pBuf) != 0)
						{
							total = get_int_value((const char *)pBuf);
						}
						info->GSV.Total = total;
						break;
					case 1:  //当前GSV语句号（01-03）
						if (strlen((const char *)pBuf) != 0)
						{
							cnt = get_int_value((const char *)pBuf);
							if (cnt == 1)  //第一条
							{
								gsv_count = 0;
							}
						}
						break;
					case 2:  //当前可视卫星总数（00-12）
						if (strlen((const char *)pBuf) != 0)
						{
							ts = get_int_value((const char *)pBuf);
						}
						info->GSV.TotalSatellite = (uint16_t)ts;
						break;
						
						//得到卫星相关数据
					case 3:  //PRN
						if (strlen((const char *)pBuf) != 0)
						{
							#if GNSS_GSV_NEW_SLMSG
							info->GSV.slmsg[gsv_count].PRNCode = (uint8_t)get_int_value((const char *)pBuf);
							#else
							info->GSV.PRNCode[gsv_count] = (uint8_t)get_int_value((const char *)pBuf);
							#endif
						}
						else
						{
							#if GNSS_GSV_NEW_SLMSG
							info->GSV.slmsg[gsv_count].PRNCode = 0;
							#else
							info->GSV.PRNCode[gsv_count] = 0;
							#endif
						}
						break;
					case 4:  //仰角
						if (strlen((const char *)pBuf) != 0)
						{
							#if GNSS_GSV_NEW_SLMSG
							info->GSV.slmsg[gsv_count].SatelliteElevationAngle = (uint8_t)get_int_value((const char *)pBuf);
							#else
							info->GSV.SatelliteElevationAngle[gsv_count] = (uint8_t)get_int_value((const char *)pBuf);
							#endif
						}
						else
						{
							#if GNSS_GSV_NEW_SLMSG
							info->GSV.slmsg[gsv_count].SatelliteElevationAngle = 0;
							#else
							info->GSV.SatelliteElevationAngle[gsv_count] = 0;
							#endif
						}
						break;
					case 5:  //方位角
						if (strlen((const char *)pBuf) != 0)
						{
							#if GNSS_GSV_NEW_SLMSG
							info->GSV.slmsg[gsv_count].SatelliteAzimuth = (uint16_t)get_int_value((const char *)pBuf);
							#else
							info->GSV.SatelliteAzimuth[gsv_count] = (uint16_t)get_int_value((const char *)pBuf);
							#endif
						}
						else
						{
							#if GNSS_GSV_NEW_SLMSG
							info->GSV.slmsg[gsv_count].SatelliteAzimuth = 0;
							#else
							info->GSV.SatelliteAzimuth[gsv_count] = 0;
							#endif
						}
						break;
					case 6:  //信噪比
						if (strlen((const char *)pBuf) != 0)
						{
							#if GNSS_GSV_NEW_SLMSG
							info->GSV.slmsg[gsv_count].SignalNoiseRatio = (uint8_t)get_int_value((const char *)pBuf);
							#else
							info->GSV.SignalNoiseRatio[gsv_count] = (uint8_t)get_int_value((const char *)pBuf);
							#endif
						}
						else
						{
							#if GNSS_GSV_NEW_SLMSG
							info->GSV.slmsg[gsv_count].SignalNoiseRatio = 0;
							#else
							info->GSV.SignalNoiseRatio[gsv_count] = 0;
							#endif
						}
						
						//判断是否还有
						if ((total - cnt) != 0)
						{
							//还有
							gsv_count++;
							index = 0;
						}
						else
						{
							//没有了
							gsv_count = 0;
						}
						break;
					default:
						break;
				}
				
				i = 0;
				index++;
			}
			else
			{
				pBuf[i] = *src;  //取分隔符之间的数字串
				i++;
			}
		}
		else  //没遇到分隔符
		{
			if (*src == ',')  //遇到下一个分隔符了
			{
				sbit = 1;
			}
		}
		src++;
	}
	
	return err;
}








/**
  *****************************************************************************
  * @Name   : GNSS数据帧解析
  *
  * @Brief  : none
  *
  * @Input  : *src:   数据帧字符串
  *           Length: 数据帧长度
  *
  * @Output : *info:  数据输出结构指针
  *
* @Return : 0: 成功
*           -1: 非法数据帧
*           -2: 校验失败
  *****************************************************************************
**/
int GNSS_Analysis(GNSS_Info_TypeDef * info, const char *src, uint32_t Length)
{
	int err = -1, i = 0, check_err = 1;
	uint32_t len = Length, cnt = 0;//, tt = 0;
	uint8_t ch = 0;
	uint8_t uBuf[128], cmp[3];
	
	if (src == NULL || *src != '$')
	{
		return err;
	}
	else
	{
		err = 0;
	}
	
	//循环读取，直到最后
	while (len)
	{
		if (*src != 0x0D)  //\r
		{
			uBuf[cnt] = *src;  //拷贝字符串
			cnt++;
			src++;
		}
		else  //遇到\r了
		{
			src++;
			if (*src == 0x0A)  //\n
			{
				src++;
				uBuf[cnt++] = '\0';  //添加结束符
				
//				for (tt = 0; tt < cnt; tt++)
//				{
//					xy_printf ("%c", uBuf[tt]);
//				}
//				xy_printf ("\r\n");
				
				//开始解析数据
				if (uBuf[0] != '$')
				{
					err = -2;  //非法数据帧
				}
				else
				{
					check_err = GNSS_CheckDataInvalid((const char *)uBuf);
					if (check_err != 0)  //校验正确
					{
						ch = uBuf[2];
						if (ch == 'P' || ch == 'p')
						{
							info->IsHybrid = 0;
						}
						else if (ch == 'N' || ch == 'n')
						{
							info->IsHybrid = 1;
						}
						else
						{
							info->IsHybrid = 0;
						}
						
						//获取关键信息字符
						for (i = 0; i < 3; i++)
						{
							cmp[i] = uBuf[i + 3];
						}
						
						ch = cmp[2];
						if (cmp[1] == 'S')
						{
							//GSV or GSA
							
							switch (ch)
							{
								case 'A':  //GSA
									err = GNSS_ParseGSA(info, (const char *)uBuf);
									if (err == 0)
									{
										xy_printf ("Is GSA Frame.");
									}
									break;
								case 'V':  //GSV
									err = GNSS_ParseGSV(info, (const char *)uBuf);
									if (err == 0)
									{
										xy_printf ("Is GSV Frame.");
									}
									break;
								default:
									break;
							}
						}
						else
						{
							//GGA or RMC or VTG
							
							switch (ch)
							{
								case 'A':  //GGA
									err = GNSS_ParseGGA(info, (const char *)uBuf);
									if (err == 0)
									{
										xy_printf ("Is GGA Frame.");
									}
									break;
								case 'C':  //RMC
									err = GNSS_ParseRMC(info, (const char *)uBuf);
									if (err == 0)
									{
										xy_printf ("Is RMC Frame.");
									}
									break;
								case 'G':  //VTG
									err = GNSS_ParseVTG(info, (const char *)uBuf);
									if (err == 0)
									{
										xy_printf ("Is VTG Frame.");
									}
									break;
								default:
									break;
							}
							
						}
					}  //End for GNSS_CheckDataInvalid()
				}
				
				len -= (cnt + 1);
				cnt = 0;
			}
			else
			{
				//不是回车换行，补上丢掉的数据
				uBuf[cnt++] = 0x0D;
			}
		}
	}
	
	if (check_err == 0)
	{
		err = -2;
	}
	
	return err;
}
