/*****************************************************************/ /**
 * @file   qosa_rda_8910_log.h
 * @brief  UIS8910 (R06) platform log adapter for QuecOS
 *
 * Called from QuecOS/R02/quecos/system/debug/qosa_log.h:
 *   #if defined(CONFIG_QOSA_UIS8910_PLATFORM_FUNC)
 *   #include "qosa_rda_8910_log.h"
 *
 * Pattern: follows uis8852's qosa_rda_log.h  calls the R06 KERNEL-native
 * osiTracePrintf() (kernel/include/osi_log_imp.h) directly, NOT the
 * old-quecos osa_log_printf() wrapper.
 *
 * Dual-path:
 *   FIRMWARE:        osiTracePrintf((level<<28)|tag, ...)
 *   UNIRTOS_APPS_COMPILE: snprintf + m_get_api_ptr / osa_trace_debug
 *
 **********************************************************************/
#ifndef __QOSA_RDA_8910_LOG_H__
#define __QOSA_RDA_8910_LOG_H__

#include "qosa_def.h"
#include "osi_log.h"

#ifdef UNIRTOS_APPS_COMPILE
#include <stdio.h>
extern int (*osa_trace_debug)(char *fmt, ...);
extern int (*m_get_api_ptr)(char *func_name);
#endif

/* 1. Log level enum */
typedef enum {
    OSA_LOG_LEVEL_NEVER = 0,
    OSA_LOG_LEVEL_ERROR,
    OSA_LOG_LEVEL_WARN,
    OSA_LOG_LEVEL_INFO,
    OSA_LOG_LEVEL_DEBUG,
    OSA_LOG_LEVEL_VERBOSE,
    OSA_LOG_LEVEL_MAX
} osa_log_level_e;

#define OSA_LOG_LEVEL  OSA_LOG_LEVEL_VERBOSE

/**
  * @enum qosa_log_error_e
  * @brief  Function execution return error codes
  */
typedef enum
{
    QOSA_LOG_ERRID_SUCCESS = 0,
    QOSA_LOG_ERRID_NOT_SUPPORT = 1 | (QOSA_COMPONENT_LOG << 16), /*!< Feature not supported */
    QOSA_LOG_ERRID_INVALID_PARAM,                                /*!< Invalid parameter */

} qosa_log_error_e;

/* 2. Tag encoding  Quectel standard 4-char uint32 */
#define OSA_MAKE_LOG_TAG(a, b, c, d) \
    ((unsigned)(a) | ((unsigned)(b) << 7) | ((unsigned)(c) << 14) | ((unsigned)(d) << 21))

/* 3. SLOG compatibility (required by shared components) */
#define SLOG_LEVEL_NEVER      OSA_LOG_LEVEL_NEVER
#define SLOG_LEVEL_ERROR      OSA_LOG_LEVEL_ERROR
#define SLOG_LEVEL_WARN       OSA_LOG_LEVEL_WARN
#define SLOG_LEVEL_INFO       OSA_LOG_LEVEL_INFO
#define SLOG_LEVEL_DEBUG      OSA_LOG_LEVEL_DEBUG
#define SLOG_LEVEL_VERBOSE    OSA_LOG_LEVEL_VERBOSE
#define SLOG_PRINT_SUBMDL_PUB  0

/* 4. R06 KERNEL-native trace function   */

