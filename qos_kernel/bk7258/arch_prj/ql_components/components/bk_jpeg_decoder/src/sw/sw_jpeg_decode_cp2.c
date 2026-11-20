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
#include "sw_jpeg_decode_cp2.h"

#include "mux_pipeline.h"

#define TAG "sw_dec"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#if (CONFIG_CACHE_ENABLE)
#include "cache.h"
#endif

#if (CONFIG_CACHE_ENABLE)
#define FLUSH_ALL_DCACHE() flush_all_dcache()
#else
#define FLUSH_ALL_DCACHE()
#endif

// #define DECODE_DIAG_DEBUG
#ifdef DECODE_DIAG_DEBUG
#define DECODER_FRAME_START() do { GPIO_UP(GPIO_DVP_D0); } while (0)
#define DECODER_FRAME_END() do { GPIO_DOWN(GPIO_DVP_D0); } while (0)
#else
#define DECODER_FRAME_START()
#define DECODER_FRAME_END()
#endif

typedef struct {
	uint8_t task_state;
	beken_semaphore_t sw_dec_sem;
	beken_queue_t sw_dec_queue;
	beken_thread_t sw_dec_thread;
	uint8_t *sw_dec_buffer;
	media_rotate_t rotate_angle;
	uint8_t *rotate_buffer;
	uint32_t out_format;
	bk_jpeg_decode_byte_order_t byte_order;
    jpeg_dec_handle_t jpeg_decode_handle;
    void *context;
} sw_dec_config_t;

static sw_dec_config_t *sw_dec_config = NULL;

__attribute__((section(".dtcm_cpu2"), aligned(0x10))) uint8_t rotate_buffer_sw_cp2[16*16*2] = {0};
__attribute__((section(".dtcm_cpu2"), aligned(0x10))) jd_workbuf_t jpeg_decode_workbuf_cp2 = {0};
__attribute__((aligned(0x10))) StaticTask_t xSWDecTaskTCB_cp2 = {0};
__attribute__((section(".dtcm_cpu2"), aligned(0x10))) StackType_t uxSWDecTaskStack_cp2[ 512 ] = {0};

bk_err_t software_decode_task_send_msg_cp2(uint32_t type, uint32_t param)
{
	int ret = BK_OK;
	media_msg_t msg;

	if (sw_dec_config && /*sw_dec_config->task_state &&*/ sw_dec_config->sw_dec_queue)
	{
		msg.event = type;
		msg.param = param;

		ret = rtos_push_to_queue(&sw_dec_config->sw_dec_queue, &msg, BEKEN_WAIT_FOREVER);

		if (ret != BK_OK)
		{
			LOGE("%s push failed\n", __func__);
		}
	}
	else
	{
		LOGE("%s, sw_dec_config error\n", __func__);
		ret = BK_FAIL;
	}

	return ret;
}

static void software_decode_task_deinit(void)
{
	LOGD("%s\r\n", __func__);
	if (sw_dec_config)
	{
		if (sw_dec_config->sw_dec_queue)
		{
			while (!rtos_is_queue_empty(&sw_dec_config->sw_dec_queue))
			{
				media_msg_t msg;
				if (rtos_pop_from_queue(&sw_dec_config->sw_dec_queue, &msg, BEKEN_NO_WAIT) == BK_OK)
				{
					LOGD("%s, %d, event:%d\n", __func__, __LINE__, msg.event);
				}
			}

			rtos_deinit_queue(&sw_dec_config->sw_dec_queue);
			sw_dec_config->sw_dec_queue = NULL;
		}
		if(sw_dec_config->sw_dec_sem)
		{
			rtos_deinit_semaphore(&sw_dec_config->sw_dec_sem);
			sw_dec_config->sw_dec_sem = NULL;
		}
		sw_dec_config->rotate_buffer = NULL;
		sw_dec_config->sw_dec_thread = NULL;

		os_free(sw_dec_config);
		sw_dec_config = NULL;
	}

	LOGD("%s complete\r\n", __func__);
}

