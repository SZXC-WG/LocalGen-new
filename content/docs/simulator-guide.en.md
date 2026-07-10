---
title: "Simulator Guide"
description: "Build and run LocalGen's threaded bot simulator with a complete option and output reference."
date: 2026-04-06T17:55:08+08:00
draft: false
weight: 60
---

> Upstream source: [`simulator/README.md`](https://github.com/SZXC-WG/LocalGen-new/blob/master/simulator/README.md), checked against `simulator/botSimulator.cpp`.

## Build and run

The normal CMake build creates `LocalGen-bot-simulator` alongside the desktop target:

```bash
cmake -B build -S . -G "Ninja Multi-Config" -DCMAKE_TOOLCHAIN_FILE=/path/to/qt.toolchain.cmake
cmake --build build --config Release
cd build/Release
./LocalGen-bot-simulator --games 10 --bots XiaruizeBot GcBot
```

Use `LocalGen-bot-simulator.exe` on Windows.

## Options

| Option | Meaning | Default |
| --- | --- | --- |
| `--games N` | Number of independent matches | `8` |
| `--width N` | Random-map width | `20` |
| `--height N` | Random-map height | `20` |
| `--map PATH` | Reuse one custom v6 `.lgmp` instead of random maps | unset |
| `--threads N` | Worker threads | hardware concurrency, capped to game count |
| `--steps N` | Maximum half-turns per match | `600` |
| `--silent` | Suppress startup and per-game output | off |
| `--shuffle` | Randomize bot-to-player-index mapping | off |
| `--latency` | Measure average `requestMove()` latency | off |
| `--bots A B ...` | Two or more registered runtime names | `XiaruizeBot GcBot` |
| `--help`, `-h` | Print usage | — |

Numeric values must be positive. `--map` supports `.lgmp` only; `.lg` and official JSON are rejected. A custom map ignores `--width` and `--height` and must have enough spawns or empty plain tiles.

## Examples

```bash
# Ten two-bot games on independently generated 20×20 maps
./LocalGen-bot-simulator --games 10 --width 20 --height 20 --steps 600 --bots XiaruizeBot GcBot

# Reuse one authored v6 map
./LocalGen-bot-simulator --games 10 --map maps/arena01.lgmp --steps 600 --bots XiaruizeBot GcBot

# Multi-bot FFA, four workers, latency column enabled
./LocalGen-bot-simulator --games 100 --threads 4 --latency --bots SmartRandomBot KtqBot ZlyBot GcBot

# Machine-readable-ish quiet log: final table only
./LocalGen-bot-simulator --games 50 --silent --bots XiaruizeBot GcBot
```

## Execution model

- Every bot receives a distinct team ID, so matches are FFA.
- Independent games run in parallel; per-game lines appear in completion order.
- Results are accumulated in game-number order before TrueSkill updates, keeping rating calculation order stable despite parallel completion.
- `--shuffle` changes player-index mapping; it does not create teams.

## Summary columns

The final table is sorted by TrueSkill, then wins. It reports Bot, TrueSkill, TrueSkill 95% CI, wins, win rate, win-rate 95% CI, average rank, average kills, survival count, average final army, and average final land. `--latency` adds average microseconds per `requestMove()` call.

## Interpretation limits

- `--steps` counts half-turns. At the limit, the rank leader is counted as winner even if multiple bots survive.
- No seed option exists; random maps and internal spawn choices are not reproducible across invocations.
- A fixed custom map reduces map variation but does not make a run deterministic.
- The tool prints text tables only; it does not export CSV, replays, or trained assets.
- Confidence intervals describe the sampled run and do not remove map/opponent-selection bias.

Record the exact command, source revision, platform, and build type with any published benchmark.
