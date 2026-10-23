/*****************************************************************/ /**
* @file qcm_mqtt.h
* @brief
* @author lawrence.liu@quectel.com
* @date 2023-08-09
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2023-08-09 <td>1.0 <td>lawrence.liu <td> Init
* <tr><td>2025-07-14 <td>1.0 <td>lawrence.liu <td> MQTT 5.0 api
* </table>
**********************************************************************/
#ifndef __QCM_MQTT_H__
#define __QCM_MQTT_H__

#include "qosa_def.h"
#include "qosa_sys.h"
#include "qcm_proj_config.h"
#include "qosa_queue_list.h"
#include "qcm_mqtt_config.h"
#ifdef CONFIG_QCM_VTLS_FUNC
#include "qcm_vtls_cfg.h"
#endif /* CONFIG_QCM_VTLS_FUNC */

/** Starting position of MQTT general error codes allocated in OSA. */
#define QCM_ERRCODE_MQTT_BASE       (QOSA_COMPONENT_MQTT << 16)

/** Minimum value of MQTT packet identifier. */
#define QCM_MQTT_MIN_PACKET_ID      0
/** Maximum value of MQTT packet identifier. */
#define QCM_MQTT_MAX_PACKET_ID      65535
#define QCM_MQTT_SUBACK_RETURN_CODE 0x80

#define QCM_MQTT_USER_PROPERTY(k, v)                                                                                                                           \
    (qcm_mqtt_user_property_t)                                                                                                                                 \
    {                                                                                                                                                          \
        {(char *)(k), qosa_strlen(k)},                                                                                                                         \
        {                                                                                                                                                      \
            (char *)(v), qosa_strlen(v)                                                                                                                        \
        }                                                                                                                                                      \
    }
#ifdef CONFIG_QCM_MQTT5_FUNC
#define QCM_MQTT_SET_SUB_TOPIC(topic, qos)                                                                                                                     \
    {                                                                                                                                                          \
        {topic, qosa_strlen(topic)}, qos,                                                                                                                      \
        {                                                                                                                                                      \
            0, 0, 0                                                                                                                                            \
        }                                                                                                                                                      \
    }
#define QCM_MQTT5_SET_SUB_TOPIC(topic, qos, a, b, c)                                                                                                           \
    {                                                                                                                                                          \
        {topic, qosa_strlen(topic)}, qos,                                                                                                                      \
        {                                                                                                                                                      \
            a, b, c                                                                                                                                            \
        }                                                                                                                                                      \
    }

#define QCM_MQTT_SET_PUB_INIT(msg_id, topic, qos, retain, payload)                                                                                             \
    {                                                                                                                                                          \
        msg_id, qos, retain, {topic, qosa_strlen(topic)}, {payload, qosa_strlen(payload)},                                                                     \
        {                                                                                                                                                      \
            0                                                                                                                                                  \
        }                                                                                                                                                      \
    }
#define QCM_MQTT5_SET_PUB_INIT(msg_id, topic, qos, retain, payload, prop)                                                                                      \
    {                                                                                                                                                          \
        msg_id, qos, retain, {topic, qosa_strlen(topic)}, {payload, qosa_strlen(payload)}, prop                                                                \
    }
#else
#define QCM_MQTT_SET_SUB_TOPIC(topic, qos)                                                                                                                     \
    {                                                                                                                                                          \
        {topic, qosa_strlen(topic)}, qos                                                                                                                       \
    }
#define QCM_MQTT_SET_PUB_INIT(msg_id, topic, qos, retain, payload)                                                                                             \
    {                                                                                                                                                          \
        msg_id, qos, retain, {topic, qosa_strlen(topic)},                                                                                                      \
        {                                                                                                                                                      \
            payload, qosa_strlen(payload)                                                                                                                      \
        }                                                                                                                                                      \
    }
#endif /* CONFIG_QCM_MQTT5_FUNC */

/**
 * @brief MQTT supported protocol versions
 */
typedef enum
{
    QCM_MQTT_VERSION_V3 = 3,     /*!< MQTT 3.0 protocol specification */
    QCM_MQTT_VERSION_V3_1_1 = 4, /*!< MQTT 3.1.1 protocol specification */
#ifdef CONFIG_QCM_MQTT5_FUNC
    QCM_MQTT_VERSION_V5 = 5,     /*!< MQTT 5.0 protocol specification */
#endif                           /* CONFIG_QCM_MQTT5_FUNC */
    QCM_MQTT_VERSION_MAX,        /*!< max , reserve */
} qcm_mqtt_support_version_e;

/**
 * @enum qcm_mqtt_quality_of_service_e.
 * @brief Enumeration of MQTT Quality of Service (QoS).
 */
typedef enum
{
    QCM_MQTT_AT_MOST_ONCE_DELIVERY = 0,  /*!< Indicates QOS=0, at most once delivery. */
    QCM_MQTT_AT_LEAST_ONCE_DELIVERY = 1, /*!<  Indicates QOS=1, at least once delivery. */
    QCM_MQTT_EXACTLY_ONCE_DELIVERY = 2,  /*!< Indicates QOS=2, exactly once delivery. */
} qcm_mqtt_quality_of_service_e;

/**
 * @enum mqtt_state_e.
 * @brief MQTT client states.
 */
