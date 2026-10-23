/*****************************************************************/ /**
* @file ql_socket.c
* @brief The encapsulation of this file should make the upper application layer as unaware as possible of the differences in the standard socket API interface layer.
* The header files and other resources called by this layer are all from the system, which can reduce modifications due to platform differences when replacing the platform later.
*
* @author larson.li@quectel.com
* @date 2023-07-25
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description
* <tr><td>2023-07-25 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#include "qcm_socket_adp.h"
#include "qosa_def.h"
#include "qosa_log.h"
#include "qosa_sys.h"
#include "qosa_queue_list.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#include "qosa_datacall.h"
#endif
#include "qcm_proj_config.h"
#include "qosa_system_config.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#include "qosa_led_event.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qosa_sockets.h"

#include "qosa_event_notify.h"
#include "qcm_proj_config.h"
#ifdef CONFIG_QCM_VTLS_FUNC
#include "qcm_vtls.h"
#include "qcm_vtls_cfg.h"
#endif /* CONFIG_QCM_VTLS_FUNC */

#define QOS_LOG_TAG                LOG_TAG_SOCK_API
#define SOCKET_MAX_CONNECTED       20
#define SOCKET_OFFSET              1
#define SOCKET_ARRAY_CONNECTED_CNT SOCKET_MAX_CONNECTED + SOCKET_OFFSET

typedef enum
{
    QCM_SOCK_MONITOR_RD = 0x01,
    QCM_SOCK_MONITOR_WR = 0x02,

    QCM_SOCK_MONITOR_MAX
} qcm_sock_monitor_event_type_e;

/**
 * @brief Globally save configuration parameters to enable TCP keep-alive for all socket connections using the socket adapter
 */
typedef struct
{
    qosa_uint8_t  kalive_onoff;          /*!< Set whether to enable TCP keep-alive*/
    qosa_uint32_t kalive_idle;           /*!< Set the idle time after which to send keepalive probes*/
    qosa_uint32_t kalive_interval;       /*!< Keepalive probe interval*/
    qosa_uint32_t kalive_cnt;            /*!< Maximum number of keepalive probe transmissions, close connection if no response*/
    qosa_uint8_t  scale;                 /*!< system windows scale */
    qosa_uint32_t retry_time;            /*!< Configure the TCP retransmission interval, must be configured together with QCM_TCP_MAX_BACKOFFS*/
    qosa_uint8_t  retry_count;           /*!< Configure the number of TCP retransmissions, must be configured together with QCM_TCP_RETRY_TIME_OPT*/
    qosa_uint8_t  delay_ack;             /*!< TCP protocol layer delayed sending, prevents TCP internal delayed waiting from grouping data for transmission, but still cannot completely avoid it.*/
    qosa_uint8_t  fast_release_timewait; /*!< Control the underlying layer to perform a fast release action for the TIME_WAIT state after a TCP close.*/
    qosa_uint32_t tcp_recv_windows_size; /*!< TCP protocol layer receive window size*/
    qosa_uint32_t tcp_send_windows_size; /*!< TCP protocol layer send serial port size*/
    qosa_uint8_t  udp_filter;            /*!< Whether to enable the underlying UDP filtering configuration of lwip, allowing the UDP server to support*/
    qosa_uint16_t tcp_mss;               /*!< TCP MSS Configuration*/
    qosa_uint8_t  dns_cache;             /*!< Whether to enable DNS caching; after enabling, DNS resolution results will be cached.*/
    qosa_uint16_t dns_retry_time;        /*!< DNS resolution retry interval*/
    qosa_uint8_t  dns_retry_cnt;         /*!< Number of DNS resolution retries*/
} qcm_socket_system_opt_cfg_t;

#ifdef CONFIG_QCM_VTLS_FUNC
typedef enum
{
    QCM_SOC_SSL_STATE_NONE = 0,
    QCM_SOC_SSL_STATE_INIT,
    QCM_SOC_SSL_STATE_CONNECTING,
    QCM_SOC_SSL_STATE_CONNECT
} qcm_ssl_conn_state_e;
#endif /* CONFIG_QCM_VTLS_FUNC */

typedef struct
{
    int                 num; /*!< The socket fd handle given to the user is actually an array index number.*/
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
    int                 pdpcid;
    int                 sim_id;
#endif
    int                 socket_fd;  /*!< The actual socket file descriptor handle*/
    int                 family;     /*!< Type information for creating a socket connection QCM_AF_INET  QCM_AF_INET6*/
    int                 type;       /*!< The created socket type QCM_SOCK_DGRAM QCM_SOCK_STREAM QCM_SOCK_RAW*/
    qcm_socket_state    state;      /*!< Current TCP connection status*/
    qosa_ip_addr_t      local_addr; /*!< IPv6 maximum length is 48*/
    int                 local_port;
    int                 remote_port;
    int                 event_mask;        /*!< Event notification mask bits*/
    qosa_bool_t         block;             /*!< Whether to block*/
    int                 last_error;        /*!< The last error value recorded for the current socket*/
    void               *user_argv;         /*!< User registration CB carries user parameter information*/
    qcm_socket_event_cb event_cb;          /*!< User registration event notification function*/
    qosa_bool_t         report_net_down;   /*!< Whether the event has already been reported when the network is disconnected*/
#ifdef CONFIG_QCM_VTLS_FUNC
    qcm_ssl_conn_state_e    ssl_state;     /*!< SSL Connection Status Management*/
    qosa_bool_t             ssl_enable;    /*!< Whether to enable SSL connection action*/
    qcm_ssl_connect_data_t *ssl_ctx;       /*!< SSL connection status handle*/
    qosa_bool_t             ssl_block;     /*!< Whether SSL performs SSL connection action in a blocking manner*/
    qosa_timer_t            ssl_wait_time; /*!< SSL handshake timeout timer pointer*/
#endif                                     /* CONFIG_QCM_VTLS_FUNC */
} qcm_socket_t;

typedef struct
{
    int          fd;         /*!< Corresponds to the real file descriptor handle of the socket*/
    unsigned int event_mask; /*!< Corresponding event status bit*/
    void        *user_parm;  /*!< sock pointer registered when creating the socket*/
} qcm_socket_Event;

static qosa_task_t   g_sock_monitor_task;
static qosa_msgq_t   g_sock_monitor_msgq;
static qcm_socket_t *g_socket_array[SOCKET_ARRAY_CONNECTED_CNT];

//Socket core task startup status: does not start by default upon boot to save memory consumption.
static qosa_bool_t g_sock_monitor_start_falg = QOSA_FALSE;

/** Used to store system configuration parameters*/
static qcm_socket_system_opt_cfg_t g_socket_system_cfg = {
    .kalive_onoff = 0,
    .kalive_idle = 60,
    .kalive_interval = 25,
    .kalive_cnt = 3,
    .scale = 1,
#ifdef CONFIG_UNIRTOS_QICFG_TCP_RETRANSCFG_FOR_QUALCOMM
    .retry_time = 6000,
    .retry_count = 5,
#else  /* CONFIG_UNIRTOS_QICFG_TCP_RETRANSCFG_FOR_QUALCOMM */
    .retry_time = 6,
    .retry_count = 5,
#endif /* CONFIG_UNIRTOS_QICFG_TCP_RETRANSCFG_FOR_QUALCOMM */
#if defined(CONFIG_QOSA_QCX216_PLATFORM_FUNC) || defined(CONFIG_QOSA_QCX217_PLATFORM_FUNC) || defined(CONFIG_QOSA_EIGEN718_PLATFORM_FUNC)
    .delay_ack = 0,
#else  /* CONFIG_QOSA_QCX216_PLATFORM_FUNC || CONFIG_QOSA_QCX217_PLATFORM_FUNC || defined(CONFIG_QOSA_EIGEN718_PLATFORM_FUNC) */
    .delay_ack = 1,
#endif /* CONFIG_QOSA_QCX216_PLATFORM_FUNC || CONFIG_QOSA_QCX217_PLATFORM_FUNC || defined(CONFIG_QOSA_EIGEN718_PLATFORM_FUNC) */
    .fast_release_timewait = 0,
    .tcp_recv_windows_size = 16384,
    .tcp_send_windows_size = 16384,
    .udp_filter = 0,
    .tcp_mss = 1360,
    .dns_cache = 1,
    .dns_retry_time = 200,
    .dns_retry_cnt = 2,
};

static fd_set g_readset;
static fd_set g_writeset;
static int    g_sockets_cnt = 0;
static int    g_fd_max = 0;

static void qcm_socket_monitor_init_check(void);

void qcm_socket_set_retrains_cnt(qosa_uint32_t retrains_cnt);
void qcm_socket_set_retrains_period(qosa_uint32_t retrains_period);
void qcm_socket_set_twrecycle(qosa_uint32_t fast_recyle);
void qcm_socket_get_twrecycle(qosa_uint32_t *fast_recyle);
void qcm_socket_set_udp_filter_mode(qosa_uint32_t mode);
void qcm_socket_get_udp_filter_mode(qosa_uint32_t *mode);
void qcm_socket_get_retrains_cnt(qosa_uint32_t *retrains_cnt);
void qcm_socket_get_retrains_period(qosa_uint32_t *retrains_period);
void qcm_socket_set_recv_windows(qosa_uint32_t tcp_recv_windows_size);
void qcm_socket_get_recv_windows(qosa_uint32_t *tcp_recv_windows_size);
void qcm_socket_set_send_windows(qosa_uint32_t tcp_send_windows_size);
void qcm_socket_get_send_windows(qosa_uint32_t *tcp_send_windows_size);
void qcm_socket_set_windows_scale(qosa_uint32_t scale);
void qcm_socket_get_windows_scale(qosa_uint32_t *scale);
void qcm_socket_set_tcp_mss(qosa_uint32_t tcp_mss);
void qcm_socket_get_tcp_mss(qosa_uint32_t *tcp_mss);
void qcm_socket_set_platform_opt(qcm_socket_t *sock);
void qcm_socket_set_dns_cache_opt(qosa_uint8_t dns_cache);
void qcm_socket_set_dns_retry_opt(qosa_uint16_t dns_retry_times, qosa_uint8_t dns_retry_cnt);

/*
* @brief Simplely determine whether the input string is of type IPV4, IPV6, or DNS.
*
* @return int
*       - Returns -1 to indicate execution failure
*       - Returns OSA_AF_INET indicating IPv4
*       - Returns OSA_AF_INET6 to indicate IPv6
*       - Returns 0 to indicate an unknown type
*
*/
int qcm_socket_checkip_is_ip46(char *ip_string)
{
    if (ip_string == QOSA_NULL)
    {
        //Error code -1: Null pointer
        return -1;
    }

    struct in_addr  addr4;
    struct in6_addr addr6;

    if (inet_pton(AF_INET, ip_string, &addr4) == 1)
    {
        //is a valid IPv4 address
        return AF_INET;
    }
    else if (inet_pton(AF_INET6, ip_string, &addr6) == 1)
    {
        return AF_INET6;
    }

    //Unknown type
    return 0;
}

/**
 * @brief Obtain new socket space
 *
 * @return int
 *       - Returns the actual upper-level fd on success
 *       - Returns -1 on failure
 */
static int qcm_socket_get_new_fd(void)
{
    int s = 0;
    for (s = SOCKET_OFFSET; s < SOCKET_ARRAY_CONNECTED_CNT; s++)
    {
        if (g_socket_array[s] == QOSA_NULL)
        {
            g_socket_array[s] = (qcm_socket_t *)qosa_malloc(sizeof(qcm_socket_t));
            if (g_socket_array[s] == QOSA_NULL)
            {
                QLOGE("malloc faliled");
                return -1;
            }
            qosa_memset(g_socket_array[s], 0, sizeof(qcm_socket_t));
            g_socket_array[s]->num = s;
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#if !(defined(CONFIG_QOSA_OEM_BSQ_FUNC) || defined(CONFIG_QOSA_OEM_ALLIED_FUNC) || defined(CONFIG_QOSA_OEM_GENUS_FUNC) || defined(CONFIG_QOSA_OEM_LT_FUNC))
            QOSA_LED_UPDATE_SOCKET_STATUS(0, s, 1);
#endif /* !(CONFIG_QOSA_OEM_BSQ_FUNC || CONFIG_QOSA_OEM_ALLIED_FUNC || CONFIG_QOSA_OEM_GENUS_FUNC) || CONFIG_QOSA_OEM_LT_FUNC*/
#endif
            return s;
        }
    }
    return -1;
}

/**
 * @brief Check if the socket array pointer exists
 *
 * @param[in] int s
 *          - Corresponding handle information for the operation
 *
 * @return qosa_bool_t
 *        - Returns OSA_TRUE
 *        - Returns OSA_FALSE if it does not exist
 *
 */
static qosa_bool_t qcm_socket_check_exist(int s)
{
    qcm_socket_monitor_init_check();
    if (g_socket_array[s] == QOSA_NULL)
    {
        return QOSA_FALSE;
    }
    return QOSA_TRUE;
}

/**
 * @brief Release the corresponding space information after the socket is closed.
 *
 * @param[in] int s
 *          - Corresponding handle information for the operation
 */
static void qcm_socket_free(int s)
{
    if (qcm_socket_check_exist(s) == QOSA_TRUE)
    {
        qosa_free(g_socket_array[s]);
        g_socket_array[s] = QOSA_NULL;
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#if !(defined(CONFIG_QOSA_OEM_BSQ_FUNC) || defined(CONFIG_QOSA_OEM_ALLIED_FUNC) || defined(CONFIG_QOSA_OEM_GENUS_FUNC) || defined(CONFIG_QOSA_OEM_LT_FUNC))
        QOSA_LED_UPDATE_SOCKET_STATUS(0, s, 0);
#endif /* !(CONFIG_QOSA_OEM_BSQ_FUNC || CONFIG_QOSA_OEM_ALLIED_FUNC || CONFIG_QOSA_OEM_GENUS_FUNC) || CONFIG_QOSA_OEM_LT_FUNC */
#endif    
    }
}

/**
 * @brief Retrieve the sock structure information corresponding to the handle
 *
 * @param[in] int s
 *          - Corresponding handle information for the operation
 *
 * @return qcm_socket_t *
 *       - Returns the corresponding sock structure pointer on success
 *       - Returns OSA_NULL on failure
 */
static qcm_socket_t *qcm_socket_get_prt(int s)
{
    qcm_socket_monitor_init_check();
    if (s >= SOCKET_OFFSET && s < SOCKET_ARRAY_CONNECTED_CNT && qcm_socket_check_exist(s) == QOSA_TRUE)
    {
        return g_socket_array[s];
    }
    return QOSA_NULL;
}

static qcm_socket_t *qcm_socket_get_ptr_with_socktfd(int socketfd)
{
    int s = 0;
    qcm_socket_monitor_init_check();
    for (s = SOCKET_OFFSET; s < SOCKET_ARRAY_CONNECTED_CNT; s++)
    {
        if (g_socket_array[s] != QOSA_NULL)
        {
            if (g_socket_array[s]->socket_fd == socketfd)
            {
                return g_socket_array[s];
            }
        }
    }
    return QOSA_NULL;
}

static void ql_apptcpip_soc_callback_func(u16_t fd, u32_t event_mask, void *sock_cb_user_data)
{
    qcm_socket_Event s_event = {0};

    s_event.fd = fd;
    s_event.event_mask = event_mask;
    s_event.user_parm = sock_cb_user_data;
    QLOGV("fd=%d mask=%d", fd, event_mask);
    //Internal resource allocation and release
    qosa_msgq_release(g_sock_monitor_msgq, sizeof(qcm_socket_Event), (qosa_uint8_t *)&s_event, QOSA_NO_WAIT);
}

/**
 * @brief Add the corresponding socket listening event type
 * Both synchronous and asynchronous modes can set corresponding listening events
 *
 * @param[in] qcm_socket_t * sock
 *          - Corresponding handle information for the operation
 *
 * @param[in] qcm_sock_monitor_event_type_e monitor_evt
 *          - Type of event to be added for listening
 *
 */
