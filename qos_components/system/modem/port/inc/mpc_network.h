/*****************************************************************/ /**
* @file mpc_network.h
* @brief Network operation interface for modem port layer
* @author Joe.tu@quectel.com
* @date 2024-03-18
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description
* <tr><td>2024-04-16 <td>1.0 <td>Joe.tu <td> Init
* </table>
**********************************************************************/
#ifndef __MPC_NETWORK__H__
#define __MPC_NETWORK__H__

#include "mpc.h"

#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 

#include "qosa_network.h"
#include "qosa_network_param.h"

int err_nw_mpc2ms(qosa_ptr err);

/**
 * @brief MPC_EVENT_NW_REG_STATUS_IND Cell registration status notification
 *
 * +-----------------------------+---------------------+--------+--------+
 * | ind_id                      | param1              | param2 | param3 |
 * +-----------------------------+---------------------+--------+--------+
 * | MPC_EVENT_NW_REG_STATUS_IND | mpc_nw_reg_status_t |        |        |
 * +-----------------------------+---------------------+--------+--------+
 *
 */

/**
 * @brief MPC_EVENT_NW_RRC_STATUS_IND RRC status notification
 *
 * +-----------------------------+----------------------------------+--------+--------+
 * | ind_id                      | param1                           | param2 | param3 |
 * +-----------------------------+----------------------------------+--------+--------+
 * | MPC_EVENT_NW_RRC_STATUS_IND | [MSB]null|mode|state|access[LSB] | null   | null   |
 * +-----------------------------+----------------------------------+--------+--------+
 *
 */

/**
 * @brief MPC_EVENT_NW_SIGNAL_QUALITY_IND Signal quality notification
 *
 * +---------------------------------+---------------------------+--------+--------+
 * | ind_id                          | param1                    | param2 | param3 |
 * +---------------------------------+---------------------------+--------+--------+
 * | MPC_EVENT_NW_SIGNAL_QUALITY_IND | mpc_nw_signal_quality_t * | null   | null   |
 * +---------------------------------+---------------------------+--------+--------+
 *
 */

/**
 * @struct mpc_nw_reg_status_t
 * @brief Cell registration status change event report
 *
 */
typedef struct
{
    qosa_nw_reg_status_e
        cs_reg_status; /*!< Platform layer should report original registration status when reporting registration status; for emergency camp status, report status 0, but need to report camp cell information */
    qosa_nw_reg_status_e ps_reg_status; /*!< PS domain registration status */
    qosa_plmn_t          plmn;          /*!< plmn */
#ifdef CONFIG_QOSA_NW_NR_SUPPORT
    qosa_uint64_t cid;                  /*!< cell id */
#else
    qosa_uint32_t cid; /*!< cell id */
#endif                                  // CONFIG_QOSA_NW_NR_SUPPORT
    qosa_uint32_t lac;                  /*!< lac or tac */
    qosa_uint32_t arfcn;                /*!< Carrier frequency number */
    qosa_uint32_t band;                 /*!< band information, qosa_nw_band_gsm_e, qosa_nw_band_wcdma_e, qosa_nw_band_lte_e */

    qosa_nw_act_e act;

#ifdef CONFIG_QOSA_NW_NR_SUPPORT
    qosa_uint16_t allowed_nssai_len;  /*!< Allowed NSSAI length; 0 means invalid */
    char          allowed_nssai[200]; /*!< Allowed NSSAI string */
#endif
    qosa_bool_t
        loc_present; /*!< Whether network information exists, platform layer must report complete serving cell static information when reporting PS network status as 1 or 5, loc_present is true. When reporting other network statuses, loc_present can be false */
} mpc_nw_reg_status_t;

