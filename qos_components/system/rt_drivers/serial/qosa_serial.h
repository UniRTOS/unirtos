/*****************************************************************/ /**
* @file qosa_serial.h
* @brief Serial driver interface for UniRTOS
* @author larson.li@quectel.com
* @date 2023-04-20
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2023-04-20 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#ifndef __SERIAL_H__
#define __SERIAL_H__

#include "qosa_def.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
#include "qosa_sys.h"
#include "qosa_device.h"

#define BAUD_RATE_2400                  2400
#define BAUD_RATE_4800                  4800
#define BAUD_RATE_9600                  9600
#define BAUD_RATE_19200                 19200
#define BAUD_RATE_38400                 38400
#define BAUD_RATE_57600                 57600
#define BAUD_RATE_115200                115200
#define BAUD_RATE_230400                230400
#define BAUD_RATE_460800                460800
#define BAUD_RATE_921600                921600
#define BAUD_RATE_2000000               2000000
#define BAUD_RATE_2500000               2500000
#define BAUD_RATE_3000000               3000000

#define DATA_BITS_5                     5
#define DATA_BITS_6                     6
#define DATA_BITS_7                     7
#define DATA_BITS_8                     8
#define DATA_BITS_9                     9

#define STOP_BITS_1                     0
#define STOP_BITS_2                     1
#define STOP_BITS_3                     2
#define STOP_BITS_4                     3

#define PARITY_NONE                     0
#define PARITY_ODD                      1
#define PARITY_EVEN                     2

#define BIT_ORDER_LSB                   0
#define BIT_ORDER_MSB                   1

#define NRZ_NORMAL                      0 /* Non Return to Zero : normal mode */
#define NRZ_INVERTED                    1 /* Non Return to Zero : inverted mode */

#define RT_DEVICE_FLAG_RX_BLOCKING      0x1000
#define RT_DEVICE_FLAG_RX_NON_BLOCKING  0x2000

#define RT_DEVICE_FLAG_TX_BLOCKING      0x4000
#define RT_DEVICE_FLAG_TX_NON_BLOCKING  0x8000

#define RT_SERIAL_RX_BLOCKING           RT_DEVICE_FLAG_RX_BLOCKING
#define RT_SERIAL_RX_NON_BLOCKING       RT_DEVICE_FLAG_RX_NON_BLOCKING
#define RT_SERIAL_TX_BLOCKING           RT_DEVICE_FLAG_TX_BLOCKING
#define RT_SERIAL_TX_NON_BLOCKING       RT_DEVICE_FLAG_TX_NON_BLOCKING

#define RT_DEVICE_CHECK_OPTMODE         0x20

#define RT_SERIAL_EVENT_RX_IND          0x01 /* Rx indication */
#define RT_SERIAL_EVENT_TX_DONE         0x02 /* Tx complete   */
#define RT_SERIAL_EVENT_RX_DMADONE      0x04 /* Rx DMA transfer done */
#define RT_SERIAL_EVENT_TX_DMADONE      0x08 /* Tx DMA transfer done */
#define RT_SERIAL_EVENT_RX_TIMEOUT      0x10 /* Rx timeout    */
#define RT_SERIAL_EVENT_DTR_INTERRUPT   0x20 /* 2024.09.29 add DTR signal triggered */

#define RT_SERIAL_ERR_OVERRUN           0x01
#define RT_SERIAL_ERR_FRAMING           0x02
#define RT_SERIAL_ERR_PARITY            0x03

#define RT_SERIAL_TX_DATAQUEUE_SIZE     2048
#define RT_SERIAL_TX_DATAQUEUE_LWM      30

#define RT_SERIAL_RX_MINBUFSZ           64
#define RT_SERIAL_TX_MINBUFSZ           64

#define RT_SERIAL_TX_BLOCKING_BUFFER    1
#define RT_SERIAL_TX_BLOCKING_NO_BUFFER 0

#define RT_SERIAL_FLOWCONTROL_CTSRTS    1
#define RT_SERIAL_FLOWCONTROL_NONE      0

