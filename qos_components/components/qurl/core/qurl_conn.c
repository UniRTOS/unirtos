/** @file         qurl_conn.c
 *  @brief        Brief Description
 *  @details      Detailed Description
 *  @author       lzm
 *  @date         2023-09-11 15:14:44
 *  @version      v1.0
 *  @copyright    Copyright By lizhuming, All Rights Reserved
 *
 **********************************************************
 *  @LOG Change Log: -----
 **********************************************************
 */

#include "qurl_def_in.h"
#include "qurl_log.h"
#include "qurl_code_in.h"
#include "qurl_core.h"
#include "qurl_port.h"
#include "qurl_url.h"
#include "qurl_select.h"
#if QURL_CFG_ENABLE_TLS
#include "qurl_tls_def.h"
#include "qurl_tls.h"
#endif /* QURL_CFG_ENABLE_TLS */
#include "qurl_conn.h"
#include "qurl_def_basic.h"

extern qurl_core_sec_t g_core_sec[QURL_CFG_CORE_SERVER_MAX];

/*[qurl][todo]connection cache, using array static lock method*/
// static qurl_conn_t *g_conn_cache[QURL_CONN_CACHE_MAX];

qurl_ecode_t qurl_conn_delete(qurl_conn_t *conn_ptr);
qurl_ecode_t qurl_conn_conn_free(qurl_conn_t *conn_ptr, int sock_index);

/*Operations and processing on g_conn_cache*/

#define conn_free(x)                                                                                                                                           \
    do {                                                                                                                                                       \
        if (x)                                                                                                                                                 \
        {                                                                                                                                                      \
            qurl_free(x);                                                                                                                                      \
            x = QOSA_NULL;                                                                                                                                     \
        }                                                                                                                                                      \
    } while (0)

/**
 * @brief By URL matching
 * @return Return the ecode
 */
static qurl_ecode_t conn_match_conn_by_url(qurl_conn_t *conn_ptr, qurl_url_t *uurl_ptr)
{
    qurl_ecode_t ret = QURL_OK;
    int          port = 0;
    char        *portnum = QOSA_NULL;
    char        *host_name = QOSA_NULL;

    if (conn_ptr == QOSA_NULL || uurl_ptr == QOSA_NULL)
    {
        return QURL_ECODE_CONN_CACHE_NOT_MATCH;
    }

    /*1. Port number matching*/
    ret = qurl_url_getopt(uurl_ptr, QURL_URL_OPT_PORT, &portnum, QURL_URL_DEFAULT_PORT);
    if (ret != QURL_OK)
    {
        return QURL_ECODE_CONN_CACHE_NOT_MATCH;
    }
    port = qurl_atoi(portnum);
    qurl_safe_free(portnum);
    if (port != conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].remote_port)
    {
        qurl_logd("port:[%d]:[%d]\r\n", conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].remote_port, port);
        return QURL_ECODE_CONN_CACHE_NOT_MATCH;
    }

    /*2. Host Matching*/
    /** [lzm][note]Note that IPv6 addresses may be passed with brackets [XX::XX], requiring special attention.*/
    ret = qurl_url_getopt(uurl_ptr, QURL_URL_OPT_HOST, &host_name, 0);
    if (ret != QURL_OK)
    {
        return QURL_ECODE_CONN_CACHE_NOT_MATCH;
    }
    else
    {
        char *name_str = QOSA_NULL;

        /*Check and correct IPv6: [xx::xx] ==> xx::xx*/
        name_str = host_name;
        if (name_str && name_str[0] == '[')
        {
            name_str[qurl_strlen(name_str) - 1] = 0;

            host_name = qurl_strdup(name_str + 1);
            qurl_safe_free(name_str);
        }
    }
    if (qurl_strcasecmp(conn_ptr->host_name, host_name) != 0)
    {
        qurl_logd("host_name:[%s]:[%s]\r\n", conn_ptr->host_name, host_name);
        qurl_safe_free(host_name);
        return QURL_ECODE_CONN_CACHE_NOT_MATCH;
    }
    qurl_safe_free(host_name);

    /*Match successful*/
    conn_ptr->lastused_timeval = qurl_time_now();

    return QURL_OK;
}

static qurl_ecode_t conn_match_conn_from_pool(qurl_core_ctrl_t *core_ptr)
{
    /*[qurl][todo]Match old conn*/
    /*1. Check the configuration parameters of the core*/
    /*2. Lock, traverse each connection in the connection pool*/
    /*3. First match the host. Then match the data according to the protocol.*/
    /*4. If the match is successful, detach it from the pool and bind it to the core, check for connection timeout, update conn_ptr->lastused_timeval, and return.*/
    /*5. If the match fails, continue traversing to the next connection.*/
    (void)core_ptr; /*Eliminate compilation warnings*/

    return QURL_ECODE_CONN_CACHE_NOT_MATCH;
}

static qurl_ecode_t conn_delete(qurl_conn_t *conn_ptr)
{
    int i = 0;

    qurl_logd("enter\r\n");

    /*1. Parameter Basic Judgment*/
    if (conn_ptr == QOSA_NULL)
    {
        return QURL_ECODE_PARAM_INVALID;
    }

    /*Release internal resources, such as: freeing socket resources, freeing malloc resources, etc.*/

    /*Release protocol stack data*/
    if (conn_ptr != QOSA_NULL && conn_ptr->port_handler->disconnect_func != QOSA_NULL)
    {
        conn_ptr->port_handler->disconnect_func(conn_ptr, QOSA_FALSE);
    }

    /*Release conn_conn related*/
    qurl_conn_conn_free(conn_ptr, QURL_SOCKET_SERVER_INDEX_0);

    for (i = 0; i < QURL_SOCKET_INDEX_LAST; i++)
    {
        qurl_conn_conn_free(conn_ptr, i);
#if QURL_CFG_ENABLE_TLS
        if (conn_ptr->conn_conn[i].tls_ctrl_ptr)
        {
            qurl_tls_free(conn_ptr->conn_conn[i].tls_ctrl_ptr);
            conn_ptr->conn_conn[i].tls_ctrl_ptr = QOSA_NULL;
        }
#endif /* QURL_CFG_ENABLE_TLS */

        if (conn_ptr->conn_conn[i].recv_cache)
        {
            qurl_dbuf_delete(conn_ptr->conn_conn[i].recv_cache);
        }
        if (conn_ptr->conn_conn[i].send_cache)
        {
            qurl_dbuf_delete(conn_ptr->conn_conn[i].send_cache);
        }
    }

    /*Release malloc*/
    conn_free(conn_ptr->host_name);

    /*Release other resources*/
    if (conn_ptr->username_ptr)
    {
        qurl_free(conn_ptr->username_ptr);
        conn_ptr->username_ptr = QOSA_NULL;
    }
    if (conn_ptr->passwd_ptr)
    {
        qurl_free(conn_ptr->passwd_ptr);
        conn_ptr->passwd_ptr = QOSA_NULL;
    }
    if (conn_ptr->deal_cache_ptr)
    {
        qurl_free(conn_ptr->deal_cache_ptr);
        conn_ptr->deal_cache_ptr = QOSA_NULL;
    }

    qurl_memset(conn_ptr, 0x00, sizeof(qurl_conn_t)); /*Leave no trace*/
    qurl_free(conn_ptr);
    conn_ptr = QOSA_NULL;

    return QURL_OK;
}

/*Domain Name Resolution Section*/
#if 1
/*[lzm][note]Consider whether it is necessary to pass in the hostname to verify that the resolution given to the current connection during this parsing is correct, to avoid pointing to the wrong IP. Personally, I think it is very necessary.
During development, simply assign arg to a data structure containing hostname and conn_ptr.*/
static void conn_resolve_cb(void *arg1, void *arg2)
{
    qurl_event_t      event = {0};
    qurl_addrinfo_t  *addrinfo_ptr = (qurl_addrinfo_t *)arg1;
    qurl_event_t     *arg_ptr = (qurl_event_t *)arg2;
    qurl_conn_t      *conn_ptr = (qurl_conn_t *)arg_ptr->param1;
    qurl_core_ctrl_t *core_ptr = QOSA_NULL;
    qurl_addrinfo_t  *rp = QOSA_NULL;
    int               sec_index = (int)arg_ptr->id;
    int               sock_index = (int)arg_ptr->param2;
    int               count = 0;

    qurl_logi("enter\r\n");

    qurl_free(arg_ptr);
    arg_ptr = QOSA_NULL;

    if (conn_ptr == QOSA_NULL)
    {
        if (addrinfo_ptr != QOSA_NULL)
        {
            qurl_socket_addinfo_free(addrinfo_ptr);
        }
        return;
    }

    core_ptr = conn_ptr->core_ptr;

    if (core_ptr != QOSA_NULL)                                      //core host
    {
        if (sec_index < 0 || sec_index >= QURL_CFG_CORE_SERVER_MAX) /*Parameter Validity Check*/
        {
            if (addrinfo_ptr != QOSA_NULL)
            {
                qurl_socket_addinfo_free(addrinfo_ptr);
            }
            qurl_loge("delete [0x%x] failed\r\n", core_ptr);
            return;
        }
        qurl_mutex_lock(g_core_sec[sec_index].lock, QURL_WAIT_FOREVER);
        if (g_core_sec[sec_index].owner != core_ptr) /*Core Legitimacy Check*/
        {
            if (addrinfo_ptr != QOSA_NULL)
            {
                qurl_socket_addinfo_free(addrinfo_ptr);
            }
            qurl_mutex_unlock(g_core_sec[sec_index].lock);
            return;
        }

        if (conn_ptr->host_name != QOSA_NULL && conn_ptr->conn_conn[sock_index].addrinfo_ptr == QOSA_NULL)  //Currently, the conn has not yet parsed the result.
        {
            conn_ptr->conn_conn[sock_index].addrinfo_ptr = addrinfo_ptr;
            for (rp = addrinfo_ptr; rp != QOSA_NULL; rp = rp->ai_next)
            {
                char  ipstr[QURL_INET6_ADDRSTRLEN] = {0};
                void *addr = QOSA_NULL;
                count++;
                if (rp->ai_family == AF_INET)
                {  // IPv4
                    struct sockaddr_in *ipv4 = (struct sockaddr_in *)rp->ai_addr;
                    addr = &(ipv4->sin_addr);
                }
                else
                {  // IPv6
#if LWIP_IPV6                 
                    struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)rp->ai_addr;
                    addr = &(ipv6->sin6_addr);
#endif
                }
                qurl_inet_ntop(rp->ai_family, addr, ipstr, sizeof(ipstr));
                qurl_logd("dns success[%d][%s]\n", count, ipstr);
            }
            conn_ptr->conn_conn[sock_index].addrinfo_cnt = count;
            qurl_logd("resolve [%s] success\r\n", conn_ptr->host_name);

            if (core_ptr->state <= QURL_STATE_RESOLVE) /*Can be unblocked*/
            {
                event.id = QURL_EVENT_CODE_RESOLVE;
                if (addrinfo_ptr != QOSA_NULL)
                {
                    event.param1 = QURL_OK;
                }
                else
                {
                    event.param1 = QURL_ECODE_CONN_NOT_FOUND_IP;
                }
                qurl_event_send(core_ptr->ipc_event, &event, QURL_WAIT_FOREVER);
            }
        }
        qurl_mutex_unlock(g_core_sec[sec_index].lock);
    }
    else  //conn_cache host
    {
        //[lzm][todo]To be developed when conn_cache is developed

        /*1. Lock*/
        /*2. Connection Validity Check*/
        /*3. Write*/
        /*4. Unlock*/

        if (addrinfo_ptr != QOSA_NULL)
        {
            qurl_socket_addinfo_free(addrinfo_ptr);
        }
    }
}
#else
static void conn_resolve_cb(qurl_addrinfo_t *addrinfo_ptr, void *arg)
{
    qurl_event_t      event = {0};
    qurl_core_ctrl_t *core_ptr = (qurl_core_ctrl_t *)arg;

    if (arg == QOSA_NULL)
    {
        return;
    }

    if (core_ptr->sec_index < 0 || core_ptr->sec_index >= QURL_CFG_CORE_SERVER_MAX) /*Parameter Validity Check*/
    {
        qurl_loge("delete [0x%x] failed\r\n", (unsigned int)core_ptr);
        return QURL_ECODE_PARAM_INVALID;
    }
    qurl_mutex_lock(g_core_sec[core_ptr->sec_index].lock, QURL_WAIT_FOREVER);
    if (g_core_sec[core_ptr->sec_index].owner != core_ptr) /*Core Legitimacy Check*/
    {
        qurl_mutex_unlock(g_core_sec[core_ptr->sec_index].lock);
        return;
    }

    event.id = QURL_EVENT_CODE_RESOLVE;
    event.param1 = addrinfo_ptr;

    qurl_event_send(core_ptr->ipc_event, &event, QURL_WAIT_FOREVER);

    qurl_mutex_unlock(g_core_sec[core_ptr->sec_index].lock);
}
#endif

