/**
 * @file aiot_mqtt_props_api.c
 * @brief mqtt_prop模块的API接口实现, 提供mqtt 5.0属性管理的能力
 *
 * @copyright Copyright (C) 2015-2020 Alibaba Group Holding Limited
 *
 */
#include <stdio.h>
#include "aiot_sysdep_api.h"
#include "aiot_state_api.h"
#include "aiot_mqtt_props_api.h"
#include "core_mqtt.h"

static mqtt_property_type_t get_type(mqtt_property_identify_t id)
{
    switch(id){
        /* Byte */
        case MQTT_PROP_ID_PAYLOAD_FORMAT_INDICATOR:
        case MQTT_PROP_ID_REQUEST_PROBLEM_INFORMATION:
        case MQTT_PROP_ID_REQUEST_RESPONSE_INFORMATION:
        case MQTT_PROP_ID_MAXIMUM_QOS:
        case MQTT_PROP_ID_RETAIN_AVAILABLE:
        case MQTT_PROP_ID_WILDCARD_SUBSCRIPTION_AVAILABLE:
        case MQTT_PROP_ID_SUBSCRIPTION_IDENTIFIERS_AVAILABLE:
        case MQTT_PROP_ID_SHARED_SUBSCRIPTION_AVAILABLE:
            return MQTT_PROP_TYPE_UINT8;

        /* uint16 */
        case MQTT_PROP_ID_SERVER_KEEP_ALIVE:
        case MQTT_PROP_ID_RECEIVE_MAXIMUM:
        case MQTT_PROP_ID_TOPIC_ALIAS_MAXIMUM:
        case MQTT_PROP_ID_TOPIC_ALIAS:
            return MQTT_PROP_TYPE_UINT16;

        /* uint32 */
        case MQTT_PROP_ID_MESSAGE_EXPIRY_INTERVAL:
        case MQTT_PROP_ID_WILL_DELAY_INTERVAL:
        case MQTT_PROP_ID_MAXIMUM_PACKET_SIZE:
        case MQTT_PROP_ID_SESSION_EXPIRY_INTERVAL:
            return MQTT_PROP_TYPE_UINT32;

        /* varint */
        case MQTT_PROP_ID_SUBSCRIPTION_IDENTIFIER:
            return MQTT_PROP_TYPE_VARIABLE;

        /* binary */
        case MQTT_PROP_ID_CORRELATION_DATA:
        case MQTT_PROP_ID_AUTHENTICATION_DATA:
            return MQTT_PROP_TYPE_BINARY;
        /* string */
        case MQTT_PROP_ID_CONTENT_TYPE:
        case MQTT_PROP_ID_RESPONSE_TOPIC:
        case MQTT_PROP_ID_ASSIGNED_CLIENT_IDENTIFER:
        case MQTT_PROP_ID_AUTHENTICATION_METHOD:
        case MQTT_PROP_ID_RESPONSE_INFORMATION:
        case MQTT_PROP_ID_SERVER_REFERENCE:
        case MQTT_PROP_ID_REASON_STRING:
            return MQTT_PROP_TYPE_STRING;

        /* string pair */
        case MQTT_PROP_ID_USER_PROPERTY:
            return MQTT_PROP_TYPE_STRING_PAIR;

        default:
            return MQTT_PROP_ID_INVALID;
    }
    return 0;
}

int32_t prop_check(mqtt_property_t *prop)
{
    switch(prop->id){
        case MQTT_PROP_ID_RECEIVE_MAXIMUM:
        if(prop->value.uint16 == 0) {
            return -1;
        }
        break;
        case MQTT_PROP_ID_SUBSCRIPTION_IDENTIFIER:
        if(prop->value.uint32 == 0) {
            return STATE_MQTT_INVALID_SUBSCRIPTION_IDENTIFIER;
        }
        break;
        case MQTT_PROP_ID_USER_PROPERTY:
        if(prop->value.str_pair.key.len > CORE_MQTT_USER_PROPERTY_KEY_MAX_LEN
            || prop->value.str_pair.value.len > CORE_MQTT_USER_PROPERTY_VALUE_MAX_LEN) {
            return STATE_MQTT_INVALID_USER_PERPERTY_LEN;
        }
        break;
        default:
            return STATE_SUCCESS;
    }
    return STATE_SUCCESS;
}

