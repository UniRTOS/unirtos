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

#include "cli.h"

#include <os/os.h>
#include <driver/psram.h>
#include <driver/aon_rtc.h>
#if CONFIG_TRNG_SUPPORT
#include <driver/trng.h>
#endif
#include "bk_general_dma.h"
#include <driver/dma.h>
#include "soc/mapping.h"
#if (CONFIG_CACHE_ENABLE)
#include "cache.h"
#endif
#if CNOFIG_PSRAM_CALIBRATE
#include "temp_detect_pub.h"
#endif

#if (CONFIG_PSRAM_AUTO_DETECT)
#include "bk_ef.h"
#endif

#define TEST_PSRAM_ACCURACY     1

/*
 * Fixed regions used by the PSRAM background tests (cpu_dma_verify / stack_stress / psram_soak / psram_rapid).
 * This project uses PSRAM as 8MB (0x60000000-0x60800000); the layout below all sits in the non-cache alias 0x60xxxxxx:
 *   cpu_test     : [0x60000000-0x60400000] 4MB
 *   task region  : [0x60400000-0x60600000] 2MB   (shared by stack_stress/psram_soak/psram_rapid)
 *   cpu_dma area : [0x60600000-0x60700000] 1MB   (src 512KB + dst 512KB)
 * For 16MB PSRAM, these macros can be overridden at compile time to enlarge the test range.
 */
#ifndef CONFIG_PSRAM_TEST_CPU_ADDR
#define CONFIG_PSRAM_TEST_CPU_ADDR       0x60000000
#define CONFIG_PSRAM_TEST_CPU_SIZE       0x00400000
#endif
#ifndef CONFIG_PSRAM_TEST_TASK_ADDR
#define CONFIG_PSRAM_TEST_TASK_ADDR      0x60400000
#define CONFIG_PSRAM_TEST_TASK_SIZE      0x00200000
#endif
#ifndef CONFIG_PSRAM_TEST_CPU_DMA_ADDR
#define CONFIG_PSRAM_TEST_CPU_DMA_ADDR   0x60600000
#define CONFIG_PSRAM_TEST_CPU_DMA_SIZE   0x00100000
#endif

/* PSRAM capacity probe/clamp: use min(runtime-probed real capacity, compile-time configured capacity); test regions must not exceed it */
#define PSRAM_TEST_BASE_NONCACHE         0x60000000u
#ifndef CONFIG_PSRAM_CAPACITY
#define CONFIG_PSRAM_CAPACITY            0x00800000u
#endif
#define PSRAM_TEST_SIZE_4M               0x00400000u
#define PSRAM_TEST_SIZE_8M               0x00800000u
#define PSRAM_TEST_SIZE_16M              0x01000000u

/* This project only supports plain GDMA (no HPDMA); transfer in 4-byte-aligned 32KB chunks (meets 32-bit DMA alignment and stays within the HW limit) */
#define PSRAM_TEST_DMA_CHUNK             0x8000u

#define write_data(addr,val)                 *((volatile uint32_t *)(addr)) = val
#define read_data(addr,val)                  val = *((volatile uint32_t *)(addr))
#define get_addr_data(addr)                  *((volatile uint32_t *)(addr))

#if (CONFIG_ARCH_RISCV)
extern u64 riscv_get_mtimer(void);
#endif

/*
 * Probe the real PSRAM capacity at runtime: when PSRAM is smaller than the address window,
 * high addresses alias (wrap) back to low addresses.
 * On the non-cache alias (0x60000000), write A at base[0], then write B at a candidate boundary;
 * if base[0] becomes B, that boundary wrapped back to base[0], i.e. the real capacity equals that candidate.
 * Only accesses within the [0,8MB) window (safe); restores the original base[0] value when done.
 */
static uint32_t psram_test_probe_capacity(void)
{
	volatile uint32_t *base = (volatile uint32_t *)PSRAM_TEST_BASE_NONCACHE;
	const uint32_t A = 0xA5A50001u;
	const uint32_t B = 0x5A5A0002u;
	uint32_t save0 = base[0];
	uint32_t cap = PSRAM_TEST_SIZE_16M;

	base[0] = A;
	base[PSRAM_TEST_SIZE_4M / 4] = B;
	if (base[0] == B)
	{
		cap = PSRAM_TEST_SIZE_4M;
		goto done;
	}

	base[0] = A;
	base[PSRAM_TEST_SIZE_8M / 4] = B;
	if (base[0] == B)
	{
		cap = PSRAM_TEST_SIZE_8M;
		goto done;
	}

done:
	base[0] = save0;
	return cap;
}

static uint32_t s_psram_test_cap = 0; /* 0 = not probed yet */

static uint32_t psram_test_get_capacity(void)
{
	if (s_psram_test_cap == 0)
	{
		uint32_t probed = psram_test_probe_capacity();
		uint32_t cfg = (uint32_t)CONFIG_PSRAM_CAPACITY;
		s_psram_test_cap = (probed < cfg) ? probed : cfg;
		CLI_LOGI("psram test capacity: probed=%uMB config=%uMB effective=%uMB\r\n",
			probed / (1024 * 1024), cfg / (1024 * 1024), s_psram_test_cap / (1024 * 1024));
	}
	return s_psram_test_cap;
}

/* Clamp [addr, addr+size) into the effective PSRAM capacity; returns usable size (0 means fully out of range) */
static uint32_t psram_test_clamp_region(uint32_t addr, uint32_t size, const char *name)
{
	uint32_t cap = psram_test_get_capacity();
	uint32_t base = (addr >= 0x64000000u) ? 0x64000000u : PSRAM_TEST_BASE_NONCACHE;
	uint32_t top = base + cap;

	if (addr >= top)
	{
		CLI_LOGE("psram test: region %s @%08x beyond capacity(%uMB), skip\r\n",
			name, addr, cap / (1024 * 1024));
		return 0;
	}
	if (addr + size > top)
	{
		uint32_t new_size = top - addr;
		CLI_LOGW("psram test: region %s clamped %uKB->%uKB by capacity(%uMB)\r\n",
			name, size / 1024, new_size / 1024, cap / (1024 * 1024));
		return new_size;
	}
	return size;
}

static void cli_psram_help(void)
{
	CLI_LOGD("psram_test start <cpu|conexist|continue_write|dma|calibrate> <cacheable 0|1> [delay_ms]\r\n");
	CLI_LOGD("psram_test stop\r\n");
	CLI_LOGD("psram_test_ext cpu_dma_verify [half_size_kb] / cpu_dma_verify_stop\r\n");
	CLI_LOGD("psram_test_ext stack_stress_start / stack_stress_stop\r\n");
	CLI_LOGD("psram_test_ext psram_soak_start [soak_ms] / psram_soak_stop\r\n");
	CLI_LOGD("psram_test_ext psram_rapid_start / psram_rapid_stop\r\n");
}

#define PSRAM_TEST_LEN               (1024 * 4)
beken_thread_t  psram_thread_hdl = NULL;

typedef struct {
	uint8_t test_running;
	uint8_t test_mode;
	uint8_t cacheable;
	uint8_t dma_channel;
	uint32_t length;
	uint32_t *data;
	uint32_t delay_time;
} psram_debug_t;

psram_debug_t * psram_debug = NULL;

static uint64_t bk_get_current_timer(void)
{
	uint64_t timer = 0;

#ifdef CONFIG_ARCH_RISCV
	timer = riscv_get_mtimer();// tick
#else // CONFIG_ARCH_RISCV

#ifdef CONFIG_AON_RTC
	timer = bk_aon_rtc_get_us();
#endif

#endif // CONFIG_ARCH_RISCV

	return timer;
}

static uint64_t bk_get_spend_time_us(uint64_t before, uint64_t after)
{
	uint64_t spend_time = 0;

	if (after == 0 || before >= after)
	{
		spend_time = 0;
		return spend_time;
	}

#ifdef CONFIG_ARCH_RISCV
	spend_time = (after - before) / 26;
#else // CONFIG_ARCH_RISCV

#ifdef CONFIG_AON_RTC
	spend_time = after - before;
#endif

#endif // CONFIG_ARCH_RISCV

	return spend_time;
}

static uint32_t s_cpu_test_pass_count = 0;
static uint32_t s_cpu_test_fail_count = 0;
static uint64_t s_cpu_test_last_status_time = 0;

