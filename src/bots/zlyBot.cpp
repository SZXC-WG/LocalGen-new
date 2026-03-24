/**
 * @file zlyBot.cpp
 *
 * ZlyBot from LocalGen v5.
 * Author: AppOfficer
 *
 * @copyright Copyright (c) SZXC Work Group.
 */

#ifndef LGEN_BOTS_ZLYBOT
#define LGEN_BOTS_ZLYBOT

#include <queue>
#include <random>

#include "core/bot.h"
#include "core/game.hpp"

class ZlyBot : public BasicBot {
   private:
    using value_t = intmax_t;
    constexpr static int64_t INF = 10'000'000'000'000'000LL;
    constexpr static Coord delta[] = {{-1, 0}, {0, -1}, {1, 0}, {0, 1}};

    enum class BotMode { ATTACK, EXPLORE, DEFEND };
    BotMode mode;

    pos_t height, width, W;
    index_t playerCnt;
    index_t id, team;
    std::vector<index_t> teamIds;
    config::Config config;

    BoardView board;
    std::vector<RankItem> rank;

    Coord focus;
    std::vector<bool> alive;
    std::deque<Coord> route;
    std::vector<Coord> generals;
    value_t tileTypeWeight[16];
    std::vector<value_t> tileValue;
    std::vector<pos_t> dist;
    std::vector<tile_type_e> tileTypeMemory;

    std::vector<int> distMark;
    int distVersion;

    struct NodeInfo {
        int dist;
        army_t army;
        Coord parent;
    };
    std::vector<NodeInfo> routeMap;
    std::vector<int> routeMark;
    int routeVersion;

    inline size_t idx(pos_t x, pos_t y) const {
        return static_cast<size_t>(x * W + y);
    }

    inline tile_type_e typeAt(pos_t x, pos_t y) {
        if (tileTypeMemory.at(idx(x, y)) != tile_type_e(-1))
            return tileTypeMemory[idx(x, y)];
        else if (board.tileAt(x, y).visible)
            return tileTypeMemory[idx(x, y)] = board.tileAt(x, y).type;
        else
            return board.tileAt(x, y).type;
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

        focus = Coord(0, 0);
        tileTypeWeight[TILE_BLANK] = 30 - 25;
        tileTypeWeight[TILE_SWAMP] = -1500;
        tileTypeWeight[TILE_MOUNTAIN] = -INF;
        tileTypeWeight[TILE_SPAWN] = 5;
        tileTypeWeight[TILE_CITY] = 30;
        tileTypeWeight[TILE_DESERT] = 1;
        tileTypeWeight[TILE_LOOKOUT] = -INF;
        tileTypeWeight[TILE_OBSERVATORY] = -INF;
        tileTypeWeight[TILE_OBSTACLE] = 30;
        alive.assign(constants.playerCount, true);
        generals.assign(constants.playerCount, Coord(-1, -1));

        tileValue.assign((height + 2) * W, 0);
        dist.assign((height + 2) * W, 0);
        tileTypeMemory.assign((height + 2) * W, tile_type_e(-1));

        distMark.assign((height + 2) * W, 0);
        distVersion = 0;

        routeMap.assign((height + 2) * W, NodeInfo{0, 0, Coord(-1, -1)});
        routeMark.assign((height + 2) * W, 0);
        routeVersion = 0;
    }

   private:
    void calcData(Coord foc) {
        ++distVersion;
        if (distVersion == 0) {
            std::fill(distMark.begin(), distMark.end(), 0);
            distVersion = 1;
        }

        dist[idx(foc.x, foc.y)] = 0;
        distMark[idx(foc.x, foc.y)] = distVersion;

        std::queue<std::pair<Coord, int>> q;
        q.emplace(foc, 0);

        while (!q.empty()) {
            auto [cur, curDist] = q.front();
            q.pop();

            for (int i = 0; i < 4; ++i) {
                Coord next = cur + delta[i];
                if (next.x < 1 || next.x > height || next.y < 1 ||
                    next.y > width)
                    continue;
                if (isImpassableTile(typeAt(next.x, next.y))) continue;
                if (distMark[idx(next.x, next.y)] == distVersion) continue;

                dist[idx(next.x, next.y)] = curDist + 1;
                distMark[idx(next.x, next.y)] = distVersion;
                q.emplace(next, curDist + 1);
            }
        }

        for (int i = 1; i <= height; ++i) {
            for (int j = 1; j <= width; ++j) {
                if (board.tileAt(i, j).occupier == id)
                    tileValue[idx(i, j)] = -INF;
                else if (distMark[idx(i, j)] != distVersion)
                    tileValue[idx(i, j)] = -INF;
                else
                    tileValue[idx(i, j)] =
                        tileTypeWeight[typeAt(i, j)] - dist[idx(i, j)];
            }
        }
    }

