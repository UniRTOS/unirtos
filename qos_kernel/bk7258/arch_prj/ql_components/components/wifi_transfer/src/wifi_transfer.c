// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//	   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os/mem.h>
#include <components/log.h>
#include "bk_wifi.h"
#include <modules/wifi.h>
#include "modules/wifi_types.h"
#include <driver/dma.h>
#include "bk_general_dma.h"
#include "wifi_transfer.h"
#include "media_utils.h"

#define TAG "wifi_trs"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

typedef struct
{
    uint8_t enable;
    image_format_t img_format;
    transfer_data_t *transfer_app_data;
    video_setup_t transfer_app_config;
    beken_semaphore_t sem;
    beken_thread_t transfer_thread;
    const media_transfer_cb_t *cb;
} wifi_transfer_cfg_t;

static wifi_transfer_cfg_t *s_wifi_transfer_cfg = NULL;
extern media_debug_t *media_debug;

void wifi_transfer_data_check_caller(const char *func_name, int line,uint8_t *data, uint32_t length)
{
	if (length >= 12)
	{
		LOGD("%s %d data length %d\n",func_name,line,length);
		LOGD("%02x %02x %02x %02x %02x %02x \r\n", data[0], data[1], data[2], data[3], data[4], data[5]);
		LOGD("%02x %02x %02x %02x %02x %02x\r\n", data[6], data[7], data[8], data[9], data[10], data[11]);
		LOGD("\r\n");
	}
}

static int send_frame_buffer_packet(wifi_transfer_cfg_t *cfg, uint8_t *data, uint32_t size)
{
	int ret = BK_FAIL;

	//wifi_transfer_data_check(data,size);

	if (cfg->cb == NULL)
		return ret;

	if (cfg->cb->prepare)
	{
		cfg->cb->prepare(data, size);
	}

	//wifi_transfer_data_check(data,size);

	ret = cfg->cb->send(data, size);

	return ret == size ? BK_OK : BK_FAIL;
}

static void wifi_transfer_send_frame(wifi_transfer_cfg_t *cfg, frame_buffer_t *buffer)
{
	int ret = BK_OK;

	uint32_t i;
	uint32_t count = buffer->length / cfg->transfer_app_config.pkt_size;
	uint32_t tail = buffer->length % cfg->transfer_app_config.pkt_size;
	transfer_data_t *transfer_app_data = cfg->transfer_app_data;
	video_setup_t *transfer_app_config = &cfg->transfer_app_config;

	uint8_t *src_address = buffer->frame;

	LOGV("%s %d transfer_app_config->pkt_size %d\n", __func__,__LINE__,cfg->transfer_app_config.pkt_size);

#if CONFIG_MEDIA_DROP_STRATEGY_ENABLE
	// check whether this frame should be dropped for some reasones such as no enough buffer
	if (cfg->cb->drop_check && cfg->cb->drop_check(buffer,(count + (tail ? 1 : 0)),cfg->transfer_app_config.pkt_header_size))
	{
		return;
	}
#endif

	transfer_app_data->id = (buffer->sequence & 0xFF);
	transfer_app_data->eof = 0;
	transfer_app_data->cnt = 0;//package seq num
	transfer_app_data->size = count + (tail ? 1 : 0);//one frame package counts

	LOGV("seq: %u, length: %u, size: %u count %d\n", buffer->sequence, buffer->length, buffer->size,transfer_app_data->size);

	for (i = 0; i < count && cfg->enable; i++)
	{
		transfer_app_data->cnt = i + 1;

		if ((tail == 0) && (i == count - 1))
		{
			transfer_app_data->eof = 1;
		}

		os_memcpy_word((uint32_t *)transfer_app_data->data, (uint32_t *)(src_address + (transfer_app_config->pkt_size * i)),
						transfer_app_config->pkt_size);

		LOGV("seq: %d [%d %d %d]\n", buffer->sequence,transfer_app_data->id,transfer_app_data->eof,transfer_app_data->cnt);

		ret = send_frame_buffer_packet(cfg, (uint8_t *)transfer_app_data, transfer_app_config->pkt_size + transfer_app_config->pkt_header_size);
		if (ret != BK_OK)
		{
			LOGV("send failed\n");
		}
	}

	if (tail)
	{
		transfer_app_data->eof = 1;
		transfer_app_data->cnt = count + 1;

		os_memcpy_word((uint32_t *)transfer_app_data->data, (uint32_t *)(src_address + (transfer_app_config->pkt_size * i)),
						(tail % 4) ? ((tail / 4 + 1) * 4) : tail);

		LOGV("seq: %d [%d %d %d]\n", buffer->sequence,transfer_app_data->id,transfer_app_data->eof,transfer_app_data->cnt);	

		ret = send_frame_buffer_packet(cfg, (uint8_t *)transfer_app_data, tail + transfer_app_config->pkt_header_size);

		if (ret != BK_OK)
		{
			LOGV("send failed\n");
		}
	}

	LOGV("length: %u, tail: %u, count: %u\n", buffer->length, tail, count);
}

static bk_err_t wifi_transfer_buffer_init(wifi_transfer_cfg_t *cfg)
{
	if (cfg == NULL)
	{
		return BK_FAIL;
	}

	if (cfg->cb->get_tx_buf)
	{
		cfg->transfer_app_data = cfg->cb->get_tx_buf();
		cfg->transfer_app_config.pkt_size = cfg->cb->get_tx_size() - cfg->transfer_app_config.pkt_header_size;

		if (cfg->transfer_app_data == NULL
			|| cfg->cb->get_tx_size() < cfg->transfer_app_config.pkt_header_size)
		{
			LOGE("%s transfer_data: %p, size: %d\n", __func__, cfg->transfer_app_data, cfg->transfer_app_config.pkt_size);
			return BK_FAIL;
		}
	}
	else
	{
		if (cfg->transfer_app_data == NULL)
		{
			cfg->transfer_app_data = os_malloc(1472);
			if (cfg->transfer_app_data == NULL)
			{
				return BK_FAIL;
			}
		}

		cfg->transfer_app_config.pkt_size = 1472 - cfg->transfer_app_config.pkt_header_size;
	}

	LOGD("%s transfer_data: %p, size: %d\n", __func__, cfg->transfer_app_data, cfg->transfer_app_config.pkt_size);

	return BK_OK;
}

