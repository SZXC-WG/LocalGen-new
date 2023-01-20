/* This is LGgame.hpp file of LocalGen.                                  */
/* Copyright (c) 2023 LocalGen-dev; All rights reserved.                 */
/* Developers: http://github.com/LocalGen-dev                            */
/* Project: http://github.com/LocalGen-dev/LocalGen-new                  */
/*                                                                       */
/* This project is licensed under the MIT license. That means you can    */
/* download, use and share a copy of the product of this project. You    */
/* may modify the source code and make contribution to it too. But, you  */
/* must print the copyright information at the front of your product.    */
/*                                                                       */
/* The full MIT license this project uses can be found here:             */
/* http://github.com/LocalGen-dev/LocalGen-new/blob/main/LICENSE.md      */

#ifndef __LGBOT_HPP__
#define __LGBOT_HPP__

#include <algorithm>
#include <vector>

namespace normalBot{
	const int dx[5] = {0,-1,0,1,0};
	const int dy[5] = {0,0,-1,0,1};
	
	int smartRandomBot(int id,playerCoord coo) {
		static std::mt19937 mtrd(std::chrono::system_clock::now().time_since_epoch().count());
		if(gameMap[coo.x][coo.y].team!=id||gameMap[coo.x][coo.y].army==0) return 0;
		struct node { int type,team; long long army; int dir; };
		node p[5]; int pl=0;
		for(int i=1; i<=4; ++i) {
			if(coo.x+dx[i]<1||coo.x+dx[i]>mapH||coo.y+dy[i]<1||coo.y+dy[i]>mapW||gameMap[coo.x+dx[i]][coo.y+dy[i]].type==2) continue;
			p[++pl]={gameMap[coo.x+dx[i]][coo.y+dy[i]].type,gameMap[coo.x+dx[i]][coo.y+dy[i]].team,gameMap[coo.x+dx[i]][coo.y+dy[i]].army,i};
		}
		bool rdret=mtrd()%2;
		auto cmp = [&](node a,node b)->bool {
			if(a.type==3&&a.team!=id) return true;
			if(b.type==3&&b.team!=id) return false;
			if(a.team==0) return rdret;
			if(b.team==0) return !rdret;
			if(a.team==id&&b.team!=id) return false;
			if(a.team!=id&&b.team==id) return true;
			if(a.team==id&&b.team==id) return a.army>b.army;
			return a.army<b.army;
		};
		std::sort(p+1,p+pl+1,cmp);
		return p[1].dir;
	}
	int ktqBot(int id,playerCoord coo){
		static std::mt19937 mtrd(std::chrono::system_clock::now().time_since_epoch().count());
		using ll = long long;
		static int swampDir[20]={3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3};
		if(gameMap[coo.x][coo.y].team!=id||gameMap[coo.x][coo.y].army==0) return 0;
		struct node{
			int to,team;
			ll army,del;
			int type;
			bool operator<(node b){
				return army<b.army||(army==b.army&&del<b.del);
			}
		};
		node p[10];
		int cnt=0;
		for(int i=1;i<=4;i++){
			int tx=coo.x+dx[i],ty=coo.y+dy[i];
			if(gameMap[tx][ty].type==2||tx<1||tx>mapH||ty<1||ty>mapW)continue;
			p[++cnt]={i,gameMap[tx][ty].team,gameMap[tx][ty].army,gameMap[tx][ty].army,gameMap[tx][ty].type};
			if(p[cnt].type!=1&&p[cnt].team==id) p[cnt].army=-p[cnt].army,p[cnt].del=-p[cnt].del;
			if(p[cnt].type==4&&p[cnt].team!=id) p[cnt].army=2*p[cnt].army-ll(1e15);
			else if(p[cnt].type==0&&p[cnt].team!=id) p[cnt].army=p[cnt].army-ll(1e15);
			else if(p[cnt].type==1) { p[cnt].del=200; p[cnt].army=-1e16; }
			else if(p[cnt].type==3&&p[cnt].team!=id) p[cnt].army=-ll(1e18);
		}
		std::sort(p+1,p+cnt+1);
	//	gotoxy(mapH+2+16+1+id,1); clearline();
	//	fputs(defTeams[id].name.c_str(),stdout);
	//	printf(": ");
	//	for(int i=1; i<=cnt; ++i) printf("{%d %d %lld %lld %d} ",p[i].to,p[i].team,p[i].army,p[i].del,p[i].type);
	//	fflush(stdout); _getch();
		for(int i=1;i<=cnt;i++) {
			if(p[i].del<gameMap[coo.x][coo.y].army) return p[i].to;
		}
		return -1;
	}
}

