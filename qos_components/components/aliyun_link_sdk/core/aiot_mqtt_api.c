/**
 * @file aiot_mqtt_api.c
 * @brief MQTT模块实现, 其中包含了连接到物联网平台和收发数据的API接口
 * @date 2019-12-27
 *
 * @copyright Copyright (C) 2015-2018 Alibaba Group Holding Limited
 *
 */

#include "core_mqtt.h"


static int32_t last_failed_error_code = 0;
static int32_t _core_mqtt_sysdep_return(int32_t sysdep_code, int32_t core_code)
{
    if (sysdep_code >= (STATE_PORT_BASE - 0x00FF) && sysdep_code < (STATE_PORT_BASE)) {
        return sysdep_code;
    } else {
        return core_code;
    }
}

static int32_t _core_mqtt_5_feature_is_enabled(core_mqtt_handle_t *mqtt_handle)
{
    return (AIOT_MQTT_VERSION_5_0 == mqtt_handle->protocol_version);
}

static void _core_mqtt_event_notify_process_handler(core_mqtt_handle_t *mqtt_handle, aiot_mqtt_event_t *event,
        core_mqtt_event_t *core_event)
{
    core_mqtt_process_data_node_t *node = NULL;

    core_list_for_each_entry(node, &mqtt_handle->process_data_list,
                             linked_node, core_mqtt_process_data_node_t) {
        node->process_data.handler(node->process_data.context, event, core_event);
    }
}

static void _core_mqtt_event_notify(core_mqtt_handle_t *mqtt_handle, aiot_mqtt_event_type_t type)
{
    aiot_mqtt_event_t event;
    memset(&event, 0, sizeof(aiot_mqtt_event_t));
    event.type = type;

    if (mqtt_handle->event_handler) {
        mqtt_handle->event_handler((void *)mqtt_handle, &event, mqtt_handle->userdata);
    }

    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->process_handler_mutex);
    _core_mqtt_event_notify_process_handler(mqtt_handle, &event, NULL);
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->process_handler_mutex);
}

static void _core_mqtt_connect_event_notify(core_mqtt_handle_t *mqtt_handle)
{
    mqtt_handle->disconnected = 0;
    if (mqtt_handle->has_connected == 0) {
        mqtt_handle->has_connected = 1;
        _core_mqtt_event_notify(mqtt_handle, AIOT_MQTTEVT_CONNECT);
    } else {
        _core_mqtt_event_notify(mqtt_handle, AIOT_MQTTEVT_RECONNECT);
    }
}

static void _core_mqtt_disconnect_event_notify(core_mqtt_handle_t *mqtt_handle,
        aiot_mqtt_disconnect_event_type_t disconnect)
{
    if (mqtt_handle->has_connected == 1 && mqtt_handle->disconnected == 0) {
        aiot_mqtt_event_t event;

        mqtt_handle->disconnected = 1;

        memset(&event, 0, sizeof(aiot_mqtt_event_t));
        event.type = AIOT_MQTTEVT_DISCONNECT;
        event.data.disconnect = disconnect;

        if (mqtt_handle->event_handler) {
            mqtt_handle->event_handler((void *)mqtt_handle, &event, mqtt_handle->userdata);
        }

        mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->process_handler_mutex);
        _core_mqtt_event_notify_process_handler(mqtt_handle, &event, NULL);
        mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->process_handler_mutex);
    }
}

static void _core_mqtt_exec_inc(core_mqtt_handle_t *mqtt_handle)
{
    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->data_mutex);
    mqtt_handle->exec_count++;
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->data_mutex);
}

static void _core_mqtt_exec_dec(core_mqtt_handle_t *mqtt_handle)
{
    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->data_mutex);
    mqtt_handle->exec_count--;
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->data_mutex);
}

static void _core_mqtt_sign_clean(core_mqtt_handle_t *mqtt_handle)
{
    if (mqtt_handle->username) {
        mqtt_handle->sysdep->core_sysdep_free(mqtt_handle->username);
        mqtt_handle->username = NULL;
    }
    if (mqtt_handle->password) {
        mqtt_handle->sysdep->core_sysdep_free(mqtt_handle->password);
        mqtt_handle->password = NULL;
    }
    if (mqtt_handle->clientid) {
        mqtt_handle->sysdep->core_sysdep_free(mqtt_handle->clientid);
        mqtt_handle->clientid = NULL;
    }
}

static int32_t _core_mqtt_handlerlist_insert(core_mqtt_handle_t *mqtt_handle, core_mqtt_sub_node_t *sub_node,
        aiot_mqtt_recv_handler_t handler, void *userdata)
{
    core_mqtt_sub_handler_node_t *node = NULL;

    core_list_for_each_entry(node, &sub_node->handle_list, linked_node, core_mqtt_sub_handler_node_t) {
        if (node->handler == handler) {
            /* exist handler, replace userdata */
            node->userdata = userdata;
            return STATE_SUCCESS;
        }
    }

    if (&node->linked_node == &sub_node->handle_list) {
        /* new handler */
        node = mqtt_handle->sysdep->core_sysdep_malloc(sizeof(core_mqtt_sub_handler_node_t), CORE_MQTT_MODULE_NAME);
        if (node == NULL) {
            return STATE_SYS_DEPEND_MALLOC_FAILED;
        }
        memset(node, 0, sizeof(core_mqtt_sub_handler_node_t));
        CORE_INIT_LIST_HEAD(&node->linked_node);
        node->handler = handler;
        node->userdata = userdata;

        core_list_add_tail(&node->linked_node, &sub_node->handle_list);
    }

    return STATE_SUCCESS;
}

static int32_t _core_mqtt_sublist_insert(core_mqtt_handle_t *mqtt_handle, core_mqtt_buff_t *topic,
        aiot_mqtt_recv_handler_t handler, void *userdata)
{
    int32_t res = STATE_SUCCESS;
    core_mqtt_sub_node_t *node = NULL;

    core_list_for_each_entry(node, &mqtt_handle->sub_list, linked_node, core_mqtt_sub_node_t) {
        if ((strlen(node->topic) == topic->len) && memcmp(node->topic, topic->buffer, topic->len) == 0) {
            /* exist topic */
            if (handler != NULL) {
                return _core_mqtt_handlerlist_insert(mqtt_handle, node, handler, userdata);
            } else {
                return STATE_SUCCESS;
            }
        }
    }

    if (&node->linked_node == &mqtt_handle->sub_list) {
        /* new topic */
        node = mqtt_handle->sysdep->core_sysdep_malloc(sizeof(core_mqtt_sub_node_t), CORE_MQTT_MODULE_NAME);
        if (node == NULL) {
            return STATE_SYS_DEPEND_MALLOC_FAILED;
        }
        memset(node, 0, sizeof(core_mqtt_sub_node_t));
        CORE_INIT_LIST_HEAD(&node->linked_node);
        CORE_INIT_LIST_HEAD(&node->handle_list);

        node->topic = mqtt_handle->sysdep->core_sysdep_malloc(topic->len + 1, CORE_MQTT_MODULE_NAME);
        if (node->topic == NULL) {
            mqtt_handle->sysdep->core_sysdep_free(node);
            return STATE_SYS_DEPEND_MALLOC_FAILED;
        }
        memset(node->topic, 0, topic->len + 1);
        memcpy(node->topic, topic->buffer, topic->len);

        if (handler != NULL) {
            res = _core_mqtt_handlerlist_insert(mqtt_handle, node, handler, userdata);
            if (res < STATE_SUCCESS) {
                mqtt_handle->sysdep->core_sysdep_free(node->topic);
                mqtt_handle->sysdep->core_sysdep_free(node);
                return res;
            }
        }

        core_list_add_tail(&node->linked_node, &mqtt_handle->sub_list);
    }
    return res;
}
static int32_t _core_mqtt_topic_alias_list_insert(core_mqtt_handle_t *mqtt_handle, core_mqtt_buff_t *topic,
        uint16_t topic_alias, struct core_list_head *list)
{
    int32_t res = STATE_SUCCESS;
    core_mqtt_topic_alias_node_t *node = NULL;

    core_list_for_each_entry(node, list, linked_node, core_mqtt_topic_alias_node_t) {
        if ((strlen(node->topic) == topic->len) && memcmp(node->topic, topic->buffer, topic->len) == 0) {
            /* exist topic */
            return STATE_SUCCESS;
        }
    }

    if (&node->linked_node == list) {
        /* new topic */
        node = mqtt_handle->sysdep->core_sysdep_malloc(sizeof(core_mqtt_topic_alias_node_t), CORE_MQTT_MODULE_NAME);
        if (node == NULL) {
            return STATE_SYS_DEPEND_MALLOC_FAILED;
        }
        memset(node, 0, sizeof(core_mqtt_topic_alias_node_t));
        CORE_INIT_LIST_HEAD(&node->linked_node);

        node->topic = mqtt_handle->sysdep->core_sysdep_malloc(topic->len + 1, CORE_MQTT_MODULE_NAME);
        if (node->topic == NULL) {
            mqtt_handle->sysdep->core_sysdep_free(node);
            return STATE_SYS_DEPEND_MALLOC_FAILED;
        }
        memset(node->topic, 0, topic->len + 1);
        memcpy(node->topic, topic->buffer, topic->len);
        node->topic_alias = topic_alias;

        core_list_add_tail(&node->linked_node, list);
    }
    return res;
}

static void _core_mqtt_topic_alias_list_remove_all(core_mqtt_handle_t *mqtt_handle)
{
    core_mqtt_topic_alias_node_t *node = NULL, *next = NULL;
    core_list_for_each_entry_safe(node, next, &mqtt_handle->rx_topic_alias_list, linked_node,
                                  core_mqtt_topic_alias_node_t) {
        core_list_del(&node->linked_node);
        mqtt_handle->sysdep->core_sysdep_free(node->topic);
        mqtt_handle->sysdep->core_sysdep_free(node);
    }

    core_list_for_each_entry_safe(node, next, &mqtt_handle->tx_topic_alias_list, linked_node,
                                  core_mqtt_topic_alias_node_t) {
        core_list_del(&node->linked_node);
        mqtt_handle->sysdep->core_sysdep_free(node->topic);
        mqtt_handle->sysdep->core_sysdep_free(node);
    }
}

static void _core_mqtt_sublist_handlerlist_destroy(core_mqtt_handle_t *mqtt_handle, struct core_list_head *list)
{
    core_mqtt_sub_handler_node_t *node = NULL, *next = NULL;

    core_list_for_each_entry_safe(node, next, list, linked_node, core_mqtt_sub_handler_node_t) {
        core_list_del(&node->linked_node);
        mqtt_handle->sysdep->core_sysdep_free(node);
    }
}

static void _core_mqtt_sublist_remove(core_mqtt_handle_t *mqtt_handle, core_mqtt_buff_t *topic)
{
    core_mqtt_sub_node_t *node = NULL, *next = NULL;

    core_list_for_each_entry_safe(node, next, &mqtt_handle->sub_list, linked_node, core_mqtt_sub_node_t) {
        if ((strlen(node->topic) == topic->len) && memcmp(node->topic, topic->buffer, topic->len) == 0) {
            core_list_del(&node->linked_node);
            _core_mqtt_sublist_handlerlist_destroy(mqtt_handle, &node->handle_list);
            mqtt_handle->sysdep->core_sysdep_free(node->topic);
            mqtt_handle->sysdep->core_sysdep_free(node);
        }
    }
}

static void _core_mqtt_sublist_remove_handler(core_mqtt_handle_t *mqtt_handle, core_mqtt_buff_t *topic,
        aiot_mqtt_recv_handler_t handler)
{
    core_mqtt_sub_node_t *node = NULL;
    core_mqtt_sub_handler_node_t *handler_node = NULL, *handler_next = NULL;

    core_list_for_each_entry(node, &mqtt_handle->sub_list, linked_node, core_mqtt_sub_node_t) {
        if ((strlen(node->topic) == topic->len) && memcmp(node->topic, topic->buffer, topic->len) == 0) {
            core_list_for_each_entry_safe(handler_node, handler_next, &node->handle_list,
                                          linked_node, core_mqtt_sub_handler_node_t) {
                if (handler_node->handler == handler) {
                    core_list_del(&handler_node->linked_node);
                    mqtt_handle->sysdep->core_sysdep_free(handler_node);
                }
            }
        }
    }
}

static void _core_mqtt_sublist_destroy(core_mqtt_handle_t *mqtt_handle)
{
    core_mqtt_sub_node_t *node = NULL, *next = NULL;

    core_list_for_each_entry_safe(node, next, &mqtt_handle->sub_list, linked_node, core_mqtt_sub_node_t) {
        core_list_del(&node->linked_node);
        _core_mqtt_sublist_handlerlist_destroy(mqtt_handle, &node->handle_list);
        mqtt_handle->sysdep->core_sysdep_free(node->topic);
        mqtt_handle->sysdep->core_sysdep_free(node);
    }
}

static int32_t _core_mqtt_topic_is_valid(core_mqtt_handle_t *mqtt_handle, char *topic, uint32_t len)
{
    uint32_t idx = 0;

    if (mqtt_handle->topic_header_check && topic[0] != '/') {
        return STATE_MQTT_TOPIC_INVALID;
    }

    for (idx = 1; idx < len; idx++) {
        if (topic[idx] == '+') {
            /* topic should contain '/+/' in the middle or '/+' in the end */
            if ((topic[idx - 1] != '/') ||
                ((idx + 1 < len) && (topic[idx + 1] != '/'))) {
                return STATE_MQTT_TOPIC_INVALID;
            }
        }
        if (topic[idx] == '#') {
            /* topic should contain '/#' in the end */
            if ((topic[idx - 1] != '/') ||
                (idx + 1 < len)) {
                return STATE_MQTT_TOPIC_INVALID;
            }
        }
    }

    return STATE_SUCCESS;
}

static int32_t _core_mqtt_append_topic_map(core_mqtt_handle_t *mqtt_handle, aiot_mqtt_topic_map_t *map)
{
    int32_t             res = STATE_SUCCESS;
    core_mqtt_buff_t    topic_buff;

    if (map->topic == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }
    if (strlen(map->topic) >= CORE_MQTT_TOPIC_MAXLEN) {
        return STATE_MQTT_TOPIC_TOO_LONG;
    }
    if ((res = _core_mqtt_topic_is_valid(mqtt_handle, (char *)map->topic, strlen((char *)map->topic))) < STATE_SUCCESS) {
        return res;
    }

    memset(&topic_buff, 0, sizeof(topic_buff));
    topic_buff.buffer = (uint8_t *)map->topic;
    topic_buff.len = strlen(map->topic);

    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->sub_mutex);
    res = _core_mqtt_sublist_insert(mqtt_handle, &topic_buff, map->handler, map->userdata);
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->sub_mutex);

    return res;
}

static int32_t _core_mqtt_remove_topic_map(core_mqtt_handle_t *mqtt_handle, aiot_mqtt_topic_map_t *map)
{
    int32_t             res = STATE_SUCCESS;
    core_mqtt_buff_t    topic_buff;

    if (map->topic == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }
    if (strlen(map->topic) >= CORE_MQTT_TOPIC_MAXLEN) {
        return STATE_MQTT_TOPIC_TOO_LONG;
    }
    if ((res = _core_mqtt_topic_is_valid(mqtt_handle, (char *)map->topic, strlen((char *)map->topic))) < STATE_SUCCESS) {
        return res;
    }

    memset(&topic_buff, 0, sizeof(topic_buff));
    topic_buff.buffer = (uint8_t *)map->topic;
    topic_buff.len = strlen(map->topic);

    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->sub_mutex);
    _core_mqtt_sublist_remove_handler(mqtt_handle, &topic_buff, map->handler);
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->sub_mutex);

    return STATE_SUCCESS;
}

static void _core_mqtt_set_utf8_encoded_str(uint8_t *input, uint16_t input_len, uint8_t *output)
{
    uint32_t idx = 0, input_idx = 0;

    /* String Length MSB */
    output[idx++] = (uint8_t)((input_len >> 8) & 0x00FF);

    /* String Length LSB */
    output[idx++] = (uint8_t)((input_len) & 0x00FF);

    /* UTF-8 Encoded Character Data */
    for (input_idx = 0; input_idx < input_len; input_idx++) {
        output[idx++] = input[input_idx];
    }
}

int32_t _write_variable(uint32_t input, uint8_t *output)
{
    uint8_t encoded_byte = 0, offset = 0;

    do {
        encoded_byte = input % 128;
        input /= 128;
        if (input > 0) {
            encoded_byte |= 128;
        }
        output[offset++] = encoded_byte;
    } while (input > 0);

    return offset;
}

int32_t _read_variable(uint8_t *input, uint32_t *output)
{
    uint8_t ch = 0;
    uint32_t multiplier = 1;
    uint32_t mqtt_remainlen = 0;
    uint8_t offset = 0;

    do {
        ch = input[offset++];
        mqtt_remainlen += (ch & 127) * multiplier;
        if (multiplier > 128 * 128 * 128) {
            return STATE_MQTT_MALFORMED_REMAINING_LEN;
        }
        multiplier *= 128;
    } while ((ch & 128) != 0 && offset < 4);

    *output = mqtt_remainlen;
    return offset;
}

