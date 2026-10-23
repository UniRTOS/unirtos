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
#include <components/bk_audio/audio_encoders/sbc_enc.h>
#include <components/bk_audio/audio_pipeline/audio_types.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_error.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <modules/sbc_encoder.h>
#include <modules/sbc_encoder_types.h>


#define TAG  "SBC_ENC"

/* dump sbc_encoder stream output sbc data by uart */
//#define SBC_ENC_DATA_DUMP_BY_UART

#ifdef SBC_ENC_DATA_DUMP_BY_UART
#include <components/bk_audio/audio_utils/uart_util.h>
static struct uart_util gl_sbc_enc_uart_util = {0};
#define SBC_ENC_DATA_DUMP_UART_ID            (1)
#define SBC_ENC_DATA_DUMP_UART_BAUD_RATE     (2000000)

#define SBC_ENC_DATA_DUMP_BY_UART_OPEN()                    uart_util_create(&gl_sbc_enc_uart_util, SBC_ENC_DATA_DUMP_UART_ID, SBC_ENC_DATA_DUMP_UART_BAUD_RATE)
#define SBC_ENC_DATA_DUMP_BY_UART_CLOSE()                   uart_util_destroy(&gl_sbc_enc_uart_util)
#define SBC_ENC_DATA_DUMP_BY_UART_DATA(data_buf, len)       uart_util_tx_data(&gl_sbc_enc_uart_util, data_buf, len)

#else

#define SBC_ENC_DATA_DUMP_BY_UART_OPEN()
#define SBC_ENC_DATA_DUMP_BY_UART_CLOSE()
#define SBC_ENC_DATA_DUMP_BY_UART_DATA(data_buf, len)

#endif  //SBC_ENC_DATA_DUMP_BY_UART


typedef struct sbc_encoder
{
    SbcEncoderContext     enc_context;      /*!< SBC encoder context */
    int                              sample_rate;         /*!< Sample rate (16000, 32000, 44100, 48000) */
    int                              channels;            /*!< Number of channels (1 for mono, 2 for stereo) */
    sbc_encoder_bitpool_t            bitpool;             /*!< Bitpool value (2-250) */
    sbc_encoder_channel_mode_t       channel_mode;        /*!< Channel mode */
    sbc_encoder_subbands_t           subbands;            /*!< Subbands (4 or 8) */
    sbc_encoder_blocks_t             blocks;              /*!< Blocks (4, 8, 12, or 16) */
    sbc_encoder_allocation_method_t  allocation_method;   /*!< Allocation method (loudness or SNR) */
    bool                             msbc_mode;           /*!< Enable mSBC mode */
} sbc_encoder_t;



static bk_err_t _sbc_encoder_open(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] _sbc_encoder_open \n", audio_element_get_tag(self));
    sbc_encoder_t *sbc_enc = (sbc_encoder_t *)audio_element_getdata(self);

    // Initialize SBC encoder
    int32_t result = sbc_encoder_init(&sbc_enc->enc_context, sbc_enc->sample_rate, sbc_enc->channels);
    if (result != SBC_ENCODER_ERROR_OK) {
        BK_LOGE(TAG, "SBC encoder initialization failed with error code: %d\n", result);
        return BK_FAIL;
    }

    // Configure SBC encoder parameters
    if (sbc_enc->msbc_mode) {
        // Set mSBC mode
        sbc_encoder_ctrl(&sbc_enc->enc_context, SBC_ENCODER_CTRL_CMD_SET_MSBC_ENCODE_MODE, 0);
    } else {
        // Set regular SBC parameters
        // Set allocation method
        sbc_encoder_ctrl(&sbc_enc->enc_context, SBC_ENCODER_CTRL_CMD_SET_ALLOCATION_METHOD, sbc_enc->allocation_method);

        // Set bitpool
        sbc_encoder_ctrl(&sbc_enc->enc_context, SBC_ENCODER_CTRL_CMD_SET_BITPOOL, sbc_enc->bitpool);

        // Set block mode
        sbc_encoder_ctrl(&sbc_enc->enc_context, SBC_ENCODER_CTRL_CMD_SET_BLOCK_MODE, sbc_enc->blocks);

        // Set channel mode
        sbc_encoder_ctrl(&sbc_enc->enc_context, SBC_ENCODER_CTRL_CMD_SET_CHANNEL_MODE, sbc_enc->channel_mode);

        // Set subband mode
        sbc_encoder_ctrl(&sbc_enc->enc_context, SBC_ENCODER_CTRL_CMD_SET_SUBBAND_MODE, sbc_enc->subbands);

        // Set sample rate index
        int sample_rate_index = 0;
        switch (sbc_enc->sample_rate) {
            case 16000:
                sample_rate_index = 0;
                break;
            case 32000:
                sample_rate_index = 1;
                break;
            case 44100:
                sample_rate_index = 2;
                break;
            case 48000:
                sample_rate_index = 3;
                break;
            default:
                sample_rate_index = 2; // Default to 44.1kHz
                break;
        }
        sbc_encoder_ctrl(&sbc_enc->enc_context, SBC_ENCODER_CTRL_CMD_SET_SAMPLE_RATE_INDEX, sample_rate_index);
    }

    return BK_OK;
}