/* 5. LOG_TAG_* constants */
#define LOG_TAG               OSA_MAKE_LOG_TAG('Q', 'U', 'E', 'C')
#define LOG_TAG_AT            OSA_MAKE_LOG_TAG('Q', 'U', 'E', 'A')
#define LOG_TAG_SIO           OSA_MAKE_LOG_TAG('Q', 'S', 'I', 'O')
#define LOG_TAG_MEMORY        OSA_MAKE_LOG_TAG('Q', 'D', 'S', 'M')
#define LOG_TAG_MEMORY_QUEUE  OSA_MAKE_LOG_TAG('Q', 'D', 'S', 'Q')
#define LOG_TAG_LIST_LINK     OSA_MAKE_LOG_TAG('Q', 'L', 'S', 'T')
#define LOG_TAG_AT_PARSER     OSA_MAKE_LOG_TAG('Q', 'A', 'T', 'P')
#define LOG_TAG_AT_RESP       OSA_MAKE_LOG_TAG('Q', 'A', 'T', 'R')
#define LOG_TAG_AT_SIO        OSA_MAKE_LOG_TAG('Q', 'A', 'T', 'S')
#define LOG_TAG_URC           OSA_MAKE_LOG_TAG('Q', 'U', 'R', 'C')
#define LOG_TAG_EVENT         OSA_MAKE_LOG_TAG('Q', 'E', 'V', 'E')
#define LOG_TAG_VFS           OSA_MAKE_LOG_TAG('Q', 'V', 'F', 'S')
#define LOG_TAG_SFILE         OSA_MAKE_LOG_TAG('Q', 'S', 'F', 'I')
#define LOG_TAG_FILE_API      OSA_MAKE_LOG_TAG('Q', 'F', 'A', 'I')
#define LOG_TAG_SOCK_API      OSA_MAKE_LOG_TAG('Q', 'S', 'O', 'C')
#define LOG_TAG_DNS_API       OSA_MAKE_LOG_TAG('Q', 'D', 'N', 'S')
#define LOG_TAG_MBED_API      OSA_MAKE_LOG_TAG('Q', 'M', 'B', 'D')
#define LOG_TAG_TCPIP_API     OSA_MAKE_LOG_TAG('Q', 'T', 'C', 'P')
#define LOG_TAG_TCPIP_NB_API  OSA_MAKE_LOG_TAG('Q', 'N', 'B', 'I')
#define LOG_TAG_PDP_API       OSA_MAKE_LOG_TAG('Q', 'P', 'D', 'P')
#define LOG_TAG_HTTP_API      OSA_MAKE_LOG_TAG('Q', 'H', 'T', 'P')
#define LOG_TAG_LWM2M_API     OSA_MAKE_LOG_TAG('Q', 'M', '2', 'M')
#define LOG_TAG_MODEM         OSA_MAKE_LOG_TAG('Q', 'M', 'D', 'M')
#define LOG_TAG_SMTP_API      OSA_MAKE_LOG_TAG('S', 'M', 'T', 'P')
#define LOG_TAG_MMS_API       OSA_MAKE_LOG_TAG('Q', 'M', 'M', 'S')
#define LOG_TAG_BASE_AT       OSA_MAKE_LOG_TAG('Q', 'B', 'S', 'A')
#define LOG_TAG_POWER_API     OSA_MAKE_LOG_TAG('Q', 'P', 'O', 'W')
#define LOG_TAG_FTP_API       OSA_MAKE_LOG_TAG('Q', 'F', 'T', 'P')
#define LOG_TAG_MQTT_API      OSA_MAKE_LOG_TAG('Q', 'M', 'Q', 'T')
#define LOG_TAG_WEB_API       OSA_MAKE_LOG_TAG('Q', 'W', 'E', 'B')
#define LOG_TAG_DM_API        OSA_MAKE_LOG_TAG('Q', 'U', 'D', 'M')
#define LOG_TAG_COAP_API      OSA_MAKE_LOG_TAG('Q', 'C', 'O', 'P')
#define LOG_TAG_FOTA_API      OSA_MAKE_LOG_TAG('Q', 'F', 'O', 'T')
#define LOG_TAG_NTP_API       OSA_MAKE_LOG_TAG('Q', 'N', 'T', 'P')
#define LOG_TAG_PING_API      OSA_MAKE_LOG_TAG('Q', 'P', 'I', 'G')
#define LOG_TAG_LBS_API       OSA_MAKE_LOG_TAG('Q', 'L', 'B', 'S')
#define LOG_TAG_WIFISCAN_API  OSA_MAKE_LOG_TAG('Q', 'W', 'I', 'S')
#define LOG_TAG_QURL_API      OSA_MAKE_LOG_TAG('Q', 'U', 'R', 'L')
#define LOG_TAG_COMPONENT     OSA_MAKE_LOG_TAG('Q', 'C', 'M', 'P')
#define LOG_TAG_MINI_HTTP     OSA_MAKE_LOG_TAG('Q', 'M', 'H', 'T')
#define LOG_TAG_SBFOTA_API    OSA_MAKE_LOG_TAG('Q', 'S', 'B', 'F')
#define LOG_TAG_BACKUP_API    OSA_MAKE_LOG_TAG('Q', 'B', 'A', 'C')
#define LOG_TAG_ADC_API       OSA_MAKE_LOG_TAG('Q', 'A', 'D', 'C')
#define LOG_TAG_QOSA          OSA_MAKE_LOG_TAG('Q', 'O', 'S', 'A')
#define LOG_TAG_SIM           OSA_MAKE_LOG_TAG('Q', 'S', 'I', 'M')
#define LOG_TAG_NETWORK       OSA_MAKE_LOG_TAG('Q', 'N', 'W', 'K')
#define LOG_TAG_DATACALL      OSA_MAKE_LOG_TAG('Q', 'D', 'A', 'T')
#define LOG_TAG_NET_API       OSA_MAKE_LOG_TAG('Q', 'N', 'E', 'T')
#define LOG_TAG_USBNET        OSA_MAKE_LOG_TAG('Q', 'U', 'N', 'E')
#define LOG_TAG_DEV           OSA_MAKE_LOG_TAG('Q', 'D', 'E', 'V')
#define LOG_TAG_CALL          OSA_MAKE_LOG_TAG('Q', 'C', 'A', 'L')
#define LOG_TAG_IMS           OSA_MAKE_LOG_TAG('Q', 'I', 'M', 'S')
#define LOG_TAG_SMS           OSA_MAKE_LOG_TAG('Q', 'S', 'M', 'S')
#define LOG_TAG_FTM           OSA_MAKE_LOG_TAG('Q', 'F', 'T', 'M')
#define LOG_TAG_DEMO          OSA_MAKE_LOG_TAG('Q', 'D', 'E', 'M')
#define LOG_TAG_FLASH_TEST    OSA_MAKE_LOG_TAG('Q', 'F', 'T', 'S')
#define LOG_TAG_USB_API       OSA_MAKE_LOG_TAG('Q', 'U', 'S', 'B')
#define LOG_TAG_UART_API      OSA_MAKE_LOG_TAG('Q', 'U', 'A', 'T')
#define LOG_TAG_RTC           OSA_MAKE_LOG_TAG('Q', 'R', 'T', 'C')
#define LOG_TAG_LED           OSA_MAKE_LOG_TAG('Q', 'L', 'E', 'D')
#define LOG_TAG_GPIO_API      OSA_MAKE_LOG_TAG('Q', 'G', 'I', 'O')
#define LOG_TAG_AUDIO_API     OSA_MAKE_LOG_TAG('Q', 'A', 'U', 'D')
#define LOG_TAG_IIC_API       OSA_MAKE_LOG_TAG('Q', 'I', 'I', 'C')
#define LOG_TAG_PWM_API       OSA_MAKE_LOG_TAG('Q', 'P', 'W', 'M')
#define LOG_TAG_SPI_API       OSA_MAKE_LOG_TAG('Q', 'S', 'P', 'I')
#define LOG_TAG_CAMERA_API    OSA_MAKE_LOG_TAG('Q', 'C', 'A', 'M')
#define LOG_TAG_GNSS_API      OSA_MAKE_LOG_TAG('Q', 'G', 'P', 'S')
#define LOG_TAG_MRD_API       OSA_MAKE_LOG_TAG('Q', 'M', 'R', 'D')
#define LOG_TAG_LPM_API       OSA_MAKE_LOG_TAG('Q', 'L', 'P', 'M')
#define LOG_TAG_CI            OSA_MAKE_LOG_TAG('Q', 'U', 'C', 'I')
#define LOG_TAG_SLEEP_API     OSA_MAKE_LOG_TAG('Q', 'S', 'L', 'P')
#define LOG_TAG_FACT_API      OSA_MAKE_LOG_TAG('F', 'A', 'C', 'T')
#define LOG_TAG_ESIM          OSA_MAKE_LOG_TAG('Q', 'E', 'S', 'M')
#define LOG_TAG_QPPP          OSA_MAKE_LOG_TAG('Q', 'P', 'P', 'P')
#define LOG_TAG_LCD_API       OSA_MAKE_LOG_TAG('Q', 'L', 'C', 'D')
#define LOG_TAG_VIRT_AT       OSA_MAKE_LOG_TAG('Q', 'V', 'I', 'R')

