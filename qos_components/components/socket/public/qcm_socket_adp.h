/*****************************************************************/ /**
* @file qcm_socket_adp.h
* @brief The purpose of this file is to make upper-level applications unaware of socket layer API interfaces as much as possible
* @author larson.li@quectel.com
* @date 2023-07-25
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2023-07-25 <td>1.0 <td>Larson.Li <td> Initial version
* </table>
**********************************************************************/
#ifndef __QOSA_SOCKET_ADP_H__
#define __QOSA_SOCKET_ADP_H__
#include "qcm_proj_config.h"
#include "qosa_def.h"
#include "qosa_sys.h"
#if (defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#include "qosa_ip_addr.h"
#endif
//#include "qosa_datacall.h"
#include "qosa_sockets.h"
#include "qcm_proj_config.h"
#ifdef CONFIG_QCM_VTLS_FUNC
#include "qcm_vtls.h"
#include "qcm_vtls_cfg.h"
#endif

//Temporary solution to cross-reference of header files, to be optimized later
typedef struct qosa_ip_addr qosa_ip_addr_t;

/**
 * @enum qcm_socket_state
 * @brief Internal socket state
 */
typedef enum
{
    QCM_SOC_STATE_NONE = 0,
    QCM_SOC_STATE_CONNECTING,                  /*!< Socket is in connecting state */
    QCM_SOC_STATE_CONNECT,                     /*!< Socket connection successful */
    QCM_SOC_STATE_LISTEN,                      /*!< Socket is in listening state */
    QCM_SOC_STATE_CLOSING,                     /*!< Socket encountered error and is preparing to close */
    QCM_SOC_STATE_CLOSED = QCM_SOC_STATE_NONE, /*!< Socket connection closed */
} qcm_socket_state;

/**
 * @enum qcm_socke_option_e
 * @brief Various socket option options
 */
typedef enum
{
    QCM_SOCK_LINGER_OPT = 1,    /*!< Used to set delayed close time */
    QCM_SOCK_UNREAD_OPT = 2,    /*!< Used to get unread status in buffer */
    QCM_TCP_ACK_OPT = 3,        /*!< Used to get the number of ACKs in current socket connection */
    QCM_SOCK_REUSEADDR_OPT = 4, /*!< Used to configure whether to enable port address reuse option */
    QCM_IP_TTL_OPT = 5,         /*!< Used to set TTL field, indicating the maximum number of hops allowed for IP packet forwarding in the network */
#ifdef CONFIG_QOSA_LINUX_PLATFROM_CFG
    QCM_TCP_UNACK_OPT = 6,      /*!< Used to get the number of unacknowledged ACKs in current socket connection */
#endif
    QCM_SOCK_NOSACK_OPT = 7,    /*!< Disables/Enables TCP Selective Acknowledgment (SACK). */
    QCM_SOCK_RCVTIMEO_OPT = 8,  /*!< Sets the timeout for receive operations. */
    QCM_TCP_NODELAY_OPT = 9,    /*!< Enables/Disables Nagle's algorithm to reduce latency. */
    QCM_SOCK_ERROR_OPT = 10,    /*!< Gets the pending error status of the socket. */
    QCM_SOCK_STATE_OPT = 11,    /*!< Gets the current state of the socket. */
} qcm_socke_option_e;

/**
 * @enum qcm_socket_system_option_e
 * @brief Configure system option options
 */
