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
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include <components/bk_audio/audio_decoders/sbc_decoder.h>
#include <components/bk_audio/audio_pipeline/audio_types.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_error.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <driver/sbc.h>
#include <driver/sbc_types.h>
#include <os/os.h>


#define TAG  "SBC_DEC"

//#define SBC_DECODER_DEBUG   //GPIO debug

#ifdef SBC_DECODER_DEBUG

#define SBC_DECODER_PROCESS_START()         do { GPIO_DOWN(33); GPIO_UP(33);} while (0)
#define SBC_DECODER_PROCESS_END()           do { GPIO_DOWN(33); } while (0)

#define SBC_DECODER_INPUT_START()           do { GPIO_DOWN(34); GPIO_UP(34);} while (0)
#define SBC_DECODER_INPUT_END()             do { GPIO_DOWN(34); } while (0)

#define SBC_DECODER_OUTPUT_START()          do { GPIO_DOWN(35); GPIO_UP(35);} while (0)
#define SBC_DECODER_OUTPUT_END()            do { GPIO_DOWN(35); } while (0)

#else

#define SBC_DECODER_PROCESS_START()
#define SBC_DECODER_PROCESS_END()

#define SBC_DECODER_INPUT_START()
#define SBC_DECODER_INPUT_END()

#define SBC_DECODER_OUTPUT_START()
#define SBC_DECODER_OUTPUT_END()

#endif

/* dump sbc_decoder stream output pcm data by uart */
//#define SBC_DEC_DATA_DUMP_BY_UART

#ifdef SBC_DEC_DATA_DUMP_BY_UART
#include <components/bk_audio/audio_utils/uart_util.h>
static struct uart_util gl_sbc_dec_uart_util = {0};
#define SBC_DEC_DATA_DUMP_UART_ID            (1)
#define SBC_DEC_DATA_DUMP_UART_BAUD_RATE     (2000000)

#define SBC_DEC_DATA_DUMP_BY_UART_OPEN()                    uart_util_create(&gl_sbc_dec_uart_util, SBC_DEC_DATA_DUMP_UART_ID, SBC_DEC_DATA_DUMP_UART_BAUD_RATE)
#define SBC_DEC_DATA_DUMP_BY_UART_CLOSE()                   uart_util_destroy(&gl_sbc_dec_uart_util)
#define SBC_DEC_DATA_DUMP_BY_UART_DATA(data_buf, len)       uart_util_tx_data(&gl_sbc_dec_uart_util, data_buf, len)

#else

#define SBC_DEC_DATA_DUMP_BY_UART_OPEN()
#define SBC_DEC_DATA_DUMP_BY_UART_CLOSE()
#define SBC_DEC_DATA_DUMP_BY_UART_DATA(data_buf, len)

#endif  //SBC_ENC_DATA_DUMP_BY_UART


/**
 * @brief SBC decoder context structure
 */
typedef struct sbc_decoder
{
    sbcdecodercontext_t    sbc;            /*!< SBC decoder context */
    uint32_t               sample_rate;    /*!< Record the sample rate of the previous frame */
    uint8_t                channel_number; /*!< Record the channel number of the previous frame */
} sbc_decoder_t;

/**
 * @brief SBC decoder frame info report function
 *        Check whether the current frame information is consistent with the last reported information,
 *        and update and notify the upper layer if it is inconsistent
 */
