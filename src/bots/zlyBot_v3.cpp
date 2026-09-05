// Copyright (C) 2026 AppOfficer
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file zlyBot_v2.2.cpp
 *
 * ZlyBot v2.2
 *
 * @author AppOfficer
 */

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <queue>
#include <string>

#include "core/bot.h"
#include "core/game.hpp"

// Uncomment the following line to enable debug logging
// #define ZLYBOT_V3_LOG

#ifdef ZLYBOT_V3_LOG
#define ZLYBOT_LOG(msg)                                               \
    do {                                                              \
        if (logFile.is_open())                                        \
            logFile << "[ZlyBot_v3 id=" << id << "] " << msg << "\n"; \
    } while (0)
#else
#define ZLYBOT_LOG(msg) \
    do {                \
    } while (0)
#endif

class ZlyBot_v3 : public BasicBot {
   private:
    using value_t = intmax_t;
    constexpr static pos_t DIST_INF = 1048576;
    constexpr static int64_t INF = 10'000'000'000'000'000LL;
    constexpr static Coord delta[] = {{-1, 0}, {0, -1}, {1, 0}, {0, 1}};

    enum class BotMode { ATTACK, EXPLORE, DEFEND };
    BotMode mode;

    pos_t height, width, W;
    index_t playerCnt;
    index_t id, team;
    std::vector<index_t> teamIds;
    config::Config config;
    army_t totalArmy;
    pos_t totalLand;

    turn_t halfTurn, turn;

    BoardView board;
    std::vector<RankItem> rank;

    inline bool inSameTeam(index_t anotherPlayer) const {
        if (anotherPlayer == -1) return false;
        return teamIds[anotherPlayer] == team;
    }
    inline bool inSameTeam(index_t player1, index_t player2) const {
        if (player1 == -1 || player2 == -1) return player1 == player2;
        return teamIds[player1] == teamIds[player2];
    }

    inline pos_t idx(pos_t x, pos_t y) const { return x * (width + 2) + y; }
    inline pos_t idx(Coord pos) const { return idx(pos.x, pos.y); }

    Coord lastTarget;
    army_t lastMovedArmy;

    struct RouteNode {
        // constants
        value_t totalArmy;
        // core values
        Coord pos;
        value_t dist;
        value_t friendArmy;
        value_t oppoArmy;
        value_t typeCost;
        // caches
        value_t distPenalty;
        inline value_t united() const {
            return /* -dist + distPenalty + */
                   // (friendArmy > oppoArmy * 20 ? totalArmy
                   //                             : (friendArmy - oppoArmy)) -
                (friendArmy - oppoArmy) - typeCost;
        }
    };
    struct RouteNodeLess {
        inline bool operator()(const RouteNode& lhs,
                               const RouteNode& rhs) const {
            return lhs.united() < rhs.united();
        }
    };
    struct RouteNodeGreater {
        inline bool operator()(const RouteNode& lhs,
                               const RouteNode& rhs) const {
            return lhs.united() > rhs.united();
        }
    };
    constexpr static RouteNodeLess routeComparerLess{};
    constexpr static RouteNodeGreater routeComparerGreater{};
    constexpr static RouteNode ROUTE_INF =
        RouteNode{0, {-1, -1}, DIST_INF, -INF, INF, 0};

    struct TileInfo {
        RouteNode routeDp;
        bool routeVis;
        tile_type_e type = tile_type_e(-1);
        index_t occupier = -1;
        army_t army = 0;
        bool visible = false;
        value_t visionValue;
        value_t dangerCoeff;
        bool isSeenBefore;
    };

    std::vector<TileInfo> memory;

    inline TileInfo& tileAt(pos_t x, pos_t y) { return memory[idx(x, y)]; }
    inline const TileInfo& tileAt(pos_t x, pos_t y) const {
        return memory[idx(x, y)];
    }
    inline TileInfo& tileAt(Coord pos) { return tileAt(pos.x, pos.y); }
    inline const TileInfo& tileAt(Coord pos) const {
        return tileAt(pos.x, pos.y);
    }