typedef enum
{
    QCM_MQTT_STATE_INIT = 0,               /*!< MQTT client initial default state, indicates the client is ready to connect. */
    QCM_MQTT_STATE_DATACALL_ING = 1,       /*!< MQTT client is performing datacall activation */
    QCM_MQTT_STATE_DNS_QUERY = 2,          /*!< MQTT client in DNS request phase. */
    QCM_MQTT_STATE_TCP_CONNECTING = 3,     /*!< MQTT client in TCP connection. */
    QCM_MQTT_STATE_SSL_HANDSHAKE = 4,      /*!< MQTT client in SSL handshake. */
    QCM_MQTT_STATE_TCP_CONNECTED = 5,      /*!< MQTT client TCP connection completed. */
    QCM_MQTT_STATE_MQTT_CONNECTING = 6,    /*!< MQTT client in protocol connection. */
    QCM_MQTT_STATE_MQTT_CONNECTED = 7,     /*!< MQTT client protocol connection completed. */
    QCM_MQTT_STATE_MQTT_DISCONNECTING = 8, /*!< MQTT client protocol closing. */
    QCM_MQTT_STATE_TCP_CLOSING = 9,        /*!< MQTT client TCP closing. */
    QCM_MQTT_STATE_TCP_CLOSED = 10,        /*!< MQTT client TCP closed state. */
} qcm_mqtt_state_e;

/**
 * @enum qcm_mqtt_send_result_type_e
 * @brief MQTT message sending result status enumeration
 * @details This enumeration defines the possible results when sending MQTT messages,
 *          including success, retry and error conditions based on QoS levels and ACK responses
 */
typedef enum
{
    QCM_MQTT_SEND_OK = 0, /*!< Data packet sent successfully. For QoS 1/2, this means ACK/PUBCOMP was received. For QoS 0, no ACK is required */
    QCM_MQTT_SEND_RETRY,  /*!< No MQTT protocol ACK was received within the specified timeout, or PUBCOMP was not completed, resulting in retransmission */
    QCM_MQTT_SEND_ERROR,  /*!< Error occurred after retry attempts were exhausted and no MQTT protocol ACK was received */
} qcm_mqtt_send_result_type_e;

/**
 * @enum qcm_mqtt_eercode_e.
 * @brief MQTT interface error codes and their corresponding issues.
 */
typedef enum
{
    QCM_MQTT_RES_OK = QOSA_OK,                              /*!< Indicates MQTT function executed successfully. */
    QCM_MQTT_RES_ERROR_GENERAL = 1 | QCM_ERRCODE_MQTT_BASE, /*!< Indicates unknown MQTT error (broad error, rarely used). */
    QCM_MQTT_RES_ERROR_NO_MEMORY,                           /*!< Indicates MQTT function failed to allocate memory. */
    QCM_MQTT_RES_ERROR_PARAM_INVALID,                       /*!< Indicates invalid parameters in MQTT function. */
    QCM_MQTT_RES_ERROR_PARAM_IS_NULL,                       /*!< Indicates null parameters in MQTT function. */
    QCM_MQTT_RES_ERROR_STATUS,                              /*!< Indicates incorrect MQTT client state. */
    QCM_MQTT_RES_ERROR_CLIENT_EXIST,                        /*!< Indicates MQTT client does not exist. */
    QCM_MQTT_RES_ERROR_ID_USE,                              /*!< Indicates MQTT client already exists. */
    QCM_MQTT_RES_ERROR_DNS,                                 /*!< Indicates MQTT function failed to get IP via DNS. */
    QCM_MQTT_RES_ERROR_IP_VERSION,                          /*!< Indicates version mismatch between MQTT server address and PDP obtained IP address. */
    QCM_MQTT_RES_ERROR_CREATE_TIMER,                        /*!< Indicates MQTT timer creation failed. */
    QCM_MQTT_RES_ERROR_START_TIMER,                         /*!< Indicates MQTT timer enable failed. */
    QCM_MQTT_RES_ERROR_STOP_TIMER,                          /*!< Indicates MQTT timer stop failed. */
    QCM_MQTT_RES_ERROR_MSG_ID_DUPLICATE,                    /*!< Indicates duplicate MSG ID not released. */
    QCM_MQTT_RES_ERROR_STORE_FAIL,                          /*!< Indicates MQTT failed to get receive cache ID. */
    QCM_MQTT_RES_NOT_READ_DATA,                             /*!< Indicates MQTT cache data read failed. */
    QCM_MQTT_RES_ERROR_SUBCRIBE,                            /*!< Indicates MQTT topic subscription failed. */
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
    QCM_MQTT_RES_ERROR_DATACALL,                            /*!< Datacall activation request failed. */
#endif
    QCM_MQTT_RES_ERROR_SOCKET_CREATE,                       /*!< MQTT SOCKET creation failed. */
    QCM_MQTT_RES_ERROR_TCP_CONNECT,                         /*!< MQTT TCP connection failed. */
    QCM_MQTT_RES_ERROR_CONNECT_TIMEOUT,                     /*!< CONNECT packet no response from server within time. */
    QCM_MQTT_RES_ERROR_TIMEOUT,                             /*!< No response within time, timeout error. */
    QCM_MQTT_RES_ERROR_TASK,                                /*!< Indicates MQTT TASK does not exist. */
    QCM_MQTT_RES_ERROR_PACKET,                              /*!< Indicates MQTT internal packet assembly error. */
    QCM_MQTT_RES_ERROR_SSL_HS_FAIL,                         /*!< Indicates SSL/TLS handshake failed. */
    QCM_MQTT_RES_ERROR_CONN_REFUSED,                        /*!< Indicates MQTT client protocol connection refused. */
    QCM_MQTT_RES_ERROR_NETWORK,                             /*!< Indicates MQTT client disconnected due to network reasons. */
    QCM_MQTT_RES_ERROR_STORE_FULL,                          /*!< Indicates that the MQTT client receive buffer is full. */
    QCM_MQTT_RES_ERROR_PROTOCOL_FAIL,                       /*!< Indicates that the MQTT client received an incorrect protocol. */
    QCM_MQTT_RES_ERROR_DATA_WOULDBLOCK,                     /*!< Indicates that the MQTT client has insufficient protocol data to parse. */
#ifdef CONFIG_QCM_MQTT5_FUNC
    QCM_MQTT_RES_ERROR_INVALID_PROPERTY_ID,                 /*!< Indicates that the MQTT client did not find a suitable property ID. */
#endif                                                      /* CONFIG_QCM_MQTT5_FUNC */
} qcm_mqtt_eercode_e;

