/*****************************************************************/ /**
* @file qosa_lpm.c
* @brief Low Power Mode operation interface
* @author harry.li@quectel.com
* @date 2025-05-8
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description
* <tr><td>2025-05-8 <td>1.0 <td>harry.Li <td> Init
* </table>
**********************************************************************/
#ifndef _QOSA_LPM_H_
#define _QOSA_LPM_H_

#include "qosa_def.h"
#include "qosa_sys.h"

/*===========================================================================
 *  Macro Definition
 ===========================================================================*/
// tau setting maximum length
#define QOSA_LPM_PTAUS_MAX_LEN                         8

// lpm delay entry sleep time, unit ms
#define QOSA_LPM_DELAY_TIME_MIN                        (1 * 1000)
#define QOSA_LPM_DELAY_TIME_MAX                        (255 * 1000)
#define QOSA_LPM_DELAY_TIME_DEFAULT                    (5 * 1000)

// lpm RRC fast release no data entry sleep time, unit s
#define QOSA_LPM_RRC_FAST_RELEASE_NO_DATA_TIME_MIN     1
#define QOSA_LPM_RRC_FAST_RELEASE_NO_DATA_TIME_MAX     50
#define QOSA_LPM_RRC_FAST_RELEASE_NO_DATA_TIME_DEFAULT 3

// lpm RRC fast release invalid parameter retry time, unit s
#define QOSA_LPM_RRC_FAST_RELEASE_RETRY_TIME_MIN       (1 * 60)
#define QOSA_LPM_RRC_FAST_RELEASE_RETRY_TIME_MAX       (600 * 60)
#define QOSA_LPM_RRC_FAST_RELEASE_RETRY_TIME_DEFAULT   (10 * 60)

// Vote handle
typedef void *qosa_handle;
typedef void (*qosa_wakeup_pin_interrupt_cb)(void *arvg);
/*===========================================================================
 * Enum
===========================================================================*/
/**
 * @enum qosa_lpm_err_e
 * @brief LPM error codes
 *
 */
typedef enum
{
    QOSA_LPM_ERR_OK = 0,

    QOSA_LPM_ERR_INVALID_PARAM = 1 | QOSA_COMPONENT_API_LPM, /*!< Invalid parameter */
    QOSA_LPM_ERR_GENERAL,                                    /*!< Configuration error */
    QOSA_LPM_ERR_NO_HANDLE,                                  /*!< Read error */

} qosa_lpm_error_e;

/**
 * @enum qosa_lpm_mode_e
 * @brief LPM sleep mode
 *
 */
typedef enum
{
    QOSA_LPM_MODE_DISABLE = 0, /*!< Disable sleep */
    QOSA_LPM_MODE_ENABLE = 1,  /*!< Allow sleep */
    QOSA_LPM_MODE_PSM = 2,     /*!< psm mode sleep */
    QOSA_LPM_MODE_MAX,
} qosa_lpm_mode_e;

/**
 * @enum qosa_lpm_dtr_e
 * @brief LPM dtr control sleep
 *
 */
typedef enum
{
    QOSA_LPM_DTR_ENABLE = 0,  /*!< Enable dtr sleep control */
    QOSA_LPM_DTR_DISABLE = 1, /*!< Disable dtr sleep control */
} qosa_lpm_dtr_e;

/**
 * @enum qosa_lpm_uart_e
 * @brief LPM uart control sleep
 *
 */
typedef enum
{
    QOSA_LPM_UART_ENABLE = 0,  /*!< Enable dtr sleep control */
    QOSA_LPM_UART_DISABLE = 1, /*!< Disable dtr sleep control */
} qosa_lpm_uart_e;

/**
 * @enum qosa_lpm_tick_e
 * @brief LPM tick type
 */
typedef enum
{
    QOSA_LPM_TICK_TX_RX_ATS24 = 0, /*!< tx rx time configured by ats24, ats24 temporarily used for mode=1 */
    QOSA_LPM_TICK_TX_RX_SCLK = 1,  /*!< tx rx time configured by qsclk, sclk temporarily used for mode=2 */
    QOSA_LPM_TICK_TX_MAX,
} qosa_lpm_tick_e;

/**
 * @enum qosa_lpm_psm_sleep_status_e
 * @brief PSM SLEEP event type
 *
 */
