---
title: "FAQ"
description: "Answers about LocalGen v6 gameplay, maps, bots, networking, replays, and builds."
date: 2026-04-06T17:54:16+08:00
draft: false
weight: 90
---

## Is LocalGen affiliated with generals.io?

No. LocalGen is an independent, fan-made open-source project. It is not affiliated with, endorsed by, sponsored by, or connected to generals.io or its original developers.

## Which v6 features are available?

Local Game, Map Creator, and the Bot simulator are available in `6.0.0-dev`. Web Game and Load Replay are still in development.

## Can I play over a LAN or through Web Game?

No. Web Game is not available yet, and LocalGen does not currently support LAN play.

## Can I load a replay?

Not yet. The visible Load Replay button does not currently open `.lgr` or `.lgra` files.

## How many humans can play a Local Game?

At most one. Only the first of 2–16 player slots offers `Human`; every other slot selects a built-in bot. Current local matches are free-for-all rather than team games.

## Can I use my own map?

Yes. Local Game discovers valid `.lgmp` maps from the `maps/` directory beside the executable. Map Creator can open `.lg`, `.lgmp`, and official map JSON, but it saves only `.lg` or `.lgmp`. A selected map must provide enough spawn points or empty plain tiles for all players.

## Does LocalGen require internet access?

Offline gameplay, local map editing, and the simulator do not. Map Creator's optional “Import from Generals.io” action sends a request to the public map API and therefore needs internet access.

## Can I write a bot in Python or run an external bot process?

No. LocalGen currently supports built-in C++17 Bots only; Python clients and external Bot processes are not supported.

## Are simulator runs reproducible?

Not exactly. The CLI has no seed option and obtains map seeds from system randomness. Record all options and run enough games, but do not describe two invocations as deterministic reproductions.

## Where are v6 settings stored?

Settings are not saved between launches yet. Local Game choices apply until you close the app.

## Where should I report a bug or propose a feature?

- [GitHub Issues](https://github.com/SZXC-WG/LocalGen-new/issues)
- [GitHub Discussions](https://github.com/SZXC-WG/LocalGen-new/discussions)
- [Contribution guide]({{< relref "contribute" >}})
- [Code of Conduct]({{< relref "docs/code-of-conduct" >}})
- [Commit regulations]({{< relref "docs/commit-regulations" >}})
- [Bot contribution workflow]({{< relref "docs/bot-contributions" >}})
