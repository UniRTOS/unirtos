/*****************************************************************/ /**
 * @file   qosa_log.h
 * @brief
 * @author larson.li@quectel.com
 * @date   2023-03-23
 *
 * @copyright  Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
 * All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
 *
 * @par EDIT HISTORY FOR MODULE
 * <table>
 * <tr><th>Date       <th>Version    <th>Author          <th>Description"
 * <tr><td>2023-03-23 <td>1.0        <td>Larson.Li       <td> Init
 * </table>
 **********************************************************************/

#ifndef __QOSA_EIGEN_LOG_H__
#define __QOSA_EIGEN_LOG_H__

#include "qosa_def.h"
#include DEBUG_LOG_HEADER_FILE
#include "debug_trace.h"

#define LOG_UART_SUPPORT        0 // 1: support UART LOG printing, 0: unsupport

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

typedef enum
{
    QOSA_LOG_LEVEL_NEVER = 0,
    QOSA_LOG_LEVEL_ERROR,
    QOSA_LOG_LEVEL_WARN,
    QOSA_LOG_LEVEL_INFO,
    QOSA_LOG_LEVEL_DEBUG,
    QOSA_LOG_LEVEL_VERBOSE,

    QOSA_LOG_LEVEL_MAX
} qosa_log_level_e;

/**
  * @brief Log output control: Control different log channel switches through bitmask
  *
  * Bit definitions:
  * - Bit 0 (lowest bit): [Master switch] 0=all closed, 1=allow subsequent channel control to take effect
  * - Bit 1: DEBUG port output         (1 << 1) = 2
  * - Bit 2: USB port output              (1 << 2) = 4
  * - Bit 3: SD card output               (1 << 3) = 8
  * - Bit 4: FLASH output             (1 << 4) = 16
  * - ... Can continue to extend other output targets
  *
  * @note
  * - Must set Bit 0 = 1 for other channel switches to be effective
  * - Call qosa_log_control_set() to set all switches, if return QOSA_LOG_ERRID_NOT_SUPPORT, indicates some channel options are not supported
  * - Supported log channels depend on platform support, can check supported channels by configuring each one by one, USB channel is usually supported
  */

// Bit definitions
#define QOSA_LOG_BIT_MASTER_ENABLE    (1U << 0)  // Bit 0: Master switch
#define QOSA_LOG_BIT_DEBUG            (1U << 1)  // Bit 1: DEBUG port
#define QOSA_LOG_BIT_USB              (1U << 2)  // Bit 2: USB port
#define QOSA_LOG_BIT_SDCARD           (1U << 3)  // Bit 3: SD card
#define QOSA_LOG_BIT_FLASH            (1U << 4)  // Bit 4: FLASH

#define QOSA_LOG_LEVEL                QOSA_LOG_LEVEL_VERBOSE
#define QOSA_MAKE_LOG_TAG(a, b, c, d) ((unsigned)(a) | ((unsigned)(b) << 7) | ((unsigned)(c) << 14) | ((unsigned)(d) << 21))

#if LOG_UART_SUPPORT
extern void qcm_uart_log_printf(int level, int tag, const char* funcname, int line, const char* fmt, ...);
#else
void qosa_log_printf(int level, int tag, const char* funcname, int line, const char* fmt, ...);
#endif

