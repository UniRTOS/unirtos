/*****************************************************************/ /**
* @file qosa_fota.h
* @brief
* @author felix.huang@quectel.com
* @date 2025-09-16
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2025-09-16 <td>1.0 <td>felix.huang <td> Init
* </table>
**********************************************************************/
#ifndef __QOSA_FLASH_H__
#define __QOSA_FLASH_H__

#include "qosa_def.h"
#include "qosa_sys.h"

// Flash erase block size definition (unit: byte)
#define QOSA_ERASE_SIZE_4K    (4 * 1024)
#define QOSA_ERASE_SIZE_32K   (32 * 1024)
#define QOSA_ERASE_SIZE_64K   (64 * 1024)
#define QOSA_ERASE_SIZE_128K  (128 * 1024)

#define QOSA_SHIFT_BIT_NUM_4K (12) /*!< 4K block size shift bit number: 4096 = 1 << 12 */
/**
 * @enum qosa_flash_err_t
 * @brief Flash erase operation error code enumeration
 *
 * Defines various error codes that may be returned by Flash erase operations
 */
typedef enum
{
    QOSA_FLASH_OK = 0, /*!< Operation successful */

    /* General errors (1-99) */
    QOSA_FLASH_ERR_INVALID_ADDR = 1,  /*!< Invalid Flash address */
    QOSA_FLASH_ERR_INVALID_PARAM = 2, /*!< Invalid parameter */
    QOSA_FLASH_ERR_NOT_ALIGNED = 3,   /*!< Address not aligned to sector boundary */
    QOSA_FLASH_ERR_OUT_OF_RANGE = 4,  /*!< Operation range exceeds partition boundary */
    QOSA_FLASH_ERR_TIMEOUT = 5,       /*!< Operation timeout */
    QOSA_FLASH_ERR_BUSY = 6,          /*!< Flash busy, unable to perform operation */
    QOSA_FLASH_ERR_NO_MEMORY = 7,     /*!< Insufficient memory */
    QOSA_FLASH_ERR_NOT_INIT = 8,      /*!< Flash not initialized */
    QOSA_FLASH_ERR_HARDWARE = 9,      /*!< Hardware error */
    QOSA_FLASH_ERR_MALLOC_FAIL = 10,  /*!< Memory allocation failed */

    /* Partition-related errors (100-199) */
    QOSA_FLASH_ERR_PARTITION_NOT_FOUND = 100, /*!< Corresponding partition not found */
    QOSA_FLASH_ERR_PARTITION_LOCKED = 101,    /*!< Partition locked, cannot operate */
    QOSA_FLASH_ERR_ACCESS_DENIED = 102,       /*!< Partition access denied */

    /* Operation-related errors (200-299) */
    QOSA_FLASH_ERR_OPERATION_FAIL = 200, /*!< Operation failed */
} qosa_flash_err_e;

/**
 * @struct qosa_flash_count_info_t
 * @brief QOSA flash block statistics information structure
 *
 * Used to store flash block size distribution statistics related information
 */
typedef struct
{
    qosa_uint16_t  block_count;       /*!< Total number of Flash blocks (4k as 1 block)    */
    qosa_uint32_t  all_count;         /*!< Total erase/write count of all blocks */
    qosa_uint16_t *block_count_array; /*!< Pointer to array for counting erase/write operations per block, categorized by block sequence number in flash */
    qosa_uint16_t  reserved[3];       /*!< Reserved field for future feature expansion */
} qosa_flash_count_info_t;

/**
 * @struct qosa_flash_stress_info_t
 * @brief QOSA flash stress test information structure
 *
 * Used to store parameters and status information related to flash stress testing
 */
