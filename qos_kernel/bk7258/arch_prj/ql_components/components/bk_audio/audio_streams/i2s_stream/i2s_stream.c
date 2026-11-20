// Copyright 2025-2026 Beken
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include <components/bk_audio/audio_streams/i2s_stream.h>
#include <components/bk_audio/audio_pipeline/audio_types.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_error.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <components/bk_audio/audio_pipeline/audio_port_info_list.h>
#include <driver/i2s.h>
#include <driver/i2s_types.h>
#include <driver/audio_ring_buff.h>


#define TAG  "I2S_STR"

/* GPIO debug configuration */
//#define I2S_STREAM_DEBUG   // Enable GPIO debug

#ifdef I2S_STREAM_DEBUG

#define I2S_DMA_ISR_START()                 do { GPIO_DOWN(32); GPIO_UP(32);} while (0)
#define I2S_DMA_ISR_END()                   do { GPIO_DOWN(32); } while (0)

#define I2S_PROCESS_START()                 do { GPIO_DOWN(33); GPIO_UP(33);} while (0)
#define I2S_PROCESS_END()                   do { GPIO_DOWN(33); } while (0)

#define I2S_READ_START()                    do { GPIO_DOWN(34); GPIO_UP(34);} while (0)
#define I2S_READ_END()                      do { GPIO_DOWN(34); } while (0)

#define I2S_WRITE_START()                   do { GPIO_DOWN(35); GPIO_UP(35);} while (0)
#define I2S_WRITE_END()                     do { GPIO_DOWN(35); } while (0)

#define I2S_OPEN_START()                    do { GPIO_DOWN(36); GPIO_UP(36);} while (0)
#define I2S_OPEN_END()                      do { GPIO_DOWN(36); } while (0)

#define I2S_CLOSE_START()                   do { GPIO_DOWN(37); GPIO_UP(37);} while (0)
#define I2S_CLOSE_END()                     do { GPIO_DOWN(37); } while (0)

#else

#define I2S_DMA_ISR_START()
#define I2S_DMA_ISR_END()

#define I2S_PROCESS_START()
#define I2S_PROCESS_END()

#define I2S_READ_START()
#define I2S_READ_END()

#define I2S_WRITE_START()
#define I2S_WRITE_END()

#define I2S_OPEN_START()
#define I2S_OPEN_END()

#define I2S_CLOSE_START()
#define I2S_CLOSE_END()

#endif

/* read i2s data count depends on debug utils, so must config CONFIG_ADK_UTILS=y when count read i2s data. */
#if CONFIG_ADK_UTILS

#define I2S_READ_DATA_COUNT

#endif  //CONFIG_ADK_UTILS

#ifdef I2S_READ_DATA_COUNT

#include <components/bk_audio/audio_utils/count_util.h>
static count_util_t i2s_read_count_util = {0};
#define I2S_READ_DATA_COUNT_INTERVAL     (1000 * 4)
#define I2S_READ_DATA_COUNT_TAG          "I2S_READ"

#define I2S_READ_DATA_COUNT_OPEN()               count_util_create(&i2s_read_count_util, I2S_READ_DATA_COUNT_INTERVAL, I2S_READ_DATA_COUNT_TAG)
#define I2S_READ_DATA_COUNT_CLOSE()              count_util_destroy(&i2s_read_count_util)
#define I2S_READ_DATA_COUNT_ADD_SIZE(size)       count_util_add_size(&i2s_read_count_util, size)

#else

#define I2S_READ_DATA_COUNT_OPEN()
#define I2S_READ_DATA_COUNT_CLOSE()
#define I2S_READ_DATA_COUNT_ADD_SIZE(size)

#endif  //I2S_READ_DATA_COUNT

/* write i2s data count depends on debug utils, so must config CONFIG_ADK_UTILS=y when count write i2s data. */
#if CONFIG_ADK_UTILS

#define I2S_WRITE_DATA_COUNT

#endif  //CONFIG_ADK_UTILS

#ifdef I2S_WRITE_DATA_COUNT

#include <components/bk_audio/audio_utils/count_util.h>
static count_util_t i2s_write_count_util = {0};
#define I2S_WRITE_DATA_COUNT_INTERVAL     (1000 * 4)
#define I2S_WRITE_DATA_COUNT_TAG          "I2S_WRITE"

#define I2S_WRITE_DATA_COUNT_OPEN()               count_util_create(&i2s_write_count_util, I2S_WRITE_DATA_COUNT_INTERVAL, I2S_WRITE_DATA_COUNT_TAG)
#define I2S_WRITE_DATA_COUNT_CLOSE()              count_util_destroy(&i2s_write_count_util)
#define I2S_WRITE_DATA_COUNT_ADD_SIZE(size)       count_util_add_size(&i2s_write_count_util, size)

#else

#define I2S_WRITE_DATA_COUNT_OPEN()
#define I2S_WRITE_DATA_COUNT_CLOSE()
#define I2S_WRITE_DATA_COUNT_ADD_SIZE(size)

#endif  //I2S_WRITE_DATA_COUNT