    std::vector<bool> alive;
    std::vector<Coord> generals;
    Coord spawn;  // general's position
    std::ofstream logFile;

    void logGrid(const char* title, const std::vector<value_t>& grid) {
#ifdef ZLYBOT_V3_LOG
        if (!logFile.is_open()) return;

        const int cw = 6;

        logFile << title << "\n";

        logFile << std::setw(2) << " ";
        for (pos_t y = 1; y <= width; ++y) logFile << std::setw(cw) << y;
        logFile << "\n";

        logFile << std::string(cw * (width + 1), '-') << "\n";

        for (pos_t x = 1; x <= height; ++x) {
            logFile << std::setw(2) << x;
            for (pos_t y = 1; y <= width; ++y) {
                value_t v = grid[idx(x, y)];
                if (v >= INF / 2)
                    logFile << std::setw(cw) << "INF";
                else if (v <= -INF / 2)
                    logFile << std::setw(cw) << "-INF";
                else
                    logFile << std::setw(cw) << v;
            }
            logFile << "\n";
        }
        logFile.flush();
#endif
    }

    inline army_t netArmy(Coord pos) const {
        return inSameTeam(tileAt(pos).occupier) ? tileAt(pos).army
                                                : -tileAt(pos).army;
    }

    std::vector<value_t> getDist(Coord start) {
        std::vector<value_t> dist((height + 2) * (width + 2), INF);
        std::queue<Coord> q;
        q.push(start);
        dist[idx(start)] = 0;
        while (!q.empty()) {
            Coord cur = q.front();
            q.pop();
            for (const auto& d : delta) {
                Coord next = cur + d;
                if (1 <= next.x && next.x <= height && 1 <= next.y &&
                    next.y <= width && !isImpassableTile(tileAt(next).type) &&
                    dist[idx(next)] == INF) {
                    dist[idx(next)] = dist[idx(cur)] + 1;
                    q.push(next);
                }
            }
        }
        return dist;
    }
    std::vector<value_t> getArmyDist(Coord start) {
        const pos_t N = (height + 2) * (width + 2);

        // Step 1: BFS for unweighted shortest hop distance
        constexpr pos_t HOP_INF = 1000000;
        std::vector<pos_t> hopDist(N, HOP_INF);
        std::queue<Coord> q;
        q.push(start);
        hopDist[idx(start)] = 0;
        while (!q.empty()) {
            Coord cur = q.front();
            q.pop();
            for (const auto& d : delta) {
                Coord next = cur + d;
                if (1 <= next.x && next.x <= height && 1 <= next.y &&
                    next.y <= width && !isImpassableTile(tileAt(next).type) &&
                    hopDist[idx(next)] == HOP_INF) {
                    hopDist[idx(next)] = hopDist[idx(cur)] + 1;
                    q.push(next);
                }
            }
        }

        // Step 2: DP on shortest-hop DAG — minimize weight among min-hop paths
        std::vector<value_t> dist(N, INF);
        dist[idx(start)] = 0;
        pos_t maxHop = 0;
        for (pos_t i = 0; i < N; ++i)
            if (hopDist[i] < HOP_INF && hopDist[i] > maxHop)
                maxHop = hopDist[i];
        for (pos_t d = 0; d <= maxHop; ++d) {
            for (pos_t x = 1; x <= height; ++x) {
                for (pos_t y = 1; y <= width; ++y) {
                    Coord cur{x, y};
                    if (isImpassableTile(tileAt(cur).type)) continue;
                    if (hopDist[idx(cur)] != d) continue;
                    if (dist[idx(cur)] == INF) continue;
                    for (const auto& dd : delta) {
                        Coord next = cur + dd;
                        if (hopDist[idx(next)] != d + 1) continue;
                        const value_t w = netArmy(next) - 1;
                        const value_t nd = dist[idx(cur)] + w;
                        if (nd < dist[idx(next)]) dist[idx(next)] = nd;
                    }
                }
            }
        }
        return dist;
    }