/**
 * @brief Find the next address of the same address family
 * @return Return the ecode
 */
static qurl_addrinfo_t *conn_addrinfo_next(qurl_conn_t *conn_ptr, int tempindex, qurl_bit_t next)
{
    qurl_addrinfo_t *ai = conn_ptr->temp_addrinfo_ptr[tempindex];
    if (ai && next)
    {
        ai = ai->ai_next;
    }

    while (ai && (ai->ai_family != conn_ptr->temp_family[tempindex]))
    {
        ai = ai->ai_next;
    }
    conn_ptr->temp_addrinfo_ptr[tempindex] = ai;
    return ai;
}

/**
 * @brief Create a conn for the core: a new conn can only be created by depending on the core.
 * @return Return the ecode
 */
static qurl_ecode_t conn_socket(qurl_socket_t *sockfd_ptr, qurl_sockaddr_ex_t *server_sa_ptr)
{
    qurl_ecode_t ret = QURL_OK;

    /*Create socket*/
    *sockfd_ptr = qurl_socket_socket(server_sa_ptr->family, server_sa_ptr->socktype, server_sa_ptr->protocol);
    if (*sockfd_ptr <= QURL_SOCKET_BAD)
    {
        return QURL_ECODE_SOCK_SOCK_FAILED;
    }

    return ret;
}

static qurl_ecode_t conn_nonblock(qurl_socket_t sockfd, int nonblock)
{
    return qurl_socket_nonblock(sockfd, nonblock);
}

static qurl_ecode_t conn_bind(qurl_socket_t sockfd, qurl_sockaddr_ex_t *local_sa_ptr)
{
    qurl_ecode_t ret = QURL_OK;
    char         ip[QURL_INET6_ADDRSTRLEN] = {0};
    const char  *ip_str = qurl_inet_ntop(local_sa_ptr->family, &(local_sa_ptr->addr_u.sai.sin_addr), ip, QURL_INET6_ADDRSTRLEN);

    ret = qurl_socket_bind(sockfd, &local_sa_ptr->addr_u.sa, local_sa_ptr->addrlen);
    if (ret < 0)
    {
        QURL_UNUSED(ip_str);
        qurl_loge("ret:%x, errno:%d, ip:%s\r\n", ret, qurl_socket_get_errno(sockfd), ip_str);
        ret = QURL_ECODE_SOCK_BIND_FAILED;
    }
    else
    {
        ret = QURL_OK;
    }

    return ret;
}

static qurl_ecode_t conn_listen(qurl_socket_t sockfd, int backlog)
{
    qurl_ecode_t ret = QURL_OK;
    int          err = 0;

    ret = qurl_socket_listen(sockfd, backlog);
    if (ret < 0)
    {
        err = qurl_socket_get_errno(sockfd); /*[lzm][todo]Should it be written to conn for recording?*/
        qurl_logd("listen faild ret:%d,err:%d\r\n", ret, err);
        if (ret == -1 && ((err == EINPROGRESS) || (err == EWOULDBLOCK)))
        {
            return QURL_OK;
        }
        return QURL_ECODE_SOCK_LISTEN_FAILED;
    }

    return QURL_OK;
}

static qurl_ecode_t conn_accept(qurl_socket_t sockfd, qurl_sockaddr_ex_t *client_addr_ptr, qurl_socket_t *client_sockfd_ptr)
{
    qurl_ecode_t  ret = QURL_OK;
    int           err = 0;
    qurl_socket_t client_sockfd = QURL_SOCKET_BAD;

    client_sockfd = qurl_socket_accept(sockfd, &client_addr_ptr->addr_u.sa, (socklen_t *)&client_addr_ptr->addrlen);
    if (client_sockfd < 0)
    {
        err = qurl_socket_get_errno(sockfd); /*[lzm][todo]Should it be written to conn for recording?*/
        QURL_UNUSED(ret);
        QURL_UNUSED(err);
        qurl_loge("listen faild ret:%d,err:%d\r\n", ret, err);
        return QURL_ECODE_SOCK_ACCEPT_FAILED;
    }
    *client_sockfd_ptr = client_sockfd;

    return QURL_OK;
}

static qurl_ecode_t conn_connect(qurl_socket_t sockfd, qurl_sockaddr_ex_t *server_addr_ptr)
{
    qurl_ecode_t ret = QURL_OK;
    int          err = 0;

    ret = qurl_socket_connect(sockfd, &server_addr_ptr->addr_u.sa, server_addr_ptr->addrlen);
    if (ret < 0)
    {
        err = qurl_socket_get_errno(sockfd); /*[lzm][todo]Should it be written to conn for recording?*/
        qurl_logd("connect faild ret:%d,err:%d\r\n", ret, err);
        if (ret == -1 && ((err == EINPROGRESS) || (err == EWOULDBLOCK)))
        {
            return QURL_OK;
        }
        return QURL_ECODE_SOCK_CONNECT_FAILED;
    }

    return QURL_OK;
}

static qurl_ecode_t conn_close(qurl_socket_t sockfd)
{
    qurl_ecode_t ret = QURL_OK;

    ret = qurl_socket_close(sockfd);
    if (ret < 0)
    {
        ret = QURL_ECODE_SOCK_CLOSE_FAILED;
    }

    return ret;
}

static qurl_ecode_t conn_single_connect(qurl_conn_t *conn_ptr, qurl_addrinfo_t *server_ai_ptr, int tempindex)
{
    qurl_ecode_t        ret = QURL_OK;
    qurl_core_ctrl_t   *core_ptr = conn_ptr->core_ptr;
    qurl_socket_t       sockfd = QURL_SOCKET_BAD;
    qurl_sockaddr_ex_t  server_sa = {0};
    qurl_sockaddr_ex_t  local_sa = {0};
    qurl_socket_t      *sockfd_ptr = &conn_ptr->temp_sock[tempindex];
    qurl_sockaddr_in_t *sai4 = (qurl_sockaddr_in_t *)&server_sa.addr_u.sas;
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
    qurl_sockaddr_in_t *local_sai4 = (qurl_sockaddr_in_t *)&local_sa.addr_u.sas;
#endif
#if QURL_CFG_ENABLE_IPV6
    qurl_sockaddr_in6_t *sai6 = (qurl_sockaddr_in6_t *)&server_sa.addr_u.sas;
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
    qurl_sockaddr_in6_t *local_sai6 = (qurl_sockaddr_in6_t *)&local_sa.addr_u.sas;
#endif
#endif
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
    qosa_datacall_ip_info_t info = {0};
    qosa_datacall_conn_t    conn = 0;
#endif
    /*Organize data*/
    server_sa.family = server_ai_ptr->ai_family;
    qurl_logd("server_ai_ptr->ai_family=%d, AF_INET6=%d,AF_INET=%d,AF_UNSPEC=%d\n",server_ai_ptr->ai_family,AF_INET6,AF_INET,AF_UNSPEC);
    server_sa.socktype = (conn_ptr->trans_type == QURL_CONN_TRANS_TYPE_TCP) ? SOCK_STREAM : SOCK_DGRAM;
    server_sa.protocol = conn_ptr->trans_type != QURL_CONN_TRANS_TYPE_TCP ? IPPROTO_UDP : IPPROTO_TCP;
    local_sa.family = server_ai_ptr->ai_family;
    local_sa.socktype = (conn_ptr->trans_type == QURL_CONN_TRANS_TYPE_TCP) ? SOCK_STREAM : SOCK_DGRAM;
    local_sa.protocol = conn_ptr->trans_type != QURL_CONN_TRANS_TYPE_TCP ? IPPROTO_UDP : server_ai_ptr->ai_protocol;
    /*Need to distinguish between IPv4 and IPv6*/
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
    conn = qosa_datacall_conn_new(0, core_ptr->usr_cfg.nw_id, QOSA_DATACALL_CONN_TCPIP);
    if (QOSA_DATACALL_OK == qosa_datacall_get_ip_info(conn, &info))
    {
#if QURL_CFG_ENABLE_IPV6
        if ((info.ip_type == QOSA_PDP_IPV6 || info.ip_type == QOSA_PDP_IPV4V6) && (server_sa.family == AF_INET6))
        {
            server_sa.addrlen = sizeof(qurl_sockaddr_in6_t);
            sai6->sin6_family = AF_INET6;
            sai6->sin6_port = qurl_htons(conn_ptr->temp_remote_port);
            sai6->sin6_addr = ((qurl_sockaddr_in6_t *)server_ai_ptr->ai_addr)->sin6_addr;

            local_sa.addrlen = sizeof(qurl_sockaddr_in6_t);
            local_sai6->sin6_family = AF_INET6;
            local_sai6->sin6_port = qurl_htons(0); /*[lzm][todo]To be developed: user-specified local port number*/
            if (core_ptr->usr_cfg.nw_id >= 0)
            {
                qurl_inet_pton(AF_INET6, core_ptr->cfg.nw_ipv6_str, &(local_sai6->sin6_addr));
            }
            else
            {
                local_sai6->sin6_addr = QURL_CFG_PORT_IN6ADDR_ANY_INIT;
            }
        }
        else /* IPV4 */
#endif
            if ((info.ip_type == QOSA_PDP_IPV4 || info.ip_type == QOSA_PDP_IPV4V6) && (server_sa.family == AF_INET))
        {
            server_sa.addrlen = sizeof(qurl_sockaddr_in_t);
            sai4->sin_family = AF_INET;
            sai4->sin_port = qurl_htons(conn_ptr->temp_remote_port);
            sai4->sin_addr = ((qurl_sockaddr_in_t *)server_ai_ptr->ai_addr)->sin_addr;

            local_sa.addrlen = sizeof(qurl_sockaddr_in_t);
            local_sai4->sin_family = AF_INET;
            local_sai4->sin_port = qurl_htons(0); /*[lzm][todo]To be developed: user-specified local port number*/
            if (core_ptr->usr_cfg.nw_id >= 0)
            {
                qurl_inet_pton(AF_INET, core_ptr->cfg.nw_ipv4_str, &(local_sai4->sin_addr));
            }
            else
            {
                local_sai4->sin_addr.s_addr = INADDR_ANY;
            }
        }
        else
        {
            /*Not supported*/
            qurl_loge("family:%d not supported", server_sa.family);
            return QURL_ECODE_CONN_FAMILY_NOT_SUPPORTED;
        }
    }
    else
    {
        /*PDP not activated*/
        qurl_loge("PDP not activated:%d", core_ptr->usr_cfg.nw_id);
        return QURL_ECODE_NETWORK_ERR;
    }
#else
#if QURL_CFG_ENABLE_IPV6
        if (server_sa.family == AF_INET6)
        {
            server_sa.addrlen = sizeof(qurl_sockaddr_in6_t);
            sai6->sin6_family = AF_INET6;
            sai6->sin6_port = qurl_htons(conn_ptr->temp_remote_port);
            sai6->sin6_addr = ((qurl_sockaddr_in6_t *)server_ai_ptr->ai_addr)->sin6_addr;

            
        }
        else /* IPV4 */
#endif
        if (server_sa.family == AF_INET)
        {
            server_sa.addrlen = sizeof(qurl_sockaddr_in_t);
            sai4->sin_family = AF_INET;
            sai4->sin_port = qurl_htons(conn_ptr->temp_remote_port);
            sai4->sin_addr = ((qurl_sockaddr_in_t *)server_ai_ptr->ai_addr)->sin_addr;
        }
        else
        {
            /*Not supported*/
            qurl_loge("family:%d not supported", server_sa.family);
            return QURL_ECODE_CONN_FAMILY_NOT_SUPPORTED;
        }
#endif
    /*Create socket*/
    ret = conn_socket(&sockfd, &server_sa);
    if (ret != QURL_OK)
    {
        return ret;
    }

    /* opt */
#if QURL_CFG_PORT_SOCKET_OPT_SO_LINGER
    if (core_ptr->usr_cfg.so_linger.l_onoff != 0)
    {
        ret = qurl_socket_setsockopt(sockfd, QURL_SOCKET_SOL_SOCKET, SO_LINGER, &(core_ptr->usr_cfg.so_linger), sizeof(core_ptr->usr_cfg.so_linger));
        if (ret != 0)
        {
            conn_close(sockfd);
            return QURL_ECODE_SOCK_SETOPT_FAILED;
        }
    }
#endif /* QURL_CFG_PORT_SOCKET_OPT_SO_LINGER */
#if QURL_CFG_PORT_SOCKET_OPT_SO_KEEPALIVE
    if (core_ptr->usr_cfg.bits.so_keepalive != QOSA_FALSE)
    {
        int keepalive = 1;
        ret = qurl_socket_setsockopt(sockfd, QURL_SOCKET_SOL_SOCKET, QURL_CFG_PORT_SOCKET_OPT_SO_KEEPALIVE, &(keepalive), sizeof(keepalive));
        if (ret != 0)
        {
            conn_close(sockfd);
            return QURL_ECODE_SOCK_SETOPT_FAILED;
        }
#if QURL_CFG_PORT_SOCKET_OPT_TCP_KEEPIDLE
        if (core_ptr->usr_cfg.tcp_keepidle > 0)
        {
            ret = qurl_socket_setsockopt(
                sockfd,
                QURL_SOCKET_IPPROTO_TCP,
                QURL_CFG_PORT_SOCKET_OPT_TCP_KEEPIDLE,
                &(core_ptr->usr_cfg.tcp_keepidle),
                sizeof(core_ptr->usr_cfg.tcp_keepidle)
            );
            if (ret != 0)
            {
                conn_close(sockfd);
                return QURL_ECODE_SOCK_SETOPT_FAILED;
            }
        }
#endif /* QURL_CFG_PORT_SOCKET_OPT_TCP_KEEPIDLE */
#if QURL_CFG_PORT_SOCKET_OPT_TCP_KEEPINTVL
        if (core_ptr->usr_cfg.tcp_keepintvl > 0)
        {
            ret = qurl_socket_setsockopt(
                sockfd,
                QURL_SOCKET_IPPROTO_TCP,
                QURL_CFG_PORT_SOCKET_OPT_TCP_KEEPINTVL,
                &(core_ptr->usr_cfg.tcp_keepintvl),
                sizeof(core_ptr->usr_cfg.tcp_keepintvl)
            );
            if (ret != 0)
            {
                conn_close(sockfd);
                return QURL_ECODE_SOCK_SETOPT_FAILED;
            }
        }
#endif /* QURL_CFG_PORT_SOCKET_OPT_TCP_KEEPINTVL */
#if QURL_CFG_PORT_SOCKET_OPT_TCP_KEEPCNT
        if (core_ptr->usr_cfg.tcp_keepcnt > 0)
        {
            ret = qurl_socket_setsockopt(
                sockfd,
                QURL_SOCKET_IPPROTO_TCP,
                QURL_CFG_PORT_SOCKET_OPT_TCP_KEEPCNT,
                &(core_ptr->usr_cfg.tcp_keepcnt),
                sizeof(core_ptr->usr_cfg.tcp_keepcnt)
            );
            if (ret != 0)
            {
                conn_close(sockfd);
                return QURL_ECODE_SOCK_SETOPT_FAILED;
            }
        }
#endif /* QURL_CFG_PORT_SOCKET_OPT_TCP_KEEPCNT */
    }
#endif /* QURL_CFG_PORT_SOCKET_OPT_SO_KEEPALIVE */
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
     /*Bind local*/
    ret = conn_bind(sockfd, &local_sa);
    if (ret != QURL_OK)
    {
        conn_close(sockfd);
        return ret;
    }
#else
    QOSA_UNUSED(local_sa);
#endif
    /*Control*/
    ret = conn_nonblock(sockfd, QOSA_TRUE);
    if (ret != QURL_OK)
    {
        conn_close(sockfd);
        return ret;
    }

    /*Connect*/
    ret = conn_connect(sockfd, &server_sa);
    if (ret != QURL_OK)
    {
        conn_close(sockfd);
        return ret;
    }

    /*Save information*/
    *sockfd_ptr = sockfd;

    /*Print information*/
    {
        char ipstr[QURL_INET6_ADDRSTRLEN] = {0};
        if (server_sa.family == AF_INET)
        {
            qurl_inet_ntop(server_sa.family, (void *)(&sai4->sin_addr), ipstr, sizeof(ipstr));
        }
#if QURL_CFG_ENABLE_IPV6
        else if (server_sa.family == AF_INET6)
        {
            qurl_inet_ntop(server_sa.family, (void *)(&sai6->sin6_addr), ipstr, sizeof(ipstr));
        }
#endif
        qurl_logd("ai_family: %d\r\n", server_sa.family);
        qurl_logd("addr_len: %d\r\n", server_sa.addrlen);
        qurl_logd("dst: [%s][%d]\r\n", ipstr, conn_ptr->temp_remote_port);
        qurl_logd("socket tmp_id,fd:[%d][%d]\r\n", tempindex, sockfd);
    }

    return ret;
}

