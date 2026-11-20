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

#include <os/mem.h>
#include <os/str.h>

#include <driver/jpeg_dec.h>
#include <driver/jpeg_dec_types.h>
#include <driver/timer.h>
#include "jpeg_dec_ll_macro_def.h"

#include <components/media_types.h>
#include "yuv_encode.h"
#include "hw_jpeg_decode_opt.h"
#include "components/bk_jpeg_decode/bk_jpeg_decode_utils.h"
#include "components/bk_jpeg_decode/bk_jpeg_decode_types_hw.h"

#ifndef CONFIG_HW_JPEG_DECODE_OPT_TASK_STACK_SIZE
#define CONFIG_HW_JPEG_DECODE_OPT_TASK_STACK_SIZE (2048)
#endif

#define TAG "hw_dec_opt"

#define HW_OPT_DECODE_TIMEOUT_MS        500
#define HW_OPT_DECODE_MSG_QUEUE_SIZE    20
#define HW_OPT_DECODE_INPUT_QUEUE_SIZE  10
#define HW_OPT_DECODE_YUV_PIXEL_BYTES   2
#define HW_OPT_DECODE_DEFAULT_LINES_PER_BLOCK  16  // Default MCU height for JPEG
#define HW_OPT_DECODE_SEMAPHORE_TIMEOUT 500        // Semaphore wait timeout in ms

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

typedef struct
{
    beken_thread_t hw_thread;
    beken_semaphore_t hw_sem;
    beken_semaphore_t hw_sync_sem;
    beken_queue_t hw_message_queue;
    beken_queue_t hw_input_queue;
    beken2_timer_t decode_timer;
    uint8_t decode_err;
    uint8_t decode_timeout;
    uint8_t hw_state;
    uint8_t decode_timer_is_running;
    hardware_opt_decode_status_t hw_decode_status;

    bk_jpeg_decode_callback_t *decode_cbs;
    uint8_t is_async;
    uint8_t *sram_buffer;
    uint32_t lines_per_block;
    bool is_pingpong;
    bk_jpeg_decode_opt_copy_method_t copy_method;

    frame_buffer_t *output_frame;
    frame_buffer_t *input_frame;
    uint32_t dec_line_cnt;

} bk_hw_jpeg_decode_opt_t;

static bk_hw_jpeg_decode_opt_t *g_hw_jpeg_decode_opt = NULL;

// Send message to decode task
static bk_err_t hardware_opt_decode_task_send_msg(hardware_opt_decode_event_type_t event, uint32_t param)
{
    int ret = BK_OK;
    hardware_opt_decode_msg_t msg = {0};

    if (g_hw_jpeg_decode_opt && g_hw_jpeg_decode_opt->hw_state && g_hw_jpeg_decode_opt->hw_message_queue)
    {
        msg.event = event;
        msg.param = param;

        ret = rtos_push_to_queue(&g_hw_jpeg_decode_opt->hw_message_queue, &msg, BEKEN_WAIT_FOREVER);
        if (ret != BK_OK)
        {
            LOGE("%s push failed\n", __func__);
        }
    }
    else
    {
        LOGE("%s task not running %d\n", __func__, event);
    }
    return ret;
}


// Output complete callback
static void hw_jpeg_decode_opt_out_complete(uint32_t format_type, uint32_t result, frame_buffer_t *out_frame)
{
    LOGV("%s %d out_frame %p\n", __func__, __LINE__, out_frame);
    if (g_hw_jpeg_decode_opt->decode_cbs != NULL && g_hw_jpeg_decode_opt->decode_cbs->out_complete != NULL)
    {
        g_hw_jpeg_decode_opt->decode_cbs->out_complete(format_type, result, out_frame);
    }
    else
    {
        LOGE("%s %d out_complete is NULL\n", __func__, __LINE__);
    }
}

// Input complete callback
static void hw_jpeg_decode_opt_in_complete(frame_buffer_t *in_frame)
{
    LOGV("%s %d in_frame %p\n", __func__, __LINE__, in_frame);
    if (g_hw_jpeg_decode_opt->decode_cbs != NULL && g_hw_jpeg_decode_opt->decode_cbs->in_complete != NULL)
    {
        g_hw_jpeg_decode_opt->decode_cbs->in_complete(in_frame);
    }
    else
    {
        LOGE("%s %d in_complete is NULL\n", __func__, __LINE__);
    }
}

