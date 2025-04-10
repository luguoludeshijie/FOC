 /*******************************************************************************
 *
 * 文件名称: Motor_Hall.c
 * 版 本 号: V1.0 
 * 作    者: linbo.liu
 * 生成日期: 2024年x月x日
 * 功能描述:hall信号处理 
 *
 *******************************************************************************/
 /*******************************************************************************
  *----------------------------------头文件--------------------------------------
  ******************************************************************************/
#include "Srv.h"
 
 /*******************************************************************************
  *----------------------------------宏定义---------------------------------------
  ******************************************************************************/
 
 
 /*******************************************************************************
  *--------------------------------数据类型定义-----------------------------------
  ******************************************************************************/
 
 
 /*******************************************************************************
  *----------------------------------函数声明-------------------------------------
  ******************************************************************************/
 static void FOC_HALL_CC_IRQ_Handler(FOC_HALL_T *foc_hall);
 
/*******************************************************************************
 *-----------------------------------常量定义------------------------------------
 ******************************************************************************/

 
/*******************************************************************************
 *-----------------------——--------全局变量定义----------------------------------
 ******************************************************************************/
 FOC_HALL_T g_Hall;

 
/*******************************************************************************
 *---------------------------—----—--函数实现------------------------------------
 ******************************************************************************/
void Motor_HALLInit(void)
{
    Move_Filter_Init(&g_Hall.Secsize_filter, 12);
    Move_Filter_Clear(&g_Hall.Secsize_filter);
}
/*******************************************************************************
 * 函 数 名: BLDCMotor_PhaseCtrl
 * 功能描述: BLDC方波控制
 * 输入参数: HALLPhase:当前hall状态
 * 输出参数: void
*******************************************************************************/
 void BLDCMotor_PhaseCtrl(int32_t HALLPhase )
{   
	switch (HALLPhase)
	{
	case 5: 
	{
		uint16_t Duty[3] = {200,0,0};
		Set_PWMValue(Duty);
	}
	break;

	case 4: 
	{
		uint16_t Duty[3] = {200,200,0};
		Set_PWMValue(Duty);
	}
	break;

	case 6:
	{
		uint16_t Duty[3] = {0,200,0};
		Set_PWMValue(Duty);
	}
	break;

	case 2: 
	{
		uint16_t Duty[3] = {0,200,200};
		Set_PWMValue(Duty);
	}
	break;

	case 3: 
	{
		uint16_t Duty[3] = {0,0,200};
		Set_PWMValue(Duty);
	}
	break;
	case 1: 
	{
		uint16_t Duty[3] = {200,0,200};
		Set_PWMValue(Duty);
	}
	break;
	}
}

/*******************************************************************************
* 函 数 名: HAL_TIM_IC_CaptureCallback
* 功能描述: 定时器中断回调
* 输入参数: htim:定时器
* 输出参数: void
*******************************************************************************/
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if(htim->Instance==TIM5){
        FOC_HALL_CC_IRQ_Handler(&g_Hall);
    }
}

