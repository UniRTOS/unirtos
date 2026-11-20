#include <stdint.h>
#include <string.h>
#include "hal_hci_core.h"
#include <os/mem.h>
#include <os/str.h>
#include "components/bluetooth/bk_ble_types.h"
#include "components/bluetooth/bk_ble.h"
#include "components/bluetooth/bk_dm_bluetooth.h"
#include "bt_ipc_core.h"
#include "hci_distinguish.h"
#include "hal_hci_internal.h"
#include "hci_parse.h"

#define LOG_TAG "hcidrv"
#define LOG_LEVEL LOG_LEVEL_INFO

enum
{
    MULTI_CONTROLLER_MODE_PRI_ALL_SEC_NONE,
    MULTI_CONTROLLER_MODE_PRI_NONE_SEC_ALL,
    MULTI_CONTROLLER_MODE_PRI_BLE_SEC_BT,
    MULTI_CONTROLLER_MODE_PRI_BT_SEC_BLE,
    MULTI_CONTROLLER_MODE_COUNT,
};

enum
{
    MULTI_CONTROLLER_DIR_PRI,
    MULTI_CONTROLLER_DIR_SEC,
    MULTI_CONTROLLER_DIR_ALL,
    MULTI_CONTROLLER_DIR_NOT_SURE,
    MULTI_CONTROLLER_DIR_COUNT,
};

enum
{
    MULTI_CONTROLLER_VOTE_START = 0,
    MULTI_CONTROLLER_VOTE_PRI = MULTI_CONTROLLER_VOTE_START,
    MULTI_CONTROLLER_VOTE_SEC,

    MULTI_CONTROLLER_VOTE_END,
};

typedef struct
{
    uint16_t opcode;
    uint8_t vote;
    uint8_t ret_status;
} multi_ct_cmd_t;

static const uint8_t s_controller_mode =
#if CONFIG_BLUETOOTH_MULTI_CONTROLLER

    #if CONFIG_BLUETOOTH_MULTI_CONTROLLER_MODE_PRI_ALL_SEC_NONE
        MULTI_CONTROLLER_MODE_PRI_ALL_SEC_NONE
    #elif CONFIG_BLUETOOTH_MULTI_CONTROLLER_MODE_PRI_NONE_SEC_ALL
        MULTI_CONTROLLER_MODE_PRI_NONE_SEC_ALL
    #elif CONFIG_BLUETOOTH_MULTI_CONTROLLER_MODE_PRI_BLE_SEC_BT
        MULTI_CONTROLLER_MODE_PRI_BLE_SEC_BT
    #elif CONFIG_BLUETOOTH_MULTI_CONTROLLER_MODE_PRI_BT_SEC_BLE
        MULTI_CONTROLLER_MODE_PRI_BT_SEC_BLE
    #endif

#else
    MULTI_CONTROLLER_MODE_PRI_ALL_SEC_NONE
    //MULTI_CONTROLLER_MODE_PRI_NONE_SEC_ALL
#endif
    ;

static bk_bluetooth_secondary_callback_t *s_secondary_cb;
static beken_queue_t s_multi_controller_cmd_queue = NULL;
static beken_mutex_t s_multi_controller_mutex = NULL;

#if CONFIG_BLUETOOTH_HOST_ENABLE_H5
    static beken_semaphore_t s_multi_controller_sem = NULL;
#endif

static uint8_t hal_hci_arb_cmd_dir(uint8_t current_controller_mode, uint16_t opcode)
{
    static const uint8_t s_cmd_dir_map[MULTI_CONTROLLER_MODE_COUNT][HCI_CMD_TYPE_COUNT] =
    {
        [MULTI_CONTROLLER_MODE_PRI_ALL_SEC_NONE] = {MULTI_CONTROLLER_DIR_PRI, MULTI_CONTROLLER_DIR_PRI, MULTI_CONTROLLER_DIR_PRI, MULTI_CONTROLLER_DIR_NOT_SURE, MULTI_CONTROLLER_DIR_PRI},
        [MULTI_CONTROLLER_MODE_PRI_NONE_SEC_ALL] = {MULTI_CONTROLLER_DIR_SEC, MULTI_CONTROLLER_DIR_SEC, MULTI_CONTROLLER_DIR_SEC, MULTI_CONTROLLER_DIR_NOT_SURE, MULTI_CONTROLLER_DIR_PRI},
        [MULTI_CONTROLLER_MODE_PRI_BLE_SEC_BT] = {MULTI_CONTROLLER_DIR_SEC, MULTI_CONTROLLER_DIR_PRI, MULTI_CONTROLLER_DIR_ALL, MULTI_CONTROLLER_DIR_NOT_SURE, MULTI_CONTROLLER_DIR_PRI},
        [MULTI_CONTROLLER_MODE_PRI_BT_SEC_BLE] = {MULTI_CONTROLLER_DIR_PRI, MULTI_CONTROLLER_DIR_SEC, MULTI_CONTROLLER_DIR_ALL, MULTI_CONTROLLER_DIR_NOT_SURE, MULTI_CONTROLLER_DIR_PRI},
    };

    return s_cmd_dir_map[current_controller_mode][hci_cmd_get_type(opcode)];
}