/**
 * @enum qcm_mqtt_client_event_e.
 * @brief Events reported by MQTT client.
 */
typedef enum
{
    QCM_MQTT_CLIENT_OPEN_EVENT = 1,         /*!< MQTT client TCP connection creation event. */
    QCM_MQTT_CLIENT_CLOSE_EVENT = 2,        /*!< MQTT client TCP connection close event. */
    QCM_MQTT_CLIENT_CONNECT_EVENT = 3,      /*!< MQTT client MQTT protocol connection creation event. */
    QCM_MQTT_CLIENT_SUBSCRIBE_EVENT = 4,    /*!< MQTT client topic subscription event. */
    QCM_MQTT_CLIENT_SUBACK_EVENT = 5,       /*!< MQTT client topic subscription response event. */
    QCM_MQTT_CLIENT_UNSUBSCRIBE_EVENT = 6,  /*!< MQTT client topic unsubscription event. */
    QCM_MQTT_CLIENT_UNSUBACK_EVENT = 7,     /*!< MQTT client unsubscribe response event. */
    QCM_MQTT_CLIENT_PUBLISH_EVENT = 8,      /*!< MQTT client message publish event. */
    QCM_MQTT_CLIENT_PING_EVENT = 9,         /*!< MQTT client PING request event. */
    QCM_MQTT_CLIENT_DISCONNECT_EVENT = 10,  /*!< MQTT client MQTT protocol connection close event. */
    QCM_MQTT_CLIENT_NEW_MESSAGE_EVENT = 11, /*!< MQTT client message received event. */
    QCM_MQTT_CLIENT_STATE_EVENT = 12,       /*!< MQTT client state event. */
#ifdef CONFIG_QCM_MQTT5_FUNC
    QCM_MQTT_CLIENT_AUTH_EVENT = 13,        /*!< MQTT client authentication event. */
#endif                                      /* CONFIG_QCM_MQTT5_FUNC */
    QCM_MQTT_CLIENT_MAX_EVENT,              /*!< MQTT client MAX event. */
} qcm_mqtt_client_event_e;

/**
 * @enum qcm_mqtt_client_close_reason_e.
 * @brief MQTT client close reasons.
 */
typedef enum
{
    QCM_MQTT_CLOSE_BY_ACIVE_CLOSE = 0,      /*!< Active close. */
    QCM_MQTT_CLOSE_BY_PEER_RESET = 1,       /*!< Close due to peer reset. */
    QCM_MQTT_CLOSE_BY_PING_TIMEOUT = 2,     /*!< Close due to ping timeout. */
    QCM_MQTT_CLOSE_BY_CONN_TIMEOUT = 3,     /*!< Close due to connection timeout. */
    QCM_MQTT_CLOSE_BY_CONNACK_FAIL = 4,     /*!< Close due to connack response failure. */
    QCM_MQTT_CLOSE_BY_DISCONN_PEER_FIN = 5, /*!< Close due to peer DISCONNECT. */
    QCM_MQTT_CLOSE_BY_PROTOCOL_FAIL = 8,    /*!< Close due to protocol error. */
    QCM_MQTT_CLOSE_BY_DISCONNECT = 9,       /*!< Close due to disconnection. */
    QCM_MQTT_CLOSE_BY_SSL_HS_FAILED = 10,   /*!< Close due to SSL handshake failure. */
    QCM_MQTT_CLOSE_BY_SEND_FAILED = 11,     /*!< Close due to send data to MQTT server failure. */
    QCM_MQTT_CLOSE_BY_PDP_DEACTED = 12,     /*!< Close due to PDP deactivation. */
    QCM_MQTT_CLOSE_BY_OTHER_FAIL = 13,      /*!< Close due to MQTT internal error. */
} qcm_mqtt_client_close_reason_e;

/**
 * @brief MQTT client event notification callback function pointer declaration.
 */
typedef void (*qcm_mqtt_client_event_cb)(qcm_mqtt_client_event_e event_id, void *evt_param, void *user_param);

/**
 * @brief Used to store data content with variable length
 */
typedef struct
{
    char         *data_ptr; /*!< Pointer to the data */
    qosa_uint32_t data_len; /*!< Length of the data */
} qcm_mqtt_data_t;

#ifdef CONFIG_QCM_MQTT5_FUNC
/**
 * @brief User property key-value pair structure
 */
typedef struct
{
    qcm_mqtt_data_t key;   /*!< Store user property -- key */
    qcm_mqtt_data_t value; /*!< Store user property -- value */
} qcm_mqtt_user_property_t;

/**
 * @brief CONNECT properties structure
 */
typedef struct
{
    qosa_uint32_t session_expiry_interval;                          /*!< Session expiry interval */
    qosa_uint16_t receive_maximum;                                  /*!< Receive maximum value */
    qosa_uint16_t topic_alias_maximum;                              /*!< Topic alias maximum value */
    qosa_uint32_t maximum_packet_size;                              /*!< Maximum packet size */
    qosa_uint8_t  request_response_information;                     /*!< Request response information */
    qosa_uint8_t  request_problem_information;                      /*!< Request problem information */
    qcm_mqtt_user_property_t
                 user_property[QCM_MQTT_MAX_USER_PROPERTY_SUPPORT]; /*!< User properties, maximum support QCM_MQTT_MAX_USER_PROPERTY_SUPPORT key-value pairs */
    qosa_uint8_t user_property_cnt;                                 /*!< Number of user properties configured */
} qcm_mqtt_conn_properties_t;
/**
 * @brief Will properties structure
 */
