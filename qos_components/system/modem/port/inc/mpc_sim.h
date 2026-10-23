/*****************************************************************/ /**
* @file mpc_sim.h
* @brief SIM card operation interface for modem port layer
* @author Joe.tu@quectel.com
* @date 2024-03-18
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description
* <tr><td>2024-03-18 <td>1.0 <td>Joe.tu <td> Init
* </table>
**********************************************************************/
#ifndef __MPC_SIM__H__
#define __MPC_SIM__H__

#include "mpc.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#include "qosa_sim.h"

/** Error code conversion, add corresponding function when adapting to platform, 0 means success */
int err_sim_mpc2ms(qosa_ptr err);

#define MPC_SIM_PIN_VALUE_MAX_LEN (8)

typedef enum
{
    MPC_CHV_1 = 1, /*!< chv1, same as SC */
    MCP_CHV_2,     /*!< chv2, same as P2 */
} mpc_chv_no_e;

/**
 * @enum mpc_sim_status_change_cause_e
 * @brief Cause of SIM STATUS change reported by bottom layer
 *
 */
typedef enum
{
    MPC_SIM_STATUS_CHANGE_CAUSE_NORMAL, /*!< Normal scenario */
    MPC_SIM_STATUS_CHANGE_CAUSE_CFUN0,  /*!< Card disconnection caused by CFUN 0, need to report this cause */
} mpc_sim_status_change_cause_e;

/**
 * @brief Parameter type of MPC_EVENT_SIM_INIT_IND event
 *
 * +------------------------+------------------------------+--------------------------------------+--------+
 * | ind_id                 | param1                       | param2                               | param3 |
 * +------------------------+------------------------------+--------------------------------------+--------+
 * | MPC_EVENT_SIM_INIT_IND | sim_status(qosa_sim_status_e) | cause(mpc_sim_status_change_cause_e) | null   |
 * +------------------------+------------------------------+--------------------------------------+--------+
 *
 */

/**
 * @brief Parameter type of MPC_EVENT_SIM_INSERT_STAT_IND event
 * @note sim_status: 0: removed 1: inserted 2:unknown
 *           slotid: 0: slot1 1:slot2
 *
 * +-------------------------------+-----------------------------+-------------+--------+
 * | ind_id                        | param1                      | param2      | param3 |
 * +-------------------------------+-----------------------------+-------------+--------+
 * | MPC_EVENT_SIM_INSERT_STAT_IND | stat(qosa_sim_insert_stat_e) | slotid(0/1) | null   |
 * +-------------------------------+-----------------------------+-------------+--------+
 *
 */

/**
 * @brief Get SIM card function block initialization status.
 *
 * @param[in] simid
 *          - SIM ID, identifying the SIM card it points to.
 * @return int Return result is in combination form of the following values
 *              - QOSA_SIMINI_STAT_START
 *              - QOSA_SIMINI_STAT_CPIN_READY
 *              - QOSA_SIMINI_STAT_SMS_DONE
 *              - QOSA_SIMINI_STAT_PB_DONE
 */
int mpc_sim_read_init_stat(qosa_uint8_t simid);

/**
 * @brief Get SIM card insertion status.
 * @note  Interface is mainly used to get SIM card insertion status in single card single standby and dual card dual standby scenarios;
 *       If you need to get the insertion status of non-activated SIM card in dual card single standby scenario, please use SLOT_INSERT related interfaces.
 *
 * @param[in] simid
 *          - SIM ID, identifying the SIM card it points to.
 * @return int SIM card insertion status, 0: removed 1: inserted 2:unknown.
 */
int mpc_sim_read_insert_stat(qosa_uint8_t simid);

/**
 * @brief Set SIM card hot swap detection function.
 *
 * @param[in] simid
 *          - SIM ID, identifying the SIM card it points to.
 * @param[in] hot_swap_config
 *          - Hot swap configuration parameters, including whether to enable hot swap function, detected level when SIM card is inserted by detection pin, and hot swap pin number
 * @return int
 */
int mpc_sim_set_sim_hot_swap(qosa_uint8_t simid, qosa_sim_hot_swap_cfg_t* hot_swap_config);

