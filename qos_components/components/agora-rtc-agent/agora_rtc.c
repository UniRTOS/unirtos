
#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_nvitem.h"
#include "qosa_log.h"
#include "agora_rtc.h"

#define QOS_LOG_TAG LOG_TAG_DEMO

#define DEFAULT_SDK_LOG_PATH "io.agora.rtc_sdk"
#define CONFIG_AE_AUDIO_FRAME_DURATION_MS  20

static agora_rtc_t agora_rtc = {0};

/* ============================= Private Functions ============================= */

agora_rtc_t *__get_rtc_instance(void)
{
    return &agora_rtc;
}

static void __send_message_2_user(agora_rtc_t *rtc, agora_rtc_msg_t *msg)
{
    if (rtc->user_message_callback)
    {
        rtc->user_message_callback(msg);
    }
}

static void __rtc_started(agora_rtc_t *rtc)
{
    rtc->state = AGORA_RTC_STATE_WORKING;
}

static bool __is_rtc_started(agora_rtc_t *rtc)
{
    if (rtc->state == AGORA_RTC_STATE_WORKING)
    {
        return true;
    }
    else
    {
        return false;
    }
}

static bool __get_rtc_status(agora_rtc_t *rtc)
{
    return rtc->state;
}


static void __rtc_stopped(agora_rtc_t *rtc)
{
    rtc->state = AGORA_RTC_STATE_IDLE;
}

static void __deep_copy_items_destroy(agora_rtc_t *rtc);

static void __key_frame_req_msg(agora_rtc_t *rtc)
{
    agora_rtc_msg_t msg;
    msg.code = AGORA_RTC_MSG_KEY_FRAME_REQUEST;
    __send_message_2_user(rtc, &msg);
}

static void __change_encode_bitrate_req_msg(agora_rtc_t *rtc, uint32_t target_bps)
{
    agora_rtc_msg_t msg;
    msg.code =  AGORA_RTC_MSG_BWE_TARGET_BITRATE_UPDATE;
    msg.data.bwe.target_bitrate = target_bps;
    __send_message_2_user(rtc, &msg);
}

static void __register_message_router(agora_rtc_t *rtc, agora_rtc_msg_notify_cb message_callback)
{
    rtc->user_message_callback = message_callback;
}

static void __on_join_channel_success(connection_id_t conn_id, uint32_t uid, int elapsed_ms)
{
    QLOGD("conn_id: %d, uid: %d, elapsed_ms: %d \n", conn_id, uid, elapsed_ms);
    agora_rtc_t *rtc = __get_rtc_instance();

    rtc->b_channel_joined = true;
    agora_rtc_msg_t msg = { .code = AGORA_RTC_MSG_JOIN_CHANNEL_SUCCESS };
    __send_message_2_user(rtc, &msg);

    __rtc_started(rtc);
}

static void __error_msg(agora_rtc_t *rtc, agora_rtc_msg_e err)
{
    agora_rtc_msg_t msg;
    msg.code = err;
    __send_message_2_user(rtc, &msg);
}

/* ============================= Event Callbacks ============================= */

static void __on_error(connection_id_t conn_id, int code, const char *msg)
{
    QLOGE("Agora error: conn_id=%d, code=%d, msg=%s\n", conn_id, code, msg ? msg : "null");
    
    agora_rtc_t *rtc = __get_rtc_instance();
    switch (code)
    {
        case ERR_INVALID_APP_ID:
            __error_msg(rtc, AGORA_RTC_MSG_INVALID_APP_ID);
            break;
        case ERR_INVALID_CHANNEL_NAME:
            __error_msg(rtc, AGORA_RTC_MSG_INVALID_CHANNEL_NAME);
            break;
        case ERR_INVALID_TOKEN:
            __error_msg(rtc, AGORA_RTC_MSG_INVALID_TOKEN);
            break;
        case ERR_TOKEN_EXPIRED:
            __error_msg(rtc, AGORA_RTC_MSG_TOKEN_EXPIRED);
            break;
        default:
            QLOGW("Unknown error code: %d\n", code);
            break;
    }
}

static void __on_user_joined(connection_id_t conn_id, uint32_t uid, int elapsed_ms)
{
    QLOGD("conn_id: %d, uid: %d, elapsed_ms: %d \n", conn_id, uid, elapsed_ms);
    agora_rtc_t *rtc = __get_rtc_instance();

    rtc->b_user_joined = true;
    agora_rtc_msg_t msg =
    {
        .code     = AGORA_RTC_MSG_USER_JOINED,
        .data.uid = uid
    };
    __send_message_2_user(rtc, &msg);
}