static ble_err_t hal_hci_driver_send_to_host_ext(uint8_t from, uint8_t *input_buf, uint16_t input_len)
{
    int32_t ret = 0;
    uint8_t type = input_buf[0];
    uint8_t *buf = input_buf + 1;

    multi_ct_cmd_t msg = {0};
    uint8_t need_rep = 0;

    LOGV("from %d hci type %d len %d data %02x%02x%02x%02x%02x%02x", from, type, input_len,
         input_buf[1],
         input_buf[2],
         input_buf[3],
         input_buf[4],
         input_buf[5],
         input_buf[6]);

    switch (type)
    {
    case DATA_TYPE_ACL:
    case DATA_TYPE_SCO:
        need_rep = 1;
        goto end;
        break;
    }

    event_hdr_t *event_hdr = (event_hdr_t *)buf;

    if (s_controller_mode == MULTI_CONTROLLER_MODE_PRI_ALL_SEC_NONE ||
            s_controller_mode == MULTI_CONTROLLER_MODE_PRI_NONE_SEC_ALL)
    {
        need_rep = 1;
        goto end;
    }

    rtos_lock_mutex(&s_multi_controller_mutex);
    ret = rtos_pop_from_queue(&s_multi_controller_cmd_queue, &msg, 0);

    if (ret)
    {
        rtos_unlock_mutex(&s_multi_controller_mutex);
        need_rep = 1;
        goto end;
    }

    uint16_t opcode = 0;
    uint8_t status = 0;
    uint8_t status_pos = 0;

    if (event_hdr->event_code == 0xe) //cmd compl
    {
        status_pos = 5;
        opcode = (((uint16_t)buf[4]) << 8 | buf[3]);
        status = buf[status_pos];
    }
    else if (event_hdr->event_code == 0xf) //cmd status
    {
        status_pos = 2;
        opcode = (((uint16_t)buf[5]) << 8 | buf[4]);
        status = buf[status_pos];
    }

    if (status)
    {
        LOGW("evt 0x%x op 0x%04x status 0x%x from %d dir %d type %d !!!", event_hdr->event_code, opcode, status, from,
             hal_hci_arb_cmd_dir(s_controller_mode, opcode), hci_cmd_get_type(opcode));
    }

    if (msg.opcode == opcode)
    {
        msg.ret_status |= status;
        msg.vote |= (1 << from);

        uint8_t all_vote_mask = 0;

        for (uint32_t i = MULTI_CONTROLLER_VOTE_START; i < MULTI_CONTROLLER_VOTE_END; i++)
        {
            all_vote_mask |= (1 << i);
        }

        if (all_vote_mask != msg.vote)
        {
            ret = rtos_push_to_queue_front(&s_multi_controller_cmd_queue, &msg, BEKEN_WAIT_FOREVER);

            if (ret)
            {
                LOGE("push to queue front err !!!");
                BK_ASSERT(0);
            }

            LOGD("prevent report because not full, op 0x%04x from %d status 0x%x", opcode, from, status);
        }
        else
        {
            buf[status_pos] = msg.ret_status;
            need_rep = 1;
            LOGD("report because full, op 0x%04x status 0x%x", opcode, msg.ret_status);
        }
    }

    rtos_unlock_mutex(&s_multi_controller_mutex);

end:;

    if (need_rep)
    {
        if (s_multi_controller_mutex)
        {
            rtos_lock_mutex(&s_multi_controller_mutex);
            bk_dual_hci_send_to_host(input_buf, input_len);
            rtos_unlock_mutex(&s_multi_controller_mutex);
        }
        else
        {
            bk_dual_hci_send_to_host(input_buf, input_len);
        }
    }

    return 0;
}

