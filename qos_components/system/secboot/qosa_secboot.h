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
#define QOSA_SECCFG_INFO_FILE_NAME  "qosa_seccfg_info.nvm"
#define QOSA_SECCFG_PWD_MAX_LEN     (16)

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
 * @enum  qosa_secboot_ioctl_cmd_e
 * @brief secboot ioctl cmd definition.
 */
typedef enum
{
    QOSA_SECBOOT_IOCTL_NONE,
    QOSA_SECBOOT_IOCTL_SET_REBOOT,
    QOSA_SECBOOT_IOCTL_GET_REBOOT,
} qosa_secboot_ioctl_cmd_e;

typedef struct
{
    char dbg_pwd[QOSA_SECCFG_PWD_MAX_LEN + 1];
    qosa_uint8_t pwd_state;
    qosa_uint8_t dbg_status;
    qosa_uint8_t dbg_log_status;
    qosa_uint8_t dbg_dump_status;
} qosa_seccfg_info_t;

/**
 * @brief SECBOOT control interface
 *
 * This function is used to control SECBOOT related features. Different operations can be performed by specifying different ioctl commands.
 *
 * @param[in] qosa_secboot_ioctl_cmd_e cmd
 *          - ioctl command, specifies the control operation to be performed. See qosa_secboot_ioctl_cmd_e for details.
 *
 * @param[in/out] void *arg
 *          - ioctl command parameter. The type and direction depend on the command. Pass NULL if not required.
 *
 * @return qosa_secboot_error_e
 *          - QOSA_SECBOOT_OK: Operation successful
 *          - Others: Error code, see qosa_secboot_error_e for details
 */
qosa_secboot_error_e qosa_secboot_ioctl(qosa_secboot_ioctl_cmd_e cmd, void *arg);

/**
 * @brief The function is used to enable secboot,
 * after enabling, the configuration will be written to the EFuse and cannot be turned off.
 * After writing the configuration information of this interface to eFuse,
 * it will automatically restart by default, unless relevant settings are made using QOSA_SECBOOT_IOCTL_SET_REBOOT.
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