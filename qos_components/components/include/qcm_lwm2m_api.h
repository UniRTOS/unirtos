/*****************************************************************/ /**
* @file qcm_lwm2m_api.h
* @brief
* @author larson.li@quectel.com
* @date 2024-01-24
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2024-01-24 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#ifndef __QCM_LWM2M_API_H__
#define __QCM_LWM2M_API_H__

#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_datacall.h"
#include "qosa_queue_list.h"
#ifdef CONFIG_QCM_LWM2M_DTLS_FUNC
#include "qcm_vtls_cfg.h"
#endif
/** Default PDP context used by LWM2M */
#define QCM_LWM2M_PDP_CID_DEFAULT                QOSA_PDP_CID_MIN

/** Default object count for LWM2M, Object IDs: Refer to qcm_lwm2m_support_object_id_e count */
#define QCM_LWM2M_DEFAULT_OBJ_CNT                14

/** Maximum number of objects a user can add in a single LWM2M connection */
#define QCM_LWM2M_OBJ_CNT_MAX                    15
/** Maximum number of objects supported by LWM2M client */
#define QCM_LWM2M_OBJ_TOTAL_MAX                  (QCM_LWM2M_DEFAULT_OBJ_CNT + QCM_LWM2M_OBJ_CNT_MAX)

/** Maximum number of instances that can be added for a single object dev_port */
#define QCM_LWM2M_INSTANCE_CNT_MAX               4
/** Maximum number of resources that can be added for an instance */
#define QCM_LWM2M_RESOURCES_CNT_MAX              14

/** Communication binding mode with server for LWM2M */
/** U corresponds to UDP only communication */
#define QCM_LWM2M_BINDING_MODE_U                 "U"
/** UQ corresponds to UDP queue communication method */
#define QCM_LWM2M_BINDING_MODE_UQ                "UQ"
/** UQS corresponds to UDP queue and SMS communication method */
#define QCM_LWM2M_BINDING_MODE_UQS               "UQS"
/** Maximum character length for LWM2M communication binding mode */
#define QCM_LWM2M_BINDING_MODE_MAX_LEN           4

/** Maximum input length for LWM2M PSK identity information */
#define QCM_LWM2M_PSK_IDENTITY_LEN_MAX           128
/** Maximum input length for PSK KEY */
#define QCM_LWM2M_PSK_KEY_LEN_MAX                256

/** Maximum length for customer configuration version number */
#define QCM_LWM2M_CUSTOMER_VERSION_NAME_LEN      64

/** Length for customer configured custom ENDPOINT name */
#define QCM_LWM2M_ENDPOINT_NAME_LEN_MAX          128

/** General limit length for customer custom configured module device information */
#define QCM_LWM2M_CUSTOMER_DEVICE_INFO_LEN       32

/** Customer custom configured manufacturer information */
#define QCM_LWM2M_CUSTOMER_MANUFACTURER_NAME_LEN 32

/** Customer custom configured module device serial number */
#define QCM_LWM2M_CUSTOMER_MODE_NUMBER_NAME_LEN  32

/** Length for customer configured custom firmware PACKAGE_URI */
#define QCM_LWM2M_FIRMWARE_URL_LEN_MAX           128

/** Maximum length for identifier storing different client types internally in LWM2M */
#define QCM_LWM2M_CONFIG_TAG_MAX_LEN             6

/** Maximum parameter length for LWM2M object16 hostdevice */
#define QCM_LWM2M_HOST_DEVICE_PARAM_LEN          32

/**
 * @enum qcm_lwm2m_event_e
 * @brief LWM2M event type reporting types
 */
typedef enum
{
    QCM_LWM2M_EVENT_REG_MSG = 0,                    /*!< LWM2M internal registration status event notification */
    QCM_LWM2M_EVENT_DEREG_MSG = 1,                  /*!< LWM2M deregistration result return */
    QCM_LWM2M_EVENT_STATE_CHG_MSG = 2,              /*!< LWM2M internal state change notification */
    QCM_LWM2M_EVENT_FW_DLOAD_MSG = 3,               /*!< Firmware 5/0/1 firmware upgrade download URL event notification */
    QCM_LWM2M_EVENT_FW_UPDATE_MSG = 4,              /*!< Firmware 5/0/2 firmware update status notification */
    QCM_LWM2M_EVENT_REBOOT_MSG = 5,                 /*!< Reboot restart event notification */
    QCM_LWM2M_EVENT_UPDATE_MSG = 6,                 /*!< Update lifetime/bindmode/object/... status update notification */
    QCM_LWM2M_EVENT_FW_DATA_MSG = 7,                /*!< Firmware 5/0/0 firmware upgrade data packet arrival notification, carrying data content */
    QCM_LWM2M_EVENT_AUTO_REG_MSG = 8,               /*!< LWM2M automatic registration notification */
    QCM_LWM2M_EVENT_CUST_OBJECT_MOD_MSG = 9,        /*!< Report add/delete custom object execution result */
    QCM_LWM2M_EVENT_CUST_OBJECT_RESP_SEND_MSG = 10, /*!< Server performs command operations on custom objects */
    QCM_LWM2M_EVENT_FW_EXIT_MSG,                    /*!< Object firmware executes UPDATE, sends message to exit registration */
    QCM_LWM2M_EVENT_FW_RESULT_MSG,                  /*!< Register after upgrade, report firmware result 5/0/5 status notification */
    QCM_LWM2M_EVENT_FW_STATE_OBSERVE_MSG,           /*!< Firmware 5/0/3 subscription notification */
    QCM_LWM2M_EVENT_UPDATE_RESP_ACK_MSG,            /*!< User executes update API action, server response status */
    QCM_LWM2M_EVENT_UPDATE_EXE_RESULT_MSG,          /*!< Update command execution result return */
    QCM_LWM2M_EVENT_ADD_OBJECT_MSG,                 /*!< User custom add object execution status notification */
    QCM_LWM2M_EVENT_DEL_OBJECT_MSG,                 /*!< User delete custom object execution status notification */
    QCM_LWM2M_EVENT_NOTIFY_RESP_ACK_MSG,            /*!< After API executes notify API notification, server response return */
    QCM_LWM2M_EVENT_RESP_WRITE_REQ_RETURN,          /*!< Response to server write request API execution result return */
    QCM_LWM2M_EVENT_RESP_EXEC_REQ_RETURN,           /*!< Response to server execute request API execution result return */
    QCM_LWM2M_EVENT_RESP_READ_REQ_RETURN,           /*!< Response to server read request API execution result return */
    QCM_LWM2M_EVENT_RESP_OBSERVE_REQ_RETURN,        /*!< Response to server observe request API execution result return */
    QCM_LWM2M_EVENT_RESP_DISCOVER_REQ_RETURN,       /*!< Response to server discover request API execution result return */
    QCM_LWM2M_EVENT_ACTIVE_NOTIFY_RESP,             /*!< Active execute notify data notification API execution result return */
    QCM_LWM2M_EVENT_UPDATE_PING_EVENT,              /*!< Lifetime periodic ping status return */
    QCM_LWM2M_EVENT_LIFETIME_CHG_MSG,               /*!< Lifetime update active notification */
    QCM_LWM2M_EVENT_MIN_PERIOD_CHG_MSG,             /*!< Minimum period time change */
    QCM_LWM2M_EVENT_MAX_PERIOD_CHG_MSG,             /*!< Maximum period time change */
    QCM_LWM2M_EVENT_BINDING_CHG_MSG,                /*!< Binding mode change */
    QCM_LWM2M_EVENT_DISABLE_CHG_MSG,                /*!< Disable server connection event periodic notification */
    QCM_LWM2M_EVENT_BOOTSTARP_REQ_TRIGGER,          /*!< Service executes 1/0/9 resource Bootstrap-Request Trigger */
    QCM_LWM2M_EVENT_REPORT_WRITE_ATTR,              /*!< Notification report DM server downstream write attributes (Write-Attributes) information*/
    QCM_LWM2M_EVENT_NEED_UPDATE,                    /*!< Notification to send update */
    QCM_LWM2M_EVENT_CELL_LOCK_REQ,                  /*!< Notification 13/0/5 cell lock AT+QNWLOCK="lte" */
    QCM_LWM2M_EVENT_MAX_EVENT_MSG = 255,
} qcm_lwm2m_event_e;

/**
 * @enum qcm_lwm2m_server_cfg_opt
 * @brief LWM2M server configuration options
 */
typedef enum
{
    QCM_LWM2M_DM_SERVER_CFG_OPT = 0,      /*!< Corresponding configuration for LWM2M server related parameter configuration options */
    QCM_LWM2M_BOOTSTRAP_SERVER_CFG_OPT,   /*!< Corresponding configuration for LWM2M bootstrap server related parameter configuration options */
    QCM_LWM2M_DIAGNOSTICS_SERVER_CFG_OPT, /*!< Corresponding configuration for LWM2M diagnostics server related parameter configuration options */
    QCM_LWM2M_REPOSITORY_SERVER_CFG_OPT,  /*!< Corresponding configuration for LWM2M repository server related parameter options */
} qcm_lwm2m_server_cfg_opt;

