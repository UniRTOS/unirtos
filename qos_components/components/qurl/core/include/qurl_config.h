/** @file         qurl_config.h
 *  @brief        Brief Description
 *  @details      Detailed Description
 *  @author       lzm
 *  @date         2023-09-01 15:26:03
 *  @version      v1.0
 *  @copyright    Copyright By lizhuming, All Rights Reserved
 *
 **********************************************************
 *  @LOG Change Log: -----
 **********************************************************
 */

#ifndef QURL_CONFIG_H
#define QURL_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
//#ifndef CONFIG_QOSA_ASR1903_PLATFORM_FUNC
//#include <strings.h>
//#endif //CONFIG_QOSA_ASR1903_PLATFORM_FUNC
#include <ctype.h>
#include <limits.h>

#include "qurl_code.h"
#include "qurl_def.h"

#include "qurl_platform_config.h"

/*Feature Trimming*/
#ifndef QURL_CFG_ENABLE_PARSEDATE
#define QURL_CFG_ENABLE_PARSEDATE (1)
#endif /* QURL_CFG_ENABLE_PARSEDATE */
#ifndef QURL_CFG_ENABLE_IPV6
#define QURL_CFG_ENABLE_IPV6 (0)
#endif /* QURL_CFG_ENABLE_IPV6 */
#ifndef QURL_CFG_ENABLE_FS
#define QURL_CFG_ENABLE_FS (0)
#endif /* QURL_CFG_ENABLE_FS */
#ifndef QURL_CFG_ENABLE_TLS_MBEDTLS
#define QURL_CFG_ENABLE_TLS_MBEDTLS (0)
#endif /* QURL_CFG_ENABLE_TLS_MBEDTLS */
#ifndef QURL_CFG_ENABLE_TLS_OPENSSL
#define QURL_CFG_ENABLE_TLS_OPENSSL (0)
#endif /* QURL_CFG_ENABLE_TLS_MBEDTLS */
#ifndef QURL_CFG_ENABLE_TLS_PLATFORM
#define QURL_CFG_ENABLE_TLS_PLATFORM (0)
#endif /* QURL_CFG_ENABLE_TLS_PLATFORM */
#define QURL_CFG_ENABLE_TLS (QURL_CFG_ENABLE_TLS_MBEDTLS | QURL_CFG_ENABLE_TLS_OPENSSL | QURL_CFG_ENABLE_TLS_PLATFORM)
#ifndef QURL_CFG_ENABLE_HTTP
#define QURL_CFG_ENABLE_HTTP (0)
#endif /* QURL_CFG_ENABLE_HTTP */
#ifndef QURL_CFG_ENABLE_HTTP2
#define QURL_CFG_ENABLE_HTTP2 (0)
#endif /* QURL_CFG_ENABLE_HTTP2 */
#ifndef QURL_CFG_ENABLE_COOKIES
#define QURL_CFG_ENABLE_COOKIES (0)
#endif /* QURL_CFG_ENABLE_COOKIES */
#ifndef QURL_CFG_ENABLE_QUIC
#define QURL_CFG_ENABLE_QUIC (0)
#endif /* QURL_CFG_ENABLE_QUIC */
#ifndef QURL_CFG_ENABLE_FTP
#define QURL_CFG_ENABLE_FTP (1)
#endif /* QURL_CFG_ENABLE_FTP */
#ifndef QURL_CFG_ENABLE_SMTP
#define QURL_CFG_ENABLE_SMTP (1)
#endif /* QURL_CFG_ENABLE_FTP */
#define QURL_CFG_ENABLE_PINGPONG (QURL_CFG_ENABLE_FTP | QURL_CFG_ENABLE_SMTP)

