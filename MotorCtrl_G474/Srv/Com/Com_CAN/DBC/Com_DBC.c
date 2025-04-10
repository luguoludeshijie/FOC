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
static void Srv_CanTxButtonStateCondition(void);
static void Srv_CanTxMotorStateCondition(void);
static void Srv_CanFilterSet(void);
static void Srv_CanCycleTimerFunction(void);
static void Srv_CanAllSendCondition(void);
static void Srv_CanIDInternalWait(void);
static void Srv_CanReceiveDeal(void);
static void Srv_CanDecideSendID(void);
static void Srv_CanSendDeal(void);

/*******************************************************************************
 *-----------------------------------常量定义------------------------------------
 ******************************************************************************/
Srv_CanTxFrame_T g_xCanTxFrame[CAN_MAX_FRAME] =        // CAN数据帧数组
{
    {
        .ucFrameIndex = CAN_BUTTON_FRAME,
        .ucSendTime = 20,
        .ucSendCnt = 0,
        .ucSendWait = 0,
        .ucSendFlag = 0,
        .Srv_CanTxFrameCallback = NULL,
    },
    {
        .ucFrameIndex = CAN_MOTOR_FRAME,
        .ucSendTime = 10,
        .ucSendCnt = 0,
        .ucSendWait = 0,
        .ucSendFlag = 0,
        .Srv_CanTxFrameCallback = NULL,
    }
};

uint16_t g_usCanIdPriority[CAN_MAX_FRAME] =          // CAN ID 优先级数组 
{
    CAN_MOTOR_FRAME,   
    CAN_BUTTON_FRAME,
};

/*******************************************************************************
 *-----------------------——--------全局变量定义----------------------------------
 ******************************************************************************/
uint16_t g_CurCanSendID;
CanTxMessage g_xCanTxData;

/*******************************************************************************
 *---------------------------—----—--函数实现------------------------------------
 ******************************************************************************/
/*******************************************************************************
* 函 数 名: Srv_CanTxButtonStateCondition
* 功能描述: 发送按键按下的条件判断
* 输入参数: void
* 输出参数: void
*******************************************************************************/
static void Srv_CanTxButtonStateCondition(void)
{
    if(g_xCanTxFrame[CAN_BUTTON_FRAME].ucSendCnt >= \
        g_xCanTxFrame[CAN_BUTTON_FRAME].ucSendTime)
    {
        g_xCanTxFrame[CAN_BUTTON_FRAME].ucSendCnt = 0;
        g_xCanTxFrame[CAN_BUTTON_FRAME].ucSendFlag = 1;

    }
}

/*******************************************************************************
* 函 数 名: Srv_CanTxMotorStateCondition
* 功能描述: 电机状态的条件判断
* 输入参数: void
* 输出参数: void
*******************************************************************************/
static void Srv_CanTxMotorStateCondition(void)
{
    if(g_xCanTxFrame[CAN_MOTOR_FRAME].ucSendCnt >= \
        g_xCanTxFrame[CAN_MOTOR_FRAME].ucSendTime)
    {   
        g_xCanTxFrame[CAN_MOTOR_FRAME].ucSendCnt = 0;
        g_xCanTxFrame[CAN_MOTOR_FRAME].ucSendFlag = 1;
    }
}

/*******************************************************************************
* 函 数 名: xxx
* 功能描述: xxx
* 输入参数: void
* 输出参数: void
*******************************************************************************/
void DBC_InfoFill(Srv_CanFrame_E arg_CanFrame,Srv_CanTxFrameCallbackPtr arg_Cb)
{
    g_xCanTxFrame[arg_CanFrame].Srv_CanTxFrameCallback = arg_Cb;
}

/*******************************************************************************
* 函 数 名: Srv_CanFilterSet
* 功能描述: CAN过滤器设置
* 输入参数: void
* 输出参数: void
*******************************************************************************/
static void Srv_CanFilterSet(void)
{
    CAN_FilterInitType xFilterConfig;
    xFilterConfig.IdType=FDCAN_STANDARD_ID;                //标准ID
    xFilterConfig.FilterIndex=0;                           //滤波器索引                   
    xFilterConfig.FilterType=FDCAN_FILTER_MASK;            //滤波器类型
    xFilterConfig.FilterConfig=FDCAN_FILTER_TO_RXFIFO0;    //过滤器0关联到FIFO0  
    xFilterConfig.FilterID1=MOTOR_ID_PI_CTRL_RXCANID;          //32位ID1
    xFilterConfig.FilterID2=0x780;                         //32位掩码
    Drv_CAN1_CONFIGFILTER(&xFilterConfig);
}

/*******************************************************************************
* 函 数 名: Srv_CanCycleTimerFunction
* 功能描述: CAN任务定时器
* 输入参数: void
* 输出参数: void
*******************************************************************************/
static void Srv_CanCycleTimerFunction(void)
{
    for(uint8_t i = 0; i < CAN_MAX_FRAME; i++)
    {
        g_xCanTxFrame[i].ucSendCnt++;
    }
}

/*******************************************************************************
* 函 数 名: Srv_CanAllSendCondition
* 功能描述: 所有要发送的CAN数据帧条件判断
* 输入参数: void
* 输出参数: void
*******************************************************************************/
static void Srv_CanAllSendCondition(void)
{
    Srv_CanTxButtonStateCondition();
    Srv_CanTxMotorStateCondition();
}

