#include "hci_parse.h"

#include "hci_distinguish.h"
#include "hal_hci_internal.h"

#include "os/os.h"
#include "os/mem.h"
#include <stdint.h>
#include <string.h>
#include "gpio_driver.h"
#include "driver/gpio.h"
#include "components/bluetooth/bk_dm_bluetooth_types.h"
#include "components/bluetooth/bk_dm_bluetooth.h"
#include "driver/uart.h"

#define LOG_TAG "hal_hci_p"
#define LOG_LEVEL LOG_LEVEL_INFO

#define HCI_DEBUG_UART_OUTPUT 0
#define HCI_DEBUG_LOG_OUTPUT 0

#if HCI_DEBUG_UART_OUTPUT
    #define HCI_DEBUG_UART_ID 1
#endif

#define HCI_RECV_IN_ISR 1

#define HCI_PACKET_PARSER_MSG_COUNT          (50 * 16)

#define BUFFER_USE_ALLOC 0
#define H4_READ_BUFF_SIZE 1050
#define HCI_PACKET_PARSER_ISR_BUFF_SIZE (1024 * 4)
#define HCI_PACKET_PARSER_ISR_INDEX_PLUS(x) (((x) + 1) % (HCI_PACKET_PARSER_ISR_BUFF_SIZE))

#define HCI_PACKET_PARSER_ISR_INDEX_PLUS_EXT(x, size) (((x) + (size)) % (HCI_PACKET_PARSER_ISR_BUFF_SIZE))

enum
{
    H4_PACKET_IDLE,
    H4_PACKET_TYPE,
    H4_PACKET_HEADER,
    H4_PACKET_CONTENT,
    H4_PACKET_END
};

// 2 bytes for opcode, 1 byte for parameter length (Volume 2, Part E, 5.4.1)
#define COMMAND_PREAMBLE_SIZE 3
// 2 bytes for handle, 2 bytes for data length (Volume 2, Part E, 5.4.2)
#define ACL_PREAMBLE_SIZE 4
// 2 bytes for handle, 1 byte for data length (Volume 2, Part E, 5.4.3)
#define SCO_PREAMBLE_SIZE 3
// 1 byte for event code, 1 byte for parameter length (Volume 2, Part E, 5.4.4)
#define EVENT_PREAMBLE_SIZE 2

#define EVENT_DATA_LENGTH_INDEX 2
#define COMMON_DATA_LENGTH_INDEX 3
#define HCI_PACKET_TYPE_TO_INDEX(type) ((type) - 1)

static const uint8_t hci_preamble_sizes[] =
{
    COMMAND_PREAMBLE_SIZE,
    ACL_PREAMBLE_SIZE,
    SCO_PREAMBLE_SIZE,
    EVENT_PREAMBLE_SIZE
};

typedef struct
{
    uint32_t type;
    uint32_t data_len;
    void *data;
} hci_packet_parser_msg_t;

enum
{
    HCI_PACKET_PARSER_MSG_NULL,
    HCI_PACKET_PARSER_IN_DATA_READY_MSG,
    HCI_PACKET_PARSER_OUT_DATA_READY_MSG,
    HCI_PACKET_PARSER_IN_H5_DATA_READY_MSG,
    HCI_PACKET_PARSER_H5_TX_ACK_TOUT_MSG,
    HCI_PACKET_PARSER_CLEAN_ALL_STATUS_MSG,
    HCI_PACKET_PARSER_EXIT_MSG
};

static beken_queue_t hci_packet_parser_msg_que = NULL;
static uint8_t hci_packet_parser_msg_que_ready = 0;
static beken_thread_t hci_parser_thread_handle = NULL;


#if BUFFER_USE_ALLOC
static uint8_t *s_hci_packet_parser_isr_buff = NULL;
static uint8_t *h4_read_buffer = NULL;
#else
static uint8_t s_hci_packet_parser_isr_buff[HCI_PACKET_PARSER_ISR_BUFF_SIZE] = {0};
static uint8_t h4_read_buffer[H4_READ_BUFF_SIZE];
#endif

volatile static uint32_t s_hci_packet_parser_isr_buff_wt = 0;
volatile static uint32_t s_hci_packet_parser_isr_buff_rd = 0;

