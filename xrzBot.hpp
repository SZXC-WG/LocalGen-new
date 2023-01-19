#include "LGmaps.hpp"
#include <bits/stdc++.h>
using namespace std;
const int dx[5] = {0, -1, 0, 1, 0};
const int dy[5] = {0, 0, -1, 0, 1};
static int armyNow;
playerCoord previousPos[16];
static int visitTime[305][305][16];
static int id;
int turnind[16];

int xrzBot(int ind, playerCoord player)
{
    static std::mt19937 mtrd(std::chrono::system_clock::now().time_since_epoch().count());
    armyNow = gameMap[player.x][player.y].army;
    turnind[ind]++;
    id = ind;
    visitTime[player.x][player.y][id]++;
    if (gameMap[player.x][player.y].army == max(0,turnind[id]-100) || gameMap[player.x][player.y].team != id)
        return 0;
    struct node
    {
        int x, y;
        int type;
        int Army;
        int teamOnIt;
        int direction;
    };
    vector<node> operat;
    for (int i = 1; i <= 4; i++)
    {
        node des;
        des.direction = i;
        des.x = player.x + dx[i];
        des.y = player.y + dy[i];
        if (gameMap[des.x][des.y].type == 2)
            continue;
        if (des.x < 1 || des.x > mapH || des.y < 1 || des.y > mapW)
            continue;
        des.type = gameMap[des.x][des.y].type;
        des.Army = gameMap[des.x][des.y].army;
        des.teamOnIt = gameMap[des.x][des.y].team;
        if (gameMap[des.x][des.y].team != id && gameMap[des.x][des.y].type == 3)
        {
            previousPos[id] = player;
            return i;
        }
        if (des.type == 4 && des.Army <= gameMap[player.x][player.y].army && des.teamOnIt != id)
        {
            previousPos[id] = player;
            return i;
        }
    }
    for (int i = 1; i <= 4; i++)
    {
        node des;
        des.direction = i;
        des.x = player.x + dx[i];
        des.y = player.y + dy[i];
        if (gameMap[des.x][des.y].type == 2)
            continue;
        if (des.x < 1 || des.x > mapH || des.y < 1 || des.y > mapW)
            continue;
        des.type = gameMap[des.x][des.y].type;
        des.Army = gameMap[des.x][des.y].army;
        des.teamOnIt = gameMap[des.x][des.y].team;
        if (gameMap[des.x][des.y].team != id && gameMap[des.x][des.y].type == 3)
            return i;
        if (des.type == 4 && des.Army <= gameMap[player.x][player.y].army)
            return i;
        int cnt = 4;
        if (des.x == previousPos[id].x && des.y == previousPos[id].y)
            cnt += 2;
        if (des.teamOnIt != id && des.teamOnIt != 0)
            cnt--;
        if (des.type == 0)
            cnt--;
        if (des.type == 1)
            cnt++;
        if (des.teamOnIt == 0)
            cnt--;
        cnt += max(0, (int)log2(visitTime[des.x][des.y][id]));
        if (mtrd() % cnt == 0)
        {
            previousPos[id] = player;
            return i;
        }
    }
    previousPos[id] = player;
    return mtrd() % 4;
}
