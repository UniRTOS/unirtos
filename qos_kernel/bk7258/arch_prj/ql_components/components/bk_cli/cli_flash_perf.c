// Copyright 2020-2026 Beken
//
// Flash throughput benchmark for comparing direct-access vs proxy modes.
// Build with CONFIG_FLASH_CP_AP_DIRECT_ACCESS=y/n and run the same CLI on AP/CP.

#include <common/bk_include.h>
#include <driver/flash.h>
#include <driver/aon_rtc.h>
#include "cli.h"
#include "flash_driver.h"
#if CONFIG_TASK_WDT
#include "bk_wdt.h"
#endif

#if defined(CONFIG_FREERTOS_SMP) && CONFIG_FREERTOS_SMP
#define FLASH_PERF_CPU_NAME        "AP"
#define FLASH_PERF_DEFAULT_ADDR    0x2a6000U
#else
#define FLASH_PERF_CPU_NAME        "CP"
#define FLASH_PERF_DEFAULT_ADDR    0x286000U
#endif

#define FLASH_PERF_DEFAULT_LEN       0x10000U
#define FLASH_PERF_DEFAULT_LOOPS     5U

#if CONFIG_FLASH_CP_AP_DIRECT_ACCESS
#define FLASH_PERF_MODE_NAME         "direct"
#else
#define FLASH_PERF_MODE_NAME         "proxy"
#endif

typedef enum {
	FLASH_PERF_OP_READ = 0,
	FLASH_PERF_OP_WRITE,
	FLASH_PERF_OP_ERASE,
	FLASH_PERF_OP_ALL,
} flash_perf_op_t;

static uint64_t flash_perf_now_us(void)
{
	return bk_aon_rtc_get_us();
}

static uint64_t flash_perf_elapsed_us(uint64_t start_us, uint64_t end_us)
{
	if (end_us >= start_us)
		return end_us - start_us;

	return (UINT64_MAX - start_us) + end_us + 1U;
}

static void flash_perf_feed_wdt(void)
{
#if CONFIG_TASK_WDT
	bk_task_wdt_feed();
#endif
}

static uint32_t flash_perf_kb_per_s(uint64_t total_bytes, uint64_t elapsed_us)
{
	if ((total_bytes == 0) || (elapsed_us == 0))
		return 0;

	return (uint32_t)((total_bytes * 1000000ULL) / elapsed_us / 1024ULL);
}

static void flash_perf_print_result(const char *op_name, uint32_t loops,
	uint64_t total_bytes, uint64_t elapsed_us)
{
	uint32_t kbps = flash_perf_kb_per_s(total_bytes, elapsed_us);
	uint64_t avg_us = (loops > 0) ? (elapsed_us / loops) : elapsed_us;

	CLI_LOGI("flash_perf cpu=%s mode=%s op=%s loops=%u bytes=%llu us=%llu avg_loop_us=%llu KB/s=%u\r\n",
		FLASH_PERF_CPU_NAME, FLASH_PERF_MODE_NAME, op_name,
		loops, (unsigned long long)total_bytes,
		(unsigned long long)elapsed_us, (unsigned long long)avg_us, kbps);
}

static bk_err_t flash_perf_validate(uint32_t start_addr, uint32_t len)
{
	if ((len == 0) || (start_addr % FLASH_SECTOR_SIZE) || (len % FLASH_SECTOR_SIZE)) {
		CLI_LOGE("flash_perf: addr/len must be non-zero and sector-aligned\r\n");
		return BK_FAIL;
	}

	return BK_OK;
}

static bk_err_t flash_perf_erase_region(uint32_t start_addr, uint32_t len)
{
	bk_flash_set_protect_type(FLASH_PROTECT_NONE);
	for (uint32_t addr = start_addr; addr < (start_addr + len); addr += FLASH_SECTOR_SIZE) {
		if (bk_flash_erase_sector(addr) != BK_OK) {
			bk_flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK);
			CLI_LOGE("flash_perf erase fail addr=0x%x\r\n", addr);
			return BK_FAIL;
		}
		flash_perf_feed_wdt();
	}
	bk_flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK);

	return BK_OK;
}

