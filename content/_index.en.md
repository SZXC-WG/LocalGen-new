---
title: "Local Generals.io"
description: "A practical guide to playing LocalGen, creating maps, exploring bots, running simulations, and contributing."
date: 2026-04-06T17:54:16+08:00
draft: false
---

**Local Generals.io (LocalGen)** is an unofficial, open-source strategy game inspired by generals.io. Version 6 is a Qt 6-based work in progress, currently labeled **`6.0.0-dev`**.

## What you can do with LocalGen

- **Local Game** — run an offline free-for-all with one optional human and built-in C++ bots, on a generated board or an installed `.lgmp` map.
- **Map Creator** — build and edit v5/v6 maps, attach metadata, open official map JSON, or optionally import a public map by title.
- **Bot Simulator** — evaluate two or more registered bots over repeated, parallel matches and compare win rate, TrueSkill, rank, kills, army, land, and optional latency.
- **In-game tools** — use a move queue, event/chat history, a collapsible leaderboard, and an optional Army/Land analysis chart.

## Features still in development

**Web Game** and **Load Replay** are visible in the menu but are not available yet. LAN/web multiplayer, replay playback, external bots, saved settings, and sound are also still in development. Local Game works offline; only the optional public-map import needs an internet connection.

## Choose a path

- [Download a published build]({{< relref "downloads" >}})
- [Build v6 from source]({{< relref "docs/getting-started" >}})
- [Start your first local game]({{< relref "docs/local-game" >}})
- [Create or convert a map]({{< relref "docs/map-creator" >}})
- [Explore the built-in bots]({{< relref "bots" >}})
- [Run bot evaluations]({{< relref "simulator" >}})

LocalGen is independent of generals.io and is not endorsed or sponsored by its owners. See the [disclaimer]({{< relref "disclaimer" >}}) for the project boundary.
