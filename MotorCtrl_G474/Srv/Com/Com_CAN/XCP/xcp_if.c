/*******************************************************************************
 * xcp_def.h
 * Note:   Copy from Vector Informatik GmbH XCP on CAN demo
 * Created on: 2023/09/16
 * Author: bin.liao
 ******************************************************************************/


/*******************************************************************************
 *------------------------------Include Section---------------------------------
 ******************************************************************************/
#include "Srv.h"

/*******************************************************************************
 *-------------------------------Declaration------------------------------------
 ******************************************************************************/
#if defined ( kXcpStationIdLength )
V_MEMROM0 MEMORY_ROM vuint8 kXcpStationId[kXcpStationIdLength] = 
    kXcpStationIdString;
#endif

/*******************************************************************************
 *-----------------------Global variable definition-----------------------------
 ******************************************************************************/

/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  Receive Handle transmission done
  * @note   
  * @param  void
  * @retval void
 ******************************************************************************/
void XcpHandler( void )
{
    CanRxMessage *pxMsg;
    pxMsg = Drv_CAN1_XCP_READ();

    if(NULL != pxMsg)
    {
        XcpCommand( (void*)pxMsg->Data);
        Drv_CAN1_XCP_FIFO_RELEASE();
    }
}

/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  Requests for the transmission of a command transfer object (CTO) or
  * data transfer object (DTO). XcpSendCallBack must be called after the 
  * successful transmission of any XCP message. The XCP Protocol Layer will 
  * not request further transmissions, until XcpSendCallBack has been called.
  * @note   
  * @param  len  Length of message data
  * @param  msg  Pointer to message
  * @retval void  
 ******************************************************************************/
void ApplXcpSend( vuint8 len, MEMORY_ROM BYTEPTR msg )
{
    if(!msg)
    {
        return;
    }
    CanTxMessage pdu_data;
    pdu_data.CanTxHeader.Identifier = MOTOR_STATE_TXCANID;
    pdu_data.CanTxHeader.TxFrameType = FDCAN_DATA_FRAME;
    pdu_data.CanTxHeader.IdType = FDCAN_STANDARD_ID;
    pdu_data.CanTxHeader.FDFormat = FDCAN_CLASSIC_CAN;
    pdu_data.CanTxHeader.BitRateSwitch = FDCAN_BRS_OFF;
    pdu_data.CanTxHeader.MessageMarker = 0;
    pdu_data.CanTxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    pdu_data.CanTxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    pdu_data.CanTxHeader.DataLength = len;
    for(uint8_t i = 0; i < len; i++)
    {
        pdu_data.Data[i] = msg[i];
    }
    
    if(HAL_OK == Drv_CAN1_SEND(&pdu_data))
    {
        ApplXcpSendStall();
    }
}

/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  Resolve a transmit stall condition in XcpPutchar or XcpSendEvent.
  * @note   
  * @param  void
  * @retval 1  successful
 ******************************************************************************/
vuint8 ApplXcpSendStall( void )
{
    XcpSendCallBack();
    return 1;
}

/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  Convert a XCP address to a pointer.
  * @note   
  * @param  void
  * @retval addr
 ******************************************************************************/
MTABYTEPTR ApplXcpGetPointer( vuint8 addr_ext, vuint32 addr )
{
    //addr_ext = addr_ext;
    return (MTABYTEPTR)addr;
}

/* Flash Programming by Flash Kernel */
#if defined ( XCP_ENABLE_BOOTLOADER_DOWNLOAD )
/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  Prepare flash programming bootloader download.
  * @note   From PROGRAM_PREPARE
  * @param  void
  * @retval addr
 ******************************************************************************/
vuint8 ApplXcpDisableNormalOperation( MTABYTEPTR a, vuint16 s )
{
    if(!a)
    {
        return;
    }
    xcp.BootloaderStartAddr = (vuint32)a;

    // Check if size and address for bootloader download area
    // are not in conflict with anything else
    if (!(xcp.BootloaderStartAddr+s-1<(vuint32)&xcp || \
        xcp.BootloaderStartAddr>(vuint32)&xcp+sizeof(xcp)-1)) {
    return 0; // Access denied
    }

    return 1;
}

/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  Start XCP/CCP flash programming bootloader.
  * @note   From PROGRAM_START
  * @param  void
  * @retval addr
 ******************************************************************************/
vuint8 ApplXcpStartBootLoader( MTABYTEPTR a )
{
    if(!a)
    {
        return;
    }
    typedef void (*xcpBootLoader_t)(void);

    ((xcpBootLoader_t)a)();
    
    return 0;
}

#endif


/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  ApplXcpInterruptDisable
  * @note   In order to compile without error reporting, the interrupt is not 
  * operated in the actual project
  * @param  
  * @retval 
 ******************************************************************************/
void ApplXcpInterruptDisable( void ){}

/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  ApplXcpInterruptEnable
  * @note   In order to compile without error reporting, the interrupt is not 
  * operated in the actual project
  * @param  
  * @retval 
 ******************************************************************************/
void ApplXcpInterruptEnable( void ){}