static ble_err_t hci_to_secondary_controller(uint8_t *buf, uint16_t len)
{
    int32_t ret = HCI_PARSER_ENCODE_RET_NO_NEED;

    if (hci_parser_get_interface()->do_encode)
    {
        ret = hci_parser_get_interface()->do_encode(buf, len);

        if (HCI_PARSER_ENCODE_RET_NO_NEED == ret)
        {
            if (s_secondary_cb && s_secondary_cb->send)
            {
                LOGV("dir send len %d", len);
                s_secondary_cb->send(buf, len);
            }
        }
        else if (ret == HCI_PARSER_ENCODE_RET_PENDING)
        {
            LOGV("encode len %d", len);
        }
        else if (ret == HCI_PARSER_ENCODE_RET_ERROR)
        {
            LOGE("do encode err");
            ret = -1;
        }
    }
    else
    {
        if (s_secondary_cb && s_secondary_cb->send)
        {
            s_secondary_cb->send(buf, len);
        }
    }

    return ret;
}

static ble_err_t hci_to_secondary_controller_with_type(uint8_t type, uint8_t *intput_buf, uint16_t input_len)
{
    int32_t ret = 0;
    uint8_t *buf = NULL;
    uint16_t len = input_len + 1;

    buf = os_malloc(len);

    if (!buf)
    {
        LOGE("alloc buff err type %d len %d", type, len);
        return -1;
    }

    buf[0] = type;
    os_memcpy(buf + 1, intput_buf, input_len);
    hci_to_secondary_controller(buf, len);
    os_free(buf);
    buf = NULL;

    return ret;
}

