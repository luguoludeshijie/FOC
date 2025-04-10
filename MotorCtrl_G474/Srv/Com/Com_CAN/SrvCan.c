/*******************************************************************************
 *
 * 文件名称: drv_Can.c
 * 版 本 号: V1.0
 * 作    者: linbo.liu
 * 生成日期: 2024年10月16日
 * 功能描述: 驱动层CAN配置
 *
*******************************************************************************/
/*******************************************************************************
 *-----------------------------------------头文件-------------------------------
 ******************************************************************************/
#include "Srv.h"

/*******************************************************************************
 *-----------------------------------------宏定义--------------------------------
 ******************************************************************************/


/*******************************************************************************
 *-----------------------------------------数据类型定义--------------------------
 ******************************************************************************/


/*******************************************************************************
 *-----------------------------------------函数声明------------------------------
 ******************************************************************************/
static void Drv_CanRxMessage(void);

/*******************************************************************************
 *-----------------------------------------常量定义------------------------------
 ******************************************************************************/


/*******************************************************************************
 *-----------------------——----------------全局变量定义--------------------------
 ******************************************************************************/
CAN_RX_FIFO g_xCan1RxFifo;
CAN_RX_FIFO g_xCan1XCPFifo;

/*******************************************************************************
 *---------------------------—----—--------函数实现------------------------------
 ******************************************************************************/
/*******************************************************************************
 * 函 数 名: Drv_GetEmptyRxBuff
 * 功能描述: 从FIFO中返回可用空间
 * 输入参数: CAN_RX_FIFO: FIFO实例
 * 输出参数: CanRxMessage: 可用的空间
 *******************************************************************************/
CanRxMessage *Drv_GetEmptyRxBuff(CAN_RX_FIFO *pFifo)
{
    unsigned char ucLast;
    if (pFifo->ucNum >= CAN_RX_FIFO_DEPTH)
    {
        return NULL;
    }
    ucLast = pFifo->ucLast + 1;
    if (ucLast >= CAN_RX_FIFO_DEPTH)
    {
        ucLast = 0;
    }
    return &pFifo->CAN_RX_FRAME[ucLast];
}

/*******************************************************************************
 * 函 数 名: Drv_AddRxBuff
 * 功能描述: 向FIFO中添加一条数据
 * 输入参数: CAN_RX_FIFO: FIFO实例
 * 输出参数: void
 *******************************************************************************/
void Drv_AddRxBuff(CAN_RX_FIFO *pFifo)
{
    if (pFifo->ucNum >= CAN_RX_FIFO_DEPTH)
    {
        return;
    }
    pFifo->ucLast++;
    if (pFifo->ucLast >= CAN_RX_FIFO_DEPTH)
    {
        pFifo->ucLast = 0;
    }
    pFifo->ucNum++;
}

/*******************************************************************************
 * 函 数 名: Drv_CanRxMessage
 * 功能描述: 从物理FIFO中读取一条数据到软件FIFO中
 * 输入参数: void
 * 输出参数: void
 *******************************************************************************/
static void Drv_CanRxMessage(void)
{
    CanRxMessage *pRxMsg;
    unsigned long filllevel;
#if (MCU_DEVICE_VARIANT == STM32G474)
    filllevel = \
        HAL_FDCAN_GetRxFifoFillLevel(&hfdcan1, FDCAN_RX_FIFO0); // 获取硬件fifo中未读取的数据量
    while (filllevel)
    {
        pRxMsg = Drv_GetEmptyRxBuff(&g_xCan1RxFifo);
        if (pRxMsg != NULL)
        {
            HAL_FDCAN_GetRxMessage(&hfdcan1, FDCAN_RX_FIFO0,\
                 &pRxMsg->CanRxHeader, pRxMsg->Data);
            Drv_AddRxBuff(&g_xCan1RxFifo);
        }
        else
        {
            // 硬件FIFO处于阻塞状态
        }
        filllevel--;
    }
#endif
}

