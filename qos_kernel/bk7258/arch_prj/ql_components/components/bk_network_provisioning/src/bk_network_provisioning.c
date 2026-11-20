// Copyright 2020-2025 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <components/log.h>
#include <components/event.h>
#include "os/os.h"
#include "os/mem.h"
#include "os/str.h"
#include "bk_network_provisioning.h"
#include "bk_wifi.h"
#include "lwip/ip_addr.h"
#include "lwip/dns.h"
#include "lwip/inet.h"
#include "port/net.h"

//TODO np
#if (CONFIG_EASY_FLASH && CONFIG_EASY_FLASH_V4)
#include "bk_ef.h"
#endif

#if CONFIG_BK_BLE_PROVISIONING
#include "ble_provisioning_priv.h"
#include "ble_provisioning.h"
#endif

#if CONFIG_NET_PAN
#include "pan_service.h"
#endif
#include "components/bluetooth/bk_dm_bluetooth.h"

static beken_thread_t network_provisioning_thread_handle = NULL;
static bk_network_provisioning_type_t config_network_type = BK_NETWORK_PROVISIONING_TYPE_MAX;
static bk_network_provisioning_status_t network_provisioning_status = BK_NETWORK_PROVISIONING_STATUS_IDLE;
static bool first_time_for_network_provisioning = true;
static bool first_time_for_network_reconnect = true;
static beken2_timer_t network_status_check_tmr = {0};
static uint8_t network_disc_evt_posted = 0;
static network_provisioning_status_cb_t network_provisioning_status_cb = NULL;

#define TAG "bk_nw_pro"

bk_err_t bk_register_network_provisioning_status_cb(network_provisioning_status_cb_t cb)
{
    BK_LOGD(TAG,"register network provisioning status cb\n");
    if (cb == NULL) {
        BK_LOGW(TAG,"network provisioning status cb is NULL\n");
        return BK_FAIL;
    }
    network_provisioning_status_cb = cb;
    return BK_OK;
}

bk_err_t bk_unregister_network_provisioning_status_cb(void)
{
    BK_LOGD(TAG,"unregister network provisioning status cb\n");
    network_provisioning_status_cb = NULL;
    return BK_OK;
}

static void bk_network_provisioning_update_status(bk_network_provisioning_status_t status, void *user_data)
{
    BK_LOGD(TAG,"network provisioning update status:%d\n", status);
    network_provisioning_status = status;
    if (network_provisioning_status_cb == NULL) {
        BK_LOGI(TAG,"network provisioning status cb is NULL\n");
        return;
    }
    network_provisioning_status_cb(status, user_data);
}

static void network_status_check(void)
{
    BK_LOGI(TAG,"reconnect timeout!\n");
    if (network_disc_evt_posted == 0) {
        bk_network_provisioning_update_status(BK_NETWORK_PROVISIONING_STATUS_RECONNECT_FAILED, NULL);
        network_disc_evt_posted = 1;
    }
}

void network_status_check_start_timeout_check(uint32_t timeout)
{
  bk_err_t err = kNoErr;
  uint32_t clk_time;

  clk_time = timeout*1000;		//timeout unit: seconds

  if (rtos_is_oneshot_timer_init(&network_status_check_tmr)) {
    BK_LOGD(TAG,"network provisioning status timer reload\n");
    rtos_oneshot_reload_timer(&network_status_check_tmr);
  } else {
    err = rtos_init_oneshot_timer(&network_status_check_tmr, clk_time, (timer_2handler_t)network_status_check, NULL, NULL);
    BK_ASSERT(kNoErr == err);

    err = rtos_start_oneshot_timer(&network_status_check_tmr);
    BK_ASSERT(kNoErr == err);
    BK_LOGD(TAG,"network provisioning status timer:%d\n", clk_time);
  }

  return;
}

void network_status_check_stop_timeout_check(void)
{
  bk_err_t ret = kNoErr;

  if (rtos_is_oneshot_timer_init(&network_status_check_tmr)) {
    if (rtos_is_oneshot_timer_running(&network_status_check_tmr)) {
      ret = rtos_stop_oneshot_timer(&network_status_check_tmr);
      BK_ASSERT(kNoErr == ret);
    }

    ret = rtos_deinit_oneshot_timer(&network_status_check_tmr);
    BK_ASSERT(kNoErr == ret);
  }
}

