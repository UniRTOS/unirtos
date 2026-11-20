#ifndef __DM_BT_PBAP_PCE_TASK_H__
#define __DM_BT_PBAP_PCE_TASK_H__

#include "components/bluetooth/bk_dm_pbap_pce.h"
#include "dm_bluetooth_task.h"

enum
{
    BT_ETHERMIND_API_REQ_SUBMSG_PBAP_PCE_INIT = BLUETOOTH_API_GROUP(BT_ETHERMIND_MSG_PBAP_PCE_REQ),
    BT_ETHERMIND_API_REQ_SUBMSG_PBAP_PCE_DEINIT,
    BT_ETHERMIND_API_REQ_SUBMSG_PBAP_PCE_REGISTER_CB,
    BT_ETHERMIND_API_REQ_SUBMSG_PBAP_PCE_CONNECT,
    BT_ETHERMIND_API_REQ_SUBMSG_PBAP_PCE_DISCONNECT,
    BT_ETHERMIND_API_REQ_SUBMSG_PBAP_PCE_GET_PHONEBOOK,
    BT_ETHERMIND_API_REQ_SUBMSG_PBAP_PCE_SET_PHONEBOOK,
    BT_ETHERMIND_API_REQ_SUBMSG_PBAP_PCE_GET_VCARD_LIST,
    BT_ETHERMIND_API_REQ_SUBMSG_PBAP_PCE_GET_VCARD,
    BT_ETHERMIND_API_REQ_SUBMSG_PBAP_PCE_GET_SIZE,
    BT_ETHERMIND_API_REQ_SUBMSG_PBAP_PCE_ABORT,
    BT_ETHERMIND_API_REQ_SUBMSG_PBAP_PCE_SEND_AUTH_RESPONSE,
};

typedef struct
{
    uint8_t  bd_addr[BK_BD_ADDR_LEN];
    uint8_t  auth_required;
    uint16_t pin_len;
    uint8_t  pin[BK_PBAP_PCE_MAX_PIN_LEN];
    uint16_t user_id_len;
    uint8_t  user_id[BK_PBAP_PCE_MAX_USER_ID_LEN];
    uint16_t max_recv_size;
} bt_pbap_pce_connect_msg_t;

typedef struct
{
    uint8_t bd_addr[BK_BD_ADDR_LEN];
} bt_pbap_pce_bd_addr_msg_t;

typedef struct
{
    char     object_name[BK_PBAP_PCE_MAX_OBJECT_NAME_LEN];
    uint16_t max_list_count;
    uint16_t list_start_offset;
    uint64_t filter;                  /* property selector (get_phonebook / get_vcard) */
    uint8_t  format;                  /* vCard format (0 = 2.1, 1 = 3.0) */
    uint8_t  reset_new_missed_calls;   /* reset new missed calls (mch/cch) */
    uint64_t vcard_selector;          /* vCard selector (0 = no selector) */
    uint8_t  vcard_selector_operator; /* 0 = OR, 1 = AND */
    uint8_t  order;                   /* sorting order (get_vcard_list) */
} bt_pbap_pce_get_msg_t;

typedef struct
{
    uint8_t path_type;
    char    path_name[BK_PBAP_PCE_MAX_OBJECT_NAME_LEN];
} bt_pbap_pce_set_path_msg_t;

typedef struct
{
    uint8_t  bd_addr[BK_BD_ADDR_LEN];
    uint16_t pin_len;
    uint8_t  pin[BK_PBAP_PCE_MAX_PIN_LEN];
    uint16_t user_id_len;
    uint8_t  user_id[BK_PBAP_PCE_MAX_USER_ID_LEN];
} bt_pbap_pce_auth_msg_t;

typedef union
{
    bt_pbap_pce_connect_msg_t  connect;
    bt_pbap_pce_bd_addr_msg_t  bd_addr_only;
    struct
    {
        uint8_t bd_addr[BK_BD_ADDR_LEN];
        bt_pbap_pce_get_msg_t get_param;
    } get_phonebook;
    struct
    {
        uint8_t bd_addr[BK_BD_ADDR_LEN];
        bt_pbap_pce_set_path_msg_t set_param;
    } set_phonebook;
    struct
    {
        uint8_t bd_addr[BK_BD_ADDR_LEN];
        bt_pbap_pce_get_msg_t get_param;
    } get_vcard_list;
    struct
    {
        uint8_t bd_addr[BK_BD_ADDR_LEN];
        bt_pbap_pce_get_msg_t get_param;
    } get_vcard;
    struct
    {
        uint8_t bd_addr[BK_BD_ADDR_LEN];
        bt_pbap_pce_get_msg_t get_param;
    } get_size;
    bt_pbap_pce_auth_msg_t auth_response;
} bt_ethermind_pbap_pce_msg_t;

void bt_pbap_pce_register_internal_callback(bk_pbap_pce_cb_t cb);

#endif