static bk_err_t software_decode_frame(frame_buffer_t *in_frame, frame_buffer_t *out_frame)
{
	bk_err_t ret = BK_OK;
	sw_jpeg_dec_res_t result;
	if ((in_frame == NULL || in_frame->frame == NULL) || (out_frame == NULL || out_frame->frame == NULL))
	{
		return BK_ERR_PARAM;
	}

	// Reconfigure output frame dimensions based on rotation angle
	// For 90/270 degree rotation, swap width and height
	// For 0/180 degree rotation or no rotation, keep original dimensions
	if (sw_dec_config->rotate_angle == ROTATE_90 || sw_dec_config->rotate_angle == ROTATE_270)
	{
		out_frame->width = in_frame->height;
		out_frame->height = in_frame->width;
	}
	else
	{
		out_frame->width = in_frame->width;
		out_frame->height = in_frame->height;
	}

	out_frame->fmt = sw_dec_config->out_format;

	FLUSH_ALL_DCACHE();
	ret = bk_jpeg_dec_sw_start_by_handle(sw_dec_config->jpeg_decode_handle, JPEGDEC_BY_FRAME, in_frame->frame, out_frame->frame,
				in_frame->length, out_frame->size, &result);
	if (ret != BK_OK)
	{
		LOGE("%s sw decoder error %x\n", __func__, ret);
	}
	return ret;
}

static uint32_t sw_jpeg_decode_conver_out_format(sw_dec_config_t *sw_dec_config, bk_jpeg_decode_sw_out_format_t out_format)
{
	uint32_t jd_format = 0;
	uint32_t pix_format = 0;
    switch (out_format)
    {
    case JPEG_DECODE_SW_OUT_FORMAT_RGB565:
		if (sw_dec_config->byte_order == JPEG_DECODE_LITTLE_ENDIAN)
        {
			pix_format = PIXEL_FMT_RGB565;
		}
		else
		{
			pix_format = PIXEL_FMT_RGB565_LE;
		}
		jd_format = JD_FORMAT_RGB565;
        break;
    case JPEG_DECODE_SW_OUT_FORMAT_YUYV:
    case JPEG_DECODE_SW_OUT_FORMAT_YUYV_ROTATE_90:
    case JPEG_DECODE_SW_OUT_FORMAT_YUYV_ROTATE_180:
    case JPEG_DECODE_SW_OUT_FORMAT_YUYV_ROTATE_270:
		pix_format = PIXEL_FMT_YUYV;
        jd_format = JD_FORMAT_YUYV;
        break;
    case JPEG_DECODE_SW_OUT_FORMAT_VUYY:
		pix_format = PIXEL_FMT_VUYY;
        jd_format = JD_FORMAT_VUYY;
        break;
    case JPEG_DECODE_SW_OUT_FORMAT_VYUY:
		pix_format = PIXEL_FMT_VYUY;
        jd_format = JD_FORMAT_VYUY;
        break;
    case JPEG_DECODE_SW_OUT_FORMAT_RGB888:
		pix_format = PIXEL_FMT_RGB888;
        jd_format = JD_FORMAT_RGB888;
        break;
    default:
		pix_format = PIXEL_FMT_YUYV;
        jd_format = JD_FORMAT_YUYV;
        break;
    }
	sw_dec_config->out_format = pix_format;
	return jd_format;
}

static uint32_t sw_jpeg_decode_get_rotate_angle(bk_jpeg_decode_sw_out_format_t out_format)
{
    switch (out_format)
    {
    case JPEG_DECODE_SW_OUT_FORMAT_YUYV_ROTATE_90:
        return ROTATE_90;
    case JPEG_DECODE_SW_OUT_FORMAT_YUYV_ROTATE_180:
        return ROTATE_180;
    case JPEG_DECODE_SW_OUT_FORMAT_YUYV_ROTATE_270:
        return ROTATE_270;
    default:
        return ROTATE_NONE;
    }
}

