/*
文件：		timer.c
内容：		定时器接口函数的实现
更新记录：
-- Ver 1.03 / 2012-02-17
详细更新记录见timer.h
*/

#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include "timer.h"

/*
名称：		s_TimingThread
功能：		定时器工作线程，该线程在接收到内核的消息后广播一个定时器触发，所有
			等待该定时器的线程将被唤醒
返回值：	NULL
*/
static void *s_TimingThread(void *pvObj)
{
	stimer_t *pstObj = pvObj;
	
	for(; ; )
	{
		//在收到内核的消息前，该线程将阻塞
		pstObj->p_iRcvId = MsgReceivePulse(pstObj->p_iChId, &pstObj->p_stPulse, sizeof(pstObj->p_stPulse), NULL);
		if((pstObj->p_iRcvId == 0) && (pstObj->p_stPulse.code == TIMER_SIGEV_CODE))
		{
			//如果接收到的消息ID和代码都正确，定时器计数自动加1,并广播条件量，如
			//果该线程未被锁定，则任何等待该定时器的线程都将被唤醒
			pstObj->dwTickCount++;
			pthread_cond_broadcast(&pstObj->p_stCond);
		}
		else
		{
			/*
			程序运行到这里有以下几种可能：
			1. 定时器被销毁(TimerDisable)
			2. 发生故障使得 p_iRcvId < 0
			3. p_stPulse.code与设定值不符
			
			发生任何情况均视为故障，该线程退出并销毁互斥体，这样任何等待该定时器
			的线程都将得到“TIMER_ER”返回值。该定时器需要先销毁，再重新初始化后才
			能再次使用
			*/
			
			//加锁以确保安全删除互斥体
			pthread_mutex_lock(&pstObj->p_stMutex);
			pthread_mutex_destroy(&pstObj->p_stMutex);
			
			pstObj->r_iStatus = TIMER_UNKOWN;
			break;
		}
	}
	
	return NULL;
}

/*
名称：		TimerInit
功能：		初始化定时器
返回值：	TIMER_OK, TIMER_ER
*/
int TimerInit(stimer_t *pstObj)
{
	struct sched_param stThreadParam;
	pthread_attr_t stThreadAttr;
	//完成初始化部件的标志
	int bFlagMutex = 0;
	int bFlagCond = 0;
	int bFlagChannel = 0;
	int bFlagConnection = 0;
	int bFlagTimer = 0;
	int bFlagThread = 0;
	
	pstObj->r_iStatus = TIMER_UNKOWN;
	for(; ; )
	{
		//初始化互斥体
		if(pthread_mutex_init(&pstObj->p_stMutex, NULL) != EOK)
			break;
		bFlagMutex = 1;
		
		if(pthread_cond_init(&pstObj->p_stCond, NULL) != EOK)
			break;
		bFlagCond = 1;
		
		//创建一个通道
		pstObj->p_iChId = ChannelCreate(_NTO_CHF_FIXED_PRIORITY);
		if(pstObj->p_iChId == -1)
			break;
		bFlagChannel = 1;
		
		//设置Sigev/信号事件？
		pstObj->p_stEvent.sigev_notify = TIMER_SIGEV_NOTIFY;
		pstObj->p_stEvent.sigev_coid = ConnectAttach(0, 0, pstObj->p_iChId,
													_NTO_SIDE_CHANNEL, 0);
		pstObj->p_stEvent.sigev_priority = TIMER_SIGEV_PRIO;
		pstObj->p_stEvent.sigev_code = TIMER_SIGEV_CODE;
		if(pstObj->p_stEvent.sigev_code == -1)
			break;
		bFlagConnection = 1;
		
		//创建一个内核定时器
		if(timer_create(CLOCK_REALTIME, &pstObj->p_stEvent, &pstObj->p_stTimerId) == -1)
			break;
		bFlagTimer = 1;
		
		//清除定时间隔设置
		TimerSetInterval(pstObj, 0);
		
		//创建定时器工作线程
		stThreadParam.sched_priority = TIMER_THRD_PRIO;
		pthread_attr_init(&stThreadAttr);
		pthread_attr_setdetachstate(&stThreadAttr, PTHREAD_CREATE_DETACHED);
		pthread_attr_setinheritsched(&stThreadAttr, PTHREAD_EXPLICIT_SCHED);
		pthread_attr_setschedparam(&stThreadAttr, &stThreadParam);
		pthread_attr_setschedpolicy(&stThreadAttr, SCHED_FIFO);
		pthread_create(&pstObj->p_stThreadId, &stThreadAttr, s_TimingThread, pstObj);
		if(pstObj->p_stThreadId == NULL)
			break;
		bFlagThread = 1;
		
		//初始化正常
		pstObj->dwTickCount = 0;
		pstObj->r_iStatus = TIMER_READY;
		return TIMER_OK;
	}
	
	//有错误，释放已初始化的部件
	if(bFlagMutex)
	{
		pthread_mutex_lock(&pstObj->p_stMutex);
		pthread_mutex_destroy(&pstObj->p_stMutex);
	}
	if(bFlagCond)
	{
		pthread_cond_destroy(&pstObj->p_stCond);
	}
	if(bFlagChannel)
	{
		ChannelDestroy(pstObj->p_iChId);
	}
	if(bFlagConnection)
	{
		ConnectDetach(pstObj->p_stEvent.sigev_coid);
	}
	if(bFlagTimer)
	{
		timer_delete(pstObj->p_stTimerId);
	}
	if(bFlagThread)
	{
		//实际上这一行永远不会执行
		pthread_abort(pstObj->p_stThreadId);
	}
	
	return TIMER_ER;
}