/*Configuration*/
/** Maximum number of concurrent services*/
#ifndef QURL_CFG_CORE_SERVER_MAX
#define QURL_CFG_CORE_SERVER_MAX (15)
#endif /* QURL_CFG_CORE_SERVER_MAX */
/** Connection data processing cache*/
#ifndef QURL_CONN_DEAL_CACHE_SIZE
#ifdef CONFIG_OSA_CONN_DEAL_CACHE_SIZE
#define QURL_CONN_DEAL_CACHE_SIZE CONFIG_OSA_CONN_DEAL_CACHE_SIZE
#else /* CONFIG_OSA_CONN_DEAL_CACHE_SIZE */
#define QURL_CONN_DEAL_CACHE_SIZE (2*1024)
#endif /* CONFIG_OSA_CONN_DEAL_CACHE_SIZE */
#endif /* QURL_CONN_DEAL_CACHE_SIZE */
/** Default TLS handshake timeout, unit: S*/
#ifndef QURL_CFG_TLS_HANDSHAKE_TIMEOUT_DEFAULT
#define QURL_CFG_TLS_HANDSHAKE_TIMEOUT_DEFAULT (30)
#endif /* QURL_CFG_TLS_HANDSHAKE_TIMEOUT_DEFAULT */
/** TLS session cache max cnt */
#ifndef QURL_CFG_TLS_SESSION_CACHE_MAX_CNT_DEFAULT
#define QURL_CFG_TLS_SESSION_CACHE_MAX_CNT_DEFAULT (8)
#endif /* QURL_CFG_TLS_SESSION_CACHE_MAX_CNT_DEFAULT */
/** Default command response time in PINGPONG, unit: MS*/
#ifndef QURL_CFG_PONG_TIMEOUT_DEFAULT
#define QURL_CFG_PONG_TIMEOUT_DEFAULT (120 * 1000)
#endif /* QURL_CFG_PONG_TIMEOUT_DEFAULT */
/** Default maximum number of HTTP multipart forms*/
#ifndef QURL_CFG_MULTIFORM_MAX_ID_DEFAULT
#define QURL_CFG_MULTIFORM_MAX_ID_DEFAULT (10)
#endif /* QURL_CFG_MULTIFORM_MAX_ID_DEFAULT */
/** Default value for HTTP multipart form BOUNDARY*/
#ifndef QURL_CFG_MULTIFORM_BOUNDARY_DEFAULT
#define QURL_CFG_MULTIFORM_BOUNDARY_DEFAULT "unirtos-qurl-lzm----------zzz"
#endif /* QURL_CFG_MULTIFORM_BOUNDARY_DEFAULT */

/** Error code base value*/
// #ifndef QURL_CFG_ECODE_BASIC_NUM
// #define QURL_CFG_ECODE_BASIC_NUM (0x80010000)
// #endif /* QURL_CFG_ECODE_BASIC_NUM */

/** Upper limit of difference (for overflow handling)*/
#define QURL_CFG_TIMEDIFF_MAX (0x7FFFFFFF)
#define QURL_CFG_TIMEDIFF_MIN (-QURL_CFG_TIMEDIFF_MAX - (1))

/** Default operation timeout value*/
#ifndef QURL_CFG_DEFAULT_PERFORM_TIMEOUT_MS
#define QURL_CFG_DEFAULT_PERFORM_TIMEOUT_MS (30 * 1000)
#endif /* QURL_CFG_DEFAULT_PERFORM_TIMEOUT_MS */
#ifndef QURL_CFG_DEFAULT_PERFORM_IDLE_TIMEOUT_MS
#define QURL_CFG_DEFAULT_PERFORM_IDLE_TIMEOUT_MS (0)
#endif /* QURL_CFG_DEFAULT_PERFORM_IDLE_TIMEOUT_MS */
#ifndef QURL_CFG_DEFAULT_PERFORM_CONN_IDLE_TIMEOUT_MS
#define QURL_CFG_DEFAULT_PERFORM_CONN_IDLE_TIMEOUT_MS (120 * 1000)
#endif /* QURL_CFG_DEFAULT_PERFORM_CONN_IDLE_TIMEOUT_MS */
#ifndef QURL_CFG_DEFAULT_PERFORM_CONN_MAXLIFETIME_MS
#define QURL_CFG_DEFAULT_PERFORM_CONN_MAXLIFETIME_MS (0)
#endif /* QURL_CFG_DEFAULT_PERFORM_CONN_MAXLIFETIME_MS */
#ifndef QURL_CFG_DEFAULT_ROUSE_CHECK_TIME_MS
#define QURL_CFG_DEFAULT_ROUSE_CHECK_TIME_MS (1000)
#endif /* QURL_CFG_DEFAULT_ROUSE_CHECK_TIME_MS */

/** URL */
#ifndef QURL_CFG_URL_LEN_MAX
#define QURL_CFG_URL_LEN_MAX (2048)
#endif /* QURL_CFG_URL_LEN_MAX */
/** SCHEME */
#ifndef QURL_CFG_URL_SCHEME_LEN_MAX
#define QURL_CFG_URL_SCHEME_LEN_MAX (20)
#endif /* QURL_CFG_URL_SCHEME_LEN_MAX */
/** Default SCHEME*/
#ifndef QURL_CFG_URL_SCHEME_DEFAULT
#define QURL_CFG_URL_SCHEME_DEFAULT "http"
#endif /* QURL_CFG_URL_SCHEME_DEFAULT */

