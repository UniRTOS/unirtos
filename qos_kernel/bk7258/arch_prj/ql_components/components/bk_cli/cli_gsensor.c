#include <stdlib.h>
#include "cli.h"
#include <components/bk_gsensor.h>
#include <components/bk_gsensor_arithmetic_demo_public.h>

static void cli_gsensor_help(void)
{
	CLI_LOGI("gsensor [init/deinit/open/close/set_normal/set_wakeup] \r\n");
	CLI_LOGI("gsensor [set_shake_param] [shake_threshold_v][stability_threshold_v][shake_check_number][shake_time_limit_ms] \r\n");
}

static void cli_gsensor_ops_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2)
	{
		cli_gsensor_help();
		return;
	}

	if (os_strcmp(argv[1], "init") == 0) {
		BK_LOG_ON_ERR(gsensor_demo_init());
	}else if (os_strcmp(argv[1], "deinit") == 0) {
		gsensor_demo_deinit();
	} else if (os_strcmp(argv[1], "open") == 0) {
		BK_LOG_ON_ERR(gsensor_demo_open());
	} else if(os_strcmp(argv[1], "close") == 0) {
		BK_LOG_ON_ERR(gsensor_demo_close());
	} else if(os_strcmp(argv[1], "set_normal") == 0) {
		BK_LOG_ON_ERR(gsensor_demo_set_normal());
	} else if(os_strcmp(argv[1], "set_wakeup") == 0) {
		BK_LOG_ON_ERR(gsensor_demo_set_wakeup());
	} else if(os_strcmp(argv[1], "set_lowpower") == 0) {
		BK_LOG_ON_ERR(gsensor_demo_lowpower_wakeup());
	} else if(os_strcmp(argv[1], "set_shake_param") == 0) {
		shake_recognition_alg_param_t shake_alg_p;
		shake_alg_p.shake_threshold_v = os_strtoul(argv[2], NULL, 10);
		shake_alg_p.stability_threshold_v = os_strtoul(argv[3], NULL, 10);
		shake_alg_p.shake_check_number = os_strtoul(argv[4], NULL, 10);
		shake_alg_p.shake_time_limit_ms = os_strtoul(argv[5], NULL, 10);
		shake_arithmetic_set_parameter(&shake_alg_p);
	} else {
		cli_gsensor_help();
		return;
	}
}

#define GSENSOR_CMD_CNT (sizeof(s_gsensor_commands) / sizeof(struct cli_command))
static const struct cli_command s_gsensor_commands[] = {
	{"gsensor", "gsensor [init/deinit/open/close/set_normal/set_wakeup]/", cli_gsensor_ops_cmd},
};

int cli_gsensor_init(void)
{
	return cli_register_commands(s_gsensor_commands, GSENSOR_CMD_CNT);
}
