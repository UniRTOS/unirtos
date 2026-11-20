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


#ifndef __PLAYER_OSAL_H__
#define __PLAYER_OSAL_H__

#include "bk_audio_player_private.h"

#include <sys/time.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <os/os.h>
#include <os/mem.h>

#include "player_mem.h"

#define AUDIO_PLAYER_TAG "aud_play"

typedef beken_mutex_t osal_mutex_t;
typedef beken_semaphore_t osal_sema_t;
typedef beken_thread_t osal_thread_t;
typedef beken_thread_arg_t osal_thread_arg_t;
typedef beken_thread_function_t osal_thread_func;
typedef void osal_thread_return_t;

typedef struct bk_signal_s
{
    beken_mutex_t mutex;
    beken_semaphore_t sema;
} bk_signal_t;


int osal_init_mutex(osal_mutex_t *mutex);
int osal_deinit_mutex(osal_mutex_t *mutex);
int osal_lock_mutex(osal_mutex_t *mutex);
int osal_unlock_mutex(osal_mutex_t *mutex);

int osal_init_sema(osal_sema_t *sema, int max_count, int init_count);
int osal_deinit_sema(osal_sema_t *sema);
int osal_wait_sema(osal_sema_t *sema, int timeout_ms);
int osal_post_sema(osal_sema_t *sema);

int osal_create_thread(osal_thread_t *thread, osal_thread_func function, uint32_t stack_size, char *name, osal_thread_arg_t arg, int priority);
int osal_delete_thread(osal_thread_t *thread);

void bk_signal_init(bk_signal_t *signal);
void bk_signal_close(bk_signal_t *signal);
int bk_signal_wait(bk_signal_t *signal, int ms);
void bk_signal_signal(bk_signal_t *signal);

void osal_usleep(int micro_seconds);

#endif
