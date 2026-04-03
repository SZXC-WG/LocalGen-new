// Copyright (C) 2026 pinkHC
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file LyBot.cpp
 *
 * LyBot is a multiplayer-oriented built-in bot for LocalGen v6.
 *
 * @author pinkHC
 */

#ifndef LGEN_BOTS_LYBOT
#define LGEN_BOTS_LYBOT

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <deque>
#include <functional>
#include <limits>
#include <numeric>
#include <optional>
#include <queue>
#include <random>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/bot.h"
#include "core/game.hpp"

class LyBot : public BasicBot {
   private:
    static constexpr std::array<Coord, 4> kDirs = {Coord{-1, 0}, Coord{0, -1},
                                                   Coord{1, 0}, Coord{0, 1}};
    static constexpr int kInf = 1e9;
    static constexpr int kRecentEdgeWindow = 16;

    struct TileMemory {
        tile_type_e type = TILE_BLANK;
        army_t army = 0;
        index_t occupier = -1;
        bool everSeen = false;
        bool visible = false;
        int lastSeenTurn = -1;
    };

    struct SearchWorkspace {
        std::vector<int> dist;
        std::vector<int> stamp;
        std::vector<Coord> parent;
        std::vector<std::pair<int, Coord>> heap;
        int activeStamp = 1;
    };

    struct PathResult {
        bool reachable = false;
        const std::vector<int>* dist = nullptr;
        const std::vector<int>* stamp = nullptr;
        const std::vector<Coord>* parent = nullptr;
        int activeStamp = 0;

        int distance(size_t node) const {
            if (dist == nullptr || stamp == nullptr || node >= dist->size())
                return kInf;
            return (*stamp)[node] == activeStamp ? (*dist)[node] : kInf;
        }

        Coord parentAt(size_t node) const {
            if (parent == nullptr || stamp == nullptr || node >= parent->size())
                return Coord{-1, -1};
            return (*stamp)[node] == activeStamp ? (*parent)[node]
                                                 : Coord{-1, -1};
        }
    };

    struct CandidateMove {
        Move move{};
        double score = -1e100;
        bool valid = false;
    };

    struct ThreatInfo {
        Coord enemy{-1, -1};
        Coord intercept{-1, -1};
        std::vector<Coord> route;
        army_t enemyArmy = 0;
        double score = -1e100;
    };

    struct RecentEdge {
        Coord from{-1, -1};
        Coord to{-1, -1};
        std::uint64_t key = 0;
    };

    pos_t height = 0;
    pos_t width = 0;
    pos_t W = 0;
    index_t id = -1;
    index_t playerCnt = 0;
    turn_t halfTurn = 0;
    turn_t fullTurn = 0;
    config::Config config;
    std::vector<index_t> teams;

    BoardView board;
    std::vector<RankItem> rankById;
    std::vector<RankItem> prevRankById;
    bool hasPrevRank = false;
    std::vector<army_t> playerArmyDelta;
    std::vector<pos_t> playerLandDelta;
    std::vector<bool> aliveById;

    std::vector<TileMemory> memory;
    std::vector<Coord> knownGenerals;
    std::vector<std::vector<double>> predictedGeneralScore;
    std::vector<std::vector<uint8_t>> candidateGeneralMask;
    std::vector<int> newlyVisibleEnemyTiles;

    std::vector<Coord> friendlyTilesCache;
    std::vector<Coord> frontierTiles;
    std::vector<Coord> ownedCitiesCache;
    std::vector<Coord> objectiveCandidates;
    std::vector<int> distFromGeneral;
    std::vector<double> enemyPressure;
    std::vector<int> pathFlow;
    std::vector<army_t> reserveArmy;
    std::vector<uint8_t> coreZoneMask;
    std::vector<uint8_t> chokeMask;
    army_t largestFriendlyArmyCache = 0;
    Coord myGeneral{-1, -1};

    index_t currentTargetPlayer = -1;
    Coord currentObjective{-1, -1};
    Coord lockedObjective{-1, -1};
    int objectiveLockUntil = -1;
    int targetLockUntil = -1;
    int lastHeavyRefreshTurn = -1;
    int lastAliveCount = -1;
    bool sawNewGeneralThisTurn = false;
    bool previousGeneralThreatState = false;
    Coord lastMoveFrom{-1, -1};
    Coord lastMoveTo{-1, -1};

    SearchWorkspace forwardSearch;
    std::deque<RecentEdge> recentEdges;
    std::unordered_map<std::uint64_t, int> recentEdgeCounts;
    std::mt19937 rng{std::random_device{}()};

    inline size_t idx(Coord c) const {
        return static_cast<size_t>(c.x * W + c.y);
    }
    inline size_t idx(pos_t x, pos_t y) const {
        return static_cast<size_t>(x * W + y);
    }

    inline bool inside(Coord c) const {
        return c.x >= 1 && c.x <= height && c.y >= 1 && c.y <= width;
    }

    inline bool isPassable(tile_type_e type) const {
        return !isImpassableTile(type) && type != TILE_OBSTACLE;
    }

    inline bool isFriendlyOccupier(index_t occupier) const {
        return occupier == id ||
               (occupier >= 0 &&
                occupier < static_cast<index_t>(teams.size()) &&
                teams[occupier] == teams[id]);
    }

    inline bool isFriendlyTile(const TileView& tile) const {
        return isFriendlyOccupier(tile.occupier);
    }

    inline bool isEnemyTile(const TileView& tile) const {
        return tile.occupier >= 0 && !isFriendlyOccupier(tile.occupier);
    }

    inline int manhattan(Coord a, Coord b) const {
        return std::abs(a.x - b.x) + std::abs(a.y - b.y);
    }

    inline std::uint64_t edgeKey(Coord a, Coord b) const {
        std::uint64_t lhs = static_cast<std::uint64_t>(idx(a));
        std::uint64_t rhs = static_cast<std::uint64_t>(idx(b));
        if (lhs > rhs) std::swap(lhs, rhs);
        return (lhs << 32U) | rhs;
    }

    int passableDegree(Coord c) const {
        int degree = 0;
        for (Coord d : kDirs) {
            Coord nxt = c + d;
            if (inside(nxt) && isPassable(memory[idx(nxt)].type)) ++degree;
        }
        return degree;
    }

