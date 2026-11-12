/*****************************************************************/ /**
* @file qosa_mifi_led_driver.h
* @brief MIFI LED public driver interfaces
* @date 2026-08-30
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
**********************************************************************/
#ifndef __QOSA_MIFI_LED_DRIVER_H__
#define __QOSA_MIFI_LED_DRIVER_H__

#include "qosa_def.h"
#include "qosa_gpio.h"

/**
 * @brief LED blink done callback, invoked in LED task context after a
 *        finite blink naturally completes. Must be short and non-blocking.
 *
 * @param[in] id   LED id (GPIO number).
 * @param[in] arg  User argument passed to qosa_mifi_led_blink().
 */
typedef void (*qosa_mifi_led_done_cb_t)(qosa_gpio_num_e id, void *arg);

/**
 * @brief Initialize the LED driver. Called once at boot.
 * @return QOSA_ERROR_OK on success, other value on failure.
 */
int qosa_mifi_led_init(void);

/**
 * @brief Turn on a LED (high level).
 * @param[in] id  LED id (GPIO number).
 * @return QOSA_ERROR_OK on success, other value on failure.
 */
int qosa_mifi_led_on(qosa_gpio_num_e id);

/**
 * @brief Turn off a LED (low level).
 * @param[in] id  LED id (GPIO number).
 * @return QOSA_ERROR_OK on success, other value on failure.
 */
int qosa_mifi_led_off(qosa_gpio_num_e id);

/**
 * @brief Blink a LED (non-blocking).
 *
 * @param[in] id         LED id (GPIO number).
 * @param[in] on_ms      ON (high) duration in ms, must be > 0.
 * @param[in] off_ms     OFF (low) duration in ms, must be > 0.
 * @param[in] times      Blink cycles. 0 = infinite, N = N full on/off cycles
 *                       then stop at stop_level.
 * @param[in] stop_level Final level after a finite blink naturally completes:
 *                       0 = off, 1 = on. Ignored when times == 0.
 * @param[in] cb         Done callback, may be QOSA_NULL. Never fired when
 *                       times == 0. Not fired when the blink is interrupted
 *                       by a newer on/off/blink on the same LED.
 * @param[in] cb_arg     User argument passed to cb.
 * @return QOSA_ERROR_OK on success, other value on failure.
 */
int qosa_mifi_led_blink(qosa_gpio_num_e id,
                        qosa_uint16_t on_ms,
                        qosa_uint16_t off_ms,
                        qosa_uint16_t times,
                        qosa_uint8_t  stop_level,
                        qosa_mifi_led_done_cb_t cb,
                        void *cb_arg);

#endif /* __QOSA_MIFI_LED_DRIVER_H__ */