typedef enum
{
    QOSA_PSM_ENTER_SLEEP
        = 0, /*!< Enter PSM sleep mode, note: some platforms may not have this event, whether to enter PSM sleep is judged by qosa_psm_pre_sleep_callback */
    QOSA_PSM_EXIT_SLEEP = 1, /*!< Exit PSM sleep mode */
} qosa_lpm_psm_sleep_status_e;

/**
 * @enum qosa_lpm_tick_e
 * @brief LPM tick type
 */
typedef enum
{
    QOSA_LPM_SLEEP_STATUS_SLEEP = 0,  /*!< Device enter sleep event */
    QOSA_LPM_SLEEP_STATUS_WAKEUP = 1, /*!< Device wake up event */
} qosa_lpm_sleep_e;

/**
 * @enum qosa_lpm_wakeup_reason_e
 * @brief Device wake up reason
 */
typedef enum
{
    QOSA_LPM_WAKEUP_REASON_DTR = 0,      /*!< DTR pin change */
    QOSA_LPM_WAKEUP_REASON_NET_DATA = 1, /*!< Receive or send network data */
    QOSA_LPM_WAKEUP_REASON_UART = 2,     /*!< UART serial port receive or send data */
    QOSA_LPM_WAKEUP_REASON_USB = 3,      /*!< USB plug/unplug */
    QOSA_LPM_WAKEUP_REASON_ACTIVE = 4,   /*!< Actively call api or AT to disable sleep */
    QOSA_LPM_WAKEUP_REASON_PIN = 5,      /*!< Wakeup triggered by a pin change event */
    QOSA_LPM_WAKEUP_REASON_OTHER = 6,    /*!< Other reasons */
    QOSA_LPM_WAKEUP_REASON_MAX,
} qosa_lpm_wakeup_reason_e;

/**
 * @enum qosa_lpm_sleep_log_e
 * @brief qosa_lpm_sleep_log_e
 */
typedef enum
{
    QOSA_SLEEP_LOG_USB = 0,        /*!< Print SLEEP log to usb port */
    QOSA_SLEEP_LOG_DEBUG = 1,      /*!< Print SLEEP log to debug port */
    QOSA_SLEEP_LOG_DEBUG_TASK = 2, /*!< SLEEP log loop for 5 seconds, print once to debug */
    QOSA_SLEEP_LOG_MAX,
} qosa_lpm_sleep_log_e;

/**
 * @enum qosa_lpm_wakeup_pin_pull_e
 * @brief WAKEUP PIN input pull mode
 */
typedef enum
{
    QOSA_LPM_WAKEUP_PIN_PULL_NONE, /*!< WAKEUP PIN input pull none */
    QOSA_LPM_WAKEUP_PIN_PULL_DOWN, /*!< WAKEUP PIN input pull down */
    QOSA_LPM_WAKEUP_PIN_PULL_UP    /*!< WAKEUP PIN input pull up */
} qosa_lpm_wakeup_pin_pull_e;

/**
 * @enum qosa_lpm_wakeup_pin_trigger_e
 * @brief WAKEUP PIN interrupt trigger mode
 */
typedef enum
{
    QOSA_LPM_TRIGGER_DISABLED,     /*!< Disable interrupt */
    QOSA_LPM_TRIGGER_FALLING_EDGE, /*!< Falling edge interrupt */
    QOSA_LPM_TRIGGER_RISING_EDGE,  /*!< Rising edge interrupt */
    QOSA_LPM_TRIGGER_BOTH_EDGE     /*!< Falling and rising edge interrupt */
} qosa_lpm_wakeup_pin_trigger_e;

/**
 * @enum qosa_lpm_wakeup_pin_level_e
 * @brief WAKEUP PIN output level
 */
typedef enum
{
    QOSA_LPM_LEVEL_LOW, /*!< WAKEUP PIN output low level */
    QOSA_LPM_LEVEL_HIGH /*!< WAKEUP PIN output high level */
} qosa_lpm_wakeup_pin_level_e;

/*===========================================================================
 *  Struct
===========================================================================*/
/**
 * @struct qosa_lpm_sleep_event_t
 * @brief Device sleep status change report
 */
typedef struct
{
    qosa_lpm_sleep_e         status; /*!< Sleep status */
    qosa_lpm_wakeup_reason_e reason; /*!< Wake up reason, only valid when device wakes up */
    qosa_uint8_t             wakeup_pin_num; /*!< Pin that triggered the wake-up, only valid when the wake-up reason is pin */
} qosa_lpm_sleep_event_t;

