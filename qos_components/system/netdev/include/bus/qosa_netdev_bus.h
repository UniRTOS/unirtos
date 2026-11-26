/*****************************************************************/ /**
 * @file qosa_netdev_bus.h
 * @brief Public bus configuration and control API for netdev bus backends.
 * @author felix.lin@quectel.com
 * @date 2026-07-03
 *
 * @copyright Copyright (c) 2026 Quectel Wireless Solution, Co., Ltd.
 * All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
 *
 * @par EDIT HISTORY FOR MODULE
 * <table>
 * <tr><th>Date <th>Version <th>Author <th>Description
 * <tr><td>2026-07-03 <td>1.0 <td>felix.lin <td> Add and normalize English Doxygen comments
 * </table>
 **********************************************************************/

#ifndef __QOSA_NETDEV_BUS_H__
#define __QOSA_NETDEV_BUS_H__

#include "qosa_netdev.h"
#ifdef CONFIG_QOSA_NETDEV_BUS_GPIO_FUNC
#include "qosa_gpio.h"
#include "qosa_pinctrl.h"
#endif
#ifdef CONFIG_QOSA_NETDEV_BUS_UART_FUNC
#include "qosa_uart.h"
#endif
#ifdef CONFIG_QOSA_NETDEV_BUS_SPI_FUNC
#include "qosa_dev.h"   /* WDT definition */
#include "qosa_spi.h"
#endif

/**
 * @enum qosa_netdev_bus_idx_e
 * @brief Netdev bus slot index.
 */
typedef enum netdev_bus_idx
{
    QOSA_NETDEV_BUS_IDX_0 = 0, /*!< Bus slot 0. */
    QOSA_NETDEV_BUS_IDX_1,     /*!< Bus slot 1. */
    QOSA_NETDEV_BUS_IDX_2,     /*!< Bus slot 2. */
    QOSA_NETDEV_BUS_IDX_3,     /*!< Bus slot 3. */
    QOSA_NETDEV_BUS_IDX_MAX_NUM, /*!< Number of bus slots. */
} qosa_netdev_bus_idx_e;

/**
 * @enum qosa_netdev_bus_type_e
 * @brief Supported netdev bus backend types.
 */
typedef enum netdev_bus_type
{
    QOSA_NETDEV_BUS_TYPE_GPIO = 0, /*!< GPIO bit-bang backend. */
    QOSA_NETDEV_BUS_TYPE_GPIO_INT, /*!< GPIO backend with interrupt support. */
    QOSA_NETDEV_BUS_TYPE_SPI,      /*!< SPI controller backend. */
    QOSA_NETDEV_BUS_TYPE_UART,     /*!< UART backend. */
} qosa_netdev_bus_type_e;

#ifdef CONFIG_QOSA_NETDEV_BUS_GPIO_FUNC
/**
 * @struct qosa_netdev_bus_gpio_t
 * @brief GPIO backend configuration.
 */
typedef struct
{
    struct
    {
        /*!< Interrupt configuration. */
        qosa_int_cfg_t i;
        /*!< Interrupt trigger type. */
        qosa_gpio_trigger_e t;
        /*!< Pinmux for the interrupt GPIO pin. Kept outside pin.p so GPIO_INT callers fill only irq. */
        qosa_uint8_t pin_num;
        /*!< Alternate function for the interrupt GPIO pin. */
        qosa_uint8_t gpio_func;
    } irq;
    struct
    {
        /*!< Generic GPIO pin configuration. */
        qosa_pin_cfg_t p;
    } pin;
} qosa_netdev_bus_gpio_t;
#endif /* CONFIG_QOSA_NETDEV_BUS_GPIO_FUNC */


#ifdef CONFIG_QOSA_NETDEV_BUS_SPI_FUNC
/**
 * @struct qosa_netdev_bus_spi_t
 * @brief SPI backend pinmux and controller configuration.
 */
