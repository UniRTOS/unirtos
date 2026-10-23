/*****************************************************************/ /**
* @file qosa_sms.h
* @brief SMS operation interface
* @author Joe.tu@quectel.com
* @date 2024-06-12
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2024-06-12 <td>1.0 <td>Joe.tu <td> Init
* </table>
**********************************************************************/

#ifndef __QOSA_SMS_H__
#define __QOSA_SMS_H__

#include "qosa_def.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#include "qosa_modem.h"

/** SMS component error code base value, used to generate SMS-related error codes */
#define QOSA_ERRCODE_SMS_BASE            (QOSA_COMPONENT_SMS << 16)
/** Maximum number of segments for long SMS */
#define MAX_LONGSMS_SEGMENT              8
/** Maximum length of single SMS */
#define MAX_SMS_LENGTH                   (160 * 4)
/** SMS buffer length */
#define SMS_BUF_LEN                      (MAX_SMS_LENGTH + 32)
/** Maximum length of long SMS */
#define MAX_LONGSMS_LENGTH               (160 * 4 * MAX_LONGSMS_SEGMENT)
/** Maximum address length */
#define QOSA_ADDRESS_MAX_LEN             21
/** Maximum PDU length */
#define QOSA_SMS_PDU_MAX_LEN             176
/** Maximum number of user data bytes that can be carried in SMS TPDU */
#define QOSA_SMS_USER_DATA_LEN           140
/** Maximum number of characters that can be carried in SMS TPDU */
#define QOSA_SMS_MAX_USER_VALID_DATA_LEN 160

/** Get sca field data length in PDU SMS */
#define QOSA_SMS_GET_SCA_PART_LEN(pdu)   ((pdu)[0] ? (1 + (pdu)[0]) : 1)

/** Parse first byte information in TPDU data */
#define QOSA_SMS_FO(pdu)                 ((pdu)->data[(pdu)->data[0] ? (pdu)->data[0] + 1 : 1])
/** TP-Message-Type-Indicator */
#define QOSA_SMS_MTI(pdu)                (QOSA_SMS_FO(pdu) & 0x03)  // 0000 0011
/** TP-Reject-Duplicates */
#define QOSA_SMS_RD(pdu)                 (QOSA_SMS_FO(pdu) & 0x04)  // 0000 0100
/** TP-Validity-Period-Format */
#define QOSA_SMS_VPF(pdu)                (QOSA_SMS_FO(pdu) & 0x18)  // 0001 1000
/** TP-Status-Report-Request */
#define QOSA_SMS_SRR(pdu)                (QOSA_SMS_FO(pdu) & 0x20)  // 0010 0000
/** TP-User-Data-Header-Indicator */
#define QOSA_SMS_UHDI(pdu)               (QOSA_SMS_FO(pdu) & 0x40)  // 0100 0000
/** TP-Reply-Path */
#define QOSA_SMS_RP(pdu)                 (QOSA_SMS_FO(pdu) & 0x80)  // 1000 0000

/** Extract SMS class from DCS */
#define QOSA_SMS_CLASS(dcs)              ((dcs)&0x03)
/** Extract SMS alphabet from DCS */
#define QOSA_SMS_ALPHABET(dcs)           (((dcs) >> 2) & 0x03)

/** SMS callback function type definition */
typedef void (*sms_callback_t)(void *ctx, void *argv);