/** Default port number (not recommended to modify)*/
#ifndef QURL_CFG_PORT_HTTP_DEFAULT
#define QURL_CFG_PORT_HTTP_DEFAULT 80
#endif /* QURL_CFG_PORT_HTTP_DEFAULT */
#ifndef QURL_CFG_PORT_HTTPS_DEFAULT
#define QURL_CFG_PORT_HTTPS_DEFAULT 443
#endif /* QURL_CFG_PORT_HTTP_DEFAULT */
#ifndef QURL_CFG_PORT_FTP_DEFAULT
#define QURL_CFG_PORT_FTP_DEFAULT 21
#endif /* QURL_CFG_PORT_FTP_DEFAULT */
#ifndef QURL_CFG_PORT_FTPS_DEFAULT
#define QURL_CFG_PORT_FTPS_DEFAULT 2990
#endif /* QURL_CFG_PORT_FTP_DEFAULT */
#ifndef QURL_CFG_PORT_SMTP_DEFAULT
#define QURL_CFG_PORT_SMTP_DEFAULT 25
#endif /* QURL_CFG_PORT_SMTP_DEFAULT */
#ifndef QURL_CFG_PORT_SMTPS_DEFAULT
#define QURL_CFG_PORT_SMTPS_DEFAULT 465
#endif /* QURL_CFG_PORT_SMTP_DEFAULT */

/* LOG */
/** Enable QURL LOG*/
#ifndef QURL_CFG_LOG
#define QURL_CFG_LOG (0)
#endif
/** QURL_CFG_PORT_LOG_FUNC: Log callback function*/
#ifndef QURL_CFG_PORT_LOG_FUNC
/*[lzm][note]Not verified, may require modification*/
#define QURL_CFG_PORT_LOG_FUNC (void)
#endif
#ifndef QURL_CFG_LOG_ERROR
#define QURL_CFG_LOG_ERROR (0)
#endif
#ifndef QURL_CFG_LOG_DEBUG
#define QURL_CFG_LOG_DEBUG (0)
#endif
#ifndef QURL_CFG_LOG_SETOPT
#define QURL_CFG_LOG_SETOPT (0)
#endif
#ifndef QURL_CFG_LOG_TLS
#define QURL_CFG_LOG_TLS (0)
#endif
#ifndef QURL_CFG_LOG_HTTP
#define QURL_CFG_LOG_HTTP (0)
#endif
#ifndef QURL_CFG_LOG_FTP
#define QURL_CFG_LOG_FTP (0)
#endif
#ifndef QURL_CFG_LOG_SMTP
#define QURL_CFG_LOG_SMTP (0)
#endif

/* PORT */
/** Assertion*/
#ifdef QURL_CFG_PORT_ASSERT_FUNC
#define QURL_ASSERT(m) QURL_CFG_PORT_ASSERT_FUNC(m)
#else
#define QURL_ASSERT(m)
#endif

