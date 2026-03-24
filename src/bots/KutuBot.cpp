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
    std::mt19937 rng{bot_random::makeBotRng(0x4B757475ULL)};

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
        largestFriendlyArmyCache = 0;
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

        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                const TileView& tile = board.tileAt(c);
                if (!isEnemyTile(tile) || tile.army <= 1) continue;
                for (Coord target : keyTargets) {
                    const auto enemyPath = weightedPath(c, [&](Coord step) {
                        int cost = 1;
                        if (memory[idx(step)].type == TILE_SWAMP) cost += 5;
                        if (step == myGeneral) cost = std::max(1, cost - 1);
                        return cost;
                    });
                    if (enemyPath.distance(idx(target)) >= kInf) continue;
                    std::vector<Coord> route =
                        reconstructPath(c, target, enemyPath);
                    if (route.empty()) continue;
                    ThreatInfo info;
                    info.enemy = c;
                    info.target = target;
                    info.route = std::move(route);
                    info.army = tile.army;
                    info.score =
                        tile.army * 3.0 - enemyPath.distance(idx(target)) * 12.0 +
                        (target == myGeneral ? 150.0 : 60.0);
                    if (!best.has_value() || info.score > best->score) {
                        best = info;
                    }
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

        for (Coord source : friendlyTilesCache) {
            const TileView& tile = board.tileAt(source);
            if (tile.army <= 1) continue;
            if (!allowGeneralSource && source == myGeneral) continue;
            const int dist = reversePath.distance(idx(source));
            if (dist >= kInf) continue;
            std::vector<Coord> route =
                reconstructPath(source, objective, reversePath);
            if (route.empty()) continue;

            const army_t sourceArmy = tile.army;
            const army_t reserve = reserveForSource(source);
            const army_t commitArmy =
                std::max<army_t>(0, sourceArmy - 1 - reserve);
            if (commitArmy <= 0) continue;

            const army_t support = supportAlongRoute(source, route);
            const int pressure = corridorPressure(route);
            const army_t need =
                estimateObjectiveNeed(objective, static_cast<int>(route.size()),
                                      mode, allowGeneralDive);

            const double distWeight = fullTurn < 12 ? 10.0 : 7.0;
            double score = commitArmy * 8.0 + support * 2.2 - dist * distWeight +
                           pressure * 4.0;
            if (tile.type == TILE_CITY) score -= 6.0;
            if (source == myGeneral) score += fullTurn < 10 ? 6.0 : -9.0;
            if (route.size() >= 2 && board.tileAt(route[0]).occupier == id &&
                board.tileAt(route[1]).occupier == id) {
                score += 8.0;
            }
            if (commitArmy + support >= need) {
                score += 70.0;
            } else {
                score -= (need - commitArmy) * 1.8;
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
                best.sourceArmy = sourceArmy;
                best.commitArmy = commitArmy;
                best.corridorSupport = support;
                best.sourceScore = score;
                best.distance = dist;
                best.rally = commitArmy + support < need;
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
        if (pressure >= need + 2) return true;
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
            Move move(MoveType::MOVE_ARMY, plan->source, plan->route.front(),
                      false);
            if (!moveLooksSafe(move)) continue;
            double totalScore = option.score + plan->sourceScore;
            if (plan->rally) {
                totalScore -=
                    mode == FocusMode::ASSAULT ? 24.0 : 8.0;
            } else {
                totalScore += 12.0;
            }
            if (plan->route.size() <= 4) totalScore += 10.0;
            if (totalScore > bestScore) {
                bestScore = totalScore;
                bestMove = move;
                bestObjective = option.target;
            }
        }
        if (bestMove.has_value()) {
            currentObjective = bestObjective;
            if (chosenObjective != nullptr) *chosenObjective = bestObjective;
        }
        return bestMove;
    }

    std::optional<Move> chooseDefensiveMove(index_t enemy) {
        if (enemy < 0 || myGeneral == Coord{-1, -1} || fullTurn < 6) {
            return std::nullopt;
        }

        auto threat = analyzeThreat(enemy);
        if (!threat.has_value()) return std::nullopt;
        const bool urgentGeneral =
            threat->target == myGeneral &&
            (threat->route.size() <= 5 ||
             threat->army >= board.tileAt(myGeneral).army + 2);
        const bool urgentCity =
            threat->target != myGeneral && threat->route.size() <= 4 &&
            threat->army >= estimatedArmyAt(threat->target);
        if (!urgentGeneral && !urgentCity) return std::nullopt;

        for (Coord d : kDirs) {
            Coord adj = threat->enemy + d;
            if (!inside(adj)) continue;
            const TileView& tile = board.tileAt(adj);
            if (tile.occupier != id || tile.army <= threat->army + 1) continue;
            Move move(MoveType::MOVE_ARMY, adj, threat->enemy, false);
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
        return chooseMoveForObjectives(objectives, FocusMode::DEFENSE, true);
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
                if (!isPassable(memory[idx(target)].type) || tile.occupier == id) {
                    continue;
                }
                if (path.distance(idx(target)) >= kInf) continue;
                double score = -path.distance(idx(target)) * 9.0;
                score -= manhattan(target, Coord{cx, cy}) * 1.7;
                if (!memory[idx(target)].everSeen) score += 80.0;
                if (!tile.visible) score += 30.0;
                if (tile.occupier == enemy) score += 55.0;
                if (tile.type == TILE_CITY) {
                    score += 260.0 - estimatedArmyAt(target) * 3.5;
                    if (mapArea > 700 && fullTurn < 18) score -= 120.0;
                    if (fullTurn < 12) score -= 80.0;
                } else if (tile.type == TILE_SWAMP) {
                    score -= 50.0;
                } else if (isEnemyTile(tile) && fullTurn < 10 &&
                           estimatedArmyAt(target) > 1) {
                    score -= 75.0;
                } else {
                    score += 24.0;
                }
                if (enemyGuess != Coord{-1, -1}) {
                    score +=
                        std::max(0.0, 36.0 - manhattan(target, enemyGuess) * 1.1);
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
        if (fullTurn < 10 || friendlyTilesCache.size() <= 6) {
            if (auto simple = chooseSimpleEconomicMove(enemy)) return simple;
        }
        auto options = bestObjectives(
            [&](Coord target) -> double {
                const TileView& tile = board.tileAt(target);
                const TileMemory& mem = memory[idx(target)];
                if (!isPassable(mem.type) || tile.occupier == id) return -1e100;
                double score = 0.0;
                if (!mem.everSeen) score += conversion ? 20.0 : 75.0;
                if (!tile.visible) score += conversion ? 6.0 : 26.0;
                if (tile.occupier == enemy) score += conversion ? 80.0 : 52.0;
                if (tile.type == TILE_CITY) {
                    score += 240.0 - estimatedArmyAt(target) * 3.3;
                    if (mapArea > 700 && fullTurn < 18) score -= 90.0;
                    if (fullTurn < 12) score -= 60.0;
                } else if (tile.type == TILE_SWAMP) {
                    score -= conversion ? 18.0 : 50.0;
                } else if (isEnemyTile(tile)) {
                    score += conversion ? 110.0 : 38.0;
                    if (fullTurn < 10 && estimatedArmyAt(target) > 1) score -= 70.0;
                } else {
                    score += conversion ? 10.0 : 22.0;
                }
                if (enemyGuess != Coord{-1, -1}) {
                    score += std::max(0.0,
                                      (conversion ? 42.0 : 32.0) -
                                          manhattan(target, enemyGuess) * 1.2);
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
            8);

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
            Move move(MoveType::MOVE_ARMY, plan->source, plan->route.front(),
                      false);
            if (!moveLooksSafe(move)) continue;
            double score = option.score + plan->sourceScore;
            if (memory[idx(option.target)].type == TILE_CITY) score += 15.0;
            if (plan->rally) score -= conversion ? 2.0 : 8.0;
            if (score > bestScore) {
                bestScore = score;
                bestMove = move;
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
        auto plan = buildFocusPlan(
            objective,
            isConversionMode(enemy) && !launchMode ? FocusMode::CONVERSION
                                                   : FocusMode::ASSAULT,
            true, true);
        if (!plan.has_value()) return std::nullopt;
        Move move(MoveType::MOVE_ARMY, plan->source, plan->route.front(), false);
        if (!moveLooksSafe(move)) return std::nullopt;
        currentObjective = objective;
        return move;
    }

    Coord chooseExpansionTarget(index_t enemy) {
        Coord enemyGuess = chooseTargetPlayerGeneral(enemy);
        const bool conversion = isConversionMode(enemy);
        double bestScore = -1e100;
        Coord best{-1, -1};
        for (pos_t x = 1; x <= height; ++x) {
            for (pos_t y = 1; y <= width; ++y) {
                Coord c{x, y};
                const TileView& tile = board.tileAt(c);
                const TileMemory& mem = memory[idx(c)];
                if (!isPassable(mem.type) || tile.occupier == id) continue;
                double score = 0.0;
                if (!mem.everSeen) score += conversion ? 18.0 : 85.0;
                if (!tile.visible) score += conversion ? 8.0 : 25.0;
                if (tile.type == TILE_CITY) {
                    score += 190.0 - estimatedArmyAt(c) * 3.0;
                } else if (tile.type == TILE_SWAMP) {
                    score -= conversion ? 15.0 : 40.0;
                } else if (isEnemyTile(tile)) {
                    score += conversion ? 125.0 : 90.0;
                    score += estimatedArmyAt(c) * 0.25;
                } else {
                    score += conversion ? 10.0 : 18.0;
                }
                if (enemyGuess != Coord{-1, -1}) {
                    score += std::max(0.0,
                                      (conversion ? 55.0 : 44.0) -
                                          manhattan(c, enemyGuess) * 1.4);
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

        const int mapArea = static_cast<int>(height * width);
        const int econWindow =
            mapArea <= 200 ? 14 : (mapArea >= 700 ? 20 : 16);
        if (fullTurn < econWindow || isConversionMode(enemy)) {
            if (auto eco = chooseEconomicMove(enemy)) return eco;
        }

        Coord enemyGeneral = chooseTargetPlayerGeneral(enemy);
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
