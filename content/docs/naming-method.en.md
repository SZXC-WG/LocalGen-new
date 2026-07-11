---
title: "Naming Method"
description: "A translated summary of the LocalGen v6 C++ naming conventions."
date: 2026-04-06T17:55:07+08:00
draft: false
weight: 40
---

You can also [read the naming guide on GitHub](https://github.com/SZXC-WG/LocalGen-new/blob/master/docs/naming-method.md).

## Naming conventions used in LocalGen v6

Use the following naming conventions in LocalGen v6:

1. **Namespaces** use lowercase letters separated by underscores.
2. **Classes, structs, and most types** use PascalCase.
3. **Enum types** use lowercase with underscores and often end in `_e`; enum constants use uppercase with underscores.
4. **Functions and variables** generally use camelCase.
5. **Constants** use uppercase letters separated by underscores.
6. **Global macros** use uppercase with underscores.
7. **Local variable names** should remain descriptive; avoid meaningless single-letter names when possible.

This guide is especially useful when contributing bots or UI/core features in the Qt-based v6 branch.

