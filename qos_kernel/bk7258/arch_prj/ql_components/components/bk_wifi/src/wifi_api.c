/*
 * Copyright 2020-2025 Beken
 *
 * @file wdrv_api.c 
 * 
 * @brief Beken Wi-Fi Driver Command Control Entry
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

#include "wifi_api.h"
#include "wdrv_main.h"
#include <common/bk_include.h>
#include <common/bk_err.h>
#include <stdint.h>
#include <stdbool.h>
#include <os/str.h>
#include <os/mem.h>
#include <os/os.h>
#include "net.h"
#include "wdrv_cntrl.h"
#include "wdrv_co_list.h"
#include "wdrv_tx.h"
#include "wifi_api_ipc.h"
#include "wdrv_cntrl.h"
#ifdef CONFIG_WIFI_VNET_CONTROLLER
#include <components/netif.h>
#endif
#if CONFIG_NETIF_LWIP
#include "lwip/inet.h"
#include "net.h"
#include "lwip/netif.h"
#endif
#if CONFIG_BK_RAW_LINK
#include "raw_link_api.h"
#endif

general_param_t *g_wlan_general_param = NULL;
ap_param_t *g_ap_param_ptr = NULL;
sta_param_t *g_sta_param_ptr = NULL;
struct scan_cfg_scan_param_tag scan_param_env = {0};

#ifdef CONFIG_WIFI_VNET_CONTROLLER
static bk_err_t bk_wifi_sta_get_ip4_config_from_cp(netif_ip4_config_t *ip_config);
static bk_err_t bk_wifi_ap_get_ip4_config_from_cp(netif_ip4_config_t *ip_config);
bk_err_t bk_wifi_sync_ip4_config_from_cp(void);
#endif

static wifi_monitor_cb_t s_monitor_ap_cb = NULL;
static wifi_filter_cb_t s_filter_ap_cb = NULL;
/* State Indication */
static uint16_t s_wifi_state_bits = 0;
#if CONFIG_P2P
static bool s_wifi_p2p_enabled = false;
static char s_wifi_p2p_dev_name[SSID_MAX_LEN + 1] = {0};
/* Default GO Intent for P2P negotiation, set via bk_wifi_p2p_enable()
 * -1 means "use underlying default (usually 15)" */
static int s_wifi_p2p_default_intent = -1;

static inline void wifi_p2p_set_enabled(bool enabled)
{
    wifi_lock();
    s_wifi_p2p_enabled = enabled;
    wifi_unlock();
}

static inline bool wifi_p2p_get_enabled(void)
{
    bool enabled;

    wifi_lock();
    enabled = s_wifi_p2p_enabled;
    wifi_unlock();
    return enabled;
}

static bk_err_t wifi_p2p_try_reserve(void)
{
    bk_err_t ret = BK_OK;

    wifi_lock();
    if (s_wifi_state_bits & WIFI_STA_STARTED_BIT || s_wifi_p2p_enabled) {
        ret = BK_ERR_STATE;
    } else {
        s_wifi_p2p_enabled = true;
    }
    wifi_unlock();
    return ret;
}

static void wifi_p2p_save_dev_name(const char *ssid, uint8_t ssid_len)
{
    char temp_ssid[SSID_MAX_LEN + 1];

    os_memcpy(temp_ssid, ssid, ssid_len);
    temp_ssid[ssid_len] = '\0';
    os_memset(s_wifi_p2p_dev_name, 0, sizeof(s_wifi_p2p_dev_name));
    os_memcpy(s_wifi_p2p_dev_name, temp_ssid, ssid_len);
    s_wifi_p2p_dev_name[ssid_len] = '\0';
}
#endif
static inline void wifi_set_state_bit_locked(uint16_t state_bit)
{
    s_wifi_state_bits |= state_bit;
}

static inline void wifi_clear_state_bit_locked(uint16_t state_bit)
{
    s_wifi_state_bits &= ~state_bit;
}

static inline void wifi_set_state_bit(uint16_t state_bit)
{
    wifi_lock();
    wifi_set_state_bit_locked(state_bit);
    wifi_unlock();
}

static inline void wifi_clear_state_bit(uint16_t state_bit)
{
    wifi_lock();
    wifi_clear_state_bit_locked(state_bit);
    wifi_unlock();
}

static inline bool wifi_is_inited(void)
{
    return (s_wifi_state_bits & WIFI_INIT_BIT);
}

bool wifi_sta_is_started(void)
{
    return (s_wifi_state_bits & WIFI_STA_STARTED_BIT);
}

static inline bool wifi_sta_is_connected(void)
{
    return (s_wifi_state_bits & WIFI_STA_CONNECTED_BIT);
}

bool wifi_ap_is_started(void)
{
    return (s_wifi_state_bits & WIFI_AP_STARTED_BIT);
}

static inline bool wifi_sta_is_configured(void)
{
    return (s_wifi_state_bits & WIFI_STA_CONFIGURED_BIT);
}

static inline bool wifi_ap_is_configured(void)
{
    return (s_wifi_state_bits & WIFI_AP_CONFIGURED_BIT);
}

/* API Reference */
static int wifi_sta_validate_config(const wifi_sta_config_t *config)
{
    if (!config) {
        WDRV_LOGV("sta config fail, null config\n");
        return BK_ERR_NULL_PARAM;
    }

    //TODO more check
    return BK_OK;
}


//TODO optimize param_config.c
//Init global STA configurations
static int wifi_sta_init_global_config(void)
{
    BK_ASSERT(g_sta_param_ptr); /* ASSERT VERIFIED */
    BK_ASSERT(g_wlan_general_param); /* ASSERT VERIFIED */

    bk_wifi_sta_get_mac((uint8_t *)(&g_sta_param_ptr->own_mac));
    g_wlan_general_param->role = CONFIG_ROLE_STA;
    WDRV_LOGD("wdrv mac addr:"BK_MAC_FORMAT"\r\n", BK_MAC_STR(g_sta_param_ptr->own_mac) );

    return BK_OK;
}

static int wifi_scan_init_global_config(void)
{
    BK_ASSERT(g_sta_param_ptr); /* ASSERT VERIFIED */
    BK_ASSERT(g_wlan_general_param); /* ASSERT VERIFIED */

    bk_wifi_sta_get_mac((uint8_t *)(&g_sta_param_ptr->own_mac));

    return BK_OK;
}

//Set STA configuration to global configuration
static int wifi_sta_set_global_config(const wifi_sta_config_t *config)
{
    g_sta_param_ptr->ssid.length = MIN(SSID_MAX_LEN, os_strlen(config->ssid));
    memcpy(g_sta_param_ptr->ssid.array, config->ssid, g_sta_param_ptr->ssid.length);

    g_sta_param_ptr->cipher_suite = config->security;

    g_sta_param_ptr->key_len = os_strlen(config->password);
    os_memcpy(g_sta_param_ptr->key, config->password, g_sta_param_ptr->key_len);
    g_sta_param_ptr->key[g_sta_param_ptr->key_len] = 0;

    WDRV_LOGD("sta config, ssid=%s password=%s security=%d\n",
                      config->ssid, config->password, config->security);
    return BK_OK;
}

static int wifi_sta_get_global_config(wifi_sta_config_t *sta_config)
{
    if (!sta_config)
        return BK_ERR_NULL_PARAM;

    os_memset(sta_config, 0, sizeof(sta_config));
    os_memcpy(sta_config->ssid, g_sta_param_ptr->ssid.array, g_sta_param_ptr->ssid.length);

    os_memcpy(sta_config->password, g_sta_param_ptr->key, g_sta_param_ptr->key_len);

    WDRV_LOGV("sta get sta_config, ssid=%s password=%s security=%d\n",
                  sta_config->ssid, sta_config->password, sta_config->security);

    return BK_OK;
}

bk_err_t bk_wifi_init(void)
{
    WDRV_LOGD("%s, %d\r\n", __func__, __LINE__);
    uint8_t mac[ETH_ALEN];

#ifdef CONFIG_WIFI_VNET_CONTROLLER
    wdrv_init();
#endif

#if CONFIG_BK_RAW_LINK
    ap_rlk_drv_init();
#endif

    if (wifi_is_inited())
    {
        WDRV_LOGD("wifi already init!\n");
        host_wlan_remove_netif();
    }

    bk_wifi_sta_get_mac((uint8_t *)mac);
    host_wlan_add_netif(mac);
    //ToDo: AP Netif should add separated
    bk_wifi_ap_get_mac((uint8_t *)mac);
    host_wlan_add_netif(mac);

#if defined(CONFIG_WIFI_VNET_CONTROLLER) && CONFIG_P2P
    host_wlan_add_p2p_netifs();
#endif
#ifdef CONFIG_WIFI_VNET_CONTROLLER
    bk_wifi_sync_ip4_config_from_cp();
#endif

    wifi_set_state_bit(WIFI_INIT_BIT);
    WDRV_LOGD("wifi inited(%x)\n", s_wifi_state_bits);

    return BK_OK;
}

bk_err_t bk_wifi_sta_stop(void)
{
    bk_err_t ret = BK_OK;

    WDRV_LOGD("sta stopping\n");

    if (!wifi_sta_is_started()) {
        WDRV_LOGD("sta stop, already stopped\n");
        return BK_OK;
    }

    bk_wifi_sta_disconnect();

    /* Post CMD to CIF */
    ret = wifi_send_com_api_cmd(STA_STOP, 0);
    if (ret != BK_OK)
    {
        WDRV_LOGD("%s failed, ret=%d\n",__func__, ret);
    }

#if CONFIG_LWIP
    host_wlan_remove_netif();
#endif

    wifi_clear_state_bit(WIFI_STA_STARTED_BIT);
    WDRV_LOGI("sta stopped(%x)\n", s_wifi_state_bits);

    return BK_OK;
}

bk_err_t bk_wifi_sta_disconnect(void)
{
    WDRV_LOGD("sta disconnecting\n");

    if (wifi_sta_is_connected()) {
#if CONFIG_LWIP
        sta_ip_down();
#endif

        //TODO do we need to post the disconnect event?
        wifi_clear_state_bit(WIFI_STA_CONNECTED_BIT);
    }

    WDRV_LOGD("sta disconnected(%x)\n", s_wifi_state_bits);
    return BK_OK;
}

bk_err_t  bk_wifi_sta_get_mac(uint8_t *mac)
{
    bk_err_t ret = 0;
    void *buffer_to_ipc = NULL;

    if (!mac)
        return BK_ERR_NULL_PARAM;

    buffer_to_ipc = os_malloc(WIFI_MAC_LEN);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memset(buffer_to_ipc, 0, WIFI_MAC_LEN);
    ret = wifi_send_com_api_cmd(STA_GET_MAC, 1, (uint32_t)buffer_to_ipc);
    os_memcpy(mac, buffer_to_ipc, WIFI_MAC_LEN);
    os_free(buffer_to_ipc);

    return ret;
}

bk_err_t bk_wifi_ap_get_mac(uint8_t *mac)
{
    bk_err_t ret = 0;
    void *buffer_to_ipc = NULL;

    if (!mac)
        return BK_ERR_NULL_PARAM;

    buffer_to_ipc = os_malloc(WIFI_MAC_LEN);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memset(buffer_to_ipc, 0, WIFI_MAC_LEN);
    ret = wifi_send_com_api_cmd(AP_GET_MAC, 1, (uint32_t)buffer_to_ipc);
    os_memcpy(mac, buffer_to_ipc, WIFI_MAC_LEN);
    os_free(buffer_to_ipc);

    return ret;
}

bk_err_t bk_wifi_p2p_get_mac(uint8_t *mac)
{
    bk_err_t ret = 0;
    void *buffer_to_ipc = NULL;

    if (!mac)
        return BK_ERR_NULL_PARAM;

    buffer_to_ipc = os_malloc(WIFI_MAC_LEN);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memset(buffer_to_ipc, 0, WIFI_MAC_LEN);
    ret = wifi_send_com_api_cmd(P2P_GET_MAC, 1, (uint32_t)buffer_to_ipc);
    os_memcpy(mac, buffer_to_ipc, WIFI_MAC_LEN);
    os_free(buffer_to_ipc);

    return ret;
}

