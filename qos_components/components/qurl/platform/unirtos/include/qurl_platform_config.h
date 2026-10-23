/** @file         qurl_platform_config.h
 *  @brief        Brief Description
 *  @details      Detailed Description
 *  @author       lzm
 *  @date         2023-11-16 15:26:03
 *  @version      v1.0
 *  @copyright    Copyright By lizhuming, All Rights Reserved
 *
 **********************************************************
 *  @LOG Change Log: -----
 **********************************************************
 */

#ifndef QURL_PLATFORM_CONFIG_H
#define QURL_PLATFORM_CONFIG_H

/** Platform side adds header file*/
#include "qosa_virtual_file.h"
#ifndef CONFIG_QOSA_QT2131_PLATFORM_FUNC
// open_source/lwip/lwip_v2.1.3/src/include/lwip/inet.h
// open_source/musl/musl_1.2.3/include/netinet/in.h
// drivers/chips/cat1/opensource_adapter/lwip_adapter/public/soc_socket.h

// The above-mentioned files in xqd have conflicts due to redefinition.
#include <netdb.h>
#endif /* CONFIG_QOSA_QT2131_PLATFORM_FUNC */
#include <errno.h>
#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_log.h"
#include "qosa_rtc.h"
#include "qosa_errno.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
#include "qosa_datacall.h"
#else
#include "qosa_ip_addr.h"
#endif
#include "qcm_proj_config.h"

#include "qosa_sockets.h"

/** Maximum number of concurrent services*/
#define QURL_CFG_CORE_SERVER_MAX      (15)

/** Error code base value (the highest bit of the system must be 1)*/
// #define QURL_CFG_ECODE_BASIC_NUM (0x80010000)
/** Event code base value (the highest bit of the system must be 1)*/
#define QURL_CFG_EVENT_CODE_BASIC_NUM (0x80020000)

/*Protocol Family*/
#if defined(LWIP_IPV6)
#define QURL_CFG_ENABLE_IPV6          LWIP_IPV6
#else
#define QURL_CFG_ENABLE_IPV6          1
#endif

/*IPV6 Address*/
#define QURL_IN6ADDR_ANY_INIT                                                                                                                                  \
    {                                                                                                                                                          \
        {                                                                                                                                                      \
            {                                                                                                                                                  \
                0, 0, 0, 0                                                                                                                                     \
            }                                                                                                                                                  \
        }                                                                                                                                                      \
    }

/* LOG */
/** Enable QURL LOG*/
#define QURL_CFG_LOG       1

/*Supported Protocol Types and Functions*/
#define QURL_CFG_ENABLE_FS 1
#ifdef CONFIG_QCM_VTLS_FUNC
#define QURL_CFG_ENABLE_TLS_PLATFORM 1
#else  /* CONFIG_QCM_VTLS_FUNC */
#define QURL_CFG_ENABLE_TLS_PLATFORM 0
#endif /* CONFIG_QCM_VTLS_FUNC */

#ifdef CONFIG_QCM_QURL_HTTP_FUNC
#define QURL_CFG_ENABLE_HTTP 1
#else  /* CONFIG_QCM_QURL_HTTP_FUNC */
#define QURL_CFG_ENABLE_HTTP 0
#endif /* CONFIG_QCM_QURL_HTTP_FUNC */

#ifdef CONFIG_QCM_QURL_FTP_FUNC
#define QURL_CFG_ENABLE_FTP 1
#else  /* CONFIG_QCM_QURL_FTP_FUNC */
#define QURL_CFG_ENABLE_FTP 0
#endif /* CONFIG_QCM_QURL_FTP_FUNC */

#ifdef CONFIG_QCM_QURL_SMTP_FUNC
#define QURL_CFG_ENABLE_SMTP 1
#else  /* CONFIG_QCM_QURL_SMTP_FUNC */
#define QURL_CFG_ENABLE_SMTP 0
#endif /* CONFIG_QCM_QURL_SMTP_FUNC */

/*HTTP specifies the default boundary*/
#if !defined(CONFIG_QOSA_QCX216_PLATFORM_FUNC) && !defined(CONFIG_QOSA_QCX217_PLATFORM_FUNC)
#define QURL_CFG_MULTIFORM_BOUNDARY_DEFAULT "-------Quectel1234567890Quectel"
#else
#define QURL_CFG_MULTIFORM_BOUNDARY_DEFAULT "---------------------------7e3291262b03f0"
#endif

