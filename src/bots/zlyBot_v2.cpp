/**
 * @file zlyBot_v2.cpp
 *
 * ZlyBot v2 from LocalGen v5.
 *
 * @copyright Copyright (c) SZXC Work Group.
 */

#ifndef LGEN_BOTS_ZLYBOT_V2
#define LGEN_BOTS_ZLYBOT_V2

#include <queue>
#include <random>

#include "../core/bot.h"
#include "../core/game.hpp"

class ZlyBot_v2 : public BasicBot {
   private:
    using value_t = intmax_t;
    constexpr static pos_t DIST_INF = 32767;
    constexpr static int64_t INF = 10'000'000'000'000'000LL;
    constexpr static Coord delta[] = {{-1, 0}, {0, -1}, {1, 0}, {0, 1}};

    enum class BotMode { ATTACK, EXPLORE, DEFEND };
    BotMode mode;

    pos_t height, width, W;
    index_t playerCnt;
    index_t id, team;
    std::vector<index_t> teamIds;
    game::config::Config config;

    turn_t halfTurn, turn;

    BoardView board;
    std::vector<game::RankItem> rank;

    inline bool inSameTeam(index_t anotherPlayer) const {
        return teamIds[anotherPlayer] == team;
    }
    inline bool inSameTeam(index_t player1, index_t player2) const {
        return teamIds[player1] == teamIds[player2];
    }

    Coord focus;
    std::vector<bool> alive;
    std::vector<Coord> generals;
    std::deque<Coord> route;
    pos_t leastUsage;
    value_t tileTypeWeight[16];
    std::vector<value_t> tileValue;
    std::vector<value_t> dist;
    std::vector<tile_type_e> tileTypeMemory;
    std::vector<army_t> tileArmyMemory;
    std::deque<Coord> prevMoves;
    std::vector<bool> inPrevMoves;
    std::vector<bool> isSeenBefore;

    inline size_t idx(pos_t x, pos_t y) const {
        return static_cast<size_t>(x * W + y);
    }

    inline void recordNewMove(Coord pos) {
        prevMoves.emplace_back(pos);
        inPrevMoves[idx(pos.x, pos.y)] = true;
        if (prevMoves.size() > 20) {
            auto front = prevMoves.front();
            prevMoves.pop_front();
            inPrevMoves[idx(front.x, front.y)] = false;
        }
    }

    inline tile_type_e typeAt(pos_t x, pos_t y) {
        if (tileTypeMemory.at(idx(x, y)) != tile_type_e(-1))
            return tileTypeMemory[idx(x, y)];
        else
            return board.tileAt(x, y).type;
    }
    inline army_t armyAt(pos_t x, pos_t y) { return tileArmyMemory[idx(x, y)]; }

    void calcData(Coord foc) {
        std::fill(dist.begin(), dist.end(), DIST_INF);
        dist[idx(foc.x, foc.y)] = 0;
        std::priority_queue<std::pair<value_t, Coord>,
                            std::vector<std::pair<value_t, Coord>>,
                            std::greater<>>
            queue;
        queue.emplace(0, foc);
        while (!queue.empty()) {
            auto [curDist, cur] = queue.top();
            queue.pop();
            if (curDist > dist[idx(cur.x, cur.y)]) continue;
            for (int i = 0; i < 4; ++i) {
                Coord next = cur + delta[i];
                if (next.x < 1 || next.x > height || next.y < 1 ||
                    next.y > width)
                    continue;
                if (isImpassableTile(typeAt(next.x, next.y))) continue;
                value_t newDist = curDist + 10;
                if (board.tileAt(next).visible) {
                    if (board.tileAt(next).occupier != id)
                        newDist += std::max(armyAt(next.x, next.y), army_t(0));
                } else
                    newDist += std::max(armyAt(next.x, next.y), army_t(0));
                if (typeAt(next.x, next.y) == TILE_SWAMP) newDist += 100;
                if (newDist < dist[idx(next.x, next.y)]) {
                    dist[idx(next.x, next.y)] = newDist;
                    queue.emplace(newDist, next);
                }
            }
        }

        for (pos_t i = 1; i <= height; ++i) {
            for (pos_t j = 1; j <= width; ++j) {
                if (board.tileAt(i, j).occupier == id)
                    tileValue[idx(i, j)] = -INF;
                else {
                    tileValue[idx(i, j)] = tileTypeWeight[typeAt(i, j)];
                    tileValue[idx(i, j)] -= dist[idx(i, j)];
                    tileValue[idx(i, j)] -= armyAt(i, j);
                    tileValue[idx(i, j)] -= isSeenBefore[idx(i, j)] *
                                            (turn - 100000.0 / rank[id].army);
                    if (board.tileAt(i, j).visible &&
                        board.tileAt(i, j).occupier != -1) {
                        army_t adjacent_minimum_same_player = INF;
                        for (int k = 0; k < 4; ++k) {
                            Coord adja = Coord(i, j) + delta[k];
                            if (adja.x < 1 || adja.x > height || adja.y < 1 ||
                                adja.y > width)
                                continue;
                            if (board.tileAt(adja).visible &&
                                board.tileAt(adja).occupier ==
                                    board.tileAt(i, j).occupier)
                                adjacent_minimum_same_player =
                                    std::min(adjacent_minimum_same_player,
                                             board.tileAt(adja).army);
                        }
                        if (adjacent_minimum_same_player == INF)
                            adjacent_minimum_same_player =
                                board.tileAt(i, j).army;
                        tileValue[idx(i, j)] +=
                            2 * (board.tileAt(i, j).army -
                                 adjacent_minimum_same_player);
                    }
                }
            }
        }
    }

