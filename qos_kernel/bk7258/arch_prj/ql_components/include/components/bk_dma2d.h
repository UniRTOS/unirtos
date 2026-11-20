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

#include "components/bk_dma2d_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************
 * component name: bk_dma2d
 * description: Public API (open interface)
 *******************************************************************/


/**
 * @brief Open a DMA2D controller handle
 * @param handle Handle to open
 * @return AVDK_ERR_OK if successful, otherwise error code
 */
avdk_err_t bk_dma2d_open(bk_dma2d_ctlr_handle_t handle);

/**
 * @brief Close a DMA2D controller handle
 * @param handle Handle to close
 * @return AVDK_ERR_OK if successful, otherwise error code
 */
avdk_err_t bk_dma2d_close(bk_dma2d_ctlr_handle_t handle);


/**
 * @brief Delete a DMA2D controller handle
 * @param handle Handle to delete
 * @return AVDK_ERR_OK if successful, otherwise error code
 */
avdk_err_t bk_dma2d_delete(bk_dma2d_ctlr_handle_t handle);

/**
 * @brief Perform fill operation using DMA2D
 * @param handle DMA2D controller handle
 * @param config Fill configuration
 * @return AVDK_ERR_OK if successful, otherwise error code
 */
avdk_err_t bk_dma2d_fill(bk_dma2d_ctlr_handle_t handle, dma2d_fill_config_t *config);


/**
 * @brief Perform memory copy operation using DMA2D
 * @param handle DMA2D controller handle
 * @param config Memory copy configuration
 * @return AVDK_ERR_OK if successful, otherwise error code
 */
avdk_err_t bk_dma2d_memcpy(bk_dma2d_ctlr_handle_t handle, dma2d_memcpy_config_t *config);

/**
 * @brief Perform pixel format conversion operation using DMA2D
 * @param handle DMA2D controller handle
 * @param config Pixel conversion configuration
 * @return AVDK_ERR_OK if successful, otherwise error code
 */
avdk_err_t bk_dma2d_pixel_conversion(bk_dma2d_ctlr_handle_t handle, dma2d_pfc_memcpy_config_t *config);

/**
 * @brief Perform blend operation using DMA2D
 * @param handle DMA2D controller handle
 * @param config Blend configuration
 * @return AVDK_ERR_OK if successful, otherwise error code
 */
avdk_err_t bk_dma2d_blend(bk_dma2d_ctlr_handle_t handle, dma2d_blend_config_t *config);


/**
 * @brief Control IO operations of DMA2D controller
 * @param handle DMA2D controller handle
 * @param ioctl_cmd IO control command
 * @param param1 Command parameter 1
 * @param param2 Command parameter 2
 * @param param3 Command parameter 3
 * @return AVDK_ERR_OK if successful, otherwise error code
 */
avdk_err_t bk_dma2d_ioctl(bk_dma2d_ctlr_handle_t handle, uint32_t ioctl_cmd, uint32_t param1, uint32_t param2, uint32_t param3);

/**
 * @brief Create a DMA2D controller handle
 * @param handle Handle to create
 * @return AVDK_ERR_OK if successful, otherwise error code
 */
avdk_err_t bk_dma2d_new(bk_dma2d_ctlr_handle_t *handle);

/**
 * @brief get dma2d handle
 * @return bk_dma2d_ctlr_handle_t handle
 */
bk_dma2d_ctlr_handle_t bk_dma2d_handle_get(void);
#ifdef __cplusplus
}
#endif