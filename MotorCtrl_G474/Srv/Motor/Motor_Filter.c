/*******************************************************************************
*
* 文件名称: Motor_Filter.c
* 版 本 号: V1.0 
* 作    者: linbo.liu
* 生成日期: 2024年x月x日
* 功能描述: 
*
*******************************************************************************/
/*******************************************************************************
 *----------------------------------头文件--------------------------------------
 ******************************************************************************/
#include "App.h"
 

/*******************************************************************************
 *----------------------------------宏定义---------------------------------------
 ******************************************************************************/


/*******************************************************************************
 *--------------------------------数据类型定义-----------------------------------
 ******************************************************************************/


/*******************************************************************************
 *----------------------------------函数声明-------------------------------------
 ******************************************************************************/


/*******************************************************************************
 *-----------------------------------常量定义------------------------------------
 ******************************************************************************/


/*******************************************************************************
 *-----------------------——--------全局变量定义----------------------------------
 ******************************************************************************/


/*******************************************************************************
 *---------------------------—----—--函数实现------------------------------------
 ******************************************************************************/

//  void PLL_Init(PLL_t *pll, float Ts)
//  {
//      pll->Ts = Ts;
 
//      Move_Filter_Init(&pll->move_filter, 200);
 
//  }
 
//  void PLL_Clear(PLL_t *pll)
//  {
//      pll->Angle_Err=0;
//      pll->P=0;
//      pll->I=0;
//      pll->Omega = 0;
//      pll->Omega_F=0;
 
//      Move_Filter_Clear(&pll->move_filter);
//  }
 
 
//  /*高频注入锁相环*/
//  void PLL_Cale_HFI(PLL_t *pll, float alpha, float beta)
//  {
//      float _cos, _sin;
 
//      _sin = arm_sin_f32(pll->Angle);
//      _cos = arm_cos_f32(pll->Angle);
 
//      pll->Angle_Err = -alpha * _sin + beta * _cos;
//      pll->P = pll->Kp * pll->Angle_Err;
//      pll->I += pll->Ki * pll->Angle_Err;
 
//      pll->Omega = pll->P + pll->I;
 
//      pll->Omega_F = pll->Omega_F*0.9f + pll->Omega *0.1f;
 
 
//      pll->Angle += pll->Omega_F * pll->Ts;
//      pll->Angle = Limit_Angle(pll->Angle);
//      Move_Filter_fill(&pll->move_filter, pll->Omega_F);
//  }
 
//  /*滑模锁相环*/
//  void PLL_Cale_SMO(PLL_t *pll, float alpha, float beta)
//  {
//      float _cos, _sin;
 
//      _sin = arm_sin_f32(pll->Angle);
//      _cos = arm_cos_f32(pll->Angle);
 
//      pll->Angle_Err = -alpha * _cos - beta * _sin;
//      pll->P = pll->Kp * pll->Angle_Err;
//      pll->I += pll->Ki * pll->Angle_Err * pll->Ts;
 
//      pll->Omega = pll->P + pll->I;
 
//      pll->Omega_F = pll->Omega_F*0.9f + pll->Omega *0.1f;
 
//      pll->Angle += pll->Omega_F * pll->Ts;
//      pll->Angle = Limit_Angle(pll->Angle);
//      Move_Filter_fill(&pll->move_filter, pll->Omega_F);
//  }
 
//  /*霍尔锁相环*/
//  void PLL_Calc_HALL(PLL_t *pll, float input_theta, float input_speed)
//  {
//      pll->Angle_Err = Limit_Angle(input_theta - pll->Angle);
 
//      if(pll->Angle_Err>=PI){
//          pll->Angle_Err -= _2PI;
//      }
//      if(pll->Angle_Err<=-PI){
//          pll->Angle_Err += _2PI;
//      }
 
//      pll->Omega += pll->Ki * pll->Angle_Err;
 
//      pll->Angle +=  (pll->Omega + pll->Angle_Err*pll->Kp)*pll->Ts;
 
//      pll->Angle = Limit_Angle(pll->Angle);
 
//      pll->Ki = 0.236f*input_speed*input_speed;
//      arm_sqrt_f32(pll->Ki, &pll->Kp);
 
//      pll->Kp *= 1.414f;
//      pll->Ki *= pll->Ts;
//  }
 
/*滑动滤波初始化*/
void Move_Filter_Init(MOVE_FILTER_T *filter, uint32_t buf_size)
{
    uint16_t i;
    if(buf_size>MOVE_FILTER_BUF_MAXSIZE)buf_size=MOVE_FILTER_BUF_MAXSIZE;

    filter->buf_size = buf_size;

    for(i=0; i<buf_size;i++){
        filter->buf[i] = 0;
    }
}

void Move_Filter_Clear(MOVE_FILTER_T *filter)
{
    uint32_t i;

    filter->buf_fill = 0;
    filter->buf_sum = 0;
    filter->buf_Index = 0;
    
    for(i=0;i<MOVE_FILTER_BUF_MAXSIZE;i++){
        filter->buf[i] = 0;
    }

    filter->val = 0;
}

void Move_Filter_fill(MOVE_FILTER_T *filter, int64_t val)
{
    filter->buf_sum -= filter->buf[filter->buf_Index];
    filter->buf[filter->buf_Index] = val;
    filter->buf_sum += filter->buf[filter->buf_Index];

    filter->buf_Index++;
    if(filter->buf_Index>=filter->buf_size){
        filter->buf_Index = 0;
    }

    if(filter->buf_fill<filter->buf_size){
        filter->buf_fill++;
    }
    
}

float Move_Filter_calculate(MOVE_FILTER_T *filter)
{   
    float buf_sum = (float)filter->buf_sum;

    if(filter->buf_fill==0)return 0;

    if(filter->buf_fill < filter->buf_size){
        buf_sum = buf_sum/(float)filter->buf_fill;
    }else{
        buf_sum = buf_sum/(float)filter->buf_size;
    }

    filter->val = buf_sum;
    return buf_sum;
} 

/*******************************************************************************
 *---------------------------—----—---结束--------------------------------------
 ******************************************************************************/