static uint8_t packet_recv_state = H4_PACKET_IDLE;
static uint32_t packet_bytes_need = 0;
static serial_data_type_t current_type = 0;
static uint32_t h4_read_length = 0;
static const hci_parser_callbacks_t *hci_packet_parser_cb = NULL;
#if CONFIG_BLUETOOTH_HOST_ENABLE_H5
    static beken_semaphore_t s_hci_parse_sem = NULL;
#endif

#if HCI_DEBUG_UART_OUTPUT
    static beken_semaphore_t s_hci_uart_log_sem = NULL;
#endif

#if HCI_RECV_IN_ISR
#else
    static beken_mutex_t s_packet_recv_mutex = NULL;
#endif

static uint8_t s_hci_h5_enable;

static int32_t hci_packet_parser_read(uint8_t *buff, uint32_t len)
{
    uint32_t index = 0;

#if HCI_RECV_IN_ISR
#else
    rtos_lock_mutex(&s_packet_recv_mutex);
#endif

    while (index < len)
    {
        if (s_hci_packet_parser_isr_buff_rd == s_hci_packet_parser_isr_buff_wt)
        {
            goto READ_END;
        }

        buff[index] = s_hci_packet_parser_isr_buff[s_hci_packet_parser_isr_buff_rd];
        index++;
        s_hci_packet_parser_isr_buff_rd = HCI_PACKET_PARSER_ISR_INDEX_PLUS(s_hci_packet_parser_isr_buff_rd);
    }

READ_END:
#if HCI_RECV_IN_ISR
#else
    rtos_unlock_mutex(&s_packet_recv_mutex);
#endif
    return index;
}

