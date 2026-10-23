/*****************************************************************/ /**
* @file qcm_ping_app.h
* @brief
* @author larson.li@quectel.com
* @date 2024-04-22
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2024-04-22 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#ifndef __QCM_PING_APP_H__
#define __QCM_PING_APP_H__

#include "qosa_def.h"
#include "qosa_sys.h"

/**
 * @enum
 * @brief Ping event results, socket errors and some parameter errors
 */
typedef enum
{
    QCM_PING_OK = 0,
    QCM_PING_SEND_END = 1,
    QCM_PING_ERR_UNKNOW = (QOSA_COMPONENT_PING << 16) | 550,                 /*!< Unknow ERROR   */
    QCM_PING_ERR_INVALID_PARAM = (QOSA_COMPONENT_PING << 16) | 552,          /*!< Invalid parameters   */
    QCM_PING_ERR_MEMORY = (QOSA_COMPONENT_PING << 16) | 553,                 /*!< Memory not enough    */
    QCM_PING_ERR_SOCKET_NEW_FAILURE = (QOSA_COMPONENT_PING << 16) | 554,     /*!< Create socket failed */
    QCM_PING_ERR_SOCKET_BIND_FAILURE = (QOSA_COMPONENT_PING << 16) | 556,    /*!< Socket bind failed   */
    QCM_PING_ERR_SOCKET_WRITE_FAILURE = (QOSA_COMPONENT_PING << 16) | 558,   /*!< Socket write failed   */
    QCM_PING_ERR_SOCKET_READ_FAILURE = (QOSA_COMPONENT_PING << 16) | 559,    /*!< Socket read failed	 */
    QCM_PING_ERR_PDP_ACTIVE_FAILURE = (QOSA_COMPONENT_PING << 16) | 561,     /*!< PDP context opening failed	 */
    QCM_PING_ERR_DNS_FAILURE = (QOSA_COMPONENT_PING << 16) | 565,            /*!< DNS parse failed     */
    QCM_PING_ERR_SOCKET_CONNECT_FAILURE = (QOSA_COMPONENT_PING << 16) | 566, /*!< Socket connect failed   */
    QCM_PING_ERR_TIMEOUT = (QOSA_COMPONENT_PING << 16) | 569,                /*!< Operation timeout    */
    QCM_PING_ERR_OPERATION_NOT_ALLOWED = (QOSA_COMPONENT_PING << 16) | 572,  /*!< Operation not allowed   */
} qcm_ping_error_e;

/**
 * @struct ping_task_msg_t
 * @brief Configuration for sending ping packets
 */
typedef struct
{
    qosa_uint32_t num_data_bytes;         /*!< Data byte size of the ping packet    */
    qosa_uint32_t ping_response_time_out; /*!< Wait time for ping response, ms      */
    qosa_uint32_t num_pings;              /*!< Number of times to ping              */
    qosa_uint32_t ttl;                    /*!< Time To Live for the ping packet     */
} qcm_ping_config_type;

/**
 * @enum qcm_ping_event_type
 * @brief Event types returned by ping command
 */
typedef enum
{
    QCM_PING_STATS = 1,   /*!< Ping execution result status report */
    QCM_PING_SUMMARY = 2, /*!< Summary report after ping command completion */
} qcm_ping_event_type;

/**
 * @struct qcm_ping_stats_type
 * @brief Process status information returned by ping command results
 */
typedef struct
{
    qosa_uint32_t ping_rtt;                                       /*!< Response waiting time after sending Ping request */
    qosa_uint32_t ping_size;                                      /*!< Byte length of sent Ping request */
    qosa_uint32_t ping_ttl;                                       /*!< TTL value of Ping request response packet */
    char          resolved_ip_addr[CONFIG_QOSA_INET6_ADDRSTRLEN]; /*!< Remote server IP address */
} qcm_ping_stats_type;

/**
 * @struct qcm_ping_summary_type
 * @brief Final summary information returned by ping command results
 */
typedef struct
{
    qosa_uint32_t min_rtt;        /*!< Minimum RTT so far, in millisecs     */
    qosa_uint32_t max_rtt;        /*!< Maximum RTT so far, in millisecs     */
    qosa_uint32_t avg_rtt;        /*!< Average RTT so far, in millisecs     */
    qosa_uint32_t num_pkts_sent;  /*!< Number of pings sent so far          */
    qosa_uint32_t num_pkts_recvd; /*!< Number of responses recieved so far  */
    qosa_uint32_t num_pkts_lost;  /*!< Number of responses not received     */
} qcm_ping_summary_type;

/**
 * @struct qcm_ping_resp_t
 * @brief Ping command response data structure
 * @details This structure uses a union to encapsulate two different types of ping response data:
 *          - When event_id is QCM_PING_STATS, use the status member, containing single ping response information
 *          - When event_id is QCM_PING_SUMMARY, use the summary member, containing statistical information after ping command completion
 *
 * @note This structure achieves memory reuse through a union, selecting different data members based on different event types
 */
typedef struct
{
    union
    {
        qcm_ping_summary_type summary; /*!< Summary information after ping command completion, including RTT statistics, packet loss rate, etc. */
        qcm_ping_stats_type   status;  /*!< Single ping response status information, including RTT, TTL, IP address, etc. */
    } type;                            /*!< Union member, select to use summary or status based on event type */
} qcm_ping_resp_t;

typedef void (*qcm_ping_event_cb)(qcm_ping_event_type event_id, qcm_ping_error_e evt_code, qcm_ping_resp_t *resp_ptr, void *user_param);

/**
 * @brief Enable ping functionality, send ping packets according to configuration parameters
 *
 * @param[in] qosa_int32_t cid
 *          - PDP cid information.
 *
 * @param[in] qosa_int32_t sim_id
 *          - SIM ID information
 *
 * @param[in] const char * host
 *          - Peer address for ping operation
 *
 * @param[in] qcm_ping_config_type * ping_options
 *          - Ping request sending data settings, request byte count, timeout time, maximum request count
 *
 * @param[in] qcm_ping_event_cb cb_fcn
 *          - Event callback function for ping operation, set event type to ping start, and handle ping callback results
 *
 * @param[in] void * user_param
 *          - User parameters carried
 *
 * @return qcm_ping_error_e
 *       - Success returns QCM_PING_OK
 *       - Failure returns others
 */
#if (defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
qcm_ping_error_e
qcm_ping_start(const char *host, qcm_ping_config_type *ping_options, qcm_ping_event_cb cb_fcn, void *user_param);
#else
qcm_ping_error_e
qcm_ping_start(qosa_int32_t cid, qosa_int32_t sim_id, const char *host, qcm_ping_config_type *ping_options, qcm_ping_event_cb cb_fcn, void *user_param);
#endif
#endif /* __QCM_PING_APP_H__ */
