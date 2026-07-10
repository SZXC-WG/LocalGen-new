---
title: "项目简介"
description: "了解 LocalGen v6 当前真正实现的功能、各项工具之间的关系，以及仍在开发中的边界。"
date: 2026-04-06T17:54:16+08:00
draft: false
weight: 10
---

## 本地策略游戏与 Bot 试验台

**Local Generals.io（LocalGen）** 用本地开源应用重现 generals.io 风格的领土策略玩法。活跃的 `master` 分支采用 C++17 与 Qt 6 重构，包含两个可执行程序：桌面应用 `LocalGen-new` 与命令行评测工具 `LocalGen-bot-simulator`。

源码当前标记的版本是 **`6.0.0-dev`**。其中的 `-dev` 很重要：这是仍在开发的主线，菜单中已经出现的入口并不一定代表功能已经完成。

## 功能快照

| 当前可用 | 当前 v6 源码尚未实现 |
| --- | --- |
| 与内置 Bot 进行离线本地自由混战 | 局域网或 Web Game 联机 |
| 随机地图与已安装的 `.lgmp` 地图 | 回放加载或播放 |
| 可导入 `.lg`、`.lgmp` 与官方 JSON 的地图编辑器 | 外部 Bot 可执行程序或 Bot 网络协议 |
| 事件/聊天记录、排行榜与可选分析图 | 声音播放与 v6 设置持久化 |
| 输出综合统计的并行 Bot 模拟器 | 通过命令行种子复现实验 |

只有第一个玩家槽位可以选择人类，因此当前桌面界面最多支持一名人类玩家。每名参与者都会被分配到独立队伍，所以本地对局与模拟器比赛都是自由混战。

## 技术与平台

- **语言：** C++17
- **界面/运行库：** Qt 6.7+（`Widgets`、`Svg`、`Network`、`Charts`）
- **构建：** CMake 3.19+ 与 Ninja 1.10+
- **CI 目标：** Linux x86_64/ARM64、macOS Intel/Apple 芯片、Windows x86_64/ARM64
- **源码许可证：** GPL-3.0-or-later；随附 Quicksand 字体使用 SIL OFL-1.1

## 日常会遇到的文件

- **`.lgmp`** 是当前 v6 的二进制地图格式，保存标题、作者、创建时间、描述、尺寸与压缩格子数据。
- **`.lg`** 是 v5 旧地图格式。当前编辑器可以读取和保存棋盘数据，但无法保留 v6 元数据。
- **官方 `.json`** 地图可以在编辑器中打开或导入，再另存为 `.lg` 或 `.lgmp`；当前不能导出 JSON。
- **`.lgr` / `.lgra`** 与 **`settings.json`** 出现在上游关联文件说明中，但当前 v6 尚无回放加载器或设置持久化实现。

## Bot 是编译代码，不是可下载模型

当前 v6 的 Bot 是编译进两个可执行程序的 C++ 类，并通过 `BotFactory` 注册。源码树中没有神经网络模型文件、推理后端、插件包或外部 Bot 运行时。

## 接下来可以看什么

- [下载已发布构建]({{< relref "downloads" >}})
- [比较公开版本]({{< relref "releases" >}})
- [从源码构建]({{< relref "docs/getting-started" >}})
- [浏览文档中心]({{< relref "docs" >}})
- [查看文件格式]({{< relref "docs/associated-files" >}})
- [阅读项目免责声明]({{< relref "disclaimer" >}})
- [访问上游仓库](https://github.com/SZXC-WG/LocalGen-new)