static void hci_packet_parser_handler(void)
{
    uint8_t type = 0;
    int32_t bytes_read = 0;

    do
    {
        switch (packet_recv_state)
        {
        case H4_PACKET_IDLE:
            packet_bytes_need = 1;

            do
            {
                bytes_read = hci_packet_parser_read(&type, 1);

                if (bytes_read == -1)
                {
                    LOGE("state = %d\n", packet_recv_state);
                    return;
                }

                if (!bytes_read )//&& packet_bytes_need)
                {
                    LOGD("state = %d, bytes_read 0", packet_recv_state);
                    return;
                }

                if (type < DATA_TYPE_COMMAND || type > DATA_TYPE_EVENT)
                {
                    LOGE("1 invalid data type: 0x%x", type);
#if HCI_DEBUG_LOG_OUTPUT || HCI_DEBUG_UART_OUTPUT
                    //GPIO_UP(55);
                    uint32_t suffix_len = 0;

                    while (hci_packet_parser_read(&type, 1) == 1)
                    {
                        suffix_len++;
                        LOGE("dump suffix data 0x%x", type);
                    }

                    //GPIO_DOWN(55);
                    LOGE("suffix len %d", suffix_len);
                    rtos_delay_milliseconds(100);
#endif
                    BK_ASSERT_EX(0, "%s 1 invalid data type: 0x%x\n", __func__, type);
                    //goto end;
                    continue;
                }
                else
                {
                    //LOGD("read byte 0x%02X", type);
                    packet_bytes_need -= bytes_read;
                    packet_recv_state = H4_PACKET_TYPE;
                    current_type = type;
                    h4_read_buffer[0] = type;
                }
            }
            while (packet_bytes_need);

        case H4_PACKET_TYPE:
            packet_bytes_need = hci_preamble_sizes[HCI_PACKET_TYPE_TO_INDEX(current_type)];
            h4_read_length = 0;
            packet_recv_state = H4_PACKET_HEADER;

        case H4_PACKET_HEADER:
            do
            {
                bytes_read = hci_packet_parser_read(&h4_read_buffer[h4_read_length + 1], packet_bytes_need);

                if (bytes_read == -1)
                {
                    LOGE("state = %d\n", packet_recv_state);
                    return;
                }

                if (!bytes_read )//&& packet_bytes_need)
                {
                    LOGD("state = %d, bytes_read 0, type : %d", packet_recv_state, current_type);
                    return;
                }

                packet_bytes_need -= bytes_read;
                h4_read_length += bytes_read;

                if (h4_read_length > H4_READ_BUFF_SIZE)
                {
                    BK_ASSERT_EX(0, "%s h4_read_length %d > H4_READ_BUFF_SIZE\n", __func__, h4_read_length);
                }
            }
            while (packet_bytes_need);

            packet_recv_state = H4_PACKET_CONTENT;

            if (current_type == DATA_TYPE_ACL)
            {
                os_memcpy(&packet_bytes_need, &h4_read_buffer[COMMON_DATA_LENGTH_INDEX], 2);

                if (packet_bytes_need > 1024)
                {
                    BK_ASSERT_EX(0, "%s packet_bytes_need %d > 1024\n", __func__, packet_bytes_need);
                }
            }
            else if (current_type == DATA_TYPE_EVENT)
            {
                packet_bytes_need = h4_read_buffer[EVENT_DATA_LENGTH_INDEX];
            }
            else if (current_type == DATA_TYPE_SCO)
            {
                packet_bytes_need = h4_read_buffer[3];
            }
            else
            {
                packet_bytes_need = h4_read_buffer[COMMON_DATA_LENGTH_INDEX];
            }

        case H4_PACKET_CONTENT:
            while (packet_bytes_need)
            {
                bytes_read = hci_packet_parser_read(&h4_read_buffer[h4_read_length + 1], packet_bytes_need);

                if (bytes_read == -1)
                {
                    LOGE("state = %d", packet_recv_state);
                    return;
                }

                if (!bytes_read)
                {
                    LOGD("state = %d, bytes_read 0", packet_recv_state);
                    return;
                }

                packet_bytes_need -= bytes_read;
                h4_read_length += bytes_read;

                if (h4_read_length > H4_READ_BUFF_SIZE)
                {
                    BK_ASSERT_EX(0, "%s h4_read_length %d > H4_READ_BUFF_SIZE\n", __func__, h4_read_length);
                }
            }

            packet_recv_state = H4_PACKET_END;

        case H4_PACKET_END:
            switch (current_type)
            {
            case DATA_TYPE_COMMAND:
            case DATA_TYPE_ACL:
            case DATA_TYPE_SCO:
            case DATA_TYPE_EVENT:
                if (hci_packet_parser_cb)
                {
#if HCI_DEBUG_LOG_OUTPUT
#define MAX_BYTES 20
                    uint8_t log_buff[MAX_BYTES + 5] = {0};
                    uint32_t index = 0;
                    uint32_t j = 0;

                    LOGI("send to host:");

                    while (j < h4_read_length + 1)
                    {
                        os_memset(log_buff, 0, MAX_BYTES);
                        index = 0;

                        for (uint32_t i = 0; i < MAX_BYTES && j < h4_read_length + 1; i++, j++)
                        {
                            index += sprintf((char *)(log_buff + index), "%02x", h4_read_buffer[j]);
                        }

                        if (index)
                        {
                            BK_LOGI("hci", "%s\n", log_buff);
                        }
                    }

#endif

#if HCI_DEBUG_UART_OUTPUT

                    if (1)//current_type != DATA_TYPE_SCO)
                    {
                        current_type |= 0b10000000;
                        rtos_lock_mutex(&s_hci_uart_log_sem);

                        bk_uart_write_bytes(HCI_DEBUG_UART_ID, &current_type, 1);
                        bk_uart_write_bytes(HCI_DEBUG_UART_ID, h4_read_buffer + 1, h4_read_length);

                        rtos_unlock_mutex(&s_hci_uart_log_sem);
                    }

#endif

                    if (hci_packet_parser_cb->notify_parse_packet_ready_ext_cb)
                    {
                        hci_packet_parser_cb->notify_parse_packet_ready_ext_cb(h4_read_buffer, h4_read_length + 1);
                    }
                    else if (hci_packet_parser_cb->notify_parse_packet_ready_cb)
                    {
                        hci_packet_parser_cb->notify_parse_packet_ready_cb(h4_read_buffer[0], h4_read_buffer + 1, h4_read_length);
                    }
                }

                break;

            default:
                LOGE("2 invalid data type: 0x%x", current_type);
                BK_ASSERT_EX(0, "%s 2 invalid data type: 0x%x\n", __func__, current_type);
                break;
            }

            break;

        default:

            break;
        }

        packet_recv_state = H4_PACKET_IDLE;
        packet_bytes_need = 0;
        current_type = 0;
        h4_read_length = 0;

    }
    while (1);
}

