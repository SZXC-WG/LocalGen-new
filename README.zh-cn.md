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

## 简介

欢迎来到 **Local Generals.io（LocalGen）** 项目！

有了 LocalGen，你如今可以**完全离线**地游玩 **[generals.io](http://generals.io)** 了——开箱即用的 bots 助你即刻开局，也能在同一**局域网**下与好友一较高下！

是否已经跃跃欲试？只需前往 [Releases 页面](http://github.com/SZXC-WG/LocalGen-new/releases)下载 **LocalGen**，即刻开启旅程。

若有新想法，或是发现了 Bug，我们竭诚欢迎你的贡献！只需**提交 Issue 或发起 Pull Request**，我们会尽快与你联系。

若你想贡献自己编写的 bot，请参阅 [贡献指南](./CONTRIBUTING.md) 与 [bot README](./src/bots/README.md) 中的相关说明。

## [版本发布（更新日志）](http://github.com/SZXC-WG/LocalGen-new/releases)

> [!IMPORTANT]
>
> 如果所提供的 Linux AppImage 无法运行，请尝试安装 OpenGL 运行时：
>
> ```bash
> sudo apt install libopengl0
> ```

## 构建本项目

你需要预先在 PATH 中安装 Qt 6.7+、CMake 3.19+ 与 Ninja 1.10+。然后运行下列命令，并将路径替换为系统中实际的 Qt6 工具链位置（通常为 `$QT_ROOT_DIR/lib/cmake/Qt6/qt.toolchain.cmake`）：

```bash
cmake -B build -S . -G "Ninja Multi-Config" -DCMAKE_TOOLCHAIN_FILE=/path/to/qt.toolchain.cmake
cmake --build build --config Release
```

完成后，可在 `build/Release` 目录下找到可执行文件。

> [!NOTE]
>
> 在 Windows 上，请在构建完成后使用 `windeployqt` 制作便携式软件包。
>
> 在 macOS 上，构建会生成 `build/Release/LocalGen-new.app`。如需部署 Qt 框架、净化 DMG 中的副本内容、进行临时签名，并将其打包为可在 Finder 中正常启动的 DMG 映像，请运行：`bash scripts/package-macos-dmg.sh build/Release/LocalGen-new.app LocalGen-new.dmg`
>
> 此处请避免使用 `macdeployqt ... -dmg`：在较新的 Qt/macOS 组合下，该命令可能在被拷贝的框架上残留不符合规范的 Bundle 元数据，从而破坏生成 DMG 内的应用签名。
