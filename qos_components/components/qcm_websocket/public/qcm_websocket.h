/*****************************************************************/ /**
* @file qcm_websocket.h
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

#ifndef __QCM_WEBSOCKET_H__
#define __QCM_WEBSOCKET_H__

#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_queue_list.h"

/*!< Defines related parameters for Web configuration */

/*!< Defines the maximum number of Web client IDs */
#define QCM_WEB_CLIENT_ID_NUM_MAX              2
/*!< Defines the maximum value of configuration items */
#define QCM_WEB_CFG_CONFIG_ID_MAX              2

#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
/*!< Defines the number of PDP context connection PDPCIDs */
#define QCM_WEB_CFG_CONN_PDP_CID_MIN           QOSA_PDP_CID_MIN
#define QCM_WEB_CFG_CONN_PDP_CID_MAX           QOSA_PDP_CID_MAX

/*!< Defines the number of PDP context connection SIMIDs */
#define QCM_WEB_CFG_CONN_SIM_CID_MIN           0
#define QCM_WEB_CFG_CONN_SIM_CID_MAX           (QOSA_SIMID_NUM - 1)
#endif
/*!< Defines the maximum value of SSL connection ID */
#define QCM_WEB_CFG_CONN_SSL_ID_MIN            0
#define QCM_WEB_CFG_CONN_SSL_ID_MAX            5

/*!< Defines the maximum length of URL */
#define QCM_WEB_CFG_CONN_URL_LEN_MAX           2048

/*!< Defines the file save path for "conn/url-ex" URL */
#define QCM_WEB_CFG_CONN_URL_EX_DIR            "/qsfs/conn_url_ex"

/*!< Defines the maximum length of sub-protocol */
#define QCM_WEB_CFG_CONN_SUBPROT_LEN_MAX       128

/*!< Defines the maximum length of extension items */
#define QCM_WEB_CFG_CONN_EXTENSION_LEN_MAX     1024

/*!< Defines the maximum length of extension items */
#define QCM_WEB_CFG_CONN_METHOD_LEN_MAX        256

/*!< Defines the minimum value of connection timeout */
#define QCM_WEB_CFG_CONN_TIMEOUT_MIN           1
/*!< Defines the maximum value of connection timeout */
#define QCM_WEB_CFG_CONN_TIMEOUT_MAX           255
/*!< Defines the default value of connection timeout, in seconds */
#define QCM_WEB_CFG_CONN_TIMEOUT_DEFAULT       60

/*!< Minimum, maximum and default values of ping interval (in seconds) */
#define QCM_WEB_CFG_PING_INTERVAL_MIN          0
#define QCM_WEB_CFG_PING_INTERVAL_MAX          255
#define QCM_WEB_CFG_PING_INTERVAL_DEFAULT      60

/*!< Minimum, maximum and default values of read buffer length (in bytes) */
#define QCM_WEB_CFG_READ_BUF_LEN_MIN           4096
#define QCM_WEB_CFG_READ_BUF_LEN_MAX           (16 * 1024)
#define QCM_WEB_CFG_READ_BUF_LEN_DEFAULT       (10 * 1024)
/*!< Minimum, maximum and default values of read operation timeout (in seconds) */
#define QCM_WEB_CFG_READ_TIMEOUT_MIN           1
#define QCM_WEB_CFG_READ_TIMEOUT_MAX           255
#define QCM_WEB_CFG_READ_TIMEOUT_DEFAULT       60

/*!< Minimum, maximum and default values of write buffer length (in bytes) */
#define QCM_WEB_CFG_WRITE_BUF_LEN_MIN          4096
#define QCM_WEB_CFG_WRITE_BUF_LEN_MAX          (16 * 1024)
#define QCM_WEB_CFG_WRITE_BUF_LEN_DEFAULT      (10 * 1024)

