
#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_nvitem.h"
#include "qosa_log.h"
#include "mongoose.h"
#include "agora_http.h"
#include "mbedtls/base64.h"
#include "agora_rtc.h"
#include "agora_rtc_agent.h"
#include "agora_token.h"
// #include "driver/h264.h"

#define QOS_LOG_TAG LOG_TAG_DEMO


#define  AGORA_PROJECT_URL  "https://api.agora.io/cn/api/conversational-ai-agent/v2/projects/"

#define  AGORA_AGENT_KEY    "agora_agent"

#define create_bot_request "{\"name\":\"%s\",\"properties\":{"               \
                            "\"channel\":\"%s\","                            \
                            "\"agent_rtc_uid\":\"%d\","                      \
                            "\"remote_rtc_uids\":[\"*\"],"                   \
                            "\"enable_string_uid\":false,"                   \
                            "\"parameters\":{\"enable_dump\":true,\"transcript\":{\"enable_words\": true},\"output_audio_codec\":\"g722\",\"audio_scenario\":\"aiserver\",\"agent_rtc_sdk_params\": \"{}\"}," \
                            "\"idle_timeout\":%d,"                            \
                            "\"advanced_features\":{\"enable_aivad\":true},"  \
                            "\"llm\":{%s},"                                   \
                            "\"asr\":{%s},"                                   \
                            "\"tts\":{%s},"                                   \
                            "\"turn_detection\":{%s},"                                   \
                            "\"filler_words\": {},"                           \
                            "\"token\":\"%s\"}}"                                


#define llm_deepseek_config   "\"url\":\"%s\","                                                                          \
                             "\"vendor\": \"deepseek\","                                                                 \
                             "\"api_key\":\"%s\","                                                                       \
                             "\"system_messages\":[{\"role\":\"system\",\"content\": \"%s\"}],"  \
                             "\"greeting_message\": \"%s\","                                           \
                             "\"failure_message\": \"%s\","                                        \
                             "\"max_history\":32,"                                                                       \
                             "\"params\":{\"model\":\"deepseek-chat\",\"top_p\": 0.8,\"temperature\": 0.8}" 

        
#define asr_fengming_config     "\"language\":\"zh-CN\""

#define tts_bytedance_config   "\"vendor\":\"bytedance_duplex\","                                                                                \
                               "\"skip_patterns\":[],"                                                                                           \
                               "\"params\":{\"api_key\":\"%s\",\"speaker\":\"zh_female_shuangkuaisisi_moon_bigtts\",\"resource_id\": \"11ebdacf15d54c2fbfc4e7e25deeb09c\"}"

#define ttys_cosyvoice_config   "\"vendor\":\"cosyvoice\","  \
                                "\"params\":{\"api_key\":\"%s\",\"model\":\"%s\",\"sample_rate\": %d,\"voice\":\"%s\"}"    

#define vad_config             "\"threshold\":%.1f,\"prefix_padding_ms\":%d,\"silence_duration_ms\":%d,\"interrupt_duration_ms\":%d"

#define turn_detection_config  "\"config\":{\"speech_threshold\":0.5,\"start_of_speech\":{\"mode\":\"keywords\",\"keywords_config\":{\"interrupt_duration_ms\": 160," \
                                "\"prefix_padding_ms\": 500,\"triggered_keywords\": [\"你好\", \"在吗\", \"hello\"]}}," \
                                "\"end_of_speech\":{\"mode\":\"vad\",\"vad_config\":{\"silence_duration_ms\": 640}}}"

enum agora_chat_agent_operation_e{
    CREATE_AGENT   =  1,
    STOP_AGENT     =  2,
    UPDATE_AGENT   =  3,
    STATUS_AGENT   =  4,
    LIST_ACTIVE_AGENT = 5,
};

struct agora_chat_agent_data{
  char               session_url[256];
  uint8_t            op_code;
  char              *op_data;
  uint32_t           data_len;
};


struct agora_chat_agent_t{
  char                        channel_name[129];
  char                       *channel_token; 
  uint32_t                    channel_uid;
  char                        agent_id[33]; 
  bool                        connected_flag;
  bool                        agent_offline;
  agora_rtc_agent_status_cb   status_cb;
  agora_rtc_agent_audio_rx_cb audio_rx_cb;
};

static struct agora_chat_agent_t  agora_agent = {0};
static agora_rtc_cred  agora_cred = {0};


static agora_rtc_config_t agora_rtc_config = DEFAULT_AGORA_RTC_CONFIG();
static agora_rtc_option_t agora_rtc_option = DEFAULT_AGORA_RTC_OPTION();

static qosa_task_t  agora_rtc_task = QOSA_NULL;
static qosa_sem_t   agora_rtc_sem = QOSA_NULL;
static bool agora_runing = QOSA_FALSE;

