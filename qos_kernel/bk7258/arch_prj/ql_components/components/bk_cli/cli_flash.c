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

#include <driver/flash.h>
#include <driver/flash_partition.h>
#include "cli.h"
#include "flash_driver.h"
#if CONFIG_FLASH_CP_AP_DIRECT_ACCESS
#include "bk_wdt.h"
#include "flash_shared_lock.h"
#endif

#if CONFIG_TFM_FLASH_NSC
#include "tfm_flash_nsc.h"
#endif

static void cli_flash_help(void)
{
	CLI_LOGD("flash driver init\n");
	CLI_LOGD("flash_driver deinit\n");
	CLI_LOGD("flash {erase|write|read} [start_addr] [len]\n");
	CLI_LOGD("flash_partition show\n");
#if CONFIG_FLASH_CP_AP_DIRECT_ACCESS
	CLI_LOGD("flash_direct_stress {start|stop|status} [loops] [delay_ms]\n");
#endif
	CLI_LOGD("flash_erase_test ble\n");
	CLI_LOGD("flash_perf {read|write|erase|all} [addr] [len] [loops]\n");
}

static void cli_flash_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *msg = NULL;
	if (argc < 2) {
		cli_flash_help();
		return;
	}

	uint32_t start_addr = os_strtoul(argv[2], NULL, 16);
	uint32_t len = os_strtoul(argv[3], NULL, 16);

	if (os_strcmp(argv[1], "erase") == 0) {
		bk_flash_set_protect_type(FLASH_PROTECT_NONE);
		for (uint32_t addr = start_addr; addr < (start_addr + len); addr += FLASH_SECTOR_SIZE) {
			bk_flash_erase_sector(addr);
		}
		bk_flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK);
		msg = CLI_CMD_RSP_SUCCEED;
	} else if (os_strcmp(argv[1], "read") == 0) {
		uint8_t buf[FLASH_PAGE_SIZE] = {0};
		for (uint32_t addr = start_addr; addr < (start_addr + len); addr += FLASH_PAGE_SIZE) {
			os_memset(buf, 0, FLASH_PAGE_SIZE);
			bk_flash_read_bytes(addr, buf, FLASH_PAGE_SIZE);
			CLI_LOGD("flash read addr:%x\r\n", addr);

			CLI_LOGD("dump read flash data:\r\n");
			for (uint32_t i = 0; i < 16; i++) {
				for (uint32_t j = 0; j < 16; j++) {
					BK_LOGD(NULL, "%02x ", buf[i * 16 + j]);
				}
				BK_LOGD(NULL, "\r\n");
			}
		}
		msg = CLI_CMD_RSP_SUCCEED;
	} else if (os_strcmp(argv[1], "write") == 0) {
		uint8_t buf[FLASH_PAGE_SIZE] = {0};
		for (uint32_t i = 0; i < FLASH_PAGE_SIZE; i++) {
			buf[i] = i;
		}
		bk_flash_set_protect_type(FLASH_PROTECT_NONE);
		for (uint32_t addr = start_addr; addr < (start_addr + len); addr += FLASH_PAGE_SIZE) {
			bk_flash_write_bytes(addr, buf, FLASH_PAGE_SIZE);
		}
		bk_flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK);
		msg = CLI_CMD_RSP_SUCCEED;
	} else if (os_strcmp(argv[1], "get_id") == 0) {
		uint32_t flash_id = bk_flash_get_id();
		CLI_LOGD("flash_id:%x\r\n", flash_id);
		msg = CLI_CMD_RSP_SUCCEED;
	} else if (os_strcmp(argv[1], "mutex_test") == 0) {
	#if CONFIG_FLASH_TEST
		extern void flash_svr_test_task(void * param);
		int task_pri = os_strtoul(argv[2], NULL, 16);
		rtos_create_thread(NULL, task_pri, "flash_test", flash_svr_test_task, 2048, NULL);
	#endif
		msg = CLI_CMD_RSP_SUCCEED;
	} else {
		cli_flash_help();
		msg = CLI_CMD_RSP_ERROR;
	}

	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

