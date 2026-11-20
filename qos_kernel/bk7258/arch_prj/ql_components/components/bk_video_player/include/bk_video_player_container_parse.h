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
#include <common/bk_typedef.h>
#include "components/avdk_utils/avdk_types.h"
#include "bk_video_player_ctlr.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Container audio parse thread function (pipeline stage 1 for audio)
 *
 * @param arg Controller pointer
 */
void bk_video_player_container_audio_parse_thread(void *arg);

/**
 * @brief Initialize container parse resources (threads and semaphores)
 *
 * @param controller Controller pointer
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_container_parse_init(private_video_player_ctlr_t *controller);

/**
 * @brief Deinitialize container parse resources (threads and semaphores)
 *
 * @param controller Controller pointer
 */
void bk_video_player_container_parse_deinit(private_video_player_ctlr_t *controller);

/**
 * @brief Add container parser to parser list
 *
 * @param controller Controller pointer
 * @param parser_ops Container parser operations
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_container_parser_list_add(private_video_player_ctlr_t *controller, video_player_container_parser_ops_t *parser_ops);

/**
 * @brief Clear all registered container parsers
 *
 * This API releases all container parser registration nodes in controller->container_parser_list.
 * It only frees the list nodes; it does NOT deinit/destroy any active parser instance, and it
 * does NOT touch ops templates.
 *
 * @param controller Controller pointer
 */
void bk_video_player_container_parser_list_clear(private_video_player_ctlr_t *controller);

#ifdef __cplusplus
}
#endif

