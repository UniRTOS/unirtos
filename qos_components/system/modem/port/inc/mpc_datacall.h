/*****************************************************************/ /**
* @file mpc_network.h
* @brief Data call operation interface for modem port layer
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
#ifndef __MPC_DATACALL__H__
#define __MPC_DATACALL__H__

#include "mpc.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#include "qosa_datacall.h"

/** LIGHT MODE MPC_DATACALL is defined as built-in protocol stack application and OPEN API usage
 * Operation scope 1: DATACALL management scope is bottom-layer PDP entity operations and information queries, similar to CGACT.
 * Operation scope 2: Responsible for handling DATACALL requests from built-in protocol stack side applications, status queries, etc.
 * For usbnet, PPP, BIP, AGPS and other internal application datacall, there is no query or operation capability.
 *
 */

/** PDP usage mode which types of applications are in use */
#define MPC_PDP_USED_FLAG_USBNET_ACTIVE    0x01
#define MPC_PDP_USED_FLAG_PPP_ACTIVE       0x02
#define MPC_PDP_USED_FLAG_TCPIP_ACTIVE     0x04
#define MPC_PDP_USED_FLAG_UNDEFINED_ACTIVE 0x08

typedef enum
{
    MPC_DATACALL_ACT_TYPE_USBNET = QOSA_DATACALL_CONN_USBNET,       /*!< Define activation type for USBNET */
    MPC_DATACALL_ACT_TYPE_PPP = QOSA_DATACALL_CONN_PPP,             /*!< Define activation type for PPP */
    MPC_DATACALL_ACT_TYPE_TCPIP = QOSA_DATACALL_CONN_TCPIP,         /*!< Define activation type for built-in protocol stack */
    MPC_DATACALL_ACT_TYPE_UNDEFINED = QOSA_DATACALL_CONN_UNDEFINED, /*!< Undefined purpose activation type, such as CGACT */
} mpc_datacall_act_type_e;

#ifdef CONFIG_QOSA_DATACALL_LIGHT_MODE
/**
 * @brief PDP status
 *
 */
typedef enum
{
    MPC_PDP_STATE_DEACTIVATED, /*!< PDP deactivated */
    MPC_PDP_STATE_ACTIVATED,   /*!< PDP activated */
} mpc_datacall_pdp_state_e;

typedef struct
{
    qosa_uint32_t pdp_state     : 1;  /*!< mpc_datacall_pdp_state_e, PDP deactivated or activated status */
    qosa_uint32_t in_nat_mode   : 1;  /*!< Whether in nat mode, 0: non-nat mode, 1: nat mode */
    qosa_uint32_t pdp_used_flag : 16; /*!< PDP usage mode which types of applications are in use, refer to MPC_PDP_USED_FLAG_xx */
    qosa_uint32_t reserve       : 14; /*!< reserve */
} mpc_datacall_pdp_state_t;
#endif

typedef struct
{
    qosa_uint8_t  challenge_len; /*!< Complete challenge message length */
    qosa_uint8_t *challenge;     /*!< Complete challenge message */
    qosa_uint8_t  response_len;  /*!< Complete response message length */
    qosa_uint8_t *response;      /*!< Complete response message */
} mpc_datacall_chap_challenge_t;

/**
 * @brief Network side deactivation trigger reported IND event.
 * @note: status: 0: deactivated, 1: activated
 *
 * +-----------------------------------+--------+-----------------+--------+
 * | ret                               | param1 | param2          | param3 |
 * +-----------------------------------+--------+-----------------+--------+
 * | MPC_EVENT_DATACALL_PDP_STATUS_IND | PDP ID | STATUS( 0 OR 1) | null   |
 * +-----------------------------------+--------+-----------------+--------+
 *
 */

/**
 * @brief Platform layer underlying application performs activation/deactivation operation, synchronizes status through this IND.
 * @note: status: 1bit, 0: deactivated, 1: activated
 *
 * +-----------------------------------------------+--------+------------------------+--------+
 * | ind_id                                        | param1 | param2                 | param3 |
 * +-----------------------------------------------+--------+------------------------+--------+
 * | MPC_EVENT_DATACALL_PDP_ACTIVE_STATUS_IND null | PDP ID | [MSB]status:1bit|[LSB] | null   |
 * +-----------------------------------------------+--------+------------------------+--------+
 *
 */

int err_datacall_mpc2ms(int err);

