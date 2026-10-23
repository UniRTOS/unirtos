/*****************************************************************/ /**
* @file qcm_ntp_app.c
* @brief NTP Client Implementation File
* @details This file implements the Network Time Protocol (NTP) client functionality, including:
*          - NTP Message Processing
*          - PDP network activation
*          - DNS domain name resolution
*          - NTP packet transmission and reception
*          - Time Synchronization Calculation
*          - Error handling and retry mechanisms
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
#include "qcm_ntp_app.h"
#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_log.h"
#include "qosa_rtc.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
#include "qosa_datacall.h"
#endif
#include "qosa_asyn_dns.h"
#include "qcm_socket_adp.h"

/**NTP TASK Priority*/
#define QCM_NTP_TASK_PRIORITY     QOSA_PRIORITY_NORMAL
/*Maximum capacity of NTP TASK message queue*/
#define QCM_NTP_MSGQ_MAX_CNT      30
/*Maximum number of NTP handles*/
#define QCM_NTP_MAX_SESSION_CNT   3
#define QCM_NTP_OFFSET_NUM        1
#define QCM_NTP_SESSION_ARRAY_CNT QCM_NTP_MAX_SESSION_CNT

/*Send NTP packet size*/
#define QCM_NTP_PACKET_SIZE       48

/*since 1900 - 1970  in seconds*/
#define QCM_JAN_1970              0x83aa7e80
/*Convert a floating-point number to an NTP format time fraction
* The NTP timestamp is a 64-bit unsigned fixed-point number, where the first 32 bits are the integer part and the last 32 bits are the fractional part.
* The fractional part has a precision of 2^-32, allowing an NTP timestamp's seconds to be accurate to 233 decimal places.
*/
#define QCM_NTPFRAC(x)            (4294 * (x) + ((1981 * (x)) >> 11))

/*In contrast to the above, if you want to set microseconds, it needs to set the clock (clock_settime) based on the incoming time in NTP format.
* Basically accurate
*/
#define QCM_USEC(x)               (((x) >> 12) - 759 * ((((x) >> 10) + 32768) >> 16))

/*Convert NTP delay and dispersion, unit s
* Shorten to 65536ms. RFC-1305 specifies time in seconds.
*/
#define QCM_SEC2U(x)              ((x)*15.2587890625)

#define QOS_LOG_TAG               LOG_TAG_NTP_API

typedef enum
{
    QCM_NTP_SEND_REQUEST = 1000,    /*!< Send NTP request action*/
    QCM_NTP_RECV_REPLY = 1001,      /*!< Received downlink return from NTP server*/
    QCM_NTP_RETRY_TIMER_EXP = 1002, /*!< Retry timeout*/
    QCM_NTP_USER_STOP = 1003,       /*!< User actively stops the action*/
    QCM_NTP_SOCKET_CLOSE = 1004,    /*!< Socket close event*/
    QCM_NTP_PDP_ACTIVE_REQ = 1005,  /*!< PDP starts requesting PDP activation*/
    QCM_NTP_DNS_PARSE_REQ = 1006,   /*!< Start the DNS resolution action*/
    QCM_NTP_PDP_RESP = 1007,        /*!< PDP activation response returned*/
    QCM_NTP_DNS_RESP = 1008,        /*!< DNS resolution returned*/
} qcm_ntp_msg_e;

/**
 * @brief ntp internal message passing transmission
 */
typedef struct
{
    qcm_ntp_msg_e msgid;
    void         *argv;
} qcm_ntp_task_msg_t;

/**
 * @brief Only save ntp client
 */
typedef struct
{
    qcm_ntp_client_id client_id;
} qcm_ntp_client_info;

/**
 * @brief NTP Time Structure
 * @details Used to represent the 64-bit timestamp in the NTP protocol, divided into coarse-grained seconds and fine-grained fractional parts
 */
struct qcm_ntp_time
{
    qosa_uint32_t coarse; /*!< Coarse-grained timestamp (seconds part), number of seconds since January 1, 1900*/
    qosa_uint32_t fine;   /*!< Fine-grained timestamp (fractional part), with a precision of 2^-32 seconds*/
};

#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   

/**
 * @brief NTP PDP activation returns session notification
 */
typedef struct
{
    qcm_ntp_client_id       client_id; /*!< NTP session operation ID*/
    qosa_datacall_act_cnf_t act_info;  /*!< PDP activation operation response content*/
} qcm_ntp_pdp_resp_t;
#endif

/**
 * @brief NTP DNS result returned
 */
typedef struct
{
    qcm_ntp_client_id       client_id; /*!< NTP session operation ID*/
    qosa_dns_error_e        status;    /*!< Corresponds to the DNS resolution operation return status*/
    struct qosa_addrinfo_s *info;      /*!< Corresponding DNS resolution content*/
} qcm_ntp_dns_resp_t;

/**
 * @brief ntp internal management status
 */
typedef struct qcm_ntp_client
{
    qcm_ntp_client_id      client_id;          /*!< Corresponding client handle ID*/
    qosa_int32_t           socket_hndl;        /*!< socket ID*/
    qosa_uint16_t          host_port;          /*!< NTP server port, default is 123*/
    qcm_ntp_config_t       user_opts;          /*!< User-configured user options*/
    struct qcm_ntp_time    time_of_send;       /*!< Record the NTP request sending time for subsequent time calculation and verification*/
    qosa_timer_t           retry_timer;        /*!< Internal retry timer*/
    qcm_ntp_sync_result_cb cb_fun, notify_fcn; /*!< User registration callback function*/
    void                  *user_arg;           /*!< User-provided parameters*/
    char                  *host_name;          /*!< Corresponding server domain name*/
    qosa_ip_addr_t         remote_ip;          /*!< Remote IP address*/
    qosa_uint8_t           ai_family;          /*!< Address family, IPV4/IPV6*/
    qosa_task_t            ntp_task;           /*!< Each NTP request creates a unique task*/
    qosa_msgq_t            ntp_msgq;           /*!< Message queue corresponding to the task*/
    qcm_ntp_result_code    final_result;       /*!< NTP completion result*/
    qosa_rtc_time_t        ntp_rt_time;        /*!< Record the result of successful NTP execution*/
} qcm_ntp_client_t;

static qosa_mutex_t      g_qcm_ntp_mutex = QOSA_NULL;
static qcm_ntp_client_t *g_qcm_ntp_mgr[QCM_NTP_SESSION_ARRAY_CNT] = {QOSA_NULL};

static qcm_ntp_result_code qcm_ntp_free_session(qcm_ntp_client_t *ntp_ptr);

/**
 * @brief Get the current system time and convert it to NTP time format
 * @details Convert system time to the 64-bit timestamp format used by the NTP protocol
 *          NTP time starts from January 1, 1900, while UNIX time starts from January 1, 1970
 *          An offset of 70 years in seconds (QCM_JAN_1970) needs to be added.
 *
 * @param[out] time_coarse Coarse-grained timestamp (seconds part)
 * @param[out] time_fine Fine-grained timestamp (fractional part, precision 2^-32 seconds)
 */