    std::vector<value_t> getAccessibility() {
        std::vector<value_t> acc((height + 2) * (width + 2), INF);
        using pq_pair = std::pair<value_t, Coord>;
        std::priority_queue<pq_pair, std::vector<pq_pair>,
                            std::greater<pq_pair>>
            queue;
        std::vector<bool> visited((height + 2) * (width + 2), false);
        acc[idx(spawn)] = 0;
        queue.emplace(0, spawn);
        while (!queue.empty()) {
            auto [ac, cur] = queue.top();
            queue.pop();
            if (ac != acc[idx(cur)]) continue;
            if (visited[idx(cur)]) continue;
            visited[idx(cur)] = true;
            value_t inc = 0;
            if (tileAt(cur).occupier == id)
                inc = 0;
            else {
                switch (tileAt(cur).type) {
                    case TILE_DESERT:      inc = 1; break;
                    case TILE_PLAIN:       inc = 1; break;
                    case TILE_CITY:        inc = 2; break;
                    case TILE_SPAWN:       inc = 2; break;
                    case TILE_SWAMP:       inc = 5; break;
                    case TILE_MOUNTAIN:
                    case TILE_LOOKOUT:
                    case TILE_OBSERVATORY: inc = INF; break;
                    case TILE_OBSTACLE:    inc = 200; break;
                    default:               inc = 0; break;
                }
                inc += tileAt(cur).army;
            }
            for (const auto& d : delta) {
                Coord next = cur + d;
                if (next.x < 1 || height < next.x || next.y < 1 ||
                    width < next.y)
                    continue;
                if (isImpassableTile(tileAt(next).type)) continue;
                if (ac + inc < acc[idx(next)]) {
                    acc[idx(next)] = ac + inc;
                    queue.emplace(ac + inc, next);
                }
            }
        }
        return acc;
    }

    std::deque<Coord> findRouteTo(Coord desti) {
        auto incNode = [&](RouteNode ori, Coord next) -> RouteNode {
            RouteNode nextNode = ori;
            nextNode.pos = next;
            nextNode.dist = nextNode.dist + 1;
            if (inSameTeam(id, tileAt(next).occupier)) {
                nextNode.friendArmy += tileAt(next).army - 1;
            } else {
                nextNode.oppoArmy += tileAt(next).army + 1;
            }
            if (tileAt(next).type == TILE_SWAMP) nextNode.oppoArmy += 1;
            value_t typeCostInc = 0;
            // switch (tileAt(next).type) {
            //     case TILE_DESERT:      typeCostInc = 0; break;
            //     case TILE_PLAIN:       typeCostInc = 0; break;
            //     case TILE_CITY:        typeCostInc = 2; break;
            //     case TILE_SPAWN:       typeCostInc = 4; break;
            //     case TILE_SWAMP:       typeCostInc = 8; break;
            //     case TILE_MOUNTAIN:
            //     case TILE_LOOKOUT:
            //     case TILE_OBSERVATORY: typeCostInc = INF; break;
            //     default:               typeCostInc = 0; break;
            // }
            nextNode.typeCost += typeCostInc;
            // nextNode.distPenalty = nextNode.dist * tileAt(desti).army;
            nextNode.distPenalty = 0;

            return nextNode;
        };

        for (TileInfo& t : memory) {
            t.routeDp = ROUTE_INF;
            t.routeVis = false;
        }

        std::vector<Coord> toTile((height + 2) * (width + 2), {-1, -1});
        std::priority_queue<RouteNode, std::vector<RouteNode>, RouteNodeLess> q;
        tileAt(desti).routeDp =
            RouteNode{totalArmy,           /* pos */ desti, /* dist */ 0,
                      /* friend */ 0,      /* oppo */ 1,    /* type cost */ 0,
                      /* dist penalty */ 0};
        q.emplace(tileAt(desti).routeDp);
        Coord start = desti;

        ZLYBOT_LOG("findRouteTo: target=("
                   << desti.x << "," << desti.y
                   << ") army=" << tileAt(desti).army
                   << " occupier=" << tileAt(desti).occupier);

        while (!q.empty()) {
            RouteNode cur = q.top();
            q.pop();
            if (tileAt(cur.pos).routeVis) continue;
            tileAt(cur.pos).routeVis = true;
            if (tileAt(cur.pos).routeDp.united() > 0 &&
                tileAt(cur.pos).routeDp.united() + netArmy(desti) > 0 &&
                tileAt(cur.pos).occupier == id) {
                start = cur.pos;
                break;
            }
            for (const auto& d : delta) {
                Coord next = cur.pos + d;
                if (1 <= next.x && next.x <= height && 1 <= next.y &&
                    next.y <= width && !isImpassableTile(tileAt(next).type) &&
                    !tileAt(next).routeVis) {
                    RouteNode nextNode = incNode(cur, next);
                    if (nextNode.united() > tileAt(next).routeDp.united()) {
                        tileAt(next).routeDp = nextNode;
                        toTile[idx(next)] = cur.pos;
                        q.emplace(nextNode);
                    }
                }
            }
        }

        std::deque<Coord> route;
        route.emplace_back(start);
        while (toTile[idx(route.back())] != Coord(-1, -1))
            route.emplace_back(toTile[idx(route.back())]);

        ZLYBOT_LOG("findRouteTo: route.size=" << route.size() << " start=("
                                              << start.x << "," << start.y
                                              << ")");
        return std::move(route);
    }

