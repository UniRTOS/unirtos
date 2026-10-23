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
#include <components/bk_audio/audio_streams/onboard_speaker_stream.h>
#include <components/bk_audio/audio_streams/array_stream.h>
#include <os/os.h>
#include "test_pcm_array.h"


#define TAG  "ONBOARD_SPEAKER_TEST"


#define TEST_CHECK_NULL(ptr) do {\
        if (ptr == NULL) {\
            BK_LOGD(TAG, "TEST_CHECK_NULL fail \n");\
            return BK_FAIL;\
        }\
    } while(0)


/* The "onboard speaker stream" element is a consumer. The element is the last element.
   Usually this element only has sink.
   The data flow model of this element is as follow:
   +-----------------+               +-------------------+
   |      array      |               |  onboard-speaker  |
   |      stream     |               |      stream       |
   |                 |               |                   |
   |                src - ringbuf - sink                 |
   |                 |               |                   |
   |                 |               |                   |
   +-----------------+               +-------------------+

   Function: Use onboard speaker stream to play mono 16k pcm audio data in array stream.

   The "array stream" element write mono 16k pcm audio data to array. The "onboard speaker stream" element read mono 16k pcm audio data from array and play the data.
*/
bk_err_t adk_onboard_speaker_test_case_0(void)
{
    audio_pipeline_handle_t pipeline;
    audio_element_handle_t onboard_spk, array_stream_reader;

    BK_LOGD(TAG, "--------- %s ----------\n", __func__);
    AUDIO_MEM_SHOW("start \n");

    BK_LOGD(TAG, "--------- step1: pipeline init ----------\n");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
    TEST_CHECK_NULL(pipeline);

    BK_LOGD(TAG, "--------- step2: init elements ----------\n");
    array_stream_cfg_t array_reader_cfg = DEFAULT_ARRAY_STREAM_CONFIG();
    array_reader_cfg.type = AUDIO_STREAM_READER;
    array_stream_reader = array_stream_init(&array_reader_cfg);
    TEST_CHECK_NULL(array_stream_reader);

    if (BK_OK != array_stream_set_data(array_stream_reader, (uint8_t *)MONO_16K_PCM_TEST_DATA, MONO_16K_PCM_TEST_DATA_LEN))
    {
        BK_LOGE(TAG, "set array stream data fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    onboard_speaker_stream_cfg_t onboard_spk_cfg = ONBOARD_SPEAKER_STREAM_CFG_DEFAULT();
    onboard_spk_cfg.sample_rate = 16000;
    onboard_spk_cfg.frame_size = 640;
    onboard_spk = onboard_speaker_stream_init(&onboard_spk_cfg);
    TEST_CHECK_NULL(onboard_spk);

    BK_LOGD(TAG, "--------- step3: pipeline register ----------\n");
    if (BK_OK != audio_pipeline_register(pipeline, array_stream_reader, "array_reader"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_register(pipeline, onboard_spk, "onboard_spk"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step4: pipeline link ----------\n");
    if (BK_OK != audio_pipeline_link(pipeline, (const char *[]){"array_reader", "onboard_spk"}, 2))
    {
        BK_LOGE(TAG, "pipeline link fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step5: init event listener ----------\n");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);
    TEST_CHECK_NULL(evt);

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

    rtos_delay_milliseconds(1000);
    BK_LOGD(TAG, "==============PAUSE======================\n");
    if (BK_OK != audio_element_pause(onboard_spk))
    {
        BK_LOGE(TAG, "%s, line: %d, audio_element_pause fail \n", __func__, __LINE__);
    }

    rtos_delay_milliseconds(5000);

    BK_LOGD(TAG, "==============RESUME======================\n");
    if (BK_OK != audio_element_resume(onboard_spk, 0, 2000))
    {
        BK_LOGE(TAG, "%s, line: %d, audio_element_resume fail \n", __func__, __LINE__);
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
            && (((int)(uintptr_t)msg.data == AEL_STATUS_STATE_STOPPED) || ((int)(uintptr_t)msg.data == AEL_STATUS_STATE_FINISHED)))
        {
            BK_LOGW(TAG, "[ * ] Stop event received \n");
            break;
        }
    }

    BK_LOGD(TAG, "--------- step7: stop pipeline ----------\n");
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

    BK_LOGD(TAG, "--------- step8: deinit pipeline ----------\n");
    if (BK_OK != audio_pipeline_terminate(pipeline))
    {
        BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_unregister(pipeline, array_stream_reader))
    {
        BK_LOGE(TAG, "pipeline unregister array_stream_reader fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_unregister(pipeline, onboard_spk))
    {
        BK_LOGE(TAG, "pipeline unregister onboard_spk fail, %d \n", __LINE__);
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

    if (BK_OK != audio_element_deinit(array_stream_reader))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_element_deinit(onboard_spk))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- onboard speaker test complete ----------\n");

    AUDIO_MEM_SHOW("end \n");

    return BK_OK;
}

