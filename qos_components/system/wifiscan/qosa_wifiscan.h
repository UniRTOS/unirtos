/*****************************************************************/ /**
* @file qosa_wifiscan.h
* @brief WiFi scan interface for UniRTOS
* @author lambert.Zhao@quectel.com
* @date 2024-08-08
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2024-08-08 <td>1.0 <td>Lambert.Zhao <td> Init
* </table>
**********************************************************************/
#ifndef __QOSA_WIFISCAN_H__
#define __QOSA_WIFISCAN_H__

#include "qosa_def.h"
#include "qosa_sys.h"

/** Minimum value for longest total scan time for wifiscan */
#define QOSA_WIFI_SCAN_TOTAL_TIME_VAL_MIN         (4000)
/** Maximum value for longest total scan time for wifiscan */
#define QOSA_WIFI_SCAN_TOTAL_TIME_VAL_MAX         (255000)
/** Default value for longest total scan time for wifiscan */
#define QOSA_WIFI_SCAN_TOTAL_TIME_VAL_DEF         (12000)

/** Minimum value for number of scan cycles for wifiscan */
#define QOSA_WIFI_SCAN_ROUND_MIN                  (1)
/** Maximum value for number of scan cycles for wifiscan */
#define QOSA_WIFI_SCAN_ROUND_MAX                  (3)
/** Default value for number of scan cycles for wifiscan */
#define QOSA_WIFI_SCAN_ROUND_DEF                  (1)

#ifdef CONFIG_QOSA_QT2131_PLATFORM_FUNC
/** Minimum value for maximum number of APs scanned by wifiscan */
#define QOSA_WIFI_SCAN_AP_CNT_MIN                 (5)
#else
/** Minimum value for maximum number of APs scanned by wifiscan */
#define QOSA_WIFI_SCAN_AP_CNT_MIN                 (4)
#endif //CONFIG_QOSA_QT2131_PLATFORM_FUNC
/** Maximum value for maximum number of APs scanned by wifiscan */
#define QOSA_WIFI_SCAN_AP_CNT_MAX                 (30)
/** Default value for maximum number of APs scanned by wifiscan */
#define QOSA_WIFI_SCAN_AP_CNT_DEF                 (5)

/** Minimum value for longest scan time per cycle for wifiscan */
#define QOSA_WIFI_SCAN_EACH_ROUND_TIMEOUT_VAL_MIN (1)
/** Maximum value for longest scan time per cycle for wifiscan */
#define QOSA_WIFI_SCAN_EACH_ROUND_TIMEOUT_VAL_MAX (255)
/** Default value for longest scan time per cycle for wifiscan */
#define QOSA_WIFI_SCAN_EACH_ROUND_TIMEOUT_VAL_DEF (5)

/** Minimum value for longest scan time per channel per cycle for wifiscan */
#define QOSA_WIFI_SCAN_TIME_MIN                   (120)
/** Maximum value for longest scan time per channel per cycle for wifiscan */
#define QOSA_WIFI_SCAN_TIME_MAX                   (5000)
/** Default value for longest scan time per channel per cycle for wifiscan */
#define QOSA_WIFI_SCAN_TIME_DEF                   (600)

/** Data priority mode */
#define QOSA_WIFI_SCAN_PRIORITY_VAL_MIN           (0)
/** wifiscan priority mode */
#define QOSA_WIFI_SCAN_PRIORITY_VAL_MAX           (1)
/** Default is data priority mode */
#define QOSA_WIFI_SCAN_PRIORITY_VAL_DEF           (0)

/**
 * @enum qosa_wifiscan_error_e
 * @brief Error codes for WIFISCAN function component
 */