typedef struct
{
    qosa_nw_rat_e rat; /*!< Serving cell rat */
    union
    {
        struct
        {
            qosa_signal_level_t signal_level; /*!< signal_level under GSM */
            qosa_ber_t          ber;          /*!< ber under GSM */
        } gsm;
        struct
        {
            /* Not recommended to use this parameter, this parameter is directly uploaded by platform layer, calculation rules are uncertain */
            qosa_uint16_t rssi_meas;   /*!< rssi measurement value under 3G, unit dBm */
            qosa_uint16_t rssi_report; /*!< rssi report value under 3G, unit dBm */
            qosa_rscp_t   rscp;        /*!< rscp under WCDMA */
            qosa_ecno_t   ecno;        /*!< ecno under WCDMA */
        } wcdma;
        struct
        {
            qosa_rssi_t rssi; /*!< RSSI under LTE */
            qosa_rsrp_t rsrp; /*!< rsrp under LTE */
            qosa_sinr_t sinr; /*!< sinr under LTE */
            qosa_rsrq_t rsrq; /*!< rsrq under LTE */
        } lte;
        struct
        {
            qosa_rsrp_t rsrp; /*!< rsrp under nr5g */
            qosa_sinr_t sinr; /*!< sinr under nr5g */
            qosa_rsrq_t rsrq; /*!< rsrq under nr5g */
        } nr5g;
    };
} mpc_nw_signal_quality_t;

typedef struct
{
    qosa_nw_act_e act;

    union
    {
        qosa_nw_nr5g_param_t  nr5g;
        qosa_nw_lte_param_t   lte;
        qosa_nw_wcdma_param_t wcdma;
        qosa_nw_gsm_param_t   gsm;
    };
} mpc_nw_scell_info_t;

typedef struct
{
    qosa_uint8_t        oper_num;
    qosa_nw_oper_list_t list[QOSA_NW_SUPP_OPER_NUM];
} mpc_nw_supp_oper_list_t;

typedef struct
{
    qosa_uint32_t year;
    qosa_uint32_t month;
    qosa_uint32_t day;
    qosa_uint32_t hour;
    qosa_uint32_t minute;
    qosa_uint32_t second;
    qosa_int32_t  timezone; /*!< time zone */
    qosa_int32_t  dst;      /*!< daylight saving time */
} mpc_nw_nitz_info_t;

typedef struct
{
    qosa_uint8_t jammdetectrslt;
} mpc_nw_jamm_detects_event_ind_t;

/**
 * @brief Get serving cell dynamic information (call bottom layer buffer data)
 *
 * @result
 * +-----------------+-------------------------+--------+--------+
 * | ret             | param1                  | param2 | param3 |
 * +-----------------+-------------------------+--------+--------+
 * | MPC_RET_SUCCESS | mpc_nw_scell_info_t     | null   | null   |
 * | MPC_RET_FAIL    | error code              | null   | null   |
 * +-----------------+-------------------------+--------+--------+
 *
 * @param[in] simid
 *          - sim ID
 * @param[in] cb
 *          - MPC asynchronous handle
 * @return int
 */
int mpc_nw_get_scell_info(qosa_uint8_t simid, mpc_cb_t cb);

/**
 * @brief Get neighbor cell information, type that does not decode SIB messages
 *
 * @result
 * +-----------------+---------------------+--------+--------+
 * | ret             | param1              | param2 | param3 |
 * +-----------------+---------------------+--------+--------+
 * | MPC_RET_SUCCESS | qosa_nw_ncell_info_t | null   | null   |
 * | MPC_RET_FAIL    | error code          | null   | null   |
 * +-----------------+---------------------+--------+--------+
 *
 * @param[in] simid
 *          -
 * @param[in] cb
 *          - MPC asynchronous handle
 * @return int
 */
int mpc_nw_get_ncell_info_basic(qosa_uint8_t simid, mpc_cb_t cb);

/**
 * @brief Get neighbor cell information, type that decodes SIB messages
 *
 * @result
 * +-----------------+---------------------+--------+--------+
 * | ret             | param1              | param2 | param3 |
 * +-----------------+---------------------+--------+--------+
 * | MPC_RET_SUCCESS | qosa_nw_ncell_info_t | null   | null   |
 * | MPC_RET_FAIL    | error code          | null   | null   |
 * +-----------------+---------------------+--------+--------+
 *
 * @param[in] simid
 *          - sim ID
 * @param[in] config
 *          - API interface configuration parameters, control API behavior
 * @param[in] cb
 *          - MPC asynchronous handle
 * @return int
 */
int mpc_nw_get_ncell_info_ext(qosa_uint8_t simid, qosa_nw_ncell_ext_config_t *config, mpc_cb_t cb);

