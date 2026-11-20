#ifndef AGORA_RTC_AGENT_H
#define AGORA_RTC_AGENT_H

#include <stddef.h>
#include <stdint.h>
#include <components/bk_audio/audio_pipeline/audio_types.h>
#include "media_types.h"

typedef enum{
    AGORA_RTC_AGENT_STA_IDLE           = 0,
    AGORA_RTC_AGENT_STA_STARTING       = 1,
    AGORA_RTC_AGENT_STA_RUNNING        = 2,
    AGORA_RTC_AGENT_STA_STOPPING       = 3,
    AGORA_RTC_AGENT_STA_STOPPED        = 4,
    AGORA_RTC_AGENT_STA_RECOVERING     = 5,
    AGORA_RTC_AGENT_STA_FAILED         = 6,
}agora_rtc_agent_status_e;

typedef struct 
{
    char  *app_id;
    char  *app_cert;
    char  *customer_key;
    char  *customer_secret;
}agora_rtc_cred;


typedef struct{
    float          threshold;
    uint32_t       prefix_padding_ms;
    uint32_t       silence_duration_ms;
    uint32_t       interrupt_duration_ms;
}agora_rtc_vad_config;

typedef struct{
    char          *api_key;
    char          *model;
    uint32_t       sample_rate;
    char          *voice;
}cosyvoice_tts_cofig;

typedef struct{
    uint32_t      user_id;

    char         *llm_vendor;
    char         *llm_url;
    char         *llm_key;
    char         *prompt;
    char         *greeting_msg;
    char         *failure_msg;

    char         *asr_vendor;
    void         *asr_vendor_params;

    char         *tts_vendor;
    void         *tts_vendor_params;

    agora_rtc_vad_config  vad_cfg;

}agora_rtc_agent_config_t;

typedef enum
{
    AGORA_RTC_AGENT_CREATE            = 1,
    AGORA_RTC_AGENT_STOP              = 2,
    AGORA_RTC_AGENT_STATUS            = 3,
    AGORA_RTC_AGENT_LIST              = 4,
}agora_rtc_agent_event_e;

#define VIDEO_FRAME_INTERVAL            500
#define BANDWIDTH_ESTIMATE_MAX_BITRATE   (2000000)

// DISABLE
#define CONFIG_AUDIO_CODEC_TYPE         AUDIO_CODEC_DISABLED
#define CONFIG_PCM_FRAME_LEN            160
#define CONFIG_PCM_SAMPLE_RATE          8000
#define CONFIG_PCM_CHANNEL_NUM          1


typedef void(*agora_rtc_agent_status_cb)(uint8_t event, void *event_param);

typedef int (*agora_rtc_agent_audio_rx_cb)(const uint8_t *data, size_t size);

int    agora_rtc_chat_config(agora_rtc_cred *cred, char *channel_name);

int    agora_rtc_chat_agent_create(agora_rtc_agent_config_t *option, agora_rtc_agent_status_cb status_cb);

int    agora_rtc_chat_agent_start(agora_rtc_agent_audio_rx_cb  rx_cb);

int    agora_rtc_chat_agent_stop();

int    agora_rtc_chat_agent_update(void *update_info);

int    agora_rtc_audio_data_send(uint8_t *data_ptr, size_t data_len, audio_enc_type_t audio_type);

int    agora_rtc_video_data_send(frame_buffer_t *frame);

#endif