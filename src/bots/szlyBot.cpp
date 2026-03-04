/**
 * @file szlyBot.cpp
 *
 * szlyBot migrated from LocalGen v5.
 *
 * @copyright Copyright (c) SZXC Work Group.
 */

#ifndef LGEN_BOTS_SZLYBOT
#define LGEN_BOTS_SZLYBOT

#include <array>
#include <cmath>
#include <queue>
#include <random>
#include <vector>

#include "../GameEngine/bot.h"
#include "../GameEngine/game.hpp"

class SzlyBot : public BasicBot {
   private:
    using value_t = int64_t;
    constexpr static value_t INF = 10'000'000'000'000'000LL;
    constexpr static Coord delta[] = {{-1, 0}, {0, -1}, {1, 0}, {0, 1}};

    enum class BotMode { ATTACK, EXPLORE };

    index_t id;
    pos_t height, width, W;
    index_t playerCount;

    BotMode mode;
    Coord lastPos;
    std::vector<Coord> seenGeneral;
    Coord myGeneral;

    std::vector<value_t> blockValue;
    std::vector<value_t> eval;
    std::vector<Coord> parent;
    std::vector<pos_t> dist;
    std::vector<tile_type_e> blockType;
    std::vector<bool> knownBlockType;
    std::array<value_t, 16> blockValueWeight;

    std::mt19937 rng{std::random_device{}()};

    inline size_t idx(pos_t x, pos_t y) const {
        return static_cast<size_t>(x * W + y);
    }

    inline bool isValidPosition(pos_t x, pos_t y,
                                const BoardView& board) const {
        return x >= 1 && x <= board.row && y >= 1 && y <= board.col &&
               !isImpassableTile(blockType[idx(x, y)]);
    }

    inline pos_t approxDist(Coord a, Coord b) const {
        return std::abs(a.x - b.x) + std::abs(a.y - b.y);
    }

    tile_type_e getType(const BoardView& board, pos_t x, pos_t y) {
        const TileView& tile = board.tileAt(x, y);
        if (tile.visible) {
            knownBlockType[idx(x, y)] = true;
            return tile.type;
        }
        if (isImpassableTile(tile.type) || tile.type == TILE_CITY) {
            return TILE_OBSTACLE;
        }
        if (tile.type == TILE_DESERT) {
            return TILE_OBSTACLE;
        }
        if (tile.type == TILE_GENERAL) {
            return TILE_BLANK;
        }
        return tile.type;
    }

    void computeRoutes(Coord st, const BoardView& board) {
        static const value_t typeValues[] = {
            /* TILE_SPAWN=0 */ -5,
            /* TILE_BLANK=1 */ -5,
            /* TILE_MOUNTAIN=2 */ -INF,
            /* TILE_CITY=3 */ -40,
            /* TILE_SWAMP=4 */ -10,
            /* TILE_DESERT=5 */ 0,
            /* TILE_LOOKOUT=6 */ -INF,
            /* TILE_OBSERVATORY=7 */ -INF,
            /* TILE_OBSTACLE=8 */ 0,
        };

        auto gv = [&](pos_t x, pos_t y) -> value_t {
            if (!isValidPosition(x, y, board)) return -INF;
            const TileView& tile = board.tileAt(x, y);
            if (tile.occupier == id) return tile.army;
            if (tile.visible) return -tile.army;
            return typeValues[blockType[idx(x, y)]];
        };

        for (pos_t i = 0; i <= height + 1; ++i) {
            for (pos_t j = 0; j <= width + 1; ++j) {
                dist[idx(i, j)] = 0x3f3f3f3f;
                eval[idx(i, j)] = -INF;
            }
        }

        std::queue<Coord> q;
        q.push(st);
        dist[idx(st.x, st.y)] = 0;
        eval[idx(st.x, st.y)] = 0;
        parent[idx(st.x, st.y)] = Coord(-1, -1);

        while (!q.empty()) {
            Coord cur = q.front();
            q.pop();
            pos_t x = cur.x, y = cur.y;
            pos_t curDist = dist[idx(x, y)];
            value_t curEval = (eval[idx(x, y)] += gv(x, y));

            for (int i = 0; i < 4; ++i) {
                pos_t nx = x + delta[i].x;
                pos_t ny = y + delta[i].y;
                if (nx < 1 || nx > height || ny < 1 || ny > width) continue;
                if (isImpassableTile(blockType[idx(nx, ny)])) continue;

                pos_t nd = curDist + 1;
                if (nd < dist[idx(nx, ny)]) {
                    dist[idx(nx, ny)] = nd;
                    eval[idx(nx, ny)] = curEval;
                    parent[idx(nx, ny)] = cur;
                    q.emplace(nx, ny);
                } else if (nd == dist[idx(nx, ny)] && curEval > eval[idx(nx, ny)]) {
                    eval[idx(nx, ny)] = curEval;
                    parent[idx(nx, ny)] = cur;
                }
            }
        }

        for (pos_t i = 1; i <= height; ++i) {
            for (pos_t j = 1; j <= width; ++j) {
                const TileView& tile = board.tileAt(i, j);
                blockValue[idx(i, j)] =
                    (tile.occupier == id)
                        ? -INF
                        : blockValueWeight[blockType[idx(i, j)]] - dist[idx(i, j)];
            }
        }
    }