/**
 * @brief Get current rat mode
 *
 * @param[in] simid
 *          - sim id
 * @param[out] rat_mode
 *          - Current rat mode, value is combination form of osa_nw_rat_e, for example, if currently locked to 2+4G, return OSA_NW_RAT_4G|QOSA_NW_RAT_2G
 * @return int
 */
int mpc_nw_get_rat_mode(qosa_uint8_t simid, qosa_uint8_t *rat_mode);

/**
 * @brief Set rat mode
 * @result
 * +-----------------+------------+--------+--------+
 * | ret             | param1     | param2 | param3 |
 * +-----------------+------------+--------+--------+
 * | MPC_RET_SUCCESS | QOSA_OK     | null   | null   |
 * | MPC_RET_FAIL    | error code | null   | null   |
 * +-----------------+------------+--------+--------+
 *
 * @param[in] simid
 *          - sim id
 * @param[in] rat_mode
 *          - Lock rat, bit2 represents GSM, bit3 represents WCDMA, bit4 represents LTE, bit5 represents NR5G
 * @param[in] effect
 *          - Whether to take effect immediately
 * @param[in] cb
 *          - MPC asynchronous handle
 * @return int
 */
int mpc_nw_set_rat_mode(qosa_uint8_t simid, qosa_uint8_t rat_mode, qosa_bool_t effect, mpc_cb_t cb);

/**
 * @brief Get rat order during network selection
 *
 * @param[in] simid
 *          - SIM ID
 * @param[out] rat_order
 *          - Rat order, array length is QOSA_NW_RAT_SEQ_NUM, array elements are osa_nw_rat_e type, priority: rat_order[0] > rat_order[1] > ... > rat_order[QOSA_NW_RAT_SEQ_NUM-1]
 * @return int
 */
int mpc_nw_get_rat_order(qosa_uint8_t simid, qosa_uint8_t rat_order[QOSA_NW_RAT_SEQ_NUM]);

/**
 * @brief Set rat order during network selection
 * @result
 * +-----------------+------------+--------+--------+
 * | ret             | param1     | param2 | param3 |
 * +-----------------+------------+--------+--------+
 * | MPC_RET_SUCCESS | QOSA_OK     | null   | null   |
 * | MPC_RET_FAIL    | error code | null   | null   |
 * +-----------------+------------+--------+--------+
 *
 * @param[in] simid
 *          - SIM ID
 * @param[in] rat_order
 *          - Rat order, array length is QOSA_NW_RAT_SEQ_NUM, array elements are osa_nw_rat_e type, priority: rat_order[0] > rat_order[1] > ... > rat_order[QOSA_NW_RAT_SEQ_NUM-1]
 * @param[in] effect
 *          - Whether to take effect immediately, if false, need to restart to make configuration take effect
 * @param[in] cb
 *          - Callback function
 * @return int
 */
int mpc_nw_set_rat_order(qosa_uint8_t simid, qosa_uint8_t rat_order[QOSA_NW_RAT_SEQ_NUM], qosa_bool_t effect, mpc_cb_t cb);

/**
 * @brief Get configured band
 *
 * @param[in] simid
 *          - simid
 * @param[out] cfg_band
 *          - Configured band items
 * @return int
 */
int mpc_nw_get_cfg_band(qosa_uint8_t simid, qosa_nw_band_t *cfg_band);

/**
 * @brief Configure band lock
 * @result
 * +-----------------+------------+--------+--------+
 * | ret             | param1     | param2 | param3 |
 * +-----------------+------------+--------+--------+
 * | MPC_RET_SUCCESS | QOSA_OK     | null   | null   |
 * | MPC_RET_FAIL    | error code | null   | null   |
 * +-----------------+------------+--------+--------+
 *
 * @param[in] simid
 *          - sim id
 * @param[in] cfg_band
 *          - Band lock configuration
 * @param[in] effect
 *          - Whether to take effect immediately, TRUE means require immediate effect, FALSE means deny
 * @param[in] cb
 *          - MPC asynchronous handle
 * @return int
 */
int mpc_nw_set_cfg_band(qosa_uint8_t simid, qosa_nw_band_t *cfg_band, qosa_bool_t effect, mpc_cb_t cb);

/**
 * @brief Get currently registered network PLMN information
 *
 * @param[in] simid
 *          - SIM ID
 * @param[out] long_name
 *          - Long network operator name
 * @param[out] short_name
 *          - Short network operator name
 * @return int
 */