mqtt_properties_t *aiot_mqtt_props_init()
{
    mqtt_properties_t *props = NULL;
    aiot_sysdep_portfile_t *sysdep = NULL;

    sysdep = aiot_sysdep_get_portfile();
    if (sysdep == NULL) {
        return NULL;
    }

    props = sysdep->core_sysdep_malloc(sizeof(mqtt_properties_t), CORE_MQTT_MODULE_NAME);
    if (props == NULL) {
        core_global_deinit(sysdep);
        return NULL;
    }

    memset(props, 0, sizeof(mqtt_properties_t));
    props->prop_num = 0;
    props->prop_array_size = 0;
    props->prop_array = NULL;

    return props;
}

int32_t aiot_mqtt_props_add(mqtt_properties_t *props, mqtt_property_t *prop)
{
    mqtt_property_t *prop_array = NULL;
    aiot_sysdep_portfile_t *sysdep = NULL;
    mqtt_property_type_t type;
    if(props == NULL || prop == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }

    sysdep = aiot_sysdep_get_portfile();
    if (sysdep == NULL) {
        return STATE_SYS_DEPEND_BASE;
    }
    /* 查看是否需要重新内存内存 */
    if(props->prop_num + 1 > props->prop_array_size) {
        props->prop_array_size += 6;
        prop_array = sysdep->core_sysdep_malloc(props->prop_array_size * sizeof(mqtt_property_t), CORE_MQTT_MODULE_NAME);
        if(prop_array == NULL) {
            return STATE_SYS_DEPEND_MALLOC_FAILED;
        }
        memset(prop_array, 0, props->prop_array_size * sizeof(mqtt_property_t));
        if(props->prop_num > 0) {
            memcpy(prop_array, props->prop_array, props->prop_num * sizeof(mqtt_property_t));
            sysdep->core_sysdep_free(props->prop_array);
        }
        props->prop_array = prop_array;
    }

    type = get_type(prop->id);
    if(props->prop_array != NULL) {
        memcpy(&props->prop_array[props->prop_num], prop, sizeof(mqtt_property_t));
        if(type == MQTT_PROP_TYPE_BINARY || type == MQTT_PROP_TYPE_STRING) {
            props->prop_array[props->prop_num].value.str.value = sysdep->core_sysdep_malloc(prop->value.str.len, CORE_MQTT_MODULE_NAME);
            memcpy(props->prop_array[props->prop_num].value.str.value, prop->value.str.value, prop->value.str.len);
        } else if(type == MQTT_PROP_TYPE_STRING_PAIR){
            props->prop_array[props->prop_num].value.str_pair.key.value = sysdep->core_sysdep_malloc(prop->value.str_pair.key.len, CORE_MQTT_MODULE_NAME);
            memcpy(props->prop_array[props->prop_num].value.str_pair.key.value, prop->value.str.value, prop->value.str_pair.key.len);

            props->prop_array[props->prop_num].value.str_pair.value.value = sysdep->core_sysdep_malloc(prop->value.str_pair.value.len, CORE_MQTT_MODULE_NAME);
            memcpy(props->prop_array[props->prop_num].value.str_pair.value.value, prop->value.str_pair.value.value, prop->value.str_pair.value.len);
        }
        props->prop_num++;
    }

    return STATE_SUCCESS;
}

mqtt_property_t *aiot_mqtt_props_get(mqtt_properties_t *props, mqtt_property_identify_t id, int32_t index)
{
    int i = 0, count = 0;
    mqtt_property_t* res = NULL;

    for (i = 0; i < props->prop_num; i++) {
        if (id == props->prop_array[i].id) {
            if (count == index) {
                res = &props->prop_array[i];
                break;
            } else {
                count++;
            }
        }
    }

    return res;
}

