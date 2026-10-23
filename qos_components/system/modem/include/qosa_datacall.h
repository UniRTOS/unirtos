/*****************************************************************/ /**
* @file qosa_datacall.h
* @brief Header file for datacall module, including data connection related definitions and interfaces
* @author Joe.tu@quectel.com
* @date 2024-05-13
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2024-05-13 <td>1.0 <td>Joe.tu <td> Init
* </table>
**********************************************************************/

#ifndef __QOSA_DATACALL_H__
#define __QOSA_DATACALL_H__

#include "qosa_def.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#include "qosa_datacall_config.h"
#include "qosa_modem.h"
#include "qosa_ip_addr.h"
#include "qosa_network.h"

/**
 * @brief Definition of datacall callback function pointer type
 *
 * @param[in] ctx Callback function context pointer, used to pass user-defined data
 * @param[in] argv Callback function parameter pointer, containing result information of asynchronous operations
 */
typedef void (*datacall_callback_cb_ptr)(void* ctx, void* argv);

#if defined (CONFIG_QOSA_NW_NB_SUPPORT) || defined(CONFIG_QOSA_NW_BJU_SUPPORT)
#define QOSA_PDPID_MIN (0)
#define QOSA_PDPID_MAX (CONFIG_QOSA_DATACALL_PDPID_MAX)
#else
/** Minimum value of PDP CID */
#define QOSA_PDPID_MIN (1)
/** Maximum value of PDP CID */
#define QOSA_PDPID_MAX (CONFIG_QOSA_DATACALL_PDPID_MAX)
#endif /*CONFIG_QOSA_NW_NB_SUPPORT*/

/** pdpid, pdp cid, profile id are different names for the same transaction, modem side should use pdpid to describe it uniformly */
#define QOSA_PDP_CID_MIN           (QOSA_PDPID_MIN)
#define QOSA_PDP_CID_MAX           (QOSA_PDPID_MAX)

/** Invalid datacall connection object value */
#define QOSA_DATACALL_CONN_INVALID (0x7fffffff)

/** Maximum length of APN name */
#define QOSA_APN_MAX_LEN           (99)
/** Maximum length of PDP username */
#define QOSA_PDP_USER_NAME_MAX_LEN (64)
/** Maximum length of PDP password */
#define QOSA_PDP_USER_PWD_MAX_LEN  (64)
/** Maximum num of PDP DNS addr */
#define QOSA_PDP_DNS_MAX_NUM       (4)
/** Maximum num of PDP IP addr */
#define QOSA_PDP_IPADDR_MAX_NUM    (2)

/** Base value of datacall error code */
#define QOSA_ERRCODE_DATACALL_BASE (QOSA_COMPONENT_DATACALL << 16)

// clang-format off
/**
 * @enum qosa_datacall_error_e
 * @brief datacall error codes
 *
 */
typedef enum
{
    QOSA_DATACALL_OK = 0,
    QOSA_DATACALL_ERR_OPERATION_NOT_ALLOWED   = 3 | QOSA_ERRCODE_DATACALL_BASE, /*!< operation not allowed */
    QOSA_DATACALL_ERR_OPERATION_NOT_SUPPORTED = 4 | QOSA_ERRCODE_DATACALL_BASE, /*!< operation not supported */
    QOSA_DATACALL_ERR_MEMORY_FULL             = 20 | QOSA_ERRCODE_DATACALL_BASE, /*!< memory full */
    QOSA_DATACALL_ERR_MEMORY_FAILURE          = 23 | QOSA_ERRCODE_DATACALL_BASE, /*!< memory failure */
    QOSA_DATACALL_ERR_INVALID_PARAM           = 53 | QOSA_ERRCODE_DATACALL_BASE, /*!< invalid parameter */
    QOSA_DATACALL_ERR_EXECUTE                 = 1 | (QOSA_ERRCODE_DATACALL_BASE + QOSA_AT_ERR_OFS),
    QOSA_DATACALL_ERR_TIMEOUT,                          /*!< Network registration timeout */
    QOSA_DATACALL_ERR_NO_ACTIVE,                        /*!< PDP not activated */
    QOSA_DATACALL_ERR_PDP_NO_DEFINED,                   /*!< PDP not defined error */
} qosa_datacall_errno_e;
// clang-format on

/**
 * @enum qosa_datacall_attach_stat_e
 * @brief datacall attachment status
 */
typedef enum
{
    QOSA_DATACALL_DETACH, /*!< Detach from network */
    QOSA_DATACALL_ATTACH, /*!< Attach to network */
} qosa_datacall_attach_stat_e;

/**
 * @enum qosa_datacall_pdp_act_opt_e
 * @brief PDP activation status, activation failed or activation successful
 */