static void qcm_ntp_gettime(qosa_uint32_t *time_coarse, qosa_uint32_t *time_fine)
{
    qosa_time_info_t now = {0, 0};
    qosa_get_system_time(&now);
    *time_coarse = now.seconds + QCM_JAN_1970;  /*Convert to NTP time reference (1900)*/
    *time_fine = QCM_NTPFRAC(now.microseconds); /*Convert microseconds to NTP fractional format*/
}

static qcm_ntp_client_t *qcm_ntp_get_ptr(qcm_ntp_client_id client_id)
{
    if (client_id < QCM_NTP_OFFSET_NUM)
    {
        QLOGE("param error");
        return QOSA_NULL;
    }
    if(client_id - QCM_NTP_OFFSET_NUM >= QCM_NTP_SESSION_ARRAY_CNT)
    {
        QLOGE("param error");
        return QOSA_NULL;
    }
    if (g_qcm_ntp_mgr[client_id - QCM_NTP_OFFSET_NUM] == QOSA_NULL)
    {
        QLOGE("get NULL");
        return QOSA_NULL;
    }
    return g_qcm_ntp_mgr[client_id - QCM_NTP_OFFSET_NUM];
}

static qcm_ntp_client_t *qcm_ntp_socket_get_ptr(qosa_int32_t sock_hd)
{
    qosa_int32_t id = 0;
    for (id = 0; id < QCM_NTP_SESSION_ARRAY_CNT; id++)
    {
        if ((g_qcm_ntp_mgr[id] != QOSA_NULL) && (g_qcm_ntp_mgr[id]->socket_hndl == sock_hd))
        {
            break;
        }
    }

    if (id < QCM_NTP_SESSION_ARRAY_CNT)
    {
        return g_qcm_ntp_mgr[id];
    }
    else
    {
        return QOSA_NULL;
    }
}

/**
 * @brief Used to send messages to the ntp main task message queue, guiding the next action to be taken.
 *
 * @param[in] qosa_int32_t msgid
 * @param[in] qcm_ntp_client_t * ntp_ptr
 *          - - ntp handle
 *
 * @return qcm_ntp_result_code
 */
static qcm_ntp_result_code qcm_ntp_req_event(qcm_ntp_msg_e msgid, qcm_ntp_client_t *ntp_ptr)
{
    qcm_ntp_task_msg_t   ntp_msg = {0};
    qcm_ntp_client_info *send_ptr = QOSA_NULL;
    int ret = QCM_NTP_SUCCESS;
    send_ptr = (qcm_ntp_client_info *)qosa_malloc(sizeof(qcm_ntp_client_info));
    if (send_ptr == QOSA_NULL)
    {
        return QCM_NTP_ERR_MEMORY;
    }
    send_ptr->client_id = ntp_ptr->client_id;

    ntp_msg.msgid = msgid;
    ntp_msg.argv = send_ptr;
    QLOGD("ntp msgid = %d send_ptr->client_id=%d", msgid, send_ptr->client_id);
    if (ntp_ptr->ntp_msgq != QOSA_NULL)
    {
        ret = qosa_msgq_release(ntp_ptr->ntp_msgq, sizeof(qcm_ntp_task_msg_t), (qosa_uint8_t *)&ntp_msg, QOSA_NO_WAIT);
        if (ret != QCM_NTP_SUCCESS)
        {
            QLOGE("ret = %d", ret);
            qosa_free(send_ptr);
            ret = (ret == QOSA_ERROR_MSGQ_INVALID_ERR) ? QCM_NTP_ERR_INVALID_PARAM : QCM_NTP_ERR_UNKNOW;
            return ret;
        }
    }
    else
    {
        qosa_free(send_ptr);
        return QCM_NTP_ERR_INVALID_PARAM;
    }
    return QCM_NTP_SUCCESS;
}

/**
 * @brief retry timeout timer
 */
static void qcm_ntp_retry_timer_callback(void *ctx)
{
    qcm_ntp_client_t *ntp_ptr = (qcm_ntp_client_t *)ctx;

    //Send a timer processing event to the time processing function
    qcm_ntp_req_event(QCM_NTP_RETRY_TIMER_EXP, ntp_ptr);
}
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   

/**
 * @brief PDP activation callback function reports
 */
static void qcm_ntp_pdp_acitve_response_cb(void *user_argv, qosa_datacall_act_cnf_t *act_info)
{
    qcm_ntp_task_msg_t   ntp_msg = {0};
    qcm_ntp_pdp_resp_t  *pdp_resp_ptr = QOSA_NULL;
    qcm_ntp_client_t    *ntp_ptr = QOSA_NULL;
    qcm_ntp_client_info *send_ptr = QOSA_NULL;
    int ret = QCM_NTP_SUCCESS;
    send_ptr = (qcm_ntp_client_info *)user_argv;
    ntp_ptr = qcm_ntp_get_ptr(send_ptr->client_id);
    if (ntp_ptr == QOSA_NULL)
    {
        goto exit;
    }

    pdp_resp_ptr = (qcm_ntp_pdp_resp_t *)qosa_malloc(sizeof(qcm_ntp_pdp_resp_t));
    if (pdp_resp_ptr == QOSA_NULL)
    {
        goto exit;
    }
    qosa_memcpy(&pdp_resp_ptr->act_info, act_info, sizeof(qosa_datacall_act_cnf_t));
    pdp_resp_ptr->client_id = send_ptr->client_id;

    ntp_msg.msgid = QCM_NTP_PDP_RESP;
    ntp_msg.argv = (void *)pdp_resp_ptr;
    ret = qosa_msgq_release(ntp_ptr->ntp_msgq, sizeof(qcm_ntp_task_msg_t), (qosa_uint8_t *)&ntp_msg, QOSA_NO_WAIT);
    if (ret != QCM_NTP_SUCCESS)
    {
        qosa_free(pdp_resp_ptr);
    }
exit:
    if (send_ptr != QOSA_NULL)
    {
        qosa_free(send_ptr);
    }
}

/**
 * @brief PDP activation request action
 */