// clang-format off
typedef enum
{
    QOSA_SMS_SUCCESS = 0,
    QOSA_SMS_CMS_UNASSIGNED_NUM       = 1 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_OPER_DETERM_BARR     = 8 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_CALL_BARRED          = 10 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_SM_TRANS_REJE        = 21 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_DEST_OOS             = 27 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_UNINDENT_SUB         = 28 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_FACILIT_REJE         = 29 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_UNKNOWN_SUB          = 30 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_NW_OOO               = 38 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_TEMP_FAIL            = 41 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_CONGESTION           = 42 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_RES_UNAVAILABLE      = 47 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_REQ_FAC_NOT_SUB      = 50 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_RFQ_FAC_NOT_IMP      = 69 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_INVALID_SM_TRV       = 81 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_INVALID_MSG          = 95 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_INVALID_MAND_INFO    = 96 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_MSG_TYPE_ERROR       = 97 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_MSG_NOT_COMP         = 98 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_INFO_ELEMENT_ERROR   = 99 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_PROT_ERROR           = 111 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_IW_UNSPEC            = 127 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_TEL_IW_NOT_SUPP      = 128 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_SMS_TYPE0_NOT_SUPP   = 129 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_CANNOT_REP_SMS       = 130 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_UNSPEC_TP_ERROR      = 143 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_DCS_NOT_SUPP         = 144 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_MSG_CLASS_NOT_SUPP   = 145 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_UNSPEC_TD_ERROR      = 159 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_CMD_CANNOT_ACT       = 160 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_CMD_UNSUPP           = 161 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_UNSPEC_TC_ERROR      = 175 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_TPDU_NOT_SUPP        = 176 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_SC_BUSY              = 192 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_NO_SC_SUB            = 193 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_SC_SYS_FAIL          = 194 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_INVALID_SME_ADDR     = 195 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_DEST_SME_BARR        = 196 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_SM_RD_SM             = 197 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_TP_VPF_NOT_SUPP      = 198 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_TP_VP_NOT_SUPP       = 199 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_D0_SIM_SMS_STO_FULL  = 208 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_NO_SMS_STO_IN_SIM    = 209 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_ERR_IN_MS            = 210 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_MEM_CAP_EXCEEDED     = 211 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_SIM_APP_TK_BUSY      = 212 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_SIM_DATA_DL_ERROR    = 213 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_UNSPEC_ERROR_CAUSE   = 255 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_ME_FAIL              = 300 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_SMS_SERVICE_RESERVED = 301 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_OPER_NOT_ALLOWED     = 302 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_OPER_NOT_SUPP        = 303 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_INVALID_PDU_PARAM    = 304 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_INVALID_TXT_PARAM    = 305 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_SIM_NOT_INSERT       = 310 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_SIM_PIN_REQUIRED     = 311 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_PH_SIM_PIN_REQUIRED  = 312 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_SIM_FAIL             = 313 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_SIM_BUSY             = 314 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_SIM_WRONG            = 315 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_SIM_PUK_REQUIRED     = 316 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_SIM_PIN2_REQUIRED    = 317 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_SIM_PUK2_REQUIRED    = 318 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_MEM_FAIL             = 320 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_INVALID_MEM_INDEX    = 321 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_MEM_FULL             = 322 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_SCA_ADDR_UNKNOWN     = 330 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_NO_NW_SERVICE        = 331 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_NW_TIMEOUT           = 332 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_NO_CNMA_ACK_EXPECTED = 340 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_UNKNOWN_ERROR        = 500 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_USER_ABORT           = 512 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_UNABLE_TO_STORE      = 513 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_INVALID_STATUS       = 514 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_INVALID_ADDR_CHAR    = 515 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_INVALID_LEN          = 516 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_INVALID_PDU_CHAR     = 517 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_INVALID_PARA         = 518 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_INVALID_LEN_OR_CHAR  = 519 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_INVALID_TXT_CHAR     = 520 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_TIMER_EXPIRED        = 521 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),
    QOSA_SMS_CMS_SMS_SEND_FAIL        = 530 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_CMS_OFS),

    QOSA_SMS_ERROR                    = 1 | (QOSA_ERRCODE_SMS_BASE + QOSA_AT_ERR_OFS),
    QOSA_SMS_NOT_INIT_ERR,
    // QOSA_SMS_SEM_CREATE_ERR,
    // QOSA_SMS_SEM_TIMEOUT_ERR,
    QOSA_SMS_NO_MSG_ERR,
} qosa_sms_err_e;
// clang-format on

/**
 * @enum qosa_sms_init_status_e
 * @brief sms init status
 *
 */
typedef enum
{
    QOSA_SMS_INIT_STATUS_NOT_READY = 0, /*!< not ready */
    QOSA_SMS_INIT_STATUS_READY = 1,     /*!< ready */
} qosa_sms_init_status_e;

/**
 * @enum qosa_sms_stor_e
 * @brief storage type of sms
 *
 */
typedef enum
{
    QOSA_SMS_STOR_ME = 0, /*!< Storage in ME */
    QOSA_SMS_STOR_SM = 1, /*!< Storage in SIM card */
} qosa_sms_stor_e;

/**
 * @enum qosa_sms_status_e
 * @brief status of sms
 *
 */
typedef enum
{
    QOSA_SMS_UNREAD = 0, /*!< Received but unread message */
    QOSA_SMS_READ = 1,   /*!< Received and read message */
    QOSA_SMS_UNSENT = 2, /*!< Stored but unsent message */
    QOSA_SMS_SENT = 3,   /*!< Stored and sent message*/
    QOSA_SMS_ALL = 4,    /*!< All messages */
} qosa_sms_status_e;

/**
 * @enum qosa_sms_msg_type_e
 * @brief message type of sms
 *
 */
typedef enum
{
    QOSA_SMS_DELIVER = 0,       /*!< Mobile terminated message */
    QOSA_SMS_SUBMIT = 1,        /*!< Mobile originated message */
    QOSA_SMS_STATUS_REPORT = 2, /*!< Status Report message */
} qosa_sms_msg_type_e;

/**
 * @enum qosa_sms_select_type_e
 * @brief record selection method
 *
 */
typedef enum
{
    QOSA_SMS_SELECT_BY_INDEX,  /*!< Select by record index */
    QOSA_SMS_SELECT_BY_STATUS, /*!< Select by record status */
} qosa_sms_select_type_e;

/**
 * @enum qosa_sms_alphabet_e
 * @brief SMS character set type
 */
typedef enum
{
    QOSA_SMS_ALPHA_7BIT = 0, /*!< 7-bit alphabet */
    QOSA_SMS_ALPHA_8BIT,     /*!< 8 bit data */
    QOSA_SMS_ALPHA_16BIT,    /*!< UCS2 (16bit) */
} qosa_sms_alphabet_e;

/**
 * @enum qosa_sms_delflag_e
 * @brief delete flag of sms
 *
 */
