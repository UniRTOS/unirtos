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

#ifndef _SBC_ENC_H_
#define _SBC_ENC_H_

#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <modules/sbc_encoder.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief      Enum of SBC Encoder bit rate
 */
typedef enum
{
    SBC_ENC_BITPOOL_MIN = 2,     /* Minimum bitpool value */
    SBC_ENC_BITPOOL_DEFAULT = 32,/* Default bitpool value */
    SBC_ENC_BITPOOL_MAX = 250,   /* Maximum bitpool value */
    SBC_ENC_BITPOOL_INVALID,     /*!< Invalid bitpool */
} sbc_encoder_bitpool_t;

/**
 * @brief      Enum of SBC Encoder channel mode
 */
typedef enum
{
    SBC_ENC_CHANNEL_MODE_MONO = 0,      /* Mono */
    SBC_ENC_CHANNEL_MODE_DUAL,          /* Dual channel */
    SBC_ENC_CHANNEL_MODE_STEREO,        /* Stereo */
    SBC_ENC_CHANNEL_MODE_JOINT_STEREO,  /* Joint stereo */
    SBC_ENC_CHANNEL_MODE_MAX,           /*!< Invalid channel mode */
} sbc_encoder_channel_mode_t;

/**
 * @brief      Enum of SBC Encoder subbands
 */
typedef enum
{
    SBC_ENC_SUBBANDS_4 = 0,  /* 4 subbands */
    SBC_ENC_SUBBANDS_8,      /* 8 subbands */
    SBC_ENC_SUBBANDS_MAX,    /*!< Invalid subbands */
} sbc_encoder_subbands_t;

/**
 * @brief      Enum of SBC Encoder blocks
 */
typedef enum
{
    SBC_ENC_BLOCKS_4 = 0,   /* 4 blocks */
    SBC_ENC_BLOCKS_8,       /* 8 blocks */
    SBC_ENC_BLOCKS_12,      /* 12 blocks */
    SBC_ENC_BLOCKS_16,      /* 16 blocks */
    SBC_ENC_BLOCKS_MAX,     /*!< Invalid blocks */
} sbc_encoder_blocks_t;

/**
 * @brief      Enum of SBC Encoder allocation method
 */
typedef enum
{
    SBC_ENC_ALLOCATION_METHOD_LOUDNESS = 0, /* Loudness allocation */
    SBC_ENC_ALLOCATION_METHOD_SNR,          /* SNR allocation */
    SBC_ENC_ALLOCATION_METHOD_MAX,          /*!< Invalid allocation method */
} sbc_encoder_allocation_method_t;

/**
 * @brief      SBC Encoder configurations
 */
typedef struct
{
    int                              buf_sz;              /*!< Element Buffer size */
    int                              out_block_size;      /*!< Size of output block */
    int                              out_block_num;       /*!< Number of output block */
    int                              task_stack;          /*!< Task stack size */
    int                              task_core;           /*!< Task running in core (0 or 1) */
    int                              task_prio;           /*!< Task priority (based on freeRTOS priority) */
    int                              sample_rate;         /*!< Sample rate (16000, 32000, 44100, 48000) */
    int                              channels;            /*!< Number of channels (1 for mono, 2 for stereo) */
    sbc_encoder_bitpool_t            bitpool;             /*!< Bitpool value (2-250) */
    sbc_encoder_channel_mode_t       channel_mode;        /*!< Channel mode */
    sbc_encoder_subbands_t           subbands;            /*!< Subbands (4 or 8) */
    sbc_encoder_blocks_t             blocks;              /*!< Blocks (4, 8, 12, or 16) */
    sbc_encoder_allocation_method_t  allocation_method;   /*!< Allocation method (loudness or SNR) */
    bool                             msbc_mode;           /*!< Enable mSBC mode */
} sbc_encoder_cfg_t;

#define SBC_ENCODER_TASK_STACK          (2 * 1024)
#define SBC_ENCODER_TASK_CORE           (1)
#define SBC_ENCODER_TASK_PRIO           (5)
#define SBC_ENCODER_BUFFER_SIZE         (240)   //120 samples per frame
#define SBC_ENCODER_OUT_BLOCK_SIZE      (512)
#define SBC_ENCODER_OUT_BLOCK_NUM       (2)

#define DEFAULT_SBC_ENCODER_CONFIG() {                    \
    .buf_sz             = SBC_ENCODER_BUFFER_SIZE,        \
    .out_block_size     = SBC_ENCODER_OUT_BLOCK_SIZE,     \
    .out_block_num      = SBC_ENCODER_OUT_BLOCK_NUM,      \
    .task_stack         = SBC_ENCODER_TASK_STACK,         \
    .task_core          = SBC_ENCODER_TASK_CORE,          \
    .task_prio          = SBC_ENCODER_TASK_PRIO,          \
    .sample_rate        = 16000,                          \
    .channels           = 1,                              \
    .bitpool            = 26,                             \
    .channel_mode       = SBC_ENC_CHANNEL_MODE_MONO,      \
    .subbands           = SBC_ENC_SUBBANDS_8,             \
    .blocks             = SBC_ENC_BLOCKS_16,              \
    .allocation_method  = SBC_ENC_ALLOCATION_METHOD_LOUDNESS, \
    .msbc_mode          = true,                           \
}

/**
 * @brief      Create a SBC encoder of Audio Element to encode incoming data using SBC format
 *
 * @param[in]      config  The configuration
 *
 * @return     The audio element handle
 *                 - Not NULL: success
 *                 - NULL: failed
 */
audio_element_handle_t sbc_enc_init(sbc_encoder_cfg_t *config);


#ifdef __cplusplus
}
#endif

#endif