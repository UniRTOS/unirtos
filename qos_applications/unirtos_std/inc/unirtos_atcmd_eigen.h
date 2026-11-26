/*****************************************************************/ /**
* @file unirtos_atcmd_qupcheck.h
* @brief
* @author jayden.zhu@quectel.com
* @date 2025-10-29
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2025-10-29 <td>1.0 <td>jayden.zhu <td> Init
* </table>
**********************************************************************/
#ifndef __UNIRTOS_ATCMD_QUPCHECK_H__
#define __UNIRTOS_ATCMD_QUPCHECK_H__

#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_at_param.h"

/*========================================================================
*  Type Definition
*========================================================================*/

typedef struct
{
    qosa_uint8_t  version[16];
    qosa_uint8_t  build_time[64];
    qosa_uint32_t file_len;
    qosa_uint32_t crc;
    qosa_uint32_t reserved[128];
} qosa_eigen_updater_info;

void qstd_exec_qupcheck_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qupdater_cmd(qosa_at_cmd_t *cmd);

#ifdef CONFIG_QAPP_UNIRTOS_AT_QGPIOV_CMD
void qstd_exec_qgpiov_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_AT_QGPIOV_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QCFEM_CMD
void qstd_exec_qcfem_cmd(qosa_at_cmd_t *cmd);
void qstd_qcfem_boot_at_init(void);
#endif /* CONFIG_QAPP_UNIRTOS_AT_QCFEM_CMD */

#endif /* __UNIRTOS_ATCMD_QUPCHECK_H__ */
