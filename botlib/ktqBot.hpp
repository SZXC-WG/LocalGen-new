#ifndef __BOT_KTQ__
#define __BOT_KTQ__

namespace ktqBot
{
    const int dx[5] = {0, -1, 0, 1, 0};
    const int dy[5] = {0, 0, -1, 0, 1};

    int calcNextMove(int id, playerCoord coo)
    {
        static std::mt19937 mtrd(std::chrono::system_clock::now().time_since_epoch().count());
        using ll = long long;
        static int swampDir[20] = {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3};
        if (gameMap[coo.x][coo.y].team != id || gameMap[coo.x][coo.y].army == 0)
            return 0;
        struct node
        {
            int to, team;
            ll army, del;
            int type;
            bool operator<(node b)
            {
                return army < b.army || (army == b.army && del < b.del);
            }
        };
        node p[10];
        int cnt = 0;
        for (int i = 1; i <= 4; i++)
        {
            int tx = coo.x + dx[i], ty = coo.y + dy[i];
            if (gameMap[tx][ty].type == 2 || tx < 1 || tx > mapH || ty < 1 || ty > mapW)
                continue;
            p[++cnt] = {i, gameMap[tx][ty].player, gameMap[tx][ty].army, gameMap[tx][ty].army, gameMap[tx][ty].type};
            if (p[cnt].type != 1 && p[cnt].team == id)
                p[cnt].army = -p[cnt].army, p[cnt].del = -p[cnt].del;
            if (p[cnt].type == 4 && p[cnt].team != id)
                p[cnt].army = 2 * p[cnt].army - ll(1e15);
            else if (p[cnt].type == 0 && p[cnt].team != id)
                p[cnt].army = p[cnt].army - ll(1e15);
            else if (p[cnt].type == 1)
            {
                p[cnt].del = 200;
                p[cnt].army = -1e16;
            }
            else if (p[cnt].type == 3 && p[cnt].team != id)
                p[cnt].army = -ll(1e18);
        }
        std::sort(p + 1, p + cnt + 1);
        //	gotoxy(mapH+2+16+1+id,1); clearline();
        //	fputs(playerInfo[id].name.c_str(),stdout);
        //	printf(": ");
        //	for(int i=1; i<=cnt; ++i) printf("{%d %d %lld %lld %d} ",p[i].to,p[i].team,p[i].army,p[i].del,p[i].type);
        //	fflush(stdout); _getch();
        for (int i = 1; i <= cnt; i++)
        {
            if (p[i].del < gameMap[coo.x][coo.y].army)
                return p[i].to;
        }
        return -1;
    }
}

#endif // __BOT_KTQ__
