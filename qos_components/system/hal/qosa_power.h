/*****************************************************************/ /**
* @file qosa_power.h
* @brief Power operation interface
* @author larson.li@quectel.com
* @date 2024-04-06
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2024-04-06 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#ifndef __QOSA_POWER_H__
#define __QOSA_POWER_H__

#include "qosa_def.h"
#include "qosa_sys.h"

/*===========================================================================
 * Enum
===========================================================================*/
/**
 * @enum qosa_powd_mode_e
 * @brief Shutdown mode selection enumeration type
 */
typedef enum
{
    QOSA_POWD_IMMDLY = 0, /*!< Immediate shutdown mode */
    QOSA_POWD_NORMAL,     /*!< Normal shutdown mode */
    QOSA_POWD_INVALID
} qosa_powd_mode_e;

/**
 * @enum qosa_reset_mode_e
 * @brief Reset mode selection enumeration type
 */
typedef enum
{
    QOSA_RESET_QUICK = 0, /*!< Do not unregister the network and wait for the FLASH operation to complete */
    QOSA_RESET_NORMAL,    /*!< Unregister the network and wait for the FLASH operation to complete */
    QOSA_RESET_FOTA,      /*!< FOTA upgrade reset */
    QOSA_RESET_UNSAFETY,  /*!< Reset immediately*/
    QOSA_RESET_INVALID
} qosa_reset_mode_e;

/**
 * @enum qosa_boot_cause_e
 * @brief Boot cause enumeration type
 */
typedef enum
{
    QOSA_BOOT_CAUSE_UNKNOWN = 0, /*!< Unknown cause boot */
    QOSA_BOOT_CAUSE_PSM_WAKE,    /*!< Deep sleep/PSM sleep wake up boot */
    QOSA_BOOT_CAUSE_PWRKEY,      /*!< Press powerkey to boot */
    QOSA_BOOT_CAUSE_RESET,       /*!< Press reset button to reset boot */
    QOSA_BOOT_CAUSE_WDG,         /*!< Watchdog reset boot */
    QOSA_BOOT_CAUSE_PANIC,       /*!< System exception dump reset boot */
    QOSA_BOOT_CAUSE_SWRESET,     /*!< Software active reset boot */
    QOSA_BOOT_CAUSE_FOTA,        /*!< FOTA process reset boot */
    QOSA_BOOT_CAUSE_USB,         /*!< Plug in USB to boot */
    QOSA_BOOT_CAUSE_INVALID
} qosa_boot_cause_e;

/**
 * @enum qosa_charge_status_e
 * @brief Charging status enumeration type
 */
typedef enum
{
    QOSA_CHARGE_IDEL = 0, /*!< 0 ME not charging */
    QOSA_CHARGE_ING,      /*!< 1 ME is charging */
    QOSA_CHARGE_STOP      /*!< 2 Charging completed */
} qosa_charge_status_e;

/**
 * @enum qosa_dev_error_e
 * @brief Function execution return error code
 */
typedef enum
{
    QOSA_POWER_SUCCESS = 0,                                /*!< Operation successful */
    QOSA_POWER_GENERAL = 1 | (QOSA_COMPONENT_POWER << 16), /*!< General error */
    QOSA_POWER_INVALID_PARAM,

} qosa_power_error_e;

/**
 * @brief Define the power key callback function pointer type
 *
 * @param pinlevel Power key status value
 */
typedef void (*qosa_pwrkey_callback)(qosa_uint8_t pinlevel);
/*===========================================================================
 * Function
===========================================================================*/
/**
 * @brief Control module platform shutdown action
 *
 * @param[in] qosa_powd_mode_e powd_mode
 *          - Corresponding shutdown mode to execute
 *
 * @return qosa_power_error_e
 *        - If successful return QOSA_POWER_SUCCESS
 *        - If failed return others
 */
qosa_power_error_e qosa_power_down(qosa_powd_mode_e powd_mode);

