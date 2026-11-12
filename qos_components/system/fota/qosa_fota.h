/*****************************************************************/ /**
* @file qosa_fota.h
* @brief FOTA operation interface
* @author larson.li@quectel.com
* @date 2024-06-13
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2024-06-13 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#ifndef __QOSA_FOTA_H__
#define __QOSA_FOTA_H__

#include "qosa_def.h"
#include "qosa_sys.h"

typedef enum
{
    QOSA_FOTA_OK = 0,
    QOSA_FOTA_OPEN_ERROR = -1,          /*!< FOTA file open error */
    QOSA_FOTA_WRITE_ERROR = -2,         /*!< FOTA data write error */
    QOSA_FOTA_READ_ERROR = -3,          /*!< FOTA data read error */
    QOSA_FOTA_ERASE_ERROR = -4,         /*!< FOTA data read error */
    QOSA_FOTA_COMPARE_ERROR = -5,       /*!< FOTA data read error */
    QOSA_FOTA_SEEK_ERROR = -6,          /*!< FOTA seek error */
    QOSA_FOTA_UPDATE_ERROR = -7,        /*!< Upgrade failed */
    QOSA_FOTA_PARAM_ERROR = -8,         /*!< Input parameter error */
    QOSA_FOTA_IMAGE_VERIFY_ERROR = -9,  /*!< Image verification failed */
    QOSA_FOTA_CODE_GENERAL_ERROR = -10, /*!< FOTA upgrade internal error */
    QOSA_FOTA_NOT_EXIST_ERROR = -11,    /*!< File does not exist */
    QOSA_FOTA_OUT_OF_MEMORY = -12,      /*!< Insufficient memory */
    QOSA_FOTA_PARTITION_ERROR = -13,    /*!< Partition error */
    QOSA_FOTA_CLOSE_ERROR = -14,        /*!< Close failed */
    QOSA_FOTA_RENAME_ERROR = -15,       /*!< Rename failed */
    QOSA_FOTA_BUF_NOT_ENOUGH = -16,     /*!< Insufficient buffer */
    QOSA_FOTA_FLASH_READ_ERROR = -17,   /*!< FLASH data read error */
    QOSA_FOTA_FLASH_WRITE_ERROR = -18,  /*!< FLASH data write error */
    QOSA_FOTA_FLASH_ERASE_ERROR = -19,  /*!< FLASH data erase error */
    QOSA_FOTA_VERIFY_NOT_ENABLE = -20,  /*!< Fota verify not enable */
    QOSA_FOTA_NOT_SUPPORT_ERROR = -21,  /*!< Fota NOT support */
} qosa_fota_errno_e;
typedef enum
{
    QOSA_FOTA_VERIFY_NOT_SUPPORT = -1,
    QOSA_FOTA_VERIFY_DISABLE = 0,
    QOSA_FOTA_VERIFY_ENABLE = 1, 
} qosa_fota_verify_control_e;

typedef enum
{
    // Currently, only the cmd for setting the fota mode has been added, and additional ones can be added as needed in the future
    QOSA_FOTA_OPT_CMD_SET_FOTA_MODE = 0,  // Set Upgrade Mode
    QOSA_FOTA_OPT_CMD_SET_MINI_URL,       // Set the package URL of QOSA_FOTA_MINI_MODE, param = char * (ASR160X), see the end of this file
}qosa_fota_opt_cmd_e;

typedef enum
{
    // DFOTA uses the original factory differential upgrade, while fullfota uses the full upgrade newly added by Quec
    QOSA_FOTA_DFOTA_MODE = 0, // Default DFOTA
    QOSA_FOTA_FULL_MODE, // Full FOTA
    QOSA_FOTA_MINI_MODE, // Mini-system FOTA (ASR160X): the mini system downloads the package after a reboot, see the end of this file
}qosa_fota_mode_e;

/**
 * @brief FOTA operation handle for platform side
 */
typedef void *qosa_fota_t;

/**
 * @brief Initialize and apply for internal FOTA resource information
 *
 * @param[in] char * fota_name
 *          - User location and name for saving FOTA firmware package
 *
 * @param[in] qosa_bool_t del_old_file
 *          - Whether to delete historical files, note that if upgrading through existing UFS files, it should be FALSE not to delete
 *
 * @return osa_fota_manage_t *
 *       - Return corresponding FOTA management handle pointer if successful
 *       - Return OSA_NULL if failed
 *
 * @note QOSA_FOTA_MINI_MODE (ASR160X): fota_name must be QOSA_NULL, the package URL is set with
 *       QOSA_FOTA_OPT_CMD_SET_MINI_URL before, see the end of this file
 */
qosa_fota_t *qosa_fota_init(char *fota_name, qosa_bool_t del_old_file);

