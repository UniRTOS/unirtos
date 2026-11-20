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

#ifndef _SBC_DECODER_H_
#define _SBC_DECODER_H_

#include <components/bk_audio/audio_pipeline/audio_element.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief      SBC Decoder configurations
 */
typedef struct
{
    int                     buf_sz;         /*!< Element Buffer size */
    int                     out_block_size; /*!< Size of output block */
    int                     out_block_num;  /*!< Number of output block */
    int                     task_stack;     /*!< Task stack size */
    int                     task_core;      /*!< Task running in core (0 or 1) */
    int                     task_prio;      /*!< Task priority (based on freeRTOS priority) */
} sbc_decoder_cfg_t;

#define SBC_DECODER_TASK_STACK          (2 * 1024)
#define SBC_DECODER_TASK_CORE           (1)
#define SBC_DECODER_TASK_PRIO           (5)
#define SBC_DECODER_BUFFER_SIZE         (512)
#define SBC_DECODER_OUT_BLOCK_SIZE      (512)
#define SBC_DECODER_OUT_BLOCK_NUM       (2)

#define DEFAULT_SBC_DECODER_CONFIG() {                  \
    .buf_sz             = SBC_DECODER_BUFFER_SIZE,      \
    .out_block_size     = SBC_DECODER_OUT_BLOCK_SIZE,   \
    .out_block_num      = SBC_DECODER_OUT_BLOCK_NUM,    \
    .task_stack         = SBC_DECODER_TASK_STACK,       \
    .task_core          = SBC_DECODER_TASK_CORE,        \
    .task_prio          = SBC_DECODER_TASK_PRIO,        \
}

/**
 * @brief      Create a SBC decoder of Audio Element to decode incoming data using SBC format
 *
 * @param[in]      config  The configuration
 *
 * @return     The audio element handle
 *                 - Not NULL: success
 *                 - NULL: failed
 */
audio_element_handle_t sbc_decoder_init(sbc_decoder_cfg_t *config);


#ifdef __cplusplus
}
#endif

#endif