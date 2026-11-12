/*****************************************************************/ /**
* @file qosa_dev_eigen.h
* @brief RTC operation interface
* @author larson.li@quectel.com
* @date 2025-12-10
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2023-08-15 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#ifndef __QOSA_DEV_EIGEN_H__
#define __QOSA_DEV_EIGEN_H__

#include "qosa_system_config.h"

#if defined (CONFIG_QOSA_EIGEN718_PLATFORM_FUNC)
#define QOSA_AUD_PIN_FUNCS_MCLK_EN_MASK 0x01
#define QOSA_AUD_PIN_FUNCS_DIN_EN_MASK 0x02
#define QOSA_AUD_PIN_FUNCS_DOUT_EN_MASK 0x04
#endif /* CONFIG_QOSA_EIGEN718_PLATFORM_FUNC */

#if defined (CONFIG_QOSA_EIGEN718_PLATFORM_FUNC) || defined (CONFIG_QOSA_QCX216_PLATFORM_FUNC) || defined (CONFIG_QOSA_QCX217_PLATFORM_FUNC)
#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_gpio.h"
#include "qosa_sim.h"
#include "qosa_license.h"
#include "qosa_ftm.h"
#ifdef CONFIG_QOSA_AUDIO_DRIVER
#include "qosa_audio.h"
#endif /* CONFIG_QOSA_AUDIO_DRIVER */

/*===========================================================================
 * Macro Definition
 ===========================================================================*/
typedef struct qosa_tim_ic_handle qosa_tim_ic_handle_t;
/** Maximum band count in one tuner group */
#define QOSA_FTM_ANT_TUNER_BAND_NUM_MAX_ONEGROUP 14
/** Maximum antenna tuner group count */
#define QOSA_FTM_ANT_TUNER_GROUP_NUM_MAX         4
/** FEM5/FEM6 PIN104=FEM5(bit5), PIN105=FEM6(bit6) bits mask */
#define QOSA_FTM_ANT_TUNER_FEM5_FEM6_MASK        ((qosa_uint8_t)(0x3U << 5))
/** RF1: FEM5=0, FEM6=0 */
#define QOSA_FTM_ANT_TUNER_RF1_GPIO_VAL          ((qosa_uint8_t)(0x0U << 5))
/** RF2: FEM5=0, FEM6=1 */
#define QOSA_FTM_ANT_TUNER_RF2_GPIO_VAL          ((qosa_uint8_t)(0x2U << 5))
/** RF3: FEM5=1, FEM6=0 */
#define QOSA_FTM_ANT_TUNER_RF3_GPIO_VAL          ((qosa_uint8_t)(0x1U << 5))
/** RF4: FEM5=1, FEM6=1 */
#define QOSA_FTM_ANT_TUNER_RF4_GPIO_VAL          ((qosa_uint8_t)(0x3U << 5))
/*===========================================================================
 * Enum
 ===========================================================================*/
typedef enum
{
    VOL_1_80V,
    VOL_3_30V
}qosa_gpio_vol_e;

typedef enum
{
    POWER_OFF,
    POWER_ON
}qosa_gpio_power_control_e;

typedef enum
{
    ROOT_CLK_26M,
    ROOT_CLK_612M
} qosa_FracDivRootClk_e;

typedef enum
{
    QOSA_TIMER_CAPTURE_RISING_EDGE      = 0U,  /**< Caputure Rising edge */
    QOSA_TIMER_CAPTURE_FALLING_EDGE     = 1U,  /**< Caputure falling edge */
    QOSA_TIMER_CAPTURE_BOTH_EDGE        = 2U,  /**< Caputure both edges */
} qosa_timerCaptureEdge_e;
typedef enum
{
    QOSA_TIMER_OK = 0,
    QOSA_TIMER_INVALID_PARAM_ERR = -1,
    QOSA_TIMER_SET_CLK_ERR= -2,
} qosa_timer_error_e;
typedef enum
{
    QOSA_TIMER_CLK_INVALID = -1,        /*!< Invalid clock frequency configuration */
    QOSA_TIMER_CLK_40KHZ = 40000,       /*!< Clock frequency: 40K */
    QOSA_TIMER_CLK_26MHZ = 26000000,    /*!< Clock frequency: 26M */
} qosa_timer_clk_e;