/**
 * @brief Get SIM card hot swap detection function configuration.
 *
 * @param[in] simid
 *          - SIM ID, identifying the SIM card it points to.
 * @param[out] hot_swap_config
 *          - Hot swap configuration parameters, including whether to enable hot swap function, detected level when SIM card is inserted by detection pin, and hot swap pin number
 * @return int
 */
int mpc_sim_get_sim_hot_swap(qosa_uint8_t simid, qosa_sim_hot_swap_cfg_t* hot_swap_config);

/**
 * @brief Get physical card slot used by application layer simid, usually used in dual card single standby scenarios
 *
 * @param[in] simid
 *          - Application layer SIM ID, always 0 in dual card single standby
 * @param[out] slotid
 *          - Actual physical card slot used, 0: card slot 1 1: card slot 2, card slot 1 or 2 are defined in hardware design document
 * @return int
 */
int mpc_sim_get_slot_id(qosa_uint8_t simid, qosa_uint8_t* slotid);

/**
 * @brief Set physical card slot used by application layer simid, usually used in dual card single standby scenarios, if available in dual card dual standby scenarios, it will trigger bottom layer to exchange physical card slots used by simid 0/1.
 * @note Need to first call cfun0, then switch slot id, then cfun4 or cfun1 to successfully switch card
 *
 * @param[in] simid
 *          - Application layer SIM ID, should always be 0 in dual card single standby
 * @param[in] slotid
 *          - Actual physical card slot used, 0: card slot 1 1: card slot 2, card slot 1 or 2 are defined in hardware design document
 * @return int
 */
int mpc_sim_set_slot_id(qosa_uint8_t simid, qosa_uint8_t slotid);

/**
 * @brief get dual SIM mode is typically used in dual SIM dual standby devices.
 *
 * @param[in] simid
 *          - Application layer SIM ID
 * @param[out] type
 *          - Dual SIM type
 * @return int
 */
int mpc_sim_get_ds_dype(qosa_uint8_t simid, qosa_sim_ds_type_e* type);

/**
 * @brief set dual SIM mode is typically used in dual SIM dual standby devices.
 *
 * @param[in] simid
 *          - Application layer SIM ID
 * @param[in] type
 *          - Dual SIM type
 * @return int
 */
int mpc_sim_set_ds_dype(qosa_uint8_t simid, qosa_sim_ds_type_e type);

/**
 * @brief Get IMSI of current card slot SIM card, do real-time query
 * @result
 * +-----------------+------------+--------+--------+
 * | ret             | param1     | param2 | param3 |
 * +-----------------+------------+--------+--------+
 * | MPC_RET_SUCCESS | char *imsi | null   | null   |
 * | MPC_RET_FAIL    | error code | null   | null   |
 * +-----------------+------------+--------+--------+
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[in] cb
 *          - MS layer callback context, asynchronous results need to use this parameter when returned through mpc_post_cnf interface
 * @return int Function returns 0 on successful execution, returns error code in other cases, can be converted to MS layer error code through err_sim_mpc2ms
 */
int mpc_sim_get_imsi(qosa_uint8_t simid, mpc_cb_t cb);

/**
 * @brief Get ICCID information of current card slot sim card, need real-time query
 * @result
 * +-----------------+-------------+--------+--------+
 * | ret             | param1      | param2 | param3 |
 * +-----------------+-------------+--------+--------+
 * | MPC_RET_SUCCESS | char *iccid | null   | null   |
 * | MPC_RET_FAIL    | error code  | null   | null   |
 * +-----------------+-------------+--------+--------+
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[in] cb
 *          - MS layer callback context, asynchronous results need to use this parameter when returned through mpc_post_cnf interface
 * @return int Function returns 0 on successful execution, returns error code in other cases, can be converted to MS layer error code through err_sim_mpc2ms
 */
int mpc_sim_get_iccid(qosa_uint8_t simid, mpc_cb_t cb);

/**
 * @brief Get PIN status
 * @result
 * +-----------------+---------------------------+--------+--------+
 * | ret             | param1                    | param2 | param3 |
 * +-----------------+---------------------------+--------+--------+
 * | MPC_RET_SUCCESS | (qosa_sim_status_e) status | null   | null   |
 * | MPC_RET_FAIL    | error code                | null   | null   |
 * +-----------------+---------------------------+--------+--------+
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[in] cb
 *          - MS layer callback context, asynchronous results need to use this parameter when returned through mpc_post_cnf interface
 * @return int Function returns 0 on successful execution, returns error code in other cases, can be converted to MS layer error code through err_sim_mpc2ms
 */