/**
 * @enum qosa_lpm_config_t
 * @brief LPM function configuration
 */
typedef struct
{
    qosa_lpm_mode_e lpm_mode;                           /*!< Sleep mode */
    qosa_uint32_t   delay_time;                         /*!< Delay sleep time ms */
    qosa_uint8_t    tau[QOSA_LPM_PTAUS_MAX_LEN + 1];    /*!< Value of tau timer T3412 */
    qosa_uint8_t    active[QOSA_LPM_PTAUS_MAX_LEN + 1]; /*!< Value of active timer T3324 */
    qosa_lpm_dtr_e  dtr_en;                             /*!< dtr wake up enable setting */
    qosa_lpm_uart_e uart_en;                            /*!< uart wake up enable setting */
    qosa_bool_t     rfr_enable;                         /*!< Whether to enable RRC fast release function */
    qosa_uint8_t    no_data_time; /*!< After no data interaction, how long to release rrc and then enter sleep state; unit: second; range: 1~50 seconds */
    qosa_uint16_t
        retry_time; /*!< Unit: second; range: 1~600 minutes; when abnormality is detected, close rrc fast release function, then reopen after retry_time interval */
} qosa_lpm_config_t;

/**
 * @struct qosa_lpm_psm_event_t
 * @brief PSM sleep status event, corresponding to event ID QOSA_EVENT_PSM_SLEEP_STATUS
 *
 */
typedef struct
{
    qosa_lpm_psm_sleep_status_e sleep_status; /*!< PSM sleep status */
} qosa_lpm_psm_event_t;

#ifdef CONFIG_QOSA_NBIOT_CUST_FUNC
/**
 * @brief PSM runtime state enumeration
 */
typedef enum
{
    QOSA_PSM_STATE_INACTIVE = 0,  /* Module is NOT in PSM mode */
    QOSA_PSM_STATE_ACTIVE   = 1,  /* Module is in PSM mode */
} qosa_psm_state_e;
#endif /* CONFIG_QOSA_NBIOT_CUST_FUNC */

/*===========================================================================
 * Function
===========================================================================*/
/**
 * @brief lpm sleep mode, delay time configuration
 *
 * @param[in] qosa_lpm_config_t * info
 *           - config structure
 *
 * @retuen None
 */
qosa_lpm_error_e qosa_lpm_config_set(qosa_lpm_config_t *info);

/**
 * @brief lpm sleep mode information acquisition
 *
 * @param[out] qosa_lpm_config_t * info
 *           - config structure
 *
 * @retuen None
 */
void qosa_lpm_config_get(qosa_lpm_config_t *info);

/**
 * @brief lpm create vote handle
 *
 * @param[in] const char * name
 *           - Name of the voter
 * @param[in] qosa_handle *handle_id
 *           - Vote handle
 *
 * @retuen qosa_lpm_err_e
 *       - Success returns QOSA_LPM_ERR_OK
 *       - Failure returns other qosa_lpm_err_e
 */
qosa_lpm_error_e qosa_lpm_app_vote_new_handle(const char *name, qosa_handle *handle_id);

/**
 * @brief lpm vote to allow entering sleep
 *
 * @param[in] qosa_handle handle_id
 *           - Vote handle
 *
 * @retuen qosa_lpm_err_e
 *       - Success returns QOSA_LPM_ERR_OK
 *       - Failure returns other qosa_lpm_err_e
 */
qosa_lpm_error_e qosa_lpm_app_vote_enable(qosa_handle handle_id);

/**
 * @brief lpm vote to prohibit entering sleep
 *
 * @param[in] qosa_handle handle_id
 *           - Vote handle
 *
 * @retuen qosa_lpm_err_e
 *       - Success returns QOSA_LPM_ERR_OK
 *       - Failure returns other qosa_lpm_err_e
 */
qosa_lpm_error_e qosa_lpm_app_vote_disable(qosa_handle handle_id);

/**
 * @brief lpm delete vote handle
 *
 * @param[in] qosa_handle handle_id
 *           - Vote handle
 *
 * @retuen qosa_lpm_err_e
 *       - Success returns QOSA_LPM_ERR_OK
 *       - Failure returns other qosa_lpm_err_e
 */
qosa_lpm_error_e qosa_lpm_app_vote_del_handle(qosa_handle handle_id);