void aiot_mqtt_props_deinit(mqtt_properties_t **props_pp)
{
    mqtt_properties_t *props = NULL;
    aiot_sysdep_portfile_t *sysdep = NULL;
    mqtt_property_type_t type;
    int32_t i = 0;
    if(props_pp == NULL || *props_pp == NULL) {
        return;
    }
    props = *props_pp;

    sysdep = aiot_sysdep_get_portfile();
    if (sysdep == NULL) {
        return;
    }

    for(i = 0; i < props->prop_num; i++) {
        type = get_type(props->prop_array[i].id);
        if(type == MQTT_PROP_TYPE_BINARY || type == MQTT_PROP_TYPE_STRING) {
            if(props->prop_array[i].value.str.value != NULL) {
                sysdep->core_sysdep_free(props->prop_array[i].value.str.value);
            }
        } else if(type == MQTT_PROP_TYPE_STRING_PAIR) {
            if(props->prop_array[i].value.str_pair.key.value != NULL) {
                sysdep->core_sysdep_free(props->prop_array[i].value.str_pair.key.value);
            }
            if(props->prop_array[i].value.str_pair.value.value != NULL) {
                sysdep->core_sysdep_free(props->prop_array[i].value.str_pair.value.value);
            }
        }
    }

    sysdep->core_sysdep_free(props->prop_array);
    sysdep->core_sysdep_free(props);

    *props_pp = NULL;
}
/* 计算prop序列化后的数据长度 */
uint32_t prop_len(mqtt_property_t *property)
{
    mqtt_property_type_t type;
    if(property == NULL) return 0;

    type = get_type(property->id);
    switch(type){
        /* Byte */
        case MQTT_PROP_TYPE_UINT8:
            return 2; /* 1 (identifier) + 1 byte */
        /* uint16 */
        case MQTT_PROP_TYPE_UINT16:
            return 3; /* 1 (identifier) + 2 bytes */
        /* uint32 */
        case MQTT_PROP_TYPE_UINT32:
            return 5; /* 1 (identifier) + 4 bytes */
        /* varint */
        case MQTT_PROP_TYPE_VARIABLE:
            if(property->value.uint32 < 128){
                return 2;
            }else if(property->value.uint32 < 16384){
                return 3;
            }else if(property->value.uint32 < 2097152){
                return 4;
            }else if(property->value.uint32 < 268435456){
                return 5;
            }else{
                return 0;
            }
        /* binary */
        case MQTT_PROP_TYPE_BINARY:
        /* string */
        case MQTT_PROP_TYPE_STRING:
            return 3U + property->value.str.len; /* 1 + 2 bytes (len) + X bytes (string) */
        /* string pair */
        case MQTT_PROP_TYPE_STRING_PAIR:
            return 5U + property->value.str_pair.key.len + property->value.str_pair.value.len; /* 1 + 2*(2 bytes (len) + X bytes (string))*/
        default:
            return 0;
    }
    return 0;
}

static int32_t _calc_props_len(mqtt_properties_t *props)
{
    int32_t size = 0, i = 0, res = STATE_SUCCESS;
    for(i = 0; i < props->prop_num; i++) {
        res = prop_check(&props->prop_array[i]);
        if(res < STATE_SUCCESS) {
            return res;
        }

        size += prop_len(&props->prop_array[i]);
    }
    return size;
}

int32_t core_mqtt_props_bound(mqtt_properties_t *props)
{
    uint32_t props_len = 0;
    int8_t offset = 0;
    uint8_t data[5];
    int32_t res = STATE_SUCCESS;

    if(props == NULL) {
        return 1;
    }
    res = _calc_props_len(props);
    if(res < STATE_SUCCESS) {
        return res;
    }

    props_len = res;
    offset = _write_variable(props_len, data);

    return offset + props_len;
}

