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

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <components/log.h>
#include <driver/pwr_clk.h>
#include <driver/dma.h>
#include <driver/i2c.h>
#include <driver/jpeg_enc.h>
#include <driver/gpio_types.h>
#include <driver/gpio.h>
#include <driver/h264.h>
#include <driver/yuv_buf.h>

#include <components/dvp_camera.h>

#include "bk_misc.h"
#include "gpio_driver.h"
#include "avdk_crc.h"
#include "media_utils.h"
#include "dvp_private.h"

#define TAG "dvp_drv"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#ifndef CONFIG_DVP_THREAD_STACK_SIZE
#define CONFIG_DVP_THREAD_STACK_SIZE 2048
#endif

#define DVP_MSG_QUEUE_SIZE 10
#define DVP_THREAD_PRIORITY 5

uint32_t s_dvp_dma_length = 0;
static const dvp_sensor_config_t **devices_list = NULL;
static uint16_t devices_size = 0;

// DVP work thread function
static void dvp_work_thread(void *param)
{
    dvp_driver_handle_t *handle = (dvp_driver_handle_t *)param;
    dvp_event_msg_t msg;
    bk_err_t ret;
    LOGD("%s thread started\n", __func__);

    rtos_set_semaphore(&handle->thread_sem);

    while (!handle->thread_should_exit)
    {
        ret = rtos_pop_from_queue(&handle->dvp_msg_queue, &msg, BEKEN_WAIT_FOREVER);
        if (ret != BK_OK)
        {
            continue;
        }

        // Process different types of events
        switch (msg.type)
        {
            case DVP_EVENT_YUV_EOF:
            {
                frame_buffer_t *yuv_frame = (frame_buffer_t *)msg.param1;
                uint8_t status = (uint8_t)msg.param2;
                if (yuv_frame && handle->callback && handle->callback->complete)
                {
                    handle->callback->complete(IMAGE_YUV, yuv_frame, status);
                }
                break;
            }

            case DVP_EVENT_JPEG_EOF:
            {
                frame_buffer_t *jpeg_frame = (frame_buffer_t *)msg.param1;
                uint8_t status = (uint8_t)msg.param2;
                if (jpeg_frame && handle->callback && handle->callback->complete)
                {
                    handle->callback->complete(IMAGE_MJPEG, jpeg_frame, status);
                }
                break;
            }

            case DVP_EVENT_H264_EOF:
            {
                frame_buffer_t *h264_frame = (frame_buffer_t *)msg.param1;
                uint8_t status = (uint8_t)msg.param2;
                if (h264_frame && handle->callback && handle->callback->complete)
                {
#ifdef CONFIG_H264_ADD_SELF_DEFINE_SEI
                    // Process H264 SEI in thread
                    h264_frame->crc = hnd_crc8(h264_frame->frame, h264_frame->length, 0xFF);
                    h264_frame->length += H264_SELF_DEFINE_SEI_SIZE;
                    os_memcpy(&handle->sei[23], (uint8_t *)h264_frame, sizeof(frame_buffer_t));
                    os_memcpy(&h264_frame->frame[h264_frame->length - H264_SELF_DEFINE_SEI_SIZE], &handle->sei[0], H264_SELF_DEFINE_SEI_SIZE);
#endif
                    handle->callback->complete(IMAGE_H264, h264_frame, status);
                }
                break;
            }

            case DVP_EVENT_EXIT:
            {
                LOGD("Exiting DVP work thread\n");
                goto exit;
            }

            default:
                LOGW("Unknown event type: %d\n", msg.type);
                break;
        }
    }

exit:
    // Release thread exit semaphore to notify deinit that thread has fully exited
    if (handle->thread_sem != NULL)
    {
        rtos_set_semaphore(&handle->thread_sem);
    }

    handle->thread_should_exit = true;
    LOGD("%s thread exited\n", __func__);
    
    rtos_delete_thread(NULL);
}

#if (MEDIA_DEBUG_TIMER_ENABLE)
static void dvp_camera_timer_handle(void *param)
{
    dvp_driver_handle_t *handle = param;
    // fps[sequence length(current) Kps]
    uint32_t fps = 0, kbps = 0;

    fps = (handle->frame_id - handle->later_seq) / MEDIA_DEBUG_TIMER_INTERVAL;
    kbps = (handle->latest_kbps - handle->later_kbps) / MEDIA_DEBUG_TIMER_INTERVAL * 8 / 1000;
    handle->later_seq = handle->frame_id;
    handle->later_kbps = handle->latest_kbps;
    LOGW("dvp:%d[%d %dKB %dKbps]\n",
        fps, handle->frame_id, handle->curr_length / 1024, kbps);
}
#endif

const dvp_sensor_config_t **get_sensor_config_devices_list(void)
{
    return devices_list;
}

int get_sensor_config_devices_num(void)
{
    return devices_size;
}

void bk_dvp_set_devices_list(const dvp_sensor_config_t **list, uint16_t size)
{
    devices_list = list;
    devices_size = size;
}

const dvp_sensor_config_t *get_sensor_config_interface_by_id(sensor_id_t id)
{
    uint32_t i;

    for (i = 0; i < devices_size; i++)
    {
        if (devices_list[i]->id == id)
        {
            return devices_list[i];
        }
    }

    return NULL;
}

const dvp_sensor_config_t *bk_dvp_get_sensor_auto_detect(void)
{
    const dvp_sensor_config_t *sensor = NULL;
    for (dvp_sensor_detect_func_t *p = &__camera_sensor_detect_array_start; p < &__camera_sensor_detect_array_end; p++)
    {
        if (p->detect)
        {
            sensor = p->detect();
            if (sensor != NULL)
            {
                return sensor;
            }
        }
    }

    return NULL;
}

static bk_err_t dvp_camera_init_device(dvp_driver_handle_t *handle)
{
    bk_dvp_config_t *config = handle->config;
    const dvp_sensor_config_t *sensor = handle->sensor;

    if (config->width != (sensor->def_ppi >> 16) ||
        config->height != (sensor->def_ppi & 0xFFFF))
    {
        if (!(pixel_ppi_to_cap((config->width << 16)
                               | config->height) & (sensor->ppi_cap)))
        {
            LOGE("%s, %d, not support this resolution...\r\n", __func__, __LINE__);
            return BK_FAIL;
        }
    }

    return BK_OK;
}

static void dvp_camera_dma_finish_callback(dma_id_t id)
{
    s_dvp_dma_length += FRAME_BUFFER_CACHE;
}

