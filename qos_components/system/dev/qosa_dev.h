/*****************************************************************/ /**
* @file qosa_dev.h
* @brief Device operation interface
* @author larson.li@quectel.com
* @date 2025-01-24
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2025-01-24 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#ifndef __QOSA_DEV_H__
#define __QOSA_DEV_H__

#include "qosa_def.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#include "qosa_sys.h"
#include "qosa_at_sio.h"

/** Maximum length of IMEI */
#define QOSA_IMEI_MAX_LEN    15
/** Maximum length of SN */
#define QOSA_SN_MAX_LEN      24

/** Minimum value for IPTRACE setting */
#define QOSA_DEV_IPTRACE_MIN 0
/** Maximum value for IPTRACE setting */
#define QOSA_DEV_IPTRACE_MAX 100

/**
 * @enum qosa_dev_error_e
 * @brief Function execution return error codes
 */
typedef enum
{
    QOSA_DEV_ERRID_SUCCESS = 0,
    QOSA_DEV_ERRID_GENERAL = 1 | (QOSA_COMPONENT_DEV << 16), /*!< Unknown error */
    QOSA_DEV_ERRID_INVALID_PARAM,                            /*!< Invalid parameter */
    QOSA_DEV_ERRID_NOT_SUPPORT,                              /*!< Feature not supported */

} qosa_dev_error_e;

typedef enum
{
    QOSA_DEV_ACCESS_TECH_GSM = 0,
    QOSA_DEV_ACCESS_TECH_GSM_COMPACT = 1,
    QOSA_DEV_ACCESS_TECH_UTRAN = 2,
    QOSA_DEV_ACCESS_TECH_GSM_wEGPRS = 3,
    QOSA_DEV_ACCESS_TECH_UTRAN_wHSDPA = 4,
    QOSA_DEV_ACCESS_TECH_UTRAN_wHSUPA = 5,
    QOSA_DEV_ACCESS_TECH_UTRAN_wHSDPA_HSUPA = 6,
    QOSA_DEV_ACCESS_TECH_E_UTRAN = 7,
    QOSA_DEV_ACCESS_TECH_OTHER = 8,
} qosa_dev_act_type_e;

/**
 * @enum qcm_lbs_wifiscan_state_e
 * @brief Current state values of wifiscan
 */
typedef enum
{
    QOSA_DUMPCFG_DISABLE = 0,      /*!< Disable dump capture */
    QOSA_DUMPCFG_ENABLE = 1,       /*!< Enable dump capture */
    QOSA_DUMPCFG_ENABLE_SD = 2,    /*!< Enable dump capture and save to SD card, requires platform support */
    QOSA_DUMPCFG_ENABLE_FLASH = 3, /*!< Enable dump capture and save to flash, requires platform support */
    QOSA_DUMPCFG_ENABLE_OTHER = 4, /*!< Enable dump capture and save to other locations, optional implementation */
    QOSA_DUMPCFG_MAX,
} qosa_dev_dumpcfg_e;

/**
 * @struct qosa_ram_info_t
 * @brief Module related RAM space information
 */
typedef struct
{
    qosa_uint32_t free_ram_size;     /*!< Available heap free space */
    qosa_uint32_t total_ram_size;    /*!< Total heap space size */
    qosa_uint32_t max_free_ram_size; /*!< Maximum contiguous available heap space */
} qosa_ram_info_t;

/**
 * @brief Get system heap information
 *
 * @param[in] qosa_ram_info_t *ram_info_ptr
 *          - Memory heap space information type to get
 *
 * @return qosa_dev_error_e
 *       - 0 indicates successful acquisition
 *       - Others indicate failure
 */
qosa_dev_error_e qosa_dev_get_memory_size(qosa_ram_info_t *ram_info_ptr);

/**
 * @brief Get Sub-memory heap information
 * @brief On some platforms, dual HEAP support may be available, such as EIGEN. This API can be selectively adapted accordingly.
 * @param[in] qosa_ram_info_t *ram_info_ptr
 *          - Sub Memory heap space information type to get
 *
 * @return qosa_dev_error_e
 *       - 0 indicates successful acquisition
 *       - Others indicate failure
 */
qosa_dev_error_e qosa_dev_get_memory_sub_size(qosa_ram_info_t *ram_info_ptr);

