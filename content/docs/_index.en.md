---
title: "Documentation"
description: "Practical, source-backed LocalGen v6 guides for building, playing, creating maps, evaluating bots, and contributing."
date: 2026-04-06T17:54:17+08:00
draft: false
weight: 50
---

These pages combine upstream documentation with checks against the current `master` implementation. Where prose and code disagree, the guides call out the current behavior instead of presenting planned features as finished.

## Start here

- [Getting Started]({{< relref "docs/getting-started" >}}) — prerequisites, build targets, run paths, and packaging notes
- [First Local Game & Controls]({{< relref "docs/local-game" >}}) — setup choices, keyboard/mouse controls, and current limits
- [Map Creator]({{< relref "docs/map-creator" >}}) — tile tools, metadata, import/export formats, and installed maps

## Bots and evaluation

- [Bot Contributions]({{< relref "docs/bot-contributions" >}}) — the current built-in-only workflow
- [Built-in Bots]({{< relref "docs/built-in-bots" >}}) — roster, API contract, registration, and CMake integration
- [Simulator Guide]({{< relref "docs/simulator-guide" >}}) — every CLI option, defaults, output semantics, and limitations

## Reference and community

- [Associated Files]({{< relref "docs/associated-files" >}}) — implemented map formats versus documented future/legacy files
- [Naming Method]({{< relref "docs/naming-method" >}})
- [Commit Regulations]({{< relref "docs/commit-regulations" >}})
- [Code of Conduct]({{< relref "docs/code-of-conduct" >}})

Current v6 is `6.0.0-dev`. Web/LAN play, replay loading, external bot processes, persistent settings, and sound playback are not implemented in the checked source.
