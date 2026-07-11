---
title: "Downloads"
description: "Download LocalGen for your platform or build the Qt app yourself."
date: 2026-04-06T17:54:16+08:00
draft: false
weight: 20
---

## Published builds

Find the latest LocalGen release notes and downloads on GitHub Releases:

- [Open LocalGen Releases on GitHub](https://github.com/SZXC-WG/LocalGen-new/releases)
- [Browse the release timeline on this site]({{< relref "releases" >}})

Read the notes attached to a release before downloading. `6.0.0-dev` is the in-progress development version, not a stable installer.

## Build targets

| Platform | Current packaging targets |
| --- | --- |
| Linux | x86_64 and ARM64 AppImage |
| macOS | Intel and Apple silicon DMG |
| Windows | x86_64 and ARM64 MSVC ZIP; x86_64 MinGW and LLVM-MinGW ZIP |

Availability varies by release, so check the file list before downloading. If a Linux AppImage reports a missing OpenGL runtime on Debian or Ubuntu, install `libopengl0`.

## After extracting or installing

Keep the packaged `maps/` and `fonts/` directories beside the desktop executable. Local Game discovers `.lgmp` files from that `maps/` directory, and the app expects the bundled Quicksand font files at startup.

With v6 you can start an offline local match or open Map Creator. Web Game and replay loading are not ready yet, even though their buttons are visible.

## Build the development version

Building requires Qt 6.7+, CMake 3.19+, Ninja 1.10+, and a C++17 compiler. See [Getting Started]({{< relref "docs/getting-started" >}}) for the exact configure, build, run, and packaging commands.

For current feature answers before downloading, read the [FAQ]({{< relref "faq" >}}).