static void qcm_socket_add_monitor(qcm_socket_t *sock, qcm_sock_monitor_event_type_e monitor_evt)
{
    qosa_uint32_t new_mask = 0;
    int           err = 0;
    int           errno_value = 0;
    int           sock_cnt = 0;
    sock_cnt = g_sockets_cnt;
    if (monitor_evt == QCM_SOCK_MONITOR_RD)
    {
        if (!FD_ISSET(sock->socket_fd, &g_readset))
        {
            FD_SET(sock->socket_fd, &g_readset);
            g_sockets_cnt++;
        }
    }
    if (monitor_evt == QCM_SOCK_MONITOR_WR)
    {
        if (!FD_ISSET(sock->socket_fd, &g_writeset))
        {
            FD_SET(sock->socket_fd, &g_writeset);
            g_sockets_cnt++;
        }
    }

    if (FD_ISSET(sock->socket_fd, &g_readset))
    {
        new_mask |= QL_DS_READ_EVENT;
    }
    if (FD_ISSET(sock->socket_fd, &g_writeset))
    {
        new_mask |= QL_DS_WRITE_EVENT;
    }

    err = unir_async_select_mask(sock->socket_fd, new_mask);
    if (err != 0)
    {
        QLOGE("err = %d errno_value=%d", err, errno_value);
        sock->event_cb(sock->socket_fd, QCM_SOCK_CLOSE_EVENT, 0, sock->user_argv);
    }

    QLOGV("...sock_cnt=%d new_mask=%x monitor_evt=%x", sock_cnt, new_mask, monitor_evt);
}

/**
 * @brief Removes the event types to be monitored, opposite to the function of qcm_socket_add_monitor
 *
 * @param[in] qcm_socket_t * sock
 *          - Corresponding handle information for the operation
 *
 * @param[in] qcm_sock_monitor_event_type_e monitor_evt
 *          - Event type to be removed
 *
 */
static void qcm_socket_remove_monitor(qcm_socket_t *sock, qcm_sock_monitor_event_type_e monitor_evt)
{
    QLOGV("...monitor_evt=%d", monitor_evt);

    if (monitor_evt == QCM_SOCK_MONITOR_RD)
    {
        if (FD_ISSET(sock->socket_fd, &g_readset))
        {
            QLOGV("...");
            FD_CLR(sock->socket_fd, &g_readset);
            g_sockets_cnt--;
        }
    }
    if (monitor_evt == QCM_SOCK_MONITOR_WR)
    {
        if (FD_ISSET(sock->socket_fd, &g_writeset))
        {
            QLOGV("...");
            FD_CLR(sock->socket_fd, &g_writeset);
            g_sockets_cnt--;
        }
    }
}

#if QCM_TCPIP_SOCKET_CALL_BACK_ENABLE
void qcm_socket_tcpack_callback(int sockfd, int evt, u16_t len)
{
    qcm_socket_t *sock = QOSA_NULL;
    QLOGV("Invalid sockfd or event type: sockfd=%d, evt=%d , len=%d\n", sockfd, evt, len);
    if (sockfd < 0 || NETCONN_EVT_SENDACKED != evt || len == 0)
    {
        QLOGE("Invalid sockfd or event type: sockfd=%d, evt=%d\n", sockfd, evt);
        return;
    }
    sock = qcm_socket_get_ptr_with_socktfd(sockfd);
    if (sock == QOSA_NULL)
    {
        QLOGE("Failed to get socket pointer for sockfd=%d\n", sockfd);
        return;
    }
    if ((sock->event_mask & QCM_SOCK_SENDENDACK_EVENT) != 0)
    {
        if (sock->event_cb != QOSA_NULL)
        {
            QLOGV("QCM_SOCK_SENDENDACK_EVENT here sockfd=%d\n ", sockfd);
            sock->event_cb(sock->num, QCM_SOCK_SENDENDACK_EVENT, len, sock->user_argv);
        }
        else
        {
            QLOGV("No event callback registered for sockfd=%d\n", sockfd);
        }
    }
}
#endif /*QCM_TCPIP_SOCKET_CALL_BACK_ENABLE*/

/**
 * @brief This function can create a socket and simultaneously bind it to the local address corresponding to the specified SIMID and PDPCID.
 * Note that the default created socket handle is in blocking mode.
 *
 * @param[in] qosa_uint8_t simid
 *          - Corresponds to the expected SIM card sequence number to which the socket is attached
 *
 * @param[in] qosa_uint8_t pdpcid
 *          - Corresponds to the expected PDP context ID to which the socket is attached
 *
 * @param[in] int family
 *          - Type information for the socket to connect to
 *          - QCM_AF_INET  QCM_AF_INET6
 *
 * @param[in] int type
 *          - Type of socket created
 *          - QCM_SOCK_DGRAM QCM_SOCK_STREAM QCM_SOCK_RAW
 *
 * @param[in] int protocol
 *          - Socket protocol connection type, typically configured as 0 for IP type.
 *          - QCM_IP_PROTOCOL QCM_TCP_PROTOCOL QCM_UDP_PROTOCOL
 *
 * @param[in] int local_port
 *          - The local port bound when creating a socket
 *
 * @param[in] qosa_bool_t block
 *          - QOSA_TRUE configures connect as a blocking interface
 *          - QOSA_FALSE configures as a non-blocking interface; before configuration, it is necessary to call qcm_socket_register_event to register the event notification function
 *            Otherwise, event notifications may be lost.
 *
 * @return int
 *          - Returns the corresponding socket handle
 *          - Returns a negative value on failure; for specific information, refer to qcm_sock_err_code.
 */
#if (defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
int qcm_socket_create(int family, int type, int protocol, int local_port, qosa_bool_t block)
#else
int qcm_socket_create(qosa_uint8_t simid, qosa_uint8_t pdpcid, int family, int type, int protocol, int local_port, qosa_bool_t block)
#endif
{
    int                     ret = 0;
    int                     s = 0;
    int                     sockfd = -1;
    qcm_socket_t           *sock = QOSA_NULL;
    int                     reuseFlag = 1;
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
    qosa_datacall_ip_info_t info = {0};
    qosa_datacall_conn_t    conn = 0;
#endif
    int                     flags = 0;
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
    qosa_datacall_errno_e   result = QOSA_DATACALL_OK;
#endif

    qcm_socket_monitor_init_check();

#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
    //Get local IP address
    qosa_memset(&info, 0, sizeof(qosa_datacall_ip_info_t));
    conn = qosa_datacall_conn_new(simid, pdpcid, QOSA_DATACALL_CONN_TCPIP);
    result = qosa_datacall_get_ip_info(conn, &info);
    if (result != QOSA_DATACALL_OK)
    {
        QLOGE("...result=%d simid=%d pdpcid=%d", result, simid, pdpcid);
        return QCM_SOCK_PDP_NO_ACTIVE;
    }
#endif

    //Create socket
#if QCM_TCPIP_SOCKET_CALL_BACK_ENABLE
    sockfd = qcm_socket_with_call_back(family, type, protocol, qcm_socket_tcpack_callback);
#else  /*QCM_TCPIP_SOCKET_CALL_BACK_ENABLE*/
    sockfd = socket(family, type, protocol);
#endif /*QCM_TCPIP_SOCKET_CALL_BACK_ENABLE*/
    if (sockfd < 0)
    {
        QLOGE("...sockfd=%d", sockfd);
        return QCM_SOCK_INTERNAL_ERROR;
    }
    QLOGV(" sockfd=%d ", sockfd);
    //Request resources
    s = qcm_socket_get_new_fd();
    if (s <= 0)
    {
        ret = QCM_SOCK_INTERNAL_ERROR;
        goto exit;
    }

    sock = qcm_socket_get_prt(s);
    if (QOSA_NULL == sock)
    {
        ret = QCM_SOCK_INTERNAL_ERROR;
        goto exit;
    }
    sock->type = type;
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
    sock->pdpcid = pdpcid;
    sock->sim_id = simid;
#endif
    sock->socket_fd = sockfd;
    sock->family = family;
    sock->state = QCM_SOC_STATE_NONE;
    sock->local_port = local_port;

    unir_set_socket2(sock->socket_fd, ql_apptcpip_soc_callback_func, sock);

    //Set port multiplexing
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuseFlag, sizeof(int));
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &reuseFlag, sizeof(int));

    //Bind local IP addresses via simid and pdpcid
#if (defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
    if (sock->family == QCM_AF_INET)
#else
    if (sock->family == QCM_AF_INET && (info.ip_type == QOSA_PDP_IPV4 || info.ip_type == QOSA_PDP_IPV4V6))
#endif
    {
        struct sockaddr_in v4_addr;
        qosa_memset(&v4_addr, 0, sizeof(struct sockaddr_in));

        v4_addr.sin_family = sock->family;
        if (local_port != 0)
            v4_addr.sin_port = htons(local_port);
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
         v4_addr.sin_addr = info.ipv4_ip.addr.ipv4_addr;
#endif
        sock->local_addr.addr.ipv4_addr = v4_addr.sin_addr;
#if (defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
        sock->local_addr.ip_vsn = QOSA_IP_V4;
#else
        sock->local_addr.ip_vsn = QOSA_PDP_IPV4;
#endif
        ret = bind(sockfd, (struct sockaddr *)&v4_addr, sizeof(struct sockaddr_in));
    }
#if (LWIP_IPV6)
#if (defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
    else if (sock->family == QCM_AF_INET6)
#else
    else if (sock->family == QCM_AF_INET6 && (info.ip_type == QOSA_PDP_IPV6 || info.ip_type == QOSA_PDP_IPV4V6))
#endif
    {
        struct sockaddr_in6 v6_addr;
        qosa_memset(&v6_addr, 0, sizeof(struct sockaddr_in6));
        v6_addr.sin6_family = sock->family;
        if (local_port != 0)
            v6_addr.sin6_port = htons(sock->local_port);
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
        v6_addr.sin6_addr = info.ipv6_ip.addr.ipv6_addr;
#endif
        sock->local_addr.addr.ipv6_addr = v6_addr.sin6_addr;
#if (defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
        sock->local_addr.ip_vsn = QOSA_IP_V6;
#else
        sock->local_addr.ip_vsn = QOSA_PDP_IPV6;
#endif
        ret = bind(sock->socket_fd, (struct sockaddr *)&v6_addr, sizeof(v6_addr));
    }
#endif
    else
    {
        ret = QCM_SOCK_INVALID_PARAM;
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
        QLOGE("ip type error=%d", info.ip_type);
#endif
        goto exit;
    }

    if (ret < 0)
    {
        ret = QCM_SOCK_INTERNAL_ERROR;
        goto exit;
    }

    if (block == QOSA_FALSE)
    {
        flags = fcntl(sock->socket_fd, F_GETFL, 0);  //Set to no block
        fcntl(sock->socket_fd, F_SETFL, flags | O_NONBLOCK);
    }
    sock->block = block;

    if (sockfd > g_fd_max)
    {
        g_fd_max = sockfd;
    }

    return s;
exit:
    if (sockfd >= 0)
    {
        close(sockfd);
    }
    if (s > 0)
    {
        qcm_socket_free(s);
    }
    return ret;
}

/**
 * @brief This function is used to perform the socket connection action. Before executing this function, DNS resolution should be successfully completed.
 *
 * @param[in] int s
 *          - The handle corresponding to the socket to be operated on
 *
 * @param[in] qosa_ip_addr_t * remote_ip
 *          - Remote server IP address
 *
 * @param[in] int remote_port
 *          - Remote port number to connect to
 *
 * @return int
 *       - Returns 0 to indicate successful execution
 *       - Returns a negative value on failure; for specific information, refer to qcm_sock_err_code.
 */
int qcm_socket_connect(int s, qosa_ip_addr_t *remote_ip, int remote_port)
{
    int           ret = 0;
    qcm_socket_t *sock = QOSA_NULL;

    sock = qcm_socket_get_prt(s);
    if (sock == QOSA_NULL || remote_ip == QOSA_NULL)
    {
        return QCM_SOCK_INTERNAL_ERROR;
    }

    QLOGV("sock->type: %d, kalive_onoff: %d", sock->type, g_socket_system_cfg.kalive_onoff);
    if (sock->type == QCM_SOCK_STREAM && g_socket_system_cfg.kalive_onoff == 1)
    {
        setsockopt(sock->socket_fd, IPPROTO_TCP, TCP_KEEPIDLE, (void *)&g_socket_system_cfg.kalive_idle, sizeof(int));
        setsockopt(sock->socket_fd, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&g_socket_system_cfg.kalive_interval, sizeof(int));
        setsockopt(sock->socket_fd, IPPROTO_TCP, TCP_KEEPCNT, (void *)&g_socket_system_cfg.kalive_cnt, sizeof(int));
        setsockopt(sock->socket_fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&g_socket_system_cfg.kalive_onoff, sizeof(int));
    }

    qcm_socket_set_platform_opt(sock);

    sock->remote_port = remote_port;
    //Execute the connect action
    if (sock->family == AF_INET)
    {
        struct sockaddr_in server;
        qosa_memset(&server, 0, sizeof(struct sockaddr_in));

        server.sin_family = sock->family;
        server.sin_port = htons(remote_port);
        server.sin_addr = remote_ip->addr.ipv4_addr;
        ret = connect(sock->socket_fd, (struct sockaddr *)&server, sizeof(struct sockaddr_in));
    }
#if (LWIP_IPV6)
    else
    {
        struct sockaddr_in6 server;
        qosa_memset(&server, 0, sizeof(struct sockaddr_in6));

        server.sin6_family = sock->family;
        server.sin6_port = htons(remote_port);
        server.sin6_addr = remote_ip->addr.ipv6_addr;

        ret = connect(sock->socket_fd, (struct sockaddr *)&server, sizeof(struct sockaddr_in6));
    }
#else
    else
    {
        return QCM_SOCK_INVALID_PARAM;
    }
#endif

    if (ret == 0)
    {
        qcm_socket_add_monitor(sock, QCM_SOCK_MONITOR_RD);
        sock->state = QCM_SOC_STATE_CONNECT;
        return QCM_SOCK_SUCCESS;
    }
    else
    {
        sock->last_error = lwip_get_error(sock->socket_fd);
        QLOGV("lasterr=%d EINPROGRESS=%d", sock->last_error, EINPROGRESS);
        //Check the current connection status to see if it is a connection error
        if (EINPROGRESS == sock->last_error)
        {
            qcm_socket_add_monitor(sock, QCM_SOCK_MONITOR_WR);
            sock->state = QCM_SOC_STATE_CONNECTING;
            return QCM_SOCK_WODBLOCK;
        }
    }
    return QCM_SOCK_INTERNAL_ERROR;
}

/**
 * @brief Used to operate the underlying event notification of sockets in asynchronous mode
 *
 * @param[in] int s
 *          - The handle corresponding to the socket to be operated on
 *
 * @param[in] qcm_sock_event_mask_opt_e event_mask
 *          - Event types to be monitored at the user layer
 *
 * @param[in] qcm_socket_event_cb func
 *          - Register CB callback function at the application layer
 *
 * @return int
 *       - Returns 0 on success
 *       - Returns a negative value on failure; for specific information, refer to qcm_sock_err_code.
 */
int qcm_socket_register_event(int s, int event_mask, qcm_socket_event_cb func, void *argv)
{
    qcm_socket_t *sock = QOSA_NULL;

    sock = qcm_socket_get_prt(s);
    if (sock == QOSA_NULL)
    {
        return QCM_SOCK_INTERNAL_ERROR;
    }
    QLOGV("... sock=%p", sock);
    sock->event_mask |= event_mask;
    sock->event_cb = func;
    sock->user_argv = argv;
    //Once everything is ready, start listening for events.
    if (QCM_SOC_STATE_CONNECT == sock->state)
    {
        qcm_socket_add_monitor(sock, QCM_SOCK_MONITOR_RD);  //Set the listening event afterwards to avoid ql_apptcpip_soc_callback_func from executing immediately without a callback, which could cause message loss.
    }

    return QCM_SOCK_SUCCESS;
}

#ifdef CONFIG_QCM_VTLS_FUNC
/**
 * @brief When the connection switches to an SSL connection, use the SSL read function for data processing.
 *
 * @param[in] qcm_socket_t * sock
 *         - The handle corresponding to the socket to be operated on
 *
 * @param[out] char * data
 *          - Data read storage address
 *
 * @param[in] int sz
 *          - The size of data to be read, the data parameter space should be larger than sz
 *
 * @return int
 *       - Returns the actual read length greater than 0 on success
 *       - Returns a negative value on failure; for specific information, refer to qcm_sock_err_code.
 */
