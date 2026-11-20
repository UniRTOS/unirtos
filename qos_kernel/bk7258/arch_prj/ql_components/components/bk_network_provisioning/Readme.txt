bk配网组件使用实例说明
1.参照ble_network_provisioning_demo.c实现以下必要函数
1）void demo_network_provisioning_status_cb(bk_network_provisioning_status_t status, void *user_data);
用来接收配网状态/重连状态更新事件
2）void ble_msg_handle_demo_cb(ble_prov_msg_t *msg);
用来处理BLE接收到的msg处理
3）调用bk_ble_provisioning_event_notify_with_data通过BLE发送数据给手机APP

2.参考projects/app/ap/app_main.c
在系统初始化最后调用如下，检测是否配过网，若配过则自动重连，若没有则进入BLE配网模式
#if CONFIG_BK_NETWORK_PROVISIONING_BLE_EXAMPLE
extern void demo_network_provisioning_status_cb(bk_network_provisioning_status_t status, void *user_data);
extern void ble_msg_handle_demo_cb(ble_prov_msg_t *msg);
extern int cli_network_provisioning_init(void);
    //for user to receive network provisioning status change event
    bk_register_network_provisioning_status_cb(demo_network_provisioning_status_cb);
    //if default provisioning type is ble, then set msg handle cb
    bk_ble_provisioning_set_msg_handle_cb(ble_msg_handle_demo_cb);
    bk_network_provisioning_init(BK_NETWORK_PROVISIONING_TYPE_BLE);
    cli_network_provisioning_init();
#endif

3.参考cli_network_provisioning，直接调用bk_network_provisioning_start可以主动进入对应对应配网模式


# BK Network Provisioning Component Usage Example
1. Refer to ble_network_provisioning_demo.c to implement the following necessary functions:
1）void demo_network_provisioning_status_cb(bk_network_provisioning_status_t status, void *user_data); Used to receive network provisioning status/reconnection status update events.
2）void ble_msg_handle_demo_cb(ble_prov_msg_t *msg); Used to handle messages received via BLE.
3）Call bk_ble_provisioning_event_notify_with_data to send data to the mobile app via BLE.

2.Refer to projects/app/ap/app_main.c At the end of system initialization, call the following to check if network provisioning has been done before. If yes, auto-reconnect; if not, enter BLE provisioning mode:
```
#if CONFIG_BK_NETWORK_PROVISIONING_BLE_EXAMPLE
extern void demo_network_provisioning_status_cb(bk_network_provisioning_status_t status, void *user_data);
extern void ble_msg_handle_demo_cb(ble_prov_msg_t *msg);
extern int cli_network_provisioning_init(void);
    // For user to receive network provisioning status change event
    bk_register_network_provisioning_status_cb(demo_network_provisioning_status_cb);
    // If default provisioning type is BLE, then set message handling callback
    bk_ble_provisioning_set_msg_handle_cb(ble_msg_handle_demo_cb);
    bk_network_provisioning_init(BK_NETWORK_PROVISIONING_TYPE_BLE);
    cli_network_provisioning_init();
#endif
```
3.Refer to cli_network_provisioning and directly call bk_network_provisioning_start to actively enter the corresponding provisioning mode.