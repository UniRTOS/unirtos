/*****************************************************************/ /**
* @file qcm_mqtt.c
* @brief
* @author lawrence.liu@quectel.com
* @date 2023-08-09
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description
* <tr><td>2023-08-09 <td>1.0 <td>lawrence.liu <td> Init
* </table>
**********************************************************************/
#include "qosa_def.h"
#include "qosa_sys.h"
#include "qcm_utf8.h"
#include "qosa_rtc.h"
#include "qcm_socket_adp.h"
#include "qosa_asyn_dns.h"
#include "qcm_mqtt.h"
#include "qcm_mqtt_packet.h"
#include "qcm_mqtt_private.h"
#include "qcm_mqtt_properties.h"
#include "qcm_proj_config.h"
#include "qosa_log.h"
#include "qosa_nvitem.h"

/** Redefine the TAG of this file */
#define QOS_LOG_TAG LOG_TAG_MQTT_API

/**
 * @brief Global management of MQTT integration business logic
 */
typedef struct
{
    qosa_bool_t        task_ready; /*!< Whether the task thread is ready*/
    qosa_task_t        mqtt_task;
    qosa_msgq_t        mqtt_msgq;
    qcm_mqtt_client_t *client[QCM_MQTT_MAX_CLIENT_CNT];
    qosa_time_t        last_time;
    qosa_mutex_t       mutex;
} qcm_mqtt_context_t;

/**
 * @brief Used to pass cyclic timeout events
 */
typedef struct
{
    qosa_uint8_t       client_id;
    qcm_mqtt_eercode_e result;
} qcm_mqtt_clientid_info_t;

#define MQTT_FREE_PTR(p)                                                                                                                                       \
    {                                                                                                                                                          \
        if ((p) != QOSA_NULL)                                                                                                                                  \
        {                                                                                                                                                      \
            qosa_free(p);                                                                                                                                      \
            (p) = QOSA_NULL;                                                                                                                                   \
        }                                                                                                                                                      \
    }

static void               qcm_mqtt_task_process_handler(void *argv);
static void               qcm_mqtt_client_tcp_send_handler(void *argv);
static void               qcm_mqtt_client_set_min_diff_time(qcm_mqtt_client_t *client_ptr);
static qcm_mqtt_eercode_e qcm_mqtt_client_event_report(qcm_mqtt_event_id_e cmd, void *argv);
static qcm_mqtt_client_t *qcm_mqtt_client_get_ptr(qosa_uint8_t client_id);

/** MQTT global structure, storing main configuration and runtime messages for MQTT.*/
static qcm_mqtt_context_t g_mqtt_context = {0};

//Record the reason for the last closure of each channel.
// Record the last error message for each socket route.
static int g_mqtt_last_close_event[QCM_MQTT_MAX_CLIENT_CNT] = {0};

void qcm_mqtt_free_data(qcm_mqtt_data_t *data_ptr)
{
    if (data_ptr != QOSA_NULL)
    {
        if (data_ptr->data_ptr != QOSA_NULL)
        {
            qosa_free(data_ptr->data_ptr);
            data_ptr->data_ptr = QOSA_NULL;
        }
        data_ptr->data_len = 0;
    }

    return;
}

qcm_mqtt_eercode_e qcm_mqtt_malloc_data(qcm_mqtt_data_t *data_ptr, qosa_uint32_t data_len, char *data)
{
    if (data_len == 0 || data_ptr == QOSA_NULL)
    {
        return QCM_MQTT_RES_OK;
    }
    if(data_len > 0 && data == QOSA_NULL)
    {
        return QCM_MQTT_RES_ERROR_PARAM_INVALID;
    }
	
    data_ptr->data_len = data_len;
    data_ptr->data_ptr = qosa_malloc(data_ptr->data_len + 1);
    if (data_ptr->data_ptr == QOSA_NULL)
    {
        return QCM_MQTT_RES_ERROR_NO_MEMORY;
    }

    qosa_memcpy(data_ptr->data_ptr, data, data_len);
    return QCM_MQTT_RES_OK;
}

#ifdef CONFIG_QCM_MQTT5_FUNC
qcm_mqtt_eercode_e qcm_mqtt_user_property_free_data(qosa_uint8_t user_property_cnt, qcm_mqtt_user_property_t *user_prop)
{
    if (user_prop == QOSA_NULL)
    {
        return QCM_MQTT_RES_OK;
    }
    qosa_uint8_t i = 0;
    for (; i < user_property_cnt; i++)
    {
        QLOGV("i:%d", i);

        qcm_mqtt_free_data(&user_prop[i].key);
        qcm_mqtt_free_data(&user_prop[i].value);
    }

    return QCM_MQTT_RES_OK;
}

qcm_mqtt_eercode_e qcm_mqtt_release_connack_props(qcm_mqtt_connack_properties_t *conn_props)
{
    if (conn_props == QOSA_NULL)
    {
        return QCM_MQTT_RES_OK;
    }

    qcm_mqtt_free_data(&conn_props->response_information);

    if (conn_props->user_property_cnt != 0)
    {
        qcm_mqtt_user_property_free_data(conn_props->user_property_cnt, conn_props->user_property);
    }

    return QCM_MQTT_RES_OK;
}

qcm_mqtt_eercode_e qcm_mqtt_release_newmsg_props(qcm_mqtt_pub_properties_t *pub_props)
{
    if (pub_props == QOSA_NULL)
    {
        return QCM_MQTT_RES_OK;
    }

    qcm_mqtt_free_data(&pub_props->response_topic);
    qcm_mqtt_free_data(&pub_props->content_type);
    qcm_mqtt_free_data(&pub_props->correlation_data);

    if (pub_props->user_property_cnt != 0)
    {
        qcm_mqtt_user_property_free_data(pub_props->user_property_cnt, pub_props->user_property);
    }

    return QCM_MQTT_RES_OK;
}

qcm_mqtt_eercode_e qcm_mqtt_topic_alias_check(qosa_uint8_t client_id, qosa_uint16_t topic_alias)
{
    qcm_mqtt_client_t *client_ptr = QOSA_NULL;

    client_ptr = (qcm_mqtt_client_t *)qcm_mqtt_client_get_ptr(client_id);
    if (client_ptr == QOSA_NULL)
    {
        return QCM_MQTT_RES_ERROR_PARAM_IS_NULL;
    }

    if (client_ptr->connack_props->topic_alias_maximum == 0)
    {
        return QCM_MQTT_RES_OK;
    }

    if (topic_alias > client_ptr->connack_props->topic_alias_maximum)
    {
        return QCM_MQTT_RES_ERROR_PARAM_INVALID;
    }

    return QCM_MQTT_RES_OK;
}

/**
 * @brief Linked List Comparison Function
 */
qosa_bool_t qcm_mqtt_client_compare_topic_alias(void *item_ptr, void *compare_val)
{
    qosa_uint16_t          *topic_alias = (qosa_uint16_t *)compare_val;
    qcm_mqtt_topic_alias_t *topic_alias_ptr = (qcm_mqtt_topic_alias_t *)item_ptr;

    QLOGI("topic_alias: %d %d", topic_alias_ptr->topic_alias, *topic_alias);

    if (topic_alias_ptr->topic_alias == *topic_alias)
    {
        return QOSA_TRUE;
    }

    return QOSA_FALSE;
}

qcm_mqtt_eercode_e qcm_mqtt_topic_alias_bind(qcm_mqtt_client_t *client_ptr, qosa_uint16_t topic_alias, qcm_mqtt_data_t *topic)
{
    qcm_mqtt_topic_alias_t *topic_alias_ptr = QOSA_NULL;

    topic_alias_ptr = qosa_q_linear_search(&client_ptr->topic_alias_list, qcm_mqtt_client_compare_topic_alias, &topic_alias);
    if (topic_alias_ptr != QOSA_NULL)
    {
        QLOGV("TopicAlias: mapping found (alias=%d -> topic=%s)", topic_alias, topic_alias_ptr->topic.data_ptr);

        qcm_mqtt_free_data(&topic_alias_ptr->topic);
        qcm_mqtt_malloc_data(&topic_alias_ptr->topic, topic->data_len, topic->data_ptr);
    }
    else
    {
        if (qosa_q_cnt(&client_ptr->topic_alias_list) > QCM_MQTT_MAX_TOPIC_ALIAS_CNT)
        {
            QLOGE("QCM_MQTT_MAX_TOPIC_ALIAS_CNT!");
            return QCM_MQTT_RES_ERROR_NO_MEMORY;
        }

        topic_alias_ptr = qosa_malloc(sizeof(qcm_mqtt_topic_alias_t));
        if (topic_alias_ptr != QOSA_NULL)
        {
            topic_alias_ptr->topic.data_len = topic->data_len;
            topic_alias_ptr->topic.data_ptr = topic->data_ptr;
            topic->data_len = 0;
            topic->data_ptr = QOSA_NULL;

            topic_alias_ptr->topic_alias = topic_alias;
            QLOGI("add topic alias : %d", topic_alias_ptr->topic_alias);
            qosa_q_link(topic_alias_ptr, &topic_alias_ptr->link);
            qosa_q_put(&client_ptr->topic_alias_list, &topic_alias_ptr->link);
        }
    }

    return QCM_MQTT_RES_OK;
}
#endif /* CONFIG_QCM_MQTT5_FUNC */

/**
 * @brief Send the reply message to the upper layer. If the data being sent involves allocated memory, it must be determined whether the transmission is successful; if the transmission fails, the memory must be freed.
 *
 * @param[in] qcm_mqtt_client_t * client_ptr
 *           - Client pointer
 *
 * @param[in] qcm_mqtt_client_event_e event_type
 *           -MQTT event types.
 *
 * @param[in] qcm_mqtt_eercode_e result
 *           - Error codes returned by the MQTT interface.
 *
 * @param[in] void * user_param
 *           - User information pointer, used during callback.
 *
 * @return qcm_mqtt_eercode_e
 *        - Returns MQTT_RES_OK to indicate success.
 *        - Returns other values from mqtt_eercode_e to indicate failure.
 */
static qcm_mqtt_eercode_e
qcm_mqtt_client_response_commone(qcm_mqtt_client_t *client_ptr, qcm_mqtt_client_event_e event_type, qcm_mqtt_eercode_e result, void *user_data)
{
    qcm_mqtt_common_resp_t *common_resp_ptr = QOSA_NULL;

    QLOGV("client_id:%d event_type:%d result:%d", client_ptr->client_id, event_type, result);

    common_resp_ptr = (qcm_mqtt_common_resp_t *)qosa_malloc(sizeof(qcm_mqtt_common_resp_t));
    if (common_resp_ptr == QOSA_NULL)
    {
        QLOGE("qosa_malloc fail");
        return QCM_MQTT_RES_ERROR_NO_MEMORY;
    }
    else
    {
        common_resp_ptr->client_id = client_ptr->client_id;
        common_resp_ptr->result = result;

        if (user_data != QOSA_NULL || event_type == QCM_MQTT_CLIENT_STATE_EVENT)
        {
            switch (event_type)
            {
                case QCM_MQTT_CLIENT_CONNECT_EVENT: {
                    mqtt_connack_packet *connack_pkt = (mqtt_connack_packet *)user_data;

                    common_resp_ptr->version = connack_pkt->version;
                    common_resp_ptr->pocotrol_code = connack_pkt->return_code;
                }
                break;

                case QCM_MQTT_CLIENT_SUBSCRIBE_EVENT:
                case QCM_MQTT_CLIENT_UNSUBSCRIBE_EVENT:
                case QCM_MQTT_CLIENT_PUBLISH_EVENT: {
                    qcm_mqtt_send_resp_t *send_pkt = (qcm_mqtt_send_resp_t *)user_data;
                    qcm_mqtt_send_resp_t *send_ptr = QOSA_NULL;

                    send_ptr = (qcm_mqtt_send_resp_t *)qosa_malloc(sizeof(qcm_mqtt_send_resp_t));
                    if (send_ptr == QOSA_NULL)
                    {
                        qosa_free(common_resp_ptr);
                        return QCM_MQTT_RES_ERROR_NO_MEMORY;
                    }

                    common_resp_ptr->msg_id = send_pkt->msg_id;
                    send_ptr->send_result = send_pkt->send_result;
                    send_ptr->retry_count = send_pkt->retry_count;
                    common_resp_ptr->data = send_ptr;
                }
                break;

                case QCM_MQTT_CLIENT_UNSUBACK_EVENT: {
                    mqtt_unsuback_packet *unsuback_pkt = (mqtt_unsuback_packet *)user_data;

                    common_resp_ptr->version = unsuback_pkt->version;
                    common_resp_ptr->msg_id = unsuback_pkt->msg_id;

#ifdef CONFIG_QCM_MQTT5_FUNC
                    common_resp_ptr->data = unsuback_pkt->options;
#endif /* CONFIG_QCM_MQTT5_FUNC */
                }
                break;

                case QCM_MQTT_CLIENT_SUBACK_EVENT: {
                    mqtt_suback_packet     *suback_pkt = (mqtt_suback_packet *)user_data;
                    qcm_mqtt_suback_resp_t *suback_resp_ptr = QOSA_NULL;

                    suback_resp_ptr = (qcm_mqtt_suback_resp_t *)qosa_malloc(sizeof(qcm_mqtt_suback_resp_t));
                    if (suback_resp_ptr == QOSA_NULL)
                    {
                        qosa_free(common_resp_ptr);
#ifdef CONFIG_QCM_MQTT5_FUNC
                        qosa_free(suback_pkt->options);
#endif /* CONFIG_QCM_MQTT5_FUNC */
                        return QCM_MQTT_RES_ERROR_NO_MEMORY;
                    }

#ifdef CONFIG_QCM_MQTT5_FUNC
                    suback_resp_ptr->options = suback_pkt->options;
#endif /* CONFIG_QCM_MQTT5_FUNC */
                    common_resp_ptr->msg_id = suback_pkt->msg_id;
                    suback_resp_ptr->qos_cnt = suback_pkt->qos_cnt;

                    suback_resp_ptr->qoss = suback_pkt->qoss;
                    common_resp_ptr->data = suback_resp_ptr;
                }
                break;

#ifdef CONFIG_QCM_MQTT5_FUNC
                case QCM_MQTT_CLIENT_DISCONNECT_EVENT: {
                    mqtt_disconnect_packet *disc_pkt = (mqtt_disconnect_packet *)user_data;

                    common_resp_ptr->data = disc_pkt->options;
                    common_resp_ptr->pocotrol_code = disc_pkt->reason_code;
                }
                break;
#endif /* CONFIG_QCM_MQTT5_FUNC */

                case QCM_MQTT_CLIENT_NEW_MESSAGE_EVENT: {
                    qcm_mqtt_new_message_t    *store_info = (qcm_mqtt_new_message_t *)user_data;
                    qcm_mqtt_new_msg_notify_t *msg_notify = QOSA_NULL;

                    msg_notify = (qcm_mqtt_new_msg_notify_t *)qosa_malloc(sizeof(qcm_mqtt_new_msg_notify_t));
                    if (msg_notify == QOSA_NULL)
                    {
                        qosa_free(common_resp_ptr);
                        return QCM_MQTT_RES_ERROR_NO_MEMORY;
                    }

                    msg_notify->store_id = store_info->store_id;
                    common_resp_ptr->data = msg_notify;
                }
                break;

                case QCM_MQTT_CLIENT_STATE_EVENT: {
                    qcm_mqtt_close_cause_t *close_cause = QOSA_NULL;

                    close_cause = (qcm_mqtt_close_cause_t *)qosa_malloc(sizeof(qcm_mqtt_close_cause_t));
                    if (close_cause == QOSA_NULL)
                    {
                        qosa_free(common_resp_ptr);
                        return QCM_MQTT_RES_ERROR_NO_MEMORY;
                    }

                    common_resp_ptr->client_id = client_ptr->client_id;
                    close_cause->close_cause = client_ptr->close_cause;
                    common_resp_ptr->data = close_cause;
                }
                break;

                default:
                    break;
            }
        }
    }

    client_ptr->event_cb(event_type, (void *)common_resp_ptr, client_ptr->user_argv);
    return QCM_MQTT_RES_OK;
}

/**
 * @brief Report the reason for closure in the case of non-active closure.
 */
static inline void qcm_mqtt_client_close_cause(qcm_mqtt_client_t *client_ptr)
{
    qcm_mqtt_client_response_commone(client_ptr, QCM_MQTT_CLIENT_STATE_EVENT, QCM_MQTT_RES_OK, QOSA_NULL);
}

/**
 * @brief Used to send message notifications to the MQTT task
 *
 * @param[in] qcm_mqtt_event_id_e cmd
 *           - Event ID, identifying the MQTT event to be reported
 * @param[in] void *argv
 *           - Event parameters, which may vary in content depending on the event ID.
 *
 * @return qcm_mqtt_eercode_e
 *        - Returns the execution result; if the task is ready and the message is successfully sent, returns QCM_MQTT_RES_OK;
 *        - If the task is not ready, return QCM_MQTT_RES_ERROR_TASK.
 */
static void qcm_mqtt_check_and_create_task(void)
{
    int ret = 0;
    if (g_mqtt_context.mutex == QOSA_NULL)
    {
        ret = qosa_mutex_create(&g_mqtt_context.mutex);
        if(ret != QOSA_OK)
        {
            QLOGE("mutex create failed");
            return;
        }
    }
    if (g_mqtt_context.task_ready == QOSA_FALSE)
    {
        if (qosa_msgq_create(&g_mqtt_context.mqtt_msgq, sizeof(qcm_mqtt_event_info_t), QCM_MQTT_MSGQ_MAX_CNT) == QOSA_OK)
        {
            if (qosa_task_create(
                    &g_mqtt_context.mqtt_task,
                    CONFIG_QCM_MQTT_TASK_STACK_SIZE,
                    QCM_MQTT_TASK_PRIO,
                    "mqtt_cl",
                    qcm_mqtt_task_process_handler,
                    &g_mqtt_context
                )
                == QOSA_OK)
            {
                g_mqtt_context.task_ready = QOSA_TRUE;
            }
            else
            {
                qosa_msgq_delete(g_mqtt_context.mqtt_msgq);
            }
        }
    }
}

static qcm_mqtt_eercode_e qcm_mqtt_client_event_report(qcm_mqtt_event_id_e cmd, void *argv)
{
    int                   ret = 0;
    qcm_mqtt_event_info_t event_info = {QCM_MQTT_MIN, QOSA_NULL};
    event_info.cmd = cmd;
    event_info.argv = argv;
    QLOGI("mqtt cmd:%d", cmd);

    if (g_mqtt_context.task_ready == QOSA_TRUE)
    {
        ret = qosa_msgq_release(g_mqtt_context.mqtt_msgq, sizeof(qcm_mqtt_event_info_t), (qosa_uint8_t *)&event_info, QOSA_NO_WAIT);
        if (ret != 0)
        {
            //There is a probability that the message queue will become full.
            qosa_uint32_t cnt_ptr = 0;
            qosa_msgq_get_cnt(g_mqtt_context.mqtt_msgq, &cnt_ptr);
            QLOGE("cnt_ptr:%d ret:%d", cnt_ptr, ret);
            return QCM_MQTT_RES_ERROR_TASK;
        }
    }
    else
    {
        QLOGE("task no ready");
        return QCM_MQTT_RES_ERROR_TASK;
    }

    return QCM_MQTT_RES_OK;
}

/**
 * @brief Send the client ID information to the MQTT Task for subsequent state execution.
 *
 * @param[in] qcm_mqtt_event_id_e cmd
 *           -MQTT event ID, used to indicate the operation that needs to be performed
 * @param[in] qcm_mqtt_client_t *client_ptr
 *           - Pointer to the MQTT client instance, containing the client ID that needs to be reported
 * @param qcm_mqtt_eercode_e result
 *       - Operation result code, indicating the success or failure of the operation
 */
static void qcm_mqtt_client_report_clientid(qcm_mqtt_event_id_e cmd, qcm_mqtt_client_t *client_ptr, qcm_mqtt_eercode_e result)
{
    qcm_mqtt_clientid_info_t *clientid_info_ptr = QOSA_NULL;

    if (client_ptr->client_state == QCM_MQTT_STATE_TCP_CLOSED)
    {
        QLOGI("...");
        return;
    }

    clientid_info_ptr = (qcm_mqtt_clientid_info_t *)qosa_malloc(sizeof(qcm_mqtt_clientid_info_t));
    if (clientid_info_ptr == QOSA_NULL)
    {
        QLOGW("...");
        return;
    }

    //add larson:
    //1. On x64 systems, the address is 50+, but we often use uint32 or int32, which causes issues during cross-platform development. That's why the osa_ptr type was introduced.
    //2. If internal state changes are not synchronized and messages are stored in the queue, then when only the client ID is present and the corresponding pointer cannot be found, it can return directly, avoiding or reducing critical issues.
    clientid_info_ptr->client_id = client_ptr->client_id;
    clientid_info_ptr->result = result;
    if (qcm_mqtt_client_event_report(cmd, clientid_info_ptr) != QCM_MQTT_RES_OK)
    {
        qosa_free(clientid_info_ptr);
    }
}

/**
 * @brief Used to save the MQTT new connection handle
 */
static qcm_mqtt_eercode_e qcm_mqtt_client_set_ptr(qosa_uint8_t client_id, qcm_mqtt_client_t *client_ptr)
{
    qcm_mqtt_eercode_e ret = QCM_MQTT_RES_ERROR_ID_USE;

    QLOGV("enter");
    if (client_id > QCM_MQTT_MAX_CLIENT_NUMBER_ID)
    {
        return QCM_MQTT_RES_ERROR_PARAM_INVALID;
    }

    qosa_mutex_lock(g_mqtt_context.mutex, QOSA_WAIT_FOREVER);

    if (g_mqtt_context.client[client_id] == QOSA_NULL)
    {
        QLOGW("NULL");
        ret = QCM_MQTT_RES_OK;
        g_mqtt_context.client[client_id] = client_ptr;
    }

    qosa_mutex_unlock(g_mqtt_context.mutex);
    return ret;
}

/**
 * @brief Obtain the MQTT client operation pointer via socket_fd
 */
static qcm_mqtt_client_t *qcm_mqtt_client_get_ptr_with_socket_fd(int socket_fd)
{
    int i = 0;
    for (i = 0; i < QCM_MQTT_MAX_CLIENT_CNT; i++)
    {
        if (g_mqtt_context.client[i] != QOSA_NULL)
        {
            if (g_mqtt_context.client[i]->socketfd == socket_fd)
            {
                return g_mqtt_context.client[i];
            }
        }
    }
    QLOGV("no search =%d !", socket_fd);
    return QOSA_NULL;
}

/**
 * @brief Get MQTT client operation pointer
 */
static qcm_mqtt_client_t *qcm_mqtt_client_get_ptr(qosa_uint8_t client_id)
{
    if (client_id > QCM_MQTT_MAX_CLIENT_NUMBER_ID)
    {
        return QOSA_NULL;
    }

    if (g_mqtt_context.client[client_id] == QOSA_NULL)
    {
        QLOGW("...%d IS QOSA_NULL!!!!!", client_id);
        return QOSA_NULL;
    }
    else
    {
        return g_mqtt_context.client[client_id];
    }
}
#ifdef CONFIG_QCM_MQTT5_FUNC
static qcm_mqtt_support_version_e qcm_mqtt_client_get_version(qosa_uint8_t client_id)
{
    if (client_id >= QCM_MQTT_MAX_CLIENT_CNT)
    {
        return QOSA_NULL;
    }

    if (g_mqtt_context.client[client_id] == QOSA_NULL)
    {
        QLOGW("...%d IS QOSA_NULL!!!!!", client_id);
        return QOSA_NULL;
    }
    else
    {
        return g_mqtt_context.client[client_id]->user_option.version;
    }
}
#endif /* CONFIG_QCM_MQTT5_FUNC */

/**
 * @brief Get MQTT client operation pointer
 */
static qosa_bool_t qcm_mqtt_client_check_id_is_exist(qosa_uint8_t client_id)
{
    return g_mqtt_context.client[client_id] != QOSA_NULL ? QOSA_TRUE : QOSA_FALSE;
}

/**
 * @brief Linked List Comparison Function
 */
qosa_bool_t qcm_mqtt_client_compare_msg_id(void *item_ptr, void *compare_val)
{
    qcm_mqtt_packet_compare_t *compare = (qcm_mqtt_packet_compare_t *)compare_val;
    qcm_mqtt_request_t        *mqtt_request = (qcm_mqtt_request_t *)item_ptr;

    if (mqtt_request->header.bits.type == compare->msg_type && mqtt_request->msg_id == compare->msg_id)
    {
        return QOSA_TRUE;
    }

    return QOSA_FALSE;
}

/**
 * @brief Remove cached messages
 */
static void qcm_mqtt_client_remove_request(qcm_mqtt_client_t *client_ptr, int msg_type, int msg_id)
{
    qcm_mqtt_request_t       *mqtt_request = QOSA_NULL;
    qcm_mqtt_packet_compare_t compare = {0};

    qosa_memset(&compare, 0, sizeof(qcm_mqtt_packet_compare_t));
    compare.msg_id = msg_id;
    compare.msg_type = msg_type;

    mqtt_request = qosa_q_linear_search(&client_ptr->pend_req_queue, qcm_mqtt_client_compare_msg_id, &compare);
    while (mqtt_request != QOSA_NULL)
    {
        qosa_q_delete(&client_ptr->pend_req_queue, &mqtt_request->link);
        qcm_mqtt_free_data(&mqtt_request->buffer.buf_ptr);
        qosa_free(mqtt_request);
        mqtt_request = QOSA_NULL;
        mqtt_request = qosa_q_linear_search(&client_ptr->pend_req_queue, qcm_mqtt_client_compare_msg_id, &compare);
    }

    mqtt_request = qosa_q_linear_search(&client_ptr->out_packet_queue, qcm_mqtt_client_compare_msg_id, &compare);
    while (mqtt_request != QOSA_NULL)
    {
        qosa_q_delete(&client_ptr->out_packet_queue, &mqtt_request->link);
        qcm_mqtt_free_data(&mqtt_request->buffer.buf_ptr);
        qosa_free(mqtt_request);
        mqtt_request = QOSA_NULL;
        mqtt_request = qosa_q_linear_search(&client_ptr->out_packet_queue, qcm_mqtt_client_compare_msg_id, &compare);
    }

    qcm_mqtt_client_set_min_diff_time(client_ptr);
}