typedef enum
{
    QOSA_PDP_OPT_DEACTIVE = 0, /*!< Not activated */
    QOSA_PDP_OPT_ACTIVE = 1,   /*!< Activated */
    QOSA_PDP_OPT_MAX
} qosa_datacall_pdp_act_opt_e;

/**
 * @enum qosa_datacall_conn_type_e
 * @brief datacall connection type
 */
typedef enum
{
    QOSA_DATACALL_CONN_USBNET = 1, /*!< Pointing to usbnet connection function */
    QOSA_DATACALL_CONN_PPP,        /*!< Pointing to PPP connection function */
    QOSA_DATACALL_CONN_TCPIP,      /*!< Pointing to all protocol stack functions, such as TCPIP, MQTT, FTP, etc. */
    QOSA_DATACALL_CONN_UNDEFINED,  /*!< Non-specific connection domain */
    QOSA_DATACALL_CONN_MAX,
} qosa_datacall_conn_type_e;

/**
 * @enum qosa_pdp_data_comp_type_e
 * @brief PDP data compression type
 */
typedef enum
{
    QOSA_PDP_DATA_COMP_OFF = 0, /*!< Data compression off */
    QOSA_PDP_DATA_COMP_ON = 1,  /*!< Data compression on */
    QOSA_PDP_DATA_COMP_V42 = 2, /*!< V.42 data compression */
    QOSA_PDP_DATA_COMP_V44 = 3, /*!< V.44 data compression */
} qosa_pdp_data_comp_type_e;

/**
 * @enum qosa_pdp_head_comp_type_e
 * @brief PDP header compression type
 */
typedef enum
{
    QOSA_PDP_HEAD_COMP_OFF = 0,     /*!< Header compression off */
    QOSA_PDP_HEAD_COMP_ON = 1,      /*!< Header compression on */
    QOSA_PDP_HEAD_COMP_RFC1144 = 2, /*!< RFC1144 header compression */
    QOSA_PDP_HEAD_COMP_RFC2507 = 3, /*!< RFC2507 header compression */
    QOSA_PDP_HEAD_COMP_RFC3095 = 4, /*!< RFC3095 header compression */
} qosa_pdp_head_comp_type_e;

/**
 * @enum qosa_pdp_ipv4_addr_alloc_type_e
 * @brief PDP IPv4 address allocation type
 */
typedef enum
{
    QOSA_PDP_ADDR_ALLOC_BY_NAS = 0,  /*!< IPv4 address allocated by NAS */
    QOSA_PDP_ADDR_ALLOC_BY_DHCP = 1, /*!< IPv4 address allocated by DHCP */
} qosa_pdp_ipv4_addr_alloc_type_e;

/**
 * @enum qosa_pdp_request_type_e
 * @brief PDP request type
 */
typedef enum
{
    QOSA_PDP_REQ_TYPE_NEW_PDP_OR_HANDOVER = 0, /*!< New PDP or handover */
    QOSA_PDP_REQ_TYPE_EMERGENCY_PDP = 1,       /*!< Emergency PDP */
    QOSA_PDP_REQ_TYPE_NEW_PDP = 2,             /*!< New PDP */
    QOSA_PDP_REQ_TYPE_HANDOVER = 3,            /*!< Handover */
    QOSA_PDP_REQ_TYPE_EMERGENCY_HANDOVER = 4,  /*!< Emergency handover */
} qosa_pdp_request_type_e;

/**
 * @enum qosa_pdp_ssc_mode_e
 * @brief PDP session and service continuity mode
 */
typedef enum
{
    QOSA_PDP_SSC_MODE_1 = 0, /*!< SSC mode 1 */
    QOSA_PDP_SSC_MODE_2 = 1, /*!< SSC mode 2 */
    QOSA_PDP_SSC_MODE_3 = 2, /*!< SSC mode 3 */
} qosa_pdp_ssc_mode_e;

/**
 * @enum qosa_pdp_ip_type_e
 * @brief PDP IP type
 */
typedef enum
{
    QOSA_PDP_INVALID = 0, /*!< Invalid IP type */
    QOSA_PDP_IPV4 = 1,    /*!< IPv4 type */
    QOSA_PDP_IPV6 = 2,    /*!< IPv6 type */
    QOSA_PDP_IPV4V6 = 3   /*!< IPv4v6 dual stack type */
} qosa_pdp_ip_type_e;

/**
 * @enum qosa_pdp_type_e
 * @brief PDP type
 */
typedef enum
{
    QOSA_PDP_TYPE_X25 = 0,    /*!< X.25 protocol type */
    QOSA_PDP_TYPE_IP = 1,     /*!< IP protocol type */
    QOSA_PDP_TYPE_IPV6 = 2,   /*!< IPv6 protocol type */
    QOSA_PDP_TYPE_IPV4V6 = 3, /*!< IPv4v6 dual stack protocol type */
    QOSA_PDP_TYPE_PPP = 5,    /*!< PPP protocol type */
#if defined (CONFIG_QOSA_NW_NB_SUPPORT) || defined(CONFIG_QOSA_NW_BJU_SUPPORT)
    QOSA_PDP_TYPE_NON_IP = 6, /*!< NON IP */
#endif                        /*CONFIG_QOSA_NW_NB_SUPPORT*/

} qosa_pdp_type_e;

