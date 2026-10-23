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
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include <components/bk_audio/audio_decoders/mp3_decoder.h>
#include <components/bk_audio/audio_pipeline/audio_types.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_error.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <os/os.h>
#include <components/bk_audio/audio_utils/debug_dump_util.h>



#define TAG  "MP3_DECODER"


/* dump mp3_decoder stream output pcm data by uart */
//#define MP3_DEC_DATA_DUMP_BY_UART

#ifdef MP3_DEC_DATA_DUMP_BY_UART
#include <components/bk_audio/audio_utils/uart_util.h>
static struct uart_util gl_mp3_dec_uart_util = {0};
#define MP3_DEC_DATA_DUMP_UART_ID            (1)
#define MP3_DEC_DATA_DUMP_UART_BAUD_RATE     (2000000)

#define MP3_DEC_DATA_DUMP_BY_UART_OPEN()                    uart_util_create(&gl_mp3_dec_uart_util, MP3_DEC_DATA_DUMP_UART_ID, MP3_DEC_DATA_DUMP_UART_BAUD_RATE)
#define MP3_DEC_DATA_DUMP_BY_UART_CLOSE()                   uart_util_destroy(&gl_mp3_dec_uart_util)
#define MP3_DEC_DATA_DUMP_BY_UART_DATA(data_buf, len)       uart_util_tx_data(&gl_mp3_dec_uart_util, data_buf, len)

#else

#define MP3_DEC_DATA_DUMP_BY_UART_OPEN()
#define MP3_DEC_DATA_DUMP_BY_UART_CLOSE()
#define MP3_DEC_DATA_DUMP_BY_UART_DATA(data_buf, len)

#endif  //MP3_DEC_DATA_DUMP_BY_UART


typedef struct mp3_decoder
{
    HMP3Decoder dec_handle;             /**< mp3 decoder handle */
    MP3FrameInfo frame_info;            /**< mp3 frame infomation */
    uint8_t *main_buff;                 /**< mainbuff save data read */
    uint32_t main_buff_size;            /**< mainbuff size */
    uint32_t main_buff_remain_size;     /**< mainbuff remain size */
    int16_t *out_pcm_buff;              /**< out pcm buffer save data decoded */
    uint32_t out_pcm_buff_size;         /**< out pcm buffer size */
    uint8_t *main_buff_readptr;         /**< read ptr of main buffer */
    bool skip_idtag_done;               /**< flag to indicate if skip_idtag operation has been done */
} mp3_decoder_t;


static int codec_mp3_read_bytes(audio_element_handle_t self, char *buffer, int wanted_size)
{
    int total_read = 0;
    int remaining = wanted_size;

    while (remaining > 0)
    {
        int r_size = audio_element_input(self, buffer + total_read, remaining);

        if (r_size > 0)
        {
            // Successfully read some data
            total_read += r_size;
            remaining -= r_size;
        }
        else if (r_size == AEL_IO_TIMEOUT)
        {
            // Timeout occurred, continue reading (data may not be ready yet)
            // Keep looping until data is available, exhausted, or error occurs
            BK_LOGV(TAG, "[%s] %s, timeout, continue reading, total_read:%d/%d \n", audio_element_get_tag(self), __func__, total_read, wanted_size);
            continue;
        }
        else
        {
            // Return value <= 0 (AEL_IO_DONE, AEL_IO_FAIL, AEL_IO_ABORT, etc.)
            // Return partial read if we got some data, otherwise return the error code directly
            BK_LOGW(TAG, "[%s] %s, audio_element_input return: %d, total_read:%d/%d \n", audio_element_get_tag(self), __func__, r_size, total_read, wanted_size);
            return r_size;
        }
    }

    return total_read;
}

