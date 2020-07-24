/*
文件：		pci1750.h
内容：		定义PCI1750对象的数据结构接口函数
更新记录：
-- Ver 0.6 / 2011-11-04
*/

#ifndef PCI1750_H_
#define PCI1750_H_

#include "pci.h"

//定义通用宏
#define PCI1750_VENDOR_ID 0x13FE
#define PCI1750_DEVICE_ID 0x1750

//定义输入电平宏
//当输入为低电平时，寄存器上得到数据为0,反之为1
#define PCI1750_IN_LO 0					//输入低电平
#define PCI1750_IN_HI 1					//输入高电平

//定义输出电平宏
//由于输出使用共集电极设计，设备需需外接上拉电源，且输出反向。即，寄存器上为0时
//对应高电平输出。
#define PCI1750_OUT_LO 1				//输出低电平
#define PCI1750_OUT_HI 0				//输出高电平

//定义PCI1750输入输出数据位域结构类型
typedef union Pci1750ValueTag
{
	struct
	{
		uint16_t xB0:1;
		uint16_t xB1:1;
		uint16_t xB2:1;
		uint16_t xB3:1;
		uint16_t xB4:1;
		uint16_t xB5:1;
		uint16_t xB6:1;
		uint16_t xB7:1;
		uint16_t xB8:1;
		uint16_t xB9:1;
		uint16_t xB10:1;
		uint16_t xB11:1;
		uint16_t xB12:1;
		uint16_t xB13:1;
		uint16_t xB14:1;
		uint16_t xB15:1;
	};
	uint16_t wValue;						//兼容参数
} pci1750_val_t;

//定义PCI1750对象控制数据结构
typedef struct Pci1750Tag
{
	uint32_t r_dwIobase;
} pci1750_t;

//接口函数
int Pci1750Init(pci1750_t *pstObj, uint16_t wIndex);
uint16_t Pci1750Get(const pci1750_t *pstObj);
uint16_t Pci1750Set(const pci1750_t *pstObj, uint16_t wValue);

/*
说明：
Pci1750Init		初始化PCI1750设备，初始化后才能进行使用。当存在多个相同设
				备时使用wIndex以区分，0为第一个设备。
Pci1750Get		读取数字信号
Pci1750Set		输出数字信号
*/
#endif
