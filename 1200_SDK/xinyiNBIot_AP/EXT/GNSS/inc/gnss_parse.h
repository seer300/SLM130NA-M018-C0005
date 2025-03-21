#ifndef _gnss_parse_h
#define _gnss_parse_h

/**
  ****************************** Support C++ **********************************
**/
#ifdef __cplusplus
extern "C"{
#endif		
/**
  *****************************************************************************
**/


/******************************************************************************
                               外部函数头文件
******************************************************************************/

#include <string.h>
#include "xy_system.h"
#include "xy_printf.h"
#include "urc_process.h"
#include "xy_cp.h"
#include "xy_lpm.h"
#include "xy_at_api.h"
#include "xy_memmap.h"
#include "hal_gpio.h"
#include "hal_csp.h"
#include "gnss_api.h"
#include "at_uart.h"


#define get_float_value( str )                   atof((const char *)str)
#define get_int_value( str )                     atoi((const char *)str)

/******************************************************************************
                                   变量定义
******************************************************************************/



/******************************************************************************
                             定义GPS数据帧信息结构
******************************************************************************/

// 日期信息
typedef struct
{
	uint16_t Year;
	uint8_t Month;
	uint8_t Date;
} GNSS_Dates_TypeDef;

// 时间信息
typedef struct
{
	uint16_t mSecond;
	uint16_t Reserved1;
	
	uint8_t Reserved2;
	uint8_t Hour;
	uint8_t Minute;
	uint8_t Second;
} GNSS_Times_TypeDef;

// 坐标信息(经纬度)
typedef struct
{
    double Value;
    double Second;  // 秒
    uint32_t Degree;  // 度
    uint32_t Minute;  // 分
} GNSS_FormatDMS_TypeDef;

typedef struct
{
    GNSS_FormatDMS_TypeDef Longitude;  //经度
	GNSS_FormatDMS_TypeDef Latitude;  //纬度

	double LongitudeOriginal;  //经度原始值
	double LatitudeOriginal;  //纬度原始值
	
	uint16_t IsNorth;  //是否北纬
	uint16_t IsEast;  //是否东经
} GNSS_Position_TypeDef;

// ==================== RMC信息 ====================
typedef struct
{
	//UTC时间信息
	GNSS_Dates_TypeDef Date;
	GNSS_Times_TypeDef Time;
	
	//经纬度信息
	GNSS_Position_TypeDef Position;
	
	//地面速率
	double GroundRateKnots;
	double GroundRateKmh;
	
	//正北航向
	double NorthAirlines;
	
	//磁偏角
	double MagneticDeclination;
	
	//模式
	uint16_t Mode;  //0=数据无效，1=自主定位，2=差分，3=估算
	
	uint8_t IsMDEast;  //磁偏角方向，E(东)或W(西)
	uint8_t IsEffectivePositioning;  //是否定位有效；0=无效；1=有效
} GNSS_RMC_TypeDef;

// ==================== GGA信息 ====================

typedef struct
{
	//UTC时间信息
	GNSS_Times_TypeDef Time;
	
	//经纬度信息
	GNSS_Position_TypeDef Position;
	
	//海拔高度
	double AltitudeHeight;  //海拔，单位：米
	
	//地球椭圆面相对大地水准面的高度
	double LevelHeight;  //单位：米
	
	//HDOP水平精度因子（0.5-99.9）
	double PrecisionFactorHDOP;
	
	//差分GPS数据期限(RTCMSC-104)，最后设立RTCM传送的秒数量；差分时间（从最近一次接收到差分信号开始的秒数，如果不是差分定位将为空）
	double DifferentialTime;
	
	//差分参考基站标号，从0000到1023(首位0也将传送)；差分站ID 0000-1023
	uint16_t DifferentialStationID;
	
	uint8_t GPS_State;  //GPS状态
	uint8_t GPS_Number;  //使用卫星数量
} GNSS_GGA_TypeDef;

// ==================== VTG信息 ====================

typedef struct
{
	//正北航向
	double NorthAirlines;
	
	//磁北航向
	double MagneticNorthAirlines;
	
	//地面速率1
	double GroundRateKnots;
	
	//地面速率2
	double GroundRateKmh;
	
	//模式
	uint32_t Mode;  //0=数据无效，1=自主定位，2=差分，3=估算
} GNSS_VTG_TypeDef;

// ==================== GSA信息 ====================

typedef struct
{
	//PDOP综合位置精度因子（0.5 - 99.9）
	double PrecisionFactorPDOP;
	
	//HDOP水平精度因子（0.5 - 99.9）
	double PrecisionFactorHDOP;
	
	//VDOP垂直精度因子（0.5 - 99.9）
	double PrecisionFactorVDOP;
	
	uint16_t Reserved;
	
	//定位模式，0=自动手动2D/3D，1=手动2D/3D
	uint8_t Mode;
	
	//定位类型，1=未定位，2=2D定位，3=3D定位
	uint8_t Type;
	
	//PRN码（伪随机噪声码）　也可以认为是卫星编号
	uint8_t PRNCode[12];
} GNSS_GSA_TypeDef;

// ==================== GSV信息 ====================

#define GNSS_GSV_NEW_SLMSG                       ( 0 )

#define GNSS_GSV_NUMBER                          ( 32 )  //4的倍数

#if GNSS_GSV_NEW_SLMSG
//卫星信息
typedef struct
{
	//卫星方位角(000～359度)
	uint16_t SatelliteAzimuth;
	uint16_t Reserved1;
	uint8_t  Reserved2;
	
	//PRN码（伪随机噪声码）　也可以认为是卫星编号
	uint8_t PRNCode;
	//卫星仰角(00～90度)
	uint8_t SatelliteElevationAngle;
	//信噪比(00～99dB Hz)
	uint8_t SignalNoiseRatio;
} GNSS_Satellite_Info;
#endif

typedef struct
{
    #if GNSS_GSV_NEW_SLMSG
	//卫星信息
	GNSS_Satellite_Info slmsg[GNSS_GSV_NUMBER];
	#endif

	//总的GSV语句电文数
	uint16_t Total;
	
	//当前可视卫星总数（00-12）
	uint16_t TotalSatellite;
	
    #if !GNSS_GSV_NEW_SLMSG
	//卫星方位角(000～359度)
	uint16_t SatelliteAzimuth[GNSS_GSV_NUMBER];
	
	//PRN码（伪随机噪声码）　也可以认为是卫星编号
	uint8_t PRNCode[GNSS_GSV_NUMBER];
	
	//卫星仰角(00～90度)
	uint8_t SatelliteElevationAngle[GNSS_GSV_NUMBER];
	
	//信噪比(00～99dB Hz)
	uint8_t SignalNoiseRatio[GNSS_GSV_NUMBER];
    #endif
} GNSS_GSV_TypeDef;


// ==================== GPS信息汇总 ====================

typedef struct
{
	GNSS_RMC_TypeDef RMC;
	GNSS_GGA_TypeDef GGA;
	GNSS_VTG_TypeDef VTG;
	GNSS_GSA_TypeDef GSA;
	GNSS_GSV_TypeDef GSV;
	
	uint16_t IsHybrid;  //混合定位模式，0：GPS定位；1：GPS+其他
	uint16_t IsDecodeFinish;  //定位信息解码完成
} GNSS_Info_TypeDef;


/******************************************************************************
                               外部调用功能函数
******************************************************************************/

extern GNSS_Info_TypeDef gnss_datas;


int GNSS_CheckDataInvalid(const char *src);
void GNSS_LatitudeLongitudeConversion(double Value, double *pValue, uint32_t *Degree, uint32_t *Cent, double *Second);

int GNSS_ParseGGA(GNSS_Info_TypeDef * info, const char *src);
int GNSS_ParseVTG(GNSS_Info_TypeDef * info, const char *src);
int GNSS_ParseRMC(GNSS_Info_TypeDef * info, const char *src);
int GNSS_ParseGSA(GNSS_Info_TypeDef * info, const char *src);
int GNSS_ParseGSV(GNSS_Info_TypeDef * info, const char *src);

int GNSS_Analysis(GNSS_Info_TypeDef * info, const char *src, uint32_t Length);


/**
  ****************************** Support C++ **********************************
**/
#ifdef __cplusplus
}
#endif
/**
  *****************************************************************************
**/


#endif
