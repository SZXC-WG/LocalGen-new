// Copyright (C) 2026 SZXC Work Group
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file ktqBot.cpp
 *
 * Legacy `ktqBot` from LocalGen v5.
 *
 * @authors ktq1124298818; GoodCoder666
 */

#include <limits>
#include <vector>

#include "core/bot.h"
#include "core/game.hpp"

class KtqBot : public BasicBot {
   private:
    constexpr static Coord dir[4] = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}};
    index_t playerId = -1;
    Coord focus{-1, -1};

    inline Coord determineMaxArmyPos(const BoardView& boardView) {
        Coord maxArmyPos(-1, -1);
        army_t maxArmy = 1;  // Find a tile with >1 army
        boardView.forEachTile([&](Coord pos, const TileView& tile) noexcept {
            if (tile.occupier == playerId && tile.army > maxArmy) {
                maxArmy = tile.army;
                maxArmyPos = pos;
            }
        });
        return maxArmyPos;
    }

   public:
    void init(index_t playerId, const GameConstantsPack&) override {
        this->playerId = playerId;
    }

    void requestMove(const BoardView& boardView,
                     const std::vector<RankItem>&) override {
        moveQueue.clear();
        if (focus.x == -1 || boardView.tileAt(focus).occupier != playerId ||
            boardView.tileAt(focus).army <= 1) {
            focus = determineMaxArmyPos(boardView);
            if (focus.x == -1) {
                moveQueue.emplace_back(MoveType::EMPTY);
                return;
            }
        }
        const army_t armyInHand = boardView.tileAt(focus).army;

        using weight_t = std::pair<army_t, army_t>;
        Coord bestTarget(-1, -1);
        weight_t bestWeight{std::numeric_limits<army_t>::max(), 0};

        for (const Coord& delta : dir) {
            Coord target = focus + delta;
            if (boardView.isInvalidPos(target)) continue;
            const TileView& tile = boardView.tileAt(target);
            const tile_type_e type =
                tile.type == TILE_DESERT ? TILE_BLANK : tile.type;
            const index_t occupier = tile.occupier;
            if (isImpassableTile(type)) continue;

            weight_t weight{tile.army, tile.army};
            if (type != TILE_SWAMP && occupier == playerId)
                weight = {-tile.army, -tile.army};
            else if (type == TILE_CITY && occupier != playerId)
                weight.first = 2 * tile.army - 1'000'000'000'000'000LL;
            else if (type == TILE_BLANK && occupier != playerId)
                weight.first = tile.army - 1'000'000'000'000'000LL;
            else if (type == TILE_SWAMP)
                weight = {-10'000'000'000'000'000LL, 200LL};
            else if (type == TILE_GENERAL && occupier != playerId)
                weight.first = -1'000'000'000'000'000'000LL;

            if (weight.second < armyInHand && weight < bestWeight) {
                bestWeight = weight;
                bestTarget = target;
            }
        }

        if (bestTarget.x == -1) {
            moveQueue.emplace_back(MoveType::EMPTY);
        } else {
            moveQueue.emplace_back(MoveType::MOVE_ARMY, focus, bestTarget,
                                   false);
            focus = bestTarget;
        }
    }
};

static BotRegistrar<KtqBot> ktqBot_reg("KtqBot");
