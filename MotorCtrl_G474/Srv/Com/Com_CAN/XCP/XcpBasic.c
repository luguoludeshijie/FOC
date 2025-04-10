/*******************************************************************************
 * XcpBasic.c
 * Note:   Copy from Vector Informatik GmbH XCP on CAN demo
 * Created on: 2023/09/16
 * Author: bin.liao
 ******************************************************************************/


/*******************************************************************************
 *------------------------------Include Section---------------------------------
 ******************************************************************************/
#include "xcpBasic.h"

/*******************************************************************************
 *-------------------------------Declaration------------------------------------
 ******************************************************************************/
/* XCP DPRAM Client Mode */

#if defined ( XCP_ENABLE_SERV_TEXT_PRINTF ) && defined ( XCP_ENABLE_TESTMODE )
    #include <stdio.h>
    #include <stdarg.h>
#endif

/******************************************************************************/
/*------------------------------Version check---------------------------------*/
/******************************************************************************/
/*---------------------------------------------------------------*/
#if ( CP_XCP_VERSION != 0x0130u )
    #error "Source and Header file are inconsistent!"
#endif
#if ( CP_XCP_RELEASE_VERSION != 0x04u )
    #error "Source and Header file are inconsistent!"
#endif

#if defined ( VGEN_GENY )
    #if defined ( CP_XCPDLL_VERSION ) 
        #if ( CP_XCPDLL_VERSION >= 0x0211u )
        #else
            #error "XCP component version and generator version are not \
                consistent!"
        #endif
    #else
        #if defined ( VGEN_VERSION_CP_XCP_MAIN )
            #if ((VGEN_VERSION_CP_XCP_MAIN > 1)||(VGEN_VERSION_CP_XCP_SUB > 5))
            #else
                #error "XCP component version and generator version are not \
                    consistent!"
            #endif
        #else
            #error "XCP component version and generator version are not \
                consistent!"
        #endif
    #endif
#endif

/* Check specifcation version of AUTOSAR CRC module. */

#if defined ( C_COMP_TASKING_ST10_CCAN ) || defined ( C_COMP_TASKING_C16X ) \
    || defined ( C_COMP_TASKING_XC16X )
    #if defined ( XCP_ENABLE_NO_P2INT_CAST ) ||\
        defined ( XCP_DISABLE_NO_P2INT_CAST )
    #else
/* Some uCs use DPPs in the small memory model and encode the information in 
    the most significant bits of pointers.*/
/* The DPP register information gets lost if these pointers are converted 
    to integers.*/
/* The following options disables casts from pointers to integer. */
        #define XCP_ENABLE_NO_P2INT_CAST
    #endif
#endif
/******************************************************************************/
/*----------------------------------Macros------------------------------------*/
/******************************************************************************/


/* Definition of macros that have to be used within the context of XcpCommand.*/
/* They have to be declared global Due to MISRA rule 91. */

/* PRQA S 3458 5 */ /* MD_MSR_19.4 */
#define error(e) { err=(e); goto negative_response; }
#define check_error(e) { err=(e); if (err!=0) { goto negative_response; } }
#define error1(e,b1)\
    { err=(e); CRM_BYTE(2)=(b1); xcp.CrmLen=3; goto negative_response1; }
#define error2(e,b1,b2) { err=(e); CRM_BYTE(2)=(b1); \
    CRM_BYTE(3)=(b2); xcp.CrmLen=4; goto negative_response1; }

#if defined ( XCP_ENABLE_SEED_KEY )
    /* Return CRC_ACCESS_LOCKED if the resource is locked. */
    #define CheckResourceProtection(resource)   if ( (xcp.ProtectionStatus & \
        (resource)) != 0 ) { error(CRC_ACCESS_LOCKED) }
    /* PRQA S 3412 */ /* MD_MSR_19.4 */
#else
    /* The resource protection is unavailable. */
    #define CheckResourceProtection(resource)
#endif



#if defined ( XCP_ENABLE_MEM_ACCESS_BY_APPL )
    #define XCP_WRITE_BYTE_2_ADDR(addr, data) \
        ApplXcpWrite( (vuint32)(addr), (vuint8)(data) )
    /* PRQA S 3453 */ /* MD_MSR_19.7 */
    #define XCP_READ_BYTE_FROM_ADDR(addr) \
        ApplXcpRead ( (vuint32)(addr) ) /* PRQA S 3453 */ /* MD_MSR_19.7 */
    #if defined ( XCP_ENABLE_CHECKSUM )
        #if ( kXcpChecksumMethod == XCP_CHECKSUM_TYPE_ADD22 ) || \
            ( kXcpChecksumMethod == XCP_CHECKSUM_TYPE_ADD24 ) || \
            ( kXcpChecksumMethod == XCP_CHECKSUM_TYPE_ADD44 )
        #define XCP_READ_CHECKSUMVALUE_FROM_ADDR(addr) \
            ApplXcpReadChecksumValue ( (vuint32)(addr) ) 
        /* PRQA S 3453 */ /* MD_MSR_19.7 */
        #else
            #define XCP_READ_CHECKSUMVALUE_FROM_ADDR(addr) \
                XCP_READ_BYTE_FROM_ADDR(addr)
            /* PRQA S 3453 *//* MD_MSR_19.7*/
        #endif
    #endif
#else
    #define XCP_WRITE_BYTE_2_ADDR(addr, data)  *(addr) = (data)
    /* PRQA S 3453 */ /* MD_MSR_19.7 */
    #define XCP_READ_BYTE_FROM_ADDR(addr)      *(addr)
    /* PRQA S 3453 */ /* MD_MSR_19.7 */
    #if defined ( XCP_ENABLE_CHECKSUM )
        #if defined ( XCP_ENABLE_CALIBRATION_MEM_ACCESS_BY_APPL )
        #define XCP_READ_CHECKSUMVALUE_FROM_ADDR(addr) \
            ApplXcpReadChecksumValue ( (vuint32)(addr) )
        /* PRQA S 3453 */ /* MD_MSR_19.7 */
        #else
        #define XCP_READ_CHECKSUMVALUE_FROM_ADDR(addr) \
            *((tXcpChecksumAddType*)(addr))
        /* PRQA S 3453 */ /* MD_MSR_19.7 */
        #endif
    #endif
#endif

/* PRQA S 3458 7 */ /* MD_MSR_19.4 */
#define XcpPlCheckControlState( )
#define XcpPlCheckControlStateRet( ret_value )

/******************************************************************************/
/*--------------------------------Constants-----------------------------------*/
/******************************************************************************/
/* Global constants with XCP Protocol Layer main and subversion */
V_MEMROM0 const vuint8 kXcpMainVersion    = (vuint8)(CP_XCP_VERSION >> 8);
V_MEMROM0 const vuint8 kXcpSubVersion     = (vuint8)(CP_XCP_VERSION & 0x00ff);
V_MEMROM0 const vuint8 kXcpReleaseVersion = (vuint8)(CP_XCP_RELEASE_VERSION);

/******************************************************************************/
/* Local data                                                                 */
/******************************************************************************/

/* This section containes all RAM locations needed by the XCP driver */

/******************************************************************************/
/* Local Data definitions with unspecified size                               */
/******************************************************************************/

RAM tXcpData xcp; /* PRQA S 3408 */ /* MD_Xcp_3408 */

#if defined ( XCP_ENABLE_SEND_QUEUE )
#else
    static tXcpDto dto;
#endif


#if defined ( XCP_ENABLE_TESTMODE )
    vuint8 gDebugLevel;
#endif

/******************************************************************************/
/* Local Data definitions with 8Bit size                                      */
/******************************************************************************/




/*****************************************************************************/
/* Prototypes for local functions                                            */
/*****************************************************************************/

static vuint8 XcpWriteMta( vuint8 size, const BYTEPTR data );
static vuint8 XcpReadMta( vuint8 size, BYTEPTR data );

#if defined ( XcpMemClr )
#else
static void XcpMemClr( BYTEPTR p, vuint16 n );
#endif

#if defined ( XCP_ENABLE_SEND_EVENT ) || defined ( XCP_ENABLE_SERV_TEXT )
static void XcpSendEv( void );
#endif

#if defined ( XCP_ENABLE_SEND_QUEUE )
static vuint8 XcpSendDtoFromQueue( void );
static void XcpQueueInit( void );
#endif

#if defined ( XCP_ENABLE_DAQ )
static void XcpFreeDaq( void );
static vuint8 XcpAllocMemory( void );
static vuint8 XcpAllocDaq( vuint8 daqCount );
static vuint8 XcpAllocOdt( vuint8 daq, vuint8 odtCount );
static vuint8 XcpAllocOdtEntry( vuint8 daq, vuint8 odt, vuint8 odtEntryCount );
static void XcpStartDaq( vuint8 daq );
static void XcpStartAllSelectedDaq( void );
static void XcpStopDaq( vuint8 daq );
static void XcpStopAllSelectedDaq( void );
static void XcpStopAllDaq( void );
#endif

/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  Initialize n bytes starting from address p with b.
  * @note   CALLED BY:  XcpFreeDaq
  * @param  p : pointer to start address.
  *         n : number of data bytes.
  *         d : data byte to initialize with.
  * @retval void
 ******************************************************************************/
#if defined ( XcpMemSet )
 /* XcpMemSet is overwritten */
#else
void XcpMemSet( BYTEPTR p, vuint16 n, vuint8 b )
{
    if(!p)
    {
        return;
    }
    for ( ; n > 0; n-- )
    {
        *p = b;
        p++; /* PRQA S 0489 */ /* MD_Xcp_0489 */
    }
}
#endif

/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  Initialize n bytes starting from address p 0.
  * @note   CALLED BY:  XcpFreeDaq, XcpInit
  * @param  p : pointer to start address.
  *         n : number of data bytes.
  * @retval void
 ******************************************************************************/
#if defined ( XcpMemClr )
 /* XcpMemClr is overwritten */
#else
/* A macro would be more efficient. Due to MISRA rule 96 violations a function 
is implemented. */
static void XcpMemClr( BYTEPTR p, vuint16 n )
{
    if( !p )
    {
        return;
    }
    XcpMemSet( p, n, (vuint8)0u);
}
#endif

/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  Copy n bytes from src to dest.A maximum of 255 Bytes can be copied 
  * at once.
  * @note   CALLED BY:  XcpEvent
  * @param  dest : pointer to destination address.
  *         src  : pointer to source address.
  *         n    : number of data bytes to copy.
  * @retval void
 ******************************************************************************/
/* Optimize this function 
   It is used in the inner loop of XcpEvent for data acquisition sampling */

#if defined ( XcpMemCpy ) || defined ( C_COMP_COSMIC_MCS12X_MSCAN12 )
 /* XcpMemCpy is overwritten */
#else
void XcpMemCpy( DAQBYTEPTR dest, const DAQBYTEPTR src, vuint8 n )
{
    if( ( !dest ) || ( !src ) )
    {
        return;
    }
    for ( ; n > 0; n-- )
    {
        /* ESCAN00092933 */
        XCP_WRITE_BYTE_2_ADDR( dest, XCP_READ_BYTE_FROM_ADDR(src) );
        dest++; /* PRQA S 0489 */ /* MD_Xcp_0489 */
        src++; /* PRQA S 0489 */ /* MD_Xcp_0489 */
    }
}
#endif

/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  Set the maximum cto/dto to be used by the XCP.
  * @note   CALLED BY:  XcpInit, Tl
  * @param  MaxCto : Maximum CTO to be used
  *         MaxDto : Maximum DTO to be used
  *         n    : number of data bytes to copy.
  * @retval void
 ******************************************************************************/
void XcpSetActiveTl( vuint8 MaxCto, vuint8 MaxDto, vuint8 ActiveTl )
{
    xcp.Daq.ActiveTl  = ActiveTl;
    #if defined ( V_ENABLE_USE_DUMMY_STATEMENT )
        MaxCto = MaxCto;
        MaxDto = MaxDto;
    #endif
}

/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  Get the currently active TL.
  * @note   ALLED BY:  Tl
  * @param  void
  * @retval Active Tl
 ******************************************************************************/
vuint8 XcpGetActiveTl( void )
{
    return( xcp.Daq.ActiveTl );
}

/******************************************************************************/
/* Transmit                                                                   */
/******************************************************************************/

/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  Transmission of a command response packet (RES),or error packet 
  * (ERR) if no other packet is pending.
  * @note   CALLED BY:  XcpBackground, XcpCommand, XcpSendCallBack, application
  * PRECONDITIONS ：XCP is initialized and in connected state and a command 
  * packet (CMD)
  *  CALLED BY:  XcpBackground, XcpCommand, XcpSendCallBack, application
  * @param  void
  * @retval void
 ******************************************************************************/
void XcpSendCrm( void )
{
    /* Activation control */
    XcpPlCheckControlState()

    #if defined ( XCP_ENABLE_SEND_QUEUE )

    ApplXcpInterruptDisable();

    if ( (xcp.SendStatus & (vuint8)XCP_SEND_PENDING) != 0 )
    {
        if ( (xcp.SendStatus & (vuint8)XCP_CRM_REQUEST) != 0 )
        {
        XCP_ASSERT(0);
        xcp.SessionStatus |= (SessionStatusType)SS_ERROR; 
        }
        xcp.SendStatus |= (vuint8)XCP_CRM_REQUEST;
    } 
    else
    {
        xcp.SendStatus |= (vuint8)XCP_CRM_PENDING;
        ApplXcpSend(xcp.CrmLen,&xcp.Crm.b[0]);
    }

        ApplXcpInterruptEnable();

    #else

    ApplXcpSend(xcp.CrmLen,&xcp.Crm.b[0]);

    #endif

    ApplXcpSendFlush();
    /* PRQA S 2006 1 */ /* MD_MSR_14.7 */
}

#if defined ( XCP_ENABLE_SEND_EVENT ) || defined ( XCP_ENABLE_SERV_TEXT )
/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  Send a EV or SERV message, if no other message is pending.
  * @note   CALLED BY:  XcpSendEvent, XcpSendCallBack, XcpPutchar
  * @param  void
  * @retval void
 ******************************************************************************/
static void XcpSendEv( void )
{
    #if defined ( XCP_ENABLE_SEND_QUEUE )

    ApplXcpInterruptDisable();

    if ( (xcp.SendStatus & (vuint8)XCP_SEND_PENDING) != 0 )
    {
        if ( (xcp.SendStatus & (vuint8)XCP_EVT_REQUEST) != 0 )
        {
        XCP_ASSERT(0);
        xcp.SessionStatus |= (SessionStatusType)SS_ERROR; 
        }
        xcp.SendStatus |= (vuint8)XCP_EVT_REQUEST;
    }
    else
    {
        xcp.SendStatus |= (vuint8)XCP_EVT_PENDING;
        ApplXcpSend(xcp.EvLen,xcp.Ev.b);
        xcp.EvLen = 0;
    }

    ApplXcpInterruptEnable();

    #else

    ApplXcpSend(xcp.EvLen,xcp.Ev.b);
    xcp.EvLen = 0;
        
    #endif

    ApplXcpSendFlush();
}
#endif /* XCP_ENABLE_SEND_EVENT || XCP_ENABLE_SERV_TEXT */


#if defined ( XCP_ENABLE_DAQ )
/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  Send a DTO.
  * @note   CALLED BY:  XcpSendDtoFromQueue, XcpEvent, XcpSendCallBack
  * @param  dto : pointer to XCP packet type definition
  * @retval void
 ******************************************************************************/
#if defined ( XcpSendDto )
/* XcpSendDto is redefined */
#else
void XcpSendDto( const tXcpDto *dto )
{
    if(!dto)
    {
        return;
    }
    ApplXcpSend( dto->l, &dto->b[0] );
}
#endif
#endif /* XCP_ENABLE_DAQ */


#if defined ( XCP_ENABLE_SEND_QUEUE )
/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  Send a DTO from the queue.
  * @note   CALLED BY:  XcpEvent, XcpSendCallBack
  * @param  dto : pointer to XCP packet type definition
  * @retval 0 : DTO has NOT been transmitted from queue. 
  *         1 : DTO has been transmitted from queue. 
 ******************************************************************************/
static vuint8 XcpSendDtoFromQueue( void )
{
    ApplXcpInterruptDisable();
    if ( ( (xcp.SendStatus & (vuint8)XCP_SEND_PENDING) == 0 ) && 
        ( xcp.QueueLen != 0 ))
    {
        xcp.SendStatus |= (vuint8)XCP_DTO_PENDING;
        XcpSendDto(&xcp.pQueue[xcp.QueueRp]);
        xcp.QueueRp++;
        if ( xcp.QueueRp >= xcp.QueueSize )
        {
            xcp.QueueRp = (vuint16)0u;
        }
        xcp.QueueLen--;
        ApplXcpInterruptEnable();
        return (vuint8)1u;
    }
    ApplXcpInterruptEnable();
    return (vuint8)0u;
    /* PRQA S 2006 1 */ /* MD_MSR_14.7 */
}
#endif /* XCP_ENABLE_SEND_QUEUE */


/******************************************************************************/
/* Transmit Queue                                                             */
/******************************************************************************/

#if defined ( XCP_ENABLE_SEND_QUEUE )
/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  Initialize the transmit queue.
  * @note   CALLED BY:  XcpFreeDaq, XcpStopDaq, XcpStopAllDaq
  * @param  
  * @retval void
 ******************************************************************************/