static uint8_t agora_audio_codec_type_mapping(audio_enc_type_t codec_type)
{
    uint8_t aud_data_type = AUDIO_DATA_TYPE_GENERIC;
    
    switch(codec_type)
    {
        case AUDIO_ENC_TYPE_G711A:
            aud_data_type = AUDIO_DATA_TYPE_PCMA;
            break;
        case AUDIO_ENC_TYPE_G711U:
            aud_data_type = AUDIO_DATA_TYPE_PCMU;
            break;
        case AUDIO_ENC_TYPE_PCM:
            aud_data_type = AUDIO_DATA_TYPE_PCM;
            break;
        case AUDIO_ENC_TYPE_G722:
            aud_data_type = AUDIO_DATA_TYPE_G722;
            break;
        case AUDIO_ENC_TYPE_OPUS:
            aud_data_type = AUDIO_DATA_TYPE_OPUS;
            break;
        default:
            QLOGE("Unknown codec type:%d\n", codec_type);
            break;
    }
    
    return aud_data_type;
}

static int agora_chat_agent_status(char *status)
{
    QOSA_ASSERT(status != NULL);
    if(qosa_strcmp(status, "FAILED") == 0)
    {
        return AGORA_RTC_AGENT_STA_FAILED;
    }
    else if(qosa_strcmp(status, "STARTING") == 0)
    {
        return AGORA_RTC_AGENT_STA_STARTING;
    }
    else if(qosa_strcmp(status, "RUNNING") == 0)
    {
        return AGORA_RTC_AGENT_STA_RUNNING;
    }
    else if(qosa_strcmp(status, "STOPPING") == 0)
    {
        return AGORA_RTC_AGENT_STA_STOPPING;
    }
    else if(qosa_strcmp(status, "STOPPED") == 0)
    {
        return AGORA_RTC_AGENT_STA_STOPPED;
    }
    else if(qosa_strcmp(status, "RECOVERING") == 0)
    {
        return AGORA_RTC_AGENT_STA_RECOVERING;
    }
    else
    {
        return AGORA_RTC_AGENT_STA_IDLE;
    }
}

static int agora_chat_agent_request_data(uint8_t **data, uint32_t  *len, void *cb_param)
{
    struct agora_chat_agent_data  *bot_data = (struct agora_chat_agent_data *)cb_param;
    char auth_param[150] = {0};
    size_t auth_len = 0;
    size_t request_len = 0;
    struct mg_str host = mg_url_host(bot_data->session_url);
    char *method = NULL;
    auth_len = qosa_snprintf(auth_param, sizeof(auth_param), "%s:%s", agora_cred.customer_key, agora_cred.customer_secret);
   
    size_t rem = auth_len % 3;
    size_t base64_auth_len = ((auth_len / 3) * 4) + (rem > 0 ? 4 : 0) + 1;
    unsigned char *auth_base64 = (unsigned char *)qosa_malloc(base64_auth_len + 1);
    mbedtls_base64_encode(auth_base64, base64_auth_len, &base64_auth_len, (const unsigned char *)auth_param, auth_len);

    request_len = 512+bot_data->data_len;
    *data = qosa_malloc(300+bot_data->data_len);
    BK_ASSERT(*data != NULL);

    if(bot_data->op_code == STATUS_AGENT || bot_data->op_code == LIST_ACTIVE_AGENT)
    {
        method = "GET";
    }
    else
    {
        method = "POST";
    }
    if(bot_data->op_data != NULL && bot_data->data_len > 0)
    {
        *len = qosa_snprintf((char *)(*data), request_len, "%s %s HTTP/1.1\r\n"
                "Host: %.*s\r\n"
                "Content-Type: application/json\r\n"
                "Authorization: Basic %s\r\n"
                "Content-Length: %d\r\n"
                "\r\n%s",
                method, mg_url_uri(bot_data->session_url),(int) host.len, host.ptr, auth_base64, bot_data->data_len, bot_data->op_data);
    }
    else
    {
        *len = qosa_snprintf((char *)(*data), request_len, "%s %s HTTP/1.1\r\n"
                "Host: %.*s\r\n"
                "Authorization: Basic %s\r\n"
                "\r\n%s",
                method, mg_url_uri(bot_data->session_url),(int) host.len, host.ptr, auth_base64);

    }
    QLOGI("request:\n%s\n", (char *)(*data));
    qosa_free(auth_base64);
    if(bot_data->op_data != NULL){
        qosa_free(bot_data->op_data);
        bot_data->op_data = NULL;
    }
    return 0;
}

