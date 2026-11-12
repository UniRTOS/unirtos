/**
 * @file qcm_m_httpclient.h
 * @brief UniRTOS Mini HTTP Client Library Header File
 * @version 1.0
 * @date 2025
 *
 * This file defines the HTTP client interface, enumeration types, and data structures.
 * Provides functions for HTTP request creation, configuration, execution, and destruction.
 */

#ifndef __QCM_M_HTTP_CLIENT_H__
#define __QCM_M_HTTP_CLIENT_H__

/**
 * @brief HTTP response data callback function type
 * @param buffer Received data buffer
 * @param size Data chunk size
 * @param nitems Number of data chunks
 * @param private_data User private data pointer
 * @return Returns 0 on success, negative error code on failure
 */
typedef int (*client_response_cb)(char* buffer, int size, int nitems, void* private_data);

/** @brief HTTP client handle type (opaque pointer) */
typedef void http_client;

/**
 * @brief HTTP client configuration options enumeration
 * Used as option parameter for qcm_m_http_client_setopt function
 */
enum
{
    HTTPCLIENT_OPT_URL,             /*!< Set request URL, parameter type: char* */
    HTTPCLIENT_OPT_METHOD,          /*!< Set HTTP request method, parameter type: int (HTTPCLIENT_REQUEST_*) */
    HTTPCLIENT_OPT_HTTP1_0,         /*!< Use HTTP/1.0 protocol, parameter type: int (0:disabled, 1:enabled) */
    HTTPCLIENT_OPT_HTTPHEADER,      /*!< Set custom HTTP header, parameter type: struct http_client_list* */
    HTTPCLIENT_OPT_POSTDATA,        /*!< Set POST request data, parameter type: char* */
    HTTPCLIENT_OPT_POSTLENGTH,      /*!< Set POST data length, parameter type: int */
    HTTPCLIENT_OPT_RESPONSECB,      /*!< Set response callback function, parameter type: client_response_cb */
    HTTPCLIENT_OPT_RESPONSECB_DATA, /*!< Set response callback user data, parameter type: void* */
    HTTPCLIENT_OPT_AUTH_TYPE,       /*!< Set authentication type, parameter type: int (HTTP_AUTH_TYPE_*) */
    HTTPCLIENT_OPT_AUTH_USENAME,    /*!< Set authentication username, parameter type: char* */
    HTTPCLIENT_OPT_AUTH_PASSWORD,   /*!< Set authentication password, parameter type: char* */
    HTTPCLIENT_OPT_PDPCID,          /*!< Set PDP context ID, parameter type: int */
    HTTPCLIENT_OPT_BIND_PORT,       /*!< Set client bind port, parameter type: int */
    HTTPCLIENT_OPT_DNS_TYPE,        /*!< Set DNS resolution type, parameter type: int */
    HTTPCLIENT_OPT_SSL_CONFIG,      /*!< Set SSL configuration, parameter type: void* */
    HTTPCLIENT_OPT_NOSACK,          /*!< Disable SACK option, parameter type: int (0:enabled, 1:disabled) */
    HTTPCLIENT_OPT_TIMEOUT_MS,      /*!< timeout configuration (Connection timeout or data waiting timeout) */
};

/**
 * @brief HTTP client information retrieval options enumeration
 * Used as information type parameter for qcm_m_http_client_getinfo function
 */
enum
{
    HTTPCLIENT_GETINFO_RESPONSE_CODE,     /*!< Get HTTP response status code, result type: int* */
    HTTPCLIENT_GETINFO_RESPONSE_DATA_LEN, /*!< Get total response data length, result type: int* */
    HTTPCLIENT_GETINFO_CHUNKED_NUM,       /*!< Get chunked encoding status (0:content_length, 1:chunked), result type: int* */
    HTTPCLIENT_GETINFO_TCP_STATE,         /*!< Get TCP connection state, result type: int* */
    HTTPCLIENT_GETINFO_RESPONSE_DATE,
};

/**
 * @brief HTTP authentication type enumeration
 */
enum
{
    HTTP_AUTH_TYPE_BASE = 1, /*!< Basic authentication method */
    HTTP_AUTH_TYPE_DIGEST    /*!< Digest authentication method */
};

/**
 * @brief HTTP request method enumeration
 */
