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

#include <common/bk_include.h>
#include <components/media_types.h>
#include <components/bk_jpeg_decode/bk_jpeg_decode_types_hw.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HARDWARE_OPT_DECODE_STATUS_IDLE = 0,
    HARDWARE_OPT_DECODE_STATUS_BUSY,
    HARDWARE_OPT_DECODE_STATUS_PAUSED,
} hardware_opt_decode_status_t;

typedef enum
{
    MUX_OPT_BUFFER_IDLE = 0,
    MUX_OPT_BUFFER_DECODING,  // Hardware is decoding to this buffer
    MUX_OPT_BUFFER_COPYING,   // Data is being copied from SRAM to PSRAM
    MUX_OPT_BUFFER_READY,     // Copy completed, buffer is ready
} mux_opt_buffer_state_t;

typedef enum {
    HARDWARE_OPT_DECODE_EVENT_DECODE_START = 0,
    HARDWARE_OPT_DECODE_EVENT_DECODE_CONTINUE,
    HARDWARE_OPT_DECODE_EVENT_DECODE_COMPLETE,
    HARDWARE_OPT_DECODE_EVENT_DECODE_TIMEOUT,
    HARDWARE_OPT_DECODE_EVENT_EXIT,
} hardware_opt_decode_event_type_t;

typedef struct
{
    uint32_t event;
    uint32_t param;
} hardware_opt_decode_msg_t;

typedef struct
{
    frame_buffer_t *src_frame;
    frame_buffer_t *dst_frame;
    uint32_t lines_per_block;
    bool is_async;
} hw_opt_decode_work_info_t;

// Request structure for async operations (stored in queue)
typedef struct
{
    frame_buffer_t *src_frame;
    uint32_t lines_per_block;
} hw_opt_decode_request_t;

// Initialization configuration structure
typedef struct
{
    bk_jpeg_decode_callback_t *decode_cbs;  // Callback functions for decode events
    uint8_t *sram_buffer;                   // SRAM buffer for optimized decode
    uint32_t lines_per_block;               // Number of lines to decode per block
    bool is_pingpong;                       // Whether to use pingpong mode
    bk_jpeg_decode_opt_copy_method_t copy_method;  // Copy method for data transfer
} hw_opt_decode_init_config_t;

/**
 * @brief Start hardware optimized JPEG decoding (synchronous)
 *
 * @param src_frame Input JPEG frame buffer
 * @param dst_frame Output decoded frame buffer
 *
 * @return
 *      - BK_OK: Success
 *      - BK_ERR_BUSY: Decoder is busy
 *      - Others: Decode failed
 */
bk_err_t hw_jpeg_decode_opt_start(frame_buffer_t *src_frame, frame_buffer_t *dst_frame);

/**
 * @brief Start hardware optimized JPEG decoding (asynchronous)
 *
 * @param src_frame Input JPEG frame buffer
 *
 * @return
 *      - BK_OK: Success
 *      - Others: Decode failed
 */
bk_err_t hw_jpeg_decode_opt_start_async(frame_buffer_t *src_frame);

/**
 * @brief Initialize hardware optimized JPEG decoder
 *
 * @param config Initialization configuration structure
 *
 * @return
 *      - BK_OK: Success
 *      - Others: Init failed
 */
bk_err_t hw_jpeg_decode_opt_init(hw_opt_decode_init_config_t *config);

/**
 * @brief Deinitialize hardware optimized JPEG decoder
 *
 * @return
 *      - BK_OK: Success
 *      - Others: Deinit failed
 */
bk_err_t hw_jpeg_decode_opt_deinit(void);

#ifdef __cplusplus
}
#endif

