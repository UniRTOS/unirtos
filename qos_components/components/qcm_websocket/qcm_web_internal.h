/*****************************************************************/ /**
* @file qcm_web_internal.h
* @brief
* @author harry.li@quectel.com
* @date 2024-04-25
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2024-04-25 <td>1.0 <td>harry.Li <td> Init
* </table>
**********************************************************************/

#ifndef __QCM_WEB_INTERNAL_H__
#define __QCM_WEB_INTERNAL_H__

#include "qosa_def.h"
#include "qosa_sys.h"
#include "qcm_proj_config.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
#include "qosa_datacall.h"
#endif
#include "qosa_queue_list.h"
#include "qcm_websocket.h"
#include "qcm_web_ws.h"
#ifdef CONFIG_QCM_VTLS_FUNC
#include "qcm_vtls_cfg.h"
#endif

#define QCM_WEB_MSGQ_SIZE            10   //Message queue size
#define QCM_WEB_HEADER_COUNT         20   //Limit the number of custom headers, currently unused
#define QCM_WEB_CLIENT_TASK_SIZE     (20 * 1024)
#define QCM_WEB_PDP_ACTIVE_TIMEOUT   30   //PDP activation timeout 30s

#define QCM_WEB_WS_PORT_DEFAULT      80   //ws default port
#define QCM_WEB_WSS_PORT_DEFAULT     443  //Default port for wss

#define QCM_WEB_WATERMARK_LIMIT      (20 * 1024)
#define QCM_WEB_WATERMARK_HIGH_LIMIT (18 * 1024)  //The maximum data size for a single transmission can be up to 16K, so it cannot be set too low.
#define QCM_WEB_WATERMARK_LOW_LIMIT                                                                                                                            \
    (QCM_WEB_WATERMARK_HIGH_LIMIT - QCM_WEB_CFG_READ_BUF_LEN_MIN                                                                                               \
    )  //The minimum read buffer size is QCM_WEB_CFG_READ_BUF_LEN_MIN. After reading a block of data of size QCM_WEB_CFG_READ_BUF_LEN_MIN, this freed space can be reused to read new data.

#define QCM_WEB_URL_MAX_LEN 256
/*
 * Defines enumeration types related to network messages.
 * These enumeration values are used to identify different message types for network operations, such as network requests, responses, DNS requests, DNS responses, etc.
 * This enumeration type provides a convenient classification method for network message processing.
 */
typedef enum
{
    QCM_WEB_MSG_NET_REQUEST = 0,       //Network request message
    QCM_WEB_MSG_NET_RESPONSE,          //Network response message
    QCM_WEB_MSG_DNS_REQUEST,           //DNS request message
    QCM_WEB_MSG_DNS_RESPONSE,          //DNS response message
    QCM_WEB_MSG_TCP_OPEN,              //TCP connection open message
    QCM_WEB_MSG_TCP_OPEN_CNF = 5,      //TCP connection open confirmation message
    QCM_WEB_MSG_TCP_OPEN_TIMEOUT,      //TCP connection open timeout message
    QCM_WEB_MSG_TCP_CLOSING,           //TCP connection closing message
    QCM_WEB_MSG_TCP_CLOSE_CNF,         //TCP connection closure confirmation message
    QCM_WEB_MSG_TCP_WRITE,             //TCP Write Message
    QCM_WEB_MSG_WS_READ = 10,          //WebSocket read message
    QCM_WEB_MSG_WS_WRITE,              //WebSocket write message
    QCM_WEB_MSG_WS_CLOSE,              //WebSocket close message
    QCM_WEB_MSG_TCP_RECV_DATA_REPORT,  //TCP Receive Data Report Message
    QCM_WEB_MSG_SOCKET_EVENT,          //Socket Event Message
    QCM_WEB_MSG_TLS_HANDSHAKE = 15,    //TLS handshake message
    QCM_WEB_MSG_HTTP_REQUEST,          //HTTP Request Message
    QCM_WEB_MSG_HTTP_RESPONSE,         //HTTP response message
    QCM_WEB_MSG_PING_REQUEST,          //PING request message
    QCM_WEB_MSG_PONG_REQUEST,          //PONG request message
} qcm_web_msg_e;