/**
 * @brief Get the unique product identifier SN number of the module
 *
 * @param[out] char * SN
 *           - Return the unique coding of the module product
 *
 * @param[in] qosa_int32_t len
 *           - Space length of the corresponding string serial
 */
qosa_dev_error_e qosa_dev_get_sn(char *SN, qosa_int32_t *len);

/**
 * @brief Get the module hardware version number.
 *
 * @param[out] char * HVN
 *           - Return the module hardware version number.
 *
 * @param[in] qosa_int32_t len
 *           - Space length of the corresponding string serial
 *
 * @return qosa_dev_error_e
 *        - Return error code, return OSA_DEV_ERRID_SUCCESS if successful,
 *        - Return OSA_DEV_ERRID_INVALID_PARAM if the parameter is invalid.
 */
qosa_dev_error_e qosa_dev_get_hvn(char *HVN, qosa_int32_t *len);

/**
 * @brief Get the IMEI of the specified SIM card of the module
 *
 * @param[in] qosa_int32_t sim_id
 *          - SIM sequence number parameter
 *
 * @param[out] char *imei
 *          - Parameter to store the obtained IMEI number
 *
 * @param[in/out] qosa_int32_t *len
 *          - Space size of the imei pointer, return the IMEI character length when successfully obtained
 *
 * @return qosa_dev_error_e
 *       -  0 indicates successful acquisition
 *       - Return -1 if acquisition fails
 */
qosa_dev_error_e qosa_dev_get_sim_imei(qosa_int32_t sim_id, char *imei, qosa_int32_t *len);

/**
 * @brief Get the product ID of the device
 *
 * This function is used to get the product ID information of the device and store it in the specified buffer 'product_id'.
 * At the same time, the actual number of characters written to the buffer is passed and returned through the 'len' parameter.
 *
 * @param[out] char *product_id
 *           - Output buffer pointer for storing the obtained product ID string.
 * @param[in,out] qosa_int32_t *len
 *           - As the maximum length of the output buffer when input, and returns the actual string length written when return.
 * @return qosa_dev_error_e
 *       - Function execution result. Returns OSA_DEV_ERRID_SUCCESS when parameters are valid and product ID is successfully obtained;
 *       - Returns OSA_DEV_ERRID_INVALID_PARAM if input parameters are invalid (such as product_id is null or len is less than or equal to 0).
 */
qosa_dev_error_e qosa_dev_get_product_id(char *product_id, qosa_int32_t *len);

/**
 * @brief Set the MAC address of the module
 *
 * @param[in] mac
 *             - Pointer to a character array containing the MAC address string to be set (format: "xx:xx:xx:xx:xx:xx").
 *
 * @return qosa_dev_error_e
 *         - Returns QOSA_DEV_ERRID_SUCCESS if the MAC address is set successfully.
 *         - Returns QOSA_DEV_ERRID_INVALID_PARAM if mac is a NULL pointer or the format is invalid.
 *         - Returns QOSA_DEV_ERRID_GENERAL if MAC address modification fails (e.g., state restriction, hardware error).
 */
qosa_dev_error_e qosa_dev_set_mac(const char *mac);

/**
 * @brief Get the MAC address of the module
 *
 * @param[out] mac
 *             - Pointer to a character array for storing the MAC address string (format: "xx:xx:xx:xx:xx:xx").
 * @param[in/out] len
 *             - On input: Specifies the length of the mac buffer.
 *             - On output: Stores the actual length of the MAC address string obtained.
 *
 * @return qosa_dev_error_e
 *         - Returns QOSA_DEV_ERRID_SUCCESS if the MAC address is obtained successfully.
 *         - Returns QOSA_DEV_ERRID_INVALID_PARAM if mac or len is a NULL pointer.
 *         - Returns QOSA_DEV_ERRID_GENERAL for hardware/software failures (e.g., MAC address unavailable).
 */
qosa_dev_error_e qosa_dev_get_mac(char *mac, qosa_int32_t *len);

/**
 * @brief reset module auto to download mode
 *
 * This function is used to reset module to auto download mode.
 *
 * @param[void]
 *
 * @return  qosa_dev_error_e
 *        - Return error code, return OSA_DEV_ERRID_SUCCESS if successful,
 */
qosa_dev_error_e qosa_dev_set_auto_download(void);

