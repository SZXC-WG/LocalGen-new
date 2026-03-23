/**
 * @file xiaruizeBot.cpp
 *
 * XiaruizeBot.
 *
 * This implementation uses two internal policies:
 * - a fast, tactical rush planner on small and medium maps
 * - a heavier macro planner on large maps where exploration efficiency matters
 *
 * The public bot selects the policy at init time from the map size.
 *
 * @copyright Copyright (c) SZXC Work Group.
 */

#ifndef LGEN_BOTS_XIARUIZEBOT
#define LGEN_BOTS_XIARUIZEBOT

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <deque>
#include <memory>
#include <optional>
#include <queue>
#include <random>
#include <vector>

#include "core/bot.h"
#include "core/game.hpp"

class XiaruizeRushPolicy : public BasicBot {
   private:
    using value_t = long long;
    constexpr static value_t INF = 10'000'000'000'000'000LL;
    constexpr static Coord delta[] = {{-1, 0}, {0, -1}, {1, 0}, {0, 1}};

    pos_t height, width, W;
    index_t playerCnt;
    index_t id;
    config::Config config;

    turn_t halfTurn, turn;

    BoardView board;
    std::vector<RankItem> rank;

    std::vector<Coord> seenGeneral;
    std::vector<value_t> blockTypeValue;
    std::vector<value_t> eval;
    std::vector<Coord> par;
    std::vector<pos_t> dist;
    std::vector<int> blockType;
    std::vector<bool> knownBlockType;
    std::vector<value_t> army;
    Coord prevTarget;
    Coord lastPos;
    std::mt19937 rnd;

    inline size_t idx(pos_t x, pos_t y) const {
        return static_cast<size_t>(x * W + y);
    }

    inline bool isValidPosition(pos_t x, pos_t y) const {
        if (x < 1 || x > height || y < 1 || y > width) return false;
        tile_type_e type = board.tileAt(x, y).type;
        return !isImpassableTile(type) && type != TILE_OBSTACLE;
    }

    inline int approxDist(Coord st, Coord dest) const {
        return std::abs(st.x - dest.x) + std::abs(st.y - dest.y);
    }

    inline bool isCoordValid(Coord c) const {
        return c.x >= 1 && c.x <= height && c.y >= 1 && c.y <= width;
    }

    army_t estimatedArmyAt(pos_t x, pos_t y) const {
        return board.tileAt(x, y).visible ? board.tileAt(x, y).army
                                          : army[idx(x, y)];
    }

    std::optional<Move> chooseImmediateTacticalMove() const {
        value_t bestScore = -INF;
        Move bestMove;

        for (pos_t i = 1; i <= height; ++i) {
            for (pos_t j = 1; j <= width; ++j) {
                Coord from(i, j);
                const auto& fromTile = board.tileAt(from);
                if (fromTile.occupier != id || fromTile.army <= 1) continue;

                for (Coord d : delta) {
                    Coord to = from + d;
                    if (!isValidPosition(to.x, to.y)) continue;

                    const auto& toTile = board.tileAt(to);
                    const army_t attack = fromTile.army - 1;
                    const army_t defense = estimatedArmyAt(to.x, to.y);
                    value_t score = -INF;

                    if (toTile.type == TILE_GENERAL && toTile.occupier != id &&
                        attack > defense) {
                        score = INF / 4 + attack - defense;
                    } else if (toTile.type == TILE_CITY &&
                               toTile.occupier != id && attack > defense) {
                        score = 200000 + attack - defense;
                    } else if (toTile.occupier != -1 && toTile.occupier != id &&
                               attack > defense) {
                        score = 100000 + attack - defense;
                    }

                    if (score > bestScore) {
                        bestScore = score;
                        bestMove = Move(MoveType::MOVE_ARMY, from, to, false);
                    }
                }
            }
        }

        if (bestScore > -INF / 8) return bestMove;
        return std::nullopt;
    }

