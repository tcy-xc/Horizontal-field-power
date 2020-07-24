/*
文件：		ps_hf_log.c
内容：		Log对象接口函数的实现
更新记录：
-- Ver 0.9 / 2011-11-25
*/

#include <stdio.h>
#include "ps_hf_log.h"
#include "ps_hf_proc.h"
#include "ps_hf_global.h"
#include <stdlib.h>
#include <string.h>

//私有函数
static int s_LogStdo(ps_log_t *pstObj);		//记录到stdout
static int s_LogFile(ps_log_t *pstObj);		//记录到文件

/*
名称：		PsLogAdd
功能：		添加一条数据记录
返回值：	SYS_OK
*/
int PsLogAdd(ps_log_t *pstObj)
{
	
	ps_proc_t *pstProc;
	
	if(pstObj->iLogLevel == PS_LOG_NOLOG)
		return SYS_OK;
	
	pstProc= pstObj->pvPsor;	
	
	//记录相关计算量
	pstObj->astData[pstObj->lIndex].Ip = pstProc->cal.Ip[1];
	pstObj->astData[pstObj->lIndex].DY_set=pstProc->parameter.DY_set;
	pstObj->astData[pstObj->lIndex].DY_real=pstProc->cal.DY[1];
	pstObj->astData[pstObj->lIndex].rogowski=pstProc->cal.rogowski;
	pstObj->astData[pstObj->lIndex].saddle=pstProc->cal.saddle;
	pstObj->astData[pstObj->lIndex].angle[0]=pstProc->cal.angle[0];
	pstObj->astData[pstObj->lIndex].angle[1]=pstProc->cal.angle[1];
	pstObj->astData[pstObj->lIndex].Ihf_set=pstProc->cal.Ihf_set;
	pstObj->astData[pstObj->lIndex].Ihf_real=pstProc->cal.Ihf_real;
	pstObj->astData[pstObj->lIndex].Uhf_ac[0]=pstProc->cal.Uhf_ac[0];
	pstObj->astData[pstObj->lIndex].Uhf_ac[1]=pstProc->cal.Uhf_ac[1];
	pstObj->astData[pstObj->lIndex].Uhf_dc=pstProc->cal.Uhf_dc;
	pstObj->astData[pstObj->lIndex].DY_err_p=pstProc->cal.DY_err_p;
	pstObj->astData[pstObj->lIndex].DY_err_i=pstProc->cal.DY_err_i;
	pstObj->astData[pstObj->lIndex].DY_err_d=pstProc->cal.DY_err_d;
	pstObj->astData[pstObj->lIndex].Ihf_err_p=pstProc->cal.Ihf_err_p;
	pstObj->astData[pstObj->lIndex].Ihf_err_i=pstProc->cal.Ihf_err_i;
	pstObj->astData[pstObj->lIndex].Ihf_err_d=pstProc->cal.Ihf_err_d;
	
	pstObj->astData[pstObj->lIndex].I_DRMP_DC2=pstProc->cal.I_DRMP_DC2;
	pstObj->astData[pstObj->lIndex].Ihf_feedback_init=pstProc->cal.Ihf_feedback_init;//add by zpchen at 20160516
	pstObj->astData[pstObj->lIndex].time=pstProc->lRunCount;

	
	//记录PCI1750卡输入输出
	pstObj->astData[pstObj->lIndex].p_stIn.wValue=g_stIO.r_stDgInValue.wValue;
	pstObj->astData[pstObj->lIndex].p_stOut.wValue=g_stIO.w_stDgOutValue.wValue;

	//索引号自动增加
	pstObj->lIndex++;
	return SYS_OK;
}

///*
//名称：		PsLogAddSysInf
//功能：		添加系统信息记录
//返回值：	SYS_OK
//*/
//int PsLogAddSysInf(ps_log_t *pstObj)
//{
//	FILE *pfFile;
//	int i;
//	char szTemp[256];
//
//	//创建文件
//
//	sprintf(szTemp, "%s", pstObj->szFilePath);	
//	printf("Logging sys_inf to file %s...\n", szTemp);
//
//	pfFile = fopen(pstObj->szFilePath, "w");
//	if(pfFile == NULL)
//	{
//		fprintf(stderr, "ERROR: CAN'T OPEN FILE \"%s\"\n", pstObj->szFilePath);
//		return ERR_FILE_OPEN;
//	}
//
////	fprintf(pfFile,"shot time \n",     );
////	fprintf(pfFile,"parameter  \n",     );
//
//	fclose(pfFile);
//	return ERR_NONE;
//	
//}

/*
名称：		PsLogOut
功能：		输出记录
返回值：	标准错误代码
*/
int PsLogOut(ps_log_t *pstObj)
{
	int s;
	
	switch(pstObj->iLogLevel)
	{
		case PS_LOG_NOLOG:
		s = ERR_NONE;
		break;
		
		//输出到屏幕
		case PS_LOG_STDO:
		s = s_LogStdo(pstObj);
		break;
		
		//输出到文件
		case PS_LOG_FILE:
		s = s_LogFile(pstObj);
		break;
		
		//全部输出
		case PS_LOG_ALL:
		s = s_LogStdo(pstObj);
		if(s != ERR_NONE)
			break;
		s = s_LogFile(pstObj);
		break;
		
		default:
		s = ERR_PARAM;
		break;
	}
	//重置索引号
	pstObj->lIndex = 0;
	
	return s;
}

