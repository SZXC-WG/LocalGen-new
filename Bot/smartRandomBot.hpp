#include <algorithm>
#include <vector>
#include "../LGmaps.hpp"

namespace smartRandomBot
{
    const int dx[5] = {0, -1, 0, 1, 0};
    const int dy[5] = {0, 0, -1, 0, 1};

    int smartRandomBot(int id, playerCoord coo)
    {
        static std::mt19937 mtrd(std::chrono::system_clock::now().time_since_epoch().count());
        if (gameMap[coo.x][coo.y].team != id || gameMap[coo.x][coo.y].army == 0)
            return 0;
        struct node
        {
            int type, team;
            long long army;
            int dir;
        };
        node p[5];
        int pl = 0;
        for (int i = 1; i <= 4; ++i)
        {
            if (coo.x + dx[i] < 1 || coo.x + dx[i] > mapH || coo.y + dy[i] < 1 || coo.y + dy[i] > mapW || gameMap[coo.x + dx[i]][coo.y + dy[i]].type == 2)
                continue;
            p[++pl] = {gameMap[coo.x + dx[i]][coo.y + dy[i]].type, gameMap[coo.x + dx[i]][coo.y + dy[i]].team, gameMap[coo.x + dx[i]][coo.y + dy[i]].army, i};
        }
        bool rdret = mtrd() % 2;
        auto cmp = [&](node a, node b) -> bool
        {
            if (a.type == 3 && a.team != id)
                return true;
            if (b.type == 3 && b.team != id)
                return false;
            if (a.team == 0)
                return rdret;
            if (b.team == 0)
                return !rdret;
            if (a.team == id && b.team != id)
                return false;
            if (a.team != id && b.team == id)
                return true;
            if (a.team == id && b.team == id)
                return a.army > b.army;
            return a.army < b.army;
        };
        std::sort(p + 1, p + pl + 1, cmp);
        return p[1].dir;
    }
}