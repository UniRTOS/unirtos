/*****************************************************************/ /**
* @file qosa_uart.h
* @brief UART operation interface
* @author bronson.zhan@quectel.com
* @date 2025-04-23
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2025-04-23 <td>1.0 <td>Bronson.Zhan <td> Init
* </table>
**********************************************************************/
#ifndef __QOSA_UART_H__
#define __QOSA_UART_H__

#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_queue_list.h"

/*===========================================================================
 * Macro Definition
 ===========================================================================*/
#define QOSA_UART_ERRCODE_BASE (QOSA_COMPONENT_API_UART << 16)

/**
 * @enum  qosa_uart_error_e
 * @brief uart error code definition.
 */
typedef enum
{
    QOSA_UART_SUCCESS = 0,
    QOSA_UART_EXECUTE_ERR = 1 | QOSA_UART_ERRCODE_BASE, /*!< Serial port execution error */
    QOSA_UART_MEM_ADDR_NULL_ERR,                        /*!< Memory address is null */
    QOSA_UART_INVALID_PARAM_ERR,                        /*!< Invalid parameter */
    QOSA_UART_OPEN_REPEAT_ERR,                          /*!< Serial port repeated open */
    QOSA_UART_NOT_OPEN_ERR,                             /*!< Serial port not open */
} qosa_uart_error_e;

/**
 * @enum  qosa_uart_port_number_e
 * @brief uart port number definition.
 */
typedef enum
{
    QOSA_PORT_NONE = -1,
    QOSA_UART_PORT_0,    /*!< uart0 */
    QOSA_UART_PORT_1,    /*!< uart1 */
    QOSA_UART_PORT_2,    /*!< uart2 */
    QOSA_UART_PORT_3,    /*!< uart3 */
    QOSA_USB_PORT_AT,    /*!< usb at port */
    QOSA_USB_PORT_MODEM, /*!< usb modem port */
    QOSA_USB_PORT_NMEA,  /*!< usb nmea port */
    QOSA_USB_PORT_ACM0,  /*!< usb acm0 port */
    QOSA_PORT_MAX,
} qosa_uart_port_number_e;

/**
 * @enum  qosa_uart_baud_e
 * @brief uart baud definition.
 */
typedef enum
{
    QOSA_UART_BAUD_AUTO = 0, /*!< Automatically detect baud rate */
    QOSA_UART_BAUD_1200 = 1200,
    QOSA_UART_BAUD_2400 = 2400,
    QOSA_UART_BAUD_4800 = 4800,
    QOSA_UART_BAUD_9600 = 9600,
    QOSA_UART_BAUD_10400 = 10400,
    QOSA_UART_BAUD_14400 = 14400,
    QOSA_UART_BAUD_19200 = 19200,
    QOSA_UART_BAUD_28800 = 28800,
    QOSA_UART_BAUD_33600 = 33600,
    QOSA_UART_BAUD_38400 = 38400,
    QOSA_UART_BAUD_57600 = 57600,
    QOSA_UART_BAUD_115200 = 115200,
    QOSA_UART_BAUD_187500 = 187500,
    QOSA_UART_BAUD_230400 = 230400,
    QOSA_UART_BAUD_460800 = 460800,
    QOSA_UART_BAUD_921600 = 921600,
    QOSA_UART_BAUD_1000000 = 1000000,
    QOSA_UART_BAUD_1843200 = 1843200,
    QOSA_UART_BAUD_2000000 = 2000000,
    QOSA_UART_BAUD_2100000 = 2100000,
    QOSA_UART_BAUD_3686400 = 3686400,
    QOSA_UART_BAUD_4000000 = 4000000,
    QOSA_UART_BAUD_4468750 = 4468750
} qosa_uart_baud_e;

/**
 * @enum  qosa_uart_databit_e
 * @brief uart databit definition.
 */
typedef enum
{
    QOSA_UART_DATABIT_7 = 7,
    QOSA_UART_DATABIT_8 = 8,
} qosa_uart_databit_e;

/**
 * @enum  qosa_uart_stopbit_e
 * @brief uart stopbit definition.
 */
typedef enum
{
    QOSA_UART_STOP_1 = 1,
    QOSA_UART_STOP_2 = 2,
} qosa_uart_stopbit_e;

/**
 * @enum  qosa_uart_paritybit_e
 * @brief uart paritybit definition.
 */
typedef enum
{
    QOSA_UART_PARITY_NONE, /*!< No parity */
    QOSA_UART_PARITY_ODD,  /*!< Odd parity */
    QOSA_UART_PARITY_EVEN, /*!< Even parity */
} qosa_uart_paritybit_e;

/**
 * @enum  qosa_uart_flowctrl_e
 * @brief uart flowctrl definition.
 */