/* Default config for serial_configure structure */
#define RT_SERIAL_CONFIG_DEFAULT                                                                                                                               \
    {                                                                                                                                                          \
        BAUD_RATE_115200,               /* 115200 bits/s */                                                                                                    \
            DATA_BITS_8,                /* 8 databits */                                                                                                       \
            STOP_BITS_1,                /* 1 stopbit */                                                                                                        \
            PARITY_NONE,                /* No parity  */                                                                                                       \
            BIT_ORDER_LSB,              /* LSB first sent */                                                                                                   \
            NRZ_NORMAL,                 /* Normal mode */                                                                                                      \
            RT_SERIAL_RX_MINBUFSZ,      /* rxBuf size */                                                                                                       \
            RT_SERIAL_TX_MINBUFSZ,      /* txBuf size */                                                                                                       \
            RT_SERIAL_FLOWCONTROL_NONE, /* Off flowcontrol */                                                                                                  \
            0                                                                                                                                                  \
    }

struct serial_configure
{
    qosa_uint32_t baud_rate;

    qosa_uint32_t data_bits   : 4;
    qosa_uint32_t stop_bits   : 2;
    qosa_uint32_t parity      : 2;
    qosa_uint32_t bit_order   : 1;
    qosa_uint32_t invert      : 1;
    qosa_uint32_t rx_bufsz    : 16;
    qosa_uint32_t tx_bufsz    : 16;
    qosa_uint32_t flowcontrol : 1;
    qosa_uint32_t reserved    : 5;
};

typedef enum
{
    SERIAL_DCD_OFF,
    SERIAL_DCD_ON,
} serial_dcd_e;
typedef struct serial_dcd
{
    qosa_uint32_t dcd_level;
} serial_dcd_t;
//------------------------------------------------------------------------------
//  SIO Escape sequence state.  Used in the detection of '+++' escape sequence.
//------------------------------------------------------------------------------
#define SERIAL_SIO_ESC_CHAR      '+'
//TODO: This event may need to be reduced a bit, reserve 20ms for system scheduling execution at 980ms, need to adjust according to actual situation
#define SERIAL_ESC_INTERVAL_TIME (1000)  //1s
typedef void (*serial_escape_process_cb)(void* argv);
// DTR signal with AT&C
typedef void (*serial_dtr_process_cb)(void* argv);

typedef struct SERIAL_WM_INFO
{
    qosa_buffer_watermark_t* sio_tx_watermark_queue_ptr;
    qosa_buffer_watermark_t* sio_rx_watermark_queue_ptr;
    qosa_bool_t              is_check_esc_flag;
} serial_wm_info_t;

typedef struct SERIAL_ESCAPE_INFO
{
    serial_escape_process_cb cb;
    void*                    argv;
} serial_escape_info_t;

typedef struct SERIAL_DTR_INFO
{
    serial_dtr_process_cb cb;
    void*                 argv;
} serial_dtr_info_t;

typedef enum
{
    SERIAL_ESC_SEQ_NULL,           //  no Escape sequencing in process
    SERIAL_ESC_SEQ_1ST_GUARDTIME,  //  1st Guardtime occurred
    SERIAL_ESC_SEQ_1ST_CHAR,       //  Received 1st delay and 1st char
    SERIAL_ESC_SEQ_2ND_CHAR,       //  Received 2nd delay and 2nd char
    SERIAL_ESC_SEQ_3RD_CHAR,       //  Received 3rd delay and 3rd char
    SERIAL_ESC_SEQ_DETECTED,       //  Escape Sequence detected
    SERIAL_ESC_SEQ_MAX             //  For Bounds checking only
} serial_esc_seq_e_type;

struct rt_serial_device
{
    struct rt_device parent;                             /*!< Corresponding device parent node, current rt_serial_device inherits rt_device attributes*/

    const struct rt_uart_ops* ops;                       /*!< Corresponding platform registered uart operation functions */
    struct serial_configure   config;                    /*!< UART serial port attribute configuration */