/*
名称：		s_LogStdo
功能：		输出到屏幕
返回值：	标准错误代码
*/
static int s_LogStdo(ps_log_t *pstObj)
{
	int i;
	
	if(pstObj->iIdxIntv <= 0)
	{
		return ERR_PARAM;
	}
	
	for(i = 0; i < pstObj->lIndex; i += pstObj->iIdxIntv)
	{
		/***comment for test2020
			printf("Ip=%.2f;\t",pstObj->astData[i].Ip); 
			printf("DY=%.2f;\t",pstObj->astData[i].DY_real);
			printf("Rogowski=%.2f;\t",pstObj->astData[i].rogowski);
			printf("Saddle=%.2f;\t",pstObj->astData[i].saddle);
			printf("Angel[0]=%.2f;\t",pstObj->astData[i].angle[0]);
			printf("Ihf_set=%.2f;\t",pstObj->astData[i].Ihf_set);
			printf("Ihf_real=%.2f;\t",pstObj->astData[i].Ihf_real);		
			printf("time=%ld;\n",pstObj->astData[i].time);
			
			***/
	}
	

	
	return ERR_NONE;
}

/*
名称：		s_LogFile
功能：		输出到文件
返回值：	标准错误代码
*/
static int s_LogFile(ps_log_t *pstObj)
{
	FILE *pfFile;
	int i;
	ps_proc_t *pstProc = pstObj->pvPsor;
    
    char filename[256];
	char dirName[100];
	int b;//dirname
	char d[10];//save dirname
    char num[20];//shotnum

    //process shotnumb
	 b = pstProc->lShotNumber / 1000;//dirname
	 printf("shotnumber=%ld\n",pstProc->lShotNumber);
	//create dir
    strcpy(dirName, "/net/main/LOG/HF_log/");//save path
    
	itoa(b, d,10);
	itoa(pstProc->lShotNumber,num,10);
	strcat(dirName,d); //copy d to dirname
    strcat(dirName,"000/");
    mkdir(dirName, S_IRWXU);//create new file
	printf("dirname=%s\n",dirName);//print for test add by xjy20190513
    //new file
	strcpy(filename, dirName);
	strcat(filename, "/hflog");
	strcat(filename,num);
	strcat(filename, ".txt");
	//open file
	pfFile = fopen(filename, "w");


//	pfFile = fopen(pstObj->szFilePath, "w");
	if(pfFile == NULL)
	{
		fprintf(stderr, "ERROR: CAN'T OPEN FILE \"%s\"\n", pstObj->szFilePath);
		return ERR_FILE_OPEN;
	}

	for(i = 0; i < pstObj->lIndex; i++)
	{
			if(pstObj->astData[i].Ihf_real<=4&&pstObj->astData[i].Ihf_real>=0)
			{
				fprintf(pfFile,"check middle state\n");
			}
			fprintf(pfFile,"Time=%7.2f\t",pstObj->astData[i].time/g_stProc.dTimeFactor);
			fprintf(pfFile,"  Ip=%9.3f\t",pstObj->astData[i].Ip); 
			fprintf(pfFile,"       Rogowski=%7.3f\t",pstObj->astData[i].rogowski);
			fprintf(pfFile,"    Saddle=%7.3f\t\n",pstObj->astData[i].saddle);

			fprintf(pfFile,"              PCI1750 IN=%u\t  ",pstObj->astData[i].p_stIn.wValue); 
			fprintf(pfFile," PCI1750 OUT=%u\t\n",pstObj->astData[i].p_stOut.wValue); 


		    fprintf(pfFile,"              Ihf_set=%8.2f\t",pstObj->astData[i].Ihf_set);
			fprintf(pfFile,"   Ihf_real=%8.2f\t",pstObj->astData[i].Ihf_real);
			fprintf(pfFile,"   Ihf_err_p=%8.4f\t",pstObj->astData[i].Ihf_err_p);    //modify 2 to 4
			fprintf(pfFile,"   Ihf_err_i=%8.4f\t",pstObj->astData[i].Ihf_err_i);
			fprintf(pfFile,"   Ihf_err_d=%8.4f\t\n",pstObj->astData[i].Ihf_err_d);

			fprintf(pfFile,"              DY_set=%8.4f\t",pstObj->astData[i].DY_set);
			fprintf(pfFile,"    DY_real=%8.4f\t",pstObj->astData[i].DY_real);
			fprintf(pfFile,"    DY_err_p=%8.4f\t",pstObj->astData[i].DY_err_p);
			fprintf(pfFile,"    DY_err_i=%8.6f\t",pstObj->astData[i].DY_err_i);
			fprintf(pfFile,"    DY_err_d=%8.4f\t\n",pstObj->astData[i].DY_err_d);   // modify 2 to 4

			fprintf(pfFile,"              Uhf_ac[0]=%5.2f\t",pstObj->astData[i].Uhf_ac[0]);
			fprintf(pfFile,"    Uhf_dc=%5.2f\t",pstObj->astData[i].Uhf_dc);
			fprintf(pfFile,"        Angel[0]=%5.2f\t",pstObj->astData[i].angle[0]);
			fprintf(pfFile,"        Angel[1]=%5.2f\t\n",pstObj->astData[i].angle[1]);//add 2020by xjy
			
			
			fprintf(pfFile,"              I_DRMP_DC2=%5.3f\t\n",pstObj->astData[i].I_DRMP_DC2);
//			fprintf(pfFile,"              Ihf_feedback_init=%5.3f\n",pstObj->astData[i].Ihf_feedback_init);
	
	}

	fclose(pfFile);
	return ERR_NONE;
}


