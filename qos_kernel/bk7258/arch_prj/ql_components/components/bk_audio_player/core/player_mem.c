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


#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <stdlib.h>
#include "string.h"
#include "player_mem.h"

//#define ENABLE_PLAYER_MEM_TRACE

#define TAG "PLAYER_MEM"

void *player_malloc(uint32_t size)
{
    void *data =  NULL;

#if CONFIG_AUDIO_PLAYER_USE_SRAM
    data = os_malloc(size);
#else
    data = psram_malloc(size);
#endif

#ifdef ENABLE_PLAYER_MEM_TRACE
    BK_LOGI(TAG, "malloc:%p, size:%d, called:0x%08x \n", data, size, (intptr_t)__builtin_return_address(0) - 2);
#endif
    if (data)
    {
        os_memset(data, 0, size);
    }
    return data;
}

void player_free(void *ptr)
{
    os_free(ptr);
#ifdef ENABLE_PLAYER_MEM_TRACE
    BK_LOGI(TAG, "free:%p, called:0x%08x \n", ptr, (intptr_t)__builtin_return_address(0) - 2);
#endif
}

void *player_realloc(void *ptr, uint32_t size)
{
    void *p = NULL;
    p = os_realloc(ptr, size);
#ifdef ENABLE_PLAYER_MEM_TRACE
    BK_LOGI(TAG, "realloc,new:%p, ptr:%p size:%d, called:0x%08x \n", p, ptr, size, (intptr_t)__builtin_return_address(0) - 2);
#endif
    return p;
}

char *player_strdup(const char *str)
{
#if CONFIG_AUDIO_PLAYER_USE_SRAM
    char *copy = os_malloc(os_strlen(str) + 1);
#else
    char *copy = psram_malloc(os_strlen(str) + 1);
#endif
    if (copy)
    {
        os_strcpy(copy, str);
    }
#ifdef ENABLE_PLAYER_MEM_TRACE
    BK_LOGI(TAG, "strdup:%p, size:%d, called:0x%08x \n", copy, os_strlen(copy), (intptr_t)__builtin_return_address(0) - 2);
#endif
    return copy;
}

void player_mem_print(char *tag, int line, const char *func)
{
    BK_LOGI(TAG, "Func:%s, Line:%d, MEM Total:%d Bytes\r\n", func, line, rtos_get_free_heap_size());
}

