/*****************************************************************/ /**
* @file qosa_camera.h
* @brief Camera operation interface
* @author bronson.zhan@quectel.com
* @date 2025-06-03
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2025-06-03 <td>1.0 <td>Bronson.Zhan <td> Init
* </table>
**********************************************************************/
#ifndef __QOSA_CAMERA_H__
#define __QOSA_CAMERA_H__

#include "qosa_def.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#include "qosa_sys.h"
#include "qosa_iic.h"

/*===========================================================================
 * Macro Definition
 ===========================================================================*/

#define QOSA_CAMERA_ERRCODE_BASE (QOSA_COMPONENT_API_CAMERA << 16)

/*===========================================================================
 * Enum
===========================================================================*/
/**
 * @enum qosa_camera_error_e
 * @brief camera error code.
 */
typedef enum
{
    QOSA_CAMERA_SUCCESS = 0,

    QOSA_CAMERA_EXECUTE_ERR = 1 | QOSA_CAMERA_ERRCODE_BASE, /*!< CAMERA execution error */
    QOSA_CAMERA_INVALID_PARAM_ERR,                          /*!< Invalid parameter */
    QOSA_CAMERA_MEM_ADDR_NULL_ERR,                          /*!< Memory address is null */
    QOSA_CAMERA_MEM_LOW_ERR,                                /*!< Insufficient memory */

    QOSA_CAMERA_DEVICE_NOT_REGISTER,                        /*!< Device not bound */
    QOSA_CAMERA_CALLBACK_NOT_REGISTER,                      /*!< Callback not bound */

    QOSA_CAMERA_INIT_ERR,                                   /*!< Initialization failed */
    QOSA_CAMERA_RELEASE_ERR,                                /*!< Deinitialization failed */

    QOSA_CAMERA_NOT_INIT,                                   /*!< Device not initialized */

    QOSA_CAMERA_MUTEX_ERROR,                                /*!< Mutex operation failed */
    QOSA_CAMERA_SEM_ERROR,                                  /*!< Semaphore operation failed */

    QOSA_CAMERA_START_ERR,                                  /*!< Start failed */
    QOSA_CAMERA_CAPTURE_ERR,                                /*!< Capture failed */

    QOSA_CAMERA_I2C_WRITE_ERR,                              /*!< I2C write failed */
    QOSA_CAMERA_I2C_READ_ERR,                               /*!< I2C read failed */
} qosa_camera_error_e;

/**
 * @enum qosa_camera_channel_e
 * @brief camera channel enum.
 */
typedef enum
{
    QOSA_CAMERA_0 = 0, /*!< Camera channel 0 */
    QOSA_CAMERA_1,     /*!< Camera channel 1 */
    QOSA_CAMERA_2,     /*!< Camera channel 2 */
    QOSA_CAMERA_MAX,
} qosa_camera_channel_e;

/**
  * @enum  qosa_camera_output_format_opt_e
  * @brief camera buffer config.
  */
typedef enum
{
    QOSA_CAMERA_OUTPUT_ONLY_Y = 0, /*!< Only Y format output(default) */
    QOSA_CAMERA_OUTPUT_YUV422,     /*!< YUV422 format output */
} qosa_camera_output_format_opt_e;

/**
  * @enum  qosa_camera_buffer_opt_e
  * @brief camera buffer config.
  */
typedef enum
{
    QOSA_CAMERA_SINGLE_BUFFER = 0, /*!< Single buffer (default) */
    QOSA_CAMERA_DOUBLE_BUFFER,     /*!< Double buffer */
} qosa_camera_buffer_opt_e;

/**
 * @enum  qosa_camera_ioctl_cmd_e
 * @brief camera ioctl cmd definition.
 */
typedef enum
{
    QOSA_CAMERA_IOCTL_NONE,
    QOSA_CAMERA_IOCTL_GET_STD_CHANNEL, /*!< Get STD version Camera default channel number
                                                        - This configuration can be the value of @ref qosa_camera_channel_e */
    QOSA_CAMERA_IOCTL_GET_OPT,         /*!< Get Camera configuration
                                                        - This configuration can be the value of @ref qosa_camera_opt_t */
    QOSA_CAMERA_IOCTL_SET_BUFFER_OPT,  /*!< Set buffer configuration
                                                        - This configuration can be the value of @ref qosa_camera_buffer_opt_e */
    QOSA_CAMERA_IOCTL_SET_FPS_OPT,     /**< Set the frame rate configuration.
                                            The value to set corresponds to an entry in @ref qosa_camera_fps_e. */
} qosa_camera_ioctl_cmd_e;