int mpc_sim_get_pin_status(qosa_uint8_t simid, mpc_cb_t cb);

/**
 * @brief Unlock PIN, check if PIN code is correct
 * @result
 * +-----------------+------------+--------+--------+
 * | ret             | param1     | param2 | param3 |
 * +-----------------+------------+--------+--------+
 * | MPC_RET_SUCCESS | QOSA_OK     | null   | null   |
 * | MPC_RET_FAIL    | error code | null   | null   |
 * +-----------------+------------+--------+--------+
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[in] pin
 *          - PIN code
 * @param[in] cb
 *          - MS layer callback context, asynchronous results need to use this parameter when returned through mpc_post_cnf interface
 * @return int Function returns 0 on successful execution, returns error code in other cases, can be converted to MS layer error code through err_sim_mpc2ms
 */
int mpc_sim_verify_pin(qosa_uint8_t simid, char* pin, mpc_cb_t cb);

/**
 * @brief Change pin code
 * @result
 * +-----------------+------------+--------+--------+
 * | ret             | param1     | param2 | param3 |
 * +-----------------+------------+--------+--------+
 * | MPC_RET_SUCCESS | QOSA_OK     | null   | null   |
 * | MPC_RET_FAIL    | error code | null   | null   |
 * +-----------------+------------+--------+--------+
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[in] chv
 *          - PIN lock type, CHV1 means SC, CHV2 means P2
 * @param[in] old_pin
 *          - Old PIN code
 * @param[in] new_pin
 *          - New PIN code
 * @param[in] cb
 *          - MS layer callback context, asynchronous results need to use this parameter when returned through mpc_post_cnf interface
 * @return int Function returns 0 on successful execution, returns error code in other cases, can be converted to MS layer error code through err_sim_mpc2ms
 */
int mpc_sim_change_pin(qosa_uint8_t simid, mpc_chv_no_e chv, char* old_pin, char* new_pin, mpc_cb_t cb);

/**
 * @brief Disable PIN function
 * @result
 * +-----------------+------------+--------+--------+
 * | ret             | param1     | param2 | param3 |
 * +-----------------+------------+--------+--------+
 * | MPC_RET_SUCCESS | QOSA_OK     | null   | null   |
 * | MPC_RET_FAIL    | error code | null   | null   |
 * +-----------------+------------+--------+--------+
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[in] pin
 *          -
 * @param[in] cb
 *          - MS layer callback context, asynchronous results need to use this parameter when returned through mpc_post_cnf interface
 * @return int Function returns 0 on successful execution, returns error code in other cases, can be converted to MS layer error code through err_sim_mpc2ms
 */
int mpc_sim_disable_pin(qosa_uint8_t simid, char* pin, mpc_cb_t cb);

/**
 * @brief Enable PIN function
 * @result
 * +-----------------+------------+--------+--------+
 * | ret             | param1     | param2 | param3 |
 * +-----------------+------------+--------+--------+
 * | MPC_RET_SUCCESS | QOSA_OK     | null   | null   |
 * | MPC_RET_FAIL    | error code | null   | null   |
 * +-----------------+------------+--------+--------+
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[in] pin
 *          -
 * @param[in] cb
 *          - MS layer callback context, asynchronous results need to use this parameter when returned through mpc_post_cnf interface
 * @return int Function returns 0 on successful execution, returns error code in other cases, can be converted to MS layer error code through err_sim_mpc2ms
 */
int mpc_sim_enable_pin(qosa_uint8_t simid, char* pin, mpc_cb_t cb);

/**
 * @brief Unlock PIN code
 * @result
 * +-----------------+------------+--------+--------+
 * | ret             | param1     | param2 | param3 |
 * +-----------------+------------+--------+--------+
 * | MPC_RET_SUCCESS | QOSA_OK     | null   | null   |
 * | MPC_RET_FAIL    | error code | null   | null   |
 * +-----------------+------------+--------+--------+
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[in] unblock_pin
 *          -
 * @param[in] new_pin
 *          -
 * @param[in] cb
 *          - MS layer callback context, asynchronous results need to use this parameter when returned through mpc_post_cnf interface
 * @return int Function returns 0 on successful execution, returns error code in other cases, can be converted to MS layer error code through err_sim_mpc2ms
 */
