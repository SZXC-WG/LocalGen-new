/**
 * @file zlyBot_v2.1.cpp
 *
 * ZlyBot v2.1 from LocalGen v5.
 *
 * @copyright Copyright (c) SZXC Work Group.
 */

#ifndef LGEN_BOTS_ZLYBOT_V2_1
#define LGEN_BOTS_ZLYBOT_V2_1

#include <queue>
#include <random>

#include "../GameEngine/bot.h"
#include "../GameEngine/game.hpp"

class ZlyBot_v2_1 : public BasicBot {
   private:
    using value_t = intmax_t;
    constexpr static pos_t DIST_INF = 32767;
    constexpr static int64_t INF = 10'000'000'000'000'000LL;
    constexpr static Coord delta[] = {{-1, 0}, {0, -1}, {1, 0}, {0, 1}};

    enum class BotMode { ATTACK, EXPLORE, DEFEND };
    BotMode mode;

    pos_t height, width;
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

    Coord focus[2];
    std::vector<bool> alive;
    std::vector<Coord> generals;
    std::deque<Coord> route;
    pos_t leastUsage;
    value_t tileTypeWeight[16];
    std::vector<std::vector<value_t>> tileValue;
    std::vector<std::vector<value_t>> tileDanger;
    std::vector<std::vector<value_t>> dist0;
    std::vector<std::vector<value_t>> dist1;
    std::vector<std::vector<value_t>> distToSpawn;
    std::vector<Coord> homeZone;
    std::vector<std::vector<tile_type_e>> tileTypeMemory;
    std::vector<std::vector<army_t>> tileArmyMemory;
    std::deque<Coord> prevMoves;
    std::vector<std::vector<bool>> inPrevMoves;
    std::vector<std::vector<bool>> isSeenBefore;
    Coord spawnCoord;

    inline void recordNewMove(Coord pos) {
        prevMoves.emplace_back(pos);
        inPrevMoves[pos.x][pos.y] = true;
        if (prevMoves.size() > 20) {
            auto front = prevMoves.front();
            prevMoves.pop_front();
            inPrevMoves[front.x][front.y] = false;
        }
    }

    inline tile_type_e typeAt(pos_t x, pos_t y) {
        if (tileTypeMemory.at(x).at(y) != tile_type_e(-1))
            return tileTypeMemory[x][y];
        else if (board.tileAt(x, y).visible)
            return tileTypeMemory[x][y] = board.tileAt(x, y).type;
        else if (board.tileAt(x, y).type == TILE_MOUNTAIN ||
                 board.tileAt(x, y).type == TILE_CITY)
            return tileTypeMemory[x][y] = TILE_OBSTACLE;
        else if (board.tileAt(x, y).type == TILE_SPAWN)
            return tileTypeMemory[x][y] = TILE_BLANK;
        else
            return board.tileAt(x, y).type;
    }
    inline army_t armyAt(pos_t x, pos_t y) { return tileArmyMemory[x][y]; }

