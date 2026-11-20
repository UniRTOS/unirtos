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

#include <driver/jpeg_dec.h>
#include <modules/jpeg_decode_sw.h>
#include <modules/tjpgd.h>

#include "media_evt.h"
#include "frame_buffer.h"
#include "sw_jpeg_decode_cp1.h"
#include "sw_jpeg_decode_cp2.h"
#include "components/bk_jpeg_decode/bk_jpeg_decode_utils.h"

#include "mux_pipeline.h"

#ifndef CONFIG_SW_JPEG_DECODE_TASK_STACK_SIZE
#define CONFIG_SW_JPEG_DECODE_TASK_STACK_SIZE (1024)
#endif

#define TAG "sw_dec"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

static void sw_jpeg_decode_out_complete(bk_jpeg_decode_sw_out_format_t format_type, uint32_t result, frame_buffer_t *out_frame, private_jpeg_decode_sw_multi_core_ctlr_t *controller)
{
    LOGV("%s %d out_frame %p\n", __func__, __LINE__, out_frame);
    if (controller->config.decode_cbs.out_complete != NULL)
    {
        controller->config.decode_cbs.out_complete(format_type, result, out_frame);
    }
	else
	{
		LOGE("%s %d out_complete is NULL\n", __func__, __LINE__);
	}
}

static void sw_jpeg_decode_in_complete(frame_buffer_t *in_frame, private_jpeg_decode_sw_multi_core_ctlr_t *controller)
{
    LOGV("%s %d in_frame %p\n", __func__, __LINE__, in_frame);
    if (controller->config.decode_cbs.in_complete != NULL)
    {
        controller->config.decode_cbs.in_complete(in_frame);
    }
    else
    {
        LOGE("%s %d in_complete is NULL\n", __func__, __LINE__);
    }
}

static frame_buffer_t *sw_jpeg_decode_out_malloc(private_jpeg_decode_sw_multi_core_ctlr_t *controller, uint32_t size)
{
    frame_buffer_t *out_frame = NULL;
    if (controller->config.decode_cbs.out_malloc != NULL)
    {
        out_frame = controller->config.decode_cbs.out_malloc(size);
    }
    LOGV("%s %d %p\n", __func__, __LINE__, out_frame);
	return out_frame;
}

