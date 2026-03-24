/**
 * @file KutuBot.cpp
 *
 * KutuBot is a 1v1-oriented built-in bot for LocalGen v6.
 */

#ifndef LGEN_BOTS_KUTUBOT
#define LGEN_BOTS_KUTUBOT

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

class KutuBot : public BasicBot {
   private:
    static constexpr std::array<Coord, 4> kDirs = {Coord{-1, 0}, Coord{0, -1},
                                                   Coord{1, 0}, Coord{0, 1}};
    static constexpr int kInf = 1e9;
    static constexpr int kRecentEdgeWindow = 14;

    struct TileMemory {
        tile_type_e type = TILE_BLANK;
        army_t army = 0;
        index_t occupier = -1;
        bool everSeen = false;
        bool visible = false;
        int lastSeenTurn = -1;
        int estimatedTurn = -1;
    };

    struct PathResult {
        bool reachable = false;
        const std::vector<int>* dist = nullptr;
        const std::vector<int>* stamp = nullptr;
        const std::vector<Coord>* parent = nullptr;
        int activeStamp = 0;

        int distance(size_t node) const {
            if (dist == nullptr || stamp == nullptr || node >= dist->size()) {
                return kInf;
            }
            return (*stamp)[node] == activeStamp ? (*dist)[node] : kInf;
        }

        Coord parentAt(size_t node) const {
            if (parent == nullptr || stamp == nullptr || node >= parent->size()) {
                return Coord{-1, -1};
            }
            return (*stamp)[node] == activeStamp ? (*parent)[node]
                                                 : Coord{-1, -1};
        }
    };

    struct SearchWorkspace {
        std::vector<int> dist;
        std::vector<int> stamp;
        std::vector<Coord> parent;
        std::vector<std::pair<int, Coord>> heap;
        int activeStamp = 1;
    };

    struct CandidateMove {
        Move move{};
        double score = -1e100;
        bool valid = false;
    };

    struct ThreatInfo {
        Coord enemy{-1, -1};
        Coord target{-1, -1};
        std::vector<Coord> route;
        army_t army = 0;
        double score = -1e100;
    };

    struct RecentEdge {
        Coord from{-1, -1};
        Coord to{-1, -1};
        std::uint64_t key = 0;
    };

    struct ObjectiveOption {
        Coord target{-1, -1};
        double score = -1e100;
    };

    struct SourceCandidate {
        Coord source{-1, -1};
        army_t sourceArmy = 0;
        army_t commitArmy = 0;
        int distance = kInf;
        double seedScore = -1e100;
    };

    enum class FocusMode : uint8_t {
        ECONOMY,
        ASSAULT,
        DEFENSE,
        CONVERSION
    };

    struct FocusPlan {
        Coord objective{-1, -1};
        Coord source{-1, -1};
        std::vector<Coord> route;
        army_t sourceArmy = 0;
        army_t commitArmy = 0;
        army_t corridorSupport = 0;
        double sourceScore = -1e100;
        int distance = kInf;
        bool rally = false;
        bool valid = false;
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
    std::vector<Coord> ownedCitiesCache;
    std::vector<Coord> duelEnemyTargetsCache;
    std::vector<Coord> duelFrontierFogCache;
    std::vector<int> visibleEnemyCountByPlayer;
    std::vector<uint8_t> duelFogMark;
    std::vector<double> duelCandidateScoreCache;
    std::vector<uint8_t> duelCandidateMark;
    std::vector<Coord> duelCandidateTouched;
    army_t largestFriendlyArmyCache = 0;
    Coord myGeneral{-1, -1};
    Coord currentObjective{-1, -1};
    Coord lockedObjective{-1, -1};
    int objectiveLockUntil = -1;
    index_t lockedTargetPlayer = -1;
    int targetLockUntil = -1;
    Coord lastMoveFrom{-1, -1};
    Coord lastMoveTo{-1, -1};

    SearchWorkspace forwardSearch;
    mutable SearchWorkspace reverseSearch;
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

    inline bool isFriendlyTile(const TileView& tile) const {
        return tile.occupier == id ||
               (tile.occupier >= 0 &&
                tile.occupier < static_cast<index_t>(teams.size()) &&
                teams[tile.occupier] == teams[id]);
    }

    inline bool isEnemyTile(const TileView& tile) const {
        return tile.occupier >= 0 && tile.occupier != id &&
               (tile.occupier >= static_cast<index_t>(teams.size()) ||
                teams[tile.occupier] != teams[id]);
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
                if (!inside(nxt)) continue;
                if (!isPassable(memory[idx(nxt)].type)) continue;
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

    PathResult weightedReversePath(Coord goal,
                                   const std::function<int(Coord)>& stepCost) const {
        PathResult result;
        if (!inside(goal)) return result;

        SearchWorkspace& workspace = reverseSearch;
        prepareWorkspace(workspace);
        const size_t goalIndex = idx(goal);
        touchNode(workspace, goalIndex);
        workspace.dist[goalIndex] = 0;
        workspace.heap.emplace_back(0, goal);
        std::push_heap(workspace.heap.begin(), workspace.heap.end(),
                       minHeapCompare);

        while (!workspace.heap.empty()) {
            std::pop_heap(workspace.heap.begin(), workspace.heap.end(),
                          minHeapCompare);
            const auto [curDist, cur] = workspace.heap.back();
            workspace.heap.pop_back();
            if (curDist != workspace.dist[idx(cur)]) continue;

            for (Coord d : kDirs) {
                Coord prev = cur + d;
                if (!inside(prev)) continue;
                if (!isPassable(memory[idx(prev)].type)) continue;
                const size_t prevIndex = idx(prev);
                touchNode(workspace, prevIndex);
                const int nd = curDist + stepCost(cur);
                if (nd < workspace.dist[prevIndex]) {
                    workspace.dist[prevIndex] = nd;
                    workspace.parent[prevIndex] = cur;
                    workspace.heap.emplace_back(nd, prev);
                    std::push_heap(workspace.heap.begin(), workspace.heap.end(),
                                   minHeapCompare);
                }
            }
        }

        result.reachable = true;
        result.dist = &workspace.dist;
        result.stamp = &workspace.stamp;
        result.parent = &workspace.parent;
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

    bool tileGrowsEveryTurn(tile_type_e type) const {
        return type == TILE_GENERAL || type == TILE_CITY;
    }

    void evolveHiddenMemory(TileMemory& mem) {
        if (!mem.everSeen || mem.visible || mem.estimatedTurn < 0) return;
        if (fullTurn <= static_cast<turn_t>(mem.estimatedTurn)) return;

        for (int turn = mem.estimatedTurn + 1; turn <= static_cast<int>(fullTurn);
             ++turn) {
            if (mem.occupier < 0) break;
            switch (mem.type) {
                case TILE_GENERAL:
                case TILE_CITY: ++mem.army; break;
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
            if (mem.occupier >= 0 && mem.army > 0 &&
                mem.type != TILE_SWAMP) {
                mem.army = std::max<army_t>(1, mem.army);
            }
        }
        mem.estimatedTurn = static_cast<int>(fullTurn);
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
        ownedCitiesCache.clear();
        duelEnemyTargetsCache.clear();
        duelFrontierFogCache.clear();
        largestFriendlyArmyCache = 0;
        std::fill(visibleEnemyCountByPlayer.begin(),
                  visibleEnemyCountByPlayer.end(), 0);
        std::fill(duelFogMark.begin(), duelFogMark.end(), 0);
        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                const TileView& tile = board.tileAt(c);
                const TileMemory& mem = memory[idx(c)];
                const bool visibleEnemyTile = tile.visible && isEnemyTile(tile);
                const bool recentEnemyTile =
                    !tile.visible && isEnemyOccupier(mem.occupier) &&
                    static_cast<int>(fullTurn) - mem.lastSeenTurn <= 4;
                if (visibleEnemyTile) {
                    if (tile.occupier >= 0 &&
                        tile.occupier < static_cast<index_t>(
                                            visibleEnemyCountByPlayer.size())) {
                        ++visibleEnemyCountByPlayer[tile.occupier];
                    }
                    duelEnemyTargetsCache.push_back(c);
                } else if (recentEnemyTile) {
                    duelEnemyTargetsCache.push_back(c);
                }
                if (visibleEnemyTile || recentEnemyTile) {
                    for (Coord d : kDirs) {
                        Coord adj = c + d;
                        if (!inside(adj)) continue;
                        const TileView& adjTile = board.tileAt(adj);
                        const TileMemory& adjMem = memory[idx(adj)];
                        const size_t adjIndex = idx(adj);
                        if (!adjTile.visible && !adjMem.everSeen &&
                            !duelFogMark[adjIndex]) {
                            duelFogMark[adjIndex] = 1;
                            duelFrontierFogCache.push_back(adj);
                        }
                    }
                }
                if (tile.occupier != id) continue;
                friendlyTilesCache.push_back(c);
                largestFriendlyArmyCache =
                    std::max(largestFriendlyArmyCache, tile.army);
                if (tile.type == TILE_CITY) ownedCitiesCache.push_back(c);
                if (tile.type == TILE_GENERAL) myGeneral = c;
            }
        }
    }

    void resetCandidateMask(index_t player) {
        auto& mask = candidateGeneralMask[player];
        std::fill(mask.begin(), mask.end(), 0);
        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                const TileMemory& mem = memory[idx(c)];
                if (!isPassable(mem.type) || mem.visible || mem.type == TILE_CITY) {
                    continue;
                }
                if (myGeneral != Coord{-1, -1} && manhattan(c, myGeneral) < 10) {
                    continue;
                }
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
                if (myGeneral != Coord{-1, -1} && manhattan(c, myGeneral) < 10) {
                    continue;
                }
                mask[idx(c)] = 1;
                heat[idx(c)] +=
                    std::max(0.0, baseScore - manhattan(c, anchor) * 2.0);
            }
        }
    }

