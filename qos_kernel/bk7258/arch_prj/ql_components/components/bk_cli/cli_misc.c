#include "cli.h"
#include <components/system.h>
#include <os/str.h>
#include "bk_sys_ctrl.h"
#include "bk_drv_model.h"
#include "release.h"
#include <driver/efuse.h>
#include <components/system.h>
#include <modules/wifi.h>
#include "sys_driver.h"
#include <driver/wdt.h>
#include <bk_wdt.h>
#include "gpio_driver.h"
#include <driver/gpio.h>
#include "bk_ps.h"
#include "bk_pm_internal_api.h"
#include "reset_reason.h"
#include <modules/pm.h>
#include "sdkconfig.h"
#include <driver/pwr_clk.h>
#if CONFIG_CACHE_ENABLE
#include "cache.h"
#endif
#include <components/ate.h>
#if CONFIG_AON_RTC
#include <driver/aon_rtc.h>
#endif
#include <driver/timer.h>
#include "phy_client.h"


void cli_misc_version_help(void)
{
	CLI_RAW_LOGI("\r\nversion\n");
	CLI_RAW_LOGI("  Show the firmware version, chip id, and soc information.\n");
	CLI_RAW_LOGI("  -no param\n");
	CLI_RAW_LOGI("  example1: version\n");
}

void cli_misc_starttype_help(void)
{
	CLI_RAW_LOGI("\r\nstarttype\n");
	CLI_RAW_LOGI("  Show the reset reason of last time.\n");
	CLI_RAW_LOGI("  -no param\n");
	CLI_RAW_LOGI("  example1: starttype\n");
}

void cli_misc_id_help(void)
{
	CLI_RAW_LOGI("\r\nid\n");
	CLI_RAW_LOGI("  Show the device id and chip id.\n");
	CLI_RAW_LOGI("  -no param\n");
	CLI_RAW_LOGI("  example1: id\n");
}

void cli_misc_reboot_help(void)
{
	CLI_RAW_LOGI("\r\nreboot\n");
	CLI_RAW_LOGI("  Reboot the device.\n");
	CLI_RAW_LOGI("  -no param\n");
	CLI_RAW_LOGI("  example1: reboot\n");
}

void cli_misc_time_help(void)
{
	CLI_RAW_LOGI("\r\ntime\n");
	CLI_RAW_LOGI("  Get the os time of the device.\n");
	CLI_RAW_LOGI("  -no param\n");
	CLI_RAW_LOGI("  example1: time\n");
}

void cli_misc_mac_help(void)
{
	CLI_RAW_LOGI("\r\nmac\n");
	CLI_RAW_LOGI("  Get the mac address of the device, include base mac, sta mac and ap mac.\n");
	CLI_RAW_LOGI("  -no param\n");
	CLI_RAW_LOGI("  example1: mac\n");
}

void cli_misc_cputest_help(void)
{
	CLI_RAW_LOGI("\r\ncputest [count]\n");
	CLI_RAW_LOGI("  Test the cpu performance.\n");
	CLI_RAW_LOGI("  -count<int><mandatory>: test count\n");
	CLI_RAW_LOGI("  example1: cputest 10\n");
}

void cli_misc_cache_help(void)
{
	CLI_RAW_LOGI("\r\ncache {mode}\n");
	CLI_RAW_LOGI("  Show cache configuration informaion or control dcache.\n");
	CLI_RAW_LOGI("  -mode<int><optional>: Enable or disable dcache. If this parameter is not filled in, it will show cache configuration information.\n");
	CLI_RAW_LOGI("  example1: cache\n");
	CLI_RAW_LOGI("  example1: cache 0\n");
	CLI_RAW_LOGI("  example2: cache 1\n");
}

void cli_misc_disable_int_interval_check_set_help(void)
{
	CLI_RAW_LOGI("\r\ndisable_int_interval_check_set [interval_time] [trace_time]\n");
	CLI_RAW_LOGI("  Set disable int check interval time(us):interval_time, trace_time\n");
	CLI_RAW_LOGI("  -interval_time<int><mandatory>: disable int check interval time(us)\n");
	CLI_RAW_LOGI("  -trace_time<int><mandatory>: trace time(us)\n");
	CLI_RAW_LOGI("  example1: disable_int_interval_check_set 1000 1000000\n");
}