/** Standard Library PORT*/
#ifdef QURL_CFG_PORT_MALLOC_FUNC
#define qurl_malloc(s) QURL_CFG_PORT_MALLOC_FUNC(s)
#else
#define qurl_malloc(s) qosa_malloc(s)
#endif
#ifdef QURL_CFG_PORT_FREE_FUNC
#define qurl_free(p) QURL_CFG_PORT_FREE_FUNC(p)
#else
#define qurl_free(p) qosa_free(p)
#endif
#ifdef QURL_CFG_PORT_SNPRINTF_FUNC
#define qurl_snprintf(x, y, ...) QURL_CFG_PORT_SNPRINTF_FUNC(x, y, ##__VA_ARGS__)
#else
#define qurl_snprintf(x, y, ...) snprintf(x, y, ##__VA_ARGS__)
#endif
#ifdef QURL_CFG_PORT_ASPRINTF_FUNC
#define qurl_asprintf(x, y, ...) QURL_CFG_PORT_ASPRINTF_FUNC(x, y, ##__VA_ARGS__)
#else
#define qurl_asprintf(x, y, ...) qurl_default_asprintf(x, y, ##__VA_ARGS__)
#endif
#ifdef QURL_CFG_PORT_STRTOL_FUNC
#define qurl_strtol(x, y, z) QURL_CFG_PORT_STRTOL_FUNC(x, y, z)
#else
#define qurl_strtol(x, y, z) strtol(x, y, z)
#endif
#ifdef QURL_CFG_PORT_STRTOUL_FUNC
#define qurl_strtoul(x, y, z) QURL_CFG_PORT_STRTOUL_FUNC(x, y, z)
#else
#define qurl_strtoul(x, y, z) strtoul(x, y, z)
#endif
#ifdef QURL_CFG_PORT_STRDUP_FUNC
#define qurl_strdup(x) QURL_CFG_PORT_STRDUP_FUNC(x)
#else
#define qurl_strdup(x) qosa_strdup(x)
#endif
#ifdef QURL_CFG_PORT_STRCHR_FUNC
#define qurl_strchr(x, y) QURL_CFG_PORT_STRCHR_FUNC(x, y)
#else
#define qurl_strchr(x, y) strchr(x, y)
#endif
#ifdef QURL_CFG_PORT_STRRCHR_FUNC
#define qurl_strrchr(x, y) QURL_CFG_PORT_STRRCHR_FUNC(x, y)
#else
#define qurl_strrchr(x, y) strrchr(x, y)
#endif
#ifdef QURL_CFG_PORT_STRCPY_FUNC
#define qurl_strcpy(x, y) QURL_CFG_PORT_STRCPY_FUNC(x, y)
#else
#define qurl_strcpy(x, y) strcpy(x, y)
#endif
#ifdef QURL_CFG_PORT_STRNCPY_FUNC
#define qurl_strncpy(x, y, z) QURL_CFG_PORT_STRNCPY_FUNC(x, y, z)
#else
#define qurl_strncpy(x, y, z) strncpy(x, y, z)
#endif
#ifdef QURL_CFG_PORT_STRLEN_FUNC
#define qurl_strlen(x) QURL_CFG_PORT_STRLEN_FUNC(x)
#else
#define qurl_strlen(x) strlen(x)
#endif
#ifdef QURL_CFG_PORT_STRCMP_FUNC
#define qurl_strcmp(x, y) QURL_CFG_PORT_STRCMP_FUNC(x, y)
#else
#define qurl_strcmp(x, y) strcmp(x, y)
#endif
#ifdef QURL_CFG_PORT_STRNCMP_FUNC
#define qurl_strncmp(x, y, z) QURL_CFG_PORT_STRNCMP_FUNC(x, y, z)
#else
#define qurl_strncmp(x, y, z) strncmp(x, y, z)
#endif
#ifdef QURL_CFG_PORT_STRNCASECMP_FUNC
#define qurl_strncasecmp(x, y, z) QURL_CFG_PORT_STRNCASECMP_FUNC(x, y, z)
#else
#define qurl_strncasecmp(x, y, z) strncasecmp(x, y, z)
#endif
#ifdef QURL_CFG_PORT_MEMCPY_FUNC
#define qurl_memcpy(x, y, z) QURL_CFG_PORT_MEMCPY_FUNC(x, y, z)
#else
#define qurl_memcpy(x, y, z) memcpy(x, y, z)
#endif
#ifdef QURL_CFG_PORT_MEMSET_FUNC
#define qurl_memset(x, y, z) QURL_CFG_PORT_MEMSET_FUNC(x, y, z)
#else
#define qurl_memset(x, y, z) memset(x, y, z)
#endif
#ifdef QURL_CFG_PORT_MEMCHR_FUNC
#define qurl_memchr(x, y, z) QURL_CFG_PORT_MEMCHR_FUNC(x, y, z)
#else
#define qurl_memchr(x, y, z) memchr(x, y, z)
#endif
#ifdef QURL_CFG_PORT_TOLOWER_FUNC
#define qurl_tolower(x) QURL_CFG_PORT_TOLOWER_FUNC(x)
#else
#define qurl_tolower(x) tolower(x)
#endif
#ifdef QURL_CFG_PORT_ATOI_FUNC
#define qurl_atoi(x) QURL_CFG_PORT_ATOI_FUNC(x)
#else
#define qurl_atoi(x) atoi(x)
#endif
#ifdef QURL_CFG_PORT_NTOHS_FUNC
#define qurl_ntohs(x) QURL_CFG_PORT_NTOHS_FUNC(x)
#else
#define qurl_ntohs(x) ntohs(x)
#endif
#ifdef QURL_CFG_PORT_HTONS_FUNC
#define qurl_htons(x) QURL_CFG_PORT_HTONS_FUNC(x)
#else
#define qurl_htons(x) htons(x)
#endif
#ifdef QURL_CFG_PORT_VSNPRINTF_FUNC
#define qurl_vsnprintf(a, b, x, y) QURL_CFG_PORT_VSNPRINTF_FUNC(a, b, x, y)
#else
#define qurl_vsnprintf(a, b, x, y) vsnprintf(a, b, x, y)
#endif
#ifdef QURL_CFG_PORT_STRCASECMP_FUNC
#define qurl_strcasecmp(x, y) QURL_CFG_PORT_STRCASECMP_FUNC(x, y)
#else
#define qurl_strcasecmp(x, y) strcasecmp(x, y)
#endif
#ifdef QURL_CFG_PORT_STRSTR_FUNC
#define qurl_strstr(x, y) QURL_CFG_PORT_STRSTR_FUNC(x, y)
#else
#define qurl_strstr(x, y) strstr(x, y)
#endif
#ifdef QURL_CFG_PORT_STRSPN_FUNC
#define qurl_strspn(x, y) QURL_CFG_PORT_STRSPN_FUNC(x, y)
#else
#define qurl_strspn(x, y) strspn(x, y)
#endif
#ifdef QURL_CFG_PORT_STRCSPN_FUNC
#define qurl_strcspn(x, y) QURL_CFG_PORT_STRCSPN_FUNC(x, y)
#else
#define qurl_strcspn(x, y) strcspn(x, y)
#endif
#ifdef QURL_CFG_PORT_SSCANF_FUNC
#define qurl_sscanf(x, y, ...) QURL_CFG_PORT_SSCANF_FUNC(x, y, ##__VA_ARGS__)
#else
#define qurl_sscanf(x, y, ...) sscanf(x, y, ##__VA_ARGS__)
#endif
#ifdef QURL_CFG_PORT_MEMCMP_FUNC
#define qurl_memcmp(x, y, z) QURL_CFG_PORT_MEMCMP_FUNC(x, y, z)
#else
#define qurl_memcmp(x, y, z) memcmp(x, y, z)
#endif
#ifdef QURL_CFG_PORT_SET_ERRNO_FUNC
#define qurl_set_errno(x) QURL_CFG_PORT_SET_ERRNO_FUNC(x)
#else
#define qurl_set_errno(x) (errno = x)
#endif
#ifdef QURL_CFG_PORT_GET_ERRNO_FUNC
#define qurl_get_errno() QURL_CFG_PORT_GET_ERRNO_FUNC()
#else
#define qurl_get_errno() (errno)
#endif
#ifdef QURL_CFG_PORT_STRPBRK_FUNC
#define qurl_strpbrk(x, y) QURL_CFG_PORT_STRPBRK_FUNC(x, y)
#else
#define qurl_strpbrk(x, y) strpbrk(x, y)
#endif
/*** Temporarily not open for modification*/
#define qurl_safe_free(x)                                                                                                                                      \
    do {                                                                                                                                                       \
        if ((x) != NULL)                                                                                                                                       \
        {                                                                                                                                                      \
            qurl_free(x);                                                                                                                                      \
            (x) = NULL;                                                                                                                                        \
        }                                                                                                                                                      \
    } while (0)