typedef enum
{
    QCM_TCP_KEEPALIVE_OPT = 1,       /*!< Used to configure whether to enable TCP keepalive option function */
    QCM_TCP_KEEPIDLE_OPT = 2,        /*!< Used to configure keepalive IDLE idle time */
    QCM_TCP_KEEPINTVL_OPT = 3,       /*!< Used to configure keepalive probe packet interval time */
    QCM_TCP_KEEPCNT_OPT = 4,         /*!< Used to configure the number of keepalive probe packets sent */
    QCM_TCP_WINDOWS_SCALING_OPT = 5, /*!< Used to modify TCP Windows scale factor of system protocol stack */
    QCM_TCP_RETRY_TIME_OPT = 6,      /*!< Configure TCP retransmission interval time, must be configured together with QCM_TCP_MAX_BACKOFFS */
    QCM_TCP_RETRY_COUNT_OPT = 7,     /*!< Configure TCP retransmission count, must be configured together with QCM_TCP_RETRY_TIME_OPT */
    QCM_TCP_DELAY_ACK_OPT = 8, /*!< TCP protocol layer delayed sending, preventing TCP internal delay waiting for data packet sending, but still cannot completely avoid */
    QCM_TCP_TIMEWAIT_FAST_RELEASE_OPT = 9, /*!< Control underlying TCP close time wait state fast release action */
    QCM_TCP_RECV_WINDOWS_OPT = 10,         /*!< TCP protocol layer receive window size */
    QCM_TCP_SEND_WINDOWS_OPT = 11,         /*!< TCP protocol layer send window size */
    QCM_UDP_FILTER_OPT = 12,               /*!< Configure whether to filter UDP packets, preventing unrecorded UDP packet access */
    QCM_TCP_MSS_OPT = 13,                  /*!< TCP MSS configuration */
    QCM_DNS_CACHE_OPT = 14,                /*!< DNS cache configuration */
    QCM_DNS_RETRY_TIME_OPT = 15,           /*!< DNS retry time configuration */
    QCM_DNS_RETRY_COUNT_OPT = 16,          /*!< DNS retry count configuration */
} qcm_socket_system_option_e;

/**
 * @enum qcm_sock_event_mask_opt_e
 * @brief Socket state event notification
 */
typedef enum
{
    QCM_SOCK_CONNECT_EVENT = 0x01,        /*!< TCP connection event notification */
    QCM_SOCK_READ_EVENT = 0x02,           /*!< Socket read event notification */
    QCM_SOCK_WRITE_EVENT = 0x04,          /*!< Socket write event notification */
    QCM_SOCK_ACCEPT_EVENT = 0x08,         /*!< Socket accept event notification */
    QCM_SOCK_CLOSE_EVENT = 0x10,          /*!< Socket close event notification */
    QCM_SOCK_SSL_HD_TIMEOUT_EVENT = 0x20, /*!< SSL handshake failure */
    QCM_SOCK_SENDENDACK_EVENT = 0x40,     /*!< TCP ACK response event notification */
    QCM_SOCK_NET_DOWN_EVENT = 0x80,       /*!< Network initiated disconnection event notification */

    QCM_SOCK_MAX
} qcm_sock_event_mask_opt_e;

/**
 * @enum qcm_sock_err_code
 * @brief Socket internal errors
 */
typedef enum
{
    QCM_SOCK_SUCCESS = 0,          /*!< Socket operation successful */
    QCM_SOCK_OUT_MEM = -1,         /*!< Memory allocation error */
    QCM_SOCK_WODBLOCK = -2,        /*!< Wouldblock error */
    QCM_SOCK_INVALID_PARAM = -3,   /*!< Invalid parameter */
    QCM_SOCK_DNS_FAIL = -4,        /*!< DNS resolution error */
    QCM_SOCK_NOT_ALLOW = -5,       /*!< Operation not allowed */
    QCM_SOCK_BROKEN = -6,          /*!< */
    QCM_SOCK_INTERNAL_ERROR = -7,  /*!< Internal error */
    QCM_SOCK_TIMEROUT = -8,        /*!< Timeout error */
    QCM_SOCK_PDP_NO_ACTIVE = -9,   /*!< PDP not activated */
#ifdef CONFIG_QCM_VTLS_FUNC
    QCM_SOCK_SSL_INIT_ERROR = -9,  /*!< SSL initialization error */
    QCM_SOCK_SSL_CONN_ERROR = -10, /*!< SSL connection error */
#endif
} qcm_sock_err_code;

