// Copyright (C) 2026 GoodCoder666
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file gcBot.cpp
 *
 * GcBot
 *
 * @author GoodCoder666
 */

#ifndef LGEN_BOTS_GCBOT
#define LGEN_BOTS_GCBOT

#include <algorithm>
#include <queue>
#include <random>
#include <vector>

#include "core/bot.h"
#include "core/game.hpp"

class GcBot : public BasicBot {
   private:
    using value_t = long long;
    constexpr static value_t INF = 10'000'000'000'000'000LL;
    constexpr static Coord delta[] = {{-1, 0}, {0, -1}, {1, 0}, {0, 1}};

    pos_t height, width, W;
    index_t playerCnt;
    index_t id;

    turn_t halfTurn, turn;

    struct TileInfo {
        tile_type_e type = TILE_PLAIN;
        army_t army = -1;
        Coord par{-1, -1};
        value_t eval = 0;
        pos_t dist = 0;
        index_t occupier = -1;
        bool known = false;
    };

    std::vector<TileInfo> tiles;
    std::vector<Coord> seenGeneral;
    value_t tileTypeValue[9];
    Coord prevTarget;
    Coord lastPos;
    std::mt19937 rnd;

    inline TileInfo& tileAt(pos_t x, pos_t y) { return tiles[x * W + y]; }
    inline const TileInfo& tileAt(pos_t x, pos_t y) const {
        return tiles[x * W + y];
    }

    inline TileInfo& tileAt(Coord coo) { return tileAt(coo.x, coo.y); }
    inline const TileInfo& tileAt(Coord coo) const {
        return tileAt(coo.x, coo.y);
    }

    inline bool accessible(pos_t x, pos_t y) const {
        if (x < 1 || x > height || y < 1 || y > width) return false;
        tile_type_e type = tileAt(x, y).type;
        return !isImpassableTile(type) && type != TILE_OBSTACLE;
    }

    inline bool accessible(Coord coo) const { return accessible(coo.x, coo.y); }

    inline int approxDist(Coord st, Coord dest) const {
        return std::abs(st.x - dest.x) + std::abs(st.y - dest.y);
    }

    void evaluateRouteCosts(Coord st) {
        const army_t cityBonus = tileAt(st).army / 12;
        auto gv = [&](const TileInfo& tile) -> value_t {
            if (tile.type == TILE_SWAMP) return -2;
            if (tile.type == TILE_OBSTACLE) return 0;
            if (!tile.known) return 2;
            if (tile.occupier == id) return tile.army - 1;
            if (tile.type == TILE_CITY) return -tile.army / 2 + cityBonus;
            return -tile.army;
        };

        for (TileInfo& tile : tiles) {
            tile.dist = 32767;
            tile.eval = -INF;
            tile.par = Coord(-1, -1);
        }

        std::queue<Coord> q;
        q.push(st);
        tileAt(st).dist = 0;
        tileAt(st).eval = 0;

        while (!q.empty()) {
            Coord cur = q.front();
            q.pop();
            TileInfo& tile = tileAt(cur);
            tile.eval += gv(tile);

            if (tile.type == TILE_SWAMP) {
                for (auto [dx, dy] : delta) {
                    pos_t nx = cur.x + dx, ny = cur.y + dy;
                    if (1 <= nx && nx <= height && 1 <= ny && ny <= width &&
                        tileAt(nx, ny).type == TILE_OBSTACLE) {
                        tile.eval++;
                        break;
                    }
                }
            }

            for (auto [dx, dy] : delta) {
                pos_t nx = cur.x + dx, ny = cur.y + dy;
                if (!accessible(nx, ny)) continue;
                TileInfo& nTile = tileAt(nx, ny);
                pos_t nd = tile.dist + 1;
                if (nd < nTile.dist) {
                    nTile.dist = nd;
                    nTile.eval = tile.eval;
                    nTile.par = cur;
                    q.emplace(nx, ny);
                } else if (nd == nTile.dist && tile.eval > nTile.eval) {
                    nTile.eval = tile.eval;
                    nTile.par = cur;
                }
            }
        }
    }

    Coord moveTowards(Coord st, Coord dest) const {
        Coord nxt;
        while ((nxt = tileAt(dest).par) != st) {
            dest = nxt;
            if (dest.x == -1 || dest.y == -1) {
                return st;
            }
        }
        return dest;
    }

    Coord maxArmyPos() const {
        value_t maxArmy = 0;
        Coord maxCoo = seenGeneral[id];
        for (pos_t i = 1; i <= height; ++i) {
            for (pos_t j = 1; j <= width; ++j) {
                const TileInfo& tile = tileAt(i, j);
                if (tile.occupier == id) {
                    value_t weightedArmy = tile.army;
                    if (tile.type == TILE_GENERAL)
                        weightedArmy -= weightedArmy / 4;
                    else if (tile.type == TILE_CITY)
                        weightedArmy -= weightedArmy / 6;
                    if (weightedArmy > maxArmy) {
                        maxArmy = weightedArmy;
                        maxCoo = Coord(i, j);
                    }
                }
            }
        }
        return maxCoo;
    }

