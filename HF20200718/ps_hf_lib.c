/*
文件：		ps_lib.c
内容：		电源控制库函数
更新记录：
-- Ver 0.9 / 2011-11-20
*/

#include <stdlib.h>
#include <stdio.h>
#include "ps_hf_lib.h"

/*
名称：		PsGetStatus
功能：		获取系统状态
返回值：	g_lStatus
*/
long PsGetStatus()
{
	return g_lStatus;
}

/*
名称：		PsGetError
功能：		获取系统错误
返回值：	g_lError
*/
long PsGetError()
{
	return g_lError;
}

/*
名称：		PsGetFault
功能：		获取设备故障
返回值：	g_lFault
*/
long PsGetFault()
{
	return g_lFault;
}

/*
名称：		PsSetStatus
功能：		设置系统状态，根据当前状态自动过渡到指定状态
返回值：	标准错误代码
*/
long PsSetStatus(long lStatus)
{
	switch(lStatus)
	{
		//===================================
		case STAT_IDLE:
		switch(g_lStatus)
		{
			case STAT_READY_COLD:
			case STAT_READY_HOT:
			PsMntSetStat(&g_stMnt, STAT_IDLE);
			break;
			
			case STAT_IDLE:
			break;
			
			case STAT_RUN:
			PsMntReset(&g_stMnt);
			PsProcStop(&g_stProc);
			break;
			
			case STAT_ERROR:
			default:
			return ERR_INVD;
		}
		g_lStatus = STAT_IDLE;
		break;
		//===================================
		case STAT_READY_COLD:
		return ERR_INVD;
		break;
		//===================================
		case STAT_READY_HOT:
		switch(g_lStatus)
		{
			case STAT_IDLE:
			case STAT_READY_COLD:
				PsMntSetStat(&g_stMnt, STAT_READY_HOT);
			break;
			
			case STAT_READY_HOT:
			break;
			
			case STAT_RUN:
			case STAT_ERROR:
			default:
			return ERR_INVD;
		}
		g_lStatus = STAT_READY_HOT;
		break;
		//===================================
		case STAT_RUN:
		switch(g_lStatus)
		{
			case STAT_READY_HOT:
			PsMntSetStat(&g_stMnt, STAT_RUN);
			PsProcRun(&g_stProc);
			break;
			
			case STAT_RUN:
			break;
			
			case STAT_IDLE:
			case STAT_READY_COLD:
			case STAT_ERROR:
			default:
			return ERR_INVD;
		}
		g_lStatus = STAT_RUN;
		break;
		//===================================
		case STAT_ERROR:
		switch(g_lStatus)
		{
			case STAT_IDLE:
			break;

			case STAT_READY_COLD:
			case STAT_READY_HOT:
			PsMntSetStat(&g_stMnt, STAT_ERROR);
			break;
			
			case STAT_ERROR:
			break;
			
			case STAT_RUN:
			PsMntSetStat(&g_stMnt, STAT_ERROR);
			PsProcStop(&g_stProc);
	
			break;
			
			default:
			return ERR_INVD;
		}
		g_lStatus = STAT_ERROR;
		break;
		//===================================
		default:
		return ERR_NONE;
	}
	
	return ERR_NONE;
}

/*
名称：		PsSetStatus2
功能：		设置系统状态，直接修改g_lStatus
警告：		该功能不检查当前状态，用户应小心使用
返回值：	g_lStatus
*/
long PsSetStatus2(long lStatus)
{
	return g_lStatus = lStatus;
}

/*
名称：		PsSetError
功能：		设置错误代码
返回值：	g_lError
*/
long PsSetError(long lError)
{
	return g_lError = lError;
}

/*
名称：		PsSetFault
功能：		设置设备故障代码
返回值：	g_lFault
*/
long PsSetFault(long lFault)
{
	return g_lFault = lFault;
}

/*
名称：		PsFatalError
功能：		严重错误退出
返回值：	SYS_OK
*/
int PsFatalError()
{

	PsProcStop(&g_stProc);
	exit(SYS_ER);
	
	return SYS_OK;
}

