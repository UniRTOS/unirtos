

#include <stdlib.h>
#include <string.h>

#include "qosa_def.h"
#include "qosa_log.h"

#include "qcm_llm_types.h"
#include "qcm_llm_runtime.h"

#define QCM_LLM_DEFAULT_TIMEOUT_MS (120 * 1000)
#define QCM_LLM_DEFAULT_MAX_TOKENS 8192
#define QCM_LLM_DEFAULT_IMAGE_MAX_BYTES (512 * 1024)

struct qcm_llm_runtime {
    qcm_llm_runtime_config_t config;
    qcm_llm_model_profile_t profile;
    const qcm_llm_backend_vtable_t *backend;
    void *backend_ctx;
};

static char *dup_or_null(const char *value)
{
    return value ? qosa_strdup(value) : NULL;
}

static bool string_is_empty(const char *value)
{
    return !value || value[0] == '\0';
}

static void runtime_config_free(qcm_llm_runtime_config_t *config)
{
    if (!config) {
        return;
    }
    qosa_free((char *)config->api_key);
    qosa_free((char *)config->backend_type);
    qosa_free((char *)config->model);
    qosa_free((char *)config->base_url);
    qosa_free((char *)config->auth_type);
    qosa_free((char *)config->max_tokens_field);

    memset(config, 0, sizeof(*config));
}

static qcm_llm_error_e runtime_config_copy(qcm_llm_runtime_config_t *dst,
                                     const qcm_llm_runtime_config_t *src)
{
    dst->api_key = dup_or_null(src->api_key);
    dst->backend_type = dup_or_null(src->backend_type);
    dst->model = dup_or_null(src->model);
    dst->base_url = dup_or_null(src->base_url);
    dst->auth_type = dup_or_null(src->auth_type);
    dst->max_tokens_field = dup_or_null(src->max_tokens_field);
    dst->timeout_ms = src->timeout_ms;
    dst->max_tokens = src->max_tokens;
    dst->image_max_bytes = src->image_max_bytes;
    dst->supports_tools = src->supports_tools;
    dst->supports_vision = src->supports_vision;
    dst->image_remote_url_only = src->image_remote_url_only;

    if ((src->api_key && !dst->api_key) ||
            (src->backend_type && !dst->backend_type) ||
            (src->model && !dst->model) ||
            (src->base_url && !dst->base_url) ||
            (src->auth_type && !dst->auth_type) ||
            (src->max_tokens_field && !dst->max_tokens_field)) {
        runtime_config_free(dst);
        return QCM_LLM_ERR_NO_MEM;
    }

    return QCM_LLM_OK;
}

