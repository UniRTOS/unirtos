/*****************************************************************/ /**
 * @file qosa_netdev_bus_internal.h
 * @brief Internal bus context and backend declarations for netdev bus management.
 * @author felix.lin@quectel.com
 * @date 2026-07-03
 *
 * @copyright Copyright (c) 2026 Quectel Wireless Solution, Co., Ltd.
 * All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
 *
 * @par EDIT HISTORY FOR MODULE
 * <table>
 * <tr><th>Date <th>Version <th>Author <th>Description
 * <tr><td>2026-07-03 <td>1.0 <td>felix.lin <td> Add English Doxygen comments
 * </table>
 **********************************************************************/

#ifndef __QOSA_NETDEV_BUS_INTERNAL_H__
#define __QOSA_NETDEV_BUS_INTERNAL_H__

#include "qosa_netdev_bus.h"
#include "qosa_netdev_internal.h"

/**
 * @struct qosa_netdev_bus_t
 * @brief Internal netdev bus slot state.
 */
typedef struct
{
    /*!< Whether the slot is registered. */
    qosa_bool_t registered;
    /*!< Registered bus backend type. */
    qosa_netdev_bus_type_e bus_type;
    union
    {
#ifdef CONFIG_QOSA_NETDEV_BUS_GPIO_FUNC
        /*!< GPIO backend configuration. */
        qosa_netdev_bus_gpio_t gpio;
#endif /* CONFIG_QOSA_NETDEV_BUS_GPIO_FUNC */
#ifdef CONFIG_QOSA_NETDEV_BUS_SPI_FUNC
        /*!< SPI backend configuration. */
        qosa_netdev_bus_spi_t spi;
#endif /* CONFIG_QOSA_NETDEV_BUS_SPI_FUNC */
#ifdef CONFIG_QOSA_NETDEV_BUS_UART_FUNC
        /*!< UART backend configuration. */
        qosa_netdev_bus_uart_t uart;
#endif /* CONFIG_QOSA_NETDEV_BUS_UART_FUNC */
    } u;
} qosa_netdev_bus_t;

/* Keep the SPI entry declared in every build. The wrapper source always
 * exports the same symbols, but SPI registration still depends on whether
 * the internal slot layout includes SPI config storage. */

/**
 * @brief Return the internal bus slot context for an index.
 *
 * @param[in] qosa_netdev_bus_idx_e bus_idx
 *          - Bus slot index.
 *
 * @return Returns the bus slot context pointer, or NULL when the index is invalid.
 */
qosa_netdev_bus_t *qosa_netdev_bus_get(qosa_netdev_bus_idx_e bus_idx);

/**
 * @brief Store bus backend configuration into a slot.
 *
 * @param[in] qosa_netdev_bus_idx_e bus_idx
 *          - Bus slot index.
 * @param[in] qosa_netdev_bus_type_e bus_type
 *          - Bus backend type.
 * @param[in] void *bus
 *          - Pointer to the backend configuration to copy.
 *
 * @return Returns qosa_netdev_err_e status code.
 */
qosa_netdev_err_e qosa_netdev_bus_set(qosa_netdev_bus_idx_e bus_idx, qosa_netdev_bus_type_e bus_type, void *bus);

/**
 * @brief Clear the internal bus slot state.
 *
 * @param[in] qosa_netdev_bus_idx_e bus_idx
 *          - Bus slot index.
 *
 * @return Returns qosa_netdev_err_e status code.
 */
qosa_netdev_err_e qosa_netdev_bus_reset(qosa_netdev_bus_idx_e bus_idx);

/**
 * @brief Initialize the registered bus backend for a slot.
 *
 * @param[in] qosa_netdev_bus_idx_e bus_idx
 *          - Bus slot index.
 *
 * @return Returns qosa_netdev_err_e status code.
 */
qosa_netdev_err_e qosa_netdev_bus_init(qosa_netdev_bus_idx_e bus_idx);

/**
 * @brief Deinitialize the registered bus backend for a slot.
 *
 * @param[in] qosa_netdev_bus_idx_e bus_idx
 *          - Bus slot index.
 *
 * @return Returns qosa_netdev_err_e status code.
 */
qosa_netdev_err_e qosa_netdev_bus_deinit(qosa_netdev_bus_idx_e bus_idx);

#ifdef CONFIG_QOSA_NETDEV_BUS_UART_FUNC
/**
 * @brief Initialize the UART bus backend.
 *
 * @param[in] qosa_netdev_bus_idx_e bus_idx
 *          - Bus slot index.
 *
 * @return Returns qosa_netdev_err_e status code.
 */
qosa_netdev_err_e qosa_netdev_bus_uart_init(qosa_netdev_bus_idx_e bus_idx);

/**
 * @brief Deinitialize the UART bus backend.
 *
 * @param[in] qosa_netdev_bus_idx_e bus_idx
 *          - Bus slot index.
 *
 * @return Returns qosa_netdev_err_e status code.
 */
qosa_netdev_err_e qosa_netdev_bus_uart_deinit(qosa_netdev_bus_idx_e bus_idx);

