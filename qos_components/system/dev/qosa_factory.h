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
#ifndef __QOSA_FACTORY_H__
#define __QOSA_FACTORY_H__

#include "qosa_def.h"

#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 

#include "qosa_sys.h"
#include "qosa_dev.h"

/**
 * @brief Set the IMEI for the corresponding SIM card number
 *
 * @param[in] qosa_uint8_t simid
 *          - SIM card number to set
 *
 * @param[in] char * imei_str
 *          - Corresponding IMEI string
 *
 * @return qosa_dev_error_e
 *        - Return error code, return OSA_DEV_ERRID_SUCCESS if successful,
 *        - Return OSA_DEV_ERRID_INVALID_PARAM if the parameter is invalid.
 */
qosa_dev_error_e qosa_dev_set_sim_imei(qosa_uint8_t simid, char *imei_str);

/**
 * @brief Set the SN number information of the module
 *
 * @param[in] char * sn_str
 *          - Corresponding SN string information to set
 *
 * @return qosa_dev_error_e
 *        - Return error code, return OSA_DEV_ERRID_SUCCESS if successful,
 *        - Return OSA_DEV_ERRID_INVALID_PARAM if the parameter is invalid.
 */
qosa_dev_error_e qosa_dev_set_sn(char *sn_str);

/**
 * @brief Get the SN2 number information of the module
 *
 * @param[in] char * SN2
 *          - SN2 string information to get
 *
 * @param[in] qosa_int32_t * len
 *          - SN2 string length, Fixed at 32 bytes.
 *
 * @return qosa_dev_error_e
 *        - Return error code, return OSA_DEV_ERRID_SUCCESS if successful,
 *        - Return OSA_DEV_ERRID_INVALID_PARAM if the parameter is invalid.
 */
qosa_dev_error_e qosa_dev_get_sn2(char *SN2, qosa_int32_t *len);

/**
 * @brief Set the SN2 number information of the module
 *
 * @param[in] char * sn2_str
 *          - Corresponding SN2 string information to set
 *
 * @return qosa_dev_error_e
 *        - Return error code, return OSA_DEV_ERRID_SUCCESS if successful,
 *        - Return OSA_DEV_ERRID_INVALID_PARAM if the parameter is invalid.
 */
qosa_dev_error_e qosa_dev_set_sn2(char *sn2_str);

/**
 * @brief Set the production mode of the module.
 *
 * @param[in] qosa_bool_t prod_mode
 *           - Pointer to osa_bool_t type, used to set the current production mode.
 *
 * @return qosa_dev_error_e
 *        - Return error code, return OSA_DEV_ERRID_SUCCESS if successful,
 *        - Return OSA_DEV_ERRID_INVALID_PARAM if the parameter is invalid.
 */
qosa_dev_error_e qosa_dev_set_prod_mode(qosa_bool_t prod_mode);

#endif//!(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif /* __QOSA_FACTORY_H__ */