static void psram_cpu_write_test(void)
{
	uint32_t i = 0;
	uint32_t error_num = 0;
	uint32_t value = 0;
	uint32_t base_addr = CONFIG_PSRAM_TEST_CPU_ADDR;
	uint32_t test_len = psram_test_clamp_region(CONFIG_PSRAM_TEST_CPU_ADDR, CONFIG_PSRAM_TEST_CPU_SIZE, "cpu_test");
	if (test_len == 0)
		return;
	/* pattern buffer length may be 32K or a fallback like 16K; adapt to the actual length (length is a power of two) */
	uint32_t word_mask = (psram_debug->length / 4) - 1;

#if TEST_PSRAM_ACCURACY
	uint32_t j = 0;
	for (j = 0; j < test_len / psram_debug->length; j ++)
	{
		for (i = 0; i < psram_debug->length; i += 4)
			write_data((base_addr + i + (j * psram_debug->length)), psram_debug->data[(i >> 2) & word_mask]);
	}
#else
	for (i = 0; i < test_len; i +=4)
		write_data(base_addr + i, 0x11223344);
#endif

	for (i = 0; i < test_len / 4; i++)
		read_data((base_addr + i * 0x4), value);

#if TEST_PSRAM_ACCURACY
	for (i = 0; i < test_len / 4; i++)
	{
		value = get_addr_data(base_addr + i * 0x4);
		if (value != (psram_debug->data[i & word_mask]))
		{
			if (error_num < 10)
				CLI_LOGE("cpu_test ERR @%08x: got %08x expect %08x xor %08x\r\n",
					base_addr + i * 0x4, value, psram_debug->data[i & word_mask],
					value ^ psram_debug->data[i & word_mask]);
			error_num++;
		}
	}
#endif

	if (error_num > 0) {
		s_cpu_test_fail_count++;
		CLI_LOGE("cpu_test FAIL: error_num=%u (pass=%u, fail=%u)\r\n",
			error_num, s_cpu_test_pass_count, s_cpu_test_fail_count);
	} else {
		s_cpu_test_pass_count++;
	}

	uint64_t now = bk_get_current_timer();
	if (now - s_cpu_test_last_status_time >= 5000000) {
		CLI_LOGD("cpu_test running [%08x-%08x]: pass=%u, fail=%u\r\n",
			base_addr, base_addr + test_len, s_cpu_test_pass_count, s_cpu_test_fail_count);
		s_cpu_test_last_status_time = now;
	}

	rtos_delay_milliseconds(psram_debug->delay_time);
}

static bk_err_t psram_dma_memcpy_by_chnl(void *out, const void *in, uint32_t len, dma_id_t cpy_chnl)
{
	dma_config_t dma_config = {0};

	dma_config.mode = DMA_WORK_MODE_SINGLE;
	dma_config.chan_prio = 0;

	dma_config.src.dev = DMA_DEV_DTCM;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.src.start_addr = (uint32_t)in;
	dma_config.src.end_addr = (uint32_t)(in + len);

	dma_config.dst.dev = DMA_DEV_DTCM;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.dst.start_addr = (uint32_t)out;
	dma_config.dst.end_addr = (uint32_t)(out + len);


	bk_dma_init(cpy_chnl, &dma_config);
	bk_dma_set_transfer_len(cpy_chnl, len);
#if (CONFIG_SPE)

	if (psram_debug->cacheable)
	{
		bk_dma_set_src_burst_len(cpy_chnl, BURST_LEN_INC8);
		bk_dma_set_dest_burst_len(cpy_chnl, BURST_LEN_INC8);
	}

	bk_dma_set_src_sec_attr(cpy_chnl, DMA_ATTR_SEC);
	bk_dma_set_dest_sec_attr(cpy_chnl, DMA_ATTR_SEC);
#endif
	bk_dma_start(cpy_chnl);

	BK_WHILE(bk_dma_get_enable_status(cpy_chnl));

	return BK_OK;
}

/*
 * Plain GDMA memory copy (supports PSRAM->PSRAM), automatically split by the GDMA single-transfer limit.
 * This function only handles chunking.
 */
static bk_err_t psram_test_dma_copy(void *dst, const void *src, uint32_t len)
{
	bk_err_t ret = BK_OK;
	uint32_t offset = 0;
	dma_id_t chnl;

	/* Dedicated channel + unbounded wait (psram_dma_memcpy_by_chnl), to avoid the generic dma_memcpy's
	 * 4ms timeout falsely tripping under PSRAM->PSRAM same-bus read/write contention and falling back to CPU os_memcpy */
	chnl = bk_dma_alloc(DMA_DEV_DTCM);
	if (chnl >= DMA_ID_MAX)
	{
		CLI_LOGE("psram_test_dma_copy: alloc dma chnl fail\r\n");
		return BK_ERR_DMA_ID;
	}

	while (offset < len)
	{
		uint32_t chunk = len - offset;
		if (chunk > PSRAM_TEST_DMA_CHUNK)
			chunk = PSRAM_TEST_DMA_CHUNK;
		chunk &= ~0x3u; /* 32-bit DMA: length aligned to 4 bytes */
		if (chunk == 0)
			break;

		ret = psram_dma_memcpy_by_chnl((void *)((uint8_t *)dst + offset),
			(const void *)((const uint8_t *)src + offset), chunk, chnl);
		if (ret != BK_OK)
			break;

		offset += chunk;
	}

	bk_dma_free(DMA_DEV_DTCM, chnl);

	return ret;
}

static void psram_dma_write_test(void)
{
	uint32_t i = 0;
	uint32_t error_num = 0;
	uint64_t rate = 0;
	uint64_t timer0, timer1;
	uint32_t total_time = 0;

	uint32_t value = 0;
	uint32_t base_addr = 0x60000000;

	if (psram_debug->cacheable)
	{
		#if !CONFIG_SOC_BK7236XX
		base_addr = 0x64000000;
		#endif
	}

	uint32_t test_len = 1024 * 1024 * 8;

	CLI_LOGD("begin write %08x-%08x test\r\n", base_addr, base_addr + test_len);

	timer0 = bk_get_current_timer();

	for (i = 0; i < test_len / psram_debug->length; i++)
	{
		psram_dma_memcpy_by_chnl((void *)(base_addr + i * psram_debug->length), psram_debug->data, psram_debug->length, psram_debug->dma_channel);
	}

	timer1 = bk_get_current_timer();
	if (timer1 > timer0)
	{
		total_time = bk_get_spend_time_us(timer0, timer1);
		rate = (test_len) * 8 / total_time;
		CLI_LOGD("finish write, use time: %ld ms, write_rate:%d Mbps\r\n", (uint32_t)(total_time / 1000), rate);
	}

	CLI_LOGD("begin read %08x-%08x test\r\n", base_addr, base_addr + test_len);

	timer0 = bk_get_current_timer();

	for (i = 0; i < test_len / 4; i++)
		read_data((base_addr + i * 0x4), value);

	timer1 = bk_get_current_timer();
	if (timer1 > timer0)
	{
		total_time = bk_get_spend_time_us(timer0, timer1);
		rate = ((uint64_t)test_len) * 8/ total_time;
		CLI_LOGD("finish read, use time: %ld ms, read_rate:%d Mbps\r\n", (uint32_t)(total_time / 1000), rate);
	}

	for (i = 0; i < test_len / psram_debug->length; i++)
	{
		for (uint32_t k = 0; k < psram_debug->length / 4; k++)
		{
			value = get_addr_data(base_addr + i * psram_debug->length + k * 0x4);

			if (value != psram_debug->data[k])
			{
				CLI_LOGD("==========%08x %08x %08x=======\n", value, psram_debug->data[k], value^psram_debug->data[k]);
				error_num++;
			}
		}
	}

	CLI_LOGD("finish compare, error_num: %ld, corr_rate: %ld.\r\n", error_num, ((test_len / 4 - error_num) * 100 / (test_len / 4)));

	rtos_delay_milliseconds(psram_debug->delay_time);

}

