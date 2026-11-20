// Copyright 2020-2021 Beken
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

#pragma once

#include "mux_pipeline.h"
#include "components/bk_video_pipeline/bk_video_pipeline.h"

typedef struct bk_video_pipeline *bk_video_pipeline_ctlr_handle_t;
typedef struct bk_video_pipeline bk_video_pipeline_ctlr_t;
typedef struct bk_video_pipeline_config bk_video_pipeline_ctlr_config_t;

typedef struct
{
    video_pipeline_module_status_t h264e_enable;
    video_pipeline_module_status_t rotate_enable;
} private_video_pipeline_status_t;

typedef struct
{
    uint32_t event;
    void *data;
} private_video_pipeline_queue_msg_t;

typedef struct
{
    bk_video_pipeline_ctlr_config_t config;
    bk_video_pipeline_decode_config_t decode_config;
    bk_video_pipeline_h264e_config_t h264e_config;
    bk_video_pipeline_ctlr_t ops;
    private_video_pipeline_status_t module_status;
} private_video_pipeline_ctlr_t;

avdk_err_t bk_video_pipeline_ctlr_new(bk_video_pipeline_ctlr_handle_t *handle, bk_video_pipeline_ctlr_config_t *config);