static int save_network_auto_restart_info(netif_if_t type)
{
	BK_FAST_CONNECT_D info_tmp = {0};
	__maybe_unused wifi_ap_config_t ap_config = {0};
    __maybe_unused wifi_sta_config_t sta_config = {0};
    //TODO np
#if 0
	bk_config_read("d_network_id", (void *)&info_tmp, sizeof(BK_FAST_CONNECT_D));
#else
    bk_get_env_enhance("d_network_id", (void *)&info_tmp, sizeof(BK_FAST_CONNECT_D));
#endif
    /*if max number of netifs is more than 15, should improve below code*/
    if (NETIF_IF_INVALID >= 15) {
        BK_LOGI(TAG, "max number of netifs reached limit");
        return BK_FAIL;
    }
	if ((info_tmp.flag & 0x8000l) != 0x8000l) {
		BK_LOGD(TAG, "erase network provisioning info, %x\r\n", info_tmp.flag);
		info_tmp.flag = 0x8000l;
	}
	if (type == NETIF_IF_STA) {
		info_tmp.flag |= BIT(NETIF_IF_STA);
		bk_wifi_sta_get_config(&sta_config);
		os_memset((char *)info_tmp.sta_ssid, 0x0, 33);
		os_memset((char *)info_tmp.sta_pwd, 0x0, 65);
		os_strcpy((char *)info_tmp.sta_ssid, (char *)sta_config.ssid);
		os_strcpy((char *)info_tmp.sta_pwd, (char *)sta_config.password);
	} else if (type == NETIF_IF_AP) {
		info_tmp.flag |= BIT(NETIF_IF_AP);
		bk_wifi_ap_get_config(&ap_config);
		os_memset((char *)info_tmp.ap_ssid, 0x0, 33);
		os_memset((char *)info_tmp.ap_pwd, 0x0, 65);
		os_strcpy((char *)info_tmp.ap_ssid, (char *)ap_config.ssid);
		os_strcpy((char *)info_tmp.ap_pwd, (char *)ap_config.password);
#if CONFIG_NET_PAN
	} else if (type == NETIF_IF_PAN) {
		info_tmp.flag |= BIT(NETIF_IF_PAN);
#endif
#if CONFIG_BK_MODEM
	} else if (type == NETIF_IF_PPP) {
		info_tmp.flag |= BIT(NETIF_IF_PPP);
#endif
#if CONFIG_P2P
	} else if (type == NETIF_IF_P2P) {
		info_tmp.flag |= BIT(NETIF_IF_P2P);
		// 获取并保存 P2P 设备名称
		const char *p2p_dev_name = bk_wifi_get_p2p_dev_name();
		os_memset((char *)info_tmp.p2p_dev_name, 0x0, 33);
		if (p2p_dev_name != NULL) {
			os_strcpy((char *)info_tmp.p2p_dev_name, p2p_dev_name);
			BK_LOGI(TAG, "Save P2P device name: %s\n", p2p_dev_name);
		} else {
			BK_LOGW(TAG, "P2P device name is NULL, using empty string\n");
		}
		info_tmp.p2p_go_intent = BK_NW_PRO_P2P_GO_INTENT_DEFAULT;
		BK_LOGI(TAG, "Save P2P GO intent: %u\n", info_tmp.p2p_go_intent);
#endif
	} else
		return -1;
    //TODO np
#if 0
	bk_config_write("d_network_id", (const void *)&info_tmp, sizeof(BK_FAST_CONNECT_D));
#else
    bk_set_env_enhance("d_network_id", (const void *)&info_tmp, sizeof(BK_FAST_CONNECT_D));
#endif

	return 0;
}

void erase_network_auto_reconnect_info(void)
{
	BK_FAST_CONNECT_D info_tmp = {0};
    //TODO np
#if 0
	bk_config_write("d_network_id", (const void *)&info_tmp, sizeof(BK_FAST_CONNECT_D));
#else
    bk_set_env_enhance("d_network_id", (const void *)&info_tmp, sizeof(BK_FAST_CONNECT_D));
#endif
}

