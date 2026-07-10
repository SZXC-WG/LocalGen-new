---
title: "First Local Game & Controls"
description: "Configure an offline LocalGen match and use the complete keyboard, mouse, queue, chat, and camera controls."
date: 2026-07-10T00:00:00+08:00
draft: false
weight: 6
---

## Configure the match

Choose **Local Game** from the main menu, then set:

- **Game Speed:** 1–1000. One half-turn is scheduled every `500 / speed` milliseconds, subject to bot processing time. Speed 1 is approximately one full turn per second.
- **Enable sounds:** currently disabled; no sound playback is wired into Local Game.
- **Show analysis:** adds a live Army/Land chart with linear/log toggles.
- **Game Map:** `Standard` generates a new random map. Other entries are valid `.lgmp` files found in the executable's `maps/` directory.
- **Map size:** Standard maps allow width and height from 1–100, defaulting to 20×20. Authored maps lock these fields to their stored dimensions.
- **Players:** 2–16. Only P1 offers `Human`; every slot can select a compiled bot, so you may also watch an all-bot game.

The current UI supports at most one human. Every participant gets a unique team ID, so the match is free-for-all.

## Select and move

Click a tile to focus it. Once a tile is focused:

| Control | Action |
| --- | --- |
| Arrow keys or `W A S D` | Move focus and queue a full-army move |
| `I J K L` | Move focus without queuing a move |
| Shift + arrow/WASD | Move focus without queuing a move |
| `Z` | Mark the next queued move to take half the army |
| `Q` | Clear the entire move queue |
| `E` | Remove the last queued move and return focus to its source |
| Space | Clear tile focus |
| `H` | Focus your general |
| `G` | Focus and center the camera on your general |
| Escape | Open surrender confirmation |

The game ignores queued moves that are no longer legal when their turn arrives. Mountains, lookouts, and observatories are impassable.

## Camera and chat

| Control | Action |
| --- | --- |
| Left-drag | Pan the board |
| Mouse wheel | Zoom around the pointer |
| Touchpad scroll | Pan |
| Pinch gesture | Zoom |
| `9` / `0` | Zoom out / in |
| `C` | Fit and center the whole map |
| Enter | Focus chat input; Enter again sends non-empty text |

Chat input is visible only when a human is present. The panel also records system notices, captures, surrenders, and the winner, with turn/half-turn markers.

## Read the overlays

- **Turn badge:** a trailing dot distinguishes one half-turn phase.
- **Leaderboard:** shows player, Army, and Land. Click it to collapse or expand player names; dead players are shaded and a skull marks players with kills.
- **Analysis:** when enabled, switch between Army/Land and linear/log views.

After the human is eliminated or the match ends, the full board is revealed and move input stops.

## Map startup errors

An authored map must be valid and non-empty. It also needs enough spawn tiles or zero-army plain tiles for every selected participant. If it fails either check, return to setup and choose fewer players or edit the map.
