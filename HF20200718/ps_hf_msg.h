/*
文件：		ps_hf_msg.h
内容：		定义Messenger对象数据结构与接口函数
更新记录：
-- Ver 0.9 / 2011-11-20
*/
#ifndef PS_MSG_H_
#define PS_MSG_H_

#include <pthread.h>
#include "network.h"
#include "ps_hf_system.h"
#include "ps_hf_io.h"

//定义Messenger对象数据结构
typedef struct PsMessengerTag
{
	unsigned wLocalPort;			//本机端口
	unsigned wRemotePort;			//远程主机端口
	char szRemoteIp[20];			//远程主机IP
	int iPrio;						//通讯线程优先级
	ps_io_t *pstIO;					//IO对象指针
	//以上参数要求在Messenger初始化以前完成初始化，并不宜再次改动
	
	net_int_t p_stNet;				//网络接口
	pthread_t p_stThreadId;			//通讯线程ID
} ps_msg_t;

//接口函数
int PsMsgInit(ps_msg_t *pstObj);	//初始化Messenger对象
#endif