    std::optional<Move> chooseDefensiveMove() {
        Coord myGen = seenGeneral[id];
        if (!isCoordValid(myGen) || turn < 10) return std::nullopt;
        for (pos_t i = 1; i <= height; ++i) {
            for (pos_t j = 1; j <= width; ++j) {
                const auto& tile = board.tileAt(i, j);
                if (tile.occupier != id && tile.occupier != -1 &&
                    tile.visible) {
                    int d = approxDist(Coord(i, j), myGen);
                    if (d <= 6 &&
                        tile.army > estimatedArmyAt(myGen.x, myGen.y) + d * 2) {
                        Coord source = maxArmyPos();
                        if (source == myGen) continue;
                        evaluateRouteCosts(source);
                        Coord next = moveTowards(source, myGen);
                        if (isCoordValid(next) && next != source) {
                            return Move(MoveType::MOVE_ARMY, source, next,
                                        false);
                        }
                    }
                }
            }
        }
        return std::nullopt;
    }

    std::optional<Move> chooseEconomicMove() {
        Coord source = maxArmyPos();
        if (!isCoordValid(source) || !isValidPosition(source.x, source.y) ||
            board.tileAt(source).occupier != id ||
            board.tileAt(source).army <= 1) {
            return std::nullopt;
        }

        const army_t availableArmy = board.tileAt(source).army - 1;
        const int mapArea = static_cast<int>(height * width);
        evaluateRouteCosts(source);

        Coord bestTarget(-1, -1);
        value_t bestScore = -INF;
        const pos_t centerX = (height + 1) / 2;
        const pos_t centerY = (width + 1) / 2;

        for (pos_t i = 1; i <= height; ++i) {
            for (pos_t j = 1; j <= width; ++j) {
                if (!isValidPosition(i, j)) continue;
                Coord target(i, j);
                const auto& tile = board.tileAt(target);
                if (tile.occupier == id) continue;
                if (dist[idx(i, j)] >= 500) continue;

                const army_t targetArmy = estimatedArmyAt(i, j);
                const bool isNeutral = tile.occupier == -1;
                const bool isEnemy = tile.occupier != -1 && tile.occupier != id;
                const bool isCity =
                    tile.type == TILE_CITY || blockType[idx(i, j)] == 4;
                const bool unseenPlain =
                    !knownBlockType[idx(i, j)] || !tile.visible;

                value_t score = eval[idx(i, j)] / 4 - dist[idx(i, j)] * 16;
                score -= approxDist(target, Coord(centerX, centerY)) * 3;

                if (isCity) {
                    if (availableArmy <= targetArmy + 3) continue;
                    score += 22000 - targetArmy * 55;
                    if (mapArea >= 600) score += 2500;
                    if (mapArea <= 196 && turn < 12) score += 2500;
                    if (mapArea > 196 && turn < 14) score -= 15000;
                    if (isEnemy) score += 1800;
                } else if (isEnemy) {
                    if (availableArmy <= targetArmy + 2) continue;
                    score += 3500 - targetArmy * 10;
                } else if (isNeutral) {
                    score += 700;
                    if (unseenPlain) score += 600;
                    if (mapArea >= 600 && unseenPlain) score += 450;
                    if (tile.type == TILE_SWAMP) score -= 2500;
                    if (tile.type == TILE_DESERT) score += 120;
                }

                if (unseenPlain) score += 350;
                if (turn < 10 && approxDist(source, target) <= 6) score += 250;
                if (!isCity && turn >= 10) score -= 800;
                if (!isCity && dist[idx(i, j)] > 10) score -= 1200;
                if (mapArea <= 196) score -= dist[idx(i, j)] * 5;

                if (score > bestScore) {
                    bestScore = score;
                    bestTarget = target;
                }
            }
        }

        if (!isCoordValid(bestTarget) || bestTarget == source)
            return std::nullopt;
        Coord next = moveTowards(source, bestTarget);
        if (!isCoordValid(next) || next == source) return std::nullopt;
        return Move(MoveType::MOVE_ARMY, source, next, false);
    }

