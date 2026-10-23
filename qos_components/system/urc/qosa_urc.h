/*****************************************************************/ /**
* @file urc.h
* @brief URC (User Ringing Control) interface definitions
* @author larson.li@quectel.com
* @date 2023-05-22
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2023-05-22 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#ifndef __QOSA_URC_H__
#define __QOSA_URC_H__

#include "qosa_def.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
#include "qosa_sys.h"
#include "qosa_at_sio.h"

/** urc error code base */
#define QOSA_ERRCODE_URC_BASE (QOSA_COMPONENT_URC << 16)

/*! osa base component error */
typedef enum
{
    QOSA_URC_SEND_RES_OK = QOSA_OK,                              /*!< URC send to cache success */
    QOSA_URC_SEND_RES_ERROR_GENERAL = 1 | QOSA_ERRCODE_URC_BASE, /*!< urc module general error */
    QOSA_URC_SEND_RES_CACHE_FULL,                                /*!< urc send cache full error */
    QOSA_URC_SEND_RES_CACHE_HIGH,                                /*!< urc cache high watermark mark */
    QOSA_URC_SEND_RES_PARAM_ERROR,                               /*!< urc param input error */
} qosa_urc_errcode_e;

/**
 * @brief URC configuration option enumeration
 *
 * This enumeration is used to define various options for URC (User Ringing Control) configuration. URC configuration allows users to customize device behavior,
 * such as how to notify users when there is an incoming call, or how to handle data reception. Each option represents a configurable parameter,
 * such as port settings, delay, length, interval, etc.
 */
typedef enum
{
    QOSA_URC_CFG_OPT_ERR = -1,    /*!< Error option */
    QOSA_URC_CFG_OPT_ENABLE = 0,  /*!< Enable URC configuration */
    QOSA_URC_CFG_OPT_PORT,        /*!< Current URC reporting port */
    QOSA_URC_CFG_OPT_DELAY,       /*!< Delay setting */
    QOSA_URC_CFG_OPT_LEN,         /*!< Length setting */
    QOSA_URC_CFG_OPT_INTERVAL,    /*!< Interval setting */
    QOSA_URC_CFG_OPT_CACHE,       /*!< Cache setting */
    QOSA_URC_CFG_OPT_RI_RING,     /*!< Incoming call ring related settings */
    QOSA_URC_CFG_OPT_RI_SMS,      /*!< SMS related settings */
    QOSA_URC_CFG_OPT_RI_OTHERS,   /*!< Other related settings */
    QOSA_URC_CFG_OPT_RI_SIG_TYPE, /*!< Signal type setting */
    QOSA_URC_CFG_OPT_METHOD_TYPE, /*!< Notification type setting */
    QOSA_URC_CFG_OPT_RI_LEVEL,    /*!< RI level setting */
    QOSA_URC_CFG_OPT_MAX          /*!< Maximum option value, used to check if enumeration value is valid */
} qosa_urc_cfg_opt_e;

typedef enum
{
    QOSA_URC_CMUX_ALL = 0,
    QOSA_URC_CMUX_CHANNEL1 = 1,
    QOSA_URC_CMUX_CHANNEL2,
    QOSA_URC_CMUX_CHANNEL3,
    QOSA_URC_CMUX_CHANNEL4,
    QOSA_URC_CMUX_CHANNEL5,
    QOSA_URC_CMUX_OFF,
    QOSA_URC_CMUX_MAX,
} qosa_urc_cmux_config_e;

/**
 * @brief RI output type enumeration
 *
 * This enumeration defines different RI (Ring Indicator) output types, used to control RI output behavior
 * It includes options from turning off output to various conditional output options, such as pulse output, always output, automatic output and waveform output
 */
typedef enum
{
    QOSA_RI_OUT_TYPE_NONE = -1, /*!< No RI output type */
    QOSA_RI_OUT_TYPE_OFF,       /*!< RI output off */
    QOSA_RI_OUT_TYPE_PULSE,     /*!< Pulse RI output */
    QOSA_RI_OUT_TYPE_ALWAYS,    /*!< Always RI output */
    QOSA_RI_OUT_TYPE_AUTO,      /*!< Automatic RI output */
    QOSA_RI_OUT_TYPE_WAVE,      /*!< Waveform RI output */
    QOSA_RI_OUT_TYPE_MAX        /*!< RI output type maximum value */
} qosa_ri_out_type_e;

/**
 * @enum qosa_ri_category_e
 * @brief Define different types of URC RI pin types
 */
typedef enum
{
    QOSA_RI_CATEG_RING,   /*!< URC RI pin pull type during call */
    QOSA_RI_CATEG_SMS,    /*!< SMS type URC RI pin pull type */
    QOSA_RI_CATEG_OTHERS, /*!< Common type URC RI pin pull type */
    QOSA_RI_CATEG_MAX
} qosa_ri_category_e;