// Allocate output buffer
static frame_buffer_t *hw_jpeg_decode_opt_out_malloc(uint32_t size)
{
    frame_buffer_t *out_frame = NULL;
    if (g_hw_jpeg_decode_opt->decode_cbs != NULL && g_hw_jpeg_decode_opt->decode_cbs->out_malloc != NULL)
    {
        out_frame = g_hw_jpeg_decode_opt->decode_cbs->out_malloc(size);
    }
    LOGV("%s %d %p\n", __func__, __LINE__, out_frame);
    return out_frame;
}

// Hardware decoder timeout callback
static void bk_driver_opt_decoder_timeout(void *arg1, void *arg2)
{
    LOGE("%s %d opt decoder timeout\n", __func__, __LINE__);

    // Stop hardware decoder to prevent further operations
    bk_jpeg_dec_stop();

    // Check global pointer before accessing
    if (g_hw_jpeg_decode_opt == NULL)
    {
        LOGE("%s %d CRITICAL: g_hw_jpeg_decode_opt is NULL in ISR!\n", __func__, __LINE__);
        return;
    }

    // Update status flags
    g_hw_jpeg_decode_opt->decode_timeout = true;
    g_hw_jpeg_decode_opt->decode_timer_is_running = false;

    hardware_opt_decode_task_send_msg(HARDWARE_OPT_DECODE_EVENT_DECODE_TIMEOUT, 0);
}

// JPEG decoder error callback
static void jpeg_dec_opt_err_cb(jpeg_dec_res_t *result)
{
    bk_err_t ret = BK_FAIL;
    LOGE("%s %d hardware decode error occurred\n", __func__, __LINE__);

    // Stop hardware decoder
    bk_jpeg_dec_stop();
    
    // Check global pointer before accessing
    if (g_hw_jpeg_decode_opt == NULL)
    {
        LOGE("%s %d CRITICAL: g_hw_jpeg_decode_opt is NULL in ISR!\n", __func__, __LINE__);
        return;
    }

    // Update error status
    g_hw_jpeg_decode_opt->decode_err = true;
    g_hw_jpeg_decode_opt->hw_decode_status = HARDWARE_OPT_DECODE_STATUS_IDLE;
    
    // IMPORTANT: Always set semaphore to unblock waiting threads
    // This prevents thread hang even in error conditions
    ret = hardware_opt_decode_task_send_msg(HARDWARE_OPT_DECODE_EVENT_DECODE_COMPLETE, BK_FAIL);
    if (ret != BK_OK)
    {
        LOGE("%s %d CRITICAL: semaphore set failed: %d, thread may hang!\n", __func__, __LINE__, ret);
    }
}

// JPEG decoder EOF callback - called when line decode completes (runs in ISR context)
static void jpeg_dec_line_eof_cb(jpeg_dec_res_t *result)
{
    bk_err_t ret = BK_OK;
    // Check global pointer before accessing
    if (g_hw_jpeg_decode_opt == NULL)
    {
        LOGE("%s %d CRITICAL: g_hw_jpeg_decode_opt is NULL in ISR!\n", __func__, __LINE__);
        return;
    }

    if (result == NULL || result->ok == false)
    {
        LOGE("%s %d hardware decode line result failed\n", __func__, __LINE__);
        g_hw_jpeg_decode_opt->decode_err = true;
        ret = hardware_opt_decode_task_send_msg(HARDWARE_OPT_DECODE_EVENT_DECODE_COMPLETE, BK_FAIL);
        if (ret != BK_OK)
        {
            LOGE("%s %d CRITICAL: semaphore set failed: %d, thread may hang!\n", __func__, __LINE__, ret);
        }
        return;
    }

    // Check for division by zero
    if (g_hw_jpeg_decode_opt->lines_per_block == 0)
    {
        LOGE("%s %d CRITICAL: lines_per_block is 0!\n", __func__, __LINE__);
        return;
    }

    g_hw_jpeg_decode_opt->dec_line_cnt++;
    if (g_hw_jpeg_decode_opt->dec_line_cnt == result->pixel_y / g_hw_jpeg_decode_opt->lines_per_block)
    {
        ret = hardware_opt_decode_task_send_msg(HARDWARE_OPT_DECODE_EVENT_DECODE_CONTINUE, g_hw_jpeg_decode_opt->dec_line_cnt);
        if (ret != BK_OK)
        {
            LOGE("%s %d CRITICAL: semaphore set failed: %d, thread may hang!\n", __func__, __LINE__, ret);
        }

        ret = hardware_opt_decode_task_send_msg(HARDWARE_OPT_DECODE_EVENT_DECODE_COMPLETE, 0);
        if (ret != BK_OK)
        {
            LOGE("%s %d CRITICAL: semaphore set failed: %d, thread may hang!\n", __func__, __LINE__, ret);
        }
    }
    else
    {
        ret = hardware_opt_decode_task_send_msg(HARDWARE_OPT_DECODE_EVENT_DECODE_CONTINUE, g_hw_jpeg_decode_opt->dec_line_cnt);
        if (ret != BK_OK)
        {
            LOGE("%s %d CRITICAL: semaphore set failed: %d, thread may hang!\n", __func__, __LINE__, ret);
        }
    }
}

