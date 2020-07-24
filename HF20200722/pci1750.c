/*
文件：		pci1750.c
内容：		PCI1750接口函数的实现
更新记录：
-- Ver 0.6 / 2011-11-04
*/

#include <hw/inout.h>
#include <sys/neutrino.h>
#include "pci1750.h"
#include "pci.h"

/*
名称：		Pci1750Init
功能：		初始化一个PCI1750对象，然后才能使用。以wIndex区分相同的设备，从0开始
返回值：	PCI_OK, PCI_ER
*/
int Pci1750Init(pci1750_t *pstObj, uint16_t wIndex)
{
	pciinfo_t stInfo;
	
	stInfo.wVendorId = PCI1750_VENDOR_ID;
	stInfo.wDeviceId = PCI1750_DEVICE_ID;
	stInfo.wIndex = wIndex;
	
	//为当前线程获取IO操作权限
	ThreadCtl(_NTO_TCTL_IO, 0);
	
	if(FindPciDev(&stInfo) == NULL)
		return PCI_ER;
	pstObj->r_dwIobase = stInfo.r_adwIobase[0];
	
	return PCI_OK;
}

/*
名称：		Pci1750Get
功能：		读取数字量
返回值：	输入值
*/
uint16_t Pci1750Get(const pci1750_t *pstObj)
{
	return in16(pstObj->r_dwIobase);
}

/*
名称：		Pci1750Set
功能：		输出数字量
返回值：	wValue
*/
uint16_t Pci1750Set(const pci1750_t *pstObj, uint16_t wValue)
{
	out16(pstObj->r_dwIobase, wValue);
	return wValue;
}