/*===========================================================================
 * callback
===========================================================================*/
typedef void (*qosa_capture_callback_cb)(qosa_tim_ic_handle_t *htim );
/*===========================================================================
 *  Struct
===========================================================================*/
typedef struct {
    qosa_uint32_t           timer_id;              /**< Timer instance ID */
    qosa_uint32_t           clock_freq_hz;         /**< Timer clock frequency in Hz */
    qosa_uint16_t           prescaler;             /**< Clock prescaler value */
} qosa_tim_base_t;
typedef struct {
    qosa_uint8_t                pin_num;            /**< GPIO pin number for input capture */
    qosa_uint32_t               polarity;           /**< Capture polarity (reference qosa_timerCaptureEdge_e) */
    qosa_capture_callback_cb    capture_cb;         /**< Callback function invoked on capture event */
} qosa_tim_ic_config_t;
struct qosa_tim_ic_handle{
    qosa_tim_base_t         base;               /**< Timer base configuration */
    qosa_tim_ic_config_t    config;             /**< Input capture channel configuration */
    qosa_uint32_t           captured_value;     /**< Last captured value */
};
typedef struct
{
    qosa_uint8_t qosa_band_num[QOSA_FTM_ANT_TUNER_BAND_NUM_MAX_ONEGROUP];   /** Supported band numbers in this group */
    qosa_uint8_t qosa_gpio_bit_mask; /** GPIO bit mask used by this group */
    qosa_uint8_t qosa_gpio_bit_val;/** GPIO bit value applied with the mask */
} qosa_ftm_ant_tuner_one_group_cfg_t;
typedef union
{
    qosa_ftm_ant_tuner_one_group_cfg_t qosa_ant_tuner_cfg[QOSA_FTM_ANT_TUNER_GROUP_NUM_MAX];/** Structured view of all antenna tuner groups */
    qosa_uint8_t                       qosa_u8[64];/** Raw byte view for storage or transfer */
} qosa_ftm_ant_tuner_cfg_t;
/*===========================================================================
 * Functions declaration
 ===========================================================================*/
/**
 * @brief Set the voltage level for GPIO operations.
 *
 * @param[in] gpio_vol
 *          - Voltage level to be set for GPIO
 *
 * @return qosa_gpio_error_e
 *       - If operation is successful, return QOSA_GPIO_SUCCESS
 *       - Return corresponding error code if failed
 */
qosa_gpio_error_e qosa_gpio_set_voltage(qosa_gpio_vol_e gpio_vol);

/**
 * @brief Turn on/off the AON GPIO power.
 *
 * @return qosa_gpio_error_e
 *       - If operation is successful, return QOSA_GPIO_SUCCESS
 *       - Return corresponding error code if failed
 */
qosa_gpio_error_e qosa_aon_gpio_power_control(qosa_gpio_power_control_e power_control);

/**
 * @brief Get the GNSS img address
 *
 * @return qosa_uint32_t
 *       - Returns 0 if not present
 *       - Returns firmware address if present
 */
qosa_uint32_t qosa_gnss_img_addr(void);

/**
 * @brief Enable 32k clock setting
 *
 * @param[in] enable
 *          - Enable 32k oscillating clock
 */
void qosa_32k_clock_enable(qosa_bool_t enable);

/**
 * @brief Synchronously get SIM power save status
 *
 * @param[in] simid
 *          - Application layer SIM ID, identifying the SIM card to read
 *          - Always 0 in dual card single standby mode
 * @param[out] status
 *          - SIM power save status
 *          - QOSA_FALSE: disabled
 *          - QOSA_TRUE: enabled
 * @return qosa_sim_err_e
 *          - QOSA_SIM_ERR_OK: Operation successful
 *          - QOSA_SIM_ERR_NOT_INSERTED: SIM card not inserted
 *          - QOSA_SIM_ERR_FAILURE: Read failed
 *          - QOSA_SIM_ERR_BUSY: SIM card busy
 *          - For other error codes, please refer to qosa_sim_err_e enumeration definition
 */
qosa_sim_err_e qosa_sim_get_sim_power_save(qosa_uint8_t simid, qosa_bool_t *status);

/**
 * @brief Synchronously set SIM power save status
 *
 * @param[in] simid
 *          - Application layer SIM ID, identifying the SIM card to read
 *          - Always 0 in dual card single standby mode
 * @param[in] status
 *          - SIM power save status
 *          - QOSA_FALSE: disable
 *          - QOSA_TRUE: enable
 * @return qosa_sim_err_e
 *          - QOSA_SIM_ERR_OK: Operation successful
 *          - QOSA_SIM_ERR_NOT_INSERTED: SIM card not inserted
 *          - QOSA_SIM_ERR_FAILURE: Read failed
 *          - QOSA_SIM_ERR_BUSY: SIM card busy
 *          - For other error codes, please refer to qosa_sim_err_e enumeration definition
 */
qosa_sim_err_e qosa_sim_set_sim_power_save(qosa_uint8_t simid, qosa_bool_t status);