/*
 * @enum qcm_lwm2m_support_object_id_e
 * @brief Corresponding to standard LWM2M defined object device IDs supported internally by unirOS
 * @note Update QCM_LWM2M_DEFAULT_OBJ_CNT when adding new objects
 *
 * For specific Object information, please refer to the OMA official website for object definitions
 */
typedef enum
{
    QCM_LWM2M_SECURITY_OBJECT_ID = 0,
    QCM_LWM2M_SERVER_OBJECT_ID,
    QCM_LWM2M_ACL_OBJECT_ID,
    QCM_LWM2M_DEVICE_OBJECT_ID,
    QCM_LWM2M_CONN_MONITOR_OBJECT_ID,
    QCM_LWM2M_FIRMWARE_UPDATE_OBJECT_ID,
    QCM_LWM2M_LOCATION_OBJECT_ID,
    QCM_LWM2M_CONN_STATS_OBJECT_ID,
    QCM_LWM2M_SOFTWARE_MGNT_OBJECT_ID = 9,
    QCM_LWM2M_APN_CONN_PROFILE_OBJECT_ID = 11,
    QCM_LWM2M_BEARER_SELECTION_OBJECT_ID = 13,
    QCM_LWM2M_HOST_DEVICE_OBJECT_ID = 16,
    QCM_LWM2M_OSCORE_OBJECT_ID = 21,
    QCM_LWM2M_POWERUP_LOG_OBJECT_ID = 3351,
    QCM_LWM2M_SERVINGCELL_MEASUREMENT_OBJECT_ID = 3370,
} qcm_lwm2m_support_object_id_e;

/**
 * @enum qcm_lwm2m_status_e
 * @brief LWM2M client internal running status, all status definitions are here
 */
typedef enum
{
    QCM_LWM2M_STATUS_NONE = 0,
    QCM_LWM2M_STATUS_INIT = 1,              /*!< LWM2M internal initialization status */
    QCM_LWM2M_STATUS_PDP_ACTIVING,          /*!< LWM2M PDP activation in progress */
    QCM_LWM2M_STATUS_PDP_ACTIVE,            /*!< LWM2M PDP activation successful */
    QCM_LWM2M_STATUS_CONFIG,                /*!< LWM2M config configuration initializing */
    QCM_LWM2M_STATUS_DNS_QUERY,             /*!< LWM2M DNS URL resolving in progress */
    QCM_LWM2M_STATUS_SOCKET_CREATE,         /*!< Socket initial */
    QCM_LWM2M_STATUS_SSL_HANDSHARK,         /*!< SSL handshaking DTLS/TLS */
    QCM_LWM2M_STATUS_BOOTSTRAPING,          /*!< Starting Bootstrap */
    QCM_LWM2M_STATUS_BOOTSTRAP,             /*!< Bootstrap completed */
    QCM_LWM2M_STATUS_BOOTSTRAP_FAILED = 10, /*!< Bootstrap registration failed */
    QCM_LWM2M_STATUS_REGISTERING,           /*!< Starting LWM2M server registration */
    QCM_LWM2M_STATUS_READY,                 /*!< LWM2M server registration successful */
    QCM_LWM2M_STATUS_REGIST_FAILED,         /*!< LWM2M registration failed */
    QCM_LWM2M_STATUS_PDP_DEACTIVE,          /*!< PDP deactivation occurred */
    QCM_LWM2M_STATUS_DEREGING = 15,         /*!< LWM2M app starting to prepare for deregistration */
    QCM_LWM2M_STATUS_DEREG,                 /*!< LWM2M app deregistration completed */
    QCM_LWM2M_STATUS_REGIST_REJECT,         /*!< LWM2M registration rejected */

} qcm_lwm2m_status_e;

/**
 * @enum qcm_lwm2m_object_operation_type_e
 * @brief Server request object resource request operation types
 */
typedef enum
{
    QCM_LWM2M_OBJECT_READ = 1,       /*!< DM server executes read action */
    QCM_LWM2M_OBJECT_WRITE,          /*!< DM server executes write action */
    QCM_LWM2M_OBJECT_WRITE_ATTR,     /*!< DM server executes write attributes request */
    QCM_LWM2M_OBJECT_EXECUTE,        /*!< DM server executes execute action */
    QCM_LWM2M_OBJECT_OBSERVE,        /*!< DM server executes observe action */
    QCM_LWM2M_OBJECT_OBSERVE_CANCEL, /*!< DM server executes observe cancel action */
    QCM_LWM2M_OBJECT_DISCOVER,       /*!< DM server executes resource discover request*/
} qcm_lwm2m_object_operation_type_e;

/**
 * @enum qcm_lwm2m_security_mode_e
 * @brief Communication security methods
 */
typedef enum
{
    QCM_LWM2M_SECURITY_MODE_PSK = 0,         /*!< LWM2M connection uses DTLS PSK encryption method (note: some platforms do not support)*/
    QCM_LWM2M_SECURITY_MODE_CERTIFICATE = 2, /*!< LWM2M connection uses DTLS encrypted communication, certificate verification method*/
    QCM_LWM2M_SECURITY_MODE_NULL = 3,        /*!< LWM2M connection does not use encryption method */
} qcm_lwm2m_security_mode_e;

/**
 * @enum qcm_lwm2m_client_startup_e
 * @brief Startup methods
 */
typedef enum
{
    QCM_LWM2M_MANUAL_STARTUP = 0,  /*!< No auto-registration on power-on */
    QCM_LWM2M_AUTO_STARTUP = 1,    /*!< Auto-registration on power-on */
    QCM_LWM2M_DISABLE_STARTUP = 2, /*!< Disable registration */
} qcm_lwm2m_startup_e;

/**
 * @enum qcm_lwm2m_client_recover_e
 * @brief Recovery operation methods
 */
typedef enum
{
    QCM_LWM2M_RECOVER_OFF = 0, /*!< Off by default */
    QCM_LWM2M_RECOVER_MANUAL,  /*!< Manually recovers */
    QCM_LWM2M_RECOVER_AUTO,    /*!< Automatically recovers */
} qcm_lwm2m_recover_e;

/**
 * @enum qcm_lwm2m_result_code_e
 * @brief Communication execution status codes
 */
typedef enum
{
    QCM_LWM2M_MIN = 0,
    QCM_LWM2M_205_CONTENT = 1,           /*!< Content, indicates the correct result. */
    QCM_LWM2M_204_CHANGED = 2,
    QCM_LWM2M_400_BAD_REQUEST = 11,      /*!< Bad Request  */
    QCM_LWM2M_401_UNAUTH = 12,           /*!< Unauthorized */
    QCM_LWM2M_404_NOT_FOUND = 13,        /*!< Not Found */
    QCM_LWM2M_405_METHOD_NOT_ALLOW = 14, /*!< Method Not Allowed */
    QCM_LWM2M_406_NOT_ACCEPT = 15,       /*!< Not Acceptable */
} qcm_lwm2m_result_code_e;

/**
* @enum qcm_lwm2m_data_format_type_e
 * @brief Communication carrier formats
*
 * @note: default: QCM_LWM2M_CONTENT_TLV, ct=11542
*/
typedef enum
{
    QCM_LWM2M_CONTENT_TLV = 4, /*!< LWM2M carries TLV data format content when connecting to DM platform */
    QCM_LWM2M_CONTENT_JSON = 6 /*!< LWM2M carries JSON data format content when connecting to DM platform */
} qcm_lwm2m_data_format_type_e;

/**
 * @enum qcm_lwm2m_bind_mode_e
 * @brief Data communication mode used internally by LWM2M app, default is UQ mode
 */
typedef enum
{
    QCM_LWM2M_BINDMODE_UDP = 0,           /*!< Binding mode UDP only, no specific data communication type specified */
    QCM_LWM2M_BINDMODE_UDP_QUEUE = 1,     /*!< Binding mode is UDP Queue mode, UDP as transport type, Queue as message communication mode */
    QCM_LWM2M_BINDMODE_UDP_QUEUE_SMS = 2, /*!< Binding mode is UDP SMS mode, UDP as transport type, SMS as message communication mode */
} qcm_lwm2m_bind_mode_e;

/**
 * @enum qcm_lwm2m_error_code_e
 * @brief Function execution return error codes
 */
