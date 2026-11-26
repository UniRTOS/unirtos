/**********************************************************************
 * @file esim_interface.h
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
#ifndef __ESIM_INTERFACE_H__
#define __ESIM_INTERFACE_H__

#include "esim_config.h"
#include "esim_error.h"
#include "qosa_def.h"
#include "qosa_queue_list.h"
#include "qosa_sys.h"

/** eSIM IMEI string length definition */
#define ESIM_IMEI_MAX_LEN              16

/** The maximum total length of data packets received by the trans interface */
#define ESIM_AT_TRANS_TOTAL_MAX_LEN    204800

/** Used to save the length of small data array space information */
#define ESIM_DATA_ARRAY_MAX_LEN        128

/** eSIM nickname string length definition */
#define ESIM_NICKNAME_MAX_LEN          64

/** eSIM ICCID string length definition */
#define ESIM_ICCID_STR_LEN             20

/** eSIM ICCID byte length definition */
#define ESIM_ICCID_BYTE_LEN            10

/** eSIM EID string length definition */
#define ESIM_EID_STR_LEN               32

/** eSIM notification data max length definition */
#define ESIM_NOTIFICATION_DATA_MAX_LEN 4096

/** eSIM SMDPP address string length definition */
#define ESIM_SMDPP_ADDRESS_MAX_LEN     128

/** eSIM activation code and confirmation code string length definition */
#define ESIM_AC_CC_CODE_MAX_LEN        128

/** eSIM EIM ID string length definition */
#define ESIM_EIM_ID_MAX_LEN            128

/** eSIM APN string length definition */
#define ESIM_APN_MAX_LEN               128

/** Polling minimum interval definition (unit: seconds) */
#define ESIM_EIM_POLL_INTERVAL_SEC_MIN 120
/** Polling maximum interval definition (unit: seconds) */
#define ESIM_EIM_POLL_INTERVAL_SEC_MAX 3888000

/** Polling delay minimum interval definition (unit: seconds) */
#define ESIM_EIM_POLL_DELAY_SEC_MIN    60

/** Polling delay maximum interval definition (unit: seconds) */
#define ESIM_EIM_POLL_DELAY_SEC_MAX    900

/**
 * @enum euicc_reset_option_e
 * @brief Used to operate eUICC reset options
 * @note Currently only supports RESET_EIM_CONFIGDATA
 */
typedef enum
{
    DELETE_OPERATIONAL_PROFILES = 0,   /*!< delete operational profiles */
    DELETE_FIELD_LOADED_TEST_PROFILES, /*!< delete field loaded test profiles */
    RESET_DEFAULT_SMDP_ADDRESS,        /*!< reset default SMDP+ address */
    DELETE_PRELOADED_TEST_PROFILES,    /*!< delete preloaded test profiles */
    DELETE_PROVISIONING_PROFILES,      /*!< delete provisioning profiles */
    RESET_EIM_CONFIGDATA,              /*!< reset eIM config data */
    RESET_IMMEDIATE_ENABLE_CONFIG,     /*!< reset immediate enable config */
} euicc_reset_option_e;

/**
 * @enum esim_notification_event_e
 * @brief Used to select eSIM notification type
 */
typedef enum
{
    LPA_NOTIFICATION_EVENT_INSTALL = 0x0780, /*!< profile install events */
    LPA_NOTIFICATION_EVENT_ENABLE = 0x0640,  /*!< profile enable events */
    LPA_NOTIFICATION_EVENT_DISABLE = 0x0520, /*!< profile disable events */
    LPA_NOTIFICATION_EVENT_DELETE = 0x0410,  /*!< profile delete events */
    LPA_NOTIFICATION_EVENT_ALL = 0x04F0      /*!< all profile events */
} esim_notification_event_e;

/**
 * @enum esim_profile_opt_e
 * @brief Used to operate profile operation options
 */
typedef enum
{
    ESIM_PROFILE_OPT_ENABLE = 0, /*!< profile enable */
    ESIM_PROFILE_OPT_DISABLE,    /*!< profile disable */
    ESIM_PROFILE_OPT_DELETE,     /*!< profile delete */
} esim_profile_opt_e;

/**
 * @struct esim_data_array_t
 * @brief Used to store the spatial information required for general small data storage
 */