static int32_t hci_packet_parser_send_msg(uint32_t type, uint8_t *data, uint32_t len)
{
    hci_packet_parser_msg_t msg = {0};
    int32_t ret = 0;
    static uint32_t fail_count = 0;

    if (hci_packet_parser_msg_que && hci_packet_parser_msg_que_ready)
    {
        msg.type = type;
        msg.data_len = len;

        if (data && len)
        {
            msg.data = os_malloc(len);

            if (!msg.data)
            {
                LOGE("alloc err, type %d len %d", type, len);
                return -1;
            }

            os_memcpy(msg.data, data, len);
        }

        ret = rtos_push_to_queue(&hci_packet_parser_msg_que, &msg, BEKEN_NO_WAIT);
    }

    if (ret)
    {
        LOGD("ret err %d, type %d %d", ret, type, len);

        if(msg.data_len && msg.data)
        {
            os_free(msg.data);
            msg.data = NULL;
        }

        if(HCI_PACKET_PARSER_IN_H5_DATA_READY_MSG != type && HCI_PACKET_PARSER_IN_DATA_READY_MSG != type)
        {
            LOGE("push queue fail for important type %d fail_count %d", type, fail_count);
            BK_ASSERT_EX(0, "%s push queue fail for important type %d fail_count %d\n", __func__, type, fail_count);
        }

        if(fail_count++ >= 50)
        {
            LOGE("push queue fail too much %d current type %d", fail_count, type);
            //BK_ASSERT_EX(0, "%s push queue fail too much %d\n", __func__, fail_count);
        }
    }
    else
    {
        fail_count = 0;
    }

    return ret;
}

static void hci_packet_parser_main(void *arg)
{
    while (1)
    {
        int32_t err = 0;
        hci_packet_parser_msg_t msg;

        err = rtos_pop_from_queue(&hci_packet_parser_msg_que, &msg, BEKEN_WAIT_FOREVER);

        if (0 == err)
        {
            switch (msg.type)
            {
            case HCI_PACKET_PARSER_IN_DATA_READY_MSG:
            {
                hci_packet_parser_handler();
            }
            break;
#if CONFIG_BLUETOOTH_HOST_ENABLE_H5

            case HCI_PACKET_PARSER_OUT_DATA_READY_MSG: // encode data to h5
            {
                uint8_t *data = (typeof(data))msg.data;

                if ((err = bk_bluetooth_h5_send(data + 1, data[0], msg.data_len - 1)) < 0)
                {
                    LOGE("h5 send err");
                }

                if (msg.data)
                {
                    os_free(msg.data);
                    msg.data = NULL;
                }
            }
            break;

            case HCI_PACKET_PARSER_IN_H5_DATA_READY_MSG: // recv h5 data
            {
                uint8_t tmp[128] = {0};
                uint32_t len = 0;

                do
                {
                    len = hci_packet_parser_read(tmp, sizeof(tmp));

                    if (len)
                    {
                        bk_bluetooth_h5_recv_data(tmp, len);
                    }
                }
                while (len);
            }
            break;

            case HCI_PACKET_PARSER_H5_TX_ACK_TOUT_MSG: // h5 tx to
                bk_bluetooth_h5_check_retran();
                break;
#endif

            case HCI_PACKET_PARSER_CLEAN_ALL_STATUS_MSG:
            case HCI_PACKET_PARSER_EXIT_MSG:
                LOGI("recv msg %d", msg.type);

                if (msg.type == HCI_PACKET_PARSER_EXIT_MSG)
                {
                    hci_packet_parser_msg_que_ready = 0;
                }

                LOGI("disable h5");
                bk_bluetooth_h5_set_h5_enable(0);
                LOGI("clean queue");

                while ((err = rtos_pop_from_queue(&hci_packet_parser_msg_que, &msg, 0)) == 0)
                {
                    if (msg.data)
                    {
                        os_free(msg.data);
                        msg.data = NULL;
                    }
                }

                s_hci_h5_enable = 0;

                LOGI("clean all status done");

                if (msg.type == HCI_PACKET_PARSER_CLEAN_ALL_STATUS_MSG)
                {
                    // if (hci_packet_parser_cb->notify_uart_reset_cb)
                    // {
                    //     hci_packet_parser_cb->notify_uart_reset_cb();
                    // }
                }

                if (msg.type == HCI_PACKET_PARSER_EXIT_MSG)
                {
                    goto end;
                }

                break;

            default:
                break;
            }
        }
    }

end:;
    LOGW("thread end");
    //_deinit_queue_wrapper(&hci_packet_parser_msg_que);
    //hci_packet_parser_msg_que = NULL;
    //hci_parser_thread_handle = NULL;
    rtos_delete_thread(NULL);
}

