/*****************************************************************/ /**
* @file qosa_at_cmd.h
* @brief
* @author larson.li@quectel.com
* @date 2023-05-01
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2023-05-01 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#ifndef _AT_CMD_H__
#define _AT_CMD_H__

#include "qosa_def.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   

#include "qosa_sys.h"
#include "qosa_at_param.h"
/*========================================================================
 *  Errcode Definition
 *========================================================================*/
typedef enum
{
    QOSA_CMD_RC_OK,             /*!< "OK" */
    QOSA_CMD_RC_CONNECT,        /*!< "CONNECT" */
    QOSA_CMD_RC_RING,           /*!< "RING/CRING" */
    QOSA_CMD_RC_NOCARRIER,      /*!< "NO CARRIER" */
    QOSA_CMD_RC_ERROR,          /*!< "ERROR" */
    QOSA_CMD_RC_NOTSUPPORT,     /*!< "NOT SUPPORT" */
    QOSA_CMD_RC_NODIALTONE,     /*!< "NO DIALTONE" */
    QOSA_CMD_RC_BUSY,           /*!< "BUSY" */
    QOSA_CMD_RC_NOANSWER,       /*!< "NO ANSWER" */
    QOSA_CMD_RC_INVCMDLINE,     /*!< "INVALID COMMAND LINE" */
    QOSA_CMD_RC_CR,             /*!< "\r\n" */
    QOSA_CMD_RC_SIMDROP,        /*!< "Sim drop" */
    QOSA_CMD_RC_SEND_OK = 17,   /*!< "SEND OK" */
    QOSA_CMD_RC_SEND_FAIL = 18, /*!< "SEND FAIL" */
    QOSA_CMD_RC_MAX
} qosa_at_resp_base_result_e;

#define QOSA_ERR_AT_UNKNOWN                             QOSA_ERR_AT_CME_EXE_FAIL

// CME error code define start
#define QOSA_ERR_AT_CME_PHONE_FAILURE                   0
#define QOSA_ERR_AT_CME_NO_CONNECT_PHONE                1
#define QOSA_ERR_AT_CME_PHONE_ADAPTER_LINK_RESERVED     2
#define QOSA_ERR_AT_CME_OPERATION_NOT_ALLOWED           3
#define QOSA_ERR_AT_CME_OPERATION_NOT_SUPPORTED         4
#define QOSA_ERR_AT_CME_PHSIM_PIN_REQUIRED              5
#define QOSA_ERR_AT_CME_PHFSIM_PIN_REQUIRED             6
#define QOSA_ERR_AT_CME_PHFSIM_PUK_REQUIRED             7
#define QOSA_ERR_AT_CME_SIM_NOT_INSERTED                10
#define QOSA_ERR_AT_CME_SIM_PIN_REQUIRED                11
#define QOSA_ERR_AT_CME_SIM_PUK_REQUIRED                12
#define QOSA_ERR_AT_CME_SIM_FAILURE                     13
#define QOSA_ERR_AT_CME_SIM_BUSY                        14
#define QOSA_ERR_AT_CME_SIM_WRONG                       15
#define QOSA_ERR_AT_CME_INCORRECT_PASSWORD              16
#define QOSA_ERR_AT_CME_SIM_PIN2_REQUIRED               17
#define QOSA_ERR_AT_CME_SIM_PUK2_REQUIRED               18
#define QOSA_ERR_AT_CME_MEMORY_FULL                     20
#define QOSA_ERR_AT_CME_INVALID_INDEX                   21
#define QOSA_ERR_AT_CME_NOT_FOUND                       22
#define QOSA_ERR_AT_CME_MEMORY_FAILURE                  23
#define QOSA_ERR_AT_CME_TEXT_LONG                       24
#define QOSA_ERR_AT_CME_INVALID_CHAR_INTEXT             25
#define QOSA_ERR_AT_CME_DAIL_STR_LONG                   26
#define QOSA_ERR_AT_CME_INVALID_CHAR_INDIAL             27
#define QOSA_ERR_AT_CME_NO_NET_SERVICE                  30
#define QOSA_ERR_AT_CME_NETWORK_TIMOUT                  31
#define QOSA_ERR_AT_CME_NOT_ALLOW_EMERGENCY             32
#define QOSA_ERR_AT_CME_NET_PER_PIN_REQUIRED            40
#define QOSA_ERR_AT_CME_NET_PER_PUK_REQUIRED            41
#define QOSA_ERR_AT_CME_NET_SUB_PER_PIN_REQ             42
#define QOSA_ERR_AT_CME_NET_SUB_PER_PUK_REQ             43
#define QOSA_ERR_AT_CME_SERVICE_PROV_PER_PIN_REQ        44
#define QOSA_ERR_AT_CME_SERVICE_PROV_PER_PUK_REQ        45
#define QOSA_ERR_AT_CME_CORPORATE_PER_PIN_REQ           46
#define QOSA_ERR_AT_CME_CORPORATE_PER_PUK_REQ           47
#define QOSA_ERR_AT_CME_PHSIM_PBK_REQUIRED              48