static int32_t _core_mqtt_conn_pkt(core_mqtt_handle_t *mqtt_handle, uint8_t **pkt, uint32_t *pkt_len,
                                   will_message_t *will_message, mqtt_properties_t *conn_prop)
{
    uint32_t idx = 0, conn_paylaod_len = 0, conn_remainlen = 0, conn_pkt_len = 0, props_len = 0, will_props_len = 0, will_len = 0;
    uint8_t *pos = NULL;
    const uint8_t conn_fixed_header = CORE_MQTT_CONN_PKT_TYPE;
    const uint8_t conn_protocol_name[] = {0x00, 0x04, 0x4D, 0x51, 0x54, 0x54};
    uint8_t conn_connect_flag = 0;
    int32_t res = 0;

    /* Property Len */
    if (_core_mqtt_5_feature_is_enabled(mqtt_handle)) {
        res = core_mqtt_props_bound(conn_prop);
        if(res < STATE_SUCCESS) {
            return res;
        }
        props_len = res;
        if(will_message != NULL) {
            res = core_mqtt_props_bound(will_message->props);
            if(res < STATE_SUCCESS) {
                return res;
            }
            will_props_len = res;
            will_len += will_props_len + strlen(will_message->topic) + will_message->payload_len + 2 * CORE_MQTT_UTF8_STR_EXTRA_LEN;
        }
    }

    /* Payload Length */
    conn_paylaod_len = (uint32_t)(strlen(mqtt_handle->clientid) + strlen(mqtt_handle->username)
                                  + strlen(mqtt_handle->password) + 3 * CORE_MQTT_UTF8_STR_EXTRA_LEN);

    /* Remain-Length Value */
    conn_remainlen = CORE_MQTT_CONN_REMAINLEN_FIXED_LEN + conn_paylaod_len + props_len + will_len;

    /* Total Packet Length */
    conn_pkt_len = CORE_MQTT_CONN_FIXED_HEADER_TOTAL_LEN + conn_paylaod_len + props_len + will_len;

    pos = mqtt_handle->sysdep->core_sysdep_malloc(conn_pkt_len, CORE_MQTT_MODULE_NAME);
    if (pos == NULL) {
        return STATE_SYS_DEPEND_MALLOC_FAILED;
    }
    memset(pos, 0, conn_pkt_len);

    /* Fixed Header */
    pos[idx++] = conn_fixed_header;

    /* Remain Length */
    idx += _write_variable(conn_remainlen, pos + idx);

    /* Protocol Name */
    memcpy(pos + idx, conn_protocol_name, CORE_MQTT_CONN_PROTOCOL_NAME_LEN);
    idx += CORE_MQTT_CONN_PROTOCOL_NAME_LEN;

    /* Protocol Version */
    if (_core_mqtt_5_feature_is_enabled(mqtt_handle)) {
        pos[idx++] = 0x05;
    } else {
        pos[idx++] = 0x04;
    }

    /* Connect Flag */
    conn_connect_flag = 0xC0 | (mqtt_handle->clean_session << 1);
    if(will_message != NULL) {
        conn_connect_flag = conn_connect_flag | 1 << 2 | will_message->qos << 3 | will_message->retain << 5;
    }
    pos[idx++] = conn_connect_flag;

    /* Keep Alive MSB */
    pos[idx++] = (uint8_t)((mqtt_handle->keep_alive_s >> 8) & 0x00FF);

    /* Keep Alive LSB */
    pos[idx++] = (uint8_t)((mqtt_handle->keep_alive_s) & 0x00FF);

    /* property */
    if (_core_mqtt_5_feature_is_enabled(mqtt_handle) && props_len > 0) {
        res = core_mqtt_props_write(conn_prop, &pos[idx], props_len);
        idx += res;
    }

    /* Payload: clientid, will property, will topic, will payload, username, password */
    _core_mqtt_set_utf8_encoded_str((uint8_t *)mqtt_handle->clientid, strlen(mqtt_handle->clientid), pos + idx);
    idx += CORE_MQTT_UTF8_STR_EXTRA_LEN + strlen(mqtt_handle->clientid);

    if(will_message != NULL) {
        res = core_mqtt_props_write(will_message->props, &pos[idx], will_props_len);
        idx += res;

        _core_mqtt_set_utf8_encoded_str((uint8_t *)will_message->topic, strlen(will_message->topic), pos + idx);
        idx += CORE_MQTT_UTF8_STR_EXTRA_LEN + strlen(will_message->topic);
        
        _core_mqtt_set_utf8_encoded_str((uint8_t *)will_message->payload, will_message->payload_len, pos + idx);
        idx += CORE_MQTT_UTF8_STR_EXTRA_LEN + will_message->payload_len;
    }

    _core_mqtt_set_utf8_encoded_str((uint8_t *)mqtt_handle->username, strlen(mqtt_handle->username), pos + idx);
    idx += CORE_MQTT_UTF8_STR_EXTRA_LEN + strlen(mqtt_handle->username);

    _core_mqtt_set_utf8_encoded_str((uint8_t *)mqtt_handle->password, strlen(mqtt_handle->password), pos + idx);
    idx += CORE_MQTT_UTF8_STR_EXTRA_LEN + strlen(mqtt_handle->password);

    *pkt = pos;
    *pkt_len = idx;

    return STATE_SUCCESS;
}

static int32_t _core_mqtt_update_connack_props(core_mqtt_handle_t *mqtt_handle, mqtt_properties_t *props)
{
    mqtt_property_t *prop = NULL;

    prop = aiot_mqtt_props_get(props, MQTT_PROP_ID_TOPIC_ALIAS_MAXIMUM, 0);
    if(prop != NULL) {
        mqtt_handle->tx_topic_alias_max = prop->value.uint16;
    }

    prop = aiot_mqtt_props_get(props, MQTT_PROP_ID_MAXIMUM_PACKET_SIZE, 0);
    if(prop != NULL) {
        mqtt_handle->tx_packet_max_size = prop->value.uint32;
    }

    prop = aiot_mqtt_props_get(props, MQTT_PROP_ID_RECEIVE_MAXIMUM, 0);
    if(prop != NULL) {
        mqtt_handle->server_receive_max = prop->value.uint16;
    }
    
    return STATE_SUCCESS;
}

static int32_t _core_mqtt_connack_handle(core_mqtt_handle_t *mqtt_handle, uint8_t *connack, uint32_t remain_len)
{
    int32_t res = STATE_SUCCESS;
    mqtt_properties_t *props = NULL;
    aiot_mqtt_recv_t packet;

    memset(&packet, 0, sizeof(aiot_mqtt_recv_t));
    if (_core_mqtt_5_feature_is_enabled(mqtt_handle)) {
        if (connack[0] != 0x00) {
            return STATE_MQTT_CONNACK_FMT_ERROR;
        }

        /* First Byte is Conack Flag */
        if (connack[1] == CORE_MQTT_CONNACK_RCODE_ACCEPTED) {
            res = STATE_SUCCESS;
        } else if (connack[1] == CORE_MQTT_V5_CONNACK_RCODE_UNACCEPTABLE_PROTOCOL_VERSION) {
            core_log(mqtt_handle->sysdep, STATE_MQTT_LOG_DISCONNECT, "MQTT invalid protocol version, disconeect\r\n");
            res = STATE_MQTT_CONNACK_RCODE_UNACCEPTABLE_PROTOCOL_VERSION;
        } else if (connack[1] == CORE_MQTT_V5_CONNACK_RCODE_SERVER_UNAVAILABLE) {
            core_log(mqtt_handle->sysdep, STATE_MQTT_LOG_DISCONNECT, "MQTT server unavailable, disconnect\r\n");
            res = STATE_MQTT_CONNACK_RCODE_SERVER_UNAVAILABLE;
        } else if (connack[1] == CORE_MQTT_V5_CONNACK_RCODE_BAD_USERNAME_PASSWORD) {
            core_log(mqtt_handle->sysdep, STATE_MQTT_LOG_DISCONNECT, "MQTT bad username or password, disconnect\r\n");
            res = STATE_MQTT_CONNACK_RCODE_BAD_USERNAME_PASSWORD;
        } else if (connack[1] == CORE_MQTT_V5_CONNACK_RCODE_NOT_AUTHORIZED) {
            core_log(mqtt_handle->sysdep, STATE_MQTT_LOG_DISCONNECT, "MQTT authorize fail, disconnect\r\n");
            res = STATE_MQTT_CONNACK_RCODE_NOT_AUTHORIZED;
        } else {
            res = STATE_MQTT_CONNACK_RCODE_UNKNOWN;
        }

        packet.type = AIOT_MQTTRECV_CON_ACK;
        /* Second Byte is Reason Code */
        packet.data.con_ack.reason_code = connack[1];

        if(connack[2] != 0) {
            props = aiot_mqtt_props_init();
            core_mqtt_props_read(connack + 2, remain_len, props);
            _core_mqtt_update_connack_props(mqtt_handle, props);
            packet.data.con_ack.props = props;
        }

        if (mqtt_handle->recv_handler) {
            mqtt_handle->recv_handler((void *)mqtt_handle, &packet, mqtt_handle->userdata);
        }
        if(props != NULL) {
            aiot_mqtt_props_deinit(&props);
        }
    } else {
        if (connack[1] == CORE_MQTT_CONNACK_RCODE_ACCEPTED) {
            res = STATE_SUCCESS;
        } else if (connack[1] == CORE_MQTT_CONNACK_RCODE_UNACCEPTABLE_PROTOCOL_VERSION) {
            core_log(mqtt_handle->sysdep, STATE_MQTT_LOG_DISCONNECT, "MQTT invalid protocol version, disconeect\r\n");
            res = STATE_MQTT_CONNACK_RCODE_UNACCEPTABLE_PROTOCOL_VERSION;
        } else if (connack[1] == CORE_MQTT_CONNACK_RCODE_SERVER_UNAVAILABLE) {
            core_log(mqtt_handle->sysdep, STATE_MQTT_LOG_DISCONNECT, "MQTT server unavailable, disconnect\r\n");
            res = STATE_MQTT_CONNACK_RCODE_SERVER_UNAVAILABLE;
        } else if (connack[1] == CORE_MQTT_CONNACK_RCODE_BAD_USERNAME_PASSWORD) {
            core_log(mqtt_handle->sysdep, STATE_MQTT_LOG_DISCONNECT, "MQTT bad username or password, disconnect\r\n");
            res = STATE_MQTT_CONNACK_RCODE_BAD_USERNAME_PASSWORD;
        } else if (connack[1] == CORE_MQTT_CONNACK_RCODE_NOT_AUTHORIZED) {
            core_log(mqtt_handle->sysdep, STATE_MQTT_LOG_DISCONNECT, "MQTT authorize fail, disconnect\r\n");
            res = STATE_MQTT_CONNACK_RCODE_NOT_AUTHORIZED;
        } else {
            res = STATE_MQTT_CONNACK_RCODE_UNKNOWN;
        }
    }
    return res;
}

static int32_t _core_mqtt_read(core_mqtt_handle_t *mqtt_handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms)
{
    int32_t res = STATE_SUCCESS;

    if (mqtt_handle->network_handle != NULL) {
        res = mqtt_handle->sysdep->core_sysdep_network_recv(mqtt_handle->network_handle, buffer, len, timeout_ms, NULL);
        if (res < STATE_SUCCESS) {
            res = _core_mqtt_sysdep_return(res, STATE_SYS_DEPEND_NWK_RECV_ERR);
        } else if ((uint32_t)res != len) {
            res = STATE_SYS_DEPEND_NWK_READ_LESSDATA;
        }
    } else {
        res = STATE_SYS_DEPEND_NWK_CLOSED;
    }

    return res;
}

static int32_t _core_mqtt_write(core_mqtt_handle_t *mqtt_handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms)
{
    int32_t res = STATE_SUCCESS;

    if (mqtt_handle->network_handle != NULL) {
        res = mqtt_handle->sysdep->core_sysdep_network_send(mqtt_handle->network_handle, buffer, len, timeout_ms, NULL);
        if (res < STATE_SUCCESS) {
            res = _core_mqtt_sysdep_return(res, STATE_SYS_DEPEND_NWK_SEND_ERR);
        } else if ((uint32_t)res != len) {
            res = STATE_SYS_DEPEND_NWK_WRITE_LESSDATA;
        }
    } else {
        res = STATE_SYS_DEPEND_NWK_CLOSED;
    }

    return res;
}


static void _core_mqtt_connect_diag(core_mqtt_handle_t *mqtt_handle, uint8_t flag)
{
    uint8_t buf[4] = {0};
    uint32_t idx = 0;

    buf[idx++] = (CORE_MQTT_DIAG_TLV_MQTT_CONNECTION >> 8) & 0x00FF;
    buf[idx++] = (CORE_MQTT_DIAG_TLV_MQTT_CONNECTION) & 0x00FF;
    buf[idx++] = 0x01;
    buf[idx++] = flag;

    core_diag(mqtt_handle->sysdep, STATE_MQTT_BASE, buf, sizeof(buf));
}

static void _core_mqtt_heartbeat_diag(core_mqtt_handle_t *mqtt_handle, uint8_t flag)
{
    uint8_t buf[4] = {0};
    uint32_t idx = 0;

    buf[idx++] = (CORE_MQTT_DIAG_TLV_MQTT_HEARTBEAT >> 8) & 0x00FF;
    buf[idx++] = (CORE_MQTT_DIAG_TLV_MQTT_HEARTBEAT) & 0x00FF;
    buf[idx++] = 0x01;
    buf[idx++] = flag;

    core_diag(mqtt_handle->sysdep, STATE_MQTT_BASE, buf, sizeof(buf));
}

static int32_t _core_mqtt_read_remainlen(core_mqtt_handle_t *mqtt_handle, uint32_t *remainlen);

static int32_t _core_mqtt_add_extend_clientid(core_mqtt_handle_t *channel_handle, char **dst_clientid, char *extend)
{
    int32_t res = STATE_SUCCESS;
    if (extend == NULL || dst_clientid == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }
    if (*dst_clientid == NULL) {
        return core_strdup(channel_handle->sysdep, dst_clientid, extend, CORE_MQTT_MODULE_NAME);
    } else {
        char *extend_clientid = *dst_clientid;
        char *src[] = {extend_clientid, extend};
        res = core_sprintf(channel_handle->sysdep, dst_clientid, "%s,%s", src, 2, CORE_MQTT_MODULE_NAME);
        channel_handle->sysdep->core_sysdep_free(extend_clientid);
        return res;
    }
}

static int32_t _core_mqtt_add_netstats_extend(core_mqtt_handle_t *mqtt_handle, char **dst_clientid)
{
    char *content_fmt = "_conn=%s_%s";
    char *content_fmt2 = "_conn=%s_%s_-%s";
    char time_str[22] = {0};
    char err_str[22] = {0};
    char *content = NULL;
    char *content_src[] = {NULL, time_str + 2, err_str + 3};
    uint8_t len = 0;
    int32_t res = STATE_SUCCESS;

    if (mqtt_handle->cred != NULL && mqtt_handle->cred->option != AIOT_SYSDEP_NETWORK_CRED_NONE) {
        content_src[0] = "tls";
    } else {
        content_src[0] = "tcp";
    }
    memset(time_str, 0, sizeof(time_str));
    core_int2hexstr(mqtt_handle->nwkstats_info.connect_time_used, time_str, &len);

    if (last_failed_error_code != 0) {
        memset(err_str, 0, sizeof(err_str));
        core_int2hexstr(last_failed_error_code, err_str, &len);
        res = core_sprintf(mqtt_handle->sysdep, &content, content_fmt2, content_src, 3, CORE_MQTT_MODULE_NAME);
        last_failed_error_code = 0;
    } else {
        res = core_sprintf(mqtt_handle->sysdep, &content, content_fmt, content_src, 2, CORE_MQTT_MODULE_NAME);
    }

    _core_mqtt_add_extend_clientid(mqtt_handle, dst_clientid, content);
    mqtt_handle->sysdep->core_sysdep_free(content);
    return res;
}