static bk_err_t music_info_report(audio_element_handle_t self)
{
    sbc_decoder_t *sbc_dec = (sbc_decoder_t *)audio_element_getdata(self);

    /* Check the frame information. If it changes, report the new frame information. */
    if (sbc_dec->sample_rate != sbc_dec->sbc.sample_rate || sbc_dec->channel_number != sbc_dec->sbc.channel_number)
    {
        BK_LOGD(TAG, "[%s] new sbc frame info sample_rate: %d, new channel_number: %d, bits: 16 \n", audio_element_get_tag(self), sbc_dec->sbc.sample_rate, sbc_dec->sbc.channel_number);

        audio_element_info_t info = {0};
        bk_err_t ret = audio_element_getinfo(self, &info);
        if (ret != BK_OK)
        {
            BK_LOGE(TAG, "[%s] audio_element_getinfo fail \n", audio_element_get_tag(self));
            return BK_FAIL;
        }

        /* SBC decoding output is fixed at 16 bits */
        info.bits = 16;
        info.sample_rates = sbc_dec->sbc.sample_rate;
        info.channels = sbc_dec->sbc.channel_number;
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

        /* update sample rate and channel number */
        sbc_dec->sample_rate = sbc_dec->sbc.sample_rate;
        sbc_dec->channel_number = sbc_dec->sbc.channel_number;
    }

    return BK_OK;
}

static bk_err_t _sbc_decoder_open(audio_element_handle_t self)
{
    BK_LOGV(TAG, "[%s] _sbc_decoder_open \n", audio_element_get_tag(self));
    sbc_decoder_t *sbc_dec = (sbc_decoder_t *)audio_element_getdata(self);

    /* Initialize SBC decoder */
    if (BK_OK != bk_sbc_decoder_init(&sbc_dec->sbc))
    {
        BK_LOGE(TAG, "[%s] bk_sbc_decoder_init fail \n", audio_element_get_tag(self));
        return BK_FAIL;
    }

    /* set read data timeout */
    audio_element_set_input_timeout(self, 20 / portTICK_RATE_MS);

    return BK_OK;
}

static int _sbc_decoder_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    //BK_LOGV(TAG, "[%s] _sbc_decoder_process \n", audio_element_get_tag(self));
    sbc_decoder_t *sbc_dec = (sbc_decoder_t *)audio_element_getdata(self);

    SBC_DECODER_PROCESS_START();

    SBC_DECODER_INPUT_START();
    int r_size = audio_element_input(self, in_buffer, in_len);
    SBC_DECODER_INPUT_END();

    int w_size = 0;
    if (r_size > 0)
    {
        /* Decode SBC frame */
        bk_err_t ret = bk_sbc_decoder_frame_decode(&sbc_dec->sbc, (uint8_t *)in_buffer, r_size);
        /* Check if the decoding is successful. */
        if (ret >= 0)
        {
            /* Check if the consumed data length is consistent with the input data length. */
            if (ret != r_size)
            {
                BK_LOGW(TAG, "[%s] %s, %d, the frame data is not one frame, cosumed_size: %d, input_size: %d \n", audio_element_get_tag(self), __func__, __LINE__, ret, r_size);
            }

            /* Check and report audio frame information changes */
            ret = music_info_report(self);
            if (ret != BK_OK)
            {
                BK_LOGE(TAG, "music_info_report failed, ret: %d\n", ret);
            }

            /* Calculate output size: channels * pcm_length * 2 bytes per sample */
            int output_size = sbc_dec->sbc.channel_number * sbc_dec->sbc.pcm_length * 2;

            SBC_DEC_DATA_DUMP_BY_UART_DATA((char *)sbc_dec->sbc.pcm_sample, output_size);

            SBC_DECODER_OUTPUT_START();
            w_size = audio_element_output(self, (char *)sbc_dec->sbc.pcm_sample, output_size);
            SBC_DECODER_OUTPUT_END();
        }
        else
        {
            switch (ret)
            {
                case SBC_DECODER_ERROR_SYNC_INCORRECT:
                    BK_LOGE(TAG, "SBC decode fail: SBC_DECODER_ERROR_SYNC_INCORRECT, skip this frame of data\n");
                    w_size = in_len;
                    break;

                case SBC_DECODER_ERROR_STREAM_EMPTY:
                    BK_LOGE(TAG, "SBC decode fail: SBC_DECODER_ERROR_STREAM_EMPTY, skip this frame of data\n");
                    w_size = in_len;
                    break;

                case SBC_DECODER_ERROR_BITPOOL_OUT_BOUNDS:
                    BK_LOGE(TAG, "SBC decode fail: SBC_DECODER_ERROR_BITPOOL_OUT_BOUNDS, skip this frame of data\n");
                    w_size = in_len;
                    break;

                default:
                    BK_LOGE(TAG, "SBC decode failed, ret: %d\n", ret);
                    w_size = AEL_IO_FAIL;
                    break;
            }
        }
    }
    else
    {
        w_size = r_size;
    }

    SBC_DECODER_PROCESS_END();

    return w_size;
}