namespace xiaruizeBot{
    const int dx[5] = {0, -1, 0, 1, 0};
    const int dy[5] = {0, 0, -1, 0, 1};
    int checkOrder[5]={0,1,2,3,4};
    int otherRobotProtection[20];
    std::vector<int> operation[20];
    bool vis[20][505][505];
    static std::mt19937 mtrd(std::chrono::system_clock::now().time_since_epoch().count());
    int sendArmyProcess[20];
    int backCountCnt[20];
    playerCoord previousPos[20];

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
        shuffle(checkOrder + 1, checkOrder + 5,mtrd);
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
                previousPos[id] = coord;
                if(gameMap[ToX][ToY].army<gameMap[coord.x][coord.y].army)
                {
                    if(!operation[gameMap[ToX][ToY].team].empty())
                        operation[id].insert(operation[id].end(), operation[gameMap[ToX][ToY].team].begin(), operation[gameMap[ToX][ToY].team].end());
                }
                return x;
            }
        }
        shuffle(checkOrder + 1, checkOrder + 5,mtrd);
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
                previousPos[id] = coord;
                return x;
            }
        }
        shuffle(checkOrder + 1, checkOrder + 5,mtrd);
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
            previousPos[id] = coord;
            return x;
        }
        shuffle(checkOrder + 1, checkOrder + 5,mtrd);
        for (int i = 1; i <= 4; i++)
        {
            x = checkOrder[i];
            int ToX = coord.x + dx[x];
            int ToY = coord.y + dy[x];
            if (ToX < 1 || ToX > mapH || ToY < 1 || ToY > mapW)
                continue;
            if (gameMap[ToX][ToY].type == 2)
                continue;
            if(previousPos[id].x==ToX&&previousPos[id].y==ToY)
                continue;
            operation[id].push_back(x);
            previousPos[id] = coord;
            return x;
        }
        return -1;
    }

    int xiaruizeBot(int id,playerCoord coord)
    {
        if(gameMap[coord.x][coord.y].army==0||gameMap[coord.x][coord.y].team!=id)
        {
            memset(vis[id], 0, sizeof(vis[id]));
            backCountCnt[id] = 1;
            otherRobotProtection[id] = std::max(0, std::min((int)operation[id].size() - 10, (int)mtrd() % 10));
            sendArmyProcess[id] = 1;
            return 0;
        }
        if(sendArmyProcess[id])
        {
            if (sendArmyProcess[id] > operation[id].size())
            {
                sendArmyProcess[id] = 0;
                return -1;
            }
            sendArmyProcess[id]++;
            if(otherRobotProtection[id])
            {
                otherRobotProtection[id]--;
                return operation[id][sendArmyProcess[id] - 2]+4;
            }
            else
                return operation[id][sendArmyProcess[id] - 2];
        }
        vis[id][coord.x][coord.y] = true;
        int returnValue = dfs(id, coord);
        if(returnValue!=-1)
        {
            backCountCnt[id] = 1;
            sendArmyProcess[id] = 0;
            return returnValue;
        }
        else
        {
            int res;
            changeDirection(operation[id][operation[id].size() - 1], res);
            operation[id].pop_back();
            return res;
        }
    }
}

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
        if (gameMap[player.x][player.y].army == 0 || gameMap[player.x][player.y].team != id)
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
            des.teamOnIt = gameMap[des.x][des.y].team;
            if (gameMap[des.x][des.y].team != id && gameMap[des.x][des.y].type == 3)
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

#endif
