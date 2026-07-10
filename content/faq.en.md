---
title: "FAQ"
description: "Current, source-backed answers about LocalGen v6 gameplay, maps, bots, networking, replays, and builds."
date: 2026-04-06T17:54:16+08:00
draft: false
weight: 90
---

## Is LocalGen affiliated with generals.io?

No. LocalGen is an independent, fan-made open-source project. It is not affiliated with, endorsed by, sponsored by, or connected to generals.io or its original developers.

## What is the current v6 status?

The active `master` source identifies itself as `6.0.0-dev`. Local Game, Map Creator, and the bot simulator work; some visible menu entries remain placeholders.

## Can I play over a LAN or through Web Game?

Not in current v6. Clicking Web Game displays a “not available yet” message. The README's older LAN wording does not match the current implementation.

## Can I load a replay?

Not yet. `.lgr` and `.lgra` are listed in the associated-files reference, but the current Load Replay button is a placeholder and no replay reader is implemented.

## How many humans can play a Local Game?

At most one. Only the first of 2–16 player slots offers `Human`; every other slot selects a built-in bot. Current local matches are free-for-all rather than team games.

## Can I use my own map?

Yes. Local Game discovers valid `.lgmp` maps from the `maps/` directory beside the executable. Map Creator can open `.lg`, `.lgmp`, and official map JSON, but it saves only `.lg` or `.lgmp`. A selected map must provide enough spawn points or empty plain tiles for all players.

## Does LocalGen require internet access?

Offline gameplay, local map editing, and the simulator do not. Map Creator's optional “Import from Generals.io” action sends a request to the public map API and therefore needs internet access.

## Can I write a bot in Python or run an external bot process?

Not with the current v6 integration. Supported bots are C++17 source files compiled into LocalGen. There is no external-bot network protocol or model runtime in the repository today.

## Are simulator runs reproducible?

Not exactly. The CLI has no seed option and obtains map seeds from system randomness. Record all options and run enough games, but do not describe two invocations as deterministic reproductions.

## Where are v6 settings stored?

The associated-files document names `settings.json`, but the current desktop app does not implement settings persistence. Local Game choices apply to that launch only.

## Where should I report a bug or propose a feature?

- [GitHub Issues](https://github.com/SZXC-WG/LocalGen-new/issues)
- [GitHub Discussions](https://github.com/SZXC-WG/LocalGen-new/discussions)
- [Contribution guide]({{< relref "contribute" >}})
- [Code of Conduct]({{< relref "docs/code-of-conduct" >}})
- [Commit regulations]({{< relref "docs/commit-regulations" >}})
- [Bot contribution workflow]({{< relref "docs/bot-contributions" >}})