// extern errocode
#define QOSA_ERR_AT_CME_EXE_NOT_SURPORT                 49
#define QOSA_ERR_AT_CME_EXE_FAIL                        50
#define QOSA_ERR_AT_CME_NO_MEMORY                       51
#define QOSA_ERR_AT_CME_OPTION_NOT_SURPORT              52
#define QOSA_ERR_AT_CME_PARAM_INVALID                   53
#define QOSA_ERR_AT_CME_EXT_REG_NOT_EXIT                54
#define QOSA_ERR_AT_CME_EXT_SMS_NOT_EXIT                55
#define QOSA_ERR_AT_CME_EXT_PBK_NOT_EXIT                56
#define QOSA_ERR_AT_CME_EXT_FFS_NOT_EXIT                57
#define QOSA_ERR_AT_CME_INVALID_COMMAND_LINE            58
#define QOSA_ERR_AT_CME_ITF_DIFFERENT                   59
#define QOSA_ERR_AT_CME_BURN_FLASH_FAIL                 60
#define QOSA_ERR_AT_CME_TFLASH_NOT_EXIST                61
#define QOSA_ERR_AT_CME_FILE_NOT_EXIST                  62
#define QOSA_ERR_AT_CME_FILE_TOO_LARGE                  63

#define QOSA_ERR_AT_CME_INVALID_DATE_OR_TIME            96
#define QOSA_ERR_AT_CME_DIR_CREATE_FAIL                 97
#define QOSA_ERR_AT_CME_DIR_NOT_EXIST                   98
#define QOSA_ERR_AT_CME_NOT_IMPLEMENTED                 99

//
// GPRS-related errors.
// Errors related to a failure to perform an Attach.(Values in parentheses are GSM 04.08 cause codes.)
//
#define QOSA_ERR_AT_CME_GPRS_ILLEGAL_MS_3               103  // Illegal MS (#3)
#define QOSA_ERR_AT_CME_GPRS_ILLEGAL_MS_6               106  // Illegal ME (#6)
#define QOSA_ERR_AT_CME_GPRS_SVR_NOT_ALLOWED            107  // GPRS services not allowed (#7)
#define QOSA_ERR_AT_CME_GPRS_PLMN_NOT_ALLOWED           111  // PLMN not allowed (#11)
#define QOSA_ERR_AT_CME_GPRS_LOCATION_AREA_NOT_ALLOWED  112  // Location area not allowed (#12)
#define QOSA_ERR_AT_CME_GPRS_ROAMING_NOT_ALLOWED        113  // Roaming not allowed in this location area (#13)

//
// Errors related to a failure to Activate a Context
//
#define QOSA_ERR_AT_CME_GPRS_OPTION_NOT_SUPPORTED       132  // service option not supported (#32)
#define QOSA_ERR_AT_CME_GPRS_OPTION_NOT_SUBSCRIBED      133  // requested service option not subscribed (#33)
#define QOSA_ERR_AT_CME_GPRS_OPTION_TEMP_ORDER_OUT      134  // service option temporarily out of order (#34)
#define QOSA_ERR_AT_CME_GPRS_PDP_AUTHENTICATION_FAILURE 149  // PDP authentication failure

