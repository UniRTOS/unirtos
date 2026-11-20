// Copyright 205-2026 Beken
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

#include <components/bk_camera_ctlr_types.h>


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create a new UVC camera controller
 * 
 * This function creates and initializes a new UVC camera controller.
 * 
 * @param handle Pointer to store the created controller handle
 * @param config Pointer to the UVC controller configuration
 * @return AVDK error code
 */
avdk_err_t bk_camera_uvc_ctlr_new(bk_camera_ctlr_handle_t *handle, bk_uvc_ctlr_config_t *config);

/**
 * @brief Create a new DVP camera controller
 * 
 * This function creates and initializes a new DVP camera controller.
 * 
 * @param handle Pointer to store the created controller handle
 * @param config Pointer to the DVP controller configuration
 * @return AVDK error code
 */
avdk_err_t bk_camera_dvp_ctlr_new(bk_camera_ctlr_handle_t *handle, bk_dvp_ctlr_config_t *config);

/**
 * @brief Open a camera controller
 * 
 * This function opens the camera associated with the given controller handle.
 * 
 * @param handle Handle to the camera controller
 * @return AVDK error code
 */
avdk_err_t bk_camera_open(bk_camera_ctlr_handle_t handle);

/**
 * @brief Close a camera controller
 * 
 * This function closes the camera associated with the given controller handle.
 * 
 * @param handle Handle to the camera controller
 * @return AVDK error code
 */
avdk_err_t bk_camera_close(bk_camera_ctlr_handle_t handle);

/**
 * @brief Suspend a camera controller
 * 
 * This function suspends the camera associated with the given controller handle.
 * 
 * @param handle Handle to the camera controller
 * @return AVDK error code
 */
avdk_err_t bk_camera_suspend(bk_camera_ctlr_handle_t handle);

/**
 * @brief Resume a camera controller
 * 
 * This function resumes the camera associated with the given controller handle.
 * 
 * @param handle Handle to the camera controller
 * @return AVDK error code
 */
avdk_err_t bk_camera_resume(bk_camera_ctlr_handle_t handle);

/**
 * @brief Perform an IOCTL operation on a camera controller
 * 
 * This function performs a device-specific IOCTL operation on the camera.
 * 
 * @param handle Handle to the camera controller
 * @param cmd Command to execute
 * @param arg Argument for the command
 * @return AVDK error code
 */
avdk_err_t bk_camera_ioctl(bk_camera_ctlr_handle_t handle, uint32_t cmd, void *arg);

/**
 * @brief Delete a camera controller
 * 
 * This function deletes and cleans up the given camera controller.
 * 
 * @param handle Handle to the camera controller
 * @return AVDK error code
 */
avdk_err_t bk_camera_delete(bk_camera_ctlr_handle_t handle);


#ifdef __cplusplus
}
#endif