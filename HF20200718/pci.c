/*
文件：		pci.c
内容：		用于查找PCI设备基址的函数实现
更新记录：
-- Ver 1.0 / 2011-07-12
*/

#include <hw/pci.h>
#include <string.h>
#include <stdlib.h>
#include "pci.h"

/*
名称：		FindPciDev
功能：		查找指定PCI设备的基址
返回值：	pstObj, 或NULL错误
*/
pciinfo_t *FindPciDev(pciinfo_t *pstObj)
{
	struct pci_dev_info stInfo;
	void *pvHdl;
	int i;
	uint32_t *pdwIoList = pstObj->r_adwIobase;
	
	memset(&stInfo, 0, sizeof(stInfo));
	
	//连接到PCI Server
	if(pci_attach(0) == -1)
		return NULL;
	
	//绑定指定的PCI设备
	stInfo.VendorId = pstObj->wVendorId;
	stInfo.DeviceId = pstObj->wDeviceId;
	pvHdl = pci_attach_device(0, PCI_SHARE | PCI_INIT_ALL,
							pstObj->wIndex, 
							&stInfo);
	
	//绑定失败，则返回NULL
	if(pvHdl == NULL)
		return NULL;
	
	//获取地址列表
	for(i = 0; i < 6; i++)
	{
		if((stInfo.BaseAddressSize[i] != 0) &&
			(PCI_IS_IO(stInfo.CpuBaseAddress[i])))
		{
			*(pdwIoList++) = PCI_IO_ADDR(stInfo.CpuBaseAddress[i]);
		}
	}
	
	//断开PCI Server
	pci_detach_device(pvHdl);
	
	return pstObj;
}
