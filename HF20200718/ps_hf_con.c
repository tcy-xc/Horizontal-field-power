/*
文件：		ps_hf_con.c
内容：		命令行控制台的实现
更新记录：
-- Ver 0.1 / 2011-12-01
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ps_hf_con.h"
#include "ps_hf_lib.h"

//私有函数
static int s_CmdProc(char *szCmd);		//处理输入的命令
static int s_CmdConfig(char *szmode,char *szCmd);	//将输入命令转换为程序配置参数

//合法命令名称列表
char *aszCmdList[] = 
{
	"abort",				//0, set to idle
	"config",				//1, config & set to ready
	"start",				//2, set to run
	"reset",				//3, reset from stop
	"exit",					//4, exit
	"echo"					//5, display infos
};

/*
名称：		PsConsole
功能：		进入命令行控制台模式
返回值：	SYS_OK
*/
int PsConsole()
{
	char szCmd[256];
	
	printf("\nCommand Line Console:\n");
	for(; ; )
	{
		printf("> ");
		//貌似任何按键到会被gets读到，所以不要乱按
		gets(szCmd);
		if(s_CmdProc(szCmd) == SYS_ER)
			fprintf(stderr, "ERROR: BAD COMMAND.\n");
	}
	
	return SYS_OK;
}

/*
名称：		s_CmdProc
功能：		处理输入命令
返回值：	SYS_OK, SYS_ER
*/
static int s_CmdProc(char *szCmd)
{
	int i;
	int s;
	int iCmdCount = sizeof(aszCmdList) / sizeof(char *);
	char aszArg[3][256];		//分别保存命令名与参数
	int iInCount;				//输入计数
	
	if(szCmd[0] == 0)
		return SYS_OK;			//输入为空（直接按回车）
	iInCount = sscanf(szCmd, "%s %s %s", aszArg[0], aszArg[1],aszArg[2]);
	
	for(i = 0; i < iCmdCount; i++)
	{
		if(strcmp(aszCmdList[i], aszArg[0]) == 0)
			break;
	}
	
	switch(i)
	{
		//例子：
		case 0:					//abort
		s = PsSetStatus(STAT_IDLE);
		break;
		
		case 1:					//config
		if(iInCount == 3)
			s = s_CmdConfig(aszArg[1],aszArg[2]);
		else
			return SYS_ER;
		break;
		
		case 2:					//start
		s = PsSetStatus(STAT_RUN);
		break;
		
		case 3:					//reset
		s = PsReset();
		break;
		
		case 4:					//exit
		if(PsGetStatus() == STAT_RUN)
			s = PsFatalError();
		else
			exit(SYS_OK);
		break;
		
		case 5:					//echo
		printf("Status: %ld\tError: %ld\tFault: %ld\n", PsGetStatus(), PsGetError(), PsGetFault());
		s = 0;
		break;
		
		default:
		return SYS_ER;
		break;
	}
	
	printf("Result: %d\n", s);
	return SYS_OK;
}

/*
名称：		s_CmdConfig
功能：		将输入的配置参数转换为PsConfig的参数并配置
返回值：	标准错误代码
*/
static int s_CmdConfig(char *szmode,char *szParam)
{
	int i;
	int s;
	int tmp;
	int iCnt;
	char **pszList;
	//以下照抄s_MsgSplit
	
	s=ERR_PARAM;
	//统计';'字符的个数
	for(i = 0, iCnt = 1; szParam[i] != 0; i++)
	{
		if(szParam[i] == ';')
			iCnt++;
	}
	
	//分配内存
	pszList = (char **)malloc(iCnt * sizeof(char **));
	if(pszList == NULL)
		return ERR_MEMORY;
	
	//取出参数字符串
	pszList[0] = szParam;
	for(i = 0, tmp = 0; szParam[i] != 0; i++)
	{
		if(szParam[i] == ';')
		{
			tmp++;
			pszList[tmp] = szParam + i + 1;
			szParam[i] = 0;
		}
	}
	if(strcmp("param",szmode) == 0)
		s = PsConfig2(pszList, iCnt);
	if(strcmp("wave",szmode) == 0)
		s = PsConfig1(pszList, iCnt);
	free(pszList);
	
	return s;
}
