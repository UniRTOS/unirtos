# 1. HAL 简介
HAL(HardWare Abstraction Layer)是用纯C编写的操作系统抽象层，它将系统底层的细节隐藏在底层，为内存、文件IO、线程、互斥锁等系统调用以及通用字符串等常用数据结构定义、网络通信等操作提供统一的接口给上层ByteRTCLite使用。用户可以实现相应系统的HAL，从而实现跨平台的ByteRTCLite。

# 2. 目录结构
```
hal
├── configs
├── inc
├── README.md
├── src
│   ├── common
│   └── platform
│       └── x86_64
└── third_party
    └── include
```
* configs: 配置文件，如mbedtls的配置文件
* inc: 头文件（不可更改）
* src: 源文件（需要用户实现）
    * common: 通用的文件
    * platform: 平台相关的实现
        * x86_64: x86_64平台的实现
* third_party: 依赖三方库的头文件

# 3. 平台适配
此工程的目标产物为libVolcEngineRTCHal.a, 需要用户自行实现。配合libVolcEngineRTC.a即可使用完整的ByteRTCLite功能。用户在实现目标平台硬件抽象层时**可参考或复用其他平台的实现**。

## 3.1 方式一：将源码集成到项目中
* 将hal目录拷贝到项目中
* 实现平台相关的文件
* 编译libVolcEngineRTCHal.a
* 连续火山商务或研发获取相应平台libVolcEngineRTC.a
* 编译完整项目

## 3.2 方式二: 在hal工程中编译得到静态库
* 实现平台相关的文件
* 编译libVolcEngineRTCHal.a
    * 修改CMakeLists.txt, 指定目标平台工具链及依赖库的头文件目录
    * mkdir build
    * cd build
    * cmake ..
    * make
* 连续火山商务或研发获取相应平台libVolcEngineRTC.a
* 编译完整项目