static int bk_nw_pro_netif_event_cb(void *arg, event_module_t event_module, int event_id, void *event_data)
{
    netif_event_got_ip4_t *got_ip;

    switch (event_id)
    {
        case EVENT_NETIF_GOT_IP4:
            network_disc_evt_posted = 0;
            got_ip = (netif_event_got_ip4_t *)event_data;
            BK_LOGI(TAG, "netif_idx %d\r got ip\n", got_ip->netif_if);
            if (got_ip->netif_if == 0) {
                const ip_addr_t *tmp_dns_server = NULL;
                tmp_dns_server = dns_getserver(0);
                if (ip_addr_get_ip4_u32(tmp_dns_server) == INADDR_ANY) {
                    bk_netif_add_dns_server(0, "8.8.8.8"); //google 1st dns
                    bk_netif_add_dns_server(1, "9.9.9.9"); //IBM quad9 dns
                }
            }
#if CONFIG_BK_MODEM
            {
             extern void ping_start(char* target_name, uint32_t times, size_t size);
             ping_start("baidu.com", 4, 0);
            }
#endif
            network_status_check_stop_timeout_check();
            if (network_provisioning_status == BK_NETWORK_PROVISIONING_STATUS_RUNNING)
            {
                save_network_auto_restart_info(got_ip->netif_if);
                bk_network_provisioning_update_status(BK_NETWORK_PROVISIONING_STATUS_SUCCEED, (void *)got_ip->netif_if);
            }
            else
            {
                bk_network_provisioning_update_status(BK_NETWORK_PROVISIONING_STATUS_RECONNECT_SUCCEED, (void *)got_ip->netif_if);
            }

            break;
        default:
            BK_LOGI(TAG, "rx event <%d %d>\n", event_module, event_id);
            break;
    }

    return BK_OK;
}

#if CONFIG_P2P
static void bk_nw_pro_report_go_client_connected(void)
{
    netif_if_t netif_type = NETIF_IF_P2P;

    BK_LOGI(TAG, "GO connected, as P2P GO\n");
    bk_wifi_p2p_stop_find();
    if (network_provisioning_status == BK_NETWORK_PROVISIONING_STATUS_RUNNING) {
        save_network_auto_restart_info(netif_type);
        bk_network_provisioning_update_status(BK_NETWORK_PROVISIONING_STATUS_SUCCEED, (void *)netif_type);
    } else {
        bk_network_provisioning_update_status(BK_NETWORK_PROVISIONING_STATUS_RECONNECT_SUCCEED, (void *)netif_type);
    }
}
#endif

static int bk_nw_pro_wifi_event_cb(void *arg, event_module_t event_module, int event_id, void *event_data)
{
    wifi_event_sta_disconnected_t *sta_disconnected;
    wifi_event_sta_connected_t *sta_connected;

    switch (event_id)
    {
        case EVENT_WIFI_STA_CONNECTED:
            sta_connected = (wifi_event_sta_connected_t *)event_data;
            BK_LOGI(TAG, "STA connected to %s\n", sta_connected->ssid);
            break;

        case EVENT_WIFI_AP_CONNECTED:
        {
#if CONFIG_P2P
            netif_if_t netif_type = bk_wifi_is_p2p_enabled() ? NETIF_IF_P2P : NETIF_IF_AP;
            if (bk_wifi_is_p2p_enabled()) {
                BK_LOGI(TAG, "GO connected, as P2P GO\n");
            } else {
                BK_LOGI(TAG, "AP connected, not in P2P mode\n");
            }
#else
            netif_if_t netif_type = NETIF_IF_AP;
            BK_LOGI(TAG, "AP connected\n");
#endif
            if (network_provisioning_status == BK_NETWORK_PROVISIONING_STATUS_RUNNING) {
                save_network_auto_restart_info(netif_type);
                bk_network_provisioning_update_status(BK_NETWORK_PROVISIONING_STATUS_SUCCEED, (void *)netif_type);
            } else {
                bk_network_provisioning_update_status(BK_NETWORK_PROVISIONING_STATUS_RECONNECT_SUCCEED, (void *)netif_type);
            }
            break;
        }

        case EVENT_WIFI_AP_DISCONNECTED:
#if CONFIG_P2P
            if (bk_wifi_is_p2p_enabled()) {
                BK_LOGI(TAG, "GO disconnected, as P2P GO\n");
                // 执行 P2P 相关的逻辑
            } else {
                BK_LOGI(TAG, "AP disconnected, not in P2P mode\n");
                // 执行常规 WiFi 逻辑
            }
#else
            BK_LOGI(TAG, "AP disconnected");
#endif
            break;

        case EVENT_WIFI_STA_DISCONNECTED:
            sta_disconnected = (wifi_event_sta_disconnected_t *)event_data;
            BK_LOGI(TAG, "STA disconnected, reason(%d)\n", sta_disconnected->disconnect_reason);
            /*drop local generated disconnect event by user*/
            if ((sta_disconnected->disconnect_reason == WIFI_REASON_DEAUTH_LEAVING &&
				sta_disconnected->local_generated == 1) ||
				(sta_disconnected->disconnect_reason == WIFI_REASON_RESERVED))
			break;
#if CONFIG_STA_AUTO_RECONNECT
            if (bk_sconf_is_net_pan_configured() && !smart_config_running) {
#if 0//CONFIG_NET_PAN
			bk_sconf_reselect_pan();
#endif
            } else
#endif
            {
			if (network_disc_evt_posted == 0) {
				if (network_provisioning_status == BK_NETWORK_PROVISIONING_STATUS_RUNNING) {
                    bk_network_provisioning_update_status(BK_NETWORK_PROVISIONING_STATUS_FAILED, NULL);
				} else {
					bk_network_provisioning_update_status(BK_NETWORK_PROVISIONING_STATUS_RECONNECT_FAILED, NULL);
				}
				network_disc_evt_posted = 1;
			}
#if CONFIG_STA_AUTO_RECONNECT
			bk_wifi_sta_connect();
#endif
            }
            break;

#if CONFIG_P2P
        case EVENT_WIFI_GO_CONNECTED:
            bk_nw_pro_report_go_client_connected();
            break;

        case EVENT_WIFI_GO_DISCONNECTED:
            BK_LOGI(TAG, "P2P GO: client disconnected, GO kept up\n");
            break;

        case EVENT_WIFI_GC_CONNECTED:
            BK_LOGI(TAG, "P2P GC: connected to remote GO\n");
            break;

        case EVENT_WIFI_GC_DISCONNECTED:
            BK_LOGI(TAG, "P2P GC: disconnected, restart find\n");
            bk_wifi_p2p_find();
            break;
#endif

        default:
            BK_LOGI(TAG, "rx event <%d %d>\n", event_module, event_id);
            break;
    }

    return BK_OK;
}

