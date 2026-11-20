#include <components/netif.h>
#include <components/log.h>
#include <common/bk_err.h>
#include <os/mem.h>
#include <os/str.h>
#include <common/sys_config.h>
#if CONFIG_NETIF_LWIP
#include "lwip/inet.h"
#include "net.h"
#include "lwip/netif.h"
#endif
#include <components/event.h>
#ifndef CONFIG_WIFI_VNET_CONTROLLER
#include "wdrv_cntrl.h"
#else
#include "bk_private/bk_wifi.h"
#include "wifi_api_ipc.h"
#endif
#if CONFIG_P2P
#include "wifi_api.h"
#endif

uint8 sta_static_ip_flag = 0;

bk_err_t bk_netif_static_ip(netif_ip4_config_t static_ip4_config)
{
	__maybe_unused bk_err_t ret = BK_OK;
	__maybe_unused void *buffer_to_ipc = NULL;
	__maybe_unused uint32_t len_ip4_config = sizeof(netif_ip4_config_t);

	sta_static_ip_flag = 1;

#ifdef CONFIG_WIFI_VNET_CONTROLLER
	/* forward static IP configuration to CP */
	buffer_to_ipc = os_malloc(len_ip4_config);
	if (!buffer_to_ipc) {
		BK_LOGE(NULL, "%s malloc failed\r\n", __func__);
		return BK_ERR_NO_MEM;
	}
	os_memcpy(buffer_to_ipc, &static_ip4_config, len_ip4_config);
	ret = wifi_send_com_api_cmd(STA_SET_IP4_STATIC_IP, 1, (uint32_t)buffer_to_ipc);
	if (ret != BK_OK) {
		BK_LOGE(NULL, "%s set sta netif ip4 config failed, ret=%d\n", __func__, ret);
		os_free(buffer_to_ipc);
		return ret;
	}
	os_free(buffer_to_ipc);
#endif

	return BK_OK;
}

bk_err_t netif_wifi_event_cb(void *arg, event_module_t event_module,
                int event_id, void *event_data)
{
	if (event_module != EVENT_MOD_WIFI) {
		return BK_OK;
	}

	switch (event_id) {
#ifdef CONFIG_WIFI_VNET_CONTROLLER
    case EVENT_WIFI_STA_CONNECTED:
		break;
#endif
	case EVENT_WIFI_STA_DISCONNECTED:
#if CONFIG_NETIF_LWIP
		sta_ip_down();
		sta_static_ip_flag = 0;
		sta_ip_mode_set(1);
#endif
		break;
#if CONFIG_P2P
	case EVENT_WIFI_GC_CONNECTED:
#if CONFIG_NETIF_LWIP && !CONFIG_WIFI_VNET_CONTROLLER
		p2p_gc_ip_start();
#endif
		break;
	case EVENT_WIFI_GC_DISCONNECTED:
#if CONFIG_NETIF_LWIP
		p2p_gc_ip_down();
#endif
		break;
#endif
	default:
		return BK_OK;
	}

	return BK_OK;
}

bk_err_t bk_netif_init(void)
{
#if (!CONFIG_RTT) && CONFIG_NO_HOSTED
	extern void net_wlan_initial(void);
	net_wlan_initial();
#endif

	BK_LOG_ON_ERR(bk_event_register_cb(EVENT_MOD_WIFI, EVENT_ID_ALL,
			netif_wifi_event_cb, NULL));
	return BK_OK;
}

#if CONFIG_NETIF_LWIP
bk_err_t netif_validate_ip4_config(const netif_ip4_config_t *ip4_config)
{
	if (!ip4_config) {
		return BK_ERR_NULL_PARAM;
	}

	return BK_OK;
}

bk_err_t bk_netif_set_ip4_config(netif_if_t ifx, const netif_ip4_config_t *ip4_config)
{
	netif_ip4_config_t *config = (netif_ip4_config_t*)ip4_config;
	int ret;
	uint32_t len_ip4_config = sizeof(netif_ip4_config_t);
	void *buffer_to_ipc = NULL;

	ret = netif_validate_ip4_config(ip4_config);
	if (ret != BK_OK) {
		return ret;
	}

	buffer_to_ipc = os_malloc(len_ip4_config);
	if (!buffer_to_ipc)
	{
		BK_LOGE(NULL, "%s malloc failed\r\n", __func__);
		return BK_ERR_NO_MEM;
	}
	os_memcpy(buffer_to_ipc, ip4_config, len_ip4_config);
	ret = wifi_send_com_api_cmd(AP_NETIF_IP4_CONFIG, 1, (uint32_t)buffer_to_ipc);
	if (ret != BK_OK)
	{
		BK_LOGE(NULL, "%s set ap netif ip4 config failed, ret=%d\n", __func__, ret);
		return ret;
	}
	os_free(buffer_to_ipc);

	if (ifx == NETIF_IF_STA) {
		ip_address_set(1 /*STA*/, 0/*static IP*/, config->ip, config->mask, config->gateway, config->dns);
	} else if (ifx == NETIF_IF_AP) {
		ip_address_set(0 /*AP*/, 0/*static IP*/, config->ip, config->mask, config->gateway, config->dns);
	} else {
		return BK_ERR_NETIF_IF;
	}

	return BK_OK;
}