static qcm_ntp_result_code qcm_ntp_service_data_call(void *argv)
{
    qcm_ntp_result_code   result = QCM_NTP_SUCCESS;
    qosa_datacall_errno_e datacall_ret = QOSA_DATACALL_OK;
    qcm_ntp_client_t     *ntp_ptr = QOSA_NULL;
    qcm_ntp_client_info  *send_ptr = QOSA_NULL;
    qcm_ntp_client_info  *req_ptr = QOSA_NULL;
    qosa_datacall_conn_t  conn = 0;

    send_ptr = (qcm_ntp_client_info *)argv;
    ntp_ptr = qcm_ntp_get_ptr(send_ptr->client_id);
    qosa_free(send_ptr);
    if (ntp_ptr == QOSA_NULL)
    {
        result = QCM_NTP_ERR_INVALID_PARAM;
        return result;
    }

    //Check module network registration status
    if (qosa_datacall_wait_attached(ntp_ptr->user_opts.sim_id, 0) == QOSA_FALSE)
    {
        QLOGE("network no register");
        result = QCM_NTP_ERR_PDP_ACTIVE_FAILURE;
        return result;
    }

    //Check SIM PDPID activation status
    conn = qosa_datacall_conn_new(ntp_ptr->user_opts.sim_id, ntp_ptr->user_opts.pdp_id, QOSA_DATACALL_CONN_TCPIP);
    if (qosa_datacall_get_status(conn) == QOSA_TRUE)
    {
        //If activation is already successful, proceed directly to DNS resolution.
        result = qcm_ntp_req_event(QCM_NTP_DNS_PARSE_REQ, ntp_ptr);
        if (result != QCM_NTP_SUCCESS)
        {
            result = QCM_NTP_ERR_PDP_ACTIVE_FAILURE;  //Report error, exit
        }
        return result;
    }
    else
    {
        req_ptr = (qcm_ntp_client_info *)qosa_malloc(sizeof(qcm_ntp_client_info));
        if (req_ptr == QOSA_NULL)
        {
            result = QCM_NTP_ERR_MEMORY;
            return result;
        }
        req_ptr->client_id = ntp_ptr->client_id;

        //Start executing activation action
        datacall_ret = qosa_datacall_start_async(conn, ntp_ptr->user_opts.retry_interval_tm, (datacall_callback_cb_ptr)qcm_ntp_pdp_acitve_response_cb, req_ptr);
        if (datacall_ret != QOSA_DATACALL_OK)
        {
            qosa_free(req_ptr);
            result = QCM_NTP_ERR_PDP_ACTIVE_FAILURE;
            return result;
        }
        //Waiting for activation to complete
        return result;
    }
}

/**
 * @brief Processing PDP activation result
 */
static qcm_ntp_result_code qcm_ntp_datacall_response_handler(void *argv)
{
    qcm_ntp_result_code result = QCM_NTP_SUCCESS;
    qcm_ntp_pdp_resp_t *pdp_resp_ptr = (qcm_ntp_pdp_resp_t *)argv;
    qcm_ntp_client_t   *ntp_ptr = QOSA_NULL;

    ntp_ptr = qcm_ntp_get_ptr(pdp_resp_ptr->client_id);
    if (ntp_ptr == QOSA_NULL)
    {
        result = QCM_NTP_ERR_INVALID_PARAM;
        goto exit;
    }

    if (pdp_resp_ptr->act_info.opt == QOSA_PDP_OPT_ACTIVE)
    {
        if (pdp_resp_ptr->act_info.status == QOSA_TRUE)
        {
            //Activation successful, proceed to the next step of DNS resolution.
            qcm_ntp_req_event(QCM_NTP_DNS_PARSE_REQ, ntp_ptr);
        }
        else
        {
            //Activation failed, return directly
            result = QCM_NTP_ERR_PDP_ACTIVE_FAILURE;
        }
    }
exit:
    qosa_free(pdp_resp_ptr);
    return result;
}
#endif
/**
 * @brief DNS resolution callback, upon successful DNS resolution, passes the content and proceeds to the next request action.
 */
static void qcm_ntp_dns_result_cb(void *argv, struct qosa_addrinfo_s **info, qosa_dns_error_e status)
{
    qcm_ntp_task_msg_t   ntp_msg = {0};
    qcm_ntp_dns_resp_t  *dns_ptr = QOSA_NULL;
    qcm_ntp_client_t    *ntp_ptr = QOSA_NULL;
    qcm_ntp_client_info *send_ptr = QOSA_NULL;
    int ret = QCM_NTP_SUCCESS;
    send_ptr = (qcm_ntp_client_info *)argv;
    ntp_ptr = qcm_ntp_get_ptr(send_ptr->client_id);
    qosa_free(send_ptr);
    if (ntp_ptr == QOSA_NULL)
    {
        goto exit;
    }

    dns_ptr = (qcm_ntp_dns_resp_t *)qosa_malloc(sizeof(qcm_ntp_dns_resp_t));
    if (dns_ptr == QOSA_NULL)
    {
        QLOGE("malloc faild");
        goto exit;
    }

    dns_ptr->client_id = ntp_ptr->client_id;
    dns_ptr->status = status;
    if (status != QOSA_DNS_RESULT_OK)
    {
        dns_ptr->info = QOSA_NULL;
    }
    else
    {
        dns_ptr->info = *info;
    }

    ntp_msg.msgid = QCM_NTP_DNS_RESP;
    ntp_msg.argv = (void *)dns_ptr;
    QLOGD("msgid = %d ", QCM_NTP_DNS_RESP);
    ret = qosa_msgq_release(ntp_ptr->ntp_msgq, sizeof(qcm_ntp_task_msg_t), (qosa_uint8_t *)&ntp_msg, QOSA_NO_WAIT);
    if (ret == QCM_NTP_SUCCESS)
    {
        return;
    }
exit:
    if (dns_ptr != QOSA_NULL)
    {
        dns_ptr->info = QOSA_NULL;
        qosa_free(dns_ptr);
    }
    if ((status == QOSA_DNS_RESULT_OK) && (*info != QOSA_NULL))
    {
        qosa_dns_result_free(*info);
    }
}

/**
 * @brief DNS Resolution Main Function
 */
