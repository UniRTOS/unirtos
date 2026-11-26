# 1029 平台底包生成记录

## 来源与验证范围

- 唯一平台来源：QSR02A04_C_SDK_LTE_E_BETA261029 / V01（Beta）。
- 使用完整原包的独立副本进行平台 `new` 编译，未使用 0511 底包。
- 原包 5351 个文件在启动前逐文件校验 SHA256；原始解压目录不参与编译。
- Git 集成基线：0782df77b1c55feb0fe66f09a2eeb72161af3de1。
- 2026-11-05 续编结束时间：15:32:39；十份底包独立验收通过时间：15:44:34。
- 下表全部十个型号均完成平台编译和底包校验，**不代表轻量应用构建或设备验证通过**。

## 已完成的型号

| 型号 | 压缩包字节数 | SHA256 |
|---|---:|---|
| [EG800ZEU_LA](qos_kernel/eigen_718/images/EG800ZEU_LA/gccout.7z) | 6833983 | 4E838CCADF25BBAF8529201016C6C7A97E388C43C7A0500E2FEC5EF19ACBAD46 |
| [EC800ZCN_LF](qos_kernel/eigen_718/images/EC800ZCN_LF/gccout.7z) | 6828928 | 957DA23CBE9D9DFA8780CD478DE0993D7DF29A05421AC1FBBD90AD599B4DA086 |
| [EG800ZCN_LA](qos_kernel/eigen_718/images/EG800ZCN_LA/gccout.7z) | 6829733 | F236E46F8EE1C39651FCB95D08525260211ED66062CB2DB4A929FEE5950DED91 |
| [EG800ZCN_LG](qos_kernel/eigen_718/images/EG800ZCN_LG/gccout.7z) | 6823806 | D09CFBD1319E83D0086BC95EDAD5266710F68D7557A45F3FC80EA8E360F88A6C |
| [EG800ZCN_LH](qos_kernel/eigen_718/images/EG800ZCN_LH/gccout.7z) | 11181673 | 7CBB6B7FB92F7BEB930126D36D375189185E1530997343E88BC67F23DD7628AA |
| [EG800ZEU_LG](qos_kernel/eigen_718/images/EG800ZEU_LG/gccout.7z) | 6833156 | 03014F5FD427DCA5243588FBD8A5DC660FBE7EB42601C26812AD660442BD91F7 |
| [EG800ZEU_LH](qos_kernel/eigen_718/images/EG800ZEU_LH/gccout.7z) | 9923445 | E235447A65602BF9F6C522C7B9031EAEF594F7707A7E634BCE422C1A87AEBC52 |
| [EG800ZGL_LA](qos_kernel/eigen_718/images/EG800ZGL_LA/gccout.7z) | 6839600 | D1A1749BD33E9613ED311C79177450F88D729596F7CC8A2FECC6024ACE4C6FE9 |
| [EG800ZLA_LA](qos_kernel/eigen_718/images/EG800ZLA_LA/gccout.7z) | 6829993 | 5DE6A5DB100B8F7E4A5EE9043619714CB1FE60B3D48153A2D6198FCD09912FD2 |
| [EG915ZEU_LA](qos_kernel/eigen_718/images/EG915ZEU_LA/gccout.7z) | 6828488 | 77BD56EF008620D0FFA5E3B129C7A3BBEE3BBC33A5E92F068FDC1C0A435B1A04 |

配套元数据：
- [EG800ZEU_LA 构建记录](qos_kernel/eigen_718/images/EG800ZEU_LA/platform-build.json)
- [EC800ZCN_LF 构建记录](qos_kernel/eigen_718/images/EC800ZCN_LF/platform-build.json)
- [EG800ZCN_LA 构建记录](qos_kernel/eigen_718/images/EG800ZCN_LA/platform-build.json)
- [EG800ZCN_LG 构建记录](qos_kernel/eigen_718/images/EG800ZCN_LG/platform-build.json)
- [EG800ZCN_LH 构建记录](qos_kernel/eigen_718/images/EG800ZCN_LH/platform-build.json)
- [EG800ZEU_LG 构建记录](qos_kernel/eigen_718/images/EG800ZEU_LG/platform-build.json)
- [EG800ZEU_LH 构建记录](qos_kernel/eigen_718/images/EG800ZEU_LH/platform-build.json)
- [EG800ZGL_LA 构建记录](qos_kernel/eigen_718/images/EG800ZGL_LA/platform-build.json)
- [EG800ZLA_LA 构建记录](qos_kernel/eigen_718/images/EG800ZLA_LA/platform-build.json)
- [EG915ZEU_LA 构建记录](qos_kernel/eigen_718/images/EG915ZEU_LA/platform-build.json)

十份底包均已完成：
1. 外层压缩包及内部基础固件包的 7z 完整性校验。
2. 解压后逐文件 SHA256、字节数及文件集合校验。
3. 按实际消费方式解压到全新的 qos_build，确认 gccout 顶层目录、桥接对象、固件包和链接脚本存在。
4. 检查外层文件和内部基础固件包的文件名清单，不含 .c/.cpp/.cc/.cxx/.s/.asm 源文件。
5. 确认两个拟闭源实现的对象已编译；底包 AP ELF 中有 USART 驱动及 ql_udate_bin_crc、ql_get_update_bin_info、ql_check_updater_image_crc 定义。

压缩包中的 generated 目录保存对应型号的生成头及配置快照；a04-platform.json 标识版本和型号，a04-files.json 记录文件摘要。

## 型号变体与构建过程说明

- EG800ZCN_LH、EG800ZEU_LH：EC718HM / IMS，CP 使用 audio 分支。
- EG800ZGL_LA：EC718PM H2 / GLO，CP 使用 oc/glo 分支。
- 其余七个型号：EC718PM H1 / OC。
- 首轮在 EG800ZCN_LA 期间被 Ctrl+C 中断；用户随后授权续编，已跳过前两份经 SHA 验证的成品，对其余八型号逐一干净编译并打包。
- 已知原包批处理问题：最初一次 clall 在 ql_ap_lib 未设置时会报错但继续；配置完成后的第二次完整 cleanall 成功执行。本批最终平台构建成功，未通过混用旧包或跳过链接来绕过错误。此清理顺序/错误传播问题仍建议单独修复。

每型号包内配置快照含 13 项生成头/配置，并不是完整的平台编译环境备份。原生日志 debug 生成头、完整合并 target 配置及响应文件未全部纳入该快照；后续应用消费需要核对配套配置，尤其不要沿用普通 OC 宏来假设 LH/GL 的配置正确。

## 当前不能宣称已完成的项目

- 2026-11-05 用户提供的 CLI 成功日志确认：EG800ZCN_LA / eigen_718（A04）及 FCM665D / bk7258 的应用构建通过。此记录来自用户实际构建，不是本次提交前重新编译。
- 其余九个 EC718 型号的完整应用构建尚未验证；十份平台底包通过不代表十型号应用全部通过。包内构建时的 app_build_validated=false 是平台产物生成当时的记录，后续应用验证范围以本节及 SDK_ORIGIN 为准。
- 应用构建入口已对齐当前底包清单，并执行版本/型号/来源及逐文件摘要校验、按型号恢复生成配置；这些前置检查不等于应用编译通过。不要把现有 gccout 缓存用于不同型号，操作见[应用验证指导](A04_BUILD_GUIDE.md)。
- 不要手工恢复旧 0511 底包来填补尚未完成的型号。
- 本轮未验证其他平台的共享头/库 ABI 兼容性，未做设备测试。
- 设备烧录和实机功能均未验证；本分支用于升级评审，不代表正式发布验收完成。