typedef struct
{
    qosa_uint8_t data_len;                      /*!< Storage space data length */
    qosa_uint8_t data[ESIM_DATA_ARRAY_MAX_LEN]; /*!< Data storage space */
} esim_data_array_t;

/**
 * @struct esim_data_ptr_t
 * @brief Used to store spatial information required for large amounts of information transmission
 */
typedef struct
{
    qosa_uint32_t data_len; /*!< Storage space data length */
    char         *data;     /*!< Data transfer pointer, if passed from platform to SDK, released by SDK, otherwise released by platform*/
} esim_data_ptr_t;

/**
 * @struct esim_profile_info_t
 * @brief Used to save the information content of a single profile
 */
typedef struct
{
    qosa_q_link_type_t link;          /*!< link type */
    esim_data_array_t  iccid_info;    /*!< iccid */
    qosa_int8_t        status;        /*!< profile status (0: disabled, 1: enabled) */
    qosa_int8_t        pro_class;     /*!< profile class (0: test, 1: provisioning, 2: operational) */
    esim_data_array_t  nickname_info; /*!< nickname */
    esim_data_array_t  provider;      /*!< provider */
    esim_data_array_t  name_info;     /*!< profile name */
} esim_profile_info_t;

/**
 * @struct esim_ntf_info_t
 * @brief Used to save notification metadata related information
 */
typedef struct
{
    qosa_q_link_type_t link;       /*!< link type  */
    qosa_int32_t       type;       /*!< nocification type */
    qosa_int32_t       seq_num;    /*!< nocification sequence number */
    esim_data_array_t  iccid_info; /*!< iccid */
} esim_ntf_info_t;

/**
 * @struct esim_ntf_complete_info_t
 * @brief Used to save the complete information of a single notification
 */
typedef struct
{
    qosa_q_link_type_t link;       /*!< link type  */
    qosa_int32_t       type;       /*!< nocification type */
    qosa_int32_t       seq_num;    /*!< nocification sequence number */
    esim_data_array_t  iccid_info; /*!< iccid */
    esim_data_array_t  smdpp_add;  /*!< SM-DP+ address */
    esim_data_ptr_t    ntf_data;   /*!< nocification BASE64 data */
} esim_ntf_complete_info_t;

/**
 * @struct esim_smdpp_indirect_result_t
 * @brief When used for indirect download, stores the data requested by SM-DP+
 */
typedef struct
{
    esim_result_errno_e ret;       /*!< Return result: ESIM_OK if success, or other error codes if failed */
    esim_data_array_t   smdpp_add; /*!< DP+ address */
    esim_data_array_t   url;       /*!< DP+ Path */
    esim_data_ptr_t     req_body;  /*!< Request data*/
    qosa_bool_t         end_flag;  /*!< True when a download error occurs or the download is complete */
} esim_smdpp_indirect_result_t;

/**
 * @struct esim_eim_info_t
 * @brief Used to save the information of a single eIM
 */
typedef struct
{
    qosa_q_link_type_t link;                        /*!< Link type */
    esim_data_array_t  eim_id;                      /*!< eIM id */
    esim_data_array_t  eim_fqdn;                    /*!< eIM SERVER FQDN*/
    qosa_int8_t        eim_id_type;                 /*!< eIM id type*/
    qosa_int32_t       association_token;           /*!< Association token, used for session binding */
    esim_data_ptr_t    eim_public_key_data;         /*!< eIM public key/certificate data, used by eUICC to verify data packet signature */
    esim_data_ptr_t    trusted_public_key_data_tls; /*!< TLS trusted public key/certificate chain for eIM server authentication */
    qosa_int32_t       eim_supported_protocol;      /*!< Protocol versions supported by eIM */
    esim_data_array_t  euicc_ci_pk_id;              /*!< CI public key identifier for signing on the eUICC */
    qosa_bool_t        indirect_profile_download;   /*!< Does it support indirect profile download via a specified protocol? */
} esim_eim_cfg_data_t;

/**
 * @struct esim_ipa_timer_cfg_t
 * @brief Used to save the IPA timer configuration
 */
typedef struct esim_ipa_timer_cfg
{
    qosa_uint32_t ipa_poll_interval; /*!< IPA polling interval parameter (unit: seconds) */
    qosa_uint32_t rollback_limit;    /*!< Rollback parameters (unit: seconds) */
} esim_ipa_timer_cfg_t;

