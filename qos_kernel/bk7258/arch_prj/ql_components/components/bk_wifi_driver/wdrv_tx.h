#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include "wdrv_ipc.h"
#include "wdrv_main.h"
#include "pbuf.h"
#include "wdrv_cntrl.h"
#if CONFIG_CONTROLLER_AP_BUFFER_COPY
#include "lwip/stats.h"
extern struct stats_mem *g_cp_lwip_mem;
extern uint32_t g_cp_stats_mem_size;
#endif
#if CONFIG_BK_RAW_LINK
int wdrv_special_txdata_sender(void *head, uint32_t vif_idx);
#endif
int wdrv_txdata_sender(struct pbuf *p, uint32_t vif_idx);
bk_err_t wdrv_txbuf_push(uint8_t channel,void* head,void* tail,uint8_t num);
int wdrv_tx_msg(uint8_t *msg, uint16_t msg_len, wdrv_cmd_cfm *cfm, uint8_t *result);
void wdrv_txdata_pre_process(uint8_t channel, void* head,uint8_t need_retry);
void wdrv_tx_complete(void *param, mb_chnl_ack_t *ack_buf);
void wdrv_tx_msg_complete(void *param, mb_chnl_ack_t *ack_buf);

#ifdef __cplusplus
}
#endif

