/*****************************************************************/ /**
* @file qosa_pwm.h
* @brief PWM operation interface
* @author larson.li@quectel.com
* @date 2025-04-29
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2025-01-09 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#ifndef __QOSA_PWM_H__
#define __QOSA_PWM_H__

#include "qosa_def.h"
#include "qosa_sys.h"

/*===========================================================================
 * Enum
===========================================================================*/
/**
 * @enum qosa_pwm_clk_src_e
 * @brief Clock source selection
 */
typedef enum
{
    QOSA_FCLK_SEL_32K = 0, /*!< Optional when output needs to be maintained during sleep */
    QOSA_FCLK_SEL_13M,     // only ASR1903 support
    QOSA_FCLK_SEL_26M,
} qosa_pwm_clk_src_e;

/**
 * @enum qosa_pwm_error_e
 * @brief PWM error codes
 */
typedef enum
{
    QOSA_PWM_SUCESS = 0,                                       /*!< Indicates PWM operation successful */
    QOSA_PWM_EXECUTE_ERR = 1 | (QOSA_COMPONENT_BSP_PWM << 16), /*!< Indicates PWM operation execution failed */
    QOSA_PWM_INVALID_PARAM_ERR,                                /*!< Indicates PWM operation parameter invalid */
    QOSA_PWM_FUNC_SET_ERR,                                     /*!< Indicates PWM FUN setting failed */
    QOSA_PWM_START_ERR,                                        /*!< Indicates PWM start failed */
    QOSA_PWM_STOP_ERR,                                         /*!< Indicates PWM stop failed */
    QOSA_PWM_REPEAT_OPEN_ERR,                                  /*!< Indicates PWM repeated open failed */
    QOSA_PWM_REPEAT_CLOSE_ERR,                                 /*!< Indicates PWM repeated close failed */
    QOSA_PWM_NO_OPEN_ERR,                                      /*!< Indicates PWM OPEN configuration failed */
    QOSA_PWM_SET_CLK_ERR,                                      /*!< Indicates PWM clock setting failed */
} qosa_pwm_error_e;

/**
 * @enum qosa_pwm_stop_option_e
 * @brief List of PWM stop option, controls pwm output level(high,low or holds current level)when stopped
 */
typedef enum
{
    QOSA_PWM_STOP_LOW      = 0U,  /**< Push pwm output low when stopped */
    QOSA_PWM_STOP_HIGH     = 1U,  /**< Push pwm output high when stopped */
    QOSA_PWM_STOP_HOLD     = 2U,  /**< Hold current output level when stopped */
} qosa_pwm_stop_option_e;
/*===========================================================================
 *  Struct
===========================================================================*/
/**
 * @enum qosa_pwm_info_t
 * @brief PWM configuration
 */
typedef struct
{
    unsigned int
        high_one_cycle_duration; /*!< PWM high-level duration (in timer counts). Defines the number of timer counts during which the PWM output remains HIGH within one period. */
    unsigned int
        total_one_cycle_duration; /*!< PWM total period duration (in timer counts). Defines the total number of timer counts in one complete PWM cycle. */
    unsigned int       pwm_psc;   /*!< PWM clock division factor */
    qosa_pwm_clk_src_e clk_src;   /*!< PWM clock source selection */
    qosa_pwm_stop_option_e pwm_stop_option; /*!< PWM stop option, controls PWM output level (high, low or holds current level) when stopped. Only valid for hardware PWM (26M clock source), not supported in 32K APWM path */
} qosa_pwm_info_t;

/*===========================================================================
 * Function
===========================================================================*/
/**
 * @brief Function to configure PWM (Pulse Width Modulation)
 *
 * @param [in] unsigned char pwm_sel
 *            - PWM selection identifier, used to specify the PWM channel to be configured
 * @param [in] qosa_pwm_info_t *pwm_info
 *            - Pointer to PWM configuration information structure, containing PWM configuration parameters
 *
 * @return qosa_pwm_error_e
 *        - If successful return QOSA_PWM_SUCESS
 *        - If failed return others
 */
qosa_pwm_error_e qosa_pwm_config(unsigned char pwm_sel, qosa_pwm_info_t *pwm_info);

/**
 * @brief Enable PWM output and configure its parameters
 *
 * @param[in] unsigned char pwm_sel
 *           - PWM output selection identifier, used to specify the PWM output channel to be enabled.
 *
 * @param[in] unsigned int high_one_cycle_duration
 *           - The number of clock cycles during which the high level is occupied within one cycle
 *
 * @return qosa_pwm_error_e
 *        - If successful return QOSA_PWM_SUCESS
 *        - If failed return others
 */
qosa_pwm_error_e qosa_pwm_enable(unsigned char pwm_sel, unsigned int high_one_cycle_duration);

/**
 * @brief Disable PWM output function
 *
 * @param[in] unsigned char pwm_sel
 *           - PWM selection identifier. Used to specify the PWM channel to be disabled.
 *
 * @return qosa_pwm_error_e
 *        - If successful return QOSA_PWM_SUCESS
 *        - If failed return others
 */
qosa_pwm_error_e qosa_pwm_disable(unsigned char pwm_sel);
#endif /* __QOSA_PWM_H__ */