int mpc_sim_unblock_pin(qosa_uint8_t simid, char* unblock_pin, char* new_pin, mpc_cb_t cb);

/**
 * @brief Get sim card pin remaining unlock attempts
 * @result
 * +-----------------+-----------------------------+--------+--------+
 * | ret             | param1                      | param2 | param3 |
 * +-----------------+-----------------------------+--------+--------+
 * | MPC_RET_SUCCESS | PUK2:8|PIN2:8|PUK1:8|PIN1:8 | null   | null   |
 * | MCP_RET_FAIL    | error code                  | null   | null   |
 * +-----------------+-----------------------------+--------+--------+
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[in] cb
 *          - MS layer callback context, asynchronous results need to use this parameter when returned through mpc_post_cnf interface
 * @return int Function returns 0 on successful execution, returns error code in other cases, can be converted to MS layer error code through err_sim_mpc2ms
 */
int mpc_sim_pin_remain_retries(qosa_uint8_t simid, mpc_cb_t cb);

/**
 * @brief SIM general access interface
 * @result
 * +-----------------+--------------+------------------+--------+
 * | ret             | param1       | param2           | param3 |
 * +-----------------+--------------+------------------+--------+
 * | MPC_RET_SUCCESS | uint8t* data | uint16t data_len | null   |
 * | MPC_RET_FAIL    | error code   | null             | null   |
 * +-----------------+--------------+------------------+--------+
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[in] cmd_data
 *          -
 * @param[in] data_len
 *          -
 * @param[in] cb
 *          - MS layer callback context, asynchronous results need to use this parameter when returned through mpc_post_cnf interface
 * @return int Function returns 0 on successful execution, returns error code in other cases, can be converted to MS layer error code through err_sim_mpc2ms
 */
int mpc_sim_generic_access(qosa_uint8_t simid, qosa_uint8_t* cmd_data, qosa_uint16_t data_len, mpc_cb_t cb);

/**
 * @brief SIM general logical channel access interface
 * @result
 * +-----------------+--------------+------------------+--------+
 * | ret             | param1       | param2           | param3 |
 * +-----------------+--------------+------------------+--------+
 * | MPC_RET_SUCCESS | uint8t* data | uint16t data_len | null   |
 * | MPC_RET_FAIL    | error code   | null             | null   |
 * +-----------------+--------------+------------------+--------+
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[in] session_id
 *          - Channel ID
 * @param[in] cmd_data
 *          - Command data
 * @param[in] data_len
 *          - Data length
 * @param[in] cb
 *          - MS layer callback context, asynchronous results need to use this parameter when returned through mpc_post_cnf interface
 * @return int Function returns 0 on successful execution, returns error code in other cases, can be converted to MS layer error code through err_sim_mpc2ms
 */
int mpc_sim_generic_logical_channel_access(qosa_uint8_t simid, qosa_uint32_t session_id, qosa_uint8_t* cmd_data, qosa_uint16_t data_len, mpc_cb_t cb);

/**
 * @brief sim restricted access interface
 * @result
 * +-----------------+----------------+--------------+------------------+
 * | ret             | param1         | param2       | param3           |
 * +-----------------+----------------+--------------+------------------+
 * | MPC_RET_SUCCESS | uint16t sw1sw2 | uint8t* data | uint16t data_len |
 * +-----------------+----------------+--------------+------------------+
 * | MPC_RET_FAIL    | error code     | null         | null             |
 * +-----------------+----------------+--------------+------------------+
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[in] cmd
 *          - Command, refer to osa_sim_restricted_access_command_e
 * @param[in] fileid
 *          - File ID
 * @param[in] p1
 *          - Command related parameters, these 3 parameters are needed except for GET RESPONSE and STATUS
 * @param[in] p2
 *          - Command related parameters, these 3 parameters are needed except for GET RESPONSE and STATUS
 * @param[in] p3
 *          - Command related parameters, these 3 parameters are needed except for GET RESPONSE and STATUS
 * @param[in] data
 *          - Command APDU data, can't be null
 * @param[in] path
 *          - Path id, can't be null
 * @param[in] cb
 *          - MS layer callback context, asynchronous results need to use this parameter when returned through mpc_post_cnf interface
 * @return int Function returns 0 on successful execution, returns error code in other cases, can be converted to MS layer error code through err_sim_mpc2ms
 */