static bk_err_t dvp_camera_dma_config(dvp_driver_handle_t *handle)
{
    bk_err_t ret = BK_OK;
    dma_config_t dma_config = {0};
    uint32_t encode_fifo_addr;
    bk_dvp_config_t *config = handle->config;

    if (config->img_format & IMAGE_H264)
    {
#ifdef CONFIG_H264
        bk_h264_get_fifo_addr(&encode_fifo_addr);
        handle->dma_channel = bk_fixed_dma_alloc(DMA_DEV_H264, DMA_ID_8);
#else
        ret = BK_FAIL;
        LOGE("h264 dma config failed\n");
        return ret;
#endif
    }
    else if (config->img_format & IMAGE_MJPEG)
    {
#ifdef CONFIG_JPEGENC_HW
        bk_jpeg_enc_get_fifo_addr(&encode_fifo_addr);
        handle->dma_channel = bk_fixed_dma_alloc(DMA_DEV_JPEG, DMA_ID_8);
#else
        ret = BK_FAIL;
        LOGE("jpeg dma config failed\n");
        return ret;
#endif
    }

    LOGV("dvp_dma id:%d \r\n", handle->dma_channel);

    if (handle->dma_channel >= DMA_ID_MAX)
    {
        LOGE("malloc dma fail \r\n");
        ret = BK_FAIL;
        return ret;
    }

    if (config->img_format & IMAGE_H264)
    {
        handle->encode_frame = handle->callback->malloc(IMAGE_H264, CONFIG_H264_FRAME_SIZE);
        handle->encode_frame->fmt = PIXEL_FMT_H264;
    }
    else if (config->img_format & IMAGE_MJPEG)
    {
        handle->encode_frame = handle->callback->malloc(IMAGE_MJPEG, CONFIG_JPEG_FRAME_SIZE);
        handle->encode_frame->fmt = PIXEL_FMT_JPEG;
    }

    if (handle->encode_frame == NULL)
    {
        LOGE("malloc frame fail \r\n");
        ret = BK_ERR_NO_MEM;
        return ret;
    }

    handle->encode_frame->width = config->width;
    handle->encode_frame->height = config->height;

    dma_config.mode = DMA_WORK_MODE_REPEAT;
    dma_config.chan_prio = 0;
    dma_config.src.width = DMA_DATA_WIDTH_32BITS;
    dma_config.src.start_addr = encode_fifo_addr;
    dma_config.dst.dev = DMA_DEV_DTCM;
    dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
    dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
    dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;

    if (config->img_format & IMAGE_H264)
    {
        dma_config.src.dev = DMA_DEV_H264;
    }
    else if (config->img_format & IMAGE_MJPEG)
    {
        dma_config.src.dev = DMA_DEV_JPEG;
    }

    dma_config.dst.start_addr = (uint32_t)handle->encode_frame->frame;
    dma_config.dst.end_addr = (uint32_t)(handle->encode_frame->frame + handle->encode_frame->size);
    BK_LOG_ON_ERR(bk_dma_init(handle->dma_channel, &dma_config));
    BK_LOG_ON_ERR(bk_dma_set_transfer_len(handle->dma_channel, FRAME_BUFFER_CACHE));
    BK_LOG_ON_ERR(bk_dma_register_isr(handle->dma_channel, NULL, dvp_camera_dma_finish_callback));
    BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(handle->dma_channel));
#if (CONFIG_SPE)
    BK_LOG_ON_ERR(bk_dma_set_src_burst_len(handle->dma_channel, BURST_LEN_SINGLE));
    BK_LOG_ON_ERR(bk_dma_set_dest_burst_len(handle->dma_channel, BURST_LEN_INC16));
    BK_LOG_ON_ERR(bk_dma_set_dest_sec_attr(handle->dma_channel, DMA_ATTR_SEC));
    BK_LOG_ON_ERR(bk_dma_set_src_sec_attr(handle->dma_channel, DMA_ATTR_SEC));
#endif
    BK_LOG_ON_ERR(bk_dma_start(handle->dma_channel));

    return ret;
}

static bk_err_t encode_yuv_dma_cpy(void *out, const void *in, uint32_t len, dma_id_t cpy_chnl)
{
    dma_config_t dma_config = {0};
    os_memset(&dma_config, 0, sizeof(dma_config_t));

    dma_config.mode = DMA_WORK_MODE_SINGLE;
    dma_config.chan_prio = 1;

    dma_config.src.dev = DMA_DEV_DTCM;
    dma_config.src.width = DMA_DATA_WIDTH_32BITS;
    dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
    dma_config.src.start_addr = (uint32_t)in;
    dma_config.src.end_addr = (uint32_t)(in + len);

    dma_config.dst.dev = DMA_DEV_DTCM;
    dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
    dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
    dma_config.dst.start_addr = (uint32_t)out;
    dma_config.dst.end_addr = (uint32_t)(out + len);

    BK_LOG_ON_ERR(bk_dma_init(cpy_chnl, &dma_config));
    BK_LOG_ON_ERR(bk_dma_set_transfer_len(cpy_chnl, len));
#if (CONFIG_SPE)
    BK_LOG_ON_ERR(bk_dma_set_src_burst_len(cpy_chnl, 3));
    BK_LOG_ON_ERR(bk_dma_set_dest_burst_len(cpy_chnl, 3));
    BK_LOG_ON_ERR(bk_dma_set_dest_sec_attr(cpy_chnl, DMA_ATTR_SEC));
    BK_LOG_ON_ERR(bk_dma_set_src_sec_attr(cpy_chnl, DMA_ATTR_SEC));
#endif

    return BK_OK;
}

static bk_err_t dvp_camera_init(dvp_driver_handle_t *handle)
{
    handle->sensor = bk_dvp_detect(handle->config);
    if (handle->sensor == NULL)
    {
        return BK_FAIL;
    }

    /* set current used camera config */
    BK_RETURN_ON_ERR(dvp_camera_init_device(handle));

    return BK_OK;
}

static bk_err_t dvp_camera_deinit(dvp_driver_handle_t *handle)
{
    // step 1: deinit dvp gpio, data cannot transfer
#ifndef CONFIG_GPIO_DEFAULT_SET_SUPPORT
    dvp_camera_io_deinit(&handle->config->io_config);
#endif

    // step 2: deinit i2c
    bk_i2c_deinit(handle->config->i2c_config.id);

    // step 3: deinit hardware
    if (handle->sensor)
    {
        bk_yuv_buf_deinit();
#ifdef CONFIG_H264
        bk_h264_encode_disable();
        bk_h264_deinit();
#endif
#ifdef CONFIG_JPEGENC_HW
        bk_jpeg_enc_deinit();
#endif
        handle->sensor = NULL;
    }

    // step 4: deinit dvp mclk
    dvp_camera_mclk_disable();

    uint16_t format = handle->config->img_format;

    if (format != IMAGE_YUV)
    {
        bk_dma_stop(handle->dma_channel);
        bk_dma_deinit(handle->dma_channel);

        if (format & IMAGE_H264)
        {
            bk_dma_free(DMA_DEV_H264, handle->dma_channel);
            if (handle->encode_frame)
            {
                handle->callback->complete(IMAGE_H264, handle->encode_frame, DVP_FRAME_ERR);
                handle->encode_frame = NULL;
            }
        }

        if (format & IMAGE_MJPEG)
        {
            bk_dma_free(DMA_DEV_JPEG, handle->dma_channel);
            if (handle->encode_frame)
            {
                handle->callback->complete(IMAGE_MJPEG, handle->encode_frame, DVP_FRAME_ERR);
                handle->encode_frame = NULL;
            }
        }

        if ((format & IMAGE_YUV) && format != IMAGE_YUV)
        {
            bk_dma_stop(handle->yuv_config.dma_collect_yuv);
            bk_dma_deinit(handle->yuv_config.dma_collect_yuv);
            bk_dma_free(DMA_DEV_DTCM, handle->yuv_config.dma_collect_yuv);
            os_memset(&handle->yuv_config, 0, sizeof(encode_yuv_config_t));
        }
    }

    // step 0: Stop and cleanup work thread
    if (handle->dvp_thread != NULL)
    {
        LOGD("Stopping DVP work thread...\n");
        
        // Send an empty message to wake up the thread and let it exit
        dvp_event_msg_t exit_msg;
        exit_msg.type = DVP_EVENT_EXIT;
        exit_msg.param1 = 0;
        exit_msg.param2 = 0;
        rtos_push_to_queue(&handle->dvp_msg_queue, &exit_msg, BEKEN_NO_WAIT);
        
        // Use semaphore to wait for thread to fully exit (must wait for semaphore to avoid abnormal access)
        bk_err_t sem_ret = rtos_get_semaphore(&handle->thread_sem, BEKEN_WAIT_FOREVER);
        if (sem_ret == BK_OK)
        {
            LOGD("DVP work thread exited successfully\n");
        }
        else
        {
            // Should not reach here in theory
            LOGE("Wait for thread exit failed! ret=%d\n", sem_ret);
        }
        
        handle->dvp_thread = NULL;
    }
    
    // Process remaining messages in queue to avoid buffer leak
    if (handle->dvp_msg_queue != NULL)
    {
        dvp_event_msg_t msg;
        int processed_count = 0;
        
        // Clear all remaining messages in queue and release corresponding buffers
        while (rtos_pop_from_queue(&handle->dvp_msg_queue, &msg, BEKEN_NO_WAIT) == BK_OK)
        {
            if (msg.type == DVP_EVENT_YUV_EOF || msg.type == DVP_EVENT_JPEG_EOF || msg.type == DVP_EVENT_H264_EOF)
            {
                frame_buffer_t *frame = (frame_buffer_t *)msg.param1;
                if (frame && handle->callback && handle->callback->complete)
                {
                    // Notify upper layer to release buffer with error status
                    switch (msg.type)
                    {
                        case DVP_EVENT_YUV_EOF:
                            handle->callback->complete(IMAGE_YUV, frame, DVP_FRAME_ERR);
                            break;
                        case DVP_EVENT_JPEG_EOF:
                            handle->callback->complete(IMAGE_MJPEG, frame, DVP_FRAME_ERR);
                            break;
                        case DVP_EVENT_H264_EOF:
                            handle->callback->complete(IMAGE_H264, frame, DVP_FRAME_ERR);
                            break;
                        default:
                            break;
                    }
                    processed_count++;
                }
            }
        }
        
        if (processed_count > 0)
        {
            LOGD("Processed %d pending messages in queue during deinit\n", processed_count);
        }
        
        // Cleanup message queue
        rtos_deinit_queue(&handle->dvp_msg_queue);
        handle->dvp_msg_queue = NULL;
    }

    if (handle->yuv_frame)
    {
        handle->callback->complete(IMAGE_YUV, handle->yuv_frame, DVP_FRAME_ERR);
        handle->yuv_frame = NULL;
    }

    if (handle->sem)
    {
        rtos_deinit_semaphore(&handle->sem);
        handle->sem = NULL;
    }

    if (handle->thread_sem)
    {
        rtos_deinit_semaphore(&handle->thread_sem);
        handle->thread_sem = NULL;
    }

#if (MEDIA_DEBUG_TIMER_ENABLE)
    if (handle->timer.handle)
    {
        rtos_stop_timer(&handle->timer);
        rtos_deinit_timer(&handle->timer);
    }
#endif

    handle->dvp_state = MASTER_TURN_OFF;
    os_free(handle);
    bk_pm_module_vote_psram_ctrl(PM_POWER_PSRAM_MODULE_NAME_VIDP_JPEG_EN,PM_POWER_MODULE_STATE_OFF);
    LOGD("%s complete!\r\n", __func__);

    return BK_OK;
}

