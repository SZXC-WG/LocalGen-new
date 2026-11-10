// Copyright (C) 2026 SZXC Work Group
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

/**
 * @file board.hpp
 *
 * LocalGen Module: core
 *
 * Map Boards
 *
 * Generic 1-indexed padded 2D tile grid container for fixed-size game boards
 * and board views.
 */

#include <cassert>
#include <concepts>
#include <functional>
#include <span>
#include <type_traits>
#include <vector>

#include "tile.hpp"
#include "utils.hpp"

/// Fixed-size 1-indexed 2D tile grid with a 1-tile dummy border around the
/// perimeter.
template <typename TileT>
class BasicBoard {
   protected:
    pos_t height_ = 0, width_ = 0;
    std::vector<TileT> tiles_;

   public:
    constexpr BasicBoard() = default;
    constexpr BasicBoard(pos_t height, pos_t width)
        : height_(height), width_(width) {
        assert(height_ >= 0 && width_ >= 0);
        // 1-indexed, dummy padding around
        tiles_.resize((height_ + 2) * (width_ + 2));
    }

    constexpr pos_t getWidth() const noexcept { return width_; }
    constexpr pos_t getHeight() const noexcept { return height_; }
    constexpr pos_t stride() const noexcept { return width_ + 2; }

    /// Direct span over the underlying `(height + 2) * (width + 2)` buffer
    /// (including border padding).
    constexpr std::span<TileT> tiles() noexcept { return tiles_; }
    constexpr std::span<const TileT> tiles() const noexcept { return tiles_; }

    /// Invoke `fn(tile)` on each valid tile in `[1..height] x [1..width]`.
    template <typename Fn>
        requires std::invocable<Fn&, TileT&>
    constexpr void forEachTile(Fn&& fn) noexcept(
        std::is_nothrow_invocable_v<Fn&, TileT&>) {
        const pos_t s = stride();
        for (pos_t x = 1; x <= height_; ++x) {
            TileT* row = tiles_.data() + x * s;
            for (pos_t y = 1; y <= width_; ++y) {
                std::invoke(fn, row[y]);
            }
        }
    }

    /// Invoke `fn(tile)` on each valid tile in `[1..height] x [1..width]`.
    template <typename Fn>
        requires std::invocable<Fn&, const TileT&>
    constexpr void forEachTile(Fn&& fn) const
        noexcept(std::is_nothrow_invocable_v<Fn&, const TileT&>) {
        const pos_t s = stride();
        for (pos_t x = 1; x <= height_; ++x) {
            const TileT* row = tiles_.data() + x * s;
            for (pos_t y = 1; y <= width_; ++y) {
                std::invoke(fn, row[y]);
            }
        }
    }

    /// Invoke `fn(coord, tile)` on each valid tile in
    /// `[1..height] x [1..width]`.
    template <typename Fn>
        requires std::invocable<Fn&, Coord, TileT&>
    constexpr void forEachTile(Fn&& fn) noexcept(
        std::is_nothrow_invocable_v<Fn&, Coord, TileT&>) {
        const pos_t s = stride();
        for (pos_t x = 1; x <= height_; ++x) {
            TileT* row = tiles_.data() + x * s;
            for (pos_t y = 1; y <= width_; ++y) {
                std::invoke(fn, Coord{x, y}, row[y]);
            }
        }
    }

    /// Invoke `fn(coord, tile)` on each valid tile in
    /// `[1..height] x [1..width]`.
    template <typename Fn>
        requires std::invocable<Fn&, Coord, const TileT&>
    constexpr void forEachTile(Fn&& fn) const
        noexcept(std::is_nothrow_invocable_v<Fn&, Coord, const TileT&>) {
        const pos_t s = stride();
        for (pos_t x = 1; x <= height_; ++x) {
            const TileT* row = tiles_.data() + x * s;
            for (pos_t y = 1; y <= width_; ++y) {
                std::invoke(fn, Coord{x, y}, row[y]);
            }
        }
    }

    constexpr bool isValidPos(pos_t x, pos_t y) const noexcept {
        return x >= 1 && x <= height_ && y >= 1 && y <= width_;
    }
    constexpr bool isValidPos(Coord pos) const noexcept {
        return isValidPos(pos.x, pos.y);
    }
    constexpr bool isInvalidPos(pos_t x, pos_t y) const noexcept {
        return !isValidPos(x, y);
    }
    constexpr bool isInvalidPos(Coord pos) const noexcept {
        return !isValidPos(pos);
    }

    constexpr TileT& tileAt(pos_t x, pos_t y) noexcept {
        return tiles_[x * (width_ + 2) + y];
    }
    constexpr const TileT& tileAt(pos_t x, pos_t y) const noexcept {
        return tiles_[x * (width_ + 2) + y];
    }
    constexpr TileT& tileAt(Coord pos) noexcept { return tileAt(pos.x, pos.y); }
    constexpr const TileT& tileAt(Coord pos) const noexcept {
        return tileAt(pos.x, pos.y);
    }
};

/// Game map board (dimensions locked after construction).
using Board = BasicBoard<Tile>;

/// A player's view of the board (dimensions locked after construction).
using BoardView = BasicBoard<TileView>;