typedef enum
{
    QOSA_RI_SIGNAL_TYPE_RESPECTIVE, /*!< RI pin toggles on specified URC port */
    QOSA_RI_SIGNAL_TYPE_PHYSICAL,   /*!< RI pin toggles on default UART1 port */
    QOSA_RI_SIGNAL_TYPE_MAX
} qosa_ri_signal_type_e;

/**
 * @enum qosa_urc_out_method_e
 * @brief URC notification type, distinguish between broadcast type and single channel type
 */
typedef enum
{
    QOSA_URC_OUT_METHOD_UNICAST,   /*!< URC single channel send */
    QOSA_URC_OUT_METHOD_BROADCAST, /*!< URC broadcast send */
} qosa_urc_out_method_e;

/**
 * @enum qosa_urc_force_method_e
 * @brief URC notification type, distinguish between broadcast type and single channel type
 */
typedef enum
{
    QOSA_URC_METHOD_FORCE_DEFAULT,   /*!< URC send according to user configuration */
    QOSA_URC_METHOD_FORCE_UNICAST,   /*!< URC force single channel send */
    QOSA_URC_METHOD_FORCE_BROADCAST, /*!< URC force broadcast send */
} qosa_urc_force_method_e;

/**
 * @enum qosa_urc_config_enable_e
 * @brief URC enable
 */
typedef enum
{
    QOSA_URC_CONFIG_DISABLE, /*!< URC disable */
    QOSA_URC_CONFIG_ENABLE,  /*!< URC enable */
} qosa_urc_config_enable_e;

/**
 * @enum osa_urc_option_t
 * @brief This enumeration is for optional functions, can use '|' to select multiple functions
 */
typedef enum
{
    QOSA_URC_SEND_DEFAULT = 0,   /*!< Default, URC does not queue jump, forcibly add S3,S4(\r\n) at the beginning, intelligently add at the end */
    QOSA_URC_SEND_FIRSTLY = 1,   /*!< urc queue jump */
    QOSA_URC_FORCE_ADD_S3S4 = 2, /*!< urc forcibly add S3,S4 at beginning and end */
    QOSA_URC_FORCE_SEND = 4,     /*!< URC force send, mainly used for FOTA upgrade */
} qosa_urc_option_t;

typedef struct
{
    qosa_ri_out_type_e out_type; /*!< RI pulse output type */
    qosa_uint32_t      duration; /*!< RI pulse duration */
    qosa_uint32_t      counter;  /*!< RI pulse counter */
} qosa_ri_pulse_t;

// Define the default value of the AT port
#if defined(CONFIG_QOSA_UIS8852_PLATFORM_FUNC) || defined(CONFIG_QOSA_QT2131_PLATFORM_FUNC) || defined(QL_NB_SUPPROT)
// The QT2131 currently only supports serial ports; it will be changed to USB with URC output as the default in the future.
#define QOSA_URC_CONFIG_ATPINDEX_DEFAULT_VALUE QOSA_DEV_SIOLIB_UART1_PORT
#else
#define QOSA_URC_CONFIG_ATPINDEX_DEFAULT_VALUE QOSA_DEV_SIOLIB_USB_AT_PORT
#endif

// Define default, maximum and minimum values for delay time, ms
#define QOSA_URC_CONFIG_DELAY_TIME_DEFAULT_VALUE               0
#define QOSA_URC_CONFIG_DELAY_TIME_MAX_VALUE                   10000
#define QOSA_URC_CONFIG_DELAY_TIME_MIN_VALUE                   0

// Define default, maximum and minimum values for RI pulse ring out type
#define QOSA_URC_CONFIG_RI_PULSE_RING_OUTTYPE_DEFAULT_VALUE    QOSA_RI_OUT_TYPE_PULSE
#define QOSA_URC_CONFIG_RI_PULSE_RING_OUTTYPE_MAX_VALUE        QOSA_RI_OUT_TYPE_PULSE
#define QOSA_URC_CONFIG_RI_PULSE_RING_OUTTYPE_MIN_VALUE        QOSA_RI_OUT_TYPE_OFF

// Define default, maximum and minimum values for RI pulse ring duration, ms
#define QOSA_URC_CONFIG_RI_PULSE_RING_DURATION_DEFAULT_VALUE   120
#define QOSA_URC_CONFIG_RI_PULSE_RING_DURATION_MAX_VALUE       2000
#define QOSA_URC_CONFIG_RI_PULSE_RING_DURATION_MIN_VALUE       1