// Handle decode start event with unified error handling
// If async: on failure, release buffers via callbacks
// If sync: on failure, release buffers and set semaphore to unblock caller
static void hw_jpeg_decode_opt_handle_decode_start(frame_buffer_t *dest_frame)
{
    bk_err_t ret = BK_OK;
    frame_buffer_t *src_frame = NULL;

    if (g_hw_jpeg_decode_opt->hw_decode_status == HARDWARE_OPT_DECODE_STATUS_BUSY)
    {
        if (dest_frame != NULL)
        {
            LOGE("%s %d this should not happen\n", __func__, __LINE__);
            hw_jpeg_decode_opt_out_complete(PIXEL_FMT_YUYV, BK_FAIL, dest_frame);
        }
        if (!g_hw_jpeg_decode_opt->is_async)
        {
            LOGE("%s %d this should not happen\n", __func__, __LINE__);
            rtos_set_semaphore(&g_hw_jpeg_decode_opt->hw_sync_sem);
        }
        return;
    }

    hardware_opt_decode_msg_t msg_temp = {0};
    ret = rtos_pop_from_queue(&g_hw_jpeg_decode_opt->hw_input_queue, &msg_temp, BEKEN_NO_WAIT);
    if (ret != BK_OK)
    {
        LOGV("%s %d pop failed: %d\n", __func__, __LINE__, ret);
        goto error;
    }

    src_frame = (frame_buffer_t *)msg_temp.param;
    if (src_frame == NULL)
    {
        LOGE("%s %d src_frame is NULL\n", __func__, __LINE__);
        goto error;
    }

    if (dest_frame == NULL)
    {
        bk_jpeg_decode_img_info_t img_info = {0};
        img_info.frame = src_frame;
        ret = bk_get_jpeg_data_info(&img_info);
        if (ret != AVDK_ERR_OK)
        {
            LOGE(" %s %d bk_get_jpeg_data_info failed %d\n", __func__, __LINE__, ret);
            goto error;
        }

        src_frame->width = img_info.width;
        src_frame->height = img_info.height;

        dest_frame = hw_jpeg_decode_opt_out_malloc(img_info.width * img_info.height * HW_OPT_DECODE_YUV_PIXEL_BYTES);
        if (dest_frame == NULL)
        {
            LOGE("%s %d out_malloc failed\n", __func__, __LINE__);
            goto error;
        }
        dest_frame->width = img_info.width;
        dest_frame->height = img_info.height;
    }

    g_hw_jpeg_decode_opt->input_frame = src_frame;
    g_hw_jpeg_decode_opt->output_frame = dest_frame;
    g_hw_jpeg_decode_opt->hw_decode_status = HARDWARE_OPT_DECODE_STATUS_BUSY;
    g_hw_jpeg_decode_opt->dec_line_cnt = 0;

    g_hw_jpeg_decode_opt->decode_timeout = false;
    g_hw_jpeg_decode_opt->decode_err = false;
    bk_jpeg_dec_out_format(PIXEL_FMT_YUYV);
    dest_frame->fmt = PIXEL_FMT_YUYV;

    g_hw_jpeg_decode_opt->decode_timer_is_running = true;
    rtos_start_oneshot_timer(&g_hw_jpeg_decode_opt->decode_timer);
    if(g_hw_jpeg_decode_opt->is_pingpong)
    {
        ret = bk_jpeg_dec_hw_start(src_frame->length, src_frame->frame, g_hw_jpeg_decode_opt->sram_buffer);
    }
    else
    {
        ret = bk_jpeg_dec_hw_start_opt(src_frame->length, src_frame->frame, g_hw_jpeg_decode_opt->sram_buffer);
    }
    if (ret != BK_OK)
    {
        LOGE("%s %d bk_jpeg_dec_hw_start failed: %d\n", __func__, __LINE__, ret);
        if (g_hw_jpeg_decode_opt->decode_timer_is_running == true)
        {
            rtos_stop_oneshot_timer(&g_hw_jpeg_decode_opt->decode_timer);
            g_hw_jpeg_decode_opt->decode_timer_is_running = false;
        }
        g_hw_jpeg_decode_opt->output_frame = NULL;
        g_hw_jpeg_decode_opt->hw_decode_status = HARDWARE_OPT_DECODE_STATUS_IDLE;
        goto error;
    }
    return;

error:
    if (src_frame != NULL)
    {
        hw_jpeg_decode_opt_in_complete(src_frame);
    }
    if (dest_frame != NULL)
    {
        hw_jpeg_decode_opt_out_complete(PIXEL_FMT_YUYV, BK_FAIL, dest_frame);
    }
    if (!g_hw_jpeg_decode_opt->is_async)
    {
        rtos_set_semaphore(&g_hw_jpeg_decode_opt->hw_sync_sem);
    }
    return;
}

