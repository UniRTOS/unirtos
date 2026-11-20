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

#include <components/avdk_utils/avdk_types.h>
#include <components/avdk_utils/avdk_check.h>
#include <components/avdk_utils/avdk_error.h>
#include "driver/dma2d_types.h"

#ifdef __cplusplus
extern "C" {
#endif


/*******************************************************************
 * component name: bk_dma2d
 * description: Public API (open interface)
 *******************************************************************/

typedef enum {
    DMA2D_TRANSFER_COMPLETE = 0,
    DMA2D_TRANSFER_ERROR,
    DMA2D_CONFIG_ERROR,
} dma2d_trans_status_t;

typedef struct
{
    dma2d_fill_t fill;              /**< dma2d fill config */
    void (*transfer_complete_cb)(dma2d_trans_status_t status, void *user_data); /**< dma2d transfer complete callback */
    bool is_sync;            /**< true: dma2d config valid immediately or wait for dma2d execute finish if current in use,
                                 false: dma2d config valid after dma2d execute finish if dma2d current is in use*/
    void *user_data;
}dma2d_fill_config_t;

typedef struct
{
    dma2d_memcpy_pfc_t memcpy;      /**< dma2d memcpy config */
    void (*transfer_complete_cb)(dma2d_trans_status_t status, void *user_data); /**< dma2d transfer complete callback */
    bool is_sync;                  /**< true: dma2d config valid immediately or wait for dma2d execute finish if current in use,
                                        false: dma2d config valid after dma2d execute finish if dma2d current is in use*/
    void *user_data;
}dma2d_memcpy_config_t;

typedef struct
{
    dma2d_memcpy_pfc_t pfc;        /**< dma2d pfc memcpy config */
    void (*transfer_complete_cb)(dma2d_trans_status_t status, void *user_data); /**< dma2d transfer complete callback */
    bool is_sync;                  /**< true: dma2d config valid immediately or wait for dma2d execute finish if current in use,
                                        false: dma2d config valid after dma2d execute finish if dma2d current is in use*/
    void *user_data;
}dma2d_pfc_memcpy_config_t;

typedef struct
{
    dma2d_offset_blend_t blend;     /**< dma2d blend config */
    void (*transfer_complete_cb)(dma2d_trans_status_t status, void *user_data); /**< dma2d transfer complete callback */
    bool is_sync;                   /**< true: dma2d config valid immediately or wait for dma2d execute finish if current in use,
                                        false: dma2d config valid after dma2d execute finish if dma2d current is in use*/
    void *user_data;
}dma2d_blend_config_t;

typedef enum {
    DMA2D_IOCTL_SET_SWRESRT,        /**< Set software reset */
    DMA2D_IOCTL_SUSPEND,            /**< Suspend DMA2D */
    DMA2D_IOCTL_RESUME,             /**< Resume DMA2D */
    DMA2D_IOCTL_TRANS_ABORT,        /**< Abort DMA2D transfer */
}dma2d_ioctl_cmd;

typedef struct bk_dma2d_ctlr *bk_dma2d_ctlr_handle_t;

typedef struct bk_dma2d_ctlr
{
    avdk_err_t (*open)(bk_dma2d_ctlr_handle_t controller);
    avdk_err_t (*fill)(bk_dma2d_ctlr_handle_t controller, dma2d_fill_config_t *config);
    avdk_err_t (*memcpy)(bk_dma2d_ctlr_handle_t controller, dma2d_memcpy_config_t *config);
    avdk_err_t (*pfc_memcpy)(bk_dma2d_ctlr_handle_t controller, dma2d_pfc_memcpy_config_t *config);
    avdk_err_t (*blend)(bk_dma2d_ctlr_handle_t controller, dma2d_blend_config_t *config);
    avdk_err_t (*delete)(bk_dma2d_ctlr_handle_t controller);
    avdk_err_t (*ioctl)(bk_dma2d_ctlr_handle_t controller, uint32_t ioctl_cmd, uint32_t param1, uint32_t param2, uint32_t param3);
    avdk_err_t (*close)(bk_dma2d_ctlr_handle_t controller);
}bk_dma2d_ctlr_t;

#ifdef __cplusplus
}
#endif