static int qcm_socket_ssl_read(qcm_socket_t *sock, char *data, int sz)
{
    int ret = 0;

    //If the SSL connection is not successfully established, an SSL handshake must be performed; if the SSL connection is successful, data can be sent.
    if (sock->ssl_state != QCM_SOC_SSL_STATE_CONNECT)
    {
        //If the calling layer does not enter before the SSL connection is established, it directly returns a wouldblock state, causing the upper-layer state machine to become disordered.
        return QCM_SOCK_SSL_CONN_ERROR;
    }
    else
    {
        qcm_vtls_result_status_e result_code = QCM_VTLS_RESULT_OK;
        ret = qcm_ssl_read(sock->ssl_ctx, data, sz, &result_code);
        QLOGV("ret = %d curlcode=%d  QCM_VTLS_SSL_CONNECT_ERR=%d", ret, result_code, QCM_VTLS_SSL_CONNECT_ERR);
        if (ret <= 0)
        {
            if (result_code == QCM_VTLS_SSL_READ_WRITE_EAGAIN || sz == 0)
            {
                //If in non-blocking mode, it will be repeatedly added.
                qcm_socket_add_monitor(sock, QCM_SOCK_MONITOR_RD);
                return QCM_SOCK_WODBLOCK;
            }
            else
            {
                sock->state = QCM_SOC_STATE_CLOSING;
                if ((sock->event_mask & QCM_SOCK_CLOSE_EVENT) != 0 && sock->event_cb != QOSA_NULL)
                {
                    sock->event_cb(sock->num, QCM_SOCK_CLOSE_EVENT, ret, sock->user_argv);
                }
                QLOGE("qcm_ssl_read error ret=%d", ret);
                return QCM_SOCK_SSL_CONN_ERROR;
            }
        }
        else
        {
            //If there is still data inside SSL, continue sending read event notifications to the upper layer to keep calling read.
            if (qcm_ssl_data_pending(sock->ssl_ctx) == QOSA_TRUE)
            {
                sock->event_cb(sock->num, QCM_SOCK_READ_EVENT, QCM_SOCK_SUCCESS, sock->user_argv);
            }
            else
            {
                //If in non-blocking mode, it will be repeatedly added.
                qcm_socket_add_monitor(sock, QCM_SOCK_MONITOR_RD);
            }
        }

        return ret;
    }
}
#endif /* CONFIG_QCM_VTLS_FUNC */
/**
 * @brief read function, used for reading data interface
 *
 * @param[in] int s
 *          - The handle corresponding to the socket to be operated on
 *
 * @param[out] char * data
 *          - Data read storage address
 *
 * @param[in] int sz
 *          - The size of data to be read, the data parameter space should be larger than sz
 *
 * @return int
 *       - Returns the actual read length greater than 0 on success
 *       - Returns a negative value on failure; for specific information, refer to qcm_sock_err_code.
 */
int qcm_socket_read(int s, char *data, int sz)
{
    int           ret = 0;
    qcm_socket_t *sock = QOSA_NULL;

    sock = qcm_socket_get_prt(s);
    if (sock == QOSA_NULL)
    {
        return QCM_SOCK_INTERNAL_ERROR;
    }
    if (sz < 0 || data == QOSA_NULL)
    {
        return QCM_SOCK_INVALID_PARAM;
    }
#ifdef CONFIG_QCM_VTLS_FUNC
    if (sock->ssl_enable == QOSA_TRUE)
    {
        return qcm_socket_ssl_read(sock, data, sz);
    }
#endif /* CONFIG_QCM_VTLS_FUNC */
    ret = recv(sock->socket_fd, data, sz, 0);
    sock->last_error = lwip_get_error(sock->socket_fd);

    QLOGV("read %d bytes from socket %d error=%d EWOULDBLOCK=%d", ret, sock->socket_fd, sock->last_error, EWOULDBLOCK);
    if ((ret < 0 && sock->last_error == EWOULDBLOCK) || ret > 0 || sz == 0)
    {
        qcm_socket_add_monitor(sock, QCM_SOCK_MONITOR_RD);
    }

    if (ret < 0)
    {
        if (sock->last_error == EWOULDBLOCK)
        {
            ret = QCM_SOCK_WODBLOCK;
        }
        else
        {
            sock->state = QCM_SOC_STATE_CLOSING;
            if ((sock->event_mask & QCM_SOCK_CLOSE_EVENT) != 0 && sock->event_cb != QOSA_NULL)
            {
                sock->event_cb(sock->num, QCM_SOCK_CLOSE_EVENT, ret, sock->user_argv);
            }
            ret = QCM_SOCK_INTERNAL_ERROR;
        }
        return ret;
    }
    else if (ret == 0)
    {
        if (sz == 0)
        {
            ret = QCM_SOCK_SUCCESS;
        }
        else
        {
            sock->state = QCM_SOC_STATE_CLOSING;
            if ((sock->event_mask & QCM_SOCK_CLOSE_EVENT) != 0 && sock->event_cb != QOSA_NULL)
            {
                sock->event_cb(sock->num, QCM_SOCK_CLOSE_EVENT, ret, sock->user_argv);
            }
            ret = QCM_SOCK_BROKEN;
        }
        return ret;
    }
    else
    {
        return ret;
    }
}
#ifdef CONFIG_QCM_VTLS_FUNC
/**
 * @brief When the connection switches to an SSL connection, use the SSL write function for data processing.
 *
 * @param[in] qcm_socket_t * sock
 *         - The handle corresponding to the socket to be operated on
 *
 * @param[out] char * data
 *          - Data to be sent
 *
 * @param[in] int sz
 *          - Data size to be sent
 *
 * @return int
 *       - Returns the actual size sent on success
 *       - Returns a negative value on failure; for specific information, refer to qcm_sock_err_code.
 */
static int qcm_socket_ssl_write(qcm_socket_t *sock, char *data, int sz)
{
    int ret = 0;
    //If the SSL connection is not successfully established, an SSL handshake must be performed; if the SSL connection is successful, data can be sent.
    if (sock->ssl_state != QCM_SOC_SSL_STATE_CONNECT)
    {
        return QCM_SOCK_SSL_CONN_ERROR;
    }
    else
    {
        qcm_vtls_result_status_e result_code = QCM_VTLS_RESULT_OK;
        ret = qcm_ssl_write(sock->ssl_ctx, data, sz, &result_code);
        QLOGV("ret = %d curlcode=%d  QCM_VTLS_SSL_CONNECT_ERR=%d", ret, result_code, QCM_VTLS_SSL_CONNECT_ERR);
        if (ret <= 0)
        {
            if (result_code == QCM_VTLS_SSL_READ_WRITE_EAGAIN || sz == 0)
            {
                qcm_socket_add_monitor(sock, QCM_SOCK_MONITOR_WR);
                return QCM_SOCK_WODBLOCK;
            }
            else
            {
                QLOGE("qcm_socket_send error ret=%d", ret);
                return QCM_SOCK_SSL_CONN_ERROR;
            }
        }
        return ret;
    }
}
#endif /* CONFIG_QCM_VTLS_FUNC */
/**
 * @brief The send function transmits data.
 *
 * @param[in] int s
 *          - The handle corresponding to the socket to be operated on
 *
 * @param[out] char * data
 *          - Data to be sent
 *
 * @param[in] int sz
 *          - Data size to be sent
 *
 * @return int
 *       - Returns the actual size sent on success
 *       - Returns a negative value on failure; for specific information, refer to qcm_sock_err_code.
 *       - If the returned actual length is less than the size of sz, the send function should be called once to send data, which will trigger the underlying layer to report a wouldblock status.
 */
int qcm_socket_send(int s, char *data, int sz)
{
    int           ret = 0;
    qcm_socket_t *sock = QOSA_NULL;

    sock = qcm_socket_get_prt(s);
    if (sock == QOSA_NULL)
    {
        return QCM_SOCK_INTERNAL_ERROR;
    }
    if (sz < 0 || data == QOSA_NULL)
    {
        return QCM_SOCK_INVALID_PARAM;
    }
#ifdef CONFIG_QCM_VTLS_FUNC
    if (sock->ssl_enable == QOSA_TRUE)
    {
        return qcm_socket_ssl_write(sock, data, sz);
    }
#endif /* CONFIG_QCM_VTLS_FUNC */
    ret = send(sock->socket_fd, data, sz, 0);

    sock->last_error = lwip_get_error(sock->socket_fd);
    QLOGV("write (%d)%d bytes to socket %d, errno:%d EWOULDBLOCK=%d", (sz), ret, sock->socket_fd, sock->last_error, EWOULDBLOCK);

    if ((ret < 0 && sock->last_error == EWOULDBLOCK) || (ret > 0 && ret < sz))
    {
        qcm_socket_add_monitor(sock, QCM_SOCK_MONITOR_WR);
    }

    if (ret <= 0)
    {
        if (sock->last_error == EWOULDBLOCK)
        {
            ret = QCM_SOCK_WODBLOCK;
        }
        else
        {
            ret = QCM_SOCK_INTERNAL_ERROR;
            sock->state = QCM_SOC_STATE_CLOSING;
            if ((sock->event_mask & QCM_SOCK_CLOSE_EVENT) != 0 && sock->event_cb != QOSA_NULL)
            {
                sock->event_cb(sock->num, QCM_SOCK_CLOSE_EVENT, ret, sock->user_argv);
            }
            ret = QCM_SOCK_BROKEN;
        }
        return ret;
    }
    else
    {
        return ret;
    }
}

/**
 * @brief socket listen function
 *
 * @param[in] int s
 *          - The handle corresponding to the socket to be operated on
 *
 * @param[in] int backlog
 *          - Maximum number of clients accepted
 *
 * @return int
 *       - Returns 0 on successful execution
 *       - Returns a negative value on failure; for specific information, refer to qcm_sock_err_code.
 */
int qcm_socket_listen(int s, int backlog)
{
    int           ret = 0;
    qcm_socket_t *sock = QOSA_NULL;

    sock = qcm_socket_get_prt(s);
    if (sock == QOSA_NULL)
    {
        return QCM_SOCK_INTERNAL_ERROR;
    }
    if (sock->type == QCM_SOCK_DGRAM)
    {
        sock->state = QCM_SOC_STATE_CONNECT;
        qcm_socket_add_monitor(sock, QCM_SOCK_MONITOR_RD);
        return QCM_SOCK_SUCCESS;
    }

    ret = listen(sock->socket_fd, backlog);

    if (ret == 0)
    {
        sock->state = QCM_SOC_STATE_LISTEN;
        qcm_socket_add_monitor(sock, QCM_SOCK_MONITOR_RD);
        return QCM_SOCK_SUCCESS;
    }
    else
    {
        return QCM_SOCK_INTERNAL_ERROR;
    }
}

/**
 * @brief socket accept accepts socket connection requests, and the newly created socket connection function will set whether it is a blocking IO socket according to the block parameter configuration of the listening socket.
 *  The function must be called after executing qcm_socket_listen.
 *
 * @param[in] int s
 *          - The handle corresponding to the socket to be operated on
 *
 * @param[out] qosa_ip_addr_t * remote_addr
 *          - Returns the IP address of the newly accepted connection
 *
 * @param[out] int * remote_port
 *          - Returns the destination port of the newly accepted connection
 *
 * @param[out] qosa_ip_addr_t * local_addr
 *          - Returns the local IP address
 *
 * @param[out] int * local_port
 *          - Returns the local port
 *
 * @return int
 *       - Returns the handle corresponding to the new connection
 *       - Returns a negative value on failure; for specific information, refer to qcm_sock_err_code.
 */
int qcm_socket_accept(int s, qosa_ip_addr_t *remote_addr, int *remote_port, qosa_ip_addr_t *local_addr, int *local_port)
{
    int                 accept_sockfd = 0;
    socklen_t           addr_len = 0;
    int                 accpet_num = 0;
    qcm_socket_t       *sock = QOSA_NULL;
    qcm_socket_t       *accept_sock = QOSA_NULL;
    struct sockaddr_in  sa;
#if (LWIP_IPV6)
    struct sockaddr_in6 sa6;
#endif

    sock = qcm_socket_get_prt(s);
    if (sock == QOSA_NULL)
    {
        return QCM_SOCK_INTERNAL_ERROR;
    }
    if (sock->family == AF_INET)
    {
        qosa_memset(&sa, 0, sizeof(struct sockaddr_in));
        addr_len = sizeof(struct sockaddr_in);
        accept_sockfd = accept(sock->socket_fd, (struct sockaddr *)&sa, (socklen_t *)&addr_len);
    }
#if (LWIP_IPV6)
    else
    {
        qosa_memset(&sa6, 0, sizeof(struct sockaddr_in6));
        addr_len = sizeof(struct sockaddr_in6);
        accept_sockfd = accept(sock->socket_fd, (struct sockaddr *)&sa6, (socklen_t *)&addr_len);
    }
#else
    else
    {
        return QCM_SOCK_INVALID_PARAM;
    }
#endif

    QLOGV("sock->type: %d, kalive_onoff: %d", sock->type, g_socket_system_cfg.kalive_onoff);
    if (sock->type == QCM_SOCK_STREAM && g_socket_system_cfg.kalive_onoff == 1)
    {
        setsockopt(accept_sockfd, IPPROTO_TCP, TCP_KEEPIDLE, (void *)&g_socket_system_cfg.kalive_idle, sizeof(int));
        setsockopt(accept_sockfd, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&g_socket_system_cfg.kalive_interval, sizeof(int));
        setsockopt(accept_sockfd, IPPROTO_TCP, TCP_KEEPCNT, (void *)&g_socket_system_cfg.kalive_cnt, sizeof(int));
        setsockopt(accept_sockfd, SOL_SOCKET, SO_KEEPALIVE, (void *)&g_socket_system_cfg.kalive_onoff, sizeof(int));
    }

    qcm_socket_add_monitor(sock, QCM_SOCK_MONITOR_RD);

    if (accept_sockfd >= 0)
    {
        int flags = 0;

        accpet_num = qcm_socket_get_new_fd();
        if (accpet_num <= 0)
        {
            close(accept_sockfd);
            return QCM_SOCK_OUT_MEM;
        }
        accept_sock = qcm_socket_get_prt(accpet_num);

        if (sock->block == QOSA_FALSE)
        {
            flags = fcntl(accept_sockfd, F_GETFL, 0);  //Set to no block
            fcntl(accept_sockfd, F_SETFL, flags | O_NONBLOCK);
        }
        qosa_memset(accept_sock, 0x00, sizeof(qcm_socket_t));

        accept_sock->num = accpet_num;
        accept_sock->socket_fd = accept_sockfd;
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
        accept_sock->pdpcid = sock->pdpcid;
        accept_sock->sim_id = sock->sim_id;
#endif
        accept_sock->family = sock->family;
        accept_sock->type = QCM_SOCK_STREAM;
        accept_sock->state = QCM_SOC_STATE_CONNECT;
        accept_sock->block = sock->block;
        accept_sock->local_addr = sock->local_addr;

        qcm_socket_set_platform_opt(accept_sock);

        if (sock->family == QCM_AF_INET)
        {
            if (remote_port != QOSA_NULL)
            {
                *remote_port = ntohs(sa.sin_port);
            }
            if (remote_addr != QOSA_NULL)
            {
                remote_addr->addr.ipv4_addr = sa.sin_addr;
#if (defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
                remote_addr->ip_vsn = QOSA_IP_V4;
#else
                remote_addr->ip_vsn = QOSA_PDP_IPV4;
#endif
            }

            //Get the local port of the new connection
            struct sockaddr_in local_sa;
            qosa_memset(&local_sa, 0, sizeof(struct sockaddr_in));
            addr_len = sizeof(struct sockaddr_in);
            getsockname(accept_sockfd, (struct sockaddr *)&local_sa, (socklen_t *)&addr_len);
            accept_sock->local_port = ntohs(local_sa.sin_port);
            if (local_port != QOSA_NULL)
            {
                *local_port = accept_sock->local_port;
            }
            if (local_addr != QOSA_NULL)
            {
                local_addr->addr.ipv4_addr = sock->local_addr.addr.ipv4_addr;
#if (defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
                local_addr->ip_vsn = QOSA_IP_V4;
#else
                local_addr->ip_vsn = QOSA_PDP_IPV4;
#endif
            }
        }
#if (LWIP_IPV6)
        else
        {
            if (remote_port != QOSA_NULL)
            {
                *remote_port = ntohs(sa6.sin6_port);
            }
            if (remote_addr != QOSA_NULL)
            {
                remote_addr->addr.ipv6_addr = sa6.sin6_addr;
#if (defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
                remote_addr->ip_vsn = QOSA_IP_V6;
#else
                remote_addr->ip_vsn = QOSA_PDP_IPV6;
#endif
            }

            //Get the local port of the new connection
            struct sockaddr_in6 local_sa6;
            qosa_memset(&local_sa6, 0, sizeof(struct sockaddr_in));
            addr_len = sizeof(struct sockaddr_in);
            getsockname(accept_sockfd, (struct sockaddr *)&local_sa6, (socklen_t *)&addr_len);
            accept_sock->local_port = ntohs(local_sa6.sin6_port);
            if (local_port != QOSA_NULL)
            {
                *local_port = accept_sock->local_port;
            }
            if (local_addr != QOSA_NULL)
            {
                local_addr->addr.ipv6_addr = sock->local_addr.addr.ipv6_addr;
#if (defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
                local_addr->ip_vsn = QOSA_IP_V6;
#else
                local_addr->ip_vsn = QOSA_PDP_IPV6;
#endif
            }
        }

    #endif
        unir_set_socket2(accept_sock->socket_fd, ql_apptcpip_soc_callback_func, accept_sock);
        //qcm_tcpip_service_new has not been created yet, qcm_socket_register_event has not set sock->event_cb
        //After the client acknowledges, sending data immediately may result in data not being reported.
        //Set the listening event afterwards to avoid immediate execution of ql_apptcpip_soc_callback_func without a callback, which could cause message loss -- set in qcm_socket_register_event.
        //Do not set the listening event immediately; wait until after qcm_socket_register_event configuration is complete---aimed to handle cases where the client sends data immediately after SYN/ACK.
        if (accept_sockfd > g_fd_max)
        {
            g_fd_max = accept_sockfd;
        }
        return accpet_num;
    }

    return QCM_SOCK_INTERNAL_ERROR;
}