#if CONFIG_BT
static bt_err_t dual_hci_data_to_cp_cb(uint8_t type, uint8_t *buf, uint16_t len)
{
    // switch (type)
    // {
    // case DATA_TYPE_COMMAND:
    // {
    //     cmd_hdr_t *cmd_hdr = (cmd_hdr_t *)buf;
    //     bt_ipc_hci_send_cmd(cmd_hdr->opcode, cmd_hdr->param, cmd_hdr->param_len);
    // }
    // break;

    // case DATA_TYPE_ACL:
    // {
    //     acl_hdr_t *acl_hdr = (acl_hdr_t *)buf;
    //     bt_ipc_hci_send_acl_data(acl_hdr->hdl_flags, acl_hdr->param, acl_hdr->datalen);
    // }
    // break;

    // case DATA_TYPE_SCO:
    // {
    //     sco_hdr_t *sco_hdr = (sco_hdr_t *)buf;
    //     bt_ipc_hci_send_sco_data(sco_hdr->conhdl_psf, sco_hdr->param, sco_hdr->datalen);
    // }
    // break;

    // default:
    //     LOGW("unknown type (%d)", type);
    //     break;
    // }

    //buff--;
    int32_t ret = 0;
    //LOGD("type %d len %d %d", type, len);

    switch (type)
    {
    case DATA_TYPE_COMMAND:
    {
        cmd_hdr_t *cmd_hdr = (cmd_hdr_t *)buf;
        uint16_t opcode = cmd_hdr->opcode; //(((uint16_t)buf[2]) << 8 | buf[1]);
        uint8_t dir = hal_hci_arb_cmd_dir(s_controller_mode, opcode);

        LOGV("type %d len %d %d", type, len, cmd_hdr->param_len);

        switch (dir)
        {
        case MULTI_CONTROLLER_DIR_PRI:
            bt_ipc_hci_send_cmd(cmd_hdr->opcode, cmd_hdr->param, cmd_hdr->param_len);
            break;

        case MULTI_CONTROLLER_DIR_SEC:
            ret = hci_to_secondary_controller_with_type(type, buf, len);
            break;

        case MULTI_CONTROLLER_DIR_ALL:
        {
            multi_ct_cmd_t msg = {0};
            msg.opcode = opcode;
            msg.vote = 0;

            ret = rtos_push_to_queue(&s_multi_controller_cmd_queue, &msg, BEKEN_WAIT_FOREVER);

            if (ret)
            {
                LOGE("push to queue err 0x%04x", opcode);
                BK_ASSERT(0);
            }

            bt_ipc_hci_send_cmd(cmd_hdr->opcode, cmd_hdr->param, cmd_hdr->param_len);
            ret = hci_to_secondary_controller_with_type(type, buf, len);
        }
        break;

        case MULTI_CONTROLLER_DIR_NOT_SURE:
            if (opcode == APP_HCI_DISCONNECT_CMD_OPCODE
                    || opcode == APP_HCI_VENDOR_SET_ACL_PRIORITY_CMD_OPCODE
                    || opcode == APP_HCI_RD_FAIL_CONTACT_CNT_CMD_OPCODE
                    || opcode == APP_HCI_RST_FAIL_CONTACT_CNT_CMD_OPCODE
                    || opcode == APP_HCI_RD_LINK_QUAL_CMD_OPCODE
                    || opcode == APP_HCI_RD_RSSI_CMD_OPCODE
                    || opcode == APP_HCI_RD_AFH_CH_MAP_CMD_OPCODE
                    || opcode == APP_HCI_RD_CLK_CMD_OPCODE
                    || opcode == APP_HCI_RD_ENC_KEY_SIZE_CMD_OPCODE)
            {
                uint8_t *tmp_buff = buf - 1;
                uint16_t acl_handle = (((uint16_t)tmp_buff[5]) << 8 | tmp_buff[4]);

                if (s_secondary_cb && s_secondary_cb->send &&
                        acl_handle >= s_secondary_cb->acl_handle_threshold_min && acl_handle <= s_secondary_cb->acl_handle_threshold_max)
                {
                    ret = hci_to_secondary_controller_with_type(type, buf, len);
                }
                else
                {
                    bt_ipc_hci_send_cmd(cmd_hdr->opcode, cmd_hdr->param, cmd_hdr->param_len);
                }
            }
            else
            {
                LOGE("unknow how to send op 0x%x dir %d type %d !!!", opcode, dir, hci_cmd_get_type(opcode));
            }

            break;
        }
    }
    break;

    case DATA_TYPE_ACL:
    {
        acl_hdr_t *acl_hdr = (acl_hdr_t *)buf;

        LOGV("type %d len %d %d", type, len, acl_hdr->datalen);

        switch (s_controller_mode)
        {
        case MULTI_CONTROLLER_MODE_PRI_ALL_SEC_NONE:
            bt_ipc_hci_send_acl_data(acl_hdr->hdl_flags, acl_hdr->param, acl_hdr->datalen);
            break;

        case MULTI_CONTROLLER_MODE_PRI_NONE_SEC_ALL:
            ret = hci_to_secondary_controller_with_type(type, buf, len);
            break;

        case MULTI_CONTROLLER_MODE_PRI_BLE_SEC_BT:
        case MULTI_CONTROLLER_MODE_PRI_BT_SEC_BLE:
        {
            uint8_t *tmp_buff = buf - 1;
            uint16_t acl_handle = (((uint16_t)tmp_buff[2]) << 8 | tmp_buff[1]);

            if (s_secondary_cb && s_secondary_cb->send &&
                    acl_handle >= s_secondary_cb->acl_handle_threshold_min && acl_handle <= s_secondary_cb->acl_handle_threshold_max)
            {
                ret = hci_to_secondary_controller_with_type(type, buf, len);
            }
            else
            {
                bt_ipc_hci_send_acl_data(acl_hdr->hdl_flags, acl_hdr->param, acl_hdr->datalen);
            }
        }
        break;
        }
    }
    break;

    case DATA_TYPE_SCO:
    {
        sco_hdr_t *sco_hdr = (sco_hdr_t *)buf;

        LOGV("type %d len %d %d", type, len, sco_hdr->datalen);

        switch (s_controller_mode)
        {
        case MULTI_CONTROLLER_MODE_PRI_ALL_SEC_NONE:
        case MULTI_CONTROLLER_MODE_PRI_BT_SEC_BLE:
            bt_ipc_hci_send_sco_data(sco_hdr->conhdl_psf, sco_hdr->param, sco_hdr->datalen);
            break;

        case MULTI_CONTROLLER_MODE_PRI_NONE_SEC_ALL:
        case MULTI_CONTROLLER_MODE_PRI_BLE_SEC_BT:
            ret = hci_to_secondary_controller_with_type(type, buf, len);
            break;
        }
    }
    break;

    default:
        LOGE("unknown type (0x%x)", type);
        break;
    }

    if (ret)
    {
        LOGE("send to controller err %d", ret);
    }

    return ret;
}

