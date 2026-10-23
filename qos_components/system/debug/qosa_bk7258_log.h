#ifndef __QOSA_BK7258_LOG_H__
#define __QOSA_BK7258_LOG_H__

#include "qosa_def.h"

// #include "components/log.h"
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

//use usb as default log port
#define QOSA_QT2131_LOG_PORT_DFT      QOSA_LOG_BIT_USB

#define QOSA_LOG_LEVEL                QOSA_LOG_LEVEL_VERBOSE
#define QOSA_MAKE_LOG_TAG(a, b, c, d) ((unsigned)(a) | ((unsigned)(b) << 7) | ((unsigned)(c) << 14) | ((unsigned)(d) << 21))

void qosa_log_printf(int level, int tag, const char* funcname, int line, const char* fmt, ...);

#define QOSA_LOG_PRINTF_TAG(level, tag, funcname, line, ...)                                                                                                   \
    do {                                                                                                                                                       \
        if (QOSA_LOG_LEVEL >= level)                                                                                                                           \
            qosa_log_printf(level, tag, funcname, line, ##__VA_ARGS__);                                                                                        \
    } while (0)




#define LOG_TAG                 QOSA_MAKE_LOG_TAG('Q', 'U', 'E', 'C')
#define LOG_TAG_AT              QOSA_MAKE_LOG_TAG('Q', 'U', 'E', 'A')
#define LOG_TAG_SIO             QOSA_MAKE_LOG_TAG('Q', 'S', 'I', 'O')
#define LOG_TAG_MEMORY          QOSA_MAKE_LOG_TAG('Q', 'D', 'S', 'M')
#define LOG_TAG_MEMORY_QUEUE    QOSA_MAKE_LOG_TAG('Q', 'D', 'S', 'Q')
#define LOG_TAG_LIST_LINK       QOSA_MAKE_LOG_TAG('Q', 'L', 'S', 'T')
#define LOG_TAG_AT_PARSER       QOSA_MAKE_LOG_TAG('Q', 'A', 'T', 'P')
#define LOG_TAG_AT_RESP         QOSA_MAKE_LOG_TAG('Q', 'A', 'T', 'R')
#define LOG_TAG_AT_SIO          QOSA_MAKE_LOG_TAG('Q', 'A', 'T', 'S')
#define LOG_TAG_URC             QOSA_MAKE_LOG_TAG('Q', 'U', 'R', 'C')
#define LOG_TAG_EVENT           QOSA_MAKE_LOG_TAG('Q', 'E', 'V', 'E')
#define LOG_TAG_VFS             QOSA_MAKE_LOG_TAG('Q', 'V', 'F', 'S')
#define LOG_TAG_SFILE           QOSA_MAKE_LOG_TAG('Q', 'S', 'F', 'I')
#define LOG_TAG_FILE_API        QOSA_MAKE_LOG_TAG('Q', 'F', 'A', 'I')
#define LOG_TAG_SOCK_API        QOSA_MAKE_LOG_TAG('Q', 'S', 'O', 'C')
#define LOG_TAG_DNS_API         QOSA_MAKE_LOG_TAG('Q', 'D', 'N', 'S')
#define LOG_TAG_MBED_API        QOSA_MAKE_LOG_TAG('Q', 'M', 'B', 'D')
#define LOG_TAG_TCPIP_API       QOSA_MAKE_LOG_TAG('Q', 'T', 'C', 'P')
#define LOG_TAG_TCPIP_NB_API    QOSA_MAKE_LOG_TAG('Q', 'N', 'B', 'I')
#define LOG_TAG_PDP_API         QOSA_MAKE_LOG_TAG('Q', 'P', 'D', 'P')
#define LOG_TAG_HTTP_API        QOSA_MAKE_LOG_TAG('Q', 'H', 'T', 'P')
#define LOG_TAG_LWM2M_API       QOSA_MAKE_LOG_TAG('Q', 'M', '2', 'M')
#define LOG_TAG_MODEM           QOSA_MAKE_LOG_TAG('Q', 'M', 'D', 'M')
#define LOG_TAG_SMTP_API        QOSA_MAKE_LOG_TAG('S', 'M', 'T', 'P')
#define LOG_TAG_MMS_API         QOSA_MAKE_LOG_TAG('Q', 'M', 'M', 'S')
#define LOG_TAG_BASE_AT         QOSA_MAKE_LOG_TAG('Q', 'B', 'S', 'A')
#define LOG_TAG_POWER_API       QOSA_MAKE_LOG_TAG('Q', 'P', 'O', 'W')
#define LOG_TAG_FTP_API         QOSA_MAKE_LOG_TAG('Q', 'F', 'T', 'P')
#define LOG_TAG_MQTT_API        QOSA_MAKE_LOG_TAG('Q', 'M', 'Q', 'T')
#define LOG_TAG_WEB_API         QOSA_MAKE_LOG_TAG('Q', 'W', 'E', 'B')
#define LOG_TAG_DM_API          QOSA_MAKE_LOG_TAG('Q', 'U', 'D', 'M')
#define LOG_TAG_FOTA_API        QOSA_MAKE_LOG_TAG('Q', 'F', 'O', 'T')
#define LOG_TAG_NTP_API         QOSA_MAKE_LOG_TAG('Q', 'N', 'T', 'P')
#define LOG_TAG_PING_API        QOSA_MAKE_LOG_TAG('Q', 'P', 'I', 'G')
#define LOG_TAG_LBS_API         QOSA_MAKE_LOG_TAG('Q', 'L', 'B', 'S')
#define LOG_TAG_WIFISCAN_API    QOSA_MAKE_LOG_TAG('Q', 'W', 'I', 'S')
#define LOG_TAG_WIFI_API        QOSA_MAKE_LOG_TAG('Q', 'W', 'I', 'F')
#define LOG_TAG_WLAN_API        QOSA_MAKE_LOG_TAG('Q', 'W', 'L', 'N')
#define LOG_TAG_QURL_API        QOSA_MAKE_LOG_TAG('Q', 'U', 'R', 'L')
#define LOG_TAG_COMPONENT       QOSA_MAKE_LOG_TAG('Q', 'C', 'M', 'P')
#define LOG_TAG_MINI_HTTP       QOSA_MAKE_LOG_TAG('Q', 'M', 'H', 'T')
#define LOG_TAG_SBFOTA_API      QOSA_MAKE_LOG_TAG('Q', 'S', 'B', 'F')
#define LOG_TAG_ADC_API         QOSA_MAKE_LOG_TAG('Q', 'A', 'D', 'C')
#define LOG_TAG_BACKUP_API      QOSA_MAKE_LOG_TAG('Q', 'B', 'A', 'C')
#define LOG_TAG_GPIO_API        QOSA_MAKE_LOG_TAG('Q', 'G', 'I', 'O')
#define LOG_TAG_USB_API         QOSA_MAKE_LOG_TAG('Q', 'U', 'S', 'B')
#define LOG_TAG_UART_API        QOSA_MAKE_LOG_TAG('Q', 'U', 'A', 'T')
#define LOG_TAG_RTC             QOSA_MAKE_LOG_TAG('Q', 'R', 'T', 'C')
#define LOG_TAG_LED             QOSA_MAKE_LOG_TAG('Q', 'L', 'E', 'D')
#define LOG_TAG_QPPP            QOSA_MAKE_LOG_TAG('Q', 'P', 'P', 'P')
#define LOG_TAG_SLEEP_API       QOSA_MAKE_LOG_TAG('Q', 'S', 'L', 'P')
#define LOG_TAG_GNSS_API        QOSA_MAKE_LOG_TAG('Q', 'G', 'P', 'S')
#define LOG_TAG_FACT_API        QOSA_MAKE_LOG_TAG('F', 'A', 'C', 'T')
#define LOG_TAG_ADC_API         QOSA_MAKE_LOG_TAG('Q', 'A', 'D', 'C')
#define LOG_TAG_IIC_API         QOSA_MAKE_LOG_TAG('Q', 'I', 'I', 'C')
#define LOG_TAG_PWM_API         QOSA_MAKE_LOG_TAG('Q', 'P', 'W', 'M')
#define LOG_TAG_LPM_API         QOSA_MAKE_LOG_TAG('Q', 'L', 'P', 'M')
#define LOG_TAG_SPI_API         QOSA_MAKE_LOG_TAG('Q', 'S', 'P', 'I')
#define LOG_TAG_COAP_API        QOSA_MAKE_LOG_TAG('Q', 'C', 'O', 'P')
#define LOG_TAG_CAMERA_API      QOSA_MAKE_LOG_TAG('Q', 'C', 'A', 'M')
#define LOG_TAG_MRD_API         QOSA_MAKE_LOG_TAG('Q', 'M', 'R', 'D')
#define LOG_TAG_CI              QOSA_MAKE_LOG_TAG('Q', 'U', 'C', 'I')
#define LOG_TAG_NET_API         QOSA_MAKE_LOG_TAG('Q', 'N', 'E', 'T')
#define LOG_TAG_DATACALL        QOSA_MAKE_LOG_TAG('Q', 'D', 'A', 'T')
#define LOG_TAG_SIM             QOSA_MAKE_LOG_TAG('Q', 'S', 'I', 'M')
#define LOG_TAG_NETWORK         QOSA_MAKE_LOG_TAG('Q', 'N', 'W', 'K')
#define LOG_TAG_USBNET          QOSA_MAKE_LOG_TAG('Q', 'U', 'N', 'E')
#define LOG_TAG_DEV             QOSA_MAKE_LOG_TAG('Q', 'D', 'E', 'V')
#define LOG_TAG_CALL            QOSA_MAKE_LOG_TAG('Q', 'C', 'A', 'L')
#define LOG_TAG_IMS             QOSA_MAKE_LOG_TAG('Q', 'I', 'M', 'S')
#define LOG_TAG_SMS             QOSA_MAKE_LOG_TAG('Q', 'S', 'M', 'S')
#define LOG_TAG_FTM             QOSA_MAKE_LOG_TAG('Q', 'F', 'T', 'M')
#define LOG_TAG_DEMO            QOSA_MAKE_LOG_TAG('Q', 'D', 'E', 'M')
#define LOG_TAG_AUDIO_API       QOSA_MAKE_LOG_TAG('Q', 'A', 'U', 'D')
#define LOG_TAG_FLASH_TEST      QOSA_MAKE_LOG_TAG('Q', 'F', 'T', 'S')

#define QOSA_LOG_E(tag, ...)    QOSA_LOG_PRINTF_TAG(QOSA_LOG_LEVEL_ERROR, tag, __func__, __LINE__, ##__VA_ARGS__)
#define QOSA_LOG_W(tag, ...)    QOSA_LOG_PRINTF_TAG(QOSA_LOG_LEVEL_WARN, tag, __func__, __LINE__, ##__VA_ARGS__)
#define QOSA_LOG_I(tag, ...)    QOSA_LOG_PRINTF_TAG(QOSA_LOG_LEVEL_INFO, tag, __func__, __LINE__, ##__VA_ARGS__)
#define QOSA_LOG_D(tag, ...)    QOSA_LOG_PRINTF_TAG(QOSA_LOG_LEVEL_DEBUG, tag, __func__, __LINE__, ##__VA_ARGS__)
#define QOSA_LOG_V(tag, ...)    QOSA_LOG_PRINTF_TAG(QOSA_LOG_LEVEL_VERBOSE, tag, __func__, __LINE__, ##__VA_ARGS__)
#define QOSA_LOG_NO_F(tag, ...) QOSA_LOG_PRINTF_TAG(QOSA_LOG_LEVEL_VERBOSE, tag, "0", __LINE__, ##__VA_ARGS__)

#define QLOGE(...)              QOSA_LOG_E(QOS_LOG_TAG, ##__VA_ARGS__)//BK_LOGE(NULL, ##__VA_ARGS__)//QOSA_LOG_E(QOS_LOG_TAG, ##__VA_ARGS__)
#define QLOGW(...)              QOSA_LOG_W(QOS_LOG_TAG, ##__VA_ARGS__)//BK_LOGW(NULL, ##__VA_ARGS__)//QOSA_LOG_W(QOS_LOG_TAG, ##__VA_ARGS__)
#define QLOGI(...)              QOSA_LOG_I(QOS_LOG_TAG, ##__VA_ARGS__)//BK_LOGI(NULL, ##__VA_ARGS__)//QOSA_LOG_I(QOS_LOG_TAG, ##__VA_ARGS__)
#define QLOGD(...)              QOSA_LOG_D(QOS_LOG_TAG, ##__VA_ARGS__)//BK_LOGD(NULL, ##__VA_ARGS__)//QOSA_LOG_D(QOS_LOG_TAG, ##__VA_ARGS__)
#define QLOGV(...)              QOSA_LOG_V(QOS_LOG_TAG, ##__VA_ARGS__)//BK_LOGI(NULL, ##__VA_ARGS__)//QOSA_LOG_V(QOS_LOG_TAG, ##__VA_ARGS__)
#define QLOGV_EX(...)           QOSA_LOG_NO_F(QOS_LOG_TAG, ##__VA_ARGS__)//BK_LOGI(NULL, ##__VA_ARGS__)//QOSA_LOG_NO_F(QOS_LOG_TAG, ##__VA_ARGS__)

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


#endif // __QOSA_BK7258_LOG_H__