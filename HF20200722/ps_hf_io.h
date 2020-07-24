/*
文件：		ps_hf_io.h
内容：		定义IO对象数据结构与接口函数
更新记录：
-- Ver 0.9 / 2011-11-20
*/
#ifndef PS_IO_H_
#define PS_IO_H_

#include "ps_hf_system.h"
#include "pci1713.h"
#include "pci1720.h"
#include "pci1750.h"
#include "timer.h"

//定义IO设备控制数据结构
typedef struct PsIOConfigTag
{
	//IO线程优先级
	int iAnPrio;
	int iDgPrio;
	
	//IO设备默认（安全）输出值
	double adDefAnValue[PCI1720_CHH + 1];
	pci1750_val_t stDefDgValue;
	
	//IO线程唤醒间隔时间
	long r_lAnInterval;
	long r_lDgInterval;
	
	//IO线程时钟
	stimer_t r_stAnTimer;
	stimer_t r_stDgTimer;
	
	//IO设备
	pci1713_t r_stAnInDev;
	pci1720_t r_stAnOutDev;
	pci1750_t r_stDgIODev;
	
	//IO线程号
	pthread_t r_stAnThreadId;
	pthread_t r_stDgThreadId;
} ps_ioconf_t;


//定义IO对象数据结构
typedef struct PsIOTag
{
	//设备控制数据，应先于对象进行初始化
	ps_ioconf_t stConf;

	//输入数据
	double r_adAnInValue[PCI1713_CHH + 1];
	pci1750_val_t r_stDgInValue;
	
	//输出数据
	double w_adAnOutValue[PCI1720_CHH + 1];
	pci1750_val_t w_stDgOutValue;
	

} ps_io_t;

//接口函数
int PsIOInit(ps_io_t *pstObj);
long PsIOAnSetInterval(ps_io_t *pstObj, long lNanoSeconds);
long PsIODgSetInterval(ps_io_t *pstObj, long lNanoSeconds);
int PsIOAnInRefresh(ps_io_t *pstObj);
int PsIOAnOutRefresh(ps_io_t *pstObj);
int PsIODgRefresh(ps_io_t *pstObj);
int PsIOAnReset(ps_io_t *pstObj);
int PsIODgReset(ps_io_t *pstObj);

/*
说明：
PsIOInit		初始化IO对象，调用前要求其stConf成员已完成初始化
PsIOAnSetInterval	设置模拟量IO线程的运行间隔
PsIODgSetInterval	设置数字量IO线程的运行间隔
PsIOAnInRefresh	立刻刷新模拟量输入值
PsIOAnOutRefresh立刻刷新模拟量输出值
PsIODgRefresh	立刻刷新数字量输入输出值
PsIOAnReset		将模拟量输出复位为默认值
PsIODgReset		将数字量输出复位为默认值
*/

#endif
