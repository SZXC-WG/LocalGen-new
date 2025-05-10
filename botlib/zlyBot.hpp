#ifndef __BOT_ZLY__
#define __BOT_ZLY__

#include "../LGdef.hpp"

#define ll long long

namespace zlyBot {
	const ll INF = 10'000'000'000'000'000LL;
	enum botModeE {
		BOT_MODE_ATTACK,
		BOT_MODE_EXPLORE,
		BOT_MODE_DEFEND,
	};
	botModeE botMode[64];
	coordS seenGeneral[64][64];
	deque<coordS> stackedMove[64];
	vector<ll> blockValueWeight[64];  // p s m g c (o)
	ll blockValue[64][505][505];
	ll dist[64][505][505];
	int blockTypeRem[64][505][505];

	// tool to make block access legal
	inline int getType(int id, int x, int y) {
		if(blockTypeRem[id][x][y] != -1) return blockTypeRem[id][x][y];
		else if(isVisible(x, y, 1 << id)) return blockTypeRem[id][x][y] = gameMap[x][y].type;
		else if(unpassable(gameMap[x][y].type) || gameMap[x][y].type == 4) return 8;
		else if(gameMap[x][y].type == 3) return 0;
		else return gameMap[x][y].type;
	}

	inline void initBot(int id) {
		blockValueWeight[id] = { 30 - LGset::plainRate[LGset::gameMode] + 1, -1500, -INF, 5, 30, 1, -INF, -INF, 30 };
		for(int i = 1; i <= LGgame::playerCnt; ++i) seenGeneral[id][i] = coordS(-1, -1);
		memset(blockTypeRem[id], -1, sizeof(blockTypeRem[id]));
		// std::ofstream db("player_"s+to_string(id)+"_debug.txt");
	}

	inline void calcData(int id, coordS coo) {
		constexpr int dx[] = { -1, 0, 1, 0 };
		constexpr int dy[] = { 0, -1, 0, 1 };
		memset(dist[id], 0x3f, sizeof(dist[id]));
		dist[id][coo.x][coo.y] = 0;
		std::queue<std::pair<coordS, int>> q;
		q.push({ coo, 0 });
		while(!q.empty()) {
			coordS cur = q.front().first;
			int curDist = q.front().second;
			q.pop();
			for(int i = 0; i < 4; ++i) {
				coordS next = cur + coordS(dx[i], dy[i]);
				if(next.x < 1 || next.x > mapH || next.y < 1 || next.y > mapW) continue;
				if(unpassable(getType(id, next.x, next.y))) continue;
				if(dist[id][next.x][next.y] != 0x3f3f3f3f3f3f3f3f) continue;
				dist[id][next.x][next.y] = curDist + 1;
				// if(getType(id,next.x,next.y)==5) continue;
				q.push({ next, curDist + 1 });
			}
		}
		for(int i = 1; i <= mapH; ++i) {
			for(int j = 1; j <= mapW; ++j) {
				if(gameMap[i][j].player == id) blockValue[id][i][j] = -INF;
				else blockValue[id][i][j] = blockValueWeight[id][getType(id, i, j)] - dist[id][i][j];
			}
		}
	}

	inline void findRoute(int id, coordS coo, coordS desti) {
		int cnt = 0;
		for(int i = 1; i <= mapH; ++i)
			for(int j = 1; j <= mapW; ++j)
				if(gameMap[i][j].player == id) ++cnt;
		cnt = sqrt(cnt) + 1;
		if((dist[id][desti.x][desti.y] - cnt) & 1) --cnt;
		cnt = 1;
		vector<vector<vector<ll>>> dp;
		vector<vector<vector<coordS>>> pr;
		auto gv = [&](int x, int y) -> ll {
			if(x < 1 || x > mapH || y < 1 || y > mapW) return -INF;
			if(unpassable(getType(id, x, y))) return -INF;
			if(gameMap[x][y].player == id) return gameMap[x][y].army;
			else {
				if(isVisible(x, y, 1 << id)) return -gameMap[x][y].army;
				else {
					if(getType(id, x, y) == 0) return -5;
					if(getType(id, x, y) == 1) return -10;
					if(getType(id, x, y) == 3) return -5;
					if(getType(id, x, y) == 4) return -40;
					if(getType(id, x, y) == 5) return -200;
				}
			}
			return 0;
		};
		dp.push_back(vector<vector<ll>>(mapH + 1, vector<ll>(mapW + 1, -INF)));
		pr.push_back(vector<vector<coordS>>(mapH + 1, vector<coordS>(mapW + 1, coordS(-1, -1))));
		dp[0][coo.x][coo.y] = gameMap[coo.x][coo.y].army;
		constexpr int dx[] = { -1, 0, 1, 0 };
		constexpr int dy[] = { 0, -1, 0, 1 };
		// printf("player %d (%ls):\n",id,playerInfo[id].name.c_str());
		for(int i = 1; i <= cnt; ++i) {
			dp.push_back(vector<vector<ll>>(mapH + 1, vector<ll>(mapW + 1, -INF)));
			pr.push_back(vector<vector<coordS>>(mapH + 1, vector<coordS>(mapW + 1, coordS(-1, -1))));
			for(int x = 1; x <= mapH; ++x) {
				for(int y = 1; y <= mapW; ++y) {
					if(dp[i - 1][x][y] == -INF) continue;
					for(int j = 0; j < 4; ++j) {
						int nx = x + dx[j], ny = y + dy[j];
						if(nx < 1 || nx > mapH || ny < 1 || ny > mapW) continue;
						if(unpassable(getType(id, nx, ny))) continue;
						ll nv = dp[i - 1][x][y] + gv(nx, ny);
						if(nv > dp[i][nx][ny]) {
							pr[i][nx][ny] = coordS(x, y);
							dp[i][nx][ny] = nv;
						}
						// if(nv>=-INF/10)
						// 	printf("i(%d) cnt(%d) fx(%d) fy(%d) nx(%d) ny(%d) dp(%lld) pr(%d,%d) nv(%lld)\n",
						// 	       i,cnt,x,y,nx,ny,dp[i][nx][ny],pr[i][nx][ny].x,pr[i][nx][ny].y,nv);
					}
				}
			}
			if(i == cnt && pr[i][desti.x][desti.y] == coordS(-1, -1)) ++cnt;
		}
		stackedMove[id].clear();
		coordS p = desti;
		while(p != coordS(-1, -1)) {
			stackedMove[id].push_front(p);
			p = pr[cnt--][p.x][p.y];
		}
		stackedMove[id].pop_front();
		// printf("(%d,%d)",coo.x,coo.y);
		// for(coordS x:stackedMove[id]) printf("->(%d,%d)",x.x,x.y);
		// printf("\n");
	}