//
// Other GPRS errors
// Other values in the range 101 - 150 are reserved for use by GPRS
//
#define QOSA_ERR_AT_CME_GPRS_INVALID_MOBILE_CLASS       150  // invalid mobile class
#define QOSA_ERR_AT_CME_GPRS_UNSPECIFIED_GPRS_ERROR     148  // unspecified GPRS error

#define QOSA_ERR_AT_CME_GPRS_UNSUPPORTED_QCI_VALUE      181

//
// GPRS-related errors end.
//

// AT_20071024_CAOW_B
// updated error code
#define QOSA_ERR_AT_CME_SIM_VERIFY_FAIL                 264
#define QOSA_ERR_AT_CME_SIM_UNBLOCK_FAIL                265
#define QOSA_ERR_AT_CME_SIM_CONDITION_NO_FULLFILLED     266
#define QOSA_ERR_AT_CME_SIM_UNBLOCK_FAIL_NO_LEFT        267
#define QOSA_ERR_AT_CME_SIM_VERIFY_FAIL_NO_LEFT         268
#define QOSA_ERR_AT_CME_SIM_INVALID_PARAMETER           269
#define QOSA_ERR_AT_CME_SIM_UNKNOW_COMMAND              270
#define QOSA_ERR_AT_CME_SIM_WRONG_CLASS                 271
#define QOSA_ERR_AT_CME_SIM_TECHNICAL_PROBLEM           272
#define QOSA_ERR_AT_CME_SIM_CHV_NEED_UNBLOCK            273
#define QOSA_ERR_AT_CME_SIM_NOEF_SELECTED               274
#define QOSA_ERR_AT_CME_SIM_FILE_UNMATCH_COMMAND        275
#define QOSA_ERR_AT_CME_SIM_CONTRADICTION_CHV           276
#define QOSA_ERR_AT_CME_SIM_CONTRADICTION_INVALIDATION  277
#define QOSA_ERR_AT_CME_SIM_MAXVALUE_REACHED            278
#define QOSA_ERR_AT_CME_SIM_PATTERN_NOT_FOUND           279
#define QOSA_ERR_AT_CME_SIM_FILEID_NOT_FOUND            280
#define QOSA_ERR_AT_CME_SIM_STK_BUSY                    281
#define QOSA_ERR_AT_CME_SIM_UNKNOW                      282
#define QOSA_ERR_AT_CME_SIM_PROFILE_ERROR               283

//add for HTTP AT COMMAND
#define QOSA_ERR_AT_CME_HTTP_NETIF_NULL                 300
#define QOSA_ERR_AT_CME_HTTP_ACTION_NEED                301
#define QOSA_ERR_AT_CME_HTTP_INIT_NEED                  302
#define QOSA_ERR_AT_CME_HTTP_PARA_CID_INVALID           303

#define QOSA_ERR_AT_CME_SNTP_SYNCING                    320

//add for MYNET AT COMMAND
#define QOSA_ERR_AT_CME_NETWORK_REFUSED                 900
#define QOSA_ERR_AT_CME_PDP_DEACTIVED                   901
#define QOSA_ERR_AT_CME_PDP_ACTIVED                     902
#define QOSA_ERR_AT_CME_TCPIP_PEER_REFUSED              910
#define QOSA_ERR_AT_CME_TCPIP_CONNECT_TIMEOUT           911  //IP OR PORT IS NOT CORRECT
#define QOSA_ERR_AT_CME_TCPIP_ALREADY_CONNECTED         912
#define QOSA_ERR_AT_CME_TCPIP_NOT_CONNECTED             913
#define QOSA_ERR_AT_CME_BUFFER_FULL                     914
#define QOSA_ERR_AT_CME_SEND_TIMEOUT                    915
#define QOSA_ERR_AT_CME_DNS_FAILED                      916
#define QOSA_ERR_AT_CME_DNS_TIMEOUT                     917
#define QOSA_ERR_AT_CME_DNS_UNKNOW_ERROR                918
#define QOSA_ERR_AT_CME_PARAM_ERROR                     980
#define QOSA_ERR_AT_CME_UNKNOWN_ERROR                   981

