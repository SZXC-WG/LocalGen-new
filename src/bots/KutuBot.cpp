// Copyright (C) 2026 pinkHC
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file KutuBot.cpp
 *
 * Unified strategic KutuBot for LocalGen v6.
 *
 * @author pinkHC
 */

#ifndef LGEN_BOTS_KUTUBOT
#define LGEN_BOTS_KUTUBOT

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <deque>
#include <iostream>
#include <limits>
#include <numeric>
#include <optional>
#include <queue>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/bot.h"
#include "core/game.hpp"

class KutuBot : public BasicBot {
   private:
    static constexpr std::array<Coord, 4> kDirs = {Coord{-1, 0}, Coord{0, -1},
                                                   Coord{1, 0}, Coord{0, 1}};
    static constexpr int kInf = 1000000000;
    static constexpr int kRecentEdgeWindow = 16;
    static constexpr bool kEnableDecisionTrace = false;
    static constexpr double kPredictionDecay = 0.986;
    static constexpr double kThreatEpsilon = 1e-9;

    struct TileMemory {
        army_t army = 0;
        index_t occupier = -1;
        int lastSeenTurn = -1;
        int estimatedTurn = -1;
        tile_type_e type = TILE_BLANK;
        bool everSeen = false;
        bool visible = false;
    };

    struct PathResult {
        const std::vector<int>* dist = nullptr;
        const std::vector<int>* stamp = nullptr;
        const std::vector<Coord>* parent = nullptr;
        const std::vector<Coord>* firstStep = nullptr;
        const std::vector<int>* depth = nullptr;
        const std::vector<int>* friendlyLead = nullptr;
        int activeStamp = 0;
        bool reachable = false;

        int distance(size_t node) const {
            if (dist == nullptr || stamp == nullptr || node >= dist->size()) {
                return kInf;
            }
            return (*stamp)[node] == activeStamp ? (*dist)[node] : kInf;
        }

        Coord parentAt(size_t node) const {
            if (parent == nullptr || stamp == nullptr ||
                node >= parent->size()) {
                return Coord{-1, -1};
            }
            return (*stamp)[node] == activeStamp ? (*parent)[node]
                                                 : Coord{-1, -1};
        }

        Coord firstStepAt(size_t node) const {
            if (firstStep == nullptr || stamp == nullptr ||
                node >= firstStep->size()) {
                return Coord{-1, -1};
            }
            return (*stamp)[node] == activeStamp ? (*firstStep)[node]
                                                 : Coord{-1, -1};
        }

        int edgeDepthAt(size_t node) const {
            if (depth == nullptr || stamp == nullptr || node >= depth->size()) {
                return kInf;
            }
            return (*stamp)[node] == activeStamp ? (*depth)[node] : kInf;
        }

        int friendlyLeadAt(size_t node) const {
            if (friendlyLead == nullptr || stamp == nullptr ||
                node >= friendlyLead->size()) {
                return 0;
            }
            return (*stamp)[node] == activeStamp ? (*friendlyLead)[node] : 0;
        }
    };

    struct SearchWorkspace {
        std::vector<int> dist;
        std::vector<int> stamp;
        std::vector<Coord> parent;
        std::vector<Coord> firstStep;
        std::vector<int> depth;
        std::vector<int> friendlyLead;
        std::vector<std::pair<int, Coord>> heap;
        int activeStamp = 1;
    };

    struct ThreatSeed {
        double bias = 0.0;
        Coord target{-1, -1};
        int rank = 0;
    };

    struct ThreatPathResult {
        const std::vector<double>* adjustedCost = nullptr;
        const std::vector<int>* rawDist = nullptr;
        const std::vector<int>* steps = nullptr;
        const std::vector<int>* stamp = nullptr;
        const std::vector<int>* seedIndex = nullptr;
        const std::vector<Coord>* parent = nullptr;
        int activeStamp = 0;
        bool reachable = false;

        double cost(size_t node) const {
            if (adjustedCost == nullptr || stamp == nullptr ||
                node >= adjustedCost->size()) {
                return 1e100;
            }
            return (*stamp)[node] == activeStamp ? (*adjustedCost)[node]
                                                 : 1e100;
        }

        int distance(size_t node) const {
            if (rawDist == nullptr || stamp == nullptr || node >= rawDist->size()) {
                return kInf;
            }
            return (*stamp)[node] == activeStamp ? (*rawDist)[node] : kInf;
        }

        int stepCount(size_t node) const {
            if (steps == nullptr || stamp == nullptr || node >= steps->size()) {
                return kInf;
            }
            return (*stamp)[node] == activeStamp ? (*steps)[node] : kInf;
        }

        int seedAt(size_t node) const {
            if (seedIndex == nullptr || stamp == nullptr ||
                node >= seedIndex->size()) {
                return -1;
            }
            return (*stamp)[node] == activeStamp ? (*seedIndex)[node] : -1;
        }

        Coord parentAt(size_t node) const {
            if (parent == nullptr || stamp == nullptr ||
                node >= parent->size()) {
                return Coord{-1, -1};
            }
            return (*stamp)[node] == activeStamp ? (*parent)[node]
                                                 : Coord{-1, -1};
        }
    };

    struct ThreatWorkspace {
        std::vector<double> adjustedCost;
        std::vector<int> rawDist;
        std::vector<int> steps;
        std::vector<int> stamp;
        std::vector<int> seedIndex;
        std::vector<Coord> parent;
        std::vector<std::pair<double, Coord>> heap;
        int activeStamp = 1;
    };

    struct RecentEdge {
        Coord from{-1, -1};
        Coord to{-1, -1};
        std::uint64_t key = 0;
    };

    enum class FocusMode : uint8_t { EXPAND, ATTACK, DEFEND, CONVERT };

    struct ObjectiveOption {
        double score = -1e100;
        Coord target{-1, -1};
        index_t targetPlayer = -1;
        bool attack = false;
        bool defend = false;
        bool city = false;
        bool exploration = false;
    };

    struct StrategicPlan {
        Move move{};
        std::vector<Coord> route;
        double objectiveScore = -1e100;
        double totalScore = -1e100;
        army_t sourceArmy = 0;
        army_t commitArmy = 0;
        army_t reserve = 0;
        army_t corridorSupport = 0;
        army_t need = 0;
        Coord source{-1, -1};
        Coord target{-1, -1};
        Coord firstStep{-1, -1};
        index_t targetPlayer = -1;
        int pathCost = kInf;
        FocusMode mode = FocusMode::EXPAND;
        bool rally = false;
        bool usingRally = false;
        bool valid = false;
    };

    struct CandidateMove {
        Move move{};
        double score = -1e100;
        double riskPenalty = 0.0;
        Coord source{-1, -1};
        Coord target{-1, -1};
        FocusMode mode = FocusMode::EXPAND;
        bool tactical = false;
        bool relievesCore = false;
        bool valid = false;
    };

    struct ThreatInfo {
        std::vector<Coord> route;
        double pressure = -1e100;
        army_t sourceArmy = 0;
        Coord source{-1, -1};
        Coord target{-1, -1};
        Coord intercept{-1, -1};
        int turns = kInf;
        bool visibleSource = false;
        bool recentSource = false;
        bool valid = false;
    };

    struct SituationMetrics {
        double frontierPressure = 0.0;
        double coreRisk = 0.0;
        double contactDensity = 0.0;
        double mobilityScore = 0.0;
        double conversionAdvantage = 0.0;
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
    std::vector<double> predictedGeneralScale;
    std::vector<uint8_t> baseGeneralCandidateMask;
    std::vector<Coord> baseGeneralCandidateList;
    std::vector<int> newlyVisibleEnemyTiles;
    std::vector<Coord> cachedGeneralGuess;
    std::vector<uint8_t> cachedGeneralHasEvidence;

    std::vector<Coord> friendlyTilesCache;
    std::vector<Coord> frontierTiles;
    std::vector<Coord> ownedCitiesCache;
    std::vector<ObjectiveOption> objectiveCandidates;
    std::vector<int> visibleEnemyCountByPlayer;
    std::vector<double> enemyPressure;
    std::vector<double> friendlyPressure;
    std::vector<army_t> reserveArmy;
    std::vector<int> distFromGeneral;
    std::vector<int> pathFlow;
    std::vector<uint8_t> coreZoneMask;
    std::vector<uint8_t> chokeMask;
    std::vector<Coord> generalPathParent;
    std::vector<Coord> generalBfsOrder;

    army_t largestFriendlyArmyCache = 0;
    army_t cachedGeneralAdjacentThreat = 0;
    double cachedGeneralLocalPressure = 0.0;
    bool cachedRelaxedGeneralOpening = false;
    Coord myGeneral{-1, -1};
    Coord currentObjective{-1, -1};
    Coord lockedObjective{-1, -1};
    int objectiveLockUntil = -1;
    index_t currentTargetPlayer = -1;
    int targetLockUntil = -1;
    Coord lastMoveFrom{-1, -1};
    Coord lastMoveTo{-1, -1};

    SearchWorkspace forwardSearch;
    mutable SearchWorkspace reverseSearch;
    mutable ThreatWorkspace closeThreatSearch;
    mutable ThreatWorkspace limitedThreatSearch;
    mutable ThreatWorkspace positiveThreatSearch;
    std::deque<RecentEdge> recentEdges;
    std::unordered_map<std::uint64_t, int> recentEdgeCounts;

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