/*!< Minimum, maximum and default values of write operation timeout (in seconds) */
#define QCM_WEB_CFG_WRITE_TIMEOUT_MIN          1
#define QCM_WEB_CFG_WRITE_TIMEOUT_MAX          255
#define QCM_WEB_CFG_WRITE_TIMEOUT_DEFAULT      60

/*!< Minimum, maximum and default values of close wait timeout (in seconds) */
/*!< Close wait refers to the time to wait for data transfer to complete before closing the connection */
#define QCM_WEB_CFG_CLOSE_WAIT_TIMEOUT_MIN     0
#define QCM_WEB_CFG_CLOSE_WAIT_TIMEOUT_MAX     255
#define QCM_WEB_CFG_CLOSE_WAIT_TIMEOUT_DEFAULT 10

/*!< Custom request header (in bytes) */
#define QCM_WEB_CFG_REQHEAD_BUF_LEN_MAX        256

/*!< Maximum value for one read (size of one socket read) */
#define QCM_WEB_TCP_READ_BUFFER_SIZE           2048
/*!< Maximum value for one write */
#define QCM_WEB_TCP_WRITE_BUFFER_SIZE          2048

#define QCM_WEB_VERSION                        13
/*
 * Define enumeration types for Web configuration
 * This enumeration type is used to specify different parameters of Web configuration, including connection parameters, timeout settings, buffer sizes, callback functions, etc.
 */
typedef enum
{   
    QCM_WEB_CFG_CONN_MIN = 0,
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
    QCM_WEB_CFG_CONN_PDPCID,   /*!< PDPCID of the connection */
    QCM_WEB_CFG_CONN_SIMCID,       /*!< SIMCID of the connection */
#endif
    QCM_WEB_CFG_CONN_SSL_CTX_ID,   /*!< SSL context ID */
    QCM_WEB_CFG_CONN_SSLCONFIG,    /*!< SSL configuration */
    QCM_WEB_CFG_CONN_URL,          /*!< URL of the connection */
    QCM_WEB_CFG_CONN_URL_EX,       /*!< Whether URL is configured through "conn/url-ex" command */
    QCM_WEB_CFG_CONN_SUBPROT_EN,   /*!< Sub-protocol enable */
    QCM_WEB_CFG_CONN_SUBPROT,      /*!< Sub-protocol */
    QCM_WEB_CFG_CONN_EXTENSION_EN, /*!< Extension function enable */
    QCM_WEB_CFG_CONN_EXTENSION,    /*!< Extension function */
    QCM_WEB_CFG_CONN_METHOD,       /*!< Connection method */
    QCM_WEB_CFG_CONN_TIMEOUT,      /*!< Connection timeout */
    QCM_WEB_CFG_PING_INTERVAL,     /*!< Ping interval */
    QCM_WEB_CFG_WRITE_BUFFERSZ,    /*!< Write buffer size */
    QCM_WEB_CFG_WRITE_TIMEOUT,     /*!< Write operation timeout */
    QCM_WEB_CFG_WRITE_ECHO,        /*!< Write operation echo */
    QCM_WEB_CFG_READ_BUFFERSZ,     /*!< Read buffer size */
    QCM_WEB_CFG_READ_TIMEOUT,      /*!< Read operation timeout */
    QCM_WEB_CFG_READ_MODE,         /*!< Read operation mode */
    QCM_WEB_CFG_CLOSE_WAITTIME,    /*!< Close wait time */
    QCM_WEB_CFG_RECV_CB,           /*!< Receive callback function */
    QCM_WEB_CFG_REQHEAD,           /*!< Custom request header addition */
    QCM_WEB_CFG_REQHEAD_DEL,       /*!< Custom request header removal */
    QCM_WEB_CFG_REQHEAD_DEL_ALL,   /*!< Remove all custom request headers */
    QCM_WEB_CFG_MAX,
} qcm_web_cfg_e;

/**
 * @brief Define error codes for OSA Web operations.
 *
 * This enumeration type defines various error codes that may be encountered during Web-related operations,
 * used to identify specific error types for error handling and debugging.
 */
