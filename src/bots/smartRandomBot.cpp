/**
 * @file SmartRandomBot.cpp
 *
 * Legacy `smartRandomBot` from LocalGen v5.
 *
 * @copyright Copyright (c) SZXC Work Group.
 */

#ifndef LGEN_BOTS_SMARTRANDOMBOT
#define LGEN_BOTS_SMARTRANDOMBOT

#include <algorithm>
#include <deque>
#include <random>
#include <vector>

#include "../GameEngine/bot.h"
#include "../GameEngine/game.hpp"

class SmartRandomBot : public BasicBot {
   private:
    constexpr static pos_t dx[5] = {0, -1, 0, 1, 0};
    constexpr static pos_t dy[5] = {0, 0, -1, 0, 1};
    std::mt19937 rng{std::random_device{}()};
    std::deque<Coord> lastCoords;
    index_t playerId = -1;

    inline Coord determineMaxArmyPos(const BoardView& boardView) {
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
    void init(index_t playerId,
              const game::GameConstantsPack& constants) override {
        this->playerId = playerId;
    }

    void requestMove(const BoardView& boardView,
                     const std::vector<game::RankItem>& rank) override {
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
        struct node {
            tile_type_e type;
            index_t team;
            army_t army;
            int dir;
            std::ptrdiff_t lastCount;
        };
        std::vector<node> p;
        int pl = 0;
        for (int i = 1; i <= 4; ++i) {
            int nx = coo.x + dx[i], ny = coo.y + dy[i];
            if (nx < 1 || nx > boardView.row || ny < 1 || ny > boardView.col)
                continue;
            const TileView& tile = boardView.tileAt(nx, ny);
            if (isImpassableTile(tile.type)) continue;
            p.push_back(node{tile.type, tile.occupier, tile.army, i,
                             std::find(lastCoords.rbegin(), lastCoords.rend(),
                                       Coord(nx, ny)) -
                                 lastCoords.rbegin()});
        }
        if (p.empty()) return;
        bool rdret = rng() & 1;
        auto cmp = [&](const node& a, const node& b) -> bool {
            if (a.lastCount != b.lastCount) return a.lastCount > b.lastCount;
            if (a.type == TILE_GENERAL && a.team != playerId) return true;
            if (b.type == TILE_GENERAL && b.team != playerId) return false;
            if (a.team == -1) return rdret;
            if (b.team == -1) return !rdret;
            if (a.team == playerId && b.team != playerId) return false;
            if (a.team != playerId && b.team == playerId) return true;
            if (a.team == playerId && b.team == playerId)
                return a.army > b.army;
            return a.army < b.army;
        };
        int best_dir = std::min_element(p.begin(), p.end(), cmp)->dir;
        Coord nextCoo(coo.x + dx[best_dir], coo.y + dy[best_dir]);
        lastCoords.push_back(nextCoo);
        if (lastCoords.size() > 100) lastCoords.pop_front();
        moveQueue.emplace_back(MoveType::MOVE_ARMY, coo, nextCoo, false);
    }
};

// Do not forget to register your bot.
static BotRegistrar<SmartRandomBot> smartRandomBot_reg("SmartRandomBot");

#endif  // LGEN_BOTS_SMARTRANDOMBOT