/*
名称：		PsConfig1
功能：		修改程序设置，并在完成后自动READY，列表项目数由iCount指定
返回值：	标准错误代码
*/
int PsConfig1(char **pszList, int iCount)
{
	int i,c;
	int aiTime[20];
	double adValue[20];
	switch(g_lStatus)
	{
		case STAT_IDLE:
		case STAT_READY_HOT:
		//仅在IDLE和READY_HOT下允许配置
		break;
		
		default:
		return ERR_INVD;
	}

	//验证配置项目数量的正确性
	c=iCount%2;

	if(c!=0)
	{
		return ERR_PARAM;
	}

	c=iCount/2;
	
    printf("\nCheck the number of nodes:\t%d\n",c);
    //设置波形
    printf("Wave:\tTick\tValue\n");
    for(i=0;i<c;i++)
    {
    	//由于设置时间与proc时间存在误差，需要进行校正。
    	aiTime[i]= g_stProc.dTimeFactor*atof(*(pszList++))+0.5;
    	adValue[i]=atof(*(pszList++));
    	printf("\t%4d\t%f\n",aiTime[i],adValue[i]);
    }
    
    if(PsProcConfig(&g_stProc,aiTime,adValue,c)!=ERR_NONE)
    	return ERR_PARAM;


	//检查是否配置了PID参数
	if(g_stProc.parameter.ParameterFlag != 1)
	{
		printf("\n***********************************\n");
		printf("CAUTION :Please Config Parameter!\n");
		printf("***********************************\n\n");
	}

	//PsLogAddSysInf(&g_stProc.stLog);

    return PsSetStatus(STAT_READY_HOT);
}

