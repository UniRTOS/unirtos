/** @file         qurl_platform_port.c
 *  @brief        Brief description: Used to adapt the platform-side interfaces required by qurl.
 *  @details      Detailed Description
 *  @author       lzm
 *  @date         2023-11-16 15:56:48
 *  @version      v1.0
 *  @copyright    Copyright By lizhuming, All Rights Reserved
 *
 **********************************************************
 *  @LOG Change Log: -----
 **********************************************************
 */

#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_log.h"
#include "qosa_rtc.h"

#if defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) || defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)
#include <fcntl.h>
#endif
#include "qosa_sockets.h"
#include "qosa_virtual_file.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
#include "qosa_datacall.h"
#endif
#include "qosa_ip_addr.h"
#include "qurl_platform_port.h"
#ifndef CONFIG_QOSA_LINUX_PLATFORM_FUNC
#include "qosa_errno.h"
#endif

#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   

#define QOS_LOG_TAG                         LOG_TAG_MODEM
#else
#define QOS_LOG_TAG                         LOG_TAG_QURL_API
#endif
// OS
qurl_ecode_t qurl_platform_thread_create(qurl_thread_t *thread_ptr, qurl_thread_cb func, unsigned char priority, unsigned int stack_size, void *arg)
{
    qurl_ecode_t ret = 0;

    ret = qosa_task_create(thread_ptr, stack_size, priority, "qurl", func, arg);
    if (ret != QOSA_OK)
    {
        return QURL_ECODE_THREAD_CREATE_FAILED;
    }

    return QURL_OK;
}
qurl_ecode_t qurl_platform_thread_delete(qurl_thread_t thread_ptr)
{
    qosa_task_delete(thread_ptr);

    return QURL_OK;
}

//Sleep ms
qurl_ecode_t qurl_platform_sleep_ms(unsigned int ms)
{
    qosa_task_sleep_ms(ms);
    return QURL_OK;
}

//Lock Creation
qurl_ecode_t qurl_platform_mutex_create(qurl_mutex_t *lock_ptr)
{
    qurl_ecode_t ret = 0;

    ret = qosa_mutex_create(lock_ptr);

    return ret == QOSA_OK ? QURL_OK : QURL_ECODE_LOCK_CREATE_FAILD;
}

//Delete lock
qurl_ecode_t qurl_platform_mutex_delete(qurl_mutex_t lock)
{
    qurl_ecode_t ret = 0;

    ret = qosa_mutex_delete(lock);

    return ret == QOSA_OK ? QURL_OK : QURL_ECODE_LOCK_DELETE_FAILD;
}

//Lock
qurl_ecode_t qurl_platform_mutex_lock(qurl_mutex_t lock, int timeout_ms)
{
    qurl_ecode_t ret = 0;

    ret = qosa_mutex_lock(lock, timeout_ms);

    switch (ret)
    {
        case QOSA_OK: {
            ret = QURL_OK;
        }
        break;
        case QOSA_ERROR_MUTEX_EBUSY_ERR: {
            ret = QURL_ECODE_LOCK_TIMEOUT;
        }
        break;
        default:
            ret = QURL_ECODE_LOCK_LOCK_FAILD;
            break;
    }

    return ret;
}

//Release lock
qurl_ecode_t qurl_platform_mutex_unlock(qurl_mutex_t lock)
{
    qurl_ecode_t ret = 0;

    if (lock == QOSA_NULL)
    {
        return QURL_ECODE_PARAM_INVALID;
    }
    ret = qosa_mutex_unlock(lock);

    return ret == QOSA_OK ? QURL_OK : QURL_ECODE_LOCK_UNLOCK_FAILD;
}

qurl_ecode_t qurl_platform_event_create(qurl_queue_t *queue_ptr, int cnt)
{
    qurl_ecode_t ret = 0;

    ret = qosa_msgq_create(queue_ptr, sizeof(qurl_event_t), cnt);

    return ret == QOSA_OK ? QURL_OK : QURL_ECODE_EVENT_FAILD;
}

qurl_ecode_t qurl_platform_event_delete(qurl_queue_t queue)
{
    qurl_ecode_t ret = 0;

    ret = qosa_msgq_delete(queue);

    return ret == QOSA_OK ? QURL_OK : QURL_ECODE_EVENT_FAILD;
}

