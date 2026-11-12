/*****************************************************************/ /**
* @file qosa_wifi_rf_test.h
* @brief WiFi RF factory test interface
* @author joe.tu@quectel.com
* @date 2026-06-16
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description
* <tr><td>2026-06-16 <td>1.0 <td>joe.tu <td> Init
* </table>
**********************************************************************/
#ifndef __QOSA_WIFI_RF_TEST_H__
#define __QOSA_WIFI_RF_TEST_H__

#include "qosa_def.h"
#include "qosa_sys.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @enum qosa_wifi_rf_test_err_e
 * @brief WiFi RF factory test error code.
 */
typedef enum
{
    QOSA_WIFI_RF_TEST_SUCCESS = 0,                                      /*!< Execution successful */
    QOSA_WIFI_RF_TEST_ERR_GENERAL = (QOSA_COMPONENT_FTM << 16) | 0x100, /*!< Execution failed */
    QOSA_WIFI_RF_TEST_ERR_INVALID_PARAM,                                /*!< Invalid parameter */
    QOSA_WIFI_RF_TEST_ERR_NOT_SUPPORTED,                                /*!< Operation is not supported */
} qosa_wifi_rf_test_err_e;

/**
 * @enum qosa_wifi_rf_phy_e
 * @brief WiFi RF TX PHY mode.
 */
typedef enum
{
    QOSA_WIFI_RF_PHY_11B = 0,           /*!< 802.11b/DSSS */
    QOSA_WIFI_RF_PHY_11G = 1,           /*!< 802.11g legacy */
    QOSA_WIFI_RF_PHY_11N = 2,           /*!< 802.11n HT */
    QOSA_WIFI_RF_PHY_11AX_HE_SU = 3,    /*!< 802.11ax HE-SU */
    QOSA_WIFI_RF_PHY_11AC_VHT = 4,      /*!< 802.11ac VHT */
    QOSA_WIFI_RF_PHY_11AX_HE_ER_SU = 5, /*!< 802.11ax HE-ER-SU */
} qosa_wifi_rf_phy_e;

/**
 * @enum qosa_wifi_rf_bw_e
 * @brief WiFi RF channel bandwidth.
 */
typedef enum
{
    QOSA_WIFI_RF_BW_20M = 0, /*!< 20 MHz */
    QOSA_WIFI_RF_BW_40M = 1, /*!< 40 MHz */
} qosa_wifi_rf_bw_e;

/**
 * @struct qosa_wifi_rf_tx_param_t
 * @brief WiFi RF TX test parameters.
 */
typedef struct
{
    qosa_int32_t       channel;  /*!< Channel, normally 1~14 or 36~165 */
    qosa_wifi_rf_phy_e phy_mode; /*!< PHY mode */
    qosa_int32_t       rate_idx; /*!< Rate index, interpreted by PHY mode */
    qosa_wifi_rf_bw_e  bw;       /*!< Channel bandwidth */
} qosa_wifi_rf_tx_param_t;

/**
 * @struct qosa_wifi_rf_rx_param_t
 * @brief WiFi RF RX test parameters.
 */
typedef struct
{
    qosa_int32_t      channel; /*!< Channel, normally 1~14 or 36~165 */
    qosa_wifi_rf_bw_e bw;      /*!< Channel bandwidth */
} qosa_wifi_rf_rx_param_t;

/**
 * @struct qosa_wifi_rf_rx_result_t
 * @brief WiFi RF RX test result.
 */
typedef struct
{
    qosa_int32_t ok_packets;    /*!< Successfully received packet count */
    qosa_int32_t error_packets; /*!< Failed or error packet count */
    qosa_int32_t rssi;          /*!< RSSI value reported by driver */
} qosa_wifi_rf_rx_result_t;

/**
 * @brief Enable WiFi RF factory test mode.
 *
 * @return qosa_wifi_rf_test_err_e
 *       - QOSA_WIFI_RF_TEST_SUCCESS if successful.
 *       - Error code otherwise.
 */
qosa_wifi_rf_test_err_e qosa_wifi_rf_test_enable(void);

/**
 * @brief Disable WiFi RF factory test mode.
 *
 * @return qosa_wifi_rf_test_err_e
 *       - QOSA_WIFI_RF_TEST_SUCCESS if successful.
 *       - Error code otherwise.
 */
qosa_wifi_rf_test_err_e qosa_wifi_rf_test_disable(void);

/**
 * @brief Start WiFi RF TX test.
 *
 * @param[in] tx_param
 *          - TX test parameters. The caller owns the memory and may release it after this function returns.
 *
 * @return qosa_wifi_rf_test_err_e
 *       - QOSA_WIFI_RF_TEST_SUCCESS if successful.
 *       - Error code otherwise.
 */
qosa_wifi_rf_test_err_e qosa_wifi_rf_test_start_tx(qosa_wifi_rf_tx_param_t *tx_param);

/**
 * @brief Stop WiFi RF TX test.
 *
 * @return qosa_wifi_rf_test_err_e
 *       - QOSA_WIFI_RF_TEST_SUCCESS if successful.
 *       - Error code otherwise.
 */
qosa_wifi_rf_test_err_e qosa_wifi_rf_test_stop_tx(void);

/**
 * @brief Start WiFi RF RX test.
 *
 * @param[in] rx_param
 *          - RX test parameters. The caller owns the memory and may release it after this function returns.
 *
 * @return qosa_wifi_rf_test_err_e
 *       - QOSA_WIFI_RF_TEST_SUCCESS if successful.
 *       - Error code otherwise.
 */
qosa_wifi_rf_test_err_e qosa_wifi_rf_test_start_rx(qosa_wifi_rf_rx_param_t *rx_param);

/**
 * @brief Stop WiFi RF RX test and get RX result.
 *
 * @param[out] rx_result
 *          - RX result buffer owned by caller.
 *
 * @return qosa_wifi_rf_test_err_e
 *       - QOSA_WIFI_RF_TEST_SUCCESS if successful.
 *       - Error code otherwise.
 */
qosa_wifi_rf_test_err_e qosa_wifi_rf_test_stop_rx(qosa_wifi_rf_rx_result_t *rx_result);

#ifdef __cplusplus
}
#endif

#endif /* __QOSA_WIFI_RF_TEST_H__ */
