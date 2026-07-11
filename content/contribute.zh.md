---
title: "参与贡献"
description: "构建活跃的 LocalGen v6 分支，提交聚焦改动，并使用当前工具评测内置 Bot。"
date: 2026-04-06T17:54:16+08:00
draft: false
weight: 80
---

## 新工作应该提交到哪里

请从基于 Qt 6 的 **`master`** 分支开始开发。v1–v4 已停止维护，v5 只接受安全修复与严重 Bug 修复；提交 Pull Request 前请查看 `CONTRIBUTING.md` 与 `CMakeLists.txt` 中的最新要求。

## 主要贡献方向

- 桌面界面、无障碍与地图编辑器改进
- 棋盘/游戏核心逻辑与性能
- `src/bots/` 下的内置 C++ Bot
- 模拟器统计、诊断与评测流程
- 地图、测试、文档、翻译、CI 与打包
- 通过 GitHub Issues 提交聚焦的 Bug 报告与功能建议

v6 目前的 Bot 扩展方式是内置 C++ Bot；外部 Bot 与 Web Game 联机接口尚未开放。

## 提交前先完成构建

你需要 Qt 6.7+、CMake 3.19+、Ninja 1.10+ 与支持 C++17 的编译器：

```bash
cmake -B build -S . -G "Ninja Multi-Config" -DCMAKE_TOOLCHAIN_FILE=/path/to/qt.toolchain.cmake
cmake --build build --config Debug
cmake --build build --config Release
```

使用 Debug 进行诊断，使用 Release 测量性能。请测试桌面目标；若改动相关，也要测试 `LocalGen-bot-simulator`。

## 一个有说服力的 Pull Request

- 只解决一个清晰目标，并说明用户可见或架构层面的影响
- 遵守项目的 C++17 与命名规范
- 附带聚焦测试或手动验证记录
- Bot 改动应给出模拟器命令与结果
- 涉及性能时说明时间与内存表现
- 行为、控制、格式或依赖变化时同步更新文档

贡献 Bot 时，请实现 `init(...)` 与 `requestMove(...)`，使用 `BotRegistrar` 注册，并把文件加入 `LOCALGEN_BOT_SOURCES`。

## 继续阅读

- [快速开始]({{< relref "docs/getting-started" >}})
- [Bot 贡献流程]({{< relref "docs/bot-contributions" >}})
- [内置 Bot API 与阵容]({{< relref "docs/built-in-bots" >}})
- [提交规范]({{< relref "docs/commit-regulations" >}})
- [行为准则]({{< relref "docs/code-of-conduct" >}})
- [Issues](https://github.com/SZXC-WG/LocalGen-new/issues) 与 [Pull Requests](https://github.com/SZXC-WG/LocalGen-new/pulls)
