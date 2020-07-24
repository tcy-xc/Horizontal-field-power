/*
文件：		ps_hf_init.c
内容：		程序初始化函数的实现
更新记录：
-- Ver 1.0 / 2012-06-20
*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "ps_hf_global.h"
#include "ps_hf_init.h"
#include "ps_hf_lib.h"

//定义常参量宏
#define PARAM_FILE "/net/main/root/ide4-workspace/HF_conf.cfg"		//配置文件路径
//#define PARAM_FILE "/home/hf_conf.cfg"		//配置文件路径
#define PARAM_CMTCHAR '#'					//配置文件注释行起始符
#define PARAM_BUFSIZE 256					//默认的缓冲区长度
#define PARAM_LISTSIZE sizeof(aszNameList)/sizeof(char *)

//配置文件中所允许的参量名称列表
static char *aszNameList[] = 
{
	//用户自定义段
	"tick_size",

	"io_an_thread_prio",
	"io_dg_thread_prio",
	"io_pci1720_defvalue0",
	"io_pci1720_defvalue1",
	"io_pci1720_defvalue2",
	"io_pci1720_defvalue3",

	"msg_thread_prio",
	"msg_remoteip",
	"msg_localport",
	"msg_remoteport",

	"mnt_thread_prio",
	"mnt_timefast",
	"mnt_timenormal",

	"proc_thread_prio",
	"proc_timenormal",
	"proc_log_level",
	"proc_log_intv",
	"proc_log_filepath"

};

//程序参量
static long s_lTickSize;					//系统响应间隔

//私有函数
static int s_DefaultConfig();				//为程序设置默认参数
static int s_ApplyConfig(char **pszParamList, int iListCount);
											//根据配置文件修改参数
static int s_LoadFile(const char *szFileName, 
						int iCount, 
						char **pszNameList, 
						char **pszParamList);//读配置文件

/*
名称：		PsInit
功能：		初始化程序
返回值：	SYS_OK, SYS_ER
*/
int PsInit()
{
	int i;
	int s;
	char *aszParamList[PARAM_LISTSIZE];					//参数指针列表
	char aszParamBuffer[PARAM_LISTSIZE][PARAM_BUFSIZE];	//参数缓存
	
	//为程序配置变量及全局变量设置默认值
	s_DefaultConfig();
	
	//清空参数缓存
	memset(aszParamBuffer, 0, PARAM_BUFSIZE * PARAM_LISTSIZE);
	
	//为参数缓存生成列表，详细说明见s_LoadFile
	for(i = 0; i < PARAM_LISTSIZE; i++)
	{
		aszParamList[i] = aszParamBuffer[i];
	}
	
	//读取配置文件
	s = s_LoadFile(PARAM_FILE, PARAM_LISTSIZE, aszNameList, aszParamList);
	switch(s)
	{
		case ERR_FILE_OPEN:
		fprintf(stderr, "ERROR: CANNOT OPEN FILE: %s.\n", PARAM_FILE);
		return SYS_ER;
		break;
		
		case ERR_PARAM:
		fprintf(stderr, "ERROR: BAD COMMAND.\n");
		return SYS_ER;
		break;
		
		case ERR_NONE:
		break;

		default:
		return SYS_ER;
		break;
	}
	
	//应用读取到的参数
	if(s_ApplyConfig(aszParamList, PARAM_LISTSIZE) != SYS_OK)
	{
		return SYS_ER;
	}
	
	//设置系统响应间隔
	s_lTickSize = TimerSetTickSize(s_lTickSize);
	if(s_lTickSize == 0)
	{
		fprintf(stderr, "ERROR: BAD TICK SIZE.\n");
		return SYS_ER;
	}
	printf("System timer clock period: %ld\n", s_lTickSize);
	
	//初始化各个对象
	s = PsIOInit(&g_stIO);
	if(s != ERR_NONE)
	{
		fprintf(stderr, "ERROR: IO FAULT, CODE: %d.\n", s);
		return SYS_ER;
	}

	s = PsMsgInit(&g_stMsg);
	if(s != ERR_NONE)
	{
		fprintf(stderr, "ERROR: MESSENGER FAULT, CODE: %d.\n", s);
		return SYS_ER;
	}

	s = PsMntInit(&g_stMnt);
	if(s != ERR_NONE)
	{
		fprintf(stderr, "ERROR: MONITOR FAULT, CODE: %d.\n", s);
		return SYS_ER;
	}
	
	s = PsProcInit(&g_stProc);
	if(s != ERR_NONE)
	{
		fprintf(stderr, "ERROR: PROCESSOR FAULT, CODE: %d.\n", s);
		return SYS_ER;
	}

	//初始化成功
	printf("All System Ready!\n\n");
	return SYS_OK;
}

