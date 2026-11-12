/*****************************************************************/ /**
* @file unirtos_atcmd_lt_tcp.h
* @brief
* @author kian.zhou@quectel.com
* @date 2026-04-27
*
* @copyright Copyright (c) 2026 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description
* <tr><td>2026-04-27 <td>1.0 <td>Kian.Zhou <td> Init
* </table>
**********************************************************************/
#ifndef __UNIR_ATCMD_LT_TCP_H__
#define __UNIR_ATCMD_LT_TCP_H__

#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_gpio.h"
#include "qosa_at_param.h"



/*===========================================================================
 * Macro Definition
 ===========================================================================*/
#define LT_TCP_MSG_MAX_LEN          (150)
#define LT_TCP_EDIT_TIMEOUT_MS      (60000)  /* 60s edit mode timeout */
#define LT_TCP_SEND_ACK_TIMEOUT_MS  (100)    /* 100ms send ACK timeout */

#define LTTCP_GPIO_OUTPUT_NUM       QOSA_GPIO_19  /* output GPIO */
#define LTTCP_GPIO_INPUT_NUM        QOSA_GPIO_18  /* input interrupt GPIO */
#define LTTCP_PIN_INPUT_NUM         62            /* USIM2_CLK pin */
#define LTTCP_PIN_OUTPUT_NUM        64            /* USIM2_DATA pin */

#define LTTCP_TASK_STACK_SIZE       (1024 * 4)
#define LTTCP_TASK_PRIORITY         QOSA_PRIORITY_NORMAL
#define LTTCP_TASK_MSGQ_SIZE        (20)

#define LTTCP_DEFAULT_PDP_CID       (1)
#define LTTCP_DEFAULT_PDP_ACT_TIME  (90)

#define LTTCP_DEFAULT_RETRY_COUNT   (3)
#define LTTCP_DEFAULT_RETRY_TIME    (1)

#define LTTCP_DEBOUNCE_TIME_MS      (20)   /* GPIO debounce time (ms) */

/*========================================================================
 * Event IDs for internal task messaging
 *========================================================================*/
typedef enum
{
    LT_TCP_EVT_GPIO_IRQ = 1,
    LT_TCP_EVT_PDP_ACT,
    LT_TCP_EVT_SOC_OPEN,
    LT_TCP_EVT_SOC_SEND,
    LT_TCP_EVT_SOC_CLOSE,
    LT_TCP_EVT_SHUTDOWN,
    LT_TCP_EVT_DEINIT,   /* sentinel: task cleans up and exits */
} lt_tcp_evt_e;

/*========================================================================
 * Structure Definition
 *========================================================================*/

/**
 * @brief LT TCP AT configuration (server address, message, etc.)
 */
typedef struct
{
    char              server_addr[128];
    qosa_uint16_t     server_port;
    char              msg[512];
    qosa_uint16_t     msg_len;
    qosa_at_dev_type_e dev_port;  /*!< Saved AT channel when entering edit mode */
} unir_lt_tcp_cnf_t;

/**
 * @brief LT TCP task context
 */
typedef struct
{
    char              server_addr[128];
    qosa_uint16_t     server_port;
    char              msg[512];
    qosa_uint16_t     msg_len;
    qosa_uint8_t      pdp_cid;
    qosa_uint32_t     pdp_active_timeout;
    qosa_task_t       task_ref;
    qosa_msgq_t       msgq;
    int               sockfd;
    int               retry_count;
    int               retry_time;
    int               send_ack_len;
    qosa_bool_t       is_init;
    qosa_timer_t      debounce_timer;
    qosa_gpio_num_e   output_gpio;
    qosa_gpio_num_e   input_gpio;
} unir_lt_tcp_context_t;

/**
 * @brief Message structure for internal task communication
 */
typedef struct
{
    lt_tcp_evt_e evt_id;
} lt_tcp_msg_t;

/*========================================================================
 * Func Definition
 *========================================================================*/
void qstd_exec_qltcfg_cmd(qosa_at_cmd_t *cmd);
#if defined(CONFIG_QOSA_OEM_SE_FUNC)
void qstd_exec_qltact_cmd(qosa_at_cmd_t *cmd);
void unir_lt_tcp_nv_restore(void);
qosa_bool_t unir_lt_tcp_is_enabled(void);
#endif
void unir_lt_tcp_init(void);
void unir_lt_tcp_deinit(void);



#endif /* __UNIR_ATCMD_LT_TCP_H__ */
