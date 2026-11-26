/**
 * @file aiot_mqtt_props_api.h
 * @brief mqtt_prop模块头文件, 提供mqtt 5.0用户属性管理的能力
 *
 * @copyright Copyright (C) 2015-2020 Alibaba Group Holding Limited
 *
 */
#ifndef __aiot_mqtt_props_API_H__
#define __aiot_mqtt_props_API_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>

/** The one byte MQTT V5 property indicator */
typedef enum  {
  MQTT_PROP_ID_PAYLOAD_FORMAT_INDICATOR = 1,     /* type uint8_t */
  MQTT_PROP_ID_MESSAGE_EXPIRY_INTERVAL = 2,      /* type uint32_t */
  MQTT_PROP_ID_CONTENT_TYPE = 3,                 /* type string */
  MQTT_PROP_ID_RESPONSE_TOPIC = 8,               /* type string */
  MQTT_PROP_ID_CORRELATION_DATA = 9,             /* type binary */
  MQTT_PROP_ID_SUBSCRIPTION_IDENTIFIER = 11,     /* type variable */
  MQTT_PROP_ID_SESSION_EXPIRY_INTERVAL = 17,     /* type uint32_t */
  MQTT_PROP_ID_ASSIGNED_CLIENT_IDENTIFER = 18,   /* type string */
  MQTT_PROP_ID_SERVER_KEEP_ALIVE = 19,           /* type uint16_t */
  MQTT_PROP_ID_AUTHENTICATION_METHOD = 21,       /* type string */
  MQTT_PROP_ID_AUTHENTICATION_DATA = 22,         /* type binary */
  MQTT_PROP_ID_REQUEST_PROBLEM_INFORMATION = 23, /* type uint8_t */
  MQTT_PROP_ID_WILL_DELAY_INTERVAL = 24,         /* type uint32_t */
  MQTT_PROP_ID_REQUEST_RESPONSE_INFORMATION = 25,/* type uint8_t */
  MQTT_PROP_ID_RESPONSE_INFORMATION = 26,        /* type string */
  MQTT_PROP_ID_SERVER_REFERENCE = 28,            /* type string */
  MQTT_PROP_ID_REASON_STRING = 31,               /* type string */
  MQTT_PROP_ID_RECEIVE_MAXIMUM = 33,             /* type uint16_t */
  MQTT_PROP_ID_TOPIC_ALIAS_MAXIMUM = 34,         /* type uint16_t */
  MQTT_PROP_ID_TOPIC_ALIAS = 35,                 /* type uint16_t */
  MQTT_PROP_ID_MAXIMUM_QOS = 36,                 /* type uint8_t */
  MQTT_PROP_ID_RETAIN_AVAILABLE = 37,            /* type uint8_t */
  MQTT_PROP_ID_USER_PROPERTY = 38,               /* type string pair */
  MQTT_PROP_ID_MAXIMUM_PACKET_SIZE = 39,         /* type uint32_t */
  MQTT_PROP_ID_WILDCARD_SUBSCRIPTION_AVAILABLE = 40, /* type uint8_t */
  MQTT_PROP_ID_SUBSCRIPTION_IDENTIFIERS_AVAILABLE = 41,/* type uint8_t */
  MQTT_PROP_ID_SHARED_SUBSCRIPTION_AVAILABLE = 42, /* type uint8_t */
} mqtt_property_identify_t;

typedef enum  {
  MQTT_PROP_TYPE_UINT8,
  MQTT_PROP_TYPE_UINT16,
  MQTT_PROP_TYPE_UINT32,
  MQTT_PROP_TYPE_VARIABLE,
  MQTT_PROP_TYPE_BINARY,
  MQTT_PROP_TYPE_STRING,
  MQTT_PROP_TYPE_STRING_PAIR,
  MQTT_PROP_ID_INVALID = 0xFF
} mqtt_property_type_t;

/**
 * @brief value-length 结构体.
 *
 * @details
 *
 * 用于MQTT 5.0协议中作为表示response topic/corelation data/disconnect reason等属性的通用的数据结构, 同时也是@ref user_property_t 的构成元素
 */
typedef struct {
    uint16_t len;
    uint8_t  *value;
} len_value_t;

/**
 * @brief MQTT 5.0协议中用户属性
 *
* @details
 *
 * 作为@ref mqtt_property_t 的结构体的成员
 *
 */
typedef struct {
    len_value_t key;
    len_value_t value;
} user_property_t;

typedef struct {
    mqtt_property_identify_t id;
    union {
        uint8_t uint8;
        uint16_t uint16;
        uint32_t uint32;
        len_value_t str;
        user_property_t str_pair;
    } value;
} mqtt_property_t;

typedef struct {
    /* 实际有效的属性个数 */
    int prop_num;
    /* 属性列表缓存的大小，默认最多10个缓存 */
    int prop_array_size;
    /* 属性的内容 */
    mqtt_property_t *prop_array;
} mqtt_properties_t;

/**
 * @brief mqtt消息属性集合初始化
 *
 * @return mqtt_properties_t *
 * @retval NULL 执行失败
 * @retval 非空NULL 执行成功
 */
mqtt_properties_t *aiot_mqtt_props_init();

/**
 * @brief 向属性集合中增加一条属性
 *
 * @param[in] props 属性集合
 * @param[in] prop 单个属性
 *
 * @return int32_t
 * @retval <STATE_SUCCESS 执行失败
 * @retval >=STATE_SUCCESS 执行成功
 */
int32_t aiot_mqtt_props_add(mqtt_properties_t *props, mqtt_property_t *prop);

/**
 * @brief 从属性集合中，查询属性
 *
 * @param[in] props 属性集合
 * @param[in] id 属性的identifer
 * @param[in] index 序号，相同id存在多个属性时，属性的序号
 *
 * @return mqtt_properties_t *
 * @retval NULL 执行失败
 * @retval 非空NULL 执行成功
 */
mqtt_property_t *aiot_mqtt_props_get(mqtt_properties_t *props, mqtt_property_identify_t id, int32_t index);

/**
 * @brief 属性集合反初始化，回收资源
 *
 * @param[in] props 属性集合
 */
void aiot_mqtt_props_deinit(mqtt_properties_t **props);

/**
 * @brief printf属性内容
 *
 * @param[in] props 属性集合
 */
int32_t aiot_mqtt_props_print(mqtt_properties_t *props);

/**
 * @brief 深拷贝属性
 *
 * @param[in] props 属性集合
 */
mqtt_properties_t *aiot_mqtt_props_copy(mqtt_properties_t *props);

#if defined(__cplusplus)
}
#endif

#endif  /* __aiot_mqtt_props_API_H__ */

