/**********************************************************************
 * @file esim_error.h
 * @brief
 * @author larson.li@quectel.com
 * @date 2024-10-21
 *
 * @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
 * All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
 *
 * @par EDIT HISTORY FOR MODULE
 * <table>
 * <tr><th>Date <th>Version <th>Author <th>Description"
 * <tr><td>2024-10-21 <td>1.0 <td>Larson.Li <td> Init
 * </table>
 **********************************************************************/
#ifndef __ESIM_ERROR_H__
#define __ESIM_ERROR_H__

#include "qosa_def.h"
#include "qosa_sys.h"

/** esim error code base define */
#define OSA_ERRCODE_ESIM_BASE (QOSA_COMPONENT_ESIM << 16)

/**
 * @enum esim_result_errno_e
 * @brief The esim SDK uses generic error codes internally.
 */
typedef enum
{
    ESIM_OK = 0,                                 /*!< ESIM success */
    ESIM_MEMROY_ERR = 1 | OSA_ERRCODE_ESIM_BASE, /*!< ESIM SDK memory error */
    ESIM_INVALID_PARAM_ERR,                      /*!< ESIM parameter error */
    ESIM_AC_CODE_ERR,                            /*!< ESIM AC code error */
    ESIM_OPERATION_TIMEOUT,                      /*!< Operation timeout error */
    ESIM_ERROR_GENERAL,                          /*!< ESIM SDK internal general error */
    ESIM_BUFF_OVERFLOW_ERR,                      /*!< Memory overflow error */
    ESIM_OUT_OF_MEMORY_ERR,                      /*!< Out of memory error */
    ESIM_OPERATION_ERR,                          /*!< Operation error */
    ESIM_MSG_SENT_ERR,                           /*!< Message queue send error */
    ESIM_APDU_SENT_ERR,                          /*!< APDU send error */
    ESIM_APDU_STATUS_ERR,                        /*!< APDU status code error */
    ESIM_APDU_PARSE_ERR,                         /*!< APDU parse error */
    ESIM_TLV_PARSE_ERR,                          /*!< TLV parse error */
    ESIM_JSON_PARSE_ERR,                         /*!< JSON parse error */
    ESIM_FS_OPERATION_ERR,                       /*!< File system operation error */
    ESIM_HTTPS_OPERATION_ERR,                    /*!< HTTPS operation error */
    ESIM_HTTPS_BUSY_ERR,                         /*!< HTTPS busy error */
    ESIM_PROFILE_DOWNLOAD_BUSY_ERR,              /*!< Profile download task busy error */
    ESIM_PROFILE_DOWNLOAD_GET_EUICCINFO1_ERR,    /*!< Profile download get euiccinfo1 error */
    ESIM_PROFILE_DOWNLOAD_GET_CHALLENAGE_ERR,    /*!< Profile download get challenge error */
    ESIM_PROFILE_DOWNLOAD_SEND_HTTPS_ERR,        /*!< Profile download send HTTPS message error */
    ESIM_PROFILE_DOWNLOAD_RESPONSE_HTTPS_ERR,    /*!< Profile download receive HTTPS response message error */
    ESIM_PROFILE_DOWNLOAD_HTTPS_HEAD_ERR,        /*!< Profile download HTTPS head error */
    ESIM_PROFILE_DOWNLOAD_SMDP_RETURN_ERR,       /*!< Profile download SMDP return error */
    ESIM_PROFILE_DOWNLOAD_WAIT_DATA_TIMEOUT_ERR, /*!< Profile download wait data timeout error */
    ESIM_NET_WORK_ERR,                           /*!< Network error */

    ESIM_ENABLE_PROFILE_ERROR_START = 0x100 | OSA_ERRCODE_ESIM_BASE,
    ESIM_ENABLE_ICCID_OR_AID_NOTFOUND = ESIM_ENABLE_PROFILE_ERROR_START | 0x01,         /*!< ENABLE ERROR ICCID or Aid not found */
    ESIM_ENABLE_PROFILE_NOT_IN_DISABLE_STATUS = ESIM_ENABLE_PROFILE_ERROR_START | 0x02, /*!< ENABLE ERROR profile not in disable status */
    ESIM_ENABLE_DISALLOWED_BY_POLICY = ESIM_ENABLE_PROFILE_ERROR_START | 0x03,          /*!< ENABLE ERROR disallowed by policy */
    ESIM_ENABLE_WRONG_PROFILE_REENABLING = ESIM_ENABLE_PROFILE_ERROR_START | 0x04,      /*!< ENABLE ERROR wrong profile re-enabling */
    ESIM_ENABLE_CAT_BUSY = ESIM_ENABLE_PROFILE_ERROR_START | 0x05,                      /*!< ENABLE ERROR CAT busy */
    ESIM_ENABLE_UNDEFINED_ERROR = ESIM_ENABLE_PROFILE_ERROR_START | 0x7F,               /*!< ENABLE ERROR undefined error */

    ESIM_DISABLE_PROFILE_ERROR_START = 0x200 | OSA_ERRCODE_ESIM_BASE,
    ESIM_DISABLE_ICCID_OR_AID_NOTFOUND = ESIM_DISABLE_PROFILE_ERROR_START | 0x01,        /*!< DISABLE ERROR ICCID or Aid not found */
    ESIM_DISABLE_PROFILE_NOT_IN_ENABLE_STATUS = ESIM_DISABLE_PROFILE_ERROR_START | 0x02, /*!< DISABLE ERROR profile not in enable status */
    ESIM_DISABLE_DISALLOWED_BY_POLICY = ESIM_DISABLE_PROFILE_ERROR_START | 0x03,         /*!< DISABLE ERROR disallowed by policy */
    ESIM_DISABLE_CAT_BUSY = ESIM_DISABLE_PROFILE_ERROR_START | 0x05,                     /*!< DISABLE ERROR CAT busy */
    ESIM_DISABLE_UNDEFINED_ERROR = ESIM_DISABLE_PROFILE_ERROR_START | 0x7F,              /*!< DISABLE ERROR undefined error */

    ESIM_DELETE_PROFILE_ERROR_START = 0x300 | OSA_ERRCODE_ESIM_BASE,
    ESIM_DELETE_ICCID_OR_AID_NOTFOUND = ESIM_DELETE_PROFILE_ERROR_START | 0x01,         /*!< DELETE ERROR ICCID or Aid not found */
    ESIM_DELETE_PROFILE_NOT_IN_DISABLE_STATUS = ESIM_DELETE_PROFILE_ERROR_START | 0x02, /*!< DELETE ERROR profile not in disable status */
    ESIM_DELETE_DISALLOWED_BY_POLICY = ESIM_DELETE_PROFILE_ERROR_START | 0x03,          /*!< DELETE ERROR disallowed by policy */
    ESIM_DELETE_UNDEFINED_ERROR = ESIM_DELETE_PROFILE_ERROR_START | 0x7F,               /*!< DELETE ERROR undefined error */

    ESIM_LIST_PROFILE_ERROR_START = 0x400 | OSA_ERRCODE_ESIM_BASE,
    ESIM_LIST_INCORRECT_INPUT_VALUES = ESIM_LIST_PROFILE_ERROR_START | 0x01, /*!< PROFILE LIST ERROR incorrect input values */
    ESIM_LIST_NO_PROFILE_IN_EUICC = ESIM_LIST_PROFILE_ERROR_START | 0x10,    /*!< PROFILE LIST ERROR no profile in euicc */
    ESIM_LIST_UNDEFINED_ERROR = ESIM_LIST_PROFILE_ERROR_START | 0x7F,        /*!< PROFILE LIST ERROR undefined error */

    ESIM_NICKNAME_PROFILE_ERROR_START = 0x500 | OSA_ERRCODE_ESIM_BASE,
    ESIM_NICKNAME_ICCID_NOT_FOUND = ESIM_NICKNAME_PROFILE_ERROR_START | 0x01, /*!< NICKNAME ERROR ICCID not found */
    ESIM_NICKNAME_UNDEFINED_ERROR = ESIM_NICKNAME_PROFILE_ERROR_START | 0x7F, /*!< NICKNAME ERROR undefined error */

    ESIM_DELETE_NOTIFICATION_ERROR_START = 0x600 | OSA_ERRCODE_ESIM_BASE,
    ESIM_NOTIFICATION_NOTHING_TO_DELETE = ESIM_DELETE_NOTIFICATION_ERROR_START | 0x01, /*!< DELETE NOTIFICATION ERROR nothing to delete */
    ESIM_NOTIFICATION_UNDEFINED_ERROR = ESIM_DELETE_NOTIFICATION_ERROR_START | 0x7F,   /*!< DELETE NOTIFICATION ERROR undefined error */

    ESIM_AUTH_SERVER_RESP_ERROR_START = 0x700 | OSA_ERRCODE_ESIM_BASE,
    ESIM_AUTH_SERVER_INVALID_CERTIFICATE = ESIM_AUTH_SERVER_RESP_ERROR_START | 0x01,      /*!< AUTH SERVER ERROR invalid certificate */
    ESIM_AUTH_SERVER_INVALID_SIGNATURE = ESIM_AUTH_SERVER_RESP_ERROR_START | 0x02,        /*!< AUTH SERVER ERROR invalid signature */
    ESIM_AUTH_SERVER_UNSUPPORTED_CURVE = ESIM_AUTH_SERVER_RESP_ERROR_START | 0x03,        /*!< AUTH SERVER ERROR unsupported curve */
    ESIM_AUTH_SERVER_NO_SESSION_CONTEXT = ESIM_AUTH_SERVER_RESP_ERROR_START | 0x04,       /*!< AUTH SERVER ERROR no session context */
    ESIM_AUTH_SERVER_INVALID_OID = ESIM_AUTH_SERVER_RESP_ERROR_START | 0x05,              /*!< AUTH SERVER ERROR invalid OID */
    ESIM_AUTH_SERVER_EUICC_CHALLENGE_MISMATCH = ESIM_AUTH_SERVER_RESP_ERROR_START | 0x06, /*!< AUTH SERVER ERROR euicc challenge mismatch */
    ESIM_AUTH_SERVER_CIPK_UNKNOWN = ESIM_AUTH_SERVER_RESP_ERROR_START | 0x07,             /*!< AUTH SERVER ERROR ciPK unknown */
    ESIM_AUTH_SERVER_UNDEFINED_ERROR = ESIM_AUTH_SERVER_RESP_ERROR_START | 0x7F,          /*!< AUTH SERVER ERROR undefined error */

    ESIM_PREPARE_DOWNLOAD_RESP_ERROR_START = 0x800 | OSA_ERRCODE_ESIM_BASE,
    ESIM_PREPARE_DOWNLOAD_INVALID_CERTIFICATE = ESIM_PREPARE_DOWNLOAD_RESP_ERROR_START | 0x01,   /*!< PREPARE DOWNLOAD ERROR invalid certificate */
    ESIM_PREPARE_DOWNLOAD_INVALID_SIGNATURE = ESIM_PREPARE_DOWNLOAD_RESP_ERROR_START | 0x02,     /*!< PREPARE DOWNLOAD ERROR invalid signature */
    ESIM_PREPARE_DOWNLOAD_UNSUPPORTED_CURVE = ESIM_PREPARE_DOWNLOAD_RESP_ERROR_START | 0x03,     /*!< PREPARE DOWNLOAD ERROR unsupported curve */
    ESIM_PREPARE_DOWNLOAD_NO_SESSION_CONTEXT = ESIM_PREPARE_DOWNLOAD_RESP_ERROR_START | 0x04,    /*!< PREPARE DOWNLOAD ERROR no session context */
    ESIM_PREPARE_DOWNLOAD_INVALID_TRANSACTIONID = ESIM_PREPARE_DOWNLOAD_RESP_ERROR_START | 0x05, /*!< PREPARE DOWNLOAD ERROR invalid TransactionId */
    ESIM_PREPARE_DOWNLOAD_UNDEFINED_ERROR = ESIM_PREPARE_DOWNLOAD_RESP_ERROR_START | 0x7F,       /*!< PREPARE DOWNLOAD ERROR undefined error */

    ESIM_PROFILE_INSTALL_RESULT_ERROR_START = 0x900 | OSA_ERRCODE_ESIM_BASE,
    ESIM_INCORRECT_INPUT_VALUES = ESIM_PROFILE_INSTALL_RESULT_ERROR_START | 0x01,            /*!< PROFILE INSTALL ERROR incorrect input values */
    ESIM_INVALID_SIGNATURE = ESIM_PROFILE_INSTALL_RESULT_ERROR_START | 0x02,                 /*!< PROFILE INSTALL ERROR invalid signature */
    ESIM_INVALID_TRANSACTIONID = ESIM_PROFILE_INSTALL_RESULT_ERROR_START | 0x03,             /*!< PROFILE INSTALL ERROR invalid TransactionId */
    ESIM_UNSUPPOTRED_CRT_VALUES = ESIM_PROFILE_INSTALL_RESULT_ERROR_START | 0x04,            /*!< PROFILE INSTALL ERROR unsupported CRT */
    ESIM_UNSUPPORTED_REMOTE_OPERATION_TYPE = ESIM_PROFILE_INSTALL_RESULT_ERROR_START | 0x05, /*!< PROFILE INSTALL ERROR unsupported remote operation type */
    ESIM_UNSUPPORTED_PROFILE_CLASS = ESIM_PROFILE_INSTALL_RESULT_ERROR_START | 0x06,         /*!< PROFILE INSTALL ERROR unsupported profile class */
    ESIM_SECP03T_STRUCTURE_ERROR = ESIM_PROFILE_INSTALL_RESULT_ERROR_START | 0x07,           /*!< PROFILE INSTALL ERROR scp03t structure error */
    ESIM_SECP03T_SECURITY_ERROR = ESIM_PROFILE_INSTALL_RESULT_ERROR_START | 0x08,            /*!< PROFILE INSTALL ERROR scp03t security error */
    ESIM_INSTALL_FAIL_DUE_TO_ICCID_ALREADY_EXISTS_ON_EUICC = ESIM_PROFILE_INSTALL_RESULT_ERROR_START | 0x09, /*!< PROFILE INSTALL ERROR ICCID already exists */
    ESIM_INSTALL_FAIL_DUE_TO_INSUFFICIENT_MEMORY_FOR_PROFILE = ESIM_PROFILE_INSTALL_RESULT_ERROR_START | 0x0A, /*!< PROFILE INSTALL ERROR insufficient memory */
    ESIM_INSTALL_FAIL_DUE_TOINTERRUOTION = ESIM_PROFILE_INSTALL_RESULT_ERROR_START | 0x0B, /*!< PROFILE INSTALL ERROR installation failed due to interruption */
    ESIM_INSTALL_FAIL_DUE_TOPE_PROCESSING_ERROR
        = ESIM_PROFILE_INSTALL_RESULT_ERROR_START | 0x0C,                  /*!< PROFILE INSTALL ERROR installation failed due to PE processing error */
    ESIM_INSTALL_FAIL_DUE_TO_DATA_MISMATCH
        = ESIM_PROFILE_INSTALL_RESULT_ERROR_START | 0x0D,                  /*!< PROFILE INSTALL ERROR installation failed due to data mismatch */
    ESIM_TEST_PROFILE_INSTALL_FAIL_DUE_TO_INVALID_NAAKEY
        = ESIM_PROFILE_INSTALL_RESULT_ERROR_START | 0x0E,                  /*!< PROFILE INSTALL ERROR test profile installation failed due to invalid NaaKey */
    ESIM_PPR_NOT_ALLOWED = ESIM_PROFILE_INSTALL_RESULT_ERROR_START | 0x0F, /*!< PROFILE INSTALL ERROR ppr not allowed */
    ESIM_INSTALL_FAIL_DUE_TO_UNDEFINED_ERROR = ESIM_PROFILE_INSTALL_RESULT_ERROR_START | 0x7F, /*!< PROFILE INSTALL ERROR undefined error */

    ESIM_RSP_PROCESS_SMDPP_RETURN_ERROR_START = 0xA00 | OSA_ERRCODE_ESIM_BASE,
    ESIM_INIT_AUTH_SMDPP_ADDRESS_REFUSE = ESIM_RSP_PROCESS_SMDPP_RETURN_ERROR_START | 0x01, /*!< 8.8.1-3.8 Invalid SM-DP+ Address*/
    ESIM_INIT_AUTH_SPECIFICATION_VERSION_NUMBER_UNSUPPORTED
        = ESIM_RSP_PROCESS_SMDPP_RETURN_ERROR_START
          | 0x02, /*!< 8.8.3-3.1 The Specification Version Number indicated by the eUICC is not supported by the SM-DP+.*/
    ESIM_INIT_AUTH_SECURITY_CONFIGURATION_UNSUPPORTED
        = ESIM_RSP_PROCESS_SMDPP_RETURN_ERROR_START | 0x03, /*!< 8.8.2-3.1 None of the proposed Public Key Identifiers is supported by the SM-DP+.*/
    ESIM_INIT_AUTH_SMDPP_CERTIFICATE_UNAVAILABLE, /*!< 8.8.4-3.7 The SM-DP+ has no CERT.DPAuth.ECDSA signed by one of the GSMA CI Public Key supported by the eUICC*/

    ESIM_AUTH_CLIENT_SMDPP_RETURN_ERROR_START = 0xB00 | OSA_ERRCODE_ESIM_BASE,
    ESIM_AUTH_CLIENT_EUM_CERTIFICATE_VERIFICATION_FAILED = ESIM_AUTH_CLIENT_SMDPP_RETURN_ERROR_START | 0x01,   /*!< 8.8.2-6.1 Certificate is invalid.*/
    ESIM_AUTH_CLIENT_EUM_CERTIFICATE_EXPIRED = ESIM_AUTH_CLIENT_SMDPP_RETURN_ERROR_START | 0x02,               /*!< 8.8.2-6.3 Certificate has expired.*/
    ESIM_AUTH_CLIENT_EUICC_CERTIFICATE_VERIFICATION_FAILED = ESIM_AUTH_CLIENT_SMDPP_RETURN_ERROR_START | 0x03, /*!< 8.8.3-6.1 Certificate is invalid.*/
    ESIM_AUTH_CLIENT_EUICC_CERTIFICATE_EXPIRED = ESIM_AUTH_CLIENT_SMDPP_RETURN_ERROR_START | 0x04,             /*!< 8.8.3-6.3 Certificate has expired.*/
    ESIM_AUTH_CLIENT_EUICC_VERIFICATION_FAILED
        = ESIM_AUTH_CLIENT_SMDPP_RETURN_ERROR_START | 0x05, /*!< 8.1-6.1 eUICC signature is invalid or serverChallenge is invalid.*/
    ESIM_AUTH_CLIENT_MATCHINGID_REFUSED = ESIM_AUTH_CLIENT_SMDPP_RETURN_ERROR_START | 0x06, /*!< 8.2.6-3.8 MatchingID (AC_Token or EventID) is refused.*/
    ESIM_AUTH_CLIENT_EID_REFUSED = ESIM_AUTH_CLIENT_SMDPP_RETURN_ERROR_START | 0x07,        /*!< 8.1.1-3.8 EID doesn't match the expected value.*/
    ESIM_AUTH_CLIENT_PROFILE_TYPE_STOPPED_ON_WARNING
        = ESIM_AUTH_CLIENT_SMDPP_RETURN_ERROR_START | 0x08,                                 /*!< 8.2.5-4.3 No eligible Profile for this eUICC/Device*/
    ESIM_AUTH_CLIENT_CI_PUBLIC_KEY_UNKNOWN
        = ESIM_AUTH_CLIENT_SMDPP_RETURN_ERROR_START
          | 0x09, /*!< 8.11.1-3.9 Unknown CI Public Key. The CI used by the EUM Certificate is not a trusted root for the SM-DP+.*/
    ESIM_AUTH_CLIENT_TRANSACTIONID_UNKNOWN
        = ESIM_AUTH_CLIENT_SMDPP_RETURN_ERROR_START | 0x0A, /*!< 8.10.1-3.9 The RSP session identified by the TransactionID is unknown.*/
    ESIM_AUTH_CLIENT_EUICC_INSUFFICIENT_MEMORY
        = ESIM_AUTH_CLIENT_SMDPP_RETURN_ERROR_START | 0x0B, /*!< 8.1-4.8 Invalid eUICC does not have sufficient space for this Profile.*/
    ESIM_AUTH_CLIENT_PROFILE_NOT_ALLOWED,                   /*!< 8.2-1.2 Profile has not yet been released.*/
    ESIM_AUTH_CLIENT_DOWNLOAD_ORDER_TIME_TO_LIVE_EXPIRED,   /*!< 8.8.5-4.10 The Download order has expired*/
    ESIM_AUTH_CLIENT_DOWNLOAD_ORDER_LIMIT_EXCEEDED,         /*!< 8.8.5-6.4 The maximum number of retries for the Profile download order has been exceeded.*/
    ESIM_AUTH_CLIENT_PPR_NOT_ALLOWED = ESIM_AUTH_CLIENT_SMDPP_RETURN_ERROR_START
                                       | 0x32, /*!< 8.2.8-1.2 The corresponding ES9+'.AuthenticateClient function returned PPR not allowed in RAT.*/
    ESIM_AUTH_CLIENT_UNDEFINED_ERROR = ESIM_AUTH_CLIENT_SMDPP_RETURN_ERROR_START | 0x7F,

    ESIM_GET_BPP_SMDPP_RETURN_ERROR_START = 0xC00 | OSA_ERRCODE_ESIM_BASE,
    ESIM_GET_BPP_EUICC_VERIFICATION_FAILED = ESIM_GET_BPP_SMDPP_RETURN_ERROR_START | 0x01, /*!< 8.1-6.1 eUICC signature is invalid.*/
    ESIM_GET_BPP_CONFIRMATION_CODE_MISSING = ESIM_GET_BPP_SMDPP_RETURN_ERROR_START | 0x02, /*!< 8.2.7-2.2 Confirmation Code is missing.*/
    ESIM_GET_BPP_CONFIRMATION_CODE_REFUSED = ESIM_GET_BPP_SMDPP_RETURN_ERROR_START | 0x03, /*!< 8.2.7-3,8 Confirmation Code is refused.*/
    ESIM_GET_BPP_CONFIRMATION_CODE_LIMIT_EXCEEDED
        = ESIM_GET_BPP_SMDPP_RETURN_ERROR_START | 0x04, /*!< 8.2.7-6.4 The maximum number of retries for the Confirmation Code has been exceeded*/
    ESIM_GET_BPP_PROFILE_UNAVAILABLE = ESIM_GET_BPP_SMDPP_RETURN_ERROR_START | 0x05,                 /*!< 8.2-3.7 BPP is not available for a new binding.*/
    ESIM_GET_BPP_DOWNLOAD_ORDER_TIME_TO_LIVE_EXPIRED = ESIM_GET_BPP_SMDPP_RETURN_ERROR_START | 0x06, /*!< 8.8.5-4.10 The Download order has expired*/
    ESIM_GET_BPP_TRANSACTIONID_UNKNOWN
        = ESIM_GET_BPP_SMDPP_RETURN_ERROR_START | 0x5F, /*!< 8.10.1-3.9 The RSP session identified by the TransactionID is unknown.*/
    ESIM_GET_BPP_UNDEFINED_ERROR = ESIM_GET_BPP_SMDPP_RETURN_ERROR_START | 0x7F,

    ESIM_RSP_PROCESS_SMDPP_RETURN_ERROR_END = 0xCFF | OSA_ERRCODE_ESIM_BASE,

    ESIM_FALLBACK_RESP_ERROR_START = 0x1000 | OSA_ERRCODE_ESIM_BASE,
    ESIM_FALLBACK_PROFILE_NOT_IN_DISABLE_STATUS = ESIM_FALLBACK_RESP_ERROR_START | 0x02, /*!< FALLBACK ERR, profile not in disabled status */
    ESIM_FALLBACK_CAT_BUSY = ESIM_FALLBACK_RESP_ERROR_START | 0x05,                      /*!< FALLBACK ERR, CAT busy */
    ESIM_FALLBACK_NOT_AVAILABLE = ESIM_FALLBACK_RESP_ERROR_START | 0x06,                 /*!< FALLBACK ERR, fallback not available */
    ESIM_FALLBACK_COMMAND_ERROR = ESIM_FALLBACK_RESP_ERROR_START | 0x07,                 /*!< FALLBACK ERR, command error */
    ESIM_FALLBACK_ECALLACTIVE = ESIM_FALLBACK_RESP_ERROR_START | 0X68,                   /*!< FALLBACK ERR, ecall active */
    ESIM_FALLBACK_UNDEFINED_ERROR = ESIM_FALLBACK_RESP_ERROR_START | 0x7F,               /*!< FALLBACK ERR, undefined error */

    ESIM_ADD_INIT_EIM_RESP_ERROR_START = 0x1100 | OSA_ERRCODE_ESIM_BASE,
    ESIM_ADD_INIT_EIM_INSUFFICIENT_MEMORY = ESIM_ADD_INIT_EIM_RESP_ERROR_START | 0x01,           /*!< ADD EIM ERR, insufficient memory */
    ESIM_ADD_INIT_EIM_ASSOCIATED_EIM_ALREADY_EXISTS = ESIM_ADD_INIT_EIM_RESP_ERROR_START | 0x02, /*!< ADD EIM ERR, associated EIM already exists */
    ESIM_ADD_INIT_EIM_CI_PK_UNKNOWN = ESIM_ADD_INIT_EIM_RESP_ERROR_START | 0x03,                 /*!< ADD EIM ERR, unknown CI public key */
    ESIM_ADD_INIT_EIM_INVALID_ASSOCIATION_TOKEN = ESIM_ADD_INIT_EIM_RESP_ERROR_START | 0x05,     /*!< ADD EIM ERR, invalid association token */
    ESIM_ADD_INIT_EIM_COUNTER_VALUE_OUT_OF_RANGE = ESIM_ADD_INIT_EIM_RESP_ERROR_START | 0x06,    /*!< ADD EIM ERR, counter value out of range */
    ESIM_ADD_INIT_EIM_COMMAND_ERROR = ESIM_ADD_INIT_EIM_RESP_ERROR_START | 0x07,                 /*!< ADD EIM ERR, command error */
    ESIM_ADD_INIT_EIM_UNDEFINED_ERROR = ESIM_ADD_INIT_EIM_RESP_ERROR_START | 0x7F,               /*!< ADD EIM ERR, undefined error */
} esim_result_errno_e;

#endif /* __ESIM_ERROR_H__ */