/**
 * @brief get operational profile
 *
 * @param[out] qosa_q_type_t *profile_list (esim_profile_info_t)
 *          - Linked list for storing profile information
 *
 * @return esim_result_errno_e
 *          - If success, ESIM_OK is returned, otherwise other error codes are returned
 */
esim_result_errno_e esim_intf_get_operational_profile(qosa_q_type_t *profile_list);

/**
 * @brief Get all profiles
 *
 * @param[out] qosa_q_type_t *profile_list (esim_profile_info_t)
 *          - Linked list for storing profile information
 *
 *
 * @return esim_result_errno_e
 *          - success, return ESIM_OK
 *          - failure, return other error codes.
 */
esim_result_errno_e esim_intf_get_all_profile(qosa_q_type_t *profile_list);

/**
 * @brief get Get EID string
 *
 * @param[out] char *eid
 *          - Used to store EID data (EID length is 32 bytes, and the size of EID should be greater than 32 bytes).
 *
 * @return esim_result_errno_e
 *          - success, return ESIM_OK
 *          - failure, return other error codes.
 */
esim_result_errno_e esim_intf_eid_get(char *eid);

/**
 * @brief Get the ESIM SDK version number
 *
 * @param[out] char *ver
 *          - Used to store ESIM SDK version number data (ver_buf space size should be greater than 32 bytes)
 *
 * @param[in] int ver_buf_size
 *          - size of ver_buf
 *
 * @return int
 *          - success, return the version number character length
 *          - failure, return 0
 */
int esim_intf_ver_get(char *ver_buf, int ver_buf_size);

/**
 * @brief Get eSIM SDK version time
 *
 * @param[out] char *cmp_buf
 *          - Used to store ESIM SDK version time data (the size of cmp_buf should be greater than 16 bytes)
 *
 * @param[in] int cmp_buf_size
 *          - Size of cmp_buf
 *
 * @return int
 *          - success: return version time and character length
 *          - failure: return 0
 */
int esim_intf_compile_data_get(char *cmp_buf, int cmp_buf_size);

/**
 * @brief Download the profile directly using the module network.
 *
 * @param[in] char *ac_code
 *          - Activation Code
 *
 * @param[in] int ac_len
 *          - Activation code length
 *
 * @param[in] char *cfmt_code
 *          - Confirmation code (set to NULL if no confirmation code is required).
 *
 * @param[in] int cfmt_len
 *          - Confirmation code length (set to 0 if no confirmation code is required).
 *
 * @param[out] esim_data_array_t *iccid
 *          - When the profile is successly downloaded, it is used to store the profile's ICCID data.
 *
 * @return esim_result_errno_e
 *          - success, return ESIM_OK
 *          - failure, return other error codes.
 */
esim_result_errno_e esim_intf_direct_download_profile(char *ac_code, int ac_len, char *cfmt_code, int cfmt_len, esim_data_array_t *iccid_str);

/**
 * @brief Use the activation code and confirmation code to start the indirect download profile process
 *
 * @param[in] char *ac_code
 *          - Activation code
 *
 * @param[in] int ac_len
 *          - Activation code  length
 *
 * @param[in] char *cfmt_code
 *          - Confirmation code (set to NULL if no confirmation code is required).
 *
 * @param[in] int cfmt_len
 *          - Confirmation code length(set to 0 if no confirmation code is required).
 *
 * @return esim_smdpp_indirect_result_t *
 *          - return the request parameters to be forwarded to DP+.
 *          - Upon success, return the request parameters to be forwarded to DP+. ret == ESIM_OK.
 *          - Upon failure, return NULL, or if ret != ESIM_OK, requires re-initiating the process.
 */
esim_smdpp_indirect_result_t *esim_intf_indirect_download_profile_start(char *ac_code, int ac_len, char *cfmt_code, int cfmt_len);

/**
 * @brief During the profile transmission process, DP+ return data to the module
 *
 * @param[in] qosa_bool_t state
 *          - Data packet status flag
 *          - OSA_FALSE, it indicates the last data packet
 *          - OOSA_TRUE, it indicates that the current data packet is not the last data packet
 *
 * @param[in] qosa_uint8_t seq
 *          - The data packet sequence number is 0, indicating the start of the data packet
 *
 * @param[in] qosa_uint8_t *data
 *          - Subcontracted data
 *
 * @param[in] qosa_uint32_t data_len
 *          - Packet data length
 *
 * @return esim_smdpp_indirect_result_t *
 *          - return the request parameters to be forwarded to DP+.
 *          - Upon success, return the request parameters to be forwarded to DP+. ret == ESIM_OK.
 *          - Upon failure, return NULL, or if ret != ESIM_OK, requires re-initiating the process.
 */
