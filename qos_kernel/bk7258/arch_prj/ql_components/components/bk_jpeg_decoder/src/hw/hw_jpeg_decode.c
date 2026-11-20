#include <os/mem.h>
#include <os/str.h>

#include <driver/jpeg_dec.h>
#include <driver/jpeg_dec_types.h>
#include <driver/timer.h>

#include <components/media_types.h>
#include "yuv_encode.h"
#include "hw_jpeg_decode.h"
#include "components/bk_jpeg_decode/bk_jpeg_decode_utils.h"

#ifndef CONFIG_HW_JPEG_DECODE_TASK_STACK_SIZE
#define CONFIG_HW_JPEG_DECODE_TASK_STACK_SIZE (1024)
#endif

#ifndef CONFIG_HW_JPEG_DECODE_TASK_PRIORITY
#define CONFIG_HW_JPEG_DECODE_TASK_PRIORITY (6)
#endif

#define TAG "hw_dec"

#define HW_DECODE_TIMEOUT_MS        200
#define HW_DECODE_MSG_QUEUE_SIZE    20
#define HW_DECODE_INPUT_QUEUE_SIZE  10
#define HW_DECODE_YUV_PIXEL_BYTES   2

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

static bk_hw_jpeg_decode_t *g_hw_jpeg_decode;

static void bk_driver_decoder_timeout(void *arg1, void *arg2)
{
    bk_err_t ret = BK_FAIL;
    LOGE("%s %d\n", __func__, __LINE__);
    bk_jpeg_dec_stop();

    // Check global pointer before accessing
    if (g_hw_jpeg_decode == NULL)
    {
        LOGE("%s %d CRITICAL: g_hw_jpeg_decode is NULL in ISR!\n", __func__, __LINE__);
        return;
    }

    g_hw_jpeg_decode->hw_decode_status = HARDWARE_DECODE_STATUS_IDLE;
    g_hw_jpeg_decode->decode_timeout = true;
    g_hw_jpeg_decode->decode_timer_is_running = false;

    ret = rtos_set_semaphore(&g_hw_jpeg_decode->hw_sync_sem);
    if (ret != BK_OK)
    {
        LOGE("%s %d semaphore set failed: %d\n", __func__, __LINE__, ret);
    }
}

static void jpeg_dec_err_cb(jpeg_dec_res_t *result)
{
    bk_err_t ret = BK_FAIL;
    LOGE("%s %d\n", __func__, __LINE__);

    if (g_hw_jpeg_decode->decode_timer_is_running == true)
    {
        rtos_stop_oneshot_timer(&g_hw_jpeg_decode->decode_timer);
        g_hw_jpeg_decode->decode_timer_is_running = false;
    }
    // Check global pointer before accessing
    if (g_hw_jpeg_decode == NULL)
    {
        LOGE("%s %d CRITICAL: g_hw_jpeg_decode is NULL in ISR!\n", __func__, __LINE__);
        return;
    }
    g_hw_jpeg_decode->decode_err = true;
    g_hw_jpeg_decode->hw_decode_status = HARDWARE_DECODE_STATUS_IDLE;
    ret = rtos_set_semaphore(&g_hw_jpeg_decode->hw_sync_sem);
    if (ret != BK_OK)
    {
        LOGE("%s %d semaphore set failed: %d\n", __func__, __LINE__, ret);
    }
}

