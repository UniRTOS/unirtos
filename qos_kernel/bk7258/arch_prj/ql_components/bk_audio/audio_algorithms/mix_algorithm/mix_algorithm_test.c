#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"

#include <components/bk_audio/audio_pipeline/audio_pipeline.h>
#include <components/bk_audio/audio_pipeline/audio_event_iface.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>

#include <components/bk_audio/audio_streams/array_stream.h>
#include <components/bk_audio/audio_streams/uart_stream.h>
#include <components/bk_audio/audio_algorithms/mix_algorithm.h>

#include "test_mix_array.h"

#define TAG "mix_algorithm_test"

#define TEST_CHECK_NULL(ptr) do {\
        if (!(ptr)) {\
            BK_LOGE(TAG, "NULL point at %s:%d \n", __FILE__, __LINE__);\
            return BK_FAIL;\
        }\
    } while (0);\


/* The "mix-algorithm" element is neither a producer nor a consumer when test element
   is neither first element nor last element of the pipeline. Usually this element has
   both src and sink. The data flow model of this element is as follow:
   +--------------+               +--------------+               +--------------+
   |     array    |               |     mix      |               |     uart     |
   |    stream    |               |  algorithm   |               |    stream    |
   |             src - ringbuf - sink           src - ringbuf - sink            |
   |              |               |              |               |              |
   +--------------+               +--------------+               +--------------+

   Function: Use mix algorithm to convert dual channel audio data to single channel.

   The "mix-algorithm" element read audio data from array through array stream, 
   convert the data from dual channel to single channel and write the data to uart through uart stream.
*/

bk_err_t adk_mix_algorithm_test_case_0(void)
{
    audio_pipeline_handle_t pipeline;
    audio_element_handle_t mix_algorithm, array_stream, uart_stream;

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
    array_stream_set_data(array_stream, (uint8_t *)a2dp_16k_dual_16bit, sizeof(a2dp_16k_dual_16bit));

    mix_algorithm_cfg_t mix_algorithm_cfg = DEFAULT_MIX_ALGORITHM_CONFIG();
    mix_algorithm_cfg.input_channel_num = 2;  // 双声道输入
    mix_algorithm = mix_algorithm_init(&mix_algorithm_cfg);
    TEST_CHECK_NULL(mix_algorithm);

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
    if (BK_OK != audio_pipeline_register(pipeline, mix_algorithm, "mix_algorithm"))
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
    if (BK_OK != audio_pipeline_link(pipeline, (const char *[]){"array_stream", "mix_algorithm", "uart_stream"}, 3))
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
        BK_LOGE(TAG, "pipeline set listener fail, %d \n", __LINE__);
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
    if (BK_OK != audio_pipeline_unregister(pipeline, mix_algorithm))
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

    if (BK_OK != audio_element_deinit(mix_algorithm))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_element_deinit(uart_stream))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- mix algorithm test complete ----------\n");

    AUDIO_MEM_SHOW("end \n");

    return BK_OK;
}