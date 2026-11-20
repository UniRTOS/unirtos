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
#include "components/avdk_utils/avdk_types.h"
#include "components/avdk_utils/avdk_check.h"
#include "mux_pipeline.h"
#include "uvc_pipeline_act.h"
#include "components/media_types.h"
#include "yuv_encode.h"
#include "components/bk_jpeg_decode/bk_jpeg_decode_hw.h"
#include "bk_jpeg_decode_ctlr.h"
#include "hw_jpeg_decode_opt.h"

#define TAG "hw_jdec_opt"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define YUV_PIXEL_BYTES 2  // YUYV format uses 2 bytes per pixel
#define HW_JPEG_DECODE_WIDTH_ALIGN 32

#define DEFAULT_IMAGE_MAX_WIDTH (864)
#define DEFAULT_LINES_PER_BLOCK (JPEG_DECODE_OPT_LINES_PER_BLOCK_8)

static bool hardware_jpeg_decode_opt_is_width_valid(uint32_t width)
{
    return (width % HW_JPEG_DECODE_WIDTH_ALIGN) == 0;
}

// Open hardware optimized decoder
static avdk_err_t hardware_jpeg_decode_opt_ctlr_open(bk_jpeg_decode_hw_ctlr_handle_t handler)
{
    avdk_err_t ret = AVDK_ERR_OK;
    private_jpeg_decode_hw_opt_ctlr_t *controller = __containerof(handler, private_jpeg_decode_hw_opt_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(controller->module_status.status == JPEG_DECODE_DISABLED, AVDK_ERR_INVAL, TAG, "jpeg decode is enabled");

    if (controller->config.image_max_width == 0)
    {
        controller->config.image_max_width = DEFAULT_IMAGE_MAX_WIDTH;
        LOGI("%s %d image_max_width is 0, use default value %d\n", __func__, __LINE__, DEFAULT_IMAGE_MAX_WIDTH);
    }

    uint8_t * sram_buffer = NULL;
    if (controller->config.sram_buffer == NULL)
    {
        if (controller->config.is_pingpong)
        {
            sram_buffer = (uint8_t *)os_malloc(controller->config.image_max_width * controller->config.lines_per_block * YUV_PIXEL_BYTES * 2);
        }
        else
        {
            sram_buffer = (uint8_t *)os_malloc(controller->config.image_max_width * controller->config.lines_per_block * YUV_PIXEL_BYTES);
        }
        if (sram_buffer == NULL)
        {
            LOGE("%s %d malloc sram buffer failed\n", __func__, __LINE__);
            return AVDK_ERR_NOMEM;
        }
        controller->sram_buffer_need_free = 1;
        controller->config.sram_buffer = sram_buffer;
    }

    hw_opt_decode_init_config_t init_config = {
        .decode_cbs = &controller->config.decode_cbs,
        .sram_buffer = controller->config.sram_buffer,
        .lines_per_block = controller->config.lines_per_block,
        .is_pingpong = controller->config.is_pingpong,
        .copy_method = controller->config.copy_method,
    };
    ret = hw_jpeg_decode_opt_init(&init_config);
    AVDK_RETURN_ON_FALSE(ret == AVDK_ERR_OK, AVDK_ERR_INVAL, TAG, "hw_decode_opt_init failed");
    
    controller->module_status.status = JPEG_DECODE_ENABLED;

    return ret;
}

// Close hardware optimized decoder
static avdk_err_t hardware_jpeg_decode_opt_ctlr_close(bk_jpeg_decode_hw_ctlr_handle_t handler)
{
    avdk_err_t ret = AVDK_ERR_OK;
    private_jpeg_decode_hw_opt_ctlr_t *controller = __containerof(handler, private_jpeg_decode_hw_opt_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(controller->module_status.status == JPEG_DECODE_ENABLED, AVDK_ERR_INVAL, TAG, "jpeg decode is disabled");

    ret = hw_jpeg_decode_opt_deinit();
    AVDK_RETURN_ON_FALSE(ret == AVDK_ERR_OK, AVDK_ERR_INVAL, TAG, "hw_jpeg_decode_opt_deinit failed");

    controller->module_status.status = JPEG_DECODE_DISABLED;

    return ret;
}

// Get image info
static avdk_err_t hardware_jpeg_decode_opt_ctlr_get_img_info(bk_jpeg_decode_hw_ctlr_handle_t handler, bk_jpeg_decode_img_info_t *info)
{
    private_jpeg_decode_hw_opt_ctlr_t *controller = __containerof(handler, private_jpeg_decode_hw_opt_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(info, AVDK_ERR_INVAL, TAG, "info is NULL");
    AVDK_RETURN_ON_FALSE(info->frame, AVDK_ERR_INVAL, TAG, "info frame is NULL");

    return bk_get_jpeg_data_info(info);
}

// Decode function with optimized support
static avdk_err_t hardware_jpeg_decode_opt_ctlr_decode(bk_jpeg_decode_hw_ctlr_handle_t handler, frame_buffer_t *in_frame, frame_buffer_t *out_frame)
{
    avdk_err_t ret = BK_OK;
    private_jpeg_decode_hw_opt_ctlr_t *controller = __containerof(handler, private_jpeg_decode_hw_opt_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(controller->module_status.status == JPEG_DECODE_ENABLED, AVDK_ERR_INVAL, TAG, "jpeg decode is disabled");
    AVDK_RETURN_ON_FALSE(in_frame, AVDK_ERR_INVAL, TAG, "in_frame is NULL");
    AVDK_RETURN_ON_FALSE(out_frame, AVDK_ERR_INVAL, TAG, "out_frame is NULL");
    AVDK_RETURN_ON_FALSE(in_frame->frame, AVDK_ERR_INVAL, TAG, "in_frame frame is NULL");
    AVDK_RETURN_ON_FALSE(out_frame->frame, AVDK_ERR_INVAL, TAG, "out_frame frame is NULL");
    AVDK_RETURN_ON_FALSE(in_frame->length > 0, AVDK_ERR_INVAL, TAG, "in_frame length is 0");
    AVDK_RETURN_ON_FALSE(out_frame->size > 0, AVDK_ERR_INVAL, TAG, "out_frame size is 0");

    bk_jpeg_decode_img_info_t img_info = {0};
    img_info.frame = in_frame;
    ret = bk_get_jpeg_data_info(&img_info);
    if (ret != AVDK_ERR_OK)
    {
        LOGE(" %s %d bk_get_jpeg_data_info failed %d\n", __func__, __LINE__, ret);
        return AVDK_ERR_INVAL;
    }
    if (!hardware_jpeg_decode_opt_is_width_valid(img_info.width))
    {
        LOGE(" %s %d hardware jpeg decode requires width %u to be multiple of %u\n",
             __func__, __LINE__, img_info.width, HW_JPEG_DECODE_WIDTH_ALIGN);
        return AVDK_ERR_INVAL;
    }

    if (controller->config.image_max_width < img_info.width)
    {
        LOGE(" %s %d image_max_width is not enough\n", __func__, __LINE__);
        return AVDK_ERR_INVAL;
    }

    if (img_info.width * img_info.height * YUV_PIXEL_BYTES > out_frame->size)
    {
        LOGE(" %s %d out_frame size is not enough\n", __func__, __LINE__);
        if (controller->config.decode_cbs.in_complete)
        {
            controller->config.decode_cbs.in_complete(in_frame);
        }
        return AVDK_ERR_INVAL;
    }

    // Set the image dimensions from parsed JPEG info to input and output frame buffers
    in_frame->width = img_info.width;
    in_frame->height = img_info.height;
    out_frame->width = img_info.width;
    out_frame->height = img_info.height;

    // Perform optimized decoding with configured lines_per_block
    ret = hw_jpeg_decode_opt_start(in_frame, out_frame);
    if (ret != BK_OK)
    {
        LOGE(" %s %d hw_jpeg_decode_opt_start failed %d\n", __func__, __LINE__, ret);
        if (controller->config.decode_cbs.in_complete)
        {
            controller->config.decode_cbs.in_complete(in_frame);
        }
        if (controller->config.decode_cbs.out_complete)
        {
            controller->config.decode_cbs.out_complete(PIXEL_FMT_YUYV, BK_FAIL, out_frame);
        }
        return ret;
    }
    return ret;
}

// Asynchronous decode with optimized support
static avdk_err_t hardware_jpeg_decode_opt_ctlr_decode_async(bk_jpeg_decode_hw_ctlr_handle_t handler, frame_buffer_t *in_frame)
{
    avdk_err_t ret = BK_OK;
    private_jpeg_decode_hw_opt_ctlr_t *controller = __containerof(handler, private_jpeg_decode_hw_opt_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(in_frame, AVDK_ERR_INVAL, TAG, "in_frame is NULL");
    AVDK_RETURN_ON_FALSE(in_frame->frame, AVDK_ERR_INVAL, TAG, "in_frame frame is NULL");
    AVDK_RETURN_ON_FALSE(controller->module_status.status == JPEG_DECODE_ENABLED, AVDK_ERR_INVAL, TAG, "jpeg decode is disabled");
    AVDK_RETURN_ON_FALSE(in_frame->length > 0, AVDK_ERR_INVAL, TAG, "in_frame length is 0");

    bk_jpeg_decode_img_info_t img_info = {0};
    img_info.frame = in_frame;
    ret = bk_get_jpeg_data_info(&img_info);
    if (ret != AVDK_ERR_OK)
    {
        LOGE(" %s %d bk_get_jpeg_data_info failed %d\n", __func__, __LINE__, ret);
        return AVDK_ERR_INVAL;
    }
    if (!hardware_jpeg_decode_opt_is_width_valid(img_info.width))
    {
        LOGE(" %s %d hardware jpeg decode requires width %u to be multiple of %u\n",
             __func__, __LINE__, img_info.width, HW_JPEG_DECODE_WIDTH_ALIGN);
        return AVDK_ERR_INVAL;
    }

    ret = hw_jpeg_decode_opt_start_async(in_frame);
    if (ret != BK_OK)
    {
        LOGE(" %s %d hw_jpeg_decode_opt_start_async failed %d\n", __func__, __LINE__, ret);
        if (controller->config.decode_cbs.in_complete)
        {
            controller->config.decode_cbs.in_complete(in_frame);
        }
        return ret;
    }
    return ret;
}

// Delete controller
static avdk_err_t hardware_jpeg_decode_opt_ctlr_delete(bk_jpeg_decode_hw_ctlr_handle_t handler)
{
    private_jpeg_decode_hw_opt_ctlr_t *controller = __containerof(handler, private_jpeg_decode_hw_opt_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(controller->module_status.status == JPEG_DECODE_DISABLED, AVDK_ERR_INVAL, TAG, "jpeg decode is enabled");

    if (controller->sram_buffer_need_free)
    {
        os_free(controller->config.sram_buffer);
        controller->config.sram_buffer = NULL;
        controller->sram_buffer_need_free = 0;
    }

    os_free(controller);
    return AVDK_ERR_OK;
}

// IOCTL command handler
static avdk_err_t hardware_jpeg_decode_opt_ctlr_ioctl(bk_jpeg_decode_hw_ctlr_handle_t handler, bk_jpeg_decode_hw_ioctl_cmd_t cmd, void *param)
{
    private_jpeg_decode_hw_opt_ctlr_t *controller = __containerof(handler, private_jpeg_decode_hw_opt_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");

    switch (cmd)
    {
    case JPEG_DECODE_HW_IOCTL_CMD_BASE:
    default:
        LOGW("%s unknown cmd: %d\n", __func__, cmd);
        break;
    }

    return AVDK_ERR_OK;
}

// Create hardware optimized decode controller
avdk_err_t bk_hardware_jpeg_decode_opt_ctlr_new(bk_jpeg_decode_hw_ctlr_handle_t *handle, bk_jpeg_decode_hw_opt_config_t *config)
{
    AVDK_RETURN_ON_FALSE(config && handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    if (config->lines_per_block == 0)
    {
        config->lines_per_block = DEFAULT_LINES_PER_BLOCK;
        LOGI("%s %d lines_per_block is 0, use default value %d\n", __func__, __LINE__, DEFAULT_LINES_PER_BLOCK);
    }

    // Set default lines_per_block if not configured
    if (!(config->lines_per_block == JPEG_DECODE_OPT_LINES_PER_BLOCK_8
        || config->lines_per_block == JPEG_DECODE_OPT_LINES_PER_BLOCK_16))
    {
        LOGE("%s lines_per_block must be 8 or 16\n", __func__);
        return AVDK_ERR_INVAL;
    }

    private_jpeg_decode_hw_opt_ctlr_t *controller = os_malloc(sizeof(private_jpeg_decode_hw_opt_ctlr_t));
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);
    os_memset(controller, 0, sizeof(private_jpeg_decode_hw_opt_ctlr_t));

    os_memcpy(&controller->config, config, sizeof(bk_jpeg_decode_hw_opt_config_t));
    
    // If sram_buffer is NULL, try to get default SRAM buffer
    if (controller->config.sram_buffer == NULL)
    {
        LOGW("%s SRAM buffer not provided, will malloc default SRAM buffer\n", __func__);
    }

    controller->ops.open = hardware_jpeg_decode_opt_ctlr_open;
    controller->ops.close = hardware_jpeg_decode_opt_ctlr_close;
    controller->ops.decode = hardware_jpeg_decode_opt_ctlr_decode;
    controller->ops.decode_async = hardware_jpeg_decode_opt_ctlr_decode_async;
    controller->ops.delete = hardware_jpeg_decode_opt_ctlr_delete;
    controller->ops.get_img_info = hardware_jpeg_decode_opt_ctlr_get_img_info;
    controller->ops.ioctl = hardware_jpeg_decode_opt_ctlr_ioctl;

    *handle = &(controller->ops);

    LOGI("%s success, lines_per_block=%d\n", __func__, controller->config.lines_per_block);

    return AVDK_ERR_OK;
}

