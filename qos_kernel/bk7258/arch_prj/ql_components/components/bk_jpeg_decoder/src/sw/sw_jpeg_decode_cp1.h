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


bk_err_t software_decode_task_send_msg_cp1(uint32_t type, uint32_t param);
bk_err_t software_decode_task_open_cp1(void *context);
bk_err_t software_decode_task_close_cp1();

#ifdef __cplusplus
}
#endif