static int32_t _core_mqtt_connect(core_mqtt_handle_t *mqtt_handle, will_message_t *will_message, mqtt_properties_t *conn_prop)
{
    int32_t res = 0;
    core_sysdep_socket_type_t socket_type = CORE_SYSDEP_SOCKET_TCP_CLIENT;
    char backup_ip[16] = {0};
    uint8_t *conn_pkt = NULL;
    uint8_t connack_fixed_header = 0;
    uint8_t *connack_ptr = NULL;
    uint32_t conn_pkt_len = 0;
    char *secure_mode = (mqtt_handle->cred == NULL) ? ("3") : ("2");
    uint8_t use_assigned_clientid = mqtt_handle->use_assigned_clientid;
    uint32_t remain_len = 0;

    if (mqtt_handle->host == NULL) {
        return STATE_USER_INPUT_MISSING_HOST;
    }

    if (mqtt_handle->security_mode != NULL) {
        secure_mode = mqtt_handle->security_mode;
    }

    if (mqtt_handle->cred && \
        mqtt_handle->cred->option == AIOT_SYSDEP_NETWORK_CRED_NONE && \
        mqtt_handle->security_mode == NULL) {
        secure_mode = "3";
    }

    if (mqtt_handle->username == NULL || mqtt_handle->password == NULL ||
        mqtt_handle->clientid == NULL) {
        /* no valid username, password or clientid, check pk/dn/ds */
        if (mqtt_handle->product_key == NULL) {
            return STATE_USER_INPUT_MISSING_PRODUCT_KEY;
        }
        if (mqtt_handle->device_name == NULL) {
            return STATE_USER_INPUT_MISSING_DEVICE_NAME;
        }
        if (mqtt_handle->device_secret == NULL) {
            return STATE_USER_INPUT_MISSING_DEVICE_SECRET;
        }
        _core_mqtt_sign_clean(mqtt_handle);

        if ((res = core_auth_mqtt_username(mqtt_handle->sysdep, &mqtt_handle->username, mqtt_handle->product_key,
                                           mqtt_handle->device_name, CORE_MQTT_MODULE_NAME)) < STATE_SUCCESS ||
            (res = core_auth_mqtt_password(mqtt_handle->sysdep, &mqtt_handle->password, mqtt_handle->product_key,
                                           mqtt_handle->device_name, mqtt_handle->device_secret, use_assigned_clientid, CORE_MQTT_MODULE_NAME)) < STATE_SUCCESS) {
            _core_mqtt_sign_clean(mqtt_handle);
            return res;
        }
        core_log1(mqtt_handle->sysdep, STATE_MQTT_LOG_HOST, "mqtt host: %s\r\n", (void *)mqtt_handle->host);
        core_log1(mqtt_handle->sysdep, STATE_MQTT_LOG_USERNAME, "user name: %s\r\n", (void *)mqtt_handle->username);
    }

    if (mqtt_handle->network_handle != NULL) {
        mqtt_handle->sysdep->core_sysdep_network_deinit(&mqtt_handle->network_handle);
    }

    _core_mqtt_connect_diag(mqtt_handle, 0x00);

    mqtt_handle->network_handle = mqtt_handle->sysdep->core_sysdep_network_init();
    if (mqtt_handle->network_handle == NULL) {
        return STATE_SYS_DEPEND_MALLOC_FAILED;
    }

    core_global_get_mqtt_backup_ip(mqtt_handle->sysdep, backup_ip);
    if (strlen(backup_ip) > 0) {
        core_log1(mqtt_handle->sysdep, STATE_MQTT_LOG_BACKUP_IP, "%s\r\n", (void *)backup_ip);
    }
    if ((res = mqtt_handle->sysdep->core_sysdep_network_setopt(mqtt_handle->network_handle, CORE_SYSDEP_NETWORK_SOCKET_TYPE,
               &socket_type)) < STATE_SUCCESS ||
        (res = mqtt_handle->sysdep->core_sysdep_network_setopt(mqtt_handle->network_handle, CORE_SYSDEP_NETWORK_HOST,
                mqtt_handle->host)) < STATE_SUCCESS ||
        (res = mqtt_handle->sysdep->core_sysdep_network_setopt(mqtt_handle->network_handle, CORE_SYSDEP_NETWORK_BACKUP_IP,
                backup_ip)) < STATE_SUCCESS ||
        (res = mqtt_handle->sysdep->core_sysdep_network_setopt(mqtt_handle->network_handle, CORE_SYSDEP_NETWORK_PORT,
                &mqtt_handle->port)) < STATE_SUCCESS ||
        (res = mqtt_handle->sysdep->core_sysdep_network_setopt(mqtt_handle->network_handle,
                CORE_SYSDEP_NETWORK_CONNECT_TIMEOUT_MS,
                &mqtt_handle->connect_timeout_ms)) < STATE_SUCCESS) {
        mqtt_handle->sysdep->core_sysdep_network_deinit(&mqtt_handle->network_handle);
        return _core_mqtt_sysdep_return(res, STATE_SYS_DEPEND_NWK_INVALID_OPTION);
    }

    if (mqtt_handle->cred != NULL) {
        if ((res = mqtt_handle->sysdep->core_sysdep_network_setopt(mqtt_handle->network_handle, CORE_SYSDEP_NETWORK_CRED,
                   mqtt_handle->cred)) < STATE_SUCCESS) {
            mqtt_handle->sysdep->core_sysdep_network_deinit(&mqtt_handle->network_handle);
            return _core_mqtt_sysdep_return(res, STATE_SYS_DEPEND_NWK_INVALID_OPTION);
        }
        if (mqtt_handle->cred->option == AIOT_SYSDEP_NETWORK_CRED_SVRCERT_PSK) {
            char *psk_id = NULL, psk[65] = {0};
            core_sysdep_psk_t sysdep_psk;

            res = core_auth_tls_psk(mqtt_handle->sysdep, &psk_id, psk, mqtt_handle->product_key, mqtt_handle->device_name,
                                    mqtt_handle->device_secret, CORE_MQTT_MODULE_NAME);
            if (res < STATE_SUCCESS) {
                return res;
            }

            memset(&sysdep_psk, 0, sizeof(core_sysdep_psk_t));
            sysdep_psk.psk_id = psk_id;
            sysdep_psk.psk = psk;
            core_log1(mqtt_handle->sysdep, STATE_MQTT_LOG_TLS_PSK, "%s\r\n", sysdep_psk.psk_id);
            core_log1(mqtt_handle->sysdep, STATE_MQTT_LOG_TLS_PSK, "%s\r\n", sysdep_psk.psk);
            res = mqtt_handle->sysdep->core_sysdep_network_setopt(mqtt_handle->network_handle, CORE_SYSDEP_NETWORK_PSK,
                    (void *)&sysdep_psk);
            mqtt_handle->sysdep->core_sysdep_free(psk_id);
            if (res < STATE_SUCCESS) {
                return _core_mqtt_sysdep_return(res, STATE_SYS_DEPEND_NWK_INVALID_OPTION);
            }
        }
        mqtt_handle->nwkstats_info.network_type = (uint8_t)mqtt_handle->cred->option;
    }

    /* Remove All Topic Alias Relationship */
    _core_mqtt_topic_alias_list_remove_all(mqtt_handle);

    /* network stats */
    mqtt_handle->nwkstats_info.connect_timestamp = mqtt_handle->sysdep->core_sysdep_time();

    if ((res = mqtt_handle->sysdep->core_sysdep_network_establish(mqtt_handle->network_handle)) < STATE_SUCCESS) {
        mqtt_handle->sysdep->core_sysdep_network_deinit(&mqtt_handle->network_handle);
        mqtt_handle->nwkstats_info.failed_timestamp = mqtt_handle->nwkstats_info.connect_timestamp;
        mqtt_handle->nwkstats_info.failed_error_code = res;
        last_failed_error_code = res;
        return _core_mqtt_sysdep_return(res, STATE_SYS_DEPEND_NWK_EST_FAILED);
    }

    mqtt_handle->nwkstats_info.connect_time_used = mqtt_handle->sysdep->core_sysdep_time() \
            - mqtt_handle->nwkstats_info.connect_timestamp;

    if (mqtt_handle->clientid == NULL) {
        char *extend_clientid = NULL;
        _core_mqtt_add_extend_clientid(mqtt_handle, &extend_clientid, mqtt_handle->extend_clientid);
        _core_mqtt_add_netstats_extend(mqtt_handle, &extend_clientid);
        if (core_auth_mqtt_clientid(mqtt_handle->sysdep, &mqtt_handle->clientid, mqtt_handle->product_key,
                                    mqtt_handle->device_name, secure_mode, extend_clientid, use_assigned_clientid,
                                    CORE_MQTT_MODULE_NAME) < STATE_SUCCESS) {
            _core_mqtt_sign_clean(mqtt_handle);
            return res;
        }
        if (extend_clientid != NULL) {
            mqtt_handle->sysdep->core_sysdep_free(extend_clientid);
        }
        /* core_log1(mqtt_handle->sysdep, STATE_MQTT_LOG_CLIENTID, "%s\r\n", (void *)mqtt_handle->clientid); */
    }
    /* Get MQTT Connect Packet */
    res = _core_mqtt_conn_pkt(mqtt_handle, &conn_pkt, &conn_pkt_len, will_message, conn_prop);
    if (res < STATE_SUCCESS) {
        return res;
    }
    /* Send MQTT Connect Packet */
    res = _core_mqtt_write(mqtt_handle, conn_pkt, conn_pkt_len, mqtt_handle->send_timeout_ms);
    mqtt_handle->sysdep->core_sysdep_free(conn_pkt);
    if (res < STATE_SUCCESS) {
        if (res == STATE_SYS_DEPEND_NWK_WRITE_LESSDATA) {
            core_log1(mqtt_handle->sysdep, STATE_MQTT_LOG_CONNECT_TIMEOUT, "MQTT connect packet send timeout: %d\r\n",
                      &mqtt_handle->send_timeout_ms);
        } else {
            if (mqtt_handle->network_handle != NULL) {
                mqtt_handle->sysdep->core_sysdep_network_deinit(&mqtt_handle->network_handle);
            }
        }
        return res;
    }

    /* Receive MQTT Connect ACK Fixed Header Byte */
    res = _core_mqtt_read(mqtt_handle, &connack_fixed_header, CORE_MQTT_FIXED_HEADER_LEN, mqtt_handle->recv_timeout_ms);
    if (res < STATE_SUCCESS) {
        if (res == STATE_SYS_DEPEND_NWK_READ_LESSDATA) {
            core_log1(mqtt_handle->sysdep, STATE_MQTT_LOG_CONNECT_TIMEOUT, "MQTT connack packet recv fixed header timeout: %d\r\n",
                      &mqtt_handle->recv_timeout_ms);
        } else {
            if (mqtt_handle->network_handle != NULL) {
                mqtt_handle->sysdep->core_sysdep_network_deinit(&mqtt_handle->network_handle);
            }
        }
        return res;
    }

    if (connack_fixed_header != CORE_MQTT_CONNACK_PKT_TYPE) {
        return STATE_MQTT_CONNACK_FMT_ERROR;
    }

    /* Receive MQTT Connect ACK remain len */
    res = _core_mqtt_read_remainlen(mqtt_handle, &remain_len);
    if (res < STATE_SUCCESS) {
        if (res == STATE_SYS_DEPEND_NWK_READ_LESSDATA) {
            core_log1(mqtt_handle->sysdep, STATE_MQTT_LOG_CONNECT_TIMEOUT, "MQTT connack packet recv remain len timeout: %d\r\n",
                      &mqtt_handle->recv_timeout_ms);
        }
        return res;
    }

    /* Connack Format Error for Mqtt 3.1 */
    if (0 == _core_mqtt_5_feature_is_enabled(mqtt_handle) && remain_len != 0x2) {
        return STATE_MQTT_CONNACK_FMT_ERROR;
    }

    connack_ptr = mqtt_handle->sysdep->core_sysdep_malloc(remain_len, CORE_MQTT_MODULE_NAME);
    if (NULL == connack_ptr) {
        return STATE_SYS_DEPEND_MALLOC_FAILED;
    }
    memset(connack_ptr, 0, remain_len);

    /* Receive MQTT Connect ACK Variable Header and Properties */
    res = _core_mqtt_read(mqtt_handle, connack_ptr, remain_len, mqtt_handle->recv_timeout_ms);

    if (res < STATE_SUCCESS) {
        if (res == STATE_SYS_DEPEND_NWK_READ_LESSDATA) {
            core_log1(mqtt_handle->sysdep, STATE_MQTT_LOG_CONNECT_TIMEOUT,
                      "MQTT connack packet variable header and property recv timeout: %d\r\n",
                      &mqtt_handle->recv_timeout_ms);
        }
        mqtt_handle->sysdep->core_sysdep_free(connack_ptr);
        return res;
    }

    res = _core_mqtt_connack_handle(mqtt_handle, connack_ptr, remain_len);
    mqtt_handle->sysdep->core_sysdep_free(connack_ptr);
    if (res < STATE_SUCCESS) {
        mqtt_handle->sysdep->core_sysdep_network_deinit(&mqtt_handle->network_handle);
        return res;
    }

    _core_mqtt_connect_diag(mqtt_handle, 0x01);

    return STATE_MQTT_CONNECT_SUCCESS;
}

static int32_t _core_mqtt_disconnect(core_mqtt_handle_t *mqtt_handle)
{
    int32_t res = 0;
    uint8_t pingreq_pkt[2] = {CORE_MQTT_DISCONNECT_PKT_TYPE, 0x00};

    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->send_mutex);
    res = _core_mqtt_write(mqtt_handle, pingreq_pkt, 2, mqtt_handle->send_timeout_ms);
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->send_mutex);
    if (res < STATE_SUCCESS) {
        if (res != STATE_SYS_DEPEND_NWK_WRITE_LESSDATA) {
            mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->send_mutex);
            mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->recv_mutex);
            if (mqtt_handle->network_handle != NULL) {
                mqtt_handle->sysdep->core_sysdep_network_deinit(&mqtt_handle->network_handle);
            }
            mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->recv_mutex);
            mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->send_mutex);
        }
        return res;
    }

    return STATE_SUCCESS;
}

static uint16_t _core_mqtt_packet_id(core_mqtt_handle_t *mqtt_handle)
{
    uint16_t packet_id = 0;

    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->data_mutex);
    if ((uint16_t)(mqtt_handle->packet_id + 1) == 0) {
        mqtt_handle->packet_id = 0;
    }
    packet_id = ++mqtt_handle->packet_id;
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->data_mutex);

    return packet_id;
}

static int32_t _core_mqtt_publist_insert(core_mqtt_handle_t *mqtt_handle, uint8_t *packet, uint32_t len,
        uint16_t packet_id)
{
    core_mqtt_pub_node_t *node = NULL;
    uint16_t pack_num = 0;

    core_list_for_each_entry(node, &mqtt_handle->pub_list, linked_node, core_mqtt_pub_node_t) {
        if (node->packet_id == packet_id) {
            return STATE_MQTT_PUBLIST_PACKET_ID_ROLL;
        }
        pack_num++;
        if (pack_num > mqtt_handle->repub_list_limit) {
            return STATE_QOS_CACHE_EXCEEDS_LIMIT;
        }
    }

    node = mqtt_handle->sysdep->core_sysdep_malloc(sizeof(core_mqtt_pub_node_t), CORE_MQTT_MODULE_NAME);
    if (node == NULL) {
        return STATE_SYS_DEPEND_MALLOC_FAILED;
    }
    memset(node, 0, sizeof(core_mqtt_pub_node_t));
    CORE_INIT_LIST_HEAD(&node->linked_node);
    node->packet_id = packet_id;
    node->packet = mqtt_handle->sysdep->core_sysdep_malloc(len, CORE_MQTT_MODULE_NAME);
    if (node->packet == NULL) {
        mqtt_handle->sysdep->core_sysdep_free(node);
        return STATE_SYS_DEPEND_MALLOC_FAILED;
    }
    memset(node->packet, 0, len);
    memcpy(node->packet, packet, len);
    node->len = len;
    node->last_send_time = mqtt_handle->sysdep->core_sysdep_time();

    core_list_add_tail(&node->linked_node, &mqtt_handle->pub_list);

    return STATE_SUCCESS;
}

static void _core_mqtt_publist_remove(core_mqtt_handle_t *mqtt_handle, uint16_t packet_id)
{
    core_mqtt_pub_node_t *node = NULL, *next = NULL;

    core_list_for_each_entry_safe(node, next, &mqtt_handle->pub_list,
                                  linked_node, core_mqtt_pub_node_t) {
        if (node->packet_id == packet_id) {
            core_list_del(&node->linked_node);
            mqtt_handle->sysdep->core_sysdep_free(node->packet);
            mqtt_handle->sysdep->core_sysdep_free(node);
            return;
        }
    }
}

static void _core_mqtt_publist_destroy(core_mqtt_handle_t *mqtt_handle)
{
    core_mqtt_pub_node_t *node = NULL, *next = NULL;

    core_list_for_each_entry_safe(node, next, &mqtt_handle->pub_list,
                                  linked_node, core_mqtt_pub_node_t) {
        core_list_del(&node->linked_node);
        mqtt_handle->sysdep->core_sysdep_free(node->packet);
        mqtt_handle->sysdep->core_sysdep_free(node);
    }
}

static int32_t _core_mqtt_subunsub(core_mqtt_handle_t *mqtt_handle, char *topic, uint16_t topic_len, sub_options_t *opts,
                                   uint8_t pkt_type, mqtt_properties_t *general_property)
{
    int32_t res = 0;
    uint16_t packet_id = 0;
    uint8_t *pkt = NULL;
    uint32_t idx = 0, pkt_len = 0, remainlen = 0;
    uint32_t props_len = 0;

    if (_core_mqtt_5_feature_is_enabled(mqtt_handle)) {
        res = core_mqtt_props_bound(general_property);
        if(res < STATE_SUCCESS) {
            return res;
        }
        props_len = res;
    }

    remainlen = CORE_MQTT_PACKETID_LEN + CORE_MQTT_UTF8_STR_EXTRA_LEN + topic_len + props_len;
    if (pkt_type == CORE_MQTT_SUB_PKT_TYPE) {
        remainlen += CORE_MQTT_REQUEST_QOS_LEN;
    }
    pkt_len = CORE_MQTT_FIXED_HEADER_LEN + CORE_MQTT_REMAINLEN_MAXLEN + remainlen;

    pkt = mqtt_handle->sysdep->core_sysdep_malloc(pkt_len, CORE_MQTT_MODULE_NAME);
    if (pkt == NULL) {
        return STATE_SYS_DEPEND_MALLOC_FAILED;
    }
    memset(pkt, 0, pkt_len);

    /* Subscribe/Unsubscribe Packet Type */
    pkt[idx++] = pkt_type | CORE_MQTT_SUB_PKT_RESERVE;

    /* Remaining Length */
    idx += _write_variable(remainlen, &pkt[idx]);

    /* Packet Id */
    packet_id = _core_mqtt_packet_id(mqtt_handle);
    pkt[idx++] = (uint8_t)((packet_id >> 8) & 0x00FF);
    pkt[idx++] = (uint8_t)((packet_id) & 0x00FF);

    if (_core_mqtt_5_feature_is_enabled(mqtt_handle) && props_len > 0) {
        res = core_mqtt_props_write(general_property, &pkt[idx], props_len);
        idx += res;
    }

    /* Topic */
    pkt[idx++] = (uint8_t)((topic_len >> 8) & 0x00FF);
    pkt[idx++] = (uint8_t)((topic_len) & 0x00FF);
    memcpy(&pkt[idx], topic, topic_len);
    idx += topic_len;

    /* sub options */
    if (pkt_type == CORE_MQTT_SUB_PKT_TYPE ) {
        if(opts != NULL) {
            pkt[idx++] = opts->qos | opts->no_local << 2 | opts->retain_as_publish << 3 | opts->retain_handling << 4;
        } else {
            pkt[idx++] = 0;
        }
    }

    pkt_len = idx;

    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->send_mutex);
    res = _core_mqtt_write(mqtt_handle, pkt, pkt_len, mqtt_handle->send_timeout_ms);
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->send_mutex);
    if (res < STATE_SUCCESS) {
        mqtt_handle->sysdep->core_sysdep_free(pkt);
        if (res != STATE_SYS_DEPEND_NWK_WRITE_LESSDATA) {
            mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->send_mutex);
            mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->recv_mutex);
            if (mqtt_handle->network_handle != NULL) {
                mqtt_handle->sysdep->core_sysdep_network_deinit(&mqtt_handle->network_handle);
            }
            mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->recv_mutex);
            mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->send_mutex);
        }
        return res;
    }

    mqtt_handle->sysdep->core_sysdep_free(pkt);

    return packet_id;
}

static int32_t _core_mqtt_heartbeat(core_mqtt_handle_t *mqtt_handle)
{
    int32_t res = 0;
    uint8_t pingreq_pkt[2] = { CORE_MQTT_PINGREQ_PKT_TYPE, 0x00 };

    _core_mqtt_heartbeat_diag(mqtt_handle, 0x00);

    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->send_mutex);
    res = _core_mqtt_write(mqtt_handle, pingreq_pkt, 2, mqtt_handle->send_timeout_ms);
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->send_mutex);
    if (res < STATE_SUCCESS) {
        if (res != STATE_SYS_DEPEND_NWK_WRITE_LESSDATA) {
            mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->send_mutex);
            mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->recv_mutex);
            if (mqtt_handle->network_handle != NULL) {
                mqtt_handle->sysdep->core_sysdep_network_deinit(&mqtt_handle->network_handle);
            }
            mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->recv_mutex);
            mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->send_mutex);
        }
        return res;
    }

    return STATE_SUCCESS;
}

