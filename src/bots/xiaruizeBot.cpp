/**
 * @file xiaruizeBot.cpp
 *
 * XiaruizeBot.
 * Author: xiaruize0911
 *
 * Original implementation: tactical capture search + focused-stack planning
 * with threat-aware defense, weighted pathfinding, and gathering.
 *
 * @copyright Copyright (c) SZXC Work Group.
 */

#ifndef LGEN_BOTS_XIARUIZEBOT
#define LGEN_BOTS_XIARUIZEBOT

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <queue>
#include <random>
#include <vector>

#include "core/bot.h"
#include "core/game.hpp"

class XiaruizeBot : public BasicBot {
   private:
    using score_t = long long;
    static constexpr score_t kInf = (1LL << 60);
    static constexpr score_t kNegInf = -(1LL << 60);
    static constexpr std::array<Coord, 4> kDirs = {Coord{-1, 0}, Coord{0, -1},
                                                   Coord{1, 0}, Coord{0, 1}};

    struct MemoryCell {
        tile_type_e terrain = TILE_BLANK;
        index_t occupier = -1;
        army_t army = 0;
        bool seen = false;
        bool visible = false;
        int lastSeenTurn = -1;
    };

    struct PathMap {
        std::vector<score_t> dist;
        std::vector<Coord> parent;
    };

    struct Candidate {
        Move move{};
        score_t score = kNegInf;
    };

    struct ThreatInfo {
        bool present = false;
        Coord source{-1, -1};
        army_t army = 0;
        int dist = 1e9;
    };

    struct HeapNode {
        score_t dist = 0;
        Coord coord{0, 0};

        bool operator<(const HeapNode& other) const {
            return dist > other.dist;
        }
    };

    pos_t height = 0;
    pos_t width = 0;
    pos_t stride = 0;
    index_t id = -1;
    index_t playerCount = 0;
    turn_t halfTurn = 0;
    turn_t fullTurn = 0;

    std::vector<index_t> teams;
    BoardView board;
    std::vector<RankItem> rankById;
    std::vector<MemoryCell> memory;
    std::vector<Coord> knownGenerals;

    Coord myGeneral{-1, -1};
    Coord focusCell{-1, -1};
    Coord macroTarget{-1, -1};
    Coord lastMoveFrom{-1, -1};
    Coord lastMoveTo{-1, -1};
    int macroTargetLockUntil = -1;

    std::mt19937 rng{std::random_device{}()};

    inline size_t idx(pos_t x, pos_t y) const {
        return static_cast<size_t>(x * stride + y);
    }
    inline size_t idx(Coord c) const { return idx(c.x, c.y); }

    inline bool inside(Coord c) const {
        return c.x >= 1 && c.x <= height && c.y >= 1 && c.y <= width;
    }

    inline bool validPlayer(index_t player) const {
        return player >= 0 && player < playerCount;
    }

    inline bool sameTeam(index_t a, index_t b) const {
        if (!validPlayer(a) || !validPlayer(b) || teams.empty()) return a == b;
        return teams[a] == teams[b];
    }

    inline bool isEnemy(index_t occupier) const {
        return validPlayer(occupier) && !sameTeam(occupier, id);
    }

    inline bool isFriendly(index_t occupier) const {
        return validPlayer(occupier) && sameTeam(occupier, id);
    }

    inline bool isAlive(index_t player) const {
        return validPlayer(player) &&
                       player < static_cast<index_t>(rankById.size())
                   ? rankById[player].alive
                   : false;
    }

    tile_type_e terrainAt(Coord c) const {
        const TileView& tile = board.tileAt(c);
        if (tile.visible) return tile.type;
        const MemoryCell& mem = memory[idx(c)];
        if (mem.seen) return mem.terrain;
        return tile.type;
    }

    index_t occupierAt(Coord c) const {
        const TileView& tile = board.tileAt(c);
        if (tile.visible) return tile.occupier;
        return memory[idx(c)].occupier;
    }

    army_t armyAt(Coord c) const {
        const TileView& tile = board.tileAt(c);
        if (tile.visible) return tile.army;
        return memory[idx(c)].army;
    }

    bool passableForMove(Coord c) const {
        if (!inside(c)) return false;
        tile_type_e type = board.tileAt(c).type;
        return !isImpassableTile(type) && type != TILE_OBSTACLE;
    }

    bool passableForPlan(Coord c) const {
        if (!inside(c)) return false;
        tile_type_e type = terrainAt(c);
        return !isImpassableTile(type) && type != TILE_OBSTACLE;
    }

    void syncRank(const std::vector<RankItem>& rank) {
        rankById = rank;
        std::sort(rankById.begin(), rankById.end(),
                  [](const RankItem& lhs, const RankItem& rhs) {
                      return lhs.player < rhs.player;
                  });
    }