typedef struct
{
    qosa_uint32_t   will_delay_interval;                            /*!< Will delay interval */
    qosa_uint32_t   message_expiry_interval;                        /*!< Message expiry interval */
    qcm_mqtt_data_t content_type;                                   /*!< Content type */
    qcm_mqtt_data_t response_topic;                                 /*!< Response topic */
    qcm_mqtt_data_t correlation_data;                               /*!< Correlation data */
    qosa_uint8_t    payload_format_indicator;                       /*!< Payload format indicator */
    qcm_mqtt_user_property_t
                 user_property[QCM_MQTT_MAX_USER_PROPERTY_SUPPORT]; /*!< User properties, maximum support QCM_MQTT_MAX_USER_PROPERTY_SUPPORT key-value pairs */
    qosa_uint8_t user_property_cnt;                                 /*!< Number of user properties configured */
} qcm_mqtt_will_properties_t;

/*!
 * @brief MQTT connection acknowledgment properties structure
 *
 * This structure is used to store the property information of MQTT CONNECT message,
 * including various connection parameters and configuration options returned by the server.
 * These properties are used to negotiate communication parameters between the client and server
 * to ensure that both sides use compatible settings.
 */
typedef struct
{
    qosa_uint32_t   session_expiry_interval;                        /*!< Session expiry interval */
    qosa_uint16_t   receive_maximum;                                /*!< Receive maximum value */
    qosa_uint8_t    maximum_qos;                                    /*!< Maximum QoS level */
    qosa_uint8_t    retain_available;                               /*!< Retain available */
    qosa_uint32_t   maximum_packet_size;                            /*!< Maximum packet size */
    qosa_uint16_t   topic_alias_maximum;                            /*!< Topic alias maximum value */
    qosa_uint8_t    wildcard_subscription_available;                /*!< Wildcard subscription available */
    qosa_uint8_t    subscription_identifier_available;              /*!< Subscription identifier available */
    qosa_uint8_t    shared_subscription_available;                  /*!< Shared subscription available */
    qosa_uint16_t   server_keep_alive;                              /*!< Server keep alive */
    qcm_mqtt_data_t response_information;                           /*!< Response information */
    qcm_mqtt_user_property_t
                 user_property[QCM_MQTT_MAX_USER_PROPERTY_SUPPORT]; /*!< User properties, maximum support QCM_MQTT_MAX_USER_PROPERTY_SUPPORT key-value pairs */
    qosa_uint8_t user_property_cnt;                                 /*!< Number of user properties configured */
} qcm_mqtt_connack_properties_t;

typedef struct
{
    qcm_mqtt_user_property_t user_property[QCM_MQTT_MAX_USER_PROPERTY_SUPPORT]; /*!< MQTT 5.0 connect properties */
    qosa_uint8_t             user_property_cnt;
} qcm_mqtt_suback_properties_t;

typedef qcm_mqtt_suback_properties_t qcm_mqtt_ack_properties_t;

/**
 * @struct qcm_mqtt_config_t
 * @brief Structure for MQTT client configuration.
 */
typedef struct
{
    qosa_uint32_t subscription_identifier;                 /*!< Subscription identifier */
    qcm_mqtt_user_property_t
        user_property[QCM_MQTT_MAX_USER_PROPERTY_SUPPORT]; /*!< User properties, maximum QCM_MQTT_MAX_USER_PROPERTY_SUPPORT key-value pairs supported */
    qosa_uint8_t user_property_cnt;                        /*!< Number of user property configurations */
} qcm_mqtt_sub_properties_t;

/**
 * @brief MQTT publish message properties structure
 *
 * This structure defines various property information carried by the MQTT client when publishing messages,
 * including message expiration time, topic alias, payload format indicator, and response-related information
 * such as response topic, content type, and correlation data. It also supports additional user-defined properties.
 */
typedef struct
{
    qosa_uint32_t            message_expiry_interval;  /*!< Message expiry interval (in seconds) */
    qosa_uint32_t            subscription_identifier;  /*!< Subscription identifier for associating subscriptions with published messages */
    qosa_uint16_t            topic_alias;              /*!< Topic alias to reduce repeated topic string transmission */
    qosa_uint8_t             payload_format_indicator; /*!< Payload format indicator, 0 means unspecified format, 1 means UTF-8 encoded text */
    qcm_mqtt_data_t          response_topic;           /*!< Response topic for receiver replies */
    qcm_mqtt_data_t          content_type;             /*!< Content type describing the payload data type or format */
    qcm_mqtt_data_t          correlation_data;         /*!< Correlation data for matching requests and responses */
    qcm_mqtt_user_property_t user_property
        [QCM_MQTT_MAX_USER_PROPERTY_SUPPORT];          /*!< User property array, up to QCM_MQTT_MAX_USER_PROPERTY_SUPPORT key-value pairs can be configured */
    qosa_uint8_t user_property_cnt;                    /*!< Current number of user properties set */
} qcm_mqtt_pub_properties_t;

/**
 * @brief MQTT disconnect properties structure
 *
 * This structure is used to store relevant property configurations when the MQTT client disconnects,
 * including session expiry interval and user-defined properties information.
 */
typedef struct
{
    qosa_uint32_t            session_expiry_interval; /*!< Session expiry interval (in seconds), controls how long the server maintains the session */
    qcm_mqtt_user_property_t user_property[QCM_MQTT_MAX_USER_PROPERTY_SUPPORT]; /*!< User property array, allowing additional key-value pair information */
    qosa_uint8_t             user_property_cnt;                                 /*!< Records the current number of user properties set */
} qcm_mqtt_disc_properties_t;