typedef struct i2s_stream
{
    i2s_gpio_group_id_t     gpio_group;       /**< I2S gpio group id */
    i2s_config_t            i2s_cfg;          /**< I2S configuration */
    i2s_channel_id_t        channel_id;       /**< I2S channel id */
    audio_stream_type_t     type;             /**< Type of stream */
    uint32_t                buff_size;        /**< Ring buffer size */
    RingBufferContext       *rb;              /**< Ring buffer context */
    int                     out_block_size;   /**< Size of output block */
    int                     out_block_num;    /**< Number of output block */
    bool                    is_open;          /**< i2s enable, true: enable, false: disable */
    beken_semaphore_t       can_process;      /**< can process */
    bool                    need_channel_expand;  /**< whether need to expand mono to stereo */
    char                    *expand_buffer;   /**< buffer for channel expansion, used when converting mono to stereo */
    int                     expand_buffer_size;  /**< size of expand buffer */
    uint8_t                 manual_config_gpio_en;  /**< Manual GPIO configuration enable flag */

#if CONFIG_ADK_I2S_STREAM_SUPPORT_MULTIPLE_SOURCE
    int                             current_port_id;        /**< the valid audio port of currently writing i2s data, 0: element->in, >=1: element->multi_in */
    SemaphoreHandle_t               lock;                   /**< input audio port info list lock */
    input_audio_port_info_list_t    input_port_list;        /**< the list of input audio port info */
#endif
} i2s_stream_t;


static i2s_stream_t *gl_i2s_stream = NULL;

#if CONFIG_ADK_I2S_STREAM_SUPPORT_MULTIPLE_SOURCE
#define input_port_list_release(handle) xSemaphoreGive(handle)
#define input_port_list_block(handle, time) xSemaphoreTake(handle, time)
#endif

static int i2s_data_handle_callback(uint32_t size)
{
    I2S_DMA_ISR_START();

    //BK_LOGD(TAG, "[%s] size: %d \n", __func__, size);
    bk_err_t ret = rtos_set_semaphore(&gl_i2s_stream->can_process);
    if (ret != BK_OK)
    {
        BK_LOGV(TAG, "%s, rtos_set_semaphore fail \n", __func__);
    }

    I2S_DMA_ISR_END();

    return size;
}


static int _i2s_close(audio_element_handle_t self)
{
    I2S_CLOSE_START();

    BK_LOGD(TAG, "[%s] _i2s_close \n", audio_element_get_tag(self));

    i2s_stream_t *i2s_stream = (i2s_stream_t *)audio_element_getdata(self);

    if (!i2s_stream->is_open)
    {
        BK_LOGD(TAG, "[%s] i2s already closed \n", audio_element_get_tag(self));
        I2S_CLOSE_END();
        return BK_OK;
    }

    /* stop i2s */
    bk_i2s_stop();

    i2s_stream->is_open = false;

    BK_LOGD(TAG, "[%s] i2s close successful \n", audio_element_get_tag(self));

    I2S_CLOSE_END();

    return BK_OK;
}