void cli_misc_dump_int_context_help(void)
{
	CLI_RAW_LOGI("\r\ndump_int_context [type]\n");
	CLI_RAW_LOGI("  Assert or crash in interruption context.\n");
	CLI_RAW_LOGI("  -type<str><mandatory>: assert or crash\n");
	CLI_RAW_LOGI("  example1: dump_int_context assert\n");
	CLI_RAW_LOGI("  example2: dump_int_context crash\n");
}

void cli_misc_dump_disable_int_help(void)
{
	CLI_RAW_LOGI("\r\ndump_disable_int [type]\n");
	CLI_RAW_LOGI("  Assert or crash in interruption critical section.\n");
	CLI_RAW_LOGI("  -type<str><mandatory>: assert or crash\n");
	CLI_RAW_LOGI("  example1: dump_disable_int assert\n");
	CLI_RAW_LOGI("  example2: dump_disable_int crash\n");
}

__maybe_unused static void cli_misc_help(void)
{
	CLI_LOGD("pwm_driver init {26M|DCO}\n");
#if (CONFIG_WIFI_ENABLE)
	CLI_LOGD("mac <mac>, get/set mac. e.g. mac c89346000001\r\n");
#endif

	CLI_LOGD("setjtagmode set jtag mode [cpu0|cpu1] [group1|group2]\r\n");
	CLI_LOGD("setcpufreq [cksel] [ckdiv_core] [ckdiv_bus] [ckdiv_cpu]\r\n");
#if CONFIG_COMMON_IO
	CLI_LOGD("testcommonio test common io\r\n");
#endif
    CLI_LOGD("dump_int_context [assert|crash]\r\n");
    CLI_LOGD("dump_disable_int [assert|crash]\r\n");
}

static int hex2num(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}


static int hex2byte(const char *hex)
{
	int a, b;
	a = hex2num(*hex++);
	if (a < 0)
		return -1;
	b = hex2num(*hex++);
	if (b < 0)
		return -1;
	return (a << 4) | b;
}

int hexstr2bin_cli(const char *hex, u8 *buf, size_t len)
{
	size_t i;
	int a;
	const char *ipos = hex;
	u8 *opos = buf;

	for (i = 0; i < len; i++) {
		a = hex2byte(ipos);
		if (a < 0)
			return -1;
		*opos++ = a;
		ipos += 2;
	}
	return 0;
}

extern volatile const uint8_t build_version[];

void get_version(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if ((argc == 2) && (!os_strncmp(argv[1], "help", 4))) {
		cli_misc_version_help();
		return;
	}

	CLI_LOGD("get_version\r\n");
	//BK_LOGD(NULL, "firmware version : %s", BEKEN_SDK_REV);
	CLI_LOGD("firmware version : %s\r\n", build_version);
	CLI_LOGD("chip id : %x \r\n", sys_drv_get_chip_id());
	CLI_LOGD("soc: %s\n", CONFIG_SOC_STR);
}

void cli_show_reset_reason(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if ((argc == 2) && (!os_strncmp(argv[1], "help", 4))) {
		cli_misc_starttype_help();
		return;
	}

	show_reset_reason();
}


void get_id(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if ((argc == 2) && (!os_strncmp(argv[1], "help", 4))) {
		cli_misc_id_help();
		return;
	}

	CLI_LOGD("get_id\r\n");
	//BK_LOGD(NULL, "id : %x_%x",sddev_control(DD_DEV_TYPE_SCTRL,CMD_GET_DEVICE_ID, NULL), sddev_control(DD_DEV_TYPE_SCTRL,CMD_GET_CHIP_ID, NULL));
	CLI_LOGD("id : %x_%x",sys_drv_get_device_id(), sys_drv_get_chip_id());
}

#if CONFIG_NTP_SYNC_RTC
#include <time/ntp.h>
#endif
static void uptime_Command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if ((argc == 2) && (!os_strncmp(argv[1], "help", 4))) {
		cli_misc_time_help();
		return;
	}

	CLI_LOGD("OS time %ldms\r\n", rtos_get_time());
