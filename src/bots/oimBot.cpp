// Copyright (C) 2026 oimasterkafuu
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file oimBot.cpp
 *
 * Built-in oimbot for LocalGen.
 *
 * @author oimasterkafuu
 */

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
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/bot.h"
#include "core/game.hpp"

class OimBot : public BasicBot {
   private:
    enum class Stance { DEFEND, GATHER, LAUNCH, HUNT };

    static constexpr std::array<Coord, 4> kDirs = {Coord{-1, 0}, Coord{0, -1},
                                                   Coord{1, 0}, Coord{0, 1}};
    static constexpr int kInf = 1e9;
    static constexpr int kLookbackMoves = 16;

    struct TileMemory {
        tile_type_e type = TILE_BLANK;
        army_t army = 0;
        index_t occupier = -1;
        bool everSeen = false;
        bool currentlyVisible = false;
        bool discoveredObstacle = false;
        int lastSeenTurn = -1;
    };

    struct PathResult {
        bool reachable = false;
        int cost = kInf;
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

    struct ReversePathResult {
        bool reachable = false;
        const std::vector<int>* dist = nullptr;
        const std::vector<int>* stamp = nullptr;
        const std::vector<Coord>* next = nullptr;
        int activeStamp = 0;

        int distance(size_t node) const {
            if (dist == nullptr || stamp == nullptr || node >= dist->size())
                return kInf;
            return (*stamp)[node] == activeStamp ? (*dist)[node] : kInf;
        }

        Coord nextAt(size_t node) const {
            if (next == nullptr || stamp == nullptr || node >= next->size())
                return Coord{-1, -1};
            return (*stamp)[node] == activeStamp ? (*next)[node]
                                                 : Coord{-1, -1};
        }
    };

    struct CandidateMove {
        Move move{};
        double score = -1e100;
        bool valid = false;
    };

    struct ThreatInfo {
        Coord source{-1, -1};
        Coord target{-1, -1};
        std::vector<Coord> route;
        army_t sourceArmy = 0;
        int turns = kInf;
        double pressure = -1e100;
        bool targetsGeneral = false;
    };

    struct TimingPlan {
        int cycleTurns = 50;
        int gatherSplit = 20;
        int launchTiming = 20;
    };

    struct SearchWorkspace {
        std::vector<int> dist;
        std::vector<int> stamp;
        std::vector<Coord> link;
        int activeStamp = 1;
        std::vector<std::pair<int, Coord>> heap;
    };

    struct RecentEdge {
        Coord from{-1, -1};
        Coord to{-1, -1};
        uint64_t undirectedKey = 0;
    };

    struct RoutePreview {
        Coord first{-1, -1};
        int steps = 0;
        bool reachable = false;
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
    BoardView prevBoard;
    bool hasPrevBoard = false;
    int decisionSerial = 0;
    std::vector<RankItem> sortedRank;
    std::vector<RankItem> prevSortedRank;
    bool hasPrevRank = false;
    std::vector<Coord> friendlyTilesCache;
    std::vector<Coord> ownedCitiesCache;
    army_t largestFriendlyArmyCache = 0;
    std::vector<TileMemory> memory;
    std::vector<Coord> knownGenerals;
    std::vector<std::vector<double>> predictedGeneralScore;
    std::vector<std::vector<uint8_t>> candidateGeneralMask;
    std::vector<uint8_t> attackCorridorMask;
    std::vector<int> newlyVisibleEnemyTiles;
    std::vector<bool> aliveById;
    std::vector<army_t> playerArmyDelta;
    std::vector<pos_t> playerLandDelta;
    std::deque<RecentEdge> recentEdges;
    std::unordered_map<uint64_t, int> recentEdgeCounts;
    Coord lastMoveFrom{-1, -1};
    Coord lastMoveTo{-1, -1};
    Coord myGeneral{-1, -1};
    Coord currentObjective{-1, -1};
    Coord previousAnchor{-1, -1};
    Coord openingLaneTarget{-1, -1};
    Coord openingLaneHeading{-1, -1};
    Coord lockedObjective{-1, -1};
    int openingLaunchTurn = 18;
    int objectiveLockUntil = -1;
    index_t lockedTargetPlayer = -1;
    int targetLockUntil = -1;
    Stance currentStance = Stance::GATHER;
    mutable std::vector<Coord> targetGeneralCache;
    mutable std::vector<int> targetGeneralCacheStamp;
    mutable std::vector<TimingPlan> timingPlanCache;
    mutable std::vector<int> timingPlanCacheStamp;
    mutable std::vector<int> distFromGeneralCache;
    mutable int distFromGeneralCacheStamp = -1;
    mutable Coord distFromGeneralSource{-1, -1};
    mutable SearchWorkspace forwardSearch;
    mutable SearchWorkspace reverseSearch;
    std::mt19937 rng{std::random_device{}()};

    inline size_t idx(pos_t x, pos_t y) const {
        return static_cast<size_t>(x * W + y);
    }
    inline size_t idx(Coord c) const { return idx(c.x, c.y); }

    inline uint64_t undirectedEdgeKey(Coord a, Coord b) const {
        uint64_t lhs = static_cast<uint64_t>(idx(a));
        uint64_t rhs = static_cast<uint64_t>(idx(b));
        if (lhs > rhs) std::swap(lhs, rhs);
        return (lhs << 32) | rhs;
    }

    inline bool inside(Coord c) const {
        return c.x >= 1 && c.x <= height && c.y >= 1 && c.y <= width;
    }

    inline bool isPassable(tile_type_e type) const {
        return !isImpassableTile(type) && type != TILE_OBSTACLE;
    }

    inline bool isTeammate(index_t other) const {
        if (other < 0 || other >= static_cast<index_t>(teams.size()))
            return false;
        return teams[other] == teams[id];
    }

    inline int manhattan(Coord a, Coord b) const {
        return std::abs(a.x - b.x) + std::abs(a.y - b.y);
    }

    inline bool isEnemyTile(const TileView& tile) const {
        return tile.occupier >= 0 && tile.occupier != id &&
               !isTeammate(tile.occupier);
    }

    inline bool isFriendlyTile(const TileView& tile) const {
        return tile.occupier == id ||
               (tile.occupier >= 0 && isTeammate(tile.occupier));
    }

    inline bool isNeutralCity(const TileView& tile) const {
        return tile.type == TILE_CITY && tile.occupier < 0;
    }

    inline Coord primaryObjective() const {
        if (lockedObjective != Coord{-1, -1} &&
            fullTurn <= static_cast<turn_t>(objectiveLockUntil)) {
            return lockedObjective;
        }
        return currentObjective;
    }

    static bool minHeapCompare(const std::pair<int, Coord>& lhs,
                               const std::pair<int, Coord>& rhs) {
        return lhs.first > rhs.first;
    }

    void prepareWorkspace(SearchWorkspace& workspace) const {
        const size_t total = static_cast<size_t>((height + 2) * W);
        if (workspace.dist.size() != total) {
            workspace.dist.assign(total, kInf);
            workspace.stamp.assign(total, 0);
            workspace.link.assign(total, Coord{-1, -1});
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

    void touchWorkspaceNode(SearchWorkspace& workspace, size_t node) const {
        if (workspace.stamp[node] == workspace.activeStamp) return;
        workspace.stamp[node] = workspace.activeStamp;
        workspace.dist[node] = kInf;
        workspace.link[node] = Coord{-1, -1};
    }

    void rebuildTurnCaches() {
        friendlyTilesCache.clear();
        ownedCitiesCache.clear();
        largestFriendlyArmyCache = 0;
        friendlyTilesCache.reserve(static_cast<size_t>(height * width / 3 + 8));
        ownedCitiesCache.reserve(12);

        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                const TileView& tile = board.tileAt(c);
                if (tile.occupier != id) continue;
                friendlyTilesCache.push_back(c);
                largestFriendlyArmyCache =
                    std::max(largestFriendlyArmyCache, tile.army);
                if (tile.type == TILE_CITY) ownedCitiesCache.push_back(c);
            }
        }

        distFromGeneralCacheStamp = -1;
        distFromGeneralSource = Coord{-1, -1};
    }

    std::vector<Coord> allCoords() const {
        std::vector<Coord> coords;
        coords.reserve(static_cast<size_t>(height * width));
        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) coords.emplace_back(x, y);
        }
        return coords;
    }

    void decayPredictions() {
        for (index_t p = 0; p < playerCnt; ++p) {
            if (p == id) continue;
            for (double& score : predictedGeneralScore[p]) score *= 0.985;
        }
    }