#define qurl_vasprintf(x, y, z) qurl_default_vasprintf(x, y, z)

/** Scattered Collection*/
#ifdef QURL_CFG_PORT_STRTOOFFT_FUNC
#define qurl_strtoofft(x, y, z, p) QURL_CFG_PORT_STRTOOFFT_FUNC(x, y, z, p)
#else
#define qurl_strtoofft(x, y, z, p) qurl_default_strtoofft(x, y, z, p)
#endif

/** OS PORT */
#define QURL_DNS_RESOLVE_TASK_PRIORITY QURL_CFG_DNS_TASK_PRIORITY
#define QURL_NETWORK_TASK_PRIORITY     QURL_CFG_NETWORK_TASK_PRIORITY
#define QURL_DNS_TASK_STACK_SIZE       QURL_CFG_DNS_TASK_STACK_SIZE
#define QURL_NETWORK_TASK_STACK_SIZE   QURL_CFG_NETWORK_TASK_STACK_SIZE

/** Thread Creation*/
#ifdef QURL_CFG_PORT_THREAD_CREATE_FUNC
#define qurl_thread_create(a, b, x, y, z) QURL_CFG_PORT_THREAD_CREATE_FUNC(a, b, x, y, z)
#else
#define qurl_thread_create(a, b, x, y, z)
#endif
/** Thread Deletion*/
#ifdef QURL_CFG_PORT_THREAD_DELETE_FUNC
#define qurl_thread_delete(x) QURL_CFG_PORT_THREAD_DELETE_FUNC(x)
#else
#define qurl_thread_delete(x)
#endif
/** Delay*/
#ifdef QURL_CFG_PORT_SLEEP_MS_FUNC
#define qurl_sleep_ms(x) QURL_CFG_PORT_SLEEP_MS_FUNC(x)
#else
#define qurl_sleep_ms(x)
#endif
/** Create lock*/
#ifdef QURL_CFG_PORT_MUTEX_CREATE_FUNC
#define qurl_mutex_create(x) QURL_CFG_PORT_MUTEX_CREATE_FUNC(x)
#else
#define qurl_mutex_create(x)
#endif
/** Delete lock*/
#ifdef QURL_CFG_PORT_MUTEX_DELETE_FUNC
#define qurl_mutex_delete(x) QURL_CFG_PORT_MUTEX_DELETE_FUNC(x)
#else
#define qurl_mutex_delete(x)
#endif
/** Apply for lock*/
#ifdef QURL_CFG_PORT_MUTEX_LOCK_FUNC
#define qurl_mutex_lock(x, y) QURL_CFG_PORT_MUTEX_LOCK_FUNC(x, y)
#else
#define qurl_mutex_lock(x, y)
#endif
/** Release lock*/
#ifdef QURL_CFG_PORT_MUTEX_UNLOCK_FUNC
#define qurl_mutex_unlock(x) QURL_CFG_PORT_MUTEX_UNLOCK_FUNC(x)
#else
#define qurl_mutex_unlock(x)
#endif
/** Create event*/
#ifdef QURL_CFG_PORT_EVENT_CREATE_FUNC
#define qurl_event_create(x, y) QURL_CFG_PORT_EVENT_CREATE_FUNC(x, y)
#else
#define qurl_event_create(x, y)
#endif
/** Delete event*/
#ifdef QURL_CFG_PORT_EVENT_DELETE_FUNC
#define qurl_event_delete(x) QURL_CFG_PORT_EVENT_DELETE_FUNC(x)
#else
#define qurl_event_delete(x)
#endif
/** Wait for event*/
#ifdef QURL_CFG_PORT_EVENT_WAIT_FUNC
#define qurl_event_wait(x, y, z) QURL_CFG_PORT_EVENT_WAIT_FUNC(x, y, z)
#else
#define qurl_event_wait(x, y, z)
#endif
/** Send event*/
#ifdef QURL_CFG_PORT_EVENT_SEND_FUNC
#define qurl_event_send(x, y, z) QURL_CFG_PORT_EVENT_SEND_FUNC(x, y, z)
#else
#define qurl_event_send(x, y, z)
#endif
/** Get current thread ID*/
#ifdef QURL_CFG_PORT_GET_THREAD_ID_FUNC
#define qurl_get_thread_id() QURL_CFG_PORT_GET_THREAD_ID_FUNC()
#else
#define qurl_get_thread_id()
#endif

