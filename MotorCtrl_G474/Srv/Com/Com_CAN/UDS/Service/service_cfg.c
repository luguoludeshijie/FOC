#include "Srv.h"

// 服务配置表
const uds_service_t uds_service_list[SID_NUM]  =
{
    /* SID   服务处理函数                            长度是否合法            是否支持默认会话
                                                                                   是否支持编程会话
                                                                                          是否支持扩展会话
                                                                                                 是否支持功能寻址
                                                                                                        是否支持肯定响应抑制
                                                                                                               安全访问等级 */
    {SID_10, service_10_SessionControl,             service_10_check_len,  TRUE,   TRUE,  TRUE,  TRUE,  TRUE,  UDS_SA_NON},
    {SID_11, service_11_EcuReset,                   service_11_check_len,  FALSE,  TRUE,  TRUE,  TRUE,  TRUE,  UDS_SA_NON},
    {SID_27, service_27_SecurityAccess,             service_27_check_len,  FALSE,  TRUE,  TRUE,  FALSE, FALSE, UDS_SA_NON},
    {SID_28, service_28_CommunicationControl,       service_28_check_len,  FALSE,  TRUE,  TRUE,  TRUE,  TRUE,  UDS_SA_NON},
    {SID_3E, service_3E_TesterPresent,              service_3E_check_len,  TRUE,   TRUE,  TRUE,  TRUE,  TRUE,  UDS_SA_NON},
    {SID_85, service_85_ControlDTCSetting,          service_85_check_len,  FALSE,  FALSE, TRUE,  TRUE,  TRUE,  UDS_SA_NON},
    {SID_22, service_22_ReadDataByIdentifier,       service_22_check_len,  TRUE,   TRUE,  TRUE,  TRUE,  FALSE, UDS_SA_NON},
    {SID_2E, service_2E_WriteDataByIdentifier,      service_2E_check_len,  FALSE,  TRUE,  TRUE,  FALSE, FALSE, UDS_SA_LV1},
    {SID_14, service_14_ClearDiagnosticInformation, service_14_check_len,  TRUE,   TRUE,  TRUE,  TRUE,  FALSE, UDS_SA_NON},
    {SID_19, service_19_ReadDTCInformation,         service_19_check_len,  TRUE,   TRUE,  TRUE,  TRUE,  FALSE, UDS_SA_NON},
    {SID_31, service_31_RoutineControl,             service_31_check_len,  FALSE,  TRUE,  TRUE,  FALSE, FALSE, UDS_SA_LV1},
    {SID_34, service_34_RequestDownload,            service_34_check_len,  FALSE,  TRUE,  TRUE,  FALSE, FALSE, UDS_SA_LV1},
    {SID_36, service_36_TransferData,               service_36_check_len,  FALSE,  TRUE,  TRUE,  FALSE, FALSE, UDS_SA_LV1},
    {SID_37, service_37_RequestTransferExit,        service_37_check_len,  FALSE,  TRUE,  TRUE,  FALSE, FALSE, UDS_SA_LV1},
};


DTCSnapshotFormat_t DTCSnapshot_record_0x960100[3] =
{
    {100,12,0},
    {101,12,0},
    {101,12,10}
};
DTCSnapshotFormat_t DTCSnapshot_record_0x960200[3] =
{
    {100,12,0},
    {101,12,0},
    {101,12,10}
};

DTC_Format_t DTC_Table[DTC_NUM] =
{
    {0x960100,0,DTCSnapshot_record_0x960100,3},//自定义 例：过流
    {0x960200,0,DTCSnapshot_record_0x960200,3}//自定义 例：过温
};
/****************EOF****************/
