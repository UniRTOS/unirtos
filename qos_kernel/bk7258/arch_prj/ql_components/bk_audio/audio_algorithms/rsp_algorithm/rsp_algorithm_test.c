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
#include <components/bk_audio/audio_algorithms/rsp_algorithm.h>
#include <os/os.h>


#define TAG  "RSP_ALGORITHM_TEST"


#define TEST_CHECK_NULL(ptr) do {\
        if (ptr == NULL) {\
            BK_LOGE(TAG, "TEST_CHECK_NULL fail, line: %d \n", __LINE__);\
            return BK_FAIL;\
        }\
    } while(0)

/* number of 20ms frames fed into the resampler in one test case */
#define RSP_TEST_FRAME_NUM      (50)

/* allowed deviation (percent) between the measured and the theoretical output/input ratio.
   the resampler is stateful and may keep a fractional frame internally, so a small
   tolerance is needed instead of an exact match. */
#define RSP_TEST_RATIO_TOLERANCE_PCT   (5)

/* runtime context shared between the test elements of one running case */
static int       g_rsp_src_rate = 0;       /**< source sample rate of the current case */
static int       g_rsp_dest_rate = 0;      /**< destination sample rate of the current case */
static uint32_t  g_rsp_src_frame_bytes = 0;/**< bytes of one 20ms source frame (16bit mono) */
static int       g_rsp_read_count = 0;     /**< number of frames already produced by the input element */
static uint32_t  g_rsp_in_bytes = 0;       /**< total bytes fed into the pipeline */
static uint32_t  g_rsp_out_bytes = 0;      /**< total bytes produced after resampling */
static int16_t  *g_rsp_in_frame = NULL;    /**< one source frame of test pcm data */

static bk_err_t _el_open(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] _el_open \n", audio_element_get_tag(self));
    return BK_OK;
}

static bk_err_t _el_close(audio_element_handle_t self)
{
    BK_LOGD(TAG, "[%s] _el_close \n", audio_element_get_tag(self));
    return BK_OK;
}

/* input test element: produce RSP_TEST_FRAME_NUM frames of source pcm then report DONE */
static int _el_read(audio_port_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    audio_element_handle_t el = (audio_element_handle_t)context;
    BK_LOGV(TAG, "[%s] _el_read, len: %d \n", audio_element_get_tag(el), len);

    if (g_rsp_read_count >= RSP_TEST_FRAME_NUM)
    {
        /* no more data, return 0 to finish the pipeline */
        return 0;
    }

    /* fill the requested length with the prepared source frame (repeating if needed) */
    if (g_rsp_in_frame && g_rsp_src_frame_bytes)
    {
        int copied = 0;
        while (copied < len)
        {
            int chunk = len - copied;
            if (chunk > (int)g_rsp_src_frame_bytes)
            {
                chunk = (int)g_rsp_src_frame_bytes;
            }
            os_memcpy(buffer + copied, g_rsp_in_frame, chunk);
            copied += chunk;
        }
    }

    g_rsp_read_count++;
    g_rsp_in_bytes += len;

    return len;
}

static int _el_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    BK_LOGV(TAG, "[%s] _el_process, in_len: %d \n", audio_element_get_tag(self), in_len);

    int r_size = audio_element_input(self, in_buffer, in_len);

    int w_size = 0;
    if (r_size > 0)
    {
        w_size = audio_element_output(self, in_buffer, r_size);
    }
    else
    {
        w_size = r_size;
    }

    return w_size;
}

/* output test element: accumulate the total number of resampled bytes */
static int _el_write(audio_port_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    audio_element_handle_t el = (audio_element_handle_t)context;
    BK_LOGV(TAG, "[%s] _el_write, len: %d \n", audio_element_get_tag(el), len);

    if (len > 0)
    {
        g_rsp_out_bytes += len;
    }

    return len;
}

