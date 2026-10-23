#ifndef QCM_LLM_RUNTIME_H
#define QCM_LLM_RUNTIME_H

#include "qcm_llm_types.h"

typedef struct {
    const char *id;
    qcm_llm_error_e (*init)(const qcm_llm_runtime_config_t *config,
                      const qcm_llm_model_profile_t *profile,
                      void **out_backend_ctx,
                      char **out_error_message);
    qcm_llm_error_e (*chat)(void *backend_ctx,
                      const qcm_llm_model_profile_t *profile,
                      const qcm_llm_chat_request_t *request,
                      qcm_llm_response_t *out_response,
                      char **out_error_message);
    qcm_llm_error_e (*infer_media)(void *backend_ctx,
                             const qcm_llm_model_profile_t *profile,
                             const qcm_llm_media_request_t *request,
                             char **out_text,
                             char **out_error_message);
    void (*deinit)(void *backend_ctx);
} qcm_llm_backend_vtable_t;

typedef struct {
    const char *auth_type;
    const char *chat_path;
    const char *max_tokens_field;
} qcm_llm_backend_defaults_t;

typedef struct {
    const char *id;
    const qcm_llm_backend_vtable_t *vtable;
    qcm_llm_backend_defaults_t defaults;
} qcm_llm_backend_registration_t;

qcm_llm_error_e qcm_llm_runtime_init(qcm_llm_runtime_t **out_runtime,
                                const qcm_llm_runtime_config_t *config,
                                char **out_error_message);

qcm_llm_error_e qcm_llm_runtime_chat(qcm_llm_runtime_t *runtime,
                                const qcm_llm_chat_request_t *request,
                                qcm_llm_response_t *out_response,
                                char **out_error_message);

qcm_llm_error_e qcm_llm_runtime_infer_media(qcm_llm_runtime_t *runtime,
                                       const qcm_llm_media_request_t *request,
                                       char **out_text,
                                       char **out_error_message);

void qcm_llm_runtime_deinit(qcm_llm_runtime_t *runtime);

qcm_llm_error_e qcm_llm_register_custom_backend(const qcm_llm_backend_registration_t *registration);

const qcm_llm_backend_registration_t *qcm_llm_find_backend_registration(const char *id);

const qcm_llm_backend_vtable_t *qcm_llm_find_custom_backend(const char *id);

#endif