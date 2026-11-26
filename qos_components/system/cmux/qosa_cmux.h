#ifndef __QOSA_CMUX__H__
#define __QOSA_CMUX__H__

#include "qosa_def.h"
#include "qosa_watermark.h"
#include "qosa_at_sio.h"
#include "mpc_cmux.h"

/**
 * @brief CMUX session configuration used when opening the multiplexer.
 */
typedef struct
{
    qosa_uint8_t  mode;
    qosa_uint8_t  subset;
    qosa_uint8_t  port_num;
    qosa_uint8_t  max_retry_num;
    qosa_uint16_t max_frame_size;
    qosa_uint16_t ack_time;
    qosa_uint16_t control_response_time;
    qosa_uint16_t wakeup_response_time;
    qosa_uint32_t speed;
    qosa_uint32_t window_size;
    qosa_at_dev_type_e dev_hd;
} qosa_cmux_config_t;

/**
 * @brief Opens CMUX mode on a physical AT device and creates virtual channels.
 *
 * @param config CMUX configuration supplied by the caller.
 * @return QOSA_OK on success, otherwise an error code.
 */
qosa_int32_t qosa_cmux_open(qosa_cmux_config_t *config);

/**
 * @brief Checks whether the specified virtual CMUX device is currently open.
 *
 * @param dev_port Virtual AT device port to query.
 *               Pass QOSA_DEV_NONE to query the physical CMUX device state.
 * @return QOSA_TRUE when the requested port is open, otherwise QOSA_FALSE.
 */
qosa_bool_t qosa_cmux_dev_is_open(qosa_at_dev_type_e dev_port);

/**
 * @brief Gets the active CMUX configuration from the event thread.
 *
 * @param config Output buffer used to receive the current configuration.
 * @return QOSA_OK on success, otherwise an error code.
 */
qosa_int32_t qosa_cmux_get_config(qosa_cmux_config_t *config);

#endif /* __QOSA_CMUX__H__ */
