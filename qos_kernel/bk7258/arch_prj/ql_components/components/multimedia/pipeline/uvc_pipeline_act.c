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
#include <driver/lcd.h>

#include "media_evt.h"


#include "yuv_encode.h"
#include "uvc_pipeline_act.h"

#include "mux_pipeline.h"

#define TAG "uvc_pipe"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

extern uint8_t *media_bt_share_buffer;

mux_sram_decode_buffer_t *mux_sram_decode_buffer = NULL;
mux_sram_rotate_buffer_t *mux_sram_rotate_buffer = NULL;
mux_sram_scale_buffer_t *mux_sram_scale_buffer = NULL;
static uint8_t pipeline_init = false;
static uint8_t h264_pipeline_init = false;
static uint8_t lcd_pipeline_init = false;

bk_err_t init_encoder_buffer(void)
{
	if (mux_sram_decode_buffer != NULL)
	{
		LOGE("%s %d mux_sram_decode_buffer is already init\n", __func__, __LINE__);
		return BK_OK;
	}
	uint8_t *buf = media_bt_share_buffer;
	if(buf == NULL)
	{
		buf = os_malloc(sizeof(mux_sram_decode_buffer_t));
		if(buf == NULL)
		{
			LOGE("%s %d os_malloc fail\n", __func__, __LINE__);
			return BK_FAIL;
		}
		mux_sram_decode_buffer = (mux_sram_decode_buffer_t *)buf;
	}
	else
	{
		mux_sram_decode_buffer = (mux_sram_decode_buffer_t *)buf;
	}
	LOGE("%s %d mux_sram_decode_buffer:%p\n", __func__, __LINE__, mux_sram_decode_buffer);
	return BK_OK;
}

bk_err_t init_rotate_buffer(void)
{
	if (mux_sram_rotate_buffer != NULL)
	{
		LOGE("%s %d mux_sram_rotate_buffer is already init\n", __func__, __LINE__);
		return BK_OK;
	}
	uint8_t *buf = media_bt_share_buffer;
	if(buf == NULL)
	{
		buf = os_malloc(sizeof(mux_sram_rotate_buffer_t));
		if(buf == NULL)
		{
			LOGE("%s %d os_malloc fail\n", __func__, __LINE__);
			return BK_FAIL;
		}
		mux_sram_rotate_buffer = (mux_sram_rotate_buffer_t *)buf;
	}
	else
	{
		mux_sram_rotate_buffer = (mux_sram_rotate_buffer_t *)(buf + sizeof(mux_sram_decode_buffer_t));
	}
	LOGE("%s %d mux_sram_rotate_buffer:%p\n", __func__, __LINE__, mux_sram_rotate_buffer);
	return BK_OK;
}

bk_err_t init_scale_buffer(void)
{
	if (mux_sram_scale_buffer != NULL)
	{
		LOGE("%s %d mux_sram_scale_buffer is already init\n", __func__, __LINE__);
		return BK_OK;
	}
	uint8_t *buf = media_bt_share_buffer;
	if(buf == NULL)
	{
		buf = os_malloc(sizeof(mux_sram_scale_buffer_t));
		if(buf == NULL)
		{
			LOGE("%s %d os_malloc fail\n", __func__, __LINE__);
			return BK_FAIL;
		}
		mux_sram_scale_buffer = (mux_sram_scale_buffer_t *)buf;
	}
	else
	{
		mux_sram_scale_buffer = (mux_sram_scale_buffer_t *)(buf + sizeof(mux_sram_decode_buffer_t) + sizeof(mux_sram_rotate_buffer_t));
	}
	LOGE("%s %d mux_sram_scale_buffer:%p\n", __func__, __LINE__, mux_sram_scale_buffer);
	return BK_OK;
}

static media_rotate_t get_rotate_angle(uint32_t rotate)
{
	media_rotate_t rot_angle = ROTATE_NONE;
	switch (rotate)
    {
        case 90:
            rot_angle = ROTATE_90;
            break;
        case 180:
            rot_angle = ROTATE_180;
            break;
        case 270:
            rot_angle = ROTATE_270;
            break;
        case 0:
        default:
            rot_angle = ROTATE_NONE;
            break;
    }
	return rot_angle;
}