static int  agora_chat_agent_respone_data(const char *data, uint32_t  len, void *cb_param)
{
    struct agora_chat_agent_data  *bot_data = (struct agora_chat_agent_data *)cb_param;
    QLOGI("%s:%d[%p/%p]\n", __func__,__LINE__,bot_data, bot_data->op_data);
    //debug
    if(len > 0 && data != NULL)
      QLOGI("%.*s\n", len, data);
    if(bot_data->op_code == STOP_AGENT)
    {
       //notify stop bot success
       
    }
    else if(bot_data->op_code == STATUS_AGENT)
    {
       QOSA_ASSERT(data != NULL && len > 0);
       struct mg_str  response = {.ptr = (const char *)data, .len = len};
       char *status = mg_json_get_str(response, "$.status");
       QLOGI("status:%s\n", status);
       int status_val = agora_chat_agent_status(status);
       qosa_free(status);
       if(agora_agent.status_cb != NULL)
       {
          agora_agent.status_cb(AGORA_RTC_AGENT_STATUS, (void *)status_val);
       }
    }
    else if(bot_data->op_code == LIST_ACTIVE_AGENT)
    {
       QOSA_ASSERT(data != NULL && len > 0);
       struct mg_str  response = {.ptr = (const char *)data, .len = len};
       long count = 0;
       count = mg_json_get_long(response, "$.data.count", count);
       QLOGI("count:%d\n", count);
       if(agora_agent.status_cb != NULL)
       {
          agora_agent.status_cb(AGORA_RTC_AGENT_LIST, (void *)(int)count);
       }  
    }
    else if(bot_data->op_code == CREATE_AGENT)
    {
       QOSA_ASSERT(data != NULL && len > 0);
       struct mg_str  response = {.ptr = (const char *)data, .len = len};
       char *agent_id = mg_json_get_str(response, "$.agent_id");

       QLOGI("agent_id:%s\n", agent_id);
       //save agent id
       qosa_memset(agora_agent.agent_id, 0x00, sizeof(agora_agent.agent_id));
       qosa_strcpy(agora_agent.agent_id, agent_id);
       qosa_nv_cfg_item_data_t agent_info[] = {
        NV_ITEM_CFG_DEF("channel_name", agora_agent.channel_name),   /*!< SIM card status URC enable configuration */
        NV_ITEM_CFG_DEF("agent_id", agora_agent.agent_id),
       };
       qosa_nv_item_json_write(AGORA_AGENT_KEY, agent_info, QOSA_ARRAY_SIZE(agent_info));
       qosa_free(agent_id);  
       if(agora_agent.status_cb != NULL)
       {
          int result = 0;
          agora_agent.status_cb(AGORA_RTC_AGENT_CREATE, (void *)result);
       }  
    }
    else
    {

    }
    
    if(bot_data->op_data != NULL){
        qosa_free(bot_data->op_data);
        bot_data->op_data != NULL;
    }
    qosa_free(bot_data);
    return 0;
}

static void  agora_chat_agent_exception_cb(void *cb_param)
{
    struct agora_chat_agent_data  *bot_data = (struct agora_chat_agent_data *)cb_param;
    int result = -1;
    QLOGI("%s:%d[%p/%p]\n", __func__,__LINE__,bot_data, bot_data->op_data);
    if(bot_data->op_code == STOP_AGENT)
    {
       //notify stop bot failure
       
    }
    else if(bot_data->op_code == STATUS_AGENT)
    {
       if(agora_agent.status_cb != NULL)
       {
          agora_agent.status_cb(AGORA_RTC_AGENT_STATUS, (void *)result);
       }
    }
    else if(bot_data->op_code == LIST_ACTIVE_AGENT)
    {
       if(agora_agent.status_cb != NULL)
       {
          int count = 0;
          agora_agent.status_cb(AGORA_RTC_AGENT_LIST, (void *)count);
       }  
    }
    else if(bot_data->op_code == CREATE_AGENT)
    {
       //notify create bot failure
       if(agora_agent.status_cb != NULL)
       {          
          agora_agent.status_cb(AGORA_RTC_AGENT_CREATE, (void *)result);
       }  
    }

    if(bot_data->op_data != NULL){
        qosa_free(bot_data->op_data);
        bot_data->op_data != NULL;
    }
    qosa_free(bot_data);
}

