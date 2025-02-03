#ifndef __BOT_ZLY_V2__
#define __BOT_ZLY_V2__

#include "../LGdef.hpp"

#define ll long long

namespace zlyBot_v2 {
	const ll INF = 10'000'000'000'000'000LL;
	enum botModeE {
		BOT_MODE_ATTACK,
		BOT_MODE_EXPLORE,
		BOT_MODE_DEFEND,
	};
	botModeE botModes[64];
	coordS seenGenerals[64][64];
	deque<coordS> stackedMoves[64];
	vector<ll> blockValueWeight[64];  // p s m g c (o)
	ll blockValue[64][505][505];
	ll dist[64][505][505];
	int blockTypeRem[64][505][505];
	ll blockArmyRem[64][505][505];

	// tool to make block access legal
	inline int getType(int id, int x, int y) {
		if(blockTypeRem[id][x][y] != -1) return blockTypeRem[id][x][y];
		else if(isVisible(x, y, 1 << id)) return blockTypeRem[id][x][y] = gameMap[x][y].type;
		else if(gameMap[x][y].type == 2 || gameMap[x][y].type == 4) return 5;
		else if(gameMap[x][y].type == 3) return 0;
		else return gameMap[x][y].type;
	}
	inline ll getArmy(int id, int x, int y) {
		if(isVisible(x, y, 1 << id)) return blockArmyRem[id][x][y] = gameMap[x][y].army;
		else return blockArmyRem[id][x][y];
	}

	inline void initBot(int botId) {
		blockValueWeight[botId] = { 30 - LGset::plainRate[LGset::gameMode] + 1, -1500, -INF, 5, 30, 30 };
		for(int playerId = 1; playerId <= LGgame::playerCnt; ++playerId) seenGenerals[botId][playerId] = coordS(-1, -1);
		memset(blockTypeRem[botId], -1, sizeof(blockTypeRem[botId]));
		memset(blockArmyRem[botId], 0, sizeof(blockArmyRem[botId]));
		// std::ofstream db("player_"s+to_string(botId)+"_debug.txt");
	}

	inline void calcData(int playerId, coordS position) {
		constexpr int delta_x[] = { -1, 0, 1, 0 };
		constexpr int delta_y[] = { 0, -1, 0, 1 };
		memset(dist[playerId], 0x3f, sizeof(dist[playerId]));
		dist[playerId][position.x][position.y] = 0;
		std::queue<std::pair<coordS, int>> queue;
		queue.push({ position, 0 });
		while(!queue.empty()) {
			coordS current = queue.front().first;
			int currentDist = queue.front().second;
			queue.pop();
			for(int i = 0; i < 4; ++i) {
				coordS next = current + coordS(delta_x[i], delta_y[i]);
				if(next.x < 1 || next.x > mapH || next.y < 1 || next.y > mapW) continue;
				if(getType(playerId, next.x, next.y) == 2) continue;
				if(dist[playerId][next.x][next.y] != 0x3f3f3f3f3f3f3f3f) continue;
				dist[playerId][next.x][next.y] = currentDist + 1;
				// if(getType(playerId,next.x,next.y)==5) continue;
				queue.push({ next, currentDist + 1 });
			}
		}
		for(int i = 1; i <= mapH; ++i) {
			for(int j = 1; j <= mapW; ++j) {
				if(gameMap[i][j].player == playerId) blockValue[playerId][i][j] = -INF;
				else blockValue[playerId][i][j] = blockValueWeight[playerId][getType(playerId, i, j)] - dist[playerId][i][j] - getArmy(playerId, i, j) / 10;
			}
		}
	}

