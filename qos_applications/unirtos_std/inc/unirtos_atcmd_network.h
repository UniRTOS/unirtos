/*****************************************************************/ /**
* @file unirtos_atcmd_network.h
* @brief
* @author Joe.tu@quectel.com
* @date 2024-03-18
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description
* <tr><td>2024-04-16 <td>1.0 <td>Joe.tu <td> Init
* </table>
**********************************************************************/
#ifndef __UNIR_ATCMD_NETWORK_H__
#define __UNIR_ATCMD_NETWORK_H__

#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_at_param.h"
#include "qosa_at_config.h"
#include "qosa_network.h"

#ifdef OSA_AT_CREG_RPT_MODE_GET
/** +CREG command's n parameter */
#define CREG_RPT_MODE_GET(x)      OSA_AT_CREG_RPT_MODE_GET(x)
#define CREG_RPT_MODE_SET(x, y)   OSA_AT_CREG_RPT_MODE_SET(x, y)

/** n parameter of the +CGREG command */
#define CGREG_RPT_MODE_GET(x)     OSA_AT_CGREG_RPT_MODE_GET(x)
#define CGREG_RPT_MODE_SET(x, y)  OSA_AT_CGREG_RPT_MODE_SET(x, y)

/** +CEREG command's n parameter */
#define CEREG_RPT_MODE_GET(x)     OSA_AT_CEREG_RPT_MODE_GET(x)
#define CEREG_RPT_MODE_SET(x, y)  OSA_AT_CEREG_RPT_MODE_SET(x, y)

/** +C5GREG command's n parameter */
#define C5GREG_RPT_MODE_GET(x)    OSA_AT_C5GREG_RPT_MODE_GET(x)
#define C5GREG_RPT_MODE_SET(x, y) OSA_AT_C5GREG_RPT_MODE_SET(x, y)
#endif /* OSA_AT_CREG_RPT_MODE_GET */

/**
 * @enum unir_nw_ctzu_mode_e
 * @brief Mode for NITZ automatic time zone update
 *
 */
typedef enum
{
    CTZU_MODE_DISABLE = 0,   /*!< Disable NITZ automatic time zone update */
    CTZU_MODE_ENABLE = 1,    /*!< Enable NITZ automatic time zone update */
    CTZU_MODE_ENABLE_RTC = 3 /*!< Enable NITZ automatic time zone update and update local time to system time */
} unir_nw_ctzu_mode_e;

/**
 * @enum unir_nw_ctzr_mode_e
 * @brief URC mode for reporting time zone changes
 *
 */
typedef enum
{
    CTZR_URC_DISABLE = 0,    /*!< Disable CTV time zone reporting */
    CTZR_URC_ENABLE = 1,     /*!< Enable CTZV time zone reporting */
    CTZR_URC_ENABLE_CTZE = 2 /*!< Enable CTZE extension time zone reporting */
} unir_nw_ctzr_mode_e;

/**
 * @enum qosa_nw_qceer_mode_e
 * @brief QCEER Command Mode
 *
 */
typedef enum
{
    QOSA_QCEER_MODE_CACHE_NO_URC = 0,    /*!< Do not clear rejection code after query + URC reporting disabled */
    QOSA_QCEER_MODE_CACHE_URC = 1,       /*!< Do not clear the rejection code after query + URC reporting enabled */
    QOSA_QCEER_MODE_NO_CACHE_NO_URC = 2, /*!< Query and clear rejection code + URC reporting disabled */
    QOSA_QCEER_MODE_NO_CACHE_URC = 3,    /*!< Query and clear rejection code + URC reporting enabled */
} qosa_nw_qceer_mode_e;

qosa_uint8_t         unir_nw_get_ctzu(qosa_uint8_t simid);
void                 unirtos_get_reject_info_param(qosa_uint8_t simid, qosa_nw_reject_info_t *reject_info);
void                 unirtos_update_reject_info_param(qosa_uint8_t simid, qosa_nw_reject_info_t *reject_info);
void                 unirtos_nw_set_qceer_mode(qosa_uint8_t simid, qosa_uint8_t mode);
qosa_nw_qceer_mode_e unirtos_nw_get_qceer_mode(qosa_uint8_t simid);

void qstd_exec_creg_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_cgreg_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_cereg_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_c5greg_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qnwinfo_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qeng_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qcell_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qcellex_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qcellinfo_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_csq_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_cesq_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qcsq_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_ctzr_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_ctzu_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_cops_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qlts_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_copn_cmd(qosa_at_cmd_t *cmd);
// void qstd_exec_cpls_cmd(qosa_at_cmd_t *cmd);
// void qstd_exec_cpol_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_cscon_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qops_cmd(qosa_at_cmd_t *cmd);
#ifdef CONFIG_QOSA_SIM_FUNC
void qstd_exec_qspn_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QOSA_SIM_FUNC */
#ifdef CONFIG_QAPP_UNIRTOS_EDRX_AT_FUNC
void qstd_exec_cedrxs_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_cedrxrdp_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qptwedrxs_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_EDRX_AT_FUNC */
// void qstd_exec_qopscfg_cmd(qosa_at_cmd_t *cmd);
// void qstd_exec_cvmod_cmd(qosa_at_cmd_t *cmd);
// void qstd_exec_ceus_cmd(qosa_at_cmd_t *cmd);
 void qstd_exec_cemode_cmd(qosa_at_cmd_t *cmd);
 void qstd_exec_cgerep_cmd(qosa_at_cmd_t *cmd);
// void qstd_exec_qnwcfg_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qnwlock_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qscan_cmd(qosa_at_cmd_t *cmd);
#ifdef CONFIG_QAPP_UNIRTOS_AT_QSCANEX_CMD
void qstd_exec_qscanex_cmd(qosa_at_cmd_t *cmd);
#endif
#ifdef CONFIG_QAPP_UNIRTOS_AT_QANTINFO_CMD
void qstd_exec_qantinfo_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_AT_QANTINFO_CMD */
void qstd_exec_qband_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_c5gnssai_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_c5gnssairdp_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qceer_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_ceer_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qautoapn_cmd(qosa_at_cmd_t *cmd);

void qstd_exec_qblackcell_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qblackcellcfgex_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qblackcellweight_cmd(qosa_at_cmd_t *cmd);

#endif /* __UNIR_ATCMD_NETWORK_H__ */
