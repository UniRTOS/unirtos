/*
 * Copyright (c) Quectel Wireless Solution, Co., Ltd.All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_log.h"
#include "unirtos_init.h"
#include "qosa_app_main.h"
#include "qosa_event_notify.h"

#define main_printf(...) QOSA_LOG_E(LOG_TAG, ##__VA_ARGS__)

//static int g_temp_debug_count = 0;

void abort(void)
{
    while (1)
        ;
}

void unirtos_demo_thread(void *argv)
{    
    ql_trace("unirtos_demo_thread into\n");

    ql_trace("app init start\n");
    unirtos_init();
    while (1)
    {
        qosa_task_sleep_sec(100);
        //ql_trace("unirtos_demo_thread %d\n", g_temp_debug_count++);
        /* code */
    }
}

application_init(unirtos_demo_thread, "unirtos_app", 4, 3, QOSA_NULL);
