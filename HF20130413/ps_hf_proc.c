#include <sched.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "ps_hf_proc.h"
#include "ps_hf_lib.h"

static void *s_ProcThread(void *pvObj);					
inline static int s_ProcFunc(ps_proc_t *pstObj);		
static void PsProcDataProcessing(ps_proc_t *pstObj);	
static void PsCheckFault(ps_proc_t *pstObj);			
static double PsProcCurrentPID(ps_proc_t *pstObj);		
static double PsProcDisplacementPID(ps_proc_t *pstObj);	
static void PsProcAngleOut(ps_proc_t *pstObj);	
static double PsProcFFT24(double sample[],double k);	

int PsProcInit(ps_proc_t *pstObj)
{
	struct sched_param stParam;
	pthread_attr_t stAttr;
	
	//绑定IO定时器
	pstObj->p_pstTimer = &pstObj->pstIO->stConf.r_stAnTimer;
		
	//将stLog绑定到自身
	pstObj->stLog.pvPsor = pstObj;
	
	//初始化
	memset(&(pstObj->cal),0,sizeof(pstObj->cal));
	pstObj->lRunCount = 1;		
	PsIOAnReset(pstObj->pstIO);
	pstObj->cal.count=48;
	pstObj->cal.angle_pref[0] = 135;
	pstObj->cal.angle_pref[1] = 135;
	pstObj->cal.InitFlag = 0;// add by zgz 20160512

	//设置主处理线程属性:detached,FIFO,prio
	pthread_attr_init(&stAttr);
	pthread_attr_setdetachstate(&stAttr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setinheritsched(&stAttr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&stAttr, SCHED_FIFO);
	stParam.sched_priority = pstObj->iPrio;
	pthread_attr_setschedparam(&stAttr, &stParam);
	
	//创建处理线程
	pthread_create(&pstObj->p_stThreadId, &stAttr, s_ProcThread, pstObj);
	if(pstObj->p_stThreadId == NULL)
	{
		return ERR_THREAD;
	}
	
	//设置IO刷新间隔
	PsIOAnSetInterval(pstObj->pstIO, pstObj->lTimeNormal);
	pstObj->dTimeFactor=(double)1000000/g_stIO.stConf.r_lAnInterval;
	printf("%f\n",pstObj->dTimeFactor);

	//校正时间参数
	pstObj->iPlasmaTimeScale=PLASMA_TIME_SCALE*pstObj->dTimeFactor+0.5;
	if(pstObj->iPlasmaTimeScale > PARAM_TIME_SCALE)
		return ERR_PARAM;

	return ERR_NONE;
}

int PsProcRun(ps_proc_t *pstObj)
{
	
	//先停止定时器,否则可能产生BUG
	TimerStop(pstObj->p_pstTimer);
	
	//锁定定时器,确保不与其它线程冲突
	TimerLock(pstObj->p_pstTimer);
	
	//允许处理函数运行
	pstObj->p_bRunFlag = 1;
	
	//重启定时器,跳过t=0的时候
	TimerStart3(pstObj->p_pstTimer);
	
	//解锁定时器
	TimerUnlock(pstObj->p_pstTimer);
	
	return SYS_OK;
}

int PsProcStop(ps_proc_t *pstObj)
{
	int s;

	//此时锁定定时器可能失败,因为在之前的程序中可能已锁定
	s = TimerLock(pstObj->p_pstTimer);
	
	//整流器输出为０
	pstObj->cal.angle[1] = 135;
	pstObj->cal.angle[0] = 135;
	PsProcAngleOut(pstObj);
	PsIOAnReset(pstObj->pstIO);
	
	//初始化参数
	memset(&(pstObj->cal),0,sizeof(pstObj->cal));
	pstObj->cal.angle_pref[0] = 135;
	pstObj->cal.angle_pref[1] = 135;
	pstObj->lRunCount = 1;
	pstObj->p_bRunFlag = 0;
	pstObj->cal.count=48;
	
	if(s == TIMER_OK)
		TimerUnlock(pstObj->p_pstTimer);
	
	//输出记录
	PsLogOut(&pstObj->stLog);
	memset(&(pstObj->stLog.astData),0,sizeof(pstObj->stLog.astData));
	printf("******* Discharge Finish **********\n");
	
	return SYS_OK;
}

int PsProcConfig(ps_proc_t *pstObj, int aiTime[], double adValue[], int iCount)
{
	int i;
	int t;
	double dDelta;
	
	//输入参数检验
	for(i = 0; i < iCount; i++)
	{
		if(	aiTime[i] < 0 ||								//时间值小于0
			aiTime[i] > DISCHARGE_TIME_SCALE ||				//时间值超过PARAM_TIME_SCALE
			(i > 0 && aiTime[i] <= aiTime[i - 1]))			//时间值减小
			return ERR_PARAM;
	}
	
	//头部填0
	for(t = 0; t < aiTime[0]; t++)
	{
		pstObj->adWaveValue[t] = 0;
	}
	
	//尾部填0
	for(t = aiTime[iCount - 1]; t < DISCHARGE_TIME_SCALE; t++)
	{
		pstObj->adWaveValue[t] = 0;
	}
	
	//中间段线性插值
	pstObj->adWaveValue[aiTime[0]] = adValue[0];
	for(i = 1; i < iCount; i++)
	{
		dDelta = (adValue[i] - adValue[i - 1]) / (aiTime[i] - aiTime[i - 1]);
		for(t = aiTime[i - 1]; t < aiTime[i]; t++)
		{
			pstObj->adWaveValue[t + 1] = pstObj->adWaveValue[t] + dDelta;
		}
	}
	return ERR_NONE;
}

static void *s_ProcThread(void *pvObj)
{
	ps_proc_t *pstObj = pvObj;
	
	TimerLock(pstObj->p_pstTimer);
	for(; ; )
	{
		//等待定时器信号
		if(TimerWait2(pstObj->p_pstTimer) == TIMER_ER)
		{
			PsSetError(ERR_FATAL);
			break;
		}
		
		//检查运行条件
		if(pstObj->p_bRunFlag !=1)
		{	
			continue;
		}
		
		//计算反馈
		s_ProcFunc(pstObj);
			
	}
	TimerUnlock(pstObj->p_pstTimer);
	return NULL;
}


inline static int s_ProcFunc(ps_proc_t *pstObj)
{
	//当运行时间<900ms
	if(pstObj->lRunCount < (pstObj->iPlasmaTimeScale))
	{		
		PsProcDataProcessing(pstObj);

		PsCheckFault(pstObj);
	
		switch(pstObj->cal.IpFlag)
		{
			/**IP_INVALID值为零，由前述memset段进行初始化,表示IP没击穿 **/
			case IP_INVALID:
			if(pstObj->lRunCount<pstObj->parameter.feedback_time)
				pstObj->cal.Ihf_set = pstObj->adWaveValue[pstObj->lRunCount];//预设输出
			else
				pstObj->cal.Ihf_set=0;
				
			pstObj->cal.angle[0]=PsProcCurrentPID(pstObj);//电流PID,计算出角度
			pstObj->cal.angle[1] = 135;			
			break;
			
			/**IP击穿 **/
			case IP_EFFICIENT:
			//if(pstObj->lRunCount < (pstObj->parameter.feedback_time - (int)(30*pstObj->dTimeFactor+0.5)))
			if(pstObj->lRunCount < (pstObj->parameter.feedback_time))
				pstObj->cal.Ihf_set = pstObj->adWaveValue[pstObj->lRunCount];//预设输出
			else
				pstObj->cal.Ihf_set = PsProcDisplacementPID(pstObj);//位移反馈输出
				
//			else if(pstObj->lRunCount < pstObj->parameter.feedback_time)
//			{
//				pstObj->cal.Ihf_set = pstObj->adWaveValue[pstObj->lRunCount];//预设输出
//				PsProcDisplacementPID(pstObj);//积分量累积
//			}
//			else if(pstObj->lRunCount >= pstObj->parameter.feedback_time)
//				pstObj->cal.Ihf_set=PsProcDisplacementPID(pstObj);//位移反馈输出
							
			pstObj->cal.angle[0]=PsProcCurrentPID(pstObj);//电流PID,计算出角度
			pstObj->cal.angle[1] = 135;
			break;
			
			/**IP破裂 **/
			case IP_DISRUPTION:
			if( pstObj->lRunCount<=(pstObj->cal.time_buf+(int)(30*pstObj->dTimeFactor+0.5)))
			{
				//从破裂时刻点起,30ms内
				//pstObj->cal.Ihf_set=pstObj->cal.Ihf_buf*(1- ( pstObj->lRunCount-pstObj->cal.time_buf )/(int)(30*pstObj->dTimeFactor+0.5));
				pstObj->cal.Ihf_set=pstObj->cal.Ihf_buf*(1- ( pstObj->lRunCount-pstObj->cal.time_buf )/36.0);
				
				pstObj->cal.angle[0]=PsProcCurrentPID(pstObj);//电流PID,计算出角度
				pstObj->cal.angle[1] = 135;
			}
			else//从破裂时刻点起,30ms后
			{
				pstObj->cal.angle[0] = 135;
				pstObj->cal.angle[1] = 135;
			}
			break;
			
			/**IP正常消失 **/
			case IP_EXTINGUISH:
			if( pstObj->lRunCount<=(pstObj->cal.time_buf+(int)(30*pstObj->dTimeFactor+0.5)))
			{
				//从破裂时刻点起,30ms内
				//pstObj->cal.Ihf_set=pstObj->cal.Ihf_buf*(1- ( pstObj->lRunCount-pstObj->cal.time_buf )/(int)(30*pstObj->dTimeFactor+0.5));
				pstObj->cal.Ihf_set=pstObj->cal.Ihf_buf*(1- ( pstObj->lRunCount-pstObj->cal.time_buf )/36.0);
				
				pstObj->cal.angle[0]=PsProcCurrentPID(pstObj);//电流PID,计算出角度
				pstObj->cal.angle[1] = 135;
			}
			else//从破裂时刻点起,30ms后
			{
				pstObj->cal.angle[0] = 135;
				pstObj->cal.angle[1] = 135;
			}
			break;
			

			default:
			pstObj->cal.angle[0] = 135;
			pstObj->cal.angle[1] = 135;
			break;

			case IP_FAULT:
			//出现了故障，控制输出由PsSetStat处理，这里应直接返回
			return SYS_ER;
		}
	}
	//放电完毕,时间>=pstObj->iPlasmaTimeScale=900ms
	else
	{
		pstObj->cal.angle[0] = 135;
		pstObj->cal.angle[1] = 135;
	}
	
	//角度输出
	PsProcAngleOut(pstObj);
	
	//添加记录
	PsLogAdd(&pstObj->stLog);
	
	//时间大于程序运行时间
	if((++pstObj->lRunCount) >= PARAM_TIME_SCALE)
	{
		//消除脉冲使能信号
		pstObj->pstIO->w_stDgOutValue.xB0 = PCI1750_OUT_HI;
		pstObj->pstIO->w_stDgOutValue.xB1 = PCI1750_OUT_HI;
		PsIODgRefresh(pstObj->pstIO);
		PsSetStatus(STAT_IDLE);
	
	}

  
	return SYS_OK;
}

static void PsProcDataProcessing(ps_proc_t *pstObj)
{
	int n,i;
	double a[11], DFT[24];

	for(i=0 ; i<11 ; i++)
		a[i]=0;
		
	n=pstObj->cal.count;

	//第０位为交流电压
	pstObj->cal.dft[0][n]=pstObj->pstIO->r_adAnInValue[0];
	pstObj->cal.dft[0][n-48]=pstObj->cal.dft[0][n];
	for(i=0;i<24;i++)
		DFT[i]=pstObj->cal.dft[0][n-23+i];	 
	 a[0] = PsProcFFT24(&DFT[0],1);

	//第1位为交流电压
	pstObj->cal.dft[1][n]=pstObj->pstIO->r_adAnInValue[1];
	pstObj->cal.dft[1][n-48]=pstObj->cal.dft[1][n];
	for(i=0;i<24;i++)
		DFT[i]=pstObj->cal.dft[1][n-23+i];	 
	 a[1] = PsProcFFT24(&DFT[0],1);

	//第2~7位
	for(i=2; i<8; i++)
		a[i]=pstObj->pstIO->r_adAnInValue[i];

	if((++pstObj->cal.count)>95)
		pstObj->cal.count=48;

	//交流电压
	pstObj->cal.Uhf_ac[0]= a[0]*20;
	pstObj->cal.Uhf_ac[1]= a[1]*20;
	//直流电压	
	pstObj->cal.Uhf_dc= a[2]*40;
	//直流电流
	pstObj->cal.Ihf_real= a[3]*200;
	//20130522 wcn
	pstObj->cal.I_DRMP_DC2= a[7];	
	//Ip
	pstObj->cal.Ip[1]=a[4 ]*(1000.0)/(0.004);//change to Ipouter. modified by zpchen at 2015.05.29.
//	pstObj->cal.Ip[1]=-a[4 ]*(1000.0)/(0.004);;//for plasma current reverse,zpchen20150703
//	pstObj->cal.Ip[1]=-a[4 ]*(1000.0)/(0.004);//change to Ipouter. modified by zpchen at 2014.05.26.
	//the signs before the signal of the displacement magnetic coil are opposite between the control system and DAQ system.2013.10.22 zpchen
	//Then pstObj->cal.rogowski= -a[5] and pstObj->cal.saddle=-a[6 initally].
//	pstObj->cal.rogowski= -a[5];//delete '*1.01*0.93', modified by zpchen20160105
//	pstObj->cal.rogowski= 0*1.01*0.93;//fixed to 0 because diagnostic problem by zpchen20151228
	pstObj->cal.rogowski= -a[5]*1.01*0.93;//modified for the change to Pcbyrog coil by zpchen20150529
//	pstObj->cal.rogowski= a[5]*1.01*0.93;;//for plasma current reverse by zpchen201506
    pstObj->cal.saddle=-a[6]*1;//2013.10.22  change from 364
//	pstObj->cal.saddle= a[6]*1;//for plasma current reverse by zpchen20150703
	//pstObj->cal.Ip[1]=a[4 ]*(1000.0)/(0.004);//for plasma current reverse, modified by zpchen at 2012.12.25

	
	/********use rmp by ld******
	 * 
	 */
	//pstObj->cal.saddle=-a[6]*-1-pstObj->cal.I_DRMP_DC2/21;//补偿内部扰动场线圈对PCB鞍形的影响20131122
//	pstObj->cal.saddle=-a[6]*1+pstObj->cal.I_DRMP_DC2/15;//modified by zpchen201506.
//	pstObj->cal.saddle=-a[6]*1+pstObj->cal.I_DRMP_DC2/24;//modified by zhulizhi 20160505
//	pstObj->cal.saddle=-a[6]*1-pstObj->cal.I_DRMP_DC2*0.055;//modified by huangzhuo 20170331
//	pstObj->cal.saddle=-a[6]*1+pstObj->cal.I_DRMP_DC2*0.055;//modified by  20170504
	pstObj->cal.saddle=-a[6]*1+pstObj->cal.I_DRMP_DC2*0.055;//modified by huangzhuo 20170608
	/**************************/
	
	
	
//	//DRMP 当t>230ms && t<470ms,
//	if((pstObj->lRunCount>=490) && (pstObj->lRunCount<810))
//	{
//		if(pstObj->lRunCount==490)
//		{
//			pstObj->cal.feedback_buf = pstObj->cal.saddle;
//		}
//		else if((pstObj->lRunCount>490) && (pstObj->lRunCount<810))
//		{
//			pstObj->cal.saddle = pstObj->cal.feedback_buf;//+0.15/1200*(pstObj->lRunCount-444);
//		}
//		else if(pstObj->lRunCount==810)
//		{
//			pstObj->cal.feedback_buf = pstObj->cal.saddle-(pstObj->cal.feedback_buf);//+0.15/1200*(684-444));
//			pstObj->cal.saddle = pstObj->cal.feedback_buf;
//		}
//		else
//		{
//			pstObj->cal.rogowski = pstObj->cal.rogowski-(pstObj->cal.feedback_buf);//+0.15/1200*(684-444));
//		}
//	}
	
	
	//位移
	if(pstObj->cal.Ip[1]>0)
	{
		pstObj->cal.DY[1]= 10*35.4*(pstObj->cal.rogowski+1.09*(pstObj->cal.saddle))/(pstObj->cal.Ip[1]/1000);//put 'Ip' back modified by zpchen at 2016.5.10
//		pstObj->cal.DY[1]=  2*pstObj->cal.DY[1];//added by zpchen for only Ysad at 20151229
		pstObj->cal.DY[1]=  pstObj->cal.DY[1]/100.0;
	}
	else
	{
		pstObj->cal.DY[1]=0;
	}
	
	

}

static void PsCheckFault(ps_proc_t *pstObj)
{
	//检查IP过流
	if(fabsf(pstObj->cal.Ip[1]) > 300000 )//modify this limit from 250000 to 300000 by zhulizhi@20160427
	{
		pstObj->cal.IpOLcount++;
		
		if(pstObj->cal.IpOLcount > 3)
		{
			pstObj->cal.IpFlag=IP_FAULT;
			pstObj->pstIO->w_stDgOutValue.xB2=PCI1750_OUT_HI;
			PsIODgRefresh(pstObj->pstIO);
			//添加记录
			PsLogAdd(&pstObj->stLog);
			PsSetStatus(STAT_ERROR);
			printf("Plasma Current Overload!\n");
			fprintf(stderr, "FAULT: INTERNAL FAULT.\n");
			PsSetFault((PLASMA_CURRENT_OVERLOAD |PsGetFault()));
		}
	}
	
//	//检查IP过流
//	if(fabsf(pstObj->cal.Ip[1]) > 250000 )
//	{
//		pstObj->cal.IpFlag=IP_FAULT;
//		pstObj->pstIO->w_stDgOutValue.xB2=PCI1750_OUT_HI;
//		PsIODgRefresh(pstObj->pstIO);
//		PsSetStatus(STAT_ERROR);
//		printf("Plasma Current Overload!\n");
//		fprintf(stderr, "FAULT: INTERNAL FAULT.\n");
//		PsSetFault((PLASMA_CURRENT_OVERLOAD |PsGetFault()));
//	}

	//检查IHF过流
	if(fabsf(pstObj->cal.Ihf_real) > 500.0 )
	{
		pstObj->cal.IpFlag=IP_FAULT;
		pstObj->pstIO->w_stDgOutValue.xB2=PCI1750_OUT_HI;
		PsIODgRefresh(pstObj->pstIO);
//		//添加记录
//		PsLogAdd(&pstObj->stLog);
		PsSetStatus(STAT_ERROR);
		printf("Rectifier Current Overload!\n");
		fprintf(stderr, "FAULT: INTERNAL FAULT.\n");
		PsSetFault((FT_RECTIFIER_CURRENT_OVERLOAD |PsGetFault()));
	}
	
	//在IP击穿时间点之前,如果IP小于25kA，IP无效，否则IP有效
	if(pstObj->lRunCount<(int)(PLASMA_BREAKDOWN_POINT*pstObj->dTimeFactor+0.5))
	{
		if(pstObj->cal.Ip[1]<25000)
			pstObj->cal.IpFlag=IP_INVALID;
		else
			pstObj->cal.IpFlag=IP_EFFICIENT;
	}
	else
	{	
		if(pstObj->cal.IpFlag==IP_EFFICIENT)
		{
			if(pstObj->cal.Ip[1]<=25000 && pstObj->lRunCount < pstObj->parameter.plasma_flat)
			{
				pstObj->cal.IpFlag=IP_DISRUPTION;
				//记录破裂时的时间和电流值
				pstObj->cal.time_buf=pstObj->lRunCount;
				pstObj->cal.Ihf_buf=pstObj->cal.Ihf_real;
			}
			
//			if(pstObj->cal.Ip[1]<=30000 && pstObj->lRunCount >= pstObj->parameter.plasma_flat)
			if(pstObj->cal.Ip[1]<=10000 && pstObj->lRunCount >= pstObj->parameter.plasma_flat)
			{
				pstObj->cal.IpFlag=IP_EXTINGUISH;
				//记录IP正常消失时的时间和电流值
				pstObj->cal.time_buf=pstObj->lRunCount;
				pstObj->cal.Ihf_buf=pstObj->cal.Ihf_real;
			}
		}
	}
				
}

static double PsProcCurrentPID(ps_proc_t *pstObj)
{
	double kp,ki,kd; 
	double  temp= 0; 
	double  angle=0;
	
	kp =pstObj->parameter.Ihf_kp;
	ki = pstObj->parameter.Ihf_ki;
	kd =pstObj->parameter.Ihf_kd;

	pstObj->cal.Ihf_err_p=pstObj->cal.Ihf_set-pstObj->cal.Ihf_real;
	pstObj->cal.Ihf_err_i+=(pstObj->cal.Ihf_set-pstObj->cal.Ihf_real)*0.001/pstObj->dTimeFactor;
	pstObj->cal.Ihf_err_d=0;
		
	temp=kp*pstObj->cal.Ihf_err_p+ki*pstObj->cal.Ihf_err_i;
		
	if(fabsf(pstObj->cal.Uhf_ac[0])>0)
		temp=temp/(2*1.35*pstObj->cal.Uhf_ac[0]);
	else
		temp=0;	
	
	if(temp>0.96)
		temp=0.96;
	if(temp<-0.707)
		temp=-0.707;
	
	angle=(180/3.1415926)*acosf(temp);
	
	return (angle);

}

static double PsProcDisplacementPID(ps_proc_t *pstObj)
{
	double Ip_pref,Ip_now,DY_pref,DY_now;
	double kip,kp,ki,kd;
	double temp=0;
	double DY_set;
	long int t;
	
	//为了表达方便
	t= pstObj->lRunCount;
	DY_set=pstObj->parameter.DY_set;
	Ip_pref=pstObj->cal.Ip[0];
	Ip_now=pstObj->cal.Ip[1];
	DY_pref=pstObj->cal.DY[0];
	DY_now=pstObj->cal.DY[1];


		
//	//当t>450ms&& t<600ms,按当前电流输出
//	if((t>=540))
//	{
////		if(t==600)
////		{
////			pstObj->cal.feedback_buf=pstObj->cal.Ihf_real;
////		}
////		temp = pstObj->cal.feedback_buf;
//        temp=107;
//		return (temp);
//	}


	
	if( t < pstObj->parameter.plasma_flat )//平顶阶段
	{
		//记录开始平顶阶段反馈时的IP,hf电流
		if(pstObj->cal.InitFlag==0)
		{
			pstObj->cal.InitFlag=1;
//			pstObj->cal.Ip[0]=pstObj->cal.Ip[1];//zpchen 2016.5.12
//			Ip_pref=pstObj->cal.Ip[0];//zpchen 2016.5.12
			pstObj->cal.Ihf_feedback_init = pstObj->adWaveValue[t];//pstObj->cal.Ihf_real; zpchen 2016.5.12
			Ip_pref = Ip_now;
			pstObj->cal.Ip[0]=Ip_pref;//add by zpchen at 2016.5.17
		}
		
//		//当位移偏差不大,按当前电流输出,deleted by zpchen at 2016.5.12
//		if(((DY_now-DY_set)<0.001)&& ((DY_now-DY_set)>-0.001))// change 'DY_now' to 'DY_now-DY_set'
//		{
//			temp = pstObj->cal.Ihf_set;//pstObj->cal.Ihf_real; zpchen 2016.5.12
//			return (temp);
//		}
		//当位移过于靠上或过于靠下直接,输出最大最小值
		if((DY_now-DY_set) > 0.06)// change 'DY_now' to 'DY_now-DY_set'
		{
			temp =20;//这个值有问题
			return (temp);
		}
		if((DY_now-DY_set) <- 0.06)// change 'DY_now' to 'DY_now-DY_set'
		{
			temp =100;
			return (temp);
		}
	
		kip = pstObj->parameter.DY_kip_flat;
		kp = pstObj->parameter.DY_kp_flat;
		ki = pstObj->parameter.DY_ki_flat;
		kd =pstObj->parameter.DY_kd_flat;
		
		//比例量
		pstObj->cal.DY_err_p = DY_now-DY_set;
		//积分量
		if(Ip_now >60000)
		pstObj->cal.DY_err_i = pstObj->cal.DY_err_i - pstObj->cal.DY_err_p*0.001/pstObj->dTimeFactor;
		//change 'DY_now-DY_set' to 'pstObj->cal.DY_err_p'

		
		//PID
		temp= -kp* pstObj->cal.DY_err_p*Ip_now;
		temp+= ki * pstObj->cal.DY_err_i*Ip_now;
//		temp+= kd* (DY_now-DY_pref)*Ip_now;//kd=0，相当于没用微分量
		
		temp=temp + kip*(Ip_now - Ip_pref)+pstObj->cal.Ihf_feedback_init;
//		temp=-temp - kip*(Ip_now - Ip_pref)+pstObj->cal.Ihf_feedback_init;//for current reverse and hf current direction remain, modified by zpchen20150703
	}
	
	else//下降阶段,t >=pstObj->parameter.plasma_flat 
	{
		//记录开始下降阶段反馈时的IP,hf电流
		if(pstObj->cal.InitFlag==1)
		{
			pstObj->cal.InitFlag=2;
//			pstObj->cal.Ip[0]=pstObj->cal.Ip[1];//zpchen 2016.5.12
//			Ip_pref=pstObj->cal.Ip[0];//zpchen 2016.5.12			
			pstObj->cal.Ihf_feedback_init += pstObj->parameter.DY_kip_flat*(Ip_now - Ip_pref) ;
			Ip_pref = Ip_now;
			pstObj->cal.Ip[0]=Ip_pref;//add by zpchen at 2016.5.17
//			pstObj->cal.Ihf_feedback_init = pstObj->cal.Ihf_set;//delete by zpchen at 2016.5.13
		}
		
//		//当位移偏差不大,按当前电流输出,deleted by zpchen at 2016.5.12
//		if(((DY_now-DY_set)<0.001)&& ((DY_now-DY_set)>-0.001))// change 'DY_now' to 'DY_now-DY_set'
//		{
//			temp = pstObj->cal.Ihf_set;//pstObj->cal.Ihf_real; zpchen 2016.5.12
//			return (temp);
//		}
		//当位移过于靠上或过于靠下直接,输出最大最小值
		if((DY_now-DY_set) > 0.06)// change 'DY_now' to 'DY_now-DY_set'
		{
			temp =20;//这个值有问题
			return (temp);
		}
		if((DY_now-DY_set) <- 0.06)// change 'DY_now' to 'DY_now-DY_set'
		{
			temp =100;
			return (temp);
		}
		
		if(Ip_now >10000)//30KA to 10kA 20121225
		{
			kip = pstObj->parameter.DY_kip_flow;
			kp = pstObj->parameter.DY_kp_flow;
			ki = pstObj->parameter.DY_ki_flow;
			kd =pstObj->parameter.DY_kd_flow;
			
			//比例量
		   	pstObj->cal.DY_err_p = DY_now-DY_set;
		   	
			//积分量		
			pstObj->cal.DY_err_i = pstObj->cal.DY_err_i - pstObj->cal.DY_err_p*0.001/pstObj->dTimeFactor;

		   	
			
			//PID
			temp= -kp* pstObj->cal.DY_err_p*Ip_now;
			temp+= ki * pstObj->cal.DY_err_i*Ip_now;
			temp+=kd* (DY_now-DY_pref)*Ip_now;
	
			temp=temp + kip*(Ip_now - Ip_pref)+pstObj->cal.Ihf_feedback_init;
//			temp=-temp - kip*(Ip_now - Ip_pref)+pstObj->cal.Ihf_feedback_init;//for current reverse and hf current direction remain, modified by zpchen20150703
		}
		else 
			temp=0;
			//当Ip开始小于30KA时,已经在PsCheckFault里面检查到了,
			//将IP状态置为IP_EXTINGUISH,不会进到这个else里面
			//但为了保证程序出口的完整性，加上此句		
	}

//	//下降阶段的处理
//	if(t==(pstObj->parameter.plasma_flat - (int)(50*pstObj->dTimeFactor+0.5)))
//		pstObj->cal.Ihf_buf=pstObj->cal.Ihf_real;
//	if( t >= pstObj->parameter.plasma_flat)
//	{
//		if(t<(int)(830*pstObj->dTimeFactor+0.5))
//			temp=pstObj->cal.Ihf_buf;
//		else
//			temp=0;
//	}

	//限制最大最小值
	if(temp> 500)
		temp= 500;
	if(temp < 0)
		temp = 0;

	//记录本周期位移
	pstObj->cal.DY[0]=pstObj->cal.DY[1];
	return(temp);
}

static void PsProcAngleOut(ps_proc_t *pstObj)
{
	double outvolt[2];
	
	//限制最大最小角度
	if( pstObj->cal.angle[0] > 150.0 )  
		pstObj->cal.angle[0] = 150.0; 
	if( pstObj->cal.angle[0] < 15.0 )   
		pstObj->cal.angle[0] = 15.0; 
		
	if( pstObj->cal.angle[1] > 150.0 )  
		pstObj->cal.angle[1] = 150.0; 
	if( pstObj->cal.angle[1] < 15.0 )   
		pstObj->cal.angle[1] = 15.0; 
		
	//限制角度变化量
	if((pstObj->cal.angle[0]-pstObj->cal.angle_pref[0]) > 55.0 ) 
		pstObj->cal.angle[0] = pstObj->cal.angle_pref[0] + 55.0;	
	if((pstObj->cal.angle[0]-pstObj->cal.angle_pref[0]) < -55.0 ) 
		pstObj->cal.angle[0] = pstObj->cal.angle_pref[0] - 55.0;
		
	if((pstObj->cal.angle[1]-pstObj->cal.angle_pref[1]) > 55.0 ) 
		pstObj->cal.angle[1] = pstObj->cal.angle_pref[1] + 55.0;	
	if((pstObj->cal.angle[1]-pstObj->cal.angle_pref[1]) < -55.0 ) 
		pstObj->cal.angle[1] = pstObj->cal.angle_pref[1] - 55.0;
		
	//记录本周期角度	
	pstObj->cal.angle_pref[0]=pstObj->cal.angle[0];
	pstObj->cal.angle[1]=pstObj->cal.angle_pref[1];

 	//angle   voltage  current     
 	// 12      5.0 V     20mA        
	// 156     0.0 V     0 mA        
	//将角度换算成电压
   	outvolt[0]=5-pstObj->cal.angle[0]/30;
	outvolt[1]=5-pstObj->cal.angle[1]/30;

	//PCI1720输出
	pstObj->pstIO->w_adAnOutValue[0]=outvolt[0];
	pstObj->pstIO->w_adAnOutValue[1]=outvolt[1];
	PsIOAnOutRefresh(pstObj->pstIO);
}

static double PsProcFFT24(double sample[],double k )
{
	double sI1,sI2,sI3,sI4,sI5,sI6;
	double sR1,sR2,sR3,sR4,sR5,sR6;
	double real,imag,value;
	
	sR1=sample[1]+sample[23]-sample[11]-sample[13];
	sR2=sample[2]+sample[22]-sample[10]-sample[14];
	sR3=sample[3]+sample[21]-sample[ 9]-sample[15];
	sR4=sample[4]+sample[20]-sample[ 8]-sample[16];
	sR5=sample[5]+sample[19]-sample[ 7]-sample[17];
	sR6=sample[0]-sample[12];
	
	sI1=sample[1]+sample[11]-sample[13]-sample[23];
	sI2=sample[2]+sample[10]-sample[14]-sample[22];
	sI3=sample[3]+sample[ 9]-sample[15]-sample[21];
	sI4=sample[4]+sample[ 8]-sample[16]-sample[20];
	sI5=sample[5]+sample[ 7]-sample[17]-sample[19];
	sI6=sample[6]-sample[18];
	
	real=(sR1*0.965926+sR2*0.866+sR3*0.7071+sR4*0.5+sR5*0.258819+sR6)/12;
	imag=(sI1*0.258819+sI2*0.5+sI3*0.7071+sI4*0.866+sI5*0.965926+sI6)/12;
	
	real=real*k;
	imag=imag*k;
	
	value= sqrt(real*real+imag*imag)/1.414;
	return value;
}

