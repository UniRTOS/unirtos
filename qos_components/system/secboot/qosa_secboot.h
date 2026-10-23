/*****************************************************************/ /**
* @file qosa_secboot.h
* @brief
* @author felix.liu@quectel.com
* @date 2026-01-20
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2026-01-20 <td>1.0 <td>Felix.Liu <td> Init
* </table>
**********************************************************************/
#ifndef __QOSA_SECBOOT_H__
#define __QOSA_SECBOOT_H__

#include "qosa_def.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
#include "qosa_sys.h"


/** SECBOOT API FUNCTION ERROR OFFSET CODE*/
#define QOSA_ERRCODE_SECBOOT_BASE (QOSA_COMPONENT_SECBOOT << 16)

/**
 * @enum qosa_secboot_error_e
 * @brief secboot error code
 */
typedef enum
{
    QOSA_SECBOOT_OK = QOSA_OK,
    QOSA_SECBOOT_EXECUTE_ERR = QOSA_ERRCODE_SECBOOT_BASE | 1,         /*!< Execution failed */    
    QOSA_SECBOOT_INVALID_PARAM_ERR ,                                  /*!< Invalid parameter */
    QOSA_SECBOOT_UNKNOW_ROM_VECTOR ,                                  /*!< Unknow rom vector */
    QOSA_SECBOOT_ERROR_MAX
} qosa_secboot_error_e;


/**
 * @brief The function is used to enable secboot,
 * after enabling, the configuration will be written to the EFuse and cannot be turned off.
 * After the configuration of this interface is written to the efuse, it will automatically restart
 * 
 * @return qosa_secboot_error_e
 *          - Return QOSA_SECBOOT_OK if successful
 *          - Return other values if failed
 */
qosa_secboot_error_e qosa_secboot_enable(void);

/**
 * @brief The function is used to get state of secboot enable 
 * @param[in] qosa_uint8_t *enable_state
 *            0:disable  1:enable
 *
 * @return qosa_secboot_error_e
 *          - Return QOSA_SECBOOT_OK if successful
 *          - Return other values if failed
 */
qosa_secboot_error_e qosa_secboot_get_enable_state(qosa_uint8_t *enable_state);

/**
 * @brief The function is used to set DM log control switch
 * @param[in] qosa_uint8_t onoff
 *            0:stop log  1:start log
 *
 * @return void
 *          - No return value
 */
void qosa_redda_set_dm_log_ctl(qosa_uint8_t onoff);

/**
 * @brief The function is used to set dump control switch
 * @param[in] qosa_uint8_t onoff
 *            0:stop log  1:start log
 *
 * @return void
 *          - No return value
 */
void qosa_redda_set_dm_dump_ctl(qosa_uint8_t onoff);
#endif// !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
#endif /* __QOSA_SECBOOT_H__ */