int mpc_nw_get_oper_name(qosa_uint8_t simid, qosa_nw_oper_name_t *long_name, qosa_nw_oper_name_t *short_name);

/**
 * @brief Get ME configured operator name information, need to specify PLMN
 *
 * @param[in] plmn
 *          - Operator PLMN
 * @param[out] long_name
 *          - Long network operator name
 * @param[out] short_name
 *          - Short network operator name
 * @return qosa_nw_err_e
 */
int mpc_nw_get_cfg_oper_name(qosa_plmn_t *plmn, qosa_nw_oper_name_t *long_name, qosa_nw_oper_name_t *short_name);

/**
 * @brief Scan to get currently supported operator list
 * @result
 * +-----------------+-------------------------+--------+--------+
 * | ret             | param1                  | param2 | param3 |
 * +-----------------+-------------------------+--------+--------+
 * | MPC_RET_SUCCESS | mpc_nw_supp_oper_list_t | null   | null   |
 * | MPC_RET_FAIL    | error code              | null   | null   |
 * +-----------------+-------------------------+--------+--------+
 *
 * @param[in] simid
 *          -
 * @param[in] cb
 *          - MPC asynchronous handle
 * @return int
 */
int mpc_nw_query_supp_operator(qosa_uint8_t simid, mpc_cb_t cb);

/**
 * @brief Perform automatic network search
 * @result
 * +-----------------+------------+--------+--------+
 * | ret             | param1     | param2 | param3 |
 * +-----------------+------------+--------+--------+
 * | MPC_RET_SUCCESS | QOSA_OK     | null   | null   |
 * | MPC_RET_FAIL    | error code | null   | null   |
 * +-----------------+------------+--------+--------+
 *
 * @param[in] simid
 *          -
 * @param[in] cb
 *          - MPC asynchronous handle
 * @return int
 */
int mpc_nw_auto_register(qosa_uint8_t simid, mpc_cb_t cb);

/**
 * @brief
 * @result
 * +-----------------+------------+--------+--------+
 * | ret             | param1     | param2 | param3 |
 * +-----------------+------------+--------+--------+
 * | MPC_RET_SUCCESS | QOSA_OK     | null   | null   |
 * | MPC_RET_FAIL    | error code | null   | null   |
 * +-----------------+------------+--------+--------+
 *
 * @param[in] simid
 *          - SIM ID
 * @param[in] format
 *          - Format of operator name, can be long or short operator name, or PLMN ID string
 * @param[in] plmn_str
 *          - Operator name string
 * @param[in] act
 *          - Network RAT, OSA_NW_ACT_UNKNOWN if not specified by user
 * @param[in] fail_to_auto
 *          - Whether to perform automatic network search after manual network selection fails
 * @param[in] cb
 *          - MPC asynchronous handle
 * @return int
 */
int mpc_nw_manual_register(qosa_uint8_t simid, qosa_nw_oper_name_format_e format, char *plmn_str, qosa_nw_act_e act, qosa_bool_t fail_to_auto, mpc_cb_t cb);

/**
 * @brief
 * @result
 * +-----------------+------------+--------+--------+
 * | ret             | param1     | param2 | param3 |
 * +-----------------+------------+--------+--------+
 * | MPC_RET_SUCCESS | QOSA_OK     | null   | null   |
 * | MPC_RET_FAIL    | error code | null   | null   |
 * +-----------------+------------+--------+--------+
 *
 * @param[in] simid
 *          - SIM ID
 * @param[in] mode
 *          - PLMN network selection mode
 * @return int
 */
int mpc_nw_get_curr_oper_info(qosa_uint8_t simid, qosa_uint8_t *mode);

/**
 * @brief Network deregistration for COPS=2
 * @result
 * +-----------------+------------+--------+--------+
 * | ret             | param1     | param2 | param3 |
 * +-----------------+------------+--------+--------+
 * | MPC_RET_SUCCESS | QOSA_OK     | null   | null   |
 * | MPC_RET_FAIL    | error code | null   | null   |
 * +-----------------+------------+--------+--------+
 *
 * @param[in] simid
 *          -
 * @param[in] cb
 *          - MPC asynchronous handle
 * @return int
 */
