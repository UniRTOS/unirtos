/*
 * Copyright 2020-2025 Beken
 *
 * @file wdrv_cntrl.c 
 * 
 * @brief Beken Wi-Fi Driver Platform Entry
 *
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

#include "wdrv_cntrl.h"
#include "wdrv_main.h"
#include "components/log.h"
#include <os/str.h>
#include <os/mem.h>
#include <os/os.h>
#include "net.h"
#include "wdrv_rx.h"
#include "wifi_api.h"
#include "wdrv_co_list.h"
#include "wdrv_tx.h"
#include <components/netif.h>
#include "components/event.h"
#include "wifi_api_ipc.h"
#include "lwip/stats.h"
#if CONFIG_BRIDGE
#include "bk_bridge.h"
#endif
#define TAG "wdrv_cntrl"

wdrv_wlan wdrv_host_env;

wifi_linkstate_reason_t connect_flag = {WIFI_LINKSTATE_STA_IDLE, WIFI_REASON_MAX};

FUNC_1PARAM_PTR connection_status_cb = 0;

void wdrv_notify_sta_got_ip(void);

static rx_handle_customer_event_cb s_rx_handle_cust_event_cb = NULL;

void bk_customer_event_register_callback(rx_handle_customer_event_cb callback)
{
    s_rx_handle_cust_event_cb = callback ;
}

void wdv_rx_handle_customer_event(void *data, uint16_t len)
{
    if (s_rx_handle_cust_event_cb) {
        s_rx_handle_cust_event_cb(data, len);
    }

}

uint32_t wdrv_param_init(void)
{
    if (NULL == g_wlan_general_param) {
        g_wlan_general_param = (general_param_t *)os_zalloc(sizeof(general_param_t));
        BK_ASSERT(g_wlan_general_param); /* ASSERT VERIFIED */
    }

    if (NULL == g_ap_param_ptr) {
        g_ap_param_ptr = (ap_param_t *)os_zalloc(sizeof(ap_param_t));
        BK_ASSERT(g_ap_param_ptr); /* ASSERT VERIFIED */
    }

    if (NULL == g_sta_param_ptr) {
        g_sta_param_ptr = (sta_param_t *)os_zalloc(sizeof(sta_param_t));
        BK_ASSERT(g_sta_param_ptr); /* ASSERT VERIFIED */
    }

    return 0;
}
uint32_t wdrv_param_deinit(void)
{
    if (NULL != g_wlan_general_param) {
        os_free(g_wlan_general_param);
    }

    if (NULL == g_ap_param_ptr) {
        os_free(g_ap_param_ptr);
    }

    if (NULL == g_sta_param_ptr) {
        os_free(g_sta_param_ptr);
    }

    return 0;
}
bk_err_t bk_wdrv_get_mac(uint8_t *mac, mac_type_t type)
{
    uint8_t mac_mask = (0xff & (2/*NX_VIRT_DEV_MAX*/ - 1));
    uint8_t mac_low;

    if (wdrv_get_mac_addr() != 0)
    {
        WDRV_LOGE("bk_wdrv_get_mac  failed\n");
        return BK_FAIL;
    }

    switch (type) {
    case MAC_TYPE_BASE:
        memcpy(mac, wdrv_host_env.macaddr_cfm.mac_addr, BK_MAC_ADDR_LEN);
        break;

    case MAC_TYPE_AP:
        mac_mask = (0xff & (2/*NX_VIRT_DEV_MAX*/ - 1));

        memcpy(mac, wdrv_host_env.macaddr_cfm.mac_addr, BK_MAC_ADDR_LEN);
        mac_low = mac[5];

        // if  NX_VIRT_DEV_MAX == 4.
        // if support AP+STA, mac addr should be equal with each other in byte0-4 & byte5[7:2],
        // byte5[1:0] can be different
        // ie: mac[5]= 0xf7,  so mac[5] can be 0xf4, f5, f6. here wre chose 0xf4
        mac[5] &= ~mac_mask;
        mac_low = ((mac_low & mac_mask) ^ mac_mask);
        mac[5] |= mac_low;
        break;

    case MAC_TYPE_STA:
        memcpy(mac, wdrv_host_env.macaddr_cfm.mac_addr, BK_MAC_ADDR_LEN);
        break;

    default:
        return BK_ERR_INVALID_MAC_TYPE;
    }

    return BK_OK;
}

int wdrv_get_mac_addr()
{
    wdrv_cmd_hdr req = {0};
    wdrv_cmd_cfm cmd_cfm = {0};
    req.cmd_id = BK_CMD_GET_MAC_ADDR;
    cmd_cfm.waitcfm = WDRV_CMD_WAITCFM;
    cmd_cfm.cfm_id = 0;

    WDRV_LOGD("wdrv_host_cmd_id : %d\n", req.cmd_id);
    //ToDo
    wdrv_tx_msg((uint8_t *)&req, sizeof(req), &cmd_cfm, NULL);
    return 0;
}

void wdrv_notify_scan_done(void *data, uint16_t len)
{
    wifi_event_scan_done_t event_data = {0};
    /* post event scan done*/
    os_memset(&event_data, 0, sizeof(event_data));
    os_memcpy(&event_data, data, len);
    bk_event_post(EVENT_MOD_WIFI, EVENT_WIFI_SCAN_DONE, &event_data,
    sizeof(event_data), BEKEN_NEVER_TIMEOUT);
}

