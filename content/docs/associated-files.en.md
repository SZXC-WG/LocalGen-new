---
title: "Associated Files"
description: "A status-aware reference for LocalGen map, replay, and settings files in the current v6 source."
date: 2026-04-06T17:55:07+08:00
draft: false
weight: 50
---

> Upstream reference: [`docs/associated-files.md`](https://github.com/SZXC-WG/LocalGen-new/blob/master/docs/associated-files.md). Implementation details below are checked against `src/core/map.hpp` and the current UI.

## Format status

| File | Role | Current v6 status |
| --- | --- | --- |
| `.lgmp` | Native v6 map | Read/write in Map Creator; readable by Local Game and simulator |
| `.lg` | Legacy v5 map | Read/write in Map Creator; metadata is not preserved; not listed directly in Local Game |
| Official `.json` | generals.io map interchange | Read/import in Map Creator; no JSON export; not directly playable |
| `.lgr` | Standard replay | Named by the upstream reference, but no current v6 reader/writer |
| `.lgra` | Advanced replay | Named by the upstream reference, but no current v6 reader/writer |
| `settings.lgsts` | v5 settings | Legacy reference only |
| `settings.json` | v6 settings | Documented filename; current app does not persist settings |

## Native v6 `.lgmp`

`.lgmp` is a binary Qt data-stream format, not JSON. It stores:

- a format magic value;
- title, author, creation datetime, and description;
- map width and height;
- compressed tile records containing tile type, the globally visible “light” bit, and army or spawn-team value.

Map Creator supports dimensions from 1×1 through 100×100. A map used for play must contain enough spawn tiles or empty plain tiles for every participant.

## Legacy `.lg`

The current editor reads and writes the v5 board encoding. It synthesizes a title and creation time when opening a file, but saving `.lg` discards v6 metadata. Use `.lgmp` for new authored maps.

## Official map JSON

The editor accepts an official map object with valid `width`, `height`, `map`, `title`, and `created_at` data, plus optional author/description fields. It can also fetch the same form from the public generals.io map API by title. After import, save a local `.lgmp` copy if you want it available to LocalGen.

## Making a map playable

Place a valid `.lgmp` file in the `maps/` directory beside `LocalGen-new`. The picker reads that directory when the Local Game dialog opens and uses map metadata for its display name.

The replay extensions and settings filenames remain useful project vocabulary, but they should not be described as working v6 features until corresponding readers/writers exist.
