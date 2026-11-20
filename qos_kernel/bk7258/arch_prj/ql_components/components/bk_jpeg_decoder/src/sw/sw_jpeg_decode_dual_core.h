// Copyright 2020-2023 Beken
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

#include "bk_jpeg_decode_ctlr.h"

#ifdef __cplusplus
extern "C" {
#endif

bk_err_t software_decode_task_dual_core_send_frame(private_jpeg_decode_sw_multi_core_ctlr_t *controller, frame_buffer_t *in_frame);
bk_err_t software_decode_task_dual_core_open(private_jpeg_decode_sw_multi_core_ctlr_t *controller);
bk_err_t software_decode_task_dual_core_close(private_jpeg_decode_sw_multi_core_ctlr_t *controller);

#ifdef __cplusplus
}
#endif