typedef enum
{
    QOSA_SMS_DEL_INDEX = 0,          /*!< Delete the message specified in <index> */
    QOSA_SMS_DEL_RECV_READ = 1,      /*!< Delete all read messages from <mem1> storage */
    QOSA_SMS_DEL_RECV_READ_SENT = 2, /*!< Delete all read messages from <mem1> storage and sent mobile originated messages */
    QOSA_SMS_DEL_RECV_READ_MO = 3,   /*!< Delete all read messages from <mem1> storage, sent and unsent mobile originated messages */
    QOSA_SMS_DEL_ALL = 4,            /*!< Delete all messages from <mem1> storage */
} qosa_sms_delflag_e;

/**
 * @enum qosa_sms_rp_type_e
 * @brief New message reply type
 *
 */
typedef enum
{
    QOSA_SMS_RP_ACK = 0, /*!< Acknowledgment reply */
    QOSA_SMS_RP_ERROR,   /*!< Error reply */
} qosa_sms_rp_type_e;

/**
 * @enum qosa_sms_more_msg_mode_e
 * @brief More Messages to Send (CMMS) mode
 *
 */
typedef enum
{
    QOSA_SMS_MORE_MSG_MODE_DISABLE = 0,    /*!< Disable More Messages to Send (CMMS) */
    QOSA_SMS_MORE_MSG_MODE_ONESHOT = 1,    /*!< One-shot mode (CMMS) */
    QOSA_SMS_MORE_MSG_MODE_CONTINUOUS = 2, /*!< Continuous mode (CMMS) */
} qosa_sms_more_msg_mode_e;

/*-------------------------------------- pa  --------------------------------------*/
/**
 * @struct qosa_sms_cfg_t
 * @brief SMS configuration parameter structure for managing AT command parameters
 *
 * This structure encapsulates SMS-related AT command configuration parameters
 * defined in the 3GPP TS 27.005 standard, including storage management,
 * new message indication, and service mode selection.
 */
typedef struct
{
    // CPMS parameters
    qosa_uint8_t mem1; /*!< Memory 1, memory from which messages are read or deleted */
    qosa_uint8_t mem2; /*!< Memory 2, memory to which writing and sending operations are performed */
    qosa_uint8_t mem3; /*!< Memory 3, memory to which received messages are stored */

    // CNMI parameters
    qosa_uint8_t nmi_mode; /*!< NMI mode */
    qosa_uint8_t nmi_mt;   /*!< NMI mt */
    qosa_uint8_t nmi_bm;   /*!< NMI bm */
    qosa_uint8_t nmi_ds;   /*!< NMI ds */
    qosa_uint8_t nmi_bfr;  /*!< NMI bfr */

    // CSMS parameters
    qosa_uint8_t service_mode; /*!< messaging service */

    // CSMP parameters
    qosa_uint8_t text_fo;  /*!< First Octet in Text Mode */
    qosa_uint8_t text_pid; /*!< Protocol Identifier in Text Mode */
    qosa_uint8_t text_dcs; /*!< Data Coding Scheme in Text Mode */
    qosa_uint8_t text_vp;  /*!< Validity Period in Text Mode */
} qosa_sms_cfg_t;

/**
 * @struct qosa_sms_stor_info_t
 * @brief Get the storage information of SMS.
 *
 */
typedef struct
{
    qosa_uint16_t used_sm;  /*!< Used storage in SIM card */
    qosa_uint16_t total_sm; /*!< Total storage in SIM card */
    qosa_uint16_t used_me;  /*!< Used storage in ME */
    qosa_uint16_t total_me; /*!< Total storage in ME */
} qosa_sms_stor_info_t;

/**
 * @struct qosa_sms_time_stamp_t
 * @brief Time stamp of SMS.
 *
 */
typedef struct
{
    qosa_uint8_t year;   /*!< Year */
    qosa_uint8_t month;  /*!< Month */
    qosa_uint8_t day;    /*!< Day */
    qosa_uint8_t hour;   /*!< Hour */
    qosa_uint8_t minute; /*!< Minute */
    qosa_uint8_t second; /*!< Second */
    qosa_int8_t  zone;   /*!< Time zone */
} qosa_sms_time_stamp_t;

/**
 * @struct qosa_sms_send_t
 * @brief data structure for SMS message(SUBMIT)
 *
 */
typedef struct
{
    qosa_uint16_t     index;                          /*!< Index in storage */
    qosa_sms_status_e status;                         /*!< Status */

    char         sca[QOSA_ADDRESS_MAX_LEN * 4 + 1];   /*!< Service center address */
    qosa_uint8_t tosca;                               /*!< Type of service center address */
    qosa_uint8_t fo;                                  /*!< First octet of 3GPP TS 23.040 SMS-SUBMIT (default 17) */
    char         da[QOSA_ADDRESS_MAX_LEN * 4 + 1];    /*!< Destination address */
    qosa_uint8_t toda;                                /*!< Type of destination address */
    qosa_uint8_t pid;                                 /*!< Protocol identifier */
    qosa_uint8_t dcs;                                 /*!< Data coding scheme */
    qosa_uint8_t vp;                                  /*!< Validity period */

    qosa_uint16_t data_len;                           /*!< number of data bytes */
    qosa_uint8_t  data[QOSA_SMS_PDU_MAX_LEN * 4 + 1]; /*!< Data */
    qosa_uint8_t  data_chset;                         /*!< character set of data */
} qosa_sms_send_t;

/**
 * @struct qosa_sms_recv_t
 * @brief Received SMS data structure
 */