static void bk_nw_pro_event_handler_init(void)
{
    BK_LOG_ON_ERR(bk_event_register_cb(EVENT_MOD_WIFI, EVENT_ID_ALL, bk_nw_pro_wifi_event_cb, NULL));
    BK_LOG_ON_ERR(bk_event_register_cb(EVENT_MOD_NETIF, EVENT_ID_ALL, bk_nw_pro_netif_event_cb, NULL));
}

static void bk_nw_pro_common_init(void)
{
    network_provisioning_status = BK_NETWORK_PROVISIONING_STATUS_RUNNING;
    first_time_for_network_reconnect = true;
    first_time_for_network_provisioning = true;
    bk_network_provisioning_update_status(BK_NETWORK_PROVISIONING_STATUS_RUNNING, NULL);
    network_status_check_stop_timeout_check();
    bk_wifi_sta_stop();
#if CONFIG_BK_MODEM
extern bk_err_t bk_modem_deinit(void);
    bk_modem_deinit();
#endif
}

static void network_provisioning_main(void)
{
    bk_nw_pro_common_init();
    BK_LOGI(TAG, "network_provisioning_main config_network_type = %d\n", config_network_type);
    switch(config_network_type)
    {
#if CONFIG_BK_BLE_PROVISIONING
        case BK_NETWORK_PROVISIONING_TYPE_BLE:
            bk_ble_np_init();
            break;
#endif
#if CONFIG_BK_AIRKISS
        case BK_NETWORK_PROVISIONING_TYPE_AIRKISS:
            break;
#endif
#if CONFIG_BK_WEB
        case BK_NETWORK_PROVISIONING_TYPE_WEB:
            break;
#endif
        case BK_NETWORK_PROVISIONING_TYPE_CONSOLE:
            break;
        default:
            break;
    }
    network_provisioning_thread_handle = NULL;
    rtos_delete_thread(NULL);
}

