/*
文件：		ps_hf_io.c
内容：		IO对象接口函数实现
更新记录：
-- Ver 1.0 / 2012-06-20
*/

#include <stdio.h>
#include "ps_hf_io.h"
#include "ps_hf_lib.h"

//定义常参量宏
#define PARAM_INTERVAL 1000000

//私有函数
static void *s_AnThread(void *pvObj);			//模拟量刷新线程
static void *s_DgThread(void *pvObj);			//数字量刷新线程
static int s_DevConfig(ps_io_t *pstObj);		//配置设备


/*
名称：		PsIOAnInRefresh
功能：		立刻刷新模拟量输入
注意		使用“inline”特性需开启O2优化级别，下同
返回值：	SYS_OK
*/
inline int PsIOAnInRefresh(ps_io_t *pstObj)
{
	Pci1713GetDouble(&pstObj->stConf.r_stAnInDev,
					pstObj->r_adAnInValue + pstObj->stConf.r_stAnInDev.byChl);
	return SYS_OK;
}

/*
名称：		PsIOAnOutRefresh
功能：		立刻刷新模拟量输出
返回值：	SYS_OK
*/
inline int PsIOAnOutRefresh(ps_io_t *pstObj)
{
	Pci1720SetDouble(&pstObj->stConf.r_stAnOutDev, 0, pstObj->w_adAnOutValue[0]);
	Pci1720SetDouble(&pstObj->stConf.r_stAnOutDev, 1, pstObj->w_adAnOutValue[1]);
//	Pci1720SetDouble(&pstObj->stConf.r_stAnOutDev, 2, pstObj->w_adAnOutValue[2]);
//	Pci1720SetDouble(&pstObj->stConf.r_stAnOutDev, 3, pstObj->w_adAnOutValue[3]);
	return SYS_OK;
}

/*
名称：		PsIODgRefresh
功能：		立刻刷新数字量输入输出值
返回值：	SYS_OK
*/
inline int PsIODgRefresh(ps_io_t *pstObj)
{
	pstObj->r_stDgInValue.wValue = Pci1750Get(&pstObj->stConf.r_stDgIODev);
	Pci1750Set(&pstObj->stConf.r_stDgIODev, pstObj->w_stDgOutValue.wValue);
	
	return SYS_OK;
}

/*
名称：		PsIOAnSetInterval
功能：		设置模拟量输入输出刷新时间间隔，由于系统特性，设置值与实际值可能不同
返回值：	实际的时间间隔
*/
long PsIOAnSetInterval(ps_io_t *pstObj, long lNanoSeconds)
{
	pstObj->stConf.r_lAnInterval = TimerSetInterval2(&pstObj->stConf.r_stAnTimer, 
													lNanoSeconds);
	TimerStart2(&pstObj->stConf.r_stAnTimer);
	
	return pstObj->stConf.r_lAnInterval;
}

/*
名称：		PsIODgSetInterval
功能：		设置数字量输入输出刷新时间间隔，由于系统特性，设置值与实际值可能不同
返回值：	实际的时间间隔
*/
long PsIODgSetInterval(ps_io_t *pstObj, long lNanoSeconds)
{
	pstObj->stConf.r_lDgInterval = TimerSetInterval2(&pstObj->stConf.r_stDgTimer, 
													lNanoSeconds);
	TimerStart2(&pstObj->stConf.r_stDgTimer);
	
	return pstObj->stConf.r_lDgInterval;
}

/*
名称：		PsIOAnReset
功能：		重置模拟量输出为默认值
返回值：	SYS_OK
*/
int PsIOAnReset(ps_io_t *pstObj)
{
	pstObj->w_adAnOutValue[0] = pstObj->stConf.adDefAnValue[0];
	pstObj->w_adAnOutValue[1] = pstObj->stConf.adDefAnValue[1];
	pstObj->w_adAnOutValue[2] = pstObj->stConf.adDefAnValue[2];
	pstObj->w_adAnOutValue[3] = pstObj->stConf.adDefAnValue[3];
	
	PsIOAnOutRefresh(pstObj);
	
	return SYS_OK;
}

/*
名称：		PsIODgReset
功能：		重置数字量输出为默认值
返回值：	SYS_OK
*/
int PsIODgReset(ps_io_t *pstObj)
{
	pstObj->w_stDgOutValue.wValue = pstObj->stConf.stDefDgValue.wValue;
	
	return SYS_OK;
}

