/*****************************************************************/ /**
* @file qosa_nvitem.h
* @brief NVM item configuration interface
* @author larson.li@quectel.com
* @date 2023-06-25
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2023-06-25 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#ifndef __NV_ITEM_CFG_H__
#define __NV_ITEM_CFG_H__

#include "qosa_def.h"
#include "qosa_sys.h"

#define NV_ITEM_CFG_DEF(key, values)                                                                                                                           \
    {                                                                                                                                                          \
        key, &values, sizeof(values)                                                                                                                           \
    }

/** FILE API function module error base offset code */
#define QOSA_ERRCODE_NVM_CFG_BASE (QOSA_COMPONENT_NV_ITEM_CFG << 16)

/**
 * @enum qosa_nvm_error_e
 * @brief Corresponds to NVM operation API interface error result return
 */
typedef enum
{
    QOSA_NVM_CFG_OK = QOSA_OK,

    QOSA_NVM_CFG_MALLOC_ERROR = 1 | QOSA_ERRCODE_NVM_CFG_BASE, /*!< Memory allocation failed */
    QOSA_NVM_CFG_PARAM_ERROR,                                  /*!< Parameter input error */
    QOSA_NVM_CFG_PATH_ERROR,                                   /*!< Input path name does not conform to specification */
    QOSA_NVM_CFG_READ_FIEL_ERROR,                              /*!< Failed to read JSON file */
    QOSA_NVM_CFG_NO_SEARCH_ERROR,                              /*!< Corresponding item name in JSON does not exist */
    QOSA_NVM_CFG_WRITE_FIEL_ERROR,                             /*!< Failed to write JSON file */
    QOSA_NVM_CFG_UNKNOW_ERROR,                                 /*!< Unknown error */

    QOSA_NVM_CFG_ERROR_MAX
} qosa_nvm_error_e;

typedef struct
{
    char         *key;        /*!< Corresponds to key value information of JSON element */
    void         *value;      /*!< Corresponds to value information of key */
    qosa_uint16_t value_size; /*!< Corresponds to size of value information element */
} qosa_nv_cfg_item_data_t;

/**
 * @brief Per-item read status for qosa_nv_item_cfg_read_ex().
 */
typedef enum
{
    QOSA_NVM_CFG_ITEM_READ_OK = 0,       /*!< Configuration item read successfully */
    QOSA_NVM_CFG_ITEM_READ_NOT_FOUND,    /*!< Configuration item not found */
    QOSA_NVM_CFG_ITEM_READ_SIZE_MISMATCH /*!< Configuration item stored length mismatch */
} qosa_nv_cfg_item_read_status_e;

/**
 * @brief Delete corresponding item element information in the JSON file
 *
 * @param[in] char * name
 *           - Corresponding file name to operate
 *
 * @param[in] qosa_nv_cfg_item_data_t * item_data
 *           - JSON element information to operate
 *
 * @param[in] qosa_uint16_t item_count
 *           - Number of elements in itemdata array
 *
 * @return qosa_nvm_error_e
 *          - Return NVM_FILE_OK indicates successful execution
 *          - Return others indicates failure
 */
qosa_nvm_error_e qosa_nv_item_cfg_delete(char *name, qosa_nv_cfg_item_data_t *item_data, qosa_uint16_t item_count);

/**
 * @brief Save corresponding JSON item information to the JSON file
 *
 * @param[in] char * name
 *           - Corresponding file name to operate
 *
 * @param[in] qosa_nv_cfg_item_data_t * item_data
 *           - JSON element information to operate
 *
 * @param[in] qosa_uint16_t item_count
 *           - Number of elements in itemdata array
 *
 * @return qosa_nvm_error_e
 *          - Return NVM_FILE_OK indicates successful execution
 *          - Return others indicates failure
 */
qosa_nvm_error_e qosa_nv_item_json_write(char *name, qosa_nv_cfg_item_data_t *item_data, qosa_uint16_t item_count);

/**
 * @brief Read and traverse JSON information saved in the JSON file, find corresponding key value
 *
 * @param[in] char * name
 *          - File name to operate
 *
 * @param[in/out] qosa_nv_cfg_item_data_t * item_data
 *          - JSON key value information to query, if found, get corresponding key value
 *
 * @param[in] qosa_uint16_t item_count
 *          - Number of elements in item_data array
 *
 * @return qosa_nvm_error_e
 *          - Return NVM_FILE_OK indicates successful execution
 *          - Return others indicates failure
 */
qosa_nvm_error_e qosa_nv_item_cfg_read(char *name, qosa_nv_cfg_item_data_t *item_data, qosa_uint16_t item_count);

/**
 * @brief Parse JSON once and return the read status for each configuration item individually.
 *
 * When a configuration item is missing or its length does not match, this API does not
 * modify the output buffer for that item and continues reading subsequent items. As long
 * as the file, root node, and configuration node can be read normally, this API returns
 * QOSA_NVM_CFG_OK; callers check item_status for each item's success or failure.
 *
 * @param[in] name
 *          - File and node path to operate on.
 * @param[in/out] item_data
 *          - Configuration items to query and their output buffers.
 * @param[out] item_status
 *          - Array of per-item read statuses, one-to-one with item_data.
 * @param[in] item_count
 *          - Number of elements in item_data and item_status.
 * @return qosa_nvm_error_e
 *          - QOSA_NVM_CFG_OK: JSON file and target node read successfully.
 *          - Other values: file, path, parameter, or memory operation failed.
 */
qosa_nvm_error_e qosa_nv_item_cfg_read_ex(char *name,
                                           qosa_nv_cfg_item_data_t *item_data,
                                           qosa_nv_cfg_item_read_status_e *item_status,
                                           qosa_uint16_t item_count);

#endif /* __NV_ITEM_CFG_H__ */