static int32_t _core_mqtt_repub(core_mqtt_handle_t *mqtt_handle)
{
    int32_t res = 0;
    uint64_t time_now = 0;
    core_mqtt_pub_node_t *node = NULL;

    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->pub_mutex);
    core_list_for_each_entry(node, &mqtt_handle->pub_list, linked_node, core_mqtt_pub_node_t) {
        time_now = mqtt_handle->sysdep->core_sysdep_time();
        if (time_now < node->last_send_time) {
            node->last_send_time = time_now;
        }
        if ((time_now - node->last_send_time) >= mqtt_handle->repub_timeout_ms) {
            mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->send_mutex);
            res = _core_mqtt_write(mqtt_handle, node->packet, node->len, mqtt_handle->send_timeout_ms);
            mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->send_mutex);
            node->last_send_time = time_now;
            if (res < STATE_SUCCESS) {
                mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->pub_mutex);
                if (res != STATE_SYS_DEPEND_NWK_WRITE_LESSDATA) {
                    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->send_mutex);
                    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->recv_mutex);
                    if (mqtt_handle->network_handle != NULL) {
                        mqtt_handle->sysdep->core_sysdep_network_deinit(&mqtt_handle->network_handle);
                    }
                    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->recv_mutex);
                    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->send_mutex);
                }
                return res;
            }
        }
    }
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->pub_mutex);

    return STATE_SUCCESS;
}

static int32_t _core_mqtt_process_datalist_insert(core_mqtt_handle_t *mqtt_handle,
        core_mqtt_process_data_t *process_data)
{
    core_mqtt_process_data_node_t *node = NULL;

    core_list_for_each_entry(node, &mqtt_handle->process_data_list,
                             linked_node, core_mqtt_process_data_node_t) {
        if (node->process_data.handler == process_data->handler) {
            node->process_data.context = process_data->context;
            return STATE_SUCCESS;
        }
    }

    if (&node->linked_node == &mqtt_handle->process_data_list) {
        node = mqtt_handle->sysdep->core_sysdep_malloc(sizeof(core_mqtt_process_data_node_t), CORE_MQTT_MODULE_NAME);
        if (node == NULL) {
            return STATE_SYS_DEPEND_MALLOC_FAILED;
        }
        memset(node, 0, sizeof(core_mqtt_process_data_node_t));
        CORE_INIT_LIST_HEAD(&node->linked_node);
        memcpy(&node->process_data, process_data, sizeof(core_mqtt_process_data_t));

        core_list_add_tail(&node->linked_node, &mqtt_handle->process_data_list);
    }

    return STATE_SUCCESS;
}

static void _core_mqtt_process_datalist_remove(core_mqtt_handle_t *mqtt_handle,
        core_mqtt_process_data_t *process_data)
{
    core_mqtt_process_data_node_t *node = NULL, *next = NULL;

    core_list_for_each_entry_safe(node, next, &mqtt_handle->process_data_list,
                                  linked_node, core_mqtt_process_data_node_t) {
        if (node->process_data.handler == process_data->handler) {
            core_list_del(&node->linked_node);
            mqtt_handle->sysdep->core_sysdep_free(node);
            return;
        }
    }
}

static void _core_mqtt_process_datalist_destroy(core_mqtt_handle_t *mqtt_handle)
{
    core_mqtt_process_data_node_t *node = NULL, *next = NULL;

    core_list_for_each_entry_safe(node, next, &mqtt_handle->process_data_list,
                                  linked_node, core_mqtt_process_data_node_t) {
        core_list_del(&node->linked_node);
        mqtt_handle->sysdep->core_sysdep_free(node);
    }
}

static int32_t _core_mqtt_append_process_data(core_mqtt_handle_t *mqtt_handle,
        core_mqtt_process_data_t *process_data)
{
    int32_t res = STATE_SUCCESS;

    if (process_data->handler == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }

    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->process_handler_mutex);
    res = _core_mqtt_process_datalist_insert(mqtt_handle, process_data);
    if (res >= STATE_SUCCESS && mqtt_handle->has_connected == 1) {
        aiot_mqtt_event_t event;

        memset(&event, 0, sizeof(aiot_mqtt_event_t));
        event.type = AIOT_MQTTEVT_CONNECT;

        process_data->handler(process_data->context, &event, NULL);
    }
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->process_handler_mutex);

    return res;
}

static int32_t _core_mqtt_remove_process_data(core_mqtt_handle_t *mqtt_handle,
        core_mqtt_process_data_t *process_data)
{
    int32_t res = STATE_SUCCESS;

    if (process_data->handler == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }

    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->process_handler_mutex);
    _core_mqtt_process_datalist_remove(mqtt_handle, process_data);
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->process_handler_mutex);

    return res;
}

static void _core_mqtt_process_data_process(core_mqtt_handle_t *mqtt_handle, core_mqtt_event_t *core_event)
{
    core_mqtt_process_data_node_t *node = NULL;

    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->process_handler_mutex);
    core_list_for_each_entry(node, &mqtt_handle->process_data_list,
                             linked_node, core_mqtt_process_data_node_t) {
        node->process_data.handler(node->process_data.context, NULL, core_event);
    }
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->process_handler_mutex);
}

static int32_t _core_mqtt_reconnect(core_mqtt_handle_t *mqtt_handle)
{
    int32_t res = STATE_SYS_DEPEND_NWK_CLOSED;
    uint64_t time_now = 0;
    uint32_t interval_ms = mqtt_handle->reconnect_params.interval_ms;
    if (mqtt_handle->reconnect_params.backoff_enabled) {
        interval_ms = mqtt_handle->reconnect_params.interval_ms * (mqtt_handle->reconnect_params.reconnect_counter + 1) +
                      mqtt_handle->reconnect_params.rand_ms;
    }

    if (mqtt_handle->network_handle != NULL) {
        return STATE_SUCCESS;
    }

    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->data_mutex);
    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->send_mutex);
    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->recv_mutex);
    time_now = mqtt_handle->sysdep->core_sysdep_time();
    if (time_now < mqtt_handle->reconnect_params.last_retry_time) {
        mqtt_handle->reconnect_params.last_retry_time = time_now;
    }
    if (time_now >= (mqtt_handle->reconnect_params.last_retry_time + interval_ms)) {
        core_log(mqtt_handle->sysdep, STATE_MQTT_LOG_RECONNECTING, "MQTT network disconnect, try to reconnecting...\r\n");
        res = _core_mqtt_connect(mqtt_handle, NULL, mqtt_handle->conn_props);
        mqtt_handle->reconnect_params.last_retry_time = mqtt_handle->sysdep->core_sysdep_time();
        if (mqtt_handle->reconnect_params.backoff_enabled) {
            if (STATE_MQTT_CONNECT_SUCCESS == res) {
                mqtt_handle->reconnect_params.reconnect_counter = 0;
            } else if (mqtt_handle->reconnect_params.reconnect_counter < CORE_MQTT_DEFAULT_RECONN_MAX_COUNTERS) {
                mqtt_handle->reconnect_params.reconnect_counter++;
            }
        }
    }
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->recv_mutex);
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->send_mutex);
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->data_mutex);

    return res;
}

static int32_t _core_mqtt_read_remainlen(core_mqtt_handle_t *mqtt_handle, uint32_t *remainlen)
{
    int32_t res = 0;
    uint8_t ch = 0;
    uint32_t multiplier = 1;
    uint32_t mqtt_remainlen = 0;

    do {
        res = _core_mqtt_read(mqtt_handle, &ch, 1, mqtt_handle->recv_timeout_ms);
        if (res < STATE_SUCCESS) {
            return res;
        }
        mqtt_remainlen += (ch & 127) * multiplier;
        if (multiplier > 128 * 128 * 128) {
            return STATE_MQTT_MALFORMED_REMAINING_LEN;
        }
        multiplier *= 128;
    } while ((ch & 128) != 0);

    *remainlen = mqtt_remainlen;

    return STATE_SUCCESS;
}

static int32_t _core_mqtt_read_remainbytes(core_mqtt_handle_t *mqtt_handle, uint32_t remainlen, uint8_t **output)
{
    int32_t res = 0;
    uint8_t *remain = NULL;

    if (remainlen > 0) {
        remain = mqtt_handle->sysdep->core_sysdep_malloc(remainlen, CORE_MQTT_MODULE_NAME);
        if (remain == NULL) {
            return STATE_SYS_DEPEND_MALLOC_FAILED;
        }
        memset(remain, 0, remainlen);

        res = _core_mqtt_read(mqtt_handle, remain, remainlen, mqtt_handle->recv_timeout_ms);
        if (res < STATE_SUCCESS) {
            mqtt_handle->sysdep->core_sysdep_free(remain);
            if (res == STATE_SYS_DEPEND_NWK_READ_LESSDATA) {
                return STATE_MQTT_MALFORMED_REMAINING_BYTES;
            } else {
                return res;
            }
        }
    }
    *output = remain;

    return STATE_SUCCESS;
}

static int32_t _core_mqtt_pingresp_handler(core_mqtt_handle_t *mqtt_handle, uint8_t *input, uint32_t len)
{
    aiot_mqtt_recv_t packet;
    uint64_t rtt = mqtt_handle->sysdep->core_sysdep_time()  \
                   - mqtt_handle->heartbeat_params.last_send_time;

    (void)input;

    if (len != 0) {
        return STATE_MQTT_RECV_INVALID_PINRESP_PACKET;
    }

    /* heartbreat rtt stats */
    if (rtt < CORE_MQTT_NWKSTATS_RTT_THRESHOLD) {
        mqtt_handle->nwkstats_info.rtt = (mqtt_handle->nwkstats_info.rtt + rtt) / 2;
    }

    memset(&packet, 0, sizeof(aiot_mqtt_recv_t));

    if (mqtt_handle->recv_handler != NULL) {
        packet.type = AIOT_MQTTRECV_HEARTBEAT_RESPONSE;
        mqtt_handle->recv_handler((void *)mqtt_handle, &packet, mqtt_handle->userdata);
    }

    _core_mqtt_heartbeat_diag(mqtt_handle, 0x01);

    return STATE_SUCCESS;
}

static int32_t _core_mqtt_puback_send(core_mqtt_handle_t *mqtt_handle, uint16_t packet_id)
{
    int32_t res = 0;
    uint8_t pkt[4] = {0};

    /* Packet Type */
    pkt[0] = CORE_MQTT_PUBACK_PKT_TYPE;

    /* Remaining Lengh */
    pkt[1] = 2;

    /* Packet Id */
    pkt[2] = (uint16_t)((packet_id >> 8) & 0x00FF);
    pkt[3] = (uint16_t)((packet_id) & 0x00FF);

    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->send_mutex);
    res = _core_mqtt_write(mqtt_handle, pkt, 4, mqtt_handle->send_timeout_ms);
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->send_mutex);
    if (res < STATE_SUCCESS) {
        if (res != STATE_SYS_DEPEND_NWK_WRITE_LESSDATA) {
            mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->send_mutex);
            mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->recv_mutex);
            if (mqtt_handle->network_handle != NULL) {
                mqtt_handle->sysdep->core_sysdep_network_deinit(&mqtt_handle->network_handle);
            }
            mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->recv_mutex);
            mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->send_mutex);
        }
        return res;
    }

    return STATE_SUCCESS;
}

int32_t _core_mqtt_topic_compare(char *topic, uint32_t topic_len, char *cmp_topic, uint32_t cmp_topic_len)
{
    uint32_t idx = 0, cmp_idx = 0;

    for (idx = 0, cmp_idx = 0; idx < topic_len; idx++) {
        /* compare topic alreay out of bounds */
        if (cmp_idx >= cmp_topic_len) {
            /* compare success only in case of the left string of topic is "/#" */
            if ((topic_len - idx == 2) && (memcmp(&topic[idx], "/#", 2) == 0)) {
                return STATE_SUCCESS;
            } else {
                return STATE_MQTT_TOPIC_COMPARE_FAILED;
            }
        }

        /* if topic reach the '#', compare success */
        if (topic[idx] == '#') {
            return STATE_SUCCESS;
        }

        if (topic[idx] == '+') {
            /* wildcard + exist */
            for (; cmp_idx < cmp_topic_len; cmp_idx++) {
                if (cmp_topic[cmp_idx] == '/') {
                    /* if topic already reach the bound, compare topic should not contain '/' */
                    if (idx + 1 == topic_len) {
                        return STATE_MQTT_TOPIC_COMPARE_FAILED;
                    } else {
                        break;
                    }
                }
            }
        } else {
            /* compare each character */
            if (topic[idx] != cmp_topic[cmp_idx]) {
                return STATE_MQTT_TOPIC_COMPARE_FAILED;
            }
            cmp_idx++;
        }
    }

    /* compare topic should be reach the end */
    if (cmp_idx < cmp_topic_len) {
        return STATE_MQTT_TOPIC_COMPARE_FAILED;
    }
    return STATE_SUCCESS;
}

static void _core_mqtt_handlerlist_append(core_mqtt_handle_t *mqtt_handle, struct core_list_head *dest,
        struct core_list_head *src, uint8_t *found)
{
    core_mqtt_sub_handler_node_t *node = NULL, *copy_node = NULL;

    core_list_for_each_entry(node, src, linked_node, core_mqtt_sub_handler_node_t) {
        copy_node = mqtt_handle->sysdep->core_sysdep_malloc(sizeof(core_mqtt_sub_handler_node_t), CORE_MQTT_MODULE_NAME);
        if (copy_node == NULL) {
            continue;
        }
        memset(copy_node, 0, sizeof(core_mqtt_sub_handler_node_t));
        CORE_INIT_LIST_HEAD(&copy_node->linked_node);
        copy_node->handler = node->handler;
        copy_node->userdata = node->userdata;

        core_list_add_tail(&copy_node->linked_node, dest);
        *found = 1;
    }
}

static void _core_mqtt_handlerlist_destroy(core_mqtt_handle_t *mqtt_handle, struct core_list_head *list)
{
    core_mqtt_sub_handler_node_t *node = NULL, *next = NULL;

    core_list_for_each_entry_safe(node, next, list, linked_node, core_mqtt_sub_handler_node_t) {
        core_list_del(&node->linked_node);
        mqtt_handle->sysdep->core_sysdep_free(node);
    }
}

uint8_t _core_mqtt_process_topic_alias(core_mqtt_handle_t *mqtt_handle, uint16_t topic_alias, uint32_t topic_len,
                                       char *topic,
                                       core_mqtt_topic_alias_node_t *alias_node)
{
    core_mqtt_topic_alias_node_t *topic_alias_node = NULL;

    uint8_t use_alias_topic = 0;
    if (topic_len > 0) {
        /* A topic appears first time during this connection. */
        core_mqtt_buff_t alias_topic = {
            .buffer = (uint8_t *)topic,
            .len = topic_len,
        };
        /* Insert it into topic alias list */
        _core_mqtt_topic_alias_list_insert(mqtt_handle, &alias_topic, topic_alias,
                                            &(mqtt_handle->rx_topic_alias_list));
    } else {
        /* topic len is 0. User previos topic alias */
        mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->topic_alias_mutex);
        core_list_for_each_entry(topic_alias_node, &mqtt_handle->rx_topic_alias_list, linked_node,
                                    core_mqtt_topic_alias_node_t) {
            if (topic_alias_node->topic_alias == topic_alias) {
                core_log(mqtt_handle->sysdep, STATE_MQTT_LOG_CONNECT, "found a rx topic alias\n");
                use_alias_topic = 1;
                memcpy(alias_node, topic_alias_node, sizeof(core_mqtt_topic_alias_node_t));
                break;
            }
        }
        mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->topic_alias_mutex);
    }
    return use_alias_topic;
}
static void _core_mqtt_call_user_handler(core_mqtt_handle_t *mqtt_handle, core_mqtt_msg_t msg, uint8_t qos, mqtt_properties_t *pub_prop)
{
    void *userdata;
    uint8_t sub_found = 0;
    core_mqtt_sub_node_t *sub_node = NULL;
    core_mqtt_sub_handler_node_t *handler_node = NULL;
    struct core_list_head handler_list_copy;
    aiot_mqtt_recv_t packet;
    uint32_t topic_len = 0;

    /* debug */
    topic_len = (uint32_t)msg.topic_len;
    core_log2(mqtt_handle->sysdep, STATE_MQTT_LOG_TOPIC, "pub: %.*s\r\n", &topic_len,
              msg.topic);
    core_log_hexdump(STATE_MQTT_LOG_HEXDUMP, '<', msg.payload, msg.payload_len);

    memset(&packet, 0, sizeof(aiot_mqtt_recv_t));
    packet.type = AIOT_MQTTRECV_PUB;
    packet.data.pub.qos = qos;
    packet.data.pub.payload = msg.payload;
    packet.data.pub.payload_len = msg.payload_len;
    packet.data.pub.topic = msg.topic;
    packet.data.pub.topic_len = msg.topic_len;
    packet.data.pub.props = pub_prop;

    /* Search Packet Handler In sublist */
    CORE_INIT_LIST_HEAD(&handler_list_copy);
    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->sub_mutex);
    core_list_for_each_entry(sub_node, &mqtt_handle->sub_list, linked_node, core_mqtt_sub_node_t) {
        if (_core_mqtt_topic_compare(sub_node->topic, (uint32_t)(strlen(sub_node->topic)), packet.data.pub.topic,
                                     packet.data.pub.topic_len) == STATE_SUCCESS) {
            _core_mqtt_handlerlist_append(mqtt_handle, &handler_list_copy, &sub_node->handle_list, &sub_found);
        }
    }
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->sub_mutex);

    core_list_for_each_entry(handler_node, &handler_list_copy,
                             linked_node, core_mqtt_sub_handler_node_t) {
        userdata = (handler_node->userdata == NULL) ? (mqtt_handle->userdata) : (handler_node->userdata);
        handler_node->handler(mqtt_handle, &packet, userdata);
    }
    _core_mqtt_handlerlist_destroy(mqtt_handle, &handler_list_copy);

    /* User Data Default Packet Handler */
    if (mqtt_handle->recv_handler && sub_found == 0) {
        mqtt_handle->recv_handler((void *)mqtt_handle, &packet, mqtt_handle->userdata);
    }
}
  