int  agora_rtc_chat_config(agora_rtc_cred *cred, char *channel_name)
{
  // struct agora_chat_agent_t  store_agent = {0};
  // qosa_nv_cfg_item_data_t agent_info[] = {
  //   NV_ITEM_CFG_DEF("channel_name", store_agent.channel_name),   /*!< SIM card status URC enable configuration */
  //   NV_ITEM_CFG_DEF("agent_id", store_agent.agent_id),
  // };
  if(cred == QOSA_NULL || cred->app_id == QOSA_NULL)
  {
    return -1;
  }
  agora_cred.app_id = qosa_strdup(cred->app_id);
  if(cred->app_cert != QOSA_NULL && qosa_strlen(cred->app_cert) > 0)
  {
    agora_cred.app_cert = qosa_strdup(cred->app_cert);
  }
  if(cred->customer_key != QOSA_NULL && qosa_strlen(cred->customer_key) > 0)
  {
    agora_cred.customer_key = qosa_strdup(cred->customer_key);
  }
  if(cred->customer_secret != QOSA_NULL && qosa_strlen(cred->customer_secret) > 0)
  {
    agora_cred.customer_secret = qosa_strdup(cred->customer_secret);
  }
  if(channel_name != QOSA_NULL)
  {
    qosa_strncpy(agora_agent.channel_name, channel_name, qosa_strlen(channel_name));
  }
  agora_agent.agent_offline = QOSA_TRUE;
 
  // if(qosa_nv_item_cfg_read(AGORA_AGENT_KEY, agent_info, QOSA_ARRAY_SIZE(agent_info)) == QOSA_NVM_CFG_OK)
  // {
  //   if(qosa_strcmp(agora_agent.channel_name, store_agent.channel_name) == 0)
  //   {
  //     qosa_strcpy(agora_agent.agent_id, store_agent.agent_id);
  //     return 0;
  //   }
  //   else 
  //   {
  //     qosa_nv_item_cfg_delete(AGORA_AGENT_KEY, agent_info, QOSA_ARRAY_SIZE(agent_info));
  //   }
  // }
  // //update agent info
  // qosa_nv_cfg_item_data_t save_info[] = {
  //    NV_ITEM_CFG_DEF("channel_name", agora_agent.channel_name),
  //    NV_ITEM_CFG_DEF("agent_id", agora_agent.agent_id),
  // };
  // qosa_nv_item_json_write(AGORA_AGENT_KEY, save_info, QOSA_ARRAY_SIZE(save_info));

  return 0;
}

