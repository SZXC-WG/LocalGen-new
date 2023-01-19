/*
By xiaruize
Bugs not fixed
testing version
*/

#include "LGmaps.hpp"
#include<bits/stdc++.h>
using namespace std;

namespace xiaruizeBot{
    const int dx[5] = {0, -1, 0, 1, 0};
    const int dy[5] = {0, 0, -1, 0, 1};
    int checkOrder[5]={0,1,2,3,4};
    vector<int> operation[20];
    bool vis[20][505][505];
    static std::mt19937 mtrd(std::chrono::system_clock::now().time_since_epoch().count());
    int sendArmyProcess[20];
    int backCountCnt[20];

    int changeDirection(int x,int &res)
    {
        switch (x)
        {
        case 1:
            res = 3;
            break;
        case 2:
            res = 4;
            break;
        case 4:
            res = 2;
            break;
        case 3:
            res = 1;
            break;
        }
    }

    int dfs(int id,playerCoord coord)
    {
        random_shuffle(checkOrder + 1, checkOrder + 4);
        int x;
        for (int i = 1; i <= 4;i++)
        {
            x = checkOrder[i]; 
            int ToX = coord.x + dx[x];
            int ToY = coord.y + dy[x];
            if(ToX<1||ToX>mapH||ToY<1||ToY>mapW)
                continue;
            if(gameMap[ToX][ToY].type==2)
                continue;
            if(gameMap[ToX][ToY].type==3&&gameMap[ToX][ToY].team!=id)
            {
                operation[id].push_back(x);
                return x;
            }
        }
        for (int i = 1; i <= 4; i++)
        {
            x = checkOrder[i];
            int ToX = coord.x + dx[x];
            int ToY = coord.y + dy[x];
            if (ToX < 1 || ToX > mapH || ToY < 1 || ToY > mapW)
                continue;
            if (gameMap[ToX][ToY].type == 2)
                continue;
            if (gameMap[ToX][ToY].type == 4 && gameMap[ToX][ToY].army<=gameMap[coord.x][coord.y].army&&gameMap[ToX][ToY].team!=id)
            {
                operation[id].push_back(x);
                return x;
            }
        }
        for (int i = 1; i <= 4; i++)
        {
            x = checkOrder[i];
            int ToX = coord.x + dx[x];
            int ToY = coord.y + dy[x];
            if(ToX<1||ToX>mapH||ToY<1||ToY>mapW)
                continue;
            if(gameMap[ToX][ToY].type==2)
                continue;
            if (vis[id][ToX][ToY])
                continue;
            operation[id].push_back(x);
            return x;
        }
        return -1;
    }

    int xiaruizeBot(int id,playerCoord coord)
    {
        if(gameMap[coord.x][coord.y].army==0||gameMap[coord.x][coord.y].team!=id)
        {
            backCountCnt[id] = 1;
            sendArmyProcess[id] = 1;
            return 0;
        }
        vis[id][coord.x][coord.y] = true;
        int returnValue = dfs(id, coord);
        if(returnValue!=-1)
        {
            backCountCnt[id] = 1;
            sendArmyProcess[id] = 0;
            return returnValue;
        }
        if(sendArmyProcess[id])
        {
            if(operation[id].size()>=sendArmyProcess[id])
            {
                backCountCnt[id] = 1;
                int res = operation[id][sendArmyProcess[id] - 1];
                sendArmyProcess[id]++;
                return res;
            }
            else
            {
                sendArmyProcess[id] = 0;
            }
        }
        else
        {
            int res;
            changeDirection(operation[id][operation[id].size() -1], res);
            operation[id].pop_back();
            backCountCnt[id]++;
            return res;
        }
    }
}
