/*******************************************************************************
 * Motor_Ctrl.c
 * Copy right 2023, All rights reserved.
 * Created on: 2024/12/31
 * Author:
 ******************************************************************************/

/*******************************************************************************
 *------------------------------Include Section---------------------------------
 ******************************************************************************/
#include "App.h"

/*******************************************************************************
 *-------------------------------Declaration------------------------------------
 ******************************************************************************/
extern void SVPWM_Cale(SVPWM_PARA_T *pV);

/*******************************************************************************
 *-----------------------Global variable definition-----------------------------
 ******************************************************************************/
CLARKE g_CLARKEPara = CLARKE_DEFAULTS;
PARK g_PARKPara = PARK_DEFAULTS;
IPARK g_IPARKPara = IPARK_DEFAULTS;
Ang_SinCos g_Park_SinCos = Ang_SinCos_DEFAULTS;
Ang_SinCos g_IPark_SinCos = Ang_SinCos_DEFAULTS;

/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
// 坐标dq转换
static void UVW_Axis_dq(void)
{
    g_CLARKEPara.Us = g_VoltCurrPara.U_Curr;
    g_CLARKEPara.Vs = g_VoltCurrPara.V_Curr;
    g_CLARKEPara.Ws = g_VoltCurrPara.W_Curr;

    CLARKE_Cale((p_CLARKE)&g_CLARKEPara); //  Clarke变换函数 等峰值变换

    g_PARKPara.Angle = g_AnglePara.E_RawAngle;
    g_Park_SinCos.table_Angle = g_PARKPara.Angle;

    SinCos_Table((p_Ang_SinCos)&g_Park_SinCos); // 根据位置读取正余弦值

    g_PARKPara.Sine = g_Park_SinCos.table_Sin;
    g_PARKPara.Cosine = g_Park_SinCos.table_Cos; // 相关参数带入进行PARK变化得到dq轴电流
    g_PARKPara.Alpha = g_CLARKEPara.Alpha;
    g_PARKPara.Beta = g_CLARKEPara.Beta;
    PARK_Cale((p_PARK)&g_PARKPara);
}

uint8_t g_Flag = 0;
void FOC_Svpwm_dq(void) // dq轴电压输入，反Park变换后带入SVPWM
{
    if(g_Flag == 0)
    {
        g_IPARKPara.Ds = 0.4;
        g_IPARKPara.Qs = 0;
        // g_IPARKPara.Ds = g_Id_PiPara.Out; // 0;
        // g_IPARKPara.Qs = g_Iq_PiPara.Out; // 3;
        // g_IPARKPara.Angle = g_AnglePara.E_RawAngle;
        g_IPARKPara.Angle = 0;
    }
    else if(g_Flag == 1)
    {
        g_IPARKPara.Ds = 0.4;
        g_IPARKPara.Qs = 0;
        // g_IPARKPara.Ds = g_Id_PiPara.Out; // 0;
        // g_IPARKPara.Qs = g_Iq_PiPara.Out; // 3;
        // g_IPARKPara.Angle = g_AnglePara.E_RawAngle;
        g_IPARKPara.Angle = g_IPARKPara.Angle + 1;
        if(g_IPARKPara.Angle >= 4096)
        {
            g_IPARKPara.Angle = 0;
        }
    }

    g_IPark_SinCos.table_Angle = g_IPARKPara.Angle;
    SinCos_Table((p_Ang_SinCos)&g_IPark_SinCos); // 反PARK变化的正余弦查表

    g_IPARKPara.Sine = g_IPark_SinCos.table_Sin;
    g_IPARKPara.Cosine = g_IPark_SinCos.table_Cos;
    IPARK_Cale((p_IPARK)&g_IPARKPara); // 反Park变换

    g_SvPWMPara.Ualpha = g_IPARKPara.Alpha;
    g_SvPWMPara.Ubeta = g_IPARKPara.Beta;
    SVPWM_Cale((SVPWM_PARA_T *)&g_SvPWMPara); // 将Alpha和Beta电压带入计算SVPWM的占空比
}

// ADC call back
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        g_ADCSampPara.U_RawCurr = HAL_ADC_GetValue(&hadc1);
        g_ADCSampPara.U_SampleCmp = TRUE;
    }
    else if (hadc->Instance == ADC2)
    {
        g_ADCSampPara.V_RawCurr = HAL_ADC_GetValue(&hadc2);
        g_ADCSampPara.V_SampleCmp = TRUE;
    }

    if (g_ADCSampPara.U_SampleCmp && g_ADCSampPara.V_SampleCmp)
    {
        g_ADCSampPara.U_SampleCmp = FALSE;
        g_ADCSampPara.V_SampleCmp = FALSE;
        g_ADCSampPara.U_Curr = g_ADCSampPara.U_RawCurr;
        g_ADCSampPara.V_Curr = g_ADCSampPara.V_RawCurr;
        ADC_UVW_deal();
        // Get_MotorAngle();
        UVW_Axis_dq(); // dq变换

        // g_Id_PiPara.Ref = 0.5;
        // g_Iq_PiPara.Ref = 0;

        // 现在有电角度、iq、id
        g_Id_PiPara.Fbk = g_PARKPara.Ds;

        g_Iq_PiPara.Fbk = g_PARKPara.Qs;
        PI_Controller(&g_Id_PiPara);
        PI_Controller(&g_Iq_PiPara);

        FOC_Svpwm_dq(); // dq轴电压输入，反Park变换后带入SVPWM计算占空比
        Set_PWMValue(g_SvPWMPara.SVPT);
    }
}

/*******************************************************************************
 *-------------------------------End of File------------------------------------
 ******************************************************************************/