---
title: "Map Creator"
description: "Create, inspect, import, convert, and install maps with the LocalGen v6 editor."
date: 2026-07-10T00:00:00+08:00
draft: false
weight: 7
---

## Start a map

Choose **Create Map** from the main menu. The editor starts with a 10×10 board; the width and height sliders support 1–100 and preserve the overlapping portion when resized.

Use a normal click to paint one tile. Right-drag paints continuously, while left-drag pans. The mouse wheel zooms, and `C` fits the board. Up/Down selects the previous or next toolbar tool.

## Tile tools

| Tool | Behavior |
| --- | --- |
| Mountain | Impassable terrain |
| Lookout | Impassable; an adjacent occupying army can reveal a 5×5 area |
| Observatory | Impassable; projects line-of-sight outward from adjacent armies, up to eight tiles |
| Desert | Captured desert does not receive the normal 25-turn global army bonus |
| Swamp | Drains one army per turn while occupied; ownership is lost at zero |
| Spawn | General start; optionally label it blank/flexible or team `A`–`Z` |
| City | Set an initial strength from -9999 to 9999; editor default is 40 |
| Neutral | Place neutral army from -9999 to 9999; zero becomes a blank tile |
| Light | Toggle global visibility on any tile |
| Erase | Restore a blank, unlit tile |

Current Local Game assigns every player a distinct team. Spawn labels still influence which group of fixed spawns maps to which randomly assigned team, but the setup UI does not create allied teams.

## Metadata

The collapsible sidebar edits:

- title;
- author;
- creation date and time;
- plain-text description.

Local Game uses a trimmed `.lgmp` title in its map picker. If a title is empty it falls back to the filename; duplicate titles are annotated with filenames.

## Open, import, and save

| Action | Supported input/output |
| --- | --- |
| Open | `.lgmp`, legacy `.lg`, official map `.json` |
| Import | Public generals.io map by exact title; requires internet |
| Save | `.lgmp` or `.lg` only |

Saving `.lg` shows a warning because that format cannot store title, author, creation time, or description. Official JSON is import-only; save it as `.lgmp` after conversion.

The JSON importer validates dimensions, tile count, title, and ISO-8601 `created_at`. Unknown official tile codes are marked as errors so they can be edited before saving.

## Install the finished map

Save the map as `.lgmp`, then copy it into `maps/` beside `LocalGen-new`. Reopen the Local Game dialog to refresh the picker. Ensure the map has enough explicit spawns or zero-army blank tiles for the intended player count.

For format status and replay/settings caveats, see [Associated Files]({{< relref "docs/associated-files" >}}).