/** INET PORT */
#ifdef QURL_CFG_PORT_INET_PTON_FUNC
#define qurl_inet_pton(x, y, z) QURL_CFG_PORT_INET_PTON_FUNC(x, y, z)
#else
#define qurl_inet_pton(x, y, z) inet_pton(x, y, z)
#endif
#ifdef QURL_CFG_PORT_INET_NTOP_FUNC
#define qurl_inet_ntop(x, y, z, a) QURL_CFG_PORT_INET_NTOP_FUNC(x, y, z, a)
#else
#define qurl_inet_ntop(x, y, z, a) inet_ntop(x, y, z, a)
#endif
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
/** NETWORK PORT */
#ifdef QURL_CFG_PORT_NW_ACTIVATE_FUNC
#define qurl_port_nw_activate(x) QURL_CFG_PORT_NW_ACTIVATE_FUNC(x)
#else
#define qurl_port_nw_activate(x) qurl_defaule_port_nw_activate(x)
#endif
#ifdef QURL_CFG_PORT_NW_IS_ACTIVATED_FUNC
#define qurl_port_nw_is_activated(x) QURL_CFG_PORT_NW_IS_ACTIVATED_FUNC(x)
#else
#define qurl_port_nw_is_activated(x) qurl_defaule_port_nw_is_activated(x)
#endif
#ifdef QURL_CFG_PORT_NW_GET_IP_FUNC
#define qurl_port_nw_get_ip(x, y, yl, z, zl) QURL_CFG_PORT_NW_GET_IP_FUNC(x, y, yl, z, zl)
#else
#define qurl_port_nw_get_ip(x, y, yl, z, zl) qurl_defaule_port_nw_get_ip(x, y, yl, z, zl)
#endif
#endif