static void XcpQueueInit(void)
{
     xcp.QueueLen = (vuint16)0u;
     xcp.QueueRp = (vuint16)0u;
}

#endif /* XCP_ENABLE_SEND_QUEUE */

/******************************************************************************/
/* Handle Mta (Memory-Transfer-Address)                                       */
/******************************************************************************/

/* Assign a pointer to a Mta */
#if defined ( XcpSetMta )
#else
    #define XcpSetMta(p,e) (xcp.Mta = (p)) /* PRQA S 3453 */ /* MD_MSR_19.7 */
#endif

/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  Write n bytes.Copying of size bytes from data to xcp.Mta
  * @note   CALLED BY:  XcpCommand
  * @param  size : number of data bytes.
  *         data : address of data.
  * @retval XCP_CMD_OK, XCP_CMD_DENIED
 ******************************************************************************/
static vuint8 XcpWriteMta( vuint8 size, const BYTEPTR data )
{
    if( !data )
    {
        return FALSE;
    }

    #if defined ( XCP_ENABLE_CALIBRATION_MEM_ACCESS_BY_APPL )
    vuint8 r;
    #endif

    /* DPRAM Client */

    /* Checked ram memory write access */

    /* EEPROM write access */

    /* Standard RAM memory write access */
    #if defined ( XCP_ENABLE_CALIBRATION_MEM_ACCESS_BY_APPL ) && \
        !defined ( XCP_ENABLE_MEM_ACCESS_BY_APPL )
    r = ApplXcpCalibrationWrite(xcp.Mta, size, data);
    xcp.Mta += size; /* PRQA S 0488 */ /* MD_Xcp_0488 */
    return r;
    #else
    while ( size > (vuint8)0u ) 
    {
        XCP_WRITE_BYTE_2_ADDR( xcp.Mta, *data );
        xcp.Mta++; /* PRQA S 0489 */ /* MD_Xcp_0489 */
        data++; /* PRQA S 0489 */ /* MD_Xcp_0489 */
        size--;
    }
    return (vuint8)XCP_CMD_OK;
    #endif
  /* PRQA S 2006 1 */ /* MD_MSR_14.7 */
}

/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  Read n bytes.Copying of size bytes from data to xcp.Mta
  * @note   CALLED BY:  XcpCommand
  * @param  size : 
  *         data : address of data.
  * @retval XCP_CMD_OK
 ******************************************************************************/
static vuint8 XcpReadMta( vuint8 size, BYTEPTR data )
{
    if( !data )
    {
        return FALSE;
    }

    #if defined ( XCP_ENABLE_CALIBRATION_MEM_ACCESS_BY_APPL )
    vuint8 r;
    #endif

    /* DPRAM Client */

    /* Checked ram memory read access */

    /* EEPROM read access */

    #if defined ( XCP_ENABLE_CALIBRATION_MEM_ACCESS_BY_APPL ) && \
        !defined ( XCP_ENABLE_MEM_ACCESS_BY_APPL )
    r = ApplXcpCalibrationRead(xcp.Mta, size, data);
    xcp.Mta += size; /* PRQA S 0488 */ /* MD_Xcp_0488 */
    return r;
    #else
    /* Standard RAM memory read access */
    while (size > 0)
    {
        /* 
        Compiler bug Tasking
        *(data++) = *(xcp.Mta++);
        */
        *(data) = XCP_READ_BYTE_FROM_ADDR( xcp.Mta );
        data++; /* PRQA S 0489 */ /* MD_Xcp_0489 */
        xcp.Mta++; /* PRQA S 0489 */ /* MD_Xcp_0489 */
        size--;
    }
    return (vuint8)XCP_CMD_OK;
    #endif
    /* PRQA S 2006 1 */ /* MD_MSR_14.7 */
}

/******************************************************************************/
/* Data Aquisition Setup                                                      */
/******************************************************************************/

#if defined ( XCP_ENABLE_DAQ )
/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  Free all dynamic DAQ lists
  * @note   CALLED BY:  XcpCommand
  * @param  void
  * @retval void
 ******************************************************************************/
static void XcpFreeDaq( void )
{
    xcp.SessionStatus &= (SessionStatusType)((~SS_DAQ) & 0xFFu);

    xcp.Daq.DaqCount = 0;
    xcp.Daq.OdtCount = 0;
    xcp.Daq.OdtEntryCount = 0;

    xcp.pOdt = (tXcpOdt*)0;
    xcp.pOdtEntryAddr = 0;
    xcp.pOdtEntrySize = 0;

    XcpMemClr((BYTEPTR)&xcp.Daq.u.b[0], (vuint16)kXcpDaqMemSize);
    /* Deviation of MISRA rule 44. */
    #if defined ( kXcpMaxEvent ) && ! defined ( XCP_ENABLE_DAQ_PRESCALER )
    XcpMemSet( (BYTEPTR)&xcp.Daq.EventDaq[0],\
        (vuint16)sizeof(xcp.Daq.EventDaq), (vuint8)0xFFu);
        /* Deviation of MISRA rule 44. */
    #endif

    #if defined ( XCP_ENABLE_SEND_QUEUE )
    XcpQueueInit();
    #endif
}

/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  Allocate Memory for daq,odt,odtEntries and Queue according to
  *     DaqCount, OdtCount and OdtEntryCount
  * @note   CALLED BY:  XcpAllocDaq, XcpAllocOdt, XcpAllocOdtEntry, XcpInit
  * @param  void
  * @retval 0, CRC_MEMORY_OVERFLOW
 ******************************************************************************/
static vuint8 XcpAllocMemory( void )
{
    vuint16 s;
    #if defined ( XCP_ENABLE_NO_P2INT_CAST  )
        vuint8* p;
        vuint8  i;
    #endif

    /* Check memory overflow */
    s = (vuint16)( ( xcp.Daq.DaqCount      *   (vuint8)sizeof(tXcpDaqList)) + 
                    ( xcp.Daq.OdtCount      *  (vuint16)sizeof(tXcpOdt)) + 
                    ( xcp.Daq.OdtEntryCount * 
                    ( (vuint8)sizeof(DAQBYTEPTR) + (vuint8)sizeof(vuint8) ) )
                );
    #if defined ( XCP_DISABLE_UNALIGNED_MEM_ACCESS )
        #if defined ( C_CPUTYPE_8BIT ) || defined ( C_CPUTYPE_16BIT )
        s += 5;
        /* Worst case 5 bytes needed for alignment */ /* ESCAN00091645 */
        #else
        s += 9;
        /* Worst case 9 bytes needed for alignment */
        #endif
    #endif

    if (s>=(vuint16)kXcpDaqMemSize) 
    {
        return (vuint8)CRC_MEMORY_OVERFLOW;
    }

    /* Force WORD alignment for ODTs */
    #if defined ( XCP_DISABLE_UNALIGNED_MEM_ACCESS )
        #if defined ( XCP_ENABLE_NO_P2INT_CAST  )
        p = (vuint8*)&xcp.Daq.u.DaqList[xcp.Daq.DaqCount];
        i = ((vuint8)p) & (vuint8)0x01u;
        p += i;
        xcp.pOdt = (tXcpOdt*)p;
        #else
            #if defined ( C_CPUTYPE_8BIT ) || defined ( C_CPUTYPE_16BIT )
            /* Align to words */
            xcp.pOdt = (tXcpOdt*)((((vuint32)
                (&xcp.Daq.u.DaqList[xcp.Daq.DaqCount])) + 
                (vuint32)1u) & (vuint32)0xFFFFFFFEu );
            /* PRQA S 0306 */ /* MD_Xcp_0306 */
            #else
            /* Align to dwords */
            xcp.pOdt = (tXcpOdt*)((((vuint32)
                (&xcp.Daq.u.DaqList[xcp.Daq.DaqCount])) + 
                (vuint32)3u) & (vuint32)0xFFFFFFFCu );
            /* PRQA S 0306 */ /* MD_Xcp_0306 */
            #endif
        #endif
    #else
    xcp.pOdt = (tXcpOdt*)&xcp.Daq.u.DaqList[xcp.Daq.DaqCount];
    #endif
  
    /* Force DWORD alignment for ODT entries */
    #if defined ( XCP_DISABLE_UNALIGNED_MEM_ACCESS )
        #if defined ( XCP_ENABLE_NO_P2INT_CAST  )
        p = (vuint8*)&xcp.pOdt[xcp.Daq.OdtCount];
        i = ((vuint8)p) & (vuint8)0x03u;
        i = (vuint8)4u - i;
        i &= 0x03;
        p += i;
        xcp.pOdtEntryAddr = (DAQBYTEPTR*)p;
        #else
        xcp.pOdtEntryAddr = (DAQBYTEPTR*)
            ((((vuint32)&xcp.pOdt[xcp.Daq.OdtCount]) + 
            (vuint32)3u) & (vuint32)0xFFFFFFFCu );
        /* PRQA S 0306 */ /* MD_Xcp_0306 */
        #endif
    #else
    xcp.pOdtEntryAddr = (DAQBYTEPTR*)&xcp.pOdt[xcp.Daq.OdtCount];
    #endif
    xcp.pOdtEntrySize = (vuint8*)&xcp.pOdtEntryAddr[xcp.Daq.OdtEntryCount];
    /* PRQA S 0310 */ /* MD_Xcp_0310 */

    #if defined ( XCP_ENABLE_SEND_QUEUE )
    /* Force WORD alignment for the queue */
        #if defined ( XCP_DISABLE_UNALIGNED_MEM_ACCESS )
            #if defined ( XCP_ENABLE_NO_P2INT_CAST )
            p = (vuint8*)&xcp.pOdtEntrySize[xcp.Daq.OdtEntryCount];
            i = ((vuint8)p) & (vuint8)0x01u;
            p += i;
            xcp.pQueue = (tXcpDto*)p;
            #else
                #if defined ( C_CPUTYPE_8BIT ) || defined ( C_CPUTYPE_16BIT )
                /* Allign to words */
                xcp.pQueue = (tXcpDto*)((((vuint32)
                    (&xcp.pOdtEntrySize[xcp.Daq.OdtEntryCount])) + \
                    (vuint32)1u) & (vuint32)0xFFFFFFFEu );
                /* PRQA S 0306 */ /* MD_Xcp_0306 */
                #else
                /* Allign to dwords */
                xcp.pQueue = (tXcpDto*)((((vuint32)
                    (&xcp.pOdtEntrySize[xcp.Daq.OdtEntryCount])) + \
                    (vuint32)3u) & (vuint32)0xFFFFFFFCu );
                /* PRQA S 0306 */ /* MD_Xcp_0306 */
                #endif
            #endif
        #else
        xcp.pQueue = (tXcpDto*)&xcp.pOdtEntrySize[xcp.Daq.OdtEntryCount];
        #endif

        xcp.QueueSize = ((vuint16)kXcpDaqMemSize - s) / sizeof(tXcpDto);

        #if defined ( kXcpSendQueueMinSize )
        if (xcp.QueueSize<(vuint16)kXcpSendQueueMinSize)
        {
            return (vuint8)CRC_MEMORY_OVERFLOW;
        }
        #else
        /* At least one queue entry per odt */
        if (xcp.QueueSize<xcp.Daq.OdtCount)
        {
            return (vuint8)CRC_MEMORY_OVERFLOW;
        }
        #endif
    #endif /* XCP_ENABLE_SEND_QUEUE */

    #if defined ( XCP_ENABLE_TESTMODE )
    if ( gDebugLevel != 0)
    {
        ApplXcpPrint("[XcpAllocMemory] %u/%u Bytes used\n",s,kXcpDaqMemSize );
        #if defined ( XCP_ENABLE_SEND_QUEUE )
        ApplXcpPrint("[XcpAllocMemory] Queuesize=%u\n",xcp.QueueSize );
        #endif
    }
    #endif

    return (vuint8)0u;
    /* PRQA S 2006 1 */ /* MD_MSR_14.7 */
}

/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  Allocate DAQ list
  * @note   CALLED BY:  XcpCommand
  * @param  daqCount
  * @retval return value of XcpAllocMemory, CRC_SEQUENCE
 ******************************************************************************/
static vuint8 XcpAllocDaq( vuint8 daqCount )
{
    #if defined ( XCP_ENABLE_PARAMETER_CHECK )
    if ( (xcp.Daq.OdtCount!=0) || (xcp.Daq.OdtEntryCount!=0) )
    {
        return (vuint8)CRC_SEQUENCE;
    }
    if( daqCount == 0 )
    {
        return (vuint8)CRC_OUT_OF_RANGE;
    }
    #endif

    xcp.Daq.DaqCount = daqCount;

    return XcpAllocMemory();
    /* PRQA S 2006 1 */ /* MD_MSR_14.7 */
}

/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  Allocate all ODTs in a DAQ list
  * @note   CALLED BY:  XcpCommand
  * @param  daq
  *         daqCount
  * @retval return value of XcpAllocMemory, CRC_SEQUENCE, CRC_MEMORY_OVERFLOW
 ******************************************************************************/
static vuint8 XcpAllocOdt( vuint8 daq, vuint8 odtCount )
{
    #if defined ( XCP_ENABLE_PARAMETER_CHECK )
    if ( (xcp.Daq.DaqCount==0) || (xcp.Daq.OdtEntryCount!=0) )
    {
        return (vuint8)CRC_SEQUENCE;
    }
    if( odtCount == 0 )
    {
        return (vuint8)CRC_OUT_OF_RANGE;
    }
    #endif

    /* Absolute ODT count must fit in a BYTE */
    #if !defined ( XCP_ENABLE_DAQ_HDR_ODT_DAQ )
        #if defined XCP_ENABLE_DAQ_OVERRUN_INDICATION
        if (((vuint16)xcp.Daq.OdtCount+(vuint16)odtCount) > (vuint16)0x7Bu)
        /* ESCAN00090685 */ /* ESCAN00092934 */
        #else
        if (((vuint16)xcp.Daq.OdtCount+(vuint16)odtCount) > (vuint16)0xFBu)
        #endif
        {
            return (vuint8)CRC_MEMORY_OVERFLOW;
        }
    #endif

    xcp.Daq.u.DaqList[daq].firstOdt = xcp.Daq.OdtCount;
    xcp.Daq.OdtCount += odtCount;
    xcp.Daq.u.DaqList[daq].lastOdt = (tXcpOdtIdx)(xcp.Daq.OdtCount-1);

    return XcpAllocMemory();
    /* PRQA S 2006 1 */ /* MD_MSR_14.7 */
}

/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  Allocate all ODT entries, Parameter odt is relative odt number
  * @note   CALLED BY:  XcpCommand
  * @param  daq
  *         odt
  *         odtEntryCount
  * @retval return value of XcpAllocMemory
 ******************************************************************************/
static vuint8 XcpAllocOdtEntry( vuint8 daq, vuint8 odt, vuint8 odtEntryCount )
{
    tXcpOdtIdx xcpFirstOdt;

    #if defined ( XCP_ENABLE_PARAMETER_CHECK )
    if ( (xcp.Daq.DaqCount==0) || (xcp.Daq.OdtCount==0) )
    {
        return (vuint8)CRC_SEQUENCE;
    }
    if (odtEntryCount==0)
    {
        return (vuint8)CRC_OUT_OF_RANGE;
    }
    #endif

    /* Absolute ODT entry count count must fit in a WORD */
    if (xcp.Daq.OdtEntryCount > (0xFFFFu - odtEntryCount))
    {
        return (vuint8)CRC_MEMORY_OVERFLOW;
    }
    xcpFirstOdt = xcp.Daq.u.DaqList[daq].firstOdt;
    xcp.pOdt[xcpFirstOdt+odt].firstOdtEntry = xcp.Daq.OdtEntryCount;
    xcp.Daq.OdtEntryCount += (vuint16)odtEntryCount;
    xcp.pOdt[xcpFirstOdt+odt].lastOdtEntry = (vuint16)(xcp.Daq.OdtEntryCount-1);

    return XcpAllocMemory();
    /* PRQA S 2006 1 */ /* MD_MSR_14.7 */
}

/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  Start DAQ
  * @note   CALLED BY:  XcpCommand, XcpStartAllSelectedDaq
  * @param  daq
  * @retval void
 ******************************************************************************/
static void XcpStartDaq( vuint8 daq )
{
    /* Initialize the DAQ list */
    DaqListFlags(daq) |= (vuint8)DAQ_FLAG_RUNNING;
    #if defined ( XCP_ENABLE_DAQ_PRESCALER )
    DaqListCycle(daq) = 1;
    #endif

    xcp.SessionStatus |= (SessionStatusType)SS_DAQ;
}

/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  Start all selected DAQs
  * @note   CALLED BY:  XcpCommand, XcpInit
  * @param  void
  * @retval void
 ******************************************************************************/
static void XcpStartAllSelectedDaq(void)
{
    vuint8 daq;
    /* Start all selected DAQs */
    for (daq=0;daq<xcp.Daq.DaqCount;daq++)
    {
        if ( (DaqListFlags(daq) & (vuint8)DAQ_FLAG_SELECTED) != 0 )
        {
            XcpStartDaq(daq);
            DaqListFlags(daq) &= (vuint8)(~DAQ_FLAG_SELECTED & 0x00FFu);
        }
    }
}

/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  Stop DAQ
  * @note   CALLED BY:  XcpCommand, XcpStopAllSelectedDaq
  * @param  daq
  * @retval void
 ******************************************************************************/