static void software_decode_main_cp2(beken_thread_arg_t data)
{
	int ret = BK_OK;
	sw_dec_config->task_state = true;

	ret = bk_jpeg_dec_sw_init_by_handle(&sw_dec_config->jpeg_decode_handle,
						(uint8_t *)&jpeg_decode_workbuf_cp2, sizeof(jd_workbuf_t));

	if (ret != BK_OK) {
		LOGE("%s, bk_jpeg_dec_sw_init failed\r\n", __func__);
	}

	sw_dec_config->out_format = PIXEL_FMT_YUYV;
	rtos_set_semaphore(&sw_dec_config->sw_dec_sem);

	while (1)
	{
		media_msg_t msg;
		ret = rtos_pop_from_queue(&sw_dec_config->sw_dec_queue, &msg, BEKEN_WAIT_FOREVER);
		if (ret == BK_OK)
		{
			switch (msg.event)
			{
				case SOFTWARE_DECODE_START:
				{
					software_decode_info_t *sw_dec_info = (software_decode_info_t *)msg.param;
					if (sw_dec_info != NULL)
					{
						DECODER_FRAME_START();
						ret = software_decode_frame(sw_dec_info->in_frame, sw_dec_info->out_frame);
						if (ret != BK_OK)
						{
							LOGE("%s sw decoder error\n", __func__);
						}
						if (sw_dec_info->complete)
						{
							sw_dec_info->complete(sw_dec_config->out_format, ret, sw_dec_info->out_frame, sw_dec_info->in_frame, sw_dec_config->context);
						}
						else
						{
							LOGE("%s dec complete ret is %d but no callback\n", __func__, ret);
						}
						DECODER_FRAME_END();
					}
				}
				break;
				case SOFTWARE_DECODE_SET_ROTATE:
				{
					bk_jpeg_decode_rotate_info_t *rotate_info = (bk_jpeg_decode_rotate_info_t *)msg.param;
					sw_dec_config->rotate_angle = rotate_info->rotate_angle;
					LOGD("%s %d, rotate_angle %d\n", __func__, __LINE__, rotate_info->rotate_angle);
					if (sw_dec_config->rotate_angle != ROTATE_NONE)
					{
						if (sw_dec_config->rotate_buffer == NULL)
						{
							sw_dec_config->rotate_buffer = rotate_buffer_sw_cp2;
						}
						else
						{
							sw_dec_config->rotate_buffer = rotate_info->rotate_buf;
						}
					}
					jd_set_rotate_by_handle(sw_dec_config->jpeg_decode_handle, sw_dec_config->rotate_angle, sw_dec_config->rotate_buffer);
				}
				break;
				case SOFTWARE_DECODE_SET_OUT_FORMAT:
				{
					uint32_t out_format = (uint32_t)msg.param;
					LOGD("%s %d, out_format %d\n", __func__, __LINE__, out_format);
					sw_dec_config->rotate_angle = sw_jpeg_decode_get_rotate_angle(out_format);
					out_format = sw_jpeg_decode_conver_out_format(sw_dec_config, out_format);
					jd_set_format_by_handle(sw_dec_config->jpeg_decode_handle, out_format);
					sw_dec_config->rotate_buffer = rotate_buffer_sw_cp2;
					jd_set_rotate_by_handle(sw_dec_config->jpeg_decode_handle, sw_dec_config->rotate_angle, sw_dec_config->rotate_buffer);
				}
				break;
				case SOFTWARE_DECODE_SET_BYTE_ORDER:
				{
					uint32_t byte_order = (uint32_t)msg.param;
					LOGD("%s %d, byte_order %d\n", __func__, __LINE__, byte_order);
					jd_set_byte_order_by_handle(sw_dec_config->jpeg_decode_handle, byte_order);
				}
				break;
				case SOFTWARE_DECODE_EXIT:
					while (!rtos_is_queue_empty(&sw_dec_config->sw_dec_queue))
					{
						ret = rtos_pop_from_queue(&sw_dec_config->sw_dec_queue, &msg, BEKEN_NO_WAIT);
						if (ret == BK_OK)
						{
							if (msg.event == SOFTWARE_DECODE_START)
							{
								software_decode_info_t *sw_dec_info = (software_decode_info_t *)msg.param;
								if (sw_dec_info->complete)
								{
									sw_dec_info->complete(sw_dec_config->out_format, ret, sw_dec_info->out_frame, sw_dec_info->in_frame, sw_dec_config->context);
								}
							}
						}
					}
					goto exit;

				default:
					break;
			}
		}
	}

exit:
	LOGD("%s, exit\r\n", __func__);
	bk_jpeg_dec_sw_deinit_by_handle(sw_dec_config->jpeg_decode_handle);
    sw_dec_config->jpeg_decode_handle = NULL;
	sw_dec_config->sw_dec_thread = NULL;
	rtos_set_semaphore(&sw_dec_config->sw_dec_sem);
	rtos_delete_thread(NULL);
}