int mpc_sim_restricted_access(
    qosa_uint8_t         simid,
    qosa_uint8_t         cmd,
    qosa_uint16_t        fileid,
    qosa_uint8_t         p1,
    qosa_uint8_t         p2,
    qosa_uint8_t         p3,
    qosa_sim_cmd_data_t* data,
    qosa_sim_path_id_t*  path,
    mpc_cb_t             cb
);

/**
 * @brief Open a logical channel, allocated session id is in return result's p1 parameter
 * @result
 * +-----------------+-------------------+--------+--------+
 * | ret             | param1            | param2 | param3 |
 * +-----------------+-------------------+--------+--------+
 * | MPC_RET_SUCCESS | uint32t sessionid | null   | null   |
 * | MPC_RET_FAIL    | error code        | null   | null   |
 * +-----------------+-------------------+--------+--------+
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[in] dfname
 *          - df name, hex value
 * @param[in] dfname_len
 *          - Length of df name
 * @param[in] cb
 *          - MS layer callback context, asynchronous results need to use this parameter when returned through mpc_post_cnf interface
 * @return int Function returns 0 on successful execution, returns error code in other cases, can be converted to MS layer error code through err_sim_mpc2ms
 */
int mpc_sim_open_logical_channel(qosa_uint8_t simid, qosa_uint8_t* dfname, qosa_uint8_t dfname_len, mpc_cb_t cb);

/**
 * @brief Close a logical channel
 * @result
 * +-----------------+------------+--------+--------+
 * | ret             | param1     | param2 | param3 |
 * +-----------------+------------+--------+--------+
 * | MPC_RET_SUCCESS | null       | null   | null   |
 * | MPC_RET_FAIL    | error code | null   | null   |
 * +-----------------+------------+--------+--------+
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[in] sessionid
 *          - sessionid, minimum value 1
 * @param[in] cb
 *          - MS layer callback context, asynchronous results need to use this parameter when returned through mpc_post_cnf interface
 * @return int Function returns 0 on successful execution, returns error code in other cases, can be converted to MS layer error code through err_sim_mpc2ms
 */
int mpc_sim_close_logical_channel(qosa_uint8_t simid, qosa_uint32_t session_id, mpc_cb_t cb);

/**
 * @brief Get facility lock
 * @result
 * +-----------------+---------------+--------+--------+
 * | ret             | param1        | param2 | param3 |
 * +-----------------+---------------+--------+--------+
 * | MPC_RET_SUCCESS | uint8t status | null   | null   |
 * | MPC_RET_FAIL    | error code    | null   | null   |
 * +-----------------+---------------+--------+--------+
 * status: 0-not active, 1-active
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[in] fac
 *          - Currently need to support SC and FD, refer to osa_sim_facility_e
 * @param[in] cb
 *          - MS layer callback context, asynchronous results need to use this parameter when returned through mpc_post_cnf interface
 * @return int Function returns 0 on successful execution, returns error code in other cases, can be converted to MS layer error code through err_sim_mpc2ms
 */
int mpc_sim_get_facility_lock(qosa_uint8_t simid, qosa_uint8_t fac, mpc_cb_t cb);

/**
 * @brief Set facility lock
 * @result
 * +-----------------+------------+--------+--------+
 * | ret             | param1     | param2 | param3 |
 * +-----------------+------------+--------+--------+
 * | MPC_RET_SUCCESS | null       | null   | null   |
 * | MPC_RET_FAIL    | error code | null   | null   |
 * +-----------------+------------+--------+--------+
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[in] fac
 *          -  fac Currently need to support SC and FD
 * @param[in] pin
 *          - PIN code (passwd)
 * @param[in] mode
 *          - 0: unlock 1:lock, do not support 2
 * @param[in] class
 *          - Parameters in 27007
 * @param[in] cb
 *          - MS layer callback context, asynchronous results need to use this parameter when returned through mpc_post_cnf interface
 * @return int Function returns 0 on successful execution, returns error code in other cases, can be converted to MS layer error code through err_sim_mpc2ms
 */
