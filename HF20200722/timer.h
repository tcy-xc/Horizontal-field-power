/*
文件：		timer.h
内容：		定义一个实时的定时器数据结构与接口函数
更新记录：
-- V 1.03 / 2012-02-17
[Add]添加函数TimerSetInterval2，可以根据系统ClockPeriod设置更为稳定的定时间隔
[Add]添加函数TimerStart3，可以在一个定时周期后启动定时器

-- V 1.02 / 2011-11-20
[Add]添加函数TimerLock，锁定一个定时器以阻止其它线程的触发
[Add]添加函数TimerUnlock，解锁一个锁定的定时器
[Add]添加函数TimerWait2，与要求锁定定时器的线程配合使用
[Add]添加函数TimerStart2，立刻启动定时器
[Mod]修改函数TimerStart，现在可以自定义启动定时器的延迟

-- V 1.01 / 2011-07-31
[Add]添加变量dwTickCount，用以表示定时器的触发次数.

-- V 1.00 / 2011-07-15
最初的版本
*/

#ifndef TIMER_H_
#define TIMER_H_

#include <sys/siginfo.h>
#include <sys/neutrino.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>

//定义错误代码宏
#define TIMER_OK 0
#define TIMER_ER -1
#define TIMER_NULL 0

//定义定时器状态宏
#define TIMER_UNKOWN 0
#define TIMER_READY 1
#define TIMER_RUN 2

//定义定时器信号参数宏
#define TIMER_SIGEV_CODE _PULSE_CODE_MINAVAIL
#define TIMER_SIGEV_PRIO 32
#define TIMER_SIGEV_NOTIFY SIGEV_PULSE

//定义定时线程参数宏
#define TIMER_THRD_PRIO 32

//定义定时器数据结构
typedef struct SyncTimerTag
{
	//公有
	unsigned long dwTickCount;			//定时器触发计数
	//只读
	int r_iStatus;						//定时器状态
	//私有
	pthread_t p_stThreadId;				//定时器线程ID
	pthread_mutex_t p_stMutex;			//互斥体
	pthread_cond_t p_stCond;			//条件量
	struct itimerspec p_stInterval;		//定时器触发间隔
	time_t p_stTimerId;					//系统时钟ID
	int p_iChId;						//时钟脉冲通道ID
	int p_iRcvId;						//从内核接收到的ID
	struct _pulse p_stPulse;			//从内核接收到的脉冲
	struct sigevent p_stEvent;			//从内核接收到的世件
} stimer_t;	

//接口函数
int TimerInit(stimer_t *pstObj);
int TimerDisable(stimer_t *pstObj);
int TimerStart(stimer_t *pstObj, unsigned long dwNanoSeconds);
int TimerStart2(stimer_t *pstObj);
int TimerStart3(stimer_t *pstObj);
int TimerStop(stimer_t *pstObj);
int TimerWait(stimer_t *pstObj);
int TimerWait2(stimer_t *pstObj);
int TimerLock(stimer_t *pstObj);
int TimerUnlock(stimer_t *pstObj);
unsigned long TimerSetTickSize(unsigned long dwNanoSeconds);
unsigned long TimerSetInterval(stimer_t *pstObj, unsigned long dwNanoSeconds);
unsigned long TimerSetInterval2(stimer_t *pstObj, unsigned long dwNanoSeconds);

/*
说明：
TimerInit		初始化定时器
TimerDisable	销毁一个定时器，销毁后需要重新初始化才能使用
TimerStart		启动定时器，延迟与间隔可以指定
TimerStart2		启动定时器，延迟为1ns，可能会延迟一个ClockPeriod
TimerStart3		启动定时器，延迟一个定时间隔
TimerStop		停止定时器
TimerWait		等待定时器触发，仅用于未锁定定时器的线程
TimerWait2		等待定时器触发，仅用于已锁定定时器的线程
TimerLock		锁定定时器
TimerUnlock		解锁定时器
TimerSetTickSize	设定系统响应时间，也是定时器最小有效时间间隔
TimerSetInterval	直接设定定时器时间间隔
TimerSetInterval	根据系统响应时间来设定定时器时间间隔
*/
#endif