/**
 * @brief Configure wakeup_pin initialization settings, including interrupt trigger edge and pull-up/pull-down
 *
 * @param[in] qosa_uint8_t wakeup_pin_num
 *          - For configuring wakeup_pin_num, refer to the pinmux table
 * 
 * @param[in] qosa_lpm_wakeup_pin_pull_e wakeup_pin_pull
 *          - Configure WAKEUP_PIN pull-up/pull-down mode
 * 
 * @param[in] qosa_lpm_wakeup_pin_trigger_e wakeup_pin_edge
 *          - configuration interrupt trigger edge
 * 
 * @param[in] qosa_wakeup_pin_interrupt_cb *int_cb
 *          - interrupt callback
 *
 * @param[in] void *cb_ctx
 *          - interrupt callback parameter
 * 
 * @retuen qosa_lpm_err_e
 *       - Success returns QOSA_LPM_ERR_OK
 *       - Failure returns other qosa_lpm_err_e
 */
qosa_lpm_error_e qosa_lpm_wakeup_pin_init(qosa_uint8_t wakeup_pin_num,qosa_lpm_wakeup_pin_pull_e wakeup_pin_pull, qosa_lpm_wakeup_pin_trigger_e wakeup_pin_edge,qosa_wakeup_pin_interrupt_cb wakeup_pin_interrupt_cb,void *cb_ctx);

/**
 * @brief Used to get the pin level
 *
 * @param[in] qosa_uint8_t wakeup_pin_num
 *          - For configuring wakeup_pin_num, refer to the pinmux table
 *
 * @param[out] qosa_lpm_wakeup_pin_level_e *level
 *          - Get wakeup pin output level
 *
 * @retuen qosa_lpm_err_e
 *       - Success returns QOSA_LPM_ERR_OK
 *       - Failure returns other qosa_lpm_err_e
 */
qosa_lpm_error_e qosa_lpm_wakeup_pin_get_level(qosa_uint8_t wakeup_pin_num, qosa_lpm_wakeup_pin_level_e *level);

/**
 * @brief Deinitialize wakeup pin configuration
 *
 * @param[in] qosa_uint8_t wakeup_pin_num
 *          - For configuring wakeup_pin_num, refer to the pinmux table
 *
 * @retuen qosa_lpm_err_e
 *       - Success returns QOSA_LPM_ERR_OK
 *       - Failure returns other qosa_lpm_err_e
 */
qosa_lpm_error_e qosa_lpm_wakeup_pin_deinit(qosa_uint8_t wakeup_pin_num);

#ifdef CONFIG_QOSA_LPM_SLEEP_LOG_FUNC
/**
 * @brief Query sleep log print mode
 *
 * @return qosa_lpm_sleep_log_e
 *        - Return to log print mode
 */
qosa_lpm_sleep_log_e qosa_lpm_get_sleep_debug(void);

/**
 * @brief Query sleep log print mode
 *
 * @param[in] qosa_lpm_sleep_log_e log_mode
 *           - Device Sleep/Wake Log Print Mode
 * @return qosa_lpm_error_e
 *        - Returns QOSA_LPM_ERR_OK if successful
 *        - Returns the corresponding error code if the operation fails
 */
qosa_lpm_error_e qosa_lpm_set_sleep_debug(qosa_lpm_sleep_log_e log_mode);

#endif /* CONFIG_QOSA_LPM_SLEEP_LOG_FUNC */

#ifdef CONFIG_QOSA_NBIOT_CUST_FUNC
/**
 * @brief Get current PSM runtime state
 *
 * This state is stored in AON memory and survives deep sleep/hibernate.
 *
 * @return qosa_psm_state_e
 *       - QOSA_PSM_STATE_ACTIVE if module is currently in PSM mode.
 *       - QOSA_PSM_STATE_INACTIVE otherwise.
 */
qosa_psm_state_e qosa_psm_state_get(void);

/**
 * @brief Set current PSM runtime state
 *
 * This state is stored in AON memory and survives deep sleep/hibernate.
 *
 * @param[in] state
 *          - QOSA_PSM_STATE_ACTIVE to mark entering PSM.
 *          - QOSA_PSM_STATE_INACTIVE to mark exiting PSM.
 */
void qosa_psm_state_set(qosa_psm_state_e state);
#endif /* CONFIG_QOSA_NBIOT_CUST_FUNC */
#endif /* _QOSA_LPM_H_ */