/*
名称：		PsIOInit
功能：		初始化IO对象
Refresh:	标准错误代码
*/
int PsIOInit(ps_io_t *pstObj)
{
	struct sched_param stParam;
	pthread_attr_t stAttr;
	
	//Pci1713
	if(Pci1713Init(&pstObj->stConf.r_stAnInDev, 0) == PCI_ER)
		return ERR_DEV_AN_IN;
	//Pci1720
	if(Pci1720Init(&pstObj->stConf.r_stAnOutDev, 0) == PCI_ER)
		return ERR_DEV_AN_OUT;
	//PCi1750
	if(Pci1750Init(&pstObj->stConf.r_stDgIODev, 0) == PCI_ER)
		return ERR_DEV_DG;
		
	//设置设备通道增益
	s_DevConfig(pstObj);
	
	//重置输出到默认参数
	PsIOAnReset(pstObj);
	PsIODgReset(pstObj);
	
	//创建定时器
	if(TimerInit(&pstObj->stConf.r_stAnTimer) == TIMER_ER)
		return ERR_TIMER;
	if(TimerInit(&pstObj->stConf.r_stDgTimer) == TIMER_ER)
		return ERR_TIMER;
	
	//设置默认间隔
	PsIOAnSetInterval(pstObj, PARAM_INTERVAL);
	PsIODgSetInterval(pstObj, PARAM_INTERVAL);
	
	//设置线程属性: detached, FIFO, prio
	pthread_attr_init(&stAttr);
	pthread_attr_setdetachstate(&stAttr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setinheritsched(&stAttr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&stAttr, SCHED_FIFO);
	stParam.sched_priority = pstObj->stConf.iAnPrio;
	pthread_attr_setschedparam(&stAttr, &stParam);
	
	//创建模拟量IO线程
	pthread_create(&pstObj->stConf.r_stAnThreadId, &stAttr, s_AnThread, pstObj);
	if(pstObj->stConf.r_stAnThreadId == NULL)
		return ERR_THREAD;
	
	//创建数字量IO线程
	stParam.sched_priority = pstObj->stConf.iDgPrio;//优先级有改变
	pthread_attr_setschedparam(&stAttr, &stParam);
	pthread_create(&pstObj->stConf.r_stDgThreadId, &stAttr, s_DgThread, pstObj);
	if(pstObj->stConf.r_stDgThreadId == NULL)
		return ERR_THREAD;
		
	return ERR_NONE;
}

/*
名称：		s_AnThread
功能：		模拟量IO刷新线程
返回值：	NULL
*/
static void *s_AnThread(void *pvObj)
{
	ps_io_t *pstObj = pvObj;
	
	//要求运行时锁定定时器
	TimerLock(&pstObj->stConf.r_stAnTimer);

	for(; ; )
	{
		if(TimerWait2(&pstObj->stConf.r_stAnTimer) == TIMER_ER)
		{
			PsSetError(ERR_FATAL);
			break;
		}
		//刷新数据
		PsIOAnInRefresh(pstObj);
		//PsIOAnOutRefresh(pstObj);

	}
	
	//退出时取消锁定
	TimerUnlock(&pstObj->stConf.r_stAnTimer);
	return NULL;
}

/*
名称：		s_DgThread
功能：		数字量IO刷新线程
返回值：	NULL
*/
static void *s_DgThread(void *pvObj)
{
	ps_io_t *pstObj = pvObj;
	
	//要求运行时锁定定时器
	TimerLock(&pstObj->stConf.r_stDgTimer);
	for(; ; )
	{
		if(TimerWait2(&pstObj->stConf.r_stDgTimer) == TIMER_ER)
		{
			PsSetError(ERR_FATAL);
			break;
		}
		//刷新
		PsIODgRefresh(pstObj);
	}
	//退出时取消锁定
	TimerUnlock(&pstObj->stConf.r_stDgTimer);
	return NULL;
}

/*
名称：		s_DevConfig
功能：		配置PCI设备
返回值：	SYS_OK
在loadfile里面
*/
static int s_DevConfig(ps_io_t *pstObj)
{
	
	int i;
	//PCI1713,除通道号外,设置全部与default中的设置相同
	//设置起始和终止通道号
	(pstObj->stConf).r_stAnInDev.byChl=0;
	(pstObj->stConf).r_stAnInDev.byChh=7;		
	//设置触发方式，0x01
	(pstObj->stConf).r_stAnInDev.uniTrigger.byParam=PCI1713_SET_SW;
	//设置通道增益，双极性-10V ~ + 10V，单端(SinGle EnDed)
	for(i=0 ; i<8 ; i++)	
	{
		(pstObj->stConf).r_stAnInDev.auniChn[i].x5Gain=PCI1713_SET_BP10;
		(pstObj->stConf).r_stAnInDev.auniChn[i].x3Sod=PCI1713_SET_SGED;
	}
	//将配置应用的设备上
	Pci1713Config(&(pstObj->stConf).r_stAnInDev);

	//PCI1720，设置全部与default中的设置相同。
	//设置同步输出位为0
	(pstObj->stConf).r_stAnOutDev.bySync=PCI1720_SET_DI;
	//设置通道增益，单极性 0~5V
	(pstObj->stConf).r_stAnOutDev.uniChn.x2Ch0=PCI1720_SET_UP05;
	(pstObj->stConf).r_stAnOutDev.uniChn.x2Ch1=PCI1720_SET_UP05;
	(pstObj->stConf).r_stAnOutDev.uniChn.x2Ch2=PCI1720_SET_UP05;
	(pstObj->stConf).r_stAnOutDev.uniChn.x2Ch3=PCI1720_SET_UP05;
	
	//将配置应用的设备上
	Pci1720Config(&(pstObj->stConf).r_stAnOutDev);

	return SYS_OK;
}



