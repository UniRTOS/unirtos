#pragma once

#include "components/bluetooth/bk_dm_gattc.h"
#include "components/bluetooth/bk_dm_gatt_common.h"
#include "dm_gatt.h"

typedef int32_t (* dm_ble_gattc_app_cb)(bk_gattc_cb_event_t event, bk_gatt_if_t gattc_if, bk_ble_gattc_cb_param_t *comm_param);

int bk_at_dm_gattc_main(cli_gatt_param_t *param);
int bk_at_dm_gattc_deinit();
int32_t bk_at_dm_gattc_connect(uint8_t *addr, uint32_t addr_type);
int32_t bk_at_dm_gattc_connect_ext(uint8_t *addr, uint32_t addr_type, bk_gap_create_conn_params_t *pm);
int32_t bk_at_dm_gattc_disconnect(uint8_t *addr);
int32_t bk_at_dm_gattc_connect_cancel(void);
int32_t bk_at_dm_gattc_discover(uint16_t conn_id);
int32_t bk_at_dm_gattc_write(uint16_t conn_id, uint16_t attr_handle, uint8_t *data, uint32_t len);
int32_t bk_at_dm_gattc_write_ext(uint16_t gatt_conn_id, uint16_t attr_handle, uint8_t *data, uint32_t len, uint8_t write_req);
int32_t bk_at_dm_gattc_read(uint16_t gatt_conn_id, uint16_t attr_handle, uint8_t *data, uint32_t len);
int32_t bk_at_dm_gattc_send_mtu_req(uint8_t *mac, uint8_t gatt_conn_id);
int bk_at_dm_gattc_add_gattc_callback(void *param);
