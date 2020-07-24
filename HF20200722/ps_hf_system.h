/*
文件：		ps_hf_system.h
内容：		定义基本系统信息
更新记录：
-- Ver 1.0 / 2012-06-20
*/

#ifndef PS_SYSTEM_H_
#define PS_SYSTEM_H_

//定义通用返回值
#define SYS_OK 0
#define SYS_ER -1

//定义系统信息宏
#define INFO_NAME "Tokamak Power Supply Control Kit "
#define INFO_VERSION "0.1.0"
#define INFO_DATE "Jan. 2012"
#define INFO_COPYRIGHT "He Yang, J-TEXT Lab. All Rights Reserved."

//定义系统状态（基于J-TEXT CODAC状态标准）
#define STAT_IDLE 0
#define STAT_READY_COLD 1
#define STAT_READY_HOT 2
#define STAT_RUN 15
#define STAT_ALARM 240
#define STAT_FINISH 241
#define STAT_BUSY 242
#define STAT_OFFLINE 254
#define STAT_ERROR 255
#define STAT_UNKOWN 256


//定义编译需要的实参量宏
#define PARAM_TIME_SCALE 3500	//默认的系统运行最大时间尺度

//标准错误代码
#define ERR_FATAL -1			//严重错误
#define ERR_NONE 0				//无错误
#define ERR_MEMORY 1			//内存分配错误
#define ERR_THREAD 2			//线程创建错误
#define ERR_TIMER 3				//定时器错误
#define ERR_INVD 5				//非法操作
#define ERR_PARAM 9				//参数错误

#define ERR_NET 10				//网络错误
#define ERR_NET_INIT 11			//网络初始化错误
#define ERR_NET_LOCAL 12		//本地端口绑定错误
#define ERR_NET_REMOTE 13		//远程主机设置错误
#define ERR_NET_OVERFLOW 14		//消息缓存溢出

#define ERR_CMD 20				//命令错误
#define ERR_CMD_NAME 21			//非法的命令名称
#define ERR_CMD_FORMAT 22		//非法的命令格式
#define ERR_CMD_STAT 23			//非法的命令状态

#define ERR_DEV 30				//设备错误
#define ERR_DEV_AN_IN 31		//模拟采集设备错误
#define ERR_DEV_AN_OUT 32		//模拟输出设备错误
#define ERR_DEV_DG 33			//数字IO设备错误

#define ERR_FILE 40				//文件错误
#define ERR_FILE_OPEN 41		//不能打开文件
#define ERR_FILE_WRTE 42		//不能写入文件

//标准故障代码
//说明：故障代码使用不同数据位来表示不同的故障
#define FT_NONE 0
#define FT_BUSBAR_TEMPERATURE_HIGH 1		   //母线温度过高
#define FT_FUSE_BREAK 2							//熔丝断了
#define FT_RECTIFIER_CURRENT_OVERLOAD 4    //整流器过流
#define PLASMA_CURRENT_OVERLOAD	8		//IP过流

#endif