#if CONFIG_TFM_FLASH_NSC
static void cli_flash_cmd_s(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *msg = NULL;
	if (argc < 2) {
		cli_flash_help();
		return;
	}

	uint32_t start_addr = os_strtoul(argv[2], NULL, 16);
	uint32_t len = os_strtoul(argv[3], NULL, 16);

	if (os_strcmp(argv[1], "erase") == 0) {
		psa_flash_set_protect_type(FLASH_PROTECT_NONE);
		for (uint32_t addr = start_addr; addr < (start_addr + len); addr += FLASH_SECTOR_SIZE) {
			psa_flash_erase_sector(addr);
		}
		psa_flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK);
		msg = CLI_CMD_RSP_SUCCEED;
	} else if (os_strcmp(argv[1], "read") == 0) {
		uint8_t buf[FLASH_PAGE_SIZE] = {0};
		for (uint32_t addr = start_addr; addr < (start_addr + len); addr += FLASH_PAGE_SIZE) {
			os_memset(buf, 0, FLASH_PAGE_SIZE);
			psa_flash_read_bytes(addr, buf, FLASH_PAGE_SIZE);
			CLI_LOGD("flash read addr:%x\r\n", addr);

			CLI_LOGD("dump read flash data:\r\n");
			for (uint32_t i = 0; i < 16; i++) {
				for (uint32_t j = 0; j < 16; j++) {
					BK_LOGD(NULL, "%02x ", buf[i * 16 + j]);
				}
				BK_LOGD(NULL, "\r\n");
			}
		}
		msg = CLI_CMD_RSP_SUCCEED;
	} else if (os_strcmp(argv[1], "write") == 0) {
		uint8_t buf[FLASH_PAGE_SIZE] = {0};
		for (uint32_t i = 0; i < FLASH_PAGE_SIZE; i++) {
			buf[i] = i;
		}
		int level = rtos_enter_critical();
		psa_flash_set_protect_type(FLASH_PROTECT_NONE);
		for (uint32_t addr = start_addr; addr < (start_addr + len); addr += FLASH_PAGE_SIZE) {
			psa_flash_write_bytes(addr, buf, FLASH_PAGE_SIZE);
		}
		psa_flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK);
		rtos_exit_critical(level);
		msg = CLI_CMD_RSP_SUCCEED;
	} else {
		cli_flash_help();
		msg = CLI_CMD_RSP_ERROR;
	}

	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}
#endif

static void cli_flash_partition_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_logic_partition_t *partition;

	if (os_strcmp(argv[1], "show") == 0) {
		for (bk_partition_t par= BK_PARTITION_BOOTLOADER; par < BK_PARTITIONS_TABLE_SIZE; par++) {
			partition = bk_flash_partition_get_info(par);
			if (partition == NULL)
				continue;

			CLI_LOGD("%4d | %11s |  Dev:%d  | 0x%08lx | 0x%08lx |\r\n", par,
					partition->partition_description, partition->partition_owner,
					partition->partition_start_addr, partition->partition_length);
		}
	} else {
		cli_flash_help();
	}
}

#if CONFIG_FLASH_CP_AP_DIRECT_ACCESS
#define FLASH_DIRECT_STRESS_REGION_CNT       2
#define FLASH_DIRECT_STRESS_DEFAULT_LOOPS    0U
#define FLASH_DIRECT_STRESS_DEFAULT_DELAY_MS 200U
#define FLASH_DIRECT_STRESS_REPORT_INTERVAL  100U
#define FLASH_DIRECT_STRESS_STACK_SIZE       3072U
#define FLASH_DIRECT_STRESS_TASK_PRIO        5
#define FLASH_DIRECT_STRESS_YIELD_MS         1U

typedef struct {
	uint32_t start_addr;
	uint32_t range;
} flash_direct_stress_region_t;

typedef struct {
	volatile uint8_t running;
	volatile uint8_t stop;
	volatile uint32_t pass_count;
	volatile uint32_t fail_count;
	volatile uint32_t current_loop;
	volatile uint32_t last_addr;
	volatile uint32_t last_len;
	volatile uint8_t last_pattern;
	uint32_t target_loops;
	uint32_t delay_ms;
	beken_thread_t thread;
	flash_direct_stress_region_t region[FLASH_DIRECT_STRESS_REGION_CNT];
} flash_direct_stress_ctx_t;

