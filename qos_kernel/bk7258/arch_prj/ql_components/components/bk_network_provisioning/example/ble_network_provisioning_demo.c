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

#include "bk_network_provisioning.h"
#include <components/event.h>
#include <components/netif.h>
#include "bk_wifi.h"
#include "bk_wifi_types.h"
#include "bk_cli.h"
#include "os/str.h"
#include "components/log.h"
#include "cli.h"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define TAG "np_demo"

typedef enum
{
    BOARDING_OP_UNKNOWN = 0,
    BOARDING_OP_STATION_START = 1,
    BOARDING_OP_SOFT_AP_START = 2,
    BOARDING_OP_SERVICE_UDP_START = 3,
    BOARDING_OP_SERVICE_TCP_START = 4,
    BOARDING_OP_SET_CS2_DID = 5,
    BOARDING_OP_SET_CS2_APILICENSE = 6,
    BOARDING_OP_SET_CS2_KEY = 7,
    BOARDING_OP_SET_CS2_INIT_STRING = 8,
    BOARDING_OP_SRRVICE_CS2_START = 9,
    BOARDING_OP_BLE_DISABLE = 10,
    BOARDING_OP_SET_WIFI_CHANNEL = 11,
    BOARDING_OP_AGENT_RSP = 12,
    BOARDING_OP_SET_AGENT_INFO = 13,
    BOARDING_OP_NET_PAN_START = 14,
    BOARDING_OP_NETWORK_PROVISIONING_FIRST_TIME = 15,
    BOARDING_OP_RESERVED = 16,
    BOARDING_OP_START_BK_MODEM = 23,
    BOARDING_OP_START_WIFI_SCAN = 24,
    BOARDING_OP_SYNC_SUPPORTED_ENGINE = 25,
    BOARDING_OP_SYNC_SUPPORTED_NETWORK = 26,

    //device reserved opcode
    BOARDING_OP_NFC_GOT_ID = 150,

    //server reserved opcode
    BOARDING_OP_SERVER_CHECK_VERSION = 500,
    BOARDING_OP_MAX
} boarding_opcode_t;

//split packet to upload wifi scan result
bool enable_ble_split_pkt = false;

static int demo_np_wifi_sta_connect(char *ssid, char *key)
{
    int ssid_len, key_len;

    wifi_sta_config_t sta_config = {0};

    ssid_len = os_strlen(ssid);

    if (32 < ssid_len)
    {
        LOGW("ssid name more than 32 Bytes\r\n");
        return BK_FAIL;
    }

    os_strcpy(sta_config.ssid, ssid);

    key_len = os_strlen(key);

    if (64 < key_len || key_len < 8)
    {
        LOGW("Invalid passphrase, expected: 8..63\r\n");
        return BK_FAIL;
    }

    os_strcpy(sta_config.password, key);
#if CONFIG_STA_AUTO_RECONNECT
    sta_config.auto_reconnect_count = 5;
    sta_config.disable_auto_reconnect_after_disconnect = true;
#endif
    LOGI("ssid:%s key:%s\r\n", sta_config.ssid, sta_config.password);
    BK_LOG_ON_ERR(bk_wifi_sta_set_config(&sta_config));
    BK_LOG_ON_ERR(bk_wifi_sta_start());

    return BK_OK;
}

static int demo_np_wlan_scan_done_handler(void *arg, event_module_t event_module,
								  int event_id, void *event_data)
{
    wifi_scan_result_t scan_result = {0};
    char payload[200];
    uint16 len = 0;
    int i = 0, j = 0;

    BK_LOG_ON_ERR(bk_wifi_scan_get_result(&scan_result));
    if (scan_result.ap_num == 0)
        goto exit;

again:
    os_memset(payload, 0, 200);
    len = os_snprintf(payload, 200, "[");
    for (i = j; i < scan_result.ap_num; i++) {
        if (!os_strlen(scan_result.aps[i].ssid))
            continue;
        if ((len + 5 + os_strlen(scan_result.aps[i].ssid)) > 200) {
            j = i;
            break;
        }
        if ((i != 0) && (len != 1))
            len += os_snprintf(payload+len, 200, ",");
        len += os_snprintf(payload+len, 200, "\"%s\"", scan_result.aps[i].ssid);
        j = i + 1;
    }
    len += os_snprintf(payload+len, 200, "]");
    LOGI("upload scan_rst %s, sended:%d, total:%d\r\n", payload, j, scan_result.ap_num);
    if ((j >= scan_result.ap_num) || (enable_ble_split_pkt == false))
        bk_ble_provisioning_event_notify_with_data(BOARDING_OP_START_WIFI_SCAN, 0, payload, len);
    else {
        bk_ble_provisioning_event_notify_with_data(BOARDING_OP_START_WIFI_SCAN, 1, payload, len);
        goto again;
    }

exit:
    bk_wifi_scan_free_result(&scan_result);

    return BK_OK;
}

