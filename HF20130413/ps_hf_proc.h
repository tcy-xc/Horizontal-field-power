/*
文件：		ps_hf_proc.h
内容：		定义Processor对象的数据结构与接口函数
更新记录：
-- Ver 0.9 / 2011-12-1
*/
/*
单位说明：
			1、接收到的数据
					时间均以ms为单位
					wave中的电流值以KA为单位
					parameter中的DX_set以cm为单位
			2、反馈计算的数据
					电流均以A为单位
					位移以m为单位
*/
#ifndef PS_PROC_H_
#define PS_PROC_H_

#include <pthread.h>
#include "ps_hf_io.h"
#include "ps_hf_log.h"
#include "ps_hf_system.h"

#define IP_INVALID 0
#define IP_EFFICIENT 1
#define IP_EXTINGUISH 2
#define IP_DISRUPTION 3
#define IP_FAULT 4

#define PLASMA_TIME_SCALE	1100
#define PLASMA_BREAKDOWN_POINT 250 //HF触发为-200ms
#define DISCHARGE_TIME_SCALE 2000 

//定义计算参数
typedef struct CalulationData
{
	double Ihf_set;				//HF整理器输出电流设定值
	double Ihf_real;				//HF整流器输出电流实际值
	double Ihf_feedback_init;		//
	double Uhf_dc;				//HF整理器输出端电压值
	double Uhf_ac[2];				//HF整流器输入端电压值
	double outvolt[2];				//HF控制角对应电压
	double angle[2];			//HF整流器控制角
	double angle_pref[2];    //记录上一周期的HF整流器控制角
	double Ip[2];					//等离子体电流,0表示上一周期值,1表示本周期值
	double DY[2];				//等离子体水平位移,0表示上一周期值,1表示本周期值
	double rogowski;			//罗柯线圈信号
	double saddle;				//鞍型线圈信号

	int IpFlag;					//IP标志
	double DY_err_p;			//位移PID计算比例量
	double DY_err_i;			//位移PID计算积分量
	double DY_err_d;			//位移PID计算微分量
	double Ihf_err_p;			//电流PID计算比例量
	double Ihf_err_i;			//电流PID计算积分量
	double Ihf_err_d;			//电流PID计算微分量
	double Ihf_buf;				//记录某次VF电流值
	int time_buf;					//记录某次时刻
	double buf[10];			//中间计算用
	double dft[2][96];			//DFT计算用
	int count;					//取平均值记数用
	double feedback_buf;
	double I_DRMP_DC2;			//20130522,wnc
	int IpOLcount;

	int InitFlag;				//反馈初始化标志,add by zgz 20160512

} cal_data;
//定义控制参数,读取控制参数成功后,将固定不变
typedef struct ParameterData
{
	int ParameterFlag;			//参数设置标志位
	
	int plasma_rise;
	int plasma_flat;				//等离子体电流平顶时间
	int feedback_time;
	double DY_set;				//等离子体水平位移设定值

	double Ihf_kp;
	double Ihf_ki;
	double Ihf_kd;

	double DY_k;				//位移反馈PID参数
	double DY_kip_flat;		
	double DY_kp_flat;
	double DY_ki_flat;				
	double DY_kd_flat;
	double DY_kip_flow;
	double DY_kp_flow;
	double DY_ki_flow;
	double DY_kd_flow;

} param_data;
//定义Processor对象的数据结构
typedef struct PsProcessorTag
{
	int iPrio;					//处理线程的优先级
	long lTimeNormal;	//设置模拟量刷新时间
	ps_io_t *pstIO;			//IO对象指针，用于采集数据访问
	ps_log_t stLog;			//Log对象，记录数据
							
	cal_data cal;						//计算参数
	param_data parameter;		//读取的参数
	
	long lRunCount;				//指示处理函数运行的次数
	double adWaveValue[DISCHARGE_TIME_SCALE];//预设值
	
	long p_lFault;					//最近的故障
	int p_bRunFlag;					//允许处理函数运行的标志
	pthread_t p_stThreadId;	//处理线程id
	stimer_t *p_pstTimer;			//定时器指针，用于与IO共享定时器
	double dTimeFactor;			 //时间校正因子
	int iPlasmaTimeScale;		//等离子体存在时间
} ps_proc_t;

//接口函数
int PsProcInit(ps_proc_t *pstObj);
int PsProcRun(ps_proc_t *pstObj);
int PsProcStop(ps_proc_t *pstObj);
int PsProcConfig(ps_proc_t *pstObj, int aiTime[], double adValue[], int iCount);

/*
说明：
PsProcInit		初始化
PsProcRun		开始反馈计算
PsProcStop		停止反馈计算
PsProcConfig	配置预设波形
*/

#endif