    inline bool isEnemyOccupier(index_t occupier) const {
        return occupier >= 0 && occupier != id &&
               (occupier >= static_cast<index_t>(teams.size()) ||
                teams[occupier] != teams[id]);
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

    static bool minHeapCompare(const std::pair<int, Coord>& lhs,
                               const std::pair<int, Coord>& rhs) {
        return lhs.first > rhs.first;
    }

    static bool minThreatHeapCompare(const std::pair<double, Coord>& lhs,
                                     const std::pair<double, Coord>& rhs) {
        return lhs.first > rhs.first;
    }

    const char* modeName(FocusMode mode) const {
        switch (mode) {
            case FocusMode::EXPAND:  return "expand";
            case FocusMode::ATTACK:  return "attack";
            case FocusMode::DEFEND:  return "defend";
            case FocusMode::CONVERT: return "convert";
        }
        return "unknown";
    }

    void traceDecision(const char* label, const CandidateMove& move,
                       const SituationMetrics& metrics) const {
        if (!kEnableDecisionTrace || !move.valid) return;
        std::cerr << "[KutuBot] " << label << " target=(" << move.target.x
                  << "," << move.target.y << ") source=(" << move.source.x
                  << "," << move.source.y << ") mode=" << modeName(move.mode)
                  << " score=" << move.score
                  << " riskPenalty=" << move.riskPenalty
                  << " tactical=" << (move.tactical ? 1 : 0)
                  << " relievesCore=" << (move.relievesCore ? 1 : 0)
                  << " frontierPressure=" << metrics.frontierPressure
                  << " coreRisk=" << metrics.coreRisk
                  << " contactDensity=" << metrics.contactDensity
                  << " mobilityScore=" << metrics.mobilityScore
                  << " conversionAdvantage=" << metrics.conversionAdvantage
                  << "\n";
    }

    void traceCompetition(const CandidateMove& direct,
                          const CandidateMove& planned,
                          const CandidateMove& economic,
                          const char* selectedLabel) const {
        if (!kEnableDecisionTrace) return;
        auto emit = [&](const char* label, const CandidateMove& move) {
            if (!move.valid) {
                std::cerr << "[KutuBot] candidate " << label << " invalid\n";
                return;
            }
            const army_t reserve =
                inside(move.source) ? reserveArmy[idx(move.source)] : 0;
            const army_t remain =
                move.valid && move.move.type == MoveType::MOVE_ARMY
                    ? moveRemainingArmy(move.move)
                    : 0;
            std::cerr << "[KutuBot] candidate " << label
                      << " score=" << move.score
                      << " riskPenalty=" << move.riskPenalty
                      << " reserve=" << reserve << " remain=" << remain
                      << " target=(" << move.target.x << "," << move.target.y
                      << ")"
                      << " source=(" << move.source.x << "," << move.source.y
                      << ")"
                      << " tactical=" << (move.tactical ? 1 : 0)
                      << " relievesCore=" << (move.relievesCore ? 1 : 0)
                      << "\n";
        };
        emit("direct", direct);
        emit("planned", planned);
        emit("economic", economic);
        std::cerr << "[KutuBot] selected " << selectedLabel << "\n";
    }

    void prepareWorkspace(SearchWorkspace& workspace) const {
        const size_t total = static_cast<size_t>((height + 2) * W);
        if (workspace.dist.size() != total) {
            workspace.dist.assign(total, kInf);
            workspace.stamp.assign(total, 0);
            workspace.parent.assign(total, Coord{-1, -1});
            workspace.firstStep.assign(total, Coord{-1, -1});
            workspace.depth.assign(total, kInf);
            workspace.friendlyLead.assign(total, 0);
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
        workspace.firstStep[node] = Coord{-1, -1};
        workspace.depth[node] = kInf;
        workspace.friendlyLead[node] = 0;
    }

    void prepareThreatWorkspace(ThreatWorkspace& workspace) const {
        const size_t total = static_cast<size_t>((height + 2) * W);
        if (workspace.adjustedCost.size() != total) {
            workspace.adjustedCost.assign(total, 1e100);
            workspace.rawDist.assign(total, kInf);
            workspace.steps.assign(total, kInf);
            workspace.stamp.assign(total, 0);
            workspace.seedIndex.assign(total, -1);
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

    void touchThreatNode(ThreatWorkspace& workspace, size_t node) const {
        if (workspace.stamp[node] == workspace.activeStamp) return;
        workspace.stamp[node] = workspace.activeStamp;
        workspace.adjustedCost[node] = 1e100;
        workspace.rawDist[node] = kInf;
        workspace.steps[node] = kInf;
        workspace.seedIndex[node] = -1;
        workspace.parent[node] = Coord{-1, -1};
    }

    bool betterThreatCandidate(const ThreatWorkspace& workspace, size_t node,
                               double adjustedCost, int rawDist, int steps,
                               int seedIndex,
                               const std::vector<ThreatSeed>& seeds) const {
        const double current = workspace.adjustedCost[node];
        if (adjustedCost + kThreatEpsilon < current) return true;
        if (current + kThreatEpsilon < adjustedCost) return false;

        const int currentSeed = workspace.seedIndex[node];
        const int currentRank =
            currentSeed >= 0 ? seeds[currentSeed].rank
                             : std::numeric_limits<int>::max();
        const int candidateRank = seeds[seedIndex].rank;
        if (candidateRank != currentRank) return candidateRank < currentRank;
        if (rawDist != workspace.rawDist[node]) return rawDist < workspace.rawDist[node];
        return steps < workspace.steps[node];
    }

    template <typename StepCostFn>
    PathResult weightedPath(Coord start, const StepCostFn& stepCost) {
        PathResult result;
        if (!inside(start)) return result;

        SearchWorkspace& workspace = forwardSearch;
        prepareWorkspace(workspace);
        auto& dist = workspace.dist;
        auto& stamp = workspace.stamp;
        auto& parent = workspace.parent;
        auto& firstStep = workspace.firstStep;
        auto& depth = workspace.depth;
        auto& friendlyLead = workspace.friendlyLead;
        auto& heap = workspace.heap;
        const size_t startIndex = idx(start);
        touchNode(workspace, startIndex);
        dist[startIndex] = 0;
        depth[startIndex] = 0;
        friendlyLead[startIndex] = 0;
        heap.emplace_back(0, start);
        std::push_heap(heap.begin(), heap.end(), minHeapCompare);

        while (!heap.empty()) {
            std::pop_heap(heap.begin(), heap.end(), minHeapCompare);
            const auto [curDist, cur] = heap.back();
            heap.pop_back();
            const size_t curIndex = idx(cur);
            if (curDist != dist[curIndex]) continue;

            const int curDepth = depth[curIndex];
            const int curLead = friendlyLead[curIndex];
            const Coord curFirstStep = firstStep[curIndex];

            for (Coord d : kDirs) {
                Coord nxt = cur + d;
                if (!inside(nxt)) continue;
                const size_t nextIndex = idx(nxt);
                if (!isPassable(memory[nextIndex].type)) continue;
                touchNode(workspace, nextIndex);
                const int nd = curDist + stepCost(nxt);
                if (nd < dist[nextIndex]) {
                    dist[nextIndex] = nd;
                    parent[nextIndex] = cur;
                    firstStep[nextIndex] =
                        cur == start ? nxt : curFirstStep;
                    depth[nextIndex] = curDepth + 1;
                    friendlyLead[nextIndex] =
                        curLead == curDepth && board.tileAt(nxt).occupier == id
                            ? curDepth + 1
                            : curLead;
                    heap.emplace_back(nd, nxt);
                    std::push_heap(heap.begin(), heap.end(), minHeapCompare);
                }
            }
        }

        result.reachable = true;
        result.dist = &dist;
        result.stamp = &stamp;
        result.parent = &parent;
        result.firstStep = &firstStep;
        result.depth = &depth;
        result.friendlyLead = &friendlyLead;
        result.activeStamp = workspace.activeStamp;
        return result;
    }

    template <typename StepCostFn>
    PathResult weightedReversePath(Coord goal,
                                   const StepCostFn& stepCost) const {
        PathResult result;
        if (!inside(goal)) return result;

        SearchWorkspace& workspace = reverseSearch;
        prepareWorkspace(workspace);
        auto& dist = workspace.dist;
        auto& stamp = workspace.stamp;
        auto& parent = workspace.parent;
        auto& firstStep = workspace.firstStep;
        auto& depth = workspace.depth;
        auto& friendlyLead = workspace.friendlyLead;
        auto& heap = workspace.heap;
        const size_t goalIndex = idx(goal);
        touchNode(workspace, goalIndex);
        dist[goalIndex] = 0;
        heap.emplace_back(0, goal);
        std::push_heap(heap.begin(), heap.end(), minHeapCompare);

        while (!heap.empty()) {
            std::pop_heap(heap.begin(), heap.end(), minHeapCompare);
            const auto [curDist, cur] = heap.back();
            heap.pop_back();
            const size_t curIndex = idx(cur);
            if (curDist != dist[curIndex]) continue;

            const int stepIntoCur = stepCost(cur);

            for (Coord d : kDirs) {
                Coord prev = cur + d;
                if (!inside(prev)) continue;
                const size_t prevIndex = idx(prev);
                if (!isPassable(memory[prevIndex].type)) continue;
                touchNode(workspace, prevIndex);
                const int nd = curDist + stepIntoCur;
                if (nd < dist[prevIndex]) {
                    dist[prevIndex] = nd;
                    parent[prevIndex] = cur;
                    heap.emplace_back(nd, prev);
                    std::push_heap(heap.begin(), heap.end(), minHeapCompare);
                }
            }
        }

        result.reachable = true;
        result.dist = &dist;
        result.stamp = &stamp;
        result.parent = &parent;
        result.firstStep = &firstStep;
        result.depth = &depth;
        result.friendlyLead = &friendlyLead;
        result.activeStamp = workspace.activeStamp;
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

    std::vector<Coord> reconstructReversePath(Coord start, Coord goal,
                                              const PathResult& path) const {
        std::vector<Coord> route;
        if (!inside(start) || !inside(goal)) return route;
        Coord cur = start;
        int guard = height * width + 5;
        while (cur != Coord{-1, -1} && cur != goal && guard-- > 0) {
            cur = path.parentAt(idx(cur));
            if (cur == Coord{-1, -1}) return {};
            route.push_back(cur);
        }
        if (cur != goal) return {};
        return route;
    }

    ThreatPathResult multiSourceThreatReversePath(
        const std::vector<ThreatSeed>& seeds, int maxRawDist,
        ThreatWorkspace& workspace) const {
        ThreatPathResult result;
        if (seeds.empty()) return result;

        prepareThreatWorkspace(workspace);
        for (size_t seedIndex = 0; seedIndex < seeds.size(); ++seedIndex) {
            const ThreatSeed& seed = seeds[seedIndex];
            if (!inside(seed.target)) continue;
            const size_t node = idx(seed.target);
            touchThreatNode(workspace, node);
            const double adjustedCost = -seed.bias / 10.5;
            if (!betterThreatCandidate(workspace, node, adjustedCost, 0, 0,
                                       static_cast<int>(seedIndex), seeds)) {
                continue;
            }
            workspace.adjustedCost[node] = adjustedCost;
            workspace.rawDist[node] = 0;
            workspace.steps[node] = 0;
            workspace.seedIndex[node] = static_cast<int>(seedIndex);
            workspace.parent[node] = Coord{-1, -1};
            workspace.heap.emplace_back(adjustedCost, seed.target);
            std::push_heap(workspace.heap.begin(), workspace.heap.end(),
                           minThreatHeapCompare);
        }

        while (!workspace.heap.empty()) {
            std::pop_heap(workspace.heap.begin(), workspace.heap.end(),
                          minThreatHeapCompare);
            const auto [curCost, cur] = workspace.heap.back();
            workspace.heap.pop_back();
            const size_t curNode = idx(cur);
            if (curCost > workspace.adjustedCost[curNode] + kThreatEpsilon) {
                continue;
            }

            const int curRawDist = workspace.rawDist[curNode];
            if (curRawDist >= maxRawDist) continue;
            const int stepCost =
                1 + (memory[curNode].type == TILE_SWAMP ? 5 : 0);

            for (Coord d : kDirs) {
                Coord prev = cur + d;
                if (!inside(prev)) continue;
                if (!isPassable(memory[idx(prev)].type)) continue;
                const int candRawDist = curRawDist + stepCost;
                if (candRawDist > maxRawDist) continue;

                const size_t prevNode = idx(prev);
                touchThreatNode(workspace, prevNode);
                const int seedIndex = workspace.seedIndex[curNode];
                const int candSteps = workspace.steps[curNode] + 1;
                const double candCost = curCost + stepCost;
                if (!betterThreatCandidate(workspace, prevNode, candCost,
                                           candRawDist, candSteps, seedIndex,
                                           seeds)) {
                    continue;
                }

                workspace.adjustedCost[prevNode] = candCost;
                workspace.rawDist[prevNode] = candRawDist;
                workspace.steps[prevNode] = candSteps;
                workspace.seedIndex[prevNode] = seedIndex;
                workspace.parent[prevNode] = cur;
                workspace.heap.emplace_back(candCost, prev);
                std::push_heap(workspace.heap.begin(), workspace.heap.end(),
                               minThreatHeapCompare);
            }
        }

        result.reachable = true;
        result.adjustedCost = &workspace.adjustedCost;
        result.rawDist = &workspace.rawDist;
        result.steps = &workspace.steps;
        result.stamp = &workspace.stamp;
        result.seedIndex = &workspace.seedIndex;
        result.parent = &workspace.parent;
        result.activeStamp = workspace.activeStamp;
        return result;
    }

    std::vector<Coord> reconstructThreatRoute(Coord start,
                                              const ThreatPathResult& path) const {
        std::vector<Coord> route;
        if (!inside(start)) return route;
        Coord cur = start;
        int guard = height * width + 5;
        while (cur != Coord{-1, -1} && guard-- > 0) {
            Coord nxt = path.parentAt(idx(cur));
            if (nxt == Coord{-1, -1}) break;
            route.push_back(nxt);
            cur = nxt;
        }
        return route;
    }

    void computeGeneralDistances() {
        const size_t total = static_cast<size_t>((height + 2) * W);
        distFromGeneral.assign(total, kInf);
        generalBfsOrder.clear();
        if (myGeneral == Coord{-1, -1}) return;

        std::queue<Coord> q;
        distFromGeneral[idx(myGeneral)] = 0;
        q.push(myGeneral);
        while (!q.empty()) {
            Coord cur = q.front();
            q.pop();
            generalBfsOrder.push_back(cur);
            for (Coord d : kDirs) {
                Coord nxt = cur + d;
                if (!inside(nxt)) continue;
                if (!isPassable(memory[idx(nxt)].type)) continue;
                if (distFromGeneral[idx(nxt)] != kInf) continue;
                distFromGeneral[idx(nxt)] = distFromGeneral[idx(cur)] + 1;
                q.push(nxt);
            }
        }
    }

    void renormalizePredictionPlayer(index_t player) {
        if (player < 0 || player >= playerCnt) return;
        double scale = predictedGeneralScale[player];
        if (std::abs(scale - 1.0) <= 1e-12) return;
        if (scale == 0.0) {
            std::fill(predictedGeneralScore[player].begin(),
                      predictedGeneralScore[player].end(), 0.0);
            predictedGeneralScale[player] = 1.0;
            return;
        }
        for (double& score : predictedGeneralScore[player]) score *= scale;
        predictedGeneralScale[player] = 1.0;
    }

    void advancePredictionDecay() {
        for (index_t player = 0; player < playerCnt; ++player) {
            if (player == id) continue;
            predictedGeneralScale[player] *= kPredictionDecay;
            if (predictedGeneralScale[player] < 1e-6) {
                renormalizePredictionPlayer(player);
            }
        }
    }

    double predictionScoreAt(index_t player, Coord c) const {
        return predictedGeneralScore[player][idx(c)] *
               predictedGeneralScale[player];
    }

    void addPredictionScore(index_t player, Coord c, double delta) {
        if (delta == 0.0 || player < 0 || player >= playerCnt || !inside(c)) {
            return;
        }
        if (predictedGeneralScale[player] < 1e-9) {
            renormalizePredictionPlayer(player);
        }
        predictedGeneralScore[player][idx(c)] +=
            delta / predictedGeneralScale[player];
    }

    bool tileGrowsEveryTurn(tile_type_e type) const {
        return type == TILE_GENERAL || type == TILE_CITY;
    }

    void evolveHiddenMemory(TileMemory& mem) {
        if (!mem.everSeen || mem.visible || mem.estimatedTurn < 0) return;
        if (fullTurn <= static_cast<turn_t>(mem.estimatedTurn)) return;

        for (int turn = mem.estimatedTurn + 1;
             turn <= static_cast<int>(fullTurn); ++turn) {
            if (mem.occupier < 0) break;
            switch (mem.type) {
                case TILE_GENERAL:
                case TILE_CITY:    ++mem.army; break;
                case TILE_BLANK:
                    if (turn > 0 && turn % 25 == 0) ++mem.army;
                    break;
                case TILE_SWAMP:
                    if (mem.army > 0) --mem.army;
                    if (mem.army <= 0) {
                        mem.army = 0;
                        mem.occupier = -1;
                    }
                    break;
                default: break;
            }
            if (mem.occupier >= 0 && mem.army > 0 && mem.type != TILE_SWAMP) {
                mem.army = std::max<army_t>(1, mem.army);
            }
        }
        mem.estimatedTurn = static_cast<int>(fullTurn);
    }

    army_t estimatedArmyAt(Coord c) const {
        const TileView& tile = board.tileAt(c);
        return tile.visible ? tile.army : memory[idx(c)].army;
    }

    bool recentEnemyAt(Coord c, int maxAge = 4) const {
        if (!inside(c)) return false;
        const TileView& tile = board.tileAt(c);
        const TileMemory& mem = memory[idx(c)];
        return !tile.visible && isEnemyOccupier(mem.occupier) &&
               mem.lastSeenTurn >= 0 &&
               static_cast<int>(fullTurn) - mem.lastSeenTurn <= maxAge;
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

    void reinforceGeneralNeighborhood(index_t player, Coord anchor,
                                      double baseScore, int maxDist,
                                      bool fogOnly) {
        for (int dx = -maxDist; dx <= maxDist; ++dx) {
            const int rem = maxDist - std::abs(dx);
            for (int dy = -rem; dy <= rem; ++dy) {
                Coord c{static_cast<pos_t>(anchor.x + dx),
                        static_cast<pos_t>(anchor.y + dy)};
                if (!inside(c)) continue;
                const TileMemory& mem = memory[idx(c)];
                if (!isPassable(mem.type) || mem.type == TILE_CITY) continue;
                if (fogOnly && mem.visible) continue;
                const double gain =
                    std::max(0.0, baseScore - manhattan(c, anchor) * 2.2);
                addPredictionScore(player, c, gain);
            }
        }
    }

    void rememberVisibleBoard() {
        advancePredictionDecay();
        newlyVisibleEnemyTiles.assign(playerCnt, 0);

        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                const TileView& tile = board.tileAt(c);
                TileMemory& mem = memory[idx(c)];
                const bool wasVisible = mem.visible;
                mem.visible = tile.visible;
                if (!tile.visible) {
                    if (!mem.everSeen) {
                        mem.type = tile.type == TILE_OBSTACLE ? TILE_OBSTACLE
                                                              : tile.type;
                        mem.estimatedTurn = static_cast<int>(fullTurn);
                    } else {
                        evolveHiddenMemory(mem);
                    }
                    continue;
                }

                mem.everSeen = true;
                mem.type = tile.type;
                mem.army = tile.army;
                mem.occupier = tile.occupier;
                mem.lastSeenTurn = static_cast<int>(fullTurn);
                mem.estimatedTurn = static_cast<int>(fullTurn);

                if (tile.type == TILE_GENERAL && tile.occupier >= 0) {
                    knownGenerals[tile.occupier] = c;
                }

                if (tile.occupier >= 0 && tile.occupier != id && !wasVisible) {
                    newlyVisibleEnemyTiles[tile.occupier]++;
                    reinforceGeneralNeighborhood(tile.occupier, c, 16.0, 7,
                                                 true);
                }
            }
        }

        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                const TileView& tile = board.tileAt(c);
                if (!tile.visible || tile.occupier < 0 || tile.occupier == id) {
                    continue;
                }
                reinforceGeneralNeighborhood(tile.occupier, c, 9.0, 4, false);
            }
        }
    }

    void buildBaseGeneralCandidateMask() {
        std::fill(baseGeneralCandidateMask.begin(),
                  baseGeneralCandidateMask.end(), 0);
        baseGeneralCandidateList.clear();
        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                const TileMemory& mem = memory[idx(c)];
                if (!isPassable(mem.type) || mem.visible ||
                    mem.type == TILE_CITY) {
                    continue;
                }
                if (myGeneral != Coord{-1, -1} && manhattan(c, myGeneral) < 9) {
                    continue;
                }
                baseGeneralCandidateMask[idx(c)] = 1;
                baseGeneralCandidateList.push_back(c);
            }
        }
    }