static int _i2s_open(audio_element_handle_t self)
{
    I2S_OPEN_START();

    BK_LOGD(TAG, "[%s] _i2s_open \n", audio_element_get_tag(self));

    i2s_stream_t *i2s_stream = (i2s_stream_t *)audio_element_getdata(self);

    if (i2s_stream->is_open)
    {
        BK_LOGD(TAG, "[%s] i2s already opened \n", audio_element_get_tag(self));
        I2S_OPEN_END();
        return BK_OK;
    }

    /* check if need to expand mono to stereo for specific work modes in WRITER mode */
    if (i2s_stream->type == AUDIO_STREAM_WRITER && i2s_stream->need_channel_expand)
    {
        /* allocate expand buffer with double size for stereo conversion */
        i2s_stream->expand_buffer_size = i2s_stream->buff_size * 2;
        if (!i2s_stream->expand_buffer)
        {
            i2s_stream->expand_buffer = (char *)audio_malloc(i2s_stream->expand_buffer_size);
            if (!i2s_stream->expand_buffer)
            {
                BK_LOGE(TAG, "%s, %d, alloc expand buffer fail \n", __func__, __LINE__);
                I2S_OPEN_END();
                return BK_FAIL;
            }
        }
        BK_LOGI(TAG, "channel expand enabled, mono to stereo \n");
    }

    /* set read/write data timeout */
    if (i2s_stream->type == AUDIO_STREAM_READER)
    {
        audio_element_set_output_timeout(self, 0);
    }
    else if (i2s_stream->type == AUDIO_STREAM_WRITER)
    {
        audio_element_set_input_timeout(self, 0);
    }

    /* write initial data for TX mode */
    if (i2s_stream->type == AUDIO_STREAM_WRITER)
    {
        uint32_t dma_rb_size = 0;
        if (i2s_stream->i2s_cfg.pcm_chl_num == 1 &&
            (i2s_stream->i2s_cfg.work_mode == I2S_WORK_MODE_I2S ||
             i2s_stream->i2s_cfg.work_mode == I2S_WORK_MODE_LEFTJUST ||
             i2s_stream->i2s_cfg.work_mode == I2S_WORK_MODE_RIGHTJUST))
        {
            dma_rb_size = i2s_stream->buff_size<<2;
        }
        else
        {
            dma_rb_size = i2s_stream->buff_size<<1;
        }

        uint8_t *temp_data = (uint8_t *)audio_malloc(dma_rb_size);
        if (temp_data)
        {
            os_memset(temp_data, 0x00, dma_rb_size);
            ring_buffer_write(i2s_stream->rb, temp_data, dma_rb_size);
            audio_free(temp_data);
        }
    }

    /* start i2s */
    if (BK_OK != bk_i2s_start())
    {
        BK_LOGE(TAG, "[%s] %s, %d, start i2s fail \n", audio_element_get_tag(self), __func__, __LINE__);
        I2S_OPEN_END();
        return BK_FAIL;
    }

#if CONFIG_ADK_I2S_STREAM_SUPPORT_MULTIPLE_SOURCE
    /* backup default input port information for WRITER mode */
    if (i2s_stream->type == AUDIO_STREAM_WRITER && audio_element_get_multi_input_max_port_num(self) > 0)
    {
        audio_port_info_t *port_info = audio_port_info_list_get_by_port_id(&i2s_stream->input_port_list, 0);
        if (port_info == NULL)
        {
            input_audio_port_info_item_t *port_info_item = audio_calloc(1, sizeof(input_audio_port_info_item_t));
            if (port_info_item == NULL)
            {
                BK_LOGE(TAG, "[%s] malloc input audio port info item fail \n", audio_element_get_tag(self));
                bk_i2s_stop();
                I2S_OPEN_END();
                return BK_FAIL;
            }
            /* initialize port information with current i2s configuration */
            port_info_item->port_info.sample_rate = i2s_stream->i2s_cfg.samp_rate;
            port_info_item->port_info.chl_num = i2s_stream->i2s_cfg.pcm_chl_num;
            port_info_item->port_info.bits = i2s_stream->i2s_cfg.data_length;
            port_info_item->port_info.port_id = 0;
            port_info_item->port_info.priority = 0;
            port_info_item->port_info.port = audio_element_get_input_port(self);
            STAILQ_INSERT_TAIL(&i2s_stream->input_port_list, port_info_item, next);
            BK_LOGD(TAG, "[%s] backup default input port info: sr=%d, chl=%d, bits=%d \n",
                    audio_element_get_tag(self),
                    port_info_item->port_info.sample_rate,
                    port_info_item->port_info.chl_num,
                    port_info_item->port_info.bits);
        }
    }
#endif

    i2s_stream->is_open = true;

    BK_LOGD(TAG, "[%s] i2s open successful \n", audio_element_get_tag(self));

    I2S_OPEN_END();

    return BK_OK;
}


static int _i2s_read(audio_port_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    I2S_READ_START();

    audio_element_handle_t el = (audio_element_handle_t)context;
    i2s_stream_t *i2s_stream = (i2s_stream_t *)audio_element_getdata(el);

    BK_LOGV(TAG, "[%s] _i2s_read, len: %d \n", audio_element_get_tag(el), len);

    int ret = 0;
    if (i2s_stream->rb && len > 0)
    {
        ret = ring_buffer_read(i2s_stream->rb, (uint8_t *)buffer, len);
        /* count the read data size */
        if (ret > 0)
        {
            I2S_READ_DATA_COUNT_ADD_SIZE(ret);
        }
    }

    I2S_READ_END();

    return ret;
}


static int _i2s_write(audio_port_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    I2S_WRITE_START();

    audio_element_handle_t el = (audio_element_handle_t)context;
    i2s_stream_t *i2s_stream = (i2s_stream_t *)audio_element_getdata(el);

    BK_LOGV(TAG, "[%s] _i2s_write, len: %d \n", audio_element_get_tag(el), len);

    int ret = 0;
    if (i2s_stream->rb && len > 0)
    {
        ret = ring_buffer_write(i2s_stream->rb, (uint8_t *)buffer, len);
        /* count the write data size */
        if (ret > 0)
        {
            I2S_WRITE_DATA_COUNT_ADD_SIZE(ret);
        }
    }

    I2S_WRITE_END();

    return ret;
}