#if LOG_UART_SUPPORT
#define QOSA_LOG_PRINTF_TAG(level, tag, funcname, line, ...)                                                                                                   \
    do {                                                                                                                                                       \
        if (QOSA_LOG_LEVEL >= level){                                                                                                                          \
            qcm_uart_log_printf(level, tag, funcname, line, ##__VA_ARGS__);                                                                                   \
        }                                                                                                                                                      \
    } while (0)
#else
#define QOSA_LOG_PRINTF_TAG(level, tag, funcname, line, ...)                                                                                                   \
    do {                                                                                                                                                       \
        if (QOSA_LOG_LEVEL >= level){                                                                                                                          \
            qosa_log_printf(level, tag, funcname, line, ##__VA_ARGS__);                                                                                        \
        }                                                                                                                                                      \
    } while (0)
#endif


#define LOG_TAG              UNILOG_UNIRTOS_UNIR
#define LOG_TAG_AT           UNILOG_UNIRTOS_QUEA
#define LOG_TAG_SIO          UNILOG_UNIRTOS_QSIO
#define LOG_TAG_MEMORY       UNILOG_UNIRTOS_QDSM
#define LOG_TAG_MEMORY_QUEUE UNILOG_UNIRTOS_QDSQ
#define LOG_TAG_LIST_LINK    UNILOG_UNIRTOS_QLST
#define LOG_TAG_AT_PARSER    UNILOG_UNIRTOS_QATP
#define LOG_TAG_AT_RESP      UNILOG_UNIRTOS_QATR
#define LOG_TAG_AT_SIO       UNILOG_UNIRTOS_QATS
#define LOG_TAG_URC          UNILOG_UNIRTOS_QURC
#define LOG_TAG_EVENT        UNILOG_UNIRTOS_QEVE
#define LOG_TAG_VFS          UNILOG_UNIRTOS_QVFS
#define LOG_TAG_SFILE        UNILOG_UNIRTOS_QSFI
#define LOG_TAG_FILE_API     UNILOG_UNIRTOS_QFAI
#define LOG_TAG_SOCK_API     UNILOG_UNIRTOS_QSOC
#define LOG_TAG_DNS_API      UNILOG_UNIRTOS_QDNS
#define LOG_TAG_MBED_API     UNILOG_UNIRTOS_QMBD
#define LOG_TAG_TCPIP_API    UNILOG_UNIRTOS_QTCP
#define LOG_TAG_TCPIP_NB_API UNILOG_UNIRTOS_QTCP_NB
#define LOG_TAG_PDP_API      UNILOG_UNIRTOS_QPDP
#define LOG_TAG_HTTP_API     UNILOG_UNIRTOS_QHTP
#define LOG_TAG_ANDLINK_API  UNILOG_UNIRTOS_QADL
#define LOG_TAG_LWM2M_API    UNILOG_UNIRTOS_QM2M
#define LOG_TAG_MODEM        UNILOG_UNIRTOS_QMDM
#define LOG_TAG_SMTP_API     UNILOG_UNIRTOS_QMTP
#define LOG_TAG_MMS_API      UNILOG_UNIRTOS_MMS
#define LOG_TAG_BASE_AT      UNILOG_UNIRTOS_QBSA
#define LOG_TAG_POWER_API    UNILOG_UNIRTOS_QPOW
#define LOG_TAG_FTP_API      UNILOG_UNIRTOS_QFTP
#define LOG_TAG_MQTT_API     UNILOG_UNIRTOS_QMQT
#define LOG_TAG_WEB_API      UNILOG_UNIRTOS_QWEB
#define LOG_TAG_DM_API       UNILOG_UNIRTOS_DM
#define LOG_TAG_FOTA_API     UNILOG_UNIRTOS_QFOT
#define LOG_TAG_NTP_API      UNILOG_UNIRTOS_QNTP
#define LOG_TAG_PING_API     UNILOG_UNIRTOS_QPIG
#define LOG_TAG_LBS_API      UNILOG_UNIRTOS_QLBS
#define LOG_TAG_WIFISCAN_API UNILOG_UNIRTOS_QWIS
#define LOG_TAG_WIFI_API     UNILOG_UNIRTOS_QWIF
#define LOG_TAG_QURL_API     UNILOG_UNIRTOS_QURL
#define LOG_TAG_COMPONENT    UNILOG_UNIRTOS_QCMP
#define LOG_TAG_MINI_HTTP    UNILOG_UNIRTOS_QMHT
#define LOG_TAG_SBFOTA_API   UNILOG_UNIRTOS_QSBF
#define LOG_TAG_ADC_API      UNILOG_UNIRTOS_QADC
#define LOG_TAG_BACKUP_API   UNILOG_UNIRTOS_QBAC
#define LOG_TAG_GPIO_API     UNILOG_UNIRTOS_QGIO
#define LOG_TAG_USB_API      UNILOG_UNIRTOS_QUSB
#define LOG_TAG_UART_API     UNILOG_UNIRTOS_QUAT
#define LOG_TAG_RTC          UNILOG_UNIRTOS_QRTC
#define LOG_TAG_LED          UNILOG_UNIRTOS_QLED
#define LOG_TAG_QPPP         UNILOG_UNIRTOS_QPPP
#define LOG_TAG_VSIM         UNILOG_UNIRTOS_VSIM
#define LOG_TAG_SLEEP_API    UNILOG_UNIRTOS_QSLP
#define LOG_TAG_GNSS_API     UNILOG_UNIRTOS_QGPS
#define LOG_TAG_FACT_API     UNILOG_UNIRTOS_FACT
#define LOG_TAG_IIC_API      UNILOG_UNIRTOS_QIIC
#define LOG_TAG_PWM_API      UNILOG_UNIRTOS_QPWM
#define LOG_TAG_LPM_API      UNILOG_UNIRTOS_QLPM
#define LOG_TAG_SPI_API      UNILOG_UNIRTOS_QSPI
#define LOG_TAG_COAP_API     UNILOG_UNIRTOS_QCOP
#define LOG_TAG_CAMERA_API   UNILOG_UNIRTOS_QCAM
#define LOG_TAG_LCD_API      UNILOG_UNIRTOS_QLCD
#define LOG_TAG_MRD_API      UNILOG_UNIRTOS_QMRD
#define LOG_TAG_CI           UNILOG_UNIRTOS_QUCI
#define LOG_TAG_NET_API      UNILOG_UNIRTOS_QNET
#define LOG_TAG_DATACALL     UNILOG_UNIRTOS_QDAT
#define LOG_TAG_SIM          UNILOG_UNIRTOS_QSIM
#define LOG_TAG_NETWORK      UNILOG_UNIRTOS_QNWK
#define LOG_TAG_USBNET       UNILOG_UNIRTOS_QUNE
#define LOG_TAG_DEV          UNILOG_UNIRTOS_QDEV
#define LOG_TAG_CALL         UNILOG_UNIRTOS_QCAL
#define LOG_TAG_IMS          UNILOG_UNIRTOS_QIMS
#define LOG_TAG_SMS          UNILOG_UNIRTOS_QSMS
#define LOG_TAG_FTM          UNILOG_UNIRTOS_QFTM
#define LOG_TAG_DEMO         UNILOG_UNIRTOS_QDEM
#define LOG_TAG_AUDIO_API    UNILOG_UNIRTOS_QAUD
#define LOG_TAG_FLASH_TEST   UNILOG_UNIRTOS_QFTS
#define LOG_TAG_ESIM         UNILOG_UNIRTOS_ESIM
#define LOG_TAG_JAMM_DETECT  UNILOG_UNIRTOS_QJAM
#define LOG_TAG_SECBOOT      UNILOG_UNIRTOS_QSEC
#define LOG_TAG_CMUX         UNILOG_UNIRTOS_CMUX
#define LOG_TAG_FIREWALL_API UNILOG_UNIRTOS_QFWL
#define LOG_TAG_NETDEV      UNILOG_UNIRTOS_QNDV
#define LOG_TAG_AGORA_RTC    UNILOG_UNIRTOS_ARTC

#ifdef UNIRTOS_APPS_COMPILE

#define QOSA_LOG_E(tag, ...)    QOSA_LOG_PRINTF_TAG(QOSA_LOG_LEVEL_ERROR, tag, __func__, __LINE__, ##__VA_ARGS__)
#define QOSA_LOG_W(tag, ...)    QOSA_LOG_PRINTF_TAG(QOSA_LOG_LEVEL_WARN, tag, __func__, __LINE__, ##__VA_ARGS__)
#define QOSA_LOG_I(tag, ...)    QOSA_LOG_PRINTF_TAG(QOSA_LOG_LEVEL_INFO, tag, __func__, __LINE__, ##__VA_ARGS__)
#define QOSA_LOG_D(tag, ...)    QOSA_LOG_PRINTF_TAG(QOSA_LOG_LEVEL_DEBUG, tag, __func__, __LINE__, ##__VA_ARGS__)
#define QOSA_LOG_V(tag, ...)    QOSA_LOG_PRINTF_TAG(QOSA_LOG_LEVEL_VERBOSE, tag, __func__, __LINE__, ##__VA_ARGS__)
#define QOSA_LOG_NO_F(tag, ...) QOSA_LOG_PRINTF_TAG(QOSA_LOG_LEVEL_VERBOSE, tag, "0", __LINE__, ##__VA_ARGS__)

#else /* UNIRTOS_APPS_COMPILE */

#define QOSA_LOG_NO_F(tag, ...) QOSA_LOG_PRINTF_TAG(QOSA_LOG_LEVEL_VERBOSE, tag, "0", __LINE__, ##__VA_ARGS__)

#define QOSA_LOG_E(tag, format, ...)                                                                                                                           \
    do {                                                                                                                                                       \
        swLogPrintf(UNILOG_UNIQUE_ID(UNILOG_CUSTOMER, tag), P_INFO, ##__VA_ARGS__);                                                                            \
        {                                                                                                                                                      \
            (void)format;                                                                                                                                      \
        }                                                                                                                                                      \
    } while (0)

#define QOSA_LOG_W(tag, format, ...)                                                                                                                           \
    do {                                                                                                                                                       \
        swLogPrintf(UNILOG_UNIQUE_ID(UNILOG_CUSTOMER, tag), P_INFO, ##__VA_ARGS__);                                                                            \
        {                                                                                                                                                      \
            (void)format;                                                                                                                                      \
        }                                                                                                                                                      \
    } while (0)

#define QOSA_LOG_I(tag, format, ...)                                                                                                                           \
    do {                                                                                                                                                       \
        swLogPrintf(UNILOG_UNIQUE_ID(UNILOG_CUSTOMER, tag), P_INFO, ##__VA_ARGS__);                                                                            \
        {                                                                                                                                                      \
            (void)format;                                                                                                                                      \
        }                                                                                                                                                      \
    } while (0)

#define QOSA_LOG_D(tag, format, ...)                                                                                                                           \
    do {                                                                                                                                                       \
        swLogPrintf(UNILOG_UNIQUE_ID(UNILOG_CUSTOMER, tag), P_INFO, ##__VA_ARGS__);                                                                            \
        {                                                                                                                                                      \
            (void)format;                                                                                                                                      \
        }                                                                                                                                                      \
    } while (0)

#define QOSA_LOG_V(tag, format, ...)                                                                                                                           \
    do {                                                                                                                                                       \
        swLogPrintf(UNILOG_UNIQUE_ID(UNILOG_CUSTOMER, tag), P_INFO, ##__VA_ARGS__);                                                                            \
        {                                                                                                                                                      \
            (void)format;                                                                                                                                      \
        }                                                                                                                                                      \
    } while (0)

#endif /* UNIRTOS_APPS_COMPILE */

#define QLOGE(...)    QOSA_LOG_E(QOS_LOG_TAG, ##__VA_ARGS__)
#define QLOGW(...)    QOSA_LOG_W(QOS_LOG_TAG, ##__VA_ARGS__)
#define QLOGI(...)    QOSA_LOG_I(QOS_LOG_TAG, ##__VA_ARGS__)
#define QLOGD(...)    QOSA_LOG_D(QOS_LOG_TAG, ##__VA_ARGS__)
#define QLOGV(...)    QOSA_LOG_V(QOS_LOG_TAG, ##__VA_ARGS__)
#define QLOGV_EX(...) QOSA_LOG_NO_F(QOS_LOG_TAG, ##__VA_ARGS__)

/**
  * @brief Set log control bitmask
  *
  * @param[in] qosa_uint32_t mask
  *           - Log master switch and individual switches
  *
  * @return qosa_log_error_e
  *        - Return QOSA_LOG_ERRID_SUCCESS if successful
  *        - Return corresponding error code if failed
  */
qosa_log_error_e qosa_log_control_set(qosa_uint32_t mask);

/**
  * @brief Get log control bitmask
  *
  * @return qosa_uint32_t
  *           - Return bitmask,
  */
qosa_uint32_t qosa_log_control_get(void);

#endif /*__QOSA_EIGEN_LOG_H__ */
