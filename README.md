<div align="center">

<img alt="Icon" src="https://github.com/SZXC-WG/LocalGen-new/blob/master/res/img/favicon.png?raw=true" height=128 />

<h1>Local Generals.io</h1>

![GitHub Created At](https://img.shields.io/github/created-at/SZXC-WG/LocalGen-new)
![GitHub Total Downloads](https://img.shields.io/github/downloads/SZXC-WG/LocalGen-new/total)
[![GitHub stars](https://img.shields.io/github/stars/SZXC-WG/LocalGen-new.svg?style=social)](https://github.com/SZXC-WG/LocalGen-new/stargazers)  
![GitHub last commit](https://img.shields.io/github/last-commit/SZXC-WG/LocalGen-new)
[![GitHub commit activity](https://img.shields.io/github/commit-activity/t/SZXC-WG/LocalGen-new?label=total%20commits)](https://github.com/SZXC-WG/LocalGen-new/commits)
[![GitHub contributors](https://img.shields.io/github/contributors/SZXC-WG/LocalGen-new)](https://github.com/SZXC-WG/LocalGen-new/contributors)  
[![GitHub Release](https://img.shields.io/github/v/release/SZXC-WG/LocalGen-new?label=latest%20stable)](https://github.com/SZXC-WG/LocalGen-new/releases/latest)
[![GitHub Release Date](https://img.shields.io/github/release-date/SZXC-WG/LocalGen-new?label=date)](https://github.com/SZXC-WG/LocalGen-new/releases/latest)  
[![GitHub Pre-Release](https://img.shields.io/github/v/release/SZXC-WG/LocalGen-new?include_prereleases&label=latest%20preview)](https://github.com/SZXC-WG/LocalGen-new/releases)
[![GitHub Pre-Release Date](https://img.shields.io/github/release-date-pre/SZXC-WG/LocalGen-new?label=date)](https://github.com/SZXC-WG/LocalGen-new/releases)

[![Qt Build](https://github.com/SZXC-WG/LocalGen-new/actions/workflows/qt-build.yml/badge.svg)](https://github.com/SZXC-WG/LocalGen-new/actions/workflows/qt-build.yml)
[![CodeQL](https://github.com/SZXC-WG/LocalGen-new/actions/workflows/codeql.yml/badge.svg)](https://github.com/SZXC-WG/LocalGen-new/actions/workflows/codeql.yml)
[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/SZXC-WG/LocalGen-new)

</div>

## Intro

Welcome to the **Local Generals.io (LocalGen)** project!

With LocalGen, you can now play **[generals.io](http://generals.io)** **entirely offline**!

Enjoy the game with **ready-to-use** bots or challenge your friends over the **same LAN** network!

Interested? Get started by downloading **LocalGen** from the [releases page](http://github.com/SZXC-WG/LocalGen-new/releases).

Got a new idea or discovered a bug? We welcome contributions! Simply **open an issue or submit a pull request**, and we’ll get back to you as soon as possible.

## [Releases (Changelog)](http://github.com/SZXC-WG/LocalGen-new/releases)

## Weak Robots? Time to Write a New One!

Absolutely! We welcome any form of contribution.

- For v6: Check out the instructions in `docs/bot-contributions.md` and `src/bots/README.md`.
- For v5: Check out the instructions at [#31](../../issues/31) for more details.

## Building the project

You'll need Qt 6.7+, CMake 3.19+, and Ninja 1.10+ installed under PATH. Run the following commands, using the actual Qt6 toolchain path on your system (typically `$QT_ROOT_DIR/lib/cmake/Qt6/qt.toolchain.cmake`):

```bash
cmake -B build -S . -G Ninja -DCMAKE_TOOLCHAIN_FILE=/path/to/qt.toolchain.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Then find the executable under `build/Release`.

## Contributing

LocalGen’s development is organized across three groups of branches:

- **`master` | `v6.x`**:  
  The `master` branch serves as the main development branch for version 6.  
  **Starting from v6, we will be migrating the UI to the Qt library**, aiming for better performance and cross-platform support. The source code will also undergo a significant refactor to improve maintainability.

- **`v5.x`**:  
  This branch covers version 5, which is now going to **long-term maintenance**.  
  Versions 2 to 5 of LocalGen are built with [EGE Graphics](https://xege.org) (24.04), and we’re working on creating an object-oriented system based on it.  
  **Note:** Releases for this branch are **Windows-only** due to EGE 24.04’s limitations. However, if you want to run it on Linux or macOS, [**Wine**](https://www.winehq.org) might be an option. (We have tested, it worked well on a Ubuntu 22.04 LTS machine.)

If you'd like to contribute, please submit a pull request to the `master` branch. We appreciate your help!

## Star History

[![Star History Chart](https://api.star-history.com/svg?repos=SZXC-WG/LocalGen-new&type=Date)](https://star-history.com/#SZXC-WG/LocalGen-new&Date)

## Disclaimer

The "Local Generals.io" project (hereinafter referred to as "LocalGen") is an independent, open-source project developed by the SZXC-WG community. LocalGen is in no way affiliated with, associated with, endorsed by, sponsored by, or connected to `generals.io` or its original developers.

`generals.io`, along with its name, logo, and all associated assets, are the exclusive trademarks and properties of their respective owners. All rights to the original online game are reserved by them.

LocalGen is a strictly unofficial, fan-made tool designed for local, offline environments. Its primary purpose is to provide an isolated space for local gameplay, educational purposes, and AI/bot development. LocalGen does not connect to, interact with, or attempt to replicate the proprietary backend services of the official `generals.io` platform.

Please note:

- Any bugs, issues, or feature requests regarding LocalGen should be directed solely to the issue tracker of this repository and should not be reported to the `generals.io` support team.
- The use of LocalGen does not imply any endorsement by `generals.io`, nor does it grant any special rights or privileges on the official `generals.io` servers.
- Users must comply with the Terms of Service of `generals.io` when using the official platform. The developers of LocalGen bear no responsibility for how individual users choose to utilize this tool in relation to the original game.

By using LocalGen, you acknowledge and agree to this disclaimer.