#if (MCU_DEVICE_VARIANT == STM32G474)
/*******************************************************************************
 * 函 数 名: HAL_FDCAN_RxFifo0Callback
 * 功能描述: Rx中断服务回调函数的重写，仅适用于STM32H743
 * 输入参数: hfdcan：句柄，RxFifo0ITs：寄存器中断标志位
 * 输出参数: void
 *******************************************************************************/
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
    if ((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != RESET) // FIFO1新数据中断
    {
        Drv_CanRxMessage();
        HAL_FDCAN_ActivateNotification(hfdcan,\
            FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0); // 切换中断线
    }
}
#endif

/*******************************************************************************
 * 函 数 名: Drv_GetFirstRxBuff
 * 功能描述: 返回软件FIFO中的第一条数据
 * 输入参数: pFifo：FIFO实例
 * 输出参数: CanRxMessage
 *******************************************************************************/
CanRxMessage *Drv_GetFirstRxBuff(CAN_RX_FIFO *pFifo)
{
    unsigned char ucFirst;
    if (pFifo->ucNum == 0)
    {
        return NULL;
    }
    ucFirst = pFifo->ucLast + 1 + CAN_RX_FIFO_DEPTH - pFifo->ucNum;

    if (ucFirst >= CAN_RX_FIFO_DEPTH)
    {
        ucFirst -= CAN_RX_FIFO_DEPTH;
    }
    return &pFifo->CAN_RX_FRAME[ucFirst];
}

/*******************************************************************************
 * 函 数 名: Drv_DecRxBuff
 * 功能描述: 从FIFO中删除一条消息
 * 输入参数: pFifo：FIFO实例
 * 输出参数: void
 *******************************************************************************/
void Drv_DecRxBuff(CAN_RX_FIFO *pFifo)
{
    if (pFifo->ucNum == 0)
    {
        return;
    }
    pFifo->ucNum--;
}

/*******************************************************************************
 * 函 数 名: Drv_CanTxMessage
 * 功能描述: 向物理FIFO中添加一条数据
 * 输入参数: xTxCmdData：需要发送的数据
 * 输出参数: HAL_StatusTypeDef: HAL_OK表示成功，HAL_ERROR表示失败
 *******************************************************************************/
Drv_StatusTypeDef Drv_CanTxMessage(FDCAN_HandleTypeDef *hfdcan, \
    CanTxMessage *xTxCmdData)
{
#if (MCU_DEVICE_VARIANT == STM32G474)
    if (HAL_OK != HAL_FDCAN_AddMessageToTxFifoQ(hfdcan, \
        &xTxCmdData->CanTxHeader, xTxCmdData->Data))
    {
        return HAL_ERROR;
    }

    return HAL_OK;
#endif
}

/*******************************************************************************
 * 函 数 名: Drv_CanConfigFilter
 * 功能描述: 配置CAN过滤器
 * 输入参数: sFilterConfig：过滤器配置
 * 输出参数: Drv_StatusTypeDef: HAL_OK表示成功，HAL_ERROR表示失败
 *******************************************************************************/
Drv_StatusTypeDef Drv_CanConfigFilter(FDCAN_HandleTypeDef *hfdcan, \
    CAN_FilterInitType *sFilterConfig)
{
#if (MCU_DEVICE_VARIANT == STM32G474)
    if (HAL_FDCAN_ConfigFilter(hfdcan, sFilterConfig) != HAL_OK)
    {
        return HAL_ERROR;
    }

    HAL_FDCAN_ConfigGlobalFilter(hfdcan, FDCAN_REJECT, \
        FDCAN_REJECT, ENABLE, ENABLE);
    /* Return function status */
    return HAL_OK;
#endif
}

/*******************************************************************************
 * 函 数 名: Drv_CanInit
 * 功能描述: CAN驱动配置初始化
 * 输入参数: void
 * 输出参数: void
 *******************************************************************************/
void Drv_CanInit(CAN_Type xCanType)
{
#if (MCU_DEVICE_VARIANT == STM32G474)
    if (xCanType == Drv_CAN1)
    {
        MX_FDCAN1_Init();
    }
    else if (xCanType == Drv_CAN2)
    {
    }
#endif
}
/*******************************************************************************
 *---------------------------—----—---结束--------------------------------------
 ******************************************************************************/