/**
 * @brief datacall attach or detach (AT+CGATT)
 * @result
 * +-----------------+------------+--------+--------+
 * | ret             | param1     | param2 | param3 |
 * +-----------------+------------+--------+--------+
 * | MPC_RET_SUCCESS | QOSA_OK     | null   | null   |
 * | MPC_RET_FAIL    | error code | null   | null   |
 * +-----------------+------------+--------+--------+
 *
 * @param[in] simid
 *          - sim ID
 * @param[in] method
 *          - 0: detach, 1: attach
 * @param[in] cb
 *          -
 * @return int
 */
int mpc_datacall_att(qosa_uint8_t simid, qosa_uint8_t method, mpc_cb_t cb);

/**
 * @brief Activate or deactivate specified PDP context
 * @result
 * +-----------------+---------------+-----------------+--------+
 * | ret             | param1        | param2          | param3 |
 * +-----------------+---------------+-----------------+--------+
 * | MPC_RET_SUCCESS | active status | null:24|pdpid:8 | null   |
 * | MPC_RET_FAIL    | error code    | null:24|pdpid:8 | null   |
 * +-----------------+---------------+-----------------+--------+
 *
 * @param[in] simid
 *          - sim ID
 * @param[in] pdpid
 *          - pdp id
 * @param[in] method
 *          - 0: deactivate 1: activate
 * @param[in] act_type
 *          - When method is 1, this parameter represents activation type, refer to mpc_datacall_act_type_e; when method is 0, this parameter is invalid
 * @param[in] cb
 *          -
 * @return int
 */
int mpc_datacall_act(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_uint8_t method, qosa_uint8_t act_type, mpc_cb_t cb);

#ifdef CONFIG_QOSA_DATACALL_LIGHT_MODE

/**
 * @brief Activate or deactivate specified PDP context
 * @result
 * +-----------------+---------------+-----------------+--------+
 * | ret             | param1        | param2          | param3 |
 * +-----------------+---------------+-----------------+--------+
 * | MPC_RET_SUCCESS | active status | null:24|pdpid:8 | null   |
 * | MPC_RET_FAIL    | error code    | null:24|pdpid:8 | null   |
 * +-----------------+---------------+-----------------+--------+
 *
 * @param[in] simid
 *          - sim ID
 * @param[in] pdpid
 *          - pdp id
 * @param[in] method
 *          - 0: deactivate 1: activate
 * @param[in] act_type
 *          - When method is 1, this parameter represents activation type, refer to mpc_datacall_act_type_e; when method is 0, this parameter is invalid
 * @param[in] cb
 *          -
 * @return int
 */
int mpc_datacall_light_mode_act(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_uint8_t method, qosa_uint8_t act_type, mpc_cb_t cb);

/**
 * @brief Get PDP status, including whether it is in activated state, whether it is in nat mode, and PDP usage status in activated state (which functional domains are using the current PDP)
 *
 * @param[in] simid
 *          - sim id
 * @param[in] pdpid
 *          - pdp id, starting from 1
 * @param[out] state
 *          - Obtained pdp status
 * @return int
 */
int mpc_datacall_get_pdp_state(qosa_uint8_t simid, qosa_uint8_t pdpid, mpc_datacall_pdp_state_t *state);
#endif

/**
 * @brief Get IP information of activated PDP context
 *
 * This interface is used to get the assigned IP information for the specified SIM card and PDP context,
 * including IPv4/IPv6 addresses and DNS server addresses. The returned ip_type is the actual PDN type
 * assigned by the network side, not the user-configured type.
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[in] pdpid
 *          - PDP context ID
 * @param[out] ip_info
 *          - Output parameter, storing the obtained IP address and DNS information
 *
 * @return
 *          - 0: Success
 *          - <0: Failure (error code meaning refers to return value definition)
 */
int mpc_datacall_get_pdp_ip_info(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_datacall_ip_info_t *ip_info);

#ifdef CONFIG_QOSA_DATACALL_MPC_DEFINED_MASK
int mpc_datacall_get_pdp_defined_flag(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_uint8_t *flag);

int mpc_datacall_set_pdp_defined_flag(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_uint8_t flag);
#endif

/**
 * @brief Set specified PDP context.
 *
 * This function is used to configure user-side PDP context information and save it to local storage
 * (such as NV configuration), while updating the global PDP Profile table in memory. After setting,
 * it can be read through related interfaces
 *
 * @param[in] simid
 *          - SIM identifier
 * @param[in] pdpid
 *          - PDP context identifier, starting from 1
 * @param[in] pdp_ctx
 *          - Pointer to PDP context configuration structure, containing parameters to be set
 * @return int
 */
