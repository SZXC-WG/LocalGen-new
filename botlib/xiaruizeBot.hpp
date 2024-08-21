#ifndef __BOT_XIARUIZE__
#define __BOT_XIARUIZE__

namespace xiaruizeBot {
	const int dx[5] = { 0, -1, 0, 1, 0 };
	const int dy[5] = { 0, 0, -1, 0, 1 };
	int checkOrder[5] = { 0, 1, 2, 3, 4 };
	int otherRobotProtection[20];
	std::vector<int> operation[20];
	bool vis[20][505][505];
	static std::mt19937 mtrd(std::chrono::system_clock::now().time_since_epoch().count());
	int sendArmyProcess[20];
	int backCountCnt[20];
	coordS previousPos[20];

	void changeDirection(int x, int& res) {
		switch(x) {
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

	int dfs(int id, coordS coord) {
		shuffle(checkOrder + 1, checkOrder + 5, mtrd);
		int x;
		for(int i = 1; i <= 4; i++) {
			x = checkOrder[i];
			int ToX = coord.x + dx[x];
			int ToY = coord.y + dy[x];
			if(ToX < 1 || ToX > mapH || ToY < 1 || ToY > mapW)
				continue;
			if(gameMap[ToX][ToY].type == 2)
				continue;
			if(gameMap[ToX][ToY].type == 3 && gameMap[ToX][ToY].player != id) {
				operation[id].push_back(x);
				previousPos[id] = coord;
				if(gameMap[ToX][ToY].army < gameMap[coord.x][coord.y].army) {
					if(!operation[gameMap[ToX][ToY].player].empty())
						operation[id].insert(operation[id].end(), operation[gameMap[ToX][ToY].player].begin(), operation[gameMap[ToX][ToY].player].end());
				}
				return x;
			}
		}
		shuffle(checkOrder + 1, checkOrder + 5, mtrd);
		for(int i = 1; i <= 4; i++) {
			x = checkOrder[i];
			int ToX = coord.x + dx[x];
			int ToY = coord.y + dy[x];
			if(ToX < 1 || ToX > mapH || ToY < 1 || ToY > mapW)
				continue;
			if(gameMap[ToX][ToY].type == 2)
				continue;
			if(gameMap[ToX][ToY].type == 4 && gameMap[ToX][ToY].army <= gameMap[coord.x][coord.y].army && gameMap[ToX][ToY].player != id) {
				operation[id].push_back(x);
				previousPos[id] = coord;
				return x;
			}
		}
		shuffle(checkOrder + 1, checkOrder + 5, mtrd);
		for(int i = 1; i <= 4; i++) {
			x = checkOrder[i];
			int ToX = coord.x + dx[x];
			int ToY = coord.y + dy[x];
			if(ToX < 1 || ToX > mapH || ToY < 1 || ToY > mapW)
				continue;
			if(gameMap[ToX][ToY].type == 2)
				continue;
			if(vis[id][ToX][ToY])
				continue;
			operation[id].push_back(x);
			previousPos[id] = coord;
			return x;
		}
		shuffle(checkOrder + 1, checkOrder + 5, mtrd);
		for(int i = 1; i <= 4; i++) {
			x = checkOrder[i];
			int ToX = coord.x + dx[x];
			int ToY = coord.y + dy[x];
			if(ToX < 1 || ToX > mapH || ToY < 1 || ToY > mapW)
				continue;
			if(gameMap[ToX][ToY].type == 2)
				continue;
			if(previousPos[id].x == ToX && previousPos[id].y == ToY)
				continue;
			operation[id].push_back(x);
			previousPos[id] = coord;
			return x;
		}
		return -1;
	}

	moveS calcNextMove(int id, coordS coord) {
		if(gameMap[coord.x][coord.y].army == 0 || gameMap[coord.x][coord.y].player != id) {
			memset(vis[id], 0, sizeof(vis[id]));
			backCountCnt[id] = 1;
			otherRobotProtection[id] = std::max(0, std::min((int)operation[id].size() - 10, (int)mtrd() % 10));
			sendArmyProcess[id] = 1;
			return moveS{ id, false, coord, LGgame::genCoo[id] };
		}
		if(sendArmyProcess[id]) {
			if(sendArmyProcess[id] > operation[id].size()) {
				sendArmyProcess[id] = 0;
				return moveS{
					id, false, coordS{ -1, -1 },
                      coordS{ -1, -1 }
				};
			}
			sendArmyProcess[id]++;
			if(otherRobotProtection[id]) {
				otherRobotProtection[id]--;
				return moveS{
					id, false, coord,
					coordS{ coord.x + dx[operation[id][sendArmyProcess[id] - 2]],
                           coord.y + dy[operation[id][sendArmyProcess[id] - 2]] }
				};
			} else
				return moveS{
					id, true, coord,
					coordS{ coord.x + dx[operation[id][sendArmyProcess[id] - 2]],
                           coord.y + dy[operation[id][sendArmyProcess[id] - 2]] }
				};
		}
		vis[id][coord.x][coord.y] = true;
		int returnValue = dfs(id, coord);
		if(returnValue != -1) {
			backCountCnt[id] = 1;
			sendArmyProcess[id] = 0;
			return moveS{
				id, true, coord,
				coordS{ coord.x + dx[returnValue],
                       coord.y + dy[returnValue] }
			};
		} else {
			int res;
			changeDirection(operation[id][operation[id].size() - 1], res);
			operation[id].pop_back();
			return moveS{
				id, true, coord, coordS{ coord.x + dx[res], coord.y + dy[res] }
			};
		}
	}
}  // namespace xiaruizeBot

#endif  // __BOT_XIARUIZE__