static void XcpStopDaq( vuint8 daq )
{
    vuint8 i;

    DaqListFlags(daq) &= (vuint8) \
        (DAQ_FLAG_DIRECTION|DAQ_FLAG_TIMESTAMP|DAQ_FLAG_NO_PID);

    /* Check if all DAQ lists are stopped */
    for (i=0;i<xcp.Daq.DaqCount;i++)
    {
        if ( (DaqListFlags(i) & (vuint8)DAQ_FLAG_RUNNING) != 0 )
        {
            return;
        }
    }

    xcp.SessionStatus &= (SessionStatusType)(~SS_DAQ & 0x00FFu);

    #if defined ( XCP_ENABLE_SEND_QUEUE )
    XcpQueueInit();
    #endif
    /* PRQA S 2006 1 */ /* MD_MSR_14.7 */
}

/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  Stop all selected DAQs
  * @note   CALLED BY:  XcpCommand
  * @param  void
  * @retval void
 ******************************************************************************/
static void XcpStopAllSelectedDaq(void)
{
    vuint8 daq;

    for (daq=0;daq<xcp.Daq.DaqCount;daq++)
    {
        if ( (DaqListFlags(daq) & (vuint8)DAQ_FLAG_SELECTED) != 0 )
        {
            XcpStopDaq(daq);
            DaqListFlags(daq) &= (vuint8)(~DAQ_FLAG_SELECTED & 0x00FFu);
        }
    }
}

/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  Stop all DAQs
  * @note   CALLED BY:  XcpCommand, XcpDisconnect
  * @param  void
  * @retval void
 ******************************************************************************/
static void XcpStopAllDaq( void )
{
    vuint8 daq;

    for (daq=0; daq<xcp.Daq.DaqCount; daq++)
    {
        DaqListFlags(daq) &= (vuint8) \
            (DAQ_FLAG_DIRECTION|DAQ_FLAG_TIMESTAMP|DAQ_FLAG_NO_PID);
    }

    xcp.SessionStatus &= (SessionStatusType)(~SS_DAQ & 0x00FFu);

    #if defined ( XCP_ENABLE_SEND_QUEUE )
    XcpQueueInit();
    #endif
}

/******************************************************************************/
/* Data Aquisition Processor                                                  */
/******************************************************************************/

/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  Handling of data acquisition or stimulation event channel.
  * @note   PRECONDITIONS ：The XCP is initialized and in connected state.
  *         CALLED BY:  application
  * @param  event : event channel number to process
  * @retval status code (XCP_EVENT_...)
 ******************************************************************************/
