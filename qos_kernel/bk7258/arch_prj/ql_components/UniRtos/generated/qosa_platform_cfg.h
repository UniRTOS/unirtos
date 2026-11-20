/*********************************************************************
 * @file   osa_platform_config.h
 * @brief  When adapting to various platforms, it is necessary to
 *         ensure compatibility with system library functions used
 *         on different platforms in order to prevent the inability
 *         to run due to the absence of library functions when using
 *          UniRTOS on new platforms.
 * @author larson.li@quectel.com
 * @date   2023-03-28
 *
 * @copyright  Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
 * All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
 *
 * @par EDIT HISTORY FOR MODULE
 * <table>
 * <tr><th>Date       <th>Version    <th>Author          <th>Description"
 * <tr><td>2023-03-28 <td>1.0        <td>Larson.Li       <td> Init
 * </table>
 **********************************************************************/

#ifndef ___QOSA_PLATFORM_CONFIG_H__
#define ___QOSA_PLATFORM_CONFIG_H__

// Buffer array space size definition
#define QOSA_ARRAY_BYTE_32                    32
#define QOSA_ARRAY_BYTE_64                    64
#define QOSA_ARRAY_BYTE_128                   128
#define QOSA_ARRAY_BYTE_256                   256
#define QOSA_ARRAY_BYTE_512                   512
#define QOSA_ARRAY_BYTE_1024                  1024
#define QOSA_ARRAY_BYTE_2048                  2048

/************************************************************************/
/* Define whether system function interfaces use platform interfaces or custom encapsulated interfaces */
/************************************************************************/
#define CONFIG_QOSA_HAVE_MALLOC
#define CONFIG_QOSA_HAVE_STRSTR
#define CONFIG_QOSA_HAVE_STRCASECMP
#define CONFIG_QOSA_HAVE_STRNCPY
#define CONFIG_QOSA_HAVE_STRCPY
#define CONFIG_QOSA_HAVE_STRNCMP
#define CONFIG_QOSA_HAVE_STRCMP
#define CONFIG_QOSA_HAVE_STRLEN
#define CONFIG_QOSA_HAVE_STRNLEN
#define CONFIG_QOSA_HAVE_STRDUP
#define CONFIG_QOSA_HAVE_VSNPRINTF
#define CONFIG_QOSA_HAVE_SNPRINTF
#define CONFIG_QOSA_HAVE_VSPRINTF
#define CONFIG_QOSA_HAVE_SPRINTF
#define CONFIG_QOSA_HAVE_STRNCASECMP
#define CONFIG_QOSA_HAVE_STRCHR
#define CONFIG_QOSA_HAVE_STRTOUL
#define CONFIG_QOSA_HAVE_STRTOL
#define CONFIG_QOSA_HAVE_STRTOULL
#define CONFIG_QOSA_HAVE_SSCANF
#define CONFIG_QOSA_HAVE_STRRCHR
#define CONFIG_QOSA_HAVE_STRTOLL
#define CONFIG_QOSA_HAVE_STRTOD
#define CONFIG_QOSA_HAVE_STRCAT
#define CONFIG_QOSA_HAVE_STRNCAT
#define CONFIG_QOSA_HAVE_STRTOK
#define CONFIG_QOSA_HAVE_STRPBRK
#define CONFIG_QOSA_HAVE_STRSPN
#define CONFIG_QOSA_HAVE_STRCSPN
#define CONFIG_QOSA_HAVE_ATOI
#define CONFIG_QOSA_HAVE_ATOL
#define CONFIG_QOSA_HAVE_ATOF
#define CONFIG_QOSA_HAVE_SYSTEM
#define CONFIG_QOSA_HAVE_RAND
/* #undef CONFIG_QOSA_HAVE_STRNSTR */

/************************************************************************/
/* Platform difference related configuration */
/************************************************************************/

/** In normal compilation environments, long occupies 8 bytes under linux64/ARM64, while other Win32/win64/linux32/ARM32 occupy 4 bytes */
/** Currently, do not enable this macro for Unisoc and Eigen platforms */
/* #undef CONFIG_QOSA_HAVE_SYS_TPYE64 */

/** Whether the platform supports C99 standard definition specifications, such as bool type int8_t/uint8_t and other types */
#define CONFIG_QOSA_PLATFORM_SUPPORT_C99

/** Refine platform priority unit, if the platform's priority definition is larger with higher priority then it's negative, if priority is smaller with higher priority then it's positive */
#define CONFIG_QOSA_PRIORITY_GRANULARITY_NUM -1

/** Platform task priority configuration sequence */
#define CONFIG_QOSA_PRIORITY_REALTIME_CFG 9
#define CONFIG_QOSA_PRIORITY_HIGH_CFG 8
#define CONFIG_QOSA_PRIORITY_ABOVE_NORMAL_CFG 5
#define CONFIG_QOSA_PRIORITY_NORMAL_CFG 4
#define CONFIG_QOSA_PRIORITY_BELOW_NORMAL_CFG 3
#define CONFIG_QOSA_PRIORITY_LOW_CFG 2
#define CONFIG_QOSA_PRIORITY_IDLE_CFG 1