bk_err_t h264_jdec_pipeline_regenerate_idr_frame(void)
{
	int ret = BK_FAIL;
#ifdef CONFIG_H264
	if (check_h264_task_is_open())
	{
		ret = h264_encode_regenerate_idr_frame();
		if (ret != BK_OK)
		{
			LOGE("%s %d h264_encode_regenerate_idr_frame fail\n", __func__, __LINE__);
		}
	}
#endif
	return ret;
}

bk_err_t h264_jdec_pipeline_open(bk_video_pipeline_h264e_config_t *config, const bk_h264e_callback_t *cb,
	 				const jpeg_callback_t *jpeg_cbs, const decode_callback_t *decode_cbs)
{
	int ret = BK_FAIL;

#ifdef CONFIG_H264

	ret = uvc_pipeline_init();
	if (ret != BK_OK)
	{
		LOGW("%s, uvc_pipeline_init fail\n", __func__);
		return ret;
	}

	ret = init_encoder_buffer();
	if (ret != BK_OK)
	{
		LOGW("%s, init_encoder_buffer fail\n", __func__);
		return ret;
	}

	if (config == NULL || cb == NULL)
	{
		LOGW("%s, param error\n", __func__);
		return ret;
	}

	ret = h264_encode_task_open(config, cb);
	if (ret != BK_OK)
	{
		goto error;
	}

	// step 2: init jpeg_decode_task
	if (!check_jpeg_decode_task_is_open())
	{
		ret = jpeg_decode_task_open(config->sw_rotate_angle, jpeg_cbs, decode_cbs);

		if (ret != BK_OK)
		{
			LOGE("%s %d jpeg_decode_task_open fail\n", __func__, __LINE__);
			goto error;
		}
		bk_jdec_buffer_request_register(PIPELINE_MOD_H264, bk_h264_encode_request, bk_h264_reset_request);
		LOGV("%s, jdec_h264_enc_en \n", __func__);
	}
	else
	{
		bk_jdec_buffer_request_register(PIPELINE_MOD_H264, bk_h264_encode_request, bk_h264_reset_request);
		LOGV("%s, jdec_h264_enc_en \n", __func__);
	}
	h264_pipeline_init = true;
	return ret;

error:
	bk_jdec_buffer_request_deregister(PIPELINE_MOD_H264);
	h264_encode_task_close();
	if (check_rotate_task_is_open() == false)
	{
		jpeg_decode_task_close();
	}
	h264_pipeline_init = false;
#endif
	return ret;
}

bk_err_t h264_jdec_pipeline_close(void)
{
#ifdef CONFIG_H264
	LOGV("%s %d\n", __func__, __LINE__);

	if (check_rotate_task_is_open())
	{
		bk_jdec_buffer_request_deregister(PIPELINE_MOD_H264);
		LOGV("%s jdec_h264_enc_en = 0 %d \n", __func__, __LINE__);
		//rtos_delay_milliseconds(200);
		h264_encode_task_close();
	}
	else
	{
		bk_jdec_buffer_request_deregister(PIPELINE_MOD_H264);
		h264_encode_task_close();
		jpeg_decode_task_close();
		LOGV("%s decode task close complete \n", __func__);
	}
	LOGD("%s complete, %d \n", __func__, __LINE__);
	h264_pipeline_init = false;
#endif
	return BK_OK;
}