bk_err_t bk_wifi_p2p_get_role(int *role)
{
#if CONFIG_WIFI_VNET_CONTROLLER
    if (!role)
        return BK_ERR_NULL_PARAM;

#if CONFIG_P2P
    *role = (int)wdrv_host_env.p2p_role;
    return BK_OK;
#else
    *role = 0;
    return BK_ERR_STATE;
#endif
#else
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;

    if (!role)
        return BK_ERR_NULL_PARAM;

    buffer_to_ipc = os_malloc(sizeof(int));
    if (!buffer_to_ipc) {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memset(buffer_to_ipc, 0, sizeof(int));
    ret = wifi_send_com_api_cmd(P2P_GET_ROLE, 1, (uint32_t)buffer_to_ipc);
    *role = *(int *)buffer_to_ipc;
    os_free(buffer_to_ipc);

    return ret;
#endif
}

uint8_t bk_wlan_ap_get_channel_config(void)
{
#if CONFIG_WIFI_VNET_CONTROLLER
    uint8_t channel = 0;
    void *buffer_to_ipc = os_malloc(1);

    if (buffer_to_ipc) {
        os_memset(buffer_to_ipc, 0, 1);
        wifi_send_com_api_cmd(WIFI_GET_AP_OPER_CHANNEL, 1,
                              (uint32_t)buffer_to_ipc);
        channel = *(uint8_t *)buffer_to_ipc;
        os_free(buffer_to_ipc);
        return channel;
    }
#endif
    return g_ap_param_ptr ? g_ap_param_ptr->chann : 0;
}

uint8_t bk_wifi_p2p_get_operating_channel(void)
{
    uint8_t channel = 0;
    void *buffer_to_ipc = NULL;

    buffer_to_ipc = os_malloc(1);
    if (!buffer_to_ipc)
        return 0;

    os_memset(buffer_to_ipc, 0, 1);
    wifi_send_com_api_cmd(P2P_GET_GROUP_CHANNEL, 1, (uint32_t)buffer_to_ipc);
    channel = *(uint8_t *)buffer_to_ipc;
    os_free(buffer_to_ipc);

    return channel;
}

bk_err_t bk_wifi_p2p_get_gc_ip4_config(netif_ip4_config_t *ip_config)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    uint32_t len_ip4_config = sizeof(netif_ip4_config_t);

    if (!ip_config)
        return BK_ERR_NULL_PARAM;

    buffer_to_ipc = os_malloc(len_ip4_config);
    if (!buffer_to_ipc) {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memset(buffer_to_ipc, 0, len_ip4_config);
    ret = wifi_send_com_api_cmd(P2P_GET_GC_NETIF_IP4_CONFIG, 1, (uint32_t)buffer_to_ipc);
    if (ret == BK_OK)
        os_memcpy(ip_config, buffer_to_ipc, len_ip4_config);
    os_free(buffer_to_ipc);

    return ret;
}

bk_err_t bk_wifi_p2p_get_go_ip4_config(netif_ip4_config_t *ip_config)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    uint32_t len_ip4_config = sizeof(netif_ip4_config_t);

    if (!ip_config)
        return BK_ERR_NULL_PARAM;

    buffer_to_ipc = os_malloc(len_ip4_config);
    if (!buffer_to_ipc) {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memset(buffer_to_ipc, 0, len_ip4_config);
    ret = wifi_send_com_api_cmd(P2P_GET_GO_NETIF_IP4_CONFIG, 1, (uint32_t)buffer_to_ipc);
    if (ret == BK_OK)
        os_memcpy(ip_config, buffer_to_ipc, len_ip4_config);
    os_free(buffer_to_ipc);

    return ret;
}

#ifdef CONFIG_WIFI_VNET_CONTROLLER
static bk_err_t bk_wifi_sta_get_ip4_config_from_cp(netif_ip4_config_t *ip_config)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    uint32_t len_ip4_config = sizeof(netif_ip4_config_t);

    if (!ip_config)
        return BK_ERR_NULL_PARAM;

    buffer_to_ipc = os_malloc(len_ip4_config);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memset(buffer_to_ipc, 0, len_ip4_config);
    ret = wifi_send_com_api_cmd(STA_GET_NETIF_IP4_CONFIG, 1, (uint32_t)buffer_to_ipc);
    if (ret == BK_OK)
    {
        os_memcpy(ip_config, buffer_to_ipc, len_ip4_config);
        if (ip_config->ip[0] != '\0' && os_strcmp(ip_config->ip, "0.0.0.0") != 0)
        {
            WDRV_LOGV("Got STA IP config from CP: %s/%s gw:%s dns:%s\r\n",
                     ip_config->ip, ip_config->mask,
                     ip_config->gateway, ip_config->dns);
        }
    }
    else
    {
        WDRV_LOGE("Failed to get STA IP config from CP, ret=%d\r\n", ret);
    }
    os_free(buffer_to_ipc);

    return ret;
}

static bk_err_t bk_wifi_ap_get_ip4_config_from_cp(netif_ip4_config_t *ip_config)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    uint32_t len_ip4_config = sizeof(netif_ip4_config_t);

    if (!ip_config)
        return BK_ERR_NULL_PARAM;

    buffer_to_ipc = os_malloc(len_ip4_config);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memset(buffer_to_ipc, 0, len_ip4_config);
    ret = wifi_send_com_api_cmd(AP_GET_NETIF_IP4_CONFIG, 1, (uint32_t)buffer_to_ipc);
    if (ret == BK_OK)
    {
        os_memcpy(ip_config, buffer_to_ipc, len_ip4_config);
        if (ip_config->ip[0] != '\0' && os_strcmp(ip_config->ip, "0.0.0.0") != 0)
        {
            WDRV_LOGV("Got AP IP config from CP: %s/%s gw:%s dns:%s\r\n",
                     ip_config->ip, ip_config->mask,
                     ip_config->gateway, ip_config->dns);
        }
    }
    else
    {
        WDRV_LOGE("Failed to get AP IP config from CP, ret=%d\r\n", ret);
    }
    os_free(buffer_to_ipc);

    return ret;
}

bk_err_t bk_wifi_sync_ip4_config_from_cp(void)
{
    netif_ip4_config_t sta_ip_config = {0};
    netif_ip4_config_t ap_ip_config = {0};
    bk_err_t ret = BK_OK;

    // Get STA IP config from CP
    if (bk_wifi_sta_get_ip4_config_from_cp(&sta_ip_config) == BK_OK)
    {
        if (sta_ip_config.ip[0] != '\0' && os_strcmp(sta_ip_config.ip, "0.0.0.0") != 0)
        {
            // Reference the processing flow of EVENT_WIFI_STA_CONNECTED:
            // 1. Disable DHCP and set static IP
            // 2. Close STA interface
            // 3. Set IP configuration
            // 4. Start STA interface
#if CONFIG_NETIF_LWIP
            sta_ip_mode_set(0);
            sta_ip_down();
#endif

            ret = bk_netif_set_ip4_config(NETIF_IF_STA, &sta_ip_config);
            if (ret == BK_OK) {
#if CONFIG_NETIF_LWIP
                sta_ip_start();
#endif
                WDRV_LOGV("STA IP config synced from CP: %s/%s gw:%s dns:%s\r\n",
                        sta_ip_config.ip, sta_ip_config.mask,
                        sta_ip_config.gateway, sta_ip_config.dns);
            } else {
                WDRV_LOGE("Failed to set STA IP config, ret=%d\r\n", ret);
            }
        }
    }

    // Get AP IP config from CP
    if (bk_wifi_ap_get_ip4_config_from_cp(&ap_ip_config) == BK_OK)
    {
        if (ap_ip_config.ip[0] != '\0' && os_strcmp(ap_ip_config.ip, "0.0.0.0") != 0)
        {
            // Reference the processing flow similar to STA:
            // 1. Close AP interface if it's already started
            // 2. Set IP configuration
            // 3. Start AP interface
#if CONFIG_NETIF_LWIP
            if (uap_ip_is_start()) {
                uap_ip_down();
            }
#endif

            ret = bk_netif_set_ip4_config(NETIF_IF_AP, &ap_ip_config);
            if (ret == BK_OK) {
#if CONFIG_NETIF_LWIP
                uap_ip_start();
#endif
                WDRV_LOGV("AP IP config synced from CP: %s/%s gw:%s dns:%s\r\n",
                        ap_ip_config.ip, ap_ip_config.mask,
                        ap_ip_config.gateway, ap_ip_config.dns);
            } else {
                WDRV_LOGE("Failed to set AP IP config, ret=%d\r\n", ret);
            }
        }
    }

    return BK_OK;
}
#endif

void bk_wifi_rc_config(uint8_t sta_idx, uint16_t rate_cfg)
{
    bk_err_t ret = BK_OK;

    ret = wifi_send_com_api_cmd(WIFI_SET_RC_CONFIG, 2, sta_idx, rate_cfg);

    if (ret != BK_OK)
    {
        WIFI_LOGE("%s failed, ret=%d\n",__func__, ret);
    }
}

bk_err_t bk_wifi_set_wifi_media_mode(bool flag)
{
    return wifi_send_com_api_cmd(WIFI_SET_MEDIA_MODE, 1, flag);
}

bk_err_t bk_wifi_set_video_quality(uint8_t quality)
{
    return wifi_send_com_api_cmd(WIFI_SET_VIDEO_QUALITY, 1, quality);
}

bk_err_t bk_wifi_set_csa_coexist_mode_flag(bool is_close)
{
    if (s_wifi_state_bits & WIFI_STA_STARTED_BIT)
    {
        WDRV_LOGW("Set csa coxist mode before starting station! state_bit=0x%x\r\n",s_wifi_state_bits);
        return BK_ERR_STATE;
    }

    return wifi_send_com_api_cmd(WIFI_SET_CSA_COEXIST_MODE_FLAG, 1, is_close);
}

#if 1 //CONFIG_WIFI_SOFTAP

static inline int is_zero_ether_addr(const u8 *a)
{
    return !(a[0] | a[1] | a[2] | a[3] | a[4] | a[5]);
}

void bk_wifi_ap_init(void)
{
    WDRV_LOGD("%s, %d\r\n", __func__, __LINE__);
    uint8_t mac[ETH_ALEN];

    if(wifi_is_inited())
    {
        WDRV_LOGD("wifi already init, reinit anyway!\n");
        uap_ip_down();
        host_wlan_remove_sap_netif();
    }

    bk_wifi_ap_get_mac((uint8_t *)mac);
    host_wlan_add_netif(mac);

    wifi_set_state_bit(WIFI_INIT_BIT);
    WDRV_LOGD("wifi inited(%x)\n", s_wifi_state_bits);

}

bk_err_t bk_wifi_ap_start(void)
{
    bk_err_t ret = BK_OK;

    WDRV_LOGV("ap starting\n");

    if (!wifi_ap_is_configured()) {
        WDRV_LOGV("start ap failed, ap not configured\n");
        return BK_ERR_WIFI_AP_NOT_CONFIG;
    }

    if (wifi_ap_is_started()) {
        WDRV_LOGV("start ap, already started, ignored\n");
        return BK_OK;
    }

#if CONFIG_LWIP
    WDRV_LOGV("ap start, ip down\n");
    //TODO move to event handler
    uap_ip_down();
#endif

    WDRV_LOGV("ap start, ap start rf\n");
#if 0
    if(wifi_ap_init_rw_driver()) {
        WDRV_LOGE("ap start fail,ap init rw driver fail!\n");
        return BK_ERR_WIFI_AP_NOT_STARTED;
    }
    //TODO return value
    if(wlan_ap_enable()) {
        WDRV_LOGE("ap start fail,ap enable fail!\n");
        return BK_ERR_WIFI_AP_NOT_STARTED;
    }
    if(wlan_ap_reload()) {
        WDRV_LOGE("ap start fail,ap reload fail!\n");
        return BK_ERR_WIFI_AP_NOT_STARTED;
    }
#endif

    bk_wifi_ap_init();

    ret = wifi_send_com_api_cmd(AP_START, 0);
    if (ret != BK_OK) {
        WDRV_LOGW("ap start failed, ret=%d\n", ret);
        return ret;
    }

    WDRV_LOGD("ap started\n");

#if CONFIG_LWIP
    //TODO move to event handler
#if CONFIG_BRIDGE
	if (bk_wifi_get_bridge_state() == BRIDGE_STATE_DISABLED)
#endif
		uap_ip_start();
#endif

    wifi_set_state_bit(WIFI_AP_STARTED_BIT);
    return BK_OK;
}