static void jpeg_dec_eof_cb(jpeg_dec_res_t *result)
{
    bk_err_t ret = BK_FAIL;
    if (g_hw_jpeg_decode->decode_timer_is_running == true)
    {
        rtos_stop_oneshot_timer(&g_hw_jpeg_decode->decode_timer);
        g_hw_jpeg_decode->decode_timer_is_running = false;
    }
    // Check global pointer before accessing
    if (g_hw_jpeg_decode == NULL)
    {
        LOGE("%s %d CRITICAL: g_hw_jpeg_decode is NULL in ISR!\n", __func__, __LINE__);
        return;
    }

    if (result->ok == false)
    {
        g_hw_jpeg_decode->decode_err = true;
        LOGE("%s %d decoder error\n", __func__, __LINE__);
    }
    g_hw_jpeg_decode->hw_decode_status = HARDWARE_DECODE_STATUS_IDLE;
    ret = rtos_set_semaphore(&g_hw_jpeg_decode->hw_sync_sem);
    if (ret != BK_OK)
    {
        LOGE("%s %d semaphore set failed: %d\n", __func__, __LINE__, ret);
    }
}

static void hw_jpeg_decode_out_complete(uint32_t format_type, uint32_t result, frame_buffer_t *out_frame)
{
    LOGV("%s %d out_frame %p\n", __func__, __LINE__, out_frame);
    if (g_hw_jpeg_decode->decode_cbs != NULL && g_hw_jpeg_decode->decode_cbs->out_complete != NULL)
    {
        g_hw_jpeg_decode->decode_cbs->out_complete(format_type, result, out_frame);
    }
	else
	{
		LOGE("%s %d out_complete is NULL\n", __func__, __LINE__);
	}
}

static void hw_jpeg_decode_in_complete(frame_buffer_t *in_frame)
{
    LOGV("%s %d in_frame %p\n", __func__, __LINE__, in_frame);
    if (g_hw_jpeg_decode->decode_cbs != NULL && g_hw_jpeg_decode->decode_cbs->in_complete != NULL)
    {
        g_hw_jpeg_decode->decode_cbs->in_complete(in_frame);
    }
    else
    {
        LOGE("%s %d in_complete is NULL\n", __func__, __LINE__);
    }
}

static frame_buffer_t *hw_jpeg_decode_out_malloc(uint32_t size)
{
    frame_buffer_t *out_frame = NULL;
    if (g_hw_jpeg_decode->decode_cbs != NULL && g_hw_jpeg_decode->decode_cbs->out_malloc != NULL)
    {
        out_frame = g_hw_jpeg_decode->decode_cbs->out_malloc(size);
    }
    LOGV("%s %d %p\n", __func__, __LINE__, out_frame);
    return out_frame;
}