static flash_direct_stress_ctx_t s_flash_direct_stress = {
	.region = {
		{0x2a6000, 0x20000}, /* OTA scratch window, disjoint from CP default. */
		{0x3f2000, 0x8000},  /* usr_config scratch window, disjoint from CP default. */
	},
};

static void cli_flash_direct_stress_yield(void)
{
#if CONFIG_TASK_WDT
	bk_task_wdt_feed();
#endif
	rtos_delay_milliseconds(FLASH_DIRECT_STRESS_YIELD_MS);
}

static bk_err_t cli_flash_direct_once(uint32_t start_addr, uint32_t len, uint8_t pattern)
{
	uint8_t wr_buf[FLASH_PAGE_SIZE] = {0};
	uint8_t rd_buf[FLASH_PAGE_SIZE] = {0};

	for (uint32_t i = 0; i < FLASH_PAGE_SIZE; i++) {
		wr_buf[i] = pattern + i;
	}

	for (uint32_t sector = start_addr; sector < (start_addr + len); sector += FLASH_SECTOR_SIZE) {
		bk_flash_set_protect_type(FLASH_PROTECT_NONE);
		if (bk_flash_erase_sector(sector) != BK_OK) {
			CLI_LOGE("flash_direct erase fail addr=0x%x\r\n", sector);
			bk_flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK);
			return BK_FAIL;
		}
		for (uint32_t addr = sector; addr < (sector + FLASH_SECTOR_SIZE); addr += FLASH_PAGE_SIZE) {
			if (bk_flash_write_bytes(addr, wr_buf, FLASH_PAGE_SIZE) != BK_OK) {
				CLI_LOGE("flash_direct write fail addr=0x%x\r\n", addr);
				bk_flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK);
				return BK_FAIL;
			}
		}
		bk_flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK);

		for (uint32_t addr = sector; addr < (sector + FLASH_SECTOR_SIZE); addr += FLASH_PAGE_SIZE) {
			os_memset(rd_buf, 0, sizeof(rd_buf));
			if (bk_flash_read_bytes(addr, rd_buf, FLASH_PAGE_SIZE) != BK_OK) {
				CLI_LOGE("flash_direct read fail addr=0x%x\r\n", addr);
				return BK_FAIL;
			}
			if (os_memcmp(wr_buf, rd_buf, FLASH_PAGE_SIZE) != 0) {
				CLI_LOGE("flash_direct verify fail addr=0x%x pattern=0x%x\r\n", addr, pattern);
				return BK_FAIL;
			}
		}
		cli_flash_direct_stress_yield();
	}

	return BK_OK;
}

static uint32_t cli_flash_direct_stress_len(uint32_t loop, uint32_t range)
{
	static const uint32_t lens[] = {
		FLASH_SECTOR_SIZE,
		FLASH_SECTOR_SIZE * 2,
		FLASH_SECTOR_SIZE * 4,
	};
	uint32_t len = lens[loop % (sizeof(lens) / sizeof(lens[0]))];

	while ((len > range) && (len > FLASH_SECTOR_SIZE))
		len >>= 1;

	return len;
}