/**
 * @brief The socket shutdown function is used to close the RD and WR channels,
 *
 * The raw socket shutdown function returns 0 on successful execution, and returns -1 on failure while setting the global errno value.
 * errno code
 *  1. EBADF socket is not a valid socket descriptor. This usually occurs when the socket has not been opened or has already been closed.
 *  2. EINVAL: The `how` parameter is not set to a valid value.
 *  3. ENOBUFS: Insufficient system resources to complete this call. This typically occurs when system resources are under high pressure.
 *  4. ENOTCONN: Socket is not connected. This error is returned if an attempt is made to close a socket that is not connected.
 *  5. ENOTSOCK: The descriptor refers to a file, not a socket.
 *
 * Based on this we only need to focus on the ENOTCONN error value
 *
 * In lwip, if currently in the process of writing, return error code EINPROGRESS; if not connected, return ENOTCONN; otherwise, return OK.
 *
 * @param[in] int s
 *          - The socket action to be performed
 *
 * @param[in] int how
 *          - Corresponding operation directions QCM_SHUT_RD QCM_SHUT_WR QCM_SHUT_RDWR
 *
 * @return int
 *        - The function returns the qcm_sock_err_code error code upon completion.
 */
int qcm_socket_shutdown(int s, int how)
{
    int           ret = 0;
    qcm_socket_t *sock = QOSA_NULL;

    sock = qcm_socket_get_prt(s);
    if (sock == QOSA_NULL)
    {
        return QCM_SOCK_INTERNAL_ERROR;
    }

    ret = shutdown(sock->socket_fd, how);
    sock->last_error = lwip_get_error(sock->socket_fd);

    QLOGV("socket %d close ret:%d, errno:%d", sock->socket_fd, ret, sock->last_error);
    if (ret == 0)
    {
        QLOGV("...");
        return QCM_SOCK_SUCCESS;
    }
    else if (ret == -1 && (sock->last_error == EINPROGRESS || sock->last_error == EWOULDBLOCK))
    {
        QLOGV("...");
        sock->state = QCM_SOC_STATE_CLOSING;

        if (how == QCM_SHUT_WR)
        {
            qcm_socket_add_monitor(sock, QCM_SOCK_MONITOR_WR);
        }
        else if (how == QCM_SHUT_RDWR)
        {
            qcm_socket_add_monitor(sock, QCM_SOCK_MONITOR_RD);
            qcm_socket_add_monitor(sock, QCM_SOCK_MONITOR_WR);
        }
        else //QCM_SHUT_RD
        {
            qcm_socket_add_monitor(sock, QCM_SOCK_MONITOR_RD);
        }
        return QCM_SOCK_WODBLOCK;
    }
    else
    {
        QLOGV("...");
        if (sock->last_error == ENOTCONN)
        {
            QLOGV("...");
            return QCM_SOCK_SUCCESS;
        }
        else
        {
            return QCM_SOCK_INTERNAL_ERROR;
        }
    }
}

/**
 * @brief The socket close function blocks internally regardless of external configuration.
 *  If you want to change the behavior of close's return, you need to configure the socket linger option time.
 *
 * @param[in] int s
 *          - The handle corresponding to the socket to be operated on
 *
 * @return int
 *       - Returns 0 on successful execution
 *       - Returns a negative value on failure; for specific information, refer to qcm_sock_err_code.
 */
int qcm_socket_close(int s)
{
    int           ret = 0;
    qcm_socket_t *sock = QOSA_NULL;
    int           flags = 0;

    sock = qcm_socket_get_prt(s);
    if (sock == QOSA_NULL)
    {
        return QCM_SOCK_INTERNAL_ERROR;
    }

    qcm_socket_remove_monitor(sock, QCM_SOCK_MONITOR_RD);
    qcm_socket_remove_monitor(sock, QCM_SOCK_MONITOR_WR);

    flags = fcntl(sock->socket_fd, F_GETFL, 0);  //Set to block
    fcntl(sock->socket_fd, F_SETFL, flags & ~O_NONBLOCK);

#ifdef CONFIG_QCM_VTLS_FUNC
    if (sock->ssl_enable == QOSA_TRUE)
    {
        if (sock->ssl_wait_time != QOSA_NULL)
        {
            if (qosa_timer_is_running(sock->ssl_wait_time) == QOSA_TRUE)
            {
                qosa_timer_stop(sock->ssl_wait_time);
            }
            qosa_timer_delete(sock->ssl_wait_time);
            sock->ssl_wait_time = QOSA_NULL;
        }
        QLOGV("close start=%p", sock->ssl_ctx);
        qcm_ssl_close(sock->ssl_ctx);
        qcm_ssl_free(sock->ssl_ctx);
        sock->ssl_ctx = QOSA_NULL;
        sock->ssl_state = QCM_SOC_SSL_STATE_NONE;
        sock->ssl_enable = QOSA_FALSE;
    }
#endif /* CONFIG_QCM_VTLS_FUNC */

    QLOGV(".");
    ret = close(sock->socket_fd);
    sock->last_error = lwip_get_error(sock->socket_fd);
    QLOGV("socket %d close ret:%d, errno:%d", sock->socket_fd, ret, sock->last_error);

    //2023-07-28 16:31:09 The expectation is that in blocking mode, it should deregister regardless of success or failure.
    qcm_socket_free(s);
    if (ret == 0)
    {
        return QCM_SOCK_SUCCESS;
    }
    else
    {
        return QCM_SOCK_INTERNAL_ERROR;
    }
}

/**
 * @brief Send data to the specified destination address
 *
 * @param[in] int s
 *          - Indicates the socket file descriptor that has been established for communication
 *
 * @param[out] qosa_ip_addr_t *remote_addr
 *          - IP address information of the destination for sending data
 *
 * @param[in] qosa_uint16_t remote_port
 *          - Destination port to send to
 *
 * @param[in] char * data
 *          - Data content to be sent
 *
 * @param[in] int sz
 *          - Data length to be sent
 *
 * @return int
 *       - Returns the actual length of data sent upon success
 *       - Returns a negative value on failure; for specific information, refer to qcm_sock_err_code.
 */
int qcm_socket_sendto(int s, qosa_ip_addr_t *remote_addr, qosa_uint16_t remote_port, char *data, int sz)
{
    int           ret = 0;
    qcm_socket_t *sock = QOSA_NULL;

    sock = qcm_socket_get_prt(s);
    if (sock == QOSA_NULL)
    {
        return QCM_SOCK_INTERNAL_ERROR;
    }

#ifdef CONFIG_QCM_VTLS_FUNC
    if (sock->ssl_enable == QOSA_TRUE)
    {
        return qcm_socket_ssl_write(sock, data, sz);
    }
#endif /* CONFIG_QCM_VTLS_FUNC */

    if (sock->family == AF_INET)
    {
        struct sockaddr_in sa;
        qosa_memset(&sa, 0, sizeof(struct sockaddr_in));
        sa.sin_family = sock->family;
        if (remote_port != 0)
            sa.sin_port = htons(remote_port);
        sa.sin_addr = remote_addr->addr.ipv4_addr;

        ret = sendto(sock->socket_fd, data, sz, 0, (struct sockaddr *)&sa, sizeof(sa));
    }
#if (LWIP_IPV6)
    else
    {
        struct sockaddr_in6 sa6;
        qosa_memset(&sa6, 0, sizeof(struct sockaddr_in6));
        sa6.sin6_family = sock->family;
        if (remote_port != 0)
            sa6.sin6_port = htons(remote_port);

        sa6.sin6_addr = remote_addr->addr.ipv6_addr;
        ret = sendto(sock->socket_fd, data, sz, 0, (struct sockaddr *)&sa6, sizeof(sa6));
    }
#else
    else
    {
        return QCM_SOCK_INVALID_PARAM;
    }
#endif
    sock->last_error = lwip_get_error(sock->socket_fd);
    QLOGV("send to %d bytes to socket, error:%d", ret, sock->last_error);
    if ((ret < 0 && sock->last_error == EWOULDBLOCK) || (ret > 0 && ret < sz))
    {
        qcm_socket_add_monitor(sock, QCM_SOCK_MONITOR_WR);
    }

    if (ret <= 0)
    {
        if (sock->last_error == EWOULDBLOCK)
        {
            ret = QCM_SOCK_WODBLOCK;
        }
        else
        {
            ret = QCM_SOCK_INTERNAL_ERROR;
        }
        return ret;
    }
    else
    {
        return ret;
    }
}

/** Receive data from the socket and store the sender's address information at the specified location.
 *
 * @param[in] int s
 *          - sockfd: indicates the established socket file descriptor used for communication
 *
 * @param[out] qosa_ip_addr_t *remote_addr
 *          - IP address information for receiving data from the other party
 *
 * @param[out] qosa_uint16_t * remote_port
 *          - Remote port that received the data
 *
 * @param[out] char * data
 *          - Pointer to the buffer for receiving data
 *
 * @param[in] int sz
 *          - Buffer size (in bytes)
 *
 * @return int
 *       - Returns the number of bytes received on success
 *       - Returns a negative value on failure; for specific information, refer to qcm_sock_err_code.
 */
int qcm_socket_recvfrom(int s, qosa_ip_addr_t *remote_addr, qosa_uint16_t *remote_port, char *data, int sz)
{
    socklen_t           addr_len;
    int                 ret = 0;
    qcm_socket_t       *sock = QOSA_NULL;
    struct sockaddr_in  sa;
#if (LWIP_IPV6)
    struct sockaddr_in6 sa6;
#endif

    sock = qcm_socket_get_prt(s);
    if (sock == QOSA_NULL)
    {
        return QCM_SOCK_INTERNAL_ERROR;
    }

#ifdef CONFIG_QCM_VTLS_FUNC
    if (sock->ssl_enable == QOSA_TRUE)
    {
        return qcm_socket_ssl_read(sock, data, sz);
    }
#endif /* CONFIG_QCM_VTLS_FUNC */

    if (sock->family == AF_INET)
    {
        qosa_memset(&sa, 0, sizeof(struct sockaddr_in));
        addr_len = sizeof(struct sockaddr_in);
        ret = recvfrom(sock->socket_fd, data, sz, 0, (struct sockaddr *)&sa, (socklen_t *)&addr_len);
        if (remote_port != QOSA_NULL)
        {
            *remote_port = ntohs(sa.sin_port);
        }
        if (remote_addr != QOSA_NULL)
        {
#if (defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
            remote_addr->ip_vsn = QOSA_IP_V4;
#else
            remote_addr->ip_vsn = QOSA_PDP_IPV4;
#endif
            remote_addr->addr.ipv4_addr = sa.sin_addr;
        }
    }
#if (LWIP_IPV6)
    else
    {
        qosa_memset(&sa6, 0, sizeof(struct sockaddr_in6));
        addr_len = sizeof(struct sockaddr_in6);
        ret = recvfrom(sock->socket_fd, data, sz, 0, (struct sockaddr *)&sa6, (socklen_t *)&addr_len);
        if (remote_port != QOSA_NULL)
        {
            *remote_port = ntohs(sa6.sin6_port);
        }
        if (remote_addr != QOSA_NULL)
        {
#if (defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
            remote_addr->ip_vsn = QOSA_IP_V6;
#else
            remote_addr->ip_vsn = QOSA_PDP_IPV6;
#endif
            remote_addr->addr.ipv6_addr = sa6.sin6_addr;
        }
    }
#else
    else
    {
        return QCM_SOCK_INVALID_PARAM;
    }
#endif
    sock->last_error = lwip_get_error(sock->socket_fd);
    if ((ret < 0 && sock->last_error == EWOULDBLOCK) || ret > 0)
    {
        qcm_socket_add_monitor(sock, QCM_SOCK_MONITOR_RD);
    }
    QLOGV("read %d bytes from socket, errno:%d", ret, sock->last_error);

    if (ret < 0)
    {
        if (sock->last_error == EWOULDBLOCK)
        {
            ret = QCM_SOCK_WODBLOCK;
        }
        else
        {
            ret = QCM_SOCK_INTERNAL_ERROR;
        }
        return ret;
    }
    else if (ret == 0)
    {
        sock->state = QCM_SOC_STATE_CLOSING;
        if ((sock->event_mask & QCM_SOCK_CLOSE_EVENT) != 0 && sock->event_cb != QOSA_NULL)
        {
            sock->event_cb(sock->num, QCM_SOCK_CLOSE_EVENT, ret, sock->user_argv);
        }
        return QCM_SOCK_BROKEN;
    }
    else
    {
        return ret;
    }
}

/**
 * @brief Get system opt options
 */
int qcm_socket_get_system_opt(qcm_socket_system_option_e opt, void *value)
{
    int                          ret = 0;
    qcm_socket_system_opt_cfg_t *system_cfg_ptr = &g_socket_system_cfg;

    switch (opt)
    {
        case QCM_TCP_KEEPALIVE_OPT: {
            qosa_uint8_t *kalive_onoff = (qosa_uint8_t *)value;
            *kalive_onoff = system_cfg_ptr->kalive_onoff;
            ret = QCM_SOCK_SUCCESS;
        }
        break;

        case QCM_TCP_KEEPIDLE_OPT: {
            qosa_uint32_t *kalive_keepidle = (qosa_uint32_t *)value;
            *kalive_keepidle = system_cfg_ptr->kalive_idle;
            ret = QCM_SOCK_SUCCESS;
        }
        break;

        case QCM_TCP_KEEPINTVL_OPT: {
            qosa_uint32_t *kalive_interval = (qosa_uint32_t *)value;
            *kalive_interval = system_cfg_ptr->kalive_interval;
            ret = QCM_SOCK_SUCCESS;
        }
        break;

        case QCM_TCP_KEEPCNT_OPT: {
            qosa_uint32_t *kalive_cnt = (qosa_uint32_t *)value;
            *kalive_cnt = system_cfg_ptr->kalive_cnt;
            ret = QCM_SOCK_SUCCESS;
        }
        break;
        case QCM_TCP_WINDOWS_SCALING_OPT: {
            qosa_uint32_t _scale = 0;
            qosa_uint8_t *scale = (qosa_uint8_t *)value;
            qcm_socket_get_windows_scale(&_scale);
            system_cfg_ptr->scale = (qosa_uint8_t)_scale;
            *scale = (qosa_uint8_t)_scale;
            ret = QCM_SOCK_SUCCESS;
        }
        break;
        case QCM_TCP_RETRY_TIME_OPT: {
            qosa_uint32_t  _time = 0;
            qosa_uint16_t *retry_time = (qosa_uint16_t *)value;

            //Get the platform retry count and retry interval, assign to current
            qcm_socket_get_retrains_period(&_time);
            system_cfg_ptr->retry_time = (qosa_uint16_t)_time;

            *retry_time = (qosa_uint16_t)_time;
            ret = QCM_SOCK_SUCCESS;
        }
        break;
        case QCM_TCP_RETRY_COUNT_OPT: {
            qosa_uint32_t _count = 0;
            qosa_uint8_t *retry_count = (qosa_uint8_t *)value;

            //Get the platform retry count and retry interval, assign to current
            qcm_socket_get_retrains_cnt(&_count);
            system_cfg_ptr->retry_count = (qosa_uint16_t)_count;
            *retry_count = _count;
            ret = QCM_SOCK_SUCCESS;
        }
        break;
        case QCM_TCP_DELAY_ACK_OPT: {
            qosa_uint8_t *delay_ack = (qosa_uint8_t *)value;
            *delay_ack = system_cfg_ptr->delay_ack;
            ret = QCM_SOCK_SUCCESS;
        }
        break;
        case QCM_TCP_TIMEWAIT_FAST_RELEASE_OPT: {
            qosa_uint32_t _fast_release_timewait = 0;
            qosa_uint8_t *fast_release_timewait = (qosa_uint8_t *)value;
            qcm_socket_get_twrecycle(&_fast_release_timewait);
            system_cfg_ptr->fast_release_timewait = (qosa_uint8_t)_fast_release_timewait;
            *fast_release_timewait = (qosa_uint8_t)_fast_release_timewait;
            ret = QCM_SOCK_SUCCESS;
        }
        break;
        case QCM_TCP_RECV_WINDOWS_OPT: {
            qosa_uint32_t *tcp_recv_windows_size = (qosa_uint32_t *)value;
            qcm_socket_get_recv_windows(tcp_recv_windows_size);
            system_cfg_ptr->tcp_recv_windows_size = *tcp_recv_windows_size;
            ret = QCM_SOCK_SUCCESS;
        }
        break;
        case QCM_TCP_SEND_WINDOWS_OPT: {
            qosa_uint32_t *tcp_send_windows_size = (qosa_uint32_t *)value;
            qcm_socket_get_send_windows(tcp_send_windows_size);
            system_cfg_ptr->tcp_send_windows_size = *tcp_send_windows_size;
            ret = QCM_SOCK_SUCCESS;
        }
        break;
        case QCM_UDP_FILTER_OPT: {
            qosa_uint32_t _udp_filter = 0;
            qosa_uint8_t *udp_filter = (qosa_uint8_t *)value;
            qcm_socket_get_udp_filter_mode(&_udp_filter);
            system_cfg_ptr->udp_filter = (qosa_uint8_t)_udp_filter;
            *udp_filter = (qosa_uint8_t)_udp_filter;
            ret = QCM_SOCK_SUCCESS;
        }
        break;
        case QCM_TCP_MSS_OPT: {
            qosa_uint32_t _tcp_mss = 0;
            qosa_uint16_t *tcp_mss = (qosa_uint16_t *)value;
            qcm_socket_get_tcp_mss(&_tcp_mss);
            system_cfg_ptr->tcp_mss = (qosa_uint16_t)_tcp_mss;
            *tcp_mss = (qosa_uint16_t)_tcp_mss;
            ret = QCM_SOCK_SUCCESS;
        }
        break;
        case QCM_DNS_CACHE_OPT: {
            qosa_uint8_t *dns_cache = (qosa_uint8_t *)value;
            *dns_cache = system_cfg_ptr->dns_cache;
            ret = QCM_SOCK_SUCCESS;
        }
        break;
        case QCM_DNS_RETRY_TIME_OPT: {
            qosa_uint16_t *dns_retry_time = (qosa_uint16_t *)value;
            *dns_retry_time = system_cfg_ptr->dns_retry_time;
            ret = QCM_SOCK_SUCCESS;
        }
        break;
        case QCM_DNS_RETRY_COUNT_OPT: {
            qosa_uint8_t *dns_retry_cnt = (qosa_uint8_t *)value;
            *dns_retry_cnt = system_cfg_ptr->dns_retry_cnt;
            ret = QCM_SOCK_SUCCESS;
        }
        break;
        default:
            ret = QCM_SOCK_NOT_ALLOW;
            break;
    }
    return ret;
}