/**
 * @enum qosa_pdp_auth_type_e
 * @brief PDP authentication parameter type
 */
typedef enum
{
    QOSA_PDP_AUTH_TYPE_NONE = 0, /*!< PDP activation without authentication */
    QOSA_PDP_AUTH_TYPE_PAP = 1,  /*!< PAP authentication */
    QOSA_PDP_AUTH_TYPE_CHAP = 2, /*!< CHAP authentication */
} qosa_pdp_auth_type_e;

/**
 * @enum qosa_pdp_procedures_e
 * @brief PDP operation type
 */
typedef enum
{
    QOSA_PDP_ACT_OPT_DEACT = 0, /*!< Deactivation operation */
    QOSA_PDP_ACT_OPT_ACT = 1,   /*!< Activation operation */
} qosa_pdp_procedures_e;

/*-------------------------------------- basic struct --------------------------------------*/

/**
 * @brief datacall connection domain object
 */
typedef qosa_base_t qosa_datacall_conn_t;

/**
 * @struct qosa_pdp_context_t
 * @brief PDP context parameters
 */
typedef struct
{
    qosa_uint8_t pdp_type;                                 /*!< PDP type, qosa_pdp_type_e */

    qosa_bool_t apn_valid;                                 /*!< Whether APN is configured */
    char        apn[QOSA_APN_MAX_LEN + 1];                 /*!< APN for PDP activation */

    qosa_bool_t    ipv4_ip_valid;                          /*!< Whether IPv4 address is configured */
    qosa_ip_addr_t ipv4_ip;                                /*!< User configured IPv4 address */

    qosa_bool_t    ipv6_ip_valid;                          /*!< Whether IPv6 address is configured */
    qosa_ip_addr_t ipv6_ip;                                /*!< User configured IPv6 address */

    qosa_bool_t               data_comp_valid;             /*!< Whether data compression is configured */
    qosa_pdp_data_comp_type_e data_comp;                   /*!< Data compression type */

    qosa_bool_t               head_comp_valid;             /*!< Whether header compression is configured */
    qosa_pdp_head_comp_type_e head_comp;                   /*!< Header compression type */

    qosa_bool_t                     ipv4_addr_alloc_valid; /*!< Whether IPv4 address allocation is configured */
    qosa_pdp_ipv4_addr_alloc_type_e ipv4_addr_alloc;       /*!< IPv4 address allocation type */

    qosa_bool_t             request_type_valid;            /*!< Whether PDP request type is configured */
    qosa_pdp_request_type_e request_type;                  /*!< PDP request type */

#ifdef CONFIG_QOSA_NW_NR_SUPPORT
    qosa_bool_t         ssc_mode_valid;  /*!< Whether SSC mode is configured */
    qosa_pdp_ssc_mode_e ssc_mode;        /*!< SSC mode */

    qosa_bool_t       s_nssai_valid;     /*!< Whether S-NSSAI is configured */
    qosa_nw_s_nssai_t s_nssai;           /*!< S-NSSAI */

    qosa_bool_t  pref_access_type_valid; /*!< Whether preferred access type is configured */
    qosa_uint8_t pref_access_type;       /*!< preferred access type 0:3GPP 1:Non-3GPP */

    qosa_bool_t  allow_on_req_valid;     /*!< Whether allow on request is configured */
    qosa_uint8_t allow_on_req;           /*!< allow on request 0:not allowed 1:allowed */

#endif                                   // CONFIG_QOSA_NW_NR_SUPPORT

    qosa_bool_t  nslpi_valid;            /*!< Whether NSLPIPresent is configured */
    qosa_uint8_t nslpi;                  /*!<  CmiPsNSLPICfg, NAS signalling priority */

    qosa_bool_t  secpco_valid;           /*!< Whether secPcoPresent is configured */
    qosa_uint8_t secpco;                 /*!<  0 Security protected transmission of PCO is not requested 1:Security protected transmission of PCO is requested*/

} qosa_pdp_context_t;

/**
 * @struct qosa_pdp_auth_context_t
 * @brief PDP authentication parameters
 *
 */
typedef struct
{
    qosa_bool_t          auth_valid;                               /*!< Whether Auth_type is configured */
    qosa_pdp_auth_type_e auth_type;                                /*!< PDP authentication type */
    qosa_bool_t          user_valid;                               /*!< Whether User is configured */
    char                 username[QOSA_PDP_USER_NAME_MAX_LEN + 1]; /*!< PDP username */
    qosa_bool_t          pass_valid;                               /*!< Whether password is configured */
    char                 password[QOSA_PDP_USER_PWD_MAX_LEN + 1];  /*!< PDP password */
} qosa_pdp_auth_context_t;