/* prepare one 20ms source frame (16bit mono) filled with a sine wave */
static bk_err_t _rsp_test_prepare_input(int src_rate)
{
    g_rsp_src_frame_bytes = src_rate / 1000 * 2 * 20;   /* 20ms, 16bit, mono */
    uint32_t sample_num = g_rsp_src_frame_bytes / 2;

    g_rsp_in_frame = (int16_t *)os_malloc(g_rsp_src_frame_bytes);
    if (g_rsp_in_frame == NULL)
    {
        BK_LOGE(TAG, "malloc input frame fail, size: %d \n", g_rsp_src_frame_bytes);
        return BK_FAIL;
    }

    /* 1kHz tone, makes the resampling result audible/inspectable if dumped */
    for (uint32_t i = 0; i < sample_num; i++)
    {
        /* simple triangle/sawtooth like pattern is enough for ratio verification */
        g_rsp_in_frame[i] = (int16_t)((i % 32) * 1000 - 16000);
    }

    return BK_OK;
}

static void _rsp_test_release_input(void)
{
    if (g_rsp_in_frame)
    {
        os_free(g_rsp_in_frame);
        g_rsp_in_frame = NULL;
    }
}

/* check that the measured output/input byte ratio matches dest_rate/src_rate */
static bool _rsp_test_check_ratio(void)
{
    if (g_rsp_in_bytes == 0 || g_rsp_out_bytes == 0)
    {
        BK_LOGE(TAG, "no data flowed, in_bytes: %u, out_bytes: %u \n", g_rsp_in_bytes, g_rsp_out_bytes);
        return false;
    }

    /* expected_out = in_bytes * dest_rate / src_rate, compared with a tolerance */
    uint64_t expected_out = (uint64_t)g_rsp_in_bytes * (uint64_t)g_rsp_dest_rate / (uint64_t)g_rsp_src_rate;
    uint64_t diff = (g_rsp_out_bytes > expected_out) ? (g_rsp_out_bytes - expected_out) : (expected_out - g_rsp_out_bytes);
    uint64_t tolerance = expected_out * RSP_TEST_RATIO_TOLERANCE_PCT / 100;

    BK_LOGI(TAG, "%d -> %d, in: %u, out: %u, expected_out: %llu, diff: %llu, tolerance: %llu \n",
            g_rsp_src_rate, g_rsp_dest_rate, g_rsp_in_bytes, g_rsp_out_bytes,
            (unsigned long long)expected_out, (unsigned long long)diff, (unsigned long long)tolerance);

    return diff <= tolerance;
}