/**
 * @brief Set system opt options
 *
 * @note Points requiring special attention for larson.li regarding tcp.keepalive: if the server actively closes, we enter a half-closed state,
 * If we enable the keepalive configuration, it will send TCP keepalive packets once per second until we perform the third and fourth handshake. Corresponding Jira
 * STUNISOC8910-4517
 */
int qcm_socket_set_system_opt(qcm_socket_system_option_e opt, void *value)
{
    int                          ret = 0;
    qcm_socket_system_opt_cfg_t *system_cfg_ptr = &g_socket_system_cfg;

    switch (opt)
    {
        case QCM_TCP_KEEPALIVE_OPT: {
            qosa_uint8_t kalive_onoff = *(qosa_uint8_t *)value;

            system_cfg_ptr->kalive_onoff = kalive_onoff;
            ret = QCM_SOCK_SUCCESS;
        }
        break;

        case QCM_TCP_KEEPIDLE_OPT: {
            qosa_uint32_t kalive_keepidle = *(qosa_uint32_t *)value;

            system_cfg_ptr->kalive_idle = kalive_keepidle;
            ret = QCM_SOCK_SUCCESS;
        }
        break;

        case QCM_TCP_KEEPINTVL_OPT: {
            qosa_uint32_t kalive_interval = *(qosa_uint32_t *)value;

            system_cfg_ptr->kalive_interval = kalive_interval;
            ret = QCM_SOCK_SUCCESS;
        }
        break;

        case QCM_TCP_KEEPCNT_OPT: {
            qosa_uint32_t kalive_cnt = *(qosa_uint32_t *)value;

            system_cfg_ptr->kalive_cnt = kalive_cnt;
            ret = QCM_SOCK_SUCCESS;
        }
        break;
        case QCM_TCP_WINDOWS_SCALING_OPT: {
            qosa_uint8_t scale = *(qosa_uint8_t *)value;
            system_cfg_ptr->scale = scale;
            qcm_socket_set_windows_scale(scale);
            ret = QCM_SOCK_SUCCESS;
        }
        break;
        case QCM_TCP_RETRY_TIME_OPT: {
            qosa_uint16_t retry_time = *(qosa_uint16_t *)value;
            system_cfg_ptr->retry_time = retry_time;
            qcm_socket_set_retrains_period(retry_time);
            ret = QCM_SOCK_SUCCESS;
        }
        break;
        case QCM_TCP_RETRY_COUNT_OPT: {
            qosa_uint8_t retry_count = *(qosa_uint8_t *)value;
            system_cfg_ptr->retry_count = retry_count;
            qcm_socket_set_retrains_cnt(retry_count);
            ret = QCM_SOCK_SUCCESS;
        }
        break;
        case QCM_TCP_DELAY_ACK_OPT: {
            qosa_uint8_t delay_ack = *(qosa_uint8_t *)value;
            system_cfg_ptr->delay_ack = delay_ack;
            ret = QCM_SOCK_SUCCESS;
        }
        break;
        case QCM_TCP_TIMEWAIT_FAST_RELEASE_OPT: {
            qosa_uint8_t fast_release_timewait = *(qosa_uint8_t *)value;
            system_cfg_ptr->fast_release_timewait = fast_release_timewait;
            qcm_socket_set_twrecycle(fast_release_timewait);
            ret = QCM_SOCK_SUCCESS;
        }
        break;
        case QCM_TCP_RECV_WINDOWS_OPT: {
            qosa_uint32_t tcp_recv_windows_size = *(qosa_uint32_t *)value;
            system_cfg_ptr->tcp_recv_windows_size = tcp_recv_windows_size;
            qcm_socket_set_recv_windows(tcp_recv_windows_size);
            ret = QCM_SOCK_SUCCESS;
        }
        break;
        case QCM_TCP_SEND_WINDOWS_OPT: {
            qosa_uint32_t tcp_send_windows_size = *(qosa_uint32_t *)value;
            system_cfg_ptr->tcp_send_windows_size = tcp_send_windows_size;
            qcm_socket_set_send_windows(tcp_send_windows_size);
            ret = QCM_SOCK_SUCCESS;
        }
        break;
        case QCM_UDP_FILTER_OPT: {
            qosa_uint8_t udp_filter = *(qosa_uint8_t *)value;
            system_cfg_ptr->udp_filter = udp_filter;
            qcm_socket_set_udp_filter_mode(udp_filter);
            ret = QCM_SOCK_SUCCESS;
        }
        break;
        case QCM_TCP_MSS_OPT: {
            qosa_uint16_t tcp_max_segment = *(qosa_uint16_t *)value;
            system_cfg_ptr->tcp_mss = tcp_max_segment;
            qcm_socket_set_tcp_mss(tcp_max_segment);
            ret = QCM_SOCK_SUCCESS;
        }
        break;
        case QCM_DNS_CACHE_OPT: {
            qosa_uint8_t dns_cache = *(qosa_uint8_t *)value;
            system_cfg_ptr->dns_cache = dns_cache;
            qcm_socket_set_dns_cache_opt(dns_cache);
            ret = QCM_SOCK_SUCCESS;
        }
        break;
        case QCM_DNS_RETRY_TIME_OPT: {
            qosa_uint16_t dns_retry_time = *(qosa_uint16_t *)value;
            system_cfg_ptr->dns_retry_time = dns_retry_time;
            qcm_socket_set_dns_retry_opt(system_cfg_ptr->dns_retry_time, system_cfg_ptr->dns_retry_cnt);
            ret = QCM_SOCK_SUCCESS;
        }
        break;
        case QCM_DNS_RETRY_COUNT_OPT: {
            qosa_uint8_t dns_retry_cnt = *(qosa_uint8_t *)value;
            system_cfg_ptr->dns_retry_cnt = dns_retry_cnt;
            qcm_socket_set_dns_retry_opt(system_cfg_ptr->dns_retry_time, system_cfg_ptr->dns_retry_cnt);
            ret = QCM_SOCK_SUCCESS;
        }
        break;
        default:
            ret = QCM_SOCK_NOT_ALLOW;
            break;
    }
    return ret;
}

/**
 * @brief Set socket options
 *
 * @param[in] int s
 *          - Indicates the socket file descriptor that has been established for communication
 *
 * @param[in] int opt
 *          - Option name, used to specify the option to be set
 *
 * @param[in] void * value
 *          - Pointer to the buffer storing the option value
 *
 * @return int
 *       - Returns 0 on success
 *       - Returns a negative value on failure; for specific information, refer to qcm_sock_err_code.
 */
int qcm_socket_set_opt(int s, qcm_socke_option_e opt, void *value)
{
    int           ret = 0;
    qcm_socket_t *sock = QOSA_NULL;

    sock = qcm_socket_get_prt(s);
    if (sock == QOSA_NULL)
    {
        return QCM_SOCK_INTERNAL_ERROR;
    }
    if (value == QOSA_NULL)
    {
        return QCM_SOCK_INVALID_PARAM;
    }


    switch (opt)
    {
        case QCM_SOCK_LINGER_OPT: {
            struct linger        _linger;
            qcm_socket_linger_t *q_linger = (qcm_socket_linger_t *)value;

            _linger.l_onoff = q_linger->on_off;
            _linger.l_linger = q_linger->linger_val;
            ret = setsockopt(sock->socket_fd, SOL_SOCKET, SO_LINGER, (void *)&_linger, sizeof(_linger));
            if (ret == 0)
            {
                ret = QCM_SOCK_SUCCESS;
            }
            else
            {
                ret = QCM_SOCK_NOT_ALLOW;
            }
#if defined(CONFIG_QOSA_QCX216_PLATFORM_FUNC) || defined(CONFIG_QOSA_QCX217_PLATFORM_FUNC) || defined(CONFIG_QOSA_EIGEN718_PLATFORM_FUNC)
            //QCX216 specific, subsequent need to add corresponding macro control. The actual behavior is that LINGER does not take effect; after sending a FIN packet, if the server does not respond, the TCP connection persists indefinitely.
            qosa_int32_t tcp_close_timeout = q_linger->linger_val;
            setsockopt(sock->socket_fd, SOL_SOCKET, SO_TCP_CLOSE_TIMEOUT_NEED_LOCAL_ABORT, (void *)&tcp_close_timeout, sizeof(tcp_close_timeout));
#endif /* CONFIG_QOSA_QCX216_PLATFORM_FUNC || CONFIG_QOSA_QCX217_PLATFORM_FUNC || defined(CONFIG_QOSA_EIGEN718_PLATFORM_FUNC) */
        }
        break;
        case QCM_SOCK_REUSEADDR_OPT: {
            ret = setsockopt(sock->socket_fd, SOL_SOCKET, SO_REUSEADDR, (void *)value, sizeof(int));
            if (ret == 0)
            {
                ret = QCM_SOCK_SUCCESS;
            }
            else
            {
                ret = QCM_SOCK_NOT_ALLOW;
            }
        }
        break;
        case QCM_IP_TTL_OPT: {
            ret = setsockopt(sock->socket_fd, IPPROTO_IP, IP_TTL, (void *)value, sizeof(int));
            if (ret == 0)
            {
                ret = QCM_SOCK_SUCCESS;
            }
            else
            {
                ret = QCM_SOCK_NOT_ALLOW;
            }
        }
        break;
/*
        case QCM_SOCK_NOSACK_OPT: {
            ret = setsockopt(sock->socket_fd, SOL_SOCKET, SO_NOSACK, (void *)value, sizeof(int));
            if (ret == 0)
            {
                ret = QCM_SOCK_SUCCESS;
            }
            else
            {
                ret = QCM_SOCK_NOT_ALLOW;
            }
        }
        break;
*/
        case QCM_SOCK_RCVTIMEO_OPT: {
            ret = setsockopt(sock->socket_fd, SOL_SOCKET, SO_RCVTIMEO, (void *)value, sizeof(int));
            if (ret == 0)
            {
                ret = QCM_SOCK_SUCCESS;
            }
            else
            {
                ret = QCM_SOCK_NOT_ALLOW;
            }
        }
        break;
        case QCM_TCP_NODELAY_OPT: {
            ret = setsockopt(sock->socket_fd, IPPROTO_TCP, TCP_NODELAY, (void *)value, sizeof(int));
            if (ret == 0)
            {
                ret = QCM_SOCK_SUCCESS;
            }
            else
            {
                ret = QCM_SOCK_NOT_ALLOW;
            }
        }
        break;
        default: {
            ret = QCM_SOCK_INVALID_PARAM;
        }
        break;
    }

    return ret;
}

/**
 * @brief Get socket option interface
 *
 * @param[in] int s
 *          - Indicates the socket file descriptor that has been established for communication
 *
 * @param[in] int opt
 *          - Option name, used to specify the option to be set
 *
 * @param[in/out] void * value
 *          - Pointer to the buffer storing the option value
 *
 * @return int
 *       - Returns 0 on success
 *       - Returns a negative value on failure; for specific information, refer to qcm_sock_err_code.
 */
int qcm_socket_get_opt(int s, qcm_socke_option_e opt, void *value)
{
    int           ret = 0;
    qcm_socket_t *sock = QOSA_NULL;

    sock = qcm_socket_get_prt(s);
    if (sock == QOSA_NULL)
    {
        return QCM_SOCK_INTERNAL_ERROR;
    }

    switch (opt)
    {
        case QCM_SOCK_UNREAD_OPT: {
            *((int *)value) = lwip_getRecvAvailSize(sock->socket_fd);
            ret = QCM_SOCK_SUCCESS;
        }
        break;
        case QCM_TCP_ACK_OPT: {
            if (sock->type == QCM_SOCK_STREAM && (sock->state == QCM_SOC_STATE_CONNECT || sock->state == QCM_SOC_STATE_CLOSING))
            {
                int size = lwip_getAckedSize(sock->socket_fd);
                QLOGV("size:%d", size);
                if (size > 0)
                {
                    *((int *)value) = size;
                    ret = QCM_SOCK_SUCCESS;
                }
                else
                {
                    ret = QCM_SOCK_INVALID_PARAM;
                }
            }
            else
            {
                ret = QCM_SOCK_NOT_ALLOW;
            }
        }
        break;
#ifdef CONFIG_QOSA_LINUX_PLATFROM_CFG
        case QCM_TCP_UNACK_OPT: {
            //Get TCP connection information
            struct tcp_info info;
            socklen_t       len = sizeof(info);

            if (getsockopt(sock->socket_fd, IPPROTO_TCP, TCP_INFO, &info, &len) == -1)
            {
                ret = QCM_SOCK_NOT_ALLOW;
            }
            else
            {
                QLOGV("tcpi_state: %u", info.tcpi_state);
                QLOGV("tcpi_ca_state: %u", info.tcpi_ca_state);
                QLOGV("tcpi_retransmits: %u", info.tcpi_retransmits);
                QLOGV("tcpi_probes: %u", info.tcpi_probes);
                QLOGV("tcpi_backoff: %u", info.tcpi_backoff);
                QLOGV("tcpi_options: %u", info.tcpi_options);
                QLOGV("tcpi_snd_wscale: %u, tcpi_rcv_wscale: %u", info.tcpi_snd_wscale, info.tcpi_rcv_wscale);
                QLOGV("tcpi_rto: %u", info.tcpi_rto);
                QLOGV("tcpi_ato: %u", info.tcpi_ato);
                QLOGV("tcpi_snd_mss: %u", info.tcpi_snd_mss);
                QLOGV("tcpi_rcv_mss: %u", info.tcpi_rcv_mss);
                QLOGV("tcpi_unacked: %u", info.tcpi_unacked);
                QLOGV("tcpi_sacked: %u", info.tcpi_sacked);
                QLOGV("tcpi_lost: %u", info.tcpi_lost);
                QLOGV("tcpi_retrans: %u", info.tcpi_retrans);
                QLOGV("tcpi_fackets: %u", info.tcpi_fackets);
                QLOGV("tcpi_last_data_sent: %u", info.tcpi_last_data_sent);
                QLOGV("tcpi_last_ack_sent: %u", info.tcpi_last_ack_sent);
                QLOGV("tcpi_last_data_recv: %u", info.tcpi_last_data_recv);
                QLOGV("tcpi_last_ack_recv: %u", info.tcpi_last_ack_recv);
                QLOGV("tcpi_pmtu: %u", info.tcpi_pmtu);
                QLOGV("tcpi_rcv_ssthresh: %u", info.tcpi_rcv_ssthresh);
                QLOGV("tcpi_rtt: %u", info.tcpi_rtt);
                QLOGV("tcpi_rttvar: %u", info.tcpi_rttvar);
                QLOGV("tcpi_snd_ssthresh: %u", info.tcpi_snd_ssthresh);
                QLOGV("tcpi_snd_cwnd: %u", info.tcpi_snd_cwnd);
                QLOGV("tcpi_advmss: %u", info.tcpi_advmss);
                QLOGV("tcpi_reordering: %u", info.tcpi_reordering);
                QLOGV("tcpi_rcv_rtt: %u", info.tcpi_rcv_rtt);
                QLOGV("tcpi_rcv_space: %u", info.tcpi_rcv_space);
                QLOGV("tcpi_total_retrans: %u", info.tcpi_total_retrans);

                //Get the amount of confirmed data
                QLOGV("Bytes acknowledged: %u\n", info.tcpi_unacked);
                qosa_uint32_t *tmp = (qosa_uint32_t *)value;
                *tmp = info.tcpi_unacked;
                ret = QCM_SOCK_SUCCESS;
            }
        }
        break;
#endif /* CONFIG_QOSA_LINUX_PLATFROM_CFG */
        case QCM_SOCK_ERROR_OPT: {
            int       info = 0;
            socklen_t len  = sizeof(info);
            ret = lwip_getsockopt(sock->socket_fd, SOL_SOCKET, SO_ERROR, &info, &len);
            if (ret == 0)
            {
                *((int *)value) = info;
                ret = QCM_SOCK_SUCCESS;
            }
            else
            {
                ret = QCM_SOCK_NOT_ALLOW;
            }
        }
        break;
        case QCM_SOCK_STATE_OPT: {
            *((int *)value) = sock->state;
            QLOGV("state: %d", sock->state);
            ret = QCM_SOCK_SUCCESS;
        }
        break;
        default: {
            ret = QCM_SOCK_INVALID_PARAM;
        }
        break;
    }

    return ret;
}