/**
 * @brief Remove cached messages
 */
static void qcm_mqtt_client_remove_request_all(qcm_mqtt_client_t *client_ptr)
{
    while (qosa_q_cnt(&client_ptr->pend_req_queue))
    {
        qcm_mqtt_request_t *mqtt_request = QOSA_NULL;

        mqtt_request = (qcm_mqtt_request_t *)qosa_q_get(&client_ptr->pend_req_queue);
        qcm_mqtt_free_data(&mqtt_request->buffer.buf_ptr);
        qosa_free(mqtt_request);
    }

    while (qosa_q_cnt(&client_ptr->out_packet_queue))
    {
        qcm_mqtt_request_t *mqtt_request = QOSA_NULL;

        mqtt_request = (qcm_mqtt_request_t *)qosa_q_get(&client_ptr->out_packet_queue);
        qcm_mqtt_free_data(&mqtt_request->buffer.buf_ptr);
        qosa_free(mqtt_request);
    }
}

/**
 * @brief Release and clean up MQTT client resources, used to free and clean all memory resources associated with the specified MQTT client, including message queues, buffers, and timers.
 *
 * @param[in] qcm_mqtt_client_t *client_ptr  indicates the target MQTT client instance whose resources need to be released.
 */
static void qcm_mqtt_client_free(qcm_mqtt_client_t *client_ptr)
{
    QLOGV("enter");
    qcm_mqtt_client_remove_request_all(client_ptr);

    while (qosa_q_cnt(&client_ptr->new_msg_pub))
    {
        qcm_mqtt_new_message_t *mqtt_request = QOSA_NULL;

        mqtt_request = (qcm_mqtt_new_message_t *)qosa_q_get(&client_ptr->new_msg_pub);
        qcm_mqtt_free_data(&mqtt_request->new_msg.topic);
        qcm_mqtt_free_data(&mqtt_request->new_msg.payload);
        qosa_free(mqtt_request);
    }

    while (qosa_q_cnt(&client_ptr->inboundMsgs))
    {
        qcm_mqtt_new_message_t *mqtt_request = QOSA_NULL;

        mqtt_request = (qcm_mqtt_new_message_t *)qosa_q_get(&client_ptr->inboundMsgs);
        qcm_mqtt_free_data(&mqtt_request->new_msg.topic);
        qcm_mqtt_free_data(&mqtt_request->new_msg.payload);
        qosa_free(mqtt_request);
    }

    if (client_ptr->cur_input_packet.input_buff != QOSA_NULL)
    {
        mqtt_input_publish_packet *pub_pkt = (mqtt_input_publish_packet *)client_ptr->cur_input_packet.input_buff;

        if (pub_pkt->topic != QOSA_NULL)
        {
            qcm_mqtt_free_data(&pub_pkt->topic->buf_ptr);
            qosa_free(pub_pkt->topic);
        }

        if (pub_pkt->payload != QOSA_NULL)
        {
            qcm_mqtt_free_data(&pub_pkt->payload->buf_ptr);
            qosa_free(pub_pkt->payload);
        }
        qosa_free(pub_pkt);
        client_ptr->cur_input_packet.input_buff = QOSA_NULL;
    }

    qosa_q_destroy(&client_ptr->inboundMsgs);
    qosa_q_destroy(&client_ptr->new_msg_pub);
    qosa_q_destroy(&client_ptr->out_packet_queue);
    qosa_q_destroy(&client_ptr->pend_req_queue);

#ifdef CONFIG_QCM_MQTT5_FUNC
    if (client_ptr->connack_props != QOSA_NULL)
    {
        qcm_mqtt_release_connack_props(client_ptr->connack_props);
        qosa_free(client_ptr->connack_props);
    }

    while (qosa_q_cnt(&client_ptr->topic_alias_list))
    {
        qcm_mqtt_topic_alias_t *topic_alias_ptr = QOSA_NULL;

        topic_alias_ptr = (qcm_mqtt_topic_alias_t *)qosa_q_get(&client_ptr->topic_alias_list);
        qcm_mqtt_free_data(&topic_alias_ptr->topic);
        qosa_free(topic_alias_ptr);
    }

    qosa_q_destroy(&client_ptr->topic_alias_list);

#endif /* CONFIG_QCM_MQTT5_FUNC */

    if (qosa_timer_is_running(client_ptr->timer) == QOSA_TRUE)
    {
        qosa_timer_stop(client_ptr->timer);
    }
    qosa_timer_delete(client_ptr->timer);

    g_mqtt_context.client[client_ptr->client_id] = QOSA_NULL;
    client_ptr->read_event_cnt = 0;
    client_ptr->client_state = QCM_MQTT_STATE_INIT;
    qosa_free(client_ptr);
    client_ptr = QOSA_NULL;
}

/**
 * @brief Linked List Comparison Function
 */
qosa_bool_t qcm_mqtt_client_qos2_compare_msg_id(void *item_ptr, void *compare_val)
{
    qosa_uint16_t          *msg_id = (qosa_uint16_t *)compare_val;
    qcm_mqtt_new_message_t *new_msg_ptr = (qcm_mqtt_new_message_t *)item_ptr;

    if (new_msg_ptr->new_msg.msg_id == *msg_id)
    {
        return QOSA_TRUE;
    }

    return QOSA_FALSE;
}

//QOS=2, upon receiving a publish packet, first insert it into the inboundMsgs linked list
static void qcm_mqtt_client_store_publish_qos2(qcm_mqtt_client_t *client_ptr, qcm_mqtt_new_message_t *new_msg_ptr)
{
    qcm_mqtt_new_message_t *old_msg_ptr = QOSA_NULL;

    //Check if there is a historical cache with the same msg_id, and delete and release it.
    old_msg_ptr = qosa_q_linear_search(&client_ptr->inboundMsgs, qcm_mqtt_client_qos2_compare_msg_id, &(new_msg_ptr->new_msg.msg_id));
    while (old_msg_ptr != QOSA_NULL)
    {
        qosa_q_delete(&client_ptr->inboundMsgs, &old_msg_ptr->link);

        qcm_mqtt_free_data(&old_msg_ptr->new_msg.topic);
        qcm_mqtt_free_data(&old_msg_ptr->new_msg.payload);

        qosa_free(old_msg_ptr);
        QLOGV("retry msg_id:%d", new_msg_ptr->new_msg.msg_id);
        old_msg_ptr = qosa_q_linear_search(&client_ptr->inboundMsgs, qcm_mqtt_client_qos2_compare_msg_id, &(new_msg_ptr->new_msg.msg_id));
    }

    qosa_q_link(new_msg_ptr, &new_msg_ptr->link);
    qosa_q_put(&client_ptr->inboundMsgs, &new_msg_ptr->link);
}

//QOS=2, remove the packet from the inboundMsgs linked list and insert it into the subscribe_msg_box only after receiving the corresponding pubrel packet.
static qcm_mqtt_eercode_e qcm_mqtt_client_read_publish_qos2(qcm_mqtt_client_t *client_ptr, qcm_mqtt_new_message_t **new_msg_ptr, qosa_uint16_t msg_id)
{
    qcm_mqtt_new_message_t *old_msg_ptr = QOSA_NULL;

    old_msg_ptr = qosa_q_linear_search(&client_ptr->inboundMsgs, qcm_mqtt_client_qos2_compare_msg_id, &msg_id);
    if (old_msg_ptr != QOSA_NULL)
    {
        qosa_q_delete(&client_ptr->inboundMsgs, &old_msg_ptr->link);
        *new_msg_ptr = old_msg_ptr;
        return QCM_MQTT_RES_OK;
    }
    else
    {
        return QCM_MQTT_RES_ERROR_PARAM_INVALID;
    }
}

/**
 * @brief Linked list comparison function to check if the queried store_id exists
 */
static qosa_bool_t qcm_mqtt_client_compare_store_id(void *item_ptr, void *compare_val)
{
    qosa_uint8_t           *store_id = (qosa_uint8_t *)compare_val;
    qcm_mqtt_new_message_t *mqtt_request = (qcm_mqtt_new_message_t *)item_ptr;

    QLOGV("store_id : %d   %d", *store_id, mqtt_request->store_id);

    if (*store_id == mqtt_request->store_id)
    {
        return QOSA_TRUE;
    }

    return QOSA_FALSE;
}

/**
 * @brief Place the newly published acknowledged messages into the queue.
 */
static qcm_mqtt_eercode_e qcm_mqtt_client_add_new_publish_message(qcm_mqtt_client_t *client_ptr, qcm_mqtt_new_message_t *new_msg_ptr)
{
    qcm_mqtt_new_message_t *old_msg_ptr = QOSA_NULL;

    int store_id = 0;
    int msg_max_cnt = client_ptr->user_option.max_recv_store_cnt;

    for (store_id = 0; store_id < msg_max_cnt; store_id++)
    {
        old_msg_ptr = qosa_q_linear_search(&client_ptr->new_msg_pub, qcm_mqtt_client_compare_store_id, (void *)&store_id);
        if (old_msg_ptr == QOSA_NULL)
        {
            QLOGV("store_id : %d", store_id);
            break;
        }
    }

    if (store_id >= msg_max_cnt)
    {
        QLOGW("msg full");
        return QCM_MQTT_RES_ERROR_STORE_FULL;
    }

    new_msg_ptr->store_id = store_id;

    qosa_q_link(new_msg_ptr, &new_msg_ptr->link);
    qosa_q_put(&client_ptr->new_msg_pub, &new_msg_ptr->link);

    return QCM_MQTT_RES_OK;
}

/**
 * @brief TCPIP layer socket event notification
 */
static void qcm_mqtt_client_socket_evt_cb(int sock_hd, int event, int code, void *user_argv)
{
    qcm_mqtt_soc_event_t *evt = QOSA_NULL;

    QLOGI("evt_cb: %d,%x,%d", sock_hd, event, code);

    if (user_argv == QOSA_NULL)
    {
        qcm_socket_linger_t linger = {.on_off = 1, .linger_val = 0};
        qcm_socket_set_opt(sock_hd, QCM_SOCK_LINGER_OPT, (void *)&linger);

        qcm_socket_close(sock_hd);
        return;
    }
    qcm_mqtt_client_t *client_ptr = (qcm_mqtt_client_t *)user_argv;
    if (event == QCM_SOCK_READ_EVENT)
    {
        QLOGI("client_ptr->read_event_cnt: %d", client_ptr->read_event_cnt);
        if (client_ptr->read_event_cnt > (QCM_MQTT_MSGQ_MAX_CNT / 4))
            return;
        client_ptr->read_event_cnt++;
    }

    evt = (qcm_mqtt_soc_event_t *)qosa_malloc(sizeof(qcm_mqtt_soc_event_t)); /*!Destroyed after task exec*/
    QOSA_ASSERT(evt != QOSA_NULL);

    evt->sock_hd = sock_hd;
    evt->event_mask = event;
    evt->code = code;
    evt->user_data = user_argv;
    if (qcm_mqtt_client_event_report(QCM_MQTT_SOCKET_EVENT, (void *)evt) != QCM_MQTT_RES_OK)
    {
        qosa_free(evt);
    }
}

static qosa_bool_t qcm_mqtt_client_compare_new_msg_id(void *item_ptr, void *compare_val)
{
    int                *msg_id = (int *)compare_val;
    qcm_mqtt_request_t *mqtt_request = (qcm_mqtt_request_t *)item_ptr;

    if (mqtt_request->msg_id == *msg_id)
    {
        return QOSA_TRUE;
    }

    return QOSA_FALSE;
}

static qosa_bool_t qcm_mqtt_client_compare_ping_type(void *item_ptr, void *compare_val)
{
    int                *type = (int *)compare_val;
    qcm_mqtt_request_t *mqtt_request = (qcm_mqtt_request_t *)item_ptr;

    if (mqtt_request->header.bits.type == *type)
    {
        return QOSA_TRUE;
    }

    return QOSA_FALSE;
}

static qosa_bool_t qcm_mqtt_client_check_request_allow(qcm_mqtt_client_t *client_ptr, int msg_id)
{
    qcm_mqtt_request_t *mqtt_request = QOSA_NULL;
    int                 check_msg_id = msg_id;

    //Check if there is a message with the same msg_id currently being sent or in a pending state.
    mqtt_request = qosa_q_linear_search(&client_ptr->out_packet_queue, qcm_mqtt_client_compare_new_msg_id, &check_msg_id);
    if (mqtt_request != QOSA_NULL)
    {
        return QOSA_FALSE;
    }

    mqtt_request = qosa_q_linear_search(&client_ptr->pend_req_queue, qcm_mqtt_client_compare_new_msg_id, &check_msg_id);
    if (mqtt_request != QOSA_NULL)
    {
        return QOSA_FALSE;
    }

    return QOSA_TRUE;
}

static qosa_bool_t qcm_mqtt_client_check_request_ping(qcm_mqtt_client_t *client_ptr, qosa_time_t now)
{
    qcm_mqtt_request_t *mqtt_request = QOSA_NULL;
    mqtt_msg_types      type = MQTT_PINGREQ;
    qosa_time_t         kalive_time_ms; // keep-alive interval ms

    //Check if there is a message with the same msg_id currently being sent or in a pending state.
    mqtt_request = qosa_q_linear_search(&client_ptr->out_packet_queue, qcm_mqtt_client_compare_ping_type, &type);
    if (mqtt_request != QOSA_NULL)
    {
        return QOSA_FALSE;
    }

    mqtt_request = qosa_q_linear_search(&client_ptr->pend_req_queue, qcm_mqtt_client_compare_ping_type, &type);
    if (mqtt_request != QOSA_NULL)
    {
        return QOSA_FALSE;
    }

    //The timer may execute a bit earlier (QCM_MQTT_TIMEOUT_BEFORE)
    //now is the actual system time currently executed by the timer
    //send_last_time + (kalive_time * 1000) is the time elapsed after the last MQTT packet was sent, after kalive_time seconds
    //When the current time exceeds send_last_time + (kalive_time * 1000) - QCM_MQTT_TIMEOUT_BEFORE, it is ready to send an MQTT PING.
    //    now       >=   send_last_time + (kalive_time * 1000) - QCM_MQTT_TIMEOUT_BEFORE
    //Otherwise, it is not needed, return QOSA_FALSE.
    kalive_time_ms = client_ptr->user_option.kalive_time * 1000;
    QLOGV(
        "now: %lld, send_last_time: %lld, now - send_last_time = %lld, kalive_time: %u",
        now,
        client_ptr->send_last_time,
        now - client_ptr->send_last_time,
        kalive_time_ms
    );

    if (now - client_ptr->send_last_time <  (kalive_time_ms - QCM_MQTT_TIMEOUT_BEFORE))
    {
        //The current time has not yet reached send_last_time + kalive_time - QCM_MQTT_TIMEOUT_BEFORE, no need to send a ping packet.
        //The interval between now and the last MQTT packet transmission action has not yet reached client_ptr->user_option.kalive_time * 1000 - QCM_MQTT_TIMEOUT_BEFORE, so there is no need to send a PING packet.
        QLOGV("now - send_last_time < kalive_time - QCM_MQTT_TIMEOUT_BEFORE");
        return QOSA_FALSE;
    }

    return QOSA_TRUE;
}

static qcm_mqtt_eercode_e qcm_mqtt_client_create_request_ping(qcm_mqtt_client_t *client_ptr)
{
    mqtt_packet         pingreq_pkt;
    qcm_mqtt_request_t *req_rb = QOSA_NULL;

    pingreq_pkt.header.byte = 0;
    pingreq_pkt.header.bits.type = MQTT_PINGREQ;

    req_rb = (qcm_mqtt_request_t *)qosa_malloc(sizeof(qcm_mqtt_request_t));
    if (req_rb == QOSA_NULL)
    {
        return QCM_MQTT_RES_ERROR_NO_MEMORY;
    }

    qosa_memset(req_rb, 0, sizeof(qcm_mqtt_request_t));
    if (MQTTPACKET_READ_COMPLETE != qcm_mqtt_serialize_zero(&(req_rb->buffer), &pingreq_pkt))
    {
        qosa_free(req_rb);
        return QCM_MQTT_RES_ERROR_PACKET;
    }
    else
    {
        req_rb->header.byte = pingreq_pkt.header.byte;
        req_rb->msg_id = 0;
    }

    req_rb->expire_time = client_ptr->send_last_time + client_ptr->user_option.kalive_time * 1000;
    qosa_q_link(req_rb, &req_rb->link);
    qosa_q_put(&client_ptr->out_packet_queue, &req_rb->link);

    qcm_mqtt_client_report_clientid(QCM_MQTT_SEND_EVENT, client_ptr, QCM_MQTT_RES_OK);
    return QCM_MQTT_RES_OK;
}

/**
 * @brief Find the minimum interval compared to the current time in the linked list
 */
static void qcm_mqtt_client_set_min_diff_time(qcm_mqtt_client_t *client_ptr)
{
    qcm_mqtt_request_t *rb = QOSA_NULL;
    qcm_mqtt_request_t *tmp = QOSA_NULL;
    qosa_time_t         cur_time = 0;
    qosa_time_t         min_time_diff = 0;  //Minimum time difference

    //First, handle the messages that have already timed out.
    rb = qosa_q_check(&client_ptr->pend_req_queue);
    while (rb != QOSA_NULL)
    {
        if (rb->expire_time <= qosa_get_system_time_milliseconds() + QCM_MQTT_TIMEOUT_BEFORE)
        {
            QLOGI("msg_type:%d", rb->header.bits.type);
            switch (rb->header.bits.type)
            {
                case MQTT_CONNECT:
                case MQTT_PINGREQ: {
                    if (rb->header.bits.type == MQTT_CONNECT)
                    {
                        client_ptr->close_cause = QCM_MQTT_CLOSE_BY_CONN_TIMEOUT;
                        qcm_mqtt_client_response_commone(client_ptr, QCM_MQTT_CLIENT_CONNECT_EVENT, QCM_MQTT_RES_ERROR_CONNECT_TIMEOUT, QOSA_NULL);
                        qcm_mqtt_client_socket_evt_cb(client_ptr->socketfd, QCM_SOCK_CLOSE_EVENT, QCM_SOCK_TIMEROUT, client_ptr);
                    }
                    else
                    {
                        client_ptr->close_cause = QCM_MQTT_CLOSE_BY_PING_TIMEOUT;
                        int                 close_time = client_ptr->user_option.close_ping_interval;
                        qcm_socket_linger_t linger = {.on_off = 1, .linger_val = close_time};

                        qcm_socket_set_opt(client_ptr->socketfd, QCM_SOCK_LINGER_OPT, (void *)&linger);
                        qcm_mqtt_client_response_commone(client_ptr, QCM_MQTT_CLIENT_PING_EVENT, QCM_MQTT_RES_ERROR_TIMEOUT, QOSA_NULL);
                        qcm_mqtt_client_close_cause(client_ptr);

                        client_ptr->client_state = QCM_MQTT_STATE_TCP_CLOSING;

                        qcm_socket_shutdown(client_ptr->socketfd, QCM_SHUT_RDWR);
                    }

                    tmp = qosa_q_next(&client_ptr->pend_req_queue, &rb->link);
                    qosa_q_delete(&client_ptr->pend_req_queue, &rb->link);
                    qcm_mqtt_free_data(&rb->buffer.buf_ptr);
                    qosa_free(rb);
                    rb = tmp;
                }
                break;

                case MQTT_SUBSCRIBE: {
                    int                  retry_cnt = client_ptr->user_option.delivery_cnt;
                    qcm_mqtt_send_resp_t send_pkt = {0};

                    tmp = qosa_q_next(&client_ptr->pend_req_queue, &rb->link);
                    qosa_q_delete(&client_ptr->pend_req_queue, &rb->link);

                    send_pkt.msg_id = rb->msg_id;

                    if (rb->retry_cnt > 0)
                    {
                        qosa_q_put(&client_ptr->out_packet_queue, &rb->link);
                        qcm_mqtt_client_report_clientid(QCM_MQTT_SEND_EVENT, client_ptr, QCM_MQTT_RES_OK);
                        rb->retry_cnt--;
                        retry_cnt -= rb->retry_cnt;  //Retry count starts from 1 and increases

                        send_pkt.send_result = QCM_MQTT_SEND_RETRY;
                        send_pkt.retry_count = retry_cnt;
                    }
                    else
                    {
                        send_pkt.send_result = QCM_MQTT_SEND_ERROR;
                        qcm_mqtt_free_data(&rb->buffer.buf_ptr);
                        qosa_free(rb);
                    }

                    rb = tmp;
                    qcm_mqtt_client_response_commone(client_ptr, QCM_MQTT_CLIENT_SUBSCRIBE_EVENT, QCM_MQTT_RES_OK, &send_pkt);
                }
                break;

                case MQTT_UNSUBSCRIBE:
                case MQTT_PUBLISH:
                case MQTT_PUBREL: {
                    int                     type = rb->header.bits.type;
                    qcm_mqtt_send_resp_t    send_pkt = {0};
                    int                     retry_cnt = client_ptr->user_option.delivery_cnt;
                    qcm_mqtt_client_event_e event_type = QCM_MQTT_CLIENT_UNSUBSCRIBE_EVENT;

                    tmp = qosa_q_next(&client_ptr->pend_req_queue, &rb->link);
                    qosa_q_delete(&client_ptr->pend_req_queue, &rb->link);

                    send_pkt.msg_id = rb->msg_id;
                    if (type == MQTT_PUBLISH || type == MQTT_PUBREL)
                    {
                        event_type = QCM_MQTT_CLIENT_PUBLISH_EVENT;
                    }

                    if (rb->retry_cnt > 0)
                    {
                        qosa_q_put(&client_ptr->out_packet_queue, &rb->link);
                        qcm_mqtt_client_report_clientid(QCM_MQTT_SEND_EVENT, client_ptr, QCM_MQTT_RES_OK);
                        rb->retry_cnt--;
                        retry_cnt -= rb->retry_cnt;  //Retry count starts from 1 and increases

                        send_pkt.send_result = QCM_MQTT_SEND_RETRY;
                        send_pkt.retry_count = retry_cnt;
                    }
                    else
                    {
                        send_pkt.send_result = QCM_MQTT_SEND_ERROR;
                        qcm_mqtt_free_data(&rb->buffer.buf_ptr);
                        qosa_free(rb);
                    }
                    rb = tmp;

                    qcm_mqtt_client_response_commone(client_ptr, event_type, QCM_MQTT_RES_OK, &send_pkt);
                }
                break;

                default:
                    //These messages should not enter the pend list; if they do, it is a code logic error.
                    QLOGE("MQTT  PROTOCOL ERROR  !!!");
                    break;
            }
        }
        else
        {
            rb = qosa_q_next(&client_ptr->pend_req_queue, &rb->link);
        }
    }

    //After all checks are completed, traverse the query to obtain the minimum time.
    cur_time = qosa_get_system_time_milliseconds();
    //If the MQTT connection is successful, check the MQTT heartbeat timeout.

    QLOGV(
        "client_state: %d, now: %ld, send_last_time: %ld, kalive_time: %ld",
        client_ptr->client_state,
        cur_time,
        client_ptr->send_last_time,
        client_ptr->user_option.kalive_time * 1000
    );

    if (client_ptr->client_state == QCM_MQTT_STATE_MQTT_CONNECTED && client_ptr->user_option.kalive_time != 0
        && qcm_mqtt_client_check_request_ping(client_ptr, cur_time) == QOSA_TRUE)
    {
        qcm_mqtt_client_create_request_ping(client_ptr);
    }

    //The timer can only be started after a successful connection; it cannot be started if the connection is disconnected or if a ping timeout occurs.
    if (client_ptr->client_state == QCM_MQTT_STATE_MQTT_CONNECTED || client_ptr->client_state == QCM_MQTT_STATE_MQTT_CONNECTING)
    {
        rb = qosa_q_check(&client_ptr->pend_req_queue);
        if (rb != QOSA_NULL)
        {
            min_time_diff = rb->expire_time;

            while (rb != QOSA_NULL)
            {
                min_time_diff = MIN(min_time_diff, rb->expire_time);
                rb = qosa_q_next(&client_ptr->pend_req_queue, &rb->link);
            }

            min_time_diff -= cur_time;
        }
        else
        {
            min_time_diff = client_ptr->user_option.kalive_time * 1000 - (cur_time - client_ptr->send_last_time);
        }

        QLOGV("min_time_diff=%ld", min_time_diff);
        if (min_time_diff > 0)
        {
            if (qosa_timer_is_running(client_ptr->timer) == QOSA_TRUE)
            {
                QLOGV("MQTT timer stop!");
                qosa_timer_stop(client_ptr->timer);
            }

            QLOGV("MQTT timer start!");
            if (qosa_timer_start(client_ptr->timer, min_time_diff, 0) != QCM_MQTT_RES_OK)
            {
                QLOGV("MQTT timer start failed!");
            }
        }
        else
        {
            qosa_timer_stop(client_ptr->timer);
        }
    }
}

/**
 * @brief Traverse and check if there are any timeout messages in the current pending queue
 */
static void qcm_mqtt_client_cyclic_timeout_handler(void *argv)
{
    qcm_mqtt_clientid_info_t *clientid_info_ptr = (qcm_mqtt_clientid_info_t *)argv;
    qcm_mqtt_client_t        *client_ptr = QOSA_NULL;

    client_ptr = qcm_mqtt_client_get_ptr(clientid_info_ptr->client_id);
    if (client_ptr == QOSA_NULL)
    {
        return;
    }

    //Get Minimum Time Difference
    qcm_mqtt_client_set_min_diff_time(client_ptr);
}

/**
 * @brief MQTT internal minimum timer timeout event notification, sends a timeout notification to the MQTT service layer task, and the MQTT task handles the timeout exception.
 */
static void qcm_mqtt_client_timeout_cb(void *argv)
{
    qcm_mqtt_client_t *client_ptr = (qcm_mqtt_client_t *)argv;
    qcm_mqtt_client_report_clientid(QCM_MQTT_CYCLIC_TIME_EVENT, client_ptr, QCM_MQTT_RES_OK);
}
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
/**
 * @brief Context: The PDP manager task needs to send a message to the TCP/IP APP task for processing by the APP task.
 */
