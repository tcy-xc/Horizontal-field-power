/*
文件：		ps_hf_global.h
内容：		定义全局变量
说明：		如果在include本文件前定义了_PS_EXTERN_FLAG，则所有全局变量将声明为外
			外部变量，同时_PS_EXTERN_FLAG会被取消定义。
更新记录：
-- Ver 1.0 / 2012-06-20
*/
#ifndef PS_GLOBAL_H_
#define PS_GLOBAL_H_

#ifdef _PS_EXTERN_FLAG
	#define _HEADER extern
#else
	#define _HEADER
#endif
#undef _PS_EXTERN_FLAG

#include "ps_hf_io.h"
#include "ps_hf_msg.h"
#include "ps_hf_mnt.h"
#include "ps_hf_proc.h"

_HEADER long g_lStatus;				//系统状态：STAT_IDLE, ...
_HEADER long g_lError;				//系统错误代码
_HEADER long g_lFault;				//设备故障代码

_HEADER ps_msg_t g_stMsg;			//Messenger对象
_HEADER ps_mnt_t g_stMnt;			//Monitor对象
_HEADER ps_proc_t g_stProc;			//Processor对象
_HEADER ps_io_t g_stIO;				//IO对象

#undef _HEADER
#endif
