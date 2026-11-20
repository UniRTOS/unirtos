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


#include "plugin_manager.h"

#include <fcntl.h>
#include <unistd.h>
#include <common/bk_include.h>
#include <os/os.h>
#include "FreeRTOS.h"
#include "task.h"
#include <components/bk_audio/audio_pipeline/audio_pipeline.h>
#include <components/bk_audio/audio_streams/raw_stream.h>
#include <components/bk_audio/audio_streams/onboard_speaker_stream.h>


#define SINK_DEVICE_CHECK_NULL(ptr) do {\
        if (ptr == NULL) {\
            BK_LOGE(AUDIO_PLAYER_TAG, "SINK_DEVICE_CHECK_NULL fail \n");\
            return BK_FAIL;\
        }\
    } while(0)


static audio_element_handle_t onboard_spk = NULL, raw_write = NULL;
static audio_pipeline_handle_t play_pipeline = NULL;
static bool audio_dac_mute_sta = false;

//#define SINK_DEVICE_DEBUG

#ifdef SINK_DEVICE_DEBUG
#include <components/bk_audio/audio_utils/uart_util.h>
static struct uart_util gl_ob_spk_dev_uart_util = {0};
#define ONBOARD_SPK_DEVICE_DEBUG_UART_ID            (1)
#define ONBOARD_SPK_DEVICE_DEBUG_UART_BAUD_RATE     (2000000)

#define ONBOARD_SPK_DEVICE_DEBUG_OPEN()                    uart_util_create(&gl_ob_spk_dev_uart_util, ONBOARD_SPK_DEVICE_DEBUG_UART_ID, ONBOARD_SPK_DEVICE_DEBUG_UART_BAUD_RATE)
#define ONBOARD_SPK_DEVICE_DEBUG_CLOSE()                   uart_util_destroy(&gl_ob_spk_dev_uart_util)
#define ONBOARD_SPK_DEVICE_DEBUG_DATA(data_buf, len)       uart_util_tx_data(&gl_ob_spk_dev_uart_util, data_buf, len)

#else

#define ONBOARD_SPK_DEVICE_DEBUG_OPEN()
#define ONBOARD_SPK_DEVICE_DEBUG_CLOSE()
#define ONBOARD_SPK_DEVICE_DEBUG_DATA(data_buf, len)
#endif


static bk_err_t play_pipeline_open(uint32_t samp_rate, uint8_t bits, uint8_t chl_num, uint8_t gain)
{
    BK_LOGI(AUDIO_PLAYER_TAG, "%s, sample_rate: %d, chl_num: %d, gain: %d \n", __func__, samp_rate, chl_num, gain);

    BK_LOGI(AUDIO_PLAYER_TAG, "step1: play pipeline init \n");
    audio_pipeline_cfg_t play_pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    play_pipeline_cfg.rb_size = 10 * 1024;
    play_pipeline = audio_pipeline_init(&play_pipeline_cfg);
    SINK_DEVICE_CHECK_NULL(play_pipeline);

    BK_LOGI(AUDIO_PLAYER_TAG, "step2: element init \n");
    onboard_speaker_stream_cfg_t onboard_spk_cfg = ONBOARD_SPEAKER_STREAM_CFG_DEFAULT();
    onboard_spk_cfg.chl_num = chl_num;
    onboard_spk_cfg.sample_rate = samp_rate;
    onboard_spk_cfg.dig_gain = gain;
    onboard_spk_cfg.frame_size = samp_rate * chl_num * 2 * 20 / 1000;
    onboard_spk_cfg.bits = bits;
    onboard_spk_cfg.task_stack = 1024;
    /* PA config */
    //onboard_spk_cfg.pa_ctrl_en = true;
    //onboard_spk_cfg.pa_ctrl_gpio = 33;
    //onboard_spk_cfg.pa_on_level = 1;
    //onboard_spk_cfg.pa_on_delay = 0;
    //onboard_spk_cfg.pa_off_delay = 0;
    onboard_spk = onboard_speaker_stream_init(&onboard_spk_cfg);
    SINK_DEVICE_CHECK_NULL(onboard_spk);

    raw_stream_cfg_t raw_write_cfg = RAW_STREAM_CFG_DEFAULT();
    raw_write_cfg.type = AUDIO_STREAM_WRITER;
    raw_write_cfg.out_block_size = 1024 * 10;
    raw_write_cfg.out_block_num = 1;
    raw_write_cfg.output_port_type = PORT_TYPE_RB;
    raw_write = raw_stream_init(&raw_write_cfg);
    SINK_DEVICE_CHECK_NULL(raw_write);

    BK_LOGI(AUDIO_PLAYER_TAG, "step3: element register \n");
    if (BK_OK != audio_pipeline_register(play_pipeline, onboard_spk, "onboard_spk"))
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, register element fail, %d \n", __func__, __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_register(play_pipeline, raw_write, "raw_write"))
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, register element fail, %d \n", __func__, __LINE__);
        return BK_FAIL;
    }

    BK_LOGI(AUDIO_PLAYER_TAG, "step4: pipeline link \n");
    /* pipeline record */
    if (BK_OK != audio_pipeline_link(play_pipeline, (const char *[]){"raw_write", "onboard_spk"}, 2))
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, pipeline link fail, %d \n", __func__, __LINE__);
        return BK_FAIL;
    }

    return BK_OK;
}

