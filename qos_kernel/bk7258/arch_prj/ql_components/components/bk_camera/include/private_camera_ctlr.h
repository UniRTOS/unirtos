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

#include <components/bk_camera_ctlr_types.h>

/**
 * @brief Camera state enumeration
 * 
 * This enumeration defines the possible states of a camera controller.
 */
typedef enum
{
    CAMERA_STATE_INIT = 0,    /**< Camera is initialized but not opened */
    CAMERA_STATE_OPENED,      /**< Camera is opened and ready for use */
} camera_state_t;

/**
 * @brief Private DVP camera controller structure
 * 
 * This structure represents the internal state of a DVP camera controller,
 * including its state, configuration, and operations.
 */
typedef struct
{
    camera_state_t state;              /**< Current state of the camera */
    bk_dvp_ctlr_config_t config;       /**< DVP camera configuration */
    bk_camera_ctlr_t ops;              /**< Camera operations interface */
    void *handle;                      /**< Handle to the DVP controller */
    uint8_t *encode_buffer;            /**< Encode buffer */
}  private_camera_dvp_ctlr_t;

/**
 * @brief Private UVC camera controller structure
 * 
 * This structure represents the internal state of a UVC camera controller,
 * including its state, configuration, and operations.
 */
typedef struct
{
    camera_state_t state;              /**< Current state of the camera */
    bk_uvc_ctlr_config_t config; /**< UVC camera configuration */
    bk_camera_ctlr_t ops;              /**< Camera operations interface */
    void *handle;                      /**< Handle to the UVC controller */
} private_camera_uvc_ctlr_t;