    Coord moveTowards(Coord st, Coord dest) const {
        int maxIterations = height * width;
        while (parent[idx(dest.x, dest.y)] != st) {
            dest = parent[idx(dest.x, dest.y)];
            if (dest.x == -1 || dest.y == -1 || --maxIterations <= 0) {
                return st;
            }
        }
        return dest;
    }

    Coord findStrongestUnit(const BoardView& board) {
        value_t maxArmy = 0;
        Coord maxCoo = myGeneral;

        for (pos_t i = 1; i <= height; ++i) {
            for (pos_t j = 1; j <= width; ++j) {
                const TileView& tile = board.tileAt(i, j);
                if (tile.occupier == id && tile.army > 0) {
                    value_t weightedArmy = tile.army;
                    if (blockType[idx(i, j)] == TILE_BLANK) {
                        weightedArmy -= weightedArmy / 4;
                    } else if (blockType[idx(i, j)] == TILE_CITY) {
                        weightedArmy -= weightedArmy / 6;
                    }
                    if (weightedArmy > maxArmy) {
                        maxArmy = weightedArmy;
                        maxCoo = Coord(i, j);
                    }
                }
            }
        }
        return maxCoo;
    }

   public:
    void init(index_t playerId,
              const game::GameConstantsPack& constants) override {
        id = playerId;
        height = constants.mapHeight;
        width = constants.mapWidth;
        W = width + 2;
        playerCount = constants.playerCount;

        blockValue.assign((height + 2) * W, 0);
        eval.assign((height + 2) * W, 0);
        parent.assign((height + 2) * W, Coord(-1, -1));
        dist.assign((height + 2) * W, 0x3f3f3f3f);
        blockType.assign((height + 2) * W, TILE_BLANK);
        knownBlockType.assign((height + 2) * W, false);
        seenGeneral.assign(playerCount, Coord(-1, -1));

        lastPos = Coord(-1, -1);
        myGeneral = Coord(-1, -1);

        blockValueWeight.fill(-INF);
        blockValueWeight[TILE_BLANK] = 30;
        blockValueWeight[TILE_SWAMP] = -1000;
        blockValueWeight[TILE_MOUNTAIN] = -INF;
        blockValueWeight[TILE_GENERAL] = 6;
        blockValueWeight[TILE_CITY] = 35;
        blockValueWeight[TILE_DESERT] = 25;
        blockValueWeight[TILE_OBSTACLE] = 25;
    }

