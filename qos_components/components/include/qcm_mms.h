/********************************************************************
* @file qcm_mms.h
* @brief
* @author lawrence.liu@quectel.com
* @date 2024-01-17
*
* @copyright Copyright (c) 2024 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2024-01-17 <td>1.0 <td>lawrence.liu <td> Init
* </table>
**********************************************************************/
#ifndef __QCM_MMS_H__
#define __QCM_MMS_H__

#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_queue_list.h"
#include "qcm_mms_config.h"

#define OSA_ERRCODE_MMS_BASE (QOSA_COMPONENT_MMS << 16)

/**
 * @enum qcm_mms_eercode_e.
 * @brief Issues corresponding to error codes returned by the MMS interface.
 */
typedef enum
{
    QCM_MMS_RES_OK = QOSA_OK,
    QCM_MMS_RES_ERROR_GENERAL = 1 | OSA_ERRCODE_MMS_BASE,
    QCM_MMS_RES_ERROR_NO_MEMORY = 2 | OSA_ERRCODE_MMS_BASE,
    QCM_MMS_RES_ERROR_PARAM_INVALID = 3 | OSA_ERRCODE_MMS_BASE,
    QCM_MMS_RES_ERROR_PARAM_IS_NULL = 4 | OSA_ERRCODE_MMS_BASE,
    QCM_MMS_RES_ERROR_WOULDBLOCK = 5 | OSA_ERRCODE_MMS_BASE,
    QCM_MMS_RES_ERROR_TASK = 6 | OSA_ERRCODE_MMS_BASE,
    QCM_MMS_RES_ERROR_URL_ERROR = 7 | OSA_ERRCODE_MMS_BASE,
    QCM_MMS_RES_ERROR_PROXYTYPE_NOSUPPORT = 8 | OSA_ERRCODE_MMS_BASE,
    QCM_MMS_RES_ERROR_PROXYADDR_ERROR = 9 | OSA_ERRCODE_MMS_BASE,
    QCM_MMS_RES_ERROR_FILE = 10 | OSA_ERRCODE_MMS_BASE,
    QCM_MMS_RES_ERROR_FILE_NOT_FOUND = 11 | OSA_ERRCODE_MMS_BASE,
    QCM_MMS_RES_ERROR_MMS_BUSY = 12 | OSA_ERRCODE_MMS_BASE,
    QCM_MMS_RES_ERROR_HTTP_RESPONSE_FAILED = 13 | OSA_ERRCODE_MMS_BASE,
    QCM_MMS_RES_ERROR_MMS_POST_RESPONS_INVALIDMSG = 14 | OSA_ERRCODE_MMS_BASE,
    QCM_MMS_RES_ERROR_POST_RESPONSE_REPORTERROR = 15 | OSA_ERRCODE_MMS_BASE,
    QCM_MMS_RES_ERROR_NETWORK_OPENFAILED = 16 | OSA_ERRCODE_MMS_BASE,
    QCM_MMS_RES_ERROR_NETWORK_ERROR = 17 | OSA_ERRCODE_MMS_BASE,
    QCM_MMS_RES_ERROR_SOC_CREATEFAILED = 18 | OSA_ERRCODE_MMS_BASE,
    QCM_MMS_RES_ERROR_SOC_CONNECTFAILED = 19 | OSA_ERRCODE_MMS_BASE,
    QCM_MMS_RES_ERROR_SOC_READFAILED = 20 | OSA_ERRCODE_MMS_BASE,
    QCM_MMS_RES_ERROR_SOC_WRITEFAILED = 21 | OSA_ERRCODE_MMS_BASE,
    QCM_MMS_RES_ERROR_SOC_CLOSE = 22 | OSA_ERRCODE_MMS_BASE,
    QCM_MMS_RES_ERROR_TIMEOUT = 23 | OSA_ERRCODE_MMS_BASE,
    QCM_MMS_RES_ERROR_ENCODE_DATAFAILED = 24 | OSA_ERRCODE_MMS_BASE,
    QCM_MMS_RES_ERROR_HTTP_DECODEERROR = 25 | OSA_ERRCODE_MMS_BASE,
    QCM_MMS_RES_ERROR_PARAM_TOADDR_FULL = 26 | OSA_ERRCODE_MMS_BASE,
    QCM_MMS_RES_ERROR_PARAM_CCADDR_FULL = 27 | OSA_ERRCODE_MMS_BASE,
    QCM_MMS_RES_ERROR_PARAM_BCCADDR_FULL = 28 | OSA_ERRCODE_MMS_BASE,
    QCM_MMS_RES_ERROR_PARAM_BODYFILE_FULL = 29 | OSA_ERRCODE_MMS_BASE,
    QCM_MMS_RES_ERROR_URL_LENGTHERROR = 30 | OSA_ERRCODE_MMS_BASE,
    QCM_MMS_RES_ERROR_PDP_ACTIVE_FAIL = 31 | OSA_ERRCODE_MMS_BASE,
    QCM_MMS_RES_ERROR_DNS_FAIL = 32 | OSA_ERRCODE_MMS_BASE,
} qcm_mms_eercode_e;

