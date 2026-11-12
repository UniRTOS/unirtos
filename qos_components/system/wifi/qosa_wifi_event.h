#ifndef __QOSA_WIFI_EVENT_H__
#define __QOSA_WIFI_EVENT_H__

#include "qosa_wifi.h"

#ifdef __cplusplus
extern "C" {
#endif

#define QOSA_WIFI_EVENT_MAC_LEN 18U
#define QOSA_WIFI_EVENT_IP_LEN  16U

typedef enum
{
    QOSA_WIFI_EVENT_CLIENT_CONNECTED = 0,
    QOSA_WIFI_EVENT_CLIENT_DISCONNECTED,
    QOSA_WIFI_EVENT_LWIP_ADD_PC,
    QOSA_WIFI_EVENT_LWIP_ADD_WLAN_CLIENT,
    QOSA_WIFI_EVENT_LWIP_ADD_ETH_SWITCH,
    QOSA_WIFI_EVENT_LWIP_DELETE_PC,
    QOSA_WIFI_EVENT_LWIP_DELETE_WLAN_CLIENT,
    QOSA_WIFI_EVENT_LWIP_DELETE_ETH_SWITCH,
    QOSA_WIFI_EVENT_MAX
} qosa_wifi_event_id_e;

#define QOSA_WIFI_EVENT_MASK(id)                     ((qosa_uint32_t)1U << (qosa_uint32_t)(id))
#define QOSA_WIFI_EVENT_MASK_CLIENT_CONNECTED        QOSA_WIFI_EVENT_MASK(QOSA_WIFI_EVENT_CLIENT_CONNECTED)
#define QOSA_WIFI_EVENT_MASK_CLIENT_DISCONNECTED     QOSA_WIFI_EVENT_MASK(QOSA_WIFI_EVENT_CLIENT_DISCONNECTED)
#define QOSA_WIFI_EVENT_MASK_LWIP_ADD_PC             QOSA_WIFI_EVENT_MASK(QOSA_WIFI_EVENT_LWIP_ADD_PC)
#define QOSA_WIFI_EVENT_MASK_LWIP_ADD_WLAN_CLIENT    QOSA_WIFI_EVENT_MASK(QOSA_WIFI_EVENT_LWIP_ADD_WLAN_CLIENT)
#define QOSA_WIFI_EVENT_MASK_LWIP_ADD_ETH_SWITCH     QOSA_WIFI_EVENT_MASK(QOSA_WIFI_EVENT_LWIP_ADD_ETH_SWITCH)
#define QOSA_WIFI_EVENT_MASK_LWIP_DELETE_PC          QOSA_WIFI_EVENT_MASK(QOSA_WIFI_EVENT_LWIP_DELETE_PC)
#define QOSA_WIFI_EVENT_MASK_LWIP_DELETE_WLAN_CLIENT QOSA_WIFI_EVENT_MASK(QOSA_WIFI_EVENT_LWIP_DELETE_WLAN_CLIENT)
#define QOSA_WIFI_EVENT_MASK_LWIP_DELETE_ETH_SWITCH  QOSA_WIFI_EVENT_MASK(QOSA_WIFI_EVENT_LWIP_DELETE_ETH_SWITCH)
#define QOSA_WIFI_EVENT_MASK_ALL                     (((qosa_uint32_t)1U << (qosa_uint32_t)QOSA_WIFI_EVENT_MAX) - 1U)

typedef struct
{
    qosa_wifi_event_id_e event_id;
    char mac[QOSA_WIFI_EVENT_MAC_LEN];
    char ip[QOSA_WIFI_EVENT_IP_LEN];
} qosa_wifi_event_info_t;

typedef void (*qosa_wifi_event_cb_t)(const qosa_wifi_event_info_t *event_info, void *context);

qosa_wifi_err_e qosa_wifi_event_init(void);
qosa_wifi_err_e qosa_wifi_event_register(qosa_uint32_t event_mask, qosa_wifi_event_cb_t callback, void *context, qosa_uint32_t *handle);
qosa_wifi_err_e qosa_wifi_event_unregister(qosa_uint32_t handle);
qosa_wifi_err_e qosa_wifi_event_notify(const qosa_wifi_event_info_t *event_info);

#ifdef __cplusplus
}
#endif

#endif /* __QOSA_WIFI_EVENT_H__ */
