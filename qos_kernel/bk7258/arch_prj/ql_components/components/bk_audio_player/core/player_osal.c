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


#include "player_osal.h"
#include <os/str.h>

int osal_init_mutex(osal_mutex_t *mutex)
{
    return rtos_init_mutex(mutex);
}

int osal_deinit_mutex(osal_mutex_t *mutex)
{
    return rtos_deinit_mutex(mutex);
}

int osal_lock_mutex(osal_mutex_t *mutex)
{
    return rtos_lock_mutex(mutex);
}

int osal_unlock_mutex(osal_mutex_t *mutex)
{
    return rtos_unlock_mutex(mutex);
}

int osal_init_sema(osal_sema_t *sema, int max_count, int init_count)
{
    return rtos_init_semaphore_ex(sema, max_count, init_count);
}

int osal_deinit_sema(osal_sema_t *sema)
{
    return rtos_deinit_semaphore(sema);
}

int osal_wait_sema(osal_sema_t *sema, int timeout_ms)
{
    return rtos_get_semaphore(sema, timeout_ms);
}

int osal_post_sema(osal_sema_t *sema)
{
    return rtos_set_semaphore(sema);
}

int osal_create_thread(osal_thread_t *thread, osal_thread_func function, uint32_t stack_size, char *name, osal_thread_arg_t arg, int priority)
{
    return rtos_create_thread(thread, (uint8_t)priority, name, function, stack_size, arg);
}

int osal_delete_thread(osal_thread_t *thread)
{
    return rtos_delete_thread(thread);
}

void bk_signal_init(bk_signal_t *signal)
{
    //  rtos_init_mutex(&signal->mutex);
    rtos_init_semaphore_ex(&signal->sema, 0x0FFF, 0);
}

void bk_signal_close(bk_signal_t *signal)
{
#if 0
    if (signal->mutex)
    {
        rtos_deinit_mutex(&signal->mutex);
        signal->mutex = NULL;
    }
#endif

    if (signal->sema)
    {
        rtos_deinit_semaphore(&signal->sema);
        signal->sema = NULL;
    }
}

int bk_signal_wait(bk_signal_t *signal, int ms)
{
    int ret;
    int count;
    int i;

    //  rtos_lock_mutex(&signal->mutex);
    count = rtos_get_semaphore_count(&signal->sema);
    for (i = 0; i < count; i++)
    {
        rtos_get_semaphore(&signal->sema, 0);    //clear outdated
    }
    //  rtos_unlock_mutex(&signal->mutex);

    ret = rtos_get_semaphore(&signal->sema, ms);
    return ret;
}

void bk_signal_signal(bk_signal_t *signal)
{
    //  rtos_lock_mutex(&signal->mutex);
    rtos_set_semaphore(&signal->sema);
    //  rtos_unlock_mutex(&signal->mutex);
}

void osal_usleep(int micro_seconds)
{
    rtos_delay_milliseconds(micro_seconds / 1000);
}

