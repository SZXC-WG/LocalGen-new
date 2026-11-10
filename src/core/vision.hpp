// Copyright (C) 2026 SZXC Work Group
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

/**
 * @file vision.hpp
 *
 * LocalGen Module: core
 *
 * Vision System
 *
 * Computes per-player hard and soft visibility masks and projects boards into
 * player-specific BoardViews.
 */

#include <algorithm>
#include <bit>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <utility>
#include <vector>

#include "board.hpp"
#include "modifiers.hpp"
#include "tile.hpp"
#include "utils.hpp"

/// Per-game visibility state machine and BoardView projector.
class Vision {
    // Visibility is stored as a 16-bit player bitmask (`mask_t`) per tile
    // across the padded `(height + 2) x (width + 2)` grid (border padding is
    // 0):
    //   - `vision`:     Hard vision during an update, merged with `softVision`
    //                   and `litTiles` at the end.
    //   - `softVision`: Persistent soft-vision memory under Fading Smog.
    //   - `scratch`:    Temporary plane for hard-vision dilation and
    //                   soft-vision connectivity.
   protected:
    using mask_t = uint16_t;
    static constexpr std::pair<int, int> kDirs[4] = {
        {-1, 0}, {1, 0}, {0, -1}, {0, 1}};

    std::vector<mask_t> vision, scratch;
    std::vector<mask_t> softVision, softReach;
    std::vector<pos_t> softFrontier, litTiles;

    /// Bitmask for `player`, or `0` if neutral (`-1`).
    static constexpr mask_t playerBit(index_t player) noexcept {
        assert(player >= -1 && player < 16);
        return uint32_t(0x8000) >> (15 - player);
    }

    /// Branchless all-ones (`0xFFFF`) or all-zeros (`0x0000`) mask.
    static constexpr mask_t maskIf(bool cond) noexcept {
        return mask_t(0) - mask_t(cond);
    }

    // ---- Hard Vision Helpers ------------------------------------------------

    /// Grant `mask` hard vision within `radius` of `(cx, cy)`.
    /// `diamond`: L1 Manhattan diamond vs L-inf Chebyshev square.
    template <int radius, bool diamond>
    void addRadiusVision(const Board& board, mask_t mask, pos_t cx,
                         pos_t cy) noexcept {
        const pos_t height = board.getHeight(), width = board.getWidth();
        const pos_t stride = board.stride();
        const pos_t xEnd = std::min(cx + radius, height);
        for (pos_t x = std::max(cx - radius, 1); x <= xEnd; ++x) {
            const pos_t span = diamond ? radius - std::abs(x - cx) : radius;
            const pos_t rowBase = x * stride;
            const pos_t tEnd = rowBase + std::min(cy + span, width);
            for (pos_t t = rowBase + std::max(cy - span, 1); t <= tEnd; ++t)
                vision[t] |= mask;
        }
    }