    void prepareWorkspace(SearchWorkspace& workspace) const {
        const size_t total = static_cast<size_t>((height + 2) * W);
        if (workspace.dist.size() != total) {
            workspace.dist.assign(total, kInf);
            workspace.stamp.assign(total, 0);
            workspace.parent.assign(total, Coord{-1, -1});
            workspace.heap.reserve(total);
        }
        if (workspace.activeStamp == std::numeric_limits<int>::max()) {
            std::fill(workspace.stamp.begin(), workspace.stamp.end(), 0);
            workspace.activeStamp = 1;
        } else {
            ++workspace.activeStamp;
        }
        workspace.heap.clear();
    }

    void touchNode(SearchWorkspace& workspace, size_t node) const {
        if (workspace.stamp[node] == workspace.activeStamp) return;
        workspace.stamp[node] = workspace.activeStamp;
        workspace.dist[node] = kInf;
        workspace.parent[node] = Coord{-1, -1};
    }

    static bool minHeapCompare(const std::pair<int, Coord>& lhs,
                               const std::pair<int, Coord>& rhs) {
        return lhs.first > rhs.first;
    }

    PathResult weightedPath(Coord start,
                            const std::function<int(Coord)>& stepCost) {
        PathResult result;
        if (!inside(start)) return result;

        prepareWorkspace(forwardSearch);
        const size_t startIndex = idx(start);
        touchNode(forwardSearch, startIndex);
        forwardSearch.dist[startIndex] = 0;
        forwardSearch.heap.emplace_back(0, start);
        std::push_heap(forwardSearch.heap.begin(), forwardSearch.heap.end(),
                       minHeapCompare);

        while (!forwardSearch.heap.empty()) {
            std::pop_heap(forwardSearch.heap.begin(), forwardSearch.heap.end(),
                          minHeapCompare);
            const auto [curDist, cur] = forwardSearch.heap.back();
            forwardSearch.heap.pop_back();
            if (curDist != forwardSearch.dist[idx(cur)]) continue;

            for (Coord d : kDirs) {
                Coord nxt = cur + d;
                if (!inside(nxt) || !isPassable(memory[idx(nxt)].type))
                    continue;
                const size_t nextIndex = idx(nxt);
                touchNode(forwardSearch, nextIndex);
                const int nd = curDist + stepCost(nxt);
                if (nd < forwardSearch.dist[nextIndex]) {
                    forwardSearch.dist[nextIndex] = nd;
                    forwardSearch.parent[nextIndex] = cur;
                    forwardSearch.heap.emplace_back(nd, nxt);
                    std::push_heap(forwardSearch.heap.begin(),
                                   forwardSearch.heap.end(), minHeapCompare);
                }
            }
        }

        result.reachable = true;
        result.dist = &forwardSearch.dist;
        result.stamp = &forwardSearch.stamp;
        result.parent = &forwardSearch.parent;
        result.activeStamp = forwardSearch.activeStamp;
        return result;
    }

    std::vector<Coord> reconstructPath(Coord start, Coord goal,
                                       const PathResult& path) const {
        std::vector<Coord> rev;
        if (!inside(start) || !inside(goal)) return rev;
        Coord cur = goal;
        while (cur != Coord{-1, -1} && cur != start) {
            rev.push_back(cur);
            cur = path.parentAt(idx(cur));
        }
        if (cur != start) return {};
        std::reverse(rev.begin(), rev.end());
        return rev;
    }

    army_t estimatedArmyAt(Coord c) const {
        const TileView& tile = board.tileAt(c);
        return tile.visible ? tile.army : memory[idx(c)].army;
    }

    void refreshRank(const std::vector<RankItem>& rank) {
        if (!rankById.empty()) {
            prevRankById = rankById;
            hasPrevRank = true;
        }
        rankById.assign(playerCnt, RankItem{});
        for (const RankItem& item : rank) {
            if (item.player >= 0 && item.player < playerCnt) {
                rankById[item.player] = item;
            }
        }
        aliveById.assign(playerCnt, false);
        playerArmyDelta.assign(playerCnt, 0);
        playerLandDelta.assign(playerCnt, 0);
        for (index_t player = 0; player < playerCnt; ++player) {
            aliveById[player] = rankById[player].alive;
            if (hasPrevRank) {
                playerArmyDelta[player] =
                    rankById[player].army - prevRankById[player].army;
                playerLandDelta[player] =
                    rankById[player].land - prevRankById[player].land;
            }
        }
    }

