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

#include "bk_video_recorder_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create a new video recorder
 *
 * @param handle Video recorder handle
 * @param config Video recorder configuration
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_recorder_new(bk_video_recorder_handle_t *handle, bk_video_recorder_config_t *config);

/**
 * @brief Open the video recorder
 *
 * @param handler Video recorder handle
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_recorder_open(bk_video_recorder_handle_t handler);

/**
 * @brief Close the video recorder
 *
 * @param handler Video recorder handle
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_recorder_close(bk_video_recorder_handle_t handler);

/**
 * @brief Start the video recording
 *
 * @param handler Video recorder handle
 * @param file_path File path to record
 * @param record_type Record type
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_recorder_start(bk_video_recorder_handle_t handler, char *file_path, uint32_t record_type);

/**
 * @brief Stop the video recording
 *
 * @param handler Video recorder handle
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_recorder_stop(bk_video_recorder_handle_t handler);

/**
 * @brief Delete the video recorder
 *
 * @param handler Video recorder handle
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_recorder_delete(bk_video_recorder_handle_t handler);

/**
 * @brief IO control for the video recorder
 *
 * @param handler Video recorder handle
 * @param cmd IO control command
 * @param param IO control parameter
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_recorder_ioctl(bk_video_recorder_handle_t handler, bk_video_recorder_ioctl_cmd_t cmd, void *param);

#ifdef __cplusplus
}
#endif
