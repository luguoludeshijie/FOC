#include "Srv.h"

/******************************************************************************
 * 函数名称: Boolean service_22_check_len(const uint8_t* msg_buf, uint16_t msg_dlc)
 * 功能说明: 检查 22 服务数据长度是否合法
 * 输入参数: uint16_t msg_dlc         --数据长度
 * 输出参数: 无
 * 函数返回: TRUE: 合法; FALSE: 非法
 * 其它说明: 无
 ******************************************************************************/
Boolean service_22_check_len(const uint8_t *msg_buf, uint16_t msg_dlc)
{
    Boolean ret = FALSE;

    (void)msg_buf;
    if (3 == msg_dlc)
        ret = TRUE;

    return ret;
}

/******************************************************************************
* 函数名称: void service_22_ReadDataByIdentifier(const uint8_t* msg_buf, uint16_t msg_dlc)
* 功能说明: 22 服务 - 根据标识符读取数据
* 输入参数: uint8_t*    msg_buf         --数据首地址
    　　　　uint8_t     msg_dlc         --数据长度
* 输出参数: 无
* 函数返回: 无
* 其它说明: 无
******************************************************************************/
void service_22_ReadDataByIdentifier(const uint8_t *msg_buf, uint16_t msg_dlc)
{
    uint8_t rsp_buf[128];
    uint16_t did;

    rsp_buf[0] = USD_GET_POSITIVE_RSP(SID_22);
    rsp_buf[1] = msg_buf[1];
    rsp_buf[2] = msg_buf[2];

    did = ((uint16_t)msg_buf[1]) << 8;
    did |= msg_buf[2];

    switch (did)
    {
    case 0xF186:
        rsp_buf[3] = get_current_session();
        uds_positive_rsp(rsp_buf, 4);
        break;

    default:
        uds_negative_rsp(SID_22, NRC_REQUEST_OUT_OF_RANGE);
        break;
    }
}

/****************EOF****************/