#if CONFIG_NTP_SYNC_RTC
	time_t cur_time = ntp_sync_to_rtc();
	if (cur_time)
	{
		CLI_LOGW("\r\nGet local time from NTP server: %s", ctime(&cur_time));
	} else {
		extern time_t timestamp_get();
		cur_time = timestamp_get();
		CLI_LOGW("Current time:%s\n", ctime(&cur_time));
	}

	struct timeval tv;
	bk_rtc_gettimeofday(&tv, 0);
	long ms_time = (tv.tv_usec / 1000);
	CLI_LOGD("%s NTP Time s=%ld\r\n", __func__, tv.tv_sec);
	CLI_LOGD("%s NTP Time ms=%ld\r\n", __func__, ms_time);
#endif

#if CONFIG_AON_RTC
	uint64_t rtc_time_us = bk_aon_rtc_get_us();
	CLI_LOGD("Aon rtc time_h:%u, time_l:%u us\n", (uint32_t)((rtc_time_us)>>32), (uint32_t)(rtc_time_us));
	CLI_LOGD("Aon rtc clock freq:%d\n", bk_rtc_get_clock_freq());
#endif
}

void reboot(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if ((argc == 2) && (!os_strncmp(argv[1], "help", 4))) {
		cli_misc_reboot_help();
		return;
	}

	bk_reboot();
}

static void mac_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint8_t base_mac[BK_MAC_ADDR_LEN]  = {0};
	uint8_t sta_mac[BK_MAC_ADDR_LEN]   = {0};
	uint8_t ap_mac[BK_MAC_ADDR_LEN]    = {0};
#if CONFIG_ETH
	uint8_t eth_mac[BK_MAC_ADDR_LEN]   = {0};
#endif

	if (argc == 1)
	{
		BK_LOG_ON_ERR(bk_ap_get_mac(base_mac, MAC_TYPE_BASE));
		BK_LOG_ON_ERR(bk_ap_wifi_sta_get_mac(sta_mac));
		BK_LOG_ON_ERR(bk_ap_wifi_ap_get_mac(ap_mac));
#if CONFIG_ETH
		BK_LOG_ON_ERR(bk_ap_get_mac(eth_mac, MAC_TYPE_ETH));
#endif
		if (0)//(ate_is_enabled())
		{
			BK_LOG_RAW("MAC address: %02x-%02x-%02x-%02x-%02x-%02x\r\n",
						base_mac[0],base_mac[1],base_mac[2],base_mac[3],base_mac[4],base_mac[5]);
		}
		else
		{
			BK_LOG_RAW("base mac: "BK_MAC_FORMAT"\n", BK_MAC_STR(base_mac));
			BK_LOG_RAW("sta mac: "BK_MAC_FORMAT"\n", BK_MAC_STR(sta_mac));
			BK_LOG_RAW("ap mac: "BK_MAC_FORMAT"\n", BK_MAC_STR(ap_mac));
#if CONFIG_ETH
			CLI_LOGD("eth mac: %pm\n", eth_mac);
#endif
		}
	}
	else if (argc == 2)
	{
		hexstr2bin_cli(argv[1], base_mac, BK_MAC_ADDR_LEN);
		bk_ap_set_base_mac(base_mac);
		if (0)//(ate_is_enabled())
			BK_LOG_RAW("Set MAC address: %02x-%02x-%02x-%02x-%02x-%02x\r\n",
						base_mac[0],base_mac[1],base_mac[2],base_mac[3],base_mac[4],base_mac[5]);
		else
			BK_LOG_RAW("set base mac: "BK_MAC_FORMAT"\n", BK_MAC_STR(base_mac));
	}
	else
		BK_LOG_RAW("mac cmd err\r\n");
}


