---
title: "文档中心"
description: "基于当前源码整理的 LocalGen v6 实用指南，覆盖构建、游玩、地图制作、Bot 评测与贡献。"
date: 2026-04-06T17:54:17+08:00
draft: false
weight: 50
---

这些页面在整理上游文档的同时，也会对照当前 `master` 实现进行核验。当文字与代码不一致时，指南会明确说明当前行为，而不会把计划功能写成已经完成。

## 从这里开始

- [快速开始]({{< relref "docs/getting-started" >}}) —— 依赖、构建目标、运行路径与打包提示
- [第一局本地游戏与操作]({{< relref "docs/local-game" >}}) —— 开局选项、键鼠控制与当前限制
- [地图编辑器]({{< relref "docs/map-creator" >}}) —— 格子工具、元数据、导入导出格式与地图安装

## Bot 与评测

- [Bot 贡献指南]({{< relref "docs/bot-contributions" >}}) —— 当前仅支持内置 Bot 的贡献流程
- [内置 Bot]({{< relref "docs/built-in-bots" >}}) —— 阵容、API 约定、注册与 CMake 集成
- [模拟器指南]({{< relref "docs/simulator-guide" >}}) —— 完整命令行选项、默认值、输出语义与限制

## 参考与社区

- [关联文件]({{< relref "docs/associated-files" >}}) —— 区分已实现地图格式与仅在文档中出现的未来/旧文件
- [命名规范]({{< relref "docs/naming-method" >}})
- [提交规范]({{< relref "docs/commit-regulations" >}})
- [行为准则]({{< relref "docs/code-of-conduct" >}})

当前 v6 版本为 `6.0.0-dev`。Web/局域网对战、回放加载、外部 Bot 进程、设置持久化与声音播放在所核验源码中均尚未实现。