/**
 * @brief Get firmware version information in QGMR command format
 *
 * This function is used to get the firmware version number in QGMR command format from global variables and store it in the specified buffer.
 *
 * @param[out] char *firmware_version
 *           - Pointer to the buffer for storing the firmware version string.
 * @param[in/out] qosa_int32_t *len
 *           - Pointer to an integer used to specify the length of the firmware_version buffer when called,
 *            and store the actual buffer length used when returning.
 * @return qosa_dev_error_e
 *        - Return error code, return OSA_DEV_ERRID_SUCCESS if successful,
 *        - Return OSA_DEV_ERRID_INVALID_PARAM if the parameter is invalid.
 */
qosa_dev_error_e qosa_dev_get_qgmr_firmware_version(char *firmware_version, qosa_int32_t *len);

/**
 * @brief Get firmware version information
 *
 * This function is used to get the firmware version number from global variables and store it in the specified buffer.
 *
 * @param[out] char *firmware_version
 *           - Pointer to the buffer for storing the firmware version string.
 * @param[in/out] qosa_int32_t *len
 *           - Pointer to an integer used to specify the length of the firmware_version buffer when called,
 *            and store the actual buffer length used when returning.
 * @return qosa_dev_error_e
 *        - Return error code, return OSA_DEV_ERRID_SUCCESS if successful,
 *        - Return OSA_DEV_ERRID_INVALID_PARAM if the parameter is invalid.
 */
qosa_dev_error_e qosa_dev_get_firmware_version(char *firmware_version, qosa_int32_t *len);

/**
 * @brief Get the sub-version number of the firmware
 *
 * This function is used to get the current firmware sub-version number from the device. The sub-version number will be copied to the provided buffer.
 *
 * @param[out] char *firmware_subversion
 *           - Pointer to a character array for storing the firmware sub-version number.
 * @param[in/out] qosa_int32_t *len
 *           - Pointer to an integer used to specify the length of the firmware_subversion buffer when called,
 *            and store the actual buffer length used when returning.
 * @return qosa_dev_error_e
 *        - Return error code, return OSA_DEV_ERRID_SUCCESS if successful,
 *        - Return OSA_DEV_ERRID_INVALID_PARAM if the parameter is invalid.
 */
qosa_dev_error_e qosa_dev_get_firmware_subversion(char *firmware_subversion, qosa_int32_t *len);

/**
 * @brief Get the qinfversion of the firmware
 *
 * This function is used to get the current firmware sub-version number from the device. The sub-version number will be copied to the provided buffer.
 *
 * @param[out] char *firmware_qinfversion
 *           - Pointer to a character array for storing the firmware sub-version number.
 *
 * @param[in/out] qosa_int32_t *len
 *           - Pointer to an integer used to specify the length of the firmware_qinfversion buffer when called,
 *            and store the actual buffer length used when returning.
 *
 * @return qosa_dev_error_e
 *        - Return error code, return OSA_DEV_ERRID_SUCCESS if successful,
 *        - Return OSA_DEV_ERRID_INVALID_PARAM if the parameter is invalid.
 */
qosa_dev_error_e qosa_dev_get_firmware_qinf(char *firmware_qinf, qosa_int32_t *len);

/**
 * @brief Get some kernel information of the firmware, different platforms have different formats
 *
 * This function is used to get partial kernel information of the firmware from global variables and store it in the specified buffer.
 *
 * @param[out] char *qversion
 *           - Pointer to the buffer for storing the firmware information string.
 * @param[in/out] qosa_int32_t *len
 *           - Pointer to an integer used to specify the length of the qversion buffer when called,
 *            and store the actual buffer length used when returning.
 * @return qosa_dev_error_e
 *        - Return error code, return OSA_DEV_ERRID_SUCCESS if successful,
 *        - Return OSA_DEV_ERRID_INVALID_PARAM if the parameter is invalid.
 */
qosa_dev_error_e qosa_dev_get_qversion(char *qversion, qosa_int32_t *len);

/**
 * @brief Get module model information
 *
 * @param[out] char *model
 *           - Pointer to a character array for storing model information.
 * @param[in/out] qosa_int32_t *len
 *           - Pointer to an integer used to specify the length of the model buffer when called,
 *            and store the actual buffer length used when returning.
 * @return qosa_dev_error_e
 *        - Return error code, return OSA_DEV_ERRID_SUCCESS if successful,
 *        - Return OSA_DEV_ERRID_INVALID_PARAM if the parameter is invalid.
 */