/**
 * @brief Enable SIM card sleep mode - This interface is specifically designed for configuring the sleep state of
 *      - the SIM card to prevent any interference with APDU communication.
 *
 * @param[in] simid
 *          - Application layer SIM ID, identifying the SIM card to read
 *          - Always 0 in dual card single standby mode
 * @param[in] state
 *          - Set SIM sleep state
 *          - QOSA_FALSE: disabled
 *          - QOSA_TRUE: enabled
 * @return qosa_sim_err_e
 *          - QOSA_SIM_ERR_OK: Operation successful
 *          - QOSA_SIM_ERR_INVALID_PARAM: Invalid param
 *          - QOSA_SIM_ERR_EXECUTE: Sim execte error
 */
qosa_sim_err_e qosa_sim_set_sim_sleep_state(qosa_uint8_t simid, qosa_bool_t state);

/**
 * @brief Get the sleep state of the SIM card
 *
 * @param[in] simid
 *          - Application layer SIM ID, identifying the SIM card to read
 *          - Always 0 in dual card single standby mode
 * @param[in] *state
 *          - Get SIM sleep state
 *          - QOSA_FALSE: disabled
 *          - QOSA_TRUE: enabled
 * @return qosa_sim_err_e
 *          - QOSA_SIM_ERR_OK: Operation successful
 *          - QOSA_SIM_ERR_INVALID_PARAM: Invalid param
 *          - QOSA_SIM_ERR_EXECUTE: Sim execte error
 */
qosa_sim_err_e qosa_sim_get_sim_sleep_state(qosa_uint8_t simid, qosa_bool_t *state);

/**
 * @brief Used to configure the mclk pin to output the desired clock frequency.
 *
 * @param[in] qosa_uint32_t pin_num
 *          - Pin number to get configuration
 *
 * @param[in] qosa_uint32_t func_sel
 *          - Configure the function selection of the pin
 *
 * @param[in] qosa_uint32_t freq
 *          - Expected output clock frequency (Hz)
 *
 * @param[in] qosa_FracDivRootClk_e clkSrc
 *          - Root clock source selection, refer to the qosa_FracDivRootClk_e enumeration definition.
 *
 * @return qosa_gpio_error_e
 *       - If operation is successful, return QOSA_GPIO_SUCCESS
 *       - Return corresponding error code if failed
 */
qosa_gpio_error_e qosa_pin_config_out_clk(qosa_uint32_t pin_num, qosa_uint32_t func_sel, qosa_uint32_t freq, qosa_FracDivRootClk_e clkSrc);

/**
 * @brief Used to Configure SWD function
 *
 * @note This function uses FUNC to configure the EC platform to enter J-Link mode.
 */
void qosa_set_swd_func(void);

/**
 * @brief Initialize the timer base clock for input capture.
 *
 * @param[in] htim
 *          - Pointer to the input capture handle
 *
 * @return qosa_int32_t
 *       - QOSA_TIMER_OK if initialization is successful
 *       - Return corresponding error code if failed
 */
qosa_int32_t qosa_tim_ic_init(qosa_tim_ic_handle_t *htim);
/**
 * @brief Deinitialize the timer and disable the clock.
 *
 * @param[in] htim
 *          - Pointer to the input capture handle
 *
 * @return qosa_int32_t
 *       - QOSA_TIMER_OK if deinitialization is successful
 *       - Return corresponding error code if failed
 */
qosa_int32_t qosa_tim_ic_deinit(qosa_tim_ic_handle_t *htim);
/**
 * @brief Configure a single input capture channel.
 *
 * @param[in] htim
 *          - Pointer to the input capture handle
 * @param[in] ch_config
 *          - Pointer to the channel configuration structure
 *
 * @return qosa_int32_t
 *       - QOSA_TIMER_OK if configuration is successful
 *       - Return corresponding error code if failed
 */
qosa_int32_t qosa_tim_ic_config_channel(qosa_tim_ic_handle_t *htim,
                                       const qosa_tim_ic_config_t *ch_config);
/**
 * @brief Start input capture and enable the capture interrupt.
 *
 * @param[in] htim
 *          - Pointer to the input capture handle
 *
 * @return qosa_int32_t
 *       - QOSA_TIMER_OK if started successfully
 *       - Return corresponding error code if failed
 */
qosa_int32_t qosa_tim_ic_start_it(qosa_tim_ic_handle_t *htim);
/**
 * @brief Stop the input capture interrupt mode.
 *
 * @param[in] htim
 *          - Pointer to the input capture handle
 *
 * @return qosa_int32_t
 *       - QOSA_TIMER_OK if stopped successfully
 *       - Return corresponding error code if failed
 */
qosa_int32_t qosa_tim_ic_stop_it(qosa_tim_ic_handle_t *htim);
/**
 * @brief Get the most recently captured counter value
 *
 * @param[in] htim
 *          - Pointer to the input capture handle
 *
 * @return qosa_int32_t
 *       - The captured counter value
 *       - Return corresponding error code if failed
 */