/**
 * @struct qosa_pdp_qos_profile_t
 * @brief PDP QoS parameters
 *
 */
typedef struct
{
    qosa_uint8_t precedence;  /*!< specifies the precedence class */
    qosa_uint8_t delay;       /*!< specifies the delay class */
    qosa_uint8_t reliability; /*!< specifies the reliability class */
    qosa_uint8_t peak;        /*!< specifies the peak throughput class */
    qosa_uint8_t mean;        /*!< specifies the mean throughput class */
} qosa_pdp_qos_profile_t;

/**
 * @struct qosa_pdp_simcid_info_t
 * @brief PDP SIM card and activation channel related information
 */
typedef struct
{
    qosa_uint8_t simid; /*!< sim id */
    qosa_uint8_t pdpid; /*!< pdp id */
} qosa_pdp_simcid_info_t;

/**
 * @struct qosa_datacall_ip_info_t
 * @brief Activation information and IP address for each SIM PDP
 */
typedef struct
{
    qosa_pdp_simcid_info_t simcid;   /*!< simID and pdpID information */
    qosa_pdp_ip_type_e     ip_type;  /*!< IP type, IPv4 IPv6 IPv4v6 */
    qosa_ip_addr_t         ipv4_ip;  /*!< IPv4 address obtained after activation */
    qosa_ip_addr_t         ipv6_ip;  /*!< IPv6 address obtained after activation */
    qosa_ip_addr_t         pri_dns;  /*!< IPv4 primary DNS address obtained after activation */
    qosa_ip_addr_t         sec_dns;  /*!< IPv4 secondary DNS address obtained after activation */
    qosa_ip_addr_t         pri6_dns; /*!< IPv6 primary DNS address obtained after activation */
    qosa_ip_addr_t         sec6_dns; /*!< IPv6 secondary DNS address obtained after activation */
} qosa_datacall_ip_info_t;

/**
 * @struct qosa_datacall_traffic_statistics_t
 * @brief Network traffic statistics information
 *
 */
typedef struct
{
    qosa_uint64_t total_uplink_bytes;             /*!< Total uplink traffic, in bytes */
    qosa_uint64_t total_downlink_bytes;           /*!< Total downlink traffic, in bytes */

    qosa_uint64_t uplink_bytes[QOSA_PDPID_MAX];   /*!< Uplink traffic per PDP, in bytes */
    qosa_uint64_t downlink_bytes[QOSA_PDPID_MAX]; /*!< Downlink traffic per PDP, in bytes */
} qosa_datacall_traffic_statistics_t;

/**
 * @struct qosa_datacall_pdp_timer_t
 * @brief PDP timer configuration
 *
 */
typedef struct
{
    qosa_uint8_t counts;      /*!< PDP retry count, 0 means no retry, which means only execute PDP active or deactivate operation once */
    qosa_uint8_t timer_value; /*!< Retry time (in seconds) */
} qosa_datacall_pdp_timer_t;

/**
 * @struct qosa_datacall_dns_t
 * @brief DNS address configuration for data connection
 */
typedef struct
{
    qosa_ip_addr_t pri_dns;  /*!< IPv4 primary DNS server address */
    qosa_ip_addr_t sec_dns;  /*!< IPv4 secondary DNS server address */
    qosa_ip_addr_t pri6_dns; /*!< IPv6 primary DNS server address */
    qosa_ip_addr_t sec6_dns; /*!< IPv6 secondary DNS server address */
} qosa_datacall_dns_t;

/**
 * @struct qosa_pdp_dynamic_context_t
 * @brief PDP dynamic context
 */
typedef struct
{
    qosa_uint8_t pdp_type;                                /*!< PDP type, qosa_pdp_type_e */
    qosa_uint8_t pdpid;                                   /*!< pdp id */
    qosa_uint8_t bearer_id;                               /*!< bear id */

    qosa_bool_t apn_valid;                                /*!< Whether APN is configured */
    char        apn[QOSA_APN_MAX_LEN + 1];                /*!< APN for PDP activation */

    qosa_ip_addr_t local_ipaddr[QOSA_PDP_IPADDR_MAX_NUM]; /*!<shows the IP address of the MT>*/
    qosa_ip_addr_t subnet[QOSA_PDP_IPADDR_MAX_NUM];       /*!<shows the subnet mask of the MT>*/

    qosa_ip_addr_t gw_addr;                               /*!<shows the Gateway Address of the MT>*/

    qosa_ip_addr_t dns_prim_addr;                         /*!<shows the IP address of the DNS prim server>*/
    qosa_ip_addr_t dns_sec_addr;                          /*!<shows the IP address of the DNS sec server>*/
    qosa_ip_addr_t dns6_prim_addr;                        /*!<shows the IPv6 address of the DNS prim server>*/
    qosa_ip_addr_t dns6_sec_addr;                         /*!<shows the IPv6 address of the DNS sec server>*/
} qosa_pdp_dynamic_context_t;

