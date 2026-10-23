/*******************************************************************************
 *
 * THIRD-PARTY NOTICE
 *
 * Portions of this file are derived from Eclipse Paho MQTT Embedded C
 * (MQTTPacket code line, package version 1.1.0).
 *
 * Original work:
 *   Copyright (c) 2014 IBM Corp.
 *
 * The Paho-derived portions are redistributed by Quectel under the
 * Eclipse Distribution License v1.0.
 *
 * See THIRD_PARTY_NOTICES.md and licenses/ for details.
 *
 ******************************************************************************/
/*****************************************************************/ /**
* @file qcm_mqtt_private.h
* @brief
* @author larson.li@quectel.com
* @date 2023-12-07
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2023-12-07 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#ifndef __QCM_MQTT_PRIVATE_H__
#define __QCM_MQTT_PRIVATE_H__

#include "qosa_def.h"
#include "qosa_queue_list.h"
#include "qosa_sys.h"
#include "qosa_asyn_dns.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#include "qosa_datacall.h"
#else
#include "qosa_ip_addr.h"
#endif
#include "qcm_mqtt_config.h"
#include "qcm_mqtt.h"
#include "qosa_log.h"

/** Timer advance time, reaching N ms earlier, because there is a time difference in task flow.*/
#define QCM_MQTT_TIMEOUT_BEFORE 100  //ms

/** Priority of the MQTT service layer task.*/
#define QCM_MQTT_TASK_PRIO      QOSA_PRIORITY_NORMAL

/**
 * @enum qcm_mqtt_subscribe_e.
 * @brief Pattern classification for MQTT subscription topics, used to distinguish between subscribing to topics and unsubscribing.
 */
typedef enum
{
    QCM_MQTT_SUBSCRIBE_MODE = 0x01,   /*MQTT subscription topic pattern.*/
    QCM_MQTT_UNSUBSCRIBE_MODE = 0x02, /*MQTT unsubscribe mode.*/
} qcm_mqtt_subscribe_e;

/**
 * @brief The types of message events to be handled by the MQTT task
 */
typedef enum
{
    QCM_MQTT_MIN = 0,
    QCM_MQTT_SOCKET_EVENT = 1,        /*!< TCPIP underlying event status notification*/
    QCM_MQTT_CYCLIC_TIME_EVENT = 2,   /*!< MQTT client loop timer timeout notification*/
    QCM_MQTT_OPEN_EVNET = 3,          /*!< MQTT open operation EVENT*/
    QCM_MQTT_SSL_SHAKEHAND_EVNET = 4, /*!< MQTT SSL handshake EVENT*/
    QCM_MQTT_DATACALL_RESP_EVENT = 5, /*!< MQTT datacall status reply event*/
    QCM_MQTT_DNS_REQUEST_EVENT = 6,   /*!< MQTT request DNS operation*/
    QCM_MQTT_DNS_RESPONSE_EVENT = 7,  /*!< MQTT DNS request status reply*/
    QCM_MQTT_SOCKET_CREATE_EVENT = 8, /*!< MQTT socket starts connecting*/
    QCM_MQTT_CONNECT_EVENT = 9,       /*!< MQTT protocol connection request*/
    QCM_MQTT_SUB_EVENT = 10,          /*!< MQTT Subscribe/Unsubscribe Request*/
    QCM_MQTT_PUB_EVENT = 11,          /*!< MQTT Publish request*/
    QCM_MQTT_DISC_EVENT = 12,         /*!< MQTT Disconnect send request*/
    QCM_MQTT_CLOSE_EVENT = 13,        /*!< MQTT resource release request*/
    QCM_MQTT_SEND_EVENT = 14,         /*!< MQTT service-side own data transmission*/
    QCM_MQTT_RECV_EVENT = 15,         /*!< MQTT service side self-request to receive data*/
} qcm_mqtt_event_id_e;

typedef enum
{
    QCM_MQTT_STORE_PUB_MSG_LOW = 0, /*!< publish message storage low status*/
    QCM_MQTT_STORE_PUB_MSG_HIGH,    /*!< publish message storage HIGH state*/
} qcm_mqtt_store_level_e;

#ifdef CONFIG_QCM_MQTT5_FUNC

/**
 * @enum qcm_mqtt_property_code_e.
 * @brief Enumeration of attribute fields in MQTT 5.0, indicating the corresponding attribute configuration in the attribute structure array.
 */
