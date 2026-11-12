/*****************************************************************/ /**
* @file unirtos_atcmd_spinet.h
* @brief SPI net AT command declarations
* @author evin.zhu
* @date 2026-07-25
*
* @copyright Copyright (c) 2026 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
**********************************************************************/

#ifndef __UNIRTOS_ATCMD_SPINET_H__
#define __UNIRTOS_ATCMD_SPINET_H__

#include "qosa_def.h"
#include "qosa_at_param.h"
#ifdef CONFIG_QAPP_UNIRTOS_SPINET_AT_FUNC
void qstd_exec_qspinet_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qspinetgpio_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_SPINET_AT_FUNC */
#endif /* __UNIRTOS_ATCMD_SPINET_H__ */