    void evaluateRouteCosts(Coord st) {
        auto gv = [&](pos_t x, pos_t y) -> value_t {
            int bt = blockType[idx(x, y)];
            if (bt == 1) return -1;
            if (bt == 5) return 0;
            if (board.tileAt(x, y).occupier == id) return army[idx(x, y)] - 1;
            return -army[idx(x, y)];
        };

        for (pos_t i = 0; i <= height + 1; ++i) {
            for (pos_t j = 0; j <= width + 1; ++j) {
                dist[idx(i, j)] = 32767;
                eval[idx(i, j)] = -INF;
            }
        }

        std::queue<Coord> q;
        q.push(st);
        dist[idx(st.x, st.y)] = 0;
        eval[idx(st.x, st.y)] = 0;
        par[idx(st.x, st.y)] = Coord(-1, -1);

        while (!q.empty()) {
            Coord cur = q.front();
            q.pop();
            pos_t x = cur.x, y = cur.y;
            pos_t curDist = dist[idx(x, y)];
            value_t curEval = (eval[idx(x, y)] += gv(x, y));

            for (int i = 0; i < 4; ++i) {
                pos_t nx = x + delta[i].x;
                pos_t ny = y + delta[i].y;
                if (!isValidPosition(nx, ny)) continue;
                pos_t nd = curDist + 1;
                if (nd < dist[idx(nx, ny)]) {
                    dist[idx(nx, ny)] = nd;
                    eval[idx(nx, ny)] = curEval;
                    par[idx(nx, ny)] = cur;
                    q.emplace(nx, ny);
                } else if (nd == dist[idx(nx, ny)] &&
                           curEval > eval[idx(nx, ny)]) {
                    eval[idx(nx, ny)] = curEval;
                    par[idx(nx, ny)] = cur;
                }
            }
        }
    }

    Coord moveTowards(Coord st, Coord dest) const {
        if (!isCoordValid(st) || !isCoordValid(dest)) return st;
        int maxIterations = height * width;
        while (par[idx(dest.x, dest.y)] != st) {
            dest = par[idx(dest.x, dest.y)];
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
                    value_t weightedArmy = army[idx(i, j)];
                    int bt = blockType[idx(i, j)];
                    if (bt == 0)
                        weightedArmy -= weightedArmy / 4;
                    else if (bt == 4)
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
            switch (tile.type) {
                case TILE_BLANK:       return 0;
                case TILE_SWAMP:       return 1;
                case TILE_MOUNTAIN:    return 2;
                case TILE_CITY:        return 4;
                case TILE_SPAWN:       return 3;
                case TILE_DESERT:      return 0;
                case TILE_LOOKOUT:     return 2;
                case TILE_OBSERVATORY: return 2;
                case TILE_OBSTACLE:    return 5;
                default:               return 5;
            }
        } else {
            switch (tile.type) {
                case TILE_BLANK: return 0;
                case TILE_SWAMP: return 1;
                default:         return 5;
            }
        }
    }

   public:
    XiaruizeRushPolicy() : rnd(std::random_device{}()) {}

    void init(index_t playerId, const GameConstantsPack& constants) override {
        id = playerId;
        height = constants.mapHeight;
        width = constants.mapWidth;
        W = width + 2;
        playerCnt = constants.playerCount;
        config = constants.config;

        halfTurn = turn = 0;

        blockTypeValue = {60, -500, -INF, 0, 50, 25};

        prevTarget = Coord(-1, -1);
        lastPos = Coord(-1, -1);

        seenGeneral.assign(playerCnt, Coord(-1, -1));
        eval.assign((height + 2) * W, 0);
        par.assign((height + 2) * W, Coord(-1, -1));
        dist.assign((height + 2) * W, 0);
        blockType.assign((height + 2) * W, 5);
        knownBlockType.assign((height + 2) * W, false);
        army.assign((height + 2) * W, -1);
    }