/*
名称：		s_LoadFile
功能：		读取配置文件
说明：		配置文件中命令的格式是“名称 = 参数”，且“名称”和“参数”间至少有一个等
			号或空格符。其中合法的“名称”由“aszNameList”所指定，与之对应的“参数”
			会以字符串保存到“pszParamList”所指的连续内存空间中。使用“iCount”指示
			“aszNameList”中总的项目数。
			
			一行配置命令如果以PARAM_CMTCHAR（默认是#）开头，则被视为注释并跳过。
			
			非法的配置命令将被提示错误。
			
			注意pszParamList的数据类型很特殊，字符串一般使用二维数组为其预留空间，
			但本函数使用的二阶指针实际上不含有数组对应的长度信息，所以直接将二维
			数组作为参数传入是不兼容的，需使用指针数组传入并提前做以下转换：
				char *aszParamList[10];
				char tszParmList[10][80];
				aszParamList[0] = tszParmList[0];
				aszParamList[1] = tszParmList[1];
				……
			这样才能将参数传入。
			
			此外，由于文件结束检验函数的副作用，配置文件的最后一行是不会被处理的，
			因此需要在文件结尾多添加一很空行。
			
返回值：	标准错误代码
*/
int s_LoadFile(const char *szFileName, int iCount, char **pszNameList, char **pszParamList)
{
	FILE *fp;
	char szBuffer[PARAM_BUFSIZE];						//文件输入缓冲
	char szFormat[PARAM_BUFSIZE];						//格式化字符串缓冲
	int iInputCount;									//sscanf输入计数
	int iLineCount;										//配置文件行号
	int i;
	
	fp = fopen(szFileName, "r");
	if(fp == NULL)
	{
		return ERR_FILE_OPEN;
	}
	
	for(iLineCount = 0; ; )
	{
		fgets(szBuffer, PARAM_BUFSIZE, fp);
		if(feof(fp) != 0)								//判断文件尾
			break;
		iLineCount++;
		
		if(szBuffer[0] == PARAM_CMTCHAR ||				//跳过一个注释行
			szBuffer[0] == '\n')						//跳过一个空行
			continue;
			
		for(iInputCount = 0, i = 0; i < iCount; i++)	//循环检查每一个参数名称
		{
			sprintf(szFormat, "%s%%*[ =]%%s", pszNameList[i]);
			iInputCount += sscanf(szBuffer, szFormat, pszParamList[i]);
		}
		
		if(iInputCount == 0)							//如果没有与之匹配的名称
		{
			printf("[%d]%s", iLineCount, szBuffer);		//输出当前行并报错
			fclose(fp);
			return ERR_PARAM;
		}
	}
	
	fclose(fp);
	return ERR_NONE;
}

/*
名称：		s_DefaultConfig
功能：		为程序设置各参数的默认值
返回值：		SYS_OK
*/
static int s_DefaultConfig()
{
	s_lTickSize = 50000;						//50us
	
	//用户代码段
	g_lStatus = STAT_IDLE;
	g_lError = ERR_NONE;
	g_lFault = FT_NONE;
	
	g_stIO.stConf.iAnPrio = 28;
	g_stIO.stConf.iDgPrio = 30;
	g_stIO.stConf.adDefAnValue[0] = 0.410447611940;//5.0*146/134-135*5.0/134.0
	g_stIO.stConf.adDefAnValue[1] = 0.410447611940;
	g_stIO.stConf.adDefAnValue[2] = 0.410447611940;
	g_stIO.stConf.adDefAnValue[3] = 0.410447611940;
	g_stIO.stConf.stDefDgValue.wValue = 0;
	
	strcpy(g_stMsg.szRemoteIp, "192.168.1.99");
	g_stMsg.wLocalPort = 10005;
	g_stMsg.wRemotePort = 10006;
	g_stMsg.iPrio = 26;
	g_stMsg.pstIO = &g_stIO;
	
	g_stMnt.iPrio = 30;
	g_stMnt.lTimeFast = 50000;					//50us
	g_stMnt.lTimeNormal = 1000000;		//1ms
	g_stMnt.pstIO = &g_stIO;
	
	g_stProc.iPrio = 28;
	g_stProc.lTimeNormal = 833333;		//1200HZ
	g_stProc.pstIO = &g_stIO;
	g_stProc.stLog.iLogLevel =PS_LOG_STDO;
	g_stProc.stLog.iIdxIntv=10;				//10ms输出间隔
	g_stProc.dTimeFactor=1;		

	return SYS_OK;
}