    void updateMemory() {
        myGeneral = Coord{-1, -1};
        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                const TileView& tile = board.tileAt(c);
                MemoryCell& mem = memory[idx(c)];
                mem.visible = tile.visible;

                if (tile.visible) {
                    mem.seen = true;
                    mem.terrain = tile.type;
                    mem.occupier = tile.occupier;
                    mem.army = tile.army;
                    mem.lastSeenTurn = fullTurn;

                    if (tile.type == TILE_GENERAL &&
                        validPlayer(tile.occupier)) {
                        knownGenerals[tile.occupier] = c;
                    }
                    if (tile.type == TILE_GENERAL && tile.occupier == id) {
                        myGeneral = c;
                    }
                } else if (!mem.seen) {
                    mem.terrain = tile.type;
                    mem.occupier = -1;
                    mem.army = 0;
                } else {
                    int stale = std::max(
                        0, static_cast<int>(fullTurn) - mem.lastSeenTurn);
                    if (isEnemy(mem.occupier)) {
                        if (stale > 0 && mem.army > 0) {
                            mem.army =
                                std::max<army_t>(0, mem.army - stale / 2);
                        }
                        if (stale > 14) mem.occupier = -1;
                    }
                }
            }
        }
    }

    std::vector<int> computePressure(bool enemySide) const {
        std::vector<int> pressure((height + 2) * stride, 0);
        int radius = enemySide ? 5 : 4;
        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                const TileView& tile = board.tileAt(x, y);
                if (!tile.visible) continue;

                bool matches =
                    enemySide ? isEnemy(tile.occupier) : tile.occupier == id;
                if (!matches) continue;

                int power = std::min<int>(tile.army, enemySide ? 100 : 120);
                for (int dx = -radius; dx <= radius; ++dx) {
                    for (int dy = -radius; dy <= radius; ++dy) {
                        Coord c{x + dx, y + dy};
                        if (!inside(c)) continue;
                        int dist = std::abs(dx) + std::abs(dy);
                        if (dist > radius) continue;
                        int gain =
                            std::max(0, power - dist * (enemySide ? 14 : 12));
                        pressure[idx(c)] += gain;
                    }
                }
            }
        }
        return pressure;
    }

    void computeBorderStats(std::vector<int>& unknownAdj,
                            std::vector<int>& enemyAdj) const {
        unknownAdj.assign((height + 2) * stride, 0);
        enemyAdj.assign((height + 2) * stride, 0);
        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                for (Coord d : kDirs) {
                    Coord nxt = c + d;
                    if (!inside(nxt)) continue;

                    if (!memory[idx(nxt)].seen &&
                        board.tileAt(nxt).type != TILE_OBSTACLE) {
                        ++unknownAdj[idx(c)];
                    }
                    if (isEnemy(occupierAt(nxt))) {
                        ++enemyAdj[idx(c)];
                    }
                }
            }
        }
    }

    std::vector<int> computeGeneralDist() const {
        std::vector<int> dist((height + 2) * stride, 1e9);
        if (!inside(myGeneral)) return dist;

        std::queue<Coord> q;
        dist[idx(myGeneral)] = 0;
        q.push(myGeneral);
        while (!q.empty()) {
            Coord cur = q.front();
            q.pop();
            for (Coord d : kDirs) {
                Coord nxt = cur + d;
                if (!passableForPlan(nxt)) continue;
                if (dist[idx(nxt)] <= dist[idx(cur)] + 1) continue;
                dist[idx(nxt)] = dist[idx(cur)] + 1;
                q.push(nxt);
            }
        }
        return dist;
    }

    ThreatInfo assessGeneralThreat() const {
        ThreatInfo best;
        if (!inside(myGeneral)) return best;

        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                const TileView& tile = board.tileAt(c);
                if (!tile.visible || !isEnemy(tile.occupier)) continue;

                const int dist =
                    std::abs(c.x - myGeneral.x) + std::abs(c.y - myGeneral.y);
                if (dist > 5) continue;
                if (!best.present || dist < best.dist ||
                    (dist == best.dist && tile.army > best.army)) {
                    best.present = true;
                    best.source = c;
                    best.army = tile.army;
                    best.dist = dist;
                }
            }
        }

        return best;
    }

    std::vector<int> computeDistanceMap(
        const std::vector<Coord>& starts) const {
        std::vector<int> dist((height + 2) * stride, 1e9);
        std::queue<Coord> q;

        for (Coord start : starts) {
            if (!inside(start) || !passableForPlan(start)) continue;
            if (dist[idx(start)] == 0) continue;
            dist[idx(start)] = 0;
            q.push(start);
        }

        while (!q.empty()) {
            Coord cur = q.front();
            q.pop();
            for (Coord d : kDirs) {
                Coord nxt = cur + d;
                if (!passableForPlan(nxt)) continue;
                if (dist[idx(nxt)] <= dist[idx(cur)] + 1) continue;
                dist[idx(nxt)] = dist[idx(cur)] + 1;
                q.push(nxt);
            }
        }

        return dist;
    }

    score_t enterCost(Coord dest, bool gatherMode) const {
        tile_type_e terrain = terrainAt(dest);
        index_t occupier = occupierAt(dest);
        army_t army = armyAt(dest);

        score_t cost = gatherMode ? 8 : 12;
        if (terrain == TILE_SWAMP) cost += gatherMode ? 120 : 70;
        if (terrain == TILE_CITY) cost += gatherMode ? 25 : 18;
        if (terrain == TILE_DESERT) cost += 4;

        if (isFriendly(occupier)) {
            cost -= std::min<score_t>(army, gatherMode ? 4 : 6);
        } else if (occupier == -1) {
            cost += static_cast<score_t>(army) * (gatherMode ? 8 : 5);
        } else if (isEnemy(occupier)) {
            cost += static_cast<score_t>(army) * (gatherMode ? 16 : 10);
            cost += gatherMode ? 90 : 40;
        }

        if (!board.tileAt(dest).visible && !memory[idx(dest)].seen) cost += 8;
        return std::max<score_t>(1, cost);
    }

    PathMap buildPathMap(Coord start, bool gatherMode) const {
        PathMap path;
        path.dist.assign((height + 2) * stride, kInf);
        path.parent.assign((height + 2) * stride, Coord{-1, -1});
        if (!inside(start) || !passableForPlan(start)) return path;

        std::priority_queue<HeapNode> pq;
        path.dist[idx(start)] = 0;
        pq.push(HeapNode{0, start});

        while (!pq.empty()) {
            HeapNode cur = pq.top();
            pq.pop();
            if (cur.dist != path.dist[idx(cur.coord)]) continue;

            for (Coord d : kDirs) {
                Coord nxt = cur.coord + d;
                if (!passableForPlan(nxt)) continue;
                score_t nd = cur.dist + enterCost(nxt, gatherMode);
                if (nd >= path.dist[idx(nxt)]) continue;
                path.dist[idx(nxt)] = nd;
                path.parent[idx(nxt)] = cur.coord;
                pq.push(HeapNode{nd, nxt});
            }
        }

        return path;
    }

    Coord firstStepOnPath(const PathMap& path, Coord start,
                          Coord target) const {
        if (!inside(start) || !inside(target) || target == start) return start;
        Coord cur = target;
        int guard = height * width + 5;
        while (guard-- > 0 && cur != start) {
            Coord prev = path.parent[idx(cur)];
            if (!inside(prev)) return start;
            if (prev == start) return cur;
            cur = prev;
        }
        return start;
    }

    Coord chooseFocus(const std::vector<int>& unknownAdj,
                      const std::vector<int>& enemyAdj,
                      const std::vector<int>& generalDist, bool crowded,
                      bool defenseMode) const {
        if (defenseMode && inside(myGeneral) &&
            board.tileAt(myGeneral).army > 1) {
            return myGeneral;
        }

        if (inside(focusCell)) {
            const TileView& tile = board.tileAt(focusCell);
            if (tile.occupier == id && tile.army > 1) return focusCell;
        }

        score_t bestScore = kNegInf;
        Coord best{-1, -1};

        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                const TileView& tile = board.tileAt(c);
                if (tile.occupier != id || tile.army <= 1) continue;

                score_t score = static_cast<score_t>(tile.army) * 45;
                score += unknownAdj[idx(c)] * (crowded ? 20 : 70);
                score += enemyAdj[idx(c)] * (crowded ? 120 : 80);
                if (inside(myGeneral) && generalDist[idx(c)] < 1e9) {
                    score -= generalDist[idx(c)] * (crowded ? 2 : 5);
                }
                if (score > bestScore) {
                    bestScore = score;
                    best = c;
                }
            }
        }

        return best;
    }

    std::optional<Move> chooseImmediateTacticalMove(bool crowded) const {
        score_t bestScore = kNegInf;
        std::optional<Move> bestMove;

        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord from{x, y};
                const TileView& src = board.tileAt(from);
                if (src.occupier != id || src.army <= 1) continue;

                for (Coord d : kDirs) {
                    Coord to = from + d;
                    if (!passableForMove(to)) continue;

                    const TileView& dst = board.tileAt(to);
                    if (dst.occupier == id) continue;

                    score_t score = kNegInf;
                    army_t moved = src.army - 1;
                    if (dst.visible) {
                        if (dst.type == TILE_GENERAL && isEnemy(dst.occupier) &&
                            moved > dst.army) {
                            score = 1000000000LL + moved - dst.army;
                        } else if (dst.type == TILE_CITY && moved > dst.army) {
                            score = 200000 + moved - dst.army;
                            if (isEnemy(dst.occupier)) score += 150000;
                        } else if (isEnemy(dst.occupier) && moved > dst.army) {
                            score = 50000 + (moved - dst.army) * 20 +
                                    (crowded ? 2000 : 0);
                        } else if (dst.occupier == -1 && moved > dst.army &&
                                   dst.type != TILE_SWAMP) {
                            score = 1200 + moved - dst.army;
                            if (dst.type == TILE_DESERT) score += 150;
                        }
                    } else if (board.tileAt(to).type != TILE_OBSTACLE) {
                        score = 400;
                    }

                    if (score > bestScore) {
                        bestScore = score;
                        bestMove = Move(MoveType::MOVE_ARMY, from, to, false);
                    }
                }
            }
        }

        return bestMove;
    }

    std::optional<Move> chooseDefenseMove(
        const std::vector<int>& enemyPressure,
        const std::vector<int>& friendlyPressure,
        const std::vector<int>& generalDist, const ThreatInfo& threat,
        bool forceDefense) const {
        if (!inside(myGeneral)) return std::nullopt;
        if (!forceDefense && enemyPressure[idx(myGeneral)] <=
                                 friendlyPressure[idx(myGeneral)] + 30) {
            return std::nullopt;
        }

        score_t bestScore = kNegInf;
        std::optional<Move> bestMove;
        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord from{x, y};
                const TileView& src = board.tileAt(from);
                if (src.occupier != id || src.army <= 1) continue;

                for (Coord d : kDirs) {
                    Coord to = from + d;
                    if (!passableForMove(to)) continue;

                    score_t score = 0;
                    if (generalDist[idx(to)] < generalDist[idx(from)])
                        score += 600;
                    score += enemyPressure[idx(to)] - enemyPressure[idx(from)];
                    if (to == myGeneral) score += 1200;
                    if (from == myGeneral) score -= 4000;
                    if (threat.present) {
                        const int fromThreatDist =
                            std::abs(from.x - threat.source.x) +
                            std::abs(from.y - threat.source.y);
                        const int toThreatDist =
                            std::abs(to.x - threat.source.x) +
                            std::abs(to.y - threat.source.y);
                        if (toThreatDist < fromThreatDist) score += 450;
                        if (generalDist[idx(to)] <=
                            generalDist[idx(from)] + 1) {
                            score += 150;
                        }
                    }
                    if (board.tileAt(to).visible &&
                        isEnemy(board.tileAt(to).occupier) &&
                        src.army - 1 > board.tileAt(to).army) {
                        score +=
                            1500 + (src.army - 1 - board.tileAt(to).army) * 20;
                    }

                    if (score > bestScore) {
                        bestScore = score;
                        bestMove = Move(MoveType::MOVE_ARMY, from, to, false);
                    }
                }
            }
        }

        if (bestScore > 0) return bestMove;
        return std::nullopt;
    }

    std::optional<Move> chooseCrowdedSkirmishMove(
        const std::vector<int>& unknownAdj, const std::vector<int>& enemyAdj,
        const std::vector<int>& enemyPressure,
        const std::vector<int>& friendlyPressure) const {
        std::vector<Coord> unknownTargets;
        std::vector<Coord> enemyTargets;
        std::vector<Coord> cityTargets;
        std::vector<Coord> generalTargets;

        for (index_t player = 0; player < playerCount; ++player) {
            if (player == id || !isAlive(player) ||
                !inside(knownGenerals[player])) {
                continue;
            }
            generalTargets.push_back(knownGenerals[player]);
        }

        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                if (!passableForPlan(c)) continue;

                if (!memory[idx(c)].seen &&
                    board.tileAt(c).type != TILE_OBSTACLE) {
                    unknownTargets.push_back(c);
                }
                if (isEnemy(occupierAt(c))) enemyTargets.push_back(c);
                if (terrainAt(c) == TILE_CITY && !isFriendly(occupierAt(c))) {
                    cityTargets.push_back(c);
                }
            }
        }

        const auto unknownDist = computeDistanceMap(unknownTargets);
        const auto enemyDist = computeDistanceMap(enemyTargets);
        const auto cityDist = computeDistanceMap(cityTargets);
        const auto generalTargetDist = computeDistanceMap(generalTargets);

        int aliveEnemies = 0;
        army_t myArmy = 0;
        army_t bestEnemyArmy = 0;
        if (id < static_cast<index_t>(rankById.size()))
            myArmy = rankById[id].army;
        for (index_t player = 0; player < playerCount; ++player) {
            if (player == id || !isAlive(player)) continue;
            ++aliveEnemies;
            if (player < static_cast<index_t>(rankById.size())) {
                bestEnemyArmy = std::max(bestEnemyArmy, rankById[player].army);
            }
        }

        const bool opening = fullTurn < 18;
        const bool ahead = myArmy >= bestEnemyArmy;
        const Coord center{static_cast<pos_t>((height + 1) / 2),
                           static_cast<pos_t>((width + 1) / 2)};

        auto frontierValue = [&](Coord c) -> int {
            return unknownAdj[idx(c)] * 7 + enemyAdj[idx(c)] * 18;
        };

        Candidate best;
        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord from{x, y};
                const TileView& src = board.tileAt(from);
                if (src.occupier != id || src.army <= 1) continue;

                const bool sourceProducer =
                    from == myGeneral || src.type == TILE_CITY;
                const bool sourceFrontier = frontierValue(from) > 0;

                for (Coord d : kDirs) {
                    Coord to = from + d;
                    if (!passableForMove(to)) continue;

                    const TileView& dst = board.tileAt(to);
                    const bool considerHalf =
                        src.army >= 4 &&
                        (sourceProducer || dst.occupier == id ||
                         enemyPressure[idx(from)] >
                             friendlyPressure[idx(from)] + 8);

                    for (int mode = 0; mode < (considerHalf ? 2 : 1); ++mode) {
                        const bool takeHalf = (mode == 1);
                        const army_t moved =
                            takeHalf ? static_cast<army_t>(src.army >> 1)
                                     : static_cast<army_t>(src.army - 1);
                        const army_t remain =
                            static_cast<army_t>(src.army - moved);
                        if (moved <= 0) continue;

                        score_t score = static_cast<score_t>(moved) * 20;
                        score += frontierValue(to) * 80;
                        score -= frontierValue(from) * 12;

                        if (from == myGeneral) {
                            score -= opening ? 180 : 1200;
                            if (enemyAdj[idx(from)] > 0) score -= 2200;
                        }
                        if (sourceProducer && remain <= 1) score -= 550;
                        if (from == lastMoveTo && to == lastMoveFrom)
                            score -= 280;
                        if (board.tileAt(to).type == TILE_SWAMP) score -= 1200;

                        if (enemyPressure[idx(from)] >
                            friendlyPressure[idx(from)] + remain * 8) {
                            score -= 1800 + static_cast<score_t>(
                                                enemyPressure[idx(from)] -
                                                friendlyPressure[idx(from)] -
                                                remain * 8) *
                                                6;
                        }
                        if (enemyPressure[idx(to)] >
                            friendlyPressure[idx(to)] + moved * 8) {
                            score -= 700 + static_cast<score_t>(
                                               enemyPressure[idx(to)] -
                                               friendlyPressure[idx(to)] -
                                               moved * 8) *
                                               3;
                        }

                        if (dst.occupier == id) {
                            if (!sourceFrontier &&
                                frontierValue(to) > frontierValue(from)) {
                                score += 1400;
                            }
                            if (enemyDist[idx(from)] < 1e9 &&
                                enemyDist[idx(to)] < enemyDist[idx(from)]) {
                                score +=
                                    static_cast<score_t>(enemyDist[idx(from)] -
                                                         enemyDist[idx(to)]) *
                                    230;
                            }
                            if (unknownDist[idx(from)] < 1e9 &&
                                unknownDist[idx(to)] < unknownDist[idx(from)]) {
                                score += static_cast<score_t>(
                                             unknownDist[idx(from)] -
                                             unknownDist[idx(to)]) *
                                         (opening ? 170 : 65);
                            }
                            if (!generalTargets.empty() &&
                                generalTargetDist[idx(from)] < 1e9 &&
                                generalTargetDist[idx(to)] <
                                    generalTargetDist[idx(from)]) {
                                score += static_cast<score_t>(
                                             generalTargetDist[idx(from)] -
                                             generalTargetDist[idx(to)]) *
                                         260;
                            }
                            if (cityDist[idx(from)] < 1e9 &&
                                cityDist[idx(to)] < cityDist[idx(from)] &&
                                src.army >= 18) {
                                score +=
                                    static_cast<score_t>(cityDist[idx(from)] -
                                                         cityDist[idx(to)]) *
                                    90;
                            }
                            if (to == myGeneral) score += 900;
                            if (takeHalf) score += 220;
                        } else if (!dst.visible) {
                            score += 1300;
                            score += unknownAdj[idx(to)] * 150;
                            score += enemyAdj[idx(to)] * 210;
                            score += static_cast<score_t>(
                                         std::abs(from.x - center.x) +
                                         std::abs(from.y - center.y) -
                                         std::abs(to.x - center.x) -
                                         std::abs(to.y - center.y)) *
                                     (opening ? 26 : 8);
                            if (takeHalf && !sourceProducer) score -= 160;
                        } else if (dst.occupier == -1) {
                            if (moved <= dst.army) continue;
                            if (dst.type == TILE_CITY) {
                                score += opening ? -2200
                                                 : 9000 - static_cast<score_t>(
                                                              dst.army) *
                                                              45;
                            } else {
                                score +=
                                    900 - static_cast<score_t>(dst.army) * 14;
                                score += unknownAdj[idx(to)] * 90;
                                score += enemyAdj[idx(to)] * 160;
                                if (dst.type == TILE_DESERT) score += 120;
                            }
                            if (takeHalf && !sourceProducer) score -= 260;
                        } else if (isEnemy(dst.occupier)) {
                            if (dst.type == TILE_GENERAL) {
                                if (moved > dst.army) {
                                    score += 1000000000LL;
                                } else {
                                    continue;
                                }
                            } else if (moved <= dst.army) {
                                continue;
                            } else {
                                score +=
                                    12000 +
                                    static_cast<score_t>(moved - dst.army) *
                                        55 +
                                    static_cast<score_t>(dst.army) * 18;
                                score += enemyAdj[idx(to)] * 260;
                                if (dst.type == TILE_CITY) score += 18000;
                            }
                            if (!ahead) score += 900;
                            if (takeHalf) score -= 320;
                        }

                        if (aliveEnemies >= 6 && !ahead &&
                            unknownAdj[idx(to)] > 0) {
                            score += 180;
                        }
                        if (enemyPressure[idx(to)] <=
                            friendlyPressure[idx(to)] + moved * 3) {
                            score += 260;
                        }

                        if (score > best.score) {
                            best.score = score;
                            best.move =
                                Move(MoveType::MOVE_ARMY, from, to, takeHalf);
                        }
                    }
                }
            }
        }

        if (best.score > 0) return best.move;
        return std::nullopt;
    }

    std::optional<Move> chooseCrowdedOpeningMove(
        const std::vector<int>& unknownAdj, const std::vector<int>& enemyAdj,
        const std::vector<int>& enemyPressure,
        const std::vector<int>& friendlyPressure) const {
        const Coord center{static_cast<pos_t>((height + 1) / 2),
                           static_cast<pos_t>((width + 1) / 2)};
        Candidate best;

        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord from{x, y};
                const TileView& src = board.tileAt(from);
                if (src.occupier != id || src.army <= 1) continue;

                for (Coord d : kDirs) {
                    Coord to = from + d;
                    if (!passableForMove(to)) continue;

                    const TileView& dst = board.tileAt(to);
                    const army_t moved = src.army - 1;
                    if (moved <= 0) continue;

                    score_t score = kNegInf;
                    if (!dst.visible) {
                        if (board.tileAt(to).type == TILE_SWAMP) continue;
                        score = 2400 + unknownAdj[idx(to)] * 260 +
                                enemyAdj[idx(to)] * 160;
                    } else if (dst.occupier == -1) {
                        if (dst.type == TILE_SWAMP || moved <= dst.army)
                            continue;
                        if (dst.type == TILE_CITY) {
                            if (fullTurn < 10 || moved <= dst.army + 4)
                                continue;
                            score = 7000 - static_cast<score_t>(dst.army) * 50;
                        } else {
                            score = 1500 - static_cast<score_t>(dst.army) * 18 +
                                    unknownAdj[idx(to)] * 180 +
                                    enemyAdj[idx(to)] * 140;
                            if (dst.type == TILE_DESERT) score += 100;
                        }
                    } else if (isEnemy(dst.occupier)) {
                        if (dst.type == TILE_GENERAL && moved > dst.army) {
                            score = 1000000000LL + moved - dst.army;
                        } else if (moved > dst.army + 2 &&
                                   enemyPressure[idx(to)] <=
                                       friendlyPressure[idx(to)] + moved * 3) {
                            score =
                                7000 +
                                static_cast<score_t>(moved - dst.army) * 70 +
                                enemyAdj[idx(to)] * 240;
                            if (dst.type == TILE_CITY) score += 12000;
                        } else {
                            continue;
                        }
                    } else {
                        continue;
                    }

                    if (from == lastMoveTo) score += 700;
                    if (from == myGeneral) {
                        score += 250;
                        if (enemyAdj[idx(from)] > 0) score -= 3500;
                    }
                    score += static_cast<score_t>(std::abs(from.x - center.x) +
                                                  std::abs(from.y - center.y) -
                                                  std::abs(to.x - center.x) -
                                                  std::abs(to.y - center.y)) *
                             35;
                    if (enemyPressure[idx(from)] >
                        friendlyPressure[idx(from)] + src.army * 6) {
                        score -= 1800;
                    }
                    if (enemyPressure[idx(to)] >
                        friendlyPressure[idx(to)] + moved * 5) {
                        score -=
                            1600 + static_cast<score_t>(
                                       enemyPressure[idx(to)] -
                                       friendlyPressure[idx(to)] - moved * 5) *
                                       6;
                    }

                    if (score > best.score) {
                        best.score = score;
                        best.move = Move(MoveType::MOVE_ARMY, from, to, false);
                    }
                }
            }
        }

        if (best.score > 0) return best.move;
        return std::nullopt;
    }

    Coord chooseBestTarget(Coord focus, const PathMap& path,
                           const std::vector<int>& unknownAdj,
                           const std::vector<int>& enemyAdj, bool crowded,
                           bool huge, bool opening,
                           bool enemyGeneralKnown) const {
        if (!inside(focus)) return Coord{-1, -1};

        const army_t focusArmy = board.tileAt(focus).army;
        score_t bestScore = kNegInf;
        Coord best{-1, -1};
        const Coord center{static_cast<pos_t>((height + 1) / 2),
                           static_cast<pos_t>((width + 1) / 2)};

        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord target{x, y};
                if (!passableForPlan(target)) continue;
                if (path.dist[idx(target)] >= kInf / 8) continue;

                const index_t occupier = occupierAt(target);
                if (isFriendly(occupier)) continue;

                const TileView& tile = board.tileAt(target);
                const army_t defense = armyAt(target);
                score_t value = kNegInf;

                if (isEnemy(occupier)) {
                    if (inside(knownGenerals[occupier]) &&
                        knownGenerals[occupier] == target) {
                        value = 100000 - static_cast<score_t>(defense) * 120;
                    } else if (tile.visible && tile.type == TILE_CITY) {
                        value = 18000 - static_cast<score_t>(defense) * 65;
                    } else {
                        value = 4500 - static_cast<score_t>(defense) * 18 +
                                enemyAdj[idx(target)] * 240;
                    }
                    if (crowded) value += 1000;
                } else {
                    if (tile.visible) {
                        if (tile.type == TILE_CITY) {
                            value = 11000 - static_cast<score_t>(defense) * 50;
                        } else if (tile.type == TILE_SWAMP) {
                            value = 80;
                        } else if (tile.type == TILE_DESERT) {
                            value = 900 - static_cast<score_t>(defense) * 4;
                        } else {
                            value = 1400 - static_cast<score_t>(defense) * 10;
                        }
                        value += unknownAdj[idx(target)] * (opening ? 100 : 55);
                    } else if (!memory[idx(target)].seen &&
                               board.tileAt(target).type != TILE_OBSTACLE) {
                        value = 1800 + unknownAdj[idx(target)] * 120;
                    }
                }

                if (value <= kNegInf / 8) continue;

                value -= path.dist[idx(target)];
                if (focusArmy <= defense && value < 50000) {
                    value -= 5000 +
                             static_cast<score_t>(defense - focusArmy + 1) * 80;
                }

                if (huge && !enemyGeneralKnown) {
                    value -= (std::abs(target.x - center.x) +
                              std::abs(target.y - center.y)) *
                             3;
                }

                if (value > bestScore) {
                    bestScore = value;
                    best = target;
                }
            }
        }

        return best;
    }

    bool isMacroTargetUseful(Coord target) const {
        if (!inside(target) || !passableForPlan(target)) return false;
        const index_t occupier = occupierAt(target);
        if (isEnemy(occupier)) return true;
        if (terrainAt(target) == TILE_CITY && !isFriendly(occupier))
            return true;
        return !memory[idx(target)].seen &&
               board.tileAt(target).type != TILE_OBSTACLE;
    }

    std::optional<Move> gatherToward(Coord focus, const PathMap& gatherPath,
                                     bool crowded) const {
        if (!inside(focus)) return std::nullopt;

        score_t bestScore = kNegInf;
        std::optional<Move> bestMove;
        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord from{x, y};
                const TileView& src = board.tileAt(from);
                if (src.occupier != id || src.army <= 1 || from == focus)
                    continue;

                Coord step = firstStepOnPath(gatherPath, focus, from);
                if (!inside(step) || step == from || !passableForMove(step))
                    continue;
                if (!isFriendly(board.tileAt(step).occupier) && step != focus)
                    continue;

                score_t score = static_cast<score_t>(src.army) * 40;
                score -= gatherPath.dist[idx(from)] * (crowded ? 4 : 2);
                if (step == focus) score += 600;
                if (from == myGeneral) score -= 2500;

                if (score > bestScore) {
                    bestScore = score;
                    bestMove = Move(MoveType::MOVE_ARMY, from, step, false);
                }
            }
        }

        if (bestScore > 0) return bestMove;
        return std::nullopt;
    }

    std::optional<Move> chooseFallback(Coord focus, bool crowded) const {
        Candidate best;

        if (!inside(focus)) {
            for (pos_t x = 1; x <= height; ++x) {
                for (pos_t y = 1; y <= width; ++y) {
                    Coord c{x, y};
                    const TileView& tile = board.tileAt(c);
                    if (tile.occupier == id && tile.army > 1 &&
                        (!inside(focus) ||
                         tile.army > board.tileAt(focus).army)) {
                        focus = c;
                    }
                }
            }
        }
        if (!inside(focus) || board.tileAt(focus).army <= 1)
            return std::nullopt;

        for (Coord d : kDirs) {
            Coord to = focus + d;
            if (!passableForMove(to)) continue;
            const TileView& dst = board.tileAt(to);

            score_t score = 0;
            if (!dst.visible) {
                score += 300;
            } else if (dst.occupier == -1) {
                if (board.tileAt(focus).army - 1 > dst.army) {
                    score += 200 - dst.army * 10;
                    if (dst.type == TILE_DESERT) score += 30;
                }
            } else if (isFriendly(dst.occupier)) {
                score += crowded ? 40 : 80;
            } else if (board.tileAt(focus).army - 1 > dst.army) {
                score += 500 + (board.tileAt(focus).army - 1 - dst.army) * 20;
            }

            if (score > best.score) {
                best.score = score;
                best.move = Move(MoveType::MOVE_ARMY, focus, to, false);
            }
        }

        if (best.score > 0) return best.move;
        return std::nullopt;
    }

   public:
    void init(index_t playerId, const GameConstantsPack& constants) override {
        id = playerId;
        playerCount = constants.playerCount;
        height = constants.mapHeight;
        width = constants.mapWidth;
        stride = width + 2;
        teams = constants.teams;

        halfTurn = fullTurn = 0;
        memory.assign((height + 2) * stride, MemoryCell{});
        knownGenerals.assign(playerCount, Coord{-1, -1});
        rankById.assign(playerCount, RankItem{});
        myGeneral = Coord{-1, -1};
        focusCell = Coord{-1, -1};
        macroTarget = Coord{-1, -1};
        lastMoveFrom = Coord{-1, -1};
        lastMoveTo = Coord{-1, -1};
        macroTargetLockUntil = -1;
    }

    void requestMove(const BoardView& boardView,
                     const std::vector<RankItem>& rank) override {
        ++halfTurn;
        fullTurn += (halfTurn & 1);

        board = boardView;
        syncRank(rank);
        updateMemory();
        moveQueue.clear();

        const int mapArea = static_cast<int>(height * width);
        const double areaPerPlayer =
            static_cast<double>(mapArea) / std::max<index_t>(1, playerCount);
        const bool crowded = areaPerPlayer <= 60.0;
        const bool huge = mapArea >= 2500;
        const bool opening = fullTurn < (crowded ? 10 : 14);

        std::vector<int> unknownAdj, enemyAdj;
        computeBorderStats(unknownAdj, enemyAdj);
        const auto enemyPressure = computePressure(true);
        const auto friendlyPressure = computePressure(false);
        const auto generalDist = computeGeneralDist();
        const ThreatInfo generalThreat = assessGeneralThreat();

        const int defenseMargin = crowded && playerCount >= 6 ? 12 : 30;
        bool defenseMode = false;
        bool forceDefense = false;
        if (inside(myGeneral)) {
            defenseMode = enemyPressure[idx(myGeneral)] >
                          friendlyPressure[idx(myGeneral)] + defenseMargin;
            if (crowded && playerCount >= 6 && generalThreat.present) {
                const army_t generalArmy = board.tileAt(myGeneral).army;
                forceDefense = generalThreat.dist <= 2 ||
                               (generalThreat.dist <= 4 &&
                                generalThreat.army + 2 >=
                                    std::max<army_t>(1, generalArmy));
                defenseMode = defenseMode || forceDefense;
            }
        }

        if (auto defense =
                chooseDefenseMove(enemyPressure, friendlyPressure, generalDist,
                                  generalThreat, forceDefense)) {
            lastMoveFrom = defense->from;
            lastMoveTo = defense->to;
            moveQueue.push_back(*defense);
            return;
        }

        if (crowded && playerCount >= 6 && fullTurn <= 12) {
            if (auto openingMove = chooseCrowdedOpeningMove(
                    unknownAdj, enemyAdj, enemyPressure, friendlyPressure)) {
                lastMoveFrom = openingMove->from;
                lastMoveTo = openingMove->to;
                moveQueue.push_back(*openingMove);
                return;
            }
        }

        if (auto tactical = chooseImmediateTacticalMove(crowded)) {
            lastMoveFrom = tactical->from;
            lastMoveTo = tactical->to;
            moveQueue.push_back(*tactical);
            return;
        }

        bool enemyGeneralKnown = false;
        for (index_t player = 0; player < playerCount; ++player) {
            if (player != id && isAlive(player) &&
                inside(knownGenerals[player])) {
                enemyGeneralKnown = true;
                break;
            }
        }

        focusCell = chooseFocus(unknownAdj, enemyAdj, generalDist, crowded,
                                defenseMode);
        if (!inside(focusCell)) return;

        const PathMap attackPath = buildPathMap(focusCell, false);
        Coord target{-1, -1};
        const bool largeMacro = mapArea >= 1600 && mapArea <= 3000 &&
                                !crowded &&
                                (playerCount <= 2 || mapArea >= 2500);
        if (largeMacro &&
            fullTurn <= static_cast<turn_t>(macroTargetLockUntil) &&
            isMacroTargetUseful(macroTarget)) {
            target = macroTarget;
        } else {
            target =
                chooseBestTarget(focusCell, attackPath, unknownAdj, enemyAdj,
                                 crowded, huge, opening, enemyGeneralKnown);
            if (largeMacro && inside(target)) {
                macroTarget = target;
                macroTargetLockUntil =
                    static_cast<int>(fullTurn) + (enemyGeneralKnown ? 16 : 10);
            }
        }

        if (inside(target) && target != focusCell) {
            Coord step = firstStepOnPath(attackPath, focusCell, target);
            if (inside(step) && step != focusCell && passableForMove(step)) {
                const TileView& focusTile = board.tileAt(focusCell);
                const TileView& stepTile = board.tileAt(step);
                const bool blockedByFight =
                    stepTile.visible && !isFriendly(stepTile.occupier) &&
                    focusTile.army - 1 <= stepTile.army &&
                    stepTile.type != TILE_GENERAL;

                if (!blockedByFight) {
                    lastMoveFrom = focusCell;
                    lastMoveTo = step;
                    moveQueue.emplace_back(MoveType::MOVE_ARMY, focusCell, step,
                                           false);
                    return;
                }
            }
        }

        const PathMap gatherPath = buildPathMap(focusCell, true);
        if (auto gather = gatherToward(focusCell, gatherPath, crowded)) {
            lastMoveFrom = gather->from;
            lastMoveTo = gather->to;
            moveQueue.push_back(*gather);
            return;
        }

        if (auto fallback = chooseFallback(focusCell, crowded)) {
            lastMoveFrom = fallback->from;
            lastMoveTo = fallback->to;
            moveQueue.push_back(*fallback);
        }
    }
};

static BotRegistrar<XiaruizeBot> xiaruizeBot_reg("XiaruizeBot");

#endif  // LGEN_BOTS_XIARUIZEBOT