static long conn_read(qurl_conn_t *conn_ptr, int sock_index, unsigned char *buf, long buf_size, qurl_ecode_t *ecode_ptr)
{
    long len = 0;
    long err = 0;

    *ecode_ptr = QURL_OK;

#if QURL_CFG_ENABLE_TLS
    if (conn_ptr->conn_conn[sock_index].bits.is_tls)
    {
        qurl_tls_ctrl_t *tls_ctrl_ptr = conn_ptr->conn_conn[sock_index].tls_ctrl_ptr;

        len = qurl_tls_read(tls_ctrl_ptr, buf, buf_size, ecode_ptr);
        if (*ecode_ptr == (qurl_ecode_t)QURL_ECODE_TLS_READ_AGAIN)
        {
            if (len < 0)
            {
                len = 0;
            }
            /*[lab][202406211800]Support returning QURL_ECODE_TLS_READ_AGAIN,
			 * Let the upper layer know that after select indicates readability, the TCP layer indeed has data available to read; it's just that the data length is not yet sufficient for TLS block decryption. Do not mistakenly assume that the TCP connection has been closed.*/
        }
        else if (*ecode_ptr == (qurl_ecode_t)QURL_ECODE_TLS_PEER_CLOSE_NOTIFY)
        {
            conn_ptr->conn_conn[sock_index].bits.rx_closed = QOSA_TRUE;
            if (len < 0)
            {
                len = 0;
            }
            *ecode_ptr = QURL_OK;
        }
    }
    else
#endif /* QURL_CFG_ENABLE_TLS */
    {
        /* socket */
        len = qurl_socket_read(conn_ptr->conn_conn[sock_index].sock_fd, buf, buf_size);
        if (len < 0)
        {
            err = qurl_socket_get_errno(conn_ptr->conn_conn[sock_index].sock_fd);
            if (err == EAGAIN)
            {
                *ecode_ptr = QURL_OK;
                len = 0;
            }
            else
            {
                qurl_loge("conn read failde:%d,%d\r\n", len, err);
                *ecode_ptr = QURL_ECODE_SOCK_READ_FAILED;
            }
        }
        else if (len == 0)
        {
            /*[lab][202405062321]Specifically fixed for LWIP.
			 * Fix the issue where the last packet of data in an FTP data connection includes a FIN. Upon receiving the FIN, the first read in LWIP returns len==0, with errno set to ENOTCONN (lwip err==ERR_CLSD).
			 * and on the next read, len==-1, errno==ENOTCONN (lwip err==ERR_CONN).
			 * Therefore, add an identification marker here to recognize it upon the first receipt.*/
            err = qurl_socket_get_errno(conn_ptr->conn_conn[sock_index].sock_fd);
            if (err == ENOTCONN && conn_ptr->conn_conn[sock_index].bits.trans_connected != QOSA_FALSE)
            {
                qurl_logd("rx_closed:fd[%d]\r\n", conn_ptr->conn_conn[sock_index].sock_fd);
                conn_ptr->conn_conn[sock_index].bits.rx_closed = QOSA_TRUE;
            }
        }
    }

    return len;
}

static long conn_write(qurl_conn_t *conn_ptr, int sock_index, unsigned char *buf, long buf_size, qurl_ecode_t *ecode_ptr)
{
    long len = 0;
    long err = 0;

    *ecode_ptr = QURL_OK;

#if QURL_CFG_ENABLE_TLS
    if (conn_ptr->conn_conn[sock_index].bits.is_tls)
    {
        qurl_tls_ctrl_t *tls_ctrl_ptr = conn_ptr->conn_conn[sock_index].tls_ctrl_ptr;

        len = qurl_tls_write(tls_ctrl_ptr, buf, buf_size, ecode_ptr);

        if (*ecode_ptr == (qurl_ecode_t)QURL_ECODE_TLS_WRITE_AGAIN)
        {
            if (len < 0)
            {
                len = 0;
            }
            *ecode_ptr = QURL_OK;
        }
    }
    else
#endif /* QURL_CFG_ENABLE_TLS */
    {
        /* socket */
        len = qurl_socket_write(conn_ptr->conn_conn[sock_index].sock_fd, buf, buf_size);
        if (len < 0)
        {
            err = qurl_socket_get_errno(conn_ptr->conn_conn[sock_index].sock_fd);
            qurl_logd("conn write failde:%d,%d\r\n", len, err);
            /*[202402241100]Resolve send buffer overflow issue*/
            if (err == EAGAIN || err == EWOULDBLOCK)
            {
                len = 0;
                *ecode_ptr = QURL_OK;
            }
            else
            {
                *ecode_ptr = QURL_ECODE_SOCK_WRITE_FAILED;
            }
        }
    }

    return len;
}

