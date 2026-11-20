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
#include "components/media_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Video Pipeline Types
 * 
 * This file defines the data types, enumerations, and structures used by the video pipeline module.
 * The video pipeline provides a unified management interface for functions such as video encoding/decoding 
 * and image rotation, simplifying the complexity of multi-module collaborative work.
 */

//this is for fix C++ cann't use delete as value
#define delete_video_pipeline delete

/**
 * @brief Default H.264 encoding configuration for 864x480 resolution at 30 FPS
 * 
 * This macro provides a default configuration for H.264 encoding with:
 * - Resolution: 864x480
 * - Frame rate: 30 FPS
 * - Software decode rotate angle: default:0 range:(0, 90, 180, 270)
 */
#define BK_H264_ENCODE_864X480_30FPS_CONFIG() \
{ \
    .width = 864, \
    .height = 480, \
    .fps = FPS30, \
    .sw_rotate_angle = ROTATE_NONE, \
}

/**
 * @brief Decoder end type enumeration
 * 
 * This enumeration defines the different types of decoder end operations
 */
typedef enum
{
    HW_DEC_END = 0,  /*!< Hardware decoding end */
    SW_DEC_END,      /*!< Software decoding end */
} dec_end_type_t;

/**
 * @brief H.264 encoder working state enumeration
 * 
 * This enumeration defines the working states of the H.264 encoder
 */
typedef enum
{
    H264E_STATE_IDLE = 0,      /*!< Encoder idle state */
    H264E_STATE_ENCODING       /*!< Encoder encoding state */
} h264e_state_t;

/**
 * @brief Video pipeline module type enumeration
 * 
 * This enumeration defines the different module types in the video pipeline
 */
typedef enum
{
    VIDEO_PIPELINE_MODULE_H264E = 0,   /*!< H.264 encoder module */
    VIDEO_PIPELINE_MODULE_ROTATE,      /*!< Image rotation module */
} video_pipeline_module_t;

/**
 * @brief Video pipeline module status enumeration
 * 
 * This enumeration defines the enable status of modules in the video pipeline
 */
typedef enum
{
    VIDEO_PIPELINE_MODULE_DISABLED = 0,  /*!< Module disabled state */
    VIDEO_PIPELINE_MODULE_ENABLED        /*!< Module enabled state */
} video_pipeline_module_status_t;

/**
 * @brief Video pipeline extended command enumeration
 * 
 * This enumeration defines the extended commands for the video pipeline
 */
typedef enum
{
    VIDEO_PIPELINE_IOCTL_CMD_BASE = 0,                  /*!< Base command start value */
} video_pipeline_ioctl_cmd_t;

/**
 * @brief H.264 encoder callback structure
 * 
 * This structure defines the callback functions for H.264 encoding operations,
 * including memory allocation and completion notification.
 *
 * @warning Callback Function Usage Notes:
 * - Blocking operations (such as long waits, sleep, etc.) are NOT recommended in callback functions
 *   to avoid impacting encoding performance and system responsiveness.
 * - Callback functions should only perform lightweight operations such as setting flags,
 *   sending messages/semaphores, etc. Move time-consuming operations to other tasks.
 */
typedef struct
{
    /**
     * @brief Allocate memory for frame buffer
     * @param size Size of memory to allocate
     * @return Pointer to allocated frame buffer
     */
    frame_buffer_t *(*malloc)(uint32_t size);

    /**
     * @brief Frame encoding completion callback
     * @param frame Pointer to the frame buffer
     * @param result Result of the encoding operation
     */
    void (*complete)(frame_buffer_t *frame, bk_err_t result);
} bk_h264e_callback_t;

/**
 * @brief JPEG decode callback structure
 * 
 * This structure defines the callback functions for JPEG decoding operations
 *
 * @warning Callback Function Usage Notes:
 * - Blocking operations (such as long waits, sleep, etc.) are NOT recommended in callback functions
 *   to avoid impacting decoding performance and system responsiveness.
 * - Callback functions should only perform lightweight operations such as setting flags,
 *   sending messages/semaphores, etc. Move time-consuming operations to other tasks.
 * - Exception: The read() callback may perform short blocking waits to acquire a ready JPEG buffer.
 */