qurl_ecode_t qurl_platform_event_wait(qurl_queue_t queue, qurl_event_t *recv_ptr, int timeout_ms)
{
    qurl_ecode_t ret = 0;

    ret = qosa_msgq_wait(queue, (qosa_uint8_t *)recv_ptr, sizeof(qurl_event_t), timeout_ms);
    QLOGV("ret:%d", ret);

    switch (ret)
    {
        case QOSA_OK: {
            ret = QURL_OK;
        }
        break;
        case QOSA_ERROR_MSGQ_TIMEOUT_ERR: {
            ret = QURL_ECODE_EVENT_TIMEOUT;
        }
        break;
        default:
            ret = QURL_ECODE_EVENT_FAILD;
            break;
    }

    return ret;
}

qurl_ecode_t qurl_platform_event_send(qurl_queue_t queue, const qurl_event_t *sent_ptr, int timeout_ms)
{
    qurl_ecode_t ret = 0;

    ret = qosa_msgq_release(queue, sizeof(qurl_event_t), (qosa_uint8_t *)sent_ptr, timeout_ms);

    switch (ret)
    {
        case QOSA_OK: {
            ret = QURL_OK;
        }
        break;
        case QOSA_ERROR_MSGQ_TIMEOUT_ERR: {
            ret = QURL_ECODE_EVENT_TIMEOUT;
        }
        break;
        default:
            ret = QURL_ECODE_EVENT_FAILD;
            break;
    }

    return ret;
}

//Get Thread ID
unsigned long qurl_platform_get_pthread_id(void)
{
    qosa_task_t id = QOSA_NULL;

    qosa_task_get_current_ref(&id);
    return (unsigned long)(id);
}

//Get current time
qurl_timeval_t qurl_platform_time_now(void)
{
    qosa_time_info_t time_info = {0};
    qurl_timeval_t   timeval = {0};

    qosa_get_system_time(&time_info);
    timeval.s = time_info.seconds;
    timeval.ms = time_info.microseconds / 1000;

    return timeval;
}
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   

// NETWORK
int qurl_platform_nw_activate(int nw_id)
{
    qosa_datacall_errno_e ret = QOSA_DATACALL_OK;
    qosa_datacall_conn_t  conn = 0;

    conn = qosa_datacall_conn_new(0, nw_id, QOSA_DATACALL_CONN_TCPIP);
    ret = qosa_datacall_start(conn, 60);
    if (ret != QOSA_DATACALL_OK)
    {
        return QURL_ECODE_NETWORK_ERR;
    }

    return QURL_OK;
}

int qurl_platform_nw_is_activated(int nw_id)
{
    qosa_datacall_errno_e ret = QOSA_DATACALL_OK;
    qosa_datacall_conn_t  conn = 0;

    conn = qosa_datacall_conn_new(0, nw_id, QOSA_DATACALL_CONN_TCPIP);
    ret = qosa_datacall_get_status(conn);
    if (ret != QOSA_TRUE)
    {
        return 0;
    }

    return 1;
}

int qurl_platform_nw_get_ip(int nw_id, char *ipv4_buf, int ipv4_buf_len, char *ipv6_buf, int ipv6_buf_len)
{
    qosa_datacall_errno_e   ret = QOSA_DATACALL_OK;
    qosa_datacall_ip_info_t info = {0};
    qosa_datacall_conn_t    conn = 0;

    conn = qosa_datacall_conn_new(0, nw_id, QOSA_DATACALL_CONN_TCPIP);
    ret = qosa_datacall_get_ip_info(conn, &info);
    if (ret == QOSA_DATACALL_OK)
    {
        if (ipv4_buf != QOSA_NULL && (info.ip_type == QOSA_PDP_IPV4 || info.ip_type == QOSA_PDP_IPV4V6))
        {
            qosa_memset(ipv4_buf, 0x00, ipv4_buf_len);
            if (inet_ntop(AF_INET, &info.ipv4_ip.addr.ipv4_addr, ipv4_buf, ipv4_buf_len) == QOSA_NULL)
            {
                return QURL_ECODE_NETWORK_ERR;
            }
        }
        if (ipv6_buf != QOSA_NULL && (info.ip_type == QOSA_PDP_IPV6 || info.ip_type == QOSA_PDP_IPV4V6))
        {
            qosa_memset(ipv6_buf, 0x00, ipv6_buf_len);
            if (inet_ntop(AF_INET6, &info.ipv6_ip.addr.ipv6_addr, ipv6_buf, ipv6_buf_len) == QOSA_NULL)
            {
                return QURL_ECODE_NETWORK_ERR;
            }
        }
        return QURL_OK;
    }

    return QURL_ECODE_NETWORK_ERR;
}
#endif
/** Platform-specific interface */
#ifdef CONFIG_QOSA_QT2131_PLATFORM_FUNC
qurl_ecode_t qurl_platform_socket_nonblock(qurl_socket_t sockfd, int nonblock)
{
    return uapi_lwip_fcntl(sockfd, F_SETFL, nonblock ? O_NONBLOCK : O_BLOCK) < 0 ? QURL_ECODE_SOCK_CTRL_FAILED : QURL_OK;
}
#else
// SOCKET