int mpc_datacall_set_pdp_context(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_pdp_context_t *pdp_ctx);

/**
 * @brief Delete specified PDP context.
 *
 * This function will delete user-side configured PDP context information based on SIM card identifier
 * and PDP ID, and synchronously update related local storage (such as NV configuration). After deletion,
 * the configuration information corresponding to that PDP ID will be cleared or reset.
 *
 * @param[in] simid
 *          - sim card ID
 * @param[in] pdpid
 *          - PDP context identifier, starting from 1
 * @return int
 *
 * @note
 *  - Delete operation only affects local configuration, will not directly notify or affect network-side PDP context
 *  - Caller needs to ensure simid and pdpid are within legal range
 */
int mpc_datacall_delete_pdp_context(qosa_uint8_t simid, qosa_uint8_t pdpid);

/**
 * @brief Get user-side configured PDP context.
 *
 * This interface is used to read local user-configured PDP context information, not including network-side issued configuration.
 *
 * @note
 * 1. This interface does not return network-side issued PDP context
 * 2. If the corresponding PDP is not configured, the returned structure will only contain IP type,
 *    its value is defined by the platform; in UniRTOS, it defaults to return QOSA_PDP_TYPE_IPV4V6, other fields are empty
 *
 * @param[in] simid
 *          - sim card ID
 * @param[in] pdpid
 *          - PDP context identifier, starting from 1
 * @param[out] pdp_ctx
 *          - Output parameter, used to store the obtained PDP context information
 * @return int
 */
int mpc_datacall_get_pdp_context(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_pdp_context_t *pdp_ctx);

/**
 * @brief Set authentication information for specified PDP context.
 *
 * This function is used to configure user-side PDP context authentication parameters (such as username, password, authentication method),
 * and save them to local storage (such as NV configuration)
 *
 * @param[in] simid
 *          - SIM identifier
 * @param[in] pdpid
 *          - PDP context identifier, starting from 1
 * @param[in] auth_ctx
 *          -  Pointer to PDP authentication information structure, containing username, password and authentication type
 * @return int
 */
int mpc_datacall_set_pdp_auth(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_pdp_auth_context_t *auth_ctx);

/**
 * @brief Get authentication information for specified PDP context.
 *
 * This function is used to read user-side configured PDP context authentication parameters (such as username, password, authentication type),
 * and return them to the structure provided by the caller
 *
 * @param[in] simid
 *          - sim id
 * @param[in] pdpid
 *          - PDP ID
 * @param[out] auth_ctx
 *          - Output parameter, used to store PDP authentication information
 * @return int
 */
int mpc_datacall_get_pdp_auth(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_pdp_auth_context_t *auth_ctx);

/**
 * @brief Set PDP QoS information
 *
 * @param[in] simid
 *          - sim ID
 * @param[in] pdpid
 *          - pdp ID
 * @param[in] qos_ctx
 *          - qos information
 * @return int
 */
int mpc_datacall_set_pdp_qos(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_pdp_qos_profile_t *qos_ctx);

/**
 * @brief Get PDP QoS information
 *
 * @param[in] simid
 *          - sim ID
 * @param[in] pdpid
 *          - pdp ID
 * @param[in] qos_ctx
 *          - qos information
 * @return int
 */
int mpc_datacall_get_pdp_qos(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_pdp_qos_profile_t *qos_ctx);

/**
 * @brief Set chap authentication information used in PPP application
 *
 * @param[in] simid
 *          - sim id
 * @param[in] pdpid
 *          - pdp id
 * @param[in] chap_challenge
 *          - Related messages
 * @return int
 */
int mpc_datacall_set_chap_challenge(qosa_uint8_t simid, qosa_uint8_t pdpid, mpc_datacall_chap_challenge_t *chap_challenge);

/**
 * @brief Get uplink and downlink traffic statistics information
 *
 * @param[in] simid
 *          - sim ID
 * @param[out] traffic_statistics
 *          - Obtained traffic statistics information
 * @return int
 */
int mpc_datacall_get_traffic_statistics(qosa_uint8_t simid, qosa_datacall_traffic_statistics_t *traffic_statistics);

/**
 * @brief Clear uplink and downlink traffic statistics information
 *
 * @param[in] simid
 *          - sim ID
 * @return int
 */