int  agora_rtc_chat_agent_create(agora_rtc_agent_config_t *option, agora_rtc_agent_status_cb status_cb)
{
  QOSA_ASSERT(option != QOSA_NULL
              && option->user_id != 0
              && option->asr_vendor != QOSA_NULL
              && option->llm_vendor != QOSA_NULL
              && option->llm_url != QOSA_NULL
              && option->llm_key != QOSA_NULL
              && option->tts_vendor != QOSA_NULL);

  char url[256] = {0};
  char *body_content = QOSA_NULL;
  size_t body_len = 0;
  char *tts_cfg = QOSA_NULL;
  char *llm_cfg = QOSA_NULL;
  char *asr_cfg = QOSA_NULL;
  // char *vad_cfg = NULL;
  char *turn_detection_cfg = QOSA_NULL;
  char *token = QOSA_NULL;
    // const char *tts_api_key = "sk-ebbaba029ab34202a4b8444e48bb4f31";//"sk-a7dab90e04ef458fbfc7646137275bf7";
   
  struct agora_chat_agent_data  *bot_data = QOSA_NULL;
    
  if(agora_cred.app_cert != QOSA_NULL && qosa_strlen(agora_cred.app_cert) > 0)
  {
      token = rtc_token_builder_build_with_uid(
            agora_cred.app_id,
            agora_cred.app_cert,
            agora_agent.channel_name,
            option->user_id,            // uid
            ROLE_PUBLISHER,
            3600*24,              // token 有效期 1 小时
            3600*24               // 权限有效期 1 小时
        );
      if(token == QOSA_NULL){
        return -1;
      }
      agora_agent.channel_token = qosa_strdup(token);
  }   
  agora_agent.status_cb = status_cb;
  agora_agent.channel_uid = option->user_id; 
  qosa_snprintf(url, 256, "%s%s/join", AGORA_PROJECT_URL, agora_cred.app_id);
    
  body_content = qosa_calloc(1,5120);
  tts_cfg = qosa_calloc(1,256);
  llm_cfg = qosa_calloc(1,1024);
  asr_cfg = qosa_calloc(1,256);
  // vad_cfg = psram_zalloc(128);
  turn_detection_cfg = qosa_calloc(1,512);

  QOSA_ASSERT(body_content != QOSA_NULL);
  QOSA_ASSERT(tts_cfg != QOSA_NULL);
  QOSA_ASSERT(llm_cfg != QOSA_NULL);
  QOSA_ASSERT(asr_cfg != QOSA_NULL);
  // BK_ASSERT(vad_cfg != NULL);
  QOSA_ASSERT(turn_detection_cfg != QOSA_NULL);

  if(qosa_strcmp(option->asr_vendor, "fengming") == 0)
  {
    qosa_strcpy(asr_cfg, asr_fengming_config);
  }
  else
  {
    QLOGI("now not support %s asr service\n", option->asr_vendor);
  }
  if(qosa_strcmp(option->llm_vendor, "deepseek") == 0)
  {
      qosa_snprintf(llm_cfg, 1024, llm_deepseek_config, option->llm_url, option->llm_key,
           option->prompt?option->prompt:"", 
           option->greeting_msg?option->greeting_msg:"", 
           option->failure_msg?option->failure_msg:"");
  }  
  else
  {
      QLOGI("now not support %s LLM\n", option->llm_vendor);
  }  
    
  if(qosa_strcmp(option->tts_vendor, "cosyvoice") == 0)
  {
    cosyvoice_tts_cofig  *cosyvoice_cfg = (cosyvoice_tts_cofig  *)option->tts_vendor_params;
    qosa_snprintf(tts_cfg, 256, ttys_cosyvoice_config, cosyvoice_cfg->api_key, cosyvoice_cfg->model, cosyvoice_cfg->sample_rate, cosyvoice_cfg->voice);
  }
  else
  {
      QLOGI("now not support %s TTS Service\n", option->tts_vendor);
  }
  qosa_strcpy(turn_detection_cfg, turn_detection_config);
    // snprintf(vad_cfg, 128, vad_config, bot_option->vad_cfg.threshold, bot_option->vad_cfg.prefix_padding_ms, bot_option->vad_cfg.silence_duration_ms, bot_option->vad_cfg.interrupt_duration_ms);

  body_len =qosa_snprintf(body_content, 5120, create_bot_request, agora_agent.channel_name,
                       agora_agent.channel_name, option->user_id, 120, llm_cfg, asr_cfg, tts_cfg, turn_detection_cfg,(token == NULL?"":token));

  qosa_free(llm_cfg);
  qosa_free(tts_cfg);
  qosa_free(asr_cfg);
  if(token != QOSA_NULL)
      qosa_free(token);
 // psram_free(vad_cfg);
  qosa_free(turn_detection_cfg);

  agora_http_callback_t  cb_fcn = {
            .read_data_cb = agora_chat_agent_request_data,
            .write_data_cb = agora_chat_agent_respone_data,
            .exception_cb = agora_chat_agent_exception_cb,
  };

    // BK_LOGI(TAG, "bot_create[%d]:%s\n", body_len, body_content);

  bot_data = qosa_calloc(1,sizeof(struct agora_chat_agent_data));
  QOSA_ASSERT(bot_data != QOSA_NULL);

  bot_data->op_code = CREATE_AGENT;
  bot_data->op_data = body_content;
  bot_data->data_len = body_len;
  qosa_strcpy(bot_data->session_url, url);

  agora_http_request(url, HTTP_METHOD_POST, cb_fcn, (void *)bot_data);
  return 0;
}

// int    agora_rtc_chat_agent_status_get()
// {
//   QLOGI("agent_id:%s\n", agora_agent.agent_id);

//   if(qosa_strlen(agora_agent.agent_id) == 0)
//   {
//     if(agora_agent.status_cb != QOSA_NULL)
//     {
//        int status_val = AGORA_RTC_AGENT_STA_IDLE;
//        agora_agent.status_cb(AGORA_RTC_AGENT_STATUS, (void *)status_val);
//     }
//     return 0;
//   }
//   char url[256] = {0};
//   struct agora_chat_agent_data  *bot_data = QOSA_NULL;
//   qosa_snprintf(url, 256, "%s%s/agents/%s", AGORA_PROJECT_URL, agora_cred.app_id, agora_agent.agent_id);

//   agora_http_callback_t  cb_fcn = {
//     .read_data_cb = agora_chat_agent_request_data,
//     .write_data_cb = agora_chat_agent_respone_data,
//     .exception_cb = agora_chat_agent_exception_cb,
//   };

//   // QLOGI("bot_create[%d]:%s\n", body_len, body_content);

//   bot_data = qosa_calloc(1,sizeof(struct agora_chat_agent_data));
//   BK_ASSERT(bot_data != NULL);

//   bot_data->op_code = STATUS_AGENT;
//   qosa_strcpy(bot_data->session_url, url);

//   agora_http_request(url, HTTP_METHOD_GET, cb_fcn, (void *)bot_data);

//   return 0;
// }