    qosa_bool_t              app_rx_high_flag;           /*!< Upper application layer (ftp/file/fota/...) wm high flag */
    qosa_bool_t              tx_in_progress;             /*!< Used to flag whether current written character has been written completely */
    qosa_buffer_watermark_t* sio_tx_watermark_queue_ptr; /*!< Used to save watermark ptr pointer passed in from rt_device */
    qosa_buffer_block_t*     sio_tx_current_wm_item_ptr; /*!< Used to save data that has been taken out from watermark */
    qosa_ptr                 sio_current_tx_ptr;         /*!< Used to save current item's array address */
    qosa_int32_t             sio_bytes_to_tx;            /*!< Used to save remaining data in current item pointer */
    qosa_buffer_watermark_t* sio_rx_watermark_queue_ptr; /*! RX watermark used to save data notified from rx interrupt*/

    /*!< Corresponding RT-Thread driver callback notification function for operations after receiving +++, note this function is called in interrupt or timer */
    serial_escape_process_cb serial_esc_cb;
    void*                    serial_esc_argv;         /*!< Corresponding escape callback func user parameter*/
    serial_esc_seq_e_type    serial_esc_seq_state;    /*!< Corresponding current +++ status */
    qosa_uint8_t             serial_esc_char_num;     /*!< Corresponding current number of +++ received */
    qosa_timer_t             serial_esc_seq_time_prt; /*!< Corresponding +++ timing timer pointer */
    // For serial, it doesn't necessarily need to know the mode status of external calls, only needs to know whether to check +++
    // For upper application layer whether it's APP or AT, serial driver doesn't need to care
    // Only set when upper application layer configures change mode
    qosa_bool_t  is_check_esc_flag; /*!< Whether to open +++ mode check*/
    qosa_uint8_t isr_event_flag;    /*!< Used for interrupt event flag */

    /* serial status monitor && do something application needs */
    qosa_uint32_t is_need_monitor; /*!< Whether to open status reporting*/
    qosa_q_type_t status_report_list;

    /*!<2024.09.21 Corresponding RT-Thread driver callback notification function for operations after receiving DTR */
    serial_dtr_info_t serial_dtr_cb;
};

//------------------------------------------------------------------------------
//  serial_status_monitor
//------------------------------------------------------------------------------
#define MONIOTR_STATUS_CHECK(flag, event) ((1 << event) & flag)
#define MONIOTR_STATUS_SET(flag, event)   (flag |= (1 << event))
#define MONIOTR_STATUS_CLEAR(flag, event) (flag &= ~(1 << event))

typedef void (*serial_status_monitor_cb)(struct rt_serial_device* serial, void* argv);
typedef struct
{
    qosa_q_link_type_t       link;         /**< list node of report */
    qosa_uint32_t            event   : 4;  /**< event of report */
    qosa_uint32_t            circ_en : 1;  /**< report is need loop*/
    qosa_uint32_t            timeout : 16; /**< timeout when not resp -callback*/
    qosa_uint32_t            rsvd    : 11;
    serial_status_monitor_cb callback;     /**< callback for user*/
    union
    {
        int   data;
        void* data_ptr;
    } extras;
} serial_status_monitor_t;

/**
 * uart operators
 */
struct rt_uart_ops
{
    int (*configure)(struct rt_serial_device* serial, struct serial_configure* cfg);

    int (*control)(struct rt_serial_device* serial, int cmd, void* arg);

    int (*read)(struct rt_serial_device* serial, qosa_uint8_t* read_buffer, qosa_uint32_t buffer_len, qosa_uint32_t* bytes_read);

    /*!< When adapting to platform, pay special attention, cannot release buff pointer, must give clear return for how much length is used */
    qosa_size_t (*transmit)(struct rt_serial_device* serial, qosa_uint8_t* buf, qosa_uint32_t size, qosa_uint32_t* bytes_written);
};

void rt_hw_serial_isr(struct rt_serial_device* serial, int event);

int rt_hw_serial_register(struct rt_serial_device* serial, const char* name, qosa_uint32_t flag, void* data);

int rt_hw_serial_event_report_register(struct rt_serial_device* serial, int status, serial_status_monitor_t* arvg);
#endif// !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
#endif /* __SERIAL_H__ */
