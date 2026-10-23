/*****************************************************************/ /**
* @file qosa_at_config.h
* @brief
* @author egan.li@quectel.com
* @date 2023-06-16
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description
* <tr><td>2023-04-03 <td>1.0 <td>egan.Li <td> Init
* </table>
**********************************************************************/
#ifndef __AT_CONFIG_H__
#define __AT_CONFIG_H__

#include "qosa_def.h"

#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
#include "qosa_at_sio.h"
/**
 * @brief AT device configuration item enumeration type
 */
typedef enum
{
    QOSA_AT_CONFIG_S0 = 0,
    QOSA_AT_CONFIG_S3,
    QOSA_AT_CONFIG_S4,
    QOSA_AT_CONFIG_S5,
    QOSA_AT_CONFIG_S7,
    QOSA_AT_CONFIG_ATE,
    QOSA_AT_CONFIG_ATQ,
    QOSA_AT_CONFIG_ATV,
    QOSA_AT_CONFIG_ATX,
    QOSA_AT_CONFIG_CMEE,
    QOSA_AT_CONFIG_CSCS,
    QOSA_AT_CONFIG_VTD,
    QOSA_AT_CONFIG_CREG,
    QOSA_AT_CONFIG_CGREG,
    QOSA_AT_CONFIG_CEREG,
    QOSA_AT_CONFIG_C5GREG,
    QOSA_AT_CONFIG_CGEREP,
    QOSA_AT_CONFIG_AMP_C,
    QOSA_AT_CONFIG_AMP_D,
    QOSA_AT_CONFIG_CIREG,
    QOSA_AT_CONFIG_CGEREP_MODE,
    QOSA_AT_CONFIG_CGEREP_BFR,
    QOSA_AT_CONFIG_MAX,
} qosa_at_config_e;

/**
 * @brief AT device UART configuration item enumeration type
 */
typedef enum
{
    QOSA_AT_UART_BAUDRATE,
    QOSA_AT_UART_DATA_BIT,
    QOSA_AT_UART_FLOW_CTRL,
    QOSA_AT_UART_PARITY_BIT,
    QOSA_AT_UART_STOP_BIT,
    QOSA_AT_UART_MAX,
} qosa_at_uart_cfg_e;

typedef enum
{
    QOSA_CS_IRA,
    QOSA_CS_GSM,
    QOSA_CS_UCS2,
    QOSA_CS_MAX,
} qosa_at_chset_e;

/**
 * @brief Get AT device configuration information
 *
 * This function is used to get the corresponding AT device configuration value based on the device port, SIM card and configuration item type.
 *
 * @param[in] qosa_at_dev_type_e dev_port
 *          - Device port type.
 *
 * @param[in] qosa_at_config_e config
 *          - Configuration item enumeration value.
 *
 * @param[in] qosa_uint8_t sim_id
 *          - SIM card ID. Default is 0, select according to actual situation.
 *
 * @return qosa_uint8_t
 *       - Return configuration value.
 */
qosa_uint8_t qosa_at_config_get(qosa_at_dev_type_e dev_port, qosa_at_config_e config, qosa_uint8_t sim_id);

/**
 * @brief Set AT device configuration information
 *
 * @param[in] qosa_at_dev_type_e dev_port
 *          - Device port type.
 *
 * @param[in] qosa_at_config_e config
 *          - Configuration item enumeration value.
 *
 * @param[in] qosa_uint8_t sim_id
 *          - SIM card ID. Default is 0, select according to actual situation.
 *
 * @param[in] qosa_uint8_t value
 *          - Configuration value.
 *
 * @return qosa_bool_t
 *       - Return whether the configuration is successful.
 */
qosa_bool_t qosa_at_config_set(qosa_at_dev_type_e dev_port, qosa_at_config_e config, qosa_uint8_t sim_id, qosa_uint8_t value);

/**
 * @brief Get current real UART related configuration information
 *
 * @param[in] qosa_at_dev_type_e dev_port
 *          - Device port type.
 *
 * @param[in] qosa_at_uart_cfg_e config
 *          - Configuration item enumeration value.
 *
 * @return qosa_uint8_t
 *       - Return configuration value.
 */
qosa_uint32_t qosa_at_uart_config_get(qosa_at_dev_type_e dev_port, qosa_at_uart_cfg_e config);

/**
 * @brief Set UART related configuration information, must use &W configuration to save and then restart to take effect, if you need real-time effect, it is recommended to use qosa_uart.h related interface configuration
 *
 * @param[in] qosa_at_dev_type_e dev_port
 *          - Device port type.
 *
 * @param[in] qosa_at_uart_cfg_e config
 *          - Configuration item enumeration value.
 *
 * @param[in] qosa_uint32_t value
 *          - Configuration value.
 *
 * @return qosa_bool_t
 *       - Return whether the configuration is successful.
 */
qosa_bool_t qosa_at_uart_config_set(qosa_at_dev_type_e dev_port, qosa_at_uart_cfg_e config, qosa_uint32_t value);
#endif//!(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif /* __AT_CONFIG_H__ */
