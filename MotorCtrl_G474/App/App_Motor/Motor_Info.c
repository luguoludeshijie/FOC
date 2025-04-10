/*******************************************************************************
 * Info.c
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

/*******************************************************************************
 *-----------------------Global variable definition-----------------------------
 ******************************************************************************/
ADC_PARA_T g_ADCSampPara;
VOLT_CURR_T g_VoltCurrPara;
SVPWM_PARA_T g_SvPWMPara;
ANGLE_Para_T g_AnglePara;
SPEED_Para_T g_SpeedPara;
/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
// 角度参数初始化
void Angle_ParaInit(void)
{
    g_AnglePara.RawAngle = 0;
    g_AnglePara.E_Angle = 0.0;
    g_AnglePara.E_RawAngle = 0;
    g_AnglePara.M_CurAngle = 0.0;
    g_AnglePara.M_PreAngle = 0.0;
    g_AnglePara.Ro_Angle = 0.0;
}

// 获取电机原始角度
void Get_MotorAngle(void)
{
    RAW_ANGLE_U rtb_RawAngle;
#if HARDWARE_IIC
    HAL_I2C_Mem_Read(&hi2c2, AS5600_ADDR, AS5600_ANGLE_H, I2C_MEMADD_SIZE_8BIT,
                     &rtb_RawAngle.xRaw_Angle.Raw_Angle_H, 1, 2);
    HAL_I2C_Mem_Read(&hi2c2, AS5600_ADDR, AS5600_ANGLE_L, I2C_MEMADD_SIZE_8BIT,
                     &rtb_RawAngle.xRaw_Angle.Raw_Angle_L, 1, 2);
#else
    rtb_RawAngle.usRaw_Angle = AS5600_ReadTwoByte(AS5600_ANGLE_H, AS5600_ANGLE_L);
#endif

    g_AnglePara.RawAngle = rtb_RawAngle.usRaw_Angle;
    g_AnglePara.M_CurAngle = (float)((float)rtb_RawAngle.usRaw_Angle / 4096) * 360;

    // uint16_t rtb_Angle0 = (uint16_t)(g_AnglePara.M_CurAngle*5);

    // uint16_t rtb_Angle1 = rtb_Angle0*MOTOR_PolePairs;

    uint32_t rtb_Angle0 = (uint16_t)(g_AnglePara.RawAngle);

    if (g_AnglePara.RawAngle >= 1196)
    {
        rtb_Angle0 -= 1196;
    }
    else
    {
        rtb_Angle0 = rtb_Angle0 + 4096 - 1196;
    }

    uint16_t rtb_Angle1 = rtb_Angle0 * MOTOR_PolePairs;

    // 对1800取余 除5
    //  g_AnglePara.E_Angle = (rtb_Angle1 % 1800)/5;
    g_AnglePara.E_RawAngle = (rtb_Angle1 % 4096);

    // if(g_AnglePara.E_RawAngle >= 3167)
    // {
    //     g_AnglePara.E_RawAngle -= 3167;
    // }
    // else
    // {
    //     g_AnglePara.E_RawAngle = g_AnglePara.E_RawAngle + 4096 - 3167;
    // }

    g_AnglePara.E_Angle = (float)((float)g_AnglePara.E_RawAngle / 4096) * 360;
}

// 角度到速度的计算函数
void Calculate_Speed(float newAngle)
{
    // 更新当前角度
    g_AnglePara.M_CurAngle = newAngle;

    // 计算角度差
    g_AnglePara.Ro_Angle = g_AnglePara.M_CurAngle - g_AnglePara.M_PreAngle;

    // 处理角度环绕问题（0度和360度之间切换）
    if (g_AnglePara.Ro_Angle > 180.00f)
    {
        g_AnglePara.Ro_Angle -= 360.00f;
    }
    else if (g_AnglePara.Ro_Angle < -180.00f)
    {
        g_AnglePara.Ro_Angle += 360.00f;
    }

    // 计算电机速度 (单位：RPM)
    g_SpeedPara.CurSpeed = ((float)g_AnglePara.Ro_Angle / 360.00f) * (1.00f / (float)SPEED_SamplingTime) * 60.00f / MOTOR_PolePairs;

    // 更新上一次的角度
    g_AnglePara.M_PreAngle = g_AnglePara.M_CurAngle;
}

