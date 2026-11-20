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

#include <common/bk_include.h>
#include <components/media_types.h>
#include <driver/psram_types.h>
#include "bk_list.h"
#include "FreeRTOS.h"
#include "event_groups.h"


#ifdef __cplusplus
extern "C" {
#endif

/*************************display*****************************/
frame_buffer_t *frame_buffer_display_malloc(uint32_t size);
void frame_buffer_display_free(frame_buffer_t *frame);

/*************************encode*****************************/
frame_buffer_t *frame_buffer_encode_malloc(uint32_t size);
void frame_buffer_encode_free(frame_buffer_t *frame);
#ifdef __cplusplus
}
#endif
