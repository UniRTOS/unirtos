/*****************************************************************/ /**
* @file qosa_at_sio.h
* @brief
* @author larson.li@quectel.com
* @date 2023-05-02
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2023-05-02 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#ifndef __AT_SIO_H__
#define __AT_SIO_H__

#include "qosa_def.h"

#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 

#include "qosa_sys.h"
#include "qosa_watermark.h"
#include "qosa_uart.h"

/**
 * Channel status information for at sio management, different from AT parser and SIO
 */
typedef enum
{
    QOSA_AT_SIO_NOT_SUPPORT = -1, /*!< AT SIO not support */
    QOSA_AT_SIO_NON = 0,
    QOSA_AT_SIO_ATCMD_MODE,    /*!< AT command mode */
    QOSA_AT_SIO_RAW_DATA_MODE, /*!< Data transparent transmission mode */
    QOSA_AT_SIO_ATO_WAIT_MODE, /*!< After +++ exit, ATO can be allowed to switch back, need to call qosa_at_sio_set_port_last_ato_wait_owner to set last dev_port before returning */
    QOSA_AT_SIO_MUX_DATA_MODE, /*!< CMUX data mode (similar to transparent transmission but without detecting +++|DTR)*/
} qosa_at_sio_dev_mode_e;

/**
 * Physical port corresponding to AT parser
 */
typedef enum
{
    QOSA_DEV_NONE = -1,
    QOSA_DEV_SIOLIB_UART1_PORT,
    QOSA_DEV_SIOLIB_UART2_PORT,
    QOSA_DEV_SIOLIB_UART3_PORT,
    QOSA_DEV_SIOLIB_USB_AT_PORT,
    QOSA_DEV_SIOLIB_USB_MODEM,
    QOSA_DEV_SIOLIB_USB_AT2_PORT,
    QOSA_DEV_SIOLIB_USB_NMEA,
    QOSA_DEV_SIOLIB_DATA_MUX_0_PORT,
    QOSA_DEV_SIOLIB_DATA_MUX_1_PORT,
    QOSA_DEV_SIOLIB_DATA_MUX_2_PORT,
    QOSA_DEV_SIOLIB_DATA_MUX_3_PORT,
    QOSA_DEV_SIOLIB_DATA_MUX_4_PORT,
    QOSA_DEV_VIRT_AT_PORT,
    QOSA_DEV_PORT_MAX,
} qosa_at_dev_type_e;

/**
 * For whether each application layer needs to save +++ exit status, used to switch back using ATO
 */
typedef enum
{
    QOSA_PORT_AT_CMD = 0,
    QOSA_PORT_AT_TEST_DEMO,   /*!< Temporary test addition */
    QOSA_PORT_AT_TCPIP_ATCMD, /*!< TCPIP standard AT command exit transparent transmission mode */
} qosa_at_sio_owner_port_e;

/**
 * EVB DCD pin status
 */
typedef enum
{
    QOSA_DCD_HIGH = 0, /*!< EVB DCD status is high */
    QOSA_DCD_LOW,      /*!< EVB DCD status is low */
} qosa_at_sio_dcd_status_e;

/**
 * @struct qosa_at_cmd_mode_switch_t
 * @brief Main information for mode switching
 */
typedef struct
{
    qosa_at_dev_type_e dev_hd;
} qosa_at_cmd_mode_switch_t;

/**
 * @struct qosa_dev_ioctl_cmd_e
 * @brief at ioctl cmd definition.
 */
typedef enum
{
    QOSA_DEV_IOCTL_NONE = 0,
    QOSA_DEV_IOCTL_SET_DCB_CFG,     /*!< Set serial port properties */
    QOSA_DEV_IOCTL_GET_DCB_CFG,     /*!< Get serial port properties */
    QOSA_DEV_IOCTL_RI_SET,          /*!< Set RI status */
    QOSA_DEV_IOCTL_CHANGE_BAUDRATE, /*!< Change baud rate */
    QOSA_DEV_IOCTL_CHECK_BAUDRATE,  /*!< check baud rate */
    QOSA_DEV_IOCTL_SET_CCIO_MODE,   /*!< Set CCIO mode (only EIGEN platform Uart available) */
    QOSA_DEV_IOCTL_MAX
} qosa_dev_ioctl_cmd_e;

/**
 * @brief For registering cb function for automatic switching to transparent transmission mode
 */
typedef qosa_bool_t (*qosa_at_data_modem_switch_cb_func)(qosa_at_dev_type_e dev_port, void* argv);

typedef void (*qosa_at_data_modem_ato_free_cb_func)(void* argv);

/**
 * @struct qosa_at_ato_switch_cb_t
 * @brief ATO command selection switch
 */
