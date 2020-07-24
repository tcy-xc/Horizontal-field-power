/*
文件：		pci.h
内容：		用于查找PCI设备基址的函数声明
更新记录：
-- Ver 1.0 / 2011-07-12
*/

#ifndef PCI_H_
#define PCI_H_

#include <stdint.h>

//错误代码
#define PCI_OK 0
#define PCI_ER -1

//定义一个PCI设备信息结构体
typedef struct PCIInfoTag
{
	uint16_t wVendorId;			//制造商ID
	uint16_t wDeviceId;			//设备ID
	uint16_t wIndex;			//设备序号
	uint32_t r_adwIobase[6];	//IO地址列表
} pciinfo_t;

//接口函数
pciinfo_t *FindPciDev(pciinfo_t *pstObj);			//查找指定设备的IO地址

/*
说明：
使用前先填入wVendorId, wDeviceId, wIndex。当主机中安装有多个相同设备时，令wIndex
为0以指明第一块设备。结果保存在r_adwIobase里。
*/

#endif
