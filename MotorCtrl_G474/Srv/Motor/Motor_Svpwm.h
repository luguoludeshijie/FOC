/*******************************************************************************
*
* 文件名称: xx.h
* 版 本 号: V1.0 
* 作    者: linbo.liu
* 生成日期: 2024年x月x日
* 功能描述: 
*
*******************************************************************************/
#ifndef _MOTOR_SVPWM_H_
#define _MOTOR_SVPWM_H_

/*******************************************************************************
 *-----------------------------------头文件-------------------------------------
 ******************************************************************************/
#include "Srv.h"

/*******************************************************************************
 *-----------------------------------宏定义-------------------------------------
 ******************************************************************************/
#define SVPWM_DEFAULTS {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0} // 初始化参数


/*******************************************************************************
 *---------------------------------数据类型定义----------------------------------
 ******************************************************************************/
typedef struct
{
	float Ualpha;		//  二相静止坐标系alpha-轴
	float Ubeta;		//  二相静止坐标系beta-轴
	float Ta;			//  三相矢量占空比Ta
	float Tb;			//  三相矢量占空比Tb
	float Tc;			//  三相矢量占空比Tc
	uint16_t SVPTa;		//  三相矢量占空比Ta
	uint16_t SVPTb;		//  三相矢量占空比Tb
	uint16_t SVPTc;		//  三相矢量占空比Tc
	float tmp1;			//  三相静止坐标系的电压temp1
	float tmp2;			//  三相静止坐标系的电压temp2
	float tmp3;			//  三相静止坐标系的电压temp3
	uint16_t VecSector; // 矢量空间扇区号
} SVPWM, *p_SVPWM;


/*******************************************************************************
 *-----------------------------------常量定义------------------------------------
 ******************************************************************************/


/*******************************************************************************
 *-----------------------——--------全局变量定义----------------------------------
 ******************************************************************************/


/*******************************************************************************
 *-----------------------------------函数接口------------------------------------
 ******************************************************************************/


/*******************************************************************************
 *---------------------------—----—--变量接口------------------------------------
 ******************************************************************************/

#endif 
/*******************************************************************************
 *---------------------------—----—---结束--------------------------------------
 ******************************************************************************/