/**
 * @brief Release and destroy FOTA upgrade related resources
 *
 * @param[in] qosa_fota_t * fota
 *          - FOTA address pointer corresponding to init initialization
 *
 */
void qosa_fota_deinit(qosa_fota_t *fota);

/**
 * @brief Get FOTA signature verification settings (enable/disable)
 *
 * @return qosa_fota_verify_control_e
 *       - Signature verification has been disabled. Return to QOSA_FOTA_VERIFY_DISABLE
 *       - Signature verification has been enabled. Return QOSA_FOTA_VERIFY_ENABLE
 */
qosa_fota_verify_control_e qosa_get_fota_verify_config(void);

/**
 * @brief Enable/Disable FOTA signature verification
 *
 * @param[in] qosa_fota_verify_control_e value
 *          - Set to TRUE to enable FOTA signature verification, and set to FALSE to disable FOTA signature verification.
 *
 * @return qosa_fota_errno_e
 *       - Return OSA_FOTA_OK if successful
 *       - Return other values if failed
 */
qosa_fota_errno_e qosa_set_fota_verify_config(qosa_fota_verify_control_e value);

/**
 * @brief Verify whether the downloaded image information is correct
 *
 * @param[in] qosa_fota_t * fota
 *          - FOTA address pointer corresponding to init initialization
 *
 * @return qosa_fota_errno_e
 *       - Return OSA_FOTA_OK if successful
 *       - Return other values if failed
 */
qosa_fota_errno_e qosa_fota_verify_image(qosa_fota_t *fota);

/**
 * @brief Get partition space size to check if it meets data writing requirements
 *
 * @param[in] qosa_fota_t * fota
 *          - FOTA address pointer corresponding to init initialization
 *
 * @return qosa_int64_t
 *       - Return actual space size
 *       - Return -1 if failed
 */
qosa_int64_t qosa_fota_get_partition_space(qosa_fota_t *fota);

/**
 * Get FOTA upgrade status after restart
 */
qosa_fota_errno_e qosa_fota_get_update_result(void);

/**
 * @brief FOTA data packet writing
 *
 * @param[in] qosa_fota_t * fota
 *          - FOTA address pointer corresponding to init initialization
 *
 * @param[in] qosa_uint8_t * payload
 *          - Corresponding upgrade data content
 *
 * @param[in] qosa_size_t payload_len
 *          - Corresponding payload data packet length
 *
 * @return qosa_fota_errno_e
 *       - Return OSA_FOTA_OK if successful
 *       - Return other values if failed
 */
qosa_fota_errno_e qosa_fota_write_packet_data(qosa_fota_t *fota, qosa_uint8_t *payload, qosa_size_t payload_len);

/**
 * @brief Set the number of URC reports reported by the underlying bootloader during upgrade
 *
 * If the platform does not need it, it can be not adapted
 *
 * @param[in] qosa_uint8_t update_urc_num
 *          - Set the number of update reports
 *
 * @return qosa_fota_errno_e
 *       - Return OSA_FOTA_OK if successful
 *       - Return other values if failed
 */
qosa_fota_errno_e qosa_fota_set_update_urc_num(qosa_fota_t *fota, qosa_uint8_t update_urc_num);

/**
 * @brief Get the current file storage space size
 *
 * @param[in] qosa_fota_t * fota
 *          - FOTA address pointer corresponding to init initialization
 *
 * @return qosa_int64_t
 *       - Return current file size
 */
qosa_int64_t qosa_fota_get_current_file_size(qosa_fota_t *fota);

/**
 * @brief Return FOTA firmware package write name
 *
 * When adapting, can return empty *length = 0
 *
 * @param[out] char * packname
 *           - Return corresponding firmware package name
 *
 * @param[in/out] int * length
 *           - Return firmware package data length
 *
 * @return qosa_fota_errno_e
 *       - Return OSA_FOTA_OK if successful
 *       - Return other values if failed
 */
qosa_fota_errno_e qosa_fota_get_default_packname(char *packname, int *length);

/**
 * @brief Set fota pack file name
 *
 * @param[in] char * packetname
 *          - Set default pack file name, note that the name here is the real folder path
 *
 * @param[in] int length
 *          - Corresponding packet name length
 *
 * @return qosa_fota_errno_e
 *       - Return OSA_FOTA_OK if successful
 *       - Return other values if failed
 */
qosa_fota_errno_e qosa_fota_set_packname(char *packetname, int length);

/**
 * @brief Delete fota file, please ensure the file is closed, if the file is opened, it will fail
 *
 */
void qosa_fota_file_reset(void);

/**
 * @brief Set FOTA upgrade flag
 *
 * @note If the platform needs to set flag separately, adapt it, otherwise it is empty, if more parameters are needed later, it can be modified to a structure
 */
void qosa_fota_flag_set(void);