qcm_llm_error_e qcm_llm_runtime_init(qcm_llm_runtime_t **out_runtime,
                                const qcm_llm_runtime_config_t *config,
                                char **out_error_message)
{
    qcm_llm_runtime_t *runtime;
    const qcm_llm_backend_registration_t *backend_registration;
    qcm_llm_error_e err;

    if (out_runtime) {
        *out_runtime = NULL;
    }
    if (out_error_message) {
        *out_error_message = NULL;
    }
    if (!out_runtime || !config || !config->api_key || !config->model ||
            !config->backend_type || !config->backend_type[0] || !out_error_message) {
        return QCM_LLM_ERR_INVALID_ARG;
    }

    backend_registration = qcm_llm_find_backend_registration(config->backend_type);
    
    if (!backend_registration || !backend_registration->vtable) {
        *out_error_message = dup_or_null("Unknown LLM backend type");
        return QCM_LLM_ERR_NOT_SUPPORTED;
    }

    runtime = qosa_calloc(1, sizeof(*runtime));
 
    if (!runtime) {
        *out_error_message = dup_or_null("Out of memory allocating runtime");

        return QCM_LLM_ERR_NO_MEM;
    }

    err = runtime_config_copy(&runtime->config, config);
    if (err != QCM_LLM_OK) {
        qosa_free(runtime);
        *out_error_message = dup_or_null("Out of memory copying runtime config");
        return err;
    }
    if (string_is_empty(runtime->config.auth_type)) {

        qosa_free((char *)runtime->config.auth_type);

        runtime->config.auth_type = dup_or_null(backend_registration->defaults.auth_type);
    }
    if (!runtime->config.timeout_ms) {
        runtime->config.timeout_ms = QCM_LLM_DEFAULT_TIMEOUT_MS;
    }
    if (!runtime->config.max_tokens) {
        runtime->config.max_tokens = QCM_LLM_DEFAULT_MAX_TOKENS;
    }
    if (!runtime->config.image_max_bytes) {
        runtime->config.image_max_bytes = QCM_LLM_DEFAULT_IMAGE_MAX_BYTES;
    }
    if (string_is_empty(runtime->config.max_tokens_field)) {
        qosa_free((char *)runtime->config.max_tokens_field);
        runtime->config.max_tokens_field = dup_or_null(backend_registration->defaults.max_tokens_field);
    }
    if (!runtime->config.backend_type ||
            (backend_registration->defaults.auth_type && !runtime->config.auth_type) ||
            (backend_registration->defaults.max_tokens_field && !runtime->config.max_tokens_field)) {
        runtime_config_free(&runtime->config);
        qosa_free(runtime);

        *out_error_message = dup_or_null("Out of memory finalizing runtime config");
        return QCM_LLM_ERR_NO_MEM;
    }

    runtime->profile.chat_path = backend_registration->defaults.chat_path ?
                                 backend_registration->defaults.chat_path : "";
    runtime->profile.max_tokens_field = runtime->config.max_tokens_field ?
                                        runtime->config.max_tokens_field : "";
    runtime->profile.supports_tools = runtime->config.supports_tools;
    runtime->profile.supports_vision = runtime->config.supports_vision;
    runtime->profile.image_remote_url_only = runtime->config.image_remote_url_only;
    runtime->backend = backend_registration->vtable;

    err = runtime->backend->init(&runtime->config, &runtime->profile, &runtime->backend_ctx, out_error_message);
    if (err != QCM_LLM_OK) {
        runtime_config_free(&runtime->config);
        qosa_free(runtime);
        return err;
    }
    *out_runtime = runtime;
    return QCM_LLM_OK;
}

qcm_llm_error_e qcm_llm_runtime_chat(qcm_llm_runtime_t *runtime,
                                const qcm_llm_chat_request_t *request,
                                qcm_llm_response_t *out_response,
                                char **out_error_message)
{
    if (!runtime || !runtime->backend || !runtime->backend->chat) {
        return QCM_LLM_ERR_INVALID_STATE;
    }

    return runtime->backend->chat(runtime->backend_ctx,
                                  &runtime->profile,
                                  request,
                                  out_response,
                                  out_error_message);
}

qcm_llm_error_e qcm_llm_runtime_infer_media(qcm_llm_runtime_t *runtime,
                                       const qcm_llm_media_request_t *request,
                                       char **out_text,
                                       char **out_error_message)
{
    if (!runtime || !runtime->backend || !runtime->backend->infer_media) {
        return QCM_LLM_ERR_INVALID_STATE;
    }

    return runtime->backend->infer_media(runtime->backend_ctx,
                                         &runtime->profile,
                                         request,
                                         out_text,
                                         out_error_message);
}

void qcm_llm_runtime_deinit(qcm_llm_runtime_t *runtime)
{
    if (!runtime) {
        return;
    }

    if (runtime->backend && runtime->backend->deinit) {
        runtime->backend->deinit(runtime->backend_ctx);
    }
    runtime_config_free(&runtime->config);

    qosa_free(runtime);
}

void qcm_llm_response_free(qcm_llm_response_t *response)
{
    size_t i;

    if (!response) {
        return;
    }


    qosa_free(response->text);
    qosa_free(response->reasoning_content);

    for (i = 0; i < response->tool_call_count; i++) {
        qosa_free(response->tool_calls[i].id);
        qosa_free(response->tool_calls[i].name);
        qosa_free(response->tool_calls[i].arguments_json);
    }

    qosa_free(response->tool_calls);

    memset(response, 0, sizeof(*response));
}