static qurl_ecode_t conn_connected_check(qurl_conn_t *conn_ptr, int sock_index, qurl_bit_t *connected_ptr)
{
    qurl_ecode_t ret = QURL_OK;
    int          i = 0;

    /*1. Check the connection flag*/
    /*2. Socket Status Check*/
    /*3. Whether sockfd is writable, writable means already connected*/

    *connected_ptr = QOSA_FALSE;

    qurl_logd("sock_index: %d", sock_index);

    /*1. Check the connection flag*/
    if (conn_ptr->conn_conn[sock_index].bits.trans_connected == QOSA_TRUE)
    {
        *connected_ptr = QOSA_TRUE;
        return ret;
    }

    /*2. Socket Status Check*/
    if (conn_ptr->state >= QURL_CONN_STATE_SOCKS_INIT && conn_ptr->state < QURL_CONN_STATE_DONE)
    {
        qurl_logd("socket state: %d", conn_ptr->state);
        /*qurl is mainly used for handling socks proxy*/
        /*[lzm][todo]To be developed*/
        return ret;
    }

    /*3. Check if temp_sock is writable; writable means connected. If connected, register it to sock.*/
    for (i = 0; i < 2; i++)
    {
        const int other = i ^ 1;
        if (conn_ptr->temp_sock[i] == QURL_SOCKET_BAD)
        {
            continue;
        }

        /*[lzm][todo]Complete the select implementation first, then return here to continue*/
        //ret = qurl_select(); // Non-blocking check for writability. Or mark as ready when select unblocks before core.
        //Connection successful: update sock value, close happy eyeballs.
        //Connection failed: clear connection.

        ret = qurl_socket_weable(conn_ptr->temp_sock[i]);
        qurl_logd("ret: %d", ret);
        qurl_logd("i: %d, temp_sock: %d", i, conn_ptr->temp_sock[i]);
        if (ret & QURL_SELECT_BIT_ERROR) /*An exception occurred*/
        {
            //connect failed (SYN connection establishment unsuccessful), record the close reason before the socket is destroyed
            //2 CLOSE_EVENT_RST: The server actively sends an RST during the SYN handshake process.
            //3 CLOSE_EVENT_SYN_TIMEOUT: TCP connection closed due to internal SYN retransmission timeout event

            int last_close_event = qurl_socket_get_last_close_event(conn_ptr->temp_sock[i]);

            if (QURL_SOCKET_INDEX_0 == sock_index)
            {
                //HTTP or FTP control connection final closure reason
                conn_ptr->index0_last_close_event = last_close_event;
            }
            else if (QURL_SOCKET_INDEX_1 == sock_index)
            {
                //FTP Data Connection Final Closure Reason
                conn_ptr->index1_last_close_event = last_close_event;
            }

            qurl_socket_close(conn_ptr->temp_sock[i]);
            conn_ptr->temp_sock[i] = QURL_SOCKET_BAD;
            conn_ptr->temp_addrinfo_ptr[i] = QOSA_NULL;

            ret = QURL_ECODE_SOCK_SELECT_FAILED;
        }
        else if (ret == QURL_SELECT_BIT_WRITE) /*Connection successful, writable*/
        {
            char            *ipstr = QOSA_NULL;
            void            *addr = QOSA_NULL;
            qurl_addrinfo_t *conn_ai_ptr = QOSA_NULL;

            /*Update the sock value, close happy eyeballs*/
            conn_ptr->conn_conn[sock_index].sock_fd = conn_ptr->temp_sock[i];
            conn_ptr->temp_sock[i] = QURL_SOCKET_BAD;
            conn_ptr->conn_conn[sock_index].bits.trans_connecting = QOSA_FALSE;
            conn_ptr->conn_conn[sock_index].bits.trans_connected = QOSA_TRUE;
            conn_ptr->conn_conn[sock_index].bits.rx_closed = QOSA_FALSE;
            conn_ptr->conn_conn[sock_index].sock_addrinfo_ptr = conn_ptr->temp_addrinfo_ptr[i];
            conn_ptr->temp_addrinfo_ptr[i] = QOSA_NULL;

            /*Store IP string*/
            conn_ai_ptr = (qurl_addrinfo_t *)conn_ptr->conn_conn[sock_index].sock_addrinfo_ptr;
            if (conn_ptr->conn_conn[sock_index].ip_str != QOSA_NULL)
            {
                qurl_free(conn_ptr->conn_conn[sock_index].ip_str);
                conn_ptr->conn_conn[sock_index].ip_str = QOSA_NULL;
            }
            ipstr = qurl_malloc(QURL_INET6_ADDRSTRLEN + 1);
            if (ipstr != QOSA_NULL)
            {
                qurl_memset(ipstr, 0x00, QURL_INET6_ADDRSTRLEN + 1);

                if (conn_ai_ptr->ai_family == AF_INET)
                {  // IPv4
                    struct sockaddr_in *ipv4 = (struct sockaddr_in *)conn_ai_ptr->ai_addr;
                    addr = &(ipv4->sin_addr);
                }
#if QURL_CFG_ENABLE_IPV6
                else
                {  // IPv6
                    struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)conn_ai_ptr->ai_addr;
                    addr = &(ipv6->sin6_addr);
                }
#else
                else
                {
                    addr = QOSA_NULL;
                }
#endif
                if (addr != QOSA_NULL)
                {
                    qurl_inet_ntop(conn_ai_ptr->ai_family, addr, ipstr, QURL_INET6_ADDRSTRLEN);
                }

                conn_ptr->conn_conn[sock_index].ip_str = ipstr;
            }

            /*Handling happy eyeballs*/
            if (conn_ptr->temp_sock[other] != QURL_SOCKET_BAD)
            {
                qurl_socket_close(conn_ptr->temp_sock[other]);
                conn_ptr->temp_sock[other] = QURL_SOCKET_BAD;
                conn_ptr->temp_addrinfo_ptr[i] = QOSA_NULL;
            }

            *connected_ptr = QOSA_TRUE;
            ret = QURL_OK;
        }
        else /*Not yet connected or other*/
        {
            ret = QURL_OK;
        }

        /*Check if sockfd is writable*/
    }
    if (ret != QURL_OK && (conn_ptr->temp_sock[0] == QURL_SOCKET_BAD) && (conn_ptr->temp_sock[1] == QURL_SOCKET_BAD))
    {
        /*temp_sock has been operated and restored: indicates that all previously connected addresses have failed.*/

        /*[lzm][todo]The previous address connection failed, but it is still possible to continue connecting to subsequent addresses. Refer to curl trynextip*/
        return QURL_ECODE_CONN_CONNECT_FAILD;  //This area is under development, so connection failure is returned for now.
    }
    else
    {
        ret = QURL_OK; /*Ma Dongmei: Haven't even started doing anything yet.*/
    }

    return ret;
}

static qurl_bit_t conn_maxage_timeout(qurl_conn_t *conn_ptr, qurl_core_ctrl_t *core_ptr)
{
    qurl_timeval_t now_timeval = {0};
    qurl_time_t    diff_time_ms = 0;

    now_timeval = qurl_time_now();
    qurl_logd("now_timeval:[%p][%d:%d]\r\n", conn_ptr, now_timeval.s, now_timeval.ms);
    if (core_ptr->usr_cfg.conn_idle_timeout_ms > 0)
    {
        diff_time_ms = qurl_time_diff_ms(conn_ptr->lastused_timeval, now_timeval);
        diff_time_ms = core_ptr->usr_cfg.conn_idle_timeout_ms - diff_time_ms;
        if (diff_time_ms < 0)
        {
            qurl_logd(
                "conn idle timeout:[%ld][%d:%d]\r\n",
                core_ptr->usr_cfg.conn_idle_timeout_ms,
                conn_ptr->lastused_timeval.s,
                conn_ptr->lastused_timeval.ms
            );
            return QOSA_TRUE;
        }
    }
    if (core_ptr->usr_cfg.conn_maxlifetime_ms > 0)
    {
        diff_time_ms = qurl_time_diff_ms(conn_ptr->created_timeval, now_timeval);
        diff_time_ms = core_ptr->usr_cfg.conn_maxlifetime_ms - diff_time_ms;
        if (diff_time_ms < 0)
        {
            qurl_logd("conn idle timeout:[%ld][%d:%d]\r\n", core_ptr->usr_cfg.conn_maxlifetime_ms, conn_ptr->created_timeval.s, conn_ptr->created_timeval.ms);
            return QOSA_TRUE;
        }
    }

    return QOSA_FALSE;
}

static qurl_bit_t conn_if_dead(qurl_conn_t *conn_ptr)
{
    qurl_bit_t        bit_ret = QOSA_FALSE;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;

    /*Time Check*/
    if (core_ptr != QOSA_NULL)
    {
        bit_ret = conn_maxage_timeout(conn_ptr, core_ptr);
        if (bit_ret)
        {
            return QOSA_TRUE;
        }
    }

    /*Connection check [lzm][note]Developable*/
    if (conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].bits.trans_connected)
    {
        qurl_ecode_t ret_select = 0;

        ret_select = qurl_socket_check(conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].sock_fd, QURL_SELECT_BIT_ERROR, 0);
        if (ret_select & QURL_SELECT_BIT_ERROR)
        {
            return QOSA_TRUE;
        }
    }

    return QOSA_FALSE;
}

#if QURL_CFG_ENABLE_TLS
/*[API] Resource Management*/
/**
 * @brief TLS connection
 * @note
 * @return Return the ecode
 */
qurl_ecode_t qurl_conn_upgrade_to_tls(qurl_conn_t *conn_ptr, qurl_core_ctrl_t *core_ptr)
{
    int             i = 0;
    qurl_ecode_t    ret = QURL_OK;
    qurl_bit_t      cfg_alloc_flag = QOSA_FALSE;
    qurl_tls_cfg_t *tls_cfg_ptr = QOSA_NULL;

    if (conn_ptr == QOSA_NULL)
    {
        return QURL_ECODE_PARAM_INVALID;
    }

    if (core_ptr->usr_cfg.tls_cfg_ptr != QOSA_NULL)
    {
        tls_cfg_ptr = core_ptr->usr_cfg.tls_cfg_ptr;
    }
    else
    {
        tls_cfg_ptr = qurl_malloc(sizeof(qurl_tls_cfg_t));
        if (tls_cfg_ptr == QOSA_NULL)
        {
            return QURL_ECODE_NO_MEMORY;
        }
        qurl_tls_cfg_init(tls_cfg_ptr);
        cfg_alloc_flag = QOSA_TRUE;
    }

    for (i = 0; i < QURL_SOCKET_INDEX_LAST; i++)
    {
        if (conn_ptr->conn_conn[i].bits.is_tls != QOSA_TRUE)
        {
            if (conn_ptr->conn_conn[i].tls_ctrl_ptr != QOSA_NULL)
            {
                qurl_tls_close(conn_ptr->conn_conn[i].tls_ctrl_ptr);
                qurl_tls_free(conn_ptr->conn_conn[i].tls_ctrl_ptr);
            }
            conn_ptr->conn_conn[i].tls_ctrl_ptr = qurl_tls_new(tls_cfg_ptr);
            if (conn_ptr->conn_conn[i].tls_ctrl_ptr == QOSA_NULL)
            {
                ret = QURL_ECODE_TLS_CREATE_FAILED;
                break;
            }
            conn_ptr->conn_conn[i].bits.is_tls = QOSA_TRUE;
        }
    }

    if (cfg_alloc_flag)
    {
        qurl_safe_free(tls_cfg_ptr);
    }

    return ret;
}
#endif /* QURL_CFG_ENABLE_TLS */

/**
 * @brief Create a conn for the core: a new conn can only be created by depending on the core.
 * @note It may be retrieved from the cache, or it may be newly created.
 * @return Return the ecode
 */