    void decayPredictions() {
        for (index_t player = 0; player < playerCnt; ++player) {
            if (player == id) continue;
            for (double& score : predictedGeneralScore[player]) score *= 0.985;
        }
    }

    void rememberVisibleBoard() {
        decayPredictions();
        newlyVisibleEnemyTiles.assign(playerCnt, 0);

        for (index_t player = 0; player < playerCnt; ++player) {
            if (player == id) continue;
            if (knownGenerals[player] == Coord{-1, -1}) resetCandidateMask(player);
        }

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
                    auto& heat = predictedGeneralScore[tile.occupier];
                    auto& mask = candidateGeneralMask[tile.occupier];
                    std::fill(heat.begin(), heat.end(), 0.0);
                    std::fill(mask.begin(), mask.end(), 0);
                    heat[idx(c)] = 1e9;
                    mask[idx(c)] = 1;
                }

                if (tile.occupier >= 0 && tile.occupier != id && !wasVisible) {
                    newlyVisibleEnemyTiles[tile.occupier]++;
                    reinforceGeneralNeighborhood(tile.occupier, c, 18.0, 7, true);
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
                reinforceGeneralNeighborhood(tile.occupier, c, 10.0, 4, false);
            }
        }
    }

    index_t chooseEnemyPlayer() const {
        index_t best = -1;
        double bestScore = -1e100;
        for (index_t player = 0; player < playerCnt; ++player) {
            if (player == id || !aliveById[player]) continue;
            double score = 0.0;
            if (knownGenerals[player] != Coord{-1, -1}) score += 120.0;
            score += newlyVisibleEnemyTiles[player] * 8.0;
            score +=
                std::max<army_t>(0, rankById[id].army - rankById[player].army) *
                0.05;
            score +=
                std::max<pos_t>(0, rankById[id].land - rankById[player].land) *
                0.5;
            score += std::max<army_t>(0, -playerArmyDelta[player]) * 1.2;
            score += std::max<pos_t>(0, -playerLandDelta[player]) * 4.0;
            if (score > bestScore) {
                bestScore = score;
                best = player;
            }
        }
        return best;
    }

    index_t chooseLockedEnemy(index_t suggested) {
        if (suggested < 0) return suggested;
        if (lockedTargetPlayer < 0 || !aliveById[lockedTargetPlayer] ||
            fullTurn > static_cast<turn_t>(targetLockUntil)) {
            lockedTargetPlayer = suggested;
            targetLockUntil = static_cast<int>(fullTurn) + 14;
            return lockedTargetPlayer;
        }
        const double currentLead =
            rankById[id].army - rankById[lockedTargetPlayer].army;
        const double nextLead = rankById[id].army - rankById[suggested].army;
        if (suggested != lockedTargetPlayer && nextLead > currentLead + 20.0) {
            lockedTargetPlayer = suggested;
            targetLockUntil = static_cast<int>(fullTurn) + 14;
        }
        return lockedTargetPlayer;
    }

    Coord chooseTargetPlayerGeneral(index_t player) const {
        if (player < 0 || player >= playerCnt) return Coord{-1, -1};
        if (knownGenerals[player] != Coord{-1, -1}) return knownGenerals[player];
        double bestScore = -1e100;
        Coord best{-1, -1};
        const auto& heat = predictedGeneralScore[player];
        const auto& mask = candidateGeneralMask[player];
        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                const TileMemory& mem = memory[idx(c)];
                if (!isPassable(mem.type) || mem.visible || mem.type == TILE_CITY) {
                    continue;
                }
                if (!mask[idx(c)]) continue;
                double score = heat[idx(c)];
                if (myGeneral != Coord{-1, -1}) {
                    score -= manhattan(c, myGeneral) * 0.1;
                }
                if (score > bestScore) {
                    bestScore = score;
                    best = c;
                }
            }
        }
        return best;
    }

    int attackPenalty(Coord c, bool allowGeneralDive) const {
        const TileMemory& mem = memory[idx(c)];
        int penalty = 2;
        if (mem.type == TILE_SWAMP) penalty += 16;
        if (mem.type == TILE_CITY) penalty += 9;
        if (!mem.everSeen) penalty += 2;
        if (isEnemyOccupier(mem.occupier)) {
            penalty += static_cast<int>(std::min<army_t>(estimatedArmyAt(c), 100));
            if (mem.type == TILE_GENERAL && allowGeneralDive) penalty -= 20;
        } else if (mem.occupier == id) {
            penalty -= 2;
        }
        return std::max(1, penalty);
    }

    Coord strongestFriendlyTile(bool includeGeneral,
                                std::optional<Coord> near = std::nullopt,
                                int nearWeight = 0) const {
        Coord best = myGeneral;
        army_t bestArmy = -1;
        for (Coord c : friendlyTilesCache) {
            const TileView& tile = board.tileAt(c);
            if (tile.army <= 1) continue;
            if (!includeGeneral && c == myGeneral) continue;
            army_t value = tile.army;
            if (tile.type == TILE_CITY) value -= std::max<army_t>(1, tile.army / 6);
            if (c == myGeneral) value -= std::max<army_t>(3, tile.army / 7);
            if (near.has_value()) value -= manhattan(c, *near) * nearWeight;
            if (value > bestArmy) {
                bestArmy = value;
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

    bool moveLooksSafe(const Move& move) const {
        if (move.type != MoveType::MOVE_ARMY) return false;
        if (!inside(move.from) || !inside(move.to)) return false;
        const TileView& src = board.tileAt(move.from);
        if (src.occupier != id || src.army <= 1) return false;
        army_t remain = move.takeHalf ? (src.army >> 1) : 1;
        if (move.from == myGeneral) {
            army_t adjacentThreat = 0;
            for (Coord d : kDirs) {
                Coord adj = myGeneral + d;
                if (!inside(adj)) continue;
                const TileView& tile = board.tileAt(adj);
                if (isEnemyTile(tile)) {
                    adjacentThreat = std::max(adjacentThreat, tile.army);
                }
                const TileMemory& mem = memory[idx(adj)];
                if (!tile.visible && isEnemyOccupier(mem.occupier) &&
                    static_cast<int>(fullTurn) - mem.lastSeenTurn <= 4) {
                    adjacentThreat = std::max(adjacentThreat, mem.army);
                }
            }
            if (remain <= adjacentThreat) return false;
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

    CandidateMove chooseDirectCaptureMove(index_t enemy) const {
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
                const army_t margin = attack - defense;
                double score = 0.0;
                if (dst.type == TILE_GENERAL) score += 1e6;
                if (dst.type == TILE_CITY) score += 300.0 + margin * 8.0;
                if (isEnemyTile(dst)) score += 120.0 + margin * 5.0;
                if (enemy >= 0 && dst.occupier == enemy) score += 40.0;
                if (currentObjective == to) score += 25.0;
                if (dst.type == TILE_SWAMP) score -= 20.0;
                if (!dst.visible && !memory[idx(to)].everSeen) score += 6.0;
                if (score > best.score) {
                    best.valid = true;
                    best.score = score;
                    best.move = Move(MoveType::MOVE_ARMY, from, to, false);
                }
            }
        }
        return best;
    }

    std::optional<ThreatInfo> analyzeThreat(index_t enemy) {
        if (enemy < 0 || myGeneral == Coord{-1, -1}) return std::nullopt;

        std::optional<ThreatInfo> best;
        std::vector<Coord> keyTargets = {myGeneral};
        for (Coord city : ownedCitiesCache) {
            if (manhattan(city, myGeneral) <= 8) keyTargets.push_back(city);
        }

        for (Coord target : keyTargets) {
            const auto enemyPath = weightedReversePath(target, [&](Coord step) {
                int cost = 1;
                if (memory[idx(step)].type == TILE_SWAMP) cost += 5;
                if (step == myGeneral) cost = std::max(1, cost - 1);
                return cost;
            });
            if (!enemyPath.reachable) continue;

            for (pos_t x = 1; x <= height; ++x) {
                for (pos_t y = 1; y <= width; ++y) {
                    Coord c{x, y};
                    const TileView& tile = board.tileAt(c);
                    if (!isEnemyTile(tile) || tile.army <= 1) continue;

                    const int dist = enemyPath.distance(idx(c));
                    if (dist >= kInf) continue;
                    const double score = tile.army * 3.0 - dist * 12.0 +
                                         (target == myGeneral ? 150.0 : 60.0);
                    if (best.has_value() && score <= best->score) continue;

                    std::vector<Coord> route =
                        reconstructPath(c, target, enemyPath);
                    if (route.empty()) continue;

                    ThreatInfo info;
                    info.enemy = c;
                    info.target = target;
                    info.route = std::move(route);
                    info.army = tile.army;
                    info.score = score;
                    best = info;
                }
            }
        }
        return best;
    }

    bool isConversionMode(index_t enemy) const {
        if (enemy < 0 || enemy >= playerCnt) return false;
        if (fullTurn < 12) return false;
        const army_t armyLead = rankById[id].army - rankById[enemy].army;
        const pos_t landLead = rankById[id].land - rankById[enemy].land;
        return armyLead >= 20 || (armyLead >= 10 && landLead >= 8) ||
               landLead >= 14;
    }

    bool isDuelAggressionProfile() const {
        return playerCnt <= 2 && static_cast<int>(height * width) <= 500;
    }

    bool isCoreOwnedCity(Coord c) const {
        if (!inside(c) || myGeneral == Coord{-1, -1}) return false;
        const TileView& tile = board.tileAt(c);
        return tile.occupier == id && tile.type == TILE_CITY &&
               manhattan(c, myGeneral) <= 5;
    }

    bool hasVisibleEnemyPresence(index_t enemy = -1) const {
        if (enemy >= 0) {
            return enemy < static_cast<index_t>(visibleEnemyCountByPlayer.size()) &&
                   visibleEnemyCountByPlayer[enemy] > 0;
        }
        return std::any_of(visibleEnemyCountByPlayer.begin(),
                           visibleEnemyCountByPlayer.end(),
                           [](int count) { return count > 0; });
    }

    bool hasEnemyAdjacent(Coord c) const {
        for (Coord d : kDirs) {
            Coord adj = c + d;
            if (!inside(adj)) continue;
            const TileView& tile = board.tileAt(adj);
            if (isEnemyTile(tile)) return true;
            const TileMemory& mem = memory[idx(adj)];
            if (!tile.visible && isEnemyOccupier(mem.occupier) &&
                static_cast<int>(fullTurn) - mem.lastSeenTurn <= 4) {
                return true;
            }
        }
        return false;
    }

    bool pullsStrongestStackAwayFrom(Coord focus, const Move& move) const {
        if (focus == Coord{-1, -1}) return false;
        Coord strongest = strongestFriendlyTile(true, focus, 1);
        if (strongest == Coord{-1, -1} || move.from != strongest) return false;
        return manhattan(move.to, focus) > manhattan(move.from, focus);
    }

    bool shouldBypassEarlyEconomy(index_t enemy, Coord enemyGuess) {
        if (!isDuelAggressionProfile() || fullTurn < 8) return false;
        if (enemyGuess != Coord{-1, -1}) return true;
        if (hasVisibleEnemyPresence(enemy)) return true;

        Coord source = strongestFriendlyTile(true);
        if (source == Coord{-1, -1} || board.tileAt(source).army <= 1) {
            return false;
        }

        for (Coord c : duelEnemyTargetsCache) {
            if (manhattan(source, c) <= 8) return true;
        }
        return false;
    }

    int planningPenalty(Coord c, FocusMode mode, bool allowGeneralDive) const {
        int cost = attackPenalty(c, allowGeneralDive);
        const TileMemory& mem = memory[idx(c)];
        const TileView& tile = board.tileAt(c);
        switch (mode) {
            case FocusMode::ECONOMY:
                if (mem.type == TILE_SWAMP && fullTurn < 24) cost += 16;
                if (!mem.everSeen) cost = std::max(1, cost - 1);
                break;
            case FocusMode::ASSAULT:
                if (!mem.everSeen) cost = std::max(1, cost - 1);
                if (tile.occupier == id) cost = std::max(1, cost - 1);
                break;
            case FocusMode::DEFENSE:
                if (myGeneral != Coord{-1, -1} && manhattan(c, myGeneral) <= 2) {
                    cost = std::max(1, cost - 2);
                }
                break;
            case FocusMode::CONVERSION:
                if (isEnemyOccupier(mem.occupier)) cost = std::max(1, cost - 2);
                if (!tile.visible) cost += 1;
                break;
        }
        return std::max(1, cost);
    }

    army_t reserveForSource(Coord source) const {
        const TileView& tile = board.tileAt(source);
        if (source == myGeneral) {
            if (fullTurn < 10) return tile.army <= 2 ? 0 : 1;
            if (fullTurn < 20) return std::max<army_t>(2, tile.army / 6);
            return std::max<army_t>(4, tile.army / 4);
        }
        if (tile.type == TILE_CITY) {
            if (fullTurn < 12) return 1;
            return std::max<army_t>(2, tile.army / 7);
        }
        return 0;
    }

    army_t supportAlongRoute(Coord source, const std::vector<Coord>& route) const {
        army_t support = 0;
        const size_t corridorLen = std::min<size_t>(route.size(), 4);
        for (Coord friendly : friendlyTilesCache) {
            if (friendly == source) continue;
            const TileView& tile = board.tileAt(friendly);
            if (tile.army <= 1) continue;
            bool nearCorridor = false;
            for (size_t i = 0; i < corridorLen; ++i) {
                if (manhattan(friendly, route[i]) <= 1) {
                    nearCorridor = true;
                    break;
                }
            }
            if (!nearCorridor) continue;
            support += std::max<army_t>(0, tile.army - 1);
        }
        return std::min<army_t>(support, 40);
    }

    int corridorPressure(const std::vector<Coord>& route) const {
        int pressure = 0;
        const size_t corridorLen = std::min<size_t>(route.size(), 5);
        for (size_t i = 0; i < corridorLen; ++i) {
            const TileMemory& mem = memory[idx(route[i])];
            if (mem.occupier == id) {
                pressure += 2;
            } else if (isEnemyOccupier(mem.occupier)) {
                pressure -= 3;
            }
            if (!mem.everSeen) pressure += 1;
        }
        return pressure;
    }

    army_t estimateObjectiveNeed(Coord objective, int distance, FocusMode mode,
                                 bool allowGeneralDive) const {
        const TileMemory& mem = memory[idx(objective)];
        army_t need = estimatedArmyAt(objective) + 1;
        if (isEnemyOccupier(mem.occupier)) need += 1;
        if (mem.type == TILE_CITY) need += 3;
        if (mem.type == TILE_GENERAL && !allowGeneralDive) need += 4;
        if (mode == FocusMode::ASSAULT && mem.type == TILE_GENERAL) need += 2;
        if (mode == FocusMode::DEFENSE) need += 1;
        if (mode == FocusMode::CONVERSION && isEnemyOccupier(mem.occupier)) {
            need += 1;
        }
        need += static_cast<army_t>(std::max(1, distance / 3));
        return need;
    }

    int objectiveOptionLimit(FocusMode mode) const {
        const int mapArea = static_cast<int>(height * width);
        int limit = 8;
        switch (mode) {
            case FocusMode::ECONOMY:
                limit = mapArea >= 900 ? 5 : 8;
                break;
            case FocusMode::CONVERSION:
                limit = mapArea >= 900 ? 6 : 8;
                break;
            case FocusMode::ASSAULT:
                limit = mapArea >= 900 ? 4 : 6;
                break;
            case FocusMode::DEFENSE:
                limit = 4;
                break;
        }
        if (fullTurn >= 30) limit = std::max(4, limit - 1);
        if (friendlyTilesCache.size() >= 80) limit = std::max(4, limit - 1);
        return limit;
    }

    int sourceOptionLimit(FocusMode mode) const {
        const int mapArea = static_cast<int>(height * width);
        int limit = 7;
        switch (mode) {
            case FocusMode::DEFENSE: limit = 5; break;
            case FocusMode::ASSAULT: limit = 6; break;
            case FocusMode::ECONOMY:
            case FocusMode::CONVERSION: limit = 7; break;
        }
        if (mapArea >= 900) limit = std::max(4, limit - 2);
        if (fullTurn >= 30) limit = std::max(4, limit - 1);
        return limit;
    }

    std::optional<FocusPlan> buildFocusPlan(Coord objective, FocusMode mode,
                                            bool allowGeneralDive,
                                            bool allowGeneralSource = true) const {
        if (!inside(objective)) return std::nullopt;
        const auto reversePath = weightedReversePath(
            objective, [&](Coord c) {
                return planningPenalty(c, mode, allowGeneralDive);
            });
        if (!reversePath.reachable) return std::nullopt;

        FocusPlan best;
        best.objective = objective;

        std::vector<SourceCandidate> candidates;
        candidates.reserve(friendlyTilesCache.size());
        const double seedDistWeight = mode == FocusMode::DEFENSE
                                          ? 5.5
                                          : (fullTurn < 12 ? 10.0 : 7.0);

        for (Coord source : friendlyTilesCache) {
            const TileView& tile = board.tileAt(source);
            if (tile.army <= 1) continue;
            if (!allowGeneralSource && source == myGeneral) continue;
            const int dist = reversePath.distance(idx(source));
            if (dist >= kInf) continue;

            const army_t sourceArmy = tile.army;
            const army_t reserve = reserveForSource(source);
            const army_t commitArmy =
                std::max<army_t>(0, sourceArmy - 1 - reserve);
            if (commitArmy <= 0) continue;

            double seedScore = commitArmy * 8.0 - dist * seedDistWeight;
            if (tile.type == TILE_CITY) seedScore -= 6.0;
            if (source == myGeneral) seedScore += fullTurn < 10 ? 4.0 : -10.0;
            if (mode == FocusMode::DEFENSE && myGeneral != Coord{-1, -1}) {
                seedScore -= manhattan(source, myGeneral) * 1.4;
            }
            if (mode == FocusMode::CONVERSION &&
                isEnemyOccupier(memory[idx(objective)].occupier)) {
                seedScore += 12.0;
            }

            candidates.push_back(
                SourceCandidate{source, sourceArmy, commitArmy, dist, seedScore});
        }

        if (candidates.empty()) return std::nullopt;
        std::sort(candidates.begin(), candidates.end(),
                  [](const SourceCandidate& lhs, const SourceCandidate& rhs) {
                      return lhs.seedScore > rhs.seedScore;
                  });
        if (static_cast<int>(candidates.size()) > sourceOptionLimit(mode)) {
            candidates.resize(sourceOptionLimit(mode));
        }

        for (const SourceCandidate& candidate : candidates) {
            const Coord source = candidate.source;
            const TileView& tile = board.tileAt(source);
            std::vector<Coord> route =
                reconstructPath(source, objective, reversePath);
            if (route.empty()) continue;

            const army_t support = supportAlongRoute(source, route);
            const int pressure = corridorPressure(route);
            const army_t need =
                estimateObjectiveNeed(objective, static_cast<int>(route.size()),
                                      mode, allowGeneralDive);

            const double distWeight = fullTurn < 12 ? 10.0 : 7.0;
            double score =
                candidate.commitArmy * 8.0 + support * 2.2 -
                candidate.distance * distWeight + pressure * 4.0;
            if (tile.type == TILE_CITY) score -= 6.0;
            if (source == myGeneral) score += fullTurn < 10 ? 6.0 : -9.0;
            if (route.size() >= 2 && board.tileAt(route[0]).occupier == id &&
                board.tileAt(route[1]).occupier == id) {
                score += 8.0;
            }
            if (candidate.commitArmy + support >= need) {
                score += 70.0;
            } else {
                score -= (need - candidate.commitArmy) * 1.8;
            }
            if (mode == FocusMode::ECONOMY && memory[idx(objective)].type == TILE_CITY) {
                score += 25.0;
            }
            if (mode == FocusMode::CONVERSION &&
                isEnemyOccupier(memory[idx(objective)].occupier)) {
                score += 35.0;
            }
            if (mode == FocusMode::DEFENSE &&
                myGeneral != Coord{-1, -1}) {
                score -= manhattan(source, myGeneral) * 1.5;
            }

            if (score > best.sourceScore) {
                best.valid = true;
                best.source = source;
                best.route = std::move(route);
                best.sourceArmy = candidate.sourceArmy;
                best.commitArmy = candidate.commitArmy;
                best.corridorSupport = support;
                best.sourceScore = score;
                best.distance = candidate.distance;
                best.rally = candidate.commitArmy + support < need;
            }
        }

        if (!best.valid) return std::nullopt;
        return best;
    }

    bool shouldLaunchNow(index_t enemy, const FocusPlan& plan) const {
        if (!plan.valid) return false;
        const army_t need = estimateObjectiveNeed(
            plan.objective, static_cast<int>(plan.route.size()),
            FocusMode::ASSAULT, true);
        const army_t pressure =
            plan.commitArmy + plan.corridorSupport / 2;
        const bool duelAggro = isDuelAggressionProfile();
        if (pressure >= need + 2) return true;
        if (duelAggro && plan.distance <= 10 && pressure >= need - 2) {
            return true;
        }
        if (duelAggro && enemy >= 0 && knownGenerals[enemy] == plan.objective &&
            pressure >= need - 3 && largestFriendlyArmyCache >= need) {
            return true;
        }
        if (enemy >= 0 && knownGenerals[enemy] == plan.objective &&
            plan.distance <= 12 && pressure >= need - 1) {
            return true;
        }
        if (playerCnt <= 2 && fullTurn >= 10 && plan.distance <= 9 &&
            largestFriendlyArmyCache >= need + 2) {
            return true;
        }
        if (enemy >= 0 &&
            (playerArmyDelta[enemy] < -4 || playerLandDelta[enemy] < -1)) {
            return true;
        }
        return isConversionMode(enemy) && plan.distance <= 10 &&
               pressure >= need - 2;
    }

    bool shouldPersistObjective(Coord objective) const {
        if (objective == Coord{-1, -1}) return false;
        if (!inside(objective)) return false;
        return board.tileAt(objective).occupier != id;
    }

    std::vector<ObjectiveOption> bestObjectives(
        const std::function<double(Coord)>& scoreFn, int limit) const {
        std::vector<ObjectiveOption> options;
        options.reserve(height * width);
        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                double score = scoreFn(c);
                if (score <= -1e90) continue;
                options.push_back(ObjectiveOption{c, score});
            }
        }
        std::sort(options.begin(), options.end(),
                  [](const ObjectiveOption& lhs, const ObjectiveOption& rhs) {
                      return lhs.score > rhs.score;
                  });
        if (static_cast<int>(options.size()) > limit) options.resize(limit);
        return options;
    }

    std::optional<Move> chooseRallyMove(const FocusPlan& plan,
                                        FocusMode mode) const {
        if (!plan.valid || plan.source == Coord{-1, -1}) return std::nullopt;

        CandidateMove best;
        const size_t corridorLen = std::min<size_t>(plan.route.size(), 3);
        auto corridorDistance = [&](Coord c) {
            int bestDist = manhattan(c, plan.source);
            for (size_t i = 0; i < corridorLen; ++i) {
                if (board.tileAt(plan.route[i]).occupier != id) continue;
                bestDist = std::min(bestDist, manhattan(c, plan.route[i]));
            }
            return bestDist;
        };

        for (Coord from : friendlyTilesCache) {
            const TileView& src = board.tileAt(from);
            if (src.army <= 1 || from == plan.source) continue;

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
                if (!moveLooksSafe(move)) continue;

                double score =
                    (src.army - 1) * 12.0 + (before - after) * 34.0;
                if (to == plan.source) score += 60.0;
                if (src.type == TILE_CITY) score -= 18.0;
                if (from == myGeneral) score -= fullTurn < 12 ? 60.0 : 120.0;
                if (mode == FocusMode::DEFENSE && myGeneral != Coord{-1, -1}) {
                    score += std::max(
                        0, manhattan(from, myGeneral) - manhattan(to, myGeneral)) *
                             18.0;
                }
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

    bool shouldPreferRally(const FocusPlan& plan, FocusMode mode) const {
        if (!plan.valid || !plan.rally) return false;
        if (mode == FocusMode::DEFENSE) return true;
        if (isDuelAggressionProfile()) {
            if (mode == FocusMode::ASSAULT && plan.distance <= 6) return false;
            if (mode == FocusMode::CONVERSION && plan.distance <= 5 &&
                inside(plan.objective)) {
                const TileView& objectiveTile = board.tileAt(plan.objective);
                if (objectiveTile.visible && isEnemyTile(objectiveTile)) {
                    return false;
                }
            }
        }
        const int mapArea = static_cast<int>(height * width);
        return plan.distance >= 6 || fullTurn >= 14 ||
               (mapArea >= 700 && fullTurn >= 8);
    }

    std::optional<Move> chooseMoveForObjectives(
        const std::vector<ObjectiveOption>& options, FocusMode mode,
        bool allowGeneralDive, Coord* chosenObjective = nullptr) {
        std::optional<Move> bestMove;
        double bestScore = -1e100;
        Coord bestObjective{-1, -1};
        for (const auto& option : options) {
            auto plan =
                buildFocusPlan(option.target, mode, allowGeneralDive, true);
            if (!plan.has_value()) continue;

            std::optional<Move> move;
            double totalScore = option.score + plan->sourceScore;
            if (shouldPreferRally(*plan, mode)) {
                move = chooseRallyMove(*plan, mode);
                if (move.has_value()) {
                    totalScore += mode == FocusMode::ASSAULT ? 8.0 : 12.0;
                } else {
                    totalScore -= mode == FocusMode::ASSAULT ? 24.0 : 8.0;
                }
            } else {
                totalScore += 12.0;
            }
            if (!move.has_value()) {
                Move directMove(MoveType::MOVE_ARMY, plan->source,
                                plan->route.front(), false);
                if (!moveLooksSafe(directMove)) continue;
                move = directMove;
            }
            if (plan->route.size() <= 4) totalScore += 10.0;
            if (totalScore > bestScore) {
                bestScore = totalScore;
                bestMove = *move;
                bestObjective = option.target;
            }
        }
        if (bestMove.has_value()) {
            currentObjective = bestObjective;
            if (chosenObjective != nullptr) *chosenObjective = bestObjective;
        }
        return bestMove;
    }

    std::optional<Move> chooseAggressiveDuelMove(index_t enemy) {
        if (!isDuelAggressionProfile() || enemy < 0 || fullTurn < 8) {
            return std::nullopt;
        }

        const Coord enemyGuess = chooseTargetPlayerGeneral(enemy);
        const bool visibleEnemy = hasVisibleEnemyPresence(enemy);
        if (enemyGuess == Coord{-1, -1} && !visibleEnemy) {
            return std::nullopt;
        }
        if (!visibleEnemy && largestFriendlyArmyCache < 9) {
            return std::nullopt;
        }

        duelCandidateTouched.clear();
        auto pushCandidate = [&](Coord target, double score) {
            if (!inside(target)) return;
            const TileView& tile = board.tileAt(target);
            const TileMemory& mem = memory[idx(target)];
            if (!isPassable(mem.type) || tile.occupier == id) return;
            if (tile.type == TILE_SWAMP) score -= 45.0;
            if (myGeneral != Coord{-1, -1}) {
                score -= manhattan(target, myGeneral) * 0.25;
            }
            if (currentObjective == target) score += 18.0;
            const size_t node = idx(target);
            if (!duelCandidateMark[node]) {
                duelCandidateMark[node] = 1;
                duelCandidateScoreCache[node] = score;
                duelCandidateTouched.push_back(target);
            } else {
                duelCandidateScoreCache[node] =
                    std::max(duelCandidateScoreCache[node], score);
            }
        };

        if (knownGenerals[enemy] != Coord{-1, -1}) {
            pushCandidate(knownGenerals[enemy], 420.0);
        }
        if (enemyGuess != Coord{-1, -1} &&
            (visibleEnemy || largestFriendlyArmyCache >= 10)) {
            for (int dx = -4; dx <= 4; ++dx) {
                for (int dy = -4; dy <= 4; ++dy) {
                    Coord target{static_cast<pos_t>(enemyGuess.x + dx),
                                 static_cast<pos_t>(enemyGuess.y + dy)};
                    const int dist = std::abs(dx) + std::abs(dy);
                    if (dist > 4) continue;
                    pushCandidate(target, 160.0 - dist * 20.0);
                }
            }
        }

        for (Coord c : duelEnemyTargetsCache) {
            const TileView& tile = board.tileAt(c);
            if (tile.visible && isEnemyTile(tile)) {
                const army_t enemyArmy = estimatedArmyAt(c);
                double score = tile.type == TILE_CITY
                                   ? 320.0 - enemyArmy * 3.2
                                   : 175.0 +
                                         std::min<army_t>(enemyArmy, 60) * 1.2;
                if (largestFriendlyArmyCache >= enemyArmy + 2) {
                    score += 28.0;
                }
                pushCandidate(c, score);
            } else {
                pushCandidate(c, 130.0);
            }
        }
        for (Coord fog : duelFrontierFogCache) {
            pushCandidate(fog, 110.0);
        }

        std::vector<ObjectiveOption> options;
        options.reserve(std::min<size_t>(12, duelCandidateTouched.size()));
        for (Coord c : duelCandidateTouched) {
            const size_t node = idx(c);
            const double score = duelCandidateScoreCache[node];
            if (score > -1e90) options.push_back(ObjectiveOption{c, score});
            duelCandidateMark[node] = 0;
            duelCandidateScoreCache[node] = -1e100;
        }
        duelCandidateTouched.clear();
        std::sort(options.begin(), options.end(),
                  [](const ObjectiveOption& lhs, const ObjectiveOption& rhs) {
                      return lhs.score > rhs.score;
                  });
        if (static_cast<int>(options.size()) > 3) options.resize(3);

        if (options.empty()) return std::nullopt;
        return chooseMoveForObjectives(options, FocusMode::ASSAULT, true);
    }

    std::optional<Move> chooseDefensiveMove(index_t enemy) {
        if (enemy < 0 || myGeneral == Coord{-1, -1} || fullTurn < 6) {
            return std::nullopt;
        }

        auto threat = analyzeThreat(enemy);
        if (!threat.has_value()) return std::nullopt;
        const bool duelAggro = isDuelAggressionProfile();
        const army_t generalArmy = board.tileAt(myGeneral).army;
        const bool urgentGeneral =
            threat->target == myGeneral &&
            (threat->route.size() <= (duelAggro ? 4 : 5) ||
             threat->army >= generalArmy + (duelAggro ? 1 : 2));
        const bool urgentCity =
            threat->target != myGeneral &&
            (duelAggro ? isCoreOwnedCity(threat->target) : true) &&
            threat->route.size() <= (duelAggro ? 3 : 4) &&
            threat->army >= estimatedArmyAt(threat->target);
        if (!urgentGeneral && !urgentCity) return std::nullopt;
        const Coord enemyFocus = chooseTargetPlayerGeneral(enemy);

        for (Coord d : kDirs) {
            Coord adj = threat->enemy + d;
            if (!inside(adj)) continue;
            const TileView& tile = board.tileAt(adj);
            if (tile.occupier != id || tile.army <= threat->army + 1) continue;
            Move move(MoveType::MOVE_ARMY, adj, threat->enemy, false);
            if (duelAggro && !urgentGeneral &&
                pullsStrongestStackAwayFrom(enemyFocus, move)) {
                continue;
            }
            if (moveLooksSafe(move)) return move;
        }

        std::vector<ObjectiveOption> objectives;
        objectives.push_back(ObjectiveOption{threat->target, 160.0});
        const size_t routeLen = threat->route.size();
        if (routeLen >= 2) {
            objectives.push_back(
                ObjectiveOption{threat->route[routeLen - 2], 148.0});
        }
        if (routeLen >= 3) {
            objectives.push_back(
                ObjectiveOption{threat->route[routeLen - 3], 138.0});
        }
        if (inside(threat->enemy)) {
            objectives.push_back(ObjectiveOption{threat->enemy, 132.0});
        }
        auto move = chooseMoveForObjectives(objectives, FocusMode::DEFENSE, true);
        if (move.has_value() && duelAggro && !urgentGeneral &&
            pullsStrongestStackAwayFrom(enemyFocus, *move)) {
            return std::nullopt;
        }
        return move;
    }

    bool canCommitToCity(Coord city, const FocusPlan& plan) const {
        if (city == Coord{-1, -1} || !plan.valid) return false;
        const TileView& cityTile = board.tileAt(city);
        if (cityTile.type != TILE_CITY || cityTile.occupier == id) return true;
        const army_t need = estimateObjectiveNeed(
            city, static_cast<int>(plan.route.size()), FocusMode::ECONOMY, true);
        return plan.commitArmy + plan.corridorSupport >= need;
    }

    std::optional<Move> chooseSimpleEconomicMove(index_t enemy) {
        Coord source = strongestFriendlyTile(true);
        if (source == Coord{-1, -1} || board.tileAt(source).army <= 1) {
            return std::nullopt;
        }

        const int mapArea = static_cast<int>(height * width);
        const bool duelAggro = isDuelAggressionProfile() && fullTurn >= 8;
        const auto path = weightedPath(source, [&](Coord c) {
            int cost = attackPenalty(c, true);
            if (memory[idx(c)].type == TILE_SWAMP && fullTurn < 24) cost += 25;
            return cost;
        });

        Coord bestTarget{-1, -1};
        double bestScore = -1e100;
        const pos_t cx = (height + 1) / 2;
        const pos_t cy = (width + 1) / 2;
        const Coord enemyGuess = chooseTargetPlayerGeneral(enemy);

        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord target{x, y};
                const TileView& tile = board.tileAt(target);
                const TileMemory& mem = memory[idx(target)];
                if (!isPassable(memory[idx(target)].type) || tile.occupier == id) {
                    continue;
                }
                if (path.distance(idx(target)) >= kInf) continue;
                double score = -path.distance(idx(target)) * 9.0;
                score -= manhattan(target, Coord{cx, cy}) * 1.7;
                if (!mem.everSeen) score += duelAggro ? 42.0 : 80.0;
                if (!tile.visible) score += duelAggro ? 12.0 : 30.0;
                if (tile.occupier == enemy) score += duelAggro ? 82.0 : 55.0;
                if (tile.type == TILE_CITY) {
                    score += 260.0 - estimatedArmyAt(target) * 3.5;
                    if (mapArea > 700 && fullTurn < 18) score -= 120.0;
                    if (fullTurn < 12) score -= 80.0;
                } else if (tile.type == TILE_SWAMP) {
                    score -= 50.0;
                } else if (isEnemyTile(tile) && fullTurn < 10 &&
                           estimatedArmyAt(target) > 1) {
                    score -= duelAggro ? 20.0 : 75.0;
                } else {
                    score += duelAggro ? 8.0 : 24.0;
                }
                if (enemyGuess != Coord{-1, -1}) {
                    score +=
                        std::max(0.0, 36.0 - manhattan(target, enemyGuess) * 1.1);
                }
                if (duelAggro) {
                    if (isEnemyTile(tile)) score += 48.0;
                    if (enemyGuess != Coord{-1, -1} &&
                        manhattan(target, enemyGuess) <= 4) {
                        score += std::max(
                            0.0, 26.0 - manhattan(target, enemyGuess) * 4.0);
                    }
                    const bool remoteNeutral =
                        tile.occupier == -1 && !isEnemyOccupier(mem.occupier) &&
                        tile.type != TILE_CITY &&
                        (enemyGuess == Coord{-1, -1} ||
                         manhattan(target, enemyGuess) > 4);
                    if (remoteNeutral) score -= tile.visible ? 15.0 : 22.0;
                }
                if (myGeneral != Coord{-1, -1} && fullTurn < 10) {
                    score += std::max(
                        0.0, 30.0 - manhattan(target, myGeneral) * 5.0);
                }
                if (score > bestScore) {
                    bestScore = score;
                    bestTarget = target;
                }
            }
        }

        if (bestTarget == Coord{-1, -1}) return std::nullopt;
        std::vector<Coord> route = reconstructPath(source, bestTarget, path);
        if (route.empty()) return std::nullopt;
        Move move(MoveType::MOVE_ARMY, source, route.front(), false);
        if (!moveLooksSafe(move)) return std::nullopt;
        currentObjective = bestTarget;
        return move;
    }

    std::optional<Move> chooseEconomicMove(index_t enemy) {
        const int mapArea = static_cast<int>(height * width);
        const Coord enemyGuess = chooseTargetPlayerGeneral(enemy);
        const bool conversion = isConversionMode(enemy);
        const bool duelAggro = isDuelAggressionProfile() && fullTurn >= 8;
        if (fullTurn < 10 || friendlyTilesCache.size() <= 6) {
            if (auto simple = chooseSimpleEconomicMove(enemy)) return simple;
        }
        auto options = bestObjectives(
            [&](Coord target) -> double {
                const TileView& tile = board.tileAt(target);
                const TileMemory& mem = memory[idx(target)];
                if (!isPassable(mem.type) || tile.occupier == id) return -1e100;
                double score = 0.0;
                if (!mem.everSeen) {
                    score += conversion ? 20.0 : (duelAggro ? 38.0 : 75.0);
                }
                if (!tile.visible) {
                    score += conversion ? 6.0 : (duelAggro ? 10.0 : 26.0);
                }
                if (tile.occupier == enemy) {
                    score += conversion ? 80.0 : (duelAggro ? 96.0 : 52.0);
                }
                if (tile.type == TILE_CITY) {
                    score += 240.0 - estimatedArmyAt(target) * 3.3;
                    if (mapArea > 700 && fullTurn < 18) score -= 90.0;
                    if (fullTurn < 12) score -= 60.0;
                } else if (tile.type == TILE_SWAMP) {
                    score -= conversion ? 18.0 : 50.0;
                } else if (isEnemyTile(tile)) {
                    score += conversion ? 110.0 : (duelAggro ? 92.0 : 38.0);
                    if (fullTurn < 10 && estimatedArmyAt(target) > 1) {
                        score -= duelAggro ? 18.0 : 70.0;
                    }
                } else {
                    score += conversion ? 10.0 : (duelAggro ? 4.0 : 22.0);
                }
                if (enemyGuess != Coord{-1, -1}) {
                    score += std::max(0.0,
                                      (conversion ? 42.0 : 32.0) -
                                          manhattan(target, enemyGuess) * 1.2);
                }
                if (duelAggro) {
                    if (enemyGuess != Coord{-1, -1} &&
                        manhattan(target, enemyGuess) <= 4) {
                        score += std::max(
                            0.0, 28.0 - manhattan(target, enemyGuess) * 4.0);
                    }
                    const bool remoteNeutral =
                        tile.occupier == -1 && !isEnemyOccupier(mem.occupier) &&
                        tile.type != TILE_CITY &&
                        (enemyGuess == Coord{-1, -1} ||
                         manhattan(target, enemyGuess) > 4);
                    if (remoteNeutral) score -= tile.visible ? 16.0 : 22.0;
                }
                if (myGeneral != Coord{-1, -1}) {
                    score -= manhattan(target, myGeneral) * (conversion ? 0.15 : 0.4);
                    if (fullTurn < 10) {
                        score += std::max(
                            0.0, 25.0 - manhattan(target, myGeneral) * 4.0);
                    }
                }
                return score;
            },
            objectiveOptionLimit(conversion ? FocusMode::CONVERSION
                                            : FocusMode::ECONOMY));

        std::optional<Move> bestMove;
        double bestScore = -1e100;
        Coord bestObjective{-1, -1};
        for (const auto& option : options) {
            auto plan = buildFocusPlan(option.target,
                                       conversion ? FocusMode::CONVERSION
                                                  : FocusMode::ECONOMY,
                                       true, true);
            if (!plan.has_value()) continue;
            if (memory[idx(option.target)].type == TILE_CITY &&
                !canCommitToCity(option.target, *plan)) {
                continue;
            }

            std::optional<Move> move;
            double score = option.score + plan->sourceScore;
            if (memory[idx(option.target)].type == TILE_CITY) score += 15.0;
            if (shouldPreferRally(*plan,
                                  conversion ? FocusMode::CONVERSION
                                             : FocusMode::ECONOMY)) {
                move = chooseRallyMove(
                    *plan, conversion ? FocusMode::CONVERSION
                                      : FocusMode::ECONOMY);
                if (move.has_value()) {
                    score += conversion ? 4.0 : 10.0;
                } else {
                    score -= conversion ? 2.0 : 8.0;
                }
            }
            if (!move.has_value()) {
                Move directMove(MoveType::MOVE_ARMY, plan->source,
                                plan->route.front(), false);
                if (!moveLooksSafe(directMove)) continue;
                move = directMove;
            }
            if (score > bestScore) {
                bestScore = score;
                bestMove = *move;
                bestObjective = option.target;
            }
        }
        if (bestMove.has_value()) {
            currentObjective = bestObjective;
        }
        if (!bestMove.has_value()) return chooseSimpleEconomicMove(enemy);
        return bestMove;
    }

    std::optional<Move> chooseObjectiveMove(index_t enemy, Coord objective,
                                            bool launchMode) {
        if (objective == Coord{-1, -1}) return std::nullopt;
        const FocusMode mode =
            isConversionMode(enemy) && !launchMode ? FocusMode::CONVERSION
                                                   : FocusMode::ASSAULT;
        auto plan = buildFocusPlan(
            objective, mode, true, true);
        if (!plan.has_value()) return std::nullopt;

        std::optional<Move> move;
        const army_t need = estimateObjectiveNeed(
            objective, static_cast<int>(plan->route.size()), mode, true);
        const bool duelDirectNow =
            isDuelAggressionProfile() && plan->commitArmy >= need - 2;
        if (!launchMode && !duelDirectNow && shouldPreferRally(*plan, mode)) {
            move = chooseRallyMove(*plan, mode);
        }
        if (!move.has_value()) {
            Move directMove(MoveType::MOVE_ARMY, plan->source, plan->route.front(),
                            false);
            if (!moveLooksSafe(directMove)) return std::nullopt;
            move = directMove;
        }
        currentObjective = objective;
        return move;
    }

    Coord chooseExpansionTarget(index_t enemy) {
        Coord enemyGuess = chooseTargetPlayerGeneral(enemy);
        const bool conversion = isConversionMode(enemy);
        const bool duelAggro = isDuelAggressionProfile() && fullTurn >= 8;
        double bestScore = -1e100;
        Coord best{-1, -1};
        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                const TileView& tile = board.tileAt(c);
                const TileMemory& mem = memory[idx(c)];
                if (!isPassable(mem.type) || tile.occupier == id) continue;
                double score = 0.0;
                if (!mem.everSeen) {
                    score += conversion ? 18.0 : (duelAggro ? 46.0 : 85.0);
                }
                if (!tile.visible) {
                    score += conversion ? 8.0 : (duelAggro ? 8.0 : 25.0);
                }
                if (tile.type == TILE_CITY) {
                    score += 190.0 - estimatedArmyAt(c) * 3.0;
                } else if (tile.type == TILE_SWAMP) {
                    score -= conversion ? 15.0 : 40.0;
                } else if (isEnemyTile(tile)) {
                    score += conversion ? 125.0 : (duelAggro ? 138.0 : 90.0);
                    score += estimatedArmyAt(c) * 0.25;
                } else {
                    score += conversion ? 10.0 : (duelAggro ? 4.0 : 18.0);
                }
                if (enemyGuess != Coord{-1, -1}) {
                    score += std::max(0.0,
                                      (conversion ? 55.0 : 44.0) -
                                          manhattan(c, enemyGuess) * 1.4);
                }
                if (duelAggro) {
                    if (enemyGuess != Coord{-1, -1} && manhattan(c, enemyGuess) <= 4) {
                        score += std::max(0.0,
                                          24.0 - manhattan(c, enemyGuess) * 4.0);
                    }
                    const bool remoteNeutral =
                        tile.occupier == -1 && !isEnemyOccupier(mem.occupier) &&
                        tile.type != TILE_CITY &&
                        (enemyGuess == Coord{-1, -1} ||
                         manhattan(c, enemyGuess) > 4);
                    if (remoteNeutral) score -= tile.visible ? 18.0 : 24.0;
                }
                if (myGeneral != Coord{-1, -1}) {
                    score -= manhattan(c, myGeneral) * (conversion ? 0.2 : 0.5);
                }
                if (score > bestScore) {
                    bestScore = score;
                    best = c;
                }
            }
        }
        return best;
    }

    std::optional<Move> fallbackMove(index_t enemy) {
        CandidateMove direct = chooseDirectCaptureMove(enemy);
        if (direct.valid && moveLooksSafe(direct.move)) return direct.move;
        Coord source = strongestFriendlyTile(true);
        if (source == Coord{-1, -1}) return std::nullopt;
        Coord best{-1, -1};
        double bestScore = -1e100;
        for (Coord d : kDirs) {
            Coord to = source + d;
            if (!inside(to)) continue;
            const TileView& tile = board.tileAt(to);
            if (isImpassableTile(tile.type) || shouldBlockOscillation(source, to)) {
                continue;
            }
            double score = 0.0;
            if (!memory[idx(to)].everSeen) score += 30.0;
            if (!tile.visible) score += 16.0;
            if (tile.type == TILE_CITY) score += 50.0 - estimatedArmyAt(to);
            if (tile.type == TILE_SWAMP) score -= 15.0;
            if (tile.occupier != id) score += 22.0;
            if (isConversionMode(enemy) && isEnemyTile(tile)) score += 25.0;
            if (score > bestScore) {
                bestScore = score;
                best = to;
            }
        }
        if (best == Coord{-1, -1}) return std::nullopt;
        Move move(MoveType::MOVE_ARMY, source, best, false);
        if (!moveLooksSafe(move)) return std::nullopt;
        return move;
    }

    std::optional<Move> selectStrategicMove() {
        index_t enemy = chooseLockedEnemy(chooseEnemyPlayer());
        const bool duelAggro = isDuelAggressionProfile();
        const Coord enemyGeneral = chooseTargetPlayerGeneral(enemy);
        CandidateMove direct = chooseDirectCaptureMove(enemy);
        if (direct.valid && moveLooksSafe(direct.move)) {
            const TileView& target = board.tileAt(direct.move.to);
            const army_t margin =
                board.tileAt(direct.move.from).army - 1 -
                estimatedArmyAt(direct.move.to);
            const bool highLeverage =
                target.type == TILE_GENERAL || target.type == TILE_CITY ||
                (isEnemyTile(target) && margin >= 4);
            if (highLeverage && direct.score > 180.0) return direct.move;
        }

        if (auto defense = chooseDefensiveMove(enemy)) return defense;

        if (auto duelAttack = chooseAggressiveDuelMove(enemy)) return duelAttack;

        const int mapArea = static_cast<int>(height * width);
        int econWindow = mapArea <= 200 ? 13 : (mapArea >= 700 ? 18 : 15);
        if (duelAggro) {
            econWindow =
                (!hasVisibleEnemyPresence(enemy) && enemyGeneral == Coord{-1, -1})
                    ? 10
                    : 8;
        } else {
            if (playerCnt <= 2) econWindow = std::max(10, econWindow - 2);
            if (largestFriendlyArmyCache >= 18) {
                econWindow = std::max(10, econWindow - 2);
            }
        }
        const bool bypassEarlyEconomy = shouldBypassEarlyEconomy(enemy, enemyGeneral);
        if (((fullTurn < econWindow && !bypassEarlyEconomy) ||
             isConversionMode(enemy)) &&
            !(duelAggro && bypassEarlyEconomy && !isConversionMode(enemy))) {
            if (auto eco = chooseEconomicMove(enemy)) return eco;
        }

        if (enemyGeneral != Coord{-1, -1}) {
            auto assaultPlan =
                buildFocusPlan(enemyGeneral, FocusMode::ASSAULT, true, true);
            if (assaultPlan.has_value() &&
                (shouldLaunchNow(enemy, *assaultPlan) ||
                 board.tileAt(assaultPlan->source).army >=
                     estimatedArmyAt(enemyGeneral) + 10)) {
                lockedObjective = enemyGeneral;
                objectiveLockUntil = static_cast<int>(fullTurn) + 14;
                Move move(MoveType::MOVE_ARMY, assaultPlan->source,
                          assaultPlan->route.front(), false);
                if (moveLooksSafe(move)) {
                    currentObjective = enemyGeneral;
                    return move;
                }
            }
        }

        if (lockedObjective != Coord{-1, -1} &&
            fullTurn <= static_cast<turn_t>(objectiveLockUntil) &&
            shouldPersistObjective(lockedObjective)) {
            if (auto attack = chooseObjectiveMove(enemy, lockedObjective, false)) {
                return attack;
            }
        }

        if (auto eco = chooseEconomicMove(enemy)) return eco;

        Coord expansion = chooseExpansionTarget(enemy);
        if (expansion != Coord{-1, -1}) {
            currentObjective = expansion;
            if (auto move = chooseObjectiveMove(enemy, expansion, false)) {
                return move;
            }
        }

        if (direct.valid && moveLooksSafe(direct.move)) return direct.move;
        return fallbackMove(enemy);
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
        predictedGeneralScore.assign(playerCnt, std::vector<double>(total, 0.0));
        candidateGeneralMask.assign(playerCnt, std::vector<uint8_t>(total, 0));
        newlyVisibleEnemyTiles.assign(playerCnt, 0);
        rankById.assign(playerCnt, RankItem{});
        prevRankById.clear();
        aliveById.assign(playerCnt, true);
        playerArmyDelta.assign(playerCnt, 0);
        playerLandDelta.assign(playerCnt, 0);

        friendlyTilesCache.clear();
        ownedCitiesCache.clear();
        duelEnemyTargetsCache.clear();
        duelFrontierFogCache.clear();
        visibleEnemyCountByPlayer.assign(playerCnt, 0);
        duelFogMark.assign(total, 0);
        duelEnemyTargetsCache.reserve(total);
        duelFrontierFogCache.reserve(total);
        duelCandidateScoreCache.assign(total, -1e100);
        duelCandidateMark.assign(total, 0);
        duelCandidateTouched.clear();
        duelCandidateTouched.reserve(total);
        largestFriendlyArmyCache = 0;
        myGeneral = Coord{-1, -1};
        currentObjective = Coord{-1, -1};
        lockedObjective = Coord{-1, -1};
        objectiveLockUntil = -1;
        lockedTargetPlayer = -1;
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

        std::optional<Move> candidate = selectStrategicMove();
        if (!candidate.has_value() || !moveLooksSafe(*candidate)) {
            candidate = fallbackMove(chooseEnemyPlayer());
        }
        if (!candidate.has_value()) return;

        pushRecentMove(candidate->from, candidate->to);
        moveQueue.push_back(*candidate);
    }
};

static BotRegistrar<KutuBot> kutu_bot_reg("KutuBot");

#endif  // LGEN_BOTS_KUTUBOT
