/*****************************************************************/ /**
* @file qosa_led_driver.h
* @brief LED Driver Layer Interface Definition
* @author
* @date 2025-09-18
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description
* <tr><td>2025-09-18 <td>1.0 <td>Initial <td> Init
* </table>
**********************************************************************/

#ifndef __QOSA_LED_DRIVER_H__
#define __QOSA_LED_DRIVER_H__

#include "qosa_def.h"

#define QOSA_LED_DRIVER_NET_STATUS QOSA_LED_DRIVER_0
#define QOSA_LED_DRIVER_STATUS     QOSA_LED_DRIVER_1
#define QOSA_LED_DRIVER_NET_MODE   QOSA_LED_DRIVER_2

/**
     * @enum qosa_led_drv_type_e
     * @brief LED Driver Type Definition
     */
typedef enum
{
    QOSA_LED_DRV_TYPE_GPIO = 0, /*!< GPIO type LED driver */
    QOSA_LED_DRV_TYPE_PWM,      /*!< PWM type LED driver */
    QOSA_LED_DRV_TYPE_MAX       /*!< Maximum value of driver type */
} qosa_led_drv_type_e;

/**
     * @enum qosa_led_driver_id_e
     * @brief LED Driver ID Definition
     */
typedef enum
{
    QOSA_LED_DRIVER_0 = 0, /*!< Driver 0, fixed as net_status */
    QOSA_LED_DRIVER_1 = 1, /*!< Driver 1 */
    QOSA_LED_DRIVER_2 = 2, /*!< Driver 2, fixed as net_mode */
    QOSA_LED_DRIVER_MAX    /*!< Drive maximum value */
} qosa_led_driver_id_e;

/**
     * @struct qosa_led_drv_config_t
     * @brief LED Driver Configuration Structure
     */
typedef struct
{
    qosa_uint8_t        driver_id;    /*!< Driver ID */
    qosa_led_drv_type_e drv_type;     /*!< Driver type */
    qosa_uint8_t        std_num;      /*!< GPIO number (valid when drv_type is GPIO), actually corresponds to qosa_std_num_e */
    qosa_uint8_t        pwm_num;      /*!< PWM number (valid when drv_type is PWM) */
    qosa_uint8_t        pwm_func;     /*!< PWM function number (valid when drv_type is PWM) */
    qosa_bool_t         active_level; /*!< Active level (TRUE: high level active, FALSE: low level active) */
    qosa_uint8_t        reserved;     /*!< Reserved field */
} qosa_led_drv_config_t;

extern qosa_led_drv_config_t g_driver_id_mapping[];

/**
 * @brief LED driver manager initialization
 *
 * @param[in] max_drivers
 *          - Maximum number of drivers
 *          - Driver ID array
 * @param[in] driver_count
 *          - Number of driver IDs
 * @return int
 */
int qosa_led_driver_manager_init(qosa_uint8_t max_drivers, const qosa_uint8_t *driver_ids, qosa_uint8_t driver_count);

/**
 * @brief Controls the LED driver
 *
 * @param[in] driver_id
 *          - Driver ID
 * @param[in] high
 *          - High-level duration, in ms (0 indicates always off)
 * @param[in] period
 *          - Period duration, in ms
 * @return int
 *          - 0: Success, Non-zero: Failure
 */
int qosa_led_driver_control(qosa_uint8_t driver_id, qosa_uint16_t high, qosa_uint16_t period);

/**
 * @brief Get the number of drivers
 *
 * @return qosa_uint8_t
 *          - The number of initialized drivers
 */
qosa_uint8_t qosa_led_driver_get_count(void);

/**
 * @brief Turn off all PWM-type LED drivers and stop GPIO software blinking
 *
 * @return int
 *          - 0: Success, Non-zero: Failure
 */
int qosa_led_driver_pwm_close(void);

#endif /* __QOSA_LED_DRIVER_H__ */
