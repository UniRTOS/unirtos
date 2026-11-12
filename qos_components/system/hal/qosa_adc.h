/*****************************************************************/ /**
* @file qosa_adc.h
* @brief ADC operation interface
* @author bronson.zhan@quectel.com
* @date 2025-04-28
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2025-04-28 <td>1.0 <td>Bronson.Zhan <td> Init
* </table>
**********************************************************************/
#ifndef __QOSA_ADC_H__
#define __QOSA_ADC_H__

#include "qosa_def.h"
#include "qosa_sys.h"

/*===========================================================================
 * Macro Definition
 ===========================================================================*/
#define QOSA_ADC_ERRCODE_BASE (QOSA_COMPONENT_API_ADC << 16)

/*========================================================================
*  Enumeration Definition
*========================================================================*/

/****************************  error code about ADC    ***************************/
/**
 * @enum qosa_adc_errcode_e
 * @brief Error codes
 */
typedef enum
{
    QOSA_ADC_SUCCESS = 0,
    QOSA_ADC_EXECUTE_ERR = 1 | QOSA_ADC_ERRCODE_BASE, /*!< Serial port execution error */
    QOSA_ADC_GET_VALUE_ERR,                           /*!< Get result error */
    QOSA_ADC_INVALID_PARAM_ERR,                       /*!< Invalid parameter */
    QOSA_ADC_MEM_ADDR_NULL_ERR,                       /*!< Memory address is null */
} qosa_adc_errcode_e;

/**
 * @enum qosa_adc_channel_e
 * @brief ADC channels
 */
typedef enum
{
    QOSA_ADC0_CHANNEL = 0, /*!< ADC0 */
    QOSA_ADC1_CHANNEL = 1, /*!< ADC1 */
    QOSA_ADC2_CHANNEL = 2, /*!< ADC2 */
    QOSA_ADC3_CHANNEL = 3, /*!< ADC3 */
    QOSA_ADC_CHANNEL_MAX,
} qosa_adc_channel_e;

/**
 * @enum qosa_adc_channel_e
 * @brief ADC division factor
 */
typedef enum
{
    QOSA_ADC_SCALE_LEVEL_0 = 0, /*!< [EIGEN platform] No internal voltage divider resistor, maximum range 1.6V
                                        [Others]    TODO                        */
    QOSA_ADC_SCALE_LEVEL_1,     /*!< [EIGEN platform] Division factor 1/2, maximum range 3.2V
                                        [Others]    TODO                        */
    QOSA_ADC_SCALE_LEVEL_2,     /*!< [EIGEN platform] Division factor 1/4, maximum range 3.3V
                                        [Others]    TODO                        */
    QOSA_ADC_SCALE_LEVEL_3,     /*!< [EIGEN platform] Division factor 1/8, maximum range 3.3V
                                        [Others]    TODO                        */
    QOSA_ADC_SCALE_LEVEL_4,     /*!< [EIGEN platform] Division factor 1/16, maximum range 3.3V
                                        [Others]    TODO                        */
    QOSA_ADC_SCALE_MAX,
} qosa_adc_aux_scale_e;

/**
 * @enum  qosa_adc_ioctl_cmd_e
 * @brief adc ioctl cmd definition.
 */
typedef enum
{
    QOSA_ADC_IOCTL_NONE,
    QOSA_ADC_IOCTL_SET_SCALE, /*!< Set division factor level */
} qosa_adc_ioctl_cmd_e;

/*===========================================================================
 * Struct
 ===========================================================================*/

/*===========================================================================
 * Functions declaration
 ===========================================================================*/

/**
 * @brief Register ADC event handler
 *
 * @param[in] qosa_adc_channel_e adc_id
 *          - ADC channel number selection
 *
 * @param[in] qosa_adc_ioctl_cmd_e cmd
 *          - Corresponding ioctl command
 *
 * @param[in/out] void * arg
 *           - Corresponding ioctl command parameters
 *
 * @return qosa_adc_errcode_e
 *          - QOSA_ADC_SUCCESS: Success
 *          - other: Error code
 */
qosa_adc_errcode_e qosa_adc_ioctl(qosa_adc_channel_e adc_id, qosa_adc_ioctl_cmd_e cmd, void *arg);

/**
 * @brief Get the voltage value of the module's external pin ADC (the interface has already calculated the voltage divider resistor internally)
 *
 * @param[in] qosa_adc_channel_e adc_id
 *          - ADC channel number selection
 *
 * @param[out] int *adc_value
 *          - Read voltage value (unit mv)
 *
 * @return qosa_adc_errcode_e
 *          - QOSA_ADC_SUCCESS: Success
 *          - other: Error code
 *
 * @note This interface is time-consuming and cannot be used in interrupts or callbacks
 */
qosa_adc_errcode_e qosa_adc_get_volt(qosa_adc_channel_e adc_id, int *adc_value);

/**
 * @brief Get the voltage value of vbat
 *
 * @param[in] NULL
 *
 * @param[out] int *adc_value
 *          - Read voltage value (unit mv)
 *
 * @return qosa_adc_errcode_e
 *          - QOSA_ADC_SUCCESS: Success
 *          - other: Error code
 *
 * @note This interface is time-consuming and cannot be used in interrupts or callbacks
 */
qosa_adc_errcode_e qosa_get_vbat_volt(int *adc_value);

/**
 * @brief Get the number of ADC channels (for AT+QADC=? dynamic report)
 *
 * @return qosa_uint8_t
 *          - Number of ADC channels (exclude THERMAL/VBAT)
 */
qosa_uint8_t qosa_adc_get_channel_num(void);

#endif /* __QOSA_ADC_H__ */
