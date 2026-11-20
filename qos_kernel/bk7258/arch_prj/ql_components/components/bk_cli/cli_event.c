#include "cli.h"
#include <components/event.h>
#include <components/log.h>

typedef struct {
	uint32_t data[32];
} test_event_data_t;

void cli_event_help(void)
{
	CLI_RAW_LOGI("\r\nevent {reg|unreg|post} {mod_id} {event_id} \n");
	CLI_RAW_LOGI("  Start and control an event. \n");
	CLI_RAW_LOGI("  -reg/unreg/post<str><mandatory>: register/unregister/post event. \n");
	CLI_RAW_LOGI("  -mod_id<int><mandatory>: module id. Defined by event_module_t in event.h. \n");
	CLI_RAW_LOGI("  -event_id<int><mandatory>: event id. -1 means all event ids. \n");
	CLI_RAW_LOGI("  example1: event reg 1 -1 \n");
	CLI_RAW_LOGI("  example2: event post 1 -1 \n");
	CLI_RAW_LOGI("  example3: event unreg 1 -1 \n");
}

static bk_err_t cli_event_cb_0(void *arg, event_module_t event_module,
							   int event_id, void *event_data)
{
	CLI_LOGD("event cb#0, event=<%d %d %x>\n", event_module, event_id, (uint32_t)event_data);
	return BK_OK;
}

static bk_err_t cli_event_cb_1(void *arg, event_module_t event_module,
							   int event_id, void *event_data)
{
	CLI_LOGD("event cb#1, event=<%d %d %x>\n", event_module, event_id, (uint32_t)event_data);
	return BK_OK;
}

static bk_err_t cli_event_cb_2(void *arg, event_module_t event_module,
							   int event_id, void *event_data)
{
	CLI_LOGD("event cb#2, event=<%d %d %x>\n", event_module, event_id, (uint32_t)event_data);
	return BK_OK;
}

static void cli_event_reg_cb(int argc, char **argv)
{
	uint32_t module_id = 0;
	uint32_t event_id = 0;
	uint32_t cb_arg = 0;
	uint32_t cb_fn_id = 0;

	module_id = os_strtoul(argv[2], NULL, 10);
	event_id = os_strtoul(argv[3], NULL, 10);

	if (argc >= 4)
		cb_arg = os_strtoul(argv[4], NULL, 10);

	if (argc >= 5)
		cb_fn_id = os_strtoul(argv[5], NULL, 10);

	if (cb_fn_id == 0)
		BK_LOG_ON_ERR(bk_event_register_cb(module_id, event_id, cli_event_cb_0, (void *)cb_arg));
	else if (cb_fn_id == 1)
		BK_LOG_ON_ERR(bk_event_register_cb(module_id, event_id, cli_event_cb_1, (void *)cb_arg));
	else if (cb_fn_id == 2)
		BK_LOG_ON_ERR(bk_event_register_cb(module_id, event_id, cli_event_cb_2, (void *)cb_arg));
	else
		BK_LOG_ON_ERR(bk_event_register_cb(module_id, event_id, cli_event_cb_0, (void *)cb_arg));
}

static void cli_event_unreg_cb(int argc, char **argv)
{
	uint32_t module_id = 0;
	uint32_t event_id = 0;
	uint32_t cb_fn_id = 0;

	module_id = os_strtoul(argv[2], NULL, 10);
	event_id = os_strtoul(argv[3], NULL, 10);

	if (argc >= 4)
		cb_fn_id = os_strtoul(argv[4], NULL, 10);

	if (cb_fn_id == 0)
		BK_LOG_ON_ERR(bk_event_unregister_cb(module_id, event_id, cli_event_cb_0));
	else if (cb_fn_id == 1)
		BK_LOG_ON_ERR(bk_event_unregister_cb(module_id, event_id, cli_event_cb_1));
	else if (cb_fn_id == 2)
		BK_LOG_ON_ERR(bk_event_unregister_cb(module_id, event_id, cli_event_cb_2));
	else
		BK_LOG_ON_ERR(bk_event_unregister_cb(module_id, event_id, cli_event_cb_0));
}


#define CLI_RETURN_ON
static void cli_event_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	test_event_data_t event_data = {0};
	uint32_t module_id = 0;
	uint32_t event_id = 0;
	char *cmd;

	if (argc != 4) {
		CLI_LOGW("invalid argc number\n");
		cli_event_help();
		return;
	}

	cmd = argv[1];

	if (argc >= 3) {
		module_id = os_strtoul(argv[2], NULL, 10);
		event_id = os_strtoul(argv[3], NULL, 10);
	}

	if (os_strcmp(cmd, "reg") == 0) {
		CLI_RET_ON_INVALID_ARGC(argc, 3);
		cli_event_reg_cb(argc, argv);
	} else if (os_strcmp(cmd, "unreg") == 0) {
		CLI_RET_ON_INVALID_ARGC(argc, 3);
		cli_event_unreg_cb(argc, argv);
	} else if (os_strcmp(cmd, "post") == 0) {
		CLI_RET_ON_INVALID_ARGC(argc, 3);
		BK_LOG_ON_ERR(bk_event_post(module_id, event_id, &event_data,
									sizeof(event_data), BEKEN_NEVER_TIMEOUT));
	} else if (os_strcmp(cmd, "dump") == 0) {
		CLI_RET_ON_INVALID_ARGC(argc, 1);
		BK_LOG_ON_ERR(bk_event_dump());
	} else if (os_strcmp(cmd, "init") == 0) {
		CLI_RET_ON_INVALID_ARGC(argc, 1);
		BK_LOG_ON_ERR(bk_event_init());
	} else if (os_strcmp(cmd, "deinit") == 0) {
		CLI_RET_ON_INVALID_ARGC(argc, 1);
		BK_LOG_ON_ERR(bk_event_deinit());
	} else if (os_strcmp(cmd, "test") == 0) {
		CLI_RET_ON_INVALID_ARGC(argc, 1);
		//cli_event_test();
	} else {
		CLI_LOGW("bad parameters\r\n");
		cli_event_help();
	}
}

#define EVENT_CMD_CNT (sizeof(s_event_commands) / sizeof(struct cli_command))
static const struct cli_command s_event_commands[] = {
	{"event", "event {reg|unreg|post} {mod_id} {event_id}", cli_event_cmd},
};

int cli_event_init(void)
{
	return cli_register_commands(s_event_commands, EVENT_CMD_CNT);
}