void wdrv_notify_sta_connected(void)
{
    wifi_event_sta_connected_t sta_connected = {0};

    os_memset(&sta_connected, 0, sizeof(sta_connected));
    os_memcpy(&sta_connected.ssid, wdrv_host_env.connect_ind.ussid,
              sizeof(wdrv_host_env.connect_ind.ussid));

    BK_LOG_ON_ERR(bk_event_post(EVENT_MOD_WIFI, EVENT_WIFI_STA_CONNECTED,
                                &sta_connected, sizeof(sta_connected),
                                BEKEN_NEVER_TIMEOUT));
}

#if CONFIG_WIFI_VNET_CONTROLLER
static void wdrv_fill_ip4_from_connect_ind(netif_ip4_config_t *ip4)
{
	os_snprintf(ip4->ip, NETIF_IP4_STR_LEN, "%u.%u.%u.%u",
		    (wdrv_host_env.connect_ind.ip >> 0) & 0xff,
		    (wdrv_host_env.connect_ind.ip >> 8) & 0xff,
		    (wdrv_host_env.connect_ind.ip >> 16) & 0xff,
		    (wdrv_host_env.connect_ind.ip >> 24) & 0xff);
	os_snprintf(ip4->mask, NETIF_IP4_STR_LEN, "%u.%u.%u.%u",
		    (wdrv_host_env.connect_ind.mk >> 0) & 0xff,
		    (wdrv_host_env.connect_ind.mk >> 8) & 0xff,
		    (wdrv_host_env.connect_ind.mk >> 16) & 0xff,
		    (wdrv_host_env.connect_ind.mk >> 24) & 0xff);
	os_snprintf(ip4->gateway, NETIF_IP4_STR_LEN, "%u.%u.%u.%u",
		    (wdrv_host_env.connect_ind.gw >> 0) & 0xff,
		    (wdrv_host_env.connect_ind.gw >> 8) & 0xff,
		    (wdrv_host_env.connect_ind.gw >> 16) & 0xff,
		    (wdrv_host_env.connect_ind.gw >> 24) & 0xff);
	os_snprintf(ip4->dns, NETIF_IP4_STR_LEN, "%u.%u.%u.%u",
		    (wdrv_host_env.connect_ind.dns >> 0) & 0xff,
		    (wdrv_host_env.connect_ind.dns >> 8) & 0xff,
		    (wdrv_host_env.connect_ind.dns >> 16) & 0xff,
		    (wdrv_host_env.connect_ind.dns >> 24) & 0xff);
}

void wdrv_notify_sta_got_ipv4(void)
{
    netif_ip4_config_t ip4 = {0};

    if (wdrv_host_env.connect_ind.ip == 0)
        return;

    wdrv_fill_ip4_from_connect_ind(&ip4);
    sta_ip_mode_set(0);
    sta_ip_down();
    ip_address_set(1, 0, ip4.ip, ip4.mask, ip4.gateway, ip4.dns);
    sta_ip_start();

    /* Single notify entry: legacy cb + EVENT_NETIF_GOT_IP4 (dedup by link state). */
    wdrv_notify_sta_got_ip();
}
#endif

#if CONFIG_P2P
void wdrv_p2p_role_clear(void)
{
    wdrv_host_env.p2p_role = 0;
}

void wdrv_notify_gc_got_ipv4(void)
{
    if (wdrv_host_env.connect_ind.ip == 0)
        return;

    p2p_gc_ip_down();
    p2p_gc_ip_apply_connect(wdrv_host_env.connect_ind.ip,
                            wdrv_host_env.connect_ind.gw,
                            wdrv_host_env.connect_ind.mk,
                            wdrv_host_env.connect_ind.dns);
}

void wdrv_notify_gc_got_ip(void)
{
    netif_event_got_ip4_t event_data = {0};
    netif_ip4_config_t ip4 = {0};

    event_data.netif_if = NETIF_IF_P2P;
    if (bk_netif_get_ip4_config(NETIF_IF_P2P, &ip4) == BK_OK)
        os_memcpy(event_data.ip, ip4.ip, NETIF_IP4_STR_LEN);
    BK_LOG_ON_ERR(bk_event_post(EVENT_MOD_NETIF, EVENT_NETIF_GOT_IP4,
                                &event_data, sizeof(event_data),
                                BEKEN_NEVER_TIMEOUT));
}
#endif

void wdrv_notify_sta_disconnected(void *data, uint16_t len)
{
    wifi_event_sta_disconnected_t sta_disconnected = {0};
    wifi_linkstate_reason_t info = {0};
    os_memcpy(&sta_disconnected, data, len);

    info.state = WIFI_LINKSTATE_STA_DISCONNECTED;
    info.reason_code = sta_disconnected.disconnect_reason;
    mhdr_set_station_status(info);

    WDRV_LOGV("sta disconnect reason %d,local %d\n",
              sta_disconnected.disconnect_reason, sta_disconnected.local_generated);
    BK_LOG_ON_ERR(bk_event_post(EVENT_MOD_WIFI, EVENT_WIFI_STA_DISCONNECTED,
                                &sta_disconnected, sizeof(sta_disconnected),
                                BEKEN_NEVER_TIMEOUT));
}