// AT_20071024_CAOW_E

// CME define end

// CMS error code define start
#define QOSA_ERR_AT_CMS_UNASSIGNED_NUM                  1
#define QOSA_ERR_AT_CMS_OPER_DETERM_BARR                8
#define QOSA_ERR_AT_CMS_CALL_BARRED                     10
#define QOSA_ERR_AT_CMS_SM_TRANS_REJE                   21
#define QOSA_ERR_AT_CMS_DEST_OOS                        27
#define QOSA_ERR_AT_CMS_UNINDENT_SUB                    28
#define QOSA_ERR_AT_CMS_FACILIT_REJE                    29
#define QOSA_ERR_AT_CMS_UNKONWN_SUB                     30
#define QOSA_ERR_AT_CMS_NW_OOO                          38
#define QOSA_ERR_AT_CMS_TMEP_FAIL                       41
#define QOSA_ERR_AT_CMS_CONGESTION                      42
#define QOSA_ERR_AT_CMS_RES_UNAVAILABLE                 47
#define QOSA_ERR_AT_CMS_REQ_FAC_NOT_SUB                 50
#define QOSA_ERR_AT_CMS_RFQ_FAC_NOT_IMP                 69
#define QOSA_ERR_AT_CMS_INVALID_SM_TRV                  81
#define QOSA_ERR_AT_CMS_INVALID_MSG                     95
#define QOSA_ERR_AT_CMS_INVALID_MAND_INFO               96
#define QOSA_ERR_AT_CMS_MSG_TYPE_ERROR                  97
#define QOSA_ERR_AT_CMS_MSG_NOT_COMP                    98
#define QOSA_ERR_AT_CMS_INFO_ELEMENT_ERROR              99
#define QOSA_ERR_AT_CMS_PROT_ERROR                      111
#define QOSA_ERR_AT_CMS_IW_UNSPEC                       127
#define QOSA_ERR_AT_CMS_TEL_IW_NOT_SUPP                 128
#define QOSA_ERR_AT_CMS_SMS_TYPE0_NOT_SUPP              129
#define QOSA_ERR_AT_CMS_CANNOT_REP_SMS                  130
#define QOSA_ERR_AT_CMS_UNSPEC_TP_ERROR                 143
#define QOSA_ERR_AT_CMS_DCS_NOT_SUPP                    144
#define QOSA_ERR_AT_CMS_MSG_CLASS_NOT_SUPP              145
#define QOSA_ERR_AT_CMS_UNSPEC_TD_ERROR                 159
#define QOSA_ERR_AT_CMS_CMD_CANNOT_ACT                  160
#define QOSA_ERR_AT_CMS_CMD_UNSUPP                      161
#define QOSA_ERR_AT_CMS_UNSPEC_TC_ERROR                 175
#define QOSA_ERR_AT_CMS_TPDU_NOT_SUPP                   176
#define QOSA_ERR_AT_CMS_SC_BUSY                         192
#define QOSA_ERR_AT_CMS_NO_SC_SUB                       193
#define QOSA_ERR_AT_CMS_SC_SYS_FAIL                     194
#define QOSA_ERR_AT_CMS_INVALID_SME_ADDR                195
#define QOSA_ERR_AT_CMS_DEST_SME_BARR                   196
#define QOSA_ERR_AT_CMS_SM_RD_SM                        197
#define QOSA_ERR_AT_CMS_TP_VPF_NOT_SUPP                 198
#define QOSA_ERR_AT_CMS_TP_VP_NOT_SUPP                  199
#define QOSA_ERR_AT_CMS_D0_SIM_SMS_STO_FULL             208
#define QOSA_ERR_AT_CMS_NO_SMS_STO_IN_SIM               209
#define QOSA_ERR_AT_CMS_ERR_IN_MS                       210
#define QOSA_ERR_AT_CMS_MEM_CAP_EXCCEEDED               211
#define QOSA_ERR_AT_CMS_SIM_APP_TK_BUSY                 212
#define QOSA_ERR_AT_CMS_SIM_DATA_DL_ERROR               213
#define QOSA_ERR_AT_CMS_UNSPEC_ERRO_CAUSE               255