int mpc_nw_deregister(qosa_uint8_t simid, mpc_cb_t cb);

/**
 * @brief Start no card frequency scan
 * @result
 * +-----------------+--------------------------------+--------+--------+
 * | ret             | param1                         | param2 | param3 |
 * +-----------------+--------------------------------+--------+--------+
 * | MPC_RET_SUCCESS | qosa_nw_freq_scan_cell_list_t * | null   | null   |
 * | MPC_RET_FAIL    | error code                     | null   | null   |
 * +-----------------+--------------------------------+--------+--------+
 *
  * @param[in] simid
  *          - sim id
  * @param[in] config
  *          - Scan configuration parameters
  * @param[in] cb
  *          - MPC asynchronous handle
  * @return int
  */
int mpc_nw_trigger_freq_scan(qosa_uint8_t simid, qosa_nw_freq_scan_t *config, mpc_cb_t cb);

/**
 * @brief Query packet domain/circuit domain registration mode information
 *
 * @param[in] simid
 *          - sim id
 * @param[out] domain
 *          - Packet domain/circuit domain registration mode parameter reference qosa_nw_srv_domain_e
 * @return int
 */
int mpc_nw_get_srv_domain(qosa_uint8_t simid, qosa_uint8_t *domain);

/**
 * @brief Set packet domain/circuit domain registration mode
 * @result
 * +-----------------+------------+--------+--------+
 * | ret             | param1     | param2 | param3 |
 * +-----------------+------------+--------+--------+
 * | MPC_RET_SUCCESS | QOSA_OK     | null   | null   |
 * | MPC_RET_FAIL    | error code | null   | null   |
 * +-----------------+------------+--------+--------+
 *
 * @param[in] simid
 *          - sim id
 * @param[in] domain
 *          - Packet domain/circuit domain registration mode parameter reference qosa_nw_srv_domain_e
 * @param[in] cb
 *          - MPC asynchronous handle
 * @return int
 */
int mpc_nw_set_srv_domain(qosa_uint8_t simid, qosa_uint8_t domain, mpc_cb_t cb);

/**
 * @brief Get voice domain registration mode information
 *
 * @param[in] simid
 *          - sim id
 * @param[in] domain
 *          - Voice domain registration mode parameter reference qosa_nw_voice_domain_e
 * @return int
 */
int mpc_nw_get_voice_domain(qosa_uint8_t simid, qosa_uint8_t *domain);

/**
 * @brief Set voice domain registration mode
 * @result
 * +-----------------+------------+--------+--------+
 * | ret             | param1     | param2 | param3 |
 * +-----------------+------------+--------+--------+
 * | MPC_RET_SUCCESS | QOSA_OK     | null   | null   |
 * | MPC_RET_FAIL    | error code | null   | null   |
 * +-----------------+------------+--------+--------+
 *
 * @param[in] simid
 *          - sim id
 * @param[in] domain
 *          - Voice domain registration mode parameter reference qosa_nw_voice_domain_e
 * @param[in] cb
 *          - MPC asynchronous handle
 * @return int
 */
int mpc_nw_set_voice_domain(qosa_uint8_t simid, qosa_uint8_t domain, mpc_cb_t cb);

/**
 * @brief get UE's usage setting
 *
 * @param[in] simid
 *          - sim id
 * @param[in] setting
 *          - UE usage setting, parameter reference qosa_nw_ue_usage_setting_e
 * @return int
 */
int mpc_nw_get_ue_usage_setting(qosa_uint8_t simid, qosa_uint8_t *setting);

/**
 * @brief set UE's usage setting
 *
 * @param[in] simid
 *          - sim id
 * @param[in] setting
 *          - UE usage setting, parameter reference qosa_nw_ue_usage_setting_e
 * @param[in] cb
 *          - MPC asynchronous handle
 * @return int
 */
int mpc_nw_set_ue_usage_setting(qosa_uint8_t simid, qosa_uint8_t setting, mpc_cb_t cb);

/**
 * @brief Get roaming preference setting
 *
 * @param[in] simid
 *          - sim id
 * @param[out] setting
 *          - Roaming preference setting, parameter reference qosa_nw_roaming_pref_e
 * @return int
 */
int mpc_nw_get_roaming_pref(qosa_uint8_t simid, qosa_uint8_t *setting);

