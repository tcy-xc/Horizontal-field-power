/*
文件：		ps_hf_mnt.h
内容：		定义Monitor对象的数据结构与接口函数
更新记录：
-- Ver 1.0 / 2012-6-20
*/

#ifndef PS_MNT_H_
#define PS_MNT_H_

#include <pthread.h>
#include "pci1750.h"
#include "timer.h"
#include "ps_hf_system.h"
#include "ps_hf_io.h"

//Monitor对象数据结构
typedef struct PsMonitorTag
{
	int iPrio;					//监控线程的优先级
	long lTimeFast;				//定时间隔参数/快/用于STAT_READY
	long lTimeNormal;			//定时间隔参数/普通
	ps_io_t *pstIO;				//数据IO对象指针，用于数据访问
	//以上参数要求在Monitor对象初始化以前完成初始化，并不再随意改动
	
	pci1750_val_t p_stIn;		//最近的输入
	pci1750_val_t p_stOut;		//最近的输出（未使用）
	long p_lFault;				//最近的故障
	
	int lExternalFault;			//外部故障信号
	long lTimeFault;			//错误时间
	long lTimeInterval;		//定时间隔
	int falut_timer_once_flag;
	stimer_t *p_pstTimer;		//与IO对象共享的时钟指针
	pthread_t p_stThreadId;		//监控线程的ID
} ps_mnt_t;

//接口函数
int PsMntInit(ps_mnt_t *pstObj);
int PsMntSetStat(ps_mnt_t *pstObj, int iStatus);
int PsMntReset(ps_mnt_t *pstObj);

/*
说明：
PsMntInit		初始化Monitor对象
PsMntSetStat	将Monitor置为iStauts对应的状态
PsMntReset		初始化Monitor相关变量
*/

#endif