typedef enum
{
    QCM_MQTT_PROPERTY_CODE_PAYLOAD_FORMAT_INDICATOR = 1,            /**< The value is 1 */
    QCM_MQTT_PROPERTY_CODE_MESSAGE_EXPIRY_INTERVAL = 2,             /**< The value is 2 */
    QCM_MQTT_PROPERTY_CODE_CONTENT_TYPE = 3,                        /**< The value is 3 */
    QCM_MQTT_PROPERTY_CODE_RESPONSE_TOPIC = 8,                      /**< The value is 8 */
    QCM_MQTT_PROPERTY_CODE_CORRELATION_DATA = 9,                    /**< The value is 9 */
    QCM_MQTT_PROPERTY_CODE_SUBSCRIPTION_IDENTIFIER = 11,            /**< The value is 11 */
    QCM_MQTT_PROPERTY_CODE_SESSION_EXPIRY_INTERVAL = 17,            /**< The value is 17 */
    QCM_MQTT_PROPERTY_CODE_ASSIGNED_CLIENT_IDENTIFIER = 18,         /**< The value is 18 */
    QCM_MQTT_PROPERTY_CODE_ASSIGNED_CLIENT_IDENTIFER = 18,          /**< The value is 18 (obsolete, misspelled) */
    QCM_MQTT_PROPERTY_CODE_SERVER_KEEP_ALIVE = 19,                  /**< The value is 19 */
    QCM_MQTT_PROPERTY_CODE_AUTHENTICATION_METHOD = 21,              /**< The value is 21 */
    QCM_MQTT_PROPERTY_CODE_AUTHENTICATION_DATA = 22,                /**< The value is 22 */
    QCM_MQTT_PROPERTY_CODE_REQUEST_PROBLEM_INFORMATION = 23,        /**< The value is 23 */
    QCM_MQTT_PROPERTY_CODE_WILL_DELAY_INTERVAL = 24,                /**< The value is 24 */
    QCM_MQTT_PROPERTY_CODE_REQUEST_RESPONSE_INFORMATION = 25,       /**< The value is 25 */
    QCM_MQTT_PROPERTY_CODE_RESPONSE_INFORMATION = 26,               /**< The value is 26 */
    QCM_MQTT_PROPERTY_CODE_SERVER_REFERENCE = 28,                   /**< The value is 28 */
    QCM_MQTT_PROPERTY_CODE_REASON_STRING = 31,                      /**< The value is 31 */
    QCM_MQTT_PROPERTY_CODE_RECEIVE_MAXIMUM = 33,                    /**< The value is 33*/
    QCM_MQTT_PROPERTY_CODE_TOPIC_ALIAS_MAXIMUM = 34,                /**< The value is 34 */
    QCM_MQTT_PROPERTY_CODE_TOPIC_ALIAS = 35,                        /**< The value is 35 */
    QCM_MQTT_PROPERTY_CODE_MAXIMUM_QOS = 36,                        /**< The value is 36 */
    QCM_MQTT_PROPERTY_CODE_RETAIN_AVAILABLE = 37,                   /**< The value is 37 */
    QCM_MQTT_PROPERTY_CODE_USER_PROPERTY = 38,                      /**< The value is 38 */
    QCM_MQTT_PROPERTY_CODE_MAXIMUM_PACKET_SIZE = 39,                /**< The value is 39 */
    QCM_MQTT_PROPERTY_CODE_WILDCARD_SUBSCRIPTION_AVAILABLE = 40,    /**< The value is 40 */
    QCM_MQTT_PROPERTY_CODE_SUBSCRIPTION_IDENTIFIERS_AVAILABLE = 41, /**< The value is 41 */
    QCM_MQTT_PROPERTY_CODE_SHARED_SUBSCRIPTION_AVAILABLE = 42       /**< The value is 241 */
} qcm_mqtt_property_code_e;

/**
 * @struct qcm_mqtt_property_t.
 * @brief Structure of attribute fields in MQTT 5.0.
 */
typedef struct
{
    qcm_mqtt_property_code_e identifier; /*!< ID of the MQTT 5.0 property field.*/

    union
    {
        qosa_uint8_t  byte;     /**< holds the value of a byte property type */
        qosa_uint16_t integer2; /**< holds the value of a 2 byte integer property type */
        qosa_uint32_t integer4; /**< holds the value of a 4 byte integer property type */
        struct
        {
            qcm_mqtt_data_t data;  /**< The value of a string property, or the name of a user property. */
            qcm_mqtt_data_t value; /**< The value of a user property. */
        };
    } value;
} qcm_mqtt_property_t;

/**
 * @struct qcm_mqtt_properties_t.
 * @brief Record all attribute fields of this sending command in MQTT 5.0.
 */
typedef struct
{
    qosa_uint32_t        count;     /*!< Count the number of currently stored attributes.*/
    qosa_uint32_t        max_count; /*!< The maximum number of attributes that the structure can support.*/
    qosa_uint32_t        length;    /*!< Count the total byte length occupied by all attributes.*/
    qcm_mqtt_property_t *array;     /*!< Pointer to the structure array where attributes are stored.*/
} qcm_mqtt_properties_t;