    void rebuildTurnCaches() {
        friendlyTilesCache.clear();
        frontierTiles.clear();
        ownedCitiesCache.clear();
        objectiveCandidates.clear();
        largestFriendlyArmyCache = 0;
        std::fill(visibleEnemyCountByPlayer.begin(),
                  visibleEnemyCountByPlayer.end(), 0);

        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                const TileView& tile = board.tileAt(c);
                const TileMemory& mem = memory[idx(c)];
                if (tile.visible && isEnemyTile(tile) && tile.occupier >= 0 &&
                    tile.occupier < static_cast<index_t>(
                                        visibleEnemyCountByPlayer.size())) {
                    ++visibleEnemyCountByPlayer[tile.occupier];
                }

                if (tile.occupier != id) continue;
                friendlyTilesCache.push_back(c);
                largestFriendlyArmyCache =
                    std::max(largestFriendlyArmyCache, tile.army);
                if (tile.type == TILE_CITY) ownedCitiesCache.push_back(c);
                if (tile.type == TILE_GENERAL) myGeneral = c;

                bool frontier = false;
                for (Coord d : kDirs) {
                    Coord adj = c + d;
                    if (!inside(adj)) continue;
                    const TileView& adjTile = board.tileAt(adj);
                    const TileMemory& adjMem = memory[idx(adj)];
                    if (!isFriendlyTile(adjTile)) frontier = true;
                    if (!adjTile.visible && !adjMem.everSeen) frontier = true;
                    if (recentEnemyAt(adj)) frontier = true;
                }
                if (frontier) frontierTiles.push_back(c);
            }
        }
    }

    void refreshGeneralGuessCache(const SituationMetrics& metrics) {
        std::fill(cachedGeneralGuess.begin(), cachedGeneralGuess.end(),
                  Coord{-1, -1});
        std::fill(cachedGeneralHasEvidence.begin(),
                  cachedGeneralHasEvidence.end(), 0);

        for (index_t player = 0; player < playerCnt; ++player) {
            if (player == id) continue;

            cachedGeneralHasEvidence[player] =
                knownGenerals[player] != Coord{-1, -1} ||
                visibleEnemyCountByPlayer[player] > 0 ||
                newlyVisibleEnemyTiles[player] > 0 ||
                metrics.contactDensity > 0.035 ||
                metrics.conversionAdvantage > 12.0;

            if (knownGenerals[player] != Coord{-1, -1}) {
                cachedGeneralGuess[player] = knownGenerals[player];
                continue;
            }

            double bestScore = -1e100;
            Coord best{-1, -1};
            const std::vector<double>& scores = predictedGeneralScore[player];
            const double scale = predictedGeneralScale[player];
            for (Coord c : baseGeneralCandidateList) {
                double score = scores[idx(c)] * scale;
                if (myGeneral != Coord{-1, -1}) {
                    score -= manhattan(c, myGeneral) * 0.18;
                }
                if (score > bestScore) {
                    bestScore = score;
                    best = c;
                }
            }
            cachedGeneralGuess[player] = best;
        }
    }

    Coord chooseTargetPlayerGeneral(index_t player) const {
        if (player < 0 || player >= playerCnt) return Coord{-1, -1};
        if (knownGenerals[player] != Coord{-1, -1})
            return knownGenerals[player];
        if (player >= 0 && player < static_cast<index_t>(cachedGeneralGuess.size())) {
            return cachedGeneralGuess[player];
        }
        return Coord{-1, -1};
    }

    int passableDegree(Coord c) const {
        int degree = 0;
        for (Coord d : kDirs) {
            Coord nxt = c + d;
            if (inside(nxt) && isPassable(memory[idx(nxt)].type)) ++degree;
        }
        return degree;
    }

    const std::vector<std::array<int, 3>>& pressureKernel(int radius) const {
        static const auto kernels = [] {
            std::array<std::vector<std::array<int, 3>>, 7> result;
            for (int r = 0; r <= 6; ++r) {
                for (int dx = -r; dx <= r; ++dx) {
                    const int rem = r - std::abs(dx);
                    for (int dy = -rem; dy <= rem; ++dy) {
                        result[r].push_back(
                            {dx, dy, std::abs(dx) + std::abs(dy)});
                    }
                }
            }
            return result;
        }();
        return kernels[std::clamp(radius, 0, 6)];
    }

    void addPressure(std::vector<double>& pressure, Coord source, army_t army,
                     double baseMultiplier, double stepLoss) const {
        if (!inside(source) || army <= 0) return;
        const int radius = std::clamp(
            static_cast<int>(
                std::sqrt(static_cast<double>(std::max<army_t>(1, army)))) +
                1,
            2, 6);
        const double capped = std::min<double>(army, 120.0) * baseMultiplier;
        const auto& kernel = pressureKernel(radius);
        for (const auto& offset : kernel) {
            Coord c{static_cast<pos_t>(source.x + offset[0]),
                    static_cast<pos_t>(source.y + offset[1])};
            if (!inside(c)) continue;
            const size_t node = idx(c);
            if (!isPassable(memory[node].type)) continue;
            const double gain = std::max(0.0, capped - offset[2] * stepLoss);
            pressure[node] += gain;
        }
    }

    void recomputeSituationMaps() {
        const size_t total = static_cast<size_t>((height + 2) * W);
        enemyPressure.assign(total, 0.0);
        friendlyPressure.assign(total, 0.0);
        reserveArmy.assign(total, 0);
        pathFlow.assign(total, 0);
        coreZoneMask.assign(total, 0);
        chokeMask.assign(total, 0);

        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                const TileView& tile = board.tileAt(c);
                if (tile.visible) {
                    if (tile.army <= 0) continue;
                    if (isEnemyTile(tile)) {
                        addPressure(enemyPressure, c, tile.army, 1.0, 15.0);
                    } else if (tile.occupier == id) {
                        addPressure(friendlyPressure, c, tile.army, 0.9, 13.0);
                    }
                    continue;
                }

                const TileMemory& mem = memory[idx(c)];
                if (!mem.everSeen || mem.army <= 0 || mem.lastSeenTurn < 0) {
                    continue;
                }
                const int age = static_cast<int>(fullTurn) - mem.lastSeenTurn;
                if (age > 4) continue;
                const army_t seenArmy =
                    std::max<army_t>(1, mem.army - static_cast<army_t>(age));
                if (isEnemyOccupier(mem.occupier)) {
                    addPressure(enemyPressure, c, seenArmy, 0.75, 16.0);
                } else if (mem.occupier == id) {
                    addPressure(friendlyPressure, c, seenArmy, 0.7, 14.0);
                }
            }
        }

        computeGeneralDistances();
        if (myGeneral == Coord{-1, -1}) return;

        const int area = static_cast<int>(height * width);
        const int coreRadius = std::clamp(
            static_cast<int>(std::sqrt(static_cast<double>(area)) / 1.8), 5,
            12);

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

        refreshGeneralSafetyCache();

        std::fill(generalPathParent.begin(), generalPathParent.end(),
                  Coord{-1, -1});
        for (Coord c : generalBfsOrder) {
            if (c == myGeneral) continue;
            const int curDist = distFromGeneral[idx(c)];
            if (curDist == kInf) continue;

            Coord best{-1, -1};
            double bestPressure = 0.0;
            bool found = false;
            for (Coord d : kDirs) {
                Coord cand = c + d;
                if (!inside(cand)) continue;
                if (!isPassable(memory[idx(cand)].type)) continue;
                if (distFromGeneral[idx(cand)] != curDist - 1) continue;
                const double candPressure = enemyPressure[idx(cand)];
                if (!found || candPressure < bestPressure) {
                    found = true;
                    best = cand;
                    bestPressure = candPressure;
                }
            }
            generalPathParent[idx(c)] = best;
        }

        for (Coord frontier : frontierTiles) {
            if (distFromGeneral[idx(frontier)] == kInf) continue;
            pathFlow[idx(frontier)]++;
        }
        for (auto it = generalBfsOrder.rbegin(); it != generalBfsOrder.rend();
             ++it) {
            const Coord cur = *it;
            if (cur == myGeneral) continue;
            const Coord parent = generalPathParent[idx(cur)];
            if (parent != Coord{-1, -1}) {
                pathFlow[idx(parent)] += pathFlow[idx(cur)];
            }
        }

        for (Coord c : friendlyTilesCache) {
            const TileView& tile = board.tileAt(c);
            army_t reserve = 0;
            if (c == myGeneral) {
                if (relaxedGeneralOpening()) {
                    reserve = 1;
                } else {
                    reserve = std::max<army_t>(
                        2, 2 + static_cast<army_t>(std::max(
                                   0.0, enemyPressure[idx(c)] * 0.08)));
                    if (fullTurn >= 60) reserve = std::max<army_t>(reserve, 3);
                }
            }
            if (coreZoneMask[idx(c)] &&
                enemyPressure[idx(c)] > friendlyPressure[idx(c)] * 0.55 + 6.0) {
                reserve = std::max<army_t>(
                    reserve, static_cast<army_t>(std::max(
                                 0.0, (enemyPressure[idx(c)] -
                                       friendlyPressure[idx(c)] * 0.55) *
                                          0.05)));
            }
            if (tile.type == TILE_CITY && coreZoneMask[idx(c)]) {
                reserve = std::max<army_t>(
                    reserve, 2 + static_cast<army_t>(std::max(
                                     0.0, enemyPressure[idx(c)] * 0.04)));
            }
            const int flow = pathFlow[idx(c)];
            const int degree = passableDegree(c);
            if (flow >= 2 && degree <= 2 && distFromGeneral[idx(c)] >= 2 &&
                distFromGeneral[idx(c)] <= coreRadius + 6) {
                chokeMask[idx(c)] = 1;
                reserve = std::max<army_t>(
                    reserve, 1 + flow / 3 +
                                 static_cast<army_t>(std::max(
                                     0.0, enemyPressure[idx(c)] * 0.03)));
            }
            const army_t movableFloor =
                tile.army <= 1 ? 0 : std::max<army_t>(0, tile.army - 2);
            reserveArmy[idx(c)] = std::min(reserve, movableFloor);
        }
    }

    SituationMetrics computeSituationMetrics() const {
        SituationMetrics metrics;

        if (!frontierTiles.empty()) {
            double frontierLoad = 0.0;
            double contactEdges = 0.0;
            for (Coord c : frontierTiles) {
                frontierLoad +=
                    std::max(0.0, enemyPressure[idx(c)] -
                                      friendlyPressure[idx(c)] * 0.65);
                for (Coord d : kDirs) {
                    Coord adj = c + d;
                    if (!inside(adj)) continue;
                    if (isEnemyTile(board.tileAt(adj)) || recentEnemyAt(adj)) {
                        contactEdges += 1.0;
                    }
                }
            }
            metrics.frontierPressure =
                frontierLoad / static_cast<double>(frontierTiles.size());
            metrics.contactDensity =
                contactEdges /
                std::max(1.0, static_cast<double>(frontierTiles.size()) * 4.0);
        }

        if (myGeneral != Coord{-1, -1}) {
            metrics.coreRisk =
                std::max(0.0, enemyPressure[idx(myGeneral)] -
                                  friendlyPressure[idx(myGeneral)] * 0.55);
            for (Coord city : ownedCitiesCache) {
                if (!coreZoneMask[idx(city)]) continue;
                metrics.coreRisk +=
                    std::max(0.0, enemyPressure[idx(city)] -
                                      friendlyPressure[idx(city)] * 0.70) *
                    0.20;
            }
        }

        double mobileArmy = 0.0;
        int mobileSources = 0;
        for (Coord c : friendlyTilesCache) {
            const TileView& tile = board.tileAt(c);
            const army_t available =
                std::max<army_t>(0, tile.army - 1 - reserveArmy[idx(c)]);
            if (available <= 0) continue;
            mobileArmy += available;
            ++mobileSources;
        }
        metrics.mobilityScore =
            mobileSources == 0
                ? 0.0
                : mobileArmy / mobileSources +
                      static_cast<double>(frontierTiles.size()) * 0.25;

        army_t strongestEnemyArmy = 0;
        double averageEnemyLand = 0.0;
        int enemyCnt = 0;
        for (index_t player = 0; player < playerCnt; ++player) {
            if (player == id || !aliveById[player] ||
                isFriendlyOccupier(player)) {
                continue;
            }
            strongestEnemyArmy =
                std::max(strongestEnemyArmy, rankById[player].army);
            averageEnemyLand += rankById[player].land;
            ++enemyCnt;
        }
        if (enemyCnt > 0) averageEnemyLand /= enemyCnt;
        metrics.conversionAdvantage =
            (rankById[id].army - strongestEnemyArmy) * 0.12 +
            (rankById[id].land - averageEnemyLand) * 0.70 -
            metrics.frontierPressure * 0.25 - metrics.coreRisk * 0.18;

        return metrics;
    }

    double targetPlayerScore(index_t player,
                             const SituationMetrics& metrics) const {
        if (player < 0 || player >= playerCnt || player == id ||
            !aliveById[player] || isFriendlyOccupier(player)) {
            return -1e100;
        }

        double score = 0.0;
        Coord guess = chooseTargetPlayerGeneral(player);
        const bool hasGuessEvidence =
            player >= 0 &&
            player < static_cast<index_t>(cachedGeneralHasEvidence.size()) &&
            cachedGeneralHasEvidence[player];
        if (knownGenerals[player] != Coord{-1, -1}) {
            score += 120.0;
        } else if (guess != Coord{-1, -1} && hasGuessEvidence) {
            score += 30.0;
        }
        if (guess != Coord{-1, -1} && hasGuessEvidence &&
            myGeneral != Coord{-1, -1}) {
            score += std::max(0.0, 72.0 - manhattan(guess, myGeneral) * 1.8);
        }

        score += newlyVisibleEnemyTiles[player] * 12.0;
        score += visibleEnemyCountByPlayer[player] * 9.0;
        score +=
            std::max<army_t>(0, rankById[id].army - rankById[player].army) *
            0.08;
        score += std::max<pos_t>(0, rankById[id].land - rankById[player].land) *
                 0.55;
        score += std::max<army_t>(0, -playerArmyDelta[player]) * 1.4;
        score += std::max<pos_t>(0, -playerLandDelta[player]) * 4.5;
        score += metrics.conversionAdvantage * 1.2;

        if (guess != Coord{-1, -1}) {
            const double localPressure =
                enemyPressure[idx(guess)] - friendlyPressure[idx(guess)];
            score += std::clamp(-localPressure * 0.18, -20.0, 20.0) *
                     (hasGuessEvidence ? 1.0 : 0.25);
        }
        return score;
    }

    index_t chooseLockedTargetPlayer(const SituationMetrics& metrics) {
        index_t suggested = -1;
        double bestScore = -1e100;
        for (index_t player = 0; player < playerCnt; ++player) {
            const double score = targetPlayerScore(player, metrics);
            if (score > bestScore) {
                bestScore = score;
                suggested = player;
            }
        }
        if (suggested < 0) return -1;

        if (currentTargetPlayer < 0 || !aliveById[currentTargetPlayer] ||
            fullTurn > static_cast<turn_t>(targetLockUntil)) {
            currentTargetPlayer = suggested;
            targetLockUntil = static_cast<int>(fullTurn) + 12;
            return currentTargetPlayer;
        }

        const double currentScore =
            targetPlayerScore(currentTargetPlayer, metrics);
        const double nextScore = targetPlayerScore(suggested, metrics);
        if (suggested != currentTargetPlayer &&
            nextScore > currentScore + 32.0) {
            currentTargetPlayer = suggested;
            targetLockUntil = static_cast<int>(fullTurn) + 12;
        }
        return currentTargetPlayer;
    }

    bool shouldPersistObjective(Coord objective) const {
        if (objective == Coord{-1, -1}) return false;
        if (!inside(objective)) return false;
        return board.tileAt(objective).occupier != id;
    }

    int attackPenalty(Coord c, bool allowGeneralDive) const {
        const TileMemory& mem = memory[idx(c)];
        int penalty = 2;
        if (mem.type == TILE_SWAMP) penalty += 18;
        if (mem.type == TILE_CITY) penalty += 10;
        if (!mem.everSeen) penalty += 3;
        if (isEnemyOccupier(mem.occupier)) {
            penalty +=
                static_cast<int>(std::min<army_t>(estimatedArmyAt(c), 120));
            if (mem.type == TILE_GENERAL && allowGeneralDive) penalty -= 22;
        } else if (mem.occupier == id) {
            penalty -= 3;
        }
        return std::max(1, penalty);
    }

    int planningPenalty(Coord c, FocusMode mode,
                        const SituationMetrics& metrics) const {
        int cost = attackPenalty(c, true);
        const TileMemory& mem = memory[idx(c)];
        const double netPressure =
            enemyPressure[idx(c)] - friendlyPressure[idx(c)];
        switch (mode) {
            case FocusMode::EXPAND:
                if (!mem.everSeen) cost = std::max(1, cost - 1);
                if (mem.type == TILE_SWAMP) cost += 12;
                if (netPressure > 8.0)
                    cost += static_cast<int>(netPressure * 0.06);
                break;
            case FocusMode::ATTACK:
                if (isEnemyOccupier(mem.occupier)) cost = std::max(1, cost - 2);
                if (mem.occupier == id) cost = std::max(1, cost - 1);
                break;
            case FocusMode::DEFEND:
                if (myGeneral != Coord{-1, -1} &&
                    distFromGeneral[idx(c)] < kInf) {
                    cost -= std::min(3, distFromGeneral[idx(c)] / 2);
                }
                break;
            case FocusMode::CONVERT:
                if (isEnemyOccupier(mem.occupier)) cost = std::max(1, cost - 3);
                if (!mem.visible) cost += 1;
                if (metrics.conversionAdvantage > 0.0) {
                    cost = std::max(
                        1,
                        cost - static_cast<int>(std::min(
                                   3.0, metrics.conversionAdvantage * 0.08)));
                }
                break;
        }
        return std::max(1, cost);
    }

    army_t availableArmy(Coord c) const {
        if (!inside(c)) return 0;
        const TileView& tile = board.tileAt(c);
        if (tile.occupier != id || tile.army <= 1) return 0;
        return std::max<army_t>(0, tile.army - 1 - reserveArmy[idx(c)]);
    }

    Coord strongestFriendlyTile(bool includeGeneral,
                                std::optional<Coord> near = std::nullopt,
                                int nearWeight = 0) const {
        Coord best{-1, -1};
        double bestScore = -1e100;
        for (Coord c : friendlyTilesCache) {
            const TileView& tile = board.tileAt(c);
            if (tile.army <= 1) continue;
            if (!includeGeneral && c == myGeneral) continue;
            double score = static_cast<double>(availableArmy(c)) * 3.0 +
                           static_cast<double>(tile.army);
            if (tile.type == TILE_CITY) score -= 4.0;
            if (c == myGeneral) score -= 8.0;
            if (chokeMask[idx(c)]) score -= 5.0;
            if (near.has_value()) score -= manhattan(c, *near) * nearWeight;
            if (score > bestScore) {
                bestScore = score;
                best = c;
            }
        }
        return best;
    }

    bool shouldBlockOscillation(Coord from, Coord to) const {
        const std::uint64_t key = edgeKey(from, to);
        auto it = recentEdgeCounts.find(key);
        const int seen = it == recentEdgeCounts.end() ? 0 : it->second;
        if (from == lastMoveTo && to == lastMoveFrom) return true;
        return seen >= 3 && board.tileAt(to).occupier == id;
    }

    army_t moveRemainingArmy(const Move& move) const {
        if (!inside(move.from)) return 0;
        const TileView& src = board.tileAt(move.from);
        if (src.occupier != id || src.army <= 1) return 0;
        return move.takeHalf ? (src.army - (src.army >> 1)) : 1;
    }

    army_t adjacentEnemyThreat(Coord origin) const {
        if (!inside(origin)) return 0;
        army_t threat = 0;
        for (Coord d : kDirs) {
            Coord adj = origin + d;
            if (!inside(adj)) continue;
            const TileView& tile = board.tileAt(adj);
            if (isEnemyTile(tile)) {
                threat = std::max(threat, tile.army);
            } else if (recentEnemyAt(adj)) {
                threat = std::max(threat, memory[idx(adj)].army);
            }
        }
        return threat;
    }

    void refreshGeneralSafetyCache() {
        cachedGeneralAdjacentThreat = 0;
        cachedGeneralLocalPressure = 0.0;
        cachedRelaxedGeneralOpening = false;
        if (myGeneral == Coord{-1, -1}) return;

        cachedGeneralAdjacentThreat = adjacentEnemyThreat(myGeneral);
        cachedGeneralLocalPressure =
            std::max(0.0, enemyPressure[idx(myGeneral)] -
                              friendlyPressure[idx(myGeneral)] * 0.55);
        if (fullTurn >= 40 || cachedGeneralAdjacentThreat > 0 ||
            cachedGeneralLocalPressure > 4.0) {
            return;
        }

        int visibleEnemyTiles = 0;
        for (index_t player = 0; player < playerCnt; ++player) {
            if (player == id || isFriendlyOccupier(player)) continue;
            visibleEnemyTiles += visibleEnemyCountByPlayer[player];
        }
        cachedRelaxedGeneralOpening = visibleEnemyTiles == 0;
    }

    bool relaxedGeneralOpening() const {
        return cachedRelaxedGeneralOpening;
    }

    bool movePassesHardSafety(const Move& move) const {
        if (move.type != MoveType::MOVE_ARMY) return false;
        if (!inside(move.from) || !inside(move.to)) return false;
        const TileView& src = board.tileAt(move.from);
        if (src.occupier != id || src.army <= 1) return false;
        const army_t remain = moveRemainingArmy(move);
        if (remain <= 0) return false;
        if (move.from == myGeneral) {
            if (remain <= cachedGeneralAdjacentThreat) return false;
            if (!cachedRelaxedGeneralOpening &&
                cachedGeneralLocalPressure > 8.0 &&
                remain <= cachedGeneralAdjacentThreat + 1) {
                return false;
            }
        }
        return true;
    }

    double moveRiskPenalty(const Move& move,
                           const SituationMetrics& metrics) const {
        if (!movePassesHardSafety(move)) return 1e9;

        const army_t remain = moveRemainingArmy(move);
        const army_t reserve = reserveArmy[idx(move.from)];
        double penalty = 0.0;

        if (remain < reserve) {
            penalty +=
                (reserve - remain) *
                (move.from == myGeneral ? (cachedRelaxedGeneralOpening ? 4.0
                                                                       : 10.0)
                                        : 7.5);
        }

        const double localPressure =
            std::max(0.0, enemyPressure[idx(move.from)] -
                              friendlyPressure[idx(move.from)] * 0.60);
        if (coreZoneMask[idx(move.from)]) {
            penalty += localPressure * 0.35;
        }
        if (chokeMask[idx(move.from)]) {
            penalty += 5.0 + localPressure * 0.18;
        }
        if (move.from == myGeneral) {
            penalty +=
                std::max<army_t>(0, cachedGeneralAdjacentThreat + 1 - remain) *
                18.0;
            penalty += std::max(0.0, metrics.coreRisk - remain * 0.9) * 0.40;
        } else if (coreZoneMask[idx(move.from)] && remain <= 2) {
            penalty += 6.0;
        }
        if (board.tileAt(move.to).type == TILE_SWAMP) {
            penalty += 4.0;
        }
        return penalty;
    }

    bool moveLooksSafe(const Move& move) const {
        return movePassesHardSafety(move);
    }

    bool immediateAdvancePossible(Coord from, Coord to) const {
        if (!inside(from) || !inside(to)) return false;
        const TileView& src = board.tileAt(from);
        if (src.occupier != id || src.army <= 1) return false;

        const TileView& dst = board.tileAt(to);
        const TileMemory& mem = memory[idx(to)];
        const index_t occupier = dst.visible ? dst.occupier : mem.occupier;
        if (isFriendlyOccupier(occupier)) return true;

        const army_t attack = src.army - 1;
        const army_t defense = estimatedArmyAt(to);
        return attack > defense;
    }

    int friendlyLeadSteps(const std::vector<Coord>& route) const {
        int lead = 0;
        for (Coord step : route) {
            if (!inside(step)) break;
            if (board.tileAt(step).occupier != id) break;
            ++lead;
        }
        return lead;
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

    army_t supportAlongRoute(Coord source,
                             const std::vector<Coord>& route) const {
        army_t support = 0;
        std::array<Coord, 4> corridor{};
        const size_t corridorLen =
            std::min<size_t>(route.size(), corridor.size());
        for (size_t i = 0; i < corridorLen; ++i) corridor[i] = route[i];
        for (Coord friendly : friendlyTilesCache) {
            if (friendly == source) continue;
            const army_t available = availableArmy(friendly);
            if (available <= 0) continue;
            bool nearCorridor = false;
            for (size_t i = 0; i < corridorLen; ++i) {
                if (manhattan(friendly, corridor[i]) <= 1) {
                    nearCorridor = true;
                    break;
                }
            }
            if (!nearCorridor) continue;
            support += available;
        }
        return std::min<army_t>(support, 48);
    }

    army_t estimateObjectiveNeed(Coord objective, int distance,
                                 FocusMode mode) const {
        const TileMemory& mem = memory[idx(objective)];
        army_t need = estimatedArmyAt(objective) + 1;
        if (isEnemyOccupier(mem.occupier)) need += 1;
        if (mem.type == TILE_CITY) need += 3;
        if (mem.type == TILE_GENERAL) need += 2;
        if (mode == FocusMode::DEFEND) need += 1;
        if (mode == FocusMode::CONVERT && isEnemyOccupier(mem.occupier)) {
            need += 1;
        }
        need += static_cast<army_t>(std::max(1, distance / 4));
        need += static_cast<army_t>(std::max(
            0.0,
            (enemyPressure[idx(objective)] - friendlyPressure[idx(objective)]) *
                0.015));
        return need;
    }

    bool shouldPreferRally(const StrategicPlan& plan,
                           const SituationMetrics& metrics) const {
        if (!plan.valid || !plan.rally) return false;
        const double gap =
            static_cast<double>(plan.need) -
            static_cast<double>(plan.commitArmy + plan.corridorSupport);
        if (plan.mode == FocusMode::DEFEND) return gap > 0.0;
        return gap > 2.0 || plan.pathCost >= 8 || metrics.coreRisk > 12.0;
    }

    std::optional<Move> chooseRallyMove(const StrategicPlan& plan,
                                        const SituationMetrics& metrics) const {
        if (!plan.valid || plan.source == Coord{-1, -1}) return std::nullopt;

        CandidateMove best;
        std::array<Coord, 4> corridor{};
        size_t corridorLen = 0;
        for (size_t i = 0; i < plan.route.size() && corridorLen < corridor.size();
             ++i) {
            if (board.tileAt(plan.route[i]).occupier != id) continue;
            corridor[corridorLen++] = plan.route[i];
        }
        auto corridorDistance = [&](Coord c) {
            int bestDist = manhattan(c, plan.source);
            for (size_t i = 0; i < corridorLen; ++i) {
                bestDist = std::min(bestDist, manhattan(c, corridor[i]));
            }
            return bestDist;
        };

        for (Coord from : friendlyTilesCache) {
            if (from == plan.source) continue;
            const TileView& src = board.tileAt(from);
            if (availableArmy(from) <= 0) continue;
            const int before = corridorDistance(from);
            if (before <= 0) continue;

            for (Coord d : kDirs) {
                Coord to = from + d;
                if (!inside(to)) continue;
                const TileView& dst = board.tileAt(to);
                if (dst.occupier != id) continue;
                if (shouldBlockOscillation(from, to)) continue;

                const int after = corridorDistance(to);
                if (after >= before) continue;

                Move move(MoveType::MOVE_ARMY, from, to, false);
                const double riskPenalty = moveRiskPenalty(move, metrics);
                if (riskPenalty >= 1e8) continue;

                double score =
                    availableArmy(from) * 11.0 + (before - after) * 30.0;
                if (to == plan.source) score += 50.0;
                if (src.type == TILE_CITY) score -= 18.0;
                if (from == myGeneral) score -= 45.0 + metrics.coreRisk * 0.8;
                if (chokeMask[idx(from)]) score -= 14.0;
                score -= riskPenalty;
                if (score > best.score) {
                    best.valid = true;
                    best.score = score;
                    best.move = move;
                }
            }
        }

        if (!best.valid) return std::nullopt;
        return best.move;
    }

    FocusMode modeForObjective(const ObjectiveOption& objective,
                               const SituationMetrics& metrics) const {
        if (objective.defend) return FocusMode::DEFEND;
        if (objective.attack && objective.targetPlayer >= 0 &&
            metrics.conversionAdvantage > 6.0) {
            return FocusMode::CONVERT;
        }
        if (objective.attack) return FocusMode::ATTACK;
        return FocusMode::EXPAND;
    }

    StrategicPlan evaluateObjectivePlan(const ObjectiveOption& objective,
                                        const SituationMetrics& metrics) const {
        StrategicPlan best;
        if (!inside(objective.target)) return best;

        const FocusMode mode = modeForObjective(objective, metrics);
        const auto reversePath = weightedReversePath(
            objective.target,
            [&](Coord c) { return planningPenalty(c, mode, metrics); });
        if (!reversePath.reachable) return best;

        struct SourceSeed {
            Coord source{-1, -1};
            army_t available = 0;
            army_t reserve = 0;
            int dist = kInf;
            double score = -1e100;
        };

        std::vector<SourceSeed> seeds;
        seeds.reserve(friendlyTilesCache.size());
        for (Coord source : friendlyTilesCache) {
            const TileView& tile = board.tileAt(source);
            const army_t available = availableArmy(source);
            if (available <= 0) continue;
            const int dist = reversePath.distance(idx(source));
            if (dist >= kInf) continue;
            double score =
                available * 9.6 - dist * 3.3 - reserveArmy[idx(source)] * 0.8;
            if (source == myGeneral) {
                score -= relaxedGeneralOpening()
                             ? 0.2
                             : 1.5 + metrics.coreRisk * 0.08;
            }
            if (chokeMask[idx(source)]) score -= 6.0;
            if (objective.defend && distFromGeneral[idx(source)] < kInf) {
                score += std::max(0, 8 - distFromGeneral[idx(source)]) * 3.0;
            }
            if (objective.attack &&
                isEnemyOccupier(memory[idx(objective.target)].occupier)) {
                score += 18.0;
            }
            seeds.push_back(SourceSeed{source, available,
                                       reserveArmy[idx(source)], dist, score});
        }

        std::sort(seeds.begin(), seeds.end(),
                  [](const SourceSeed& lhs, const SourceSeed& rhs) {
                      return lhs.score > rhs.score;
                  });
        const int sourceLimit = std::clamp(
            4 + static_cast<int>(metrics.mobilityScore * 0.08), 4, 9);
        if (static_cast<int>(seeds.size()) > sourceLimit) {
            seeds.resize(sourceLimit);
        }

        for (const SourceSeed& seed : seeds) {
            std::vector<Coord> route = reconstructReversePath(
                seed.source, objective.target, reversePath);
            if (route.empty()) continue;
            const Coord firstStep = route.front();
            if (shouldBlockOscillation(seed.source, firstStep)) continue;

            StrategicPlan plan;
            plan.source = seed.source;
            plan.target = objective.target;
            plan.firstStep = firstStep;
            plan.route = route;
            plan.targetPlayer = objective.targetPlayer;
            plan.mode = mode;
            plan.sourceArmy = board.tileAt(seed.source).army;
            plan.commitArmy = seed.available;
            plan.reserve = seed.reserve;
            plan.corridorSupport = supportAlongRoute(seed.source, route);
            plan.need = estimateObjectiveNeed(
                objective.target, static_cast<int>(route.size()), mode);
            plan.objectiveScore = objective.score;
            plan.pathCost = seed.dist;
            plan.rally = plan.commitArmy + plan.corridorSupport < plan.need;
            const int friendlyLead = friendlyLeadSteps(route);

            double routeGain = 0.0;
            for (size_t i = 0; i < route.size() && i < 4; ++i) {
                const Coord step = route[i];
                const TileView& stepTile = board.tileAt(step);
                const TileMemory& stepMem = memory[idx(step)];
                if (isEnemyTile(stepTile) ||
                    isEnemyOccupier(stepMem.occupier)) {
                    routeGain += 22.0 - static_cast<double>(i) * 3.0;
                }
                if (!stepMem.everSeen || !stepTile.visible) {
                    routeGain += 12.0 - static_cast<double>(i) * 1.5;
                }
                if (stepTile.type == TILE_CITY || stepMem.type == TILE_CITY) {
                    routeGain += 24.0;
                }
            }

            double score = objective.score * 1.20 + seed.score;
            score += plan.corridorSupport * 2.8;
            score += routeGain;
            score -= seed.dist * 2.4;
            if (mode == FocusMode::EXPAND) {
                score -= friendlyLead * 26.0;
                if (friendlyLead == 0) {
                    score += 24.0;
                } else if (friendlyLead == 1) {
                    score += 6.0;
                }
            } else if (friendlyLead >= 2) {
                score -= friendlyLead * 8.0;
            }
            if (plan.commitArmy + plan.corridorSupport >= plan.need) {
                score += 96.0;
            } else {
                score -=
                    (plan.need - (plan.commitArmy + plan.corridorSupport)) *
                    0.8;
            }
            if (objective.attack) score += 28.0;
            if (objective.attack && (metrics.contactDensity > 0.045 ||
                                     metrics.conversionAdvantage > 8.0)) {
                score += 32.0;
            }
            if (objective.city) {
                score += 42.0;
                if (metrics.contactDensity < 0.08) score += 24.0;
                if (friendlyLead == 0) score += 18.0;
            }
            if (objective.exploration) score += 12.0;
            if (currentObjective == objective.target) score += 8.0;
            if (lockedObjective == objective.target &&
                fullTurn <= static_cast<turn_t>(objectiveLockUntil)) {
                score += 10.0;
            }
            if (seed.source == myGeneral && !objective.defend) {
                score -= 1.0 + metrics.coreRisk * 0.08;
            }

            Move directMove(MoveType::MOVE_ARMY, seed.source, firstStep, false);
            plan.move = directMove;
            plan.usingRally = false;
            plan.valid = true;
            const bool directFeasible =
                immediateAdvancePossible(seed.source, firstStep);

            if (!directFeasible || shouldPreferRally(plan, metrics)) {
                auto rallyMove = chooseRallyMove(plan, metrics);
                if (rallyMove.has_value()) {
                    plan.move = *rallyMove;
                    plan.usingRally = true;
                    score += 12.0;
                } else if (!directFeasible) {
                    continue;
                } else if (plan.commitArmy + plan.corridorSupport < plan.need) {
                    score -= 12.0;
                }
            } else {
                score += 10.0;
            }

            if (objective.city &&
                plan.commitArmy + plan.corridorSupport + 2 < plan.need &&
                !plan.usingRally) {
                continue;
            }
            const double riskPenalty = moveRiskPenalty(plan.move, metrics);
            if (riskPenalty >= 1e8) continue;
            score -= riskPenalty;

            plan.totalScore = score;
            if (!best.valid || plan.totalScore > best.totalScore) best = plan;
        }

        return best;
    }

    std::optional<ThreatInfo> analyzeThreat() const {
        if (myGeneral == Coord{-1, -1}) return std::nullopt;

        std::vector<Coord> keyTargets = {myGeneral};
        for (Coord city : ownedCitiesCache) {
            if (coreZoneMask[idx(city)]) keyTargets.push_back(city);
        }

        std::vector<ThreatSeed> allSeeds;
        std::vector<ThreatSeed> limitedSeeds;
        std::vector<ThreatSeed> positiveSeeds;
        allSeeds.reserve(keyTargets.size());
        limitedSeeds.reserve(keyTargets.size());
        positiveSeeds.reserve(keyTargets.size());

        for (size_t rank = 0; rank < keyTargets.size(); ++rank) {
            const Coord target = keyTargets[rank];
            const double targetPressure =
                enemyPressure[idx(target)] - friendlyPressure[idx(target)];
            ThreatSeed seed{
                targetPressure * 0.9 + (target == myGeneral ? 55.0 : 24.0),
                target,
                static_cast<int>(rank),
            };
            allSeeds.push_back(seed);
            if (targetPressure > 0.0) {
                positiveSeeds.push_back(seed);
            } else {
                limitedSeeds.push_back(seed);
            }
        }

        const ThreatPathResult closePath =
            multiSourceThreatReversePath(allSeeds, 4, closeThreatSearch);
        const ThreatPathResult limitedPath =
            multiSourceThreatReversePath(limitedSeeds, 5, limitedThreatSearch);
        const ThreatPathResult positivePath =
            multiSourceThreatReversePath(positiveSeeds, 14, positiveThreatSearch);

        std::optional<ThreatInfo> best;
        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord source{x, y};
                army_t sourceArmy = 0;
                bool valid = false;
                bool visibleSource = false;
                bool recentSource = false;
                const TileView& tile = board.tileAt(source);
                if (isEnemyTile(tile) && tile.army > 1) {
                    sourceArmy = tile.army;
                    valid = true;
                    visibleSource = true;
                } else if (recentEnemyAt(source, 2) &&
                           memory[idx(source)].army > 1) {
                    sourceArmy = memory[idx(source)].army;
                    valid = true;
                    recentSource = true;
                }
                if (!valid) continue;

                const ThreatPathResult* sourceBestPath = nullptr;
                const ThreatSeed* sourceBestSeed = nullptr;
                double sourceBestPressure = -1e100;
                int sourceBestRank = std::numeric_limits<int>::max();
                auto considerPath = [&](const ThreatPathResult& path,
                                        const std::vector<ThreatSeed>& seeds) {
                    if (!path.reachable) return;
                    const size_t node = idx(source);
                    const int seedIndex = path.seedAt(node);
                    if (seedIndex < 0 || seedIndex >= static_cast<int>(seeds.size())) {
                        return;
                    }
                    const int dist = path.distance(node);
                    if (dist >= kInf) return;
                    const int turns = path.stepCount(node);
                    if (turns >= kInf || turns <= 0) return;

                    const ThreatSeed& seed = seeds[seedIndex];
                    const double pressure =
                        sourceArmy * 2.2 - dist * 10.5 + seed.bias;

                    if (sourceBestPath == nullptr ||
                        pressure > sourceBestPressure + kThreatEpsilon ||
                        (std::abs(pressure - sourceBestPressure) <=
                             kThreatEpsilon &&
                         seed.rank < sourceBestRank)) {
                        sourceBestPath = &path;
                        sourceBestSeed = &seed;
                        sourceBestPressure = pressure;
                        sourceBestRank = seed.rank;
                    }
                };

                if (recentSource && !coreZoneMask[idx(source)]) {
                    considerPath(closePath, allSeeds);
                } else {
                    considerPath(limitedPath, limitedSeeds);
                    considerPath(positivePath, positiveSeeds);
                }

                if (sourceBestPath == nullptr || sourceBestSeed == nullptr) {
                    continue;
                }

                std::vector<Coord> route =
                    reconstructThreatRoute(source, *sourceBestPath);
                if (route.empty()) continue;

                ThreatInfo info;
                info.source = source;
                info.target = sourceBestSeed->target;
                info.route = std::move(route);
                info.intercept =
                    info.route[std::min<size_t>(1, info.route.size() - 1)];
                info.sourceArmy = sourceArmy;
                info.turns = sourceBestPath->stepCount(idx(source));
                info.visibleSource = visibleSource;
                info.recentSource = recentSource;
                info.pressure = sourceBestPressure;
                info.valid = true;

                if (!best.has_value() ||
                    info.pressure > best->pressure + kThreatEpsilon) {
                    best = std::move(info);
                }
            }
        }
        return best;
    }

    bool shouldDefend(const ThreatInfo& threat,
                      const SituationMetrics& metrics) const {
        if (!threat.valid) return false;
        const TileView& targetTile = board.tileAt(threat.target);
        const double targetPressure = enemyPressure[idx(threat.target)] -
                                      friendlyPressure[idx(threat.target)];
        if (threat.target == myGeneral) {
            if (threat.turns <= 3) return true;
            if (metrics.coreRisk > 8.0 && threat.turns <= 6) return true;
            if (targetPressure > 8.0 && threat.turns <= 5 &&
                threat.sourceArmy + 2 >= targetTile.army) {
                return true;
            }
            return false;
        }
        return metrics.frontierPressure > 8.0 && targetPressure > 5.0 &&
               threat.turns <= 4 && threat.pressure > 30.0;
    }

    std::optional<Move> chooseDefensiveMove(
        const SituationMetrics& metrics,
        const std::optional<ThreatInfo>& threat) {
        if (!threat.has_value() || !shouldDefend(*threat, metrics)) {
            return std::nullopt;
        }

        for (Coord d : kDirs) {
            Coord from = threat->intercept + d;
            if (!inside(from)) continue;
            const TileView& tile = board.tileAt(from);
            if (tile.occupier != id ||
                tile.army <= estimatedArmyAt(threat->intercept) + 1) {
                continue;
            }
            Move move(MoveType::MOVE_ARMY, from, threat->intercept, false);
            if (moveLooksSafe(move)) return move;
        }

        std::vector<ObjectiveOption> defenseObjectives;
        defenseObjectives.push_back(ObjectiveOption{240.0, threat->intercept,
                                                    currentTargetPlayer,
                                                    true, true, false, false});
        defenseObjectives.push_back(ObjectiveOption{210.0, threat->source,
                                                    currentTargetPlayer,
                                                    true, true, false, false});
        for (size_t i = 0; i < threat->route.size() && i < 3; ++i) {
            defenseObjectives.push_back(
                ObjectiveOption{190.0 - i * 14.0, threat->route[i],
                                currentTargetPlayer, true, true, false, false});
        }

        StrategicPlan best;
        for (const ObjectiveOption& objective : defenseObjectives) {
            StrategicPlan plan = evaluateObjectivePlan(objective, metrics);
            if (plan.valid &&
                (!best.valid || plan.totalScore > best.totalScore)) {
                best = std::move(plan);
            }
        }
        if (!best.valid) return std::nullopt;
        currentObjective = best.target;
        lockedObjective = best.target;
        objectiveLockUntil = static_cast<int>(fullTurn) + 6;
        return best.move;
    }

    CandidateMove chooseDirectCaptureMove(
        index_t targetPlayer, const SituationMetrics& metrics) const {
        CandidateMove best;
        for (Coord from : friendlyTilesCache) {
            const TileView& src = board.tileAt(from);
            if (src.army <= 1) continue;
            for (Coord d : kDirs) {
                Coord to = from + d;
                if (!inside(to)) continue;
                const TileView& dst = board.tileAt(to);
                if (isImpassableTile(dst.type)) continue;
                if (isFriendlyTile(dst)) continue;
                if (shouldBlockOscillation(from, to)) continue;

                const army_t attack = src.army - 1;
                const army_t defense = estimatedArmyAt(to);
                if (attack <= defense) continue;

                Move move(MoveType::MOVE_ARMY, from, to, false);
                const double riskPenalty = moveRiskPenalty(move, metrics);
                if (riskPenalty >= 1e8) continue;

                const army_t margin = attack - defense;
                const bool enemyTile = isEnemyTile(dst);
                const bool relievesCore =
                    coreZoneMask[idx(to)] && (enemyTile || recentEnemyAt(to) ||
                                              estimatedArmyAt(to) >= 2);
                const bool tactical =
                    dst.type == TILE_GENERAL || dst.type == TILE_CITY ||
                    (enemyTile && margin >= 3) || relievesCore;
                double score = 0.0;
                if (dst.type == TILE_GENERAL) score += 1e6;
                if (dst.type == TILE_CITY) score += 290.0 + margin * 8.5;
                if (enemyTile) score += 105.0 + margin * 5.5;
                if (dst.occupier == targetPlayer) score += 45.0;
                if (currentObjective == to) score += tactical ? 18.0 : 6.0;
                if (relievesCore) score += 45.0;
                if (dst.type == TILE_SWAMP) score -= 20.0;
                if (!dst.visible && !memory[idx(to)].everSeen) {
                    score += tactical ? 6.0 : -8.0;
                }
                if (!enemyTile && dst.type != TILE_CITY) score -= 26.0;
                score -= riskPenalty;

                if (score > best.score) {
                    best.valid = true;
                    best.score = score;
                    best.riskPenalty = riskPenalty;
                    best.move = move;
                    best.source = from;
                    best.target = to;
                    best.mode =
                        enemyTile ? FocusMode::ATTACK : FocusMode::EXPAND;
                    best.tactical = tactical;
                    best.relievesCore = relievesCore;
                }
            }
        }
        return best;
    }

    std::vector<ObjectiveOption> buildObjectiveCandidates(
        index_t targetPlayer, const SituationMetrics& metrics,
        const std::optional<ThreatInfo>& threat) {
        std::vector<ObjectiveOption> defenseOptions;
        std::vector<ObjectiveOption> assaultOptions;
        std::vector<ObjectiveOption> cityOptions;
        std::vector<ObjectiveOption> pressureOptions;
        std::vector<ObjectiveOption> expandOptions;

        auto addObjective = [&](const ObjectiveOption& option) {
            if (!inside(option.target)) return;
            if (!isPassable(memory[idx(option.target)].type)) return;
            if (board.tileAt(option.target).occupier == id) return;
            if (option.defend) {
                defenseOptions.push_back(option);
            } else if (option.city) {
                cityOptions.push_back(option);
            } else if (option.attack) {
                assaultOptions.push_back(option);
            } else if (option.exploration) {
                expandOptions.push_back(option);
            } else {
                pressureOptions.push_back(option);
            }
        };

        if (threat.has_value()) {
            addObjective(ObjectiveOption{250.0, threat->intercept, targetPlayer,
                                         true, true, false, false});
            addObjective(ObjectiveOption{220.0, threat->source, targetPlayer,
                                         true, true, false, false});
        }

        Coord targetGuess = chooseTargetPlayerGeneral(targetPlayer);
        const int enemyContactSignal =
            targetPlayer >= 0 ? visibleEnemyCountByPlayer[targetPlayer] +
                                    newlyVisibleEnemyTiles[targetPlayer]
                              : 0;
        const bool shouldPressureTarget = enemyContactSignal > 0 ||
                                          metrics.contactDensity > 0.045 ||
                                          metrics.conversionAdvantage > 8.0;
        const bool canChaseGuess =
            targetPlayer >= 0 &&
            targetPlayer < static_cast<index_t>(cachedGeneralHasEvidence.size()) &&
            cachedGeneralHasEvidence[targetPlayer];
        if (targetGuess != Coord{-1, -1} && canChaseGuess) {
            addObjective(ObjectiveOption{230.0, targetGuess, targetPlayer, true,
                                         false, false, false});
            for (Coord d : kDirs) {
                Coord adj = targetGuess + d;
                addObjective(ObjectiveOption{180.0, adj, targetPlayer, true,
                                             false, false, false});
            }
        }

        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                const TileView& tile = board.tileAt(c);
                const TileMemory& mem = memory[idx(c)];
                if (!isPassable(mem.type) || tile.occupier == id) continue;

                const index_t occupier =
                    tile.visible ? tile.occupier : mem.occupier;
                const bool enemy = isEnemyOccupier(occupier);
                const bool city =
                    mem.type == TILE_CITY || tile.type == TILE_CITY;
                const bool exploration = !mem.everSeen || !tile.visible;
                double score = 0.0;
                if (enemy) {
                    score +=
                        62.0 + std::min<army_t>(estimatedArmyAt(c), 80) * 0.45;
                    if (shouldPressureTarget) score += 24.0;
                } else {
                    score += 22.0;
                }
                if (occupier == targetPlayer) {
                    score += shouldPressureTarget ? 92.0 : 50.0;
                }
                if (city) {
                    score += 236.0 - estimatedArmyAt(c) * 2.8;
                    if (metrics.contactDensity < 0.08) score += 32.0;
                    if (metrics.conversionAdvantage < 6.0) {
                        score +=
                            std::max(0.0, 12.0 - metrics.conversionAdvantage);
                    }
                    if (enemy) score += shouldPressureTarget ? 32.0 : 12.0;
                }
                if (!mem.everSeen) {
                    score += 86.0 - metrics.contactDensity * 12.0 +
                             std::max(0.0, -metrics.conversionAdvantage * 1.2);
                    if (shouldPressureTarget) score -= 22.0;
                }
                if (!tile.visible) score += 20.0;
                if (mem.type == TILE_SWAMP) score -= 42.0;
                if (currentObjective == c) score += 10.0;
                if (lockedObjective == c &&
                    fullTurn <= static_cast<turn_t>(objectiveLockUntil)) {
                    score += 12.0;
                }
                if (targetGuess != Coord{-1, -1} && canChaseGuess) {
                    score +=
                        std::max(0.0, 52.0 - manhattan(c, targetGuess) * 3.2);
                }
                if (myGeneral != Coord{-1, -1}) {
                    score -= manhattan(c, myGeneral) *
                             (metrics.coreRisk > 12.0 ? 0.85 : 0.35);
                }
                const double netPressure =
                    enemyPressure[idx(c)] - friendlyPressure[idx(c)];
                if (!enemy && netPressure > 0.0) score -= netPressure * 0.25;
                if (enemy && metrics.conversionAdvantage > 0.0) {
                    score += std::min(30.0, metrics.conversionAdvantage * 2.5);
                } else if (enemy && metrics.conversionAdvantage < 0.0) {
                    score -= std::min(42.0, -metrics.conversionAdvantage * 2.6);
                }
                if (!enemy && shouldPressureTarget && !city && !exploration) {
                    score -= 26.0;
                }
                if (enemy && !city && !exploration &&
                    metrics.contactDensity < 0.08 &&
                    metrics.conversionAdvantage < 4.0) {
                    score -= 48.0;
                }
                if (score <= -1e90) continue;

                ObjectiveOption option{
                    score,      c,
                    occupier >= 0 ? occupier : targetPlayer, enemy,
                    false,      city,
                    exploration};
                const bool nearTargetGuess = targetGuess != Coord{-1, -1} &&
                                             canChaseGuess &&
                                             manhattan(c, targetGuess) <= 4;
                if (enemy && (occupier == targetPlayer || nearTargetGuess)) {
                    option.score += 18.0;
                    assaultOptions.push_back(option);
                } else if (city) {
                    cityOptions.push_back(option);
                } else if (enemy) {
                    pressureOptions.push_back(option);
                } else if (exploration) {
                    expandOptions.push_back(option);
                } else {
                    pressureOptions.push_back(option);
                }
            }
        }

        auto sortByScore = [](std::vector<ObjectiveOption>& bucket) {
            std::sort(
                bucket.begin(), bucket.end(),
                [](const ObjectiveOption& lhs, const ObjectiveOption& rhs) {
                    return lhs.score > rhs.score;
                });
        };
        sortByScore(defenseOptions);
        sortByScore(assaultOptions);
        sortByScore(cityOptions);
        sortByScore(pressureOptions);
        sortByScore(expandOptions);

        std::vector<ObjectiveOption> unique;
        std::vector<uint8_t> seen(static_cast<size_t>((height + 2) * W), 0);
        const int objectiveLimit =
            std::clamp(8 + static_cast<int>(metrics.mobilityScore * 0.12 -
                                            metrics.coreRisk * 0.04),
                       8, 16);
        auto appendBucket = [&](const std::vector<ObjectiveOption>& bucket,
                                int limit) {
            int taken = 0;
            for (const auto& option : bucket) {
                if (taken >= limit ||
                    static_cast<int>(unique.size()) >= objectiveLimit) {
                    break;
                }
                const size_t node = idx(option.target);
                if (seen[node]) continue;
                seen[node] = 1;
                unique.push_back(option);
                ++taken;
            }
        };

        appendBucket(defenseOptions, 4);
        appendBucket(assaultOptions, 5);
        appendBucket(cityOptions, 4);
        appendBucket(pressureOptions, 4);
        appendBucket(expandOptions, 5);

        objectiveCandidates = unique;
        return objectiveCandidates;
    }

    CandidateMove chooseBestStrategicMove(
        const std::vector<ObjectiveOption>& objectives,
        const SituationMetrics& metrics) {
        CandidateMove best;
        StrategicPlan bestPlan;

        for (const ObjectiveOption& objective : objectives) {
            StrategicPlan plan = evaluateObjectivePlan(objective, metrics);
            if (!plan.valid) continue;
            if (!best.valid || plan.totalScore > best.score) {
                best.valid = true;
                best.score = plan.totalScore;
                best.move = plan.move;
                best.source = plan.source;
                best.target = plan.target;
                best.mode = plan.mode;
                bestPlan = std::move(plan);
            }
        }

        if (!best.valid) return best;

        currentObjective = bestPlan.target;
        if (bestPlan.targetPlayer >= 0)
            currentTargetPlayer = bestPlan.targetPlayer;
        if (bestPlan.mode != FocusMode::EXPAND ||
            bestPlan.objectiveScore >= 170.0) {
            lockedObjective = bestPlan.target;
            objectiveLockUntil = static_cast<int>(fullTurn) + 12;
        }
        best.riskPenalty = moveRiskPenalty(best.move, metrics);
        best.tactical = bestPlan.mode != FocusMode::EXPAND ||
                        bestPlan.objectiveScore >= 180.0;
        best.relievesCore = bestPlan.mode == FocusMode::DEFEND;
        return best;
    }

    CandidateMove chooseEconomicPlannerMove(index_t targetPlayer,
                                            const SituationMetrics& metrics) {
        CandidateMove best;
        const Coord targetGuess = chooseTargetPlayerGeneral(targetPlayer);
        struct SourceSeed {
            Coord source{-1, -1};
            double score = -1e100;
        };
        std::vector<SourceSeed> seeds;
        for (Coord source : friendlyTilesCache) {
            const army_t available = availableArmy(source);
            if (available <= 0) continue;
            double score = available * 6.8 - reserveArmy[idx(source)] * 1.0;
            if (source == myGeneral)
                score -= relaxedGeneralOpening() ? 0.5 : 2.5;
            if (chokeMask[idx(source)]) score -= 6.0;
            if (currentObjective != Coord{-1, -1}) {
                score -= manhattan(source, currentObjective) * 0.7;
            }
            seeds.push_back(SourceSeed{source, score});
        }
        std::sort(seeds.begin(), seeds.end(),
                  [](const SourceSeed& lhs, const SourceSeed& rhs) {
                      return lhs.score > rhs.score;
                  });
        if (seeds.size() > 5) seeds.resize(5);

        for (const SourceSeed& seed : seeds) {
            const Coord source = seed.source;
            const auto path = weightedPath(source, [&](Coord c) {
                int cost = attackPenalty(c, true);
                if (memory[idx(c)].type == TILE_SWAMP) cost += 18;
                const double netPressure =
                    enemyPressure[idx(c)] - friendlyPressure[idx(c)];
                if (netPressure > 6.0) {
                    cost += static_cast<int>(netPressure * 0.05);
                }
                return std::max(1, cost);
            });

            for (pos_t x = 1; x <= height; ++x) {
                for (pos_t y = 1; y <= width; ++y) {
                    Coord target{x, y};
                    const TileView& tile = board.tileAt(target);
                    const TileMemory& mem = memory[idx(target)];
                    if (!isPassable(mem.type) || tile.occupier == id) continue;
                    const int dist = path.distance(idx(target));
                    if (dist >= kInf) continue;

                    const index_t occupier =
                        tile.visible ? tile.occupier : mem.occupier;
                    const bool enemy = isEnemyOccupier(occupier);
                    const bool city =
                        tile.type == TILE_CITY || mem.type == TILE_CITY;
                    const bool exploration = !mem.everSeen || !tile.visible;
                    double score = seed.score - dist * 6.4;
                    if (exploration) {
                        score += 80.0 - metrics.contactDensity * 12.0 +
                                 std::max(0.0, -metrics.conversionAdvantage);
                    }
                    if (!tile.visible) score += 22.0;
                    if (enemy) score += 28.0 + estimatedArmyAt(target) * 0.10;
                    if (occupier == targetPlayer) score += 18.0;
                    if (city) {
                        score += 224.0 - estimatedArmyAt(target) * 2.5;
                        if (metrics.contactDensity < 0.08) score += 36.0;
                        if (metrics.conversionAdvantage < 6.0) {
                            score += std::max(
                                0.0, 10.0 - metrics.conversionAdvantage);
                        }
                    }
                    if (mem.type == TILE_SWAMP) score -= 45.0;
                    if (currentObjective == target) score += 8.0;
                    if (targetGuess != Coord{-1, -1}) {
                        score += std::max(
                            0.0, 32.0 - manhattan(target, targetGuess) * 2.0);
                    }
                    if (myGeneral != Coord{-1, -1}) {
                        score -= manhattan(target, myGeneral) *
                                 (metrics.coreRisk > 10.0 ? 0.60 : 0.20);
                    }
                    const double netPressure = enemyPressure[idx(target)] -
                                               friendlyPressure[idx(target)];
                    if (!enemy && netPressure > 0.0)
                        score -= netPressure * 0.18;
                    if (!city && !enemy && !exploration) score -= 28.0;
                    if (enemy && !city && !exploration) score -= 24.0;
                    if (enemy && !city && metrics.contactDensity < 0.08 &&
                        metrics.conversionAdvantage < 4.0) {
                        score -= 38.0;
                    }

                    const Coord firstStep = path.firstStepAt(idx(target));
                    if (firstStep == Coord{-1, -1}) continue;
                    const int lead = path.friendlyLeadAt(idx(target));
                    score -= lead * 20.0;
                    if (lead == 0) score += 18.0;
                    if (lead > 2 && !city) score -= 20.0;
                    if (!immediateAdvancePossible(source, firstStep)) continue;
                    Move move(MoveType::MOVE_ARMY, source, firstStep, false);
                    const double riskPenalty = moveRiskPenalty(move, metrics);
                    if (riskPenalty >= 1e8) continue;
                    score -= riskPenalty;

                    if (score > best.score) {
                        best.valid = true;
                        best.score = score;
                        best.riskPenalty = riskPenalty;
                        best.move = move;
                        best.source = source;
                        best.target = target;
                        best.mode =
                            enemy ? FocusMode::ATTACK : FocusMode::EXPAND;
                        best.tactical = city || enemy;
                        best.relievesCore = false;
                    }
                }
            }
        }

        return best;
    }

    std::optional<Move> fallbackMove(index_t targetPlayer,
                                     const SituationMetrics& metrics) const {
        Coord source = strongestFriendlyTile(true, currentObjective, 1);
        if (source == Coord{-1, -1}) return std::nullopt;

        Coord bestTarget{-1, -1};
        double bestScore = -1e100;
        for (Coord d : kDirs) {
            Coord to = source + d;
            if (!inside(to)) continue;
            const TileView& tile = board.tileAt(to);
            if (isImpassableTile(tile.type) ||
                shouldBlockOscillation(source, to)) {
                continue;
            }
            Move move(MoveType::MOVE_ARMY, source, to, false);
            const double riskPenalty = moveRiskPenalty(move, metrics);
            if (riskPenalty >= 1e8) continue;

            double score = 0.0;
            const TileMemory& mem = memory[idx(to)];
            const index_t occupier =
                tile.visible ? tile.occupier : mem.occupier;
            if (!mem.everSeen) score += 42.0;
            if (!tile.visible) score += 18.0;
            if (tile.type == TILE_CITY)
                score += 75.0 - estimatedArmyAt(to) * 2.5;
            if (tile.type == TILE_SWAMP) score -= 18.0;
            if (occupier != id) score += 24.0;
            if (occupier == targetPlayer) score += 18.0;
            if (currentObjective == to) score += 12.0;
            score -= riskPenalty;
            if (score > bestScore) {
                bestScore = score;
                bestTarget = to;
            }
        }

        if (bestTarget == Coord{-1, -1}) return std::nullopt;
        Move move(MoveType::MOVE_ARMY, source, bestTarget, false);
        if (!movePassesHardSafety(move)) return std::nullopt;
        return move;
    }

    std::optional<Move> selectStrategicMove(const SituationMetrics& metrics) {
        const index_t targetPlayer = chooseLockedTargetPlayer(metrics);
        const std::optional<ThreatInfo> threat = analyzeThreat();

        CandidateMove direct = chooseDirectCaptureMove(targetPlayer, metrics);
        if (direct.valid && direct.score > 600000.0) {
            traceDecision("forced-direct", direct, metrics);
            return direct.move;
        }

        if (auto defense = chooseDefensiveMove(metrics, threat)) return defense;

        if (direct.valid && direct.tactical &&
            (direct.score > 300.0 || direct.relievesCore) &&
            metrics.coreRisk <= 18.0) {
            traceDecision("direct", direct, metrics);
            return direct.move;
        }

        auto objectives =
            buildObjectiveCandidates(targetPlayer, metrics, threat);

        if (lockedObjective != Coord{-1, -1} &&
            fullTurn <= static_cast<turn_t>(objectiveLockUntil) &&
            shouldPersistObjective(lockedObjective)) {
            bool alreadyPresent =
                std::any_of(objectives.begin(), objectives.end(),
                            [&](const ObjectiveOption& option) {
                                return option.target == lockedObjective;
                            });
            if (!alreadyPresent) {
                objectives.insert(
                    objectives.begin(),
                    ObjectiveOption{
                        220.0, lockedObjective, targetPlayer, true, false,
                        board.tileAt(lockedObjective).type == TILE_CITY,
                        false});
            }
        }

        CandidateMove planned = chooseBestStrategicMove(objectives, metrics);
        CandidateMove economic =
            chooseEconomicPlannerMove(targetPlayer, metrics);
        if (planned.valid &&
            (!economic.valid || planned.score + 18.0 >= economic.score ||
             planned.mode == FocusMode::ATTACK ||
             planned.mode == FocusMode::CONVERT)) {
            traceCompetition(direct, planned, economic, "planned");
            traceDecision("planned", planned, metrics);
            return planned.move;
        }

        if (economic.valid) {
            traceCompetition(direct, planned, economic, "economic");
            traceDecision("economic", economic, metrics);
            currentObjective = economic.target;
            return economic.move;
        }

        if (direct.valid && direct.tactical &&
            movePassesHardSafety(direct.move)) {
            traceCompetition(direct, planned, economic, "late-direct");
            traceDecision("late-direct", direct, metrics);
            return direct.move;
        }

        return fallbackMove(targetPlayer, metrics);
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
        predictedGeneralScale.assign(playerCnt, 1.0);
        baseGeneralCandidateMask.assign(total, 0);
        baseGeneralCandidateList.clear();
        baseGeneralCandidateList.reserve(height * width);
        newlyVisibleEnemyTiles.assign(playerCnt, 0);
        cachedGeneralGuess.assign(playerCnt, Coord{-1, -1});
        cachedGeneralHasEvidence.assign(playerCnt, 0);
        rankById.assign(playerCnt, RankItem{});
        prevRankById.clear();
        aliveById.assign(playerCnt, true);
        playerArmyDelta.assign(playerCnt, 0);
        playerLandDelta.assign(playerCnt, 0);
        visibleEnemyCountByPlayer.assign(playerCnt, 0);
        enemyPressure.assign(total, 0.0);
        friendlyPressure.assign(total, 0.0);
        reserveArmy.assign(total, 0);
        distFromGeneral.assign(total, kInf);
        pathFlow.assign(total, 0);
        coreZoneMask.assign(total, 0);
        chokeMask.assign(total, 0);
        generalPathParent.assign(total, Coord{-1, -1});
        generalBfsOrder.clear();

        friendlyTilesCache.clear();
        frontierTiles.clear();
        ownedCitiesCache.clear();
        objectiveCandidates.clear();
        largestFriendlyArmyCache = 0;
        cachedGeneralAdjacentThreat = 0;
        cachedGeneralLocalPressure = 0.0;
        cachedRelaxedGeneralOpening = false;
        myGeneral = Coord{-1, -1};
        currentObjective = Coord{-1, -1};
        lockedObjective = Coord{-1, -1};
        objectiveLockUntil = -1;
        currentTargetPlayer = -1;
        targetLockUntil = -1;
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
        myGeneral = Coord{-1, -1};
        rememberVisibleBoard();
        rebuildTurnCaches();
        buildBaseGeneralCandidateMask();
        recomputeSituationMaps();
        const SituationMetrics metrics = computeSituationMetrics();
        refreshGeneralGuessCache(metrics);

        std::optional<Move> candidate = selectStrategicMove(metrics);
        if (!candidate.has_value() || !moveLooksSafe(*candidate)) {
            candidate = fallbackMove(currentTargetPlayer, metrics);
        }
        if (!candidate.has_value()) return;

        pushRecentMove(candidate->from, candidate->to);
        moveQueue.push_back(*candidate);
    }
};

static BotRegistrar<KutuBot> kutu_bot_reg("KutuBot");

#endif  // LGEN_BOTS_KUTUBOT