// int    agora_rtc_chat_active_agents_get()
// {
//   char url[256] = {0};
//   struct agora_chat_agent_data  *bot_data = NULL;
//   //https://api.agora.io/cn/api/conversational-ai-agent/v2/projects/{appid}/agents?channel="111"&state=2&limit=20
//   qosa_snprintf(url, 256, "%s%s/agents?channel=\"%s\"&status=%d&limit=20", AGORA_PROJECT_URL, agora_cred.app_id, agora_agent.channel_name, AGORA_RTC_AGENT_STA_RUNNING);
//   bot_data = qosa_calloc(1,sizeof(struct agora_chat_agent_data));
//   QOSA_ASSERT(bot_data != NULL);
//   agora_http_callback_t  cb_fcn = {
//       .read_data_cb = agora_chat_agent_request_data,
//       .write_data_cb = agora_chat_agent_respone_data,
//       .exception_cb = agora_chat_agent_exception_cb,
//   };
//   bot_data->op_code = LIST_ACTIVE_AGENT;
//   qosa_strcpy(bot_data->session_url, url);

//   agora_http_request(url, HTTP_METHOD_GET, cb_fcn, (void *)bot_data);
//   return 0;
// }

// int    agora_rtc_chat_agent_stop()
// {
//   char url[256] = {0};

//   if(strlen(agora_agent.agent_id) == 0)
//   {
//     QLOGI("there are no exist agent!!\n");
//     return -1;
//   }
//   qosa_snprintf(url, 256, "%s%s/agents/%s/leave", AGORA_PROJECT_URL, agora_cred.app_id, agora_agent.agent_id);

//   struct agora_chat_agent_data  *bot_data = QOSA_NULL;
//   bot_data = qosa_calloc(1,sizeof(struct agora_chat_agent_data));
//   QOSA_ASSERT(bot_data != QOSA_NULL);
//   agora_http_callback_t  cb_fcn = {
//       .read_data_cb = agora_chat_agent_request_data,
//       .write_data_cb = agora_chat_agent_respone_data,
//       .exception_cb = agora_chat_agent_exception_cb,
//   };
//   bot_data->op_code = STOP_AGENT;
//   qosa_strcpy(bot_data->session_url, url);

//   agora_http_request(url, HTTP_METHOD_POST, cb_fcn, (void *)bot_data);
    
//   return 0;
// }

