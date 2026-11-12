/*****************************************************************/ /**
* @file qosa_iic.h
* @brief I2C operation interface
* @author bronson.zhan@quectel.com
* @date 2025-05-08
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2025-05-08 <td>1.0 <td>Bronson.Zhan <td> Init
* </table>
**********************************************************************/
#ifndef __QOSA_IIC_H__
#define __QOSA_IIC_H__

#include "qosa_def.h"
#include "qosa_sys.h"

/*===========================================================================
 * Macro Definition
 ===========================================================================*/
#define QOSA_I2C_ERRCODE_BASE (QOSA_COMPONENT_API_I2C << 16)

/*===========================================================================
 * Enum
===========================================================================*/
/**
 * @enum qosa_i2c_mode_e
 * @brief i2c speed mode enum.
 */
typedef enum
{
    QOSA_IIC_SLOW_MODE = 1,       /*!< Slow mode (50K) */
    QOSA_IIC_STANDARD_MODE = 2,   /*!< Standard mode (100K) */
    QOSA_IIC_FAST_MODE = 3,       /*!< Fast mode (400K) */
    QOSA_IIC_FAST_PLUS_MODE = 4,  /*!< Fast+ mode (1MHz) */
    QOSA_IIC_HIGH_SPEED_MODE = 5, /*!< High speed mode (3.4M) */
} qosa_i2c_mode_e;

/**
 * @enum qosa_i2c_channel_e
 * @brief i2c channel enum.
 */
typedef enum
{
    QOSA_I2C_0 = 0, /*!< I2C channel 0 */
    QOSA_I2C_1,     /*!< I2C channel 1 */
    QOSA_I2C_2,     /*!< I2C channel 2 */
    QOSA_I2C_3,     /*!< I2C channel 3 */
} qosa_i2c_channel_e;

/**
 * @enum qosa_i2c_error_e
 * @brief i2c error codes
 */
typedef enum
{
    QOSA_I2C_SUCCESS = 0,

    QOSA_I2C_EXECUTE_ERR = 1 | QOSA_I2C_ERRCODE_BASE, /*!< I2C execution error */
    QOSA_I2C_INVALID_PARAM_ERR,                       /*!< Invalid parameter */

    QOSA_I2C_INIT_ERR,                                /*!< Initialization failed */
    QOSA_I2C_RELEASE_ERR,                             /*!< Deinitialization failed */

    QOSA_I2C_WRITE_ERR,                               /*!< Write failed */
    QOSA_I2C_READ_ERR,                                /*!< Read failed */
} qosa_i2c_error_e;

/**
 * @enum qosa_i2c_reg_addr_len_e
 * @brief i2c register address length enum.
 */
typedef enum
{
    QOSA_I2C_REG_ADDR_LEN_1BYTE = 1, /*!< Register address length is 1 byte */
    QOSA_I2C_REG_ADDR_LEN_2BYTE = 2, /*!< Register address length is 2 bytes */
} qosa_i2c_reg_addr_len_e;

/**
 * @enum qosa_i2c_ioctl_cmd_e
 * @brief i2c ioctl command enum.
 */
typedef enum
{
    QOSA_I2C_IOCTL_SET_REG_ADDR_LEN = 0, /*!< Set I2C register address length */
} qosa_i2c_ioctl_cmd_e;



/*===========================================================================
 * Function
===========================================================================*/

/**
 * @brief Initialize I2C master mode
 *
 * @param[in] qosa_i2c_channel_e i2c_no
 *          - I2C channel number selection
 *
 * @param[in] qosa_i2c_mode_e Mode
 *          - I2C rate selection
 *
 * @return qosa_i2c_error_e
 *          - QOSA_I2C_SUCCESS: Success
 *          - other: Error code
 */
qosa_i2c_error_e qosa_i2c_init(qosa_i2c_channel_e i2c_no, qosa_i2c_mode_e Mode);

/**
 * @brief Deinitialize I2C master mode
 *
 * @param[in] qosa_i2c_channel_e i2c_no
 *          - I2C channel number selection
 *
 * @return qosa_i2c_error_e
 *          - QOSA_I2C_SUCCESS: Success
 *          - other: Error code
 */
qosa_i2c_error_e qosa_i2c_deinit(qosa_i2c_channel_e i2c_no);

/**
 * @brief I2C ioctl control
 *
 * @param[in] qosa_i2c_channel_e i2c_no
 *          - I2C channel number selection
 *
 * @param[in] qosa_i2c_ioctl_cmd_e cmd
 *          - I2C ioctl command
 *
 * @param[in/out] void *arg
 *          - I2C ioctl command parameter
 *
 * @return qosa_i2c_error_e
 *          - QOSA_I2C_SUCCESS: Success
 *          - other: Error code
 */
qosa_i2c_error_e qosa_i2c_ioctl(qosa_i2c_channel_e i2c_no, qosa_i2c_ioctl_cmd_e cmd, void *arg);

/**
 * @brief I2C master mode write data
 *
 * @param[in] qosa_i2c_channel_e i2c_no
 *          - I2C channel number selection
 *
 * @param[in] qosa_uint8_t slave
 *          - I2C slave device address
 *
 * @param[in] qosa_uint16_t addr
 *          - Register address of I2C slave device to be set (8-bit address automatically converted)
 *          - Sending mode is big-endian mode (example: 0x1234 sending order is 0x12 0x34)
 *
 * @param[in] qosa_uint8_t *data
 *          - Address of data to be sent
 *
 * @param[in] qosa_uint32_t length
 *          - Length of data to be sent
 *
 * @return qosa_i2c_error_e
 *          - QOSA_I2C_SUCCESS: Success
 *          - other: Error code
 */
qosa_i2c_error_e qosa_i2c_write(qosa_i2c_channel_e i2c_no, qosa_uint8_t slave, qosa_uint16_t addr, qosa_uint8_t *data, qosa_uint32_t length);

/**
 * @brief I2C master mode read data
 *
 * @param[in] qosa_i2c_channel_e i2c_no
 *          - I2C channel number selection
 *
 * @param[in] qosa_uint8_t slave
 *          - I2C slave device address
 *
 * @param[in] qosa_uint16_t addr
 *          - Register address of I2C slave device to be read (8-bit address automatically converted)
 *          - Sending mode is big-endian mode (example: 0x1234 sending order is 0x12 0x34)
 *
 * @param[out] qosa_uint8_t *data
 *          - Address to store read data
 *
 * @param[in] qosa_uint32_t length
 *          - Length of data to be read
 *
 * @return qosa_i2c_error_e
 *          - QOSA_I2C_SUCCESS: Success
 *          - other: Error code
 */
qosa_i2c_error_e qosa_i2c_read(qosa_i2c_channel_e i2c_no, qosa_uint8_t slave, qosa_uint16_t addr, qosa_uint8_t *data, qosa_uint32_t length);

#endif /* __QOSA_IIC_H__ */