static void hw_jpeg_decode_opt_handle_decode_continue(uint32_t dec_line_cnt)
{
    if (g_hw_jpeg_decode_opt->output_frame == NULL)
    {
        LOGE("%s %d output_frame is NULL\n", __func__, __LINE__);
        return;
    }
    if(g_hw_jpeg_decode_opt->is_pingpong)
    {
        if(dec_line_cnt < g_hw_jpeg_decode_opt->output_frame->height / g_hw_jpeg_decode_opt->lines_per_block)
        {
            bk_jpeg_dec_by_line_start();
        }
        uint32_t offset_in = (1 - (dec_line_cnt & 1)) * g_hw_jpeg_decode_opt->output_frame->width * g_hw_jpeg_decode_opt->lines_per_block * HW_OPT_DECODE_YUV_PIXEL_BYTES;
        uint32_t offset_out = (dec_line_cnt - 1) * g_hw_jpeg_decode_opt->output_frame->width * g_hw_jpeg_decode_opt->lines_per_block * HW_OPT_DECODE_YUV_PIXEL_BYTES;
        if(g_hw_jpeg_decode_opt->copy_method == JPEG_DECODE_OPT_COPY_METHOD_MEMCPY)
        {
            os_memcpy(g_hw_jpeg_decode_opt->output_frame->frame + offset_out,
                    g_hw_jpeg_decode_opt->sram_buffer + offset_in, g_hw_jpeg_decode_opt->output_frame->width * g_hw_jpeg_decode_opt->lines_per_block * HW_OPT_DECODE_YUV_PIXEL_BYTES);
        }
        else if(g_hw_jpeg_decode_opt->copy_method == JPEG_DECODE_OPT_COPY_METHOD_DMA)
        {
            LOGI("%s %d dma copy method is not supported now ,use memcpy instead\n", __func__, __LINE__);
            os_memcpy(g_hw_jpeg_decode_opt->output_frame->frame + offset_out,
                g_hw_jpeg_decode_opt->sram_buffer + offset_in, g_hw_jpeg_decode_opt->output_frame->width * g_hw_jpeg_decode_opt->lines_per_block * HW_OPT_DECODE_YUV_PIXEL_BYTES);
        }
    }
    else
    {
        uint32_t offset_out = (dec_line_cnt - 1) * g_hw_jpeg_decode_opt->output_frame->width * g_hw_jpeg_decode_opt->lines_per_block * HW_OPT_DECODE_YUV_PIXEL_BYTES;
        if(g_hw_jpeg_decode_opt->copy_method == JPEG_DECODE_OPT_COPY_METHOD_MEMCPY)
        {
            os_memcpy(g_hw_jpeg_decode_opt->output_frame->frame + offset_out,
                g_hw_jpeg_decode_opt->sram_buffer, g_hw_jpeg_decode_opt->output_frame->width * g_hw_jpeg_decode_opt->lines_per_block * HW_OPT_DECODE_YUV_PIXEL_BYTES);
        }
        else if(g_hw_jpeg_decode_opt->copy_method == JPEG_DECODE_OPT_COPY_METHOD_DMA)
        {
            LOGI("%s %d dma copy method is not supported now ,use memcpy instead\n", __func__, __LINE__);
            os_memcpy(g_hw_jpeg_decode_opt->output_frame->frame + offset_out,
                g_hw_jpeg_decode_opt->sram_buffer, g_hw_jpeg_decode_opt->output_frame->width * g_hw_jpeg_decode_opt->lines_per_block * HW_OPT_DECODE_YUV_PIXEL_BYTES);
        }
        jpeg_dec_ll_set_reg0x59_value((uint32_t)(g_hw_jpeg_decode_opt->sram_buffer));
        if(dec_line_cnt < g_hw_jpeg_decode_opt->output_frame->height / g_hw_jpeg_decode_opt->lines_per_block)
        {
            bk_jpeg_dec_by_line_start();
        }
    }
}