void wdrv_notify_sap_sta_connected(void)
{
    wifi_event_ap_connected_t ap_connected = {0};

    os_memset(&ap_connected, 0, sizeof(ap_connected));
    os_memcpy(ap_connected.mac, wdrv_host_env.ap_assoc_sta_addr_ind.sub_sta_addr, ETH_ALEN);
    BK_LOG_ON_ERR(bk_event_post(EVENT_MOD_WIFI, EVENT_WIFI_AP_CONNECTED,
                                &ap_connected, sizeof(ap_connected), BEKEN_NEVER_TIMEOUT));
}

void wdrv_notify_sta_got_ipv6(void)
{
#if CONFIG_IPV6
    netif_event_got_ip6_t got_ipv6 = {0};
    struct ipv6_config ipv6_configs[MAX_IPV6_ADDRESSES];
    uint8_t addr_count;
    int i;

    addr_count = wdrv_host_env.ipv6_ind.addr_count;
    if (addr_count > NETIF_MAX_IPV6_ADDRESSES)
        addr_count = NETIF_MAX_IPV6_ADDRESSES;
    if (addr_count == 0)
        return;

    got_ipv6.netif_if = NETIF_IF_STA;
    got_ipv6.addr_count = addr_count;
    for (i = 0; i < addr_count; i++) {
        os_memcpy(&got_ipv6.ipv6_addr[i], &wdrv_host_env.ipv6_ind.ipv6_addr[i],
                  sizeof(got_ipv6.ipv6_addr[i]));
        os_memcpy(&ipv6_configs[i].address, got_ipv6.ipv6_addr[i].address, 16);
        ipv6_configs[i].addr_state = got_ipv6.ipv6_addr[i].addr_state;
    }

    if (net_configure_ipv6_address(ipv6_configs, addr_count, net_get_sta_handle()) != 0) {
        WDRV_LOGE(TAG, "configure IPv6 address failed\n");
        return;
    }

    BK_LOG_ON_ERR(bk_event_post(EVENT_MOD_NETIF, EVENT_NETIF_GOT_IP6,
                                &got_ipv6, sizeof(got_ipv6), BEKEN_NEVER_TIMEOUT));
#endif
}

void wdrv_notify_sap_sta_disconnected(void)
{
    wifi_event_ap_connected_t ap_disconnected = {0};

    os_memset(&ap_disconnected, 0, sizeof(ap_disconnected));
    os_memcpy(ap_disconnected.mac, wdrv_host_env.ap_assoc_sta_addr_ind.sub_sta_addr, ETH_ALEN);
#if CONFIG_BRIDGE
    bk_bridge_hook_sta_disconnected(ap_disconnected.mac);
#endif
    BK_LOG_ON_ERR(bk_event_post(EVENT_MOD_WIFI, EVENT_WIFI_AP_DISCONNECTED,
                                &ap_disconnected, sizeof(ap_disconnected), BEKEN_NEVER_TIMEOUT));
}

void mhdr_set_station_status(wifi_linkstate_reason_t info)
{
	GLOBAL_INT_DECLARATION();

	GLOBAL_INT_DISABLE();
	connect_flag.state = info.state;
	connect_flag.reason_code = info.reason_code;
	GLOBAL_INT_RESTORE();
}

wifi_linkstate_reason_t mhdr_get_station_status(void)
{
	return connect_flag;
}

void wdrv_reset_sta_link_state(wifi_link_state_t state)
{
	wifi_linkstate_reason_t info = {
		.state = state,
		.reason_code = WIFI_REASON_MAX,
	};

	mhdr_set_station_status(info);
	wdrv_host_env.wlan_link_sta_status = state;
}

FUNC_1PARAM_PTR bk_wlan_get_status_cb(void)
{
	return connection_status_cb;
}

void bk_wlan_status_register_cb(FUNC_1PARAM_PTR cb)
{
	connection_status_cb = cb;
}

void wifi_netif_call_status_cb_when_sta_got_ip(void)
{
	FUNC_1PARAM_PTR fn;
	wifi_linkstate_reason_t info = mhdr_get_station_status();

	fn = (FUNC_1PARAM_PTR)bk_wlan_get_status_cb();
	if(fn) {
		info.state = WIFI_LINKSTATE_STA_GOT_IP;
		info.reason_code = WIFI_REASON_MAX;
		(*fn)(&info);
	}
}

