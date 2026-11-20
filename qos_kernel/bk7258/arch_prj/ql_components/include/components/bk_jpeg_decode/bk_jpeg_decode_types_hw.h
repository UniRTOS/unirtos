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
#define delete_hw delete

/**
 * @brief Hardware JPEG Decoder Types
 * 
 * This file defines the data types, enumerations, and structures used by the hardware-based JPEG decoder module.
 * All types in this file are specific to hardware-based JPEG decoding implementation.
 */

/**
 * @brief Hardware JPEG decoder extended command enumeration
 *
 * This enumeration defines the extended control commands available for the
 * hardware JPEG decoder through the ioctl interface.
 */
typedef enum
{
    JPEG_DECODE_HW_IOCTL_CMD_BASE = 0,                  /*!< Base command start value for hardware decoder */
} bk_jpeg_decode_hw_ioctl_cmd_t;

/**
 * @brief Hardware JPEG decoder configuration structure
 *
 * This structure contains configuration parameters for creating a hardware JPEG decoder instance.
 */
typedef struct bk_jpeg_decode_hw_config
{
    bk_jpeg_decode_callback_t decode_cbs;  /*!< JPEG decode callback functions */
} bk_jpeg_decode_hw_config_t;

/**
 * @brief Hardware JPEG decoder handle type
 *
 * This type defines the handle used to reference a hardware JPEG decoder instance.
 */
typedef struct bk_jpeg_decode_hw *bk_jpeg_decode_hw_handle_t;

/**
 * @brief Copy method enumeration for SRAM to PSRAM transfer
 */
typedef enum
{
    JPEG_DECODE_OPT_COPY_METHOD_MEMCPY = 0,  /*!< Use os_memcpy for data transfer */
    JPEG_DECODE_OPT_COPY_METHOD_DMA,         /*!< Use DMA for data transfer (faster) */
} bk_jpeg_decode_opt_copy_method_t;

/**
 * @brief Copy method enumeration for SRAM to PSRAM transfer
 */
typedef enum
{
    JPEG_DECODE_OPT_LINES_PER_BLOCK_8 = 8,         /*!< Use DMA for data transfer (faster) */
    JPEG_DECODE_OPT_LINES_PER_BLOCK_16 = 16,         /*!< Use DMA for data transfer (faster) */
} bk_jpeg_decode_opt_lines_per_block_t;

/**
 * @brief Hardware optimized JPEG decoder configuration structure
 *
 * This structure contains configuration parameters for creating a hardware optimized JPEG decoder instance.
 */
typedef struct bk_jpeg_decode_hw_opt_config
{
    bk_jpeg_decode_callback_t decode_cbs;  /*!< JPEG decode callback functions */
    uint8_t *sram_buffer;                  /*!< SRAM buffer for optimized decode (NULL to auto-allocate)
                                                 Buffer size should be: image_max_width * lines_per_block * 2 (bytes per pixel YUYV)
                                                 If pingpong mode: image_max_width * lines_per_block * 2 * 2 */
    uint32_t image_max_width;              /*!< Maximum width of the image */
    uint8_t is_pingpong;                   /*!< Whether to use pingpong mode */
    bk_jpeg_decode_opt_lines_per_block_t lines_per_block;               /*!< Number of lines to decode per block */
    bk_jpeg_decode_opt_copy_method_t copy_method;  /*!< Copy method for data transfer */
} bk_jpeg_decode_hw_opt_config_t;

/**
 * @brief Hardware JPEG decoder operations structure
 *
 * This structure defines the operations that can be performed on a hardware JPEG decoder instance.
 * It implements the virtual function table pattern for the decoder interface.
 */
typedef struct bk_jpeg_decode_hw
{
    avdk_err_t (*open)(bk_jpeg_decode_hw_handle_t handle); /*!< Open the hardware JPEG decoder */
    avdk_err_t (*close)(bk_jpeg_decode_hw_handle_t handle); /*!< Close the hardware JPEG decoder */
    avdk_err_t (*decode)(bk_jpeg_decode_hw_handle_t handle, frame_buffer_t *in_frame, frame_buffer_t *out_frame); /*!< Decode JPEG frame */
    avdk_err_t (*decode_async)(bk_jpeg_decode_hw_handle_t handle, frame_buffer_t *in_frame); /*!< Decode JPEG frame async */
    avdk_err_t (*delete_hw)(bk_jpeg_decode_hw_handle_t handle); /*!< Delete the hardware JPEG decoder instance */
    avdk_err_t (*get_img_info)(bk_jpeg_decode_hw_handle_t handle, bk_jpeg_decode_img_info_t *info); /*!< Get image information before decoding */
    avdk_err_t (*ioctl)(bk_jpeg_decode_hw_handle_t handle, bk_jpeg_decode_hw_ioctl_cmd_t cmd, void *param); /*!< Extended interface for additional functionalities */
} bk_jpeg_decode_hw_t;

#ifdef __cplusplus

}
#endif

#undef delete_hw