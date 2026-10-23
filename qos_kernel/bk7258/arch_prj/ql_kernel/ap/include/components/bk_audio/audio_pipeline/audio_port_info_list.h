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

#ifndef _AUDIO_PORT_INFO_LIST_H_
#define _AUDIO_PORT_INFO_LIST_H_

#include <components/bk_audio/audio_pipeline/bsd_queue.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Audio port state of multiple input audio port
 */
typedef enum
{
    APT_STATE_RUNNING       = 0,                /*!< audio port running, onboard speaker play audio port data */
    APT_STATE_PAUSED        = 1,                /*!< audio port paused, onboard speaker pause play audio port data */
    APT_STATE_FINISHED      = 2,                /*!< audio port finished, onboard speaker play audio port data finish */
} audio_port_state_t;

/**
 * @brief Audio port state notify callback function
 *
 * @param[in] state       The state of audio port
 * @param[in] port_info   Pointer to the audio port info
 * @param[in] user_data   User data pointer
 *
 * @return
 *     - 0: Success
 *     - Others: Failed
 */
typedef int (*audio_port_state_notify)(int state, void *port_info, void *user_data);

/**
 * @brief Audio port information
 */
typedef struct audio_port_info
{
    uint8_t                 chl_num;            /*!< speaker channel number */
    uint32_t                sample_rate;        /*!< speaker sample rate */
    int32_t                 dig_gain;           /*!< audio dac digital gain: value range: 0x00 ~ 0x3f(-45db ~ 18db, 0x2d: 0db), suggest: 0x2d */
    int32_t                 ana_gain;           /*!< audio dac analog gain: value range: , suggest: */
    uint8_t                 bits;               /*!< Bit wide (8, 16, 24, 32 bits) */
    uint8_t                 port_id;            /*!< the valid audio port of currently reading speaker data, 0: element->in, >=1: element->multi_in */
    uint8_t                 priority;           /*!< the priority of the audio port. The lower the value, the higher the priority to be processed. The default value is 0 (highest priority). */
    audio_port_handle_t     port;               /*!< the audio port handle */
    audio_port_state_notify notify_cb;          /*!< the audio port state notify callback function */
    void                    *user_data;         /*!< the user data of audio port state notify callback function */
    bool                    port_data_valid;    /*!< port data validity flag, true: valid (data can be used for playback), false: invalid (data will be replaced with silence) */
} audio_port_info_t;

#define DEFAULT_AUDIO_PORT_INFO() {     \
    .chl_num = 1,                       \
    .sample_rate = 8000,                \
    .dig_gain = 0x2d,                   \
    .ana_gain = 0x0A,                   \
    .bits = 16,                         \
    .port_id = 0,                       \
    .priority = 0,                      \
    .port = NULL,                       \
    .notify_cb = NULL,                  \
    .user_data = NULL,                  \
    .port_data_valid = true,            \
}

/**
 * @brief Input audio port info item
 */
typedef struct input_audio_port_info_item
{
    STAILQ_ENTRY(input_audio_port_info_item)    next;       /*!< next item in list */
    audio_port_info_t                           port_info;  /*!< audio port information */
} input_audio_port_info_item_t;

/**
 * @brief Input audio port info list type
 */
typedef STAILQ_HEAD(input_audio_port_info_list, input_audio_port_info_item) input_audio_port_info_list_t;

/**
 * @brief Initialize the input audio port info list
 *
 * @param[in] input_port_list  Pointer to the input audio port info list
 *
 * @return
 *     - BK_OK: Success
 *     - BK_FAIL: Failed
 */
bk_err_t audio_port_info_list_init(input_audio_port_info_list_t *input_port_list);

/**
 * @brief Debug print input audio port list
 *
 * @param[in] input_port_list  Pointer to the input audio port info list
 * @param[in] func             Function name for debug
 * @param[in] line             Line number for debug
 *
 * @return None
 */
void audio_port_info_list_debug_print(input_audio_port_info_list_t *input_port_list, const char *func, int line);

/**
 * @brief Traverse the input_audio_port_info_list according to port priority to obtain the high-priority port id with valid data
 *
 * @param[in] input_port_list  Pointer to the input audio port info list
 *
 * @return
 *     - >=0: Valid port id
 *     - -1: No valid port found
 */
int8_t audio_port_info_list_get_valid_port_id(input_audio_port_info_list_t *input_port_list);

/**
 * @brief Get audio port info by port id
 *
 * @param[in] input_port_list  Pointer to the input audio port info list
 * @param[in] input_port_id    The port id to search for
 *
 * @return
 *     - Not NULL: Pointer to the audio port info
 *     - NULL: Port not found
 */
audio_port_info_t *audio_port_info_list_get_by_port_id(input_audio_port_info_list_t *input_port_list, uint8_t input_port_id);

/**
 * @brief Add audio port to list in priority order
 *
 * @param[in] input_port_list  Pointer to the input audio port info list
 * @param[in] port_info        Pointer to the audio port info to add
 *
 * @return
 *     - BK_OK: Success
 *     - BK_FAIL: Failed
 */
bk_err_t audio_port_info_list_add(input_audio_port_info_list_t *input_port_list, audio_port_info_t *port_info);

/**
 * @brief Update audio port in list, if port is NULL, remove this port from list
 *
 * @param[in] input_port_list  Pointer to the input audio port info list
 * @param[in] port_info        Pointer to the audio port info to update
 *
 * @return
 *     - BK_OK: Success
 *     - BK_FAIL: Failed
 */
bk_err_t audio_port_info_list_update(input_audio_port_info_list_t *input_port_list, audio_port_info_t *port_info);

/**
 * @brief Clear and free all nodes in the input audio port info list
 *
 * @param[in] input_port_list  Pointer to the input audio port info list
 *
 * @return
 *     - BK_OK: Success
 *     - BK_FAIL: Failed
 */
bk_err_t audio_port_info_list_clear(input_audio_port_info_list_t *input_port_list);

#ifdef __cplusplus
}
#endif

#endif /* _AUDIO_PORT_INFO_LIST_H_ */