/**
 * @brief Get whether it has been checked if the data contains special packet headers
 *
 * @return int
 *       - Return firmware package check flag, download firmware type mode can only be distinguished by firmware package data parsing
 *
 * @note Upper layer handles URC reporting
 */
qosa_bool_t qosa_fota_get_updater_info(void);

/**
 * @brief Start updater upgrade
 *
 * @note Currently only used by eigen platform, other platforms may not adapt, directly return QOSA_FOTA_OK
 */
int qosa_fota_updater_start(void);

/**
 * @brief Set fota option
 *
 * @param[in] qosa_fota_opt_cmd_e cmd
 *          - Set fota option cmd
 *
 * @param[in] void *param
 *          - corresponds to the parameter passed to cmd 
 *
 * @return qosa_fota_errno_e
 *       - Return OSA_FOTA_OK if successful
 *       - Return other values if failed
 */
qosa_fota_errno_e qosa_fota_set_opt(qosa_fota_opt_cmd_e cmd, void *param);

/*
 * Mini-system FOTA: QOSA_FOTA_MINI_MODE, ASR160X only.
 *
 * The main system only checks the request and arms the mini system. After the reboot the mini
 * system downloads and applies the package on its own, like AT+QFOTADL, reports the
 * +QIND: "FOTA" URCs on the AT ports and boots the new firmware.
 *   qosa_fota_set_opt(QOSA_FOTA_OPT_CMD_SET_FOTA_MODE, QOSA_FOTA_MINI_MODE)
 *   qosa_fota_get_default_packname()  gives an empty name in this mode: there is no package
 *                                     file, the URL goes to QOSA_FOTA_OPT_CMD_SET_MINI_URL
 *   qosa_fota_set_opt(QOSA_FOTA_OPT_CMD_SET_MINI_URL, url)
 *                                     NUL-terminated http:// or https:// URL, at most 255 bytes,
 *                                     ending with the file name. Its extension gives the layout:
 *                                     ending in '1' (<name>.mini_1) = two packages, <name>.mini_2
 *                                     sits next to it; anything else = one merged package.
 *                                     QOSA_FOTA_PARAM_ERROR if the URL cannot be used.
 *   qosa_fota_init(QOSA_NULL, QOSA_TRUE)
 *                                     opens the request for that URL. QOSA_NULL if no URL was set,
 *                                     in the mini system, or while another request is open. Each
 *                                     request takes the URL, the next one sets it again.
 *   qosa_fota_verify_image()          merged package: downloads its first 4 KB over PDP context 1,
 *                                     which the mini system uses too, and checks them against
 *                                     the running firmware (QOSA_FOTA_READ_ERROR: no HTTP
 *                                     206, connect, DNS or TLS; QOSA_FOTA_IMAGE_VERIFY_ERROR: wrong
 *                                     package). Then saves the URL (QOSA_FOTA_FLASH_WRITE_ERROR).
 *                                     Secure FOTA (signature check before the reboot) is not
 *                                     supported yet.
 *   qosa_fota_deinit()
 *   qosa_fota_flag_set()              arms the mini system, it starts at the next reboot
 *   qosa_power_reset(QOSA_RESET_FOTA) reboot now, or leave it to a later reboot
 * Blocking calls, the task needs at least 8 KB of stack. Do not run it together with AT+QFOTADL.
 *
 * Application full FOTA: QOSA_FOTA_FULL_MODE, ASR160X OpenCPU builds only.
 *
 * It needs a "customer_fs" partition in the flash layout (the customer adds it to the partition
 * JSON and to the download template). The kernel mounts that partition as its own LittleFS
 * volume; without it qosa_fota_set_opt() refuses QOSA_FOTA_FULL_MODE with
 * QOSA_FOTA_NOT_SUPPORT_ERROR.
 *
 * The package is the FBFMake full package of ap_application.bin of the CSDK build
 * (fbfmake_full.bat: FBFMake -f config -a a -b a, one image whose Flash_Entry_Address is the offset
 * of ap_application), as for the application full FOTA of the ASR1606 open SDK. A raw
 * ap_application.bin is refused.
 *   qosa_fota_set_opt(QOSA_FOTA_OPT_CMD_SET_FOTA_MODE, QOSA_FOTA_FULL_MODE)
 *   qosa_fota_init(path or QOSA_NULL, QOSA_TRUE), qosa_fota_write_packet_data() ...
 *   qosa_fota_verify_image(), qosa_fota_deinit(), qosa_fota_flag_set(), then reset.
 * In this mode qosa_fota_get_default_packname() returns the package file that QOSA_NULL selects.
 * qosa_fota_init(path, QOSA_FALSE) verifies a package file that is already on the module instead.
 * At the next boot the kernel copies the image into ap_application before the application
 * starts, then reboots once more. qosa_fota_get_update_result() reports that install until
 * qosa_fota_file_reset() deletes the package and its record.
 */

#endif /* __QOSA_FOTA_H__ */