    void findRoute(Coord start, Coord desti) {
        ++routeVersion;
        if (routeVersion == 0) {
            std::fill(routeMark.begin(), routeMark.end(), 0);
            routeVersion = 1;
        }

        std::queue<Coord> q;
        q.push(start);

        routeMark[idx(start.x, start.y)] = routeVersion;
        routeMap[idx(start.x, start.y)] = {0, board.tileAt(start).army,
                                           Coord(-1, -1)};

        bool found = false;
        int foundDist = -1;

        while (!q.empty()) {
            Coord u = q.front();
            q.pop();

            int curDist = routeMap[idx(u.x, u.y)].dist;
            value_t curArmy = routeMap[idx(u.x, u.y)].army;

            if (found && curDist >= foundDist) continue;

            for (int i = 0; i < 4; ++i) {
                Coord v = u + delta[i];
                if (v.x < 1 || v.x > height || v.y < 1 || v.y > width) continue;
                if (isImpassableTile(typeAt(v.x, v.y))) continue;

                value_t gain = 0;
                if (board.tileAt(v).occupier == id)
                    gain = board.tileAt(v).army;
                else if (board.tileAt(v).visible)
                    gain = -board.tileAt(v).army;
                else {
                    tile_type_e t = typeAt(v.x, v.y);
                    if (t == TILE_BLANK)
                        gain = -5;
                    else if (t == TILE_SWAMP)
                        gain = -10;
                    else if (t == TILE_SPAWN)
                        gain = -5;
                    else if (t == TILE_CITY)
                        gain = -40;
                    else if (t == TILE_DESERT)
                        gain = -200;
                }

                value_t newArmy = curArmy + gain;
                int newDist = curDist + 1;

                if (routeMark[idx(v.x, v.y)] != routeVersion) {
                    routeMark[idx(v.x, v.y)] = routeVersion;
                    routeMap[idx(v.x, v.y)] = {newDist, newArmy, u};
                    q.push(v);

                    if (v == desti) {
                        found = true;
                        foundDist = newDist;
                    }
                } else {
                    if (routeMap[idx(v.x, v.y)].dist == newDist) {
                        if (newArmy > routeMap[idx(v.x, v.y)].army) {
                            routeMap[idx(v.x, v.y)].army = newArmy;
                            routeMap[idx(v.x, v.y)].parent = u;
                        }
                    }
                }
            }
        }

        route.clear();
        if (found) {
            Coord p = desti;
            while (p != start && p != Coord(-1, -1)) {
                route.push_front(p);
                if (routeMark[idx(p.x, p.y)] != routeVersion) break;
                p = routeMap[idx(p.x, p.y)].parent;
            }
        }
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

   public:
    void requestMove(const BoardView& boardView,
                     const std::vector<RankItem>& _rank) override {
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

        for (pos_t i = 1; i <= height; ++i)
            for (pos_t j = 1; j <= width; ++j)
                if (board.tileAt(i, j).visible &&
                    board.tileAt(i, j).type == TILE_GENERAL)
                    generals[board.tileAt(i, j).occupier] = Coord(i, j);

        if (board.tileAt(focus).occupier != id ||
            board.tileAt(focus).army == 0) {
            army_t mxArmy = -1;
            Coord mxCoo = generals[id];
            for (pos_t i = 1; i <= height; ++i) {
                for (pos_t j = 1; j <= width; ++j) {
                    if (board.tileAt(i, j).occupier == id) {
                        if (board.tileAt(i, j).army > mxArmy) {
                            mxArmy = board.tileAt(i, j).army;
                            mxCoo = Coord{i, j};
                        }
                    }
                }
            }
            focus = mxCoo;
        }

        mode = BotMode::EXPLORE;
        int goalGeneralId = -1;
        army_t goalGeneralArmy = INF;
        for (int i = 0; i < playerCnt; ++i) {
            if (i != id && generals[i] != Coord(-1, -1) && alive[i]) {
                mode = BotMode::ATTACK;
                if (rank[i].army < goalGeneralArmy) {
                    goalGeneralArmy = rank[i].army;
                    goalGeneralId = i;
                }
            }
        }

        calcData(focus);

        if (mode == BotMode::ATTACK) {
            findRoute(focus, generals[goalGeneralId]);
            if (!route.empty()) {
                Move ret =
                    Move(MoveType::MOVE_ARMY, focus, route.front(), false);
                focus = route.front();
                route.pop_front();
                moveQueue.emplace_back(ret);
            }
        } else if (mode == BotMode::EXPLORE) {
            Coord bestTarget = focus;
            value_t bestValue = -INF;
            for (int i = 1; i <= height; ++i) {
                for (int j = 1; j <= width; ++j) {
                    if (tileValue[idx(i, j)] > bestValue) {
                        bestValue = tileValue[idx(i, j)];
                        bestTarget = Coord(i, j);
                    }
                }
            }
            findRoute(focus, bestTarget);
            if (!route.empty()) {
                Move ret =
                    Move{MoveType::MOVE_ARMY, focus, route.front(), false};
                focus = route.front();
                route.pop_front();
                moveQueue.emplace_back(ret);
            }
        }
    }
};

static BotRegistrar<ZlyBot> zlyBot_reg("ZlyBot");

#endif  // LGEN_BOTS_ZLYBOT