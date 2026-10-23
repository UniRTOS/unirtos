
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qosa_def.h"
#include "qosa_log.h"

// #include "ql_crt_bundle.h"
// #include "ql_http_client.h"
#include "mongoose.h"
#include "task.h"
#include "qcm_llm_transport.h"

#define QOS_LOG_TAG LOG_TAG_COMPONENT

#define QCM_LLM_HTTP_RB_INITIAL_CAP 4096

static volatile bool *s_abort_flag = NULL;
static TaskHandle_t   s_abort_owner = NULL;

void qcm_llm_http_arm_abort(volatile bool *flag)
{
    TaskHandle_t self = xTaskGetCurrentTaskHandle();
    QOSA_ASSERT(s_abort_flag == NULL || s_abort_owner == self);
    s_abort_flag = flag;
    s_abort_owner = self;
}

void qcm_llm_http_disarm_abort(void)
{
    if (s_abort_owner == xTaskGetCurrentTaskHandle()) {
        s_abort_flag = NULL;
        s_abort_owner = NULL;
    }
}

static inline bool abort_requested(void)
{
    return s_abort_flag &&
           s_abort_owner == xTaskGetCurrentTaskHandle() &&
           *s_abort_flag;
}

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} response_buffer_t;

static char *dup_printf(const char *fmt, ...)
{
    va_list args;
    va_list copy;
    int needed;
    char *buf;

    va_start(args, fmt);
    va_copy(copy, args);
    needed = vsnprintf(NULL, 0, fmt, copy);
    va_end(copy);
    if (needed < 0) {
        va_end(args);
        return NULL;
    }

    buf = qosa_calloc(1, (size_t)needed + 1);

    if (!buf) {
        va_end(args);
        return NULL;
    }

    vsnprintf(buf, (size_t)needed + 1, fmt, args);
    va_end(args);
    return buf;
}

static char *sanitize_utf8_body_copy(const char *body)
{
    size_t src = 0;
    size_t dst = 0;
    size_t len = 0;
    char *sanitized = NULL;

    if (!body) {
        return NULL;
    }

    len = strlen(body);

    sanitized = qosa_calloc(1, len + 1);

    if (!sanitized) {
        return NULL;
    }

    while (body[src]) {
        unsigned char c = (unsigned char)body[src];

        if (c < 0x80) {
            sanitized[dst++] = body[src++];
        } else if (c >= 0xC2 && c <= 0xDF) {
            if (body[src + 1] && ((unsigned char)body[src + 1] & 0xC0) == 0x80) {
                sanitized[dst++] = body[src++];
                sanitized[dst++] = body[src++];
            } else {
                sanitized[dst++] = ' ';
                src++;
            }
        } else if (c >= 0xE0 && c <= 0xEF) {
            if (body[src + 1] && body[src + 2] &&
                    ((unsigned char)body[src + 1] & 0xC0) == 0x80 &&
                    ((unsigned char)body[src + 2] & 0xC0) == 0x80) {
                sanitized[dst++] = body[src++];
                sanitized[dst++] = body[src++];
                sanitized[dst++] = body[src++];
            } else {
                sanitized[dst++] = ' ';
                src++;
            }
        } else if (c >= 0xF0 && c <= 0xF4) {
            if (body[src + 1] && body[src + 2] && body[src + 3] &&
                    ((unsigned char)body[src + 1] & 0xC0) == 0x80 &&
                    ((unsigned char)body[src + 2] & 0xC0) == 0x80 &&
                    ((unsigned char)body[src + 3] & 0xC0) == 0x80) {
                sanitized[dst++] = body[src++];
                sanitized[dst++] = body[src++];
                sanitized[dst++] = body[src++];
                sanitized[dst++] = body[src++];
            } else {
                sanitized[dst++] = ' ';
                src++;
            }
        } else {
            sanitized[dst++] = ' ';
            src++;
        }
    }

    sanitized[dst] = '\0';
    return sanitized;
}

static qcm_llm_error_e response_buffer_init(response_buffer_t *buffer)
{
    if (!buffer) {
        return QCM_LLM_ERR_INVALID_ARG;
    }

    buffer->data = qosa_calloc(1, QCM_LLM_HTTP_RB_INITIAL_CAP);

    if (!buffer->data) {
        return QCM_LLM_ERR_NO_MEM;
    }

    buffer->cap = QCM_LLM_HTTP_RB_INITIAL_CAP;
    buffer->len = 0;
    return QCM_LLM_OK;
}

