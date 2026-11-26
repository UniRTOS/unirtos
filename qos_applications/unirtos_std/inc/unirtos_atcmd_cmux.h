/*****************************************************************/ /**
* @file unirtos_atcmd_cmux.h
* @brief
* @author GitHub Copilot
* @date 2026-06-13
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
**********************************************************************/
#ifndef __UNIRTOS_ATCMD_CMUX_H__
#define __UNIRTOS_ATCMD_CMUX_H__

#include "qosa_def.h"
#include "qosa_at_param.h"

#define UNIRTOS_CMUX_PORT_NUM_DEFAULT             4
#define UNIRTOS_CMUX_MAX_PARAM_NUM                9
#define UNIRTOS_CMUX_T1_T2_UNIT_MS                10
#define UNIRTOS_CMUX_BAUD_INDEX_3000000           12

#define UNIRTOS_CMUX_MODE_DEFAULT                 0
#define UNIRTOS_CMUX_SUBSET_DEFAULT               0
#define UNIRTOS_CMUX_BAUDRATE_DEFAULT             5
#define UNIRTOS_CMUX_FRAME_SIZE_DEFAULT           127
#define UNIRTOS_CMUX_T1_ACK_TIME_DEFAULT          10
#define UNIRTOS_CMUX_RETRY_NUM_DEFAULT            3
#define UNIRTOS_CMUX_T2_CTRL_RSP_TIME_DEFAULT     30
#define UNIRTOS_CMUX_T3_WAKEUP_TIME_DEFAULT       10
#define UNIRTOS_CMUX_SERIAL_SIZE_DEFAULT          2

#define UNIRTOS_CMUX_MODE_MIN                     0
#define UNIRTOS_CMUX_MODE_MAX                     0
#define UNIRTOS_CMUX_SUBSET_MIN                   0
#define UNIRTOS_CMUX_SUBSET_MAX                   0
#define UNIRTOS_CMUX_BAUDRATE_MIN                 1
#define UNIRTOS_CMUX_BAUDRATE_MAX                 8
#define UNIRTOS_CMUX_FRAME_SIZE_MIN               1
#define UNIRTOS_CMUX_FRAME_SIZE_MAX               2048
#define UNIRTOS_CMUX_T1_ACK_TIME_MIN              1
#define UNIRTOS_CMUX_T1_ACK_TIME_MAX              255
#define UNIRTOS_CMUX_RETRY_NUM_MIN                0
#define UNIRTOS_CMUX_RETRY_NUM_MAX                100
#define UNIRTOS_CMUX_T2_CTRL_RSP_TIME_MIN         2
#define UNIRTOS_CMUX_T2_CTRL_RSP_TIME_MAX         255
#define UNIRTOS_CMUX_T3_WAKEUP_TIME_MIN           1
#define UNIRTOS_CMUX_T3_WAKEUP_TIME_MAX           255
#define UNIRTOS_CMUX_SERIAL_SIZE_MIN              1
#define UNIRTOS_CMUX_SERIAL_SIZE_MAX              7

void qstd_exec_cmux_cmd(qosa_at_cmd_t *cmd);
void unirtos_cmux_at_init(void);

#endif /* __UNIRTOS_ATCMD_CMUX_H__ */
