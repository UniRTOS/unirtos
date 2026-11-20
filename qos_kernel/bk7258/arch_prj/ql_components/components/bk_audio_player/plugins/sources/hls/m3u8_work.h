// Copyright 2024-2025 Beken
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


#ifndef _M3U8_WORK_H_
#define _M3U8_WORK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <modules/m3u8.h>


#define M3U8_ITEM_SZ            (128)

typedef struct m3u8_session
{
    EventGroupHandle_t state_event;
    int running;
    osal_thread_t tid;
    osal_sema_t thread_sem;

    top_m3u8_info_t m3u8_info;
} m3u8_session_t;

/**
 * @brief   Malloc memory in player
 *
 * @param[in]  size   memory size
 *
 * @return
 *     - valid pointer on success
 *     - NULL when any errors
 */
int hls_m3u8_init(top_m3u8_info_t *top_m3u8, char *top_m3u8_url);

int hls_m3u8_deinit(top_m3u8_info_t *top_m3u8);

int hls_m3u8_fetch(m3u8_session_t *m3u8, const char *url);

int hls_m3u8_thread_create(m3u8_session_t *m3u8);

int hls_m3u8_thread_destroy(m3u8_session_t *m3u8);

#ifdef __cplusplus
}
#endif

#endif /*_M3U8_WORK_H_*/
