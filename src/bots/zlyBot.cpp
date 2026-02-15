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
    constexpr static army_t INF = 10'000'000'000'000'000LL;
    constexpr static Coord delta[] = {{-1, 0}, {0, -1}, {1, 0}, {0, 1}};

    enum class BotMode {
        ATTACK,
        EXPLORE,
        DEFEND,
    };
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
    army_t tileTypeWeight[16];
    std::vector<std::vector<army_t>> tileValue;
    std::vector<std::vector<army_t>> dist;
    std::vector<std::vector<tile_type_e>> tileTypeMemory;

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

        tileValue.assign(height + 2, std::vector<army_t>(width + 2));
        dist.assign(height + 2, std::vector<army_t>(width + 2));
        tileTypeMemory.assign(
            height + 2, std::vector<tile_type_e>(width + 2, tile_type_e(-1)));
    }

   private:
    void calcData(Coord foc) {
        dist.assign(height + 2, std::vector<army_t>(width + 2, INF));
        dist[foc.x][foc.y] = 0;
        std::queue<std::pair<Coord, int>> q;
        q.push({foc, 0});
        while (!q.empty()) {
            auto [cur, curDist] = q.front();
            q.pop();
            for (int i = 0; i < 4; ++i) {
                Coord next = cur + delta[i];
                if (next.x < 1 || next.x > height || next.y < 1 ||
                    next.y > width)
                    continue;
                if (isImpassableTile(typeAt(next.x, next.y))) continue;
                if (dist[next.x][next.y] != INF) continue;
                dist[next.x][next.y] = curDist + 1;
                q.push({next, curDist + 1});
            }
        }
        for (int i = 1; i <= height; ++i) {
            for (int j = 1; j <= width; ++j) {
                if (board.tileAt(i, j).occupier == id)
                    tileValue[i][j] = -INF;
                else
                    tileValue[i][j] = tileTypeWeight[typeAt(i, j)] - dist[i][j];
            }
        }
    }

    void findRoute(Coord start, Coord desti) {
        int cnt = 1;
        std::vector<std::vector<std::vector<army_t>>> dp;
        std::vector<std::vector<std::vector<Coord>>> pr;
        auto gv = [&](int x, int y) -> army_t {
            if (x < 1 || x > height || y < 1 || y > width) return -INF;
            if (isImpassableTile(typeAt(x, y))) return -INF;
            if (board.tileAt(x, y).occupier == id)
                return board.tileAt(x, y).army;
            else {
                if (board.tileAt(x, y).visible)
                    return -board.tileAt(x, y).army;
                else {
                    if (typeAt(x, y) == 0) return -5;
                    if (typeAt(x, y) == 1) return -10;
                    if (typeAt(x, y) == 3) return -5;
                    if (typeAt(x, y) == 4) return -40;
                    if (typeAt(x, y) == 5) return -200;
                }
            }
            return 0;
        };
        dp.emplace_back(height + 1, std::vector<army_t>(width + 1, -INF));
        pr.emplace_back(height + 1,
                        std::vector<Coord>(width + 1, Coord(-1, -1)));
        dp[0][start.x][start.y] = board.tileAt(start).army;
        // printf("player %d (%ls):\n",id,playerInfo[id].name.c_str());
        for (int i = 1; i <= cnt; ++i) {
            dp.emplace_back(height + 1, std::vector<army_t>(width + 1, -INF));
            pr.emplace_back(height + 1,
                            std::vector<Coord>(width + 1, Coord(-1, -1)));
            for (pos_t x = 1; x <= height; ++x) {
                for (pos_t y = 1; y <= width; ++y) {
                    if (dp[i - 1][x][y] == -INF) continue;
                    for (int j = 0; j < 4; ++j) {
                        int nx = x + delta[j].x, ny = y + delta[j].y;
                        if (nx < 1 || nx > height || ny < 1 || ny > width)
                            continue;
                        if (isImpassableTile(typeAt(nx, ny))) continue;
                        army_t nv = dp[i - 1][x][y] + gv(nx, ny);
                        if (nv > dp[i][nx][ny]) {
                            pr[i][nx][ny] = Coord(x, y);
                            dp[i][nx][ny] = nv;
                        }
                    }
                }
            }
            if (i == cnt && pr[i][desti.x][desti.y] == Coord(-1, -1)) ++cnt;
        }
        route.clear();
        Coord p = desti;
        while (p != Coord(-1, -1)) {
            route.push_front(p);
            p = pr[cnt--][p.x][p.y];
        }
        route.pop_front();
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
            long long mxArmy = 0;
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
            Move ret = Move(MoveType::MOVE_ARMY, focus, route.front(), false);
            focus = route.front();
            route.pop_front();
            moveQueue.emplace_back(ret);
        } else if (mode == BotMode::EXPLORE) {
            struct node {
                Coord focus;
                army_t value;
            };
            std::vector<node> v;
            for (int i = 1; i <= height; ++i)
                for (int j = 1; j <= width; ++j)
                    v.push_back(node{Coord(i, j), tileValue[i][j]});
            sort(v.begin(), v.end(), [](const node& a, const node& b) {
                return a.value > b.value;
            });
            findRoute(focus, v[0].focus);
            Move ret = Move{MoveType::MOVE_ARMY, focus, route.front(), false};
            focus = route.front();
            route.pop_front();
            moveQueue.emplace_back(ret);
        }
    }
};

// Do not forget to register your bot.
static BotRegistrar<ZlyBot> zlyBot_reg("ZlyBot");

#endif  // LGEN_BOTS_ZLYBOT