#define QOSA_ERR_AT_CMS_ME_FAIL                         300
#define QOSA_ERR_AT_CMS_SMS_SERVIEC_RESERVED            301
#define QOSA_ERR_AT_CMS_OPER_NOT_ALLOWED                302
#define QOSA_ERR_AT_CMS_OPER_NOT_SUPP                   303
#define QOSA_ERR_AT_CMS_INVALID_PDU_PARAM               304
#define QOSA_ERR_AT_CMS_INVALID_TXT_PARAM               305
#define QOSA_ERR_AT_CMS_SIM_NOT_INSERT                  310
#define QOSA_ERR_AT_CMS_SIM_PIN_REQUIRED                311
#define QOSA_ERR_AT_CMS_PH_SIM_PIN_REQUIRED             312
#define QOSA_ERR_AT_CMS_SIM_FAIL                        313
#define QOSA_ERR_AT_CMS_SIM_BUSY                        314
#define QOSA_ERR_AT_CMS_SIM_WRONG                       315
#define QOSA_ERR_AT_CMS_SIM_PUK_REQUIRED                316
#define QOSA_ERR_AT_CMS_SIM_PIN2_REQUIRED               317
#define QOSA_ERR_AT_CMS_SIM_PUK2_REQUIRED               318

#define QOSA_ERR_AT_CMS_MEM_FAIL                        320
#define QOSA_ERR_AT_CMS_INVALID_MEM_INDEX               321
#define QOSA_ERR_AT_CMS_MEM_FULL                        322
#define QOSA_ERR_AT_CMS_SCA_ADDR_UNKNOWN                330
#define QOSA_ERR_AT_CMS_NO_NW_SERVICE                   331
#define QOSA_ERR_AT_CMS_NW_TIMEOUT                      332
#define QOSA_ERR_AT_CMS_NO_CNMA_ACK_EXPECTED            340
#define QOSA_ERR_AT_CMS_UNKNOWN_ERROR                   500

#define QOSA_ERR_AT_CMS_USER_ABORT                      512
#define QOSA_ERR_AT_CMS_UNABLE_TO_STORE                 513
#define QOSA_ERR_AT_CMS_INVALID_STATUS                  514
#define QOSA_ERR_AT_CMS_INVALID_ADDR_CHAR               515
#define QOSA_ERR_AT_CMS_INVALID_LEN                     516
#define QOSA_ERR_AT_CMS_INVALID_PDU_CHAR                517
#define QOSA_ERR_AT_CMS_INVALID_PARA                    518
#define QOSA_ERR_AT_CMS_INVALID_LEN_OR_CHAR             519
#define QOSA_ERR_AT_CMS_INVALID_TXT_CHAR                520
#define QOSA_ERR_AT_CMS_TIMER_EXPIRED                   521

#define QOSA_ERR_AT_CMS_SMS_SEND_FAIL                   530

// AUDIO ERRORS
#define QOSA_ERR_AT_CME_AUDIO_PALY_ERROR                901
#define QOSA_ERR_AT_CME_AUDIO_PARAM_INVALID             902
#define QOSA_ERR_AT_CME_AUDIO_OPTION_NOT_SURPORT        903
#define QOSA_ERR_AT_CME_AUDIO_WORK_BUSY                 904
#define QOSA_ERR_AT_CME_AUDIO_FILE_NOT_EXIT             905
#define QOSA_ERR_AT_CME_AUDIO_FILE_ALREADY_EXIT         906
#define QOSA_ERR_AT_CME_AUDIO_FILE_NO_SPACE             907
#define QOSA_ERR_AT_CME_AUDIO_NOT_INIT                  919

#define QOSA_DO_WHILE0(expr)                                                                                                                                   \
    do {                                                                                                                                                       \
        expr                                                                                                                                                   \
    } while (0)

typedef void (*qosa_at_timeout_cb_ptr)(qosa_at_cmd_t *cmd);