/*-------------------------------------- event --------------------------------------*/

/**
 * @struct qosa_datacall_nw_deact_event_t
 * @brief PDP deactivation event reporting parameters (triggered by network side) -- corresponding event: QOSA_EVENT_NW_PDN_DEACT
 *
 */
typedef struct
{
    qosa_uint8_t simid; /*!< SIM card ID */
    qosa_uint8_t pdpid; /*!< PDPID */
} qosa_datacall_nw_deact_event_t;

/**
 * @struct qosa_datacall_act_event_t
 * @brief PDP activation or deactivation event reporting parameters (caused by calling activation or deactivation API interface) -- corresponding event: QOSA_EVENT_NET_PDP_ACT
 *
 */
typedef struct
{
    qosa_uint8_t                simid; /*!< SIM card ID */
    qosa_uint8_t                pdpid; /*!< PDPID */
    qosa_datacall_pdp_act_opt_e opt;   /*!< Activation or deactivation */
} qosa_datacall_act_event_t;
/*-------------------------------------- pa --------------------------------------*/

/*-------------------------------------- cnf --------------------------------------*/

/* All cnf results need to ensure that the first two parameters of the structure are simid, err_code in order, that is OSA_MODEM_COMMON_CNF_HDR, user layer does not need to release parameter content, after executing the callback, it will be directly released internally */

/**
 * @struct qosa_datacall_general_cnf_t
 * @brief General result return for datacall module asynchronous interface, used to return simple success or failure, no additional result return type
 *
 */
typedef qosa_modem_general_cnf_t qosa_datacall_general_cnf_t;

/**
 * @struct qosa_datacall_act_cnf_t
 * @brief PDP activation result status notification
 */
typedef struct
{
    QOSA_MODEM_COMMON_CNF_HDR;
    qosa_uint8_t                pdpid; /*!< PDPID */
    qosa_datacall_pdp_act_opt_e opt;   /*!< Execute activation and deactivation actions */
    qosa_bool_t
        status; /*!< TRUE: execute opt operation successfully, FALSE: execute opt operation failed, there is some duplication with err_code, can consider deleting */
    qosa_datacall_ip_info_t
        ip_info; /*!< IP address information carried after PDP activation, only carried when activation is successful, this field is meaningless when activation is unsuccessful */
} qosa_datacall_act_cnf_t;

/*-------------------------------------- api --------------------------------------*/

/**
 * @brief Activate or deactivate network attachment
 *
 * This function is used to control the data network attachment status of the specified SIM card, you can choose to attach to the network or detach from the network.
 * The operation result will be returned asynchronously through the callback function, the asynchronously returned result structure is `qosa_datacall_general_cnf_t`
 *
 * @param[in] simid
 *          - sim ID
 * @param[in] method
 *          - Attachment operation method
 *              - QOSA_DATACALL_DETACH : Detach from network
 *              - QOSA_DATACALL_ATTACH : Attach to network
 * @param[in] cb
 *          - Callback function pointer, used to receive asynchronous result notification
 * @param[in] ctx
 *          - Callback function context pointer, used to pass user context information in the callback function
 * @return qosa_datacall_errno_e
 */
qosa_datacall_errno_e qosa_datacall_attach(qosa_uint8_t simid, qosa_uint8_t method, datacall_callback_cb_ptr cb, void* ctx);

/**
 * @brief Block waiting for specified SIM card to successfully register with network
 *
 * This interface will check the network attachment status in an intermittent polling manner within the specified timeout period.
 * If successful registration is detected within the timeout period, it will return immediately; if the attachment is still unsuccessful after timeout, it will return failure.
 *
 * @param[in] simid
 *          - sim ID
 * @param[in] timeout
 *          - Timeout time (in seconds)
 * @return qosa_bool_t
 *          - Return OSA_TRUE for successful registration, return OSA_FALSE for timeout
 * @note
 *          - This function is a blocking interface, which will block the current thread during the call.
 *          - Suitable for use in simple scenarios; for non-blocking or asynchronous notification, it is recommended to use event or callback mechanisms.
 */
qosa_bool_t qosa_datacall_wait_attached(qosa_uint8_t simid, qosa_uint32_t timeout);

/**
 * @brief Generate a connection object for a specific functional domain
 *
 * @param[in] simid
 *          - sim id used by the connection object
 * @param[in] pdpid
 *          - pdp id used by the connection object
 * @param[in] conn_type
 *          - Functional domain of datacall connection application
 * @return qosa_datacall_conn_t* Return connection object, if parameter is illegal, will return OSA_DATACALL_CONN_INVALID
 */