#else

static ble_err_t ble_hci_data_to_cp_cb(uint8_t *buf, uint16_t len)
{
    uint8_t type = buf[0];

    switch (type)
    {
    case DATA_TYPE_COMMAND:
    {
        cmd_hdr_t *cmd_hdr = (cmd_hdr_t *)(buf + 1);
        bt_ipc_hci_send_cmd(cmd_hdr->opcode, cmd_hdr->param, cmd_hdr->param_len);
    }
    break;

    case DATA_TYPE_ACL:
    {
        acl_hdr_t *acl_hdr = (acl_hdr_t *)(buf + 1);
        bt_ipc_hci_send_acl_data(acl_hdr->hdl_flags, acl_hdr->param, acl_hdr->datalen);
    }
    break;

    default:
        LOGW("unknown type (%d)", type);
        break;
    }

    return 0;
}
#endif

static void hal_hci_driver_send_to_host(uint8_t *buf, uint16_t len)
{
    //LOGD("%s, type %d,len %d",__func__, buf[0],len);
#if CONFIG_BT
    hal_hci_driver_send_to_host_ext(MULTI_CONTROLLER_VOTE_PRI, buf, len);
#else
    bk_ble_hci_send_to_host(buf, len);
#endif
}


static int32_t hci_evt_secondary_report(uint8_t *data, uint32_t len)
{
    switch (s_controller_mode)
    {
    case MULTI_CONTROLLER_MODE_PRI_ALL_SEC_NONE:
        LOGE("can't recv evt in mode MULTI_CONTROLLER_MODE_PRI_ALL_SEC_NONE");
        return 0;
        break;

    case MULTI_CONTROLLER_MODE_PRI_NONE_SEC_ALL:

        break;

    case MULTI_CONTROLLER_MODE_PRI_BLE_SEC_BT:
    case MULTI_CONTROLLER_MODE_PRI_BT_SEC_BLE:
    {

    }
    break;
    }

    hci_parser_get_interface()->do_parse(data, len);

    return 0;
}

// static void notify_parse_packet_ready_cb(uint8_t type, void *data, uint16_t len)
// {

// }

static void notify_parse_packet_ready_ext_cb(void *input_data, uint16_t input_len)
{
    uint8_t *buff = ((uint8_t *)input_data) + 1;
    uint8_t type = ((uint8_t *)input_data)[0];
    //uint16_t len = input_len - 1;

    switch (type)
    {
    case DATA_TYPE_ACL:
    case DATA_TYPE_SCO:
        hal_hci_driver_send_to_host_ext(MULTI_CONTROLLER_VOTE_SEC, input_data, input_len);
        break;

    case DATA_TYPE_EVENT:
    {
        uint8_t evt = buff[0];

        switch (evt)
        {
        case 0x10: //hardware err
        {
            if (buff[2] == 0x8)
            {
                //ignore
                LOGV("ignore hardware err 0x8");
                return;
            }
            else
            {
                LOGE("secondary controller hardware err 0x%x !!!", buff[2]);
                BK_ASSERT_EX(0, "%s secondary controller hardware err 0x%x !!!\n", __func__, buff[2]);
            }
        }
        break;

        case 0x0e:
        {
#if CONFIG_BLUETOOTH_HOST_ENABLE_H5
            uint16_t op = (((uint16_t)buff[4]) << 8 | buff[3]);
            uint8_t status = buff[5];

            if (op == APP_HCI_VENDOR_H5_INIT_CMD_OPCODE)
            {
                if (status)
                {
                    LOGE("recv h5 enable err status 0x%x", status);
                    BK_ASSERT(0);
                    break;
                }
                else
                {
                    if (s_multi_controller_sem)
                    {
                        if (rtos_set_semaphore(&s_multi_controller_sem))
                        {
                            LOGE("set sem err");
                            BK_ASSERT(0);
                            break;
                        }
                    }

                    LOGI("enable vnd h5 ok");
                }

                return;
            }
#endif
        }
        break;
        }

        hal_hci_driver_send_to_host_ext(MULTI_CONTROLLER_VOTE_SEC, input_data, input_len);
    }
    break;

    default:
        LOGE("unknow type 0x%x", type);
        break;
    }
}

static void notify_encode_packet_ready_cb(void *data, uint16_t len)
{
    if (s_secondary_cb && s_secondary_cb->send)
    {
        //LOGD("send len %d", len);
        s_secondary_cb->send(data, len);
    }
}