typedef enum QOSA_ATCI_RESULT_CODE
{
    QOSA_ATCI_RESULT_CODE_NULL,
    QOSA_ATCI_RESULT_CODE_OK,
    QOSA_ATCI_RESULT_CODE_ERROR,
    QOSA_ATCI_RESULT_CODE_CME_ERROR,
    QOSA_ATCI_RESULT_CODE_CMS_ERROR,
    QOSA_ATCI_RESULT_CODE_MAX
} qosa_atci_result_code_e;

/**  Mainly used for the current AT channel mode status of the application layer */
typedef enum
{
    QOSA_AT_MODE_NODE = 0,
    QOSA_AT_ATC_CMD_MODE,  /*!< AT command line mode */
    QOSA_AT_ATC_EDIT_MODE, /*!< Enter greater than sign edit mode "> " */
} qosa_at_parser_mode_e;

/**
 * helper data structure for integer string map
 */
typedef struct
{
    qosa_uint32_t value; /*!< integer value */
    const char   *str;   /*!< string value */
} qosa_osi_value_str_map_t;

/** Edit mode callback function type */
typedef qosa_uint32_t (*qosa_at_edit_entry_cb_ptr)(qosa_uint8_t *buff, qosa_uint32_t size, void *argv);

typedef struct
{
    void                     *user_ctx; /*!< Callback function user input parameter */
    qosa_at_dev_type_e        dev_port; /*!< Corresponding AT dev channel */
    qosa_at_edit_entry_cb_ptr cb;       /*!< Callback function to be registered */
    qosa_at_parser_mode_e     mode;     /*!< Record and save the original AT parser mode status */
} qosa_at_edit_param_t;

/**
* @brief Get corresponding string information according to the corresponding error code
*
* @param[in] qosa_at_resp_base_result_e code - code error type enumeration
*
* @return const char * Return string information corresponding to code
*/
const char *qosa_at_set_basic_result_code(qosa_at_resp_base_result_e code);

/**
* @brief Output corresponding string information according to the corresponding error code, similar to the at_set_basic_result_code
*        function, this function directly outputs string information to the current dev channel
*
* @param[in] qosa_at_dev_type_e dev_port - Device corresponding to qosa_at_dev_type_e enumeration
* @param[in] int code - code error type enumeration
*
* @note After execution, the AT command will not be returned, only the specified information is output
*/
void qosa_at_resp_basic_result_code(qosa_at_dev_type_e dev_port, qosa_at_resp_base_result_e code);

/**
* @brief AT parsing completion function, clear the current state, and send an event to the AT thread to read new data
*
* @param[in] qosa_at_dev_type_e dev_port - Device corresponding to qosa_at_dev_type_e enumeration
*
* @return int 0 indicates successful execution, others indicate failure
*
*/
int qosa_at_resp_finish(qosa_at_dev_type_e dev_port);

/**
* @brief Output default string without ending the AT command
*
* @param[in] qosa_at_dev_type_e dev_port - Channel for the AT command to be executed
* @param[in] const char * text - String information to be output
* @param[in] qosa_size_t length - Length information corresponding to the string
* @param[in] qosa_uint8_t padding - if need add \r\n at the begin and/or end of the string,
*            the begin padding is effected by atv
*			 0: no add s3,s4,
*            1: auto detect s3,s4 and add to begain and end,
*            2: force add s3,s4 at the begin and end,
*			 3: force add s3,s4 at the begin,
*            4: force add s3,s4 at the end, and 1 is default
*
* @return int Return 0 indicates successful execution, non-zero indicates failed execution
*
* @note This command does not end the current AT state, still in AT command mode
*/
int qosa_at_resp_cmd_info_text(qosa_at_dev_type_e dev_port, const char *text, qosa_size_t length, unsigned char padding);