/**
 * OSA Network Status Enumeration
 * Used to represent different states of network operations
 */
typedef enum
{
    QCM_WEB_STATUS_INIT = 0,                 /*!< Initialization state*/
    QCM_WEB_STATUS_NET,                      /*!< Network status, indicating whether the network is available*/
    QCM_WEB_STATUS_DNS,                      /*!< DNS resolution status, indicating whether DNS resolution is in progress*/
    QCM_WEB_STATUS_TCP_CONNECTING,           /*!< TCP connection in progress state, indicating whether a TCP connection is being attempted to be established.*/
    QCM_WEB_STATUS_TCP_CONNECTED,            /*!< TCP connection established, indicating that the TCP connection has been successfully established.*/
    QCM_WEB_STATUS_TLS_HANDSHAKE = 5,        /*!< HTTP handshake status, indicating whether an HTTP handshake is in progress*/
    QCM_WEB_STATUS_TLS_HANDSHAKE_OK,         /*!< HTTP handshake status, indicating whether an HTTP handshake is in progress*/
    QCM_WEB_STATUS_TLS_HANDSHAKE_FAIL,       /*!< HTTP handshake status, indicating whether an HTTP handshake is in progress*/
    QCM_WEB_STATUS_HTTP_HANDSHAKE,           /*!< HTTP handshake status, indicating whether an HTTP handshake is in progress*/
    QCM_WEB_STATUS_HTTP_HANDSHAKE_OK,        /*!< HTTP handshake status, indicating a successful handshake*/
    QCM_WEB_STATUS_HTTP_HANDSHAKE_FAIL = 10, /*!< HTTP handshake status, indicating handshake failure*/
    QCM_WEB_STATUS_WEBSOCKET,                /*!< WebSocket status, indicating whether the WebSocket connection has been established*/
    QCM_WEB_STATUS_TCP_CLOSING,              /*!< Closing state, indicating the connection is being closed*/
    QCM_WEB_STATUS_TCP_CLOSED,               /*!< Closed state, indicating the connection has been closed*/
} qcm_web_status_e;

/**
 * @brief Define a structure type named qcm_web_config_internal_t
 *
 * This structure is used to encapsulate web configuration-related data and synchronization mechanisms.
 * It includes:
 * @param qcm_web_config_t config: stores the specific parameters for web configuration
 * @param qosa_q_type_t header_list: queue for storing request headers
 * @param qosa_mutex_t lock: Mutex lock to ensure thread-safe access to configuration data
 */
typedef struct
{
    qcm_web_config_t config; /*!< Web Configuration Parameters*/
    qosa_mutex_t     lock;   /*!< Access mutex*/
} qcm_web_config_internal_t;

/**
 * @brief Define a structure of type qcm_web_client_t to represent Web client information
 *
 * This structure contains various information about the client, such as client ID, configuration ID, data, user identifier, and status.
 */
