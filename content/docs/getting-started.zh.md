---
title: "快速开始"
description: "下载或构建 LocalGen v6，找到两个可执行程序，并确认地图与字体目录完整。"
date: 2026-07-10T00:00:00+08:00
draft: false
weight: 5
---

## 选择发布版或源码构建

如果希望使用已经打包的版本，请从 [GitHub Releases](https://github.com/SZXC-WG/LocalGen-new/releases) 开始，并阅读该版本的说明与附件列表。当前 `master` 源码将自身标记为 `6.0.0-dev`，应把它视为活跃开发版本。

## 源码构建依赖

- Qt 6.7 或更高版本，并包含 Widgets、SVG、Network 与 Charts
- CMake 3.19 或更高版本
- Ninja 1.10 或更高版本
- 支持 C++17 的编译器

请确保 `cmake` 与 `ninja` 已加入 `PATH`，并找到 Qt 工具链文件，通常位于：

```text
$QT_ROOT_DIR/lib/cmake/Qt6/qt.toolchain.cmake
```

## 配置与构建

在仓库根目录运行：

```bash
cmake -B build -S . -G "Ninja Multi-Config" -DCMAKE_TOOLCHAIN_FILE=/path/to/qt.toolchain.cmake
cmake --build build --config Release
```

诊断代码时可使用 `--config Debug`。贡献者应同时测试 Debug 与 Release；性能比较必须使用 Release。

构建会生成两个目标：

| 目标 | 用途 |
| --- | --- |
| `LocalGen-new` | Qt 桌面应用：本地对局与地图编辑器 |
| `LocalGen-bot-simulator` | 命令行内置 Bot 评测工具 |

## 运行桌面应用

Release 输出通常位于 `build/Release`：

```text
Windows: build\Release\LocalGen-new.exe
Linux:   build/Release/LocalGen-new
macOS:   build/Release/LocalGen-new.app
```

构建后步骤会把 `maps/` 与 `fonts/` 复制到桌面可执行文件旁。不要将它们分开：本地对局会扫描 `maps/*.lgmp`，启动时还会加载三份随附 Quicksand 字体。

可以用下面的命令确认模拟器构建：

```bash
cd build/Release
./LocalGen-bot-simulator --games 8 --bots XiaruizeBot GcBot
```

Windows 请使用 `LocalGen-bot-simulator.exe`。

## 打包提示

- **Windows：** 如果需要便携目录，请在构建后运行 Qt 的 `windeployqt`。
- **macOS：** 使用 `bash scripts/package-macos-dmg.sh build/Release/LocalGen-new.app LocalGen-new.dmg`；上游 README 不建议本项目直接使用 `macdeployqt ... -dmg`。
- **Linux：** 如果打包的 AppImage 在 Debian/Ubuntu 上因缺少 OpenGL 而启动失败，请安装 `libopengl0`。

## 启动后会看到什么

本地对局与地图编辑器已经可用。Web Game 与 Load Replay 当前只会提示“尚不可用”，声音播放也未实现。下一步请阅读[第一局本地游戏与操作]({{< relref "docs/local-game" >}})。