/**
 * @struct qcm_socket_linger_t
 * @brief Socket linger configuration time structure
 */
typedef struct
{
    int on_off;     /*!< option on/off */
    int linger_val; /*!< linger time in seconds */
} qcm_socket_linger_t;

/** Same as SOCK_STREAM */
#define QCM_SOCK_STREAM               1
/** Same as SOCK_DGRAM */
#define QCM_SOCK_DGRAM                2
/** Same as SOCK_RAW */
#define QCM_SOCK_RAW                  3

/** Same as AF_UNSPEC */
#define QCM_AF_UNSPEC                 0
/** Same as AF_INET */
#define QCM_AF_INET                   2
/** Same as PF_INET */
#define QCM_PF_INET                   QCM_AF_INET
/** Same as AF_INET6 */
#define QCM_AF_INET6                  10
/** Same as PF_INET6 */
#define QCM_PF_INET6                  QCM_AF_INET6

/** Same as IPPROTO_IP */
#define QCM_IP_PROTOCOL               0
/** Same as IPPROTO_TCP */
#define QCM_TCP_PROTOCOL              6
/** Same as IPPROTO_UDP */
#define QCM_UDP_PROTOCOL              17

/** Read channel, actually does not send any data packets */
#define QCM_SHUT_RD                   0
/** Write channel, actually sends FIN packets */
#define QCM_SHUT_WR                   1
/** Read-write channel, actually sends FIN packets, also releases socket resources, generally not used */
#define QCM_SHUT_RDWR                 2

/** TCP windows minimum value */
#define QCM_TCPIP_TCP_WINDOW_SIZE_MIN 16
/** TCP windows maximum value */
#define QCM_TCPIP_TCP_WINDOW_SIZE_MAX 100

#if (defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#define QCM_TCPIP_SOCKET_CALL_BACK_ENABLE 0
#else
#define QCM_TCPIP_SOCKET_CALL_BACK_ENABLE 1
#endif
/** Socket callback function switch */
#if defined(CONFIG_QOSA_ASR1903_PLATFORM_FUNC) || defined(CONFIG_QOSA_ASR160X_PLATFORM_FUNC) || defined(CONFIG_QOSA_ASR1805_PLATFORM_FUNC)
#define qcm_socket_with_call_back(family, type, protocol, qcm_socket_tcpack_callback) lwip_socket_with_callback(family, type, protocol, qcm_socket_tcpack_callback)
#elif (defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#define qcm_socket_with_call_back(family, type, protocol, qcm_socket_tcpack_callback) 
#else /* CONFIG_QOSA_ASR1903_PLATFORM_FUNC || CONFIG_QOSA_ASR160X_PLATFORM_FUNC || CONFIG_QOSA_ASR1805_PLATFORM_FUNC */
#define qcm_socket_with_call_back(family, type, protocol, qcm_socket_tcpack_callback) lwip_socket_with_call_back(family, type, protocol, qcm_socket_tcpack_callback)
#endif /* CONFIG_QOSA_ASR1903_PLATFORM_FUNC || CONFIG_QOSA_ASR160X_PLATFORM_FUNC || CONFIG_QOSA_ASR1805_PLATFORM_FUNC */

/** Event notification callback function interface */
typedef void (*qcm_socket_event_cb)(int sock_hndl, int event, int code, void *user_argv);

