#ifndef   QCM_LLM_API_H
#define   QCM_LLM_API_H


#include "qcm_llm_types.h"

typedef struct {
    const char *api_key;
    const char *backend_type;
    const char *model;
    const char *base_url;
    const char *auth_type;
    const char *max_tokens_field;
    uint32_t timeout_ms;
    uint32_t max_tokens;
    size_t image_max_bytes;
    bool supports_tools;
    bool supports_vision;
    bool image_remote_url_only;
} qcm_llm_config_t;



qcm_llm_error_e qcm_llm_init(const qcm_llm_config_t *config, char **out_error_message);

qcm_llm_error_e qcm_llm_chat_messages(const char *system_prompt,
                                      cJSON *messages,
                                      const char *tools_json,
                                      qcm_llm_response_t *out_response,
                                      char **out_error_message);

qcm_llm_error_e qcm_llm_chat(const char *system_prompt,
                             const char *user_text,
                             char **out_text,
                             char **out_error_message);

qcm_llm_error_e qcm_llm_analyze_image(const char *system_prompt,
                                      const char *user_prompt,
                                      const char *image_path,
                                      char **out_text,
                                      char **out_error_message);

qcm_llm_error_e qcm_llm_infer_media(const qcm_llm_media_request_t *request,
                                    char **out_text,
                                    char **out_error_message);

void qcm_llm_response_free(qcm_llm_response_t *response);

#endif