typedef struct
{
    int                  client_id;                                 /*!< Unique identifier of the client*/
    int                  config_id;                                 /*!< Client configuration ID*/
    char                *data;                                      /*!< Client-related data*/
    int                  user;                                      /*!< Associated user identifier*/
    qcm_web_status_e     status;                                    /*!< Client status*/
    int                  sock_hd;                                   /*!< The socket handle of the client*/
    web_socket_url       url;                                       /*!< The URL of the web socket to which the client will connect*/
    qosa_ip_addr_t       local_addr;                                /*!< Local IP address of the client*/
    qosa_uint32_t        ai_family;                                 /*!< Address family, specifies the IP address version (e.g., IPv4 or IPv6)*/
    qosa_ip_addr_t       remote_ip;                                 /*!< Remote server IP address*/
    char                 remote_host_addr[QCM_WEB_URL_MAX_LEN + 1]; /*!< Host address string of the remote server*/
    char                *web_response_hd;                           /*!< The response header received by the web client, enabling it to be concatenated into a complete response header*/
    int                  web_response_hd_len;                       /*!< The length of the response header received by the web client, with a maximum header limit of 20k.*/
    char                 web_key[32];                               /*!< Key-value for web client*/
    char                 web_accept_key[32];                        /*!< Key-value pairs accepted by the web client*/
    qosa_timer_t         timer;                                     /*!< Client TCP connection timer*/
    qosa_timer_t         ping_timer;                                /*!< Client's PING timer*/
    int                  result;                                    /*!< Result of client operation*/
    web_transfer_session transfer_session;                          /*!< Client Transmission Session*/
    web_socket_frame_t   r_frame;                                   /*!< WebSocket frame received by the client*/
    web_socket_frame_t   s_frame;                                   /*!< WebSocket frame sent by the client*/
    qosa_q_type_t        s_frame_list;            /*!< In ws_construct_frame, when constructing a new frame, each new frame's information is attached as a node to this linked list.*/
    int                  tx_total_size;           /*!< Record: Total data sent from TX to the module*/
    int                  send_payload_total_size; /*!< Record: How much data from TX has been successfully sent out via send*/
#ifdef CONFIG_QCM_VTLS_FUNC
    qcm_ssl_config_t *ssl_config;                 /*!< Configure the parameters for the SSL connection*/
#endif
    qosa_mutex_t lock;                            /*!< Mutex for client operations*/
} qcm_web_client_t;

/**
 * @brief Defines a structure type named qcm_web_service_t, used to encapsulate the task ID, message queue, and mutex of the web service.
 *
 * This structure contains the following members:
 * @param taskid: Represents the task ID of the web service, of type qcm_task_t.
 * @param msgq: Represents the message queue used by the web service for message communication, of type qcm_msgq_t.
 * @param lock: Indicates the mutex used by the web service for thread synchronization and resource access control, of type qcm_mutex_t.
 */
typedef struct
{
    qosa_task_t  taskid;  //Task ID of the web service
    qosa_msgq_t  msgq;    //Message queue for web services
    qosa_mutex_t lock;    //Mutex for Web Service
} qcm_web_service_t;

/**
 * @brief Define a structure type qcm_web_msg_t to represent Web messages.
 *
 * The structure contains the following fields:
 * @param msgid: Message ID, used to identify different messages.
 * @param param1: Parameter 1, can pass different parameters according to specific message requirements.
 * @param param2: Parameter 2, similar to param1, used to pass additional parameters.
 * @param argv: A pointer to arguments, which can point to data of any type, used for passing more complex or variable parameters.
 */
typedef struct
{
    qcm_web_msg_e msgid;  /*!< Message ID*/
    int           param1; /*!< Parameter 1*/
    int           param2; /*!< Parameter 2*/
    void         *argv;   /*!< Pass parameters*/
} qcm_web_msg_t;

/**
 * @brief Defined the qcm_web_param_t type, which is a structure used to encapsulate client information.
 *
 * This structure contains the following members:
 * @param client_id: The unique identifier of the client, used to distinguish between different clients.
 * @param config_id: The configuration ID of the client, used to identify the client's configuration information.
 * @param data: Client-related data, which can store any type of data; its specific purpose depends on the context.
 * @param status: The status of the client, used to indicate the current state of the client (such as connection status, operational status, etc.).
 */
typedef struct
{
    int              client_id; /*!< Unique identifier of the client*/
    int              config_id; /*!< Client configuration ID*/
    void            *data;      /*!< Client-related data*/
    qcm_web_status_e status;    /*!< Client status*/
} qcm_web_param_t;

