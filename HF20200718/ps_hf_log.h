/*
文件：		ps_hf_log.h
内容：		定义Log对象数据结构和接口函数，用于记录所需信息
更新记录：
-- Ver 0.9 / 2013-04-13
*/
#ifndef PS_LOG_H_
#define PS_LOG_H_

#include "ps_hf_system.h"
#include "pci1750.h"
#include "ps_hf_io.h"

//定义常参量宏
#define PS_LOG_NOLOG 0			//不记录
#define PS_LOG_FILE 1			//记录到文件
#define PS_LOG_STDO 2			//记录到标准输出设备
#define PS_LOG_ALL 9			//记录到以上所有

//定义Log记录数据结构
typedef struct PsLogDataTag
{
	double Ip;					//等离子体电流
	double DY_set;			//等离子体垂直位移设定值
	double DY_real;		//等离子体垂直位移实际值
	double rogowski;		//罗柯线圈信号
	double saddle;			//鞍型线圈信号
	double Ihf_set;				//HF整理器输出电流设定值
	double Ihf_real;			//HF整流器输出电流实际值
	double Uhf_ac[2];			//HF整流器输入端电压值
	double Uhf_dc;				//HF整流器输出端电压值
	double angle[2];			//HF整流器控制角
	double Ihf_err_p;			//HF电流P值，没乘kp系数之前
	double Ihf_err_i;			//HF电流I值，没乘ki系数之前
	double Ihf_err_d;			//HF电流D值，没乘kd系数之前
	double DY_err_p;			//垂直位移P值，没乘kp系数之前
	double DY_err_i;			//垂直位移I值，没乘ki系数之前
	double DY_err_d;			//垂直位移D值，没乘kd系数之前
	double I_DRMP_DC2;			//RMP current
	double Ihf_feedback_init;	//at 2016.5.16 by zpchen

	long time;
	pci1750_val_t p_stIn;		//最近的输入
	pci1750_val_t p_stOut;		//最近的输出
} ps_logdata_t;

//定义Log对象数据结构
typedef struct PsLogTag
{
	int iLogLevel;				//记录等级
	char szFilePath[128];		//记录文件路径
	void *pvPsor;				//一个Log对象要求绑定到一个Processor对象
	long lIndex;				//记录数索引
	int iIdxIntv;				//输出到stdout时lIndex的间隔
	ps_logdata_t astData[PARAM_TIME_SCALE];//记录的数据
	
} ps_log_t;

//接口函数
int PsLogAdd(ps_log_t *pstObj);
int PsLogOut(ps_log_t *pstObj);
//int PsLogAddSysInf(ps_log_t *pstObj);

/*
说明：
PsLogAddData		添加一条记录
PsLogOut		输出记录
PsLogAddSysInf 将系统信息写入文件
*/

#endif