vuint8 XcpEvent( vuint8 event )
{
    tXcpDto *dtop;
    BYTEPTR d;
    vuint8  status;
    vuint16 e,el;
    vuint8  n;
    vuint8  daq;
    tXcpOdtIdx odt;
    vuint8  i;
    #if defined ( XCP_ENABLE_DAQ_TIMESTAMP ) && \
        defined ( XCP_DISABLE_UNALIGNED_MEM_ACCESS )
    XcpDaqTimestampType t;
    #endif
    #if defined ( C_COMP_COSMIC_MCS12X_MSCAN12 )
    DAQBYTEPTR src;
    #endif

    status = (vuint8)0u;

    if ( (xcp.SessionStatus & (SessionStatusType)SS_DAQ) == 0 )
    {
        return (vuint8)XCP_EVENT_NOP;
    }

    /* Activation control */
    XcpPlCheckControlStateRet((vuint8)XCP_EVENT_NOP)

    #if defined ( kXcpMaxEvent ) && ! defined ( XCP_ENABLE_DAQ_PRESCALER )

        #if defined ( XCP_ENABLE_PARAMETER_CHECK )
        if (event >= (vuint8)kXcpMaxEvent)
        {
            return (vuint8)XCP_EVENT_NOP;
        }
        #endif

        BEGIN_PROFILE(4); /* Timingtest */
        daq = xcp.Daq.EventDaq[event];
        if ( ((daq<xcp.Daq.DaqCount) && \
            ( (DaqListFlags(daq) & (vuint8)DAQ_FLAG_RUNNING) != 0 )) != 0 )
        {

    #else

    BEGIN_PROFILE(4); /* Timingtest */
    for (daq=0; daq<xcp.Daq.DaqCount; daq++)
    {
        if ( (DaqListFlags(daq)& (vuint8)DAQ_FLAG_RUNNING) == 0 )
        {
            continue; /* PRQA S 0770 */ /* MD_Xcp_0770 */
        }
        if ( DaqListEventChannel(daq) != event )
        {
            continue; /* PRQA S 0770 */ /* MD_Xcp_0770 */
        }

    #endif

    #if defined ( XCP_ENABLE_DAQ_PRESCALER )
        DaqListCycle(daq)--;
        if ( DaqListCycle(daq) == (vuint8)0 )
        {
            DaqListCycle(daq) = DaqListPrescaler(daq);
    #endif

            /* Data Stimulation (STIM) */

            for (odt=DaqListFirstOdt(daq);odt<=DaqListLastOdt(daq);odt++)
            {
                #if defined ( XCP_ENABLE_SEND_QUEUE )
                vuint16 qs;
                #endif
                status |= (vuint8)XCP_EVENT_DAQ;

                ApplXcpInterruptDisable();
                /* The following code is not reentrant */

    #if defined ( XCP_ENABLE_SEND_QUEUE )
                /* Check if there is space in the queue for this ODT */
                if (xcp.QueueLen>=xcp.QueueSize)
                {
                    status |= (vuint8)XCP_EVENT_DAQ_OVERRUN;
                    /* Queue overflow */
                    DaqListFlags(daq) |= (vuint8)DAQ_FLAG_OVERRUN;
                    goto next_odt; /* PRQA S 2001 */ /* MD_Xcp_2001 */
                }

                qs = (xcp.QueueRp + xcp.QueueLen);
                if(qs >= xcp.QueueSize)
                {
                    qs -= xcp.QueueSize;
                }

                dtop = &xcp.pQueue[qs];
        #if defined ( XCP_SEND_QUEUE_SAMPLE_ODT )
                xcp.QueueLen++;
        #endif
    #else
                dtop = &dto;
    #endif /* XCP_ENABLE_SEND_QUEUE */

    #if defined ( XCP_ENABLE_DAQ_HDR_ODT_DAQ )

                /* ODT,DAQ */
                dtop->b[0] = odt-DaqListFirstOdt(daq);
                /* Relative odt number */
                dtop->b[1] = daq;
                i = 2;

    #else

                /* PID */
                dtop->b[0] = odt;
                i = 1;

    #endif

                /* Use BIT7 of PID or ODT to indicate overruns */
    #if defined ( XCP_ENABLE_SEND_QUEUE )
        #if defined ( XCP_ENABLE_DAQ_OVERRUN_INDICATION )
                if ( (DaqListFlags(daq) & (vuint8)DAQ_FLAG_OVERRUN) != 0 )
                {
                    dtop->b[0] |= (vuint8)0x80;
                    DaqListFlags(daq) &= (vuint8)(~DAQ_FLAG_OVERRUN & 0xFFu);
                }
        #endif
    #endif

    /* Timestamps */
    #if defined ( XCP_ENABLE_DAQ_TIMESTAMP )

        #if !defined ( XCP_ENABLE_DAQ_TIMESTAMP_FIXED )
                if ( (DaqListFlags(daq) & (vuint8)DAQ_FLAG_TIMESTAMP) != 0 )
                {
        #endif

                    if (odt==DaqListFirstOdt(daq))
                    {
        #if defined ( XCP_DISABLE_UNALIGNED_MEM_ACCESS )
                        t = ApplXcpGetTimestamp();
        #endif

        #if defined ( XCP_ENABLE_DAQ_HDR_ODT_DAQ )

                        *(XcpDaqTimestampType*)&dtop->b[2]=ApplXcpGetTimestamp();
                        i = 2 + XcpDaqTimestampSize;

        #else /* XCP_ENABLE_DAQ_HDR_ODT_DAQ */

            #if defined ( XCP_DISABLE_UNALIGNED_MEM_ACCESS )
                #if defined ( XCP_CPUTYPE_BIGENDIAN )  /* Avoid WORD access */
                    #if( kXcpDaqTimestampSize == DAQ_TIMESTAMP_BYTE )
                        dtop->b[i+0u] = (vuint8)t;
                    #elif( kXcpDaqTimestampSize == DAQ_TIMESTAMP_WORD )
                        dtop->b[i+1u] = (vuint8)t;
                        dtop->b[i+0u] = (vuint8)(t>>8);
                    #elif( kXcpDaqTimestampSize == DAQ_TIMESTAMP_DWORD )
                        dtop->b[i+3u] = (vuint8)t;
                        dtop->b[i+2u] = (vuint8)(t>>8);
                        dtop->b[i+1u] = (vuint8)(t>>16);
                        dtop->b[i+0u] = (vuint8)(t>>24);
                    #endif
                #else
                        dtop->b[i+0u] = (vuint8)t;
                    #if( kXcpDaqTimestampSize > DAQ_TIMESTAMP_BYTE )
                        dtop->b[i+1u] = (vuint8)(t>>8);
                    #endif
                    #if( kXcpDaqTimestampSize > DAQ_TIMESTAMP_WORD )
                        dtop->b[i+2u] = (vuint8)(t>>16);
                        dtop->b[i+3u] = (vuint8)(t>>24);
                    #endif
                #endif
            #else
                        *(XcpDaqTimestampType*)&dtop->b[i]=ApplXcpGetTimestamp();
            #endif /* XCP_DISABLE_UNALIGNED_MEM_ACCESS */
                        i += XcpDaqTimestampSize;

        #endif /* XCP_ENABLE_DAQ_HDR_ODT_DAQ */
                    }

        #if !defined ( XCP_ENABLE_DAQ_TIMESTAMP_FIXED )
                }
        #endif

    #endif /* XCP_ENABLE_DAQ_TIMESTAMP */

                /* Sample data */
                /* This is the inner loop, optimize here */
                e = DaqListOdtFirstEntry(odt);
                if (OdtEntrySize(e)==0)
                {
                    goto next_odt; /* PRQA S 2001 */ /* MD_Xcp_2001 */
                }
                el = DaqListOdtLastEntry(odt);
                d = (vuint8*)&dtop->b[i];
                
                while (e<=el)
                {
                    n = OdtEntrySize(e);
                    if (n == 0)
                    {
                        break;
                    }
    #if defined ( C_COMP_COSMIC_MCS12X_MSCAN12 )
                    src = (vuint32)OdtEntryAddr(e);
                    for ( ; n > 0; n-- )
                    {
                        *d = XCP_READ_BYTE_FROM_ADDR( src );
                        d++;
                        src++;
                    }
    #else
                    XcpMemCpy((DAQBYTEPTR)d, OdtEntryAddr(e), n);
                    d = &d[n]; 
        /* d += n; Suppress warning for MISRA rule 101 (pointer arithmetic) */
    #endif
                    e++;
                }
                dtop->l = (vuint8)(d-(&dtop->b[0]) ); 
                /* PRQA S 0488 */ /* MD_Xcp_0488 */
                XCP_ASSERT(dtop->l<=kXcpMaxDTO);

                /* Queue or transmit the DTO */
    #if defined ( XCP_ENABLE_SEND_QUEUE )
        #if defined ( XCP_SEND_QUEUE_SAMPLE_ODT )
        /* No action yet */
        #else
                if ( (xcp.SendStatus & (vuint8)XCP_SEND_PENDING) != 0 )
                {
                    xcp.QueueLen++;
                }
                else
                {
                    xcp.SendStatus |= (vuint8)XCP_DTO_PENDING;
                    XcpSendDto(dtop);
                }
        #endif
    #else
                XcpSendDto(&dto);
    #endif /* XCP_ENABLE_SEND_QUEUE */
                next_odt:

                ApplXcpInterruptEnable();

            } /* odt */

    #if defined ( XCP_ENABLE_SEND_QUEUE ) && defined ( XCP_SEND_QUEUE_SAMPLE_ODT )
            (void)XcpSendDtoFromQueue();
    #endif


    #if defined ( XCP_ENABLE_DAQ_PRESCALER )
        }
    #endif
  
    } /* daq */

    END_PROFILE(4); /* Timingtest */
    return status; 
    /* PRQA S 2006 4 */ /* MD_MSR_14.7 */
    /* PRQA S 6010 3 */ /* MD_MSR_STPTH */
    /* PRQA S 6030 2 */ /* MD_MSR_STCYC */
    /* PRQA S 6080 1 */ /* MD_MSR_STMIF */
}

#endif /* XCP_ENABLE_DAQ */


/******************************************************************************/
/* Background Processing                                                      */
/* Used for Checksum Calculation                                              */
/******************************************************************************/

#if defined ( XCP_ENABLE_CHECKSUM )
/* Table for CCITT checksum calculation */
    #if ( kXcpChecksumMethod == XCP_CHECKSUM_TYPE_CRC16CCITT )



    V_MEMROM0 const vuint16 CRC16CCITTtab[256] = 
    { /* PRQA S 3408 */ /* MD_Xcp_3408 */
        #if defined ( XCP_ENABLE_CRC16CCITT_REFLECTED )
    0x0000,0x1189,0x2312,0x329B,0x4624,0x57AD,0x6536,0x74BF,
    0x8C48,0x9DC1,0xAF5A,0xBED3,0xCA6C,0xDBE5,0xE97E,0xF8F7,
    0x1081,0x0108,0x3393,0x221A,0x56A5,0x472C,0x75B7,0x643E,
    0x9CC9,0x8D40,0xBFDB,0xAE52,0xDAED,0xCB64,0xF9FF,0xE876,
    0x2102,0x308B,0x0210,0x1399,0x6726,0x76AF,0x4434,0x55BD,
    0xAD4A,0xBCC3,0x8E58,0x9FD1,0xEB6E,0xFAE7,0xC87C,0xD9F5,
    0x3183,0x200A,0x1291,0x0318,0x77A7,0x662E,0x54B5,0x453C,
    0xBDCB,0xAC42,0x9ED9,0x8F50,0xFBEF,0xEA66,0xD8FD,0xC974,
    0x4204,0x538D,0x6116,0x709F,0x0420,0x15A9,0x2732,0x36BB,
    0xCE4C,0xDFC5,0xED5E,0xFCD7,0x8868,0x99E1,0xAB7A,0xBAF3,
    0x5285,0x430C,0x7197,0x601E,0x14A1,0x0528,0x37B3,0x263A,
    0xDECD,0xCF44,0xFDDF,0xEC56,0x98E9,0x8960,0xBBFB,0xAA72,
    0x6306,0x728F,0x4014,0x519D,0x2522,0x34AB,0x0630,0x17B9,
    0xEF4E,0xFEC7,0xCC5C,0xDDD5,0xA96A,0xB8E3,0x8A78,0x9BF1,
    0x7387,0x620E,0x5095,0x411C,0x35A3,0x242A,0x16B1,0x0738,
    0xFFCF,0xEE46,0xDCDD,0xCD54,0xB9EB,0xA862,0x9AF9,0x8B70,
    0x8408,0x9581,0xA71A,0xB693,0xC22C,0xD3A5,0xE13E,0xF0B7,
    0x0840,0x19C9,0x2B52,0x3ADB,0x4E64,0x5FED,0x6D76,0x7CFF,
    0x9489,0x8500,0xB79B,0xA612,0xD2AD,0xC324,0xF1BF,0xE036,
    0x18C1,0x0948,0x3BD3,0x2A5A,0x5EE5,0x4F6C,0x7DF7,0x6C7E,
    0xA50A,0xB483,0x8618,0x9791,0xE32E,0xF2A7,0xC03C,0xD1B5,
    0x2942,0x38CB,0x0A50,0x1BD9,0x6F66,0x7EEF,0x4C74,0x5DFD,
    0xB58B,0xA402,0x9699,0x8710,0xF3AF,0xE226,0xD0BD,0xC134,
    0x39C3,0x284A,0x1AD1,0x0B58,0x7FE7,0x6E6E,0x5CF5,0x4D7C,
    0xC60C,0xD785,0xE51E,0xF497,0x8028,0x91A1,0xA33A,0xB2B3,
    0x4A44,0x5BCD,0x6956,0x78DF,0x0C60,0x1DE9,0x2F72,0x3EFB,
    0xD68D,0xC704,0xF59F,0xE416,0x90A9,0x8120,0xB3BB,0xA232,
    0x5AC5,0x4B4C,0x79D7,0x685E,0x1CE1,0x0D68,0x3FF3,0x2E7A,
    0xE70E,0xF687,0xC41C,0xD595,0xA12A,0xB0A3,0x8238,0x93B1,
    0x6B46,0x7ACF,0x4854,0x59DD,0x2D62,0x3CEB,0x0E70,0x1FF9,
    0xF78F,0xE606,0xD49D,0xC514,0xB1AB,0xA022,0x92B9,0x8330,
    0x7BC7,0x6A4E,0x58D5,0x495C,0x3DE3,0x2C6A,0x1EF1,0x0F78
        #else
    0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7u,
    0x8108,0x9129,0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1efu,
    0x1231,0x0210,0x3273,0x2252,0x52b5,0x4294,0x72f7,0x62d6u,
    0x9339,0x8318,0xb37b,0xa35a,0xd3bd,0xc39c,0xf3ff,0xe3deu,
    0x2462,0x3443,0x0420,0x1401,0x64e6,0x74c7,0x44a4,0x5485u,
    0xa56a,0xb54b,0x8528,0x9509,0xe5ee,0xf5cf,0xc5ac,0xd58du,
    0x3653,0x2672,0x1611,0x0630,0x76d7,0x66f6,0x5695,0x46b4u,
    0xb75b,0xa77a,0x9719,0x8738,0xf7df,0xe7fe,0xd79d,0xc7bcu,
    0x48c4,0x58e5,0x6886,0x78a7,0x0840,0x1861,0x2802,0x3823u,
    0xc9cc,0xd9ed,0xe98e,0xf9af,0x8948,0x9969,0xa90a,0xb92bu,
    0x5af5,0x4ad4,0x7ab7,0x6a96,0x1a71,0x0a50,0x3a33,0x2a12u,
    0xdbfd,0xcbdc,0xfbbf,0xeb9e,0x9b79,0x8b58,0xbb3b,0xab1au,
    0x6ca6,0x7c87,0x4ce4,0x5cc5,0x2c22,0x3c03,0x0c60,0x1c41u,
    0xedae,0xfd8f,0xcdec,0xddcd,0xad2a,0xbd0b,0x8d68,0x9d49u,
    0x7e97,0x6eb6,0x5ed5,0x4ef4,0x3e13,0x2e32,0x1e51,0x0e70u,
    0xff9f,0xefbe,0xdfdd,0xcffc,0xbf1b,0xaf3a,0x9f59,0x8f78u,
    0x9188,0x81a9,0xb1ca,0xa1eb,0xd10c,0xc12d,0xf14e,0xe16fu,
    0x1080,0x00a1,0x30c2,0x20e3,0x5004,0x4025,0x7046,0x6067u,
    0x83b9,0x9398,0xa3fb,0xb3da,0xc33d,0xd31c,0xe37f,0xf35eu,
    0x02b1,0x1290,0x22f3,0x32d2,0x4235,0x5214,0x6277,0x7256u,
    0xb5ea,0xa5cb,0x95a8,0x8589,0xf56e,0xe54f,0xd52c,0xc50du,
    0x34e2,0x24c3,0x14a0,0x0481,0x7466,0x6447,0x5424,0x4405u,
    0xa7db,0xb7fa,0x8799,0x97b8,0xe75f,0xf77e,0xc71d,0xd73cu,
    0x26d3,0x36f2,0x0691,0x16b0,0x6657,0x7676,0x4615,0x5634u,
    0xd94c,0xc96d,0xf90e,0xe92f,0x99c8,0x89e9,0xb98a,0xa9abu,
    0x5844,0x4865,0x7806,0x6827,0x18c0,0x08e1,0x3882,0x28a3u,
    0xcb7d,0xdb5c,0xeb3f,0xfb1e,0x8bf9,0x9bd8,0xabbb,0xbb9au,
    0x4a75,0x5a54,0x6a37,0x7a16,0x0af1,0x1ad0,0x2ab3,0x3a92u,
    0xfd2e,0xed0f,0xdd6c,0xcd4d,0xbdaa,0xad8b,0x9de8,0x8dc9u,
    0x7c26,0x6c07,0x5c64,0x4c45,0x3ca2,0x2c83,0x1ce0,0x0cc1u,
    0xef1f,0xff3e,0xcf5d,0xdf7c,0xaf9b,0xbfba,0x8fd9,0x9ff8u,
    0x6e17,0x7e36,0x4e55,0x5e74,0x2e93,0x3eb2,0x0ed1,0x1ef0u
        #endif /* defined ( XCP_ENABLE_CRC16CCITT_REFLECTED ) */
    };

    #endif /* ( kXcpChecksumMethod == XCP_CHECKSUM_TYPE_CRC16CCITT ) */

    #if !defined ( kXcpChecksumBlockSize )
        #define kXcpChecksumBlockSize 256
    #endif

#endif /* defined ( XCP_ENABLE_CHECKSUM ) */

/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  perform background calculation of checksum
  * @note   CALLED BY:  application
  * @param  void
  * @retval 0 : background calculation finished
|           1 : background calculation still pending
 ******************************************************************************/
vuint8 XcpBackground( void )
{

    BEGIN_PROFILE(3); /* Timingtest */

    /* Activation control */
    XcpPlCheckControlStateRet((vuint8)0u)

    /* STORE_DAQ_REQ or CLEAR_DAQ_REQ pending */
    #if defined ( XCP_ENABLE_DAQ )
    #endif /* XCP_ENABLE_DAQ */

    /* XCP checksum calculation */
    #if defined ( XCP_ENABLE_CHECKSUM ) && !defined ( XCP_ENABLE_CUSTOM_CRC )

    /*
        Checksum algorithm is not defined by the standard
        Type is defined by tXcpChecksumSumType, the maximum blocksize is 64K
    */

    /* Checksum calculation in progress */
    if ( (xcp.CheckSumSize) != (tXcpChecksumSumType)0u )
    {

        vuint16 n;

        if (xcp.CheckSumSize<=(kXcpChecksumBlockSize-1))
        {
            n = xcp.CheckSumSize;
            xcp.CheckSumSize = 0;
        }
        else
        {
            n = (vuint16)kXcpChecksumBlockSize;
            xcp.CheckSumSize -= kXcpChecksumBlockSize;
        }


        do
        {
            tXcpChecksumAddType data;
            data = XCP_READ_CHECKSUMVALUE_FROM_ADDR( xcp.Mta );

    #if ( kXcpChecksumMethod == XCP_CHECKSUM_TYPE_CRC16CCITT )

        #if defined ( XCP_ENABLE_CRC16CCITT_REFLECTED )
        /* CRC16 CCITT Reflected: Refin = true and refout = true. */
        xcp.CheckSum = CRC16CCITTtab[((vuint8)(xcp.CheckSum&0xFF)) ^ data] ^ \
            ((vuint8)((xcp.CheckSum>>8)&0xFF));
        #else
        /* CRC16 CCITT */
        xcp.CheckSum = CRC16CCITTtab[((vuint8)((xcp.CheckSum >> 8)&0xFF)) ^ \
            data] ^ (xcp.CheckSum << 8);
        #endif

        xcp.Mta++; /* PRQA S 0489 */ /* MD_Xcp_0489 */
        n--;

    #else

        /* Checksum calculation method: XCP_ADD_xx */
        xcp.CheckSum += (tXcpChecksumSumType)(data);
        #if ( kXcpChecksumMethod == XCP_CHECKSUM_TYPE_ADD11 )
        xcp.CheckSum &= 0xFF;
        #endif
        xcp.Mta += (vuint8)tXcpChecksumAddSize; 
        /* PRQA S 0488 */ /* MD_Xcp_0488 */
        n       -= (vuint8)tXcpChecksumAddSize;

    #endif
    }
    while (n!=0);


        /* Checksum calculation finished ? */
        if ( (xcp.CheckSumSize) != 0 )
        {
            END_PROFILE(3); /* Timingtest */
            return (vuint8)1u; /* Still pending */
        }

        CRM_BUILD_CHECKSUM_TYPE = kXcpChecksumMethod;
        /* PRQA S 3757 1 */ /* MD_Xcp_3757 */
        CRM_BUILD_CHECKSUM_RESULT_WRITE(xcp.CheckSum); 
        /* PRQA S 3109 */ /* MD_MSR_14.3 */
        xcp.CrmLen = CRM_BUILD_CHECKSUM_LEN;

    #if defined ( XCP_ENABLE_TESTMODE )
        if ( gDebugLevel != 0) 
        {
        ApplXcpPrint("<- 0xFF checksum=%08Xh, type=%02Xh\n", \
            CRM_BUILD_CHECKSUM_RESULT,CRM_BUILD_CHECKSUM_TYPE);
        }
    #endif

        XcpSendCrm();

    } /* xcp.CheckSumSize */
#endif /* XCP_ENABLE_CHECKSUM && !XCP_ENABLE_CUSTOM_CRC */

    /* Application specific background ground loop. */ 
    ApplXcpBackground();

    END_PROFILE(3); /* Timingtest */

    return (vuint8)0u;
    /* PRQA S 2006 2 */ /* MD_MSR_14.7 */
    /* PRQA S 6010 1 */ /* MD_MSR_STPTH */
}


/******************************************************************************/
/* Command Processor                                                          */
/******************************************************************************/


/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  If the XCP slave is connected to a XCP master a call of this 
  * function discontinues the connection (transition to disconnected state).
  * If the XCP slave is not connected this function performs no action.
  * @note   PRECONDITIONS：XCP is initialized and in connected state. 
  * CALLED BY:  XcpCommand
  * @param  void
  * @retval void
 ******************************************************************************/
void XcpDisconnect( void )
{
    /* Activation control */
    XcpPlCheckControlState()

    xcp.SessionStatus &= (SessionStatusType)(~SS_CONNECTED & 0xFFu);

    #if defined ( XCP_ENABLE_DAQ )
    XcpStopAllDaq();
    #endif
    #if defined ( XCP_ENABLE_SEED_KEY )
    /* Lock all resources */
    xcp.ProtectionStatus = (vuint8)RM_CAL_PAG|RM_DAQ|RM_PGM|RM_STIM;
    #endif

  /* PRQA S 2006 1 */ /* MD_MSR_14.7 */
}

/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  XcpCommand
  * @note   BY:   XcpSendCallBack, XCP Transport Layer
  * @param  pCmd : data of received CTO message.
  * @retval void
 ******************************************************************************/
void XcpCommand( const vuint32* pCommand )
{
    const tXcpCto* pCmd = (const tXcpCto*) pCommand;
    /* PRQA S 0310 */ /* MD_Xcp_0310 */
    vuint8 err;

    /* Activation control */
    XcpPlCheckControlState()

    /* XCP Data Stimulation Handler */
    /* XCP Command Handler */

    BEGIN_PROFILE(1); /* Timingtest */

    /* CONNECT */
    if (CRO_CMD==CC_CONNECT)
    {

        /* Prepare the default response */
        /* ESCAN00023570 */
        CRM_CMD = 0xFF; /* No Error */
        xcp.CrmLen = 1; /* Length = 1 */

    #if defined ( XCP_ENABLE_TESTMODE )
        if ( gDebugLevel != 0) 
        {
            ApplXcpPrint("\n-> CONNECT mode=%u\n",CRO_CONNECT_MODE);
        }
    #endif

        /* DPRAM */
        /* DPRAM Client */
        
        /* Reset DAQ */
        /* Do not reset DAQ if in resume mode */ 
    #if defined ( XCP_ENABLE_DAQ )
        if ( (xcp.SessionStatus & (SessionStatusType)SS_RESUME) == 0 )
        {
            XcpFreeDaq();
        #if defined ( XCP_ENABLE_SEND_QUEUE )
            xcp.SendStatus = 0; /* Clear all transmission flags */
        #endif
        }
    #endif /* XCP_ENABLE_DAQ */

    #if defined ( XCP_ENABLE_SEED_KEY )
        /* Lock all resources. */
        xcp.ProtectionStatus = (vuint8)RM_CAL_PAG|RM_DAQ|RM_PGM|RM_STIM;
    #endif

    /* Reset Session Status */
        xcp.SessionStatus = (SessionStatusType)SS_CONNECTED;

        xcp.CrmLen = CRM_CONNECT_LEN;

        /*Versions of the XCP Protocol Layer and Transport Layer Specifications.*/
        CRM_CONNECT_TRANSPORT_VERSION = (vuint8) \
            ( (vuint16)XCP_TRANSPORT_LAYER_VERSION >> 8 );
        CRM_CONNECT_PROTOCOL_VERSION =  (vuint8)( (vuint16)XCP_VERSION >> 8 );

        CRM_CONNECT_MAX_CTO_SIZE = kXcpMaxCTO;
        CRM_CONNECT_MAX_DTO_SIZE_WRITE(kXcpMaxDTO);
        /* PRQA S 3109 */ /* MD_MSR_14.3 */

    #if defined ( XCP_ENABLE_CALIBRATION_PAGE )
        CRM_CONNECT_RESOURCE = RM_CAL_PAG;            /* Calibration */
    #else
        CRM_CONNECT_RESOURCE = 0x00;                  /* Reset resource mask */
    #endif
    #if defined ( XCP_ENABLE_DAQ )
        CRM_CONNECT_RESOURCE |= (vuint8)RM_DAQ;       /* Data Acquisition */
    #endif

        CRM_CONNECT_COMM_BASIC = 0;
    #if defined ( XCP_ENABLE_COMM_MODE_INFO )
        CRM_CONNECT_COMM_BASIC |= (vuint8)CMB_OPTIONAL;
    #endif
    #if defined ( XCP_CPUTYPE_BIGENDIAN )
        CRM_CONNECT_COMM_BASIC |= (vuint8)PI_MOTOROLA;
    #endif

        XCP_PRINT(("<- 0xFF version=%02Xh/%02Xh, maxcro=%02Xh, \
            maxdto=%02Xh, resource=%02X, mode=%02X\n",
        CRM_CONNECT_PROTOCOL_VERSION,
        CRM_CONNECT_TRANSPORT_VERSION,
        CRM_CONNECT_MAX_CTO_SIZE,
        CRM_CONNECT_MAX_DTO_SIZE,
        CRM_CONNECT_RESOURCE,
        CRM_CONNECT_COMM_BASIC));

        goto positive_response; /* PRQA S 2001 */ /* MD_Xcp_2001 */

    }

    /* Handle other commands only if connected */
    else /* CC_CONNECT */
    {
        if ( (xcp.SessionStatus & (SessionStatusType)SS_CONNECTED) != 0 )
        {
    /* Ignore commands if the previous command sequence has not been completed */
    #if defined ( XCP_ENABLE_SEND_QUEUE )
        if ( (xcp.SendStatus & (vuint8)(XCP_CRM_PENDING|XCP_CRM_REQUEST)) != 0 )
        {
            xcp.SessionStatus |= (SessionStatusType)SS_ERROR; 
            END_PROFILE(1); /* Timingtest */

            /* No response */
            return;
        }
    #endif

    #if defined ( XCP_ENABLE_GET_SESSION_STATUS_API )
        xcp.SessionStatus |= (SessionStatusType)SS_POLLING;
    #endif

        /* Prepare the default response */
        /* ESCAN00023570 */
        CRM_CMD = 0xFF; /* No Error */
        xcp.CrmLen = 1; /* Length = 1 */

        switch (CRO_CMD)
        {

            case CC_SYNC:
            {
    /* Always return a negative response with the error code ERR_CMD_SYNCH. */
                xcp.CrmLen = CRM_SYNCH_LEN;
                CRM_CMD    = PID_ERR;
                CRM_ERR    = CRC_CMD_SYNCH;
           
    #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0) 
                {
                ApplXcpPrint("-> SYNC\n");
                ApplXcpPrint("<- 0xFE 0x00\n");
                }
    #endif
            }
            break;


    #if defined ( XCP_ENABLE_COMM_MODE_INFO )
            case CC_GET_COMM_MODE_INFO:
            {
                xcp.CrmLen = CRM_GET_COMM_MODE_INFO_LEN;
        /* Transmit the version of the XCP Protocol Layer implementation.    */
        /* The higher nibble is the main version, the lower the sub version. */
        /* The lower nibble overflows, if the sub version is greater than 15.*/
                CRM_GET_COMM_MODE_INFO_DRIVER_VERSION = (vuint8) \
                    ( ((CP_XCP_VERSION & 0x0F00) >> 4) | (CP_XCP_VERSION & 0x000F));
                CRM_GET_COMM_MODE_INFO_COMM_OPTIONAL = 0;
                CRM_GET_COMM_MODE_INFO_QUEUE_SIZE = 0;
                CRM_GET_COMM_MODE_INFO_MAX_BS = 0;
                CRM_GET_COMM_MODE_INFO_MIN_ST = 0;

    #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("-> GET_COMM_MODE_INFO\n");
                    ApplXcpPrint("<- 0xFF \n");
                }
    #endif

            }
          break;
    #endif /* XCP_ENABLE_COMM_MODE_INFO */


            case CC_DISCONNECT:
            {
                xcp.CrmLen = CRM_DISCONNECT_LEN;
                XcpDisconnect();

    #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("-> DISCONNECT\n");
                    ApplXcpPrint("<- 0xFF\n");
                }
    #endif
            }
            break;

                       
    #if defined ( kXcpStationIdLength ) || defined ( XCP_ENABLE_GET_ID_GENERIC )
            case CC_GET_ID:
            {
                xcp.CrmLen = CRM_GET_ID_LEN;
                CRM_GET_ID_MODE = 0;
                CRM_GET_ID_LENGTH_WRITE(0x00);
                /* PRQA S 3109 */ /* MD_MSR_14.3 */

        #if defined ( kXcpStationIdLength )
                if ( CRO_GET_ID_TYPE == IDT_ASAM_NAME )   /* Type = ASAM MC2 */
                {
                    CRM_GET_ID_LENGTH_WRITE((vuint32)kXcpStationIdLength); 
                    /* PRQA S 3109 */ /* MD_MSR_14.3 */
                    XcpSetMta( ApplXcpGetPointer(0xFF, (vuint32) \
                        (&kXcpStationId[0])), 0xFF); 
                    /* PRQA S 0306 */ /* MD_Xcp_0306 */
                } else
        #endif
                {
        #if defined ( XCP_ENABLE_GET_ID_GENERIC )
                    MTABYTEPTR pData;
                /* PRQA S 3757 1 */ /* MD_Xcp_3757 */
                    CRM_GET_ID_LENGTH_WRITE(ApplXcpGetIdData \
                        (&pData, CRO_GET_ID_TYPE));
                        /* PRQA S 3109 */ /* MD_MSR_14.3 */
                    XcpSetMta(ApplXcpGetPointer(0x00, (vuint32)pData), 0xFF);
                    /* PRQA S 0306 */ /* MD_Xcp_0306 */
        #endif
              }

        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("-> GET_ID type=%u\n",CRO_GET_ID_TYPE);
                    ApplXcpPrint("<- 0xFF mode=%u,len=%u\n", \
                        CRM_GET_ID_MODE,CRM_GET_ID_LENGTH);
                }
        #endif
            }
            break;
    #endif


            case CC_GET_STATUS: 
            {
                xcp.CrmLen = CRM_GET_STATUS_LEN;
                CRM_GET_STATUS_STATUS = (vuint8)xcp.SessionStatus;
    #if defined ( XCP_ENABLE_SEED_KEY )
        /* Return current resource protection status. If a bit is one, 
            the associated resource is locked. */
                CRM_GET_STATUS_PROTECTION = xcp.ProtectionStatus;
    #else
                CRM_GET_STATUS_PROTECTION = 0;
    #endif

                /* Session configuration ID not available. */
                CRM_GET_STATUS_CONFIG_ID_WRITE(0x00);

    #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("-> GET_STATUS\n");
                    ApplXcpPrint("<- 0xFF sessionstatus=%02Xh, \
                        protectionstatus=%02X\n",CRM_GET_STATUS_STATUS, \
                        CRM_GET_STATUS_PROTECTION);
                }
    #endif
            }
            break;


    #if defined ( XCP_ENABLE_SEED_KEY )

            case CC_GET_SEED:
            {
        /* Only seeds with a maximum length of MAX_CTO-2 are supported so far. */

        /* Check whether the first part or a remaining part of the seed is 
        requested. */
                if (CRO_GET_SEED_MODE == 0x01)
                {
                    /* Remaining parts of seeds are not supported so far. */
                    error(CRC_OUT_OF_RANGE) /* PRQA S 2001 */ /* MD_Xcp_2001 */
                }
                else
                {
        #if defined ( XCP_ENABLE_PARAMETER_CHECK )
                    /* Only one resource may be requested at one time. */
                    switch (CRO_GET_SEED_RESOURCE)
                    {
                    case RM_CAL_PAG: 
                        break;
                    case RM_PGM:
                        break;
                    case RM_DAQ:
                        break;
                    case RM_STIM:
                        break;
                    default:
                        error(CRC_OUT_OF_RANGE) /* PRQA S 2001 */ /* MD_Xcp_2001 */
                    }

        #endif
                    if ((xcp.ProtectionStatus & CRO_GET_SEED_RESOURCE) != 0)   /* locked */
                    {
                        vuint8 seedLength;
                        seedLength = ApplXcpGetSeed(CRO_GET_SEED_RESOURCE, \
                            CRM_GET_SEED_DATA); 
                    if (seedLength > (vuint8)(kXcpMaxCTO-2))
                    {
                    /* A maximum seed length of MAX_CTO-2 is supported. */
                        error(CRC_OUT_OF_RANGE) 
                        /* PRQA S 2001 */ /* MD_Xcp_2001 */
                    }
                        CRM_GET_SEED_LENGTH = seedLength;
                    }
                    else                       /* Unlocked */
                    {
                    /* return 0 if the resource is unprotected. */
                        CRM_GET_SEED_LENGTH = 0;
                    }
                xcp.CrmLen = (vuint8)CRM_GET_SEED_LEN;
                }

        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("-> GET_SEED resource=%02Xh\n", \
                        CRO_GET_SEED_RESOURCE);
                    ApplXcpPrint("<- 0xFF length=%02Xh, \
                        seed=%02X%02X%02X%02X%02X%02X\n",CRM_GET_SEED_LENGTH, \
                         CRM_GET_SEED_DATA[0],CRM_GET_SEED_DATA[1], \
                         CRM_GET_SEED_DATA[2],CRM_GET_SEED_DATA[3], \
                         CRM_GET_SEED_DATA[4],CRM_GET_SEED_DATA[5]);
                }
        #endif
            }
            break;

            case CC_UNLOCK:
            {
                vuint8 resource;
              
        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("-> UNLOCK key=%02X%02X%02X%02X%02X%02X\n", \
                    CRO_UNLOCK_KEY[0],CRO_UNLOCK_KEY[1],CRO_UNLOCK_KEY[2], \
                    CRO_UNLOCK_KEY[3],CRO_UNLOCK_KEY[4],CRO_UNLOCK_KEY[5]);
                }
        #endif

        /* Only keys with a maximum length of MAX_CTO-2 are supported so far. */
                if (CRO_UNLOCK_LENGTH > (vuint8)(kXcpMaxCTO-2))
                {
                    error(CRC_SEQUENCE) /* PRQA S 2001 */ /* MD_Xcp_2001 */
                }
                else
                {
                    resource = ApplXcpUnlock(CRO_UNLOCK_KEY, CRO_UNLOCK_LENGTH);
                    if ( resource == (vuint8)0x00u )
                    {
                        /* Key wrong ! */
                        /* Send ERR_ACCESS_LOCKED and go to disconnected state. */
                        XcpDisconnect();
                        error(CRC_ACCESS_LOCKED) 
                        /* PRQA S 2001 */ /* MD_Xcp_2001 */
                    }
                    else
                    {
                /* unlock (reset) the appropriate resource protection mask bit. */
                        xcp.ProtectionStatus &= (vuint8)~resource;

                        /* ..and return the current resource protection status. */
                        CRM_UNLOCK_PROTECTION = xcp.ProtectionStatus;
                        xcp.CrmLen = CRM_UNLOCK_LEN;
                    }
                }

        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("<- 0xFF\n");
                }
        #endif
            }
            break;

    #endif /* XCP_ENABLE_SEED_KEY */


    #if defined ( XCP_ENABLE_CALIBRATION_PAGE )

            case CC_SET_CAL_PAGE:
            {
        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("-> SET_CAL_PAGE segment=%u, \
                        page =%u,mode=%02Xh\n",CRO_SET_CAL_PAGE_SEGMENT, \
                        CRO_SET_CAL_PAGE_PAGE,CRO_SET_CAL_PAGE_MODE);
                }
        #endif

                check_error( ApplXcpSetCalPage(CRO_SET_CAL_PAGE_SEGMENT, \
                    CRO_SET_CAL_PAGE_PAGE,CRO_SET_CAL_PAGE_MODE) )
                /* PRQA S 2001 */ /* MD_Xcp_2001 */

        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("<- 0xFF\n");
                }
        #endif
            }
            break;

            case CC_GET_CAL_PAGE:
            {

        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("-> GET_CAL_PAGE Segment=%u, Mode=%u\n", \
                        CRO_GET_CAL_PAGE_SEGMENT,CRO_GET_CAL_PAGE_MODE);
                }
        #endif

                xcp.CrmLen = CRM_GET_CAL_PAGE_LEN;
                CRM_GET_CAL_PAGE_PAGE = ApplXcpGetCalPage( \
                    CRO_GET_CAL_PAGE_SEGMENT,CRO_GET_CAL_PAGE_MODE);

        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("<- 0xFF page=%u\n",CRM_GET_CAL_PAGE_PAGE);
                }
        #endif
            }
            break;

    #endif /* XCP_ENABLE_CALIBRATION_PAGE */


    #if defined ( XCP_ENABLE_PAGE_INFO )
            /* Paging Information optional */
            case CC_GET_PAG_PROCESSOR_INFO:
            {
        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("-> CC_GET_PAG_PROCESSOR_INFO");
                }
        #endif
       
                xcp.CrmLen = CRM_GET_PAG_PROCESSOR_INFO_LEN;
            
                CRM_GET_PAG_PROCESSOR_INFO_MAX_SEGMENT = (vuint8)kXcpMaxSegment;
        #if defined ( XCP_ENABLE_PAGE_FREEZE )
                CRM_GET_PAG_PROCESSOR_INFO_PROPERTIES  = 1; 
                /* FREEZE_SUPPORTED = 1 */
        #else
                CRM_GET_PAG_PROCESSOR_INFO_PROPERTIES  = 0;
                /* FREEZE_SUPPORTED = 0 */
        #endif
        
        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("<- 0xFF maxsegment=%u\n",(vuint8)kXcpMaxSegment);
                }
        #endif
        
            }
            break;  


            case CC_GET_SEGMENT_INFO:
            {
        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("-> CC_GET_SEGMENT_INFO");
                }
        #endif

                if (CRO_GET_SEGMENT_INFO_NUMBER >= (vuint8)kXcpMaxSegment)
                {
                    error(CRC_OUT_OF_RANGE) /* PRQA S 2001 */ /* MD_Xcp_2001 */
                }

                xcp.CrmLen = CRM_GET_SEGMENT_INFO_LEN;
                /* Get standard info for a segment.
                    SEGMENT_INFO and MAPPING_INDEX are don't care
                */
                if (CRO_GET_SEGMENT_INFO_MODE == 1)
                {
                    CRM_GET_SEGMENT_INFO_MAX_PAGES         = 2;
                    CRM_GET_SEGMENT_INFO_ADDRESS_EXTENSION = 0;
                    CRM_GET_SEGMENT_INFO_MAX_MAPPING       = 0;
                    CRM_GET_SEGMENT_INFO_COMPRESSION       = 0;
                    CRM_GET_SEGMENT_INFO_ENCRYPTION        = 0;
                }
              /* Get basic address info for a segment:
                   SEGMENT_INFO contains address range information and 
                   MAPPING_INDEX is don't care
                 Get address mapping for a segment:
                   SEGMENT_INFO contains address range information and 
                   MAPPING_INDEX indicates the range MAPPING_INFO belongs to
              */
                if ( (CRO_GET_SEGMENT_INFO_MODE == 0) || \
                    (CRO_GET_SEGMENT_INFO_MODE == 2) )
                {
                    CRM_GET_SEGMENT_INFO_MAPPING_INFO = 0;
                }

        #if defined ( XCP_ENABLE_TESTMODE )
                if (gDebugLevel && CRO_GET_SEGMENT_INFO_MODE == 1)
                {
                    ApplXcpPrint("<- 0xFF Mode=%u, Maxpages=%u, AddrExt=%u, \
                        Mappingmax=%u, Compression=%u, Encryption=%u\n", \
                        CRO_GET_SEGMENT_INFO_MODE,CRM_GET_SEGMENT_INFO_MAX_PAGES, \
                        CRM_GET_SEGMENT_INFO_ADDRESS_EXTENSION, \
                        CRM_GET_SEGMENT_INFO_MAX_MAPPING, \
                        CRM_GET_SEGMENT_INFO_COMPRESSION, \
                        CRM_GET_SEGMENT_INFO_ENCRYPTION);
                }
                if (gDebugLevel && CRO_GET_SEGMENT_INFO_MODE == 0 || \
                    CRO_GET_SEGMENT_INFO_MODE == 2) 
                {
                    ApplXcpPrint("<- 0xFF Mode=%u, MappingInfo=%u\n", \
                        CRO_GET_SEGMENT_INFO_MODE,CRM_GET_SEGMENT_INFO_MAPPING_INFO);
                }
        #endif
            }
            break;

            case CC_GET_PAGE_INFO:
            {
        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("-> CC_GET_PAGE_INFO");
                }
        #endif

                if ( ( CRO_GET_PAGE_INFO_SEGMENT_NUMBER > \
                    ((vuint8)(kXcpMaxSegment-1)&0xFFu) ) || \
                    ( CRO_GET_PAGE_INFO_PAGE_NUMBER > \
                    ((vuint8)(kXcpMaxPages-1)&0xFFu) ) )
                {
                    error(CRC_OUT_OF_RANGE) /* PRQA S 2001 */ /* MD_Xcp_2001 */
                }
            
                xcp.CrmLen = CRM_GET_PAGE_INFO_LEN;
                CRM_GET_PAGE_INFO_PROPERTIES = 0;
               
        #if defined ( XCP_ENABLE_TESTMODE )
              if ( gDebugLevel != 0)
              {
                ApplXcpPrint("<- 0xFF properties=%u\n",CRM_GET_PAGE_INFO_PROPERTIES);
              }
        #endif
            }
            break;
    #endif /* defined ( XCP_BASIC_VERSION_COMMENT ) */

          /* Paging freeze mode support */
    #if defined ( XCP_ENABLE_PAGE_FREEZE )
            case CC_SET_SEGMENT_MODE:
            {
        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("-> CC_SET_SEGMENT_MODE segment=%u, \
                        mode=%u\n",CRO_SET_SEGMENT_MODE_SEGMENT, \
                        CRO_SET_SEGMENT_MODE_MODE);
                }
        #endif

                if (CRO_SET_SEGMENT_MODE_SEGMENT > \
                    ((vuint8)(kXcpMaxSegment-1)&0xFFu))
                {
                    error(CRC_OUT_OF_RANGE) /* PRQA S 2001 */ /* MD_Xcp_2001 */
                }

                /* inform application about Set Segment Mode command */
                ApplXcpSetFreezeMode(CRO_SET_SEGMENT_MODE_SEGMENT, \
                    CRO_SET_SEGMENT_MODE_MODE);
                /* CRO_SET_SEGMENT_MODE_MODE;*/
                xcp.CrmLen = CRM_SET_SEGMENT_MODE_LEN;
   
        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("<- 0xFF\n");
                }
        #endif
            }
            break;

            case CC_GET_SEGMENT_MODE:
            {
        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("-> CC_GET_SEGMENT_MODE");
                }
        #endif

                if (CRO_GET_SEGMENT_MODE_SEGMENT > ((vuint8)(kXcpMaxSegment-1)&0xFFu))
                {
                    error(CRC_OUT_OF_RANGE) /* PRQA S 2001 */ /* MD_Xcp_2001 */
                }

                /* request current freeze mode information from application */
                CRM_GET_SEGMENT_MODE_MODE = ApplXcpGetFreezeMode \
                    (CRO_GET_SEGMENT_MODE_SEGMENT);

                xcp.CrmLen = CRM_GET_SEGMENT_MODE_LEN;

        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("<- 0xFF mode=%u\n",CRM_GET_SEGMENT_MODE_MODE);
                }
        #endif
            }
            break;
    #endif /* XCP_ENABLE_PAGE_FREEZE */

          /* Copy cal page support */
    #if defined ( XCP_ENABLE_PAGE_COPY )
            case CC_COPY_CAL_PAGE:
            {
        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("-> CC_COPY_CAL_PAGE");
                }
        #endif

                xcp.CrmLen = CRM_COPY_CAL_PAGE_LEN;

                err = ApplXcpCopyCalPage(CRO_COPY_CAL_PAGE_SRC_SEGMENT, \
                    CRO_COPY_CAL_PAGE_SRC_PAGE,CRO_COPY_CAL_PAGE_DEST_SEGMENT, \
                    CRO_COPY_CAL_PAGE_DEST_PAGE);

                if (err==(vuint8)XCP_CMD_PENDING) 
                {
                    goto no_response; /* PRQA S 2001 */ /* MD_Xcp_2001 */
                }

                check_error( err ) /* PRQA S 2001 */ /* MD_Xcp_2001 */

        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("<- 0xFF\n");
                }
        #endif
            }
            break;
    #endif /* XCP_ENABLE_PAGE_COPY */


            case CC_SET_MTA:
            {
    #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("-> SET_MTA addr=%08Xh, \
                        addrext=%02Xh\n",CRO_SET_MTA_ADDR,CRO_SET_MTA_EXT);
                }
    #endif
                XcpSetMta(ApplXcpGetPointer(CRO_SET_MTA_EXT,CRO_SET_MTA_ADDR), \
                    CRO_SET_MTA_EXT);


    #if defined ( XCP_ENABLE_TESTMODE )
              if ( gDebugLevel != 0)
              {
                ApplXcpPrint("<- 0xFF\n");
              }
    #endif
            }
            break;


            case CC_DOWNLOAD: /* PRQA S 2003 */ /* MD_Xcp_2003 */
            {
    #if defined ( XCP_ENABLE_CALIBRATION )
                vuint8 size;

        #if defined ( XCP_ENABLE_TESTMODE )
                if (gDebugLevel && (CRO_CMD != CC_DOWNLOAD_NEXT))
                {
                    vuint16 i;
                    ApplXcpPrint("-> DOWNLOAD size=%u, data=",CRO_DOWNLOAD_SIZE);
                    for (i=0;(i<CRO_DOWNLOAD_SIZE) && (i<CRO_DOWNLOAD_MAX_SIZE);i++)
                    {
                        ApplXcpPrint("%02X ",CRO_DOWNLOAD_DATA[i]);
                    }
                        ApplXcpPrint("\n");
                }
        #endif

                CheckResourceProtection( RM_CAL_PAG )
                /* PRQA S 2001 */ /* MD_Xcp_2001 */

                size = CRO_DOWNLOAD_SIZE;
                if (size>CRO_DOWNLOAD_MAX_SIZE)
                {
                    error(CRC_OUT_OF_RANGE) /* PRQA S 2001 */ /* MD_Xcp_2001 */
                }

                err = XcpWriteMta(size,CRO_DOWNLOAD_DATA);
                if (err==(vuint8)XCP_CMD_PENDING) 
                {
                    goto no_response; /* PRQA S 2001 */ /* MD_Xcp_2001 */
                }
                if (err==(vuint8)XCP_CMD_DENIED)
                {
                    error(CRC_WRITE_PROTECTED) /* PRQA S 2001 */ /* MD_Xcp_2001 */
                }
                if (err==(vuint8)XCP_CMD_SYNTAX)
                {
                    error(CRC_CMD_SYNTAX) /* PRQA S 2001 */ /* MD_Xcp_2001 */
                }

                    
        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("<- 0xFF\n");
                }
        #endif
    #else
                error(CRC_CMD_UNKNOWN) /* PRQA S 2001 */ /* MD_Xcp_2001 */
    #endif /* !defined ( XCP_ENABLE_CALIBRATION ) */
                              
            }
            break;

            case CC_DOWNLOAD_MAX:
            {
    #if defined ( XCP_ENABLE_CALIBRATION )
        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    vuint16 i;
                    ApplXcpPrint("DOWNLOAD_MAX data=");
                    for (i=0;i<CRO_DOWNLOAD_MAX_MAX_SIZE;i++)
                    {
                        ApplXcpPrint("%02X ",CRO_DOWNLOAD_MAX_DATA[i]);
                    }
                    ApplXcpPrint("\n");
                }
        #endif

              CheckResourceProtection( RM_CAL_PAG ) 
              /* PRQA S 2001 */ /* MD_Xcp_2001 */

                err = XcpWriteMta(CRO_DOWNLOAD_MAX_MAX_SIZE,CRO_DOWNLOAD_MAX_DATA);
                if (err==(vuint8)XCP_CMD_PENDING)
                {
                    return;
                }
                if (err==(vuint8)XCP_CMD_DENIED)
                {
                    error(CRC_WRITE_PROTECTED) /* PRQA S 2001 */ /* MD_Xcp_2001 */
                }
                if (err==(vuint8)XCP_CMD_SYNTAX)
                {
                    error(CRC_CMD_SYNTAX) /* PRQA S 2001 */ /* MD_Xcp_2001 */
                }

        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("<- 0xFF\n");
                }
        #endif
    #else
                error(CRC_ACCESS_DENIED) /* PRQA S 2001 */ /* MD_Xcp_2001 */
    #endif /* !defined ( XCP_ENABLE_CALIBRATION ) */
            }
            break;



            case CC_UPLOAD:
            {
                vuint8 size = CRO_UPLOAD_SIZE;

    #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("-> UPLOAD size=%u\n",size);
                }
    #endif

                if ( size > (vuint8)CRM_UPLOAD_MAX_SIZE )
                {
                    error(CRC_OUT_OF_RANGE) /* PRQA S 2001 */ /* MD_Xcp_2001 */
                }
                err = XcpReadMta(size,CRM_UPLOAD_DATA);
                xcp.CrmLen = (vuint8)((CRM_UPLOAD_LEN+size)&0xFFu);
                if (err==(vuint8)XCP_CMD_PENDING)
                {
                    goto no_response; /* PRQA S 2001 */ /* MD_Xcp_2001 */
                }
                if (err==(vuint8)XCP_CMD_DENIED)
                {
                    error(CRC_ACCESS_DENIED) /* PRQA S 2001 */ /* MD_Xcp_2001 */
                }

    #if defined ( XCP_ENABLE_TESTMODE )
              if ( gDebugLevel != 0)
              {
                vuint16 i;
                ApplXcpPrint("<- 0xFF data=");
                for (i=0;i<size;i++) 
                {
                  ApplXcpPrint("%02X ",CRM_UPLOAD_DATA[i]);
                }
                ApplXcpPrint("\n");
              }
    #endif
            }
            break;

            case CC_SHORT_UPLOAD:
            {
    #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("-> SHORT_UPLOAD addr=%08Xh, addrext=%02Xh, \
                        size=%u\n",CRO_SHORT_UPLOAD_ADDR,CRO_SHORT_UPLOAD_EXT, \
                        CRO_SHORT_UPLOAD_SIZE);
                }
    #endif

    #if defined ( XCP_ENABLE_PARAMETER_CHECK )
                if (CRO_SHORT_UPLOAD_SIZE > (vuint8)CRM_SHORT_UPLOAD_MAX_SIZE)
                {
                    error(CRC_OUT_OF_RANGE) /* PRQA S 2001 */ /* MD_Xcp_2001 */
                }
    #endif
                XcpSetMta(ApplXcpGetPointer(CRO_SHORT_UPLOAD_EXT, \
                    CRO_SHORT_UPLOAD_ADDR),CRO_SHORT_UPLOAD_EXT);
                err = XcpReadMta(CRO_SHORT_UPLOAD_SIZE,CRM_SHORT_UPLOAD_DATA);
                xcp.CrmLen = (vuint8)((CRM_SHORT_UPLOAD_LEN+CRO_SHORT_UPLOAD_SIZE) \
                    &0xFFu);
                if (err==(vuint8)XCP_CMD_PENDING)
                {
                    goto no_response; 
                    /* ESCAN00014775 */ /* PRQA S 2001 */ /* MD_Xcp_2001 */
                }
                if (err==(vuint8)XCP_CMD_DENIED)
                {
                    error(CRC_ACCESS_DENIED) /* PRQA S 2001 */ /* MD_Xcp_2001 */
                }

    #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    vuint16 i;
                    ApplXcpPrint("<- 0xFF data=");
                    for (i=0; i < (vuint16)CRO_SHORT_UPLOAD_SIZE; i++)
                    {
                        ApplXcpPrint("%02X ",CRM_SHORT_UPLOAD_DATA[i]);
                    }
                        ApplXcpPrint("\n");
                }
    #endif
            }
            break;


    #if defined ( XCP_ENABLE_CHECKSUM )

            case CC_BUILD_CHECKSUM: /* Build Checksum */
            {
                vuint32 s;

        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("-> BUILD_CHECKSUM size=%u\n", \
                        CRO_BUILD_CHECKSUM_SIZE);
                }
        #endif
                s = CRO_BUILD_CHECKSUM_SIZE;

        #if !defined ( XCP_ENABLE_CUSTOM_CRC )
              /* Initialization of checksum calculation. */
            #if ( kXcpChecksumMethod == XCP_CHECKSUM_TYPE_CRC16CCITT ) || \
                ( kXcpChecksumMethod == XCP_CHECKSUM_TYPE_CRC32 )
              xcp.CheckSum = (tXcpChecksumSumType)0xFFFFFFFFu;
            #else
                xcp.CheckSum = (tXcpChecksumSumType)0u;
            #endif
            /* The blocksize is limited to WORD length. */
            /* If the blocksize exceeds the allowed maximum transmit negative response. */
                if ( (s & (vuint32)0xffff0000u) != (vuint32)0u )
                {
                    CRM_BUILD_CHECKSUM_RESULT_WRITE(0xFFFFu); 
                    /* Range, max. 64K-1 */ /* PRQA S 3109 */ /* MD_MSR_14.3 */
                    err = CRC_OUT_OF_RANGE;
                    xcp.CrmLen = CRM_BUILD_CHECKSUM_LEN;
                    /* Response length is arbitrary */
                    goto negative_response1; /* PRQA S 2001 */ /* MD_Xcp_2001 */
                } 
                else
        #endif
                {
              
        #if defined ( XCP_ENABLE_PARAMETER_CHECK )
        /* Parameter check whether the block size is modulo 2 for ADD_22, ADD_24 */
        /* and ADD_44 (modulo 4).                                                */
            #if ( kXcpChecksumMethod == XCP_CHECKSUM_TYPE_ADD22 ) || \
                ( kXcpChecksumMethod == XCP_CHECKSUM_TYPE_ADD24 ) || \
                ( kXcpChecksumMethod == XCP_CHECKSUM_TYPE_ADD44 )
                    if ( (s % (vuint32)sizeof(tXcpChecksumAddType)) != \
                        (vuint32)0u )
                    {
                        error(CRC_OUT_OF_RANGE) /* PRQA S 2001 */ /* MD_Xcp_2001 */
                    }
                    else
            #endif
        #endif
                    {

        #if defined ( XCP_ENABLE_CUSTOM_CRC )
                        err = ApplXcpCalculateChecksum( xcp.Mta, xcp.Crm.b, s );
                        xcp.Mta += s;
                        if (err==(vuint8)XCP_CMD_PENDING)
                        {
                            goto no_response; /* PRQA S 2001 */ /* MD_Xcp_2001 */
                        }
                        if (err!=(vuint8)XCP_CMD_OK)
                        {
                            error(CRC_CMD_UNKNOWN)
                            /* PRQA S 2001 */ /* MD_Xcp_2001 */
                        }
        #else
                        xcp.CheckSumSize = (vuint16)s;
                    /* Checksum calculation will be performed by XcpBackground() */
                        goto no_response; /* PRQA S 2001 */ /* MD_Xcp_2001 */
        #endif
                    }
                }
            }
        #if defined ( XCP_ENABLE_CUSTOM_CRC )
            break;
        #endif

    #endif /* XCP_ENABLE_CHECKSUM */


    #if defined ( XCP_ENABLE_DAQ )

        #if defined ( XCP_ENABLE_DAQ_PROCESSOR_INFO )

            case CC_GET_DAQ_PROCESSOR_INFO: /* PRQA S 2003 */ /* MD_Xcp_2003 */
            {
            #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("-> GET_DAQ_PROCESSOR_INFO\n");
                }
            #endif

                xcp.CrmLen = CRM_GET_DAQ_PROCESSOR_INFO_LEN;
                CRM_GET_DAQ_PROCESSOR_INFO_MIN_DAQ = 0;
                /* PRQA S 3757 1 */ /* MD_Xcp_3757 */
                CRM_GET_DAQ_PROCESSOR_INFO_MAX_DAQ_WRITE(xcp.Daq.DaqCount);
               /* dynamic or static */ /* PRQA S 3109 */ /* MD_MSR_14.3 */
            #if defined ( kXcpMaxEvent )
                CRM_GET_DAQ_PROCESSOR_INFO_MAX_EVENT_WRITE(kXcpMaxEvent);
                /* PRQA S 3109 */ /* MD_MSR_14.3 */
            #else
                CRM_GET_DAQ_PROCESSOR_INFO_MAX_EVENT_WRITE(0x00); /* Unknown */
            #endif
            #if defined ( XCP_ENABLE_DAQ_HDR_ODT_DAQ )
            /* DTO identification field type: Relative ODT number, absolute list 
            number (BYTE) */
                CRM_GET_DAQ_PROCESSOR_INFO_DAQ_KEY_BYTE = (vuint8)DAQ_HDR_ODT_DAQB;
            #else
                /* DTO identification field type: Absolute ODT number */
                CRM_GET_DAQ_PROCESSOR_INFO_DAQ_KEY_BYTE = (vuint8)DAQ_HDR_PID;
            #endif
                CRM_GET_DAQ_PROCESSOR_INFO_PROPERTIES = (vuint8) \
                    ( DAQ_PROPERTY_CONFIG_TYPE
            #if defined ( XCP_ENABLE_DAQ_TIMESTAMP )
                    | DAQ_PROPERTY_TIMESTAMP
            #endif
            #if defined ( XCP_ENABLE_DAQ_PRESCALER )
                    | DAQ_PROPERTY_PRESCALER
            #endif
            #if defined ( XCP_ENABLE_DAQ_OVERRUN_INDICATION ) 
            /* DAQ_PROPERTY_OVERLOAD_INDICATION */
                    | DAQ_OVERLOAD_INDICATION_PID
            #endif
                );

            #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("<- 0xFF\n");
                }
            #endif
            }
            break;

        #endif /* XCP_ENABLE_DAQ_PROCESSOR_INFO */

        #if defined ( XCP_ENABLE_DAQ_RESOLUTION_INFO )

            case CC_GET_DAQ_RESOLUTION_INFO: /* PRQA S 2003 */ /* MD_Xcp_2003 */
            {
            #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0) 
                {
                    ApplXcpPrint("-> GET_DAQ_RESOLUTION_INFO\n");
                }
            #endif

                xcp.CrmLen = CRM_GET_DAQ_RESOLUTION_INFO_LEN;
                CRM_GET_DAQ_RESOLUTION_INFO_GRANULARITY_DAQ = 1;
                CRM_GET_DAQ_RESOLUTION_INFO_GRANULARITY_STIM = 1;
                CRM_GET_DAQ_RESOLUTION_INFO_MAX_SIZE_DAQ  = \
                    (vuint8)XCP_MAX_ODT_ENTRY_SIZE;
                CRM_GET_DAQ_RESOLUTION_INFO_MAX_SIZE_STIM = \
                    (vuint8)XCP_MAX_ODT_ENTRY_SIZE;
            #if defined ( XCP_ENABLE_DAQ_TIMESTAMP )
                CRM_GET_DAQ_RESOLUTION_INFO_TIMESTAMP_MODE = \
                    kXcpDaqTimestampUnit | (vuint8)kXcpDaqTimestampSize
                #if defined ( XCP_ENABLE_DAQ_TIMESTAMP_FIXED )
                    | DAQ_TIMESTAMP_FIXED
                #endif
                    ;
                CRM_GET_DAQ_RESOLUTION_INFO_TIMESTAMP_TICKS_WRITE \
                    (kXcpDaqTimestampTicksPerUnit); 
                /* BCD coded */ /* PRQA S 3109 */ /* MD_MSR_14.3 */
            #else
                CRM_GET_DAQ_RESOLUTION_INFO_TIMESTAMP_MODE = 0;
                CRM_GET_DAQ_RESOLUTION_INFO_TIMESTAMP_TICKS_WRITE(0x00);
            #endif /* XCP_ENABLE_DAQ_TIMESTAMP */
          
            #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("<- 0xFF , mode=%02Xh, , ticks=%02Xh\n", \
                        CRM_GET_DAQ_RESOLUTION_INFO_TIMESTAMP_MODE, \
                        CRM_GET_DAQ_RESOLUTION_INFO_TIMESTAMP_TICKS);
                }
            #endif
            }
            break;
        #endif /* XCP_ENABLE_DAQ_RESOLUTION_INFO */

        #if defined ( XCP_ENABLE_DAQ_EVENT_INFO ) && defined ( kXcpMaxEvent )
            case CC_GET_DAQ_EVENT_INFO: /* PRQA S 2003 */ /* MD_Xcp_2003 */
            {
                vuint8 event = (vuint8)CRO_GET_DAQ_EVENT_INFO_EVENT;

            #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("-> GET_DAQ_EVENT_INFO event=%u\n",event);
                }
            #endif

            #if defined ( XCP_ENABLE_PARAMETER_CHECK )
                if (event >= (vuint8)kXcpMaxEvent )
                {
                    error(CRC_OUT_OF_RANGE) /* PRQA S 2001 */ /* MD_Xcp_2001 */
                } 
            #endif

                xcp.CrmLen = CRM_GET_DAQ_EVENT_INFO_LEN;
                CRM_GET_DAQ_EVENT_INFO_PROPERTIES = kXcpEventDirection[event];
                CRM_GET_DAQ_EVENT_INFO_MAX_DAQ_LIST = 1; 
                /* Only one DAQ-List available per event channel */
                CRM_GET_DAQ_EVENT_INFO_NAME_LENGTH = kXcpEventNameLength[event];
                CRM_GET_DAQ_EVENT_INFO_TIME_CYCLE = kXcpEventCycle[event];
            #if defined ( XCP_ENABLE_CANAPE_5_5_X_SUPPORT )
                CRM_GET_DAQ_EVENT_INFO_TIME_UNIT = kXcpEventUnit[event];
            #else
                CRM_GET_DAQ_EVENT_INFO_TIME_UNIT = (vuint8)(kXcpEventUnit[event]>>4); 
                /* ESCAN00090639 */
            #endif
                CRM_GET_DAQ_EVENT_INFO_PRIORITY = 0;
                /* Event channel prioritization is not supported. */
                XcpSetMta( ApplXcpGetPointer( 0xFF, (vuint32)kXcpEventName[event]),\
                 0xFF ); /* PRQA S 0306 */ /* MD_Xcp_0306 */

            #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("<- 0xFF name=%s, unit=%u, cycle=%u\n", \
                        kXcpEventName[event],CRM_GET_DAQ_EVENT_INFO_TIME_UNIT, \
                        CRM_GET_DAQ_EVENT_INFO_TIME_CYCLE);
                }
            #endif
            }
            break;
        #endif /* XCP_ENABLE_DAQ_EVENT_INFO && kXcpMaxEvent */


            case CC_FREE_DAQ:
            {
        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("-> FREE_DAQ\n");
                }
        #endif

                CheckResourceProtection( RM_DAQ ) /* PRQA S 2001 */ /* MD_Xcp_2001 */

                XcpFreeDaq();

        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("<- 0xFF\n");
                }
        #endif
            }
            break;

            case CC_ALLOC_DAQ:
            {
                vuint8 count = (vuint8)CRO_ALLOC_DAQ_COUNT;
 
        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("-> ALLOC_DAQ count=%u\n",count);
                }
        #endif

                check_error( XcpAllocDaq(count) ) /* PRQA S 2001 */ /* MD_Xcp_2001 */

        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("<- 0xFF\n");
                }
        #endif
            }
            break;

            case CC_ALLOC_ODT:
            {
                vuint8 daq = (vuint8)CRO_ALLOC_ODT_DAQ;
                vuint8 count = CRO_ALLOC_ODT_COUNT;
 
        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("-> ALLOC_ODT daq=%u, count=%u\n",daq,count);
                }
        #endif

        #if defined ( XCP_ENABLE_PARAMETER_CHECK )
                if (daq>=xcp.Daq.DaqCount)
                {
                    error(CRC_OUT_OF_RANGE) /* PRQA S 2001 */ /* MD_Xcp_2001 */
                }
        #endif

                check_error( XcpAllocOdt(daq, count) ) 
                /* PRQA S 2001 */ /* MD_Xcp_2001 */

        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("<- 0xFF\n");
                }
        #endif
            }
            break;

            case CC_ALLOC_ODT_ENTRY:
            {
                vuint8 daq = (vuint8)CRO_ALLOC_ODT_ENTRY_DAQ;
                vuint8 odt = CRO_ALLOC_ODT_ENTRY_ODT;
                vuint8 count = CRO_ALLOC_ODT_ENTRY_COUNT;
 
        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("-> ALLOC_ODT_ENTRY daq=%u, odt=%u, \
                        count=%u\n",daq,odt,count);
                }
        #endif

        #if defined ( XCP_ENABLE_PARAMETER_CHECK )
                if ( (daq>=xcp.Daq.DaqCount) || (odt>=(vuint8)DaqListOdtCount(daq)) )
                {
                    error(CRC_OUT_OF_RANGE) /* PRQA S 2001 */ /* MD_Xcp_2001 */
                }
        #endif

                check_error( XcpAllocOdtEntry(daq, odt, count) ) 
                /* PRQA S 2001 */ /* MD_Xcp_2001 */

        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("<- 0xFF\n");
                }
        #endif
            }
            break;

            case CC_GET_DAQ_LIST_MODE:
            {
                vuint8 daq = (vuint8)CRO_GET_DAQ_LIST_MODE_DAQ;

        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("-> GET_DAQ_LIST_MODE daq=%u\n",daq);
                }
        #endif

        #if defined ( XCP_ENABLE_PARAMETER_CHECK )
                if (daq>=xcp.Daq.DaqCount)
                {
                    error(CRC_OUT_OF_RANGE) /* PRQA S 2001 */ /* MD_Xcp_2001 */
                }
        #endif
          
                xcp.CrmLen = CRM_GET_DAQ_LIST_MODE_LEN;
                CRM_GET_DAQ_LIST_MODE_MODE = DaqListFlags(daq);
        #if defined ( XCP_ENABLE_DAQ_PRESCALER )
                CRM_GET_DAQ_LIST_MODE_PRESCALER = DaqListPrescaler(daq);
        #else
                CRM_GET_DAQ_LIST_MODE_PRESCALER = 1;
        #endif
        #if defined ( kXcpMaxEvent )
                CRM_GET_DAQ_LIST_MODE_EVENTCHANNEL_WRITE(0); 
                /* #### Lookup in EventDaq[] */ /* PRQA S 3109 */ /* MD_MSR_14.3 */
        #else
              CRM_GET_DAQ_LIST_MODE_EVENTCHANNEL_WRITE(DaqListEventChannel(daq));
        #endif
              CRM_GET_DAQ_LIST_MODE_PRIORITY = 0; 
              /* DAQ-list prioritization is not supported. */

        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("<- 0xFF mode=%02X, prescaler=%u, eventChannel=%u, \
                        priority=%u, /*maxOdtEntrySize=%u*/  \n", \
                        CRM_GET_DAQ_LIST_MODE_MODE,\
                        CRM_GET_DAQ_LIST_MODE_PRESCALER, \
                        CRM_GET_DAQ_LIST_MODE_EVENTCHANNEL, \
                        CRM_GET_DAQ_LIST_MODE_PRIORITY);
                }
        #endif
            }
            break;

            case CC_SET_DAQ_LIST_MODE:
            {
                vuint8 daq = (vuint8)CRO_SET_DAQ_LIST_MODE_DAQ;
        #if defined ( XCP_ENABLE_TESTMODE ) || \
            defined ( XCP_ENABLE_DAQ_PRESCALER ) || \
            ( !defined ( XCP_ENABLE_DAQ_PRESCALER ) && \
                defined ( XCP_ENABLE_PARAMETER_CHECK ) )
                vuint8 xcpPrescaler = CRO_SET_DAQ_LIST_MODE_PRESCALER;
        #endif
                vuint8 event = (vuint8)(CRO_SET_DAQ_LIST_MODE_EVENTCHANNEL&0xFFu);
 
        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("-> SET_DAQ_LIST_MODE daq=%u, mode=%02Xh, \
                        prescaler=%u, eventchannel=%u\n", \
                        daq,CRO_SET_DAQ_LIST_MODE_MODE,xcpPrescaler,event);
                }
        #endif

        #if defined ( XCP_ENABLE_PARAMETER_CHECK )
                if (daq>=xcp.Daq.DaqCount)
                {
                    error(CRC_OUT_OF_RANGE) /* PRQA S 2001 */ /* MD_Xcp_2001 */
                }
            #if defined ( kXcpMaxEvent )
                if (event >= (vuint8)kXcpMaxEvent)
                {
                    error(CRC_OUT_OF_RANGE) /* PRQA S 2001 */ /* MD_Xcp_2001 */
                }
            #endif
            #if !defined ( XCP_ENABLE_DAQ_PRESCALER )
                if (xcpPrescaler!=1)
                {
                    error(CRC_OUT_OF_RANGE) /* PRQA S 2001 */ /* MD_Xcp_2001 */
                }
            #endif
                if (CRO_SET_DAQ_LIST_MODE_PRIORITY!=0)
                /* Priorization is not supported */
                {
                    error(CRC_OUT_OF_RANGE) /* PRQA S 2001 */ /* MD_Xcp_2001 */
                } 
        #endif

        #if defined ( XCP_ENABLE_DAQ_PRESCALER )
                if (xcpPrescaler==0)
                {
                    xcpPrescaler = 1;
                }
                DaqListPrescaler(daq) = xcpPrescaler;
        #endif
        #if defined ( kXcpMaxEvent ) && ! defined ( XCP_ENABLE_DAQ_PRESCALER )
                xcp.Daq.EventDaq[event] = daq;
        #else
                DaqListEventChannel(daq) = event;
        #endif
                DaqListFlags(daq) = CRO_SET_DAQ_LIST_MODE_MODE;


        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("<- 0xFF\n");
                }
        #endif
            break;
            }


            case CC_SET_DAQ_PTR:
            {
                vuint8 daq = (vuint8) (CRO_SET_DAQ_PTR_DAQ&0xFFu);
                vuint8 odt = CRO_SET_DAQ_PTR_ODT;
                vuint8 idx = CRO_SET_DAQ_PTR_IDX;
                tXcpOdtIdx odt0 = (tXcpOdtIdx)(DaqListFirstOdt(daq)+odt); 
                /* Absolute odt number */

        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("-> SET_DAQ_PTR daq=%u,odt=%u,idx=%u\n", \
                        daq,odt,idx);
                }
        #endif

        #if defined ( XCP_ENABLE_PARAMETER_CHECK )
                if ( (daq>=xcp.Daq.DaqCount) || (odt>=(vuint8)DaqListOdtCount(daq))\
                    || (idx>=(vuint8)DaqListOdtEntryCount(odt0)) )
                {
                    error(CRC_OUT_OF_RANGE) /* PRQA S 2001 */ /* MD_Xcp_2001 */
                } 
        #endif

                xcp.CrmLen = CRM_SET_DAQ_PTR_LEN;
                xcp.DaqListPtr = DaqListOdtFirstEntry(odt0)+idx;
        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("<- 0xFF \n");
                }
        #endif
            }
            break;

            case CC_WRITE_DAQ: /* Write DAQ entry */
            {
                DAQBYTEPTR addr;
        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("-> WRITE_DAQ size=%u,addr=%08Xh,%02Xh\n", \
                        CRO_WRITE_DAQ_SIZE,CRO_WRITE_DAQ_ADDR,CRO_WRITE_DAQ_EXT);
                }
        #endif

                CheckResourceProtection( RM_DAQ ) /* PRQA S 2001 */ /* MD_Xcp_2001 */

        #if defined ( XCP_ENABLE_PARAMETER_CHECK )
                if ( ((vuint8)CRO_WRITE_DAQ_SIZE==(vuint8)0u ) || \
                    (CRO_WRITE_DAQ_SIZE > (vuint8)XCP_MAX_ODT_ENTRY_SIZE) )
                {
                    error(CRC_OUT_OF_RANGE) /* PRQA S 2001 */ /* MD_Xcp_2001 */
                }
                if ( (0u == xcp.Daq.DaqCount) || (0u == xcp.Daq.OdtCount) || \
                    (0u == xcp.Daq.OdtEntryCount) )
                {
                    error(CRC_DAQ_CONDIF) /* PRQA S 2001 */ /* MD_Xcp_2001 */
                } 
        #endif
                addr = (DAQBYTEPTR)ApplXcpGetPointer \
                    (CRO_WRITE_DAQ_EXT,CRO_WRITE_DAQ_ADDR);

                xcp.CrmLen = CRM_WRITE_DAQ_LEN;
                OdtEntrySize(xcp.DaqListPtr) = CRO_WRITE_DAQ_SIZE;
                OdtEntryAddr(xcp.DaqListPtr) = addr;

                xcp.DaqListPtr++; /* Autoincrement */

        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("<- 0xFF\n");
                }
        #endif
            }
            break;

            case CC_START_STOP_DAQ_LIST:
            {
                vuint8 daq = (vuint8)(CRO_START_STOP_DAQ&0xFFu);

                CheckResourceProtection( RM_DAQ ) /* PRQA S 2001 */ /* MD_Xcp_2001 */

        #if defined ( XCP_ENABLE_PARAMETER_CHECK )
                if (daq>=xcp.Daq.DaqCount)
                {
                    error(CRC_OUT_OF_RANGE) /* PRQA S 2001 */ /* MD_Xcp_2001 */
                }
        #endif

                if ( (CRO_START_STOP_MODE==1 ) || (CRO_START_STOP_MODE==2) )
                {
                    DaqListFlags(daq) |= (vuint8)DAQ_FLAG_SELECTED;
        #if defined ( XCP_ENABLE_TESTMODE )
                    if ( gDebugLevel != 0)
                    {
                        XcpPrintDaqList((vuint8)(CRO_START_STOP_DAQ)&0xFFu);
                        ApplXcpPrint("-> START_STOP mode=%02Xh, daq=%u\n", \
                            CRO_START_STOP_MODE,CRO_START_STOP_DAQ);
                    }
        #endif
                    if ( CRO_START_STOP_MODE == (vuint8)1u )
                    {
                        XcpStartDaq(daq);
                    }
                    xcp.CrmLen = CRM_START_STOP_LEN;
                    CRM_START_STOP_FIRST_PID = DaqListFirstPid(daq);
                }
                else
                {
                    XcpStopDaq(daq);
        #if defined ( XCP_ENABLE_TESTMODE )
                    if ( gDebugLevel != 0)
                    {
                        ApplXcpPrint("-> START_STOP mode=%02Xh\n",CRO_START_STOP_MODE);
                    }
        #endif
                }

        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("<- 0xFF\n");
                }
        #endif
            }
            break;

            case CC_START_STOP_SYNCH:
            {
        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("-> CC_START_STOP_SYNCH mode=%02Xh\n", \
                        CRO_START_STOP_MODE);
                }
        #endif

                CheckResourceProtection( RM_DAQ ) /* PRQA S 2001 */ /* MD_Xcp_2001 */

                if( (0 == xcp.Daq.DaqCount) || (0 == xcp.Daq.OdtCount) || \
                    (0 == xcp.Daq.OdtEntryCount) )
                {
                    error(CRC_DAQ_CONDIF) /* PRQA S 2001 */ /* MD_Xcp_2001 */
                }

                if (CRO_START_STOP_MODE==2) /* stop selected */
                {
                    XcpStopAllSelectedDaq();
                } 
                else
                {
                    if (CRO_START_STOP_MODE==1) /* start selected */
                    {
                    XcpStartAllSelectedDaq();
                    }
                    else
                    {
                        /* CRO_START_STOP_MODE==0 : stop all */
                        XcpStopAllDaq();
                    }
                }
        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("<- 0xFF\n");
                }
        #endif
            }
            break;

        #if defined ( XCP_ENABLE_DAQ_TIMESTAMP )
            case CC_GET_DAQ_CLOCK:
            {
                xcp.CrmLen = CRM_GET_DAQ_CLOCK_LEN;
                /* PRQA S 3757 1 */ /* MD_Xcp_3757 */
                CRM_GET_DAQ_CLOCK_TIME_WRITE((vuint32)ApplXcpGetTimestamp()); 
                /* PRQA S 3109 */ /* MD_MSR_14.3 */

            #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("-> GET_DAQ_CLOCK\n");
                    ApplXcpPrint("<- 0xFF time=%04Xh\n",CRM_GET_DAQ_CLOCK_TIME);
                }
            #endif
            }
            break;
        #endif

    #endif /* XCP_ENABLE_DAQ */


        /* Flash Programming Kernel Download */

        /* Flash Programming direct and Kernel */

        /* Flash Programming  */


    #if defined ( XCP_ENABLE_USER_COMMAND ) 
            case CC_USER_CMD:
            {

        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("-> CC_USER_CMD cmd=%u\n", CRO_BYTE(1));
                }
        #endif

                {

        #if defined ( XCP_ENABLE_USER_COMMAND )
                err = ApplXcpUserService( (const BYTEPTR) &CRO_BYTE(0) );

                if (err==(vuint8)XCP_CMD_PENDING)
                {
                    goto no_response;  /* PRQA S 2001 */ /* MD_Xcp_2001 */
                }
                    if (err==(vuint8)XCP_CMD_SYNTAX)
                {
                    error(CRC_CMD_SYNTAX) /* PRQA S 2001 */ /* MD_Xcp_2001 */
                }
        #endif
                }

        #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("<- 0xFF\n");
                }
        #endif

            }
            break;
    #endif


            default: /* unknown */
            {
    #if defined ( XCP_ENABLE_TESTMODE )
                if ( gDebugLevel != 0)
                {
                    ApplXcpPrint("-> UNKNOWN COMMAND %02X\n", CRO_CMD);
                }
    #endif
                error(CRC_CMD_UNKNOWN) /* PRQA S 2001 */ /* MD_Xcp_2001 */
            }

        } /* switch */

        goto positive_response; /* PRQA S 2001 */ /* MD_Xcp_2001 */
        }

        /* Not connected */
        else
        {
            goto no_response; /* PRQA S 2001 */ /* MD_Xcp_2001 */
        }
    }     /* CC_CONNECT */

    negative_response:
    xcp.CrmLen = 2;

    #if defined ( XCP_ENABLE_CHECKSUM ) 
    negative_response1:
    #endif
    CRM_CMD = (vuint8)PID_ERR;
    CRM_ERR = (vuint8)err;
    #if defined ( XCP_ENABLE_TESTMODE )
    if ( gDebugLevel != 0) 
    {
        ApplXcpPrint("<- 0xFE error=%02Xh\n",err);
    }
    #endif
 
    positive_response:
    XcpSendCrm();

    no_response:
    END_PROFILE(1); /* Timingtest */
    return;
    /* PRQA S 2006 5 */ /* MD_MSR_14.7 */
    /* PRQA S 6010 4 */ /* MD_MSR_STPTH */
    /* PRQA S 6030 3 */ /* MD_MSR_STCYC */
    /* PRQA S 6050 2 */ /* MD_MSR_STCAL */
    /* PRQA S 6080 1 */ /* MD_MSR_STMIF */
}


