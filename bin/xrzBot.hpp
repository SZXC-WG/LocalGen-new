#ifndef __BOT_XRZ__
#define __BOT_XRZ__

#include "../LGdef.hpp"
namespace xrzBot
{
    const int dx[5] = {0, -1, 0, 1, 0};
    const int dy[5] = {0, 0, -1, 0, 1};
    static int armyNow;
    int checkOrder[5] = {0, 1, 2, 3, 4};
    playerCoord previousPos[17];
    static int visitTime[17][505][505];
    static int turnCount[17];
    static int id;

    int xrzBot(int ind, playerCoord player)
    {
        srand(time(0));
        static std::mt19937 mtrd(std::chrono::system_clock::now().time_since_epoch().count());
        armyNow = gameMap[player.x][player.y].army;
        id = ind;
        turnCount[id]++;
        visitTime[id][player.x][player.y]++;
        if (gameMap[player.x][player.y].army == 0 || gameMap[player.x][player.y].player != id)
        {
            memset(visitTime[id], 0, sizeof(visitTime[id]));
            return 0;
        }
        struct node
        {
            int x, y;
            int type;
            int Army;
            int teamOnIt;
            int direction;
        };
        std::vector<node> operat;
        std::random_shuffle(checkOrder + 1, checkOrder + 5);
        int okDir = 0;
        for (int j = 1; j <= 4; j++)
        {
            int i = checkOrder[j];
            node des;
            des.direction = i;
            des.x = player.x + dx[i];
            des.y = player.y + dy[i];
            if (gameMap[des.x][des.y].type == 2)
                continue;
            if (des.x < 1 || des.x > mapH || des.y < 1 || des.y > mapW)
                continue;
            okDir = i;
            des.type = gameMap[des.x][des.y].type;
            des.Army = gameMap[des.x][des.y].army;
            des.teamOnIt = gameMap[des.x][des.y].player;
            if (gameMap[des.x][des.y].player != id && gameMap[des.x][des.y].type == 3)
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
        int i;
        int timeToTry = 1000;
        while (timeToTry--)
        {
            i = (mtrd() % 4 + rand() % 4 + rand() * rand() % 4) % 4 + 1;
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
            des.teamOnIt = gameMap[des.x][des.y].player;
            if (gameMap[des.x][des.y].player != id && gameMap[des.x][des.y].type == 3)
                return i;
            if (des.type == 4 && des.Army <= gameMap[player.x][player.y].army && des.teamOnIt == 0)
                return i;
            int cnt = 4;
            if (des.x == previousPos[id].x && des.y == previousPos[id].y)
                cnt += turnCount[id] * 10;
            if (des.teamOnIt != id && des.teamOnIt != 0)
                cnt--;
            if (des.type == 0)
                cnt--;
            if (des.type == 1)
                cnt += 2;
            if (des.teamOnIt == 0)
                cnt--;
            if (des.teamOnIt == id && des.Army >= 2000)
                cnt--;
            cnt += std::max(0, visitTime[id][des.x][des.y] * 10);
            if (mtrd() % cnt == 0)
            {
                previousPos[id] = player;
                return i;
            }
        }
        return okDir;
    }
}

#endif // __BOT_XRZ__