void wdrv_notify_sta_got_ip(void)
{
    wifi_linkstate_reason_t info;
    netif_ip4_config_t wdrv_got_ip = {0};
    netif_event_got_ip4_t event_data = {0};

    info = mhdr_get_station_status();
    if (info.state == WIFI_LINKSTATE_STA_GOT_IP)
        return;

    info.state = WIFI_LINKSTATE_STA_GOT_IP;
    info.reason_code = WIFI_REASON_MAX;
    mhdr_set_station_status(info);
    wifi_netif_call_status_cb_when_sta_got_ip();

    event_data.netif_if = NETIF_IF_STA;
#if CONFIG_P2P
    //TODO current not support p2p coexist with sta or softap
    if (bk_wifi_is_p2p_enabled()) {
        event_data.netif_if = NETIF_IF_P2P;
    }
#endif
    BK_LOG_ON_ERR(bk_netif_get_ip4_config(NETIF_IF_STA, &wdrv_got_ip));
    os_memcpy(event_data.ip, wdrv_got_ip.ip, NETIF_IP4_STR_LEN);

    BK_LOG_ON_ERR(bk_event_post(EVENT_MOD_NETIF, EVENT_NETIF_GOT_IP4,
                                &event_data, sizeof(event_data), BEKEN_NEVER_TIMEOUT));
}

int bk_wdrv_send_customer_data(uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0) {
        WDRV_LOGE("bk_wdrv_send_customer_data : invalid input parameters\n");
        return -1;
    }
    struct wdrv_customer_req cust_req = {0};
    wdrv_cmd_cfm cmd_cfm = {0};
    cust_req.cmd_hdr.cmd_id = BK_CMD_CUSTOMER_DATA;
    cust_req.cmd_hdr.len = len;
    cmd_cfm.waitcfm = WDRV_CMD_NOWAITCFM;
    cmd_cfm.cfm_id = 0;

    os_memcpy(cust_req.data, data, len);
    WDRV_LOGV("bk_wdrv_send_customer_data : %d, len: %d\n", cust_req.cmd_hdr.cmd_id, len + sizeof(wdrv_cmd_hdr));
#if 0
    WDRV_LOGD("customer_data: ");
    for (int i = 0; i < len + sizeof(wdrv_cmd_hdr); i++) {
        WDRV_LOGD("%02x ", cust_req.data[i]);
    }
    WDRV_LOGD("\n");
#endif
    wdrv_tx_msg((uint8_t *)&cust_req, len + sizeof(wdrv_cmd_hdr) , &cmd_cfm, NULL);
    return 0;
}

bk_err_t bk_wdrv_customer_transfer(uint16_t cmd_id, uint8_t *data, uint16_t len)
{
    int ret = 0;

    if (data == NULL || len == 0) {
        WDRV_LOGE("%s : Invalid input parameters\n", __func__);
        return -1;
    }

    cifd_cust_msg_hdr_t *cust_trans = os_malloc(sizeof(cifd_cust_msg_hdr_t) + len);

    cust_trans->cmd_id = cmd_id;
    cust_trans->len = len;

    WDRV_LOGV("%s, cmd_id: %d, len: %d\n", __func__, cust_trans->cmd_id, sizeof(cifd_cust_msg_hdr_t) + cust_trans->len);
    os_memcpy((uint8_t*)cust_trans + sizeof(cifd_cust_msg_hdr_t), data, len);
    ret = bk_wdrv_send_customer_data((uint8_t*)cust_trans, sizeof(cifd_cust_msg_hdr_t) + cust_trans->len);

    os_free(cust_trans);

    return ret;
}


bk_err_t bk_wdrv_customer_transfer_rsp(uint16_t cmd_id, uint8_t *data, uint16_t len,
                      uint8_t *response_buf, uint16_t response_buf_size, uint16_t *response_len)
{
    int ret = 0;
    cifd_cust_msg_hdr_t *cust_trans = NULL;
    struct wdrv_customer_req cust_req = {0};
    wdrv_cmd_cfm cmd_cfm = {0};
    uint32_t total_len = 0;

    if (response_buf == NULL || response_len == NULL) {
        WIFI_LOGE("%s : Invalid response parameters\n", __func__);
        return BK_ERR_PARAM;
    }

    total_len = sizeof(cifd_cust_msg_hdr_t) + (data ? len : 0);
    cust_trans = os_malloc(total_len);
    if (cust_trans == NULL) {
        WIFI_LOGE("%s : malloc failed\n", __func__);
        return BK_ERR_NO_MEM;
    }

    cust_trans->cmd_id = cmd_id;
    cust_trans->len = len;
    if (data && len > 0) {
        os_memcpy(cust_trans->payload, data, len);
    }

    cust_req.cmd_hdr.cmd_id = BK_CMD_CUSTOMER_DATA;
    cust_req.cmd_hdr.len = total_len;
    cmd_cfm.waitcfm = WDRV_CMD_WAITCFM;
    cmd_cfm.cfm_id = 0;

    os_memcpy(cust_req.data, (uint8_t*)cust_trans, total_len);

    ret = wdrv_tx_msg((uint8_t *)&cust_req, sizeof(wdrv_cmd_hdr) + total_len, &cmd_cfm, response_buf);

    os_free(cust_trans);

    if (ret < 0) {
        WIFI_LOGE("%s : send failed, ret=%d\n", __func__, ret);
        *response_len = 0;
        return BK_ERR_TIMEOUT;
    }

    if (ret > 0 && ret <= response_buf_size) {
        *response_len = (uint16_t)ret;
        WIFI_LOGV("%s : received response, len=%d\n", __func__, *response_len);
        return BK_OK;
    } else {
        WIFI_LOGE("%s : invalid response len=%d\n", __func__, ret);
        *response_len = 0;
        return BK_ERR_PARAM;
    }
}