static void psram_write_continue_test(void)
{
	uint32_t i = 0;
	uint32_t error_num = 0;
	uint64_t rate = 0;
	uint64_t timer0, timer1;
	uint32_t total_time = 0;

	uint32_t value = 0;
	uint32_t base_addr = 0x60000000;

	if (psram_debug->cacheable)
	{
		#if !CONFIG_SOC_BK7236XX
		base_addr = 0x64000000;
		#endif
	}

	uint32_t test_len = 1024 * 1024 * 8;

	CLI_LOGD("begin write %08x-%08x test\r\n", base_addr, base_addr + test_len);
	timer0 = bk_get_current_timer();
	for (i = 0; i < test_len; i += psram_debug->length)
	{
		bk_psram_memcpy((uint8_t *)(base_addr + i), (uint8_t *)&psram_debug->data[0], psram_debug->length);
	}

	timer1 = bk_get_current_timer();
	if (timer1 > timer0)
	{
		total_time = bk_get_spend_time_us(timer0, timer1);
		rate = ((uint64_t)test_len) * 1000000 / total_time;
		CLI_LOGD("finish write, use time: %ld ms, write_rate:%d%ld byte/s\r\n", (uint32_t)(total_time / 1000),
			(uint32_t)(rate >> 32), (uint32_t)(rate & 0xFFFFFFFF));
	}


	CLI_LOGD("begin read %08x-%08x test\r\n", base_addr, base_addr + test_len);
	timer0 = bk_get_current_timer();
	for (i = 0; i < test_len / 4; i++)
		read_data((base_addr + i * 0x4), value);

	timer1 = bk_get_current_timer();
	if (timer1 > timer0)
	{
		total_time = bk_get_spend_time_us(timer0, timer1);
		rate = ((uint64_t)test_len) * 1000000 / total_time;
		CLI_LOGD("finish read, use time: %ld ms, read_rate:%d%ld byte/s\r\n", (uint32_t)(total_time / 1000),
			(uint32_t)(rate >> 32), (uint32_t)(rate & 0xFFFFFFFF));
	}

	for (i = 0; i < test_len / psram_debug->length; i++)
	{
		for (uint32_t k = 0; k < psram_debug->length / 4; k++)
		{
			value = get_addr_data(base_addr + i * psram_debug->length + k * 0x4);

			if (value != psram_debug->data[k])
			{
				error_num++;
			}
		}
	}

	CLI_LOGD("finish compare, error_num: %ld, corr_rate: %ld.\r\n", error_num, (test_len / 4 - error_num) * 100 / (test_len / 4));

	rtos_delay_milliseconds(psram_debug->delay_time);

}