/** Standard Library Related APIs*/
#define QURL_CFG_PORT_MALLOC_FUNC                      qosa_malloc
#define QURL_CFG_PORT_FREE_FUNC                        qosa_free
#define QURL_CFG_PORT_ATOI_FUNC                        qosa_atoi
#define QURL_CFG_PORT_SNPRINTF_FUNC                    qosa_snprintf
#define QURL_CFG_PORT_VSNPRINTF_FUNC                   qosa_vsnprintf
#define QURL_CFG_PORT_STRNCASECMP_FUNC                 qosa_strncasecmp
#define QURL_CFG_PORT_STRTOL_FUNC                      qosa_strtol
#define QURL_CFG_PORT_STRTOUL_FUNC                     qosa_strtoul
#define QURL_CFG_PORT_STRDUP_FUNC                      qosa_strdup
#define QURL_CFG_PORT_STRCHR_FUNC                      qosa_strchr
#define QURL_CFG_PORT_STRRCHR_FUNC                     qosa_strrchr
#define QURL_CFG_PORT_STRCPY_FUNC                      qosa_strcpy
#define QURL_CFG_PORT_STRNCPY_FUNC                     qosa_strncpy
#define QURL_CFG_PORT_STRLEN_FUNC                      qosa_strlen
#define QURL_CFG_PORT_STRCMP_FUNC                      qosa_strcmp
#define QURL_CFG_PORT_STRNCMP_FUNC                     qosa_strncmp
#define QURL_CFG_PORT_MEMCPY_FUNC                      qosa_memcpy
#define QURL_CFG_PORT_MEMSET_FUNC                      qosa_memset
#define QURL_CFG_PORT_MEMCHR_FUNC                      qosa_memchr
#define QURL_CFG_PORT_MEMCMP_FUNC                      qosa_memcmp
#define QURL_CFG_PORT_TOLOWER_FUNC                     qosa_tolower
#define QURL_CFG_PORT_STRCASECMP_FUNC                  qosa_strcasecmp
#define QURL_CFG_PORT_STRSTR_FUNC                      qosa_strstr
#define QURL_CFG_PORT_STRSPN_FUNC                      qosa_strspn
#define QURL_CFG_PORT_STRCSPN_FUNC                     qosa_strcspn
#define QURL_CFG_PORT_SSCANF_FUNC                      qosa_sscanf
#define QURL_CFG_PORT_GET_ERRNO_FUNC                   qosa_get_errno
#define QURL_CFG_PORT_STRPBRK_FUNC                     qosa_strpbrk

/** OS: Description, FUNC has format type requirements*/
#define QURL_CFG_PORT_THREAD_CREATE_FUNC               qurl_platform_thread_create
#define QURL_CFG_PORT_THREAD_DELETE_FUNC               qurl_platform_thread_delete
#define QURL_CFG_PORT_SLEEP_MS_FUNC                    qurl_platform_sleep_ms
#define QURL_CFG_PORT_MUTEX_CREATE_FUNC                qurl_platform_mutex_create
#define QURL_CFG_PORT_MUTEX_DELETE_FUNC                qurl_platform_mutex_delete
#define QURL_CFG_PORT_MUTEX_LOCK_FUNC                  qurl_platform_mutex_lock
#define QURL_CFG_PORT_MUTEX_UNLOCK_FUNC                qurl_platform_mutex_unlock
#define QURL_CFG_PORT_EVENT_CREATE_FUNC                qurl_platform_event_create
#define QURL_CFG_PORT_EVENT_DELETE_FUNC                qurl_platform_event_delete
#define QURL_CFG_PORT_EVENT_WAIT_FUNC                  qurl_platform_event_wait
#define QURL_CFG_PORT_EVENT_SEND_FUNC                  qurl_platform_event_send
#define QURL_CFG_PORT_GET_THREAD_ID_FUNC               qurl_platform_get_pthread_id
#define QURL_CFG_PORT_TIME_NOW_FUNC                    qurl_platform_time_now
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
/** NETWORK: Description, FUNC has format type requirements*/
#define QURL_CFG_PORT_NW_ACTIVATE_FUNC                 qurl_platform_nw_activate
#define QURL_CFG_PORT_NW_IS_ACTIVATED_FUNC             qurl_platform_nw_is_activated
#define QURL_CFG_PORT_NW_GET_IP_FUNC                   qurl_platform_nw_get_ip
#endif
/** SOCKET: Description, FUNC has format type requirements*/
#define QURL_CFG_PORT_SOCKET_RESOLVE_FUNC              qurl_platform_socket_dns_get
#define QURL_CFG_PORT_SOCKET_ADDRINFO_FREE_FUNC        freeaddrinfo

