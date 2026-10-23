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
#include <components/bk_audio/audio_decoders/mp3_decoder.h>
#include <os/os.h>
#include <components/bk_audio/audio_streams/array_stream.h>
#include <components/bk_audio/audio_streams/onboard_speaker_stream.h>
#include "test_mp3_array.h"
#if CONFIG_ADK_VFS_STREAM
#include <components/bk_audio/audio_streams/vfs_stream.h>
#include "bk_posix.h"
#endif

#define TAG  "MP3_DECODER_TEST"

#define TEST_CHECK_NULL(ptr) do {\
        if (ptr == NULL) {\
            BK_LOGD(TAG, "TEST_CHECK_NULL fail \n");\
            return BK_FAIL;\
        }\
    } while(0)


/* The "mp3-decoder" element is neither a producer nor a consumer when test element
   is neither first element nor last element of the pipeline. Usually this element has
   both src and sink. The data flow model of this element is as follow:
   +--------------+               +--------------+               +--------------+
   |    array     |               |     mp3      |               |   onboard    |
   |    stream    |               |   decoder    |               |   speaker    |
   |             src - ringbuf - sink           src - ringbuf - sink            |
   |              |               |              |               |              |
   +--------------+               +--------------+               +--------------+

   Function: Use mp3 decoder to decode mp3 data from array and play through onboard speaker.

   The "array-stream" element write mp3 data from array to ringbuffer. The
   "mp3-decoder" element read audio data from ringbuffer, decode the data to pcm format
   and write the data to ringbuffer. The "onboard-speaker" element read pcm data from
   ringbuffer, and play the audio.
*/
bk_err_t adk_mp3_decoder_test_case_0(void)
{
    audio_pipeline_handle_t pipeline;
    audio_element_handle_t array_stream, mp3_dec, speaker_stream;
    audio_event_iface_handle_t evt;

    BK_LOGD(TAG, "--------- %s ----------\n", __func__);
    AUDIO_MEM_SHOW("start \n");

    BK_LOGD(TAG, "--------- step1: pipeline init ----------\n");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
    TEST_CHECK_NULL(pipeline);

    BK_LOGD(TAG, "--------- step2: init elements ----------\n");
    array_stream_cfg_t array_cfg = DEFAULT_ARRAY_STREAM_CONFIG();
    array_cfg.type = AUDIO_STREAM_READER;
    array_cfg.buf_sz = MP3_DECODER_MAIN_BUFF_SIZE;
    array_stream = array_stream_init(&array_cfg);
    TEST_CHECK_NULL(array_stream);

    if (BK_OK != array_stream_set_data(array_stream, (uint8_t *)MP3_TEST_DATA, MP3_TEST_DATA_LEN))
    {
        BK_LOGE(TAG, "set array data fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
    mp3_dec = mp3_decoder_init(&mp3_cfg);
    TEST_CHECK_NULL(mp3_dec);

    onboard_speaker_stream_cfg_t speaker_cfg = DEFAULT_ONBOARD_SPEAKER_STREAM_CONFIG();
    speaker_stream = onboard_speaker_stream_init(&speaker_cfg);
    TEST_CHECK_NULL(speaker_stream);

    BK_LOGD(TAG, "--------- step3: pipeline register ----------\n");
    if (BK_OK != audio_pipeline_register(pipeline, array_stream, "array_stream"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_register(pipeline, mp3_dec, "mp3_dec"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_register(pipeline, speaker_stream, "speaker_stream"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step4: pipeline link ----------\n");
    if (BK_OK != audio_pipeline_link(pipeline, (const char *[]){"array_stream", "mp3_dec", "speaker_stream"}, 3))
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
        static int count = 0;
        audio_event_iface_msg_t msg;
        bk_err_t ret = audio_event_iface_listen(evt, &msg, 500 / portTICK_RATE_MS);
        if (ret != BK_OK)
        {
            BK_LOGE(TAG, "[ * ] Event interface error : %d \n", ret);
            count++;
            if (count == 2)
            {
                audio_pipeline_pause(pipeline);
                rtos_delay_milliseconds(5000);
                audio_pipeline_resume(pipeline);
            }
            continue;
        }

        if (msg.source == (void *) mp3_dec && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO)
        {
            audio_element_info_t music_info = {0};
            audio_element_getinfo(mp3_dec, &music_info);
            BK_LOGD(TAG, "[ * ] Receive music info from mp3 decoder, sample_rates=%d, bits=%d, ch=%d\n",
                    music_info.sample_rates, music_info.bits, music_info.channels);

            onboard_speaker_stream_set_param(speaker_stream, music_info.sample_rates, music_info.bits, music_info.channels);
            continue;
        }

        if (msg.cmd == AEL_MSG_CMD_REPORT_STATUS
            && (((int)msg.data == AEL_STATUS_STATE_STOPPED)
                || ((int)msg.data == AEL_STATUS_STATE_FINISHED)
                || (int)msg.data == AEL_STATUS_ERROR_PROCESS))
        {
            if ((int)msg.data == AEL_STATUS_ERROR_PROCESS)
            {
                BK_LOGW(TAG, "[ * ] Stop event received \n");
                break;
            }

            if (msg.source == (void *) speaker_stream && 
                ((int)msg.data == AEL_STATUS_STATE_FINISHED
                || (int)msg.data == AEL_STATUS_STATE_STOPPED))
            {
                BK_LOGW(TAG, "[ * ] Speaker stream finished or stopped \n");
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
    if (BK_OK != audio_pipeline_unregister(pipeline, mp3_dec))
    {
        BK_LOGE(TAG, "pipeline unregister element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_unregister(pipeline, speaker_stream))
    {
        BK_LOGE(TAG, "pipeline unregister element fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_remove_listener(pipeline))
    {
        BK_LOGE(TAG, "listener remove fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_event_iface_destroy(evt))
    {
        BK_LOGE(TAG, "listener destroy fail, %d \n", __LINE__);
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

    if (BK_OK != audio_element_deinit(mp3_dec))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_element_deinit(speaker_stream))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- mp3 decoder test complete ----------\n");
    AUDIO_MEM_SHOW("end \n");

    return BK_OK;
}

#if CONFIG_ADK_VFS_STREAM
static bool is_mounted = false;
/* mount sdcard */
static int vfs_mount_sd0_fatfs(void)
{
	int ret = BK_OK;
	if(!is_mounted) {
		struct bk_fatfs_partition partition;
		char *fs_name = NULL;
		fs_name = "fatfs";
		partition.part_type = FATFS_DEVICE;
		partition.part_dev.device_name = FATFS_DEV_SDCARD;
		partition.mount_path = VFS_SD_0_PATITION_0;
		ret = mount("SOURCE_NONE", partition.mount_path, fs_name, 0, &partition);
		is_mounted = true;
		BK_LOGI(TAG, "func %s, mount : %s\n", __func__, partition.mount_path);
	}
	return ret;
}

static bk_err_t vfs_unmount_sd0_fatfs(void)
{
    is_mounted = false;
    return umount(VFS_SD_0_PATITION_0);
}


/* The "mp3-decoder" element is neither a producer nor a consumer when test element
   is neither first element nor last element of the pipeline. Usually this element has
   both src and sink. The data flow model of this element is as follow:
   +--------------+               +--------------+               +--------------+
   |     vfs      |               |     mp3      |               |   onboard    |
   |    stream    |               |   decoder    |               |   speaker    |
   |             src - ringbuf - sink           src - ringbuf - sink            |
   |              |               |              |               |              |
   +--------------+               +--------------+               +--------------+

   Function: Use mp3 decoder to decode mp3 file from filesystem and play through onboard speaker.

   The "vfs-stream" element read mp3 file from tfcard to ringbuffer. The
   "mp3-decoder" element read audio data from ringbuffer, decode the data to pcm format
   and write the data to ringbuffer. The "onboard-speaker" element read pcm data from
   ringbuffer, and play the audio.
*/
bk_err_t adk_mp3_decoder_test_case_1(char *file_path)
{
    audio_pipeline_handle_t pipeline;
    audio_element_handle_t vfs_stream, mp3_dec, speaker_stream;
    audio_event_iface_handle_t evt;

    BK_LOGD(TAG, "--------- %s ----------\n", __func__);
    AUDIO_MEM_SHOW("start \n");

    if (BK_OK != vfs_mount_sd0_fatfs())
    {
        BK_LOGE(TAG, "mount tfcard fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    rtos_delay_milliseconds(2000);

    if (file_path == NULL)
    {
        BK_LOGE(TAG, "file path is NULL, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step1: pipeline init ----------\n");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
    TEST_CHECK_NULL(pipeline);

    BK_LOGD(TAG, "--------- step2: init elements ----------\n");
    vfs_stream_cfg_t vfs_cfg = DEFAULT_VFS_STREAM_CONFIG();
    vfs_cfg.type = AUDIO_STREAM_READER;
    vfs_cfg.buf_sz = 8192;
    vfs_cfg.out_block_size = MP3_DECODER_MAIN_BUFF_SIZE;
    vfs_stream = vfs_stream_init(&vfs_cfg);
    TEST_CHECK_NULL(vfs_stream);

    if (BK_OK != audio_element_set_uri(vfs_stream, file_path))
    {
        BK_LOGE(TAG, "set uri fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
    mp3_dec = mp3_decoder_init(&mp3_cfg);
    TEST_CHECK_NULL(mp3_dec);

    onboard_speaker_stream_cfg_t speaker_cfg = DEFAULT_ONBOARD_SPEAKER_STREAM_CONFIG();
    speaker_cfg.sample_rate = 44100;
    speaker_cfg.chl_num = 2;
    speaker_cfg.bits = 16;
    speaker_stream = onboard_speaker_stream_init(&speaker_cfg);
    TEST_CHECK_NULL(speaker_stream);

    BK_LOGD(TAG, "--------- step3: pipeline register ----------\n");
    if (BK_OK != audio_pipeline_register(pipeline, vfs_stream, "vfs_stream"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_register(pipeline, mp3_dec, "mp3_dec"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_register(pipeline, speaker_stream, "speaker_stream"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step4: pipeline link ----------\n");
    if (BK_OK != audio_pipeline_link(pipeline, (const char *[]){"vfs_stream", "mp3_dec", "speaker_stream"}, 3))
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
        static int count = 0;
        audio_event_iface_msg_t msg;
        bk_err_t ret = audio_event_iface_listen(evt, &msg, 1000 / portTICK_RATE_MS);
        if (ret != BK_OK)
        {
            BK_LOGE(TAG, "[ * ] Event interface error : %d \n", ret);
            count++;
            if (count == 10)
            {
                audio_pipeline_pause(pipeline);
                rtos_delay_milliseconds(5000);
                audio_pipeline_resume(pipeline);
            }
            continue;
        }

        if (msg.source == (void *) mp3_dec && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO)
        {
            audio_element_info_t music_info = {0};
            audio_element_getinfo(mp3_dec, &music_info);
            BK_LOGD(TAG, "[ * ] Receive music info from mp3 decoder, sample_rates=%d, bits=%d, ch=%d\n",
                    music_info.sample_rates, music_info.bits, music_info.channels);

            onboard_speaker_stream_set_param(speaker_stream, music_info.sample_rates, 
                                            music_info.bits, music_info.channels);
            continue;
        }

        if (msg.cmd == AEL_MSG_CMD_REPORT_STATUS
            && (((int)msg.data == AEL_STATUS_STATE_STOPPED)
                || ((int)msg.data == AEL_STATUS_STATE_FINISHED)
                || (int)msg.data == AEL_STATUS_ERROR_PROCESS))
        {
            /* read mp3 file finish, wait decode complete */
            if (msg.source == (void *)vfs_stream && (int)msg.data == AEL_STATUS_STATE_FINISHED)
            {
                //not stop
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
    if (BK_OK != audio_pipeline_unregister(pipeline, vfs_stream))
    {
        BK_LOGE(TAG, "pipeline unregister element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_unregister(pipeline, mp3_dec))
    {
        BK_LOGE(TAG, "pipeline unregister element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_unregister(pipeline, speaker_stream))
    {
        BK_LOGE(TAG, "pipeline unregister element fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_remove_listener(pipeline))
    {
        BK_LOGE(TAG, "listener remove fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_event_iface_destroy(evt))
    {
        BK_LOGE(TAG, "listener destroy fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_deinit(pipeline))
    {
        BK_LOGE(TAG, "pipeline deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_element_deinit(vfs_stream))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_element_deinit(mp3_dec))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_element_deinit(speaker_stream))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    vfs_unmount_sd0_fatfs();

    BK_LOGD(TAG, "--------- mp3 decoder test complete ----------\n");
    AUDIO_MEM_SHOW("end \n");

    return BK_OK;
}
#endif