static int32_t hci_parser_task_init(void)
{
    int32_t ret = 0;

    if ((!hci_parser_thread_handle) && (!hci_packet_parser_msg_que))
    {
        ret = rtos_init_queue(&hci_packet_parser_msg_que,
                              "hci_packet_parser_msg_que",
                              sizeof(hci_packet_parser_msg_t),
                              HCI_PACKET_PARSER_MSG_COUNT);

        if (ret != 0)
        {
            LOGE("hci packet parser msg queue failed");
            return -1;
        }

        hci_packet_parser_msg_que_ready = 1;

        ret = rtos_create_thread(&hci_parser_thread_handle,
                                 1,
                                 "hci_packet_parser",
                                 (void *)hci_packet_parser_main,
                                 2096,
                                 (void *)0);

        if (ret != 0)
        {
            LOGE("hci packet parser task init fail");
            rtos_deinit_queue(&hci_packet_parser_msg_que);
            hci_packet_parser_msg_que = NULL;
            hci_parser_thread_handle = NULL;
        }

        return 0;
    }
    else
    {
        return -1;
    }
}

static int32_t hci_packet_do_parse(uint8_t *data, uint32_t len)
{
    int ret = 0;

#if HCI_RECV_IN_ISR
#else
    rtos_lock_mutex(&s_packet_recv_mutex);
#endif

    for (uint32_t i = 0; i < len; i++)
    {
        if (HCI_PACKET_PARSER_ISR_INDEX_PLUS(s_hci_packet_parser_isr_buff_wt) == s_hci_packet_parser_isr_buff_rd)
        {
            LOGE("wt == rd, drop data !!!");
#if CONFIG_BLUETOOTH_HOST_ENABLE_H5
#else
            BK_ASSERT(0);
#endif
            hci_packet_parser_send_msg(s_hci_h5_enable ? HCI_PACKET_PARSER_IN_H5_DATA_READY_MSG : HCI_PACKET_PARSER_IN_DATA_READY_MSG, NULL, 0);
            goto end;
        }

        s_hci_packet_parser_isr_buff[s_hci_packet_parser_isr_buff_wt] = data[i];
        s_hci_packet_parser_isr_buff_wt = HCI_PACKET_PARSER_ISR_INDEX_PLUS(s_hci_packet_parser_isr_buff_wt);
    }

    hci_packet_parser_send_msg(s_hci_h5_enable ? HCI_PACKET_PARSER_IN_H5_DATA_READY_MSG : HCI_PACKET_PARSER_IN_DATA_READY_MSG, NULL, 0);

end:;
#if HCI_RECV_IN_ISR
#else
    rtos_unlock_mutex(&s_packet_recv_mutex);
#endif

    (void)ret;
    return 0;
}

static int32_t hci_packet_do_encode(uint8_t *data, uint32_t len)
{
    int32_t ret = HCI_PARSER_ENCODE_RET_NO_NEED;

    uint16_t opcode = 0;
    uint8_t status = 0;

    // if (data[0] == DATA_TYPE_EVENT && data[1] == HCI_CMD_CMP_EVT_CODE && data[2] >= 4)
    // {
    //     opcode = ((data[5] << 8) | data[4]);
    //     status = data[6];
    // }

    // if (opcode == HCI_VS_BEKEN_SOFT_REBOOT_RESP_NOW_CMD_OPCODE && !status)
    // {
    //     //clean all status

    //     ret = hci_packet_parser_send_msg(HCI_PACKET_PARSER_CLEAN_ALL_STATUS_MSG, data, len);

    //     if (ret)
    //     {
    //         LOGE("send HCI_PACKET_PARSER_CLEAN_ALL_STATUS_MSG err !!!");
    //     }

    //     return HCI_PARSER_ENCODE_RET_PENDING;
    // }

    if (!s_hci_h5_enable)
    {
        LOGD("no h5 len %d", len);

#if HCI_DEBUG_UART_OUTPUT

        if (1)//data[0] != DATA_TYPE_SCO)
        {
            rtos_lock_mutex(&s_hci_uart_log_sem);
            bk_uart_write_bytes(HCI_DEBUG_UART_ID, data, len);
            rtos_unlock_mutex(&s_hci_uart_log_sem);
        }

#endif
        // switch (opcode)
        // {
        // case HCI_VS_BEKEN_H5_INIT_CMD_OPCODE:
        // {
        //     if (!status)
        //     {
        //         LOGI("h5 init OK");
        //         s_hci_h5_enable = 1;
        //         h5l_set_h5_enable(1);
        //     }
        // }
        // break;

        // case HCI_VS_BEKEN_SET_CONTROLLER_BAUD_CMD_OPCODE:
        // {
        //     if (!status)
        //     {
        //         uint32_t final_baud = 0;
        //         hci_baud_2_platform_baud(data[7], &final_baud);

        //         if (hci_packet_parser_cb->notify_encode_packet_ready_cb)
        //         {
        //             hci_packet_parser_cb->notify_encode_packet_ready_cb(data, len);
        //         }

        //         LOGI("start covert baud to final baud %d", final_baud);
        //         _bt_wrapper(_delay_milliseconds, 20);

        //         if (hci_packet_parser_cb->notify_uart_change_cb)
        //         {
        //             hci_packet_parser_cb->notify_uart_change_cb(final_baud);
        //         }
        //     }

        //     ret = HCI_PARSER_ENCODE_RET_PENDING;
        // }
        // break;

        // default:
        //     break;
        // }
    }
    else
    {
        ret = hci_packet_parser_send_msg(HCI_PACKET_PARSER_OUT_DATA_READY_MSG, data, len);

        if (ret == 0)
        {
            ret = HCI_PARSER_ENCODE_RET_PENDING;
        }
        else
        {
            LOGE("send packet msg err");
            ret = HCI_PARSER_ENCODE_RET_ERROR;
        }
    }

    (void)status;
    (void)opcode;
    return ret;
}

