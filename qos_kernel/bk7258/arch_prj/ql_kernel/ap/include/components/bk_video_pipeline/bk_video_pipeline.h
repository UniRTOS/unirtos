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

#include "components/bk_video_pipeline/bk_video_pipeline_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Video Pipeline API
 * 
 * This file provides the external interface for the video pipeline module,
 * which handles various video operations including H264 encoding and image rotation.
 * The video pipeline provides a unified management interface for functions such as 
 * video encoding/decoding and image rotation, simplifying the complexity of 
 * multi-module collaborative work.
 */

/**
 * @brief Create a new video pipeline instance
 *
 * This function creates and initializes a new video pipeline instance with the specified configuration.
 *
 * @param[out] handle Pointer to store the video pipeline handle
 * @param[in] config Video pipeline configuration parameters
 * @return 
 *  - BK_OK: Success
 *  - Others: Fail
 */
avdk_err_t bk_video_pipeline_new(bk_video_pipeline_handle_t *handle, bk_video_pipeline_config_t *config);

/**
 * @brief Open H.264 encoder
 *
 * This function opens and configures the H.264 encoder module in the video pipeline.
 *
 * @param[in] handle Video pipeline handle
 * @param[in] config Video pipeline configuration parameters for H.264 encoding
 * @return 
 *  - BK_OK: Success
 *  - Others: Fail
 */
avdk_err_t bk_video_pipeline_open_h264e(bk_video_pipeline_handle_t handle, bk_video_pipeline_h264e_config_t *config);

/**
 * @brief Close H.264 encoder
 *
 * This function closes the H.264 encoder module in the video pipeline.
 *
 * @param[in] handle Video pipeline handle
 * @return 
 *  - BK_OK: Success
 *  - Others: Fail
 */
avdk_err_t bk_video_pipeline_close_h264e(bk_video_pipeline_handle_t handle);

/**
 * @brief Open image rotation module
 *
 * This function opens and configures the image rotation module in the video pipeline.
 *
 * @param[in] handle Video pipeline handle
 * @param[in] config Video pipeline configuration parameters for image rotation
 * @return 
 *  - BK_OK: Success
 *  - Others: Fail
 */
avdk_err_t bk_video_pipeline_open_rotate(bk_video_pipeline_handle_t handle, bk_video_pipeline_decode_config_t *config);

/**
 * @brief Close image rotation module
 *
 * This function closes the image rotation module in the video pipeline.
 *
 * @param[in] handle Video pipeline handle
 * @return 
 *  - BK_OK: Success
 *  - Others: Fail
 */
avdk_err_t bk_video_pipeline_close_rotate(bk_video_pipeline_handle_t handle);

/**
 * @brief Get the status of a video pipeline module
 *
 * This function retrieves the enable status of a specified module in the video pipeline.
 *
 * @param[in] handler Video pipeline handle
 * @param[in] module Module to query (H264E or ROTATE)
 * @param[out] status Pointer to store the module status
 * @return 
 *  - BK_OK: Success
 *  - Others: Fail
 */
avdk_err_t bk_video_pipeline_get_module_status(bk_video_pipeline_handle_t handler, video_pipeline_module_t module, video_pipeline_module_status_t *status);

/**
 * @brief Reset the decode module to re-detect image format
 * 
 * This function is used when switching cameras to internally re-acquire the image format.
 * It resets the decode module in the video pipeline.
 *
 * @param[in] handler Video pipeline handle
 * @return 
 *  - BK_OK: Success
 *  - Others: Fail
 */
avdk_err_t bk_video_pipeline_reset_decode(bk_video_pipeline_handle_t handler);

/**
 * @brief Delete the video pipeline instance
 *
 * This function deletes and frees resources associated with a video pipeline instance.
 *
 * @param[in] handle Video pipeline handle
 * @return 
 *  - BK_OK: Success
 *  - Others: Fail
 */
avdk_err_t bk_video_pipeline_delete(bk_video_pipeline_handle_t handle);

/**
 * @brief Extended interface for video pipeline
 *        Used for additional functionalities like getting encoder status
 *
 * This function provides an extended interface for additional functionalities in the video pipeline.
 *
 * @param[in] handle Video pipeline handle
 * @param[in] cmd Command code (defined in pipeline_ext_cmd_t)
 * @param[in,out] param Command parameters
 * @return 
 *  - BK_OK: Success
 *  - Others: Fail
 */
avdk_err_t bk_video_pipeline_ioctl(bk_video_pipeline_handle_t handle, video_pipeline_ioctl_cmd_t cmd, void *param);

#ifdef __cplusplus
}
#endif