static qcm_llm_error_e response_buffer_append(response_buffer_t *buffer, const char *data, size_t len)
{
    char *grown;
    size_t cap;

    if (!buffer || !data) {
        return QCM_LLM_ERR_INVALID_ARG;
    }
    cap = buffer->cap;
    while (buffer->len + len + 1 > cap) {
        cap *= 2;
    }

    if (cap != buffer->cap) {
        grown = qosa_realloc(buffer->data, cap);

        if (!grown) {
            return QCM_LLM_ERR_NO_MEM;
        }
        buffer->data = grown;
        buffer->cap = cap;
    }

    memcpy(buffer->data + buffer->len, data, len);
    buffer->len += len;
    buffer->data[buffer->len] = '\0';
    return QCM_LLM_OK;
}

static void response_buffer_free(response_buffer_t *buffer)
{
    if (!buffer) {
        return;
    }


    qosa_free(buffer->data);

    memset(buffer, 0, sizeof(*buffer));
}

typedef struct {
    response_buffer_t *buffer;
    const qcm_llm_http_json_request_t *request;
    const char *body;
    const char *auth_header_value;
    int status_code;
    bool done;
    bool failed;
} mongoose_http_context_t;

static const char *auth_header_name(const char *auth_type)
{
    if (auth_type && qosa_strcmp(auth_type, "api-key") == 0) {
        return "X-API-Key";
    }
    return "Authorization";
}

static void mongoose_http_send_request(struct mg_connection *connection,
                                       mongoose_http_context_t *context)
{
    struct mg_str host = mg_url_host(context->request->url);
    const char *uri = mg_url_uri(context->request->url);
    size_t body_len = qosa_strlen(context->body);
    size_t i;

    if (mg_printf(connection,
                  "POST %s HTTP/1.1\r\nHost: %.*s\r\n"
                  "Content-Type: application/json\r\nConnection: close\r\n",
                  uri, (int) host.len, host.ptr) == 0) {
        context->failed = true;
        context->done = true;
        return;
    }
    if (context->auth_header_value &&
        mg_printf(connection, "%s: %s\r\n",
                  auth_header_name(context->request->auth_type),
                  context->auth_header_value) == 0) {
        context->failed = true;
        context->done = true;
        return;
    }
    for (i = 0; i < context->request->header_count; i++) {
        const qcm_llm_http_header_t *header = &context->request->headers[i];

        if (header->name && header->name[0] && header->value &&
            mg_printf(connection, "%s: %s\r\n", header->name,
                      header->value) == 0) {
            context->failed = true;
            context->done = true;
            return;
        }
    }
    if (mg_printf(connection, "Content-Length: %lu\r\n\r\n",
                  (unsigned long) body_len) == 0 ||
        !mg_send(connection, context->body, body_len)) {
        context->failed = true;
        context->done = true;
    }
}

static void mongoose_http_event_handler(struct mg_connection *connection,
                                       int event, void *event_data,
                                       void *fn_data)
{
    mongoose_http_context_t *context = (mongoose_http_context_t *) fn_data;

    if (abort_requested()) {
        context->failed = true;
        context->done = true;
        return;
    }
    if (event == MG_EV_CONNECT) {
        if (strncmp(context->request->url, "https://", 8) == 0) {
            struct mg_tls_opts tls_options = {0};
            tls_options.srvname = mg_url_host(context->request->url);
            mg_tls_init(connection, &tls_options);
        } else {
            mongoose_http_send_request(connection, context);
        }
    } else if (event == MG_EV_TLS_HS) {
        mongoose_http_send_request(connection, context);
    } else if (event == MG_EV_HTTP_MSG) {
        struct mg_http_message *message = (struct mg_http_message *) event_data;
        context->status_code = mg_http_status(message);
        if (response_buffer_append(context->buffer, message->body.ptr,
                                   message->body.len) != QCM_LLM_OK) {
            context->failed = true;
        }
        context->done = true;
    } else if (event == MG_EV_ERROR) {
        context->failed = true;
        context->done = true;
    }
}

static char *build_auth_header_value(const char *auth_type, const char *api_key)
{
    const char *kind = auth_type ? auth_type : "bearer";

    if (!api_key || !api_key[0]) {
        return NULL;
    }
    if (qosa_strcmp(kind, "none") == 0) {
        return NULL;
    }
    if (qosa_strcmp(kind, "api-key") == 0) {
        return qosa_strdup(api_key);
    }

    return dup_printf("Bearer %s", api_key);
}

