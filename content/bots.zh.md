---
title: "Bot"
description: "了解编译进 LocalGen v6 的 C++ Bot，以及基于当前源码的准确贡献方式。"
date: 2026-04-06T17:54:16+08:00
draft: false
weight: 60
---

## 当前 Bot 的工作方式

当前 v6 **只支持内置 C++ Bot**。每个 Bot 都会编译进桌面应用与模拟器，再由 `BotFactory` 按名称创建。当前源码中没有外部 Bot 协议、插件加载器、模型下载或推理后端。

## 当前源码阵容

复杂度来自上游 Bot README 中的粗略单回合最坏情况估计。这里的 $n$ 表示地图格子数，$k$ 表示多源规划器考虑的候选起点数量。

| Bot / 注册名称 | 已启用 | 近似复杂度 | 策略简介 |
| --- | --- | --- | --- |
| DummyBot | 否 | $O(n)$ | 示例型启发式贪心 |
| SmartRandomBot | 是 | $O(n)$ | 最大兵力栈贪心 |
| KtqBot | 是 | $O(n)$ | 单目标局部贪心 |
| XrzBot | 否 | $O(n)$ | 聚焦型随机贪心 |
| ZlyBot | 是 | $O(n)$ | 单目标 BFS 启发式 |
| ZlyBot v2 | 是 | $O(n \log n)$ | 带记忆的加权搜索 |
| ZlyBot v2.1 | 是 | $O(n \log n)$ | 双焦点防守搜索 |
| SzlyBot | 是 | $O(n)$ | 地形加权 BFS 启发式 |
| GcBot | 是 | $O(n)$ | 自适应启发式 BFS |
| XiaruizeBot | 是 | $O(kn^2)$ | 多源战略搜索 |
| KutuBot | 是 | $O(n \log n)$ | 统一战略目标规划 |
| LyBot | 否 | $O(n^2)$ | 多人局目标规划 |
| `oimbot` | 是 | $O(n^2)$ | 带记忆的威胁/目标规划 |

“已启用”表示该源文件出现在 `LOCALGEN_BOT_SOURCES` 中，并会被编译进当前可执行程序。模拟器命令行中的名称需要完全匹配；`oimbot` 使用全小写。

## 最小实现约定

一个新的内置 Bot 应当：

1. 使用 C++17，并将实现放在单个 `src/bots/*.cpp` 文件中；
2. 包含 `src/core/bot.h` 并继承 `BasicBot`；
3. 实现 `init(...)` 与 `requestMove(...)`；
4. 通过 `BotRegistrar<YourBot>` 注册唯一运行时名称；
5. 将源文件加入 `CMakeLists.txt` 的 `LOCALGEN_BOT_SOURCES`；
6. 在 Pull Request 中附带测试或模拟器证据，以及简洁的算法与性能说明。

请参考当前已编译 Bot 的写法：现有代码使用 `BotRegistrar`，并不存在 `REGISTER_BOT` 宏。

继续阅读[内置 Bot 详细参考]({{< relref "docs/built-in-bots" >}})、[贡献流程]({{< relref "docs/bot-contributions" >}})或[模拟器指南]({{< relref "docs/simulator-guide" >}})。