/*  6. qosa_log_printf  dual-path
 *    FIRMWARE:          osiTracePrintf((level<<28)|tag, ...)
 *    UNIRTOS_APPS_COMPILE: snprintf + m_get_api_ptr/osa_trace_debug */

#ifdef UNIRTOS_APPS_COMPILE
#define qosa_log_printf(level, tag, funcname, line, fmt, ...) \
    qosa_log_output(((unsigned)(level) << 28) | (unsigned)(tag), \
        "%s %d " fmt, funcname, line, ##__VA_ARGS__)
#else
#define qosa_log_printf(level, tag, funcname, line, fmt, ...) \
    osiTracePrintf(((unsigned)(level) << 28) | (unsigned)(tag), \
        "%s %d " fmt, funcname, line, ##__VA_ARGS__)
#endif

/* 7. QUECOSA_LOG_PRINTF_TAG / QOSA_LOG_PRINTF_TAG  log wrappers */
#define QUECOSA_LOG_PRINTF_TAG(level, tag, funcname, line, ...) \
    do { \
        if (OSA_LOG_LEVEL >= level) \
            qosa_log_printf(level, tag, funcname, line, ##__VA_ARGS__); \
    } while (0)

#define QOSA_LOG_PRINTF_TAG(level, tag, funcname, line, ...) \
    QUECOSA_LOG_PRINTF_TAG(level, tag, funcname, line, ##__VA_ARGS__)

/* 8. QOSA_LOG_* convenience macros */
#define QOSA_LOG_E(tag, ...) QOSA_LOG_PRINTF_TAG(OSA_LOG_LEVEL_ERROR,   tag, __func__, __LINE__, ##__VA_ARGS__)
#define QOSA_LOG_W(tag, ...) QOSA_LOG_PRINTF_TAG(OSA_LOG_LEVEL_WARN,    tag, __func__, __LINE__, ##__VA_ARGS__)
#define QOSA_LOG_I(tag, ...) QOSA_LOG_PRINTF_TAG(OSA_LOG_LEVEL_INFO,    tag, __func__, __LINE__, ##__VA_ARGS__)
#define QOSA_LOG_D(tag, ...) QOSA_LOG_PRINTF_TAG(OSA_LOG_LEVEL_DEBUG,   tag, __func__, __LINE__, ##__VA_ARGS__)
#define QOSA_LOG_V(tag, ...) QOSA_LOG_PRINTF_TAG(OSA_LOG_LEVEL_VERBOSE, tag, __func__, __LINE__, ##__VA_ARGS__)

/* 9. OSA_LOG_* (legacy macros for backward compat) */
#define OSA_LOG_E(tag, ...)   QUECOSA_LOG_PRINTF_TAG(OSA_LOG_LEVEL_ERROR,   tag, __func__, __LINE__, ##__VA_ARGS__)
#define OSA_LOG_W(tag, ...)   QUECOSA_LOG_PRINTF_TAG(OSA_LOG_LEVEL_WARN,    tag, __func__, __LINE__, ##__VA_ARGS__)
#define OSA_LOG_I(tag, ...)   QUECOSA_LOG_PRINTF_TAG(OSA_LOG_LEVEL_INFO,    tag, __func__, __LINE__, ##__VA_ARGS__)
#define OSA_LOG_D(tag, ...)   QUECOSA_LOG_PRINTF_TAG(OSA_LOG_LEVEL_DEBUG,   tag, __func__, __LINE__, ##__VA_ARGS__)
#define OSA_LOG_V(tag, ...)   QUECOSA_LOG_PRINTF_TAG(OSA_LOG_LEVEL_VERBOSE, tag, __func__, __LINE__, ##__VA_ARGS__)
#define OSA_LOG_NO_F(tag, ...) QUECOSA_LOG_PRINTF_TAG(OSA_LOG_LEVEL_VERBOSE, tag, "0", __LINE__, ##__VA_ARGS__)

/* 10. QLOGX short macros (Apps layer) */
#define QLOGE(...)       QOSA_LOG_E(QOS_LOG_TAG, ##__VA_ARGS__)
#define QLOGW(...)       QOSA_LOG_W(QOS_LOG_TAG, ##__VA_ARGS__)
#define QLOGI(...)       QOSA_LOG_I(QOS_LOG_TAG, ##__VA_ARGS__)
#define QLOGD(...)       QOSA_LOG_D(QOS_LOG_TAG, ##__VA_ARGS__)
#define QLOGV(...)       QOSA_LOG_V(QOS_LOG_TAG, ##__VA_ARGS__)
#define QLOGV_EX(...)    OSA_LOG_NO_F(QOS_LOG_TAG, ##__VA_ARGS__)

/* 11. Log control & output API */
extern void       qosa_log_output(unsigned tag, const char *fmt, ...);
qosa_log_error_e  qosa_log_control_set(qosa_uint32_t mask);
qosa_uint32_t     qosa_log_control_get(void);

#endif /* __QOSA_RDA_8910_LOG_H__ */