typedef struct
{
    /*!< To not receive our own publications, set to 1.
	 *  0 is the original MQTT behaviour - all messages matching the subscription are received.
	 */
    unsigned char noLocal;
    /*!< To keep the retain flag as on the original publish message, set to 1.
	 *  If 0, defaults to the original MQTT behaviour where the retain flag is only set on
	 *  publications sent by a broker if in response to a subscribe request.
	 */
    unsigned char retainAsPublished;
    /*!< 0 - send retained messages at the time of the subscribe (original MQTT behaviour)
	 *  1 - send retained messages on subscribe only if the subscription is new
	 *  2 - do not send retained messages at all
	 */
    unsigned char retainHandling;
} qcm_mqtt_subscribe_options_t;
#endif /* CONFIG_QCM_MQTT5_FUNC */

/**
 * @struct qcm_mqtt_client_info_t
 * @brief Current MQTT internal running state and information
 */
typedef struct
{
    int              client_id;                              /*!< Corresponding client's client id */
    qosa_uint16_t    svr_port;                               /*!< Corresponding server remote port */
    char             srv_host[CONFIG_QOSA_HOST_URL_MAX_LEN]; /*!< Corresponding server address string */
    qcm_mqtt_state_e client_state;                           /*!< Corresponding internal connection state */
} qcm_mqtt_client_info_t;

/**
 * @struct qcm_mqtt_config_t。
 * @brief MQTT client configuration structure.
 */
typedef struct
{
    qcm_mqtt_support_version_e    version;             /*!< MQTT client connection version number (QCM_MQTT_VERSION_V3/QCM_MQTT_VERSION_V4) */
    qosa_uint8_t                  pdp_cid;             /*!< MQTT client used PDP (bearer) number */
    qosa_uint8_t                  sim_id;              /*!< MQTT client used SIM card (0/1) */
    qosa_uint32_t                 kalive_time;         /*!< MQTT client keep-alive interval configuration */
    qosa_uint32_t                 delivery_time;       /*!< MQTT client retransmission interval (valid when QOS is 1/2) */
    qosa_uint8_t                  delivery_cnt;        /*!< MQTT client retransmission count (valid when QOS is 1/2) */
    qosa_uint8_t                  will_flag;           /*!< MQTT client will message enable flag */
    qcm_mqtt_quality_of_service_e will_qos;            /*!< MQTT client will message quality of service */
    qcm_mqtt_data_t               will_topic;          /*!< MQTT client will topic name */
    qcm_mqtt_data_t               will_message;        /*!< MQTT client will message */
    qosa_uint32_t                 connect_time;        /*!< PDP activation maximum timeout, in seconds (S) */
    qosa_uint32_t                 close_ping_interval; /*!< Maximum delay close time after ping timeout */
    qosa_uint32_t                 rec_buf_size;        /*!< MQTT client socket read buffer length */
    qosa_uint8_t                  max_recv_store_cnt;  /*!< Maximum number of PUB messages cached from server */
    qosa_bool_t                   clean_session;       /*!< Clear session. (0: try to reuse previous session; 1: reconnect each time, no history retained)*/
    qosa_bool_t                   will_retain;  /*!< Enable will message (0: not retained, only online can receive; 1: server will retain will message) */
#ifdef CONFIG_QCM_VTLS_FUNC
    qosa_bool_t       ssl_enable;               /*!< MQTT client SSL/TLS enable flag */
    qosa_uint8_t      ssl_ctx_id;               /*!< User configured SSL/TLS context ID */
    qcm_ssl_config_t *ssl_config;               /*!< User SSL/TLS config configuration */
#endif                                          /* CONFIG_QCM_VTLS_FUNC */
#ifdef CONFIG_QCM_MQTT5_FUNC
    qcm_mqtt_conn_properties_t conn_properties; /*!< Configure MQTT5.0 CONNECT properties */
    qcm_mqtt_will_properties_t will_properties; /*!< Configure MQTT5.0 Will properties */
#endif                                          /* CONFIG_QCM_MQTT5_FUNC */
} qcm_mqtt_config_t;
typedef struct
{
    qcm_mqtt_data_t topic;             /*!< Topic that the client subscribes to */
    qosa_uint8_t    qos;               /*!< QoS level for client's published messages */
#ifdef CONFIG_QCM_MQTT5_FUNC
    qcm_mqtt_subscribe_options_t opts; /*!< MQTT 5.0 subscription options */
#endif                                 /* CONFIG_QCM_MQTT5_FUNC */
} qcm_mqtt_sub_topic_t;

/**
 * @struct qcm_mqtt_config_t
 * @brief Structure for MQTT client configuration.
 */
typedef struct
{
    int                   msg_id;         /*!< Message identifier */
    qcm_mqtt_sub_topic_t *topics;         /*!< Collection of subscription topic information */
    qosa_uint8_t          topic_cnt;      /*!< Number of subscribed topics */
#ifdef CONFIG_QCM_MQTT5_FUNC
    qcm_mqtt_sub_properties_t properties; /*!< SUBSCRIBE properties configuration */
#endif                                    /*!< CONFIG_QCM_MQTT5_FUNC */
} qcm_mqtt_sub_config_t;

typedef struct
{
    int                           msg_id;  /*!< MQTT message id, 0 if QOS=0. */
    qcm_mqtt_quality_of_service_e qos;     /*!< Quality of Service (QOS) for MQTT publish. */
    qosa_bool_t                   retain;  /*!< Retain flag for MQTT publish. */
    qcm_mqtt_data_t               topic;   /*!< Topic name for MQTT publish. */
    qcm_mqtt_data_t               payload; /*!< Message content for MQTT publish. */
#ifdef CONFIG_QCM_MQTT5_FUNC
    qcm_mqtt_pub_properties_t properties;  /*!< MQTT 5.0 publish properties */
#endif                                     /*!< CONFIG_QCM_MQTT5_FUNC */
} qcm_mqtt_pub_config_t;