static void qcm_mqtt_client_datacall_cb(void *user_param, qosa_datacall_act_cnf_t *argv)
{
    qcm_mqtt_clientid_info_t *clientid_info_ptr = (qcm_mqtt_clientid_info_t *)user_param;
    qcm_mqtt_pdp_resp_t      *pdp_resp_ptr = QOSA_NULL;

    if (qcm_mqtt_client_check_id_is_exist(clientid_info_ptr->client_id) == QOSA_FALSE)
    {
        goto exit;
    }

    pdp_resp_ptr = (qcm_mqtt_pdp_resp_t *)qosa_malloc(sizeof(qcm_mqtt_pdp_resp_t));
    if (pdp_resp_ptr == QOSA_NULL)
    {
        goto exit;
    }

    QLOGI("datacall cb status: %d", pdp_resp_ptr->act_info.status);

    qosa_memcpy(&pdp_resp_ptr->act_info, argv, sizeof(qosa_datacall_act_cnf_t));
    pdp_resp_ptr->client_id = clientid_info_ptr->client_id;

    if (qcm_mqtt_client_event_report(QCM_MQTT_DATACALL_RESP_EVENT, pdp_resp_ptr) != QCM_MQTT_RES_OK)
    {
        qosa_free(pdp_resp_ptr);
    }

exit:
    qosa_free(clientid_info_ptr);
    return;
}
#endif
/**
 * @brief Check if the corresponding MQTT client is already in a protocol-connected state.
 */
static qosa_bool_t qcm_mqtt_client_is_protocol_conn_success(qosa_uint8_t client_id)
{
    qcm_mqtt_client_t *client_ptr = QOSA_NULL;

    //Perform state judgment here; do not leave state judgment to the user's control. If the call is not in a connected success state, it should directly return failure.
    qosa_mutex_lock(g_mqtt_context.mutex, QOSA_WAIT_FOREVER);
    client_ptr = qcm_mqtt_client_get_ptr(client_id);
    if (client_ptr == QOSA_NULL)
    {
        qosa_mutex_unlock(g_mqtt_context.mutex);
        return QOSA_FALSE;
    }

    if (client_ptr->client_state != QCM_MQTT_STATE_MQTT_CONNECTED)
    {
        qosa_mutex_unlock(g_mqtt_context.mutex);
        return QOSA_FALSE;
    }
    qosa_mutex_unlock(g_mqtt_context.mutex);

    return QOSA_TRUE;
}

#ifdef CONFIG_QCM_VTLS_FUNC
/**
 * @brief
 */
static void qcm_mqtt_client_ssl_shakehand_handler(void *argv)
{
    qcm_mqtt_eercode_e        result = QCM_MQTT_RES_OK;
    qcm_mqtt_clientid_info_t *clientid_info_ptr = (qcm_mqtt_clientid_info_t *)argv;
    qcm_mqtt_client_t        *client_ptr = QOSA_NULL;

    client_ptr = qcm_mqtt_client_get_ptr(clientid_info_ptr->client_id);
    if (client_ptr == QOSA_NULL)
    {
        return;
    }

    QLOGV("client_state : %d", client_ptr->client_state);

    if (!(client_ptr->client_state == QCM_MQTT_STATE_TCP_CONNECTING || client_ptr->client_state == QCM_MQTT_STATE_SSL_HANDSHAKE))
    {
        result = QCM_MQTT_RES_ERROR_SSL_HS_FAIL;
        goto exit;
    }

    if (qcm_socket_ssl_is_init_already(client_ptr->socketfd) == QOSA_FALSE)
    {
        if (qcm_socket_ssl_config(client_ptr->socketfd, client_ptr->user_option.ssl_config, client_ptr->server_addr, QOSA_FALSE) != QCM_SOCK_SUCCESS)
        {
            result = QCM_MQTT_RES_ERROR_SSL_HS_FAIL;
        }
    }

    if (result == QCM_MQTT_RES_OK)
    {
        qcm_sock_err_code ret = qcm_socket_ssl_connect(client_ptr->socketfd);
        QLOGV("ssl ret=%d", ret);

        if (ret != QCM_SOCK_SUCCESS)
        {
            if (ret == QCM_SOCK_WODBLOCK)
            {
                client_ptr->client_state = QCM_MQTT_STATE_SSL_HANDSHAKE;
                //SSL handshake not completed, need to continue waiting
                QLOGV("ssl_handshark continue!!!");
            }
            else
            {
                QLOGV("ssl_handshark error!!!");
                client_ptr->client_state = QCM_MQTT_STATE_TCP_CLOSING;
                result = QCM_MQTT_RES_ERROR_SSL_HS_FAIL;
                goto exit;
            }
        }
        else
        {
            client_ptr->client_state = QCM_MQTT_STATE_TCP_CONNECTED;
            QLOGV("ssl_handshark success!!!");
            result = QCM_MQTT_RES_OK;
            goto exit;
        }
    }

    return;
exit:
    qcm_mqtt_client_response_commone(client_ptr, QCM_MQTT_CLIENT_OPEN_EVENT, result, QOSA_NULL);
    if (result != QCM_MQTT_RES_OK)
    {
        qcm_socket_linger_t linger = {.on_off = 1, .linger_val = 0};
        qcm_socket_set_opt(client_ptr->socketfd, QCM_SOCK_LINGER_OPT, (void *)&linger);

        qcm_socket_close(client_ptr->socketfd);
        //Directly release all resources
        qcm_mqtt_client_free(client_ptr);
    }
}
#endif

/**
 * @brief Save the add information passed by the OPEN API interface, while checking the network and initiating a network registration request.
 */
static void qcm_mqtt_client_open_handler(void *argv)
{
    qcm_mqtt_eercode_e    result = QCM_MQTT_RES_OK;
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
    qosa_datacall_errno_e datacall_state = QOSA_DATACALL_OK;
#endif
    qcm_mqtt_open_param_t  *open_param_ptr = (qcm_mqtt_open_param_t *)argv;
    qcm_mqtt_client_t      *client_ptr = QOSA_NULL;
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
    qosa_datacall_ip_info_t info = {0};
    qosa_datacall_conn_t    conn = 0;
#endif
    client_ptr = qcm_mqtt_client_get_ptr(open_param_ptr->client_id);
    if (client_ptr == QOSA_NULL)
    {
        return;
    }

    if (client_ptr->client_state != QCM_MQTT_STATE_INIT)
    {
        result = QCM_MQTT_RES_ERROR_STATUS;
        goto exit;
    }

    qosa_memcpy(client_ptr->server_addr, open_param_ptr->addr, qosa_strlen(open_param_ptr->addr));
    client_ptr->server_port = open_param_ptr->port;
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
    qosa_memset(&info, 0, sizeof(qosa_datacall_ip_info_t));

    conn = qosa_datacall_conn_new(client_ptr->user_option.sim_id, client_ptr->user_option.pdp_cid, QOSA_DATACALL_CONN_TCPIP);
    datacall_state = qosa_datacall_get_ip_info(conn, &info);
    QLOGI("sim_id %d pdp_cid %d", client_ptr->user_option.sim_id, client_ptr->user_option.pdp_cid);
    QLOGV("datacall status %d ", datacall_state);

    if (datacall_state == QOSA_DATACALL_ERR_NO_ACTIVE)
    {
        qcm_mqtt_clientid_info_t *clientid_info_ptr = QOSA_NULL;

        clientid_info_ptr = (qcm_mqtt_clientid_info_t *)qosa_malloc(sizeof(qcm_mqtt_clientid_info_t));
        if (clientid_info_ptr == QOSA_NULL)
        {
            result = QCM_MQTT_RES_ERROR_NO_MEMORY;
            goto exit;
        }
        qosa_memset(clientid_info_ptr, 0, sizeof(qcm_mqtt_clientid_info_t));
        clientid_info_ptr->client_id = client_ptr->client_id;

        //PDP not activated, starting activation procedure
        if (qosa_datacall_start_async(conn, client_ptr->user_option.connect_time, (datacall_callback_cb_ptr)qcm_mqtt_client_datacall_cb, clientid_info_ptr)
            != QOSA_DATACALL_OK)
        {
            result = QCM_MQTT_RES_ERROR_DATACALL;
            qosa_free(clientid_info_ptr);
            goto exit;
        }
        client_ptr->client_state = QCM_MQTT_STATE_DATACALL_ING;
    }
    else
#endif
    {
        //PDP is activated, determine whether the address version entered by the user is the same as the address version obtained after the current PDP activation.
        int                 server_ip_info = 0;
        qcm_mqtt_event_id_e event = QCM_MQTT_SOCKET_CREATE_EVENT;

        server_ip_info = qcm_socket_checkip_is_ip46(client_ptr->server_addr);
        if (server_ip_info == 0)
        {
            //The server address entered is a domain name, so first request DNS directly.
            client_ptr->client_state = QCM_MQTT_STATE_DNS_QUERY;
            event = QCM_MQTT_DNS_REQUEST_EVENT;
        }
        else
        {
#if (defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
            if (server_ip_info == QCM_AF_INET6)
#else
            if ((server_ip_info == QCM_AF_INET6) && ((info.ip_type == QOSA_PDP_IPV6) || (info.ip_type == QOSA_PDP_IPV4V6)))
#endif
            {
                client_ptr->ai_family = QCM_AF_INET6;
                inet_pton(QCM_AF_INET6, client_ptr->server_addr, &client_ptr->server_ip.addr.ipv6_addr);
#if (defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
                client_ptr->server_ip.ip_vsn = QOSA_IP_V6;
#else
                client_ptr->server_ip.ip_vsn = QOSA_PDP_IPV6;
#endif
            }
#if (defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
            else if (server_ip_info == QCM_AF_INET)
#else
            else if ((server_ip_info == QCM_AF_INET) && ((info.ip_type == QOSA_PDP_IPV4) || (info.ip_type == QOSA_PDP_IPV4V6)))
#endif
            {
                client_ptr->ai_family = QCM_AF_INET;
                inet_pton(QCM_AF_INET, client_ptr->server_addr, &client_ptr->server_ip.addr.ipv4_addr);
#if (defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
                client_ptr->server_ip.ip_vsn = QOSA_IP_V4;
#else
                client_ptr->server_ip.ip_vsn = QOSA_PDP_IPV4;
#endif
            }
            else
            {
                result = QCM_MQTT_RES_ERROR_IP_VERSION;
                goto exit;
            }
        }
        qcm_mqtt_client_report_clientid(event, client_ptr, result);
    }

    return;
exit:
    QLOGV("qcm_mqtt_client_open_handler fail!");
    qcm_mqtt_client_response_commone(client_ptr, QCM_MQTT_CLIENT_OPEN_EVENT, result, QOSA_NULL);
    if (result != QCM_MQTT_RES_OK)
    {
        //Directly release all resources
        qcm_mqtt_client_free(client_ptr);
    }
}
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
/**
 * @brief Process datacall activation status notification. This function handles the MQTT client's data call activation status response. Based on the incoming activation information, it determines
 *        Whether to perform DNS queries or set the client's IP information.
 *
 * @param[in] void *argv
 *           - Pointer to the qcm_mqtt_pdp_resp_t structure, containing detailed information about the activation status.
 */
static void qcm_mqtt_client_datacall_resp_handler(void *argv)
{
    qcm_mqtt_eercode_e   result = QCM_MQTT_RES_OK;
    qcm_mqtt_pdp_resp_t *pdp_resp_ptr = (qcm_mqtt_pdp_resp_t *)argv;
    qcm_mqtt_client_t   *client_ptr = QOSA_NULL;

    client_ptr = qcm_mqtt_client_get_ptr(pdp_resp_ptr->client_id);
    if (client_ptr == QOSA_NULL)
    {
        MQTT_FREE_PTR(pdp_resp_ptr);
        return;
    }

    if (pdp_resp_ptr->act_info.opt == QOSA_PDP_OPT_ACTIVE)
    {
        if (pdp_resp_ptr->act_info.status == QOSA_TRUE)
        {
            qcm_mqtt_event_id_e      event = QCM_MQTT_SOCKET_CREATE_EVENT;
            qosa_datacall_ip_info_t *ip_info = &pdp_resp_ptr->act_info.ip_info;
            int                      server_ip_info = 0;

            server_ip_info = qcm_socket_checkip_is_ip46(client_ptr->server_addr);
            if (server_ip_info == 0)
            {
                client_ptr->client_state = QCM_MQTT_STATE_DNS_QUERY;
                event = QCM_MQTT_DNS_REQUEST_EVENT;
            }
            else
            {
                if ((server_ip_info == QCM_AF_INET6) && ((ip_info->ip_type == QOSA_PDP_IPV6) || (ip_info->ip_type == QOSA_PDP_IPV4V6)))
                {
                    client_ptr->ai_family = QCM_AF_INET6;
                    inet_pton(QCM_AF_INET6, client_ptr->server_addr, &client_ptr->server_ip.addr.ipv6_addr);
                    client_ptr->server_ip.ip_vsn = QOSA_PDP_IPV6;
                }
                else if ((server_ip_info == QCM_AF_INET) && ((ip_info->ip_type == QOSA_PDP_IPV4) || (ip_info->ip_type == QOSA_PDP_IPV4V6)))
                {
                    client_ptr->ai_family = QCM_AF_INET;
                    inet_pton(QCM_AF_INET, client_ptr->server_addr, &client_ptr->server_ip.addr.ipv4_addr);
                    client_ptr->server_ip.ip_vsn = QOSA_PDP_IPV4;
                }
                else
                {
                    result = QCM_MQTT_RES_ERROR_IP_VERSION;
                    goto exit;
                }
            }
            qcm_mqtt_client_report_clientid(event, client_ptr, result);
        }
        else
        {
            result = QCM_MQTT_RES_ERROR_DATACALL;
            goto exit;
        }
    }
    else
    {
        QLOGE("PDP_OPT_DEACTIVE!");
        result = QCM_MQTT_RES_ERROR_DATACALL;
        goto exit;
    }

    MQTT_FREE_PTR(pdp_resp_ptr);
    return;
exit:
    MQTT_FREE_PTR(pdp_resp_ptr);
    QLOGI("qcm_mqtt_client_open_handler fail!");
    qcm_mqtt_client_response_commone(client_ptr, QCM_MQTT_CLIENT_OPEN_EVENT, result, QOSA_NULL);
    if (result != QCM_MQTT_RES_OK)
    {
        //Directly release all resources
        qcm_mqtt_client_free(client_ptr);
    }
}

/**
 * @brief When the PDP (Packet Data Protocol) activation status is deactivated, handle the shutdown operation of the MQTT client associated with that PDP CID.
 *
 * @param[in] qcm_mqtt_client_t *client_ptr
 *           - Pointer to the network deactivation client.
 */
static void qcm_mqtt_client_pdp_deacted_handler(qcm_mqtt_client_t *client_ptr)
{
    QLOGV("mqtt No.%d is about to be shut down due to pdp deact!", client_ptr->client_id);

    qcm_socket_close(client_ptr->socketfd);
    //1: Connection disconnected or reset by the server; the newly added error code (QCM_MQTT_CLOSE_BY_PDP_DEACTED:12) currently has little significance, keep it consistent with the documentation/case
    client_ptr->close_cause = QCM_MQTT_CLOSE_BY_PEER_RESET;

    qcm_mqtt_client_close_cause(client_ptr);
    qcm_mqtt_client_free(client_ptr);
}
#endif
/**
 * @brief MQTT client DNS query result callback function. This static function serves as the callback entry point after the MQTT client DNS resolution is completed, responsible for handling the DNS query result.
 *
 * @param[in] void *argv Reserved parameter for the callback function, used to pass specific identifiers of the MQTT client (such as client ID and other information).
 * @param[out] struct qosa_addrinfo_s **info A pointer to a pointer of the struct osa_addrinfo_s, used to receive the set of address information obtained from the DNS query, which includes IP address, port number, and other details.
 * @param[in] qosa_dns_error_e status DNS query status, indicating the success or failure of the query and possible error conditions. The specific status can be determined via the enumeration qosa_dns_error_e.
 */
static void qcm_mqtt_client_dns_result_cb(void *argv, struct qosa_addrinfo_s **info, qosa_dns_error_e status)
{
    qcm_mqtt_dns_resp_t      *dns_ptr = QOSA_NULL;
    qcm_mqtt_clientid_info_t *clientid_info_ptr = (qcm_mqtt_clientid_info_t *)argv;

    if (qcm_mqtt_client_check_id_is_exist(clientid_info_ptr->client_id) == QOSA_FALSE)
    {
        goto exit;
    }

    dns_ptr = (qcm_mqtt_dns_resp_t *)qosa_malloc(sizeof(qcm_mqtt_dns_resp_t));
    if (dns_ptr == QOSA_NULL)
    {
        goto exit;
    }

    dns_ptr->client_id = clientid_info_ptr->client_id;
    if (status != QOSA_DNS_RESULT_OK)
    {
        dns_ptr->info = QOSA_NULL;
    }
    else
    {
        dns_ptr->info = *info;
    }
    dns_ptr->status = status;

    if (qcm_mqtt_client_event_report(QCM_MQTT_DNS_RESPONSE_EVENT, dns_ptr) != QCM_MQTT_RES_OK)
    {
        qosa_free(dns_ptr);
    }
exit:
    qosa_free(clientid_info_ptr);
    return;
}

/**
 * @brief Handle DNS resolution
 */
static void qcm_mqtt_client_dns_parse_handler(void *argv)
{
    qcm_mqtt_eercode_e        result = QCM_MQTT_RES_OK;
    struct qosa_addrinfo_s    addrinfo_list = {0};
    qcm_mqtt_clientid_info_t *callback_ptr = QOSA_NULL;
    qcm_mqtt_clientid_info_t *clientid_info_ptr = (qcm_mqtt_clientid_info_t *)argv;
    qcm_mqtt_client_t        *client_ptr = QOSA_NULL;

    client_ptr = qcm_mqtt_client_get_ptr(clientid_info_ptr->client_id);
    if (client_ptr == QOSA_NULL)
    {
        MQTT_FREE_PTR(clientid_info_ptr);
        return;
    }

    callback_ptr = (qcm_mqtt_clientid_info_t *)qosa_malloc(sizeof(qcm_mqtt_clientid_info_t));
    if (callback_ptr == QOSA_NULL)
    {
        result = QCM_MQTT_RES_ERROR_NO_MEMORY;
        goto exit;
    }
    callback_ptr->client_id = clientid_info_ptr->client_id;

    if (qosa_dns_asyn_getaddrinfo(
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
            client_ptr->user_option.sim_id,
            client_ptr->user_option.pdp_cid,
#endif
            client_ptr->server_addr,
            &addrinfo_list,
            qcm_mqtt_client_dns_result_cb,
            (void *)callback_ptr
        )!= QOSA_DNS_RESULT_OK)
    {
        qosa_free(callback_ptr);
        result = QCM_MQTT_RES_ERROR_DNS; /*! Report ERROR, exit*/
    }

exit:
    QLOGI("remote add=%s result=%d ", client_ptr->server_addr, result);
    MQTT_FREE_PTR(clientid_info_ptr);

    if (result != QCM_MQTT_RES_OK)
    {
        qcm_mqtt_client_response_commone(client_ptr, QCM_MQTT_CLIENT_OPEN_EVENT, result, QOSA_NULL);
        //Directly release all resources
        qcm_mqtt_client_free(client_ptr);
    }
}

/**
 * @brief Processing DNS result return
 */
static void qcm_mqtt_client_dns_resp_handler(void *argv)
{
    qcm_mqtt_dns_resp_t *dns_ptr = (qcm_mqtt_dns_resp_t *)argv;
    qcm_mqtt_client_t   *client_ptr = QOSA_NULL;

    client_ptr = qcm_mqtt_client_get_ptr(dns_ptr->client_id);
    if (client_ptr == QOSA_NULL)
    {
        if (dns_ptr->info != QOSA_NULL && dns_ptr->status == QOSA_DNS_RESULT_OK)
        {
            qosa_dns_result_free(dns_ptr->info);
        }
        MQTT_FREE_PTR(dns_ptr);
        return;
    }

    if (dns_ptr->info == QOSA_NULL || dns_ptr->status != QOSA_DNS_RESULT_OK)
    {
        qcm_mqtt_client_response_commone(client_ptr, QCM_MQTT_CLIENT_OPEN_EVENT, QCM_MQTT_RES_ERROR_DNS, QOSA_NULL);
        MQTT_FREE_PTR(dns_ptr);
        //Directly release all resources
        qcm_mqtt_client_free(client_ptr);
    }
    else
    {
        //After obtaining the IP address via DNS, start creating the TCP connection.
        QLOGV("ip_addr:%s", dns_ptr->info->ip_addr);
        if (dns_ptr->info->ai_family == QCM_AF_INET)
        {
            inet_pton(QCM_AF_INET, dns_ptr->info->ip_addr, &client_ptr->server_ip.addr.ipv4_addr);
#if (defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
            client_ptr->server_ip.ip_vsn = QOSA_IP_V4;
#else
            client_ptr->server_ip.ip_vsn = QOSA_PDP_IPV4;
#endif
        }
        else
        {
            inet_pton(QCM_AF_INET6, dns_ptr->info->ip_addr, &client_ptr->server_ip.addr.ipv6_addr);
#if (defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
            client_ptr->server_ip.ip_vsn = QOSA_IP_V6;
#else
            client_ptr->server_ip.ip_vsn = QOSA_PDP_IPV6;
#endif
        }
        client_ptr->ai_family = dns_ptr->info->ai_family;
        QLOGV("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!ai_family=%d", dns_ptr->info->ai_family);

        qcm_mqtt_client_report_clientid(QCM_MQTT_SOCKET_CREATE_EVENT, client_ptr, QCM_MQTT_RES_OK);
        //Release DNS address memory space
        qosa_dns_result_free(dns_ptr->info);
        MQTT_FREE_PTR(dns_ptr);
    }
}

/**
 *  @brief MQTT Subscription Topic Clear Topic List Function
 *
 * @param[in] qcm_mqtt_unsub_t * unsub_info
 *           -PQTT subscription information structure pointer
 *
 * @return qcm_mqtt_eercode_e
 *        - Returns MQTT_RES_OK to indicate success.
 *        - Returns other values from mqtt_eercode_e to indicate failure.
 */
qcm_mqtt_eercode_e qcm_mqtt_sub_data_free(qcm_mqtt_sub_param_t *sub_info)
{
    if (sub_info == QOSA_NULL)
    {
        return QCM_MQTT_RES_ERROR_PARAM_INVALID;
    }

    if (sub_info->topic_list.mutex == QOSA_NULL)
    {
        goto exit;
    }

    for (; qosa_q_cnt(&(sub_info->topic_list)) != 0;)
    {
        qcm_mqtt_sub_topic_t *topic_ptr = QOSA_NULL;
        topic_ptr = (qcm_mqtt_sub_topic_t *)qosa_q_get(&(sub_info->topic_list));
        qosa_free(topic_ptr->topic.data_ptr);
        qosa_free(topic_ptr);
    }
    qosa_q_destroy(&(sub_info->topic_list));

exit:
#ifdef CONFIG_QCM_MQTT5_FUNC
    qcm_mqtt_properties_free(&sub_info->props);
#endif /* CONFIG_QCM_MQTT5_FUNC */

    qosa_free(sub_info);
    return QCM_MQTT_RES_OK;
}

static qcm_mqtt_eercode_e qcm_mqtt_pub_data_free(qcm_mqtt_pub_param_t *pub_info)
{
    if (pub_info == QOSA_NULL)
    {
        return QCM_MQTT_RES_OK;
    }

    qcm_mqtt_free_data(&pub_info->topic);
    qcm_mqtt_free_data(&pub_info->payload);

#ifdef CONFIG_QCM_MQTT5_FUNC
    QLOGV("qcm_mqtt_pub_data_free %p", &pub_info->props);
    qcm_mqtt_properties_free(&pub_info->props);
#endif /* CONFIG_QCM_MQTT5_FUNC */

    qosa_free(pub_info);
    return QCM_MQTT_RES_OK;
}

/**
 * @brief Get the last shutdown reason for each MQTT channel
 *
 * @param[in] int client_id
 *          - MQTT corresponding client ID
 */
int qcm_mqtt_get_last_close_event(int client_id)
{
    int close_event = 0;

    if (client_id < 0 || client_id > QCM_MQTT_MAX_CLIENT_CNT - QCM_MQTT_CLIENT_ID_OFFSET)
    {
        return -1;
    }

    QLOGI("socket_id: %d", client_id);

    close_event = g_mqtt_last_close_event[client_id];
    QLOGI("close_event: %d", close_event);

    //1 CLOSE_EVENT_FIN: TCP connection disconnection event caused by the server actively sending a FIN packet.
    //2 CLOSE_EVENT_RST: During the SYN handshake, the server actively sends an RST / After the connection is established, the server actively sends an RST.
    //3 CLOSE_EVENT_SYN_TIMEOUT: TCP connection closed due to internal SYN retransmission timeout event
    //4 CLOSE_EVENT_ACK_TIMEOUT: TCP connection disconnection event caused by internal ACK retransmission timeout

    if (close_event > 0 && close_event < 3)
    {
        //1 2 The close event is caused by the server sending FIN or RST, which is not necessarily a problem and should not be reported.
        //Only report shutdown events caused by the module (3, 4)
        close_event = 0;
    }

    return close_event;
}

/**
 * @brief Initialize the shutdown reason for a specified MQTT channel
 *
 * @param[in] int client_id
 *          - MQTT corresponding client id
 */
void qcm_mqtt_last_close_event_init(int client_id)
{
    if (client_id < 0 || client_id > QCM_MQTT_MAX_CLIENT_CNT - QCM_MQTT_CLIENT_ID_OFFSET)
    {
        return;
    }

    g_mqtt_last_close_event[client_id] = 0;
}

/**
 * @brief After DNS resolution is successful and the IP address is obtained, the TCP creation process begins.
 */