esim_smdpp_indirect_result_t *esim_intf_indirect_download_data_transfer(qosa_bool_t state, qosa_uint8_t seq, qosa_uint8_t *data, qosa_uint32_t data_len);

/**
 * @brief Set the ipa polling interval
 *
 * @param[in] unsigned int *sec
 *          - Pointer to the polling interval (in seconds).
 *          - If the pointer is null, a polling operation is triggered immediately.
 *          - If the pointer is not null, the value range is [0, ESIM_EIM_POLL_INTERVAL_SEC_MIN - ESIM_EIM_POLL_INTERVAL_SEC_MAX]
 *          - A value of 0 disables polling; other values ​​indicate the polling interval.
 *
 * @return esim_result_errno_e
 *          - success, return ESIM_OK
 *          - failure, return other error codes.
 */
esim_result_errno_e esim_intf_poll_interval_set(qosa_uint32_t *sec);

/**
 * @brief Get IPA polling interval and rollback time
 *
 * @param[out] esim_ipa_timer_cfg_t *time_cfg
 *          - pointer to storage time configuration
 *
 * @return esim_result_errno_e
 *          - success, return ESIM_OK
 *          - failure, return other error codes.
 */
esim_result_errno_e esim_intf_ipa_time_cfg_get(esim_ipa_timer_cfg_t *time_cfg);

/**
 * @brief  Get all eIM information and store it in the given linked list
 *
 * @param[out] qosa_q_type_t *eim_info_list
 *          - Linked list structure storing eIM data (node ​​type: esim_eim_cfg_data_t)
 *
 * @return esim_result_errno_e
 *          - success, return ESIM_OK
 *          - failure, return other error codes.
 */
esim_result_errno_e esim_intf_eim_info_get(qosa_q_type_t *eim_info_list);

/**
 * @brief This function is used to add the first eIM information
 *
 * @param[in] qosa_bool_t state
 *          - Data packet status flag
 *          - OSA_FALSE, it indicates the last data packet,
 *          - OOSA_TRUE, it indicates that the current data packet is not the last data packet
 *
 * @param[in] qosa_uint8_t seq
 *          - A data packet sequence number of 0 indicates the start of a data packet.
 *
 * @param[in] qosa_uint8_t *data
 *          - eIM message packet data
 *
 * @param[in] qosa_uint32_t data_len
 *          - Packet data length
 *
 * @return esim_result_errno_e
 *          - success, return ESIM_OK
 *          - failure, return other error codes.
 */
esim_result_errno_e esim_intf_initail_eim_add(qosa_bool_t state, qosa_uint8_t seq, qosa_uint8_t *data, qosa_uint32_t data_len);

/**
 * @brief Choose the eIM to connect to during polling.
 *
 * @param[in] qosa_uint8_t *eim_id
 *          - eIM ID string that already exists in EISM
 *
 * @return esim_result_errno_e
 *          - success, return ESIM_OK
 *          - failure, return other error codes.
 */
esim_result_errno_e esim_intf_eim_select(qosa_uint8_t *eim_id);

/**
 * @brief  Execute fallback operation
 *
 * @return esim_result_errno_e
 *          - success, return ESIM_OK
 *          - failure, return other error codes.
 */
esim_result_errno_e esim_intf_fallback_exte(void);

/**
 * @brief  Execute fallback return operation
 *
 * @return esim_result_errno_e
 *          - success, return ESIM_OK
 *          - failure, return other error codes.
 */
esim_result_errno_e esim_intf_fallback_return_exte(void);

/**
 * @brief  Reset eUICC memory. Currently, it is only used to reset eIM information.
 *
 * @param[in] euicc_reset_option_e opt
 *          - Memory Options
 *
 * @return esim_result_errno_e
 *          - success, return ESIM_OK
 *          - failure, return other error codes.
 */
esim_result_errno_e esim_intf_euicc_memory_reset(euicc_reset_option_e opt);

/**
 * @brief Get polling delay events
 *
 * @param[out] int *delay_s
 *          - Storage polling latency, in seconds
 *
 * @return esim_result_errno_e
 *          - success, return ESIM_OK
 *          - failure, return other error codes.
 */
