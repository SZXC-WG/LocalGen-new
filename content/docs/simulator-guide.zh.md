---
title: "模拟器指南"
description: "构建并运行 LocalGen 多线程 Bot 模拟器，查看完整参数与输出参考。"
date: 2026-04-06T17:55:08+08:00
draft: false
weight: 60
---

> 上游来源：[`simulator/README.md`](https://github.com/SZXC-WG/LocalGen-new/blob/master/simulator/README.md)，并已对照 `simulator/botSimulator.cpp` 核验。

## 构建与运行

常规 CMake 构建会在桌面目标之外生成 `LocalGen-bot-simulator`：

```bash
cmake -B build -S . -G "Ninja Multi-Config" -DCMAKE_TOOLCHAIN_FILE=/path/to/qt.toolchain.cmake
cmake --build build --config Release
cd build/Release
./LocalGen-bot-simulator --games 10 --bots XiaruizeBot GcBot
```

Windows 请使用 `LocalGen-bot-simulator.exe`。

## 参数

| 参数 | 含义 | 默认值 |
| --- | --- | --- |
| `--games N` | 独立比赛数量 | `8` |
| `--width N` | 随机地图宽度 | `20` |
| `--height N` | 随机地图高度 | `20` |
| `--map PATH` | 重复使用一张 v6 `.lgmp`，不生成随机地图 | 未设置 |
| `--threads N` | 工作线程数 | CPU 并发数，不超过比赛数 |
| `--steps N` | 每局最大半回合数 | `600` |
| `--silent` | 隐藏启动与逐局输出 | 关闭 |
| `--shuffle` | 随机调整 Bot 与玩家编号的映射 | 关闭 |
| `--latency` | 测量平均 `requestMove()` 延迟 | 关闭 |
| `--bots A B ...` | 两个或更多已注册运行时名称 | `XiaruizeBot GcBot` |
| `--help`、`-h` | 打印用法 | — |

数值参数必须为正数。`--map` 只支持 `.lgmp`，会拒绝 `.lg` 与官方 JSON。自定义地图会忽略 `--width` 和 `--height`，并且必须有足够的出生格或空白平地。

## 示例

```bash
# 在独立生成的 20×20 地图上运行十局双 Bot 比赛
./LocalGen-bot-simulator --games 10 --width 20 --height 20 --steps 600 --bots XiaruizeBot GcBot

# 重复使用一张手工 v6 地图
./LocalGen-bot-simulator --games 10 --map maps/arena01.lgmp --steps 600 --bots XiaruizeBot GcBot

# 多 Bot 自由混战，四个线程，并输出延迟列
./LocalGen-bot-simulator --games 100 --threads 4 --latency --bots SmartRandomBot KtqBot ZlyBot GcBot

# 安静日志：只保留最终表格
./LocalGen-bot-simulator --games 50 --silent --bots XiaruizeBot GcBot
```

## 执行模型

- 每个 Bot 都会获得独立队伍编号，因此比赛是自由混战。
- 独立比赛并行运行；逐局结果按完成顺序出现。
- 在更新 TrueSkill 前，结果会按局号顺序汇总，因此即使并行完成，评分计算顺序仍保持稳定。
- `--shuffle` 只会改变玩家编号映射，不会创建队伍。

## 汇总列

最终表格先按 TrueSkill、再按胜场排序。列包括 Bot、TrueSkill、TrueSkill 95% 置信区间、胜场、胜率、胜率 95% 置信区间、平均排名、平均击杀、存活次数、平均最终兵力与平均最终领地。`--latency` 会增加每次 `requestMove()` 调用的平均微秒数。

## 解读限制

- `--steps` 计算半回合。达到上限时，即使仍有多个 Bot 存活，排名领先者也会被计为胜者。
- 当前没有种子参数；随机地图与内部出生分配无法跨调用复现。
- 固定自定义地图只能减少地图差异，并不能让实验变成确定性运行。
- 工具只输出文本表格，不导出 CSV、回放或训练资产。
- 置信区间只描述本次样本，无法消除地图与对手选择偏差。

发布基准时，请同时记录完整命令、源码版本、平台与构建类型。