static void psram_test_unit(void)
{
#if (CONFIG_BK7256_SOC)

	uint32_t i = 0;
	uint32_t error_num = 0;
	uint64_t rate = 0;
	uint64_t timer0, timer1;
	uint32_t total_time = 0;

	uint32_t start_address = (uint32_t)&psram_map->reserved;
	uint32_t end_address = (8 * 1024 * 1024 + 0x60000000);
	uint32_t test_len = end_address - start_address;

	uint32_t value = 0;
	CLI_LOGD("begin write %08X-%08X, count: %d\n", start_address, end_address, test_len / 4);

	timer0 = bk_get_current_timer();

	for (i = 0; i < test_len / 4; i++)
		write_data((start_address + i * 0x4), 0x11111111 + i);

	timer1 = bk_get_current_timer();
	if (timer1 > timer0)
	{
		total_time = bk_get_spend_time_us(timer0, timer1);
		rate = ((uint64_t)test_len) * 1000000 / total_time;
		CLI_LOGD("finish write, use time: %ld ms, write_rate:%d%ld byte/s\r\n", (uint32_t)(total_time / 1000),
			(uint32_t)(rate >> 32), (uint32_t)(rate & 0xFFFFFFFF));
	}



	CLI_LOGD("begin write %08X-%08X, count: %d\n", start_address, end_address, test_len);
	timer0 = bk_get_current_timer();
	for (i = 0; i < test_len / 4; i++)
		read_data((start_address + i * 0x4), value);

	timer1 = bk_get_current_timer();
	if (timer1 > timer0)
	{
		total_time = bk_get_spend_time_us(timer0, timer1);
		rate = ((uint64_t)test_len) * 1000000 / total_time;
		CLI_LOGD("finish read, use time: %ld ms, read_rate:%d%ld byte/s\r\n", (uint32_t)(total_time / 1000),
			(uint32_t)(rate >> 32), (uint32_t)(rate & 0xFFFFFFFF));
	}



	CLI_LOGD("begin write %08X-%08X, count: %d\n", start_address, end_address, test_len);

	for (i = 0; i < test_len / 4; i++)
	{
		value = get_addr_data(start_address + i * 0x4);
		if (value != (0x11111111 + i))
			error_num++;
	}

	CLI_LOGD("finish compare, error_num: %ld, corr_rate: %ld\n", error_num, ((test_len / 4 - error_num) * 100 / (test_len / 4 ));

	rtos_delay_milliseconds(psram_debug->delay_time);
#endif
}

//TODO fix/refactoring it during v5 verification
static uint32_t psram_calibrate_read_write_test(uint32_t start_address, uint32_t test_len)
{
	uint32_t error_num = 0;
	uint32_t value = 0;
	uint32_t i = 0;

	for (i = 0; i < test_len / 4; i++)
		write_data((start_address + i * 0x4), 0x11111111 + i);

	for (i = 0; i < test_len / 4; i++)
	{
		value = get_addr_data(start_address + i * 0x4);
		if (value != (0x11111111 + i))
			error_num++;
	}

	return error_num;
}

static void psram_calibrate_test(void)
{
#if CNOFIG_PSRAM_CALIBRATE
	static uint32_t s_temperature = 0;
	uint32_t cur_temperature = -1;
	uint32_t err_cnt = 0;
	uint32_t v = 0;

	if (BK_OK != temp_detect_get_temperature(&cur_temperature)) {
		CLI_LOGE("failed to get temperature\r\n");
		return;
	}

	uint32_t diff = (cur_temperature > s_temperature) ? (cur_temperature - s_temperature) : (s_temperature - cur_temperature);
	if (diff > 100 ) { //TODO give a reasonable value
		for (int i = 0; i < 63; i++) {
			uint32_t v = (i & 7) | ((i & ~7) << 3);
			REG_WRITE(0x46080000 + (5 << 2), v);
			err_cnt = psram_calibrate_read_write_test(0x60000000, 10240);
			if (err_cnt) {
				BK_LOGD(NULL, "%d %d %d %d %d\r\n", cur_temperature, (i & 7), (i >> 6) & 3, (i >> 8) & 3, err_cnt);
			}
		}
	}
#endif
}

static void psram_test_main(void)
{
	if (psram_debug->test_mode == 0) {
		s_cpu_test_pass_count = 0;
		s_cpu_test_fail_count = 0;
		s_cpu_test_last_status_time = bk_get_current_timer();
		CLI_LOGD("cpu_test started: region [%08x-%08x] (%uKB)\r\n",
			CONFIG_PSRAM_TEST_CPU_ADDR,
			CONFIG_PSRAM_TEST_CPU_ADDR + CONFIG_PSRAM_TEST_CPU_SIZE,
			CONFIG_PSRAM_TEST_CPU_SIZE / 1024);
	}

	while (psram_debug->test_running)
	{
		if (psram_debug->test_mode == 0)
		{
			psram_cpu_write_test();
		} else if (psram_debug->test_mode == 1) {
			psram_test_unit();
		} else if (psram_debug->test_mode == 2) {
			psram_write_continue_test();
		} else if (psram_debug->test_mode == 3) {
			psram_dma_write_test();
		} else {
			psram_calibrate_test();
		}
	}

	CLI_LOGD("psram_test task exit\n");

	if (psram_debug)
	{
		if (psram_debug->data)
		{
			os_free(psram_debug->data);
			psram_debug->data = NULL;
		}

		bk_dma_free(DMA_DEV_DTCM, psram_debug->dma_channel);

		os_free(psram_debug);
		psram_debug = NULL;
	}

	psram_thread_hdl = NULL;
	rtos_delete_thread(NULL);
}

static bk_err_t psram_task_init(void)
{
	bk_err_t ret = BK_OK;

	if (!psram_thread_hdl)
	{
		ret = rtos_create_thread(&psram_thread_hdl,
								 4,
								 "psram_debug",
								 (beken_thread_function_t)psram_test_main,
								 4 * 1024,
								 (beken_thread_arg_t)NULL);
		if (ret != BK_OK)
		{
			psram_thread_hdl = NULL;
			CLI_LOGE("Error: Failed to create psram test task: %d\r\n", ret);
			return BK_ERR_NOT_INIT;
		}

		return BK_OK;
	}
	else
		return BK_OK;
}

static void cli_psram_cmd_handle(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *msg = NULL;

	if (os_strcmp(argv[1], "start") == 0)
	{
		bk_psram_init();

		psram_debug = (psram_debug_t *)os_malloc(sizeof(psram_debug_t));

		if (psram_debug == NULL)
		{
			CLI_LOGE("psram test malloc failed!\r\n");
			msg = CLI_CMD_RSP_ERROR;
			os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
			return;
		}

		os_memset(psram_debug, 0, sizeof(psram_debug_t));

		if (psram_debug->data == NULL)
		{
			/* Step down from 32K to 4K when the SRAM heap is tight; all are powers of two and do not affect test quality */
			uint32_t try_len = 1024 * 32;
			while (try_len >= 1024 * 4)
			{
				psram_debug->data = (uint32_t *)os_malloc(try_len);
				if (psram_debug->data != NULL)
				{
					psram_debug->length = try_len;
					break;
				}
				try_len >>= 1;
			}
			if (psram_debug->data == NULL)
			{
				CLI_LOGE("malloc error!\r\n");
				os_free(psram_debug);
				psram_debug = NULL;
				return;
			}
			CLI_LOGD("psram test: data buffer = %u bytes\r\n", psram_debug->length);
		}

		for (int i = 0; i < psram_debug->length / 4; i++)
		{
#if CONFIG_TRNG_SUPPORT
			psram_debug->data[i] = bk_rand() + i;
#else
			psram_debug->data[i] = 0xA55AA55A + i + (i << 8) + (i << 16) + (i << 24);
#endif
		}

		psram_debug->test_running = 1;

		if (os_strcmp(argv[2], "cpu") == 0)
		{
			psram_debug->test_mode = 0;
		}
		else if (os_strcmp(argv[2], "conexist") == 0)
		{
			psram_debug->test_mode = 1;
		}
		else if (os_strcmp(argv[2], "continue_write") == 0)
		{
			psram_debug->test_mode = 2;
		} else if (os_strcmp(argv[2], "dma") == 0) {
			psram_debug->test_mode = 3;
			psram_debug->dma_channel = bk_dma_alloc(DMA_DEV_DTCM);
		} else if (os_strcmp(argv[2], "calibrate") == 0) {
			psram_debug->test_mode = 4;
		}

		if (os_strcmp(argv[3], "1") == 0)
		{
			psram_debug->cacheable = 1;
		}
		else
		{
			psram_debug->cacheable = 0;
		}

		psram_debug->delay_time = 500;

		if (argc >= 5)
		{
			psram_debug->delay_time = os_strtoul(argv[4], NULL, 10);

			if (psram_debug->delay_time == 0)
				psram_debug->delay_time = 500;
		}

		if (psram_task_init() != kNoErr)
		{
			CLI_LOGE("psram test failed!\r\n");
			msg = CLI_CMD_RSP_ERROR;
			os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
			return;
		}

		CLI_LOGD("psram test start success!\r\n");
		msg = CLI_CMD_RSP_SUCCEED;
	}
	else if (os_strcmp(argv[1], "stop") == 0)
	{
		if (psram_thread_hdl)
		{
			psram_debug->test_running = 0;
		}

		while (psram_thread_hdl)
		{
			rtos_delay_milliseconds(10);
		}

		bk_psram_deinit();
		CLI_LOGD("psram test stop success!\r\n");
		msg = CLI_CMD_RSP_SUCCEED;
	}
	else if (os_strcmp(argv[1], "strcat") == 0)
	{
		uint8_t *data = psram_malloc(20);
		if (data == NULL)
		{
			CLI_LOGD("psram malloc error!\r\n");
			msg = CLI_CMD_RSP_ERROR;
			os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
			return;
		}
		os_memset(data, 0, 20);
		bk_psram_strcat((char *)data, (char *)argv[2]);
		bk_psram_strcat((char *)data, (char *)argv[2]);
		bk_psram_strcat((char *)data, (char *)argv[2]);
		psram_free(data);
		msg = CLI_CMD_RSP_SUCCEED;
	}
	else
	{
		cli_psram_help();
		msg = CLI_CMD_RSP_ERROR;
	}

	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

static uint32_t test_frame_strip(uint8_t *src, uint32_t size)
{
	uint8_t sram_tmp[16] = {0x71, 0xfb, 0x84, 0x1f, 0x53, 0x5a, 0xd9, 0xd9, 0x8e, 0xd2, 0x76, 0x3f, 0xff, 0xff, 0xff, 0xd9};

	static uint8_t flag = 0;
	flag ++;
	sram_tmp[0] += flag;

	{
		BK_LOGD(NULL, "1====>>>> %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %p %d\n",
			src[size - 16], src[size - 15], src[size - 14], src[size - 13],
			src[size - 12], src[size - 11], src[size - 10], src[size - 9],
			src[size - 8], src[size - 7], src[size - 6], src[size - 5],
			src[size - 4], src[size - 3], src[size - 2], src[size - 1], src, size);
//		for (uint8_t i = 0; i < 16; i++)
//		{
//			src[i] = sram_tmp[i];
//		}
		bk_psram_word_memcpy(src, sram_tmp, 16);

		BK_LOGD(NULL, "2===>>> %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %p %d\n",
			src[size - 16], src[size - 15], src[size - 14], src[size - 13],
			src[size - 12], src[size - 11], src[size - 10], src[size - 9],
			src[size - 8], src[size - 7], src[size - 6], src[size - 5],
			src[size - 4], src[size - 3], src[size - 2], src[size - 1], src, size);

#if (CONFIG_CACHE_ENABLE)
		flush_dcache(src, 16);
#endif

		BK_LOGD(NULL, "3==>> %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %p %d\n",
			src[size - 16], src[size - 15], src[size - 14], src[size - 13],
			src[size - 12], src[size - 11], src[size - 10], src[size - 9],
			src[size - 8], src[size - 7], src[size - 6], src[size - 5],
			src[size - 4], src[size - 3], src[size - 2], src[size - 1], src, size);

		bk_psram_word_memcpy(src, sram_tmp, 16);

		BK_LOGD(NULL, "4=> %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %p %d\n",
			src[size - 16], src[size - 15], src[size - 14], src[size - 13],
			src[size - 12], src[size - 11], src[size - 10], src[size - 9],
			src[size - 8], src[size - 7], src[size - 6], src[size - 5],
			src[size - 4], src[size - 3], src[size - 2], src[size - 1], src, size);
	}

	return size;
}


void cli_test_psram_cache_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
#if CONFIG_DEBUG_VERSION
	uint32_t address, size;
	BK_LOGD(NULL, "cli_test_psram_cache_cmd\r\n");
	if (argc >= 3)
	{
		address = strtoll(argv[1], NULL, 16);
		size = strtoll(argv[2], NULL, 16);
		BK_LOGD(NULL, "test psram cache, address: 0x%08X size: 0x%08X\r\n", address, 16);

		test_frame_strip((uint8_t *)address, size);
	} else {
		BK_LOGD(NULL, "psram_cache <addr> <size>\r\n");
	}
#endif
}

#if (CONFIG_MPC)
#include <driver/mpc.h>

#define BUFFER_SIZE         (34)
#define TEST_VALUE_START    0x41

static void fill_buffer(uint8_t *pBuffer, uint32_t uwBufferLenght, uint32_t uwOffset)
{
	uint32_t tmpIndex = 0;

	/* Put in global buffer different values */
	for (tmpIndex = 0; tmpIndex < uwBufferLenght; tmpIndex++ ) {
		pBuffer[tmpIndex] = tmpIndex + uwOffset;
	}
}

static void cli_psram_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int i;
	uint8_t *test_addr_sec = NULL;
	char *msg = NULL;
	uint8_t psram_tx_buffer[BUFFER_SIZE] = {0};
	uint8_t psram_rx_buffer[BUFFER_SIZE] = {0};

	fill_buffer(psram_tx_buffer, BUFFER_SIZE, TEST_VALUE_START);

	/*set first block non-sec and second block sec*/
	bk_mpc_driver_init();
	bk_mpc_set_secure_attribute(MPC_DEV_PSRAM, 0, 1, MPC_BLOCK_NON_SECURE);
	bk_mpc_set_secure_attribute(MPC_DEV_PSRAM, bk_mpc_get_block_size(MPC_DEV_PSRAM), 1, MPC_BLOCK_SECURE);

	test_addr_sec = (uint8_t *)(SOC_PSRAM_DATA_ADDR_SEC + bk_mpc_get_block_size(MPC_DEV_PSRAM));
	bk_psram_memcpy(test_addr_sec, psram_tx_buffer, BUFFER_SIZE);
	bk_psram_memread(test_addr_sec, psram_rx_buffer, BUFFER_SIZE);

	for (i = 0; i < BUFFER_SIZE; i++) {
		BK_LOGD(NULL, "%02x ", psram_rx_buffer[i]);
	}
	BK_LOGD(NULL, "\r\n");
	msg = CLI_CMD_RSP_SUCCEED;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}
#endif