bk_err_t hw_jpeg_decode_opt_start(frame_buffer_t *src_frame, frame_buffer_t *dst_frame)
{
    bk_err_t ret = BK_OK;

    if (g_hw_jpeg_decode_opt == NULL || !g_hw_jpeg_decode_opt->hw_state)
    {
        LOGE("%s decoder not initialized\n", __func__);
        return AVDK_ERR_GENERIC;
    }

    if (g_hw_jpeg_decode_opt->hw_decode_status == HARDWARE_OPT_DECODE_STATUS_BUSY)
    {
        return BK_ERR_BUSY;
    }

    g_hw_jpeg_decode_opt->is_async = false;

    hardware_opt_decode_msg_t msg = {0};
    msg.event = HARDWARE_OPT_DECODE_EVENT_DECODE_START;
    msg.param = (uint32_t)src_frame;

    ret = rtos_push_to_queue(&g_hw_jpeg_decode_opt->hw_input_queue, &msg, BEKEN_WAIT_FOREVER);
    if (ret != BK_OK)
    {
        LOGE("%s rtos_push_to_queue failed: %d\n", __func__, ret);
        return ret;
    }

    rtos_get_semaphore(&g_hw_jpeg_decode_opt->hw_sync_sem, BEKEN_NO_WAIT);

    // Send decode start event (param=1 indicates sync mode)
    ret = hardware_opt_decode_task_send_msg(HARDWARE_OPT_DECODE_EVENT_DECODE_START, (uint32_t)dst_frame);
    if (ret != BK_OK)
    {
        LOGE("%s send msg failed: %d\n", __func__, ret);
        return ret;
    }

    // Wait for completion (ISR will handle line-by-line decode and copy automatically)
    // ISR sets semaphore when all lines are decoded
    rtos_get_semaphore(&g_hw_jpeg_decode_opt->hw_sync_sem, BEKEN_NEVER_TIMEOUT);

    // Check decode result
    if (g_hw_jpeg_decode_opt->decode_timeout == true)
    {
        ret = AVDK_ERR_HWERROR;
    }
    else if (g_hw_jpeg_decode_opt->decode_err == true)
    {
        ret = AVDK_ERR_GENERIC;
    }
    else
    {

    }
    // Always notify callbacks (success or failure)
    hw_jpeg_decode_opt_in_complete(src_frame);
    hw_jpeg_decode_opt_out_complete(PIXEL_FMT_YUYV, ret, dst_frame);

    return ret;
}

bk_err_t hw_jpeg_decode_opt_start_async(frame_buffer_t *src_frame)
{
    bk_err_t ret = BK_OK;

    if (g_hw_jpeg_decode_opt == NULL || !g_hw_jpeg_decode_opt->hw_state)
    {
        LOGE("%s decoder not initialized\n", __func__);
        return AVDK_ERR_GENERIC;
    }

    g_hw_jpeg_decode_opt->is_async = true;

    // For async mode, save request info in input_queue to avoid race condition
    // IMPORTANT: Do NOT use work_info for async as it can be overwritten by multiple async calls
    hardware_opt_decode_msg_t request = {
        .event = HARDWARE_OPT_DECODE_EVENT_DECODE_START,
        .param = (uint32_t)src_frame,
    };
    
    ret = rtos_push_to_queue(&g_hw_jpeg_decode_opt->hw_input_queue, &request, BEKEN_WAIT_FOREVER);
    if (ret != BK_OK)
    {
        LOGE("%s rtos_push_to_queue failed: %d\n", __func__, ret);
        return ret;
    }

    // Send decode start event (param=0 for async, will read from input_queue)
    ret = hardware_opt_decode_task_send_msg(HARDWARE_OPT_DECODE_EVENT_DECODE_START, 0);
    if (ret != BK_OK)
    {
        LOGE("%s send msg failed: %d\n", __func__, ret);
    }

    return ret;
}

