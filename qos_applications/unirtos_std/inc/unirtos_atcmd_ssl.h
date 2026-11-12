/*****************************************************************/ /**
* @file unirtos_atcmd_ssl.h
* @brief
* @author larson.li@quectel.com
* @date 2023-08-22
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2023-08-22 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#ifndef __UNIRTOS_ATCMD_SSL_H__
#define __UNIRTOS_ATCMD_SSL_H__

#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_at_param.h"
#ifdef CONFIG_QCM_VTLS_FUNC
#include "qcm_vtls_cfg.h"
#endif /* CONFIG_QCM_VTLS_FUNC */
//SSL config number of configuration items
#define UNIR_SSL_CTX_ID_MIN 0
#define UNIR_SSL_CTX_ID_MAX 5

qcm_ssl_config_t *unir_get_ssl_config(qosa_uint8_t ctx_id);

void qstd_exec_ssl_qsslopen_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_ssl_qsslstate_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_ssl_qsslsend_cmd_func(qosa_at_cmd_t *cmd);
void qstd_exec_ssl_qsslrecv_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_ssl_qsslclose_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_ssl_qsslopenex_cmd(qosa_at_cmd_t *cmd);
void qstd_ssl_qsslcfg_cmd_func(qosa_at_cmd_t *cmd);

/**
 * Checks if the specified cipher suite is valid
 *
 * @param int cs_id The ID of the cipher suite
 * @return Returns OSA_TRUE if the cipher suite is valid, otherwise returns OSA_FALSE
 */
qosa_bool_t unir_ssl_ciphersuit_is_valid(int cs_id);

#ifdef CONFIG_QCM_QURL_FUNC
#include "qurl.h"
/**
 * @brief Releases dynamically allocated memory resources for QURL TLS configuration
 *
 * This function iterates through the members of the TLS configuration structure. If it finds any pointing to non-NULL dynamically allocated memory, it frees that memory.
 * Finally, the function also clears the entire TLS configuration structure.
 *
 * @param tls_cfg_ptr A pointer to the qurl_tls_cfg_t structure, which contains the configuration information required for a TLS connection.
 */
void unir_qurl_tls_cfg_free(qurl_tls_cfg_t *tls_cfg_ptr);

/**
 * @brief Convert UniRTOS TLS configuration to QURL TLS configuration
 *
 * @param  qurl_tls_cfg_t *tls_cfg_ptr
 *         - Pointer to a qurl_tls_cfg_t structure, which contains the configuration information required for a TLS connection.
 *
 * @param  qcm_ssl_config_t *qcm_ssl_cfg_ptr
 *         - Pointer to a qcm_ssl_config_t structure, which contains the configuration information required for a UniRTOS TLS connection.
 *
 * @return qosa_common_error_e
 *        - Returns OSA_OK on success.
 *        - Returns other values on failure.
 */
qosa_common_error_e unir_tls_cfg_unirtos_to_qurl(qurl_tls_cfg_t *tls_cfg_ptr, qcm_ssl_config_t *qcm_ssl_cfg_ptr);

#endif /* CONFIG_QCM_QURL_FUNC */

#endif /* __UNIRTOS_ATCMD_SSL_H__ */