qosa_int32_t qosa_tim_ic_get_captured_value(qosa_tim_ic_handle_t *htim);
/**
 * @brief Get the current timer counter value
 *
 * @param[in] htim
 *          - Pointer to the input capture handle
 *
 * @return qosa_int32_t
 *       - The current counter value
 *       - Return corresponding error code if failed
 */
qosa_int32_t qosa_tim_ic_get_counter(qosa_tim_ic_handle_t *htim);

/**
 * @brief Save license authorization information to the platform factory partition.
 *
 * @param[in] auth_info     Plain authorization information to save.
 * @param[in] auth_info_len Authorization information length.
 *
 * @return qosa_license_error_e
 *         Returns QOSA_LIC_OK for success,
 *         Returns others for failure
 */
qosa_license_error_e qosa_license_set_auth_info(unsigned char *auth_info, int auth_info_len);

/**
 * @brief Read license authorization information from the platform factory partition.
 *
 * @param[out] auth_info     Buffer used to store plain authorization information.
 * @param[in,out] auth_info_len Input buffer length, output real authorization information length.
 *
 * @return qosa_license_error_e
 *         Returns QOSA_LIC_OK for success,
 *         Returns others for failure
 */
qosa_license_error_e qosa_license_get_auth_info(unsigned char *auth_info, int *auth_info_len);

/**
 * @brief Get the QR code authorization information.
 *
 * @param[out] qrcode_info    Buffer used to store the QR code authorization information.
 *
 * @return int
 *         Returns the authorization information length for success,
 *         Returns others for failure
 */
int qosa_qingya_get_qrlience(char *qrcode_info);

/**
 * @brief Get the IMEI information.
 *
 * @param[out] qrcode_info    Buffer used to store the IMEI information.
 *
 * @return int
 *         Returns the IMEI information length for success,
 *         Returns others for failure
 */
int qosa_qingya_get_imei(char *qrcode_info);

/**
 * @brief Get antenna tuner FEM GPIO configuration
 *
 * @param[out] pCfg
 *          - Pointer to antenna tuner configuration buffer
 * @return qosa_ftm_err_e
       - Returns QOSA_FTM_OK for success,
       - Returns others for failure
 */
qosa_ftm_err_e qosa_ftm_ant_tuner_fem_gpio_get_cfg(qosa_ftm_ant_tuner_cfg_t *pCfg);

/**
 * @brief Set antenna tuner FEM GPIO full configuration
 *
 * @param[in] pCfg
 *          - Pointer to antenna tuner configuration buffer
 * @return qosa_ftm_err_e
       - Returns QOSA_FTM_OK for success,
       - Returns others for failure
 */
qosa_ftm_err_e qosa_ftm_ant_tuner_fem_gpio_set_cfg(qosa_ftm_ant_tuner_cfg_t *pCfg);

/**
 * @brief Set one antenna tuner FEM GPIO group configuration
 *
 * @param[in] groupIdx
 *          - Group index
 * @param[in] pGroupCfg
 *          - Group configuration pointer
 * @return qosa_ftm_err_e
       - Returns QOSA_FTM_OK for success,
       - Returns others for failure
 */
qosa_ftm_err_e qosa_ftm_ant_tuner_fem_gpio_set_group(qosa_uint8_t groupIdx, qosa_ftm_ant_tuner_one_group_cfg_t *pGroupCfg);

/**
 * @brief Reset antenna tuner FEM GPIO configuration to default
 *
 * @return qosa_ftm_err_e
       - Returns QOSA_FTM_OK for success,
       - Returns others for failure
 */
qosa_ftm_err_e qosa_ftm_ant_tuner_fem_gpio_reset_cfg(void);

#endif /* CONFIG_QOSA_EIGEN718_PLATFORM_FUNC */

#if defined (CONFIG_QOSA_EIGEN718_PLATFORM_FUNC)
#ifdef CONFIG_QOSA_AUDIO_DRIVER
/**
 * @brief Set I2S pin enable
 *
 * @param qosa_uint32_t enMask
 *         - Pin enable mask, like QOSA_AUD_PIN_FUNCS_MCLK_EN_MASK.
 *
 * @return qosa_aud_errcode_e Error code
 *         - Other error codes: Return corresponding error codes based on specific operations
 */
qosa_aud_errcode_e qosa_aud_driver_setPinEn(qosa_uint32_t enMask);
#endif /* CONFIG_QOSA_AUDIO_DRIVER */
#endif /* CONFIG_QOSA_EIGEN718_PLATFORM_FUNC */

#endif /* __QOSA_DEV_EIGEN_H__ */