typedef struct
{
    qosa_at_data_modem_switch_cb_func   cmd_cb;    /*!< Corresponding function block command transparent transmission recovery call function */
    qosa_at_data_modem_ato_free_cb_func free_cb;   /*!< Resource release request when canceling ATO registration */
    void*                               user_argv; /*!< User parameters carried by user registration */
    qosa_uint32_t                       argc;      /*!< AT channel parameters */
} qosa_at_ato_switch_cb_t;

/**
 * Registration function before AT dev channel enters transparent transmission mode
 */
typedef struct
{
    /*!< Handle +++ arrival event status processing */
    qosa_at_sio_dev_mode_e (*at_escape_sequence_handler)(qosa_at_dev_type_e dev_port, void*);
    /*!< Handle dtr transition status processing*/
    void (*dtr_changed_sig_handler)(qosa_at_dev_type_e dev_port, qosa_bool_t dtr_status, void*);
    void* user_argv;
} qosa_at_sio_func_tbl_t;

/**
 * @struct qosa_dev_ioctl_type
 * @brief at ioctl configuration kinetic energy
 */
typedef struct
{
    qosa_uart_config_t uart_cfg;  /*!< UART basic configuration parameters */
    qosa_bool_t        level;     /*!< Level configuration parameters */
    qosa_int32_t       baudrate;  /*!< Baud rate setting value */
    qosa_uart_mode_e   ccio_mode; /*!< UART working mode configuration */
} qosa_dev_ioctl_type;

/**
 * @brief  Get the current status mode of the corresponding AT dev channel, mainly used to get whether it is ATO WAIT mode
 *
 * @param dev  -- AT dev channel to get
 * @return qosa_at_sio_dev_mode_e -- Return the current status mode of the dev channel
 */
qosa_at_sio_dev_mode_e qosa_at_sio_get_at_dev_mode(qosa_at_dev_type_e dev_port);

/**
 * @brief For setting whether the specified AT dev channel stays in transparent transmission mode for switching using ATO
 *
 * @param qosa_at_ato_switch_cb_t *func_tbl
 *      - Registered ato recovery cb status
 *
 * @param qosa_at_dev_type_e dev_port
 *      - AT dev channel to stay
 *
 * @note  Special attention this function context must be operated under AT task
 */
void qosa_at_sio_set_port_last_ato_wait_owner(qosa_at_ato_switch_cb_t* func_tbl, qosa_at_dev_type_e dev_port);

/**
 * @brief For getting the owner information of the specified AT dev channel
 *
 * @param dev_port -- AT dev channel to get
 * @return qosa_at_ato_switch_cb_t -- Return current dev ATO stay information
 */
qosa_at_ato_switch_cb_t* qosa_at_sio_get_port_last_ato_wait_owner(qosa_at_dev_type_e dev_port);

/**
 * @brief For user to clear the registered ATO recovery status resources
 *
 * @param qosa_at_ato_switch_cb_t* func_tbl
 *      - Handle information to be cleared when unregistering
 *
 * @param qosa_at_dev_type_e dev_port
 *      - AT dev channel when registering
 *
 * @note  Special attention this function context must be operated under AT task
 */
void qosa_at_sio_clean_last_ato_owner(qosa_at_ato_switch_cb_t* func_tbl, qosa_at_dev_type_e dev_port);

/**
 * @brief Before switching to RAW mode, need to set registration callback function, so that when underlying event notification occurs
 *        can find the upper layer corresponding dev channel operation interface
 *
 * @param qosa_at_sio_func_tbl_t * func_tbl - Corresponding callback function to be registered,
 * @param qosa_at_dev_type_e * dev - Corresponding AT command channel
 *
 * @return qosa_bool_t - QOSA_TRUE function execution successful
 *                      QOSA_FALSE function execution failed
 * @note This function call context must be AT task context, otherwise trigger abort
 */
qosa_bool_t qosa_at_sio_register_callback_func(qosa_at_sio_func_tbl_t* func_tbl, qosa_at_dev_type_e dev_port);

/**
 * @brief Switch the current AT dev channel to RAW mode, or revert to ATCMD mode,
 *
 * @param uart_mode -- Corresponding DEV current channel mode RAW MODE or ATCMD MODE
 * @param rx_wm_ptr -- If RAW_MODE, it is the rx watermark pointer to be switched, if ATCMD_MODE can be empty
 *                  Function will default to switch to the original AT channel watermark pointer
 * @param tx_wm_ptr -- If RAW_MODE, it is the tx watermark pointer to be switched, if ATCMD_MODE can be empty
 *                  Function will default to switch to the original AT channel watermark pointer
 * @param dev -- Corresponding physical channel to be switched by the upper layer
 * @return qosa_bool_t -- QOSA_TRUE function call configuration successful
 *                       QOSA_FALSE function call configuration failed
 *
 * @note 1. If preparing to switch to RAW MODE, need to call qosa_at_sio_register_callback_func
 *       function to set the corresponding callback function before calling this function
 *       2. This function call context must be AT task context, otherwise trigger abort
 */