/* 将prop序列化后写入内存中,返回数据长度 */
int32_t prop_write(mqtt_property_t *property, uint8_t *data, uint32_t size)
{
    mqtt_property_type_t type;
    if(!property) return -1;
    if(size < 2) return -1;

    type = get_type(property->id);
    switch(type){
        /* Byte */
        case MQTT_PROP_TYPE_UINT8:
            *(data++) = property->id;
            *(data++) = property->value.uint8;
            return 2; /* 1 (identifier) + 1 byte */
        /* uint16 */
        case MQTT_PROP_TYPE_UINT16:
            *(data++) = property->id;
            *(data++) = (property->value.uint16 & 0xFF00) >> 8;
            *(data++) = (property->value.uint16 & 0x00FF);
            return 3; /* 1 (identifier) + 2 bytes */
        /* uint32 */
        case MQTT_PROP_TYPE_UINT32:
            *(data++) = property->id;
            *(data++) = (property->value.uint16 & 0xFF000000) >> 24;
            *(data++) = (property->value.uint16 & 0x00FF0000) >> 16;
            *(data++) = (property->value.uint16 & 0x0000FF00) >> 8;
            *(data++) = (property->value.uint16 & 0x000000FF);
            return 5; /* 1 (identifier) + 4 bytes */
        /* varint */
        case MQTT_PROP_TYPE_VARIABLE:
            *(data++) = property->id;
            return _write_variable(property->value.uint32, data) + 1;
        /* binary */
        case MQTT_PROP_TYPE_BINARY:
        /* string */
        case MQTT_PROP_TYPE_STRING:
            *(data++) = property->id;
            *(data++) = (property->value.str.len & 0xFF00) >> 8;
            *(data++) = (property->value.str.len & 0x00FF);
            memcpy(data, property->value.str.value, property->value.str.len);
            return 3U + property->value.str.len; /* 1 + 2 bytes (len) + X bytes (string) */
        /* string pair */
        case MQTT_PROP_TYPE_STRING_PAIR:
            *(data++) = property->id;
            *(data++) = (property->value.str_pair.key.len & 0xFF00) >> 8;
            *(data++) = (property->value.str_pair.key.len & 0x00FF);
            memcpy(data, property->value.str_pair.key.value, property->value.str_pair.key.len);
            data += property->value.str_pair.key.len;
            *(data++) = (property->value.str_pair.value.len & 0xFF00) >> 8;
            *(data++) = (property->value.str_pair.value.len & 0x00FF);
            memcpy(data, property->value.str_pair.value.value, property->value.str_pair.value.len);
            return 5U + property->value.str_pair.key.len + property->value.str_pair.value.len; /* 1 + 2*(2 bytes (len) + X bytes (string))*/
        default:
            return 0;
    }
    return 0;
}

int32_t core_mqtt_props_write(mqtt_properties_t *props, uint8_t *data, uint32_t size)
{
    uint32_t used = 0;
    int32_t i = 0, res = 0;
    uint32_t props_len = 0;
    uint8_t offset = 0;

    if (data == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }

    if(props == NULL) {
        if (size == 0) {
            return STATE_USER_INPUT_OUT_RANGE;
        }
        *data = 0;
        return 1;
    }

    props_len = _calc_props_len(props);
    offset = _write_variable(props_len, data);
    if ((uint32_t)offset + props_len > size) {
        return STATE_USER_INPUT_OUT_RANGE;
    }
    for(i = 0; i < props->prop_num; i++) {
        if(STATE_SUCCESS == prop_check(&props->prop_array[i])) {
            res = prop_write(&props->prop_array[i], data + offset + used, props_len - used);
            if(res > 0) {
                used += (uint32_t)res;
            }
        }
    }
    return (int32_t)(used + offset);
}