/**
* @brief User returns AT command execution result, ends the corresponding AT command
*
* @param[in] qosa_at_dev_type_e dev_port - Channel for the AT command to be executed
*
* @param[in] qosa_atci_result_code_e resultCode - Corresponding to different types of return results
*            QOSA_ATCI_RESULT_CODE_OK, return type is OK, if resp_buff content is not empty, it will first output rsp_buffer content and then return OK
*            QOSA_ATCI_RESULT_CODE_ERROR, return type is ERROR
*            QOSA_ATCI_RESULT_CODE_CME_ERROR, return CME type ERROR error code
*            QOSA_ATCI_RESULT_CODE_CMS_ERROR, return CMS type ERROR error code
*
* @param[in] qosa_uint32_t report_code - Mainly used when resultCode is QOSA_ATCI_RESULT_CODE_CME_ERROR and
*            QOSA_ATCI_RESULT_CODE_CMS_ERROR to carry error codes, error code table see ERR_AT_CME_xxx and ERR_AT_CMS_xxx
*            macro definition types
*
* @param[in] char * rsp_buffer - Only valid when resultCode result is QOSA_ATCI_RESULT_CODE_OK
*
* @param[in] qosa_uint8_t padding - if need add \r\n at the begin and/or end of the string,
*            the begin padding is effected by atv
*			 0: no add s3,s4,
*            1: auto detect s3,s4 and add to begain and end,
*            2: force add s3,s4 at the begin and end,
*			 3: force add s3,s4 at the begin,
*            4: force add s3,s4 at the end, and 1 is default
*
* @return void
*
* @note Pay attention to the type of padding, after the function is executed, the AT command has been answered and re-entered AT command mode
*/
void qosa_at_resp_cmd(qosa_at_dev_type_e dev_port, qosa_atci_result_code_e resultCode, qosa_uint32_t report_code, char *rsp_buffer, qosa_uint8_t padding);

/**
* @brief The function is mainly used to output the greater than sign format ">" when entering edit mode
*
* @param[in] qosa_at_dev_type_e dev_port - Channel for the AT command to be executed
* @param[in] int formatcfg - Corresponding format type
*                               1. Output content is s3s4>s3s4 == \r\n>\r\n
*                               0. Output content is s3s4> space == "\r\n> "
*/
void qosa_at_resp_cmd_out_put_prompt(qosa_at_dev_type_e dev_port, int formatcfg);

/**
* @brief Provide for user layer call, used to specify AT command to configure custom timeout
*
* @param[in] qosa_at_dev_type_e dev_port - Device corresponding to qosa_at_dev_type_e enumeration
* @param[in] qosa_uint32_t timeout - Corresponding configured command timeout
* @param[in] qosa_at_timeout_cb_ptr handler - Corresponding cb function called by AT thread after timeout
*
* @return qosa_bool_t
*           QOSA_TRUE successful execution
*           QOSA_FALSE failed execution
*/
qosa_bool_t qosa_at_resp_set_timeout_cb(qosa_at_dev_type_e dev_port, qosa_uint32_t timeout, qosa_at_timeout_cb_ptr handler);

/**
 * @brief Configure AT command to enter edit mode
 *
 * @param dev_port   Corresponding dev of the channel
 * @param cb    User callback function for receiving data after entering edit mode
 * @param param User custom parameter
 *
 * @return int  Return 0 successful execution
 *              Return non-zero failed execution
 *
 * @note Note that this function must run in the AT task context, executed in AT command parsing
 */
int qosa_at_enter_edit_mode(qosa_at_dev_type_e dev_port, qosa_at_edit_entry_cb_ptr cb, void *param);

/**
* @brief AT command corresponding dev channel exits edit mode
*
* @param[in] qosa_at_dev_type_e dev_port - Corresponding dev of the AT channel
*
* @return int - 0 indicates successful execution, non-zero indicates failure
*/
int qosa_at_exit_edit_mode(qosa_at_dev_type_e dev_port);

/**
 * @brief Used to get the mode of the current AT parser, whether in AT parsing mode or in edit mode
 *        .
 *
 * @param[in] qosa_at_dev_type_e dev_port
 *            Dev channel information to get status
 *
 * @return qosa_bool_t
 *         Return OSA_TRUE indicates that the current AT parser is in AT command mode
 *         Return OSA_FALSE indicates that the current AT parser is in edit mode
 *
 * @note The currently obtained is only the mode in the AT parser, and you also need to get the status mode in the AT SIO to
 *       finally determine the current AT channel status.
 */
