/*
 * Copyright 2020-2025 Beken

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 *     http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#if CONFIG_BK_RAW_LINK
#include <os/os.h>
#include <stdbool.h>
#include "soc/soc.h"
#include "conv_utf8_pub.h"
#include "wdrv_tx.h"
#include "wifi_api_ipc.h"
#include "raw_link_api.h"
#include "modules/wifi.h"

static rlk_transfer_cb_t s_rlk_transfer_cb = {0};


struct rlkd_env_t rlkd_env = {0};

static bool rlkd_tx_mem_try_acquire(uint32_t size)
{
    bool granted = true;

    if (!size) {
        return true;
    }

    if (!rlkd_env.mem_lock_init) {
        return true;
    }

    rtos_lock_mutex(&rlkd_env.mem_lock);
    if ((rlkd_env.tx_mem_in_use + size) <= rlkd_env.tx_mem_limit) {
        rlkd_env.tx_mem_in_use += size;
        granted = true;
    } else {
        RLKD_LOGV("rlk tx mem limit reached: need=%u used=%u limit=%u\r\n",
                  size, rlkd_env.tx_mem_in_use, rlkd_env.tx_mem_limit);
        granted = false;
    }
    rtos_unlock_mutex(&rlkd_env.mem_lock);

    return granted;
}

static void rlkd_tx_mem_release(uint32_t size)
{
    if (!size || !rlkd_env.mem_lock_init) {
        return;
    }

    rtos_lock_mutex(&rlkd_env.mem_lock);
    if (rlkd_env.tx_mem_in_use >= size) {
        rlkd_env.tx_mem_in_use -= size;
    } else {
        RLKD_LOGW("rlk tx mem release underflow: release=%u used=%u\r\n",
                  size, rlkd_env.tx_mem_in_use);
        rlkd_env.tx_mem_in_use = 0;
    }
    rtos_unlock_mutex(&rlkd_env.mem_lock);
}


bk_err_t bk_rlk_register_send_cb(bk_rlk_send_cb_t cb)
{
    s_rlk_transfer_cb.send_cb = cb;
    return wifi_send_com_api_cmd(RLK_REGISTER_SEND_CB, 0);
}

bk_rlk_send_cb_t bk_rlk_get_send_cb(void)
{
    return s_rlk_transfer_cb.send_cb;
}

bk_err_t bk_rlk_send_register_ind(uint8_t * msg_payload)
{
    struct rlk_send_struct
    {
        cpdu_t cp;
        struct bk_rx_msg_hdr rx_msg_hdr;
        uint32_t para[3];
        uint32_t payload[1];
    };

    bk_err_t ret = BK_OK;
    const uint8_t *peer_mac_addr = NULL;
    bk_rlk_send_status_t status = BK_RLK_SEND_SUCCESS;

    struct pbuf* pbuf = NULL;
    cpdu_t * cpdu = NULL;
    uint8_t vif_idx = 0;

    pbuf = (struct pbuf*)((uint8_t*)msg_payload + sizeof(uint32_t) - sizeof(cpdu_t) - sizeof(wdrv_rx_msg) - sizeof(struct pbuf));
    struct rlk_send_struct* rlk_hdr = (struct rlk_send_struct*)(pbuf + 1);

    peer_mac_addr = (const uint8_t*)rlk_hdr->para[0];
    status = *(bk_rlk_send_status_t *)rlk_hdr->para[1];

    rlkd_msg_send_cb_ind(peer_mac_addr, status);

    cpdu = (cpdu_t*)(pbuf + 1);
    cpdu->co_hdr.need_free = 1;//RXC free
    vif_idx = cpdu->co_hdr.vif_idx;
    ret = wdrv_txdata_sender(pbuf,vif_idx);//vif null, just for free this RXC pbuf
    return ret;
}

bk_err_t bk_rlk_unregister_send_cb(void)
{
    s_rlk_transfer_cb.send_cb = NULL;
    return wifi_send_com_api_cmd(RLK_UNREGISTER_SEND_CB, 0);
}

bk_err_t bk_rlk_register_send_ex_cb(void *cb)
{
    s_rlk_transfer_cb.send_ex_cb = cb;
    return BK_OK;
}

void *bk_rlk_get_send_ex_cb(void)
{
    return s_rlk_transfer_cb.send_ex_cb;
}

bk_err_t bk_rlk_ungister_send_ex_cb(void)
{
    s_rlk_transfer_cb.send_ex_cb = NULL;
    return BK_OK;
}

bk_err_t bk_rlk_send_ex_register_ind(uint8_t * msg_payload)
{
    struct rlk_send_ex_struct
    {
        cpdu_t cp;
        struct bk_rx_msg_hdr rx_msg_hdr;
        uint32_t para[3];
        uint32_t payload[1];
    };

    bk_err_t ret = BK_OK;
    void *args = NULL;
    bool status = false;

    struct pbuf* pbuf = NULL;
    cpdu_t * cpdu = NULL;
    uint8_t vif_idx = 0;

    pbuf = (struct pbuf*)((uint8_t*)msg_payload + sizeof(uint32_t) - sizeof(cpdu_t) - sizeof(wdrv_rx_msg) - sizeof(struct pbuf));
    struct rlk_send_ex_struct* rlk_hdr = (struct rlk_send_ex_struct*)(pbuf + 1);

    args = (void *)rlk_hdr->para[0];
    status = (bool)rlk_hdr->para[1];

    rlkd_msg_send_ex_cb_ind(args, status);

    cpdu = (cpdu_t*)(pbuf + 1);
    cpdu->co_hdr.need_free = 1;//RXC free
    vif_idx = cpdu->co_hdr.vif_idx;
    ret = wdrv_txdata_sender(pbuf,vif_idx);//vif null, just for free this RXC pbuf

    return ret;
}

bk_err_t bk_rlk_register_recv_cb(bk_rlk_recv_cb_t cb)
{
    s_rlk_transfer_cb.recv_cb = cb;
    return wifi_send_com_api_cmd(RLK_REGISTER_RECV_CB, 0);

    return BK_OK;
}

bk_rlk_recv_cb_t bk_rlk_get_recv_cb(void)
{
    return s_rlk_transfer_cb.recv_cb;
}

bk_err_t bk_rlk_recv_register_ind(uint8_t * msg_payload)
{
    struct rlk_recv_struct
    {
        cpdu_t cp;
        struct bk_rx_msg_hdr rx_msg_hdr;
        uint32_t para[3];
        uint32_t payload[1];
    };

    bk_err_t ret = BK_OK;
    bk_rlk_recv_info_t *rx_info = NULL;

    struct pbuf* pbuf = NULL;
    cpdu_t * cpdu = NULL;
    uint8_t vif_idx = 0;

    pbuf = (struct pbuf*)((uint8_t*)msg_payload + sizeof(uint32_t) - sizeof(cpdu_t) - sizeof(wdrv_rx_msg) - sizeof(struct pbuf));
    struct rlk_recv_struct* rlk_hdr = (struct rlk_recv_struct*)(pbuf + 1);

    rx_info = (bk_rlk_recv_info_t *)rlk_hdr->para[0];

    rlkd_msg_recv_cb_ind(rx_info);

    cpdu = (cpdu_t*)(pbuf + 1);
    cpdu->co_hdr.need_free = 1;//RXC free
    vif_idx = cpdu->co_hdr.vif_idx;
    ret = wdrv_txdata_sender(pbuf,vif_idx);//vif null, just for free this RXC pbuf
    return ret;
}

bk_err_t bk_rlk_ungister_recv_cb(void)
{
    s_rlk_transfer_cb.recv_cb = NULL;
    return wifi_send_com_api_cmd(RLK_UNREGISTER_RECV_CB, 0);
}

bk_err_t bk_rlk_register_acs_cfm_cb(bk_rlk_acs_cb_t cb)
{
    s_rlk_transfer_cb.acs_cb = cb;
    return wifi_send_com_api_cmd(RLK_REGISTER_ACS_CFM_CB, 0);

    return BK_OK;
}

bk_rlk_acs_cb_t bk_rlk_get_acs_cb(void)
{
    return s_rlk_transfer_cb.acs_cb;
}

bk_err_t bk_rlk_acs_cfm_register_ind(uint8_t * msg_payload)
{
    struct rlk_acs_cfm_struct
    {
        cpdu_t cp;
        struct bk_rx_msg_hdr rx_msg_hdr;
        uint32_t para[3];
        uint32_t payload[1];
    };

    bk_err_t ret = BK_OK;
    const uint32_t *chanstatus = NULL;
    uint32_t num_channel = 0;
    uint32_t best_channel = 0;

    bk_rlk_acs_cb_t cb = bk_rlk_get_acs_cb();

    //uint8_t *payload_temp =  NULL;
    struct pbuf* pbuf = NULL;
    cpdu_t * cpdu = NULL;
    uint8_t vif_idx = 0;

    pbuf = (struct pbuf*)((uint8_t*)msg_payload + sizeof(uint32_t) - sizeof(cpdu_t) - sizeof(wdrv_rx_msg) - sizeof(struct pbuf));
    struct rlk_acs_cfm_struct* rlk_hdr = (struct rlk_acs_cfm_struct*)(pbuf + 1);

    chanstatus = (const uint32_t *)rlk_hdr->para[0];
    num_channel = rlk_hdr->para[1];
    best_channel = rlk_hdr->para[2];

    if(cb)
    {
        cb(chanstatus, num_channel, best_channel);//This payload will free in next line.
    }

    cpdu = (cpdu_t*)(pbuf + 1);
    cpdu->co_hdr.need_free = 1;//RXC free
    vif_idx = cpdu->co_hdr.vif_idx;
    ret = wdrv_txdata_sender(pbuf,vif_idx);//vif null, just for free this RXC pbuf
    return ret;
}

bk_err_t bk_rlk_unregister_acs_cfm_cb(void)
{
    s_rlk_transfer_cb.acs_cb = NULL;
    return wifi_send_com_api_cmd(RLK_UNREGISTER_ACS_CFM_CB, 0);
}

bk_err_t bk_rlk_register_scan_cfm_cb(bk_rlk_scan_cb_t cb)
{
    s_rlk_transfer_cb.scan_cb = cb;
    return wifi_send_com_api_cmd(RLK_REGISTER_SCAN_CFM_CB, 0);
}

bk_rlk_scan_cb_t bk_rlk_get_scan_cb(void)
{
    return s_rlk_transfer_cb.scan_cb;
}

bk_err_t bk_rlk_scan_cfm_register_ind(uint8_t * msg_payload)
{
    struct rlk_scan_cfm_struct
    {
        cpdu_t cp;
        struct bk_rx_msg_hdr rx_msg_hdr;
        uint32_t para[3];
        uint32_t payload[1];
    };

    bk_err_t ret = BK_OK;
    bk_rlk_scan_result_t *result = NULL;

    bk_rlk_scan_cb_t cb = bk_rlk_get_scan_cb();

    //uint8_t *payload_temp =  NULL;
    struct pbuf* pbuf = NULL;
    cpdu_t * cpdu = NULL;
    uint8_t vif_idx = 0;

    pbuf = (struct pbuf*)((uint8_t*)msg_payload + sizeof(uint32_t) - sizeof(cpdu_t) - sizeof(wdrv_rx_msg) - sizeof(struct pbuf));
    struct rlk_scan_cfm_struct* rlk_hdr = (struct rlk_scan_cfm_struct*)(pbuf + 1);

    result = (bk_rlk_scan_result_t *)rlk_hdr->para[0];

    if(cb)
    {
        cb(result);//This payload will free in next line.
    }

    cpdu = (cpdu_t*)(pbuf + 1);
    cpdu->co_hdr.need_free = 1;//RXC free
    vif_idx = cpdu->co_hdr.vif_idx;
    ret = wdrv_txdata_sender(pbuf,vif_idx);//vif null, just for free this RXC pbuf
    return ret;
}

bk_err_t bk_rlk_unregister_scan_cb(void)
{
    s_rlk_transfer_cb.scan_cb = NULL;
    return wifi_send_com_api_cmd(RLK_UNREGISTER_SCAN_CFM_CB, 0);
}

bk_err_t bk_rlk_init(void)
{
    return wifi_send_com_api_cmd(RLK_INIT, 0);
}

bk_err_t bk_rlk_deinit(void)
{
    return wifi_send_com_api_cmd(RLK_DEINIT, 0);
}

bk_err_t bk_rlk_set_channel(uint8_t chan)
{
    return wifi_send_com_api_cmd(RLK_SET_CHANNEL,1,chan);
}

uint8_t bk_rlk_get_channel(void)
{
    uint8_t chan = 0xFF;
    void *temp_chan = NULL;

    temp_chan = os_malloc(sizeof(uint8_t));
    if (!temp_chan)
    {
        RLKD_LOGE("%s malloc failed\r\n", __func__);
        return chan;
    }

    wifi_send_com_api_cmd(RLK_GET_CHANNEL, 1, (uint32_t)temp_chan);

    chan = *(uint8_t *)temp_chan;
    os_free(temp_chan);

    return chan;
}

bk_err_t bk_rlk_send(const uint8_t *peer_mac_addr, const void *data, size_t len)
{
    void *buffer_to_ipc = NULL;
    uint32_t total_len = 0;
    uint32_t align_mac_len = RLKD_API_MEM_ALIGN_SIZE(RLK_WIFI_MAC_ADDR_LEN);
    bk_err_t ret = BK_OK;
	uint16_t min_rsv_mem __maybe_unused = 30*1024;

#if 1 //CONFIG_RWNX_SW_TXQ
	//bk_wifi_get_min_rsv_mem(&min_rsv_mem);

	// if the left free heap size is less than threshold value, the new coming
	// packets will be dropped and a buffer error will be feedbacked to upper layer
	if (rtos_get_free_heap_size() < min_rsv_mem) {
		//RWNX_LOGW("tx failed mem:%d cnt:%d\n", rtos_get_free_heap_size(), skb_get_pending_cnt());
		return BK_ERR_NO_MEM;
	}
#endif

    total_len = sizeof(struct ctrl_cmd_hdr) + align_mac_len + len;

    if (!rlkd_tx_mem_try_acquire(total_len)) {
        return BK_ERR_NO_MEM;
    }

    buffer_to_ipc = os_malloc(total_len);

    if (!buffer_to_ipc)
    {
        RLKD_LOGV("%s malloc failed\r\n", __func__);
        rlkd_tx_mem_release(total_len);
        return BK_ERR_NO_MEM;
    }

    os_memcpy((uint8_t *)buffer_to_ipc + sizeof(struct ctrl_cmd_hdr), peer_mac_addr, RLK_WIFI_MAC_ADDR_LEN);
    os_memcpy((uint8_t *)buffer_to_ipc + sizeof(struct ctrl_cmd_hdr) + align_mac_len, data, len);

    struct ctrl_cmd_hdr *cpdu = (struct ctrl_cmd_hdr *)buffer_to_ipc;

    cpdu->co_hdr.length = len;
    cpdu->co_hdr.type = TX_MSDU_DATA;
    cpdu->co_hdr.need_free = 0;
    cpdu->co_hdr.vif_idx = 0; //?
    cpdu->co_hdr.special_type = TX_RAW_LINK_TYPE;
    cpdu->msg_hdr.id = RLK_TX_SEND_EVT;

    ret = wdrv_special_txdata_sender(cpdu,0);

    if (ret != BK_OK)
    {
        rlkd_tx_mem_release(total_len);
        os_free(buffer_to_ipc);
    }

    return (ret == BK_OK ? len : ret);
}

bk_err_t bk_rlk_send_ex(const uint8_t *peer_mac_addr, const bk_rlk_config_info_t *rlk_tx)
{
    void *buffer_to_ipc = NULL;
    uint32_t total_len = 0;
    uint32_t align_mac_len = RLKD_API_MEM_ALIGN_SIZE(RLK_WIFI_MAC_ADDR_LEN);
    bk_err_t ret = BK_OK;

    total_len = align_mac_len + sizeof(bk_rlk_config_info_t) ;

    if ((rlk_tx != NULL) && (rlk_tx->len != 0))
    {
        total_len += rlk_tx->len;
    }

    buffer_to_ipc = os_malloc(total_len);
    if (!buffer_to_ipc)
    {
        RLKD_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    if ((rlk_tx != NULL) && (rlk_tx->cb != NULL))
    {
        bk_rlk_register_send_ex_cb(rlk_tx->cb);
    }

    os_memcpy(buffer_to_ipc, peer_mac_addr, RLK_WIFI_MAC_ADDR_LEN);
    os_memcpy((uint8_t *)buffer_to_ipc + align_mac_len, rlk_tx, sizeof(bk_rlk_config_info_t));

    if ((rlk_tx != NULL) && (rlk_tx->len != 0))
    {
        os_memcpy((uint8_t *)buffer_to_ipc + align_mac_len + sizeof(bk_rlk_config_info_t), rlk_tx->data, rlk_tx->len);
        ((bk_rlk_config_info_t *)((uint8_t *)buffer_to_ipc + align_mac_len))->data = (uint8_t *)buffer_to_ipc + align_mac_len + sizeof(bk_rlk_config_info_t);
    }

    wifi_send_com_api_cmd(RLK_SEND_EX, 3, (uint32_t)buffer_to_ipc, (uint32_t)(buffer_to_ipc + align_mac_len),&ret);

    os_free(buffer_to_ipc);

    return ret;
}

bk_err_t bk_rlk_send_by_oui(const uint8_t *peer_mac_addr, const void *data, size_t len, uint8_t mac_type, uint8_t *oui)
{
    void *buffer_to_ipc = NULL;
    uint32_t total_len = 0;
    uint32_t align_mac_len = RLKD_API_MEM_ALIGN_SIZE(RLK_WIFI_MAC_ADDR_LEN);
    uint32_t align_oui_len = RLKD_API_MEM_ALIGN_SIZE(RLK_WIFI_MAC_ADDR_LEN/2);

    total_len = align_mac_len + len + align_oui_len;

    buffer_to_ipc = os_malloc(total_len);
    if (!buffer_to_ipc)
    {
        RLKD_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memcpy(buffer_to_ipc, peer_mac_addr, RLK_WIFI_MAC_ADDR_LEN);
    os_memcpy((uint8_t *)buffer_to_ipc + align_mac_len, data, len);
    os_memcpy((uint8_t *)buffer_to_ipc + align_mac_len + len, oui, RLK_WIFI_MAC_ADDR_LEN/2);

    wifi_send_com_api_cmd(RLK_SEND_BY_OUI, 5, (uint32_t)buffer_to_ipc, (uint32_t)(buffer_to_ipc + align_mac_len),
                           len, mac_type, (uint32_t)(buffer_to_ipc + align_mac_len + len));

    os_free(buffer_to_ipc);

    return BK_OK;
}

bk_err_t bk_rlk_add_peer(const bk_rlk_peer_info_t *peer)
{
    void *buffer_to_ipc = NULL;

    buffer_to_ipc = os_malloc(sizeof(bk_rlk_peer_info_t));
    if (!buffer_to_ipc)
    {
        RLKD_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memcpy(buffer_to_ipc, peer, sizeof(bk_rlk_peer_info_t));
    wifi_send_com_api_cmd(RLK_ADD_PEER, 1, (uint32_t)buffer_to_ipc);

    os_free(buffer_to_ipc);

    return BK_OK;
}

bk_err_t bk_rlk_del_peer(const uint8_t *peer_mac_addr)
{
    void *buffer_to_ipc = NULL;
    uint32_t align_mac_len = RLKD_API_MEM_ALIGN_SIZE(RLK_WIFI_MAC_ADDR_LEN);
 
    buffer_to_ipc = os_malloc(align_mac_len);
    if (!buffer_to_ipc)
    {
        RLKD_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memcpy(buffer_to_ipc, peer_mac_addr, RLK_WIFI_MAC_ADDR_LEN);
    wifi_send_com_api_cmd(RLK_DEL_PEER, 1, (uint32_t)buffer_to_ipc);

    os_free(buffer_to_ipc);

    return BK_OK;
}

bk_err_t bk_rlk_get_peer(const uint8_t *peer_mac_addr, bk_rlk_peer_info_t *peer)
{
    void *buffer_to_ipc = NULL;
    uint32_t total_len = 0;
    uint32_t align_mac_len = RLKD_API_MEM_ALIGN_SIZE(RLK_WIFI_MAC_ADDR_LEN);

    total_len = align_mac_len + sizeof(bk_rlk_peer_info_t); // MAC + peer info

    buffer_to_ipc = os_malloc(total_len);
    if (!buffer_to_ipc)
    {
        RLKD_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memcpy(buffer_to_ipc, peer_mac_addr, RLK_WIFI_MAC_ADDR_LEN);
    wifi_send_com_api_cmd(RLK_GET_PEER, 2, (uint32_t)buffer_to_ipc, (uint32_t)(buffer_to_ipc + align_mac_len));
    os_memcpy(peer, (uint8_t*)buffer_to_ipc + align_mac_len, sizeof(bk_rlk_peer_info_t));

    os_free(buffer_to_ipc);

    return BK_OK;
}

bk_err_t bk_rlk_is_peer_exist(const uint8_t *peer_mac_addr)
{
    void *buffer_to_ipc = NULL;
    bk_err_t ret_cfm = BK_OK;
    uint32_t align_mac_len = RLKD_API_MEM_ALIGN_SIZE(RLK_WIFI_MAC_ADDR_LEN);

    buffer_to_ipc = os_malloc(align_mac_len);
    if (!buffer_to_ipc)
    {
        RLKD_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memcpy(buffer_to_ipc, peer_mac_addr, RLK_WIFI_MAC_ADDR_LEN);
    wifi_send_com_api_cmd(RLK_IS_PEER_EXIST, 2, (uint32_t)buffer_to_ipc, &ret_cfm);

    os_free(buffer_to_ipc);

    return ret_cfm;
}

bk_err_t bk_rlk_get_peer_num(uint32_t *total_num)
{
    void *temp_total_num = NULL;

    temp_total_num = os_malloc(sizeof(uint32_t));
    if (!temp_total_num)
    {
        RLKD_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    wifi_send_com_api_cmd(RLK_GET_PEER_NUM, 1, (uint32_t)temp_total_num);
    *total_num = *(uint32_t *)temp_total_num;

    os_free(temp_total_num);

    return BK_OK;
}

bk_err_t bk_rlk_set_tx_ac(uint8_t ac)
{
    wifi_send_com_api_cmd(RLK_SET_TX_AC, 1, ac);
    return BK_OK;
}

bk_err_t bk_rlk_set_tx_timeout_ms(uint16_t timeout_ms)
{
    wifi_send_com_api_cmd(RLK_SET_TX_TIMEOUT_MS, 1, timeout_ms);
    return BK_OK;
}

bk_err_t bk_rlk_set_tx_power(uint32_t power)
{
    wifi_send_com_api_cmd(RLK_SET_TX_POWER, 1, power);
    return BK_OK;
}

bk_err_t bk_rlk_set_tx_rate(uint32_t rate)
{
    wifi_send_com_api_cmd(RLK_SET_TX_RATE, 1, rate);
    return BK_OK;
}

bk_err_t bk_rlk_set_tx_retry_cnt(uint32_t retry_cnt)
{
    wifi_send_com_api_cmd(RLK_SET_TX_RETRY_CNT, 1, retry_cnt);
    return BK_OK;
}

bk_err_t bk_rlk_sleep(void)
{
    wifi_send_com_api_cmd(RLK_SLEEP, 0);
    return BK_OK;
}

bk_err_t bk_rlk_wakeup(void)
{
    wifi_send_com_api_cmd(RLK_WAKEUP, 0);
    return BK_OK;
}

bk_err_t bk_rlk_add_white_list(uint8_t mac_type, uint8_t *oui)
{
    void *buffer_to_ipc = NULL;
    uint32_t total_len = RLKD_API_MEM_ALIGN_SIZE(RLK_WIFI_MAC_ADDR_LEN/2);

    buffer_to_ipc = os_malloc(total_len);
    if (!buffer_to_ipc)
    {
        RLKD_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memcpy((uint8_t*)buffer_to_ipc , oui, RLK_WIFI_MAC_ADDR_LEN/2);
    wifi_send_com_api_cmd(RLK_ADD_WHITE_LIST, 2, mac_type, (uint32_t)buffer_to_ipc);

    os_free(buffer_to_ipc);

    return BK_OK;
}

bk_err_t bk_rlk_del_white_list(uint8_t mac_type, uint8_t *oui)
{
    void *buffer_to_ipc = NULL;
    uint32_t total_len = RLKD_API_MEM_ALIGN_SIZE(RLK_WIFI_MAC_ADDR_LEN/2);;

    buffer_to_ipc = os_malloc(total_len);
    if (!buffer_to_ipc)
    {
        RLKD_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memcpy((uint8_t*)buffer_to_ipc, oui, RLK_WIFI_MAC_ADDR_LEN/2);
    wifi_send_com_api_cmd(RLK_DEL_WHITE_LIST, 2, mac_type, (uint32_t)buffer_to_ipc);

    os_free(buffer_to_ipc);

    return BK_OK;
}

bk_err_t bk_rlk_set_mac_hdr_type(uint16_t type)
{
    wifi_send_com_api_cmd(RLK_SET_MAC_HDR_TYPE, 1, type); 
    return BK_OK;
}

bk_err_t bk_rlk_mac_hdr_reinit(void)
{
    wifi_send_com_api_cmd(RLK_MAC_HDR_REINIT, 0);
    return BK_OK;
}

bk_err_t bk_rlk_acs_check(void)
{
    wifi_send_com_api_cmd(RLK_ACS_CHECK, 0);
    return BK_OK;
}

bk_err_t bk_rlk_scan(bk_rlk_scan_info_t *scan_info)
{
    void *buffer_to_ipc = NULL;
    uint32_t total_len = sizeof(bk_rlk_scan_info_t);

    if ((scan_info != NULL) && (scan_info->ssid != NULL))
    {
        total_len +=  strlen(scan_info->ssid) + 1;
    }

    buffer_to_ipc = os_malloc(total_len);
    if (!buffer_to_ipc)
    {
        RLKD_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memcpy(buffer_to_ipc, scan_info, sizeof(bk_rlk_scan_info_t));
    if ((scan_info != NULL) && (scan_info->ssid != NULL))
    {
        os_memcpy((uint8_t*)buffer_to_ipc + sizeof(bk_rlk_scan_info_t), scan_info->ssid, (strlen(scan_info->ssid) + 1));
        ((bk_rlk_scan_info_t*)buffer_to_ipc)->ssid = (char*)((uint8_t*)buffer_to_ipc + sizeof(bk_rlk_scan_info_t));
    }

    wifi_send_com_api_cmd(RLK_SCAN, 1, (uint32_t)buffer_to_ipc);

    os_free(buffer_to_ipc);

    return BK_OK;
}

bk_err_t bk_rlk_set_role(bk_rlk_role_t role, bk_rlk_extra_ies_info_t *ies_info)
{
    void *buffer_to_ipc = NULL;
    uint32_t total_len = sizeof(bk_rlk_role_t) + sizeof(bk_rlk_extra_ies_info_t);

    if ((ies_info != NULL) && (ies_info->extra_ies_len != 0))
    {
        total_len += ies_info->extra_ies_len;
    }

    buffer_to_ipc = os_malloc(total_len);
    if (!buffer_to_ipc)
    {
        RLKD_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memcpy(buffer_to_ipc, &role, sizeof(bk_rlk_role_t));
    os_memcpy((uint8_t*)buffer_to_ipc + sizeof(bk_rlk_role_t), ies_info, sizeof(bk_rlk_extra_ies_info_t));

    if ((ies_info != NULL) && (ies_info->extra_ies_len != 0))
    {
        os_memcpy((uint8_t*)buffer_to_ipc + sizeof(bk_rlk_role_t) + sizeof(bk_rlk_extra_ies_info_t), ies_info->extra_ies, ies_info->extra_ies_len);
        ((bk_rlk_extra_ies_info_t*)buffer_to_ipc)->extra_ies = (uint8_t*)buffer_to_ipc + sizeof(bk_rlk_role_t) + sizeof(bk_rlk_extra_ies_info_t);
    }

    wifi_send_com_api_cmd(RLK_SET_ROLE, 2, (uint32_t)buffer_to_ipc,(uint32_t)(buffer_to_ipc + sizeof(bk_rlk_role_t)));

    os_free(buffer_to_ipc);

    return BK_OK;
}

bk_err_t bk_rlk_slave_app_init(char *ssid)
{
    void *buffer_to_ipc = NULL;
    uint32_t total_len = strlen(ssid) + 1;

    buffer_to_ipc = os_malloc(total_len);
    if (!buffer_to_ipc)
    {
        RLKD_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memcpy(buffer_to_ipc, ssid, total_len);
    wifi_send_com_api_cmd(RLK_SLAVE_APP_INIT, 1, (uint32_t)buffer_to_ipc);

    os_free(buffer_to_ipc);

    return BK_OK;
}

bk_err_t bk_rlk_slave_bssid_app_init(uint8_t *bssid)
{
    void *buffer_to_ipc = NULL;
    uint32_t total_len = RLKD_API_MEM_ALIGN_SIZE(RLK_WIFI_MAC_ADDR_LEN); // BSSID is 6 bytes

    buffer_to_ipc = os_malloc(total_len);
    if (!buffer_to_ipc)
    {
        RLKD_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memcpy(buffer_to_ipc, bssid, RLK_WIFI_MAC_ADDR_LEN);
    wifi_send_com_api_cmd(RLK_SLAVE_BSSID_APP_INIT, 1, (uint32_t)buffer_to_ipc);

    os_free(buffer_to_ipc);

    return BK_OK;
}

bk_err_t bk_rlk_set_acs_auto_switch_chan(uint32_t auto_switch)
{
    wifi_send_com_api_cmd(RLK_SET_ACS_AUTO_SWITCH_CHAN, 1, auto_switch);
    return BK_OK;
}

void rlkd_send_ex_cb_handle(struct rlkd_msg msg)
{
    void *args = (uint8_t *)msg.arg;
    bool status = *((bool *)((uint8_t *)msg.arg + sizeof(uint32_t)));
    void (*cb)(void *, bool) = bk_rlk_get_send_ex_cb();

    if(cb)
    {
        cb(args, status);
    }

    os_free((void *)msg.arg);
}

bk_err_t rlkd_msg_send_ex_cb_ind(void *args, bool status)
{
    bk_err_t ret = BK_OK;
    struct rlkd_msg msg;
    void *buffer = NULL;
    uint32_t total_len = sizeof(uint32_t) + sizeof(bool);

    buffer = os_malloc(total_len);
    if (!buffer)
    {
        RLKD_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memcpy(buffer, args, sizeof(uint32_t));
    os_memcpy((uint8_t*)buffer + sizeof(uint32_t), &status, sizeof(bool));

    msg.msg_id = RLKD_TASK_MSG_SEND_EX_CB;
    msg.arg = (uint32_t)buffer;
    msg.len = total_len;


    ret = rtos_push_to_queue(&rlkd_env.io_queue, &msg, BEKEN_NO_WAIT);
    if (BK_OK != ret) {
        RLKD_LOGW("%s failed, ret=%d\r\n",__func__, ret);
    }

    return ret;
}

void rlkd_send_cb_handle(struct rlkd_msg msg)
{
    uint8_t *peer_mac_addr = (uint8_t *)msg.arg;
    bk_rlk_send_status_t *status = (bk_rlk_send_status_t *)((uint8_t *)msg.arg + RLKD_API_MEM_ALIGN_SIZE(RLK_WIFI_MAC_ADDR_LEN));
    bk_rlk_send_cb_t cb = bk_rlk_get_send_cb();

    if(cb)
    {
        cb(peer_mac_addr, *status);
    }

    os_free((void *)msg.arg);
}

bk_err_t rlkd_msg_send_cb_ind(const uint8_t *peer_mac_addr, bk_rlk_send_status_t status)
{
    bk_err_t ret = BK_OK;
    struct rlkd_msg msg;
    void *buffer = NULL;
    uint32_t total_len =  RLKD_API_MEM_ALIGN_SIZE(RLK_WIFI_MAC_ADDR_LEN) + sizeof(bk_rlk_send_status_t) ;

    buffer = os_malloc(total_len);
    if (!buffer)
    {
        RLKD_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memcpy(buffer, peer_mac_addr, RLK_WIFI_MAC_ADDR_LEN);
    os_memcpy((uint8_t*)buffer + RLKD_API_MEM_ALIGN_SIZE(RLK_WIFI_MAC_ADDR_LEN), &status, sizeof(bk_rlk_send_status_t));

    msg.msg_id = RLKD_TASK_MSG_SEND_CB;
    msg.arg = (uint32_t)buffer;
    msg.len = total_len;

    ret = rtos_push_to_queue(&rlkd_env.io_queue, &msg, BEKEN_NO_WAIT);
    if (BK_OK != ret) {
        RLKD_LOGW("%s failed, ret=%d\r\n",__func__, ret);
    }

    return ret;
}

void rlkd_recv_cb_handle(struct rlkd_msg msg)
{
    bk_rlk_recv_info_t *rx_info = (bk_rlk_recv_info_t *)msg.arg;
    bk_rlk_recv_cb_t cb = bk_rlk_get_recv_cb();

    if(cb)
    {
        cb(rx_info);
    }

    os_free(rx_info);
}

bk_err_t rlkd_msg_recv_cb_ind(bk_rlk_recv_info_t *rx_info)
{
    bk_err_t ret = BK_OK;
    struct rlkd_msg msg;
    void *buffer = NULL;
    uint32_t total_len = sizeof(bk_rlk_recv_info_t) + rx_info->len;

    buffer = os_malloc(total_len);
    if (!buffer)
    {
        RLKD_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memcpy(buffer, rx_info, sizeof(bk_rlk_recv_info_t));
    os_memcpy((uint8_t*)buffer + sizeof(bk_rlk_recv_info_t), rx_info->data, rx_info->len);

    msg.msg_id = RLKD_TASK_MSG_RECV_CB;
    msg.arg = (uint32_t)buffer;
    msg.len = total_len;


    ret = rtos_push_to_queue(&rlkd_env.io_queue, &msg, BEKEN_NO_WAIT);
    if (BK_OK != ret) {
        RLKD_LOGW("%s failed, ret=%d\r\n",__func__, ret);
    }

    return ret;
}

void rlkd_main(void *arg)
{
    bk_err_t ret;
    struct rlkd_msg msg;

    while (1)
    {
        ret = rtos_pop_from_queue(&rlkd_env.io_queue, &msg, BEKEN_WAIT_FOREVER);

        if (BK_OK != ret) {
            continue;
        }

        RLKD_LOGV("%s,%d, msg.msg_id:%d\n",__func__,__LINE__,msg.msg_id);
        switch (msg.msg_id)
        {
            case RLKD_TASK_MSG_SEND_CB: 
                rlkd_send_cb_handle(msg);
                break;
            case RLKD_TASK_MSG_SEND_EX_CB:
                rlkd_send_ex_cb_handle(msg);
                break;
            case RLKD_TASK_MSG_RECV_CB:
                rlkd_recv_cb_handle(msg);
                break;
            default:
                break;
        }
    }
}

bk_err_t ap_rlk_drv_init(void)
{
    bk_err_t ret = BK_OK;
    RLKD_LOGD("%s\n", __func__);

    if(rlkd_env.is_init)
    {
        RLKD_LOGE("%s has already init\n", __func__);
        return ret;
    }

    rlkd_env.tx_mem_limit = RLKD_TX_MEM_LIMIT_BYTES;
    rlkd_env.tx_mem_in_use = 0;
    rlkd_env.mem_lock_init = 0;

    ret = rtos_init_queue(&rlkd_env.io_queue, "rlkd_queue", sizeof(struct rlkd_msg), RLKD_QUEUE_LEN);

    if (ret != BK_OK)
    {
        RLKD_LOGE("%s init queue failed:%d\n", __func__, ret);
        goto rlkd_init_failed;
    }

    ret = rtos_init_mutex(&rlkd_env.mem_lock);
    if (ret != BK_OK)
    {
        RLKD_LOGE("%s init mem mutex failed:%d\n", __func__, ret);
        goto rlkd_init_failed;
    }
    rlkd_env.mem_lock_init = 1;

    ret = rtos_smp_create_thread(&rlkd_env.handle,
                                RLKD_TASK_PRIO,
                                "rlkd_thread",
                                (beken_thread_function_t)rlkd_main,
                                4096,
                                (beken_thread_arg_t)0);

    if (ret != BK_OK)
    {
        RLKD_LOGE("%s create task failed:%d\n", __func__, ret);
        goto rlkd_init_failed;
    }

    rlkd_env.is_init = 1;

    return ret;
rlkd_init_failed:
    rlkd_deinit();
    return ret;
}

/**
 * @brief Handle memory free request from CP side
 * @param mem_addr Memory address to be freed
 */