static void qcm_mqtt_client_socket_create_handler(void *argv)
{
    qosa_int32_t              sock_hd = 0;
    qcm_mqtt_eercode_e        ret = QCM_MQTT_RES_OK;
    qcm_sock_err_code         socket_ret = QCM_SOCK_SUCCESS;
    qosa_int32_t              event_mask;
    qcm_mqtt_clientid_info_t *clientid_info_ptr = (qcm_mqtt_clientid_info_t *)argv;
    qcm_mqtt_client_t        *client_ptr = QOSA_NULL;

    client_ptr = qcm_mqtt_client_get_ptr(clientid_info_ptr->client_id);
    if (client_ptr == QOSA_NULL)
    {
        qosa_free(clientid_info_ptr);
        return;
    }

    sock_hd = qcm_socket_create(
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
        client_ptr->user_option.sim_id,
        client_ptr->user_option.pdp_cid,
#endif
        client_ptr->ai_family,
        QCM_SOCK_STREAM,
        QCM_TCP_PROTOCOL,
        0 /*local_port*/,
        QOSA_FALSE
    );
    if (sock_hd <= 0)
    {
        ret = QCM_MQTT_RES_ERROR_SOCKET_CREATE;
        goto exit;
    }

    QLOGI("Socket create successful! sock_hd : %d", sock_hd);

    //qcm_socket_create => socket => lwip_socket => netconn_new_with_proto_and_callback => netconn_alloc
    //In netconn_alloc, conn->last_close_event is set to CLOSE_EVENT_NORMAL, and here the array element g_tcpip_last_close_event corresponding to client_id is also initialized synchronously.
    g_mqtt_last_close_event[client_ptr->client_id] = 0;
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
#ifdef CONFIG_OSA_COMP_MQTT_USE_NET_DOWN_EVENT
    event_mask
        = QCM_SOCK_CONNECT_EVENT | QCM_SOCK_READ_EVENT | QCM_SOCK_WRITE_EVENT | QCM_SOCK_CLOSE_EVENT | QCM_SOCK_SSL_HD_TIMEOUT_EVENT | QCM_SOCK_NET_DOWN_EVENT;
#else  /* CONFIG_OSA_COMP_MQTT_USE_NET_DOWN_EVENT */
    event_mask = QCM_SOCK_CONNECT_EVENT | QCM_SOCK_READ_EVENT | QCM_SOCK_WRITE_EVENT | QCM_SOCK_CLOSE_EVENT | QCM_SOCK_SSL_HD_TIMEOUT_EVENT;
#endif /* CONFIG_OSA_COMP_MQTT_USE_NET_DOWN_EVENT */
#else
    event_mask = QCM_SOCK_CONNECT_EVENT | QCM_SOCK_READ_EVENT | QCM_SOCK_WRITE_EVENT | QCM_SOCK_CLOSE_EVENT | QCM_SOCK_SSL_HD_TIMEOUT_EVENT;
#endif
    qcm_socket_register_event(sock_hd, event_mask, qcm_mqtt_client_socket_evt_cb, (void *)client_ptr);

    socket_ret = qcm_socket_connect(sock_hd, &client_ptr->server_ip, client_ptr->server_port);
    if (socket_ret == QCM_SOCK_SUCCESS)
    {
        QLOGV("CONNECTED this is error,please check");

        client_ptr->client_state = QCM_MQTT_STATE_TCP_CONNECTED;
        client_ptr->socketfd = sock_hd;
    }
    else
    {
        if (QCM_SOCK_WODBLOCK == socket_ret)
        {
            client_ptr->socketfd = sock_hd;
            client_ptr->client_state = QCM_MQTT_STATE_TCP_CONNECTING;
            ret = QCM_MQTT_RES_OK;
        }
        else
        {
            ret = QCM_MQTT_RES_ERROR_TCP_CONNECT;
        }
    }

exit:
    QLOGI("result=%d", ret);
    qosa_free(clientid_info_ptr);

    if (ret != QCM_MQTT_RES_OK)
    {
        qcm_mqtt_client_response_commone(client_ptr, QCM_MQTT_CLIENT_OPEN_EVENT, ret, QOSA_NULL);
        if (sock_hd > 0)
        {
            qcm_socket_close(sock_hd);
        }
        //Directly release all resources
        qcm_mqtt_client_free(client_ptr);
    }
}

/**
 * @brief MQTT Core Protocol Parsing
 */
static qcm_mqtt_eercode_e qcm_mqtt_client_parse_incoming(qcm_mqtt_client_t *client_ptr)
{
    mqtt_packet_errors result = MQTTPACKET_READ_COMPLETE;
    QLOGV("type:%d,remain_len:%d", client_ptr->cur_input_packet.header.bits.type, client_ptr->cur_input_packet.varible_header.remain_len);

    if (QCM_MQTT_STATE_MQTT_CONNECTED != client_ptr->client_state)
    {
        if (!((QCM_MQTT_STATE_MQTT_CONNECTING == client_ptr->client_state) && (MQTT_CONNACK == client_ptr->cur_input_packet.header.bits.type)))
        {
            QLOGE("unexpected type");
            qosa_memset(&client_ptr->cur_input_packet, 0, sizeof(qcm_mqtt_input_info_t));
            return QCM_MQTT_RES_ERROR_STATUS;
        }
    }

    if (client_ptr->cur_input_packet.input_buff == QOSA_NULL)
    {
        client_ptr->cur_input_packet.buf_pos += (1 + client_ptr->cur_input_packet.varible_header.varible_header_len);
    }

    switch (client_ptr->cur_input_packet.header.bits.type)
    {
        case MQTT_CONNACK: {
            mqtt_connack_packet connack_pkt = {0};

            connack_pkt.header.byte = client_ptr->cur_input_packet.header.byte;
            connack_pkt.version = client_ptr->user_option.version;

            result = qcm_mqtt_deserialize_connectack(&(client_ptr->cur_input_packet), &connack_pkt, client_ptr->cur_input_packet.varible_header.remain_len);
            if (result == MQTTPACKET_READ_COMPLETE)
            {
                if (connack_pkt.return_code == MQTT_CONNECTION_ACCEPTED)
                {
                    client_ptr->client_state = QCM_MQTT_STATE_MQTT_CONNECTED;
#ifdef CONFIG_QCM_MQTT5_FUNC
                    if (connack_pkt.options != QOSA_NULL)
                    {
                        client_ptr->connack_props = connack_pkt.options;
                        if (connack_pkt.options->server_keep_alive > 0)
                        {
                            client_ptr->user_option.kalive_time = connack_pkt.options->server_keep_alive;
                        }
                    }
#endif /* CONFIG_QCM_MQTT5_FUNC */
                }
                else
                {
                    client_ptr->close_cause = QCM_MQTT_CLOSE_BY_CONNACK_FAIL;
                }
                QLOGV("return_code:%d", connack_pkt.return_code);
                qcm_mqtt_client_remove_request(client_ptr, MQTT_CONNECT, 0);
                qcm_mqtt_client_response_commone(client_ptr, QCM_MQTT_CLIENT_CONNECT_EVENT, QCM_MQTT_RES_OK, &connack_pkt);
            }
        }
        break;

        case MQTT_PINGRESP: {
            mqtt_packet pingresp_pkt;
            qosa_memset(&pingresp_pkt, 0x00, sizeof(mqtt_packet));

            pingresp_pkt.header.byte = client_ptr->cur_input_packet.header.byte;
            pingresp_pkt.version = client_ptr->user_option.version;

            qcm_mqtt_deserialize_zero(&(client_ptr->cur_input_packet), &pingresp_pkt, client_ptr->cur_input_packet.varible_header.remain_len);
            qcm_mqtt_client_remove_request(client_ptr, MQTT_PINGREQ, 0);
        }
        break;

        case MQTT_SUBACK: {
            mqtt_suback_packet suback_pkt;

            qosa_memset(&suback_pkt, 0x00, sizeof(mqtt_suback_packet));

            suback_pkt.header.byte = client_ptr->cur_input_packet.header.byte;
            suback_pkt.version = client_ptr->user_option.version;

            result = qcm_mqtt_deserialize_subscribe_ack(&(client_ptr->cur_input_packet), &suback_pkt, client_ptr->cur_input_packet.varible_header.remain_len);
            if (result == MQTTPACKET_READ_COMPLETE)
            {
                qcm_mqtt_client_remove_request(client_ptr, MQTT_SUBSCRIBE, suback_pkt.msg_id);
                qcm_mqtt_client_response_commone(client_ptr, QCM_MQTT_CLIENT_SUBACK_EVENT, QCM_MQTT_RES_OK, &suback_pkt);
            }
            else
            {
#ifdef CONFIG_QCM_MQTT5_FUNC
                qosa_free(suback_pkt.options);
#endif /* CONFIG_QCM_MQTT5_FUNC */
            }
        }
        break;

        case MQTT_UNSUBACK: {
            mqtt_unsuback_packet unsuback_pkt;

            qosa_memset(&unsuback_pkt, 0x00, sizeof(mqtt_unsuback_packet));

            unsuback_pkt.header.byte = client_ptr->cur_input_packet.header.byte;
            unsuback_pkt.version = client_ptr->user_option.version;

            result = qcm_mqtt_deserialize_unsuback(&(client_ptr->cur_input_packet), &unsuback_pkt, client_ptr->cur_input_packet.varible_header.remain_len);
            if (result == MQTTPACKET_READ_COMPLETE)
            {
                qcm_mqtt_client_remove_request(client_ptr, MQTT_UNSUBSCRIBE, unsuback_pkt.msg_id);
                qcm_mqtt_client_response_commone(client_ptr, QCM_MQTT_CLIENT_UNSUBACK_EVENT, QCM_MQTT_RES_OK, &unsuback_pkt);
            }
        }
        break;

        case MQTT_PUBACK: {
            mqtt_puback_packet   puback_pkt;
            qcm_mqtt_send_resp_t send_resp = {0};

            qosa_memset(&puback_pkt, 0x00, sizeof(mqtt_unsuback_packet));

            puback_pkt.header.byte = client_ptr->cur_input_packet.header.byte;
            puback_pkt.version = client_ptr->user_option.version;

            result = qcm_mqtt_deserialize_ack(&(client_ptr->cur_input_packet), &puback_pkt, client_ptr->cur_input_packet.varible_header.remain_len);

            if (result == MQTTPACKET_READ_COMPLETE)
            {
                send_resp.msg_id = puback_pkt.msg_id;
                send_resp.send_result = QCM_MQTT_SEND_OK;
#ifdef CONFIG_QCM_MQTT5_FUNC
                //Todo To be used
                qosa_free(puback_pkt.options);
#endif /* CONFIG_QCM_MQTT5_FUNC */                
                qcm_mqtt_client_remove_request(client_ptr, MQTT_PUBLISH, puback_pkt.msg_id);
                qcm_mqtt_client_response_commone(client_ptr, QCM_MQTT_CLIENT_PUBLISH_EVENT, QCM_MQTT_RES_OK, &send_resp);
            }
        }
        break;

        case MQTT_PUBREC: {
            //According to the MQTT protocol specification, receiving a REC indicates that the PUB was not successfully published; the PUBLISH message can only be reported after fully waiting for PUBCOMP.
            //PUBREC command response, as defined by the protocol specification, should be directly discarded if no PUBCOMP is received.
            //According to the protocol, if the client does not receive a PUBCOMP packet before the PUBREL transmission times out, it must resend the PUBREL packet without resending the PUBLISH packet. Therefore, after receiving the PUBREC packet, the PUBLISH packet in the pending list should be deleted.

            mqtt_pubrec_packet  pubrec_pkt;
            mqtt_pubrel_packet  pubrel_pkt;
            qcm_mqtt_request_t *rb = QOSA_NULL;

            qosa_memset(&pubrec_pkt, 0x00, sizeof(mqtt_pubrec_packet));

            pubrec_pkt.header.byte = client_ptr->cur_input_packet.header.byte;
            pubrec_pkt.version = client_ptr->user_option.version;

            qcm_mqtt_deserialize_ack(&(client_ptr->cur_input_packet), &pubrec_pkt, client_ptr->cur_input_packet.varible_header.remain_len);

            qcm_mqtt_client_remove_request(client_ptr, MQTT_PUBLISH, pubrec_pkt.msg_id);
#ifdef CONFIG_QCM_MQTT5_FUNC
            qosa_free(pubrec_pkt.options);
#endif /* CONFIG_QCM_MQTT5_FUNC */

            pubrel_pkt.header.byte = 0;
            pubrel_pkt.header.bits.type = MQTT_PUBREL;
            pubrel_pkt.header.bits.qos = 1;

            pubrel_pkt.msg_id = pubrec_pkt.msg_id;

            rb = (qcm_mqtt_request_t *)qosa_malloc(sizeof(qcm_mqtt_request_t));
            if (rb == QOSA_NULL)
            {
                QLOGE("qosa_malloc fail");
                break;
            }

            qosa_memset(rb, 0x00, sizeof(qcm_mqtt_request_t));

            if (MQTTPACKET_READ_COMPLETE != qcm_mqtt_serialize_ack(&(rb->buffer), &pubrel_pkt))
            {
                qosa_free(rb);
            }
            else
            {
                rb->header.byte = pubrel_pkt.header.byte;
                rb->msg_id = pubrec_pkt.msg_id;
                rb->retry_cnt = client_ptr->user_option.delivery_cnt;

                qosa_q_link(rb, &rb->link);
                qosa_q_put(&client_ptr->out_packet_queue, &rb->link);
            }
            qcm_mqtt_client_report_clientid(QCM_MQTT_SEND_EVENT, client_ptr, QCM_MQTT_RES_OK);
        }
        break;

        case MQTT_PUBCOMP: {
            //MQTT Qos2 Publish message sent successfully
            mqtt_pubcomp_packet  pubcomp_pkt;
            qcm_mqtt_send_resp_t send_resp = {0};

            qosa_memset(&pubcomp_pkt, 0x00, sizeof(mqtt_pubcomp_packet));

            pubcomp_pkt.header.byte = client_ptr->cur_input_packet.header.byte;
            pubcomp_pkt.version = client_ptr->user_option.version;

            result = qcm_mqtt_deserialize_ack(&(client_ptr->cur_input_packet), &pubcomp_pkt, client_ptr->cur_input_packet.varible_header.remain_len);
            if (result == MQTTPACKET_READ_COMPLETE)
            {
                send_resp.msg_id = pubcomp_pkt.msg_id;
                send_resp.send_result = QCM_MQTT_SEND_OK;

                qcm_mqtt_client_remove_request(client_ptr, MQTT_PUBREL, pubcomp_pkt.msg_id);
                qcm_mqtt_client_response_commone(client_ptr, QCM_MQTT_CLIENT_PUBLISH_EVENT, QCM_MQTT_RES_OK, &send_resp);
            }
#ifdef CONFIG_QCM_MQTT5_FUNC
            //Todo: No usage reporting, destroy and release options on initialization
            qosa_free(pubcomp_pkt.options);
#endif /* CONFIG_QCM_MQTT5_FUNC */
        }
        break;

        case MQTT_PUBLISH: {
            qcm_mqtt_new_message_t *new_msg_ptr = QOSA_NULL;

            new_msg_ptr = (qcm_mqtt_new_message_t *)qosa_malloc(sizeof(qcm_mqtt_new_message_t));
            if (new_msg_ptr == QOSA_NULL)
            {
                QLOGE("no space");
                return QCM_MQTT_RES_ERROR_NO_MEMORY;
            }

            new_msg_ptr->new_msg.version = client_ptr->user_option.version;

            result = qcm_mqtt_deserialize_publish(&(client_ptr->cur_input_packet), &new_msg_ptr->new_msg);
            if (MQTTPACKET_READ_COMPLETE != result)
            {
                qosa_free(new_msg_ptr);
                break;
            }

#ifdef CONFIG_QCM_MQTT5_FUNC
            if (new_msg_ptr->new_msg.options != QOSA_NULL && new_msg_ptr->new_msg.options->topic_alias > 0)
            {
                if (new_msg_ptr->new_msg.topic.data_len == 1)
                {
                    qcm_mqtt_free_data(&new_msg_ptr->new_msg.topic);
                    qcm_mqtt_topic_alias_t *topic_alias_ptr = QOSA_NULL;

                    topic_alias_ptr
                        = qosa_q_linear_search(&client_ptr->topic_alias_list, qcm_mqtt_client_compare_topic_alias, &new_msg_ptr->new_msg.options->topic_alias);
                    if (topic_alias_ptr != QOSA_NULL)
                    {
                        QLOGV(
                            "TopicAlias: mapping found (alias=%d -> topic='%s', inbound)",
                            new_msg_ptr->new_msg.options->topic_alias,
                            topic_alias_ptr->topic.data_ptr
                        );

                        qcm_mqtt_malloc_data(&new_msg_ptr->new_msg.topic, topic_alias_ptr->topic.data_len, topic_alias_ptr->topic.data_ptr);
                    }
                    else
                    {
                        QLOGV("TopicAlias: mapping no found (alias=%d)", new_msg_ptr->new_msg.options->topic_alias);
                    }
                }
                else
                {
                    qcm_mqtt_data_t topic_cp = {0};

                    qcm_mqtt_malloc_data(&topic_cp, new_msg_ptr->new_msg.topic.data_len, new_msg_ptr->new_msg.topic.data_ptr);
                    if (qcm_mqtt_topic_alias_bind(client_ptr, new_msg_ptr->new_msg.options->topic_alias, &topic_cp) != QCM_MQTT_RES_OK)
                    {
                        QLOGW("TopicAlias: add fail");
                    }
                }
            }
#endif /* CONFIG_QCM_MQTT5_FUNC */

            QLOGV("qos:%d", client_ptr->cur_input_packet.header.bits.qos);
            if (client_ptr->cur_input_packet.header.bits.qos == 2)
            {
                //According to the protocol specification, for QoS 2, the PUBLISH message must be retransmitted before receiving the PUBREC message.
                //According to the protocol, when using QoS 2, upon receiving a PUBLISH message, a PUBREC message must be sent in response and the PUBREC message must not be retransmitted. If the PUBREC message times out, appropriate measures should be taken, with the specific actions to be defined by the implementer.
                //December 11, 2023, 00:33 larson.li I believe a timeout retransmission configuration should be set for PUBREC, and it should be sent only once. After checking the PUBREC timeout, the cached message in the QoS2 queue should be directly deleted,
                //December 11, 2023, 00:35 larson.li At the same time, when receiving the next PUBLISH message, check if the MSGID is greater than the msgid in Qos2, clear the Qos2 queue cache. The MQTT protocol stipulates that message queue IDs must be executed sequentially.
                mqtt_puback_packet  pubrec_pkt = {0};
                qcm_mqtt_request_t *rb = QOSA_NULL;

                qcm_mqtt_client_store_publish_qos2(client_ptr, new_msg_ptr);  //QOS=2 requires waiting until pubrel is received before notifying the application layer

                pubrec_pkt.header.byte = 0;
                pubrec_pkt.header.bits.type = MQTT_PUBREC;
                pubrec_pkt.header.bits.qos = 0;

                pubrec_pkt.msg_id = new_msg_ptr->new_msg.msg_id;

                rb = qosa_malloc(sizeof(qcm_mqtt_request_t));
                if (rb == QOSA_NULL)
                {
                    QLOGE("qosa_malloc fail");
                    break;
                }

                qosa_memset(rb, 0x00, sizeof(qcm_mqtt_request_t));

                if (MQTTPACKET_READ_COMPLETE != qcm_mqtt_serialize_ack(&(rb->buffer), &pubrec_pkt))
                {
                    qosa_free(rb);
                }
                else
                {
                    rb->header.byte = pubrec_pkt.header.byte;
                    rb->msg_id = pubrec_pkt.msg_id;
                    rb->retry_cnt = 0;
                    qosa_q_link(rb, &rb->link);
                    qosa_q_put(&client_ptr->out_packet_queue, &rb->link);
                }
                qcm_mqtt_client_report_clientid(QCM_MQTT_SEND_EVENT, client_ptr, QCM_MQTT_RES_OK);
            }
            else
            {
                if (qcm_mqtt_client_add_new_publish_message(client_ptr, new_msg_ptr) == QCM_MQTT_RES_ERROR_STORE_FULL)
                {
                    qcm_mqtt_free_data(&new_msg_ptr->new_msg.topic);
                    qcm_mqtt_free_data(&new_msg_ptr->new_msg.payload);

                    qosa_free(new_msg_ptr);
                }
                else
                {
                    if (client_ptr->cur_input_packet.header.bits.qos == 1)
                    {
                        mqtt_puback_packet  puback_pkt;
                        qcm_mqtt_request_t *rb = QOSA_NULL;

                        puback_pkt.header.byte = 0;
                        puback_pkt.header.bits.type = MQTT_PUBACK;
                        puback_pkt.header.bits.qos = 0;

                        puback_pkt.msg_id = new_msg_ptr->new_msg.msg_id;

                        rb = qosa_malloc(sizeof(qcm_mqtt_request_t));
                        if (rb == QOSA_NULL)
                        {
                            QLOGE("qosa_malloc fail");
                            break;
                        }

                        qosa_memset(rb, 0x00, sizeof(qcm_mqtt_request_t));

                        if (MQTTPACKET_READ_COMPLETE != qcm_mqtt_serialize_ack(&(rb->buffer), &puback_pkt))
                        {
                            qosa_free(rb);
                        }
                        else
                        {
                            rb->header.byte = puback_pkt.header.byte;
                            rb->msg_id = puback_pkt.msg_id;
                            rb->retry_cnt = 0;
                            qosa_q_link(rb, &rb->link);
                            qosa_q_put(&client_ptr->out_packet_queue, &rb->link);
                        }
                        qcm_mqtt_client_report_clientid(QCM_MQTT_SEND_EVENT, client_ptr, QCM_MQTT_RES_OK);
                    }
                    qcm_mqtt_client_response_commone(client_ptr, QCM_MQTT_CLIENT_NEW_MESSAGE_EVENT, QCM_MQTT_RES_OK, new_msg_ptr);
                }
            }
        }
        break;

        case MQTT_PUBREL: {
            //The MQTT protocol stipulates that upon receiving a PUBREL message, it can be confirmed that the PUBLISH Qos2 packet has been received. The protocol only requires that a PUBCOMP message must be sent after receiving PUBREL, and there is no need to retransmit the PUBCOMP message.
            mqtt_pubrel_packet      pubrel_pkt = {0};
            mqtt_pubcomp_packet     pubcomp_pkt = {0};
            qcm_mqtt_request_t     *rb = QOSA_NULL;
            qcm_mqtt_new_message_t *new_msg_ptr = QOSA_NULL;

            qcm_mqtt_deserialize_ack(&(client_ptr->cur_input_packet), &pubrel_pkt, client_ptr->cur_input_packet.varible_header.remain_len);
            pubrel_pkt.header.byte = client_ptr->cur_input_packet.header.byte;

            qcm_mqtt_client_remove_request(client_ptr, MQTT_PUBREC, pubrel_pkt.msg_id);

            if (qcm_mqtt_client_read_publish_qos2(client_ptr, &new_msg_ptr, pubrel_pkt.msg_id)
                == QCM_MQTT_RES_OK)  //Upon receiving PUBREL, retrieve the packet with the corresponding msg_id from the inboundMsgs linked list.
            {
                if ((qcm_mqtt_client_add_new_publish_message(client_ptr, new_msg_ptr)) != QCM_MQTT_RES_OK)  //Insert into the subcribe_msg_box linked list
                {
                    //Release the memory space corresponding to msg_id in the inboundMsgs linked list
                    qcm_mqtt_free_data(&new_msg_ptr->new_msg.topic);
                    qcm_mqtt_free_data(&new_msg_ptr->new_msg.payload);

                    qosa_free(new_msg_ptr);
                    // if (0 == client_ptr->user_option.recv_mode)
                    // {
                    //     client_ptr->cur_input_packet.buf_pos = tmp_pos;
                    //     client_ptr->sotre_level = QCM_MQTT_STORE_PUB_MSG_HIGH;
                    // }
                }
                else
                {
                    qcm_mqtt_client_response_commone(client_ptr, QCM_MQTT_CLIENT_NEW_MESSAGE_EVENT, QCM_MQTT_RES_OK, new_msg_ptr);
                }
            }

            pubcomp_pkt.header.byte = 0;
            pubcomp_pkt.header.bits.type = MQTT_PUBCOMP;
            pubcomp_pkt.header.bits.qos = 0;

            pubcomp_pkt.msg_id = pubrel_pkt.msg_id;

            rb = qosa_malloc(sizeof(qcm_mqtt_request_t));
            if (rb == QOSA_NULL)
            {
                QLOGE("qosa_malloc fail");
                break;
            }

            qosa_memset(rb, 0x00, sizeof(qcm_mqtt_request_t));

            if (MQTTPACKET_READ_COMPLETE != qcm_mqtt_serialize_ack(&(rb->buffer), &pubcomp_pkt))
            {
                qosa_free(rb);
            }
            else
            {
                rb->header.byte = pubcomp_pkt.header.byte;
                rb->msg_id = pubcomp_pkt.msg_id;
                rb->retry_cnt = 0;
                qosa_q_link(rb, &rb->link);
                qosa_q_put(&client_ptr->out_packet_queue, &rb->link);
            }
            qcm_mqtt_client_report_clientid(QCM_MQTT_SEND_EVENT, client_ptr, QCM_MQTT_RES_OK);
        }
        break;

#ifdef CONFIG_QCM_MQTT5_FUNC
        case MQTT_DISCONNECT: {
            mqtt_disconnect_packet disc_pkt;

            qosa_memset(&disc_pkt, 0x00, sizeof(mqtt_disconnect_packet));

            disc_pkt.header.byte = client_ptr->cur_input_packet.header.byte;
            disc_pkt.version = client_ptr->user_option.version;

            qcm_mqtt_deserialize_ack(&(client_ptr->cur_input_packet), &disc_pkt, client_ptr->cur_input_packet.varible_header.remain_len);

            qcm_mqtt_client_response_commone(client_ptr, QCM_MQTT_CLIENT_DISCONNECT_EVENT, QCM_MQTT_RES_OK, &disc_pkt);
        }
        break;

        case MQTT_AUTH: {
            mqtt_auth_packet auth_pkt;

            qosa_memset(&auth_pkt, 0x00, sizeof(mqtt_auth_packet));

            auth_pkt.header.byte = client_ptr->cur_input_packet.header.byte;
            auth_pkt.version = client_ptr->user_option.version;

            qcm_mqtt_deserialize_ack(&(client_ptr->cur_input_packet), &auth_pkt, client_ptr->cur_input_packet.varible_header.remain_len);

            qcm_mqtt_client_response_commone(client_ptr, QCM_MQTT_CLIENT_AUTH_EVENT, QCM_MQTT_RES_OK, &auth_pkt);
            //Todo: No usage report, destroy and release auth_pkt.options here.
            qosa_free(auth_pkt.options);
        }
        break;
#endif /* CONFIG_QCM_MQTT5_FUNC */

        default:
            client_ptr->cur_input_packet.buf_pos += 1;
            break;
    }

    if (result == MQTTPACKET_READ_COMPLETE)
    {
        qosa_memset(&client_ptr->cur_input_packet.header, 0, sizeof(qcm_mqtt_header));
        qosa_memset(&client_ptr->cur_input_packet.varible_header, 0, sizeof(mqtt_varible_header));
        return QCM_MQTT_RES_OK;
    }
    else
    {
        QLOGI("buf_pos=%d, %d", result, client_ptr->cur_input_packet.buf_pos);
        return QCM_MQTT_RES_ERROR_DATA_WOULDBLOCK;
    }
}