	inline void findRoute(int playerId, coordS start, coordS destination) {
		int unitCount = 0;
		for(int i = 1; i <= mapH; ++i)
			for(int j = 1; j <= mapW; ++j)
				if(gameMap[i][j].player == playerId) ++unitCount;
		unitCount = sqrt(unitCount) + 1;
		if((dist[playerId][destination.x][destination.y] - unitCount) & 1) --unitCount;
		unitCount = 1;
		vector<vector<vector<ll>>> dp;
		vector<vector<vector<coordS>>> prev;
		auto getVal = [&](int x, int y) -> ll {
			if(x < 1 || x > mapH || y < 1 || y > mapW) return -INF;
			if(getType(playerId, x, y) == 2) return -INF;
			if(gameMap[x][y].player == playerId) return gameMap[x][y].army;
			else {
				return -getArmy(playerId, x, y);
				// if(isVisible(x, y, 1 << playerId)) return -gameMap[x][y].army;
				// else {
				// 	if(getType(playerId, x, y) == 0) return -5;
				// 	if(getType(playerId, x, y) == 1) return -10;
				// 	if(getType(playerId, x, y) == 3) return -5;
				// 	if(getType(playerId, x, y) == 4) return -40;
				// 	if(getType(playerId, x, y) == 5) return -200;
				// }
			}
			return 0;
		};
		dp.push_back(vector<vector<ll>>(mapH + 1, vector<ll>(mapW + 1, -INF)));
		prev.push_back(vector<vector<coordS>>(mapH + 1, vector<coordS>(mapW + 1, coordS(-1, -1))));
		dp[0][start.x][start.y] = gameMap[start.x][start.y].army;
		constexpr int dx[] = { -1, 0, 1, 0 };
		constexpr int dy[] = { 0, -1, 0, 1 };
		for(int i = 1; i <= unitCount; ++i) {
			dp.push_back(vector<vector<ll>>(mapH + 1, vector<ll>(mapW + 1, -INF)));
			prev.push_back(vector<vector<coordS>>(mapH + 1, vector<coordS>(mapW + 1, coordS(-1, -1))));
			for(int x = 1; x <= mapH; ++x) {
				for(int y = 1; y <= mapW; ++y) {
					if(dp[i - 1][x][y] == -INF) continue;
					for(int j = 0; j < 4; ++j) {
						int nx = x + dx[j], ny = y + dy[j];
						if(nx < 1 || nx > mapH || ny < 1 || ny > mapW) continue;
						if(getType(playerId, nx, ny) == 2) continue;
						ll newValue = dp[i - 1][x][y] + getVal(nx, ny);
						if(newValue > dp[i][nx][ny]) {
							prev[i][nx][ny] = coordS(x, y);
							dp[i][nx][ny] = newValue;
						}
					}
				}
			}
			if(i == unitCount && prev[i][destination.x][destination.y] == coordS(-1, -1)) ++unitCount;
		}
		// int additionalCount = log(unitCount);
		// for(int i = unitCount + 1; i <= unitCount + additionalCount; ++i) {
		// 	dp.push_back(vector<vector<ll>>(mapH + 1, vector<ll>(mapW + 1, -INF)));
		// 	prev.push_back(vector<vector<coordS>>(mapH + 1, vector<coordS>(mapW + 1, coordS(-1, -1))));
		// 	for(int x = 1; x <= mapH; ++x) {
		// 		for(int y = 1; y <= mapW; ++y) {
		// 			if(dp[i - 1][x][y] == -INF) continue;
		// 			for(int j = 0; j < 4; ++j) {
		// 				int nx = x + dx[j], ny = y + dy[j];
		// 				if(nx < 1 || nx > mapH || ny < 1 || ny > mapW) continue;
		// 				if(getType(playerId, nx, ny) == 2) continue;
		// 				ll newValue = dp[i - 1][x][y] + getVal(nx, ny);
		// 				if(newValue > dp[i][nx][ny]) {
		// 					prev[i][nx][ny] = coordS(x, y);
		// 					dp[i][nx][ny] = newValue;
		// 				}
		// 			}
		// 		}
		// 	}
		// }
		int finalChose = unitCount;
		ll finalChoseValue = dp[unitCount][destination.x][destination.y];
		// for(int i = unitCount + 1; i <= unitCount + additionalCount; ++i) {
		// 	if(dp[i][destination.x][destination.y] > finalChoseValue) {
		// 		finalChose = i, finalChoseValue = dp[i][destination.x][destination.y];
		// 	}
		// }
		stackedMoves[playerId].clear();
		coordS pos = destination;
		while(pos != coordS(-1, -1)) {
			stackedMoves[playerId].push_front(pos);
			pos = prev[finalChose--][pos.x][pos.y];
		}
		stackedMoves[playerId].pop_front();
	}

