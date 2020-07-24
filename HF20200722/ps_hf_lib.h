/*
文件：		ps_hf_lib.h
内容：		定义电源控制库函数
说明：		原则上讲只有PsInit和本库内的函数允许访问全局变量，但PsInit实现略为
			复杂，且作用层次不同，所以单独编写。
更新记录：
-- Ver 0.9 / 2011-12-1
*/

#ifndef PS_LIB_H_
#define PS_LIB_H_

#define _PS_EXTERN_FLAG
#include "ps_hf_global.h"

// Functions
long PsGetStatus();
long PsGetError();
long PsGetFault();

long PsSetStatus(long lStatus);	
long PsSetStatus2(long lStatus);
long PsSetError(long lError);
long PsSetFault(long lFault);

int PsConfig1(char **pszList, int iCount);
int PsConfig2(char **pszList, int iCount);
int PsReset();
int PsFatalError();

/*
说明：
PsGetStatus		获取系统状态代码
PsGetError		获取系统错误代码
PsGetFault		获取设备故障代码
PsSetStatus		设置系统状态，根据当前状态自动过渡到指定状态
PsSetStatus2	设置系统状态，直接修改g_lStatus
PsSetError		设置系统错误代码
PsSetFault		设置设备故障代码
PsConfig		修改程序设置，并自动ready
PsReset			从STAT_ERROR复位
PsFatalError	严重错误退出
*/

#endif