    void updateMemory(const BoardView& board) {
        for (pos_t i = 1; i <= height; ++i) {
            for (pos_t j = 1; j <= width; ++j) {
                const TileView& view = board.tileAt(i, j);
                TileInfo& tile = tileAt(i, j);
                tile_type_e type =
                    view.type == TILE_DESERT ? TILE_PLAIN : view.type;
                if (view.visible) {
                    tile.known = true;
                    tile.type = type;
                    tile.occupier = view.occupier;
                    if (type == TILE_GENERAL && tile.occupier != -1)
                        seenGeneral[tile.occupier] = Coord(i, j);
                    army_t seenArmy = view.army;
                    tile.army = (tile.army < 0 || tile.occupier == id ||
                                 tile.occupier == -1)
                                    ? seenArmy
                                    : (seenArmy + tile.army) / 2;
                } else {
                    if (!tile.known) tile.type = type;
                    if (tile.occupier == id) {
                        tile.occupier = view.occupier;
                        tile.army = view.army;
                    }
                }
            }
        }
    }

    Move selectOpening() {
        Coord coo = seenGeneral[id];
        army_t minArmy = tileAt(coo).army - 1;
        lastPos = coo;
        for (auto [dx, dy] : delta) {
            auto nx = coo.x + dx, ny = coo.y + dy;
            if (accessible(nx, ny)) {
                const TileInfo& tile = tileAt(nx, ny);
                if (tile.type != TILE_CITY && tile.army >= 0) continue;
                if (tile.army < minArmy) {
                    minArmy = tile.army;
                    lastPos = Coord(nx, ny);
                }
            }
        }
        if (lastPos == coo) return Move();
        return Move(MoveType::MOVE_ARMY, coo, lastPos, false);
    }

   public:
    GcBot() : rnd(std::random_device{}()) {}

    void init(index_t playerId, const GameConstantsPack& constants) override {
        id = playerId;
        height = constants.mapHeight;
        width = constants.mapWidth;
        W = width + 2;
        playerCnt = constants.playerCount;

        halfTurn = turn = 0;

        std::fill(tileTypeValue, tileTypeValue + 9, -INF);
        tileTypeValue[TILE_SPAWN] = 0;

        prevTarget = Coord(-1, -1);
        lastPos = Coord(-1, -1);

        seenGeneral.assign(playerCnt, Coord(-1, -1));
        tiles.resize((height + 2) * W);
    }

