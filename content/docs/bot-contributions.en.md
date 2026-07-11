---
title: "Bot Contributions"
description: "Add, test, and propose a built-in C++ Bot for LocalGen v6."
date: 2026-04-06T17:55:07+08:00
draft: false
weight: 20
---

Before you begin, read [`CONTRIBUTING.md`](https://github.com/SZXC-WG/LocalGen-new/blob/master/CONTRIBUTING.md) and the [Bot guide](https://github.com/SZXC-WG/LocalGen-new/blob/master/src/bots/README.md).

## Supported integration today

LocalGen v6 currently accepts **Bots built directly into the application**. External executables, arbitrary-language clients, and network Bot protocols are not supported yet.

A built-in bot is available to both Local Game and `LocalGen-bot-simulator` because both targets compile the same `LOCALGEN_BOT_SOURCES` list.

## Build your Bot

1. Create one uniquely named `src/bots/MyBot.cpp` file using C++17.
2. Include `src/core/bot.h`.
3. Derive your class from `BasicBot`.
4. Implement both required methods:
   - `init(index_t playerId, const GameConstantsPack& constants)`
   - `requestMove(const BoardView& boardView, const std::vector<RankItem>& rank)`
5. Register a unique runtime name with the current pattern:

```cpp
static BotRegistrar<MyBot> myBotRegistrar("MyBot");
```

6. Add `src/bots/MyBot.cpp` to `LOCALGEN_BOT_SOURCES` in the top-level `CMakeLists.txt`.
7. Build both Debug and Release configurations.

Use `BotRegistrar` to register a Bot; the older `REGISTER_BOT` pattern is no longer supported.

## Evaluate before proposing

Test on multiple map sizes, authored maps, opponent combinations, and player counts. A useful report includes:

- exact simulator command lines;
- number of games and half-turn limit;
- random versus named `.lgmp` maps;
- win rate, TrueSkill, average rank, and kills;
- `--latency` results for performance-sensitive logic;
- known failure modes and memory/per-turn complexity.

Simulator runs are not seed-reproducible today, so use enough games and avoid presenting a single batch as deterministic proof.

## Pull request expectations

- Explain the strategy and rough worst-case per-turn complexity.
- Keep code readable, documented where needed, and within C++17.
- Avoid a pure random-move placeholder.
- Verify long matches do not show obvious leaks or unbounded work.
- Update the bot roster documentation when enabling the new source.

See [Built-in Bots]({{< relref "docs/built-in-bots" >}}) for the current roster and [Simulator Guide]({{< relref "docs/simulator-guide" >}}) for all evaluation options.