/**
 * @enum  qosa_camera_fps_e
 * @brief Frame rate options for the camera.
 */
typedef enum
{
    QOSA_CAMERA_15_FPS = 0, /**< 15 frames per second. */
    QOSA_CAMERA_30_FPS,     /**< 30 frames per second. */
} qosa_camera_fps_e;
/*===========================================================================
 *  Struct
===========================================================================*/
/**
 * @struct  qosa_camera_opt_t
 * @brief camera option.
 */
typedef struct
{
    const char *camera_name;                 /*!<  Camera model name */

    qosa_i2c_channel_e i2c_channel;          /*!<  Control i2c channel */
    qosa_i2c_mode_e    i2c_mode;             /*!<  Control i2c channel rate */
    qosa_uint8_t       i2c_sensor_id[2];     /*!<  Control i2c recognition ID */
    qosa_uint8_t       i2c_addr;             /*!<  Control i2c address */

    qosa_uint32_t camera_frequence;          /*!<  Camera SPI rate */

    qosa_uint16_t camera_image_max_width;    /*!<  Single frame maximum width */
    qosa_uint16_t camera_image_max_height;   /*!<  Single frame maximum height */

    qosa_uint8_t camera_image_output_format; /*!<  Camera output format */
} qosa_camera_opt_t;

/**
 * @struct qosa_camera_crop_info_t
 * @brief Defines a structure type qosa_camera_crop_info_t used to describe the camera cropping area information.
 *
 */
typedef struct
{
    qosa_uint16_t start_x; /*!< The X coordinate of the starting point of the crop region */
    qosa_uint16_t start_y; /*!< The Y coordinate of the starting point of the crop region */
    qosa_uint16_t end_x;   /*!< The X coordinate of the ending point of the crop region */
    qosa_uint16_t end_y;   /*!< The Y coordinate of the ending point of the crop region */
} qosa_camera_crop_info_t;

/*===========================================================================
 * Function
===========================================================================*/

/**
 * @brief Initialize CAMERA controller (internal initialization will find I2C channel by itself, but still need to configure pin func)
 *
 * @param[in] qosa_camera_channel_e camera_no
 *          - CAMERA channel number selection
 *
 * @param[in] qosa_uint32_t width
 *          - CAMERA output image width setting
 *
 * @param[in] qosa_uint32_t height
 *          - CAMERA output image height setting
 *
* @param[in]  qosa_camera_output_format_opt_e image_output_format
 *          - CAMERA output image format
 *
 * @return qosa_camera_error_e
 *          - QOSA_CAMERA_SUCCESS: Success
 *          - other: Error code
 */
qosa_camera_error_e
qosa_camera_init(qosa_camera_channel_e camera_no, qosa_uint32_t width, qosa_uint32_t height, qosa_camera_output_format_opt_e image_output_format);

/**
 * @brief Deinitialize CAMERA controller
 *
 * @param[in] qosa_camera_channel_e camera_no
 *          - CAMERA channel number selection
 *
 * @return qosa_camera_error_e
 *          - QOSA_CAMERA_SUCCESS: Success
 *          - other: Error code
 */
qosa_camera_error_e qosa_camera_deinit(qosa_camera_channel_e camera_no);

/**
 * @brief CAMERA controller starts capturing images
 *
 * @param[in] qosa_camera_channel_e camera_no
 *          - CAMERA channel number selection
 *
 * @return qosa_camera_error_e
 *          - QOSA_CAMERA_SUCCESS: Success
 *          - other: Error code
 */
qosa_camera_error_e qosa_camera_start(qosa_camera_channel_e camera_no);

/**
 * @brief CAMERA controller stops capturing images
 *
 * @param[in] qosa_camera_channel_e camera_no
 *          - CAMERA channel number selection
 *
 * @return qosa_camera_error_e
 *          - QOSA_CAMERA_SUCCESS: Success
 *          - other: Error code
 */
qosa_camera_error_e qosa_camera_stop(qosa_camera_channel_e camera_no);

/**
 * @brief CAMERA captures an image from preview
 *
 * @param[in] qosa_camera_channel_e camera_no
 *          - CAMERA channel number selection
 *
 * @param[out] qosa_uint8_t **imageBuf
 *          - An image data read from the preview buffer
 *
 * @return qosa_camera_error_e
 *          - QOSA_CAMERA_SUCCESS: Success
 *          - other: Error code
 */
qosa_camera_error_e qosa_camera_preview_image(qosa_camera_channel_e camera_no, qosa_uint8_t **imageBuf);