static void qcm_mqtt_client_continue_parse(qcm_mqtt_client_t *client_ptr)
{
    /*The offset position of 'remaining length'*/
    int rc = 0;
    /*The 'Remaining Length' field in an MQTT packet*/
    qosa_uint32_t remain_len = 0;
    QLOGV("...");

    if (client_ptr->cur_input_packet.varible_header.remain_len == 0)
    {
        /*The first byte is considered the MQTT Header*/
        client_ptr->cur_input_packet.header.byte = (unsigned char)client_ptr->cur_input_packet.buff[0];
        if (MQTTPACKET_READ_COMPLETE != qcm_mqtt_check_header(client_ptr->cur_input_packet.header))
        {
            //According to the protocol, when the header is corrupted, the TCP connection should be terminated.
            //Disconnect the TCP connection, send an event for TCP disconnection handling, and report the status.
            qcm_mqtt_disc_param_t *dics_ptr = QOSA_NULL;

            dics_ptr = (qcm_mqtt_disc_param_t *)qosa_malloc(sizeof(qcm_mqtt_disc_param_t));
            if (dics_ptr == QOSA_NULL)
            {
                QLOGW("...");
                return;
            }

            client_ptr->close_cause = QCM_MQTT_CLOSE_BY_PROTOCOL_FAIL;
            if (qcm_mqtt_client_event_report(QCM_MQTT_DISC_EVENT, dics_ptr) != QCM_MQTT_RES_OK)
            {
                qosa_free(dics_ptr);
            }
            qcm_mqtt_client_socket_evt_cb(client_ptr->socketfd, QCM_SOCK_CLOSE_EVENT, QCM_SOCK_SUCCESS, client_ptr);
        }

        rc = qcm_mqtt_remaining_length_decode((const char *)(client_ptr->cur_input_packet.buff + 1), client_ptr->cur_input_packet.buf_len - 1, &remain_len);
        QLOGI("rc : %d remain_len : %d", rc, remain_len);
        //If the parsed length is less than the buffer length, it indicates that there is still a lot of data not yet received, and more data needs to be received to complete the parsing of a data packet.
        if (rc < 0)
        {
            return;
        }

        client_ptr->cur_input_packet.varible_header.varible_header_len = rc;
        client_ptr->cur_input_packet.varible_header.remain_len = remain_len;
    }

    qcm_mqtt_client_parse_incoming(client_ptr);
    QLOGV("buf len:%d, pos:%d", client_ptr->cur_input_packet.buf_len, client_ptr->cur_input_packet.buf_pos);
    QLOGV("remain_len:%d", client_ptr->cur_input_packet.varible_header.remain_len);

    if (client_ptr->cur_input_packet.buf_pos == client_ptr->cur_input_packet.buf_len)
    {
        client_ptr->cur_input_packet.buf_len = 0;
        client_ptr->cur_input_packet.buf_pos = 0;  //WODBLOCK can't zero -- cur_input_packet.header/varible_header
        qosa_memset(client_ptr->cur_input_packet.buff, 0, sizeof(QCM_MQTT_MAX_READ_BUFF_LEN));

        qcm_mqtt_client_socket_evt_cb(client_ptr->socketfd, QCM_SOCK_READ_EVENT, QCM_SOCK_SUCCESS, client_ptr);
    }
    else
    {
        /**
         *  Move the unread data forward.
         */
        qosa_uint32_t buff_remain_len = 0;

        buff_remain_len = client_ptr->cur_input_packet.buf_len - client_ptr->cur_input_packet.buf_pos;
        qosa_memmove(client_ptr->cur_input_packet.buff, client_ptr->cur_input_packet.buff + client_ptr->cur_input_packet.buf_pos, buff_remain_len);

        client_ptr->cur_input_packet.buf_len = buff_remain_len;
        client_ptr->cur_input_packet.buf_pos = 0;

        if (qosa_q_cnt(&(client_ptr->new_msg_pub)) < client_ptr->user_option.max_recv_store_cnt)
        {
            QLOGV("continue");
            //2023-12-11 01:09:14 Send a reav event instead of directly nesting calls
            qcm_mqtt_client_socket_evt_cb(client_ptr->socketfd, QCM_SOCK_READ_EVENT, QCM_SOCK_SUCCESS, client_ptr);
        }
    }
}

/**
 * @brief Handle socket connect event notification
 */
static void qcm_mqtt_client_event_connect(qcm_mqtt_client_t *client_ptr, int result_code)
{
    if (result_code == QCM_SOCK_SUCCESS)
    {
#ifdef CONFIG_QCM_VTLS_FUNC
        QLOGV("ssl_enable : %d", client_ptr->user_option.ssl_enable);
        if (client_ptr->user_option.ssl_enable == QOSA_TRUE)
        {
            if (qcm_mqtt_client_event_report(QCM_MQTT_SSL_SHAKEHAND_EVNET, client_ptr) != QCM_MQTT_RES_OK)
            {
                qosa_free(client_ptr);
            }
            return;
        }
#endif
        client_ptr->client_state = QCM_MQTT_STATE_TCP_CONNECTED;
        QLOGV("mqtt tcp connect");
        qcm_mqtt_client_response_commone(client_ptr, QCM_MQTT_CLIENT_OPEN_EVENT, QCM_MQTT_RES_OK, QOSA_NULL);
    }
    else
    {
        //connect failed, record the close reason before the socket is destroyed
        //2 CLOSE_EVENT_RST: The server actively sends an RST during the SYN handshake process.
        //3 CLOSE_EVENT_SYN_TIMEOUT: TCP connection closed due to internal SYN retransmission timeout event
        QLOGI("client_id: %d", client_ptr->client_id);
        QLOGV("socketfd: %d", client_ptr->socketfd);
        g_mqtt_last_close_event[client_ptr->client_id] = qcm_socket_get_last_close_event(client_ptr->socketfd);

        qcm_socket_close(client_ptr->socketfd);
        client_ptr->socketfd = 0;
        qcm_mqtt_client_response_commone(client_ptr, QCM_MQTT_CLIENT_OPEN_EVENT, QCM_MQTT_RES_ERROR_TCP_CONNECT, QOSA_NULL);
        //Directly release all resources
        qcm_mqtt_client_free(client_ptr);
    }
}

/**
 * @brief Handle socket close event notification
 */
static void qcm_mqtt_client_event_close(qcm_mqtt_client_t *client_ptr)
{
    int ret = 0;
    QOSA_UNUSED(ret);
    //If a close event arrives, it indicates that the underlying connection has been disconnected. At this point, calling the close action will also quickly close it.
    //qcm_socket_close(client_ptr->socketfd);
    QLOGV("enter");
    if (client_ptr == QOSA_NULL)
    {
        return;
    }
    QLOGI("client_state %d", client_ptr->client_state);

    //If the user actively initiates QMTDISC (Disconnect Request), causing the server to send FIN/RST to disconnect, this is a normal closure behavior, and there is no need to record the closure event.
    if (client_ptr->client_state != QCM_MQTT_STATE_MQTT_DISCONNECTING)
    {
        //Closure event not caused by a Disconnect request

        //Received QCM_SOCK_CLOSE_EVENT, record the final cause of closure here.
        //1 CLOSE_EVENT_FIN: TCP connection disconnection event caused by the server actively sending a FIN packet.
        //2 CLOSE_EVENT_RST: The server actively sends an RST after the connection is established.
        //4 CLOSE_EVENT_ACK_TIMEOUT: TCP connection disconnection event caused by internal ACK retransmission timeout
        QLOGV("client_id: %d", client_ptr->client_id);
        QLOGV("sock_hd: %d", client_ptr->socketfd);
        g_mqtt_last_close_event[client_ptr->client_id] = qcm_socket_get_last_close_event(client_ptr->socketfd);
    }

#ifdef CONFIG_QCM_VTLS_FUNC
    if (client_ptr->user_option.ssl_enable == QOSA_TRUE && client_ptr->client_state == QCM_MQTT_STATE_SSL_HANDSHAKE)
    {
        qcm_socket_linger_t linger = {.on_off = 1, .linger_val = 0};
        qcm_socket_set_opt(client_ptr->socketfd, QCM_SOCK_LINGER_OPT, (void *)&linger);

        qcm_socket_close(client_ptr->socketfd);
        //Directly release all resources
        qcm_mqtt_client_free(client_ptr);
        return;
    }
#endif /* CONFIG_QCM_VTLS_FUNC */

    if (client_ptr->client_state == QCM_MQTT_STATE_MQTT_CONNECTED || client_ptr->client_state == QCM_MQTT_STATE_TCP_CONNECTED
        || client_ptr->client_state == QCM_MQTT_STATE_MQTT_CONNECTING)
    {
        //After the TCP socket connection is successfully established, receiving a socket close event may only indicate that the connection is not fully established at the MQTT protocol layer.
        //At this point, report the status, clear the pending retransmission cache, and the remaining resource variables will be released by the mqtt close command.
        if (client_ptr->close_cause != QCM_MQTT_CLOSE_BY_CONN_TIMEOUT && client_ptr->close_cause != QCM_MQTT_CLOSE_BY_CONNACK_FAIL)
            client_ptr->close_cause = QCM_MQTT_CLOSE_BY_PEER_RESET;
        client_ptr->client_state = QCM_MQTT_STATE_TCP_CLOSING;

        //ret = qcm_socket_shutdown(client_ptr->socketfd, QCM_SHUT_RDWR);
        ret = qcm_socket_close(client_ptr->socketfd);
        QLOGI("close ret=%d", ret);
        qcm_mqtt_client_remove_request_all(client_ptr);
        qcm_mqtt_client_close_cause(client_ptr);

        qcm_mqtt_client_free(client_ptr);
    }
    else if (client_ptr->client_state == QCM_MQTT_STATE_MQTT_DISCONNECTING)
    {
        //According to the email resolution, to prevent the general customer base from experiencing situations where only a DISCONNECT packet is sent but not received by the server, which could lead to the server incorrectly publishing a will message, the module has added the reporting of +QMTSTAT: 1,5 to notify the application layer that the server has actively disconnected.
        client_ptr->close_cause = QCM_MQTT_CLOSE_BY_DISCONN_PEER_FIN;
        client_ptr->client_state = QCM_MQTT_STATE_TCP_CLOSING;

        //ret = qcm_socket_shutdown(client_ptr->socketfd, QCM_SHUT_RDWR);
        ret = qcm_socket_close(client_ptr->socketfd);
        QLOGI("disconnect ret=%d", ret);

        qcm_mqtt_client_close_cause(client_ptr);
        qcm_mqtt_client_free(client_ptr);
    }
    else if (client_ptr->client_state == QCM_MQTT_STATE_TCP_CLOSING)
    {
        QLOGI("CLOSING");
        qcm_socket_close(client_ptr->socketfd);

        //The user actively performed an MQTT CLOSE action; if the socket close is not completed, release all resources here and report that the close command was executed successfully.
        client_ptr->client_state = QCM_MQTT_STATE_TCP_CLOSED;
        if (client_ptr->close_cause == QCM_MQTT_CLOSE_BY_ACIVE_CLOSE)
        {
            //Proactively close reporting MQTT closed successfully
            qcm_mqtt_client_response_commone(client_ptr, QCM_MQTT_CLIENT_CLOSE_EVENT, QCM_MQTT_RES_OK, QOSA_NULL);
        }
        qcm_mqtt_client_free(client_ptr);
    }
    //The protocol also stipulates that after sending a disconnect message, the client should actively close the TCP connection. Therefore, after sending the disconnect message, `shutdown` should also be called to close the WR operation, triggering `select` to return a read event.
}

/**
 * @brief Handling socket read event notifications
 */
static void qcm_mqtt_client_event_read(qcm_mqtt_client_t *client_ptr)
{
    int read_len = QCM_MQTT_MAX_READ_BUFF_LEN - client_ptr->cur_input_packet.buf_len;

    if (read_len == 0)
    {
        goto exit;
    }

    read_len = qcm_socket_read(client_ptr->socketfd, (char *)(client_ptr->cur_input_packet.buff + client_ptr->cur_input_packet.buf_len), read_len);
    QLOGI("read_len:%d", read_len);

    if (read_len <= 0)
    {
        if (read_len != QCM_SOCK_WODBLOCK)
        {
            //Do not consider that qcm_socket_read has already sent a close event internally.
        }
        else
        {
            if (client_ptr->cur_input_packet.buf_len != 0)
            {
                goto exit;
            }
        }

        return;
    }
    client_ptr->cur_input_packet.buf_len += read_len;

exit:
    qcm_mqtt_client_continue_parse(client_ptr);
}

/**
 * @brief Handle socket write event notification
 */
static void qcm_mqtt_client_event_write(qcm_mqtt_client_t *client_ptr)
{
    qcm_mqtt_clientid_info_t *clientid_info_ptr = QOSA_NULL;

    if (client_ptr->client_state == QCM_MQTT_STATE_TCP_CLOSED)
    {
        QLOGV("...");
        return;
    }

    clientid_info_ptr = (qcm_mqtt_clientid_info_t *)qosa_malloc(sizeof(qcm_mqtt_clientid_info_t));
    if (clientid_info_ptr == QOSA_NULL)
    {
        QLOGV("...");
        return;
    }

    //add larson:
    //1. On x64 systems, addresses are 50+ bits, but we often use uint32 or int32, which causes issues during cross-platform development. That's why the osa_ptr type was introduced.
    //2. If internal state changes are not synchronized and messages are stored in the queue, then when only the client ID is present and the corresponding pointer cannot be found, it can simply return, avoiding or reducing critical section issues.
    clientid_info_ptr->client_id = client_ptr->client_id;
    qcm_mqtt_client_tcp_send_handler(clientid_info_ptr);
}

/**
 * @brief Process TCP layer reported message events
 */
static void qcm_mqtt_client_socket_event_handler(void *argv)
{
    qcm_mqtt_soc_event_t *soc_event = (qcm_mqtt_soc_event_t *)argv;
    qcm_mqtt_client_t    *client_ptr = (qcm_mqtt_client_t *)soc_event->user_data;

    client_ptr = qcm_mqtt_client_get_ptr_with_socket_fd(soc_event->sock_hd);
    if (client_ptr == QOSA_NULL || client_ptr != soc_event->user_data)
    {
        MQTT_FREE_PTR(soc_event);
        return;
    }

    QLOGI("evt:%d, %x  client id: %d", soc_event->event_mask, soc_event->sock_hd, client_ptr->client_id);

    if (soc_event->event_mask & QCM_SOCK_ACCEPT_EVENT)
    {
        QLOGV("QCM_SOCK_ACCEPT_EVENT");
    }
    else if (soc_event->event_mask & QCM_SOCK_CONNECT_EVENT)
    {
        QLOGV("QCM_SOCK_CONNECT_EVENT");
        qcm_mqtt_client_event_connect(client_ptr, soc_event->code);
    }
    else if (soc_event->event_mask & QCM_SOCK_CLOSE_EVENT)
    {
        QLOGV("QCM_SOCK_CLOSE_EVENT");
        qcm_mqtt_client_event_close(client_ptr);
    }
    else if (soc_event->event_mask & QCM_SOCK_READ_EVENT)
    {
        QLOGV("QCM_SOCK_READ_EVENT");
        client_ptr->read_event_cnt--;
        QLOGV("client_state :%d, read_event_cnt:%d", client_ptr->client_state, client_ptr->read_event_cnt);

#ifdef CONFIG_QCM_VTLS_FUNC
        if (client_ptr->user_option.ssl_enable == QOSA_TRUE && client_ptr->client_state == QCM_MQTT_STATE_SSL_HANDSHAKE)
        {
            qcm_mqtt_client_ssl_shakehand_handler(client_ptr);
        }
        else
#endif
        {
            qcm_mqtt_client_event_read(client_ptr);
        }
    }
    else if (soc_event->event_mask & QCM_SOCK_WRITE_EVENT)
    {
        QLOGV("QCM_SOCK_WRITE_EVENT");
        qcm_mqtt_client_event_write(client_ptr);
    }
    else if (soc_event->event_mask & QCM_SOCK_SSL_HD_TIMEOUT_EVENT)
    {
        QLOGV("QCM_SOCK_SSL_HD_TIMEOUT_EVENT");
        qcm_mqtt_client_event_close(client_ptr);
    }
    else if (soc_event->event_mask & QCM_SOCK_SENDENDACK_EVENT)
    {
        QLOGV("QCM_SOCK_SENDENDACK_EVENT");
    }
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
    else if (soc_event->event_mask & QCM_SOCK_NET_DOWN_EVENT)
    {
        QLOGV("QCM_SOCK_NET_DOWN_EVENT");
        qcm_mqtt_client_pdp_deacted_handler(client_ptr);
    }
#endif
    MQTT_FREE_PTR(soc_event);
}

/**
 * @brief Perform MQTT connect packet encoding action processing
 */
static void qcm_mqtt_client_conn_encode_handler(void *argv)
{
    qcm_mqtt_eercode_e     result = QCM_MQTT_RES_OK;
    mqtt_connect_packet    conn_pkt;
    qcm_mqtt_request_t    *rb = QOSA_NULL;
    qcm_mqtt_conn_param_t *conn_param_ptr = (qcm_mqtt_conn_param_t *)argv;
    qcm_mqtt_client_t     *client_ptr = QOSA_NULL;

    QOSA_ASSERT(conn_param_ptr != QOSA_NULL);

    client_ptr = qcm_mqtt_client_get_ptr(conn_param_ptr->client_id);
    if (client_ptr == QOSA_NULL)
    {
        return;
    }

    qosa_memset(&conn_pkt, 0x00, sizeof(mqtt_connect_packet));

    conn_pkt.version = client_ptr->user_option.version;
    conn_pkt.header.byte = 0;
    conn_pkt.header.bits.type = MQTT_CONNECT;
    conn_pkt.header.bits.qos = 0;
    conn_pkt.header.bits.dup = 0;
    conn_pkt.header.bits.retain = 0;
    conn_pkt.flags.all = 0;
    conn_pkt.flags.bits.will = client_ptr->user_option.will_flag;
    conn_pkt.flags.bits.willQos = client_ptr->user_option.will_qos;
    conn_pkt.flags.bits.willRetain = client_ptr->user_option.will_retain;
    conn_pkt.flags.bits.cleanstart = client_ptr->user_option.clean_session;

    conn_pkt.client_id = (char *)conn_param_ptr->client_name.data_ptr;

#ifdef CONFIG_QCM_MQTT5_FUNC
    if (conn_pkt.version == QCM_MQTT_VERSION_V5)
    {
        qcm_mqtt_conn_properties_t *conn_properties = &client_ptr->user_option.conn_properties;

        if (conn_properties->session_expiry_interval != 0)
        {
            qcm_mqtt_data_to_property(&conn_pkt.conn_props, QCM_MQTT_PROPERTY_CODE_SESSION_EXPIRY_INTERVAL, &conn_properties->session_expiry_interval);
        }

        if (conn_properties->receive_maximum != 0)
        {
            qcm_mqtt_data_to_property(&conn_pkt.conn_props, QCM_MQTT_PROPERTY_CODE_RECEIVE_MAXIMUM, &conn_properties->receive_maximum);
        }

        if (conn_properties->topic_alias_maximum != 0)
        {
            qcm_mqtt_data_to_property(&conn_pkt.conn_props, QCM_MQTT_PROPERTY_CODE_TOPIC_ALIAS_MAXIMUM, &conn_properties->topic_alias_maximum);
        }

        if (conn_properties->maximum_packet_size != 0)
        {
            qcm_mqtt_data_to_property(&conn_pkt.conn_props, QCM_MQTT_PROPERTY_CODE_MAXIMUM_PACKET_SIZE, &conn_properties->maximum_packet_size);
        }

        if (conn_properties->request_response_information != 0)
        {
            qcm_mqtt_data_to_property(
                &conn_pkt.conn_props,
                QCM_MQTT_PROPERTY_CODE_REQUEST_RESPONSE_INFORMATION,
                &conn_properties->request_response_information
            );
        }

        if (conn_properties->request_problem_information != 0)
        {
            qcm_mqtt_data_to_property(&conn_pkt.conn_props, QCM_MQTT_PROPERTY_CODE_REQUEST_PROBLEM_INFORMATION, &conn_properties->request_problem_information);
        }

        qosa_uint8_t i = 0;
        for (; (i < conn_properties->user_property_cnt) && (i < QCM_MQTT_MAX_USER_PROPERTY_SUPPORT); i++)
        {
            qcm_mqtt_data_to_property(&conn_pkt.conn_props, QCM_MQTT_PROPERTY_CODE_USER_PROPERTY, &conn_properties->user_property[i]);
        }
    }
#endif

    QLOGI("username.data_len : %d", conn_param_ptr->username.data_len);
    if (conn_param_ptr->username.data_len != 0)
    {
        if (qcm_utf8_validate_string(conn_param_ptr->username.data_ptr) == 0)
        {
            result = QCM_MQTT_RES_ERROR_PARAM_INVALID;
            goto exit;
        }
        conn_pkt.flags.bits.username = 1;
        QLOGV("conn_pkt.flags.bits.username:%d", conn_pkt.flags.bits.username);
        conn_pkt.username = conn_param_ptr->username.data_ptr;

        QLOGV("password.data_len : %d", conn_param_ptr->password.data_len);
        if (conn_param_ptr->password.data_len)
        {
            if (qcm_utf8_validate_string(conn_param_ptr->password.data_ptr) == 0)
            {
                result = QCM_MQTT_RES_ERROR_PARAM_INVALID;
                goto exit;
            }
            conn_pkt.flags.bits.password = 1;
            QLOGV("conn_pkt.flags.bits.password:%d", conn_pkt.flags.bits.password);

            conn_pkt.password = conn_param_ptr->password.data_ptr;
        }
    }

    if (conn_pkt.flags.bits.will == QOSA_TRUE)
    {
        if (qcm_utf8_validate_string(client_ptr->user_option.will_topic.data_ptr) == 0
            || qcm_utf8_validate_string(client_ptr->user_option.will_message.data_ptr) == 0)
        {
            result = QCM_MQTT_RES_ERROR_PARAM_INVALID;
            goto exit;
        }
        conn_pkt.will_topic = client_ptr->user_option.will_topic.data_ptr;
        conn_pkt.will_msg = client_ptr->user_option.will_message.data_ptr;
#ifdef CONFIG_QCM_MQTT5_FUNC
        if (conn_pkt.version == QCM_MQTT_VERSION_V5)
        {
            qcm_mqtt_will_properties_t *will_properties = &client_ptr->user_option.will_properties;

            if (will_properties->will_delay_interval != 0)
            {
                qcm_mqtt_data_to_property(&conn_pkt.will_props, QCM_MQTT_PROPERTY_CODE_WILL_DELAY_INTERVAL, &will_properties->will_delay_interval);
            }

            if (will_properties->message_expiry_interval != 0)
            {
                qcm_mqtt_data_to_property(&conn_pkt.will_props, QCM_MQTT_PROPERTY_CODE_MESSAGE_EXPIRY_INTERVAL, &will_properties->message_expiry_interval);
            }

            if (will_properties->payload_format_indicator != 0)
            {
                qcm_mqtt_data_to_property(&conn_pkt.will_props, QCM_MQTT_PROPERTY_CODE_PAYLOAD_FORMAT_INDICATOR, &will_properties->payload_format_indicator);
            }

            if (will_properties->content_type.data_len != 0 && will_properties->content_type.data_ptr != QOSA_NULL)
            {
                qcm_mqtt_data_to_property(&conn_pkt.will_props, QCM_MQTT_PROPERTY_CODE_CONTENT_TYPE, &will_properties->content_type);
            }

            if (will_properties->response_topic.data_len != 0 && will_properties->response_topic.data_ptr != QOSA_NULL)
            {
                qcm_mqtt_data_to_property(&conn_pkt.will_props, QCM_MQTT_PROPERTY_CODE_RESPONSE_TOPIC, &will_properties->response_topic);
            }

            if (will_properties->correlation_data.data_len != 0 && will_properties->correlation_data.data_ptr != QOSA_NULL)
            {
                qcm_mqtt_data_to_property(&conn_pkt.will_props, QCM_MQTT_PROPERTY_CODE_CORRELATION_DATA, &will_properties->correlation_data);
            }

            qosa_uint8_t i = 0;
            for (; (i < will_properties->user_property_cnt) && (i < QCM_MQTT_MAX_USER_PROPERTY_SUPPORT); i++)
            {
                qcm_mqtt_data_to_property(&conn_pkt.will_props, QCM_MQTT_PROPERTY_CODE_USER_PROPERTY, &will_properties->user_property[i]);
            }
        }
#endif
    }

    conn_pkt.keep_alive_interval = client_ptr->user_option.kalive_time;

    rb = qosa_malloc(sizeof(qcm_mqtt_request_t));
    if (rb == QOSA_NULL)
    {
        result = QCM_MQTT_RES_ERROR_NO_MEMORY;
        goto exit;
    }

    qosa_memset(rb, 0x00, sizeof(qcm_mqtt_request_t));

    if (MQTTPACKET_READ_COMPLETE != qcm_mqtt_serialize_connect(&(rb->buffer), &conn_pkt))
    {
        qosa_free(rb);
        result = QCM_MQTT_RES_ERROR_NO_MEMORY;
        goto exit;
    }
    rb->header.byte = conn_pkt.header.byte;
    rb->msg_id = 0;
    rb->retry_cnt = 1;

    qosa_q_link(rb, &rb->link);
    qosa_q_put(&client_ptr->out_packet_queue, &rb->link);

exit:
    QLOGI("result : %x", result);

    if (conn_param_ptr->client_name.data_ptr != QOSA_NULL)
    {
        qosa_free(conn_param_ptr->client_name.data_ptr);
        conn_param_ptr->client_name.data_ptr = QOSA_NULL;
    }

    if (conn_param_ptr->username.data_ptr != QOSA_NULL)
    {
        qosa_free(conn_param_ptr->username.data_ptr);
        conn_param_ptr->username.data_ptr = QOSA_NULL;
    }

    if (conn_param_ptr->password.data_ptr != QOSA_NULL)
    {
        qosa_free(conn_param_ptr->password.data_ptr);
        conn_param_ptr->password.data_ptr = QOSA_NULL;
    }

#ifdef CONFIG_QCM_MQTT5_FUNC
    if (conn_pkt.conn_props.count != 0)
    {
        qcm_mqtt_properties_free(&conn_pkt.conn_props);
    }

    if (conn_pkt.will_props.count != 0)
    {
        qcm_mqtt_properties_free(&conn_pkt.will_props);
    }
#endif

    if (result == QCM_MQTT_RES_OK)
    {
        qcm_mqtt_client_report_clientid(QCM_MQTT_SEND_EVENT, client_ptr, QCM_MQTT_RES_OK);
    }
    else
    {
        qcm_mqtt_client_response_commone(client_ptr, QCM_MQTT_CLIENT_CONNECT_EVENT, result, QOSA_NULL);
    }
}