// Define default, maximum and minimum values for RI pulse ring counter
#define QOSA_URC_CONFIG_RI_PULSE_RING_COUNTER_DEFAULT_VALUE    1
#define QOSA_URC_CONFIG_RI_PULSE_RING_COUNTER_MAX_VALUE        5
#define QOSA_URC_CONFIG_RI_PULSE_RING_COUNTER_MIN_VALUE        1

// Define default, maximum and minimum values for RI pulse SMS out type
#define QOSA_URC_CONFIG_RI_PULSE_SMS_OUTTYPE_DEFAULT_VALUE     QOSA_RI_OUT_TYPE_PULSE
#define QOSA_URC_CONFIG_RI_PULSE_SMS_OUTTYPE_MAX_VALUE         QOSA_RI_OUT_TYPE_PULSE
#define QOSA_URC_CONFIG_RI_PULSE_SMS_OUTTYPE_MIN_VALUE         QOSA_RI_OUT_TYPE_OFF

// Define default, maximum and minimum values for RI pulse SMS duration, ms
#define QOSA_URC_CONFIG_RI_PULSE_SMS_DURATION_DEFAULT_VALUE    120
#define QOSA_URC_CONFIG_RI_PULSE_SMS_DURATION_MAX_VALUE        2000
#define QOSA_URC_CONFIG_RI_PULSE_SMS_DURATION_MIN_VALUE        1

// Define default, maximum and minimum values for RI pulse SMS counter
#define QOSA_URC_CONFIG_RI_PULSE_SMS_COUNTER_DEFAULT_VALUE     1
#define QOSA_URC_CONFIG_RI_PULSE_SMS_COUNTER_MAX_VALUE         5
#define QOSA_URC_CONFIG_RI_PULSE_SMS_COUNTER_MIN_VALUE         1

// Define default, maximum and minimum values for RI pulse others out type
#define QOSA_URC_CONFIG_RI_PULSE_OTHERS_OUTTYPE_DEFAULT_VALUE  QOSA_RI_OUT_TYPE_PULSE
#define QOSA_URC_CONFIG_RI_PULSE_OTHERS_OUTTYPE_MAX_VALUE      QOSA_RI_OUT_TYPE_PULSE
#define QOSA_URC_CONFIG_RI_PULSE_OTHERS_OUTTYPE_MIN_VALUE      QOSA_RI_OUT_TYPE_OFF

// Define default, maximum and minimum values for RI pulse others duration
#define QOSA_URC_CONFIG_RI_PULSE_OTHERS_DURATION_DEFAULT_VALUE 120
#define QOSA_URC_CONFIG_RI_PULSE_OTHERS_DURATION_MAX_VALUE     2000
#define QOSA_URC_CONFIG_RI_PULSE_OTHERS_DURATION_MIN_VALUE     1

// Define default, maximum and minimum values for RI pulse others counter
#define QOSA_URC_CONFIG_RI_PULSE_OTHERS_COUNTER_DEFAULT_VALUE  1
#define QOSA_URC_CONFIG_RI_PULSE_OTHERS_COUNTER_MAX_VALUE      5
#define QOSA_URC_CONFIG_RI_PULSE_OTHERS_COUNTER_MIN_VALUE      1

// Define default value for RI signal type
#define QOSA_URC_CONFIG_RI_SIGNAL_TYPE_DEFAULT_VALUE           QOSA_RI_SIGNAL_TYPE_RESPECTIVE

// Define default value for configuration length
#define QOSA_URC_CONFIG_LEN_DEFAULT_VALUE                      CONFIG_QOSA_URC_SEND_MAX_LEN
#define QOSA_URC_CONFIG_LEN_MAX_VALUE                          CONFIG_QOSA_URC_SEND_MAX_LEN
#define QOSA_URC_CONFIG_LEN_MIN_VALUE                          0

// Define default, maximum and minimum values for interval time
#define QOSA_URC_CONFIG_INTERVAL_DEFAULT_VALUE                 0
#define QOSA_URC_CONFIG_INTERVAL_MAX_VALUE                     120
#define QOSA_URC_CONFIG_INTERVAL_MIN_VALUE                     0

/**
 * @brief URC refresh cache, notify URC to process cache and write characters to SIO
 *
 */
int qcm_urc_flush_cache(void);

/**
 * @brief Used for external configuration of URC attribute status
 *
 * @param[in] qosa_urc_cfg_opt_e urc_cfg_opt
 *            Specific attribute enumeration type to be configured
 *
 * @param[in] void * option
 *            Specific content to be configured
 *
 * @return qosa_urc_errcode_e
 *         Return QOSA_URC_SEND_RES_OK indicates successful execution
 *         Return others indicates execution failure
 *
 */
qosa_urc_errcode_e qcm_urc_config_set_opt(qosa_urc_cfg_opt_e urc_cfg_opt, void *option);

