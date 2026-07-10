---
title: "模拟器"
description: "通过命令行，在随机地图或 v6 地图上并行评测 LocalGen 已注册 Bot。"
date: 2026-04-06T17:54:16+08:00
draft: false
weight: 70
---

## 不启动桌面界面的 Bot 评测

`LocalGen-bot-simulator` 与桌面应用共用棋盘、游戏、地图加载器和内置 Bot 注册表。它会通过多个工作线程运行自由混战，并输出逐局结果与最终汇总表。

完成 Release 构建后，在 `build/Release` 中运行：

```bash
./LocalGen-bot-simulator --games 10 --width 20 --height 20 --steps 600 --bots XiaruizeBot GcBot
./LocalGen-bot-simulator --games 10 --map maps/arena01.lgmp --steps 600 --bots XiaruizeBot GcBot
./LocalGen-bot-simulator --games 50 --silent --bots XiaruizeBot GcBot
```

Windows 使用 `LocalGen-bot-simulator.exe`，选项保持不变。

## 汇总表包含什么

- FFA TrueSkill 评分与 95% 置信区间
- 胜场、胜率及胜率置信区间
- 平均排名、击杀、最终兵力与最终领地
- 存活次数
- 启用 `--latency` 后的平均 `requestMove()` 延迟

## 重要语义

- 默认运行 8 局、使用 20×20 随机地图、上限 600 个**半回合**、自动选择线程数，并让 `XiaruizeBot GcBot` 对战。
- `--map` 只接受 v6 `.lgmp`；指定后会忽略 `--width` 与 `--height`。
- 至少需要两个有效的已注册 Bot 名称，且运行时名称区分大小写。
- `--silent` 只保留最终汇总。不启用时，对局会在完成后立即输出，所以显示顺序可能与局号不同。
- 达到步数上限时，即使仍有多个 Bot 存活，当前领先者也会被报告并计为胜者。
- 当前命令行**无法复现实验随机性**：它没有种子选项，地图种子来自系统随机源。

发布基准结果前，请先阅读[完整参数参考]({{< relref "docs/simulator-guide" >}})。