int mpc_sim_set_facility_lock(qosa_uint8_t simid, qosa_sim_facility_e fac, qosa_sim_pin_t* pin, qosa_uint8_t mode, qosa_uint8_t class, mpc_cb_t cb);

/**
 * @brief Get simcard phone number configuration
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[in] phonenumber
 *          -  sim card phone number configuration
 * @return int Function returns 0 on successful execution, returns error code in other cases, can be converted to MS layer error code through err_sim_mpc2ms
 */
int mpc_sim_get_phonenumber(qosa_uint8_t simid, qosa_sim_phonenumber_t* phonenumber);

/**
* @brief Get the update count information of a specific file
* @result
* + - - - - - - - - - - - - - - - - - + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - + - - - - - - - - - - - - - - - - - - - - - - - - - - - + - - - - - - - - +
* | ret             | param1                                  | param2                    | param3 |
* + - - - - - - - - - - - - - - - - - + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - + - - - - - - - - - - - - - - - - - - - - - - - - - - - + - - - - - - - - +
* | MPC_RET_SUCCESS | qosa_sim_ef_info_t ef_info[ef_id_cnt]   | qosa_uint8_t ef_id_cnt    | null   |
* | MPC_RET_FAIL    | error code                              | null                      | null   |
* + - - - - - - - - - - - - - - - - - + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - + - - - - - - - - - - - - - - - - - - - - - - - - - - - + - - - - - - - - +
*
* @param[in] simid
*            - Application layer SIM ID, identifying the SIM card to be read
*              It is always 0 in dual SIM single standby mode
* @param[in] ef_id
*            - List of specific file ids
*              For example, {0x6F5B, 0X6F7B, 0X6F7E, 0X6F73, 0X6F08, 0X6F09, 0X6FE4, 0X6FE3}
* @param[in] ef_id_cnt
*            - Specific number of files
* @param[in] cb
*            - Callback function
* @return int
*/
int mpc_sim_get_file_update_count(qosa_uint8_t simid, qosa_uint16_t* ef_id, qosa_uint8_t ef_id_cnt, mpc_cb_t cb);

/**
 * @brief get SIM card write protection configure
 * @result
 * +-----------------+-------------------+-----------+--------+
 * | ret             | param1            | param1    | param3 |
 * +-----------------+-------------------+-----------+--------+
 * | MPC_RET_SUCCESS | enable            | bit_map   | null   |
 * | MPC_RET_FAIL    | error code        | null      | null   |
 * +-----------------+-------------------+-----------+--------+
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[in] cb
 *          - MS layer callback context, asynchronous results need to use this parameter when returned through mpc_post_cnf interface
 * @return int Function returns 0 on successful execution, returns error code in other cases, can be converted to MS layer error code through err_sim_mpc2ms
 */
int mpc_sim_get_simwprotect_mode(qosa_uint8_t simid, mpc_cb_t cb);

/**
 * @brief set SIM card write protection configure
 * @result
 * +-----------------+---------------+--------+--------+
 * | ret             | param1        | param2 | param3 |
 * +-----------------+---------------+--------+--------+
 * | MPC_RET_SUCCESS | null          | null   | null   |
 * | MPC_RET_FAIL    | error code    | null   | null   |
 * +-----------------+---------------+--------+--------+
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[in] enable
 *          - 0:not active 1:active, bitmap valid 2:active , bitmap invalid; the file protection list uses the default value.
 * @param[in] bit_map
 *          - Files that need to be write protected
 * @param[in] cb
 *          - MS layer callback context, asynchronous results need to use this parameter when returned through mpc_post_cnf interface
 * @return int Function returns 0 on successful execution, returns error code in other cases, can be converted to MS layer error code through err_sim_mpc2ms
 */
int mpc_sim_set_simwprotect_mode(qosa_uint8_t simid, qosa_uint8_t enable, qosa_uint8_t bit_map, mpc_cb_t cb);

#endif// !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif
