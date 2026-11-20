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
#include "components/avdk_utils/avdk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

// File list node
typedef struct video_player_file_node_s
{
    char *file_path;
    struct video_player_file_node_s *next;
    struct video_player_file_node_s *prev;
} video_player_file_node_t;

// File list structure
typedef struct video_player_file_list_s
{
    video_player_file_node_t *head;
    video_player_file_node_t *tail;
    video_player_file_node_t *current;
    beken_mutex_t mutex;
} video_player_file_list_t;

/**
 * @brief Initialize file list
 *
 * @param list File list pointer
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t file_list_init(video_player_file_list_t *list);

/**
 * @brief Deinitialize file list
 *
 * @param list File list pointer
 */
void file_list_deinit(video_player_file_list_t *list);

/**
 * @brief Add file to list
 *
 * @param list File list pointer
 * @param file_path File path to add
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t file_list_add(video_player_file_list_t *list, const char *file_path);

/**
 * @brief Remove file from list
 *
 * @param list File list pointer
 * @param file_path File path to remove
 * @return avdk_err_t AVDK_ERR_OK on success, AVDK_ERR_NODEV if not found
 */
avdk_err_t file_list_remove(video_player_file_list_t *list, const char *file_path);

/**
 * @brief Clear all files from list
 *
 * @param list File list pointer
 */
void file_list_clear(video_player_file_list_t *list);

/**
 * @brief Find next file in list (with loop)
 *
 * @param list File list pointer
 * @return video_player_file_node_t* Next file node, NULL if list is empty
 */
video_player_file_node_t *file_list_find_next(video_player_file_list_t *list);

/**
 * @brief Find previous file in list (with loop)
 *
 * @param list File list pointer
 * @return video_player_file_node_t* Previous file node, NULL if list is empty
 */
video_player_file_node_t *file_list_find_prev(video_player_file_list_t *list);

/**
 * @brief Set current file in list
 *
 * @param list File list pointer
 * @param file_path File path to set as current
 * @return avdk_err_t AVDK_ERR_OK on success, AVDK_ERR_NODEV if not found
 */
avdk_err_t file_list_set_current(video_player_file_list_t *list, const char *file_path);

/**
 * @brief Get current file in list
 *
 * @param list File list pointer
 * @return video_player_file_node_t* Current file node, NULL if not set
 */
video_player_file_node_t *file_list_get_current(video_player_file_list_t *list);

/**
 * @brief Check if file exists and size is not zero
 *
 * @param file_path File path to check
 * @return avdk_err_t AVDK_ERR_OK if valid, error code on failure
 */
avdk_err_t file_list_check_file_valid(const char *file_path);

#ifdef __cplusplus
}
#endif

