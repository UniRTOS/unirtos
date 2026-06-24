# UniRTOS SDK

[English](README.md) | 简体中文

UniRTOS SDK 是一个面向移远通信蜂窝模组各类平台的统一嵌入式开发套件，提供一致的接口与组件体系，支持跨平台开发与高效移植，集成丰富的中间件、驱动、协议栈和应用示例，助力快速开发和部署嵌入式智能终端。

## 主要特性
- 支持多平台（如 eigen_718）
- 基于 FreeRTOS 内核，适配多种硬件架构
- 组件化设计，支持灵活裁剪
- 丰富的驱动与中间件（网络、存储、音频、摄像头、FOTA、NVM 等）
- Kconfig 配置系统，支持 menuconfig 图形化配置
- CMake 构建系统，支持 Ninja/Make 多后端
- 提供标准应用示例和二次开发接口

## 目录结构
```
├── build.sh                # 一键构建脚本（Windows）
├── CMakeLists.txt          # 顶层 CMake 配置
├── Kconfig                 # 主配置入口
├── qos_applications/       # 应用示例与业务代码
│   ├── app_init/           # 应用初始化与注册
│   └── unirtos_std/        # 标准应用集
├── qos_components/         # 系统组件与中间件
│   ├── components/         # 可裁剪功能组件
│   └── system/             # 系统服务、驱动、协议栈
├── qos_kernel/             # 平台内核适配与库
│   └── eigen_718/          # 典型平台适配
├── qos_tools/              # 构建工具链与脚本
│   └── python/             # 构建/配置/打包 Python 工具
└── ...
```

## 快速上手

### 1. 安装工具链与 CLI

- [开发准备](https://www.quectel.com.cn/unirtos/docs?docs_page=快速上手/开发准备/开发准备.html)
- [安装交叉编译工具链](https://www.quectel.com.cn/unirtos/docs?docs_page=快速上手/环境搭建/环境搭建.html)
- [安装 Python3](https://www.python.org/downloads/)
- [安装 Git](https://git-scm.com)
- 安装 `unirtos-cli`：`pip install unirtos-cli`

安装完成后，确认以下命令可用：

```bash
python --version # Python3
git --version
unirtos --version # 1.0.5 及以上版本
unirtos-cli version # 1.0.9 及以上版本
```

### 2. 使用 unirtos-cli 创建项目

```bash
unirtos-cli new unirtos-app
cd unirtos-app
```

编辑 `env_config.json`，至少配置以下字段：
- `sdk.version`
- `build.module`

可选字段：
- `build.version`
- `libraries.list`

### 3. 拉取 SDK 与依赖库

```bash
unirtos-cli env-setup
```

### 4. 配置、编译与清理

```bash
unirtos-cli menuconfig
unirtos-cli build
unirtos-cli clean
```

### 产物说明
- 构建产物位于项目目录下的 `qos_build/release/<version>/`
- 其中 `<version>` 取值规则：优先使用 `unirtos-cli build -v/--version` 指定的名称；未指定时使用 `env_config.json` 中的 `build.version`；若仍未配置，则使用应用根目录名称（即 `env_config.json` 所在文件夹名称）。

## 论坛社区
[点击访问](https://forumschinese.quectel.com/c/66-category/66)

## 贡献指南
欢迎提交 Issue、PR 参与共建。
提交时请附上清晰的改动说明与基础验证结果。

---

如需商业支持或定制开发，请联系维护者。
