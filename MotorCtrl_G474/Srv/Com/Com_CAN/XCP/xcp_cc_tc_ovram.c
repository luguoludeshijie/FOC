/*******************************************************************************
 * xcp_cc_tc_ovram.c
 * Note:   Copy from Vector Informatik GmbH XCP on CAN demo
 * Created on: 2023/09/16
 * Author: bin.liao
 ******************************************************************************/


/*******************************************************************************
 *------------------------------Include Section---------------------------------
 ******************************************************************************/
#include "Srv.h"
#ifdef XCP_CC_TC_OVRAM

/*******************************************************************************
 *-------------------------------Declaration------------------------------------
 ******************************************************************************/
#define IGNORE_SEGMENT_NUMBER
#define CALPAGE_RAM 1
#define CALPAGE_FLASH 0

/*******************************************************************************
 *-----------------------Global variable definition-----------------------------
 ******************************************************************************/
static vuint8 gCC_XCP_CalPage = CALPAGE_RAM; // RAM
static vuint8 gCC_ECU_CalPage = CALPAGE_RAM; // RAM

/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  ApplXcpCalibrationRead
  * @note   
  * @param  
  * @retval 
 ******************************************************************************/
static vuint8 read(vuint32 a)
{
  return *(vuint8 *)a; // Normal read
}

/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  write
  * @note   
  * @param  
  * @retval 
 ******************************************************************************/
static vuint8 XCP_FAR write(vuint32 a, vuint8 d)
{
  *(vuint8 *)a = d; // Normal write
  return XCP_CMD_OK;
}

/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  ApplXcpCalibrationWrite
  * @note   
  * @param  
  * @retval 
 ******************************************************************************/
vuint8 ApplXcpCalibrationWrite (MTABYTEPTR addr, vuint8 size, 
    MEMORY_ROM BYTEPTR data)
{
    if(!addr)
    {
        return XCP_CMD_DENIED;
    }
    while (size > 0)
    {
        write((vuint32)addr, *data);

        addr++;
        data++;
        size--;
    }
    return XCP_CMD_OK;
}

/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  ApplXcpCalibrationRead
  * @note   
  * @param  
  * @retval 
 ******************************************************************************/
vuint8 ApplXcpCalibrationRead ( MTABYTEPTR addr, vuint8 size, BYTEPTR data )
{
    if((!addr) || (!data) )
    {
        return XCP_CMD_DENIED;
    }
    while (size > 0)
    {
        *data = read((vuint32)addr);
        data++;
        addr++;
        size--;
    }
    return XCP_CMD_OK;
}

/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  ApplXcpReadChecksumValue
  * @note   This function is not used in this project, but in order to prevent 
  * Canape from reporting errors,it directly returns OK.
  * @param  
  * @retval 
 ******************************************************************************/
#if defined(XCP_ENABLE_CHECKSUM)
tXcpChecksumAddType ApplXcpReadChecksumValue(vuint32 addr)
{
    tXcpChecksumAddType s;

    ApplXcpCalibrationRead((MTABYTEPTR)addr, sizeof(tXcpChecksumAddType), 
        (BYTEPTR)&s);
    return s;
}
#endif

/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  ApplXcpGetCalPage
  * @note   This function is not used in this project, but in order to prevent 
  * Canape from reporting errors,it directly returns OK.
  * @param  
  * @retval 
 ******************************************************************************/
vuint8 ApplXcpGetCalPage(vuint8 segment, vuint8 mode)
{
    // Segment number is ignored
    #ifndef IGNORE_SEGMENT_NUMBER
        if (segment != 0)
        return CRC_SEGMENT_NOT_VALID; // Only segment 0 allowed
    #endif

    if ((mode & (CAL_ECU | CAL_XCP)) == CAL_XCP){
        return gCC_XCP_CalPage;
    }
    return gCC_ECU_CalPage;
}

/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  ApplXcpSetCalPage
  * @note   This function is not used in this project, but in order to prevent 
  * Canape from reporting errors,it directly returns OK.
  * @param  
  * @retval 
 ******************************************************************************/
vuint8 ApplXcpSetCalPage(vuint8 segment, vuint8 page, vuint8 mode)
{
    // Segment number is ignored
    #ifndef IGNORE_SEGMENT_NUMBER
        if (segment != 0)
        return CRC_SEGMENT_NOT_VALID; // Only segment 0 allowed
    #endif

    if (!(mode & CAL_ALL))
    {
        return CRC_OUT_OF_RANGE; // Mode must be CAL_ALL (all segments)
    }
    if (page > 1)
    {
        return CRC_PAGE_NOT_VALID; // Page may be 0=FLASH or 1=RAM
    }

    if ((mode & (CAL_ECU | CAL_XCP)) == (CAL_ECU | CAL_XCP))
    { // Switch ECU and XCP access 
        if (page != gCC_ECU_CalPage)
        {
            gCC_XCP_CalPage = page;
            gCC_ECU_CalPage = page;
        }
    }
    else if (mode & CAL_ECU)
    { // Switch only ECU access
        if (page != gCC_ECU_CalPage)
        {
            gCC_ECU_CalPage = page;
        }
    }
    else if (mode & CAL_XCP)
    {
        // Switch only XCP access
        gCC_XCP_CalPage = page;
    }
    else
    {
        //nothing
    }

    return 0;
}

/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  ApplXcpCopyCalPage
  * @note   This function is not used in this project, but in order to prevent 
  * Canape from reporting errors,it directly returns OK.
  * @param  
  * @retval 
 ******************************************************************************/
vuint8 ApplXcpCopyCalPage(vuint8 srcSeg, vuint8 srcPage, vuint8 destSeg, 
    vuint8 destPage)
{
    return 0;
}

/*******************************************************************************
 *--------------------------------Functions-------------------------------------
 ******************************************************************************/
/*******************************************************************************
  * @brief  APP XCP init
  * @note   
  * @param  
  * @retval 
 ******************************************************************************/
void ApplXcpInit(void)
{
  gCC_XCP_CalPage = CALPAGE_RAM; // RAM
  gCC_ECU_CalPage = CALPAGE_RAM; // RAM
}

#endif // XCP_CC_TC_OVRAM