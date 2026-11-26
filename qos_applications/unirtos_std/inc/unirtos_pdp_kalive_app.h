/*****************************************************************/ /**
* @file unirtos_pdp_kalive_app.h
* @brief
* @author larson.li@quectel.com
* @date 2024-05-29
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2024-05-29 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#ifndef __UNIRTOS_NC_KEEPALIVE_APP_H__
#define __UNIRTOS_NC_KEEPALIVE_APP_H__

#include "qosa_def.h"
#include "qosa_sys.h"

/** Configure the startup type as IPv4 configuration address */
#define OSA_PDP_KALIVE_IPV4          1
/** Configure the startup type as IPv6 configuration address */
#define OSA_PDP_KALIVE_IPV6          2

/** pdp keepalive error code base */
#define OSA_PDP_KEEP_ERRCODE_OS_BASE (QOSA_COMPONENT_NC_KEEP << 16)

typedef enum
{
    PDP_KEEP_RESULT_OK = 0,
    PDP_KEEP_FAILED_INIT_ERR = 1 | OSA_PDP_KEEP_ERRCODE_OS_BASE, /*!< init initialization error */
    PDP_KEEP_FAILED_PARAM_ERR,                                   /*!< Parameter input error */

    PDP_KEEP_RESULT_MAX
} qosa_pdp_keep_err_status_e;

/**
 * @struct qstd_pdp_kalive_opt
 * @brief  Used for managing PDP keepalive functionality
 */
typedef struct
{
    int           option_enable;    /*!< Configure whether the option is enabled */
    int           keepalive_enable; /*!< Configure whether the PDP keepalive function is enabled */
    int           pdp_cid;          /*!< Configure the PDP CID function to keep alive */
    int           sim_id;           /*!< Configure the SIM SID to be kept alive */
    qosa_uint32_t idle_time;        /*!< Configure the keep-alive period, unit: second(s) */
} qstd_pdp_kalive_opt;

/**
 * @brief Retrieves the current default configuration startup information for external use.
 *
 * @param[out] qstd_tcpip_nc_kalive_opt * opt_val
 *          -   Corresponds to the existing configuration information.
 *
 * @return qosa_pdp_keep_err_status_e
 *        - Returns NC_KEEP_RESULT_OK on success.
 *        - Returns other values on failure.
 */
qosa_pdp_keep_err_status_e qstd_get_pdp_kalive(qstd_pdp_kalive_opt *opt_val);

/**
 * @brief Set startup PDP keepalive information
 *
 * @param[in] qstd_tcpip_pdp_kalive_opt * opt_val
 *          - The PDP keepalive startup parameters to be configured
 *
 * @return qosa_pdp_keep_err_status_e
 *        - Returns NC_KEEP_RESULT_OK on success
 *        - Returns other values on failure
 */
qosa_pdp_keep_err_status_e qstd_set_pdp_kalive(qstd_pdp_kalive_opt *opt_val);

/**
 * @brief Get the IP address configuration information for PDP keepalive
 *
 * @param[in] int ip_type
 *          - Corresponds to OSA_PDP_KALIVE_IPV4 or OSA_PDP_KALIVE_IPV6
 *
 * @param[out] char * * pdp_kalive_ip_addr
 *           - The obtained IP address information, as a plain text string
 *
 * @param[out] int * pdp_kalive_ip_remote
 *           - The obtained remote port information
 *
 * @return qosa_pdp_keep_err_status_e
 *        - Returns NC_KEEP_RESULT_OK on success
 *        - Returns other values on failure
 */
qosa_pdp_keep_err_status_e qstd_pdp_kalive_config_get_opt(int ip_type, char **pdp_kalive_ip_addr, int *pdp_kalive_ip_remote);

/**
 * @brief Set user-defined PDP keepalive IP address configuration information
 *
 * @param[in] int ip_type
 *          - Corresponds to OSA_PDP_KALIVE_IPV4 or OSA_PDP_KALIVE_IPV6
 *
 * @param[in] const char * ip_str
 *          - The remote IP address string to set
 *
 * @param[in] int remote_port
 *          - The configured remote port
 *
 * @return qosa_pdp_keep_err_status_e
 *        - Returns NC_KEEP_RESULT_OK on success
 *        - Returns other values on failure
 */
qosa_pdp_keep_err_status_e qstd_pdp_kalive_ip_config_set_opt(int ip_type, const char *ip_str, int remote_port);

#endif /* __UNIRTOS_NC_KEEPALIVE_APP_H__ */