static qcm_ntp_result_code qcm_ntp_service_dns_parse(void *argv)
{
    qcm_ntp_result_code     result = QCM_NTP_SUCCESS;
    qosa_dns_error_e        dns_ret = QOSA_DNS_RESULT_OK;
    struct qosa_addrinfo_s  addrinfo_list = {0};
    qcm_ntp_client_t       *ntp_ptr = QOSA_NULL;
    qcm_ntp_client_info    *send_ptr = QOSA_NULL;
    qcm_ntp_client_info    *req_ptr = QOSA_NULL;
    int                     ip_type = 0;
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
    qosa_datacall_ip_info_t info = {0};
    qosa_datacall_conn_t    conn = 0;
    qosa_datacall_errno_e   ret = QOSA_DATACALL_OK;
#endif
    send_ptr = (qcm_ntp_client_info *)argv;
    ntp_ptr = qcm_ntp_get_ptr(send_ptr->client_id);
    qosa_free(send_ptr);
    if (ntp_ptr == QOSA_NULL)
    {
        result = QCM_NTP_ERR_DNS_FAILURE;
        goto exit;
    }

#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
    //Get the datacall IP type to determine whether it is IPv4 or IPv6.
    conn = qosa_datacall_conn_new(ntp_ptr->user_opts.sim_id, ntp_ptr->user_opts.pdp_id, QOSA_DATACALL_CONN_TCPIP);
    ret = qosa_datacall_get_ip_info(conn, &info);
    if (ret == QOSA_DATACALL_ERR_NO_ACTIVE)
    {
        //Activation failed, return directly
        result = QCM_NTP_ERR_PDP_ACTIVE_FAILURE;
        goto exit;
    }

    QLOGD("host_name :%s ip_type=%d", ntp_ptr->host_name, info.ip_type);
#endif
    //Get whether the user input is of IP type
    ip_type = qcm_socket_checkip_is_ip46(ntp_ptr->host_name);
    if (ip_type == QCM_AF_INET || ip_type == QCM_AF_INET6)
    {
        //If it is an IP type and matches the datacall IP type, or is a V4V6 type, proceed directly to the next request action.
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
         if (ip_type == QCM_AF_INET && (info.ip_type == QOSA_PDP_IPV4 || info.ip_type == QOSA_PDP_IPV4V6))
#else
         if (ip_type == QCM_AF_INET)
#endif
         {
            inet_pton(QCM_AF_INET, ntp_ptr->host_name, &ntp_ptr->remote_ip.addr.ipv4_addr);
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
             ntp_ptr->remote_ip.ip_vsn = QOSA_PDP_IPV4;
#else
            ntp_ptr->remote_ip.ip_vsn = QOSA_IP_V4;
#endif
        }
        //If it is an IPV6 type and matches the datacall IP type, or is a V4V6 type, proceed directly to the next request action.
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
        else if (ip_type == QCM_AF_INET6 && (info.ip_type == QOSA_PDP_IPV6 || info.ip_type == QOSA_PDP_IPV4V6))        
#else
        else if (ip_type == QCM_AF_INET6)
#endif
        {
            inet_pton(QCM_AF_INET6, ntp_ptr->host_name, &ntp_ptr->remote_ip.addr.ipv6_addr);
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
            ntp_ptr->remote_ip.ip_vsn = QOSA_PDP_IPV6;
#else
            ntp_ptr->remote_ip.ip_vsn = QOSA_IP_V6;
#endif
        }
        else
        {
            //If it is an IP type and does not match the datacall IP type, directly return an error.
            result = QCM_NTP_ERR_OPERATION_BLOCK;
            goto exit;
        }
        ntp_ptr->ai_family = ip_type;
        QLOGD("ip_type = %d", ip_type);
        QLOGD("ai_family = %d", ntp_ptr->ai_family);
        //Proceed directly to the next request action.
        qcm_ntp_req_event(QCM_NTP_SEND_REQUEST, ntp_ptr);
    }
    else
    {
        //If it is not an IP type, perform DNS resolution.
        req_ptr = (qcm_ntp_client_info *)qosa_malloc(sizeof(qcm_ntp_client_info));
        if (req_ptr == QOSA_NULL)
        {
            result = QCM_NTP_ERR_MEMORY;
            goto exit;
        }
        req_ptr->client_id = ntp_ptr->client_id;

        //DNS resolution success callback or direct return
        dns_ret = qosa_dns_asyn_getaddrinfo(
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
            ntp_ptr->user_opts.sim_id,
            ntp_ptr->user_opts.pdp_id,
#endif
            ntp_ptr->host_name,
            &addrinfo_list,
            qcm_ntp_dns_result_cb,
            (void *)req_ptr
        );
    }

    if (dns_ret != QOSA_DNS_RESULT_OK)
    {
        qosa_free(req_ptr);
        result = QCM_NTP_ERR_DNS_FAILURE;  //Report error
    }

exit:
    return result;
}

/**
 * @brief Process DNS resolution results
 */
static qcm_ntp_result_code qcm_ntp_dns_response_handler(void *argv)
{
    qcm_ntp_result_code result = QCM_NTP_SUCCESS;
    qcm_ntp_dns_resp_t *dns_ptr = (qcm_ntp_dns_resp_t *)argv;
    qcm_ntp_client_t   *ntp_ptr = QOSA_NULL;

    ntp_ptr = qcm_ntp_get_ptr(dns_ptr->client_id);
    if (ntp_ptr == QOSA_NULL)
    {
        result = QCM_NTP_ERR_INVALID_PARAM;
        goto exit;
    }

    if (dns_ptr->info == QOSA_NULL || dns_ptr->status != QOSA_DNS_RESULT_OK)
    {
        result = QCM_NTP_ERR_DNS_FAILURE;
    }
    else
    {
        //DNS resolution successful
        if (dns_ptr->info->ai_family == QCM_AF_INET)
        {
            inet_pton(QCM_AF_INET, dns_ptr->info->ip_addr, &ntp_ptr->remote_ip.addr.ipv4_addr);
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
             ntp_ptr->remote_ip.ip_vsn = QOSA_PDP_IPV4;
#else
            ntp_ptr->remote_ip.ip_vsn = QOSA_IP_V4;
#endif
        }
        else
        {
            inet_pton(QCM_AF_INET6, dns_ptr->info->ip_addr, &ntp_ptr->remote_ip.addr.ipv6_addr);
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
             ntp_ptr->remote_ip.ip_vsn = QOSA_PDP_IPV6;
#else
            ntp_ptr->remote_ip.ip_vsn = QOSA_IP_V6;
#endif
        }
        ntp_ptr->ai_family = dns_ptr->info->ai_family;

        QLOGD("dns_ptr->info->ai_family=%d ntp_ptr->ai_family = %d ntp_ptr=%p", dns_ptr->info->ai_family, ntp_ptr->ai_family, ntp_ptr);

        qosa_dns_result_free(dns_ptr->info);
        qcm_ntp_req_event(QCM_NTP_SEND_REQUEST, ntp_ptr);
    }

exit:
    qosa_free(dns_ptr);

    return result;
}

static void qcm_ntp_socket_event_cb(int sock_hd, int event, int code, void *user_data)
{
    qcm_ntp_client_t *ntp_ptr = QOSA_NULL;
    QOSA_UNUSED(user_data); /*user_data points to ntp_session*/
    QLOGD("enter ntp event cb");
    ntp_ptr = qcm_ntp_socket_get_ptr(sock_hd);
    if (ntp_ptr == QOSA_NULL || ntp_ptr != user_data)
    {
        return;
    }
    QLOGD("evt:%d,%d,%x", sock_hd, event, code);

    if (event & QCM_SOCK_READ_EVENT)
    {
        qcm_ntp_req_event(QCM_NTP_RECV_REPLY, (void *)ntp_ptr);
    }
    if (event & QCM_SOCK_WRITE_EVENT)
    {
        qcm_ntp_req_event(QCM_NTP_SEND_REQUEST, (void *)ntp_ptr);
    }
    if (event & QCM_SOCK_CLOSE_EVENT)
    {
        qcm_ntp_req_event(QCM_NTP_SOCKET_CLOSE, (void *)ntp_ptr);
    }
}