typedef struct
{
    qosa_uint16_t     index;                                 /*!< Index position in storage */
    qosa_sms_status_e status;                                /*!< SMS status */

    char                  sca[QOSA_ADDRESS_MAX_LEN * 4 + 1]; /*!< Service center address */
    qosa_uint8_t          tosca;                             /*!< Service center address type */
    qosa_uint8_t          fo;                                /*!< PDU first octet */
    char                  oa[QOSA_ADDRESS_MAX_LEN * 4 + 1];  /*!< Source address */
    qosa_uint8_t          tooa;                              /*!< Source address type */
    qosa_uint8_t          pid;                               /*!< Protocol identifier */
    qosa_uint8_t          dcs;                               /*!< Data coding scheme */
    qosa_sms_time_stamp_t scts;                              /*!< Service center timestamp */

    qosa_uint16_t data_len;                                  /*!< Data length: number of data bytes in text mode, TPDU byte length in PDU mode */
    qosa_uint8_t  data[QOSA_SMS_PDU_MAX_LEN * 4 + 1]; /*!< Data: contains all PDU data in PDU mode, only contains the SMS itself in TP-user data in text mode */
    qosa_uint8_t  data_chset;                         /*!< Character set: data character set in text mode, TPDU character set in PDU mode */
} qosa_sms_recv_t;

/**
 * @struct qosa_sms_rept_t
 * @brief SMS status report structure
 */
typedef struct
{
    qosa_uint16_t     index;                                 /*!< Index position in storage */
    qosa_sms_status_e status;                                /*!< SMS status */

    char                  sca[QOSA_ADDRESS_MAX_LEN * 4 + 1]; /*!< Service center address */
    qosa_uint8_t          tosca;                             /*!< Service center address type */
    qosa_uint8_t          fo;                                /*!< 3GPP TS 23.040 SMS-SUBMIT first octet (default 17) */
    qosa_uint8_t          mr;                                /*!< Message reference number */
    char                  ra[QOSA_ADDRESS_MAX_LEN * 4 + 1];  /*!< Recipient address */
    qosa_uint8_t          tora;                              /*!< Recipient address type */
    qosa_sms_time_stamp_t scts;                              /*!< Service center timestamp */
    qosa_sms_time_stamp_t dt;                                /*!< TP-Discharge-Time time string format */
    qosa_uint8_t          st;                                /*!< 3GPP TS 23.040 TP-Status integer format */
} qosa_sms_rept_t;

/**
 * @struct qosa_sms_concat_t
 * @brief SMS concatenation information structure
 */
typedef struct
{
    qosa_uint16_t msg_ref_number; /*!< Message Reference Number, only used if is_concatenated is true */
    qosa_uint8_t  msg_seg;        /*!< Message Segment Number, only used if is_concatenated is true */
    qosa_uint8_t  msg_total;      /*!< Total Number of Segments, only used if is_concatenated is true */
} qosa_sms_concat_t;

/**
 * @struct qosa_sms_msg_t
 * @brief SMS message structure
 */
typedef struct
{
    qosa_sms_msg_type_e msg_type; /*!< Message type */
    union
    {
        qosa_sms_send_t send;          /*!< Send message structure */
        qosa_sms_recv_t recv;          /*!< Receive message structure */
        qosa_sms_rept_t report;        /*!< Status report structure */
    } text;                            /*!< Text message union */

    qosa_bool_t       is_concatenated; /*!< Indicates if the message is concatenated */
    qosa_sms_concat_t concat;          /*!< Concatenation information */
} qosa_sms_msg_t;

typedef struct
{
    qosa_uint8_t data_len;                   /*!< Overall TPDU data byte count, includes SCA, other lengths calculated through conversion */
    qosa_uint8_t data[QOSA_SMS_PDU_MAX_LEN]; /*!< SCA with TPDU */
} qosa_sms_pdu_t;

typedef struct
{
    qosa_uint16_t       index;    /*!< index in storage */
    qosa_sms_status_e   status;   /*!< status of the message */
    qosa_sms_msg_type_e msg_type; /*!< message type */
    qosa_sms_pdu_t      pdu;      /*!< PDU data */
} qosa_sms_record_t;

/**
 * @struct qosa_sms_address_info_t
 * @brief SMS address structure
 *
 */
typedef struct
{
    qosa_uint8_t address_type;                         /*!<  Address Type Information */
    qosa_uint8_t address_len;                          /*!<  Address length, number of digits in number */
    qosa_uint8_t address_number[QOSA_ADDRESS_MAX_LEN]; /*!<  Address number "13512345678" */
} qosa_sms_address_info_t;

/**
 * @struct qosa_sms_send_param_t
 * @brief SMS sending parameter structure
 *
 */
typedef struct
{
    qosa_sms_pdu_t pdu; /*!< PDU data */
} qosa_sms_send_param_t;

/**
 * @struct qosa_sms_send_spec_param_t
 * @brief Parameter structure for sending SMS from specified storage.
 *
 */
typedef struct
{
    qosa_sms_stor_e         stor;  /*!< Storage to send from */
    qosa_uint16_t           index; /*!< Index of the message to send, Zero-indexed */
    qosa_sms_address_info_t da;    /*!< Destination address */
} qosa_sms_send_spec_param_t;

/**
 * @struct qosa_sms_write_param_t
 * @brief Parameter structure for SMS write operation.
 *
 */