static void cli_psram_cmd_handle_ext(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t addr = 0x60000000;
	uint32_t i = 0;
	uint32_t length = 512;
	char *msg = NULL;

	if (argc < 2)
	{
		msg = CLI_CMD_RSP_ERROR;
		goto out;
	}

	if (os_strcmp(argv[1], "init") == 0)
	{
		/*init psram*/
		bk_psram_init();
		msg = CLI_CMD_RSP_SUCCEED;
	}
	else if (os_strcmp(argv[1], "delete_flash") == 0)
	{
#if (CONFIG_PSRAM_AUTO_DETECT)
		bk_set_env_enhance(PSRAM_CHIP_ID, NULL, 0);
		msg = CLI_CMD_RSP_SUCCEED;
#else
		msg = CLI_CMD_RSP_ERROR;
#endif
	}
	else if (os_strcmp(argv[1], "clk") == 0)
	{
		uint16_t clk = os_strtoul(argv[2], NULL, 10);

		switch (clk)
		{
			case 80:
				bk_psram_set_clk(PSRAM_80M);
				break;

			case 120:
				bk_psram_set_clk(PSRAM_120M);
				break;

			case 160:
				bk_psram_set_clk(PSRAM_160M);
				break;

			case 240:
				bk_psram_set_clk(PSRAM_240M);
				break;

			default:
				CLI_LOGE("can not support this clk!\r\n");
				break;
		}

		msg = CLI_CMD_RSP_SUCCEED;
	}
	else if (os_strcmp(argv[1], "byte") == 0)
	{
		uint8_t  value = 0;
		if (argc < 4)
		{
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}

		addr = os_strtoul(argv[2], NULL, 16);
		length = os_strtoul(argv[3], NULL, 10);
		if (length & 0x3)
		{
			length = ((length >> 2) + 1) << 2;
		}

		if (addr > 0x60800000 || addr < 0x60000000 || length == 0)
		{
			msg = CLI_CMD_RSP_ERROR;
		}
		else
		{
			for (i = 0; i < length; i++)
			{
				*((volatile uint8_t *)addr + i) = i;
			}

			for (i = 0; i < length; i++)
			{
				value = *((volatile uint8_t *)addr + i);

				if (value != (i % 256))
				{
					CLI_LOGD("index:%d, value:%d\r\n", i, value);
					break;
				}
			}

			if (i < length)
			{
				msg = CLI_CMD_RSP_ERROR;
			}
			else
			{
				msg = CLI_CMD_RSP_SUCCEED;
			}
		}
	}
	else if (os_strcmp(argv[1], "word") == 0)
	{
		if (argc < 4)
		{
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}

		addr = os_strtoul(argv[2], NULL, 16);
		length = os_strtoul(argv[3], NULL, 10);
		if (length & 0x3)
		{
			length = ((length >> 2) + 1) << 2;
		}

		if (addr > 0x60800000 || addr < 0x60000000 || length == 0)
		{
			msg = CLI_CMD_RSP_ERROR;
		}
		else
		{
			for (i = 0; i < length; i += 4)
			{
				*(((volatile uint32_t *)(addr + i))) = (i << 24) + (i << 16) + (i << 8) + i + 0x11223344;
			}
		}

		msg = CLI_CMD_RSP_SUCCEED;
	}
	else if (os_strcmp(argv[1], "rewrite") == 0)
	{
		if (argc < 3)
		{
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}

		addr = os_strtoul(argv[2], NULL, 16);
		length = 20;

		if (addr > 0x60800000 || addr < 0x60000000 || length == 0)
		{
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}

		for (i = 0; i < length; i += 4)
		{
			*(((volatile uint32_t *)(addr + i))) = 0x11111111 * i + 0x11223344;
		}

		for (i = 0; i < length * 100; i++)
		{
			*((volatile uint32_t *)addr) = 0x44332211;
		}

		for (i = 0; i < length; i ++)
		{
			CLI_LOGD("0x%08x\r\n", *((volatile uint32_t *)(addr + i * 4)));
		}

		msg = CLI_CMD_RSP_SUCCEED;
	}
	else if (os_strcmp(argv[1], "read") == 0)
	{
		if (argc < 4)
		{
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}

		addr = os_strtoul(argv[2], NULL, 16);
		length = os_strtoul(argv[3], NULL, 10);
		if (length & 0x3)
		{
			length = ((length >> 2) + 1) << 2;
		}

		if (length == 0)
		{
			msg = CLI_CMD_RSP_ERROR;
		}
		else
		{
			uint8_t *src = (uint8_t *)addr;

			for (i = 0; i < length; i++)
			{
				if ((i % 32) == 0)
				{
					CLI_LOGD("\r\n");
				}

				CLI_LOGD("%02x ", *(src + i));
			}
			CLI_LOGD("\r\n");
			msg = CLI_CMD_RSP_SUCCEED;
		}
	}
	else if (os_strcmp(argv[1], "cache") == 0)
	{
		if (argc < 4)
		{
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}

		addr = os_strtoul(argv[2], NULL, 16);
		length = os_strtoul(argv[3], NULL, 10);
		if (length & 0x3)
		{
			length = ((length >> 2) + 1) << 2;
		}

		if (length == 0)
		{
			msg = CLI_CMD_RSP_ERROR;
		}
		else
		{
#if (CONFIG_CACHE_ENABLE)
			flush_dcache((uint8_t *)addr, length);
#endif
			msg = CLI_CMD_RSP_SUCCEED;
		}
	}
	else if (os_strcmp(argv[1], "cover_write") == 0)
	{
		int ret = 0;
		uint32_t start = 0, end = 0;
		uint8_t enable = 0;
		uint8_t id = 0;

		if (argc < 4)
		{
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}

		id = os_strtoul(argv[2], NULL, 10);

		if (os_strcmp(argv[3], "1") == 0)
		{
			if (argc < 6)
			{
				msg = CLI_CMD_RSP_ERROR;
				goto out;
			}
			enable = 1;
			start = os_strtoul(argv[4], NULL, 16);
			end = os_strtoul(argv[5], NULL, 16);
		}
			
		else
			enable = 0;

		if (enable)
			ret = bk_psram_enable_write_through(id, start, end);
		else
			ret = bk_psram_disable_write_through(id);

		if (ret == BK_OK)
			msg = CLI_CMD_RSP_SUCCEED;
		else
			msg = CLI_CMD_RSP_ERROR;
	}
	else if (os_strcmp(argv[1], "deinit") == 0)
	{
		/*init psram*/
		bk_psram_deinit();
		msg = CLI_CMD_RSP_SUCCEED;
	}
	else if (os_strcmp(argv[1], "cpu_dma_verify") == 0)
	{
		extern void psram_cdv_start(uint32_t half_size_kb);
		uint32_t half_kb = 0;
		if (argc >= 3)
			half_kb = os_strtoul(argv[2], NULL, 10);
		psram_cdv_start(half_kb);
		msg = CLI_CMD_RSP_SUCCEED;
	}
	else if (os_strcmp(argv[1], "cpu_dma_verify_stop") == 0)
	{
		extern void psram_cdv_stop(void);
		psram_cdv_stop();
		msg = CLI_CMD_RSP_SUCCEED;
	}
	else if (os_strcmp(argv[1], "stack_stress_start") == 0)
	{
		extern void psram_stack_stress_start(void);
		psram_stack_stress_start();
		msg = CLI_CMD_RSP_SUCCEED;
	}
	else if (os_strcmp(argv[1], "stack_stress_stop") == 0)
	{
		extern void psram_stack_stress_stop(void);
		psram_stack_stress_stop();
		msg = CLI_CMD_RSP_SUCCEED;
	}
	else if (os_strcmp(argv[1], "psram_soak_start") == 0)
	{
		extern void psram_soak_start(uint32_t soak_ms);
		uint32_t soak_ms = 30000;
		if (argc >= 3)
			soak_ms = os_strtoul(argv[2], NULL, 10);
		psram_soak_start(soak_ms);
		msg = CLI_CMD_RSP_SUCCEED;
	}
	else if (os_strcmp(argv[1], "psram_soak_stop") == 0)
	{
		extern void psram_soak_stop(void);
		psram_soak_stop();
		msg = CLI_CMD_RSP_SUCCEED;
	}
	else if (os_strcmp(argv[1], "psram_rapid_start") == 0)
	{
		extern void psram_rapid_start(void);
		psram_rapid_start();
		msg = CLI_CMD_RSP_SUCCEED;
	}
	else if (os_strcmp(argv[1], "psram_rapid_stop") == 0)
	{
		extern void psram_rapid_stop(void);
		psram_rapid_stop();
		msg = CLI_CMD_RSP_SUCCEED;
	}

out:
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}


/* ========== cpu_dma_verify: CPU write + GDMA copy + CPU verify, looping in background until stop ========== */

typedef struct {
	volatile uint8_t running;
	uint32_t half_size;
	volatile uint32_t pass_count;
	volatile uint32_t fail_count;
	beken_thread_t task_hdl;
} cdv_ctx_t;

static cdv_ctx_t s_cdv_ctx = {0};