qosa_datacall_conn_t qosa_datacall_conn_new(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_datacall_conn_type_e conn_type);

/**
 * @brief Open specific datacall connection object (synchronous)
 *
 * @param[in] conn
 *          - datacall connection object
 * @param[in] max_wait_time
 *          - Maximum timeout time, in seconds; 0 means using default timeout time
 * @return qosa_datacall_errno_e
 */
qosa_datacall_errno_e qosa_datacall_start(qosa_datacall_conn_t conn, qosa_uint32_t max_wait_time);

/**
 * @brief Open the specified datacall connection object (asynchronous interface)
 *
 * This function is used to start a data connection object. After successful call, the actual result will be returned asynchronously through the callback function,
 * the callback parameter is `qosa_datacall_act_cnf_t`, containing detailed result information of data connection activation.
 *
 * @param[in] conn
 *          - datacall connection object
 * @param[in] max_wait_time
 *          - Maximum timeout time, in seconds; 0 means using default timeout time
 * @param[in] cb
 *          - Callback function pointer, used to receive asynchronous result notification
 * @param[in] ctx
 *          - Callback function context pointer, used to pass user context information in the callback function
 * @return qosa_datacall_errno_e
 */
qosa_datacall_errno_e qosa_datacall_start_async(qosa_datacall_conn_t conn, qosa_uint32_t max_wait_time, datacall_callback_cb_ptr cb, void* ctx);

/**
 * @brief Close specific datacall connection object (synchronous interface)
 *
 * @param[in] conn
 *          - datacall connection object
 * @param[in] max_wait_time
 *          - Maximum timeout time, in seconds; 0 means using default timeout time
 * @return qosa_datacall_errno_e
 */
qosa_datacall_errno_e qosa_datacall_stop(qosa_datacall_conn_t conn, qosa_uint32_t max_wait_time);

/**
 * @brief Close specific datacall connection object (asynchronous interface)
 *
 * @param[in] conn
 *          - datacall connection object
 * @param[in] max_wait_time
 *          - Maximum timeout time, in seconds; 0 means using default timeout time
 * @param[in] cb
 *          - Callback function pointer, used to receive asynchronous result notification
 * @param[in] ctx
 *          - Callback function context pointer, used to pass user context information in the callback function
 * @return qosa_datacall_errno_e
 */
qosa_datacall_errno_e qosa_datacall_stop_async(qosa_datacall_conn_t conn, qosa_uint32_t max_wait_time, datacall_callback_cb_ptr cb, void* ctx);

/**
 * @brief Get the connection status of the specified PDP
 *
 * @param[in] simid
 *          - sim id
 * @param[in] pdpid
 *          - pdp id
 * @return qosa_bool_t
 *          - Connection status, 0: inactive status 1: active status
 */
#define qosa_datacall_get_pdp_status(simid, pdpid) qosa_datacall_get_status(qosa_datacall_conn_new(simid, pdpid, QOSA_DATACALL_CONN_UNDEFINED))

/**
 * @brief Get the connection status of a specific datacall connection object, before using a specific PDP connection on the application side, you can check this status, if it is already in connection status, it means the connection is being used
 *
 * @param[in] conn
 *          - datacall connection object
 * @return qosa_bool_t
 *          - Connection status, 0: inactive status 1: active status
 */
qosa_bool_t qosa_datacall_get_status(qosa_datacall_conn_t conn);

/**
 * @brief Get the IP address information of the specified PDP
 *
 * @param[in] simid
 *          - sim id
 * @param[in] pdpid
 *          - pdp id
 * @param[out] info
 *          - PDP ip information
 * @return qosa_bool_t
 */
#define qosa_datacall_get_pdp_ip_info(simid, pdpid, info) qosa_datacall_get_ip_info(qosa_datacall_conn_new(simid, pdpid, QOSA_DATACALL_CONN_UNDEFINED), info)

/**
 * @brief Get detailed information of a specific datacall connection object, including IP address and other information of the connection object
 *
 * @param[in] conn
 *          - datacall connection object
 * @param[out] info
 *          - Connection object information
 * @return qosa_datacall_errno_e
 */
qosa_datacall_errno_e qosa_datacall_get_ip_info(qosa_datacall_conn_t conn, qosa_datacall_ip_info_t* info);

/**
 * @brief Get the DNS server IP address of the specified PDP
 *
 * @param[in] simid
 *          - SIMID to operate and get
 *
 * @param[in] pdpid
 *          - PDPID to operate and get
 *
 * @param[out] dns
 *          - Obtained DNS address
 *
 * @return qosa_datacall_errno_e
 *       - Return OSA_DATACALL_OK for success
 *       - Return others for failure
 */