static void cli_flash_direct_stress_task(void *param)
{
	flash_direct_stress_ctx_t *ctx = (flash_direct_stress_ctx_t *)param;

	ctx->pass_count = 0;
	ctx->fail_count = 0;
	ctx->current_loop = 0;
	ctx->running = 1;
	ctx->stop = 0;

	while (!ctx->stop && ((ctx->target_loops == 0) || (ctx->current_loop < ctx->target_loops))) {
		flash_direct_stress_region_t *region = &ctx->region[ctx->current_loop % FLASH_DIRECT_STRESS_REGION_CNT];
		uint32_t len = cli_flash_direct_stress_len(ctx->current_loop, region->range);
		uint32_t span = region->range - len + FLASH_SECTOR_SIZE;
		uint32_t addr = region->start_addr + ((ctx->current_loop * FLASH_SECTOR_SIZE) % span);
		uint8_t pattern = (uint8_t)(0x80 + ctx->current_loop);

		ctx->last_addr = addr;
		ctx->last_len = len;
		ctx->last_pattern = pattern;

		if (cli_flash_direct_once(addr, len, pattern) != BK_OK) {
			ctx->fail_count++;
			CLI_LOGE("flash_direct_stress FAIL loop=%u addr=0x%x len=0x%x pattern=0x%x pass=%u fail=%u\r\n",
				ctx->current_loop, addr, len, pattern, ctx->pass_count, ctx->fail_count);
			bk_flash_shared_dump();
			break;
		}

		ctx->pass_count++;
		ctx->current_loop++;

		if ((ctx->pass_count % FLASH_DIRECT_STRESS_REPORT_INTERVAL) == 0) {
			CLI_LOGI("flash_direct_stress progress pass=%u fail=%u loop=%u addr=0x%x len=0x%x\r\n",
				ctx->pass_count, ctx->fail_count, ctx->current_loop, addr, len);
			bk_flash_shared_dump();
		}

		if (ctx->delay_ms)
			rtos_delay_milliseconds(ctx->delay_ms);
	}

	CLI_LOGI("flash_direct_stress stopped pass=%u fail=%u loop=%u last=0x%x/0x%x pattern=0x%x\r\n",
		ctx->pass_count, ctx->fail_count, ctx->current_loop,
		ctx->last_addr, ctx->last_len, ctx->last_pattern);
	bk_flash_shared_dump();
	ctx->running = 0;
	ctx->thread = NULL;
	rtos_delete_thread(NULL);
}

static void cli_flash_direct_stress_status(void)
{
	flash_direct_stress_ctx_t *ctx = &s_flash_direct_stress;

	CLI_LOGI("flash_direct_stress running=%u stop=%u target=%u delay=%u pass=%u fail=%u loop=%u last=0x%x/0x%x pattern=0x%x\r\n",
		ctx->running, ctx->stop, ctx->target_loops, ctx->delay_ms,
		ctx->pass_count, ctx->fail_count, ctx->current_loop,
		ctx->last_addr, ctx->last_len, ctx->last_pattern);
	bk_flash_shared_dump();
}

static void cli_flash_direct_stress_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *msg = CLI_CMD_RSP_ERROR;
	flash_direct_stress_ctx_t *ctx = &s_flash_direct_stress;

	if (argc < 2) {
		CLI_LOGI("flash_direct_stress {start|stop|status} [loops] [delay_ms]\r\n");
		os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		if (ctx->running) {
			CLI_LOGE("flash_direct_stress already running\r\n");
			os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
			return;
		}

		ctx->target_loops = (argc >= 3) ? os_strtoul(argv[2], NULL, 10) : FLASH_DIRECT_STRESS_DEFAULT_LOOPS;
		ctx->delay_ms = (argc >= 4) ? os_strtoul(argv[3], NULL, 10) : FLASH_DIRECT_STRESS_DEFAULT_DELAY_MS;
		ctx->stop = 0;
		ctx->running = 1;
		if (rtos_create_thread(&ctx->thread, FLASH_DIRECT_STRESS_TASK_PRIO, "flash_stress",
				(beken_thread_function_t)cli_flash_direct_stress_task,
				FLASH_DIRECT_STRESS_STACK_SIZE, ctx) != BK_OK) {
			ctx->thread = NULL;
			ctx->running = 0;
			CLI_LOGE("flash_direct_stress create task failed\r\n");
			os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
			return;
		}
		msg = CLI_CMD_RSP_SUCCEED;
	} else if (os_strcmp(argv[1], "stop") == 0) {
		ctx->stop = 1;
		msg = CLI_CMD_RSP_SUCCEED;
	} else if (os_strcmp(argv[1], "status") == 0) {
		cli_flash_direct_stress_status();
		msg = CLI_CMD_RSP_SUCCEED;
	} else {
		CLI_LOGI("flash_direct_stress {start|stop|status} [loops] [delay_ms]\r\n");
	}

	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