static void cdv_task_main(void *arg)
{
	uint32_t half_size = s_cdv_ctx.half_size;
	uint32_t src_base = CONFIG_PSRAM_TEST_CPU_DMA_ADDR;
	uint32_t dst_base = src_base + half_size;
	uint64_t last_status_time = bk_get_current_timer();

	CLI_LOGD("cpu_dma_verify started: src=[%08x-%08x] dst=[%08x-%08x] half=%uKB\r\n",
		src_base, src_base + half_size, dst_base, dst_base + half_size, half_size / 1024);

	while (s_cdv_ctx.running) {
		uint32_t error_num = 0;
		bk_err_t dma_ret = BK_OK;

		for (uint32_t idx = 0; idx < half_size / 4; idx++)
			write_data(src_base + idx * 4, (uint32_t)(idx * 0x10004001 + 0xA5A5A5A5));

		dma_ret = psram_test_dma_copy((void *)dst_base, (const void *)src_base, half_size);

		if (dma_ret != BK_OK) {
			s_cdv_ctx.fail_count++;
			CLI_LOGE("cpu_dma_verify: DMA copy failed (%d), fail=%u\r\n", dma_ret, s_cdv_ctx.fail_count);
			rtos_delay_milliseconds(100);
			continue;
		}

		for (uint32_t idx = 0; idx < half_size / 4; idx++) {
			uint32_t expect = (uint32_t)(idx * 0x10004001 + 0xA5A5A5A5);
			uint32_t actual = get_addr_data(dst_base + idx * 4);
			if (actual != expect) {
				error_num++;
				if (error_num <= 10)
					CLI_LOGE("cpu_dma_verify ERR @%08x: got %08x expect %08x xor %08x\r\n",
						dst_base + idx * 4, actual, expect, actual ^ expect);
			}
		}

		if (error_num > 0) {
			s_cdv_ctx.fail_count++;
			CLI_LOGE("cpu_dma_verify FAIL: error_num=%u (pass=%u, fail=%u)\r\n",
				error_num, s_cdv_ctx.pass_count, s_cdv_ctx.fail_count);
		} else {
			s_cdv_ctx.pass_count++;
		}

		uint64_t now = bk_get_current_timer();
		if (now - last_status_time >= 5000000) {
			CLI_LOGD("cpu_dma_verify running [%08x-%08x]: pass=%u, fail=%u\r\n",
				CONFIG_PSRAM_TEST_CPU_DMA_ADDR,
				CONFIG_PSRAM_TEST_CPU_DMA_ADDR + CONFIG_PSRAM_TEST_CPU_DMA_SIZE,
				s_cdv_ctx.pass_count, s_cdv_ctx.fail_count);
			last_status_time = now;
		}

		rtos_delay_milliseconds(10);
	}

	CLI_LOGD("cpu_dma_verify stopped: pass=%u, fail=%u\r\n", s_cdv_ctx.pass_count, s_cdv_ctx.fail_count);
	s_cdv_ctx.task_hdl = NULL;
	rtos_delete_thread(NULL);
}

void psram_cdv_start(uint32_t half_size_kb)
{
	if (s_cdv_ctx.running) {
		CLI_LOGD("cpu_dma_verify already running\r\n");
		return;
	}
	os_memset(&s_cdv_ctx, 0, sizeof(s_cdv_ctx));

	uint32_t total = psram_test_clamp_region(CONFIG_PSRAM_TEST_CPU_DMA_ADDR, CONFIG_PSRAM_TEST_CPU_DMA_SIZE, "cpu_dma");
	if (total < 8) {
		CLI_LOGE("cpu_dma_verify: no room within psram capacity\r\n");
		return;
	}
	uint32_t max_half = (total / 2) & ~0x7u; /* half must be 4-byte aligned (DMA) */
	uint32_t half = max_half;
	if (half_size_kb > 0 && (half_size_kb * 1024) <= max_half)
		half = (half_size_kb * 1024) & ~0x7u;

	s_cdv_ctx.half_size = half;
	s_cdv_ctx.running = 1;

	bk_err_t ret = rtos_create_thread(&s_cdv_ctx.task_hdl,
		4, "cdv_task",
		(beken_thread_function_t)cdv_task_main,
		4 * 1024,
		(beken_thread_arg_t)NULL);
	if (ret != BK_OK) {
		CLI_LOGE("cpu_dma_verify: create task failed %d\r\n", ret);
		s_cdv_ctx.running = 0;
		s_cdv_ctx.task_hdl = NULL;
	}
}

void psram_cdv_stop(void)
{
	if (!s_cdv_ctx.running) {
		CLI_LOGD("cpu_dma_verify not running\r\n");
		return;
	}
	s_cdv_ctx.running = 0;
	while (s_cdv_ctx.task_hdl != NULL)
		rtos_delay_milliseconds(10);
	CLI_LOGD("cpu_dma_verify stopped: pass=%u, fail=%u\r\n", s_cdv_ctx.pass_count, s_cdv_ctx.fail_count);
}


/* ========== stack_stress: PSRAM-stack parent task + 10 PSRAM-stack workers ========== */

#define STACK_STRESS_WORKER_CNT     10
#define STACK_STRESS_LOCAL_BUF_SZ   (4 * 1024)
/* AP core PSRAM_HEAP is only 640KB; worker stack is 16KB: 10 concurrent ~160KB + parent 16KB, well below heap capacity */
#define STACK_STRESS_WORKER_STACK   (16 * 1024)
#define STACK_STRESS_PARENT_STACK   (16 * 1024)

typedef struct {
	volatile uint8_t running;
	volatile uint32_t error_count;
	volatile uint32_t completed_rounds;
	volatile uint8_t worker_done[STACK_STRESS_WORKER_CNT];
	beken_thread_t parent_hdl;
} stack_stress_ctx_t;

static stack_stress_ctx_t s_ss_ctx = {0};

static void stack_stress_worker(void *arg)
{
	uint32_t packed = (uint32_t)(uintptr_t)arg;
	uint32_t slot_id = packed & 0xFF;
	uint32_t worker_id = packed >> 8;
	uint8_t buf_a[STACK_STRESS_LOCAL_BUF_SZ];
	uint8_t buf_b[STACK_STRESS_LOCAL_BUF_SZ];

	for (uint32_t i = 0; i < STACK_STRESS_LOCAL_BUF_SZ; i++)
		buf_a[i] = (uint8_t)(i + worker_id * 37 + 0x5A);

	os_memcpy(buf_b, buf_a, STACK_STRESS_LOCAL_BUF_SZ);

	for (uint32_t i = 0; i < STACK_STRESS_LOCAL_BUF_SZ; i++) {
		if (buf_a[i] != buf_b[i]) {
			s_ss_ctx.error_count++;
			CLI_LOGE("worker%u: mismatch @%u: %02x vs %02x\r\n",
				worker_id, i, buf_a[i], buf_b[i]);
			break;
		}
	}

	if (slot_id < STACK_STRESS_WORKER_CNT)
		s_ss_ctx.worker_done[slot_id] = 1;

	while (1)
		rtos_delay_milliseconds(1000);
}

static uint32_t ss_simple_rand(uint32_t *seed)
{
	*seed = (*seed) * 1103515245u + 12345u;
	return (*seed >> 16) & 0x7FFF;
}

static int ss_generate_cd_sequence(uint8_t *seq, uint32_t *seed)
{
	for (int retry = 0; retry < 200; retry++) {
		uint8_t tmp[STACK_STRESS_WORKER_CNT * 2];
		for (int i = 0; i < STACK_STRESS_WORKER_CNT; i++) {
			tmp[i] = 'C';
			tmp[i + STACK_STRESS_WORKER_CNT] = 'D';
		}
		int n = STACK_STRESS_WORKER_CNT * 2;
		for (int i = n - 1; i > 0; i--) {
			int j = ss_simple_rand(seed) % (i + 1);
			uint8_t t = tmp[i]; tmp[i] = tmp[j]; tmp[j] = t;
		}
		int depth = 0, valid = 1;
		for (int i = 0; i < n; i++) {
			if (tmp[i] == 'C') depth++;
			else depth--;
			if (depth < 0) { valid = 0; break; }
		}
		if (valid && depth == 0) {
			os_memcpy(seq, tmp, n);
			return 0;
		}
	}
	return -1;
}