qurl_ecode_t qurl_conn_create(qurl_core_ctrl_t *core_ptr)
{
    qurl_ecode_t ret = QURL_OK;
    qurl_conn_t *conn_ptr = QOSA_NULL;
    char        *str_ptr = QOSA_NULL;
    int          i = 0;

    qurl_logi("enter");

    /*[qurl][todo]Match old conn*/
    /*1. Check the configuration parameters of the core*/
    if (core_ptr->conn_ptr != QOSA_NULL)
    {
        qurl_bit_t bit_ret = QOSA_FALSE;
        /*[202403181025] Modified state machine logic, when executing a new round of the protocol stack (such as new creation, binding, persistence), it is necessary to re-execute setup_connect_func().*/
        /*[code][lab][port_prot]Execute application protocol function*/
        conn_ptr = core_ptr->conn_ptr;

        /*First, determine if a timeout has occurred*/
        bit_ret = conn_if_dead(conn_ptr);
        if (bit_ret != QOSA_FALSE)
        {
            ret = qurl_conn_delete(conn_ptr); /*When using long connections or persistent connections, deletion is not required.*/
        }
        else
        {
            /*Then determine if it still matches*/
            ret = conn_match_conn_by_url(conn_ptr, core_ptr->cfg.uurl_ptr);
            if (ret == QURL_OK)
            {
                if (conn_ptr->port_handler->setup_connect_func != QOSA_NULL)
                {
                    ret = conn_ptr->port_handler->setup_connect_func(conn_ptr);
                    if (ret != QURL_OK)
                    {
                        goto exit;
                    }
                }
                conn_ptr->bits.is_reuse = QOSA_TRUE; /*Marked for reuse*/
                return QURL_OK;
            }
            else
            {
                /*[lzm][todo]Here we should distinguish between qurl_conn_detach and qurl_conn_delete*/
                if (core_ptr->usr_cfg.bits.reuse_forbid)
                {
                    ret = qurl_conn_delete(conn_ptr); /*When using long connections or persistent connections, deletion is not required.*/
                }
                else
                {
                    // ret = qurl_conn_detach(conn_ptr);
                }
                core_ptr->conn_ptr = QOSA_NULL;
                qurl_logd("ret:[%x]", ret);
            }
        }
    }

    /*2. Check if the flag forces a new creation*/
    if (core_ptr->usr_cfg.bits.reuse_fresh == QOSA_FALSE)
    {
        /*3. Matching*/
        ret = conn_match_conn_from_pool(core_ptr);
        if (ret == QURL_OK)
        {
            /*Match successful*/
            return ret;
        }
    }

    /*4. Create New Connection*/
    conn_ptr = qurl_malloc(sizeof(qurl_conn_t));
    if (conn_ptr == QOSA_NULL)
    {
        return QURL_ECODE_NO_MEMORY;
    }
    qurl_memset(conn_ptr, 0x00, sizeof(qurl_conn_t));

    /*Initialization work*/
    conn_ptr->state = QURL_CONN_STATE_INIT;
    conn_ptr->created_timeval = qurl_time_now();
    conn_ptr->lastused_timeval = conn_ptr->created_timeval;
    qurl_logd("created_timeval:[%p][%d:%d]\r\n", conn_ptr, conn_ptr->created_timeval.s, conn_ptr->created_timeval.ms);

    /*Socket interface related*/
    for (i = 0; i < QURL_SOCKET_INDEX_LAST; i++)
    {
        conn_ptr->conn_conn[i].sock_fd = QURL_SOCKET_BAD;
        /*Determine if a local port number is specified.*/
        conn_ptr->conn_conn[i].local_port = 0;
        conn_ptr->conn_conn[i].remote_port = 0;

        ret = qurl_dbuf_new(&conn_ptr->conn_conn[i].send_cache, QURL_DBUF_NO_LIMIT);
        if (ret != QURL_OK)
        {
            goto exit;
        }
        ret = qurl_dbuf_new(&conn_ptr->conn_conn[i].recv_cache, QURL_DBUF_NO_LIMIT);
        if (ret != QURL_OK)
        {
            goto exit;
        }
    }
    conn_ptr->conn_conn_server.sock_fd = QURL_SOCKET_BAD;
    conn_ptr->temp_sock[0] = QURL_SOCKET_BAD;
    conn_ptr->temp_sock[1] = QURL_SOCKET_BAD;

    /*Determine if a transport layer protocol is specified*/
    conn_ptr->trans_type = QURL_CONN_TRANS_TYPE_TCP;

    /*Matching application protocol stack*/
    /** Get scheme*/
    ret = qurl_url_getopt(core_ptr->cfg.uurl_ptr, QURL_URL_OPT_SCHEME, &str_ptr, 0);
    if (ret != QURL_OK)
    {
        goto exit;
    }
    /** Match*/
    ret = qurl_port_prot_match_by_scheme(str_ptr, &conn_ptr->basic_port_handler);
    if (ret != QURL_OK)
    {
        goto exit;
    }
    conn_ptr->port_handler = conn_ptr->basic_port_handler;
    /** Storage Information*/
    if (core_ptr->cfg.scheme != QOSA_NULL)
    {
        qurl_free(core_ptr->cfg.scheme);
    }
    core_ptr->cfg.scheme = qurl_strdup(str_ptr);
    conn_free(str_ptr);
    str_ptr = QOSA_NULL;

    /* TLS */
#if QURL_CFG_ENABLE_TLS
    if (conn_ptr->basic_port_handler->flags & QURL_PROT_FLAGS_SSL)
    {
        ret = qurl_conn_upgrade_to_tls(conn_ptr, core_ptr);
        if (ret != QURL_OK)
        {
            goto exit;
        }
    }
#endif /* QURL_CFG_ENABLE_TLS */

    /*Obtain some information to core and conn*/
    /** Update part of the core information here instead of in qurl_trans_init, considering the need to update the URL after redirection (of course, if such a situation is not found after actual development is completed, it can be moved back to transfer processing).*/
    /** Username*/
    if (core_ptr->usr_cfg.username_ptr == QOSA_NULL)
    {
        /*Users are only allowed to retrieve from the URL if they have not set a username through the options.*/

        ret = qurl_url_getopt(core_ptr->cfg.uurl_ptr, QURL_URL_OPT_USER, &core_ptr->cfg.user, 0);
        if (ret == QURL_OK)
        {
            char *decoded_ptr = QOSA_NULL;
            ret = qurl_url_decode(
                core_ptr->cfg.user,
                0,
                &decoded_ptr,
                QOSA_NULL,
                conn_ptr->port_handler->flags & QURL_PROT_FLAGS_USERPWDCTRL ? QURL_URL_REJECT_ZERO : QURL_URL_REJECT_CTRL
            );
            if (ret != QURL_OK)
            {
                goto exit;
            }
            qurl_free(str_ptr);
            str_ptr = QOSA_NULL;
            if (core_ptr->cfg.user != QOSA_NULL)
            {
                qurl_free(core_ptr->cfg.user);
                core_ptr->cfg.user = QOSA_NULL;
            }
            core_ptr->cfg.user = decoded_ptr;
            conn_ptr->username_ptr = qurl_strdup(decoded_ptr);
        }
        else if (ret != (qurl_ecode_t)QURL_ECODE_URL_NO_USER)
        {
            goto exit;
        }
    }
    else
    {
        conn_ptr->username_ptr = qurl_strdup(core_ptr->usr_cfg.username_ptr);
    }
    /** Password*/
    if (core_ptr->usr_cfg.password_ptr == QOSA_NULL)
    {
        /*The user is only allowed to fetch from the URL if a password has not been set via the options.*/

        ret = qurl_url_getopt(core_ptr->cfg.uurl_ptr, QURL_URL_OPT_PASSWORD, &str_ptr, 0);
        if (ret == QURL_OK)
        {
            char *decoded_ptr = QOSA_NULL;
            ret = qurl_url_decode(
                str_ptr,
                0,
                &decoded_ptr,
                QOSA_NULL,
                conn_ptr->port_handler->flags & QURL_PROT_FLAGS_USERPWDCTRL ? QURL_URL_REJECT_ZERO : QURL_URL_REJECT_CTRL
            );
            if (ret != QURL_OK)
            {
                goto exit;
            }
            qurl_free(str_ptr);
            str_ptr = QOSA_NULL;
            if (core_ptr->cfg.passwd != QOSA_NULL)
            {
                qurl_free(core_ptr->cfg.passwd);
                core_ptr->cfg.passwd = QOSA_NULL;
            }
            core_ptr->cfg.passwd = decoded_ptr;
            conn_ptr->passwd_ptr = qurl_strdup(decoded_ptr);
            if (conn_ptr->passwd_ptr == QOSA_NULL)
            {
                goto exit;
            }
        }
        else if (ret != (qurl_ecode_t)QURL_ECODE_URL_NO_PASSWORD)
        {
            goto exit;
        }
    }
    else
    {
        conn_ptr->passwd_ptr = qurl_strdup(core_ptr->usr_cfg.password_ptr);
    }

    /*Retrieve some information from the URL to CONN*/
    /** Target hostname*/
    ret = qurl_url_getopt(core_ptr->cfg.uurl_ptr, QURL_URL_OPT_HOST, &conn_ptr->host_name, 0);
    if (ret != QURL_OK)
    {
        goto exit;
    }
    else
    {
        /*Check and correct IPv6: [xx::xx] ==> xx::xx*/
        char *hostname = QOSA_NULL;
        hostname = conn_ptr->host_name;
        if (hostname && hostname[0] == '[')
        {
            hostname[qurl_strlen(hostname) - 1] = 0;

            conn_ptr->host_name = qurl_strdup(hostname + 1);
            qurl_free(hostname);
        }
    }
    if (conn_ptr->host_name == QOSA_NULL)
    {
        goto exit;
    }
    qurl_logd("host_name:%s\r\n", conn_ptr->host_name);
    /** Target host port*/
    if (core_ptr->usr_cfg.port && core_ptr->cfg.bits.allow_port)
    {
        /*Allows the use of user-configured port numbers.*/
        conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].remote_port = (int)core_ptr->usr_cfg.port;
    }
    else
    {
        ret = qurl_url_getopt(core_ptr->cfg.uurl_ptr, QURL_URL_OPT_PORT, &str_ptr, QURL_URL_DEFAULT_PORT);
        if (ret != QURL_OK)
        {
            goto exit;
        }
        conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].remote_port = qurl_atoi(str_ptr);
        conn_free(str_ptr);
        str_ptr = QOSA_NULL;
    }

    conn_ptr->deal_cache_ptr = qurl_malloc(QURL_CONN_DEAL_CACHE_SIZE + 1);
    if (conn_ptr->deal_cache_ptr == QOSA_NULL)
    {
        ret = QURL_ECODE_NO_MEMORY;
        goto exit;
    }
    conn_ptr->deal_cache_ptr[QURL_CONN_DEAL_CACHE_SIZE] = 0;

    /*Flag assignment*/
    /*General*/
    conn_ptr->bits.tcp_fastopen = core_ptr->usr_cfg.bits.tcp_fastopen;
    /** FTP Data Connection Related*/
    conn_ptr->bits.ftp_use_epsv = core_ptr->usr_cfg.bits.ftp_use_epsv;
    conn_ptr->bits.ftp_use_eprt = core_ptr->usr_cfg.bits.ftp_use_eprt;

    /* [lzm][todo]parseurlandfillconn */

    /*conn binds the owner*/
    conn_ptr->core_ptr = core_ptr; /*conn binds to the owner core*/

    /*[code][lab][port_prot]Execute application protocol function*/
    if (conn_ptr->port_handler->setup_connect_func != QOSA_NULL)
    {
        ret = conn_ptr->port_handler->setup_connect_func(conn_ptr);
        if (ret != QURL_OK)
        {
            goto exit;
        }
    }

    /*Initialize index0_last_close_event, index1_last_close_event*/
    conn_ptr->index0_last_close_event = 0;
    conn_ptr->index1_last_close_event = 0;
    conn_ptr->index_error_judgment = 0;  //Used to return values, forced to wait for the corresponding
    /*Owner binding is effective*/
    core_ptr->conn_ptr = conn_ptr; /*core binds the currently used conn*/

    return QURL_OK;
exit:
    if (str_ptr)
    {
        qurl_free(str_ptr);
    }

    if (conn_ptr != QOSA_NULL)
    {
        conn_delete(conn_ptr);
        conn_ptr = QOSA_NULL;
    }
    return ret;
}

qurl_ecode_t qurl_conn_delete(qurl_conn_t *conn_ptr)
{
    qurl_ecode_t ret = QURL_OK;

    qurl_logd("enter\r\n");

    /*1. Parameter Basic Judgment*/
    if (conn_ptr == QOSA_NULL)
    {
        return QURL_ECODE_PARAM_INVALID;
    }

    /*Based on the host providing security*/
    if (conn_ptr->core_ptr != QOSA_NULL)  //core host
    {
#if 0                                     //[lzm][note]Before unbinding the core, it can only be found and safely called by the core. Therefore, no secondary legality check is needed.
		qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;
		if (core_ptr->sec_index < 0 || core_ptr->sec_index >= QURL_CFG_CORE_SERVER_MAX) /*Parameter Validity Check*/
		{
			qurl_loge("delete [0x%x] failed\r\n", (unsigned int)core_ptr);
			return;
		}
		if (core_ptr->owner_thread_id != qurl_get_thread_id()) /*Thread Permission Detection*/
		{
			qurl_loge("perform [0x%x] failed\r\n", (unsigned int)core_ptr);
			return QURL_ECODE_NO_PERMISSION;
		}
		qurl_mutex_lock(g_core_sec[core_ptr->sec_index].lock, QURL_WAIT_FOREVER);
		if (g_core_sec[core_ptr->sec_index].owner != core_ptr) /*Core Legitimacy Check*/
		{
			qurl_mutex_unlock(g_core_sec[core_ptr->sec_index].lock);
			return;
		}
		qurl_mutex_unlock(g_core_sec[core_ptr->sec_index].lock);
#endif
        /*Unbind first*/
        conn_ptr->core_ptr->conn_ptr = QOSA_NULL;

        ret = conn_delete(conn_ptr);
    }
    else  //conn_cache host
    {
        //[lzm][todo]To be developed when conn_cache is developed

        /*1. Lock*/
        /*2. Connection Legitimacy Check*/
        /*3. Delete*/
        /*4. Unlock*/
    }

    return ret;
}

/*[API] APIs for use by other components*/
/**
 * @brief Listen
 *
 * @param conn_ptr
 * @param sock_index
 * @return qurl_ecode_t
 */
