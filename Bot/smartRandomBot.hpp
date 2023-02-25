#include <algorithm>
#include <vector>
#include "../LGmaps.hpp"

#ifndef __BOT_SMARTRANDOM__
#define __BOT_SMARTRANDOM__

namespace smartRandomBot {
	const int dx[5] = {0, -1, 0, 1, 0};
	const int dy[5] = {0, 0, -1, 0, 1};

	int smartRandomBot(int id, playerCoord coo) {
		static std::mt19937 mtrd(std::chrono::system_clock::now().time_since_epoch().count());
		static deque<playerCoord> lastCoord[20];
		if(gameMap[coo.x][coo.y].team != id || gameMap[coo.x][coo.y].army == 0)
			return 0;
		struct node {
			int type, team;
			long long army;
			int dir;
			bool isLast;
		};
		node p[5];
		int pl = 0;
		for(int i = 1; i <= 4; ++i) {
			register int nx = coo.x + dx[i], ny = coo.y + dy[i];
			if(nx < 1 || nx > mapH || ny < 1 || ny > mapW || gameMap[nx][ny].type == 2)
				continue;
			p[++pl] = {
				gameMap[nx][ny].type, gameMap[nx][ny].team, gameMap[nx][ny].army, i,
				std::find(lastCoord[id].begin(), lastCoord[id].end(), playerCoord{nx, ny})!=lastCoord[id].end()
			};
		}
		bool rdret = mtrd() % 2;
		auto cmp = [&](node a, node b) -> bool {
			if(a.isLast) return false;
			if(b.isLast) return true;
			if(a.type == 3 && a.team != id)
				return true;
			if(b.type == 3 && b.team != id)
				return false;
			if(a.team == 0)
				return rdret;
			if(b.team == 0)
				return !rdret;
			if(a.team == id && b.team != id)
				return false;
			if(a.team != id && b.team == id)
				return true;
			if(a.team == id && b.team == id)
				return a.army > b.army;
			return a.army < b.army;
		};
		std::sort(p + 1, p + pl + 1, cmp);
		lastCoord[id].push_back(coo);
		if(lastCoord[id].size() > 5) lastCoord[id].pop_front();
		return p[1].dir;
	}
}

#endif // __BOT_SMARTRANDOM__