void ble_msg_handle_demo_cb(ble_prov_msg_t *msg)
{
    switch (msg->event)
    {
        case BOARDING_OP_STATION_START:
        {
            bk_ble_provisioning_info_t *bk_ble_provisioning_info = bk_ble_provisioning_get_boarding_info();
            demo_np_wifi_sta_connect(bk_ble_provisioning_info->ble_prov_info.ssid_value,
                                        bk_ble_provisioning_info->ble_prov_info.password_value);
            bk_event_unregister_cb(EVENT_MOD_WIFI, EVENT_WIFI_SCAN_DONE,
                                                        demo_np_wlan_scan_done_handler);
        }
        break;

        case BOARDING_OP_START_WIFI_SCAN:
        {
            LOGI("BOARDING_OP_START_WIFI_SCAN\n");
            if (msg->param) {
                if (*(uint8_t *)msg->param == 1)
                    enable_ble_split_pkt = true;
                else
                    enable_ble_split_pkt = false;
            }
            bk_event_register_cb(EVENT_MOD_WIFI, EVENT_WIFI_SCAN_DONE,
                                        demo_np_wlan_scan_done_handler, NULL);
            BK_LOG_ON_ERR(bk_wifi_scan_start(NULL));
        }
        break;

        case BOARDING_OP_BLE_DISABLE:
        {
            LOGI("close bluetooth ing\n");
#if CONFIG_BLUETOOTH
            bk_ble_provisioning_deinit();
            bk_bluetooth_deinit();
            LOGI("close bluetooth finish!\r\n");
#endif
        }
        break;

        case BOARDING_OP_SYNC_SUPPORTED_NETWORK:
        {
            uint8_t *val = NULL, len = 0;
            val = bk_sconf_get_supported_network(&len);
            bk_ble_provisioning_event_notify_with_data(BOARDING_OP_SYNC_SUPPORTED_NETWORK, 0, (char *)val, len);
            if (val)
                os_free(val);
        }
        break;

        default:
        {
            LOGI("%s %d, do nothing\r\n", __func__, msg->event);
        }
        break;
    }
}

void demo_network_provisioning_status_cb(bk_network_provisioning_status_t status, void *user_data)
{
    LOGI("demo network provisioning status: %d\n", status);
    switch (status)
    {
        case BK_NETWORK_PROVISIONING_STATUS_IDLE:
            break;
        case BK_NETWORK_PROVISIONING_STATUS_RUNNING:
            break;
        case BK_NETWORK_PROVISIONING_STATUS_SUCCEED:
            if (bk_network_provisioning_get_type() == BK_NETWORK_PROVISIONING_TYPE_BLE) {
                netif_if_t netif_idx = (netif_if_t)user_data;
                netif_ip4_config_t ip4_config = {0};

                bk_netif_get_ip4_config(netif_idx, &ip4_config);
                LOGI("netif_idx:%d, ip: %s\n", netif_idx, ip4_config.ip);
                bk_ble_provisioning_event_notify_with_data(BOARDING_OP_STATION_START, BK_OK, ip4_config.ip, strlen(ip4_config.ip));
            }
            break;
        case BK_NETWORK_PROVISIONING_STATUS_FAILED:
            break;
        case BK_NETWORK_PROVISIONING_STATUS_RECONNECTING:
            break;
        case BK_NETWORK_PROVISIONING_STATUS_RECONNECT_FAILED:
            break;
        case BK_NETWORK_PROVISIONING_STATUS_RECONNECT_SUCCEED:
            break;
        default:
            break;
    }
}

