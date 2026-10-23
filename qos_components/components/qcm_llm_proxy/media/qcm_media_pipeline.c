/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "qcm_media_pipeline.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include "qosa_def.h"
#include "qosa_log.h"
#include "qosa_virtual_file.h"

#include "mbedtls/base64.h"

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

static const char *image_mime_from_path(const char *path)
{
    const char *dot;

    if (!path || !path[0]) {
        return NULL;
    }

    dot = qosa_strrchr(path, '.');
    if (!dot) {
        return NULL;
    }

    if (qosa_strcasecmp(dot, ".jpg") == 0 || qosa_strcasecmp(dot, ".jpeg") == 0) {
        return "image/jpeg";
    }
    if (qosa_strcasecmp(dot, ".png") == 0) {
        return "image/png";
    }
    if (qosa_strcasecmp(dot, ".gif") == 0) {
        return "image/gif";
    }
    if (qosa_strcasecmp(dot, ".webp") == 0) {
        return "image/webp";
    }

    return NULL;
}

static qcm_llm_error_e prepare_local_path_asset(const qcm_media_asset_t *asset,
                                          const qcm_llm_model_profile_t *profile,
                                          size_t image_max_bytes,
                                          qcm_media_prepared_t *out_prepared,
                                          char **out_error_message)
{
    struct qosa_vfs_stat_t st = {0};
    int file;
    unsigned char *raw = NULL;
    unsigned char *encoded = NULL;
    size_t encoded_len = 0;
    size_t prefix_len;
    size_t read_len;
    const char *mime;
    char *data_url = NULL;

    if (!asset->path || !asset->path[0]) {
        *out_error_message = dup_printf("media path is empty");
        return QCM_LLM_ERR_INVALID_ARG;
    }
    if (asset->path[0] != '/') {
        *out_error_message = dup_printf("media path must be an absolute path");
        return QCM_LLM_ERR_INVALID_ARG;
    }

    mime = asset->mime_type ? asset->mime_type : image_mime_from_path(asset->path);
    if (!mime) {
        *out_error_message = dup_printf("Only local jpg/jpeg/png/gif/webp files are supported");
        return QCM_LLM_ERR_NOT_SUPPORTED;
    }
    if (qosa_vfs_stat(asset->path, &st) != 0) {
        *out_error_message = dup_printf("Media file not found: %s", asset->path);
        return QCM_LLM_ERR_NOT_FOUND;
    }
    if (st.st_size <= 0) {
        *out_error_message = dup_printf("Media file is empty: %s", asset->path);
        return QCM_LLM_ERR_INVALID_SIZE;
    }
    if ((size_t)st.st_size > image_max_bytes) {
        *out_error_message = dup_printf("Media file is too large (%ld bytes > %u bytes)",
                                        (long)st.st_size,
                                        (unsigned)image_max_bytes);
        return QCM_LLM_ERR_INVALID_SIZE;
    }

    file = qosa_vfs_open(asset->path, QOSA_VFS_O_RDONLY);
    if (file < 0) {
        *out_error_message = dup_printf("Failed to open media file: %s", asset->path);
        return QCM_LLM_FAIL;
    }
    raw = qosa_malloc((size_t)st.st_size);    
    if (!raw) {
        *out_error_message = dup_printf("Out of memory reading media");
        qosa_vfs_close(file);
        return QCM_LLM_ERR_NO_MEM;
    }

    read_len = qosa_vfs_read(file, raw, (size_t)st.st_size);
    qosa_vfs_close(file);
    if (read_len != (size_t)st.st_size) {
        qosa_free(raw);
        *out_error_message = dup_printf("Failed to read full media file: %s", asset->path);
        return QCM_LLM_FAIL;
    }

    encoded_len = ((size_t)st.st_size + 2) / 3 * 4;
    encoded = qosa_calloc(1, encoded_len + 1);
    if (!encoded) {
        qosa_free(raw);

        *out_error_message = dup_printf("Out of memory encoding media");
        return QCM_LLM_ERR_NO_MEM;
    }

    if (mbedtls_base64_encode(encoded, encoded_len + 1, &encoded_len, raw, (size_t)st.st_size) != 0) {
        qosa_free(raw);
        qosa_free(encoded);
        *out_error_message = dup_printf("Failed to base64-encode media");
        return QCM_LLM_FAIL;
    }

    prefix_len = qosa_strlen("data:") + qosa_strlen(mime) + qosa_strlen(";base64,");
    data_url = qosa_calloc(1, prefix_len + encoded_len + 1);
    if (!data_url) {

        qosa_free(raw);
        qosa_free(encoded);

        *out_error_message = dup_printf("Out of memory building media payload");
        return QCM_LLM_ERR_NO_MEM;
    }

    qosa_snprintf(data_url, prefix_len + 1, "data:%s;base64,", mime);
    qosa_memcpy(data_url + prefix_len, encoded, encoded_len);
    data_url[prefix_len + encoded_len] = '\0';

    out_prepared->kind = QCM_MEDIA_PREPARED_KIND_DATA_URL;
    out_prepared->payload = data_url;
    out_prepared->original_size = (size_t)st.st_size;
    qosa_strlcpy(out_prepared->mime_type, mime, sizeof(out_prepared->mime_type));


    qosa_free(raw);
    qosa_free(encoded);

    (void)profile;
    return QCM_LLM_OK;
}

qcm_llm_error_e qcm_media_prepare_asset(const qcm_media_asset_t *asset,
                                   const qcm_llm_model_profile_t *profile,
                                   size_t image_max_bytes,
                                   qcm_media_prepared_t *out_prepared,
                                   char **out_error_message)
{
    if (out_prepared) {
        memset(out_prepared, 0, sizeof(*out_prepared));
    }
    if (out_error_message) {
        *out_error_message = NULL;
    }
    if (!asset || !profile || !out_prepared || !out_error_message) {
        return QCM_LLM_ERR_INVALID_ARG;
    }

    if (asset->kind == QCM_MEDIA_ASSET_KIND_REMOTE_URL) {
        if (!asset->url || !asset->url[0]) {
            *out_error_message = dup_printf("media url is empty");
            return QCM_LLM_ERR_INVALID_ARG;
        }
        out_prepared->kind = QCM_MEDIA_PREPARED_KIND_REMOTE_URL;
        out_prepared->payload = qosa_strdup(asset->url);
        if (!out_prepared->payload) {
            *out_error_message = dup_printf("Out of memory copying media URL");
            return QCM_LLM_ERR_NO_MEM;
        }
        return QCM_LLM_OK;
    }

    if (asset->kind != QCM_MEDIA_ASSET_KIND_LOCAL_PATH) {
        *out_error_message = dup_printf("Unsupported media asset kind");
        return QCM_LLM_ERR_NOT_SUPPORTED;
    }

    if (profile->image_remote_url_only) {
        *out_error_message = dup_printf("Selected profile only supports remote image URLs");
        return QCM_LLM_ERR_NOT_SUPPORTED;
    }

    return prepare_local_path_asset(asset, profile, image_max_bytes, out_prepared, out_error_message);
}

void qcm_media_prepared_free(qcm_media_prepared_t *prepared)
{
    if (!prepared) {
        return;
    }

    qosa_free(prepared->payload);
    memset(prepared, 0, sizeof(*prepared));
}
