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
#include "components/bk_dma2d_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************
 * component name: bk_dma2d
 * description: Private API (internal interface)
 ******************************************************************/

typedef enum {
    DMA2D_STATE_IDLE, 
    DMA2D_STATE_INITIALIZED,
    DMA2D_STATE_RUNNING,
    DMA2D_STATE_BUSY,
    DMA2D_STATE_ERROR,
    DMA2D_STATE_SUSPEND
} dma2d_state_t;


// DMA2D message structure
// Config is embedded in message to prevent race condition in async operations
typedef struct
{
    uint32_t event;
    uint32_t param;  // Reserved for controller pointer
    union {
        dma2d_fill_config_t fill;
        dma2d_memcpy_config_t memcpy;
        dma2d_pfc_memcpy_config_t pfc;
        dma2d_blend_config_t blend;
    } config;  // Config embedded in message (used for async operations)
} dma2d_msg_t;

typedef enum
{
    DMA2D_FILL_REQUEST,
    DMA2D_BLEND_REQUEST,
    DMA2D_MEMCPY_REQUEST,
    DMA2D_PFC_MEMCPY_REQUEST,
    DMA2D_EXIT,
} dma2d_msg_type_t;

typedef struct
{
    uint8_t task_running : 1;
    dma2d_state_t state;
    beken_mutex_t lock;
    beken_semaphore_t sem;
    beken_semaphore_t dma2d_sem;
    beken_thread_t task;
    beken_queue_t queue;
    dma2d_msg_type_t current_operation;
} dma2d_ctlr_context_t;

typedef struct
{
    dma2d_ctlr_context_t context;
    // Config for sync operations (async config is in message)
    dma2d_fill_config_t fill_config;
    dma2d_memcpy_config_t memcpy_config;
    dma2d_pfc_memcpy_config_t pfc_memcpy_config;
    dma2d_blend_config_t blend_config;
    bk_dma2d_ctlr_t ops;
    uint32_t ref_count;    // Reference count for new/delete pairing
    uint32_t open_count;   // Open count for open/close pairing
} private_dma2d_ctlr_t;

/**
 * @brief Create DMA2D controller
 * @param handle Output parameter, used to store the created DMA2D controller handle
 * @return Operation result, AVDK_ERR_OK indicates success
 */
avdk_err_t bk_dma2d_ctlr_new(bk_dma2d_ctlr_handle_t *handle);

/**
 * @brief get dma2d control handle
 * @return handle
 */
bk_dma2d_ctlr_handle_t bk_dma2d_ctlr_get(void);

#ifdef __cplusplus
}
#endif