static void dvp_camera_reset_hardware_modules_handler(dvp_driver_handle_t *handle)
{
    bk_dvp_config_t *config = handle->config;

    bk_yuv_buf_stop(YUV_MODE);

#ifdef CONFIG_JPEGENC_HW
    if (config->img_format & IMAGE_MJPEG)
    {
        bk_yuv_buf_stop(JPEG_MODE);
        bk_jpeg_enc_soft_reset();
    }
#endif

#ifdef CONFIG_H264
    if (config->img_format & IMAGE_H264)
    {
        bk_yuv_buf_stop(H264_MODE);
        bk_h264_encode_disable();
        bk_h264_init(config->width, config->height);
    }
#endif

    bk_yuv_buf_soft_reset();

    handle->yuv_config.yuv_data_offset = 0;

    if (handle->dma_channel < DMA_ID_MAX)
    {
        s_dvp_dma_length = 0;
        bk_dma_stop(handle->dma_channel);
        if (handle->encode_frame)
        {
            handle->encode_frame->length = 0;
        }
        bk_dma_start(handle->dma_channel);
    }

    if (handle->config->img_format & IMAGE_H264)
    {
        bk_yuv_buf_start(H264_MODE);
        bk_h264_encode_enable();
    }
    else if (handle->config->img_format & IMAGE_MJPEG)
    {
        bk_yuv_buf_start(JPEG_MODE);
    }
    else
    {
        bk_yuv_buf_start(YUV_MODE);
    }
}

static void dvp_camera_sensor_ppi_err_handler(yuv_buf_unit_t id, void *param)
{
    DVP_PPI_ERROR_ENTRY();

    dvp_driver_handle_t *handle = (dvp_driver_handle_t *)param;

    if (handle->dvp_state != MASTER_TURN_ON)
    {
        LOGD("%s, %d\r\n", __func__, __LINE__);
        DVP_PPI_ERROR_OUT();
        return;
    }

    if (!handle->error)
    {
        LOGV("%s, %d\n", __func__, __LINE__);
        handle->error = true;
    }

    DVP_PPI_ERROR_OUT();
}

static void yuv_sm0_line_done(yuv_buf_unit_t id, void *param)
{
    dvp_driver_handle_t *handle = (dvp_driver_handle_t *)param;

    if (handle->dvp_state != MASTER_TURN_ON)
    {
        return;
    }

    encode_yuv_config_t *yuv_config = &handle->yuv_config;
    if ((yuv_config->yuv_data_offset + yuv_config->yuv_pingpong_length) > handle->yuv_frame->length)
    {
        yuv_config->yuv_data_offset = 0;
        handle->error = true;
    }

    if (bk_dma_get_enable_status(yuv_config->dma_collect_yuv))
    {
        LOGW("%s, dma is busy\n", __func__);
        // dma is busy
        handle->error = true;
    }

    bk_dma_stop(yuv_config->dma_collect_yuv);

    if (handle->error)
    {
        // do not memcpy yuv data
        return;
    }

    bk_dma_set_src_start_addr(yuv_config->dma_collect_yuv,
                              (uint32_t)yuv_config->yuv_em_addr);
    bk_dma_set_dest_start_addr(yuv_config->dma_collect_yuv,
                               (uint32_t)(handle->yuv_frame->frame + yuv_config->yuv_data_offset));
    bk_dma_start(yuv_config->dma_collect_yuv);
    yuv_config->yuv_data_offset += yuv_config->yuv_pingpong_length;
}

static void yuv_sm1_line_done(yuv_buf_unit_t id, void *param)
{
    dvp_driver_handle_t *handle = (dvp_driver_handle_t *)param;

    if (handle->dvp_state != MASTER_TURN_ON)
    {
        return;
    }

    encode_yuv_config_t *yuv_config = &handle->yuv_config;
    if ((yuv_config->yuv_data_offset + yuv_config->yuv_pingpong_length) > handle->yuv_frame->length)
    {
        yuv_config->yuv_data_offset = 0;
        handle->error = true;
    }

    if (bk_dma_get_enable_status(yuv_config->dma_collect_yuv))
    {
        LOGW("%s, dma is busy\n", __func__);
        // dma is busy
        handle->error = true;
    }

    bk_dma_stop(yuv_config->dma_collect_yuv);

    if (handle->error)
    {
        // do not memcpy yuv data
        return;
    }

    bk_dma_set_src_start_addr(yuv_config->dma_collect_yuv,
                              (uint32_t)yuv_config->yuv_em_addr + yuv_config->yuv_pingpong_length);
    bk_dma_set_dest_start_addr(yuv_config->dma_collect_yuv,
                               (uint32_t)(handle->yuv_frame->frame + yuv_config->yuv_data_offset));
    bk_dma_start(yuv_config->dma_collect_yuv);
    yuv_config->yuv_data_offset += yuv_config->yuv_pingpong_length;
}