static bk_err_t play_pipeline_close(void)
{
    BK_LOGI(AUDIO_PLAYER_TAG, "%s \n", __func__);

    BK_LOGI(AUDIO_PLAYER_TAG, "step1: terminate play pipeline \n");
    if (BK_OK != audio_pipeline_terminate(play_pipeline))
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, pipeline terminate fail, %d \n", __func__, __LINE__);
        return BK_FAIL;
    }

    BK_LOGI(AUDIO_PLAYER_TAG, "step2: element unregister \n");
    if (BK_OK != audio_pipeline_unregister(play_pipeline, raw_write))
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, pipeline terminate fail, %d \n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_unregister(play_pipeline, onboard_spk))
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, pipeline terminate fail, %d \n", __func__, __LINE__);
        return BK_FAIL;
    }

    BK_LOGI(AUDIO_PLAYER_TAG, "step3: play pipeline deinit \n");
    if (BK_OK != audio_pipeline_deinit(play_pipeline))
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, pipeline terminate fail, %d \n", __func__, __LINE__);
        return BK_FAIL;
    }
    else
    {
        play_pipeline = NULL;
    }

    BK_LOGI(AUDIO_PLAYER_TAG, "step3: element deinit \n");
    if (BK_OK != audio_element_deinit(raw_write))
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, element deinit fail, %d \n", __func__, __LINE__);
        return BK_FAIL;
    }
    else
    {
        raw_write = NULL;
    }
    if (BK_OK != audio_element_deinit(onboard_spk))
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, element deinit fail, %d \n", __func__, __LINE__);
        return BK_FAIL;
    }
    else
    {
        onboard_spk = NULL;
    }

    return BK_OK;
}

static bk_err_t play_pipeline_start(void)
{
    BK_LOGI(AUDIO_PLAYER_TAG, "play pipeline run \n");

    if (BK_OK != audio_pipeline_run(play_pipeline))
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, pipeline run fail, %d \n", __func__, __LINE__);
        return BK_FAIL;
    }

    return BK_OK;
}

static bk_err_t play_pipeline_pause(void)
{
    BK_LOGI(AUDIO_PLAYER_TAG, "%s \n", __func__);

    audio_element_set_input_timeout(onboard_spk, 0);
    if (BK_OK != audio_pipeline_pause(play_pipeline))
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, play pipeline pause fail, %d \n", __func__, __LINE__);
        return BK_FAIL;
    }

    return BK_OK;
}

static bk_err_t play_pipeline_resume(void)
{
    BK_LOGI(AUDIO_PLAYER_TAG, "%s \n", __func__);

    if (BK_OK != audio_pipeline_resume(play_pipeline))
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, play pipeline resume fail, %d \n", __func__, __LINE__);
        return BK_FAIL;
    }
    audio_element_set_input_timeout(onboard_spk, 2000);

    return BK_OK;
}

static bk_err_t play_pipeline_mute(void)
{
    BK_LOGI(AUDIO_PLAYER_TAG, "%s \n", __func__);

    if (onboard_spk == NULL || BK_OK != onboard_speaker_stream_dac_mute_en(onboard_spk, 1))
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, play pipeline mute fail, %d \n", __func__, __LINE__);
        return BK_FAIL;
    }

    return BK_OK;
}

static bk_err_t play_pipeline_unmute(void)
{
    BK_LOGI(AUDIO_PLAYER_TAG, "%s \n", __func__);

    if (onboard_spk == NULL || BK_OK != onboard_speaker_stream_dac_mute_en(onboard_spk, 0))
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, play pipeline unmute fail, %d \n", __func__, __LINE__);
        return BK_FAIL;
    }

    return BK_OK;
}

