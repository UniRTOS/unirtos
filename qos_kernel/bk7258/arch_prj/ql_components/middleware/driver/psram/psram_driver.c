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
#include <stdint.h>
#include <stdbool.h>
#include <driver/int.h>
#include <os/mem.h>
#include "clock_driver.h"
#include "sys_driver.h"
#include "psram_hal.h"
#include "driver/psram_types.h"
#include "psram_driver.h"
#include <driver/psram.h>
#include <modules/pm.h>
#if (CONFIG_PSRAM_AUTO_DETECT)
#include "bk_ef.h"
#endif

#define PSRAM_CHECK_FLAG   0x3CA5C3A5

#define PSRAM_ADDRESS_ALIGNMENT    4
#define PSRAM_BYTES_PER_WORD       4
#define PSRAM_MAX_STRING_LEN       1024

typedef struct {
	uint32_t psram_id;
	uint32_t magic_code;
} psram_flash_t;

#if (CONFIG_PSRAM_AUTO_DETECT)
static bool s_psram_id_need_write = false;
static beken_semaphore_t s_psram_sem = NULL;
static beken_thread_t psram_task = NULL;
#endif

extern void bk_delay_us(uint32_t us);
static bool s_psram_server_is_init = false;
static bool s_psram_heap_is_init = false;
static beken_mutex_t s_psram_channel_mutex = NULL;
static uint8_t s_psram_channelmap = 0;

#define PSRAM_RETURN_ON_SERVER_NOT_INIT() do {\
				if (!s_psram_server_is_init) {\
					return BK_ERR_PSRAM_SERVER_NOT_INIT;\
				}\
			} while(0)

bk_err_t bk_psram_set_clk(psram_clk_t clk)
{
	bk_err_t ret = BK_OK;

	psram_hal_set_clk(clk);

	return ret;
}
bk_err_t bk_psram_heap_init_flag_set(bool init)
{
	bk_err_t ret = BK_OK;
	s_psram_heap_is_init = init;
	return ret;
}
bool bk_psram_heap_init_flag_get()
{
	return s_psram_heap_is_init;
}

bk_err_t bk_psram_set_voltage(psram_voltage_t voltage)
{
	bk_err_t ret = BK_OK;

	psram_hal_set_voltage(voltage);

	return ret;
}

bk_err_t bk_psram_set_transfer_mode(psram_tansfer_mode_t transfer_mode)
{
	bk_err_t ret = BK_OK;

	psram_hal_set_transfer_mode(transfer_mode);

	return ret;
}

psram_write_through_area_t bk_psram_alloc_write_through_channel(void)
{
	uint8_t channel = 0;
	bk_err_t ret = BK_OK;
	
	if (s_psram_channel_mutex == NULL)
	{
		ret = rtos_init_mutex(&s_psram_channel_mutex);
		if (ret != BK_OK) {
			PSRAM_LOGE("Failed to create psram channel mutex\n");
			return channel;
		}
	}

	if (s_psram_channel_mutex) {
		rtos_lock_mutex(&s_psram_channel_mutex);
	}
	
	for (channel = 0; channel < PSRAM_WRITE_THROUGH_AREA_COUNT; channel++)
	{
		if ((s_psram_channelmap & (0x1 << channel)) == 0)
		{
			s_psram_channelmap |= (0x1 << channel);
			break;
		}
	}
	
	if (s_psram_channel_mutex) {
		rtos_unlock_mutex(&s_psram_channel_mutex);
	}

	return channel;
}

bk_err_t bk_psram_free_write_through_channel(psram_write_through_area_t area)
{
	if (area >= PSRAM_WRITE_THROUGH_AREA_COUNT)
	{
		PSRAM_LOGE("%s over range failed\r\n", __func__);
		return BK_ERR_PARAM;
	}

	if (s_psram_channel_mutex) {
		rtos_lock_mutex(&s_psram_channel_mutex);
	}
	
	if (s_psram_channelmap & (0x1 << area)) {
		s_psram_channelmap &= ~(0x1 << area);
	}
	
	if (s_psram_channel_mutex) {
		rtos_unlock_mutex(&s_psram_channel_mutex);
	}

	return BK_OK;
}

bk_err_t  bk_psram_enable_write_through(psram_write_through_area_t area, uint32_t start, uint32_t end)
{
	return psram_hal_set_write_through(area, 1, start, end);
}

bk_err_t bk_psram_disable_write_through(psram_write_through_area_t area)
{
	return psram_hal_set_write_through(area, 0, 0, 0);
}

bk_err_t bk_psram_calibrate(void)
{
#if CONFIG_PSRAM_CALIBRATE
	//TODO add calibrate strategy after get it from digital team
	return BK_OK;
#else
	return BK_OK;
#endif
}