static void __on_user_offline(connection_id_t conn_id, uint32_t uid, int reason)
{
    QLOGD("conn_id: %d, uid: %d, reason: %d \n", conn_id, uid, reason);
    agora_rtc_t *rtc = __get_rtc_instance();

    rtc->b_user_joined = false;
    agora_rtc_msg_t msg =
    {
        .code     = AGORA_RTC_MSG_USER_OFFLINE,
        .data.uid = uid
    };
    __send_message_2_user(rtc, &msg);
}

static void __on_key_frame_gen_req(connection_id_t conn_id, uint32_t uid, video_stream_type_e stream_type)
{
    QLOGD("Key frame request: conn_id=%d, uid=%u, stream_type=%d\n", conn_id, uid, stream_type);
    
    agora_rtc_t *rtc = __get_rtc_instance();
    __key_frame_req_msg(rtc);
}

static void __on_audio_data(connection_id_t conn_id, uint32_t uid, uint16_t sent_ts,
                            const void *data_ptr, size_t data_len, const audio_frame_info_t *info_ptr)
{
    agora_rtc_t *rtc = __get_rtc_instance();
    if (!__is_rtc_started(rtc) || !rtc->b_channel_joined || !rtc->b_user_joined)
    {
        return;
    }
    
    if (rtc->audio_rx_data_handle)
    {
        rtc->audio_rx_data_handle((unsigned char *)data_ptr, data_len, info_ptr);
    }
}

static void __on_video_data(connection_id_t conn_id, uint32_t uid, uint16_t sent_ts, 
                            const void *data_ptr, size_t data_len, const video_frame_info_t *info_ptr)
{
    agora_rtc_t *rtc = __get_rtc_instance();
    if (!__is_rtc_started(rtc) || !rtc->b_channel_joined || !rtc->b_user_joined)
    {
        QLOGI("Wrong rtc state, rtc_started: %s, channel_joined: %s, user_joined: %s \n",
            __is_rtc_started(rtc) ? "true" : "false", rtc->b_channel_joined ? "true" : "false", rtc->b_user_joined ? "true" : "false");
        return;
    }
    
    QLOGD("ch=%s, uid=%d sent_ts=%u codec=%u, data_ptr=%p, data_len=%d",
        rtc->agora_rtc_option.p_channel_name, (int)uid, sent_ts, info_ptr->data_type, data_ptr, (int)data_len);

    if (rtc->video_rx_data_handle)
    {
        rtc->video_rx_data_handle((const uint8_t *)data_ptr, data_len, info_ptr);
    }
    else
    {
        QLOGD("aud_rx_data_handle is NULL \n");
    }
}

static void __on_user_mute_audio(connection_id_t conn_id, uint32_t uid, bool muted)
{
    QLOGI("User mute audio: conn_id=%u, uid=%u, muted=%d\n", conn_id, uid, muted);  // Disabled: event log
}

static void __on_user_mute_video(connection_id_t conn_id, uint32_t uid, bool muted)
{
     QLOGI("User mute video: conn_id=%u, uid=%u, muted=%d\n", conn_id, uid, muted);  // Disabled: event log
}

static void __on_target_bitrate_changed(connection_id_t conn_id, uint32_t target_bps)
{
    agora_rtc_t *rtc = __get_rtc_instance();
    if (!__is_rtc_started(rtc))
    {
        // LOGI("Pipeline not started. Skip bitrate changed process\n");  // Disabled: too verbose
        return;
    }
    
    QLOGD("Target bitrate changed: conn_id=%d, from %u to %u bps\n", 
         conn_id, rtc->target_bitrate, target_bps);
    
    rtc->target_bitrate = target_bps;
    __change_encode_bitrate_req_msg(rtc, target_bps);
}

static void __on_connection_lost(connection_id_t conn_id)
{
    QLOGE("Connection lost: conn_id=%d\n", conn_id);
    
    agora_rtc_t *rtc = __get_rtc_instance();
    rtc->b_channel_joined = false;
    
    agora_rtc_msg_t msg = {
        .code = AGORA_RTC_MSG_CONNECTION_LOST
    };
    __send_message_2_user(rtc, &msg);
}