// Hardware opt decode thread
static void hw_jpeg_decode_opt_thread(void *arg)
{
    bk_err_t ret = BK_OK;
    ret = bk_jpeg_dec_driver_init();
    if (ret != BK_OK)
    {
        LOGE("%s hw jpeg_dec_driver_init failed: %d\n", __func__, ret);
    }

    ret = rtos_init_oneshot_timer(&g_hw_jpeg_decode_opt->decode_timer,
                                   HW_OPT_DECODE_TIMEOUT_MS,
                                   bk_driver_opt_decoder_timeout,
                                   NULL,
                                   NULL);
    if (ret != BK_OK)
    {
        LOGE("%s rtos_init_oneshot_timer failed: %d\n", __func__, ret);
    }

    bk_jpeg_dec_isr_register(DEC_ERR, jpeg_dec_opt_err_cb);
    if (g_hw_jpeg_decode_opt->lines_per_block == 8)
    {
        bk_jpeg_dec_line_num_set(LINE_8);
    }
    else if (g_hw_jpeg_decode_opt->lines_per_block == 16)
    {
        bk_jpeg_dec_line_num_set(LINE_16);
    }
    else
    {
        LOGE("%s lines_per_block must be 8 or 16\n", __func__);
        return;
    }
    bk_jpeg_dec_isr_register(DEC_EVERY_LINE_INT, jpeg_dec_line_eof_cb);

    g_hw_jpeg_decode_opt->hw_state = true;

    rtos_set_semaphore(&g_hw_jpeg_decode_opt->hw_sem);

    hardware_opt_decode_msg_t msg = {0};
    while (g_hw_jpeg_decode_opt->hw_state)
    {
        ret = rtos_pop_from_queue(&g_hw_jpeg_decode_opt->hw_message_queue, &msg, BEKEN_NEVER_TIMEOUT);
        if (ret != BK_OK)
        {
            LOGE("%s pop failed: %d\n", __func__, ret);
            continue;
        }
    
        switch (msg.event)
        {
        case HARDWARE_OPT_DECODE_EVENT_DECODE_START:
        {
            frame_buffer_t *dest_frame = (frame_buffer_t *)msg.param;
            hw_jpeg_decode_opt_handle_decode_start(dest_frame);
        }
            break;
            
        case HARDWARE_OPT_DECODE_EVENT_DECODE_CONTINUE:
        {
            uint32_t dec_line_cnt = (uint32_t)msg.param;
            hw_jpeg_decode_opt_handle_decode_continue(dec_line_cnt);
        }
            break;
        
        case HARDWARE_OPT_DECODE_EVENT_DECODE_COMPLETE:
        {
            g_hw_jpeg_decode_opt->hw_decode_status = HARDWARE_OPT_DECODE_STATUS_IDLE;
            ret = (bk_err_t)msg.param;
            if (g_hw_jpeg_decode_opt->decode_timer_is_running == true)
            {
                rtos_stop_oneshot_timer(&g_hw_jpeg_decode_opt->decode_timer);
                g_hw_jpeg_decode_opt->decode_timer_is_running = false;
            }
            if(g_hw_jpeg_decode_opt->is_async)
            {
                hw_jpeg_decode_opt_out_complete(PIXEL_FMT_YUYV, ret, g_hw_jpeg_decode_opt->output_frame);
                hw_jpeg_decode_opt_in_complete(g_hw_jpeg_decode_opt->input_frame);
                g_hw_jpeg_decode_opt->input_frame = NULL;
                g_hw_jpeg_decode_opt->output_frame = NULL;
                if (!rtos_is_queue_empty(&g_hw_jpeg_decode_opt->hw_input_queue))
                {
                    ret = hardware_opt_decode_task_send_msg(HARDWARE_OPT_DECODE_EVENT_DECODE_START, 0);
                    if (ret != BK_OK)
                    {
                        LOGE("%s send msg failed: %d\n", __func__, ret);
                    }
                }
            }
            else
            {
                rtos_set_semaphore(&g_hw_jpeg_decode_opt->hw_sync_sem);
            }
        }
            break;

        case HARDWARE_OPT_DECODE_EVENT_DECODE_TIMEOUT:
        {
            g_hw_jpeg_decode_opt->hw_decode_status = HARDWARE_OPT_DECODE_STATUS_IDLE;
            if (g_hw_jpeg_decode_opt->decode_timer_is_running == true)
            {
                rtos_stop_oneshot_timer(&g_hw_jpeg_decode_opt->decode_timer);
                g_hw_jpeg_decode_opt->decode_timer_is_running = false;
            }
            if (g_hw_jpeg_decode_opt->is_async)
            {
                hw_jpeg_decode_opt_out_complete(PIXEL_FMT_YUYV, AVDK_ERR_HWERROR, g_hw_jpeg_decode_opt->output_frame);
                hw_jpeg_decode_opt_in_complete(g_hw_jpeg_decode_opt->input_frame);
                g_hw_jpeg_decode_opt->input_frame = NULL;
                g_hw_jpeg_decode_opt->output_frame = NULL;

                if (!rtos_is_queue_empty(&g_hw_jpeg_decode_opt->hw_input_queue))
                {
                    ret = hardware_opt_decode_task_send_msg(HARDWARE_OPT_DECODE_EVENT_DECODE_START, 0);
                    if (ret != BK_OK)
                    {
                        LOGE("%s send msg failed: %d\n", __func__, ret);
                    }
                }
            }
            else
            {
                rtos_set_semaphore(&g_hw_jpeg_decode_opt->hw_sync_sem);
            }
        }
            break;
        case HARDWARE_OPT_DECODE_EVENT_EXIT:
            g_hw_jpeg_decode_opt->hw_state = false;
            break;
            
        default:
            break;
        }
    }

    bk_jpeg_dec_driver_deinit();
    rtos_set_semaphore(&g_hw_jpeg_decode_opt->hw_sem);
    rtos_delete_thread(NULL);
}