typedef enum
{
    //lwm2m error code
    QCM_LWM2M_ERRID_SUCCESS = 0,
    QCM_LWM2M_ERRID_TIMEOUT = 1 | (QOSA_COMPONENT_LWM2M << 16),               /*!< generic error,this error is only returned when aspecial exception occurs */
    QCM_LWM2M_ERRID_INVALID_PARAM,                                            /*!< Using invalid parameters */
    QCM_LWM2M_ERRID_RECOVERED_FAILED,                                         /*!< Recovered failed */
    QCM_LWM2M_ERRID_UPDATE_FAIED,                                             /*!< update failed */
    QCM_LWM2M_ERRID_SERVER_REP_RST,                                           /*!< Server replied RST, reset */
    QCM_LWM2M_ERRID_NOT_REGISTER_ERROR,                                       /*!< LWM2M not registered successfully, corresponding operation failed */
    QCM_LWM2M_ERRID_NO_MEMORY,                                                /*!< Memory exhausted */
    QCM_LWM2M_ERRID_SOCK_BIND_FAIL,                                           /*!< Local IP address binding error */
    QCM_LWM2M_ERRID_SOCK_CONN_FAIL,                                           /*!< Socket connection error */
    QCM_LWM2M_ERRID_DNS_FAIL,                                                 /*!< DNS resolution error */
    QCM_LWM2M_ERRID_REGISTER_FAIL,                                            /*!< LWM2M registration failed */
    QCM_LWM2M_ERRID_INIT_ALREADY,                                             /*!< LWM2M corresponding client id has already been initialized */
    QCM_LWM2M_ERRID_DATA_LENGTH,                                              /*!< Data length error */

    QCM_LWM2M_ERRID_LOAD_CONFIG_FAIL,                                         /*!< LWM2M internal configuration file loading error */
    QCM_LWM2M_ERRID_OPERATE_FILE_FAIL,                                        /*!< LWM2M internal configuration file operation error */
    QCM_LWM2M_ERRID_URL_FAIL,                                                 /*!< URL processing error */
    QCM_LWM2M_ERRID_WODBLOCK,                                                 /*!< No current task in progress, need to execute again */
    QCM_LWM2M_ERRID_SOCKET_CREATE_FAIL,                                       /*!< Socket creation failed */
    QCM_LWM2M_ERRID_SSL_HS_FAIL,                                              /*!< SSL handshake failed */
    QCM_LWM2M_ERRID_NETWORK_FAIL,                                             /*!< Network related error */
    QCM_LWM2M_ERRID_SUCCESS_REGISTERED = 33 | (QOSA_COMPONENT_LWM2M << 16),   /*!< Successfully registered and logged in */
    QCM_LWM2M_ERRID_FAILED_CREATE_CLIENT = 34 | (QOSA_COMPONENT_LWM2M << 16), /*!< Failed to create client */
    QCM_LWM2M_ERRID_BAD_REQUEST = 400 | (QOSA_COMPONENT_LWM2M << 16),         /*!< Bad request */
    QCM_LWM2M_ERRID_FORBIDDEN = 403 | (QOSA_COMPONENT_LWM2M << 16),           /*!< Forbidden */
    QCM_LWM2M_ERRID_NOT_FOUND = 404 | (QOSA_COMPONENT_LWM2M << 16),           /*!< Not found */
    QCM_LWM2M_ERRID_PRECONDITION_FAILED = 412 | (QOSA_COMPONENT_LWM2M << 16), /*!< Precondition failed */
    QCM_LWM2M_ERRID_GENERAL,                                                  /*!< generic error,this error is only returned when aspecial exception occurs */

    //lwm2m statuc code
    QCM_LWM2M_ERRID_DATA_SEND_FAIL = 2,              /*!< Socket data sending failed */
    QCM_LWM2M_ERRID_RST = 9,                         /*!< Reset */
    QCM_LWM2M_ERRID_STATUS_BAD_REQUEST,              /*!< bad request */
    QCM_LWM2M_ERRID_NOT_ALLOW = 13,                  /*!< Operation not allowed */
    QCM_LWM2M_ERRID_STATUS_NOT_FOUND,                /*!< Not found */
    QCM_LWM2M_ERRID_STATUS_PRECONDITION_FAILED = 22, /*!< Precondition failed */

} qcm_lwm2m_error_code_e;

/**
 * @brief Socket connection types
 */
typedef enum
{
    QCM_LWM2M_SOCK_TYPE_UDP = 0,  /*!< (Default) Use UDP to connect to LWM2M server */
    QCM_LWM2M_SOCK_TYPE_TCP = 1,  /*!< Use TCP to connect to LWM2M server */
    QCM_LWM2M_SOCK_TYPE_DTLS = 2, /*!< Use UDP DTLS non-PSK to connect to LWM2M server */
    QCM_LWM2M_SOCK_TYPE_TLS = 3,  /*!< Use TCP TLS to connect to LWM2M server*/

    QCM_LWM2M_SOCK_TYPE_MAX
} qcm_lwm2m_sock_type_e;

/**
 * @brief Custom configuration information types
 */
typedef enum
{
    QCM_LWM2M_CUSTOM_TYPE_DEVICE = 0,      /*!< (Default) Configure device information, qcm_lwm2m_device_info_t */
    QCM_LWM2M_CUSTOM_TYPE_HOST_DEVICE = 1, /*!< (Default) Configure Host device information, qcm_lwm2m_host_device_t */
    QCM_LWM2M_CUSTOM_TYPE_MAX
} qcm_lwm2m_custom_type_e;

/**
 * @struct qcm_lwm2m_dev_info_t
 * @brief LWM2M custom device information
 */
typedef struct
{
    char manufacturer[QCM_LWM2M_CUSTOMER_MANUFACTURER_NAME_LEN]; /*!< Device manufacturer */
    char model_number[QCM_LWM2M_CUSTOMER_MODE_NUMBER_NAME_LEN];  /*!< Device model */
    char hw_version[QCM_LWM2M_CUSTOMER_VERSION_NAME_LEN];        /*!< Device hardware version */
    char sw_version[QCM_LWM2M_CUSTOMER_VERSION_NAME_LEN];        /*!< Device software version */
    char fw_version[QCM_LWM2M_CUSTOMER_VERSION_NAME_LEN];        /*!< Device firmware version */
    char device_type[QCM_LWM2M_CUSTOMER_DEVICE_INFO_LEN];        /*!< Device type name */
} qcm_lwm2m_device_info_t;

/**
 * @struct qcm_lwm2m_host_device_t
 * @brief LWM2M custom object 16 host device information
 */
typedef struct
{
    qosa_uint16_t host_id;
    char          host_dev_id[QCM_LWM2M_HOST_DEVICE_PARAM_LEN];
    char          host_dev_manufacturer[QCM_LWM2M_HOST_DEVICE_PARAM_LEN];
    char          host_dev_model[QCM_LWM2M_HOST_DEVICE_PARAM_LEN];
    char          host_dev_swv[QCM_LWM2M_HOST_DEVICE_PARAM_LEN];
} qcm_lwm2m_host_device_t;

/**
 * @brief The request data structure for waiting for a response
 */
typedef struct
{
    qosa_q_link_type_t                list;
    qosa_uint16_t                     mid;            /*!< Corresponding to the actual MID value within the system*/
    qcm_lwm2m_object_operation_type_e operation_type; /*!< The type of operation of this request */
} lwm2m_pend_request_t;

/** Used for client interface LWM2M internal status event notification and message event processing */
typedef void (*qcm_lwm2m_client_notify_cb)(qcm_lwm2m_event_e event_id, void *arg);

/**
 * @struct qcm_lwm2m_server_info_t
 * @brief User-oriented, passing server information
 */
typedef struct
{
    qosa_bool_t               is_valid;      /*!< Clear this server information to FALSE */
    qosa_bool_t               bootstrap;     /*!< Whether it is connecting to bootstrap startup server*/
    qosa_bool_t               storing;       /*!< Whether to store offline notify information to be reported to server; default: OSA_FALSE not stored */
    qosa_int32_t              ssid;          /*!< Server ID */
    qcm_lwm2m_security_mode_e security_mode; /*!< Security mode 0: psk; 3: nosec */
    char                      server_url[CONFIG_QOSA_HOST_URL_MAX_LEN + 1]; /*!< Configure server address */
    int                       server_port;                                  /*!< Configure server port number */
    char                      psk_id[QCM_LWM2M_PSK_IDENTITY_LEN_MAX + 1];   /*!< PSK ID */
    char                      psk_key[QCM_LWM2M_PSK_KEY_LEN_MAX + 1];       /*!< PSK key */
    qosa_int32_t              life_time;                                    /*!< Heartbeat period */
    qosa_int32_t              period_min;                                   /*!< Observer minimum period */
    qosa_int32_t              period_max;                                   /*!< Observer maximum period */
    qosa_uint32_t              disable_timeout; /*!< Disable server connection duration, after timeout, retry connection registration */
    qcm_lwm2m_bind_mode_e     bind_mode;       /*!< Binding default: U or UQ, later also has UQS mode */
    char                      endpoint_name[QCM_LWM2M_ENDPOINT_NAME_LEN_MAX + 1];  /*!< Client endpoint */
    char                      ca_cert_path[CONFIG_QOSA_FILE_MAX_NAME_LEN + 1];     /*!< When using DTLS, server CA certificate path */
    char                      client_cert_path[CONFIG_QOSA_FILE_MAX_NAME_LEN + 1]; /*!< When using DTLS, client certificate path */
    char                      client_key_path[CONFIG_QOSA_FILE_MAX_NAME_LEN + 1];  /*!< When using DTLS, client certificate key path */
} qcm_lwm2m_server_info_t;