static bk_err_t hardware_decode_task_send_msg(hardware_decode_event_type_t event, uint32_t param)
{
	int ret = BK_OK;
	hardware_decode_msg_t msg = {0};

	if (g_hw_jpeg_decode && g_hw_jpeg_decode->hw_state && g_hw_jpeg_decode->hw_message_queue)
	{
		msg.event = event;
		msg.param = param;

		ret = rtos_push_to_queue(&g_hw_jpeg_decode->hw_message_queue, &msg, BEKEN_WAIT_FOREVER);
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

static bk_err_t hw_jpeg_decode_start_decode(frame_buffer_t *src_frame, frame_buffer_t *dst_frame)
{
    int ret = BK_OK;
    g_hw_jpeg_decode->decode_timeout = false;
    g_hw_jpeg_decode->decode_err = false;
    bk_jpeg_dec_out_format(PIXEL_FMT_YUYV);
    dst_frame->fmt = PIXEL_FMT_YUYV;
    g_hw_jpeg_decode->hw_decode_status = HARDWARE_DECODE_STATUS_BUSY;
    ret = bk_jpeg_dec_hw_start(src_frame->length, src_frame->frame, dst_frame->frame);

    if (ret != BK_OK)
    {
        LOGD("%s, length:%d\r\n", __func__, src_frame->length);
        return ret;
    }

    return ret;
}

bk_err_t hw_jpeg_decode_start(frame_buffer_t *src_frame, frame_buffer_t *dst_frame)
{
    bk_err_t ret = BK_OK;

    if (g_hw_jpeg_decode && g_hw_jpeg_decode->hw_state)
    {
        if (g_hw_jpeg_decode->hw_decode_status == HARDWARE_DECODE_STATUS_BUSY)
        {
            return BK_ERR_BUSY;
        }
        hardware_decode_msg_t msg = {0};
        msg.event = HARDWARE_DECODE_EVENT_DECODE_START;
        msg.param = (uint32_t)src_frame;
        ret = rtos_push_to_queue(&g_hw_jpeg_decode->hw_input_queue, &msg, BEKEN_WAIT_FOREVER);
        if (ret != BK_OK)
        {
            LOGE("%s rtos_push_to_queue failed: %d\n", __func__, ret);
            return ret;
        }

        rtos_get_semaphore(&g_hw_jpeg_decode->hw_sync_sem, BEKEN_NO_WAIT);
        ret = hardware_decode_task_send_msg(HARDWARE_DECODE_EVENT_DECODE_START, (uint32_t)dst_frame);
        if (ret != BK_OK)
        {
            LOGE("%s send msg failed: %d\n", __func__, ret);
            return ret;
        }

        ret = rtos_get_semaphore(&g_hw_jpeg_decode->hw_sync_sem, BEKEN_WAIT_FOREVER);
        if (g_hw_jpeg_decode->decode_timer_is_running == true)
        {
            rtos_stop_oneshot_timer(&g_hw_jpeg_decode->decode_timer);
            g_hw_jpeg_decode->decode_timer_is_running = false;
        }
        if (g_hw_jpeg_decode->decode_timeout == true)
        {
            ret = AVDK_ERR_HWERROR;
        }
        if (g_hw_jpeg_decode->decode_err == true)
        {
            ret = AVDK_ERR_GENERIC;
        }
        if (ret == BK_OK)
        {
            hw_jpeg_decode_in_complete(src_frame);
            hw_jpeg_decode_out_complete(PIXEL_FMT_YUYV, ret, dst_frame);
        }
    }
    else
    {
        ret = AVDK_ERR_GENERIC;
    }
    return ret;
}

bk_err_t hw_jpeg_decode_start_async(frame_buffer_t *src_frame)
{
    bk_err_t ret = BK_OK;

    if (g_hw_jpeg_decode && g_hw_jpeg_decode->hw_state)
    {
        hardware_decode_msg_t msg = {0};
        msg.event = HARDWARE_DECODE_EVENT_DECODE_START;
        msg.param = (uint32_t)src_frame;
        ret = rtos_push_to_queue(&g_hw_jpeg_decode->hw_input_queue, &msg, BEKEN_WAIT_FOREVER);
        if (ret != BK_OK)
        {
            LOGE("%s rtos_push_to_queue failed: %d\n", __func__, ret);
            return ret;
        }

        ret = hardware_decode_task_send_msg(HARDWARE_DECODE_EVENT_DECODE_START, 0);
        if (ret != BK_OK)
        {
            LOGE("%s send msg failed: %d\n", __func__, ret);
        }
    }
    else
    {
        ret = AVDK_ERR_GENERIC;
    }
    return ret;
}

static void hw_jpeg_decode_thread(void *arg)
{
    bk_err_t ret = BK_OK;
    ret = bk_jpeg_dec_driver_init();
    if (ret != BK_OK)
    {
        LOGE("%s hw jpeg_dec_driver_init failed: %d\n", __func__, ret);
    }

    bk_jpeg_dec_isr_register(DEC_ERR, jpeg_dec_err_cb);
    bk_jpeg_dec_isr_register(DEC_END_OF_FRAME, jpeg_dec_eof_cb);

    g_hw_jpeg_decode->hw_state = true;

    rtos_set_semaphore(&g_hw_jpeg_decode->hw_sem);

    hardware_decode_msg_t msg = {0};
    while (g_hw_jpeg_decode->hw_state)
    {
        ret = rtos_pop_from_queue(&g_hw_jpeg_decode->hw_message_queue, &msg, BEKEN_NEVER_TIMEOUT);
        if (ret != BK_OK)
        {
            LOGE("%s pop failed: %d\n", __func__, ret);
            continue;
        }
        switch (msg.event)
        {
        case HARDWARE_DECODE_EVENT_DECODE_START:
        {
            frame_buffer_t *out_frame = (frame_buffer_t *)msg.param;
            frame_buffer_t *in_frame = NULL;
            if (out_frame != NULL)
            {
                //同步解码，等待在hw_jpeg_decode_start函数中
                hardware_decode_msg_t msg_temp = {0};
                ret = rtos_pop_from_queue(&g_hw_jpeg_decode->hw_input_queue, &msg_temp, BEKEN_NO_WAIT);
                if (ret != BK_OK)
                {
                    hw_jpeg_decode_out_complete(PIXEL_FMT_YUYV, ret, out_frame);
                    break;
                }
                in_frame = (frame_buffer_t *)msg_temp.param;
                g_hw_jpeg_decode->decode_timer_is_running = true;
                rtos_start_oneshot_timer(&g_hw_jpeg_decode->decode_timer);
                ret = hw_jpeg_decode_start_decode(in_frame, out_frame);
                if (ret != BK_OK)
                {
                    LOGE("%s decode start failed: %d\n", __func__, ret);
                    hw_jpeg_decode_in_complete(in_frame);
                    hw_jpeg_decode_out_complete(PIXEL_FMT_YUYV, ret, out_frame);
                    rtos_stop_oneshot_timer(&g_hw_jpeg_decode->decode_timer);
                    g_hw_jpeg_decode->decode_timer_is_running = false;
                    rtos_set_semaphore(&g_hw_jpeg_decode->hw_sync_sem);
                }
            }
            else
            {
                //异步解码，在硬解码task中等待
                hardware_decode_msg_t msg_temp = {0};
                ret = rtos_pop_from_queue(&g_hw_jpeg_decode->hw_input_queue, &msg_temp, BEKEN_NO_WAIT);
                if (ret != BK_OK)
                {
                    break;
                }
                in_frame = (frame_buffer_t *)msg_temp.param;
                bk_jpeg_decode_img_info_t img_info = {0};
                img_info.frame = in_frame;
                ret = bk_get_jpeg_data_info(&img_info);
                if (ret != AVDK_ERR_OK)
                {
                    LOGE(" %s %d bk_get_jpeg_data_info failed %d\n", __func__, __LINE__, ret);
                    hw_jpeg_decode_in_complete(in_frame);
                    break;
                }

                // Set the image dimensions from parsed JPEG info to input and output frame buffers
                in_frame->width = img_info.width;
                in_frame->height = img_info.height;

                out_frame = hw_jpeg_decode_out_malloc(img_info.width * img_info.height * HW_DECODE_YUV_PIXEL_BYTES);
                if (out_frame == NULL)
                {
                    LOGE("%s %d out_malloc failed\n", __func__, __LINE__);
                    hw_jpeg_decode_in_complete(in_frame);
                    break;
                }
                out_frame->width = img_info.width;
                out_frame->height = img_info.height;

                g_hw_jpeg_decode->decode_timer_is_running = true;
                rtos_start_oneshot_timer(&g_hw_jpeg_decode->decode_timer);
                ret = hw_jpeg_decode_start_decode(in_frame, out_frame);
                if (ret != BK_OK)
                {
                    LOGE("%s decode start failed: %d\n", __func__, ret);
                    hw_jpeg_decode_in_complete(in_frame);
                    hw_jpeg_decode_out_complete(PIXEL_FMT_YUYV, ret, out_frame);
                    rtos_stop_oneshot_timer(&g_hw_jpeg_decode->decode_timer);
                    g_hw_jpeg_decode->decode_timer_is_running = false;
                    break;
                }

                ret = rtos_get_semaphore(&g_hw_jpeg_decode->hw_sync_sem, BEKEN_NEVER_TIMEOUT);

                if (g_hw_jpeg_decode->decode_timer_is_running == true)
                {
                    rtos_stop_oneshot_timer(&g_hw_jpeg_decode->decode_timer);
                    g_hw_jpeg_decode->decode_timer_is_running = false;
                }
                if (g_hw_jpeg_decode->decode_timeout == true)
                {
                    ret = AVDK_ERR_HWERROR;
                }
                if (g_hw_jpeg_decode->decode_err == true)
                {
                    ret = AVDK_ERR_GENERIC;
                }
                hw_jpeg_decode_in_complete(in_frame);
                hw_jpeg_decode_out_complete(PIXEL_FMT_YUYV, ret, out_frame);
            }
        }
            break;
        case HARDWARE_DECODE_EVENT_EXIT:
            g_hw_jpeg_decode->hw_state = false;
            break;
        default:
            break;
        }
    }

    bk_jpeg_dec_driver_deinit();
    rtos_set_semaphore(&g_hw_jpeg_decode->hw_sem);
    rtos_delete_thread(NULL);
}

static void hw_jpeg_decode_destory(void)
{
    if (g_hw_jpeg_decode)
    {
        if (g_hw_jpeg_decode->decode_timer_is_running == true)
        {
            rtos_stop_oneshot_timer(&g_hw_jpeg_decode->decode_timer);
            g_hw_jpeg_decode->decode_timer_is_running = false;
        }
        if (rtos_is_oneshot_timer_init(&g_hw_jpeg_decode->decode_timer))
        {
            rtos_deinit_oneshot_timer(&g_hw_jpeg_decode->decode_timer);
        }
        if (g_hw_jpeg_decode->hw_sem != NULL)
        {
            rtos_deinit_semaphore(&g_hw_jpeg_decode->hw_sem);
            g_hw_jpeg_decode->hw_sem = NULL;
        }
        if (g_hw_jpeg_decode->hw_sync_sem != NULL)
        {
            rtos_deinit_semaphore(&g_hw_jpeg_decode->hw_sync_sem);
            g_hw_jpeg_decode->hw_sync_sem = NULL;
        }
        if (g_hw_jpeg_decode->hw_input_queue != NULL)
        {
            bk_err_t ret = BK_OK;
            while (!rtos_is_queue_empty(&g_hw_jpeg_decode->hw_input_queue))
            {
                hardware_decode_msg_t msg = {0};
                ret = rtos_pop_from_queue(&g_hw_jpeg_decode->hw_input_queue, &msg, BEKEN_NO_WAIT);
                if (ret == BK_OK)
                {
                    frame_buffer_t *in_frame = (frame_buffer_t *)msg.param;
                    hw_jpeg_decode_in_complete(in_frame);
                }
            }
            rtos_deinit_queue(&g_hw_jpeg_decode->hw_input_queue);
            g_hw_jpeg_decode->hw_input_queue = NULL;
        }
        if (g_hw_jpeg_decode->hw_message_queue != NULL)
        {
            bk_err_t ret = BK_OK;
            while (!rtos_is_queue_empty(&g_hw_jpeg_decode->hw_message_queue))
            {
                hardware_decode_msg_t msg = {0};
                ret = rtos_pop_from_queue(&g_hw_jpeg_decode->hw_message_queue, &msg, BEKEN_NO_WAIT);
                if (ret == BK_OK)
                {
                    if (msg.event == HARDWARE_DECODE_EVENT_DECODE_START)
                    {
                        frame_buffer_t *out_frame = (frame_buffer_t *)msg.param;
                        if (out_frame != NULL)
                        {
                            hw_jpeg_decode_out_complete(PIXEL_FMT_YUYV, BK_FAIL, out_frame);
                        }
                    }
                }
            }

            rtos_deinit_queue(&g_hw_jpeg_decode->hw_message_queue);
            g_hw_jpeg_decode->hw_message_queue = NULL;
        }
        g_hw_jpeg_decode->hw_thread = NULL;
        os_free(g_hw_jpeg_decode);
        g_hw_jpeg_decode = NULL;
    }
}

bk_err_t hw_jpeg_decode_init(bk_jpeg_decode_callback_t *decode_cbs)
{
    bk_err_t ret = BK_OK;
    if (g_hw_jpeg_decode && g_hw_jpeg_decode->hw_state != false)
    {
        LOGD("%s, already init\n", __func__);
        return ret;
    }

    g_hw_jpeg_decode = os_malloc(sizeof(bk_hw_jpeg_decode_t));
    if (g_hw_jpeg_decode == NULL)
    {
        LOGE("%s %d g_hw_jpeg_decode malloc failed\n", __func__, __LINE__); 
        return BK_ERR_NO_MEM;
    }
    os_memset(g_hw_jpeg_decode, 0, sizeof(bk_hw_jpeg_decode_t));

    g_hw_jpeg_decode->decode_cbs = decode_cbs;

    ret = rtos_init_semaphore(&g_hw_jpeg_decode->hw_sem, 1);
    if (ret != BK_OK)
    {
        LOGE("%s hw_sem init failed: %d\n", __func__, ret);
        return ret;
    }

    ret = rtos_init_semaphore(&g_hw_jpeg_decode->hw_sync_sem, 1);
    if (ret != BK_OK)
    {
        LOGE("%s hw_sync_sem init failed: %d\n", __func__, ret);
        return ret;
    }

    ret = rtos_init_queue(&g_hw_jpeg_decode->hw_message_queue, "hw_msg_queue", sizeof(hardware_decode_msg_t), HW_DECODE_MSG_QUEUE_SIZE);
    if (ret != BK_OK)
    {
        LOGE("%s hw_message_queue init failed: %d\n", __func__, ret);
        goto error;
    }

    ret = rtos_init_queue(&g_hw_jpeg_decode->hw_input_queue, "hw_input_queue", sizeof(hardware_decode_msg_t), HW_DECODE_INPUT_QUEUE_SIZE);
    if (ret != BK_OK)
    {
        LOGE("%s hw_input_queue init failed: %d\n", __func__, ret);
        goto error;
    }

    ret = rtos_init_oneshot_timer(&g_hw_jpeg_decode->decode_timer, HW_DECODE_TIMEOUT_MS, bk_driver_decoder_timeout, NULL, NULL);
    if (ret != BK_OK)
    {
        LOGE("%s hw decode_timer init failed: %d\n", __func__, ret);
        goto error;
    }

	ret = rtos_create_thread(&g_hw_jpeg_decode->hw_thread,
                            CONFIG_HW_JPEG_DECODE_TASK_PRIORITY,
							"hw_dec_thread",
							(beken_thread_function_t)hw_jpeg_decode_thread,
						    CONFIG_HW_JPEG_DECODE_TASK_STACK_SIZE,
							NULL);
    if (ret != BK_OK)
    {
        LOGE("%s hw rtos_create_thread init failed: %d\n", __func__, ret);
        goto error;
    }

    rtos_get_semaphore(&g_hw_jpeg_decode->hw_sem, BEKEN_WAIT_FOREVER);

    return ret;

error:
    hw_jpeg_decode_destory();
    return ret;
}

bk_err_t hw_jpeg_decode_deinit(void)
{
    bk_err_t ret = BK_OK;
    if (g_hw_jpeg_decode == NULL || g_hw_jpeg_decode->hw_state == false)
    {
        LOGV("%s, already deinit\n", __func__);
        return ret;
    }
    LOGD("%s \r\n", __func__);

    hardware_decode_task_send_msg(HARDWARE_DECODE_EVENT_EXIT, 0);
    ret = rtos_get_semaphore(&g_hw_jpeg_decode->hw_sem, BEKEN_WAIT_FOREVER);

    g_hw_jpeg_decode->hw_state = false;
    hw_jpeg_decode_destory();
    return ret;
}