/* 将prop序列化后写入内存中,返回数据长度 */
int32_t prop_read(mqtt_property_t *property, uint8_t *data, uint32_t size)
{
    mqtt_property_type_t type;
    aiot_sysdep_portfile_t *sysdep = NULL;
    if(!property) return -1;
    if(size < 2) return -1;

    sysdep = aiot_sysdep_get_portfile();
    if (sysdep == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }

    property->id = *(data++);
    type = get_type(property->id);
    switch(type){
        /* Byte */
        case MQTT_PROP_TYPE_UINT8:
            property->value.uint8 = *(data++);
            return 2; /* 1 (identifier) + 1 byte */
        /* uint16 */
        case MQTT_PROP_TYPE_UINT16:
            property->value.uint16 = *(data++) << 8;
            property->value.uint16 |= *(data++);
            return 3; /* 1 (identifier) + 2 bytes */
        /* uint32 */
        case MQTT_PROP_TYPE_UINT32:
            property->value.uint32 = *(data++) << 24;
            property->value.uint32 |= *(data++) << 16;
            property->value.uint32 |= *(data++) << 8;
            property->value.uint32 |= *(data++);
            return 5; /* 1 (identifier) + 4 bytes */
        /* varint */
        case MQTT_PROP_TYPE_VARIABLE:
            return _read_variable(data, &property->value.uint32) + 1;
        /* binary */
        case MQTT_PROP_TYPE_BINARY:
        /* string */
        case MQTT_PROP_TYPE_STRING:
            property->value.str.len = *(data++) << 8;
            property->value.str.len |= *(data++);
            property->value.str.value = sysdep->core_sysdep_malloc(property->value.str.len, CORE_MQTT_MODULE_NAME);
            memcpy(property->value.str.value, data, property->value.str.len);
            return 3U + property->value.str.len; /* 1 + 2 bytes (len) + X bytes (string) */
        /* string pair */
        case MQTT_PROP_TYPE_STRING_PAIR:
            property->value.str_pair.key.len = *(data++) << 8;
            property->value.str_pair.key.len |= *(data++);
            property->value.str_pair.key.value = sysdep->core_sysdep_malloc(property->value.str_pair.key.len, CORE_MQTT_MODULE_NAME);
            memcpy(property->value.str_pair.key.value, data, property->value.str_pair.key.len);
            data += property->value.str_pair.key.len;
            property->value.str_pair.value.len = *(data++) << 8;
            property->value.str_pair.value.len |= *(data++);
            property->value.str_pair.value.value = sysdep->core_sysdep_malloc(property->value.str_pair.value.len, CORE_MQTT_MODULE_NAME);
            memcpy(property->value.str_pair.value.value, data, property->value.str_pair.value.len);
            return 5U + property->value.str_pair.key.len + property->value.str_pair.value.len; /* 1 + 2*(2 bytes (len) + X bytes (string))*/
        default:
            return 0;
    }
    return 0;
}

int32_t core_mqtt_props_read(uint8_t *data, uint32_t size, mqtt_properties_t *props)
{
    uint32_t used = 0;
    int32_t res = 0;
    uint32_t props_len = 0;
    uint8_t offset = 0;
    mqtt_property_t prop;
    mqtt_property_type_t type;
    aiot_sysdep_portfile_t *sysdep = NULL;

    if (data == NULL || props == NULL) {
        return STATE_USER_INPUT_NULL_POINTER;
    }

    sysdep = aiot_sysdep_get_portfile();
    if (sysdep == NULL) {
        return STATE_PORT_INPUT_NULL_POINTER;
    }

    offset = _read_variable(data, &props_len);
    if ((uint32_t)offset + props_len > size) {
        return STATE_MQTT_INVALID_PROPERTY_LEN;
    }
    while(used < props_len) {
        memset(&prop, 0, sizeof(prop));
        res = prop_read(&prop, data + offset + used, props_len - used);
        if(res > 0) {
            used += (uint32_t)res;
            aiot_mqtt_props_add(props, &prop);
            type = get_type(prop.id);
            if(type == MQTT_PROP_TYPE_BINARY || type == MQTT_PROP_TYPE_STRING) {
                if(prop.value.str.value != NULL) {
                    sysdep->core_sysdep_free(prop.value.str.value);
                }
            } else if(type == MQTT_PROP_TYPE_STRING_PAIR) {
                if(prop.value.str_pair.key.value != NULL) {
                    sysdep->core_sysdep_free(prop.value.str_pair.key.value);
                }
                if(prop.value.str_pair.value.value != NULL) {
                    sysdep->core_sysdep_free(prop.value.str_pair.value.value);
                }
            }
        } else {
            return STATE_MQTT_INVALID_PROPERTY_LEN;
        }
    }
    return (int32_t)(offset + used);
}