typedef struct
{
    /*!< MOSI pin number. */
    qosa_uint8_t mosi_pin_num;
    /*!< MOSI pin function. */
    qosa_uint8_t mosi_func;
    /*!< MISO pin number. */
    qosa_uint8_t miso_pin_num;
    /*!< MISO pin function. */
    qosa_uint8_t miso_func;
    /*!< Clock pin number. */
    qosa_uint8_t clk_pin_num;
    /*!< Clock pin function. */
    qosa_uint8_t clk_func;
    /*!< Chip-select pin number. */
    qosa_uint8_t cs_pin_num;
    /*!< Chip-select pin function. */
    qosa_uint8_t cs_func;
    /*!< Optional software-controlled chip-select GPIO number. */
    qosa_uint8_t cs_gpio_num;
    struct
    {
        /*!< SPI port selection. */
        qosa_spi_port_e p;
        /*!< SPI clock setting. */
        qosa_spi_clk_e c;
        /*!< SPI transfer mode. */
        qosa_spi_transmit_mode_e t;
        /*!< SPI mode. */
        qosa_spi_mode_e m;
        /*!< SPI frame format. */
        qosa_spi_frame_format_e f;
        /*!< SPI NSS mode. */
        qosa_spi_nss_mode_e n;
        /*!< SPI bit order. */
        qosa_spi_bit_order_e b;
        /*!< SPI data width. */
        qosa_spi_data_width_e w;
    } cfg;
} qosa_netdev_bus_spi_t;
#endif /* CONFIG_QOSA_NETDEV_BUS_SPI_FUNC */

#ifdef CONFIG_QOSA_NETDEV_BUS_UART_FUNC
/**
 * @struct qosa_netdev_bus_uart_t
 * @brief UART backend pinmux and controller configuration.
 */
typedef struct
{
    /*!< TX pin number. */
    qosa_uint8_t tx_pin_num;
    /*!< TX pin function. */
    qosa_uint8_t tx_func;
    /*!< RX pin number. */
    qosa_uint8_t rx_pin_num;
    /*!< RX pin function. */
    qosa_uint8_t rx_func;
    /*!< CTS pin number. */
    qosa_uint8_t ctx_pin_num;
    /*!< CTS pin function. */
    qosa_uint8_t ctx_func;
    /*!< RTS pin number. */
    qosa_uint8_t rts_pin_num;
    /*!< RTS pin function. */
    qosa_uint8_t rts_func;
    struct
    {
        /*!< UART port selection. */
        qosa_uart_port_number_e p;
        /*!< UART line configuration. */
        qosa_uart_config_t c;
        /*!< UART status monitor configuration. */
        qosa_uart_status_monitor_t s;
    } cfg;
} qosa_netdev_bus_uart_t;
#endif /* CONFIG_QOSA_NETDEV_BUS_UART_FUNC */

/**
 * @enum qosa_netdev_bus_append_op_e
 * @brief Auxiliary operations supported by qosa_netdev_bus_append().
 */
typedef enum
{
    QOSA_NETDEV_BUS_APPEND_OP_NONE = 0,       /*!< No append operation. */
    QOSA_NETDEV_BUS_APPEND_OP_IOCTL,          /*!< Generic IOCTL request. */
    QOSA_NETDEV_BUS_APPEND_OP_SPI_SOFT_CS_SET, /*!< Set software SPI chip-select level. */
} qosa_netdev_bus_append_op_e;

/**
 * @struct qosa_netdev_bus_ioctl_t
 * @brief Generic bus IOCTL request payload.
 */
typedef struct
{
    /*!< IOCTL command code. */
    qosa_uint32_t cmd;
    /*!< IOCTL argument pointer. */
    void *arg;
} qosa_netdev_bus_ioctl_t;

/**
 * @struct qosa_netdev_bus_append_t
 * @brief Append-operation payload container.
 */
typedef struct
{
    /*!< Append operation selector. */
    qosa_netdev_bus_append_op_e op;
    union
    {
        /*!< IOCTL payload used by QOSA_NETDEV_BUS_APPEND_OP_IOCTL. */
        qosa_netdev_bus_ioctl_t ioctl;
        /*!< GPIO level used by QOSA_NETDEV_BUS_APPEND_OP_SPI_SOFT_CS_SET. */
        qosa_gpio_level_e gpio_level;
    } u;
} qosa_netdev_bus_append_t;

