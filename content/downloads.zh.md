---
title: "下载"
description: "下载适合你平台的 LocalGen，或自己构建 Qt 桌面应用。"
date: 2026-04-06T17:54:16+08:00
draft: false
weight: 20
---

## 已发布构建

前往 GitHub Releases 查看最新的 LocalGen 版本说明与下载文件：

- [打开 LocalGen GitHub Releases](https://github.com/SZXC-WG/LocalGen-new/releases)
- [浏览本站整理的版本时间线]({{< relref "releases" >}})

想直接游玩时，请选择标为稳定版且适合你平台的附件。`6.0.0-dev` 是持续开发版本，不等同于稳定安装包。

## 构建目标

| 平台 | 当前打包目标 |
| --- | --- |
| Linux | x86_64 与 ARM64 AppImage |
| macOS | Intel 与 Apple 芯片 DMG |
| Windows | x86_64 与 ARM64 MSVC ZIP；x86_64 MinGW 与 LLVM-MinGW ZIP |

并非每个发布版本都会提供全部目标，因此下载前请检查文件列表。如果 Debian 或 Ubuntu 上的 Linux AppImage 提示缺少 OpenGL 运行库，请安装 `libopengl0`。

## 解压或安装之后

请保留桌面可执行文件旁的 `maps/` 与 `fonts/` 目录。本地对局只会从该 `maps/` 目录发现 `.lgmp` 文件，应用启动时也会读取随附的 Quicksand 字体。

使用 v6 可以开始离线本地对局或打开地图编辑器。即使主菜单上已经出现 Web Game 与回放入口，这两项功能目前仍不可用。

## 构建开发版本

从源码构建需要 Qt 6.7+、CMake 3.19+、Ninja 1.10+ 与支持 C++17 的编译器。请参阅[快速开始]({{< relref "docs/getting-started" >}})中的配置、构建、运行与打包命令。

下载前如需确认当前功能，请阅读[常见问题]({{< relref "faq" >}})。