bk_err_t wifi_ap_validate_config(const wifi_ap_config_t *ap_config)
{
    if (!ap_config)
        return BK_ERR_NULL_PARAM;
#if (CONFIG_SOC_BK7239XX) && CONFIG_WIFI_BAND_5G
    if (ap_config->channel >= 36 && ap_config->channel <=165) {

        //check if configured channel is avaliable channel and no need for radat detection
        int selected_channels_size = 0;
        extern int* rw_select_5g_non_radar_avaliable_channels(int *selected_channels_size);
        int *non_radar_avaliable_channels = rw_select_5g_non_radar_avaliable_channels(&selected_channels_size);

        for (int i = 0; i < selected_channels_size; i++) {
            if (non_radar_avaliable_channels[i] == ap_config->channel)
                return BK_OK;
        }

        //TODO more parameter checking
        WDRV_LOGE("[%s]configured unavaliable or dfs channel\r\n",__FUNCTION__);
        return BK_ERR_NOT_FOUND;
    }
#endif
    return BK_OK;
}

static bk_err_t wifi_ap_set_config(const wifi_ap_config_t *ap_config)
{
    BK_ASSERT(g_ap_param_ptr); /* ASSERT VERIFIED */
    BK_ASSERT(g_wlan_general_param); /* ASSERT VERIFIED */

    if (is_zero_ether_addr((u8 *)&g_ap_param_ptr->bssid))
        bk_wifi_ap_get_mac((uint8_t *)(&g_ap_param_ptr->bssid));

    //TODO
    if ((ap_config->channel >= 1 && ap_config->channel <=14)
#if CONFIG_SOC_BK7239XX
        || (ap_config->channel >= 36 && ap_config->channel <=165)
#endif
        ) {
        g_ap_param_ptr->chann = ap_config->channel;
    } else if (ap_config->channel == 0) {
        uint8_t cur_chan = (uint8_t)bk_wifi_get_channel();
        g_ap_param_ptr->chann = (cur_chan != 0) ? cur_chan : DEFAULT_CHANNEL_AP;
    } else {
        WDRV_LOGE("error:invalid channel\r\n");
        return BK_FAIL;
    }

    if(ap_config->max_con == 0 || ap_config->max_con > 2) {
        if(ap_config->max_con == 0)
            WDRV_LOGW("the max conn num is zero, set it is default\n");
        if(ap_config->max_con > 2)
            WDRV_LOGW("the max conn num is more than SUPPORTED_MAX_STA_NUM, set it is default\n");

        g_ap_param_ptr->max_con = 2;
    } else {
        g_ap_param_ptr->max_con = ap_config->max_con;
    }
    g_ap_param_ptr->disable_dns_server = ap_config->disable_dns_server;
    g_wlan_general_param->role = CONFIG_ROLE_AP;
    //TODO why need this???
    //bk_wlan_set_coexist_at_init_phase(CONFIG_ROLE_AP);

    g_ap_param_ptr->ssid.length = MIN(SSID_MAX_LEN, os_strlen(ap_config->ssid));
    os_memcpy(g_ap_param_ptr->ssid.array, ap_config->ssid, g_ap_param_ptr->ssid.length);
    g_ap_param_ptr->key_len = os_strlen(ap_config->password);
    g_ap_param_ptr->hidden_ssid = ap_config->hidden;
    if (g_ap_param_ptr->key_len < 8) {
        g_ap_param_ptr->cipher_suite = WIFI_SECURITY_NONE;
    } else {
        switch (ap_config->security) {
        case WIFI_SECURITY_WPA_TKIP:
            g_ap_param_ptr->cipher_suite = WIFI_SECURITY_WPA_TKIP;
            break;
        case WIFI_SECURITY_WPA_AES:
            g_ap_param_ptr->cipher_suite = WIFI_SECURITY_WPA_AES;
            break;
        case WIFI_SECURITY_WPA_MIXED:
            g_ap_param_ptr->cipher_suite = WIFI_SECURITY_WPA_MIXED;
            break;
        case WIFI_SECURITY_WPA2_TKIP:
            g_ap_param_ptr->cipher_suite = WIFI_SECURITY_WPA2_TKIP;
            break;
        case WIFI_SECURITY_WPA2_AES:
            g_ap_param_ptr->cipher_suite = WIFI_SECURITY_WPA2_AES;
            break;
        case WIFI_SECURITY_WPA2_MIXED:
            g_ap_param_ptr->cipher_suite = WIFI_SECURITY_WPA2_MIXED;
            break;
#if CONFIG_SOFTAP_WPA3
        case WIFI_SECURITY_WPA3_SAE:
            g_ap_param_ptr->cipher_suite = WIFI_SECURITY_WPA3_SAE;
            break;
        case WIFI_SECURITY_WPA3_WPA2_MIXED:
            g_ap_param_ptr->cipher_suite = WIFI_SECURITY_WPA3_WPA2_MIXED;
            break;
#endif
        default:
            g_ap_param_ptr->cipher_suite = WIFI_SECURITY_WPA2_AES;
            break;
        }
        os_memset(g_ap_param_ptr->key, 0, sizeof(g_ap_param_ptr->key));
        os_memcpy(g_ap_param_ptr->key, ap_config->password, g_ap_param_ptr->key_len);
    }
#if CONFIG_AP_VSIE
    g_ap_param_ptr->vsie_len = ap_config->vsie_len;
    if (ap_config->vsie_len)
        os_memcpy(g_ap_param_ptr->vsie, ap_config->vsie, g_ap_param_ptr->vsie_len);
#endif

    g_wlan_general_param->dhcp_enable = 1;
    g_ap_param_ptr->hidden_ssid = ap_config->hidden;

    return BK_OK;
}

bk_err_t bk_wifi_ap_set_config(const wifi_ap_config_t *ap_config)
{
    int ret = BK_OK;
    void *buffer_to_ipc = NULL;
    uint32_t len = sizeof(wifi_ap_config_t);

    WDRV_LOGD("ap configuring\n");

#if 0
    if (!wifi_is_inited()) {
        WDRV_LOGD("set ap config fail, wifi not init\n");
        return BK_ERR_WIFI_NOT_INIT;
    }
#endif

    ret = wifi_ap_validate_config(ap_config);
    if (ret != BK_OK)
        return ret;

    ret = wifi_ap_set_config(ap_config);
    if (ret != BK_OK)
        return ret;

    buffer_to_ipc = os_malloc(len);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    {
        wifi_ap_config_t ipc_config;

        os_memcpy(&ipc_config, ap_config, len);
        ipc_config.security = (wifi_security_t)g_ap_param_ptr->cipher_suite;
        ipc_config.channel = g_ap_param_ptr->chann;
        os_memcpy(buffer_to_ipc, &ipc_config, len);
    }

    ret = wifi_send_com_api_cmd(AP_SET_CONFIG, 1, (uint32_t)buffer_to_ipc);
    if (ret != BK_OK)
    {
        WDRV_LOGE("%s set config failed, ret=%d\n", __func__, ret);
        return ret;
    }

    os_free(buffer_to_ipc);

    wifi_set_state_bit(WIFI_AP_CONFIGURED_BIT);
    WDRV_LOGD("ap configured\n");

    if (wifi_ap_is_started()) {
#if CONFIG_WIFI_VNET_CONTROLLER
        /* CP already restarts RF on AP_SET_CONFIG; refresh AP-side netif/DHCP only. */
#if CONFIG_LWIP
        uap_ip_start();
#endif
#else
        BK_LOG_ON_ERR(bk_wifi_ap_stop());
        BK_LOG_ON_ERR(bk_wifi_ap_start());
#endif
    }
    return BK_OK;
}

bk_err_t bk_wifi_ap_stop(void)
{
    bk_err_t ret = BK_OK;

    if (!wifi_ap_is_started()) {
        WDRV_LOGD("ap stop: already stopped\n");
        return BK_OK;
    }

    ret = wifi_send_com_api_cmd(AP_STOP, 0);
    if (ret != BK_OK) {
        WDRV_LOGW("ap stop fail ret %d\n", ret);
    }

    WDRV_LOGI("ap stopped\n");
    uap_ip_down();
    host_wlan_remove_sap_netif();
    wifi_clear_state_bit(WIFI_AP_STARTED_BIT);

    return BK_OK;
}