/**
 * @brief Write data through the UART bus backend.
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
qosa_netdev_err_e qosa_netdev_bus_uart_write(qosa_netdev_bus_idx_e bus_idx, qosa_uint8_t *data, qosa_uint32_t len);

/**
 * @brief Read data through the UART bus backend.
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
qosa_netdev_err_e qosa_netdev_bus_uart_read(qosa_netdev_bus_idx_e bus_idx, qosa_uint8_t *data, qosa_uint32_t len);

/**
 * @brief Perform a full-duplex transfer through the UART bus backend.
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
qosa_netdev_err_e qosa_netdev_bus_uart_write_read(qosa_netdev_bus_idx_e bus_idx, qosa_uint8_t *indata, qosa_uint8_t *outdata, qosa_uint32_t len);

/**
 * @brief Apply an append operation through the UART bus backend.
 *
 * @param[in] qosa_netdev_bus_idx_e bus_idx
 *          - Bus slot index.
 * @param[in] qosa_netdev_bus_append_t *append
 *          - Pointer to the append request.
 *
 * @return Returns qosa_netdev_err_e status code.
 */
qosa_netdev_err_e qosa_netdev_bus_uart_append(qosa_netdev_bus_idx_e bus_idx, qosa_netdev_bus_append_t *append);

#endif /* CONFIG_QOSA_NETDEV_BUS_UART_FUNC */

#ifdef CONFIG_QOSA_NETDEV_BUS_SPI_FUNC
/**
 * @brief Initialize the SPI bus backend.
 *
 * @param[in] qosa_netdev_bus_idx_e bus_idx
 *          - Bus slot index.
 *
 * @return Returns qosa_netdev_err_e status code.
 */
qosa_netdev_err_e qosa_netdev_bus_spi_init(qosa_netdev_bus_idx_e bus_idx);

/**
 * @brief Deinitialize the SPI bus backend.
 *
 * @param[in] qosa_netdev_bus_idx_e bus_idx
 *          - Bus slot index.
 *
 * @return Returns qosa_netdev_err_e status code.
 */
qosa_netdev_err_e qosa_netdev_bus_spi_deinit(qosa_netdev_bus_idx_e bus_idx);

/**
 * @brief Write data through the SPI bus backend.
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
qosa_netdev_err_e qosa_netdev_bus_spi_write(qosa_netdev_bus_idx_e bus_idx, qosa_uint8_t *data, qosa_uint32_t len);

/**
 * @brief Read data through the SPI bus backend.
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
qosa_netdev_err_e qosa_netdev_bus_spi_read(qosa_netdev_bus_idx_e bus_idx, qosa_uint8_t *data, qosa_uint32_t len);

/**
 * @brief Perform a full-duplex transfer through the SPI bus backend.
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
qosa_netdev_err_e qosa_netdev_bus_spi_write_read(qosa_netdev_bus_idx_e bus_idx, qosa_uint8_t *indata, qosa_uint8_t *outdata, qosa_uint32_t len);

/**
 * @brief Apply an append operation through the SPI bus backend.
 *
 * @param[in] qosa_netdev_bus_idx_e bus_idx
 *          - Bus slot index.
 * @param[in] qosa_netdev_bus_append_t *append
 *          - Pointer to the append request.
 *
 * @return Returns qosa_netdev_err_e status code.
 */
qosa_netdev_err_e qosa_netdev_bus_spi_append(qosa_netdev_bus_idx_e bus_idx, qosa_netdev_bus_append_t *append);
#endif /* CONFIG_QOSA_NETDEV_BUS_SPI_FUNC */

#ifdef CONFIG_QOSA_NETDEV_BUS_GPIO_FUNC
/**
 * @brief Initialize the GPIO bus backend.
 *
 * @param[in] qosa_netdev_bus_idx_e bus_idx
 *          - Bus slot index.
 *
 * @return Returns qosa_netdev_err_e status code.
 */
qosa_netdev_err_e qosa_netdev_bus_gpio_init(qosa_netdev_bus_idx_e bus_idx);

/**
 * @brief Deinitialize the GPIO bus backend.
 *
 * @param[in] qosa_netdev_bus_idx_e bus_idx
 *          - Bus slot index.
 *
 * @return Returns qosa_netdev_err_e status code.
 */
qosa_netdev_err_e qosa_netdev_bus_gpio_deinit(qosa_netdev_bus_idx_e bus_idx);

/**
 * @brief Write data through the GPIO bus backend.
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
qosa_netdev_err_e qosa_netdev_bus_gpio_write(qosa_netdev_bus_idx_e bus_idx, qosa_uint8_t *data, qosa_uint32_t len);

/**
 * @brief Read data through the GPIO bus backend.
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
qosa_netdev_err_e qosa_netdev_bus_gpio_read(qosa_netdev_bus_idx_e bus_idx, qosa_uint8_t *data, qosa_uint32_t len);

/**
 * @brief Perform a full-duplex transfer through the GPIO bus backend.
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
qosa_netdev_err_e qosa_netdev_bus_gpio_write_read(qosa_netdev_bus_idx_e bus_idx, qosa_uint8_t *indata, qosa_uint8_t *outdata, qosa_uint32_t len);

/**
 * @brief Apply an append operation through the GPIO bus backend.
 *
 * @param[in] qosa_netdev_bus_idx_e bus_idx
 *          - Bus slot index.
 * @param[in] qosa_netdev_bus_append_t *append
 *          - Pointer to the append request.
 *
 * @return Returns qosa_netdev_err_e status code.
 */
qosa_netdev_err_e qosa_netdev_bus_gpio_append(qosa_netdev_bus_idx_e bus_idx, qosa_netdev_bus_append_t *append);

#endif /* CONFIG_QOSA_NETDEV_BUS_GPIO_FUNC */

#endif /* __QOSA_NETDEV_BUS_INTERNAL_H__ */