// Destroy hardware opt decoder resources
static void hw_jpeg_decode_opt_destroy(void)
{
    if (g_hw_jpeg_decode_opt)
    {
        bk_err_t ret = BK_OK;
        
        if (g_hw_jpeg_decode_opt->decode_timer_is_running == true)
        {
            rtos_stop_oneshot_timer(&g_hw_jpeg_decode_opt->decode_timer);
            g_hw_jpeg_decode_opt->decode_timer_is_running = false;
        }
        if (rtos_is_oneshot_timer_init(&g_hw_jpeg_decode_opt->decode_timer))
        {
            rtos_deinit_oneshot_timer(&g_hw_jpeg_decode_opt->decode_timer);
        }
        if (g_hw_jpeg_decode_opt->hw_sem != NULL)
        {
            rtos_deinit_semaphore(&g_hw_jpeg_decode_opt->hw_sem);
            g_hw_jpeg_decode_opt->hw_sem = NULL;
        }
        if (g_hw_jpeg_decode_opt->hw_sync_sem != NULL)
        {
            rtos_deinit_semaphore(&g_hw_jpeg_decode_opt->hw_sync_sem);
            g_hw_jpeg_decode_opt->hw_sync_sem = NULL;
        }
        if (g_hw_jpeg_decode_opt->hw_input_queue != NULL)
        {
            // Clean up any pending async requests
            while (!rtos_is_queue_empty(&g_hw_jpeg_decode_opt->hw_input_queue))
            {
                hw_opt_decode_request_t request = {0};
                ret = rtos_pop_from_queue(&g_hw_jpeg_decode_opt->hw_input_queue, &request, BEKEN_NO_WAIT);
                if (ret == BK_OK && request.src_frame != NULL)
                {
                    LOGW("%s cleaning up pending async request\n", __func__);
                    hw_jpeg_decode_opt_in_complete(request.src_frame);
                }
            }
            rtos_deinit_queue(&g_hw_jpeg_decode_opt->hw_input_queue);
            g_hw_jpeg_decode_opt->hw_input_queue = NULL;
        }
        if (g_hw_jpeg_decode_opt->hw_message_queue != NULL)
        {
            while (!rtos_is_queue_empty(&g_hw_jpeg_decode_opt->hw_message_queue))
            {
                hardware_opt_decode_msg_t msg = {0};
                ret = rtos_pop_from_queue(&g_hw_jpeg_decode_opt->hw_message_queue, &msg, BEKEN_NO_WAIT);
                if (ret == BK_OK)
                {
                    if (msg.event == HARDWARE_OPT_DECODE_EVENT_DECODE_START)
                    {
                        frame_buffer_t *out_frame = (frame_buffer_t *)msg.param;
                        if (out_frame != NULL)
                        {
                            hw_jpeg_decode_opt_out_complete(PIXEL_FMT_YUYV, BK_FAIL, out_frame);
                        }
                    }
                }
            }
            rtos_deinit_queue(&g_hw_jpeg_decode_opt->hw_message_queue);
            g_hw_jpeg_decode_opt->hw_message_queue = NULL;
        }

        os_free(g_hw_jpeg_decode_opt);
        g_hw_jpeg_decode_opt = NULL;
    }
}