/**
 * @brief Register a bus backend in a slot.
 *
 * @param[in] qosa_netdev_bus_idx_e bus_idx
 *          - Bus slot index.
 * @param[in] qosa_netdev_bus_type_e bus_type
 *          - Bus backend type.
 * @param[in] void *bus
 *          - Pointer to the backend configuration structure.
 *
 * @return Returns qosa_netdev_err_e status code.
 */
qosa_netdev_err_e qosa_netdev_bus_register(qosa_netdev_bus_idx_e bus_idx, qosa_netdev_bus_type_e bus_type, void *bus);

/**
 * @brief Unregister a bus backend from a slot.
 *
 * @param[in] qosa_netdev_bus_idx_e bus_idx
 *          - Bus slot index.
 *
 * @return Returns qosa_netdev_err_e status code.
 */
qosa_netdev_err_e qosa_netdev_bus_unregister(qosa_netdev_bus_idx_e bus_idx);

/**
 * @brief Write data through a registered bus backend.
 *
 * @param[in] qosa_netdev_bus_idx_e bus_idx
 *          - Bus slot index.
 * @param[in] qosa_uint8_t *data
 *          - Pointer to the transmit buffer.
 * @param[in] qosa_uint32_t len
 *          - Number of bytes to write.
 *
 * @return Returns qosa_netdev_err_e status code.
 */
qosa_netdev_err_e qosa_netdev_bus_write(qosa_netdev_bus_idx_e bus_idx, qosa_uint8_t *data, qosa_uint32_t len);

/**
 * @brief Query whether a bus slot is currently registered.
 *
 * @param[in] bus_idx Bus slot index.
 *
 * @return QOSA_TRUE if the slot is registered, QOSA_FALSE otherwise.
 */
qosa_bool_t qosa_netdev_bus_is_registered(qosa_netdev_bus_idx_e bus_idx);

/**
 * @brief Read data through a registered bus backend.
 *
 * @param[in] qosa_netdev_bus_idx_e bus_idx
 *          - Bus slot index.
 * @param[out] qosa_uint8_t *data
 *          - Pointer to the receive buffer.
 * @param[in] qosa_uint32_t len
 *          - Number of bytes to read.
 *
 * @return Returns qosa_netdev_err_e status code.
 */
qosa_netdev_err_e qosa_netdev_bus_read(qosa_netdev_bus_idx_e bus_idx, qosa_uint8_t *data, qosa_uint32_t len);

/**
 * @brief Perform a full-duplex transfer through a registered bus backend.
 *
 * @param[in] qosa_netdev_bus_idx_e bus_idx
 *          - Bus slot index.
 * @param[in] qosa_uint8_t *indata
 *          - Pointer to the transmit buffer.
 * @param[out] qosa_uint8_t *outdata
 *          - Pointer to the receive buffer.
 * @param[in] qosa_uint32_t len
 *          - Number of bytes to transfer.
 *
 * @return Returns qosa_netdev_err_e status code.
 */
qosa_netdev_err_e qosa_netdev_bus_write_read(qosa_netdev_bus_idx_e bus_idx, qosa_uint8_t *indata, qosa_uint8_t *outdata, qosa_uint32_t len);

/**
 * @brief Execute an append operation on a registered bus backend.
 *
 * @param[in] qosa_netdev_bus_idx_e bus_idx
 *          - Bus slot index.
 * @param[in] qosa_netdev_bus_append_t *append
 *          - Pointer to the append request.
 *
 * @return Returns qosa_netdev_err_e status code.
 */
qosa_netdev_err_e qosa_netdev_bus_append(qosa_netdev_bus_idx_e bus_idx, qosa_netdev_bus_append_t *append);

#endif /* __QOSA_NETDEV_BUS_H__ */
