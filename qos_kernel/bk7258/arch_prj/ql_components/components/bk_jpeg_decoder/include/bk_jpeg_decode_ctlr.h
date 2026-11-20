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

#include "mux_pipeline.h"
#include "components/bk_jpeg_decode/bk_jpeg_decode_types_sw.h"
#include "components/bk_jpeg_decode/bk_jpeg_decode_types_hw.h"
#include <modules/tjpgd.h>

typedef struct bk_jpeg_decode_hw *bk_jpeg_decode_hw_ctlr_handle_t;
typedef struct bk_jpeg_decode_hw bk_jpeg_decode_hw_ctlr_t;
typedef struct bk_jpeg_decode_hw_config bk_jpeg_decode_hw_ctlr_config_t;
typedef struct bk_jpeg_decode_hw_opt_config bk_jpeg_decode_hw_opt_ctlr_config_t;

typedef struct bk_jpeg_decode_sw *bk_jpeg_decode_sw_ctlr_handle_t;
typedef struct bk_jpeg_decode_sw bk_jpeg_decode_sw_ctlr_t;
typedef struct bk_jpeg_decode_sw_config bk_jpeg_decode_sw_ctlr_config_t;


typedef struct
{
    jpeg_decode_status_t status;
} private_jpeg_decode_status_t;

typedef struct
{
    frame_buffer_t *frame;
} private_jpeg_decode_param_t;

typedef struct {
	frame_buffer_t *in_frame;
	frame_buffer_t *out_frame;
	bk_err_t (*complete)(uint32_t format_type, uint32_t result, frame_buffer_t *out_frame, frame_buffer_t *in_frame, void *context); /*!< Callback when decoding is complete */
} software_decode_info_t;

typedef enum {
	SOFTWARE_DECODE_START = 0,
    SOFTWARE_DECODE_SET_ROTATE,
    SOFTWARE_DECODE_SET_OUT_FORMAT,
	SOFTWARE_DECODE_SET_BYTE_ORDER,
	SOFTWARE_DECODE_EXIT,
} software_decode_msg_type_t;

typedef enum {
	SOFTWARE_DECODE_EVENT_DECODE_START = 0,
    SOFTWARE_DECODE_EVENT_DECODE_COMPLETE,
	SOFTWARE_DECODE_EVENT_EXIT,
} software_decode_event_type_t;

typedef struct
{
    uint32_t event;
    uint32_t param;
} software_decode_msg_t;

typedef struct
{
    bk_jpeg_decode_sw_ctlr_config_t config;
    bk_jpeg_decode_sw_t ops;
    private_jpeg_decode_status_t module_status;
    jpeg_dec_handle_t jpeg_dec_handle;
    software_decode_info_t sw_dec_info;
    bk_jpeg_decode_rotate_info_t rotate_info;
} private_jpeg_decode_sw_ctlr_t;

typedef struct
{
    bk_jpeg_decode_sw_ctlr_config_t config;
    bk_jpeg_decode_sw_t ops;
    bk_jpeg_decode_rotate_info_t rotate_info;
    private_jpeg_decode_status_t module_status[MAX_DECODE_CORE];
    jpeg_dec_handle_t jpeg_dec_handle[MAX_DECODE_CORE];
    software_decode_info_t sw_dec_info[MAX_DECODE_CORE];
    beken_queue_t input_queue;
    beken_queue_t output_queue;
    beken_queue_t message_queue;
    beken_semaphore_t sem;
    beken_mutex_t lock;
    beken_thread_t thread;
    uint8_t task_running;
    uint32_t cp1_busy;
    uint32_t cp2_busy;
} private_jpeg_decode_sw_multi_core_ctlr_t;

typedef struct
{
    bk_jpeg_decode_hw_t ops;
    bk_jpeg_decode_hw_ctlr_config_t config;
    private_jpeg_decode_status_t module_status;
} private_jpeg_decode_hw_ctlr_t;

typedef struct
{
    bk_jpeg_decode_hw_t ops;
    bk_jpeg_decode_hw_opt_ctlr_config_t config;
    private_jpeg_decode_status_t module_status;
    uint8_t sram_buffer_need_free;
} private_jpeg_decode_hw_opt_ctlr_t;

avdk_err_t bk_software_jpeg_decode_ctlr_new(bk_jpeg_decode_sw_ctlr_handle_t *handle, bk_jpeg_decode_sw_ctlr_config_t *config);
avdk_err_t bk_software_jpeg_decode_on_multi_core_ctlr_new(bk_jpeg_decode_sw_ctlr_handle_t *handle, bk_jpeg_decode_sw_ctlr_config_t *config);
avdk_err_t bk_hardware_jpeg_decode_ctlr_new(bk_jpeg_decode_hw_ctlr_handle_t *handle, bk_jpeg_decode_hw_ctlr_config_t *config);
avdk_err_t bk_hardware_jpeg_decode_opt_ctlr_new(bk_jpeg_decode_hw_ctlr_handle_t *handle, bk_jpeg_decode_hw_opt_ctlr_config_t *config);