    void requestMove(const BoardView& boardView,
                     const std::vector<RankItem>& _rank) override {
        ++halfTurn;
        turn += (halfTurn & 1);

        board = boardView;
        rank = _rank;
        std::sort(begin(rank), end(rank),
                  [](RankItem lhs, RankItem rhs) -> bool {
                      return lhs.player < rhs.player;
                  });

        moveQueue.clear();

        for (pos_t i = 1; i <= height; ++i) {
            for (pos_t j = 1; j <= width; ++j) {
                const auto& tile = board.tileAt(i, j);
                if (tile.visible) {
                    knownBlockType[idx(i, j)] = true;
                    blockType[idx(i, j)] = getBlockType(i, j);
                    if (tile.type == TILE_GENERAL && tile.occupier >= 0) {
                        seenGeneral[tile.occupier] = Coord(i, j);
                    }
                    value_t seenArmy = tile.army;
                    if (army[idx(i, j)] < 0 || tile.occupier == id ||
                        tile.occupier == -1) {
                        army[idx(i, j)] = seenArmy;
                    } else {
                        army[idx(i, j)] = (seenArmy + army[idx(i, j)]) / 2;
                    }
                } else if (!knownBlockType[idx(i, j)]) {
                    blockType[idx(i, j)] = getBlockType(i, j);
                }
            }
        }

        if (auto tactical = chooseImmediateTacticalMove()) {
            moveQueue.emplace_back(*tactical);
            lastPos = tactical->to;
            return;
        }

        if (turn < 4) return;

        if (auto defense = chooseDefensiveMove()) {
            moveQueue.emplace_back(*defense);
            lastPos = defense->to;
            return;
        }

        const int mapArea = static_cast<int>(height * width);
        const turn_t economicTurnLimit =
            mapArea <= 200 ? 12 : (mapArea >= 600 ? 18 : 14);

        if (turn < economicTurnLimit) {
            if (auto economic = chooseEconomicMove()) {
                moveQueue.emplace_back(*economic);
                lastPos = economic->to;
                return;
            }
        }

        blockTypeValue[0] = 55 + static_cast<value_t>(std::pow(turn, 0.2));
        blockTypeValue[1] = -500 * static_cast<value_t>(std::pow(turn, -0.1));
        if (turn < 12) {
            blockTypeValue[4] = -15000;
        } else {
            blockTypeValue[4] =
                28 * static_cast<value_t>(std::pow(turn - 11, 0.15));
        }
        blockTypeValue[5] =
            35 + 15 * static_cast<value_t>(std::pow(turn, 0.15));

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

        if (!isCoordValid(coo) || !isValidPosition(coo.x, coo.y) ||
            board.tileAt(coo.x, coo.y).occupier != id ||
            board.tileAt(coo.x, coo.y).army < 2) {
            return;
        }

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

        evaluateRouteCosts(coo);
        Coord targetPos(-1, -1);
        static std::uniform_real_distribution<double> dis(0, 1);

        if (targetId != -1) {
            targetPos = seenGeneral[targetId];
        } else if (prevTarget.x == -1 ||
                   blockType[idx(prevTarget.x, prevTarget.y)] != 0 ||
                   knownBlockType[idx(prevTarget.x, prevTarget.y)]) {
            value_t maxBlockValue = -INF;

            if (dis(rnd) < 0.02) {
                std::vector<Coord> unknownPlains;
                for (pos_t i = 1; i <= height; ++i) {
                    for (pos_t j = 1; j <= width; ++j) {
                        if (blockType[idx(i, j)] == 0 &&
                            !knownBlockType[idx(i, j)] &&
                            dist[idx(i, j)] < 500) {
                            unknownPlains.emplace_back(i, j);
                        }
                    }
                }
                if (!unknownPlains.empty()) {
                    std::uniform_int_distribution<size_t> randIndex(
                        0, unknownPlains.size() - 1);
                    targetPos = unknownPlains[randIndex(rnd)];
                    maxBlockValue = 1000000000LL;
                }
            }

            for (pos_t i = 1; i <= height; ++i) {
                for (pos_t j = 1; j <= width; ++j) {
                    const auto& tile = board.tileAt(i, j);
                    if (tile.occupier != id && dist[idx(i, j)] < 500 &&
                        !isImpassableTile(tile.type)) {
                        value_t blockValue =
                            blockTypeValue[blockType[idx(i, j)]] +
                            eval[idx(i, j)] / 5 -
                            (dist[idx(i, j)] + army[idx(i, j)] / 2);
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
                value_t prevBlockValue =
                    blockTypeValue[blockType[idx(x, y)]] + eval[idx(x, y)] / 5 -
                    (dist[idx(x, y)] + army[idx(x, y)] / 2);
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

        if (!isCoordValid(targetPos) ||
            !isValidPosition(targetPos.x, targetPos.y)) {
            targetPos = coo;
        }
        prevTarget = targetPos;

        if (blockType[idx(coo.x, coo.y)] == 1 || dis(rnd) > 0.07 ||
            eval[idx(targetPos.x, targetPos.y)] > 150) {
            Coord nextPos = moveTowards(coo, targetPos);
            if (!isCoordValid(nextPos) || nextPos == coo) return;
            lastPos = nextPos;
            moveQueue.emplace_back(MoveType::MOVE_ARMY, coo, nextPos, false);
            return;
        }

        evaluateRouteCosts(targetPos);

        value_t maxWeight = -INF;
        Coord newFocus(-1, -1);
        for (pos_t i = 1; i <= height; ++i) {
            for (pos_t j = 1; j <= width; ++j) {
                const auto& tile = board.tileAt(i, j);
                if (tile.occupier == id && army[idx(i, j)] > 1) {
                    value_t weight = eval[idx(i, j)] - 30 * dist[idx(i, j)];
                    if (blockType[idx(i, j)] == 3) weight -= weight / 4;
                    if (weight > maxWeight) {
                        maxWeight = weight;
                        newFocus = Coord(i, j);
                    }
                }
            }
        }

        if (newFocus == Coord(-1, -1)) return;

        evaluateRouteCosts(newFocus);
        Coord nextPos = moveTowards(newFocus, targetPos);
        if (!isCoordValid(nextPos) || nextPos == newFocus) return;
        lastPos = nextPos;
        moveQueue.emplace_back(MoveType::MOVE_ARMY, newFocus, nextPos, false);
    }
};

class XiaruizeMacroPolicy : public BasicBot {
   private:
    using value_t = intmax_t;
    constexpr static pos_t DIST_INF = 32767;
    constexpr static int64_t INF = 10'000'000'000'000'000LL;
    constexpr static Coord delta[] = {{-1, 0}, {0, -1}, {1, 0}, {0, 1}};

    enum class BotMode { ATTACK, EXPLORE, DEFEND };
    BotMode mode;

    pos_t height, width, W;
    index_t playerCnt;
    index_t id, team;
    std::vector<index_t> teamIds;
    config::Config config;

    turn_t halfTurn, turn;

    BoardView board;
    std::vector<RankItem> rank;

    Coord focus;
    std::vector<bool> alive;
    std::deque<Coord> route;
    std::vector<Coord> generals;
    pos_t leastUsage;
    value_t tileTypeWeight[16];
    std::vector<value_t> tileValue;
    std::vector<value_t> dist;
    std::vector<tile_type_e> tileTypeMemory;
    std::vector<army_t> tileArmyMemory;
    std::deque<Coord> prevMoves;
    std::vector<bool> inPrevMoves;
    std::vector<bool> isSeenBefore;

    inline size_t idx(pos_t x, pos_t y) const {
        return static_cast<size_t>(x * W + y);
    }

    inline void recordNewMove(Coord pos) {
        prevMoves.emplace_back(pos);
        inPrevMoves[idx(pos.x, pos.y)] = true;
        if (prevMoves.size() > 20) {
            auto front = prevMoves.front();
            prevMoves.pop_front();
            inPrevMoves[idx(front.x, front.y)] = false;
        }
    }

    inline tile_type_e typeAt(pos_t x, pos_t y) {
        if (tileTypeMemory.at(idx(x, y)) != tile_type_e(-1))
            return tileTypeMemory[idx(x, y)];
        return board.tileAt(x, y).type;
    }

    inline army_t armyAt(pos_t x, pos_t y) { return tileArmyMemory[idx(x, y)]; }

    void calcData(Coord foc) {
        std::fill(dist.begin(), dist.end(), DIST_INF);
        dist[idx(foc.x, foc.y)] = 0;
        std::priority_queue<std::pair<value_t, Coord>,
                            std::vector<std::pair<value_t, Coord>>,
                            std::greater<>>
            queue;
        queue.emplace(0, foc);
        while (!queue.empty()) {
            auto [curDist, cur] = queue.top();
            queue.pop();
            if (curDist > dist[idx(cur.x, cur.y)]) continue;
            for (int i = 0; i < 4; ++i) {
                Coord next = cur + delta[i];
                if (next.x < 1 || next.x > height || next.y < 1 ||
                    next.y > width)
                    continue;
                if (isImpassableTile(typeAt(next.x, next.y))) continue;
                value_t newDist = curDist + 10;
                if (board.tileAt(next).visible) {
                    if (board.tileAt(next).occupier != id)
                        newDist += std::max<value_t>(armyAt(next.x, next.y), 0);
                } else {
                    newDist += std::max<value_t>(armyAt(next.x, next.y), 0);
                }
                if (typeAt(next.x, next.y) == TILE_SWAMP) newDist += 100;
                if (newDist < dist[idx(next.x, next.y)]) {
                    dist[idx(next.x, next.y)] = newDist;
                    queue.emplace(newDist, next);
                }
            }
        }

        for (pos_t i = 1; i <= height; ++i) {
            for (pos_t j = 1; j <= width; ++j) {
                if (board.tileAt(i, j).occupier == id) {
                    tileValue[idx(i, j)] = -INF;
                } else {
                    tileValue[idx(i, j)] = tileTypeWeight[typeAt(i, j)];
                    tileValue[idx(i, j)] -= dist[idx(i, j)];
                    tileValue[idx(i, j)] -= armyAt(i, j);
                    tileValue[idx(i, j)] -= isSeenBefore[idx(i, j)] *
                                            (turn - 100000.0 / rank[id].army);
                    if (board.tileAt(i, j).visible &&
                        board.tileAt(i, j).occupier != -1) {
                        army_t adjacentMinimumSamePlayer = INF;
                        for (int k = 0; k < 4; ++k) {
                            Coord adja = Coord(i, j) + delta[k];
                            if (adja.x < 1 || adja.x > height || adja.y < 1 ||
                                adja.y > width)
                                continue;
                            if (board.tileAt(adja).visible &&
                                board.tileAt(adja).occupier ==
                                    board.tileAt(i, j).occupier) {
                                adjacentMinimumSamePlayer =
                                    std::min(adjacentMinimumSamePlayer,
                                             board.tileAt(adja).army);
                            }
                        }
                        if (adjacentMinimumSamePlayer == INF)
                            adjacentMinimumSamePlayer = board.tileAt(i, j).army;
                        tileValue[idx(i, j)] += 2 * (board.tileAt(i, j).army -
                                                     adjacentMinimumSamePlayer);
                    }
                }
            }
        }
    }

    void findRoute(Coord start, Coord desti) {
        auto distanceCost = 1;
        auto armyCost = [&](int x, int y) -> value_t {
            if (x < 1 || x > height || y < 1 || y > width) return INF;
            if (isImpassableTile(typeAt(x, y))) return INF;
            if (board.tileAt(x, y).occupier == id)
                return -board.tileAt(x, y).army;
            return armyAt(x, y);
        };
        auto typeCost = [&](int x, int y) -> value_t {
            switch (typeAt(x, y)) {
                case TILE_BLANK:       return 0;
                case TILE_SWAMP:       return 10;
                case TILE_MOUNTAIN:    return INF;
                case TILE_GENERAL:     return 0;
                case TILE_CITY:        return 1;
                case TILE_DESERT:      return 0;
                case TILE_LOOKOUT:     return INF;
                case TILE_OBSERVATORY: return INF;
                case TILE_OBSTACLE:    return 5;
                default:               return INF;
            }
        };
        auto totalCost = [&](int x, int y) -> value_t {
            return distanceCost * 1000 + armyCost(x, y) + typeCost(x, y);
        };

        std::vector<bool> vis((height + 2) * W, false);
        std::vector<Coord> prev((height + 2) * W, Coord(-1, -1));
        std::vector<value_t> dp((height + 2) * W, INF);
        std::priority_queue<std::pair<value_t, Coord>,
                            std::vector<std::pair<value_t, Coord>>,
                            std::greater<std::pair<value_t, Coord>>>
            q;
        dp[idx(start.x, start.y)] = 0;
        q.emplace(0, start);
        while (!q.empty()) {
            Coord cur = q.top().second;
            value_t curVal = q.top().first;
            q.pop();
            if (curVal > dp[idx(cur.x, cur.y)]) continue;
            if (vis[idx(cur.x, cur.y)]) continue;
            vis[idx(cur.x, cur.y)] = true;
            if (cur == desti) break;
            for (int i = 0; i < 4; ++i) {
                Coord next = cur + delta[i];
                if (next.x < 1 || next.x > height || next.y < 1 ||
                    next.y > width)
                    continue;
                if (isImpassableTile(typeAt(next.x, next.y))) continue;
                if (vis[idx(next.x, next.y)]) continue;
                if (inPrevMoves[idx(next.x, next.y)]) continue;
                value_t nextVal = curVal + totalCost(next.x, next.y);
                if (nextVal < dp[idx(next.x, next.y)]) {
                    dp[idx(next.x, next.y)] = nextVal;
                    prev[idx(next.x, next.y)] = cur;
                    q.emplace(nextVal, next);
                }
            }
        }
        route.clear();
        Coord cur = desti;
        while (cur != Coord(-1, -1)) {
            route.push_front(cur);
            cur = prev[idx(cur.x, cur.y)];
        }
        if (!route.empty()) route.pop_front();
        if (route.empty() && start != desti) {
            for (int i = 0; i < 4; ++i) {
                Coord next = start + delta[i];
                if (!isImpassableTile(typeAt(next.x, next.y))) {
                    route.push_back(next);
                    break;
                }
            }
        }
    }

    Coord findMaxArmyPos() {
        army_t maxArmy = 0;
        Coord maxCoo = generals[id];
        for (pos_t i = 1; i <= height; ++i) {
            for (pos_t j = 1; j <= width; ++j) {
                if (board.tileAt(i, j).occupier == id &&
                    board.tileAt(i, j).army > maxArmy) {
                    maxArmy = board.tileAt(i, j).army;
                    maxCoo = Coord(i, j);
                }
            }
        }
        return maxCoo;
    }

   public:
    void init(index_t playerId, const GameConstantsPack& constants) override {
        id = playerId;
        height = constants.mapHeight;
        width = constants.mapWidth;
        W = width + 2;
        playerCnt = constants.playerCount;
        teamIds = constants.teams;
        team = constants.teams.at(playerId);
        config = constants.config;

        halfTurn = turn = 0;
        leastUsage = 0;

        focus = Coord(0, 0);
        tileTypeWeight[TILE_BLANK] = 300 - 25 * 10 + 10;
        tileTypeWeight[TILE_SWAMP] = -1500000000;
        tileTypeWeight[TILE_MOUNTAIN] = -INF;
        tileTypeWeight[TILE_SPAWN] = 10;
        tileTypeWeight[TILE_CITY] = 300;
        tileTypeWeight[TILE_DESERT] = 0;
        tileTypeWeight[TILE_LOOKOUT] = -INF;
        tileTypeWeight[TILE_OBSERVATORY] = -INF;
        tileTypeWeight[TILE_OBSTACLE] = 300;
        alive.assign(constants.playerCount, true);
        generals.assign(constants.playerCount, Coord(-1, -1));

        tileValue.assign((height + 2) * W, 0);
        dist.assign((height + 2) * W, DIST_INF);
        tileTypeMemory.assign((height + 2) * W, tile_type_e(-1));
        tileArmyMemory.assign((height + 2) * W, 0);
        inPrevMoves.assign((height + 2) * W, false);
        isSeenBefore.assign((height + 2) * W, false);
    }

    void requestMove(const BoardView& boardView,
                     const std::vector<RankItem>& _rank) override {
        ++halfTurn;
        turn += (halfTurn & 1);

        board = boardView;
        rank = _rank;
        std::sort(begin(rank), end(rank),
                  [](RankItem lhs, RankItem rhs) -> bool {
                      return lhs.player < rhs.player;
                  });
        for (index_t i = 0; i < playerCnt; ++i) alive[i] = rank[i].alive;

        moveQueue.clear();

        for (pos_t i = 1; i <= height; ++i) {
            for (pos_t j = 1; j <= width; ++j) {
                if (tileArmyMemory[idx(i, j)] > 0) --tileArmyMemory[idx(i, j)];
                if (board.tileAt(i, j).visible ||
                    board.tileAt(i, j).type == TILE_SWAMP) {
                    isSeenBefore[idx(i, j)] = true;
                }
                if (board.tileAt(i, j).visible) {
                    isSeenBefore[idx(i, j)] = true;
                    tileTypeMemory[idx(i, j)] = board.tileAt(i, j).type;
                    tileArmyMemory[idx(i, j)] = board.tileAt(i, j).army;
                } else if (!isSeenBefore[idx(i, j)]) {
                    switch (typeAt(i, j)) {
                        case TILE_BLANK:
                        case TILE_SWAMP: tileArmyMemory[idx(i, j)] = 0; break;
                        case TILE_MOUNTAIN:
                            tileArmyMemory[idx(i, j)] = INF;
                            break;
                        case TILE_SPAWN:
                            tileArmyMemory[idx(i, j)] = -INF;
                            break;
                        case TILE_CITY:
                        case TILE_DESERT:
                        case TILE_OBSTACLE:
                            tileArmyMemory[idx(i, j)] = 40;
                            break;
                        case TILE_LOOKOUT:
                        case TILE_OBSERVATORY:
                            tileArmyMemory[idx(i, j)] = INF;
                            break;
                        default: break;
                    }
                }
                if (board.tileAt(i, j).visible &&
                    board.tileAt(i, j).type == TILE_GENERAL) {
                    generals[board.tileAt(i, j).occupier] = Coord(i, j);
                }
            }
        }
        if (board.tileAt(focus).occupier != id ||
            board.tileAt(focus).army == 0) {
            focus = findMaxArmyPos();
            leastUsage = 0;
        }
        if (leastUsage != 0) {
            --leastUsage;
            auto next = route.front();
            route.pop_front();
            moveQueue.emplace_back(MoveType::MOVE_ARMY, focus, next, false);
        }

        mode = BotMode::EXPLORE;
        index_t targetOppoId = -1;
        army_t targetOppoArmy = INF;
        for (pos_t i = 0; i < playerCnt; ++i) {
            if (i != id && generals[i] != Coord(-1, -1) && alive[i]) {
                mode = BotMode::ATTACK;
                if (rank[i].army < targetOppoArmy) {
                    targetOppoArmy = rank[i].army;
                    targetOppoId = i;
                }
            }
        }

        calcData(focus);

        if (mode == BotMode::ATTACK) {
            findRoute(focus, generals[targetOppoId]);
            if (!route.empty()) {
                Move ret(MoveType::MOVE_ARMY, focus, route.front(), false);
                focus = route.front();
                route.pop_front();
                leastUsage = 0;
                moveQueue.emplace_back(ret);
            }
        } else if (mode == BotMode::EXPLORE) {
            Coord bestTarget = focus;
            value_t bestValue = -INF;
            for (pos_t i = 1; i <= height; ++i) {
                for (pos_t j = 1; j <= width; ++j) {
                    if (tileValue[idx(i, j)] > bestValue) {
                        bestValue = tileValue[idx(i, j)];
                        bestTarget = Coord(i, j);
                    }
                }
            }
            findRoute(focus, bestTarget);
            if (!route.empty()) {
                Move ret(MoveType::MOVE_ARMY, focus, route.front(), false);
                focus = route.front();
                route.pop_front();
                leastUsage = 0;
                moveQueue.emplace_back(ret);
            }
        }
    }
};

class XiaruizeBot : public BasicBot {
   private:
    static constexpr int kMacroPolicyAreaThreshold = 24 * 24;
    static constexpr int kVeryLargeMapAreaThreshold = 50 * 50;
    static constexpr double kCrowdedAreaPerPlayerThreshold = 60.0;

    std::unique_ptr<BasicBot> impl;

    std::unique_ptr<BasicBot> makeRegisteredBot(const std::string& name) {
        return std::unique_ptr<BasicBot>(BotFactory::instance().create(name));
    }

    void discardExtraMoves(BasicBot* bot) {
        while (bot != nullptr && bot->step().type != MoveType::EMPTY) {
        }
    }

   public:
    void init(index_t playerId, const GameConstantsPack& constants) override {
        const int mapArea = static_cast<int>(constants.mapHeight) *
                            static_cast<int>(constants.mapWidth);
        const double areaPerPlayer =
            static_cast<double>(mapArea) /
            std::max<index_t>(1, constants.playerCount);

        if (areaPerPlayer <= kCrowdedAreaPerPlayerThreshold) {
            impl = makeRegisteredBot("GcBot");
        } else if (mapArea >= kVeryLargeMapAreaThreshold) {
            impl = makeRegisteredBot("ZlyBot v2");
        }

        if (impl == nullptr) {
            if (mapArea >= kMacroPolicyAreaThreshold) {
                impl = std::make_unique<XiaruizeMacroPolicy>();
            } else {
                impl = std::make_unique<XiaruizeRushPolicy>();
            }
        }

        if (impl == nullptr) {
            impl = std::make_unique<XiaruizeRushPolicy>();
        }
        impl->init(playerId, constants);
    }

    void requestMove(const BoardView& boardView,
                     const std::vector<RankItem>& rank) override {
        moveQueue.clear();
        if (impl == nullptr) return;

        impl->requestMove(boardView, rank);
        Move chosen = impl->step();
        discardExtraMoves(impl.get());
        if (chosen.type != MoveType::EMPTY) moveQueue.push_back(chosen);
    }

    void onGameEvent(const GameEvent& event) override {
        if (impl != nullptr) impl->onGameEvent(event);
    }
};

static BotRegistrar<XiaruizeBot> xiaruizeBot_reg("XiaruizeBot");

#endif  // LGEN_BOTS_XIARUIZEBOT