    void requestMove(const BoardView& boardView,
                     const std::vector<game::RankItem>& rank) override {
        moveQueue.clear();

        for (pos_t i = 1; i <= height; ++i) {
            for (pos_t j = 1; j <= width; ++j) {
                const TileView& tile = boardView.tileAt(i, j);
                if (tile.visible && tile.type == TILE_GENERAL) {
                    if (tile.occupier == id) {
                        myGeneral = Coord(i, j);
                    } else if (tile.occupier >= 0 &&
                               tile.occupier < playerCount) {
                        seenGeneral[tile.occupier] = Coord(i, j);
                    }
                }
            }
        }

        for (pos_t i = 1; i <= height; ++i) {
            for (pos_t j = 1; j <= width; ++j) {
                if (!knownBlockType[idx(i, j)]) {
                    blockType[idx(i, j)] = getType(boardView, i, j);
                }
            }
        }

        Coord coo(-1, -1);
        if (lastPos.x != -1) {
            const TileView& tile = boardView.tileAt(lastPos);
            if (tile.occupier == id && tile.army > 1) {
                coo = lastPos;
            }
        }
        if (coo.x == -1) {
            coo = findStrongestUnit(boardView);
        }
        if (coo.x == -1 || boardView.tileAt(coo).army <= 1) {
            return;
        }

        mode = BotMode::EXPLORE;
        value_t minArmy = INF;
        index_t targetId = -1;

        for (const auto& item : rank) {
            if (item.player != id && seenGeneral[item.player].x != -1 && item.alive) {
                mode = BotMode::ATTACK;
                if (item.army < minArmy) {
                    minArmy = item.army;
                    targetId = item.player;
                }
            }
        }

        computeRoutes(coo, boardView);

        Coord target = coo;
        double P_multiplier = std::max(0.7, std::pow(0.85, 0));

        if (mode == BotMode::ATTACK && targetId != -1) {
            target = seenGeneral[targetId];
        } else {
            value_t maxBlockValue = -INF;
            for (pos_t i = 1; i <= height; ++i) {
                for (pos_t j = 1; j <= width; ++j) {
                    if (blockValue[idx(i, j)] > maxBlockValue) {
                        maxBlockValue = blockValue[idx(i, j)];
                        target = Coord(i, j);
                    }
                }
            }
            P_multiplier *= 0.85;
        }

        if (parent[idx(target.x, target.y)].x == -1) {
            pos_t minDist = 0x3f3f3f3f;
            for (pos_t i = 1; i <= height; ++i) {
                for (pos_t j = 1; j <= width; ++j) {
                    if (parent[idx(i, j)].x != -1 && dist[idx(i, j)] < minDist) {
                        minDist = dist[idx(i, j)];
                        target = Coord(i, j);
                    }
                }
            }
        }

        Coord fastestMove = moveTowards(coo, target);

        if (blockType[idx(coo.x, coo.y)] == TILE_SWAMP) {
            moveQueue.emplace_back(MoveType::MOVE_ARMY, coo, fastestMove,
                                   false);
            lastPos = fastestMove;
            return;
        }

        Coord gatherPos(-1, -1);
        double maxWeight = -1e18;

        for (int i = 0; i < 4; ++i) {
            pos_t nx = coo.x + delta[i].x;
            pos_t ny = coo.y + delta[i].y;
            if (nx < 1 || nx > height || ny < 1 || ny > width) continue;

            Coord nc(nx, ny);
            const TileView& adjTile = boardView.tileAt(nx, ny);

            if (nc != fastestMove && adjTile.occupier == id &&
                adjTile.army > 2) {
                double weight = adjTile.army;
                if (myGeneral.x != -1) {
                    weight -= 250.0 / (approxDist(nc, myGeneral) + 0.5);
                } else {
                    weight -= 3;
                }
                if (blockType[idx(nx, ny)] == TILE_SWAMP) {
                    weight += 1;
                } else if (blockType[idx(nx, ny)] == TILE_GENERAL) {
                    weight *= 0.6;
                } else if (blockType[idx(nx, ny)] == TILE_CITY) {
                    weight *= 0.8;
                }
                if (weight > maxWeight) {
                    maxWeight = weight;
                    gatherPos = nc;
                }
            }
        }

        std::uniform_real_distribution<double> dis(-1.0, 1.0);
        if (gatherPos.x == -1 ||
            dis(rng) > P_multiplier * std::tanh(maxWeight * 0.015)) {
            moveQueue.emplace_back(MoveType::MOVE_ARMY, coo, fastestMove,
                                   false);
            lastPos = fastestMove;
        } else {
            moveQueue.emplace_back(MoveType::MOVE_ARMY, gatherPos, coo, false);
            lastPos = coo;
        }
    }
};

static BotRegistrar<SzlyBot> szlyBot_reg("SzlyBot");

#endif  // LGEN_BOTS_SZLYBOT