/**
 * @brief NTP packet assembly and transmission function, used for NTP self-packet assembly, socket creation and connection, and sending
 * @param[in] ntp_client_t *      ntp_session           ntp handle
 * @return    qosa_int32_t         Result code               Refer to qcm_ntp_result_code
 */
static qcm_ntp_result_code qcm_ntp_send_sync_request(void *argv)
{
    int                  ret = 0;
    qosa_int32_t         data[12] = {0};
    qcm_ntp_result_code  result = QCM_NTP_SUCCESS;
    qcm_ntp_client_t    *ntp_ptr = QOSA_NULL;
    qcm_ntp_client_info *send_ptr = QOSA_NULL;

    send_ptr = (qcm_ntp_client_info *)argv;
    ntp_ptr = qcm_ntp_get_ptr(send_ptr->client_id);
    qosa_free(send_ptr);
    if (ntp_ptr == QOSA_NULL)
    {
        return QCM_NTP_ERR_UNKNOW;
    }

/**When the LI value is 11, it indicates an alarm state, and the clock is not synchronized; for other values, NTP itself does not process it. 2 bits*/
#define LI      0
/**Version number 3bits*/
#define VN      3
/**The MODE field indicates the NTP operating mode, where 3 represents a client request to the server, and the server responds with this field set to 4, 3 bits.*/
#define MODE    3
/**Time Source Hierarchy       8bits*/
#define STRATUM 0
/**Polling interval, the time interval between two NTP packets 8bits*/
#define POLL    4
/**System clock accuracy     8bits*/
#define PREC    -6

    if (ntp_ptr->socket_hndl == 0)
    {
        QLOGD("sock_hd :%d ntp_ptr=%p ntp_ptr->ai_family=%d", ntp_ptr->socket_hndl, ntp_ptr, ntp_ptr->ai_family);

#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
        ntp_ptr->socket_hndl
            = qcm_socket_create(ntp_ptr->user_opts.sim_id, ntp_ptr->user_opts.pdp_id, ntp_ptr->ai_family, QCM_SOCK_DGRAM, QCM_UDP_PROTOCOL, 0, QOSA_FALSE);
#else
        ntp_ptr->socket_hndl = qcm_socket_create(ntp_ptr->ai_family, QCM_SOCK_DGRAM, QCM_UDP_PROTOCOL, 0, QOSA_FALSE);

#endif
        if (ntp_ptr->socket_hndl < 0)
        {
            return QCM_NTP_ERR_SOCKET_NEW_FAILURE;
        }

        qcm_socket_register_event(
            ntp_ptr->socket_hndl,
            QCM_SOCK_CLOSE_EVENT | QCM_SOCK_READ_EVENT | QCM_SOCK_WRITE_EVENT,
            qcm_ntp_socket_event_cb,
            (void *)ntp_ptr
        );

        QLOGD("CONNECT port :%d", ntp_ptr->host_port);
        if (qcm_socket_connect(ntp_ptr->socket_hndl, &ntp_ptr->remote_ip, ntp_ptr->host_port) != 0)
        {
            QLOGE("qcm_socket_connect failed");
            return QCM_NTP_ERR_SOCKET_CONNECT_FAILURE;
        }
    }

    qosa_memset(data, 0, sizeof data);
    data[0] = htonl((LI << 30) | (VN << 27) | (MODE << 24) | (STRATUM << 16) | (POLL << 8) | (PREC & 0xff));
    data[1] = htonl(1 << 16); /*Root Delay (seconds) Round-trip time from the local device to the primary reference clock source*/
    data[2] = htonl(1 << 16); /*Root Dispersion (seconds) The maximum error of the system clock relative to the primary reference clock*/

    //Save the sending time to the local structure; later, compare it with the server data to check if the times match!!!
    qcm_ntp_gettime(&ntp_ptr->time_of_send.coarse, &ntp_ptr->time_of_send.fine);
    data[10] = htonl(ntp_ptr->time_of_send.coarse); /* Transmit Timestamp coarse */
    data[11] = htonl(ntp_ptr->time_of_send.fine);   /* Transmit Timestamp fine   */

    QLOGD("sendto  port :%d", ntp_ptr->host_port);
    ret = qcm_socket_sendto(ntp_ptr->socket_hndl, &ntp_ptr->remote_ip, ntp_ptr->host_port, (char *)data, QCM_NTP_PACKET_SIZE);
    if (ret > 0 && ret == QCM_NTP_PACKET_SIZE)
    {
        qosa_bool_t start = qosa_timer_is_running(ntp_ptr->retry_timer);
        if (start)
        {
            qosa_timer_stop(ntp_ptr->retry_timer);
        }
        //Start a one-shot timer
        ret = qosa_timer_start(ntp_ptr->retry_timer, ntp_ptr->user_opts.retry_interval_tm * 1000, 0);
        if (ret != 0)
        {
            QLOGE("timer start failed");
            result=QCM_NTP_ERR_UNKNOW;
            goto exit;
        }
    }
    else
    {
        result = QCM_NTP_ERR_SOCKET_WRITE_FAILURE;
        goto exit;
    }

exit:
    return result;
}

/**
 * @brief Calculate the difference between two NTP timestamps (in microseconds)
 * @details Calculate the time difference between two 64-bit NTP timestamps, returning the time difference with microsecond precision.
 *
 * @param[in] start Start timestamp
 * @param[in] stop End timestamp
 * @return double time_difference (microseconds)
 */
static double qcm_ntp_diff(struct qcm_ntp_time *start, struct qcm_ntp_time *stop)
{
    qosa_int32_t  a; /*Coarse-grained time difference (seconds)*/
    qosa_uint32_t b; /*Fine-grained time difference (fractional part)*/

    /*Calculate the difference in the seconds part*/
    a = stop->coarse - start->coarse;

    /*Process the difference of the fractional part, considering the borrow situation.*/
    if (stop->fine >= start->fine)
    {
        /*Normal case: The fractional part of the end time is greater than or equal to the start time.*/
        b = stop->fine - start->fine;
    }
    else
    {
        /*Borrowing is required when the fractional part of the end time is less than that of the start time.*/
        b = start->fine - stop->fine;
        b = ~b; /*Take the complement to obtain the two's complement representation*/
        a -= 1; /*Subtract 1 from the seconds part*/
    }

    /*Convert the result to microseconds: seconds part * 1e6 + fractional part * (1e6 / 2^32)*/
    return a * 1.e6 + b * (1.e6 / 4294967296.0);
}

/**
 * @brief Processing NTP Server Response
 * @details Parse the NTP server's returned data packet and calculate the time synchronization result
 *          Using the four timestamps (T1-T4) of the NTP protocol to calculate network delay and time offset
 *          T1: Client send time (stored in time_of_send)
 *          T2: Server Receive Time (obtained from the response packet)
 *          T3: Server transmission time (obtained from the response packet)
 *          T4: Client Receive Time (Current Time)
 *
 * @param[in] argv Message parameters containing the client ID
 * @return qcm_ntp_result_code Processing Result
 */