/*
名称：		TimerDisable
功能：		销毁一个定时器，销毁后需重新初始化才能使用
返回值：	TIMER_OK
*/
int TimerDisable(stimer_t *pstObj)
{
	timer_delete(pstObj->p_stTimerId);
	ConnectDetach(pstObj->p_stEvent.sigev_coid);
	ChannelDestroy(pstObj->p_iChId);
	pthread_cond_destroy(&pstObj->p_stCond);
	/*
	在连接断开（ConnectDetach）与通道销毁后（ChannelDestroy）后，MsgReceivePuse
	函数必然返回一个错误，这样定时器的工作线程就可以把余下的清理工作完成。
	*/
	return TIMER_OK;
}

/*
名称：		TimerStart
功能：		启动定时器，以NanoSeconds指定启动延时
返回值：	TIMER_OK, TIMER_ER
*/
int TimerStart(stimer_t *pstObj, unsigned long NanoSeconds)
{
	pstObj->p_stInterval.it_value.tv_sec = 0;
	pstObj->p_stInterval.it_value.tv_nsec = NanoSeconds;
	if(timer_settime(pstObj->p_stTimerId, 0, &pstObj->p_stInterval, NULL) == -1)
	{
		return TIMER_ER;
	}
	else
	{
		pstObj->r_iStatus = TIMER_RUN;
	}
	
	return TIMER_OK;
}

/*
名称：		TimerStart2
功能：		启动定时器，延时1ns，实际延时可能为1个ClockPeroid，即1个TickSize
返回值：	TIMER_OK, TIMER_ER
*/
int TimerStart2(stimer_t *pstObj)
{
	return TimerStart(pstObj, 1);
}

/*
名称：		TimerStart3
功能：		启动定时器，延时1个定时间隔
返回值：	TIMER_OK, TIMER_ER
*/
int TimerStart3(stimer_t *pstObj)
{
	return TimerStart(pstObj, pstObj->p_stInterval.it_interval.tv_nsec);
}

/*
名称：		TiemrStop
功能：		停止定时器
返回值：	TIMER_OK, TIMER_ER
*/
int TimerStop(stimer_t *pstObj)
{
	return TimerStart(pstObj, 0);
}

/*
名称：		TimerWait
功能：		阻塞主调线程，直到接收到定时器的触发信号，仅未锁定定时器的线程中使用
返回值：	TIMER_OK, TIMER_ER
*/
int TimerWait(stimer_t *pstObj)
{
	if(pthread_mutex_lock(&pstObj->p_stMutex) != EOK ||
		pthread_cond_wait(&pstObj->p_stCond, &pstObj->p_stMutex) != EOK)
	{
		pthread_mutex_unlock(&pstObj->p_stMutex);
		return TIMER_ER;
	}
	pthread_mutex_unlock(&pstObj->p_stMutex);
	return TIMER_OK;
}