esim_result_errno_e esim_intf_ipa_pll_delay_get(int *delay_s);

/**
 * @brief Set the polling delay event to delay the start of the polling operation when the machine is turned on.
 *
 * @param[in] int delay_s
 *          - The polling delay time is set in seconds.
 *
 * @return esim_result_errno_e
 *          - success, return ESIM_OK
 *          - failure, return other error codes.
 */
esim_result_errno_e esim_intf_ipa_pll_delay_set(int delay_s);

/**
 * @brief Traverse the event notifications cached in the eUICC
 *
 * @param[in] esim_notification_event_e notification_event
 *          - Notification types that need to be traversed
 *
 * @param[out] q_type *ntf_list (esim_ntf_info_t)
 *          - notification storage list
 *
 * @return esim_result_errno_e
 *          - success, return ESIM_OK
 *          - failure, return other error codes.
 */
esim_result_errno_e esim_intf_notification_list(esim_notification_event_e notification_event, qosa_q_type_t *ntf_list);

/**
 * @brief Remove an event notification from the eUICC cached event notification list based on its serial number.
 *
 * @param[in] int seq_num
 *          - The sequence number of the notification to be removed
 *
 * @return esim_result_errno_e
 *          - success, return ESIM_OK
 *          - failure, return other error codes.
 */
esim_result_errno_e esim_intf_notification_remove(int seq_num);

/**
 * @brief Retrieve complete information about an event notification
 *        from the eUICC cache based on the serial number.
 *
 * @param[in] int seq_num
 *          - Event notification sequence number
 *
 * @param[out] esim_ntf_complete_info_t *ntf_info
 *          - Pointer used to store retrieved notification information
 *
 * @return esim_result_errno_e
 *          - success, return ESIM_OK
 *          - failure, return other error codes.
 */
esim_result_errno_e esim_intf_retrieve_notification_by_seq(int seq_num, esim_ntf_complete_info_t *ntf_info);

/**
 * @brief Send the event notification in the eUICC cache to the SM-DP+ server
 *        according to the sequence number and remove it after success sending
 *
 * @param[in] int seq_num
 *          - notification sequence number
 *
 * @return esim_result_errno_e
 *          - success, return ESIM_OK
 *          - failure, return other error codes.
 */
esim_result_errno_e esim_intf_report_notification_by_seq(int seq_num);

#ifdef ESIM_COMPATIBLE_LPA_INTERFACE

/**
 * @brief  Manage (enable, disable, delete) profiles in the eUICC.
 *
 * @param[in] esim_profile_opt_e opt
 *          - profile operation options
 *
 * @param[in] char *iccid
 *          - The ICCID string of the profile
 *
 * @param[in] int iccid_len
 *          - ICCID length
 *
 * @param[in] qosa_bool_t refresh
 *          - Whether to set the refresh flag
 *
 * @return esim_result_errno_e
 *          - success, return ESIM_OK
 *          - failure, return other error codes.
 */
esim_result_errno_e esim_intf_profile_handle(esim_profile_opt_e opt, char *iccid, int iccid_len, qosa_bool_t refresh);

/**
 * @brief Modify the nickname information of the specified profile based on ICCID
 *
 * @param[in] char *nickname
 *          - Nickname string
 *
 * @param[in] int nickname_len
 *          - Nickname length
 *
 * @param[in] char *iccid
 *          - The ICCID string of the profile
 *
 * @param[in] int iccid_len
 *          -  ICCID length
 *
 * @return esim_result_errno_e
 *          - success, return ESIM_OK
 *          - failure, return other error codes.
 */
esim_result_errno_e esim_intf_profile_nickname_set(char *nickname, int nickname_len, char *iccid, int iccid_len);
#endif /* ESIM_COMPATIBLE_LPA_INTERFACE */

/**
 * @brief Initialize the esim ipa service.
 *
 * @return esim_result_errno_e
 *          - success, return 0.
 *          - failure, return other.
 */
int esim_intf_server_start(void);

/**
 * @brief Deactivate and release the esim ipa service
 *
 * @return esim_result_errno_e
 *          - success, return 0.
 *          - failure, return other.
 */
int esim_intf_server_stop(void);

#endif /* __ESIM_INTERFACE_H__ */