/**
 * @brief
 */
typedef enum
{
    QCM_MMS_DELETEL_ALL_MODE = 0,
    QCM_MMS_TO_ADDR_MODE = 1,
    QCM_MMS_CC_ADDR_MODE = 2,
    QCM_MMS_BCC_ADDR_MODE = 3,
    QCM_MMS_TITEL_MODE = 4,
    QCM_MMS_ATTACHMENT_MODE = 5,
} qcm_mms_edit_mode_e;

typedef enum
{
    MMS_PARAM_VALID_1HOUR = 0,
    MMS_PARAM_VALID_12HOURS,
    MMS_PARAM_VALID_24HOURS,
    MMS_PARAM_VALID_2DAYS,
    MMS_PARAM_VALID_1WEEK,
    MMS_PARAM_VALID_MAXIMUM,
    MMS_PARAM_VALID_DEFAULT
} mms_valid_type;

typedef enum
{
    MMS_PARAM_PRIORITY_LOWEST = 0,
    MMS_PARAM_PRIORITY_MORMAL,
    MMS_PARAM_PRIORITY_HIGHEST,
    MMS_PARAM_PRIORITY_DEFAULT
} mms_priority_type;

typedef enum
{
    MMS_PARAM_SENDERADDR_HIDE = 0,
    MMS_PARAM_SENDERADDR_SHOW,
    MMS_PARAM_SENDERADDR_DEFAULT
} mms_visible_type;

typedef enum
{
    MMS_PARAM_CLASS_PERSONAL = 0,
    MMS_PARAM_CLASS_ADVERTISEMENT,
    MMS_PARAM_CLASS_INFORMATIONAL,
    MMS_PARAM_CLASS_AUTO,
    MMS_PARAM_CLASS_DEFAULT
} mms_class_type;

typedef enum
{
    ENCODE_END = -1,
    ENCODE_MessageType = 0, /*send req step*/
    ENCODE_Transaction_ID,
    ENCODE_Version,
    ENCODE_Date,
    ENCODE_From,
    ENCODE_To,
    ENCODE_Cc,
    ENCODE_Bcc,
    ENCODE_Subject,
    ENCODE_Message_Class,
    ENCODE_MMS_Expiry,
    ENCODE_MMS_Delivery_Time,
    ENCODE_MMS_Priority,
    ENCODE_MMS_Sender_Visibility,
    ENCODE_MMS_Delivery_Report,
    ENCODE_Mms_Read_Report,
    ENCODE_Content_Type,
    ENCODE_BODY_Context_Type, /*body step*/
    ENCODE_BODY_Context_Location,
    ENCODE_BODY_Context_ID,
    ENCODE_BODY_Data,
    ENCODE_Status_value,
    NUM_OF_ENCODE
} mms_header_id_type;

typedef struct
{
    const char   *charsetname;
    qosa_uint16_t charsetvalue;
    qosa_bool_t   tail00;
} mms_charset_type;

typedef struct
{
    const char  *file_ext;
    const char  *mime_string;
    qosa_uint8_t filetype;
    qosa_uint8_t well_known_media;
} mms_file_mime_t;