qosa_bool_t qosa_at_sio_ex_change_mode(
    qosa_at_sio_dev_mode_e   uart_mode,
    qosa_buffer_watermark_t* rx_wm_ptr,
    qosa_buffer_watermark_t* tx_wm_ptr,
    qosa_at_dev_type_e       dev_port
);

/**
 * @brief For actively notifying data mode (transparent transmission mode/edit mode) to exit to AT command mode
 *
 * @param[in] qosa_at_dev_type_e * dev
 *          - dev channel corresponding to status switching
 *
 */
void qosa_at_sio_report_data_mode_switch_atcmd(qosa_at_dev_type_e dev_port);

/**
 * @brief Get tx watermark remaining space size
 *
 * @param[in] qosa_at_dev_type_e * dev
 *            AT dev channel to get
 *
 * @return qosa_uint32_t
 *         Return the length of remaining space
 *
 */
qosa_uint32_t qosa_at_sio_get_tx_remaining_buff(qosa_at_dev_type_e dev_port);

/**
 * @brief Get the device name string corresponding to the dev
 *
 * @param[in] qosa_at_dev_type_e * dev
 *            dev channel to operate
 *
 * @return char*
 *         Return the actual corresponding channel string
 *
 */
char* qosa_at_sio_get_dev_name(qosa_at_dev_type_e dev_port);

/**
 * @brief User AT command writes data to the corresponding dev channel, which will use the uint8 type data of AT result
 *        buffer_block save.
 *
 * @param[in] qosa_at_dev_type_e * dev - Corresponding AT command channel
 * @param[in] qosa_uint8_t * resp - String information to be written
 * @param[in] qosa_uint16_t len - Length of string information to be written
 */
void qosa_at_sio_dev_write(qosa_at_dev_type_e dev_port, qosa_uint8_t* resp, qosa_uint16_t len);

/**
 * @brief User AT command writes data to the corresponding dev channel, directly sending item data
 *
 * @param[in] qosa_at_dev_type_e dev_port - Corresponding AT command channel
 * @param[in] qosa_buffer_block_t* data_ptr - Item address to be written
 * @return int
 *         Return the actual data length sent by serial port
 */
int qosa_at_sio_dev_transmit(qosa_at_dev_type_e dev_port, qosa_buffer_block_t* data_ptr);

/**
 * @brief Determine whether AT channel is executing AT
 *
 * @param[in] qosa_at_dev_type_e dev_port
 *          - AT device port
 * @return Return whether the corresponding AT channel has AT executing, QOSA_FALSE: no AT executing, QOSA_TRUE: AT executing.
 */
qosa_bool_t qosa_at_is_parsing(qosa_at_dev_type_e dev_port);

/**
 * @brief Set the status of EVB DCD pin, the high and low status of DCD pin will change according to AT&C and whether it is data mode
 *
 * @param[in] dev_port Corresponding AT command channel
 * @param[in] status DCD status
 */
void qosa_at_sio_set_dcd_status(qosa_at_dev_type_e dev_port, qosa_at_sio_dcd_status_e status);

/**
 * @brief Get tx buf remaining size
 *
 * @param[in] dev_port Corresponding AT command channel
 * @note Difference from remaining_buff, remaining_buff gets the remaining size of sio, some platforms are not fully connected to sio, so this interface is added
 */
qosa_uint32_t qosa_at_get_tx_avail_size(qosa_at_dev_type_e dev_port);

/**
 * @brief at ioctl configuration
 *
 * @param[in] dev_port Corresponding AT command channel
 * @param[in] cmd Corresponding ioctl command
 * @param[in/out] type Corresponding ioctl command parameters
 */
qosa_bool_t qosa_at_dev_port_ioctl(qosa_at_dev_type_e dev_port, qosa_dev_ioctl_cmd_e cmd, qosa_dev_ioctl_type* type);

/**
 * @brief  Used to send virtual AT instructions
 *
 * @param  data      Virtual AT instruction string
 * @param  data_len  Virtual AT instruction string length
 *
 * @return Returns 0 on success, other error codes on failure.
 *
 */
int qosa_virt_at_send(char* at_cmd, int at_cmd_len);

// Define the type of callback function
typedef void (*wm_item_report_cb_t)(void*);

/**
 * @brief Register watermark callback function
 * @param cb Callback function pointer
 * @param p_arg Callback function argument
 * @return OSA_TRUE Success, OSA_FALSE Failure
 *
 * This function is used to register a callback function to the daemon task:
 * 1. Create a new callback node
 * 2. Check if the current item count has been restored
 * 3. If restored, call the callback directly and return
 * 4. Otherwise, add the callback to the queue and start the daemon task
 */
qosa_bool_t qosa_register_watermark_callback(wm_item_report_cb_t cb, void* p_arg);
#endif//!(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif /* __AT_SIO_H__ */