    void resetCandidateMask(index_t player) {
        if (player < 0 || player >= playerCnt) return;
        auto& mask = candidateGeneralMask[player];
        std::fill(mask.begin(), mask.end(), 0);
        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                const TileMemory& mem = memory[idx(c)];
                if (!isPassable(mem.type)) continue;
                if (mem.currentlyVisible) continue;
                if (mem.type == TILE_CITY) continue;
                if (myGeneral != Coord{-1, -1} && manhattan(c, myGeneral) < 11)
                    continue;
                mask[idx(c)] = 1;
            }
        }
    }

    void applyVisibilityExclusions(index_t player) {
        if (player < 0 || player >= playerCnt) return;
        auto& mask = candidateGeneralMask[player];
        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                const TileView& tile = board.tileAt(c);
                if (!tile.visible) continue;
                if (tile.type == TILE_GENERAL && tile.occupier == player) {
                    std::fill(mask.begin(), mask.end(), 0);
                    mask[idx(c)] = 1;
                    return;
                }
                mask[idx(c)] = 0;
            }
        }
    }

    void enforceCandidateNonEmpty(index_t player) {
        if (player < 0 || player >= playerCnt) return;
        const auto& mask = candidateGeneralMask[player];
        for (uint8_t v : mask) {
            if (v != 0) return;
        }
        resetCandidateMask(player);
        applyVisibilityExclusions(player);
    }

    void reinforceCandidateNeighborhood(index_t player, Coord anchor,
                                        double baseValue, int maxDist,
                                        bool requireFog = true) {
        if (player < 0 || player >= playerCnt || anchor == Coord{-1, -1})
            return;
        auto& mask = candidateGeneralMask[player];
        auto& heat = predictedGeneralScore[player];
        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                if (manhattan(c, anchor) > maxDist) continue;
                const TileMemory& mem = memory[idx(c)];
                if (!isPassable(mem.type) || mem.type == TILE_CITY) continue;
                if (requireFog && mem.currentlyVisible) continue;
                if (myGeneral != Coord{-1, -1} && manhattan(c, myGeneral) < 11)
                    continue;
                mask[idx(c)] = 1;
                heat[idx(c)] +=
                    std::max(0.0, baseValue - manhattan(c, anchor) * 2.25);
            }
        }
    }

    void pushRecentMove(Coord from, Coord to) {
        uint64_t edgeKey = undirectedEdgeKey(from, to);
        recentEdges.push_back(RecentEdge{from, to, edgeKey});
        ++recentEdgeCounts[edgeKey];
        if (static_cast<int>(recentEdges.size()) > kLookbackMoves) {
            RecentEdge expired = recentEdges.front();
            recentEdges.pop_front();
            auto it = recentEdgeCounts.find(expired.undirectedKey);
            if (it != recentEdgeCounts.end()) {
                if (--it->second <= 0) recentEdgeCounts.erase(it);
            }
        }
        lastMoveFrom = from;
        lastMoveTo = to;
    }

    bool isRecentBounce(Coord from, Coord to) const {
        return recentEdgeCounts.find(undirectedEdgeKey(from, to)) !=
               recentEdgeCounts.end();
    }

    bool isImmediateReverse(Coord from, Coord to) const {
        return from == lastMoveTo && to == lastMoveFrom;
    }

    int oscillationPairCount(Coord from, Coord to, int window = 10) const {
        (void)window;
        auto it = recentEdgeCounts.find(undirectedEdgeKey(from, to));
        return it == recentEdgeCounts.end() ? 0 : it->second;
    }

    bool isTacticalReverseWorthIt(Coord from, Coord to) const {
        if (!inside(from) || !inside(to)) return false;
        const TileView& src = board.tileAt(from);
        const TileView& dst = board.tileAt(to);
        if (src.army <= 1) return false;
        army_t attack = src.army - 1;
        army_t defense = estimatedArmyAt(to);
        if (dst.type == TILE_GENERAL) return attack > defense;
        if (isEnemyTile(dst)) return attack > defense;
        if (dst.type == TILE_CITY) return attack > defense;
        return false;
    }

    bool shouldBlockOscillation(Coord from, Coord to) const {
        if (!inside(from) || !inside(to)) return false;
        if (isTacticalReverseWorthIt(from, to)) return false;

        const TileView& dst = board.tileAt(to);
        Coord objective = primaryObjective();
        bool friendlyBackstep = isFriendlyTile(dst);
        bool driftingAway = false;

        if (objective != Coord{-1, -1}) {
            int fromDist = manhattan(from, objective);
            int toDist = manhattan(to, objective);
            driftingAway = toDist > fromDist;
            if (!attackCorridorMask.empty() && attackCorridorMask[idx(from)] &&
                !attackCorridorMask[idx(to)] && toDist >= fromDist) {
                driftingAway = true;
            }
        }

        int pairCount = oscillationPairCount(from, to, 12);
        if (isImmediateReverse(from, to) && friendlyBackstep) return true;
        if (pairCount >= 2 && friendlyBackstep && driftingAway) return true;
        if (pairCount >= 3 && friendlyBackstep) return true;
        return false;
    }

    void rememberVisibleBoard() {
        newlyVisibleEnemyTiles.assign(playerCnt, 0);
        decayPredictions();

        for (index_t p = 0; p < playerCnt; ++p) {
            if (p == id) continue;
            if (knownGenerals[p] == Coord{-1, -1}) {
                enforceCandidateNonEmpty(p);
                applyVisibilityExclusions(p);
            }
        }

        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                const TileView& tile = board.tileAt(c);
                TileMemory& mem = memory[idx(c)];
                bool wasVisible = mem.currentlyVisible;
                mem.currentlyVisible = tile.visible;
                if (!tile.visible) {
                    if (!mem.everSeen && tile.type == TILE_OBSTACLE) {
                        mem.type = TILE_OBSTACLE;
                        mem.discoveredObstacle = true;
                    } else if (!mem.everSeen) {
                        mem.type = tile.type;
                    }
                    if (mem.occupier >= 0 && mem.army > 0) {
                        mem.army = std::max<army_t>(1, mem.army - 1);
                    }
                    continue;
                }

                bool newlyVisibleEnemy =
                    !wasVisible &&
                    mem.lastSeenTurn != static_cast<int>(fullTurn) &&
                    tile.occupier >= 0 && tile.occupier != id &&
                    !isTeammate(tile.occupier);

                bool enemyDisappearedIntoFog = false;
                if (hasPrevBoard && !tile.visible && inside(c)) {
                    const TileView& prev = prevBoard.tileAt(c);
                    enemyDisappearedIntoFog =
                        prev.visible && prev.occupier >= 0 &&
                        prev.occupier != id && !isTeammate(prev.occupier);
                }

                mem.type = tile.type;
                mem.army = tile.army;
                mem.occupier = tile.occupier;
                mem.everSeen = true;
                mem.lastSeenTurn = static_cast<int>(fullTurn);
                mem.discoveredObstacle =
                    tile.type == TILE_MOUNTAIN || tile.type == TILE_LOOKOUT ||
                    tile.type == TILE_OBSERVATORY || tile.type == TILE_OBSTACLE;

                if (tile.type == TILE_GENERAL && tile.occupier >= 0) {
                    knownGenerals[tile.occupier] = c;
                    std::fill(candidateGeneralMask[tile.occupier].begin(),
                              candidateGeneralMask[tile.occupier].end(), 0);
                    candidateGeneralMask[tile.occupier][idx(c)] = 1;
                    std::fill(predictedGeneralScore[tile.occupier].begin(),
                              predictedGeneralScore[tile.occupier].end(), 0.0);
                    predictedGeneralScore[tile.occupier][idx(c)] = 1e9;
                }
                if (tile.type == TILE_GENERAL && tile.occupier == id)
                    myGeneral = c;

                if (newlyVisibleEnemy) newlyVisibleEnemyTiles[tile.occupier]++;

                if (newlyVisibleEnemy) {
                    reinforceCandidateNeighborhood(tile.occupier, c, 18.0, 8);
                }

                if (enemyDisappearedIntoFog) {
                    index_t prevOwner = prevBoard.tileAt(c).occupier;
                    if (prevOwner >= 0 && prevOwner != id &&
                        !isTeammate(prevOwner) &&
                        knownGenerals[prevOwner] == Coord{-1, -1}) {
                        reinforceCandidateNeighborhood(prevOwner, c, 8.0, 5,
                                                       false);
                    }
                }
            }
        }

        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                const TileView& tile = board.tileAt(c);
                if (!tile.visible || tile.occupier < 0 || tile.occupier == id ||
                    isTeammate(tile.occupier)) {
                    continue;
                }
                updatePredictionFromEnemyTile(c, tile.occupier);
            }
        }

        for (index_t p = 0; p < playerCnt; ++p) {
            if (p == id || knownGenerals[p] != Coord{-1, -1}) continue;
            applyVisibilityExclusions(p);
            enforceCandidateNonEmpty(p);
        }
    }

    void updatePredictionFromEnemyTile(Coord enemyTile, index_t player) {
        if (knownGenerals[player] != Coord{-1, -1}) return;
        auto& scores = predictedGeneralScore[player];
        auto& mask = candidateGeneralMask[player];
        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                if (!isPassable(memory[idx(c)].type)) continue;
                if (memory[idx(c)].currentlyVisible) continue;
                if (memory[idx(c)].type == TILE_CITY) continue;
                if (!mask[idx(c)]) continue;
                int d = manhattan(c, enemyTile);
                int fromUs =
                    myGeneral == Coord{-1, -1} ? 0 : manhattan(c, myGeneral);
                double value =
                    std::max(0.0, 18.0 - static_cast<double>(d) * 1.75);
                value += std::min(10.0, fromUs * 0.3);
                if (enemyTile.x == 1 || enemyTile.x == height ||
                    enemyTile.y == 1 || enemyTile.y == width) {
                    value -= 4.0;
                }
                scores[idx(c)] += value;
            }
        }
    }

    Coord chooseTargetPlayerGeneral(index_t player) const {
        if (player < 0 || player >= playerCnt) return Coord{-1, -1};
        if (knownGenerals[player] != Coord{-1, -1})
            return knownGenerals[player];
        if (player < static_cast<index_t>(targetGeneralCacheStamp.size()) &&
            targetGeneralCacheStamp[player] == decisionSerial) {
            return targetGeneralCache[player];
        }
        const auto& scores = predictedGeneralScore[player];
        const auto& mask = candidateGeneralMask[player];
        double bestScore = -1e100;
        Coord best{-1, -1};
        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                const TileMemory& mem = memory[idx(c)];
                if (!isPassable(mem.type) || mem.currentlyVisible ||
                    mem.type == TILE_CITY)
                    continue;
                if (!mask[idx(c)]) continue;
                double score = scores[idx(c)];
                if (myGeneral != Coord{-1, -1})
                    score -= manhattan(c, myGeneral) * 0.1;
                if (score > bestScore) {
                    bestScore = score;
                    best = c;
                }
            }
        }
        if (player < static_cast<index_t>(targetGeneralCache.size())) {
            targetGeneralCache[player] = best;
            targetGeneralCacheStamp[player] = decisionSerial;
        }
        return best;
    }

    double scoreTargetPlayer(index_t player) const {
        if (player == id || isTeammate(player) || !aliveById[player])
            return -1e100;
        double score = 0.0;
        Coord known = knownGenerals[player];
        if (known != Coord{-1, -1}) score += 250.0;
        score += newlyVisibleEnemyTiles[player] * 10.0;
        Coord guess = chooseTargetPlayerGeneral(player);
        if (guess != Coord{-1, -1} && myGeneral != Coord{-1, -1}) {
            score += std::max(0.0, 80.0 - manhattan(guess, myGeneral) * 2.0);
        }
        if (lockedTargetPlayer == player &&
            fullTurn <= static_cast<turn_t>(targetLockUntil))
            score += 35.0;
        for (const auto& item : sortedRank) {
            if (item.player != player) continue;
            score += item.alive ? 0.0 : -1e6;
            score += std::max<army_t>(0, 220 - item.army) * 0.22;
            score += std::max(0, 120 - item.land) * 0.55;
            if (player < static_cast<index_t>(playerArmyDelta.size())) {
                score += std::max<army_t>(0, -playerArmyDelta[player]) * 1.2;
                score += std::max<pos_t>(0, -playerLandDelta[player]) * 6;
                score -= std::max<army_t>(0, playerArmyDelta[player]) * 0.35;
                score -= std::max<pos_t>(0, playerLandDelta[player]) * 2;
            }
            break;
        }
        return score;
    }

    index_t chooseTargetPlayer() const {
        index_t best = -1;
        double bestScore = -1e100;
        for (index_t player = 0; player < playerCnt; ++player) {
            double score = scoreTargetPlayer(player);
            if (score > bestScore) {
                bestScore = score;
                best = player;
            }
        }
        return best;
    }

    index_t chooseLockedTargetPlayer(index_t suggested) {
        if (suggested < 0) return suggested;
        if (lockedTargetPlayer < 0 || !aliveById[lockedTargetPlayer] ||
            fullTurn > static_cast<turn_t>(targetLockUntil)) {
            lockedTargetPlayer = suggested;
            targetLockUntil = static_cast<int>(fullTurn) + 16;
            return lockedTargetPlayer;
        }
        double lockedScore = scoreTargetPlayer(lockedTargetPlayer);
        double newScore = scoreTargetPlayer(suggested);
        if (suggested != lockedTargetPlayer && newScore > lockedScore + 80.0) {
            lockedTargetPlayer = suggested;
            targetLockUntil = static_cast<int>(fullTurn) + 16;
        }
        return lockedTargetPlayer;
    }

    PathResult weightedPath(Coord start,
                            const std::function<int(Coord)>& stepCost) const {
        PathResult result;
        if (!inside(start)) return result;

        prepareWorkspace(forwardSearch);
        size_t startIndex = idx(start);
        touchWorkspaceNode(forwardSearch, startIndex);
        forwardSearch.dist[startIndex] = 0;
        forwardSearch.heap.emplace_back(0, start);
        std::push_heap(forwardSearch.heap.begin(), forwardSearch.heap.end(),
                       minHeapCompare);

        while (!forwardSearch.heap.empty()) {
            std::pop_heap(forwardSearch.heap.begin(), forwardSearch.heap.end(),
                          minHeapCompare);
            auto [curDist, cur] = forwardSearch.heap.back();
            forwardSearch.heap.pop_back();
            if (curDist != forwardSearch.dist[idx(cur)]) continue;
            for (Coord d : kDirs) {
                Coord nxt = cur + d;
                if (!inside(nxt)) continue;
                const TileMemory& mem = memory[idx(nxt)];
                if (!isPassable(mem.type)) continue;
                size_t nextIndex = idx(nxt);
                touchWorkspaceNode(forwardSearch, nextIndex);
                int nd = curDist + stepCost(nxt);
                if (nd < forwardSearch.dist[nextIndex]) {
                    forwardSearch.dist[nextIndex] = nd;
                    forwardSearch.link[nextIndex] = cur;
                    forwardSearch.heap.emplace_back(nd, nxt);
                    std::push_heap(forwardSearch.heap.begin(),
                                   forwardSearch.heap.end(), minHeapCompare);
                }
            }
        }
        result.reachable = true;
        result.dist = &forwardSearch.dist;
        result.stamp = &forwardSearch.stamp;
        result.parent = &forwardSearch.link;
        result.activeStamp = forwardSearch.activeStamp;
        return result;
    }

    ReversePathResult weightedReversePath(
        Coord goal, const std::function<int(Coord)>& stepCost) const {
        ReversePathResult result;
        if (!inside(goal)) return result;

        prepareWorkspace(reverseSearch);
        size_t goalIndex = idx(goal);
        touchWorkspaceNode(reverseSearch, goalIndex);
        reverseSearch.dist[goalIndex] = 0;
        reverseSearch.heap.emplace_back(0, goal);
        std::push_heap(reverseSearch.heap.begin(), reverseSearch.heap.end(),
                       minHeapCompare);

        while (!reverseSearch.heap.empty()) {
            std::pop_heap(reverseSearch.heap.begin(), reverseSearch.heap.end(),
                          minHeapCompare);
            auto [curDist, cur] = reverseSearch.heap.back();
            reverseSearch.heap.pop_back();
            if (curDist != reverseSearch.dist[idx(cur)]) continue;
            for (Coord d : kDirs) {
                Coord prev = cur + d;
                if (!inside(prev)) continue;
                const TileMemory& mem = memory[idx(prev)];
                if (!isPassable(mem.type)) continue;
                size_t prevIndex = idx(prev);
                touchWorkspaceNode(reverseSearch, prevIndex);
                int nd = curDist + stepCost(cur);
                if (nd < reverseSearch.dist[prevIndex]) {
                    reverseSearch.dist[prevIndex] = nd;
                    reverseSearch.link[prevIndex] = cur;
                    reverseSearch.heap.emplace_back(nd, prev);
                    std::push_heap(reverseSearch.heap.begin(),
                                   reverseSearch.heap.end(), minHeapCompare);
                }
            }
        }

        result.reachable = true;
        result.dist = &reverseSearch.dist;
        result.stamp = &reverseSearch.stamp;
        result.next = &reverseSearch.link;
        result.activeStamp = reverseSearch.activeStamp;
        return result;
    }

    std::vector<Coord> reconstructPath(Coord start, Coord goal,
                                       const PathResult& path) const {
        std::vector<Coord> rev;
        if (goal == Coord{-1, -1}) return rev;
        Coord cur = goal;
        while (cur != Coord{-1, -1} && cur != start) {
            rev.push_back(cur);
            cur = path.parentAt(idx(cur));
        }
        if (cur != start) return {};
        std::reverse(rev.begin(), rev.end());
        return rev;
    }

    RoutePreview previewRoute(Coord start, Coord goal,
                              const ReversePathResult& path) const {
        RoutePreview preview;
        if (!inside(start) || !inside(goal)) return preview;
        if (start == goal) {
            preview.reachable = true;
            return preview;
        }

        Coord cur = start;
        const int maxSteps = static_cast<int>(height * width) + 5;
        for (int step = 0; step < maxSteps && cur != goal; ++step) {
            Coord next = path.nextAt(idx(cur));
            if (next == Coord{-1, -1}) return RoutePreview{};
            if (preview.first == Coord{-1, -1}) preview.first = next;
            cur = next;
            ++preview.steps;
        }
        if (cur != goal) return RoutePreview{};
        preview.reachable = true;
        return preview;
    }

    army_t estimatedArmyAt(Coord c) const {
        const TileView& tile = board.tileAt(c);
        if (tile.visible) return tile.army;
        return memory[idx(c)].army;
    }

    int attackPenalty(Coord c, bool allowEnemyGeneralDive = true) const {
        const TileMemory& mem = memory[idx(c)];
        int penalty = 2;
        if (mem.type == TILE_SWAMP) penalty += 18;
        if (mem.type == TILE_CITY) penalty += 10;
        if (!mem.everSeen) penalty += 3;
        if (mem.occupier >= 0 && mem.occupier != id &&
            !isTeammate(mem.occupier)) {
            penalty +=
                static_cast<int>(std::min<army_t>(estimatedArmyAt(c), 120));
            if (mem.type == TILE_GENERAL && allowEnemyGeneralDive)
                penalty -= 25;
        } else if (mem.occupier == id) {
            penalty -= 3;
        }
        return std::max(1, penalty);
    }

    const std::vector<Coord>& ourTiles() const { return friendlyTilesCache; }

    const std::vector<Coord>& ownedCities() const { return ownedCitiesCache; }

    Coord strongestFriendlyTile(bool includeGeneral = true,
                                std::optional<Coord> near = std::nullopt,
                                int nearWeight = 0) const {
        Coord best = myGeneral;
        army_t bestArmy = -1;
        for (Coord c : ourTiles()) {
            const TileView& tile = board.tileAt(c);
            if (tile.army <= 1) continue;
            if (!includeGeneral && c == myGeneral) continue;
            army_t value = tile.army;
            if (tile.type == TILE_CITY)
                value -= std::max<army_t>(1, tile.army / 6);
            if (c == myGeneral) value -= std::max<army_t>(3, tile.army / 8);
            if (near.has_value()) value -= manhattan(c, *near) * nearWeight;
            if (value > bestArmy) {
                bestArmy = value;
                best = c;
            }
        }
        return best;
    }

    Coord strongestTileNearObjective(Coord objective) const {
        Coord best{-1, -1};
        double bestScore = -1e100;
        for (Coord c : ourTiles()) {
            const TileView& tile = board.tileAt(c);
            if (tile.army <= 1) continue;
            double score = static_cast<double>(tile.army);
            score -= manhattan(c, objective) * 4.5;
            if (c == myGeneral) score -= 8.0;
            if (tile.type == TILE_CITY) score -= 3.0;
            if (score > bestScore) {
                bestScore = score;
                best = c;
            }
        }
        if (best == Coord{-1, -1})
            best = strongestFriendlyTile(true, objective, 2);
        return best;
    }

    army_t cityCommitRequirement(Coord city, int routeLen = 0) const {
        army_t required = std::max<army_t>(2, estimatedArmyAt(city) + 2);
        required += static_cast<army_t>(std::max(0, routeLen - 1) / 2);
        return required;
    }

    bool canReasonablyCommitToCity(Coord city, Coord source,
                                   const ReversePathResult& reversePath) const {
        if (city == Coord{-1, -1} || source == Coord{-1, -1}) return false;
        if (!isNeutralCity(board.tileAt(city))) return true;
        const TileView& src = board.tileAt(source);
        if (src.occupier != id || src.army <= 1) return false;
        if (reversePath.distance(idx(source)) >= kInf) return false;
        RoutePreview route = previewRoute(source, city, reversePath);
        if (!route.reachable || route.first == Coord{-1, -1}) return false;
        return src.army - 1 >= cityCommitRequirement(city, route.steps);
    }

    bool canReasonablyCommitToCity(Coord city, Coord source) const {
        if (city == Coord{-1, -1} || source == Coord{-1, -1}) return false;
        if (!isNeutralCity(board.tileAt(city))) return true;
        auto reversePath = weightedReversePath(
            city, [&](Coord c) { return attackPenalty(c, true); });
        return canReasonablyCommitToCity(city, source, reversePath);
    }

    bool anyReasonableCityCommit(Coord city, army_t minSourceArmy = 2) const {
        if (!isNeutralCity(board.tileAt(city))) return true;
        auto reversePath = weightedReversePath(
            city, [&](Coord c) { return attackPenalty(c, true); });
        for (Coord source : ourTiles()) {
            const TileView& src = board.tileAt(source);
            if (src.army < minSourceArmy) continue;
            if (canReasonablyCommitToCity(city, source, reversePath))
                return true;
        }
        return false;
    }

    bool shouldConsiderNeutralCityTarget(Coord city, index_t targetPlayer,
                                         bool openingPhase) const {
        if (city == Coord{-1, -1}) return false;
        const TileView& tile = board.tileAt(city);
        if (!isNeutralCity(tile)) return true;

        army_t cityArmy = estimatedArmyAt(city);
        army_t maxArmy = largestFriendlyArmy();
        if (!anyReasonableCityCommit(city, openingPhase ? 2 : 3)) return false;

        if (openingPhase) {
            if (cityArmy >= 30) return false;
            return maxArmy >= cityArmy + 2;
        }

        TimingPlan plan = computeTimingPlan(targetPlayer);
        int offset = currentCycleOffset(plan.cycleTurns);
        bool inLaunchWindow = offset >= plan.launchTiming;
        bool targetWeakening =
            targetPlayer >= 0 &&
            targetPlayer < static_cast<index_t>(playerArmyDelta.size()) &&
            (playerArmyDelta[targetPlayer] < 0 ||
             playerLandDelta[targetPlayer] < 0);

        if (!inLaunchWindow && !targetWeakening) {
            if (cityArmy > 20) return false;
            if (cityArmy > maxArmy / 2 + 4) return false;
        }

        if (cityArmy > maxArmy + 2) return false;
        return true;
    }

    bool openingHasOnlyGeneralArmy() const {
        if (myGeneral == Coord{-1, -1}) return false;
        for (Coord tile : ourTiles()) {
            if (tile != myGeneral && board.tileAt(tile).army > 1) return false;
        }
        return true;
    }

    int countAdjacentOpeningSwamps() const {
        if (myGeneral == Coord{-1, -1}) return 0;
        int count = 0;
        for (Coord d : kDirs) {
            Coord nxt = myGeneral + d;
            if (!inside(nxt)) continue;
            if (board.tileAt(nxt).type == TILE_SWAMP) count++;
        }
        return count;
    }

    int computeOpeningLaunchTurn() const {
        int launch = 18;
        int mapArea = static_cast<int>(height * width);
        if (mapArea <= 196) launch = 16;
        if (mapArea >= 500) launch = 20;
        launch += std::min(2, countAdjacentOpeningSwamps());
        return std::max(12, std::min(24, launch));
    }

    bool shouldDelayOpeningExploration() const {
        if (myGeneral == Coord{-1, -1}) return false;
        if (fullTurn >= openingLaunchTurn) return false;
        if (!openingHasOnlyGeneralArmy()) return false;
        return true;
    }

    bool shouldAvoidOpeningSwamp(Coord tile) const {
        if (tile == Coord{-1, -1}) return false;
        if (fullTurn >= 50) return false;
        const TileView& dst = board.tileAt(tile);
        if (dst.type != TILE_SWAMP) return false;
        return dst.occupier < 0;
    }

    Coord chooseExplicitNeutralCityTarget(index_t targetPlayer,
                                          bool openingPhase) const {
        Coord best{-1, -1};
        double bestScore = -1e100;
        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                const TileView& tile = board.tileAt(c);
                if (!isNeutralCity(tile)) continue;
                if (!shouldCommitExplicitCityNow(c, targetPlayer, openingPhase))
                    continue;
                double score = 250.0 - estimatedArmyAt(c) * 3.0;
                if (myGeneral != Coord{-1, -1})
                    score -= manhattan(c, myGeneral) * 2.0;
                if (shouldAvoidOpeningSwamp(c)) score -= 1000.0;
                if (!tile.visible) score += 10.0;
                if (score > bestScore) {
                    bestScore = score;
                    best = c;
                }
            }
        }
        return best;
    }

    bool shouldCommitExplicitCityNow(Coord city, index_t targetPlayer,
                                     bool openingPhase) const {
        if (city == Coord{-1, -1}) return false;
        const TileView& tile = board.tileAt(city);
        if (!isNeutralCity(tile)) return false;
        if (!shouldConsiderNeutralCityTarget(city, targetPlayer, openingPhase))
            return false;

        army_t cityArmy = estimatedArmyAt(city);
        army_t maxArmy = largestFriendlyArmy();
        if (openingPhase) {
            if (fullTurn < openingLaunchTurn) return false;
            if (cityArmy >= 30) return false;
            return maxArmy >= cityArmy + 4;
        }

        TimingPlan plan = computeTimingPlan(targetPlayer);
        int offset = currentCycleOffset(plan.cycleTurns);
        bool inLaunchWindow = offset >= plan.launchTiming;
        bool targetWeakening =
            targetPlayer >= 0 &&
            targetPlayer < static_cast<index_t>(playerArmyDelta.size()) &&
            (playerArmyDelta[targetPlayer] < 0 ||
             playerLandDelta[targetPlayer] < 0);
        if (!inLaunchWindow && !targetWeakening && cityArmy > 12) return false;
        return maxArmy >= cityArmy + 3;
    }

    std::optional<Move> getExplicitCityAttackMove(index_t targetPlayer,
                                                  bool openingPhase) {
        Coord city =
            chooseExplicitNeutralCityTarget(targetPlayer, openingPhase);
        if (city == Coord{-1, -1}) return std::nullopt;

        Coord source = strongestTileNearObjective(city);
        if (source == Coord{-1, -1}) return std::nullopt;
        if (!canReasonablyCommitToCity(city, source)) return std::nullopt;

        auto path = weightedPath(source, [&](Coord c) {
            int cost = attackPenalty(c, true);
            if (openingPhase && shouldAvoidOpeningSwamp(c)) cost += 1000;
            return std::max(1, cost);
        });
        if (path.distance(idx(city)) >= kInf) return std::nullopt;
        auto route = reconstructPath(source, city, path);
        if (route.empty()) return std::nullopt;
        currentObjective = city;
        return Move(MoveType::MOVE_ARMY, source, route.front(), false);
    }

    const std::vector<int>& bfsDistance(Coord start) const {
        if (distFromGeneralCache.size() !=
            static_cast<size_t>((height + 2) * W)) {
            distFromGeneralCache.assign(static_cast<size_t>((height + 2) * W),
                                        kInf);
        }
        if (start == Coord{-1, -1}) {
            std::fill(distFromGeneralCache.begin(), distFromGeneralCache.end(),
                      kInf);
            distFromGeneralCacheStamp = decisionSerial;
            distFromGeneralSource = start;
            return distFromGeneralCache;
        }
        if (distFromGeneralCacheStamp == decisionSerial &&
            distFromGeneralSource == start) {
            return distFromGeneralCache;
        }

        std::fill(distFromGeneralCache.begin(), distFromGeneralCache.end(),
                  kInf);
        std::queue<Coord> q;
        q.push(start);
        distFromGeneralCache[idx(start)] = 0;
        while (!q.empty()) {
            Coord cur = q.front();
            q.pop();
            for (Coord d : kDirs) {
                Coord nxt = cur + d;
                if (!inside(nxt)) continue;
                if (!isPassable(memory[idx(nxt)].type)) continue;
                if (distFromGeneralCache[idx(nxt)] != kInf) continue;
                distFromGeneralCache[idx(nxt)] =
                    distFromGeneralCache[idx(cur)] + 1;
                q.push(nxt);
            }
        }
        distFromGeneralCacheStamp = decisionSerial;
        distFromGeneralSource = start;
        return distFromGeneralCache;
    }

    army_t largestFriendlyArmy() const { return largestFriendlyArmyCache; }

    int currentCycleOffset(int cycleTurns = 50) const {
        if (cycleTurns <= 0) return 0;
        return static_cast<int>(fullTurn % cycleTurns);
    }

    TimingPlan computeTimingPlan(index_t targetPlayer) const {
        if (targetPlayer >= 0 &&
            targetPlayer < static_cast<index_t>(timingPlanCacheStamp.size()) &&
            timingPlanCacheStamp[targetPlayer] == decisionSerial) {
            return timingPlanCache[targetPlayer];
        }

        TimingPlan plan;
        Coord target = chooseTargetPlayerGeneral(targetPlayer);
        int pathLength = 8;
        int fogOnPath = 0;
        int enemyOnPath = 0;

        if (myGeneral != Coord{-1, -1} && target != Coord{-1, -1}) {
            auto path = weightedPath(
                myGeneral, [&](Coord c) { return attackPenalty(c, true); });
            if (path.distance(idx(target)) < kInf) {
                auto route = reconstructPath(myGeneral, target, path);
                pathLength = std::max<int>(8, static_cast<int>(route.size()));
                for (Coord c : route) {
                    const TileMemory& mem = memory[idx(c)];
                    const TileView& tile = board.tileAt(c);
                    if (!mem.everSeen || !tile.visible) fogOnPath++;
                    if (isEnemyTile(tile)) enemyOnPath++;
                }
            }
        }

        int gatherSplit = 20;
        if (fullTurn < 100)
            gatherSplit = 20;
        else if (fullTurn < 160)
            gatherSplit = 22;
        else
            gatherSplit = 24;

        if (targetPlayer >= 0 &&
            targetPlayer < static_cast<index_t>(playerLandDelta.size())) {
            if (playerLandDelta[targetPlayer] < 0) gatherSplit -= 2;
            if (playerArmyDelta[targetPlayer] < 0) gatherSplit -= 1;
            if (playerLandDelta[targetPlayer] > 0 &&
                playerArmyDelta[targetPlayer] > 0)
                gatherSplit += 2;
        }

        int launchTiming = 50 - pathLength - 4 + enemyOnPath / 2;
        launchTiming -=
            std::max(0, 2 * fogOnPath - pathLength / 2 - enemyOnPath);
        gatherSplit = std::max(12, std::min(gatherSplit, launchTiming));
        launchTiming = std::max(gatherSplit, launchTiming);

        plan.gatherSplit = std::min(34, gatherSplit);
        plan.launchTiming = std::min(44, launchTiming);
        if (targetPlayer >= 0 &&
            targetPlayer < static_cast<index_t>(timingPlanCache.size())) {
            timingPlanCache[targetPlayer] = plan;
            timingPlanCacheStamp[targetPlayer] = decisionSerial;
        }
        return plan;
    }

    bool shouldLaunchNow(index_t targetPlayer) const {
        TimingPlan plan = computeTimingPlan(targetPlayer);
        int offset = currentCycleOffset(plan.cycleTurns);
        if (offset >= plan.launchTiming) return true;
        if (targetPlayer >= 0 &&
            targetPlayer < static_cast<index_t>(playerArmyDelta.size())) {
            if (playerArmyDelta[targetPlayer] < -4 ||
                playerLandDelta[targetPlayer] < -2)
                return true;
        }
        return currentStance == Stance::HUNT;
    }

    Coord chooseOpeningLaneTarget(index_t targetPlayer) const {
        Coord best{-1, -1};
        double bestScore = -1e100;
        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                const TileView& tile = board.tileAt(c);
                if (isImpassableTile(tile.type) || isFriendlyTile(tile))
                    continue;
                double score = 0.0;
                if (!memory[idx(c)].everSeen) score += 90.0;
                if (!tile.visible) score += 30.0;
                if (tile.type == TILE_CITY) {
                    if (!shouldConsiderNeutralCityTarget(c, targetPlayer, true))
                        continue;
                    score += 160.0 - estimatedArmyAt(c);
                }
                if (shouldAvoidOpeningSwamp(c)) continue;
                if (tile.type == TILE_SWAMP) score -= 25.0;
                int centerDist = std::abs(c.x * 2 - (height + 1)) +
                                 std::abs(c.y * 2 - (width + 1));
                score += std::max(0.0, 16.0 - centerDist * 0.5);
                if (myGeneral != Coord{-1, -1})
                    score -= manhattan(c, myGeneral) * 1.2;
                if (score > bestScore) {
                    bestScore = score;
                    best = c;
                }
            }
        }
        return best;
    }

    Coord extendOpeningLaneTarget(Coord base) const {
        if (base == Coord{-1, -1}) return Coord{-1, -1};
        Coord heading = openingLaneHeading;
        if (heading == Coord{-1, -1} && myGeneral != Coord{-1, -1}) {
            heading = Coord{static_cast<pos_t>(base.x - myGeneral.x),
                            static_cast<pos_t>(base.y - myGeneral.y)};
        }
        Coord best{-1, -1};
        double bestScore = -1e100;
        for (Coord d : kDirs) {
            Coord c = base + d;
            if (!inside(c)) continue;
            const TileView& tile = board.tileAt(c);
            if (isImpassableTile(tile.type) || isFriendlyTile(tile)) continue;
            if (shouldAvoidOpeningSwamp(c)) continue;
            if (isNeutralCity(tile)) continue;

            double score = 0.0;
            if (!memory[idx(c)].everSeen) score += 90.0;
            if (!tile.visible) score += 25.0;
            if (tile.type == TILE_BLANK) score += 20.0;
            if (heading != Coord{-1, -1})
                score += d.x * heading.x + d.y * heading.y;
            if (myGeneral != Coord{-1, -1})
                score -= manhattan(c, myGeneral) * 0.8;
            if (score > bestScore) {
                bestScore = score;
                best = c;
            }
        }
        return best;
    }

    Coord currentOpeningLaneTarget(index_t targetPlayer) {
        if (openingLaneTarget == Coord{-1, -1}) {
            openingLaneTarget = chooseOpeningLaneTarget(targetPlayer);
            if (openingLaneTarget != Coord{-1, -1} &&
                myGeneral != Coord{-1, -1}) {
                openingLaneHeading = Coord{
                    static_cast<pos_t>(openingLaneTarget.x - myGeneral.x),
                    static_cast<pos_t>(openingLaneTarget.y - myGeneral.y)};
            }
            return openingLaneTarget;
        }

        if (inside(openingLaneTarget) &&
            board.tileAt(openingLaneTarget).occupier == id) {
            Coord extended = extendOpeningLaneTarget(openingLaneTarget);
            if (extended != Coord{-1, -1}) {
                openingLaneHeading =
                    Coord{static_cast<pos_t>(extended.x - openingLaneTarget.x),
                          static_cast<pos_t>(extended.y - openingLaneTarget.y)};
                openingLaneTarget = extended;
            }
        }
        return openingLaneTarget;
    }

    std::optional<Move> getOpeningPlannerMove(index_t targetPlayer) {
        if (myGeneral == Coord{-1, -1}) return std::nullopt;
        if (shouldDelayOpeningExploration()) return std::nullopt;
        Coord primary = currentOpeningLaneTarget(targetPlayer);
        Coord enemyGuess = chooseTargetPlayerGeneral(targetPlayer);
        std::vector<Coord> targets;
        if (primary != Coord{-1, -1}) targets.push_back(primary);
        if (enemyGuess != Coord{-1, -1} && fullTurn > 12)
            targets.push_back(enemyGuess);

        CandidateMove direct = chooseDirectCaptureMove();
        if (direct.valid && direct.score > 190.0) return direct.move;
        if (auto city = getExplicitCityAttackMove(targetPlayer, true))
            return city;
        if (auto gather = getGatherMoveToTargets(targets, false, 2))
            return gather;
        return std::nullopt;
    }

    void updateAttackCorridor(index_t targetPlayer) {
        attackCorridorMask.assign(static_cast<size_t>((height + 2) * W), 0);
        if (myGeneral == Coord{-1, -1}) return;
        Coord target = chooseTargetPlayerGeneral(targetPlayer);
        if (target == Coord{-1, -1}) return;
        auto path = weightedPath(myGeneral, [&](Coord c) {
            int cost = attackPenalty(c, true);
            if (!memory[idx(c)].everSeen) cost -= 1;
            return std::max(1, cost);
        });
        if (path.distance(idx(target)) >= kInf) return;
        auto route = reconstructPath(myGeneral, target, path);
        for (Coord c : route) {
            attackCorridorMask[idx(c)] = 1;
            for (Coord d : kDirs) {
                Coord adj = c + d;
                if (inside(adj)) attackCorridorMask[idx(adj)] = 1;
            }
        }
    }

    Stance chooseStance(index_t targetPlayer,
                        const std::optional<ThreatInfo>& threat) const {
        if (threat.has_value() && shouldDefend(*threat)) return Stance::DEFEND;
        Coord target = chooseTargetPlayerGeneral(targetPlayer);
        army_t bestArmy = largestFriendlyArmy();
        bool knowsGeneral =
            targetPlayer >= 0 && knownGenerals[targetPlayer] != Coord{-1, -1};
        bool targetWeakening =
            targetPlayer >= 0 &&
            targetPlayer < static_cast<index_t>(playerArmyDelta.size()) &&
            (playerArmyDelta[targetPlayer] < 0 ||
             playerLandDelta[targetPlayer] < 0);
        if (knowsGeneral && bestArmy >= estimatedArmyAt(target) + 8)
            return Stance::HUNT;
        if (targetWeakening && bestArmy >= 20) return Stance::LAUNCH;
        if (fullTurn < 40 || bestArmy < 18) return Stance::GATHER;
        if (target != Coord{-1, -1} && bestArmy >= 22) return Stance::LAUNCH;
        return Stance::GATHER;
    }

    CandidateMove chooseDirectCaptureMove() const {
        CandidateMove best;
        for (Coord from : ourTiles()) {
            const TileView& src = board.tileAt(from);
            if (src.army <= 1) continue;
            for (Coord d : kDirs) {
                Coord to = from + d;
                if (!inside(to)) continue;
                const TileView& dst = board.tileAt(to);
                if (isImpassableTile(dst.type)) continue;
                if (isFriendlyTile(dst)) continue;
                army_t attack = src.army - 1;
                army_t defense = estimatedArmyAt(to);
                if (attack <= defense) continue;
                if (isNeutralCity(dst) && attack < cityCommitRequirement(to, 1))
                    continue;
                if (shouldAvoidOpeningSwamp(to) && dst.type != TILE_CITY)
                    continue;
                if (shouldBlockOscillation(from, to)) continue;
                double score = 0.0;
                if (dst.type == TILE_GENERAL) score += 1e6;
                if (dst.type == TILE_CITY) score += 450.0 - defense;
                if (isEnemyTile(dst)) score += 170.0 + defense * 0.25;
                if (!dst.visible) score += 65.0;
                if (!memory[idx(to)].everSeen) score += 55.0;
                if (dst.type == TILE_BLANK) score += 24.0;
                if (dst.type == TILE_SWAMP) score -= 18.0;
                if (to == currentObjective) score += 70.0;
                if (myGeneral != Coord{-1, -1})
                    score -= manhattan(to, myGeneral) * 0.35;
                if (isRecentBounce(from, to)) score -= 75.0;
                if (score > best.score) {
                    best.score = score;
                    best.move = Move(MoveType::MOVE_ARMY, from, to, false);
                    best.valid = true;
                }
            }
        }
        return best;
    }

    std::optional<ThreatInfo> analyzeFastestThreat() const {
        if (myGeneral == Coord{-1, -1}) return std::nullopt;

        std::vector<Coord> keyTargets;
        keyTargets.push_back(myGeneral);
        for (Coord city : ownedCities()) {
            if (manhattan(city, myGeneral) <= 10) keyTargets.push_back(city);
        }

        std::optional<ThreatInfo> best;
        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord enemy{x, y};
                const TileView& tile = board.tileAt(enemy);
                if (!tile.visible || tile.occupier < 0 || tile.occupier == id ||
                    isTeammate(tile.occupier)) {
                    continue;
                }
                if (tile.army <= 1) continue;

                auto pathFromEnemy = weightedPath(enemy, [&](Coord c) {
                    int cost = 1;
                    const TileMemory& mem = memory[idx(c)];
                    if (mem.type == TILE_SWAMP) cost += 5;
                    if (board.tileAt(c).occupier == id) cost += 1;
                    return cost;
                });

                for (Coord target : keyTargets) {
                    if (pathFromEnemy.distance(idx(target)) >= kInf) continue;
                    auto route = reconstructPath(enemy, target, pathFromEnemy);
                    if (route.empty()) continue;

                    double targetValue = target == myGeneral ? 140.0 : 55.0;
                    double pressure =
                        tile.army * 3.2 -
                        pathFromEnemy.distance(idx(target)) * 11.0 +
                        targetValue;
                    if (target == myGeneral &&
                        board.tileAt(myGeneral).army < tile.army + 4)
                        pressure += 35.0;
                    if (knownGenerals[tile.occupier] != Coord{-1, -1})
                        pressure += 12.0;

                    ThreatInfo info;
                    info.source = enemy;
                    info.target = target;
                    info.route = std::move(route);
                    info.sourceArmy = tile.army;
                    info.turns = static_cast<int>(info.route.size());
                    info.pressure = pressure;
                    info.targetsGeneral = target == myGeneral;

                    if (!best.has_value() || info.pressure > best->pressure)
                        best = info;
                }
            }
        }

        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord enemy{x, y};
                const TileMemory& mem = memory[idx(enemy)];
                if (mem.currentlyVisible || mem.occupier < 0 ||
                    mem.occupier == id || isTeammate(mem.occupier) ||
                    mem.lastSeenTurn < 0)
                    continue;
                if (static_cast<int>(fullTurn) - mem.lastSeenTurn > 6) continue;
                if (mem.army <= 1) continue;

                auto pathFromEnemy = weightedPath(enemy, [&](Coord c) {
                    int cost = 1;
                    const TileMemory& nxt = memory[idx(c)];
                    if (nxt.type == TILE_SWAMP) cost += 5;
                    if (board.tileAt(c).occupier == id) cost += 1;
                    return cost;
                });

                for (Coord target : keyTargets) {
                    if (pathFromEnemy.distance(idx(target)) >= kInf) continue;
                    auto route = reconstructPath(enemy, target, pathFromEnemy);
                    if (route.empty()) continue;
                    double pressure =
                        mem.army * 2.4 -
                        pathFromEnemy.distance(idx(target)) * 10.0 +
                        (target == myGeneral ? 110.0 : 35.0);
                    ThreatInfo info;
                    info.source = enemy;
                    info.target = target;
                    info.route = std::move(route);
                    info.sourceArmy = mem.army;
                    info.turns = static_cast<int>(info.route.size());
                    info.pressure = pressure;
                    info.targetsGeneral = target == myGeneral;
                    if (!best.has_value() || info.pressure > best->pressure)
                        best = info;
                }
            }
        }
        return best;
    }

    bool shouldDefend(const ThreatInfo& threat) const {
        if (threat.target == Coord{-1, -1}) return false;
        const TileView& targetTile = board.tileAt(threat.target);
        if (threat.targetsGeneral) {
            if (threat.turns <= 3) return true;
            if (threat.turns <= 5 && threat.sourceArmy >= targetTile.army - 2)
                return true;
            if (threat.turns <= 7 && threat.sourceArmy >= targetTile.army + 6)
                return true;
            return threat.pressure > 20.0;
        }
        return threat.turns <= 4 && threat.sourceArmy >= targetTile.army;
    }

    std::optional<Move> getGatherMoveToTargets(
        const std::vector<Coord>& targets, bool defenseMode,
        army_t minSourceArmy = 2) const {
        if (targets.empty()) return std::nullopt;

        CandidateMove best;
        std::set<size_t> uniqueTargets;
        for (Coord target : targets) {
            if (!inside(target)) continue;
            if (!uniqueTargets.insert(idx(target)).second) continue;
            const TileView& targetTile = board.tileAt(target);
            auto reversePath = weightedReversePath(target, [&](Coord c) {
                int cost = defenseMode ? 2 : 3;
                const TileView& dst = board.tileAt(c);
                if (dst.occupier != id)
                    cost +=
                        attackPenalty(c, !defenseMode) + (defenseMode ? 2 : 6);
                if (defenseMode && myGeneral != Coord{-1, -1})
                    cost += std::max(0, manhattan(c, myGeneral) - 3);
                if (!defenseMode && currentObjective != Coord{-1, -1})
                    cost += manhattan(c, currentObjective) / 6;
                return std::max(1, cost);
            });

            for (Coord source : ourTiles()) {
                const TileView& src = board.tileAt(source);
                if (src.army < minSourceArmy) continue;
                if (source == target) continue;
                if (isNeutralCity(targetTile) &&
                    !canReasonablyCommitToCity(target, source, reversePath))
                    continue;
                if (isNeutralCity(targetTile) && defenseMode == false &&
                    currentObjective == target && !shouldLaunchNow(-1) &&
                    estimatedArmyAt(target) > 20)
                    continue;

                if (reversePath.distance(idx(source)) >= kInf) continue;
                RoutePreview route = previewRoute(source, target, reversePath);
                if (!route.reachable || route.first == Coord{-1, -1}) continue;

                double score = static_cast<double>(src.army - 1) *
                               (defenseMode ? 2.6 : 1.8);
                score -= reversePath.distance(idx(source)) *
                         (defenseMode ? 3.8 : 2.7);
                score -= route.steps * (defenseMode ? 4.5 : 2.5);
                if (source == myGeneral) score -= defenseMode ? 4.0 : 10.0;
                if (board.tileAt(source).type == TILE_CITY) score -= 3.0;
                if (target == currentObjective) score += 12.0;
                if (!defenseMode && !attackCorridorMask.empty()) {
                    if (attackCorridorMask[idx(source)]) score += 12.0;
                    if (attackCorridorMask[idx(route.first)]) score += 16.0;
                    if (lockedObjective != Coord{-1, -1}) {
                        score -= manhattan(route.first, lockedObjective) * 0.6;
                    }
                }
                if (defenseMode && myGeneral != Coord{-1, -1})
                    score -= manhattan(source, myGeneral) * 1.5;
                if (!defenseMode && currentObjective != Coord{-1, -1})
                    score -= manhattan(source, currentObjective) * 0.8;
                if (shouldBlockOscillation(source, route.first)) continue;
                if (isRecentBounce(source, route.first)) score -= 60.0;

                if (score > best.score) {
                    best.score = score;
                    best.move =
                        Move(MoveType::MOVE_ARMY, source, route.first, false);
                    best.valid = true;
                }
            }
        }

        if (best.valid) return best.move;
        return std::nullopt;
    }

    std::optional<Move> getDefenseMove(const ThreatInfo& threat) const {
        if (threat.source == Coord{-1, -1}) return std::nullopt;

        CandidateMove direct;
        std::set<size_t> defenseTargets;
        defenseTargets.insert(idx(threat.source));
        for (Coord step : threat.route) {
            defenseTargets.insert(idx(step));
            if (step == threat.target) break;
            if (manhattan(step, threat.target) <= 2) break;
        }

        for (size_t targetIndex : defenseTargets) {
            Coord target{static_cast<pos_t>(targetIndex / W),
                         static_cast<pos_t>(targetIndex % W)};
            for (Coord d : kDirs) {
                Coord adj = target + d;
                if (!inside(adj)) continue;
                const TileView& tile = board.tileAt(adj);
                if (tile.occupier != id ||
                    tile.army <= estimatedArmyAt(target) + 1)
                    continue;
                double score =
                    500.0 - manhattan(adj, threat.target) * 12.0 + tile.army;
                if (target == threat.source) score += 60.0;
                if (score > direct.score) {
                    direct.score = score;
                    direct.move = Move(MoveType::MOVE_ARMY, adj, target, false);
                    direct.valid = true;
                }
            }
        }
        if (direct.valid) return direct.move;

        std::vector<Coord> gatherTargets;
        gatherTargets.push_back(threat.target);
        gatherTargets.push_back(threat.source);
        for (Coord step : threat.route) {
            gatherTargets.push_back(step);
            if (manhattan(step, threat.target) <= 1) break;
        }
        return getGatherMoveToTargets(gatherTargets, true, 2);
    }

    std::optional<Move> getQuickGeneralKill(index_t targetPlayer) const {
        if (targetPlayer < 0) return std::nullopt;
        Coord target = chooseTargetPlayerGeneral(targetPlayer);
        if (target == Coord{-1, -1}) return std::nullopt;
        Coord source = strongestTileNearObjective(target);
        if (source == Coord{-1, -1}) return std::nullopt;
        const TileView& src = board.tileAt(source);
        if (src.army <= 1) return std::nullopt;

        auto path = weightedPath(source, [&](Coord c) {
            int cost = attackPenalty(c, true);
            if (!memory[idx(c)].everSeen) cost += 1;
            return cost;
        });
        if (path.distance(idx(target)) >= kInf) return std::nullopt;
        auto route = reconstructPath(source, target, path);
        if (route.empty()) return std::nullopt;

        army_t needed = std::max<army_t>(2, estimatedArmyAt(target) + 2);
        needed += static_cast<army_t>(route.size() / 2);
        if (knownGenerals[targetPlayer] == Coord{-1, -1}) needed += 4;
        if (src.army - 1 < needed) return std::nullopt;
        return Move(MoveType::MOVE_ARMY, source, route.front(), false);
    }

    Coord bestExpansionTarget(index_t targetPlayer) const {
        Coord enemyGuess = chooseTargetPlayerGeneral(targetPlayer);
        const auto& distFromGeneral = bfsDistance(myGeneral);
        Coord best{-1, -1};
        double bestScore = -1e100;
        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                const TileView& tile = board.tileAt(c);
                if (isImpassableTile(tile.type) || isFriendlyTile(tile))
                    continue;
                double score = 0.0;
                if (!memory[idx(c)].everSeen) score += 120.0;
                if (!tile.visible) score += 40.0;
                if (tile.type == TILE_CITY) {
                    if (!shouldConsiderNeutralCityTarget(c, targetPlayer,
                                                         false))
                        continue;
                    score += 220.0 - std::min<army_t>(estimatedArmyAt(c), 150);
                }
                if (shouldAvoidOpeningSwamp(c)) continue;
                if (isEnemyTile(tile)) score += 110.0 + tile.army * 0.35;
                if (tile.type == TILE_BLANK) score += 35.0;
                if (tile.type == TILE_SWAMP) score -= 28.0;
                if (myGeneral != Coord{-1, -1} &&
                    distFromGeneral[idx(c)] < kInf) {
                    score +=
                        std::max(0.0, 18.0 - distFromGeneral[idx(c)] * 0.65);
                }
                int centerDist = std::abs(c.x * 2 - (height + 1)) +
                                 std::abs(c.y * 2 - (width + 1));
                score += std::max(0.0, 18.0 - centerDist * 0.5);
                if (enemyGuess != Coord{-1, -1})
                    score +=
                        std::max(0.0, 42.0 - manhattan(c, enemyGuess) * 1.6);
                if (!attackCorridorMask.empty() && attackCorridorMask[idx(c)])
                    score += 22.0;
                if (c == currentObjective) score += 24.0;
                if (score > bestScore) {
                    bestScore = score;
                    best = c;
                }
            }
        }
        return best;
    }

    std::optional<Move> getExpansionMove(index_t targetPlayer) {
        Coord target = bestExpansionTarget(targetPlayer);
        if (target == Coord{-1, -1}) return std::nullopt;
        currentObjective = target;
        Coord source = strongestTileNearObjective(target);
        if (source == Coord{-1, -1}) return std::nullopt;
        if (isNeutralCity(board.tileAt(target)) &&
            !canReasonablyCommitToCity(target, source))
            return std::nullopt;
        auto path = weightedPath(source, [&](Coord c) {
            int cost = attackPenalty(c, true);
            if (shouldAvoidOpeningSwamp(c)) cost += 1000;
            if (!memory[idx(c)].everSeen) cost -= 1;
            if (targetPlayer >= 0) {
                Coord guess = chooseTargetPlayerGeneral(targetPlayer);
                if (guess != Coord{-1, -1}) cost += manhattan(c, guess) / 5;
            }
            return std::max(1, cost);
        });
        if (path.distance(idx(target)) >= kInf) return std::nullopt;
        auto route = reconstructPath(source, target, path);
        if (route.empty()) return std::nullopt;
        return Move(MoveType::MOVE_ARMY, source, route.front(), false);
    }

    std::optional<Move> getGatherMove(index_t targetPlayer) {
        std::vector<Coord> targets;
        Coord target = currentObjective;
        if (target == Coord{-1, -1}) target = bestExpansionTarget(targetPlayer);
        if (target != Coord{-1, -1} && isNeutralCity(board.tileAt(target)) &&
            !shouldConsiderNeutralCityTarget(target, targetPlayer, false)) {
            target = Coord{-1, -1};
        }
        if (target != Coord{-1, -1}) targets.push_back(target);
        if (auto city = getExplicitCityAttackMove(targetPlayer, false))
            return city;

        Coord enemyGuess = chooseTargetPlayerGeneral(targetPlayer);
        if (enemyGuess != Coord{-1, -1}) {
            targets.push_back(enemyGuess);
            if (myGeneral != Coord{-1, -1}) {
                auto path = weightedPath(
                    myGeneral, [&](Coord c) { return attackPenalty(c, true); });
                if (path.distance(idx(enemyGuess)) < kInf) {
                    auto route = reconstructPath(myGeneral, enemyGuess, path);
                    for (size_t i = 0; i < route.size() && i < 6; ++i)
                        targets.push_back(route[i]);
                }
            }
        }
        return getGatherMoveToTargets(targets, false, 3);
    }

    std::optional<Move> getOpeningMove(index_t targetPlayer) {
        if (shouldDelayOpeningExploration()) return std::nullopt;
        if (auto planned = getOpeningPlannerMove(targetPlayer)) return planned;
        CandidateMove direct = chooseDirectCaptureMove();
        if (direct.valid && direct.score > 200.0) return direct.move;
        Coord target{-1, -1};
        double bestScore = -1e100;
        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                const TileView& tile = board.tileAt(c);
                if (isImpassableTile(tile.type) || isFriendlyTile(tile))
                    continue;
                double score = 0.0;
                if (tile.type == TILE_CITY) {
                    if (!shouldCommitExplicitCityNow(c, targetPlayer, true))
                        continue;
                    score += 180.0 - estimatedArmyAt(c);
                }
                if (!memory[idx(c)].everSeen) score += 120.0;
                if (!tile.visible) score += 50.0;
                if (shouldAvoidOpeningSwamp(c)) continue;
                if (tile.type == TILE_SWAMP) score -= 20.0;
                if (myGeneral != Coord{-1, -1})
                    score -= manhattan(c, myGeneral) * 1.5;
                if (score > bestScore) {
                    bestScore = score;
                    target = c;
                }
            }
        }
        if (target == Coord{-1, -1}) return std::nullopt;
        currentObjective = target;
        Coord source = strongestFriendlyTile(true, target, 2);
        if (source == Coord{-1, -1}) return std::nullopt;
        if (isNeutralCity(board.tileAt(target)) &&
            !canReasonablyCommitToCity(target, source))
            return std::nullopt;
        auto path = weightedPath(
            source, [&](Coord c) { return attackPenalty(c, true); });
        auto route = reconstructPath(source, target, path);
        if (route.empty()) return std::nullopt;
        return Move(MoveType::MOVE_ARMY, source, route.front(), false);
    }

    std::optional<Move> fallbackMove() const {
        CandidateMove direct = chooseDirectCaptureMove();
        if (direct.valid) return direct.move;
        Coord source = strongestFriendlyTile(true, std::nullopt, 0);
        if (source == Coord{-1, -1}) return std::nullopt;
        std::vector<Coord> choices;
        for (Coord d : kDirs) {
            Coord to = source + d;
            if (!inside(to)) continue;
            const TileView& tile = board.tileAt(to);
            if (isImpassableTile(tile.type)) continue;
            if (shouldBlockOscillation(source, to)) continue;
            if (tile.occupier == id && isRecentBounce(source, to)) continue;
            choices.push_back(to);
        }
        if (choices.empty()) return std::nullopt;
        std::sort(choices.begin(), choices.end(), [&](Coord lhs, Coord rhs) {
            auto score = [&](Coord c) {
                double value = 0.0;
                const TileView& tile = board.tileAt(c);
                if (!memory[idx(c)].everSeen) value += 50.0;
                if (tile.occupier != id) value += 25.0;
                if (tile.type == TILE_CITY) value += 80.0 - estimatedArmyAt(c);
                if (tile.type == TILE_SWAMP) value -= 18.0;
                return value;
            };
            return score(lhs) > score(rhs);
        });
        return Move(MoveType::MOVE_ARMY, source, choices.front(), false);
    }

    bool moveLooksSafe(const Move& move) const {
        if (move.type != MoveType::MOVE_ARMY) return false;
        if (!inside(move.from) || !inside(move.to)) return false;
        const TileView& src = board.tileAt(move.from);
        if (src.occupier != id || src.army <= 1) return false;
        army_t remain = move.takeHalf ? (src.army >> 1) : 1;
        if (move.from == myGeneral) {
            army_t nearbyThreat = 0;
            for (Coord d : kDirs) {
                Coord adj = myGeneral + d;
                if (!inside(adj)) continue;
                const TileView& tile = board.tileAt(adj);
                if (isEnemyTile(tile))
                    nearbyThreat = std::max<army_t>(nearbyThreat, tile.army);
            }
            if (remain <= nearbyThreat) return false;
        }
        if (myGeneral != Coord{-1, -1}) {
            for (Coord d : kDirs) {
                Coord adj = myGeneral + d;
                if (!inside(adj)) continue;
                const TileMemory& mem = memory[idx(adj)];
                if (mem.currentlyVisible || mem.occupier < 0 ||
                    mem.occupier == id || isTeammate(mem.occupier))
                    continue;
                if (static_cast<int>(fullTurn) - mem.lastSeenTurn > 4) continue;
                if (move.from == myGeneral && remain <= mem.army) return false;
            }
        }
        return true;
    }

    std::optional<Move> selectStrategicMove() {
        if (fullTurn <= 2) return std::nullopt;
        CandidateMove direct = chooseDirectCaptureMove();
        auto threat = analyzeFastestThreat();
        index_t targetPlayer = chooseLockedTargetPlayer(chooseTargetPlayer());
        updateAttackCorridor(targetPlayer);
        currentStance = chooseStance(targetPlayer, threat);
        if ((currentStance == Stance::LAUNCH ||
             currentStance == Stance::HUNT) &&
            chooseTargetPlayerGeneral(targetPlayer) != Coord{-1, -1}) {
            if (lockedObjective == Coord{-1, -1} ||
                fullTurn > static_cast<turn_t>(objectiveLockUntil)) {
                lockedObjective = chooseTargetPlayerGeneral(targetPlayer);
                objectiveLockUntil = static_cast<int>(fullTurn) + 14;
            }
        }

        if (currentStance == Stance::DEFEND && threat.has_value()) {
            if (auto defense = getDefenseMove(*threat)) return defense;
        }

        if (currentStance == Stance::HUNT) {
            if (auto kill = getQuickGeneralKill(targetPlayer)) return kill;
        }

        if (fullTurn < 50) {
            if (auto open = getOpeningMove(targetPlayer)) return open;
        }

        if (direct.valid && direct.score > 120.0) return direct.move;
        if (shouldLaunchNow(targetPlayer) || currentStance == Stance::LAUNCH ||
            currentStance == Stance::HUNT) {
            if (auto expansion = getExpansionMove(targetPlayer))
                return expansion;
        }
        if (auto expansion = getExpansionMove(targetPlayer)) return expansion;
        if (auto gather = getGatherMove(targetPlayer)) return gather;
        if (direct.valid) return direct.move;
        return fallbackMove();
    }

    void refreshRank(const std::vector<RankItem>& rank) {
        if (!sortedRank.empty()) {
            prevSortedRank = sortedRank;
            hasPrevRank = true;
        }
        sortedRank = rank;
        std::sort(sortedRank.begin(), sortedRank.end(),
                  [](const RankItem& lhs, const RankItem& rhs) {
                      return lhs.player < rhs.player;
                  });
        aliveById.assign(playerCnt, false);
        playerArmyDelta.assign(playerCnt, 0);
        playerLandDelta.assign(playerCnt, 0);
        for (const auto& item : sortedRank) {
            if (item.player >= 0 && item.player < playerCnt) {
                aliveById[item.player] = item.alive;
                if (hasPrevRank &&
                    item.player < static_cast<index_t>(prevSortedRank.size()) &&
                    prevSortedRank[item.player].player == item.player) {
                    playerArmyDelta[item.player] =
                        item.army - prevSortedRank[item.player].army;
                    playerLandDelta[item.player] =
                        item.land - prevSortedRank[item.player].land;
                }
            }
        }
    }

   public:
    void init(index_t playerId, const GameConstantsPack& constants) override {
        id = playerId;
        height = constants.mapHeight;
        width = constants.mapWidth;
        W = width + 2;
        playerCnt = constants.playerCount;
        teams = constants.teams;
        config = constants.config;
        halfTurn = 0;
        fullTurn = 0;
        memory.assign(static_cast<size_t>((height + 2) * W), TileMemory{});
        knownGenerals.assign(playerCnt, Coord{-1, -1});
        predictedGeneralScore.assign(
            playerCnt,
            std::vector<double>(static_cast<size_t>((height + 2) * W), 0.0));
        candidateGeneralMask.assign(
            playerCnt,
            std::vector<uint8_t>(static_cast<size_t>((height + 2) * W), 0));
        attackCorridorMask.assign(static_cast<size_t>((height + 2) * W), 0);
        newlyVisibleEnemyTiles.assign(playerCnt, 0);
        aliveById.assign(playerCnt, true);
        playerArmyDelta.assign(playerCnt, 0);
        playerLandDelta.assign(playerCnt, 0);
        friendlyTilesCache.clear();
        ownedCitiesCache.clear();
        largestFriendlyArmyCache = 0;
        recentEdges.clear();
        recentEdgeCounts.clear();
        lastMoveFrom = Coord{-1, -1};
        lastMoveTo = Coord{-1, -1};
        myGeneral = Coord{-1, -1};
        currentObjective = Coord{-1, -1};
        previousAnchor = Coord{-1, -1};
        openingLaneTarget = Coord{-1, -1};
        openingLaneHeading = Coord{-1, -1};
        lockedObjective = Coord{-1, -1};
        objectiveLockUntil = -1;
        lockedTargetPlayer = -1;
        targetLockUntil = -1;
        decisionSerial = 0;
        targetGeneralCache.assign(playerCnt, Coord{-1, -1});
        targetGeneralCacheStamp.assign(playerCnt, -1);
        timingPlanCache.assign(playerCnt, TimingPlan{});
        timingPlanCacheStamp.assign(playerCnt, -1);
        distFromGeneralCache.assign(static_cast<size_t>((height + 2) * W),
                                    kInf);
        distFromGeneralCacheStamp = -1;
        distFromGeneralSource = Coord{-1, -1};
        openingLaunchTurn = computeOpeningLaunchTurn();
        currentStance = Stance::GATHER;
        prevBoard = BoardView{};
        hasPrevBoard = false;
        prevSortedRank.clear();
        hasPrevRank = false;
    }

    void requestMove(const BoardView& boardView,
                     const std::vector<RankItem>& rank) override {
        ++halfTurn;
        fullTurn += (halfTurn & 1);
        ++decisionSerial;
        board = boardView;
        refreshRank(rank);
        moveQueue.clear();

        rememberVisibleBoard();
        rebuildTurnCaches();
        auto candidate = selectStrategicMove();
        if (!candidate.has_value() || !moveLooksSafe(*candidate))
            candidate = fallbackMove();
        if (!candidate.has_value()) return;
        pushRecentMove(candidate->from, candidate->to);
        previousAnchor = candidate->from;
        moveQueue.push_back(*candidate);
        prevBoard = board;
        hasPrevBoard = true;
    }
};

static BotRegistrar<OimBot> oim_bot_reg("oimbot");