void bk_set_jtag_mode(uint32_t cpu_id, uint32_t group_id);
static void set_jtag_mode(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 3) {
		cli_misc_help();
		return;
	}

	uint32_t cpu_id = 0;
	uint32_t group_id = 0;

	if (os_strcmp(argv[1], "cpu0") == 0) {
		cpu_id = 0;
		CLI_LOGD("gpio Jtag CPU0\r\n");
	} else if (os_strcmp(argv[1], "cpu1") == 0) {
		cpu_id = 1;
		CLI_LOGD("gpio Jtag CPU1\r\n");
	} else if (os_strcmp(argv[1], "cpu2") == 0) {
		cpu_id = 2;
		CLI_LOGD("gpio Jtag CPU2\r\n");
	} else {
		cli_misc_help();
	}

	if (os_strcmp(argv[2], "group1") == 0) {
		group_id = 0;
		CLI_LOGD("gpio Jtag group1\r\n");
	} else if (os_strcmp(argv[2], "group2") == 0) {
		group_id = 1;
		CLI_LOGD("gpio Jtag group2\r\n");
	} else
		cli_misc_help();

	bk_set_jtag_mode(cpu_id, group_id);

	CLI_LOGD("set_jtag_mode end.\r\n");
}


/*
	//For BK7256 ckdiv_bus is 0x8[6]
	//For BK7236 ckdiv_bus is the same as ckdiv_cpu1 0x5[4]

	//============ BK7236 Sample begin============

		//cpu0:160m;cpu1:160m;bus:160m
		setcpufreq 3 2 0 0 0

		//cpu0:160m;cpu1:320m;bus:160m
		setcpufreq 2 0 0 0 1

		//cpu0:120m;cpu1:120m;bus:120m
		setcpufreq 3 3 0 0 0

		//cpu0:120m;cpu1:240m;bus:120m
		setcpufreq 3 1 0 0 1

	//============ BK7236 Sample end============
*/
static void set_cpu_clock_freq(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 6) {
		cli_misc_help();
		return;
	}

	uint32_t cksel_core = 3;
	uint32_t ckdiv_core = 3;
	uint32_t ckdiv_bus  = 0;
	uint32_t ckdiv_cpu0 = 0;
	uint32_t ckdiv_cpu1 = 0;


	cksel_core = os_strtoul(argv[1], NULL, 10);
	ckdiv_core = os_strtoul(argv[2], NULL, 10);

	ckdiv_cpu1  = os_strtoul(argv[5], NULL, 10);

	CLI_LOGD("set_cpu_clock_freq: [cksel_core:%d] [ckdiv_core:%d] [ckdiv_bus:%d] [ckdiv_cpu0:%d] [ckdiv_cpu1:%d]\r\n",
			cksel_core, ckdiv_core, ckdiv_bus, ckdiv_cpu0, ckdiv_cpu1);
	pm_core_bus_clock_ctrl(cksel_core, ckdiv_core, ckdiv_bus, ckdiv_cpu0, ckdiv_cpu1);


	CLI_LOGD("set_cpu_clock_freq end.\r\n");
}


#if CONFIG_COMMON_IO
extern int common_io_test_main(int argc, const char * argv[]);
void test_common_io(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	CLI_LOGD("common io test begin.===================.\r\n");
	common_io_test_main(0, NULL);
	CLI_LOGD("common io test end.====================.\r\n");
}
#endif


void set_printf_uart_port(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	unsigned char uart_port = 0;

	if (argc != 2) {
		BK_LOGD(NULL, "set log/shell uart port 0/1/2");
		return;
	}

	uart_port = os_strtoul(argv[1], NULL, 10);
	BK_LOGD(NULL, "set_printf_uart_port: %d.\r\n", uart_port);

	if (uart_port < UART_ID_MAX) {
#if CONFIG_SHELL_ASYNCLOG
		shell_set_uart_port(uart_port);
#else
		bk_set_printf_port(uart_port);
#endif
	} else {
		BK_LOGD(NULL, "uart_port must be 0/1/2.\r\n");
	}

	BK_LOGD(NULL, "uart_port end.\r\n");
}

#if CONFIG_CACHE_ENABLE
static void prvBUS(void) {
	union {
		char a[10];
		int i;
	} u;

	int *p = (int *) &(u.a[1]);
	BK_LOGD(NULL, "prvBUS() enter(%x).\n", &(u.a[1]));
	BK_LOGD(NULL, "prvBUS() p(%x).\n", p);
	*p = 17;
	BK_LOGD(NULL, "prvBUS() left().\n");
}

