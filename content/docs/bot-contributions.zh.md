---
title: "Bot 贡献指南"
description: "为 LocalGen v6 添加、测试并提交内置 C++ Bot。"
date: 2026-04-06T17:55:07+08:00
draft: false
weight: 20
---

开始之前，请阅读 [`CONTRIBUTING.md`](https://github.com/SZXC-WG/LocalGen-new/blob/master/CONTRIBUTING.md) 与 [Bot 指南](https://github.com/SZXC-WG/LocalGen-new/blob/master/src/bots/README.md)。

## 当前支持的集成方式

LocalGen v6 目前只接受**直接构建进应用的 Bot**。外部可执行程序、任意语言客户端与网络 Bot 协议暂不受支持。

内置 Bot 会同时出现在本地对局与 `LocalGen-bot-simulator` 中，因为两个目标共用同一份 `LOCALGEN_BOT_SOURCES` 列表。

## 构建你的 Bot

1. 创建一个名称唯一、使用 C++17 的 `src/bots/MyBot.cpp` 文件。
2. 包含 `src/core/bot.h`。
3. 让 Bot 类继承 `BasicBot`。
4. 实现两个必需方法：
   - `init(index_t playerId, const GameConstantsPack& constants)`
   - `requestMove(const BoardView& boardView, const std::vector<RankItem>& rank)`
5. 使用当前模式注册唯一运行时名称：

```cpp
static BotRegistrar<MyBot> myBotRegistrar("MyBot");
```

6. 将 `src/bots/MyBot.cpp` 加入顶层 `CMakeLists.txt` 的 `LOCALGEN_BOT_SOURCES`。
7. 同时构建 Debug 与 Release 配置。

注册 Bot 时请使用 `BotRegistrar`；旧说明中的 `REGISTER_BOT` 不适用于当前版本。

## 提交前评测

请在多种地图尺寸、手工地图、对手组合与玩家数量下测试。一个有用的报告应包含：

- 完整模拟器命令；
- 对局数与半回合上限；
- 随机地图或指定 `.lgmp` 地图；
- 胜率、TrueSkill、平均排名与击杀；
- 对性能敏感逻辑给出 `--latency` 结果；
- 已知失败场景，以及内存/单回合复杂度。

当前模拟器无法通过种子复现，因此应运行足够多的比赛，不要把单批结果描述为确定性证明。

## Pull Request 期望

- 说明策略与粗略单回合最坏情况复杂度。
- 保持代码可读，在必要处添加文档，并限制在 C++17。
- 避免提交纯随机移动的占位 Bot。
- 验证长局中没有明显泄漏或无限增长的工作量。
- 启用新源文件时同步更新 Bot 阵容文档。

当前阵容见[内置 Bot]({{< relref "docs/built-in-bots" >}})，所有评测选项见[模拟器指南]({{< relref "docs/simulator-guide" >}})。