static bk_err_t software_decode_task_dual_core_send_msg(software_decode_event_type_t event, uint32_t param, private_jpeg_decode_sw_multi_core_ctlr_t *controller)
{
	int ret = BK_OK;
	software_decode_msg_t msg = {0};

	if (controller && controller->task_running && controller->message_queue)
	{
		msg.event = event;
		msg.param = param;

		ret = rtos_push_to_queue(&controller->message_queue, &msg, BEKEN_WAIT_FOREVER);

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

/**
 * @brief CP1 decoder callback function
 *
 * @param format_type Format type
 * @param result Decoding result
 * @param out_frame Output frame
 * @return bk_err_t Operation result
 */
static bk_err_t cp1_decode_complete(uint32_t format_type, uint32_t result, frame_buffer_t *out_frame, frame_buffer_t *in_frame, void *context)
{
    LOGV("CP decode complete 1, format: %d, result: %d %p\n", format_type, result, out_frame);
    private_jpeg_decode_sw_multi_core_ctlr_t *controller = (private_jpeg_decode_sw_multi_core_ctlr_t *)context;

    if (controller->config.core_id == (JPEG_DECODE_CORE_ID_1 | JPEG_DECODE_CORE_ID_2))
    {
        sw_jpeg_decode_in_complete(in_frame, controller);
        if (result != BK_OK)
        {
            sw_jpeg_decode_out_complete(format_type, result, out_frame, controller);
            controller->sw_dec_info[0].in_frame = NULL;
            controller->sw_dec_info[0].out_frame = NULL;
            controller->sw_dec_info[0].complete = NULL;
            controller->cp1_busy = 0;
        }
        else
        {
            software_decode_task_dual_core_send_msg(SOFTWARE_DECODE_EVENT_DECODE_COMPLETE, JPEG_DECODE_CORE_ID_1, controller);
        }
        software_decode_task_dual_core_send_msg(SOFTWARE_DECODE_EVENT_DECODE_START, 0, controller);
    }
    else
    {
        LOGE("%s %d core_id %d is not support\n", __func__, __LINE__, controller->config.core_id);
    }

    return BK_OK;
}

/**
 * @brief CP2 decoder callback function
 *
 * @param format_type Format type
 * @param result Decoding result
 * @param out_frame Output frame
 * @return bk_err_t Operation result
 */
static bk_err_t cp2_decode_complete(uint32_t format_type, uint32_t result, frame_buffer_t *out_frame, frame_buffer_t *in_frame, void *context)
{
    LOGV("CP decode complete 2, format: %d, result: %d %p\n", format_type, result, out_frame);
    private_jpeg_decode_sw_multi_core_ctlr_t *controller = (private_jpeg_decode_sw_multi_core_ctlr_t *)context;

    if (controller->config.core_id == (JPEG_DECODE_CORE_ID_1 | JPEG_DECODE_CORE_ID_2))
    {
        sw_jpeg_decode_in_complete(in_frame, controller);
        if (result != BK_OK)
        {
            sw_jpeg_decode_out_complete(format_type, result, out_frame, controller);
            controller->sw_dec_info[1].in_frame = NULL;
            controller->sw_dec_info[1].out_frame = NULL;
            controller->sw_dec_info[1].complete = NULL;
            controller->cp2_busy = 0;
        }
        else
        {
            software_decode_task_dual_core_send_msg(SOFTWARE_DECODE_EVENT_DECODE_COMPLETE, JPEG_DECODE_CORE_ID_2, controller);
        }
        software_decode_task_dual_core_send_msg(SOFTWARE_DECODE_EVENT_DECODE_START, 0, controller);
    }
	else
    {
        LOGE("%s %d core_id %d is not support\n", __func__, __LINE__, controller->config.core_id);
    }

    return BK_OK;
}

static bk_err_t software_decode_dual_core_decode(private_jpeg_decode_sw_multi_core_ctlr_t *controller)
{
	bk_err_t ret = BK_OK;
	frame_buffer_t *in_frame = NULL;
	frame_buffer_t *out_frame = NULL;
	uint8_t is_cp2 = 0;

	if (controller->cp2_busy == 0)
	{
		uint32_t frame_ptr = 0;
		uint32_t alloc_size = 0;
		is_cp2 = 1;

		ret = rtos_pop_from_queue(&controller->input_queue, &frame_ptr, BEKEN_NO_WAIT);
		if (ret != BK_OK) {
			goto error;
		}

		in_frame = (frame_buffer_t *)frame_ptr;
		bk_jpeg_decode_img_info_t img_info = {0};
		img_info.frame = in_frame;
		ret = bk_get_jpeg_data_info(&img_info);
		if (ret != AVDK_ERR_OK)
		{
			LOGE(" %s %d bk_get_jpeg_data_info failed %d\n", __func__, __LINE__, ret);
			goto error;
		}

		// Set the image dimensions from parsed JPEG info to input and output frame buffers
		// Note: If rotation is applied, the output width and height will be reconfigured internally after rotation
		in_frame->width = img_info.width;
		in_frame->height = img_info.height;

		// Calculate allocation size based on format
		if (controller->config.out_format == JPEG_DECODE_SW_OUT_FORMAT_GRAY)
		{
			alloc_size = img_info.width * img_info.height;
		}
		else if (controller->config.out_format == JPEG_DECODE_SW_OUT_FORMAT_RGB888)
		{
			alloc_size = img_info.width * img_info.height * 3;
		}
		else
		{
			alloc_size = img_info.width * img_info.height * 2;
		}

		out_frame = sw_jpeg_decode_out_malloc(controller, alloc_size);
		if (out_frame == NULL)
		{
			LOGD(" %s %d no idle buffer found, skip this frame\n", __func__, __LINE__);
			ret = BK_ERR_NO_MEM;
			goto error;
		}

		out_frame->width = img_info.width;
		out_frame->height = img_info.height;
		out_frame->sequence = in_frame->sequence;
		controller->cp2_busy = 1;
		controller->sw_dec_info[1].in_frame = in_frame;
		controller->sw_dec_info[1].out_frame = out_frame;
		controller->sw_dec_info[1].complete = cp2_decode_complete;
		ret = software_decode_task_send_msg_cp2(SOFTWARE_DECODE_START, (uint32_t)&controller->sw_dec_info[1]);
		if (ret != BK_OK)
		{
			LOGE(" %s %d rtos_push_to_queue failed %d\n", __func__, __LINE__, ret);
			goto error;
		}
	}
	else if (controller->cp1_busy == 0)
	{
		uint32_t frame_ptr = 0;
		uint32_t alloc_size = 0;
		is_cp2 = 0;

		ret = rtos_pop_from_queue(&controller->input_queue, &frame_ptr, BEKEN_NO_WAIT);
		if (ret != BK_OK) {
			goto error;
		}

		in_frame = (frame_buffer_t *)frame_ptr;
		bk_jpeg_decode_img_info_t img_info = {0};
		img_info.frame = in_frame;
		ret = bk_get_jpeg_data_info(&img_info);
		if (ret != AVDK_ERR_OK)
		{
			LOGE(" %s %d bk_get_jpeg_data_info failed %d\n", __func__, __LINE__, ret);
			goto error;
		}

		// Set the image dimensions from parsed JPEG info to input and output frame buffers
		// Note: If rotation is applied, the output width and height will be reconfigured internally after rotation
		in_frame->width = img_info.width;
		in_frame->height = img_info.height;

		// Calculate allocation size based on format
		if (controller->config.out_format == JPEG_DECODE_SW_OUT_FORMAT_GRAY)
		{
			alloc_size = img_info.width * img_info.height;
		}
		else if (controller->config.out_format == JPEG_DECODE_SW_OUT_FORMAT_RGB888)
		{
			alloc_size = img_info.width * img_info.height * 3;
		}
		else
		{
			alloc_size = img_info.width * img_info.height * 2;
		}

		out_frame = sw_jpeg_decode_out_malloc(controller, alloc_size);
		if (out_frame == NULL)
		{
			LOGE(" %s %d out_malloc failed\n", __func__, __LINE__);
			ret = BK_ERR_NO_MEM;
			goto error;
		}

		out_frame->width = img_info.width;
		out_frame->height = img_info.height;
		out_frame->sequence = in_frame->sequence;
		controller->sw_dec_info[0].in_frame = in_frame;
		controller->sw_dec_info[0].out_frame = out_frame;
		controller->sw_dec_info[0].complete = cp1_decode_complete;
		controller->cp1_busy = 1;

		ret = software_decode_task_send_msg_cp1(SOFTWARE_DECODE_START, (uint32_t)&controller->sw_dec_info[0]);
		if (ret != BK_OK)
		{
			LOGE(" %s %d rtos_push_to_queue failed %d\n", __func__, __LINE__, ret);
			goto error;
		}
	}

	return ret;

error:
	// Release input frame if exists
	if (in_frame)
	{
		sw_jpeg_decode_in_complete(in_frame, controller);
	}

	// Release output frame if exists
	if (out_frame)
	{
		sw_jpeg_decode_out_complete(out_frame->fmt, BK_FAIL, out_frame, controller);
	}

	// Clean up decoder state if busy flag was set
	if (is_cp2 && controller->cp2_busy)
	{
		controller->sw_dec_info[1].in_frame = NULL;
		controller->sw_dec_info[1].out_frame = NULL;
		controller->sw_dec_info[1].complete = NULL;
		controller->cp2_busy = 0;
	}
	else if (!is_cp2 && controller->cp1_busy)
	{
		controller->sw_dec_info[0].in_frame = NULL;
		controller->sw_dec_info[0].out_frame = NULL;
		controller->sw_dec_info[0].complete = NULL;
		controller->cp1_busy = 0;
	}

	return ret;
}

static bk_err_t software_decode_dual_core_output_all_frame(private_jpeg_decode_sw_multi_core_ctlr_t *controller)
{
	bk_err_t ret = BK_OK;
	while (!rtos_is_queue_empty(&controller->output_queue))
	{
		uint32_t frame_ptr = 0;
		ret = rtos_pop_from_queue(&controller->output_queue, &frame_ptr, BEKEN_NO_WAIT);
		if (ret == BK_OK) {
			frame_buffer_t *out_frame = (frame_buffer_t *)frame_ptr;
			sw_jpeg_decode_out_complete(out_frame->fmt,
								BK_OK, out_frame, controller);
		}
	}
	return ret;
}

static bk_err_t software_decode_dual_core_complete(private_jpeg_decode_sw_multi_core_ctlr_t *controller, uint32_t core_id)
{
	bk_err_t ret = BK_OK;
	if (core_id == JPEG_DECODE_CORE_ID_1)
	{
		if (controller->cp2_busy)
		{
			if (controller->sw_dec_info[0].out_frame->sequence > controller->sw_dec_info[1].out_frame->sequence)
			{
				ret = rtos_push_to_queue(&controller->output_queue, &controller->sw_dec_info[0].out_frame, BEKEN_NO_WAIT);
				if (ret != BK_OK) {
					LOGE(" %s %d rtos_push_to_queue failed %d\n", __func__, __LINE__, ret);
					sw_jpeg_decode_out_complete(controller->sw_dec_info[0].out_frame->fmt,
											BK_FAIL, controller->sw_dec_info[0].out_frame, controller);
				}
			}
			else
			{
				sw_jpeg_decode_out_complete(controller->sw_dec_info[0].out_frame->fmt,
						BK_OK, controller->sw_dec_info[0].out_frame, controller);
				software_decode_dual_core_output_all_frame(controller);
			}
		}
		else
		{
			sw_jpeg_decode_out_complete(controller->sw_dec_info[0].out_frame->fmt,
									BK_OK, controller->sw_dec_info[0].out_frame, controller);
			software_decode_dual_core_output_all_frame(controller);
		}
		controller->sw_dec_info[0].in_frame = NULL;
		controller->sw_dec_info[0].out_frame = NULL;
		controller->sw_dec_info[0].complete = NULL;
		controller->cp1_busy = 0;
	}
	else if (core_id == JPEG_DECODE_CORE_ID_2)
	{
		if (controller->cp1_busy)
		{
			if (controller->sw_dec_info[1].out_frame->sequence > controller->sw_dec_info[0].out_frame->sequence)
			{
				ret = rtos_push_to_queue(&controller->output_queue, &controller->sw_dec_info[1].out_frame, BEKEN_NO_WAIT);
				if (ret != BK_OK) {
					LOGE(" %s %d rtos_push_to_queue failed %d\n", __func__, __LINE__, ret);
					sw_jpeg_decode_out_complete(controller->sw_dec_info[1].out_frame->fmt,
										BK_FAIL, controller->sw_dec_info[1].out_frame, controller);
				}
			}
			else
			{
				sw_jpeg_decode_out_complete(controller->sw_dec_info[1].out_frame->fmt,
						BK_OK, controller->sw_dec_info[1].out_frame, controller);
				software_decode_dual_core_output_all_frame(controller);
			}
		}
		else
		{
			sw_jpeg_decode_out_complete(controller->sw_dec_info[1].out_frame->fmt,
									BK_OK, controller->sw_dec_info[1].out_frame, controller);
			software_decode_dual_core_output_all_frame(controller);
		}
		controller->sw_dec_info[1].in_frame = NULL;
		controller->sw_dec_info[1].out_frame = NULL;
		controller->sw_dec_info[1].complete = NULL;
		controller->cp2_busy = 0;
	}
	return ret;
}

static void software_decode_dual_core_main(beken_thread_arg_t data)
{
    private_jpeg_decode_sw_multi_core_ctlr_t *controller = (private_jpeg_decode_sw_multi_core_ctlr_t *)data;
    rtos_set_semaphore(&controller->sem);
    software_decode_msg_t msg = {0};
    bk_err_t ret = BK_OK;

    while (controller->task_running) {
        ret = rtos_pop_from_queue(&controller->message_queue, &msg, BEKEN_WAIT_FOREVER);
        if (ret != BK_OK) {
            continue;
        }

        switch (msg.event)
        {
        case SOFTWARE_DECODE_EVENT_DECODE_START:
            software_decode_dual_core_decode(controller);
            break;
        case SOFTWARE_DECODE_EVENT_DECODE_COMPLETE:
            software_decode_dual_core_complete(controller, msg.param);
            break;
        case SOFTWARE_DECODE_EVENT_EXIT:
        {
            controller->task_running = 0;

            uint32_t frame_ptr = 0;
            while (!rtos_is_queue_empty(&controller->output_queue))
            {
                ret = rtos_pop_from_queue(&controller->output_queue, &frame_ptr, BEKEN_NO_WAIT);
                if (ret == BK_OK) {
                    frame_buffer_t *out_frame = (frame_buffer_t *)frame_ptr;
                    sw_jpeg_decode_out_complete(out_frame->fmt,
                                        BK_FAIL, out_frame, controller);
                }
            }
            while (!rtos_is_queue_empty(&controller->input_queue))
            {
                ret = rtos_pop_from_queue(&controller->input_queue, &frame_ptr, BEKEN_NO_WAIT);
                if (ret == BK_OK) {
                    frame_buffer_t *in_frame = (frame_buffer_t *)frame_ptr;
                    sw_jpeg_decode_in_complete(in_frame, controller);
                }
            }
            while (!rtos_is_queue_empty(&controller->message_queue))
            {
                software_decode_msg_t msg_temp = {0};
                ret = rtos_pop_from_queue(&controller->message_queue, &msg_temp, BEKEN_NO_WAIT);
                if (ret == BK_OK) {
                    if (msg_temp.event == SOFTWARE_DECODE_EVENT_DECODE_START)
                    {
                        continue;
                    }
                    else if (msg_temp.event == SOFTWARE_DECODE_EVENT_DECODE_COMPLETE)
                    {
                        if (controller->cp2_busy)
                        {
                            sw_jpeg_decode_out_complete(controller->sw_dec_info[1].out_frame->fmt,
                                                BK_FAIL, controller->sw_dec_info[1].out_frame, controller);
                            controller->sw_dec_info[1].in_frame = NULL;
                            controller->sw_dec_info[1].out_frame = NULL;
                            controller->sw_dec_info[1].complete = NULL;
                            controller->cp2_busy = 0;
                        }
                        if (controller->cp1_busy)
                        {
                            sw_jpeg_decode_out_complete(controller->sw_dec_info[0].out_frame->fmt,
                                                BK_FAIL, controller->sw_dec_info[0].out_frame, controller);
                            controller->sw_dec_info[0].in_frame = NULL;
                            controller->sw_dec_info[0].out_frame = NULL;
                            controller->sw_dec_info[0].complete = NULL;
                            controller->cp1_busy = 0;
                        }
                    }
                }
            }
            LOGE("%s %d exit\n", __func__, __LINE__);
        }
            break;
        default:
            break;
        }
    }
    
    rtos_set_semaphore(&controller->sem);
    rtos_delete_thread(NULL);
}

static void software_decode_task_dual_core_destroy(void *context)
{
	private_jpeg_decode_sw_multi_core_ctlr_t *controller = (private_jpeg_decode_sw_multi_core_ctlr_t *)context;
	controller->module_status[0].status = JPEG_DECODE_DISABLED;
	controller->module_status[1].status = JPEG_DECODE_DISABLED;

	if (controller->input_queue)
	{
		rtos_deinit_queue(&controller->input_queue);
		controller->input_queue = NULL;
	}
	if (controller->output_queue)
	{
		rtos_deinit_queue(&controller->output_queue);
		controller->output_queue = NULL;
	}
	if (controller->message_queue)
	{
		rtos_deinit_queue(&controller->message_queue);
		controller->message_queue = NULL;
	}
	if (controller->sem)
	{
		rtos_deinit_semaphore(&controller->sem);
		controller->sem = NULL;
	}
	if (controller->thread)
	{
		controller->thread = NULL;
	}
}

bk_err_t software_decode_task_dual_core_send_frame(private_jpeg_decode_sw_multi_core_ctlr_t *controller, frame_buffer_t *in_frame)
{
	avdk_err_t ret = AVDK_ERR_SHUTDOWN;
	if (controller && controller->task_running)
	{
		ret = rtos_push_to_queue(&controller->input_queue, &in_frame, BEKEN_NO_WAIT);
		if(ret != AVDK_ERR_OK)
		{
			return ret;
		}
		ret = software_decode_task_dual_core_send_msg(SOFTWARE_DECODE_EVENT_DECODE_START, 0, controller);
		return ret;
	}
	return ret;
}


bk_err_t software_decode_task_dual_core_open(private_jpeg_decode_sw_multi_core_ctlr_t *controller)
{
	bk_err_t ret = BK_OK;
	if (controller->module_status[0].status == JPEG_DECODE_ENABLED || controller->module_status[1].status == JPEG_DECODE_ENABLED)
	{
		ret = BK_OK;
		LOGE("%s %d already open\n", __func__, __LINE__);
		return ret;
	}

	// 双核模式，初始化两个核心
	ret = software_decode_task_open_cp1(controller);
	if (ret != BK_OK) {
		LOGE("%s %d Failed to init cp1\n", __func__, __LINE__);
		goto exit;
	}
	ret = software_decode_task_open_cp2(controller);
	if (ret != BK_OK) {
		LOGE("%s %d Failed to init cp2\n", __func__, __LINE__);
		goto exit;
	}

	// 初始化状态
	controller->cp1_busy = 0;
	controller->cp2_busy = 0;
	
	// 初始化消息队列
	ret = rtos_init_queue(&controller->message_queue, "multi_core_queue", sizeof(software_decode_msg_t), 20);
	if (ret != BK_OK) {
		LOGE("%s %d Failed to init message queue\n", __func__, __LINE__);
		goto exit;
	}
	ret = rtos_init_queue(&controller->input_queue, "input_queue", sizeof(uint32_t), 10);
	if (ret != BK_OK) {
		LOGE("%s %d Failed to init input queue\n", __func__, __LINE__);
		goto exit;
	}
	ret = rtos_init_queue(&controller->output_queue, "output_queue", sizeof(uint32_t), 4);
	if (ret != BK_OK) {
		LOGE("%s %d Failed to init output queue\n", __func__, __LINE__);
		goto exit;
	}
	
	// 初始化解码任务线程
	controller->task_running = 1;
	rtos_init_semaphore(&controller->sem, 1);
	ret = rtos_create_thread(&controller->thread,
							BEKEN_DEFAULT_WORKER_PRIORITY,
							"decode_task",
							(beken_thread_function_t)software_decode_dual_core_main,
							CONFIG_SW_JPEG_DECODE_TASK_STACK_SIZE,
							controller);
	if (ret != BK_OK) {
		LOGE("Failed to create decode task thread\n");
		goto exit;
	}
	rtos_get_semaphore(&controller->sem, BEKEN_WAIT_FOREVER);
	controller->module_status[0].status = JPEG_DECODE_ENABLED;
	controller->module_status[1].status = JPEG_DECODE_ENABLED;

	LOGE("%s %d open success\n", __func__, __LINE__);
	return ret;

exit:
	software_decode_task_close_cp1();
	software_decode_task_close_cp2();
	software_decode_task_dual_core_destroy(controller);
	return ret;
}

bk_err_t software_decode_task_dual_core_close(private_jpeg_decode_sw_multi_core_ctlr_t *controller)
{
	LOGD("%s  %d\n", __func__, __LINE__);
	bk_err_t ret = BK_OK;

	ret = software_decode_task_close_cp1();
	if (ret != BK_OK)
	{
		LOGE("%s %d software_decode_task_close_cp1 failed %d\n", __func__, __LINE__, ret);
	}
	ret = software_decode_task_close_cp2();
	if (ret != BK_OK)
	{
		LOGE("%s %d software_decode_task_close_cp2 failed %d\n", __func__, __LINE__, ret);
	}
	software_decode_task_dual_core_send_msg(SOFTWARE_DECODE_EVENT_EXIT, 0, controller);
	rtos_get_semaphore(&controller->sem, BEKEN_WAIT_FOREVER);
	software_decode_task_dual_core_destroy(controller);

	LOGD("%s complete, %d\n", __func__, __LINE__);

	return BK_OK;
}