qosa_datacall_errno_e qosa_datacall_get_dns_addr(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_datacall_dns_t* dns);

/**
 * @brief Get the DNS server IP address of the specified PDP
 *
 * @param[in] simid
 *          - SIMID to operate and get
 *
 * @param[in] pdpid
 *          - PDPID to operate and get
 *
 * @param[in] dns
 *          - DNS address to be modified, including pri_dns, sec_dns, pri6_dns, sec6_dns; where pri_dns, sec_dns represent IPv4 DNS addresses,
 *            if you want to set, you need to set its ip_vsn to QOSA_PDP_IPV4; pri6_dns, sec6_dns represent IPv6 DNS addresses, if you want to set, you need to set its ip_vsn to QOSA_PDP_IPV6
 *
 * @return qosa_datacall_errno_e
 *       - Return OSA_DATACALL_OK for success
 *       - Return others for failure
 */
qosa_datacall_errno_e qosa_datacall_set_dns_addr(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_datacall_dns_t* dns);

/**
 * @brief Query whether the specified PDP is defined
 *
 * This interface is used to check whether a PDP is in a defined state.
 * Defined state means that the user side has configured the profile corresponding to the PDP.
 *
 * @param[in] simid
 *          - sim id
 * @param[in] pdpid
 *          - pdp id
 * @return qosa_bool_t
 *          - QOSA_TRUE  : PDP is defined
 *          - QOSA_FALSE : PDP is not defined
 */
qosa_bool_t qosa_datacall_is_defined(qosa_uint8_t simid, qosa_uint8_t pdpid);

/**
 * @brief Set PDP context for a specific PDP
 *
 * @param[in] simid
 *          - sim ID
 * @param[in] pdpid
 *          - pdp ID
 * @param[in] pdp_ctx
 *          - pdp context
 * @return qosa_datacall_errno_e
 *          - QOSA_DATACALL_OK : Success
 *          - Other error codes   : Failure, please refer to qosa_datacall_errno_e for details
 */
qosa_datacall_errno_e qosa_datacall_set_pdp_context(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_pdp_context_t* pdp_ctx);

/**
 * @brief Delete PDP context for a specific PDP
 *
 * @param[in] simid
 *          - sim ID
 * @param[in] pdpid
 *          - pdp ID
 * @return qosa_datacall_errno_e
 *          - QOSA_DATACALL_OK : Success
 *          - Other error codes   : Failure, please refer to qosa_datacall_errno_e for details
 */
qosa_datacall_errno_e qosa_datacall_delete_pdp_context(qosa_uint8_t simid, qosa_uint8_t pdpid);

/**
 * @brief Get PDP context for a specific PDP
 *
 * @param[in] simid
 *          - sim ID
 * @param[in] pdpid
 *          - pdp ID
 * @param[out] pdp_ctx
 *          - pdp context
 * @return qosa_datacall_errno_e
 *          - QOSA_DATACALL_OK : Success
 *          - Other error codes   : Failure, please refer to qosa_datacall_errno_e for details
 */
qosa_datacall_errno_e qosa_datacall_get_pdp_context(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_pdp_context_t* pdp_ctx);

/**
 * @brief Set PDP auth context for a specific PDP
 *
 * @param[in] simid
 *          - sim ID
 * @param[in] pdpid
 *          - pdp ID
 * @param[in] auth_ctx
 *          - pdp auth context
 * @return qosa_datacall_errno_e
 *          - QOSA_DATACALL_OK : Success
 *          - Other error codes   : Failure, please refer to qosa_datacall_errno_e for details
 */
qosa_datacall_errno_e qosa_datacall_set_pdp_auth(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_pdp_auth_context_t* auth_ctx);

/**
 * @brief Get PDP auth context for a specific PDP
 *
 * @param[in] simid
 *          - sim ID
 * @param[in] pdpid
 *          - pdp ID
 * @param[in] auth_ctx
 *          - pdp auth context
 * @return qosa_datacall_errno_e
 *          - QOSA_DATACALL_OK : Success
 *          - Other error codes   : Failure, please refer to qosa_datacall_errno_e for details
 */
qosa_datacall_errno_e qosa_datacall_get_pdp_auth(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_pdp_auth_context_t* auth_ctx);

/**
 * @brief Define QoS profile for a specific PDP
 *
 * @param[in] simid
 *          - sim ID
 * @param[in] pdpid
 *          - pdp ID
 * @param[in] qos_profile
 *          - qos profile
 * @return qosa_datacall_errno_e
 *          - QOSA_DATACALL_OK : Success
 *          - Other error codes   : Failure, please refer to qosa_datacall_errno_e for details
 */
qosa_datacall_errno_e qosa_datacall_set_pdp_qos(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_pdp_qos_profile_t* qos_profile);