    void findRoute(Coord start, Coord desti) {
        auto DisInc = 1;
        auto ArmyInc = [&](int x, int y) -> value_t {
            if (x < 1 || x > height || y < 1 || y > width) return INF;
            if (isImpassableTile(typeAt(x, y))) return INF;
            if (board.tileAt(x, y).occupier == id)
                return -board.tileAt(x, y).army;
            else
                return armyAt(x, y);
        };
        auto TypeInc = [&](int x, int y) -> value_t {
            switch (typeAt(x, y)) {
                case TILE_BLANK:       return 0;
                case TILE_SWAMP:       return 10;
                case TILE_MOUNTAIN:    return INF;
                case TILE_GENERAL:     return 0;
                case TILE_CITY:        return 1;
                case TILE_DESERT:      return 0;
                case TILE_LOOKOUT:     return INF;
                case TILE_OBSERVATORY: return INF;
                case TILE_OBSTACLE:    return 5;
                default:               return INF;
            }
        };
        auto UnitedInc = [&](int x, int y) -> value_t {
            return DisInc * 1000 + ArmyInc(x, y) + TypeInc(x, y);
        };
        std::vector<bool> vis((height + 2) * W, false);
        std::vector<Coord> prev((height + 2) * W, Coord(-1, -1));
        std::vector<value_t> dp((height + 2) * W, INF);
        std::priority_queue<std::pair<value_t, Coord>,
                            std::vector<std::pair<value_t, Coord>>,
                            std::greater<std::pair<value_t, Coord>>>
            q;
        dp[idx(start.x, start.y)] = 0;
        q.emplace(0, start);
        while (!q.empty()) {
            Coord cur = q.top().second;
            value_t curVal = q.top().first;
            q.pop();
            if (curVal > dp[idx(cur.x, cur.y)]) continue;
            if (vis[idx(cur.x, cur.y)]) continue;
            vis[idx(cur.x, cur.y)] = true;
            if (cur == desti) break;
            for (int i = 0; i < 4; ++i) {
                Coord next = cur + delta[i];
                if (next.x < 1 || next.x > height || next.y < 1 ||
                    next.y > width)
                    continue;
                if (isImpassableTile(typeAt(next.x, next.y))) continue;
                if (vis[idx(next.x, next.y)]) continue;
                if (inPrevMoves[idx(next.x, next.y)]) continue;
                value_t nextVal = curVal + UnitedInc(next.x, next.y);
                if (nextVal < dp[idx(next.x, next.y)]) {
                    dp[idx(next.x, next.y)] = nextVal;
                    prev[idx(next.x, next.y)] = cur;
                    q.emplace(nextVal, next);
                }
            }
        }
        route.clear();
        Coord cur = desti;
        while (cur != Coord(-1, -1)) {
            route.push_front(cur);
            cur = prev[idx(cur.x, cur.y)];
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
        return;
    }

    Coord findMaxArmyPos() {
        army_t maxArmy = 0;
        Coord maxCoo = generals[id];
        for (pos_t i = 1; i <= height; ++i) {
            for (pos_t j = 1; j <= width; ++j) {
                if (board.tileAt(i, j).occupier == id &&
                    board.tileAt(i, j).army > maxArmy) {
                    maxArmy = board.tileAt(i, j).army;
                    maxCoo = Coord(i, j);
                }
            }
        }
        return maxCoo;
    }

   public:
    void init(index_t playerId,
              const game::GameConstantsPack& constants) override {
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

        focus = Coord(0, 0);
        tileTypeWeight[TILE_BLANK] = 300 - 25 * 10 + 10;
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

        tileValue.assign((height + 2) * W, 0);
        dist.assign((height + 2) * W, DIST_INF);
        tileTypeMemory.assign((height + 2) * W, tile_type_e(-1));
        tileArmyMemory.assign((height + 2) * W, 0);
        inPrevMoves.assign((height + 2) * W, false);
        isSeenBefore.assign((height + 2) * W, false);
    }

    void requestMove(const BoardView& boardView,
                     const std::vector<game::RankItem>& _rank) override {
        ++halfTurn;
        turn += (halfTurn & 1);

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

        for (pos_t i = 1; i <= height; ++i) {
            for (pos_t j = 1; j <= width; ++j) {
                if (tileArmyMemory[idx(i, j)] > 0) --tileArmyMemory[idx(i, j)];
                if (board.tileAt(i, j).visible ||
                    board.tileAt(i, j).type == TILE_SWAMP) {
                    isSeenBefore[idx(i, j)] = true;
                }
                if (board.tileAt(i, j).visible) {
                    isSeenBefore[idx(i, j)] = true;
                    tileTypeMemory[idx(i, j)] = board.tileAt(i, j).type;
                    tileArmyMemory[idx(i, j)] = board.tileAt(i, j).army;
                } else if (!isSeenBefore[idx(i, j)]) {
                    switch (typeAt(i, j)) {
                        case TILE_BLANK: tileArmyMemory[idx(i, j)] = 0; break;
                        case TILE_SWAMP: tileArmyMemory[idx(i, j)] = 0; break;
                        case TILE_MOUNTAIN:
                            tileArmyMemory[idx(i, j)] = INF;
                            break;
                        case TILE_SPAWN:
                            tileArmyMemory[idx(i, j)] = -INF;
                            break;
                        case TILE_CITY:   tileArmyMemory[idx(i, j)] = 40; break;
                        case TILE_DESERT: tileArmyMemory[idx(i, j)] = 40; break;
                        case TILE_LOOKOUT:
                            tileArmyMemory[idx(i, j)] = INF;
                            break;
                        case TILE_OBSERVATORY:
                            tileArmyMemory[idx(i, j)] = INF;
                            break;
                        case TILE_OBSTACLE:
                            tileArmyMemory[idx(i, j)] = 40;
                            break;
                    }
                }
                if (board.tileAt(i, j).visible &&
                    board.tileAt(i, j).type == TILE_GENERAL)
                    generals[board.tileAt(i, j).occupier] = Coord(i, j);
            }
        }
        if (board.tileAt(focus).occupier != id ||
            board.tileAt(focus).army == 0) {
            focus = findMaxArmyPos();
            leastUsage = 0;
        }
        if (leastUsage != 0) {
            --leastUsage;
            auto next = route.front();
            route.pop_front();
            moveQueue.emplace_back(MoveType::MOVE_ARMY, focus, next, false);
        }
        mode = BotMode::EXPLORE;
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

        calcData(focus);

        if (mode == BotMode::ATTACK) {
            findRoute(focus, generals[targetOppoId]);
            if (!route.empty()) {
                Move ret =
                    Move(MoveType::MOVE_ARMY, focus, route.front(), false);
                focus = route.front();
                route.pop_front();
                leastUsage = std::min((pos_t)route.size(), (0));
                moveQueue.emplace_back(ret);
            }
        } else if (mode == BotMode::EXPLORE) {
            Coord bestTarget = focus;
            value_t bestValue = -INF;
            for (pos_t i = 1; i <= height; ++i) {
                for (pos_t j = 1; j <= width; ++j) {
                    if (tileValue[idx(i, j)] > bestValue) {
                        bestValue = tileValue[idx(i, j)];
                        bestTarget = Coord(i, j);
                    }
                }
            }
            findRoute(focus, bestTarget);
            if (!route.empty()) {
                Move ret =
                    Move(MoveType::MOVE_ARMY, focus, route.front(), false);
                focus = route.front();
                route.pop_front();
                leastUsage = std::min((pos_t)route.size(), (0));
                moveQueue.emplace_back(ret);
            }
        }
    }
};

static BotRegistrar<ZlyBot_v2> zlyBot_v2_reg("ZlyBot v2");

#endif  // LGEN_BOTS_ZLYBOT_V2