    void rebuildTurnCaches() {
        friendlyTilesCache.clear();
        frontierTiles.clear();
        ownedCitiesCache.clear();
        largestFriendlyArmyCache = 0;
        myGeneral = Coord{-1, -1};

        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                const TileView& tile = board.tileAt(c);
                if (tile.occupier != id) continue;
                friendlyTilesCache.push_back(c);
                largestFriendlyArmyCache =
                    std::max(largestFriendlyArmyCache, tile.army);
                if (tile.type == TILE_CITY) ownedCitiesCache.push_back(c);
                if (tile.type == TILE_GENERAL) myGeneral = c;
                bool boundary = false;
                for (Coord d : kDirs) {
                    Coord nxt = c + d;
                    if (!inside(nxt)) continue;
                    if (!isFriendlyOccupier(board.tileAt(nxt).occupier)) {
                        boundary = true;
                        break;
                    }
                }
                if (boundary) frontierTiles.push_back(c);
            }
        }
    }

    void decayPredictions() {
        for (index_t player = 0; player < playerCnt; ++player) {
            if (player == id) continue;
            for (double& score : predictedGeneralScore[player]) score *= 0.985;
        }
    }

    void resetCandidateMask(index_t player) {
        auto& mask = candidateGeneralMask[player];
        std::fill(mask.begin(), mask.end(), 0);
        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                const TileMemory& mem = memory[idx(c)];
                if (!isPassable(mem.type) || mem.visible ||
                    mem.type == TILE_CITY)
                    continue;
                if (myGeneral != Coord{-1, -1} && manhattan(c, myGeneral) < 11)
                    continue;
                mask[idx(c)] = 1;
            }
        }
    }

    void reinforceGeneralNeighborhood(index_t player, Coord anchor,
                                      double baseScore, int maxDist,
                                      bool fogOnly) {
        auto& mask = candidateGeneralMask[player];
        auto& heat = predictedGeneralScore[player];
        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                const TileMemory& mem = memory[idx(c)];
                if (!isPassable(mem.type) || mem.type == TILE_CITY) continue;
                if (fogOnly && mem.visible) continue;
                if (manhattan(c, anchor) > maxDist) continue;
                if (myGeneral != Coord{-1, -1} && manhattan(c, myGeneral) < 11)
                    continue;
                mask[idx(c)] = 1;
                heat[idx(c)] +=
                    std::max(0.0, baseScore - manhattan(c, anchor) * 2.1);
            }
        }
    }

    void rememberVisibleBoard() {
        decayPredictions();
        sawNewGeneralThisTurn = false;
        newlyVisibleEnemyTiles.assign(playerCnt, 0);

        for (index_t player = 0; player < playerCnt; ++player) {
            if (player == id) continue;
            if (knownGenerals[player] == Coord{-1, -1})
                resetCandidateMask(player);
        }

        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                const TileView& tile = board.tileAt(c);
                TileMemory& mem = memory[idx(c)];
                const bool wasVisible = mem.visible;
                mem.visible = tile.visible;
                if (!tile.visible) {
                    if (!mem.everSeen) mem.type = tile.type;
                    if (mem.occupier >= 0 &&
                        !isFriendlyOccupier(mem.occupier) && mem.army > 0 &&
                        mem.lastSeenTurn >= 0 &&
                        static_cast<int>(fullTurn) - mem.lastSeenTurn <= 4) {
                        mem.army = std::max<army_t>(1, mem.army - 1);
                    }
                    continue;
                }

                mem.everSeen = true;
                mem.type = tile.type;
                mem.army = tile.army;
                mem.occupier = tile.occupier;
                mem.lastSeenTurn = static_cast<int>(fullTurn);

                if (tile.type == TILE_GENERAL && tile.occupier >= 0) {
                    if (knownGenerals[tile.occupier] != c)
                        sawNewGeneralThisTurn = true;
                    knownGenerals[tile.occupier] = c;
                    auto& heat = predictedGeneralScore[tile.occupier];
                    auto& mask = candidateGeneralMask[tile.occupier];
                    std::fill(heat.begin(), heat.end(), 0.0);
                    std::fill(mask.begin(), mask.end(), 0);
                    heat[idx(c)] = 1e9;
                    mask[idx(c)] = 1;
                }

                if (tile.occupier >= 0 && !isFriendlyOccupier(tile.occupier) &&
                    !wasVisible) {
                    newlyVisibleEnemyTiles[tile.occupier]++;
                    reinforceGeneralNeighborhood(tile.occupier, c, 18.0, 8,
                                                 true);
                }
            }
        }

        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                const TileView& tile = board.tileAt(c);
                if (!tile.visible || tile.occupier < 0 ||
                    isFriendlyOccupier(tile.occupier))
                    continue;
                reinforceGeneralNeighborhood(tile.occupier, c, 9.0, 5, false);
            }
        }
    }

    Coord chooseTargetPlayerGeneral(index_t player) const {
        if (player < 0 || player >= playerCnt) return Coord{-1, -1};
        if (knownGenerals[player] != Coord{-1, -1})
            return knownGenerals[player];
        double bestScore = -1e100;
        Coord best{-1, -1};
        const auto& heat = predictedGeneralScore[player];
        const auto& mask = candidateGeneralMask[player];
        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                const TileMemory& mem = memory[idx(c)];
                if (!isPassable(mem.type) || mem.visible ||
                    mem.type == TILE_CITY)
                    continue;
                if (!mask[idx(c)]) continue;
                double score = heat[idx(c)];
                if (myGeneral != Coord{-1, -1})
                    score -= manhattan(c, myGeneral) * 0.12;
                if (score > bestScore) {
                    bestScore = score;
                    best = c;
                }
            }
        }
        return best;
    }

    void computeEnemyPressure() {
        std::fill(enemyPressure.begin(), enemyPressure.end(), 0.0);
        const int range = height * width >= 1600 ? 4 : 5;
        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord enemy{x, y};
                const TileView& tile = board.tileAt(enemy);
                if (!isEnemyTile(tile)) continue;
                for (int dx = -range; dx <= range; ++dx) {
                    for (int dy = -range; dy <= range; ++dy) {
                        Coord c{static_cast<pos_t>(enemy.x + dx),
                                static_cast<pos_t>(enemy.y + dy)};
                        if (!inside(c)) continue;
                        const int dist = std::abs(dx) + std::abs(dy);
                        if (dist > range) continue;
                        enemyPressure[idx(c)] = std::max(
                            enemyPressure[idx(c)],
                            static_cast<double>(tile.army) - dist * 2.2);
                    }
                }
            }
        }
    }

    void computeGeneralDistances() {
        std::fill(distFromGeneral.begin(), distFromGeneral.end(), kInf);
        if (myGeneral == Coord{-1, -1}) return;
        std::queue<Coord> q;
        q.push(myGeneral);
        distFromGeneral[idx(myGeneral)] = 0;
        while (!q.empty()) {
            Coord cur = q.front();
            q.pop();
            for (Coord d : kDirs) {
                Coord nxt = cur + d;
                if (!inside(nxt) || !isPassable(memory[idx(nxt)].type))
                    continue;
                if (distFromGeneral[idx(nxt)] != kInf) continue;
                distFromGeneral[idx(nxt)] = distFromGeneral[idx(cur)] + 1;
                q.push(nxt);
            }
        }
    }

    void recomputeDefenseZones() {
        std::fill(coreZoneMask.begin(), coreZoneMask.end(), 0);
        std::fill(chokeMask.begin(), chokeMask.end(), 0);
        std::fill(reserveArmy.begin(), reserveArmy.end(), 0);
        std::fill(pathFlow.begin(), pathFlow.end(), 0);
        if (myGeneral == Coord{-1, -1}) return;

        computeGeneralDistances();
        const int area = static_cast<int>(height * width);
        const int coreRadius = area >= 1600 ? 12 : (area >= 900 ? 10 : 8);

        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                if (distFromGeneral[idx(c)] <= coreRadius)
                    coreZoneMask[idx(c)] = 1;
            }
        }
        for (Coord city : ownedCitiesCache) {
            if (distFromGeneral[idx(city)] <= coreRadius + 3) {
                coreZoneMask[idx(city)] = 1;
            }
        }

        for (Coord frontier : frontierTiles) {
            if (distFromGeneral[idx(frontier)] == kInf) continue;
            Coord cur = frontier;
            int guard = height * width + 5;
            while (cur != myGeneral && guard-- > 0) {
                pathFlow[idx(cur)]++;
                Coord next = cur;
                int bestDist = distFromGeneral[idx(cur)];
                double bestPressure = enemyPressure[idx(cur)];
                for (Coord d : kDirs) {
                    Coord cand = cur + d;
                    if (!inside(cand) || distFromGeneral[idx(cand)] >= bestDist)
                        continue;
                    if (!isPassable(memory[idx(cand)].type)) continue;
                    if (distFromGeneral[idx(cand)] < bestDist ||
                        enemyPressure[idx(cand)] < bestPressure) {
                        next = cand;
                        bestDist = distFromGeneral[idx(cand)];
                        bestPressure = enemyPressure[idx(cand)];
                    }
                }
                if (next == cur) break;
                cur = next;
            }
            pathFlow[idx(myGeneral)]++;
        }

        for (Coord c : friendlyTilesCache) {
            const TileView& tile = board.tileAt(c);
            army_t reserve = 0;
            if (c == myGeneral) {
                reserve = area >= 1600 ? 9 : 7;
                reserve += static_cast<army_t>(
                    std::max(0.0, enemyPressure[idx(c)] * 0.12));
            }
            if (tile.type == TILE_CITY &&
                distFromGeneral[idx(c)] <= coreRadius + 4) {
                reserve = std::max<army_t>(
                    reserve, 2 + static_cast<army_t>(std::max(
                                     0.0, enemyPressure[idx(c)] * 0.08)));
            }
            const int flow = pathFlow[idx(c)];
            const int degree = passableDegree(c);
            if (flow >=
                    std::max(2, static_cast<int>(frontierTiles.size() / 12)) &&
                degree <= 2 && distFromGeneral[idx(c)] >= 2 &&
                distFromGeneral[idx(c)] <= coreRadius + 6) {
                chokeMask[idx(c)] = 1;
                reserve = std::max<army_t>(
                    reserve, 1 + flow / 3 +
                                 static_cast<army_t>(std::max(
                                     0.0, enemyPressure[idx(c)] * 0.05)));
            }
            if (coreZoneMask[idx(c)] &&
                enemyPressure[idx(c)] >
                    std::max(4.0, rankById[id].army * 0.01)) {
                reserve = std::max<army_t>(
                    reserve, static_cast<army_t>(enemyPressure[idx(c)] * 0.05));
            }
            reserveArmy[idx(c)] = reserve;
        }
    }

    bool hasGeneralThreatNow() const {
        if (myGeneral == Coord{-1, -1}) return false;
        return enemyPressure[idx(myGeneral)] >
               board.tileAt(myGeneral).army * 0.6;
    }

    bool shouldRunHeavyRefresh() const {
        if (playerCnt < 3) return false;
        const int aliveCount = static_cast<int>(
            std::count(aliveById.begin(), aliveById.end(), true));
        const int area = static_cast<int>(height * width);
        const int interval = area >= 1600 ? 8 : (area >= 900 ? 6 : 4);
        if (lastHeavyRefreshTurn < 0) return true;
        if (aliveCount != lastAliveCount) return true;
        if (sawNewGeneralThisTurn) return true;
        if (previousGeneralThreatState != hasGeneralThreatNow()) return true;
        if (currentObjective == Coord{-1, -1}) return true;
        if (inside(currentObjective) &&
            board.tileAt(currentObjective).occupier == id)
            return true;
        if (fullTurn - lastHeavyRefreshTurn >= interval) return true;
        return false;
    }

    double targetPlayerScore(index_t player) const {
        if (player < 0 || player >= playerCnt || player == id ||
            !aliveById[player] || teams[player] == teams[id]) {
            return -1e100;
        }
        const army_t myArmy = rankById[id].army;
        const pos_t myLand = rankById[id].land;
        int weakSignals = 0;
        double score = 0.0;
        if (myArmy >= rankById[player].army +
                          std::max<army_t>(10, rankById[player].army / 5)) {
            ++weakSignals;
            score += 80.0;
        }
        if (myLand >= rankById[player].land +
                          std::max<pos_t>(6, rankById[player].land / 4)) {
            ++weakSignals;
            score += 65.0;
        }
        if (playerArmyDelta[player] < -2 && playerLandDelta[player] < 0) {
            ++weakSignals;
            score += 75.0;
        }
        Coord known = chooseTargetPlayerGeneral(player);
        if (known != Coord{-1, -1}) {
            ++weakSignals;
            score += 55.0;
            if (myGeneral != Coord{-1, -1}) {
                const int dist = manhattan(known, myGeneral);
                if (dist <= (height * width >= 1600 ? 24 : 18)) {
                    ++weakSignals;
                    score += std::max(0.0, 45.0 - dist * 1.2);
                }
            }
        }
        if (newlyVisibleEnemyTiles[player] > 0)
            score += newlyVisibleEnemyTiles[player] * 12.0;
        if (weakSignals < 2) score -= 120.0;
        score += std::max<army_t>(0, -playerArmyDelta[player]) * 1.4;
        score += std::max<pos_t>(0, -playerLandDelta[player]) * 5.0;
        return score;
    }

    index_t chooseWeakTargetPlayer() const {
        index_t best = -1;
        double bestScore = -1e100;
        for (index_t player = 0; player < playerCnt; ++player) {
            const double score = targetPlayerScore(player);
            if (score > bestScore) {
                bestScore = score;
                best = player;
            }
        }
        return bestScore > -100.0 ? best : -1;
    }

    index_t chooseFallbackTargetPlayer() const {
        index_t best = -1;
        double bestScore = -1e100;
        for (index_t player = 0; player < playerCnt; ++player) {
            if (player == id || !aliveById[player] ||
                teams[player] == teams[id])
                continue;
            double score = 0.0;
            Coord guess = chooseTargetPlayerGeneral(player);
            if (guess != Coord{-1, -1} && myGeneral != Coord{-1, -1}) {
                score +=
                    std::max(0.0, 90.0 - manhattan(guess, myGeneral) * 2.0);
            }
            score += std::max<army_t>(0, 220 - rankById[player].army) * 0.25;
            score += std::max<pos_t>(0, 130 - rankById[player].land) * 0.4;
            if (score > bestScore) {
                bestScore = score;
                best = player;
            }
        }
        return best;
    }

    index_t chooseLockedTargetPlayer() {
        index_t suggested = chooseWeakTargetPlayer();
        if (suggested < 0) suggested = chooseFallbackTargetPlayer();
        if (suggested < 0) return -1;
        if (currentTargetPlayer < 0 || !aliveById[currentTargetPlayer] ||
            fullTurn > static_cast<turn_t>(targetLockUntil)) {
            currentTargetPlayer = suggested;
            targetLockUntil =
                static_cast<int>(fullTurn) + (height * width >= 1600 ? 16 : 12);
            return currentTargetPlayer;
        }
        const double currentScore = targetPlayerScore(currentTargetPlayer);
        const double nextScore = targetPlayerScore(suggested);
        if (suggested != currentTargetPlayer &&
            nextScore > currentScore + 70.0) {
            currentTargetPlayer = suggested;
            targetLockUntil =
                static_cast<int>(fullTurn) + (height * width >= 1600 ? 16 : 12);
        }
        return currentTargetPlayer;
    }

    army_t availableFullMoveArmy(Coord from) const {
        const TileView& tile = board.tileAt(from);
        if (tile.occupier != id || tile.army <= 1) return 0;
        return tile.army - 1 - reserveArmy[idx(from)];
    }

    std::optional<Move> buildReserveAwareMove(Coord from, Coord to,
                                              bool tacticalPreferred) const {
        if (!inside(from) || !inside(to)) return std::nullopt;
        const TileView& src = board.tileAt(from);
        if (src.occupier != id || src.army <= 1) return std::nullopt;
        if (isImpassableTile(board.tileAt(to).type)) return std::nullopt;

        const army_t reserve = reserveArmy[idx(from)];
        const army_t remainFull = 1;
        const army_t remainHalf = src.army - (src.army >> 1);
        const army_t fullAttack = src.army - 1;
        const army_t halfAttack = src.army >> 1;

        if (tacticalPreferred && remainFull >= reserve) {
            return Move(MoveType::MOVE_ARMY, from, to, false);
        }
        if (remainFull >= reserve && reserve == 0) {
            return Move(MoveType::MOVE_ARMY, from, to, false);
        }
        if (remainFull >= reserve &&
            reserve <= std::max<army_t>(1, src.army / 6) &&
            !chokeMask[idx(from)] && from != myGeneral) {
            return Move(MoveType::MOVE_ARMY, from, to, false);
        }
        if (remainFull < reserve && reserve <= 2 && !chokeMask[idx(from)] &&
            from != myGeneral && src.type != TILE_CITY &&
            enemyPressure[idx(from)] < 3.0) {
            return Move(MoveType::MOVE_ARMY, from, to, false);
        }
        if (remainHalf >= reserve && halfAttack > 0) {
            return Move(MoveType::MOVE_ARMY, from, to, true);
        }
        if (remainFull >= reserve && fullAttack > 0) {
            return Move(MoveType::MOVE_ARMY, from, to, false);
        }
        return std::nullopt;
    }

    bool shouldBlockOscillation(Coord from, Coord to) const {
        auto it = recentEdgeCounts.find(edgeKey(from, to));
        const int seen = it == recentEdgeCounts.end() ? 0 : it->second;
        if (from == lastMoveTo && to == lastMoveFrom &&
            board.tileAt(to).occupier == id)
            return true;
        return seen >= 3 && board.tileAt(to).occupier == id;
    }

    bool moveLooksSafe(const Move& move) const {
        if (move.type != MoveType::MOVE_ARMY || !inside(move.from) ||
            !inside(move.to))
            return false;
        const TileView& src = board.tileAt(move.from);
        if (src.occupier != id || src.army <= 1) return false;
        const army_t remain = move.takeHalf ? (src.army - (src.army >> 1)) : 1;
        if (remain < reserveArmy[idx(move.from)]) return false;
        if (move.from == myGeneral) {
            if (remain <=
                std::max<army_t>(
                    1, static_cast<army_t>(enemyPressure[idx(myGeneral)])))
                return false;
        }
        return true;
    }

    void pushRecentMove(Coord from, Coord to) {
        const std::uint64_t key = edgeKey(from, to);
        recentEdges.push_back(RecentEdge{from, to, key});
        ++recentEdgeCounts[key];
        if (static_cast<int>(recentEdges.size()) > kRecentEdgeWindow) {
            const RecentEdge old = recentEdges.front();
            recentEdges.pop_front();
            auto it = recentEdgeCounts.find(old.key);
            if (it != recentEdgeCounts.end() && --it->second <= 0) {
                recentEdgeCounts.erase(it);
            }
        }
        lastMoveFrom = from;
        lastMoveTo = to;
    }

    CandidateMove chooseImmediateHarvestMove(index_t targetPlayer) const {
        CandidateMove best;
        for (Coord from : friendlyTilesCache) {
            const TileView& src = board.tileAt(from);
            if (src.army <= 1) continue;
            for (Coord d : kDirs) {
                Coord to = from + d;
                if (!inside(to)) continue;
                const TileView& dst = board.tileAt(to);
                if (isImpassableTile(dst.type) || isFriendlyTile(dst)) continue;
                if (shouldBlockOscillation(from, to)) continue;
                auto moveOpt = buildReserveAwareMove(from, to, true);
                if (!moveOpt.has_value()) continue;
                const army_t attack =
                    moveOpt->takeHalf ? (src.army >> 1) : (src.army - 1);
                const army_t defense = estimatedArmyAt(to);
                if (attack <= defense) continue;

                double score = 0.0;
                if (dst.type == TILE_GENERAL) score += 1e6;
                if (dst.type == TILE_CITY) score += 420.0 - defense;
                if (isEnemyTile(dst)) score += 180.0 + defense * 0.3;
                if (dst.occupier == targetPlayer) score += 70.0;
                if (!dst.visible) score += 25.0;
                if (!memory[idx(to)].everSeen) score += 25.0;
                if (dst.type == TILE_SWAMP) score -= 18.0;
                if (score > best.score && moveLooksSafe(*moveOpt)) {
                    best.valid = true;
                    best.score = score;
                    best.move = *moveOpt;
                }
            }
        }
        return best;
    }

    std::optional<ThreatInfo> analyzeGeneralThreat() {
        if (myGeneral == Coord{-1, -1}) return std::nullopt;
        std::vector<std::pair<double, Coord>> candidates;
        candidates.reserve(8);
        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                const TileView& tile = board.tileAt(c);
                if (!isEnemyTile(tile) || tile.army <= 1) continue;
                if (distFromGeneral[idx(c)] == kInf ||
                    distFromGeneral[idx(c)] > 16)
                    continue;
                double score = tile.army * 2.8 - distFromGeneral[idx(c)] * 7.0;
                if (score > 0) candidates.emplace_back(score, c);
            }
        }
        std::sort(candidates.begin(), candidates.end(),
                  [](const auto& lhs, const auto& rhs) {
                      return lhs.first > rhs.first;
                  });
        if (candidates.size() > 6) candidates.resize(6);

        std::optional<ThreatInfo> best;
        for (const auto& [baseScore, enemy] : candidates) {
            std::vector<Coord> route;
            Coord cur = enemy;
            int guard = height * width + 5;
            while (cur != myGeneral && guard-- > 0) {
                Coord next = cur;
                int bestDist = distFromGeneral[idx(cur)];
                double bestPressure = enemyPressure[idx(cur)];
                for (Coord d : kDirs) {
                    Coord cand = cur + d;
                    if (!inside(cand) || !isPassable(memory[idx(cand)].type))
                        continue;
                    if (distFromGeneral[idx(cand)] >= bestDist) continue;
                    if (distFromGeneral[idx(cand)] < bestDist ||
                        enemyPressure[idx(cand)] < bestPressure) {
                        next = cand;
                        bestDist = distFromGeneral[idx(cand)];
                        bestPressure = enemyPressure[idx(cand)];
                    }
                }
                if (next == cur) break;
                route.push_back(next);
                cur = next;
            }
            if (route.empty() || route.back() != myGeneral) continue;
            ThreatInfo info;
            info.enemy = enemy;
            info.route = route;
            info.intercept = route[std::min<std::size_t>(1, route.size() - 1)];
            info.enemyArmy = board.tileAt(enemy).army;
            info.score =
                baseScore + 150.0 - static_cast<double>(route.size()) * 10.0;
            if (!best.has_value() || info.score > best->score) best = info;
        }
        return best;
    }

    std::optional<Move> chooseDefenseMove() {
        auto threat = analyzeGeneralThreat();
        if (!threat.has_value()) return std::nullopt;
        const bool urgent =
            threat->enemyArmy >= board.tileAt(myGeneral).army - 1 ||
            threat->route.size() <= 5 ||
            enemyPressure[idx(myGeneral)] > board.tileAt(myGeneral).army * 0.7;
        if (!urgent) return std::nullopt;

        for (Coord d : kDirs) {
            Coord from = threat->intercept + d;
            if (!inside(from)) continue;
            const TileView& tile = board.tileAt(from);
            if (tile.occupier != id ||
                tile.army <= estimatedArmyAt(threat->intercept) + 1)
                continue;
            auto moveOpt = buildReserveAwareMove(from, threat->intercept, true);
            if (moveOpt.has_value() && moveLooksSafe(*moveOpt)) return moveOpt;
        }

        std::vector<std::pair<double, Coord>> sources;
        for (Coord source : friendlyTilesCache) {
            const TileView& tile = board.tileAt(source);
            if (tile.army <= reserveArmy[idx(source)] + 1) continue;
            double score = tile.army - manhattan(source, myGeneral) * 2.0 -
                           manhattan(source, threat->intercept) * 1.3;
            if (source == myGeneral) score -= 6.0;
            if (chokeMask[idx(source)]) score -= 8.0;
            sources.emplace_back(score, source);
        }
        std::sort(sources.begin(), sources.end(),
                  [](const auto& lhs, const auto& rhs) {
                      return lhs.first > rhs.first;
                  });
        if (sources.size() > 3) sources.resize(3);

        CandidateMove best;
        for (const auto& [_, source] : sources) {
            auto path = weightedPath(source, [&](Coord c) {
                int cost = 2;
                if (memory[idx(c)].type == TILE_SWAMP) cost += 4;
                return cost;
            });
            if (path.distance(idx(threat->intercept)) >= kInf) continue;
            std::vector<Coord> route =
                reconstructPath(source, threat->intercept, path);
            if (route.empty()) continue;
            auto moveOpt = buildReserveAwareMove(source, route.front(), false);
            if (!moveOpt.has_value() || !moveLooksSafe(*moveOpt)) continue;
            double score = board.tileAt(source).army * 2.0 -
                           path.distance(idx(threat->intercept)) * 4.0;
            if (route.front() == threat->intercept) score += 20.0;
            if (score > best.score) {
                best.score = score;
                best.valid = true;
                best.move = *moveOpt;
            }
        }
        if (best.valid) return best.move;
        return std::nullopt;
    }

    void recomputeObjectiveCandidates(index_t targetPlayer) {
        objectiveCandidates.clear();
        std::vector<std::pair<double, Coord>> best;
        const std::size_t objectiveLimit = height * width >= 1600 ? 16 : 24;
        best.reserve(objectiveLimit);
        const Coord targetGuess = chooseTargetPlayerGeneral(targetPlayer);
        const bool bigMap = height * width >= 1600;

        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                const TileView& tile = board.tileAt(c);
                if (!isPassable(memory[idx(c)].type) || tile.occupier == id)
                    continue;
                double score = 0.0;
                if (!memory[idx(c)].everSeen) score += bigMap ? 100.0 : 85.0;
                if (!tile.visible) score += 30.0;
                if (tile.type == TILE_CITY)
                    score += 240.0 - estimatedArmyAt(c) * 2.8;
                if (isEnemyTile(tile)) score += 130.0 + tile.army * 0.3;
                if (tile.occupier == targetPlayer) score += 90.0;
                if (tile.type == TILE_SWAMP) score -= 30.0;
                if (myGeneral != Coord{-1, -1}) {
                    const int dist = distFromGeneral[idx(c)];
                    if (dist < kInf) score += std::max(0.0, 26.0 - dist * 0.7);
                }
                if (targetGuess != Coord{-1, -1}) {
                    score += std::max(0.0, (bigMap ? 65.0 : 45.0) -
                                               manhattan(c, targetGuess) * 1.2);
                }
                const int centerDist = std::abs(c.x * 2 - (height + 1)) +
                                       std::abs(c.y * 2 - (width + 1));
                score += std::max(0.0, 18.0 - centerDist * 0.5);

                if (best.size() < objectiveLimit) {
                    best.emplace_back(score, c);
                    std::push_heap(best.begin(), best.end(),
                                   [](const auto& lhs, const auto& rhs) {
                                       return lhs.first > rhs.first;
                                   });
                } else if (score > best.front().first) {
                    std::pop_heap(best.begin(), best.end(),
                                  [](const auto& lhs, const auto& rhs) {
                                      return lhs.first > rhs.first;
                                  });
                    best.back() = std::make_pair(score, c);
                    std::push_heap(best.begin(), best.end(),
                                   [](const auto& lhs, const auto& rhs) {
                                       return lhs.first > rhs.first;
                                   });
                }
            }
        }

        std::sort(best.begin(), best.end(),
                  [](const auto& lhs, const auto& rhs) {
                      return lhs.first > rhs.first;
                  });
        for (const auto& item : best)
            objectiveCandidates.push_back(item.second);
    }

    void maybeHeavyRefresh() {
        if (!shouldRunHeavyRefresh()) return;
        recomputeDefenseZones();
        currentTargetPlayer = chooseLockedTargetPlayer();
        recomputeObjectiveCandidates(currentTargetPlayer);
        const int aliveCount = static_cast<int>(
            std::count(aliveById.begin(), aliveById.end(), true));
        const int area = static_cast<int>(height * width);
        const int openingTurns = area >= 1600 ? 26 : (area >= 900 ? 22 : 16);
        const bool aggressiveWindow =
            aliveCount <= 3 || fullTurn >= openingTurns ||
            targetPlayerScore(currentTargetPlayer) > 170.0;
        if (lockedObjective != Coord{-1, -1} &&
            fullTurn <= static_cast<turn_t>(objectiveLockUntil) &&
            inside(lockedObjective) &&
            board.tileAt(lockedObjective).occupier != id) {
            currentObjective = lockedObjective;
        } else if (aggressiveWindow && currentTargetPlayer >= 0) {
            Coord generalGuess = chooseTargetPlayerGeneral(currentTargetPlayer);
            if (generalGuess != Coord{-1, -1}) {
                currentObjective = generalGuess;
                lockedObjective = generalGuess;
                objectiveLockUntil = static_cast<int>(fullTurn) +
                                     (height * width >= 1600 ? 16 : 12);
            } else if (!objectiveCandidates.empty()) {
                currentObjective = objectiveCandidates.front();
            }
        } else if (!objectiveCandidates.empty()) {
            currentObjective = objectiveCandidates.front();
        }
        lastHeavyRefreshTurn = static_cast<int>(fullTurn);
        lastAliveCount = static_cast<int>(
            std::count(aliveById.begin(), aliveById.end(), true));
        previousGeneralThreatState = hasGeneralThreatNow();
    }

    std::optional<Move> chooseObjectiveMove(index_t targetPlayer) {
        if (currentObjective == Coord{-1, -1}) return std::nullopt;
        if (!inside(currentObjective) ||
            board.tileAt(currentObjective).occupier == id) {
            for (Coord c : objectiveCandidates) {
                if (inside(c) && board.tileAt(c).occupier != id) {
                    currentObjective = c;
                    break;
                }
            }
            if (board.tileAt(currentObjective).occupier == id)
                return std::nullopt;
        }

        std::vector<std::pair<double, Coord>> sources;
        for (Coord source : friendlyTilesCache) {
            const TileView& tile = board.tileAt(source);
            if (tile.army <= reserveArmy[idx(source)] + 1) continue;
            double score =
                tile.army - manhattan(source, currentObjective) * 1.8;
            if (source == myGeneral) score -= 7.0;
            if (chokeMask[idx(source)]) score -= 10.0;
            if (tile.type == TILE_CITY) score -= 4.0;
            sources.emplace_back(score, source);
        }
        std::sort(sources.begin(), sources.end(),
                  [](const auto& lhs, const auto& rhs) {
                      return lhs.first > rhs.first;
                  });
        if (sources.size() > (height * width >= 1600 ? 2u : 4u))
            sources.resize(height * width >= 1600 ? 2u : 4u);

        CandidateMove best;
        const Coord targetGuess = chooseTargetPlayerGeneral(targetPlayer);
        for (const auto& [base, source] : sources) {
            auto path = weightedPath(source, [&](Coord c) {
                int cost = 2;
                const TileMemory& mem = memory[idx(c)];
                if (mem.type == TILE_SWAMP) cost += 8;
                if (!mem.everSeen) cost += 1;
                if (board.tileAt(c).occupier >= 0 &&
                    !isFriendlyOccupier(board.tileAt(c).occupier))
                    cost += std::min<army_t>(estimatedArmyAt(c), 90);
                if (targetGuess != Coord{-1, -1})
                    cost += manhattan(c, targetGuess) / 8;
                return std::max(1, cost);
            });
            if (path.distance(idx(currentObjective)) >= kInf) continue;
            std::vector<Coord> route =
                reconstructPath(source, currentObjective, path);
            if (route.empty()) continue;
            auto moveOpt = buildReserveAwareMove(source, route.front(), false);
            if (!moveOpt.has_value() || !moveLooksSafe(*moveOpt) ||
                shouldBlockOscillation(source, route.front()))
                continue;
            double score = base - path.distance(idx(currentObjective)) * 2.5;
            if (route.front() == currentObjective) score += 25.0;
            if (board.tileAt(currentObjective).occupier == targetPlayer)
                score += 30.0;
            if (score > best.score) {
                best.score = score;
                best.valid = true;
                best.move = *moveOpt;
            }
        }
        if (best.valid) return best.move;
        return std::nullopt;
    }

    std::optional<Move> fallbackMove(index_t targetPlayer) const {
        CandidateMove direct = chooseImmediateHarvestMove(targetPlayer);
        if (direct.valid && moveLooksSafe(direct.move)) return direct.move;

        Coord bestSource{-1, -1};
        army_t bestArmy = 0;
        for (Coord source : friendlyTilesCache) {
            const TileView& tile = board.tileAt(source);
            if (tile.army <= reserveArmy[idx(source)] + 1) continue;
            army_t value = tile.army - reserveArmy[idx(source)];
            if (value > bestArmy) {
                bestArmy = value;
                bestSource = source;
            }
        }
        if (bestSource == Coord{-1, -1}) return std::nullopt;

        Move bestMove;
        double bestScore = -1e100;
        for (Coord d : kDirs) {
            Coord to = bestSource + d;
            if (!inside(to) || isImpassableTile(board.tileAt(to).type) ||
                shouldBlockOscillation(bestSource, to))
                continue;
            auto moveOpt = buildReserveAwareMove(bestSource, to, false);
            if (!moveOpt.has_value() || !moveLooksSafe(*moveOpt)) continue;
            double score = 0.0;
            const TileView& tile = board.tileAt(to);
            if (!memory[idx(to)].everSeen) score += 50.0;
            if (!tile.visible) score += 18.0;
            if (tile.occupier != id) score += 22.0;
            if (tile.type == TILE_CITY) score += 70.0 - estimatedArmyAt(to);
            if (tile.type == TILE_SWAMP) score -= 14.0;
            if (score > bestScore) {
                bestScore = score;
                bestMove = *moveOpt;
            }
        }
        if (bestScore <= -1e90) return std::nullopt;
        return bestMove;
    }

    std::optional<Move> selectStrategicMove() {
        if (playerCnt < 3) {
            index_t fallbackTarget = chooseFallbackTargetPlayer();
            CandidateMove direct = chooseImmediateHarvestMove(fallbackTarget);
            if (direct.valid && moveLooksSafe(direct.move)) return direct.move;
            if (auto defend = chooseDefenseMove()) return defend;
            maybeHeavyRefresh();
            if (auto move = chooseObjectiveMove(fallbackTarget)) return move;
            return fallbackMove(fallbackTarget);
        }

        maybeHeavyRefresh();
        const index_t targetPlayer = currentTargetPlayer;
        CandidateMove direct = chooseImmediateHarvestMove(targetPlayer);
        if (direct.valid && direct.score > 180.0 && moveLooksSafe(direct.move))
            return direct.move;

        if (auto defend = chooseDefenseMove()) return defend;

        if (auto move = chooseObjectiveMove(targetPlayer)) return move;

        if (direct.valid && moveLooksSafe(direct.move)) return direct.move;
        return fallbackMove(targetPlayer);
    }

   public:
    void init(index_t playerId, const GameConstantsPack& constants) override {
        id = playerId;
        height = constants.mapHeight;
        width = constants.mapWidth;
        W = width + 2;
        playerCnt = constants.playerCount;
        config = constants.config;
        teams = constants.teams;
        halfTurn = 0;
        fullTurn = 0;

        const size_t total = static_cast<size_t>((height + 2) * W);
        memory.assign(total, TileMemory{});
        knownGenerals.assign(playerCnt, Coord{-1, -1});
        predictedGeneralScore.assign(playerCnt,
                                     std::vector<double>(total, 0.0));
        candidateGeneralMask.assign(playerCnt, std::vector<uint8_t>(total, 0));
        newlyVisibleEnemyTiles.assign(playerCnt, 0);
        rankById.assign(playerCnt, RankItem{});
        prevRankById.clear();
        aliveById.assign(playerCnt, true);
        playerArmyDelta.assign(playerCnt, 0);
        playerLandDelta.assign(playerCnt, 0);

        friendlyTilesCache.clear();
        frontierTiles.clear();
        ownedCitiesCache.clear();
        objectiveCandidates.clear();
        distFromGeneral.assign(total, kInf);
        enemyPressure.assign(total, 0.0);
        pathFlow.assign(total, 0);
        reserveArmy.assign(total, 0);
        coreZoneMask.assign(total, 0);
        chokeMask.assign(total, 0);

        largestFriendlyArmyCache = 0;
        myGeneral = Coord{-1, -1};
        currentTargetPlayer = -1;
        currentObjective = Coord{-1, -1};
        lockedObjective = Coord{-1, -1};
        objectiveLockUntil = -1;
        targetLockUntil = -1;
        lastHeavyRefreshTurn = -1;
        lastAliveCount = -1;
        sawNewGeneralThisTurn = false;
        previousGeneralThreatState = false;
        lastMoveFrom = Coord{-1, -1};
        lastMoveTo = Coord{-1, -1};

        recentEdges.clear();
        recentEdgeCounts.clear();
        hasPrevRank = false;
    }

    void requestMove(const BoardView& boardView,
                     const std::vector<RankItem>& rank) override {
        ++halfTurn;
        fullTurn += (halfTurn & 1);
        board = boardView;
        moveQueue.clear();

        refreshRank(rank);
        rememberVisibleBoard();
        rebuildTurnCaches();
        computeEnemyPressure();

        std::optional<Move> candidate = selectStrategicMove();
        if (!candidate.has_value() || !moveLooksSafe(*candidate)) {
            candidate = fallbackMove(currentTargetPlayer);
        }
        if (!candidate.has_value()) return;

        pushRecentMove(candidate->from, candidate->to);
        moveQueue.push_back(*candidate);
    }
};

static BotRegistrar<LyBot> ly_bot_reg("LyBot");

#endif  // LGEN_BOTS_LYBOT
