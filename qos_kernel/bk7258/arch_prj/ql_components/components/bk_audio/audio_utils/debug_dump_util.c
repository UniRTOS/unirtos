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
#include "sdkconfig.h"
#include <os/os.h>
#include <os/mem.h>
#include <components/bk_audio/audio_utils/uart_util.h>
#include <components/bk_audio/audio_utils/debug_dump_util.h>

#if CONFIG_ADK_DEBUG_DUMP_UTIL

#define AUD_DUMP_TAG "aud_dump"

#define LOGI(...) BK_LOGI(AUD_DUMP_TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(AUD_DUMP_TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(AUD_DUMP_TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(AUD_DUMP_TAG, ##__VA_ARGS__)
#if CONFIG_ADK_DEBUG_DUMP_DATA_TYPE_EXTENSION
volatile debug_dump_data_header_t dump_header[HEADER_ARRAY_CNT] = 
{
    {
        .header_magicword_part1 = HEADER_MAGICWORD_PART1,
        .header_magicword_part2 = HEADER_MAGICWORD_PART2,
        .data_flow_num = 1,
        .data_flow[0] = 
         {
            .dump_type = DUMP_TYPE_DEC_OUT_DATA,
            .dump_file_type = DUMP_FILE_TYPE_PCM,
            .len = 0,
            .sample_rate = 16000,
            .frame_in_ms = 20,
            .ch_num = 1,
         },
        .seq_no = 0
    },
    {
        .header_magicword_part1 = HEADER_MAGICWORD_PART1,
        .header_magicword_part2 = HEADER_MAGICWORD_PART2,
        .data_flow_num = 1,
        .data_flow[0] = 
         {
            .dump_type = DUMP_TYPE_DEC_IN_DATA,
            .dump_file_type = DUMP_FILE_TYPE_G722,
            .len = 0,
            .sample_rate = 16000,
            .frame_in_ms = 20,
            .ch_num = 1,
         },
        .seq_no = 0
    },
    {
        .header_magicword_part1 = HEADER_MAGICWORD_PART1,
        .header_magicword_part2 = HEADER_MAGICWORD_PART2,
        .data_flow_num = 1,
        .data_flow[0] = 
         {
            .dump_type = DUMP_TYPE_ENC_OUT_DATA,
            .dump_file_type = DUMP_FILE_TYPE_G722,
            .len = 0,
            .sample_rate = 16000,
            .frame_in_ms = 20,
            .ch_num = 1,
         },
        .seq_no = 0
    },
    {
        .header_magicword_part1 = HEADER_MAGICWORD_PART1,
        .header_magicword_part2 = HEADER_MAGICWORD_PART2,
        .data_flow_num = 3,
        .data_flow[0] = 
         {
            .dump_type = DUMP_TYPE_AEC_MIC_DATA,
            .dump_file_type = DUMP_FILE_TYPE_PCM,
            .len = 0,
            .sample_rate = 16000,
            .frame_in_ms = 20,
            .ch_num = 1,
         },
        .data_flow[1] = 
         {
            .dump_type = DUMP_TYPE_AEC_REF_DATA,
            .dump_file_type = DUMP_FILE_TYPE_PCM,
            .len = 0,
            .sample_rate = 16000,
            .frame_in_ms = 20,
            .ch_num = 1,
         },
        .data_flow[2] = 
         {
            .dump_type = DUMP_TYPE_AEC_OUT_DATA,
            .dump_file_type = DUMP_FILE_TYPE_PCM,
            .len = 0,
            .sample_rate = 16000,
            .frame_in_ms = 20,
            .ch_num = 1,
         },
        .seq_no = 0
    },
    {
        .header_magicword_part1 = HEADER_MAGICWORD_PART1,
        .header_magicword_part2 = HEADER_MAGICWORD_PART2,
        .data_flow_num = 1,
        .data_flow[0] = 
         {
            .dump_type = DUMP_TYPE_ENC_IN_DATA,
            .dump_file_type = DUMP_FILE_TYPE_PCM,
            .len = 0,
            .sample_rate = 16000,
            .frame_in_ms = 20,
            .ch_num = 1,
         },
        .seq_no = 0
    },
    {
        .header_magicword_part1 = HEADER_MAGICWORD_PART1,
        .header_magicword_part2 = HEADER_MAGICWORD_PART2,
        .data_flow_num = 1,
        .data_flow[0] = 
         {
            .dump_type = DUMP_TYPE_EQ_IN_DATA,
            .dump_file_type = DUMP_FILE_TYPE_PCM,
            .len = 0,
            .sample_rate = 16000,
            .frame_in_ms = 20,
            .ch_num = 1,
         },
        .seq_no = 0
    },
    {
        .header_magicword_part1 = HEADER_MAGICWORD_PART1,
        .header_magicword_part2 = HEADER_MAGICWORD_PART2,
        .data_flow_num = 1,
        .data_flow[0] = 
         {
            .dump_type = DUMP_TYPE_EQ_OUT_DATA,
            .dump_file_type = DUMP_FILE_TYPE_PCM,
            .len = 0,
            .sample_rate = 16000,
            .frame_in_ms = 20,
            .ch_num = 1,
         },
        .seq_no = 0
    },
};
#else
volatile debug_dump_data_header_t dump_header[HEADER_ARRAY_CNT] = 
{
    {
        .header_magicword_part1 = HEADER_MAGICWORD_PART1,
        .header_magicword_part2 = HEADER_MAGICWORD_PART2,
        .data_flow_num = 1,
        .data_flow[0] = 
         {
            .dump_type = DUMP_TYPE_DEC_OUT_DATA,
            .dump_file_type = DUMP_FILE_TYPE_PCM,
            .len = 0,
         },
        .seq_no = 0
    },
    {
        .header_magicword_part1 = HEADER_MAGICWORD_PART1,
        .header_magicword_part2 = HEADER_MAGICWORD_PART2,
        .data_flow_num = 1,
        .data_flow[0] = 
         {
            .dump_type = DUMP_TYPE_DEC_IN_DATA,
            .dump_file_type = DUMP_FILE_TYPE_G722,
            .len = 0,
         },
        .seq_no = 0
    },
    {
        .header_magicword_part1 = HEADER_MAGICWORD_PART1,
        .header_magicword_part2 = HEADER_MAGICWORD_PART2,
        .data_flow_num = 1,
        .data_flow[0] = 
         {
            .dump_type = DUMP_TYPE_ENC_OUT_DATA,
            .dump_file_type = DUMP_FILE_TYPE_G722,
            .len = 0,
         },
        .seq_no = 0
    },
    {
        .header_magicword_part1 = HEADER_MAGICWORD_PART1,
        .header_magicword_part2 = HEADER_MAGICWORD_PART2,
        .data_flow_num = 3,
        .data_flow[0] = 
         {
            .dump_type = DUMP_TYPE_AEC_MIC_DATA,
            .dump_file_type = DUMP_FILE_TYPE_PCM,
            .len = 0,
         },
        .data_flow[1] = 
         {
            .dump_type = DUMP_TYPE_AEC_REF_DATA,
            .dump_file_type = DUMP_FILE_TYPE_PCM,
            .len = 0,
         },
        .data_flow[2] = 
         {
            .dump_type = DUMP_TYPE_AEC_OUT_DATA,
            .dump_file_type = DUMP_FILE_TYPE_PCM,
            .len = 0,
         },
        .seq_no = 0
    },
    {
        .header_magicword_part1 = HEADER_MAGICWORD_PART1,
        .header_magicword_part2 = HEADER_MAGICWORD_PART2,
        .data_flow_num = 1,
        .data_flow[0] = 
         {
            .dump_type = DUMP_TYPE_ENC_IN_DATA,
            .dump_file_type = DUMP_FILE_TYPE_PCM,
            .len = 0,
         },
        .seq_no = 0
    },
    {
        .header_magicword_part1 = HEADER_MAGICWORD_PART1,
        .header_magicword_part2 = HEADER_MAGICWORD_PART2,
        .data_flow_num = 1,
        .data_flow[0] = 
         {
            .dump_type = DUMP_TYPE_EQ_IN_DATA,
            .dump_file_type = DUMP_FILE_TYPE_PCM,
            .len = 0,
         },
        .seq_no = 0
    },
    {
        .header_magicword_part1 = HEADER_MAGICWORD_PART1,
        .header_magicword_part2 = HEADER_MAGICWORD_PART2,
        .data_flow_num = 1,
        .data_flow[0] = 
         {
            .dump_type = DUMP_TYPE_EQ_OUT_DATA,
            .dump_file_type = DUMP_FILE_TYPE_PCM,
            .len = 0,
         },
        .seq_no = 0
    },
};
#endif

const uint8_t g_dump_type2header_array_idx[DUMP_TYPE_MAX] = 
{
    DUMP_TYPE_DEC_OUT_DATA,
    DUMP_TYPE_DEC_IN_DATA,
    DUMP_TYPE_ENC_OUT_DATA,
    DUMP_TYPE_AEC_MIC_DATA,
    DUMP_TYPE_AEC_MIC_DATA,
    DUMP_TYPE_AEC_MIC_DATA,
    DUMP_TYPE_ENC_IN_DATA-2,//AEC MIC/REF/OUT DATA use same HEADER
    DUMP_TYPE_EQ_IN_DATA-2,//AEC MIC/REF/OUT DATA use same HEADER
    DUMP_TYPE_EQ_OUT_DATA-2,//AEC MIC/REF/OUT DATA use same HEADER
};

struct uart_util g_debug_data_uart_util = {0};
uint16_t g_aud_data_dump_bitmap = 0;

#endif /*CONFIG_ADK_DEBUG_DUMP_UTIL*/