typedef struct
{
    qosa_sms_stor_e   stor;   /*!< Storage to write to */
    qosa_sms_status_e status; /*!< status of the message */
    qosa_sms_pdu_t    pdu;    /*!< PDU data */
} qosa_sms_write_param_t;

/**
 * @struct qosa_sms_read_param_t
 * @brief Parameter structure for SMS read operation.
 *
 */
typedef struct
{
    qosa_sms_stor_e stor;  /*!< Storage to read from */
    qosa_uint16_t   index; /*!< Index of the message to read, Zero-indexed */
} qosa_sms_read_param_t;

/**
 * @struct qosa_sms_delete_param_t
 * @brief param for delete sms record
 *
 */
typedef struct
{
    qosa_sms_stor_e        stor;          /*!< Storage to delete from */
    qosa_sms_select_type_e index_or_stat; /*!< Index or status of the message to delete */
    qosa_uint16_t          index;         /*!< Index of the message to delete, Zero-indexed, only used if index_or_stat is QOSA_SMS_SELECT_BY_INDEX */
    qosa_sms_delflag_e     delflag;       /*!< Status of the message to delete, only used if index_or_stat is QOSA_SMS_SELECT_BY_STATUS */
} qosa_sms_delete_param_t;

/*-------------------------------------- event --------------------------------------*/

/**
 * @struct qosa_sms_init_status_event_t
 * @brief SMS module initialization status event parameters
 *
 */
typedef struct
{
    qosa_uint8_t           simid;  /*!< SIM card ID */
    qosa_sms_init_status_e status; /*!< Whether SMS module is initialized OK, 0:not ready 1:ready */
} qosa_sms_init_status_event_t;

/**
 * @struct qosa_sms_new_msg_event_t
 *      PDU invalid, data_len is 0:
 *                          record `index`, `msg_type` parameters are valid, storage parameter is valid
 *      index parameter invalid, value is 0xffff:
 *                          record `msg_type` parameter is valid, storage parameter is valid
 *
 *
 * @brief Data content carried by event QOSA_EVENT_MODEM_SMS_NEW_MSG report
 *
 */
typedef struct
{
    qosa_uint8_t      simid;   /*!< SIM card ID */
    qosa_sms_stor_e   storage; /*!< Storage type */
    qosa_sms_record_t record;  /*!< SMS record, if record index is 0xFFFF, it means the SMS was not stored, storage field is invalid */
} qosa_sms_new_msg_event_t;

/**
 * @struct qosa_sms_storage_full_event_t
 * @brief Data content carried by event QOSA_EVENT_MODEM_SMS_STORAGE_FULL report
 *
 */
typedef struct
{
    qosa_uint8_t    simid;   /*!< SIM card ID */
    qosa_sms_stor_e storage; /*!< Storage type */
} qosa_sms_storage_full_event_t;

/*-------------------------------------- cnf --------------------------------------*/

/* All cnf results need to ensure the first two parameters of the structure are simid and err_code in order, which is OSA_MODEM_COMMON_CNF_HDR, user layer does not need to release parameter content, after executing the callback, internally it will be released directly */

/**
 * @struct qosa_sms_general_cnf_t
 * @brief SMS general result return, used to return simple success or failure, no additional result return type
 *
 */
typedef qosa_modem_general_cnf_t qosa_sms_general_cnf_t;

/**
 * @struct qosa_sms_send_pdu_cnf_t
 * @brief the result of qosa_sms_send_pdu_async API
 *
 */
typedef struct
{
    QOSA_MODEM_COMMON_CNF_HDR; /*!< common header, simid and err_code */
    qosa_uint8_t mr;           /*!< Message Reference */
} qosa_sms_send_pdu_cnf_t;

/**
 * @struct qosa_sms_send_spec_pdu_cnf_t
 * @brief the result of qosa_sms_send_spec_pdu_async API
 *
 */
typedef struct
{
    QOSA_MODEM_COMMON_CNF_HDR; /*!< common header, simid and err_code */
    qosa_uint8_t mr;           /*!< Message Reference */
} qosa_sms_send_spec_pdu_cnf_t;

/**
 * @struct qosa_sms_write_pdu_cnf_t
 * @brief the result of qosa_sms_write_pdu_async API
 *
 */
typedef struct
{
    QOSA_MODEM_COMMON_CNF_HDR; /*!< common header, simid and err_code */
    qosa_uint16_t index;       /*!<  Return the stored SMS index after successful sending */
} qosa_sms_write_pdu_cnf_t;

/**
 * @struct qosa_sms_read_pdu_cnf_t
 * @brief the result of qosa_sms_read_pdu_async API
 *
 */
typedef struct
{
    QOSA_MODEM_COMMON_CNF_HDR; /*!< common header, simid and err_code */
    qosa_sms_record_t record;  /*!< SMS record */
} qosa_sms_read_pdu_cnf_t;

/*-------------------------------------- api --------------------------------------*/

/**
 * @brief Get SMS configuration information
 *
 * @param[in] simid
 *          - sim id
 * @param[out] cfg
 *          - SMS configuration information
 * @return qosa_sms_err_e
 */
qosa_sms_err_e qosa_sms_get_config(qosa_uint8_t simid, qosa_sms_cfg_t *cfg);