static void cli_network_provisioning(char *pcWriteBuffer, int xWriteBufferLen, int argC, char **argV)
{
    if (argC == 1) {
        bk_network_provisioning_start(BK_NETWORK_PROVISIONING_TYPE_BLE);
    } else if (argC == 2) {
        if (os_strcmp(argV[1], "ble") == 0) {
            bk_network_provisioning_start(BK_NETWORK_PROVISIONING_TYPE_BLE);
        } else if (os_strcmp(argV[1], "console") == 0) {
            bk_network_provisioning_start(BK_NETWORK_PROVISIONING_TYPE_CONSOLE);
        }
    }
}

static void cli_erase_network_provisioning_info(char *pcWriteBuffer, int xWriteBufferLen, int argC, char **argV)
{
    erase_network_auto_reconnect_info();
}

int demo_netif_event_cb(void *arg, event_module_t event_module,
					   int event_id, void *event_data)
{
	netif_event_got_ip4_t *got_ip;

	switch (event_id) {
	case EVENT_NETIF_GOT_IP4:
		got_ip = (netif_event_got_ip4_t *)event_data;
		LOGD("%s got ip\n", got_ip->netif_if == NETIF_IF_STA ? "BK STA" : "unknown netif");
		break;
	default:
		LOGD("rx event <%d %d>\n", event_module, event_id);
		break;
	}

	return BK_OK;
}

int demo_wifi_event_cb(void *arg, event_module_t event_module,
					  int event_id, void *event_data)
{
	wifi_event_sta_disconnected_t *sta_disconnected;
	wifi_event_sta_connected_t *sta_connected;
	wifi_event_ap_disconnected_t *ap_disconnected;
	wifi_event_ap_connected_t *ap_connected;
	wifi_event_network_found_t *network_found;

	switch (event_id) {
	case EVENT_WIFI_STA_CONNECTED:
		sta_connected = (wifi_event_sta_connected_t *)event_data;
		LOGD("BK STA connected %s\n", sta_connected->ssid);
		break;

	case EVENT_WIFI_STA_DISCONNECTED:
		sta_disconnected = (wifi_event_sta_disconnected_t *)event_data;
		LOGD("BK STA disconnected, reason(%d)%s\n", sta_disconnected->disconnect_reason,
			sta_disconnected->local_generated ? ", local_generated" : "");
		break;

	case EVENT_WIFI_AP_CONNECTED:
		ap_connected = (wifi_event_ap_connected_t *)event_data;
		LOGD(BK_MAC_FORMAT" connected to BK AP\n", BK_MAC_STR(ap_connected->mac));
		break;

	case EVENT_WIFI_AP_DISCONNECTED:
		ap_disconnected = (wifi_event_ap_disconnected_t *)event_data;
		LOGD(BK_MAC_FORMAT" disconnected from BK AP\n", BK_MAC_STR(ap_disconnected->mac));
		break;

	case EVENT_WIFI_NETWORK_FOUND:
		network_found = (wifi_event_network_found_t *)event_data;
		LOGD(" target AP: %s, bssid %pm found\n", network_found->ssid, network_found->bssid);
		break;

	default:
		LOGD("rx event <%d %d>\n", event_module, event_id);
		break;
	}

	return BK_OK;
}
#define NP_CMD_COUNT (sizeof(s_network_provisioning_commands) / sizeof(s_network_provisioning_commands[0]))
static const struct cli_command s_network_provisioning_commands[] = {
    {"np", "np or np [ble]|[console]", cli_network_provisioning},
    {"np_erase", "np_erase", cli_erase_network_provisioning_info},
};

int cli_network_provisioning_init(void)
{
    //bk_event_register_cb(EVENT_MOD_WIFI, EVENT_ID_ALL, demo_wifi_event_cb, NULL);
    //bk_event_register_cb(EVENT_MOD_NETIF, EVENT_ID_ALL, demo_netif_event_cb, NULL);
    return cli_register_commands(s_network_provisioning_commands, NP_CMD_COUNT);
}