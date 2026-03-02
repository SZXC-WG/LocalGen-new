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

    // Game constants (from init)
    index_t id;
    pos_t height, width;
    index_t playerCount;

    // State maintenance
    BotMode mode;
    Coord lastPos;                   // Replaces v5 focus system
    std::vector<Coord> seenGeneral;  // Records enemy general positions
    Coord myGeneral;                 // Own general position

    // Data storage
    std::vector<std::vector<value_t>> blockValue;
    std::vector<std::vector<value_t>> eval;
    std::vector<std::vector<Coord>> parent;
    std::vector<std::vector<pos_t>> dist;
    std::vector<std::vector<tile_type_e>> blockType;
    std::vector<std::vector<bool>> knownBlockType;
    std::array<value_t, 16>
        blockValueWeight;  // Terrain weights (indexed by v6 enum)

    std::mt19937 rng{std::random_device{}()};

    inline bool isValidPosition(pos_t x, pos_t y,
                                const BoardView& board) const {
        return x >= 1 && x <= board.row && y >= 1 && y <= board.col &&
               !isImpassableTile(blockType[x][y]);
    }

    inline pos_t approxDist(Coord a, Coord b) const {
        return std::abs(a.x - b.x) + std::abs(a.y - b.y);
    }

    tile_type_e getType(const BoardView& board, pos_t x, pos_t y) {
        const TileView& tile = board.tileAt(x, y);
        if (tile.visible) {
            knownBlockType[x][y] = true;
            return tile.type;
        }
        // v5: unpassable or city -> unknown (5)
        // v6: map to TILE_OBSTACLE
        if (isImpassableTile(tile.type) || tile.type == TILE_CITY) {
            return TILE_OBSTACLE;
        }
        // v5: desert -> unknown (5) when invisible
        // v6: map to TILE_OBSTACLE
        if (tile.type == TILE_DESERT) {
            return TILE_OBSTACLE;
        }
        // Invisible general appears as blank
        if (tile.type == TILE_GENERAL) {
            return TILE_BLANK;
        }
        // plain/swamp remain as-is
        return tile.type;
    }

    void computeRoutes(Coord st, const BoardView& board) {
        // Type values for eval computation
        // Indexed by v6 tile_type_e
        // v5: {-5, -10, -INF, -5, -40, 0, -INF, -INF, -200}
        // Mapped to v6 enum values
        static const value_t typeValues[] = {
            /* TILE_SPAWN=0 */ -5,
            /* TILE_BLANK=1 */ -5,
            /* TILE_MOUNTAIN=2 */ -INF,
            /* TILE_CITY=3 */ -40,
            /* TILE_SWAMP=4 */ -10,
            /* TILE_DESERT=5 */ 0,
            /* TILE_LOOKOUT=6 */ -INF,
            /* TILE_OBSERVATORY=7 */ -INF,
            /* TILE_OBSTACLE=8 */ 0,  // v5: unknown = 0
        };

        auto gv = [&](pos_t x, pos_t y) -> value_t {
            if (!isValidPosition(x, y, board)) return -INF;
            const TileView& tile = board.tileAt(x, y);
            if (tile.occupier == id) return tile.army;
            if (tile.visible) return -tile.army;
            return typeValues[blockType[x][y]];
        };

        // Reset arrays
        for (pos_t i = 0; i <= height + 1; ++i) {
            for (pos_t j = 0; j <= width + 1; ++j) {
                dist[i][j] = 0x3f3f3f3f;
                eval[i][j] = -INF;
            }
        }

        std::queue<Coord> q;
        q.push(st);
        dist[st.x][st.y] = 0;
        eval[st.x][st.y] = 0;
        parent[st.x][st.y] = Coord(-1, -1);

        while (!q.empty()) {
            Coord cur = q.front();
            q.pop();
            pos_t x = cur.x, y = cur.y;
            pos_t curDist = dist[x][y];
            value_t curEval = (eval[x][y] += gv(x, y));

            for (int i = 0; i < 4; ++i) {
                pos_t nx = x + delta[i].x;
                pos_t ny = y + delta[i].y;
                if (nx < 1 || nx > height || ny < 1 || ny > width) continue;
                if (isImpassableTile(blockType[nx][ny])) continue;

                pos_t nd = curDist + 1;
                if (nd < dist[nx][ny]) {
                    dist[nx][ny] = nd;
                    eval[nx][ny] = curEval;
                    parent[nx][ny] = cur;
                    q.emplace(nx, ny);
                } else if (nd == dist[nx][ny] && curEval > eval[nx][ny]) {
                    eval[nx][ny] = curEval;
                    parent[nx][ny] = cur;
                }
            }
        }

        // Compute block values
        for (pos_t i = 1; i <= height; ++i) {
            for (pos_t j = 1; j <= width; ++j) {
                const TileView& tile = board.tileAt(i, j);
                blockValue[i][j] =
                    (tile.occupier == id)
                        ? -INF
                        : blockValueWeight[blockType[i][j]] - dist[i][j];
            }
        }
    }

    Coord moveTowards(Coord st, Coord dest) const {
        // Safety: limit iterations to prevent infinite loop
        int maxIterations = height * width;
        while (parent[dest.x][dest.y] != st) {
            dest = parent[dest.x][dest.y];
            // Safety check: if parent is invalid or max iterations reached
            if (dest.x == -1 || dest.y == -1 || --maxIterations <= 0) {
                return st;  // Return start position as fallback
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
                    // Apply terrain modifiers
                    if (blockType[i][j] == TILE_BLANK) {
                        weightedArmy -= weightedArmy / 4;
                    } else if (blockType[i][j] == TILE_CITY) {
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
        playerCount = constants.playerCount;

        // Initialize data structures
        blockValue.assign(height + 2, std::vector<value_t>(width + 2, 0));
        eval.assign(height + 2, std::vector<value_t>(width + 2, 0));
        parent.assign(height + 2, std::vector<Coord>(width + 2, Coord(-1, -1)));
        dist.assign(height + 2, std::vector<pos_t>(width + 2, 0x3f3f3f3f));
        blockType.assign(height + 2,
                         std::vector<tile_type_e>(width + 2, TILE_BLANK));
        knownBlockType.assign(height + 2, std::vector<bool>(width + 2, false));
        seenGeneral.assign(playerCount, Coord(-1, -1));

        lastPos = Coord(-1, -1);
        myGeneral = Coord(-1, -1);

        // Initialize block value weights (indexed by v6 tile_type_e)
        // v5: {31 - plainRate, -1000, -INF, 6, 35, 25}
        // Mapped to v6 enum values
        blockValueWeight.fill(-INF);
        blockValueWeight[TILE_BLANK] = 30;  // plain
        blockValueWeight[TILE_SWAMP] = -1000;
        blockValueWeight[TILE_MOUNTAIN] = -INF;
        blockValueWeight[TILE_GENERAL] = 6;  // general (spawn)
        blockValueWeight[TILE_CITY] = 35;
        blockValueWeight[TILE_DESERT] = 25;
        blockValueWeight[TILE_OBSTACLE] = 25;  // unknown
    }

    void requestMove(const BoardView& boardView,
                     const std::vector<game::RankItem>& rank) override {
        moveQueue.clear();

        // 1. Update seen generals
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

        // 2. Update block types
        for (pos_t i = 1; i <= height; ++i) {
            for (pos_t j = 1; j <= width; ++j) {
                if (!knownBlockType[i][j]) {
                    blockType[i][j] = getType(boardView, i, j);
                }
            }
        }

        // 3. Determine starting position (replace v5 focus system)
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
            return;  // No valid move
        }

        // 4. Determine bot mode
        // Note: rank is sorted by army count, NOT indexed by player ID
        // Use rank[i].player to get the actual player ID
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

        // 5. Compute routes from starting position
        computeRoutes(coo, boardView);

        // 6. Determine target and move
        Coord target = coo;  // Default to current position
        double P_multiplier =
            std::max(0.7, std::pow(0.85, 0));  // No turn counter in v6

        if (mode == BotMode::ATTACK && targetId != -1) {
            target = seenGeneral[targetId];
        } else {
            // Explore mode: find highest value block
            value_t maxBlockValue = -INF;
            for (pos_t i = 1; i <= height; ++i) {
                for (pos_t j = 1; j <= width; ++j) {
                    if (blockValue[i][j] > maxBlockValue) {
                        maxBlockValue = blockValue[i][j];
                        target = Coord(i, j);
                    }
                }
            }
            P_multiplier *= 0.85;
        }

        // Safety check: ensure target is reachable
        if (parent[target.x][target.y].x == -1) {
            // Target not reachable, find nearest reachable
            pos_t minDist = 0x3f3f3f3f;
            for (pos_t i = 1; i <= height; ++i) {
                for (pos_t j = 1; j <= width; ++j) {
                    if (parent[i][j].x != -1 && dist[i][j] < minDist) {
                        minDist = dist[i][j];
                        target = Coord(i, j);
                    }
                }
            }
        }

        // 7. Smart move with potential gathering
        Coord fastestMove = moveTowards(coo, target);

        // Check if on swamp - always move directly
        if (blockType[coo.x][coo.y] == TILE_SWAMP) {
            moveQueue.emplace_back(MoveType::MOVE_ARMY, coo, fastestMove,
                                   false);
            lastPos = fastestMove;
            return;
        }

        // Try to gather nearby army
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
                // Distance penalty from general
                if (myGeneral.x != -1) {
                    weight -= 250.0 / (approxDist(nc, myGeneral) + 0.5);
                } else {
                    weight -= 3;
                }
                // Terrain modifiers
                if (blockType[nx][ny] == TILE_SWAMP) {
                    weight += 1;
                } else if (blockType[nx][ny] == TILE_GENERAL) {
                    weight *= 0.6;
                } else if (blockType[nx][ny] == TILE_CITY) {
                    weight *= 0.8;
                }
                if (weight > maxWeight) {
                    maxWeight = weight;
                    gatherPos = nc;
                }
            }
        }

        // Random decision between direct move and gathering
        std::uniform_real_distribution<double> dis(-1.0, 1.0);
        if (gatherPos.x == -1 ||
            dis(rng) > P_multiplier * std::tanh(maxWeight * 0.015)) {
            // Direct move towards target
            moveQueue.emplace_back(MoveType::MOVE_ARMY, coo, fastestMove,
                                   false);
            lastPos = fastestMove;
        } else {
            // Gather army (move from gatherPos to coo)
            moveQueue.emplace_back(MoveType::MOVE_ARMY, gatherPos, coo, false);
            lastPos = coo;
        }
    }
};

static BotRegistrar<SzlyBot> szlyBot_reg("SzlyBot");

#endif  // LGEN_BOTS_SZLYBOT
