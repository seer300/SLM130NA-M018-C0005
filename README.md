# SLM130NA-M018-C0005
NB-iot模块 SLM130NA   
使用芯翼XY1200S平台开发

## 首次编译前准备
1、请前往本项目 [Release页面](https://github.com/seer300/SLM130NA-M018-C0005/releases/tag/buildtools) 下载编译工具包（芯翼提供）  
解压到`1200_SDK`目录下

2、进入`exclusive-tools`目录，解压好相关开发软件

## 开始编译
### Windows平台
进入`1200_SDK`目录  
运行`xybuild.bat all`开始完整编译  
运行`xybuild.bat clean`清理编译环境  

编译成功后，`1200_SDK/Allbins`目录下即为编译产物

## 打包固件
在`1200_SDK`目录下，运行`cp_AllbinsF.bat`脚本  

该脚本会将`Allbins`下的`arm.img cp.img loginfo.info`三个文件拷贝到`V1200FB10002R00C0005P02\allbins`目录下  

启动LogView软件，执行打包流程。路径选择`V1200FB10002R00C0005P02\allbins`目录即可

## 分支说明
### master
M018 默认分支，芯翼1200s海外默认版本  
### 130NA-S  
西班牙警示灯项目 M021 分支（HT-BC100X-EU）, 外挂了华大北斗HD8125 GNSS定位芯片。  
移植了来自XY3100的GNSS支持代码。  

## 补丁合入记录
### 2025年3月10日 SDK_V1200FB10002R00C0005P02
| 修改模块 | 修改说明 |
| ----------- | ----------- |
| 平台 | OPEN形态下，3GPP断电保存方案处理优化 |
| MODEM | 针对漫游网络场景，NV添加PLMN配置项，客户配置后可在搜网时一起搜：ucEhplmnNum 数量 aucEhplmn 具体PLMN |