static void dvp_camera_vsync_negedge_handler(yuv_buf_unit_t id, void *param)
{
    DVP_VSYNC_ENTRY();

    dvp_driver_handle_t *handle = (dvp_driver_handle_t *)param;

    if (handle->dvp_state == MASTER_TURNING_OFF)
    {
        bk_yuv_buf_stop(YUV_MODE);
        bk_yuv_buf_stop(JPEG_MODE);
        bk_yuv_buf_stop(H264_MODE);

        if (handle->sem != NULL)
        {
            rtos_set_semaphore(&handle->sem);
        }
        DVP_VSYNC_OUT();
        return;
    }

    if (handle->error)
    {
        DVP_RESET_ENTRY();
        handle->error = false;
        handle->sequence = 0;
        dvp_camera_reset_hardware_modules_handler(handle);
        LOGV("reset OK \r\n");
        DVP_RESET_OUT();
    }

#ifdef CONFIG_H264
    if (handle->regenerate_idr)
    {
        handle->sequence = 0;
        bk_h264_soft_reset();
        handle->regenerate_idr = false;
    }
#endif
    DVP_VSYNC_OUT();
}

static void dvp_camera_yuv_eof_handler(yuv_buf_unit_t id, void *param)
{
    frame_buffer_t *new_yuv = NULL;
    dvp_driver_handle_t *handle = (dvp_driver_handle_t *)param;
    dvp_event_msg_t msg;

    DVP_YUV_EOF_ENTRY();

    if (handle->dvp_state != MASTER_TURN_ON)
    {
        DVP_YUV_EOF_OUT();
        return;
    }

    if (handle->error)
    {
        LOGE("%s, yuv frame error\r\n", __func__);
    }

    uint32_t size = handle->config->width * handle->config->height * 2;

    handle->yuv_frame->sequence = handle->frame_id++;
    handle->yuv_frame->timestamp = get_current_timestamp();

    new_yuv = handle->callback->malloc(IMAGE_YUV, size);
    if (new_yuv)
    {
        new_yuv->width = handle->yuv_frame->width;
        new_yuv->height = handle->yuv_frame->height;
        new_yuv->fmt = handle->yuv_frame->fmt;
        new_yuv->length = size;

        // Put completion notification task into queue for thread processing
        msg.type = DVP_EVENT_YUV_EOF;
        msg.param1 = (uint32_t)handle->yuv_frame;
        msg.param2 = DVP_FRAME_OK;

        bk_err_t ret = rtos_push_to_queue(&handle->dvp_msg_queue, &msg, BEKEN_NO_WAIT);
        if (ret != BK_OK)
        {
            LOGE("Failed to send YUV EOF event, ret=%d, release frame directly\n", ret);
            // If queue is full, release the frame directly
            handle->callback->complete(IMAGE_YUV, handle->yuv_frame, DVP_FRAME_ERR);
        }

        handle->yuv_frame = new_yuv;
    }
    else
    {
        //LOGE("%s malloc frame failed\n", __func__);
    }

    bk_yuv_buf_set_em_base_addr((uint32_t)handle->yuv_frame->frame);

    DVP_YUV_EOF_OUT();
}

static void dvp_camera_jpeg_eof_handler(jpeg_unit_t id, void *param)
{
    DVP_JPEG_EOF_ENTRY();
    dvp_driver_handle_t *handle = (dvp_driver_handle_t *)param;
    frame_buffer_t *frame_buffer = NULL;

    uint32_t real_length = 0, recv_length = 0;

    if (handle->dvp_state != MASTER_TURN_ON)
    {
        DVP_JPEG_EOF_OUT();
        return;
    }

    bk_dma_flush_src_buffer(handle->dma_channel);
    real_length = bk_jpeg_enc_get_frame_size();
    recv_length = FRAME_BUFFER_CACHE - bk_dma_get_remain_len(handle->dma_channel);
    bk_dma_stop(handle->dma_channel);

    s_dvp_dma_length += recv_length - JPEG_CRC_SIZE;
    handle->dma_length = s_dvp_dma_length;

    if (handle->dma_length != real_length)
    {
        uint32_t left_length = real_length - handle->dma_length;
        if (left_length != FRAME_BUFFER_CACHE)
        {
            DVP_SIZE_ERROR_ENTRY();
            LOGW("%s size no match:%d-%d=%d\n", __func__, real_length, handle->dma_length, left_length);
            handle->error = true;
            DVP_SIZE_ERROR_OUT();
        }
    }

    handle->dma_length = 0;
    s_dvp_dma_length = 0;

    if (handle->error)
    {
        handle->encode_frame->length = 0;
        DVP_JPEG_EOF_OUT();
        return;
    }

    handle->encode_frame->timestamp = get_current_timestamp();

    for (uint32_t i = real_length; i > real_length - 10; i--)
    {
        if (handle->encode_frame->frame[i - 1] == 0xD9
            && handle->encode_frame->frame[i - 2] == 0xFF)
        {
            real_length = i;
            handle->eof = true;
            break;
        }

        handle->eof = false;
    }

    if (handle->eof)
    {
#if (MEDIA_DEBUG_TIMER_ENABLE)
        handle->latest_kbps += real_length;
        handle->curr_length = real_length;
#endif
        handle->encode_frame->length = real_length;
        handle->encode_frame->sequence = handle->frame_id++;
        frame_buffer = handle->callback->malloc(IMAGE_MJPEG, CONFIG_JPEG_FRAME_SIZE);
        if (frame_buffer == NULL)
        {
            //LOGE("alloc frame error\n");
            //frame_buffer_reset(handle->encode_frame);
            handle->encode_frame->length = 0;
        }
        else
        {
            dvp_event_msg_t msg;
            frame_buffer->width = handle->config->width;
            frame_buffer->height = handle->config->height;
            frame_buffer->fmt = PIXEL_FMT_JPEG;
            frame_buffer->length = real_length;

            // Put completion notification task into queue for thread processing
            msg.type = DVP_EVENT_JPEG_EOF;
            msg.param1 = (uint32_t)handle->encode_frame;
            msg.param2 = DVP_FRAME_OK;

            bk_err_t ret = rtos_push_to_queue(&handle->dvp_msg_queue, &msg, BEKEN_NO_WAIT);
            if (ret != BK_OK)
            {
                LOGE("Failed to send JPEG EOF event, ret=%d, release frame directly\n", ret);
                // If queue is full, release the frame directly
                handle->callback->complete(IMAGE_MJPEG, handle->encode_frame, DVP_FRAME_ERR);
            }

            handle->encode_frame = frame_buffer;
        }
    }
    else
    {
        handle->encode_frame->length = 0;
    }

    handle->eof = false;
    if (handle->encode_frame == NULL
        || handle->encode_frame->frame == NULL)
    {
        LOGE("alloc frame error\n");
        return;
    }

    bk_dma_set_dest_addr(handle->dma_channel, (uint32_t)handle->encode_frame->frame, (uint32_t)(handle->encode_frame->frame + handle->encode_frame->size));
    bk_dma_start(handle->dma_channel);

    if (handle->config->img_format & IMAGE_YUV)
    {
        DVP_YUV_EOF_ENTRY();
        encode_yuv_config_t *yuv_config = &handle->yuv_config;
        frame_buffer_t *new_yuv = NULL;
        uint32_t size = handle->config->width * handle->config->height * 2;
        yuv_config->yuv_data_offset = 0;
        bk_dma_flush_src_buffer(yuv_config->dma_collect_yuv);
        handle->yuv_frame->sequence = handle->frame_id - 1;
        handle->yuv_frame->timestamp = get_current_timestamp();
        LOGV("%s, ppi:%d-%d, length:%d, fmt:%d, seq:%d, %p\r\n", __func__, handle->yuv_frame->width,
            handle->yuv_frame->height, handle->yuv_frame->length,
            handle->yuv_frame->fmt, handle->yuv_frame->sequence, handle->yuv_frame);
        new_yuv = handle->callback->malloc(IMAGE_YUV, size);
        if (new_yuv)
        {
            dvp_event_msg_t yuv_msg;
            new_yuv->width = handle->yuv_frame->width;
            new_yuv->height = handle->yuv_frame->height;
            new_yuv->fmt = handle->yuv_frame->fmt;
            new_yuv->length = size;

            // Put YUV completion notification task into queue for thread processing
            yuv_msg.type = DVP_EVENT_YUV_EOF;
            yuv_msg.param1 = (uint32_t)handle->yuv_frame;
            yuv_msg.param2 = DVP_FRAME_OK;

            bk_err_t ret = rtos_push_to_queue(&handle->dvp_msg_queue, &yuv_msg, BEKEN_NO_WAIT);
            if (ret != BK_OK)
            {
                LOGE("Failed to send YUV EOF event in JPEG mode, ret=%d, release frame directly\n", ret);
                // If queue is full, release the frame directly
                handle->callback->complete(IMAGE_YUV, handle->yuv_frame, DVP_FRAME_ERR);
            }

            handle->yuv_frame = new_yuv;
        }
        else
        {
            //LOGE("%s malloc new yuv frame failed\n", __func__);
        }
        DVP_YUV_EOF_OUT();
    }

    DVP_JPEG_EOF_OUT();

    return;
}