qosa_dev_error_e qosa_dev_get_model(char *model, qosa_int32_t *len);

/**
 * @brief Get module oem information
 *
 * @param[out] char *oem
 *           - Pointer to a character array for storing oem information.
 *
 * @param[in/out] qosa_int32_t *len
 *           - Pointer to an integer used to specify the length of the oem buffer when called,
 *            and store the actual buffer length used when returning.
 *
 * @return qosa_dev_error_e
 *        - Return error code, return OSA_DEV_ERRID_SUCCESS if successful,
 *        - Return OSA_DEV_ERRID_INVALID_PARAM if the parameter is invalid.
 */
qosa_dev_error_e qosa_dev_get_oem(char *oem, qosa_int32_t *len);

/**
 * @brief Get customer firmware version information
 * 
 * This function is used to obtain the customer-customized firmware version string of the device. 
 * When the QUECTEL_OPEN_MODE_SUPPORT macro is defined, it calls the get_osa_dev_mob_cust_rev() 
 * function to get version information; otherwise it returns an empty string.
 * 
 * @param[out] version Pointer to buffer for storing customer firmware version information
 * @param[in/out] len Input as the length of the version buffer, output as the actual length of the retrieved version string
 * 
 * @return qosa_dev_error_e Error code
 * @retval QOSA_DEV_ERRID_SUCCESS Successfully obtained version information
 * @retval QOSA_DEV_ERRID_INVALID_PARAM Invalid parameters (version or len is NULL)
 * 
 * @note Caller must ensure the passed version buffer is large enough to accommodate the version string
 * @see get_osa_dev_mob_cust_rev()
 */
qosa_dev_error_e qosa_dev_get_cust_firmware_version(char *version, qosa_int32_t* len);

/**
 * @brief Get module model information
 *
 * @param[out] char *product
 *           - Pointer to a character array for storing product information.
 *
 * @param[in/out] qosa_int32_t *len
 *           - Pointer to an integer used to specify the length of the product buffer when called,
 *            and store the actual buffer length used when returning.
 *
 * @return qosa_dev_error_e
 *        - Return error code, return OSA_DEV_ERRID_SUCCESS if successful,
 *        - Return OSA_DEV_ERRID_INVALID_PARAM if the parameter is invalid.
 */
qosa_dev_error_e qosa_dev_get_usb_product(char *product, qosa_int32_t *len);

/**
 * @brief Get module chip type
 *
 * @param[out] char *chip_type
 *           - Pointer to a character array for storing the module chip type.
 *
 * @param[in/out] qosa_int32_t *len
 *           - Pointer to an integer used to specify the length of the chip_type buffer when called,
 *            and store the actual buffer length used when returning.
 *
 * @return qosa_dev_error_e
 *        - Return error code, return OSA_DEV_ERRID_SUCCESS if successful,
 *        - Return OSA_DEV_ERRID_INVALID_PARAM if the parameter is invalid.
 */
qosa_dev_error_e qosa_dev_get_chip_type(char *chip_type, qosa_int32_t *len);

/**
 * @brief Get module chip manufacturer
 *
 * @param[out] char *chip_manufacturer
 *           - Pointer to a character array for storing the module chip manufacturer.
 *
 * @param[in/out] qosa_int32_t *len
 *           - Pointer to an integer used to specify the length of the chip_manufacturer buffer when called,
 *            and store the actual buffer length used when returning.
 *
 * @return qosa_dev_error_e
 *        - Return error code, return OSA_DEV_ERRID_SUCCESS if successful,
 *        - Return OSA_DEV_ERRID_INVALID_PARAM if the parameter is invalid.
 */
qosa_dev_error_e qosa_dev_get_chip_manufacturer(char *chip_manufacturer, qosa_int32_t *len);

/**
 * @brief Get module SVN information
 *
 * @param[out] qosa_uint8_t *svn
 *           - Pointer to a character array for storing svn information.
 *
 * @return qosa_dev_error_e
 *        - Return error code, return OSA_DEV_ERRID_SUCCESS if successful,
 *        - Return OSA_DEV_ERRID_INVALID_PARAM if the parameter is invalid.
 */
qosa_dev_error_e qosa_dev_get_svn(char *svn, qosa_int32_t *len);