static int np_sta_start(char *oob_ssid, char *connect_key)
{
	wifi_sta_config_t sta_config = {0};
	int len;

	len = os_strlen(oob_ssid);
	if (32 < len) {
		BK_LOGI(TAG, "ssid name more than 32 Bytes\r\n");
		return BK_FAIL;
	}
	os_strcpy(sta_config.ssid, oob_ssid);
	os_strcpy(sta_config.password, connect_key);

#if CONFIG_STA_AUTO_RECONNECT
	sta_config.auto_reconnect_count = 5;
	sta_config.disable_auto_reconnect_after_disconnect = true;
#endif

	BK_LOGI(TAG, "ssid:%s key:%s\r\n", sta_config.ssid, sta_config.password);
	BK_LOG_ON_ERR(bk_wifi_sta_set_config(&sta_config));
	BK_LOG_ON_ERR(bk_wifi_sta_start());
	return BK_OK;
}

static int np_softap_start(char *ap_ssid, char *ap_key, uint8_t ap_channel)
{
	wifi_ap_config_t ap_config = {0};//WIFI_DEFAULT_AP_CONFIG();
	netif_ip4_config_t ip4_config = {0};
	int len, key_len = 0;
	len = os_strlen(ap_ssid);
	if (ap_key)
		key_len = os_strlen(ap_key);
	if (32 < len || 0 == len) {
		BK_LOGE(TAG, "invalid ssid name length\r\n");
		return BK_FAIL;
	}
	if (8 > key_len)
		BK_LOGI(TAG, "key less than 8 Bytes, the security will be set NONE\r\n");
	if (64 < key_len) {
		BK_LOGE(TAG, "key more than 64 Bytes\r\n");
		return BK_FAIL;
	}
    os_strcpy(ip4_config.ip, WLAN_DEFAULT_IP);
    os_strcpy(ip4_config.mask, WLAN_DEFAULT_MASK);
    os_strcpy(ip4_config.gateway, WLAN_DEFAULT_GW);
    os_strcpy(ip4_config.dns, WLAN_DEFAULT_GW);
	BK_RETURN_ON_ERR(bk_netif_set_ip4_config(NETIF_IF_AP, &ip4_config));
	os_strcpy(ap_config.ssid, ap_ssid);
	if (ap_key)
		os_strcpy(ap_config.password, ap_key);
	ap_config.channel = ap_channel;
	BK_LOGI(TAG, "ssid:%s  key:%s\r\n", ap_config.ssid, ap_config.password);
	BK_RETURN_ON_ERR(bk_wifi_ap_set_config(&ap_config));
	BK_RETURN_ON_ERR(bk_wifi_ap_start());
	return BK_OK;
}

static netif_if_t bk_network_auto_reconnect(bool val)	//val true means from disconnect to reconnecting
{
	BK_FAST_CONNECT_D info = {0};
    netif_if_t netif_if = NETIF_IF_INVALID;
    //TODO np
#if 0
	//bk_config_read("d_network_id", (void *)&info, sizeof(BK_FAST_CONNECT_D));
#else
    bk_get_env_enhance("d_network_id", (void *)&info, sizeof(BK_FAST_CONNECT_D));
#endif
    if ((info.flag & 0x8000l) != 0x8000l) {
		BK_LOGI(TAG, "Please do network provisioning fisrtly\r\n");
		return NETIF_IF_INVALID;
	}
	if (info.flag & BIT(NETIF_IF_STA)) {
		np_sta_start((char *)info.sta_ssid, (char *)info.sta_pwd);
		netif_if = NETIF_IF_STA;
	}
	else if (info.flag & BIT(NETIF_IF_AP)) {
		np_softap_start((char *)info.ap_ssid, (char *)info.ap_pwd, info.ap_channel);
		netif_if = NETIF_IF_AP;
	}
#if CONFIG_NET_PAN
	else if (info.flag & BIT(NETIF_IF_PAN)) {
		pan_service_init();
		bt_start_pan_reconnect();
		netif_if = NETIF_IF_PAN;
	}
#endif
#if CONFIG_BK_MODEM
	else if (info.flag & BIT(NETIF_IF_PPP)) {
		extern bk_err_t bk_modem_init(uint8_t comm_proto, uint8_t comm_if);
		bk_modem_init(1, 1);
		netif_if = NETIF_IF_PPP;
	}
#endif
#if CONFIG_P2P
	else if (info.flag & BIT(NETIF_IF_P2P)) {
		int p2p_intent = info.p2p_go_intent;

		if (p2p_intent <= 0 || p2p_intent > 15) {
			p2p_intent = BK_NW_PRO_P2P_GO_INTENT_DEFAULT;
		}
		BK_LOGI(TAG, "P2P auto-reconnect: dev=%s intent=%d\n",
			info.p2p_dev_name, p2p_intent);
		bk_wifi_p2p_enable_with_intent((char *)info.p2p_dev_name, p2p_intent);
		bk_wifi_p2p_find();
		netif_if = NETIF_IF_P2P;
	}
#endif

    if (netif_if != NETIF_IF_INVALID) {
        if (val == false) {
			network_status_check_stop_timeout_check();
			network_status_check_start_timeout_check(50);    //50s
			bk_network_provisioning_update_status(BK_NETWORK_PROVISIONING_STATUS_RECONNECTING, NULL);
			network_disc_evt_posted = 0;
		}
    }
	return netif_if;
}

