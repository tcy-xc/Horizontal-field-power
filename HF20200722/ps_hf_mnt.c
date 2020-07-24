/*
文件：		ps_hf_mnt.c
内容：		Monitor对象接口函数的实现
更新记录：
-- Ver 1.0 / 2012-06-20
*/

#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include "ps_hf_mnt.h"
#include "ps_hf_lib.h"

//私有函数
static void *s_MntThread(void *pvObj);			//监控线程
static int s_MntChkStat(ps_mnt_t *pstObj);		//检查输入数字量的状态

/*
名称：		PsMntInit
功能：		初始化
返回值：		标准错误代码
*/
int PsMntInit(ps_mnt_t *pstObj)
{
	struct sched_param stParam;
	pthread_attr_t stAttr;
	
	//初始化相关变量
	pstObj->p_stIn.wValue = 0;
	pstObj->p_stOut.wValue = 0;
	pstObj-> p_lFault=0;				
	pstObj->lExternalFault=0;			
	pstObj->lTimeFault=0;	
	pstObj->falut_timer_once_flag = 0;	
	//指定定时器
	pstObj->p_pstTimer = &pstObj->pstIO->stConf.r_stDgTimer;
	
	//设置线程属性: detached, FIFO, prio
	pthread_attr_init(&stAttr);
	pthread_attr_setdetachstate(&stAttr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setinheritsched(&stAttr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&stAttr, SCHED_FIFO);
	stParam.sched_priority = pstObj->iPrio;
	pthread_attr_setschedparam(&stAttr, &stParam);
	
	//创建监控线程
	pthread_create(&pstObj->p_stThreadId, &stAttr, s_MntThread, pstObj);
	if(pstObj->p_stThreadId == NULL)
	{
		return ERR_THREAD;
	}
	
	//设置IO刷新时间，重置lTimeInterval
	PsMntSetStat(pstObj, STAT_IDLE);
	
	return ERR_NONE;
}

/*
名称：		PsMntSetStat
功能：		根据给定状态对Monitor进行对应转换
注意：		本函数不验证转换的合理性，请用户自行注意
返回值：		iStatus
*/
int PsMntSetStat(ps_mnt_t *pstObj, int iStatus)
{
	switch(iStatus)
	{
		case STAT_READY_HOT:
		//缩短扫描时间间隔，增加精确性
		pstObj->lTimeInterval=PsIODgSetInterval(pstObj->pstIO, pstObj->lTimeFast);
		break;
		
		case STAT_RUN:
		case STAT_IDLE:
		case STAT_ERROR:
		case STAT_READY_COLD:
		pstObj->lTimeInterval=PsIODgSetInterval(pstObj->pstIO, pstObj->lTimeNormal);
		break;
		
		default:
		break;
	}
	
	return iStatus;
}

/*
名称：		s_MntThread
功能：		用于监测PCI1750上的故障信号和触发信号。同时输出内部故障信号和脉冲封锁信号
返回值：		NULL
*/
static void *s_MntThread(void *pvObj)
{
	ps_mnt_t *pstObj = pvObj;
	unsigned s;
	
	//要求锁定定时器
	TimerLock(pstObj->p_pstTimer);

	for(; ; )
	{
		//等待定时器信号
		if(TimerWait2(pstObj->p_pstTimer) == TIMER_ER)
		{
			PsSetError(ERR_FATAL);
		}

		//致命错误处理
		if(PsGetError() == ERR_FATAL)
		{
			fprintf(stderr, "ERROR: FATAL ERROR.\n");
			PsFatalError();
			break;
		}
		//读取数字量输入
		s = g_stIO.r_stDgInValue.wValue;
		//只有状态改变时才重新检查
		if(s != pstObj->p_stIn.wValue)
		{
			printf("DIO = %x\n",s);
			pstObj->p_stIn.wValue = s;
			s_MntChkStat(pstObj);
		}

		//当有内部故障或外部故障时，错误计时
		if( (PsGetFault()>0)||(pstObj->lExternalFault>0))
		{
			pstObj->lTimeFault= pstObj->lTimeFault+pstObj->lTimeInterval;
		}
				
		//当错误出现1s，关闭脉冲使能信号
		if(pstObj->lTimeFault>1000000000)
		{
			if(pstObj->falut_timer_once_flag == 0)//add by ly @ 20170331
			{
				printf("!!!!!!!!!!!!!!!!!!!!!!!!!!MNTlTimeFault>1000000000!!!!!!!!!!!!!!!!!!!\n");//20151124 ly
				pstObj->falut_timer_once_flag = 1;
				printf("external fault = %d, internal fault = %ld\n", pstObj->lExternalFault, PsGetFault());
			}
			//消除脉冲使能信号
			pstObj->pstIO->w_stDgOutValue.xB0 = PCI1750_OUT_HI;
			pstObj->pstIO->w_stDgOutValue.xB1 = PCI1750_OUT_HI;
			PsIODgRefresh(pstObj->pstIO);
			pstObj->lTimeFault=0;
		}
		
//		printf("Vhf = %f, Vac = %f, Ip = %f\n",pstObj->pstIO->r_adAnInValue[2], pstObj->pstIO->r_adAnInValue[0], pstObj->pstIO->r_adAnInValue[4]);
		
	}
	
	TimerUnlock(pstObj->p_pstTimer);
	return NULL;
}

/*
名称：		s_MntChkStat
功能：		检察输入状态并进行相应操作
返回值：		SYS_OK
*/
static int s_MntChkStat(ps_mnt_t *pstObj)
{
	
	//清除最近的故障记录
	pstObj->p_lFault = FT_NONE;



	//检测内部故障,相应位为0表示故障
	if(pstObj->p_stIn.xB0==PCI1750_IN_LO)//1750-0,母线温度过高
	{
		pstObj->p_lFault += FT_BUSBAR_TEMPERATURE_HIGH;
		printf("Busbar Temperature high!\n");
	}

	if(pstObj->p_stIn.xB1==PCI1750_IN_LO)//1750-1,熔丝断了
	{
		printf("Fuse Break!\n");
		pstObj->p_lFault += FT_FUSE_BREAK;
	}
		
	if(pstObj->p_lFault>0)
	{
		//当设备发生故障时应当停止运行。此时先产生一个内部故障信号，并清除故障信号
		pstObj->pstIO->w_stDgOutValue.xB2 = PCI1750_OUT_LO;
		PsIODgRefresh(pstObj->pstIO);
		PsSetStatus(STAT_ERROR);
		fprintf(stderr, "FAULT: INTERNAL FAULT.\n");
	}

	//如果没有故障发生，则清除“内部故障”信号
	if(pstObj->p_lFault == FT_NONE)
	{
		pstObj->pstIO->w_stDgOutValue.xB2 = PCI1750_OUT_HI;
		PsIODgRefresh(pstObj->pstIO);
	}
	
	//设置故障代码，同时保留以前的故障代码
	PsSetFault((pstObj->p_lFault |PsGetFault()));
/*
	//外部故障，相应位为1表示故障
	if(pstObj->p_stIn.xB3 == PCI1750_IN_HI)
	{
		//由于故障与本机无关，将系统转入空闲状态，并输出信息
		pstObj->lExternalFault=1;
		PsSetStatus(STAT_IDLE);
		fprintf(stderr, "FAULT: EXTERNAL FAULT.\n");	
	}
*/
	//检测触发信号，相应位为1表示触发
	if(pstObj->p_stIn.xB4 == PCI1750_IN_HI && PsGetStatus() == STAT_READY_HOT)
	{
		//设置整流器1脉冲使能信号(整流器2不使能)
		pstObj->pstIO->w_stDgOutValue.xB0 = PCI1750_OUT_LO;
		pstObj->pstIO->w_stDgOutValue.xB1 = PCI1750_OUT_HI;
		PsIODgRefresh(pstObj->pstIO);
		PsSetStatus(STAT_RUN);
		printf("*****Discharge Start!*****\n");
	}
	
	
	return SYS_OK;
}

/*
名称：		PsMntReset
功能：		初始化mnt相关变量
返回值：		SYS_OK
*/
int PsMntReset(ps_mnt_t *pstObj)
{
		//清空输入输出记录,初始化变量
		pstObj->p_stIn.wValue = 0;
		pstObj->p_stOut.wValue = 0;
		pstObj-> p_lFault=0;				
		pstObj->lExternalFault=0;			
		pstObj->lTimeFault=0;		
		pstObj->falut_timer_once_flag = 0;	
			
		//消除脉冲使能信号
		pstObj->pstIO->w_stDgOutValue.xB1 = PCI1750_OUT_HI;
		pstObj->pstIO->w_stDgOutValue.xB2 = PCI1750_OUT_HI;
		PsIODgRefresh(pstObj->pstIO);

		//设置数字量IO刷新时间
		PsMntSetStat(pstObj, STAT_IDLE);

		return SYS_OK;
}