    void requestMove(const BoardView& board,
                     const std::vector<RankItem>& rank) override {
        ++halfTurn;
        turn += (halfTurn & 1);
        moveQueue.clear();
        updateMemory(board);

        const RankItem& self = *std::find_if(
            rank.begin(), rank.end(),
            [this](const RankItem& item) { return item.player == id; });

        if (turn < 13 && self.land == 1) {
            moveQueue.push_back(selectOpening());
            return;
        }

        const double armyStrength =
            std::log(std::min(static_cast<army_t>(3000), self.army) + 1.0);
        tileTypeValue[TILE_PLAIN] = 55 - static_cast<value_t>(5 * armyStrength);
        tileTypeValue[TILE_SWAMP] =
            -500 + static_cast<value_t>(50 * armyStrength);
        tileTypeValue[TILE_CITY] =
            static_cast<value_t>(armyStrength * armyStrength);
        tileTypeValue[TILE_OBSTACLE] =
            35 + static_cast<value_t>(4 * armyStrength);

        Coord coo = lastPos;
        if (!accessible(coo) || tileAt(coo).occupier != id ||
            tileAt(coo).army < 2) {
            coo = maxArmyPos();
            prevTarget = Coord(-1, -1);
        } else if (coo == prevTarget ||
                   (prevTarget.x != -1 && tileAt(prevTarget).occupier == id)) {
            prevTarget = Coord(-1, -1);
        }

        value_t minArmy = INF;
        index_t targetId = -1;
        for (const RankItem& item : rank) {
            index_t that = item.player;
            if (that != id && item.alive && seenGeneral[that].x != -1) {
                value_t thatArmy = item.army;
                if (thatArmy < minArmy) {
                    minArmy = thatArmy;
                    targetId = that;
                }
            }
        }

        evaluateRouteCosts(coo);
        Coord targetPos(-1, -1);
        static std::uniform_real_distribution<double> dis(0, 1);

        if (targetId != -1) {
            targetPos = seenGeneral[targetId];
        } else if (prevTarget.x == -1 ||
                   tileAt(prevTarget).type != TILE_PLAIN ||
                   tileAt(prevTarget).known) {
            value_t maxBlockValue = -INF;

            std::vector<std::pair<value_t, Coord>> unknownPlains;
            for (pos_t i = 1; i <= height; ++i) {
                for (pos_t j = 1; j <= width; ++j) {
                    const TileInfo& tile = tileAt(i, j);
                    if (tile.dist >= 500 || tile.eval <= -100 ||
                        (tile.type != TILE_PLAIN && tile.type != TILE_SWAMP))
                        continue;
                    bool needExploration =
                        (tile.type == TILE_PLAIN && !tile.known);
                    if (!needExploration)
                        for (auto [dx, dy] : delta) {
                            auto nx = i + dx, ny = j + dy;
                            if (!accessible(nx, ny)) continue;
                            const TileInfo& nTile = tileAt(nx, ny);
                            if (nTile.type == TILE_OBSTACLE && !nTile.known) {
                                needExploration = true;
                                break;
                            }
                        }
                    if (needExploration)
                        unknownPlains.emplace_back(tile.dist, Coord(i, j));
                }
            }
            if (!unknownPlains.empty()) {
                std::size_t k = unknownPlains.size() / 4;
                std::nth_element(unknownPlains.begin(),
                                 unknownPlains.begin() + k,
                                 unknownPlains.end());
                targetPos = unknownPlains[k].second;
                maxBlockValue =
                    static_cast<value_t>(4.0 * std::sqrt(self.army)) -
                    tileAt(targetPos).dist * 3;
            }

            auto computeTileValue = [&](const Coord& pos) -> value_t {
                const TileInfo& tile = tileAt(pos);
                value_t tileValue = tileTypeValue[tile.type] + tile.eval / 5 -
                                    (tile.dist + tile.army / 2);
                if (seenGeneral[id].x != -1) {
                    tileValue -= static_cast<value_t>(
                        std::sqrt(approxDist(pos, seenGeneral[id]) * 40.0));
                }
                return tileValue;
            };

            for (pos_t i = 1; i <= height; ++i) {
                for (pos_t j = 1; j <= width; ++j) {
                    Coord pos(i, j);
                    const TileInfo& tile = tileAt(pos);
                    if (tile.occupier != id && tile.dist < 500 &&
                        !isImpassableTile(tile.type) &&
                        tile.type != TILE_SPAWN) {
                        value_t blockValue = computeTileValue(pos);
                        if (blockValue > maxBlockValue) {
                            maxBlockValue = blockValue;
                            targetPos = pos;
                        }
                    }
                }
            }

            if (prevTarget.x != -1) {
                const TileInfo& pTile = tileAt(prevTarget);
                if (pTile.occupier != id && !isImpassableTile(pTile.type)) {
                    value_t prevBlockValue = computeTileValue(prevTarget);
                    value_t keepTargetMargin =
                        std::max(25.0, 0.03 * std::pow(self.army, 0.86));
                    if (maxBlockValue - prevBlockValue < keepTargetMargin) {
                        targetPos = prevTarget;
                    }
                }
            }
        } else {
            targetPos = prevTarget;
        }
        prevTarget = targetPos;

        if (targetPos.x == -1) {
            lastPos = Coord(-1, -1);
            moveQueue.emplace_back(MoveType::EMPTY);
            return;
        }

        if (tileAt(coo).type == TILE_SWAMP || dis(rnd) > 0.07 ||
            tileAt(targetPos).eval > 150) {
            Coord nextPos = moveTowards(coo, targetPos);
            lastPos = nextPos;
            moveQueue.emplace_back(MoveType::MOVE_ARMY, coo, nextPos, false);
            return;
        }

        evaluateRouteCosts(targetPos);

        value_t maxWeight = -INF;
        Coord newFocus(-1, -1);
        for (pos_t i = 1; i <= height; ++i) {
            for (pos_t j = 1; j <= width; ++j) {
                const TileInfo& tile = tileAt(i, j);
                if (tile.occupier == id && tile.army > 1) {
                    value_t weight = tile.eval - 30 * tile.dist;
                    if (tile.type == TILE_GENERAL) weight -= weight / 4;
                    if (weight > maxWeight) {
                        maxWeight = weight;
                        newFocus = Coord(i, j);
                    }
                }
            }
        }

        if (newFocus == Coord(-1, -1)) {
            lastPos = Coord(-1, -1);
            moveQueue.emplace_back(MoveType::EMPTY);
            return;
        }

        evaluateRouteCosts(newFocus);
        Coord nextPos = moveTowards(newFocus, targetPos);
        lastPos = nextPos;
        moveQueue.emplace_back(MoveType::MOVE_ARMY, newFocus, nextPos, false);
    }
};

static BotRegistrar<GcBot> gcBot_reg("GcBot");

#endif  // LGEN_BOTS_GCBOT