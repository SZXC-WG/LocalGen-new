---
title: "Bots"
description: "Meet the C++ bots compiled into LocalGen v6 and learn the source-backed contribution path."
date: 2026-04-06T17:54:16+08:00
draft: false
weight: 60
---

## How bots work today

Current v6 supports **built-in C++ bots only**. Each bot is compiled into the desktop app and simulator, then created by name through `BotFactory`. There is no external-bot protocol, plug-in loader, model download, or inference backend in the current source.

## Current source roster

The complexity values are rough per-turn worst-case estimates from the upstream bot README. Here, $n$ is the number of map tiles and $k$ is the number of candidate source stacks considered by a multi-source planner.

| Bot / registry name | Enabled | Approx. cost | Strategy summary |
| --- | --- | --- | --- |
| DummyBot | No | $O(n)$ | Example heuristic greedy |
| SmartRandomBot | Yes | $O(n)$ | Largest-stack greedy |
| KtqBot | Yes | $O(n)$ | Single-focus local greedy |
| XrzBot | No | $O(n)$ | Focused random greedy |
| ZlyBot | Yes | $O(n)$ | Single-focus BFS heuristic |
| ZlyBot v2 | Yes | $O(n \log n)$ | Memory-aware weighted search |
| ZlyBot v2.1 | Yes | $O(n \log n)$ | Dual-focus defensive search |
| SzlyBot | Yes | $O(n)$ | Terrain-weighted BFS heuristic |
| GcBot | Yes | $O(n)$ | Adaptive heuristic BFS |
| XiaruizeBot | Yes | $O(kn^2)$ | Multi-source strategic search |
| KutuBot | Yes | $O(n \log n)$ | Unified strategic objective planner |
| LyBot | No | $O(n^2)$ | Multiplayer objective planner |
| `oimbot` | Yes | $O(n^2)$ | Memory-aware threat/objective planner |

“Enabled” means the source file appears in `LOCALGEN_BOT_SOURCES` and is compiled into the current executables. Exact names matter on the simulator command line; `oimbot` is lowercase.

## Minimal implementation contract

A new built-in bot should:

1. use C++17 and live in one `src/bots/*.cpp` file;
2. include `src/core/bot.h` and derive from `BasicBot`;
3. implement `init(...)` and `requestMove(...)`;
4. register a unique runtime name with `BotRegistrar<YourBot>`;
5. add the file to `LOCALGEN_BOT_SOURCES` in `CMakeLists.txt`;
6. include tests or simulator evidence and concise algorithm/performance notes in the pull request.

Follow the pattern used by a currently compiled bot: the current code uses `BotRegistrar`, not a `REGISTER_BOT` macro.

Read the [detailed built-in bot reference]({{< relref "docs/built-in-bots" >}}), the [contribution workflow]({{< relref "docs/bot-contributions" >}}), or the [simulator guide]({{< relref "docs/simulator-guide" >}}).