typedef struct
{
    qosa_q_link_type_t link;
    qcm_mqtt_data_t    topic;
    qosa_uint16_t      topic_alias;
} qcm_mqtt_topic_alias_t;
#endif /* CONFIG_QCM_MQTT5_FUNC */

/**
 * @struct qcm_mqtt_packet_buffer
 * @brief Fragmented packet storage information
 */
typedef struct
{
    qosa_uint32_t   buf_len; /*!< Corresponds to the total length of data in the current packet*/
    qcm_mqtt_data_t buf_ptr; /*!< Corresponding encoded data content to be sent*/
    qosa_uint32_t   buf_pos; /*!< Offset of already sent length*/
} qcm_mqtt_packet_buffer;

/**
 * @brief MQTT Message Queue Notification Event
 */
typedef struct
{
    qcm_mqtt_event_id_e cmd;
    void               *argv;
} qcm_mqtt_event_info_t;

/**
 * @brief Corresponding to the newly received message content of MQTT
 */
typedef struct
{
    int             msg_id;
    qcm_mqtt_data_t topic;
    qcm_mqtt_data_t payload;
} qcm_mqtt_client_publish_ind_t;

/**
 * @brief mqtt receive publish message storage
 */
typedef struct
{
    qosa_uint8_t                  mqtt_store_cnt;         /*!< MQTT cache count*/
    qosa_uint8_t                  mqtt_payload_total_len; /*!< Total cached payload length for MQTT*/
    qcm_mqtt_client_publish_ind_t store[QCM_MQTT_MAX_STORE_MSG_CNT];
} qcm_mqtt_puhblish_store_e;

/**
 * @strcut qcm_tcpapp_pdp_resp_t
 * @brief PDP activation response content
 */
typedef struct
{
    qosa_uint8_t            client_id; /*!< Client ID requested during PDP activation*/
    qosa_dns_error_e        status;    /*!< Corresponds to the DNS resolution operation return status*/
    struct qosa_addrinfo_s *info;      /*!< Corresponding DNS resolution content*/
} qcm_mqtt_dns_resp_t;

typedef struct
{
    int           varible_header_len;
    qosa_uint32_t remain_len;
} mqtt_varible_header;

typedef union
{
    unsigned char byte; /**< the whole byte */
    struct
    {
        unsigned char retain : 1; /**< retained flag bit */
        unsigned char qos    : 2; /**< QoS value, 0, 1 or 2 */
        unsigned char dup    : 1; /**< DUP flag bit */
        unsigned char type   : 4; /**< message type nibble */
    } bits;
} qcm_mqtt_header;

typedef struct
{
    qcm_mqtt_header     header;
    mqtt_varible_header varible_header;
    qosa_uint32_t       buf_len;
    qosa_uint8_t        buff[QCM_MQTT_MAX_READ_BUFF_LEN];
    qosa_uint32_t       buf_pos;
    char               *input_buff; /*!< Used to record the header information of a PUB packet when the received PUB packet is too large.*/
} qcm_mqtt_input_info_t;

/**
 * @strcut qcm_tcpapp_pdp_resp_t
 * @brief PDP activation response content
 */
typedef struct
{
    qosa_uint8_t            client_id; /*!< Client ID requested during PDP activation*/
    qosa_datacall_act_cnf_t act_info;  /*!< PDP activation operation response content*/
} qcm_mqtt_pdp_resp_t;

/**
 * @struct qcm_mqtt_soc_event_t parameter passing
 * @brief select report information
 */
typedef struct
{
    qosa_int32_t sock_hd;    /*!< Number of socket channels, index value, not socketfd*/
    qosa_int32_t event_mask; /*!< Mask time type*/
    qosa_int32_t code;       /*!< Execution result/error code*/
    void        *user_data;  /*!< Pointer to upper-layer application parameters*/
} qcm_mqtt_soc_event_t;

/**
 * @brief Information about the MQTT client
 */