/**
 * @brief Perform data encoding
 */
static void qcm_mqtt_client_sub_encode_handler(void *argv)
{
    qcm_mqtt_eercode_e    result = QCM_MQTT_RES_OK;
    mqtt_subscribe_packet sub_pkt;
    qcm_mqtt_request_t   *rb = QOSA_NULL;
    qcm_mqtt_sub_param_t *sub_param_ptr = (qcm_mqtt_sub_param_t *)argv;
    qcm_mqtt_client_t    *client_ptr = QOSA_NULL;

    QOSA_ASSERT(sub_param_ptr != QOSA_NULL);

    client_ptr = qcm_mqtt_client_get_ptr(sub_param_ptr->client_id);
    if (client_ptr == QOSA_NULL)
    {
        return;
    }

    qosa_memset(&sub_pkt, 0x00, sizeof(mqtt_subscribe_packet));

    sub_pkt.version = client_ptr->user_option.version;
    sub_pkt.header.byte = 0;
    sub_pkt.header.bits.type = (sub_param_ptr->mode == QCM_MQTT_SUBSCRIBE_MODE) ? MQTT_SUBSCRIBE : MQTT_UNSUBSCRIBE;
    sub_pkt.header.bits.qos = 1;

    sub_pkt.msg_id = sub_param_ptr->msg_id;
    sub_pkt.topic_list = &(sub_param_ptr->topic_list);
    sub_pkt.packet_len = sub_param_ptr->packet_len;
#ifdef CONFIG_QCM_MQTT5_FUNC
    sub_pkt.props = &sub_param_ptr->props;
#endif /* CONFIG_QCM_MQTT5_FUNC */

    if (client_ptr->client_state != QCM_MQTT_STATE_MQTT_CONNECTED)
    {
        result = QCM_MQTT_RES_ERROR_STATUS;
        goto exit;
    }

    if (qcm_mqtt_client_check_request_allow(client_ptr, sub_pkt.msg_id) != QOSA_TRUE)
    {
        result = QCM_MQTT_RES_ERROR_MSG_ID_DUPLICATE;
        goto exit;
    }

    rb = qosa_malloc(sizeof(qcm_mqtt_request_t));
    if (rb == QOSA_NULL)
    {
        result = QCM_MQTT_RES_ERROR_NO_MEMORY;
        goto exit;
    }

    qosa_memset(rb, 0x00, sizeof(qcm_mqtt_request_t));

    if (MQTTPACKET_READ_COMPLETE != qcm_mqtt_serialize_subscribe(&(rb->buffer), &sub_pkt))
    {
        qosa_free(rb);
        result = QCM_MQTT_RES_ERROR_NO_MEMORY;
        goto exit;
    }
    rb->header.byte = sub_pkt.header.byte;
    rb->msg_id = sub_pkt.msg_id;
    rb->retry_cnt = client_ptr->user_option.delivery_cnt;

    qosa_q_link(rb, &rb->link);
    qosa_q_put(&client_ptr->out_packet_queue, &rb->link);

exit:
    qcm_mqtt_sub_data_free(sub_param_ptr);

    if (result == QCM_MQTT_RES_OK)
    {
        qcm_mqtt_client_report_clientid(QCM_MQTT_SEND_EVENT, client_ptr, QCM_MQTT_RES_OK);
    }
    else
    {
        qcm_mqtt_send_resp_t send_pkt = {0};

        send_pkt.msg_id = sub_pkt.msg_id;
        send_pkt.send_result = QCM_MQTT_SEND_ERROR;
        qcm_mqtt_client_response_commone(client_ptr, QCM_MQTT_CLIENT_SUBSCRIBE_EVENT, QCM_MQTT_RES_OK, &send_pkt);
    }
}

/**
 * @brief Perform data encoding
 */
static void qcm_mqtt_client_pub_encode_handler(void *argv)
{
    qcm_mqtt_eercode_e    result = QCM_MQTT_RES_OK;
    mqtt_publish_packet   pub_pkt;
    qcm_mqtt_request_t   *rb = QOSA_NULL;
    qcm_mqtt_pub_param_t *pub_param_ptr = (qcm_mqtt_pub_param_t *)argv;
    qcm_mqtt_client_t    *client_ptr = QOSA_NULL;
    qosa_uint16_t         msg_id = pub_param_ptr->msg_id;

    QOSA_ASSERT(pub_param_ptr != QOSA_NULL);

    client_ptr = qcm_mqtt_client_get_ptr(pub_param_ptr->client_id);
    if (client_ptr == QOSA_NULL)
    {
        return;
    }
    qosa_memset(&pub_pkt, 0x00, sizeof(mqtt_publish_packet));

    pub_pkt.version = client_ptr->user_option.version;
    pub_pkt.header.byte = 0;
    pub_pkt.header.bits.type = MQTT_PUBLISH;
    pub_pkt.header.bits.retain = pub_param_ptr->retain;
    pub_pkt.header.bits.qos = pub_param_ptr->qos;
    pub_pkt.header.bits.dup = 0;

    pub_pkt.msg_id = msg_id;
    pub_pkt.topic.data_len = pub_param_ptr->topic.data_len;
    pub_pkt.topic.data_ptr = pub_param_ptr->topic.data_ptr;
    pub_pkt.payload.data_len = pub_param_ptr->payload.data_len;
    pub_pkt.payload.data_ptr = pub_param_ptr->payload.data_ptr;
#ifdef CONFIG_QCM_MQTT5_FUNC
    if (pub_pkt.version == QCM_MQTT_VERSION_V5)
    {
        pub_pkt.props = &pub_param_ptr->props;
    }
#endif /* CONFIG_QCM_MQTT5_FUNC */

    if (client_ptr->client_state != QCM_MQTT_STATE_MQTT_CONNECTED)
    {
        result = QCM_MQTT_RES_ERROR_STATUS;
        goto exit;
    }

    if (qcm_mqtt_client_check_request_allow(client_ptr, msg_id) != QOSA_TRUE)
    {
        result = QCM_MQTT_RES_ERROR_MSG_ID_DUPLICATE;
        goto exit;
    }

    rb = qosa_malloc(sizeof(qcm_mqtt_request_t));
    if (rb == QOSA_NULL)
    {
        result = QCM_MQTT_RES_ERROR_NO_MEMORY;
        goto exit;
    }

    qosa_memset(rb, 0x00, sizeof(qcm_mqtt_request_t));

    if (MQTTPACKET_READ_COMPLETE != qcm_mqtt_serialize_publish(&(rb->buffer), &pub_pkt))
    {
        //The possible failure is the internal large buffer allocation failure, so there is no need to free the internal large buffer.
        qosa_free(rb);
        result = QCM_MQTT_RES_ERROR_NO_MEMORY;
        goto exit;
    }
    rb->header.byte = pub_pkt.header.byte;
    rb->msg_id = msg_id;
    rb->retry_cnt = client_ptr->user_option.delivery_cnt;

    qosa_q_link(rb, &rb->link);
    qosa_q_put(&client_ptr->out_packet_queue, &rb->link);

exit:
    qcm_mqtt_pub_data_free(pub_param_ptr);

    if (result == QCM_MQTT_RES_OK)
    {
        qcm_mqtt_client_report_clientid(QCM_MQTT_SEND_EVENT, client_ptr, QCM_MQTT_RES_OK);
    }
    else
    {
        qcm_mqtt_send_resp_t send_pkt = {0};

        send_pkt.send_result = QCM_MQTT_SEND_ERROR;
        qcm_mqtt_client_response_commone(client_ptr, QCM_MQTT_CLIENT_PUBLISH_EVENT, result, &send_pkt);
    }
}

/**
 * @brief MQTT disconnect data encoding
 */
static void qcm_mqtt_client_disc_encode_handler(void *argv)
{
    qcm_mqtt_eercode_e     result = QCM_MQTT_RES_OK;
    mqtt_disconnect_packet disc_pkt = {0};
    qcm_mqtt_request_t    *rb = QOSA_NULL;
    qcm_mqtt_disc_param_t *disc_ptr = (qcm_mqtt_disc_param_t *)argv;
    qcm_mqtt_client_t     *client_ptr = QOSA_NULL;

    QOSA_ASSERT(disc_ptr != QOSA_NULL);

    QLOGI("mqtt id:%d", disc_ptr->client_id);

    client_ptr = qcm_mqtt_client_get_ptr(disc_ptr->client_id);
    if (client_ptr == QOSA_NULL)
    {
        qosa_free(disc_ptr);
        return;
    }

    if (client_ptr->client_state != QCM_MQTT_STATE_MQTT_CONNECTED)
    {
        result = QCM_MQTT_RES_ERROR_STATUS;
        goto exit;
    }

    qosa_memset(&disc_pkt, 0x00, sizeof(mqtt_disconnect_packet));

    disc_pkt.header.byte = 0;
    disc_pkt.header.bits.type = MQTT_DISCONNECT;
    disc_pkt.version = client_ptr->user_option.version;
#ifdef CONFIG_QCM_MQTT5_FUNC
    disc_pkt.reason_code = disc_ptr->result;
    disc_pkt.props = &disc_ptr->props;
#endif /* CONFIG_QCM_MQTT5_FUNC */

    rb = qosa_malloc(sizeof(qcm_mqtt_request_t));
    if (rb == QOSA_NULL)
    {
        result = QCM_MQTT_RES_ERROR_NO_MEMORY;
        goto exit;
    }

    qosa_memset(rb, 0x00, sizeof(qcm_mqtt_request_t));

    if (MQTTPACKET_READ_COMPLETE != qcm_mqtt_serialize_disconnect(&(rb->buffer), &disc_pkt))
    {
        qosa_free(rb);
        result = QCM_MQTT_RES_ERROR_NO_MEMORY;
        goto exit;
    }
    rb->header.byte = disc_pkt.header.byte;
    rb->msg_id = 0;
    rb->retry_cnt = 1;

    qosa_q_link(rb, &rb->link);
    qosa_q_put(&client_ptr->out_packet_queue, &rb->link);

exit:
#ifdef CONFIG_QCM_MQTT5_FUNC
    qcm_mqtt_properties_free(&disc_ptr->props);
#endif /* CONFIG_QCM_MQTT5_FUNC */
    qosa_free(disc_ptr);

    if (result == QCM_MQTT_RES_OK)
    {
        qcm_mqtt_client_report_clientid(QCM_MQTT_SEND_EVENT, client_ptr, QCM_MQTT_RES_OK);
    }
    //qcm_mqtt_client_response_commone(client_ptr, QCM_MQTT_CLIENT_DISCONNECT_EVENT, result, QOSA_NULL);
}

/**
 * @brief Save the add information passed by the OPEN API interface, while checking the network and initiating a network registration request.
 */
static void qcm_mqtt_client_close_handler(void *argv)
{
    int                       ret = 0;
    qcm_mqtt_clientid_info_t *clientid_info_ptr = (qcm_mqtt_clientid_info_t *)argv;
    qcm_mqtt_client_t        *client_ptr = QOSA_NULL;
    qosa_uint8_t              client_id = 0;

    client_id = clientid_info_ptr->client_id;
    qosa_free(clientid_info_ptr);

    QLOGI("ENTER client_id=%d", client_id);
    client_ptr = qcm_mqtt_client_get_ptr(client_id);
    if (client_ptr == QOSA_NULL)
    {
        QLOGW("NULL");
        return;
    }

    if (client_ptr->client_state < QCM_MQTT_STATE_TCP_CONNECTED)
    {
        //Report OPEN URC
        //Report CLOSE execution success URC
        if (client_ptr->socketfd > 0)
        {
            qcm_socket_close(client_ptr->socketfd);
        }
        QLOGI("client state =%d", client_ptr->client_state);
        qcm_mqtt_client_response_commone(client_ptr, QCM_MQTT_CLIENT_OPEN_EVENT, QCM_MQTT_RES_ERROR_TCP_CONNECT, QOSA_NULL);
        qcm_mqtt_client_response_commone(client_ptr, QCM_MQTT_CLIENT_CLOSE_EVENT, QCM_MQTT_RES_OK, QOSA_NULL);
        return;
    }

    ret = qcm_socket_shutdown(client_ptr->socketfd, QCM_SHUT_RDWR);
    QLOGI("shutdown ret=%d", ret);
    if (ret == QCM_SOCK_SUCCESS)
    {
        qcm_socket_close(client_ptr->socketfd);
        qcm_mqtt_client_response_commone(client_ptr, QCM_MQTT_CLIENT_CLOSE_EVENT, QCM_MQTT_RES_OK, QOSA_NULL);
        qcm_mqtt_client_free(client_ptr);
    }
    else
    {
        client_ptr->client_state = QCM_MQTT_STATE_TCP_CLOSING;
        client_ptr->close_cause = QCM_MQTT_CLOSE_BY_ACIVE_CLOSE;
    }
}

/**
 * @brief TCP Data Transmission Processing Task
 */
static void qcm_mqtt_client_tcp_send_handler(void *argv)
{
    int                       send_len = 0;
    int                       ret = 0;
    qcm_mqtt_request_t       *rb = QOSA_NULL;
    qcm_mqtt_request_t       *tmp = QOSA_NULL;
    qcm_mqtt_clientid_info_t *clientid_info_ptr = (qcm_mqtt_clientid_info_t *)argv;
    qcm_mqtt_client_t        *client_ptr = QOSA_NULL;
    qosa_uint8_t              client_id = 0;
    qcm_mqtt_send_resp_t      send_resp = {0};

    client_id = clientid_info_ptr->client_id;
    qosa_free(clientid_info_ptr);

    client_ptr = qcm_mqtt_client_get_ptr(client_id);
    if (client_ptr == QOSA_NULL)
    {
        return;
    }

    //Each time, retrieve the pending transmission buffer data from the pending transmission buffer queue
    rb = qosa_q_check(&client_ptr->out_packet_queue);
    while (rb != QOSA_NULL)
    {
        send_len = rb->buffer.buf_len - rb->buffer.buf_pos;
        if (send_len <= 0)
        {
            tmp = qosa_q_next(&client_ptr->out_packet_queue, &rb->link);
            qosa_q_delete(&client_ptr->out_packet_queue, &rb->link);
            qcm_mqtt_free_data(&rb->buffer.buf_ptr);
            qosa_free(rb);
            rb = tmp;
            continue;
        }

        if (rb->header.bits.type == MQTT_CONNECT)
        {
            client_ptr->client_state = QCM_MQTT_STATE_MQTT_CONNECTING;
        }
        if (rb->header.bits.type == MQTT_DISCONNECT)
        {
            client_ptr->client_state = QCM_MQTT_STATE_MQTT_DISCONNECTING;
        }

        ret = qcm_socket_send(client_ptr->socketfd, rb->buffer.buf_ptr.data_ptr + rb->buffer.buf_pos, send_len);
        QLOGI("send ret=%d,send_len=%d", ret, send_len);
        if (ret < 0)
        {
            if (ret == QCM_SOCK_WODBLOCK)
            {
                //If write blocking occurs, then it should directly exit the while loop and wait for the TCP write event notification.
                break;
            }
            else
            {
                break;
            }
        }
        else if (ret == send_len)
        {
            client_ptr->send_last_time = qosa_get_system_time_milliseconds();
            QLOGV("send_last_time: %lld", client_ptr->send_last_time);

            //Send successful, waiting for ack on the pend_req_queue linked list
            rb->buffer.buf_pos = 0;
            //Record the write node information of rb, and delete the current node of rb.
            tmp = qosa_q_next(&client_ptr->out_packet_queue, &rb->link);
            qosa_q_delete(&client_ptr->out_packet_queue, &rb->link);

            if (rb->header.bits.type == MQTT_DISCONNECT)
            {
                /**
                 *  After the MQTT client actively sends a disconnect packet, a URC report is generated. This only occurs when the user actively closes the connection or when it is closed due to a protocol error.
                 */
                if (client_ptr->close_cause == QCM_MQTT_CLOSE_BY_ACIVE_CLOSE)
                {
                    qcm_mqtt_client_response_commone(client_ptr, QCM_MQTT_CLIENT_DISCONNECT_EVENT, QCM_MQTT_RES_OK, QOSA_NULL);
                }
                else
                {
                    qcm_mqtt_client_response_commone(client_ptr, QCM_MQTT_CLIENT_DISCONNECT_EVENT, QCM_MQTT_RES_ERROR_PROTOCOL_FAIL, QOSA_NULL);
                }

                qcm_mqtt_free_data(&rb->buffer.buf_ptr);
                qosa_free(rb);
                rb = QOSA_NULL;
                //Get the next message body to be sent from the queue
                rb = tmp;
                continue;
            }

            QLOGI("type: %d, qos: %d", rb->header.bits.type, rb->header.bits.qos);

            //If it is an MQTT publish message, due to the different restrictions of QoS 0, 1, and 2, timeout retransmission will not be performed when there are several pub ack and req headers; only the timeout check action will be carried out.
            //December 9, 2023 16:37:44 Disconnect also does not require retransmission, as there is no disconnect acknowledgment response in the first place.
            if ((rb->header.bits.type == MQTT_PUBLISH && rb->header.bits.qos == 0) || rb->header.bits.type == MQTT_PUBCOMP
                || rb->header.bits.type == MQTT_PUBACK || rb->header.bits.type == MQTT_PUBREC || rb->header.bits.type == MQTT_DISCONNECT)
            {
                rb->retry_cnt = 0;
            }

            QLOGV("retry_cnt:%d", rb->retry_cnt);
            if (rb->retry_cnt == 0 && rb->header.bits.type != MQTT_PINGREQ)
            {
                //If retry_cnt, then there is no need to wait for any response, and the rb can be released directly.
                if (rb->header.bits.type == MQTT_PUBLISH && rb->header.bits.qos == 0)
                {
                    //For QoS 0, it is not necessary to wait for the PUBACK to arrive before reporting a successful publication.
                    send_resp.msg_id = rb->msg_id;
                    send_resp.send_result = QCM_MQTT_SEND_OK;
                    qcm_mqtt_client_response_commone(client_ptr, QCM_MQTT_CLIENT_PUBLISH_EVENT, QCM_MQTT_RES_OK, &send_resp);
                }

                qcm_mqtt_free_data(&rb->buffer.buf_ptr);
                qosa_free(rb);
                rb = QOSA_NULL;

                //PUBLISH qos 0 also requires executing qcm_mqtt_client_set_min_diff_time
                //Since client_ptr->send_last_time = qosa_get_system_time_milliseconds() has already updated send_last_time
                //It is necessary for qcm_mqtt_client_set_min_diff_time to stop the currently running client_ptr->timer and restart a timer with kalive_time as the interval.

                //Here, qosa_q_put(&client_ptr->pend_req_queue, &rb->link) is not executed.
                //Therefore, in qcm_mqtt_client_set_min_diff_time, min_time_diff = client_ptr->user_option.kalive_time * 1000 is executed, restarting the timer with kalive_time as the interval.
            }
            else
            {
                //The rb data needs to be cached into the pending linked list, and the timer should be started simultaneously.
                if (rb->header.bits.type == MQTT_PINGREQ && client_ptr->user_option.kalive_time != 0)
                {
                    rb->expire_time = qosa_get_system_time_milliseconds() + client_ptr->user_option.kalive_time * 1000;
                }
                else
                {
                    rb->expire_time = qosa_get_system_time_milliseconds() + client_ptr->user_option.delivery_time * 1000;
                }
                qosa_q_link(rb, &rb->link);
                qosa_q_put(&client_ptr->pend_req_queue, &rb->link);

                QLOGI("expire_time : %lld", rb->expire_time);
            }

            qcm_mqtt_client_set_min_diff_time(client_ptr);

            //Get the next message body to be sent from the queue
            rb = tmp;
        }
        else  //TX BUFFER full, continue waiting for transmission on the client_ptr->output linked list
        {
            rb->buffer.buf_pos += ret;
        }
    }
}

/**
 * @brief For handling the core MQTT external event business.
 *
 * @param[in] void * argv
 *           - Global structure pointer for the MQTT client.
 */
static void qcm_mqtt_task_process_handler(void *argv)
{
    int                   ret = 0;
    qcm_mqtt_context_t   *mqtt_ctx_ptr = (qcm_mqtt_context_t *)argv;
    qcm_mqtt_event_info_t event_info = {QCM_MQTT_MIN, QOSA_NULL};

    while (1)
    {
        qosa_memset(&event_info, 0, sizeof(qcm_mqtt_event_info_t));
        ret = qosa_msgq_wait(mqtt_ctx_ptr->mqtt_msgq, (qosa_uint8_t *)&event_info, sizeof(qcm_mqtt_event_info_t), QOSA_WAIT_FOREVER);
        if (ret != QOSA_OK)
        {
            QLOGW("..");
            qosa_task_sleep_sec(1);
            continue;
        }

        QLOGI("task event : %d", event_info.cmd);

        switch (event_info.cmd)
        {
            case QCM_MQTT_SOCKET_EVENT: {
                qcm_mqtt_client_socket_event_handler(event_info.argv);
            }
            break;

            case QCM_MQTT_OPEN_EVNET: {
                qcm_mqtt_client_open_handler(event_info.argv);
                MQTT_FREE_PTR(event_info.argv);
            }
            break;

#ifdef CONFIG_QCM_VTLS_FUNC
            case QCM_MQTT_SSL_SHAKEHAND_EVNET: {
                qcm_mqtt_client_ssl_shakehand_handler(event_info.argv);
            }
            break;
#endif
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
            case QCM_MQTT_DATACALL_RESP_EVENT: {
                qcm_mqtt_client_datacall_resp_handler(event_info.argv);
            }
            break;
#endif
            case QCM_MQTT_DNS_REQUEST_EVENT: {
                qcm_mqtt_client_dns_parse_handler(event_info.argv);
            }
            break;

            case QCM_MQTT_DNS_RESPONSE_EVENT: {
                qcm_mqtt_client_dns_resp_handler(event_info.argv);
            }
            break;

            case QCM_MQTT_SOCKET_CREATE_EVENT: {
                qcm_mqtt_client_socket_create_handler(event_info.argv);
            }
            break;

            case QCM_MQTT_CONNECT_EVENT: {
                qcm_mqtt_client_conn_encode_handler(event_info.argv);
                MQTT_FREE_PTR(event_info.argv);
            }
            break;

            case QCM_MQTT_SUB_EVENT: {
                qcm_mqtt_client_sub_encode_handler(event_info.argv);
            }
            break;

            case QCM_MQTT_PUB_EVENT: {
                qcm_mqtt_client_pub_encode_handler(event_info.argv);
            }
            break;

            case QCM_MQTT_DISC_EVENT: {
                qcm_mqtt_client_disc_encode_handler(event_info.argv);
            }
            break;

            case QCM_MQTT_CLOSE_EVENT: {
                qcm_mqtt_client_close_handler(event_info.argv);
            }
            break;

            case QCM_MQTT_SEND_EVENT: {
                qcm_mqtt_client_tcp_send_handler(event_info.argv);
            }
            break;

            case QCM_MQTT_CYCLIC_TIME_EVENT: {
                qcm_mqtt_client_cyclic_timeout_handler(event_info.argv);
                MQTT_FREE_PTR(event_info.argv);
            }
            break;

            default:
                break;
        }
    }
}

/**
 *  @brief MQTT subscription or unsubscription packet encoding and processing function.
 *
 * @param[in] qosa_uint8_t client_id
 *           -MQTT client ID.
 * @param[in] qcm_mqtt_sub_config_t * sub_info
 *           - Topic information for subscription or unsubscription.
 * @param[in] qcm_mqtt_subscribe_e mode
 *           - Subscribe or unsubscribe mode.
 * @return qcm_mqtt_eercode_e
 *        - Returns MQTT_RES_OK to indicate success.
 *        - Returns other values from mqtt_eercode_e to indicate failure.
 */