#ifdef CONFIG_QOSA_QT2131_PLATFORM_FUNC
#define QURL_CFG_PORT_SOCKET_SOCKET_FUNC               uapi_lwip_socket
#define QURL_CFG_PORT_SOCKET_BIND_FUNC                 uapi_lwip_bind
#define QURL_CFG_PORT_SOCKET_LISTEN_FUNC               uapi_lwip_listen
#define QURL_CFG_PORT_SOCKET_ACCEPT_FUNC               uapi_lwip_accept
#define QURL_CFG_PORT_SOCKET_CONNECT_FUNC              uapi_lwip_connect
#define QURL_CFG_PORT_SOCKET_READ_FUNC                 uapi_lwip_read
#define QURL_CFG_PORT_SOCKET_WRITE_FUNC                uapi_lwip_write
#define QURL_CFG_PORT_SOCKET_SELECT_FUNC               uapi_lwip_select
#define QURL_CFG_PORT_SOCKET_CLOSE_FUNC                uapi_lwip_close
#define QURL_CFG_PORT_SOCKET_SETSOCKOPT_FUNC           uapi_lwip_setsockopt
#else /* CONFIG_QOSA_QT2131_PLATFORM_FUNC */
#define QURL_CFG_PORT_SOCKET_SOCKET_FUNC               socket
#define QURL_CFG_PORT_SOCKET_BIND_FUNC                 bind
#define QURL_CFG_PORT_SOCKET_LISTEN_FUNC               listen
#define QURL_CFG_PORT_SOCKET_ACCEPT_FUNC               accept
#define QURL_CFG_PORT_SOCKET_CONNECT_FUNC              connect
#define QURL_CFG_PORT_SOCKET_READ_FUNC                 read
#define QURL_CFG_PORT_SOCKET_WRITE_FUNC                write
#define QURL_CFG_PORT_SOCKET_SELECT_FUNC               select
#define QURL_CFG_PORT_SOCKET_CLOSE_FUNC                close
#define QURL_CFG_PORT_SOCKET_SETSOCKOPT_FUNC           setsockopt
#endif /* CONFIG_QOSA_QT2131_PLATFORM_FUNC */

#define QURL_CFG_PORT_SOCKET_NONBLOCK_FUNC             qurl_platform_socket_nonblock
#define QURL_CFG_PORT_SOCKET_GET_ERRNO_FUNC            qurl_platform_socket_get_errno
#define QURL_CFG_PORT_SOCKET_OPT_SO_KEEPALIVE          SO_KEEPALIVE
#define QURL_CFG_PORT_SOCKET_OPT_TCP_KEEPIDLE          TCP_KEEPIDLE
#define QURL_CFG_PORT_SOCKET_OPT_TCP_KEEPINTVL         TCP_KEEPINTVL
#define QURL_CFG_PORT_SOCKET_OPT_TCP_KEEPCNT           TCP_KEEPCNT
#define QURL_CFG_PORT_IN6ADDR_ANY_INIT                 (struct in6_addr) QURL_IN6ADDR_ANY_INIT
#define QURL_CFG_PORT_SOCKET_GET_LAST_CLOSE_EVENT_FUNC qurl_platform_get_last_close_event

/** FS */
#define QURL_CFG_PORT_FOPEN_FUNC                       qosa_vfs_open
#define QURL_CFG_PORT_FREAD_FUNC                       qosa_vfs_read
#define QURL_CFG_PORT_FGETSIZE_FUNC                    qurl_platform_fgetsize
#define QURL_CFG_PORT_FLSEEK_FUNC                      qosa_vfs_lseek
#define QURL_CFG_PORT_FCLOSE_FUNC                      qosa_vfs_close
#define QURL_CFG_PORT_F_O_RDONLY                       QOSA_VFS_O_RDONLY
#define QURL_CFG_PORT_F_O_RDWR                         QOSA_VFS_O_RDWR
#define QURL_CFG_PORT_F_O_CREAT                        QOSA_VFS_O_CREAT
#define QURL_CFG_PORT_F_O_EXCL                         QOSA_VFS_O_EXCL
/** TLS */

/** task info */
#define QURL_CFG_DNS_TASK_PRIORITY                     QOSA_PRIORITY_NORMAL
#define QURL_CFG_NETWORK_TASK_PRIORITY                 QOSA_PRIORITY_NORMAL

#define QURL_CFG_DNS_TASK_STACK_SIZE                   (3072)
#define QURL_CFG_NETWORK_TASK_STACK_SIZE               (3072)

#endif /* Head define end*/