bk_err_t lcd_jdec_pipeline_open(bk_video_pipeline_decode_config_t *config,
								const jpeg_callback_t *jpeg_cbs,
								const decode_callback_t *decode_cbs)
{
	int ret = BK_OK;

	ret = uvc_pipeline_init();
	if (ret != BK_OK)
	{
		LOGW("%s, uvc_pipeline_init fail\n", __func__);
		return ret;
	}

	ret = init_encoder_buffer();
	if (ret != BK_OK)
	{
		LOGW("%s, init_encoder_buffer fail\n", __func__);
		return ret;
	}
	ret = init_rotate_buffer();
	if (ret != BK_OK)
	{
		LOGW("%s, init_rotate_buffer fail\n", __func__);
		return ret;
	}

#if SUPPORTED_IMAGE_MAX_720P
	ret = init_scale_buffer();
	if (ret != BK_OK)
	{
		LOGW("%s, init_scale_buffer fail\n", __func__);
		return ret;
	}

	lcd_scale_t local_lcd_scale = {PPI_1280X720, PPI_864X480};  // {PPI_864X480, PPI_480X480}, {PPI_1280X720, PPI_864X480}, {PPI_640X480, PPI_480X800};{PPI_480X320, PPI_480X864};
	ret = scale_task_open(&local_lcd_scale, decode_cbs);
	if (ret != BK_OK)
	{
		goto error;
	}
#endif

	rot_open_t rot_open = {0};
	media_rotate_t rot_angle = config->rotate_angle;
	rot_open.mode = config->rotate_mode;
	rot_open.angle = rot_angle;
	ret = rotate_task_open(&rot_open, decode_cbs);

	if (ret != BK_OK)
	{
		LOGE("%s %d rotate_task_open fail\n", __func__, __LINE__);
		goto error;
	}

	if (!check_jpeg_decode_task_is_open())
	{
		ret = jpeg_decode_task_open(rot_angle, jpeg_cbs, decode_cbs);

		if (ret != BK_OK)
		{
			LOGE("%s, jpeg_decode_task_open fail\n", __func__);
			return ret;
		}
#if SUPPORTED_IMAGE_MAX_720P
		bk_jdec_buffer_request_register(PIPELINE_MOD_SCALE, bk_scale_encode_request, bk_scale_reset_request);
#else
		bk_jdec_buffer_request_register(PIPELINE_MOD_ROTATE, bk_rotate_encode_request, bk_rotate_reset_request);
#endif
	}
	else
	{
#if SUPPORTED_IMAGE_MAX_720P
		bk_jdec_buffer_request_register(PIPELINE_MOD_SCALE, bk_scale_encode_request, bk_scale_reset_request);
#else
		bk_jdec_buffer_request_register(PIPELINE_MOD_ROTATE, bk_rotate_encode_request, bk_rotate_reset_request);
#endif
	}
	LOGV("%s %d\n", __func__, __LINE__);
	lcd_pipeline_init = true;
	return ret;

error:
	LOGD("%s fail\n", __func__, __LINE__);
	rotate_task_close();
#ifdef CONFIG_H264
	if (check_h264_task_is_open() == false)
	{
		jpeg_decode_task_close();
	}
#endif
	lcd_pipeline_init = false;
	return BK_FAIL;
}

bk_err_t lcd_jdec_pipeline_close(void)
{
	int ret = BK_OK;
#ifdef CONFIG_H264
	LOGV("%s %d\n", __func__, __LINE__);

	if (check_h264_task_is_open())
	{
#if SUPPORTED_IMAGE_MAX_720P
		LOGV("%s deregister scale, %d \n", __func__, __LINE__);
		rotate_task_close();
		bk_jdec_buffer_request_deregister(PIPELINE_MOD_SCALE);
		scale_task_close();
#else
		LOGV("%s deregister rotate, %d \n", __func__, __LINE__);
		bk_jdec_buffer_request_deregister(PIPELINE_MOD_ROTATE);
		rotate_task_close();
#endif
	}
	else
#endif
	{
#if SUPPORTED_IMAGE_MAX_720P
		LOGV("%s deregister scale, %d \n", __func__, __LINE__);
		rotate_task_close();
		bk_jdec_buffer_request_deregister(PIPELINE_MOD_SCALE);
		scale_task_close();
#else
		LOGV("%s deregister rotate, %d \n", __func__, __LINE__);
		bk_jdec_buffer_request_deregister(PIPELINE_MOD_ROTATE);
		rotate_task_close();
#endif

		ret = jpeg_decode_task_close();
		if (ret != BK_OK)
		{
			LOGE("%s %d decode task close fail\n", __func__, __LINE__);
			return ret;
		}
	}
	lcd_pipeline_init = false;
	LOGV("%s complete, %d \n", __func__, __LINE__);

	return BK_OK;
}

bk_err_t uvc_pipeline_init(void)
{
	bk_err_t ret = BK_OK;

	if (pipeline_init)
	{
		return ret;
	}

	ret = bk_jdec_pipeline_init();
	if (ret != BK_OK)
	{
		LOGW("%s, bk_jdec_pipeline_init fail\n", __func__);
		goto error;
	}

#if SUPPORTED_IMAGE_MAX_720P
	ret = bk_scale_pipeline_init();
	if (ret != BK_OK)
	{
		LOGW("%s, bk_scale_pipeline_init fail\n", __func__);
		goto error;
	}
#endif

	ret = bk_rotate_pipeline_init();
	if (ret != BK_OK)
	{
		LOGW("%s, bk_rotate_pipeline_init fail\n", __func__);
		goto error;
	}

#ifdef CONFIG_H264
	ret = bk_h264_pipeline_init();
	if (ret != BK_OK)
	{
		LOGW("%s, bk_h264_pipeline_init fail\n", __func__);
		goto error;
	}
#endif

	pipeline_init = true;

	return BK_OK;

error:
	bk_jdec_pipeline_deinit();
#if SUPPORTED_IMAGE_MAX_720P
	bk_scale_pipeline_deinit();
#endif
	bk_rotate_pipeline_deinit();
#ifdef CONFIG_H264
	bk_h264_pipeline_deinit();
#endif

	pipeline_init = false;
	return ret;
}

