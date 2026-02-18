/**
 * @file zlyBot.cpp
 *
 * Legacy `zlyBot` from LocalGen v5.
 *
 * @copyright Copyright (c) SZXC Work Group.
 */

#ifndef LGEN_BOTS_ZLYBOT
#define LGEN_BOTS_ZLYBOT

#include <queue>
#include <random>

#include "../GameEngine/bot.h"
#include "../GameEngine/game.hpp"

class ZlyBot : public BasicBot {
   private:
    using value_t = intmax_t;
    constexpr static int64_t INF = 10'000'000'000'000'000LL;
    constexpr static Coord delta[] = {{-1, 0}, {0, -1}, {1, 0}, {0, 1}};

    enum class BotMode { ATTACK, EXPLORE, DEFEND };
    BotMode mode;

    pos_t height, width;
    index_t playerCnt;
    index_t id, team;
    std::vector<index_t> teamIds;
    game::config::Config config;

    BoardView board;
    std::vector<game::RankItem> rank;

    Coord focus;
    std::vector<bool> alive;
    std::deque<Coord> route;
    std::vector<Coord> generals;
    value_t tileTypeWeight[16];
    std::vector<std::vector<value_t>> tileValue;
    std::vector<std::vector<pos_t>> dist;
    std::vector<std::vector<tile_type_e>> tileTypeMemory;

    std::vector<std::vector<int>> distMark;
    int distVersion;

    struct NodeInfo {
        int dist;
        army_t army;
        Coord parent;
    };
    std::vector<std::vector<NodeInfo>> routeMap;
    std::vector<std::vector<int>> routeMark;
    int routeVersion;

    inline tile_type_e typeAt(pos_t x, pos_t y) {
        if (tileTypeMemory.at(x).at(y) != -1)
            return tileTypeMemory[x][y];
        else if (board.tileAt(x, y).visible)
            return tileTypeMemory[x][y] = board.tileAt(x, y).type;
        else
            return board.tileAt(x, y).type;
    }

   public:
    void init(index_t playerId,
              const game::GameConstantsPack& constants) override {
        id = playerId;
        height = constants.mapHeight;
        width = constants.mapWidth;
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

        tileValue.assign(height + 2, std::vector<value_t>(width + 2));
        dist.assign(height + 2, std::vector<pos_t>(width + 2));
        tileTypeMemory.assign(
            height + 2, std::vector<tile_type_e>(width + 2, tile_type_e(-1)));

        distMark.assign(height + 2, std::vector<int>(width + 2, 0));
        distVersion = 0;

        routeMap.assign(height + 2, std::vector<NodeInfo>(width + 2));
        routeMark.assign(height + 2, std::vector<int>(width + 2, 0));
        routeVersion = 0;
    }

   private:
    void calcData(Coord foc) {
        ++distVersion;
        if (distVersion == 0) {
            distMark.assign(height + 2, std::vector<int>(width + 2, 0));
            distVersion = 1;
        }

        dist[foc.x][foc.y] = 0;
        distMark[foc.x][foc.y] = distVersion;

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
                if (distMark[next.x][next.y] == distVersion) continue;

                dist[next.x][next.y] = curDist + 1;
                distMark[next.x][next.y] = distVersion;
                q.emplace(next, curDist + 1);
            }
        }

        for (int i = 1; i <= height; ++i) {
            for (int j = 1; j <= width; ++j) {
                if (board.tileAt(i, j).occupier == id)
                    tileValue[i][j] = -INF;
                else if (distMark[i][j] != distVersion)
                    tileValue[i][j] = -INF;
                else
                    tileValue[i][j] = tileTypeWeight[typeAt(i, j)] - dist[i][j];
            }
        }
    }

    void findRoute(Coord start, Coord desti) {
        ++routeVersion;
        if (routeVersion == 0) {
            routeMark.assign(height + 2, std::vector<int>(width + 2, 0));
            routeVersion = 1;
        }

        std::queue<Coord> q;
        q.push(start);

        routeMark[start.x][start.y] = routeVersion;
        routeMap[start.x][start.y] = {0, board.tileAt(start).army,
                                      Coord(-1, -1)};

        bool found = false;
        int foundDist = -1;

        while (!q.empty()) {
            Coord u = q.front();
            q.pop();

            int curDist = routeMap[u.x][u.y].dist;
            value_t curArmy = routeMap[u.x][u.y].army;

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

                if (routeMark[v.x][v.y] != routeVersion) {
                    routeMark[v.x][v.y] = routeVersion;
                    routeMap[v.x][v.y] = {newDist, newArmy, u};
                    q.push(v);

                    if (v == desti) {
                        found = true;
                        foundDist = newDist;
                    }
                } else {
                    if (routeMap[v.x][v.y].dist == newDist) {
                        if (newArmy > routeMap[v.x][v.y].army) {
                            routeMap[v.x][v.y].army = newArmy;
                            routeMap[v.x][v.y].parent = u;
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
                if (routeMark[p.x][p.y] != routeVersion) break;
                p = routeMap[p.x][p.y].parent;
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
                     const std::vector<game::RankItem>& _rank) override {
        board = boardView;
        rank = _rank;
        std::sort(begin(rank), end(rank),
                  [](game::RankItem lhs, game::RankItem rhs) -> bool {
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
                    if (tileValue[i][j] > bestValue) {
                        bestValue = tileValue[i][j];
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

// Do not forget to register your bot.
static BotRegistrar<ZlyBot> zlyBot_reg("ZlyBot");

#endif  // LGEN_BOTS_ZLYBOT