typedef enum
{
    QOSA_WIFISCAN_SUCCESS = 0,                                              /*!< Execution successful */
    QOSA_WIFISCAN_EXECUTE_ERR = (QOSA_COMPONENT_WIFISCAN << 16) | 1,        /*!< Execution failed */
    QOSA_WIFISCAN_MEM_ADDR_NULL_ERR = (QOSA_COMPONENT_WIFISCAN << 16) | 2,  /*!< Memory allocation failed */
    QOSA_WIFISCAN_INVALID_PARAM_ERR = (QOSA_COMPONENT_WIFISCAN << 16) | 3,  /*!< Invalid parameter */
    QOSA_WIFISCAN_SEMAPHORE_WAIT_ERR = (QOSA_COMPONENT_WIFISCAN << 16) | 4, /*!< Semaphore wait exception */
    QOSA_WIFISCAN_MUTEX_TIMEOUT_ERR = (QOSA_COMPONENT_WIFISCAN << 16) | 5,  /*!< Mutex acquisition exception */
    QOSA_WIFISCAN_OPEN_FAIL = (QOSA_COMPONENT_WIFISCAN << 16) | 6,          /*!< WIFISCAN open exception */
    QOSA_WIFISCAN_BUSY_ERR = (QOSA_COMPONENT_WIFISCAN << 16) | 7,           /*!< WIFISCAN busy, such as during scanning */
    QOSA_WIFISCAN_ALREADY_OPEN_ERR = (QOSA_COMPONENT_WIFISCAN << 16) | 8,   /*!< WIFISCAN duplicate open attempt error */
    QOSA_WIFISCAN_NOT_OPEN_ERR = (QOSA_COMPONENT_WIFISCAN << 16) | 9,       /*!< WIFISCAN not open */
    QOSA_WIFISCAN_HW_OCCUPIED_ERR = (QOSA_COMPONENT_WIFISCAN << 16) | 10,   /*!< Hardware occupied by other functions, such as Bluetooth */
    QOSA_WIFISCAN_NO_SET_CB_ERR = (QOSA_COMPONENT_WIFISCAN << 16) | 11,     /*!< Callback function not configured */
} qosa_wifiscan_error_e;

/**
 * @enum qosa_wifiscan_channel_e
 * @brief Enumeration of wifiscan channels 1~13
 */
typedef enum
{
    QOSA_WIFISCAN_CHANNEL_ALL_BIT = 0x1FFF,
    QOSA_WIFISCAN_CHANNEL_ONE = 0x0001,      /*!< Channel 1 */
    QOSA_WIFISCAN_CHANNEL_TWO = 0x0002,      /*!< Channel 2 */
    QOSA_WIFISCAN_CHANNEL_THREE = 0x0004,    /*!< Channel 3 */
    QOSA_WIFISCAN_CHANNEL_FOUR = 0x0008,     /*!< Channel 4 */
    QOSA_WIFISCAN_CHANNEL_FIVE = 0x0010,     /*!< Channel 5 */
    QOSA_WIFISCAN_CHANNEL_SIX = 0x0020,      /*!< Channel 6 */
    QOSA_WIFISCAN_CHANNEL_SEVEN = 0x0040,    /*!< Channel 7 */
    QOSA_WIFISCAN_CHANNEL_EIGHT = 0x0080,    /*!< Channel 8 */
    QOSA_WIFISCAN_CHANNEL_NINE = 0x0100,     /*!< Channel 9 */
    QOSA_WIFISCAN_CHANNEL_TEN = 0x0200,      /*!< Channel 10 */
    QOSA_WIFISCAN_CHANNEL_ELEVEN = 0x0400,   /*!< Channel 11 */
    QOSA_WIFISCAN_CHANNEL_TWELVE = 0x0800,   /*!< Channel 12 */
    QOSA_WIFISCAN_CHANNEL_THIRTEEN = 0x1000, /*!< Channel 13 */
} qosa_wifiscan_channel_e;

/**
 * @struct qosa_wifi_ap_info_t
 * @brief Specific information of each WIFI AP scanned by wifiscan
 */
typedef struct
{
    qosa_uint8_t bssid[6]; /*!< WIFI AP MAC address */
    qosa_uint8_t channel;  /*!< Channel it is on */
    qosa_int8_t  rssival;  /*!< Signal strength, unit dBm */
    qosa_uint8_t ssid_len; /*!< SSID length*/
    qosa_uint8_t ssid[33]; /*!< WIFI AP SSID name */
    char         reserve;
} qosa_wifi_ap_info_t;

/**
 * @struct qosa_wifiscan_config_t
 * @brief wifiscan configuration
  *
 */