qurl_ecode_t qurl_conn_listen(qurl_conn_t *conn_ptr, int backlog)
{
    unsigned short      port = 0;
    qurl_ecode_t        ret = QURL_OK;
    qurl_core_ctrl_t   *core_ptr = conn_ptr->core_ptr;
    qurl_addrinfo_t    *rp_ptr = QOSA_NULL;
    qurl_socket_t       sockfd = QURL_SOCKET_BAD;
    qurl_sockaddr_ex_t  server_sa = {0};
    qurl_sockaddr_ex_t  local_sa = {0};
    qurl_sockaddr_t    *local_usa = (qurl_sockaddr_t *)&local_sa.addr_u.sa;
    qurl_sockaddr_in_t *local_sai4 = (qurl_sockaddr_in_t *)&local_sa.addr_u.sai;
#if QURL_CFG_ENABLE_IPV6
    qurl_sockaddr_in6_t *local_sai6 = (qurl_sockaddr_in6_t *)&local_sa.addr_u.sas;
#endif /* QURL_CFG_ENABLE_IPV6 */
    char *ipstr = QOSA_NULL;
    void *addr = QOSA_NULL;

    server_sa.socktype = (conn_ptr->trans_type == QURL_CONN_TRANS_TYPE_TCP) ? SOCK_STREAM : SOCK_DGRAM;

    qurl_logi("qurl_conn_listen");

    /*Create socket*/
    for (rp_ptr = conn_ptr->conn_conn_server.addrinfo_ptr; rp_ptr != QOSA_NULL; rp_ptr = rp_ptr->ai_next)
    {
        server_sa.family = rp_ptr->ai_family;
        server_sa.protocol = conn_ptr->trans_type != QURL_CONN_TRANS_TYPE_TCP ? IPPROTO_UDP : rp_ptr->ai_protocol;
        ret = conn_socket(&sockfd, &server_sa);
        if (ret != QURL_OK)
        {
            continue;
        }
        break;
    }
    if (sockfd <= QURL_SOCKET_BAD || rp_ptr == QOSA_NULL)
    {
        ret = QURL_ECODE_SOCK_SOCK_FAILED;
        goto exit;
    }

    /* opt */
#if QURL_CFG_PORT_SOCKET_OPT_SO_LINGER
    if (core_ptr->usr_cfg.so_linger.l_onoff != 0)
    {
        ret = qurl_socket_setsockopt(sockfd, QURL_SOCKET_SOL_SOCKET, SO_LINGER, &(core_ptr->usr_cfg.so_linger), sizeof(core_ptr->usr_cfg.so_linger));
        if (ret != 0)
        {
            ret = QURL_ECODE_SOCK_SETOPT_FAILED;
            goto exit;
        }
    }
#endif /* QURL_CFG_PORT_SOCKET_OPT_SO_LINGER */
#if QURL_CFG_PORT_SOCKET_OPT_SO_KEEPALIVE
    if (core_ptr->usr_cfg.bits.so_keepalive != QOSA_FALSE)
    {
        int keepalive = 1;
        ret = qurl_socket_setsockopt(sockfd, QURL_SOCKET_SOL_SOCKET, QURL_CFG_PORT_SOCKET_OPT_SO_KEEPALIVE, &(keepalive), sizeof(keepalive));
        if (ret != 0)
        {
            conn_close(sockfd);
            return QURL_ECODE_SOCK_SETOPT_FAILED;
        }
#if QURL_CFG_PORT_SOCKET_OPT_TCP_KEEPIDLE
        if (core_ptr->usr_cfg.tcp_keepidle > 0)
        {
            ret = qurl_socket_setsockopt(
                sockfd,
                QURL_SOCKET_IPPROTO_TCP,
                QURL_CFG_PORT_SOCKET_OPT_TCP_KEEPIDLE,
                &(core_ptr->usr_cfg.tcp_keepidle),
                sizeof(core_ptr->usr_cfg.tcp_keepidle)
            );
            if (ret != 0)
            {
                conn_close(sockfd);
                return QURL_ECODE_SOCK_SETOPT_FAILED;
            }
        }
#endif /* QURL_CFG_PORT_SOCKET_OPT_TCP_KEEPIDLE */
#if QURL_CFG_PORT_SOCKET_OPT_TCP_KEEPINTVL
        if (core_ptr->usr_cfg.tcp_keepintvl > 0)
        {
            ret = qurl_socket_setsockopt(
                sockfd,
                QURL_SOCKET_IPPROTO_TCP,
                QURL_CFG_PORT_SOCKET_OPT_TCP_KEEPINTVL,
                &(core_ptr->usr_cfg.tcp_keepintvl),
                sizeof(core_ptr->usr_cfg.tcp_keepintvl)
            );
            if (ret != 0)
            {
                conn_close(sockfd);
                return QURL_ECODE_SOCK_SETOPT_FAILED;
            }
        }
#endif /* QURL_CFG_PORT_SOCKET_OPT_TCP_KEEPINTVL */
#if QURL_CFG_PORT_SOCKET_OPT_TCP_KEEPCNT
        if (core_ptr->usr_cfg.tcp_keepcnt > 0)
        {
            ret = qurl_socket_setsockopt(
                sockfd,
                QURL_SOCKET_IPPROTO_TCP,
                QURL_CFG_PORT_SOCKET_OPT_TCP_KEEPCNT,
                &(core_ptr->usr_cfg.tcp_keepcnt),
                sizeof(core_ptr->usr_cfg.tcp_keepcnt)
            );
            if (ret != 0)
            {
                conn_close(sockfd);
                return QURL_ECODE_SOCK_SETOPT_FAILED;
            }
        }
#endif /* QURL_CFG_PORT_SOCKET_OPT_TCP_KEEPCNT */
    }
#endif /* QURL_CFG_PORT_SOCKET_OPT_SO_KEEPALIVE */

    /*Polling to bind local port number*/
    qurl_memcpy(local_usa, rp_ptr->ai_addr, rp_ptr->ai_addrlen);
    local_sa.addrlen = rp_ptr->ai_addrlen;
    for (port = conn_ptr->conn_conn_server.port_min; port <= conn_ptr->conn_conn_server.port_max;)
    {
        if (local_usa->sa_family == AF_INET)
        {
            local_sai4->sin_port = qurl_htons(port);
        }
#if QURL_CFG_ENABLE_IPV6
        else
        {
            local_sai6->sin6_port = qurl_htons(port);
        }
#endif /* QURL_CFG_ENABLE_IPV6 */

        ret = conn_bind(sockfd, &local_sa);
        if (ret != QURL_OK)
        {
            port++;
            continue;
        }
        break;
    }

    if (ret != QURL_OK || port > conn_ptr->conn_conn_server.port_max)
    {
        ret = QURL_ECODE_SOCK_BIND_FAILED;
        goto exit;
    }

    if (port == 0)
    {
        qurl_socklen_t addr_len = 0;

        /*That is a random port number, which needs to be read from the service.*/
        /*Reuse local_sa*/
        if (rp_ptr->ai_family == AF_INET)
        {
            addr_len = sizeof(qurl_sockaddr_in_t);
            ret = qurl_socket_getsockname(sockfd, (qurl_sockaddr_t *)local_sai4, &addr_len);
            if (ret == -1)
            {
                ret = QURL_ECODE_SOCK_BIND_FAILED;
                goto exit;
            }
            port = qurl_ntohs(local_sai4->sin_port);
        }
#if QURL_CFG_ENABLE_IPV6
        else
        {
            addr_len = sizeof(qurl_sockaddr_in6_t);
            ret = qurl_socket_getsockname(sockfd, (qurl_sockaddr_t *)local_sai6, &addr_len);
            if (ret == -1)
            {
                ret = QURL_ECODE_SOCK_BIND_FAILED;
                goto exit;
            }
            port = qurl_ntohs(local_sai6->sin6_port);
        }
#endif
    }

    /* listen */
    ret = conn_listen(sockfd, backlog);
    if (ret != QURL_OK)
    {
        goto exit;
    }

    ipstr = qurl_malloc(QURL_INET6_ADDRSTRLEN + 1);
    if (ipstr != QOSA_NULL)
    {
        qurl_memset(ipstr, 0x00, QURL_INET6_ADDRSTRLEN + 1);

        if (rp_ptr->ai_family == AF_INET)
        {  // IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)rp_ptr->ai_addr;
            addr = &(ipv4->sin_addr);
        }
#if QURL_CFG_ENABLE_IPV6
        else
        {  // IPv6
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)rp_ptr->ai_addr;
            addr = &(ipv6->sin6_addr);
        }
#endif /* QURL_CFG_ENABLE_IPV6 */
        qurl_inet_ntop(rp_ptr->ai_family, addr, ipstr, QURL_INET6_ADDRSTRLEN);
    }

    conn_ptr->conn_conn_server.local_port = port;
    conn_ptr->conn_conn_server.sock_fd = sockfd;
    conn_ptr->conn_conn_server.sock_addrinfo_ptr = rp_ptr;
    conn_ptr->conn_conn_server.ip_str = ipstr;

    return QURL_OK;

exit:
    if (sockfd > QURL_SOCKET_BAD)
    {
        qurl_socket_close(sockfd);
    }

    return ret;
}

/**
 * @brief accept a connection
 *
 * @param conn_ptr
 * @param sock_index[in]  Which socket table entry received the data
 * @return qurl_ecode_t
 */
qurl_ecode_t qurl_conn_accept(qurl_conn_t *conn_ptr, int sock_index)
{
    qurl_ecode_t        ret = QURL_OK;
    qurl_socket_t       sockfd = conn_ptr->conn_conn_server.sock_fd;
    qurl_socket_t       client_sockfd = QURL_SOCKET_BAD;
    qurl_sockaddr_ex_t  client_addr = {0};
    qurl_sockaddr_t    *client_sa = (qurl_sockaddr_t *)&client_addr.addr_u.sa;
    qurl_sockaddr_in_t *client_sai4 = (qurl_sockaddr_in_t *)&client_addr.addr_u.sai;
#if QURL_CFG_ENABLE_IPV6
    qurl_sockaddr_in6_t *client_sai6 = (qurl_sockaddr_in6_t *)&client_addr.addr_u.sai6;
#endif /* QURL_CFG_ENABLE_IPV6 */
    char *ipstr = QOSA_NULL;
    void *addr = QOSA_NULL;

    client_addr.addrlen = sizeof(qurl_sockaddr_t);

    ret = conn_accept(sockfd, &client_addr, &client_sockfd);
    if (ret != QURL_OK)
    {
        return ret;
    }

    qurl_logi("qurl_conn_accept");

    /*Store TCP client information*/
    ipstr = qurl_malloc(QURL_INET6_ADDRSTRLEN + 1);
    if (ipstr != QOSA_NULL)
    {
        qurl_memset(ipstr, 0x00, QURL_INET6_ADDRSTRLEN + 1);

        if (client_sa->sa_family == AF_INET)
        {  // IPv4
            addr = &(client_sai4->sin_addr);
        }
#if QURL_CFG_ENABLE_IPV6
        else
        {  // IPv6
            addr = &(client_sai6->sin6_addr);
        }
#endif /* QURL_CFG_ENABLE_IPV6 */
        qurl_inet_ntop(client_sa->sa_family, addr, ipstr, QURL_INET6_ADDRSTRLEN);
    }

    conn_ptr->conn_conn[sock_index].sock_fd = client_sockfd;
    conn_ptr->conn_conn[sock_index].local_port = conn_ptr->conn_conn_server.local_port;
    if (client_sa->sa_family == AF_INET)
    {
        conn_ptr->conn_conn[sock_index].remote_port = qurl_ntohs(client_sai4->sin_port);
    }
#if QURL_CFG_ENABLE_IPV6
    else
    {
        conn_ptr->conn_conn[sock_index].remote_port = qurl_ntohs(client_sai6->sin6_port);
    }
#endif
    conn_ptr->conn_conn[sock_index].ip_str = ipstr;
    conn_ptr->conn_conn[sock_index].bits.is_server = QOSA_TRUE;       /*Local is a TCP server connection*/
    conn_ptr->conn_conn[sock_index].bits.trans_connecting = QOSA_FALSE;
    conn_ptr->conn_conn[sock_index].bits.trans_connected = QOSA_TRUE; /*Connection established*/
    conn_ptr->conn_conn[sock_index].bits.rx_closed = QOSA_FALSE;

    qurl_logd("accept:[%s][%d]\r\n", conn_ptr->conn_conn[sock_index].ip_str, conn_ptr->conn_conn[sock_index].remote_port);

    return QURL_OK;
}