    void updateMemory() {
        for (pos_t i = 1; i <= height; ++i) {
            for (pos_t j = 1; j <= width; ++j) {
                TileInfo& t = tileAt(i, j);
                const TileView& view = board.tileAt(i, j);
                if (view.visible || view.type == TILE_SWAMP) {
                    t.isSeenBefore = true;
                }
                if (view.visible) {
                    t.type = view.type;
                    t.army = view.army;
                    t.occupier = view.occupier;
                    t.visible = true;
                    if (view.type == TILE_GENERAL)
                        generals[view.occupier] = Coord(i, j);
                } else {
                    t.visible = false;
                    t.occupier = -1;
                    if (!t.isSeenBefore) {
                        t.type = view.type;
                        t.army = view.army;
                    }
                }
            }
        }
        if (generals[id] != Coord(-1, -1)) spawn = generals[id];
        ZLYBOT_LOG("updateMemory: turn=" << turn << " spawn=(" << spawn.x << ","
                                         << spawn.y << ")");
    }

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
        mode = BotMode::EXPLORE;

        alive.assign(playerCnt, true);
        generals.assign(playerCnt, Coord(-1, -1));
        memory.assign((height + 2) * W, TileInfo{});
        spawn = Coord(-1, -1);

#ifdef ZLYBOT_V3_LOG
        logFile.open("zlybot_v3_" + std::to_string(id) + ".log");
#endif
        ZLYBOT_LOG("init: id=" << id << " height=" << height << " width="
                               << width << " playerCnt=" << playerCnt
                               << " team=" << team);
    }

    void requestMove(const BoardView& boardView,
                     const std::vector<RankItem>& _rank) override {
        ++halfTurn;
        turn += (halfTurn & 1);

        board = boardView;
        rank = _rank;
        updateMemory();

        std::sort(std::begin(rank), std::end(rank),
                  [](RankItem lhs, RankItem rhs) -> bool {
                      return lhs.player < rhs.player;
                  });
        for (index_t i = 0; i < playerCnt; ++i) {
            alive[i] = rank[i].alive;
        }
        totalArmy = rank[id].army;
        totalLand = rank[id].land;
        ZLYBOT_LOG("requestMove: turn=" << turn << " halfTurn=" << halfTurn
                                        << " totalArmy=" << totalArmy
                                        << " totalLand=" << totalLand);

        // Danger Coefficient
        auto distToSpawn = getDist(spawn);
        auto armyDistToSpawn = getArmyDist(spawn);
        auto accessibility = getAccessibility();
        value_t geographicImportance;
        value_t visionImportance;

        logGrid("--- distToSpawn (hops) ---", distToSpawn);
        logGrid("--- armyDistToSpawn (weighted) ---", armyDistToSpawn);
        logGrid("--- accessibility ---", accessibility);

        constexpr value_t lastTargetBonus = 100;
        auto calcValue = [&](Coord pos) -> value_t {
            value_t res = 0;
            if (pos == lastTarget) res += lastTargetBonus + lastMovedArmy;
            res -= tileAt(pos).army;
            if (tileAt(pos).occupier != -1 && !inSameTeam(tileAt(pos).occupier))
                res += 0 - 4 * distToSpawn[idx(pos)] -
                       2 * armyDistToSpawn[idx(pos)];
            res -= 2 * distToSpawn[idx(pos)];
            value_t typeValue = 0;
            switch (tileAt(pos).type) {
                case TILE_DESERT:      typeValue = -1; break;
                case TILE_PLAIN:       typeValue = 0; break;
                case TILE_CITY:        typeValue = 35; break;
                case TILE_SPAWN:       typeValue = 100; break;
                case TILE_SWAMP:       typeValue = -100; break;
                case TILE_MOUNTAIN:
                case TILE_LOOKOUT:
                case TILE_OBSERVATORY: typeValue = -INF; break;
                default:               typeValue = 0; break;
            }
            res += typeValue;
            res -= accessibility[idx(pos)];
            return res;
        };

        Coord target = spawn;
        value_t targetValue = -INF;
        std::vector<value_t> valueGrid((height + 2) * W, -INF);
        for (index_t i = 1; i <= height; ++i) {
            for (index_t j = 1; j <= width; ++j) {
                Coord cur = {i, j};
                if (inSameTeam(tileAt(cur).occupier)) continue;
                if (isImpassableTile(tileAt(cur).type)) continue;
                if (tileAt(cur).type == TILE_SWAMP &&
                    tileAt(cur).occupier == -1)
                    continue;
                valueGrid[idx(cur)] = calcValue(cur);
                if (valueGrid[idx(cur)] > targetValue) {
                    targetValue = valueGrid[idx(cur)];
                    target = cur;
                }
            }
        }

        logGrid("--- valueGrid ---", valueGrid);

        lastTarget = target;

        // Coord target = Coord(1, 1);

        std::deque<Coord> route;
        route = findRouteTo(target);

        std::vector<value_t> unitedGrid((height + 2) * W, -INF);
        for (pos_t x = 1; x <= height; ++x)
            for (pos_t y = 1; y <= width; ++y)
                unitedGrid[idx(x, y)] = tileAt(x, y).routeDp.united();
        logGrid("--- united() after findRouteTo ---", unitedGrid);

        if (route.size() >= 2) {
            const Coord& from = route[0];
            const Coord& to = route[1];
            Move ret = Move(MoveType::MOVE_ARMY, from, to, false);

            if (tileAt(from).type != TILE_SWAMP &&
                (tileAt(from).routeDp.friendArmy -
                 tileAt(from).routeDp.oppoArmy + netArmy(target) - 1 -
                 tileAt(from).routeDp.dist) >= tileAt(from).army / 2)
                ret.takeHalf = true;

            const army_t takenArmy = tileAt(from).army >> ret.takeHalf;
            if (tileAt(to).occupier != id)
                tileAt(to).army -= takenArmy;
            else
                tileAt(to).army += takenArmy;
            tileAt(from).army -= takenArmy;
            lastMovedArmy = takenArmy;

            route.pop_front();
            moveQueue.emplace_back(ret);
            ZLYBOT_LOG("requestMove: MOVE from=("
                       << from.x << "," << from.y << ") to=(" << to.x << ","
                       << to.y << ") takeHalf=" << (int)ret.takeHalf
                       << " army=" << (int)tileAt(from).army);
        } else {
            ZLYBOT_LOG("requestMove: no route (size=" << route.size() << ")");
        }
    }
};

static BotRegistrar<ZlyBot_v3> ZlyBot_v3_reg("ZlyBot v3-alpha.1");