static void __on_rejoin_channel_success(connection_id_t conn_id, uint32_t uid, int elapsed_ms)
{
    QLOGI("Rejoin channel success: conn_id=%d, uid=%u, elapsed_ms=%d\n", conn_id, uid, elapsed_ms);
    
    agora_rtc_t *rtc = __get_rtc_instance();
    rtc->b_channel_joined = true;
    
    agora_rtc_msg_t msg = { .code = AGORA_RTC_MSG_REJOIN_CHANNEL_SUCCESS };
    __send_message_2_user(rtc, &msg);
}

static void __register_agora_rtc_event_handler(agora_rtc_t *rtc)
{
    rtc->agora_rtc_event_handler.on_join_channel_success = __on_join_channel_success;
    rtc->agora_rtc_event_handler.on_error = __on_error;
    rtc->agora_rtc_event_handler.on_user_joined = __on_user_joined;
    rtc->agora_rtc_event_handler.on_user_offline = __on_user_offline;
    rtc->agora_rtc_event_handler.on_key_frame_gen_req = __on_key_frame_gen_req;
    rtc->agora_rtc_event_handler.on_audio_data = __on_audio_data;
    rtc->agora_rtc_event_handler.on_video_data = __on_video_data;
    rtc->agora_rtc_event_handler.on_target_bitrate_changed = __on_target_bitrate_changed;
    rtc->agora_rtc_event_handler.on_connection_lost = __on_connection_lost;
    rtc->agora_rtc_event_handler.on_rejoin_channel_success = __on_rejoin_channel_success;
    rtc->agora_rtc_event_handler.on_user_mute_audio = __on_user_mute_audio;
    rtc->agora_rtc_event_handler.on_user_mute_video = __on_user_mute_video;
}

static void __deep_copy_items_destroy(agora_rtc_t *rtc)
{
    if (rtc->agora_rtc_option.p_channel_name != QOSA_NULL)
    {
        qosa_free((void *)rtc->agora_rtc_option.p_channel_name);
        rtc->agora_rtc_option.p_channel_name = QOSA_NULL;
    }

    if (rtc->agora_rtc_option.p_token != QOSA_NULL)
    {
        qosa_free((void *)rtc->agora_rtc_option.p_token);
        rtc->agora_rtc_option.p_token = QOSA_NULL;
    }
}