static bk_err_t _sbc_decoder_close(audio_element_handle_t self)
{
    BK_LOGV(TAG, "[%s] _sbc_decoder_close \n", audio_element_get_tag(self));
    sbc_decoder_t *sbc_dec = (sbc_decoder_t *)audio_element_getdata(self);
    audio_element_state_t state = audio_element_get_state(self);
    
    /* Deinitialize SBC decoder */
    bk_sbc_decoder_deinit();

    // Reset info to default values to ensure music info will be reported on next open
    // Keep info unchanged when in PAUSED state to avoid re-reporting after resume
    if (state != AEL_STATE_PAUSED)
    {
        audio_element_info_t info = {0};
        bk_err_t ret = audio_element_getinfo(self, &info);
        if (ret == BK_OK)
        {
            info.sample_rates = 0;
            info.channels = 0;
            info.bits = 0;
            audio_element_setinfo(self, &info);
        }
        /* Initialize default sbc frame info, the value will be updated in the decoding process */
        sbc_dec->sample_rate = 0;
        sbc_dec->channel_number = 0;
        BK_LOGV(TAG, "[%s] Component in state %d, reset info \n", audio_element_get_tag(self), state);
    }

    return BK_OK;
}

static bk_err_t _sbc_decoder_destroy(audio_element_handle_t self)
{
    sbc_decoder_t *sbc_dec = (sbc_decoder_t *)audio_element_getdata(self);
    audio_free(sbc_dec);

    SBC_DEC_DATA_DUMP_BY_UART_CLOSE();

    return BK_OK;
}

audio_element_handle_t sbc_decoder_init(sbc_decoder_cfg_t *config)
{
    audio_element_handle_t el;
    sbc_decoder_t *sbc_dec = audio_calloc(1, sizeof(sbc_decoder_t));

    AUDIO_MEM_CHECK(TAG, sbc_dec, return NULL);

    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.open = _sbc_decoder_open;
    cfg.close = _sbc_decoder_close;
    cfg.seek = NULL;
    cfg.process = _sbc_decoder_process;
    cfg.destroy = _sbc_decoder_destroy;
    cfg.in_type = PORT_TYPE_FB;
    cfg.read = NULL;
    cfg.out_type = PORT_TYPE_RB;
    cfg.write = NULL;
    cfg.task_stack = config->task_stack;
    cfg.task_prio = config->task_prio;
    cfg.task_core = config->task_core;
    cfg.out_block_size = config->out_block_size;
    cfg.out_block_num = config->out_block_num;
    cfg.buffer_len = config->buf_sz;

    cfg.tag = "sbc_decoder";

    el = audio_element_init(&cfg);

    AUDIO_MEM_CHECK(TAG, el, goto _sbc_decoder_init_exit);
    audio_element_setdata(el, sbc_dec);

    /* Set the initial SBC audio frame information */
    audio_element_info_t info = {0};
    audio_element_getinfo(el, &info);
    info.sample_rates = 0;
    info.channels = 0;
    info.bits = 0;
    info.codec_fmt = BK_CODEC_TYPE_SBC;
    audio_element_setinfo(el, &info);

    SBC_DEC_DATA_DUMP_BY_UART_OPEN();

    return el;
_sbc_decoder_init_exit:
    audio_free(sbc_dec);
    return NULL;
}