static qcm_mqtt_eercode_e qcm_mqtt_send_sub_or_unsub(qosa_uint8_t client_id, qcm_mqtt_sub_config_t *sub_info, qcm_mqtt_subscribe_e mode)
{
    qcm_mqtt_sub_param_t *subscribe_param_ptr = QOSA_NULL;
    qosa_uint8_t          i = 0;
    qosa_uint8_t          qos_len = (mode == QCM_MQTT_SUBSCRIBE_MODE) ? MQTT_SUBSCRIBE_PACKET_QOS_LEN : 0;
    qcm_mqtt_sub_topic_t *topics = sub_info->topics;
    int                   ret = 0;
#ifdef CONFIG_QCM_MQTT5_FUNC
    qcm_mqtt_sub_properties_t *props = &sub_info->properties;
#endif /* CONFIG_QCM_MQTT5_FUNC */

    if (client_id > QCM_MQTT_MAX_CLIENT_NUMBER_ID || qcm_mqtt_client_check_id_is_exist(client_id) == QOSA_FALSE)
    {
        return QCM_MQTT_RES_ERROR_PARAM_INVALID;
    }

    //Check if the MQTT protocol connection is successful and ready for MQTT protocol data transmission.
    if (qcm_mqtt_client_is_protocol_conn_success(client_id) == QOSA_FALSE)
    {
        return QCM_MQTT_RES_ERROR_STATUS;
    }

    if ((sub_info->msg_id < QCM_MQTT_MIN_PACKET_ID) || (sub_info->msg_id > QCM_MQTT_MAX_PACKET_ID))
    {
        return QCM_MQTT_RES_ERROR_PARAM_INVALID;
    }

    subscribe_param_ptr = qosa_malloc(sizeof(qcm_mqtt_sub_param_t));
    if (subscribe_param_ptr == QOSA_NULL)
    {
        return QCM_MQTT_RES_ERROR_NO_MEMORY;
    }

    subscribe_param_ptr->client_id = client_id;
    qosa_q_init(&(subscribe_param_ptr->topic_list));
    subscribe_param_ptr->msg_id = sub_info->msg_id;
    subscribe_param_ptr->mode = mode;
    subscribe_param_ptr->packet_len = MQTT_PACKET_HEAD_LEN;

    for (i = 0; i < sub_info->topic_cnt; i++)
    {
        qcm_mqtt_list_topic_t *topic_ptr = QOSA_NULL;

        if (qcm_utf8_validate_string(topics[i].topic.data_ptr) == 0 || (topics[i].qos > QCM_MQTT_EXACTLY_ONCE_DELIVERY))
        {
            return QCM_MQTT_RES_ERROR_PARAM_INVALID;
        }

        topic_ptr = qosa_malloc(sizeof(qcm_mqtt_list_topic_t));
        if (topic_ptr == QOSA_NULL)
        {
            qosa_free(subscribe_param_ptr);
            return QCM_MQTT_RES_ERROR_NO_MEMORY;
        }
        topic_ptr->topic.data_len = topics[i].topic.data_len;

        if (mode == QCM_MQTT_SUBSCRIBE_MODE)
        {
            topic_ptr->qos = topics[i].qos;
#ifdef CONFIG_QCM_MQTT5_FUNC
            topic_ptr->opts.noLocal = topics[i].opts.noLocal;
            topic_ptr->opts.retainAsPublished = topics[i].opts.retainAsPublished;
            topic_ptr->opts.retainHandling = topics[i].opts.retainHandling;
#endif /* CONFIG_QCM_MQTT5_FUNC */
        }

        if (qcm_mqtt_malloc_data(&topic_ptr->topic, topic_ptr->topic.data_len, topics[i].topic.data_ptr) != QCM_MQTT_RES_OK)
        {
            qosa_free(subscribe_param_ptr);
            qosa_free(topic_ptr);
            return QCM_MQTT_RES_ERROR_NO_MEMORY;
        }

        subscribe_param_ptr->packet_len += MQTT_PACKET_LENGTH_HEAD_LEN + topic_ptr->topic.data_len + qos_len;
        qosa_q_link((void *)topic_ptr, &(topic_ptr->link));
        qosa_q_put(&(subscribe_param_ptr->topic_list), &(topic_ptr->link));
    }

#ifdef CONFIG_QCM_MQTT5_FUNC
    if (qcm_mqtt_client_get_version(client_id) == QCM_MQTT_VERSION_V5)
    {
        if (mode == QCM_MQTT_SUBSCRIBE_MODE && props->subscription_identifier > 0)
        {
            qcm_mqtt_data_to_property(&subscribe_param_ptr->props, QCM_MQTT_PROPERTY_CODE_SUBSCRIPTION_IDENTIFIER, &props->subscription_identifier);
        }

        for (i = 0; (i < props->user_property_cnt) && (i < QCM_MQTT_MAX_USER_PROPERTY_SUPPORT); i++)
        {
            qcm_mqtt_data_to_property(&subscribe_param_ptr->props, QCM_MQTT_PROPERTY_CODE_USER_PROPERTY, &props->user_property[i]);
        }
    }
#endif

    ret = qcm_mqtt_client_event_report(QCM_MQTT_SUB_EVENT, subscribe_param_ptr);
    if (ret != QCM_MQTT_RES_OK)
    {
        qosa_free(subscribe_param_ptr);
    }
    return ret;
}

/**
 *  @brief MQTT OPEN API interface.
 */

/**
 * @brief Apply for an MQTT client handle, only for the OPEN client group; STD directly uses the corresponding clientID from the upper layer for management.
 *
 * @return qosa_uint8_t
 *        - A return value greater than 0 indicates successful allocation of the MQTT client handle.
 *        - A return value equal to 0 indicates failure to allocate the MQTT client handle.
 */
qosa_uint8_t qcm_mqtt_client_create(void)
{
    qcm_mqtt_check_and_create_task();
    int i = QCM_MQTT_CLIENT_ID_OFFSET;
    for (i = QCM_MQTT_CLIENT_ID_OFFSET; i < QCM_MQTT_MAX_CLIENT_CNT; i++)
    {
        if (g_mqtt_context.client[i] == QOSA_NULL)
        {
            QLOGI("mqtt id:%d", i);
            return (i);
        }
    }

    return 0;
}

/**
 * @brief The user actively calls to load the default MQTT property configuration.
 *
 * @param[out] qcm_mqtt_config_t * config_ptr
 *           - Output the default MQTT configuration items
 *
 * @return qcm_mqtt_eercode_e
 *        - Returns MQTT_RES_OK to indicate success.
 *        - Returns other values from mqtt_eercode_e to indicate failure.
 */
qcm_mqtt_eercode_e qcm_mqtt_client_default_config(qcm_mqtt_config_t *config_ptr)
{
    if (config_ptr == QOSA_NULL)
    {
        return QCM_MQTT_RES_ERROR_PARAM_IS_NULL;
    }

    qosa_memset(config_ptr, 0, sizeof(qcm_mqtt_config_t));

    config_ptr->version = QCM_MQTT_DEFAULT_VERSION;
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
    config_ptr->pdp_cid = QCM_MQTT_DEFAULT_PDP_CID;
    config_ptr->sim_id = QCM_MQTT_DEFAULT_SIM_ID;
#endif
    config_ptr->clean_session = QCM_MQTT_DEFAULT_CLEAN_SESSION;
    config_ptr->kalive_time = QCM_MQTT_DEFAULT_KALIVE_TIME;
    config_ptr->delivery_time = QCM_MQTT_DEFAULT_DELIVERY_TIME;
    config_ptr->delivery_cnt = QCM_MQTT_DEFAULT_DELIVERY_CNT;
    config_ptr->will_flag = QCM_MQTT_DEFAULT_WILL_FLAG;
    config_ptr->will_qos = QCM_MQTT_DEFAULT_WILL_QOS;
    config_ptr->will_retain = QCM_MQTT_DEFAULT_WILL_RETAIN;
    config_ptr->max_recv_store_cnt = QCM_MQTT_MAX_STORE_MSG_CNT;
    config_ptr->rec_buf_size = QCM_MQTT_MAX_READ_BUFF_LEN;
    config_ptr->connect_time = QCM_MQTT_MAX_PDP_ACTIVE_TIMEOUT;
    config_ptr->close_ping_interval = QCM_MQTT_DEFAULT_CLOSE_TIME_INTERVAL;

    return QCM_MQTT_RES_OK;
}

/**
 * @brief Check MQTT client configuration.
 *
 * @param[in] qcm_mqtt_config_t *config_ptr
 *               - Configuration pointer, containing the maximum receive storage count, receive buffer size, and MQTT version number.
 */
static void qcm_mqtt_client_check_config(qcm_mqtt_config_t *config_ptr)
{
    if (config_ptr->max_recv_store_cnt == 0)
    {
        config_ptr->max_recv_store_cnt = QCM_MQTT_MAX_STORE_MSG_CNT;
    }

    if (config_ptr->rec_buf_size == 0)
    {
        config_ptr->rec_buf_size = QCM_MQTT_MAX_READ_BUFF_LEN;
    }

    if (config_ptr->version == 0)
    {
        config_ptr->version = QCM_MQTT_DEFAULT_VERSION;
    }

    return;
}

/**
 * @brief Set the corresponding client identifier for MQTT to initialize internal resource allocation
 *
 * @param[in] qosa_uint8_t client_id
 *           -MQTT client handle.
 *
 * @param[in] qcm_mqtt_config_t *config_ptr
 *           -PQTT configuration parameter structure pointer.
 *
 * @param[in] qcm_mqtt_client_event_cb evt_cb
 *           - Notification handler function when the MQTT client receives an event.
 *
 * @param[in] void * user_param
 *           - User information pointer, used during callback.
 *
 * @return qcm_mqtt_eercode_e
 *        - Returns MQTT_RES_OK to indicate success.
 *        - Returns other values from mqtt_eercode_e indicating failure.
 */
qcm_mqtt_eercode_e qcm_mqtt_client_init(qosa_uint8_t client_id, qcm_mqtt_config_t *config_ptr, qcm_mqtt_client_event_cb evt_cb, void *user_param)
{
    qcm_mqtt_eercode_e ret = QCM_MQTT_RES_OK;
    qcm_mqtt_client_t *client_ptr = QOSA_NULL;

    if (evt_cb == QOSA_NULL || client_id > QCM_MQTT_MAX_CLIENT_NUMBER_ID)
    {
        return QCM_MQTT_RES_ERROR_PARAM_INVALID;
    }

    qcm_mqtt_check_and_create_task();

    client_ptr = (qcm_mqtt_client_t *)qosa_malloc(sizeof(qcm_mqtt_client_t));
    if (client_ptr == QOSA_NULL)
    {
        return QCM_MQTT_RES_ERROR_NO_MEMORY;
    }
    qosa_memset(client_ptr, 0, sizeof(qcm_mqtt_client_t));

    ret = qcm_mqtt_client_set_ptr(client_id, client_ptr);
    if (ret != QCM_MQTT_RES_OK)
    {
        qosa_free(client_ptr);
        return ret;
    }

    if (config_ptr == QOSA_NULL)
    {
        qcm_mqtt_client_default_config(&(client_ptr->user_option));
    }
    else
    {
        client_ptr->user_option.version = config_ptr->version;
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
        client_ptr->user_option.pdp_cid = config_ptr->pdp_cid;
        client_ptr->user_option.sim_id = config_ptr->sim_id;
#endif
        client_ptr->user_option.clean_session = config_ptr->clean_session;
        client_ptr->user_option.kalive_time = config_ptr->kalive_time;
        client_ptr->user_option.delivery_time = config_ptr->delivery_time;
        client_ptr->user_option.delivery_cnt = config_ptr->delivery_cnt;
        client_ptr->user_option.will_flag = config_ptr->will_flag;
        client_ptr->user_option.will_qos = config_ptr->will_qos;
        client_ptr->user_option.will_retain = config_ptr->will_retain;
        client_ptr->user_option.connect_time = config_ptr->connect_time;
        client_ptr->user_option.close_ping_interval = config_ptr->close_ping_interval;
        client_ptr->user_option.rec_buf_size = config_ptr->rec_buf_size;
        client_ptr->user_option.max_recv_store_cnt = config_ptr->max_recv_store_cnt;
        client_ptr->user_option.connect_time = config_ptr->connect_time;
        qcm_mqtt_malloc_data(&client_ptr->user_option.will_topic, config_ptr->will_topic.data_len, config_ptr->will_topic.data_ptr);
        qcm_mqtt_malloc_data(&client_ptr->user_option.will_message, config_ptr->will_message.data_len, config_ptr->will_message.data_ptr);

        qcm_mqtt_client_check_config(&(client_ptr->user_option));

#ifdef CONFIG_QCM_VTLS_FUNC
        client_ptr->user_option.ssl_enable = config_ptr->ssl_enable;
        client_ptr->user_option.ssl_ctx_id = config_ptr->ssl_ctx_id;
        client_ptr->user_option.ssl_config = config_ptr->ssl_config;
#endif /* CONFIG_QCM_VTLS_FUNC */

#ifdef CONFIG_QCM_MQTT5_FUNC
        client_ptr->user_option.conn_properties.session_expiry_interval = config_ptr->conn_properties.session_expiry_interval;
        client_ptr->user_option.conn_properties.receive_maximum = config_ptr->conn_properties.receive_maximum;
        client_ptr->user_option.conn_properties.topic_alias_maximum = config_ptr->conn_properties.topic_alias_maximum;
        client_ptr->user_option.conn_properties.maximum_packet_size = config_ptr->conn_properties.maximum_packet_size;
        client_ptr->user_option.conn_properties.request_response_information = config_ptr->conn_properties.request_response_information;
        client_ptr->user_option.conn_properties.request_problem_information = config_ptr->conn_properties.request_problem_information;

        if (config_ptr->conn_properties.user_property_cnt != 0)
        {
            qosa_uint8_t i = 0;
            for (; (i < QCM_MQTT_MAX_USER_PROPERTY_SUPPORT) && (i < config_ptr->conn_properties.user_property_cnt); i++)
            {
                if (config_ptr->conn_properties.user_property[i].key.data_len != 0 && config_ptr->conn_properties.user_property[i].key.data_ptr != QOSA_NULL)
                {
                    qcm_mqtt_malloc_data(
                        &client_ptr->user_option.conn_properties.user_property[i].key,
                        config_ptr->conn_properties.user_property[i].key.data_len,
                        config_ptr->conn_properties.user_property[i].key.data_ptr
                    );
                }

                if (config_ptr->conn_properties.user_property[i].value.data_len != 0
                    && config_ptr->conn_properties.user_property[i].value.data_ptr != QOSA_NULL)
                {
                    qcm_mqtt_malloc_data(
                        &client_ptr->user_option.conn_properties.user_property[i].value,
                        config_ptr->conn_properties.user_property[i].value.data_len,
                        config_ptr->conn_properties.user_property[i].value.data_ptr
                    );
                }
                client_ptr->user_option.conn_properties.user_property_cnt++;
            }
        }

        if(client_ptr->user_option.will_flag == 1)
        {
            client_ptr->user_option.will_properties.will_delay_interval = config_ptr->will_properties.will_delay_interval;
            client_ptr->user_option.will_properties.message_expiry_interval = config_ptr->will_properties.message_expiry_interval;
            client_ptr->user_option.will_properties.payload_format_indicator = config_ptr->will_properties.payload_format_indicator;
            qcm_mqtt_malloc_data(
                &client_ptr->user_option.will_properties.content_type,
                config_ptr->will_properties.content_type.data_len,
                config_ptr->will_properties.content_type.data_ptr
            );
            qcm_mqtt_malloc_data(
                &client_ptr->user_option.will_properties.response_topic,
                config_ptr->will_properties.response_topic.data_len,
                config_ptr->will_properties.response_topic.data_ptr
            );
            qcm_mqtt_malloc_data(
                &client_ptr->user_option.will_properties.correlation_data,
                config_ptr->will_properties.correlation_data.data_len,
                config_ptr->will_properties.correlation_data.data_ptr
            );
        }
        qosa_q_init(&client_ptr->topic_alias_list);
#endif /* CONFIG_QCM_MQTT5_FUNC */
    }

    client_ptr->client_id = client_id;
    client_ptr->client_state = QCM_MQTT_STATE_INIT;
    client_ptr->event_cb = evt_cb;
    client_ptr->user_argv = user_param;
    client_ptr->read_event_cnt = 0;

#ifdef CONFIG_QCM_VTLS_FUNC
    QLOGI("ssl_enable : %d", client_ptr->user_option.ssl_enable);
#endif /* CONFIG_QCM_VTLS_FUNC */

    //Initialize the send queue
    qosa_q_init(&client_ptr->out_packet_queue);
    //Initialize pending queue
    qosa_q_init(&client_ptr->pend_req_queue);
    qosa_q_init(&client_ptr->new_msg_pub);
    qosa_q_init(&client_ptr->inboundMsgs);
    //Initialize timer
    qosa_timer_create(&client_ptr->timer, qcm_mqtt_client_timeout_cb, client_ptr);

    return ret;
}

/**
 * @brief For MQTT TCP connection requests.
 *
 * @param[in] qosa_uint8_t client_id
 *           -MQTT client handle.
 * @param[in] const char *host
 *           - The address of the MQTT server, which can be a domain name or an IP address.
 * @param[in] qosa_uint16_t port
 *           - Port number of the MQTT server.
 *
 * @return qcm_mqtt_eercode_e
 *        - Returns MQTT_RES_OK to indicate success.
 *        - Returns other values from mqtt_eercode_e indicating failure.
 */
qcm_mqtt_eercode_e qcm_mqtt_client_open(qosa_uint8_t client_id, char *addr, qosa_uint16_t port)
{
    qcm_mqtt_open_param_t *open_param_ptr = QOSA_NULL;
    qcm_mqtt_eercode_e     ret = QCM_MQTT_RES_OK;

    qcm_mqtt_check_and_create_task();

    if(addr == QOSA_NULL)
    {
        return QCM_MQTT_RES_ERROR_PARAM_INVALID;
    }
    if (qcm_socket_checkip_is_ip46(addr) == -1)
    {
        return QCM_MQTT_RES_ERROR_PARAM_INVALID;
    }

    if (qosa_strlen(addr) > CONFIG_QOSA_HOST_URL_MAX_LEN || (port == 0) || client_id > QCM_MQTT_MAX_CLIENT_NUMBER_ID
        || qcm_mqtt_client_check_id_is_exist(client_id) == QOSA_FALSE)
    {
        return QCM_MQTT_RES_ERROR_PARAM_INVALID;
    }
    QLOGV("addr: %s port: %d", addr, port);

    open_param_ptr = (qcm_mqtt_open_param_t *)qosa_malloc(sizeof(qcm_mqtt_open_param_t));
    if (open_param_ptr == QOSA_NULL)
    {
        return QCM_MQTT_RES_ERROR_NO_MEMORY;
    }
    qosa_memset(open_param_ptr, 0, sizeof(qcm_mqtt_open_param_t));

    open_param_ptr->client_id = client_id;
    open_param_ptr->port = port;
    qosa_memcpy(open_param_ptr->addr, addr, qosa_strlen(addr));

    ret = qcm_mqtt_client_event_report(QCM_MQTT_OPEN_EVNET, open_param_ptr);
    if (ret != QCM_MQTT_RES_OK)
    {
        qosa_free(open_param_ptr);
    }

    return ret;
}

/**
 * @brief Used after the MQTT TCP connection is successfully established, the user actively calls the API to send an MQTT protocol connect request.
 *
 * @param[in] qosa_uint8_t client_id
 *          - MQTT client handle.
 *
 * @param[in] const char * client_name
 *          - Pointer to the string of the MQTT client identifier.
 *
 * @param[in] int client_name_len
 *          - Corresponding client identifier string length
 *
 * @param[in] const char * username
 *          - Pointer to the string for the MQTT client username.
 *
 * @param[in] int username_len
 *          - Corresponding name string length
 *
 * @param[in] const char * password
 *          - Pointer to the string of the MQTT client password.
 *
 * @param[in] int password_len
 *          - Corresponding password length
 *
 * @return qcm_mqtt_eercode_e
 *        - Returns MQTT_RES_OK to indicate success.
 *        - Returns other values from mqtt_eercode_e indicating failure.
 */
qcm_mqtt_eercode_e qcm_mqtt_client_connect(
    qosa_uint8_t client_id,
    const char  *client_name,
    int          client_name_len,
    const char  *username,
    int          username_len,
    const char  *password,
    int          password_len
)
{
    qcm_mqtt_conn_param_t *conn_param_ptr = QOSA_NULL;
    qcm_mqtt_eercode_e     ret = QCM_MQTT_RES_ERROR_NO_MEMORY;
    qcm_mqtt_client_t     *client_ptr = QOSA_NULL;

    if (client_id > QCM_MQTT_MAX_CLIENT_NUMBER_ID || qcm_mqtt_client_check_id_is_exist(client_id) == QOSA_FALSE)
    {
        return QCM_MQTT_RES_ERROR_PARAM_INVALID;
    }

    qcm_mqtt_check_and_create_task();
    //Perform state judgment here; do not leave state judgment to the user's control. If the call is not in a successfully connected state, it should directly return failure.
    qosa_mutex_lock(g_mqtt_context.mutex, QOSA_WAIT_FOREVER);
    client_ptr = qcm_mqtt_client_get_ptr(client_id);
    if (client_ptr == QOSA_NULL)
    {
        qosa_mutex_unlock(g_mqtt_context.mutex);
        return QCM_MQTT_RES_ERROR_STATUS;
    }

    if (client_ptr->client_state != QCM_MQTT_STATE_TCP_CONNECTED)
    {
        qosa_mutex_unlock(g_mqtt_context.mutex);
        return QCM_MQTT_RES_ERROR_STATUS;
    }
    qosa_mutex_unlock(g_mqtt_context.mutex);

    conn_param_ptr = (qcm_mqtt_conn_param_t *)qosa_malloc(sizeof(qcm_mqtt_conn_param_t));
    if (conn_param_ptr == QOSA_NULL)
    {
        return QCM_MQTT_RES_ERROR_NO_MEMORY;
    }

    conn_param_ptr->client_id = client_id;
    if (qcm_mqtt_malloc_data(&conn_param_ptr->client_name, client_name_len, (char *)client_name) != QCM_MQTT_RES_OK)
    {
        goto exit;
    }

    QLOGV("username:%x", username);
    QLOGV("password:%x", password);

    if (username != QOSA_NULL)
    {
        QLOGV("username:%d", username_len);

        if (qcm_mqtt_malloc_data(&conn_param_ptr->username, username_len, (char *)username) != QCM_MQTT_RES_OK)
        {
            goto exit;
        }
    }
    if ((
#ifdef CONFIG_QCM_MQTT5_FUNC
            qcm_mqtt_client_get_version(client_id) == QCM_MQTT_VERSION_V5 ||
#endif /* CONFIG_QCM_MQTT5_FUNC */
            username != QOSA_NULL
        )
        && password != QOSA_NULL)
    {
        QLOGV("password:%d", password_len);

        if (qcm_mqtt_malloc_data(&conn_param_ptr->password, password_len, (char *)password) != QCM_MQTT_RES_OK)
        {
            goto exit;
        }
    }

    ret = qcm_mqtt_client_event_report(QCM_MQTT_CONNECT_EVENT, conn_param_ptr);

exit:
    if (ret != QCM_MQTT_RES_OK)
    {
        if (conn_param_ptr != QOSA_NULL)
        {
            qcm_mqtt_free_data(&conn_param_ptr->client_name);
            qcm_mqtt_free_data(&conn_param_ptr->username);
            qcm_mqtt_free_data(&conn_param_ptr->password);
            qosa_free(conn_param_ptr);
        }
    }
    return ret;
}

#ifdef CONFIG_QCM_MQTT5_FUNC
/**
 *  @brief Retrieve the information from the connect ack sent by the server
 *
 * @param[in] qosa_uint8_t client_id
 *           - MQTT client handle.
 * @param[out] qcm_mqtt_connack_properties_t * conn_props
 *           - The attribute information in the server's CONNACK reply.
 * @return qcm_mqtt_eercode_e
 *        - Returns MQTT_RES_OK to indicate success.
 *        - Returns other values from mqtt_eercode_e indicating failure.
 */
qcm_mqtt_eercode_e qcm_mqtt_client_get_server_info(qosa_uint8_t client_id, qcm_mqtt_connack_properties_t *conn_props)
{
    qcm_mqtt_client_t *client_ptr = QOSA_NULL;
    qcm_mqtt_check_and_create_task();

    if (conn_props == QOSA_NULL)
    {
        return QCM_MQTT_RES_ERROR_PARAM_INVALID;
    }

    client_ptr = (qcm_mqtt_client_t *)qcm_mqtt_client_get_ptr(client_id);
    if (client_ptr == QOSA_NULL)
    {
        return QCM_MQTT_RES_ERROR_PARAM_INVALID;
    }

    if (client_ptr->connack_props != QOSA_NULL)
    {
        conn_props->session_expiry_interval = client_ptr->connack_props->session_expiry_interval;
        conn_props->receive_maximum = client_ptr->connack_props->receive_maximum;
        conn_props->maximum_qos = client_ptr->connack_props->maximum_qos;
        conn_props->retain_available = client_ptr->connack_props->retain_available;
        conn_props->maximum_packet_size = client_ptr->connack_props->maximum_packet_size;
        conn_props->topic_alias_maximum = client_ptr->connack_props->topic_alias_maximum;
        conn_props->wildcard_subscription_available = client_ptr->connack_props->wildcard_subscription_available;
        conn_props->subscription_identifier_available = client_ptr->connack_props->subscription_identifier_available;
        conn_props->shared_subscription_available = client_ptr->connack_props->shared_subscription_available;
        conn_props->server_keep_alive = client_ptr->connack_props->server_keep_alive;

        QLOGI("session_expiry_interval:%d", client_ptr->connack_props->session_expiry_interval);
        QLOGV("receive_maximum:%d", client_ptr->connack_props->receive_maximum);
        QLOGV("maximum_qos:%d", client_ptr->connack_props->maximum_qos);
        QLOGV("retain_available:%d", client_ptr->connack_props->retain_available);
        QLOGV("maximum_packet_size:%d", client_ptr->connack_props->maximum_packet_size);
        QLOGV("topic_alias_maximum:%d", client_ptr->connack_props->topic_alias_maximum);
        QLOGV("wildcard_subscription_available:%d", client_ptr->connack_props->wildcard_subscription_available);
        QLOGV("subscription_identifier_available:%d", client_ptr->connack_props->subscription_identifier_available);
        QLOGV("shared_subscription_available:%d", client_ptr->connack_props->shared_subscription_available);
        QLOGV("server_keep_alive:%d", client_ptr->connack_props->server_keep_alive);

        if (qcm_mqtt_malloc_data(
                &conn_props->response_information,
                client_ptr->connack_props->response_information.data_len,
                client_ptr->connack_props->response_information.data_ptr
            )
            != QCM_MQTT_RES_OK)
        {
            goto exit;
        }

        if (client_ptr->connack_props->user_property_cnt != 0)
        {
            qosa_uint8_t i = 0;
            for (; i < conn_props->user_property_cnt; i++)
            {
                if (client_ptr->connack_props->user_property[i].key.data_len != 0)
                {
                    if (qcm_mqtt_malloc_data(
                            &conn_props->user_property[i].key,
                            client_ptr->connack_props->user_property[i].key.data_len,
                            client_ptr->connack_props->user_property[i].key.data_ptr
                        )
                        != QCM_MQTT_RES_OK)
                    {
                        goto exit;
                    }
                }

                if (client_ptr->connack_props->user_property[i].value.data_len != 0)
                {
                    if (qcm_mqtt_malloc_data(
                            &conn_props->user_property[i].value,
                            client_ptr->connack_props->user_property[i].value.data_len,
                            client_ptr->connack_props->user_property[i].value.data_ptr
                        )
                        != QCM_MQTT_RES_OK)
                    {
                        goto exit;
                    }
                }

                conn_props->user_property_cnt++;
            }
        }
    }

    return QCM_MQTT_RES_OK;

exit:
    qcm_mqtt_release_connack_props(conn_props);
    return QCM_MQTT_RES_ERROR_NO_MEMORY;
}
#endif /* CONFIG_QCM_MQTT5_FUNC */