/**
 * @brief MQTT disconnect configuration structure
 *
 * This structure is used to configure parameters when the MQTT client disconnects.
 * Depending on the definition of CONFIG_QCM_MQTT5_FUNC macro, the structure may include
 * MQTT5.0 disconnect properties configuration.
 *
 * @note When CONFIG_QCM_MQTT5_FUNC macro is defined, the structure includes properties member for MQTT5.0 functionality
 * @note When CONFIG_QCM_MQTT5_FUNC macro is not defined, this structure is empty
 */
typedef struct
{
#ifdef CONFIG_QCM_MQTT5_FUNC
    qcm_mqtt_disc_properties_t properties; /*!< MQTT5.0 disconnect properties configuration */
#endif                                     /* CONFIG_QCM_MQTT5_FUNC */
} qcm_mqtt_disc_config_t;

/**
 * @brief Structure for MQTT received message
 *
 * This structure represents a published message received from the MQTT server,
 * containing the message ID, topic, payload data, and optional MQTT 5.0
 * property information.
 *
 */
typedef struct
{
    qosa_uint16_t   msg_id;                /*!< MQTT message id */
    qcm_mqtt_data_t topic;                 /*!< topic string */
    qcm_mqtt_data_t payload;               /*!< binary payload, length delimited */
#ifdef CONFIG_QCM_MQTT5_FUNC
    qcm_mqtt_pub_properties_t *properties; /*!< MQTT 5.0 disconnect properties */
#endif                                     /*!< CONFIG_QCM_MQTT5_FUNC */
} qcm_mqtt_recv_pub_t;
/******************************* MQTT Event Reporting start *******************************/
/**
 * @brief MQTT common response status notification.
 */
typedef struct
{
    qcm_mqtt_support_version_e version;       /*!< MQTT client connection version (QCM_MQTT_VERSION_V3/QCM_MQTT_VERSION_V4/QCM_MQTT_VERSION_V5). */
    qosa_uint8_t               client_id;     /*!< Corresponding client handle */
    qcm_mqtt_eercode_e         result;        /*!< Command execution result status return */
    qosa_uint8_t               pocotrol_code; /*!< Protocol return code */
    qosa_uint16_t              msg_id;        /*!< MQTT message id */
    void                      *data;
} qcm_mqtt_common_resp_t;

/**
 * @brief MQTT subscribe topic response status notification
 */
typedef struct
{
    qosa_uint8_t *qoss;    /*!< list of granted QoSs */
    int           qos_cnt; /*!< Number of responded QoSs */
#ifdef CONFIG_QCM_MQTT5_FUNC
    qcm_mqtt_suback_properties_t *options;
#endif /* CONFIG_QCM_MQTT5_FUNC */
} qcm_mqtt_suback_resp_t;

/**
 * @brief MQTT subscribe topic status notification
 */
typedef struct
{
    qosa_uint16_t               msg_id;      /*!< MQTT message id */
    qosa_uint8_t                retry_count; /*!< Retransmission count if retransmission occurs */
    qcm_mqtt_send_result_type_e send_result; /*!< Indicates the result of the current report */
} qcm_mqtt_send_resp_t;

/**
 * @brief MQTT publish response status notification.
 */
typedef struct
{
    qosa_uint8_t client_id; /*!< Corresponding client handle */
    qosa_uint8_t store_id;  /*!< MQTT message id */
} qcm_mqtt_new_msg_notify_t;

/**
 * @brief MQTT close reason status notification.
 */
typedef struct
{
    qcm_mqtt_client_close_reason_e close_cause; /*!< MQTT client close reason. */
} qcm_mqtt_close_cause_t;

/******************************* MQTT Event Reporting End *******************************/
/**
 * @brief Free data in MQTT data structure
 *
 * @param data_ptr Pointer to MQTT data structure
 */
void qcm_mqtt_free_data(qcm_mqtt_data_t *data_ptr);

/**
 * @brief Allocate memory for MQTT data structure and copy data
 *
 * @param data_ptr Pointer to MQTT data structure
 * @param data_len Data length
 * @param data Pointer to source data
 * @return qcm_mqtt_eercode_e Error code, returns QCM_MQTT_ERRCODE_OK on success
 */
qcm_mqtt_eercode_e qcm_mqtt_malloc_data(qcm_mqtt_data_t *data_ptr, qosa_uint32_t data_len, char *data);

#ifdef CONFIG_QCM_MQTT5_FUNC
/**
 * @brief Free MQTT user property data
 *
 * @param user_property_cnt Number of user properties
 * @param user_prop Pointer to user property array
 * @return qcm_mqtt_eercode_e Error code, returns QCM_MQTT_ERRCODE_OK on success
 */
qcm_mqtt_eercode_e qcm_mqtt_user_property_free_data(qosa_uint8_t user_property_cnt, qcm_mqtt_user_property_t *user_prop);
#endif /* CONFIG_QCM_MQTT5_FUNC */

/**
 * @brief User actively calls to load MQTT default attribute configuration
 *
 * @param[out] qcm_mqtt_config_t * config_ptr
 *           - Output default mqtt config configuration items
 *
 * @return qcm_mqtt_eercode_e
 *        - Return MQTT_RES_OK for success.
 *        - Return other values in mqtt_eercode_e for failure.
 */
qcm_mqtt_eercode_e qcm_mqtt_client_default_config(qcm_mqtt_config_t *config_ptr);

/**
 * @brief Request MQTT client handle, only used for OPEN client group, STD directly uses the corresponding clientID from the upper layer for management.
 *
 * @return qosa_uint8_t
 *        - Return value greater than 0 indicates successful MQTT client handle request.
 *        - Return value equal to 0 indicates failed MQTT client handle request.
 */