bk_err_t uvc_pipeline_deinit(void)
{
	bk_err_t ret = BK_FAIL;

	if (h264_pipeline_init)
	{
		LOGW("%s, h264 is open , don't deinit uvc pipeline\n", __func__);
		return ret;
	}
	if (lcd_pipeline_init)
	{
		LOGW("%s, lcd is open , don't deinit uvc pipeline\n", __func__);
		return ret;
	}
	ret = bk_jdec_pipeline_deinit();
	if (ret != BK_OK)
	{
		LOGW("%s, bk_jdec_pipeline_deinit fail\n", __func__);
	}
#if SUPPORTED_IMAGE_MAX_7200P
	ret = bk_scale_pipeline_deinit();
	if (ret != BK_OK)
	{
		LOGW("%s, bk_scale_pipeline_deinit fail\n", __func__);
	}
#endif
	ret = bk_rotate_pipeline_deinit();
	if (ret != BK_OK)
	{
		LOGW("%s, bk_rotate_pipeline_deinit fail\n", __func__);
	}
#ifdef CONFIG_H264
	ret = bk_h264_pipeline_deinit();
	if (ret != BK_OK)
	{
		LOGW("%s, bk_h264_pipeline_deinit fail\n", __func__);
	}
#endif

	pipeline_init = false;
	LOGW("%s %d complete\n", __func__, __LINE__);
	return ret;
}

static bk_err_t deinit_encoder_buffer(void)
{
	uint8_t *buf = media_bt_share_buffer;
	if(buf == NULL)
	{
		if (mux_sram_decode_buffer != NULL)
		{
			os_free(mux_sram_decode_buffer);
			mux_sram_decode_buffer = NULL;
		}
	}
	else
	{
		mux_sram_decode_buffer = NULL;
	}
	return BK_OK;
}

static bk_err_t deinit_rotate_buffer(void)
{
	uint8_t *buf = media_bt_share_buffer;
	if(buf == NULL)
	{
		if (mux_sram_rotate_buffer != NULL)
		{
			os_free(mux_sram_rotate_buffer);
			mux_sram_rotate_buffer = NULL;
		}
	}
	else
	{
		mux_sram_rotate_buffer = NULL;
	}
	return BK_OK;
}

static bk_err_t deinit_scale_buffer(void)
{
	uint8_t *buf = media_bt_share_buffer;
	if(buf == NULL)
	{
		if (mux_sram_scale_buffer != NULL)
		{
			os_free(mux_sram_scale_buffer);
			mux_sram_scale_buffer = NULL;
		}
	}
	else{
		mux_sram_scale_buffer = NULL;
	}
	return BK_OK;
}

bk_err_t deinit_all_buffer(void)
{
	bk_err_t ret = BK_OK;
	if (h264_pipeline_init)
	{
		LOGW("%s, h264 is open , don't deinit all buffer\n", __func__);
		return ret;
	}
	if (lcd_pipeline_init)
	{
		LOGW("%s, lcd is open , don't deinit all buffer\n", __func__);
		return ret;
	}
	ret = deinit_encoder_buffer();
	if (ret != BK_OK)
	{
		LOGW("%s, deinit_encoder_buffer fail\n", __func__);
	}
	ret = deinit_rotate_buffer();
	if (ret != BK_OK)
	{
		LOGW("%s, deinit_rotate_buffer fail\n", __func__);
	}
	ret = deinit_scale_buffer();
	if (ret != BK_OK)
	{
		LOGW("%s, deinit_scale_buffer fail\n", __func__);
	}
	LOGW("%s %d\n", __func__, __LINE__);
	return ret;
}

uint8_t *get_mux_sram_decode_buffer(void)
{
	return mux_sram_decode_buffer->decoder;
}

uint8_t *get_mux_sram_rotate_buffer(void)
{
	return mux_sram_rotate_buffer->rotate;
}

uint8_t *get_mux_sram_scale_buffer(void)
{
	return mux_sram_scale_buffer->scale;
}

