#ifndef   QCM_LLM_TYPES_H
#define   QCM_LLM_TYPES_H


#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Beken cJSON.h is gated by CONFIG_CJSON_USE from sdkconfig.h
#include "sdkconfig.h"
#include "cJSON.h"
#include "qosa_def.h"

// #if defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC)
// #include "os/os.h"
// #include "os/str.h"
// #include "os/mem.h"
// #endif

// #if defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC)
// #define QCM_LLM_MALLOC(n)      psram_malloc(n)
// #define QCM_LLM_CALLOC(c, n)   psram_zalloc(c*n)
// #define QCM_LLM_FREE(p)        psram_free(p)
// #else
// #define QCM_LLM_MALLOC(n)      os_malloc(n)
// #define QCM_LLM_CALLOC(c, n)   os_calloc(c, n)
// #define QCM_LLM_FREE(p)        os_free(p)
// #endif

#define QCM_ERRCODE_LLM_API_BASE (QOSA_COMPONENT_LLM << 16)

typedef enum
{
    QCM_LLM_OK = 0, /*!< Operation successful */

    QCM_LLM_FAIL                = QCM_ERRCODE_LLM_API_BASE | 400,
    QCM_LLM_ERR_NO_MEM          = QCM_ERRCODE_LLM_API_BASE | 401,
    QCM_LLM_ERR_INVALID_ARG     = QCM_ERRCODE_LLM_API_BASE | 402,
    QCM_LLM_ERR_NOT_SUPPORTED   = QCM_ERRCODE_LLM_API_BASE | 403,
    QCM_LLM_ERR_INVALID_STATE   = QCM_ERRCODE_LLM_API_BASE | 404,
    QCM_LLM_ERR_NOT_FOUND       = QCM_ERRCODE_LLM_API_BASE | 405,
    QCM_LLM_ERR_INVALID_SIZE    = QCM_ERRCODE_LLM_API_BASE | 406,
    QCM_LLM_ERR_INVALID_RESPONSE = QCM_ERRCODE_LLM_API_BASE | 407,
    QCM_LLM_ERROR_MAX,                                           /*!< Maximum error code value, used for boundary checking */
} qcm_llm_error_e;

typedef struct qcm_llm_runtime qcm_llm_runtime_t;

typedef enum {
    QCM_MEDIA_ASSET_KIND_LOCAL_PATH = 0,
    QCM_MEDIA_ASSET_KIND_REMOTE_URL = 1,
    QCM_MEDIA_ASSET_KIND_INLINE_BYTES = 2,
} qcm_media_asset_kind_t;

typedef enum {
    QCM_MEDIA_PREPARED_KIND_DATA_URL = 0,
    QCM_MEDIA_PREPARED_KIND_REMOTE_URL = 1,
} qcm_media_prepared_kind_t;

typedef struct {
    qcm_media_asset_kind_t kind;
    const char *path;
    const char *url;
    const uint8_t *bytes;
    size_t byte_count;
    const char *mime_type;
} qcm_media_asset_t;

typedef struct {
    qcm_media_prepared_kind_t kind;
    char *payload;
    char mime_type[32];
    size_t original_size;
} qcm_media_prepared_t;

typedef struct {
    const char *chat_path;
    const char *max_tokens_field;
    bool supports_tools;
    bool supports_vision;
    bool image_remote_url_only;
} qcm_llm_model_profile_t;

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
} qcm_llm_runtime_config_t;

typedef struct {
    char *id;
    char *name;
    char *arguments_json;
} qcm_llm_tool_call_t;

typedef struct {
    char *text;
    char *reasoning_content;
    qcm_llm_tool_call_t *tool_calls;
    size_t tool_call_count;
} qcm_llm_response_t;

typedef struct {
    const char *system_prompt;
    cJSON *messages;
    const char *tools_json;
} qcm_llm_chat_request_t;

typedef struct {
    const char *system_prompt;
    const char *user_prompt;
    const qcm_media_asset_t *media;
    size_t media_count;
} qcm_llm_media_request_t;

typedef struct {
    const char *name;
    const char *value;
} qcm_llm_http_header_t;

typedef struct {
    const char *url;
    const char *body;
    const char *api_key;
    const char *auth_type;
    uint32_t timeout_ms;
    const qcm_llm_http_header_t *headers;
    size_t header_count;
} qcm_llm_http_json_request_t;

typedef struct {
    char *body;
    int status_code;
} qcm_llm_http_response_t;

#endif