uint8_t *bk_sconf_get_supported_network(uint8_t *len)
{
    uint8_t tmp_val[3] = {0}, i = 0, *val = NULL;

//#ifdef CONFIG_WIFI_ENABLE
    tmp_val[i++] = 0;
//#endif
#ifdef CONFIG_BK_MODEM
    tmp_val[i++]  = 1;
#endif
#ifdef CONFIG_NET_PAN
    tmp_val[i++]  = 2;
#endif
    val = os_zalloc(i);
    os_memcpy(val, tmp_val, i);
    *len = i;
    return val;
}

bk_network_provisioning_type_t bk_network_provisioning_get_type(void)
{
    return config_network_type;
}

void bk_network_provisioning_get_send_cb(
    void (**send)(uint16_t opcode, int status),
    void (**send_with_data)(uint16_t opcode, int status, char *payload, uint16_t length)
)
{
#if CONFIG_BK_BLE_PROVISIONING
    if(send)
    {
        switch(config_network_type)
        {
        case BK_NETWORK_PROVISIONING_TYPE_BLE:
            *send = bk_ble_provisioning_event_notify;
            break;

        default:
            BK_LOGE(TAG, "unknow network type %d\r\n", config_network_type);
            return;
            break;
        }
    }

    if(send_with_data)
    {
        switch(config_network_type)
        {
        case BK_NETWORK_PROVISIONING_TYPE_BLE:
            *send_with_data = bk_ble_provisioning_event_notify_with_data;
            break;

        default:
            BK_LOGE(TAG, "unknow network type %d\r\n", config_network_type);
            return;
            break;
        }
    }
#else
    BK_LOGE(TAG, "%s component not enable\n", __func__);
#endif
}

/*used for press button to start network provisioning*/
bk_err_t bk_network_provisioning_start(bk_network_provisioning_type_t type)
{
    int ret = 0;

    network_disc_evt_posted = 0;

    if (network_provisioning_thread_handle) {
        BK_LOGI(TAG, "Network provisioning ongoing, please try later!\n");
        return BK_OK;
    }
    BK_LOGI(TAG, "Start to network provisioning!\n");
    config_network_type = type;
    ret = rtos_create_thread(&network_provisioning_thread_handle,
                                BEKEN_APPLICATION_PRIORITY,
                                "wifi_network_provisioning",
                                (beken_thread_function_t)network_provisioning_main,
                                4096,
                                NULL);
    if (ret != kNoErr)
    {
        BK_LOGE(TAG, "wifi network provisioning task fail: %d\r\n", ret);
        network_provisioning_thread_handle = NULL;
    }
    return BK_OK;
}

/*used in app_main.c when system bootup*/
bk_err_t bk_network_provisioning_init(bk_network_provisioning_type_t default_type)
{
    netif_if_t netif_if = NETIF_IF_INVALID;

    bk_nw_pro_event_handler_init();
    netif_if = bk_network_auto_reconnect(false);

    if (netif_if == NETIF_IF_INVALID)
    {
        if (default_type < BK_NETWORK_PROVISIONING_TYPE_MAX)
        {
            bk_network_provisioning_start(default_type);
        }
        else
        {
#if CONFIG_BK_BLE_PROVISIONING
            /*default use BLE network provisioning*/
            bk_network_provisioning_start(BK_NETWORK_PROVISIONING_TYPE_BLE);
#endif
        }
    }
    else
    {
#if CONFIG_NET_PAN
        if (netif_if != NETIF_IF_PAN)
#endif
        {
#if CONFIG_NET_PAN && !(CONFIG_A2DP_SINK_DEMO || CONFIG_HFP_HF_DEMO)
            bk_bluetooth_deinit();
#endif
        }
    }

    return BK_OK;
}