static void  agora_rtc_user_notify_msg_handle(agora_rtc_msg_t *p_msg)
{
    switch (p_msg->code)
    {
        case AGORA_RTC_MSG_JOIN_CHANNEL_SUCCESS:
            agora_agent.connected_flag = QOSA_TRUE;
            QLOGI("Join channel success\n");
            break;
            
        case AGORA_RTC_MSG_REJOIN_CHANNEL_SUCCESS:
            agora_agent.connected_flag = QOSA_TRUE;
            QLOGI("Rejoin channel success\n");
            break;
            
        case AGORA_RTC_MSG_USER_JOINED:
            QLOGI("User joined\n");
            agora_agent.agent_offline = QOSA_FALSE;
            break;
            
        case AGORA_RTC_MSG_USER_OFFLINE:
            QLOGI("User offline\n");
            agora_agent.agent_offline = QOSA_TRUE;
            break;
            
        case AGORA_RTC_MSG_CONNECTION_LOST:
            QLOGE("Connection lost. Please check WiFi status\n");
            agora_agent.connected_flag = QOSA_FALSE;
            break;
            
        case AGORA_RTC_MSG_INVALID_APP_ID:
            QLOGE("Invalid App ID. Please double check\n");
            break;
            
        case AGORA_RTC_MSG_INVALID_CHANNEL_NAME:
            QLOGE("Invalid channel name. Please double check\n");
            break;
            
        case AGORA_RTC_MSG_INVALID_TOKEN:
        case AGORA_RTC_MSG_TOKEN_EXPIRED:
            QLOGE("Invalid or expired token. Please double check\n");
            break;
            
        case AGORA_RTC_MSG_BWE_TARGET_BITRATE_UPDATE:
            // Handle bitrate update if needed
            // g_target_bps = p_msg->data.bwe.target_bitrate;
            break;
            
        case AGORA_RTC_MSG_KEY_FRAME_REQUEST:
            // Handle key frame request if needed
            break;
            
        default:
            break;
    }
}
static int agora_rtc_user_audio_rx_data_handle(unsigned char *data, unsigned int size, const audio_frame_info_t *info_ptr)
{
  if(agora_agent.audio_rx_cb != QOSA_NULL)
  {
    return agora_agent.audio_rx_cb(data, size);
  }
  return -1;
}
static void agora_rtc_main(void *ctx)
{
  int ret = 0;
  agora_rtc_config.p_appid = (char *)qosa_malloc(strlen(agora_cred.app_id) + 1);
  qosa_strcpy((char *)agora_rtc_config.p_appid, agora_cred.app_id);

  QLOGI("agora_main use record appid:%s %s\r\n", agora_rtc_config.p_appid,agora_cred.app_id);
    
  agora_rtc_config.log_disable = false;//true;
  agora_rtc_config.bwe_param_max_bps = BANDWIDTH_ESTIMATE_MAX_BITRATE;
    
  // Create Agora RTC instance
  ret = __agora_rtc_create(&agora_rtc_config, (agora_rtc_msg_notify_cb)agora_rtc_user_notify_msg_handle);
  if (ret != BK_OK)
  {
      QLOGE("__agora_rtc_create failed\n");
      goto exit;
  }
  agora_rtc_option.p_channel_name = (char *)qosa_malloc(qosa_strlen(agora_agent.channel_name) + 1);

  qosa_strcpy((char *)agora_rtc_option.p_channel_name, agora_agent.channel_name);
  agora_rtc_option.audio_config.audio_data_type = CONFIG_AUDIO_CODEC_TYPE;
  agora_rtc_option.audio_config.pcm_sample_rate = 16000;
    // agora_rtc_option.audio_config.audio_data_type = CONFIG_AUDIO_CODEC_TYPE;
  if (agora_agent.channel_token != QOSA_NULL)
  {
     QLOGI("agora_main use record token:%s \r\n", agora_agent.channel_token);
     agora_rtc_option.p_token = agora_agent.channel_token;
  }
  agora_rtc_option.uid = agora_agent.channel_uid; 

  QLOGI("p_token:%s\n",agora_rtc_option.p_token);
  QLOGI("uid:%d\n",agora_rtc_option.uid);
  QLOGI("audio_data_type:%d\n",agora_rtc_option.audio_config.audio_data_type);
    // return;
    // Start Agora RTC
  ret = __agora_rtc_start(&agora_rtc_option);
  QLOGI("rtc_start ret:%d\n", ret);
  if (ret != QOSA_OK)
  {
      QLOGE("__agora_rtc_start failed, ret:%d\n", ret);
      goto exit;
  }
  
  agora_runing = true;

  qosa_sem_release(agora_rtc_sem);
    
  /* Wait until we join channel successfully */
  while (!agora_agent.connected_flag)
  {
      if (!agora_runing)
      {
          goto exit;
      }
      qosa_task_sleep_ms(100);
  }
    
  QLOGI("-----Agora RTC join channel success-----\n");
  /* Main loop */

  ret = __agora_rtc_register_audio_rx_handle((agora_rtc_audio_rx_data_handle)agora_rtc_user_audio_rx_data_handle);
  if (ret != BK_OK)
  {
    QLOGE("Failed to register audio RX handle, ret:%d\n", ret);
  }    

  while (agora_runing)
  {
    qosa_task_sleep_ms(100);
  }
exit:
  __agora_rtc_register_audio_rx_handle(NULL);

  /* Stop Agora RTC */
  __agora_rtc_stop();
    
  /* Destroy Agora RTC */
  __agora_rtc_destroy();
    
  if (agora_rtc_config.p_appid)
  {
      qosa_free((char *)agora_rtc_config.p_appid);
      agora_rtc_config.p_appid = QOSA_NULL;
  }
    
  if (agora_rtc_option.p_channel_name)
  {
      qosa_free((char *)agora_rtc_option.p_channel_name);
      agora_rtc_option.p_channel_name = QOSA_NULL;
  }
    
  agora_agent.connected_flag = QOSA_FALSE;
  agora_rtc_task = QOSA_NULL;
  agora_runing = QOSA_FALSE;
    
  qosa_sem_release(&agora_rtc_sem);
  QLOGI("agora task exit!\n");
  qosa_task_delete(NULL);  
}

int    agora_rtc_chat_agent_start(agora_rtc_agent_audio_rx_cb  rx_cb)
{
  agora_agent.audio_rx_cb = rx_cb;
  qosa_sem_create(&agora_rtc_sem, 0);
  
  QOSA_ASSERT(agora_rtc_sem != QOSA_NULL);

  if(qosa_task_create(&agora_rtc_task, 16*1024,4, "agora", agora_rtc_main,NULL) != QOSA_ERROR_OK)
  {
    return -1;
  }

  qosa_sem_wait(agora_rtc_sem, QOSA_WAIT_FOREVER);
    
  QLOGI("Create agora app task complete\n");
    
  return QOSA_OK;                            
}

int    agora_rtc_chat_agent_stop()
{
    if (!agora_runing)
    {
        QLOGI("Agora not started\n");
        return QOSA_OK;
    }
    
    agora_runing = false;
    qosa_sem_wait(agora_rtc_sem, QOSA_WAIT_FOREVER);
    
    qosa_sem_delete(agora_rtc_sem);
    agora_rtc_sem = QOSA_NULL;
    
    return QOSA_OK;
}

int    agora_rtc_chat_agent_update(void *update_info)
{
  return QOSA_OK;
}