/**
 * @brief Set SMS configuration information
 *
 * @param[in] simid
 *          - sim id
 * @param[in] cfg
 *          - SMS configuration information
 * @return qosa_sms_err_e
 */
qosa_sms_err_e qosa_sms_set_config(qosa_uint8_t simid, qosa_sms_cfg_t *cfg);

/**
 * @brief Convert text SMS to PDU SMS
 *
 * @note For SUBMIT messages, FO, PID, DCS and VP fields need to be specified in the msg parameter.
 *
 * @param[in] msg
 *          - Text SMS structure
 * @param[out] record
 *          - SMS structure
 * @return qosa_sms_err_e
 */
qosa_sms_err_e qosa_sms_text_to_pdu(qosa_sms_msg_t *msg, qosa_sms_record_t *record);

/**
 * @brief Convert PDU SMS to text SMS
 *
 * @param[in] record
 *          - SMS structure
 * @param[out] msg
 *          - Text SMS structure
 * @return qosa_sms_err_e
 */
qosa_sms_err_e qosa_sms_pdu_to_text(qosa_sms_record_t *record, qosa_sms_msg_t *msg);

/**
 * @brief Asynchronously send a PDU format SMS.
 *
 * This interface is used to directly send a PDU format SMS without first writing to SMS storage. It returns immediately after calling, and the sending process is notified asynchronously through callback.
 *
 * @param[in] simid
 *          - SIM card number, indicating target card slot (such as SIM1, SIM2, etc.).
 * @param[in] send
 *          - Pointer to `qosa_sms_send_param_t` structure, containing SMS PDU data and storage information used for sending.
 * @param[in] cb
 *          - Asynchronous callback function, called after sending is completed or failed.
 * @param[in] ctx
 *          - User-defined context pointer, will be returned as-is in the callback.
 * @return
 *          - QOSA_SMS_SUCCESS: Sending request has been successfully submitted;
 *          - Other error codes: Such as illegal parameters, resource busy or internal errors.
 * @note
 *          - The destination address (DA) in the `pdu` field must be set correctly;
 *          - Will not write SMS to storage, only used for immediate sending;
 *          - Sending result (success or failure) needs to be confirmed through the callback function.
 */
qosa_sms_err_e qosa_sms_send_pdu_async(qosa_uint8_t simid, qosa_sms_send_param_t *send, sms_callback_t cb, void *ctx);

/**
 * @brief Asynchronously send an existing PDU SMS from specified storage.
 *
 * This interface is used to asynchronously send an existing PDU format SMS from the module's SMS storage area. It returns immediately after calling, and the sending result is notified asynchronously through callback, return result is `qosa_sms_send_spec_pdu_cnf_t`.
 *
 * @param[in] simid
 *          - SIM card number, identifying target card slot.
 * @param[in] send
 *          - Pointer to `qosa_sms_send_spec_param_t` structure, specifying SMS source storage, index position and recipient address.
 * @param[in] cb
 *          - Callback function after sending is completed, user can get the result in the callback.
 * @param[in] ctx
 *          - User-defined context pointer, returned as-is in the callback function.
 * @return
 *          - QOSA_SMS_SUCCESS: Sending request has been successfully submitted;
 *          - Other error codes: Parameter illegal, resource conflict or storage invalid, etc.
 * @note
 *          - This interface is suitable for scenarios where SMS is pre-written and then sent as needed;
 *          - The SMS content corresponding to the storage index must be in legal PDU format;
 *          - If the SMS contains destination address (DA), it will overwrite the original setting with the structure `da`.
 */
qosa_sms_err_e qosa_sms_send_spec_pdu_async(qosa_uint8_t simid, qosa_sms_send_spec_param_t *send, sms_callback_t cb, void *ctx);

/**
 * @brief Asynchronously write a PDU format SMS to specified storage area.
 *
 * This interface is used to asynchronously write a PDU format SMS to the module's SMS storage area. It returns immediately after calling, and the actual writing result is returned asynchronously through the callback function, return result is `qosa_sms_write_pdu_cnf_t`.
 *
 * @param[in] simid
 *          - SIM card number, identifying target card slot.
 * @param[in] write
 *          - Pointer to `qosa_sms_write_param_t` structure, containing write target storage area, SMS status and PDU content.
 * @param[in] cb
 *          - Callback function after write operation is completed, user can handle write results or error information here.
 * @param[in] ctx
 *          - User-defined context pointer, passed through in the callback.
 * @return
 *          - QOSA_SMS_SUCCESS: Write request has been successfully submitted;
 *          - Other error codes: Parameter error or resource unavailable, etc.
 * @note
 *          - PDU data needs to be pre-encoded by the caller;
 *          - Write result and allocated index number are returned in the callback;
 *          - The status after writing depends on the `status` field setting (such as QOSA_SMS_STATUS_UNREAD indicates unread SMS).
 */
qosa_sms_err_e qosa_sms_write_pdu_async(qosa_uint8_t simid, qosa_sms_write_param_t *write, sms_callback_t cb, void *ctx);

