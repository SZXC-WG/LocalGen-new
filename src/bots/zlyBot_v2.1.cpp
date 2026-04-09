// Copyright (C) 2026 AppOfficer
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file zlyBot_v2.1.cpp
 *
 * ZlyBot v2.1
 *
 * @author AppOfficer
 */

#ifndef LGEN_BOTS_ZLYBOT_V2_1
#define LGEN_BOTS_ZLYBOT_V2_1

#include <queue>

#include "core/bot.h"
#include "core/game.hpp"

class ZlyBot_v2_1 : public BasicBot {
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

    turn_t halfTurn, turn;

    BoardView board;
    std::vector<RankItem> rank;

    inline bool inSameTeam(index_t anotherPlayer) const {
        return teamIds[anotherPlayer] == team;
    }
    inline bool inSameTeam(index_t player1, index_t player2) const {
        return teamIds[player1] == teamIds[player2];
    }

    struct RouteNode {
        Coord pos;
        value_t dist;
        value_t friendArmy;
        value_t oppoArmy;
        value_t typeCost;
        inline value_t united() const {
            return dist * 1000 - (friendArmy - oppoArmy) + typeCost;
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
        RouteNode{{-1, -1}, DIST_INF, -INF, INF, 0};

    struct TileInfo {
        RouteNode routeDp = ROUTE_INF;
        // 8-byte aligned fields
        army_t army = 0;
        value_t value = 0;
        value_t danger = 0;
        value_t dist0 = DIST_INF;
        value_t dist1 = DIST_INF;
        value_t distToSpawn = DIST_INF;
        // 4-byte aligned fields
        tile_type_e type = tile_type_e(-1);
        index_t occupier = -1;
        // 8-byte aligned struct (contains two 4-byte fields)
        Coord routePrev{-1, -1};
        // 1-byte aligned fields
        bool visible = false;
        bool isSeenBefore = false;
        bool inPrevMoves = false;
        bool routeVis = false;
    };

    std::vector<TileInfo> tiles;
    Coord focus[2];
    std::vector<bool> alive;
    std::vector<Coord> generals;
    std::deque<Coord> route;
    pos_t leastUsage;
    value_t tileTypeWeight[16];
    std::vector<Coord> homeZone;
    std::deque<Coord> prevMoves;
    Coord spawnCoord;

    inline size_t idx(pos_t x, pos_t y) const {
        return static_cast<size_t>(x * W + y);
    }
    inline size_t idx(Coord coo) const {
        return static_cast<size_t>(coo.x * W + coo.y);
    }

    inline TileInfo& tileAt(pos_t x, pos_t y) { return tiles[idx(x, y)]; }
    inline const TileInfo& tileAt(pos_t x, pos_t y) const {
        return tiles[idx(x, y)];
    }
    inline TileInfo& tileAt(Coord coo) { return tiles[idx(coo)]; }
    inline const TileInfo& tileAt(Coord coo) const { return tiles[idx(coo)]; }

    inline void recordNewMove(Coord pos) {
        prevMoves.emplace_back(pos);
        tileAt(pos).inPrevMoves = true;
        if (prevMoves.size() > 20) {
            auto front = prevMoves.front();
            prevMoves.pop_front();
            tileAt(front).inPrevMoves = false;
        }
    }

    inline tile_type_e typeAt(pos_t x, pos_t y) {
        TileInfo& t = tileAt(x, y);
        if (t.type != tile_type_e(-1))
            return t.type;
        else if (board.tileAt(x, y).visible)
            return t.type = board.tileAt(x, y).type;
        else if (board.tileAt(x, y).type == TILE_MOUNTAIN ||
                 board.tileAt(x, y).type == TILE_CITY)
            return t.type = TILE_OBSTACLE;
        else if (board.tileAt(x, y).type == TILE_SPAWN)
            return t.type = TILE_BLANK;
        else
            return board.tileAt(x, y).type;
    }
    inline tile_type_e typeAt(Coord pos) { return typeAt(pos.x, pos.y); }

    template <typename DistPtr, typename WeightFunc>
    void dijkstra(Coord start, DistPtr dist, WeightFunc weight) {
        for (TileInfo& t : tiles) t.*dist = DIST_INF;
        tileAt(start).*dist = 0;
        std::priority_queue<std::pair<value_t, Coord>,
                            std::vector<std::pair<value_t, Coord>>,
                            std::greater<>>
            queue;
        queue.emplace(0, start);
        while (!queue.empty()) {
            auto [curDist, cur] = queue.top();
            queue.pop();
            if (curDist > tileAt(cur).*dist) continue;
            for (int i = 0; i < 4; ++i) {
                Coord next = cur + delta[i];
                if (next.x < 1 || next.x > height || next.y < 1 ||
                    next.y > width)
                    continue;
                if (isImpassableTile(typeAt(next.x, next.y))) continue;
                value_t newDist = curDist + weight(next.x, next.y);
                TileInfo& nt = tileAt(next);
                if (newDist < nt.*dist) {
                    nt.*dist = newDist;
                    queue.emplace(newDist, next);
                }
            }
        }
    }

    void dijkstraBFS(Coord start) {
        for (TileInfo& t : tiles) t.distToSpawn = DIST_INF;
        tileAt(start).distToSpawn = 0;
        std::vector<Coord> queue;
        queue.reserve(tiles.size());
        queue.emplace_back(start);
        size_t head = 0;
        while (head < queue.size()) {
            Coord cur = queue[head++];
            for (int i = 0; i < 4; ++i) {
                Coord next = cur + delta[i];
                if (next.x < 1 || next.x > height || next.y < 1 ||
                    next.y > width)
                    continue;
                if (isImpassableTile(typeAt(next.x, next.y))) continue;
                TileInfo& nt = tileAt(next);
                if (nt.distToSpawn != DIST_INF) continue;
                nt.distToSpawn = tileAt(cur).distToSpawn + 1;
                queue.emplace_back(next);
            }
        }
    }

    BotMode updateMode(value_t homeZoneDangerAver, bool homeZoneThreat,
                       value_t homeZoneDanger) {
        if ((mode == BotMode::ATTACK && homeZoneDangerAver > 5) ||
            (mode == BotMode::EXPLORE && homeZoneDangerAver > 0) ||
            homeZoneThreat) {
            return BotMode::DEFEND;
        }
        if (mode == BotMode::DEFEND && homeZoneDanger <= 0) {
            return BotMode::EXPLORE;
        }
        return mode;
    }

    void calcData(Coord foc0, Coord foc1) {
        auto edgeWeight = [&](pos_t x, pos_t y) -> value_t {
            const TileInfo& t = tileAt(x, y);
            value_t w = 10;
            if (t.visible) {
                if (t.occupier != id) w += std::max(t.army, army_t(0));
            } else {
                w += std::max(t.army, army_t(0));
            }
            if (typeAt(x, y) == TILE_SWAMP) w += 100;
            return w;
        };

        dijkstra(foc0, &TileInfo::dist0, edgeWeight);
        dijkstra(foc1, &TileInfo::dist1, edgeWeight);

        {
            homeZone.clear();
            dijkstraBFS(spawnCoord);
            auto defenseDist = std::min(
                20, std::max(3, static_cast<int>(rank[id].land / 15.0)));
            for (pos_t i = 1; i <= height; ++i) {
                for (pos_t j = 1; j <= width; ++j) {
                    const TileInfo& t = tileAt(i, j);
                    if (t.distToSpawn <= defenseDist &&
                        !(typeAt(i, j) == TILE_SWAMP &&
                          (!t.visible || t.occupier == -1)))
                        homeZone.emplace_back(i, j);
                }
            }
        }

        for (pos_t i = 1; i <= height; ++i) {
            for (pos_t j = 1; j <= width; ++j) {
                TileInfo& t = tileAt(i, j);
                if (t.occupier == id) {
                    t.value = -INF;
                    t.danger = -INF;
                } else {
                    tile_type_e tt = typeAt(i, j);
                    t.value = tileTypeWeight[tt];
                    t.value -= t.dist0;
                    t.value -= t.army;
                    t.danger = -tileTypeWeight[tt];
                    t.danger -= t.distToSpawn * 2;
                    t.danger -= t.dist1;
                    t.value -=
                        t.isSeenBefore * (turn - 100000.0 / rank[id].army);
                    if (t.visible && t.occupier != -1) {
                        army_t adjacent_minimum_same_player = INF;
                        for (int k = 0; k < 4; ++k) {
                            Coord adja = Coord(i, j) + delta[k];
                            if (adja.x < 1 || adja.x > height || adja.y < 1 ||
                                adja.y > width)
                                continue;
                            const TileInfo& at = tileAt(adja);
                            if (at.visible && at.occupier == t.occupier)
                                adjacent_minimum_same_player = std::min(
                                    adjacent_minimum_same_player, at.army);
                        }
                        if (adjacent_minimum_same_player == INF)
                            adjacent_minimum_same_player = t.army;
                        t.value += 2 * (t.army - adjacent_minimum_same_player);
                    }
                    if (t.visible) {
                        if (t.occupier != -1)
                            t.danger += 2 * t.army;
                        else
                            t.danger -= t.army / 2;
                    }
                }
            }
        }
    }

    void findRoute(Coord start, Coord desti) {
        constexpr value_t DIST_INC = 1;

        auto typeCost = [](tile_type_e type) -> value_t {
            switch (type) {
                case TILE_BLANK:    return 0; break;
                case TILE_SWAMP:    return 10; break;
                case TILE_SPAWN:    return 0; break;
                case TILE_CITY:     return 1; break;
                case TILE_DESERT:   return 0; break;
                case TILE_OBSTACLE: return 5; break;
                default:            return INF;
            }
        };
        auto incNode = [&](RouteNode ori, Coord next) -> RouteNode {
            if (std::clamp(next.x, 1, height) != next.x ||
                std::clamp(next.y, 1, width) != next.y)
                return ROUTE_INF;
            const tile_type_e tt = typeAt(next);
            if (isImpassableTile(tt)) return ROUTE_INF;
            ori.dist += DIST_INC;
            const TileInfo& t = tileAt(next);
            if (ori.pos == spawnCoord &&
                (mode == BotMode::EXPLORE || mode == BotMode::DEFEND))
                ori.friendArmy >>= 1;
            if (inSameTeam(t.occupier, id))
                ori.friendArmy += t.army;
            else
                ori.oppoArmy += t.army;
            ori.typeCost += typeCost(tt);
            ori.pos = next;
            return ori;
        };

        for (TileInfo& t : tiles) {
            t.routeDp = ROUTE_INF;
            t.routePrev = Coord(-1, -1);
            t.routeVis = false;
        }
        std::priority_queue<RouteNode, std::vector<RouteNode>, RouteNodeGreater>
            q;
        tileAt(start).routeDp = RouteNode{start, 0, tileAt(start).army, 0, 0};
        q.emplace(tileAt(start).routeDp);
        while (!q.empty()) {
            RouteNode cur = q.top();
            q.pop();
            TileInfo& ct = tileAt(cur.pos);
            if (ct.routeVis) continue;
            if (routeComparerGreater(cur, ct.routeDp)) continue;
            ct.routeVis = true;
            if (cur.pos == desti) break;
            for (int i = 0; i < 4; ++i) {
                Coord next = cur.pos + delta[i];
                if (next.x < 1 || next.x > height || next.y < 1 ||
                    next.y > width)
                    continue;
                if (isImpassableTile(typeAt(next.x, next.y))) continue;
                TileInfo& nt = tileAt(next);
                if (nt.routeVis) continue;
                if (nt.inPrevMoves) continue;
                auto nextNode = incNode(cur, next);
                if (routeComparerLess(nextNode, nt.routeDp)) {
                    nt.routeDp = nextNode;
                    nt.routePrev = cur.pos;
                    q.emplace(nextNode);
                }
            }
        }
        route.clear();
        Coord cur = desti;
        while (cur != Coord(-1, -1)) {
            route.push_front(cur);
            cur = tileAt(cur).routePrev;
        }
        route.pop_front();
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
                const TileInfo& t = tileAt(i, j);
                if (t.occupier == id && t.army > maxArmy) {
                    maxArmy = t.army;
                    maxCoo = Coord(i, j);
                }
            }
        }
        return maxCoo;
    }