	moveS calcNextMove(int playerId, coordS currentPos) {
		for(int row = 1; row <= mapH; ++row)
			for(int col = 1; col <= mapW; ++col)
				if(isVisible(row, col, 1 << playerId) && gameMap[row][col].player != playerId && gameMap[row][col].type == 3)
					seenGenerals[playerId][gameMap[row][col].player] = coordS(row, col);
		// std::ofstream db("player_"s+to_string(playerId)+"_debug.txt"s,std::ios::app);
		if(gameMap[currentPos.x][currentPos.y].player != playerId || gameMap[currentPos.x][currentPos.y].army == 0) {
			long long maxArmy = 0;
			coordS maxCoo = LGgame::genCoo[playerId];
			for(int row = 1; row <= mapH; ++row) {
				for(int col = 1; col <= mapW; ++col) {
					if(gameMap[row][col].player == playerId) {
						if(gameMap[row][col].army > maxArmy) {
							maxArmy = gameMap[row][col].army;
							maxCoo = coordS{ row, col };
						}
					}
				}
			}
			// db << "Turn " << LGgame::curTurn << " from (" << currentPos.x << "," << currentPos.y << ") to (" << maxCoo.x << "," << maxCoo.y << ")" << std::endl;
			// db.close();
			// return moveS{ playerId, false, currentPos, maxCoo };
			currentPos = maxCoo;
		}
		botModes[playerId] = BOT_MODE_EXPLORE;
		int targetGeneralId = -1;
		ll targetGeneralArmy = INF;
		for(int i = 1; i <= LGgame::playerCnt; ++i) {
			if(seenGenerals[playerId][i] != coordS(-1, -1) && LGgame::isAlive[i]) {
				botModes[playerId] = BOT_MODE_ATTACK;
				if(LGgame::gameStats[i].back().army < targetGeneralArmy) {
					targetGeneralArmy = LGgame::gameStats[i].back().army;
					targetGeneralId = i;
				}
			}
		}
		calcData(playerId, currentPos);
		if(botModes[playerId] == BOT_MODE_ATTACK) {
			// db << "Desti: " << seenGenerals[playerId][targetGeneralId].x << "," << seenGenerals[playerId][targetGeneralId].y << std::endl;
			findRoute(playerId, currentPos, seenGenerals[playerId][targetGeneralId]);
			moveS ret = moveS{ playerId, true, currentPos, stackedMoves[playerId].front() };
			// db << "stackedMoves size: " << stackedMoves[playerId].size() << std::endl;
			stackedMoves[playerId].pop_front();
			// db << "Turn " << LGgame::curTurn << " from (" << ret.from.x << "," << ret.from.y << ") to (" << ret.to.x << "," << ret.to.y << ")" << std::endl;
			// db.close();
			return ret;
		} else if(botModes[playerId] == BOT_MODE_EXPLORE) {
			struct node {
				coordS pos;
				ll value;
			};
			vector<node> nodes;
			for(int row = 1; row <= mapH; ++row)
				for(int col = 1; col <= mapW; ++col) nodes.push_back(node{ coordS(row, col), blockValue[playerId][row][col] });
			sort(nodes.begin(), nodes.end(), [](const node& a, const node& b) { return a.value > b.value; });
			// db << "Desti: " << nodes[0].pos.x << "," << nodes[0].pos.y << std::endl;
			findRoute(playerId, currentPos, nodes[0].pos);
			// db << "stackedMoves size: " << stackedMoves[playerId].size() << std::endl;
			moveS ret = moveS{ playerId, true, currentPos, stackedMoves[playerId].front() };
			stackedMoves[playerId].pop_front();
			// db << "Turn " << LGgame::curTurn << " from (" << ret.from.x << "," << ret.from.y << ") to (" << ret.to.x << "," << ret.to.y << ")" << std::endl;
			// db.close();
			return ret;
		}
		return moveS{ playerId, false, currentPos, currentPos };
	}
}  // namespace zlyBot_v2

#undef ll

#endif  // __BOT_ZLY_V2__