bk_err_t bk_netif_get_ip4_config(netif_if_t ifx, netif_ip4_config_t *ip4_config)
{
	struct wlan_ip_config addr;

	if (!ip4_config) {
		return BK_ERR_NULL_PARAM;
	}

	os_memset(&addr, 0, sizeof(struct wlan_ip_config));
	if (ifx == NETIF_IF_STA) {
		net_get_if_addr(&addr, net_get_sta_handle());
	} else if (ifx == NETIF_IF_AP) {
		net_get_if_addr(&addr, net_get_uap_handle());
#ifdef CONFIG_ETH
	} else if (ifx == NETIF_IF_ETH) {
		net_get_if_addr(&addr, net_get_eth_handle());
#endif
#if CONFIG_BRIDGE
	} else if (ifx == NETIF_IF_BRIDGE) {
		net_get_if_addr(&addr, net_get_br_handle());
#endif
#if CONFIG_NET_PAN
	} else if (ifx == NETIF_IF_PAN) {
		net_get_if_addr(&addr, net_get_pan_handle());
#endif
#if CONFIG_LWIP_PPP_SUPPORT
	} else if (ifx == NETIF_IF_PPP) {
		net_get_if_addr(&addr, net_get_ppp_netif_handle());
#endif
#if CONFIG_BK_MODEM
	} else if (ifx == NETIF_IF_MODEM) {
		net_get_if_addr(&addr, net_get_modem_handle());
#endif
#if CONFIG_P2P
	} else if (ifx == NETIF_IF_P2P) {
		int role = 0;

		if (bk_wifi_p2p_get_role(&role) != BK_OK || role == 0)
			return BK_ERR_NETIF_IF;
		if (role == 1)
			net_get_p2p_go_if_addr(&addr);
		else
			net_get_p2p_gc_if_addr(&addr);
#endif
	} else {
		return BK_ERR_NETIF_IF;
	}

	os_strcpy(ip4_config->ip, inet_ntoa(addr.ipv4.address));
	os_strcpy(ip4_config->mask, inet_ntoa(addr.ipv4.netmask));
	os_strcpy(ip4_config->gateway, inet_ntoa(addr.ipv4.gw));
	os_strcpy(ip4_config->dns, inet_ntoa(addr.ipv4.dns1));

	return BK_OK;
}

#if CONFIG_IPV6
bk_err_t bk_netif_get_ip6_addr_info(netif_if_t ifx)
{
       int i;
       int num = 0;
       struct netif *netif;

       if (ifx == NETIF_IF_STA) {
               netif = net_get_sta_handle();
       } else if (ifx == NETIF_IF_AP) {
               netif = net_get_uap_handle();
       } else {
               return BK_ERR_NETIF_IF;
       }

       for (i = 0; i < MAX_IPV6_ADDRESSES; i++) {
                       u8 *ipv6_addr;
                       ipv6_addr = (u8*)ip_2_ip6(&netif->ip6_addr[i])->addr;///&addr->ipv6[i].address;

                       BK_LOGD(NULL, "ipv6_addr[%d] %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x\r\n", i,
                                       ipv6_addr[0], ipv6_addr[1], ipv6_addr[2], ipv6_addr[3],
                                       ipv6_addr[4], ipv6_addr[5], ipv6_addr[6], ipv6_addr[7],
                                       ipv6_addr[8], ipv6_addr[9], ipv6_addr[10], ipv6_addr[11],
                                       ipv6_addr[12], ipv6_addr[13], ipv6_addr[14], ipv6_addr[15]);
                       BK_LOGD(NULL, "ipv6_state[%d] 0x%x\r\n", i, netif->ip6_addr_state[i]);
                       num++;
       }

       return num;
}
#endif

bk_err_t bk_netif_dhcpc_start(netif_if_t ifx)
{
	char *empty_ip = "";

	if (ifx != NETIF_IF_STA) {
		return BK_ERR_NETIF_IF;
	}

	ip_address_set(1/*STA*/, 1/*DHCP enabled*/, empty_ip, empty_ip, empty_ip, empty_ip);
	return BK_OK;
}
#endif