/**
 * @brief This function can create a socket and bind the local address corresponding to SIMID and PDPCID
 * Note that the created socket handle is in blocking mode by default
 *
 * @param[in] qosa_uint8_t simid
 *          - SIM card sequence number expected for socket attachment
 *
 * @param[in] qosa_uint8_t pdpcid
 *          - PDP scenario ID expected for socket attachment
 *
 * @param[in] int family
 *          - Type information for socket connection to be created
 *          - QCM_AF_INET  QCM_AF_INET6
 *
 * @param[in] int type
 *          - Socket type to be created
 *          - QCM_SOCK_DGRAM QCM_SOCK_STREAM QCM_SOCK_RAW
 *
 * @param[in] int protocol
 *          - Socket protocol connection type, usually configured as 0 IP type
 *          - QCM_IP_PROTOCOL QCM_TCP_PROTOCOL QCM_UDP_PROTOCOL
 *
 * @param[in] int local_port
 *          - Local port bound when creating socket
 *
 * @param[in] qosa_bool_t block
 *          - QOSA_TRUE configures connect as blocking interface
 *          - QOSA_FALSE configures as non-blocking interface, need to call qcm_socket_register_event to register event notification function before configuration
 *            otherwise event notifications may be lost
 *
 * @return int
 *          - Returns corresponding socket handle
 *          - Returns negative value on failure, refer to qcm_sock_err_code for specific information
 */
#if (defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
int qcm_socket_create(int family, int type, int protocol, int local_port, qosa_bool_t block);
#else
int qcm_socket_create(qosa_uint8_t simid, qosa_uint8_t pdpcid, int family, int type, int protocol, int local_port, qosa_bool_t block);
#endif
/**
 * @brief This function is used to perform socket connection action, should be executed after successful DNS resolution
 *
 * @param[in] int s
 *          - Socket handle to operate
 *
 * @param[in] qosa_ip_addr_t * remote_ip
 *          - Remote server IP address
 *
 * @param[in] int remote_port
 *          - Remote port number to connect to
 *
 * @return int
 *       - Returns 0 for successful execution
 *       - Returns negative value on failure, refer to qcm_sock_err_code for specific information
 */
int qcm_socket_connect(int s, qosa_ip_addr_t *remote_ip, int remote_port);

/**
 * @brief Used to operate asynchronous mode socket underlying focus event notification
 *
 * @param[in] int s
 *          - Socket handle to operate
 *
 * @param[in] qcm_sock_event_mask_opt_e event_mask
 *          - Event type to be monitored by user layer
 *
 * @param[in] qcm_socket_event_cb func
 *          - Corresponding user layer registered CB callback function
 *
 * @return int
 *       - Returns 0 on success
 *       - Returns negative value on failure, refer to qcm_sock_err_code for specific information
 */
int qcm_socket_register_event(int s, int event_mask, qcm_socket_event_cb func, void *argv);

/**
 * @brief Read function, used for data reading interface
 *
 * @param[in] int s
 *          - Socket handle to operate
 *
 * @param[out] char * data
 *          - Data reading storage address
 *
 * @param[in] int sz
 *          - Data size to read, data parameter space should be greater than sz
 *
 * @return int
 *       - Returns actual read length greater than 0 on success
 *       - Returns negative value on failure, refer to qcm_sock_err_code for specific information
 */
int qcm_socket_read(int s, char *data, int sz);

/**
 * @brief Send function to send data
 *
 * @param[in] int s
 *          - Socket handle to operate
 *
 * @param[out] char * data
 *          - Data to be sent
 *
 * @param[in] int sz
 *          - Data size to be sent
 *
 * @return int
 *       - Returns actual sent size on success
 *       - Returns negative value on failure, refer to qcm_sock_err_code for specific information
 *       - If actual length returned is less than sz size, should call send function again to send data to trigger underlying wouldblock status report
 */
int qcm_socket_send(int s, char *data, int sz);

/**
 * @brief Socket listen function
 *
 * @param[in] int s
 *          - Socket handle to operate
 *
 * @param[in] int backlog
 *          - Maximum number of accepted clients
 *
 * @return int
 *       - Returns 0 on successful execution
 *       - Returns negative value on failure, refer to qcm_sock_err_code for specific information
 */
int qcm_socket_listen(int s, int backlog);