typedef struct
{
    qosa_uint16_t           max_ap_cnt; /*!< Maximum number of APs scanned by wifiscan */
    qosa_wifiscan_channel_e channel;    /*!< wifiscan scan channel, 1 bit represents 1 channel*/
    qosa_uint8_t            scan_round; /*!< Number of wifiscan scan cycles */
    qosa_uint32_t           ch_time;    /*!< Longest scan time per channel per cycle for wifiscan, unit: ms */

    qosa_uint32_t max_timeout;          /*!< Maximum scan time for one wifiscan scan request, unit: ms */
    qosa_uint32_t scan_timeout;         /*!< Maximum timeout time for each round of wifiscan scan, unit: s */
    qosa_uint8_t  wifi_priority;        /*!< wifiscan priority 0-DATA_PREFERRED  1-WIFI_PREFERRED */
} qosa_wifiscan_config_t;

/**
 * @brief Callback function registered by WIFISCAN function
 *
 * @param[in] void *user_data
 *          - Asynchronous callback user data
 *
 * @param[in] qosa_wifiscan_error_e result
 *          - Result return code corresponding to wifiscan scan
 *
 * @param[in] qosa_uint32_t ap_cnt
 *          - Number of APs scanned by wifiscan
 *
 * @param[in] qosa_wifi_ap_info_t *ap_infos
 *          - Information of all APs scanned by wifiscan
 *
 */
typedef void (*qosa_wifiscan_callback)(void *user_data, qosa_wifiscan_error_e result, qosa_uint32_t ap_cnt, qosa_wifi_ap_info_t *ap_infos);

/**
 * @brief This function is used to open WIFISCAN
 *
 * @return qosa_wifiscan_error_e
 *       - Return QOSA_WIFISCAN_SUCCESS if successful
 *       - Return other values if failed
 */
qosa_wifiscan_error_e qosa_wifiscan_open(void);

/**
 * @brief This function is used to close WIFISCAN
 *
 * @return qosa_wifiscan_error_e
 *       - Return QOSA_WIFISCAN_SUCCESS if successful
 *       - Return other values if failed
 */
qosa_wifiscan_error_e qosa_wifiscan_close(void);

/**
 * @brief This function is used to perform WIFISCAN synchronous mode scan
 *
 * @param[in,out] qosa_uint16_t *p_ap_cnt
 *              - Number of APs scanned
 *
 * @param[in,out] qosa_wifi_ap_info_t *p_ap_infos
 *              - Information of all scanned APs
 *
 * @return qosa_wifiscan_error_e
 *       - Return QOSA_WIFISCAN_SUCCESS if successful
 *       - Return other values if failed
 */
qosa_wifiscan_error_e qosa_wifiscan_do(qosa_uint16_t *p_ap_cnt, qosa_wifi_ap_info_t *p_ap_infos);

/**
 * @brief This function is used to start WIFISCAN asynchronous mode scan
 *
 * @return qosa_wifiscan_error_e
 *       - Return QOSA_WIFISCAN_SUCCESS if successful
 *       - Return other values if failed
 */
qosa_wifiscan_error_e qosa_wifiscan_async(void);

/**
 * @brief This function is used to configure WIFISCAN scan parameters
 *
 * @param[in] qosa_wifiscan_config_t *wifiscan_config
 *          - wifiscan parameter configuration
 *
 * @return qosa_wifiscan_error_e
 *       - Return QOSA_WIFISCAN_SUCCESS if successful
 *       - Return other values if failed
 */
qosa_wifiscan_error_e qosa_wifiscan_option_set(qosa_wifiscan_config_t *wifiscan_config);

/**
 * @brief This function is used to get wifiscan configuration parameters
 *
 * @param[out] qosa_wifiscan_config_t *wifiscan_config
 *          - wifiscan parameter pointer
 *
 * @return qosa_wifiscan_error_e
 *       - Return QOSA_WIFISCAN_SUCCESS if successful
 *       - Return other values if failed
 */
qosa_wifiscan_error_e qosa_wifiscan_get_config(qosa_wifiscan_config_t *wifiscan_config);

/**
 * @brief This function is used to register callback function
 *
 * @param[in] qosa_wifiscan_callback wifiscan_cb
 *          - Callback function pointer
 *
 * @param[in] void *user_data
 *          - Asynchronous callback user data
 *
 * @return qosa_wifiscan_error_e
 *       - Return QOSA_WIFISCAN_SUCCESS if successful
 *       - Return other values if failed
 */
qosa_wifiscan_error_e qosa_wifiscan_register_cb(qosa_wifiscan_callback wifiscan_cb, void *user_data);

#endif /* __QOSA_WIFISCAN_H__ */
