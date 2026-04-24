// Copyright (C) 2026 oimasterkafuu and contributors
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file OimBot.cpp
 *
 * Rebuilt strategic oimBot for LocalGen v6.
 *
 * @author oimasterkafuu and contributors
 */

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

class OimBot : public BasicBot {
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
            if (rawDist == nullptr || stamp == nullptr ||
                node >= rawDist->size()) {
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
    std::vector<army_t> visibleArmyByPlayer;
    std::vector<army_t> hiddenReserveEstimate;
    std::vector<army_t> hiddenReserveDelta;
    std::vector<double> duelExpansionHeat;
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
    Coord openingTarget{-1, -1};
    int openingTargetUntil = -1;
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

    inline int mapArea() const { return static_cast<int>(height * width); }

    bool duelMode() const {
        int enemyCount = 0;
        for (index_t player = 0; player < playerCnt; ++player) {
            if (player == id || !aliveById[player] || isFriendlyOccupier(player)) {
                continue;
            }
            ++enemyCount;
        }
        return enemyCount <= 1;
    }

    double expansionUrgency(const SituationMetrics& metrics) const {
        const bool duel = duelMode();
        double urgency = 0.0;
        const double areaFactor =
            std::max(0.0, static_cast<double>(mapArea()) - 380.0) / 50.0;
        urgency += std::min(8.0, areaFactor);
        if (!duel) urgency += 1.5;
        if (playerCnt > 2) urgency += 1.5;
        if (mapArea() >= 500 && fullTurn < 140) {
            urgency += (140.0 - static_cast<double>(fullTurn)) * 0.035;
        }
        if (!duel || mapArea() >= 520) {
            urgency += std::max(0.0, 0.055 - metrics.contactDensity) * 90.0;
            urgency += std::max(0.0, 4.0 - metrics.conversionAdvantage) * 0.35;
        }
        urgency -= std::max(0.0, metrics.coreRisk - 8.0) * 0.18;
        return std::clamp(urgency, 0.0, 12.0);
    }

    double intelWeight() const { return duelMode() ? 1.0 : 0.0; }

    bool upcomingMoveGetsGrowthTick() const { return (halfTurn & 1) == 1; }

    bool upcomingMoveGetsGlobalGrowthTick() const {
        return upcomingMoveGetsGrowthTick() && fullTurn > 1 &&
               ((fullTurn - 1) % 25 == 0);
    }

    int turnsUntilNextGlobalGrowth() const {
        const int nextGrowthTurn =
            std::max(0, static_cast<int>(fullTurn) - 1 +
                            (upcomingMoveGetsGrowthTick() ? 0 : 1));
        if (nextGrowthTurn > 0 && nextGrowthTurn % 25 == 0) return 0;
        const int remainder = nextGrowthTurn % 25;
        return remainder == 0 ? 25 : 25 - remainder;
    }

    bool inDuelTempoPhase(const SituationMetrics& metrics) const {
        if (!duelMode()) return false;
        if (mapArea() > 450) return false;
        if (metrics.contactDensity > (mapArea() >= 520 ? 0.050 : 0.065)) {
            return false;
        }
        if (metrics.coreRisk > 10.0) return false;
        const int limit =
            mapArea() >= 650 ? 130 : (mapArea() >= 500 ? 105 : 64);
        return fullTurn <= limit;
    }

    bool shouldBuildDuelExpansionHeat(const SituationMetrics& metrics) const {
        if (!duelMode()) return false;
        if (metrics.contactDensity > 0.060) return false;
        if (metrics.coreRisk > 10.5) return false;
        const int limit =
            mapArea() >= 650 ? 135 : (mapArea() >= 500 ? 115 : 70);
        return fullTurn <= limit;
    }

    double duelExpansionPlanningWeight() const {
        return duelMode() && mapArea() <= 450 ? 1.0 : 0.0;
    }

    index_t visibleOrRememberedOccupier(Coord c) const {
        if (!inside(c)) return -1;
        const TileView& tile = board.tileAt(c);
        return tile.visible ? tile.occupier : memory[idx(c)].occupier;
    }

    double captureTimingBonus(Coord from, Coord to) const {
        if (!inside(from) || !inside(to) || !upcomingMoveGetsGrowthTick()) {
            return 0.0;
        }
        const TileView& src = board.tileAt(from);
        const TileView& dst = board.tileAt(to);
        const TileMemory& mem = memory[idx(to)];
        if (src.occupier != id || src.army <= 1) return 0.0;
        const index_t occupier = dst.visible ? dst.occupier : mem.occupier;
        if (isFriendlyOccupier(occupier)) return 0.0;
        if (src.army - 1 <= estimatedArmyAt(to)) return 0.0;

        double bonus = 4.0;
        if (isEnemyOccupier(occupier)) bonus += 10.0;
        if (dst.type == TILE_CITY || mem.type == TILE_CITY) bonus += 12.0;
        if (dst.type == TILE_GENERAL || mem.type == TILE_GENERAL) bonus += 18.0;
        if (!mem.everSeen || !dst.visible) bonus += 2.0;
        if (upcomingMoveGetsGlobalGrowthTick()) {
            bonus += 7.0;
            if (isEnemyOccupier(occupier)) bonus += 8.0;
            if (dst.type != TILE_CITY && mem.type != TILE_CITY &&
                dst.type != TILE_GENERAL && mem.type != TILE_GENERAL) {
                bonus += 2.5;
            }
            bonus +=
                std::min(8.0, std::max(0.0, localExpansionPotential(to)) * 0.75);
        }
        return bonus;
    }

    double localExpansionPotential(Coord c) const {
        if (!inside(c)) return 0.0;
        double score = 0.0;
        for (Coord d : kDirs) {
            Coord adj = c + d;
            if (!inside(adj)) continue;
            const TileView& tile = board.tileAt(adj);
            const TileMemory& mem = memory[idx(adj)];
            if (!isPassable(mem.type)) continue;
            if (isFriendlyTile(tile)) {
                score -= 0.8;
                continue;
            }
            if (!mem.everSeen) {
                score += 8.0;
            } else if (!tile.visible) {
                score += 4.0;
            } else if (tile.occupier < 0) {
                score += 2.5;
            } else if (isEnemyTile(tile)) {
                score += 1.5;
            }
            if (tile.type == TILE_SWAMP || mem.type == TILE_SWAMP) score -= 1.2;
        }
        for (Coord d : kDirs) {
            Coord mid = c + d;
            if (!inside(mid)) continue;
            const TileMemory& midMem = memory[idx(mid)];
            if (!isPassable(midMem.type)) continue;
            for (Coord d2 : kDirs) {
                Coord far = mid + d2;
                if (!inside(far) || far == c) continue;
                const TileView& farTile = board.tileAt(far);
                const TileMemory& farMem = memory[idx(far)];
                if (!isPassable(farMem.type) || isFriendlyTile(farTile)) continue;
                if (!farMem.everSeen) score += 1.2;
                else if (!farTile.visible) score += 0.7;
            }
        }
        return score;
    }

    double duelExpansionHeatAt(Coord c) const {
        if (!inside(c) || duelExpansionHeat.empty()) return 0.0;
        return duelExpansionHeat[idx(c)];
    }

    bool shouldAvoidOpeningSwamp(Coord c) const {
        if (!inside(c)) return false;
        const tile_type_e type =
            board.tileAt(c).visible ? board.tileAt(c).type : memory[idx(c)].type;
        return fullTurn <= 30 && type == TILE_SWAMP;
    }

    bool inOpeningPhase(const SituationMetrics& metrics) const {
        if (!duelMode()) return false;
        if (mapArea() > 450) return false;
        const int limit =
            duelMode() ? (mapArea() >= 500 ? 26 : 20)
                       : (mapArea() >= 500 ? 22 : 16);
        if (fullTurn > limit) return false;
        if (metrics.contactDensity > 0.06) return false;
        if (metrics.coreRisk > 9.0) return false;
        return true;
    }

    std::optional<Move> chooseOpeningMove(index_t targetPlayer,
                                          const SituationMetrics& metrics) {
        if (!inOpeningPhase(metrics) || myGeneral == Coord{-1, -1}) {
            return std::nullopt;
        }

        Coord source = myGeneral;
        if (board.tileAt(source).army <= 1) {
            source = strongestFriendlyTile(true, std::nullopt, 0);
        }
        if (source == Coord{-1, -1} || board.tileAt(source).army <= 1) {
            return std::nullopt;
        }

        const Coord enemyGuess = chooseTargetPlayerGeneral(targetPlayer);
        const bool canChaseGuess =
            targetPlayer >= 0 &&
            targetPlayer < static_cast<index_t>(cachedGeneralHasEvidence.size()) &&
            cachedGeneralHasEvidence[targetPlayer];
        const int reach =
            std::clamp(4 + static_cast<int>(board.tileAt(source).army / 8) +
                           (mapArea() >= 500 ? 1 : 0),
                       4, 7);
        const auto path = weightedPath(source, [&](Coord c) {
            int cost = 2;
            const TileMemory& mem = memory[idx(c)];
            const TileView& tile = board.tileAt(c);
            if (!mem.everSeen) cost -= 1;
            if (!tile.visible) cost -= 1;
            if (shouldAvoidOpeningSwamp(c)) cost += 24;
            if (mem.type == TILE_CITY || tile.type == TILE_CITY) {
                cost += std::max<int>(4, static_cast<int>(estimatedArmyAt(c) / 3));
            }
            if (isEnemyTile(tile))
                cost += std::max<int>(0, static_cast<int>(estimatedArmyAt(c) / 4));
            return std::max(1, cost);
        });

        Coord bestTarget{-1, -1};
        double bestScore = -1e100;
        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord target{x, y};
                const int dist = path.distance(idx(target));
                if (dist >= kInf || dist > reach) continue;
                const TileView& tile = board.tileAt(target);
                const TileMemory& mem = memory[idx(target)];
                if (!isPassable(mem.type) || tile.occupier == id) continue;
                if (shouldAvoidOpeningSwamp(target)) continue;

                const bool enemy = isEnemyTile(tile);
                const bool city = tile.type == TILE_CITY || mem.type == TILE_CITY;
                double score = 0.0;
                if (!mem.everSeen) score += 120.0;
                if (!tile.visible) score += 36.0;
                if (tile.occupier < 0 && !city) score += 18.0;
                score += localExpansionPotential(target) * 8.5;
                score += captureTimingBonus(source, path.firstStepAt(idx(target)));
                score -= dist * 9.0;

                const int centerDist = std::abs(target.x * 2 - (height + 1)) +
                                       std::abs(target.y * 2 - (width + 1));
                score += std::max(0.0, 18.0 - centerDist * 0.45);

                if (city) {
                    const army_t need = estimatedArmyAt(target) + 2;
                    if (board.tileAt(source).army - 1 < need) continue;
                    score += 220.0 - estimatedArmyAt(target) * 5.5;
                }
                if (enemy) {
                    if (board.tileAt(source).army - 1 <= estimatedArmyAt(target)) continue;
                    score += 46.0 + estimatedArmyAt(target) * 0.8;
                }
                if (target == openingTarget &&
                    fullTurn <= static_cast<turn_t>(openingTargetUntil)) {
                    score += 28.0;
                }
                if (enemyGuess != Coord{-1, -1} && canChaseGuess && fullTurn >= 8) {
                    score += std::max(0.0, 28.0 - manhattan(target, enemyGuess) * 1.6);
                }
                if (score > bestScore) {
                    bestScore = score;
                    bestTarget = target;
                }
            }
        }

