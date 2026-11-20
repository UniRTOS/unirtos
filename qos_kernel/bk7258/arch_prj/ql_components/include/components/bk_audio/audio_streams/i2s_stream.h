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


#ifndef _I2S_STREAM_H_
#define _I2S_STREAM_H_

#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <components/bk_audio/audio_pipeline/audio_types.h>
#include <components/bk_audio/audio_pipeline/audio_port_info_list.h>
#include <driver/i2s.h>
#include <driver/i2s_types.h>


#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief   I2S Stream configurations
 */
typedef struct
{
    i2s_gpio_group_id_t     gpio_group;       /*!< I2S gpio group id */
    i2s_role_t              role;             /*!< I2S role: master/slave */
    i2s_work_mode_t         work_mode;        /*!< I2S work mode */
    i2s_lrck_invert_en_t    lrck_invert;      /*!< I2S lrck invert enable/disable */
    i2s_sck_invert_en_t     sck_invert;       /*!< I2S sck invert enable/disable */
    i2s_lsb_first_en_t      lsb_first_en;     /*!< I2S lsb first enable/disable */
    uint32_t                sync_length;      /*!< I2S sync length */
    uint32_t                data_length;      /*!< I2S data length */
    uint32_t                pcm_dlength;      /*!< I2S pcm D length */
    i2s_lrcom_store_mode_t  store_mode;       /*!< I2S store mode */
    i2s_samp_rate_t         samp_rate;        /*!< I2S sample rate */
    uint8_t                 pcm_chl_num;      /*!< PCM channel number */
    i2s_channel_id_t        channel_id;       /*!< I2S channel id */
    audio_stream_type_t     type;             /*!< Type of stream: READER/WRITER */
    uint32_t                buff_size;        /*!< Ring buffer size (bytes) */
    int                     out_block_size;   /*!< Size of output block */
    int                     out_block_num;    /*!< Number of output block */
    int                     task_stack;       /*!< Task stack size */
    int                     task_core;        /*!< Task running in core (0 or 1) */
    int                     task_prio;        /*!< Task priority (based on freeRTOS priority) */
    int                     multi_in_port_num;   /*!< The number of multiple input audio port */
    int                     multi_out_port_num;  /*!< The number of multiple output audio port */
    uint8_t                 manual_config_gpio_en; /*!< Manual GPIO configuration enable: 0=auto config GPIO, 1=manual config GPIO by application */
} i2s_stream_cfg_t;


#define I2S_STREAM_GPIO_GROUP           (I2S_GPIO_GROUP_2)
#define I2S_STREAM_ROLE                 (I2S_ROLE_MASTER)
#define I2S_STREAM_WORK_MODE            (I2S_WORK_MODE_I2S)
#define I2S_STREAM_SAMP_RATE            (I2S_SAMP_RATE_16000)
#define I2S_STREAM_CHANNEL_ID           (I2S_CHANNEL_1)
#define I2S_STREAM_BUFF_SIZE            (640)
#define I2S_STREAM_BLOCK_SIZE           (640)
#define I2S_STREAM_BLOCK_NUM            (2)
#define I2S_STREAM_TASK_STACK           (2048)
#define I2S_STREAM_TASK_CORE            (1)
#define I2S_STREAM_TASK_PRIO            (BEKEN_DEFAULT_WORKER_PRIORITY - 1)

#define DEFAULT_I2S_STREAM_CONFIG() {                       \
        .gpio_group = I2S_STREAM_GPIO_GROUP,                \
        .role = I2S_STREAM_ROLE,                            \
        .work_mode = I2S_STREAM_WORK_MODE,                  \
        .lrck_invert = I2S_LRCK_INVERT_DISABLE,             \
        .sck_invert = I2S_SCK_INVERT_DISABLE,               \
        .lsb_first_en = I2S_LSB_FIRST_DISABLE,              \
        .sync_length = 0,                                   \
        .data_length = 16,                                  \
        .pcm_dlength = 0,                                   \
        .store_mode = I2S_LRCOM_STORE_16R16L,               \
        .samp_rate = I2S_STREAM_SAMP_RATE,                  \
        .pcm_chl_num = 2,                                   \
        .channel_id = I2S_STREAM_CHANNEL_ID,                \
        .type = AUDIO_STREAM_NONE,                          \
        .buff_size = I2S_STREAM_BUFF_SIZE,                  \
        .out_block_size = I2S_STREAM_BLOCK_SIZE,            \
        .out_block_num = I2S_STREAM_BLOCK_NUM,              \
        .task_stack = I2S_STREAM_TASK_STACK,                \
        .task_core = I2S_STREAM_TASK_CORE,                  \
        .task_prio = I2S_STREAM_TASK_PRIO,                  \
        .multi_in_port_num = 0,                             \
        .multi_out_port_num = 0,                            \
        .manual_config_gpio_en = 0,                         \
    }

/**
 * @brief      Create a handle to an Audio Element to stream data from/to I2S peripheral.
 *
 * @param[in]      config  The configuration
 *
 * @return         The Audio Element handle
 *                 - Not NULL: success
 *                 - NULL: failed
 */
audio_element_handle_t i2s_stream_init(i2s_stream_cfg_t *config);

#if CONFIG_ADK_I2S_STREAM_SUPPORT_MULTIPLE_SOURCE
/**
 * @brief      Get the input port information of the i2s stream based on the port ID.
 *
 * @param[in]      i2s_stream  The element handle of the i2s stream
 * @param[in]      port_id  Valid audio port ID (0: element->in, >=1: element->multi_in)
 * @param[out]     port_info  Pointer to the structure used to store the obtained audio port information pointer
 *
 * @return         Result
 *                 - BK_OK: Success
 *                 - Others: Failure
 */
bk_err_t i2s_stream_get_input_port_info_by_port_id(audio_element_handle_t i2s_stream, uint8_t port_id, audio_port_info_t **port_info);

/**
 * @brief      Set input audio port info.
 * @note       Do not call set_input_port_info in audio_port_info_t->notify_cb. 
 *             Because the callback function is called in the context of the audio port, and the mutex lock is not allowed to be used in the callback function.
 *
 * @param[in]      i2s_stream  element handle
 * @param[in]      port_info  audio port info
 *
 * @return         Result
 *                 - BK_OK: success
 *                 - other: failed
 */
bk_err_t i2s_stream_set_input_port_info(audio_element_handle_t i2s_stream, audio_port_info_t *port_info);
#endif

#ifdef __cplusplus
}
#endif

#endif