/**
 * @struct lwm2m_startup_t
 * @brief LWM2M startup management parameters
 * @note Auto-start configuration is managed and saved by user side, custom parameter attributes
 */
typedef struct
{
    qosa_uint8_t  per_regstatus_feat; /*!< After server issues restart, re-register connection */
    qosa_uint8_t  factory_reset;      /*!< Factory reset */
    qosa_uint16_t wait_afterBS;       /*!< After BS completion, delay duration, then register connection */
} qcm_lwm2m_startup_t;

/**
 * @struct lwm2m_connect_t
 * @brief Used for reconnection related configuration parameters
 */
typedef struct
{
    qosa_bool_t   reconnecting;       /*!< Reconnect flag whether to enable reconnection attempts or directly report error and exit */
    qosa_uint8_t  reconnection_times; /*!< Number of reconnections */
    qosa_time_t   reconn_delay_time;  /*!< Time count */
    qosa_int32_t  fail_resaon;        /*!< Server failure reason error code */
    qosa_uint32_t max_delay_time;     /*!< Maximum delay time of reconnect */
    qosa_uint32_t unitreconntime;     /*!< Unit of delay time of reconnect */
    qosa_uint32_t multreconntime;     /*!< Multiple of delay time of reconnect */
} qcm_lwm2m_reconnect_t;

/**
 * @struct lwm2m_client_config_t
 * @brief Server common information configuration
 */
typedef struct
{
    qosa_uint8_t  is_valid;           /*!< Whether configuration is valid */
    qosa_uint8_t  sim_id;
    qosa_uint8_t  pdp_cid;            /*!< Configure which PDP context to use for LWM2M registration */
    qosa_uint8_t  lifetime_enable;    /*!< Heartbeat enable */
    qosa_uint8_t  per_regstatus_feat; /*!< Server issued restart flag */
    qosa_uint8_t  rai_enable;         /*!< RAI enable */
    qosa_uint8_t  obs_auto_ack;       /*!< auto_ack_enable obs automatic reply */
    qosa_uint8_t  dtls_mode;          /*!< 0: full handshake 1: dtls resumption */
    qosa_bool_t   recovery_mode;      /*!< Power-on recovery mode, TRUE enable */
    qosa_uint8_t  retransmit_mode;    /*!< Retransmission interval mode */
    qosa_uint8_t  retrans_max_times;  /*!< Maximum number of packet retransmission attempts */
    qosa_uint8_t  dtls_version;       /*!< DTLS version */
    qosa_uint8_t  ack_timeout;        /*!< Reply timeout time */
    qosa_uint32_t register_timeout;   /*!< Register server timeout time, for PDP activation, DNS resolution timeout parameters should not exceed this value */
    qosa_uint32_t dtls_lifetime;      /*!< DTLS handshake heartbeat */
    qcm_lwm2m_reconnect_t lwm2m_connect;            /*!< Socket connect management information */
    qcm_lwm2m_startup_t   lwm2m_startup;            /*!< Startup management */
    qosa_uint8_t          def_inst_rsp_data_format; /*!< Data format, default 0 JSON format, 1 TLV format */
    qcm_lwm2m_sock_type_e sock_type;                /*!< Internal communication transmission method */
    qosa_bool_t           discover_func_support;    /*!< Whether to enable discover function, default not enabled */
    qosa_bool_t           write_attributes_support; /*!< Whether to enable write attributes support, default not enabled */
    qosa_uint32_t         deregister_timeout;       /*!< Deregister server timeout time */

    // Used to configure whether to enable corresponding objects; objects 0/1 cannot be configured, they are fixed enabled
    qosa_int32_t obj_id_container
        [QCM_LWM2M_DEFAULT_OBJ_CNT]; /*!< Object ID stored object ID resources, inherent objects in code such as: 2,3,4,5,7,9,11,3351,3370, etc. Configure whether to enable */
    qosa_uint32_t dns_ttl; /*!< DNS TTL cache time */
    // Parameters added sequentially later, need to consider new parameters without losing original configuration
} qcm_lwm2m_client_config_t;

/**
 * @struct lwm2m_uri_t
 * @brief The so-called URI corresponds to object-related information ID
 */
typedef struct
{
    qosa_uint16_t objectId;
    qosa_uint16_t instanceId;
    qosa_uint16_t resourceId;
#ifndef LWM2M_VERSION_1_0
    qosa_uint16_t resourceInstanceId;
#endif
} lwm2m_uri_t;

/*
 * The lwm2m_data_t is used to store LWM2M resource values in a hierarchical way.
 * Depending on the type the value is different:
 * - LWM2M_TYPE_OBJECT, LWM2M_TYPE_OBJECT_INSTANCE, LWM2M_TYPE_MULTIPLE_RESOURCE: value.asChildren
 * - LWM2M_TYPE_STRING, LWM2M_TYPE_OPAQUE, LWM2M_TYPE_CORE_LINK: value.asBuffer
 * - LWM2M_TYPE_INTEGER, LWM2M_TYPE_TIME: value.asInteger
 * - LWM2M_TYPE_UNSIGNED_INTEGER: value.asUnsigned
 * - LWM2M_TYPE_FLOAT: value.asFloat
 * - LWM2M_TYPE_BOOLEAN: value.asBoolean
 *
 * LWM2M_TYPE_STRING is also used when the data is in text format.
 */
typedef enum
{
    LWM2M_TYPE_UNDEFINED = 0,
    LWM2M_TYPE_OBJECT,
    LWM2M_TYPE_OBJECT_INSTANCE,
    LWM2M_TYPE_MULTIPLE_RESOURCE,

    LWM2M_TYPE_STRING,
    LWM2M_TYPE_OPAQUE,
    LWM2M_TYPE_INTEGER,
    LWM2M_TYPE_UNSIGNED_INTEGER,
    LWM2M_TYPE_FLOAT,
    LWM2M_TYPE_BOOLEAN,

    LWM2M_TYPE_OBJECT_LINK,
    LWM2M_TYPE_CORE_LINK
} lwm2m_data_type_t;

typedef struct _lwm2m_data_t lwm2m_data_t;

struct _lwm2m_data_t
{
    lwm2m_data_type_t type; /*!< Data type */
    qosa_uint16_t     id;   /*!< Attribute resource ID */
    union
    {
        qosa_bool_t   asBoolean;  /*!< Store boolean value */
        qosa_int64_t  asInteger;  /*!< Store signed integer value */
        qosa_uint64_t asUnsigned; /*!< Store unsigned integer value */
        double        asFloat;    /*!< Store floating-point value */
        struct
        {
            qosa_size_t   length; /*!< Store string/opaque/... length value */
            qosa_uint8_t *buffer; /*!< Store string/opaque/... */
        } asBuffer;
        struct
        {
            qosa_size_t   count; /*!< Multiple resource count */
            lwm2m_data_t *array; /*!< Pointer to multiple resource structure */
        } asChildren;
        struct
        {
            qosa_uint16_t objectId;         /*!< Associated object ID */
            qosa_uint16_t objectInstanceId; /*!< Associated object instance ID */
        } asObjLink;
    } value;                                /*!< Data value */
};

/**
 * @struct qcm_lwm2m_event_info_t
 * @brief Event reporting structure where msg_info points to specific transaction structure parameters, such as qcm_lwm2m_fw_data_t
 */
typedef struct
{
    qosa_int8_t        client_id;
    qosa_int32_t       result; /*!<  execution result */
    qcm_lwm2m_status_e status; /*!<  m2m_client_status */
    qosa_int32_t       ssid;   /*!<  short server id */
    void              *msg_info;
} qcm_lwm2m_event_info_t;

/**
 * @struct qcm_lwm2m_run_status_resp_t
 * @brief Reports current internal LWM2M registration status changes
 */
typedef struct
{
    qosa_uint8_t       client_id;
    qcm_lwm2m_status_e status; /*!< Current LWM2M running status */
} qcm_lwm2m_run_status_resp_t;

#if 1
/************************************************************************/
/* API request execution requests                                       */
/************************************************************************/

/**
 * @struct  qcm_lwm2m_instance_t
 * @brief  Instance information parameters
 */
typedef struct
{
    qosa_uint16_t instance_id;  /*!< object instance id */
    qosa_uint16_t resource_cnt; /*!< resource count */
    qosa_uint16_t resource[QCM_LWM2M_RESOURCES_CNT_MAX];
} qcm_lwm2m_instance_t;

/**
 * @struct  qcm_lwm2m_obj_t
 * @brief  Object information parameters
 */
