/*
文件：		pci1720.c
内容：		PCI1720控制函数的实现
更新记录：
-- Ver 0.5 / 2011-11-16
*/

#include <hw/inout.h>
#include <sys/neutrino.h>
#include "pci.h"
#include "pci1720.h"

//定义一个默认的PCI1720控制对象，用于初始化
static const pci1720_t sc_stPci1720Default = 
{
	.bySync = PCI1720_SET_DI,				//禁用同步
	.uniChn =
	{
		{
			.x2Ch0 = PCI1720_SET_UP05,		//所有通道设置为单极性0-5V
			.x2Ch1 = PCI1720_SET_UP05,
			.x2Ch2 = PCI1720_SET_UP05,
			.x2Ch3 = PCI1720_SET_UP05,
		}
	}
};

/*
名称：		Pci1720Init
功能：		初始化一个Pci1720对象，用wIndex=0表示第一个设备
返回值：	PCI_OK, 或者PCI_ER
*/
int Pci1720Init(pci1720_t *pstObj, uint16_t wIndex)
{
	pciinfo_t stInfo;
	
	stInfo.wVendorId = PCI1720_VENDOR_ID;
	stInfo.wDeviceId = PCI1720_DEVICE_ID;
	stInfo.wIndex = wIndex;
	
	//为当前线程获取IO操作权限
	ThreadCtl(_NTO_TCTL_IO, 0);
	
	if(FindPciDev(&stInfo) == NULL)
		return PCI_ER;
	*pstObj = sc_stPci1720Default;
	//PCI1720使用列表中第二个地址作为基址
	pstObj->r_dwIobase = stInfo.r_adwIobase[1];
	Pci1720Config(pstObj);
	
	return PCI_OK;
}

/*
名称：		Pci1720Config
功能：		将配置应用的设备上
返回值：	PCI_OK
*/
int Pci1720Config(const pci1720_t *pstObj)
{
	out8(pstObj->r_dwIobase + 8, pstObj->uniChn.byParam);
	out8(pstObj->r_dwIobase + 15, pstObj->bySync);
	
	return PCI_OK;
}

/*
名称：		Pci1720SetDouble
功能：		以浮点型参数设置设备输出，通道由iChn指定
返回值：	dValue
*/
double Pci1720SetDouble(const pci1720_t *pstObj, int iChn, double dValue)
{
	uint16_t wValue;
	int iOffset = 0;
	int iRange = PCI1720_URANGE;
	double dAmp = PCI1720_AMP05;
	uint8_t byChnSet;
	
	//获取指定通道的增益设置，并根据设置自动转换为整型值
	byChnSet = (pstObj->uniChn.byParam >> (iChn * 2)) & 0x03;
	switch(byChnSet)
	{
		case PCI1720_SET_UP05:
		break;
		
		case PCI1720_SET_UP10:
		dAmp = PCI1720_AMP10;
		break;
		
		case PCI1720_SET_BP05:
		iOffset = PCI1720_OFFSET;
		iRange = PCI1720_BRANGE;
		break;
		
		case PCI1720_SET_BP10:
		iOffset = PCI1720_OFFSET;
		iRange = PCI1720_BRANGE;
		dAmp = PCI1720_AMP10;
		break;
		
		default:
		break;
	}
	
	wValue = iRange * dValue / dAmp + 0.5 + iOffset;
	//将超出表示范围的数值转换为正确结果
	if(wValue > 0x7FFF)
		wValue = 0;
	if(wValue > 0xFFF)
		wValue = 0xFFF;
	
	//以整型参数设置设备输出，通道由iChn指定
	out16(pstObj->r_dwIobase + iChn * 2, wValue);

	return dValue;
}