int hal_hci_driver_secondary_controller_init(bk_bluetooth_secondary_callback_t *cb)
{
    int32_t ret = 0;

    LOGI("start");

    ret = rtos_init_queue(&s_multi_controller_cmd_queue,
                          "multi_controller_cmd_queue",
                          sizeof(multi_ct_cmd_t),
                          20);

    if (ret)
    {
        LOGE("init queue err !!!");
        return -1;
    }

    ret = rtos_init_mutex(&s_multi_controller_mutex);

    if (ret)
    {
        LOGE("init mutex err !!!");
        return -1;
    }

    static const hci_parser_callbacks_t s_hci_parser_cb =
    {
        //.notify_parse_packet_ready_cb = notify_parse_packet_ready_cb,
        .notify_parse_packet_ready_ext_cb = notify_parse_packet_ready_ext_cb,
        .notify_encode_packet_ready_cb = notify_encode_packet_ready_cb,
        // .notify_uart_change_cb = uart_controller_only_uart_change_cb,
        // .notify_uart_reset_cb = uart_controller_only_uart_reset_cb,
        // .read_byte = uart_controller_only_read_byte,
    };

    if (cb->init(hci_evt_secondary_report))
    {
        LOGE("init err !!!");
        return -1;
    }

    ret = hci_parser_get_interface()->init((hci_parser_callbacks_t *)&s_hci_parser_cb);

    if (ret)
    {
        LOGE("hci parser init err");
        return -1;
    }

    s_secondary_cb = cb;

#if CONFIG_BLUETOOTH_HOST_ENABLE_H5

    if (!s_multi_controller_sem)
    {
        ret = rtos_init_semaphore(&s_multi_controller_sem, 1);

        if (ret)
        {
            LOGE("init sem err %d", ret);
            return -1;
        }
    }

    if (s_secondary_cb && s_secondary_cb->send)
    {
        const uint8_t enable_h5[] = {DATA_TYPE_COMMAND, APP_HCI_VENDOR_H5_INIT_CMD_OPCODE & 0xff, (APP_HCI_VENDOR_H5_INIT_CMD_OPCODE >> 8 & 0xff), 1, 1};
        s_secondary_cb->send((uint8_t *)enable_h5, sizeof(enable_h5));
    }

    LOGI("start wait h5 enable");

    ret = rtos_get_semaphore(&s_multi_controller_sem, BEKEN_WAIT_FOREVER);

    if (ret)
    {
        LOGE("wait h5 enable err %d", ret);
        return -1;
    }

    LOGI("wait h5 enable completed");

    if (s_multi_controller_sem)
    {
        rtos_deinit_semaphore(&s_multi_controller_sem);
        s_multi_controller_sem = NULL;
    }

    if (hci_parser_get_interface()->set_h5_enable(1))
    {
        LOGE("enable h5 err");
        return -1;
    }

#endif

    LOGI("end");
    return 0;
}

int hal_hci_driver_secondary_controller_deinit(void)
{
    LOGI("start");
#if CONFIG_BLUETOOTH_HOST_ENABLE_H5

    if (hci_parser_get_interface()->set_h5_enable(0))
    {
        LOGE("disable h5 err");
        return -1;
    }

#endif
    hci_parser_get_interface()->deinit();

    s_secondary_cb->deinit();
    s_secondary_cb = NULL;

    rtos_deinit_queue(&s_multi_controller_cmd_queue);
    s_multi_controller_cmd_queue = NULL;

    rtos_deinit_mutex(&s_multi_controller_mutex);
    s_multi_controller_mutex = NULL;

    LOGI("end");
    return 0;
}

int hal_hci_driver_open(void)
{
    int ret;
#if CONFIG_BT
    ret = bk_dual_host_register_hci_callback(dual_hci_data_to_cp_cb);
#else
    ret = bk_ble_host_register_hci_callback(ble_hci_data_to_cp_cb);
#endif
    bt_ipc_register_hci_send_callback(hal_hci_driver_send_to_host);
    return ret;
}

int hal_hci_driver_close(void)
{
    int ret;

    bt_ipc_register_hci_send_callback(NULL);
#if CONFIG_BT
    ret = bk_dual_host_register_hci_callback(NULL);
#else
    ret = bk_ble_host_register_hci_callback(NULL);
#endif
    return ret;
}