static char *parse_error_message_body(const char *body, int status)
{
    cJSON *root;
    cJSON *error;
    cJSON *message;
    char *fallback;

    if (!body || !body[0]) {
        return dup_printf("HTTP %d", status);
    }
    QLOGI("llm http error:%s\n", body);
    root = cJSON_Parse(body);
    if (!root) {
        return dup_printf("HTTP %d: %.160s", status, body);
    }

    error = cJSON_GetObjectItem(root, "error");
    if (error && cJSON_IsObject(error)) {
        message = cJSON_GetObjectItem(error, "message");
        if (message && cJSON_IsString(message) && message->valuestring[0]) {
            fallback = dup_printf("HTTP %d: %s", status, message->valuestring);
            cJSON_Delete(root);
            return fallback;
        }
    }

    message = cJSON_GetObjectItem(root, "message");
    if (message && cJSON_IsString(message) && message->valuestring[0]) {
        fallback = dup_printf("HTTP %d: %s", status, message->valuestring);
        cJSON_Delete(root);
        return fallback;
    }

    cJSON_Delete(root);
    return dup_printf("HTTP %d: %.160s", status, body);
}

qcm_llm_error_e qcm_llm_http_post_json(const qcm_llm_http_json_request_t *request,
                                  qcm_llm_http_response_t *out_response,
                                  char **out_error_message)
{
    response_buffer_t buffer = {0};
    struct mg_mgr manager;
    struct mg_connection *connection;
    mongoose_http_context_t context = {0};
    char *auth_header_value = NULL;
    char *sanitized_body = NULL;
    qcm_llm_error_e err;
    uint64_t timeout_ms;
    uint64_t start_ms;

    if (out_response) {
        memset(out_response, 0, sizeof(*out_response));
    }
    if (out_error_message) {
        *out_error_message = NULL;
    }
    if (!request || !request->url || !request->body || !out_response || !out_error_message) {
        return QCM_LLM_ERR_INVALID_ARG;
    }

    sanitized_body = sanitize_utf8_body_copy(request->body);
    if (!sanitized_body) {
        *out_error_message = dup_printf("Out of memory sanitizing HTTP request body");
        QLOGE("OOM sanitizing HTTP request body");
        return QCM_LLM_ERR_NO_MEM;
    }

    err = response_buffer_init(&buffer);
    if (err != QCM_LLM_OK) {
        *out_error_message = dup_printf("Out of memory allocating HTTP buffer");
        QLOGE("OOM allocating HTTP response buffer");
        goto cleanup;
    }

    auth_header_value = build_auth_header_value(request->auth_type, request->api_key);
    context.buffer = &buffer;
    context.request = request;
    context.body = sanitized_body;
    context.auth_header_value = auth_header_value;
    timeout_ms = request->timeout_ms ? request->timeout_ms : 120000;
    start_ms = mg_millis();
    mg_mgr_init(&manager);
    QLOGD("POST %s", request->url);
    connection = mg_http_connect(&manager, request->url,
                                 mongoose_http_event_handler, &context);
    if (connection == NULL) {
        context.failed = true;
        context.done = true;
    }
    while (!context.done && mg_millis() - start_ms < timeout_ms) {
        if (abort_requested()) {
            context.failed = true;
            context.done = true;
            break;
        }
        mg_mgr_poll(&manager, 50);
    }
    if (!context.done) {
        context.failed = true;
    }
    mg_mgr_free(&manager);

    if (abort_requested()) {
        *out_error_message = dup_printf("HTTP request aborted by caller");
        err = QCM_LLM_ERR_INVALID_STATE;
        goto cleanup;
    }
    if (context.failed) {
        *out_error_message = dup_printf("HTTP request failed or timed out");
        QLOGE("HTTP request failed for %s", request->url);
        err = QCM_LLM_FAIL;
        goto cleanup;
    }

    QLOGD("HTTP status=%d", context.status_code);
    if (context.status_code != 200) {
        err = QCM_LLM_FAIL;
        *out_error_message = parse_error_message_body(buffer.data, context.status_code);
        QLOGE("LLM error: %s\n", *out_error_message ? *out_error_message : "(null)");
        goto cleanup;
    }

    out_response->status_code = context.status_code;
    out_response->body = buffer.data;
    buffer.data = NULL;
    err = QCM_LLM_OK;

cleanup:
    qosa_free(auth_header_value);
    qosa_free(sanitized_body);

    response_buffer_free(&buffer);
    return err;
}

void qcm_llm_http_response_free(qcm_llm_http_response_t *response)
{
    if (!response) {
        return;
    }

    qosa_free(response->body);

    memset(response, 0, sizeof(*response));
}