static int32_t _core_mqtt_pub_handler(core_mqtt_handle_t *mqtt_handle, uint8_t *input, uint32_t len, uint8_t qos,
                                      uint32_t remain_len)
{
    uint32_t idx = 0;
    uint16_t packet_id = 0;
    uint16_t utf8_strlen = 0;
    core_mqtt_msg_t src, dest;
    int32_t res = STATE_SUCCESS;
    core_mqtt_topic_alias_node_t topic_alias_node = {0};
    uint8_t use_alias_topic  = 0;
    mqtt_properties_t *props = NULL;
    mqtt_property_t *alias_prop = NULL;

    (void)remain_len;

    if (input == NULL || len == 0 || qos > CORE_MQTT_QOS1) {
        return STATE_MQTT_RECV_INVALID_PUBLISH_PACKET;
    }

    /* Topic Length */
    if (0 == _core_mqtt_5_feature_is_enabled(mqtt_handle) && len < 2) {
        return STATE_MQTT_RECV_INVALID_PUBLISH_PACKET;
    }

    utf8_strlen = input[idx++] << 8;
    utf8_strlen |= input[idx++];

    src.topic = (char *)&input[idx];
    src.topic_len = utf8_strlen;
    idx += utf8_strlen;

    /* Packet Id For QOS1 */
    if (len < idx) {
        return STATE_MQTT_RECV_INVALID_PUBLISH_PACKET;
    }
    if (qos == CORE_MQTT_QOS1) {
        if (len < idx + 2) {
            return STATE_MQTT_RECV_INVALID_PUBLISH_PACKET;
        }
        packet_id = input[idx++] << 8;
        packet_id |= input[idx++];

    }

    if (_core_mqtt_5_feature_is_enabled(mqtt_handle)) {
        props = aiot_mqtt_props_init();
        res = core_mqtt_props_read(&input[idx], len - idx, props);
        idx += res;

        alias_prop = aiot_mqtt_props_get(props, MQTT_PROP_ID_TOPIC_ALIAS, 0);
        if(alias_prop != NULL) {
            use_alias_topic = _core_mqtt_process_topic_alias(mqtt_handle, alias_prop->value.uint16, src.topic_len,
                    src.topic, &topic_alias_node);
        }
    }

    /* Payload Len */
    if ((int64_t)len - CORE_MQTT_PUBLISH_TOPICLEN_LEN - (int64_t)src.topic_len < 0) {
        return STATE_MQTT_RECV_INVALID_PUBLISH_PACKET;
    }
    src.payload = &input[idx];
    src.payload_len = len - CORE_MQTT_PUBLISH_TOPICLEN_LEN - src.topic_len - res;

    if (qos == CORE_MQTT_QOS1) {
        src.payload_len -= CORE_MQTT_PACKETID_LEN;
    }

    /* Publish Ack For QOS1 */
    if (qos == CORE_MQTT_QOS1) {
        _core_mqtt_puback_send(mqtt_handle, packet_id);
    }

    /* User previos topic alias */
    if (_core_mqtt_5_feature_is_enabled(mqtt_handle) && 1 == use_alias_topic) {
        src.topic = topic_alias_node.topic;
        src.topic_len = strlen(src.topic);
    }

    if(mqtt_handle->decompress.handler != NULL) {
        res = mqtt_handle->decompress.handler(mqtt_handle->decompress.context, &src, &dest);
        if(res < STATE_SUCCESS) {
            core_log(mqtt_handle->sysdep, STATE_MQTT_BASE, "decompress error \r\n");
            return res;
        }
        _core_mqtt_call_user_handler(mqtt_handle, dest, qos, props);
        /* 如预处理有生成新的payload,需要进行资源回收 */
        if(res == STATE_COMPRESS_SUCCESS && dest.payload != NULL) {
            mqtt_handle->sysdep->core_sysdep_free(dest.payload);
        }
    } else{
        _core_mqtt_call_user_handler(mqtt_handle, src, qos, props);
    }

    /* Reclaims Mem of User Properties */
    if (props != NULL) {
        aiot_mqtt_props_deinit(&props);
    }

    return STATE_SUCCESS;
}


void _core_mqtt_flow_control_inc(core_mqtt_handle_t *mqtt_handle)
{
    if (_core_mqtt_5_feature_is_enabled(mqtt_handle) && mqtt_handle->flow_control_enabled) {
        mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->pub_mutex);
        mqtt_handle->server_receive_max++;
        mqtt_handle->server_receive_max = (mqtt_handle->server_receive_max > CORE_DEFAULT_SERVER_RECEIVE_MAX) ?
                                          CORE_DEFAULT_SERVER_RECEIVE_MAX : mqtt_handle->server_receive_max;
        mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->pub_mutex);
    }
}

static int32_t _core_mqtt_puback_handler(core_mqtt_handle_t *mqtt_handle, uint8_t *input, uint32_t len)
{
    aiot_mqtt_recv_t packet;
    mqtt_properties_t *props = NULL;
    int32_t res = 0;

    /* even maltform puback received, flow control should be re-calculated as well */
    _core_mqtt_flow_control_inc(mqtt_handle);

    if (input == NULL) {
        return STATE_MQTT_RECV_INVALID_PUBACK_PACKET;
    }

    if (0 == _core_mqtt_5_feature_is_enabled(mqtt_handle) && len != 2) {
        return STATE_MQTT_RECV_INVALID_PUBACK_PACKET;
    } else if (_core_mqtt_5_feature_is_enabled(mqtt_handle) && len < 2) {
        return STATE_MQTT_RECV_INVALID_PUBACK_PACKET;
    }

    memset(&packet, 0, sizeof(aiot_mqtt_recv_t));
    packet.type = AIOT_MQTTRECV_PUB_ACK;

    /* Packet Id */
    packet.data.pub_ack.packet_id = input[0] << 8;
    packet.data.pub_ack.packet_id |= input[1];

    /* Properties */
    if (1 == _core_mqtt_5_feature_is_enabled(mqtt_handle) && len > 2) {
        props = aiot_mqtt_props_init();
        res = core_mqtt_props_read(&input[2], len - 2, props);
        if(res != 0) {
            packet.data.pub_ack.props = props;
        }
    }

    /* Remove Packet From republist */
    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->pub_mutex);
    _core_mqtt_publist_remove(mqtt_handle, packet.data.pub_ack.packet_id);
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->pub_mutex);

    /* User Ctrl Packet Handler */
    if (mqtt_handle->recv_handler) {
        mqtt_handle->recv_handler((void *)mqtt_handle, &packet, mqtt_handle->userdata);
    }

    if(props != NULL) {
        aiot_mqtt_props_deinit(&props);
    }

    return STATE_SUCCESS;
}

static int32_t _core_mqtt_server_disconnect_handler(core_mqtt_handle_t *mqtt_handle, uint8_t *input, uint32_t len)
{
    aiot_mqtt_recv_t packet;
    mqtt_properties_t *props = NULL;

    memset(&packet, 0, sizeof(aiot_mqtt_recv_t));
    packet.type = AIOT_MQTTRECV_DISCONNECT;
    /* 当remainlen < 1时，表示正常断开 */
    if (input == NULL || len == 0) {
        packet.data.server_disconnect.reason_code = 0;
    } else if(len < 2) {
        return STATE_MQTT_RECV_INVALID_SERVER_DISCONNECT_PACKET;
    } else {
        packet.data.server_disconnect.reason_code = input[0];
        props = aiot_mqtt_props_init();
        core_mqtt_props_read(&input[1], len - 1, props);
        packet.data.server_disconnect.props = props;
    }

    /* User Ctrl Packet Handler */
    if (mqtt_handle->recv_handler) {
        mqtt_handle->recv_handler((void *)mqtt_handle, &packet, mqtt_handle->userdata);
    }
    if(props != NULL) {
        aiot_mqtt_props_deinit(&props);
    }
    /* close socket connect with mqtt broker */
    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->send_mutex);
    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->recv_mutex);
    if (mqtt_handle->network_handle != NULL) {
        mqtt_handle->sysdep->core_sysdep_network_deinit(&mqtt_handle->network_handle);
    }
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->recv_mutex);
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->send_mutex);

    _core_mqtt_disconnect_event_notify(mqtt_handle, AIOT_MQTTDISCONNEVT_NETWORK_DISCONNECT);
    core_log(mqtt_handle->sysdep, STATE_MQTT_LOG_DISCONNECT, "MQTT receives server disconnect, disconnect\r\n");

    return STATE_SUCCESS;
}

static void _core_mqtt_subunsuback_handler(core_mqtt_handle_t *mqtt_handle, uint8_t *input, uint32_t len,
        uint8_t packet_type)
{
    uint32_t idx = 0;
    aiot_mqtt_recv_t packet;
    int32_t res = STATE_SUCCESS;
    mqtt_properties_t *props = NULL;

    if (0 == _core_mqtt_5_feature_is_enabled(mqtt_handle)) {
        if (input == NULL || len == 0 ||
            (packet_type == CORE_MQTT_SUBACK_PKT_TYPE && len % 3 != 0) ||
            (packet_type == CORE_MQTT_UNSUBACK_PKT_TYPE && len % 2 != 0)) {
            return;
        }

        for (idx = 0; idx < len;) {
            memset(&packet, 0, sizeof(aiot_mqtt_recv_t));
            if (packet_type == CORE_MQTT_SUBACK_PKT_TYPE) {
                packet.type = AIOT_MQTTRECV_SUB_ACK;
                packet.data.sub_ack.packet_id = input[idx] << 8;
                packet.data.sub_ack.packet_id |= input[idx + 1];
            } else {
                packet.type = AIOT_MQTTRECV_UNSUB_ACK;
                packet.data.unsub_ack.packet_id = input[idx] << 8;
                packet.data.unsub_ack.packet_id |= input[idx + 1];
            }

            if (packet_type == CORE_MQTT_SUBACK_PKT_TYPE) {
                if (input[idx + 2] == CORE_MQTT_SUBACK_RCODE_MAXQOS0 ||
                    input[idx + 2] == CORE_MQTT_SUBACK_RCODE_MAXQOS1 ||
                    input[idx + 2] == CORE_MQTT_SUBACK_RCODE_MAXQOS2) {
                    packet.data.sub_ack.res = STATE_SUCCESS;
                    packet.data.sub_ack.max_qos = input[idx + 2];
                } else if (input[idx + 2] == CORE_MQTT_SUBACK_RCODE_FAILURE) {
                    packet.data.sub_ack.res = STATE_MQTT_SUBACK_RCODE_FAILURE;
                } else {
                    packet.data.sub_ack.res = STATE_MQTT_SUBACK_RCODE_UNKNOWN;
                }
                idx += 3;
            } else {
                idx += 2;
            }

            if (mqtt_handle->recv_handler) {
                mqtt_handle->recv_handler((void *)mqtt_handle, &packet, mqtt_handle->userdata);
            }
        }
    } else {
        if (input == NULL || len == 0) {
            return;
        }

        /* Packet ID */
        memset(&packet, 0, sizeof(aiot_mqtt_recv_t));
        if (packet_type == CORE_MQTT_SUBACK_PKT_TYPE) {
            packet.type = AIOT_MQTTRECV_SUB_ACK;
            packet.data.sub_ack.packet_id = input[idx] << 8;
            packet.data.sub_ack.packet_id |= input[idx + 1];
        } else {
            packet.type = AIOT_MQTTRECV_UNSUB_ACK;
            packet.data.unsub_ack.packet_id = input[idx] << 8;
            packet.data.unsub_ack.packet_id |= input[idx + 1];
        }
        idx += 2;

        /* Properties */
        props = aiot_mqtt_props_init();
        res = core_mqtt_props_read(&input[idx], len - idx, props);
        idx += res;

        /* Suback Flags */
        if (packet_type == CORE_MQTT_SUBACK_PKT_TYPE) {
            packet.data.sub_ack.props = props;
            if (input[idx] == CORE_MQTT_SUBACK_RCODE_MAXQOS0 ||
                input[idx] == CORE_MQTT_SUBACK_RCODE_MAXQOS1 ||
                input[idx] == CORE_MQTT_SUBACK_RCODE_MAXQOS2) {
                packet.data.sub_ack.res = STATE_SUCCESS;
                packet.data.sub_ack.max_qos = input[idx];
            } else if (input[idx] == CORE_MQTT_SUBACK_RCODE_FAILURE) {
                packet.data.sub_ack.res = STATE_MQTT_SUBACK_RCODE_FAILURE;
            } else {
                packet.data.sub_ack.res = STATE_MQTT_SUBACK_RCODE_UNKNOWN;
            }
        } else {
            packet.data.unsub_ack.props = props;
        }

        if (mqtt_handle->recv_handler) {
            mqtt_handle->recv_handler((void *)mqtt_handle, &packet, mqtt_handle->userdata);
        }

        if(props != NULL) {
            aiot_mqtt_props_deinit(&props);
        }
    }
}

int32_t core_mqtt_setopt(void *handle, core_mqtt_option_t option, void *data)
{
    int32_t res = 0;
    core_mqtt_handle_t *mqtt_handle = (core_mqtt_handle_t *)handle;

    if (mqtt_handle == NULL || data == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }
    if (option >= CORE_MQTTOPT_MAX) {
        return STATE_USER_INPUT_OUT_RANGE;
    }

    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->data_mutex);
    switch (option) {
        case CORE_MQTTOPT_APPEND_PROCESS_HANDLER: {
            mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->data_mutex);
            res = _core_mqtt_append_process_data(mqtt_handle, (core_mqtt_process_data_t *)data);
            mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->data_mutex);
        }
        break;
        case CORE_MQTTOPT_REMOVE_PROCESS_HANDLER: {
            mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->data_mutex);
            res = _core_mqtt_remove_process_data(mqtt_handle, (core_mqtt_process_data_t *)data);
            mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->data_mutex);
        }
        break;
        case CORE_MQTTOPT_COMPRESS_HANDLER: {
            mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->data_mutex);
            mqtt_handle->compress = *(core_mqtt_compress_data_t *)data;
            mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->data_mutex);
        }
        break;
        case CORE_MQTTOPT_DECOMPRESS_HANDLER: {
            mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->data_mutex);
            mqtt_handle->decompress = *(core_mqtt_compress_data_t *)data;
            mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->data_mutex);
        }
        break;
        default: {
            res = STATE_USER_INPUT_UNKNOWN_OPTION;
        }
    }
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->data_mutex);

    return res;
}

void *aiot_mqtt_init(void)
{
    int32_t res = STATE_SUCCESS;
    uint32_t rand_value = 0;
    core_mqtt_handle_t *mqtt_handle = NULL;
    aiot_sysdep_portfile_t *sysdep = NULL;

    sysdep = aiot_sysdep_get_portfile();
    if (sysdep == NULL) {
        return NULL;
    }

    res = core_global_init(sysdep);
    if (res < STATE_SUCCESS) {
        return NULL;
    }

    mqtt_handle = sysdep->core_sysdep_malloc(sizeof(core_mqtt_handle_t), CORE_MQTT_MODULE_NAME);
    if (mqtt_handle == NULL) {
        core_global_deinit(sysdep);
        return NULL;
    }
    sysdep->core_sysdep_rand((uint8_t *)&rand_value, sizeof(rand_value));
    memset(mqtt_handle, 0, sizeof(core_mqtt_handle_t));

    mqtt_handle->sysdep = sysdep;
    mqtt_handle->keep_alive_s = CORE_MQTT_DEFAULT_KEEPALIVE_S;
    mqtt_handle->clean_session = CORE_MQTT_DEFAULT_CLEAN_SESSION;
    mqtt_handle->connect_timeout_ms = CORE_MQTT_DEFAULT_CONNECT_TIMEOUT_MS;
    mqtt_handle->heartbeat_params.interval_ms = CORE_MQTT_DEFAULT_HEARTBEAT_INTERVAL_MS;
    mqtt_handle->heartbeat_params.max_lost_times = CORE_MQTT_DEFAULT_HEARTBEAT_MAX_LOST_TIMES;
    mqtt_handle->reconnect_params.enabled = CORE_MQTT_DEFAULT_RECONN_ENABLED;
    mqtt_handle->reconnect_params.interval_ms = CORE_MQTT_DEFAULT_RECONN_INTERVAL_MS;
    mqtt_handle->reconnect_params.backoff_enabled = 1;
    mqtt_handle->reconnect_params.rand_ms = rand_value % CORE_MQTT_DEFAULT_RECONN_RANDLIMIT_MS -
                                            CORE_MQTT_DEFAULT_RECONN_RANDLIMIT_MS / 2;
    mqtt_handle->reconnect_params.reconnect_counter = 0;
    mqtt_handle->send_timeout_ms = CORE_MQTT_DEFAULT_SEND_TIMEOUT_MS;
    mqtt_handle->recv_timeout_ms = CORE_MQTT_DEFAULT_RECV_TIMEOUT_MS;
    mqtt_handle->repub_timeout_ms = CORE_MQTT_DEFAULT_REPUB_TIMEOUT_MS;
    mqtt_handle->deinit_timeout_ms = CORE_MQTT_DEFAULT_DEINIT_TIMEOUT_MS;
    mqtt_handle->repub_list_limit = CORE_MQTT_DEFAULT_REPUB_LIST_LIMIT;

    mqtt_handle->data_mutex = sysdep->core_sysdep_mutex_init();
    mqtt_handle->send_mutex = sysdep->core_sysdep_mutex_init();
    mqtt_handle->recv_mutex = sysdep->core_sysdep_mutex_init();
    mqtt_handle->sub_mutex = sysdep->core_sysdep_mutex_init();
    mqtt_handle->pub_mutex = sysdep->core_sysdep_mutex_init();
    mqtt_handle->topic_alias_mutex = sysdep->core_sysdep_mutex_init();
    mqtt_handle->process_handler_mutex = sysdep->core_sysdep_mutex_init();
    mqtt_handle->protocol_version = AIOT_MQTT_VERSION_3_1;
    mqtt_handle->tx_packet_max_size = CORE_TX_PKT_MAX_LENGTH;

    CORE_INIT_LIST_HEAD(&mqtt_handle->sub_list);
    CORE_INIT_LIST_HEAD(&mqtt_handle->pub_list);
    CORE_INIT_LIST_HEAD(&mqtt_handle->process_data_list);
    CORE_INIT_LIST_HEAD(&mqtt_handle->rx_topic_alias_list);
    CORE_INIT_LIST_HEAD(&mqtt_handle->tx_topic_alias_list);

    mqtt_handle->exec_enabled = 1;
    mqtt_handle->server_receive_max = CORE_DEFAULT_SERVER_RECEIVE_MAX;
    mqtt_handle->topic_header_check = 1;

    return mqtt_handle;
}