/**
 * @brief Socket accept function to accept socket connection requests, newly created socket connection function will set whether it's a blocking IO socket according to the listening socket block parameter configuration
 *  Function needs to be executed after calling qcm_socket_listen
 *
 * @param[in] int s
 *          - Socket handle to operate
 *
 * @param[out] qosa_ip_addr_t * remote_addr
 *          - Returns IP address of accepted new connection
 *
 * @param[out] int * remote_port
 *          - Returns destination port of accepted new connection
 *
 * @param[out] qosa_ip_addr_t * local_addr
 *          - Returns local IP address
 *
 * @param[out] int * local_port
 *          - Returns local port
 *
 * @return int
 *       - Returns handle corresponding to new connection
 *       - Returns negative value on failure, refer to qcm_sock_err_code for specific information
 */
int qcm_socket_accept(int s, qosa_ip_addr_t *remote_addr, int *remote_port, qosa_ip_addr_t *local_addr, int *local_port);

/**
 * @brief Socket close function, internally blocks regardless of external configuration
 *  If you want to change close return event, need to configure socket linger option time
 *
 * @param[in] int s
 *          - Socket handle to operate
 *
 * @return int
 *       - Returns 0 on successful execution
 *       - Returns negative value on failure, refer to qcm_sock_err_code for specific information
 */
int qcm_socket_close(int s);

/**
 * @brief Send data to specified destination address
 *
 * @param[in] int s
 *          - Socket file descriptor already established for communication
 *
 * @param[out] qosa_ip_addr_t *remote_addr
 *          - IP address information of the other party's data to be sent
 *
 * @param[in] qosa_uint16_t remote_port
 *          - Destination port to be sent
 *
 * @param[in] char * data
 *          - Data content to be sent
 *
 * @param[in] int sz
 *          - Data length size to be sent
 *
 * @return int
 *       - Returns actual sent data length size on success
 *       - Returns negative value on failure, refer to qcm_sock_err_code for specific information
 */
int qcm_socket_sendto(int s, qosa_ip_addr_t *remote_addr, qosa_uint16_t remote_port, char *data, int sz);

/** Receive data from socket and store sender's address information in specified location
 *
 * @param[in] int s
 *          - sockfd: socket file descriptor already established for communication
 *
 * @param[out] qosa_ip_addr_t *remote_addr
 *          - IP address information of the other party's received data
 *
 * @param[out] qosa_uint16_t * remote_port
 *          - Remote port of received data
 *
 * @param[out] char * data
 *          - Pointer to buffer for receiving data
 *
 * @param[in] int sz
 *          - Buffer size (in bytes)
 *
 * @return int
 *       - Returns number of bytes received on success
 *       - Returns negative value on failure, refer to qcm_sock_err_code for specific information
 */
int qcm_socket_recvfrom(int s, qosa_ip_addr_t *remote_addr, qosa_uint16_t *remote_port, char *data, int sz);

/**
 * @brief Set socket options
 *
 * @param[in] int s
 *          - Socket file descriptor already established for communication
 *
 * @param[in] int opt
 *          - Option name, used to specify the option to be set
 *
 * @param[in] void * value
 *          - Pointer to buffer storing option value
 *
 * @return int
 *       - Returns 0 on success
 *       - Returns negative value on failure, refer to qcm_sock_err_code for specific information
 */
int qcm_socket_set_opt(int s, qcm_socke_option_e opt, void *value);

/**
 * @brief Get socket options interface
 *
 * @param[in] int s
 *          - Socket file descriptor already established for communication
 *
 * @param[in] int opt
 *          - Option name, used to specify the option to be set
 *
 * @param[in/out] void * value
 *          - Pointer to buffer storing option value
 *
 * @return int
 *       - Returns 0 on success
 *       - Returns negative value on failure, refer to qcm_sock_err_code for specific information
 */
int qcm_socket_get_opt(int s, qcm_socke_option_e opt, void *value);

#ifdef CONFIG_QCM_VTLS_FUNC
/**
 * @brief Configure SSL config information to be used
 *
 * @param[in] int s
 *          - Socket file descriptor already established for communication
 *
 * @param[in] qcm_ssl_config_t * ssl_ptr
 *          - Pointer to SSL CONFIG configuration file
 *
 * @parsm[in] qosa_bool_t block
 *          - Configure whether SSL handshake session is blocking or non-blocking
 *
 * @return int
 *       - Returns 0 on success
 *       - Returns negative value on failure, refer to qcm_sock_err_code for specific information
 */