/**
 * @brief Check if the connection has been completed, including the transport layer and TLS connection (if TLS is used).
 * If it is an application protocol connection (such as FTP requiring successful login), please refer to qurl_conn_core_prot_connected_check
 *
 * @param conn_ptr
 * @param sock_index
 * @return qurl_ecode_t
 */
qurl_bit_t qurl_conn_is_connected(qurl_conn_t *conn_ptr, int sock_index)
{
#if QURL_CFG_ENABLE_TLS
    if (conn_ptr->conn_conn[sock_index].bits.is_tls)
    {
        return conn_ptr->conn_conn[sock_index].bits.tls_connected;
    }
    else
#endif /* QURL_CFG_ENABLE_TLS */
    {
        return conn_ptr->conn_conn[sock_index].bits.trans_connected;
    }
}

/**
 * @brief Establish connection, including protocol connection (tls) (supports reentrancy)
 *
 * @param conn_ptr
 * @param sock_index
 * @return qurl_ecode_t
 */
qurl_ecode_t qurl_conn_connect(qurl_conn_t *conn_ptr, int sock_index, qurl_bit_t *done_ptr)
{
    qurl_bit_t   connected = QOSA_FALSE;
    qurl_ecode_t ret = QURL_OK;

    *done_ptr = QOSA_FALSE;

    if (
#if QURL_CFG_ENABLE_TLS
	    (conn_ptr->conn_conn[sock_index].bits.is_tls == QOSA_TRUE && conn_ptr->conn_conn[sock_index].bits.tls_connected) ||
#endif /* QURL_CFG_ENABLE_TLS */
	    (conn_ptr->conn_conn[sock_index].bits.is_tls == QOSA_FALSE && conn_ptr->conn_conn[sock_index].bits.prot_connected))
    {
        *done_ptr = QOSA_TRUE;
        return QURL_OK;
    }

    // 	if (sock_index > QURL_SOCKET_INDEX_0)
    /*[lzm][note]Since QURL_SOCKET_INDEX_0 is handled in the state machine qurl_conn_core_connect, there is no need to handle QURL_SOCKET_INDEX_0 here.*/
    {
        if (conn_ptr->conn_conn[sock_index].bits.trans_connecting)
        {
            /*Check if connected*/
            ret = conn_connected_check(conn_ptr, sock_index, &connected);
            if (ret != QURL_OK)
            {
                return ret;
            }
        }
        else if (conn_ptr->conn_conn[sock_index].bits.trans_connected == QOSA_FALSE)
        {
            if (conn_ptr->conn_conn[sock_index].addrinfo_cnt <= 0)
            {
                return QURL_ECODE_CONN_CONNECT_FAILED;
            }
            /*[lzm][note]A transport layer connection needs to be initiated. This situation typically occurs when a socket has not even been created yet, usually used for entirely new connections, such as initiating a second connection.
[note]	If a transport layer connection needs to be initiated here, first configure conn_ptr->temp_addrinfo_ptr[0]*/
            conn_ptr->temp_addrinfo_ptr[0] = conn_ptr->conn_conn[sock_index].addrinfo_ptr;
            conn_ptr->temp_family[0] = conn_ptr->temp_addrinfo_ptr[0] ? conn_ptr->temp_addrinfo_ptr[0]->ai_family : 0;
            conn_ptr->temp_remote_port = conn_ptr->conn_conn[sock_index].remote_port;
            ret = conn_single_connect(conn_ptr, conn_ptr->temp_addrinfo_ptr[0], 0);
            if (ret != QURL_OK)
            {
                return ret;
            }
            conn_ptr->conn_conn[sock_index].bits.trans_connecting = QOSA_TRUE;
            return ret;
        }
    }

#if QURL_CFG_ENABLE_TLS
    if (conn_ptr->conn_conn[sock_index].bits.is_tls)
    {
        if (conn_ptr->conn_conn[sock_index].bits.trans_connected)
        {
            /*Configure before connecting*/
            qurl_tls_set_option(conn_ptr->conn_conn[sock_index].tls_ctrl_ptr, QURL_TLS_OPT_SOCK_FD, (void *)&conn_ptr->conn_conn[sock_index].sock_fd);
            qurl_tls_set_option(conn_ptr->conn_conn[sock_index].tls_ctrl_ptr, QURL_TLS_OPT_REMOTE_HOST_NAME, (void *)conn_ptr->host_name);
            qurl_tls_set_option(conn_ptr->conn_conn[sock_index].tls_ctrl_ptr, QURL_TLS_OPT_REMOTE_PORT, (void *)&conn_ptr->conn_conn[sock_index].remote_port);
            qurl_tls_set_option(
                conn_ptr->conn_conn[sock_index].tls_ctrl_ptr,
                QURL_TLS_OPT_MATCH_REMOTE_PORT,
                (void *)&conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].remote_port
            );

            /*Blocking mode connection*/
            ret = qurl_tls_connect(conn_ptr->conn_conn[sock_index].tls_ctrl_ptr);
            if (ret == QURL_OK)
            {
                qurl_logd("[%d]tls connected.\r\n", sock_index);
                conn_ptr->conn_conn[sock_index].bits.tls_connected = QOSA_TRUE;
                *done_ptr = QOSA_TRUE;
            }
            else
            {
                qurl_logd("ret:[%x]\r\n", ret);
            }
        }
        else
        {
            /*QURL_SOCKET_INDEX_0 needs to be completed in the state machine.*/
            qurl_logd("sock_index:[%d]:need connect in core state!!!\r\n", sock_index);
        }
    }
    else
#endif /* QURL_CFG_ENABLE_TLS */
        if (conn_ptr->conn_conn[sock_index].bits.trans_connected)
        {
            // conn_ptr->conn_conn[sock_index].bits.tls_connected = QOSA_TRUE;
            *done_ptr = QOSA_TRUE;
        }

    return ret;
}

qurl_ecode_t qurl_conn_close(qurl_conn_t *conn_ptr, int sock_index)
{
    qurl_ecode_t ret = QURL_OK;

    if (sock_index >= QURL_SOCKET_INDEX_0 && sock_index < QURL_SOCKET_INDEX_LAST)
    {
        if (conn_ptr->conn_conn[sock_index].sock_fd == QURL_SOCKET_BAD)
        {
            return ret;
        }

#if QURL_CFG_ENABLE_TLS
        if (conn_ptr->conn_conn[sock_index].tls_ctrl_ptr)
        {
            qurl_tls_close(conn_ptr->conn_conn[sock_index].tls_ctrl_ptr);
        }
#endif /* QURL_CFG_ENABLE_TLS */

        ret = conn_close(conn_ptr->conn_conn[sock_index].sock_fd);
        if (ret != QURL_OK)
        {
            return ret;
        }

        conn_ptr->conn_conn[sock_index].sock_fd = QURL_SOCKET_BAD;
        conn_ptr->conn_conn[sock_index].bits.trans_connecting = QOSA_FALSE;
        conn_ptr->conn_conn[sock_index].bits.trans_connected = QOSA_FALSE;
        conn_ptr->conn_conn[sock_index].bits.tls_connected = QOSA_FALSE;
        conn_ptr->conn_conn[sock_index].bits.prot_connected = QOSA_FALSE;
        conn_ptr->conn_conn[sock_index].bits.rx_closed = QOSA_FALSE;
    }
    else if (sock_index >= QURL_SOCKET_SERVER_INDEX_0 && sock_index < QURL_SOCKET_SERVER_INDEX_LAST)
    {
        if (conn_ptr->conn_conn_server.sock_fd == QURL_SOCKET_BAD)
        {
            return ret;
        }

        ret = conn_close(conn_ptr->conn_conn_server.sock_fd);
        if (ret != QURL_OK)
        {
            return ret;
        }

        conn_ptr->conn_conn_server.sock_fd = QURL_SOCKET_BAD;
    }
    else
    {
        ret = QURL_ECODE_PARAM_INVALID;
    }

    return ret;
}

/**
 * @brief conn_conn/conn_conn_server fully releases dynamic resources. (Excluding static resources of conn, such as tls's new, dbuf's new)
 * @return
 */
qurl_ecode_t qurl_conn_conn_free(qurl_conn_t *conn_ptr, int sock_index)
{
    qurl_ecode_t ret = QURL_OK;

    qurl_conn_close(conn_ptr, sock_index);

    if (sock_index >= QURL_SOCKET_INDEX_0 && sock_index < QURL_SOCKET_INDEX_LAST)
    {
        if (conn_ptr->conn_conn[sock_index].addrinfo_ptr != QOSA_NULL)
        {
            qurl_socket_addinfo_free(conn_ptr->conn_conn[sock_index].addrinfo_ptr);
            conn_ptr->conn_conn[sock_index].addrinfo_ptr = QOSA_NULL;
        }
        conn_ptr->conn_conn[sock_index].addrinfo_cnt = 0;
        qurl_safe_free(conn_ptr->conn_conn[sock_index].ip_str);
    }
    else if (sock_index >= QURL_SOCKET_SERVER_INDEX_0 && sock_index < QURL_SOCKET_SERVER_INDEX_LAST)
    {
        if (conn_ptr->conn_conn_server.addrinfo_ptr != QOSA_NULL)
        {
            qurl_socket_addinfo_free(conn_ptr->conn_conn_server.addrinfo_ptr);
            conn_ptr->conn_conn_server.addrinfo_ptr = QOSA_NULL;
        }
        conn_ptr->conn_conn_server.addrinfo_cnt = 0;
        qurl_safe_free(conn_ptr->conn_conn_server.ip_str);
    }
    else
    {
        ret = QURL_ECODE_PARAM_INVALID;
    }

    return ret;
}

/**
 * @brief Send connection buffer data
 * @return
 */
qurl_ecode_t qurl_conn_send_cache(qurl_conn_t *conn_ptr, int sock_index)
{
    qurl_ecode_t ret = QURL_OK;
    long         len = 0;
    char         continue_flag = 1;
    long         send_data_len = 0;

    while (continue_flag)
    {
        continue_flag = 0;
        send_data_len = QURL_CONN_DEAL_CACHE_SIZE;
        ret = qurl_dbuf_read_data_try(conn_ptr->conn_conn[sock_index].send_cache, conn_ptr->deal_cache_ptr, &send_data_len, 0);
        if (ret != QURL_OK || send_data_len == 0)
        {
            break;
        }

        len = conn_write(conn_ptr, sock_index, conn_ptr->deal_cache_ptr, send_data_len, &ret);
        qurl_logd("test len:[%ld][%d]ret:[%x]\r\n", len, send_data_len, ret);
        if (len > 0)
        {
            qurl_dbuf_pull_data(conn_ptr->conn_conn[sock_index].send_cache, QOSA_NULL, len);
            ret = QURL_OK;
            continue_flag = 1;
        }
        else if (ret != QURL_OK)
        {
            return ret;
        }
        else
        {
            break;
        }
    }

    return ret;
}

/**
 * @brief Receive data to the conn buffer
 * @return
 */