/**
 * @brief Set roaming preference setting
 *
 * @param[in] simid
 *          - sim id
 * @param[in] setting
 *          - Roaming preference setting, parameter reference qosa_nw_roaming_pref_e
 * @param[in] effect
 *          - Effect mode, 0-take effect after restart, 1-take effect immediately
 * @param[in] cb
 *          - MPC asynchronous handle
 * @return int
 */
int mpc_nw_set_roaming_pref(qosa_uint8_t simid, qosa_uint8_t setting, qosa_uint8_t effect, mpc_cb_t cb);

/**
 * @brief Set frequency lock and unlock
 * @result
 * +-----------------+------------+--------+--------+
 * | ret             | param1     | param2 | param3 |
 * +-----------------+------------+--------+--------+
 * | MPC_RET_SUCCESS | QOSA_OK     | null   | null   |
 * | MPC_RET_FAIL    | error code | null   | null   |
 * +-----------------+------------+--------+--------+
 *
 * @param[in] simid
 *          - sim id
 * @param[in] rat
 *          - RAT type for frequency lock operation
 * @param[in] opcode
 *          - qosa_nw_freq_lock_opcode_e Frequency lock operation code, lock frequency band or unlock
 * @param[in] freq_lock
 *          - Frequency lock list
 * @param[in] cb
 *          - MPC asynchronous handle
 * @return int
 */
int mpc_nw_freq_lock(qosa_uint8_t simid, qosa_uint8_t rat, qosa_uint8_t opcode, qosa_nw_freq_lock_list_t *freq_lock, mpc_cb_t cb);

/**
 * @brief Get frequency lock configuration
 * @note If the returned num is 0, it means no lock
 *
 * @param[in] simid
 *          - sim id
 * @param[in] rat
 *          - RAT type for frequency lock operation
 * @param[in] freq_lock
 *          - Frequency lock list
 * @return int
 */
int mpc_nw_get_freq_lock_config(qosa_uint8_t simid, qosa_uint8_t rat, qosa_nw_freq_lock_list_t *freq_lock_list);

/**
 * @brief Set default configured NSSAI
 *
 * @param[in] simid
 *          - SIM card identifier
 *          - 0 represents SIM card 1, 1 represents SIM card 2
 * @param[in] length
 *          - Length of NSSAI, number of bytes in NSSAI data, not nssai_data string length
 * @param[in] nssai_data
 *          - NSSAI configuration data
 *          - String form, multiple S-NSSAI separated by colon `:`
 * @return int
 *          - Return 0 on success
 *          - Return corresponding error code on failure
 */
int mpc_nw_nssai_set_default_config(qosa_uint8_t simid, qosa_uint8_t length, const char *nssai_data);

/**
 * @brief Get NSSAI configuration
 *
 * @param[in] simid
 *          - SIM card identifier
 * @param[in] type
 *          - NSSAI configuration type, reference qosa_nw_nssai_type_e
 * @param[in] plmn
 *          - Specified plmn, if OSA_NULL, means no PLMN information
 * @param[out] list
 *          - Returned NSSAI configuration list
 * @return int
 */
int mpc_nw_nssai_get_nssai(qosa_uint8_t simid, qosa_nw_nssai_type_e type, qosa_plmn_t *plmn, qosa_nw_nssai_list_t *list);

/**
 * @brief Set rrc fast release configuration
 *
 * @param[in] simid
 *          - sim id
 * @param[in] fast_release
 *          - rrc fast release configuration
 * @param[in] cb
 *          - MPC asynchronous handle
 * @return int
 */
int mpc_nw_set_rrc_fast_release(qosa_uint8_t simid, qosa_nw_rrc_fast_release_t *fast_release, mpc_cb_t cb);

/**
 * @brief Get rrc fast release configuration
 *
 * @param[in] simid
 *          - SIM card identifier
 * @param[out] rrc_fast_release
 *          - rrc fast release configuration
 * @return int
 */
int mpc_nw_get_rrc_fast_release(qosa_uint8_t simid, qosa_nw_rrc_fast_release_t *rrc_fast_release);

/**
 * @brief rrc fast release
 *
 * @return int
 */