static void dvp_camera_h264_eof_handler(h264_unit_t id, void *param)
{
    DVP_H264_EOF_ENTRY();

    dvp_driver_handle_t *handle = (dvp_driver_handle_t *)param;

    if (handle->dvp_state != MASTER_TURN_ON)
    {
        DVP_H264_EOF_OUT();
        return;
    }

    uint32_t real_length = bk_h264_get_encode_count() * 4;
    uint32_t remain_length = 0;
    frame_buffer_t *new_frame = NULL;

    handle->sequence++;

    if (handle->sequence > H264_GOP_FRAME_CNT)
    {
        handle->sequence = 1;
    }

    if (handle->sequence == 1)
    {
        handle->i_frame = 1;
    }
    else
    {
        handle->i_frame = 0;
    }

#if (CONFIG_H264_GOP_START_IDR_FRAME)
    if (handle->sequence == H264_GOP_FRAME_CNT)
    {
        bk_h264_soft_reset();
        handle->sequence = 0;
    }
#endif

    if (real_length > CONFIG_H264_FRAME_SIZE - 0x20)
    {
        LOGE("%s size over h264 buffer range, %d\r\n", __func__, real_length);
        handle->error = true;
    }

    bk_dma_flush_src_buffer(handle->dma_channel);

    remain_length = FRAME_BUFFER_CACHE - bk_dma_get_remain_len(handle->dma_channel);

    bk_dma_stop(handle->dma_channel);

    s_dvp_dma_length += remain_length;
    handle->dma_length = s_dvp_dma_length;

    if (handle->dma_length != real_length)
    {
        uint32_t left_length = real_length - handle->dma_length;
        if (left_length != FRAME_BUFFER_CACHE)
        {
            DVP_SIZE_ERROR_ENTRY();
            LOGW("%s size no match:%d-%d=%d\n", __func__, real_length, handle->dma_length, left_length);
            handle->error = true;
            DVP_SIZE_ERROR_OUT();
        }
    }

    s_dvp_dma_length = 0;
    handle->dma_length = 0;
    handle->encode_frame->sequence = handle->frame_id++;

    if (handle->error || handle->regenerate_idr)
    {
        handle->encode_frame->length = 0;
        handle->sequence = 0;
        if (handle->regenerate_idr)
        {
            bk_h264_soft_reset();
            handle->regenerate_idr = false;
        }
        DVP_H264_EOF_OUT();
        goto out;
    }

#if (MEDIA_DEBUG_TIMER_ENABLE)
    handle->latest_kbps += real_length;
    handle->curr_length = real_length;
#endif

    handle->encode_frame->length = real_length;
    handle->encode_frame->timestamp = get_current_timestamp();

    if (handle->i_frame)
    {
        handle->encode_frame->h264_type |= 1 << H264_NAL_I_FRAME;
#if (CONFIG_H264_GOP_START_IDR_FRAME)
        handle->encode_frame->h264_type |= (1 << H264_NAL_SPS) | (1 << H264_NAL_PPS) | (1 << H264_NAL_IDR_SLICE);
#endif
    }
    else
    {
        handle->encode_frame->h264_type |= 1 << H264_NAL_P_FRAME;
    }

    handle->encode_frame->timestamp = get_current_timestamp();

    // SEI processing has been moved to work thread to avoid long interrupt callback time

    new_frame = handle->callback->malloc(IMAGE_H264, CONFIG_H264_FRAME_SIZE);
    if (new_frame)
    {
        dvp_event_msg_t msg;
        new_frame->width = handle->config->width;
        new_frame->height = handle->config->height;
        new_frame->fmt = PIXEL_FMT_H264;
        new_frame->length = real_length;

        // Put completion notification task into queue for thread processing
        msg.type = DVP_EVENT_H264_EOF;
        msg.param1 = (uint32_t)handle->encode_frame;
        msg.param2 = DVP_FRAME_OK;

        bk_err_t ret = rtos_push_to_queue(&handle->dvp_msg_queue, &msg, BEKEN_NO_WAIT);
        if (ret != BK_OK)
        {
            LOGE("Failed to send H264 EOF event, ret=%d, release frame directly\n", ret);
            // If queue is full, release the frame directly
            handle->callback->complete(IMAGE_H264, handle->encode_frame, DVP_FRAME_ERR);
        }

        handle->encode_frame = new_frame;
    }
    else
    {
        bk_h264_soft_reset();
        handle->encode_frame->length = 0;
        handle->encode_frame->h264_type = 0;
        handle->sequence = 0;
    }

out:
    bk_dma_set_dest_addr(handle->dma_channel, (uint32_t)handle->encode_frame->frame, (uint32_t)(handle->encode_frame->frame + handle->encode_frame->size));
    bk_dma_start(handle->dma_channel);

    if (handle->config->img_format & IMAGE_YUV)
    {
        DVP_YUV_EOF_ENTRY();
        frame_buffer_t *new_yuv = NULL;
        uint32_t size = handle->config->width * handle->config->height * 2;
        handle->yuv_config.yuv_data_offset = 0;
        bk_dma_flush_src_buffer(handle->yuv_config.dma_collect_yuv);
        handle->yuv_frame->sequence =  handle->frame_id - 1;
        handle->yuv_frame->timestamp = get_current_timestamp();
        LOGV("%s, ppi:%d-%d, length:%d, fmt:%d, seq:%d, %p\r\n", __func__, handle->yuv_frame->width,
            handle->yuv_frame->height, handle->yuv_frame->length,
            handle->yuv_frame->fmt, handle->yuv_frame->sequence, handle->yuv_frame);
        if (handle->error == false)
        {
            new_yuv = handle->callback->malloc(IMAGE_YUV, size);
            if (new_yuv)
            {
                dvp_event_msg_t yuv_msg;
                new_yuv->width = handle->yuv_frame->width;
                new_yuv->height = handle->yuv_frame->height;
                new_yuv->fmt = handle->yuv_frame->fmt;
                new_yuv->length = size;

                // Put YUV completion notification task into queue for thread processing
                yuv_msg.type = DVP_EVENT_YUV_EOF;
                yuv_msg.param1 = (uint32_t)handle->yuv_frame;
                yuv_msg.param2 = DVP_FRAME_OK;
                
                bk_err_t ret = rtos_push_to_queue(&handle->dvp_msg_queue, &yuv_msg, BEKEN_NO_WAIT);
                if (ret != BK_OK)
                {
                    LOGE("Failed to send YUV EOF event in H264 mode, ret=%d, release frame directly\n", ret);
                    // If queue is full, release the frame directly
                    handle->callback->complete(IMAGE_YUV, handle->yuv_frame, DVP_FRAME_ERR);
                }

                handle->yuv_frame = new_yuv;
            }
            else
            {
                //LOGE("%s malloc new yuv frame failed\n", __func__);
            }
        }
        DVP_YUV_EOF_OUT();
    }

    DVP_H264_EOF_OUT();

    return;
}