int32_t aiot_mqtt_setopt(void *handle, aiot_mqtt_option_t option, void *data)
{
    int32_t res = STATE_SUCCESS;
    core_mqtt_handle_t *mqtt_handle = (core_mqtt_handle_t *)handle;

    if (mqtt_handle == NULL || data == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }
    if (option >= AIOT_MQTTOPT_MAX) {
        return STATE_USER_INPUT_OUT_RANGE;
    }
    if (mqtt_handle->exec_enabled == 0) {
        return STATE_USER_INPUT_EXEC_DISABLED;
    }

    _core_mqtt_exec_inc(mqtt_handle);

    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->data_mutex);
    switch (option) {
        case AIOT_MQTTOPT_HOST: {
            res = core_strdup(mqtt_handle->sysdep, &mqtt_handle->host, (char *)data, CORE_MQTT_MODULE_NAME);
        }
        break;
        case AIOT_MQTTOPT_PORT: {
            mqtt_handle->port = *(uint16_t *)data;
        }
        break;
        case AIOT_MQTTOPT_PRODUCT_KEY: {
            res = core_strdup(mqtt_handle->sysdep, &mqtt_handle->product_key, (char *)data, CORE_MQTT_MODULE_NAME);
        }
        break;
        case AIOT_MQTTOPT_DEVICE_NAME: {
            res = core_strdup(mqtt_handle->sysdep, &mqtt_handle->device_name, (char *)data, CORE_MQTT_MODULE_NAME);
        }
        break;
        case AIOT_MQTTOPT_DEVICE_SECRET: {
            res = core_strdup(mqtt_handle->sysdep, &mqtt_handle->device_secret, (char *)data, CORE_MQTT_MODULE_NAME);
            _core_mqtt_sign_clean(mqtt_handle);
        }
        break;
        case AIOT_MQTTOPT_EXTEND_CLIENTID: {
            res = core_strdup(mqtt_handle->sysdep, &mqtt_handle->extend_clientid, (char *)data, CORE_MQTT_MODULE_NAME);
            _core_mqtt_sign_clean(mqtt_handle);
        }
        break;
        case AIOT_MQTTOPT_SECURITY_MODE: {
            res = core_strdup(mqtt_handle->sysdep, &mqtt_handle->security_mode, (char *)data, CORE_MQTT_MODULE_NAME);
            _core_mqtt_sign_clean(mqtt_handle);
        }
        break;
        case AIOT_MQTTOPT_USERNAME: {
            res = core_strdup(mqtt_handle->sysdep, &mqtt_handle->username, (char *)data, CORE_MQTT_MODULE_NAME);
        }
        break;
        case AIOT_MQTTOPT_PASSWORD: {
            res = core_strdup(mqtt_handle->sysdep, &mqtt_handle->password, (char *)data, CORE_MQTT_MODULE_NAME);
        }
        break;
        case AIOT_MQTTOPT_CLIENTID: {
            res = core_strdup(mqtt_handle->sysdep, &mqtt_handle->clientid, (char *)data, CORE_MQTT_MODULE_NAME);
        }
        break;
        case AIOT_MQTTOPT_KEEPALIVE_SEC: {
            mqtt_handle->keep_alive_s = *(uint16_t *)data;
        }
        break;
        case AIOT_MQTTOPT_CLEAN_SESSION: {
            if (*(uint8_t *)data != 0 && *(uint8_t *)data != 1) {
                res = STATE_USER_INPUT_OUT_RANGE;
            }
            mqtt_handle->clean_session = *(uint8_t *)data;
        }
        break;
        case AIOT_MQTTOPT_NETWORK_CRED: {
            if (mqtt_handle->cred != NULL) {
                mqtt_handle->sysdep->core_sysdep_free(mqtt_handle->cred);
                mqtt_handle->cred = NULL;
            }
            mqtt_handle->cred = mqtt_handle->sysdep->core_sysdep_malloc(sizeof(aiot_sysdep_network_cred_t), CORE_MQTT_MODULE_NAME);
            if (mqtt_handle->cred != NULL) {
                memset(mqtt_handle->cred, 0, sizeof(aiot_sysdep_network_cred_t));
                memcpy(mqtt_handle->cred, data, sizeof(aiot_sysdep_network_cred_t));
            } else {
                res = STATE_SYS_DEPEND_MALLOC_FAILED;
            }
        }
        break;
        case AIOT_MQTTOPT_CONNECT_TIMEOUT_MS: {
            mqtt_handle->connect_timeout_ms = *(uint32_t *)data;
        }
        break;
        case AIOT_MQTTOPT_HEARTBEAT_INTERVAL_MS: {
            mqtt_handle->heartbeat_params.interval_ms = *(uint32_t *)data;
        }
        break;
        case AIOT_MQTTOPT_HEARTBEAT_MAX_LOST: {
            mqtt_handle->heartbeat_params.max_lost_times = *(uint8_t *)data;
        }
        break;
        case AIOT_MQTTOPT_RECONN_ENABLED: {
            if (*(uint8_t *)data != 0 && *(uint8_t *)data != 1) {
                res = STATE_USER_INPUT_OUT_RANGE;
            }
            mqtt_handle->reconnect_params.enabled = *(uint8_t *)data;
        }
        break;
        case AIOT_MQTTOPT_RECONN_INTERVAL_MS: {
            mqtt_handle->reconnect_params.interval_ms = *(uint32_t *)data;
            mqtt_handle->reconnect_params.backoff_enabled = 0;
        }
        break;
        case AIOT_MQTTOPT_SEND_TIMEOUT_MS: {
            mqtt_handle->send_timeout_ms = *(uint32_t *)data;
        }
        break;
        case AIOT_MQTTOPT_RECV_TIMEOUT_MS: {
            mqtt_handle->recv_timeout_ms = *(uint32_t *)data;
        }
        break;
        case AIOT_MQTTOPT_REPUB_TIMEOUT_MS: {
            mqtt_handle->repub_timeout_ms = *(uint32_t *)data;
        }
        break;
        case AIOT_MQTTOPT_DEINIT_TIMEOUT_MS: {
            mqtt_handle->deinit_timeout_ms = *(uint32_t *)data;
        }
        break;
        case AIOT_MQTTOPT_RECV_HANDLER: {
            mqtt_handle->recv_handler = (aiot_mqtt_recv_handler_t)data;
        }
        break;
        case AIOT_MQTTOPT_EVENT_HANDLER: {
            mqtt_handle->event_handler = (aiot_mqtt_event_handler_t)data;
        }
        break;
        case AIOT_MQTTOPT_APPEND_TOPIC_MAP: {
            mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->data_mutex);
            res = _core_mqtt_append_topic_map(mqtt_handle, (aiot_mqtt_topic_map_t *)data);
            mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->data_mutex);
        }
        break;
        case AIOT_MQTTOPT_REMOVE_TOPIC_MAP: {
            mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->data_mutex);
            res = _core_mqtt_remove_topic_map(mqtt_handle, (aiot_mqtt_topic_map_t *)data);
            mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->data_mutex);
        }
        break;
        case AIOT_MQTTOPT_APPEND_REQUESTID: {
            mqtt_handle->append_requestid = *(uint8_t *)data;
        }
        break;
        case AIOT_MQTTOPT_USERDATA: {
            mqtt_handle->userdata = data;
        }
        break;
        case AIOT_MQTTOPT_VERSION: {
            uint8_t version = *(uint8_t *)data;
            mqtt_handle->protocol_version = version;
        }
        break;
        case  AIOT_MQTTOPT_ASSIGNED_CLIENTID: {
            uint8_t use_assigned_clientid = *(uint8_t *)data;
            mqtt_handle->use_assigned_clientid = use_assigned_clientid;
        }
        break;
        case AIOT_MQTTOPT_FLOW_CONTROL_ENABLED: {
            mqtt_handle->flow_control_enabled = *(uint8_t *)data;
        }
        break;
        case AIOT_MQTTOPT_MAX_REPUB_NUM: {
            mqtt_handle->repub_list_limit = *(uint16_t *)data;
        }
        break;
        case AIOT_MQTTOPT_TOPIC_HEADER_CHECK: {
            if(*(uint8_t *)data == 1 || *(uint8_t *)data == 0) {
                mqtt_handle->topic_header_check = *(uint8_t *)data;
            } else {
                res = STATE_USER_INPUT_OUT_RANGE;
            }
        }
        break;
        
        default: {
            res = STATE_USER_INPUT_UNKNOWN_OPTION;
        }
    }
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->data_mutex);

    _core_mqtt_exec_dec(mqtt_handle);

    return res;
}

int32_t aiot_mqtt_deinit(void **handle)
{
    uint64_t deinit_timestart = 0;
    core_mqtt_handle_t *mqtt_handle = NULL;
    core_mqtt_event_t core_event;

    if (handle == NULL || *handle == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }

    mqtt_handle = *(core_mqtt_handle_t **)handle;

    if (mqtt_handle->exec_enabled == 0) {
        return STATE_USER_INPUT_EXEC_DISABLED;
    }

    mqtt_handle->exec_enabled = 0;
    deinit_timestart = mqtt_handle->sysdep->core_sysdep_time();
    do {
        if (mqtt_handle->exec_count == 0) {
            break;
        }
        mqtt_handle->sysdep->core_sysdep_sleep(CORE_MQTT_DEINIT_INTERVAL_MS);
    } while ((mqtt_handle->sysdep->core_sysdep_time() - deinit_timestart) < mqtt_handle->deinit_timeout_ms);

    if (mqtt_handle->exec_count != 0) {
        return STATE_MQTT_DEINIT_TIMEOUT;
    }

    _core_mqtt_topic_alias_list_remove_all(mqtt_handle);

    memset(&core_event, 0, sizeof(core_mqtt_event_t));
    core_event.type = CORE_MQTTEVT_DEINIT;
    _core_mqtt_process_data_process(mqtt_handle, &core_event);

    *handle = NULL;

    if (mqtt_handle->network_handle != NULL) {
        mqtt_handle->sysdep->core_sysdep_network_deinit(&mqtt_handle->network_handle);
    }
    if (mqtt_handle->host != NULL) {
        mqtt_handle->sysdep->core_sysdep_free(mqtt_handle->host);
    }
    if (mqtt_handle->product_key != NULL) {
        mqtt_handle->sysdep->core_sysdep_free(mqtt_handle->product_key);
    }
    if (mqtt_handle->device_name != NULL) {
        mqtt_handle->sysdep->core_sysdep_free(mqtt_handle->device_name);
    }
    if (mqtt_handle->device_secret != NULL) {
        mqtt_handle->sysdep->core_sysdep_free(mqtt_handle->device_secret);
    }
    if (mqtt_handle->username != NULL) {
        mqtt_handle->sysdep->core_sysdep_free(mqtt_handle->username);
    }
    if (mqtt_handle->password != NULL) {
        mqtt_handle->sysdep->core_sysdep_free(mqtt_handle->password);
    }
    if (mqtt_handle->clientid != NULL) {
        mqtt_handle->sysdep->core_sysdep_free(mqtt_handle->clientid);
    }
    if (mqtt_handle->extend_clientid != NULL) {
        mqtt_handle->sysdep->core_sysdep_free(mqtt_handle->extend_clientid);
    }
    if (mqtt_handle->security_mode != NULL) {
        mqtt_handle->sysdep->core_sysdep_free(mqtt_handle->security_mode);
    }
    if (mqtt_handle->cred != NULL) {
        mqtt_handle->sysdep->core_sysdep_free(mqtt_handle->cred);
    }
    if (mqtt_handle->conn_props != NULL) {
        aiot_mqtt_props_deinit(&mqtt_handle->conn_props);
    }

    mqtt_handle->sysdep->core_sysdep_mutex_deinit(&mqtt_handle->data_mutex);
    mqtt_handle->sysdep->core_sysdep_mutex_deinit(&mqtt_handle->send_mutex);
    mqtt_handle->sysdep->core_sysdep_mutex_deinit(&mqtt_handle->recv_mutex);
    mqtt_handle->sysdep->core_sysdep_mutex_deinit(&mqtt_handle->sub_mutex);
    mqtt_handle->sysdep->core_sysdep_mutex_deinit(&mqtt_handle->topic_alias_mutex);
    mqtt_handle->sysdep->core_sysdep_mutex_deinit(&mqtt_handle->pub_mutex);
    mqtt_handle->sysdep->core_sysdep_mutex_deinit(&mqtt_handle->process_handler_mutex);

    _core_mqtt_sublist_destroy(mqtt_handle);
    _core_mqtt_publist_destroy(mqtt_handle);
    _core_mqtt_process_datalist_destroy(mqtt_handle);

    core_global_deinit(mqtt_handle->sysdep);

    mqtt_handle->sysdep->core_sysdep_free(mqtt_handle);

    return STATE_SUCCESS;
}

static int32_t _mqtt_connect_with_prop(void *handle, will_message_t *will_message, mqtt_properties_t *connect_property)
{
    int32_t res = STATE_SUCCESS;
    core_mqtt_handle_t *mqtt_handle = (core_mqtt_handle_t *)handle;
    uint64_t time_ent_ms = 0;

    if (mqtt_handle == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }

    time_ent_ms = mqtt_handle->sysdep->core_sysdep_time();

    if (mqtt_handle->exec_enabled == 0) {
        return STATE_USER_INPUT_EXEC_DISABLED;
    }

    _core_mqtt_exec_inc(mqtt_handle);

    mqtt_handle->disconnect_api_called = 0;

    /* if network connection exist, close first */
    core_log(mqtt_handle->sysdep, STATE_MQTT_LOG_CONNECT, "MQTT user calls aiot_mqtt_connect api, connect\r\n");
    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->send_mutex);
    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->recv_mutex);

    res = _core_mqtt_connect(mqtt_handle, will_message, connect_property);

    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->recv_mutex);
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->send_mutex);

    if (res == STATE_MQTT_CONNECT_SUCCESS) {
        uint64_t time_ms = mqtt_handle->sysdep->core_sysdep_time();
        uint32_t time_delta = (uint32_t)(time_ms - time_ent_ms);

        core_log1(mqtt_handle->sysdep, STATE_MQTT_LOG_CONNECT, "MQTT connect success in %d ms\r\n", (void *)&time_delta);
        _core_mqtt_connect_event_notify(mqtt_handle);
        res = STATE_SUCCESS;
    } else {
        last_failed_error_code = res;
        _core_mqtt_disconnect_event_notify(mqtt_handle, AIOT_MQTTDISCONNEVT_NETWORK_DISCONNECT);
    }

    _core_mqtt_exec_dec(mqtt_handle);

    return res;
}

int32_t aiot_mqtt_connect(void *handle)
{
    return _mqtt_connect_with_prop(handle, NULL, NULL);
}

int32_t aiot_mqtt_disconnect(void *handle)
{
    int32_t res = STATE_SUCCESS;
    core_mqtt_handle_t *mqtt_handle = (core_mqtt_handle_t *)handle;

    if (mqtt_handle == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }
    if (mqtt_handle->exec_enabled == 0) {
        return STATE_USER_INPUT_EXEC_DISABLED;
    }

    _core_mqtt_exec_inc(mqtt_handle);

    mqtt_handle->disconnect_api_called = 1;

    /* send mqtt disconnect packet to mqtt broker first */
    _core_mqtt_disconnect(handle);

    /* close socket connect with mqtt broker */
    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->send_mutex);
    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->recv_mutex);
    if (mqtt_handle->network_handle != NULL) {
        mqtt_handle->sysdep->core_sysdep_network_deinit(&mqtt_handle->network_handle);
    }
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->recv_mutex);
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->send_mutex);

    _core_mqtt_disconnect_event_notify(mqtt_handle, AIOT_MQTTDISCONNEVT_NETWORK_DISCONNECT);
    core_log(mqtt_handle->sysdep, STATE_MQTT_LOG_DISCONNECT, "MQTT user calls aiot_mqtt_disconnect api, disconnect\r\n");

    _core_mqtt_exec_dec(mqtt_handle);

    return res;
}

int32_t aiot_mqtt_heartbeat(void *handle)
{
    core_mqtt_handle_t *mqtt_handle = (core_mqtt_handle_t *)handle;

    if (mqtt_handle == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }

    return _core_mqtt_heartbeat(mqtt_handle);
}

int32_t aiot_mqtt_process(void *handle)
{
    int32_t res = STATE_SUCCESS;
    uint64_t time_now = 0;
    core_mqtt_handle_t *mqtt_handle = (core_mqtt_handle_t *)handle;

    if (mqtt_handle == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }
    if (mqtt_handle->exec_enabled == 0) {
        return STATE_USER_INPUT_EXEC_DISABLED;
    }

    _core_mqtt_exec_inc(mqtt_handle);

    /* mqtt PINREQ packet */
    time_now = mqtt_handle->sysdep->core_sysdep_time();
    if (time_now < mqtt_handle->heartbeat_params.last_send_time) {
        mqtt_handle->heartbeat_params.last_send_time = time_now;
    }
    if ((time_now - mqtt_handle->heartbeat_params.last_send_time) >= mqtt_handle->heartbeat_params.interval_ms) {
        res = _core_mqtt_heartbeat(mqtt_handle);
        mqtt_handle->heartbeat_params.last_send_time = time_now;
        mqtt_handle->heartbeat_params.lost_times++;
    }

    /* mqtt QoS1 packet republish */
    _core_mqtt_repub(mqtt_handle);

    /* mqtt process handler process */
    _core_mqtt_process_data_process(mqtt_handle, NULL);

    _core_mqtt_exec_dec(mqtt_handle);

    return res;
}
static int32_t _core_mqtt_tx_topic_alias_process(core_mqtt_handle_t *mqtt_handle, core_mqtt_buff_t *topic,
        uint16_t *alias_id_prt)
{
    core_mqtt_topic_alias_node_t *topic_alias_node = NULL;
    uint16_t alias_id = 0;
    int32_t replace_topic_with_alias = 0;

    /* Topic Alias */
    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->topic_alias_mutex);
    core_list_for_each_entry(topic_alias_node, &mqtt_handle->tx_topic_alias_list, linked_node,
                             core_mqtt_topic_alias_node_t) {
        if (memcmp(topic_alias_node->topic, topic->buffer, topic->len) == 0) {
            core_log(mqtt_handle->sysdep, STATE_MQTT_LOG_CONNECT, "tx use topic alias\n");
            alias_id = topic_alias_node->topic_alias;
            replace_topic_with_alias = 1;
            break;
        }
    }
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->topic_alias_mutex);

    /* save the tx topic in tx_topic_alias_list */
    if (0 == replace_topic_with_alias) {
        alias_id = mqtt_handle->tx_topic_alias;
        alias_id++;
        /* make sure that it does not exceed tx_topic_alias_max */
        if (alias_id <= mqtt_handle->tx_topic_alias_max) {
            mqtt_handle->tx_topic_alias = alias_id;
            mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->topic_alias_mutex);
            _core_mqtt_topic_alias_list_insert(mqtt_handle, topic, alias_id, &(mqtt_handle->tx_topic_alias_list));
            mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->topic_alias_mutex);
        } else {
            /* exceed tx_topic_alias_max, no more alias could be used */
            alias_id = 0;
        }
    }
    *alias_id_prt = alias_id;
    return replace_topic_with_alias;
}

