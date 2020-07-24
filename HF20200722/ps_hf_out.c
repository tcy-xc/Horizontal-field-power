#include"ps_hf_out.h"
#include"ps_hf_proc.h"
#include"ps_hf_io.h"
#include "pci1750.h"
#include<math.h>
#include <stdio.h>

static int plusCount;   //��������
static int minusCount;     //��������
static int sign;   //������־λ
static int xsign=0;

static double PsProcCurrentPID(ps_proc_t *pstObj);
static void PsProcCurrentI1(ps_proc_t *pstObj);
static void PsProcCurrentI2(ps_proc_t *pstObj);
static void SetSign(ps_proc_t *pstObj);
static void DgOut0(ps_proc_t* pstObj);
static void DgOut1(ps_proc_t* pstObj);
static void DgOut2(ps_proc_t* pstObj);

void PIDoutAngle(ps_proc_t* pstObj)
{
	double plusmin = 4;
	double minusmax = -1;

	if (pstObj->cal.Ihf_real > plusmin&& pstObj->cal.Ihf_set > 0)
	{
		sign = 1;
		xsign = 1;
		pstObj->ang[0] = PsProcCurrentPID(pstObj);
		pstObj->ang[1] = 135;
		plusCount = 0;
		minusCount = 0;                                   //��Դ1����
	}
	if (pstObj->cal.Ihf_real < minusmax && pstObj->cal.Ihf_set < 0)
	{
		sign = 2;
		xsign = 2;
		pstObj->ang[0] = 135;
		pstObj->ang[1] = PsProcCurrentPID(pstObj);
		plusCount = 0;
		minusCount = 0;
	}




	if (pstObj->cal.Ihf_real > plusmin&& pstObj->cal.Ihf_set < 0)
	{
		xsign = 0;
		pstObj->ang[0] = 135;
		pstObj->ang[1] = 135;
		
		PsProcCurrentPID(pstObj);
		plusCount = 0;
		if (pstObj->cal.Ihf_real < 10)
		{
			SetSign(pstObj);
			minusCount++;
			if(minusCount==2)
			{
				PsProcCurrentI2(pstObj);
			}
		}
	}           //��Դ1����
	if (pstObj->cal.Ihf_real < minusmax && pstObj->cal.Ihf_set>0)
	{
		xsign = 0;
		pstObj->ang[0] = 135;
		pstObj->ang[1] = 135;

		PsProcCurrentPID(pstObj);
		minusCount = 0;
		if (pstObj->cal.Ihf_real > -10)
		{
			SetSign(pstObj);
			plusCount++;
			if(plusCount==2)
			{
				PsProcCurrentI1(pstObj);
			}
		}
	}             //��ʱ��Դ0Ӧ�÷���


	if (pstObj->cal.Ihf_real >= minusmax && pstObj->cal.Ihf_real <= plusmin) //�м�״̬
	{
		if (pstObj->cal.Ihf_set > 0)
		{
			SetSign(pstObj);
			plusCount++;
			minusCount = 0;
			pstObj->ang[0] = PsProcCurrentPID(pstObj);
			pstObj->ang[1] = 135;
			
			
		}
		if (pstObj->cal.Ihf_set < 0)
		{
			SetSign(pstObj);
			plusCount = 0;
			minusCount++;
			pstObj->ang[0] = 135;
			pstObj->ang[1] = PsProcCurrentPID(pstObj);
			
		
		}
		
		

	}


	if (pstObj->cal.Ihf_set<0.1&&pstObj->cal.Ihf_set>-0.1)
	{
		sign = 0;
		pstObj->ang[0] = 135;
		pstObj->ang[1] = 135;
		plusCount = 0;
		minusCount = 0;
	}
}

//static double Mid_OUT()
//{
//	
//}

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
	if (sign==1)
	{
		angle = (180 / 3.1415926) * acosf(temp);
	}
	else if(sign==2)
	{
		angle=180- (180 / 3.1415926) * acosf(temp);
	}
	else
	{
		angle=135;
	}
	

	return (angle);

}

static void PsProcCurrentI1(ps_proc_t *pstObj)
{
	pstObj->cal.Ihf_err_i=1;
}


static void PsProcCurrentI2(ps_proc_t *pstObj)
{
	pstObj->cal.Ihf_err_i=-1;
}


//void Test()
//{
//	printf("test OK!\n");
//}
void DgOut0(ps_proc_t* pstObj)
{
	sign = 0;
	pstObj->pstIO->w_stDgOutValue.xB1 = PCI1750_OUT_HI;
	pstObj->pstIO->w_stDgOutValue.xB0 = PCI1750_OUT_HI;
	PsIODgRefresh(pstObj->pstIO);
}

void DgOut1(ps_proc_t* pstObj)
{
	sign = 1;
	pstObj->pstIO->w_stDgOutValue.xB0 = PCI1750_OUT_LO;
	pstObj->pstIO->w_stDgOutValue.xB1 = PCI1750_OUT_HI;
	PsIODgRefresh(pstObj->pstIO);
}

void DgOut2(ps_proc_t* pstObj)
{
	sign = 2;
	pstObj->pstIO->w_stDgOutValue.xB0 = PCI1750_OUT_HI;
	pstObj->pstIO->w_stDgOutValue.xB1 = PCI1750_OUT_LO;
	PsIODgRefresh(pstObj->pstIO);
}




void SetSign(ps_proc_t *pstObj)
{
	if (pstObj->cal.Ihf_set>0)
	{


		if (xsign==1)
		{
			
		}
		else
		{
			if (plusCount > 2)
			{
				DgOut1(pstObj);

			}
			else
			{
				DgOut0(pstObj);
			}
		}
			
		
	}
	else if (pstObj->cal.Ihf_set < 0)
	{
		if (xsign == 2)
		{

		}
		else
		{
			if (minusCount > 2)
			{
				DgOut2(pstObj);

			}
			else
			{
				DgOut0(pstObj);
			}
		}
			
		
	}
	else
	{

		plusCount = 0;
		minusCount = 0;
	}

}

