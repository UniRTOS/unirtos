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

#include <avdk_error.h>
#include <driver/hal/hal_yuv_buf_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Generic JPEG Encoder Types
 * 
 * This file defines the common data types, enumerations, and structures used by both
 * software and hardware JPEG encoder modules. These types are shared between different
 * implementations of the JPEG encoder.
 */

/**
 * @brief JPEG encode frame compression tuning parameters
 *
 * This structure defines the expected output JPEG size range for size-based rate control.
 * When supported by the encoder, it will adjust compression (e.g., quantization/quality) to
 * keep the encoded JPEG frame size within the configured range.
 *
 * Notes:
 * - The unit of all size fields is byte.
 * - The caller should ensure `max_size_bytes` is greater than `min_size_bytes`.
 */
typedef struct
{
    uint32_t min_size_bytes;    /*!< Expected minimum encoded JPEG frame size, in bytes */
    uint32_t max_size_bytes;    /*!< Expected maximum encoded JPEG frame size, in bytes */
} bk_jpeg_encode_frame_compress_t;

/**
 * @brief Hardware JPEG Encoder Types
 * 
 * This file defines the data types, enumerations, and structures used by the JPEG encoder module.
 * All types in this file are specific to hardware-based JPEG encoding implementation.
 */

/**
 * @brief JPEG encoder extended command enumeration
 *
 * This enumeration defines the extended control commands available for the
 * JPEG encoder through the ioctl interface.
 */
#define JPEG_ENCODE_IOCTL_CMD_BASE (0)
#define JPEG_ENCODE_IOCTL_CMD_SET_COMPRESS_PARAM (1)
#define JPEG_ENCODE_IOCTL_CMD_GET_COMPRESS_PARAM (2)

/**
 * @brief JPEG encoder configuration structure
 *
 * This structure contains configuration parameters for creating a JPEG encoder instance.
 */
typedef struct
{
    uint16_t width;         /*!< Input: Image width in pixels */
    uint16_t height;        /*!< Input: Image height in pixels */
    yuv_format_t yuv_format; /*!< Input: YUV format (byte order: big-endian) */
} bk_jpeg_encode_ctlr_config_t;

typedef struct bk_jpeg_encode_ctlr *bk_jpeg_encode_ctlr_handle_t;

/**
 * @brief JPEG encoder operations structure
 *
 * This structure defines the operations that can be performed on a JPEG encoder instance.
 * It implements the virtual function table pattern for the encoder interface.
 */
typedef struct bk_jpeg_encode_ctlr
{
    avdk_err_t (*open)(bk_jpeg_encode_ctlr_handle_t handle); /*!< Open the JPEG encoder */
    avdk_err_t (*close)(bk_jpeg_encode_ctlr_handle_t handle); /*!< Close the JPEG encoder */
    avdk_err_t (*encode)(bk_jpeg_encode_ctlr_handle_t handle, frame_buffer_t *in_frame, frame_buffer_t *out_frame); /*!< Encode JPEG frame */
    avdk_err_t (*del)(bk_jpeg_encode_ctlr_handle_t handle); /*!< Delete the JPEG encoder instance */
    avdk_err_t (*ioctl)(bk_jpeg_encode_ctlr_handle_t handle, uint32_t cmd, void *param); /*!< Extended interface for additional functionalities */
} bk_jpeg_encode_ctlr_t;

#ifdef __cplusplus
}
#endif
