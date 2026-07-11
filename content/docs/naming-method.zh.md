---
title: "命名规范"
description: "整理 LocalGen v6 中 C++ 代码命名方式的规则。"
date: 2026-04-06T17:55:07+08:00
draft: false
weight: 40
---

你也可以在 [GitHub 上阅读命名规范](https://github.com/SZXC-WG/LocalGen-new/blob/master/docs/naming-method.md)。

## LocalGen v6 的命名约定

LocalGen v6 建议使用以下命名约定：

1. **命名空间** 使用全小写，并用下划线分隔。
2. **类、结构体与大多数类型名** 使用大驼峰命名法（PascalCase）。
3. **枚举类型** 使用全小写加下划线，通常以 `_e` 结尾；枚举值使用全大写加下划线。
4. **函数和变量** 一般使用小驼峰命名法（camelCase）。
5. **常量** 使用全大写加下划线。
6. **全局宏** 使用全大写加下划线。
7. **局部变量** 应尽量具备描述性，避免无意义的单字母命名。

如果你准备为 v6 分支贡献 Bot、UI 或核心逻辑，这份规范非常值得先看一遍。