/* skip id3 tag */
static int codec_mp3_skip_idtag(audio_element_handle_t self)
{
    int offset = 0;
    uint8_t *tag;
    int r_size = 0;

    mp3_decoder_t *mp3_dec = (mp3_decoder_t *)audio_element_getdata(self);

    tag = mp3_dec->main_buff_readptr;

    // Set timeout to maximum delay for skip_idtag operation
    audio_element_set_input_timeout(self, portMAX_DELAY);

    /* read idtag v2 */
    // Read first 3 bytes to check if it's ID3 tag
    r_size = codec_mp3_read_bytes(self, (char *)(mp3_dec->main_buff), 3);
    if (r_size != 3)
    {
        // Return value is not the expected length, check if it's AEL_IO_FAIL
        if (r_size == AEL_IO_FAIL)
        {
            BK_LOGE(TAG, "[%s] %s, failed to read 3 bytes, AEL_IO_FAIL \n", audio_element_get_tag(self), __func__);
            return -1;
        }
        return 0;
    }

    mp3_dec->main_buff_remain_size = 3;

    // Check if it's ID3 tag
    if (tag[0] == 'I' && tag[1] == 'D' && tag[2] == '3')
    {
        int size;

        // Read next 7 bytes to get ID3 tag size
        r_size = codec_mp3_read_bytes(self, (char *)(mp3_dec->main_buff + 3), 7);
        if (r_size != 7)
        {
            // Return value is not the expected length, check if it's AEL_IO_FAIL
            if (r_size == AEL_IO_FAIL)
            {
                BK_LOGE(TAG, "[%s] %s, failed to read ID3 tag header, AEL_IO_FAIL \n", audio_element_get_tag(self), __func__);
                return -1;
            }
            return 0;
        }

        // Calculate ID3 tag size
        size = ((tag[6] & 0x7F) << 21) | ((tag[7] & 0x7F) << 14) | ((tag[8] & 0x7F) << 7) | ((tag[9] & 0x7F));
        offset = size + 10;

        BK_LOGV(TAG, "[%s] %s, ID3 tag detected, size: %d, total offset: %d \n", audio_element_get_tag(self), __func__, size, offset);

        /* read all of idv3 tag data */
        if (size > 0)
        {
            int rest_size = size;
            int total_read = 0;

            while (rest_size > 0)
            {
                int chunk;
                int length;

                // Determine chunk size based on remaining size and buffer size
                if (rest_size > mp3_dec->main_buff_size)
                {
                    chunk = mp3_dec->main_buff_size;
                }
                else
                {
                    chunk = rest_size;
                }

                // Read chunk of data
                length = codec_mp3_read_bytes(self, (char *)(mp3_dec->main_buff), chunk);

                if (length == chunk)
                {
                    // Successfully read the expected chunk size
                    total_read += length;
                    rest_size -= length;
                }
                else
                {
                    // Return value is not the expected length, check if it's AEL_IO_FAIL
                    if (length == AEL_IO_FAIL)
                    {
                        BK_LOGE(TAG, "[%s] %s, AEL_IO_FAIL while reading ID3 tag, read %d/%d bytes \n", audio_element_get_tag(self), __func__, total_read, size);
                        return -1;
                    }
                    return 0;
                }
            }

            BK_LOGV(TAG, "[%s] %s, successfully skipped ID3 tag, total size: %d bytes \n", audio_element_get_tag(self), __func__, total_read);
        }

        // Reset buffer pointers after skipping ID3 tag
        mp3_dec->main_buff_remain_size = 0;
        mp3_dec->main_buff_readptr = mp3_dec->main_buff;
    }
    else
    {
        // Not an ID3 tag, no offset
        BK_LOGV(TAG, "[%s] %s, no ID3 tag detected \n", audio_element_get_tag(self), __func__);
        offset = 0;
    }

    return offset;
}

static bk_err_t _mp3_decoder_open(audio_element_handle_t self)
{
    BK_LOGV(TAG, "[%s] _mp3_decoder_open \n", audio_element_get_tag(self));
    mp3_decoder_t *mp3_dec = (mp3_decoder_t *)audio_element_getdata(self);
    mp3_dec->main_buff_readptr = mp3_dec->main_buff;

    /* set read data timeout */
    audio_element_set_input_timeout(self, portMAX_DELAY);   // 2000, 15 / portTICK_RATE_MS

    return BK_OK;
}