static bk_err_t dvp_camera_jpeg_config_init(dvp_driver_handle_t *handle)
{
    int ret = BK_FAIL;
#ifdef CONFIG_JPEGENC_HW
    jpeg_config_t jpeg_config = {0};
    bk_dvp_config_t *config = handle->config;
    const dvp_sensor_config_t *sensor = handle->sensor;

    jpeg_config.x_pixel = config->width / 8;
    jpeg_config.y_pixel = config->height / 8;
    jpeg_config.vsync = sensor->vsync;
    jpeg_config.hsync = sensor->hsync;
    jpeg_config.clk = sensor->clk;
    jpeg_config.mode = JPEG_MODE;

    switch (sensor->fmt)
    {
        case PIXEL_FMT_YUYV:
            jpeg_config.sensor_fmt = YUV_FORMAT_YUYV;
            break;

        case PIXEL_FMT_UYVY:
            jpeg_config.sensor_fmt = YUV_FORMAT_UYVY;
            break;

        case PIXEL_FMT_YYUV:
            jpeg_config.sensor_fmt = YUV_FORMAT_YYUV;
            break;

        case PIXEL_FMT_UVYY:
            jpeg_config.sensor_fmt = YUV_FORMAT_UVYY;
            break;

        default:
            LOGE("JPEG MODULE not support this sensor input format\r\n");
            ret = kParamErr;
            return ret;
    }

    ret = bk_jpeg_enc_init(&jpeg_config);
    if (ret != BK_OK)
    {
        LOGE("jpeg init error\n");
    }

#endif
    return ret;
}

bk_err_t dvp_camera_yuv_buf_config_init(dvp_driver_handle_t *handle)
{
    int ret = BK_OK;
    bk_dvp_config_t *config = handle->config;
    const dvp_sensor_config_t *sensor = handle->sensor;
    yuv_buf_config_t yuv_mode_config = {0};

    if (config->img_format == IMAGE_YUV)
    {
        yuv_mode_config.work_mode = YUV_MODE;
    }
    else if (config->img_format & IMAGE_MJPEG)
    {
        yuv_mode_config.work_mode = JPEG_MODE;
    }
    else if (config->img_format & IMAGE_H264)
    {
        yuv_mode_config.work_mode = H264_MODE;
    }

    yuv_mode_config.mclk_div = YUV_MCLK_DIV_3;

    yuv_mode_config.x_pixel = config->width / 8;
    yuv_mode_config.y_pixel = config->height / 8;
    yuv_mode_config.yuv_mode_cfg.vsync = sensor->vsync;
    yuv_mode_config.yuv_mode_cfg.hsync = sensor->hsync;

    LOGD("%s, %d-%d, fmt:%X\r\n", __func__, config->width, config->height, config->img_format);

    switch (sensor->fmt)
    {
        case PIXEL_FMT_YUYV:
            yuv_mode_config.yuv_mode_cfg.yuv_format = YUV_FORMAT_YUYV;
            break;

        case PIXEL_FMT_UYVY:
            yuv_mode_config.yuv_mode_cfg.yuv_format = YUV_FORMAT_UYVY;
            break;

        case PIXEL_FMT_YYUV:
            yuv_mode_config.yuv_mode_cfg.yuv_format = YUV_FORMAT_YYUV;
            break;

        case PIXEL_FMT_UVYY:
            yuv_mode_config.yuv_mode_cfg.yuv_format = YUV_FORMAT_UVYY;
            break;

        default:
            LOGE("YUV_BUF MODULE not support this sensor input format\r\n");
            ret = BK_ERR_PARAM;
    }

    if (ret != BK_OK)
    {
        return ret;
    }

    if (config->img_format & IMAGE_H264 || config->img_format & IMAGE_MJPEG)
    {
        if (handle->encode_buffer == NULL)
        {
            LOGE("encode buffer is NULL\r\n");
            return BK_ERR_NO_MEM;
        }

        LOGD("%s, encode_buf:%p\r\n", __func__, handle->encode_buffer);

        yuv_mode_config.base_addr = handle->encode_buffer;
    }

    ret = bk_yuv_buf_init(&yuv_mode_config);
    if (ret != BK_OK)
    {
        LOGE("yuv_buf yuv mode init error\n");
    }

    return ret;
}

static bk_err_t dvp_camera_yuv_mode(dvp_driver_handle_t *handle)
{
    LOGD("%s, %d\r\n", __func__, __LINE__);
    int ret = BK_OK;
    uint32_t size = 0;
    bk_dvp_config_t *config = handle->config;

    ret = dvp_camera_yuv_buf_config_init(handle);
    if (ret != BK_OK)
    {
        return ret;
    }

    size = config->width * config->height * 2;
    handle->yuv_frame = handle->callback->malloc(IMAGE_YUV, size);
    if (handle->yuv_frame == NULL)
    {
        LOGE("malloc frame fail \r\n");
        ret = BK_ERR_NO_MEM;
        return ret;
    }

    handle->yuv_frame->width = config->width;
    handle->yuv_frame->height = config->height;
    handle->yuv_frame->fmt = handle->sensor->fmt;
    handle->yuv_frame->length = size;
    bk_yuv_buf_set_em_base_addr((uint32_t)handle->yuv_frame->frame);

    return ret;
}

static bk_err_t dvp_camera_jpeg_mode(dvp_driver_handle_t *handle)
{
    LOGD("%s, %d\r\n", __func__, __LINE__);
    int ret = BK_OK;
    bk_dvp_config_t *config = handle->config;

    ret = dvp_camera_dma_config(handle);

    if (ret != BK_OK)
    {
        LOGE("dma init failed\n");
        return ret;
    }

    ret = dvp_camera_yuv_buf_config_init(handle);
    if (ret != BK_OK)
    {
        return ret;
    }

    ret = dvp_camera_jpeg_config_init(handle);
    if (ret != BK_OK)
    {
        return ret;
    }

    if (config->img_format & IMAGE_YUV)
    {
        uint32_t size = config->width * config->height * 2;
        handle->yuv_frame = handle->callback->malloc(IMAGE_YUV, size);
        if (handle->yuv_frame == NULL)
        {
            LOGE("yuv_frame malloc failed!\r\n");
            ret = BK_ERR_NO_MEM;
            return ret;
        }

        handle->yuv_frame->width = config->width;
        handle->yuv_frame->height = config->height;
        handle->yuv_frame->fmt = handle->sensor->fmt;
        handle->yuv_frame->length = size;
        handle->yuv_config.yuv_em_addr = bk_yuv_buf_get_em_base_addr();
        LOGV("yuv buffer base addr:%08x\r\n", handle->yuv_config.yuv_em_addr);
        handle->yuv_config.dma_collect_yuv = bk_dma_alloc(DMA_DEV_DTCM);
        handle->yuv_config.yuv_pingpong_length = config->width * 8 * 2;
        handle->yuv_config.yuv_data_offset = 0;
        LOGV("dma_collect_yuv id is %d \r\n", handle->yuv_config.dma_collect_yuv);

        encode_yuv_dma_cpy(handle->yuv_frame->frame,
                           (uint32_t *)handle->yuv_config.yuv_em_addr,
                           handle->yuv_config.yuv_pingpong_length,
                           handle->yuv_config.dma_collect_yuv);
    }

    return ret;
}