    /// Dilate `scratch` by 1 step in all 8 directions into `vision`.
    void dilateHardVision(const Board& board) noexcept {
        const pos_t height = board.getHeight(), width = board.getWidth();
        const pos_t stride = board.stride();
        const mask_t* __restrict s = scratch.data();
        mask_t* __restrict v = vision.data();
        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1, t = x * stride + 1; y <= width; ++y, ++t) {
                v[t] |= s[t - stride - 1] | s[t - stride] | s[t - stride + 1] |
                        s[t - 1] | s[t] | s[t + 1] | s[t + stride - 1] |
                        s[t + stride] | s[t + stride + 1];
            }
        }
    }

    /// Grant cross-shaped hard vision around `(x, y)` to orthogonal neighbors.
    /// Extend it to 5x5 for the uncontested leader in adjacent army totals.
    void addLookoutVision(const Board& board, pos_t x, pos_t y) noexcept {
        army_t adjacentArmies[16] = {};
        mask_t adjacentMask = 0;
        for (auto [dx, dy] : kDirs) {
            const Tile& nb = board.tileAt(x + dx, y + dy);
            if (nb.occupier < 0) continue;
            adjacentArmies[nb.occupier] += nb.army;
            adjacentMask |= 1 << nb.occupier;
        }
        if (!adjacentMask) return;
        addRadiusVision<1, true>(board, adjacentMask, x, y);

        mask_t leaderMask = 0;
        army_t largest = -2;
        army_t runnerUp = -2;  // -2 < (any positive integer) * 66 / 100 - 1
        while (adjacentMask) {
            const index_t player = std::countr_zero(adjacentMask);
            const mask_t bit = adjacentMask & (~adjacentMask + 1);
            const army_t army = adjacentArmies[player];
            if (army > largest) {
                leaderMask = bit;
                runnerUp = largest;
                largest = army;
            } else if (army > runnerUp) {
                runnerUp = army;
            }
            adjacentMask ^= bit;
        }

        if (runnerUp < largest * 66 / 100 - 1) {
            addRadiusVision<2, false>(board, leaderMask, x, y);
        }
    }

    /// Project a 7-tile hard-vision ray across `(x, y)` away from each
    /// occupied orthogonal neighbor.
    void addObservatoryVision(const Board& board, pos_t x, pos_t y) noexcept {
        const pos_t height = board.getHeight(), width = board.getWidth();
        const pos_t stride = board.stride();
        const pos_t center = x * stride + y;
        for (auto [dx, dy] : kDirs) {
            const index_t player = board.tileAt(x - dx, y - dy).occupier;
            if (player == -1) continue;
            const mask_t mask = 1 << player;
            const pos_t edgeDist = dx ? (dx > 0 ? height - x : x - 1)
                                      : (dy > 0 ? width - y : y - 1);
            const pos_t range = std::min<pos_t>(7, edgeDist);
            const pos_t stepOffset = dx * stride + dy;
            for (pos_t k = 0, t = center; k <= range; ++k, t += stepOffset) {
                vision[t] |= mask;
            }
        }
    }

    /// Recompute hard vision specialized for the active vision modifiers.
    template <bool watchtower, bool mistyVeil>
    void updateHardVision(const Board& board) {
        const pos_t height = board.getHeight(), width = board.getWidth();
        const pos_t stride = board.stride();
        const Tile* map = board.tiles().data();
        litTiles.clear();
        std::fill(vision.begin(), vision.end(), 0);

        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1, t = x * stride + 1; y <= width; ++y, ++t) {
                const Tile& tile = map[t];
                const mask_t owner = playerBit(tile.occupier);

                scratch[t] = owner;
                if constexpr (mistyVeil) vision[t] |= owner;

                if (tile.lit) litTiles.push_back(t);

                if (tile.type == TILE_LOOKOUT && tile.occupier == -1) {
                    addLookoutVision(board, x, y);
                } else if (tile.type == TILE_OBSERVATORY &&
                           tile.occupier == -1) {
                    addObservatoryVision(board, x, y);
                } else if constexpr (watchtower) {
                    if (!isCityLikeTile(tile.type)) continue;
                    if (owner) addRadiusVision<4, true>(board, owner, x, y);
                    // Under Misty Veil, reveal cities to adjacent owners
                    // (border padding has occupier == -1, which maps to 0).
                    if constexpr (mistyVeil) {
                        vision[t] |= playerBit(map[t - stride].occupier) |
                                     playerBit(map[t + stride].occupier) |
                                     playerBit(map[t - 1].occupier) |
                                     playerBit(map[t + 1].occupier);
                    }
                }
            }
        }

        if constexpr (!mistyVeil) dilateHardVision(board);
    }

    // ---- Soft Vision (Fading Smog) Helpers ----------------------------------

    /// Bitwise OR of the 4 orthogonal neighbors of `t` in `plane`.
    static constexpr mask_t orthoMask(const std::vector<mask_t>& plane, pos_t t,
                                      pos_t stride) noexcept {
        return plane[t - stride] | plane[t + stride] | plane[t - 1] |
               plane[t + 1];
    }

    /// Hard vision on `t` suppresses any other player's soft vision on `t`.
    mask_t knownVision(pos_t t) const noexcept {
        return vision[t] ? vision[t] : softVision[t];
    }

    /// Prune soft vision to 4-connected components rooted at owned territory
    /// (blocked by enemy territory), optionally expanding by 1 orthogonal step.
    void updateSoftVision(const Board& board, bool expand) {
        const pos_t height = board.getHeight(), width = board.getWidth();
        const pos_t stride = board.stride();
        const pos_t end = static_cast<pos_t>(board.tiles().size()) - stride;
        mask_t* pass = scratch.data();
        mask_t* reach = softReach.data();
        softFrontier.clear();

        // 1. Seed owned tiles (`pass[t]` holds `owner` from updateHardVision)
        //    and sweep forward (top-left to bottom-right).
        for (pos_t t = stride + 1; t < end - 1; ++t) {
            if (const mask_t owner = pass[t]) {
                pass[t] = 0;
                reach[t] = owner;
                continue;
            }
            const mask_t known = knownVision(t);
            pass[t] = known;
            reach[t] = known ? known & (reach[t - stride] | reach[t - 1]) : 0;
        }

        // 2. Sweep backward (bottom-right to top-left) and queue reverse turns.
        for (pos_t t = end - 2; t > stride; --t) {
            const mask_t known = pass[t];
            if (!known) continue;
            const pos_t below = t + stride, right = t + 1;
            const mask_t added =
                known & (reach[below] | reach[right]) & ~reach[t];
            if (!added) continue;
            reach[t] |= added;

            if (added & ((pass[below] & ~reach[below]) |
                         (pass[right] & ~reach[right]))) {
                softFrontier.push_back(t);
            }
        }

        // 3. Flood-fill any remaining winding paths.
        while (!softFrontier.empty()) {
            const pos_t t = softFrontier.back();
            softFrontier.pop_back();
            const mask_t curReach = reach[t];
            for (pos_t nb : {t - stride, t + stride, t - 1, t + 1}) {
                const mask_t added = curReach & pass[nb] & ~reach[nb];
                if (added) {
                    reach[nb] |= added;
                    softFrontier.push_back(nb);
                }
            }
        }

        // 4. Every 25 half-turns, expand connected soft vision by 1 step.
        if (expand) {
            const Tile* map = board.tiles().data();
            for (pos_t t = stride; t < end; ++t) {
                pass[t] = reach[t] & softVision[t] &
                          maskIf(!isImpassableTile(map[t].type));
            }
            for (pos_t x = 1; x <= height; ++x) {
                for (pos_t y = 1, t = x * stride + 1; y <= width; ++y, ++t) {
                    softVision[t] |= orthoMask(scratch, t, stride);
                }
            }
        }

        // 5. Commit pruned soft vision and merge into final vision.
        for (pos_t t = stride; t < end; ++t) {
            const mask_t next =
                knownVision(t) & orthoMask(softReach, t, stride);
            softVision[t] = next;
            vision[t] |= next;
        }
    }

   public:
    /// Recompute player visibility after a board update.
    void update(const Board& board, const Modifiers& modifiers,
                index_t playerCount, turn_t halfTurn = 0) {
        assert(isValidModifiers(modifiers));
        assert(0 < playerCount && playerCount <= 16);
        const mask_t allPlayers = (uint32_t(1) << playerCount) - 1;
        const pos_t height = board.getHeight(), width = board.getWidth();
        const pos_t stride = board.stride();
        const std::size_t totalSize = board.tiles().size();

        if (vision.size() != totalSize) {
            vision.assign(totalSize, 0);
            scratch.assign(totalSize, 0);
            softVision.clear();
        }

        if (modifiers.crystalClear) {
            for (pos_t x = 1; x <= height; ++x)
                std::fill_n(vision.begin() + x * stride + 1, width, allPlayers);
            softVision.clear();
            return;
        }

        if (modifiers.watchtower)
            modifiers.mistyVeil ? updateHardVision<true, true>(board)
                                : updateHardVision<true, false>(board);
        else
            modifiers.mistyVeil ? updateHardVision<false, true>(board)
                                : updateHardVision<false, false>(board);

        if (!modifiers.fadingSmog) {
            softVision.clear();
        } else if (softVision.empty() || halfTurn == 0) {
            softReach.assign(totalSize, 0);
            softVision = vision;
        } else {
            updateSoftVision(board, halfTurn % 25 == 0);
        }

        for (pos_t t : litTiles) vision[t] = allPlayers;
    }

    /// Tile visibility check for a player.
    constexpr bool visible(const Board& board, pos_t x, pos_t y,
                           index_t player) const noexcept {
        if (board.isInvalidPos(x, y)) return false;
        const pos_t t = x * board.stride() + y;
        return vision[t] >> player & 1;
    }

    /// Same as above, but using %Coord.
    constexpr bool visible(const Board& board, const Coord& pos,
                           index_t player) const noexcept {
        return visible(board, pos.x, pos.y, player);
    }

    /// Populate a player's view of `board` into `boardView`.
    constexpr void view(const Board& board, index_t player,
                        BoardView& boardView) const {
        const pos_t height = board.getHeight(), width = board.getWidth();
        if (boardView.getHeight() != height || boardView.getWidth() != width) {
            boardView = BoardView(height, width);
        }
        const pos_t stride = board.stride();
        const mask_t mask = playerBit(player);
        const auto srcTiles = board.tiles();
        const auto dstTiles = boardView.tiles();
        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1, t = x * stride + 1; y <= width; ++y, ++t)
                dstTiles[t].updateFrom(srcTiles[t], vision[t] & mask);
        }
    }

    /// Return a player's view of `board`.
    constexpr BoardView view(const Board& board, index_t player) const {
        BoardView boardView(board.getHeight(), board.getWidth());
        view(board, player, boardView);
        return boardView;
    }

    /// Return a single tile's view for `player`.
    constexpr TileView view(const Board& board, index_t player, pos_t x,
                            pos_t y) const noexcept {
        return {board.tileAt(x, y), visible(board, x, y, player)};
    }

    /// Same as above, but using %Coord.
    constexpr TileView view(const Board& board, index_t player,
                            Coord pos) const noexcept {
        return {board.tileAt(pos), visible(board, pos, player)};
    }

    /// Return a full (unobstructed) view of `board`.
    static constexpr BoardView fullView(const Board& board) {
        const pos_t height = board.getHeight(), width = board.getWidth();
        BoardView boardView(height, width);
        const pos_t stride = board.stride();
        const auto srcTiles = board.tiles();
        const auto dstTiles = boardView.tiles();
        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1, t = x * stride + 1; y <= width; ++y, ++t)
                dstTiles[t].updateFrom(srcTiles[t], true);
        }
        return boardView;
    }
};