static bk_err_t _mp3_decoder_close(audio_element_handle_t self)
{
    BK_LOGV(TAG, "[%s] _mp3_decoder_close \n", audio_element_get_tag(self));
    mp3_decoder_t *mp3_dec = (mp3_decoder_t *)audio_element_getdata(self);
    audio_element_state_t state = audio_element_get_state(self);

    // Reset skip_idtag_done flag and info when component is not in PAUSED state
    // Keep the flag unchanged when in PAUSED state to avoid re-executing skip_idtag after resume
    if (state != AEL_STATE_PAUSED)
    {
        mp3_dec->skip_idtag_done = false;
        // Reset info to default values to ensure music info will be reported on next open
        audio_element_info_t info = {0};
        bk_err_t ret = audio_element_getinfo(self, &info);
        if (ret == BK_OK)
        {
            info.sample_rates = 0;
            info.channels = 0;
            info.bits = 0;
            audio_element_setinfo(self, &info);
        }
        BK_LOGV(TAG, "[%s] Component in state %d, reset skip_idtag_done flag and info \n", audio_element_get_tag(self), state);
    }
    else
    {
        BK_LOGV(TAG, "[%s] Component in PAUSED state, keep skip_idtag_done flag unchanged \n", audio_element_get_tag(self));
    }

    return BK_OK;
}

static bk_err_t music_info_report(audio_element_handle_t self)
{
    mp3_decoder_t *mp3_dec = (mp3_decoder_t *)audio_element_getdata(self);

    audio_element_info_t info = {0};
    bk_err_t ret = audio_element_getinfo(self, &info);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "[%s] audio_element_getinfo fail \n", audio_element_get_tag(self));
        return BK_FAIL;
    }

    /* check frame information, report new frame information if frame information change */
    if (mp3_dec->frame_info.bitsPerSample != info.bits
        || mp3_dec->frame_info.samprate != info.sample_rates
        || mp3_dec->frame_info.nChans != info.channels)
    {
        info.bits = mp3_dec->frame_info.bitsPerSample;
        info.sample_rates = mp3_dec->frame_info.samprate;
        info.channels = mp3_dec->frame_info.nChans;
        ret = audio_element_setinfo(self, &info);
        if (ret != BK_OK)
        {
            BK_LOGE(TAG, "[%s] audio_element_setinfo fail \n", audio_element_get_tag(self));
            return BK_FAIL;
        }
        ret = audio_element_report_info(self);
        if (ret != BK_OK)
        {
            BK_LOGE(TAG, "[%s] audio_element_report_info fail \n", audio_element_get_tag(self));
            return BK_FAIL;
        }
    }
    return BK_OK;
}

static int check_mp3_sync_word(mp3_decoder_t *mp3_dec)
{
    int err;

    MP3FrameInfo frame_info = {0};

    err = MP3GetNextFrameInfo(mp3_dec->dec_handle, &frame_info, mp3_dec->main_buff_readptr);
    if (err == ERR_MP3_INVALID_FRAMEHEADER)
    {
        BK_LOGE(TAG, "%s, ERR_MP3_INVALID_FRAMEHEADER, %d\n", __func__, __LINE__);
        goto __err;
    }
    else if (err != ERR_MP3_NONE)
    {
        BK_LOGE(TAG, "%s, MP3GetNextFrameInfo fail, err=%d, %d\n", __func__, err, __LINE__);
        goto __err;
    }
    else if (frame_info.nChans != 1 && frame_info.nChans != 2)
    {
        BK_LOGE(TAG, "%s, nChans is not 1 or 2, nChans=%d, %d\n", __func__, frame_info.nChans, __LINE__);
        goto __err;
    }
    else if (frame_info.bitsPerSample != 16 && frame_info.bitsPerSample != 8)
    {
        BK_LOGE(TAG, "%s, bitsPerSample is not 16 or 8, bitsPerSample=%d, %d\n", __func__, frame_info.bitsPerSample, __LINE__);
        goto __err;
    }
    else
    {
        //noting todo
    }

    return 0;

__err:
    return -1;
}

static int _mp3_decoder_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    bk_err_t ret = BK_OK;
    int r_size = 0;

    BK_LOGV(TAG, "[%s] _mp3_decoder_process \n", audio_element_get_tag(self));
    mp3_decoder_t *mp3_dec = (mp3_decoder_t *)audio_element_getdata(self);

    // Check if skip_idtag operation needs to be performed
    // This is done in process function instead of open to avoid timeout issues
    // when data stream is not ready yet
    if (!mp3_dec->skip_idtag_done)
    {
        int skip_ret = codec_mp3_skip_idtag(self);
        if (skip_ret < 0)
        {
            BK_LOGE(TAG, "[%s] codec_mp3_skip_idtag fail \n", audio_element_get_tag(self));
            return AEL_PROCESS_FAIL;
        }
        mp3_dec->skip_idtag_done = true;
        BK_LOGV(TAG, "[%s] skip_idtag completed, offset: %d \n", audio_element_get_tag(self), skip_ret);
    }

    /* set read data timeout */
    audio_element_set_input_timeout(self, 20 / portTICK_RATE_MS);