typedef enum
{
    MMS_RESPONSE_OK = 0x80,
    MMS_RESPOSNE_ERR_UNSPECIFIED = 0x81,
    MMS_RESPONSE_ERR_DENIED = 0x82,
    MMS_RESPONSE_ERR_MMS_FORMAT = 0x83,
    MMS_RESPONSE_ERR_MMS_ADDRESS = 0x84,
    MMS_RESPONSE_ERR_NOT_FOUND = 0x85,
    MMS_RESPONSE_ERR_NETWORK = 0x86,
    MMS_RESPONSE_ERR_CONTENT = 0x87,
    MMS_RESPONSE_ERR_UNSUPPORTED = 0x88
} mms_response_status_type;

/**
 * @enum: qcm_mms_address_type_e.
 * @brief: Address type of an MMS recipient.
 */
typedef enum
{
    QCM_MMS_ADDRESS_TYPE_INVALID = 0,
    QCM_MMS_ADDRESS_TYPE_NUMBER  = 1,
    QCM_MMS_ADDRESS_TYPE_EMAIL   = 2,
    QCM_MMS_ADDRESS_TYPE_OTHER   = 3
} qcm_mms_address_type_e;

/**
 * @struct: qcm_mms_address_t.
 * @brief: Node stored in the recipient address list of qcm_mms_common_cfg.
 */
typedef struct qcm_mms_address_s
{
    qosa_q_link_type_t     link;
    char                   address[QCM_MMS_ADDRESS_TRANS_MAX_LENGTH + 1];
    qosa_uint16_t          addresslength;
    qcm_mms_address_type_e addresstype;
} qcm_mms_address_t;

/**
 * @struct: qcm_mms_body_t.
 * @brief:
 */
typedef struct qcm_mms_body_s
{
    qosa_q_link_type_t link;
    qosa_uint32_t      data_length;
    qosa_uint8_t       context_type;
    char               context_type_Text[QCM_MMS_CONTENTTYPETEXT_MAX_LENGTH + 1];
    char               sendfilenamefullpath[QCM_MMS_SENDFILE_MAX_FULLPATHLENGTH + 1];
    char               sendfilename[QCM_MMS_SENDFILE_MAX_NAMELENGTH + 1];
    qosa_uint16_t      charset;
    char               content_location[QCM_MMS_CONTENTLOCATION_MAX_LENGTH + 1];
    qosa_uint8_t       content_id[QCM_MMS_CONTENTLOCATION_MAX_LENGTH + 1];
} qcm_mms_body_t;

/**
 * @brief Status Return Notification
 */
typedef struct
{
    qcm_mms_eercode_e result;             /*!< Command word execution result status return*/
    qosa_uint16_t     pocotrol_code;      /*!< Protocol return result code*/
    qosa_uint8_t      mms_error_msg[256]; /*!< String returned by MMSC*/
} qcm_mms_comm_resp_t;

typedef void (*qcm_mms_event_cb)(qcm_mms_comm_resp_t *evt_param);

typedef struct
{
    qosa_uint8_t  subject[QCM_MMS_SUBJECT_MAX_LENGTH + 1];
    qosa_uint16_t subject_charset; /*!< The character type when sending MMS titles.*/
    qosa_uint32_t subject_length;
} qcm_mms_subject;

typedef struct
{
    qosa_int32_t year;
    qosa_int32_t month;
    qosa_int32_t day;
    qosa_int32_t hour;
    qosa_int32_t minute;
    qosa_int32_t second;
} mms_date;

/**
 * @struct: qcm_mms_attachment_info_t.
 * @brief:
 */
typedef struct
{
    qosa_int32_t result;
    qosa_uint8_t attachment[256];
} qcm_mms_attachment_info_t;

/**
 * @struct: qcm_mms_addr_info_t.
 * @brief:
 */
typedef struct
{
    qcm_mms_edit_mode_e addr_type;
    qosa_uint8_t        addr[128];
} qcm_mms_addr_info_t;

typedef struct
{
    qosa_uint16_t encodestep;
    qosa_int32_t *encode_arrange;
    qosa_size_t   param1;
    qosa_int32_t  param2;
    qosa_bool_t   encode_end;
    qosa_uint32_t sending_file;
} qcm_mms_encode_info_t;

