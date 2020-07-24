/*
文件：		pci1713.h
内容：		定义PCI1713采集卡控制的数据结构与接口函数
更新记录：
-- Ver 0.5 / 2011-07-12
*/

#ifndef PCI1713_H_
#define PCI1713_H_

#include "pci.h"

//定义通用宏
#define PCI1713_VENDOR_ID 0x13fe			//PCI1713的制造商ID
#define PCI1713_DEVICE_ID 0x1713			//PCI1713的设备ID
#define PCI1713_OPBYTE 0xff
#define PCI1713_OPWORD 0xffff
#define PCI1713_CHL 0						//最小通道号
#define PCI1713_CHH 31						//最大通道号
#define PCI1713_SET_EN 1					//启用
#define PCI1713_SET_DI 0					//禁用

//定义通道极性与增益设置宏
#define PCI1713_SET_BP05 0x00				//双极性 -5V ~ +5V
#define PCI1713_SET_BP02 0x01				//-2.5V ~ +2.5V
#define PCI1713_SET_BP01 0x02				//-1.25V ~ +1.25V
#define PCI1713_SET_BP00 0x03				//-0.625V ~ +0.625V
#define PCI1713_SET_BP10 0x04				//-10V ~ + 10V
#define PCI1713_SET_UP10 0x10				//单极性 0V ~ 10V
#define PCI1713_SET_UP05 0x11				//0V ~ 5V
#define PCI1713_SET_UP02 0x12				//0V ~ 2.5V
#define PCI1713_SET_UP01 0x13				//0V ~ 1.25V
#define PCI1713_SET_SGED 0x00				//单端(SinGle EnDed)
#define PCI1713_SET_DIFF 0x01				//差分(DIFFerential)

//定义触发设置宏
#define PCI1713_SET_SW 0x01					//软件触发
#define PCI1713_SET_PACER 0x02				//PACER触发
#define PCI1713_SET_EXT 0x04				//外部触发

//定义缓冲状态宏
#define PCI1713_FE 0x01						//FIFO 空
#define PCI1713_FH 0x02						//FIFO 半满
#define PCI1713_FF 0x04						//FIFO 满

//定义AD数据转换宏
#define PCI1713_OFFSET 0x0800				//双极性数值偏移量
#define PCI1713_URANGE 0x1000				//单极性AD范围
#define PCI1713_BRANGE 0x0800				//双极性AD范围
#define PCI1713_AMP10 10.0					//峰值10.0V，下类似
#define PCI1713_AMP05 5.0
#define PCI1713_AMP02 2.5
#define PCI1713_AMP01 1.25
#define PCI1713_AMP00 0.625

//定义采集通道控制结构体(iobase+2)
typedef union Pci1713ChannelTag
{
	struct
	{
		uint8_t x5Gain:5;					//极性及增益
		uint8_t x3Sod:3;					//单端或差分
	};
	uint8_t byParam;						//兼容参数
} pci1713_channel_t;

//定义触发控制结构体(iobase+6)
typedef union Pci1713TriggerTag
{
	struct
	{
		uint8_t x3Src:3;					//触发源
		uint8_t xGate:1;					//允许外部触发
		uint8_t xIrqen:1;					//允许中断
		uint8_t xIrqfh:1;					//允许FIFO半满中断
		uint8_t :2;							//保留位
	};
	uint8_t byParam;						//兼容参数
} pci1713_trigger_t;

//定义PCI1713对象结构体
typedef struct Pci1713Tag
{
	uint32_t r_dwIobase;					//设备IO基址
	uint8_t byChl;							//起始扫描通道号
	uint8_t byChh;							//终止扫描通道号
	pci1713_trigger_t uniTrigger;			//触发设置
	pci1713_channel_t auniChn[PCI1713_CHH - PCI1713_CHL + 1];
											//通道设置
} pci1713_t;

//接口函数
int Pci1713Init(pci1713_t *pstObj, uint16_t wIndex);
int Pci1713ChnCopy(pci1713_t *pstObj, int iSrc, int iChl, int iChh);
int Pci1713Config(const pci1713_t *pstObj);
int *Pci1713GetInt(const pci1713_t *pstObj, int *piBuf);
double *Pci1713GetDouble(const pci1713_t *pstObj, double *pdBuf);

/*
说明：
Pci1713Init			初始化PCI1713对象，初始化后才能修改设置
Pci1713ChnCopy		将iScr通道的设置复制到iChl和iChh之间的所有通道
Pci1713Config		应用PCI1713对象的设置
Pci1713GetInt		读取采集数值，以整型将各通道的采集值写入指定的缓冲区
Pci1713GetDouble	读取采集数值，以双精型将各通道的采集值写入指定的缓冲区
*/

#endif