__retry:
    if (mp3_dec->main_buff_remain_size < mp3_dec->main_buff_size)
    {
        if (mp3_dec->main_buff_remain_size > 0)
        {
            os_memmove(mp3_dec->main_buff, mp3_dec->main_buff_readptr, mp3_dec->main_buff_remain_size);
        }
        mp3_dec->main_buff_readptr = mp3_dec->main_buff;
        r_size = audio_element_input(self, (char *)(mp3_dec->main_buff + mp3_dec->main_buff_remain_size), mp3_dec->main_buff_size - mp3_dec->main_buff_remain_size);
        /* Check r_size value */
        if (r_size >= 0)
        {
            mp3_dec->main_buff_remain_size = mp3_dec->main_buff_remain_size + r_size;
            //mp3_dec->main_buff_readptr = mp3_dec->main_buff;
            if(is_aud_dump_valid(DUMP_TYPE_DEC_IN_DATA))
            {
                /*update header*/
                DEBUG_DATA_DUMP_UPDATE_HEADER_DUMP_FILE_TYPE(DUMP_TYPE_DEC_IN_DATA, 0, DUMP_FILE_TYPE_MP3);
                DEBUG_DATA_DUMP_UPDATE_HEADER_DATA_FLOW_LEN(DUMP_TYPE_DEC_IN_DATA, 0, r_size);
                DEBUG_DATA_DUMP_UPDATE_HEADER_TIMESTAMP(DUMP_TYPE_DEC_IN_DATA);

                /*dump data function is called by multi-thread,need suspend task scheduler until data dump finished*/
                DEBUG_DATA_DUMP_SUSPEND_ALL;

                /*dump header*/
                DEBUG_DATA_DUMP_BY_UART_HEADER(DUMP_TYPE_DEC_IN_DATA);

                /*dump data*/
                DEBUG_DATA_DUMP_BY_UART_DATA((mp3_dec->main_buff + mp3_dec->main_buff_remain_size), r_size);
                DEBUG_DATA_DUMP_RESUME_ALL;

                /*update seq*/
                DEBUG_DATA_DUMP_UPDATE_HEADER_SEQ_NUM(DUMP_TYPE_DEC_IN_DATA);
            }
        }
        else
        {
            if (r_size == AEL_IO_TIMEOUT)
            {
                BK_LOGE(TAG, "[%s] read mp3 data timeout, retry, r_size: %d \n", audio_element_get_tag(self), r_size);
                goto __retry;
            }
            else if (r_size == AEL_IO_DONE || r_size == AEL_IO_OK)
            {
                /* Data reading is complete. Use the remaining MP3 data for decoding. */
                //nothing todo
                BK_LOGD(TAG, "[%s] Data reading is complete. \n", audio_element_get_tag(self));
                BK_LOGD(TAG, "[%s] main_buff_remain_size: %d \n", audio_element_get_tag(self), mp3_dec->main_buff_remain_size);
            }
            else
            {
                /* stop mp3 decode */
                return r_size;
            }
        }
    }

    if (mp3_dec->main_buff_remain_size == 0)
    {
        /* the remaining data is empty, return AEL_IO_DONE and stop mp3 decoder */
        return r_size;
    }

    int offset = MP3FindSyncWord(mp3_dec->main_buff_readptr, mp3_dec->main_buff_remain_size);
    if (offset < 0)
    {
        BK_LOGD(TAG, "[%s] MP3FindSyncWord not find \n", audio_element_get_tag(self));
        mp3_dec->main_buff_remain_size = 0;
        goto __retry;
    }
    else
    {
        mp3_dec->main_buff_readptr += offset;
        mp3_dec->main_buff_remain_size -= offset;

        if (check_mp3_sync_word(mp3_dec) == -1)
        {
            if (mp3_dec->main_buff_remain_size > 0)
            {
                mp3_dec->main_buff_remain_size --;
                mp3_dec->main_buff_readptr ++;
                goto __retry;
            }
            else
            {
                BK_LOGE(TAG, "[%s] check_mp3_sync_word fail \n", audio_element_get_tag(self));
                return AEL_PROCESS_FAIL;
                /* check sync word fail, read data  */
                //goto __retry;
            }
        }

        ret = MP3Decode(mp3_dec->dec_handle, &mp3_dec->main_buff_readptr, (int *)&mp3_dec->main_buff_remain_size, mp3_dec->out_pcm_buff, 0);
        if (ret != ERR_MP3_NONE)
        {
            switch (ret)
            {
                case ERR_MP3_INDATA_UNDERFLOW:
                    BK_LOGW(TAG, "ERR_MP3_INDATA_UNDERFLOW.\n");
                    mp3_dec->main_buff_remain_size = 0;
                    goto __retry;
                    break;

                case ERR_MP3_MAINDATA_UNDERFLOW:
                    /* do nothing - next call to decode will provide more mainData */
                    BK_LOGW(TAG, "ERR_MP3_MAINDATA_UNDERFLOW.\n");
                    goto __retry;
                    break;

                default:
                    /* Fault tolerance: try resynchronization instead of failing directly */
                    BK_LOGW(TAG, "MP3Decode failed, code is %d, attempting resync\n", ret);
                    BK_LOGW(TAG, "main_buff_remain_size: %d \n", mp3_dec->main_buff_remain_size);

                    /* Find the next sync word in the remaining data */
                    int resync_offset = MP3FindSyncWord(mp3_dec->main_buff_readptr, mp3_dec->main_buff_remain_size);
                    if (resync_offset >= 0)
                    {
                        /* Found the sync word, skip the bad frame and continue decoding */
                        mp3_dec->main_buff_readptr += resync_offset;
                        mp3_dec->main_buff_remain_size -= resync_offset;
                        BK_LOGW(TAG, "Resync successful: offset=%d, remain=%d\n", resync_offset, mp3_dec->main_buff_remain_size);
                        goto __retry;
                    }
                    else
                    {
                        /* Not found the sync word, clear the buffer and re-read the data */
                        BK_LOGW(TAG, "Resync failed, requesting more data\n");
                        mp3_dec->main_buff_remain_size = 0;
                        goto __retry;
                    }
                    break;
            }
        }
        else
        {
            MP3GetLastFrameInfo(mp3_dec->dec_handle, &mp3_dec->frame_info);
            BK_LOGV(TAG, "bitsPerSample: %d, Samprate: %d\r\n", mp3_dec->frame_info.bitsPerSample, mp3_dec->frame_info.samprate);
            BK_LOGV(TAG, "Channel: %d, Version: %d, Layer: %d\r\n", mp3_dec->frame_info.nChans, mp3_dec->frame_info.version, mp3_dec->frame_info.layer);
            BK_LOGV(TAG, "OutputSamps: %d\r\n", mp3_dec->frame_info.outputSamps);
            r_size = mp3_dec->frame_info.outputSamps * mp3_dec->frame_info.bitsPerSample / 8;
            BK_LOGV(TAG, "MP3Decode complete, r_size: %d \n", r_size);
        }
    }

    ret = music_info_report(self);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "music_info_report \n");
        return 0;
    }

    int w_size = 0;
    if (r_size > 0)
    {
        w_size = audio_element_output(self, (char *)mp3_dec->out_pcm_buff, r_size);
        MP3_DEC_DATA_DUMP_BY_UART_DATA(mp3_dec->out_pcm_buff, r_size);
        if(is_aud_dump_valid(DUMP_TYPE_DEC_OUT_DATA))
        {
            /*update header*/
            DEBUG_DATA_DUMP_UPDATE_HEADER_DUMP_FILE_TYPE(DUMP_TYPE_DEC_OUT_DATA, 0, DUMP_FILE_TYPE_PCM);
            DEBUG_DATA_DUMP_UPDATE_HEADER_DATA_FLOW_LEN(DUMP_TYPE_DEC_OUT_DATA, 0, r_size);
            DEBUG_DATA_DUMP_UPDATE_HEADER_TIMESTAMP(DUMP_TYPE_DEC_OUT_DATA);

            /*dump data function is called by multi-thread,need suspend task scheduler until data dump finished*/
            DEBUG_DATA_DUMP_SUSPEND_ALL;

            /*dump header*/
            DEBUG_DATA_DUMP_BY_UART_HEADER(DUMP_TYPE_DEC_OUT_DATA);

            /*dump data*/
            DEBUG_DATA_DUMP_BY_UART_DATA(mp3_dec->out_pcm_buff, r_size);
            DEBUG_DATA_DUMP_RESUME_ALL;

            /*update seq*/
            DEBUG_DATA_DUMP_UPDATE_HEADER_SEQ_NUM(DUMP_TYPE_DEC_OUT_DATA);
        }
    }
    else
    {
        w_size = r_size;
    }

    return w_size;
}