qurl_ecode_t qurl_platform_socket_nonblock(qurl_socket_t sockfd, int nonblock)
{
    int flags = fcntl(sockfd, F_GETFL, 0);

    QLOGV("O_NONBLOCK=%x\n",O_NONBLOCK);
    if (flags < 0) {
        QLOGV("sockfd=%d, F_GETFL failed, errno=%d\n", sockfd, errno);
        return QURL_ECODE_SOCK_CTRL_FAILED;
    }
    if (nonblock) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }

    int ret = fcntl(sockfd, F_SETFL, flags);
    if (ret < 0) {
        QLOGV("sockfd=%d, F_SETFL failed, errno=%d\n", sockfd, errno);
        return QURL_ECODE_SOCK_CTRL_FAILED;
    }
    return QURL_OK;
    // int flags = 0;
    // return fcntl(sockfd, F_SETFL, nonblock ? flags | O_NONBLOCK : flags & ~O_NONBLOCK) < 0 ? QURL_ECODE_SOCK_CTRL_FAILED : QURL_OK;
}
#endif

int qurl_platform_socket_get_errno(qurl_socket_t sockfd)
{
    (void)sockfd;

#ifdef CONFIG_QOSA_LINUX_PLATFORM_FUNC
    return errno;
#else
    return lwip_get_error(sockfd);
#endif
}

// FS
long qurl_platform_fgetsize(qurl_file_t fd)
{
    struct qosa_vfs_stat_t st = {0};

    if (qosa_vfs_fstat(fd, &st) == 0)
    {
        return st.st_size;
    }
    else
    {
        return 0;
    }
}

int qurl_platform_socket_dns_get(const char *nodename, const char *servname, struct addrinfo *hints, struct addrinfo **ai, int nw_id)
{
    QOSA_UNUSED(nw_id);
/** The underlying DNS retrieval interfaces are different */
#if defined(CONFIG_QOSA_ASR1903_PLATFORM_FUNC) || defined(CONFIG_QOSA_RDA8910_R06_PLATFORM_FUNC) || defined(CONFIG_QOSA_ASR160X_PLATFORM_FUNC) || defined(CONFIG_QOSA_ASR1805_PLATFORM_FUNC)
    return getaddrinfo_with_pcid(nodename, servname, hints, ai, nw_id);
#elif defined(CONFIG_QOSA_QCX216_PLATFORM_FUNC) || defined(CONFIG_QOSA_QCX217_PLATFORM_FUNC) || defined(CONFIG_QOSA_EIGEN718_PLATFORM_FUNC)
    return getaddrinfowithcid(nodename, servname, hints, ai, nw_id);
#elif defined(CONFIG_QOSA_XY4100_PLATFORM_FUNC)
    return getaddrinfo_ext(nw_id, nodename, QOSA_NULL, QOSA_NULL, servname, hints, ai);
#else
    return getaddrinfo(nodename, servname, hints, ai);
#endif
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
int qurl_platform_get_last_close_event(int s)
{
    QOSA_UNUSED(s);
#ifdef CONFIG_QOSA_LINUX_PLATFORM_FUNC
    //Linux directly returns 0
    return 0;
#elif defined(CONFIG_QOSA_EIGEN718_PLATFORM_FUNC) || defined(CONFIG_QOSA_ASR1903_PLATFORM_FUNC)
    return lwip_get_last_close_event(s);
#else  /* CONFIG_QOSA_LINUX_PLATFORM_FUNC */
    return 0;
#endif /* CONFIG_QOSA_LINUX_PLATFORM_FUNC */
}