typedef enum
{
    QCM_WEB_ERR_OK = 0,                   /*!< Operation successful */
    QCM_WEB_ERROR_UNKNOW = 1,             /*!< Unknown error */
    QCM_WEB_ERR_UNSUPPORTED_PROTOCOL = 2, /*!< Protocol error */
    QCM_WEB_ERR_INVAL_PARM = 3,           /*!< Invalid parameter error */
    QCM_WEB_ERR_NOMEM = 4,                /*!< Insufficient memory error */
    QCM_WEB_ERR_TIMEDOUT = 5,             /*!< Timeout error */
    QCM_WEB_ERR_BUSY = 6,                 /*!< Busy error */
    QCM_WEB_ERR_NOT_INIT = 7,             /*!< Not initialized error */
    QCM_WEB_ERR_NET_FAIL = 8,             /*!< Network error */
    QCM_WEB_ERR_ID_OCCUPIED = 9,          /*!< ID occupied error */
    QCM_WEB_ERR_INVALID_URL_FORMAT = 10,  /*!< Invalid URL error */
    QCM_WEB_ERR_INVALID_PORT = 11,        /*!< Invalid port error */
    QCM_WEB_ERR_TASK_CREATE = 12,         /*!< Task creation failure error */
    QCM_WEB_ERR_NET_CLOSE = 13,           /*!< Network connection closed error */
    QCM_WEB_ERR_NET_DOWN = 14,            /*!< Network failure error */
    QCM_WEB_ERR_WRITE_ = 15,              /*!< Insufficient memory error */

    QCM_WEB_ERR_DNS_FAIL = 16,            /*!< DNS resolution failure error */
    QCM_WEB_ERR_TCP_CONNECT_FAIL = 17,    /*!< TCP connection failure error */
    QCM_WEB_ERR_TLS_HANDSHAKE_FAIL = 18,  /*!< TLS handshake failure error */
    QCM_WEB_ERR_HTTP_UPGRADE_FAIL = 19,   /*!< HTTP upgrade failure error */
    QCM_WEB_ERR_SEND_BUFF_FULL = 20,      /*!< Send buffer full error */
    QCM_WEB_ERR_OPERATION_PENDING = 21,   /*!< Operation incomplete error */
} qcm_web_err_e;

/**
 * @brief Data echo mode enumeration
 *
 * This enumeration defines two modes for data echo: echo off and echo on.
 */
typedef enum
{
    QCM_WEB_ECHO_OFF = 0, /*!< Echo off mode, value is 0 */
    QCM_WEB_ECHO_ON = 1,  /*!< Echo on mode, value is 1 */
} qcm_web_echo_e;

/**
 * @brief Defines enumeration type for data reporting
 *
 * This enumeration type defines two modes: buffer mode and direct push mode.
 */
typedef enum
{
    QCM_WEB_MODE_BUFFER = 0, /*!< Buffer mode: data is cached first and then processed */
    QCM_WEB_MODE_PUSH = 1,   /*!< Direct push mode: data is sent directly without caching */
} qcm_web_mode_e;

/**
 * @brief Defines whether the websocket URL is configured through "conn/url-ex"
 *
 * This enumeration type is used to indicate whether the websocket URL is configured through "conn/url-ex".
 */
typedef enum
{
    QCM_WEB_URL_EX_DISABLE = 0, /*!< URL not configured through "conn/url-ex" */
    QCM_WEB_URL_EX_ENABLE = 1,  /*!< URL configured through "conn/url-ex" */
} qcm_web_url_ex_e;

/**
 * @brief Defines enumeration type for websocket sub-protocol enable status
 *
 * This enumeration type is used to represent whether the websocket sub-protocol is enabled or disabled.
 */
typedef enum
{
    QCM_WEB_SUBPROT_DISABLE = 0, /*!< Indicates that the websocket sub-protocol is disabled */
    QCM_WEB_SUBPROT_ENABLE = 1,  /*!< Indicates that the websocket sub-protocol is enabled */
} qcm_web_subprot_e;

