// Copyright (C) 2026 SZXC Work Group
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file smartRandomBot.cpp
 *
 * Legacy `smartRandomBot` from LocalGen v5.
 *
 * @authors AppOfficer; GoodCoder666
 */

#include <algorithm>
#include <deque>
#include <random>
#include <ranges>
#include <tuple>
#include <vector>

#include "core/bot.h"
#include "core/game.hpp"

class SmartRandomBot : public BasicBot {
   private:
    constexpr static Coord directions[4] = {{-1, 0}, {0, -1}, {1, 0}, {0, 1}};
    std::mt19937 rng{std::random_device{}()};
    std::deque<Coord> lastCoords;
    index_t playerId = -1;

    Coord determineMaxArmyPos(const BoardView& boardView) {
        Coord maxArmyPos(-1, -1);
        army_t maxArmy = 0;
        for (pos_t i = 1; i <= boardView.row; ++i) {
            for (pos_t j = 1; j <= boardView.col; ++j) {
                const TileView& tile = boardView.tileAt(i, j);
                if (tile.occupier == playerId && tile.army > maxArmy) {
                    maxArmy = tile.army;
                    maxArmyPos = Coord(i, j);
                }
            }
        }
        return maxArmyPos;
    }

   public:
    void init(index_t playerId, const GameConstantsPack&) override {
        this->playerId = playerId;
    }

    void requestMove(const BoardView& boardView,
                     const std::vector<RankItem>&) override {
        moveQueue.clear();

        // The concept of "focus" does not exist any more in v6.
        // We will refer to `lastCoords.back()`, and try to move from it.
        Coord coo(-1, -1);
        if (!lastCoords.empty()) {
            Coord alt = lastCoords.back();
            const TileView& tile = boardView.tileAt(alt);
            if (tile.occupier == playerId && tile.army > 1) {
                coo = alt;
            }
        }
        if (coo.x == -1) coo = determineMaxArmyPos(boardView);
        if (coo.x == -1 || boardView.tileAt(coo).army <= 1) {
            lastCoords.clear();
            return;
        }

        // Filter valid moves from `coo`
        auto isValidPos = [&](Coord pos) {
            return pos.x >= 1 && pos.x <= boardView.row && pos.y >= 1 &&
                   pos.y <= boardView.col;
        };
        auto candidates =
            directions |
            std::views::transform([&](Coord offset) { return coo + offset; }) |
            std::views::filter([&](Coord next) {
                return isValidPos(next) &&
                       !isImpassableTile(boardView.tileAt(next).type);
            });
        if (candidates.empty()) return;

        // Determine next move by priority
        bool rdret = rng() & 1;
        auto recentCoords = lastCoords | std::views::reverse;
        Coord nextCoo = std::ranges::min(candidates, {}, [&](Coord next) {
            const TileView& tile = boardView.tileAt(next);
            bool own = tile.occupier == playerId;
            return std::tuple{
                recentCoords.begin() - std::ranges::find(recentCoords, next),
                tile.type != TILE_GENERAL || own,
                (tile.occupier == -1) != rdret,
                own,
                own ? ~tile.army : tile.army,
            };
        });

        // Record & execute the move
        lastCoords.push_back(nextCoo);
        if (lastCoords.size() > 100) lastCoords.pop_front();
        moveQueue.emplace_back(MoveType::MOVE_ARMY, coo, nextCoo, false);
    }
};

static BotRegistrar<SmartRandomBot> smartRandomBot_reg("SmartRandomBot");
