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

#include <stdint.h> 
#include <os/os.h>
#include "components/bk_dma2d.h"
#include "bk_dma2d_ctlr.h"

#define TAG "bk_dma2d"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)


/**
 * @brief Delete a DMA2D controller handle
 * @param handle Handle to delete
 * @return AVDK_ERR_OK if successful, otherwise error code
 */
avdk_err_t bk_dma2d_delete(bk_dma2d_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->delete, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->delete(handle);
}

/**
 * @brief Perform fill operation using DMA2D
 * @param handle DMA2D controller handle
 * @param config Fill configuration
 * @return AVDK_ERR_OK if successful, otherwise error code
 */
avdk_err_t bk_dma2d_fill(bk_dma2d_ctlr_handle_t handle, dma2d_fill_config_t *config)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(config, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->fill, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->fill(handle, config);
}

/**
 * @brief Perform memory copy operation using DMA2D
 * @param handle DMA2D controller handle
 * @param config Memory copy configuration
 * @return AVDK_ERR_OK if successful, otherwise error code
 */
avdk_err_t bk_dma2d_memcpy(bk_dma2d_ctlr_handle_t handle, dma2d_memcpy_config_t *config)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(config, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->memcpy, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->memcpy(handle, config);
}

/**
 * @brief Perform pixel format conversion operation using DMA2D
 * @param handle DMA2D controller handle
 * @param config Pixel conversion configuration
 * @return AVDK_ERR_OK if successful, otherwise error code
 */
avdk_err_t bk_dma2d_pixel_conversion(bk_dma2d_ctlr_handle_t handle, dma2d_pfc_memcpy_config_t *config)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(config, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->pfc_memcpy, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->pfc_memcpy(handle, config);
}

/**
 * @brief Perform blend operation using DMA2D
 * @param handle DMA2D controller handle
 * @param config Blend configuration
 * @return AVDK_ERR_OK if successful, otherwise error code
 */
avdk_err_t bk_dma2d_blend(bk_dma2d_ctlr_handle_t handle, dma2d_blend_config_t *config)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(config, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->blend, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->blend(handle, config);
}

/**
 * @brief Control IO operations of DMA2D controller
 * @param handle DMA2D controller handle
 * @param ioctl_cmd IO control command
 * @param param1 Command parameter 1
 * @param param2 Command parameter 2
 * @param param3 Command parameter 3
 * @return AVDK_ERR_OK if successful, otherwise error code
 */
avdk_err_t bk_dma2d_ioctl(bk_dma2d_ctlr_handle_t handle, uint32_t ioctl_cmd, uint32_t param1, uint32_t param2, uint32_t param3)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->ioctl, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->ioctl(handle, ioctl_cmd, param1, param2, param3);
}


avdk_err_t bk_dma2d_open(bk_dma2d_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->open, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->open(handle);
}


avdk_err_t bk_dma2d_close(bk_dma2d_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->close, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->close(handle);
}

avdk_err_t bk_dma2d_new(bk_dma2d_ctlr_handle_t *handle)
{
    avdk_err_t ret = AVDK_ERR_OK;
    AVDK_RETURN_ON_FALSE(*handle == NULL, AVDK_ERR_INVAL, TAG, "handle is not NULL\n");
    ret = bk_dma2d_ctlr_new(handle);
    AVDK_RETURN_ON_FALSE(ret == BK_OK, ret, TAG, "new failed\n");
    return ret;
}

bk_dma2d_ctlr_handle_t bk_dma2d_handle_get(void)
{
    return bk_dma2d_ctlr_get();
}