static void cli_flash_direct_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *msg = CLI_CMD_RSP_ERROR;

	if (argc < 4) {
		CLI_LOGI("flash_direct <start_addr> <len> <loops> [pattern]\r\n");
		os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
		return;
	}

	uint32_t start_addr = os_strtoul(argv[1], NULL, 16);
	uint32_t len = os_strtoul(argv[2], NULL, 16);
	uint32_t loops = os_strtoul(argv[3], NULL, 10);
	uint8_t pattern = (argc >= 5) ? (uint8_t)os_strtoul(argv[4], NULL, 16) : 0x5a;

	if ((len == 0) || (loops == 0) || (start_addr % FLASH_SECTOR_SIZE) ||
		(len % FLASH_SECTOR_SIZE)) {
		CLI_LOGE("flash_direct requires sector-aligned addr/len, len and loops non-zero\r\n");
		bk_flash_shared_dump();
		os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
		return;
	}

	for (uint32_t loop = 0; loop < loops; loop++) {
		if (cli_flash_direct_once(start_addr, len, pattern + loop) != BK_OK) {
			CLI_LOGE("flash_direct FAIL loop=%u addr=0x%x len=0x%x\r\n", loop, start_addr, len);
			bk_flash_shared_dump();
			os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
			return;
		}
		CLI_LOGI("flash_direct PASS loop=%u addr=0x%x len=0x%x\r\n", loop, start_addr, len);
	}

	bk_flash_shared_dump();
	msg = CLI_CMD_RSP_SUCCEED;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

static void cli_flash_lock_state_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *msg = CLI_CMD_RSP_SUCCEED;

	bk_flash_shared_dump();
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}
#endif

void flash_erase_with_ble_sleep(uint32_t erase_addr)
{
    uint32_t  anchor_time = 0;
    uint32_t  temp_time = 0;
    uint8_t   flash_erase_ready = 0;

    anchor_time = rtos_get_time();
    while(1)
    {
        flash_erase_ready = ble_callback_deal_handler(ERASE_FLASH_TIMEOUT);
        temp_time = rtos_get_time();
        if(temp_time >= anchor_time)
        {
            temp_time -= anchor_time;
        }
        else
        {
            temp_time += (0xFFFFFFFF - anchor_time);
        }
        if(temp_time >= ERASE_TOUCH_TIMEOUT)
            flash_erase_ready = 1;
        if(flash_erase_ready == 1)
        {
            bk_flash_erase_sector(erase_addr);
            flash_erase_ready = 0;
            break;
        }
        else
        {
            rtos_delay_milliseconds(2);
        }
    }
}

static void cli_flash_erase_test_with_ble(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *msg = NULL;
	uint32_t start_addr = 0x260000;
	uint32_t erase_len = 0x180000;

	if (os_strcmp(argv[1], "ble") == 0) {

		for (uint32_t erase_addr = start_addr; erase_addr <= (start_addr + erase_len);) {
			flash_erase_with_ble_sleep(erase_addr);
			erase_addr += FLASH_SECTOR_SIZE;
			CLI_LOGD("erase_addr:%x\r\n", erase_addr);
		}
		CLI_LOGD("cli_flash_erase_test_with_ble finish.\r\n");
		msg = CLI_CMD_RSP_SUCCEED;
	} else {
		cli_flash_help();
		msg = CLI_CMD_RSP_ERROR;
	}
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

extern void cli_flash_perf_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

#define FLASH_CMD_CNT (sizeof(s_flash_commands) / sizeof(struct cli_command))
static const struct cli_command s_flash_commands[] = {
	{"flash", "flash {erase|read|write} [start_addr] [len]", cli_flash_cmd},
#if CONFIG_TFM_FLASH_NSC
	{"flash_s", "flash {erase|read|write} [start_addr] [len]", cli_flash_cmd_s},
#endif
	{"flash_partition", "flash_partition {show}", cli_flash_partition_cmd},
#if CONFIG_FLASH_CP_AP_DIRECT_ACCESS
	{"flash_direct", "flash_direct <start_addr> <len> <loops> [pattern]", cli_flash_direct_cmd},
	{"flash_direct_stress", "flash_direct_stress {start|stop|status} [loops] [delay_ms]", cli_flash_direct_stress_cmd},
	{"flash_lock_state", "flash_lock_state", cli_flash_lock_state_cmd},
#endif
	{"flash_erase_test", "cli_flash_erase_test with ble connecting", cli_flash_erase_test_with_ble},
	{"flash_perf", "flash_perf {read|write|erase|all} [addr] [len] [loops]", cli_flash_perf_cmd},
};

int cli_flash_init(void)
{
	BK_LOG_ON_ERR(bk_flash_driver_init());
	return cli_register_commands(s_flash_commands, FLASH_CMD_CNT);
}

