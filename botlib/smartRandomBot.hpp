#ifndef __BOT_SMARTRANDOM__
#define __BOT_SMARTRANDOM__

#include "../LGdef.hpp"

namespace smartRandomBot {
	const int dx[5] = { 0, -1, 0, 1, 0 };
	const int dy[5] = { 0, 0, -1, 0, 1 };

	moveS calcNextMove(int id, coordS coo) {
		static std::mt19937 mtrd(std::chrono::system_clock::now().time_since_epoch().count());
		static deque<coordS> lastCoord[20];
		if(gameMap[coo.x][coo.y].player != id || gameMap[coo.x][coo.y].army == 0) {
			long long mxArmy = 0;
			coordS mxCoo = LGgame::genCoo[id];
			for(int i = 1; i <= mapH; ++i) {
				for(int j = 1; j <= mapW; ++j) {
					if(gameMap[i][j].player == id) {
						if(gameMap[i][j].army > mxArmy) {
							mxArmy = gameMap[i][j].army;
							mxCoo = coordS{ i, j };
						}
					}
				}
			}
			// return moveS{ id, false, coo, mxCoo };
			coo = mxCoo;
		}
		struct node {
			int type, team;
			long long army;
			int dir;
			std::ptrdiff_t lastCount;
		};
		node p[5];
		int pl = 0;
		for(int i = 1; i <= 4; ++i) {
			int nx = coo.x + dx[i], ny = coo.y + dy[i];
			if(nx < 1 || nx > mapH || ny < 1 || ny > mapW || unpassable(gameMap[nx][ny].type))
				continue;
			p[++pl] = {
				gameMap[nx][ny].type, gameMap[nx][ny].player, gameMap[nx][ny].army, i,
				std::find(lastCoord[id].rbegin(), lastCoord[id].rend(), coordS{ nx, ny }) - lastCoord[id].rbegin()
			};
		}
		bool rdret = mtrd() % 2;
		auto cmp = [&](node a, node b) -> bool {
			if(a.lastCount != b.lastCount)
				return a.lastCount > b.lastCount;
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
		if(lastCoord[id].size() > 100) lastCoord[id].pop_front();
		return moveS{
			id, true, coo, coordS{ coo.x + dx[p[1].dir], coo.y + dy[p[1].dir] }
		};
	}
}  // namespace smartRandomBot

#endif  // __BOT_SMARTRANDOM__