typedef struct
{
    qosa_uint16_t        obj_id;       /*!< object id */
    qosa_uint16_t        instance_cnt; /*!< instance count */
    qcm_lwm2m_instance_t instance[QCM_LWM2M_INSTANCE_CNT_MAX];
} qcm_lwm2m_obj_t;

/**
 * @struct  qcm_lwm2m_obj_info_t
 * @brief  Object collection information parameters
 */
typedef struct
{
    qosa_int8_t     object_cnt;                    /*!< object count */
    qcm_lwm2m_obj_t object[QCM_LWM2M_OBJ_CNT_MAX]; /*!< object storage for user-defined objects */
} qcm_lwm2m_client_cust_obj_info_t;

/**
 * @enum qcm_lwm2m_ack_data_type_e
 * @brief Response type marker CON/NON
 */
typedef enum
{
    QCM_LWM2M_NON_ACK_MODE = 0,
    QCM_LWM2M_CON_ACK_MODE = 1
} qcm_lwm2m_ack_data_type_e;

/**
 * @struct qcm_lwm2m_cust_object_notify_t
 * @brief Structure for actively notifying server information
 */
typedef struct
{
    qosa_int8_t               client_id;   /*!<  Client serial index value, instance id */
    qosa_uint16_t             obj_id;      /*!< object id */
    qosa_uint16_t             instance_id; /*!< object instance id */
    qosa_int32_t              data_size;   /*!< Data count in lwm2m_data_t */
    qcm_lwm2m_ack_data_type_e ack_type;    /*!< 0 means NON type data, 1 means CON type data */
    lwm2m_data_t             *dataP;       /*!< resource data, needs to be released using lwm2m_data_free function */
} qcm_lwm2m_acitve_notify_info_t;

/**
 * @struct qcm_lwm2m_client_resp_read_req_info_t
 * @brief Structure for actively responding to server's read request content
 */
typedef struct
{
    qosa_int8_t                       client_id;
    qcm_lwm2m_object_operation_type_e type;        /*!< Active response type: respond to read, write, exec, observe */
    qosa_int32_t                      mid;         /*!< COAP HEADER: message id   */
    qcm_lwm2m_result_code_e           result;      /*!< COAP RESULT CODE */
    qosa_uint16_t                     obj_id;      /*!< object id */
    qosa_uint16_t                     instance_id; /*!< object instance id */
    qosa_int32_t                      data_size;   /*!< Data count in lwm2m_data_t */
    lwm2m_data_t                     *dataP;       /*!< resource data, needs to be released using lwm2m_data_free function */
} qcm_lwm2m_client_resp_read_req_info_t;

/**
 * @brief Structure for actively responding to server's observe request content
 */
typedef qcm_lwm2m_client_resp_read_req_info_t qcm_lwm2m_client_resp_observe_req_info_t;

#endif

/**
 * @struct  qcm_lwm2m_write_resource_t
 * @brief  Modify default object resource values, currently supports object id: 5/...
 */
typedef struct
{
    qosa_int8_t   client_id;
    qosa_int32_t  obj_id;      /*! object id  */
    qosa_int32_t  instance_id; /*! object instance id */
    qosa_int32_t  data_size;   /*!< Data count in lwm2m_data_t */
    lwm2m_data_t *dataP;       /*!< resource data, needs to be released using lwm2m_data_free function */
} qcm_lwm2m_write_resource_t;

#if 1
/************************************************************************/
/*  API response results                                               */
/************************************************************************/

/**
 * @struct qcm_lwm2m_register_resp_t
 * @brief Reports current LWM2M registration status
 */
typedef struct
{
    qosa_uint8_t           client_id;
    qosa_uint16_t          mid;        /*!< message ID */
    qcm_lwm2m_event_e      type_event; /*!< Event type */
    qcm_lwm2m_error_code_e result;     /*!< Execution result notification */
} qcm_lwm2m_register_resp_t;

/**
 * @brief Deregistration execution result report
 */
typedef qcm_lwm2m_register_resp_t qcm_lwm2m_deregister_resp_t;

/**
 * @brief Add object execution result report
 */
typedef qcm_lwm2m_register_resp_t qcm_lwm2m_add_object_resp_t;

/**
 * @brief Delete object execution result report
 */
typedef qcm_lwm2m_register_resp_t qcm_lwm2m_del_object_resp_t;
/**
 * @brief Firmware FOTA message result report
 */
typedef qcm_lwm2m_register_resp_t qcm_lwm2m_firmware_fota_t;

/**
 * @brief Execution response to write request API execution result
 */
typedef qcm_lwm2m_register_resp_t qcm_lwm2m_resp_write_req_return_t;

/**
 * @brief Execution response to exec request API execution result
 */
typedef qcm_lwm2m_register_resp_t qcm_lwm2m_resp_exec_req_return_t;

/**
 * @brief Execution response to read request API execution result
 */
typedef qcm_lwm2m_register_resp_t qcm_lwm2m_resp_read_req_return_t;

/**
 * @brief Execution response to observe request API execution result
 */
typedef qcm_lwm2m_register_resp_t qcm_lwm2m_resp_observe_req_return_t;

/**
 * @struct qcm_lwm2m_firmware_url_t
 * @brief Firmware package URL download address
 */
typedef struct
{
    qosa_uint8_t url[QCM_LWM2M_FIRMWARE_URL_LEN_MAX]; /*!< package url */
} qcm_lwm2m_firmware_url_t;

/**
 * @struct qcm_lwm2m_data_change_report_t
 * @brief Data change report
 */
typedef struct
{
    qosa_uint8_t           client_id;
    qcm_lwm2m_error_code_e result; /*!< Execution result notification */
    int                    data_size;
    lwm2m_data_t          *dataP;
} qcm_lwm2m_data_change_report_t;

/**
 * @brief Update lifetime ping status active notification
 */
typedef qcm_lwm2m_data_change_report_t qcm_lwm2m_update_ping_report_t;

/**
 * @brief Lifetime status change active notification
 */
typedef qcm_lwm2m_data_change_report_t qcm_lwm2m_lifetime_chg_report_t;

/**
 * @struct qcm_lwm2m_api_update_resp_t
 * @brief Execute update API interface, execution result returned asynchronously
 */
typedef struct
{
    qosa_uint8_t           client_id;
    qosa_uint16_t          mid;
    qcm_lwm2m_error_code_e result;
} qcm_lwm2m_update_resp_t;

/**
 * @enum qcm_lwm2m_changer_type_e
 * @brief User operation execution result, active report type
 */
typedef enum
{
    QCM_LWM2M_CHANGER_NONE = 0, /*!< Initial state */
    QCM_LWM2M_CHANGER_ADD_OBJECT = 1,
    QCM_LWM2M_CHANGER_DEL_OBJECT,
    QCM_LWM2M_CHANGER_LIFETIME_PARM,
    QCM_LWM2M_CHANGER_BIND_PARM,
    QCM_LWM2M_CHANGER_NOTIFY,
    QCM_LWM2M_CHANGER_ACTIVE_UPDATE,
} qcm_lwm2m_changer_type_e;

/**
 * @struct qcm_lwm2m_obj_chg_resp_t
 * @brief LWM2M change execution status return
 */
typedef struct
{
    qcm_lwm2m_changer_type_e opt_code; /*!<  qcm_lwm2m_chg_e */
    qcm_lwm2m_error_code_e   result;   /*!<  execution result */
} qcm_lwm2m_obj_chg_resp_t;

/**
 * @struct qcm_lwm2m_notify_resp_t
 * @brief Result response return after executing active notify notification
 */
typedef struct
{
    qosa_uint8_t              client_id;
    qosa_uint16_t             mid;
    qcm_lwm2m_ack_data_type_e ack_type;
    qcm_lwm2m_error_code_e    result;
} qcm_lwm2m_notify_resp_t;

/**
 * @struct qcm_lwm2m_notify_resp_t
 * @brief Result response return after executing active notify notification
 */
typedef struct
{
    qosa_uint8_t           client_id;
    qosa_uint16_t          mid;         /*!< Corresponding notify CON packet message ID */
    qcm_lwm2m_error_code_e result;      /*!< Corresponding notify response result */
    qosa_uint16_t          payload_len; /*!< Data length carried in server's CON ACK response packet */
    qosa_uint8_t          *payload;     /*!< Data carried in server's CON ACK response packet */
} qcm_lwm2m_notify_resp_ack_t;

/**
 * @struct qcm_lwm2m_cus_object_resp_t
 * @brief Data content for DM server to operate on customer-defined objects
 */
typedef struct
{
    qosa_uint8_t                      client_id;
    qcm_lwm2m_object_operation_type_e type;        /*!< Received server message type */
    qosa_int32_t                      mid;         /*!< COAP HEADER: message id   */
    qcm_lwm2m_result_code_e           result;      /*!< COAP RESULT CODE */
    qosa_uint16_t                     obj_id;      /*!< object id */
    qosa_uint16_t                     instance_id; /*!< object instance id */
    qosa_int32_t                      data_size;   /*!< Data count in lwm2m_data_t */
    lwm2m_data_t                     *dataP;       /*!< resource data, needs to be released using lwm2m_data_free function */
} qcm_lwm2m_cus_object_report_t;
/**
 * @struct qcm_lwm2m_cus_object_write_attr_report_t
 * @brief Corresponding to DM server downstream data write object attributes request
 */
