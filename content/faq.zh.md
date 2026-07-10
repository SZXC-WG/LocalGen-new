---
title: "常见问题"
description: "基于当前源码回答 LocalGen v6 的玩法、地图、Bot、联机、回放与构建问题。"
date: 2026-04-06T17:54:16+08:00
draft: false
weight: 90
---

## LocalGen 和 generals.io 官方有关系吗？

没有。LocalGen 是一个独立的粉丝开源项目，不隶属于 generals.io 或其原始开发者，也未获得其认可、赞助或官方合作。

## 当前 v6 处于什么状态？

活跃的 `master` 源码将自身标记为 `6.0.0-dev`。本地对局、地图编辑器与 Bot 模拟器已经可用；部分可见菜单入口仍只是占位功能。

## 可以通过局域网或 Web Game 联机吗？

当前 v6 不可以。点击 Web Game 只会显示“尚不可用”。README 中较早的局域网描述与当前实现不一致。

## 可以加载回放吗？

暂时不行。关联文件说明列出了 `.lgr` 与 `.lgra`，但当前 Load Replay 按钮只是占位入口，源码中也没有回放读取器。

## 一局本地游戏可以有几名人类玩家？

最多一名。2–16 个玩家槽位中，只有第一个可以选择 `Human`，其他槽位只能选择内置 Bot。当前本地对局是自由混战，不是组队模式。

## 可以使用自己的地图吗？

可以。本地对局会从可执行文件旁的 `maps/` 目录发现有效 `.lgmp`。地图编辑器可以打开 `.lg`、`.lgmp` 与官方地图 JSON，但只能保存为 `.lg` 或 `.lgmp`。所选地图必须为所有玩家提供足够的出生点或空白平地。

## LocalGen 需要联网吗？

离线对局、本地地图编辑与模拟器都不需要。地图编辑器中可选的 “Import from Generals.io” 操作会请求公开地图 API，因此该操作需要网络。

## 可以用 Python 编写 Bot，或运行外部 Bot 进程吗？

当前 v6 集成不支持。受支持的 Bot 是编译进 LocalGen 的 C++17 源文件；仓库目前没有外部 Bot 网络协议或模型运行时。

## 模拟器实验可以复现吗？

不能精确复现。当前命令行没有种子选项，地图种子来自系统随机源。你可以记录全部选项并运行足够多的比赛，但不应把两次调用描述为确定性复现。

## v6 设置保存在什么位置？

关联文件文档提到 `settings.json`，但当前桌面应用尚未实现设置持久化。本地对局选项只作用于当次启动。

## 应该去哪里报告 Bug 或提出功能建议？

- [GitHub Issues](https://github.com/SZXC-WG/LocalGen-new/issues)
- [GitHub Discussions](https://github.com/SZXC-WG/LocalGen-new/discussions)
- [贡献指南]({{< relref "contribute" >}})
- [行为准则]({{< relref "docs/code-of-conduct" >}})
- [提交规范]({{< relref "docs/commit-regulations" >}})
- [Bot 贡献流程]({{< relref "docs/bot-contributions" >}})