#ifdef CONFIG_QCM_VTLS_FUNC

/**
 * @brief Data reading in SSL blocking mode
 */
static int qcm_socket_ssl_block_read(void *ctx, unsigned char *buf, qosa_size_t len)
{
    qcm_ssl_connect_data_t *ssl_ctx = (qcm_ssl_connect_data_t *)ctx;
    qcm_socket_t           *sock = QOSA_NULL;
    int                     ret = 0;
    int                     last_error = 0;

    sock = qcm_socket_get_prt(ssl_ctx->ssl_config.socket_fd);
    if (sock == QOSA_NULL)
    {
        return -1;
    }

    ret = read(sock->socket_fd, buf, len);
    last_error = lwip_get_error(sock->socket_fd);
    QLOGV("ret=%d", ret);

    if (sock->block == QOSA_TRUE)
    {
        if (ret <= 0)
        {
            ret = QCM_VTLS_IO_ERR_SSL_CONN_RESET;
        }

        return ret;
    }
    if (ret < 0)
    {
        if (last_error == EWOULDBLOCK)
        {
            ret = QCM_VTLS_IO_ERR_SSL_WANT_READ;
        }
        else
        {
            ret = QCM_VTLS_IO_ERR_SSL_CONN_RESET;
        }
    }
    else if (ret == 0)
    {
        if (len == 0)
        {
            ret = QCM_VTLS_IO_ERR_SSL_WANT_READ;
        }
        else
        {
            ret = QCM_VTLS_IO_ERR_SSL_CONN_RESET;
        }
    }

    return ret;
}

/**
 * @brief SSL blocking connection data read function
 */
static int qcm_socket_ssl_block_write(void *ctx, const unsigned char *buf, qosa_size_t len)
{
    qcm_ssl_connect_data_t *ssl_ctx = (qcm_ssl_connect_data_t *)ctx;
    qcm_socket_t           *sock = QOSA_NULL;
    int                     ret = 0;
    int                     last_error = 0;
    QLOGV("socket_fd=%d", ssl_ctx->ssl_config.socket_fd);
    sock = qcm_socket_get_prt(ssl_ctx->ssl_config.socket_fd);
    if (sock == QOSA_NULL)
    {
        return -1;
    }
    ret = write(sock->socket_fd, buf, len);
    last_error = lwip_get_error(sock->socket_fd);
    QLOGV("ret=%d", ret);

    if (sock->block == QOSA_TRUE)
    {
        if (ret <= 0)
        {
            ret = QCM_VTLS_IO_ERR_SSL_CONN_RESET;
        }
        return ret;
    }
    if (ret < 0)
    {
        if (last_error == EWOULDBLOCK)
        {
            ret = QCM_VTLS_IO_ERR_SSL_WANT_WRITE;
        }
        else
        {
            ret = QCM_VTLS_IO_ERR_SSL_CONN_RESET;
        }
    }
    else if (ret == 0)
    {
        if (len == 0)
        {
            ret = QCM_VTLS_IO_ERR_SSL_WANT_WRITE;
        }
        else
        {
            ret = QCM_VTLS_IO_ERR_SSL_CONN_RESET;
        }
    }

    return ret;
}
/**
 * @brief In blocking mode, SSL internally needs to actively perform an independent select to monitor socket fd events, used to provide interactive verification completion.
 */
static int qcm_socket_ssl_block_select(void *ctx, qosa_bool_t read_flag, qosa_bool_t write_flag, qosa_uint32_t wait_time)
{
    qcm_ssl_connect_data_t *ssl_ctx = (qcm_ssl_connect_data_t *)ctx;
    qcm_socket_t           *sock = QOSA_NULL;
    int                     ret = 0;
    fd_set                  readset;
    fd_set                  writeset;
    struct timeval          tm;

    sock = qcm_socket_get_prt(ssl_ctx->ssl_config.socket_fd);
    if (sock == QOSA_NULL)
    {
        return -1;
    }

    FD_ZERO(&readset);
    FD_ZERO(&writeset);
    if (read_flag == QOSA_TRUE)
    {
        FD_SET(sock->socket_fd, &readset);
    }
    if (write_flag == QOSA_TRUE)
    {
        FD_SET(sock->socket_fd, &writeset);
    }

    tm.tv_sec = wait_time;
    tm.tv_usec = 0;

    ret = select(sock->socket_fd + 1, &readset, &writeset, QOSA_NULL, &tm);
    QLOGV("ret=%d", ret);
    return ret;
}

/**
 * @brief In non-blocking mode, after completing the SSL READ operation, it is necessary to add an RD event listener.
 */
static int qcm_socket_ssl_noblock_read(void *ctx, unsigned char *buf, qosa_size_t len)
{
    qcm_ssl_connect_data_t *ssl_ctx = (qcm_ssl_connect_data_t *)ctx;
    qcm_socket_t           *sock = (qcm_socket_t *)ssl_ctx->ssl_config.socket_fd;
    int                     ret = 0;
    int                     last_error = 0;

    QLOGV("socket_fd = %d", sock->socket_fd);
    ret = read(sock->socket_fd, buf, len);
    last_error = lwip_get_error(sock->socket_fd);
    QLOGV("ret=%d last_error=%d EWOULDBLOCK=%d", ret, last_error, EWOULDBLOCK);

    if (ret < 0)
    {
        if (last_error == EWOULDBLOCK)
        {
            qcm_socket_add_monitor(sock, QCM_SOCK_MONITOR_RD);
            ret = QCM_VTLS_IO_ERR_SSL_WANT_READ;
        }
        else
        {
            ret = QCM_VTLS_IO_ERR_SSL_CONN_RESET;
        }
    }
    else if (ret == 0)
    {
        if (len == 0)
        {
            qcm_socket_add_monitor(sock, QCM_SOCK_MONITOR_RD);
            ret = QCM_VTLS_IO_ERR_SSL_WANT_READ;
        }
        else
        {
            ret = QCM_VTLS_IO_ERR_SSL_CONN_RESET;
        }
    }
    else
    {
        //If in non-blocking mode, it will be repeatedly added.
        qcm_socket_add_monitor(sock, QCM_SOCK_MONITOR_RD);
    }

    return ret;
}

/**
 * @brief In non-blocking mode, after completing the SSL WRITE operation, it is necessary to add WR event monitoring.
 */
static int qcm_socket_ssl_noblock_write(void *ctx, const unsigned char *buf, qosa_size_t len)
{
    QLOGV("...");
    qcm_ssl_connect_data_t *ssl_ctx = (qcm_ssl_connect_data_t *)ctx;
    qcm_socket_t           *sock = (qcm_socket_t *)ssl_ctx->ssl_config.socket_fd;
    int                     ret = 0;
    int                     last_error = 0;

    QLOGV("sock->socket_fd = %d", sock->socket_fd);
    ret = write(sock->socket_fd, buf, len);
    last_error = lwip_get_error(sock->socket_fd);
    QLOGV("ret=%d last_error=%d", ret, last_error);

    if (ret < 0)
    {
        if (last_error == EWOULDBLOCK)
        {
            qcm_socket_add_monitor(sock, QCM_SOCK_MONITOR_WR);
            ret = QCM_VTLS_IO_ERR_SSL_WANT_WRITE;
        }
        else
        {
            ret = QCM_VTLS_IO_ERR_SSL_CONN_RESET;
        }
    }
    else if (ret == 0)
    {
        if (len == 0)
        {
            ret = QCM_VTLS_IO_ERR_SSL_WANT_WRITE;
        }
        else
        {
            ret = QCM_VTLS_IO_ERR_SSL_CONN_RESET;
        }
    }
    else
    {
    }

    return ret;
}

/**
 * @brief Configure the SSLconfig information to be used
 *
 * @param[in] int s
 *          - Indicates the socket file descriptor that has been established for communication
 *
 * @param[in] qcm_ssl_config_t * ssl_ptr
 *          - Indicates the SSL CONFIG configuration file pointer
 *
 * @parsm[in] qosa_bool_t block
 *          - Configure whether the SSL handshake session is performed in blocking or non-blocking mode
 *
 * @return int
 *       - Returns 0 on success
 *       - Returns a negative value on failure; for specific information, refer to qcm_sock_err_code.
 */
qcm_sock_err_code qcm_socket_ssl_config(int s, qcm_ssl_config_t *ssl_ptr, const char *hostname, qosa_bool_t block)
{
    qcm_vtls_result_status_e ret = QCM_VTLS_RESULT_OK;
    qcm_socket_t            *sock = QOSA_NULL;
    qcm_ssl_connect_data_t  *ssl_ctx = QOSA_NULL;

    sock = qcm_socket_get_prt(s);
    if (sock == QOSA_NULL)
    {
        return QCM_SOCK_INTERNAL_ERROR;
    }

    if (sock->ssl_ctx != QOSA_NULL)
    {
        return QCM_SOCK_SUCCESS;
    }

    ssl_ctx = qcm_ssl_new(ssl_ptr);
    if (ssl_ctx == QOSA_NULL)
    {
        return QCM_SOCK_SSL_INIT_ERROR;
    }

    ssl_ctx->ssl_config.socket_fd = (qosa_ptr)sock;
    if (block == QOSA_TRUE)
    {
        ssl_ctx->ssl_config.io_read = qcm_socket_ssl_block_read;
        ssl_ctx->ssl_config.io_write = qcm_socket_ssl_block_write;
        ssl_ctx->ssl_config.io_select = qcm_socket_ssl_block_select;
    }
    else
    {
        ssl_ctx->ssl_config.io_read = qcm_socket_ssl_noblock_read;
        ssl_ctx->ssl_config.io_write = qcm_socket_ssl_noblock_write;
    }

    if ((ssl_ctx->ssl_config.sni_enable == QOSA_TRUE || ssl_ctx->ssl_config.session_cache_enable == QOSA_TRUE) && hostname != QOSA_NULL)
    {
        //Copy the host domain name; if SNI is not enabled, the client does not need to perform this step.
        ret = qcm_ssl_set_hostinfo(ssl_ctx, hostname, sock->remote_port);
        QLOGV("sni");
        if (ret != QCM_VTLS_RESULT_OK)
        {
            QLOGE("error = %d", ret);
            qcm_ssl_free(ssl_ctx);
            return QCM_SOCK_OUT_MEM;
        }
    }

    sock->ssl_block = block;
    sock->ssl_ctx = ssl_ctx;
    sock->ssl_enable = QOSA_TRUE;
    sock->ssl_state = QCM_SOC_SSL_STATE_INIT;

    return QCM_SOCK_SUCCESS;
}

/**
 * @brief Check if SSL config has been initialized
 */
qosa_bool_t qcm_socket_ssl_is_init_already(int s)
{
    qcm_socket_t *sock = QOSA_NULL;

    sock = qcm_socket_get_prt(s);
    if (sock == QOSA_NULL)
    {
        return QOSA_FALSE;
    }

    if (sock->ssl_ctx != QOSA_NULL)
    {
        return QOSA_TRUE;
    }

    return QOSA_FALSE;
}

/**
 * @brief ssl Handshake Timeout Timer
 */
static void _ssl_handshark_wait_timeout_cb(void *argv)
{
    qcm_socket_t *sock = (qcm_socket_t *)argv;
    QLOGE("%p", sock);
    QLOGE("%d ssl timeout", sock->num);
    if (sock->event_cb)
    {
        sock->event_cb(sock->num, QCM_SOCK_SSL_HD_TIMEOUT_EVENT, QCM_SOCK_INTERNAL_ERROR, sock->user_argv);
    }
}

/**
 * @brief SSL Connection Authentication
 *
 * The blocking and non-blocking modes are controlled by the block parameter configured in qcm_socket_ssl_config.
 *
 * @param[in] int s
 *          - Indicates the socket file descriptor that has been established for communication
 *
 * @return int
 *       - The function returns 0 upon successful execution.
 *       - Returns a negative value on failure; for specific information, refer to qcm_sock_err_code.
 */
qcm_sock_err_code qcm_socket_ssl_connect(int s)
{
    qcm_vtls_result_status_e ret = QCM_VTLS_RESULT_OK;
    qcm_socket_t            *sock = QOSA_NULL;
    qcm_ssl_connect_data_t  *ssl_ctx = QOSA_NULL;
    qosa_bool_t              done = QOSA_FALSE;

    sock = qcm_socket_get_prt(s);
    if (sock == QOSA_NULL)
    {
        return QCM_SOCK_INTERNAL_ERROR;
    }

    if (sock->ssl_enable != QOSA_TRUE)
    {
        //SSL not initialized
        return QCM_SOCK_SSL_INIT_ERROR;
    }

    ssl_ctx = sock->ssl_ctx;
    sock->ssl_state = QCM_SOC_SSL_STATE_CONNECTING;

    //Establish SSL connection in blocking mode
    if (sock->ssl_block == QOSA_TRUE)
    {
        ret = qcm_ssl_connect(ssl_ctx);
        if (ret == QCM_VTLS_RESULT_OK)
        {
            sock->ssl_state = QCM_SOC_SSL_STATE_CONNECT;
            QLOGV("ssl connect success");
            //SSL connection successful
            return QCM_SOCK_SUCCESS;
        }
        else
        {
            QLOGE("ssl connect error ret = %d", ret);
            //SSL connection failed
            return QCM_SOCK_SSL_CONN_ERROR;
        }
    }
    //Establish SSL connection in non-blocking mode
    else
    {
        ret = qcm_ssl_connect_nonblocking(ssl_ctx, &done);
        if (ret != QCM_VTLS_RESULT_OK)
        {
            QLOGE("ssl connect error = %d", ret);
            //qcm_ssl_free(ssl_ctx);
            return QCM_SOCK_SSL_CONN_ERROR;
        }
        else
        {
            if (done == QOSA_TRUE)
            {
                sock->ssl_state = QCM_SOC_SSL_STATE_CONNECT;
                if (sock->ssl_wait_time != QOSA_NULL)
                {
                    if (qosa_timer_is_running(sock->ssl_wait_time) == QOSA_TRUE)
                    {
                        qosa_timer_stop(sock->ssl_wait_time);
                    }
                    qosa_timer_delete(sock->ssl_wait_time);
                    sock->ssl_wait_time = QOSA_NULL;
                }
                QLOGV("ssl_handshark success");
            }
            else
            {
                if (sock->ssl_wait_time == QOSA_NULL)
                {
                    qosa_timer_create(&sock->ssl_wait_time, _ssl_handshark_wait_timeout_cb, sock);
                    qosa_timer_start(sock->ssl_wait_time, ssl_ctx->ssl_config.ssl_negotiate_timeout * 1000, 0);
                }
                QLOGV("ssl connect continue...");
                return QCM_SOCK_WODBLOCK;
            }
        }
    }
    return QCM_SOCK_SUCCESS;
}

