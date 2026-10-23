/** @file         qurl_def_basic.h
 *  @brief        Brief description: Common basic definitions used internally by qurl
 *  @details      Detailed Description
 *  @author       lzm
 *  @date         2023-10-19 13:39:36
 *  @version      v1.0
 *  @copyright    Copyright By lizhuming, All Rights Reserved
 *
 **********************************************************
 *  @LOG Change Log: -----
 **********************************************************
 */

#ifndef QURL_DEF_BASIC_H
#define QURL_DEF_BASIC_H

#include "qurl_config.h"

#define QURL_WAIT_FOREVER     (-1)
#define QURL_SOCKET_BAD       (-1)
/** Maximum length of IPV4\IPV6 addresses*/
#define QURL_INET4_ADDRSTRLEN (sizeof("255:255:255:255"))
#define QURL_INET6_ADDRSTRLEN (sizeof("ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"))

/*Functional Functions*/
#define QURL_MAX(a, b)        (((a) > (b)) ? (a) : (b))
#define QURL_MIN(a, b)        (((a) < (b)) ? (a) : (b))
#define QURL_UNUSED(a)        (void)(a)

/**
 * @brief file handle type
 */
typedef int qurl_file_t;
/**
 * @brief socket handle type
 */
typedef int qurl_socket_t;
/**
 * @brief Protocol Type
 */
typedef unsigned int qurl_prot_t;
/**
 * @brief Port-related: thread
 */
typedef void *qurl_thread_t;
/**
 * @brief Port-related: thread function
 */
typedef void (*qurl_thread_cb)(void *);
/**
 * @brief Port-related: lock
 */
typedef void *qurl_mutex_t;
/**
 * @brief Port-related: queue
 */
typedef void *qurl_queue_t;
/**
 * @brief Port-related: network activate
 */
typedef void (*qurl_nw_activate_cb)(int, void *);
/**
 * @brief Port-related: socket DNS
 */
typedef void (*qurl_resolve_cb)(void *, void *);
/**
 * @brief Port-related: IPC events
 */
struct qurl_event_s
{
    unsigned long id;
    unsigned long param1;
    unsigned long param2;
    unsigned long param3;
};
typedef struct qurl_event_s qurl_event_t;
/**
 * @struct qurl_timeval_t
 * @brief Time data
 */
struct qurl_timeval_s
{
    unsigned long long s;  /*second*/
    unsigned long long ms; /*milliseconds*/
};
typedef struct qurl_timeval_s qurl_timeval_t;
typedef long long             qurl_time_t;
#define QURL_TIME_T_MAX LONG_MAX
#define QURL_TIME_T_MIN LONG_MIN

#if 0
/**
 * @struct  qurl_addrinfo_t
 * @brief 
 */
struct qurl_addrinfo_s
{
    int ai_family; /*Address Family*/
    char ip_addr[QURL_INET6_ADDRSTRLEN]; /*Address in string form*/
    struct qurl_addrinfo_s *next; /*Next address*/
};
typedef struct qurl_addrinfo_s qurl_addrinfo_t;
#endif
typedef struct addrinfo         qurl_addrinfo_t;
typedef struct sockaddr         qurl_sockaddr_t;
typedef struct sockaddr_in      qurl_sockaddr_in_t;
typedef struct sockaddr_in6     qurl_sockaddr_in6_t;
typedef struct sockaddr_storage qurl_sockaddr_storage_t;

/**
 * @struct  qurl_sock_addr_t
 * @brief socket data structure
 */
struct qurl_sockaddr_ex_s
{
    int          family;   /*!< Address family (e.g., AF_INET, AF_INET6)*/
    int          socktype; /*!< Socket type (e.g., SOCK_STREAM, SOCK_DGRAM)*/
    int          protocol; /*!< Protocol type (e.g., IPPROTO_TCP, IPPROTO_UDP)*/
    unsigned int addrlen;  /*!< indicates the address length (in bytes)*/
    union
    {
        struct sockaddr         sa;
        struct sockaddr_in      sai;
#if LWIP_IPV6
        struct sockaddr_in6     sai6;
#endif
        struct sockaddr_storage sas;
    } addr_u;
};
typedef struct qurl_sockaddr_ex_s qurl_sockaddr_ex_t;

#endif /* Head define end*/