/**
 * @brief Control module platform to execute reset action
 *
 * @param[in] qosa_reset_mode_e reset_mode
 *          - Corresponding reset mode to execute
 *
 * @return qosa_power_error_e
 *        - If successful return QOSA_POWER_SUCCESS
 *        - If failed return others
 */
qosa_power_error_e qosa_power_reset(qosa_reset_mode_e reset_mode);

/**
 * @brief Get platform boot cause
 *
 * @param[out] boot_cause
 *          - Boot cause
 * @return qosa_power_error_e
 *        - If successful return QOSA_POWER_SUCCESS
 *        - If failed return others
 */
qosa_power_error_e qosa_power_get_boot_cause(qosa_boot_cause_e *boot_cause);

/**
 * @brief Get battery charging status and power level
 *
 * @param[out] qosa_charge_status_e * nBcs
 *          - Battery charging status. 0 ME not charging, 1 ME is charging, 2 Charging completed
 *
 * @param[out] qosa_uint8_t * nBcl
 *          - 0-100 Battery remaining percentage
 *
 * @param[out] qosa_uint32_t * vol
 *          - Battery voltage. Unit: mV
 *
 * @return qosa_power_error_e
 *        - If successful return QOSA_POWER_SUCCESS
 *        - If failed return others
 */
qosa_power_error_e qosa_power_get_charger_status(qosa_charge_status_e *nBcs, qosa_uint8_t *nBcl, qosa_uint32_t *vol);

/**
 * @brief Get the PWRKEY pin level of the module
 *
 * @param[out] qosa_uint8_t *pwrkey_level
 *          - pwrkey_level PWRKEY pin level
 *
 * @return qosa_power_error_e
 *        - If successful return QOSA_POWER_SUCCESS
 *        - If failed return others
 */
qosa_power_error_e qosa_get_pwrkey_level(qosa_uint8_t *pwrkey_level);

/**
 * @brief Register callback function for PWRKEY power-off event
 *
 * @param[in] qosa_pwrkey_callback pwrkey_cb
 *          - pwrkey_cb Callback function pointer for PWRKEY power-off
 *
 * @return qosa_power_error_e
 *        - If successful return QOSA_POWER_SUCCESS
 *        - If failed return others
 */
qosa_power_error_e qosa_pwrkey_callback_register(qosa_pwrkey_callback pwrkey_cb);

/**
 * @brief Get power save mode
 *
 * @param[out] char **mode
 *          - Returns power save mode string ("0"=disabled, "1"=enabled)
 *
 * @return int
 *        - 0 if successful, non-zero if failed
 */
int qosa_power_get_mode(char **mode);

/**
 * @brief Set power save mode
 *
 * @param[in] const char *mode
 *          - Power save mode string ("0"=disabled, "1"=enabled)
 *
 * @return int
 *        - 0 if successful, non-zero if failed
 */
int qosa_power_set_mode(const char *mode);

/**
 * @brief Get auto shutdown time
 *
 * @param[out] char **time_min
 *          - Returns shutdown time in minutes as string
 *
 * @return int
 *        - 0 if successful, non-zero if failed
 */
int qosa_power_get_shutdown_time(char **time_min);

/**
 * @brief Set auto shutdown time
 *
 * @param[in] const char *time_min
 *          - Shutdown time in minutes as string
 *
 * @return int
 *        - 0 if successful, non-zero if failed
 */
int qosa_power_set_shutdown_time(const char *time_min);

/**
 * @brief Get auto sleep time
 *
 * @param[out] char **time_min
 *          - Returns sleep time in minutes as string
 *
 * @return int
 *        - 0 if successful, non-zero if failed
 */
int qosa_power_get_sleep_time(char **time_min);

/**
 * @brief Set auto sleep time
 *
 * @param[in] const char *time_min
 *          - Sleep time in minutes as string
 *
 * @return int
 *        - 0 if successful, non-zero if failed
 */
int qosa_power_set_sleep_time(const char *time_min);

#endif /* __QOSA_POWER_H__ */