int mpc_nw_rrc_fast_release(qosa_uint8_t simid);
/**
 * @brief Get nitz time information
 *
 * @param[in] simid
 *          - SIM card identifier
 * @param[out] nitz_info
 *          - nitz information
 * @return int
 */
int mpc_nw_get_nitz_time_info(qosa_uint8_t simid, qosa_nw_nitz_event_t *nitz_info);

/**
 * @brief Set PSM mode
 *
 * @param[in] simid
 *          - SIM card identifier
 * @param[in] psm_enable
 *          - Whether psm is enabled
 * @param[in] periodic_tau_time
 *          - String format, TAU TIME value
 * @param[in] active_time
 *          - String format, ACTIVE TIME value
 * @return int
 */
int mpc_nw_set_psm(qosa_uint8_t simid, qosa_bool_t psm_enable, char *periodic_tau_time, char *active_time);

/**
 * @brief Get PSM mode configuration
 * @note 1. If enable is false, no requirement for string return values;
 *       2. If true, should return user configured TAU/active time values; if user has not configured, expect to return a default value consistent with actual situation, at least need to return an empty string, not allowed to return NULL.
 *       3. Input parameters psm_enable/periodic_tau_time/active_time are allowed to be empty
 * @param[in] simid
 *          - SIM card identifier
 * @param[out] psm_enable
 *          - Whether bottom layer PSM mode is enabled
 * @param[out] periodic_tau_time
 *          - tau time value, string format;
 * @param[out] active_time
 *          -
 * @return int
 */
int mpc_nw_get_psm(qosa_uint8_t simid, qosa_bool_t *psm_enable, char *periodic_tau_time, char *active_time);

/**
 * @brief Set Jamming detection function
 *
 * @result
 * +-----------------+-------------------------------------+--------+--------+
 * | ret             | param1                              | param2 | param3 |
 * +-----------------+-------------------------------------+--------+--------+
 * | MPC_RET_SUCCESS | qosa_nw_jamm_detect_setting_param_t | null   | null   |
 * | MPC_RET_FAIL    | error code                          | null   | null   |
 * +-----------------+-------------------------------------+--------+--------+
 *
 * @param[in] simid
 *          - sim ID
 * @param[in] jdc_func
 *          - Jamming detection parameter information
 * @param[in] cb
 *          - MPC asynchronous handle
 * @return int
 */
int mpc_nw_set_jamm_detect_func(qosa_uint8_t simid, qosa_nw_jamm_detect_setting_param_t *jdc_func, mpc_cb_t cb);

/**
 * @brief Get Jamming detection function
 *
 * @result
 * +-----------------+-------------------------------------+--------+--------+
 * | ret             | param1                              | param2 | param3 |
 * +-----------------+-------------------------------------+--------+--------+
 * | MPC_RET_SUCCESS | qosa_nw_jamm_detect_setting_param_t | null   | null   |
 * | MPC_RET_FAIL    | error code                          | null   | null   |
 * +-----------------+-------------------------------------+--------+--------+
 *
 * @param[in] jdc
 *          - Jamming detection parameter information
 * @return int
 */
int mpc_nw_get_jamm_detect_func(qosa_uint8_t simid, mpc_cb_t cb);

/**
 * @brief Set ue operation mode
 *
 * @param[in] simid
 *          - SIM card identifier
 * @param[in] mode
 *          - ue operation mode
 * @param[in] cb
 *          - async callback function
 * @return int
 */
int mpc_nw_set_ue_operation_mode(qosa_uint8_t simid, qosa_uint8_t *mode, mpc_cb_t cb);

/**
 * @brief Get ue operation mode
 *
 * @param[in] simid
 *          - SIM card identifier
 * @param[out] mode
 *          - ue operation mode
 * @return int
 */int mpc_nw_get_ue_operation_mode(qosa_uint8_t simid, qosa_uint8_t *mode);

/**
 * @brief Set T3402
 *
 * @param[in] simid
 *          - SIM card identifier
 * @param[in] value
 *          - T3402 value
 * @return int
 */
int mpc_nw_blacklist_set_t3402(qosa_uint8_t simid, qosa_uint32_t value);

/**
 * @brief Get T3402
 *
 * @param[in] simid
 *          - SIM card identifier
 * @param[in] value
 *          - T3402 value
 * @return int
 */