/**
 * @brief Defines enumeration type for websocket extension protocol enable status
 *
 * This enumeration type is used to represent whether the websocket extension protocol is enabled or disabled.
 */
typedef enum
{
    QCM_WEB_EXTENSION_DISABLE = 0, /*!< WebSocket disables custom extensions */
    QCM_WEB_EXTENSION_ENABLE = 1,  /*!< WebSocket enables custom extensions */
} qcm_web_extension_e;

/**
 * @brief Defines the type of data to be sent
 */
typedef enum
{
    QCM_WEB_OPCODE_TEXT = 0,   /*!< Text frame */
    QCM_WEB_OPCODE_BINARY = 1, /*!< Binary frame */
} qcm_web_opcode_e;

/**
 * @brief Defines QCM_WEB_CFG_RECV_CB callback type enumeration
 *
 * This enumeration defines three types of network operations: open, receive, and close.
 */
typedef enum
{
    QCM_WEB_TYPE_OPEN = 0, /*!< Open network connection */
    QCM_WEB_TYPE_RECV,     /*!< Receive network data */
    QCM_WEB_TYPE_CLOSE,    /*!< Close network connection */
    QCM_WEB_TYPE_WRITE,    /*!< Can continue to send network data */
} qcm_web_cb_type_e;

/**
 * @brief QWSTATE command query socket connection status enumeration
 */
typedef enum
{
    QCM_WEB_CONN_STA_IDLE = 0,                       /*!< Connection not yet established or has been closed and destroyed */
    QCM_WEB_CONN_STA_CONNECTING,                     /*!< Client is connecting */
    QCM_WEB_CONN_STA_CONNECTED,                      /*!< Client connection established */
    QCM_WEB_CONN_STA_CLOSING,                        /*!< Connection is being disconnected */
    QCM_WEB_CONN_STA_CLOSED = QCM_WEB_CONN_STA_IDLE, /*!< Connection not yet established or has been closed and destroyed */
} qcm_web_conn_status_e;

/**
 * qcm_web_cb_t type definition
 * Used to define QCM_WEB_CFG_RECV_CB callback structure, storing callback-related data and status.
 */
typedef struct
{
    qcm_web_cb_type_e type;   /*!< Callback type, used to identify different callback events */
    char             *data;   /*!< Pointer to callback data, used to store websocket data */
    int               size;   /*!< Data size, representing the byte length of websocket data */
    int               result; /*!< Callback result, used to represent the success or failure status of websocket open or close execution */
} qcm_web_cb_t;

/**
 * qcm_web_recv_cb_t type definition
 * Define a pointer type for a Web receive callback function.
 *
 * @param client_id Client ID, identifying the client that triggered the callback
 * @param cb_t Pointer to qcm_web_cb_t structure, containing specific information about the callback event (such as type, data, result, etc.)
 *
 */
typedef int (*qcm_web_recv_cb_t)(int client_id, qcm_web_cb_t *cb_t);

/**
 * @brief Defines the structure type for qcm_ws_open_proc parameters
 *
 * This structure is used to store client ID and configuration ID for opening resources or services through the network.
 */
typedef struct
{
    int client_id; /*!< Client identifier, used to uniquely identify a client */
    int config_id; /*!< Configuration identifier, used to specify the configuration information of the client */
} qcm_web_open_t;

/**
 * @brief Defines a structure type for configuring or reading websocket config information
 *
 * This structure contains various types of data fields, used to store different parts of configuration information.
 * It includes string type data, string length, integer data, and a pointer type data.
 */
typedef struct
{
    char *str_value; /*!< String type data, used to store the string part of configuration information */
    int   str_len;   /*!< String length, identifying the effective character length of str_value field */
    int   int_value; /*!< Integer data, used to store the integer part of configuration information */
    void *ptr_value; /*!< Pointer type, can be used to store pointers to any type of data needed in configuration information, including header_list, cb */
} qcm_web_cfg_t;