static void create_thread_on_cp2(beken_thread_arg_t data)
{
	beken_semaphore_t tem_sem = (beken_semaphore_t)data;
	sw_dec_config->sw_dec_thread = xTaskCreateStaticPinnedToCore( (TaskFunction_t)software_decode_main_cp2,
										"sw_dec_task",
										512,
										( void * ) NULL,
										9 - BEKEN_DEFAULT_WORKER_PRIORITY,
										uxSWDecTaskStack_cp2,
										&xSWDecTaskTCB_cp2,
										1);
	if (sw_dec_config->sw_dec_thread == NULL)
	{
		LOGE("%s %d create thread sw_dec_task failed\r\n", __func__, __LINE__);
		rtos_set_semaphore(&sw_dec_config->sw_dec_sem);
	}
	rtos_set_semaphore(&tem_sem);
	rtos_delete_thread(NULL);
}

bk_err_t software_decode_task_open_cp2(void *context)
{
	int ret = BK_OK;
	LOGD("%s\r\n", __func__);

	if (sw_dec_config != NULL && sw_dec_config->task_state)
	{
		LOGE("%s have been opened!\r\n", __func__);
		return ret;
	}

	sw_dec_config = (sw_dec_config_t *)os_malloc(sizeof(sw_dec_config_t));
	if (sw_dec_config == NULL)
	{
		LOGE("%s, malloc sw_dec_config failed\r\n", __func__);
		return BK_FAIL;
	}

	os_memset(sw_dec_config, 0, sizeof(sw_dec_config_t));

	ret = rtos_init_semaphore(&sw_dec_config->sw_dec_sem, 1);
	if (ret != BK_OK)
	{
		LOGE("%s, init sw_dec_config->sw_dec_sem failed\r\n", __func__);
		goto error;
	}

	ret = rtos_init_queue(&sw_dec_config->sw_dec_queue,
							"sw_dec_que",
							sizeof(media_msg_t),
							15);

	if (ret != BK_OK)
	{
		LOGE("%s, init sw_dec_que failed\r\n", __func__);
		goto error;
	}

	beken_semaphore_t sw_dec_sem_temp;
	ret = rtos_init_semaphore(&sw_dec_sem_temp, 1);
	if (ret != BK_OK)
	{
		LOGE("%s, init sw_dec_sem_temp failed\r\n", __func__);
		goto error;
	}

	beken_thread_t sw_dec_thread_temp;
	/* create a thread on core 1 */
	ret = rtos_core1_create_thread(&sw_dec_thread_temp,
							BEKEN_DEFAULT_WORKER_PRIORITY,
							"sw_dec_thread_temp",
							(beken_thread_function_t)create_thread_on_cp2,
							1024,
							sw_dec_sem_temp);

	if (ret != BK_OK)
	{
		LOGE("%s, init sw_dec_thread_temp failed\r\n", __func__);
		rtos_deinit_semaphore(&sw_dec_sem_temp);
		goto error;
	}

	rtos_get_semaphore(&sw_dec_sem_temp, BEKEN_NEVER_TIMEOUT);

	rtos_get_semaphore(&sw_dec_config->sw_dec_sem, BEKEN_NEVER_TIMEOUT);

	rtos_deinit_semaphore(&sw_dec_sem_temp);

	sw_dec_config->context = context;

	if (sw_dec_config->sw_dec_thread == NULL)
	{
		LOGD("%s, %d\n", __func__, __LINE__);
		goto error;
	}
	LOGD("%s complete\r\n", __func__);

	return ret;

error:

	LOGE("%s, open failed\r\n", __func__);

	software_decode_task_deinit();

	return ret;
}

bk_err_t software_decode_task_close_cp2()
{
	LOGD("%s  %d\n", __func__, __LINE__);

	if (sw_dec_config == NULL || !sw_dec_config->task_state)
	{
		return BK_FAIL;
	}

	if (sw_dec_config->sw_dec_thread)
	{
		sw_dec_config->task_state = false;

		software_decode_task_send_msg_cp2(SOFTWARE_DECODE_EXIT, 0);

		rtos_get_semaphore(&sw_dec_config->sw_dec_sem, BEKEN_NEVER_TIMEOUT);
	}

	software_decode_task_deinit();

	LOGD("%s complete, %d\n", __func__, __LINE__);

	return BK_OK;
}
