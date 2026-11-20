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

#ifndef _MIX_ALGORITHM_H_
#define _MIX_ALGORITHM_H_

#include <components/bk_audio/audio_pipeline/audio_element.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief      Mix Algorithm configurations
 */
typedef struct
{
    int                     buf_sz;             /*!< Element Buffer size */
    int                     out_block_size;     /*!< Size of output block */
    int                     out_block_num;      /*!< Number of output block */
    int                     task_stack;         /*!< Task stack size */
    int                     task_core;          /*!< Task running in core (0 or 1) */
    int                     task_prio;          /*!< Task priority (based on freeRTOS priority) */
    int                     input_channel_num;  /*!< Number of input channels */
} mix_algorithm_cfg_t;

#define MIX_ALGORITHM_TASK_STACK          (1 * 1024)
#define MIX_ALGORITHM_TASK_CORE           (1)
#define MIX_ALGORITHM_TASK_PRIO           (5)
#define MIX_ALGORITHM_BUFFER_SIZE         (320)
#define MIX_ALGORITHM_OUT_BLOCK_SIZE      (160)
#define MIX_ALGORITHM_OUT_BLOCK_NUM       (2)
#define MIX_ALGORITHM_INPUT_CHANNEL_NUM   (2) /* Default 2 input channels */

#define DEFAULT_MIX_ALGORITHM_CONFIG() {                    \
    .buf_sz             = MIX_ALGORITHM_BUFFER_SIZE,        \
    .out_block_size     = MIX_ALGORITHM_OUT_BLOCK_SIZE,     \
    .out_block_num      = MIX_ALGORITHM_OUT_BLOCK_NUM,      \
    .task_stack         = MIX_ALGORITHM_TASK_STACK,         \
    .task_core          = MIX_ALGORITHM_TASK_CORE,          \
    .task_prio          = MIX_ALGORITHM_TASK_PRIO,          \
    .input_channel_num  = MIX_ALGORITHM_INPUT_CHANNEL_NUM,  \
}

/**
 * @brief      Create a Mix algorithm of Audio Element to mix multi-channel audio data
 *
 * @param[in]      config  The configuration
 *
 * @return     The audio element handle
 *                 - Not NULL: success
 *                 - NULL: failed
 */
audio_element_handle_t mix_algorithm_init(mix_algorithm_cfg_t *config);


#ifdef __cplusplus
}
#endif

#endif