#if CONFIG_BLUETOOTH_HOST_ENABLE_H5
// static void h5_decode_data_cb(uint8_t type, void *data, uint16_t len)
// {
//     if (hci_packet_parser_cb)
//     {
//         if (hci_packet_parser_cb->notify_parse_packet_ready_ext_cb)
//         {
//             hci_packet_parser_cb->notify_parse_packet_ready_ext_cb(type, data, len);
//         }
//         else if (hci_packet_parser_cb->notify_parse_packet_ready_cb)
//         {
//             hci_packet_parser_cb->notify_parse_packet_ready_cb(data, len);
//         }
//     }
// }

static void h5_decode_data_ext_cb(void *data, uint16_t len)
{
    uint8_t *buff = data;

    if (hci_packet_parser_cb)
    {
        if (hci_packet_parser_cb->notify_parse_packet_ready_ext_cb)
        {
            hci_packet_parser_cb->notify_parse_packet_ready_ext_cb(data, len);
        }
        else if (hci_packet_parser_cb->notify_parse_packet_ready_cb)
        {
            hci_packet_parser_cb->notify_parse_packet_ready_cb(buff[0], buff + 1, len - 1);
        }
    }
}

static int32_t h5_encode_data_cb(void *data, uint16_t len)
{
    if (hci_packet_parser_cb->notify_encode_packet_ready_cb)
    {
        hci_packet_parser_cb->notify_encode_packet_ready_cb(data, len);
    }

    return 0;
}

static void h5_notify_tx_ack_tout_cb(void)
{
    hci_packet_parser_msg_t msg = {0};

    LOGD("");

    if (hci_packet_parser_msg_que)
    {
        msg.type = HCI_PACKET_PARSER_H5_TX_ACK_TOUT_MSG;
        rtos_push_to_queue(&hci_packet_parser_msg_que, &msg, BEKEN_NO_WAIT);
    }
}

static void h5_nego_completed_cb(void)
{
    LOGI("");

    if (s_hci_parse_sem)
    {
        if (rtos_set_semaphore(&s_hci_parse_sem))
        {
            LOGE("set sem err");
            BK_ASSERT(0);
        }
    }
}
#endif

static int32_t set_h5_enable(uint8_t enable)
{
    int32_t ret = 0;
#if CONFIG_BLUETOOTH_HOST_ENABLE_H5

    if (enable && s_hci_h5_enable)
    {
        LOGE("already enable");
        ret = -1;
        goto end;
    }

    if (!enable && !s_hci_h5_enable)
    {
        LOGE("already deinit");
        ret = -1;
        goto end;
    }

    if (enable)
    {
        ret = rtos_init_semaphore(&s_hci_parse_sem, 1);

        if (ret)
        {
            LOGE("init sem err");
            ret = -1;
            goto end;
        }

        bk_bluetooth_h5_set_h5_enable(1);
        s_hci_h5_enable = 1;
        bk_bluetooth_h5_send_sync_req(6, 0, 1);

        LOGI("start wait h5 sync/config");

        ret = rtos_get_semaphore(&s_hci_parse_sem, BEKEN_WAIT_FOREVER);

        if (ret)
        {
            LOGE("wait sem err %d", ret);
            BK_ASSERT(0);
            ret = -1;
            goto end;
        }

        LOGI("wait h5 sync/config enable completed");
    }
    else
    {
        s_hci_h5_enable = 0;
        bk_bluetooth_h5_set_h5_enable(0);
    }

end:;

    if (s_hci_parse_sem)
    {
        rtos_deinit_mutex(&s_hci_parse_sem);
        s_hci_parse_sem = NULL;
    }

#else
    LOGW("h5 not enable");
#endif
    return ret;
}