#if CONFIG_ADK_I2S_STREAM_SUPPORT_MULTIPLE_SOURCE
static bk_err_t _update_i2s_config(audio_element_handle_t i2s_stream, uint8_t current_port_id, uint8_t new_port_id)
{
    i2s_stream_t *i2s = (i2s_stream_t *)audio_element_getdata(i2s_stream);
    bk_err_t ret = BK_OK;

    audio_port_info_t *current_port_info = audio_port_info_list_get_by_port_id(&i2s->input_port_list, current_port_id);
    audio_port_info_t *new_port_info = audio_port_info_list_get_by_port_id(&i2s->input_port_list, new_port_id);

    if (new_port_info)
    {
        /* check whether the port information is changed */
        if (current_port_info && current_port_info->sample_rate == new_port_info->sample_rate &&
            current_port_info->chl_num == new_port_info->chl_num &&
            current_port_info->bits == new_port_info->bits)
        {
            BK_LOGV(TAG, "%s, line: %d, the port information is not changed \n", __func__, __LINE__);
            i2s->current_port_id = new_port_id;
        }
        else
        {
            /* update i2s configuration */
            bool was_open = i2s->is_open;
            if (was_open)
            {
                //_i2s_close(i2s_stream);
            }

            /* update sample rate if changed */
            if (!current_port_info || current_port_info->sample_rate != new_port_info->sample_rate)
            {
                i2s->i2s_cfg.samp_rate = new_port_info->sample_rate;
                BK_LOGD(TAG, "%s, line: %d, update i2s sample rate: %d->%d ok \n", __func__, __LINE__,
                        current_port_info ? current_port_info->sample_rate : -1, new_port_info->sample_rate);
            }

            /* update channel number if changed */
            if (!current_port_info || current_port_info->chl_num != new_port_info->chl_num)
            {
                i2s->i2s_cfg.pcm_chl_num = new_port_info->chl_num;
                BK_LOGD(TAG, "%s, line: %d, update i2s channel: %d->%d ok \n", __func__, __LINE__,
                        current_port_info ? current_port_info->chl_num : -1, new_port_info->chl_num);

                /* update channel expand configuration if needed */
                if (i2s->type == AUDIO_STREAM_WRITER)
                {
                    i2s->need_channel_expand = (new_port_info->chl_num == 1 &&
                                      (i2s->i2s_cfg.work_mode == I2S_WORK_MODE_I2S ||
                                       i2s->i2s_cfg.work_mode == I2S_WORK_MODE_LEFTJUST ||
                                       i2s->i2s_cfg.work_mode == I2S_WORK_MODE_RIGHTJUST));

                }
            }

            /* not support update bits */
            if (!current_port_info || current_port_info->bits != new_port_info->bits)
            {
                BK_LOGD(TAG, "%s, line: %d, not support update i2s bits: %d->%d \n", __func__, __LINE__,
                        current_port_info ? current_port_info->bits : -1, new_port_info->bits);
            }

            i2s->current_port_id = new_port_id;

            /* reopen i2s if it was previously open */
            if (was_open)
            {
                //ret = _i2s_open(i2s_stream);
                if (ret != BK_OK)
                {
                    BK_LOGE(TAG, "%s, line: %d, reopen i2s fail \n", __func__, __LINE__);
                }
            }
        }
    }
    else
    {
        BK_LOGE(TAG, "%s, line: %d, new_port_id: %d is not valid \n", __func__, __LINE__, new_port_id);
        return BK_FAIL;
    }

    return ret;
}
#endif

static int _i2s_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    I2S_PROCESS_START();

    i2s_stream_t *i2s_stream = (i2s_stream_t *)audio_element_getdata(self);

    BK_LOGV(TAG, "[%s] _i2s_process, in_len: %d \n", audio_element_get_tag(self), in_len);

    /* wait for semaphore */
    if (BK_OK != rtos_get_semaphore(&i2s_stream->can_process, 2000 / portTICK_RATE_MS))
    {
        BK_LOGW(TAG, "[%s] semaphore get timeout 2000ms\n", audio_element_get_tag(self));
        //return 0;
    }

    /* read input data */
#if CONFIG_ADK_I2S_STREAM_SUPPORT_MULTIPLE_SOURCE
    int r_size = 0;
    /* check if multi input port is configured for WRITER mode */
    if (i2s_stream->type == AUDIO_STREAM_WRITER && audio_element_get_multi_input_max_port_num(self) > 0)
    {
        int old_port_id = i2s_stream->current_port_id;
        int valid_port_id = audio_port_info_list_get_valid_port_id(&i2s_stream->input_port_list);
        if (valid_port_id == -1)
        {
            /* Use default audio port, if all audio port is empty */
            BK_LOGV(TAG, "%s, line: %d, valid_port_id: %d \n", __func__, __LINE__, valid_port_id);
            valid_port_id = 0;
        }

        /* switch to new valid port if changed */
        if (valid_port_id != i2s_stream->current_port_id)
        {
            BK_LOGD(TAG, "%s, line: %d, valid_port_id: %d -> %d\n", __func__, __LINE__, i2s_stream->current_port_id, valid_port_id);
            if (BK_OK != _update_i2s_config(self, i2s_stream->current_port_id, valid_port_id))
            {
                BK_LOGE(TAG, "%s, line: %d, update i2s config fail \n", __func__, __LINE__);
                /* continue processing even if update fails */
            }
        }

        /* handle audio port state notify callback */
        if (old_port_id != i2s_stream->current_port_id)
        {
            if (old_port_id == 0)
            {
                input_port_list_block(i2s_stream->lock, portMAX_DELAY);
                audio_port_info_t *current_port_info = audio_port_info_list_get_by_port_id(&i2s_stream->input_port_list, i2s_stream->current_port_id);
                if (current_port_info && current_port_info->notify_cb)
                {
                    current_port_info->notify_cb(APT_STATE_RUNNING, (void *)current_port_info, current_port_info->user_data);
                }
                input_port_list_release(i2s_stream->lock);
            }
            else
            {
                if (i2s_stream->current_port_id == 0)
                {
                    input_port_list_block(i2s_stream->lock, portMAX_DELAY);
                    audio_port_info_t *old_port_info = audio_port_info_list_get_by_port_id(&i2s_stream->input_port_list, old_port_id);
                    if (old_port_info && old_port_info->notify_cb)
                    {
                        old_port_info->notify_cb(APT_STATE_FINISHED, (void *)old_port_info, old_port_info->user_data);
                    }
                    input_port_list_release(i2s_stream->lock);
                }
                else
                {
                    input_port_list_block(i2s_stream->lock, portMAX_DELAY);
                    audio_port_info_t *current_port_info = audio_port_info_list_get_by_port_id(&i2s_stream->input_port_list, i2s_stream->current_port_id);
                    audio_port_info_t *old_port_info = audio_port_info_list_get_by_port_id(&i2s_stream->input_port_list, old_port_id);

                    if (current_port_info && current_port_info->notify_cb)
                    {
                        current_port_info->notify_cb(APT_STATE_RUNNING, (void *)current_port_info, current_port_info->user_data);
                    }

                    if (old_port_info != NULL)
                    {
                        if (current_port_info->priority > old_port_info->priority)
                        {
                            if (old_port_info && old_port_info->notify_cb)
                            {
                                old_port_info->notify_cb(APT_STATE_PAUSED, (void *)old_port_info, old_port_info->user_data);
                            }
                        }
                        else
                        {
                            if (old_port_info && old_port_info->notify_cb)
                            {
                                old_port_info->notify_cb(APT_STATE_FINISHED, (void *)old_port_info, old_port_info->user_data);
                            }
                        }
                    }

                    input_port_list_release(i2s_stream->lock);
                }
            }
            BK_LOGD(TAG, "%s, line: %d, old_port_id: %d -> %d\n", __func__, __LINE__, old_port_id, i2s_stream->current_port_id);
        }

        /* read data from valid port */
        if (i2s_stream->current_port_id == 0)
        {
            r_size = audio_element_input(self, in_buffer, in_len);
        }
        else
        {
            r_size = audio_element_multi_input(self, in_buffer, in_len, i2s_stream->current_port_id - 1, 0);
            //BK_LOGD(TAG, "%s, line: %d, multi_input: %d, in_len: %d, r_size: %d \n", __func__, __LINE__, i2s_stream->current_port_id - 1, in_len, r_size);
        }
    }
    else
    {
        r_size = audio_element_input(self, in_buffer, in_len);
    }