        if (bestTarget == Coord{-1, -1}) return std::nullopt;
        const Coord firstStep = path.firstStepAt(idx(bestTarget));
        if (firstStep == Coord{-1, -1}) return std::nullopt;
        Move move(MoveType::MOVE_ARMY, source, firstStep, false);
        if (!moveLooksSafe(move)) return std::nullopt;
        openingTarget = bestTarget;
        openingTargetUntil = static_cast<int>(fullTurn) + 4;
        currentObjective = bestTarget;
        return move;
    }

    std::optional<Move> chooseOpeningRolloutMove(index_t targetPlayer,
                                                 const SituationMetrics& metrics) {
        if (!inOpeningPhase(metrics) || myGeneral == Coord{-1, -1}) {
            return std::nullopt;
        }

        struct SourceSeed {
            Coord source{-1, -1};
            double score = -1e100;
        };
        std::vector<SourceSeed> seeds;
        seeds.reserve(friendlyTilesCache.size());
        for (Coord source : friendlyTilesCache) {
            const TileView& tile = board.tileAt(source);
            if (tile.army <= 1) continue;
            double score = tile.army * 7.0 - reserveArmy[idx(source)] * 1.4;
            if (source == myGeneral) score += relaxedGeneralOpening() ? 8.0 : 2.0;
            score += duelExpansionHeatAt(source) * 0.20;
            score += localExpansionPotential(source) * 1.2;
            if (std::find(frontierTiles.begin(), frontierTiles.end(), source) !=
                frontierTiles.end()) {
                score += 12.0;
            }
            seeds.push_back(SourceSeed{source, score});
        }
        std::sort(seeds.begin(), seeds.end(),
                  [](const SourceSeed& lhs, const SourceSeed& rhs) {
                      return lhs.score > rhs.score;
                  });
        if (seeds.size() > 5) seeds.resize(5);

        const Coord enemyGuess = chooseTargetPlayerGeneral(targetPlayer);
        const bool hasEnemyGuess = inside(enemyGuess);
        const double guessWeight =
            hasEnemyGuess
                ? (targetPlayer >= 0 &&
                           targetPlayer <
                               static_cast<index_t>(cachedGeneralHasEvidence.size()) &&
                       cachedGeneralHasEvidence[targetPlayer]
                       ? 1.0
                       : 0.55)
                : 0.0;
        const int horizon = std::clamp(3 + static_cast<int>(fullTurn / 6), 3, 5);
        const size_t total = static_cast<size_t>((height + 2) * W);
        std::vector<uint8_t> pathMark(total, 0);

        auto stepGetsGrowthTick = [&](int offset) {
            return ((halfTurn + offset) & 1) == 1;
        };
        auto stepGetsGlobalGrowthTick = [&](int offset) {
            const int futureHalfTurn = static_cast<int>(halfTurn) + offset;
            const int futureFullTurn =
                static_cast<int>(fullTurn) + (((futureHalfTurn & 1) == 1) ? 0 : 1);
            return ((futureHalfTurn & 1) == 1) && futureFullTurn > 1 &&
                   ((futureFullTurn - 1) % 25 == 0);
        };

        CandidateMove best;
        std::function<double(Coord, army_t, int)> dfs =
            [&](Coord cur, army_t movingArmy, int depth) -> double {
            if (depth >= horizon || movingArmy <= 1) {
                double tail = movingArmy * 5.5;
                tail += localExpansionPotential(cur) * 5.0;
                tail += duelExpansionHeatAt(cur) * 1.6;
                return tail;
            }

            double bestFuture = movingArmy * 4.0 + duelExpansionHeatAt(cur) * 1.2;
            for (Coord d : kDirs) {
                Coord nxt = cur + d;
                if (!inside(nxt)) continue;
                const size_t node = idx(nxt);
                if (pathMark[node]) continue;
                const TileView& tile = board.tileAt(nxt);
                const TileMemory& mem = memory[node];
                if (!isPassable(mem.type)) continue;
                const index_t occupier = tile.visible ? tile.occupier : mem.occupier;
                if (isFriendlyOccupier(occupier)) continue;

                const army_t defense = estimatedArmyAt(nxt);
                if (movingArmy - 1 <= defense) continue;
                const army_t after = movingArmy - 1 - defense;
                if (after <= 0) continue;

                const bool enemy = isEnemyOccupier(occupier);
                const bool city = tile.type == TILE_CITY || mem.type == TILE_CITY;
                const bool exploration = !mem.everSeen || !tile.visible;
                double gain = 0.0;
                if (!mem.everSeen) gain += 44.0;
                else if (!tile.visible) gain += 24.0;
                else if (occupier < 0) gain += 12.0;
                if (enemy) gain += 9.0;
                gain += localExpansionPotential(nxt) * 7.8;
                gain += duelExpansionHeatAt(nxt) * 2.1;

                const int centerDist = std::abs(nxt.x * 2 - (height + 1)) +
                                       std::abs(nxt.y * 2 - (width + 1));
                gain += std::max(0.0, 20.0 - centerDist * 0.55);
                if (hasEnemyGuess) {
                    gain += std::max(0.0, 26.0 - manhattan(nxt, enemyGuess) * 1.7) *
                            guessWeight;
                }
                if (city) {
                    gain += 38.0 - estimatedArmyAt(nxt) * 1.1;
                    if (fullTurn + depth < 18) gain -= 18.0;
                }
                if (mem.type == TILE_SWAMP || tile.type == TILE_SWAMP) gain -= 26.0;
                if (stepGetsGrowthTick(depth)) gain += 7.0;
                if (stepGetsGlobalGrowthTick(depth)) gain += 12.0;
                if (enemy && stepGetsGrowthTick(depth)) gain += 4.0;
                if (after <= 2) gain -= 10.0;

                int frontierOptions = 0;
                for (Coord d2 : kDirs) {
                    Coord adj = nxt + d2;
                    if (!inside(adj)) continue;
                    const TileView& adjTile = board.tileAt(adj);
                    const TileMemory& adjMem = memory[idx(adj)];
                    if (!isPassable(adjMem.type)) continue;
                    const index_t adjOcc =
                        adjTile.visible ? adjTile.occupier : adjMem.occupier;
                    if (!isFriendlyOccupier(adjOcc) && !pathMark[idx(adj)]) {
                        ++frontierOptions;
                    }
                }
                gain += frontierOptions * 4.5;

                pathMark[node] = 1;
                gain += 0.82 * dfs(nxt, after, depth + 1);
                pathMark[node] = 0;
                bestFuture = std::max(bestFuture, gain);
            }
            return bestFuture;
        };

        for (const SourceSeed& seed : seeds) {
            const Coord source = seed.source;
            const TileView& src = board.tileAt(source);
            for (Coord d : kDirs) {
                Coord to = source + d;
                if (!inside(to) || shouldBlockOscillation(source, to)) continue;
                const TileView& dst = board.tileAt(to);
                const TileMemory& mem = memory[idx(to)];
                if (!isPassable(mem.type)) continue;
                const index_t occupier = dst.visible ? dst.occupier : mem.occupier;
                if (isFriendlyOccupier(occupier)) continue;

                const army_t defense = estimatedArmyAt(to);
                if (src.army - 1 <= defense) continue;
                const army_t after = src.army - 1 - defense;
                if (after <= 0) continue;

                Move move(MoveType::MOVE_ARMY, source, to, false);
                const double riskPenalty = moveRiskPenalty(move, metrics);
                if (riskPenalty >= 1e8) continue;

                double score = seed.score;
                score += captureTimingBonus(source, to);
                score += localExpansionPotential(to) * 7.5;
                score += duelExpansionHeatAt(to) * 2.6;
                if (!mem.everSeen) score += 52.0;
                else if (!dst.visible) score += 28.0;
                if (hasEnemyGuess) {
                    score += std::max(0.0, 32.0 - manhattan(to, enemyGuess) * 1.9) *
                             guessWeight;
                }
                pathMark[idx(source)] = 1;
                pathMark[idx(to)] = 1;
                score += 0.84 * dfs(to, after, 1);
                pathMark[idx(to)] = 0;
                pathMark[idx(source)] = 0;
                score -= riskPenalty;

                if (score > best.score) {
                    best.valid = true;
                    best.score = score;
                    best.riskPenalty = riskPenalty;
                    best.move = move;
                    best.source = source;
                    best.target = to;
                    best.mode = FocusMode::EXPAND;
                    best.tactical = false;
                    best.relievesCore = false;
                }
            }
        }

        if (!best.valid || best.score < 110.0) return std::nullopt;
        currentObjective = best.target;
        openingTarget = best.target;
        openingTargetUntil = static_cast<int>(fullTurn) + 3;
        return best.move;
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
        std::cerr << "[OimBot] " << label << " target=(" << move.target.x
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
                std::cerr << "[OimBot] candidate " << label << " invalid\n";
                return;
            }
            const army_t reserve =
                inside(move.source) ? reserveArmy[idx(move.source)] : 0;
            const army_t remain =
                move.valid && move.move.type == MoveType::MOVE_ARMY
                    ? moveRemainingArmy(move.move)
                    : 0;
            std::cerr << "[OimBot] candidate " << label
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
        std::cerr << "[OimBot] selected " << selectedLabel << "\n";
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
        const int currentRank = currentSeed >= 0
                                    ? seeds[currentSeed].rank
                                    : std::numeric_limits<int>::max();
        const int candidateRank = seeds[seedIndex].rank;
        if (candidateRank != currentRank) return candidateRank < currentRank;
        if (rawDist != workspace.rawDist[node])
            return rawDist < workspace.rawDist[node];
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
                    firstStep[nextIndex] = cur == start ? nxt : curFirstStep;
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

    std::vector<Coord> reconstructThreatRoute(
        Coord start, const ThreatPathResult& path) const {
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
        const std::vector<army_t> prevHiddenReserve = hiddenReserveEstimate;
        friendlyTilesCache.clear();
        frontierTiles.clear();
        ownedCitiesCache.clear();
        objectiveCandidates.clear();
        largestFriendlyArmyCache = 0;
        std::fill(visibleEnemyCountByPlayer.begin(),
                  visibleEnemyCountByPlayer.end(), 0);
        std::fill(visibleArmyByPlayer.begin(), visibleArmyByPlayer.end(), 0);

        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                const TileView& tile = board.tileAt(c);
                const TileMemory& mem = memory[idx(c)];
                if (tile.visible && isEnemyTile(tile) && tile.occupier >= 0 &&
                    tile.occupier < static_cast<index_t>(
                                        visibleEnemyCountByPlayer.size())) {
                    ++visibleEnemyCountByPlayer[tile.occupier];
                    visibleArmyByPlayer[tile.occupier] += tile.army;
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

        hiddenReserveEstimate.assign(playerCnt, 0);
        hiddenReserveDelta.assign(playerCnt, 0);
        for (index_t player = 0; player < playerCnt; ++player) {
            hiddenReserveEstimate[player] =
                std::max<army_t>(0, rankById[player].army - visibleArmyByPlayer[player]);
            if (player < static_cast<index_t>(prevHiddenReserve.size())) {
                hiddenReserveDelta[player] =
                    hiddenReserveEstimate[player] - prevHiddenReserve[player];
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
        if (player >= 0 &&
            player < static_cast<index_t>(cachedGeneralGuess.size())) {
            return cachedGeneralGuess[player];
        }
        return Coord{-1, -1};
    }

    void rebuildDuelExpansionHeat(index_t targetPlayer,
                                  const SituationMetrics& metrics) {
        const size_t total = static_cast<size_t>((height + 2) * W);
        duelExpansionHeat.assign(total, 0.0);
        if (!shouldBuildDuelExpansionHeat(metrics)) return;

        static constexpr std::array<double, 9> kStepDiscount = {
            0.0, 1.00, 0.87, 0.75, 0.64, 0.54, 0.46, 0.39, 0.33};
        const int radius = mapArea() >= 650 ? 8 : (mapArea() >= 520 ? 7 : 6);
        const double landTempoMultiplier = [&]() {
            const int turnsUntil = turnsUntilNextGlobalGrowth();
            if (turnsUntil == 0) return 1.34;
            if (turnsUntil <= 3) return 1.18;
            if (turnsUntil <= 6) return 1.08;
            return 1.0;
        }();

        Coord enemyGuess = chooseTargetPlayerGeneral(targetPlayer);
        const bool hasEnemyGuess =
            inside(enemyGuess) && targetPlayer >= 0 &&
            targetPlayer < static_cast<index_t>(cachedGeneralHasEvidence.size()) &&
            cachedGeneralHasEvidence[targetPlayer];

        std::vector<int> enemyDist(total, -1);
        if (hasEnemyGuess) {
            std::deque<Coord> q;
            q.push_back(enemyGuess);
            enemyDist[idx(enemyGuess)] = 0;
            while (!q.empty()) {
                const Coord cur = q.front();
                q.pop_front();
                const int curDist = enemyDist[idx(cur)];
                for (Coord d : kDirs) {
                    Coord nxt = cur + d;
                    if (!inside(nxt)) continue;
                    const size_t node = idx(nxt);
                    if (enemyDist[node] != -1 || !isPassable(memory[node].type)) {
                        continue;
                    }
                    enemyDist[node] = curDist + 1;
                    q.push_back(nxt);
                }
            }
        }

        std::vector<int> dist(total, -1);
        std::vector<int> stamp(total, 0);
        int activeStamp = 1;
        std::deque<Coord> q;

        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord start{x, y};
                const size_t startNode = idx(start);
                const TileView& startTile = board.tileAt(start);
                const TileMemory& startMem = memory[startNode];
                if (!isPassable(startMem.type)) continue;

                bool startFrontier = startTile.occupier != id;
                if (!startFrontier) {
                    for (Coord d : kDirs) {
                        Coord adj = start + d;
                        if (!inside(adj)) continue;
                        const TileView& adjTile = board.tileAt(adj);
                        const TileMemory& adjMem = memory[idx(adj)];
                        if (!isFriendlyTile(adjTile) || !adjTile.visible ||
                            !adjMem.everSeen || recentEnemyAt(adj)) {
                            startFrontier = true;
                            break;
                        }
                    }
                }
                if (startTile.occupier == id && !startFrontier) {
                    duelExpansionHeat[startNode] = -12.0;
                    continue;
                }

                if (activeStamp == std::numeric_limits<int>::max()) {
                    std::fill(stamp.begin(), stamp.end(), 0);
                    activeStamp = 1;
                } else {
                    ++activeStamp;
                }
                q.clear();
                stamp[startNode] = activeStamp;
                dist[startNode] = 0;
                q.push_back(start);

                double score = 0.0;
                if (!startMem.everSeen) score += 14.0 * landTempoMultiplier;
                if (!startTile.visible) score += 5.0;
                if (startTile.occupier < 0) score += 3.0;
                if (startMem.type == TILE_SWAMP || startTile.type == TILE_SWAMP) {
                    score -= 8.0;
                }
                if (startMem.type == TILE_CITY || startTile.type == TILE_CITY) {
                    score += std::max(0.0, 18.0 - estimatedArmyAt(start) * 0.18);
                }

                while (!q.empty()) {
                    const Coord cur = q.front();
                    q.pop_front();
                    const int curDist = dist[idx(cur)];
                    if (curDist >= radius) continue;

                    for (Coord d : kDirs) {
                        Coord nxt = cur + d;
                        if (!inside(nxt)) continue;
                        const size_t node = idx(nxt);
                        if (stamp[node] == activeStamp ||
                            !isPassable(memory[node].type)) {
                            continue;
                        }
                        stamp[node] = activeStamp;
                        dist[node] = curDist + 1;
                        q.push_back(nxt);

                        const TileView& tile = board.tileAt(nxt);
                        const TileMemory& mem = memory[node];
                        if (tile.occupier == id) continue;

                        double cell = 0.0;
                        if (!mem.everSeen) {
                            cell += 8.8 * landTempoMultiplier;
                        } else if (!tile.visible) {
                            cell += 5.0 * landTempoMultiplier;
                        } else if (tile.occupier < 0) {
                            cell += 2.6 * landTempoMultiplier;
                        } else if (isEnemyOccupier(tile.occupier)) {
                            cell += 3.0;
                        }
                        if (tile.type == TILE_CITY || mem.type == TILE_CITY) {
                            cell += std::max(0.0,
                                             10.0 - estimatedArmyAt(nxt) * 0.08);
                        }
                        if (tile.type == TILE_SWAMP || mem.type == TILE_SWAMP) {
                            cell -= 2.6;
                        }
                        cell += std::max(0, passableDegree(nxt) - 2) * 0.85;

                        double contest = 1.0;
                        if (hasEnemyGuess && enemyDist[node] >= 0) {
                            contest = std::clamp(
                                1.0 + (enemyDist[node] - dist[node] - 2) * 0.08,
                                0.50, 1.65);
                        }
                        score += cell * kStepDiscount[dist[node]] * contest;
                    }
                }

                const int centerDist = std::abs(start.x * 2 - (height + 1)) +
                                       std::abs(start.y * 2 - (width + 1));
                score += std::max(0.0, 26.0 - centerDist * 0.40);
                if (myGeneral != Coord{-1, -1}) {
                    score -= manhattan(start, myGeneral) *
                             (mapArea() >= 520 ? 0.14 : 0.09);
                }
                if (startTile.occupier == id) score -= 6.0;
                duelExpansionHeat[startNode] = score;
            }
        }
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
        const double intel = intelWeight();
        score +=
            std::max<army_t>(0, 36 - hiddenReserveEstimate[player]) * 0.9 * intel;
        score -=
            std::max<army_t>(0, hiddenReserveEstimate[player] - 48) * 0.12 * intel;
        score += std::max<army_t>(0, -hiddenReserveDelta[player]) * 0.55 * intel;
        score -= std::max<army_t>(0, hiddenReserveDelta[player]) * 0.25 * intel;

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

    bool relaxedGeneralOpening() const { return cachedRelaxedGeneralOpening; }

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
            penalty += (reserve - remain) *
                       (move.from == myGeneral
                            ? (cachedRelaxedGeneralOpening ? 4.0 : 10.0)
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
        for (size_t i = 0;
             i < plan.route.size() && corridorLen < corridor.size(); ++i) {
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
        const double expansionBias = expansionUrgency(metrics);
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
            4 + static_cast<int>(metrics.mobilityScore * 0.08) +
                static_cast<int>(expansionBias * 0.12),
            4, 10);
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
                score += 42.0 + expansionBias * 2.4;
                if (metrics.contactDensity < 0.08) score += 24.0;
                if (friendlyLead == 0) score += 18.0;
            }
            if (objective.exploration) score += 12.0 + expansionBias * 1.6;
            if (mode == FocusMode::ATTACK && expansionBias > 8.0 &&
                metrics.contactDensity < 0.08 &&
                metrics.conversionAdvantage < 8.0) {
                score -= expansionBias * 2.1;
            }
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

            score += captureTimingBonus(seed.source, firstStep);

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
        const ThreatPathResult positivePath = multiSourceThreatReversePath(
            positiveSeeds, 14, positiveThreatSearch);

        std::optional<ThreatInfo> best;
        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord source{x, y};
                army_t sourceArmy = 0;
                bool valid = false;
                bool visibleSource = false;
                bool recentSource = false;
                index_t sourcePlayer = -1;
                const TileView& tile = board.tileAt(source);
                if (isEnemyTile(tile) && tile.army > 1) {
                    sourceArmy = tile.army;
                    valid = true;
                    visibleSource = true;
                    sourcePlayer = tile.occupier;
                } else if (recentEnemyAt(source, 2) &&
                           memory[idx(source)].army > 1) {
                    sourceArmy = memory[idx(source)].army;
                    valid = true;
                    recentSource = true;
                    sourcePlayer = memory[idx(source)].occupier;
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
                    if (seedIndex < 0 ||
                        seedIndex >= static_cast<int>(seeds.size())) {
                        return;
                    }
                    const int dist = path.distance(node);
                    if (dist >= kInf) return;
                    const int turns = path.stepCount(node);
                    if (turns >= kInf || turns <= 0) return;

                    const ThreatSeed& seed = seeds[seedIndex];
                    double pressure =
                        sourceArmy * 2.2 - dist * 10.5 + seed.bias;
                    if (isEnemyOccupier(sourcePlayer)) {
                        const double intel = intelWeight();
                        pressure +=
                            std::min<army_t>(hiddenReserveEstimate[sourcePlayer], 40) *
                            (seed.target == myGeneral ? 0.16 : 0.09) * intel;
                        pressure +=
                            std::max<army_t>(0, hiddenReserveDelta[sourcePlayer]) *
                            (seed.target == myGeneral ? 0.45 : 0.20) * intel;
                    }

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

    std::optional<StrategicPlan> chooseCounterRacePlan(
        const ThreatInfo& threat, const SituationMetrics& metrics,
        index_t fallbackTargetPlayer) const {
        if (!threat.valid || myGeneral == Coord{-1, -1} ||
            threat.target != myGeneral) {
            return std::nullopt;
        }
        if (!duelMode()) return std::nullopt;

        index_t threatPlayer = visibleOrRememberedOccupier(threat.source);
        if (!isEnemyOccupier(threatPlayer)) threatPlayer = fallbackTargetPlayer;
        if (!isEnemyOccupier(threatPlayer)) return std::nullopt;

        const Coord target = chooseTargetPlayerGeneral(threatPlayer);
        if (!inside(target) || knownGenerals[threatPlayer] != target) {
            return std::nullopt;
        }

        ObjectiveOption counter{
            360.0 + std::max(0.0, metrics.conversionAdvantage * 2.0), target,
            threatPlayer, true, false, false, false};
        StrategicPlan plan = evaluateObjectivePlan(counter, metrics);
        if (!plan.valid || plan.usingRally) return std::nullopt;
        if (plan.route.empty()) return std::nullopt;
        if (static_cast<int>(plan.route.size()) >= threat.turns) {
            return std::nullopt;
        }
        if (plan.commitArmy + plan.corridorSupport < plan.need + 1) {
            return std::nullopt;
        }
        if (!moveLooksSafe(plan.move)) return std::nullopt;
        return plan;
    }

    std::optional<Move> chooseDefensiveMove(
        const SituationMetrics& metrics,
        const std::optional<ThreatInfo>& threat) {
        if (!threat.has_value() || !shouldDefend(*threat, metrics)) {
            return std::nullopt;
        }

        if (auto counterRace =
                chooseCounterRacePlan(*threat, metrics, currentTargetPlayer)) {
            currentObjective = counterRace->target;
            currentTargetPlayer = counterRace->targetPlayer;
            lockedObjective = counterRace->target;
            objectiveLockUntil = static_cast<int>(fullTurn) + 6;
            return counterRace->move;
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
                                                    currentTargetPlayer, true,
                                                    true, false, false});
        defenseObjectives.push_back(ObjectiveOption{210.0, threat->source,
                                                    currentTargetPlayer, true,
                                                    true, false, false});
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

    std::optional<Move> chooseDuelTempoMove(index_t targetPlayer,
                                            const SituationMetrics& metrics) {
        if (!inDuelTempoPhase(metrics)) return std::nullopt;

        struct SourceSeed {
            Coord source{-1, -1};
            army_t available = 0;
            double score = -1e100;
        };
        std::vector<SourceSeed> seeds;
        seeds.reserve(friendlyTilesCache.size());
        for (Coord source : friendlyTilesCache) {
            const army_t available = availableArmy(source);
            if (available <= 0) continue;

            double score = available * 7.3 - reserveArmy[idx(source)] * 0.9;
            if (source == myGeneral) {
                score -= relaxedGeneralOpening() ? 1.0 : 4.5;
            }
            if (chokeMask[idx(source)]) score -= 5.5;
            if (std::find(frontierTiles.begin(), frontierTiles.end(), source) !=
                frontierTiles.end()) {
                score += 9.0;
            }
            score += duelExpansionHeatAt(source) * 0.20;
            seeds.push_back(SourceSeed{source, available, score});
        }
        std::sort(seeds.begin(), seeds.end(),
                  [](const SourceSeed& lhs, const SourceSeed& rhs) {
                      return lhs.score > rhs.score;
                  });
        const size_t seedLimit = static_cast<size_t>(
            std::clamp(mapArea() >= 520 ? 8 : 6, 5, 8));
        if (seeds.size() > seedLimit) seeds.resize(seedLimit);

        const Coord targetGuess = chooseTargetPlayerGeneral(targetPlayer);
        const bool hasTargetGuess =
            inside(targetGuess) && targetPlayer >= 0 &&
            targetPlayer < static_cast<index_t>(cachedGeneralHasEvidence.size()) &&
            cachedGeneralHasEvidence[targetPlayer];
        const double landTempoMultiplier = [&]() {
            const int turnsUntil = turnsUntilNextGlobalGrowth();
            if (turnsUntil == 0) return 1.25;
            if (turnsUntil <= 3) return 1.12;
            return 1.0;
        }();

        CandidateMove best;
        for (const SourceSeed& seed : seeds) {
            const Coord source = seed.source;
            const auto path = weightedPath(source, [&](Coord c) {
                int cost = 2;
                const TileMemory& mem = memory[idx(c)];
                const TileView& tile = board.tileAt(c);
                if (!mem.everSeen) cost -= 1;
                else if (!tile.visible) cost -= 1;
                if (mem.type == TILE_SWAMP || tile.type == TILE_SWAMP) cost += 5;
                if (mem.type == TILE_CITY || tile.type == TILE_CITY) {
                    cost += std::max<int>(3, static_cast<int>(estimatedArmyAt(c) / 6));
                }
                if (isEnemyOccupier(visibleOrRememberedOccupier(c))) {
                    cost +=
                        std::max<int>(0, static_cast<int>(estimatedArmyAt(c) / 8));
                }
                return std::max(1, cost);
            });

            const int reach =
                std::clamp(8 + static_cast<int>(seed.available / 6) +
                               (mapArea() >= 520 ? 2 : 0),
                           8, mapArea() >= 650 ? 14 : 12);

            for (pos_t x = 1; x <= height; ++x) {
                for (pos_t y = 1; y <= width; ++y) {
                    Coord target{x, y};
                    const int dist = path.distance(idx(target));
                    if (dist >= kInf || dist > reach) continue;

                    const TileView& tile = board.tileAt(target);
                    const TileMemory& mem = memory[idx(target)];
                    if (!isPassable(mem.type) || tile.occupier == id) continue;

                    const index_t occupier =
                        tile.visible ? tile.occupier : mem.occupier;
                    const bool enemy = isEnemyOccupier(occupier);
                    const bool city =
                        tile.type == TILE_CITY || mem.type == TILE_CITY;
                    const bool exploration = !mem.everSeen || !tile.visible;
                    const Coord firstStep = path.firstStepAt(idx(target));
                    if (firstStep == Coord{-1, -1}) continue;
                    if (!immediateAdvancePossible(source, firstStep)) continue;

                    double score = seed.score - dist * 5.0;
                    score += duelExpansionHeatAt(target) * 3.0;
                    score += localExpansionPotential(target) * 1.8;
                    if (exploration) score += 20.0 * landTempoMultiplier;
                    if (!tile.visible) score += 12.0 * landTempoMultiplier;
                    if (enemy) {
                        score -= 20.0 + estimatedArmyAt(target) * 0.35;
                        if (occupier == targetPlayer && fullTurn >= 36) {
                            score += 24.0;
                        }
                    }
                    if (city) {
                        score += 84.0 - estimatedArmyAt(target) * 2.0;
                        if (fullTurn < 26) score -= 14.0;
                    }
                    if (mem.type == TILE_SWAMP || tile.type == TILE_SWAMP) {
                        score -= 26.0;
                    }
                    if (hasTargetGuess) {
                        score += std::max(
                            0.0, 18.0 - manhattan(target, targetGuess) * 1.35);
                    }
                    if (path.friendlyLeadAt(idx(target)) == 0) {
                        score += 14.0;
                    } else if (path.friendlyLeadAt(idx(target)) > 2) {
                        score -= 18.0;
                    }

                    Move move(MoveType::MOVE_ARMY, source, firstStep, false);
                    const double riskPenalty = moveRiskPenalty(move, metrics);
                    if (riskPenalty >= 1e8) continue;
                    score += captureTimingBonus(source, firstStep);
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

        if (!best.valid || best.score < 140.0) return std::nullopt;
        currentObjective = best.target;
        return best.move;
    }

    std::optional<Move> chooseTinyDuelPressureMove(
        index_t targetPlayer, const SituationMetrics& metrics) {
        if (!duelMode() || mapArea() > 400) return std::nullopt;
        if (fullTurn < 14 || fullTurn > 70) return std::nullopt;
        if (metrics.coreRisk > 10.0) return std::nullopt;

        Coord target = chooseTargetPlayerGeneral(targetPlayer);
        if (!inside(target)) {
            target = Coord{static_cast<pos_t>((height + 1) / 2),
                           static_cast<pos_t>((width + 1) / 2)};
        }
        if (!inside(target) || !isPassable(memory[idx(target)].type)) {
            return std::nullopt;
        }

        struct SourceSeed {
            Coord source{-1, -1};
            double score = -1e100;
        };
        std::vector<SourceSeed> seeds;
        for (Coord source : friendlyTilesCache) {
            const army_t available = availableArmy(source);
            if (available <= 0) continue;
            double score = available * 8.4 - reserveArmy[idx(source)] * 1.0;
            score -= manhattan(source, target) * 1.4;
            if (source == myGeneral) score -= relaxedGeneralOpening() ? 1.0 : 3.0;
            if (chokeMask[idx(source)]) score -= 5.0;
            seeds.push_back(SourceSeed{source, score});
        }
        std::sort(seeds.begin(), seeds.end(),
                  [](const SourceSeed& lhs, const SourceSeed& rhs) {
                      return lhs.score > rhs.score;
                  });
        if (seeds.size() > 5) seeds.resize(5);

        CandidateMove best;
        for (const SourceSeed& seed : seeds) {
            const Coord source = seed.source;
            const auto path = weightedPath(source, [&](Coord c) {
                int cost = 2;
                const TileMemory& mem = memory[idx(c)];
                const TileView& tile = board.tileAt(c);
                if (!mem.everSeen) cost -= 1;
                if (!tile.visible) cost -= 1;
                if (mem.type == TILE_SWAMP || tile.type == TILE_SWAMP) cost += 10;
                if (mem.type == TILE_CITY || tile.type == TILE_CITY) {
                    cost += std::max<int>(5, static_cast<int>(estimatedArmyAt(c) / 4));
                }
                if (isEnemyOccupier(visibleOrRememberedOccupier(c))) {
                    cost += std::max<int>(0, static_cast<int>(estimatedArmyAt(c) / 6));
                }
                return std::max(1, cost);
            });

            const int dist = path.distance(idx(target));
            const Coord firstStep = path.firstStepAt(idx(target));
            if (dist >= kInf || firstStep == Coord{-1, -1}) continue;
            if (!immediateAdvancePossible(source, firstStep)) continue;

            Move move(MoveType::MOVE_ARMY, source, firstStep, false);
            const double riskPenalty = moveRiskPenalty(move, metrics);
            if (riskPenalty >= 1e8) continue;

            double score = seed.score - dist * 8.2;
            score += duelExpansionHeatAt(firstStep) * 1.0;
            score += localExpansionPotential(firstStep) * 2.0;
            score += captureTimingBonus(source, firstStep);
            score += std::max(0.0, metrics.conversionAdvantage) * 2.8;
            if (currentObjective != Coord{-1, -1}) {
                score -= manhattan(firstStep, currentObjective) * 0.7;
            }

            const TileView& dst = board.tileAt(firstStep);
            const TileMemory& mem = memory[idx(firstStep)];
            const index_t occupier = dst.visible ? dst.occupier : mem.occupier;
            if (isEnemyOccupier(occupier)) score += 18.0;
            if (!mem.everSeen) score += 8.0;
            if (mem.type == TILE_SWAMP || dst.type == TILE_SWAMP) score -= 18.0;

            if (score > best.score) {
                best.valid = true;
                best.score = score - riskPenalty;
                best.riskPenalty = riskPenalty;
                best.move = move;
                best.source = source;
                best.target = target;
                best.mode = FocusMode::ATTACK;
                best.tactical = true;
                best.relievesCore = false;
            }
        }

        if (!best.valid || best.score < 95.0) return std::nullopt;
        currentObjective = best.target;
        return best.move;
    }

    std::optional<Move> chooseDuelExpansionCaptureMove(
        index_t targetPlayer, const SituationMetrics& metrics) {
        if (!duelMode() || mapArea() < 500) return std::nullopt;
        if (metrics.contactDensity > 0.050 || metrics.coreRisk > 10.0) {
            return std::nullopt;
        }
        if (fullTurn > (mapArea() >= 650 ? 135 : 120)) return std::nullopt;

        CandidateMove best;
        const Coord targetGuess = chooseTargetPlayerGeneral(targetPlayer);
        const bool hasTargetGuess =
            inside(targetGuess) && targetPlayer >= 0 &&
            targetPlayer < static_cast<index_t>(cachedGeneralHasEvidence.size()) &&
            cachedGeneralHasEvidence[targetPlayer];

        for (Coord from : friendlyTilesCache) {
            const TileView& src = board.tileAt(from);
            if (src.army <= 1) continue;

            for (Coord d : kDirs) {
                Coord to = from + d;
                if (!inside(to) || shouldBlockOscillation(from, to)) continue;

                const TileView& dst = board.tileAt(to);
                const TileMemory& mem = memory[idx(to)];
                if (!isPassable(mem.type) || isFriendlyTile(dst)) continue;

                const army_t attack = src.army - 1;
                const army_t defense = estimatedArmyAt(to);
                if (attack <= defense) continue;

                const index_t occupier =
                    dst.visible ? dst.occupier : mem.occupier;
                const bool enemy = isEnemyOccupier(occupier);
                const bool city =
                    dst.type == TILE_CITY || mem.type == TILE_CITY;
                const bool exploration = !mem.everSeen || !dst.visible;
                Move move(MoveType::MOVE_ARMY, from, to, false);
                const double riskPenalty = moveRiskPenalty(move, metrics);
                if (riskPenalty >= 1e8) continue;

                double score = 0.0;
                if (!mem.everSeen) score += 96.0;
                else if (!dst.visible) score += 44.0;
                else if (occupier < 0) score += 18.0;
                score += duelExpansionHeatAt(to) * 2.25;
                score += localExpansionPotential(to) * 7.2;
                score += captureTimingBonus(from, to);

                const int centerDist = std::abs(to.x * 2 - (height + 1)) +
                                       std::abs(to.y * 2 - (width + 1));
                score += std::max(0.0, 18.0 - centerDist * 0.34);
                if (city) {
                    score += 82.0 - estimatedArmyAt(to) * 1.9;
                    if (fullTurn < 25) score -= 16.0;
                }
                if (enemy) {
                    score -= 28.0 + estimatedArmyAt(to) * 0.30;
                    if (occupier == targetPlayer && fullTurn >= 55) {
                        score += 16.0;
                    }
                }
                if (exploration) score += 16.0;
                if (mem.type == TILE_SWAMP || dst.type == TILE_SWAMP) {
                    score -= 24.0;
                }
                if (hasTargetGuess) {
                    score +=
                        std::max(0.0, 16.0 - manhattan(to, targetGuess) * 1.20);
                }
                if (from == myGeneral) {
                    score -= relaxedGeneralOpening() ? 1.5 : 5.5;
                }
                score -= riskPenalty;

                if (score > best.score) {
                    best.valid = true;
                    best.score = score;
                    best.riskPenalty = riskPenalty;
                    best.move = move;
                    best.source = from;
                    best.target = to;
                    best.mode = enemy ? FocusMode::ATTACK : FocusMode::EXPAND;
                    best.tactical = city || enemy;
                    best.relievesCore = false;
                }
            }
        }

        if (!best.valid || best.score < 120.0) return std::nullopt;
        currentObjective = best.target;
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
                score += captureTimingBonus(from, to);
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
        const double duelSmallAggression =
            duelMode() && mapArea() <= 450 ? 1.0 : 0.0;
        const int enemyContactSignal =
            targetPlayer >= 0 ? visibleEnemyCountByPlayer[targetPlayer] +
                                    newlyVisibleEnemyTiles[targetPlayer]
                              : 0;
        const bool shouldPressureTarget = enemyContactSignal > 0 ||
                                          metrics.contactDensity > 0.045 ||
                                          (duelSmallAggression > 0.0 &&
                                           fullTurn >= 16) ||
                                          metrics.conversionAdvantage > 8.0;
        const double expansionBias = expansionUrgency(metrics);
        const double expansionIntel = duelMode() ? 1.0 : 0.0;
        const bool canChaseGuess =
            targetPlayer >= 0 &&
            targetPlayer <
                static_cast<index_t>(cachedGeneralHasEvidence.size()) &&
            (cachedGeneralHasEvidence[targetPlayer] ||
             (duelSmallAggression > 0.0 && fullTurn >= 12 &&
              targetGuess != Coord{-1, -1}));
        if (targetGuess != Coord{-1, -1} && canChaseGuess) {
            addObjective(
                ObjectiveOption{230.0 - expansionBias * 2.6 +
                                    duelSmallAggression * 36.0,
                                targetGuess,
                                targetPlayer, true, false, false, false});
            for (Coord d : kDirs) {
                Coord adj = targetGuess + d;
                addObjective(ObjectiveOption{
                    180.0 - expansionBias * 2.0 + duelSmallAggression * 28.0,
                    adj, targetPlayer, true,
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
                    score += duelSmallAggression * 14.0;
                } else {
                    score += 22.0;
                }
                if (occupier == targetPlayer) {
                    score += shouldPressureTarget
                                 ? 92.0 + duelSmallAggression * 24.0
                                 : 50.0 + duelSmallAggression * 16.0;
                }
                if (city) {
                    score += 236.0 - estimatedArmyAt(c) * 2.8;
                    score += expansionBias * 3.6;
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
                    score += expansionBias * 2.8;
                    if (shouldPressureTarget) score -= 22.0;
                }
                if (!enemy) {
                    score += duelExpansionHeatAt(c) * 0.95 * expansionIntel *
                             duelExpansionPlanningWeight();
                }
                if (!enemy) score += localExpansionPotential(c) * 2.8 * expansionIntel;
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
                    score -= 26.0 + expansionBias * 1.4;
                }
                if (enemy && !city && !exploration &&
                    metrics.contactDensity < 0.08 &&
                    metrics.conversionAdvantage < 4.0) {
                    score -= 48.0 + expansionBias * 2.0;
                }
                if (score <= -1e90) continue;

                ObjectiveOption option{
                    score,      c,     occupier >= 0 ? occupier : targetPlayer,
                    enemy,      false, city,
                    exploration};
                const bool nearTargetGuess = targetGuess != Coord{-1, -1} &&
                                             canChaseGuess &&
                                             manhattan(c, targetGuess) <= 4;
                if (enemy && (occupier == targetPlayer || nearTargetGuess)) {
                    option.score += 18.0 + duelSmallAggression * 22.0;
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
                                            metrics.coreRisk * 0.04 +
                                            expansionBias * 0.22),
                       8, 18);
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
        appendBucket(cityOptions, 4 + (expansionBias >= 7.0 ? 1 : 0));
        appendBucket(pressureOptions, 4);
        appendBucket(expandOptions, 5 + (expansionBias >= 9.0 ? 1 : 0));

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
        const double expansionBias = expansionUrgency(metrics);
        const double expansionIntel = duelMode() ? 1.0 : 0.0;
        const double duelSmallAggression =
            duelMode() && mapArea() <= 450 ? 1.0 : 0.0;
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
        const size_t seedLimit = static_cast<size_t>(
            std::clamp(5 + static_cast<int>(expansionBias * 0.18), 5, 8));
        if (seeds.size() > seedLimit) seeds.resize(seedLimit);

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
                        score += expansionBias * 3.2;
                    }
                    if (!enemy) {
                        score += duelExpansionHeatAt(target) * 1.10 *
                                 expansionIntel * duelExpansionPlanningWeight();
                    }
                    if (!enemy)
                        score += localExpansionPotential(target) * 3.0 * expansionIntel;
                    if (!tile.visible) score += 22.0;
                    if (enemy) score += 28.0 + estimatedArmyAt(target) * 0.10;
                    if (occupier == targetPlayer) score += 18.0;
                    if (city) {
                        score += 224.0 - estimatedArmyAt(target) * 2.5;
                        score += expansionBias * 4.0;
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
                            0.0, 32.0 + duelSmallAggression * 16.0 -
                                     expansionBias * 1.2 -
                                     manhattan(target, targetGuess) * 2.0);
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
                    if (enemy && !city && !exploration)
                        score -= 24.0 + expansionBias * 1.4 -
                                 duelSmallAggression * 16.0;
                    if (enemy && !city && metrics.contactDensity < 0.08 &&
                        metrics.conversionAdvantage < 4.0) {
                        score -= 38.0 + expansionBias * 1.8 -
                                 duelSmallAggression * 20.0;
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
                    score += captureTimingBonus(source, firstStep);
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
        const double expansionIntel = duelMode() ? 1.0 : 0.0;

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
            if (occupier != id)
                score += localExpansionPotential(to) * 1.6 * expansionIntel;
            if (!tile.visible) score += 18.0;
            if (tile.type == TILE_CITY)
                score += 75.0 - estimatedArmyAt(to) * 2.5;
            if (tile.type == TILE_SWAMP) score -= 18.0;
            if (occupier != id) score += 24.0;
            if (occupier == targetPlayer) score += 18.0;
            if (currentObjective == to) score += 12.0;
            score += captureTimingBonus(source, to);
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
        const double expansionBias = expansionUrgency(metrics);
        rebuildDuelExpansionHeat(targetPlayer, metrics);

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

        if (auto rolloutOpening = chooseOpeningRolloutMove(targetPlayer, metrics)) {
            return rolloutOpening;
        }

        if (auto opening = chooseOpeningMove(targetPlayer, metrics)) {
            return opening;
        }

        if (auto tinyPressure = chooseTinyDuelPressureMove(targetPlayer, metrics)) {
            return tinyPressure;
        }

        if (auto tempo = chooseDuelTempoMove(targetPlayer, metrics)) {
            return tempo;
        }

        if (auto bloom = chooseDuelExpansionCaptureMove(targetPlayer, metrics)) {
            return bloom;
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
        double plannedMargin = 18.0;
        if (planned.valid && planned.mode == FocusMode::EXPAND) {
            plannedMargin -= expansionBias * 2.2;
        } else if (planned.valid && planned.mode == FocusMode::ATTACK &&
                   metrics.contactDensity < 0.08 &&
                   metrics.conversionAdvantage < 8.0) {
            plannedMargin -= expansionBias * 0.9;
        }
        if (planned.valid &&
            (!economic.valid || planned.score + plannedMargin >= economic.score ||
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
        visibleArmyByPlayer.assign(playerCnt, 0);
        hiddenReserveEstimate.assign(playerCnt, 0);
        hiddenReserveDelta.assign(playerCnt, 0);
        duelExpansionHeat.assign(total, 0.0);
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
        openingTarget = Coord{-1, -1};
        openingTargetUntil = -1;
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

static BotRegistrar<OimBot> oim_bot_reg("oimbot");
