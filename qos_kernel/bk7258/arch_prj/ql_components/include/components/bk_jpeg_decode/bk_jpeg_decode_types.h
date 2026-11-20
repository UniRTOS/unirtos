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
#include "frame_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Generic JPEG Decoder Types
 * 
 * This file defines the common data types, enumerations, and structures used by both
 * software and hardware JPEG decoder modules. These types are shared between different
 * implementations of the JPEG decoder.
 */

 /**
 * @brief JPEG decoder status enumeration
 *
 * This enumeration defines the operational states of the JPEG decoder.
 */
typedef enum
{
    JPEG_DECODE_DISABLED = 0,  /*!< Decoder is disabled and cannot perform decoding operations */
    JPEG_DECODE_ENABLED        /*!< Decoder is enabled and ready for decoding operations */
} jpeg_decode_status_t;

/**
 * @brief JPEG image format enumeration
 *
 * This enumeration defines the supported image formats for JPEG decoding.
 * These formats specify different chroma subsampling schemes for YUV color space.
 */
typedef enum
{
    JPEG_FMT_ERR,           /*!< Invalid or unsupported image format */
    JPEG_FMT_YUV444,        /*!< YUV 4:4:4 format - full chroma resolution, no subsampling */
    JPEG_FMT_YUV422,        /*!< YUV 4:2:2 format - horizontal chroma subsampling by 2:1 */
    JPEG_FMT_YUV420,        /*!< YUV 4:2:0 format - horizontal and vertical chroma subsampling by 2:1 */
    JPEG_FMT_YUV400,        /*!< YUV 4:0:0 format - grayscale image with only luma component */
} jpeg_img_fmt_t;

/**
 * @brief JPEG decode image info structure
 *
 * This structure is used to retrieve the information and format of a JPEG image before decoding.
 */
typedef struct bk_jpeg_decode_img_info
{
    /*!< [Input] Frame buffer containing the encoded JPEG data, frame->frame and frame->length must be valid */
    frame_buffer_t *frame;
    uint32_t width;         /*!< Output: Image width in pixels */
    uint32_t height;        /*!< Output: Image height in pixels */
    jpeg_img_fmt_t format;  /*!< Output: Image format (YUV444, YUV422, YUV420, etc.) */
} bk_jpeg_decode_img_info_t;

/**
 * @brief JPEG decode callback structure
 *
 * This structure defines callback functions for JPEG decoding operations,
 * allowing the application to be notified of significant events.
 *
 * @warning Callback Function Usage Notes:
 * - Blocking operations (such as long waits, sleep, etc.) are NOT recommended in callback functions
 *   to avoid impacting decoding performance and system responsiveness.
 * - Callback functions should only perform lightweight operations such as setting flags,
 *   sending messages/semaphores, etc. Move time-consuming operations to other tasks.
 */
typedef struct {
    bk_err_t (*in_complete)(frame_buffer_t *in_frame); /*!< Callback when input data decoding is complete */
    frame_buffer_t *(*out_malloc)(uint32_t size); /*!< Callback to allocate output buffer */
    bk_err_t (*out_complete)(uint32_t format_type, uint32_t result, frame_buffer_t *out_frame); /*!< Callback when output data processing is complete */
} bk_jpeg_decode_callback_t;

#ifdef __cplusplus
}
#endif