int32_t aiot_mqtt_props_print(mqtt_properties_t *props)
{
    int32_t i = 0;
    uint32_t value = 0;
    aiot_sysdep_portfile_t *sysdep = NULL;

    if(props == NULL) {
        return STATE_PORT_INPUT_NULL_POINTER;
    }
    sysdep = aiot_sysdep_get_portfile();
    for(i = 0; i < props->prop_num; i++) {
        switch(props->prop_array[i].id){
            /* Byte */
            case MQTT_PROP_ID_PAYLOAD_FORMAT_INDICATOR: {
                value = props->prop_array[i].value.uint8;
                core_log1(sysdep, 0, "MQTT_PROP_ID_PAYLOAD_FORMAT_INDICATOR:%d \n", &value);
            }
            break;
            case MQTT_PROP_ID_REQUEST_PROBLEM_INFORMATION:{
                value = props->prop_array[i].value.uint8;
                core_log1(sysdep, 0, "MQTT_PROP_ID_REQUEST_PROBLEM_INFORMATION:%d \n", &value);
            }
            break;
            case MQTT_PROP_ID_REQUEST_RESPONSE_INFORMATION:{
                value = props->prop_array[i].value.uint8;
                core_log1(sysdep, 0, "MQTT_PROP_ID_REQUEST_RESPONSE_INFORMATION:%d \n", &value);
            }
            break;
            case MQTT_PROP_ID_MAXIMUM_QOS:{
                value = props->prop_array[i].value.uint8;
                core_log1(sysdep, 0, "MQTT_PROP_ID_MAXIMUM_QOS:%d \n", &value);
            }
            break;
            case MQTT_PROP_ID_RETAIN_AVAILABLE:{
                value = props->prop_array[i].value.uint8;
                core_log1(sysdep, 0, "MQTT_PROP_ID_RETAIN_AVAILABLE:%d \n", &value);
            }
            break;
            case MQTT_PROP_ID_WILDCARD_SUBSCRIPTION_AVAILABLE:{
                value = props->prop_array[i].value.uint8;
                core_log1(sysdep, 0, "MQTT_PROP_ID_WILDCARD_SUBSCRIPTION_AVAILABLE:%d \n", &value);
            }
            break;
            case MQTT_PROP_ID_SUBSCRIPTION_IDENTIFIERS_AVAILABLE:{
                value = props->prop_array[i].value.uint8;
                core_log1(sysdep, 0, "MQTT_PROP_ID_SUBSCRIPTION_IDENTIFIERS_AVAILABLE:%d \n", &value);
            }
            break;
            case MQTT_PROP_ID_SHARED_SUBSCRIPTION_AVAILABLE:{
                value = props->prop_array[i].value.uint8;
                core_log1(sysdep, 0, "MQTT_PROP_ID_SHARED_SUBSCRIPTION_AVAILABLE:%d \n", &value);
            }
            break;

            /* uint16 */
            case MQTT_PROP_ID_SERVER_KEEP_ALIVE:{
                value = props->prop_array[i].value.uint16;
                core_log1(sysdep, 0, "MQTT_PROP_ID_SERVER_KEEP_ALIVE:%d \n", &value);
            }
            break;

            case MQTT_PROP_ID_RECEIVE_MAXIMUM:{
                value = props->prop_array[i].value.uint16;
                core_log1(sysdep, 0, "MQTT_PROP_ID_RECEIVE_MAXIMUM:%d \n", &value);
            }
            break;
            case MQTT_PROP_ID_TOPIC_ALIAS_MAXIMUM:{
                value = props->prop_array[i].value.uint16;
                core_log1(sysdep, 0, "MQTT_PROP_ID_TOPIC_ALIAS_MAXIMUM:%d \n", &value);
            }
            break;
            case MQTT_PROP_ID_TOPIC_ALIAS:{
                value = props->prop_array[i].value.uint16;
                core_log1(sysdep, 0, "MQTT_PROP_ID_TOPIC_ALIAS:%d \n", &value);
            }
            break;

            /* uint32 */
            case MQTT_PROP_ID_MESSAGE_EXPIRY_INTERVAL:{
                value = props->prop_array[i].value.uint32;
                core_log1(sysdep, 0, "MQTT_PROP_ID_MESSAGE_EXPIRY_INTERVAL:%d \n", &value);
            }
            break;
            case MQTT_PROP_ID_WILL_DELAY_INTERVAL:{
                value = props->prop_array[i].value.uint32;
                core_log1(sysdep, 0, "MQTT_PROP_ID_WILL_DELAY_INTERVAL:%d \n", &value);
            }
            break;
            case MQTT_PROP_ID_MAXIMUM_PACKET_SIZE:{
                value = props->prop_array[i].value.uint32;
                core_log1(sysdep, 0, "MQTT_PROP_ID_MAXIMUM_PACKET_SIZE:%d \n", &value);
            }
            break;
            case MQTT_PROP_ID_SESSION_EXPIRY_INTERVAL:{
                value = props->prop_array[i].value.uint32;
                core_log1(sysdep, 0, "MQTT_PROP_ID_SESSION_EXPIRY_INTERVAL:%d \n", &value);
            }
            break;

            /* varint */
            case MQTT_PROP_ID_SUBSCRIPTION_IDENTIFIER:{
                value = props->prop_array[i].value.uint32;
                core_log1(sysdep, 0, "MQTT_PROP_ID_SUBSCRIPTION_IDENTIFIER:%d \n", &value);
            }
            break;

            /* binary */
            case MQTT_PROP_ID_CORRELATION_DATA:
            case MQTT_PROP_ID_AUTHENTICATION_DATA:
                return MQTT_PROP_TYPE_BINARY;
            /* string */
            case MQTT_PROP_ID_CONTENT_TYPE:
            case MQTT_PROP_ID_RESPONSE_TOPIC:
            case MQTT_PROP_ID_ASSIGNED_CLIENT_IDENTIFER:
            case MQTT_PROP_ID_AUTHENTICATION_METHOD:
            case MQTT_PROP_ID_RESPONSE_INFORMATION:
            case MQTT_PROP_ID_SERVER_REFERENCE:
            case MQTT_PROP_ID_REASON_STRING:
                return MQTT_PROP_TYPE_STRING;

            /* string pair */
            case MQTT_PROP_ID_USER_PROPERTY:
                return MQTT_PROP_TYPE_STRING_PAIR;

            default:
                return MQTT_PROP_ID_INVALID;
        }
    }
    return 0;
}