static bk_err_t flash_perf_write_region(uint32_t start_addr, uint32_t len, uint8_t pattern)
{
	uint8_t wr_buf[FLASH_SECTOR_SIZE];
	bk_err_t ret = BK_OK;

	bk_flash_set_protect_type(FLASH_PROTECT_NONE);
	for (uint32_t offset = 0; offset < len; offset += FLASH_SECTOR_SIZE) {
		uint32_t chunk = len - offset;

		if (chunk > FLASH_SECTOR_SIZE)
			chunk = FLASH_SECTOR_SIZE;

		for (uint32_t i = 0; i < chunk; i++)
			wr_buf[i] = (uint8_t)(pattern + i + offset);

		if (bk_flash_write_bytes(start_addr + offset, wr_buf, chunk) != BK_OK) {
			CLI_LOGE("flash_perf write fail addr=0x%x len=0x%x\r\n",
				start_addr + offset, chunk);
			ret = BK_FAIL;
			break;
		}
		flash_perf_feed_wdt();
	}
	bk_flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK);

	return ret;
}

static bk_err_t flash_perf_read_region(uint32_t start_addr, uint32_t len)
{
	uint8_t rd_buf[FLASH_PAGE_SIZE];
	volatile uint8_t sink = 0;

	for (uint32_t addr = start_addr; addr < (start_addr + len); addr += FLASH_PAGE_SIZE) {
		if (bk_flash_read_bytes(addr, rd_buf, FLASH_PAGE_SIZE) != BK_OK) {
			CLI_LOGE("flash_perf read fail addr=0x%x\r\n", addr);
			return BK_FAIL;
		}
		sink ^= rd_buf[0];
		flash_perf_feed_wdt();
	}

	(void)sink;
	return BK_OK;
}

static bk_err_t flash_perf_run_once(flash_perf_op_t op, uint32_t start_addr,
	uint32_t len, uint8_t pattern, uint64_t *elapsed_us, uint64_t *total_bytes)
{
	uint64_t start = flash_perf_now_us();
	bk_err_t ret = BK_OK;

	switch (op) {
	case FLASH_PERF_OP_ERASE:
		*total_bytes = len;
		ret = flash_perf_erase_region(start_addr, len);
		break;
	case FLASH_PERF_OP_WRITE:
		*total_bytes = len;
		ret = flash_perf_write_region(start_addr, len, pattern);
		break;
	case FLASH_PERF_OP_READ:
		*total_bytes = len;
		ret = flash_perf_read_region(start_addr, len);
		break;
	default:
		ret = BK_FAIL;
		break;
	}

	*elapsed_us = flash_perf_elapsed_us(start, flash_perf_now_us());
	return ret;
}

static const char *flash_perf_op_name(flash_perf_op_t op)
{
	switch (op) {
	case FLASH_PERF_OP_READ:
		return "read";
	case FLASH_PERF_OP_WRITE:
		return "write";
	case FLASH_PERF_OP_ERASE:
		return "erase";
	default:
		return "unknown";
	}
}