bk_err_t wdrv_cntrl_get_cif_stats()
{
    struct get_cif_stats
    {
        wdrv_cmd_hdr cmd_hdr;
        wdrv_cmd_cfm cmd_cfm;
    };
    struct get_cif_stats req = {0};

    req.cmd_hdr.cmd_id =  BK_INTERFACE_DEBUG_CMD;
    req.cmd_cfm.waitcfm = WDRV_CMD_NOWAITCFM;
    req.cmd_cfm.cfm_id = 0;
    WDRV_LOGD("%s,%d\n",__func__,__LINE__);

    wdrv_tx_msg((uint8_t *)&req, sizeof(req), &req.cmd_cfm, NULL);

    return BK_OK;
}
#if CONFIG_CONTROLLER_AP_BUFFER_COPY
bk_err_t wdrv_cntrl_get_cp_lwip_mem_addr()
{
    bk_err_t ret = BK_OK;
    struct get_cif_stats
    {
        wdrv_cmd_hdr cmd_hdr;
        wdrv_cmd_cfm cmd_cfm;
    };
    struct get_cif_stats req = {0};

    req.cmd_hdr.cmd_id =  BK_CP_LWIP_MEM_ADDR_CMD;
    req.cmd_cfm.waitcfm = WDRV_CMD_WAITCFM;
    req.cmd_cfm.cfm_id = 0;


    wdrv_tx_msg((uint8_t *)&req, sizeof(req), &req.cmd_cfm, NULL);
    
    if(g_cp_lwip_mem == NULL)
    {
        ret = BK_FAIL;
        BK_ASSERT(0);
    }
    if(g_cp_stats_mem_size != sizeof(struct stats_mem))
    {
        ret = BK_FAIL;
        BK_ASSERT(0);
    }
    WDRV_LOGV("%s,%d,addr:0x%x\n","cp_lwip_mem_addr",__LINE__,g_cp_lwip_mem);

    return ret;
}
#endif
void wdrv_rx_handle_cmd_confirm(wdrv_rx_msg *msg)
{
    WDRV_LOGV("%s,%d\n",__func__,__LINE__);

    switch(BK_CFM_GET_CMD_ID(msg->id)) {
        case BK_CMD_GET_MAC_ADDR:
            os_memcpy(&wdrv_host_env.macaddr_cfm, msg->param, sizeof(struct wdrv_mac_addr_cfm));
            WDRV_LOGV("MAC addr: %02x:%02x:%02x:%02x:%02x:%02x\n",
            wdrv_host_env.macaddr_cfm.mac_addr[0], wdrv_host_env.macaddr_cfm.mac_addr[1],
            wdrv_host_env.macaddr_cfm.mac_addr[2], wdrv_host_env.macaddr_cfm.mac_addr[3],
            wdrv_host_env.macaddr_cfm.mac_addr[4], wdrv_host_env.macaddr_cfm.mac_addr[5]);
            break;
        case BK_CMD_GET_WLAN_STATUS:
            break;
        case BK_CMD_CONNECT:
            WDRV_LOGD("SET-MCU-WLAN: start connect\r\n");
            break;
        case BK_CMD_DISCONNECT:
            WDRV_LOGV("SET-MCU-WLAN: start disconnect\r\n");
            break;
        case BK_CMD_SET_MEDIA_MODE:
            WDRV_LOGV("SET-MCU-WLAN: set media mode\r\n");
            break;
        case BK_CMD_SET_MEDIA_QUALITY:
            WDRV_LOGV("SET-MCU-WLAN: set media quality\r\n");
            break;
        case BK_CMD_START_AP:
            WDRV_LOGV("MCU-AP-STATE: start AP\r\n");
            break;
#if CONFIG_CONTROLLER_AP_BUFFER_COPY
        case BK_CP_LWIP_MEM_ADDR_CMD:
        {
            g_cp_lwip_mem = (void*)msg->param[0];
            g_cp_stats_mem_size =  msg->param[1];
            WDRV_LOGD("CP_LWIP_MEM_ADDR_CMD addr:0x%x \r\n",g_cp_lwip_mem);
            break;
        }
#endif
        default:
            WDRV_LOGD("%s,%d,ID:0x%x\n",__func__,__LINE__,BK_CFM_GET_CMD_ID(msg->id));
            break;
    }
    wdrv_rx_confirm_tx_msg(msg);

}
void wdrv_rx_handle_wifi_api_event(wdrv_rx_msg *msg)
{
    wifi_handle_api_evt(msg->id, (uint8_t *)msg->param, msg->param_len);
}

#if CONFIG_WIFI_VNET_CONTROLLER
static wifi_event_t wdrv_cif_to_wifi_event(uint16_t cif_evt)
{
	switch (cif_evt) {
	case CIF_WIFI_EVT_STA_CONNECTED:
		return EVENT_WIFI_STA_CONNECTED;
	case CIF_WIFI_EVT_STA_DISCONNECTED:
		return EVENT_WIFI_STA_DISCONNECTED;
#if CONFIG_P2P
	case CIF_WIFI_EVT_GO_CONNECTED:
		return EVENT_WIFI_GO_CONNECTED;
	case CIF_WIFI_EVT_GO_DISCONNECTED:
		return EVENT_WIFI_GO_DISCONNECTED;
	case CIF_WIFI_EVT_GC_CONNECTED:
		return EVENT_WIFI_GC_CONNECTED;
	case CIF_WIFI_EVT_GC_DISCONNECTED:
		return EVENT_WIFI_GC_DISCONNECTED;
#endif
	default:
		return EVENT_WIFI_COUNT;
	}
}

