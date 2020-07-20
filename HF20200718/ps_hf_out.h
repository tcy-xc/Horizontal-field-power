/*
文件：		ps_hf_out.h
内容：		定义out对象的数据结构与接口函数
更新记录：
-- Ver 0.9 / 2020.07.13
*/
/*
单位说明：
			1、接收到的数据
					时间均以ms为单位
					wave中的电流值以KA为单位
					parameter中的DX_set以cm为单位
			2、反馈计算的数据
					电流均以A为单位
					位移以m为单位
*/

#ifndef PS_OUT_H
#define PS_OUT_H

// Functions
void PIDoutAngle(ps_proc_t* pstObj);
void Test();

/*
说明：
PIDoutAngle  正负状态变换函数
Test 测试函数
*/

#endif