int mpc_nw_blacklist_get_t3402(qosa_uint8_t simid, qosa_uint32_t *value);

/**
 * @brief Set UE cell selection parameters
 *
 * @param[in] simid
 *          - SIM ID
 *
 * @param[out] all_param
 *          - parameter reference qosa_nw_cell_select_param_t
 *
  * @param[in] cb
 *          - async callback function
 * @return int
 */
int mpc_nw_set_cell_select_param(qosa_uint8_t simid, qosa_nw_cell_select_param_t *all_param);

/**
 * @brief Get UE cell selection parameters
 *
 * @param[in] simid
 *          - SIM ID
 *
 * @param[out] all_param
 *          - parameter reference qosa_nw_cell_select_param_t
 *
 *  * @param[in] cb
 *          - async callback function
 * @return int
 */
int mpc_nw_get_cell_select_param(qosa_uint8_t simid, qosa_nw_cell_select_param_t *all_param);

/**
 * @brief Set UE cell blacklist parameters
 *
 * @param[in] simid
 *          - SIM ID
 *
 * @param[in] all_param
 *          - parameter reference qosa_nw_blacklist_t, contains blacklist configuration
 * @return int
 */
int mpc_nw_set_blacklist_param(qosa_uint8_t simid, qosa_nw_blacklist_t *all_param);

/**
 * @brief Get UE cell blacklist parameters
 *
 * @param[in] simid
 *          - SIM ID
 *
 * @param[out] all_param
 *          - parameter reference qosa_nw_blacklist_t, to store the queried blacklist information
 * @return int
 */
int mpc_nw_get_blacklist_param(qosa_uint8_t simid, qosa_nw_blacklist_t *all_param);

/**
 * @brief Set IMS mode configuration via MPC network interface
 *
 * This function configures the IMS mode for the specified SIM card through the MPC network interface, including IMS enable setting and VoLTE capability flag.
 *
 * @param[in] simid
 *          - Identifier of the SIM card to configure
 * 
 * @param[in] ims_conf
 *          - IMS configuration value (e.g., enable or disable)
 * @return int
 */
int mpc_nw_set_ims_mode(qosa_uint8_t simid, qosa_uint8_t ims_conf);

/**
 * @brief Get IMS mode configuration via MPC network interface
 *
 * This function retrieves the current IMS mode settings for the specified SIM card through the MPC network interface, including IMS enable status and VoLTE capability.
 *
 * @param[in] simid
 *          - Identifier of the SIM card to query
 * 
 * @param[out] ims_conf
 *          - Pointer to a variable that will receive the IMS configuration value
 * 
 * @param[out] volte_cap
 *          - Pointer to a variable that will receive the VoLTE capability flag
 * @return int
 */
int mpc_nw_get_ims_mode(qosa_uint8_t simid, qosa_uint8_t *ims_conf, qosa_bool_t *volte_cap);

#ifndef CONFIG_QOSA_NW_FUNC
/**
 * @brief Get registration status, used for registration status query by other functional modules when NW module is trimmed
 * @note Output parameters cs_status and ps_status are allowed to be empty, parameter values refer to stat parameter value of creg command, 1&5 indicate successful registration
 *
 * @param[in] simid
 *          - simid
 * @param[out] cs_status
 *          - CS domain network registration status
 * @param[out] ps_status
 *          - PS domain network registration status
 * @return int
 */
int mpc_nw_get_reg_status(qosa_uint8_t simid, qosa_uint8_t *cs_status, qosa_uint8_t *ps_status);
#endif

/**
 * @brief Get UE cell blacklist parameters
 *
 * @param[in] simid
 *          - SIM ID
 *
 * @param[out] enable
 *          - parameter reference qosa_bool_t, to store the queried autoapn enable information
 * @return int
 */
int mpc_nw_get_autoapn_param(qosa_uint8_t simid, qosa_bool_t *enable);
/**
 * @brief Set UE cell blacklist parameters
 *
 * @param[in] simid
 *          - SIM ID
 *
 * @param[in] enable
 *          - parameter reference qosa_bool_t, to store the queried autoapn enable information
 * @return int
 */
int mpc_nw_set_autoapn_param(qosa_uint8_t simid, qosa_bool_t enable);

#endif// !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 

#endif /* __MPC_NETWORK__H__ */