/*
名称：		TimerWait2
功能：		阻塞主调线程，直到接收到定时器的触发信号，仅已锁定定时器的线程中使用
			当使用该函数进入阻塞状态后，主调线程将自动解锁；当线程唤醒时又会自动
			锁定
返回值：	TIMER_OK, TIMER_ER
*/
int TimerWait2(stimer_t *pstObj)
{
	if(pthread_cond_wait(&pstObj->p_stCond, &pstObj->p_stMutex) != EOK)
		return TIMER_ER;
	
	return TIMER_OK;
}

/*
名称：		TimerLock
功能：		为主调线程锁定定时器。
			在定时器锁定后，其仍然会按照预设的时钟触发，但所有等待该定时器的线程
			将保持阻塞(TimerWait和TimerWait2均受影响)，直到主调线程将定时器解锁。
			若该定时器已被其它线程锁定，则该函数会阻塞主调进程直到该定时器解锁。
			即所有锁定同一个定时器的线程最多只有一个在运行。
返回值：	TIMER_OK, TIMER_ER
*/
int TimerLock(stimer_t *pstObj)
{
	if(pthread_mutex_lock(&pstObj->p_stMutex) != EOK)
		return TIMER_ER;
		
	return TIMER_OK;
}

/*
名称：		TimerUnlock
功能：		解锁定时器
返回值：	TIMER_OK;
*/
int TimerUnlock(stimer_t *pstObj)
{
	pthread_mutex_unlock(&pstObj->p_stMutex);
	return TIMER_OK;
}

/*
名称：		TimerSetTickSize
功能：		设置定时器的“tick size”，在qnx系统中又称为“clock period”，我称之为
			系统的“响应周期/时间”。这是一个系统全局参数，意为内核每隔该时间就会
			核查一次内核定时器的运行情况。因此定时器的时间间隔设置小于响应周期是
			没有意义的。
			该值设置太小，则内核负担加大；过小则会出错
注意：		由于系统的特殊性，实际的TickSize可能与用户设置值不同
返回值：	实际的“TickSize”， 0表示错误
*/
unsigned long TimerSetTickSize(unsigned long dwNanoSeconds)
{
	struct _clockperiod stNew;
	struct _clockperiod stOld;
	stNew.nsec = dwNanoSeconds;
	stNew.fract = 0;
	if(ClockPeriod(CLOCK_REALTIME, &stNew, NULL, 0) == -1)
		return 0;
	ClockPeriod(CLOCK_REALTIME, NULL, &stOld, 0);
	
	return stOld.nsec;
}

/*
名称：		TimerSetInterval
功能：		设置定时器的触发间隔为dwNanoSeconds指定的时间
注意：		TimerStart后才能生效
返回值：	dwNanoSeconds
*/
unsigned long TimerSetInterval(stimer_t *pstObj, unsigned long dwNanoSeconds)
{
	pstObj->p_stInterval.it_interval.tv_sec = 0;
	pstObj->p_stInterval.it_interval.tv_nsec = dwNanoSeconds;
	return dwNanoSeconds;
}

/*
名称：		TimerSetInterval2
功能：		根据系统当前的响应周期修正设置的定时器触发间隔，使得触发时间更为规则
注意：		TimerStart后才能生效
返回值：	实际的触发间隔，0表示错误
*/
unsigned long TimerSetInterval2(stimer_t *pstObj, unsigned long dwNanoSeconds)
{
	struct _clockperiod stClockPeriod;
	unsigned long dwTmp;
	
	if(ClockPeriod(CLOCK_REALTIME, NULL, &stClockPeriod, 0) == -1)
		return 0;
	dwTmp = dwNanoSeconds / stClockPeriod.nsec;
	if((2 * dwTmp + 1) * stClockPeriod.nsec > 2 * dwNanoSeconds)
	{
		// (n + 1) * t - N > N - n * t
		dwNanoSeconds = dwTmp * stClockPeriod.nsec;
	}
	else
	{
		dwNanoSeconds = (dwTmp + 1) * stClockPeriod.nsec;
	}
	
	pstObj->p_stInterval.it_interval.tv_sec = 0;
	pstObj->p_stInterval.it_interval.tv_nsec = dwNanoSeconds;
	
	return dwNanoSeconds;
}