typedef struct
{
    qosa_uint8_t  pdp_cid;                                 /*!< Number of PDP channels used.*/
    qosa_uint32_t connect_time;                            /*!< Maximum timeout for PDP activation.*/
    qosa_uint8_t  mms_url[CONFIG_QOSA_HOST_URL_MAX_LEN];   /*!< The URL name of MMSC.*/
    qosa_uint8_t  mms_proxy[CONFIG_QOSA_HOST_URL_MAX_LEN]; /*!< MMS proxy gateway.*/
    qosa_uint16_t mms_proxy_port;                          /*!< The port number of the MMS proxy gateway.*/
    qosa_uint16_t mms_charset;                             /*!< The character type of MMS messages.*/
    qosa_uint8_t  validity_period;                         /*!< Validity period of the MMS message.*/
    qosa_uint8_t  priority;                                /*!< Priority of MMS messages.*/
    qosa_bool_t   is_send_rpt;                             /*!< Whether to provide a report.*/
    qosa_bool_t   is_read_rpt;                             /*!< Whether to read the report.*/
    qosa_uint8_t  visible;                                 /*!< Hidden address.*/
    qosa_uint8_t class;                                    /*!< The category of MMS messages.*/
    qosa_uint32_t   send_timeout;                          /*!< Maximum timeout for sending MMS messages.*/
    qosa_uint16_t   maxpktlen;
    qosa_bool_t     is_add_x_up_calling_id;
    qosa_q_type_t   to_addr_list;
    qosa_q_type_t   cc_addr_list;
    qosa_q_type_t   bcc_addr_list;
    qosa_q_type_t   body_list;
    qcm_mms_subject subject;
    qosa_mutex_t    mutex;
} qcm_mms_common_cfg;

/**
 * @brief
 *
 * @param[in] mms_user_config
 * @return qcm_mms_eercode_e
 */
qcm_mms_eercode_e qcm_mms_default_config(qcm_mms_common_cfg *mms_user_config);

/**
 * @brief
 *
 * @param[in] mms_user_config
 * @param[in] function
 * @param[in] optstring
 * @return qcm_mms_eercode_e
 */
qcm_mms_eercode_e qcm_mms_add_msg_addr(qcm_mms_common_cfg *mms_user_config, qcm_mms_edit_mode_e function, char *optstring);

/**
 * @brief
 *
 * @param[in] mms_user_config
 * @param[in] optstring
 * @return qcm_mms_eercode_e
 */
qcm_mms_eercode_e qcm_mms_add_msg_title(qcm_mms_common_cfg *mms_user_config, char *optstring);

/**
 * @brief
 *
 * @param[in] mms_user_config
 * @param[in] optstring
 * @return qcm_mms_eercode_e
 */
qcm_mms_eercode_e qcm_mms_add_msg_attachment(qcm_mms_common_cfg *mms_user_config, char *optstring);

/**
 *  @brief
 *
 * @param[in] mms_user_config
 * @param[in] function
 * @return qcm_mms_eercode_e
 */
qcm_mms_eercode_e qcm_mms_delete_msg_handle(qcm_mms_common_cfg *mms_user_config, qcm_mms_edit_mode_e function);

/**
 * @brief Send the configured MMS message.
 *
 * @param[in] mms_user_config
 * @param[in] event_cb
 * @return qcm_mms_eercode_e
 */
qcm_mms_eercode_e qcm_mms_send(qcm_mms_common_cfg *mms_user_config, qcm_mms_event_cb event_cb);

/**
  * @brief
  *
  * @param[in] charset_name
  * @param[out] charset_val
  * @return qosa_int32_t
  */
qosa_int32_t qcm_mms_get_charset_val(char *charset_name, qosa_uint16_t *charset_val);

/**
  * @brief
  *
  * @param[in] charset
  * @param[out] charset_name
  * @return qosa_int32_t
  */
qosa_int32_t qcm_mms_get_charset_name(qosa_uint16_t charset, qosa_uint8_t **charset_name);

/**
  * @brief
  *
  * @param[in] hex_value
  * @param[in] hex_str
  * @param[in] max_output_len
  * @return qosa_int32_t
  */
qosa_int32_t qcm_mms_convert_hex_value_to_str(char *hex_value, char *hex_str, qosa_uint32_t max_output_len);

#endif /* __QCM_MMS_H__ */