qosa_uint8_t qcm_mqtt_client_create(void);

/**
 * @brief Set MQTT corresponding client identifier for internal resource initialization
 *
 * @param[in] qosa_uint8_t client_id
 *           - MQTT client handle.
 *
 * @param[in] qcm_mqtt_config_t *config_ptr
 *           - Pointer to MQTT configuration parameter structure.
 *
 * @param[in] qcm_mqtt_client_event_cb evt_cb
 *           - Event notification handling function when MQTT client receives events.
 *
 * @param[in] void * user_param
 *           - User information pointer, used in callback.
 *
 * @return qcm_mqtt_eercode_e
 *        - Return MQTT_RES_OK for success.
 *        - Return other values in mqtt_eercode_e for failure.
 */
qcm_mqtt_eercode_e qcm_mqtt_client_init(qosa_uint8_t client_id, qcm_mqtt_config_t *config_ptr, qcm_mqtt_client_event_cb evt_cb, void *user_param);
/**
 * @brief Used for MQTT TCP connection request.
 *
 * @param[in] qosa_uint8_t client_id
 *           - MQTT client handle.
 * @param[in] char *addr
 *           - MQTT server address, can be domain name or IP address.
 * @param[in] qosa_uint16_t port
 *           - MQTT server port number.
 *
 * @return qcm_mqtt_eercode_e
 *        - Returns MQTT_RES_OK for success.
 *        - Returns other values in mqtt_eercode_e for failure.
 */
qcm_mqtt_eercode_e qcm_mqtt_client_open(qosa_uint8_t client_id, char *addr, qosa_uint16_t port);
/**
 * @brief After MQTT TCP connection is successful, user actively calls API to send MQTT protocol connect request
 *
 * @param[in] qosa_uint8_t client_id
 *          - MQTT client handle.
 * @param[in] const char * client_name
 *          - Pointer to MQTT client identifier string.
 *
 * @param[in] int client_name_len
 *          - Corresponding client identifier string length
 *
 * @param[in] const char * username
 *          - Pointer to MQTT client username string.
 *
 * @param[in] int username_len
 *          - Corresponding username string length
 *
 * @param[in] const char * password
 *          - Pointer to MQTT client password string.
 *
 * @param[in] int password_len
 *          - Corresponding password length
 *
 * @return qcm_mqtt_eercode_e
 *        - Return MQTT_RES_OK for success.
 *        - Return other values in mqtt_eercode_e for failure.
 */
qcm_mqtt_eercode_e qcm_mqtt_client_connect(
    qosa_uint8_t client_id,
    const char  *client_name,
    int          client_name_len,
    const char  *username,
    int          username_len,
    const char  *password,
    int          password_len
);

#ifdef CONFIG_QCM_MQTT5_FUNC
/**
 * @brief Check if MQTT topic alias is valid
 *
 * This function is used to verify whether the specified client's MQTT topic alias meets the specification requirements
 *
 * @param client_id Client identifier, used to identify a specific MQTT client
 * @param topic_alias Topic alias, the alias value to be validated
 *
 * @return qcm_mqtt_eercode_e Returns error code enumeration value
 *         - QCM_MQTT_ERRCODE_SUCCESS: Check successful, alias is valid
 *         - QCM_MQTT_ERRCODE_INVALID_ALIAS: Alias is invalid
 *         - Other error codes: Other errors occurred during the check
 */
qcm_mqtt_eercode_e qcm_mqtt_topic_alias_check(qosa_uint8_t client_id, qosa_uint16_t topic_alias);

/**
 *  @brief Get information from the connect ack sent by the server
 *
 * @param[in] qosa_uint8_t client_id
 *           - MQTT client handle.
 * @param[out] qcm_mqtt_connack_properties_t * conn_props
 *           - Property information in the connack reply from the server.
 * @return qcm_mqtt_eercode_e
 *        - Returns MQTT_RES_OK for success.
 *        - Returns other values in mqtt_eercode_e for failure.
 */
qcm_mqtt_eercode_e qcm_mqtt_client_get_server_info(qosa_uint8_t client_id, qcm_mqtt_connack_properties_t *conn_props);

/**
 *  @brief Release the connect ack information obtained from the underlying layer.
 *
 * @param[out] qcm_mqtt_connack_properties_t * conn_props
 *           - Property information in the connack reply from the server.
 * @return qcm_mqtt_eercode_e
 *        - Returns MQTT_RES_OK for success.
 *        - Returns other values in mqtt_eercode_e for failure.
 */
qcm_mqtt_eercode_e qcm_mqtt_release_connack_props(qcm_mqtt_connack_properties_t *conn_props);

/**
 * @brief Release MQTT new message publish property resources
 *
 * This function is used to release the dynamically allocated memory resources in the MQTT publish message property structure,
 * including optional property fields such as topic alias, response topic, correlation data, and user properties.
 *
 * @param pub_props Pointer to the MQTT publish message property structure
 *
 * @return qcm_mqtt_eercode_e Error code
 *         - QCM_MQTT_EERCODE_OK: Execution successful
 *         - Other values: Execution failed
 */
qcm_mqtt_eercode_e qcm_mqtt_release_newmsg_props(qcm_mqtt_pub_properties_t *pub_props);
#endif /* CONFIG_QCM_MQTT5_FUNC */

/**
 * @brief MQTT client subscribe to topic request.
 *
 * @param[in] qosa_uint8_t client_id
 *           - MQTT client handle.
 * @param[in] qcm_mqtt_sub_config_t * sub_info
 *          - Pointer to the subscription topic information structure, containing topic list and topic count information.
 *
 * @return qcm_mqtt_eercode_e
 *        - Returns MQTT_RES_OK for success.
 *        - Returns other values in mqtt_eercode_e for failure.
 */