static uint8_t *binary_copy(uint8_t *in, uint32_t inlen)
{
    uint8_t *res = NULL;
    aiot_sysdep_portfile_t *sysdep = NULL;

    sysdep = aiot_sysdep_get_portfile();
    if (sysdep == NULL) {
        return NULL;
    }

    res = sysdep->core_sysdep_malloc(inlen, CORE_MQTT_MODULE_NAME);
    if(res == NULL) {
        return NULL;
    }

    memcpy(res, in, inlen);

    return res;
}

static int32_t prop_copy(mqtt_property_t *in, mqtt_property_t *out)
{
    out->id = in->id;
    switch(in->id){
        /* Byte */
        case MQTT_PROP_ID_PAYLOAD_FORMAT_INDICATOR:
        case MQTT_PROP_ID_REQUEST_PROBLEM_INFORMATION:
        case MQTT_PROP_ID_REQUEST_RESPONSE_INFORMATION:
        case MQTT_PROP_ID_MAXIMUM_QOS:
        case MQTT_PROP_ID_RETAIN_AVAILABLE:
        case MQTT_PROP_ID_WILDCARD_SUBSCRIPTION_AVAILABLE:
        case MQTT_PROP_ID_SUBSCRIPTION_IDENTIFIERS_AVAILABLE:
        case MQTT_PROP_ID_SHARED_SUBSCRIPTION_AVAILABLE:
            out->value.uint8 = in->value.uint8;
        break;
        /* uint16 */
        case MQTT_PROP_ID_SERVER_KEEP_ALIVE:
        case MQTT_PROP_ID_RECEIVE_MAXIMUM:
        case MQTT_PROP_ID_TOPIC_ALIAS_MAXIMUM:
        case MQTT_PROP_ID_TOPIC_ALIAS:
            out->value.uint16 = in->value.uint16;
        break;

        /* uint32 */
        case MQTT_PROP_ID_MESSAGE_EXPIRY_INTERVAL:
        case MQTT_PROP_ID_WILL_DELAY_INTERVAL:
        case MQTT_PROP_ID_MAXIMUM_PACKET_SIZE:
        case MQTT_PROP_ID_SESSION_EXPIRY_INTERVAL:
            out->value.uint32 = in->value.uint32;
        break;
        /* varint */
        case MQTT_PROP_ID_SUBSCRIPTION_IDENTIFIER:
            out->value.uint32 = in->value.uint32;
        break;

        /* binary */
        case MQTT_PROP_ID_CORRELATION_DATA:
        case MQTT_PROP_ID_AUTHENTICATION_DATA:
            out->value.str.len = in->value.str.len;
            out->value.str.value = binary_copy(in->value.str.value, in->value.str.len);
        break;
        /* string */
        case MQTT_PROP_ID_CONTENT_TYPE:
        case MQTT_PROP_ID_RESPONSE_TOPIC:
        case MQTT_PROP_ID_ASSIGNED_CLIENT_IDENTIFER:
        case MQTT_PROP_ID_AUTHENTICATION_METHOD:
        case MQTT_PROP_ID_RESPONSE_INFORMATION:
        case MQTT_PROP_ID_SERVER_REFERENCE:
        case MQTT_PROP_ID_REASON_STRING:
            out->value.str.len = in->value.str.len;
            out->value.str.value = binary_copy(in->value.str.value, in->value.str.len);
        break;

        /* string pair */
        case MQTT_PROP_ID_USER_PROPERTY:
            out->value.str_pair.key.len = in->value.str_pair.key.len;
            out->value.str_pair.key.value = binary_copy(in->value.str_pair.key.value, in->value.str_pair.key.len);
            out->value.str_pair.value.len = in->value.str_pair.key.len;
            out->value.str_pair.value.value = binary_copy(in->value.str_pair.key.value, in->value.str_pair.key.len);
        break;

        default:
            return MQTT_PROP_ID_INVALID;
    }
    return 0;
}

mqtt_properties_t *aiot_mqtt_props_copy(mqtt_properties_t *props)
{
    mqtt_properties_t *copy = NULL;
    aiot_sysdep_portfile_t *sysdep = NULL;
    int i = 0;

    sysdep = aiot_sysdep_get_portfile();
    if (sysdep == NULL) {
        return NULL;
    }

    copy = sysdep->core_sysdep_malloc(sizeof(mqtt_properties_t), CORE_MQTT_MODULE_NAME);
    if (copy == NULL) {
        core_global_deinit(sysdep);
        return NULL;
    }

    memset(copy, 0, sizeof(mqtt_properties_t));
    copy->prop_num = props->prop_num;
    copy->prop_array_size = props->prop_array_size;
    if(copy->prop_num > 0) {
        copy->prop_array = sysdep->core_sysdep_malloc(sizeof(mqtt_property_t) * props->prop_array_size, CORE_MQTT_MODULE_NAME);
        if(copy->prop_array == NULL) {
            sysdep->core_sysdep_free(copy);
            return NULL;
        }

        for(i = 0; i < props->prop_num; i++) {
            prop_copy(&props->prop_array[i], &copy->prop_array[i]);
        }
    }

    return copy;
}