bk_err_t bk_psram_id_auto_detect(void)
{
	//psram only init in cp, so return BK_FAIL
	return BK_FAIL;
}

bk_err_t bk_psram_init(void)
{
	//psram only init in cp, so return BK_FAIL
	return BK_FAIL;
}

bk_err_t bk_psram_deinit(void)
{
	//psram only init in cp, so return BK_FAIL
	return BK_FAIL;
}

bk_err_t bk_psram_memcpy(uint8_t *start_addr, uint8_t *data_buf, uint32_t len)
{
	int i;
	uint32_t val;
	uint8_t *pb = NULL, *pd = NULL;

	PSRAM_RETURN_ON_SERVER_NOT_INIT();

	if (((uint32_t)start_addr & (PSRAM_ADDRESS_ALIGNMENT - 1)) != 0 || 
	    ((uint32_t)data_buf & (PSRAM_ADDRESS_ALIGNMENT - 1)) != 0)
	{
		PSRAM_LOGE("address not aligned to %d bytes\r\n", PSRAM_ADDRESS_ALIGNMENT);
		return BK_FAIL;
	}

	while (len) {
		if (len < PSRAM_BYTES_PER_WORD) {
			val = *((uint32_t *)(start_addr));
			pb = (uint8_t *)&val;
			pd = (uint8_t *)data_buf;
			for (i = 0; i < len; i++) {
				*pb++ = *pd++;
			}
			*(uint32_t *)(start_addr) = val;
			len = 0;
		} else {
			val = *((uint32_t *)data_buf);
			*(uint32_t *)(start_addr) = val;
			data_buf += PSRAM_BYTES_PER_WORD;
			start_addr += PSRAM_BYTES_PER_WORD;
			len -= PSRAM_BYTES_PER_WORD;
		}
	}

	return BK_OK;
}

bk_err_t bk_psram_memread(uint8_t *start_addr, uint8_t *data_buf, uint32_t len)
{
	int i;
	uint32_t val;
	uint8_t *pb, *pd;

	PSRAM_RETURN_ON_SERVER_NOT_INIT();

	if (((uint32_t)start_addr & (PSRAM_ADDRESS_ALIGNMENT - 1)) != 0 || 
	    ((uint32_t)data_buf & (PSRAM_ADDRESS_ALIGNMENT - 1)) != 0)
	{
		PSRAM_LOGE("address not aligned to %d bytes\r\n", PSRAM_ADDRESS_ALIGNMENT);
		return BK_FAIL;
	}

	while (len) {
		if (len < PSRAM_BYTES_PER_WORD) {
			val = *((uint32_t *)(start_addr));
			pb = (uint8_t *)&val;
			pd = (uint8_t *)data_buf;
			for (i = 0; i < len; i++) {
				*pd++ = *pb++;
			}
			len = 0;
		} else {
			val = *((uint32_t *)(start_addr));
			*((uint32_t *)data_buf) = val;
			data_buf += PSRAM_BYTES_PER_WORD;
			start_addr += PSRAM_BYTES_PER_WORD;
			len -= PSRAM_BYTES_PER_WORD;
		}
	}

	return BK_OK;
}

char *bk_psram_strcat(char *start_addr, const char *data_buf)
{
	int i, j;
	uint32_t val;
	uint8_t *pb;
	uint8_t *pd = (uint8_t *)data_buf;
	uint32_t max_iterations = PSRAM_MAX_STRING_LEN / PSRAM_BYTES_PER_WORD;
	uint32_t iteration_count = 0;

	if (!s_psram_server_is_init) {
		return NULL;
	}

	if (start_addr == NULL || data_buf == NULL) {
		return NULL;
	}

	if (*pd == '\0') {
		return start_addr;
	}

	do {
		if (iteration_count++ > max_iterations) {
			PSRAM_LOGE("String too long or no null terminator found\r\n");
			return NULL;
		}
		
		val = *((uint32_t *)(start_addr));
		pb = (uint8_t *)&val;
		
		// Find the end of the existing string
		for (i = 0; i < PSRAM_BYTES_PER_WORD; i++) {
			if (*(pb + i) == '\0') {
				// Found end of existing string, append new data
				for (j = i; j < PSRAM_BYTES_PER_WORD && *pd != '\0'; j++) {
					*(pb + j) = *pd++;
				}
				
				// If we've reached the end of the input string, add null terminator
				if (*pd == '\0' && j < PSRAM_BYTES_PER_WORD) {
					*(pb + j) = '\0';
				}
				
				*((uint32_t *)(start_addr)) = val;
				
				if (*pd == '\0') {
					return start_addr;
				}
				break;
			}
		}
		
		// If no null terminator found in this word, move to next word
		if (i == PSRAM_BYTES_PER_WORD) {
			start_addr += PSRAM_BYTES_PER_WORD;
		}
		
	} while (*pd != '\0');

	return start_addr;
}