static uint8_t get_h5_enable(void)
{
#if CONFIG_BLUETOOTH_HOST_ENABLE_H5
    return s_hci_h5_enable;
#else
    LOGW("h5 not enable");
    return 0;
#endif
}

#if HCI_DEBUG_UART_OUTPUT
static int32_t uart_debug_enable(uint8_t uart_id, uint8_t enable, uint32_t band)
{
    bk_err_t ret = 0;

    typedef struct
    {
        uart_id_t uart_id;
        uart_config_t uart_config;
        uint32_t init_baud;
        uint32_t nego_baud;
        gpio_id_t reset_pin;
    } bsc_config_t;

    bsc_config_t s_bsc_config =
    {
        .uart_config =
        {
            .baud_rate = UART_BAUDRATE_115200,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_NONE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_FLOWCTRL_DISABLE,
            .src_clk = UART_SCLK_XTAL_26M,
            .enable_sw_flow_ctrl = 0,
        },
    };

    s_bsc_config.uart_id = uart_id;

    //LOGI("uart id %d %s baud %d", uart_id, enable ? "enable" : "disable", band);

    if (enable)
    {
        switch (band)
        {
        case 115200:
            s_bsc_config.uart_config.baud_rate = UART_BAUDRATE_115200;
            break;

        case 921600:
            s_bsc_config.uart_config.baud_rate = UART_BAUDRATE_921600;
            break;

        case 1000000:
            s_bsc_config.uart_config.baud_rate = 1000000;
            break;

        case 3250000:
            s_bsc_config.uart_config.baud_rate = UART_BAUDRATE_3250000;
            break;

        case 2000000:
        default:
            s_bsc_config.uart_config.baud_rate = UART_BAUDRATE_2000000;
            break;
        }

        switch (uart_id)
        {
        case UART_ID_0:
            gpio_dev_unmap(GPIO_10);
            gpio_dev_unmap(GPIO_11);
            gpio_dev_map(GPIO_10, GPIO_DEV_UART0_RXD);
            gpio_dev_map(GPIO_11, GPIO_DEV_UART0_TXD);

            // gpio_dev_unmap(GPIO_12);
            // gpio_dev_unmap(GPIO_13);
            //s_bsc_config.uart_config.flow_ctrl = UART_FLOWCTRL_CTS_RTS;
            break;

        case UART_ID_1:
            gpio_dev_unmap(GPIO_0);
            gpio_dev_unmap(GPIO_1);
            gpio_dev_map(GPIO_0, GPIO_DEV_UART1_TXD);
            gpio_dev_map(GPIO_1, GPIO_DEV_UART1_RXD);
            break;

        case UART_ID_2:
            gpio_dev_unmap(GPIO_30);
            gpio_dev_unmap(GPIO_31);
            gpio_dev_map(GPIO_30, GPIO_DEV_UART2_RXD);
            gpio_dev_map(GPIO_31, GPIO_DEV_UART2_TXD);
            // gpio_dev_unmap(GPIO_40);
            // gpio_dev_unmap(GPIO_41);
            // gpio_dev_map(GPIO_40, GPIO_DEV_UART2_RXD);
            // gpio_dev_map(GPIO_41, GPIO_DEV_UART2_TXD);
            break;

        default:
            LOGE("uart_id err %d", uart_id);
            return -1;
            break;
        }

        //LOGD("before init uart %d", uart_id);
        ret = bk_uart_init(uart_id, &s_bsc_config.uart_config);
        //LOGD("after init uart %d", uart_id);

        if (ret != 0)
        {
            LOGE("bk_uart_init err, ret %d", ret);
            return ret;
        }

        //LOGI("ble uart %d enable", uart_id);
    }
    else
    {
        bk_uart_deinit(uart_id);
        //LOGI("ble uart %d disable", uart_id);
    }

    return ret;
}
#endif