#else
    int r_size = audio_element_input(self, in_buffer, in_len);
#endif

    int w_size = 0;

    if (r_size == AEL_IO_TIMEOUT)
    {
        if (i2s_stream->type == AUDIO_STREAM_WRITER && i2s_stream->need_channel_expand)
        {
            os_memset(i2s_stream->expand_buffer, 0x00, i2s_stream->expand_buffer_size);
            w_size = audio_element_output(self, i2s_stream->expand_buffer, i2s_stream->expand_buffer_size);
        }
        else
        {
            w_size = audio_element_output(self, in_buffer, r_size);
        }
    }
    else if (r_size > 0)
    {
        /* perform mono to stereo expansion if needed */
        if (i2s_stream->need_channel_expand && i2s_stream->expand_buffer)
        {
            /* check data alignment for 16-bit samples */
            if (r_size % sizeof(int16_t) != 0)
            {
                BK_LOGE(TAG, "[%s] invalid data size for 16-bit samples: %d \n", audio_element_get_tag(self), r_size);
                w_size = 0;
            }
            /* ensure the expanded data fits in the buffer */
            else if (r_size * 2 <= i2s_stream->expand_buffer_size)
            {
                /* convert mono to stereo by duplicating each sample */
                int16_t *src = (int16_t *)in_buffer;
                int16_t *dst = (int16_t *)i2s_stream->expand_buffer;
                int samples = r_size / sizeof(int16_t);  /* number of 16-bit samples */

                for (int i = 0; i < samples; i++)
                {
                    dst[i * 2] = src[i];        /* left channel */
                    dst[i * 2 + 1] = src[i];    /* right channel (same as left) */
                }

                /* write expanded stereo data */
                w_size = audio_element_output(self, i2s_stream->expand_buffer, r_size * 2);
                /* write data to multi output ports if configured */
                audio_element_multi_output(self, in_buffer, r_size, 0);

                BK_LOGV(TAG, "[%s] mono to stereo: %d -> %d bytes \n", audio_element_get_tag(self), r_size, r_size * 2);
            }
            else
            {
                BK_LOGE(TAG, "[%s] expand buffer too small: %d < %d \n", audio_element_get_tag(self), i2s_stream->expand_buffer_size, r_size * 2);
                w_size = 0;
            }
        }
        else
        {
            /* no channel expansion needed, write data directly */
            w_size = audio_element_output(self, in_buffer, r_size);
            /* write data to multi output ports if configured */
            audio_element_multi_output(self, in_buffer, r_size, 0);
        }
    }
    else
    {
#if CONFIG_ADK_I2S_STREAM_SUPPORT_MULTIPLE_SOURCE
        /* When reading data from other sources returns AEL_IO_ABORT, it indicates that the data source has stopped.
           In this case, no error code is returned to prevent the i2s from stopping playback.
         */
        if (i2s_stream->current_port_id > 0)
        {
            w_size = in_len;
        }
        else
        {
            w_size = r_size;
        }
#else
        w_size = r_size;
#endif
    }

    I2S_PROCESS_END();

    return w_size;
}


static int _i2s_destroy(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] _i2s_destroy \n", audio_element_get_tag(self));

    i2s_stream_t *i2s_stream = (i2s_stream_t *)audio_element_getdata(self);

