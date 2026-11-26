# 文件说明

记录`Link SDK-4.x`的更新历史

# 更新内容

+ 2020-05-06: SDK 4.0.0版本正式发布
+ 2021-04-20: SDK 4.1.0版本正式发布
  +  新增安全隧道和远程调试功能
  +  新增AT模组驱动框架,支持模组快速适配
  +  mbedtls安全层抽象
  +  支持单报文多url的OTA
  +  新增基于mqtt的动态注册功能
  +  支持MQTT 5.0协议
+ 2022-10-28:
  +  支持coap协议接入及动态注册
  +  mbedtls版本升级至2.13.1
+ 2023-01-17:
  +  支持数据压缩的功能
  +  DTLS支持裁剪
+ 2023-09-15:
  + OTA支持动态下发的port
  + 支持接入mqtt云网关
  + 支持任意topic
+ 2023-11-06: SDK 4.2.0版本正式发布
  + 端口8883，支持新证书S1，有效期至2053年
  + mbedtls支持sha384/512算法

# 模块状态


| 模块名                                      | 更新时间    | Commit ID
|---------------------------------------------|-------------|---------------------------------------------
| 核心模块(core)                              | 2024-07-19  | 371cfb3fca12ce58715040f2c548ba0b32a44db7
| 基于MQTT的动态注册(components/dynreg-mqtt)  | 2023-11-06  | b232a650901de606d75761d71d6de30b127ac3bc
| 引导服务模块(components/bootstrap)          | 2020-07-30  | 566135cde8a63c2b5944877ea8c8189c0712b4f7
| 时间同步模块(components/ntp)                | 2023-11-17  | a2102c7e33dad39e8a566186078753930a9cad49
| 设备诊断模块(components/diag)               | 2021-10-18  | 18897e1421952e4eda11e82a61f573654f2bcc69
| 固件升级模块(components/ota)                | 2023-11-27  | 9e7a1a2dacd26002f953055c4a84972904bcfcac
| 设备日志模块(components/logpost)            | 2023-11-17  | a77ff38278b6bbc54f024639e6a0f33929f57cd5
| 物模型模块(components/data-model)           | 2023-11-17  | 3ad9e4b5177f73a02119542c6fce66003af1f119
| 动态注册(components/dynreg)                 | 2022-12-13  | cd069167a8f93afa0950b0fb03593001e4c29ddf
| (components/mqtt_upload)                    | 2023-11-27  | f6082a5e91ca6ac3d0f7dba22272a4b6fcc4bf04
| (components/compress)                       | 2023-11-17  | 026fbea4611c22d7e22d527126c022fa83cb4f07
| 任务管理模块(components/task)               | 2023-11-27  | e62f402fbcbb4821c7c7a6c04206a3fbe41f6862



