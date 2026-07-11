---
title: "Getting Started"
description: "Download or build LocalGen v6, locate both executables, and verify the required maps and fonts."
date: 2026-07-10T00:00:00+08:00
draft: false
weight: 5
---

## Choose a published build or source build

For a packaged version, start at [GitHub Releases](https://github.com/SZXC-WG/LocalGen-new/releases) and read that release's notes and file list. To try or contribute to the latest development work, build `6.0.0-dev` from the `master` branch.

## Source prerequisites

- Qt 6.7 or newer, including Widgets, SVG, Network, and Charts
- CMake 3.19 or newer
- Ninja 1.10 or newer
- a C++17 compiler

Make sure `cmake` and `ninja` are on `PATH`, and locate Qt's toolchain file, usually:

```text
$QT_ROOT_DIR/lib/cmake/Qt6/qt.toolchain.cmake
```

## Configure and build

From the repository root:

```bash
cmake -B build -S . -G "Ninja Multi-Config" -DCMAKE_TOOLCHAIN_FILE=/path/to/qt.toolchain.cmake
cmake --build build --config Release
```

Use `--config Debug` when diagnosing code. Contributors should test both Debug and Release; performance comparisons should use Release.

The build creates two targets:

| Target | Purpose |
| --- | --- |
| `LocalGen-new` | Qt desktop app: Local Game and Map Creator |
| `LocalGen-bot-simulator` | Command-line built-in-bot evaluator |

## Run the desktop app

Release outputs are normally under `build/Release`:

```text
Windows: build\Release\LocalGen-new.exe
Linux:   build/Release/LocalGen-new
macOS:   build/Release/LocalGen-new.app
```

The post-build step copies `maps/` and `fonts/` beside the desktop executable. Do not separate them: the Local Game picker scans `maps/*.lgmp`, while startup loads the three bundled Quicksand font files.

To confirm the simulator build:

```bash
cd build/Release
./LocalGen-bot-simulator --games 8 --bots XiaruizeBot GcBot
```

On Windows, use `LocalGen-bot-simulator.exe`.

## Packaging notes

- **Windows:** run Qt's `windeployqt` after building if you need a portable folder.
- **macOS:** use `bash scripts/package-macos-dmg.sh build/Release/LocalGen-new.app LocalGen-new.dmg`; avoid `macdeployqt ... -dmg` because it can produce an invalid app signature for this project.
- **Linux:** if a packaged AppImage fails because OpenGL is missing on Debian/Ubuntu, install `libopengl0`.

## What to expect on launch

Local Game and Map Creator are functional. Web Game and Load Replay currently display “not available yet.” Sound playback is also unavailable. Continue with [First Local Game & Controls]({{< relref "docs/local-game" >}}).
