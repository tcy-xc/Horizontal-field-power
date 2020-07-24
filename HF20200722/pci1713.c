/*
文件：		pci1713.c
内容：		PCI1713采集卡接口函数的实现
更新记录：
-- Ver 0.5 / 2013-05-02
*/

#include <hw/inout.h>
#include <sys/neutrino.h>
#include "pci.h"
#include "pci1713.h"

//定义一个默认的PCI1713对象，用于初始化操作
static const pci1713_t sc_stPci1713Default = 
{
	.byChl = PCI1713_CHL,					//只启用通道0
	.byChh = PCI1713_CHL,
	.uniTrigger =
	{
		{
			.x3Src = PCI1713_SET_SW,		//目前只支持软件触发
			.xGate = PCI1713_SET_DI,		//禁止外部触发
			.xIrqen = PCI1713_SET_DI,		//禁止中断
			.xIrqfh = PCI1713_SET_DI		//禁止FIFO半满中断
		}
	},
	.auniChn =
	{
		{
			{
			.x5Gain = PCI1713_SET_BP10,		//设置通道0的增益
			.x3Sod = PCI1713_SET_SGED
			}
		}
	}
};

/*
名称：		Pci1713Init
功能：		初始化PCI1713设备。必须先初始化该对象才能设置和使用。当有多个PCI1713
			设备时使用wIndex以区分，该值从0开始。
返回值：	PCI_OK, 或PCI_ER
*/
int Pci1713Init(pci1713_t *pstObj, uint16_t wIndex)
{
	pciinfo_t stInfo;
	
	stInfo.wVendorId = PCI1713_VENDOR_ID;
	stInfo.wDeviceId = PCI1713_DEVICE_ID;
	stInfo.wIndex = wIndex;
	
	//为当前线程获取IO操作权限
	ThreadCtl(_NTO_TCTL_IO, 0);
	
	if(FindPciDev(&stInfo) == NULL)
		return PCI_ER;
	
	*pstObj = sc_stPci1713Default;
	pstObj->r_dwIobase = stInfo.r_adwIobase[0];
	Pci1713Config(pstObj);
	
	return PCI_OK;
}

/*
名称：		Pci1713ChnCopy
功能：		复制PCI1713通判配置参数，以iSrc为源通道，复制到从iChl到iChh的通道
返回值：	PCI_OK
*/
int Pci1713ChnCopy(pci1713_t *pstObj, int iSrc, int iChl, int iChh)
{
	int i;
	for(i = iChl; i <= iChh; i++)
	{
		pstObj->auniChn[i] = pstObj->auniChn[iSrc];
	}
	
	return PCI_OK;
}

/*
名称：		Pci1713Config
功能：		应用设置，使之生效
返回值：	PCI_OK
*/
int Pci1713Config(const pci1713_t *pstObj)
{
	uint8_t i;
	
	//写入触发配置
	out8(pstObj->r_dwIobase + 6, pstObj->uniTrigger.byParam);
	
	//写入通道配置
	for(i = pstObj->byChl; i <= pstObj->byChh; i++)
	{
		out8(pstObj->r_dwIobase + 4, i);
		out8(pstObj->r_dwIobase + 5, i);
		out8(pstObj->r_dwIobase + 2, pstObj->auniChn[i].byParam);
	}
	
	//写入采集通道扫描范围
	out8(pstObj->r_dwIobase + 4, pstObj->byChl);
	out8(pstObj->r_dwIobase + 5, pstObj->byChh);
	
	//清除中断及FIFO标志
	out16(pstObj->r_dwIobase + 8, PCI1713_OPWORD);
	
	return PCI_OK;
}

/*
名称：		Pci1713GetInt
功能：		将12位采集数值以整型按通道号从低到高保存到piBuf所指的内存中
返回值：	piBuf
*/
int *Pci1713GetInt(const pci1713_t *pstObj, int *piBuf)
{
	uint8_t i;
	uint8_t byStat;
	
	for(i = pstObj->byChl; i <= pstObj->byChh; i++)
	{
		//软件触发操作
		out16(pstObj->r_dwIobase, PCI1713_OPWORD);
		//等待采集完成
		do {
			byStat = in8(pstObj->r_dwIobase + 7);
		}while(byStat & PCI1713_FE == 1);
		//采集数值的首4位是通道号，需去除
		*(piBuf++) = in16(pstObj->r_dwIobase) & 0x0FFF;
	}
	
	return piBuf;
}

/*
名称：		Pci1713GetDouble
功能：		与Pci1713GetInt相似，但将采集得到的整型数值根据通道增益设置转换为浮
			点数后保存到pdBuf所指的内存中
返回值：	pdBuf
*/
double *Pci1713GetDouble(const pci1713_t *pstObj, double *pdBuf)
{
	int aiBuf[PCI1713_CHH + 1];
	uint8_t i;
	int iOffset = 0;
	int iRange = 0;
	double dAmp = 0;
	
	//首先得到整型采集值
	Pci1713GetInt(pstObj, aiBuf);
	//然后根据通道增益计算对应的浮点值
	for(i = pstObj->byChl; i <= pstObj->byChh; i++)
	{
		switch(pstObj->auniChn[i].x5Gain)
		{
			case PCI1713_SET_BP10:
			iOffset = PCI1713_OFFSET;
			iRange = PCI1713_BRANGE;
			dAmp = PCI1713_AMP10;
			break;
			
			case PCI1713_SET_BP05:
			iOffset = PCI1713_OFFSET;
			iRange = PCI1713_BRANGE;
	 		dAmp = PCI1713_AMP05;
	 		break;
	 		
	 		case PCI1713_SET_BP02:
	 		iOffset = PCI1713_OFFSET;
			iRange = PCI1713_BRANGE;
	 		dAmp = PCI1713_AMP02;
	 		break;
	 		
	 		case PCI1713_SET_BP01:
			iOffset = PCI1713_OFFSET;
			iRange = PCI1713_BRANGE;	 		
	 		dAmp = PCI1713_AMP01;
	 		break;
	 		
	 		case PCI1713_SET_BP00:
			iOffset = PCI1713_OFFSET;
			iRange = PCI1713_BRANGE;
	 		dAmp = PCI1713_AMP00;
	 		break;
	 		
	 		case PCI1713_SET_UP10:
	 		iOffset = 0;
	 		iRange = PCI1713_URANGE;
	 		dAmp = PCI1713_AMP10;
	 		break;
	 		
	 		case PCI1713_SET_UP05:
	 		iOffset = 0;
	 		iRange = PCI1713_URANGE;
	 		dAmp = PCI1713_AMP05;
	 		break;
	 		
	 		case PCI1713_SET_UP02:
	 		iOffset = 0;
	 		iRange = PCI1713_URANGE;
	 		dAmp = PCI1713_AMP02;
	 		break;
	 		
	 		case PCI1713_SET_UP01:
	 		iOffset = 0;
	 		iRange = PCI1713_URANGE;
	 		dAmp = PCI1713_AMP01;
	 		break;
			
			default:
			break;
		}
		*(pdBuf++) = (aiBuf[i - pstObj->byChl] - iOffset) * dAmp / iRange;
	}


	
	return pdBuf;
}

