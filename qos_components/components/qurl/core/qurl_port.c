/** @file         qurl_port.c
 *  @brief        Brief description: Southbound part
 *  @details      Detailed Description
 *  @author       lzm
 *  @date         2023-09-05 16:42:31
 *  @version      v1.0
 *  @copyright    Copyright By lizhuming, All Rights Reserved
 *
 **********************************************************
 *  @LOG Change Log: -----
 **********************************************************
 */

#include "qurl_port.h"
#include "qurl_log.h"
#include "qurl_http.h"
#include "qurl_ftp.h"

#include "qurl_smtp.h"

/*Protocol Stack*/
static const qurl_port_prot_t *const g_port_protocols[] = {
#if QURL_CFG_ENABLE_HTTP
    /*HTTP protocol port*/
    &g_qurl_port_prot_http,

#if QURL_CFG_ENABLE_TLS
    &g_qurl_port_prot_https,
#endif
#endif

#if QURL_CFG_ENABLE_FTP
    /*FTP protocol port*/
    &g_qurl_port_prot_ftp,
#if QURL_CFG_ENABLE_TLS
    /*FTP protocol port*/
    &g_qurl_port_prot_ftps,
#endif
#endif
#if QURL_CFG_ENABLE_SMTP
    /*SMTP protocol port*/
    &g_qurl_port_prot_smtp,
#if QURL_CFG_ENABLE_TLS
    &g_qurl_port_prot_smtps,
#endif
#endif
    /* safe */
    (qurl_port_prot_t *)QOSA_NULL,

};

qurl_ecode_t qurl_port_prot_match_by_scheme(const char *scheme, const qurl_port_prot_t **protocol_pptr)
{
    qurl_ecode_t                   ret = QURL_ECODE_PROT_NO_MATCH;
    const qurl_port_prot_t *const *pptr = QOSA_NULL;
    const qurl_port_prot_t        *ptr = QOSA_NULL;

    for (pptr = g_port_protocols; (ptr = *pptr) != QOSA_NULL; pptr++)
    {
        if (!qurl_strcasecmp(ptr->scheme, scheme))
        {
            if (protocol_pptr != QOSA_NULL)
            {
                *protocol_pptr = ptr;
            }
            ret = QURL_OK;
            break;
        }
    }

    return ret;
}

qurl_timeval_t qurl_time_now(void)
{
    qurl_timeval_t time_info = {0};

    time_info = qurl_platform_time_now();

    return time_info;
}

/**
 * @brief Time difference between current data and old data
 *
 * @param old_timeval
 * @param new_timeval
 * @return qurl_time_t
 */
qurl_time_t qurl_time_diff_ms(qurl_timeval_t old_timeval, qurl_timeval_t new_timeval)
{
    qurl_time_t diff_s = 0;

    diff_s = new_timeval.s - old_timeval.s;

    if (diff_s >= (QURL_CFG_TIMEDIFF_MAX / 1000))
    {
        return QURL_CFG_TIMEDIFF_MAX;
    }
    else if (diff_s <= (QURL_CFG_TIMEDIFF_MIN / 1000))
    {
        return QURL_CFG_TIMEDIFF_MIN;
    }

    return diff_s * 1000 + (new_timeval.ms - old_timeval.ms);
}

/*[lzm][todo]If there is no asynchronous DNS, encapsulate synchronous DNS as asynchronous DNS*/
#if defined(QURL_CFG_PORT_SOCKET_RESOLVE_FUNC) && !defined(QURL_CFG_PORT_SOCKET_RESOLVE_ASYNC_FUNC)
struct qurl_port_dns_thread_data_s
{
    const char     *hostname;
    qurl_resolve_cb found_cb;
    void           *cb_arg;
    int             dns_addrtype;
    int             nw_id;
};
typedef struct qurl_port_dns_thread_data_s qurl_port_dns_thread_data_t;

