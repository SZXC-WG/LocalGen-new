#ifndef __BOT_ZLY_V2__
#define __BOT_ZLY_V2__

#include <queue>

#include "../LGdef.hpp"

#define ll long long

// #define DEBUG_ZLY_V2

namespace zlyBot_v2 {
const ll INF = 10'000'000'000'000'000LL;
enum botModeE {
    BOT_MODE_ATTACK,
    BOT_MODE_EXPLORE,
    BOT_MODE_DEFEND,
};
botModeE botModes[64];
coordS seenGenerals[64][64];
deque<coordS> stackedMoves[64];
int leastUsage[64];
vector<ll> blockValueWeight[64];  // p s m g c (o)
ll blockValue[64][505][505];
ll dist[64][505][505];
int blockTypeRem[64][505][505];
ll blockArmyRem[64][505][505];
deque<coordS> previousMoves[64];
bool inPreviousMoves[64][505][505];
bool isSeenBefore[64][505][505];
// int passedTimes[64][505][505];
#ifdef DEBUG_ZLY_V2
std::wofstream db;
#endif

inline void recordNewMove(int playerId, coordS position) {
    previousMoves[playerId].push_back(position);
    inPreviousMoves[playerId][position.x][position.y] = true;
    if (previousMoves[playerId].size() > 20) {
        auto front = previousMoves[playerId].front();
        previousMoves[playerId].pop_front();
        inPreviousMoves[playerId][front.x][front.y] = false;
    }
}

// tool to make block access legal
inline int getType(int id, int x, int y) {
    if (blockTypeRem[id][x][y] != -1)
        return blockTypeRem[id][x][y];
    else if (isVisible(x, y, 1 << id))
        return blockTypeRem[id][x][y] = gameMap[x][y].type;
    else if (gameMap[x][y].type == 2 || gameMap[x][y].type == 4)
        return 5;
    else if (gameMap[x][y].type == 3)
        return 0;
    else
        return gameMap[x][y].type;
}
inline ll getArmy(int id, int x, int y) {
    if (isVisible(x, y, 1 << id))
        return blockArmyRem[id][x][y] = gameMap[x][y].army;
    else
        return blockArmyRem[id][x][y];
}

inline void initBot(int botId) {
    // memset(passedTimes[botId], 0, sizeof(passedTimes[botId]));
    memset(isSeenBefore[botId], 0, sizeof(isSeenBefore[botId]));
    blockValueWeight[botId] = {
        300 - LGset::plainRate[LGset::gameMode] * 10 + 10,
        -1500000000,
        -INF,
        10,
        300,
        0,
        -INF,
        -INF,
        300};
    for (int playerId = 1; playerId <= LGgame::playerCnt; ++playerId)
        seenGenerals[botId][playerId] = coordS(-1, -1);
    memset(blockTypeRem[botId], -1, sizeof(blockTypeRem[botId]));
    memset(blockArmyRem[botId], 0, sizeof(blockArmyRem[botId]));
    for (int i = 1; i <= mapH; ++i) {
        for (int j = 1; j <= mapW; ++j) {
            switch (getType(botId, i, j)) {
                case 0: blockArmyRem[botId][i][j] = 0; break;
                case 1: blockArmyRem[botId][i][j] = 0; break;
                case 2: blockArmyRem[botId][i][j] = INF; break;
                case 3: blockArmyRem[botId][i][j] = -INF; break;
                case 4: blockArmyRem[botId][i][j] = 40; break;
                case 5: blockArmyRem[botId][i][j] = 40; break;
            }
        }
    }
#ifdef DEBUG_ZLY_V2
    std::wofstream db("player_"s + to_string(botId) + "_debug.txt");
    db << "This is the debug file of the bot illustrating player " << botId
       << " (" << playerInfo[botId].name << ")." << std::endl;
    db.close();
#endif
}

inline void calcData(int playerId, coordS position) {
    constexpr int delta_x[] = {-1, 0, 1, 0};
    constexpr int delta_y[] = {0, -1, 0, 1};
    memset(dist[playerId], 0x3f, sizeof(dist[playerId]));
    dist[playerId][position.x][position.y] = 0;
    std::priority_queue<std::pair<ll, coordS>,
                        std::vector<std::pair<ll, coordS>>, std::greater<>>
        queue;
    queue.push({0, position});
#ifdef DEBUG_ZLY_V2
    db << "in calcData bfs:" << std::endl;
#endif
    while (!queue.empty()) {
        coordS current = queue.top().second;
        ll currentDist = queue.top().first;
        queue.pop();
        if (currentDist > dist[playerId][current.x][current.y]) continue;
        for (int i = 0; i < 4; ++i) {
            coordS next = current + coordS(delta_x[i], delta_y[i]);
            if (next.x < 1 || next.x > mapH || next.y < 1 || next.y > mapW)
                continue;
            if (unpassable(getType(playerId, next.x, next.y))) continue;
            ll newDist = currentDist + 10;
            if (isVisible(next.x, next.y, 1 << playerId)) {
                if (gmp(next) != playerId) newDist += max(gma(next), 0ll);
            } else
                newDist += max(getArmy(playerId, next.x, next.y), 0ll);
            if (getType(playerId, next.x, next.y) == BLOCK_SWAMP)
                newDist += 100;
            if (newDist < dist[playerId][next.x][next.y]) {
                dist[playerId][next.x][next.y] = newDist;
                queue.push({newDist, next});
            }
        }
    }
#ifdef DEBUG_ZLY_V2
    db << "end of calcData bfs." << std::endl;
#endif
    for (int i = 1; i <= mapH; ++i) {
        for (int j = 1; j <= mapW; ++j) {
            if (isVisible(i, j, 1 << playerId) ||
                getType(playerId, i, j) == BLOCK_SWAMP)
                isSeenBefore[playerId][i][j] = true;
            if (gameMap[i][j].player == playerId)
                blockValue[playerId][i][j] = -INF;
            else {
                blockValue[playerId][i][j] =
                    blockValueWeight[playerId][getType(playerId, i, j)];
                blockValue[playerId][i][j] -= dist[playerId][i][j];
                blockValue[playerId][i][j] -= getArmy(playerId, i, j);
                if (LGset::gameMode == 0)
                    blockValue[playerId][i][j] -=
                        isSeenBefore[playerId][i][j] *
                        (LGgame::curTurn -
                         100000.0 / LGgame::gameStats[playerId].back().army);
                // blockValue -= passedTimes[playerId][i][j];
                if (isVisible(i, j, 1 << playerId) &&
                    gameMap[i][j].player != 0) {
                    ll adjacent_minimum_same_player = INF;
                    for (int k = 0; k < 4; ++k) {
                        coordS adja =
                            coordS(i, j) + coordS(delta_x[k], delta_y[k]);
                        if (adja.x < 1 || adja.x > mapH || adja.y < 1 ||
                            adja.y > mapW)
                            continue;
                        if (isVisible(adja.x, adja.y,
                                      1 << playerId && gmp(adja) == playerId))
                            adjacent_minimum_same_player =
                                min(adjacent_minimum_same_player,
                                    dist[playerId][adja.x][adja.y]);
                    }
                    if (adjacent_minimum_same_player == INF)
                        adjacent_minimum_same_player = gma(i, j);
                    blockValue[playerId][i][j] +=
                        gma(i, j) - adjacent_minimum_same_player;
                }
            }
        }
    }
}

inline void findRoute(int playerId, coordS start, coordS destination) {
    // integrated value bfs
    // distance & needed.army
    auto DisInc = 1;
    auto ArmyInc = [&](int x, int y) -> ll {
        if (x < 1 || x > mapH || y < 1 || y > mapW) return INF;
        if (unpassable(getType(playerId, x, y))) return INF;
        if (gameMap[x][y].player == playerId)
            return -gameMap[x][y].army;
        else
            return getArmy(playerId, x, y);
    };
    auto TypeInc = [&](int x, int y) -> ll {
        switch (getType(playerId, x, y)) {
            case 0:  return 0;
            case 1:  return 10;
            case 2:  return INF;
            case 3:  return 0;
            case 4:  return 1;
            case 5:  return 0;
            case 6:  return INF;
            case 7:  return INF;
            case 8:  return 5;
            default: return INF;
        }
    };
    auto UnitedInc = [&](int x, int y) -> ll {
        return DisInc * 1000 + ArmyInc(x, y) +
               TypeInc(x, y) /*  + passedTimes[playerId][x][y] */;
    };
    constexpr int dx[] = {-1, 0, 1, 0};
    constexpr int dy[] = {0, -1, 0, 1};
    vector<vector<bool>> vis(mapH + 1, vector<bool>(mapW + 1, false));
    vector<vector<coordS>> prev(mapH + 1,
                                vector<coordS>(mapW + 1, coordS(-1, -1)));
    vector<vector<ll>> dp(mapH + 1, vector<ll>(mapW + 1, INF));
    std::priority_queue<std::pair<ll, coordS>, vector<std::pair<ll, coordS>>,
                        std::greater<std::pair<ll, coordS>>>
        q;
    dp[start.x][start.y] = 0;
    q.push({0, start});
    int cnt = 0;
    while (!q.empty()) {
        coordS current = q.top().second;
        ll currentVal = q.top().first;
        q.pop();
        if (currentVal > dp[current.x][current.y]) continue;
        // printf("zlyBot v2: (findRoute) LINE %d output: IN LOOP %d times:
        // cur(%d,%d) VAL(%lld)\n",
        //        __LINE__, ++cnt, current.x, current.y, currentVal);
        if (vis[current.x][current.y]) continue;
        vis[current.x][current.y] = true;
        if (current == destination) break;
        for (int i = 0; i < 4; ++i) {
            coordS next = current + coordS(dx[i], dy[i]);
            if (next.x < 1 || next.x > mapH || next.y < 1 || next.y > mapW)
                continue;
            if (unpassable(getType(playerId, next.x, next.y))) continue;
            if (vis[next.x][next.y]) continue;
            if (inPreviousMoves[playerId][next.x][next.y]) continue;
            ll nextVal = currentVal + UnitedInc(next.x, next.y);
            if (nextVal < dp[next.x][next.y]) {
                dp[next.x][next.y] = nextVal;
                prev[next.x][next.y] = current;
                q.push({nextVal, next});
            }
        }
    }
    stackedMoves[playerId].clear();
    coordS current = destination;
    while (current != coordS(-1, -1)) {
        stackedMoves[playerId].push_front(current);
        current = prev[current.x][current.y];
    }
    stackedMoves[playerId].pop_front();
    return;
}

moveS calcNextMove(int playerId, coordS currentPos) {
    // printf("zlyBot v2: ID(%d), COORD(%d,%d)\n", playerId, currentPos.x,
    // currentPos.y);
    for (int row = 1; row <= mapH; ++row) {
        for (int col = 1; col <= mapW; ++col) {
            if (blockArmyRem[playerId][row][col] > 0)
                --blockArmyRem[playerId][row][col];
        }
    }
    for (int row = 1; row <= mapH; ++row)
        for (int col = 1; col <= mapW; ++col)
            if (isVisible(row, col, 1 << playerId) &&
                gameMap[row][col].player != playerId &&
                gameMap[row][col].type == 3)
                seenGenerals[playerId][gameMap[row][col].player] =
                    coordS(row, col);
#ifdef DEBUG_ZLY_V2
    db.open("player_"s + to_string(playerId) + "_debug.txt"s, std::ios::app);
    db << std::endl;
#endif
    if (gameMap[currentPos.x][currentPos.y].player != playerId ||
        gameMap[currentPos.x][currentPos.y].army == 0) {
        long long maxArmy = 0;
        coordS maxCoo = LGgame::genCoo[playerId];
        for (int row = 1; row <= mapH; ++row) {
            for (int col = 1; col <= mapW; ++col) {
                if (gameMap[row][col].player == playerId) {
                    if (gameMap[row][col].army > maxArmy) {
                        maxArmy = gameMap[row][col].army;
                        maxCoo = coordS{row, col};
                    }
                }
            }
        }
        leastUsage[playerId] = 0;
        currentPos = maxCoo;
    }
    // ++passedTimes[playerId][currentPos.x][currentPos.y];
    if (leastUsage[playerId] != 0) {
        --leastUsage[playerId];
        auto next = stackedMoves[playerId].front();
        stackedMoves[playerId].pop_front();
        return moveS{playerId, true, currentPos, next};
    }
    botModes[playerId] = BOT_MODE_EXPLORE;
    int targetGeneralId = -1;
    ll targetGeneralArmy = INF;
    for (int i = 1; i <= LGgame::playerCnt; ++i) {
        if (seenGenerals[playerId][i] != coordS(-1, -1) && LGgame::isAlive[i]) {
            botModes[playerId] = BOT_MODE_ATTACK;
            if (LGgame::gameStats[i].back().army < targetGeneralArmy) {
                targetGeneralArmy = LGgame::gameStats[i].back().army;
                targetGeneralId = i;
            }
        }
    }
    calcData(playerId, currentPos);
    if (botModes[playerId] == BOT_MODE_ATTACK) {
#ifdef DEBUG_ZLY_V2
        db << "Desti: " << seenGenerals[playerId][targetGeneralId].x << ","
           << seenGenerals[playerId][targetGeneralId].y << std::endl;
#endif
        findRoute(playerId, currentPos,
                  seenGenerals[playerId][targetGeneralId]);
        // recordNewMove(playerId, stackedMoves[playerId].front());
        moveS ret =
            moveS{playerId, true, currentPos, stackedMoves[playerId].front()};
#ifdef DEBUG_ZLY_V2
        db << "stackedMoves size: " << stackedMoves[playerId].size()
           << std::endl;
        db << "stackedMoves:";
        for (auto& i : stackedMoves[playerId])
            db << " (" << i.x << "," << i.y << ")";
        db << std::endl;
#endif
        stackedMoves[playerId].pop_front();
        leastUsage[playerId] = min((int)stackedMoves[playerId].size(), (0));
#ifdef DEBUG_ZLY_V2
        db << "Turn " << LGgame::curTurn << " from (" << ret.from.x << ","
           << ret.from.y << ") to (" << ret.to.x << "," << ret.to.y << ")"
           << std::endl;
        db.close();
#endif
        return ret;
    } else if (botModes[playerId] == BOT_MODE_EXPLORE) {
        struct node {
            coordS pos;
            ll value;
        };
        vector<node> nodes;
        for (int row = 1; row <= mapH; ++row)
            for (int col = 1; col <= mapW; ++col)
                nodes.push_back(
                    node{coordS(row, col), blockValue[playerId][row][col]});
        sort(nodes.begin(), nodes.end(),
             [](const node& a, const node& b) { return a.value > b.value; });
#ifdef DEBUG_ZLY_V2
        db << "Desti: " << nodes[0].pos.x << "," << nodes[0].pos.y << std::endl;
        db << "Desti value: " << nodes[0].value << std::endl;
        db << "Next 4 unselected nodes:";
        for (int i = 1; i < 5; ++i)
            db << " (" << nodes[i].pos.x << "," << nodes[i].pos.y << ";"
               << nodes[i].value << ")";
        db << std::endl;
#endif
        findRoute(playerId, currentPos, nodes[0].pos);
#ifdef DEBUG_ZLY_V2
        db << "stackedMoves size: " << stackedMoves[playerId].size()
           << std::endl;
        db << "stackedMoves:";
        for (auto& i : stackedMoves[playerId])
            db << " (" << i.x << "," << i.y << ")";
        db << std::endl;
#endif
        moveS ret =
            moveS{playerId, true, currentPos, stackedMoves[playerId].front()};
        stackedMoves[playerId].pop_front();
        leastUsage[playerId] = min((int)stackedMoves[playerId].size(), (0));
#ifdef DEBUG_ZLY_V2
        db << "Turn " << LGgame::curTurn << " from (" << ret.from.x << ","
           << ret.from.y << ") to (" << ret.to.x << "," << ret.to.y << ")"
           << std::endl;
        db.close();
#endif
        return ret;
    }
    return moveS{playerId, false, currentPos, currentPos};
}
}  // namespace zlyBot_v2

#undef ll

#endif  // __BOT_ZLY_V2__