/**
 * @brief Asynchronously read SMS at specified index position (PDU format).
 *
 * This function is used to asynchronously read SMS content from the specified storage area. The reading result will be returned through the callback function, return content is PDU encoded format, return result is `qosa_sms_read_pdu_cnf_t`.
 *
 * @param[in] simid
 *          - SIM card number, identifying the card slot used.
 * @param[in] read
 *          - Pointer to `qosa_sms_read_param_t` structure, containing read parameters, such as SMS storage area and index value.
 * @param[in] cb
 *          - Callback function pointer when reading is completed, reading results (including SMS content and status) will be returned asynchronously through this function.
 * @param[in] ctx
 *          - User-defined context pointer, passed back as a callback parameter to facilitate upper layer identification of call context.
 * @return
 *          - QOSA_SMS_SUCCESS: Read request submitted successfully;
 *          - Other error codes: Request submission failed or parameter error.
 * @note
 *          - This function is an asynchronous interface, actual SMS data is returned through the callback function;
 *          - Returned data is in raw PDU format, needs to be parsed by itself.
 */
qosa_sms_err_e qosa_sms_read_pdu_async(qosa_uint8_t simid, qosa_sms_read_param_t *read, sms_callback_t cb, void *ctx);

/**
 * @brief Asynchronously delete specified SMS.
 *
 * This function is used to asynchronously delete SMS on the specified SIM card, can delete in batches by index or status. After deletion is completed, the callback function will be called, return result is `qosa_sms_general_cnf_t`.
 *
 * @param[in] simid
 *          - SIM card number, identifying target card slot.
 * @param[in] delete
 *          - Pointer to `qosa_sms_delete_param_t`, containing deletion parameters, such as storage type, deletion method, index or status, etc.
 * @param[in] cb
 *          - Callback function pointer after deletion is completed, used to notify the caller of operation results.
 * @param[in] ctx
 *          - Callback context pointer, will be passed as-is to the callback function, used to carry user context information.
 * @return
 *          - QOSA_SMS_SUCCESS: Deletion request has been successfully submitted;
 *          - Other error codes: Operation submission failed.
 * @note
 *          - This is an asynchronous operation, results are notified by the callback function.
 *          - Specific deletion rules depend on `delete` parameter settings.
 */
qosa_sms_err_e qosa_sms_delete_async(qosa_uint8_t simid, qosa_sms_delete_param_t *delete, sms_callback_t cb, void *ctx);

/**
 * @brief Get SMS storage usage for specified SIM card.
 *
 * This function is an external interface, calling internal function `qosa_ms_sms_get_stor_info` to get SMS storage information for the specified SIM card,
 * including total capacity and used count for SIM storage (SM) and module internal storage (ME).
 *
 * @param[in] simid
 *          - SIM card number, identifying target card slot.
 * @param[out] info
 *          - Pointer to `qosa_sms_stor_info_t` structure, used to receive query results.
 * @return
 *          - QOSA_SMS_SUCCESS: Query successful;
 *          - QOSA_SMS_NOT_INIT_ERR: SMS module not initialized;
 *          - Other error codes: Query failed.
 * @see qosa_ms_sms_get_stor_info
 */
qosa_sms_err_e qosa_sms_get_stor_info(qosa_uint8_t simid, qosa_sms_stor_info_t *info);

/**
 * @brief Get index list of all used SMS records in specified SIM card and storage.
 *
 * This function is an external interface provided by the SMS module, used to query all used SMS record indexes in the specified SIM card and storage type.
 * The underlying synchronous interface is called to complete the specific query.
 *
 * @param[in] simid
 *          - SIM card number, identifying target card slot.
 * @param[in] storage
 *          - SMS storage type, enumeration type `qosa_sms_stor_e`.
 * @param[out] record_list
 *          - Pre-allocated array, used to store queried SMS record indexes.
 * @param[in,out] size
 *          - Input: `record_list` array capacity;
 *          - Output: Actual number of indexes written.
 * @return
 *          - QOSA_SMS_SUCCESS: Query successful;
 *          - Other error codes: Query failed.
 */
qosa_sms_err_e qosa_sms_get_used_record(qosa_uint8_t simid, qosa_sms_stor_e storage, qosa_uint16_t *record_list, qosa_uint16_t *size);

/**
 * @brief Query SMS record index list with specified status.
 *
 * This function provides a unified interface for filtering SMS records with specified status from the specified SIM card and storage location,
 * and writes their indexes to the array provided by the caller. Commonly used for upper-layer business logic such as SMS list display, status classification filtering, etc.
 *
 * Supports querying from SIM card (SM) or module internal storage (ME). If you need to query all statuses, please use `QOSA_SMS_STATUS_ALL`.
 *
 * @param[in] simid
 *          - SIM card number (usually 0 or 1), used to specify target card slot.
 * @param[in] storage
 *          - SMS storage type, enumeration value `qosa_sms_stor_e`:
 *            - `QOSA_SMS_STOR_SM`: SIM card storage;
 *            - `QOSA_SMS_STOR_ME`: Module internal storage.
 * @param[in] status
 *          - Target SMS status, enumeration value `qosa_sms_status_e`, such as:
 *            - `QOSA_SMS_UNREAD`: Unread;
 *            - `QOSA_SMS_READ`: Read;
 *            - `QOSA_SMS_SENT`: Sent;
 *            - `QOSA_SMS_ALL`: Match all statuses.
 * @param[out] record_list
 *          - Pre-allocated array pointer, used to store matched SMS record indexes (starting index is 0).
 * @param[in,out] size
 *          - Input: Maximum length of `record_list` array;
 *          - Output: Actual number of matched SMS records.
 * @return
 *          - `QOSA_SMS_SUCCESS`: Operation successful;
 *          - Other error codes: Indicates failure, such as invalid parameters, system not initialized, etc.
 * @note
 * -        - Please ensure SMS subsystem is initialized before calling;
 * -        - This function is a synchronous call, does not involve callback or event notification.
 */