static qcm_ntp_result_code qcm_ntp_recv_sync_reply(void *argv)
{
    qosa_uint32_t       data[12] = {0};                              /*NTP packet buffer*/
    qosa_uint32_t       li, vn, mode, stratum, poll, prec;           /*NTP Packet Header Fields*/
    qosa_uint32_t       delay, disp, refid;                          /*Other fields of the NTP packet*/
    struct qcm_ntp_time arrival, reftime, orgtime, rectime, xmttime; /*Each timestamp*/
    double              el_time, st_time, skew1, skew2;              /*Time calculation intermediate result*/
    qosa_uint32_t       freq = 0;                                    /*Frequency Offset*/
    qcm_ntp_result_code result = 0;
    int                 ret = 0;
    qosa_time_t         rt = 0; /*Converted UNIX timestamp*/

    qcm_ntp_client_t    *ntp_ptr = QOSA_NULL;
    qcm_ntp_client_info *send_ptr = QOSA_NULL;

    send_ptr = (qcm_ntp_client_info *)argv;
    ntp_ptr = qcm_ntp_get_ptr(send_ptr->client_id);
    qosa_free(send_ptr);
    if (ntp_ptr == QOSA_NULL)
    {
        return QCM_NTP_ERR_UNKNOW;
    }

    QLOGD("enter ntp reply");

    //Receive NTP response packet
    ret = qcm_socket_recvfrom(ntp_ptr->socket_hndl, QOSA_NULL, QOSA_NULL, (char *)data, QCM_NTP_PACKET_SIZE);
    if (ret == QCM_NTP_PACKET_SIZE)
    {
        //Stop retry timer
        if (qosa_timer_is_running(ntp_ptr->retry_timer))
            qosa_timer_stop(ntp_ptr->retry_timer);

            //Parsing NTP packet header fields
#define Data(i) ntohl(((qosa_uint32_t *)data)[i])
        li = Data(0) >> 30 & 0x03;      /* Leap Indicator */
        vn = Data(0) >> 27 & 0x07;      /* Version Number */
        mode = Data(0) >> 24 & 0x07;    /* Mode */
        stratum = Data(0) >> 16 & 0xff; /* Stratum */
        poll = Data(0) >> 8 & 0xff;     /* Poll Interval */
        prec = Data(0) & 0xff;          /* Precision */
        if (prec & 0x80)
            prec |= 0xffffff00;         /*Processing signed precision values*/
        delay = Data(1);                /* Root Delay */
        disp = Data(2);                 /* Root Dispersion */
        refid = Data(3);                /* Reference ID */

        //Parse timestamp field
        reftime.coarse = Data(4);  /* Reference Timestamp */
        reftime.fine = Data(5);
        orgtime.coarse = Data(6);  /* Originate Timestamp (T1) */
        orgtime.fine = Data(7);
        rectime.coarse = Data(8);  /* Receive Timestamp (T2) */
        rectime.fine = Data(9);
        xmttime.coarse = Data(10); /* Transmit Timestamp (T3) */
        xmttime.fine = Data(11);
#undef Data

        //Get the current time as the arrival time T4
        qcm_ntp_gettime(&arrival.coarse, &arrival.fine);

        //Calculate various time differences
        el_time = qcm_ntp_diff(&orgtime, &arrival); /*T4 - T1: Total time elapsed*/
        st_time = qcm_ntp_diff(&rectime, &xmttime); /*T3 - T2: Server processing time*/
        skew1 = qcm_ntp_diff(&orgtime, &rectime);   /*T2 - T1: Client-to-server latency*/
        skew2 = qcm_ntp_diff(&xmttime, &arrival);   /*T4 - T3: Server to client latency*/

        //Record detailed debugging information
        QLOGD("LI=%d  VN=%d  Mode=%d  Stratum=%d  Poll=%d  Precision=%d\n", li, vn, mode, stratum, poll, prec);
        QLOGV(
            //The current compiler does not support formatted printing with specified precision.
            "Delay=%f  Dispersion=%f  Refid=%u.%u.%u.%u\n",
            QCM_SEC2U(delay),
            QCM_SEC2U(disp),
            refid >> 24 & 0xff,
            refid >> 16 & 0xff,
            refid >> 8 & 0xff,
            refid & 0xff
        );
        QLOGV("Reference %u.%u\n", reftime.coarse, QCM_USEC(reftime.fine));
        QLOGV("Originate %u.%u\n", orgtime.coarse, QCM_USEC(orgtime.fine));
        QLOGV("Send %u.%u\n", ntp_ptr->time_of_send.coarse, QCM_USEC(ntp_ptr->time_of_send.fine));
        QLOGV("Receive   %u.%u\n", rectime.coarse, QCM_USEC(rectime.fine));
        QLOGV("Transmit  %u.%u\n", xmttime.coarse, QCM_USEC(xmttime.fine));
        QLOGV(
            "Total elapsed: %f\n"
            "Server stall:  %f\n"
            "Slop:          %f\n",
            el_time,
            st_time,
            el_time - st_time
        );  //Round-trip delay of NTP packets
        QLOGV(
            "Skew:          %f\n"
            "Frequency:     %d\n"
            " day   second     elapsed    stall     skew  dispersion  freq\n",
            (skew1 - skew2) / 2,
            freq
        );  //Time difference between client and server

        //Verify timestamp consistency: the T1 returned by the server should equal the T1 sent by the client.
        if (orgtime.coarse == ntp_ptr->time_of_send.coarse && orgtime.fine == ntp_ptr->time_of_send.fine)
        {
            //Time synchronization successful, converting NTP time to UNIX time
            rt = xmttime.coarse - QCM_JAN_1970;
            qosa_rtc_gmtime_r(&rt, &ntp_ptr->ntp_rt_time);

            //Set special return value to notify the main task that the loop has ended
            result = QCM_NTP_ERR_NEGATIVE;
            goto exit;
        }
        else
        {
            //Timestamp mismatch, possibly due to replay attack or packet error
            result = QCM_NTP_ERR_UNKNOW;
            goto exit;
        }
    }
    else
    {
        result = QCM_NTP_ERR_SOCKET_READ_FAILURE;
        return result;
    }
exit:
    return result;
}

static qcm_ntp_result_code qcm_ntp_timeout_send_request(void *argv)
{
    qcm_ntp_result_code  result = QCM_NTP_SUCCESS;
    qcm_ntp_client_t    *ntp_ptr = QOSA_NULL;
    qcm_ntp_client_info *send_ptr = QOSA_NULL;

    send_ptr = (qcm_ntp_client_info *)argv;
    ntp_ptr = qcm_ntp_get_ptr(send_ptr->client_id);
    if (ntp_ptr == QOSA_NULL)
    {
        qosa_free(send_ptr);
        return QCM_NTP_ERR_UNKNOW;
    }

    if (ntp_ptr->user_opts.retry_cnt > 0)
    {
        ntp_ptr->user_opts.retry_cnt--;
        QLOGD("retry cnt :%d", ntp_ptr->user_opts.retry_cnt);
        result = qcm_ntp_send_sync_request(send_ptr);
        if (QCM_NTP_SUCCESS != result)
        {
            qosa_free(send_ptr);
        }

    }
    else
    {
        qosa_free(send_ptr);
        //No response after retry attempts
        result = QCM_NTP_ERR_TIMEOUT;
    }
    return result;
}

