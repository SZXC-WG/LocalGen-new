---
title: "Contribute"
description: "Build the active LocalGen v6 branch, contribute focused changes, and benchmark built-in bots with the current tools."
date: 2026-04-06T17:54:16+08:00
draft: false
weight: 80
---

## Where new work belongs

The active development branch is **`master`**, the Qt-based v6 line. Versions 1–4 are no longer maintained; v5 accepts only security and critical bug fixes. The upstream contribution guide is marked as an early draft, so verify procedural details against current code and `CMakeLists.txt`.

## Main contribution paths

- desktop UI, accessibility, and Map Creator improvements
- core board/game logic and performance
- built-in C++ bots under `src/bots/`
- simulator statistics, diagnostics, and evaluation workflows
- maps, tests, documentation, translations, CI, and packaging
- focused bug reports and feature proposals through GitHub Issues

External bot processes and Web Game networking are not implemented in current v6, so they should not be documented as available contribution APIs.

## Build before submitting

You need Qt 6.7+, CMake 3.19+, Ninja 1.10+, and a C++17 compiler:

```bash
cmake -B build -S . -G "Ninja Multi-Config" -DCMAKE_TOOLCHAIN_FILE=/path/to/qt.toolchain.cmake
cmake --build build --config Debug
cmake --build build --config Release
```

Use Debug for diagnosis and Release for performance measurements. Test the desktop target and, when relevant, `LocalGen-bot-simulator`.

## A strong pull request

- keeps one clear purpose and explains the user-visible or architectural effect
- follows the project's C++17 and naming conventions
- includes focused tests or manual verification notes
- gives simulator commands and results for bot changes
- discusses time and memory behavior when performance is relevant
- updates documentation when behavior, controls, formats, or prerequisites change

For a bot, implement `init(...)` and `requestMove(...)`, register it with `BotRegistrar`, and add its file to `LOCALGEN_BOT_SOURCES`.

## Continue

- [Getting Started]({{< relref "docs/getting-started" >}})
- [Bot contribution workflow]({{< relref "docs/bot-contributions" >}})
- [Built-in bot API and roster]({{< relref "docs/built-in-bots" >}})
- [Commit regulations]({{< relref "docs/commit-regulations" >}})
- [Code of Conduct]({{< relref "docs/code-of-conduct" >}})
- [Issues](https://github.com/SZXC-WG/LocalGen-new/issues) and [Pull Requests](https://github.com/SZXC-WG/LocalGen-new/pulls)