/******************************************************************************/
/* Send notification callback                                                 */
/******************************************************************************/

/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  Notifies the XCP Protocol Layer about the successful transmission 
  * of a XCP packet.
  * @note   CALLED BY:  XCP Transport Layer
  * @param  void
  * @retval 0 : if the XCP Protocol Layer is idle (no transmit messages 
  *         are pending)
 ******************************************************************************/
vuint8 XcpSendCallBack( void )
{
    BEGIN_PROFILE(2); /* Timingtest */

    /* Activation control */
    XcpPlCheckControlStateRet((vuint8)1u)

    #if defined ( XCP_ENABLE_DAQ ) && defined ( XCP_ENABLE_SEND_QUEUE )

    /* Clear all pending flags */
    /* A pending flag indicates that ApplXcpSend() is in progress */
    xcp.SendStatus &= (vuint8)(~XCP_SEND_PENDING & 0xFFu);

    /* Now check if there is another transmit request */
 
    /* Send a RES or ERR (CRM) message */
    if ( (xcp.SendStatus & (vuint8)XCP_CRM_REQUEST) != 0 )
    {
        xcp.SendStatus &= (vuint8)(~XCP_CRM_REQUEST & 0xFFu);
        XcpSendCrm();
        END_PROFILE(2); /* Timingtest */
        return (vuint8)0x01u;
    }

    /* Send a EV or SERV message */
    #if defined ( XCP_ENABLE_SEND_EVENT ) || defined ( XCP_ENABLE_SERV_TEXT )
    if ( (xcp.SendStatus & (vuint8)XCP_EVT_REQUEST) != 0 )
    {
        xcp.SendStatus &= (vuint8)(~XCP_EVT_REQUEST & 0xFFu);
        XcpSendEv();
        END_PROFILE(2); /* Timingtest */
        return (vuint8)0x01u;
    }
    #endif

    /* Send a DAQ message from the queue or from the buffer */
    if ( (xcp.SessionStatus & (SessionStatusType)SS_DAQ) != 0 )
    {
        if ( XcpSendDtoFromQueue() != 0 )
        {
            END_PROFILE(2); /* Timingtest */
            return (vuint8)0x01u;
        }
    }
    #endif /* XCP_ENABLE_DAQ && XCP_ENABLE_SEND_QUEUE */

    /* Continue a pending block upload command */

    END_PROFILE(2); /* Timingtest */
    return (vuint8)0x00u;
    /* PRQA S 2006 1 */ /* MD_MSR_14.7 */
}