static bk_err_t play_pipeline_stop(void)
{
    BK_LOGI(AUDIO_PLAYER_TAG, "%s \n", __func__);

    if (BK_OK != audio_pipeline_stop(play_pipeline))
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, play pipeline stop fail, %d \n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_wait_for_stop(play_pipeline))
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, play pipeline wait stop fail, %d \n", __func__, __LINE__);
        return BK_FAIL;
    }

    return BK_OK;
}

static bk_err_t play_pipeline_change_frame_info(int rate, int ch, int bits)
{
    BK_LOGI(AUDIO_PLAYER_TAG, "%s \n", __func__);

    if (BK_OK != onboard_speaker_stream_set_param(onboard_spk, rate, bits, ch))
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, change frame info fail, %d \n", __func__, __LINE__);
        return BK_FAIL;
    }
    else
    {
        BK_LOGI(AUDIO_PLAYER_TAG, "change frame info ok \n");
    }

    return BK_OK;
}

static bk_err_t play_pipeline_set_volume(int volume)
{
    BK_LOGI(AUDIO_PLAYER_TAG, "%s \n", __func__);

    if (BK_OK != onboard_speaker_stream_set_digital_gain(onboard_spk, volume))
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, set volume fail, %d \n", __func__, __LINE__);
        return BK_FAIL;
    }
    else
    {
        BK_LOGI(AUDIO_PLAYER_TAG, "set volume ok \n");
    }

    return BK_OK;
}

static bk_err_t write_spk_data(char *buffer, uint32_t size)
{
    return raw_stream_write(raw_write, buffer, size);
}

static int device_sink_open(audio_sink_type_t sink_type, void *param, bk_audio_player_sink_t **sink_pp)
{
    bk_audio_player_sink_t *sink;
    int ret;
    bk_audio_player_handle_t player;
    audio_info_t *info;
    int gain;
    device_sink_param_t *dev_param;

    if (sink_type != AUDIO_SINK_DEVICE)
    {
        return AUDIO_PLAYER_INVALID;
    }

    dev_param = (device_sink_param_t *)param;
    if (!dev_param || !dev_param->info || !dev_param->player)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, invalid device_sink_param\n", __func__);
        return AUDIO_PLAYER_ERR;
    }

    info = dev_param->info;
    player = dev_param->player;

    sink = audio_sink_new(sizeof(audio_info_t));
    if (!sink)
    {
        return AUDIO_PLAYER_NO_MEM;
    }

    audio_info_t *priv = (audio_info_t *)sink->sink_priv;

    priv->frame_size = info->frame_size;
    priv->sample_rate = info->sample_rate;
    priv->sample_bits = info->sample_bits;
    priv->channel_number = info->channel_number;
    priv->bps = info->bps;

    sink->info.sampRate = priv->sample_rate;
    sink->info.bitsPerSample = priv->sample_bits;
    sink->info.nChans = priv->channel_number;

    gain = player->spk_gain * 63 / 100;

    bk_pm_module_vote_cpu_freq(PM_DEV_ID_AUDIO, PM_CPU_FRQ_240M);

    ret = play_pipeline_open(info->sample_rate, info->sample_bits, info->channel_number, gain);
    if (ret != BK_OK)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, play pipeline open fail, ret:%d, %d \n", __func__, ret, __LINE__);
        return AUDIO_PLAYER_ERR;
    }

    ONBOARD_SPK_DEVICE_DEBUG_OPEN();

    /* first mute dac to avoid "pop" voice */
    ret = play_pipeline_mute();
    if (ret != BK_OK)
    {
        BK_LOGD(AUDIO_PLAYER_TAG, "%s, paly pipeline mute fail, ret: %d, %d \n", __func__, ret, __LINE__);
    }
    else
    {
        audio_dac_mute_sta = true;
    }

    ret = play_pipeline_start();
    if (ret != BK_OK)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, play pipeline start fail, ret:%d, %d \n", __func__, ret, __LINE__);
        return AUDIO_PLAYER_ERR;
    }

    *sink_pp = sink;

    BK_LOGE(AUDIO_PLAYER_TAG, "device_sink_open complete \n");

    return AUDIO_PLAYER_OK;
}

static int device_sink_close(bk_audio_player_sink_t *sink)
{
    bk_err_t ret = BK_OK;
    BK_LOGI(AUDIO_PLAYER_TAG, "%s \n", __func__);

    ret = play_pipeline_stop();
    if (ret != BK_OK)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, play pipeline stop fail, ret: %d, %d \n", __func__, ret, __LINE__);
    }

    ret = play_pipeline_close();
    if (ret != BK_OK)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, play pipeline close fail, ret: %d, %d \n", __func__, ret, __LINE__);
    }

    bk_pm_module_vote_cpu_freq(PM_DEV_ID_AUDIO, PM_CPU_FRQ_DEFAULT);

    BK_LOGI(AUDIO_PLAYER_TAG, "device_sink_close \n");
    return AUDIO_PLAYER_OK;
}

