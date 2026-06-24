# UniRTOS SDK

[English](README.md) | [简体中文](README_zh.md)

UniRTOS SDK is a unified embedded development kit for multiple Quectel cellular module platforms. It provides consistent APIs and component architecture, supports cross-platform development and efficient porting, and integrates rich middleware, drivers, protocol stacks, and application examples to accelerate the development and deployment of intelligent embedded devices.

## Key Features
- Multi-platform support (for example, eigen_718)
- Based on the FreeRTOS kernel and adaptable to multiple hardware architectures
- Modular design with flexible feature trimming
- Rich drivers and middleware (networking, storage, audio, camera, FOTA, NVM, and more)
- Kconfig-based configuration with menuconfig UI support
- CMake build system with Ninja/Make backends
- Standard application examples and secondary development interfaces

## Directory Structure
```
├── build.sh                # One-click build script (Windows)
├── CMakeLists.txt          # Top-level CMake configuration
├── Kconfig                 # Main configuration entry
├── qos_applications/       # Application examples and business code
│   ├── app_init/           # Application initialization and registration
│   └── unirtos_std/        # Standard application set
├── qos_components/         # System components and middleware
│   ├── components/         # Modular feature components
│   └── system/             # System services, drivers, protocol stacks
├── qos_kernel/             # Platform kernel adaptation and libraries
│   └── eigen_718/          # Typical platform adaptation
├── qos_tools/              # Build toolchain and scripts
│   └── python/             # Python tools for build/config/package
└── ...
```

## Quick Start

### 1. Install Toolchain and CLI

- [Development Preparation](https://www.quectel.com.cn/unirtos/docs?docs_page=%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B/%E5%BC%80%E5%8F%91%E5%87%86%E5%A4%87/%E5%BC%80%E5%8F%91%E5%87%86%E5%A4%87.html)
- [Install Cross-Compilation Toolchain](https://www.quectel.com.cn/unirtos/docs?docs_page=%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B/%E7%8E%AF%E5%A2%83%E6%90%AD%E5%BB%BA/%E7%8E%AF%E5%A2%83%E6%90%AD%E5%BB%BA.html)
- [Install Python3](https://www.python.org/downloads/)
- [Install Git](https://git-scm.com)
- Install `unirtos-cli`: `pip install unirtos-cli`

Verify the environment:

```bash
python --version # Python3
git --version
unirtos --version # 1.0.5 or later
unirtos-cli version # 1.0.9 or later
```

### 2. Create a Project with unirtos-cli

```bash
unirtos-cli new unirtos-app
cd unirtos-app
```

Edit `env_config.json` and configure at least:
- `sdk.version`
- `build.module`

Optional fields:
- `build.version`
- `libraries.list`

### 3. Pull SDK and Libraries

```bash
unirtos-cli env-setup
```

### 4. Configure, Build, and Clean

```bash
unirtos-cli menuconfig
unirtos-cli build
unirtos-cli clean
```

### Build Outputs
- Build outputs are generated in `qos_build/release/<version>/` under the project directory.
- `<version>` is resolved in this order: use the name passed by `unirtos-cli build -v/--version` first; if not provided, use `build.version` from `env_config.json`; otherwise, use the app root folder name (the folder containing `env_config.json`).

## Community Forum
[Visit the Forum](https://forumschinese.quectel.com/c/66-category/66)

## Contribution Guide
Issues and pull requests are welcome.
Please include a clear change summary and basic verification results in your submission.

---

For commercial support or custom development, please contact the maintainers.
