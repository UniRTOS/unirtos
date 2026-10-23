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

#include "components/bk_display_types.h"

#ifdef __cplusplus
extern "C" {
#endif


/*******************************************************************
 * component name: bk_display
 * description: Public API (open interface)
 *******************************************************************/

/**
 * @brief Open display controller
 * @param handle Display controller handle
 * @return Operation result, AVDK_ERR_OK indicates success
 */
avdk_err_t bk_display_open(bk_display_ctlr_handle_t handle);

/**
 * @brief Close display controller
 * @param handle Display controller handle
 * @return Operation result, AVDK_ERR_OK indicates success
 */
avdk_err_t bk_display_close(bk_display_ctlr_handle_t handle);

/**
 * @brief Delete display controller
 * @param handle Display controller handle
 * @return Operation result, AVDK_ERR_OK indicates success
 */
avdk_err_t bk_display_delete(bk_display_ctlr_handle_t handle);

/**
 * @brief Flush display content
 * @param handle Display controller handle
 * @param frame Frame data to be displayed
 * @param free_t Frame data release callback function
 * @return Operation result, AVDK_ERR_OK indicates success
 */
avdk_err_t bk_display_flush(bk_display_ctlr_handle_t handle, frame_buffer_t *frame, bk_err_t (*free_t)(void *args));

/**
 * @brief Control IO operations of display controller
 * @param handle Display controller handle
 * @param cmd IO control command
 * @param param Command data
 * @return Operation result, AVDK_ERR_OK indicates success
 */
avdk_err_t bk_display_ioctl(bk_display_ctlr_handle_t handle, uint32_t cmd, void *arg);

/**
 * @brief Create MCU display controller
 * @param handle Output parameter, used to store the created display controller handle
 * @param config MCU display controller configuration parameters
 * @return Operation result, AVDK_ERR_OK indicates success
 */
avdk_err_t bk_display_mcu_new(bk_display_ctlr_handle_t *handle, bk_display_mcu_ctlr_config_t *config);

/**
 * @brief Create RGB display controller
 * @param handle Output parameter, used to store the created display controller handle
 * @param config RGB display controller configuration parameters
 * @return Operation result, AVDK_ERR_OK indicates success
 */
avdk_err_t bk_display_rgb_new(bk_display_ctlr_handle_t *handle, bk_display_rgb_ctlr_config_t *config);

/**
 * @brief Create SPI display controller
 * @param handle Output parameter, used to store the created display controller handle
 * @param config SPI display controller configuration parameters
 * @return Operation result, AVDK_ERR_OK indicates success
 */
avdk_err_t bk_display_spi_new(bk_display_ctlr_handle_t *handle, bk_display_spi_ctlr_config_t *config);

/**
 * @brief Create QSPI display controller
 * @param handle Output parameter, used to store the created display controller handle
 * @param config QSPI display controller configuration parameters
 * @return Operation result, AVDK_ERR_OK indicates success
 */
avdk_err_t bk_display_qspi_new(bk_display_ctlr_handle_t *handle, bk_display_qspi_ctlr_config_t *config);

/**
 * @brief Create Dual QSPI display controller
 * @param handle Output parameter, used to store the created display controller handle
 * @param config Dual QSPI display controller configuration parameters
 * @return Operation result, AVDK_ERR_OK indicates success
 */
avdk_err_t bk_display_dual_qspi_new(bk_display_ctlr_handle_t *handle, bk_display_dual_qspi_ctlr_config_t *config);

/**
 * @brief Create Dual SPI display controller
 * @param handle Output parameter, used to store the created display controller handle
 * @param config Dual SPI display controller configuration parameters
 * @return Operation result, AVDK_ERR_OK indicates success
 */
avdk_err_t bk_display_dual_spi_new(bk_display_ctlr_handle_t *handle, bk_display_dual_spi_ctlr_config_t *config);

#ifdef __cplusplus
}
#endif


