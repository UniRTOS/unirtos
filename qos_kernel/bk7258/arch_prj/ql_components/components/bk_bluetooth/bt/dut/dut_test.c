#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bk_cli.h"
#include <common/sys_config.h>
#include <common/bk_kernel_err.h>
#include "cli.h"
#include "components/bluetooth/bk_dm_bluetooth_types.h"
#include "components/bluetooth/bk_dm_bt.h"
#include "components/bluetooth/bk_dm_gap_bt_types.h"
#include "components/bluetooth/bk_dm_gap_bt.h"
#include "components/bluetooth/bk_dm_gap_bt_types.h"

#define LOG_TAG "bt_dut"
#define CMD_RSP_SUCCEED               "DUT TEST RSP:OK\r\n"
#define CMD_RSP_ERROR                 "DUT TEST RSP:ERROR\r\n"
extern void ble_dut_start(uint8_t uart_id);
extern void ble_dut_stop(void);
extern uint16_t hci_common_api_handler_no_params(uint16_t opcode);
#define HCI_ENABLE_DEVICE_UNDER_TEST_MODE_OPCODE                0x1803U
#define BT_hci_enable_device_under_test_mode() hci_common_api_handler_no_params(HCI_ENABLE_DEVICE_UNDER_TEST_MODE_OPCODE)
static char *dut_device_name = "BK_DUT_TEST";

static void bt_dut_write_response(char *pcWriteBuffer, int xWriteBufferLen, const char *cmd_rsp)
{
    size_t rsp_len;

    if ((pcWriteBuffer == NULL) || (xWriteBufferLen <= 0) || (cmd_rsp == NULL))
    {
        return;
    }

    rsp_len = strlen(cmd_rsp);
    if (rsp_len >= (size_t)xWriteBufferLen)
    {
        rsp_len = xWriteBufferLen - 1;
    }

    memcpy(pcWriteBuffer, cmd_rsp, rsp_len);
    pcWriteBuffer[rsp_len] = '\0';
}

void bt_dut_test_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    char *cmd_rsp = CMD_RSP_SUCCEED;
    bk_err_t err = BK_OK;
    uint16_t hci_ret = BK_OK;

    if ((argc < 2) || (argv == NULL) || (argv[1] == NULL))
    {
        goto error;
    }

    if (strcmp(argv[1], "enable") == 0)
    {
        BK_LOGI(LOG_TAG, "enable dut test \n");
        ble_dut_start(UART_ID_MAX);
        err = bk_bt_gap_set_scan_mode(BK_BT_CONNECTABLE, BK_BT_DISCOVERABLE);
        if (err != BK_OK)
        {
            BK_LOGE(LOG_TAG, "set scan mode failed: %d\n", err);
            // ble_dut_stop();
            goto error;
        }

        hci_ret = BT_hci_enable_device_under_test_mode();
        if (hci_ret != BK_OK)
        {
            BK_LOGE(LOG_TAG, "enable dut mode failed: %d\n", hci_ret);
            err = bk_bt_gap_set_scan_mode(BK_BT_NON_CONNECTABLE, BK_BT_NON_DISCOVERABLE);
            if (err != BK_OK)
            {
                BK_LOGE(LOG_TAG, "set scan mode failed: %d\n", err);
            }
            // ble_dut_stop();
            goto error;
        }
    }
    else if (strcmp(argv[1], "disable") == 0)
    {
        ble_dut_stop();
        err = bk_bt_gap_set_scan_mode(BK_BT_NON_CONNECTABLE, BK_BT_NON_DISCOVERABLE);
        if (err != BK_OK)
        {
            BK_LOGE(LOG_TAG, "set scan mode failed: %d\n", err);
            goto error;
        }

        BK_LOGI(LOG_TAG, "disable dut test \n");
    }
    else if (strcmp(argv[1], "scan_enable") == 0)
    {
        BK_LOGI(LOG_TAG, "scan_enable\n");
        err = bk_bt_gap_set_scan_mode(BK_BT_CONNECTABLE, BK_BT_DISCOVERABLE);
        if (err != BK_OK)
        {
            BK_LOGE(LOG_TAG, "set scan mode failed: %d\n", err);
            goto error;
        }
    }
    else
    {
        goto error;
    }
    bt_dut_write_response(pcWriteBuffer, xWriteBufferLen, cmd_rsp);
    return;
error:
    cmd_rsp = CMD_RSP_ERROR;
    bt_dut_write_response(pcWriteBuffer, xWriteBufferLen, cmd_rsp);
}
static void bt_dut_gap_callback(bk_gap_bt_cb_event_t event, bk_bt_gap_cb_param_t *param)
{
    bk_err_t err = BK_OK;

    BK_LOGI(LOG_TAG, "bt_dut_gap_callback event: %d \n", event);
    switch (event)
    {
    case BK_BT_GAP_ACL_CONN_CMPL_STAT_EVT:
    {
        BK_LOGI(LOG_TAG, "connected \n");
        err = bk_bt_gap_set_scan_mode(BK_BT_NON_CONNECTABLE, BK_BT_NON_DISCOVERABLE);
        if (err != BK_OK)
        {
            BK_LOGE(LOG_TAG, "set scan mode failed: %d\n", err);
        }
        break;
    }
    case BK_BT_GAP_ACL_DISCONN_CMPL_STAT_EVT:
    {
        BK_LOGI(LOG_TAG, "disconnected \n");
        err = bk_bt_gap_set_scan_mode(BK_BT_CONNECTABLE, BK_BT_DISCOVERABLE);
        if (err != BK_OK)
        {
            BK_LOGE(LOG_TAG, "set scan mode failed: %d\n", err);
        }
        break;
    }
    default:
        break;
    }
}
static const struct cli_command s_dut_test_commands[] =
{
    {"bt_dut_test", "bt_dut_test arg1 arg2 ... argn",  bt_dut_test_command},
};
void dut_test_init()
{
    bk_err_t err = BK_OK;

    err = cli_register_commands(s_dut_test_commands, sizeof(s_dut_test_commands) / sizeof(struct cli_command));
    if (err != BK_OK)
    {
        BK_LOGE(LOG_TAG, "register cli command failed: %d\n", err);
    }

    err = bk_bt_gap_set_local_name((uint8_t *)dut_device_name, os_strlen(dut_device_name));
    if (err != BK_OK)
    {
        BK_LOGE(LOG_TAG, "set local name failed: %d\n", err);
    }

    err = bk_bt_gap_register_callback(bt_dut_gap_callback);
    if (err != BK_OK)
    {
        BK_LOGE(LOG_TAG, "register gap callback failed: %d\n", err);
    }

    BK_LOGI(LOG_TAG, "dut_test_init\n");
}
