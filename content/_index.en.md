---
title: "Local Generals.io"
description: "A source-backed guide to LocalGen's offline game, map editor, built-in bots, simulator, releases, and contributor docs."
date: 2026-04-06T17:54:16+08:00
draft: false
---

**Local Generals.io (LocalGen)** is an unofficial, open-source strategy game inspired by generals.io. The current `master` branch is the Qt 6 rewrite, identified in source as **`6.0.0-dev`**.

## What works in the current v6 source

- **Local Game** — run an offline free-for-all with one optional human and built-in C++ bots, on a generated board or an installed `.lgmp` map.
- **Map Creator** — build and edit v5/v6 maps, attach metadata, open official map JSON, or optionally import a public map by title.
- **Bot Simulator** — evaluate two or more registered bots over repeated, parallel matches and compare win rate, TrueSkill, rank, kills, army, land, and optional latency.
- **In-game tools** — use a move queue, event/chat history, a collapsible leaderboard, and an optional Army/Land analysis chart.

## Current development boundaries

The main menu also shows **Web Game** and **Load Replay**, but both are placeholders in the current source. LAN/web multiplayer, replay playback, external bot processes, persistent v6 settings, and sound playback are not implemented yet. Local gameplay itself is offline; only the Map Creator's optional generals.io map import needs a network connection.

## Choose a path

- [Download a published build]({{< relref "downloads" >}})
- [Build v6 from source]({{< relref "docs/getting-started" >}})
- [Start your first local game]({{< relref "docs/local-game" >}})
- [Create or convert a map]({{< relref "docs/map-creator" >}})
- [Explore the built-in bots]({{< relref "bots" >}})
- [Run bot evaluations]({{< relref "simulator" >}})

LocalGen is independent of generals.io and is not endorsed or sponsored by its owners. See the [disclaimer]({{< relref "disclaimer" >}}) for the project boundary.