typedef struct
{
    qosa_uint8_t  client_id;
    qosa_int32_t  mid;           /*!< COAP HEADER: message id   */
    qosa_uint16_t obj_id;        /*!< object id */
    qosa_uint16_t instance_id;   /*!< object instance id */
    qosa_uint16_t resource_id;   /*!< Corresponding object resource ID */
    qosa_uint8_t  toSet;         /*!< Bitmask indicating attributes to be set */
    qosa_uint8_t  toClear;       /*!< Bitmask indicating attributes to be cleared */
    qosa_uint32_t minPeriod;     /*!< Minimum period for resource updates (seconds) */
    qosa_uint32_t maxPeriod;     /*!< Maximum period for resource updates (seconds) */
    double        greaterThan;   /*!< Trigger notification when resource value is greater than this */
    double        lessThan;      /*!< Trigger notification when resource value is less than this */
    double        step;          /*!< Trigger notification when resource value changes by more than this step */
    char         *attr_buff;     /*!< Server-issued write attribute string separated by ";" */
    qosa_uint8_t  attr_buff_len; /*!< Corresponding write attribute string length */
} qcm_lwm2m_cus_object_write_attr_report_t;

#endif

/**
 * @enum qcm_lwm2m_firmware_package_e
 * @brief LWM2M firmware FOTA package download indication
 * @note:
0: First packet of firmware download
1: Firmware downloading
2: Last packet of firmware download
4: Internal deferred-update signal (not a real packet type)
*/
typedef enum
{
    QCM_LWM2M_FIRWARE_FIRST_PACKET_DATA = 0,
    QCM_LWM2M_FIRWARE_INTERMEDIATE_PACKET_DATA = 1,
    QCM_LWM2M_FIRWARE_LAST_PACKET_DATA = 2,
    QCM_LWM2M_FIRWARE_ABNORMAL_PACKET_DATA = 3,
    QCM_LWM2M_FIRWARE_UPDATE_PENDING_PACKET_DATA = 4 /*!< Internal signal: server UPDATE received during verify, trigger deferred update flow */
} qcm_lwm2m_firmware_package_e;

/**
 * @struct qcm_lwm2m_firmware_data_t
 * @brief In-band download data report
 */
typedef struct
{
    qcm_lwm2m_firmware_package_e eof;      /*!> download end sign */
    qosa_int16_t                 length;   /*!> data len */
    qosa_uint8_t                *data;
    qosa_int32_t                 err_code; /*!> error code*/
} qcm_lwm2m_firmware_data_t;

typedef enum
{
    LWM2M_CONTENT_TEXT = 0,  // Also used as undefined
    LWM2M_CONTENT_LINK = 40,
    LWM2M_CONTENT_OPAQUE = 42,
    LWM2M_CONTENT_TLV_OLD = 1542,   // Keep old value for backward-compatibility
    LWM2M_CONTENT_TLV = 11542,
    LWM2M_CONTENT_JSON_OLD = 1543,  // Keep old value for backward-compatibility
    LWM2M_CONTENT_JSON = 11543,
    LWM2M_CONTENT_SENML_JSON = 110,
    LWM2M_CONTENT_CBOR = 60,
    LWM2M_CONTENT_SENML_CBOR = 112,
} lwm2m_media_type_t;

/**
 * @enum lwm2m_firmware_state_e
 * @brief LWM2M firmware FOTA running status
 * note:
0: Idle (before downloading or after successful updating)
1: Downloading (The data sequence is on the way)
2: Downloaded
3: Updating
*/
typedef enum
{
    QCM_LWM2M_FIRWARE_STATE_IDLE = 0,
    QCM_LWM2M_FIRWARE_STATE_DOWNLOADING = 1,
    QCM_LWM2M_FIRWARE_STATE_DOWNLOADED = 2,
    QCM_LWM2M_FIRWARE_STATE_UPDATING = 3,
} lwm2m_firmware_state_e;

/**
 * @enum lwm2m_firmware_uptate_result_e
 * @brief LWM2M firmware FOTA update result value
 * note:
0: Initial value. Once the updating process is initiated (Download /Update), this Resource MUST be reset to Initial value.
1: Firmware updated successfully.
2: Not enough flash memory for the new firmware package.
3: Out of RAM during downloading process.
4: Connection lost during downloading process.
5: Integrity check failure for new downloaded package.
6: Unsupported package type.
7: Invalid URI.
8: Firmware update failed.
9: Unsupported protocol.
*/
typedef enum
{
    QCM_LWM2M_FIRWARE_UPDATE_DEFAULT = 0,
    QCM_LWM2M_FIRWARE_UPDATE_SUCCESSFUL,
    QCM_LWM2M_FIRWARE_UPDATE_NOT_ENOUGH_STORAGE,
    QCM_LWM2M_FIRWARE_UPDATE_OUT_OF_MEMORY,
    QCM_LWM2M_FIRWARE_UPDATE_CONNECTION_LOST,
    QCM_LWM2M_FIRWARE_UPDATE_CRC_FAILED,
    QCM_LWM2M_FIRWARE_UPDATE_UNSUPPORTED_TYPE,
    QCM_LWM2M_FIRWARE_UPDATE_INVALID_URI,
    QCM_LWM2M_FIRMWARE_UPDATE_FAILED,
    QCM_LWM2M_FIRMWARE_UPDATE_PROTOCOLFAIL
} lwm2m_firmware_uptate_result_e;

lwm2m_data_t *lwm2m_data_new(int size);
int           lwm2m_data_parse(lwm2m_uri_t *uriP, const qosa_uint8_t *buffer, qosa_size_t bufferLen, lwm2m_media_type_t format, lwm2m_data_t **dataP);
int           lwm2m_data_serialize(lwm2m_uri_t *uriP, int size, lwm2m_data_t *dataP, lwm2m_media_type_t *formatP, qosa_uint8_t **bufferP);
void          lwm2m_data_free(int size, lwm2m_data_t *dataP);
int           lwm2m_data_append(int *sizeP, lwm2m_data_t **dataP, int addDataSize, lwm2m_data_t *addDataP);
int           lwm2m_data_append_one(int *sizeP, lwm2m_data_t **dataP, lwm2m_data_type_t type, qosa_uint16_t id);

void lwm2m_data_encode_string(const char *string, lwm2m_data_t *dataP);
void lwm2m_data_encode_nstring(const char *string, qosa_size_t length, lwm2m_data_t *dataP);
void lwm2m_data_encode_opaque(const qosa_uint8_t *buffer, qosa_size_t length, lwm2m_data_t *dataP);
void lwm2m_data_encode_int(qosa_int64_t value, lwm2m_data_t *dataP);
int  lwm2m_data_decode_int(const lwm2m_data_t *dataP, qosa_int64_t *valueP);
void lwm2m_data_encode_uint(qosa_uint64_t value, lwm2m_data_t *dataP);
int  lwm2m_data_decode_uint(const lwm2m_data_t *dataP, qosa_uint64_t *valueP);
void lwm2m_data_encode_float(double value, lwm2m_data_t *dataP);
int  lwm2m_data_decode_float(const lwm2m_data_t *dataP, double *valueP);
void lwm2m_data_encode_bool(qosa_bool_t value, lwm2m_data_t *dataP);
int  lwm2m_data_decode_bool(const lwm2m_data_t *dataP, qosa_bool_t *valueP);
void lwm2m_data_encode_objlink(qosa_uint16_t objectId, qosa_uint16_t objectInstanceId, lwm2m_data_t *dataP);
void lwm2m_data_encode_corelink(const char *corelink, lwm2m_data_t *dataP);
void lwm2m_data_encode_instances(lwm2m_data_t *subDataP, qosa_size_t count, lwm2m_data_t *dataP);
void lwm2m_data_include(lwm2m_data_t *subDataP, qosa_size_t count, lwm2m_data_t *dataP);
//add larson February 22, 2024 11:37:04
int lwm2m_data_decode_string(const lwm2m_data_t *dataP, char *string, qosa_size_t length);

/**
 * @brief Initialize LWM2M app internal resource information
 *
 * @param[in] qosa_int8_t client_id
 *          - Client ID resource that needs to be initialized
 *
 * @param[in] qcm_lwm2m_client_notify_cb notify_cb
 *          - Event notification callback function for the application layer
 *
 * @return qcm_lwm2m_error_code_e
 *       - Returns QCM_LWM2M_ERRID_SUCCESS on success
 *       - Returns other values on failure
 */
qcm_lwm2m_error_code_e qcm_lwm2m_client_init(qosa_int8_t client_id, qcm_lwm2m_client_notify_cb notify_cb, const char *config_tag);