/*
名称：		s_ApplyConfig
功能：		应用配置文件到程序参数
返回值：		SYS_OK, SYS_ER
*/
static int s_ApplyConfig(char **pszParamList, int iListCount)
{
	int i;
	
	for(i = 0; i < iListCount; i++)
	{
		if(pszParamList[i][0] == 0)				//该参量为空值
			continue;
		switch(i)
		{
			case 0:
			//tick_size
			s_lTickSize =atol(pszParamList[i]);
			printf("tick_size = %ld\n", s_lTickSize);
			break;
			
			case 1:
			//io_an_thread_prio
			g_stIO.stConf.iAnPrio = atoi(pszParamList[i]);
			printf("io_an_thread_prio= %d\n", g_stIO.stConf.iAnPrio);
			break;

			case 2:
			//io_dg_thread_prio
			g_stIO.stConf.iDgPrio = atoi(pszParamList[i]);
			printf("io_dg_thread_prio= %d\n", g_stIO.stConf.iDgPrio);
			break;
			
			case 3:
			//io_pci1720_defvalue0
			g_stIO.stConf.adDefAnValue[0] = atof(pszParamList[i]);
			printf("io_pci1720_defvalue0 = %f\n", g_stIO.stConf.adDefAnValue[0]);
			break;
			
			case 4:
			//io_pci1720_defvalue1
			g_stIO.stConf.adDefAnValue[1] = atof(pszParamList[i]);
			printf("io_pci1720_defvalue1 = %f\n", g_stIO.stConf.adDefAnValue[1]);
			break;
			
			case 5:
			//io_pci1720_defvalue2
			g_stIO.stConf.adDefAnValue[2] = atof(pszParamList[i]);
			printf("io_pci1720_defvalue2 = %f\n", g_stIO.stConf.adDefAnValue[2]);
			break;
			
			case 6:
			//io_pci1720_defvalue3
			g_stIO.stConf.adDefAnValue[3] = atof(pszParamList[i]);
			printf("io_pci1720_defvalue3 = %f\n", g_stIO.stConf.adDefAnValue[3]);
			break;

			case 7:
			//msg_thread_prio
			g_stMsg.iPrio =  atoi(pszParamList[i]);
			printf("msg_thread_prio= %d\n",g_stMsg.iPrio);
			break;

			case 8:
			//msg_remoteip
			strcpy(g_stMsg.szRemoteIp, pszParamList[i]);
			printf("msg_remoteip= %s\n", g_stMsg.szRemoteIp);
			break;
			
			case 9:
			//msg_localport
			g_stMsg.wLocalPort =  atoi(pszParamList[i]);
			printf("msg_localport= %d\n", g_stMsg.wLocalPort);
			break;

			case 10:
			//msg_remoteport
			g_stMsg.wRemotePort =  atoi(pszParamList[i]);
			printf("msg_remoteport= %d\n", g_stMsg.wRemotePort);
			break;

			case 11:
			//mnt_thread_prio
			g_stMnt.iPrio = atoi(pszParamList[i]);
			printf("mnt_thread_prio= %d\n", g_stMnt.iPrio);
			break;

			case 12:
			//mnt_timefast
			g_stMnt.lTimeFast = atol(pszParamList[i]);
			printf("mnt_timefast= %ld\n", g_stMnt.lTimeFast);
			break;

			case 13:
			//mnt_timenormal
			g_stMnt.lTimeNormal = atol(pszParamList[i]);
			printf("mnt_timenormal= %ld\n", g_stMnt.lTimeNormal);
			break;

			case 14:
			//proc_thread_prio
			g_stProc.iPrio = atoi(pszParamList[i]);
			printf("proc_thread_prio= %d\n", g_stProc.iPrio);
			break;
			
			case 15:
			//proc_timenormal
			g_stProc.lTimeNormal = atol(pszParamList[i]);
			printf("proc_timenormal= %ld\n", g_stProc.lTimeNormal);
			break;

			case 16:
			//proc_log_level
			g_stProc.stLog.iLogLevel =atoi(pszParamList[i]);
			printf("proc_log_level= %d\n", g_stProc.stLog.iLogLevel);
			break;

			case 17:
			//proc_log_intv
			g_stProc.stLog.iIdxIntv=atoi(pszParamList[i]);
			printf("proc_log_intv= %d\n", g_stProc.stLog.iIdxIntv);
			break;
			
			case 18:
			//proc_log_filepath
			strcpy(g_stProc.stLog.szFilePath, pszParamList[i]);
			printf("proc_log_filepath= %s\n", g_stProc.stLog.szFilePath);
			break;


			default:
			break;
		}
	}
	
	return SYS_OK;
}
