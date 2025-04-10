/*******************************************************************************
*
* 文件名称: xx.c
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
/*******************************************************************************
* 函 数 名: Srv_CanButtonTxData
* 功能描述: 发送按键按下的数据
* 输入参数: void
* 输出参数: void
*******************************************************************************/
static void App_CanTxButtonStateFill(void)
{
    g_xCanTxData.CanTxHeader.Identifier = BUTTON_STATE_TXCANID;
    g_xCanTxData.CanTxHeader.TxFrameType = FDCAN_DATA_FRAME;
    g_xCanTxData.CanTxHeader.IdType = FDCAN_STANDARD_ID;
    g_xCanTxData.CanTxHeader.FDFormat = FDCAN_CLASSIC_CAN;
    g_xCanTxData.CanTxHeader.BitRateSwitch = FDCAN_BRS_OFF;
    g_xCanTxData.CanTxHeader.MessageMarker = 0;
    g_xCanTxData.CanTxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    g_xCanTxData.CanTxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    g_xCanTxData.CanTxHeader.DataLength = FDCAN_DLC_BYTES_8;

    for (uint8_t i = 0; i < FDCAN_DLC_BYTES_8; i++)
    {
        g_xCanTxData.Data[i] = i;
    }
}

/*******************************************************************************
* 函 数 名: Srv_CanTxMotorState
* 功能描述: 电机状态的填充
* 输入参数: void
* 输出参数: void
*******************************************************************************/
static void App_CanTxMotorStateFill(void)
{
    g_xCanTxData.CanTxHeader.Identifier = MOTOR_STATE_TXCANID;
    g_xCanTxData.CanTxHeader.TxFrameType = FDCAN_DATA_FRAME;
    g_xCanTxData.CanTxHeader.IdType = FDCAN_STANDARD_ID;
    g_xCanTxData.CanTxHeader.FDFormat = FDCAN_CLASSIC_CAN;
    g_xCanTxData.CanTxHeader.BitRateSwitch = FDCAN_BRS_OFF;
    g_xCanTxData.CanTxHeader.MessageMarker = 0;
    g_xCanTxData.CanTxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    g_xCanTxData.CanTxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    g_xCanTxData.CanTxHeader.DataLength = FDCAN_DLC_BYTES_8;
    struct ctrl_ems_0x180_t rtb_0x180Data = 
    {
        g_AnglePara.M_CurAngle,
        g_AnglePara.E_Angle
    };
    ctrl_ems_0x180_pack(g_xCanTxData.Data,&rtb_0x180Data,8);
}

/*******************************************************************************
* 函 数 名: App_DBCInit
* 功能描述: 初始化DBC数据填充
* 输入参数: void
* 输出参数: void
*******************************************************************************/
void App_DBCInit(void)
{
    DBC_InfoFill(CAN_BUTTON_FRAME,App_CanTxButtonStateFill);
    DBC_InfoFill(CAN_MOTOR_FRAME,App_CanTxMotorStateFill);
}
/*******************************************************************************
 *---------------------------—----—---结束--------------------------------------
 ******************************************************************************/