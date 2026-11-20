// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <common/bk_err.h>
#include <components/media_types.h>


#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief tp sensor id type
 * @{
 */
typedef enum
{
	TP_ID_UNKNOW = 0,
	TP_ID_GT911,
	TP_ID_GT1151,
	TP_ID_FT6336,
	TP_ID_HY4633,
	TP_ID_CST816D,
} tp_sensor_id_t;

/**
 * @brief tp sensor interrupt trigger type
 * @{
 */
typedef enum
{
	TP_INT_TYPE_RISING_EDGE = 0,
	TP_INT_TYPE_FALLING_EDGE,
	TP_INT_TYPE_LOW_LEVEL,
	TP_INT_TYPE_HIGH_LEVEL,
} tp_int_type_t;

/**
 * @brief tp sensor software reset type
 * @{
 */
typedef enum
{
	TP_RESET_TYPE_INVALID = 0,
	TP_RESET_TYPE_VALID,
} tp_reset_type_t;

/**
 * @brief tp event type
 * @{
 */
typedef enum
{
	TP_EVENT_TYPE_NONE = 0,
	TP_EVENT_TYPE_DOWN,
	TP_EVENT_TYPE_UP,
	TP_EVENT_TYPE_MOVE
} tp_event_type_t;

/**
 * @brief tp data
 * @{
 */
typedef struct
{
    uint8_t          event;
    uint8_t          track_id;
    uint16_t         x_coordinate;
    uint16_t         y_coordinate;
    uint8_t          width;
    uint32_t         timestamp;
} tp_data_t;

/**
 * @brief tp config
 * @{
 */
typedef struct
{
	media_ppi_t ppi;  /**< sensor image resolution */
	tp_int_type_t int_type;  /**< sensor interrupt trigger type */
	uint8_t refresh_rate;  /**< sensor refresh rate(unit: ms) */
	uint8_t tp_num;  /**< sensor touch number */
} tp_config_t;

/**
 * @brief tp sensor user config
 * @{
 */
typedef struct
{
	uint16_t x_size;  /**< x coordinate size */
	uint16_t y_size;  /**< y coordinate size */
	tp_int_type_t int_type;  /**< sensor interrupt trigger type */
	uint8_t refresh_rate;  /**< sensor refresh rate(unit: ms) */
	uint8_t tp_num;  /**< sensor touch number */
} tp_sensor_user_config_t;

/**
 * @brief tp i2c process
 * @{
 */
typedef struct
{
	int (*read_uint8) (uint8_t addr, uint8_t reg, uint8_t *buff, uint16_t len);  /**< i2c read by one byte */
	int (*write_uint8) (uint8_t addr, uint8_t reg, uint8_t *buff, uint16_t len);  /**< i2c write by one byte */
	int (*read_uint16) (uint8_t addr, uint16_t reg, uint8_t *buff, uint16_t len);  /**< i2c read by two byte */
	int (*write_uint16) (uint8_t addr, uint16_t reg, uint8_t *buff, uint16_t len);  /**< i2c write by two byte */
} tp_i2c_callback_t;

/**
 * @brief tp sensor configure
 * @{
 */
typedef struct
{
	char *name;  /**< sensor name */
	uint8_t id;  /**< sensor type, sensor_id_t */
	media_ppi_t def_ppi;  /**< sensor default resolution */
	tp_int_type_t def_int_type;  /**< sensor default interrupt trigger type */
	uint8_t def_refresh_rate;  /**< sensor default refresh rate(unit: ms) */
	uint8_t def_tp_num;  /**< sensor default touch number */
	uint8_t address;  /**< sensor write register address by i2c */
	bool (*detect)(const tp_i2c_callback_t *cb);  /**< auto detect used tp sensor */
	int (*init)(const tp_i2c_callback_t *cb, tp_sensor_user_config_t *config);  /**< init tp sensor */
	int (*read_tp_info)(const tp_i2c_callback_t *cb, uint8_t max_num, uint8_t *buff);  /**<  read tp information of sensor */
} tp_sensor_config_t;

/**
 * @brief current used tp config
 * @{
 */
typedef struct
{
	char *name;  /**< sensor name */
	uint8_t id;  /**< sensor type, sensor_id_t */
	media_ppi_t ppi;  /**< sensor image resolution */
	tp_int_type_t int_type;  /**< sensor interrupt trigger type */
	uint8_t refresh_rate;  /**< sensor refresh rate(unit: ms) */
	uint8_t tp_num;  /**< sensor touch number */
} tp_device_t;

/**
 * @brief TP sensor detection function structure
 */
typedef struct
{
    const tp_sensor_config_t *(*detect)(const tp_i2c_callback_t *cb);  /**< Detection function pointer */
} tp_sensor_detect_func_t;

/**
 * @brief Section attribute macro implementation
 * @param SECTION Section name
 * @param COUNTER Counter
 */
#define _SECTION_ATTR_IMPL(SECTION, COUNTER)    __attribute__((section(SECTION "." _CPIMTER_STRINGIFY(COUNTER))))

/**
 * @brief Stringify macro
 * @param COUNTER Counter
 */
#define _CPIMTER_STRINGIFY(COUNTER) #COUNTER

/**
 * @brief TP sensor detection function section registration macro
 * @param f Detection function name
 *
 * This macro registers a sensor detection function into a dedicated section,
 * enabling the system to automatically discover and run detection routines.
 */
#define BK_TP_SENSOR_DETECT_SECTION(f)                                                             \
    const tp_sensor_config_t * __bk_tp_sensor_##f(const tp_i2c_callback_t *cb);                   \
    __attribute__((used)) _SECTION_ATTR_IMPL(".tp_sensor_detect_function_list", __COUNTER__)       \
    const tp_sensor_detect_func_t tp_sensor_##f = {                                               \
        .detect = __bk_tp_sensor_##f,                                                              \
    } ;                                                                                             \
    const tp_sensor_config_t * __bk_tp_sensor_##f(const tp_i2c_callback_t *cb)                     \
    { return f(cb); }

extern tp_sensor_detect_func_t __tp_sensor_detect_array_start;
extern tp_sensor_detect_func_t __tp_sensor_detect_array_end;

/*
 * @}
 */

#ifdef __cplusplus
}
#endif