/******************************************************************************/
/* Initialization / de-initialization                                         */
/******************************************************************************/

/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  Initialization of the XCP Protocol Layer Application specific 
  *     initialization ( e.g. Vector XCP on CAN Transport Layer )
  * @note   PRECONDITIONS：the data link layer has to be initialized. 
  * CALLED BY:  application
  * @param  void
  * @retval void
 ******************************************************************************/
void XcpInit( void )
{
    #if defined ( XCP_ENABLE_TESTMODE )
    gDebugLevel = 1;
    #endif

    /* Application specific initialization function. */
    ApplXcpInit();

    /* Initialize all XCP variables to zero */
    XcpMemClr((BYTEPTR)&xcp,(vuint16)sizeof(xcp)); 
    /* PRQA S 0310 */ /* MD_Xcp_0310_ByteCopy */

    /* We set 8 bytes as default for CAN */
    XcpSetActiveTl(8, 8, 0);

    /* Initialize the RAM interface */

    /* Initialize the session status (ESCAN00013899) */
    xcp.SessionStatus = (SessionStatusType)0u;

    #if defined ( XCP_ENABLE_SEND_QUEUE)
    /* Initialize the transmit queue (ESCAN00013899) */
    xcp.SendStatus = (vuint8)0u;
    #endif

    /* Resume DAQ */
    #if defined ( XCP_ENABLE_DAQ )
    #endif /* XCP_ENABLE_DAQ */
}