static int32_t __agora_init(agora_rtc_config_t *p_config)
{
    int rval;
    uint32_t bwe_min_bps = 500000;
    uint32_t bwe_max_bps = 2000000;
    uint32_t bwe_start_bps = 800000;
    rtc_service_option_t service_opt = { 0 };

    agora_rtc_t *rtc = __get_rtc_instance();

    rtc->state = AGORA_RTC_STATE_NULL;
    rtc->audio_rx_data_handle = QOSA_NULL;
    rtc->conn_id = (connection_id_t)0;
    rtc->user_message_callback = QOSA_NULL;
    rtc->b_user_joined = QOSA_FALSE;
    rtc->b_channel_joined = QOSA_FALSE;
    rtc->agora_rtc_event_handler.on_audio_data = QOSA_NULL;
    rtc->agora_rtc_event_handler.on_connection_lost = QOSA_NULL;
    rtc->agora_rtc_event_handler.on_error = QOSA_NULL;
    rtc->agora_rtc_event_handler.on_join_channel_success = QOSA_NULL;
#ifdef CONFIG_RTCM
    rtc->agora_rtc_event_handler.on_media_ctrl_receive = QOSA_NULL;
#endif
    rtc->agora_rtc_event_handler.on_mixed_audio_data = QOSA_NULL;
    rtc->agora_rtc_event_handler.on_rejoin_channel_success = QOSA_NULL;
    rtc->agora_rtc_event_handler.on_token_privilege_will_expire = QOSA_NULL;
    rtc->agora_rtc_event_handler.on_user_joined = QOSA_NULL;
    rtc->agora_rtc_event_handler.on_user_mute_audio = QOSA_NULL;
    rtc->agora_rtc_event_handler.on_user_mute_video = QOSA_NULL;
    rtc->agora_rtc_event_handler.on_video_data = QOSA_NULL;
    rtc->agora_rtc_event_handler.on_target_bitrate_changed = QOSA_NULL;
    rtc->agora_rtc_event_handler.on_key_frame_gen_req = QOSA_NULL;
    rtc->agora_rtc_event_handler.on_user_offline = QOSA_NULL;
    rtc->target_bitrate = 0;
    rtc->audio_rx_data_handle = QOSA_NULL;
    rtc->video_rx_data_handle = QOSA_NULL;

    rtc->agora_rtc_config.license[0] = '\0';
    rtc->agora_rtc_config.enable_bwe_param = QOSA_TRUE;
    rtc->agora_rtc_config.bwe_param_max_bps = bwe_max_bps;
    rtc->agora_rtc_config.log_disable = QOSA_TRUE;
    rtc->agora_rtc_config.area_code = AREA_CODE_GLOB;
    QLOGI("Agora RTC SDK area_code: %d\n", rtc->agora_rtc_config.area_code);


    rtc->agora_rtc_config.p_appid = (char *)qosa_malloc(qosa_strlen(p_config->p_appid) + 1);
    if (rtc->agora_rtc_config.p_appid == QOSA_NULL)
    {
        QLOGE("malloc app_id fail, size: %d \n", (qosa_strlen(p_config->p_appid) + 1));
        goto agora_init_error;
    }
    qosa_strcpy((char *)rtc->agora_rtc_config.p_appid, p_config->p_appid);
    QLOGI("Agora RTC SDK app_id: %s \n", rtc->agora_rtc_config.p_appid);

    qosa_memcpy(rtc->agora_rtc_config.license, p_config->license, 32);
    QLOGI("Agora RTC SDK license: ");
    for (int n = 0; n < 32; n++)
    {
        QLOGI("%c", rtc->agora_rtc_config.license[n]);
    }
    QLOGI("\n");

    rtc->agora_rtc_config.enable_bwe_param = p_config->enable_bwe_param;
    rtc->agora_rtc_config.bwe_param_max_bps = p_config->bwe_param_max_bps;
    rtc->agora_rtc_config.log_disable = p_config->log_disable;
    rtc->agora_rtc_config.area_code = p_config->area_code;

    QLOGI("Agora RTC SDK enable_bwe_param: %d\n", rtc->agora_rtc_config.enable_bwe_param);
    QLOGI("Agora RTC SDK bwe_param_max_bps: %d\n", rtc->agora_rtc_config.bwe_param_max_bps);
    QLOGI("Agora RTC SDK log_disable: %d\n", rtc->agora_rtc_config.log_disable);
    QLOGI("Agora RTC SDK area_code: %d\n", rtc->agora_rtc_config.area_code);


    if (rtc->agora_rtc_config.enable_bwe_param)
    {
        bwe_max_bps = p_config->bwe_param_max_bps;
    }
    else
    {
        rtc->agora_rtc_config.bwe_param_max_bps = bwe_max_bps;
    }

    // step1. agora sdk version.
    QLOGI("Agora RTC SDK v%s \n", agora_rtc_get_version());
    //step4. register event handler
    __register_agora_rtc_event_handler(rtc);

    // return BK_OK;

    //step6. agora rtc init.
    service_opt.area_code = p_config->area_code;
    service_opt.log_cfg.log_disable = p_config->log_disable;
    service_opt.log_cfg.log_path = DEFAULT_SDK_LOG_PATH;
    service_opt.log_cfg.log_level = RTC_LOG_WARNING;
    qosa_memcpy(service_opt.license_value, p_config->license, 33);

    QLOGI("Agora RTC SDK area_code: %d\n", service_opt.area_code);
    QLOGI("Agora RTC SDK log_disable: %d\n", service_opt.log_cfg.log_disable);
    QLOGI("Agora RTC SDK log_path: %s\n", service_opt.log_cfg.log_path);
    QLOGI("Agora RTC SDK log_level: %d\n", service_opt.log_cfg.log_level);
    QLOGI("Agora RTC SDK license_value: %s\n", service_opt.license_value);
    QLOGI("Agora RTC SDK p_appid: %s\n", rtc->agora_rtc_config.p_appid);


    // return BK_OK;

    rval = agora_rtc_init(rtc->agora_rtc_config.p_appid, &rtc->agora_rtc_event_handler, &service_opt);
    if (rval < 0)
    {
        QLOGI("agora rtc init failed, rval=%d error=%s\n", rval, agora_rtc_err_2_str(rval));
        goto agora_init_error;
    }

    return QOSA_OK;
    
    QLOGI("BWE[%u,%u,%u]\n", bwe_min_bps, bwe_max_bps, bwe_start_bps);
    rval = agora_rtc_set_bwe_param(CONNECTION_ID_ALL, bwe_min_bps, bwe_max_bps, bwe_start_bps);
    if (rval < 0)
    {
        QLOGI("set BWE failed \n");
    }
    else
    {
        QLOGI("agora_rtc_set_bwe_param ok \n");
    }

    return QOSA_OK;

agora_init_error:
    if (rtc->agora_rtc_config.p_appid != QOSA_NULL)
    {
        qosa_free((void *)rtc->agora_rtc_config.p_appid);
        rtc->agora_rtc_config.p_appid = QOSA_NULL;
    }

    return QOSA_ERROR_GENERAL;
}