static bk_err_t _sbc_encoder_close(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] _sbc_encoder_close \n", audio_element_get_tag(self));
    // SBC encoder doesn't require explicit release

    return BK_OK;
}

static int _sbc_encoder_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    BK_LOGV(TAG, "[%s] _sbc_encoder_process \n", audio_element_get_tag(self));
    sbc_encoder_t *sbc_enc = (sbc_encoder_t *)audio_element_getdata(self);

    int r_size = audio_element_input(self, in_buffer, in_len);

    int w_size = 0;
    if (r_size > 0)
    {
        // Calculate number of frames based on PCM length
        int frame_size = sbc_enc->enc_context.pcm_length * sizeof(int16_t);
        int num_frames = r_size / frame_size;

        if (num_frames > 0) {
            unsigned char *sbc_out_ptr = audio_malloc(BT_SBC_MAX_FRAME_SIZE * num_frames);
            AUDIO_MEM_CHECK(TAG, sbc_out_ptr, return -1);

            int total_encoded_len = 0;
            for (int i = 0; i < num_frames; i++) {
                int16_t *pcm_frame = (int16_t *)(in_buffer + i * frame_size);

                // Encode using SBC
                int encoded_len = sbc_encoder_encode(&sbc_enc->enc_context, pcm_frame);

                if (encoded_len > 0) {
                    // Copy encoded data to output buffer
                    if (sbc_enc->msbc_mode) {
                        memcpy(sbc_out_ptr + total_encoded_len, sbc_enc->enc_context.frame_id, 2);
                        total_encoded_len += 2;
                    }
                    memcpy(sbc_out_ptr + total_encoded_len, sbc_enc->enc_context.stream, encoded_len);
                    total_encoded_len += encoded_len;
                } else {
                    BK_LOGE(TAG, "SBC encoding failed with error code: %d\n", encoded_len);
                    audio_free(sbc_out_ptr);
                    return encoded_len; // Return error code
                }
            }

            SBC_ENC_DATA_DUMP_BY_UART_DATA(sbc_out_ptr, total_encoded_len);

            w_size = audio_element_output(self, (char *)sbc_out_ptr, total_encoded_len);
            audio_free(sbc_out_ptr);
        } else {
            w_size = 0;
        }
    }
    else
    {
        w_size = r_size;
    }
    return w_size;
}

static bk_err_t _sbc_encoder_destroy(audio_element_handle_t self)
{
    sbc_encoder_t *sbc_enc = (sbc_encoder_t *)audio_element_getdata(self);
    audio_free(sbc_enc);

    SBC_ENC_DATA_DUMP_BY_UART_CLOSE();

    return BK_OK;
}


audio_element_handle_t sbc_enc_init(sbc_encoder_cfg_t *config)
{
    audio_element_handle_t el;
    sbc_encoder_t *sbc_enc = audio_calloc(1, sizeof(sbc_encoder_t));

    AUDIO_MEM_CHECK(TAG, sbc_enc, return NULL);

    // Copy only necessary configuration parameters
    sbc_enc->sample_rate = config->sample_rate;
    sbc_enc->channels = config->channels;
    sbc_enc->bitpool = config->bitpool;
    sbc_enc->channel_mode = config->channel_mode;
    sbc_enc->subbands = config->subbands;
    sbc_enc->blocks = config->blocks;
    sbc_enc->allocation_method = config->allocation_method;
    sbc_enc->msbc_mode = config->msbc_mode;

    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.open = _sbc_encoder_open;
    cfg.close = _sbc_encoder_close;
    cfg.seek = NULL;
    cfg.process = _sbc_encoder_process;
    cfg.destroy = _sbc_encoder_destroy;
    cfg.in_type = PORT_TYPE_RB;
    cfg.read = NULL;
    cfg.out_type = PORT_TYPE_FB;
    cfg.write = NULL;
    cfg.task_stack = config->task_stack;
    cfg.task_prio = config->task_prio;
    cfg.task_core = config->task_core;
    cfg.out_block_size = config->out_block_size;
    cfg.out_block_num = config->out_block_num;
    cfg.buffer_len = config->buf_sz;

    cfg.tag = "sbc_encoder";

    el = audio_element_init(&cfg);

    AUDIO_MEM_CHECK(TAG, el, goto _sbc_encoder_init_exit);
    audio_element_setdata(el, sbc_enc);

    SBC_ENC_DATA_DUMP_BY_UART_OPEN();

    return el;
_sbc_encoder_init_exit:
    audio_free(sbc_enc);
    return NULL;
}