int    agora_rtc_audio_data_send(uint8_t *data_ptr, size_t data_len, audio_enc_type_t audio_type)
{
    int rval = 0;
    audio_frame_info_t info = { 0 };
    agora_rtc_t *rtc = NULL;
    
    if (QOSA_FALSE == agora_agent.connected_flag)
    {
        // QLOGI("Failed to send audio data, g_connected_flag:%d\n", g_connected_flag);  // Disabled: too verbose
        return QOSA_ERROR_GENERAL;
    }
    
    rtc = __get_rtc_instance();
    if (!rtc || (rtc->state != AGORA_RTC_STATE_WORKING))
    {
        return QOSA_ERROR_GENERAL;
    }
    
    info.data_type = agora_audio_codec_type_mapping(audio_type);
    // QLOGI("audio_type:%d[%d]\n", audio_type, info.data_type);
    rval = agora_rtc_send_audio_data(rtc->conn_id, data_ptr, data_len, &info);
    // QLOGI("send return:%d\n", rval);
    if (rval < 0)
    {
        QLOGE("Failed to send audio data, rval:%d, err:%s\n", rval, agora_rtc_err_2_str(rval));  // Disabled: too verbose
        return QOSA_ERROR_GENERAL;
    }
    
    return data_len;
}

int    agora_rtc_video_data_send(frame_buffer_t *frame)
{
    // int rval = 0;
    // video_frame_info_t info = { 0 };
    // static uint64_t before = 0, curr = 0;
    // agora_rtc_t *rtc = QOSA_NULL;
    
    // curr = qosa_get_system_time_seconds()*1000;
    
    // if (QOSA_FALSE == agora_agent.connected_flag)
    // {
    //     // LOGI("Failed to send video data, g_connected_flag:%d\n", g_connected_flag);  // Disabled: too verbose
    //     return QOSA_ERROR_GENERAL;
    // }
    
    // /* Validate frame parameter */
    // if (frame == QOSA_NULL || frame->frame == QOSA_NULL || frame->length == 0)
    // {
    //     return QOSA_ERROR_GENERAL;
    // }
    
    // /* Setup video frame info based on format */
    // info.stream_type = VIDEO_STREAM_HIGH;
    // if (frame->fmt == PIXEL_FMT_H264)
    // {
    //     /* Check if it's an I-frame (only send I-frames) */
    //     if ((frame->h264_type & (1 << H264_NAL_I_FRAME)) == 0)
    //     {
    //         // LOGD("%s: skip non-I frame\n", __func__);
    //         return QOSA_OK;
    //     }
        
    //     info.data_type = VIDEO_DATA_TYPE_H264;
    //     info.frame_type = VIDEO_FRAME_AUTO_DETECT;
    //     // info.frame_rate = 1000/VIDEO_FRAME_INTERVAL;
    // }
    // else if (frame->fmt == PIXEL_FMT_JPEG)
    // {
    //     info.data_type = VIDEO_DATA_TYPE_GENERIC_JPEG;
    //     info.frame_type = VIDEO_FRAME_KEY;
    // }
    // else if (frame->fmt == PIXEL_FMT_H265)
    // {
    //     info.data_type = VIDEO_DATA_TYPE_H265;
    //     info.frame_type = VIDEO_FRAME_AUTO_DETECT;
    // }
    // else
    // {
    //     QLOGE("%s: Unsupported format: %d\n", __func__, frame->fmt);
    //     return QOSA_ERROR_GENERAL;
    // }
    
    // /* Frame rate control - send every VIDEO_FRAME_INTERVAL ms */
    // if (curr <= before || curr - before < VIDEO_FRAME_INTERVAL)
    // {
    //     return QOSA_OK;
    // }
    
    // rtc = __get_rtc_instance();
    // if (!rtc || (rtc->state != AGORA_RTC_STATE_WORKING))
    // {
    //     // LOGI("Failed to send video data, rtc->state:%d\n", rtc->state);  // Disabled: too verbose
    //     return QOSA_ERROR_GENERAL;
    // }
    
    // /* Send video data to agora_rtc */
    // rval = agora_rtc_send_video_data(rtc->conn_id, (uint8_t *)frame->frame, (size_t)frame->length, &info);
    // if (rval < 0)
    // {
    //     // LOGW("%s: send video data failed, rval=%d data_type=%d len=%d frame_type=%d\n",
    //     //      __func__, rval, info.data_type, (int)frame->length, info.frame_type);  // Disabled: too verbose
    // }
    // else
    // {
    //     // LOGD("%s: send video data successfully, len=%d\n", __func__, (int)frame->length);
    // }
    
    // /* Update timestamp */
    // before = curr;
    
    return QOSA_OK;
}