/**
 * @brief Define a structure type for network socket events
 *
 * This structure is used to encapsulate various event information in network socket operations, facilitating processing by upper-layer applications.
 *
 * @param sock_fd     socket channel number, index value, not the actual socket file descriptor.
 * @param event_mask  Event mask, used to indicate the types of events of interest.
 * @param code         Execution result or error code, used to indicate the success or failure of an operation.
 * @param user_data    A pointer to user data passed by the upper-layer application, which can be used to access context information during event handling.
 */
typedef struct
{
    int   sock_fd;    /*!< Number of socket channels, index value, not socketfd*/
    int   event_mask; /*!< Mask time type*/
    int   code;       /*!< Execution result/error code*/
    void *user_data;  /*!< Pointer to upper-layer application parameters*/
} qcm_web_soc_event_t;

/**
 * @brief Define a structure type named qcm_web_data_t for storing web data.
 *
 * This structure contains two members:
 * @param data: pointer to the character array storing the actual data.
 * @param data_len: An integer variable representing the data length.
 */
typedef struct
{
    char *data;     /*!< Pointer to the character array storing the actual data.*/
    int   opcode;   /*!< Text Frame*/
    int   data_len; /*!< Data length.*/
} qcm_web_data_t;

/**
 * @brief Get the WebSocket client pointer for the specified client ID
 *
 * This function searches for the corresponding WebSocket client instance globally by client ID and returns its pointer.
 *
 * @param client_id Client ID, used to identify a unique WebSocket client
 * @return qcm_web_client_t* Pointer to the found WebSocket client; returns NULL if not found
 */
qcm_web_client_t *qcm_ws_client_get_ptr(int client_id);

/**
 * @brief Get the network configuration pointer for the specified configuration ID
 *
 * This function is used to search for the configuration with the specified ID from the internal network configuration table and returns its structure pointer.
 *
 * @param config_id The ID of the configuration, used to identify a specific network configuration
 * @return qcm_web_config_internal_t* Pointer to the found network configuration structure; returns NULL if not found
 */
qcm_web_config_internal_t *qcm_ws_config_get_ptr(int config_id);

/**
 * @brief Send message to WebSocket server
 *
 * @param msg_id Message ID, used to identify the type of message being sent
 * @param param1 The first parameter, the specific meaning depends on the message type
 * @param argv pointer to the buffer containing the message content
 * @return int Status code for the send operation, returns 0 on success, non-zero on failure
 */
int qcm_ws_msg_send(int msg_id, int param1, void *argv);

/**
 * @brief Send message to WebSocket server (supports two parameters)
 *
 * @param msg_id Message ID, used to identify the type of message being sent
 * @param param1 The first parameter, the specific meaning depends on the message type
 * @param param2 The second parameter, the specific meaning depends on the message type.
 * @param argv pointer to the buffer containing the message content
 * @return int Status code for the send operation, returns 0 on success, non-zero on failure.
 */
int qcm_ws_msg_send_2(int msg_id, int param1, int param2, void *argv);
/**
 * @brief Get the length of the WebSocket header
 * @param header_list pointer to the WebSocket header list
 * @return Returns the length of the WebSocket header
 */
int qcm_ws_header_length_get(qosa_q_type_t *header_list);

/**
 * @brief WebSocket receive callback function
 * @param client_id Client ID
 * @param type Message type
 * @param result status
 * @param data message data
 * @param len data length
 */
void qcm_ws_recv_cb(int client_id, qcm_web_cb_type_e type, int result, char *data, int len);

/**
 * @brief Release WebSocket client resources
 * @param client_id Client ID
 */
void qcm_ws_client_free(int client_id);

/**
 * @brief Initialize WebSocket send and receive watermarks
 * @param client_id Client ID
 */
void qcm_ws_tx_rx_watermark_init(int client_id);

/**
 * @brief Destroy the WebSocket send and receive watermarks
 * @param client_id Client ID
 */
void qcm_ws_tx_rx_deinit(int client_id);

#endif /* __QCM_WEB_INTERNAL_H__ */