static void stack_stress_parent(void *arg)
{
	beken_thread_t workers[STACK_STRESS_WORKER_CNT] = {NULL};
	uint32_t seed = (uint32_t)bk_get_current_timer() ^ 0xDEADBEEF;
	uint8_t cd_seq[STACK_STRESS_WORKER_CNT * 2];
	int total_steps = STACK_STRESS_WORKER_CNT * 2;
	uint64_t last_status_time = bk_get_current_timer();

	CLI_LOGD("stack_stress started: task_region [%08x-%08x] (%uKB), worker_stack=%uKB, local_buf=%uKB\r\n",
		CONFIG_PSRAM_TEST_TASK_ADDR,
		CONFIG_PSRAM_TEST_TASK_ADDR + CONFIG_PSRAM_TEST_TASK_SIZE,
		CONFIG_PSRAM_TEST_TASK_SIZE / 1024,
		STACK_STRESS_WORKER_STACK / 1024,
		STACK_STRESS_LOCAL_BUF_SZ / 1024);

	while (s_ss_ctx.running) {
		if (ss_generate_cd_sequence(cd_seq, &seed) != 0) {
			CLI_LOGE("stack_stress: failed to generate CD sequence\r\n");
			rtos_delay_milliseconds(100);
			continue;
		}

		os_memset((void *)s_ss_ctx.worker_done, 0, sizeof(s_ss_ctx.worker_done));

		int create_idx = 0, destroy_idx = 0;
		for (int step = 0; step < total_steps && s_ss_ctx.running; step++) {
			if (cd_seq[step] == 'C') {
				int slot = -1;
				for (int s = 0; s < STACK_STRESS_WORKER_CNT; s++) {
					if (workers[s] == NULL) { slot = s; break; }
				}
				if (slot < 0) {
					CLI_LOGE("stack_stress: no free slot (bug)\r\n");
					break;
				}
				char name[12];
				s_ss_ctx.worker_done[slot] = 0;
				os_snprintf(name, sizeof(name), "ss_w%d", slot);
				uint32_t packed_arg = ((s_ss_ctx.completed_rounds * STACK_STRESS_WORKER_CNT + slot) << 8) | (slot & 0xFF);
				bk_err_t ret = rtos_create_psram_thread(&workers[slot],
					5, name,
					(beken_thread_function_t)stack_stress_worker,
					STACK_STRESS_WORKER_STACK,
					(beken_thread_arg_t)(uintptr_t)packed_arg);
				if (ret != BK_OK) {
					CLI_LOGE("stack_stress: create %s failed %d\r\n", name, ret);
					workers[slot] = NULL;
				} else {
					CLI_LOGD("stack_stress: create %s (round=%u)\r\n", name, s_ss_ctx.completed_rounds);
				}
				create_idx++;
				rtos_delay_milliseconds(50 + ss_simple_rand(&seed) % 100);
			} else {
				int slot = -1;
				for (int s = 0; s < STACK_STRESS_WORKER_CNT; s++) {
					if (workers[s] != NULL) { slot = s; break; }
				}
				if (slot < 0) {
					CLI_LOGE("stack_stress: no active slot (bug)\r\n");
					break;
				}
				while (!s_ss_ctx.worker_done[slot])
					rtos_delay_milliseconds(10);
				CLI_LOGD("stack_stress: destroy ss_w%d (round=%u)\r\n", slot, s_ss_ctx.completed_rounds);
				rtos_delete_thread(&workers[slot]);
				workers[slot] = NULL;
				destroy_idx++;
			}
		}

		for (int s = 0; s < STACK_STRESS_WORKER_CNT; s++) {
			if (workers[s] != NULL) {
				while (!s_ss_ctx.worker_done[s])
					rtos_delay_milliseconds(10);
				CLI_LOGD("stack_stress: cleanup ss_w%d\r\n", s);
				rtos_delete_thread(&workers[s]);
				workers[s] = NULL;
			}
		}

		s_ss_ctx.completed_rounds++;

		uint64_t now = bk_get_current_timer();
		if (now - last_status_time >= 5000000) {
			CLI_LOGD("stack_stress running [%08x-%08x]: rounds=%u, errors=%u\r\n",
				CONFIG_PSRAM_TEST_TASK_ADDR,
				CONFIG_PSRAM_TEST_TASK_ADDR + CONFIG_PSRAM_TEST_TASK_SIZE,
				s_ss_ctx.completed_rounds, s_ss_ctx.error_count);
			last_status_time = now;
		}
	}

	CLI_LOGD("stack_stress stopped: rounds=%u, errors=%u\r\n",
		s_ss_ctx.completed_rounds, s_ss_ctx.error_count);
	s_ss_ctx.parent_hdl = NULL;
	rtos_delete_thread(NULL);
}

void psram_stack_stress_start(void)
{
	if (s_ss_ctx.running) {
		CLI_LOGD("stack_stress already running\r\n");
		return;
	}
	os_memset(&s_ss_ctx, 0, sizeof(s_ss_ctx));
	s_ss_ctx.running = 1;

	bk_err_t ret = rtos_create_psram_thread(&s_ss_ctx.parent_hdl,
		4, "ss_parent",
		(beken_thread_function_t)stack_stress_parent,
		STACK_STRESS_PARENT_STACK,
		(beken_thread_arg_t)NULL);
	if (ret != BK_OK) {
		CLI_LOGE("stack_stress: create parent failed %d\r\n", ret);
		s_ss_ctx.running = 0;
		s_ss_ctx.parent_hdl = NULL;
	}
}

void psram_stack_stress_stop(void)
{
	if (!s_ss_ctx.running) {
		CLI_LOGD("stack_stress not running\r\n");
		return;
	}
	s_ss_ctx.running = 0;
	while (s_ss_ctx.parent_hdl != NULL)
		rtos_delay_milliseconds(10);
	CLI_LOGD("stack_stress stopped: rounds=%u, errors=%u\r\n",
		s_ss_ctx.completed_rounds, s_ss_ctx.error_count);
}


/* ========== psram_soak: write - dwell - read-back bit-flip detection ========== */

static const uint32_t s_soak_patterns[] = {
	0x55555555, 0xAAAAAAAA,
	0x00000000, 0xFFFFFFFF,
	0x12345678, 0xA5A5A5A5,
	0x0F0F0F0F, 0xF0F0F0F0,
};
#define SOAK_PATTERN_CNT  (sizeof(s_soak_patterns) / sizeof(s_soak_patterns[0]))
#define SOAK_TASK_STACK   (8 * 1024)

typedef struct {
	volatile uint8_t running;
	volatile uint32_t pass_count;
	volatile uint32_t fail_count;
	volatile uint32_t bitflip_words;
	uint32_t soak_ms;
	beken_thread_t task_hdl;
} soak_ctx_t;

static soak_ctx_t s_soak_ctx = {0};

static void soak_task_main(void *arg)
{
	uint32_t base_addr = CONFIG_PSRAM_TEST_TASK_ADDR;
	uint32_t region_size = psram_test_clamp_region(CONFIG_PSRAM_TEST_TASK_ADDR, CONFIG_PSRAM_TEST_TASK_SIZE, "psram_soak");
	if (region_size == 0) {
		s_soak_ctx.running = 0;
		s_soak_ctx.task_hdl = NULL;
		rtos_delete_thread(NULL);
		return;
	}
	uint32_t words = region_size / 4;
	volatile uint32_t *p32 = (volatile uint32_t *)base_addr;
	uint32_t soak_ms = s_soak_ctx.soak_ms;
	uint32_t pat_idx = 0;
	uint64_t last_status_time = bk_get_current_timer();

	CLI_LOGD("psram_soak started: region [%08x-%08x] (%uKB), soak_ms=%u, patterns=%u\r\n",
		base_addr, base_addr + region_size, region_size / 1024, soak_ms, SOAK_PATTERN_CNT);

	while (s_soak_ctx.running) {
		uint32_t pat = s_soak_patterns[pat_idx % SOAK_PATTERN_CNT];

		for (uint32_t i = 0; i < words; i++)
			p32[i] = pat ^ i;

		rtos_delay_milliseconds(soak_ms);

		uint32_t error_num = 0;
		for (uint32_t i = 0; i < words; i++) {
			uint32_t actual = p32[i];
			uint32_t expect = pat ^ i;
			if (actual != expect) {
				error_num++;
				if (error_num <= 10)
					CLI_LOGE("soak bit-flip @%08x: got %08x expect %08x xor %08x (pat[%u]=%08x)\r\n",
						base_addr + i * 4, actual, expect, actual ^ expect, pat_idx % SOAK_PATTERN_CNT, pat);
			}
		}

		if (error_num > 0) {
			s_soak_ctx.fail_count++;
			s_soak_ctx.bitflip_words += error_num;
			CLI_LOGE("soak FAIL pat[%u]=%08x: %u words flipped (total_pass=%u, total_fail=%u, total_flip=%u)\r\n",
				pat_idx % SOAK_PATTERN_CNT, pat, error_num,
				s_soak_ctx.pass_count, s_soak_ctx.fail_count, s_soak_ctx.bitflip_words);
		} else {
			s_soak_ctx.pass_count++;
		}

		uint64_t now = bk_get_current_timer();
		if (now - last_status_time >= 5000000) {
			CLI_LOGD("psram_soak running [%08x-%08x]: pat[%u]=%08x, pass=%u, fail=%u, flip_words=%u\r\n",
				base_addr, base_addr + region_size,
				pat_idx % SOAK_PATTERN_CNT, pat,
				s_soak_ctx.pass_count, s_soak_ctx.fail_count, s_soak_ctx.bitflip_words);
			last_status_time = now;
		}

		pat_idx++;
	}

	CLI_LOGD("psram_soak stopped: pass=%u, fail=%u, flip_words=%u\r\n",
		s_soak_ctx.pass_count, s_soak_ctx.fail_count, s_soak_ctx.bitflip_words);
	s_soak_ctx.task_hdl = NULL;
	rtos_delete_thread(NULL);
}

