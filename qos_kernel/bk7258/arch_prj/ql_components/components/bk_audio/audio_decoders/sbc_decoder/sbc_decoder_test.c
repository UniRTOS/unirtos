/*
 * Copyright 2025-2026 Beken
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "FreeRTOS.h"
#include "task.h"
#include <components/bk_audio/audio_pipeline/audio_pipeline.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_decoders/sbc_decoder.h>
#include <components/bk_audio/audio_streams/uart_stream.h>
#include <components/bk_audio/audio_streams/raw_stream.h>
#include "test_sbc_array.h"

#define TAG "SBC_DECODER_TEST"

#define TEST_FRAME_SIZE 119

#define TEST_CHECK_NULL(ptr) do {\
        if (ptr == NULL) {\
            BK_LOGD(TAG, "TEST_CHECK_NULL fail \n");\
            return BK_FAIL;\
        }\
    } while(0)



bk_err_t adk_sbc_decoder_test_case_0(void)
{
    bk_err_t ret = BK_OK;
    audio_pipeline_handle_t pipeline = NULL;
    audio_element_handle_t raw_stream = NULL;
    audio_element_handle_t sbc_dec = NULL;
    audio_element_handle_t uart_stream = NULL;
    audio_event_iface_handle_t evt = NULL;

    BK_LOGD(TAG, "--------- SBC Decoder Test Start ----------\n");

    BK_LOGD(TAG, "--------- step1: pipeline init ----------\n");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
    TEST_CHECK_NULL(pipeline);

    BK_LOGD(TAG, "--------- step2: init elements ----------\n");
    // 初始化raw_stream，配置为frame buffer模式
    raw_stream_cfg_t raw_cfg = DEFAULT_RAW_STREAM_CONFIG();
    raw_cfg.type = AUDIO_STREAM_READER;
    raw_cfg.out_block_size = TEST_FRAME_SIZE;  // 设置为宏指定的固定size
    raw_cfg.out_block_num = 4;
    raw_cfg.output_port_type = PORT_TYPE_FB;
    raw_stream = raw_stream_init(&raw_cfg);
    TEST_CHECK_NULL(raw_stream);

    // 初始化SBC解码器
    sbc_decoder_cfg_t sbc_decoder_cfg = DEFAULT_SBC_DECODER_CONFIG();
    sbc_dec = sbc_decoder_init(&sbc_decoder_cfg);
    TEST_CHECK_NULL(sbc_dec);

    // 初始化UART流用于输出
    uart_stream_cfg_t uart_cfg = DEFAULT_UART_STREAM_CONFIG();
    uart_cfg.type = AUDIO_STREAM_WRITER;
    uart_stream = uart_stream_init(&uart_cfg);
    TEST_CHECK_NULL(uart_stream);

    BK_LOGD(TAG, "--------- step3: pipeline register ----------\n");
    if (BK_OK != audio_pipeline_register(pipeline, raw_stream, "raw_stream"))
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
    if (BK_OK != audio_pipeline_link(pipeline, (const char *[]) {"raw_stream", "sbc_dec", "uart_stream"}, 3))
    {
        BK_LOGE(TAG, "pipeline link fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step5: init event listener ----------\n");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    evt = audio_event_iface_init(&evt_cfg);
    TEST_CHECK_NULL(evt);

    if (BK_OK != audio_pipeline_set_listener(pipeline, evt))
    {
        BK_LOGE(TAG, "pipeline set listener fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step6: pipeline run ----------\n");
    if (BK_OK != audio_pipeline_run(pipeline))
    {
        BK_LOGE(TAG, "pipeline run fail, %d ", __LINE__);
        return BK_FAIL;
    }

    // 等待数组读取结束退出
    while (1)
    {
        // 按照宏定义的TEST_FRAME_SIZE大小写入数据的逻辑
        static int current_index = 0;
        static bool data_all_written = false;
        
        // 如果数据尚未全部写入完成，则继续写入
        if (!data_all_written)
        {
            // 计算本次实际写入的字节数，不超过TEST_FRAME_SIZE和剩余数据量
            int bytes_to_write = (current_index + TEST_FRAME_SIZE <= sizeof(a2dp_sbc_music)) ? 
                                    TEST_FRAME_SIZE : (sizeof(a2dp_sbc_music) - current_index);

            int bytes_written = raw_stream_write(raw_stream, (char *)(a2dp_sbc_music + current_index), bytes_to_write);
            if (bytes_written > 0)
            {
                current_index += bytes_written;
                BK_LOGD(TAG, "[ * ] Written %zu bytes (frame size: %d), total written: %zu/%zu bytes \n", 
                       bytes_written, TEST_FRAME_SIZE, current_index, sizeof(a2dp_sbc_music));
                
                // 如果所有数据都已写入完成，通知pipeline
                if (current_index >= sizeof(a2dp_sbc_music))
                {
                    BK_LOGD(TAG, "[ * ] All data written to raw_stream \n");
                    ret = audio_element_set_port_done(raw_stream);
                    if (ret == BK_OK)
                    {
                        BK_LOGD(TAG, "[ * ] Notify pipeline raw_stream data done \n");
                        data_all_written = true;
                    }
                    else
                    {
                        BK_LOGE(TAG, "[ * ] Failed to notify pipeline, ret=%d \n", ret);
                    }
                }
            }
            else if (ret != BK_OK)
            {
                BK_LOGE(TAG, "[ * ] Failed to write data to raw_stream, ret=%d \n", ret);
                // 出错时稍微延迟重试
                rtos_delay_milliseconds(10);
            }
            // 如果写入了0字节，但没有错误，可能是流已满，稍微延迟后重试
            else if (bytes_written == 0)
            {
                BK_LOGD(TAG, "[ * ] raw_stream buffer is full, waiting... \n");
                rtos_delay_milliseconds(10);
            }
        }

        // 监听事件，使用较短的超时时间，以便能够定期检查是否需要写入数据
        audio_event_iface_msg_t msg;
        ret = audio_event_iface_listen(evt, &msg, BK_MS_TO_TICKS(10));
        if (ret == BK_OK)
        {
            if (msg.cmd == AEL_MSG_CMD_REPORT_STATUS
                && msg.source == uart_stream
                && (((int)msg.data == AEL_STATUS_STATE_STOPPED) || ((int)msg.data == AEL_STATUS_STATE_FINISHED)))
            {
                BK_LOGW(TAG, "[ * ] Stop event received \n");
                break;
            }
        }
        else if (ret != BK_ERR_TIMEOUT)
        {
            BK_LOGE(TAG, "[ * ] Event interface error : %d \n", ret);
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
    if (BK_OK != audio_pipeline_unregister(pipeline, raw_stream))
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

    if (BK_OK != audio_element_deinit(raw_stream))
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

    BK_LOGD(TAG, "--------- SBC decoder test complete ----------\n");

    AUDIO_MEM_SHOW("end \n");

    return BK_OK;
}