int mpc_datacall_clear_traffic_statistics(qosa_uint8_t simid);

/**
 * @brief Save uplink and downlink traffic information to NV
 *
 * @param[in] simid
 *          - sim ID
 * @return int
 */
int mpc_datacall_save_traffic_statistics(qosa_uint8_t simid);

/**
 * @brief Get PDP activation/deactivation timer configuration information
 *
 * @param[in] simid
 *          - sim ID
 * @param[in] rat
 *          - Timer belonging rat to be obtained, refer to osa_network.h's osa_nw_rat_e
 * @param[in] procedure
 *          - Type of timer to be obtained, refer to osa_datacall_pdp_act_opt_e, activation or deactivation operation timer
 * @param[in] pdp_timer
 *          - Timer configuration
 * @return int
 */
int mpc_datacall_get_pdp_timer(qosa_uint8_t simid, qosa_uint8_t rat, qosa_uint8_t procedure, qosa_datacall_pdp_timer_t *pdp_timer);

/**
 * @brief Configure PDP activation/deactivation timer configuration
 *
 * @param[in] simid
 *          - sim ID
 * @param[in] rat
 *          - Timer belonging rat to be configured, refer to osa_network.h's osa_nw_rat_e
 * @param[in] procedure
 *          - Type of timer to be configured, refer to osa_datacall_pdp_act_opt_e, activation or deactivation operation timer
 * @param[in] pdp_timer
 *          - Timer configuration
 * @return int
 */
int mpc_datacall_set_pdp_timer(qosa_uint8_t simid, qosa_uint8_t rat, qosa_uint8_t procedure, qosa_datacall_pdp_timer_t *pdp_timer);

/**
 * @brief Get DNS server IP address
 *
 * @param[in] qosa_uint8_t simid
 *          - SIMID to operate and get
 *
 * @param[in] qosa_uint8_t pdpid
 *          - PDPID to operate and get
 *
 * @param[out] qosa_datacall_dns_t * dns
 *          - Obtained DNS address
 *
 * @return int
 *
 */
int mpc_datacall_get_dns_addr(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_datacall_dns_t *dns);

/**
 * @brief Set DNS server IP address
 *
 * @param[in] qosa_uint8_t simid
 *          - SIMID to operate and get
 *
 * @param[in] qosa_uint8_t pdpid
 *          - PDPID to operate and get
 *
 * @param[in] qosa_datacall_dns_t * dns
 *          - DNS address to be modified
 *
 * @return int
 *
 */
int mpc_datacall_set_dns_addr(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_datacall_dns_t *dns);

/**
 * @brief Get dynamic pdp ctx
 *
 * @param[in] qosa_uint8_t simid
 *          - SIMID to operate and get
 *
 * @param[in] qosa_uint8_t pdpid
 *          - PDPID to operate and get
 *
 * @param[out] qosa_pdp_dynamic_context_t * pdp_dynamic_ctx
 *          - PDP dynamic context
 *
 * @return int
 *
 */
int mpc_datacall_get_dynamic_pdp_ctx(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_pdp_dynamic_context_t *pdp_dynamic_ctx);

/**
 * @brief Set pdpreact mode
 * 
 *  pdpreact: When certain conditions are met(e.g., APN change), reconfigure the APN and IP type information for this pdp. 
 *  Permit the initiation of signaling interactions such as PDN connect request to the network.
 * 
 * @param[in] simid
 *          - SIM ID
 *
 * @param[in] enable
 *          - pdpreact configuration. Whether to enable pdpreact: 1 indicates enabled, 0 indicates disabled.
 * 
 * @return int
 */
int mpc_datacall_set_pdpreact_mode(qosa_uint8_t simid, qosa_bool_t enable);

/**
 * @brief Get pdpreact mode
 * 
 *  pdpreact: When certain conditions are met(e.g., APN change), reconfigure the APN and IP type information for this pdp. 
 *  Permit the initiation of signaling interactions such as PDN connect request to the network.
 * 
 * @param[in] simid
 *          - SIM ID
 *
 * @param[out] enable
 *          - pdpreact configuration. Whether to enable pdpreact: 1 indicates enabled, 0 indicates disabled.
 *
 * @return int
 */
int mpc_datacall_get_pdpreact_mode(qosa_uint8_t simid, qosa_bool_t *enable);

#endif//!(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif /* __MPC_DATACALL__H__ */