qosa_bool_t qosa_at_get_is_cmd_mode(qosa_at_dev_type_e dev_port);

/**
 * @brief Get corresponding SIM ID sequence number through dev channel
 *
 * @param[in] qosa_at_dev_type_e dev_port
 *            To get the dev channel information for the SIM ID
 *
 * @return qosa_uint8_t
 *         The application layer SIM ID corresponding to the current dev channel
 */
qosa_uint8_t qosa_get_sim_by_dev(qosa_at_dev_type_e dev_port);

/**
 * @brief Set the corresponding SIM ID number via the dev channel.
 *
 * @param[in] qosa_at_dev_type_e dev_port
 *            To set the dev channel information for the SIM ID
 *
 * @param[in] qosa_uint8_t sim_id
 *            SIM ID to be set
 *
 * @return qosa_bool_t
 *         return OSA_TRUE indicates that the set was successful.
 *         return OSA_FALSE indicates that the set failed.
 *
 * @note This function is only supported on devices with dual SIM dual standby enabled.
 */
qosa_bool_t qosa_set_sim_by_dev(qosa_at_dev_type_e dev_port, qosa_uint8_t sim_id);

/**
 * @brief Used for APP side to dynamically add AT commands
 *
 * @param[in] const qosa_at_desc_t * desc
 *          - Pointer to the AT command list to be added
 *
 * @param[in] qosa_uint32_t list_len
 *          - Number of commands in the desc command list
 *
 */
void qosa_at_parser_add_cust_at(const qosa_at_desc_t *desc, qosa_uint32_t list_len);

#define qosa_at_resp_error(dev_port)              qosa_at_resp_cmd(dev_port, QOSA_ATCI_RESULT_CODE_ERROR, QOSA_CMD_RC_ERROR, QOSA_NULL, 1);
#define qosa_at_resp_ok(dev_port)                 qosa_at_resp_cmd(dev_port, QOSA_ATCI_RESULT_CODE_OK, QOSA_CMD_RC_OK, QOSA_NULL, 1);
#define qosa_at_resp_cms_error(dev_port, errcode) qosa_at_resp_cmd(dev_port, QOSA_ATCI_RESULT_CODE_CMS_ERROR, errcode, QOSA_NULL, 1);
#define qosa_at_resp_cme_error(dev_port, errcode) qosa_at_resp_cmd(dev_port, QOSA_ATCI_RESULT_CODE_CME_ERROR, errcode, QOSA_NULL, 1);
#define qosa_at_resp_text(dev_port, text)         qosa_at_resp_cmd(dev_port, QOSA_ATCI_RESULT_CODE_OK, QOSA_CMD_RC_OK, text, 1);
#define qosa_at_resp_ntext(dev_port, text, len)   qosa_at_resp_cmd_info_text(dev_port, text, len, 1)

#define QOSA_RETURN_CME_ERR(dev_port, err)                                                                                                                     \
    {                                                                                                                                                          \
        qosa_at_resp_cme_error(dev_port, err);                                                                                                                 \
        return;                                                                                                                                                \
    }
#define QOSA_RETURN_CMS_ERR(dev_port, err)                                                                                                                     \
    {                                                                                                                                                          \
        qosa_at_resp_cms_error(dev_port, err);                                                                                                                 \
        return;                                                                                                                                                \
    }
#define QOSA_RETURN_OK(dev_port)                                                                                                                               \
    {                                                                                                                                                          \
        qosa_at_resp_ok(dev_port);                                                                                                                             \
        return;                                                                                                                                                \
    }
#define QOSA_RETURN_ERROR(dev_port)                                                                                                                            \
    {                                                                                                                                                          \
        qosa_at_resp_error(dev_port);                                                                                                                          \
        return;                                                                                                                                                \
    }
#define QOSA_RETURN_FOR_ASYNC()                                                                                                                                \
    {                                                                                                                                                          \
        return;                                                                                                                                                \
    }
#endif//!(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
#endif /* _AT_CMD_H__ */