#if CONFIG_ADK_I2S_STREAM_SUPPORT_MULTIPLE_SOURCE
    /* free input port list */
    if (i2s_stream && i2s_stream->lock)
    {
        audio_port_info_list_clear(&i2s_stream->input_port_list);
        vSemaphoreDelete(i2s_stream->lock);
        i2s_stream->lock = NULL;
    }
#endif

    if (i2s_stream)
    {
        /* deinit i2s channel */
        i2s_txrx_type_t txrx_type = (i2s_stream->type == AUDIO_STREAM_READER) ? I2S_TXRX_TYPE_RX : I2S_TXRX_TYPE_TX;
        bk_i2s_chl_deinit(i2s_stream->channel_id, txrx_type);

        /* deinit i2s */
        bk_i2s_deinit();

        /* clear global i2s stream pointer */
        gl_i2s_stream = NULL;

        /* deinit i2s driver */
        bk_i2s_driver_deinit();

        /* deinit semaphore */
        if (i2s_stream->can_process)
        {
            rtos_deinit_semaphore(&i2s_stream->can_process);
            i2s_stream->can_process = NULL;
        }

        /* free expand buffer */
        if (i2s_stream->expand_buffer)
        {
            audio_free(i2s_stream->expand_buffer);
            i2s_stream->expand_buffer = NULL;
            i2s_stream->expand_buffer_size = 0;
        }

        /* destroy data count utility based on stream type */
        if (i2s_stream->type == AUDIO_STREAM_READER)
        {
            I2S_READ_DATA_COUNT_CLOSE();
        }
        else if (i2s_stream->type == AUDIO_STREAM_WRITER)
        {
            I2S_WRITE_DATA_COUNT_CLOSE();
        }

        audio_free(i2s_stream);
        i2s_stream = NULL;
    }

    return BK_OK;
}

#if CONFIG_ADK_I2S_STREAM_SUPPORT_MULTIPLE_SOURCE
bk_err_t i2s_stream_get_input_port_info_by_port_id(audio_element_handle_t i2s_stream, uint8_t port_id, audio_port_info_t **port_info)
{
    if (!i2s_stream || !port_info)
    {
        BK_LOGE(TAG, "%s, %d, invalid parameter\n", __func__, __LINE__);
        return BK_ERR_ADK_INVALID_ARG;
    }

    i2s_stream_t *i2s = (i2s_stream_t *)audio_element_getdata(i2s_stream);
    if (!i2s)
    {
        BK_LOGE(TAG, "%s, %d, i2s_stream is NULL\n", __func__, __LINE__);
        return BK_ERR_ADK_INVALID_ARG;
    }

    if (audio_element_get_multi_input_max_port_num(i2s_stream) <= 0)
    {
        BK_LOGE(TAG, "%s, %d, multi_in_port_num is 0\n", __func__, __LINE__);
        return BK_ERR_ADK_INVALID_ARG;
    }

    input_port_list_block(i2s->lock, portMAX_DELAY);
    *port_info = audio_port_info_list_get_by_port_id(&i2s->input_port_list, port_id);
    input_port_list_release(i2s->lock);

    if (*port_info == NULL)
    {
        BK_LOGE(TAG, "%s, %d, port_info is NULL, port_id: %d\n", __func__, __LINE__, port_id);
        return BK_FAIL;
    }

    return BK_OK;
}

bk_err_t i2s_stream_set_input_port_info(audio_element_handle_t i2s_stream, audio_port_info_t *port_info)
{
    if (!i2s_stream || !port_info)
    {
        BK_LOGE(TAG, "%s, %d, invalid parameter\n", __func__, __LINE__);
        return BK_ERR_ADK_INVALID_ARG;
    }

    i2s_stream_t *i2s = (i2s_stream_t *)audio_element_getdata(i2s_stream);
    if (!i2s)
    {
        BK_LOGE(TAG, "%s, %d, i2s_stream is NULL\n", __func__, __LINE__);
        return BK_ERR_ADK_INVALID_ARG;
    }

    if (port_info->port_id > audio_element_get_multi_input_max_port_num(i2s_stream))
    {
        BK_LOGE(TAG, "%s, %d, port_id: %d out of range: 0 ~ %d \n", __func__, __LINE__, port_info->port_id, audio_element_get_multi_input_max_port_num(i2s_stream) + 1);
        return BK_ERR_ADK_INVALID_ARG;
    }

    BK_LOGD(TAG, "%s, line: %d, port_id: %d, priority: %d, port: %p \n", __func__, __LINE__, port_info->port_id, port_info->priority, port_info->port);

    /* Check all ports in the input port list to see if there is a port with the same port ID as port_info, and update the port information */
    audio_port_info_t *tmp_port_info = audio_port_info_list_get_by_port_id(&i2s->input_port_list, port_info->port_id);
    if (tmp_port_info)
    {
        /* check whether port_info is same as tmp_port_info */
        if (memcmp(tmp_port_info, port_info, sizeof(audio_port_info_t)) == 0)
        {
            BK_LOGD(TAG, "%s, line: %d, port_info is same as tmp_port_info, not update\n", __func__, __LINE__);
            return BK_OK;
        }

        /* update audio port info in port list */
        input_port_list_block(i2s->lock, portMAX_DELAY);
        audio_port_info_list_update(&i2s->input_port_list, port_info);
        input_port_list_release(i2s->lock);
        /* check and update audio port */
        if (port_info->port_id == 0)
        {
            if (audio_element_get_input_port(i2s_stream) != port_info->port)
            {
                audio_element_set_input_port(i2s_stream, port_info->port);
            }
        }
        else
        {
            if (audio_element_get_multi_input_port(i2s_stream, port_info->port_id - 1) != port_info->port)
            {
                audio_element_set_multi_input_port(i2s_stream, port_info->port, (int)(port_info->port_id - 1));
            }
        }
    }
    else
    {
        /* if port_info->port is NULL, not add to list */
        if (port_info->port != NULL)
        {
            /* add new port */
            input_port_list_block(i2s->lock, portMAX_DELAY);
            audio_port_info_list_add(&i2s->input_port_list, port_info);
            input_port_list_release(i2s->lock);
        }
        /* check and update audio port */
        if (port_info->port_id == 0)
        {
            audio_element_set_input_port(i2s_stream, port_info->port);
        }
        else
        {
            audio_element_set_multi_input_port(i2s_stream, port_info->port, (int)(port_info->port_id - 1));
        }
    }

    return BK_OK;
}
#endif