void qurl_dns_thread_cb(void *arg)
{
    qurl_port_dns_thread_data_t *data_ptr = (qurl_port_dns_thread_data_t *)arg;

    struct addrinfo hints;
    qurl_memset(&hints, 0, sizeof(hints));
    hints.ai_family = data_ptr->dns_addrtype;
    hints.ai_socktype = SOCK_STREAM;
#if !defined(CONFIG_QOSA_ASR160X_PLATFORM_FUNC) && !defined(CONFIG_QOSA_ASR1803_PLATFORM_FUNC)
    //Only for the ASR1903 platform and lwip1.4.1 lwip such as 1606. Once ASR1903 is upgraded to lwip2.0 later, it can be changed back to support.
    hints.ai_flags = AI_ADDRCONFIG;
#endif /* !defined(CONFIG_QOSA_ASR160X_PLATFORM_FUNC) && !defined(CONFIG_QOSA_ASR1803_PLATFORM_FUNC) */

    struct addrinfo *ai = QOSA_NULL;
    // printf("resoulve:[%s]\r\n", data_ptr->hostname);
    int ret = qurl_socket_resolve(data_ptr->hostname, QOSA_NULL, &hints, &ai, data_ptr->nw_id);
    if (ret != 0)
    {
        qurl_loge("getaddrinfo: %d\n", ret);
    }

#if 0
    qurl_addrinfo_t *head = QOSA_NULL; //Linked list head pointer
    qurl_addrinfo_t *cur = QOSA_NULL; //Current node pointer

    /*Convert DNS results to QUrl format*/
    struct addrinfo* addr = ai;
    while (addr != QOSA_NULL) {
        //Create a new node
        qurl_addrinfo_t* node = (qurl_addrinfo_t*)qurl_malloc(sizeof(qurl_addrinfo_t));
        if (node == QOSA_NULL) {
            perror("malloc");
            break;
        }

        //Fill node data
        node->ai_family = addr->ai_family;
        if (addr->ai_family == AF_INET) {
            struct sockaddr_in* addr_in = (struct sockaddr_in*)addr->ai_addr;
            inet_ntop(AF_INET, &(addr_in->sin_addr), node->ip_addr, QURL_INET6_ADDRSTRLEN);
        } else if (addr->ai_family == AF_INET6) {
            struct sockaddr_in6* addr_in6 = (struct sockaddr_in6*)addr->ai_addr;
            inet_ntop(AF_INET6, &(addr_in6->sin6_addr), node->ip_addr, QURL_INET6_ADDRSTRLEN);
        }
        node->next = QOSA_NULL;

        //Insert a node into the linked list
        if (head == QOSA_NULL) {
            head = node;
            cur = node;
        } else {
            cur->next = node;
            cur = node;
        }

        addr = addr->ai_next;
    }

    data_ptr->found_cb(head, data_ptr->cb_arg);

    //Release linked list memory
    struct qurl_addrinfo_s* temp;
    while (head != QOSA_NULL) {
        temp = head;
        head = head->next;
        qurl_free(temp);
    }
#else
    data_ptr->found_cb(ai, data_ptr->cb_arg);
#endif

    //freeaddrinfo(ai); //found_cb release ai memory
    qurl_free(data_ptr);

    qurl_thread_delete(QOSA_NULL);
}

qurl_ecode_t qurl_socket_resolve_async(const char *hostname, qurl_resolve_cb found, void *callback_arg, unsigned char dns_addrtype, int nw_id)
{
    qurl_thread_t                thread_ptr = QOSA_NULL;
    qurl_port_dns_thread_data_t *data_ptr = {0};

    data_ptr = (qurl_port_dns_thread_data_t *)qurl_malloc(sizeof(qurl_port_dns_thread_data_t));
    data_ptr->hostname = hostname;
    data_ptr->found_cb = found;
    data_ptr->cb_arg = callback_arg;
    data_ptr->dns_addrtype = dns_addrtype;
    data_ptr->nw_id = nw_id;

    qurl_thread_create(&thread_ptr, qurl_dns_thread_cb, QURL_DNS_RESOLVE_TASK_PRIORITY, QURL_DNS_TASK_STACK_SIZE, (void *)data_ptr);

    return QURL_OK;
}

#endif