/**
 * @brief Only release SSL resources without closing the socket handle
 *
 * @param[in] int s
 *          - Indicates the socket file descriptor that has been established for communication
 *
 * @return int
 *       - The function returns 0 upon successful execution.
 *       - Returns a negative value on failure; for specific information, refer to qcm_sock_err_code.
 */
qcm_sock_err_code qcm_socket_ssl_clean(int s)
{
    qcm_socket_t *sock = QOSA_NULL;

    sock = qcm_socket_get_prt(s);
    if (sock == QOSA_NULL)
    {
        return QCM_SOCK_INTERNAL_ERROR;
    }

    if (sock->ssl_enable != QOSA_TRUE)
    {
        return QCM_SOCK_SUCCESS;
    }

    if (sock->ssl_enable == QOSA_TRUE)
    {
        if (sock->ssl_wait_time != QOSA_NULL)
        {
            if (qosa_timer_is_running(sock->ssl_wait_time) == QOSA_TRUE)
            {
                qosa_timer_stop(sock->ssl_wait_time);
            }
            qosa_timer_delete(sock->ssl_wait_time);
            sock->ssl_wait_time = QOSA_NULL;
        }
        QLOGV("harry osa clean=%p,s=%d", sock->ssl_ctx, s);
        qcm_ssl_close(sock->ssl_ctx);
        qcm_ssl_free(sock->ssl_ctx);
        sock->ssl_state = QCM_SOC_SSL_STATE_NONE;
        sock->ssl_enable = QOSA_FALSE;
    }
    return QCM_SOCK_SUCCESS;
}

#endif /* CONFIG_QCM_VTLS_FUNC */

/**
 * @brief Socket simulation event notification function, mainly implemented on the Linux side by adjusting the interval time using select.
 *        On the LWIP side, it mainly intercepts net socket layer event notifications to achieve this.
 */
static void qcm_socket_monitor_task(void *pData)
{
    qcm_socket_Event s_event = {0};
    qcm_socket_t    *sock = QOSA_NULL;
    int              ret = 0;

    QOSA_UNUSED(pData);

    for (;;)
    {
        ret = qosa_msgq_wait(g_sock_monitor_msgq, (qosa_uint8_t *)&s_event, sizeof(qcm_socket_Event), QOSA_WAIT_FOREVER);
        if (ret != QOSA_OK)
        {
            QLOGE("...");
            continue;
        }

        sock = qcm_socket_get_ptr_with_socktfd(s_event.fd);
        if (sock == QOSA_NULL)
        {
            QLOGE("...");
            continue;
        }

        if (sock != s_event.user_parm)
        {
            QLOGE("sock=%p s_event.user_parm=%p", sock, s_event.user_parm);
            continue;
        }

        if (s_event.event_mask & QL_DS_READ_EVENT)
        {
            QLOGV("socket %d READABLE, state:%d", sock->socket_fd, sock->state);
            qcm_socket_remove_monitor(sock, QCM_SOCK_MONITOR_RD);
            if (sock->state == QCM_SOC_STATE_LISTEN)
            {
                ret = QCM_SOCK_SUCCESS;
                if ((sock->event_mask & QCM_SOCK_ACCEPT_EVENT) != 0 && sock->event_cb != QOSA_NULL)
                {
                    sock->event_cb(sock->num, QCM_SOCK_ACCEPT_EVENT, ret, sock->user_argv);
                }
            }
            else if (sock->state == QCM_SOC_STATE_CONNECT)
            {
                ret = QCM_SOCK_SUCCESS; /*result code, not state value*/
                if ((sock->event_mask & QCM_SOCK_READ_EVENT) != 0 && sock->event_cb != QOSA_NULL)
                {
                    sock->event_cb(sock->num, QCM_SOCK_READ_EVENT, ret, sock->user_argv);
                }
                else
                {
                    QLOGE("NO QCM_SOCK_READ_EVENT");
                }
            }
            else if (sock->state == QCM_SOC_STATE_CLOSING)
            {
                ret = QCM_SOCK_SUCCESS;
                if ((sock->event_mask & QCM_SOCK_CLOSE_EVENT) != 0 && sock->event_cb != QOSA_NULL)
                {
                    sock->event_cb(sock->num, QCM_SOCK_CLOSE_EVENT, ret, sock->user_argv);
                }
                else
                {
                    QLOGE("NO QCM_SOCK_CLOSE_EVENT");
                }
            }
        }
        if (s_event.event_mask & QL_DS_WRITE_EVENT)
        {
            QLOGV("socket %d state(%d)", sock->socket_fd, sock->state);
            qcm_socket_remove_monitor(sock, QCM_SOCK_MONITOR_WR);
            if (sock->state == QCM_SOC_STATE_CONNECTING)
            {
                int       so_error;
                socklen_t so_error_len = sizeof(so_error);
                getsockopt(sock->socket_fd, SOL_SOCKET, SO_ERROR, &so_error, &so_error_len);
                QLOGV("errno: %d", so_error);
                if (so_error == 0 || so_error == EISCONN)
                {
                    sock->state = QCM_SOC_STATE_CONNECT;
                    ret = QCM_SOCK_SUCCESS;
                    qcm_socket_add_monitor(sock, QCM_SOCK_MONITOR_RD);
                }
                else
                {
                    ret = QCM_SOCK_INTERNAL_ERROR;
                }
                QLOGV("socket %d state(%d)", sock->socket_fd, sock->state);
                if ((sock->event_mask & QCM_SOCK_CONNECT_EVENT) != 0 && sock->event_cb != QOSA_NULL)
                {
                    sock->event_cb(sock->num, QCM_SOCK_CONNECT_EVENT, ret, sock->user_argv);
                }
            }
            else if (sock->state == QCM_SOC_STATE_CONNECT)
            {
                ret = QCM_SOCK_SUCCESS;
                if ((sock->event_mask & QCM_SOCK_WRITE_EVENT) != 0 && sock->event_cb != QOSA_NULL)
                {
                    sock->event_cb(sock->num, QCM_SOCK_WRITE_EVENT, ret, sock->user_argv);
                }
            }
            else if (sock->state == QCM_SOC_STATE_CLOSING)
            {
                ret = QCM_SOCK_SUCCESS;
                if ((sock->event_mask & QCM_SOCK_CLOSE_EVENT) != 0 && sock->event_cb != QOSA_NULL)
                {
                    sock->event_cb(sock->num, QCM_SOCK_CLOSE_EVENT, ret, sock->user_argv);
                }
                else
                {
                    QLOGE("NO QCM_SOCK_CLOSE_EVENT");
                }
            }
        }
#if (defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
        if (s_event.event_mask & QL_DS_SENDACK_EVENT)
        {
            if ((sock->event_mask & QCM_SOCK_SENDENDACK_EVENT) != 0 && sock->event_cb != QOSA_NULL)
            {
                extern int32_t lwip_getAckedSize(int nSocket);
                QLOGV("QCM_SOCK_SENDENDACK_EVENT here sockfd=%d\n ", sock->socket_fd);
                int len = lwip_getAckedSize(sock->socket_fd);
                sock->event_cb(sock->num, QCM_SOCK_SENDENDACK_EVENT, len, sock->user_argv);
            }
        }
#endif
    }
}

/**
 * @brief Find the corresponding socket to notify the disconnection event.
 */
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
static void qcm_socket_report_network_down_event(qosa_uint8_t simid, qosa_uint8_t pdpid)
{
    int           index = 0;
    qcm_socket_t *sock = QOSA_NULL;
    for (index = SOCKET_OFFSET; index < SOCKET_ARRAY_CONNECTED_CNT; index++)
    {
        if (qcm_socket_check_exist(index) == QOSA_FALSE)
        {
            continue;
        }

        sock = qcm_socket_get_prt(index);

        if (sock == QOSA_NULL || sock->sim_id != simid || sock->pdpcid != pdpid)
        {
            continue;
        }

        if (sock->socket_fd != -1)
        {
            if (sock->event_mask & QCM_SOCK_NET_DOWN_EVENT)
            {
                if (sock->event_cb != QOSA_NULL && sock->report_net_down == QOSA_FALSE)
                {
                    //Notify once
                    sock->report_net_down = QOSA_TRUE;
                    sock->event_cb(sock->num, QCM_SOCK_NET_DOWN_EVENT, 0, sock->user_argv);
                }
            }
        }
    }
}

/**
 * @brief Handle PDP network loss event notification. If the SIM CID matches the PDP CID, actively notify the network loss event.
 */
static int qcm_socket_pdp_deact_cb(void *user_argv, void *argv)
{
    QOSA_UNUSED(user_argv);
    qosa_datacall_act_event_t *pdp_act_status = (qosa_datacall_act_event_t *)argv;

    if (pdp_act_status->opt == QOSA_PDP_OPT_ACTIVE)
    {
        return 0;
    }
    qcm_socket_report_network_down_event(pdp_act_status->simid, pdp_act_status->pdpid);
    return 0;
}

/**
 * @brief Handle network disconnection event notification
 */
static int qcm_socket_nw_pdp_deact_notify(void *user_argv, void *argv)
{
    QOSA_UNUSED(user_argv);
    qosa_datacall_act_event_t *notify = (qosa_datacall_act_event_t *)argv;

    qcm_socket_report_network_down_event(notify->simid, notify->pdpid);
    return 0;
}
#endif

static void qcm_socket_monitor_init(void)
{
    int err = 0;

#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
    qosa_event_notify_register(QOSA_EVENT_NET_PDP_ACT, qcm_socket_pdp_deact_cb, QOSA_NULL);
    qosa_event_notify_register(QOSA_EVENT_NW_PDN_DEACT, qcm_socket_nw_pdp_deact_notify, QOSA_NULL);
#endif
    qosa_msgq_create(&g_sock_monitor_msgq, sizeof(qcm_socket_Event), 30);

    err = qosa_task_create(&g_sock_monitor_task, CONFIG_QCM_MONITOR_TASK_STACK_SIZE, QOSA_PRIORITY_ABOVE_NORMAL, "socket", qcm_socket_monitor_task, QOSA_NULL);
    if (err != QOSA_OK)
    {
        QLOGE("task create error! err=%d ", err);
        return;
    }
}

static void qcm_socket_monitor_init_check(void)
{
    if (g_sock_monitor_start_falg == QOSA_FALSE)
    {
        qcm_socket_monitor_init();
        g_sock_monitor_start_falg = QOSA_TRUE;
    }
}

int g_tcp_recv_window_size = QCM_TCPIP_TCP_WINDOW_SIZE_MIN * 1024;
int g_tcp_send_window_size = QCM_TCPIP_TCP_WINDOW_SIZE_MIN * 1024;
int tcp_mss_value = 1360;
#if defined(CONFIG_QOSA_QCX216_PLATFORM_FUNC) || defined(CONFIG_QOSA_QCX217_PLATFORM_FUNC) || defined(CONFIG_QOSA_ASR1903_PLATFORM_FUNC)                       \
    || defined(CONFIG_QOSA_RDA8910_R06_PLATFORM_FUNC) || defined(CONFIG_QOSA_EIGEN718_PLATFORM_FUNC) || defined(CONFIG_QOSA_UIS8852_PLATFORM_FUNC) \
    || defined(CONFIG_QOSA_ASR160X_PLATFORM_FUNC) || defined(CONFIG_QOSA_ASR1805_PLATFORM_FUNC)
extern qosa_uint8_t tcp_tw_recycle;
#endif /* CONFIG_QOSA_QCX216_PLATFORM_FUNC || CONFIG_QOSA_QCX217_PLATFORM_FUNC || CONFIG_QOSA_ASR1903_PLATFORM_FUNC || CONFIG_QOSA_RDA8910_R06_PLATFORM_FUNC || defined(CONFIG_QOSA_EIGEN718_PLATFORM_FUNC) */

/**
 * @brief Set TCP retransmission count
 *
 * @param[in] qosa_uint32_t retrains_cnt
 *          - Set TCP retransmission count
 *
 * @param[in] qosa_uint32_t retrains_period
 *          - Set TCP retransmission time interval
 */
void qcm_socket_set_retrains_cnt(qosa_uint32_t retrains_cnt)
{
    unir_set_lwip_tcp_max_rtx(retrains_cnt);
    unir_set_lwip_tcp_syn_max_rtx(retrains_cnt);
}

/**
 * @brief Set TCP retransmission time interval
 *
 * @param[in] qosa_uint32_t retrains_cnt
 *          - Set TCP retransmission count
 *
 * @param[in] qosa_uint32_t retrains_period
 *          - Set TCP retransmission time interval, unit: ms
 */
void qcm_socket_set_retrains_period(qosa_uint32_t retrains_period)
{
    unir_set_lwip_tcp_rto_init(retrains_period);
}

/**
 * @brief Get TCP retransmission count
 *
 * @param[in] qosa_uint32_t retrains_cnt
 *          - Set TCP retransmission count
 *
 * @param[in] qosa_uint32_t retrains_period
 *          - Set TCP retransmission time interval
 */
void qcm_socket_get_retrains_cnt(qosa_uint32_t *retrains_cnt)
{
    *retrains_cnt = unir_get_lwip_tcp_max_rtx();
}

/**
 * @brief Get TCP retransmission interval
 *
 * @param[in] qosa_uint32_t retrains_cnt
 *          - Set TCP retransmission count
 *
 * @param[in] qosa_uint32_t retrains_period
 *          - Set TCP retransmission time interval, unit: ms
 */
void qcm_socket_get_retrains_period(qosa_uint32_t *retrains_period)
{
    *retrains_period = unir_get_lwip_tcp_rto_init();
}

/**
 * @brief Enable or disable fast PCB release upon disconnection
 *
 * @param[in] qosa_uint32_t fast_recyle
 *          - 0: Disable fast release PCB
 *          - 1: Enable quick-release PCB
 *
 */
void qcm_socket_set_twrecycle(qosa_uint32_t fast_recyle)
{
#if defined(CONFIG_QOSA_QCX216_PLATFORM_FUNC) || defined(CONFIG_QOSA_QCX217_PLATFORM_FUNC) || defined(CONFIG_QOSA_EIGEN718_PLATFORM_FUNC)
    QOSA_UNUSED(fast_recyle);
#else  /* CONFIG_QOSA_QCX216_PLATFORM_FUNC || CONFIG_QOSA_QCX217_PLATFORM_FUNC || defined(CONFIG_QOSA_EIGEN718_PLATFORM_FUNC) */
    unir_set_lwip_tcp_twrecycle(fast_recyle);
#endif /* CONFIG_QOSA_QCX216_PLATFORM_FUNC || CONFIG_QOSA_QCX217_PLATFORM_FUNC || defined(CONFIG_QOSA_EIGEN718_PLATFORM_FUNC) */
}

void qcm_socket_get_twrecycle(qosa_uint32_t *fast_recyle)
{
#if defined(CONFIG_QOSA_QCX216_PLATFORM_FUNC) || defined(CONFIG_QOSA_QCX217_PLATFORM_FUNC) || defined(CONFIG_QOSA_EIGEN718_PLATFORM_FUNC)
    QOSA_UNUSED(fast_recyle);
#else  /* CONFIG_QOSA_QCX216_PLATFORM_FUNC || CONFIG_QOSA_QCX217_PLATFORM_FUNC || defined(CONFIG_QOSA_EIGEN718_PLATFORM_FUNC) */
    *fast_recyle = unir_get_lwip_tcp_twrecycle();
#endif /* CONFIG_QOSA_QCX216_PLATFORM_FUNC || CONFIG_QOSA_QCX217_PLATFORM_FUNC || defined(CONFIG_QOSA_EIGEN718_PLATFORM_FUNC) */
}

/**
 * @brief Set UDP filtering mode
 *
 * @param [in] qosa_uint32_t mode
 *           - 0: Disable UDP filtering mode
 *           - 1: Enable UDP filtering mode
 */
