/*****************************************************************/ /**
* @file qosa_mifi_key_driver.h
* @brief Public key/button management interface
* @author
* @date 2026-07-09
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description
* <tr><td>2026-07-09 <td>1.0 <td>Initial <td> Init
* </table>
**********************************************************************/

#ifndef __QOSA_MIFI_KEY_DRIVER_H__
#define __QOSA_MIFI_KEY_DRIVER_H__

#include "qosa_def.h"
#include "qosa_gpio.h"

/**
 * @enum qosa_key_event_e
 * @brief Key event type reported to upper layer.
 */
typedef enum
{
    QOSA_KEY_EVENT_SINGLE_CLICK = 0, /*!< Single click event after multi-click window timeout */
    QOSA_KEY_EVENT_MULTI_CLICK,      /*!< Multi click event, click_count > 1 */
    QOSA_KEY_EVENT_LONG_PRESS,       /*!< Long press event, only reported once per press cycle */
    QOSA_KEY_EVENT_LONG_PRESS_RELEASE, /*!< Key release after a reported long press */
    QOSA_KEY_EVENT_MAX
} qosa_key_event_e;

/**
 * @enum qosa_key_role_e
 * @brief Key role definition used to identify key purpose.
 */
typedef enum
{
    QOSA_KEY_ROLE_POWERKEY = 0, /*!< Power key, event source from power-key callback */
    QOSA_KEY_ROLE_SYS_REST,     /*!< System reset key, event source from GPIO interrupt */
    QOSA_KEY_ROLE_USER1,        /*!< User defined key role 1, event source from GPIO interrupt */
    QOSA_KEY_ROLE_USER2,        /*!< User defined key role 2, event source from GPIO interrupt */
    QOSA_KEY_ROLE_MAX
} qosa_key_role_e;

/**
 * @struct qosa_key_event_info_t
 * @brief Event payload reported to `qosa_key_event_cb`.
 */
typedef struct
{
    qosa_key_role_e  key_role;    /*!< Registered key role */
    qosa_key_event_e event;       /*!< Event type */
    qosa_uint32_t    click_count; /*!< Valid for multi-click/single-click summary */
    qosa_uint32_t    duration_ms; /*!< Duration from press/release sequence in ms */
} qosa_key_event_info_t;

/**
 * @brief Upper-layer callback for key events.
 *
 * @param[in] event_info
 *          - Event information.
 * @param[in] user_ctx
 *          - User context from registration.
 */
typedef void (*qosa_key_event_cb)(const qosa_key_event_info_t *event_info, void *user_ctx);

/**
 * @struct qosa_key_cfg_t
 * @brief Key registration configuration.
 */
typedef struct
{
    qosa_key_role_e   key_role;       /*!< Key role, unique in module */
    qosa_gpio_num_e   gpio_num;       /*!< GPIO for this key, ignored when role is POWERKEY */
    qosa_gpio_level_e active_level;   /*!< Active level: pressed level */
    qosa_gpio_pull_e  gpio_pull;      /*!< Pull mode used during IRQ registration */
    qosa_uint32_t     debounce_ms;    /*!< Software debounce time */
    qosa_uint32_t     multi_click_ms; /*!< Multi-click aggregation window */
    qosa_uint32_t     long_press_ms;  /*!< Long-press trigger threshold */
    qosa_key_event_cb event_cb;       /*!< Upper-layer callback */
    void             *user_ctx;       /*!< User context for callback */
} qosa_key_cfg_t;

/**
 * @brief Register a key definition.
 *
 * The module binds the GPIO interrupt internally, performs software debounce,
 * and reports single-click, multi-click, and long-press events through a task.
 * POWERKEY role uses `qosa_pwrkey_callback_register` as event source.
 * Other roles use GPIO interrupt as event source.
 * Press/release edges are used as internal state inputs and are not exposed.
 *
 * @param[in] cfg
 *          - Key configuration.
 * @return int
 *          - QOSA_ERROR_OK on success.
 *          - Other values on failure.
 */
int qosa_key_register(const qosa_key_cfg_t *cfg);

/**
 * @brief Unregister a previously registered key.
 *
 * @param[in] key_role
 *          - Key role used in registration.
 * @return int
 *          - QOSA_ERROR_OK on success.
 *          - Other values on failure.
 */
int qosa_key_unregister(qosa_key_role_e key_role);

#endif /* __QOSA_MIFI_KEY_DRIVER_H__ */