    void calcData(Coord foc0, Coord foc1) {
        {
            dist0.assign(height + 2, std::vector<value_t>(width + 2, DIST_INF));
            dist0[foc0.x][foc0.y] = 0;
            std::priority_queue<std::pair<value_t, Coord>,
                                std::vector<std::pair<value_t, Coord>>,
                                std::greater<>>
                queue;
            queue.emplace(0, foc0);
            while (!queue.empty()) {
                auto [curDist, cur] = queue.top();
                queue.pop();
                if (curDist > dist0[cur.x][cur.y]) continue;
                for (int i = 0; i < 4; ++i) {
                    Coord next = cur + delta[i];
                    if (next.x < 1 || next.x > height || next.y < 1 ||
                        next.y > width)
                        continue;
                    if (isImpassableTile(typeAt(next.x, next.y))) continue;
                    value_t newDist = curDist + 10;
                    if (board.tileAt(next).visible) {
                        if (board.tileAt(next).occupier != id)
                            newDist +=
                                std::max(armyAt(next.x, next.y), value_t(0));
                    } else
                        newDist += std::max(armyAt(next.x, next.y), value_t(0));
                    if (typeAt(next.x, next.y) == TILE_SWAMP) newDist += 100;
                    if (newDist < dist0[next.x][next.y]) {
                        dist0[next.x][next.y] = newDist;
                        queue.emplace(newDist, next);
                    }
                }
            }
        }
        {
            dist1.assign(height + 2, std::vector<value_t>(width + 2, DIST_INF));
            dist1[foc1.x][foc1.y] = 0;
            std::priority_queue<std::pair<value_t, Coord>,
                                std::vector<std::pair<value_t, Coord>>,
                                std::greater<>>
                queue;
            queue.emplace(0, foc1);
            while (!queue.empty()) {
                auto [curDist, cur] = queue.top();
                queue.pop();
                if (curDist > dist1[cur.x][cur.y]) continue;
                for (int i = 0; i < 4; ++i) {
                    Coord next = cur + delta[i];
                    if (next.x < 1 || next.x > height || next.y < 1 ||
                        next.y > width)
                        continue;
                    if (isImpassableTile(typeAt(next.x, next.y))) continue;
                    value_t newDist = curDist + 10;
                    if (board.tileAt(next).visible) {
                        if (board.tileAt(next).occupier != id)
                            newDist +=
                                std::max(armyAt(next.x, next.y), value_t(0));
                    } else
                        newDist += std::max(armyAt(next.x, next.y), value_t(0));
                    if (typeAt(next.x, next.y) == TILE_SWAMP) newDist += 100;
                    if (newDist < dist1[next.x][next.y]) {
                        dist1[next.x][next.y] = newDist;
                        queue.emplace(newDist, next);
                    }
                }
            }
        }
        {
            homeZone.clear();
            distToSpawn.assign(height + 2,
                               std::vector<value_t>(width + 2, DIST_INF));
            distToSpawn[spawnCoord.x][spawnCoord.y] = 0;
            std::priority_queue<std::pair<value_t, Coord>,
                                std::vector<std::pair<value_t, Coord>>,
                                std::greater<>>
                queue;
            queue.emplace(0, spawnCoord);
            auto defenseDist = std::min(
                20, std::max(3, static_cast<int>(rank[id].land / 15.0)));
            while (!queue.empty()) {
                auto [curDist, cur] = queue.top();
                queue.pop();
                if (curDist > distToSpawn[cur.x][cur.y]) continue;
                if (curDist <= defenseDist &&
                    !(typeAt(cur.x, cur.y) == TILE_SWAMP &&
                      (!board.tileAt(cur).visible ||
                       board.tileAt(cur).occupier == -1)))
                    homeZone.emplace_back(cur);
                for (int i = 0; i < 4; ++i) {
                    Coord next = cur + delta[i];
                    if (next.x < 1 || next.x > height || next.y < 1 ||
                        next.y > width)
                        continue;
                    if (isImpassableTile(typeAt(next.x, next.y))) continue;
                    value_t newDist = curDist + 1;
                    if (newDist < distToSpawn[next.x][next.y]) {
                        distToSpawn[next.x][next.y] = newDist;
                        queue.emplace(newDist, next);
                    }
                }
            }
        }

        for (pos_t i = 1; i <= height; ++i) {
            for (pos_t j = 1; j <= width; ++j) {
                if (board.tileAt(i, j).occupier == id) {
                    tileValue[i][j] = -INF;
                    tileDanger[i][j] = -INF;
                } else {
                    tileValue[i][j] = tileTypeWeight[typeAt(i, j)];
                    tileValue[i][j] -= dist0[i][j];
                    tileValue[i][j] -= armyAt(i, j);
                    tileDanger[i][j] = -tileTypeWeight[typeAt(i, j)];
                    tileDanger[i][j] -= distToSpawn[i][j] * 2;
                    tileDanger[i][j] -= dist1[i][j];
                    tileValue[i][j] -=
                        isSeenBefore[i][j] * (turn - 100000.0 / rank[id].army);
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
                        tileValue[i][j] += 2 * (board.tileAt(i, j).army -
                                                adjacent_minimum_same_player);
                    }
                    if (board.tileAt(i, j).visible) {
                        if (board.tileAt(i, j).occupier != -1)
                            tileDanger[i][j] += 2 * board.tileAt(i, j).army;
                        else
                            tileDanger[i][j] -= board.tileAt(i, j).army / 2;
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
                case TILE_SPAWN:       return 0;
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
        std::vector<std::vector<bool>> vis(height + 2,
                                           std::vector<bool>(width + 2, false));
        std::vector<std::vector<Coord>> prev(
            height + 2, std::vector<Coord>(width + 2, Coord(-1, -1)));
        std::vector<std::vector<value_t>> dp(
            height + 2, std::vector<value_t>(width + 2, INF));
        std::priority_queue<std::pair<value_t, Coord>,
                            std::vector<std::pair<value_t, Coord>>,
                            std::greater<std::pair<value_t, Coord>>>
            q;
        dp[start.x][start.y] = 0;
        q.emplace(0, start);
        while (!q.empty()) {
            Coord cur = q.top().second;
            value_t curVal = q.top().first;
            q.pop();
            if (curVal > dp[cur.x][cur.y]) continue;
            if (vis[cur.x][cur.y]) continue;
            vis[cur.x][cur.y] = true;
            if (cur == desti) break;
            for (int i = 0; i < 4; ++i) {
                Coord next = cur + delta[i];
                if (next.x < 1 || next.x > height || next.y < 1 ||
                    next.y > width)
                    continue;
                if (isImpassableTile(typeAt(next.x, next.y))) continue;
                if (vis[next.x][next.y]) continue;
                if (inPrevMoves[next.x][next.y]) continue;
                value_t nextVal = curVal + UnitedInc(next.x, next.y);
                if (nextVal < dp[next.x][next.y]) {
                    dp[next.x][next.y] = nextVal;
                    prev[next.x][next.y] = cur;
                    q.emplace(nextVal, next);
                }
            }
        }
        route.clear();
        Coord cur = desti;
        while (cur != Coord(-1, -1)) {
            route.push_front(cur);
            cur = prev[cur.x][cur.y];
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
                if (board.tileAt(i, j).occupier == id &&
                    board.tileAt(i, j).army > maxArmy) {
                    maxArmy = board.tileAt(i, j).army;
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
            if (board.tileAt(pos).visible && board.tileAt(pos).occupier == id) {
                if (board.tileAt(pos).army > maxArmy) {
                    maxArmy = board.tileAt(pos).army;
                    maxCoo = pos;
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
        playerCnt = constants.playerCount;
        teamIds = constants.teams;
        team = constants.teams.at(playerId);
        config = constants.config;

        halfTurn = turn = 0;
        leastUsage = 0;

        focus[0] = focus[1] = Coord(0, 0);
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

        tileValue.assign(height + 2, std::vector<value_t>(width + 2));
        tileDanger.assign(height + 2, std::vector<value_t>(width + 2));
        dist0.assign(height + 2, std::vector<value_t>(width + 2));
        dist1.assign(height + 2, std::vector<value_t>(width + 2));
        distToSpawn.assign(height + 2, std::vector<value_t>(width + 2));
        tileTypeMemory.assign(
            height + 2, std::vector<tile_type_e>(width + 2, tile_type_e(-1)));
        tileArmyMemory.assign(height + 2, std::vector<army_t>(width + 2, 0));
        inPrevMoves.assign(height + 2, std::vector<bool>(width + 2, false));
        isSeenBefore.assign(height + 2, std::vector<bool>(width + 2, false));
        mode = BotMode::EXPLORE;
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
                if (tileArmyMemory[i][j] > 0) --tileArmyMemory[i][j];
                if (board.tileAt(i, j).visible ||
                    board.tileAt(i, j).type == TILE_SWAMP) {
                    isSeenBefore[i][j] = true;
                }
                if (board.tileAt(i, j).visible) {
                    tileTypeMemory[i][j] = board.tileAt(i, j).type;
                    tileArmyMemory[i][j] = board.tileAt(i, j).army;
                } else if (!isSeenBefore[i][j]) {
                    switch (typeAt(i, j)) {
                        case TILE_BLANK:    tileArmyMemory[i][j] = 0; break;
                        case TILE_SWAMP:    tileArmyMemory[i][j] = 0; break;
                        case TILE_MOUNTAIN: tileArmyMemory[i][j] = INF; break;
                        case TILE_SPAWN:    tileArmyMemory[i][j] = -INF; break;
                        case TILE_CITY:     tileArmyMemory[i][j] = 40; break;
                        case TILE_DESERT:   tileArmyMemory[i][j] = 40; break;
                        case TILE_LOOKOUT:  tileArmyMemory[i][j] = INF; break;
                        case TILE_OBSERVATORY:
                            tileArmyMemory[i][j] = INF;
                            break;
                        case TILE_OBSTACLE: tileArmyMemory[i][j] = 40; break;
                    }
                }
                if (board.tileAt(i, j).visible &&
                    board.tileAt(i, j).type == TILE_GENERAL)
                    generals[board.tileAt(i, j).occupier] = Coord(i, j);
            }
        }
        if (generals[id] != Coord(-1, -1)) spawnCoord = generals[id];
        if (board.tileAt(focus[0]).occupier != id ||
            board.tileAt(focus[0]).army == 0) {
            focus[0] = findMaxArmyPos();
            leastUsage = 0;
        }
        if (board.tileAt(focus[1]).occupier != id ||
            board.tileAt(focus[1]).army == 0) {
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
        for (auto pos : homeZone) {
            if (!board.tileAt(pos).visible ||
                board.tileAt(pos).occupier != id) {
                homeZoneDanger += tileDanger[pos.x][pos.y] + 10;
            }
            if (board.tileAt(pos).visible && board.tileAt(pos).occupier != -1 &&
                board.tileAt(pos).occupier != id &&
                board.tileAt(pos).army > board.tileAt(spawnCoord).army)
                homeZoneThreat = true;
        }
        value_t homeZoneDangerAver =
            homeZoneDanger / static_cast<value_t>(homeZone.size());

        if ((mode == BotMode::ATTACK && homeZoneDangerAver > 5) ||
            (mode == BotMode::EXPLORE && homeZoneDangerAver > 0) ||
            homeZoneThreat) {
            mode = BotMode::DEFEND;
            focus[1] = findMaxArmyPosInHomeZone();
        } else if (mode == BotMode::DEFEND && homeZoneDanger <= 0) {
            mode = BotMode::EXPLORE;
        }

        if (mode == BotMode::ATTACK) {
            findRoute(focus[0], generals[targetOppoId]);
            if (!route.empty()) {
                Move ret =
                    Move(MoveType::MOVE_ARMY, focus[0], route.front(), false);
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
                    if (!isImpassableTile(typeAt(i, j)) &&
                        tileValue[i][j] > bestValue) {
                        bestValue = tileValue[i][j];
                        bestTarget = Coord(i, j);
                    }
                }
            }
            findRoute(focus[0], bestTarget);
            if (!route.empty()) {
                Move ret =
                    Move(MoveType::MOVE_ARMY, focus[0], route.front(), false);
                focus[0] = route.front();
                route.pop_front();
                leastUsage = std::min((pos_t)route.size(), pos_t(0));
                moveQueue.emplace_back(ret);
            }
        } else if (mode == BotMode::DEFEND) {
            Coord bestTarget = focus[1];
            value_t bestDanger = -INF;
            for (auto pos : homeZone) {
                if (tileDanger[pos.x][pos.y] > bestDanger) {
                    bestDanger = tileDanger[pos.x][pos.y];
                    bestTarget = pos;
                }
            }
            findRoute(focus[1], bestTarget);
            if (!route.empty()) {
                Move ret =
                    Move(MoveType::MOVE_ARMY, focus[1], route.front(), false);
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