/**
 * @brief MQTT client publishes and subscribes to topic requests.
 *
 * @param[in] qosa_uint8_t client_id
 *           -MQTT client handle.
 * @param[in] qcm_mqtt_sub_config_t * sub_info
 *          - Pointer to the structure for subscribing to topic information, containing details such as the topic list and the number of topics.
 *
 * @return qcm_mqtt_eercode_e
 *        - Returns MQTT_RES_OK to indicate success.
 *        - Returns other values from mqtt_eercode_e indicating failure.
 */
qcm_mqtt_eercode_e qcm_mqtt_client_subscribe(qosa_uint8_t client_id, qcm_mqtt_sub_config_t *sub_info)
{
    qcm_mqtt_check_and_create_task();

    if (sub_info == QOSA_NULL || sub_info->topics == QOSA_NULL || sub_info->topic_cnt == 0)
    {
        return QCM_MQTT_RES_ERROR_PARAM_IS_NULL;
    }

    return qcm_mqtt_send_sub_or_unsub(client_id, sub_info, QCM_MQTT_SUBSCRIBE_MODE);
}

/**
 * @brief The MQTT client sends a request to unsubscribe from a topic.
 *
 * @param[in] qosa_uint8_t client_id
 *           - MQTT client handle.
 * @param[in] qcm_mqtt_sub_config_t * sub_info
 *          - Pointer to the structure for subscribing to topic information, containing details such as the topic list and the number of topics.
 *
 * @return qcm_mqtt_eercode_e
 *       - Returns MQTT_RES_OK to indicate success.
 *       - Returns other values from mqtt_eercode_e indicate failure.
 */
qcm_mqtt_eercode_e qcm_mqtt_client_unsubscribe(qosa_uint8_t client_id, qcm_mqtt_sub_config_t *sub_info)
{
    qcm_mqtt_check_and_create_task();

    if (sub_info == QOSA_NULL)
    {
        return QCM_MQTT_RES_ERROR_PARAM_IS_NULL;
    }

    if (sub_info->topics == QOSA_NULL || sub_info->topic_cnt == 0)
    {
        return QCM_MQTT_RES_OK;
    }

    return qcm_mqtt_send_sub_or_unsub(client_id, sub_info, QCM_MQTT_UNSUBSCRIBE_MODE);
}

/**
 * @brief The MQTT client actively publishes business messages.
 *
 * @param[in] qosa_uint8_t client_id
 *           -MQTT client handle.
 * @param[in] qcm_mqtt_pub_config_t * pub_info
 *           -MQTT PUB configuration structure.
 *
 * @return qcm_mqtt_eercode_e
 *        - Returns MQTT_RES_OK to indicate success.
 *        - Returns other values from mqtt_eercode_e indicating failure.
 */
qcm_mqtt_eercode_e qcm_mqtt_client_publish(qosa_uint8_t client_id, qcm_mqtt_pub_config_t *pub_info)
{
    qcm_mqtt_eercode_e    ret = QCM_MQTT_RES_OK;
    qcm_mqtt_pub_param_t *publish_param_ptr = QOSA_NULL;

    QLOGI("msg_id:%d", client_id);
    qcm_mqtt_check_and_create_task();

    if (client_id > QCM_MQTT_MAX_CLIENT_NUMBER_ID || qcm_mqtt_client_check_id_is_exist(client_id) == QOSA_FALSE)
    {
        return QCM_MQTT_RES_ERROR_PARAM_INVALID;
    }

    if ((pub_info->msg_id < QCM_MQTT_MIN_PACKET_ID) || (pub_info->msg_id > QCM_MQTT_MAX_PACKET_ID))
    {
        return QCM_MQTT_RES_ERROR_PARAM_INVALID;
    }

    if (pub_info->topic.data_len == 0 || pub_info->topic.data_ptr == QOSA_NULL)
    {
#ifdef CONFIG_QCM_MQTT5_FUNC
        if (!((qcm_mqtt_client_get_version(client_id) >= QCM_MQTT_VERSION_V5) && (pub_info->properties.topic_alias > 0)
              && qcm_mqtt_topic_alias_check(client_id, pub_info->properties.topic_alias) == QCM_MQTT_RES_OK))
#endif /* CONFIG_QCM_MQTT5_FUNC */
        {
            return QCM_MQTT_RES_ERROR_PARAM_INVALID;
        }
    }
    else
    {
        if (qcm_utf8_validate_string(pub_info->topic.data_ptr) == 0)
        {
            return QCM_MQTT_RES_ERROR_PARAM_INVALID;
        }
    }

    if (pub_info->qos > QCM_MQTT_EXACTLY_ONCE_DELIVERY)
    {
        return QCM_MQTT_RES_ERROR_PARAM_INVALID;
    }

    //Check if the MQTT protocol connection is successful and ready for MQTT protocol data transmission.
    if (qcm_mqtt_client_is_protocol_conn_success(client_id) == QOSA_FALSE)
    {
        return QCM_MQTT_RES_ERROR_STATUS;
    }

    publish_param_ptr = qosa_malloc(sizeof(qcm_mqtt_pub_param_t));
    if (publish_param_ptr == QOSA_NULL)
    {
        return QCM_MQTT_RES_ERROR_NO_MEMORY;
    }

    publish_param_ptr->client_id = client_id;
    publish_param_ptr->msg_id = pub_info->msg_id;
    publish_param_ptr->qos = pub_info->qos;
    publish_param_ptr->retain = pub_info->retain;

    if ((pub_info->topic.data_len != 0 && pub_info->topic.data_ptr != QOSA_NULL)
        && qcm_mqtt_malloc_data(&publish_param_ptr->topic, pub_info->topic.data_len, pub_info->topic.data_ptr) != QCM_MQTT_RES_OK)
    {
        ret = QCM_MQTT_RES_ERROR_NO_MEMORY;
        goto exit;
    }

    if (qcm_mqtt_malloc_data(&publish_param_ptr->payload, pub_info->payload.data_len, pub_info->payload.data_ptr) != QCM_MQTT_RES_OK)
    {
        ret = QCM_MQTT_RES_ERROR_NO_MEMORY;
        goto exit;
    }

#ifdef CONFIG_QCM_MQTT5_FUNC
    if (qcm_mqtt_client_get_version(client_id) == QCM_MQTT_VERSION_V5)
    {
        if (pub_info->properties.message_expiry_interval != 0)
        {
            qcm_mqtt_data_to_property(&publish_param_ptr->props, QCM_MQTT_PROPERTY_CODE_MESSAGE_EXPIRY_INTERVAL, &pub_info->properties.message_expiry_interval);
        }

        if (pub_info->properties.subscription_identifier != 0)
        {
            qcm_mqtt_data_to_property(&publish_param_ptr->props, QCM_MQTT_PROPERTY_CODE_SUBSCRIPTION_IDENTIFIER, &pub_info->properties.subscription_identifier);
        }

        if (pub_info->properties.topic_alias != 0)
        {
            qcm_mqtt_data_to_property(&publish_param_ptr->props, QCM_MQTT_PROPERTY_CODE_TOPIC_ALIAS, &pub_info->properties.topic_alias);
        }

        if (pub_info->properties.payload_format_indicator != 0)
        {
            qcm_mqtt_data_to_property(
                &publish_param_ptr->props,
                QCM_MQTT_PROPERTY_CODE_PAYLOAD_FORMAT_INDICATOR,
                &pub_info->properties.payload_format_indicator
            );
        }

        if (pub_info->properties.response_topic.data_len != 0 && pub_info->properties.response_topic.data_ptr != QOSA_NULL)
        {
            qcm_mqtt_data_to_property(&publish_param_ptr->props, QCM_MQTT_PROPERTY_CODE_RESPONSE_TOPIC, &pub_info->properties.response_topic);
        }

        if (pub_info->properties.content_type.data_len != 0 && pub_info->properties.content_type.data_ptr != QOSA_NULL)
        {
            qcm_mqtt_data_to_property(&publish_param_ptr->props, QCM_MQTT_PROPERTY_CODE_CONTENT_TYPE, &pub_info->properties.content_type);
        }

        if (pub_info->properties.correlation_data.data_len != 0 && pub_info->properties.correlation_data.data_ptr != QOSA_NULL)
        {
            qcm_mqtt_data_to_property(&publish_param_ptr->props, QCM_MQTT_PROPERTY_CODE_CORRELATION_DATA, &pub_info->properties.correlation_data);
        }

        qosa_uint8_t i = 0;
        for (; (i < pub_info->properties.user_property_cnt) && (i < QCM_MQTT_MAX_USER_PROPERTY_SUPPORT); i++)
        {
            qcm_mqtt_data_to_property(&publish_param_ptr->props, QCM_MQTT_PROPERTY_CODE_USER_PROPERTY, &pub_info->properties.user_property[i]);
        }
    }
#endif

    ret = qcm_mqtt_client_event_report(QCM_MQTT_PUB_EVENT, publish_param_ptr);

exit:
    if (ret != QCM_MQTT_RES_OK)
    {
        if (publish_param_ptr != QOSA_NULL)
        {
            qcm_mqtt_free_data(&publish_param_ptr->topic);
            qcm_mqtt_free_data(&publish_param_ptr->payload);
            qosa_free(publish_param_ptr);
        }
    }

    return ret;
}

/**
 * @brief The MQTT client closes the TCP connection with the server.
 *
 * @param[in] qosa_uint8_t client_id
 *           -MQTT client handle.
 *
 * @return qcm_mqtt_eercode_e
 *        - Returns MQTT_RES_OK to indicate success.
 *        - Returns other values from mqtt_eercode_e indicating failure.
 */
qcm_mqtt_eercode_e qcm_mqtt_client_disconnect(qosa_uint8_t client_id, qcm_mqtt_disc_config_t *disconn_info)
{
    QOSA_UNUSED(disconn_info);
    qcm_mqtt_disc_param_t *disc_ptr = QOSA_NULL;
    qcm_mqtt_check_and_create_task();

    if (client_id > QCM_MQTT_MAX_CLIENT_NUMBER_ID || qcm_mqtt_client_check_id_is_exist(client_id) == QOSA_FALSE)
    {
        return QCM_MQTT_RES_ERROR_PARAM_INVALID;
    }

    //Check if the MQTT protocol connection is successful and ready for MQTT protocol data transmission.
    if (qcm_mqtt_client_is_protocol_conn_success(client_id) == QOSA_FALSE)
    {
        return QCM_MQTT_RES_ERROR_STATUS;
    }

    disc_ptr = qosa_malloc(sizeof(qcm_mqtt_disc_param_t));
    if (disc_ptr == QOSA_NULL)
    {
        return QCM_MQTT_RES_ERROR_NO_MEMORY;
    }
    disc_ptr->client_id = client_id;

#ifdef CONFIG_QCM_MQTT5_FUNC
    if (qcm_mqtt_client_get_version(client_id) == QCM_MQTT_VERSION_V5 && disconn_info != QOSA_NULL)
    {
        if (disconn_info->properties.session_expiry_interval != 0)
        {
            qcm_mqtt_data_to_property(&disc_ptr->props, QCM_MQTT_PROPERTY_CODE_SESSION_EXPIRY_INTERVAL, &disconn_info->properties.session_expiry_interval);
        }

        qosa_uint8_t i = 0;
        for (; (i < disconn_info->properties.user_property_cnt) && (i < QCM_MQTT_MAX_USER_PROPERTY_SUPPORT); i++)
        {
            qcm_mqtt_data_to_property(&disc_ptr->props, QCM_MQTT_PROPERTY_CODE_USER_PROPERTY, &disconn_info->properties.user_property[i]);
        }
    }
#endif

    if (qcm_mqtt_client_event_report(QCM_MQTT_DISC_EVENT, disc_ptr) != QCM_MQTT_RES_OK)
    {
        qosa_free(disc_ptr);
        return QCM_MQTT_RES_ERROR_TASK;
    }

    return QCM_MQTT_RES_OK;
}

/**
 * @brief The MQTT client closes the TCP connection with the server.
 *
 * @param[in] qosa_uint8_t client_id
 *           -MQTT client handle.
 *
 * @return qcm_mqtt_eercode_e
 *        - Returns MQTT_RES_OK to indicate success.
 *        - Returns other values from mqtt_eercode_e to indicate failure.
 */
qcm_mqtt_eercode_e qcm_mqtt_client_close(qosa_uint8_t client_id)
{
    qcm_mqtt_clientid_info_t *clientid_info_ptr = QOSA_NULL;
    qcm_mqtt_client_t        *client_ptr = QOSA_NULL;
    qcm_mqtt_check_and_create_task();

    if (client_id > QCM_MQTT_MAX_CLIENT_NUMBER_ID || qcm_mqtt_client_check_id_is_exist(client_id) == QOSA_FALSE)
    {
        return QCM_MQTT_RES_ERROR_PARAM_INVALID;
    }

    //Perform state judgment here; do not leave state judgment to the user's control. If the call is not in a successfully connected state, it should directly return failure.
    qosa_mutex_lock(g_mqtt_context.mutex, QOSA_WAIT_FOREVER);
    client_ptr = qcm_mqtt_client_get_ptr(client_id);
    if (client_ptr == QOSA_NULL)
    {
        qosa_mutex_unlock(g_mqtt_context.mutex);
        return QCM_MQTT_RES_ERROR_STATUS;
    }

    if (client_ptr->client_state <= QCM_MQTT_STATE_TCP_CONNECTING || client_ptr->client_state >= QCM_MQTT_STATE_TCP_CLOSING)
    {
        qosa_mutex_unlock(g_mqtt_context.mutex);
        return QCM_MQTT_RES_ERROR_STATUS;
    }
    qosa_mutex_unlock(g_mqtt_context.mutex);

    clientid_info_ptr = qosa_malloc(sizeof(qcm_mqtt_clientid_info_t));
    if (clientid_info_ptr == QOSA_NULL)
    {
        return QCM_MQTT_RES_ERROR_NO_MEMORY;
    }
    clientid_info_ptr->client_id = client_id;

    if (qcm_mqtt_client_event_report(QCM_MQTT_CLOSE_EVENT, clientid_info_ptr) != QCM_MQTT_RES_OK)
    {
        qosa_free(clientid_info_ptr);
        return QCM_MQTT_RES_ERROR_TASK;
    }

    return QCM_MQTT_RES_OK;
}

/**
 * @brief Read the message published to the subscribed topic received by the MQTT client.
 *
 * @param[in] qosa_uint8_t client_id
 *           -MQTT client handle.
 *
 * @param[in] qosa_uint8_t store_id
 *           - Handle to client cached data.
 *
 * @param[out] int *msg_id
 *            - Read the message identifier for the data received by the client.
 *
 * @param[out] char **topic
 *            - Read the pointer address of the topic string for this data received by the client.
 *
 * @param[out] char **payload
 *            - Pointer address of the message string received by the client this time.
 *
 * @param[out] qosa_uint32_t *payload_len
 *            - Read the length of the message string received by the client this time.
 *
 * @return qcm_mqtt_eercode_e
 *        - Returns MQTT_RES_OK to indicate success.
 *        - Returns other values from mqtt_eercode_e indicating failure.
 */
qcm_mqtt_eercode_e qcm_mqtt_client_read_subcribe_message(qosa_uint8_t client_id, qosa_uint8_t store_id, qcm_mqtt_recv_pub_t *recv_pub)
{
    qcm_mqtt_client_t      *client_ptr = QOSA_NULL;
    qcm_mqtt_new_message_t *new_msg_ptr = QOSA_NULL;
    qosa_uint8_t            find_store = store_id;
    qcm_mqtt_check_and_create_task();

    client_ptr = (qcm_mqtt_client_t *)qcm_mqtt_client_get_ptr(client_id);
    if (client_ptr == QOSA_NULL)
    {
        return QCM_MQTT_RES_ERROR_PARAM_IS_NULL;
    }

    qosa_mutex_lock(g_mqtt_context.mutex, QOSA_WAIT_FOREVER);

    new_msg_ptr = qosa_q_linear_search(&client_ptr->new_msg_pub, qcm_mqtt_client_compare_store_id, (void *)&find_store);
    if (new_msg_ptr == QOSA_NULL)
    {
        QLOGW("new_msg_ptr inexistence!");
        qosa_mutex_unlock(g_mqtt_context.mutex);
        return QCM_MQTT_RES_ERROR_STORE_FAIL;
    }

    recv_pub->msg_id = new_msg_ptr->new_msg.msg_id;
    recv_pub->topic.data_len = new_msg_ptr->new_msg.topic.data_len;
    recv_pub->topic.data_ptr = new_msg_ptr->new_msg.topic.data_ptr;
    recv_pub->payload.data_len = new_msg_ptr->new_msg.payload.data_len;
    recv_pub->payload.data_ptr = new_msg_ptr->new_msg.payload.data_ptr;

#ifdef CONFIG_QCM_MQTT5_FUNC
    recv_pub->properties = new_msg_ptr->new_msg.options;
#endif /* CONFIG_QCM_MQTT5_FUNC */

    qosa_q_delete(&client_ptr->new_msg_pub, &new_msg_ptr->link);
    qosa_free(new_msg_ptr);
    qosa_mutex_unlock(g_mqtt_context.mutex);

    return QCM_MQTT_RES_OK;
}

/**
 * @brief Calculate how much unread data is in the buff cache.
 *
 * @param[in] qosa_uint8_t client_id
 *           -MQTT client handle.
 * @return qosa_uint8_t
 *        - Returns the number of data bytes available for reading from the corresponding client.
 */
qosa_uint32_t qcm_mqtt_client_wait_read_cnt(qosa_uint8_t client_id)
{
    qcm_mqtt_client_t *client_ptr = QOSA_NULL;
    qcm_mqtt_check_and_create_task();

    client_ptr = (qcm_mqtt_client_t *)qcm_mqtt_client_get_ptr(client_id);
    if (client_ptr == QOSA_NULL)
    {
        return 0;
    }

    return qosa_q_cnt(&client_ptr->new_msg_pub);
}

/**
 * @brief Read the cached data from the corresponding MQTT client.
 *
 * @param[in] qosa_uint8_t client_id
 *           -MQTT client handle.
 * @param[out] int *msg_id
 *            - Read the message identifier for the data received by the client.
 * @param[out] char **topic
 *            - Read the pointer address of the topic string for this data received by the client.
 * @param[out] char **payload
 *            - Pointer address of the message string received by the client this time.
 * @param[out] qosa_uint32_t *payload_len
 *            - Read the length of the message string received by the client this time.
 * @return qcm_mqtt_eercode_e
 *        - Returns MQTT_RES_OK to indicate success.
 *        - Returns other values from mqtt_eercode_e to indicate failure.
 */
qcm_mqtt_eercode_e qcm_mqtt_client_read_cacha_data(qosa_uint8_t client_id, qcm_mqtt_recv_pub_t *recv_pub)
{
    qcm_mqtt_client_t      *client_ptr = QOSA_NULL;
    qcm_mqtt_new_message_t *new_msg_ptr = QOSA_NULL;
    qcm_mqtt_check_and_create_task();

    client_ptr = (qcm_mqtt_client_t *)qcm_mqtt_client_get_ptr(client_id);
    if (client_ptr == QOSA_NULL)
    {
        return QCM_MQTT_RES_ERROR_PARAM_IS_NULL;
    }

    if (qosa_q_cnt(&client_ptr->new_msg_pub) == 0)
    {
        return QCM_MQTT_RES_NOT_READ_DATA;
    }

    qosa_mutex_lock(g_mqtt_context.mutex, QOSA_WAIT_FOREVER);

    new_msg_ptr = qosa_q_get(&client_ptr->new_msg_pub);
    if (new_msg_ptr == QOSA_NULL)
    {
        qosa_mutex_unlock(g_mqtt_context.mutex);
        return QCM_MQTT_RES_ERROR_STORE_FAIL;
    }

    recv_pub->msg_id = new_msg_ptr->new_msg.msg_id;
    recv_pub->topic.data_len = new_msg_ptr->new_msg.topic.data_len;
    recv_pub->topic.data_ptr = new_msg_ptr->new_msg.topic.data_ptr;
    recv_pub->payload.data_len = new_msg_ptr->new_msg.payload.data_len;
    recv_pub->payload.data_ptr = new_msg_ptr->new_msg.payload.data_ptr;

#ifdef CONFIG_QCM_MQTT5_FUNC
    recv_pub->properties = new_msg_ptr->new_msg.options;
#endif /* CONFIG_QCM_MQTT5_FUNC */

    qosa_free(new_msg_ptr);
    qosa_mutex_unlock(g_mqtt_context.mutex);

    return QCM_MQTT_RES_OK;
}

/**
 * @brief Used to obtain the internal status information of the corresponding client.
 *
 * @param[in] qosa_uint8_t client_id
 *          - Corresponding client information to be queried
 *
 * @param[out] qcm_mqtt_client_info_t * client_info
 *          - Returns the corresponding client's internal status information
 *
 * @return qcm_mqtt_eercode_e
 *        - Returns MQTT_RES_OK to indicate success.
 *        - Returns other values from mqtt_eercode_e to indicate failure.
 */
qcm_mqtt_eercode_e qcm_mqtt_client_get_state_info(qosa_uint8_t client_id, qcm_mqtt_client_info_t *client_info)
{
    qcm_mqtt_client_t *client_ptr = QOSA_NULL;
    qcm_mqtt_check_and_create_task();

    client_ptr = (qcm_mqtt_client_t *)qcm_mqtt_client_get_ptr(client_id);
    if (client_ptr == QOSA_NULL)
    {
        return QCM_MQTT_RES_ERROR_CLIENT_EXIST;
    }

    qosa_mutex_lock(g_mqtt_context.mutex, QOSA_WAIT_FOREVER);

    client_info->client_id = client_id;
    client_info->svr_port = client_ptr->server_port;
    qosa_memcpy(client_info->srv_host, client_ptr->server_addr, CONFIG_QOSA_HOST_URL_MAX_LEN);
    client_info->client_state = client_ptr->client_state;
    qosa_mutex_unlock(g_mqtt_context.mutex);

    return QCM_MQTT_RES_OK;
}

qcm_mqtt_eercode_e qcm_mqtt_nv_write(char *nv_path, void *data, qosa_uint16_t data_len)
{
    char unify_nv_name[16] = "unify";

    if (qosa_strstr(nv_path, "unify_m") != QOSA_NULL)
    {
        qosa_memcpy(unify_nv_name, "unify_m", qosa_strlen("unify_m"));
    }
    else if (qosa_strstr(nv_path, "unify_d") != QOSA_NULL)
    {
        qosa_memcpy(unify_nv_name, "unify_d", qosa_strlen("unify_d"));
    }
    qosa_nv_cfg_item_data_t unifycfg_table[] = {0};

    unifycfg_table[0].key = unify_nv_name;
    unifycfg_table[0].value = data;
    unifycfg_table[0].value_size = data_len;

    QLOGV("%s   %s", nv_path, unify_nv_name);
    if (qosa_nv_item_json_write(nv_path, unifycfg_table, QOSA_ARRAY_SIZE(unifycfg_table)) == QOSA_NVM_CFG_OK)
    {
        return QCM_MQTT_RES_OK;
    }
    else
    {
        QLOGE("write fail");
        return QCM_MQTT_RES_ERROR_PARAM_INVALID;
    }
}

qcm_mqtt_eercode_e qcm_mqtt_nv_read(char *nv_path, void *data, qosa_uint16_t data_len)
{
    char unify_nv_name[16] = "unify";

    if (qosa_strstr(nv_path, "unify_m") != QOSA_NULL)
    {
        qosa_memcpy(unify_nv_name, "unify_m", qosa_strlen("unify_m"));
    }
    else if (qosa_strstr(nv_path, "unify_d") != QOSA_NULL)
    {
        qosa_memcpy(unify_nv_name, "unify_d", qosa_strlen("unify_d"));
    }
    qosa_nv_cfg_item_data_t unifycfg_table[] = {0};

    unifycfg_table[0].key = unify_nv_name;
    unifycfg_table[0].value = data;
    unifycfg_table[0].value_size = data_len;

    QLOGV("%s   %s", nv_path, unify_nv_name);
    if (qosa_nv_item_cfg_read(nv_path, unifycfg_table, QOSA_ARRAY_SIZE(unifycfg_table)) == QOSA_NVM_CFG_OK)
    {
        return QCM_MQTT_RES_OK;
    }
    else
    {
        QLOGE("write fail");
        return QCM_MQTT_RES_ERROR_PARAM_INVALID;
    }
}