/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  De-initialization of the XCP Protocol Layer.
  * @note   CALLED BY:  application PRECONDITIONS: The XCP Protocol Layer has
  * to be initialized.
  * @param  void
  * @retval void
 ******************************************************************************/
void XcpExit( void )
{
    /* Activation control */
    XcpPlCheckControlState()

    /* Deinitialize the RAM interface */
    /* PRQA S 2006 1 */ /* MD_MSR_14.7 */
}

/****************************************************************************/
/* Print via SERV/SERV_TEXT                                                */
/****************************************************************************/
#if defined ( XCP_ENABLE_SERV_TEXT )
    #if defined ( XCP_ENABLE_SERV_TEXT_PUTCHAR )
/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  Put a char into a service request packet (SERV).
  * @note   CALLED BY:  application, XcpPrint PRECONDITIONS: XCP is initialized 
  * and in connected state.
  * @param  c : character
  * @retval void
 ******************************************************************************/
void XcpPutchar( const vuint8 c )
{
    /* Activation control */
    XcpPlCheckControlState()

    /* Check for stall condition */         
    #if defined ( XCP_ENABLE_SEND_QUEUE )

    while ( (xcp.SendStatus & (vuint8)XCP_EVT_REQUEST) != 0 )
    {
        if ( ApplXcpSendStall() == 0 )
        {
            return; /* Abort */
        }
    }

    #endif

    /* If xcp.EvLen!=0 there is a pending text message*/
    if (xcp.EvLen<2)
    {
        xcp.EvLen = 2;
    }

    xcp.Ev.b[xcp.EvLen] = c;
    xcp.EvLen++;

    if ( (xcp.EvLen>=(vuint8)kXcpMaxCTO) || (c==(vuint8)0x00u) )  /* Flush */
    { 
        EV_BYTE(0) = 0xFC; /* SERV */
        EV_BYTE(1) = 0x01; /* SERV_TEXT*/
        XcpSendEv();
    }
    /* PRQA S 2006 1 */ /* MD_MSR_14.7 */
}

        #if defined ( XCP_ENABLE_SERV_TEXT_PRINT )
