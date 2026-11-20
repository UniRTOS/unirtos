// Copyright 2024-2025 Beken
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

/* This file is used to debug uac work status by collecting statistics on the uac mic and speaker. */
#include <common/bk_include.h>
#include <common/bk_err.h>
#include <os/os.h>
#include <os/mem.h>
#include "cli.h"
#include <components/bk_audio/audio_utils/debug_dump_util.h>

#if CONFIG_ADK_DEBUG_DUMP_UTIL
#define AUD_DUMP_CLI_TAG "aud_cli"

#define LOGI(...) BK_LOGI(AUD_DUMP_CLI_TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(AUD_DUMP_CLI_TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(AUD_DUMP_CLI_TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(AUD_DUMP_CLI_TAG, ##__VA_ARGS__)

void aud_dump_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint16_t dump_bitmap_pre = get_aud_dump_bitmap();
    uint16_t dump_bitmap;
    if (argc < 2)
    {
        goto cmd_fail;
    }

    /* audio test */
    if(2 == argc)
    {
        if (os_strcmp(argv[1], "stop") == 0)
        {
            clr_aud_dump_bitmap();
            LOGI("dump stop\n!");
        }
        else
        {
            goto cmd_fail;
        }
    }
    else if(3 == argc)
    {
        if (os_strcmp(argv[1], "enc_out") == 0)
        {
            if (os_strtoul(argv[2], NULL, 10))
            {
                set_aud_dump_bitmap_bit(DUMP_TYPE_ENC_OUT_DATA);
                LOGI("dump aud enc out data(tx_mic)\n!");
            }
            else
            {
                clr_aud_dump_bitmap_bit(DUMP_TYPE_ENC_OUT_DATA);
            }
        }
        else if (os_strcmp(argv[1], "dec_in") == 0)
        {
            if (os_strtoul(argv[2], NULL, 10))
            {
                set_aud_dump_bitmap_bit(DUMP_TYPE_DEC_IN_DATA);
                LOGI("dump aud dec in data(rx spk)\n!");
            }
            else
            {
                clr_aud_dump_bitmap_bit(DUMP_TYPE_DEC_IN_DATA);
            }
        }
        else if (os_strcmp(argv[1], "enc_in") == 0)
        {
            if (os_strtoul(argv[2], NULL, 10))
            {
                set_aud_dump_bitmap_bit(DUMP_TYPE_ENC_IN_DATA);
                LOGI("dump aud enc in data\n!");
            }
            else
            {
                clr_aud_dump_bitmap_bit(DUMP_TYPE_ENC_IN_DATA);
            }
        }
        else if (os_strcmp(argv[1], "dec_out") == 0)
        {
            if (os_strtoul(argv[2], NULL, 10))
            {
                set_aud_dump_bitmap_bit(DUMP_TYPE_DEC_OUT_DATA);
                LOGI("dump aud dec_out data\n!");
            }
            else
            {
                clr_aud_dump_bitmap_bit(DUMP_TYPE_DEC_OUT_DATA);
            }
        }
        else if (os_strcmp(argv[1], "aec_all") == 0)
        {
            if (os_strtoul(argv[2], NULL, 10))
            {
                set_aud_dump_bitmap_bit(DUMP_TYPE_AEC_MIC_DATA);
                LOGI("dump aud aec all data\n!");
            }
            else
            {
                clr_aud_dump_bitmap_bit(DUMP_TYPE_AEC_MIC_DATA);
            }
        }
        else if (os_strcmp(argv[1], "eq_in") == 0)
        {
            if (os_strtoul(argv[2], NULL, 10))
            {
                set_aud_dump_bitmap_bit(DUMP_TYPE_EQ_IN_DATA);
                LOGI("dump aud eq in data\n!");
            }
            else
            {
                clr_aud_dump_bitmap_bit(DUMP_TYPE_EQ_IN_DATA);
            }
        }
        else if (os_strcmp(argv[1], "eq_out") == 0)
        {
            if (os_strtoul(argv[2], NULL, 10))
            {
                set_aud_dump_bitmap_bit(DUMP_TYPE_EQ_OUT_DATA);
                LOGI("dump aud eq_out data\n!");
            }
            else
            {
                clr_aud_dump_bitmap_bit(DUMP_TYPE_EQ_OUT_DATA);
            }
        }
        else
        {
            goto cmd_fail;
        }
    }
    else
    {
        goto cmd_fail;
    }

    dump_bitmap = get_aud_dump_bitmap();
    LOGI("pre dump_bitmap:0x%x cur:0x%x \n!",dump_bitmap_pre,dump_bitmap);
    if((!dump_bitmap_pre) && (dump_bitmap))
    {
        LOGI("open dump uart\n!");
        DEBUG_DATA_DUMP_BY_UART_OPEN();
    }

    if((dump_bitmap_pre) && (!dump_bitmap))
    {
        LOGI("close dump uart\n!");
        DEBUG_DATA_DUMP_BY_UART_CLOSE();
    }

    return;

cmd_fail:
    LOGE("cmd fail:audio_dump {enc_out|dec_in|enc_in|dec_out|aec_all|eq_in|eq_out|stop [value]}\n");
}

static const struct cli_command s_aud_dump_commands[] =
{
    {"audio_dump", "audio_dump ...", aud_dump_cmd},
};

#define AUD_ENGINE_CMD_CNT   (sizeof(s_aud_dump_commands) / sizeof(struct cli_command))

int aud_dump_cli_init(void)
{
    return cli_register_commands(s_aud_dump_commands, AUD_ENGINE_CMD_CNT);
}
#endif /*CONFIG_ADK_DEBUG_DUMP_UTIL*/