audio_element_handle_t i2s_stream_init(i2s_stream_cfg_t *config)
{
    if (!config)
    {
        BK_LOGE(TAG, "config is NULL\n");
        return NULL;
    }

    audio_element_handle_t el;
    i2s_stream_t *i2s_stream = audio_calloc(1, sizeof(i2s_stream_t));
    AUDIO_MEM_CHECK(TAG, i2s_stream, return NULL);

    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.open = _i2s_open;
    cfg.close = _i2s_close;
    cfg.process = _i2s_process;
    cfg.destroy = _i2s_destroy;
    cfg.task_stack = config->task_stack;
    cfg.task_prio = config->task_prio;
    cfg.task_core = config->task_core;
    cfg.buffer_len = config->out_block_size;
    cfg.out_block_size = config->out_block_size;
    cfg.out_block_num = config->out_block_num;
    cfg.multi_in_port_num = config->multi_in_port_num;
    cfg.multi_out_port_num = config->multi_out_port_num;

    if (config->type == AUDIO_STREAM_READER)
    {
        cfg.in_type = PORT_TYPE_CB;
        cfg.read = _i2s_read;
        cfg.out_type = PORT_TYPE_RB;
        cfg.write = NULL;
    }
    else if (config->type == AUDIO_STREAM_WRITER)
    {
        cfg.in_type = PORT_TYPE_RB;
        cfg.read = NULL;
        cfg.out_type = PORT_TYPE_CB;
        cfg.write = _i2s_write;
    }
    else
    {
        BK_LOGE(TAG, "i2s type: %d, is not support, please check\n", config->type);
        goto _i2s_init_exit;
    }

    cfg.tag = "i2s_stream";
    BK_LOGD(TAG, "buffer_len: %d, out_block_size: %d, out_block_num: %d\n", cfg.buffer_len, cfg.out_block_size, cfg.out_block_num);

    /* config i2s */
    i2s_stream->gpio_group = config->gpio_group;
    i2s_stream->i2s_cfg.role = config->role;
    i2s_stream->i2s_cfg.work_mode = config->work_mode;
    i2s_stream->i2s_cfg.lrck_invert = config->lrck_invert;
    i2s_stream->i2s_cfg.sck_invert = config->sck_invert;
    i2s_stream->i2s_cfg.lsb_first_en = config->lsb_first_en;
    i2s_stream->i2s_cfg.sync_length = config->sync_length;
    i2s_stream->i2s_cfg.data_length = config->data_length;
    i2s_stream->i2s_cfg.pcm_dlength = config->pcm_dlength;
    i2s_stream->i2s_cfg.store_mode = config->store_mode;
    i2s_stream->i2s_cfg.samp_rate = config->samp_rate;
    i2s_stream->i2s_cfg.pcm_chl_num = config->pcm_chl_num;
    i2s_stream->channel_id = config->channel_id;
    i2s_stream->type = config->type;
    i2s_stream->buff_size = config->buff_size;
    i2s_stream->out_block_size = config->out_block_size;
    i2s_stream->out_block_num = config->out_block_num;
    i2s_stream->is_open = false;
    i2s_stream->can_process = NULL;
    i2s_stream->need_channel_expand = false;
    i2s_stream->expand_buffer = NULL;
    i2s_stream->expand_buffer_size = 0;
    i2s_stream->manual_config_gpio_en = config->manual_config_gpio_en;

    /* init semaphore */
    bk_err_t ret = rtos_init_semaphore(&i2s_stream->can_process, 1);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "%s, %d, rtos_init_semaphore fail\n", __func__, __LINE__);
        goto _i2s_init_exit;
    }

    /* init i2s driver */
    if (BK_OK != bk_i2s_driver_init())
    {
        BK_LOGE(TAG, "%s, %d, init i2s driver fail \n", __func__, __LINE__);
        goto _i2s_init_exit;
    }

    /* init i2s configure */
    if (config->manual_config_gpio_en)
    {
        // GPIO configuration is done by application layer
        if (BK_OK != bk_i2s_init_without_gpio(i2s_stream->gpio_group, &i2s_stream->i2s_cfg))
        {
            BK_LOGE(TAG, "%s, %d, init i2s config without gpio fail \n", __func__, __LINE__);
            goto _i2s_init_exit;
        }
    }
    else
    {
        // GPIO configuration is done by driver
        if (BK_OK != bk_i2s_init(i2s_stream->gpio_group, &i2s_stream->i2s_cfg))
        {
            BK_LOGE(TAG, "%s, %d, init i2s config fail \n", __func__, __LINE__);
            goto _i2s_init_exit;
        }
    }

    /* init i2s channel */
    if (i2s_stream->type == AUDIO_STREAM_WRITER &&
            i2s_stream->i2s_cfg.pcm_chl_num == 1 &&
            (i2s_stream->i2s_cfg.work_mode == I2S_WORK_MODE_I2S ||
             i2s_stream->i2s_cfg.work_mode == I2S_WORK_MODE_LEFTJUST ||
             i2s_stream->i2s_cfg.work_mode == I2S_WORK_MODE_RIGHTJUST))
    {
        i2s_stream->need_channel_expand = true;
        /* allocate expand buffer with double size for stereo conversion */
        i2s_stream->expand_buffer_size = i2s_stream->buff_size * 2;
        i2s_stream->expand_buffer = (char *)audio_malloc(i2s_stream->expand_buffer_size);
        if (!i2s_stream->expand_buffer)
        {
            BK_LOGE(TAG, "%s, %d, alloc expand buffer fail \n", __func__, __LINE__);
            goto _i2s_init_exit;
        }
    }

    i2s_txrx_type_t txrx_type = (i2s_stream->type == AUDIO_STREAM_READER) ? I2S_TXRX_TYPE_RX : I2S_TXRX_TYPE_TX;
    uint32_t dma_rb_size = i2s_stream->need_channel_expand ? i2s_stream->buff_size<<2 : i2s_stream->buff_size<<1;

    if (BK_OK != bk_i2s_chl_init(i2s_stream->channel_id, txrx_type, dma_rb_size, i2s_data_handle_callback, &i2s_stream->rb))
    {
        BK_LOGE(TAG, "%s, %d, init i2s channel fail \n", __func__, __LINE__);
        goto _i2s_init_exit;
    }

    gl_i2s_stream = i2s_stream;