typedef enum
{
    QOSA_FC_NONE = 0, /*!< no flow control */
    QOSA_FC_HW,       /*!< hardware flow control */
    QOSA_FC_HW_RTS,   /*!< hardware flow control, RTS */
    QOSA_FC_HW_CTS,   /*!< hardware flow control, CTS */
} qosa_uart_flowctrl_e;

/**
 * @enum  qosa_uart_event_e
 * @brief uart event definition.
 */
typedef enum
{
    QOSA_UART_EVENT_RX_INDICATE = 1 << 0, /*!< RX event reach notification, only notify once, after each user calls read function, will check again if RX still has data, will report RX event again */
    QOSA_UART_EVENT_TX_COMPLETE = 1 << 1, /*!< TX complete event, each time call write interface, after bottom layer write complete, will report TX complete event */
    QOSA_UART_EVENT_TX_LOW = 1 << 2, /*!< TX FIFO below water level event, each time TX FIFO below water level, will report once */
} qosa_uart_event_e;

/**
 * @enum  qosa_uart_mode_e
 * @brief uart transmission mode.
 */
typedef enum
{
    QOSA_UART_MODE_NORMAL = 1,        /*!< Uart mode, used for user to transmit custom data */
    QOSA_UART_MODE_AT = 2,            /*!< AT mode, user cannot call Uart related input/output API for use in this mode */
    QOSA_UART_TX_MODE_CCIO = 3,           /*!< Send by CCIO/OPAQ */
    QOSA_UART_TX_MODE_DRIVER = 4,         /*!< Send by driver polling */
    QOSA_UART_TX_MODE_DRIVER_DMA = 5,     /*!< Send by driver DMA */
} qosa_uart_mode_e;

/**
 * @struct  qosa_uart_config_t
 * @brief uart config definition.
 */
typedef struct
{
    qosa_uart_baud_e      baudrate;   /*!< Baud rate */
    qosa_uart_databit_e   data_bit;   /*!< Data bit */
    qosa_uart_stopbit_e   stop_bit;   /*!< Stop bit */
    qosa_uart_paritybit_e parity_bit; /*!< Parity bit */
    qosa_uart_flowctrl_e  flow_ctrl;  /*!< Flow control */
} qosa_uart_config_t;

typedef struct
{
    qosa_uart_port_number_e port; /*!< Serial port number */
    qosa_uint32_t
          event_id;  /*!< Event set generated by serial port, may carry multiple events each time, corresponding to qosa_uart_event_e, take value by bit */
    void *user_data; /*!< User-defined data pointer, need user management and release */
} qosa_uart_cb_param_t;

/**
 * @brief uart event handling callback function type definition
 *
 * @param[out] qosa_uart_cb_param_t
 *          - Event parameters reported by uart bottom layer, specific description see qosa_uart_cb_param_t definition
 *
 */
typedef void (*qosa_uart_callback)(qosa_uart_cb_param_t *param);

/**
 * @struct  qosa_uart_status_monitor_t
 * @brief uart status monitor definition.
 */
typedef struct
{
    qosa_uint32_t      event_mask; /*!< Event types that need to be monitored, corresponding to the content defined by qosa_uart_event_e structure */
    qosa_uart_callback callback;   /*!< User callback */
    void              *user_data;  /*!< User-defined data pointer, need user management and release */
} qosa_uart_status_monitor_t;

/**
 * @enum  qosa_uart_ioctl_cmd_e
 * @brief uart ioctl cmd definition.
 */
typedef enum
{
    QOSA_UART_IOCTL_NONE,
    QOSA_UART_IOCTL_SET_DCB_CFG,     /*!< Set serial port properties */
    QOSA_UART_IOCTL_GET_DCB_CFG,     /*!< Get serial port properties */
    QOSA_UART_IOCTL_SET_CCIO_MODE,   /*!< Set CCIO mode (only EIGEN platform Uart available) */
    QOSA_UART_IOCTL_RI_GET,          /*!< Get RI status */
    QOSA_UART_IOCTL_RI_SET,          /*!< Set RI status */
    QOSA_UART_IOCTL_DTR_GET,         /*!< Get DTR status */
    QOSA_UART_IOCTL_RTS_SET,         /*!< Set RTS status */
    QOSA_UART_IOCTL_RTS_GET,         /*!< Get RTS status */
    QOSA_UART_IOCTL_CTS_GET,         /*!< Get CTS status */
    QOSA_UART_IOCTL_DCD_GET,         /*!< Get DCD status */
    QOSA_UART_IOCTL_DCD_SET,         /*!< Set DCD status */
    QOSA_UART_IOCTL_CHANGE_BAUDRATE, /*!< Change baud rate */
    QOSA_UART_IOCTL_DROP_RX_CACHE,   /*!< Drop pending RX cache */
    QOSA_UART_IOCTL_RECORD_DTR_FUNC, /*!< Set DTR callback function */
} qosa_uart_ioctl_cmd_e;

