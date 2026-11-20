#include <stdlib.h>
#include "cli.h"
#include "bat_monitor.h"

static void cli_battery_help(void)
{
	CLI_LOGI("battery [init/deinit/get_voltage/get_level/get_battery_info/get_status] \r\n");
}

extern void battery_monitor_init( void );
extern void battery_monitor_deinit( void );
extern int32_t battery_get_charge_level(uint8_t *pLevel);
extern int32_t battery_get_current(uint16_t *pCurrent);
extern int32_t battery_get_voltage(uint16_t *pVoltage);
extern bool battery_if_is_charging(void);
extern IotBatteryInfo_t * battery_if_get_info(void);

static void cli_battery_ops_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2)
	{
		cli_battery_help();
		return;
	}

	if (os_strcmp(argv[1], "init") == 0) {
		battery_monitor_init();
	}else if (os_strcmp(argv[1], "deinit") == 0) {
		battery_monitor_deinit();
	}else if(os_strcmp(argv[1], "get_voltage") == 0) {
        uint16_t voltage = 0;
		BK_LOG_ON_ERR(battery_get_voltage(&voltage));
        printf("Current battery voltage = %u mV\n", voltage);
	}else if(os_strcmp(argv[1], "get_level") == 0) {
        uint8_t level = 0;
		BK_LOG_ON_ERR(battery_get_charge_level(&level));
        printf("Battery level = %u%%\n", level);
	}else if(os_strcmp(argv[1], "get_status") == 0) {
		if(battery_if_is_charging())
            printf("Battery is currently charging!\n");
        else
            printf("Battery is not charging!\n");
	}else if(os_strcmp(argv[1], "get_battery_info") == 0) {
		IotBatteryInfo_t *info = battery_if_get_info();
        if (info)
        {
            printf("Battery min voltage = %u mV, max voltage = %u mV\n",
                info->usMinVoltage, info->usMaxVoltage);
        }
	}else {
		cli_battery_help();
		return;
	}
}

#define BATTERY_CMD_CNT (sizeof(s_battery_commands) / sizeof(struct cli_command))
static const struct cli_command s_battery_commands[] = {
	{"battery", "battery [init/deinit/get_voltage/get_level/get_battery_info/get_status]", cli_battery_ops_cmd},
};

int cli_battery_init(void)
{
	return cli_register_commands(s_battery_commands, BATTERY_CMD_CNT);
}