static void qcm_ntp_main_task(void *argv)
{
    int                 ret = 0;
    qcm_ntp_result_code result = 0;
    qcm_ntp_task_msg_t  ntp_wait_msg = {0};
    qcm_ntp_client_t   *ntp_ptr = (qcm_ntp_client_t *)argv;

    QLOGD("ntp task running");
    for (;;)
    {
        QLOGD("ntp qosa_msgq_wait");
        ret = qosa_msgq_wait(ntp_ptr->ntp_msgq, (qosa_uint8_t *)&ntp_wait_msg, sizeof(qcm_ntp_task_msg_t), QOSA_WAIT_FOREVER);
        if (ret != 0)
        {
            QLOGE("ret=%d", ret);
            qosa_task_sleep_sec(1);
            continue;
        }
        QLOGD("enter ntp task,msgid   = %d", ntp_wait_msg.msgid);
        switch (ntp_wait_msg.msgid)
        {
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
            //PDP Request
            case QCM_NTP_PDP_ACTIVE_REQ: {
                result = qcm_ntp_service_data_call(ntp_wait_msg.argv);
            }
            break;
#endif
            //DNS Resolution
            case QCM_NTP_DNS_PARSE_REQ: {
                result = qcm_ntp_service_dns_parse(ntp_wait_msg.argv);
            }
            break;

            //Send ntp request
            case QCM_NTP_SEND_REQUEST: {
                result = qcm_ntp_send_sync_request(ntp_wait_msg.argv);
            }
            break;
            //Process the received NTP echo message
            case QCM_NTP_RECV_REPLY: {
                result = qcm_ntp_recv_sync_reply(ntp_wait_msg.argv);
            }
            break;
            //Execution timeout event
            case QCM_NTP_RETRY_TIMER_EXP: {
                result = qcm_ntp_timeout_send_request(ntp_wait_msg.argv);
            }
            break;
            case QCM_NTP_USER_STOP:
            case QCM_NTP_SOCKET_CLOSE: {
                result = (ntp_ptr->final_result != QCM_NTP_SUCCESS) ? ntp_ptr->final_result : QCM_NTP_ERR_UNKNOW;
            }
            break;
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
            case QCM_NTP_PDP_RESP: {
                result = qcm_ntp_datacall_response_handler(ntp_wait_msg.argv);
            }
            break;
#endif
            case QCM_NTP_DNS_RESP: {
                result = qcm_ntp_dns_response_handler(ntp_wait_msg.argv);
            }
            break;
            default:
                break;
        }
        QLOGD("exit ret = %d", result);
        if (result != QCM_NTP_SUCCESS)
            break;
    }
    QLOGD("over ret = %d", result);
    ntp_ptr->final_result = (result == QCM_NTP_ERR_NEGATIVE) ? QCM_NTP_SUCCESS : result;

    //Call AT callback function to report URC
    ntp_ptr->notify_fcn(ntp_ptr->client_id, ntp_ptr->final_result, &ntp_ptr->ntp_rt_time, ntp_ptr->user_arg);
    qcm_ntp_free_session(ntp_ptr);

    //The task will be automatically destroyed after it exits naturally.
}

/**
 * @brief Release the corresponding NTP client pointer address
 */
static qcm_ntp_result_code qcm_ntp_free_session(qcm_ntp_client_t *ntp_ptr)
{
    int                ret = 0;
    qosa_uint32_t      msgq_cnt = 0;
    qcm_ntp_task_msg_t ntp_wait_msg = {0};

    qosa_mutex_lock(g_qcm_ntp_mutex, QOSA_WAIT_FOREVER);

    if (ntp_ptr->retry_timer != QOSA_NULL)
    {
        qosa_timer_stop(ntp_ptr->retry_timer);
        qosa_timer_delete(ntp_ptr->retry_timer);
        ntp_ptr->retry_timer = QOSA_NULL;
    }

    if (ntp_ptr->socket_hndl != 0)
    {
        qcm_socket_close(ntp_ptr->socket_hndl);
        ntp_ptr->socket_hndl = 0;
    }

    if (ntp_ptr->host_name != QOSA_NULL)
    {
        qosa_free(ntp_ptr->host_name);
        ntp_ptr->host_name = QOSA_NULL;
    }

    ret = qosa_msgq_get_cnt(ntp_ptr->ntp_msgq, &msgq_cnt);
    while (ret == 0 && msgq_cnt != 0)
    {
        ret = qosa_msgq_wait(ntp_ptr->ntp_msgq, (qosa_uint8_t *)&ntp_wait_msg, sizeof(qcm_ntp_task_msg_t), QOSA_NO_WAIT);
        if (ret == 0)
        {
            qosa_free(ntp_wait_msg.argv);
        }
        msgq_cnt--;
    }
    qosa_msgq_delete(ntp_ptr->ntp_msgq);

    g_qcm_ntp_mgr[ntp_ptr->client_id - QCM_NTP_OFFSET_NUM] = QOSA_NULL;
    qosa_memset(ntp_ptr, 0, sizeof(qcm_ntp_client_t));
    qosa_free(ntp_ptr);

    qosa_mutex_unlock(g_qcm_ntp_mutex);

    return QCM_NTP_SUCCESS;
}

/**
 * @brief Initialize the NTP request and obtain the corresponding NTP operation handle.
 *
 * @return qcm_ntp_client_id
 *       - Returns a client ID greater than 0 on success
 *       - Returns -1 on failure
 */