#if CONFIG_ADK_I2S_STREAM_SUPPORT_MULTIPLE_SOURCE
    i2s_stream->current_port_id = 0;

    /* init input port list if multi_in_port_num > 0 */
    if (cfg.multi_in_port_num > 0)
    {
        audio_port_info_list_init(&i2s_stream->input_port_list);

        i2s_stream->lock = xSemaphoreCreateMutex();
        if (i2s_stream->lock == NULL)
        {
            BK_LOGE(TAG, "[%s] create semaphore fail \n", cfg.tag);
            goto _i2s_init_exit_with_chl_deinit;
        }
    }
#endif

    BK_LOGD(TAG, "gpio_group: %d, role: %d, work_mode: %d, samp_rate: %d, channel_id: %d, type: %d\n",
            i2s_stream->gpio_group, i2s_stream->i2s_cfg.role, i2s_stream->i2s_cfg.work_mode,
            i2s_stream->i2s_cfg.samp_rate, i2s_stream->channel_id, i2s_stream->type);

    el = audio_element_init(&cfg);
    AUDIO_MEM_CHECK(TAG, el, goto _i2s_init_exit_with_chl_deinit);
    audio_element_setdata(el, i2s_stream);

    audio_element_info_t info = {0};
    info.sample_rates = 16000;
    info.channels = 2;
    info.bits = 16;
    info.codec_fmt = BK_CODEC_TYPE_PCM;
    audio_element_setinfo(el, &info);

    /* initialize data count utility based on stream type */
    if (i2s_stream->type == AUDIO_STREAM_READER)
    {
        I2S_READ_DATA_COUNT_OPEN();
    }
    else if (i2s_stream->type == AUDIO_STREAM_WRITER)
    {
        I2S_WRITE_DATA_COUNT_OPEN();
    }

    return el;

/* error handling: release resources in reverse order */
_i2s_init_exit_with_chl_deinit:
#if CONFIG_ADK_I2S_STREAM_SUPPORT_MULTIPLE_SOURCE
    if (i2s_stream && i2s_stream->lock)
    {
        audio_port_info_list_clear(&i2s_stream->input_port_list);
        vSemaphoreDelete(i2s_stream->lock);
        i2s_stream->lock = NULL;
    }
#endif
    bk_i2s_chl_deinit(i2s_stream->channel_id, txrx_type);

_i2s_init_exit:

    if (i2s_stream->expand_buffer)
    {
        audio_free(i2s_stream->expand_buffer);
        i2s_stream->expand_buffer = NULL;
        i2s_stream->expand_buffer_size = 0;
    }

    bk_i2s_deinit();

    gl_i2s_stream = NULL;

    bk_i2s_driver_deinit();

    if (i2s_stream && i2s_stream->can_process)
    {
        rtos_deinit_semaphore(&i2s_stream->can_process);
        i2s_stream->can_process = NULL;
    }

    audio_free(i2s_stream);
    i2s_stream = NULL;
    return NULL;
}

