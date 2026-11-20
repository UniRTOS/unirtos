# Bluetooth DUT Test 使用说明

本文件提供 Bluetooth DUT(Device Under Test) 测试入口。启用后，设备会进入 DUT 测试相关流程，并通过经典蓝牙 GAP 接口控制可连接、可发现状态，供 CMW 等测试仪表发现和连接。

## 宏配置

AP 侧需要使能：

```text
CONFIG_BT
CONFIG_BLUTOOTH_ENABLE_BT_DUT_TEST
```

CP 侧需要使能：

```text
CONFIG_BT
```

## CLI 命令

命令格式：

```text
bt_dut_test <sub_cmd>
```

### `bt_dut_test enable`

使能 DUT 测试模式。

执行内容：

- 调用 `ble_dut_start(UART_ID_MAX)` 启动 BLE DUT。
- 设置经典蓝牙为可连接、可发现。
- 发送 HCI `Enable Device Under Test Mode` 命令，opcode 为 `0x1803`。

### `bt_dut_test disable`

关闭 DUT 测试模式。

执行内容：

- 调用 `ble_dut_stop()` 停止 BLE DUT。
- 设置经典蓝牙为不可连接、不可发现。

### `bt_dut_test scan_enable`

单独打开经典蓝牙可连接、可发现状态。

该命令只调用 `bk_bt_gap_set_scan_mode(BK_BT_CONNECTABLE, BK_BT_DISCOVERABLE)`，不会启动 BLE DUT，也不会发送 HCI DUT 使能命令。

## 测试方法

1. 确认 AP/CP 侧宏配置已使能，并完成编译烧录。
2. 短接串口小板 ATE 跳线。
3. 板子上电，等待蓝牙协议栈初始化完成。
4. 在 CLI 下发命令：

   ```text
   ap_cmd bt_dut_test enable
   ```

5. 在 CMW 上搜索设备 `BK_DUT_TEST`，发现后发起连接并进行 DUT 测试。
6. 测试结束后，可执行以下命令关闭 DUT 测试模式：

   ```text
   ap_cmd bt_dut_test disable
   ```