qcm_sock_err_code qcm_socket_ssl_config(int s, qcm_ssl_config_t *ssl_ptr, const char *hostname, qosa_bool_t block);

/**
 * @brief SSL connection authentication
 *
 * Function blocking and non-blocking mode is controlled by block parameter configured in qcm_socket_ssl_config
 *
 * @param[in] int s
 *          - Socket file descriptor already established for communication
 *
 * @return int
 *       - Returns 0 on successful function execution
 *       - Returns negative value on failure, refer to qcm_sock_err_code for specific information
 */
qcm_sock_err_code qcm_socket_ssl_connect(int s);

/**
 * @brief Get whether SSL config has been initialized
 */
qosa_bool_t qcm_socket_ssl_is_init_already(int s);

/**
 * @brief Only release SSL resources without closing socket socket handle
 *
 * @param[in] int s
 *          - Socket file descriptor already established for communication
 *
 * @return int
 *       - Returns 0 on successful function execution
 *       - Returns negative value on failure, refer to qcm_sock_err_code for specific information
 */
qcm_sock_err_code qcm_socket_ssl_clean(int s);
#endif /* CONFIG_QCM_VTLS_FUNC */

/**
 * @brief Socket shutdown function, used to close RD WR channels
 *
 * Original socket shutdown function returns 0 on successful execution, returns -1 on failure and sets global errno value
 * errno code
 *  1. EBADF socket is not a valid socket descriptor. This usually occurs when socket has not been opened or has been closed.
 *  2. EINVAL: how parameter is not set to a valid value.
 *  3. ENOBUFS: Not enough system resources to complete this call. This usually occurs when system resources are tight.
 *  4. ENOTCONN: socket is not connected. If trying to close a socket that is not connected, this error will be returned.
 *  5. ENOTSOCK: descriptor points to a file, not a socket.
 *
 * Based on this, we only need to pay attention to ENOTCONN error value
 *
 * In lwip, if currently in write process, returns error code EINPROGRESS, if connection not in returns ENOTCONN, otherwise returns OK
 *
 * @param[in] int s
 *          - Socket action to operate
 *
 * @param[in] int how
 *          - Corresponding operation direction QCM_SHUT_RD QCM_SHUT_WR QCM_SHUT_RDWR
 *
 * @return int
 *        - Function ends returning qcm_sock_err_code error code
 */
int qcm_socket_shutdown(int s, int how);

/**
 * @brief Set system option options
 */
int qcm_socket_set_system_opt(qcm_socket_system_option_e opt, void *value);

/**
 * @brief Get system option options
 */
int qcm_socket_get_system_opt(qcm_socket_system_option_e opt, void *value);

/*
* @brief Simply determine if input string is IPV4, IPV6, DNS type
*
* @return int
*       - Returns -1 flagging execution failure
*       - Returns OSA_AF_INET indicating IPv4
*       - Returns OSA_AF_INET6 indicating IPv6
*       - Returns 0 indicating unknown type
*
*/
int qcm_socket_checkip_is_ip46(char *ip_string);

/**
 * @brief Network IP to dotted string conversion
 * @param[in] src Network byte order IP address
 * @param[out] dst Dotted string IP address
 * @param[in] size Dotted string IP address buffer size
 * @note Input parameter is network byte order, output parameter is dotted string, such as 192.168.1.100
 * This function solves the problem of inconsistent format size with previous platform, characters are converted to uppercase display
 */
const char *qcm_inet_ntop(int af, const void *src, char *dst, int size);

/**
 * @brief Get TCP last close reason
 *
 * @param[in] int s
 *          - Socket handle to operate
 *
 * @return int Returns specific close reason
 *
 */
int qcm_socket_get_last_close_event(int s);

#endif /* __QOSA_SOCKET_H__ */