static int32_t _core_mqtt_check_flow_control(core_mqtt_handle_t *mqtt_handle, uint8_t qos)
{
    if (mqtt_handle->flow_control_enabled && qos == CORE_MQTT_QOS1) {
        uint16_t server_receive_max = 0;
        mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->pub_mutex);
        server_receive_max = mqtt_handle->server_receive_max;
        mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->pub_mutex);

        if (server_receive_max <= 0) {
            return STATE_MQTT_RECEIVE_MAX_EXCEEDED;
        }
    }
    return STATE_SUCCESS;
}

static void _core_mqtt_check_flow_dec(core_mqtt_handle_t *mqtt_handle)
{
    if (_core_mqtt_5_feature_is_enabled(mqtt_handle) && mqtt_handle->flow_control_enabled
        && mqtt_handle->server_receive_max > 0) {
        mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->pub_mutex);
        mqtt_handle->server_receive_max--;
        mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->pub_mutex);
    }
}

static int32_t _core_mqtt_check_tx_payload_len(uint32_t pkt_len, core_mqtt_handle_t *mqtt_handle)
{
    if (pkt_len >= mqtt_handle->tx_packet_max_size) {
        _core_mqtt_exec_dec(mqtt_handle);
        return STATE_MQTT_INVALID_TX_PACK_SIZE;
    }
    return STATE_SUCCESS;
}

static int32_t _core_mqtt_pub(void *handle, core_mqtt_buff_t *topic, core_mqtt_buff_t *payload, uint8_t qos, uint8_t retain,
                              mqtt_properties_t *pub_prop)
{
    int32_t res = STATE_SUCCESS;
    uint16_t packet_id = 0;
    uint8_t *pkt = NULL;
    uint32_t idx = 0, remainlen = 0, pkt_len = 0;
    core_mqtt_handle_t *mqtt_handle = (core_mqtt_handle_t *)handle;
    uint8_t replace_topic_with_alias = 0;
    uint32_t props_len = 0;
    uint16_t alias_id = 0;
    mqtt_properties_t *copy_props = NULL;
    mqtt_property_t alias_prop = {.id = MQTT_PROP_ID_TOPIC_ALIAS};

    if (mqtt_handle == NULL ||
        payload == NULL || payload->buffer == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }

    if (mqtt_handle->network_handle == NULL) {
        return STATE_SYS_DEPEND_NWK_CLOSED;
    }

    if (topic->len == 0 || qos > CORE_MQTT_QOS_MAX) {
        return STATE_USER_INPUT_OUT_RANGE;
    }

    if (_core_mqtt_5_feature_is_enabled(mqtt_handle)) {
        /* Flow Control Check*/
        res = _core_mqtt_check_flow_control(mqtt_handle, qos);
        if (res < STATE_SUCCESS) {
            return res;
        }

        if(pub_prop != NULL) {
            copy_props = aiot_mqtt_props_copy(pub_prop);
        } else {
            copy_props = aiot_mqtt_props_init();
        }

        if(copy_props == NULL) {
            return STATE_PORT_MALLOC_FAILED;
        }
        res = core_mqtt_props_bound(copy_props);
        if(res < STATE_SUCCESS){
            aiot_mqtt_props_deinit(&copy_props);
            return res;
        }

        /* Topic Alias*/
        if(qos == 0) {
            replace_topic_with_alias = _core_mqtt_tx_topic_alias_process(mqtt_handle, topic, &alias_id);
            if(alias_id > 0) {
                alias_prop.value.uint16 = alias_id;
                aiot_mqtt_props_add(copy_props, &alias_prop);
            }
        }
    }

    if (mqtt_handle->exec_enabled == 0) {
        return STATE_USER_INPUT_EXEC_DISABLED;
    }

    _core_mqtt_exec_inc(mqtt_handle);

    /* 计算packet总长度 */
    if (0 == replace_topic_with_alias) {
        remainlen = topic->len + payload->len + CORE_MQTT_UTF8_STR_EXTRA_LEN;
    } else {
        remainlen = payload->len + CORE_MQTT_UTF8_STR_EXTRA_LEN;
    }

    if (qos == CORE_MQTT_QOS1) {
        remainlen += CORE_MQTT_PACKETID_LEN;
    }

    if (_core_mqtt_5_feature_is_enabled(mqtt_handle)) {
        props_len = core_mqtt_props_bound(copy_props);
        remainlen += props_len;
    }
    pkt_len = CORE_MQTT_FIXED_HEADER_LEN + CORE_MQTT_REMAINLEN_MAXLEN + remainlen;

    /* 申请内存并对packet进行赋值 */
    pkt = mqtt_handle->sysdep->core_sysdep_malloc(pkt_len, CORE_MQTT_MODULE_NAME);
    if (pkt == NULL) {
        _core_mqtt_exec_dec(mqtt_handle);
        return STATE_SYS_DEPEND_MALLOC_FAILED;
    }
    memset(pkt, 0, pkt_len);

    /* Publish Packet Type */
    pkt[idx++] = CORE_MQTT_PUBLISH_PKT_TYPE | (qos << 1) | retain;

    /* Remaining Length */
    idx += _write_variable(remainlen, &pkt[idx]);

    /* Topic Length */
    if (replace_topic_with_alias == 0) {
        /* topic len is > 0 when alias DOES NOT replaces topic */
        _core_mqtt_set_utf8_encoded_str((uint8_t *)topic->buffer, topic->len, &pkt[idx]);
        idx += CORE_MQTT_UTF8_STR_EXTRA_LEN + topic->len;
    } else {
        /* topic len is 0 when alias replaces topic */
        pkt[idx++] = 0;
        pkt[idx++] = 0;
    }

    /* Packet Id For QOS 1*/
    if (qos == CORE_MQTT_QOS1) {
        packet_id = _core_mqtt_packet_id(handle);
        pkt[idx++] = (uint8_t)((packet_id >> 8) & 0x00FF);
        pkt[idx++] = (uint8_t)((packet_id) & 0x00FF);
    }

    if (_core_mqtt_5_feature_is_enabled(mqtt_handle) && copy_props != NULL) {
        res = core_mqtt_props_write(copy_props, &pkt[idx], props_len);
        idx += res;
        aiot_mqtt_props_deinit(&copy_props);
    }

    /* Payload */
    memcpy(&pkt[idx], payload->buffer, payload->len);
    idx += payload->len;

    pkt_len = idx;

    /* Ensure tx pack size does not overflow  */
    res = _core_mqtt_check_tx_payload_len(pkt_len, mqtt_handle);
    if (res < STATE_SUCCESS) {
        mqtt_handle->sysdep->core_sysdep_free(pkt);
        return res;
    }

    if (qos == CORE_MQTT_QOS1) {
        mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->pub_mutex);
        res = _core_mqtt_publist_insert(mqtt_handle, pkt, pkt_len, packet_id);
        mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->pub_mutex);
        if (res < STATE_SUCCESS) {
            mqtt_handle->sysdep->core_sysdep_free(pkt);
            _core_mqtt_exec_dec(mqtt_handle);
            return res;
        }
    }

    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->send_mutex);
    res = _core_mqtt_write(handle, pkt, pkt_len, mqtt_handle->send_timeout_ms);
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->send_mutex);
    if (res < STATE_SUCCESS) {
        mqtt_handle->sysdep->core_sysdep_free(pkt);
        if (res != STATE_SYS_DEPEND_NWK_WRITE_LESSDATA) {
            mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->send_mutex);
            mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->recv_mutex);
            if (mqtt_handle->network_handle != NULL) {
                mqtt_handle->sysdep->core_sysdep_network_deinit(&mqtt_handle->network_handle);
            }
            mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->recv_mutex);
            mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->send_mutex);
        }
        _core_mqtt_exec_dec(mqtt_handle);
        return res;
    }

    mqtt_handle->sysdep->core_sysdep_free(pkt);

    if (qos == CORE_MQTT_QOS1) {
        _core_mqtt_exec_dec(mqtt_handle);
        _core_mqtt_check_flow_dec(mqtt_handle);
        return (int32_t)packet_id;
    }

    _core_mqtt_exec_dec(mqtt_handle);

    return STATE_SUCCESS;
}

int32_t _core_mqtt_disconnect_with_prop(void *handle,  uint8_t reason_code, mqtt_properties_t *discon_property)
{
    int32_t res = 0;
    uint32_t pkt_len = 0;
    uint8_t *pkt = NULL;
    uint32_t idx = 0;
    core_mqtt_handle_t *mqtt_handle = (core_mqtt_handle_t *)handle;
    uint32_t props_len = 0;
    uint32_t pkt_len_offset = 0;
    uint8_t  pkt_len_array[4] = {0};
    uint32_t pkt_remain_len = 0;

    if (_core_mqtt_5_feature_is_enabled(mqtt_handle)) {
        res = core_mqtt_props_bound(discon_property);
        if(res < STATE_SUCCESS) {
            return res;
        }
        props_len = res;
    }

    pkt_remain_len = props_len + CORE_MQTT_V5_DISCONNECT_REASON_CODE_LEN;
    pkt_len_offset = _write_variable(pkt_remain_len, pkt_len_array);

    pkt_len =  CORE_MQTT_FIXED_HEADER_LEN + pkt_len_offset + pkt_remain_len ;
    pkt = mqtt_handle->sysdep->core_sysdep_malloc(pkt_len, CORE_MQTT_MODULE_NAME);
    if (NULL == pkt) {
        return STATE_PORT_MALLOC_FAILED;
    }
    memset(pkt, 0, pkt_len);

    pkt[idx++] = CORE_MQTT_DISCONNECT_PKT_TYPE;
    memcpy(&pkt[idx], pkt_len_array, pkt_len_offset);
    idx += pkt_len_offset;
    pkt[idx++] = reason_code;
    idx += core_mqtt_props_write(discon_property, &pkt[idx], pkt_len - idx);
    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->send_mutex);
    res = _core_mqtt_write(handle, pkt, idx, mqtt_handle->send_timeout_ms);
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->send_mutex);
    mqtt_handle->sysdep->core_sysdep_free(pkt);

    return res;
}

int32_t aiot_mqtt_disconnect_v5(void *handle, uint8_t reason_code, mqtt_properties_t *discon_property)
{
    int32_t res = STATE_SUCCESS;
    core_mqtt_handle_t *mqtt_handle = (core_mqtt_handle_t *)handle;

    if (mqtt_handle == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }

    if (0 == (_core_mqtt_5_feature_is_enabled(mqtt_handle))) {
        return STATE_MQTT_INVALID_PROTOCOL_VERSION;
    }

    if (mqtt_handle->exec_enabled == 0) {
        return STATE_USER_INPUT_EXEC_DISABLED;
    }

    _core_mqtt_exec_inc(mqtt_handle);

    mqtt_handle->disconnect_api_called = 1;

    /* send mqtt disconnect packet to mqtt broker first */
    res = _core_mqtt_disconnect_with_prop(handle, reason_code, discon_property);
    if (res <= STATE_SUCCESS) {
        _core_mqtt_exec_dec(mqtt_handle);
        return res;
    }
    _core_mqtt_topic_alias_list_remove_all(mqtt_handle);

    /* close socket connect with mqtt broker */
    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->send_mutex);
    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->recv_mutex);
    if (mqtt_handle->network_handle != NULL) {
        mqtt_handle->sysdep->core_sysdep_network_deinit(&mqtt_handle->network_handle);
    }
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->recv_mutex);
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->send_mutex);

    _core_mqtt_disconnect_event_notify(mqtt_handle, AIOT_MQTTDISCONNEVT_NETWORK_DISCONNECT);
    core_log(mqtt_handle->sysdep, STATE_MQTT_LOG_DISCONNECT, "MQTT user calls aiot_mqtt_disconnect api, disconnect\r\n");

    _core_mqtt_exec_dec(mqtt_handle);
    if (res > STATE_SUCCESS) {
        res = STATE_SUCCESS;
    }

    return res;
}

static int32_t _core_mqtt_pub_params_check(void *handle, char *topic, uint8_t *payload, uint32_t payload_len, uint8_t qos, uint8_t retain)
{
    core_mqtt_handle_t *mqtt_handle = (core_mqtt_handle_t *)handle;
    if (NULL == topic || NULL == handle || payload == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }

    if (strlen(topic) >= CORE_MQTT_TOPIC_MAXLEN) {
        return STATE_MQTT_TOPIC_TOO_LONG;
    }

    if(strlen(topic) == 0 || qos > CORE_MQTT_QOS_MAX || retain > 1) {
        return STATE_USER_INPUT_OUT_RANGE;
    }

    if (payload_len >= CORE_MQTT_PAYLOAD_MAXLEN) {
        return STATE_MQTT_PUB_PAYLOAD_TOO_LONG;
    }

    if (mqtt_handle->network_handle == NULL) {
        return STATE_SYS_DEPEND_NWK_CLOSED;
    }

    if (mqtt_handle->exec_enabled == 0) {
        return STATE_USER_INPUT_EXEC_DISABLED;
    }

    return STATE_SUCCESS;
}

int32_t _core_mqtt_append_rid(core_mqtt_handle_t *mqtt_handle, char *topic, char **new_topic)
{
    char *rid_prefix = "?_rid=";
    uint64_t timestamp = core_log_get_timestamp(mqtt_handle->sysdep);
    uint32_t rand = 0;
    char *buffer = NULL;
    uint32_t buffer_len = strlen(topic) + strlen(rid_prefix) + 32;
 
    buffer = mqtt_handle->sysdep->core_sysdep_malloc(buffer_len, CORE_MQTT_MODULE_NAME);
    if (NULL == buffer) {
        return STATE_PORT_MALLOC_FAILED;
    }
    memset(buffer, 0, buffer_len);
 
    memcpy(buffer, topic, strlen(topic));
    memcpy(buffer + strlen(buffer), rid_prefix, strlen(rid_prefix));
    core_uint642str(timestamp, buffer + strlen(buffer), NULL);
    mqtt_handle->sysdep->core_sysdep_rand((uint8_t *)&rand, sizeof(rand));
    core_uint2str(rand, buffer + strlen(buffer), NULL);
 
    *new_topic = buffer;
 
    return STATE_SUCCESS;
}

int32_t _core_mqtt_compress(core_mqtt_handle_t *mqtt_handle, char *topic, uint8_t *payload, uint32_t payload_len, core_mqtt_msg_t *msg)
{
    core_mqtt_msg_t dest_msg;
    int32_t res = STATE_SUCCESS;
    if(mqtt_handle == NULL || topic == NULL || msg == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }

    msg->topic = topic;
    msg->topic_len = strlen(topic);
    msg->payload = payload;
    msg->payload_len = payload_len;

    res = mqtt_handle->compress.handler(mqtt_handle->compress.context, msg, &dest_msg);
    if(res >= STATE_SUCCESS) {
        /* copy new msg */
        memcpy(msg, &dest_msg, sizeof(*msg));
    }

    return res;
}

int32_t _mqtt_pub_with_prop(void *handle, char *topic, uint8_t *payload, uint32_t payload_len, uint8_t qos, uint8_t retain, mqtt_properties_t *pub_prop)
{
    core_mqtt_handle_t *mqtt_handle = (core_mqtt_handle_t *)handle;
    core_mqtt_buff_t    topic_buff = {0};
    core_mqtt_buff_t    payload_buff;
    core_mqtt_msg_t     msg;
    char *new_topic = NULL;
    int32_t res = STATE_SUCCESS, compress_res = STATE_SUCCESS;

    res = _core_mqtt_pub_params_check(handle, topic, payload, payload_len, qos, retain);
    if(res != STATE_SUCCESS) {
        return res;
    }

    core_log1(mqtt_handle->sysdep, STATE_MQTT_LOG_TOPIC, "pub: %s\r\n", topic);
    core_log_hexdump(STATE_MQTT_LOG_HEXDUMP, '>', payload, payload_len);

        /* append rid */
    if (1 == mqtt_handle->append_requestid) {
        _core_mqtt_append_rid(mqtt_handle, topic, &new_topic);
        topic = new_topic;
    }

    memset(&topic_buff, 0, sizeof(topic_buff));
    topic_buff.buffer = (uint8_t *)topic;
    topic_buff.len = strlen(topic);

    memset(&payload_buff, 0, sizeof(payload_buff));
    payload_buff.buffer = payload;
    payload_buff.len = payload_len;

    if(mqtt_handle->compress.handler != NULL) {
        memset(&msg, 0, sizeof(msg));
        compress_res = _core_mqtt_compress(mqtt_handle, topic, payload, payload_len, &msg);
        if(compress_res < STATE_SUCCESS) {
            core_log(mqtt_handle->sysdep, STATE_MQTT_BASE, "compress error \r\n");
            return compress_res;
        }
        payload_buff.buffer = msg.payload;
        payload_buff.len = msg.payload_len;
    }
    res = _core_mqtt_pub(handle, &topic_buff, &payload_buff, qos, retain, pub_prop);

    /* 如append rid生成新的topic，资源回收 */
    if (new_topic != NULL) {
        mqtt_handle->sysdep->core_sysdep_free(new_topic);
    }
    /* 如compress中成功执行压缩，需要释放新的payload资源 */
    if (compress_res == STATE_COMPRESS_SUCCESS && msg.payload != NULL) {
        mqtt_handle->sysdep->core_sysdep_free(msg.payload);
    }
    return res;
}