/** Define the maximum length of driver setting character names */
#define CONFIG_QOSA_DEVICE_NAME_MAX 8

/** Define structure alignment length */
#define CONFIG_QOSA_ALIGN_SIZE 4


/* the max number that is allowed to be registered of every single event */
/************************************************************************/
/* A single Event type can accept up to 20 registered cb functions */
/************************************************************************/
// September 29, 2024, 20:38:49 If needed in the future, gradually increase to reduce space
#define CONFIG_QOSA_EVENT_NOTIFY_REGISTER_MAX_EVENT 17

/************************************************************************/
/* File system corresponding space size configuration table */
/************************************************************************/
/** Maximum file name length */
#define CONFIG_QOSA_FILE_MAX_NAME_LEN 63

/** Maximum folder path length */
#define CONFIG_QOSA_FILE_MAX_FOLDER_LEN 63

/** Maximum file path prefix length eg: EXNSFFS: */
#define CONFIG_QOSA_FILE_PREFIX_LEN 10

// Maximum length of NVM JSON node
#define CONFIG_QOSA_NVM_JSON_NODE_MAX_LEN 40

/************************************************************************/
/* fota configuration table */
/************************************************************************/
//fota updater report urc number
#define CONFIG_QOSA_FOTA_UPDATE_URC_MAX 100

/************************************************************************/
/* UniRTOS custom pool size calculation method */
/************************************************************************/
/** Custom heap header length */
#define CONFIG_QOSA_MEMORYI_HEAP_HEAD_SIZE 64

/** Custom single ITEM size */
#define CONFIG_QOSA_MEMORY_BUF_ITEM_SIZE 256

/** Custom ITEM count */
#define CONFIG_QOSA_MEMORY_BUF_ITEM_CNT 96

/************************************************************************/
/* UniRTOS pdp activation profile related */
/************************************************************************/
/** pdp activation apn username password maximum length */
/* #undef CONFIG_QOSA_PDP_USERPWD_PROFILE_SIZE */

/************************************************************************/
/* IP address length related */
/************************************************************************/
/** IPv4 address maximum length */
#define CONFIG_QOSA_INET_ADDRSTRLEN 16

/** IPv6 address maximum length */
#define CONFIG_QOSA_INET6_ADDRSTRLEN 48

/** Maximum length of user input domain name */
#define CONFIG_QOSA_HOST_URL_MAX_LEN 256

/************************************************************************/
/* unirtos platform level configuration macro definition */
/************************************************************************/
/** File system reserved space size, mainly reserved for NV saving */
#define CONFIG_QOSA_FILE_CONFIG_RESERVE_SIZE 10240

/** System ticks per second */
#define CONFIG_QOSA_SYS_TICK_PER_SECOND 1000

/** Support unirtos custom mbedtls memory usage optimization and trimming */
#define CONFIG_QOSA_MBEDTLS_MEMORY_OPTIMIZTAION_SUPPORT

/** Enable mbedTLS ECDHE-ECDSA key exchange support */
/* #undef CONFIG_QOSA_MBEDTLS_ECDHE_ECDSA_SUPPORT */

/** Enable mbedTLS SHA384 hash algorithm support */
/* #undef CONFIG_QOSA_MBEDTLS_SHA384_SUPPORT */


/************************************************************************/
/* unirtos system side business task space configuration */
/************************************************************************/
// WIFISCAN TASK stack size
/* #undef CONFIG_QOSA_COMP_WIFISCAN_TASK_SIZE */

// DNS task stack size
#define CONFIG_QOSA_SYS_DNS_TASK_SIZE 2048

// Destroy task stack size
#define CONFIG_QOSA_SYS_DESTROY_SIZE 2048

// AT parser core center task stack size
#define CONFIG_QOSA_CENTER_TASK_STACK_SIZE 5120

/**
 * @brief LPM thread stack size
 * @brief Stack size for LPM task
 */
#define CONFIG_QOSA_SYS_LPM_TASK_SIZE 4096

/**
 * @brief LPM thread stack size
 * @brief Stack size for LPM RFR task
 */
#define CONFIG_QOSA_SYS_LPM_RFR_TASK_SIZE 2048

/**
 * @brief LPM thread stack size
 * @brief Stack size for LPM PLAT task
 */
#define CONFIG_QOSA_SYS_LPM_PLAT_TASK_SIZE 1024

/**
 * @brief LPM sleep debug thread stack size
 * @brief Stack size for LPM task
 */
/* #undef CONFIG_QOSA_SYS_LPM_LOG_TASK_SIZE */

/**
 * @brief SERIAL EVENT thread stack size
 * @brief Stack size for SERIAL EVENT task
 */
/* #undef CONFIG_QOSA_SERIAL_EVENT_TASK_SIZE */



/** Has Watermark enabled item merge optimization */
#define CONFIG_QOSA_WATERMARK_ITEM_MERGE

#endif /* ___QOSA_PLATFORM_CONFIG_H__ */