/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  Transmission of a service request packet (SERV).
  * @note   CALLED BY:  application PRECONDITIONS: XCP is initialized and in 
  * connected state.
  * @param  *str : pointer to a string
  * @retval void
 ******************************************************************************/
void XcpPrint( const vuint8 *str )
{
    if( !str )
    {
        return;
    }
    /* Activation control */
    XcpPlCheckControlState()

    /* Transmit the text message. */
    while ( *str != 0x00 )
    {
        XcpPutchar(*str);
        str++; /* PRQA S 0489 */ /* MD_Xcp_0489 */
    }
    /* Transmit the terminating 0x00. */
    XcpPutchar( (vuint8)0x00u );
    /* PRQA S 2006 1 */ /* MD_MSR_14.7 */
}

        #endif
        #if defined ( XCP_ENABLE_SERV_TEXT_PRINTF )

/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  Printf into a SERV_TEXT message
  * @note   CALLED BY:  application
  * connected state.
  * @param  *str : pointer to a string
  *         ...  : varaibale number of parameters (see printf)
  * @retval void
 ******************************************************************************/
void XcpPrintf( const vuint8 *str, ... )
{
    if(!str)
    {
        return;
    }
    va_list argptr;
    vuint8 buf[128];

    /* Activation control */
    XcpPlCheckControlState()

    va_start(argptr,str);
    //vsprintf((vsint8*)buf,( const vsint8*)str,argptr );
    va_end(argptr);

    /* Transmit the text message*/
    {
        vuint8 *p = buf;
        while (*p != 0)
        {
            XcpPutchar(*p);
            p++; /* PRQA S 0489 */ /* MD_Xcp_0489 */
        }
    }
    /* Transmit the terminating 0x00. */
    XcpPutchar( 0x00 );
}

        #endif /* XCP_ENABLE_SERV_TEXT_PRINTF */
    #endif /* XCP_ENABLE_SERV_TEXT_PUTCHAR */
#endif /* XCP_ENABLE_SERV_TEXT */


#if defined ( XCP_ENABLE_SEND_EVENT )
/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  Transmission of an event packet (EV).
  * @note   CALLED BY:  application
  * connected state.
  * @param  evc : event code 
  *         c   : pointer to event data
  *         len : event data length
  * @retval void
 ******************************************************************************/
void XcpSendEvent( vuint8 evc, const BYTEPTR c, vuint8 len)
{
    vuint8 i;

    /* Activation control */
    XcpPlCheckControlState()

    /* Check for stall condition */         
    #if defined ( XCP_ENABLE_SEND_QUEUE )
    while ( (xcp.SendStatus & (vuint8)XCP_EVT_REQUEST) != (vuint8)0u )
    {
        if (!ApplXcpSendStall())
        {
            return; /* Abort */
        }
    }
    #endif

    EV_BYTE(0)  = PID_EV; /* Event*/
    EV_BYTE(1)  = evc;  /* Event Code*/
    xcp.EvLen   = 2;
    
    if (len <= (vuint8)(kXcpMaxCTO-2) )
    {
        if (c != 0x00u)
        {
            for (i = 0; i < len; i++)
            {
                xcp.Ev.b[xcp.EvLen]       = c[i];
                xcp.EvLen++;
            }
        } 
        else
        {
            xcp.EvLen += len;
        }
    }

    #if defined ( XCP_ENABLE_TESTMODE )
    if ( gDebugLevel != 0)
    {
        ApplXcpPrint("[XcpSendEvent]");
        for (i = 0; i < xcp.EvLen; i++)
        {
            ApplXcpPrint(" %02x",xcp.Ev.b[i]);
        }
        ApplXcpPrint("\n");
    }
    #endif

    XcpSendEv();
    /* PRQA S 2006 1 */ /* MD_MSR_14.7 */
}

#endif /* XCP_ENABLE_SEND_EVENT */

#if defined ( XCP_ENABLE_GET_CONNECTION_STATE ) || \
    defined ( XCP_ENABLE_GET_SESSION_STATUS_API )

/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  Get the connection state of the XCP Protocol Layer
  * @note   CALLED BY:  XcpPreCopy
  * @param  void
  * @retval XCP_CONNECTED    : XCP is connected
  *         XCP_DISCONNECTED : XCP is disconnected
 ******************************************************************************/
vuint8 XcpGetState( void )
{
    return ((xcp.SessionStatus & (SessionStatusType)SS_CONNECTED) > 0u) ? \
        XCP_CONNECTED : XCP_DISCONNECTED;
}
#endif


#if defined ( XCP_ENABLE_GET_SESSION_STATUS_API )
/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  Get the session state of the XCP Protocol Layer
  * @note   
  * @param  void
  * @retval Xcp session state
 ******************************************************************************/
SessionStatusType XcpGetSessionStatus( void )
{
    SessionStatusType result;

    result = xcp.SessionStatus;
    /* Reset the polling state */
    xcp.SessionStatus &= (SessionStatusType)(~SS_POLLING);
    return(result);
} /* PRQA S 2006 */ /* MD_MSR_14.7 */
#endif


#if defined ( XCP_ENABLE_GET_XCP_DATA_POINTER )
/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  Get the pointer to the internal xcp structure
  * @note   CALLED BY:  Application
  * @param  tXcpData ** pXcpData: Pointer to Pointer that is set to xcp
  * @retval void
 ******************************************************************************/
void XcpGetXcpDataPointer( RAM tXcpData ** pXcpData )
{
    if(!pXcpData)
    {
        return;
    }
    *pXcpData = &xcp;
}
#endif


#if defined ( XCP_ENABLE_VERSION_INFO_API )
/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  Returns version information of module
  * @note   
  * @param  Pointer to location at which version information shall be stored at
  * @retval void
 ******************************************************************************/
void XcpGetVersionInfo(Std_VersionInfoType *XcpVerInfoPtr)
{
    if(!XcpVerInfoPtr)
    {
        return;
    }
    /* Since this service only access non-volatile data and no channel parameter
    is passed, checking of the channel handle and initialization is omitted. */
    XcpVerInfoPtr->vendorID = XCP_VENDOR_ID;
    XcpVerInfoPtr->moduleID = XCP_MODULE_ID;
    XcpVerInfoPtr->sw_major_version = (CP_XCP_VERSION >> 8u);
    XcpVerInfoPtr->sw_minor_version = (CP_XCP_VERSION & 0xff);
    XcpVerInfoPtr->sw_patch_version = CP_XCP_RELEASE_VERSION;
}
#endif /* XCP_ENABLE_VERSION_INFO_API */


/******************************************************************************/
/* Test                                                                       */
/* Some screen output functions for test and diagnostics                      */
/******************************************************************************/


#if defined ( XCP_ENABLE_TESTMODE )
    #if defined ( XCP_ENABLE_DAQ )
/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  Print all DAQ lists to screen
  * @note   
  * @param  void
  * @retval void
 ******************************************************************************/
void XcpPrintDaqList( vuint8 daq )
{
    vuint8 i;
    vuint16 e;

    /* Activation control */
    XcpPlCheckControlState()

    if (daq>=xcp.Daq.DaqCount)
    {
        return;
    }

    ApplXcpPrint("DAQ %u:\n",daq);
    #if defined ( kXcpMaxEvent ) 
    for (i=0;i<kXcpMaxEvent;i++)
    {
        if (xcp.Daq.EventDaq[i]==daq)
        {
            ApplXcpPrint(" eventchannel=%04Xh,",i);
        }
    }
    #else
    ApplXcpPrint(" eventchannel=%04Xh,",DaqListEventChannel(daq));
    #endif
    #if defined (XCP_ENABLE_DAQ_PRESCALER )
    ApplXcpPrint(" prescaler=%u,",DaqListPrescaler(daq));
    #endif
    ApplXcpPrint(" firstOdt=%u,",DaqListFirstOdt(daq));
    ApplXcpPrint(" lastOdt=%u,",DaqListLastOdt(daq));
    ApplXcpPrint(" flags=%02Xh\n",DaqListFlags(daq));
    ApplXcpPrint(" firstPid=%02Xh\n",DaqListFirstPid(daq)); 
    for (i=DaqListFirstOdt(daq);i<=DaqListLastOdt(daq);i++)
    {
        ApplXcpPrint("  ODT %u (%u):\n",i-DaqListFirstOdt(daq),i);
        ApplXcpPrint("   pid=%u:\n",i);
        ApplXcpPrint("   firstOdtEntry=%u,lastOdtEntry=%u:\n", \
            DaqListOdtFirstEntry(i),DaqListOdtLastEntry(i));
        for (e=DaqListOdtFirstEntry(i);e<=DaqListOdtLastEntry(i);e++)
        {
            ApplXcpPrint("   [%08Xh,%u]\n",OdtEntryAddr(e),OdtEntrySize(e));
        }
    } /* j */
} /* Deviation of MISRA rule 82 (more than one return path). */

    #endif /* XCP_ENABLE_DAQ */
#endif /* XCP_ENABLE_TESTMODE */

/******************************************************************************/
/* Consistency checks                                                         */
/******************************************************************************/

/* Check definition of endianess of CPU */

#if defined ( XCP_CPUTYPE_LITTLEENDIAN ) || defined ( XCP_CPUTYPE_BIGENDIAN )
#else
    #error "Please define XCP_CPUTYPE_LITTLEENDIAN or XCP_CPUTYPE_BIGENDIAN."
#endif

/* Check consistency of alignment switch */

#if defined ( XCP_DISABLE_UNALIGNED_MEM_ACCESS )
    #if defined ( C_CPUTYPE_8BIT )
        #error "XCP consistency error: Forcing alignment for 8-bit \
            microcontrollers is useless."
    #endif
    #if defined ( XCP_ENABLE_UNALIGNED_MEM_ACCESS )
        #error "XCP consistency error: Select either alignment or no alignment."
    #endif
#else /* if defined ( XCP_DISABLE_UNALIGNED_MEM_ACCESS ) */
    #if defined ( XCP_ENABLE_UNALIGNED_MEM_ACCESS )
    #else
        #error "XCP consistency error: Usage of alignment not specified."
    #endif
#endif /* if defined ( XCP_DISABLE_UNALIGNED_MEM_ACCESS ) */

/******************************************************************************/
/* Organi check                                                               */
/******************************************************************************/



















/* module specific MISRA deviations:
  MD_Xcp_0306:    Rule 11.3
      Reason:     Pointer alignment has to be set manually due to dynamically structures; solved via cast to int, AND calculation and cast back to a pointer.
      Risk:       Maintainability reduced due to optimization.
      Prevention: Covered by code review.
  MD_Xcp_0310:    Rule 11.4
      Reason:     Alignment is given by calling function.
      Risk:       Maintainability reduced due to optimization.
      Prevention: Covered by code review.
  MD_Xcp_0310_ByteCopy: Rule 11.4
      Reason:     XCP accesses arrays always byte-wise (ODT handling and interpretation)
      Risk:       Maintainability reduced due to optimization.
      Prevention: Covered by code review.
  MD_Xcp_0311_StimBuffer: Rule 11.5
      Reason:     Casting to remove const for optimization reason; XCP command data is re-used for status flag handling.
      Risk:       Maintainability reduced due to optimization.
      Prevention: Covered by code review.
  MD_Xcp_0488:    Rule 17.4
      Reason:     For optimization reasons direct pointer arithmetic is used.
      Risk:       There is no risk as this has no effect on the code.
      Prevention: Covered by code review.
  MD_Xcp_0489:    Rule 17.4
      Reason:     For optimization reasons direct pointer arithmetic is used.
      Risk:       There is no risk as this has no effect on the code.
      Prevention: Covered by code review.
  MD_Xcp_0770:    Rule 14.5
      Reason:     For optimization reasons more than one break/continue was used to leave the for loop.
      Risk:       Maintainability reduced due to multiple break/continue statements.
      Prevention: Covered by code review.
  MD_Xcp_2001:    Rule 14.4
      Reason:     For optimization reasons a goto is used.
      Risk:       There is no risk as this the goto is only used function internally.
      Prevention: Covered by code review.
  MD_Xcp_2003:    Rule 15.2
      Reason:     For optimization reasons fall-through is used.
      Risk:       There is no risk as this the goto is only used function internally.
      Prevention: Covered by code review.
  MD_Xcp_3397:    Rule 12.1
      Reason:     The statement as used in the ASAM specification is used.
      Risk:       There is no risk as this is reviewd by ASAM group.
      Prevention: Covered by code review.
  MD_Xcp_3408:    Rule 8.8
      Reason:     The variable is not defined as static to be accessible for calibration. No prototype is required for this.
      Risk:       There is no risk as this variable is not accessed externally.
      Prevention: Covered by code review.
  MD_Xcp_3757:    Rule 10.1
      Reason:     Due to byte access the variable is split.
      Risk:       There is no risk as this variable is positive.
      Prevention: Covered by code review.
  MD_Xcp_4130:    Rule 12.7
      Reason:     When a vuint8 is in reality 16bit then proper value range must be handled by and mask.
      Risk:       There is no risk as the target variable is intended to be uint8.
      Prevention: Covered by code review.
*/

/**********************************************************************************************************************
 *  END OF FILE: XcpProf.c
 *********************************************************************************************************************/
