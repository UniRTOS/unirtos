/*****************************************************************/ /**
* @file qcm_ntp_app.h
* @brief
* @author larson.li@quectel.com
* @date 2024-04-24
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2024-04-24 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#ifndef __QCM_NTP_APP_H__
#define __QCM_NTP_APP_H__

#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_rtc.h"

// NTP default maximum retry interval time in seconds
#define QCM_NTP_DEFAULT_RETRY_TIMES 15
// NTP default maximum retry count
#define QCM_NTP_DEFAULT_RETRY_CNT   3

typedef int qcm_ntp_client_id;

/**
 * @brief Configuration parameters supported by users when using NTP to get time
 */
typedef struct
{
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
    qosa_uint8_t  sim_id;            /*!< Corresponding SIM card ID sequence number */
    qosa_uint8_t  pdp_id;            /*!< Corresponding PDP activation scenario sequence number */
#endif
    qosa_uint32_t num_data_bytes;    /*!< Length of NTP sending data packet */
    qosa_bool_t   sync_local_time;   /*!< Whether to automatically set the synchronized time as local time */
    qosa_uint8_t  retry_cnt;         /*!< NTP data sending retry count */
    qosa_uint16_t retry_interval_tm; /*!< NTP data sending retry interval time */
} qcm_ntp_config_t;

typedef enum
{
    QCM_NTP_ERR_WOULDBLOCK = -2,
    QCM_NTP_ERR_NEGATIVE = -1,
    QCM_NTP_SUCCESS = 0,
    QCM_NTP_ERR_UNKNOW = (QOSA_COMPONENT_NTP << 16) | 550 /*!< Unknow ERROR   */,
    QCM_NTP_ERR_OPERATION_BLOCK = (QOSA_COMPONENT_NTP << 16) | 551 /*!< Operation blocked   */,
    QCM_NTP_ERR_INVALID_PARAM = (QOSA_COMPONENT_NTP << 16) | 552 /*!< Invalid parameters   */,
    QCM_NTP_ERR_MEMORY = (QOSA_COMPONENT_NTP << 16) | 553 /*!<  Memory not enough   */,
    QCM_NTP_ERR_SOCKET_NEW_FAILURE = (QOSA_COMPONENT_NTP << 16) | 554 /*!< Create socket failed */,
    QCM_NTP_ERR_SOCKET_BIND_FAILURE = (QOSA_COMPONENT_NTP << 16) | 556 /*!< Socket bind failed   */,
    QCM_NTP_ERR_SOCKET_WRITE_FAILURE = (QOSA_COMPONENT_NTP << 16) | 558 /*!< Socket write failed   */,
    QCM_NTP_ERR_SOCKET_READ_FAILURE = (QOSA_COMPONENT_NTP << 16) | 559 /*!< Socket read failed	 */,
    QCM_NTP_ERR_PDP_ACTIVE_FAILURE = (QOSA_COMPONENT_NTP << 16) | 561 /*!< PDP context opening failed	 */,
    QCM_NTP_ERR_DNS_FAILURE = (QOSA_COMPONENT_NTP << 16) | 565 /*!< DNS parse failed     */,
    QCM_NTP_ERR_SOCKET_CONNECT_FAILURE = (QOSA_COMPONENT_NTP << 16) | 566 /*!< Socket connect failed   */,
    QCM_NTP_ERR_TIMEOUT = (QOSA_COMPONENT_NTP << 16) | 569 /*!< Operation timeout    */,
} qcm_ntp_result_code;

typedef void (*qcm_ntp_sync_result_cb)(qcm_ntp_client_id client_id, qcm_ntp_result_code result, qosa_rtc_time_t *sync_time, void *arg);

/**
 * @brief Initialize NTP request and get the corresponding NTP operation handle
 *
 * Note: Resources will be automatically released after NTP callback notification, cannot be managed
 *
 * @return qcm_ntp_client_id
 *       - Success returns client id greater than 0
 *       - Failure returns -1
 */
qcm_ntp_client_id qcm_ntp_client_new(void);

/**
 * @brief NTP asynchronous request action API interface
 *
 * @param[in] qcm_ntp_client_id client_id
 *          - Corresponding NTP client id obtained from qcm_ntp_client_new function
 *
 * @param[in] const char * host_name
 *          - Corresponding host domain name or IP address
 *
 * @param[in] qosa_uint16_t host_port
 *          - Corresponding NTP server port, generally defaults to 123
 *
 * @param[in] qcm_ntp_config_type * ping_options
 *          - Corresponding user configured parameters
 *
 * @param[in] qcm_ntp_sync_result_cb cb_fun
 *          - Corresponding result return callback function
 *
 * @param[in] void * user_param
 *          - User carried parameters
 *
 * @return qcm_ntp_result_code
 *       - If successful, return QCM_NTP_SUCCESS
 *       - If failed, return other values
 */
qcm_ntp_result_code qcm_ntp_sync_start(
    qcm_ntp_client_id      client_id,
    const char            *host_name,
    qosa_uint16_t          host_port,
    qcm_ntp_config_t      *ntp_options,
    qcm_ntp_sync_result_cb cb_fun,
    void                  *user_param
);

/**
 * @brief NTP release function, actually automatically executed after NTP completion
 * @note Here for the convenience of API pairing
 */
qcm_ntp_result_code qcm_ntp_client_free(qcm_ntp_client_id client_id);

#endif /* __QCM_NTP_APP_H__ */
