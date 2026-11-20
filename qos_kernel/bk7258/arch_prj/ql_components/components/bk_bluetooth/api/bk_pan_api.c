
#include "bk_pan_private.h"

bt_err_t bk_bt_pan_register_callback(bk_pan_cb_t callback)
{
    return bt_pan_register_callback_internal(callback);
}

bt_err_t bk_bt_pan_init(uint8_t role)
{
    return bt_pan_init_internal(role);
}

bt_err_t bk_bt_pan_connect(uint8_t *bda, uint8_t src_role, uint8_t dst_role)
{
    return bt_pan_connect_internal(bda, src_role, dst_role);
}

bt_err_t bk_bt_pan_write(uint8_t *bda, eth_data_t *eth_data)
{
    return bt_pan_write_internal(bda, eth_data);
}

bt_err_t bk_bt_pan_disconnect(uint8_t *bda)
{
    return bt_pan_disconnect_internal(bda);
}

bt_err_t bk_bt_pan_set_protocol_filters(uint8_t *bda, np_type_filter_t *np_type)
{
    return bt_pan_set_protocol_filters_internal(bda, np_type);
}