qcm_mqtt_eercode_e qcm_mqtt_client_subscribe(qosa_uint8_t client_id, qcm_mqtt_sub_config_t *sub_info);

/**
 * @brief MQTT client unsubscribe topic request.
 *
 * @param[in] qosa_uint8_t client_id
 *           - MQTT client handle.
 * @param[in] qcm_mqtt_sub_config_t * sub_info
 *          - Pointer to the subscription topic information structure, containing topic list and topic count information.
 *
 * @return qcm_mqtt_eercode_e
 *       - Returns MQTT_RES_OK for success.
 *       - Returns other values in mqtt_eercode_e for failure.
 */
qcm_mqtt_eercode_e qcm_mqtt_client_unsubscribe(qosa_uint8_t client_id, qcm_mqtt_sub_config_t *sub_info);

/**
 * @brief MQTT client actively publish business message.
 *
 * @param[in] qosa_uint8_t client_id
 *           - MQTT client handle.
 * @param[in] qcm_mqtt_pub_config_t * pub_info
 *           - MQTT PUB configuration structure.
 *
 * @return qcm_mqtt_eercode_e
 *        - Returns MQTT_RES_OK for success.
 *        - Returns other values in mqtt_eercode_e for failure.
 */
qcm_mqtt_eercode_e qcm_mqtt_client_publish(qosa_uint8_t client_id, qcm_mqtt_pub_config_t *pub_info);

/**
 * @brief Disconnect MQTT client connection
 *
 * This function is used to disconnect the connection of the specified MQTT client,
 * and optionally send disconnection configuration information
 *
 * @param[in] client_id MQTT client ID to disconnect
 * @param[in] disconn_info Pointer to disconnection configuration information,
 *            containing parameter configurations for disconnection
 *
 * @return qcm_mqtt_eercode_e Returns operation result status code
 *         - Returns QCM_MQTT_SUCCESS on success
 *         - Returns corresponding error code on failure
 */
qcm_mqtt_eercode_e qcm_mqtt_client_disconnect(qosa_uint8_t client_id, qcm_mqtt_disc_config_t *disconn_info);
/**
 * @brief Close TCP connection between MQTT client and server.
 *
 * @param[in] qosa_uint8_t client_id
 *           - MQTT client handle.
 *
 * @return qcm_mqtt_eercode_e
 *        - Returns MQTT_RES_OK for success.
 *        - Returns other values from mqtt_eercode_e for failure.
 */
qcm_mqtt_eercode_e qcm_mqtt_client_close(qosa_uint8_t client_id);

/**
 * @brief Get the count of pending messages to read for the specified MQTT client
 *
 * @param client_id Client ID
 * @return qosa_uint32_t Returns the number of messages pending to be read
 */
qosa_uint32_t qcm_mqtt_client_wait_read_cnt(qosa_uint8_t client_id);

/**
 * @brief Read subscription messages from the specified MQTT client
 *
 * @param client_id Client ID
 * @param store_id Storage ID
 * @param recv_pub Pointer to the received publish message structure
 * @return qcm_mqtt_eercode_e Returns operation result error code
 */
qcm_mqtt_eercode_e qcm_mqtt_client_read_subcribe_message(qosa_uint8_t client_id, qosa_uint8_t store_id, qcm_mqtt_recv_pub_t *recv_pub);

/**
 * @brief Read cached data from the specified MQTT client
 *
 * @param client_id Client ID
 * @param recv_pub Pointer to the received publish message structure
 * @return qcm_mqtt_eercode_e Returns operation result error code
 */
qcm_mqtt_eercode_e qcm_mqtt_client_read_cacha_data(qosa_uint8_t client_id, qcm_mqtt_recv_pub_t *recv_pub);

/**
 * @brief Get corresponding client internal state information
 *
 * @param[in] qosa_uint8_t client_id
 *          - Corresponding client information to query
 * @param[out] qcm_mqtt_client_info_t * client_info
 *          - Return corresponding client internal state information
 *
 * @return qcm_mqtt_eercode_e
 *        - Return MQTT_RES_OK for success.
 *        - Return other values in mqtt_eercode_e for failure.
 */
qcm_mqtt_eercode_e qcm_mqtt_client_get_state_info(qosa_uint8_t client_id, qcm_mqtt_client_info_t *client_info);

/**
 * @brief Get MQTT last close reason for each client
 *
 * @param[in] int client_id
 *          - MQTT corresponding client id
 */
int qcm_mqtt_get_last_close_event(int client_id);

/**
 * @brief Initialize MQTT specified client close reason
 *
 * @param[in] int client_id
 *          - MQTT corresponding client id
 */
void qcm_mqtt_last_close_event_init(int client_id);

/**
 * @brief Write data to persistent storage
 *
 * @param[in] nv_path  Storage path
 * @param[in] data     Data to write
 * @param[in] data_len Data length in bytes
 *
 * @return qcm_mqtt_eercode_e
 *        - Return QCM_MQTT_RES_OK for success.
 *        - Return other values in mqtt_eercode_e for failure.
 */
qcm_mqtt_eercode_e qcm_mqtt_nv_write(char *nv_path, void *data, qosa_uint16_t data_len);

/**
 * @brief Read data from persistent storage
 *
 * @param[in]  nv_path  Storage path/identifier
 * @param[out] data     Buffer to store read data
 * @param[in]  data_len Buffer size in bytes
 *
 * @return qcm_mqtt_eercode_e
 *        - Return QCM_MQTT_RES_OK for success.
 *        - Return other values in mqtt_eercode_e for failure.
 */
qcm_mqtt_eercode_e qcm_mqtt_nv_read(char *nv_path, void *data, qosa_uint16_t data_len);

#endif /* __QCM_MQTT_H__ */