/**
 * @brief De-register LWM2M operation and destroy resources
 *
 * @param[in] qosa_int8_t client_id
 *          - Corresponding clientID from osa_m2m_client_init initialization
 *
 * @return qcm_lwm2m_error_code_e
 *       - Returns QCM_LWM2M_ERRID_SUCCESS on success
 *       - Returns other values on failure
 */
qcm_lwm2m_error_code_e qcm_lwm2m_client_deinit(qosa_int8_t client_id);

/**
 * @brief After LWM2M initialization is completed and corresponding config items are configured, start LWM2M registration
 *
 * @param[in] qcm_lwm2m_reg_t * reg_param
 *          - Parameters related to LWM2M registration request
 *
 * @return qcm_lwm2m_error_code_e
 *       - Returns QCM_LWM2M_ERRID_SUCCESS on success
 *       - Returns other values on failure
 */
qcm_lwm2m_error_code_e qcm_lwm2m_client_register(qosa_int8_t client_id);

/**
 * @brief LWM2M app actively updates lifetime time
 *
 * @param[in] qosa_int8_t client_id
 *          - Client ID to be operated and updated
 *
 * @param[in] qosa_uint32_t lifetime
 *          - New time to be updated
 *
 * @return qcm_lwm2m_error_code_e
 *       - Returns QCM_LWM2M_ERRID_SUCCESS on success
 *       - Returns other values on failure
 */
qcm_lwm2m_error_code_e qcm_lwm2m_client_update_lifetime(qosa_int8_t client_id, qosa_uint32_t lifetime, qosa_bool_t with_objects);

/**
 * @brief Update the binding mode between LWM2M app and service
 *
 * @param[in] qosa_int8_t client_id
 *          - Client ID to be operated and updated
 *
 * @param[in] qcm_lwm2m_bind_mode_e bind_mode
 *          - Binding mode type to be set
 *
 * @return qcm_lwm2m_error_code_e
 *       - Returns QCM_LWM2M_ERRID_SUCCESS on success
 *       - Returns other values on failure
 */
qcm_lwm2m_error_code_e qcm_lwm2m_client_update_bind_mode(qosa_int8_t client_id, qcm_lwm2m_bind_mode_e bind_mode, qosa_bool_t with_objects);

/**
 * @brief LWM2M initiates deregistration request to server
 *
 * @param[in] qcm_lwm2m_dereg_t * dereg_ptr
 *          - LWM2M deregistration request parameters
 *
 * @return qcm_lwm2m_error_code_e
 *       - Returns QCM_LWM2M_ERRID_SUCCESS on success
 *       - Returns other values on failure
 */
qcm_lwm2m_error_code_e qcm_lwm2m_client_dereg(qosa_int8_t client_id);

/**
 * @brief Get the LWM2M running status corresponding to the client ID connection
 *
 * @param[in] qosa_int8_t client_id
 *          - LWM2M client ID to be operated
 *
 * @param[out] qcm_lwm2m_status_e *status
 *          - Returns the current internal running status of LWM2M
 *
 * @return qcm_lwm2m_error_code_e
 *       - Returns QCM_LWM2M_ERRID_SUCCESS on success
 *       - Returns other values on failure
 */
qcm_lwm2m_error_code_e qcm_lwm2m_client_get_status(qosa_int8_t client_id, qcm_lwm2m_status_e *status);

/**
 * @brief Module deep sleep wake-up or power-on again, restore previous running state
 */
qcm_lwm2m_error_code_e qcm_lwm2m_client_recover(qosa_int8_t client_id, qcm_lwm2m_client_notify_cb notify_cb);

/**
 * @brief Module restart
 */
qcm_lwm2m_error_code_e qcm_lwm2m_client_reboot(void);
#ifdef CONFIG_QCM_LWM2M_DTLS_FUNC
/**
 * @brief Configure SSL properties
 *
 * @param[in] qosa_int8_t client_id
 *          - LWM2M client ID to be operated
 *
 * @param[in] qcm_ssl_config_t *ssl_config
 *          - User-defined SSL configuration
 *
 * @return qcm_lwm2m_error_code_e
 *       - Returns QCM_LWM2M_ERRID_SUCCESS on success
 *       - Returns other values on failure
 */
qcm_lwm2m_error_code_e qcm_lwm2m_client_dtls_config(qosa_int8_t client_id, qcm_ssl_config_t *ssl_config);
#endif
/**
 * @brief Customize LWM2M app properties before connection starts
 *
 * @param[in] qosa_int8_t client_id
 *          - LWM2M client ID to be operated
 *
 * @param[in] qcm_lwm2m_client_config_t * config_ptr
 *          - Pointer to the config content to be modified
 *
 * @return qcm_lwm2m_error_code_e
 *       - Returns QCM_LWM2M_ERRID_SUCCESS on success
 *       - Returns other values on failure
 */
qcm_lwm2m_error_code_e qcm_lwm2m_client_set_config(qosa_int8_t client_id, qcm_lwm2m_client_config_t *config_ptr);

/**
 * @brief Get the current configuration set by LWM2M app
 *
 * @param[in] qosa_int8_t client_id
 *          - LWM2M client ID to be operated
 *
 * @param[out] qcm_lwm2m_client_config_t * config_ptr
 *          - Returns the corresponding config configuration
 *
 * @return qcm_lwm2m_error_code_e
 *       - Returns QCM_LWM2M_ERRID_SUCCESS on success
 *       - Returns other values on failure
 */
qcm_lwm2m_error_code_e qcm_lwm2m_client_get_config(qosa_int8_t client_id, qcm_lwm2m_client_config_t *config_ptr);

/**
 * @brief Set the server configuration corresponding to the client ID
 *
 * @param[in] qosa_int8_t client_id
 *          - LWM2M client ID to be operated
 *
 * @param[in] qcm_lwm2m_server_opt_e type
 *          - Server type to be configured
 *
 * @param[in] qcm_lwm2m_server_info_t * server_ptr
 *          - Server setting content to be configured
 *
 * @return qcm_lwm2m_error_code_e
 *       - Returns QCM_LWM2M_ERRID_SUCCESS on success
 *       - Returns other values on failure
 */
qcm_lwm2m_error_code_e qcm_lwm2m_client_set_server_config(qosa_int8_t client_id, qcm_lwm2m_server_info_t *server_ptr);

/**
 * @brief Get the configuration content of the server type corresponding to the client ID
 *
 * @param[in] qosa_int8_t client_id
 *          - LWM2M client ID to be operated
 *
 * @param[in] qcm_lwm2m_server_opt_e type
 *          - Server type to be configured
 *
 * @param[out] qcm_lwm2m_server_info_t * server_ptr
 *          - Get the corresponding server configuration content
 *
 * @return qcm_lwm2m_error_code_e
 *       - Returns QCM_LWM2M_ERRID_SUCCESS on success
 *       - Returns other values on failure
 */
qcm_lwm2m_error_code_e qcm_lwm2m_client_get_server_config(qosa_int8_t client_id, qcm_lwm2m_server_info_t *server_ptr);

/**
 * @brief Customize object attribute information
 *
 * @param[in] qosa_int8_t client_id
 *          - LWM2M client ID to be operated
 *
 * @param[in] qcm_lwm2m_custom_type_e custom_type
 *          - Enumeration value of the structure parameter to be modified
 *
 * @param[in] void *custom_info
 *          - Pointer to the object attribute structure parameter to be modified
 *
 * @return qcm_lwm2m_error_code_e
 *       - Returns QCM_LWM2M_ERRID_SUCCESS on success
 *       - Returns other values on failure
 *
 * @note Currently supports setting object3/16 qcm_lwm2m_device_info_t/qcm_lwm2m_host_device_t
 */
qcm_lwm2m_error_code_e qcm_lwm2m_client_set_custom_info(qosa_int8_t client_id, qcm_lwm2m_custom_type_e custom_type, void *custom_info);

/**
 * @brief Get custom object attribute information
 *
 * @param[in] qosa_int8_t client_id
 *          - LWM2M client ID to be operated
 *
 * @param[in] qcm_lwm2m_custom_type_e custom_type
 *          - Enumeration value of the structure parameter to be modified
 *
 * @param[in/out] void *custom_info
 *          - Pointer to the object attribute structure parameter to be modified
 *
 * @return qcm_lwm2m_error_code_e
 *       - Returns QCM_LWM2M_ERRID_SUCCESS on success
 *       - Returns other values on failure
 *
 * @note Currently supports setting object3/16 qcm_lwm2m_device_info_t/qcm_lwm2m_host_device_t
 */
qcm_lwm2m_error_code_e qcm_lwm2m_client_get_custom_info(qosa_int8_t client_id, qcm_lwm2m_custom_type_e custom_type, void *custom_info);

/**
 * @brief LWM2M client restore factory settings configuration
 *
 * @return qcm_lwm2m_error_code_e
 *       - Returns QCM_LWM2M_ERRID_SUCCESS on success
 *       - Returns other values on failure
 */
