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
    HARDWARE_DECODE_STATUS_IDLE = 0,
    HARDWARE_DECODE_STATUS_BUSY,
} hardware_decode_status_t;

typedef struct
{
	beken_thread_t hw_thread;
	beken_semaphore_t hw_sem;
	beken_semaphore_t hw_sync_sem;
	beken_queue_t hw_message_queue;
	beken_queue_t hw_input_queue;
	beken2_timer_t decode_timer;
	uint8_t decode_timer_is_running;
	uint8_t decode_err;
	uint8_t decode_timeout;
	uint8_t hw_state;
	hardware_decode_status_t hw_decode_status;
	bk_jpeg_decode_callback_t *decode_cbs;
} bk_hw_jpeg_decode_t;

typedef enum {
	HARDWARE_DECODE_EVENT_DECODE_START = 0,
    HARDWARE_DECODE_EVENT_DECODE_COMPLETE,
	HARDWARE_DECODE_EVENT_EXIT,
} hardware_decode_event_type_t;

typedef struct
{
    uint32_t event;
    uint32_t param;
} hardware_decode_msg_t;



bk_err_t hw_jpeg_decode_start(frame_buffer_t *src_frame, frame_buffer_t *dst_frame);
bk_err_t hw_jpeg_decode_start_async(frame_buffer_t *src_frame);
bk_err_t hw_jpeg_decode_init(bk_jpeg_decode_callback_t *decode_cbs);
bk_err_t hw_jpeg_decode_deinit(void);

#ifdef __cplusplus
}
#endif