/**
 * @brief Get QoS profile for a specific PDP
 *
 * @param[in] simid
 *          - sim ID
 * @param[in] pdpid
 *          - pdp ID
 * @param[in] qos_profile
 *          - qos profile
 * @return qosa_datacall_errno_e
 *          - QOSA_DATACALL_OK : Success
 *          - Other error codes   : Failure, please refer to qosa_datacall_errno_e for details
 */
qosa_datacall_errno_e qosa_datacall_get_pdp_qos(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_pdp_qos_profile_t* qos_profile);

/**
 * @brief Get uplink and downlink traffic statistics information
 *
 * @param[in] simid
 *          - sim ID
 * @param[out] traffic_statistics
 *          - Obtained traffic statistics information
 * @return qosa_datacall_errno_e
 *          - QOSA_DATACALL_OK : Success
 *          - Other error codes   : Failure, please refer to qosa_datacall_errno_e for details
 */
qosa_datacall_errno_e qosa_datacall_get_traffic_statistics(qosa_uint8_t simid, qosa_datacall_traffic_statistics_t* traffic_statistics);

/**
 * @brief Clear uplink and downlink traffic statistics information
 *
 * @param[in] simid
 *          - sim ID
 * @return qosa_datacall_errno_e
 *          - QOSA_DATACALL_OK : Success
 *          - Other error codes   : Failure, please refer to qosa_datacall_errno_e for details
 */
qosa_datacall_errno_e qosa_datacall_clear_traffic_statistics(qosa_uint8_t simid);

/**
 * @brief Save uplink and downlink traffic information to NV
 *
 * @param[in] simid
 *          - sim ID
 * @return qosa_datacall_errno_e
 *          - QOSA_DATACALL_OK : Success
 *          - Other error codes   : Failure, please refer to qosa_datacall_errno_e for details
 */
qosa_datacall_errno_e qosa_datacall_save_traffic_statistics(qosa_uint8_t simid);

/**
 * @brief Configure PDP activation/deactivation timer configuration
 *
 * @param[in] simid
 *          - sim ID
 * @param[in] rat
 *          - RAT to which the timer to be configured belongs, refer to qosa_nw_rat_e in osa_network.h
 * @param[in] procedure
 *          - Type of timer to be configured, parameter reference qosa_datacall_pdp_act_opt_e, timer for activation or deactivation operation
 * @param[in] pdp_timer
 *          - Timer configuration
 * @return qosa_datacall_errno_e
 *          - QOSA_DATACALL_OK : Success
 *          - Other error codes   : Failure, please refer to qosa_datacall_errno_e for details
 */
qosa_datacall_errno_e qosa_datacall_set_pdp_timer(qosa_uint8_t simid, qosa_uint8_t rat, qosa_uint8_t procedure, qosa_datacall_pdp_timer_t* pdp_timer);

/**
 * @brief Get PDP activation/deactivation timer configuration information
 *
 * @param[in] simid
 *          - sim ID
 * @param[in] rat
 *          - RAT to which the timer to be obtained belongs, refer to qosa_nw_rat_e in osa_network.h
 * @param[in] procedure
 *          - Type of timer to be obtained, parameter reference qosa_datacall_pdp_act_opt_e, timer for activation or deactivation operation
 * @param[in] pdp_timer
 *          - Timer configuration
 * @return qosa_datacall_errno_e
 *          - QOSA_DATACALL_OK : Success
 *          - Other error codes   : Failure, please refer to qosa_datacall_errno_e for details
 */
qosa_datacall_errno_e qosa_datacall_get_pdp_timer(qosa_uint8_t simid, qosa_uint8_t rat, qosa_uint8_t procedure, qosa_datacall_pdp_timer_t* pdp_timer);

/**
 * @brief Get dynamic pdp ctx
 *
 * @param[in] simid
 *          - sim ID
 * @param[in] pdpid
 *          - pdp ID
 * @param[out] pdp_dynamic_ctx
 *          - PDP dynamic context
 * @return int
 */
qosa_datacall_errno_e qosa_datacall_get_dynamic_pdp_ctx(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_pdp_dynamic_context_t* pdp_dynamic_ctx);

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
 * @return qosa_datacall_errno_e
 *       - Returns QOSA_DATACALL_OK on success
 *       - Returns other values on failure
 */
qosa_datacall_errno_e qosa_datacall_set_pdpreact_mode(qosa_uint8_t simid, qosa_bool_t enable);

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
 * @return qosa_datacall_errno_e
 *       - Returns QOSA_DATACALL_OK on success
 *       - Returns other values on failure
 */
qosa_datacall_errno_e qosa_datacall_get_pdpreact_mode(qosa_uint8_t simid, qosa_bool_t *enable);
#endif//!(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif /* __QOSA_DATACALL_H__ */