int32_t aiot_mqtt_pub(void *handle, char *topic, uint8_t *payload, uint32_t payload_len, uint8_t qos)
{
    return  _mqtt_pub_with_prop(handle, topic, payload, payload_len, qos, 0, NULL);
}

static int32_t _core_mqtt_sub(void *handle, core_mqtt_buff_t *topic, aiot_mqtt_recv_handler_t handler,
                              sub_options_t *opts,
                              void *userdata, mqtt_properties_t *sub_prop)
{
    int32_t res = STATE_SUCCESS;
    core_mqtt_handle_t *mqtt_handle = (core_mqtt_handle_t *)handle;

    if (mqtt_handle == NULL || topic == NULL || topic->buffer == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }
    if (topic->len == 0 || (opts != NULL && opts->qos > CORE_MQTT_QOS_MAX)) {
        return STATE_USER_INPUT_OUT_RANGE;
    }
    if ((res = _core_mqtt_topic_is_valid(mqtt_handle, (char *)topic->buffer, topic->len)) < STATE_SUCCESS) {
        return STATE_MQTT_TOPIC_INVALID;
    }
    if (mqtt_handle->exec_enabled == 0) {
        return STATE_USER_INPUT_EXEC_DISABLED;
    }

    _core_mqtt_exec_inc(mqtt_handle);

    core_log2(mqtt_handle->sysdep, STATE_MQTT_LOG_TOPIC, "sub: %.*s\r\n", &topic->len, topic->buffer);

    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->sub_mutex);
    res = _core_mqtt_sublist_insert(mqtt_handle, topic, handler, userdata);
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->sub_mutex);

    if (res < STATE_SUCCESS) {
        return res;
    }

    /* send subscribe packet */
    res = _core_mqtt_subunsub(mqtt_handle, (char *)topic->buffer, topic->len, opts, CORE_MQTT_SUB_PKT_TYPE,
                              sub_prop);

    _core_mqtt_exec_dec(mqtt_handle);

    return res;
}

static int32_t _mqtt_sub_with_prop(void *handle, char *topic, aiot_mqtt_recv_handler_t handler, sub_options_t *opts,
                                   void *userdata, mqtt_properties_t *sub_prop)
{
    core_mqtt_buff_t    topic_buff;

    if (NULL == topic) {
        return STATE_USER_INPUT_NULL_POINTER;
    }

    if (strlen(topic) >= CORE_MQTT_TOPIC_MAXLEN) {
        return STATE_MQTT_TOPIC_TOO_LONG;
    }
    if(opts != NULL) {
        if(opts->qos > 1 || opts->no_local > 1 || opts->retain_as_publish > 1 || opts->retain_handling > 2) {
            return STATE_PORT_INPUT_OUT_RANGE;
        }
    }

    memset(&topic_buff, 0, sizeof(topic_buff));

    topic_buff.buffer = (uint8_t *)topic;
    topic_buff.len = strlen(topic);

    return _core_mqtt_sub(handle, &topic_buff, handler, opts, userdata, sub_prop);
}

int32_t aiot_mqtt_sub(void *handle, char *topic, aiot_mqtt_recv_handler_t handler, uint8_t qos, void *userdata)
{
    sub_options_t opts;
    memset(&opts, 0, sizeof(opts));
    opts.qos = qos;
    return _mqtt_sub_with_prop(handle, topic, handler, &opts, userdata, NULL);
}

static int32_t _core_mqtt_unsub(void *handle, core_mqtt_buff_t *topic, mqtt_properties_t *unsub_prop)
{
    int32_t res = STATE_SUCCESS;
    core_mqtt_handle_t *mqtt_handle = (core_mqtt_handle_t *)handle;

    if (mqtt_handle == NULL || topic == NULL || topic->buffer == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }
    if (topic->len == 0) {
        return STATE_USER_INPUT_OUT_RANGE;
    }
    if ((res = _core_mqtt_topic_is_valid(mqtt_handle, (char *)topic->buffer, topic->len)) < STATE_SUCCESS) {
        return STATE_MQTT_TOPIC_INVALID;
    }
    if (mqtt_handle->exec_enabled == 0) {
        return STATE_USER_INPUT_EXEC_DISABLED;
    }

    _core_mqtt_exec_inc(mqtt_handle);

    core_log2(mqtt_handle->sysdep, STATE_MQTT_LOG_TOPIC, "unsub: %.*s\r\n", &topic->len, topic->buffer);

    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->sub_mutex);
    _core_mqtt_sublist_remove(mqtt_handle, topic);
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->sub_mutex);

    res = _core_mqtt_subunsub(mqtt_handle, (char *)topic->buffer, topic->len, NULL, CORE_MQTT_UNSUB_PKT_TYPE,
                              unsub_prop);

    _core_mqtt_exec_dec(mqtt_handle);

    return res;
}

static int32_t _mqtt_unsub(void *handle, char *topic, mqtt_properties_t *unsub_prop)
{
    core_mqtt_buff_t    topic_buff;

    if (NULL == topic) {
        return STATE_USER_INPUT_NULL_POINTER;
    }

    if (strlen(topic) >= CORE_MQTT_TOPIC_MAXLEN) {
        return STATE_MQTT_TOPIC_TOO_LONG;
    }

    memset(&topic_buff, 0, sizeof(topic_buff));

    topic_buff.buffer = (uint8_t *)topic;
    topic_buff.len = strlen(topic);

    return _core_mqtt_unsub(handle, &topic_buff, unsub_prop);
}


int32_t aiot_mqtt_unsub(void *handle, char *topic)
{
    return _mqtt_unsub(handle, topic, NULL);
}

int32_t aiot_mqtt_unsub_v5(void *handle, char *topic, mqtt_properties_t *unsub_prop)
{
    core_mqtt_handle_t *mqtt_handle = (core_mqtt_handle_t *)handle;

    if (mqtt_handle == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }

    if (0 == _core_mqtt_5_feature_is_enabled(mqtt_handle)) {
        return STATE_MQTT_INVALID_PROTOCOL_VERSION;
    }

    return _mqtt_unsub(handle, topic, unsub_prop);
}

int32_t aiot_mqtt_recv(void *handle)
{
    int32_t res = STATE_SUCCESS;
    uint32_t mqtt_remainlen = 0;
    uint8_t mqtt_fixed_header[CORE_MQTT_FIXED_HEADER_LEN] = {0};
    uint8_t mqtt_pkt_type = 0, mqtt_pkt_reserved = 0;
    uint8_t *remain = NULL;
    core_mqtt_handle_t *mqtt_handle = (core_mqtt_handle_t *)handle;

    if (mqtt_handle == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }
    if (mqtt_handle->exec_enabled == 0) {
        return STATE_USER_INPUT_EXEC_DISABLED;
    }

    _core_mqtt_exec_inc(mqtt_handle);

    /* network error reconnect */
    if (mqtt_handle->network_handle == NULL) {
        _core_mqtt_disconnect_event_notify(mqtt_handle, AIOT_MQTTDISCONNEVT_NETWORK_DISCONNECT);
    }
    if (mqtt_handle->reconnect_params.enabled == 1 && mqtt_handle->disconnect_api_called == 0) {
        res = _core_mqtt_reconnect(mqtt_handle);
        if (res < STATE_SUCCESS) {
            if (res == STATE_MQTT_CONNECT_SUCCESS) {
                mqtt_handle->heartbeat_params.lost_times = 0;
                core_log(mqtt_handle->sysdep, STATE_MQTT_LOG_CONNECT, "MQTT network reconnect success\r\n");
                _core_mqtt_connect_event_notify(mqtt_handle);
                res = STATE_SUCCESS;
            } else {
                _core_mqtt_exec_dec(mqtt_handle);
                return res;
            }
        }
    }

    /* heartbeat missing reconnect */
    if (mqtt_handle->heartbeat_params.lost_times > mqtt_handle->heartbeat_params.max_lost_times) {
        _core_mqtt_disconnect_event_notify(mqtt_handle, AIOT_MQTTDISCONNEVT_HEARTBEAT_DISCONNECT);
        if (mqtt_handle->reconnect_params.enabled == 1 && mqtt_handle->disconnect_api_called == 0) {
            core_log1(mqtt_handle->sysdep, STATE_MQTT_LOG_RECONNECTING, "MQTT heartbeat lost %d times, try to reconnecting...\r\n",
                      &mqtt_handle->heartbeat_params.lost_times);
            mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->send_mutex);
            mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->recv_mutex);
            res = _core_mqtt_connect(mqtt_handle, NULL, NULL);
            mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->recv_mutex);
            mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->send_mutex);
            if (res < STATE_SUCCESS) {
                if (res == STATE_MQTT_CONNECT_SUCCESS) {
                    mqtt_handle->heartbeat_params.lost_times = 0;
                    core_log(mqtt_handle->sysdep, STATE_MQTT_LOG_CONNECT, "MQTT network reconnect success\r\n");
                    _core_mqtt_connect_event_notify(mqtt_handle);
                    res = STATE_SUCCESS;
                } else {
                    _core_mqtt_exec_dec(mqtt_handle);
                    return res;
                }
            }
        }
    }

    /* Read Packet Type Of MQTT Fixed Header, Get  And Remaining Packet Length */
    mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->recv_mutex);
    res = _core_mqtt_read(handle, mqtt_fixed_header, CORE_MQTT_FIXED_HEADER_LEN, mqtt_handle->recv_timeout_ms);
    if (res < STATE_SUCCESS) {
        mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->recv_mutex);
        _core_mqtt_exec_dec(mqtt_handle);
        if (res == STATE_SYS_DEPEND_NWK_READ_LESSDATA) {
            res = STATE_SUCCESS;
        } else {
            mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->send_mutex);
            mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->recv_mutex);
            if (mqtt_handle->network_handle != NULL) {
                mqtt_handle->sysdep->core_sysdep_network_deinit(&mqtt_handle->network_handle);
            }
            mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->recv_mutex);
            mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->send_mutex);
        }
        return res;
    }

    /* Read Remaining Packet Length Of MQTT Fixed Header */
    res = _core_mqtt_read_remainlen(mqtt_handle, &mqtt_remainlen);
    if (res < STATE_SUCCESS) {
        mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->recv_mutex);
        mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->send_mutex);
        mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->recv_mutex);
        if (mqtt_handle->network_handle != NULL) {
            mqtt_handle->sysdep->core_sysdep_network_deinit(&mqtt_handle->network_handle);
        }
        mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->recv_mutex);
        mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->send_mutex);
        _core_mqtt_exec_dec(mqtt_handle);
        return res;
    }

    /* Read Remaining Bytes */
    res = _core_mqtt_read_remainbytes(handle, mqtt_remainlen, &remain);

    if (res < STATE_SUCCESS) {
        mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->recv_mutex);
        mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->send_mutex);
        mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->recv_mutex);
        if (mqtt_handle->network_handle != NULL) {
            mqtt_handle->sysdep->core_sysdep_network_deinit(&mqtt_handle->network_handle);
        }
        mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->recv_mutex);
        mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->send_mutex);
        _core_mqtt_exec_dec(mqtt_handle);
        return res;
    }
    mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->recv_mutex);

    /* Get Packet Type */
    mqtt_pkt_type = mqtt_fixed_header[0] & 0xF0;
    mqtt_pkt_reserved = mqtt_fixed_header[0] & 0x0F;

    /* reset ping response missing times */
    mqtt_handle->heartbeat_params.lost_times = 0;

    switch (mqtt_pkt_type) {
        case CORE_MQTT_PINGRESP_PKT_TYPE: {
            res = _core_mqtt_pingresp_handler(mqtt_handle, remain, mqtt_remainlen);
        }
        break;
        case CORE_MQTT_PUBLISH_PKT_TYPE: {
            res = _core_mqtt_pub_handler(handle, remain, mqtt_remainlen, ((mqtt_pkt_reserved >> 1) & 0x03), mqtt_remainlen);
        }
        break;
        case CORE_MQTT_PUBACK_PKT_TYPE: {
            res = _core_mqtt_puback_handler(handle, remain, mqtt_remainlen);
        }
        break;
        case CORE_MQTT_SUBACK_PKT_TYPE: {
            _core_mqtt_subunsuback_handler(handle, remain, mqtt_remainlen, CORE_MQTT_SUBACK_PKT_TYPE);
        }
        break;
        case CORE_MQTT_UNSUBACK_PKT_TYPE: {
            _core_mqtt_subunsuback_handler(handle, remain, mqtt_remainlen, CORE_MQTT_UNSUBACK_PKT_TYPE);
        }
        break;
        case CORE_MQTT_SERVER_DISCONNECT_PKT_TYPE: {
            _core_mqtt_server_disconnect_handler(handle, remain, mqtt_remainlen);
        }
        break;

        case CORE_MQTT_PUBREC_PKT_TYPE:
        case CORE_MQTT_PUBREL_PKT_TYPE:
        case CORE_MQTT_PUBCOMP_PKT_TYPE: {

        }
        break;
        default: {
            res = STATE_MQTT_PACKET_TYPE_UNKNOWN;
        }
    }

    if (remain) {
        mqtt_handle->sysdep->core_sysdep_free(remain);
    }

    if (res < STATE_SUCCESS) {
        mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->send_mutex);
        mqtt_handle->sysdep->core_sysdep_mutex_lock(mqtt_handle->recv_mutex);
        if (mqtt_handle->network_handle != NULL) {
            mqtt_handle->sysdep->core_sysdep_network_deinit(&mqtt_handle->network_handle);
        }
        mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->recv_mutex);
        mqtt_handle->sysdep->core_sysdep_mutex_unlock(mqtt_handle->send_mutex);
    }

    _core_mqtt_exec_dec(mqtt_handle);

    return res;
}

char *core_mqtt_get_product_key(void *handle)
{
    core_mqtt_handle_t *mqtt_handle = (core_mqtt_handle_t *)handle;

    if (handle == NULL) {
        return NULL;
    }

    return mqtt_handle->product_key;
}

char *core_mqtt_get_device_name(void *handle)
{
    core_mqtt_handle_t *mqtt_handle = (core_mqtt_handle_t *)handle;

    if (handle == NULL) {
        return NULL;
    }

    return mqtt_handle->device_name;
}

uint16_t core_mqtt_get_port(void *handle)
{
    core_mqtt_handle_t *mqtt_handle = (core_mqtt_handle_t *)handle;

    if (handle == NULL) {
        return 0;
    }

    return mqtt_handle->port;
}

int32_t core_mqtt_get_nwkstats(void *handle, core_mqtt_nwkstats_info_t *nwk_stats_info)
{
    core_mqtt_handle_t *mqtt_handle = (core_mqtt_handle_t *)handle;
    if (handle == NULL) {
        return 0;
    }

    memcpy(nwk_stats_info, &mqtt_handle->nwkstats_info, sizeof(core_mqtt_nwkstats_info_t));
    mqtt_handle->nwkstats_info.failed_error_code = 0;
    mqtt_handle->nwkstats_info.connect_time_used = 0;
    return STATE_SUCCESS;
}

int32_t aiot_mqtt_pub_v5(void *handle, char *topic, uint8_t *payload, uint32_t payload_len, uint8_t qos,
                                uint8_t retain, mqtt_properties_t *pub_prop)
{
    core_mqtt_handle_t *mqtt_handle = (core_mqtt_handle_t *)handle;

    if (NULL == handle) {
        return STATE_USER_INPUT_NULL_POINTER;
    }
    if (0 == (_core_mqtt_5_feature_is_enabled(mqtt_handle))) {
        return STATE_MQTT_INVALID_PROTOCOL_VERSION;
    }
    return  _mqtt_pub_with_prop(handle, topic, payload, payload_len, qos, retain, pub_prop);
}

int32_t aiot_mqtt_sub_v5(void *handle, char *topic, sub_options_t *opts, aiot_mqtt_recv_handler_t handler,
                                void *userdata, mqtt_properties_t *sub_prop)
{
    core_mqtt_handle_t *mqtt_handle = (core_mqtt_handle_t *)handle;

    if (mqtt_handle == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }

    if (0 == _core_mqtt_5_feature_is_enabled(mqtt_handle)) {
        return STATE_MQTT_INVALID_PROTOCOL_VERSION;
    }

    return _mqtt_sub_with_prop(mqtt_handle, topic, handler, opts, userdata, sub_prop);
}

int32_t _core_will_message_check(will_message_t *will_message)
{
    if(will_message == NULL) {
        return STATE_SUCCESS;
    }
    if (NULL == will_message->topic || will_message->payload == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }

    if (strlen(will_message->topic) >= CORE_MQTT_TOPIC_MAXLEN) {
        return STATE_MQTT_TOPIC_TOO_LONG;
    }

    if(strlen(will_message->topic) == 0 || will_message->qos > CORE_MQTT_QOS_MAX
        || will_message->retain > 1) {
        return STATE_USER_INPUT_OUT_RANGE;
    }

    if (will_message->payload_len >= 65536) {
        return STATE_MQTT_PUB_PAYLOAD_TOO_LONG;
    }

    return STATE_SUCCESS;
}

int32_t aiot_mqtt_connect_v5(void *handle, will_message_t *will_message, mqtt_properties_t *conn_prop)
{
    core_mqtt_handle_t *mqtt_handle = (core_mqtt_handle_t *)handle;
    int32_t res = STATE_SUCCESS;

    if (mqtt_handle == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }
    if (0 == _core_mqtt_5_feature_is_enabled(mqtt_handle)) {
        return STATE_MQTT_INVALID_PROTOCOL_VERSION;
    }
    res = _core_will_message_check(will_message);
    if(res != STATE_SUCCESS){
        return res;
    }

    /* clear previous connection properties */
    if (NULL != mqtt_handle->conn_props) {
        aiot_mqtt_props_deinit(&mqtt_handle->conn_props);
    }
    return _mqtt_connect_with_prop(handle, will_message, conn_prop);
}