static void wdrv_handle_wifi_event_ind(cif_wifi_event_ind_t *ind)
{
    wifi_event_t evt;
    wifi_event_sta_disconnected_t *disc;
    wifi_event_sta_connected_t *conn;
#if CONFIG_P2P
    wifi_event_ap_connected_t *go_ev;
#endif

    if (!ind || ind->data_len > CIF_WIFI_EVENT_IND_MAX_DATA)
        return;

    evt = wdrv_cif_to_wifi_event(ind->event_id);
    if (evt >= EVENT_WIFI_COUNT)
        return;

    switch (evt) {
    case EVENT_WIFI_STA_CONNECTED:
        wdrv_reset_sta_link_state(WIFI_LINKSTATE_STA_CONNECTED);
        wdrv_host_env.wlan_mode = WIFI_MODE_STA;
        if (ind->data_len >= sizeof(wifi_event_sta_connected_t)) {
            conn = (wifi_event_sta_connected_t *)ind->data;
            os_memcpy(wdrv_host_env.connect_ind.ussid, conn->ssid,
                      sizeof(wdrv_host_env.connect_ind.ussid));
        }
        break;
#if CONFIG_P2P
    case EVENT_WIFI_GC_CONNECTED:
        wdrv_host_env.wlan_link_sta_status = WIFI_LINKSTATE_STA_CONNECTED;
        wdrv_host_env.wlan_mode = WIFI_MODE_STA;
        wdrv_host_env.connect_ind.vif_idx = 3;
        wdrv_host_env.p2p_role = 2;
        if (ind->data_len >= sizeof(wifi_event_sta_connected_t)) {
            conn = (wifi_event_sta_connected_t *)ind->data;
            os_memcpy(wdrv_host_env.connect_ind.ussid, conn->ssid,
                      sizeof(wdrv_host_env.connect_ind.ussid));
        }
        break;
#endif
    case EVENT_WIFI_STA_DISCONNECTED:
#if CONFIG_P2P
    case EVENT_WIFI_GC_DISCONNECTED:
#endif
        if (ind->data_len >= sizeof(wifi_event_sta_disconnected_t)) {
            disc = (wifi_event_sta_disconnected_t *)ind->data;
            wifi_linkstate_reason_t info = {
                .state = WIFI_LINKSTATE_STA_DISCONNECTED,
                .reason_code = disc->disconnect_reason,
            };
            mhdr_set_station_status(info);
        }
        wdrv_host_env.wlan_link_sta_status = WIFI_LINKSTATE_STA_DISCONNECTED;
        wdrv_host_env.wlan_mode = WIFI_MODE_IDLE;
#if CONFIG_P2P
        if (evt == EVENT_WIFI_GC_DISCONNECTED) {
            wdrv_host_env.connect_ind.vif_idx = 0;
            wdrv_host_env.p2p_role = 0;
        }
#endif
        break;
#if CONFIG_P2P
    case EVENT_WIFI_GO_CONNECTED:
    case EVENT_WIFI_GO_DISCONNECTED:
        if (ind->data_len >= sizeof(wifi_event_ap_connected_t)) {
            go_ev = (wifi_event_ap_connected_t *)ind->data;
            os_memcpy(wdrv_host_env.ap_assoc_sta_addr_ind.sub_sta_addr,
                      go_ev->mac, ETH_ALEN);
        }
        break;
#endif
    default:
        break;
    }

    BK_LOG_ON_ERR(bk_event_post(EVENT_MOD_WIFI, evt, ind->data,
                                ind->data_len, BEKEN_NEVER_TIMEOUT));
}
#endif

