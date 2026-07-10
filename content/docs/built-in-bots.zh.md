---
title: "内置 Bot"
description: "LocalGen 已编译 Bot 阵容、API 约定、运行时名称与 CMake 集成说明。"
date: 2026-04-06T17:55:08+08:00
draft: false
weight: 70
---

> 上游概览：[`src/bots/README.md`](https://github.com/SZXC-WG/LocalGen-new/blob/master/src/bots/README.md)。启用状态已对照当前 `CMakeLists.txt` 中的 `LOCALGEN_BOT_SOURCES` 核验。

## 当前阵容

| Bot | 作者 | 已启用 | 近似复杂度 | 简介 |
| --- | --- | --- | --- | --- |
| DummyBot | AppOfficer | 否 | $O(n)$ | 示例型启发式贪心 |
| SmartRandomBot | AppOfficer / GoodCoder666 | 是 | $O(n)$ | 最大兵力栈贪心 |
| KtqBot | ktq1124298818 / GoodCoder666 | 是 | $O(n)$ | 单目标局部贪心 |
| XrzBot | xiaruize0911 | 否 | $O(n)$ | 聚焦型随机贪心 |
| ZlyBot | AppOfficer | 是 | $O(n)$ | 单目标 BFS 启发式 |
| ZlyBot v2 | AppOfficer | 是 | $O(n \log n)$ | 带记忆的加权搜索 |
| ZlyBot v2.1 | AppOfficer | 是 | $O(n \log n)$ | 双焦点防守搜索 |
| SzlyBot | GoodCoder666 | 是 | $O(n)$ | 地形加权 BFS 启发式 |
| GcBot | GoodCoder666 | 是 | $O(n)$ | 自适应启发式 BFS |
| XiaruizeBot | xiaruize0911 | 是 | $O(kn^2)$ | 多源战略搜索 |
| KutuBot | pinkHC | 是 | $O(n \log n)$ | 统一战略目标规划 |
| LyBot | pinkHC | 否 | $O(n^2)$ | 多人局目标规划 |
| `oimbot` | oimasterkafuu | 是 | $O(n^2)$ | 带记忆的威胁/目标规划 |

十个已启用运行时名称为 `SmartRandomBot`、`KtqBot`、`XiaruizeBot`、`SzlyBot`、`ZlyBot`、`ZlyBot v2`、`ZlyBot v2.1`、`GcBot`、`KutuBot` 与 `oimbot`。

## API 约定

`BasicBot` 继承自 `Player`。具体 Bot 类必须实现：

```cpp
void init(index_t playerId, const GameConstantsPack& constants) override;
void requestMove(const BoardView& boardView,
                 const std::vector<RankItem>& rank) override;
```

移动会加入继承而来的队列。Bot 还可以通过可选的 `onWin`、`onCapture`、`onSurrender` 与 `onText` 钩子消费游戏事件。

## 注册与构建集成

使用 `BotRegistrar` 注册运行时名称：

```cpp
static BotRegistrar<MyBot> myBotRegistrar("MyBot");
```

随后把源文件加入 `LOCALGEN_BOT_SOURCES`。这份列表会同时编译进桌面应用与模拟器。如果某个文件存在于 `src/bots/`，却没有加入列表，它就不会在运行时出现。

`--bots` 会按字面使用 Bot 名称；大小写与空格必须完全一致。传入未知名称时，程序会打印可用注册名称并退出。

下一步请参阅[Bot 贡献指南]({{< relref "docs/bot-contributions" >}})。
