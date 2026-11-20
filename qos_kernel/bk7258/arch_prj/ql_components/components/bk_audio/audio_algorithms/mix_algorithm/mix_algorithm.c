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
#include <components/bk_audio/audio_algorithms/mix_algorithm.h>
#include <components/bk_audio/audio_pipeline/audio_types.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_error.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>

#define TAG  "MIX_ALGORITHM"

/* dump mix_algorithm stream output data by uart */
//#define MIX_ALGORITHM_DATA_DUMP_BY_UART

#ifdef MIX_ALGORITHM_DATA_DUMP_BY_UART
#include <components/bk_audio/audio_utils/uart_util.h>
static struct uart_util gl_mix_algorithm_uart_util = {0};
#define MIX_ALGORITHM_DATA_DUMP_UART_ID            (1)
#define MIX_ALGORITHM_DATA_DUMP_UART_BAUD_RATE     (2000000)

#define MIX_ALGORITHM_DATA_DUMP_BY_UART_OPEN()                    uart_util_create(&gl_mix_algorithm_uart_util, MIX_ALGORITHM_DATA_DUMP_UART_ID, MIX_ALGORITHM_DATA_DUMP_UART_BAUD_RATE)
#define MIX_ALGORITHM_DATA_DUMP_BY_UART_CLOSE()                   uart_util_destroy(&gl_mix_algorithm_uart_util)
#define MIX_ALGORITHM_DATA_DUMP_BY_UART_DATA(data_buf, len)       uart_util_tx_data(&gl_mix_algorithm_uart_util, data_buf, len)

#else

#define MIX_ALGORITHM_DATA_DUMP_BY_UART_OPEN()
#define MIX_ALGORITHM_DATA_DUMP_BY_UART_CLOSE()
#define MIX_ALGORITHM_DATA_DUMP_BY_UART_DATA(data_buf, len)

#endif  //MIX_ALGORITHM_DATA_DUMP_BY_UART



typedef struct mix_algorithm
{
    int                     input_channel_num; /*!< Number of input channels */
} mix_algorithm_t;



static bk_err_t _mix_algorithm_open(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] _mix_algorithm_open \n", audio_element_get_tag(self));

    /* set read data timeout */
    audio_element_set_input_timeout(self, 20 / portTICK_RATE_MS);   // 2000, 15 / portTICK_RATE_MS

    return BK_OK;
}

static bk_err_t _mix_algorithm_close(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] _mix_algorithm_close \n", audio_element_get_tag(self));
    return BK_OK;
}

static int _mix_algorithm_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    BK_LOGV(TAG, "[%s] _mix_algorithm_process \n", audio_element_get_tag(self));
    mix_algorithm_t *mix_alg = (mix_algorithm_t *)audio_element_getdata(self);

    int r_size = audio_element_input(self, in_buffer, in_len);
    //BK_LOGD(TAG, "[%s] r_size: %d \n", audio_element_get_tag(self), r_size);

    int w_size = 0;
    if (r_size > 0)
    {
        // The input data is in a fixed 16-bit wide PCM format
        int samples_per_channel = r_size / (2 * mix_alg->input_channel_num);

        // Additive mixing processing
        int16_t *input_samples = (int16_t *)in_buffer;
        for (int i = 0; i < samples_per_channel; i++)
        {
            int32_t sum = 0;
            for (int ch = 0; ch < mix_alg->input_channel_num; ch++)
            {
                sum += (int32_t)input_samples[i * mix_alg->input_channel_num + ch];
            }

            input_samples[i] = (int16_t)(sum / mix_alg->input_channel_num);
        }

        MIX_ALGORITHM_DATA_DUMP_BY_UART_DATA((char *)input_samples, samples_per_channel * 2);

        w_size = audio_element_output(self, (char *)input_samples, samples_per_channel * 2);
    }
    else
    {
        w_size = r_size;
    }

    return w_size;
}

static bk_err_t _mix_algorithm_destroy(audio_element_handle_t self)
{
    mix_algorithm_t *mix_alg = (mix_algorithm_t *)audio_element_getdata(self);
    audio_free(mix_alg);

    MIX_ALGORITHM_DATA_DUMP_BY_UART_CLOSE();

    return BK_OK;
}


audio_element_handle_t mix_algorithm_init(mix_algorithm_cfg_t *config)
{
    audio_element_handle_t el;
    mix_algorithm_t *mix_alg = audio_calloc(1, sizeof(mix_algorithm_t));

    AUDIO_MEM_CHECK(TAG, mix_alg, return NULL);

    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.open = _mix_algorithm_open;
    cfg.close = _mix_algorithm_close;
    cfg.seek = NULL;
    cfg.process = _mix_algorithm_process;
    cfg.destroy = _mix_algorithm_destroy;
    cfg.in_type = PORT_TYPE_RB;
    cfg.read = NULL;
    cfg.out_type = PORT_TYPE_RB;
    cfg.write = NULL;
    cfg.task_stack = config->task_stack;
    cfg.task_prio = config->task_prio;
    cfg.task_core = config->task_core;
    cfg.out_block_size = config->out_block_size;
    cfg.out_block_num = config->out_block_num;
    cfg.buffer_len = config->buf_sz;

    cfg.tag = "mix_algorithm";
    mix_alg->input_channel_num = config->input_channel_num;

    el = audio_element_init(&cfg);

    AUDIO_MEM_CHECK(TAG, el, goto _mix_algorithm_init_exit);
    audio_element_setdata(el, mix_alg);

    MIX_ALGORITHM_DATA_DUMP_BY_UART_OPEN();

    return el;
_mix_algorithm_init_exit:
    audio_free(mix_alg);
    return NULL;
}