bk_err_t __agora_rtc_create(agora_rtc_config_t *p_config, agora_rtc_msg_notify_cb message_callback)
{
    if (0 != __agora_init(p_config))
    {
        QLOGE("__agora_init fail.\n");
        return QOSA_ERROR_GENERAL;
    }
    else
    {
        QLOGI("__agora_init ok.\n");
    }

    agora_rtc_t *rtc = __get_rtc_instance();
    if (rtc)
    {
        __register_message_router(rtc, message_callback);
    }
    else
    {
        return QOSA_ERROR_GENERAL;
    }
    QLOGI("rtc create successfully.\n");
    return QOSA_OK;
}

bk_err_t __agora_rtc_destroy(void)
{
    int rval = agora_rtc_fini();
    if (rval != 0)
    {
        QLOGE("agora_rtc_fini failed: %d, %s\n", rval, agora_rtc_err_2_str(rval));
    }
    
    agora_rtc_t *rtc = __get_rtc_instance();
    
    if (rtc->agora_rtc_config.p_appid != QOSA_NULL)
    {
        qosa_free((void *)rtc->agora_rtc_config.p_appid);
        rtc->agora_rtc_config.p_appid = QOSA_NULL;
    }
    
    rtc->state = AGORA_RTC_STATE_NULL;
    
    QLOGI("Agora RTC destroyed successfully\n");
    return QOSA_OK;
}


int __agora_rtc_start(agora_rtc_option_t *option)
{
    int rval = 0;
    agora_rtc_t *rtc = __get_rtc_instance();
    if (!rtc)
    {
        return QOSA_ERROR_GENERAL;
    }

    /*copy channel name */
    if (rtc->agora_rtc_option.p_channel_name)
    {
        qosa_free((void *)rtc->agora_rtc_option.p_channel_name);
        rtc->agora_rtc_option.p_channel_name = QOSA_NULL;
    }
    rtc->agora_rtc_option.p_channel_name = (char *)qosa_malloc(qosa_strlen(option->p_channel_name) + 1);
    if (rtc->agora_rtc_option.p_channel_name == QOSA_NULL)
    {
        QLOGE("malloc channel_name fail, size: %d \n", (qosa_strlen(option->p_channel_name) + 1));
        goto agora_rtc_start_fail;
    }
    qosa_strcpy((void *)rtc->agora_rtc_option.p_channel_name, option->p_channel_name);
    QLOGI("Agora RTC SDK channel_name: %s \n", rtc->agora_rtc_option.p_channel_name);

    /*copy token */
    if (option->p_token)
    {
        if (rtc->agora_rtc_option.p_token)
        {
            qosa_free((void *)rtc->agora_rtc_option.p_token);
            rtc->agora_rtc_option.p_token = QOSA_NULL;
        }
        rtc->agora_rtc_option.p_token = (char *)qosa_malloc(qosa_strlen(option->p_token) + 1);
        if (rtc->agora_rtc_option.p_token == QOSA_NULL)
        {
            QLOGE("malloc token fail, size: %d \n", (qosa_strlen(option->p_token) + 1));
            goto agora_rtc_start_fail;
        }
        qosa_strcpy((void *)rtc->agora_rtc_option.p_token, option->p_token);
    }
    else
    {
        rtc->agora_rtc_option.p_token = QOSA_NULL;
    }
    QLOGI("Agora RTC SDK token: %s \n", rtc->agora_rtc_option.p_token);

    rtc->agora_rtc_option.uid = option->uid;
    rtc->agora_rtc_option.auto_subscribe_audio = option->auto_subscribe_audio;
    rtc->agora_rtc_option.auto_subscribe_video = option->auto_subscribe_video;
    rtc->agora_rtc_option.audio_config.audio_data_type = option->audio_config.audio_data_type;//AUDIO_DATA_TYPE_PCMU;
    rtc->agora_rtc_option.audio_config.pcm_sample_rate = option->audio_config.pcm_sample_rate;
    rtc->agora_rtc_option.audio_config.pcm_channel_num = option->audio_config.pcm_channel_num;

    rtc_channel_options_t channel_options = { 0 };

    channel_options.auto_subscribe_audio = rtc->agora_rtc_option.auto_subscribe_audio;
    channel_options.auto_subscribe_video = rtc->agora_rtc_option.auto_subscribe_video;
    channel_options.audio_codec_opt.audio_codec_type = option->audio_config.audio_data_type;
    //channel_options.audio_codec_opt.audio_codec_type = AUDIO_CODEC_DISABLED;
    channel_options.audio_codec_opt.pcm_sample_rate = option->audio_config.pcm_sample_rate;
    channel_options.audio_codec_opt.pcm_channel_num = option->audio_config.pcm_channel_num;
    channel_options.audio_codec_opt.pcm_duration = CONFIG_AE_AUDIO_FRAME_DURATION_MS;

    // open jitter buffer
    if (channel_options.audio_codec_opt.audio_codec_type != AUDIO_CODEC_DISABLED)
    {
        channel_options.enable_audio_jitter_buffer = QOSA_TRUE;
    }

    QLOGI("auto_subscribe_audio: %d, auto_subscribe_video: %d \n",
         channel_options.auto_subscribe_audio ? 1 : 0, channel_options.auto_subscribe_video ? 1 : 0);
    QLOGI("audio_codec_type: %d, pcm_sample_rate: %d, pcm_channel_num: %d, pcm_duration: %d\n",
         channel_options.audio_codec_opt.audio_codec_type, channel_options.audio_codec_opt.pcm_sample_rate, channel_options.audio_codec_opt.pcm_channel_num, channel_options.audio_codec_opt.pcm_duration);

    // step8. join channel
    rval = agora_rtc_create_connection(&(rtc->conn_id));
    if (0 != rval)
    {
        QLOGI("agora_rtc_create_connection failure: %d, %s \n", rval, agora_rtc_err_2_str(rval));
        goto agora_rtc_start_fail;
    }

    QLOGI("Agora RTC SDK uid: %d \n", rtc->agora_rtc_option.uid);
    rval = agora_rtc_join_channel(rtc->conn_id, rtc->agora_rtc_option.p_channel_name, rtc->agora_rtc_option.uid, rtc->agora_rtc_option.p_token, &channel_options);
    if (rval < 0)
    {
        QLOGI("join channel %s failed, rval=%d error=%s \n",
             rtc->agora_rtc_option.p_channel_name ? rtc->agora_rtc_option.p_channel_name : "", rval, agora_rtc_err_2_str(rval));
        goto agora_rtc_start_fail;
    }

    QLOGI("Joining channel %s ... \n", rtc->agora_rtc_option.p_channel_name);
    return QOSA_OK;

agora_rtc_start_fail:
    __deep_copy_items_destroy(rtc);
    return QOSA_ERROR_GENERAL;
}


