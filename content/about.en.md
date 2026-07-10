---
title: "About"
description: "What LocalGen v6 currently implements, how its tools fit together, and where the development boundaries are."
date: 2026-04-06T17:54:16+08:00
draft: false
weight: 10
---

## A local strategy game and bot workbench

**Local Generals.io (LocalGen)** recreates generals.io-style territory strategy in a local, open-source application. Its active `master` branch is a C++17 and Qt 6 rewrite with two executables: the `LocalGen-new` desktop app and the `LocalGen-bot-simulator` command-line evaluator.

The source currently identifies itself as **`6.0.0-dev`**. That suffix matters: this is an active development line, and visible menu entries do not always represent finished features.

## Capability snapshot

| Available now | Not implemented in current v6 source |
| --- | --- |
| Offline local FFA against built-in bots | LAN or Web Game networking |
| Random maps and installed `.lgmp` maps | Replay loading or playback |
| Map Creator with `.lg`, `.lgmp`, and official JSON import | External bot executables or a bot network protocol |
| Chat/event log, leaderboard, and optional analysis chart | Sound playback and persisted v6 settings |
| Parallel bot simulator with aggregate statistics | Seeded/reproducible simulator runs from the CLI |

Only the first player slot can be human, so the current desktop UI supports one human at most. Every participant is assigned a distinct team, making local games and simulator matches free-for-all.

## Technology and platform reach

- **Language:** C++17
- **UI/runtime:** Qt 6.7+ (`Widgets`, `Svg`, `Network`, and `Charts`)
- **Build:** CMake 3.19+ with Ninja 1.10+
- **CI targets:** Linux x86_64/ARM64, macOS Intel/Apple silicon, and Windows x86_64/ARM64
- **Source license:** GPL-3.0-or-later; bundled Quicksand fonts use SIL OFL-1.1

## Files in everyday use

- **`.lgmp`** is the current v6 binary map format. It stores title, author, creation time, description, dimensions, and compressed tile data.
- **`.lg`** is the legacy v5 map format. The current editor can open and save its board data, but it cannot preserve v6 metadata.
- **Official `.json`** maps can be opened or imported into the editor, then saved as `.lg` or `.lgmp`; JSON export is not implemented.
- **`.lgr` / `.lgra`** and **`settings.json`** are listed in the upstream associated-files reference, but current v6 has no replay loader or settings persistence implementation.

## Bots are compiled code, not downloadable models

Current v6 bots are C++ classes compiled into both executables and registered through `BotFactory`. There are no neural-model files, inference backends, plug-in packages, or external-bot runtime in this source tree.

## Explore next

- [Download a published build]({{< relref "downloads" >}})
- [Compare published releases]({{< relref "releases" >}})
- [Build from source]({{< relref "docs/getting-started" >}})
- [Browse the documentation]({{< relref "docs" >}})
- [Review file formats]({{< relref "docs/associated-files" >}})
- [Read the project disclaimer]({{< relref "disclaimer" >}})
- [Visit the upstream repository](https://github.com/SZXC-WG/LocalGen-new)
