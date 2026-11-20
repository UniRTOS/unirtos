#pragma once

#include <stdint.h>

#define BK_PERFORMANCE_PROFILE_ENABLE 1

int32_t bk_performance_test_profile_init(
    uint16_t service_uuid,
    uint16_t char_uuid,
    uint8_t tx_method,
    int32_t (*recv_cb)(uint16_t gatt_conn_handle, uint8_t *data, uint32_t len));

int32_t bk_performance_test_profile_deinit(uint8_t deinit_bluetooth_future);

void bk_performance_test_profile_set_data_len(uint16_t len);
void bk_performance_test_profile_set_interval(uint16_t interval);
void bk_performance_test_profile_enable_tx(uint8_t enable);
void bk_performance_test_profile_enable_statistics(uint8_t enable, uint8_t type);