	moveS calcNextMove(int id, coordS coo) {
		for(int i = 1; i <= mapH; ++i)
			for(int j = 1; j <= mapW; ++j)
				if(isVisible(i, j, 1 << id) && gameMap[i][j].player != id && gameMap[i][j].type == 3)
					seenGeneral[id][gameMap[i][j].player] = coordS(i, j);
		// std::ofstream db("player_"s+to_string(id)+"_debug.txt"s,std::ios::app);
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
			// db << "Turn " << LGgame::curTurn << " from (" << coo.x << "," << coo.y << ") to (" << mxCoo.x << "," << mxCoo.y << ")" << std::endl;
			// db.close();
			// return moveS{ id, false, coo, mxCoo };
			coo = mxCoo;
		}
		botMode[id] = BOT_MODE_EXPLORE;
		int goalGeneralId = -1;
		ll goalGeneralArmy = INF;
		for(int i = 1; i <= LGgame::playerCnt; ++i) {
			if(seenGeneral[id][i] != coordS(-1, -1) && LGgame::isAlive[i]) {
				botMode[id] = BOT_MODE_ATTACK;
				if(LGgame::gameStats[i].back().army < goalGeneralArmy) {
					goalGeneralArmy = LGgame::gameStats[i].back().army;
					goalGeneralId = i;
				}
			}
		}
		calcData(id, coo);
		if(botMode[id] == BOT_MODE_ATTACK) {
			// db << "Desti: " << seenGeneral[id][goalGeneralId].x << "," << seenGeneral[id][goalGeneralId].y << std::endl;
			findRoute(id, coo, seenGeneral[id][goalGeneralId]);
			moveS ret = moveS{ id, true, coo, stackedMove[id].front() };
			// db << "stackedMove size: " << stackedMove[id].size() << std::endl;
			stackedMove[id].pop_front();
			// db << "Turn " << LGgame::curTurn << " from (" << ret.from.x << "," << ret.from.y << ") to (" << ret.to.x << "," << ret.to.y << ")" << std::endl;
			// db.close();
			return ret;
		} else if(botMode[id] == BOT_MODE_EXPLORE) {
			struct node {
				coordS coo;
				ll value;
			};
			vector<node> v;
			for(int i = 1; i <= mapH; ++i)
				for(int j = 1; j <= mapW; ++j) v.push_back(node{ coordS(i, j), blockValue[id][i][j] });
			sort(v.begin(), v.end(), [](const node& a, const node& b) { return a.value > b.value; });
			// db << "Desti: " << v[0].coo.x << "," << v[0].coo.y << std::endl;
			findRoute(id, coo, v[0].coo);
			// db << "stackedMove size: " << stackedMove[id].size() << std::endl;
			moveS ret = moveS{ id, true, coo, stackedMove[id].front() };
			stackedMove[id].pop_front();
			// db << "Turn " << LGgame::curTurn << " from (" << ret.from.x << "," << ret.from.y << ") to (" << ret.to.x << "," << ret.to.y << ")" << std::endl;
			// db.close();
			return ret;
		}
		return moveS{ id, false, coo, coo };
	}
}  // namespace zlyBot

#undef ll

#endif  // __BOT_ZLY__