bk_err_t hw_jpeg_decode_opt_init(hw_opt_decode_init_config_t *config)
{
    bk_err_t ret = BK_OK;

    if (g_hw_jpeg_decode_opt != NULL)
    {
        LOGE("%s already initialized\n", __func__);
        return BK_ERR_STATE;
    }

    if (config == NULL)
    {
        LOGE("%s config is NULL\n", __func__);
        return BK_ERR_PARAM;
    }

    if (config->decode_cbs == NULL)
    {
        LOGE("%s decode_cbs is NULL\n", __func__);
        return BK_ERR_PARAM;
    }

    g_hw_jpeg_decode_opt = (bk_hw_jpeg_decode_opt_t *)os_malloc(sizeof(bk_hw_jpeg_decode_opt_t));
    if (g_hw_jpeg_decode_opt == NULL)
    {
        LOGE("%s malloc failed\n", __func__);
        return BK_ERR_NO_MEM;
    }
    os_memset(g_hw_jpeg_decode_opt, 0, sizeof(bk_hw_jpeg_decode_opt_t));

    g_hw_jpeg_decode_opt->sram_buffer = config->sram_buffer;
    g_hw_jpeg_decode_opt->decode_cbs = config->decode_cbs;
    g_hw_jpeg_decode_opt->hw_decode_status = HARDWARE_OPT_DECODE_STATUS_IDLE;
    g_hw_jpeg_decode_opt->lines_per_block = config->lines_per_block;
    g_hw_jpeg_decode_opt->is_pingpong = config->is_pingpong;
    g_hw_jpeg_decode_opt->copy_method = config->copy_method;
    
    // Initialize semaphores
    ret = rtos_init_semaphore(&g_hw_jpeg_decode_opt->hw_sem, 1);
    if (ret != BK_OK)
    {
        LOGE("%s hw_sem init failed: %d\n", __func__, ret);
        goto error;
    }

    ret = rtos_init_semaphore(&g_hw_jpeg_decode_opt->hw_sync_sem, 1);
    if (ret != BK_OK)
    {
        LOGE("%s hw_sync_sem init failed: %d\n", __func__, ret);
        goto error;
    }

    // Initialize queues
    ret = rtos_init_queue(&g_hw_jpeg_decode_opt->hw_message_queue,
                          "hw_opt_msg_queue",
                          sizeof(hardware_opt_decode_msg_t),
                          HW_OPT_DECODE_MSG_QUEUE_SIZE);
    if (ret != BK_OK)
    {
        LOGE("%s hw_message_queue init failed: %d\n", __func__, ret);
        goto error;
    }

    ret = rtos_init_queue(&g_hw_jpeg_decode_opt->hw_input_queue,
                          "hw_opt_input_queue",
                          sizeof(hw_opt_decode_request_t),
                          HW_OPT_DECODE_INPUT_QUEUE_SIZE);
    if (ret != BK_OK)
    {
        LOGE("%s hw_input_queue init failed: %d\n", __func__, ret);
        goto error;
    }

    // Create decode thread
    ret = rtos_create_thread(&g_hw_jpeg_decode_opt->hw_thread,
                             BEKEN_DEFAULT_WORKER_PRIORITY,
                             "hw_opt_dec",
                             hw_jpeg_decode_opt_thread,
                             CONFIG_HW_JPEG_DECODE_OPT_TASK_STACK_SIZE,
                             NULL);
    if (ret != BK_OK)
    {
        LOGE("%s create thread failed: %d\n", __func__, ret);
        goto error;
    }

    // Wait for thread initialization
    ret = rtos_get_semaphore(&g_hw_jpeg_decode_opt->hw_sem, BEKEN_WAIT_FOREVER);
    if (ret != BK_OK)
    {
        LOGE("%s get semaphore failed: %d\n", __func__, ret);
        goto error;
    }

    LOGI("%s success\n", __func__);
    return BK_OK;

error:
    hw_jpeg_decode_opt_destroy();
    return ret;
}

bk_err_t hw_jpeg_decode_opt_deinit(void)
{
    bk_err_t ret = BK_OK;

    if (g_hw_jpeg_decode_opt == NULL)
    {
        LOGE("%s not initialized\n", __func__);
        return BK_ERR_STATE;
    }

    // Send exit message to thread
    ret = hardware_opt_decode_task_send_msg(HARDWARE_OPT_DECODE_EVENT_EXIT, 0);
    if (ret != BK_OK)
    {
        LOGE("%s send exit msg failed: %d\n", __func__, ret);
    }

    // Wait for thread to exit
    ret = rtos_get_semaphore(&g_hw_jpeg_decode_opt->hw_sem, BEKEN_WAIT_FOREVER);
    if (ret != BK_OK)
    {
        LOGE("%s get semaphore failed: %d\n", __func__, ret);
    }

    hw_jpeg_decode_opt_destroy();

    LOGI("%s success\n", __func__);
    return BK_OK;
}