__attribute__ ((__optimize__ ("-fno-tree-loop-distribute-patterns"))) \
static void test_fluscache(int count) {
    uint64_t saved_aon_time = 0, cur_aon_time = 0, diff_time = 0;
    uint32_t diff_us = 0;

	bk_pm_module_vote_cpu_freq(PM_DEV_ID_DEFAULT,PM_CPU_FRQ_120M);
	bk_pm_module_vote_cpu_freq(PM_DEV_ID_DEFAULT,PM_CPU_FRQ_480M);

    uint32_t intbk = rtos_enter_critical();

#if CONFIG_AON_RTC
    saved_aon_time = bk_aon_rtc_get_us();
#endif
	for(int i = 0; i < count; i++) {
		flush_all_dcache();
	}

#if CONFIG_AON_RTC
    cur_aon_time = bk_aon_rtc_get_us();
    diff_time = (cur_aon_time - saved_aon_time);
    diff_us = (uint32_t)diff_time;
#endif

    rtos_exit_critical(intbk);

    BK_DUMP_OUT("saved time: 0x%x:0x%08x\r\n", (u32)(saved_aon_time >> 32), (u32)(saved_aon_time & 0xFFFFFFFF));
    BK_DUMP_OUT("curr time: 0x%x:0x%08x\r\n", (u32)(cur_aon_time >> 32), (u32)(cur_aon_time & 0xFFFFFFFF));
    BK_DUMP_OUT("diff time: 0x%x:0x%08x\r\n", (u32)(diff_time >> 32), (u32)(diff_time & 0xFFFFFFFF));

    BK_DUMP_OUT("test_flushcache end, time consume=%d us\r\n", diff_us);

}

void cli_cache_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if ((argc == 2) && (!os_strncmp(argv[1], "help", 4))) {
		cli_misc_cache_help();
		return;
	}

	if (argc < 2) {
		show_cache_config_info();
	}

	uint32_t mode = os_strtoul(argv[1], NULL, 10);
	BK_LOGD(NULL, "cache mode(%d).\n", mode);

	if (mode == 0) {
		enable_dcache(0);
	} else if (mode == 1) {
		enable_dcache(1);
	} else {
		// prvBUS();
		test_fluscache(mode);
	}

}

#endif

__attribute__ ((__optimize__ ("-fno-tree-loop-distribute-patterns"))) \
int32_t cpu_test(uint32_t count) {
#if CONFIG_SOC_BK7236XX || (CONFIG_SOC_BK7239XX) || (CONFIG_SOC_BK7286XX)
	#define portNVIC_SYSTICK_CURRENT_VALUE_REG    ( *( ( volatile uint32_t * ) 0xe000e018 ) )

    uint32_t i;
    volatile uint32_t time_begin = 0;
    volatile uint32_t time_end = 0;

    time_begin = portNVIC_SYSTICK_CURRENT_VALUE_REG;
    for(i = 0; i < count; i++)
    {
		__asm volatile("nop \n");
    }

    time_end = portNVIC_SYSTICK_CURRENT_VALUE_REG;
    CLI_LOGD("cpu_test: count[%d], begin[%d], end[%d], duration[%d].\r\n", count, time_begin, time_end, time_end - time_begin);
#endif

    return 0;
}

#if (defined CONFIG_ISR_DISABLE_TIME_STATISTIC)
void cli_disable_int_check_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if ((argc == 2) && (!os_strncmp(argv[1], "help", 4))) {
		cli_misc_disable_int_interval_check_set_help();
		return;
	}

    extern void bk_set_disable_isr_check_time_value(uint32_t assert_interval, uint32_t trace_interval);
	if (argc > 2) {
		uint32_t assert_time = os_strtoul(argv[1], NULL, 10);
		uint32_t trace_time = os_strtoul(argv[2], NULL, 10);
		bk_set_disable_isr_check_time_value(assert_time, trace_time);
        BK_LOGD(NULL, "set disable isr check time ok,assert time: %dms,trace_time:%dms",assert_time/1000,trace_time/1000);
	}

	return;
}
#endif

static void cli_cpu_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t count = 0;

	if ((argc == 2) && (!os_strncmp(argv[1], "help", 4))) {
		cli_misc_cputest_help();
		return;
	}

	if (argc < 2) {
		CLI_LOGD("cputest [count]\r\n");
		return;
	}
	count = os_strtoul(argv[1], NULL, 10);
	cpu_test(count);
	CLI_LOGD("cputest end.\r\n");
}