typedef struct
{
    qosa_uint8_t                   client_id;
    qcm_mqtt_config_t              user_option;      /*!< Configuration items carried by the user during initialization*/
    qcm_mqtt_puhblish_store_e      publish_store_q;  /*!< Received publish storage*/
    qosa_q_type_t                  out_packet_queue; /*!< Message queue to be sent*/
    qosa_q_type_t                  pend_req_queue;   /*!< Message queue for sent messages without ACK, used for timer operations*/
    qcm_mqtt_input_info_t          cur_input_packet; /*!< A packet of MQTT message data received*/
    qcm_mqtt_state_e               client_state;
    int                            socketfd;
    qosa_uint16_t                  server_port;
    qosa_ip_addr_t                 server_ip;
    char                           server_addr[CONFIG_QOSA_HOST_URL_MAX_LEN + 1];
    qosa_uint8_t                   ai_family;
    qosa_uint16_t                  packet_id_seq;
    qosa_timer_t                   timer;          /*!< Set a cyclic timer*/
    qosa_time_t                    send_last_time; /*!< Last message sending event for the current client ID*/
    qcm_mqtt_client_event_cb       event_cb;
    void                          *user_argv;
    qcm_mqtt_store_level_e         sotre_level;
    qosa_q_type_t                  new_msg_pub;    /*!< Used to store confirmed publish messages*/
    qosa_q_type_t                  inboundMsgs;    /*!< QOS=2 cached publish message*/
    qcm_mqtt_client_close_reason_e close_cause;
    int                            read_event_cnt; /*!< Counter value for triggering the MQTT read event*/
#ifdef CONFIG_QCM_MQTT5_FUNC
    qosa_q_type_t                  topic_alias_list;
    qcm_mqtt_connack_properties_t *connack_props; /**< MQTT 5.0 properties */
#endif                                            /* CONFIG_QCM_MQTT5_FUNC */
} qcm_mqtt_client_t;

typedef struct
{
    qosa_q_link_type_t link;
    qcm_mqtt_data_t    topic;
    qosa_uint8_t       qos;
#ifdef CONFIG_QCM_MQTT5_FUNC
    qcm_mqtt_subscribe_options_t opts; /*!< MQTT 5.0 Subscription Options*/
#endif                                 /* CONFIG_QCM_MQTT5_FUNC */
} qcm_mqtt_list_topic_t;

/**
 * @brief Used to pass the address and port information for configuring the MQTT TCP connection.
 */
typedef struct
{
    qosa_uint8_t  client_id;
    char          addr[CONFIG_QOSA_HOST_URL_MAX_LEN];
    qosa_uint16_t port;
} qcm_mqtt_open_param_t;

/**
 * @brief Used to initiate a protocol connection action request after the MQTT TCP connection is successfully established.
 */
typedef struct
{
    qosa_uint8_t    client_id;
    qcm_mqtt_data_t client_name;
    qcm_mqtt_data_t username;
    qcm_mqtt_data_t password;
} qcm_mqtt_conn_param_t;

typedef struct
{
    qosa_q_link_type_t link;
    qcm_mqtt_data_t    topic;
} qcm_mqtt_unsub_topic_t;

typedef struct
{
    qosa_uint8_t         client_id;
    qosa_uint32_t        packet_len;
    qcm_mqtt_subscribe_e mode;
    int                  msg_id; /*!< MQTT message id, if QOS=0 then it is 0.*/
    qosa_q_type_t        topic_list;
#ifdef CONFIG_QCM_MQTT5_FUNC
    qcm_mqtt_properties_t props; /**< MQTT 5.0 properties */
#endif                           /* CONFIG_QCM_MQTT5_FUNC */
} qcm_mqtt_sub_param_t;

typedef struct
{
    qosa_uint8_t                  client_id;
    int                           msg_id;  /*!< MQTT message id, if QOS=0 then it is 0.*/
    qcm_mqtt_quality_of_service_e qos;     /*!< The Quality of Service (QOS) for MQTT publish.*/
    qosa_bool_t                   retain;  /*!< The retain flag for MQTT publish.*/
    qcm_mqtt_data_t               topic;   /*!< The topic name for MQTT publish.*/
    qcm_mqtt_data_t               payload; /*!< The content of the MQTT publish message.*/
#ifdef CONFIG_QCM_MQTT5_FUNC
    qcm_mqtt_properties_t props;           /**< MQTT 5.0 properties */
#endif                                     /* CONFIG_QCM_MQTT5_FUNC */
} qcm_mqtt_pub_param_t;

/**
 * @brief Used to transmit the MQTT 5.0 DISCONNECT send request.
 */
typedef struct
{
    qosa_uint8_t client_id;
#ifdef CONFIG_QCM_MQTT5_FUNC
    qosa_uint8_t          result;
    qcm_mqtt_properties_t props; /**< MQTT 5.0 properties */
#endif                           /* CONFIG_QCM_MQTT5_FUNC */
} qcm_mqtt_disc_param_t;

#ifdef CONFIG_QCM_MQTT5_FUNC
qcm_mqtt_eercode_e qcm_mqtt_topic_alias_bind(qcm_mqtt_client_t *client_ptr, qosa_uint16_t topic_alias, qcm_mqtt_data_t *topic);
#endif /* CONFIG_QCM_MQTT5_FUNC */

#endif /* __QCM_MQTT_PRIVATE_H__ */