static int32_t hci_packet_parser_init(const hci_parser_callbacks_t *parser_callbacks)
{
    int32_t ret = 0;

    hci_packet_parser_cb = parser_callbacks;
    packet_recv_state = H4_PACKET_IDLE;

    s_hci_packet_parser_isr_buff_wt = 0;
    s_hci_packet_parser_isr_buff_rd = 0;

#if HCI_DEBUG_UART_OUTPUT
    uart_debug_enable(HCI_DEBUG_UART_ID, 1, 2000000);

    if (rtos_init_mutex(&s_hci_uart_log_sem))
    {
        LOGE("init s_hci_uart_log_sem err");
    }

#endif

#if HCI_RECV_IN_ISR
#else

    if (rtos_init_mutex(&s_packet_recv_mutex))
    {
        LOGE("init s_packet_recv_mutex err");
    }

#endif

#if BUFFER_USE_ALLOC
    h4_read_buffer = (uint8_t *)_malloc_wrapper(H4_READ_BUFF_SIZE);
    BT_ASSERT_ERR(h4_read_buffer);

    s_hci_packet_parser_isr_buff = (uint8_t *)_malloc_wrapper(HCI_PACKET_PARSER_ISR_BUFF_SIZE);
    BT_ASSERT_ERR(s_hci_packet_parser_isr_buff);
#endif

    ret = hci_parser_task_init();
#if CONFIG_BLUETOOTH_HOST_ENABLE_H5
    static const bluetooth_h5_cb_t s_h5_cb =
    {
        //.decode_data_cb = h5_decode_data_cb,
        .decode_data_ext_cb = h5_decode_data_ext_cb,
        .encode_data_cb = h5_encode_data_cb,
        .notify_tx_ack_tout_cb = h5_notify_tx_ack_tout_cb,
        .notify_h5_nego_completed = h5_nego_completed_cb,
    };

    bk_bluetooth_h5_init((bluetooth_h5_cb_t *)&s_h5_cb);
#endif

#if 0
    gpio_dev_unmap(55);
    GPIO_DOWN(55);
#endif
    return ret;
}

static int32_t hci_packet_parser_deinit(void)
{
    if (hci_parser_thread_handle)
    {
        hci_packet_parser_send_msg(HCI_PACKET_PARSER_EXIT_MSG, NULL, 0);
        //todo: wait thread end
        LOGI("wait task end");
        rtos_thread_join(&hci_parser_thread_handle);
        hci_parser_thread_handle = NULL;
        LOGI("task end");
        rtos_deinit_queue(&hci_packet_parser_msg_que);
        hci_packet_parser_msg_que = NULL;
        hci_packet_parser_cb = NULL;
    }

#if CONFIG_BLUETOOTH_HOST_ENABLE_H5
    bk_bluetooth_h5_set_h5_enable(0);
    bk_bluetooth_h5_deinit();
    s_hci_h5_enable = 0;
#endif

#if BUFFER_USE_ALLOC

    if (h4_read_buffer)
    {
        os_free(h4_read_buffer);
        h4_read_buffer = NULL;
    }

    if (s_hci_packet_parser_isr_buff)
    {
        os_free(s_hci_packet_parser_isr_buff);
        s_hci_packet_parser_isr_buff = NULL;
    }

#endif

#if HCI_DEBUG_UART_OUTPUT
    uart_debug_enable(HCI_DEBUG_UART_ID, 0, 2000000);

    if (s_hci_uart_log_sem)
    {
        rtos_deinit_semaphore(&s_hci_uart_log_sem);
        s_hci_uart_log_sem = NULL;
    }

#endif

#if HCI_RECV_IN_ISR
#else

    if (s_packet_recv_mutex)
    {
        rtos_deinit_mutex(&s_packet_recv_mutex);
        s_packet_recv_mutex = NULL;
    }

#endif

    return 0;
}

static const hci_parser_t s_interface =
{
    .init = hci_packet_parser_init,
    .deinit = hci_packet_parser_deinit,
    .do_parse = hci_packet_do_parse,
    .do_encode = hci_packet_do_encode,
    .set_h5_enable = set_h5_enable,
    .get_h5_enable = get_h5_enable,
};

const hci_parser_t *hci_parser_get_interface(void)
{
    return &s_interface;
}
