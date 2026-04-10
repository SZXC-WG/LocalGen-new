// Copyright (C) 2026 xiaruize0911
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file xiaruizeBot.cpp
 *
 * Multi-source strategic version of XiaruizeBot.
 * Core idea: enumerate several strong source stacks, build a weighted path map
 * for each source, score all targets globally, then execute the first step of
 * the best source-target plan.
 *
 * @author xiaruize0911
 */

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

    struct ScoredCoord {
        Coord c{-1, -1};
        score_t score = kNegInf;
    };

    struct StrategicCandidate {
        Move move{};
        Coord source{-1, -1};
        Coord target{-1, -1};
        score_t score = kNegInf;
    };

    pos_t height = 0;
    pos_t width = 0;
    pos_t stride = 0;
    index_t id = -1;
    index_t playerCount = 0;
    turn_t halfTurn = 0;
    turn_t fullTurn = 0;
    config::Config config;

    std::vector<index_t> teams;
    BoardView board;
    std::vector<RankItem> rankById;
    std::vector<MemoryCell> memory;
    std::vector<Coord> knownGenerals;
    std::vector<std::vector<double>> predictedGeneralScore;
    std::vector<std::vector<uint8_t>> candidateGeneralMask;

    Coord myGeneral{-1, -1};
    Coord lastMoveFrom{-1, -1};
    Coord lastMoveTo{-1, -1};
    Coord lastStrategicTarget{-1, -1};

    std::mt19937 rng{std::random_device{}()};

    inline size_t idx(pos_t x, pos_t y) const {
        return static_cast<size_t>(x * stride + y);
    }
    inline size_t idx(Coord c) const { return idx(c.x, c.y); }

    inline bool inside(Coord c) const {
        return c.x >= 1 && c.x <= height && c.y >= 1 && c.y <= width;
    }

    inline int manhattan(Coord a, Coord b) const {
        return std::abs(a.x - b.x) + std::abs(a.y - b.y);
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
                    mem.lastSeenTurn = static_cast<int>(fullTurn);

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
                    if (isEnemy(mem.occupier) && stale > 0 && mem.army > 0) {
                        mem.army = std::max<army_t>(0, mem.army - stale / 2);
                    }
                    if (isEnemy(mem.occupier) && stale > 14) mem.occupier = -1;
                }
            }
        }
    }

    void reinforceGeneralNeighborhood(index_t player, Coord anchor,
                                      double baseScore, int maxDist) {
        if (!validPlayer(player) || player == id) return;
        auto& heat = predictedGeneralScore[player];
        auto& mask = candidateGeneralMask[player];
        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                if (manhattan(c, anchor) > maxDist) continue;
                if (!passableForPlan(c) || terrainAt(c) == TILE_CITY) continue;
                if (board.tileAt(c).visible) continue;
                if (inside(myGeneral) && manhattan(c, myGeneral) < 8) continue;
                mask[idx(c)] = 1;
                heat[idx(c)] +=
                    std::max(0.0, baseScore - manhattan(c, anchor) * 2.0);
            }
        }
    }

    void updateGeneralPredictions() {
        if (predictedGeneralScore.size() != static_cast<size_t>(playerCount) ||
            candidateGeneralMask.size() != static_cast<size_t>(playerCount)) {
            return;
        }

        for (index_t player = 0; player < playerCount; ++player) {
            if (player == id) continue;
            auto& heat = predictedGeneralScore[player];
            auto& mask = candidateGeneralMask[player];

            if (inside(knownGenerals[player])) {
                std::fill(heat.begin(), heat.end(), 0.0);
                std::fill(mask.begin(), mask.end(), 0);
                heat[idx(knownGenerals[player])] = 1e12;
                mask[idx(knownGenerals[player])] = 1;
                continue;
            }

            for (double& score : heat) score *= 0.987;
            for (pos_t x = 1; x <= height; ++x) {
                for (pos_t y = 1; y <= width; ++y) {
                    Coord c{x, y};
                    const bool valid =
                        passableForPlan(c) && terrainAt(c) != TILE_CITY &&
                        !board.tileAt(c).visible &&
                        (!inside(myGeneral) || manhattan(c, myGeneral) >= 8);
                    mask[idx(c)] = valid ? 1 : 0;
                    if (!valid) heat[idx(c)] = 0.0;
                }
            }
        }

        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                const TileView& tile = board.tileAt(c);
                if (tile.visible && isEnemy(tile.occupier)) {
                    reinforceGeneralNeighborhood(tile.occupier, c, 14.0, 6);
                    continue;
                }
                const MemoryCell& mem = memory[idx(c)];
                if (!tile.visible && isEnemy(mem.occupier) &&
                    static_cast<int>(fullTurn) - mem.lastSeenTurn <= 4) {
                    reinforceGeneralNeighborhood(mem.occupier, c, 8.0, 4);
                }
            }
        }
    }

    Coord choosePredictedGeneral(index_t player) const {
        if (!validPlayer(player) || player == id ||
            player >= static_cast<index_t>(predictedGeneralScore.size())) {
            return Coord{-1, -1};
        }
        if (inside(knownGenerals[player])) return knownGenerals[player];

        const auto& heat = predictedGeneralScore[player];
        const auto& mask = candidateGeneralMask[player];
        double bestScore = 0.0;
        Coord best{-1, -1};
        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                if (!mask[idx(c)]) continue;
                double score = heat[idx(c)];
                if (score <= bestScore) continue;
                bestScore = score;
                best = c;
            }
        }
        return best;
    }

    std::vector<int> computePressure(bool enemySide) const {
        std::vector<int> pressure((height + 2) * stride, 0);
        const int radius = enemySide ? 5 : 4;
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
                    if (isEnemy(occupierAt(nxt))) ++enemyAdj[idx(c)];
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

    ThreatInfo assessGeneralThreat() const {
        ThreatInfo best;
        if (!inside(myGeneral)) return best;
        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                const TileView& tile = board.tileAt(c);
                if (!tile.visible || !isEnemy(tile.occupier)) continue;
                int dist =
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

    std::optional<Move> chooseForcedWinMove() const {
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
                    if (!dst.visible || dst.type != TILE_GENERAL ||
                        !isEnemy(dst.occupier))
                        continue;
                    if (src.army - 1 <= dst.army) continue;
                    score_t score = 1000000000LL + (src.army - 1 - dst.army);
                    if (score > bestScore) {
                        bestScore = score;
                        bestMove = Move(MoveType::MOVE_ARMY, from, to, false);
                    }
                }
            }
        }
        return bestMove;
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
                        } else if (isEnemy(dst.occupier) &&
                                   moved > dst.army + (crowded ? 1 : 2)) {
                            score = 50000 + (moved - dst.army) * 20 +
                                    (crowded ? 2000 : 0);
                        }
                    }

                    if (score > bestScore) {
                        bestScore = score;
                        bestMove = Move(MoveType::MOVE_ARMY, from, to, false);
                    }
                }
            }
        }

        if (bestScore < 30000) return std::nullopt;
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
                        score += 700;
                    if (to == myGeneral) score += 1400;
                    if (from == myGeneral) score -= 4500;
                    score += enemyPressure[idx(to)] - enemyPressure[idx(from)];
                    score -= friendlyPressure[idx(from)] / 6;

                    if (threat.present) {
                        int fromThreatDist =
                            std::abs(from.x - threat.source.x) +
                            std::abs(from.y - threat.source.y);
                        int toThreatDist = std::abs(to.x - threat.source.x) +
                                           std::abs(to.y - threat.source.y);
                        if (toThreatDist < fromThreatDist) score += 450;
                        if (toThreatDist <= 1) score += 300;
                    }

                    const TileView& dst = board.tileAt(to);
                    if (dst.visible && isEnemy(dst.occupier) &&
                        src.army - 1 > dst.army) {
                        score += 1800 + (src.army - 1 - dst.army) * 20;
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

    std::vector<Coord> enumerateSources(
        const std::vector<int>& unknownAdj, const std::vector<int>& enemyAdj,
        const std::vector<int>& generalDist,
        const std::vector<int>& enemyPressure,
        const std::vector<int>& friendlyPressure, bool crowded) const {
        std::vector<ScoredCoord> scored;
        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                const TileView& tile = board.tileAt(c);
                if (tile.occupier != id || tile.army <= 1) continue;

                score_t score = static_cast<score_t>(tile.army) * 55;
                score += static_cast<score_t>(unknownAdj[idx(c)]) *
                         (crowded ? 18 : 55);
                score += static_cast<score_t>(enemyAdj[idx(c)]) *
                         (crowded ? 130 : 95);
                if (generalDist[idx(c)] < 1e9)
                    score -= static_cast<score_t>(generalDist[idx(c)]) * 4;
                if (friendlyPressure[idx(c)] > enemyPressure[idx(c)])
                    score += 120;
                if (enemyPressure[idx(c)] >
                    friendlyPressure[idx(c)] + tile.army * 10)
                    score -= 1800;
                if (c == myGeneral) score -= 5000;
                if (tile.type == TILE_CITY && tile.army <= 4) score -= 1200;
                if (c == lastMoveTo) score += 350;
                scored.push_back({c, score});
            }
        }
        std::sort(scored.begin(), scored.end(),
                  [](const ScoredCoord& a, const ScoredCoord& b) {
                      if (a.score != b.score) return a.score > b.score;
                      return a.c < b.c;
                  });

        std::vector<Coord> res;
        const size_t limit = crowded ? 4 : 6;
        for (size_t i = 0; i < scored.size() && res.size() < limit; ++i)
            res.push_back(scored[i].c);
        return res;
    }

    score_t evaluateTargetValue(Coord source, Coord target, Coord firstStep,
                                const PathMap& path,
                                const std::vector<int>& unknownAdj,
                                const std::vector<int>& enemyAdj,
                                const std::vector<int>& enemyPressure,
                                const std::vector<int>& friendlyPressure,
                                const std::vector<int>& generalDist,
                                bool crowded, bool huge, bool opening,
                                bool enemyGeneralKnown, bool duelMode,
                                int mapArea, Coord enemyHint) const {
        if (!inside(source) || !inside(target) || !inside(firstStep))
            return kNegInf;
        if (path.dist[idx(target)] >= kInf / 8) return kNegInf;
        if (!passableForMove(firstStep)) return kNegInf;

        const TileView& src = board.tileAt(source);
        const army_t sourceArmy = src.army;
        if (sourceArmy <= 1) return kNegInf;

        const index_t occupier = occupierAt(target);
        const army_t defense = armyAt(target);
        const tile_type_e terrain = terrainAt(target);
        score_t score = kNegInf;

        if (isEnemy(occupier)) {
            if (inside(knownGenerals[occupier]) &&
                knownGenerals[occupier] == target) {
                score = 1000000000LL - static_cast<score_t>(defense) * 200;
            } else if (terrain == TILE_CITY) {
                score = 32000 - static_cast<score_t>(defense) * 60;
            } else {
                score = 7000 - static_cast<score_t>(defense) * 20 +
                        static_cast<score_t>(enemyAdj[idx(target)]) * 260;
                if (duelMode) score += 1800;
            }
        } else if (occupier == -1) {
            if (!memory[idx(target)].seen &&
                board.tileAt(target).type != TILE_OBSTACLE) {
                score =
                    2500 + static_cast<score_t>(unknownAdj[idx(target)]) * 140;
            } else if (terrain == TILE_CITY) {
                score = 15000 - static_cast<score_t>(defense) * 50;
            } else if (terrain == TILE_SWAMP) {
                score = 80;
            } else if (terrain == TILE_DESERT) {
                score = 900 - static_cast<score_t>(defense) * 4 +
                        static_cast<score_t>(unknownAdj[idx(target)]) * 35;
            } else {
                score = 1400 - static_cast<score_t>(defense) * 10 +
                        static_cast<score_t>(unknownAdj[idx(target)]) *
                            (opening ? 90 : 50);
            }
        }
        if (score <= kNegInf / 8) return score;

        if (terrain == TILE_CITY) {
            if (opening) score -= crowded ? 1600 : 4200;
            if (duelMode && fullTurn < 18)
                score -= mapArea >= 700 ? 3600 : 1800;
        }

        score += static_cast<score_t>(enemyAdj[idx(target)]) * 70;
        score +=
            static_cast<score_t>(unknownAdj[idx(target)]) * (opening ? 16 : 8);
        if (target == lastStrategicTarget) score += duelMode ? 1800 : 600;
        if (huge && !enemyGeneralKnown) {
            Coord center{static_cast<pos_t>((height + 1) / 2),
                         static_cast<pos_t>((width + 1) / 2)};
            score -= static_cast<score_t>(std::abs(target.x - center.x) +
                                          std::abs(target.y - center.y)) *
                     3;
        }
        if (duelMode && inside(enemyHint)) {
            const int hintDist = manhattan(target, enemyHint);
            score += std::max<score_t>(
                0, 3600 - static_cast<score_t>(hintDist) * 240);
            if (target == enemyHint) score += 2200;
            if (occupier == -1 && memory[idx(target)].seen &&
                terrain != TILE_CITY && hintDist > 6) {
                score -= 1200;
            }
        }

        score -= path.dist[idx(target)];

        const army_t requiredArmy =
            defense + 1 + (terrain == TILE_CITY ? 2 : 0);
        if (sourceArmy <= requiredArmy && score < 100000000LL) {
            score -=
                5000 + static_cast<score_t>(requiredArmy - sourceArmy + 1) * 90;
        }

        const TileView& stepTile = board.tileAt(firstStep);
        if (stepTile.visible && !isFriendly(stepTile.occupier)) {
            if (sourceArmy - 1 <= stepTile.army &&
                !(stepTile.type == TILE_GENERAL &&
                  isEnemy(stepTile.occupier))) {
                return kNegInf;
            }
        }

        if (source == myGeneral) score -= crowded ? 800 : 2600;
        if (src.type == TILE_CITY && sourceArmy <= 4) score -= 1800;
        if (generalDist[idx(source)] <= 2 &&
            generalDist[idx(firstStep)] > generalDist[idx(source)])
            score -= 900;
        if (enemyPressure[idx(firstStep)] >
            friendlyPressure[idx(firstStep)] + (sourceArmy - 1) * 6) {
            score -=
                1800 + static_cast<score_t>(enemyPressure[idx(firstStep)] -
                                            friendlyPressure[idx(firstStep)] -
                                            (sourceArmy - 1) * 6) *
                           8;
        }
        if (source == lastMoveTo && firstStep == lastMoveFrom) score -= 400;
        if (terrainAt(firstStep) == TILE_SWAMP) score -= 1200;

        return score;
    }

    std::optional<Move> chooseGlobalStrategicMove(
        const std::vector<int>& unknownAdj, const std::vector<int>& enemyAdj,
        const std::vector<int>& enemyPressure,
        const std::vector<int>& friendlyPressure,
        const std::vector<int>& generalDist, bool crowded, bool huge,
        bool opening, bool enemyGeneralKnown, bool duelMode, int mapArea,
        Coord enemyHint, Coord& chosenSource, Coord& chosenTarget) const {
        auto sources =
            enumerateSources(unknownAdj, enemyAdj, generalDist, enemyPressure,
                             friendlyPressure, crowded);
        StrategicCandidate best;
        for (Coord source : sources) {
            PathMap path = buildPathMap(source, false);
            for (pos_t x = 1; x <= height; ++x) {
                for (pos_t y = 1; y <= width; ++y) {
                    Coord target{x, y};
                    if (!passableForPlan(target)) continue;
                    if (isFriendly(occupierAt(target))) continue;
                    Coord firstStep = firstStepOnPath(path, source, target);
                    if (firstStep == source) continue;
                    score_t score = evaluateTargetValue(
                        source, target, firstStep, path, unknownAdj, enemyAdj,
                        enemyPressure, friendlyPressure, generalDist, crowded,
                        huge, opening, enemyGeneralKnown, duelMode, mapArea,
                        enemyHint);
                    if (score > best.score) {
                        best.score = score;
                        best.source = source;
                        best.target = target;
                        best.move =
                            Move(MoveType::MOVE_ARMY, source, firstStep, false);
                    }
                }
            }
        }
        if (best.score <= 0) return std::nullopt;
        chosenSource = best.source;
        chosenTarget = best.target;
        return best.move;
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
                if (from == lastMoveTo && step == lastMoveFrom) score -= 250;
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
        score_t bestScore = kNegInf;
        std::optional<Move> bestMove;
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
                score += 320;
            } else if (dst.occupier == -1) {
                if (board.tileAt(focus).army - 1 > dst.army) {
                    score += 220 - dst.army * 10;
                    if (dst.type == TILE_DESERT) score += 30;
                    if (dst.type == TILE_SWAMP) score -= 500;
                }
            } else if (isFriendly(dst.occupier)) {
                score += crowded ? 40 : 80;
            } else if (board.tileAt(focus).army - 1 > dst.army) {
                score += 550 + (board.tileAt(focus).army - 1 - dst.army) * 20;
            }
            if (to == lastMoveFrom) score -= 120;
            if (score > bestScore) {
                bestScore = score;
                bestMove = Move(MoveType::MOVE_ARMY, focus, to, false);
            }
        }
        if (bestScore > 0) return bestMove;
        return std::nullopt;
    }

   public:
    void init(index_t playerId, const GameConstantsPack& constants) override {
        id = playerId;
        playerCount = constants.playerCount;
        height = constants.mapHeight;
        width = constants.mapWidth;
        stride = width + 2;
        config = constants.config;
        teams = constants.teams;

        halfTurn = fullTurn = 0;
        memory.assign((height + 2) * stride, MemoryCell{});
        knownGenerals.assign(playerCount, Coord{-1, -1});
        predictedGeneralScore.assign(
            playerCount, std::vector<double>((height + 2) * stride, 0.0));
        candidateGeneralMask.assign(
            playerCount, std::vector<uint8_t>((height + 2) * stride, 0));
        rankById.assign(playerCount, RankItem{});
        myGeneral = Coord{-1, -1};
        lastMoveFrom = Coord{-1, -1};
        lastMoveTo = Coord{-1, -1};
        lastStrategicTarget = Coord{-1, -1};
    }

    void requestMove(const BoardView& boardView,
                     const std::vector<RankItem>& rank) override {
        ++halfTurn;
        fullTurn += (halfTurn & 1);

        board = boardView;
        syncRank(rank);
        updateMemory();
        updateGeneralPredictions();
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

        bool defenseMode = false;
        bool forceDefense = false;
        if (inside(myGeneral)) {
            int defenseMargin = crowded && playerCount >= 6 ? 12 : 30;
            defenseMode = enemyPressure[idx(myGeneral)] >
                          friendlyPressure[idx(myGeneral)] + defenseMargin;
            if (crowded && playerCount >= 6 && generalThreat.present) {
                army_t generalArmy = board.tileAt(myGeneral).army;
                forceDefense = generalThreat.dist <= 2 ||
                               (generalThreat.dist <= 4 &&
                                generalThreat.army + 2 >=
                                    std::max<army_t>(1, generalArmy));
                defenseMode = defenseMode || forceDefense;
            }
        }

        if (auto forcedWin = chooseForcedWinMove()) {
            lastMoveFrom = forcedWin->from;
            lastMoveTo = forcedWin->to;
            moveQueue.push_back(*forcedWin);
            return;
        }

        if (auto defense =
                chooseDefenseMove(enemyPressure, friendlyPressure, generalDist,
                                  generalThreat, forceDefense)) {
            lastMoveFrom = defense->from;
            lastMoveTo = defense->to;
            moveQueue.push_back(*defense);
            return;
        }

        bool enemyGeneralKnown = false;
        int aliveEnemies = 0;
        index_t duelEnemy = -1;
        for (index_t player = 0; player < playerCount; ++player) {
            if (player == id || !isAlive(player)) continue;
            ++aliveEnemies;
            duelEnemy = player;
            if (inside(knownGenerals[player])) enemyGeneralKnown = true;
        }
        const bool duelMode = aliveEnemies == 1;
        Coord enemyHint{-1, -1};
        if (duelMode && validPlayer(duelEnemy)) {
            enemyHint = choosePredictedGeneral(duelEnemy);
            if (inside(enemyHint)) {
                enemyGeneralKnown = true;
            }
        }

        if (crowded && aliveEnemies >= 4 && fullTurn <= 12) {
            if (auto openingMove = chooseCrowdedOpeningMove(
                    unknownAdj, enemyAdj, enemyPressure, friendlyPressure)) {
                lastMoveFrom = openingMove->from;
                lastMoveTo = openingMove->to;
                moveQueue.push_back(*openingMove);
                return;
            }
        }

        if (crowded && aliveEnemies >= 4) {
            if (auto skirmish = chooseCrowdedSkirmishMove(
                    unknownAdj, enemyAdj, enemyPressure, friendlyPressure)) {
                lastMoveFrom = skirmish->from;
                lastMoveTo = skirmish->to;
                moveQueue.push_back(*skirmish);
                return;
            }
        }

        if (auto tactical = chooseImmediateTacticalMove(crowded)) {
            lastMoveFrom = tactical->from;
            lastMoveTo = tactical->to;
            moveQueue.push_back(*tactical);
            return;
        }

        Coord focus{-1, -1}, target{-1, -1};
        if (auto strategic = chooseGlobalStrategicMove(
                unknownAdj, enemyAdj, enemyPressure, friendlyPressure,
                generalDist, crowded, huge, opening, enemyGeneralKnown,
                duelMode, mapArea, enemyHint, focus, target)) {
            lastMoveFrom = strategic->from;
            lastMoveTo = strategic->to;
            lastStrategicTarget = target;
            moveQueue.push_back(*strategic);
            return;
        }

        if (!inside(focus)) {
            auto sources =
                enumerateSources(unknownAdj, enemyAdj, generalDist,
                                 enemyPressure, friendlyPressure, crowded);
            if (!sources.empty()) focus = sources.front();
        }

        const PathMap gatherPath = buildPathMap(focus, true);
        if (auto gather = gatherToward(focus, gatherPath, crowded)) {
            lastMoveFrom = gather->from;
            lastMoveTo = gather->to;
            moveQueue.push_back(*gather);
            return;
        }

        if (auto fallback = chooseFallback(focus, crowded)) {
            lastMoveFrom = fallback->from;
            lastMoveTo = fallback->to;
            moveQueue.push_back(*fallback);
        }
    }
};

static BotRegistrar<XiaruizeBot> xiaruizeBot_reg("XiaruizeBot");