enum
{
    HTTPCLIENT_REQUEST_GET,  /*!< GET request method */
    HTTPCLIENT_REQUEST_POST, /*!< POST request method */
    HTTPCLIENT_REQUEST_PUT,  /*!< PUT request method */

    HTTPCLIENT_REQUEST_MAX,  /*!< Maximum request method value (internal use) */
};

/**
 * @brief HTTP client error code enumeration
 */
enum
{
    HTTP_CLIENT_ERROR = -1,            /*!< General error */
    HTTP_CLIENT_OK = 0,                /*!< Operation successful */
    HTTP_CLIENT_BAD_SOCKET_ID,         /*!< Invalid socket ID */
    HTTP_CLIENT_NO_MEMORY,             /*!< Memory allocation failed */
    HTTP_CLIENT_BAD_HTTP_REQUEST,      /*!< Invalid HTTP request */
    HTTP_CLIENT_SEND_FAILED,           /*!< Data send failed */
    HTTP_CLIENT_RECV_FAILED,           /*!< Data receive failed */
    HTTP_CLIENT_HEADER_SIZE_TOO_LARGE, /*!< HTTP header too large */
    HTTP_CLIENT_GET_INVALID_HEADER,    /*!< Got invalid HTTP header */
    HTTP_CLIENT_PROCESS_HEADER_FAILED, /*!< HTTP header processing failed */
    HTTP_CLIENT_CALLBACK_FAILED,       /*!< Callback function execution failed */
    HTTP_CLIENT_CREATE_SOCKET_FAILED,  /*!< Socket creation failed */
    HTTP_CLIENT_INVALID_LOCATION,      /*!< Invalid redirect location */
    HTTP_CLIENT_ACTIVE_STOP,           /*!< Client actively stopped */
    HTTP_CLIENT_DNS_QUERY_FAILED,      /*!< DNS query failed */
    HTTP_CLIENT_BUSY,                  /*!< Client busy */
    HTTP_CLIENT_TIMEOUT,               /*!< Connection timeout or data waiting timeout */
    HTTP_CLIENT_URL_MALFORMED_INPUT,   /*!< URL format error */
};

/**
 * @brief HTTP client linked list structure
 * Used to store HTTP header information or other linked list data
 */
struct http_client_list
{
    char*                    data; /*!< Data content */
    struct http_client_list* next; /*!< Pointer to next node */
};

/**
 * @brief Initialize HTTP Client
 * @return Returns HTTP client handle on success, NULL on failure
 */
extern struct http_client* qcm_m_http_client_init(void);

/**
 * @brief Close and release HTTP client resources
 * @param client HTTP client handle
 */
extern void qcm_m_http_client_shutdown(struct http_client* client);

/**
 * @brief Append data to HTTP client linked list
 * @param list Linked list head pointer
 * @param data Data string to append
 * @return Returns new linked list head pointer on success, NULL on failure
 */
extern struct http_client_list* qcm_m_http_client_list_append(struct http_client_list* list, const char* data);

/**
 * @brief Free the memory of the entire HTTP client linked list
 * @param list Pointer to the head of the linked list to be freed
 */
extern void qcm_m_http_client_list_destroy(struct http_client_list* list);

/**
 * @brief Set HTTP Client Options
 * @param client HTTP client handle
 * @param tag Option type (HTTPCLIENT_OPT_*)
 * @param ... Variable arguments, passing corresponding parameters according to tag type
 * @return Returns HTTP_CLIENT_OK on success, corresponding error code on failure
 */
extern int qcm_m_http_client_setopt(struct http_client* client, int tag, ...);

/**
 * @brief Execute HTTP Request
 * @param client HTTP client handle
 * @return Returns HTTP_CLIENT_OK on success, corresponding error code on failure
 */
extern int qcm_m_http_client_perform(struct http_client* client);

/**
 * @brief Get HTTP Client Information
 * @param client HTTP client handle
 * @param tag Information type (HTTPCLIENT_GETINFO_*)
 * @param result Pointer to store the result
 * @return Returns HTTP_CLIENT_OK on success, corresponding error code on failure
 */
extern int qcm_m_http_client_getinfo(struct http_client* client, int tag, void* result);

/**
 * @brief Actively Stop HTTP Client Operation
 * @param client HTTP client handle
 */
extern void qcm_m_http_client_stop(struct http_client* client);

#endif /* __QCM_M_HTTP_CLIENT_H__ */
