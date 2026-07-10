---
title: "Local Generals.io"
description: "基于源码整理的 LocalGen 指南，覆盖离线对局、地图编辑、内置 Bot、模拟器、版本发布与贡献文档。"
date: 2026-04-06T17:54:16+08:00
draft: false
---

**Local Generals.io（LocalGen）** 是一个受 generals.io 启发的非官方开源策略游戏。当前 `master` 分支是基于 Qt 6 的重构版本，源码中的版本号为 **`6.0.0-dev`**。

## 当前 v6 源码已经实现的功能

- **本地对局** —— 在完全离线的自由混战中，让一名可选的人类玩家与内置 C++ Bot 对战；地图可随机生成，也可从已安装的 `.lgmp` 中选择。
- **地图编辑器** —— 创建与编辑 v5/v6 地图、填写元数据、打开官方地图 JSON，或按名称联网导入公开地图。
- **Bot 模拟器** —— 并行重复运行两个或更多已注册 Bot 的比赛，并比较胜率、TrueSkill、排名、击杀、兵力、领地及可选延迟数据。
- **对局工具** —— 使用移动队列、事件/聊天记录、可折叠排行榜，以及可选的兵力/领地分析图表。

## 当前开发边界

主菜单中虽然已经显示 **Web Game** 和 **Load Replay**，但当前源码里两者都只是占位入口。局域网/网络对战、回放播放、外部 Bot 进程、v6 设置持久化与声音播放均尚未实现。本地对局本身无需联网；只有地图编辑器中可选的 generals.io 地图导入功能需要网络。

## 选择你的入口

- [下载已发布版本]({{< relref "downloads" >}})
- [从源码构建 v6]({{< relref "docs/getting-started" >}})
- [开始第一局本地对战]({{< relref "docs/local-game" >}})
- [制作或转换地图]({{< relref "docs/map-creator" >}})
- [了解内置 Bot]({{< relref "bots" >}})
- [运行 Bot 评测]({{< relref "simulator" >}})

LocalGen 与 generals.io 相互独立，也未获得其所有者的认可或赞助。项目边界请参阅[免责声明]({{< relref "disclaimer" >}})。
