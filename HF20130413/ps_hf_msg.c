/*
文件：		ps_hf_msg.c
内容：		Messenger对象接口函数的实现
更新记录：
-- Ver 1.0 / 2012-06-20
*/
#include <sched.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "ps_hf_lib.h"
#include "ps_hf_msg.h"

//定义常参量宏
#define PARAM_MSGSIZE 4096			//默认的信息长度
#define PARAM_SEPCHAR ';'			//默认的消息分隔符

//允许的操作命令名列表
static char *s_aszNameList[] = 
{
	"echo",				//0
	"config",			//1
	"config2",			//2
	"start",			    //3
	"abort",			    //4
	"reset"				//5
};

//私有函数
static void *s_MsgThread(void *pvObj);				//主通信线程
static char *s_MsgProc(char *szRecv, char *szSend);	//消息处理函数
static int s_MsgSplit(char *szMsg, char ***ppszList);//消息分割函数
static int s_MsgCmdProc(char **pszList, int iCmdCount);//命令处理函数

/*
名称：		PsMsgInit
功能：		初始化一个Messenger
返回值：	标准错误代码
*/
int PsMsgInit(ps_msg_t *pstObj)
{
	struct sched_param stParam;
	pthread_attr_t stAttr;
	
	//初始化网络接口
	if(NetInit(&pstObj->p_stNet) == NET_ER)
		return ERR_NET_INIT;
	if(NetSetLocal(&pstObj->p_stNet, pstObj->wLocalPort) == NET_ER)
		return ERR_NET_LOCAL;
	if(NetSetRemote(&pstObj->p_stNet, pstObj->szRemoteIp, pstObj->wRemotePort) == NET_ER)
		return ERR_NET_REMOTE;
	
	//设置通讯线程的属性: detached, FIFO, priority
	pthread_attr_init(&stAttr);
	pthread_attr_setdetachstate(&stAttr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setinheritsched(&stAttr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&stAttr, SCHED_FIFO);
	stParam.sched_priority = pstObj->iPrio;
	pthread_attr_setschedparam(&stAttr, &stParam);
	
	//创建通讯线程
	pthread_create(&pstObj->p_stThreadId, &stAttr, s_MsgThread, pstObj);
	if(pstObj->p_stThreadId == NULL)
		return ERR_THREAD;
		
	return ERR_NONE;
}

/*
名称：		s_MsgerThread
功能：		主通讯线程
返回值：	NULL
*/
static void *s_MsgThread(void *pvObj)
{
	ps_msg_t *pstObj = pvObj;
	char szRecv[PARAM_MSGSIZE];					//接收到的消息
	char szSend[PARAM_MSGSIZE];					//发送的消息
	int s;

	for(; ; )
	{
		//由于接收到的消息无'\0'，需提前设置内存后才能作为字符串处理
		memset(szRecv, 0, PARAM_MSGSIZE);
		
		//接收消息
		s = NetRecv(&pstObj->p_stNet, szRecv, PARAM_MSGSIZE);
		if(s == NET_ER)
		{
			fprintf(stderr, "ERROR: NET RECV ERROR.\n");
			sleep(1);
			continue;
		}

		//处理
		s_MsgProc(szRecv, szSend);
		
		//回送消息
		s = NetSend(&pstObj->p_stNet, szSend, strlen(szSend));
		if(s == NET_ER)
		{
			fprintf(stderr, "ERROR: NET SEND ERROR.\n");
		}
	}
	
	return NULL;
}

/*
名称：		s_MsgProc
功能：		根据接收到消息的执行结果，生成对应的回送消息
返回值：	szSend
*/
static char *s_MsgProc(char *szRecv, char *szSend)
{
	char **pszCmdList;
	int c;					//消息分段计数
	int s;					//保存命令的返回值
	
	for(; ; )
	{
		//检查消息长度是否溢出
		if(szRecv[PARAM_MSGSIZE - 1] != 0)
		{
			fprintf(stderr, "ERROR: NET RECV OVERFLOW.\n");
			s = ERR_NET_OVERFLOW;
			break;
		}
		
		//分割接收到的消息
		c = s_MsgSplit(szRecv, &pszCmdList);
		if(c < 0)
		{
			s = -c;
			fprintf(stderr, "ERROR: NET MSG SPLIT ERROR.\n");
			break;
		}
		
		//处理命令
		s = s_MsgCmdProc(pszCmdList, c);
		
		//清理动态分配的内存
		free(pszCmdList);
		
		if(s != ERR_NONE)
		{
			fprintf(stderr, "ERROR: NET INVD COMMAND.\n");
			break;
		}
		
		break;
	}
	
	//生成回送的消息
	sprintf(szSend, "HF;%ld;%ld;%d", PsGetStatus(), PsGetFault(), s);
	
	return szSend;
}

/*
名称：		s_MsgSplit
功能：		将接收到的消息分割为单独的命令。一般是将消息中的分隔符';'替换为字符
			串结束符'\0'，并将每段字符串的起始位置记录到一个数组中。
注意：		本函数使用了动态分配的内存，因此要求在完成后将ppszList释放
返回值：	正数表示分解后的命令个数，负数为负的标准错误代码
*/
static int s_MsgSplit(char *szMsg, char ***ppszList)
{
	int i;
	int iIndex;
	int c;
	
	//分隔符计数
	for(c = 1, i = 0; szMsg[i] != 0; i++)	//至少有1个命令
	{
		if(szMsg[i] == PARAM_SEPCHAR)			//默认是';'
			c++;
	}
	
	//为列表分配内存
	*ppszList = (char **)malloc(c * sizeof(char **));
	if(*ppszList == NULL)
		return -ERR_MEMORY;
	
	//将字符串的地址填入列表中
	(*ppszList)[0] = szMsg;
	for(iIndex = 0, i = 0; szMsg[i] != 0; i++)
	{
		if(szMsg[i] == ';')
		{
			iIndex++;						//第iIndex个命令
			(*ppszList)[iIndex] = szMsg + i + 1;
			szMsg[i] = 0;				//替换为字符串的结束符
		}
	}
	
	return c;
}

/*
名称：		s_MsgCmdProc
功能：		处理接收到的命令
返回值：	标准错误代码
*/
static int s_MsgCmdProc(char **pszList, int iCmdCount)
{
	int i;
	int iNameCount = sizeof(s_aszNameList) / sizeof(char *);
	int s;
	
	//循环检查允许的命令列表中配置的名称
	for(i = 0; i < iNameCount; i++)
	{
		if(strcmp(pszList[1], s_aszNameList[i]) == 0)
			break;
	}
	
	switch(i)
	{
		case 0:						//Echo
		s = ERR_NONE;
		break;
		
		case 1:						//Config1
		s = PsConfig1(pszList + 2, iCmdCount - 2);
		break;
		
		case 2:						//Config2
		s = PsConfig2(pszList + 2, iCmdCount - 2);
		break;
		
		case 3:						//Go
		s = PsSetStatus(STAT_RUN);
		break;
		
		case 4:						//Abort
		s = PsSetStatus(STAT_IDLE);
		printf("Abort\n");
		break;
		
		case 5:						//Reset
		s = PsReset();
		printf("Reset\n");
		break;
		
		default:
		s = ERR_CMD_NAME;
	}
	
	return s;
}
