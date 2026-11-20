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

#include <common/bk_include.h>
#include <components/log.h>
#include <common/bk_err.h>
#include <components/system.h>
#include <driver/wdt.h>
#include "bk_misc.h"
#include "reset_reason.h"
#include "drv_model_pub.h"
#include "bk_wifi_types.h"
#include "bk_wifi.h"
#include "aon_pmu_driver.h"
#include "wdt_driver.h"
#include <modules/pm.h>
#include "mb_ipc_cmd.h"

#define TAG "sys"


#define REBOOT_CALLBACK_FUNC_MAX  8
static reboot_callback_func s_reboot_cb[REBOOT_CALLBACK_FUNC_MAX] = {NULL};

static void bk_reboot_callback_exe(void)
{
	for (size_t i = 0; i < REBOOT_CALLBACK_FUNC_MAX; i++) {
		if (s_reboot_cb[i] != NULL) {
			s_reboot_cb[i]();
		}
	}

}

void bk_reboot_callback_register(reboot_callback_func func)
{

	for (size_t i = 0; i < REBOOT_CALLBACK_FUNC_MAX; i++) {
		if (s_reboot_cb[i] == NULL) {
			s_reboot_cb[i] = func;
			return;
		}
	}
	BK_LOGE(TAG, "number of reboot callback function is up to max.\r\n");
}

void bk_reboot_ex(uint32_t reset_reason)
{
	BK_LOGD(TAG, "cpu1 reboot\r\n");
	
	if(reset_reason < RESET_SOURCE_UNKNOWN) {
		bk_misc_set_reset_reason(reset_reason);
	}

	bk_reboot_callback_exe();
	
	ipc_send_cpu1_need_reboot();
	while(1);
}

void bk_reboot(void)
{
	bk_reboot_ex(RESET_SOURCE_REBOOT);
}