qosa_sms_err_e qosa_sms_get_spec_record(qosa_uint8_t simid, qosa_sms_stor_e storage, qosa_sms_status_e status, qosa_uint16_t *record_list, qosa_uint16_t *size);

/**
 * @brief Reply to network side confirming receipt of SMS message
 *
 * @param[in] simid
 *          - sim id
 * @param[in] ack_type
 *          - QOSA_SMS_RP_ACK: Confirm receipt of SMS message, QOSA_SMS_RP_ERROR: Error receiving SMS message
 * @return qosa_sms_err_e
 */
qosa_sms_err_e qosa_sms_new_message_ack(qosa_uint8_t simid, qosa_sms_rp_type_e ack_type);

/**
  * @brief Set SMS center number
  *
  * @param[in] simid
  *          - sim id
  * @param[in] sca
  *          - SMS center number
  * @return qosa_sms_err_e
  */
qosa_sms_err_e qosa_sms_set_sca(qosa_uint8_t simid, qosa_sms_address_info_t *sca);

/**
 * @brief Get SMS center number
 *
 * @param[in] simid
 *          - sim id
 * @param[out] sca
 *          - SMS center number
 * @return qosa_sms_err_e
 */
qosa_sms_err_e qosa_sms_get_sca(qosa_uint8_t simid, qosa_sms_address_info_t *sca);

/**
 * @brief Control the connection maintenance strategy between multiple SMS messages to improve efficiency of continuously sending multiple SMS messages.
 *
 * @param[in] simid
 *          - sim id
 * @param[in] mode
 *          - QOSA_SMS_MORE_MSG_MODE_DISABLE: Disable this feature, close link immediately after sending one message.
 *          - QOSA_SMS_MORE_MSG_MODE_ONESHOT: Enable this feature, link automatically closes when no SMS is sent for a short time (1~5 seconds), and reset state to 0.
 *          - QOSA_SMS_MORE_MSG_MODE_CONTINUOUS: Enable this feature, link automatically closes when no SMS is sent, but state is not automatically reset to 0.
 * @return qosa_sms_err_e
 */
qosa_sms_err_e qosa_sms_set_more_msg_mode(qosa_uint8_t simid, qosa_sms_more_msg_mode_e mode);

/**
 * @brief Get current settings of multiple SMS message connection maintenance strategy.
 *
 * @param[in] simid
 *          - sim id
 * @param[out] mode
 *          - Pointer to output variable, used to return current mode value
 * @return qosa_sms_err_e
 */
qosa_sms_err_e qosa_sms_get_more_msg_mode(qosa_uint8_t simid, qosa_sms_more_msg_mode_e *mode);

/**
 * @brief Convert address information to text format, if it's an international number, '+' will be automatically added before the number, text character encoding is determined by cscs
 *
 * @param[in] address
 *          - the address to be converted to text
 * @param[out] text
 *          - the text representation of the address
 * @param[in] len
 *          - the length of the text buffer
 * @return qosa_sms_err_e
 */
qosa_sms_err_e qosa_sms_address_to_text(qosa_sms_address_info_t *address, char *text, qosa_uint16_t len);

/**
 * @brief Convert a text representation of an address to a binary representation
 *
 * @param[in] text
 *          - the text representation of the address
 * @param[in] len
 *          - the length of the text buffer
 * @param[in] address_type
 *          - the type of the address
 * @param[out] address
 *          - the binary representation of the address
 * @return qosa_sms_err_e
 */
qosa_sms_err_e qosa_sms_text_to_address(char *text, qosa_uint16_t len, qosa_uint8_t address_type, qosa_sms_address_info_t *address);

/**
 * @brief Set SMS character encoding format
 *
 * @param[in] charset
 *          - SMS character encoding format
 * @return qosa_sms_err_e
 */
qosa_sms_err_e qosa_sms_set_charset(qosa_uint8_t charset);

/**
 * @brief  
 *    Convert UTF-8 string to UCS-2 encoded hexadecimal string
 * @param[in] utf8_txt
 *          - UTF-8 encoded input string to be converted
 * @param[out] ucs2_character
 *          - Output buffer for storing UCS-2 hexadecimal string
 * @param[in] buffer_size
 *          - Size of output buffer in bytes
 * @return qosa_sms_err_e
 *          - QOSA_SMS_SUCCESS on successful conversion
 *          - QOSA_SMS_CMS_INVALID_PARA for invalid parameters
 *          - QOSA_SMS_ERROR on conversion failure
 * @note
 *    - Caller must allocate sufficient memory for ucs2_character
 *    - Maximum buffer size required: (strlen(utf8_txt) * 4 + 1) bytes
 *      (Each UCS-2 character requires 4 hexadecimal digits + null terminator)
 */
qosa_sms_err_e qosa_sms_utf8_to_ucs2(const char* utf8_txt, char* ucs2_character, qosa_size_t buffer_size);
#endif// !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif /* __QOSA_SMS_H__ */
