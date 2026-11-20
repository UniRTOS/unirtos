#include "cli.h"
#include "wdrv_debug.h"
#include "wdrv_main.h"

#if CONFIG_WIFI_DRIVER_DEBUG

#define WDRV_CMD_CNT (sizeof(s_wdrv_commands) / sizeof(struct cli_command))
static void wdrv_cmd_help(void)
{
    printf("Usage: wdrv <command> [args...]\r\n");
    printf("wdrv stats - Dump driver statistics\r\n");
}
static void wdrv_handle_cli_commmand(char *pcWriteBuffer, int xWriteBufferLen, int argC, char **argV)
{
    if(argC <= 1) {
        printf("Invalid argC = %d.\r\n", argC);
        wdrv_cmd_help();
        return;
    }
    else if ((argC == 2) && 
        (!os_strcmp(argV[1], "-h") || !os_strcmp(argV[1], "help"))) {
        wdrv_cmd_help();
        return;
    }

    /// Wi-Fi command
    if (strcasecmp(argV[1], "stats") == 0) {
        wdrv_print_debug_info();
    }
    else {
        printf("Invalid wdrv command\n");
        wdrv_cmd_help();
    }
}

static const struct cli_command s_wdrv_commands[] = {
    {"wdrv", "wdrv", wdrv_handle_cli_commmand},
};

int wdrv_cli_init(void)
{
    return cli_register_commands(s_wdrv_commands, WDRV_CMD_CNT);
}


#endif // CONFIG_WIFI_DRIVER_DEBUG
