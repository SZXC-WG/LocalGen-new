---
title: "Simulator"
description: "Evaluate LocalGen's registered bots over parallel random-map or v6-map matches from the command line."
date: 2026-04-06T17:54:16+08:00
draft: false
weight: 70
---

## Bot evaluation without the desktop UI

`LocalGen-bot-simulator` uses the same board, game, map loader, and built-in bot registry as the desktop app. It runs free-for-all matches in worker threads and prints per-game outcomes plus an aggregate table.

After a Release build, run it from `build/Release`:

```bash
./LocalGen-bot-simulator --games 10 --width 20 --height 20 --steps 600 --bots XiaruizeBot GcBot
./LocalGen-bot-simulator --games 10 --map maps/arena01.lgmp --steps 600 --bots XiaruizeBot GcBot
./LocalGen-bot-simulator --games 50 --silent --bots XiaruizeBot GcBot
```

Windows uses `LocalGen-bot-simulator.exe` with the same options.

## What the summary reports

- FFA TrueSkill rating and 95% confidence interval
- wins, win rate, and win-rate confidence interval
- average rank, kills, final army, and final land
- survival count
- average `requestMove()` latency when `--latency` is enabled

## Important semantics

- Defaults are 8 games, a 20×20 random map, 600 **half-turns**, automatic worker count, and `XiaruizeBot GcBot`.
- `--map` accepts only v6 `.lgmp`; when present, `--width` and `--height` are ignored.
- At least two valid registered bot names are required. Runtime names are case-sensitive.
- `--silent` keeps only the final summary. Without it, games print as soon as they finish, so output order may differ from game number order.
- When the step limit is reached, the current leader is reported and counted as the winner even if several bots remain alive.
- Runs are **not reproducible from the current CLI**: there is no seed option, and map seeds come from system randomness.

See the [complete flag reference]({{< relref "docs/simulator-guide" >}}) before publishing benchmark results.