#if CONFIG_EXTERN_32K
void cli_set_clock_source(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	extern bk_err_t pm_clk_32k_source_switch(pm_lpo_src_e lpo_src);
	unsigned char clock_source = 0;

	if (argc != 2) {
		BK_LOGD(NULL, "set clock source, 0: PM_LPO_SRC_DIVD, 1: PM_LPO_SRC_X32K.\r\n");
		return;
	}

	clock_source = os_strtoul(argv[1], NULL, 10);
	if (clock_source == 0) {
		pm_clk_32k_source_switch(PM_LPO_SRC_DIVD);
	} else {
		pm_clk_32k_source_switch(PM_LPO_SRC_X32K);
	}

	BK_LOGD(NULL, "set clock source end.\r\n");
}
#endif


static uint32_t  g_dump_flag;
static void timer1_examples_isr(timer_id_t timer_id)
{
    CLI_LOGD("timer1(%d) enter timer1_example_isr\r\n", timer_id);
    
    if (g_dump_flag == 0 ) {
        BK_ASSERT(false);
    } else if (g_dump_flag == 1) {
        os_write_word(0, 0x1234);
    }     
}

void cli_dump_in_context(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if ((argc == 2) && (!os_strncmp(argv[1], "help", 4))) {
		cli_misc_dump_int_context_help();
		return;
	}

    if (argc < 2) {
		cli_misc_help();
		return;
	}

    if (os_strcmp(argv[1], "assert") == 0) {
        CLI_LOGD("set cpu assert in interruption context\r\n");
		g_dump_flag = 0;
	} else if (os_strcmp(argv[1], "crash") == 0) {
        CLI_LOGD("set cpu crash in interruption context\r\n");
		g_dump_flag = 1;
	}  else {
		cli_misc_help();
        return;
	}
    CLI_LOGD("start timer...\r\n");
    BK_LOG_ON_ERR(bk_timer_driver_init());
    BK_LOG_ON_ERR(bk_timer_start(TIMER_ID3, 20, timer1_examples_isr));
}

void cli_dump_disable_int(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if ((argc == 2) && (!os_strncmp(argv[1], "help", 4))) {
		cli_misc_dump_disable_int_help();
		return;
	}

    if (argc < 2) {
		cli_misc_help();
		return;
	}

    uint32_t s_dump_flag;
    uint32_t int_level;
    if (os_strcmp(argv[1], "assert") == 0) {
        CLI_LOGD("set cpu assert when enter critical\r\n");
        s_dump_flag = 1;
	} else if (os_strcmp(argv[1], "crash") == 0) {
        CLI_LOGD("set cpu crash when enter critical\r\n");
        s_dump_flag = 2;
	}  else {
		cli_misc_help();
        return;
	}

    int_level = rtos_enter_critical();

    if (s_dump_flag == 1 ) {
        BK_ASSERT(false);
    } else if (s_dump_flag == 2) {
        os_write_word(0, 0x1234);
    } 

    rtos_exit_critical(int_level);

}

#define MISC_CMD_CNT (sizeof(s_misc_commands) / sizeof(struct cli_command))
static const struct cli_command s_misc_commands[] = {
	{"version", NULL, get_version},
	{"starttype", "show start reason type",cli_show_reset_reason},
	{"id", NULL, get_id},
	{"reboot", "reboot system", reboot},
	{"time",   "system time",   uptime_Command},
	{"mac", "mac [mac], get/set mac. e.g. mac c89346000001", mac_command},

	{"cputest", "cputest [count]", cli_cpu_test},
#if CONFIG_CACHE_ENABLE
	{"cache", "show cache config info", cli_cache_cmd},
#endif

#if CONFIG_ISR_DISABLE_TIME_STATISTIC
	{"disable_int_interval_check_set", "set disable int check interval time(us):interval_time, trace_time", cli_disable_int_check_cmd},
#endif

    {"dump_int_context", "assert or crash in interruption context", cli_dump_in_context},
    {"dump_disable_int", "assert or crash in interruption critical section", cli_dump_disable_int},
};

int cli_misc_init(void)
{
	return cli_register_commands(s_misc_commands, MISC_CMD_CNT);
}