bk_err_t bk_wifi_free_get_sta_list_memory(wlan_ap_stas_t *stas)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    uint32_t len = sizeof(wlan_ap_stas_t);

    if (stas == NULL) {
        WIFI_LOGW("%s failed, invalid stas\r\n", __func__);
        return BK_ERR_PARAM;
    }

    buffer_to_ipc = os_malloc(len);
    if (!buffer_to_ipc)
    {
        WIFI_LOGW("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memcpy(buffer_to_ipc, stas, len);
    ret = wifi_send_com_api_cmd(FREE_GET_STA_LIST_MEMORY, 1, (uint32_t)buffer_to_ipc);

    if (ret != BK_OK) {
        WIFI_LOGW("%s wifi_send_com_api_cmd failed with error %d\r\n", __func__, ret);
    }

    os_free(buffer_to_ipc);

    return ret;
}

bk_err_t bk_wifi_ap_get_sta_list(wlan_ap_stas_t *stas)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    uint32_t len = sizeof(wlan_ap_stas_t);

    if (stas == NULL) {
        WIFI_LOGW("%s failed, invalid stas\r\n", __func__);
        return BK_ERR_PARAM;
    }
    os_memset(stas, 0, sizeof(*stas));

    buffer_to_ipc = os_malloc(len);
    if (!buffer_to_ipc)
    {
        WIFI_LOGW("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memset(buffer_to_ipc, 0, len);
    ret = wifi_send_com_api_cmd(AP_GET_STA_LIST, 1, (uint32_t)buffer_to_ipc);

    if(ret == BK_OK) {
        stas->num = ((struct wlan_ap_stas *)buffer_to_ipc)->num;
        if(stas->num > 0) {
            /// malloc the cp sta list to ap
            wlan_ap_sta_t *sta_list = os_malloc(stas->num * sizeof(wlan_ap_sta_t));
            if (NULL != sta_list) {
                os_memcpy(sta_list, ((struct wlan_ap_stas *)buffer_to_ipc)->sta, stas->num * sizeof(wlan_ap_sta_t));
                stas->sta = sta_list;
                WIFI_LOGV("ap get sta list: %d\n", stas->num);
                for (int i = 0; i < stas->num; i++) {
                    WIFI_LOGV("sta %d: addr %02X:%02X:%02X:%02X:%02X:%02X\n", i, stas->sta[i].addr[0], stas->sta[i].addr[1], 
                        stas->sta[i].addr[2], stas->sta[i].addr[3], stas->sta[i].addr[4], stas->sta[i].addr[5]);
                    WIFI_LOGV("sta %d: rssi %d\n", i, stas->sta[i].rssi);
                    WIFI_LOGV("sta %d: ip %d.%d.%d.%d\n", i, (stas->sta[i].ipaddr & 0xFF), (stas->sta[i].ipaddr >> 8) & 0xFF,
                        (stas->sta[i].ipaddr >> 16) & 0xFF, (stas->sta[i].ipaddr >> 24) & 0xFF);
                }
            } else {
                WIFI_LOGW("%s malloc sta list failed\r\n", __func__);
                ret = BK_ERR_NO_MEM;
            }

            /// free the cp sta list memory
            bk_wifi_free_get_sta_list_memory(buffer_to_ipc);
        }
    }

    if(buffer_to_ipc) 
        os_free(buffer_to_ipc);
    return ret;
}

#endif

bk_err_t bk_wifi_ap_get_config(wifi_ap_config_t *ap_config)
{
    if (!ap_config)
        return BK_ERR_NULL_PARAM;

    os_memcpy(ap_config->ssid, g_ap_param_ptr->ssid.array, g_ap_param_ptr->ssid.length);
    os_memcpy(ap_config->password, g_ap_param_ptr->key, g_ap_param_ptr->key_len);
    ap_config->channel = g_ap_param_ptr->chann;
    ap_config->security = g_ap_param_ptr->cipher_suite;

    return BK_OK;
}

bk_err_t bk_wifi_set_ap_channel(uint8_t channel)
{
    int ret = 0;

    WIFI_LOGI("bk_wifi_set_ap_channel:%d\r\n", channel);

    if ((channel < 1) || (channel > 14)) {
        WIFI_LOGW("please input the valid param, channel:%d\n", channel);
        return BK_FAIL;
    }

    ret = wifi_send_com_api_cmd(AP_SET_CHANNEL, 1, channel);
    if (ret != BK_OK) {
        WIFI_LOGI("ap set channel fail ret %d\n", ret);
    }

    return ret;
}

bk_err_t bk_wifi_set_ap_csa_cnt(uint8_t csa_cnt)
{
    int ret = 0;

    WIFI_LOGI("bk_wifi_set_ap_csa_cnt:%d\r\n", csa_cnt);

    ret = wifi_send_com_api_cmd(AP_SET_CSA_COUNT, 1, csa_cnt);
    if (ret != BK_OK) {
        WIFI_LOGW("ap set csa cnt fail ret %d\n", ret); 
    }

    return ret;
}

bk_err_t bk_wifi_set_ap_channel_stop(void)
{
    int ret = 0;

    WIFI_LOGI("bk_wifi_set_ap_channel_stop\r\n");

    ret = wifi_send_com_api_cmd(AP_SET_CHANNEL_STOP, 0);
    if (ret != BK_OK) {
        WIFI_LOGW("ap set channel stop fail ret %d\n", ret);
    }

    return ret;
}
bk_err_t bk_wifi_sta_get_link_status(wifi_link_status_t *link_status)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    uint32_t len = sizeof(wifi_link_status_t);

    if (link_status == NULL) {
        WIFI_LOGE("%s failed, invalid pointer\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    buffer_to_ipc = os_malloc(len);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memset(buffer_to_ipc, 0, len);
    ret = wifi_send_com_api_cmd(STA_GET_LINK_STATUS, 1, (uint32_t)buffer_to_ipc);

    os_memcpy(link_status, buffer_to_ipc, len);
    os_free(buffer_to_ipc);

    return ret;
}

bk_err_t bk_wifi_get_channel(void)
{
    uint8_t channel = 0;
    void *buffer_to_ipc = NULL;

    buffer_to_ipc = os_malloc(1);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memset(buffer_to_ipc, 0, 1);
    wifi_send_com_api_cmd(WIFI_GET_CHANNEL, 1, (uint32_t)buffer_to_ipc);

    channel = *(uint8_t *)buffer_to_ipc;
    os_free(buffer_to_ipc);

    WIFI_LOGD("%s: %d \n", __func__, channel);

    return channel;
}

bk_err_t bk_wifi_set_country(const wifi_country_t *country)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    uint32_t len = sizeof(wifi_country_t);

    if (country == NULL) {
        WIFI_LOGE("%s failed, invalid input param\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    buffer_to_ipc = os_malloc(len);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memcpy(buffer_to_ipc, country, len);
    ret = wifi_send_com_api_cmd(WIFI_SET_COUNTRY, 1, (uint32_t)buffer_to_ipc);

    os_free(buffer_to_ipc);

    return ret;
}

bk_err_t bk_wifi_get_country(wifi_country_t *country)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    uint32_t len = sizeof(wifi_country_t);

    if (country == NULL) {
        WIFI_LOGE("%s failed, invalid input param\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    buffer_to_ipc = os_malloc(len);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memset(buffer_to_ipc, 0, len);
    ret = wifi_send_com_api_cmd(WIFI_GET_COUNTRY, 1, (uint32_t)buffer_to_ipc);

    os_memcpy(country, buffer_to_ipc, len);
    os_free(buffer_to_ipc);

    return ret;

}

bk_err_t bk_wifi_get_listen_interval(uint8_t *listen_interval)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;

    buffer_to_ipc = os_malloc(1);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memset(buffer_to_ipc, 0, 1);
    ret = wifi_send_com_api_cmd(STA_GET_LISTEN_INTERVAL, 1, (uint32_t)buffer_to_ipc);

    *listen_interval = *(uint8_t *)buffer_to_ipc;
    os_free(buffer_to_ipc);

    WIFI_LOGD("%s: %d \n", __func__, *listen_interval);

    return ret;
}

bk_err_t bk_wifi_send_listen_interval_req(uint8_t interval)
{
    bk_err_t ret = BK_OK;

    ret = wifi_send_com_api_cmd(STA_SET_LISTEN_INTERVAL, 1, interval);

    return ret;
}

bk_err_t bk_wifi_send_bcn_loss_int_req(uint8_t interval,uint8_t repeat_num)
{
    bk_err_t ret = BK_OK;

    ret = wifi_send_com_api_cmd(STA_SET_BCN_LOSS_INT, 2, interval, repeat_num);

    return ret;
}

bk_err_t bk_wifi_set_bcn_recv_win(uint8_t default_win, uint8_t max_win, uint8_t step)
{
    bk_err_t ret = BK_OK;

    ret = wifi_send_com_api_cmd(STA_SET_BCN_RECV_WIN, 3, default_win, max_win, step);

    return ret;
}

bk_err_t bk_wifi_set_bcn_loss_time(uint8_t wait_cnt, uint8_t wake_cnt)
{
    bk_err_t ret = BK_OK;

    ret = wifi_send_com_api_cmd(STA_SET_BCN_LOSS_TIME, 2, wait_cnt, wake_cnt);

    return ret;
}

bk_err_t bk_wifi_set_bcn_miss_time(uint8_t bcnmiss_time)
{
    bk_err_t ret = BK_OK;

    ret = wifi_send_com_api_cmd(STA_SET_BCN_MISS_TIME, 1, bcnmiss_time);

    return ret;
}

bk_err_t bk_wifi_sta_get_linkstate_with_reason(wifi_linkstate_reason_t *info)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    uint32_t len = sizeof(wifi_linkstate_reason_t);

    if (info == NULL) {
        WIFI_LOGE("%s failed, invalid input param\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    buffer_to_ipc = os_malloc(len);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memset(buffer_to_ipc, 0, len);
    ret = wifi_send_com_api_cmd(STA_GET_LINK_STATE_WITH_REASON, 1, (uint32_t)buffer_to_ipc);

    os_memcpy(info, buffer_to_ipc, len);
    os_free(buffer_to_ipc);

    return ret;
}

bk_err_t bk_wifi_get_support_wifi_mode(uint8_t* support_mode)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;

    buffer_to_ipc = os_malloc(1);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memset(buffer_to_ipc, 0, 1);
    ret = wifi_send_com_api_cmd(WIFI_GET_SUPPORT_MODE, 1, (uint32_t)buffer_to_ipc);

    *support_mode = *(uint8_t *)buffer_to_ipc;
    os_free(buffer_to_ipc);

    WIFI_LOGD("%s: %d \n", __func__, *support_mode);

    return ret;
}

bk_err_t bk_scan_country_code(uint8_t *country_code, int *len)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc_1 = NULL;
    void *buffer_to_ipc_2 = NULL;
    uint8_t len_2 = sizeof(int);

    if (country_code == NULL) {
        WIFI_LOGE("%s failed, invalid input param\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    buffer_to_ipc_1 = os_malloc(MAC_COUNTRY_STRING_LEN);
    if (!buffer_to_ipc_1)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    buffer_to_ipc_2 = os_malloc(len_2);
    if (!buffer_to_ipc_2)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    *len = 0;
    os_memset(buffer_to_ipc_1, 0, MAC_COUNTRY_STRING_LEN);
    os_memset(buffer_to_ipc_2, 0, len_2);
    ret = wifi_send_com_api_cmd(SCAN_CONTRY_CODE, 2, (uint32_t)buffer_to_ipc_1, (uint32_t)buffer_to_ipc_2);

    os_memcpy(len, buffer_to_ipc_2, len_2);
    os_free(buffer_to_ipc_2);

    WIFI_LOGD("%s: cc_len %d \n", __func__, *len);

    if (*len > 0)
        os_memcpy(country_code, buffer_to_ipc_1, *len);
    os_free(buffer_to_ipc_1);

    return ret;
}

static wifi_beacon_cc_rxed_t g_scan_cc_rxed_cb = NULL;
void *g_scan_cc_ctxt = NULL;
bk_err_t bk_wifi_bcn_cc_rxed_register_cb(const wifi_beacon_cc_rxed_t cc_cb, void *ctxt)
{
    bk_err_t ret = BK_OK;
    bool enable = (cc_cb == NULL)? false: true;

    g_scan_cc_rxed_cb = cc_cb;
    g_scan_cc_ctxt = ctxt;

    ret = wifi_send_com_api_cmd(WIFI_GET_BCN_CC, 1, enable);

    return ret;
}

bk_err_t bk_wifi_bcn_cc_rxed_cb(void *data, uint16_t len)
{
    uint8_t *cc;
    uint8_t cc_len = len;

    cc = os_malloc(cc_len);
    os_memcpy(cc, (uint8_t *)data, cc_len);

    if (g_scan_cc_rxed_cb)
        g_scan_cc_rxed_cb(g_scan_cc_ctxt, cc, cc_len);

    os_free(cc);

    return 0;
}

bk_err_t bk_wifi_sta_connect(void)
{
    wifi_sta_config_t sta_config = { 0 };

    WDRV_LOGD("sta connecting\n");

    wifi_sta_get_global_config(&sta_config);

    if (!wifi_sta_is_started()) {
        WDRV_LOGD("sta connect fail, sta not start\n");
        return BK_ERR_WIFI_STA_NOT_STARTED;
    }

    wifi_set_state_bit(WIFI_STA_CONNECTED_BIT);
    WDRV_LOGD("sta connected(%x)\n", s_wifi_state_bits);

    return BK_OK;
}

bk_err_t bk_wifi_sta_start(void)
{
    bk_err_t ret = BK_OK;
    bool start_new = true;

    WDRV_LOGD("sta starting\n");
    wdrv_reset_sta_link_state(WIFI_LINKSTATE_STA_CONNECTING);

    if (!wifi_sta_is_configured()) {
        WDRV_LOGD("sta start fail, sta not configured\n");
        return BK_ERR_WIFI_STA_NOT_CONFIG;
    }

    wifi_lock();
#if CONFIG_P2P
    if (s_wifi_p2p_enabled) {
        ret = BK_ERR_STATE;
        start_new = false;
    }
#endif
    if (start_new && (s_wifi_state_bits & WIFI_STA_STARTED_BIT)) {
        start_new = false;
    }
    if (start_new) {
        wifi_set_state_bit_locked(WIFI_STA_STARTED_BIT);
    }
    wifi_unlock();

#if CONFIG_P2P
    if (ret == BK_ERR_STATE) {
        WDRV_LOGD("sta start fail, p2p already enabled\n");
        return ret;
    }
#endif
    if (!start_new) {
        WDRV_LOGD("sta already started, ignored!\n");
        return BK_OK;
    }

    wifi_sta_init_global_config();

    //bk_wifi_init();

    /* always connect the AP automatically */
    bk_wifi_sta_connect();

    ret = wifi_send_com_api_cmd(STA_START, 0);
    if (ret != BK_OK)
    {
        WDRV_LOGD("%s failed, ret=%d\n",__func__, ret);
    }

    return BK_OK;
}

bk_err_t bk_wifi_sta_set_config(const wifi_sta_config_t *config)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    uint32_t len = sizeof(wifi_sta_config_t);

    WDRV_LOGD("sta configuring\n");

//    if (!wifi_is_inited()) {
//        WDRV_LOGV("set sta config fail, wifi not init\n");
//        return BK_ERR_WIFI_NOT_INIT;
//    }

    if (config == NULL) {
        WIFI_LOGE("%s failed, invalid config\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    ret = wifi_sta_validate_config(config);
    if (ret != BK_OK) {
        WDRV_LOGD("set config fail, invalid param\n");
        return ret;
    }

    wifi_sta_set_global_config(config);
    wdrv_reset_sta_link_state(WIFI_LINKSTATE_STA_IDLE);

    wifi_set_state_bit(WIFI_STA_CONFIGURED_BIT);
    WDRV_LOGD("sta configured(%x)\n", s_wifi_state_bits);


    buffer_to_ipc = os_malloc(len);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memcpy(buffer_to_ipc, config, len);
    ret = wifi_send_com_api_cmd(STA_SET_CONFIG, 1, (uint32_t)buffer_to_ipc);

    os_free(buffer_to_ipc);

    return ret;
}
bk_err_t bk_wifi_sta_get_config(wifi_sta_config_t *config)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    uint32_t len = sizeof(wifi_sta_config_t);

    //WIFI_LOGE("%s config len:%d\r\n", __func__, len);
    if (config == NULL) {
        WIFI_LOGE("%s failed, invalid config\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memset(config, 0, sizeof(config));

    if (!wifi_sta_is_configured())
        return BK_ERR_WIFI_STA_NOT_CONFIG;

    buffer_to_ipc = os_malloc(len);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memset(buffer_to_ipc, 0, len);
    ret = wifi_send_com_api_cmd(STA_GET_CONFIG, 1, (uint32_t)buffer_to_ipc);

    os_memcpy(config, buffer_to_ipc, len);
    os_free(buffer_to_ipc);

    return ret;
}

bk_err_t bk_wifi_sta_pm_enable(void)
{
    return wifi_send_com_api_cmd(STA_PM_ENABLE, 0);
}

bk_err_t bk_wifi_sta_pm_disable(void)
{
    return wifi_send_com_api_cmd(STA_PM_DISABLE, 0);
}

bk_err_t bk_wifi_monitor_start(void)
{
    return wifi_send_com_api_cmd(MONITOR_START, 0);
}

bk_err_t bk_wifi_monitor_stop(void)
{
    return wifi_send_com_api_cmd(MONITOR_STOP, 0);
}

bk_err_t bk_wifi_monitor_set_channel(const wifi_channel_t *chan)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    uint32_t len = sizeof(wifi_channel_t);

    if (chan == NULL) {
        WIFI_LOGE("%s failed, invalid config\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    buffer_to_ipc = os_malloc(len);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memcpy(buffer_to_ipc, chan, len);
    ret = wifi_send_com_api_cmd(MONITOR_SET_CHANNEL, 1, (uint32_t)buffer_to_ipc);

    os_free(buffer_to_ipc);

    return ret;
}

bk_err_t bk_wifi_monitor_result_register(void)
{
    uint8_t ret = BK_OK;
    uint8_t type = WIFI_MONITOR_RESULT;

    ret = wifi_send_com_api_cmd(MONITOR_REGISTER_CB, 1, type);

    return ret;
}

bk_err_t bk_wifi_monitor_get_result(void)
{
    uint8_t ret = BK_OK;
    wifi_monitor_result_t result;
    void *buffer_to_ipc = NULL;
    uint32_t len = sizeof(wifi_monitor_result_t);

    buffer_to_ipc = os_malloc(len);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memset(buffer_to_ipc, 0, len);
    ret = wifi_send_com_api_cmd(MONITOR_GET_RESULT, 1, (uint32_t)buffer_to_ipc);

    os_memcpy(&result, buffer_to_ipc, len);
    os_free(buffer_to_ipc);

    WIFI_LOGI("total:      %d\n", result.rx_cnt_total);
    WIFI_LOGI("mgmt:       %d\n", result.rx_cnt_mgmt);
    WIFI_LOGI("data:       %d\n", result.rx_cnt_data);
    WIFI_LOGI("ctrl:       %d\n", result.rx_cnt_ctrl);
    WIFI_LOGI("0 - 255:    %d\n", result.rx_cnt_0_255);
    WIFI_LOGI("256 - 511:  %d\n", result.rx_cnt_256_511);
    WIFI_LOGI("512 - 1023: %d\n", result.rx_cnt_512_1023);
    WIFI_LOGI(">=1024:     %d\n", result.rx_cnt_1024);

    return ret;
}

void bk_wifi_ps_config(uint8_t ps_id, uint8_t ps_val, uint8_t ps_val1)
{
    wifi_send_com_api_cmd(PS_CONFIG, 3, ps_id, ps_val, ps_val1);

    return;
}

bk_err_t bk_wifi_send_raw(uint8_t *buffer, int len)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;

    if (buffer == NULL) {
        WIFI_LOGE("%s failed, invalid config\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    buffer_to_ipc = os_malloc(len);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memcpy(buffer_to_ipc, buffer, len);
    ret = wifi_send_com_api_cmd(SEND_RAW, 2, (uint32_t)buffer_to_ipc, len);

    os_free(buffer_to_ipc);

    return ret;
}

bk_err_t bk_wifi_manual_cal_rfcali_status(void)
{
    return wifi_send_com_api_cmd(PHY_CAL_RFCALI, 0);
}

bk_err_t bk_wifi_capa_config(wifi_capability_t capa_id, uint32_t capa_val)
{
    return wifi_send_com_api_cmd(WIFI_CAPA_CONFIG, 2, (uint32_t)capa_id, capa_val);
}

bk_err_t bk_wifi_set_mac_address(char *mac)
{
    bk_err_t ret = 0;
    void *buffer_to_ipc = NULL;

    if (!mac)
        return BK_ERR_NULL_PARAM;

    buffer_to_ipc = os_malloc(WIFI_MAC_LEN);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memcpy(buffer_to_ipc, mac, WIFI_MAC_LEN);
    ret = wifi_send_com_api_cmd(SET_MAC_ADDRESS, 1, (uint32_t)buffer_to_ipc);
    os_free(buffer_to_ipc);

    return ret;
}

bk_err_t bk_wifi_scan_start(const wifi_scan_config_t *scan_config)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    uint32_t len = sizeof(wifi_scan_config_t);
    uint8_t ssid_len = 0;

    WDRV_LOGD("scaning\n");

    wifi_scan_init_global_config();
    wifi_set_state_bit(WIFI_PURE_SCAN_STARTED_BIT);

    if (scan_config ) {
        ssid_len = MIN(SSID_MAX_LEN, os_strlen((char *)scan_config->ssid));

        if((0 != scan_config->scan_type) ||(0 != scan_config->chan_cnt) ||(0 != scan_config->duration)) {
            scan_param_env.set_param = 1;
            scan_param_env.scan_type = scan_config->scan_type;
            scan_param_env.chan_cnt = scan_config->chan_cnt;
            if(WIFI_MAX_SCAN_CHAN_DUR < scan_config->duration) {
                WDRV_LOGW("scan duration is too long %dus,need less than 200ms\r\n",scan_config->duration);
                scan_param_env.duration = WIFI_MAX_SCAN_CHAN_DUR * 1000;
            } else
                scan_param_env.duration = scan_config->duration * 1000;
            os_memcpy(scan_param_env.chan_nb, scan_config->chan_nb, scan_config->chan_cnt);
        }
    }

    WIFI_LOGD("%s ssid_len:%d\r\n", __func__, ssid_len);
    //WIFI_LOGE("%s config len:%d\r\n", __func__, len);
    if (scan_config == NULL) {
        return wifi_send_com_api_cmd(SCAN_START, 1, 0);
    } else {
        buffer_to_ipc = os_malloc(len);
        if (!buffer_to_ipc)
        {
            WIFI_LOGE("%s malloc failed\r\n", __func__);
            return BK_ERR_NO_MEM;
        }

        os_memcpy(buffer_to_ipc, scan_config, len);

        ret = wifi_send_com_api_cmd(SCAN_START, 1, (uint32_t)buffer_to_ipc);

        os_free(buffer_to_ipc);

        return ret;
    }
}

bk_err_t bk_wifi_scan_stop(void)
{
    return wifi_send_com_api_cmd(SCAN_STOP, 0);
}

bk_err_t bk_wifi_scan_get_result(wifi_scan_result_t *scan_result)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    uint32_t len = sizeof(wifi_scan_result_t);

    if (scan_result == NULL) {
        WIFI_LOGW("%s failed, invalid scan_result\r\n", __func__);
        return BK_ERR_PARAM;
    }
    os_memset(scan_result, 0, sizeof(*scan_result));

    buffer_to_ipc = os_malloc(len);
    if (!buffer_to_ipc)
    {
        WIFI_LOGW("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memset(buffer_to_ipc, 0, len);
    ret = wifi_send_com_api_cmd(SCAN_RESULT, 1, (uint32_t)buffer_to_ipc);
    if (ret != BK_OK) {
        WIFI_LOGW("%s wifi_send_com_api_cmd failed: %d\r\n", __func__, ret);
        goto _free_and_exit;
    }

    os_memcpy(scan_result, buffer_to_ipc, len);

    if (scan_result->ap_num > 0 && scan_result->aps == NULL) {
        WIFI_LOGW("%s failed, aps pointer is NULL but ap_num is %d\r\n", __func__, scan_result->ap_num);
        scan_result->ap_num = 0;
        ret = BK_ERR_PARAM; //update reason code
        goto _free_and_exit;
    }

_free_and_exit:
    os_free(buffer_to_ipc);
    return ret;
}

static const char *wifi_sec_type_string_api(wifi_security_t security)
{
    switch (security) {
    case WIFI_SECURITY_NONE:
        return "NONE";
    case WIFI_SECURITY_WEP:
        return "WEP";
    case WIFI_SECURITY_WPA_TKIP:
        return "WPA-TKIP";
    case WIFI_SECURITY_WPA_AES:
        return "WPA-AES";
    case WIFI_SECURITY_WPA_MIXED:
        return "WPA-MIX";
    case WIFI_SECURITY_WPA2_TKIP:
        return "WPA2-TKIP";
    case WIFI_SECURITY_WPA2_AES:
        return "WPA2-AES";
    case WIFI_SECURITY_WPA2_MIXED:
        return "WPA2-MIX";
    case WIFI_SECURITY_WPA3_SAE:
        return "WPA3-SAE";
    case WIFI_SECURITY_WPA3_WPA2_MIXED:
        return "WPA3-WPA2-MIX";
    case WIFI_SECURITY_EAP:
        return "EAP";
    case WIFI_SECURITY_OWE:
        return "OWE";
    case WIFI_SECURITY_AUTO:
        return "AUTO";
#ifdef CONFIG_WAPI_SUPPORT
    case WIFI_SECURITY_TYPE_WAPI_PSK:
        return "WAPI_PSK";
    case WIFI_SECURITY_TYPE_WAPI_CERT:
        return "WAPI_CERT";
#endif
    default:
        return "UNKNOWN";
    }
}

static void wifi_scan_dump_ap(const wifi_scan_ap_info_t *ap)
{
    const char *security_str = wifi_sec_type_string_api(ap->security);
#if (CONFIG_SHELL_ASYNCLOG)
    shell_cmd_ind_out("%-32s " BK_MAC_FORMAT "   %4d %2d %s\r\n",
               ap->ssid, BK_MAC_STR(ap->bssid), (int8_t)ap->rssi, ap->channel, security_str);
#else
    WIFI_LOG_RAW("%-32s " BK_MAC_FORMAT "   %4d %2d %s\n",
               ap->ssid, BK_MAC_STR(ap->bssid), (int8_t)ap->rssi, ap->channel, security_str);
#endif
}

bk_err_t bk_wifi_scan_dump_result(const wifi_scan_result_t *scan_result)
{
    int i;

    if (!scan_result) {
#if (CONFIG_SHELL_ASYNCLOG)
        shell_cmd_ind_out("scan doesn't found AP\n");
#else
        WIFI_LOGW("Invalid scan_result(NULL)\r\n");
#endif
        return BK_ERR_PARAM;
    }

    if (scan_result->ap_num < 0 || scan_result->ap_num > 100) {
        WIFI_LOGW("Invalid AP count: %d\r\n", scan_result->ap_num);
        return BK_ERR_PARAM;
    }

    if ((scan_result->ap_num > 0) && (!scan_result->aps)) {
        WIFI_LOGW("scan number is %d, but AP info is NULL\n", scan_result->ap_num);
        return BK_ERR_PARAM;
    }
#if (CONFIG_SHELL_ASYNCLOG)
    shell_cmd_ind_out("scan found %d AP\r\n", scan_result->ap_num);
    shell_cmd_ind_out("%32s %17s   %4s %4s %s\r\n", "              SSID              ",
               "      BSSID      ", "RSSI", "chan", "security");
    shell_cmd_ind_out("%32s %17s   %4s %4s %s\r\n", "--------------------------------",
               "-----------------", "----", "----", "---------\n");
#else
    WIFI_LOGD("scan found %d AP\n", scan_result->ap_num);
    WIFI_LOG_RAW("%32s %17s   %4s %4s %s\n", "              SSID              ",
               "      BSSID      ", "RSSI", "chan", "security");
    WIFI_LOG_RAW("%32s %17s   %4s %4s %s\n", "--------------------------------",
               "-----------------", "----", "----", "---------\n");
#endif
    for (i = 0; i < scan_result->ap_num; i++) {
        wifi_scan_dump_ap(&scan_result->aps[i]);
        rtos_delay_milliseconds(10);
    }

    //WIFI_LOG_RAW("\n");

    return BK_OK;
}

void bk_wifi_scan_free_result(wifi_scan_result_t *scan_result)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    uint32_t len = sizeof(wifi_scan_result_t);

    if (scan_result == NULL) {
        WIFI_LOGW("%s failed, invalid scan_result\r\n", __func__);
        return;
    }

    buffer_to_ipc = os_malloc(len);
    if (!buffer_to_ipc)
    {
        WIFI_LOGW("%s malloc failed\r\n", __func__);
        return;
    }

    os_memcpy(buffer_to_ipc, scan_result, len);
    ret = wifi_send_com_api_cmd(SCAN_RESULT_FREE, 1, (uint32_t)buffer_to_ipc);

    if (ret != BK_OK) {
        WIFI_LOGW("%s wifi_send_com_api_cmd failed with error %d\r\n", __func__, ret);
    }

    os_free(buffer_to_ipc);
}

//MONITOR
bk_err_t bk_wifi_monitor_register_cb(const wifi_monitor_cb_t monitor_cb)
{
	uint8_t type = WIFI_MONITOR_DATA;

    s_monitor_ap_cb = monitor_cb;
    return wifi_send_com_api_cmd(MONITOR_REGISTER_CB, 1, type);
}

wifi_monitor_cb_t bk_wifi_monitor_get_cb(void)
{
    return s_monitor_ap_cb;
}

bk_err_t bk_wifi_monitor_register_ind(uint8_t * msg_payload)
{
    struct monitor_struct
    {
        cpdu_t cp;
        struct bk_rx_msg_hdr rx_msg_hdr;
        uint32_t para[3];
        uint32_t payload[1];
    };

    bk_err_t ret = BK_OK;
    const uint8_t *frame = NULL;
    uint32_t len;
    const wifi_frame_info_t *frame_info;
    wifi_filter_cb_t cb = bk_wifi_monitor_get_cb();
    //uint8_t *payload_temp =  NULL;
    struct pbuf* pbuf = NULL;
    cpdu_t * cpdu = NULL;
    uint8_t vif_idx = 0;

    pbuf = (struct pbuf*)((uint8_t*)msg_payload + sizeof(uint32_t) - sizeof(cpdu_t) - sizeof(wdrv_rx_msg) - sizeof(struct pbuf));
    struct monitor_struct* mon_hdr = (struct monitor_struct*)(pbuf + 1);
    
    len = mon_hdr->para[0];
    frame = (const uint8_t*)mon_hdr->para[2];
    frame_info = (const wifi_frame_info_t *)mon_hdr->para[1];

    if(cb)
    {
        cb(frame,len,frame_info);//This payload will free in next line.
    }

    cpdu = (cpdu_t*)(pbuf + 1);
    cpdu->co_hdr.need_free = 1;//RXC free
    vif_idx = cpdu->co_hdr.vif_idx;
    ret = wdrv_txdata_sender(pbuf,vif_idx);//vif null, just for free this RXC pbuf
    return ret;
}
//Filter
bk_err_t bk_wifi_filter_register_cb(const wifi_filter_cb_t filter_cb)
{
    s_filter_ap_cb = filter_cb;
    return wifi_send_com_api_cmd(FILTER_REGISTER_CB, 0);
}

wifi_filter_cb_t bk_wifi_filter_get_cb(void)
{
    return s_filter_ap_cb;
}
bk_err_t bk_wifi_filter_register_ind(uint8_t * msg_payload)
{
    struct filter_struct
    {
        cpdu_t cp;
        struct bk_rx_msg_hdr rx_msg_hdr;
        uint32_t para[3];
        uint32_t payload[1];
    };

    bk_err_t ret = BK_OK;
    const uint8_t *frame = NULL;
    uint32_t len;
    const wifi_frame_info_t *frame_info;
    wifi_filter_cb_t cb = bk_wifi_filter_get_cb();
    //uint8_t *payload_temp =  NULL;
    struct pbuf* pbuf = NULL;
    cpdu_t * cpdu = NULL;
    uint8_t vif_idx = 0;

    pbuf = (struct pbuf*)((uint8_t*)msg_payload + sizeof(uint32_t) - sizeof(cpdu_t) - sizeof(wdrv_rx_msg) - sizeof(struct pbuf));
    struct filter_struct* filter_hdr = (struct filter_struct*)(pbuf + 1);
    
    len = filter_hdr->para[0];
    frame = (const uint8_t*)filter_hdr->para[2];
    frame_info = (const wifi_frame_info_t *)filter_hdr->para[1];
    //bk_mem_dump("ap",(uint32_t)pbuf,200);
    if(cb)
    {
        cb(frame,len,frame_info);//This payload will free in next line.
    }

    cpdu = (cpdu_t*)(pbuf + 1);
    cpdu->co_hdr.need_free = 1;//RXC free
    vif_idx = cpdu->co_hdr.vif_idx;
    ret = wdrv_txdata_sender(pbuf,vif_idx);//vif null, just for free this RXC pbuf
    return ret;
}
bk_err_t bk_wifi_send_arp_set_rate_req(uint16_t arp_tx_rate)
{
    return wifi_send_com_api_cmd(SEND_ARP_SET_RATE_REQ, 1, arp_tx_rate);
}

bk_err_t bk_wifi_get_status(wifi_status_t *status)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    uint32_t len = sizeof(wifi_status_t);

    if (status == NULL) {
        WIFI_LOGE("%s failed, invalid pointer\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    buffer_to_ipc = os_malloc(len);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memset(buffer_to_ipc, 0, len);
    ret = wifi_send_com_api_cmd(GET_STATUS, 1, (uint32_t)buffer_to_ipc);

    os_memcpy(status, buffer_to_ipc, len);
    os_free(buffer_to_ipc);

    return ret;
}

bk_err_t bk_wifi_set_block_bcmc_en(uint8_t config)
{
    bk_err_t ret = BK_OK;

    ret = wifi_send_com_api_cmd(WIFI_SET_BLOCK_BCMC_EN, 1, config);

    return ret;
}

bool bk_wifi_get_block_bcmc_en(void)
{
    void *buffer_to_ipc = NULL;
    bool bcmcm_en;

    buffer_to_ipc = os_malloc(1);
    os_memset(buffer_to_ipc, 0, 1);
    wifi_send_com_api_cmd(WIFI_GET_BLOCK_BCMC_EN, 1, (uint32_t)buffer_to_ipc);

    bcmcm_en = *(bool *)buffer_to_ipc;
    os_free(buffer_to_ipc);

    return bcmcm_en;
}

bk_err_t bk_wifi_ftm_start(const wifi_ftm_config_t *config, wifi_ftm_results_t *ftm_results)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc_1 = NULL;
    void *buffer_to_ipc_2 = NULL;
    uint32_t len1 = sizeof(wifi_ftm_config_t);
    uint32_t len2 = sizeof(wifi_ftm_results_t);

    if ((config == NULL) || (ftm_results == NULL)) {
        WIFI_LOGE("%s failed, invalid pointer\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    buffer_to_ipc_1 = os_malloc(len1);
    buffer_to_ipc_2 = os_malloc(len2);
    if (!buffer_to_ipc_1 || !buffer_to_ipc_2)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memcpy(buffer_to_ipc_1, config, len1);
    os_memset(buffer_to_ipc_2, 0, len2);
    ret = wifi_send_com_api_cmd(FTM_START, 2, (uint32_t)buffer_to_ipc_1, (uint32_t)buffer_to_ipc_2);

    os_memcpy(ftm_results, buffer_to_ipc_2, len2);
    os_free(buffer_to_ipc_1);
    os_free(buffer_to_ipc_2);

    return ret;
}

bk_err_t bk_wifi_ftm_dump_result(const wifi_ftm_results_t *ftm_results)
{
    if (!ftm_results) {
        WIFI_LOGD("ftm doesn't found responser\n");
        return BK_OK;
    }

    if ((ftm_results->nb_ftm_rsp > 0) && (!ftm_results->rsp)) {
        WIFI_LOGE("ftm responser number is %d, but responser info is NULL\n", ftm_results->nb_ftm_rsp);
        return BK_ERR_PARAM;
    }

    WIFI_LOGD("ftm found %d responser\n", ftm_results->nb_ftm_rsp);

    for (int i = 0; i < ftm_results->nb_ftm_rsp; i++) {
        WIFI_LOGD("The distance to " WIFI_MAC_FORMAT " is %.2f meters, rtt is %d nSec \n",
            WIFI_MAC_STR(ftm_results->rsp[i].bssid), ftm_results->rsp[i].distance, ftm_results->rsp[i].rtt);
        rtos_delay_milliseconds(10);
    }

    WIFI_LOG_RAW("\n");

    return BK_OK;
}

bk_err_t bk_wifi_ftm_free_result(wifi_ftm_results_t *ftm_results)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    uint32_t len = sizeof(wifi_ftm_results_t);

    if (ftm_results == NULL) {
        WIFI_LOGE("%s failed, invalid pointer\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    if (ftm_results->rsp == NULL)
    {
        WIFI_LOGE("%s no need to free, num %d\r\n", __func__, ftm_results->nb_ftm_rsp);
        return BK_OK;
    }

    buffer_to_ipc = os_malloc(len);

    os_memcpy(buffer_to_ipc, ftm_results, len);
    ret = wifi_send_com_api_cmd(FTM_FREE_RESULT, 1, (uint32_t)buffer_to_ipc);

    os_free(buffer_to_ipc);

    return ret;
}


bk_err_t bk_wifi_csi_alg_config(double thres1)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    uint8_t len = sizeof(double);

    buffer_to_ipc = os_malloc(len);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memcpy(buffer_to_ipc, &thres1, len);
    ret = wifi_send_com_api_cmd(CSI_ALG_CONFIG, 1, (uint32_t)buffer_to_ipc);

    os_free(buffer_to_ipc);

    return ret;

}

bk_err_t bk_wifi_csi_start_req(uint8_t csi_work_type,uint8_t csi_work_mode,uint8_t csi_work_identity,uint8_t csi_data_format,
                               uint32_t csi_data_interval,uint32_t delay)
{
    bk_err_t ret = BK_OK;

    ret = wifi_send_com_api_cmd(CSI_START, 6, csi_work_type, csi_work_mode, csi_work_identity,
                                csi_data_format, csi_data_interval, delay);

    return ret;
}

bk_err_t bk_wifi_csi_stop_req(void)
{
    bk_err_t ret = BK_OK;

    ret = wifi_send_com_api_cmd(CSI_STOP, 0);

    return ret;
}

bk_err_t bk_wifi_csi_static_param_reset_req(uint8_t update_cali_mode,uint32_t cali_cnt)
{
    bk_err_t ret = BK_OK;

    ret = wifi_send_com_api_cmd(CSI_STATIC_PARAM_RESET, 2, update_cali_mode, cali_cnt);

    return ret;
}

wifi_csi_cb_t g_wifi_csi_info_handler = NULL;
void bk_wifi_csi_info_cb_register(wifi_csi_cb_t cb)
{
    bool enable = (cb == NULL)? false: true;

    g_wifi_csi_info_handler = cb;

    wifi_send_com_api_cmd(CSI_INFO_GET, 1, enable);

    return;
}

void bk_wifi_csi_info_cb(void *data)
{
    struct wifi_csi_info_t *info;
    uint16_t len = sizeof(struct wifi_csi_info_t);

    info = os_malloc(len);
    os_memcpy(info, (uint8_t *)data, len);

    if(g_wifi_csi_info_handler)
        g_wifi_csi_info_handler(info);

    os_free(info);
}


bk_err_t bk_wifi_csi_demo_turn_on_light(uint8_t color, bool flicker)
{
    bk_err_t ret = BK_OK;

    ret = wifi_send_com_api_cmd(CSI_DEMO_LIGHT, 2, color, flicker);

    return ret;
}

#if CONFIG_BRIDGE
bk_err_t bk_wifi_check_client_mac_connected(uint8_t *mac)
{
    void *buffer_to_ipc = NULL;
    bk_err_t ret = BK_OK;

    buffer_to_ipc = os_malloc(WIFI_MAC_LEN);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }
    os_memcpy(buffer_to_ipc, mac, WIFI_MAC_LEN);
    ret = wifi_send_com_api_cmd(CHECK_CLIENT_MAC_CONNECTED, 1, (uint32_t)buffer_to_ipc);
    os_free(buffer_to_ipc);
    return ret;
}

#include "netif/bridgeif.h"
#include "lwip/netifapi.h"
#include "lwip/inet.h"
#include "bk_private/bk_bridge_ctx.h"
#include "bk_private/bk_bridge_fsm.h"
#include <components/event.h>

/*
 * bridge_state / bridge_config live in bk_bridge_ctrl.c.
 * Every state mutation goes through bk_bridge_fsm_transition(), which
 * serialises concurrent transitions, validates the source->target pair
 * against a legal table, and pairs the local write with the CP IPC
 * atomically so the cores never disagree across the IPC barrier.
 * bk_wifi_sync_bridge_state() and bk_bridge_ctx_set_state_local() remain
 * exported only for the FSM itself — application code MUST NOT call them
 * directly.
 */

/*private apis for beken bridge start*/
bk_bridge_state_t bk_wifi_get_bridge_state(void)
{
    return bk_bridge_ctx_get_state();
}

bk_err_t bk_wifi_sync_bridge_state(bk_bridge_state_t br_state)
{
    /* Synchronous IPC: returns only after CP ack. Inline enum, no heap. */
    return wifi_send_com_api_cmd(SET_BRIDGE_SYNC_STATE, 1, (uint32_t)br_state);
}

bk_err_t bk_wifi_save_bridge_config(bk_bridge_config_t *br_config)
{
    if (br_config == NULL) {
        WIFI_LOGE("%s failed, invalid pointer\r\n", __func__);
        return BK_FAIL;
    }
    return bk_bridge_ctx_save_config(br_config);
}

static beken_thread_t br_start_thread_internal;

/*
 * Monotonic bring-up generation (BSS zero-init at boot; never reset to 0).
 * br_start_thread snapshots the value at entry; bk_br_bringup_invalidate()
 * bumps it on bridge_stop / switch_bridge_to_sta / abort so an in-flight
 * thread sees token mismatch and rolls back instead of finishing ENABLED
 * after teardown.
 */
static volatile uint32_t s_br_bringup_token;

/* True while br_start_thread_internal is running (cleared in thread done:). */
static volatile bool s_br_start_thread_running;

static bool bk_br_bringup_active(uint32_t token)
{
    return (bk_bridge_ctx_get_state() == BRIDGE_STATE_ENABLING) &&
           (s_br_bringup_token == token);
}

static void bk_br_bringup_invalidate(void)
{
    s_br_bringup_token++;
}

/*
 * Wait without timeout until br_start_thread_internal exits.
 * Must not be called from that thread (would deadlock).
 */
static void bk_br_wait_bringup_thread_idle(void)
{
    while (s_br_start_thread_running) {
        rtos_delay_milliseconds(10);
    }
}

static bk_err_t bk_wifi_start_softap_for_bridge(void);
static void bk_wifi_switch_bridge_to_sta(void);

static bk_err_t bk_bridge_event_cb(void *arg, event_module_t event_module,
    int event_id, void *event_data)
{
    (void)arg;

    if (event_module == EVENT_MOD_NETIF && event_id == EVENT_NETIF_GOT_IP4) {
        netif_event_got_ip4_t *got_ip = (netif_event_got_ip4_t *)event_data;

        if (got_ip != NULL && got_ip->netif_if == NETIF_IF_STA) {
            bk_wifi_start_softap_for_bridge();
        }
    } else if (event_module == EVENT_MOD_WIFI &&
               event_id == EVENT_WIFI_STA_DISCONNECTED) {
        bk_wifi_switch_bridge_to_sta();
    }
    return BK_OK;
}

static void bk_bridge_register_event_cb(void)
{
    BK_LOG_ON_ERR(bk_event_register_cb(EVENT_MOD_NETIF, EVENT_NETIF_GOT_IP4,
        bk_bridge_event_cb, NULL));
    BK_LOG_ON_ERR(bk_event_register_cb(EVENT_MOD_WIFI, EVENT_WIFI_STA_DISCONNECTED,
        bk_bridge_event_cb, NULL));
}

static void bk_bridge_unregister_event_cb(void)
{
    BK_LOG_ON_ERR(bk_event_unregister_cb(EVENT_MOD_NETIF, EVENT_NETIF_GOT_IP4,
        bk_bridge_event_cb));
    BK_LOG_ON_ERR(bk_event_unregister_cb(EVENT_MOD_WIFI, EVENT_WIFI_STA_DISCONNECTED,
        bk_bridge_event_cb));
}

static void bk_bridge_abort_enabling(void)
{
    bk_bridge_unregister_event_cb();
    bk_br_bringup_invalidate();
    if (bk_bridge_ctx_get_state() == BRIDGE_STATE_ENABLING) {
        bk_bridge_fsm_transition(BRIDGE_STATE_DISABLING);
        bk_bridge_fsm_transition(BRIDGE_STATE_DISABLED);
    }
}

static void bk_br_start_rollback(bool ap_started)
{
    /*
     * Called from br_start_thread on failure — do not
     * bk_br_wait_bringup_thread_idle() here (deadlock).
     */
    if (ap_started) {
        bk_wifi_ap_stop();
    }
    bridge_netif_teardown();
    if (bk_bridge_ctx_get_state() == BRIDGE_STATE_ENABLING ||
        bk_bridge_ctx_get_state() == BRIDGE_STATE_ENABLED) {
        bk_bridge_fsm_transition(BRIDGE_STATE_DISABLING);
        bk_bridge_fsm_transition(BRIDGE_STATE_DISABLED);
    }
}

void bk_br_start_internal(void* data)
{
    int len, key_len = 0;
    uint8_t mac[6] = {0};
    bridgeif_initdata_t mybr_initdata = {0};
    netif_ip4_config_t ip4_config = {0};
    ip4_addr_t my_ip, my_gw, my_mask;
    wifi_link_status_t link_status = {0};
    wifi_ap_config_t ap_config = {0};//WIFI_DEFAULT_AP_CONFIG();
    wifi_link_status_t link_sta_status = {0};
    bk_bridge_config_t *bridge_config = bk_bridge_ctx_get_config();
    struct netif *br_netif = (struct netif *)net_get_br_handle();
    bool ap_started = false;
    uint32_t bringup_token;
    err_t lwip_err;

    (void)data;

    /* Snapshot generation; stop/abort may ++ s_br_bringup_token before we finish. */
    bringup_token = s_br_bringup_token;

    WIFI_LOGI("bk_wifi_start_softap_for_bridge, ssid: %s, key: %s, channel: %d, hostname: %s, state: %d\r\n",
            bridge_config->br_info.ssid, bridge_config->sta_config.password,
            bridge_config->br_info.channel, bridge_config->hostname,
            bk_bridge_ctx_get_state());
    if (!bk_br_bringup_active(bringup_token)) {
        goto done;
    }

    len = os_strlen(bridge_config->br_info.ssid);
    if (bridge_config->sta_config.password[0]) {
        key_len = os_strlen(bridge_config->sta_config.password);
    }
    if (SSID_MAX_LEN < len) {
        WIFI_LOGE("ssid name more than 32 Bytes\r\n");
        goto fail;
    }
    if (0 == len) {
        WIFI_LOGE("ssid name must not be null\r\n");
        goto fail;
    }
    if (8 > key_len) {
        WIFI_LOGE("key less than 8 Bytes, the security will be set NONE\r\n");
    }
    if (64 < key_len) {
        WIFI_LOGE("key more than 64 Bytes\r\n");
        goto fail;
    }

    if (!bk_br_bringup_active(bringup_token)) {
        goto fail;
    }

    bk_wifi_sta_get_mac(mac);
    os_memcpy(br_netif->hwaddr, mac, 6);
    os_memcpy(&mybr_initdata.ethaddr, mac, 6);
    mybr_initdata.max_fdb_dynamic_entries = 16;
    mybr_initdata.max_fdb_static_entries = 4;
    mybr_initdata.max_ports = 4;
    bk_netif_get_ip4_config(NETIF_IF_STA, &ip4_config);
    inet_aton((char *)&ip4_config.ip, &my_ip);
    inet_aton((char *)&ip4_config.gateway, &my_gw);
    inet_aton((char *)&ip4_config.mask, &my_mask);

    lwip_err = netifapi_netif_add(br_netif, &my_ip, &my_mask, &my_gw,
                                  &mybr_initdata, bridgeif_init, netif_input);
    if (lwip_err != ERR_OK) {
        WIFI_LOGE("bridge netif_add failed, err=%d\r\n", lwip_err);
        goto fail;
    }

    lwip_err = bridgeif_add_port(br_netif, (struct netif *)net_get_sta_handle());
    if (lwip_err != ERR_OK) {
        WIFI_LOGE("bridge add sta port failed, err=%d\r\n", lwip_err);
        goto fail;
    }

    if (bridge_config->hostname) {
        netif_set_hostname((struct netif *)net_get_sta_handle(), bridge_config->hostname);
    } else {
        netif_set_hostname((struct netif *)net_get_sta_handle(), "Beken_Bridge");
    }
    bk_wifi_sta_get_link_status(&link_status);

    if (!bk_br_bringup_active(bringup_token)) {
        goto fail;
    }

    os_strcpy(ip4_config.ip, WLAN_ANY_IP);
    os_strcpy(ip4_config.mask, WLAN_ANY_IP);
    os_strcpy(ip4_config.gateway, WLAN_ANY_IP);
    os_strcpy(ip4_config.dns, WLAN_ANY_IP);
    bk_netif_set_ip4_config(NETIF_IF_AP, &ip4_config);
    os_memcpy(&ap_config, &bridge_config->br_info, sizeof(ap_config));
    os_memset(&link_sta_status, 0x0, sizeof(link_sta_status));
    bk_wifi_sta_get_link_status(&link_sta_status);
    if (link_sta_status.security == WIFI_SECURITY_NONE) {
        ap_config.password[0] = '\0';
    } else if (bridge_config->sta_config.password[0]) {
        os_strcpy(ap_config.password, bridge_config->sta_config.password);
    }
    ap_config.security = link_sta_status.security;
    ap_config.disable_dns_server = 1;
    WIFI_LOGD("ssid:%s  key:%s\r\n", ap_config.ssid, ap_config.password);
    bk_wifi_ap_set_config(&ap_config);
    if (bk_wifi_ap_start() != BK_OK) {
        WIFI_LOGE("bridge softap start failed\r\n");
        goto fail;
    }
    ap_started = true;

    if (!bk_br_bringup_active(bringup_token)) {
        goto fail;
    }

    lwip_err = bridgeif_add_port(br_netif, (struct netif *)net_get_uap_handle());
    if (lwip_err != ERR_OK) {
        WIFI_LOGE("bridge add uap port failed, err=%d\r\n", lwip_err);
        goto fail;
    }

    netifapi_netif_set_default(net_get_br_handle());
    netifapi_netif_set_up(br_netif);
    bridge_set_ip_start_flag(true);

    if (!bk_br_bringup_active(bringup_token)) {
        goto fail;
    }
    bk_bridge_fsm_transition(BRIDGE_STATE_ENABLED);
    goto done;

fail:
    bk_br_start_rollback(ap_started);

done:
    s_br_start_thread_running = false;
    rtos_delete_thread(NULL);
}

static bk_err_t bk_wifi_start_softap_for_bridge(void)
{
    bk_err_t ret = BK_OK;

    if (bk_bridge_ctx_get_state() != BRIDGE_STATE_ENABLING) {
        return BK_OK;
    }

    if (s_br_start_thread_running) {
        WIFI_LOGW("bridge bring-up thread already running\r\n");
        return BK_OK;
    }

    /* Retry on a later GOT_IP if a previous attempt exited without br0 up. */
    if (bridge_ip_is_start()) {
        return BK_OK;
    }

    s_br_start_thread_running = true;
    ret = rtos_create_thread(&br_start_thread_internal,
        BEKEN_APPLICATION_PRIORITY,
        "br_start_thread_internal",
        (beken_thread_function_t)bk_br_start_internal,
        4*1024,
        0);
    if (ret != BK_OK) {
        WIFI_LOGE("create bridge start thread failed\r\n");
        s_br_start_thread_running = false;
    }
    return ret;
}

static void bk_wifi_switch_bridge_to_sta(void)
{
    bk_bridge_state_t st = bk_bridge_ctx_get_state();
    /*
     * Only tear down softap/br0 after bridge is fully up.  Transient STA
     * disconnect during ENABLING (assoc deauth, roam) must not abort
     * bring-up; br_start_thread / bk_bridge_abort_enabling() own that path.
     */
    if (st == BRIDGE_STATE_ENABLED) {
        /*
         * Tell CP DISABLING before we tear down br0 / sap.  Otherwise
         * CP fast-path keeps believing bridge is ENABLED and would
         * forward IPC frames to AP while bridgeif_private is being freed.
         */
        bk_br_bringup_invalidate();
        bk_bridge_fsm_transition(BRIDGE_STATE_DISABLING);
        bk_br_wait_bringup_thread_idle();
        bridge_ip_stop();
        /*just stop softap, station still work*/
        netifapi_netif_set_default(net_get_sta_handle());
        bridge_set_ip_start_flag(false);
        bk_wifi_ap_stop();
        bk_bridge_fsm_transition(BRIDGE_STATE_DISABLED);
    }
}
/*private apis for beken bridge end*/

bk_err_t bk_bridge_stop(void)
{
    bk_bridge_unregister_event_cb();

    /*
     * The `state > DISABLING` guard filters out DISABLED/DISABLING before
     * doing any work.  We keep it so redundant stop does not pay for two
     * FSM acquisitions or log a no-op DISABLED -> DISABLING attempt.
     */
    if (bk_bridge_ctx_get_state() > BRIDGE_STATE_DISABLING) {
        bk_bridge_config_t *br_cfg = bk_bridge_ctx_get_config();

        bk_br_bringup_invalidate();
        bk_bridge_fsm_transition(BRIDGE_STATE_DISABLING);
        if (!br_cfg->keep_sta_on_close) {
            if (bk_wifi_sta_stop())
                BK_LOGD(NULL, "bridge stop sta fail\r\n");
        }
        if(bk_wifi_ap_stop())
            BK_LOGD(NULL, "bridge stop ap fail\r\n");
        bk_br_wait_bringup_thread_idle();
        bridge_ip_stop();
        bk_bridge_fsm_transition(BRIDGE_STATE_DISABLED);
    }
    return BK_OK;
}

bk_err_t bk_bridge_start(bk_bridge_config_t *br_config)
{
    int len = 0;
    bk_bridge_state_t st;

    if (br_config == NULL) {
        WIFI_LOGE("%s failed, invalid pointer\r\n", __func__);
        return BK_ERR_PARAM;
    }

    st = bk_bridge_ctx_get_state();
    if (st != BRIDGE_STATE_DISABLED) {
        WIFI_LOGW("bridge already open, please close first\r\n");
        return BK_FAIL;
    }

    bk_bridge_fsm_transition(BRIDGE_STATE_ENABLING);
    bk_wifi_save_bridge_config(br_config);
    bk_bridge_register_event_cb();

    len = os_strlen(br_config->sta_config.ssid);
    if (33 < len) {
        LWIP_LOGD("ssid name more than 32 Bytes\r\n");
        bk_bridge_abort_enabling();
        return BK_FAIL;
    }

    LWIP_LOGD("ssid:%s key:%s\r\n", br_config->sta_config.ssid,
              br_config->sta_config.password);
    BK_LOG_ON_ERR(bk_wifi_sta_set_config(&br_config->sta_config));
    BK_LOG_ON_ERR(bk_wifi_sta_start());

    return BK_OK;
}
#endif

#if CONFIG_P2P
bk_err_t bk_wifi_p2p_enable(const char *ssid)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    const char *default_ssid = "BEKEN SMP_P2P";
    const char *actual_ssid = NULL;

    ret = wifi_p2p_try_reserve();
    if (ret != BK_OK) {
        WIFI_LOGE("%s: p2p enable fail, sta started or p2p enabled\r\n", __func__);
        return ret;
    }

    if (ssid != NULL && os_strlen(ssid) > 0) {
        // Use provided SSID
        actual_ssid = ssid;
    } else if (s_wifi_p2p_dev_name[0] != '\0') {
        // Use saved SSID if available
        actual_ssid = s_wifi_p2p_dev_name;
    } else {
        // Use default SSID
        actual_ssid = default_ssid;
    }

    uint8_t ssid_len = os_strlen(actual_ssid);
    if (ssid_len > SSID_MAX_LEN) {
        WIFI_LOGE("%s: SSID too long (%d > %d)\r\n", __func__, ssid_len, SSID_MAX_LEN);
        wifi_p2p_set_enabled(false);
        return BK_ERR_PARAM;
    }
    buffer_to_ipc = os_malloc(ssid_len + 1);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        wifi_p2p_set_enabled(false);
        return BK_ERR_NO_MEM;
    }
    os_memcpy(buffer_to_ipc, actual_ssid, ssid_len);
    ((char *)buffer_to_ipc)[ssid_len] = '\0'; // Add EOF

    ret = wifi_send_com_api_cmd(P2P_ENABLE, 1, (uint32_t)buffer_to_ipc);
    os_free(buffer_to_ipc);

    if (ret == BK_OK) {
        wifi_p2p_save_dev_name(actual_ssid, ssid_len);
        wdrv_p2p_role_clear();
    } else {
        wifi_p2p_set_enabled(false);
        WIFI_LOGE("AP: IPC P2P_ENABLE FAILED! SSID NOT saved! ret=%d\n", ret);
    }

    return ret;
}

bk_err_t bk_wifi_p2p_enable_with_intent(const char *ssid, int intent)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    const char *default_ssid = "BEKEN SMP_P2P";
    const char *actual_ssid = NULL;

    /* Validate and save default GO Intent if provided:
     * intent: -1 => keep previous/default; 0..15 => new default */
    if (intent < -1 || intent > 15) {
        WIFI_LOGE("%s: invalid intent value %d (must be -1 or 0..15)\r\n", __func__, intent);
        return BK_ERR_PARAM;
    }
    if (intent != -1) {
        s_wifi_p2p_default_intent = intent;
    }

    ret = wifi_p2p_try_reserve();
    if (ret != BK_OK) {
        WIFI_LOGE("%s: p2p enable fail, sta started or p2p enabled\r\n", __func__);
        return ret;
    }

    if (ssid != NULL && os_strlen(ssid) > 0) {
        // Use provided SSID
        actual_ssid = ssid;
    } else if (s_wifi_p2p_dev_name[0] != '\0') {
        // Use saved SSID if available
        actual_ssid = s_wifi_p2p_dev_name;
    } else {
        // Use default SSID
        actual_ssid = default_ssid;
    }

    uint8_t ssid_len = os_strlen(actual_ssid);
    if (ssid_len > SSID_MAX_LEN) {
        WIFI_LOGE("%s: SSID too long (%d > %d)\r\n", __func__, ssid_len, SSID_MAX_LEN);
        wifi_p2p_set_enabled(false);
        return BK_ERR_PARAM;
    }
    buffer_to_ipc = os_malloc(ssid_len + 1);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        wifi_p2p_set_enabled(false);
        return BK_ERR_NO_MEM;
    }
    os_memcpy(buffer_to_ipc, actual_ssid, ssid_len);
    ((char *)buffer_to_ipc)[ssid_len] = '\0'; // Add EOF
    // Pass intent to CP side: use saved default if intent is -1
    int intent_to_send = (intent == -1) ? s_wifi_p2p_default_intent : intent;
    if (intent_to_send == -1) {
        intent_to_send = 0;  // Fallback to default GC intent
    }
    ret = wifi_send_com_api_cmd(P2P_ENABLE_WITH_INTENT, 2, (uint32_t)buffer_to_ipc, intent_to_send);
    os_free(buffer_to_ipc);

    if (ret == BK_OK) {
        wifi_p2p_save_dev_name(actual_ssid, ssid_len);
        wdrv_p2p_role_clear();
    } else {
        wifi_p2p_set_enabled(false);
        WIFI_LOGE("AP: IPC P2P_ENABLE FAILED! SSID NOT saved! ret=%d\n", ret);
    }

    return ret;
}

bk_err_t bk_wifi_p2p_find(void)
{
    return wifi_send_com_api_cmd(P2P_FIND, 0);
}

bk_err_t bk_wifi_p2p_listen(void)
{
    return wifi_send_com_api_cmd(P2P_LISTEN, 0);
}

bk_err_t bk_wifi_p2p_stop_find(void)
{
    return wifi_send_com_api_cmd(P2P_STOP_FIND, 0);
}

bk_err_t bk_wifi_p2p_connect(const uint8_t *mac, int method, int intent)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    int actual_intent = intent;

    if (!mac)
        return BK_ERR_NULL_PARAM;

    /* If caller passes -1, use default intent set by bk_wifi_p2p_enable().
     * Fallback to 0 (GC preference) if no default configured. */
    if (actual_intent == -1) {
        if (s_wifi_p2p_default_intent >= 0) {
            actual_intent = s_wifi_p2p_default_intent;
        } else {
            actual_intent = 0;
        }
    }

    buffer_to_ipc = os_malloc(WIFI_MAC_LEN);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memcpy(buffer_to_ipc, mac, WIFI_MAC_LEN);
    ret = wifi_send_com_api_cmd(P2P_CONNECT, 3, (uint32_t)buffer_to_ipc, method, actual_intent);
    os_free(buffer_to_ipc);

    return ret;
}

bk_err_t bk_wifi_p2p_cancel(void)
{
    bk_err_t ret;
#if CONFIG_WIFI_VNET_CONTROLLER
    int role = (int)wdrv_host_env.p2p_role;

    if (role == 1)
        p2p_go_ip_down();
    else if (role == 2)
        p2p_gc_ip_down();
#endif
    ret = wifi_send_com_api_cmd(P2P_CANCEL, 0);
#if CONFIG_WIFI_VNET_CONTROLLER
    if (role != 0)
        wdrv_p2p_role_clear();
#endif
    return ret;
}

bool bk_wifi_is_p2p_enabled(void)
{
    return wifi_p2p_get_enabled();
}

const char *bk_wifi_get_p2p_dev_name(void)
{
    // Return saved SSID, NULL if empty
    if (s_wifi_p2p_dev_name[0] != '\0') {
        return s_wifi_p2p_dev_name;
    }
    return NULL;
}

bk_err_t bk_wifi_p2p_disable(void)
{
    bk_err_t ret = wifi_send_com_api_cmd(P2P_DISABLE, 0);
    if (ret == BK_OK) {
        wifi_p2p_set_enabled(false);
        wdrv_p2p_role_clear();
    }
    return ret;
}

#endif