/**
 * @brief Defines a network connection structure, used to store configuration information required to establish a network connection.
 *
 * This structure includes parameters such as PDP context ID, SIM card ID, SSL context ID, target URL, sub-protocol enable and name, etc.,
 * used to configure and identify a network connection (such as WebSocket connection).
 */
typedef struct
{
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
    int   pdp_cid;       /*!< PDP context ID, used to identify a session in the mobile network. */
    int   sim_cid;       /*!< SIM card ID, used to identify the SIM card in use. */
#endif
    char *url;           /*!< Target URL, specifying the server address and path to connect to. */
    int   url_len;       /*!< URL data length, used to specify the length of the url string. */
    int   url_ex;        /*!< Whether URL is configured through "conn/url-ex". */
    int   subprot_en;    /*!< Sub-protocol enable flag, 1 means enable, 0 means disable. */
    char *subprot;       /*!< Sub-protocol name, when sub-protocol is enabled, specify the sub-protocol name to use. */
    int   subprot_len;   /*!< subprot data length, used to specify the length of the subprot string. */
    int   extension_en;  /*!< WebSocket extension enable flag, 1 means enable, 0 means disable. */
    char *extension;     /*!< WebSocket extension content, used to specify the WebSocket extension to use when connecting. */
    int   extension_len; /*!< extension data length, used to specify the length of the extension string. */
    char *method;        /*!< method connection method content */
    int   method_len;    /*!< method connection method length */
    int   timeout;       /*!< WebSocket connection timeout, in milliseconds. */
    int   ssl_ctx_id;    /*!< SSL context ID */
    void *ssl_config;    /*!< SSL configuration content pointer, pointing to the structure containing SSL/TLS configuration information. */
} qcm_web_conn_t;

/**
 * @brief Defines the qcm_web_ping_t structure, used to configure parameters for ping keep-alive functionality
 */
typedef struct
{
    int interval; /*!< Ping keep-alive interval, in seconds (s) */
} qcm_web_ping_t;

/**
 * Defines the structure for OSA Websocket read configuration
 *
 * This structure includes the following members:
 * - buffer_sz: Size of Websocket receive cache, in bytes. Valid range is 4096-16384, default value is 10240.
 * - timeout: Timeout for sending data, in milliseconds.
 * - mode: Data receiving mode. 0 means buffer mode, where the first received data will be stored in the cache and the data length will be returned (default mode); 1 means direct push mode, where received data will be sent directly without caching.
 * - recv_cb: Receive callback function, used to process received data.
 */
typedef struct
{
    int buffer_sz; /*!< Size of Websocket receive cache, in bytes. Valid range 4096-16384, default value 10240. */
    int timeout;   /*!< Timeout for sending data, in milliseconds. */
    int mode; /*!< Data receiving mode. 0 means buffer mode (default), where the first received data is stored in the cache and the data length is returned; 1 means direct push mode, where data is sent directly. */
    qcm_web_recv_cb_t recv_cb; /*!< Receive callback function, used to process received data. */
} qcm_web_read_t;

/**
 * @brief Defines the structure for WebSocket write configuration
 *
 * This structure is used to configure parameters related to WebSocket write operations, including send cache size, timeout for receiving data, and whether to enable echo functionality.
 */
typedef struct
{
    int buffer_sz; /*!< Size of websocket send cache buf, in bytes. Configurable range is 4096-16384, default value is 10240. */
    int timeout;   /*!< Timeout for receiving data, in milliseconds. Used to set the wait timeout when receiving data. */
    int echo;      /*!< Control flag for whether to echo sent data. 0 means disable echo function (default), 1 means enable echo function.*/
} qcm_web_write_t;

/**
 * @brief Defines the timeout parameter when web socket is closed
 *
 * This structure contains an integer variable wait_time, used to specify the timeout when web socket is closed.
 */
