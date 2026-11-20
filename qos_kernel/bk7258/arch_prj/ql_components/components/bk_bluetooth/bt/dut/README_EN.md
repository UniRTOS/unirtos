# Bluetooth DUT Test Guide

This file provides the Bluetooth DUT(Device Under Test) test entry. After DUT mode is enabled, the device enters the related test flow and uses the Classic Bluetooth GAP interface to control the connectable and discoverable states, so that CMW or other test instruments can discover and connect to the device.

## Macro Configuration

Enable the following macros on the AP side:

```text
CONFIG_BT
CONFIG_BLUTOOTH_ENABLE_BT_DUT_TEST
```

Enable the following macro on the CP side:

```text
CONFIG_BT
```

## CLI Commands

Command format:

```text
bt_dut_test <sub_cmd>
```

### `bt_dut_test enable`

Enable DUT test mode.

Actions:

- Call `ble_dut_start(UART_ID_MAX)` to start BLE DUT.
- Set Classic Bluetooth to connectable and discoverable.
- Send the HCI `Enable Device Under Test Mode` command with opcode `0x1803`.

### `bt_dut_test disable`

Disable DUT test mode.

Actions:

- Call `ble_dut_stop()` to stop BLE DUT.
- Set Classic Bluetooth to non-connectable and non-discoverable.

### `bt_dut_test scan_enable`

Enable only the Classic Bluetooth connectable and discoverable states.

This command only calls `bk_bt_gap_set_scan_mode(BK_BT_CONNECTABLE, BK_BT_DISCOVERABLE)`. It does not start BLE DUT or send the HCI DUT enable command.

## Test Procedure

1. Make sure the required AP/CP macros are enabled, then build and flash the firmware.
2. Short the ATE jumper on the serial adapter board.
3. Power on the board and wait until the Bluetooth stack initialization is complete.
4. Run the following CLI command:
  ```text
   ap_cmd bt_dut_test enable
  ```
5. Search for the `BK_DUT_TEST` device on the CMW. After the device is found, connect to it and start DUT testing.
6. After the test is complete, run the following command to disable DUT test mode:
  ```text
   ap_cmd bt_dut_test disable
  ```