/**
 * @brief Get module chip temperature
 *
 * @param[out] qosa_int32_t * temp_num
 *          - Return module chip temperature value
 *
 * @return qosa_dev_error_e
 *        - Return error code, return OSA_DEV_ERRID_SUCCESS if successful,
 *        - Return OSA_DEV_ERRID_INVALID_PARAM if the parameter is invalid.
 */
qosa_dev_error_e qosa_dev_get_temp_value(qosa_int32_t *temp_num);

/**
 * @brief Get the chip ID of the module
 *
 * @param[out] qosa_uint64_t *chip_id
 *          - Corresponding CPU ID information to set
 *
 * @return qosa_dev_error_e
 *        - Return error code, return OSA_DEV_ERRID_SUCCESS if successful,
 *        - Return OSA_DEV_ERRID_INVALID_PARAM if the parameter is invalid.
 */
qosa_dev_error_e qosa_dev_get_cpu_uid(qosa_uint64_t *chip_id);

/**
 * @brief Get SD mount status.
 *
 * @param[in] qosa_uint8_t sdmmc_part
 *           - SD partition.
 *
 * @param[out] qosa_int64_t *log_size
 *           - Log size saved to the first partition of the SD card.
 *
 * @return qosa_dev_error_e
 *        - SD mount successful, return OSA_DEV_ERRID_SUCCESS,
 *        - SD not mounted, return OSA_DEV_ERRID_GENERAL.
 */

qosa_dev_error_e qosa_dev_get_sdmmc_is_mount(qosa_int32_t sdmmc_part, qosa_int64_t *log_size);

/**
 *
 * @brief Reset file system
 *
 * @return qosa_bool_t
 *         - Return OSA_TRUE for success, OSA_FALSE for failure
 *
 */
qosa_bool_t qosa_dev_filesystem_reset(void);

/**
 *
 * @brief Watchdog feed
 *
 */
void qosa_dev_watch_dog_update(void);

/**
 * @brief Get the current PLMN short name of the module.
 *
 * @param[in] qosa_uint8_t simid
 *           - SIM card number to set.
 *
 * @param[out] char *short_name
 *           - Pointer to char type, get the current PLMN short name.
 *
 * @param[in] qosa_int32_t len
 *           - Pointer to an integer, used to specify the length of the apn buffer when called, does not return the actual written length.
 *
 * @return qosa_dev_error_e
 *        - Return error code, return OSA_DEV_ERRID_SUCCESS if successful,
 *        - Return OSA_DEV_ERRID_INVALID_PARAM if the parameter is invalid.
 */
qosa_dev_error_e qosa_dev_get_plmn_short_name_sync(qosa_uint8_t simid, char *short_name, qosa_int32_t len);

/**
 * @brief Get the production mode of the module.
 *
 * @return qosa_boot_t
 *        - Return the production mode of the module.
 */
qosa_bool_t qosa_dev_get_prod_mode(void);


/**
 * @brief Query dump capture switch
 *
 * @param[in] qosa_dev_dumpcfg_e *dumpcfg
 *           - dumpcfg attribute.
 *
 * @return qosa_dev_error_e
 *        - Return QOSA_DEV_ERRID_SUCCESS if successful
 *        - Return corresponding error code if failed
 */
qosa_dev_error_e qosa_dev_get_dumpcfg(qosa_dev_dumpcfg_e *dumpcfg);

/**
 * @brief Configure dump capture switch
 *
 * @param[in] qosa_dev_dumpcfg_e dumpcfg
 *           - dumpcfg capture attribute.
 *
 * @return qosa_dev_error_e
 *        - Return QOSA_DEV_ERRID_SUCCESS if successful
 *        - Return corresponding error code if failed
 *
 * @note Not all platforms or models support all dump saving methods, actual support needs to be confirmed according to the return value
 */
qosa_dev_error_e qosa_dev_set_dumpcfg(qosa_dev_dumpcfg_e dumpcfg);

/**
 * @brief Check whether the dump occurred. This is only valid when dumpcfg is set to 3.
 *
 * @return qosa_bool_t
 *        - Returning QOSA_TRUE indicates that the dump has occurred.
 *        - Returning QOSA_FALSE indicates that the dump has not occurred.
 */
qosa_bool_t qosa_dev_dump_occure_check(void);

/**
 * @brief Query IP packet log capture percentage
 *
 * @param[in] qosa_uint32_t *percent
 *           - Percentage of packet log printing, in %, minimum is 0 to close, maximum is 100 to capture all.
 * @return qosa_dev_error_e
 *        - Return QOSA_DEV_ERRID_SUCCESS if successful
 *        - Return corresponding error code if failed
 */
