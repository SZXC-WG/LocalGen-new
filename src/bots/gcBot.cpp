/**
 * @file gcBot.cpp
 *
 * gcBot from LocalGen v5.
 * Features: army memory with exponential smoothing, dynamic weight adjustment,
 *           prevTarget continuity, dual BFS army gathering.
 *
 * @copyright Copyright (c) SZXC Work Group.
 */

#ifndef LGEN_BOTS_GCBOT
#define LGEN_BOTS_GCBOT

#include <array>
#include <queue>
#include <random>

#include "../GameEngine/bot.h"
#include "../GameEngine/game.hpp"

class GcBot : public BasicBot {
   private:
    using value_t = long long;
    constexpr static value_t INF = 10'000'000'000'000'000LL;
    constexpr static Coord delta[] = {{-1, 0}, {0, -1}, {1, 0}, {0, 1}};

    pos_t height, width;
    index_t playerCnt;
    index_t id;
    game::config::Config config;

    turn_t halfTurn, turn;

    BoardView board;
    std::vector<game::RankItem> rank;

    std::vector<Coord> seenGeneral;  // Last seen positions of other generals
    std::vector<value_t> blockTypeValue;
    std::vector<std::vector<value_t>> eval;
    std::vector<std::vector<Coord>> par;
    std::vector<std::vector<pos_t>> dist;
    // 0->plain, 1->swamp, 2->mountain, 3->general, 4->city, 5->unknown
    std::vector<std::vector<int>> blockType;
    std::vector<std::vector<bool>> knownBlockType;
    std::vector<std::vector<value_t>> army;
    Coord prevTarget;
    Coord lastPos;  // Replaces v5's focus system
    std::mt19937 rnd;

    inline bool isValidPosition(pos_t x, pos_t y) const {
        if (x < 1 || x > height || y < 1 || y > width) return false;
        tile_type_e type = board.tileAt(x, y).type;
        // TILE_OBSTACLE represents unseen mountain/city/lookout/observatory
        return !isImpassableTile(type) && type != TILE_OBSTACLE;
    }

    inline int approxDist(Coord st, Coord dest) const {
        return std::abs(st.x - dest.x) + std::abs(st.y - dest.y);
    }

    void evaluateRouteCosts(Coord st) {
        // v5 typeValues: {-5, -10, -INF, -5, -40, 0, -INF, -INF, -200}
        // Mapped to v6 tile types
        static const value_t typeValues[] = {
            /* TILE_BLANK=1 */ -5,
            /* TILE_SWAMP=2 */ -10,
            /* TILE_MOUNTAIN=3 */ -INF,
            /* TILE_CITY=4 */ -40,
            /* TILE_GENERAL=5 */ -5,
            /* TILE_SPAWN=6 */ 0,
            /* TILE_DESERT=7 */ -INF,
            /* TILE_LOOKOUT=8 */ -INF,
            /* TILE_OBSTACLE=9 */ 0  // unknown -> 0
        };

        auto gv = [&](pos_t x, pos_t y) -> value_t {
            int bt = blockType[x][y];
            if (bt == 1) return -1;  // swamp
            if (bt == 5) return 0;   // unknown
            if (board.tileAt(x, y).occupier == id) return army[x][y] - 1;
            return -army[x][y];
        };

        for (pos_t i = 0; i <= height + 1; ++i) {
            for (pos_t j = 0; j <= width + 1; ++j) {
                dist[i][j] = 32767;
                eval[i][j] = -INF;
            }
        }

        std::queue<Coord> q;
        q.push(st);
        dist[st.x][st.y] = 0;
        eval[st.x][st.y] = 0;
        par[st.x][st.y] = Coord(-1, -1);

        while (!q.empty()) {
            Coord cur = q.front();
            q.pop();
            pos_t x = cur.x, y = cur.y;
            pos_t curDist = dist[x][y];
            value_t curEval = (eval[x][y] += gv(x, y));

            for (int i = 0; i < 4; ++i) {
                pos_t nx = x + delta[i].x;
                pos_t ny = y + delta[i].y;
                if (!isValidPosition(nx, ny)) continue;
                pos_t nd = curDist + 1;
                if (nd < dist[nx][ny]) {
                    dist[nx][ny] = nd;
                    eval[nx][ny] = curEval;
                    par[nx][ny] = cur;
                    q.emplace(nx, ny);
                } else if (nd == dist[nx][ny] && curEval > eval[nx][ny]) {
                    eval[nx][ny] = curEval;
                    par[nx][ny] = cur;
                }
            }
        }
    }