static bk_err_t dvp_camera_h264_mode(dvp_driver_handle_t *handle)
{
    LOGD("%s, %d\r\n", __func__, __LINE__);
    int ret = BK_OK;
    bk_dvp_config_t *config = handle->config;

    ret = dvp_camera_yuv_buf_config_init(handle);
    if (ret != BK_OK)
    {
        return ret;
    }

#ifdef CONFIG_H264
    ret = bk_h264_init(config->width, config->height);
#else
    ret = BK_FAIL;
#endif
    if (ret != BK_OK)
    {
        LOGE("h264 init failed\n");
        return ret;
    }

    ret = dvp_camera_dma_config(handle);
    if (ret != BK_OK)
    {
        LOGE("dma init failed\n");
        return ret;
    }

#ifdef CONFIG_H264_ADD_SELF_DEFINE_SEI
    os_memset(&handle->sei[0], 0xFF, H264_SELF_DEFINE_SEI_SIZE);

    h264_encode_sei_init(&handle->sei[0]);
#endif

    if (config->img_format & IMAGE_YUV)
    {
        uint32_t size = config->width * config->height * 2;
        handle->yuv_frame = handle->callback->malloc(IMAGE_YUV, size);
        if (handle->yuv_frame == NULL)
        {
            LOGE("yuv_frame malloc failed!\r\n");
            ret = BK_ERR_NO_MEM;
            return ret;
        }

        handle->yuv_frame->width = config->width;
        handle->yuv_frame->height = config->height;
        handle->yuv_frame->fmt = handle->sensor->fmt;
        handle->yuv_frame->length = size;
        handle->yuv_config.yuv_em_addr = bk_yuv_buf_get_em_base_addr();
        LOGV("yuv buffer base addr:%08x\r\n", handle->yuv_config.yuv_em_addr);
        handle->yuv_config.dma_collect_yuv = bk_dma_alloc(DMA_DEV_DTCM);
        handle->yuv_config.yuv_pingpong_length = config->width * 16 * 2;
        handle->yuv_config.yuv_data_offset = 0;
        LOGV("dma_collect_yuv id is %d \r\n", handle->yuv_config.dma_collect_yuv);

        encode_yuv_dma_cpy(handle->yuv_frame->frame,
                           (uint32_t *)handle->yuv_config.yuv_em_addr,
                           handle->yuv_config.yuv_pingpong_length,
                           handle->yuv_config.dma_collect_yuv);
    }

    return ret;
}

static void dvp_camera_register_isr_function(dvp_driver_handle_t *handle)
{
    uint16_t format = handle->config->img_format;

    LOGD("%s, %d, fmt:%d\r\n", __func__, __LINE__, format);
    switch (format)
    {
        case IMAGE_YUV:
            bk_yuv_buf_register_isr(YUV_BUF_YUV_ARV, dvp_camera_yuv_eof_handler, (void *)handle);
            break;

        case IMAGE_MJPEG:
        case (IMAGE_MJPEG | IMAGE_YUV):
#ifdef CONFIG_JPEGENC_HW
            bk_jpeg_enc_register_isr(JPEG_EOF, dvp_camera_jpeg_eof_handler, (void *)handle);
            bk_jpeg_enc_register_isr(JPEG_FRAME_ERR, dvp_camera_sensor_ppi_err_handler, (void *)handle);
#endif
            break;

        case IMAGE_H264:
        case (IMAGE_H264 | IMAGE_YUV):
#ifdef CONFIG_H264
            bk_h264_register_isr(H264_FINAL_OUT, dvp_camera_h264_eof_handler, (void *)handle);
#endif
            break;

        default:
            break;
    }

    if ((format & IMAGE_YUV) && (format != IMAGE_YUV))
    {
        bk_yuv_buf_register_isr(YUV_BUF_SM0_WR, yuv_sm0_line_done, (void *)handle);
        bk_yuv_buf_register_isr(YUV_BUF_SM1_WR, yuv_sm1_line_done, (void *)handle);
    }

    bk_yuv_buf_register_isr(YUV_BUF_VSYNC_NEGEDGE, dvp_camera_vsync_negedge_handler, (void *)handle);

    bk_yuv_buf_register_isr(YUV_BUF_SEN_RESL, dvp_camera_sensor_ppi_err_handler, (void *)handle);
    bk_yuv_buf_register_isr(YUV_BUF_FULL, dvp_camera_sensor_ppi_err_handler, (void *)handle);
    bk_yuv_buf_register_isr(YUV_BUF_H264_ERR, dvp_camera_sensor_ppi_err_handler, (void *)handle);
    bk_yuv_buf_register_isr(YUV_BUF_ENC_SLOW, dvp_camera_sensor_ppi_err_handler, (void *)handle);
}

static void dvp_sensor_reset(uint8_t pwdn_pin, uint8_t reset_pin)
{
    // pull low pwdn pin, sensor power enable
    if (pwdn_pin != 0xFF)
    {
        gpio_dev_unmap(pwdn_pin);
        bk_gpio_enable_output(pwdn_pin);
        bk_gpio_set_output_high(pwdn_pin);
        bk_gpio_set_output_low(pwdn_pin);
        rtos_delay_milliseconds(10);
    }

    // pull up reset pin, sensor reset disable
    if (reset_pin != 0xFF)
    {
        gpio_dev_unmap(reset_pin);
        bk_gpio_enable_output(reset_pin);
        bk_gpio_set_output_low(reset_pin);
        bk_gpio_set_output_high(reset_pin);
        rtos_delay_milliseconds(10);
    }
}

const dvp_sensor_config_t *bk_dvp_detect(bk_dvp_config_t *config)
{
    i2c_config_t i2c_config = {0};
    const dvp_sensor_config_t *sensor = NULL;

    // step 1: power on sensor
    dvp_sensor_reset(config->pwdn_pin, config->reset_pin);

    // step 2: map dvp io by config
#ifndef CONFIG_GPIO_DEFAULT_SET_SUPPORT
    dvp_camera_io_init(&config->io_config);
#endif

    // step 3: enable dvp input xclk
    dvp_camera_mclk_enable(config->clk_source);

    // step 4: init i2c
    i2c_config.baud_rate = config->i2c_config.baud_rate;
    i2c_config.addr_mode = I2C_ADDR_MODE_7BIT;
    bk_i2c_init(config->i2c_config.id, &i2c_config);

    // step 5: detect sensor
    sensor = bk_dvp_get_sensor_auto_detect();

    if (sensor == NULL)
    {
        LOGE("%s no dvp camera found\n", __func__);
    }
    else
    {
        LOGD("auto detect success, dvp camera name:%s\r\n", sensor->name);
    }

    return sensor;
}