// PWM参数初始化
void PWM_ParaInit(void)
{
    g_SvPWMPara.Ualpha = 0;
    g_SvPWMPara.Ubeta = 0;
    g_SvPWMPara.T[0] = 0;
    g_SvPWMPara.T[1] = 0;
    g_SvPWMPara.T[2] = 0;
    g_SvPWMPara.SVPT[0] = 0;
    g_SvPWMPara.SVPT[1] = 0;
    g_SvPWMPara.SVPT[2] = 0;
    g_SvPWMPara.tmp[0] = 0;
    g_SvPWMPara.tmp[1] = 0;
    g_SvPWMPara.tmp[2] = 0;
    g_SvPWMPara.VecSector = 0;
}

// ADC参数初始化
void ADC_ParaInit(void)
{
    g_ADCSampPara.BUS_RawCurr = 0;
    g_ADCSampPara.U_RawCurr = 0;
    g_ADCSampPara.V_RawCurr = 0;
    g_ADCSampPara.W_RawCurr = 0;
    g_ADCSampPara.BUS_Curr = 0;
    g_ADCSampPara.U_Curr = 0;
    g_ADCSampPara.V_Curr = 0;
    g_ADCSampPara.W_Curr = 0;
    g_ADCSampPara.BUS_Voltage = 0;
    g_ADCSampPara.OffsetBUS_Curr = 0;
    g_ADCSampPara.OffsetU_Curr = 0;
    g_ADCSampPara.OffsetV_Curr = 0;
    g_ADCSampPara.OffsetW_Curr = 0;
    g_ADCSampPara.ADC_InitCmp = FALSE;
    g_ADCSampPara.BUS_SampleCmp = FALSE;
    g_ADCSampPara.U_SampleCmp = FALSE;
    g_ADCSampPara.V_SampleCmp = FALSE;
    g_ADCSampPara.W_SampleCmp = FALSE;

    HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
    HAL_ADC_Start_IT(&hadc1);
    HAL_ADCEx_Calibration_Start(&hadc2, ADC_SINGLE_ENDED);
    HAL_ADC_Start_IT(&hadc2);

    Set_PwmTrgAdcStart();
    Set_PwmStart();
    Set_PWMValue(g_SvPWMPara.SVPT);
    HAL_GPIO_WritePin(Motor_Start_GPIO_Port, Motor_Start_Pin, GPIO_PIN_SET);
    // HAL_Delay(20);
}

// ADC数据处理
void ADC_UVW_deal(void) // 计算电流实际值
{
    float U_Curr = 0, V_Curr = 0, W_Curr = 0;
    // DSP是0--3V 对应12位AD4096
    U_Curr = -(float)(((int16_t)(g_ADCSampPara.U_Curr - g_ADCSampPara.OffsetU_Curr) / 4096.0 * 3.3 - 1.25) / 0.12);
    V_Curr = -(float)(((int16_t)(g_ADCSampPara.V_Curr - g_ADCSampPara.OffsetV_Curr) / 4096.0 * 3.3 - 1.25) / 0.12);
    W_Curr = -(U_Curr + V_Curr); // 相电流Iu Iv  Iw和等于0

    g_VoltCurrPara.U_Curr = U_Curr;
    g_VoltCurrPara.V_Curr = V_Curr;
    g_VoltCurrPara.W_Curr = W_Curr;
}

/*******************************************************************************
 *-------------------------------End of File------------------------------------
 ******************************************************************************/