typedef struct
{
    int wait_time; /*!< Specify the timeout for web socket close, in specific time units such as milliseconds or seconds */
} qcm_web_close_t;

/*
 * Define a structure type for network packet header addition
 * This structure includes the following members:
 * - list: Linked list node, used to add packet headers to the linked list for management
 * - data: Pointer to the character array storing packet header data
 * - len: Length of packet header data
 */
typedef struct
{
    char *key;       /*!< Store the key of the data, field name */
    char *value;     /*!< Pointer to the character array storing packet header data, field content */
    int   key_len;   /*!< Length of packet header data */
    int   value_len; /*!< Length of packet header data */
} qcm_web_header_data_t;

/*
 * Define a structure type for network packet header addition
 * This structure includes the following members:
 * - list: Linked list node, used to add packet headers to the linked list for management
 * - data: Pointer to the character array storing packet header data
 * - len: Length of packet header data
 */
typedef struct
{
    qosa_q_link_type_t    list; /*!< Linked list node, for linked list management of packet headers */
    qcm_web_header_data_t data; /*!< Linked list data */
} qcm_web_header_list_t;

/**
 * @brief Define a structure type qcm_web_config_t, used to configure various operations of network connections
 *
 * This structure contains configurations for various operations of network connections, specifically including:
 * - conn: Network connection related configuration, detailed structure definition in qcm_web_conn_t.
 * - ping: Related configuration for sending ping requests, detailed structure definition in qcm_web_ping_t.
 * - read: Related configuration for reading data from network connections, detailed structure definition in qcm_web_read_t.
 * - write: Related configuration for writing data to network connections, detailed structure definition in qcm_web_write_t.
 * - close: Related configuration for closing network connections, detailed structure definition in qcm_web_close_t.
 *
 */
typedef struct
{
    qcm_web_conn_t  conn;        /*!< Network connection configuration */
    qcm_web_ping_t  ping;        /*!< Ping request configuration */
    qcm_web_read_t  read;        /*!< Read data configuration */
    qcm_web_write_t write;       /*!< Write data configuration */
    qcm_web_close_t close;       /*!< Close connection configuration */
    qosa_q_type_t   header_list; /*!< Request header queue */
} qcm_web_config_t;

/**
 * @brief Set configuration items for Web service.
 *
 * This function is used to set specified Web service configuration items based on configuration ID and configuration option tag.
 * Configuration items can be various service parameters, such as port number, authentication information, etc.
 *
 * @param config_id ID of the configuration item, used to identify the specific configuration item to be set.
 * @param opt_tag Option tag of the configuration item, used to specify the configuration attribute to be modified.
 * @param ptr Pointer to the memory address of the value to be set. Depending on the configuration item, this parameter is used to configure strings and function pointers.
 * @param value Integer value of the configuration item. Depending on the configuration item, the meaning of this value varies. If the configuration item is a string, this parameter is the length of the string.
 * If the configuration item is a function pointer, this parameter has no meaning, if the configuration item is an integer, this parameter is the integer value.
 *
 * @return The function returns an error code, indicating the success or failure of the operation. Returns OSA_WS_SUCCESS on success, returns the corresponding error code on failure.
 */
qcm_web_err_e qcm_ws_cfg_set(int config_id, qcm_web_cfg_e opt_tag, void *ptr, int value);

/**
 * @brief Get Web configuration information
 *
 * This function is used to get specific configuration information from Web configuration based on configuration ID and option tag.
 *
 * @param config_id Configuration ID, used to identify specific configuration items.
 * @param opt_tag Option tag, used to specify the type of configuration information to get.
 * @param cfg Pointer to a structure used to store the obtained configuration information.
 *
 * @return Returns the result status code of the get operation, returns OSA_WS_CFG_SUCCESS on success, returns error code on failure.
 */
qcm_web_err_e qcm_ws_cfg_get(int config_id, qcm_web_cfg_e opt_tag, qcm_web_cfg_t *cfg);