typedef struct
{
    qosa_uint32_t urc_channel : 4;  /*!< unirtos urc channel parameter */
    qosa_uint32_t erase_type  : 2;  /*!< Erase type, corresponding to qosa_erase_size_e enumeration value */
    qosa_uint32_t count       : 10; /*!< Test loop counter */
    qosa_uint32_t write_val   : 8;  /*!< Data written to flash */
    qosa_uint32_t is_running  : 1;  /*!< Test running status flag: 1-running, 0-stopped */
    qosa_uint32_t reserved    : 7;  /*!< Reserved bits */
    qosa_uint32_t test_enabled;     /*!< Test enable flag: 1-enable test, 0-disable test */
    qosa_uint16_t test_interval;    /*!< Test interval time */
    qosa_uint16_t urc_threshold;    /*!< URC reporting threshold */
    qosa_uint32_t start_address;    /*!< Start address for erase/write operations */
    qosa_uint32_t max_count;        /*!< Maximum test loop count */
    qosa_uint32_t test_count;       /*!< Current completed test loop count */
    qosa_uint16_t Erase_time;       /*!< Erase operation duration */
    qosa_uint16_t Read_time;        /*!< Read operation duration */
    qosa_uint16_t Write_time;       /*!< Write operation duration */
} qosa_flash_stress_info_t;

/**
 * @enum flash_test_resp_cmd_e
 * @brief Used as ID when Flash test task sends messages to center task, indicating URC report type
 *
 */
typedef enum
{
    FLASH_TEST_RESP_START, /*!< Flash stress test start */
    FLASH_TEST_RESP_STATE, /*!< Flash stress test reports current test status */
    FLASH_TEST_RESP_ERR,   /*!< Flash stress test encountered error */
    FLASH_TEST_RESP_END,   /*!< Flash stress test end */
    FLASH_TEST_RESP_MAX
} flash_test_resp_cmd_e;

/**
 * @brief Message body for flash stress test task to send to AT task, used for URC reporting
 */
typedef struct
{
    flash_test_resp_cmd_e cmd_id;
    void                 *argv;
} qosa_flash_test_event_cmd_info;

/**
  * @struct qosa_flash_partition_t
  * @brief Flash partition descriptor
  */
typedef struct
{
    const char   *name;       /*!< Partition name */
    qosa_uint32_t start_addr; /*!< Partition start address */
    qosa_uint32_t length;     /*!< Partition size in bytes */
    qosa_bool_t   writable;   /*!< Read-only/protected or not: QOSA_TRUE read-only/protected, QOSA_FALSE writable*/
} qosa_flash_partition_t;

/**
 * @struct qosa_flash_fbp_status_t
 * @brief Flash FBP descriptor
 */
typedef struct
{
    qosa_uint32_t fbp_flag;   /*!< FBP enable flag */
    qosa_uint32_t start_addr; /*!< FBP start address */
    qosa_uint32_t end_addr;   /*!< FBP end address */
} qosa_flash_fbp_status_t;

/**
 * @brief Get Flash erase/write count statistics information
 *
 * This function is used to obtain global Flash erase/write operation count statistics information, including block size distribution,
 * various operation counts and other detailed information. Mainly used for test result query and performance analysis.
 *
 * @return qosa_flash_count_info_t* Pointer to the Flash erase/write count information structure
 *
 */
qosa_flash_count_info_t *qosa_get_flash_erase_count(void);

/**
 * @brief Initialize LFS erase statistics tracking and register the flash operation callback.
 *
 * @return int Operation execution result.
 * @retval QOSA_FLASH_OK Initialization succeeded or was already completed.
 * @retval Other error codes Initialization failed.
 */
int qosa_flash_erase_count_init(void);

/**
 * @brief Start Flash stress test task
 *
 * This function is used to create and start the Flash stress test task. If the task already exists, it will not be created again.
 *
 * @param[in] param Pointer to stress test parameters containing test configuration information
 *
 * @return int Operation execution result
 * @retval QOSA_FLASH_OK Task creation successful or task already exists
 * @retval Other error codes Task creation failed
 *
 */