/**
 * @brief Used to get current urc internal configuration items
 *
 * @param[in] qosa_urc_cfg_opt_e urc_cfg_opt
 *            Specific attribute enumeration type to get configuration
 *
 * @param[out] void * option
 *             Specific attribute configuration obtained
 *
 * @return qosa_urc_errcode_e
 *         Return QOSA_URC_SEND_RES_OK indicates successful execution
 *         Return others indicates execution failure
 */
qosa_urc_errcode_e qcm_urc_config_get_opt(qosa_urc_cfg_opt_e urc_cfg_opt, void *option);

/**
 * @brief URC status preparation completed, start outputting first URC
 *
 * @param[in] char * str
 *            Character content for first URC to output, generally "RDY"
 */
void qcm_urc_start_rdy(char *str);

#ifdef CONFIG_QOSA_NBIOT_CUST_FUNC
/**
 * @brief URC status preparation completed, start outputting first URC (NBIOT platform)
 */
void qcm_urc_start_nb();
#endif /* CONFIG_QOSA_NBIOT_CUST_FUNC */

/**
 * @brief Get RI output type based on string
 *
 * @param[in] char * _ri_outtype_str
 *            String information to be queried
 *
 * @return ri_out_type_e
 *         Return corresponding _ri_outtype_str parameter type
 *
 */
qosa_ri_out_type_e qcm_urc_ri_get_outtype_enum(char *qcm_ri_outtype_str);

/**
 * @brief Get corresponding string information based on ri outtype type
 *
 * @param[in] ri_out_type_e ri_out_type
 *            Type to be queried
 *
 * @return char *
 *         Return corresponding string information for ri_out_type,
 *         Return QOSA_NULL if not found
 *
 * @note Function usage precautions (delete if none)
 */
char *qcm_urc_ri_get_outtype_str(qosa_ri_out_type_e ri_out_type);

/**
 * @brief Before shutdown, wait for specified time to allow URC to completely output
 *
 * @param[in] qosa_uint16_t delay_ms
 *         - Wait for specified time
 */
void qcm_urc_send_wait_complete(qosa_uint16_t delay_ms);

/**
 * @brief URC send output function
 *        1. Send AT to default port, urc_dev fill DEV_NONE
 *        2. Send broadcast URC, urc_dev fill valid value, URC will be sent to all ports
 *        3. URC reported from execution port, urc_dev fill link_id in AT command
 *        4. "\r\n" configuration at URC beginning and end can be set through option parameter
 *
 * @param[in] qosa_at_dev_type_e dev_port
 *            To send dev information to a certain dev channel, if dev information is QOSA_DEV_NONE, then send URC information to default
 *            configured URC port.
 *
 * @param[in] char * data
 *            URC string information to be sent
 *
 * @param[in] qosa_uint32_t len
 *            Data length of data parameter to be sent
 *
 * @param[in] qurc_out_method_e opt_mthod
 *            Type to send URC, but single channel send or broadcast. If single channel send
 *            then need to carry correct dev channel information, if broadcast, then dev just needs to be valid.
 *
 * @param[in] qurc_ri_category_e urc_cate
 *            URC data send type, is SMS
 *
 * @param[in] osa_urc_option_t option
 *
 * @return qosa_urc_errcode_e
 *
 */
qosa_urc_errcode_e qcm_urc_send_report(
    qosa_at_dev_type_e    dev_port,
    char                 *data,
    qosa_uint32_t         len,
    qosa_urc_out_method_e opt_mthod,
    qosa_ri_category_e    urc_cate,
    qosa_urc_option_t     option
);

/**
 * @brief ri out flush data if pending.
 *
 * @param[in] qosa_at_dev_type_e dev_port
 *         - at device port
 */
qosa_bool_t qcm_urc_ri_out_flush_data_is_pending(qosa_at_dev_type_e dev_port);

#ifdef CONFIG_UNIR_CMUX_SUPPORT
/**
 * @brief Get CMUX URC reporting port
 *
 * @param[in] void
 *
 * @return qurc_cmux_config_e
 */
qcm_urc_cmux_config_e unir_cmux_get_urc_port(void);

/**
 * @brief Set CMUX URC reporting port
 *
 * @param[in] urc_port ref to qurc_cmux_config_e
 *
 * @return
 * QOSA_URC_SEND_RES_OK   set ok
 * <0                    set fail ref to qosa_urc_errcode_e
 */
qosa_urc_errcode_e unir_cmux_set_urc_port(qosa_urc_cmux_config_e urc_port);
#endif /* CONFIG_UNIR_CMUX_SUPPORT */
#endif// !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
#endif /* __QOSA_URC_H__ */
