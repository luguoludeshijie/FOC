/*******************************************************************************
*
* 文件名称: drvCan_H743.h
* 版 本 号: V1.0 
* 作    者: linbo.liu
* 生成日期: 2024年x月x日
* 功能描述: 驱动层CAN配置
*
*******************************************************************************/
#ifndef _SRVCAN_H_
#define _SRVCAN_H_

/*******************************************************************************
 *-----------------------------------------头文件-------------------------------
 ******************************************************************************/
#include "Srv.h"


/*******************************************************************************
 *-----------------------------------------宏定义-------------------------------
 ******************************************************************************/
#define MCU_DEVICE_VARIANT   STM32G474

typedef struct 
{
    FDCAN_TxHeaderTypeDef CanTxHeader;
    uint8_t              Data[8];       //目前只是用标准CAN
}FDCAN_TXMessage_H743;

typedef struct 
{
    FDCAN_RxHeaderTypeDef CanRxHeader;
    uint8_t              Data[8];
}FDCAN_RXMessage_H743;

#if (MCU_DEVICE_VARIANT == STM32G474)
    typedef FDCAN_TXMessage_H743 CanTxMessage;
    typedef FDCAN_RXMessage_H743 CanRxMessage;
    typedef FDCAN_FilterTypeDef  CAN_FilterInitType;
    typedef HAL_StatusTypeDef Drv_StatusTypeDef;

#elif (MCU_DEVICE_VARIANT == STM32F103)

#endif

#define USED_CAN1        1
#define USED_CAN2        0


#define CAN_RX_FIFO_DEPTH       5   //软件FIFO的深度

/*******************************************************************************
 *-----------------------------------------数据类型定义--------------------------
 ******************************************************************************/
typedef enum
{
    Drv_CAN1,
    Drv_CAN2,
}CAN_Type;

typedef struct
{
    uint8_t ucLast;
    uint8_t ucNum;
    CanRxMessage CAN_RX_FRAME[CAN_RX_FIFO_DEPTH];
}CAN_RX_FIFO;


/*******************************************************************************
 *-----------------------------------------常量定义------------------------------
 ******************************************************************************/


/*******************************************************************************
 *-----------------------——----------------全局变量定义--------------------------
 ******************************************************************************/


/*******************************************************************************
 *-----------------------------------------函数接口------------------------------
 ******************************************************************************/
CanRxMessage *Drv_GetEmptyRxBuff(CAN_RX_FIFO *pFifo);
void Drv_AddRxBuff(CAN_RX_FIFO *pFifo);
void Drv_CanInit(CAN_Type xCanType);
Drv_StatusTypeDef Drv_CanConfigFilter(FDCAN_HandleTypeDef *hfdcan,\
    CAN_FilterInitType* sFilterConfig);
Drv_StatusTypeDef Drv_CanTxMessage(FDCAN_HandleTypeDef *hfdcan,\
    CanTxMessage *xTxCmdData);
void Drv_DecRxBuff(CAN_RX_FIFO * pFifo);
CanRxMessage * Drv_GetFirstRxBuff(CAN_RX_FIFO * pFifo);

#if USED_CAN1
#define Drv_CAN1_INIT()                        (Drv_CanInit(Drv_CAN1))
#define Drv_CAN1_CONFIGFILTER(xFilterConfig)   (Drv_CanConfigFilter(&hfdcan1,xFilterConfig))
#define Drv_CAN1_SEND(xTxCmdData)              (Drv_CanTxMessage(&hfdcan1,xTxCmdData))
#define Drv_CAN1_READ()                        (Drv_GetFirstRxBuff(&g_xCan1RxFifo))
#define Drv_CAN1_FIFO_RELEASE()                (Drv_DecRxBuff(&g_xCan1RxFifo))

#define Drv_CAN1_XCP_READ()                    (Drv_GetFirstRxBuff(&g_xCan1XCPFifo))
#define Drv_CAN1_XCP_FIFO_RELEASE()            (Drv_DecRxBuff(&g_xCan1XCPFifo))

#endif

#if USED_CAN2
#define Drv_CAN2_INIT()                        (Drv_CanInit(Drv_CAN2))
#define Drv_CAN2_CONFIGFILTER(xFilterConfig)   (Drv_CanConfigFilter(&hfdcan2,xFilterConfig))
#define Drv_CAN2_SEND(xTxCmdData)              (Drv_CanTxMessage(&hfdcan2,xTxCmdData))
#define Drv_CAN2_READ()                     (Drv_GetFirstRxBuff(&g_xCan2RxFifo))
#define Drv_CAN2_FIFO_RELEASE()                (Drv_DecRxBuff(&g_xCan2RxFifo))

#endif

/*******************************************************************************
 *---------------------------—----—--------变量接口------------------------------
 ******************************************************************************/
extern CAN_RX_FIFO g_xCan1RxFifo;
extern CAN_RX_FIFO g_xCan1XCPFifo;

#endif
/*******************************************************************************
 *---------------------------—----—---结束--------------------------------------
 ******************************************************************************/