qosa_dev_error_e qosa_dev_get_iptrace(qosa_uint32_t *percent);

/**
 * @brief Configure IP packet log capture percentage
 *
 * @param[in] qosa_uint32_t percent
 *           - IP packet log printing percentage, in %, minimum is 0 to close, maximum is 100 to print all.
 * @return qosa_dev_error_e
 *        - Return QOSA_DEV_ERRID_SUCCESS if successful
 *        - Return corresponding error code if failed
 */
qosa_dev_error_e qosa_dev_set_iptrace(qosa_uint32_t percent);

/**
 * @brief Set the application running start address It should be noted that when A upgrades B and saves the startup address for the next time,
          flash writing is required. Therefore, the frequency should not be too high to prevent the flash from being damaged
 *
 * @param[in] qos_app_flash_start_addr
 *             - The flash memory start address where the application is stored
 * @param[in] qos_app_ram_start_addr
 *             - The RAM start address where the application will be loaded and executed
 *
 * @return void
 */

void qosa_set_app_runing_start_addr(qosa_uint32_t qos_app_flash_start_addr, qosa_uint32_t qos_app_ram_start_addr);

/*===========================================================================
 * CPU usage monitoring unified API
 *
 * Cross-platform CPU loading measurement.
 *   period_ms range: 200 ~ 60000 (ms)
 *   The callback is invoked from non-interrupt context (worker task).
 *   If called again while already running, the previous monitoring is
 *   stopped first and a new one starts (last-call-wins).
 *
 * Error codes:
 *   QOSA_CPU_USAGE_OK              Success
 *   QOSA_CPU_USAGE_ERR_PARAM       Invalid parameter
 *   QOSA_CPU_USAGE_ERR_RESOURCE    Insufficient resources
 *   QOSA_CPU_USAGE_ERR_NOT_SUPPORT Platform not supported
 ===========================================================================*/

/** Period range constraints (ms) */
#define QOSA_CPU_USAGE_PERIOD_MIN_MS    200U
#define QOSA_CPU_USAGE_PERIOD_MAX_MS    60000U

/** Error codes */
typedef enum
{
    QOSA_CPU_USAGE_OK              = 0,   /*!< Success */
    QOSA_CPU_USAGE_ERR_PARAM       = -1,  /*!< Invalid parameter */
    QOSA_CPU_USAGE_ERR_RESOURCE    = -2,  /*!< Insufficient resources */
    QOSA_CPU_USAGE_ERR_NOT_SUPPORT = -3,  /*!< Platform not supported */
} qosa_cpu_usage_errcode_e;

/**
 * @brief CPU usage callback function type
 *
 * @param[in] cpu_usage_pct  CPU usage percentage for the current period (0-100)
 */
typedef void (*qosa_cpu_usage_cb_t)(qosa_uint8_t cpu_usage_pct);

/**
 * @brief Start CPU usage periodic monitoring
 *
 * Starts a periodic timer. At the end of each period, the CPU usage percentage
 * for that period is reported through the callback. If called again while already
 * running, the previous monitoring is stopped first and a new one starts.
 *
 * @param[in] period_ms  Monitoring period in milliseconds, range: 200~60000
 * @param[in] cb         Callback function pointer, must not be NULL.
 *                       The callback is invoked from non-interrupt context.
 *
 * @return  QOSA_CPU_USAGE_OK           Success
 * @return  QOSA_CPU_USAGE_ERR_PARAM    Invalid parameter (period out of range or cb is NULL)
 * @return  QOSA_CPU_USAGE_ERR_RESOURCE Insufficient resources (timer / task creation failed)
 * @return  QOSA_CPU_USAGE_ERR_NOT_SUPPORT  Platform does not support this feature
 */
qosa_int32_t qosa_cpu_usage_start(qosa_uint32_t period_ms, qosa_cpu_usage_cb_t cb);

/**
 * @brief Stop CPU usage monitoring and release all resources
 *
 * Idempotent: safe to call even if monitoring is not running.
 *
 * @return  QOSA_CPU_USAGE_OK on success
 */
qosa_int32_t qosa_cpu_usage_stop(void);

#endif// !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif /* __QOSA_DEV_H__ */
