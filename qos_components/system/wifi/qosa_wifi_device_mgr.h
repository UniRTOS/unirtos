#ifndef __QOSA_WIFI_DEVICE_MGR_H__
#define __QOSA_WIFI_DEVICE_MGR_H__

#include "qosa_wifi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    QOSA_WIFI_CONN_TYPE_WIFI = 0,
    QOSA_WIFI_CONN_TYPE_ETH = 1,
} qosa_wifi_conn_type_e;

typedef struct
{
    qosa_uint8_t ip[4];
    char mac[QOSA_WIFI_DEVICE_MAC_STR_LEN];
    char name[QOSA_WIFI_DEVICE_NAME_STR_LEN];
} qosa_wifi_sta_info_t;

typedef struct
{
    qosa_uint16_t sta_count;
    qosa_wifi_sta_info_t info[QOSA_WIFI_MAX_NUM_CLIENTS];
} qosa_wifi_sta_list_t;

typedef struct
{
    char mac[QOSA_WIFI_DEVICE_MAC_STR_LEN];
    char name[QOSA_WIFI_DEVICE_NAME_STR_LEN];
} qosa_wifi_blacklist_info_t;

typedef struct
{
    qosa_uint16_t count;
    qosa_wifi_blacklist_info_t info[QOSA_WIFI_MAX_BLACKLIST_NUM];
} qosa_wifi_blacklist_t;

typedef struct
{
    char mac[QOSA_WIFI_DEVICE_MAC_STR_LEN];
    char name[QOSA_WIFI_DEVICE_NAME_STR_LEN];
    char ip[QOSA_WIFI_DEVICE_IP_STR_LEN];
    qosa_wifi_conn_type_e conn_type;
    qosa_uint32_t first_connect_time;
    qosa_uint32_t last_connect_time;
    qosa_uint32_t connect_count;
    qosa_bool_t is_blocked;
} qosa_wifi_device_history_info_t;

typedef struct
{
    qosa_uint16_t count;
    qosa_wifi_device_history_info_t info[QOSA_WIFI_DEVICE_HISTORY_MAX_NUM];
} qosa_wifi_device_history_list_t;

qosa_bool_t qosa_wifi_device_mgr_is_valid_mac(const char *mac);
qosa_wifi_err_e qosa_wifi_device_mgr_get_stainfo(qosa_wifi_sta_list_t *sta_list);
qosa_wifi_err_e qosa_wifi_device_mgr_get_sta_count(qosa_uint16_t *sta_count);
qosa_wifi_err_e qosa_wifi_device_mgr_get_blacklist(qosa_wifi_blacklist_t *blacklist);
qosa_wifi_err_e qosa_wifi_device_mgr_add_to_blacklist(const char *mac, const char *name);
qosa_wifi_err_e qosa_wifi_device_mgr_remove_from_blacklist(const char *mac);
qosa_wifi_err_e qosa_wifi_device_mgr_set_sta_name(const char *mac, const char *name);
qosa_wifi_err_e qosa_wifi_device_mgr_get_device_history(qosa_wifi_device_history_list_t *list);
qosa_wifi_err_e qosa_wifi_device_mgr_get_device_history_by_mac(const char *mac, qosa_wifi_device_history_info_t *info);
qosa_wifi_err_e qosa_wifi_device_mgr_disconnect_sta(const char *mac);

#ifdef __cplusplus
}
#endif

#endif /* __QOSA_WIFI_DEVICE_MGR_H__ */