/*
   The "rsp-algorithm" element resamples pcm data from src_rate to dest_rate. The data flow model
   used by this test is as follow:

   +--------------+               +--------------+               +--------------+
   |   test in    |               |     rsp      |               |   test out   |
   |   (reader)   |               |  algorithm   |               |   (writer)   |
   |             src - ringbuf - sink           src - ringbuf - sink            |
   |              |               |              |               |              |
   +--------------+               +--------------+               +--------------+

   Function: feed RSP_TEST_FRAME_NUM frames of source pcm (16bit mono, 20ms/frame) into the
   resampler, accumulate the resampled output, and verify that the output/input byte ratio
   equals dest_rate/src_rate within RSP_TEST_RATIO_TOLERANCE_PCT.
*/
static bk_err_t _rsp_algorithm_run_case(int src_rate, int dest_rate)
{
    audio_pipeline_handle_t pipeline = NULL;
    audio_element_handle_t rsp_alg = NULL, test_stream_in = NULL, test_stream_out = NULL;
    audio_event_iface_handle_t evt = NULL;
    bk_err_t test_ret = BK_OK;

    BK_LOGD(TAG, "--------- %s: %d -> %d ----------\n", __func__, src_rate, dest_rate);
    AUDIO_MEM_SHOW("start \n");

    /* reset runtime context for this case */
    g_rsp_src_rate = src_rate;
    g_rsp_dest_rate = dest_rate;
    g_rsp_read_count = 0;
    g_rsp_in_bytes = 0;
    g_rsp_out_bytes = 0;

    if (BK_OK != _rsp_test_prepare_input(src_rate))
    {
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step1: pipeline init ----------\n");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
    if (pipeline == NULL)
    {
        _rsp_test_release_input();
        BK_LOGE(TAG, "pipeline init fail, line: %d \n", __LINE__);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- step2: init elements ----------\n");
    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    cfg.open = _el_open;
    cfg.close = _el_close;
    cfg.process = _el_process;
    cfg.in_type = PORT_TYPE_CB;   /* producer: input comes from the read callback */
    cfg.read = _el_read;
    cfg.out_type = PORT_TYPE_RB;
    cfg.write = NULL;
    cfg.buffer_len = src_rate / 1000 * 2 * 20;   /* one source frame */
    cfg.tag = "stream_in";
    test_stream_in = audio_element_init(&cfg);

    cfg.open = _el_open;
    cfg.close = _el_close;
    cfg.process = _el_process;
    cfg.in_type = PORT_TYPE_RB;
    cfg.read = NULL;
    cfg.out_type = PORT_TYPE_CB;  /* consumer: output goes to the write callback */
    cfg.write = _el_write;
    cfg.buffer_len = dest_rate / 1000 * 2 * 20;   /* one destination frame */
    cfg.tag = "stream_out";
    test_stream_out = audio_element_init(&cfg);

    rsp_algorithm_cfg_t rsp_alg_cfg = DEFAULT_RSP_ALGORITHM_CONFIG();
    rsp_alg_cfg.rsp_cfg.src_rate = src_rate;
    rsp_alg_cfg.rsp_cfg.dest_rate = dest_rate;
    rsp_alg_cfg.rsp_cfg.src_ch = 1;
    rsp_alg_cfg.rsp_cfg.dest_ch = 1;
    rsp_alg_cfg.rsp_cfg.src_bits = 16;
    rsp_alg_cfg.rsp_cfg.dest_bits = 16;
    rsp_alg = rsp_algorithm_init(&rsp_alg_cfg);

    if (test_stream_in == NULL || test_stream_out == NULL || rsp_alg == NULL)
    {
        BK_LOGE(TAG, "init elements fail, line: %d \n", __LINE__);
        test_ret = BK_FAIL;
        goto _rsp_test_exit;
    }

    BK_LOGD(TAG, "--------- step3: pipeline register ----------\n");
    if (BK_OK != audio_pipeline_register(pipeline, test_stream_in, "stream_in")
        || BK_OK != audio_pipeline_register(pipeline, rsp_alg, "rsp_alg")
        || BK_OK != audio_pipeline_register(pipeline, test_stream_out, "stream_out"))
    {
        BK_LOGE(TAG, "register element fail, line: %d \n", __LINE__);
        test_ret = BK_FAIL;
        goto _rsp_test_exit;
    }

    BK_LOGD(TAG, "--------- step4: pipeline link ----------\n");
    if (BK_OK != audio_pipeline_link(pipeline, (const char *[]){"stream_in", "rsp_alg", "stream_out"}, 3))
    {
        BK_LOGE(TAG, "pipeline link fail, line: %d \n", __LINE__);
        test_ret = BK_FAIL;
        goto _rsp_test_exit;
    }

    BK_LOGD(TAG, "--------- step5: init event listener ----------\n");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    evt = audio_event_iface_init(&evt_cfg);
    if (evt == NULL || BK_OK != audio_pipeline_set_listener(pipeline, evt))
    {
        BK_LOGE(TAG, "set listener fail, line: %d \n", __LINE__);
        test_ret = BK_FAIL;
        goto _rsp_test_exit;
    }

    BK_LOGD(TAG, "--------- step6: pipeline run ----------\n");
    if (BK_OK != audio_pipeline_run(pipeline))
    {
        BK_LOGE(TAG, "pipeline run fail, line: %d \n", __LINE__);
        test_ret = BK_FAIL;
        goto _rsp_test_exit;
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
            && msg.source == (void *)test_stream_out
            && (((int)msg.data == AEL_STATUS_STATE_STOPPED) || ((int)msg.data == AEL_STATUS_STATE_FINISHED)))
        {
            BK_LOGW(TAG, "[ * ] Stop event received \n");
            break;
        }
    }

    BK_LOGD(TAG, "--------- step7: deinit pipeline ----------\n");
    audio_pipeline_stop(pipeline);
    audio_pipeline_wait_for_stop(pipeline);
    audio_pipeline_terminate(pipeline);

_rsp_test_exit:
    if (pipeline)
    {
        if (test_stream_in)
        {
            audio_pipeline_unregister(pipeline, test_stream_in);
        }
        if (rsp_alg)
        {
            audio_pipeline_unregister(pipeline, rsp_alg);
        }
        if (test_stream_out)
        {
            audio_pipeline_unregister(pipeline, test_stream_out);
        }
        if (evt)
        {
            audio_pipeline_remove_listener(pipeline);
        }
    }
    if (evt)
    {
        audio_event_iface_destroy(evt);
    }
    if (pipeline)
    {
        audio_pipeline_deinit(pipeline);
    }
    if (test_stream_in)
    {
        audio_element_deinit(test_stream_in);
    }
    if (rsp_alg)
    {
        audio_element_deinit(rsp_alg);
    }
    if (test_stream_out)
    {
        audio_element_deinit(test_stream_out);
    }

    /* verify the resample ratio only when the pipeline ran successfully */
    if (test_ret == BK_OK && !_rsp_test_check_ratio())
    {
        test_ret = BK_FAIL;
    }

    _rsp_test_release_input();

    BK_LOGD(TAG, "--------- rsp algorithm test %d -> %d: %s ----------\n",
            src_rate, dest_rate, (test_ret == BK_OK) ? "PASS" : "FAIL");
    AUDIO_MEM_SHOW("end \n");

    return test_ret;
}

/* case 0: upsample 8000 -> 16000 (integer ratio x2) */
bk_err_t adk_rsp_algorithm_test_case_0(void)
{
    return _rsp_algorithm_run_case(8000, 16000);
}

/* case 1: downsample 16000 -> 8000 (integer ratio x0.5) */
bk_err_t adk_rsp_algorithm_test_case_1(void)
{
    return _rsp_algorithm_run_case(16000, 8000);
}

/* case 2: upsample 16000 -> 48000 (integer ratio x3, exercises >2x upsampling) */
bk_err_t adk_rsp_algorithm_test_case_2(void)
{
    return _rsp_algorithm_run_case(16000, 48000);
}

/* case 3: non integer ratio downsample 44100 -> 16000 */
bk_err_t adk_rsp_algorithm_test_case_3(void)
{
    return _rsp_algorithm_run_case(44100, 16000);
}

/* case 4: invalid config, src_rate == dest_rate, rsp_algorithm_init() must return NULL */
bk_err_t adk_rsp_algorithm_test_case_4(void)
{
    BK_LOGD(TAG, "--------- %s: src_rate == dest_rate (expect init fail) ----------\n", __func__);

    rsp_algorithm_cfg_t rsp_alg_cfg = DEFAULT_RSP_ALGORITHM_CONFIG();
    rsp_alg_cfg.rsp_cfg.src_rate = 16000;
    rsp_alg_cfg.rsp_cfg.dest_rate = 16000;
    audio_element_handle_t rsp_alg = rsp_algorithm_init(&rsp_alg_cfg);
    if (rsp_alg != NULL)
    {
        BK_LOGE(TAG, "rsp_algorithm_init should fail when src_rate == dest_rate \n");
        audio_element_deinit(rsp_alg);
        return BK_FAIL;
    }

    BK_LOGD(TAG, "--------- rsp algorithm test src==dest: PASS ----------\n");
    return BK_OK;
}

/* run all rsp algorithm test cases */
bk_err_t adk_rsp_algorithm_test(void)
{
    bk_err_t ret = BK_OK;

    ret |= adk_rsp_algorithm_test_case_0();
    ret |= adk_rsp_algorithm_test_case_1();
    ret |= adk_rsp_algorithm_test_case_2();
    ret |= adk_rsp_algorithm_test_case_3();
    ret |= adk_rsp_algorithm_test_case_4();

    BK_LOGI(TAG, "--------- rsp algorithm all test cases: %s ----------\n", (ret == BK_OK) ? "PASS" : "FAIL");

    return ret;
}