    Coord moveTowards(Coord st, Coord dest) const {
        int maxIterations = height * width;
        while (par[dest.x][dest.y] != st) {
            dest = par[dest.x][dest.y];
            if (dest.x == -1 || dest.y == -1 || --maxIterations <= 0) {
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
                if (board.tileAt(i, j).occupier == id) {
                    value_t weightedArmy = army[i][j];
                    int bt = blockType[i][j];
                    if (bt == 0)  // plain
                        weightedArmy -= weightedArmy / 4;
                    else if (bt == 4)  // city
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

    int getBlockType(pos_t x, pos_t y) const {
        const auto& tile = board.tileAt(x, y);
        if (tile.visible) {
            // Map v6 tile_type_e to v5 blockType
            // v5 blockType: 0=plain, 1=swamp, 2=mountain, 3=general, 4=city,
            // 5=unknown
            switch (tile.type) {
                case TILE_BLANK:    return 0;  // plain
                case TILE_SWAMP:    return 1;  // swamp
                case TILE_MOUNTAIN: return 2;  // mountain
                case TILE_CITY:     return 4;      // city
                case TILE_SPAWN:
                    return 3;  // general (TILE_GENERAL = TILE_SPAWN)
                case TILE_DESERT:
                    return 0;  // treat as plain (invisible desert)
                case TILE_LOOKOUT:     return 2;  // treat as mountain
                case TILE_OBSERVATORY: return 2;  // treat as mountain
                case TILE_OBSTACLE:    return 5;  // unknown
                default:               return 5;
            }
        } else {
            // Not visible - TileView has already converted the type:
            // SPAWN/CAPTURED_GENERAL/DESERT -> TILE_BLANK
            // MOUNTAIN/CITY/LOOKOUT/OBSERVATORY -> TILE_OBSTACLE
            // BLANK/SWAMP -> unchanged
            switch (tile.type) {
                case TILE_BLANK:  return 0;  // plain (could be hidden spawn/desert)
                case TILE_SWAMP:  return 1;  // swamp
                default:          return 5;  // unknown (TILE_OBSTACLE, etc.)
            }
        }
    }

   public:
    GcBot() : rnd(std::random_device{}()) {}

    void init(index_t playerId,
              const game::GameConstantsPack& constants) override {
        id = playerId;
        height = constants.mapHeight;
        width = constants.mapWidth;
        playerCnt = constants.playerCount;
        config = constants.config;

        halfTurn = turn = 0;

        // Initialize blockTypeValue: 0->plain, 1->swamp, 2->mountain,
        // 3->general, 4->city, 5->unknown
        blockTypeValue = {60, -500, -INF, 0, 50, 25};

        prevTarget = Coord(-1, -1);
        lastPos = Coord(-1, -1);

        seenGeneral.assign(playerCnt, Coord(-1, -1));
        eval.assign(height + 2, std::vector<value_t>(width + 2, 0));
        par.assign(height + 2, std::vector<Coord>(width + 2, Coord(-1, -1)));
        dist.assign(height + 2, std::vector<pos_t>(width + 2, 0));
        blockType.assign(height + 2, std::vector<int>(width + 2, 5));
        knownBlockType.assign(height + 2, std::vector<bool>(width + 2, false));
        army.assign(height + 2, std::vector<value_t>(width + 2, -1));
    }

    void requestMove(const BoardView& boardView,
                     const std::vector<game::RankItem>& _rank) override {
        ++halfTurn;
        turn += (halfTurn & 1);

        board = boardView;
        rank = _rank;
        // Sort rank by player ID so rank[i] corresponds to player i
        std::sort(begin(rank), end(rank),
                  [](game::RankItem lhs, game::RankItem rhs) -> bool {
                      return lhs.player < rhs.player;
                  });

        moveQueue.clear();

        // Update block types, seen generals & army
        for (pos_t i = 1; i <= height; ++i) {
            for (pos_t j = 1; j <= width; ++j) {
                const auto& tile = board.tileAt(i, j);
                if (tile.visible) {
                    knownBlockType[i][j] = true;
                    blockType[i][j] = getBlockType(i, j);
                    if (tile.type == TILE_GENERAL && tile.occupier >= 0) {
                        seenGeneral[tile.occupier] = Coord(i, j);
                    }
                    value_t seenArmy = tile.army;
                    // Apply exponential smoothing after first seen
                    if (army[i][j] < 0 || tile.occupier == id ||
                        tile.occupier == -1) {
                        army[i][j] = seenArmy;
                    } else {
                        army[i][j] = (seenArmy + army[i][j]) / 2;
                    }
                } else if (!knownBlockType[i][j]) {
                    blockType[i][j] = getBlockType(i, j);
                }
            }
        }

        // Dynamic block values
        if (turn < 13) return;

        blockTypeValue[0] =
            55 + static_cast<value_t>(std::pow(turn, 0.2));  // plain
        blockTypeValue[1] =
            -500 * static_cast<value_t>(std::pow(turn, -0.1));  // swamp
        blockTypeValue[4] =
            28 * static_cast<value_t>(std::pow(turn - 12, 0.15));  // city
        blockTypeValue[5] =
            35 + 15 * static_cast<value_t>(std::pow(turn, 0.15));  // unknown

        // Focus disabled? Use lastPos instead of passed focus
        Coord coo = lastPos;
        if (!isValidPosition(coo.x, coo.y) ||
            board.tileAt(coo.x, coo.y).occupier != id ||
            board.tileAt(coo.x, coo.y).army < 2) {
            coo = maxArmyPos();
            prevTarget = Coord(-1, -1);
        } else if (coo == prevTarget ||
                   (prevTarget.x != -1 &&
                    board.tileAt(prevTarget.x, prevTarget.y).occupier == id)) {
            prevTarget = Coord(-1, -1);
        }

        // Attack if possible
        value_t minArmy = INF;
        index_t targetId = -1;
        for (index_t i = 0; i < playerCnt; ++i) {
            if (i != id && seenGeneral[i] != Coord(-1, -1) && rank[i].alive) {
                value_t thatArmy = rank[i].army;
                if (thatArmy < minArmy) {
                    minArmy = thatArmy;
                    targetId = i;
                }
            }
        }

        // Determine target
        evaluateRouteCosts(coo);
        Coord targetPos(-1, -1);
        static std::uniform_real_distribution<double> dis(0, 1);

        if (targetId != -1) {
            targetPos = seenGeneral[targetId];
        } else if (prevTarget.x == -1 ||
                   blockType[prevTarget.x][prevTarget.y] != 0 ||
                   knownBlockType[prevTarget.x][prevTarget.y]) {
            value_t maxBlockValue = -INF;

            if (dis(rnd) < 0.02) {
                // Exploration: find random plain with unknown type (might be a
                // general)
                std::vector<Coord> unknownPlains;
                for (pos_t i = 1; i <= height; ++i) {
                    for (pos_t j = 1; j <= width; ++j) {
                        if (blockType[i][j] == 0 && !knownBlockType[i][j] &&
                            dist[i][j] < 500) {
                            unknownPlains.emplace_back(i, j);
                        }
                    }
                }
                if (!unknownPlains.empty()) {
                    std::uniform_int_distribution<size_t> randIndex(
                        0, unknownPlains.size() - 1);
                    targetPos = unknownPlains[randIndex(rnd)];
                    maxBlockValue = 1e9;
                }
            }

            for (pos_t i = 1; i <= height; ++i) {
                for (pos_t j = 1; j <= width; ++j) {
                    const auto& tile = board.tileAt(i, j);
                    if (tile.occupier != id && dist[i][j] < 500 &&
                        !isImpassableTile(tile.type)) {
                        value_t blockValue = blockTypeValue[blockType[i][j]] +
                                             eval[i][j] / 5 -
                                             (dist[i][j] + army[i][j] / 2);
                        // v6 is classic mode (gameMode=0): prioritize targets
                        // near own general
                        if (seenGeneral[id].x != -1) {
                            blockValue -=
                                approxDist(Coord(i, j), seenGeneral[id]) * 5LL;
                        }
                        if (blockValue > maxBlockValue) {
                            maxBlockValue = blockValue;
                            targetPos = Coord(i, j);
                        }
                    }
                }
            }

            pos_t x = prevTarget.x, y = prevTarget.y;
            if (x != -1 && board.tileAt(x, y).occupier != id &&
                !isImpassableTile(board.tileAt(x, y).type)) {
                value_t prevBlockValue = blockTypeValue[blockType[x][y]] +
                                         eval[x][y] / 5 -
                                         (dist[x][y] + army[x][y] / 2);
                // v6 is classic mode (gameMode=0): prioritize targets near own
                // general
                if (seenGeneral[id].x != -1) {
                    prevBlockValue -=
                        approxDist(Coord(x, y), seenGeneral[id]) * 5LL;
                }
                if (std::abs(prevBlockValue - maxBlockValue) < 25) {
                    targetPos = prevTarget;
                }
            }
        } else {
            targetPos = prevTarget;
        }
        prevTarget = targetPos;

        if (blockType[coo.x][coo.y] == 1 || dis(rnd) > 0.07 ||
            eval[targetPos.x][targetPos.y] > 150) {
            Coord nextPos = moveTowards(coo, targetPos);
            lastPos = nextPos;
            moveQueue.emplace_back(MoveType::MOVE_ARMY, coo, nextPos, false);
            return;
        }

        evaluateRouteCosts(targetPos);

        // Find own cell with maximum weight
        value_t maxWeight = -INF;
        Coord newFocus(-1, -1);
        for (pos_t i = 1; i <= height; ++i) {
            for (pos_t j = 1; j <= width; ++j) {
                const auto& tile = board.tileAt(i, j);
                if (tile.occupier == id && army[i][j] > 1) {
                    value_t weight = eval[i][j] - 30 * dist[i][j];
                    if (blockType[i][j] == 3) weight -= weight / 4;
                    if (weight > maxWeight) {
                        maxWeight = weight;
                        newFocus = Coord(i, j);
                    }
                }
            }
        }

        if (newFocus == Coord(-1, -1)) {
            lastPos = coo;
            moveQueue.emplace_back(MoveType::MOVE_ARMY, coo, coo, false);
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