typedef struct {
    frame_buffer_t *(*read)(uint32_t timeout_ms);    /*!< Callback to read/acquire a prepared JPEG buffer, short blocking wait is allowed */
    bk_err_t (*complete)(bk_err_t result, frame_buffer_t *out_frame); /*!< Callback when decoding is complete */
} jpeg_callback_t;

/**
 * @brief Image codec callback structure
 * 
 * This structure defines the callback functions for image codec operations
 *
 * @warning Callback Function Usage Notes:
 * - Blocking operations (such as long waits, sleep, etc.) are NOT recommended in callback functions
 *   to avoid impacting decoding performance and system responsiveness.
 * - Callback functions should only perform lightweight operations such as setting flags,
 *   sending messages/semaphores, etc. Move time-consuming operations to other tasks.
 */
typedef struct {
    frame_buffer_t *(*malloc)(uint32_t size);    /*!< Callback to allocate frame buffer */
    bk_err_t (*free)(frame_buffer_t *frame);     /*!< Callback to free frame buffer */
    bk_err_t (*complete)(dec_end_type_t format_type, bk_err_t result, frame_buffer_t *out_frame); /*!< Callback when processing is complete */                   /*!< Rotation angle */
} decode_callback_t;

/**
 * @brief Video pipeline H.264 encoder configuration structure
 * 
 * This structure defines the configuration parameters for the H.264 encoder in the video pipeline
 */
typedef struct bk_video_pipeline_h264e_config
{
    uint16_t width;            /**< Width of the video frame */
    uint16_t height;           /**< Height of the video frame */
    uint32_t fps;              /**< Frames per second */
    media_rotate_t sw_rotate_angle;  /**< Software rotation angle */
    const bk_h264e_callback_t *h264e_cb;        /*!< H.264 encoder callback functions */
} bk_video_pipeline_h264e_config_t;

/**
 * @brief Video pipeline decode configuration structure
 * 
 * This structure defines the configuration parameters for the decode module in the video pipeline
 */
typedef struct bk_video_pipeline_decode_config
{
    media_rotate_mode_t rotate_mode;   /*!< Rotation mode (1: software rotation, 0: hardware rotation) */
    media_rotate_t rotate_angle;                  /*!< Rotation angle */
} bk_video_pipeline_decode_config_t;

/**
 * @brief Video pipeline configuration structure
 * 
 * This structure defines the main configuration parameters for the video pipeline
 */
typedef struct bk_video_pipeline_config
{
    const jpeg_callback_t *jpeg_cbs; /*!< JPEG decode callback functions */
    const decode_callback_t *decode_cbs; /*!< Image codec callback functions */
} bk_video_pipeline_config_t;

/**
 * @brief Video pipeline handle type
 * 
 * This is the handle type for the video pipeline instance
 */
typedef struct bk_video_pipeline *bk_video_pipeline_handle_t;

/**
 * @brief Video pipeline operations structure
 * 
 * This structure defines the operations available for the video pipeline instance
 */
typedef struct bk_video_pipeline
{
    avdk_err_t (*open_h264e)(bk_video_pipeline_handle_t handle, bk_video_pipeline_h264e_config_t *config); /*!< Open H.264 encoder */
    avdk_err_t (*close_h264e)(bk_video_pipeline_handle_t handle); /*!< Close H.264 encoder */
    avdk_err_t (*open_rotate)(bk_video_pipeline_handle_t handle, bk_video_pipeline_decode_config_t *config); /*!< Open image rotation module */
    avdk_err_t (*close_rotate)(bk_video_pipeline_handle_t handle); /*!< Close image rotation module */
    avdk_err_t (*reset_decode)(bk_video_pipeline_handle_t handle); /*!< Reset decode module, This function is used when switching cameras to internally re-acquire the image format. */
    avdk_err_t (*get_module_status)(bk_video_pipeline_handle_t handle, video_pipeline_module_t module, video_pipeline_module_status_t *status); /*!< Get module enable state */
    avdk_err_t (*delete_video_pipeline)(bk_video_pipeline_handle_t handle); /*!< Delete the video pipeline instance */
    avdk_err_t (*ioctl)(bk_video_pipeline_handle_t handle, video_pipeline_ioctl_cmd_t cmd, void *param); /*!< Extended interface for additional functionalities */
} bk_video_pipeline_t;

#ifdef __cplusplus
}
#endif