/*******************************************************************************
* 函 数 名: Srv_CanIDInteriorTimer
* 功能描述: 实现CAN ID内部等待,即满足条件后等待发送的计时
* 输入参数: void
* 输出参数: void
*******************************************************************************/
static void Srv_CanIDInternalWait(void)
{
    for(uint8_t i = 0; i < CAN_MAX_FRAME; i++)
    {
        if(g_xCanTxFrame[i].ucSendFlag)
        {
            g_xCanTxFrame[i].ucSendWait++;
        }
    }
}

/*******************************************************************************
* 函 数 名: Srv_CanReceiveDeal
* 功能描述: Can数据接收处理
* 输入参数: void
* 输出参数: void
*******************************************************************************/
static void Srv_CanReceiveDeal(void)
{
    CanRxMessage *pxMsg;
    while(1)
    {
        pxMsg = Drv_CAN1_READ();
        if(pxMsg == NULL)
        {
            return;
        }

        switch(pxMsg->CanRxHeader.Identifier)
        {
            
            case MOTOR_ID_PI_CTRL_RXCANID:
            {
                struct ctrl_ems_0x200_t rtb_dsd;
                ctrl_ems_0x200_unpack(  &rtb_dsd,
                                        pxMsg->Data,
                                        pxMsg->CanRxHeader.DataLength);
                //g_Id_PiPara.Kp = rtb_dsd.motor_tor_id_kp;
                //g_Id_PiPara.Ki = rtb_dsd.motor_tor_id_ki;
            }
            break;
            case MOTOR_IQ_PI_CTRL_RXCANID:
            {
                struct ctrl_ems_0x201_t rtb_dsd;
                ctrl_ems_0x201_unpack(  &rtb_dsd,
                                        pxMsg->Data,
                                        pxMsg->CanRxHeader.DataLength);
                //g_Iq_PiPara.Kp = rtb_dsd.motor_tor_iq_kp;
                //g_Iq_PiPara.Ki = rtb_dsd.motor_tor_iq_ki;
            }
                
            break;
            default:
            break;
        }
        Drv_CAN1_FIFO_RELEASE();
    }
}

/*******************************************************************************
* 函 数 名: Srv_CanDecideSendID
* 功能描述: 发送ID仲裁函数，遇到同时到达的CAN ID时，进行ID仲裁
* 输入参数: void
* 输出参数: void
*******************************************************************************/
static void Srv_CanDecideSendID(void)
{
    uint8_t ucIndex = 0;
    if(g_CurCanSendID != CAN_MAX_FRAME)
    {
        return;
    }
    for(uint8_t i = 0; i < CAN_MAX_FRAME; i++)
    {
        if(g_xCanTxFrame[ucIndex].ucSendWait < g_xCanTxFrame[i].ucSendWait)
        {
            ucIndex = i;    // 找到内部第一个等待发送最大的帧索引
        }
    }
    if((ucIndex == 0) && (g_xCanTxFrame[ucIndex].ucSendFlag == 0))
    {
        return;   //不满足发送条件
    }

    for(uint8_t i = (ucIndex + 1); i < CAN_MAX_FRAME; i++)
    {
        if(g_xCanTxFrame[i].ucSendWait == g_xCanTxFrame[ucIndex].ucSendWait)
        {
            for(uint8_t j = 0; j < CAN_MAX_FRAME; j++)
            {
                if(g_xCanTxFrame[i].ucFrameIndex == g_usCanIdPriority[j])
                {
                    ucIndex = i;
                    break;
                }

                if(g_xCanTxFrame[ucIndex].ucFrameIndex == g_usCanIdPriority[j])
                {
                    break;
                }
            }
        }
    }
    g_CurCanSendID = ucIndex;  // 记录下当前需要发送的CAN ID
}

/*******************************************************************************
* 函 数 名: Srv_CanSendDeal
* 功能描述: CAN数据发送处理
* 输入参数: void
* 输出参数: void
*******************************************************************************/
static void Srv_CanSendDeal(void)
{
    if(g_CurCanSendID == CAN_MAX_FRAME)
    {
        return;   
    }
    if(g_xCanTxFrame[g_CurCanSendID].Srv_CanTxFrameCallback != NULL)
    {
        g_xCanTxFrame[g_CurCanSendID].Srv_CanTxFrameCallback();
    }

    if(HAL_OK == Drv_CAN1_SEND(&g_xCanTxData))
    {
        g_xCanTxFrame[g_CurCanSendID].ucSendWait = 0;
        g_xCanTxFrame[g_CurCanSendID].ucSendFlag = 0;
        
        g_CurCanSendID = CAN_MAX_FRAME;
    }
    else
    {
        //添加失败措施处理
    }
}

/*******************************************************************************
* 函 数 名: Srv_CanInit
* 功能描述: Srv层CAN初始化
* 输入参数: void
* 输出参数: void
*******************************************************************************/
void Srv_CanInit(void)
{
    Drv_CAN1_INIT();
    Srv_CanFilterSet();
}

/*******************************************************************************
* 函 数 名: Srv_CanMain
* 功能描述: CAN任务主函数
* 输入参数: void
* 输出参数: void
*******************************************************************************/
void Srv_CanMain(void)
{
    Srv_CanCycleTimerFunction(); 
    Srv_CanAllSendCondition();  
    Srv_CanIDInternalWait();

    Srv_CanReceiveDeal();
    Srv_CanDecideSendID();
    Srv_CanSendDeal();
}

/*******************************************************************************
 *---------------------------—----—---结束--------------------------------------
 ******************************************************************************/