extern IPARK g_IPARKPara;
/*******************************************************************************
* 函 数 名: FOC_HALL_CC_IRQ_Handler
* 功能描述: 霍尔中断处理
* 输入参数: foc_hall:Hall结构体
* 输出参数: void
*******************************************************************************/
static void FOC_HALL_CC_IRQ_Handler(FOC_HALL_T *foc_hall)
{
    uint8_t HallState = HALL_U_GET 
                        | HALL_V_GET << 1
                        | HALL_W_GET << 2;
    uint32_t hHighSpeedCapture = (int64_t)htim5.Instance->CCR1;

    
    foc_hall->Sector = HallState;
    if(foc_hall->Sector_pre != foc_hall->Sector)
    {
        Move_Filter_fill(&g_Hall.Secsize_filter, g_IPARKPara.Angle);
        foc_hall->Sector_size[foc_hall->Sector] =  \
            (uint16_t)Move_Filter_calculate(&g_Hall.Secsize_filter);
    }
    foc_hall->Sector_pre = foc_hall->Sector;
    foc_hall->Sector_Map[HallState] = hHighSpeedCapture;
    // BLDCMotor_PhaseCtrl(HallState);
    // /*必须在切换扇区前计算上一个扇区的补偿系数*/
    // Hall_SectorComp_Caculate(foc_hall, hHighSpeedCapture);

    // switch (HallState)
    // {
    //     case 4:
    //         if(foc_hall->sector_pre==6){
    //             foc_hall->speed.dir = COROTATION;
    //         }else if(foc_hall->sector_pre==5){
    //             foc_hall->speed.dir = REVERSAL;
    //         }
    //         foc_hall->sector = 0;
    //         break;
    //     case 5:
    //         if(foc_hall->sector_pre==4){
    //             foc_hall->speed.dir = COROTATION;
    //         }else if(foc_hall->sector_pre==1){
    //             foc_hall->speed.dir = REVERSAL;
    //         }
    //         foc_hall->sector = 1;
    //         break;
    //     case 1:
    //         if(foc_hall->sector_pre==5){
    //             foc_hall->speed.dir = COROTATION;
    //         }else if(foc_hall->sector_pre==3){
    //             foc_hall->speed.dir = REVERSAL;
    //         }
    //         foc_hall->sector = 2;
    //         break;
    //     case 3:
    //         if(foc_hall->sector_pre==1){
    //             foc_hall->speed.dir = COROTATION;
    //         }else if(foc_hall->sector_pre==2){
    //             foc_hall->speed.dir = REVERSAL;
    //         }
    //         foc_hall->sector = 3;
    //         break;
    //     case 2:
    //         if(foc_hall->sector_pre==3){
    //             foc_hall->speed.dir = COROTATION;
    //         }else if(foc_hall->sector_pre==6){
    //             foc_hall->speed.dir = REVERSAL;
    //         }
    //         foc_hall->sector = 4;
    //         break;
    //     case 6:
    //         if(foc_hall->sector_pre==2){
    //             foc_hall->speed.dir = COROTATION;
    //         }else if(foc_hall->sector_pre==5){
    //             foc_hall->speed.dir = REVERSAL;
    //         }
    //         foc_hall->sector = 5;
    //         break;
    // }
    // foc_hall->sector_pre = HallState;

    // /*角度累计, 堵转补偿 积分补偿清零*/
    // // foc_hall->Lock_ElAngle = 0.0f;
    // foc_hall->Sector_ElAngle_Sum = 0.0f;
    // foc_hall->Sector_CompIntegral = 0.0f;  

    // if(foc_hall->speed.dir==COROTATION){
    //     foc_hall->Sector_ElAngle = foc_hall->sector_pos[foc_hall->sector];
    // }else{
    //     foc_hall->Sector_ElAngle = foc_hall->sector_pos_c[foc_hall->sector];
    // }

    // /*存入滑动滤波器缓冲区*/
    // Move_Filter_fill(&foc_hall->Period_filter, hHighSpeedCapture);
    // /*计算扇区速度*/
    // float AvrCount = Move_Filter_calculate(&foc_hall->Period_filter);               /* 平均每个扇区的计数值 */
    // float t = AvrCount / foc_hall->hall_freq;                                       /* 一个周期的时间 */
    // foc_hall->AvrElSpeed = _PI_3/t;                                                 /* 霍尔计算的电角速度 */
    // foc_hall->AvrElSpeedDpp = foc_hall->AvrElSpeed /foc_hall->foc_freq;             /* 一个foc周期内增加的霍尔扇区内电角度*/

   
    // /*用于调试霍尔位置偏差的*/
    // Hall_Parameter_Calculate(foc_hall, hHighSpeedCapture);
    // Hall_Parameter_Debug(foc_hall);

}
 
/*******************************************************************************
 *---------------------------—----—---结束--------------------------------------
******************************************************************************/
 