void qcm_socket_set_udp_filter_mode(qosa_uint32_t mode)
{
#if defined(CONFIG_QOSA_ASR1903_PLATFORM_FUNC) || defined(CONFIG_QOSA_UIS8852_PLATFORM_FUNC) || defined(CONFIG_QOSA_ASR1805_PLATFORM_FUNC)
    QOSA_UNUSED(mode);
#elif (defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
    QOSA_UNUSED(mode);
#else  /* CONFIG_QOSA_ASR1903_PLATFORM_FUNC || CONFIG_QOSA_UIS8852_PLATFORM_FUNC || CONFIG_QOSA_ASR1805_PLATFORM_FUNC */
    unir_set_lwip_udp_filter_mode(mode);
#endif /* defined(CONFIG_QOSA_ASR1903_PLATFORM_FUNC) || defined(CONFIG_QOSA_UIS8852_PLATFORM_FUNC) || defined(CONFIG_QOSA_ASR1805_PLATFORM_FUNC) */
}

/**
 * @brief Get UDP filter mode
 */
void qcm_socket_get_udp_filter_mode(qosa_uint32_t *mode)
{
#if (defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
    QOSA_UNUSED(mode);
#else
    *mode = unir_get_lwip_udp_filter_mode();
#endif
}

/**
 * @brief Set the send buffer size
 *
 */
void qcm_socket_set_recv_windows(qosa_uint32_t tcp_recv_windows_size)
{
    unir_set_lwip_tcp_recv_window(tcp_recv_windows_size);
}

/**
 * @brief Get the value of lwip tcp_recv_window
 */
void qcm_socket_get_recv_windows(qosa_uint32_t *tcp_recv_windows_size)
{
    *tcp_recv_windows_size = unir_get_lwip_tcp_recv_window();
}

/**
 * @brief Set receive buffer size
 *
 */
void qcm_socket_set_send_windows(qosa_uint32_t tcp_send_windows_size)
{
    unir_set_lwip_tcp_send_window(tcp_send_windows_size);
}

/**
 * @brief Get the value of lwip tcp_send_window
 */
void qcm_socket_get_send_windows(qosa_uint32_t *tcp_send_windows_size)
{
    *tcp_send_windows_size = unir_get_lwip_tcp_send_window();
}

/**
 * @brief Set Windows scaling ratio
 *
 */
void qcm_socket_set_windows_scale(qosa_uint32_t scale)
{
    unir_set_lwip_tcp_window_scale(scale);
}

/**
 * @brief Get the value of lwip window_scale
 */
void qcm_socket_get_windows_scale(qosa_uint32_t *scale)
{
    *scale = unir_get_lwip_tcp_window_scale();
}

/**
 * @brief Set TCP MSS size
 *
 */
void qcm_socket_set_tcp_mss(qosa_uint32_t tcp_mss)
{
    unir_set_lwip_tcp_mss(tcp_mss);
}

/**
 * @brief Get the value of lwip tcp_mss
 */
void qcm_socket_get_tcp_mss(qosa_uint32_t *tcp_mss)
{
    *tcp_mss = unir_get_lwip_tcp_mss();
}

/**
 * @brief The EC platform needs to set specific socket options based on the socket handle.
 *
 */
void qcm_socket_set_platform_opt(qcm_socket_t *sock)
{
    QOSA_UNUSED(sock);
#if defined(CONFIG_QOSA_QCX216_PLATFORM_FUNC) || defined(CONFIG_QOSA_QCX217_PLATFORM_FUNC) || defined(CONFIG_QOSA_EIGEN718_PLATFORM_FUNC)
    setsockopt(sock->socket_fd, SOL_SOCKET, SO_TCP_MAX_RETRY_TIMES, (void *)&(g_socket_system_cfg.retry_count), sizeof(int));
    setsockopt(sock->socket_fd, SOL_SOCKET, SO_TCP_INIT_RETRY_TIME, (void *)&(g_socket_system_cfg.retry_time), sizeof(int));
    setsockopt(sock->socket_fd, SOL_SOCKET, SO_TCP_TIME_WAIT, (void *)&g_socket_system_cfg.fast_release_timewait, sizeof(int));

    if (g_socket_system_cfg.delay_ack == 1)
    {
        setsockopt(sock->socket_fd, IPPROTO_TCP, TCP_DELAY_AK, (void *)&g_socket_system_cfg.delay_ack, sizeof(int));
    }
    setsockopt(sock->socket_fd, SOL_SOCKET, SO_RCVBUF, (void *)&g_socket_system_cfg.tcp_recv_windows_size, sizeof(int));
    setsockopt(sock->socket_fd, SOL_SOCKET, SO_SNDBUF, (void *)&g_socket_system_cfg.tcp_send_windows_size, sizeof(int));
#elif defined(CONFIG_QOSA_ASR1903_PLATFORM_FUNC) || defined(CONFIG_QOSA_ASR1805_PLATFORM_FUNC)
    if (g_socket_system_cfg.delay_ack == 0)
    {
        setsockopt(sock->socket_fd, IPPROTO_TCP, TCP_DELAY_AK, (void *)&g_socket_system_cfg.delay_ack, sizeof(int));
    }
    setsockopt(sock->socket_fd, SOL_SOCKET, SO_RCVBUF, (void *)&g_socket_system_cfg.tcp_recv_windows_size, sizeof(int));
#endif /* CONFIG_QOSA_QCX216_PLATFORM_FUNC || CONFIG_QOSA_QCX217_PLATFORM_FUNC || defined(CONFIG_QOSA_EIGEN718_PLATFORM_FUNC) */
}

/**
 * @brief Set DNS enable/disable cache
 */
void qcm_socket_set_dns_cache_opt(qosa_uint8_t dns_cache)
{
    QOSA_UNUSED(dns_cache);
/** Platform-specific interface */
#if defined(CONFIG_QOSA_QCX216_PLATFORM_FUNC) || defined(CONFIG_QOSA_QCX217_PLATFORM_FUNC) || defined(CONFIG_QOSA_EIGEN718_PLATFORM_FUNC)
    extern void dns_disable_all_cache(void);
    extern void dns_enable_all_cache(void);
    if (dns_cache == 0)
        dns_disable_all_cache();
    else
        dns_enable_all_cache();
#endif /* CONFIG_QOSA_QCX216_PLATFORM_FUNC || CONFIG_QOSA_QCX217_PLATFORM_FUNC || defined(CONFIG_QOSA_EIGEN718_PLATFORM_FUNC) */
}

/**
 * @brief Set DNS retry count and retransmission interval
 */
void qcm_socket_set_dns_retry_opt(qosa_uint16_t dns_retry_times, qosa_uint8_t dns_retry_cnt)
{
    QOSA_UNUSED(dns_retry_times);
    QOSA_UNUSED(dns_retry_cnt);
/** Platform-specific interface */
#if defined(CONFIG_QOSA_QCX216_PLATFORM_FUNC) || defined(CONFIG_QOSA_QCX217_PLATFORM_FUNC) || defined(CONFIG_QOSA_EIGEN718_PLATFORM_FUNC)
    extern void dns_set_retry_params(int retry_cnt, int retry_interval_time);
    dns_set_retry_params(dns_retry_cnt, dns_retry_times);
#endif /* CONFIG_QOSA_QCX216_PLATFORM_FUNC || CONFIG_QOSA_QCX217_PLATFORM_FUNC || defined(CONFIG_QOSA_EIGEN718_PLATFORM_FUNC) */
}

/**
 * @brief Network IP to Dotted String
 * @param[in] src IP address in network byte order
 * @param[out] dst Dotted-decimal string IP address
 * @param[in] size Size of the dotted-decimal string IP address buffer
 * @note The input parameters are in network byte order, and the output parameters are in dotted-decimal string format, such as 192.168.1.100
 * This function resolves the issue of inconsistent format sizes with previous platforms, converting characters to uppercase for display.
 */
const char *qcm_inet_ntop(int af, const void *src, char *dst, int size)
{
    char *tmp = QOSA_NULL;
    //inet_ntop has different definitions across platforms; typically, the returned string is in uppercase, so it is uniformly converted to uppercase here.
    //inet_ntop standalone usage note: check if the returned string is lowercase
    if (inet_ntop(af, src, dst, size) != QOSA_NULL)
    {
        qosa_touppercase(dst);
        tmp = dst;
    }

    return (const char *)tmp;
}

/**
 * @brief Get TCP last close reason
 *
 * @param[in] int s
 *          - The handle corresponding to the socket to be operated on
 *
 * @return int returns the specific reason for closure
 *
 */
int qcm_socket_get_last_close_event(int s)
{
    QOSA_UNUSED(s);
    //To be modified after adaptation to other platforms in the future
#if defined(CONFIG_QOSA_EIGEN718_PLATFORM_FUNC) || defined(CONFIG_QOSA_ASR1903_PLATFORM_FUNC) || defined(CONFIG_QOSA_ASR160X_PLATFORM_FUNC) || defined(CONFIG_QOSA_ASR1805_PLATFORM_FUNC)
    qcm_socket_t *sock = QOSA_NULL;
    sock = qcm_socket_get_prt(s);
    if (sock == QOSA_NULL)
    {
        return QCM_SOCK_INTERNAL_ERROR;
    }
    return lwip_get_last_close_event(sock->socket_fd);
#else  /* CONFIG_QOSA_EIGEN718_PLATFORM_FUNC */
    return 0;
#endif /* CONFIG_QOSA_EIGEN718_PLATFORM_FUNC */
}

/************************************************************************/
/*Research and Usage of the epoll Function*/
/************************************************************************/
#if 0
/**
 * The idea of using epoll as the socket core was not implemented due to the following issues encountered.
 * 1. The epoll function cannot modify or add corresponding listening events after executing epoll_wait.
 */

static struct epoll_event events[SOCKET_MAX_CONNECTED];

/**
 * @brief Set the socket handle to be added to the epoll monitoring list
 *
 * For edge-triggered epoll on Linux, the following characteristics have been verified through practical testing:
 * 1. When a socket acts as a client, even if it simultaneously listens for both IN and OUT events during a connect, only the OUT event will be reported as ready.
 * 2. When a socket is used as a listening server, an IN event will be reported when data arrives.
 * 3. If a new socket is simultaneously monitored for IN and OUT, an OUT event will be reported immediately.
 * 4. If an IN event reported by a socket is not handled, and the socket is added back to epoll monitoring, the IN event will not be reported again. However, when new data arrives on this socket again, the IN event will still be reported.
 * 5. If the data length returned by calling the send function is not equal to the length of the data sent, and if the out event is being monitored, an out event notification will be reported.
 * 6. If the return value of the send function is not equal to the length of the data to be sent, calling the send function again to continue sending data will return -1 and the state will switch to wouldblock.
 *
 * @param[in] qcm_socket_t * sock
 *          - Corresponding handle information for the operation
 *
 * @param[in] int monitor_evt
 *
 */
static void qcm_socket_add_monitor(qcm_socket_t *sock, int monitor_evt)
{
    struct epoll_event ev;

    ev.data.ptr = sock;
    ev.data.fd = sock->socket_fd;

    if (monitor_evt == QCM_SOCK_MONITOR_RD)
    {
        ev.events = EPOLLIN | EPOLLET;
    }
    if (monitor_evt == QCM_SOCK_MONITOR_WR)
    {
        ev.events = EPOLLIN | EPOLLET | EPOLLOUT;
    }

    if (epoll_ctl(g_epollfd, EPOLL_CTL_ADD, sock->socket_fd, &ev) == -1)
    {
        perror("epoll_ctl");
    }
}

static void qcm_socket_remove_monitor(qcm_socket_t *sock, int monitor_evt)
{
    struct epoll_event ev;

    ev.data.ptr = sock;
    ev.data.fd = sock->socket_fd;
    if (monitor_evt == QCM_SOCK_MONITOR_WR)
    {
        ev.events = EPOLLIN | EPOLLET;
    }
    if (epoll_ctl(g_epollfd, EPOLL_CTL_ADD, sock->socket_fd, &ev) == -1)
    {
        perror("epoll_ctl");
    }
}

static void ql_socket_del_monitor(qcm_socket_t *sock)
{
    //Delete the specified file descriptor
    if (epoll_ctl(g_epollfd, EPOLL_CTL_DEL, sock->socket_fd, QOSA_NULL) == -1)
    {
        perror("epoll_ctl");
    }
}


static void qcm_socket_monitor_task(void *pData)
{
    int         ret = 0;
    int         i = 0;
    int         nfds = 0;  //Number of prepared file descriptors
    qcm_socket_t *sock = QOSA_NULL;

    while (1)
    {
        nfds = epoll_wait(g_epollfd, events, SOCKET_MAX_CONNECTED, -1);
        if (nfds == -1)
        {
            perror("epoll_wait error!");
            QOSA_ASSERT(0);
        }
        for (i = 0; i < nfds; i++)
        {
            sock = (qcm_socket_t *)events[i].data.ptr;
            if (events[i].events & EPOLLIN)
            {
                //Read event is ready
                QLOGV("EPOLLIN num=%d sockfd=%d ", sock->num, sock->socket_fd);
                if (sock->state == QCM_SOC_STATE_LISTEN)
                {
                    ret = QCM_SOCK_SUCCESS;
                    if ((sock->event_mask & QCM_SOCK_ACCEPT_EVENT) != 0 && sock->event_cb != QOSA_NULL)
                    {
                        sock->event_cb(sock->num, QCM_SOCK_ACCEPT_EVENT, ret);
                    }
                }
                else if (sock->state == QCM_SOC_STATE_CONNECT)
                {
                    ret = QCM_SOCK_SUCCESS; /*result code, not state value*/
                    if ((sock->event_mask & QCM_SOCK_READ_EVENT) != 0 && sock->event_cb != QOSA_NULL)
                    {
                        sock->event_cb(sock->num, QCM_SOCK_READ_EVENT, ret);
                    }
                }
                else if (sock->state == QCM_SOC_STATE_CLOSING)
                {
                    ret = QCM_SOCK_SUCCESS;
                    if ((sock->event_mask & QCM_SOCK_CLOSE_EVENT) != 0 && sock->event_cb != QOSA_NULL)
                    {
                        sock->event_cb(sock->num, QCM_SOCK_CLOSE_EVENT, ret);
                    }
                }
            }
            else if (events[i].events & EPOLLOUT)
            {
                //write event is ready
                QLOGV("EPOLLOUT num=%d sockfd=%d ", sock->num, sock->socket_fd);
                if (sock->state == QCM_SOC_STATE_CONNECTING)
                {
                    int       so_error;
                    socklen_t so_error_len = sizeof(so_error);
                    getsockopt(sock->socket_fd, SOL_SOCKET, SO_ERROR, &so_error, &so_error_len);
                    if (so_error == 0 || so_error == EISCONN)
                    {
                        sock->state = QCM_SOC_STATE_CONNECT;
                        ret = QCM_SOCK_SUCCESS;
                    }
                    else
                    {
                        ret = QCM_SOCK_INTERNAL_ERROR;
                    }
                    QLOGV("socket %d state(%d)", sock->socket_fd, sock->state);
                    if ((sock->event_mask & QCM_SOCK_CLOSE_EVENT) != 0 && sock->event_cb != QOSA_NULL)
                    {
                        sock->event_cb(sock->num, QCM_SOCK_CONNECT_EVENT, ret);
                    }
                }
                else if (sock->state == QCM_SOC_STATE_CONNECT)
                {
                    ret = QCM_SOCK_SUCCESS;
                    if ((sock->event_mask & QCM_SOCK_WRITE_EVENT) != 0 && sock->event_cb != QOSA_NULL)
                    {
                        sock->event_cb(sock->num, QCM_SOCK_WRITE_EVENT, ret);
                    }
                }
                if (sock->state == QCM_SOC_STATE_CLOSING)
                {
                    ret = QCM_SOCK_SUCCESS;
                    if ((sock->event_mask & QCM_SOCK_CLOSE_EVENT) != 0 && sock->event_cb != QOSA_NULL)
                    {
                        sock->event_cb(sock->num, QCM_SOCK_CLOSE_EVENT, ret);
                    }
                    else
                    {
                        QLOGV("QCM_SOCK_CLOSE_EVENT");
                    }
                }
                else
                {
                    QLOGV("QCM_SOC_STATE_CLOSING");
                }
            }
            else if (events[i].events & EPOLLERR)
            {
                //Indicates that the corresponding file descriptor has encountered an error
                QLOGV("EPOLLERR num=%d sockfd=%d ", sock->num, sock->socket_fd);
            }
            else if (events[i].events & EPOLLHUP)
            {
                //Indicates that the corresponding file descriptor has been hung up
                QLOGV("EPOLLHUP num=%d sockfd=%d ", sock->num, sock->socket_fd);
            }
            else
            {
                QLOGV("events = %d ", events[i].events);
            }
        }
    }
}

#endif