/*
名称：		PsConfig2
功能：		修改程序设置，并在完成后自动READY，列表项目数由iCount指定
返回值：		标准错误代码
*/
int PsConfig2(char **pszList, int iCount)
{
	
	switch(g_lStatus)
	{
		case STAT_IDLE:
		case STAT_READY_HOT:
		//仅在IDLE和READY_HOT下允许配置
		break;
		
		default:
		return ERR_INVD;
	}

	//验证配置项目数量的正确性
	if(iCount!=14)
	{
		printf("Parameter Counts !=14\n");
		printf("Counts =%d\n",iCount);
		return ERR_PARAM;
	}
	
	
	//设置参数 
	printf("\n***************************PARAMETER INFO************************\n");
	printf("*****************************************************************\n");
	
	g_stProc.parameter.plasma_flat=(atoi(*(pszList++)))+200;//由于HF比OH提前200ms触发，故加200
	g_stProc.parameter.plasma_flat=(int)(g_stProc.dTimeFactor*(g_stProc.parameter.plasma_flat)+0.5);
	printf("plasma_flat=%d;", g_stProc.parameter.plasma_flat);
	
	g_stProc.parameter.feedback_time=(atoi(*(pszList++)))+200;
	g_stProc.parameter.feedback_time=(int)(g_stProc.dTimeFactor*(g_stProc.parameter.feedback_time)+0.5);
	printf("feedback_time=%d;", g_stProc.parameter.feedback_time);
	
	g_stProc.parameter.DY_set=(atof(*(pszList++)))/100;
	printf("DY_set=%.5f;\n", g_stProc.parameter.DY_set);

	g_stProc.parameter.DY_kp_flat=atof(*(pszList++));	
	printf("DY_kp_flat=%.5f;",g_stProc.parameter.DY_kp_flat);
	
	g_stProc.parameter. DY_ki_flat=atof(*(pszList++));	
	printf("DY_ki_flat=%.5f;", g_stProc.parameter.DY_ki_flat);

	g_stProc.parameter. DY_kd_flat=atof(*(pszList++));	
	printf("DY_kd_flat=%.5f;", g_stProc.parameter.DY_kd_flat);
	
	g_stProc.parameter.DY_kip_flat=atof(*(pszList++));	
	printf("DY_kip_flat=%.5f;\n",g_stProc.parameter.DY_kip_flat);
	
	g_stProc.parameter.DY_kp_flow=atof(*(pszList++));
	printf("DY_kp_flow=%.5f;", g_stProc.parameter.DY_kp_flow);

	g_stProc.parameter.DY_ki_flow=atof(*(pszList++));
	printf("DY_ki_flow=%.5f;", g_stProc.parameter.DY_ki_flow);

	g_stProc.parameter.DY_kd_flow=atof(*(pszList++));
	printf("DY_kd_flow=%.5f;", g_stProc.parameter.DY_kd_flow);
	
	g_stProc.parameter.DY_kip_flow=atof(*(pszList++));
	printf("DY_kip_flow=%.5f;\n", g_stProc.parameter.DY_kip_flow);
	
	g_stProc.parameter.Ihf_kp=atof(*(pszList++));
	printf("Ihf_kp=%.5f;", g_stProc.parameter.Ihf_kp);

	g_stProc.parameter.Ihf_ki=atof(*(pszList++));
	printf("Ihf_ki=%.5f;", g_stProc.parameter.Ihf_ki);
	
	g_stProc.parameter.Ihf_kd=atof(*(pszList++));
	printf("Ihf_kd=%.5f;\n", g_stProc.parameter.Ihf_kd);

	printf("*****************************************************************\n\n");
	
//	//设置参数 
//	printf("\n*******************PARAMETER INFO****************\n");
//	printf("*************************************************\n");
//	
//	g_stProc.parameter.plasma_flat=(atoi(*(pszList++)))+200;//由于HF比OH提前200ms触发，故加200
//	g_stProc.parameter.plasma_flat=(int)(g_stProc.dTimeFactor*(g_stProc.parameter.plasma_flat)+0.5);
//	printf("plasma_flat=%d;", g_stProc.parameter.plasma_flat);
//		
//	g_stProc.parameter.DY_set=(atof(*(pszList++)))/100;
//	printf("DY_set=%.5f;\n", g_stProc.parameter.DY_set);
//	
//	g_stProc.parameter.DY_k=atof(*(pszList++));
//	printf("DY_k=%.5f;\n", g_stProc.parameter.DY_k);
//	
//	g_stProc.parameter.DY_kip_flat=atof(*(pszList++));	
//	printf("DY_kip_flat=%.5f;\n",g_stProc.parameter.DY_kip_flat);
//
//	g_stProc.parameter.DY_kp_flat=atof(*(pszList++));	
//	printf("DY_kp_flat=%.5f;",g_stProc.parameter.DY_kp_flat);
//	
//	g_stProc.parameter. DY_kd_flat=atof(*(pszList++));	
//	printf("DY_kd_flat=%.5f;", g_stProc.parameter.DY_kd_flat);
//	
//	g_stProc.parameter.DY_kip_flow=atof(*(pszList++));
//	printf("DY_kip_flow=%.5f;\n", g_stProc.parameter.DY_kip_flow);
//	
//	g_stProc.parameter.DY_kp_flow=atof(*(pszList++));
//	printf("DY_kp_flow=%.5f;", g_stProc.parameter.DY_kp_flow);
//
//	g_stProc.parameter.DY_kd_flow=atof(*(pszList++));
//	printf("DY_kd_flow=%.5f;", g_stProc.parameter.DY_kd_flow);
//
//	printf("\n*************************************************\n\n");
//	
//
//	//注：以下参数在原来程序中是直接在程序里赋值，
//	//      为了统一，建议从电源界面配置，这需要改电源配置界面
//	//      目前为了调试方便，就在此直接赋值
//	g_stProc.parameter.plasma_rise=(int)(g_stProc.dTimeFactor*125+0.5);
//	g_stProc.parameter.feedback_time=(int)(g_stProc.dTimeFactor*220+0.5);
//	g_stProc.parameter.DY_ki_flat=0.002;
//	g_stProc.parameter.DY_ki_flow=0.002;
//	g_stProc.parameter.Ihf_kp=0.69;//0.6;   20121117
//	g_stProc.parameter.Ihf_ki=28.77;//12;   20121117
//	g_stProc.parameter.Ihf_kd=0;

	 //将参数标志位置1，表示配置了参数
	g_stProc.parameter.ParameterFlag = 1;
	
	return ERR_NONE;
}

/*
名称：		PsReset
功能：		从STOP状态复位
返回值：	标准错误代码
*/
int PsReset()
{
	switch(g_lStatus)
	{
		case STAT_ERROR:
		g_lStatus = STAT_IDLE;
		g_lError = ERR_NONE;
		g_lFault = FT_NONE;
		PsMntReset(&g_stMnt);
		
		default:
		PsSetStatus(STAT_IDLE);
	}
	
	return  ERR_NONE;
}
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    