qurl_ecode_t qurl_conn_recv_cache(qurl_conn_t *conn_ptr, int sock_index, long max_size)
{
    qurl_ecode_t ret = QURL_OK;
    long         len = 0;
    long         recv_data_len = 0;
    long         recv_max_len = 0;

    /*[lab][202405062321] Since it has already been marked as a legitimate TCP closure, there is no need to fetch data from the socket anymore. (Decoupling the subtle differences in socket standard interfaces across various TCP/IP protocol stacks)*/
    if (conn_ptr->conn_conn[sock_index].bits.rx_closed != QOSA_FALSE)
    {
        return QURL_OK;
    }

    recv_max_len = qurl_dbuf_vaild_size(conn_ptr->conn_conn[sock_index].recv_cache);
    if (recv_max_len >= max_size)
    {
        return QURL_OK;
    }
    recv_max_len = max_size - recv_max_len;

    while (recv_data_len < recv_max_len)
    {
        len = conn_read(conn_ptr, sock_index, conn_ptr->deal_cache_ptr, recv_max_len - recv_data_len, &ret);
        if (len > 0)
        {
            ret = qurl_dbuf_push_data(conn_ptr->conn_conn[sock_index].recv_cache, conn_ptr->deal_cache_ptr, len);
            recv_data_len += len;
            ret = QURL_OK;
        }
        else if (ret != QURL_OK)
        {
            /*[lzm][note]Refer to the 202402051200 note. Mainly to avoid simultaneously receiving the last packet of data and a FIN, which could cause an error response to be sent to the upper layer after new data is received here, misleading the upper layer's processing.*/
            if (recv_data_len > 0)
            {
                ret = QURL_OK;
            }
            return ret;
        }
        else
        {
            break;
        }
    }

    return ret;
}

/**
 * @brief The length of data in the receive buffer of conn
 * @return
 */
long qurl_conn_recv_cache_paending(qurl_conn_t *conn_ptr, int sock_index)
{
    return qurl_dbuf_vaild_size(conn_ptr->conn_conn[sock_index].recv_cache);
}

/*[API] APIs exclusively for core state machine usage*/
/**
 * @brief Connection detachment
 * @note Detachment can only be controlled by the core. Detachment requires checking for recycling into g_conn_cache; if the conn cannot be placed into g_conn_cache, it must be destroyed on the spot.
 * @return Return the ecode
 */
qurl_ecode_t qurl_conn_core_detach(qurl_core_ctrl_t *core_ptr)
{
    /*[lzm][todo]To be developed*/
    (void)core_ptr; /*Eliminate warnings*/

    return QURL_OK;
}

/*Domain Name Resolution Section*/
qurl_ecode_t qurl_conn_core_resolve_check(qurl_conn_t *conn_ptr)
{
    if (conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].addrinfo_cnt != 0 && conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].addrinfo_ptr != QOSA_NULL)
    {
        return QURL_OK;
    }
    else
    {
        return QURL_ECODE_CONN_NOT_RESOLVE; /*No parsing result yet*/
    }
}

qurl_ecode_t qurl_conn_core_resolve(qurl_conn_t *conn_ptr)
{
    qurl_ecode_t      ret = 0;
    qurl_event_t     *arg_ptr = QOSA_NULL;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;

    /*[note] If DNS resolution fails, the core must be notified to avoid re-entering the resolve process, otherwise it will fall into a resolve loop before timeout.*/

    if (core_ptr == QOSA_NULL)
    {
        /*[lzm][note]The connection must be bound to a core to initiate domain name resolution. It can be modified.*/
        return QURL_ECODE_CONN_NOT_ATTACH;
    }

    arg_ptr = qurl_malloc(sizeof(qurl_event_t));
    if (arg_ptr == QOSA_NULL)
    {
        return QURL_ECODE_NO_MEMORY;
    }

    arg_ptr->id = (unsigned long)(core_ptr->sec_index);
    arg_ptr->param1 = (unsigned long)conn_ptr;
    arg_ptr->param2 = (unsigned long)QURL_SOCKET_INDEX_0;

    ret = qurl_socket_resolve_async(
        conn_ptr->host_name,
        conn_resolve_cb,
        arg_ptr,
        conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].resolve_addrtype,
    #if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
        core_ptr->usr_cfg.nw_id
    #else
        -1
    #endif
    );

    return ret;
}

qurl_ecode_t qurl_conn_core_connect(qurl_conn_t *conn_ptr)
{
    qurl_ecode_t ret = 0;
    int          i = 0;

    /*[lzm][todo]Prepare to connect data*/
    if (conn_ptr->temp_sock[0] != QURL_SOCKET_BAD || conn_ptr->temp_sock[1] != QURL_SOCKET_BAD)
    {
        return ret;
    }

    /* happy eyeballs */
    conn_ptr->temp_addrinfo_ptr[0] = conn_ptr->temp_addrinfo_ptr[1] = conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].addrinfo_ptr;
    // conn_ptr->temp_sock[0] = conn_ptr->temp_sock[1] = QURL_SOCKET_BAD;
    conn_ptr->temp_family[0] = conn_ptr->temp_addrinfo_ptr[0] ? conn_ptr->temp_addrinfo_ptr[0]->ai_family : 0;
#if QURL_CFG_ENABLE_IPV6
    conn_ptr->temp_family[1] = conn_ptr->temp_family[0] == AF_INET6 ? AF_INET : AF_INET6;
#else
    conn_ptr->temp_family[1] = AF_UNSPEC;
#endif
    conn_ptr->temp_remote_port = conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].remote_port;
    conn_addrinfo_next(conn_ptr, 1, QOSA_TRUE); /*Overlap is meaningless, so it is corrected here.*/

    ret = QURL_ECODE_CONN_CONNECT_FAILED;
    for (i = 0; i < 2 && ret != QURL_OK; i++)
    {
        while (conn_ptr->temp_addrinfo_ptr[i])
        {
            qurl_logd("qurl socket create");
            ret = conn_single_connect(conn_ptr, conn_ptr->temp_addrinfo_ptr[i], i);
            if (ret == QURL_OK)
            {
                //core_perform => qurl_conn_core_connect => conn_single_connect => conn_socket => qurl_socket_socket => socket/lwip_socket
                //socket => lwip_socket => netconn_new_with_proto_and_callback => netconn_alloc
                //In netconn_alloc, conn->last_close_event is set to CLOSE_EVENT_NORMAL, and it is also initialized synchronously here.

                //This is the establishment of HTTP or FTP control connections, initializing index0_last_close_event
                conn_ptr->index0_last_close_event = 0;
                break;
            }
            conn_addrinfo_next(conn_ptr, i, QOSA_TRUE);
        }
    }

    return ret;
}

qurl_ecode_t qurl_conn_core_connected_check(qurl_conn_t *conn_ptr, int sock_index, qurl_bit_t *connected_ptr)
{
    return conn_connected_check(conn_ptr, sock_index, connected_ptr);
}

/**
 * @brief Protocol Connection Check
 *
 * @param conn_ptr
 * @param sock_index
 * @param connected_ptr
 * @return qurl_ecode_t
 */
qurl_ecode_t qurl_conn_core_prot_connected_check(qurl_conn_t *conn_ptr, int sock_index, qurl_bit_t *connected_ptr)
{
    qurl_ecode_t ret = QURL_OK;

    *connected_ptr = QOSA_FALSE;

    /*1. Check the connection flag*/
    if (conn_ptr->conn_conn[sock_index].bits.prot_connected == QOSA_TRUE)
    {
        *connected_ptr = QOSA_TRUE;
        return ret;
    }

    if (conn_ptr->port_handler->send_prot_connect_func == QOSA_NULL)
    {
        *connected_ptr = QOSA_TRUE;
        return ret;
    }

    return ret;
}

/**
 * @brief Initiate protocol connection
 * @return
 */
qurl_ecode_t qurl_conn_core_send_prot_connect(qurl_conn_t *conn_ptr, qurl_bit_t *done)
{
    qurl_ecode_t ret = QURL_OK;

    if (conn_ptr->port_handler->send_prot_connect_func != QOSA_NULL)
    {
        *done = QOSA_FALSE;
        ret = conn_ptr->port_handler->send_prot_connect_func(conn_ptr, done);
        if (ret == QURL_OK && *done == QOSA_TRUE)
        {
            conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].bits.prot_connected = QOSA_TRUE;
        }
    }
    else
    {
        conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].bits.prot_connected = QOSA_TRUE;
        *done = QOSA_TRUE; /*No protocol connection required*/
    }

    return ret;
}

/**
 * @brief Connecting to protocol
 * @return
 */
qurl_ecode_t qurl_conn_core_prot_connect(qurl_conn_t *conn_ptr, qurl_bit_t *done)
{
    qurl_ecode_t ret = QURL_OK;

    if (conn_ptr->port_handler->prot_connect_func != QOSA_NULL)
    {
        *done = QOSA_FALSE;
        ret = conn_ptr->port_handler->prot_connect_func(conn_ptr, done);
        if (ret == QURL_OK && *done == QOSA_TRUE)
        {
            conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].bits.prot_connected = QOSA_TRUE;
        }
    }
    else
    {
        conn_ptr->conn_conn[QURL_SOCKET_INDEX_0].bits.prot_connected = QOSA_TRUE;
        *done = QOSA_TRUE; /*No protocol connection required*/
    }

    return ret;
}

/**
 * @brief Data transmission completed
 * @return
 */
qurl_ecode_t qurl_conn_core_req(qurl_conn_t *conn_ptr, qurl_bit_t *done)
{
    qurl_ecode_t ret = QURL_OK;

    /*Core initialization of connection request status*/
    conn_ptr->bits.req_more = QOSA_FALSE; /*[lzm][note]By default, more is not executed; it needs to be triggered by enabling the protocol stack.*/

    if (conn_ptr->port_handler->req_func != QOSA_NULL)
    {
        *done = QOSA_FALSE;
        ret = conn_ptr->port_handler->req_func(conn_ptr, done);
    }
    else
    {
        *done = QOSA_TRUE; /*No need to initiate a request*/
    }

    return ret;
}

/**
 * @brief Data transmission in progress
 * @return
 */
qurl_ecode_t qurl_conn_core_reqing(qurl_conn_t *conn_ptr, qurl_bit_t *done)
{
    qurl_ecode_t ret = QURL_OK;

    if (conn_ptr->port_handler->reqing_func != QOSA_NULL)
    {
        *done = QOSA_FALSE;
        ret = conn_ptr->port_handler->reqing_func(conn_ptr, done);
    }
    else
    {
        *done = QOSA_TRUE; /*No need to initiate a request*/
    }

    return ret;
}

/**
 * @brief Second stage request
 * @return
 */
qurl_ecode_t qurl_conn_core_req_more(qurl_conn_t *conn_ptr, int *completep_ptr)
{
    qurl_ecode_t ret = QURL_OK;

    *completep_ptr = 0;
    if (conn_ptr->port_handler->req_more_func != QOSA_NULL)
    {
        ret = conn_ptr->port_handler->req_more_func(conn_ptr, completep_ptr);
    }
    else
    {
        *completep_ptr = 1;
    }

    return ret;
}

/**
 * @brief Data Transmission
 * @return
 */
qurl_ecode_t qurl_conn_core_trans_data(qurl_conn_t *conn_ptr, qurl_bit_t *done)
{
    qurl_ecode_t ret = QURL_OK;

    if (conn_ptr->port_handler->trans_data_func != QOSA_NULL)
    {
        *done = QOSA_FALSE;
        ret = conn_ptr->port_handler->trans_data_func(conn_ptr, done);
    }
    else
    {
        *done = QOSA_TRUE; /*No data transmission is required*/
    }

    return ret;
}

/**
 * @brief Data transmission completed
 * @param[in] premature: Whether it is closed prematurely
 * @return
 */
qurl_ecode_t qurl_conn_core_trans_done(qurl_conn_t *conn_ptr, qurl_ecode_t ecode, qurl_bit_t premature)
{
    qurl_ecode_t      ret = QURL_OK;
    qurl_ecode_t      trans_done_ret = QURL_OK;
    qurl_core_ctrl_t *core_ptr = conn_ptr->core_ptr;

    qurl_logd("enter\r\n");

    if (conn_ptr->port_handler->trans_done_func != QOSA_NULL)
    {
        trans_done_ret = conn_ptr->port_handler->trans_done_func(conn_ptr, ecode, premature);
    }
    /*[lzm][todo]Here we should distinguish between qurl_conn_detach and qurl_conn_delete*/
    if (core_ptr->usr_cfg.bits.reuse_hold == QOSA_FALSE)
    {
        if (core_ptr->usr_cfg.bits.reuse_forbid)
        {
            ret = qurl_conn_delete(conn_ptr); /*When using long connections or persistent connections, deletion is not required.*/
        }
        else
        {
            // ret = qurl_conn_detach(conn_ptr);
        }
    }

    return trans_done_ret != QURL_OK ? trans_done_ret : ret;
}
