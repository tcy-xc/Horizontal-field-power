#include"ps_hf_out.h"
#include"ps_hf_proc.h"
#include"ps_hf_io.h"
#include "pci1750.h"
#include<math.h>
#include <stdio.h>

static int plusCount;   //正向计数
static int minusCount;     //负向计数
static int sign;   //正负标志位

static double PsProcCurrentPID(ps_proc_t *pstObj);


void PIDoutAngle(ps_proc_t *pstObj)
{
	double plusmin = 3;
	double minusmax = -3;

	if (pstObj->cal.Ihf_set == 0)
	{
		pstObj->ang[0] = 135;
		pstObj->ang[1] = 135;   
		plusCount = 0;
		minusCount = 0;
	}
	else if (pstObj->cal.Ihf_real > plusmin&& pstObj->cal.Ihf_set > 0)
	{
		sign = 0;
		pstObj->ang[0] = PsProcCurrentPID(pstObj);
		pstObj->ang[1] = 135;
		plusCount = 0;
		minusCount = 0;                                   //电源1封锁
	}
	else if (pstObj->cal.Ihf_real > plusmin&& pstObj->cal.Ihf_set < 0)
	{
		pstObj->ang[0] = 135;
		pstObj->ang[1] = 135;
		plusCount = 0; 
		minusCount = 0;
	}           //电源1封锁
	else if (pstObj->cal.Ihf_real < minusmax&&pstObj->cal.Ihf_set>0)
	{
		pstObj->ang[0] = 135;
		pstObj->ang[1] = 135;
		plusCount = 0; 
		minusCount = 0;
	}             //此时电源0应该封锁
	else if (pstObj->cal.Ihf_real < minusmax && pstObj->cal.Ihf_set<0)
	{
		sign = 1;
		pstObj->ang[0] = 135;
		pstObj->ang[1] = PsProcCurrentPID(pstObj);
		plusCount = 0;
		minusCount = 0;
	}
	else if (pstObj->cal.Ihf_real >= minusmax && pstObj->cal.Ihf_real <= plusmin) //中间状态
	{
		if (pstObj->cal.Ihf_set > 0)
		{
			plusCount++; minusCount = 0;
		}
		if (pstObj->cal.Ihf_set < 0)
		{
			plusCount = 0; minusCount++;
		}
		if (plusCount > 3)
		{
			sign = 0;
			
			pstObj->pstIO->w_stDgOutValue.xB1 = PCI1750_OUT_HI;
			pstObj->pstIO->w_stDgOutValue.xB0 = PCI1750_OUT_LO;
			pstObj->ang[0] = PsProcCurrentPID(pstObj);
			pstObj->ang[1] = 135;
			

		}                            //此时进行电源0解锁，电源1封锁操作
		else if (minusCount > 3)
		{
			sign = 1;
			pstObj->pstIO->w_stDgOutValue.xB0 = PCI1750_OUT_HI;
			pstObj->pstIO->w_stDgOutValue.xB1 = PCI1750_OUT_LO;
			pstObj->ang[0] = 135;
			pstObj->ang[1] = PsProcCurrentPID(pstObj);
			

		}
		else 
		{ 
			pstObj->ang[0]= 135; 
			pstObj->ang[1] = 135;

		}                      // 


	}

}

static double PsProcCurrentPID(ps_proc_t *pstObj)
{
	double kp, ki, kd;
	double  temp = 0;
	double  angle = 0;

	kp = pstObj->parameter.Ihf_kp;
	ki = pstObj->parameter.Ihf_ki;
	kd = pstObj->parameter.Ihf_kd;

	pstObj->cal.Ihf_err_p = pstObj->cal.Ihf_set - pstObj->cal.Ihf_real;
	pstObj->cal.Ihf_err_i += (pstObj->cal.Ihf_set - pstObj->cal.Ihf_real) * 0.001 / pstObj->dTimeFactor;
	pstObj->cal.Ihf_err_d = 0;

	temp = kp * pstObj->cal.Ihf_err_p + ki * pstObj->cal.Ihf_err_i;

	if (fabsf(pstObj->cal.Uhf_ac[0]) > 0)
		temp = temp / (2 * 1.35 * pstObj->cal.Uhf_ac[0]);
	else
		temp = 0;

	if (temp > 0.707)
		temp = 0.707;
	if (temp < -0.707)
		temp = -0.707;
	if (sign=0)
	{
		angle = (180 / 3.1415926) * acosf(temp);
	}
	else
	{
		angle=180- (180 / 3.1415926) * acosf(temp);
	}
	

	return (angle);

}
void Test()
{
	printf("test OK!\n");
}