    Coord findMaxArmyPosInHomeZone() {
        army_t maxArmy = 0;
        Coord maxCoo = spawnCoord;
        for (auto pos : homeZone) {
            const TileInfo& t = tileAt(pos);
            if (t.visible && t.occupier == id) {
                if (t.army > maxArmy) {
                    maxArmy = t.army;
                    maxCoo = pos;
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

        focus[0] = focus[1] = Coord(0, 0);
        tileTypeWeight[TILE_BLANK] = 60;
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

        tiles.assign((height + 2) * W, TileInfo{});
        mode = BotMode::EXPLORE;
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
        for (index_t i = 0; i < playerCnt; ++i) {
            alive[i] = rank[i].alive;
        }

        moveQueue.clear();

        for (pos_t i = 1; i <= height; ++i) {
            for (pos_t j = 1; j <= width; ++j) {
                TileInfo& t = tileAt(i, j);
                const TileView& view = board.tileAt(i, j);

                if (t.army > 0) --t.army;

                if (view.visible || view.type == TILE_SWAMP) {
                    t.isSeenBefore = true;
                }
                if (view.visible) {
                    t.type = view.type;
                    t.army = view.army;
                    t.occupier = view.occupier;
                    t.visible = true;
                } else {
                    t.visible = false;
                    t.occupier = -1;
                    if (!t.isSeenBefore) {
                        switch (typeAt(i, j)) {
                            case TILE_BLANK:       t.army = 0; break;
                            case TILE_SWAMP:       t.army = 0; break;
                            case TILE_MOUNTAIN:
                            case TILE_LOOKOUT:
                            case TILE_OBSERVATORY: t.army = INF; break;
                            case TILE_SPAWN:       t.army = -INF; break;
                            case TILE_CITY:
                            case TILE_DESERT:
                            case TILE_OBSTACLE:    t.army = 40; break;
                            default:               break;
                        }
                    }
                }
                if (view.visible && view.type == TILE_GENERAL)
                    generals[view.occupier] = Coord(i, j);
            }
        }
        if (generals[id] != Coord(-1, -1)) spawnCoord = generals[id];
        if (tileAt(focus[0]).occupier != id || tileAt(focus[0]).army == 0) {
            focus[0] = findMaxArmyPos();
            leastUsage = 0;
        }
        if (tileAt(focus[1]).occupier != id || tileAt(focus[1]).army == 0) {
            focus[1] = findMaxArmyPosInHomeZone();
        }
        if (leastUsage != 0) {
            --leastUsage;
            auto next = route.front();
            route.pop_front();
            moveQueue.emplace_back(MoveType::MOVE_ARMY, focus[0], next, false);
            focus[0] = next;
        }
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
        if (targetOppoId == -1 && mode == BotMode::ATTACK)
            mode = BotMode::EXPLORE;

        calcData(focus[0], focus[1]);

        bool homeZoneThreat = false;
        value_t homeZoneDanger = 0;
        const TileInfo& spawnTile = tileAt(spawnCoord);
        for (auto pos : homeZone) {
            const TileInfo& t = tileAt(pos);
            if (!t.visible || t.occupier != id) {
                homeZoneDanger += t.danger + 10;
            }
            if (t.visible && t.occupier != -1 && t.occupier != id &&
                t.army > spawnTile.army)
                homeZoneThreat = true;
        }
        value_t homeZoneDangerAver =
            homeZoneDanger / static_cast<value_t>(homeZone.size());

        mode = updateMode(homeZoneDangerAver, homeZoneThreat, homeZoneDanger);
        if (mode == BotMode::DEFEND) focus[1] = findMaxArmyPosInHomeZone();

        if (mode == BotMode::ATTACK) {
            findRoute(focus[0], generals[targetOppoId]);
            if (!route.empty()) {
                Move ret =
                    Move(MoveType::MOVE_ARMY, focus[0], route.front(), false);

                const army_t takenArmy = tileAt(focus[0]).army >> ret.takeHalf;
                if (tileAt(route.front()).occupier != id)
                    tileAt(route.front()).army -= takenArmy;
                else
                    tileAt(route.front()).army += takenArmy;
                tileAt(focus[0]).army -= takenArmy;

                focus[0] = route.front();
                route.pop_front();
                leastUsage = std::min((pos_t)route.size(), pos_t(0));
                moveQueue.emplace_back(ret);
            }
        } else if (mode == BotMode::EXPLORE) {
            Coord bestTarget = focus[0];
            value_t bestValue = -INF;
            for (pos_t i = 1; i <= height; ++i) {
                for (pos_t j = 1; j <= width; ++j) {
                    const TileInfo& t = tileAt(i, j);
                    if (!isImpassableTile(typeAt(i, j)) &&
                        t.value > bestValue) {
                        bestValue = t.value;
                        bestTarget = Coord(i, j);
                    }
                }
            }
            findRoute(focus[0], bestTarget);
            if (!route.empty()) {
                Move ret =
                    Move(MoveType::MOVE_ARMY, focus[0], route.front(), false);
                if (focus[0] == generals[id]) ret.takeHalf = true;

                const army_t takenArmy = tileAt(focus[0]).army >> ret.takeHalf;
                if (tileAt(route.front()).occupier != id)
                    tileAt(route.front()).army -= takenArmy;
                else
                    tileAt(route.front()).army += takenArmy;
                tileAt(focus[0]).army -= takenArmy;

                focus[0] = route.front();
                route.pop_front();
                leastUsage = std::min((pos_t)route.size(), pos_t(0));
                moveQueue.emplace_back(ret);
            }
        } else if (mode == BotMode::DEFEND) {
            Coord bestTarget = focus[1];
            value_t bestDanger = -INF;
            for (auto pos : homeZone) {
                const TileInfo& t = tileAt(pos);
                if (t.danger > bestDanger) {
                    bestDanger = t.danger;
                    bestTarget = pos;
                }
            }
            findRoute(focus[1], bestTarget);
            if (!route.empty()) {
                Move ret =
                    Move(MoveType::MOVE_ARMY, focus[1], route.front(), false);
                if (focus[1] == generals[id]) ret.takeHalf = true;

                const army_t takenArmy = tileAt(focus[1]).army >> ret.takeHalf;
                if (tileAt(route.front()).occupier != id)
                    tileAt(route.front()).army -= takenArmy;
                else
                    tileAt(route.front()).army += takenArmy;
                tileAt(focus[1]).army -= takenArmy;

                focus[1] = route.front();
                route.pop_front();
                leastUsage = std::min((pos_t)route.size(), pos_t(0));
                moveQueue.emplace_back(ret);
            }
        }
    }
};

static BotRegistrar<ZlyBot_v2_1> zlyBot_v2_1_reg("ZlyBot v2.1");

#endif