bk_err_t rlkd_handle_free_mem_req(uint32_t mem_addr)
{
    void *ptr_to_free = (void *)mem_addr;
    struct ctrl_cmd_hdr *cpdu = NULL;
    uint32_t total_len = 0;
    
    if (!ptr_to_free) {
        RLKD_LOGE("[RLK] Invalid memory address for free: 0x%08X\r\n", mem_addr);
        return BK_ERR_PARAM;
    }
    
    RLKD_LOGV("[RLK] Free memory from CP: addr=0x%08X\r\n", mem_addr);

    cpdu = (struct ctrl_cmd_hdr *)ptr_to_free;
    total_len = sizeof(struct ctrl_cmd_hdr) +
                RLKD_API_MEM_ALIGN_SIZE(RLK_WIFI_MAC_ADDR_LEN) +
                cpdu->co_hdr.length;

    rlkd_tx_mem_release(total_len);
    
    // Free memory allocated by AP side
    os_free(ptr_to_free);
    
    return BK_OK;
}

bk_err_t rlkd_deinit(void)
{
    RLKD_LOGD("rlkd_deinit\n");
    if (rlkd_env.handle) {
        rtos_delete_thread(&rlkd_env.handle);
        rlkd_env.handle = NULL;
    }

    if (rlkd_env.io_queue) {
        rtos_deinit_queue(&rlkd_env.io_queue);
        rlkd_env.io_queue = NULL;
    }

    if (rlkd_env.mem_lock_init) {
        rtos_deinit_mutex(&rlkd_env.mem_lock);
        rlkd_env.mem_lock_init = 0;
    }

    rlkd_env.tx_mem_in_use = 0;
    rlkd_env.is_init = 0;

    return BK_OK;
}

#endif // CONFIG_BK_RAW_LINK

