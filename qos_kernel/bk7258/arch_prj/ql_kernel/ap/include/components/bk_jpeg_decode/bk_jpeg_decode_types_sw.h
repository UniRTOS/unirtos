// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "components/avdk_utils/avdk_error.h"
#include "components/bk_jpeg_decode/bk_jpeg_decode_types.h"

#ifdef __cplusplus
extern "C" {
#endif

//this is for fix C++ cann't use delete as value
#define delete_sw delete

/**
 * @brief Software JPEG Decoder Types
 * 
 * This file defines the data types, enumerations, and structures used by the software-based JPEG decoder module.
 * All types in this file are specific to software-based JPEG decoding implementation.
 */

#define SW_DECODE_ROTATE_BUFFER_SIZE (16 * 16 * 2)
#define MAX_DECODE_CORE (2)

#define JPEG_DECODE_CORE_ID_1 (1 << 0)
#define JPEG_DECODE_CORE_ID_2 (1 << 1)

/**
 * @brief JPEG decode byte order enumeration
 *
 * This enumeration defines the supported byte order configurations for decoded image data.
 */
typedef enum{
	JPEG_DECODE_LITTLE_ENDIAN = 0,  /*!< Little-endian byte order (LSB first) */
	JPEG_DECODE_BIG_ENDIAN           /*!< Big-endian byte order (MSB first) */
} bk_jpeg_decode_byte_order_t;

/**
 * @brief Software JPEG decoder output format enumeration
 *
 * This enumeration defines the supported output formats for the software JPEG decoder.
 * These formats include various color spaces and rotation options.
 */
typedef enum
{
    JPEG_DECODE_SW_OUT_FORMAT_GRAY = 0,                 /*!< Grayscale output format */
    JPEG_DECODE_SW_OUT_FORMAT_RGB565 = 1,               /*!< RGB565 output format (16-bit per pixel) */
    JPEG_DECODE_SW_OUT_FORMAT_RGB888 = 2,               /*!< RGB888 output format (24-bit per pixel) */
    JPEG_DECODE_SW_OUT_FORMAT_YUYV = 3,                 /*!< YUYV interleaved format (4:2:2) */
    JPEG_DECODE_SW_OUT_FORMAT_YUYV_ROTATE_90 = 4,       /*!< YUYV format with 90-degree clockwise rotation */
    JPEG_DECODE_SW_OUT_FORMAT_YUYV_ROTATE_180 = 5,      /*!< YUYV format with 180-degree rotation */
    JPEG_DECODE_SW_OUT_FORMAT_YUYV_ROTATE_270 = 6,      /*!< YUYV format with 270-degree clockwise rotation */
    JPEG_DECODE_SW_OUT_FORMAT_VYUY = 7,                 /*!< VYUY interleaved format (alternative 4:2:2) */
    JPEG_DECODE_SW_OUT_FORMAT_VUYY = 8,                 /*!< VUYY interleaved format (another alternative 4:2:2) */
} bk_jpeg_decode_sw_out_format_t;

/**
 * @brief Software JPEG decoder extended command enumeration
 *
 * This enumeration defines the extended control commands available for the
 * software JPEG decoder through the ioctl interface.
 */
typedef enum
{
    JPEG_DECODE_SW_IOCTL_CMD_BASE = 0,                  /*!< Base command start value for software decoder */
} bk_jpeg_decode_sw_ioctl_cmd_t;

/**
 * @brief JPEG decode rotate information structure
 * @note If no rotation is needed, set rotate_angle to ROTATE_NONE and rotate_buf to NULL
 *       If rotation is needed, set rotate_angle to the desired rotation angle (ROTATE_90, ROTATE_180, ROTATE_270)
 *       If rotate_buf is NULL, internal buffer (16*16*2) will be allocated
 *       If rotate_buf is not NULL, external buffer will be used
 */
typedef struct bk_jpeg_decode_rotate_info
{
    uint32_t rotate_angle;  /*!< Rotate angle */
    uint8_t *rotate_buf;    /*!< Rotate buffer */
} bk_jpeg_decode_rotate_info_t;

/**
 * @brief JPEG decode output format command structure
 *
 * This structure specifies the desired output format for decoded JPEG images.
 * It is used with the JPEG_DECODE_SW_IOCTL_CMD_SET_OUT_FORMAT command.
 */
typedef struct bk_jpeg_decode_out_format_info
{
    bk_jpeg_decode_sw_out_format_t out_format;  /*!< Output image format (RGB565, RGB888, YUYV, etc.) */
} bk_jpeg_decode_out_format_info_t;

/**
 * @brief JPEG decode byte order command structure
 *
 * This structure specifies the byte order for decoded JPEG image data.
 * It is used with the JPEG_DECODE_SW_IOCTL_CMD_SET_BYTE_ORDER command.
 */
typedef struct bk_jpeg_decode_byte_order_info
{
    bk_jpeg_decode_byte_order_t byte_order;  /*!< Byte order (JPEG_DECODE_LITTLE_ENDIAN or JPEG_DECODE_BIG_ENDIAN) */
} bk_jpeg_decode_byte_order_info_t;

/**
 * @brief Software JPEG decoder configuration structure
 *
 * This structure contains configuration parameters for creating a software JPEG decoder instance.
 */
typedef struct bk_jpeg_decode_sw_config
{
    bk_jpeg_decode_callback_t decode_cbs;  /*!< JPEG decode callback functions */
    uint32_t core_id;                     /*!< CPU core ID on which the decoder will run */
    bk_jpeg_decode_sw_out_format_t out_format; /*!< Output image format */
    bk_jpeg_decode_byte_order_t byte_order;  /*!< Byte order for decoded data */
} bk_jpeg_decode_sw_config_t;

typedef struct bk_jpeg_decode_sw_out_frame_info
{
    bk_jpeg_decode_sw_out_format_t out_format; /*!< Output image format */
    bk_jpeg_decode_byte_order_t byte_order;  /*!< Byte order for decoded data */
} bk_jpeg_decode_sw_out_frame_info_t;

/**
 * @brief Software JPEG decoder handle type
 *
 * This type defines the handle used to reference a software JPEG decoder instance.
 */
typedef struct bk_jpeg_decode_sw *bk_jpeg_decode_sw_handle_t;

/**
 * @brief Software JPEG decoder operations structure
 *
 * This structure defines the operations that can be performed on a software JPEG decoder instance.
 * It implements the virtual function table pattern for the decoder interface.
 */
typedef struct bk_jpeg_decode_sw
{
    avdk_err_t (*open)(bk_jpeg_decode_sw_handle_t handle); /*!< Open the software JPEG decoder */
    avdk_err_t (*close)(bk_jpeg_decode_sw_handle_t handle); /*!< Close the software JPEG decoder */
    avdk_err_t (*decode)(bk_jpeg_decode_sw_handle_t handle, frame_buffer_t *in_frame, frame_buffer_t *out_frame); /*!< Decode JPEG frame */
    avdk_err_t (*decode_async)(bk_jpeg_decode_sw_handle_t handle, frame_buffer_t *in_frame); /*!< Decode JPEG frame asynchronously */
    avdk_err_t (*delete_sw)(bk_jpeg_decode_sw_handle_t handle); /*!< Delete the software JPEG decoder instance */
    avdk_err_t (*set_config)(bk_jpeg_decode_sw_handle_t handle, bk_jpeg_decode_sw_out_frame_info_t *config); /*!< Set the output format for decoded images */
    avdk_err_t (*get_img_info)(bk_jpeg_decode_sw_handle_t handle, bk_jpeg_decode_img_info_t *info); /*!< Get image information before decoding */
    avdk_err_t (*ioctl)(bk_jpeg_decode_sw_handle_t handle, bk_jpeg_decode_sw_ioctl_cmd_t cmd, void *param); /*!< Extended interface for additional functionalities */
} bk_jpeg_decode_sw_t;

#ifdef __cplusplus

}
#endif