static void wifi_transfer_buffer_deinit(wifi_transfer_cfg_t *cfg)
{
    if (cfg->cb->get_tx_buf == NULL)
    {
        if (cfg->transfer_app_data)
        {
            os_free(cfg->transfer_app_data);
            cfg->transfer_app_data = NULL;
        }
    }

    if (cfg->sem)
    {
        rtos_deinit_semaphore(&cfg->sem);
        cfg->sem = NULL;
    }

    os_free(cfg);
}

static void wifi_transfer_task_entry(beken_thread_arg_t data)
{
    wifi_transfer_cfg_t *cfg = (wifi_transfer_cfg_t *)data;
    frame_buffer_t *frame = NULL;
    cfg->enable = true;
    rtos_set_semaphore(&cfg->sem);
    uint32_t before = 0, after = 0;
    uint8_t log_enable = 0;

    while (cfg->enable)
    {
        frame = cfg->cb->read(cfg->img_format, 50);
        if (frame == NULL)
        {
            log_enable ++;
            if (log_enable > 100)
            {
                LOGD("%s, read frame null format:%x\n", __func__, cfg->img_format);
                log_enable = 0;
            }
            continue;
        }

        if (frame->sequence < 5)
        {
            LOGD("%s, frame sequence %d\n", __func__, frame->sequence);
        }

        log_enable = 0;
        before = get_current_timestamp();
        media_debug->begin_trs = true;
        media_debug->end_trs = false;

        wifi_transfer_send_frame(cfg, frame);

        media_debug->end_trs = true;
        media_debug->begin_trs = false;

        after = get_current_timestamp();

        media_debug->meantimes += (after - before);
        media_debug->fps_wifi++;
        media_debug->wifi_kbps += frame->length;

        cfg->cb->free(cfg->img_format, frame);
        frame = NULL;
    }

    cfg->transfer_thread = NULL;
    rtos_set_semaphore(&cfg->sem);
    rtos_delete_thread(NULL);
}

bk_err_t bk_wifi_transfer_frame_open(const media_transfer_cb_t *cb, uint16_t img_format)
{
    if (cb == NULL)
    {
        return BK_FAIL;
    }

    if (s_wifi_transfer_cfg)
    {
        LOGW("%s, already opened, img_format: %d", __func__, s_wifi_transfer_cfg->img_format);
        return BK_OK;
    }

    s_wifi_transfer_cfg = os_malloc(sizeof(wifi_transfer_cfg_t));
    if (s_wifi_transfer_cfg == NULL)
    {
        LOGE("% malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    memset(s_wifi_transfer_cfg, 0, sizeof(wifi_transfer_cfg_t));

    s_wifi_transfer_cfg->img_format = img_format;
    s_wifi_transfer_cfg->cb = cb;
    s_wifi_transfer_cfg->transfer_app_config.pkt_header_size = sizeof(transfer_data_t);

    if (rtos_init_semaphore(&s_wifi_transfer_cfg->sem, 1) != BK_OK)
    {
        LOGE("%s rtos_init_semaphore failed\n", __func__);
        goto error;
    }

    if (wifi_transfer_buffer_init(s_wifi_transfer_cfg) != BK_OK)
    {
        LOGE("%s wifi_transfer_buffer_init failed\n", __func__);
        goto error;
    }

// need create task to read frame
bk_err_t ret = rtos_create_thread(&s_wifi_transfer_cfg->transfer_thread,
                                BEKEN_DEFAULT_WORKER_PRIORITY,
                                "trs_task",
                                (beken_thread_function_t)wifi_transfer_task_entry,
                                2560,
                                (beken_thread_arg_t)s_wifi_transfer_cfg);

    if (BK_OK != ret)
    {
        LOGE("%s transfer_app_task init failed\n", __func__);
        ret = BK_ERR_NO_MEM;
        goto error;
    }

    rtos_get_semaphore(&s_wifi_transfer_cfg->sem, BEKEN_NEVER_TIMEOUT);

    bk_wifi_set_wifi_media_mode(true);

    bk_wifi_set_video_quality(WIFI_VIDEO_QUALITY_SD);

    return BK_OK;

error:
    if (s_wifi_transfer_cfg->transfer_thread)
    {
        wifi_transfer_buffer_deinit(s_wifi_transfer_cfg);
    }
    s_wifi_transfer_cfg = NULL;
    return BK_FAIL;
}

bk_err_t bk_wifi_transfer_frame_close(void)
{
	wifi_transfer_cfg_t *cfg = s_wifi_transfer_cfg;

	if (cfg == NULL)
	{
		return BK_OK;
	}

	if (!cfg->enable)
	{
		LOGE("%s, have been close!\r\n", __func__);
		return BK_FAIL;
	}

	cfg->enable = 0;
	rtos_get_semaphore(&cfg->sem, BEKEN_NEVER_TIMEOUT);

	wifi_transfer_buffer_deinit(cfg);

	bk_wifi_set_wifi_media_mode(false);

	bk_wifi_set_video_quality(WIFI_VIDEO_QUALITY_HD);

	s_wifi_transfer_cfg = NULL;

	LOGD("%s, close success!\r\n", __func__);

	return BK_OK;
}
