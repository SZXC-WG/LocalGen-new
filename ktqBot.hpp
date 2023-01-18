#include <algorithm>
#include "LGmaps.hpp"
const int dx[5] = {0, -1, 0, 1, 0};
const int dy[5] = {0, 0, -1, 0, 1};
int visitTimeCount[505][505];

int ktqBot(int id, playerCoord coo)
{
	using ll = long long;
	if (gameMap[coo.x][coo.y].team != id || gameMap[coo.x][coo.y].army == 0)
		return 0;
	visitTimeCount[coo.x][coo.y]++;
	struct node
	{
		int to, team;
		ll army, del;
		int type;
		int priority;
		bool operator<(node b)
		{
			if (priority != b.priority)
				if (rand() % 6 != 0)
					return priority < b.priority;
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
		p[++cnt] = {1, gameMap[tx][ty].team, gameMap[tx][ty].army, gameMap[tx][ty].army, gameMap[tx][ty].type, visitTimeCount[tx][ty]};
		if (p[cnt].type == 4 && p[cnt].type == id)
			p[cnt].army *= -2;
		else if (p[cnt].type == 0 && p[cnt].type == id)
			p[cnt].army *= -1;
		else if (p[cnt].type == 1)
			p[cnt].del += 80;
	}
	std::sort(p + 1, p + cnt + 1);
	int go = p[1].to;
	for (int i = 1; i <= cnt; i++)
	{
		if (p[i].del < gameMap[coo.x][coo.y].army)
			return p[i].to;
	}
	return go;
}