qcm_ntp_client_id qcm_ntp_client_new(void)
{
    int               ret = 0;
    int               i = 0;
    qcm_ntp_client_t *ntp_ptr = QOSA_NULL;

    for (i = 0; i < QCM_NTP_SESSION_ARRAY_CNT; i++)
    {
        if (g_qcm_ntp_mgr[i] == QOSA_NULL)
        {
            break;
        }
    }

    if (i == QCM_NTP_SESSION_ARRAY_CNT)
    {
        return -1;
    }

    ntp_ptr = (qcm_ntp_client_t *)qosa_malloc(sizeof(qcm_ntp_client_t));
    if (ntp_ptr == QOSA_NULL)
    {
        return -1;
    }

    qosa_memset(ntp_ptr, 0, sizeof(qcm_ntp_client_t));

    ntp_ptr->client_id = i + QCM_NTP_OFFSET_NUM;
    ntp_ptr->socket_hndl = 0;

    ret = qosa_timer_create(&ntp_ptr->retry_timer, qcm_ntp_retry_timer_callback, ntp_ptr);
    if (ret != 0)
    {
        goto exit;
    }

    ret = qosa_msgq_create(&ntp_ptr->ntp_msgq, sizeof(qcm_ntp_task_msg_t), QCM_NTP_MSGQ_MAX_CNT);
    if (ret != 0)
    {
        goto exit;
    }

    ret = qosa_task_create(&ntp_ptr->ntp_task, CONFIG_QCM_NTP_TASK_SIZE, QCM_NTP_TASK_PRIORITY, "ntp_task", qcm_ntp_main_task, ntp_ptr);
    if (ret != 0)
    {
        goto exit;
    }

    g_qcm_ntp_mgr[i] = ntp_ptr;

    return ntp_ptr->client_id;

exit:
    if (ntp_ptr->retry_timer != QOSA_NULL)
    {
        qosa_timer_delete(ntp_ptr->retry_timer);
    }
    if (ntp_ptr->ntp_msgq != QOSA_NULL)
    {
        qosa_msgq_delete(ntp_ptr->ntp_msgq);
    }
    if (ntp_ptr->ntp_task != QOSA_NULL)
    {
        qosa_task_delete(ntp_ptr->ntp_task);
    }
    qosa_free(ntp_ptr);

    return -1;
}

/**
 * @brief ntp asynchronous request action API interface
 *
 * @param[in] qcm_ntp_client_id client_id
 *          - Corresponds to the NTP client ID obtained from the `qcm_ntp_client_new` function
 *
 * @param[in] const char * host_name
 *          - Corresponding host domain name or IP address
 *
 * @param[in] qosa_uint16_t host_port
 *          - Corresponding to the NTP server port, generally defaults to 123
 *
 * @param[in] qcm_ntp_config_type * ping_options
 *          - For the user-configured parameters
 *
 * @param[in] qcm_ntp_sync_result_cb cb_fun
 *          - Corresponding result callback function
 *
 * @param[in] void * user_param
 *          - Parameters carried by the user
 *
 * @return qcm_ntp_result_code
 *       - Returns QCM_NTP_SUCCESS if successful
 *       - If it fails, returns other values
 */
qcm_ntp_result_code qcm_ntp_sync_start(
    qcm_ntp_client_id      client_id,
    const char            *host_name,
    qosa_uint16_t          host_port,
    qcm_ntp_config_t      *ntp_options,
    qcm_ntp_sync_result_cb cb_fun,
    void                  *user_param
)
{
    int                  ret = 0;
    qcm_ntp_client_t    *ntp_ptr = QOSA_NULL;
    char                *host_addr = QOSA_NULL;
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
    qosa_datacall_conn_t conn = 0;
#endif
    if (host_name == QOSA_NULL || ntp_options == QOSA_NULL||cb_fun == QOSA_NULL)
    {
        return QCM_NTP_ERR_INVALID_PARAM;
    }

    if (g_qcm_ntp_mutex == QOSA_NULL)
    {
        ret = qosa_mutex_create(&g_qcm_ntp_mutex);
        if (ret != 0)
        {
            return QCM_NTP_ERR_UNKNOW;
        }
    }
    QLOGD("ENTER NTP");

    //Lock to prevent internal task release while simultaneously performing task creation here
    //and simultaneously operate on assignment and deletion of the qcm_ntp_client_t session pointer
    qosa_mutex_lock(g_qcm_ntp_mutex, QOSA_WAIT_FOREVER);

    ntp_ptr = qcm_ntp_get_ptr(client_id);
    if (ntp_ptr == QOSA_NULL)
    {
        qosa_mutex_unlock(g_qcm_ntp_mutex);
        return QCM_NTP_ERR_INVALID_PARAM;
    }

    host_addr = qosa_malloc(qosa_strlen(host_name) + 1);
    if (host_addr == QOSA_NULL)
    {
        qosa_mutex_unlock(g_qcm_ntp_mutex);
        return QCM_NTP_ERR_MEMORY;
    }

    ntp_ptr->host_port = host_port;
    qosa_memcpy(host_addr, host_name, qosa_strlen(host_name));
    host_addr[qosa_strlen(host_name)] = '\0';
    ntp_ptr->host_name = host_addr;
    qosa_memcpy(&(ntp_ptr->user_opts), ntp_options, sizeof(qcm_ntp_config_t));
    ntp_ptr->notify_fcn = cb_fun;
    ntp_ptr->user_arg = user_param;
    qosa_mutex_unlock(g_qcm_ntp_mutex);
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
    //Check if the current PDP is activated; if already activated, proceed directly with DNS resolution; otherwise, perform PDP activation.
    conn = qosa_datacall_conn_new(ntp_options->sim_id, ntp_options->pdp_id, QOSA_DATACALL_CONN_TCPIP);
    if (qosa_datacall_get_status(conn) == QOSA_TRUE)
    {
        //PDP activation directly proceeds to DNS resolution
        return qcm_ntp_req_event(QCM_NTP_DNS_PARSE_REQ, ntp_ptr);
    }
    else
    {
        //Perform PDP request action
        return  qcm_ntp_req_event(QCM_NTP_PDP_ACTIVE_REQ, ntp_ptr);
    }
#else
     return qcm_ntp_req_event(QCM_NTP_DNS_PARSE_REQ, ntp_ptr);
#endif
}

//July 2, 2024, 13:19:47 Do not use stop to release resources; after the callback notifies the client, the task will automatically exit,
//When the outer AT has a higher priority and attempts to acquire again before the inner one has started to release, but after sending to the task, the task resources
#if 0
/**
 * @brief Execute ntp request to stop and release NTP resources
 *
 * Special note: Do not perform qcm_ntp_stop action within the ntp callback function.
 *
 * @param[in] qcm_ntp_client_id client_id
 *          - Corresponds to the NTP client ID obtained from the `qcm_ntp_client_new` function
 *
 * @return qcm_ntp_result_code
 *       - Returns QCM_NTP_SUCCESS if successful
 *       - If it fails, returns other values
 */
qcm_ntp_result_code qcm_ntp_stop(qcm_ntp_client_id client_id)
{
    qcm_ntp_client_t *ntp_ptr = QOSA_NULL;
    ntp_ptr = qcm_ntp_get_ptr(client_id);
    if (ntp_ptr == QOSA_NULL)
    {
        return QCM_NTP_SUCCESS;
    }

    return qcm_ntp_req_event(QCM_NTP_USER_STOP, ntp_ptr);
}
#endif

/**
 * @brief ntp release function, actual release will be automatically executed after ntp completion
 * @note Here for the convenience of API pairing
 */
qcm_ntp_result_code qcm_ntp_client_free(qcm_ntp_client_id client_id)
{
    QOSA_UNUSED(client_id);

    return QCM_NTP_SUCCESS;
}
