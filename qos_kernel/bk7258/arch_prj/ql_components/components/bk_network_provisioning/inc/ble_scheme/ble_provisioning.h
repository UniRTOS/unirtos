#ifndef __BLE_PROVISIONING_H__
#define __BLE_PROVISIONING_H__

typedef void (*ble_provisioning_op_cb_t)(uint16_t opcode, uint16_t length, uint8_t *data);
typedef struct
{
    char *ssid_value;
    char *password_value;
    ble_provisioning_op_cb_t cb;
    uint8_t boarding_notify[2];
    uint16_t ssid_length;
    uint16_t password_length;
} ble_provisioning_info_t;

typedef struct
{
    ble_provisioning_info_t ble_prov_info;
    uint16_t channel;
} bk_ble_provisioning_info_t;

typedef struct
{
    int32_t event;
    uint32_t param;
    uint16_t length;
} ble_prov_msg_t;

typedef void (*ble_msg_handle_cb_t)(ble_prov_msg_t *msg);

bk_ble_provisioning_info_t * bk_ble_provisioning_get_boarding_info(void);
// int bk_ble_provisioning_init(void);
// int bk_ble_provisioning_deinit(void);
void bk_ble_provisioning_event_notify(uint16_t opcode, int status);
void bk_ble_provisioning_event_notify_with_data(uint16_t opcode, int status, char *payload, uint16_t length);

#endif