int qosa_flash_stress_test_start(qosa_uint32_t *param);

/**
 * @brief Get the total number of configured Flash partitions in the system
 *
 * @return qosa_size_t Total number of partitions.
 *         If returns 0, indicates no valid partition table found or an error occurred.
 */
qosa_size_t qosa_flash_partition_get_count(void);

/**
 * @brief Get Flash partition information by index into caller-provided buffer
 * @param index Partition index number
 * @param out_part Output parameter, pointer to user-allocated partition information structure
 * @return Success: 0;
 *         Failure: Error code
 */
int qosa_flash_partition_get_index(qosa_size_t index, qosa_flash_partition_t *out_part);

/**
 * @brief Get Flash partition information by partition name into caller-provided buffer
 * @param name Partition name
 * @param out_part Output parameter, pointer to user-allocated partition information structure
 * @return Success: 0;
 *         Failure: Error code
 */
int qosa_flash_partition_get_name(const char *name, qosa_flash_partition_t *out_part);

/**
 * @brief Get Flash FBP status and address information
 * @param fbp_flag Output parameter, pointer to FBP status flag
 * @param fbp_address Output parameter, pointer to FBP address structure
 * @return Success: QOSA_FLASH_OK;
 *         Failure: Error code
 */
qosa_int32_t qosa_flash_fbp_status_get(qosa_flash_fbp_status_t *fbp_address);

/**
 * @brief Set Flash FBP status
 * @param enable FBP status flag
 * @return Success: QOSA_FLASH_OK;
 *         Failure: Error code
 */
qosa_int32_t qosa_flash_fbp_enable_set(qosa_bool_t enable);

/**
 * @brief Enable/disable software-layer erase/write protection for flash
 *
 * @param address Flash address or sector number to control protection for
 * @param size    Size of the region to protect
 * @param enable  TRUE - enable protection, FALSE - disable protection
 * @return qosa_int32_t Operation status code
 * @retval QOSA_FLASH_OK Operation successful
 * @retval Other error codes Operation failed
 */
qosa_int32_t qosa_flash_Protection_interface_set(qosa_uint32_t address, qosa_size_t size, qosa_bool_t enable);

/**
 * @brief Get the enable/disable status of flash software interface write protection
 *
 * @param address Flash address to query protection status
 * @param size    Size of the region to query
 * @param enable  Output parameter, pointer to boolean variable to store protection status
 *                TRUE - Protection enabled, FALSE - Protection disabled
 * @return qosa_int32_t Operation status code
 * @retval QOSA_FLASH_OK Operation successful
 * @retval Other error codes Operation failed
 */
qosa_int32_t qosa_flash_Protection_interface_get(qosa_uint32_t address, qosa_size_t size, qosa_bool_t *enable);

/**
 * @brief The test AT command interface for the differential compensation function is implemented on the platform side.
 * 
 * @param partition_name The partition name to be tested
 * @param type           0 erases by sector, 1 erases by block.  
 * @param erase_num      Number of blocks/sectors to erase
 * 
 * @return Returns QOSA_TRUE on success, QOSA_FALSE on failure.
 * 
 */
qosa_bool_t qosa_flash_repair_erase_test(const char * partition_name, qosa_uint8_t type, qosa_uint32_t erase_num);

/**
 * @brief Get the flash Quad Enable (QE) status. Implemented on the platform side.
 *
 * @return QOSA_TRUE - QE enabled, QOSA_FALSE - QE disabled.
 */
qosa_bool_t qosa_flash_qe_get(void);

/**
 * @brief Set the flash Quad Enable (QE) status, with read-back verification.
 *
 * @param enable QOSA_TRUE to enable QE, QOSA_FALSE to disable QE.
 * @return QOSA_TRUE on success (read-back confirmed), QOSA_FALSE on failure.
 */
qosa_bool_t qosa_flash_qe_set(qosa_bool_t enable);

#endif