qcm_lwm2m_error_code_e qcm_lwm2m_client_reset(void);

/**
 * @brief User adds custom object resources
 *
 *  This API is asynchronous, whether the object information is successfully added is reported through event callback;
 *  Use this API for AT command usage or adding objects after registered connection
 *
 * - object id        - 10001
 *   - instance id    - 0
 *     - resource     - 5001 5002 5003
 *
 * @param[in] qosa_int8_t client_id
 *          - Client ID to be operated
 *
 * @param[in] qosa_uint32_t obj_id
 *          - Object ID to be added
 *
 * @param[in] qosa_uint32_t instance_id
 *          - Instance ID to be added
 *
 * @param[in] qosa_uint32_t resource_cnt
 *          - Number of resources to be added
 *
 * @param[in] qosa_uint32_t res_id[]
 *          - Array of resource values to be added
 *
 * @return qcm_lwm2m_error_code_e
 *       - Returns QCM_LWM2M_ERRID_SUCCESS on success
 *       - Returns other values on failure
 */
qcm_lwm2m_error_code_e qcm_lwm2m_client_add_customer_object(
    qosa_int8_t   client_id,
    qosa_uint32_t obj_id,
    qosa_uint32_t instance_id,
    qosa_uint32_t resource_cnt,
    qosa_uint32_t res_id[]
);

/**
 * @brief User deletes custom object resources
 */
qcm_lwm2m_error_code_e qcm_lwm2m_client_del_customer_object(qosa_int8_t client_id, qosa_uint32_t obj_id);

/**
* @brief Query user-defined object information
*
*/
qcm_lwm2m_error_code_e qcm_lwm2m_client_get_customer_object(qosa_int8_t client_id, qcm_lwm2m_client_cust_obj_info_t *obj_ptr);

/**
 * @brief Check if the corresponding object is a standard object
 *
 * @param[in] qosa_uint16_t obj_id
 *          - Object ID number to be checked
 *
 * @return qosa_bool_t
 *       - Returns OSA_TRUE if it is a default object
 *       - Returns OSA_FALSE if not
 */
qosa_bool_t qcm_lwm2m_client_check_is_standard_object(qosa_uint16_t obj_id);

/**
 * @brief For subscribed resources, actively report data changes to the server
 */
qcm_lwm2m_error_code_e qcm_lwm2m_client_object_notify(qcm_lwm2m_acitve_notify_info_t *notify_info);

/**
 * @brief Reset all internal LWM2M configurations
 */
qcm_lwm2m_error_code_e qcm_lwm2m_client_config_reset(char *config_tag);

/**
 * @brief Respond to server's write request action, respond to write request
 *
 * @param[in] qosa_int8_t client_id
 *          - Client ID action to be operated
 *
 * @param[in] qosa_uint16_t mid
 *          - Corresponding message ID
 *
 * @param[in] qcm_lwm2m_result_code_e result
 *          - Corresponding response result status
 *
 * @return qcm_lwm2m_error_code_e
 *       - Returns QCM_LWM2M_ERRID_SUCCESS on success
 *       - Returns other values on failure
 */
qcm_lwm2m_error_code_e qcm_lwm2m_client_response_dm_write_request(qosa_int8_t client_id, qosa_uint16_t mid, qcm_lwm2m_result_code_e result);

/**
 * @brief Respond to server's execute request action, respond to execute request
 *
 * @param[in] qosa_int8_t client_id
 *          - Client ID action to be operated
 *
 * @param[in] qosa_uint16_t mid
 *          - Corresponding message ID
 *
 * @param[in] qcm_lwm2m_result_code_e result
 *          - Corresponding response result status
 *
 * @return qcm_lwm2m_error_code_e
 *       - Returns QCM_LWM2M_ERRID_SUCCESS on success
 *       - Returns other values on failure
 */
qcm_lwm2m_error_code_e qcm_lwm2m_client_response_dm_execute_request(qosa_int8_t client_id, qosa_uint16_t mid, qcm_lwm2m_result_code_e result);

/**
 * @brief Respond to server's read request action, respond to read request
 *
 * @param[in] qcm_lwm2m_client_resp_read_req_info_t * resp_ptr
 *          - Response message content when responding to read request
 *
 * @return qcm_lwm2m_error_code_e
 *       - Returns QCM_LWM2M_ERRID_SUCCESS on success
 *       - Returns other values on failure
 */
qcm_lwm2m_error_code_e qcm_lwm2m_client_response_dm_read_request(qcm_lwm2m_client_resp_read_req_info_t *resp_ptr);

/**
 * @brief Respond to server's observe request action, respond to observe request
 *
 * @param[in] qcm_lwm2m_client_resp_observe_req_info_t * resp_ptr
 *          - Response message content when responding to observe request
 *
 * @return qcm_lwm2m_error_code_e
 *       - Returns QCM_LWM2M_ERRID_SUCCESS on success
 *       - Returns other values on failure
 */
qcm_lwm2m_error_code_e qcm_lwm2m_client_response_dm_observe_request(qcm_lwm2m_client_resp_observe_req_info_t *resp_ptr);

/**
 * @brief Respond to server's write attributes request action, respond to write attributes
 *
 * @param[in] qosa_int8_t client_id
 *          - Client ID action to be operated
 *
 * @param[in] qosa_uint16_t mid
 *          - Corresponding message ID
 *
 * @param[in] qcm_lwm2m_result_code_e result
 *          - Corresponding response result status
 *
 * @return qcm_lwm2m_error_code_e
 *       - Returns QCM_LWM2M_ERRID_SUCCESS on success
 *       - Returns other values on failure
 */
qcm_lwm2m_error_code_e qcm_lwm2m_client_response_dm_write_att_request(qosa_int8_t client_id, qosa_uint16_t mid, qcm_lwm2m_result_code_e result);

/**
 * @brief User actively responds to server's discover request
 *
 *  This API is asynchronous, whether the object information is successfully added is reported through event callback;
 *  Use this API for AT command usage or adding objects after registered connection
 *
 * - object id        - 10001
 *   - instance id    - 0
 *     - resource     - 5001 5002 5003
 *
 * @param[in] qosa_int8_t client_id
 *          - Client ID to be operated
 *
 * @param[in] qosa_uint32_t message_id
 *          - Corresponding response message ID
 *
 * @param[in] qcm_lwm2m_result_code_e result
 *          - Corresponding response result status
 *
 * @param[in] qosa_uint32_t obj_id
 *          - Object ID to be added
 *
 * @param[in] qosa_uint32_t resource_cnt
 *          - Number of resources to be added
 *
 * @param[in] qosa_uint32_t res_id[]
 *          - Array of resource values to be added
 *
 * @return qcm_lwm2m_error_code_e
 *       - Returns QCM_LWM2M_ERRID_SUCCESS on success
 *       - Returns other values on failure
 */
qcm_lwm2m_error_code_e qcm_lwm2m_client_response_dm_discover_request(
    qosa_int8_t             client_id,
    qosa_uint32_t           message_id,
    qcm_lwm2m_result_code_e result,
    qosa_uint16_t           obj_id,
    qosa_uint16_t           resource_cnt,
    qosa_uint16_t           res_id[]
);

/**
 * @brief User configures custom object resources
 *
 * This API is synchronous, the return value can determine whether the added object information is successful,
 * Generally used for OPEN projects, configure and add objects before registered connection, use this API
 *
 * - object id        - 10001
 *   - instance id    - 0
 *     - resource     - 5001 5002 5003
 *
 * @param[in] qosa_int8_t client_id
 *          - Client ID to be operated
 *
 * @param[in] qosa_uint32_t obj_id
 *          - Object ID to be added
 *
 * @param[in] qosa_uint32_t instance_id
 *          - Instance ID to be added
 *
 * @param[in] qosa_uint32_t resource_cnt
 *          - Number of resources to be added
 *
 * @param[in] qosa_uint32_t res_id[]
 *          - Array of resource values to be added
 *
 * @return qcm_lwm2m_error_code_e
 *       - Returns QCM_LWM2M_ERRID_SUCCESS on success
 *       - Returns other values on failure
 */
qcm_lwm2m_error_code_e qcm_lwm2m_client_config_set_customer_object(
    qosa_int8_t   client_id,
    qosa_uint16_t obj_id,
    qosa_uint16_t instance_id,
    qosa_uint8_t  resource_cnt,
    qosa_uint16_t res_id[]
);

/**
 * @brief Actively modify default object parameter values, such as supporting object firmware/...
 *
 * @param[in] qcm_lwm2m_write_resource_t * resource_info
 *          - Resource value parameters to be modified
 *
 * @return qcm_lwm2m_error_code_e
 *       - Returns QCM_LWM2M_ERRID_SUCCESS on success
 *       - Returns other values on failure
 */
qcm_lwm2m_error_code_e qcm_lwm2m_client_write_resource(qcm_lwm2m_write_resource_t *resource_info);

#endif /* __QCM_LWM2M_API_H__ */
