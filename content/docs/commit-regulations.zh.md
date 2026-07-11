---
title: "提交规范"
description: "整理 LocalGen 开发中对提交信息、提交大小和协作方式的要求。"
date: 2026-04-06T17:55:07+08:00
draft: false
weight: 30
---

你也可以在 [GitHub 上阅读提交规范](https://github.com/SZXC-WG/LocalGen-new/blob/master/docs/commit-regulations.md)。

## 目标

提交规范的目标，是让项目历史保持一致、清晰并且易于维护。

## 提交信息结构

建议使用以下提交信息结构：

```text
<type>(<scope>): <subject>

<body>

<footer>
```

### 常见类型

- `feat` —— 新功能
- `upd` —— 更新已有功能
- `fix` —— 修复 Bug
- `docs` —— 文档修改
- `style` —— 仅样式 / 格式调整
- `refactor` —— 不改变功能的重构
- `chore` —— 维护、依赖、构建类修改
- `test` —— 测试新增或调整
- `ci` —— 持续集成相关修改

## Subject 的建议

- 使用命令式动词
- 简洁但要足够明确
- 避免 “Fix stuff” 这类模糊描述
- 除专有名词外尽量使用小写

## 提交大小与频率

项目偏好的提交应当具备以下特征：

- **小而聚焦**
- **原子性强**，便于单独审查或回滚
- **足够频繁** 展示进展，但不要因为过多碎片提交制造噪音

## 其他建议

- 始终编写有意义的提交信息
- 在合并前按需要进行 rebase
- 对过度碎片化的提交进行 squash，再进入主分支