static bk_err_t _mp3_decoder_destroy(audio_element_handle_t self)
{
    mp3_decoder_t *mp3_dec = (mp3_decoder_t *)audio_element_getdata(self);

    if (mp3_dec->main_buff)
    {
        audio_free(mp3_dec->main_buff);
        mp3_dec->main_buff = NULL;
    }
    if (mp3_dec->out_pcm_buff)
    {
        audio_free(mp3_dec->out_pcm_buff);
        mp3_dec->out_pcm_buff = NULL;
    }
    if (mp3_dec->dec_handle)
    {
        MP3FreeDecoder(mp3_dec->dec_handle);
        mp3_dec->dec_handle = NULL;
    }
    audio_free(mp3_dec);

    MP3_DEC_DATA_DUMP_BY_UART_CLOSE();

    return BK_OK;
}

audio_element_handle_t mp3_decoder_init(mp3_decoder_cfg_t *config)
{
    audio_element_handle_t el;
    mp3_decoder_t *mp3_dec = audio_calloc(1, sizeof(mp3_decoder_t));
    AUDIO_MEM_CHECK(TAG, mp3_dec, return NULL);
    os_memset(mp3_dec, 0, sizeof(mp3_decoder_t));

    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.open = _mp3_decoder_open;
    cfg.close = _mp3_decoder_close;
    cfg.seek = NULL;
    cfg.process = _mp3_decoder_process;
    cfg.destroy = _mp3_decoder_destroy;
    cfg.read = NULL;
    cfg.write = NULL;
    cfg.in_type = PORT_TYPE_RB;
    cfg.out_type = PORT_TYPE_RB;
    cfg.task_stack = config->task_stack;
    cfg.task_prio = config->task_prio;
    cfg.task_core = config->task_core;
    cfg.out_block_size = config->out_block_size;
    cfg.out_block_num = config->out_block_num;
    cfg.buffer_len = config->main_buff_size;
    cfg.tag = "mp3_decoder";

    mp3_dec->main_buff_size = config->main_buff_size;
    mp3_dec->out_pcm_buff_size = config->out_pcm_buff_size;
    mp3_dec->dec_handle = MP3InitDecoder();
    AUDIO_MEM_CHECK(TAG, mp3_dec->dec_handle, goto _mp3_decoder_init_exit);
    mp3_dec->main_buff = (uint8_t *)audio_malloc(mp3_dec->main_buff_size);
    AUDIO_MEM_CHECK(TAG, mp3_dec->main_buff, goto _mp3_decoder_init_exit);
    mp3_dec->out_pcm_buff = (int16_t *)audio_malloc(mp3_dec->out_pcm_buff_size);
    AUDIO_MEM_CHECK(TAG, mp3_dec->out_pcm_buff, goto _mp3_decoder_init_exit);

    el = audio_element_init(&cfg);

    AUDIO_MEM_CHECK(TAG, el, goto _mp3_decoder_init_exit);
    audio_element_setdata(el, mp3_dec);

    audio_element_info_t info = {0};
    audio_element_getinfo(el, &info);
    info.sample_rates = 0;
    info.channels = 0;
    info.bits = 0;
    info.codec_fmt = BK_CODEC_TYPE_MP3;
    audio_element_setinfo(el, &info);

    MP3_DEC_DATA_DUMP_BY_UART_OPEN();

    return el;
_mp3_decoder_init_exit:
    if (mp3_dec->main_buff)
    {
        audio_free(mp3_dec->main_buff);
        mp3_dec->main_buff = NULL;
    }
    if (mp3_dec->out_pcm_buff)
    {
        audio_free(mp3_dec->out_pcm_buff);
        mp3_dec->out_pcm_buff = NULL;
    }
    if (mp3_dec->dec_handle)
    {
        MP3FreeDecoder(mp3_dec->dec_handle);
        mp3_dec->dec_handle = NULL;
    }
    audio_free(mp3_dec);
    mp3_dec = NULL;
    return NULL;
}


