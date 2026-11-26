#ifndef __MPC_CMUX_H__
#define __MPC_CMUX_H__

#include "qosa_def.h"
#include "qosa_at_sio.h"

#define QOSA_CMUX_PORT_MAX      5
#define QOSA_CMUX_DATA_PORT_MAX (QOSA_CMUX_PORT_MAX - 1)

/**
 * @brief Control commands used to update or query CMUX virtual serial line state.
 */
typedef enum
{
    QOSA_CMUX_SERIAL_IOC_DCD_SET = 1,
    QOSA_CMUX_SERIAL_IOC_DCD_GET,
    QOSA_CMUX_SERIAL_IOC_RI_SET,
    QOSA_CMUX_SERIAL_IOC_RI_GET,
    QOSA_CMUX_SERIAL_IOC_RTS_SET,
    QOSA_CMUX_SERIAL_IOC_RTS_GET,
    QOSA_CMUX_SERIAL_IOC_DTR_UPDATE,
    QOSA_CMUX_SERIAL_IOC_DTR_GET,
    QOSA_CMUX_SERIAL_IOC_CTS_UPDATE,
    QOSA_CMUX_SERIAL_IOC_CTS_GET,
} qosa_cmux_serial_ioc_cmd_e;

/**
 * @brief Callback used to transfer CMUX payload through an AT device port.
 *
 * @param dev_port Source or destination AT device port.
 * @param data Payload buffer.
 * @param len Payload length in bytes.
 * @return QOSA_OK on success, otherwise an error code.
 */
typedef qosa_int32_t (*qosa_cmux_data_cb)(qosa_at_dev_type_e dev_port, qosa_uint8_t *data, qosa_uint16_t len);

typedef qosa_int32_t (*qosa_cmux_ioctl_cb)(qosa_at_dev_type_e dev_port, qosa_cmux_serial_ioc_cmd_e cmd, qosa_int32_t *level);

/**
 * @brief Describes one virtual CMUX device exposed by the platform adapter.
 */
typedef struct
{
    qosa_cmux_data_cb  mpc_write;
    qosa_uint8_t       dlci;
    qosa_at_dev_type_e dev_port;
} mpc_cmux_dev_t;

/**
 * @brief Configuration used to create MPC-side CMUX virtual devices.
 */
typedef struct
{
    qosa_at_dev_type_e dev_hd;
    qosa_uint8_t       mode;
    qosa_uint8_t       port_num;
    qosa_uint32_t      speed;
    qosa_cmux_data_cb  data_to_cmux;
    qosa_cmux_ioctl_cb cmux_ioctl;
} mpc_cmux_config_t;

/**
 * @brief Initializes the platform MPC CMUX virtual devices.
 *
 * @param config MPC CMUX initialization parameters.
 * @return Allocated virtual device table, or NULL on failure.
 */
mpc_cmux_dev_t *mpc_cmux_init(mpc_cmux_config_t *config);

#endif /* __MPC_CMUX_H__ */
