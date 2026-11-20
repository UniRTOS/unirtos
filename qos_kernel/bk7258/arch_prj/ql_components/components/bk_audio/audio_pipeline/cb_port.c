// Copyright 2025-2026 Beken
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include <components/bk_audio/audio_pipeline/audio_types.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_error.h>
#include <components/bk_audio/audio_pipeline/audio_port.h>
#include <components/bk_audio/audio_pipeline/cb_port.h>

#define TAG  "CB_PORT"

// Check if cb_port and cb_port->cb are valid
#define CB_PORT_CHECK(cb_port, self, action) \
    do { \
        if (!(cb_port) || !(cb_port)->cb) { \
            BK_LOGE(TAG, "[%s] %s, cb_port:%p or cb_port->cb:%p is NULL\n", \
                    audio_port_get_tag(self), __func__, cb_port, (cb_port) ? (cb_port)->cb : NULL); \
            action; \
        } \
    } while (0)

typedef struct callback_port
{
    port_stream_func cb;
    void *ctx;
} callback_port_t;


static bk_err_t _callback_port_open(audio_port_handle_t self)
{
    //nothing todo

    return BK_OK;
}

static bk_err_t _callback_port_close(audio_port_handle_t self)
{
    //nothing todo

    return BK_OK;
}

static bk_err_t _callback_port_abort(audio_port_handle_t self)
{
    //nothing todo

    return BK_OK;
}

static bk_err_t _callback_port_reset(audio_port_handle_t self)
{
    //nothing todo

    return BK_OK;
}

static bk_err_t _callback_port_read(audio_port_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    AUDIO_MEM_CHECK(TAG, self, return BK_FAIL);
    callback_port_t *cb_port = (callback_port_t *)audio_port_get_data(self);
    CB_PORT_CHECK(cb_port, self, return BK_FAIL);

    return cb_port->cb(self, buffer, len, ticks_to_wait, cb_port->ctx);
}

static bk_err_t _callback_port_write(audio_port_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    AUDIO_MEM_CHECK(TAG, self, return BK_FAIL);
    callback_port_t *cb_port = (callback_port_t *)audio_port_get_data(self);
    CB_PORT_CHECK(cb_port, self, return BK_FAIL);

    return cb_port->cb(self, buffer, len, ticks_to_wait, cb_port->ctx);
}

static bk_err_t _callback_port_destroy(audio_port_handle_t self)
{
    AUDIO_MEM_CHECK(TAG, self, return BK_OK);
    callback_port_t *cb_port = (callback_port_t *)audio_port_get_data(self);

    if (cb_port)
    {
        audio_free(cb_port);
        cb_port = NULL;
    }

    return BK_OK;
}

static bk_err_t _callback_port_write_done(audio_port_handle_t self)
{
    //nothing todo

    return BK_OK;
}

audio_port_handle_t callback_port_init(callback_port_cfg_t *config)
{
    callback_port_t *cb_port = audio_calloc(1, sizeof(callback_port_t));
    AUDIO_MEM_CHECK(TAG, cb_port, return NULL);

    cb_port->cb = config->cb;
    cb_port->ctx = config->ctx;

    audio_port_cfg_t cfg = {0};
    cfg.open = _callback_port_open;
    cfg.close = _callback_port_close;
    cfg.destroy = _callback_port_destroy;
    cfg.abort = _callback_port_abort;
    cfg.reset = _callback_port_reset;
    cfg.read = _callback_port_read;
    cfg.write = _callback_port_write;
    cfg.write_done = _callback_port_write_done;
    audio_port_handle_t port = audio_port_init(&cfg);
    AUDIO_MEM_CHECK(TAG, port, goto fail);
    audio_port_set_type(port, PORT_TYPE_CB);
    audio_port_set_data(port, cb_port);

    BK_LOGD(TAG, "callback port init, port:%p\n", port);
    return port;

fail:

    if (cb_port)
    {
        audio_free(cb_port);
        cb_port = NULL;
    }

    return NULL;
}