static int device_sink_write(bk_audio_player_sink_t *sink, char *buffer, int len)
{
    while (len)
    {
        ONBOARD_SPK_DEVICE_DEBUG_DATA(buffer, len);

        int write_len = write_spk_data(buffer, len);
        /* unmute dac after write data to audio dac */
        if (audio_dac_mute_sta)
        {
            int ret = play_pipeline_unmute();
            if (ret != BK_OK)
            {
                BK_LOGD(AUDIO_PLAYER_TAG, "%s, paly pipeline mute fail, ret: %d, %d \n", __func__, ret, __LINE__);
            }
            else
            {
                audio_dac_mute_sta = false;
            }
        }
        return write_len;
    }

    return len;
}


static int device_sink_control(bk_audio_player_sink_t *sink, audio_sink_control_t control)
{
    int ret = AUDIO_PLAYER_OK;
    if (control == AUDIO_SINK_PAUSE)
    {
        /* first mute audio dac, then pause audio pipeline */
        ret = play_pipeline_mute();
        if (ret != BK_OK)
        {
            BK_LOGD(AUDIO_PLAYER_TAG, "%s, paly pipeline mute fail, ret: %d, %d \n", __func__, ret, __LINE__);
        }
        else
        {
            audio_dac_mute_sta = true;
        }
        ret = play_pipeline_pause();
        if (ret != BK_OK)
        {
            BK_LOGD(AUDIO_PLAYER_TAG, "%s, paly pipeline pause fail, ret: %d, %d \n", __func__, ret, __LINE__);
        }
    }
    else if (control == AUDIO_SINK_RESUME)
    {
        /* first resume audio pipeline, then unmute audio dac */
        ret = play_pipeline_resume();
        if (ret != BK_OK)
        {
            BK_LOGD(AUDIO_PLAYER_TAG, "%s, paly pipeline resume fail, ret: %d, %d \n", __func__, ret, __LINE__);
        }
        ret = play_pipeline_unmute();
        if (ret != BK_OK)
        {
            BK_LOGD(AUDIO_PLAYER_TAG, "%s, paly pipeline mute fail, ret: %d, %d \n", __func__, ret, __LINE__);
        }
        else
        {
            audio_dac_mute_sta = false;
        }
    }
    else if (control == AUDIO_SINK_MUTE)
    {
        ret = play_pipeline_mute();
        if (ret != BK_OK)
        {
            BK_LOGD(AUDIO_PLAYER_TAG, "%s, paly pipeline mute fail, ret: %d, %d \n", __func__, ret, __LINE__);
        }
        else
        {
            audio_dac_mute_sta = true;
        }
    }
    else if (control == AUDIO_SINK_UNMUTE)
    {
        ret = play_pipeline_unmute();
        if (ret != BK_OK)
        {
            BK_LOGD(AUDIO_PLAYER_TAG, "%s, paly pipeline mute fail, ret: %d, %d \n", __func__, ret, __LINE__);
        }
        else
        {
            audio_dac_mute_sta = false;
        }
    }
    else if (control == AUDIO_SINK_FRAME_INFO_CHANGE)
    {
        ret = play_pipeline_change_frame_info(sink->info.sampRate, sink->info.nChans, sink->info.bitsPerSample);
        if (ret != BK_OK)
        {
            BK_LOGD(AUDIO_PLAYER_TAG, "%s, paly pipeline change frame info fail, ret: %d, %d \n", __func__, ret, __LINE__);
        }
    }
    else if (control == AUDIO_SINK_SET_VOLUME)
    {
        ret = play_pipeline_set_volume(sink->info.volume);
        if (ret != BK_OK)
        {
            BK_LOGD(AUDIO_PLAYER_TAG, "%s, paly pipeline set volume fail, ret: %d, %d \n", __func__, ret, __LINE__);
        }
    }
    else
    {
        //nothing to do
    }

    return ret;
}

const bk_audio_player_sink_ops_t device_sink_ops =
{
    .open =    device_sink_open,
    .write =   device_sink_write,
    .control = device_sink_control,
    .close =   device_sink_close,
};

/* Get onboard speaker sink operations structure */
const bk_audio_player_sink_ops_t *bk_audio_player_get_onboard_speaker_sink_ops(void)
{
    return &device_sink_ops;
}
