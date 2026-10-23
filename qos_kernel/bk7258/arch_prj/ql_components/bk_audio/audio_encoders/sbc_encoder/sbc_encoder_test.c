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


#include "FreeRTOS.h"
#include "task.h"
#include <components/bk_audio/audio_pipeline/audio_pipeline.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_encoders/sbc_enc.h>
#include <components/bk_audio/audio_streams/array_stream.h>
#include <components/bk_audio/audio_streams/uart_stream.h>
#include <os/os.h>
#include "test_sbc_enc_array.h"

#if (CONFIG_ADK_SBC_DECODER)
#include <components/bk_audio/audio_decoders/sbc_decoder.h>
#endif

#define TAG "sbc_encoder_test"

#define TEST_CHECK_NULL(ptr) ({                                    \
    if (!(ptr)) {                                                  \
        BK_LOGE(TAG, "NULL pointer, %d \n", __LINE__);             \
        return BK_FAIL;                                            \
    }                                                              \
})

/* The "sbc-encoder" element is neither a producer nor a consumer when test element
   is neither first element nor last element of the pipeline. Usually this element has
   both src and sink. The data flow model of this element is as follow:
   +--------------+               +--------------+               +--------------+
   |     array    |               |     sbc      |               |     uart     |
   |    stream    |               |   encoder    |               |    stream    |
   |             src - ringbuf - sink           src - ringbuf - sink            |
   |              |               |              |               |              |
   +--------------+               +--------------+               +--------------+

   Function: Use sbc encoder to encode fixed audio data in array.

   The "sbc-encoder" element read audio data from array through array stream, encode the data to sbc
   format and write the data to uart through uart stream.
*/
bk_err_t adk_sbc_encoder_test_case_0(void)
{
    audio_pipeline_handle_t pipeline;
    audio_element_handle_t sbc_enc, array_stream, uart_stream;

    BK_LOGD(TAG, "--------- %s ----------\n", __func__);
    AUDIO_MEM_SHOW("start \n");

    BK_LOGD(TAG, "--------- step1: pipeline init ----------\n");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
    TEST_CHECK_NULL(pipeline);

    BK_LOGD(TAG, "--------- step2: init elements ----------\n");
    array_stream_cfg_t array_cfg = DEFAULT_ARRAY_STREAM_CONFIG();
    array_cfg.type = AUDIO_STREAM_READER;
    array_stream = array_stream_init(&array_cfg);
    TEST_CHECK_NULL(array_stream);
    array_stream_set_data(array_stream, (uint8_t *)network_provision_16k_mono_16bit_en, sizeof(network_provision_16k_mono_16bit_en));

    sbc_encoder_cfg_t sbc_encoder_cfg = DEFAULT_SBC_ENCODER_CONFIG();
    sbc_enc = sbc_enc_init(&sbc_encoder_cfg);
    TEST_CHECK_NULL(sbc_enc);

    uart_stream_cfg_t uart_cfg = DEFAULT_UART_STREAM_CONFIG();
    uart_cfg.type = AUDIO_STREAM_WRITER;
    uart_stream = uart_stream_init(&uart_cfg);
    TEST_CHECK_NULL(uart_stream);

    BK_LOGD(TAG, "--------- step3: pipeline register ----------\n");
    if (BK_OK != audio_pipeline_register(pipeline, array_stream, "array_stream"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_register(pipeline, sbc_enc, "sbc_enc"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_register(pipeline, uart_stream, "uart_stream"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step4: pipeline link ----------\n");
    if (BK_OK != audio_pipeline_link(pipeline, (const char *[]){"array_stream", "sbc_enc", "uart_stream"}, 3))
    {
        BK_LOGE(TAG, "pipeline link fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step5: init event listener ----------\n");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);

    if (BK_OK != audio_pipeline_set_listener(pipeline, evt))
    {
        BK_LOGE(TAG, "set listener fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step6: pipeline run ----------\n");
    if (BK_OK != audio_pipeline_run(pipeline))
    {
        BK_LOGE(TAG, "pipeline run fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    while (1)
    {
        audio_event_iface_msg_t msg;
        bk_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
        if (ret != BK_OK)
        {
            BK_LOGE(TAG, "[ * ] Event interface error : %d \n", ret);
            continue;
        }

        if (msg.cmd == AEL_MSG_CMD_REPORT_STATUS
            && msg.source == uart_stream
            && (((int)msg.data == AEL_STATUS_STATE_STOPPED) || ((int)msg.data == AEL_STATUS_STATE_FINISHED)))
        {
            BK_LOGW(TAG, "[ * ] Stop event received \n");
            break;
        }
    }

    BK_LOGD(TAG, "--------- step7: deinit pipeline ----------\n");
    if (BK_OK != audio_pipeline_terminate(pipeline))
    {
        BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_unregister(pipeline, array_stream))
    {
        BK_LOGE(TAG, "pipeline unregister element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_unregister(pipeline, sbc_enc))
    {
        BK_LOGE(TAG, "pipeline unregister element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_unregister(pipeline, uart_stream))
    {
        BK_LOGE(TAG, "pipeline unregister element fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_remove_listener(pipeline))
    {
        BK_LOGE(TAG, "pipeline remove listener fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_event_iface_destroy(evt))
    {
        BK_LOGE(TAG, "event iface destroy fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_deinit(pipeline))
    {
        BK_LOGE(TAG, "pipeline deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }


    if (BK_OK != audio_element_deinit(array_stream))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_element_deinit(sbc_enc))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_element_deinit(uart_stream))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- sbc encoder test complete ----------\n");

    AUDIO_MEM_SHOW("end \n");

    return BK_OK;
}

#if (CONFIG_ADK_SBC_DECODER)
/* The "sbc-encoder" and "sbc-decoder" elements are neither a producer nor a consumer when test element
   is neither first element nor last element of the pipeline. Usually this element has
   both src and sink. The data flow model of this element is as follow:
   +--------------+                +--------------+               +--------------+               +--------------+
   |     array    |                |     sbc      |               |     sbc      |               |     uart     |
   |    stream    |                |   encoder    |               |   decoder    |               |    stream    |
   |             src - ringbuf - sink            src - ringbuf - sink           src - ringbuf - sink            |
   |              |                |              |               |              |               |              |
   +--------------+                +--------------+               +--------------+               +--------------+

   Function: Use sbc encoder to encode fixed audio data in array, then use sbc decoder to decode it, and finally output to uart.

   The "array-stream" element read audio data from array. The "sbc-encoder" element read audio data from ringbuffer,
   encode the data to sbc format and write the data to ringbuffer. The "sbc-decoder" element read sbc data from ringbuffer,
   decode the data to pcm format and write the data to ringbuffer. The "uart-stream" element read pcm data from ringbuffer,
   and write it to uart.
*/
bk_err_t adk_sbc_encoder_test_case_1(void)
{
    audio_pipeline_handle_t pipeline;
    audio_element_handle_t sbc_enc, sbc_dec, array_stream, uart_stream;

    BK_LOGD(TAG, "--------- %s ----------\n", __func__);
    AUDIO_MEM_SHOW("start \n");

    BK_LOGD(TAG, "--------- step1: pipeline init ----------\n");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
    TEST_CHECK_NULL(pipeline);

    BK_LOGD(TAG, "--------- step2: init elements ----------\n");
    array_stream_cfg_t array_cfg = DEFAULT_ARRAY_STREAM_CONFIG();
    array_cfg.type = AUDIO_STREAM_READER;
    array_stream = array_stream_init(&array_cfg);
    TEST_CHECK_NULL(array_stream);
    array_stream_set_data(array_stream, (uint8_t *)network_provision_16k_mono_16bit_en, sizeof(network_provision_16k_mono_16bit_en));

    sbc_encoder_cfg_t sbc_encoder_cfg = DEFAULT_SBC_ENCODER_CONFIG();
    sbc_enc = sbc_enc_init(&sbc_encoder_cfg);
    TEST_CHECK_NULL(sbc_enc);

    sbc_decoder_cfg_t sbc_decoder_cfg = DEFAULT_SBC_DECODER_CONFIG();
    sbc_dec = sbc_decoder_init(&sbc_decoder_cfg);
    TEST_CHECK_NULL(sbc_dec);

    uart_stream_cfg_t uart_cfg = DEFAULT_UART_STREAM_CONFIG();
    uart_cfg.type = AUDIO_STREAM_WRITER;
    uart_stream = uart_stream_init(&uart_cfg);
    TEST_CHECK_NULL(uart_stream);

    BK_LOGD(TAG, "--------- step3: pipeline register ----------\n");
    if (BK_OK != audio_pipeline_register(pipeline, array_stream, "array_stream"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_register(pipeline, sbc_enc, "sbc_enc"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_register(pipeline, sbc_dec, "sbc_dec"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_register(pipeline, uart_stream, "uart_stream"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step4: pipeline link ----------\n");
    if (BK_OK != audio_pipeline_link(pipeline, (const char *[]) {"array_stream", "sbc_enc", "sbc_dec", "uart_stream"}, 4))
    {
        BK_LOGE(TAG, "pipeline link fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step5: init event listener ----------\n");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);

    if (BK_OK != audio_pipeline_set_listener(pipeline, evt))
    {
        BK_LOGE(TAG, "set listener fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step6: pipeline run ----------\n");
    if (BK_OK != audio_pipeline_run(pipeline))
    {
        BK_LOGE(TAG, "pipeline run fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    while (1)
    {
        audio_event_iface_msg_t msg;
        bk_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
        if (ret != BK_OK)
        {
            BK_LOGE(TAG, "[ * ] Event interface error : %d \n", ret);
            continue;
        }

        if (msg.source == (void *) sbc_dec && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO)
        {
            audio_element_info_t music_info = {0};
            audio_element_getinfo(sbc_dec, &music_info);
            BK_LOGD(TAG, "[ * ] Receive music info from sbc decoder, sample_rates=%d, bits=%d, ch=%d\n",
                    music_info.sample_rates, music_info.bits, music_info.channels);
            continue;
        }

        if (msg.cmd == AEL_MSG_CMD_REPORT_STATUS
            && (((int)msg.data == AEL_STATUS_STATE_STOPPED)
                || ((int)msg.data == AEL_STATUS_STATE_FINISHED)
                || (int)msg.data == AEL_STATUS_ERROR_PROCESS))
        {
            /* read array finish, wait encode and decode complete */
            if (msg.source == (void *)array_stream && (int)msg.data == AEL_STATUS_STATE_FINISHED)
            {
                // not stop
            }
            else
            {
                BK_LOGW(TAG, "[ * ] Stop event received \n");
                break;
            }
        }
    }

    BK_LOGD(TAG, "--------- step7: deinit pipeline ----------\n");
    if (BK_OK != audio_pipeline_stop(pipeline))
    {
        BK_LOGE(TAG, "pipeline stop fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_wait_for_stop(pipeline))
    {
        BK_LOGE(TAG, "pipeline wait stop fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_terminate(pipeline))
    {
        BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_unregister(pipeline, array_stream))
    {
        BK_LOGE(TAG, "pipeline unregister element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_unregister(pipeline, sbc_enc))
    {
        BK_LOGE(TAG, "pipeline unregister element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_unregister(pipeline, sbc_dec))
    {
        BK_LOGE(TAG, "pipeline unregister element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_unregister(pipeline, uart_stream))
    {
        BK_LOGE(TAG, "pipeline unregister element fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_remove_listener(pipeline))
    {
        BK_LOGE(TAG, "pipeline remove listener fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_event_iface_destroy(evt))
    {
        BK_LOGE(TAG, "event iface destroy fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_deinit(pipeline))
    {
        BK_LOGE(TAG, "pipeline deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }


    if (BK_OK != audio_element_deinit(array_stream))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_element_deinit(sbc_enc))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_element_deinit(sbc_dec))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_element_deinit(uart_stream))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- sbc encoder and decoder test complete ----------\n");

    AUDIO_MEM_SHOW("end \n");

    return BK_OK;
}
#endif