void psram_soak_start(uint32_t soak_ms)
{
	if (s_soak_ctx.running) {
		CLI_LOGD("psram_soak already running\r\n");
		return;
	}
	os_memset(&s_soak_ctx, 0, sizeof(s_soak_ctx));
	s_soak_ctx.running = 1;
	s_soak_ctx.soak_ms = soak_ms ? soak_ms : 30000;

	bk_err_t ret = rtos_create_thread(&s_soak_ctx.task_hdl,
		5, "soak_task",
		(beken_thread_function_t)soak_task_main,
		SOAK_TASK_STACK,
		(beken_thread_arg_t)NULL);
	if (ret != BK_OK) {
		CLI_LOGE("psram_soak: create task failed %d\r\n", ret);
		s_soak_ctx.running = 0;
		s_soak_ctx.task_hdl = NULL;
	}
}

void psram_soak_stop(void)
{
	if (!s_soak_ctx.running) {
		CLI_LOGD("psram_soak not running\r\n");
		return;
	}
	s_soak_ctx.running = 0;
	while (s_soak_ctx.task_hdl != NULL)
		rtos_delay_milliseconds(10);
	CLI_LOGD("psram_soak stopped: pass=%u, fail=%u, flip_words=%u\r\n",
		s_soak_ctx.pass_count, s_soak_ctx.fail_count, s_soak_ctx.bitflip_words);
}


/* ========== psram_rapid: rapid write then immediate read-back pattern verify ========== */

#define RAPID_TASK_STACK  (8 * 1024)

typedef struct {
	volatile uint8_t running;
	volatile uint32_t pass_count;
	volatile uint32_t fail_count;
	volatile uint32_t bitflip_words;
	beken_thread_t task_hdl;
} rapid_ctx_t;

static rapid_ctx_t s_rapid_ctx = {0};

static void rapid_task_main(void *arg)
{
	uint32_t base_addr = CONFIG_PSRAM_TEST_TASK_ADDR;
	uint32_t region_size = psram_test_clamp_region(CONFIG_PSRAM_TEST_TASK_ADDR, CONFIG_PSRAM_TEST_TASK_SIZE, "psram_rapid");
	if (region_size == 0) {
		s_rapid_ctx.running = 0;
		s_rapid_ctx.task_hdl = NULL;
		rtos_delete_thread(NULL);
		return;
	}
	uint32_t words = region_size / 4;
	volatile uint32_t *p32 = (volatile uint32_t *)base_addr;
	uint32_t pat_idx = 0;
	uint64_t last_status_time = bk_get_current_timer();

	CLI_LOGD("psram_rapid started: region [%08x-%08x] (%uKB), patterns=%u\r\n",
		base_addr, base_addr + region_size, region_size / 1024, SOAK_PATTERN_CNT);

	while (s_rapid_ctx.running) {
		uint32_t pat = s_soak_patterns[pat_idx % SOAK_PATTERN_CNT];

		for (uint32_t i = 0; i < words; i++)
			p32[i] = pat ^ i;

		uint32_t error_num = 0;
		for (uint32_t i = 0; i < words; i++) {
			uint32_t actual = p32[i];
			uint32_t expect = pat ^ i;
			if (actual != expect) {
				error_num++;
				if (error_num <= 10)
					CLI_LOGE("rapid err @%08x: got %08x expect %08x xor %08x (pat[%u]=%08x)\r\n",
						base_addr + i * 4, actual, expect, actual ^ expect,
						pat_idx % SOAK_PATTERN_CNT, pat);
			}
		}

		if (error_num > 0) {
			s_rapid_ctx.fail_count++;
			s_rapid_ctx.bitflip_words += error_num;
			CLI_LOGE("rapid FAIL pat[%u]=%08x: %u words err (pass=%u, fail=%u, total_err=%u)\r\n",
				pat_idx % SOAK_PATTERN_CNT, pat, error_num,
				s_rapid_ctx.pass_count, s_rapid_ctx.fail_count, s_rapid_ctx.bitflip_words);
		} else {
			s_rapid_ctx.pass_count++;
		}

		uint64_t now = bk_get_current_timer();
		if (now - last_status_time >= 5000000) {
			CLI_LOGD("psram_rapid running [%08x-%08x]: pat[%u]=%08x, pass=%u, fail=%u, err_words=%u\r\n",
				base_addr, base_addr + region_size,
				pat_idx % SOAK_PATTERN_CNT, pat,
				s_rapid_ctx.pass_count, s_rapid_ctx.fail_count, s_rapid_ctx.bitflip_words);
			last_status_time = now;
		}

		pat_idx++;

		/* Yield one tick so the IDLE task can feed task_wdt, preventing a busy loop from starving IDLE and tripping the watchdog assert */
		rtos_delay_milliseconds(1);
	}

	CLI_LOGD("psram_rapid stopped: pass=%u, fail=%u, err_words=%u\r\n",
		s_rapid_ctx.pass_count, s_rapid_ctx.fail_count, s_rapid_ctx.bitflip_words);
	s_rapid_ctx.task_hdl = NULL;
	rtos_delete_thread(NULL);
}

void psram_rapid_start(void)
{
	if (s_rapid_ctx.running) {
		CLI_LOGD("psram_rapid already running\r\n");
		return;
	}
	if (s_soak_ctx.running) {
		CLI_LOGE("psram_rapid: cannot start while psram_soak is running (shared region)\r\n");
		return;
	}
	os_memset(&s_rapid_ctx, 0, sizeof(s_rapid_ctx));
	s_rapid_ctx.running = 1;

	bk_err_t ret = rtos_create_thread(&s_rapid_ctx.task_hdl,
		5, "rapid_task",
		(beken_thread_function_t)rapid_task_main,
		RAPID_TASK_STACK,
		(beken_thread_arg_t)NULL);
	if (ret != BK_OK) {
		CLI_LOGE("psram_rapid: create task failed %d\r\n", ret);
		s_rapid_ctx.running = 0;
		s_rapid_ctx.task_hdl = NULL;
	}
}

void psram_rapid_stop(void)
{
	if (!s_rapid_ctx.running) {
		CLI_LOGD("psram_rapid not running\r\n");
		return;
	}
	s_rapid_ctx.running = 0;
	while (s_rapid_ctx.task_hdl != NULL)
		rtos_delay_milliseconds(10);
	CLI_LOGD("psram_rapid stopped: pass=%u, fail=%u, err_words=%u\r\n",
		s_rapid_ctx.pass_count, s_rapid_ctx.fail_count, s_rapid_ctx.bitflip_words);
}


beken_thread_t  psram_task_hdl = NULL;

static void psram_task_main(void)
{
	while (1) {
		rtos_delay_milliseconds(3000);
		CLI_LOGD("psram_task_main is running.\r\n");
	}

	psram_task_hdl = NULL;
	rtos_delete_thread(NULL);
}

static void cli_create_psram_task_handle(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;

	if (!psram_task_hdl)
	{
		ret = rtos_create_psram_thread(&psram_task_hdl,
								 4,
								 "psram_task",
								 (beken_thread_function_t)psram_task_main,
								 4 * 1024,
								 (beken_thread_arg_t)NULL);
		if (ret != BK_OK)
		{
			psram_task_hdl = NULL;
			CLI_LOGE("Error: Failed to create psram test task: %d\r\n", ret);
			return;
		}

		return;
	}
}

static void cli_delete_psram_task_handle(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if(psram_task_hdl) {
		rtos_delete_thread(&psram_task_hdl);
		psram_task_hdl = NULL;
	}
}


#define PSRAM_CNT (sizeof(s_psram_commands) / sizeof(struct cli_command))
static const struct cli_command s_psram_commands[] = {
	{"psram_test_ext", "init|byte|word|rewrite|read|deinit|cpu_dma_verify|cpu_dma_verify_stop|stack_stress_start|stack_stress_stop|psram_soak_start|psram_soak_stop|psram_rapid_start|psram_rapid_stop", cli_psram_cmd_handle_ext},
	{"psram_test", "start|stop", cli_psram_cmd_handle},
	{"psram_cache", "psram_cache <addr> <size>", cli_test_psram_cache_cmd},
#if (CONFIG_MPC)
	{"psram_mpc", "", cli_psram_test},
#endif
	{"psram_task_create", "create task on psram", cli_create_psram_task_handle},
	{"psram_task_delete", "delete task on psram", cli_delete_psram_task_handle},
};

int cli_psram_init(void)
{
	return cli_register_commands(s_psram_commands, PSRAM_CNT);
}


