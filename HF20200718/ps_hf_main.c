/*
文件：		ps_hf_main.c
内容：		电源程序主函数的实现
更新记录：
-- Ver 1.0 / 2012-06-20
*/

#include <stdio.h>
#include "ps_hf_system.h"
#include "ps_hf_main.h"
#include "ps_hf_init.h"
#include "ps_hf_con.h"
#include"ps_hf_out.h"

int main(int argc, char *argv[])
{
	PsMain();
	return 0;
}

/*
名称：		PsMain()
功能：		电源程序的主函数
返回值：	SYS_OK, SYS_ER
*/

int PsMain()
{
	printf("\n\n***************SYSTEM INFO***************\n");
	printf("***********This is HF20130413!***********\n");
	Test();
	printf("*****************************************");
	printf("\n%s\nVersion: %s @ %s\n%s\n", INFO_NAME, INFO_VERSION, INFO_DATE, INFO_COPYRIGHT);
	printf("*****************************************\n\n");
	if(PsInit() == SYS_ER)		//初始化
		return SYS_ER;
	PsConsole();						//进入命令行控制台
	return SYS_OK;
}