/**
 * @brief CAMERA release preview image
 *
 * @param[in] qosa_camera_channel_e camera_no
 *          - CAMERA channel number selection
 *
 * @param[out] qosa_uint8_t **imageBuf
 *          - Put the preview buffer back into the preview pool
 *
 * @return qosa_camera_error_e
 *          - QOSA_CAMERA_SUCCESS: Success
 *          - other: Error code
 */
qosa_camera_error_e qosa_camera_free_preview_image(qosa_camera_channel_e camera_no, qosa_uint8_t *imageBuf);

/**
 * @brief Send data to the I2C channel corresponding to the CAMERA channel
 *
 * @param[in] qosa_camera_channel_e camera_no
 *          - CAMERA channel number selection
 *
 * @param[in] qosa_uint8_t addr
 *          - I2C register address to be configured
 *
 * @param[in] qosa_uint8_t *data
 *          - I2C register data to be configured
 *
 * @param[in] qosa_uint32_t len
 *          - I2C register data length to be configured
 *
 * @return qosa_camera_error_e
 *          - QOSA_CAMERA_SUCCESS: Success
 *          - other: Error code
 */
qosa_camera_error_e qosa_camera_i2c_write(qosa_camera_channel_e camera_no, qosa_uint8_t addr, qosa_uint8_t *data, qosa_uint32_t len);

/**
 * @brief Receive data from the I2C channel corresponding to the CAMERA channel
 *
 * @param[in] qosa_camera_channel_e camera_no
 *          - CAMERA channel number selection
 *
 * @param[in] qosa_uint8_t addr
 *          - I2C register address to be obtained
 *
 * @param[out] qosa_uint8_t *data
 *          - I2C register data to be obtained
 *
 * @param[in] qosa_uint32_t len
 *          - I2C register data length to be obtained
 *
 * @return qosa_camera_error_e
 *          - QOSA_CAMERA_SUCCESS: Success
 *          - other: Error code
 */
qosa_camera_error_e qosa_camera_i2c_read(qosa_camera_channel_e camera_no, qosa_uint8_t addr, qosa_uint8_t *data, qosa_uint32_t len);

/**
 * @brief Register CAMERA event handler
 *
 * @param[in] qosa_camera_channel_e camera_no
 *          - CAMERA channel number selection
 *
 * @param[in] qosa_camera_ioctl_cmd_e cmd
 *          - Corresponding ioctl command
 *
 * @param[in/out] void * arg
 *           - Corresponding ioctl command parameters
 *
 * @return qosa_camera_error_e
 *          - QOSA_CAMERA_SUCCESS: Success
 *          - other: Error code
 */
qosa_camera_error_e qosa_camera_ioctl(qosa_camera_channel_e camera_no, qosa_camera_ioctl_cmd_e cmd, void *arg);

/**
 * @brief Convert input image data to JPEG format
 *
 * @param quality JPEG compression quality, typically in range 1-100, higher values mean better quality
 * @param crop_info Pointer to image cropping information structure, specifying the region to be cropped
 * @param pInBuf Input image data buffer pointer
 * @param uInWidth Width of the input image (in pixels)
 * @param uInHeight Height of the input image (in pixels)
 * @param pOutBuf Output JPEG data buffer pointer
 * @param pOutLength Pointer to output data length, will be updated with actual output length after function execution
 * @param image_format Output image format option enumeration
 * @return qosa_camera_error_e Return operation result status code
 */
qosa_camera_error_e qosa_camera_convert_to_jpeg(
    qosa_uint8_t                    quality,
    qosa_camera_crop_info_t        *crop_info,
    qosa_uint8_t                   *pInBuf,
    qosa_uint32_t                   uInWidth,
    qosa_uint32_t                   uInHeight,
    qosa_uint8_t                   *pOutBuf,
    qosa_uint32_t                  *pOutLength,
    qosa_camera_output_format_opt_e image_format
);

/**
 * @brief Calculate suggested output buffer length for JPEG encoding
 *
 * @param quality JPEG compression quality, typically in range 1-100, higher values mean better quality
 * @param uInWidth Width of the input image (in pixels)
 * @param uInHeight Height of the input image (in pixels)
 * @param pOutLength Pointer to required buffer length, will be updated with suggested length after function execution
 * @return qosa_camera_error_e Return operation result status code
 */
qosa_camera_error_e qosa_camera_get_suggest_length_for_jpeg(qosa_uint8_t quality, qosa_uint32_t uInWidth, qosa_uint32_t uInHeight, qosa_uint32_t *pOutLength);
#endif
#endif /* __QOSA_CAMERA_H__ */
