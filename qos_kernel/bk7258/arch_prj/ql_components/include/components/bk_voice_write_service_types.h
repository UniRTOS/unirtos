// Copyright 2025-2026 Beken
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

#include <components/bk_voice_service_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Voice Write Service Types
 * 
 * This file defines the data types and structures used by the
 * voice write service module. These types are used to configure
 * and control voice write service operations.
 */

/**
 * @brief Voice write handle type
 */
typedef struct voice_write *voice_write_handle_t;

/**
 * @brief Voice write configuration structure
 */
typedef struct {
    voice_handle_t voice_handle;            /*!< voice handle */
    uint32_t start_threshold;               /*!< The threshold to start writing data */
    uint32_t pause_threshold;               /*!< The threshold to pause writing data */
    int task_stack;                         /*!< Task stack size */
    int task_core;                          /*!< Task running in core (0 or 1) */
    int task_prio;                          /*!< Task priority (based on freeRTOS priority) */
    audio_mem_type_t mem_type;              /*!< memory type used, sram, psram or audio heap */
    audio_buf_type_t write_buf_type;        /*!< write buffer type used, frame buffer or ring buffer */
    int node_size;                          /*!< frame buffer node size or ring buffer pool size*/
    int node_num;                           /*!< frame buffer node number or 1 for ring buffer */
} voice_write_cfg_t;

/**
 * @brief Default voice write task priority
 */
#define VOICE_WRITE_TASK_PRIO           (BEKEN_DEFAULT_WORKER_PRIORITY - 1)

/**
 * @brief Default voice write pool size
 */
#define VOICE_WRITE_POOL_SIZE           (3200)

/**
 * @brief Default voice write start threshold
 */
#define VOICE_WRITE_START_THRESHOLD     (1280)

/**
 * @brief Default voice write pause threshold
 */
#define VOICE_WRITE_PAUSE_THRESHOLD     (0)

/**
 * @brief Default voice write configuration
 * 
 * This configuration defines default settings for voice write service operations using:
 * - Voice handle: NULL
 * - Start threshold: VOICE_WRITE_START_THRESHOLD
 * - Pause threshold: VOICE_WRITE_PAUSE_THRESHOLD
 * - Task stack size: 2048 bytes
 * - Task running in core: 0
 * - Task priority: VOICE_READ_TASK_PRIO
 * - Memory type: AUDIO_MEM_TYPE_PSRAM
 * - Write buffer type: AUDIO_BUF_TYPE_RB
 * - Node size: VOICE_WRITE_POOL_SIZE
 * - Node number: 1
 */
#define VOICE_WRITE_CFG_DEFAULT() {                 \
    .voice_handle = NULL,                           \
    .start_threshold = VOICE_WRITE_START_THRESHOLD, \
    .pause_threshold = VOICE_WRITE_PAUSE_THRESHOLD, \
    .task_stack = 2048,                             \
    .task_core = 0,                                 \
    .task_prio = VOICE_WRITE_TASK_PRIO,              \
    .mem_type = AUDIO_MEM_TYPE_PSRAM,               \
    .write_buf_type = AUDIO_BUF_TYPE_RB,            \
    .node_size = VOICE_WRITE_POOL_SIZE,             \
    .node_num = 1,                                  \
}


#ifdef __cplusplus
}
#endif