int __agora_rtc_stop(void)
{
    int rval = 0;
    agora_rtc_t *rtc = __get_rtc_instance();
    
    if (!rtc || rtc->state == AGORA_RTC_STATE_NULL)
    {
        QLOGW("RTC not initialized\n");
        return QOSA_ERROR_GENERAL;
    }
    
    __rtc_stopped(rtc);
    
    // Leave channel
    rval = agora_rtc_leave_channel(rtc->conn_id);
    if (rval < 0)
    {
        QLOGE("agora_rtc_leave_channel failed, rval=%d error=%s\n", rval, agora_rtc_err_2_str(rval));
    }
    
    // Destroy connection
    rval = agora_rtc_destroy_connection(rtc->conn_id);
    if (rval < 0)
    {
        QLOGE("agora_rtc_destroy_connection failed, rval=%d error=%s\n", rval, agora_rtc_err_2_str(rval));
    }
    
    __deep_copy_items_destroy(rtc);
    
    QLOGI("Agora RTC stopped successfully\n");
    return QOSA_OK;
}

bk_err_t __agora_rtc_register_audio_rx_handle(agora_rtc_audio_rx_data_handle audio_rx_handle)
{
    agora_rtc_t *rtc = __get_rtc_instance();
    
    if (!rtc)
    {
        return QOSA_ERROR_GENERAL;
    }
    
    rtc->audio_rx_data_handle = audio_rx_handle;
    return QOSA_OK;
}

bk_err_t __agora_rtc_register_video_rx_handle(agora_rtc_video_rx_data_handle video_rx_handle)
{
    agora_rtc_t *rtc = __get_rtc_instance();
    
    if (!rtc)
    {
        return QOSA_ERROR_GENERAL;
    }
    
    rtc->video_rx_data_handle = video_rx_handle;
    return QOSA_OK;
}