/** SOCKET PORT */
/** Type*/
#ifdef QURL_CFG_PORT_SOCKET_SOCKETLEN_TYPE
#define qurl_socklen_t QURL_CFG_PORT_SOCKET_SOCKETLEN_TYPE
#else
#define qurl_socklen_t socklen_t
#endif
/** dns */
#ifdef QURL_CFG_PORT_SOCKET_RESOLVE_FUNC
#define qurl_socket_resolve(x, y, z, a, b) QURL_CFG_PORT_SOCKET_RESOLVE_FUNC(x, y, z, a, b)
#endif
#ifdef QURL_CFG_PORT_SOCKET_RESOLVE_ASYNC_FUNC
#define qurl_socket_resolve_async(x, y, z, a, b) QURL_CFG_PORT_SOCKET_RESOLVE_ASYNC_FUNC(x, y, z, a, b)
#endif
#ifdef QURL_CFG_PORT_SOCKET_ADDRINFO_FREE_FUNC
#define qurl_socket_addinfo_free(x) QURL_CFG_PORT_SOCKET_ADDRINFO_FREE_FUNC(x)
#endif
/** socket */
#ifdef QURL_CFG_PORT_SOCKET_SOCKET_FUNC
#define qurl_socket_socket(x, y, z) QURL_CFG_PORT_SOCKET_SOCKET_FUNC(x, y, z)
#else
#define qurl_socket_socket(x)
#endif
/** bind */
#ifdef QURL_CFG_PORT_SOCKET_BIND_FUNC
#define qurl_socket_bind(x, y, z) QURL_CFG_PORT_SOCKET_BIND_FUNC(x, y, z)
#else
#define qurl_socket_bind(x)
#endif
/** listen */
#ifdef QURL_CFG_PORT_SOCKET_LISTEN_FUNC
#define qurl_socket_listen(x, y) QURL_CFG_PORT_SOCKET_LISTEN_FUNC(x, y)
#else
#define qurl_socket_listen(x, y)
#endif
/** accept */
#ifdef QURL_CFG_PORT_SOCKET_ACCEPT_FUNC
#define qurl_socket_accept(x, y, z) QURL_CFG_PORT_SOCKET_ACCEPT_FUNC(x, y, z)
#else
#define qurl_socket_accept(x, y, z)
#endif
/** connect */
#ifdef QURL_CFG_PORT_SOCKET_CONNECT_FUNC
#define qurl_socket_connect(x, y, z) QURL_CFG_PORT_SOCKET_CONNECT_FUNC(x, y, z)
#else
#define qurl_socket_connect(x, y, z)
#endif
/** read */
#ifdef QURL_CFG_PORT_SOCKET_READ_FUNC
#define qurl_socket_read(x, y, z) QURL_CFG_PORT_SOCKET_READ_FUNC(x, y, z)
#else
#define qurl_socket_read(x, y, z)
#endif
/** write */
#ifdef QURL_CFG_PORT_SOCKET_WRITE_FUNC
#define qurl_socket_write(x, y, z) QURL_CFG_PORT_SOCKET_WRITE_FUNC(x, y, z)
#else
#define qurl_socket_write(x, y, z)
#endif
/** select */
#ifdef QURL_CFG_PORT_SOCKET_SELECT_FUNC
#define qurl_socket_select(x, y, z, a, b) QURL_CFG_PORT_SOCKET_SELECT_FUNC(x, y, z, a, b)
#else
#define qurl_socket_select(x, y, z, a, b)
#endif
/** close */
#ifdef QURL_CFG_PORT_SOCKET_CLOSE_FUNC
#define qurl_socket_close(x) QURL_CFG_PORT_SOCKET_CLOSE_FUNC(x)
#else
#define qurl_socket_close(x)
#endif
/** getsockname */
#ifdef QURL_CFG_PORT_SOCKET_GETSOCKNAME_FUNC
#define qurl_socket_getsockname(x, y, z) QURL_CFG_PORT_SOCKET_CLOSE_FUNC(x, y, z)
#else
#define qurl_socket_getsockname(x, y, z) getsockname(x, y, z)
#endif
/** setsockopt */
#ifdef QURL_CFG_PORT_SOCKET_SETSOCKOPT_FUNC
#define qurl_socket_setsockopt(x, y, z, a, b) QURL_CFG_PORT_SOCKET_SETSOCKOPT_FUNC(x, y, z, a, b)
#else
#define qurl_socket_setsockopt(x, y, z, a, b)
#endif
/*** SOL_SOCKET */
#ifdef QURL_CFG_PORT_SOCKET_OPT_SOL_SOCKET
#define QURL_SOCKET_SOL_SOCKET QURL_CFG_PORT_SOCKET_OPT_SOL_SOCKET
#else
#define QURL_SOCKET_SOL_SOCKET SOL_SOCKET
#endif
/*** SO_LINGER */
#ifdef QURL_CFG_PORT_SOCKET_OPT_SO_LINGER
#define QURL_SOCKET_SO_LINGER QURL_CFG_PORT_SOCKET_OPT_SO_LINGER
#else
#define QURL_SOCKET_SO_LINGER (0)
#endif
/*** SO_KEEPALIVE */
#ifdef QURL_CFG_PORT_SOCKET_OPT_SO_KEEPALIVE
#define QURL_SOCKET_SO_KEEPALIVE QURL_CFG_PORT_SOCKET_OPT_SO_KEEPALIVE
#else
#define QURL_SOCKET_SO_KEEPALIVE (0)
#endif
/*** IPPROTO_TCP */
#ifdef QURL_CFG_PORT_SOCKET_OPT_IPPROTO_TCP
#define QURL_SOCKET_IPPROTO_TCP QURL_CFG_PORT_SOCKET_OPT_IPPROTO_TCP
#else
#define QURL_SOCKET_IPPROTO_TCP IPPROTO_TCP
#endif
/*** TCP_KEEPIDLE */
#ifdef QURL_CFG_PORT_SOCKET_OPT_TCP_KEEPIDLE
#define QURL_SOCKET_TCP_KEEPIDLE QURL_CFG_PORT_SOCKET_OPT_TCP_KEEPIDLE
#else
#define QURL_SOCKET_TCP_KEEPIDLE (0)
#endif
/*** TCP_KEEPINTVL */
#ifdef QURL_CFG_PORT_SOCKET_OPT_TCP_KEEPINTVL
#define QURL_SOCKET_TCP_KEEPINTVL QURL_CFG_PORT_SOCKET_OPT_TCP_KEEPINTVL
#else
#define QURL_SOCKET_TCP_KEEPINTVL (0)
#endif
/*** TCP_KEEPCNT */
#ifdef QURL_CFG_PORT_SOCKET_OPT_TCP_KEEPCNT
#define QURL_SOCKET_TCP_KEEPCNT QURL_CFG_PORT_SOCKET_OPT_TCP_KEEPCNT
#else
#define QURL_SOCKET_TCP_KEEPCNT (0)
#endif
/** NONBLOCK */
#ifdef QURL_CFG_PORT_SOCKET_NONBLOCK_FUNC
#define qurl_socket_nonblock(x, y) QURL_CFG_PORT_SOCKET_NONBLOCK_FUNC(x, y)
#else
#define qurl_socket_nonblock(x, y)
#endif
/** errno */
#ifdef QURL_CFG_PORT_SOCKET_GET_ERRNO_FUNC
#define qurl_socket_get_errno(x) QURL_CFG_PORT_SOCKET_GET_ERRNO_FUNC(x)
#else
#define qurl_socket_get_errno(x)
#endif
/** last close event **/
#ifdef QURL_CFG_PORT_SOCKET_GET_LAST_CLOSE_EVENT_FUNC
#define qurl_socket_get_last_close_event(x) QURL_CFG_PORT_SOCKET_GET_LAST_CLOSE_EVENT_FUNC(x)
#else
#define qurl_socket_get_last_close_event(x) (0)
#endif