/**
 * @brief Set all network configuration parameters
 *
 * This function is used to set all network configuration parameters based on the given configuration ID and new network configuration structure.
 *
 * @param config_id Configuration ID, used to specify the configuration item to be set.
 * @param new_config Pointer to the structure containing new configuration parameters.
 * @return Returns the result status code of the operation, success or failure.
 */
qcm_web_err_e qcm_ws_cfg_set_all(int config_id, const qcm_web_config_t *new_config);

/**
 * @brief Get all Web configuration information for the specified configuration ID
 *
 * This function is used to get all Web configuration information for the specified configuration ID from the system. It returns a pointer to a qcm_web_config_t structure array,
 * which contains all Web configuration items associated with the specified configuration ID. This function gets the original configuration information, do not modify it arbitrarily
 *
 * @param config_id Configuration ID, used to specify the type or range of configuration information to get
 * @return qcm_web_config_t* Pointer to the array containing all Web configuration information. Returns NULL if no configuration is found.
 */
qcm_web_config_t *qcm_ws_cfg_get_all(int config_id);

/**
 * @brief Handler function for opening WebSocket connection.
 *
 * This function is responsible for initializing and opening a WebSocket connection.
 *
 * @param open_t Pointer to qcm_web_open_t structure, containing configuration information needed to open the WebSocket connection.
 * @return qcm_web_err_e Function execution status enumeration value, returns QCM_WEB_SUCCESS on success, returns corresponding error code on failure.
 */
qcm_web_err_e qcm_ws_open_proc(qcm_web_open_t *open_t);

/**
 * @brief Close WebSocket connection
 *
 * This function is used to close the WebSocket connection for the specified client ID.
 *
 * @param client_id Client ID, identifying the WebSocket connection to be closed.
 * @return qcm_web_err_e Returns the result status code of the operation, success or failure.
 */
qcm_web_err_e qcm_ws_close_proc(int client_id);

/**
 * Read WebSocket data
 *
 * This function is used to read data from the WebSocket client. It will try to read data from the specified client ID and store the read data in the provided buffer.
 *
 * @param client_id Client ID, used to identify the WebSocket client from which to read data.
 * @param data Pointer to the buffer used to store the read data. The read data will be stored in this buffer.
 * @param size Pointer to an integer, used to provide the buffer size when the function is called, and return the actual read data size when the function returns.
 * @return Returns the result status code of the operation, returns OSA_WS_SUCCESS on success, returns other error codes on failure.
 */
qcm_web_err_e qcm_ws_read_proc(int client_id, char *data, int *size);

/**
 * @brief Write data to the specified WebSocket client.
 *
 * @param client_id Client ID, used to identify the WebSocket client that will receive the data.
 * @param data Pointer to the data to be written.
 * @param len Length of the data to be written.
 * @return qcm_web_err_e Returns the result status code of the operation, success or error type.
 * @note If the return is
 */
qcm_web_err_e qcm_ws_write_proc(int client_id, char *data, int len, qcm_web_opcode_e opcode);

/**
 * Get the current socket connection status and client status
 *
 * @param client_id Client ID, used to identify the specific WebSocket client
 * @param socket_state Returns the socket connection status.
 * @param client_state Returns the client status.
 * @return If successful, returns QCM_WEB_OK. If failed, returns error code;
 */
int qcm_ws_client_get_curr_conn_status(int client_id, int *socket_state, int *client_state);

/**
 * Get how much data has been sent from TX to the module, and how much of this data has been successfully sent through send
 *
 * @param client_id Client ID, used to identify the specific WebSocket client
 * @param tx_total_size Get data: how much data has been sent from TX to the module in total
 * @param send_size Get data: how much data has been successfully sent through send
 * @return If successful, returns QCM_WEB_OK. If failed, returns error code;
 */
qcm_web_err_e qcm_ws_client_get_send_size(int client_id, int *tx_total_size, int *send_size);

#endif /* __QCM_WEBSOCKET_H__ */
