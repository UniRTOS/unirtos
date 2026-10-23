/** @file         qurl_platform_port.h
 *  @brief        Brief Description
 *  @details      Detailed Description
 *  @author       lzm
 *  @date         2023-11-16 15:57:16
 *  @version      v1.0
 *  @copyright    Copyright By lizhuming, All Rights Reserved
 *
 **********************************************************
 *  @LOG Change Log: -----
 **********************************************************
*/

#ifndef QURL_PLATFORM_PORT_H
#define QURL_PLATFORM_PORT_H

#include "qurl_config.h"
#include "qurl_code.h"
#include "qurl_def.h"
#include "qurl_def_basic.h"

/** OS */
/** create */
qurl_ecode_t qurl_platform_thread_create(qurl_thread_t *thread_ptr, qurl_thread_cb func, unsigned char priority, unsigned int stack_size, void *arg);
/** delete */
qurl_ecode_t qurl_platform_thread_delete(qurl_thread_t thread_ptr);
/** sleep */
qurl_ecode_t qurl_platform_sleep_ms(unsigned int ms);
/** mutex */
qurl_ecode_t qurl_platform_mutex_create(qurl_mutex_t *lock_ptr);
//Delete lock
qurl_ecode_t qurl_platform_mutex_delete(qurl_mutex_t lock);
//Lock
qurl_ecode_t qurl_platform_mutex_lock(qurl_mutex_t lock, int timeout_ms);
//Release lock
qurl_ecode_t qurl_platform_mutex_unlock(qurl_mutex_t lock);

qurl_ecode_t qurl_platform_event_create(qurl_queue_t *queue_ptr, int cnt);

qurl_ecode_t qurl_platform_event_delete(qurl_queue_t queue);

qurl_ecode_t qurl_platform_event_wait(qurl_queue_t queue, qurl_event_t *recv_ptr, int timeout_ms);

qurl_ecode_t qurl_platform_event_send(qurl_queue_t queue, const qurl_event_t *sent_ptr, int timeout_ms);

//Get current thread ID
unsigned long qurl_platform_get_pthread_id(void);

qurl_timeval_t qurl_platform_time_now(void);
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
int qurl_platform_nw_activate(int nw_id);

int qurl_platform_nw_is_activated(int nw_id);

int qurl_platform_nw_get_ip(int nw_id, char *ipv4_buf, int ipv4_buf_len, char *ipv6_buf, int ipv6_buf_len);
#endif
qurl_ecode_t qurl_platform_socket_nonblock(qurl_socket_t sockfd, int nonblock);

int qurl_platform_socket_get_errno(qurl_socket_t sockfd);

long qurl_platform_fgetsize(qurl_file_t fd);

int qurl_platform_socket_dns_get(const char *nodename, const char *servname, struct addrinfo *hints, struct addrinfo **ai, int nw_id);

int qurl_platform_get_last_close_event(int s);

#endif /* Head define end*/