bk_err_t bk_dvp_open(camera_handle_t *handle, bk_dvp_config_t *cfg, const bk_dvp_callback_t *cb, uint8_t *encode_buffer)
{
    bk_err_t ret = BK_FAIL;

    LOGD("%s\n", __func__);

    if (cfg == NULL || cb == NULL)
    {
        LOGE("%s, %d, param error\n", __func__, __LINE__);
        return ret;
    }

    if (*handle != NULL)
    {
        LOGD("%s, %d, already open\n", __func__, __LINE__);
        return ret;
    }

    dvp_driver_handle_t *dvp_handle = (dvp_driver_handle_t *)os_malloc(sizeof(dvp_driver_handle_t));
    if (dvp_handle == NULL)
    {
        LOGE("%s, %d, malloc fail\n", __func__, __LINE__);
        return ret;
    }

    os_memset(dvp_handle, 0, sizeof(dvp_driver_handle_t));

    dvp_handle->config = cfg;
    dvp_handle->callback = cb;
    dvp_handle->encode_buffer = encode_buffer;

    bk_pm_module_vote_psram_ctrl(PM_POWER_PSRAM_MODULE_NAME_VIDP_JPEG_EN,PM_POWER_MODULE_STATE_ON);

    ret = rtos_init_semaphore(&dvp_handle->sem, 1);
    if (ret != BK_OK)
    {
        LOGE("%s, dvp_handle->sem malloc fail....\r\n", __func__);
        goto error;
    }

    // Create thread exit semaphore (initial value 1)
    ret = rtos_init_semaphore(&dvp_handle->thread_sem, 1);
    if (ret != BK_OK)
    {
        LOGE("%s, thread_sem init fail....\r\n", __func__);
        goto error;
    }

    // Create message queue
    ret = rtos_init_queue(&dvp_handle->dvp_msg_queue, 
                         "dvp_msg_queue",
                         sizeof(dvp_event_msg_t),
                         DVP_MSG_QUEUE_SIZE);
    if (ret != BK_OK)
    {
        LOGE("%s, dvp_msg_queue init fail....\r\n", __func__);
        goto error;
    }

    // Initialize thread exit flag
    dvp_handle->thread_should_exit = false;

    // Create work thread
    ret = rtos_create_thread(&dvp_handle->dvp_thread,
                            DVP_THREAD_PRIORITY,
                            "dvp_work_thread",
                            dvp_work_thread,
                            CONFIG_DVP_THREAD_STACK_SIZE,
                            (beken_thread_arg_t)dvp_handle);
    if (ret != BK_OK)
    {
        LOGE("%s, dvp_work_thread create fail....\r\n", __func__);
        goto error;
    }

    rtos_get_semaphore(&dvp_handle->thread_sem, BEKEN_WAIT_FOREVER);

#if (MEDIA_DEBUG_TIMER_ENABLE)
    ret = rtos_init_timer(&dvp_handle->timer, MEDIA_DEBUG_TIMER_INTERVAL * 1000,
        dvp_camera_timer_handle, dvp_handle);

    if (ret != BK_OK)
    {
        LOGE("%s, handle->timer fail....\r\n", __func__);
        goto error;
    }

    rtos_start_timer(&dvp_handle->timer);
#endif
    dvp_handle->dvp_state = MASTER_TURNING_ON;  

    DVP_DIAG_DEBUG_INIT();

    // step 1: for camera sensor, init other device
    ret = dvp_camera_init(dvp_handle);
    if (ret != BK_OK)
    {
        LOGD("%s, %d\r\n", __func__, __LINE__);
        goto error;
    }

    // step 2: init hardware modules
    switch (dvp_handle->config->img_format)
    {
        case IMAGE_YUV:
            ret = dvp_camera_yuv_mode(dvp_handle);
            break;

        case IMAGE_MJPEG:
        case (IMAGE_MJPEG | IMAGE_YUV):
            ret = dvp_camera_jpeg_mode(dvp_handle);
            break;

        case IMAGE_H264:
        case (IMAGE_H264 | IMAGE_YUV):
            ret = dvp_camera_h264_mode(dvp_handle);
            break;

        default:
            ret = BK_FAIL;
    }

    if (ret != BK_OK)
    {
        LOGD("%s, %d\r\n", __func__, __LINE__);
        goto error;
    }

    // step 4: maybe need register isr_func
    dvp_camera_register_isr_function(dvp_handle);

    // step 5: start hardware function in different mode

    if (dvp_handle->config->img_format == IMAGE_YUV)
    {
        bk_yuv_buf_start(YUV_MODE);
    }
    else if (dvp_handle->config->img_format & IMAGE_MJPEG)
    {
        bk_yuv_buf_start(JPEG_MODE);
    }
    else if (dvp_handle->config->img_format & IMAGE_H264)
    {
        bk_yuv_buf_start(H264_MODE);
#ifdef CONFIG_H264
        bk_h264_encode_enable();
#endif
    }

    s_dvp_dma_length = 0;
    dvp_handle->dvp_state = MASTER_TURN_ON;

    // step 6: init dvp camera sensor register
    dvp_handle->sensor->init();
    dvp_handle->sensor->set_ppi((dvp_handle->config->width << 16) | dvp_handle->config->height);
    dvp_handle->sensor->set_fps(dvp_handle->config->fps);

    LOGD("dvp open success %d X %d, %d, %X\n", dvp_handle->config->width, dvp_handle->config->height,
        dvp_handle->config->fps, dvp_handle->config->img_format);

    *handle = dvp_handle;

    return ret;

error:

    dvp_camera_deinit(dvp_handle);

    return ret;
}

bk_err_t bk_dvp_close(camera_handle_t handle)
{
    dvp_driver_handle_t *dvp_handle = (dvp_driver_handle_t *)handle;
    if (dvp_handle == NULL || dvp_handle->dvp_state == MASTER_TURN_OFF)
    {
        LOGD("%s, dvp not open!\r\n", __func__);
        goto out;
    }

    dvp_handle->dvp_state = MASTER_TURNING_OFF;

    if (BK_OK != rtos_get_semaphore(&dvp_handle->sem, 500))
    {
        LOGW("Not wait yuv vsync negedge!\r\n");
    }

out:

    dvp_camera_deinit(dvp_handle);

    return BK_OK;
}

bk_err_t bk_dvp_h264_idr_reset(camera_handle_t handle)
{
    bk_err_t ret = BK_FAIL;
#ifdef CONFIG_H264
    dvp_driver_handle_t *dvp_handle = (dvp_driver_handle_t *)handle;
    if (dvp_handle && dvp_handle->config->img_format & IMAGE_H264)
    {
        dvp_handle->regenerate_idr = true;
        ret = BK_OK;
    }
    else
    {
        LOGW("%s, not enable h264 func...\n", __func__);
    }
#endif
    return ret;
}

bk_err_t bk_dvp_suspend(camera_handle_t handle)
{
    bk_err_t ret = BK_OK;
    DVP_SOFT_REST_ENTRY();
    dvp_driver_handle_t *dvp_handle = (dvp_driver_handle_t *)handle;
    if (dvp_handle)
    {
        // stop encode and reset hardware modules
        if (dvp_handle->config->img_format & IMAGE_H264)
        {
            bk_yuv_buf_stop(H264_MODE);
            bk_h264_encode_disable();
            bk_h264_global_soft_reset(true);
        }
        else if (dvp_handle->config->img_format & IMAGE_MJPEG)
        {
            bk_yuv_buf_stop(JPEG_MODE);
            bk_jpeg_enc_global_soft_reset(true);
        }
        else
        {
            bk_yuv_buf_stop(YUV_MODE);
        }

        bk_yuv_buf_global_soft_reset(true);
    }

    return ret;
}

bk_err_t bk_dvp_resume(camera_handle_t handle)
{
    bk_err_t ret = BK_OK;
    DVP_SOFT_REST_OUT();
    dvp_driver_handle_t *dvp_handle = (dvp_driver_handle_t *)handle;
    if (dvp_handle)
    {
        dvp_handle->error = true;

        if (dvp_handle->config->img_format & IMAGE_H264)
        {
            bk_h264_global_soft_reset(false);
        }
        else if (dvp_handle->config->img_format & IMAGE_MJPEG)
        {
            bk_jpeg_enc_global_soft_reset(false);
        }
        else
        {
            bk_yuv_buf_global_soft_reset(false);
        }

        dvp_camera_vsync_negedge_handler(0, dvp_handle);
    }

    return ret;
}

bk_err_t bk_dvp_sensor_write_register(camera_handle_t handle, dvp_sensor_reg_val_t *reg_val)
{
    bk_err_t ret = BK_FAIL;
    dvp_driver_handle_t *dvp_handle = (dvp_driver_handle_t *)handle;
    if (dvp_handle && dvp_handle->sensor)
    {
        dvp_handle->sensor->write_register(reg_val->reg, reg_val->val);
        ret = BK_OK;
    }

    return ret;
}

bk_err_t bk_dvp_sensor_read_register(camera_handle_t handle, dvp_sensor_reg_val_t *reg_val)
{
    bk_err_t ret = BK_FAIL;
    dvp_driver_handle_t *dvp_handle = (dvp_driver_handle_t *)handle;
    if (dvp_handle && dvp_handle->sensor)
    {
        dvp_handle->sensor->read_register(reg_val->reg, &reg_val->val);
        ret = BK_OK;
    }

    return ret;
}