/** VFS PORT */
/*** open */
#if ((QURL_CFG_ENABLE_FS != 0) && (defined QURL_CFG_PORT_FOPEN_FUNC))
#define qurl_fopen(x, y) QURL_CFG_PORT_FOPEN_FUNC(x, y)
#else
#define qurl_fopen(x, y) qurl_default_open(x, y)
#endif
/*** read */
#if ((QURL_CFG_ENABLE_FS != 0) && (defined QURL_CFG_PORT_FREAD_FUNC))
#define qurl_fread(x, y, z) QURL_CFG_PORT_FREAD_FUNC(x, y, z)
#else
#define qurl_fread(x, y, z) qurl_default_read(x, y, z)
#endif
/*** get_size */
#if ((QURL_CFG_ENABLE_FS != 0) && (defined QURL_CFG_PORT_FGETSIZE_FUNC))
#define qurl_fgetsize(x) QURL_CFG_PORT_FGETSIZE_FUNC(x)
#else
#define qurl_fgetsize(x) qurl_default_getsize(x)
#endif
/*** lseek */
#if ((QURL_CFG_ENABLE_FS != 0) && (defined QURL_CFG_PORT_FLSEEK_FUNC))
#define qurl_flseek(x, y, z) QURL_CFG_PORT_FLSEEK_FUNC(x, y, z)
#else
#define qurl_flseek(x, y, z) qurl_default_lseek(x, y, z)
#endif
/*** close */
#if ((QURL_CFG_ENABLE_FS != 0) && (defined QURL_CFG_PORT_FCLOSE_FUNC))
#define qurl_fclose(x) QURL_CFG_PORT_FCLOSE_FUNC(x)
#else
#define qurl_fclose(x) qurl_default_close(x)
#endif
/*** opt O_RDONLY */
#ifdef QURL_CFG_PORT_F_O_RDONLY
#define QURL_O_RDONLY QURL_CFG_PORT_F_O_RDONLY
#else
#define QURL_O_RDONLY (0)
#endif
/*** opt O_RDWR */
#ifdef QURL_CFG_PORT_F_O_RDWR
#define QURL_O_RDWR QURL_CFG_PORT_F_O_RDWR
#else
#define QURL_O_RDWR (0)
#endif
/*** opt O_CREAT */
#ifdef QURL_CFG_PORT_F_O_CREAT
#define QURL_O_CREAT QURL_CFG_PORT_F_O_CREAT
#else
#define QURL_O_CREAT (0)
#endif
/*** opt O_EXCL */
#ifdef QURL_CFG_PORT_F_O_EXCL
#define QURL_O_EXCL QURL_CFG_PORT_F_O_EXCL
#else
#define QURL_O_EXCL (0)
#endif

/** TLS PORT */
/*** base64 */
#ifdef QURL_CFG_PORT_TLS_BASE64_ENCODE_FUNC
#define qurl_tls_base64_encode(a, b, x, y, z) QURL_CFG_PORT_TLS_BASE64_ENCODE_FUNC(a, b, x, y, z)
#else
#define qurl_tls_base64_encode(a, b, x, y, z) qurl_default_base64_encode(a, b, x, y, z)
#endif
#ifdef QURL_CFG_PORT_TLS_BASE64_DECODE_FUNC
#define qurl_tls_base64_decode(a, b, x, y, z) QURL_CFG_PORT_TLS_BASE64_DECODE_FUNC(a, b, x, y, z)
#else
#define qurl_tls_base64_decode(a, b, x, y, z) qurl_default_base64_decode(a, b, x, y, z)
#endif

/*Macro function verification*/
#include "qurl_config_check.h"

#endif /* Head define end*/