/**
 * @brief Register uart event handler
 *
 * @param[in] qosa_uart_port_number_e port
 *          - Serial port selection
 *
 * @param[in] qosa_uart_ioctl_cmd_e cmd
 *          - Corresponding ioctl command
 *
 * @param[in/out] void * arg
 *           - Corresponding ioctl command parameters
 *
 * @return qosa_uart_error_e
 *          - QOSA_UART_SUCCESS: Success
 *          - other: Error code
 */
qosa_uart_error_e qosa_uart_ioctl(qosa_uart_port_number_e port, qosa_uart_ioctl_cmd_e cmd, void *arg);

/**
 * @brief Open UART device
 *
 * @param[in] qosa_uart_port_number_e port
 *          - Serial port selection
 *
 * @return qosa_uart_error_e
 *          - QOSA_UART_SUCCESS: Open successful
 *          - other: Error code
 */
qosa_uart_error_e qosa_uart_open(qosa_uart_port_number_e port);

/**
 * @brief Close UART device
 *
 * @param[in] qosa_uart_port_number_e port
 *          - Serial port selection
 *
 * @return qosa_uart_error_e
 *          - QOSA_UART_SUCCESS: Close successful
 *          - other: Error code
 */
qosa_uart_error_e qosa_uart_close(qosa_uart_port_number_e port);

/**
 * @brief UART write operation
 *
 * @param[in] qosa_uart_port_number_e port
 *          - Serial port selection
 *
 * @param[in] unsigned char *data
 *          - Pointer to data to be written
 *
 * @param[in] unsigned int data_len
 *          - Length of data to be written
 *
 * @return int
 *          - < 0: Error code
 *          - >= 0: Actual byte length written
 */
int qosa_uart_write(qosa_uart_port_number_e port, unsigned char *data, unsigned int data_len);

/**
 * @brief UART read operation
 *
 * This function is used to read data from the specified UART serial port.
 *
 * @param[in] qosa_uart_port_number_e port
 *          - Serial port selection, specify which UART serial port to read data from.
 *
 * @param[out] unsigned char *data
 *          - Read data buffer pointer, used to store data read from UART serial port.
 *
 * @param[in] unsigned int data_len
 *          - Length of data to read, specify how many bytes of data to read from UART serial port at most.
 *
 * @return int
 *          - < 0: Indicates an error occurred during reading, return error code.
 *          - >= 0: Indicates successful data reading length, return actual number of bytes read.
 */
int qosa_uart_read(qosa_uart_port_number_e port, unsigned char *data, unsigned int data_len);

/**
 * @brief Get UART read buffer remaining space size
 *
 * This function is used to query the remaining readable data size in the read buffer of the specified UART serial port.
 *
 * @param[in] qosa_uart_port_number_e port
 *          - Serial port selection, specify the UART serial port to query.
 *
 * @return int
 *          - < 0: Indicates an error occurred during query, return error code.
 *          - >= 0: Indicates remaining data size in read buffer (in bytes).
 */
int qosa_uart_read_available(qosa_uart_port_number_e port);

/**
 * @brief Write remaining get
 *
 * This function is used to query the remaining writable data size in the write buffer of the specified UART serial port.
 *
 * @param[in] qosa_uart_port_number_e port
 *          - Serial port selection, specify the UART serial port to query.
 *
 * @return int
 *          - < 0: Indicates an error occurred during query, return error code.
 *          - >= 0: Indicates remaining space size in write buffer (in bytes).
 */
int qosa_uart_write_available(qosa_uart_port_number_e port);

/**
 * @brief Register UART event callback function
 *
 * This function is used to register a callback function for the specified UART serial port, so that it can be called when UART events occur.
 * If `NULL` is passed as the callback function pointer, it will cancel the previously registered callback function, so that polling method can be used to read data.
 *
 * @param[in] qosa_uart_port_number_e port
 *          - Serial port selection, specify the UART serial port to register callback function for.
 *
 * @param[in] qosa_uart_status_monitor_t *arvg
 *          - Callback function information to be configured
 *
 * @return qosa_uart_error_e
 *          - QOSA_UART_SUCCESS: Indicates registration successful.
 *          - other: Indicates an error occurred during registration, return error code.
 */
qosa_uart_error_e qosa_uart_register_cb(qosa_uart_port_number_e port, qosa_uart_status_monitor_t *arvg);

/**
 * @brief Check if the current module platform supports a certain baud rate
 *
 * @param[in] qosa_uart_port_number_e port
 *          - Serial port selection
 *
 * @param[in] qosa_uint32_t baudrate
 *          - Baud rate to check
 *
 * @return qosa_bool_t
 *          - QOSA_TRUE: Support
 *          - QOSA_FALSE: Not support
 */
qosa_bool_t qosa_uart_check_support_baudrate(qosa_uart_port_number_e port, qosa_uint32_t baudrate);

#endif /* __QOSA_UART_H__ */
