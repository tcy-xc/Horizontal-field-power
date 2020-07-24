/*
文件：		pci1720.h
内容：		定义一个用于Pci1720卡控制的对象及其接口函数
更新记录：
-- Ver 0.5 / 2011-07-21
*/

#ifndef PCI1720_H_
#define PCI1720_H_

#include "pci.h"

//定义通用宏
#define PCI1720_VENDOR_ID 0x13FE
#define PCI1720_DEVICE_ID 0x1720
#define PCI1720_OPBYTE 0xFF				
#define PCI1720_CHL 0					//起始通道号
#define PCI1720_CHH 3					//终止通道号
#define PCI1720_SET_EN 1				//启用
#define PCI1720_SET_DI 0				//禁用

//定义通道增益宏
#define PCI1720_SET_UP05 0x00			//单极性 0~5V
#define PCI1720_SET_UP10 0x01			//0~10V
#define PCI1720_SET_BP05 0x02			//双极性 -5V~5V
#define PCI1720_SET_BP10 0x03			//-10V~10V

//定义输出转换宏
#define PCI1720_OFFSET 0x800			//双极性输出偏移
#define PCI1720_URANGE 0x1000			//单极性数值范围
#define PCI1720_BRANGE 0x0800			//双极性数值范围
#define PCI1720_AMP10 10.0
#define PCI1720_AMP05 5.0

//定义PCI1720输出使用的数据类型
typedef union Pci1720DataTag
{
	struct
	{
		//手册要求写寄存器时先写低字节，再写高字节，但实测用out16输出并无不妥
		uint8_t byLo;					//低字节
		uint8_t byHi;					//高字节
	};
	struct
	{
		uint16_t x12Value:12;			//12位DA值
		uint16_t :4;					//保留位
	};
	uint16_t wParam;					//兼容参数
} pci1720_data_t;

//定义PCI1720通道设置数据结构
typedef union Pci1720ChannelTag
{
	struct
	{
		uint8_t x2Ch0:2;				//通道0的输出增益设置
		uint8_t x2Ch1:2;
		uint8_t x2Ch2:2;
		uint8_t x2Ch3:2;
	};
	uint8_t byParam;					//兼容参数
} pci1720_chn_t;

//定义PCI1720对象的控制结构
typedef struct Pci1720Tag
{
	uint32_t r_dwIobase;
	uint8_t bySync;						//启用/禁用同步
	pci1720_chn_t uniChn;				//通道输出增益设置
} pci1720_t;

//接口函数
int Pci1720Init(pci1720_t *pstObj, uint16_t wIndex);
int Pci1720Config(const pci1720_t *pstObj);
double Pci1720SetDouble(const pci1720_t *pstObj, int iChn, double dValue);

/*
说明：
Pci1720Init			初始化函数，PCI1720对象须经初始化后才能进行设置与操作，当存
					在多个相同设备时使用wIndex以区分，0表示第一个设备。
Pci1720Config		应用修改的配置
Pci1720SetDouble	以浮点格式输出，通道由iChn指定
*/

#endif
