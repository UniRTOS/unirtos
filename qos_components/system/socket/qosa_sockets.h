/*****************************************************************/ /**
* @file qosa_sockets.h
* @brief Socket operation interface
* @author larson.li@quectel.com
* @date 2024-09-03
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2024-11-13 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#ifndef __QOSA_SOCKETS_H__
#define __QOSA_SOCKETS_H__

// Common header files
#include <string.h>

// CONFIG_QOSA_LINUX_PLATFORM_FUNC exclusive header files
#ifdef CONFIG_QOSA_LINUX_PLATFORM_FUNC
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <fcntl.h>
#endif /* CONFIG_QOSA_LINUX_PLATFORM_FUNC */

// CONFIG_QOSA_ASR1903_PLATFORM_FUNC exclusive header files
#if defined(CONFIG_QOSA_ASR1903_PLATFORM_FUNC) || defined(CONFIG_QOSA_ASR160X_PLATFORM_FUNC) || defined(CONFIG_QOSA_ASR1805_PLATFORM_FUNC)
#include "inet.h"
//#include "inet6.h"
#include "sockets.h"
#include "lwip/tcp.h"
#include "qosa_errno.h"
#include "lwip/api.h"
#include "lwip/inet_chksum.h"
#endif /* CONFIG_QOSA_ASR1903_PLATFORM_FUNC || CONFIG_QOSA_ASR160X_PLATFORM_FUNC || CONFIG_QOSA_ASR1805_PLATFORM_FUNC */

// CONFIG_QOSA_QCX216_PLATFORM_FUNC || CONFIG_QOSA_QCX217_PLATFORM_FUNC exclusive header files
#if defined(CONFIG_QOSA_QCX216_PLATFORM_FUNC) || defined(CONFIG_QOSA_QCX217_PLATFORM_FUNC) || defined(CONFIG_QOSA_EIGEN718_PLATFORM_FUNC)
#include <unistd.h>
#include "sockets.h"
#include "qosa_errno.h"
#include "lwip/tcp.h"
#include "netdb.h"
#include "lwip/api.h"
#endif /* CONFIG_QOSA_QCX216_PLATFORM_FUNC || CONFIG_QOSA_QCX217_PLATFORM_FUNC || defined(CONFIG_QOSA_EIGEN718_PLATFORM_FUNC)*/

#ifdef CONFIG_QOSA_RDA8910_R06_PLATFORM_FUNC
#include <fcntl.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <errno.h>
#include "lwip/tcp.h"
#include "lwip/inet.h"
#endif /* CONFIG_QOSA_RDA8910_R06_PLATFORM_FUNC */

#ifdef CONFIG_QOSA_UIS8852_PLATFORM_FUNC
#include "lwip/netif.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "lwip/api.h"
#include "lwip/netdb.h"
#endif /* CONFIG_QOSA_UIS8852_PLATFORM_FUNC */

#ifdef CONFIG_QOSA_QT2131_PLATFORM_FUNC
#include "lwip/inet.h"
#include "soc_socket.h"

#endif
#if defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)
#include "lwip/netif.h"
// #include "lwip/sockets.h"
#include "lwip/inet.h"
#include "lwip/api.h"
#include "lwip/netdb.h"
#include "lwip/tcp.h"
#include "lwip/udp.h"
#include "lwip/raw.h"
#endif
#endif /* __QOSA_SOCKETS_H__ */