void wdrv_rx_handle_wifi_cntrl_event(wdrv_rx_msg *msg)
{
    WDRV_LOGD("%s,%d,%d\n",__func__,__LINE__,msg->id);
    //int loop_idx = 0;
    switch(msg->id) {
        case BK_EVT_IPV4_IND:
        {
            struct {
                uint8_t ussid[33];
                uint8_t rssi;
                uint32_t ip;
                uint32_t mk;
                uint32_t gw;
                uint32_t dns;
                uint8_t vif_idx;
            } *cp_ind = (void *)msg->param;

            wdrv_host_env.wlan_link_sta_status = WIFI_LINKSTATE_STA_CONNECTED;
            wdrv_host_env.wlan_mode = WIFI_MODE_STA;
            os_memcpy(wdrv_host_env.connect_ind.ussid, cp_ind->ussid,
                      sizeof(wdrv_host_env.connect_ind.ussid));
            wdrv_host_env.connect_ind.rssi = (int8_t)cp_ind->rssi;
            wdrv_host_env.connect_ind.ip = cp_ind->ip;
            wdrv_host_env.connect_ind.mk = cp_ind->mk;
            wdrv_host_env.connect_ind.gw = cp_ind->gw;
            wdrv_host_env.connect_ind.dns = cp_ind->dns;
            wdrv_host_env.connect_ind.vif_idx = cp_ind->vif_idx;
            WDRV_LOGD(TAG, "WLAN-INDICATE: connected\n");
#if 0
            BK_LOGD(NULL, "WLAN-INDICATE: connect to \'%s\' (%3d dBm)\r\n",
                wdrv_host_env.connect_ind.ussid, wdrv_host_env.connect_ind.rssi);
            BK_LOGD(NULL, "ip: %d.%d.%d.%d, mk: %d.%d.%d.%d, gw: %d.%d.%d.%d, dns: %d.%d.%d.%d\n",
                (wdrv_host_env.connect_ind.ip >> 0 ) & 0xff, (wdrv_host_env.connect_ind.ip >> 8 ) & 0xff,
                (wdrv_host_env.connect_ind.ip >> 16) & 0xff, (wdrv_host_env.connect_ind.ip >> 24) & 0xff,
                (wdrv_host_env.connect_ind.mk >> 0 ) & 0xff, (wdrv_host_env.connect_ind.mk >> 8 ) & 0xff,
                (wdrv_host_env.connect_ind.mk >> 16) & 0xff, (wdrv_host_env.connect_ind.mk >> 24) & 0xff,
                (wdrv_host_env.connect_ind.gw >> 0 ) & 0xff, (wdrv_host_env.connect_ind.gw >> 8 ) & 0xff,
                (wdrv_host_env.connect_ind.gw >> 16) & 0xff, (wdrv_host_env.connect_ind.gw >> 24) & 0xff,
                (wdrv_host_env.connect_ind.dns >> 0 ) & 0xff, (wdrv_host_env.connect_ind.dns >> 8 ) & 0xff,
                (wdrv_host_env.connect_ind.dns >> 16) & 0xff, (wdrv_host_env.connect_ind.dns >> 24) & 0xff);
#endif
#if CONFIG_P2P
            if (cp_ind->vif_idx == 3) {
                wdrv_host_env.p2p_role = 2;
                wdrv_notify_gc_got_ipv4();
                break;
            }
#endif
#if CONFIG_WIFI_VNET_CONTROLLER
            wdrv_notify_sta_got_ipv4();
#endif
            break;
        }
        case BK_EVT_IPV6_IND:
#if CONFIG_IPV6
            if (!msg->param || msg->param_len < sizeof(struct wdrv_ipv6_ind)) {
                WDRV_LOGE(TAG, "invalid IPv6 ind, len=%u\n", msg->param_len);
                break;
            }
            os_memset(&wdrv_host_env.ipv6_ind, 0, sizeof(wdrv_host_env.ipv6_ind));
            os_memcpy(&wdrv_host_env.ipv6_ind, msg->param, sizeof(struct wdrv_ipv6_ind));
            if (wdrv_host_env.ipv6_ind.addr_count > MAX_IPV6_ADDRESSES_IN_MSG)
                wdrv_host_env.ipv6_ind.addr_count = MAX_IPV6_ADDRESSES_IN_MSG;
            WDRV_LOGD(TAG, "IPv6 address count: %d\n", wdrv_host_env.ipv6_ind.addr_count);
            wdrv_notify_sta_got_ipv6();
#endif
            break;
        case BK_EVT_DISCONNECT_IND:
            WDRV_LOGD("WLAN-INDICATE: disconected and stop send data\n");
            wdrv_notify_sta_disconnected(msg->param, msg->param_len);
            break;
        case BK_EVT_CUSTOMER_IND:
            WDRV_LOGD(TAG, "Smart Config\n");
            wdv_rx_handle_customer_event(msg->param, msg->param_len);
            break;
        case BK_EVT_START_AP_IND:
            os_memcpy(&wdrv_host_env.ap_status_cfm, msg->param, sizeof(struct wdrv_ap_status_cfm));
            if (wdrv_host_env.ap_status_cfm.status == CONTROLLER_AP_START) {
                WDRV_LOGD("MCU-AP-STATE: start AP Success\n");
#if 0
                BK_LOGD(NULL, "ip: %d.%d.%d.%d, mk: %d.%d.%d.%d, gw: %d.%d.%d.%d, dns: %d.%d.%d.%d\n",
                    (wdrv_host_env.ap_status_cfm.ip >> 0 ) & 0xff, (wdrv_host_env.ap_status_cfm.ip >> 8 ) & 0xff,
                    (wdrv_host_env.ap_status_cfm.ip >> 16) & 0xff, (wdrv_host_env.ap_status_cfm.ip >> 24) & 0xff,
                    (wdrv_host_env.ap_status_cfm.mk >> 0 ) & 0xff, (wdrv_host_env.ap_status_cfm.mk >> 8 ) & 0xff,
                    (wdrv_host_env.ap_status_cfm.mk >> 16) & 0xff, (wdrv_host_env.ap_status_cfm.mk >> 24) & 0xff,
                    (wdrv_host_env.ap_status_cfm.gw >> 0 ) & 0xff, (wdrv_host_env.ap_status_cfm.gw >> 8 ) & 0xff,
                    (wdrv_host_env.ap_status_cfm.gw >> 16) & 0xff, (wdrv_host_env.ap_status_cfm.gw >> 24) & 0xff,);
#endif
                wdrv_host_env.wlan_mode = WIFI_MODE_AP;
            } else if (wdrv_host_env.ap_status_cfm.status == CONTROLLER_AP_CLOSE) {
                WDRV_LOGE("MCU-AP-STATE: start AP Fail\n");
            }
            break;
        case BK_EVT_SCAN_WIFI_IND:
            WDRV_LOGI("BK_EVT_SCAN_WIFI_IND\n");
            wdrv_notify_scan_done(msg->param, msg->param_len);
            break;
        case BK_EVT_ASSOC_AP_IND:
            os_memcpy(&wdrv_host_env.ap_assoc_sta_addr_ind, msg->param, sizeof(struct wdrv_ap_assoc_sta_ind));
            WDRV_LOGD("AP-INDICATE: %x:%x:%x:%x:%x:%x connected\n",
                      wdrv_host_env.ap_assoc_sta_addr_ind.sub_sta_addr[0], wdrv_host_env.ap_assoc_sta_addr_ind.sub_sta_addr[1],
                      wdrv_host_env.ap_assoc_sta_addr_ind.sub_sta_addr[2], wdrv_host_env.ap_assoc_sta_addr_ind.sub_sta_addr[3],
                      wdrv_host_env.ap_assoc_sta_addr_ind.sub_sta_addr[4], wdrv_host_env.ap_assoc_sta_addr_ind.sub_sta_addr[5]);

            wdrv_notify_sap_sta_connected();
            break;
        case BK_EVT_DISASSOC_AP_IND:
            WDRV_LOGV("AP-INDICATE: disassoc\n");
            os_memcpy(&wdrv_host_env.ap_assoc_sta_addr_ind, msg->param, sizeof(struct wdrv_ap_assoc_sta_ind));
            WDRV_LOGV("%x:%x:%x:%x:%x:%x\n",
                      wdrv_host_env.ap_assoc_sta_addr_ind.sub_sta_addr[0], wdrv_host_env.ap_assoc_sta_addr_ind.sub_sta_addr[1],
                      wdrv_host_env.ap_assoc_sta_addr_ind.sub_sta_addr[2], wdrv_host_env.ap_assoc_sta_addr_ind.sub_sta_addr[3],
                      wdrv_host_env.ap_assoc_sta_addr_ind.sub_sta_addr[4], wdrv_host_env.ap_assoc_sta_addr_ind.sub_sta_addr[5]);

            wdrv_notify_sap_sta_disconnected();
            break;
        case BK_EVT_STOP_AP_IND:
            wdrv_host_env.ap_status_cfm.status = CONTROLLER_AP_CLOSE;
            wdrv_host_env.wlan_mode = WIFI_MODE_IDLE;
            WDRV_LOGV("MCU-AP-STATE: stop AP success\n");
            break;
        case BK_EVT_BCN_CC_RXED:
            bk_wifi_bcn_cc_rxed_cb(msg->param, msg->param_len);
        case BK_EVT_CSI_INFO_IND:
            bk_wifi_csi_info_cb(msg->param);
            break;
#if CONFIG_WIFI_VNET_CONTROLLER
        case BK_EVT_WIFI_EVENT_IND:
            wdrv_handle_wifi_event_ind((cif_wifi_event_ind_t *)msg->param);
            break;
#endif
#if CONFIG_P2P
        case BK_EVT_P2P_GO_START_IND:
            wdrv_host_env.p2p_role = 1;
            if (msg->param_len >= 6)
                p2p_go_ip_start_with_mac((const uint8_t *)msg->param);
            else
                p2p_go_ip_start();
            break;
        case BK_EVT_P2P_GO_STOP_IND:
            wdrv_host_env.p2p_role = 0;
            p2p_go_ip_down();
            break;
#endif
        default:
            WDRV_LOGD("%s msg %x invaild\n", __func__, msg->id);
            return;
    }
}
void wdrv_rx_handle_event(wdrv_rx_msg *msg)
{
    WDRV_LOGV("%s,%d\n",__func__,__LINE__);
    if ((msg->id >= BK_EVT_WIFI_API_START) && (msg->id <= BK_EVT_WIFI_API_END))
    {
        wdrv_rx_handle_wifi_api_event(msg);
    }
    else
    {
        wdrv_rx_handle_wifi_cntrl_event(msg);
    }
}

bk_err_t wdrv_host_init(void)
{
    bk_err_t ret = BK_OK;
    WDRV_LOGV("%s, %d\r\n", __func__, __LINE__);

    co_list_init((struct co_list *)&wdrv_host_env.cfm_pending_list);
    rtos_init_mutex(&wdrv_host_env.cfm_lock);

    if(wdrv_get_mac_addr() != 0)
        return BK_FAIL;
#if CONFIG_CONTROLLER_AP_BUFFER_COPY
    ret = wdrv_cntrl_get_cp_lwip_mem_addr();
#endif
    /* init wdrv common params */
    wdrv_param_init();

    /* Init Wi-Fi driver netif */
    //bk_wifi_init();
    return ret;
}