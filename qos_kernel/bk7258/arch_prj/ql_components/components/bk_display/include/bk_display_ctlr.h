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

#include <os/os.h>
#include "components/bk_display_types.h"

#ifdef __cplusplus
extern "C" {
#endif


/*******************************************************************
 * component name: bk_display
 * description: Private API (internal interface)
 ******************************************************************/

typedef struct
{
    uint32_t event;
    uint32_t param;
    uint32_t param2;
} display_msg_t;

typedef enum
{
    DISPLAY_FRAME_REQUEST,
    DISPLAY_FRAME_FREE,
    DISPLAY_FRAME_EXIT,
} display_msg_type_t;

typedef struct
{
    uint8_t disp_task_running : 1;
    uint8_t lcd_type;
    uint16_t lcd_width;
    uint16_t lcd_height;
    frame_buffer_t *pingpong_frame;
    frame_buffer_t *display_frame;
    beken_semaphore_t disp_sem;
    beken_semaphore_t disp_task_sem;
    beken_thread_t disp_task;
    beken_queue_t queue;
    flush_complete_t pingpong_frame_cb;
    flush_complete_t display_frame_cb;
    const lcd_device_t *lcd_device;
    bk_lcd_spi_handle_t *spi_bus_handle;
    beken_mutex_t lock;
} private_display_rgb_context_t;

typedef struct
{
    uint8_t disp_task_running : 1;
    uint8_t lcd_type;
    uint16_t lcd_width;
    uint16_t lcd_height;
    frame_buffer_t *pingpong_frame;
    frame_buffer_t *display_frame;
    beken_semaphore_t disp_sem;
    beken_semaphore_t disp_task_sem;
    beken_thread_t disp_task;
    beken_queue_t queue;
    flush_complete_t pingpong_frame_cb;
    flush_complete_t display_frame_cb;
    const lcd_device_t *lcd_device;
    bk_lcd_i80_handle_t *i80_bus_handle;
    beken_mutex_t lock;
} private_display_mcu_context_t;

typedef struct
{
    bool disp_task_running;
    beken_semaphore_t disp_task_sem;
    beken_thread_t disp_task;
    beken_queue_t queue;
    beken_mutex_t lock;
    qspi_id_t qspi_id;
    uint8_t reset_pin;
    const lcd_device_t *device;
    frame_buffer_t *display_frame;
    flush_complete_t display_frame_cb;
    bool lcd_display_flag;
} private_display_qspi_context_t;

typedef struct
{
    bool disp_task_running;
    beken_semaphore_t disp_task_sem;
    beken_thread_t disp_task;
    beken_queue_t queue;
    beken_mutex_t lock;
    qspi_id_t spi_id;
    uint8_t reset_pin;
    uint8_t dc_pin;
    const lcd_device_t *device;
    frame_buffer_t *display_frame;
    flush_complete_t display_frame_cb;
    bool lcd_display_flag;
} private_display_spi_context_t;

typedef struct
{
    bk_display_rgb_ctlr_config_t config;
    private_display_rgb_context_t rgb_context;
    bk_display_ctlr_t ops;
#ifdef CONFIG_DISPLAY_RGB888_HIGH_BIT_SHIFT
    uint8_t *rgb888_bitshift_sram;
#endif
} private_display_rgb_ctlr_t;

typedef struct
{
    bk_display_mcu_ctlr_config_t config;
    private_display_mcu_context_t mcu_context;
    bk_display_ctlr_t ops;
} private_display_mcu_ctlr_t;

typedef struct
{
    bk_display_qspi_ctlr_config_t config;
    private_display_qspi_context_t qspi_context;
    bk_display_ctlr_t ops;
} private_display_qspi_ctlr_t;

typedef struct
{
    bk_display_spi_ctlr_config_t config;
    private_display_spi_context_t spi_context;
    bk_display_ctlr_t ops;
} private_display_spi_ctlr_t;

typedef struct
{
    bk_display_dual_qspi_ctlr_config_t config;
    bk_display_ctlr_handle_t disp_handle0;
    bk_display_ctlr_handle_t disp_handle1;
    frame_buffer_t *disp_buffer1;
    frame_buffer_t *disp_buffer2;
    bk_display_ctlr_t ops;
} private_display_dual_qspi_ctlr_t;

typedef struct
{
    bk_display_dual_spi_ctlr_config_t config;
    bk_display_ctlr_handle_t disp_handle0;
    bk_display_ctlr_handle_t disp_handle1;
    frame_buffer_t *disp_buffer1;
    frame_buffer_t *disp_buffer2;
    bk_display_ctlr_t ops;
} private_display_dual_spi_ctlr_t;


avdk_err_t bk_display_rgb_ctlr_new(bk_display_ctlr_handle_t *handle, bk_display_rgb_ctlr_config_t *config);

avdk_err_t bk_display_mcu_ctlr_new(bk_display_ctlr_handle_t *handle, bk_display_mcu_ctlr_config_t *config);

avdk_err_t bk_display_qspi_ctlr_new(bk_display_ctlr_handle_t *handle, bk_display_qspi_ctlr_config_t *config);

avdk_err_t bk_display_spi_ctlr_new(bk_display_ctlr_handle_t *handle, bk_display_spi_ctlr_config_t *config);

avdk_err_t bk_display_dual_qspi_ctlr_new(bk_display_ctlr_handle_t *handle, bk_display_dual_qspi_ctlr_config_t *config);

avdk_err_t bk_display_dual_spi_ctlr_new(bk_display_ctlr_handle_t *handle, bk_display_dual_spi_ctlr_config_t *config);


#ifdef __cplusplus
}
#endif
