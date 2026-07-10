---
title: "Built-in Bots"
description: "The compiled LocalGen bot roster, API contract, runtime names, and CMake integration."
date: 2026-04-06T17:55:08+08:00
draft: false
weight: 70
---

> Upstream overview: [`src/bots/README.md`](https://github.com/SZXC-WG/LocalGen-new/blob/master/src/bots/README.md). Enabled status is checked against `LOCALGEN_BOT_SOURCES` in current `CMakeLists.txt`.

## Current roster

| Bot | Author | Enabled | Approx. cost | Summary |
| --- | --- | --- | --- | --- |
| DummyBot | AppOfficer | No | $O(n)$ | Example heuristic greedy |
| SmartRandomBot | AppOfficer / GoodCoder666 | Yes | $O(n)$ | Largest-stack greedy |
| KtqBot | ktq1124298818 / GoodCoder666 | Yes | $O(n)$ | Single-focus local greedy |
| XrzBot | xiaruize0911 | No | $O(n)$ | Focused random greedy |
| ZlyBot | AppOfficer | Yes | $O(n)$ | Single-focus BFS heuristic |
| ZlyBot v2 | AppOfficer | Yes | $O(n \log n)$ | Memory-aware weighted search |
| ZlyBot v2.1 | AppOfficer | Yes | $O(n \log n)$ | Dual-focus defensive search |
| SzlyBot | GoodCoder666 | Yes | $O(n)$ | Terrain-weighted BFS heuristic |
| GcBot | GoodCoder666 | Yes | $O(n)$ | Adaptive heuristic BFS |
| XiaruizeBot | xiaruize0911 | Yes | $O(kn^2)$ | Multi-source strategic search |
| KutuBot | pinkHC | Yes | $O(n \log n)$ | Unified strategic objective planner |
| LyBot | pinkHC | No | $O(n^2)$ | Multiplayer objective planner |
| `oimbot` | oimasterkafuu | Yes | $O(n^2)$ | Memory-aware threat/objective planner |

The ten enabled runtime names are `SmartRandomBot`, `KtqBot`, `XiaruizeBot`, `SzlyBot`, `ZlyBot`, `ZlyBot v2`, `ZlyBot v2.1`, `GcBot`, `KutuBot`, and `oimbot`.

## API contract

`BasicBot` inherits from `Player`. A concrete class must implement:

```cpp
void init(index_t playerId, const GameConstantsPack& constants) override;
void requestMove(const BoardView& boardView,
                 const std::vector<RankItem>& rank) override;
```

Moves are appended to the inherited queue. Bots may also consume game events through the optional `onWin`, `onCapture`, `onSurrender`, and `onText` hooks.

## Registration and build integration

Register a runtime name using `BotRegistrar`:

```cpp
static BotRegistrar<MyBot> myBotRegistrar("MyBot");
```

Then add the source to `LOCALGEN_BOT_SOURCES`. That list is compiled into the desktop app and simulator. If a source exists under `src/bots/` but is absent from the list, it is not available at runtime.

Bot names are used literally by `--bots`; capitalization and spaces must match. Passing an unknown name prints the available registry names and exits.

Continue with the [Bot Contributions]({{< relref "docs/bot-contributions" >}}) checklist.