static bk_err_t flash_perf_benchmark(flash_perf_op_t op, uint32_t start_addr,
	uint32_t len, uint32_t loops)
{
	uint64_t total_elapsed_us = 0;
	uint64_t total_bytes = 0;
	uint8_t pattern = 0xa5;

	if (flash_perf_validate(start_addr, len) != BK_OK)
		return BK_FAIL;

	if (loops == 0)
		loops = FLASH_PERF_DEFAULT_LOOPS;

	CLI_LOGI("flash_perf start cpu=%s mode=%s op=%s addr=0x%x len=0x%x loops=%u\r\n",
		FLASH_PERF_CPU_NAME, FLASH_PERF_MODE_NAME, flash_perf_op_name(op),
		start_addr, len, loops);

	if ((op == FLASH_PERF_OP_WRITE) || (op == FLASH_PERF_OP_ALL)) {
		if (flash_perf_erase_region(start_addr, len) != BK_OK)
			return BK_FAIL;
	}

	for (uint32_t loop = 0; loop < loops; loop++) {
		uint64_t elapsed_us = 0;
		uint64_t bytes = 0;

		if (op == FLASH_PERF_OP_ALL) {
			uint64_t part_us = 0;
			uint64_t part_bytes = 0;

			if (flash_perf_run_once(FLASH_PERF_OP_ERASE, start_addr, len, pattern, &part_us, &part_bytes) != BK_OK)
				return BK_FAIL;
			flash_perf_print_result("erase", 1, part_bytes, part_us);
			total_elapsed_us += part_us;
			total_bytes += part_bytes;

			if (flash_perf_run_once(FLASH_PERF_OP_WRITE, start_addr, len, (uint8_t)(pattern + loop), &part_us, &part_bytes) != BK_OK)
				return BK_FAIL;
			flash_perf_print_result("write", 1, part_bytes, part_us);
			total_elapsed_us += part_us;
			total_bytes += part_bytes;

			if (flash_perf_run_once(FLASH_PERF_OP_READ, start_addr, len, pattern, &part_us, &part_bytes) != BK_OK)
				return BK_FAIL;
			flash_perf_print_result("read", 1, part_bytes, part_us);
			total_elapsed_us += part_us;
			total_bytes += part_bytes;
		} else {
			if (flash_perf_run_once(op, start_addr, len, (uint8_t)(pattern + loop), &elapsed_us, &bytes) != BK_OK)
				return BK_FAIL;
			total_elapsed_us += elapsed_us;
			total_bytes += bytes;
		}
	}

	if (op == FLASH_PERF_OP_ALL) {
		flash_perf_print_result("all_total", loops, total_bytes, total_elapsed_us);
	} else {
		flash_perf_print_result(flash_perf_op_name(op), loops, total_bytes, total_elapsed_us);
	}

	CLI_LOGI("flash_perf done cpu=%s mode=%s\r\n", FLASH_PERF_CPU_NAME, FLASH_PERF_MODE_NAME);
	return BK_OK;
}

void cli_flash_perf_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *msg = CLI_CMD_RSP_ERROR;
	flash_perf_op_t op;
	uint32_t start_addr = FLASH_PERF_DEFAULT_ADDR;
	uint32_t len = FLASH_PERF_DEFAULT_LEN;
	uint32_t loops = FLASH_PERF_DEFAULT_LOOPS;

	if (argc < 2) {
		CLI_LOGI("flash_perf {read|write|erase|all} [addr] [len] [loops]\r\n");
		CLI_LOGI("  cpu=%s mode=%s default addr=0x%x len=0x%x loops=%u\r\n",
			FLASH_PERF_CPU_NAME, FLASH_PERF_MODE_NAME,
			FLASH_PERF_DEFAULT_ADDR, FLASH_PERF_DEFAULT_LEN, FLASH_PERF_DEFAULT_LOOPS);
		os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
		return;
	}

	if (os_strcmp(argv[1], "read") == 0) {
		op = FLASH_PERF_OP_READ;
	} else if (os_strcmp(argv[1], "write") == 0) {
		op = FLASH_PERF_OP_WRITE;
	} else if (os_strcmp(argv[1], "erase") == 0) {
		op = FLASH_PERF_OP_ERASE;
	} else if (os_strcmp(argv[1], "all") == 0) {
		op = FLASH_PERF_OP_ALL;
	} else {
		CLI_LOGI("flash_perf {read|write|erase|all} [addr] [len] [loops]\r\n");
		os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
		return;
	}

	if (argc >= 3)
		start_addr = os_strtoul(argv[2], NULL, 16);
	if (argc >= 4)
		len = os_strtoul(argv[3], NULL, 16);
	if (argc >= 5)
		loops = os_strtoul(argv[4], NULL, 10);

	if (flash_perf_benchmark(op, start_addr, len, loops) == BK_OK)
		msg = CLI_CMD_RSP_SUCCEED;

	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}
