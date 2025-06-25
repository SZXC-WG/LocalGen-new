#ifndef __BOT_GC__
#define __BOT_GC__

#include "../LGdef.hpp"
#include <unordered_map>
#include <array>
#include <queue>
#include <random>

namespace gcBot {

	using ll = long long;
	const ll INF = 10'000'000'000'000'000LL;

	constexpr int dx[] = { -1, 0, 1, 0 };
	constexpr int dy[] = { 0, -1, 0, 1 };
	std::random_device rd;

	class Bot {
	   private:
		int id;                  // Current bot's identifier
		coordS seenGeneral[64];  // Stores last seen positions of other generals
		std::array<ll, 6> blockTypeValue;
		ll eval[256][256];
		coordS par[256][256];
		int dist[256][256];
		// 0->plain, 1->swamp, 2->mountain, 3->general, 4->city, 5->unknown
		int blockType[256][256];
		bool knownBlockType[256][256];
		ll army[256][256];
		coordS prevTarget;
		std::mt19937 rnd;

		static inline bool isValidPosition(int x, int y) {
			return x >= 1 && x <= mapH && y >= 1 && y <= mapW && !unpassable(gameMap[x][y].type);
		}

		static inline int approxDist(coordS st, coordS dest) {
			return std::abs(st.x - dest.x) + std::abs(st.y - dest.y);
		}

		void evaluateRouteCosts(coordS st) {
			static const ll typeValues[] = { -5, -10, -INF, -5, -40, 0, -INF, -INF, -200 };
			auto gv = [&](int x, int y) -> ll {
				if(blockType[x][y] == 1) return -1;
				if(blockType[x][y] == 5) return 0;
				if(gameMap[x][y].player == id) return army[x][y] - 1;
				return -army[x][y];
			};

			memset(dist, 0x3f, sizeof(dist));
			memset(eval, -0x3f, sizeof(eval));

			std::queue<coordS> q;
			q.push(st);
			dist[st.x][st.y] = eval[st.x][st.y] = 0;
			par[st.x][st.y] = coordS(-1, -1);

			while(!q.empty()) {
				coordS cur = q.front();
				q.pop();
				int x = cur.x, y = cur.y, curDist = dist[x][y];
				ll curEval = (eval[x][y] += gv(x, y));

				for(int i = 0; i < 4; ++i) {
					int nx = x + dx[i], ny = y + dy[i];
					if(!isValidPosition(nx, ny)) continue;
					int nd = curDist + 1;
					if(nd < dist[nx][ny]) {
						dist[nx][ny] = nd, eval[nx][ny] = curEval, par[nx][ny] = cur;
						q.emplace(nx, ny);
					} else if(nd == dist[nx][ny] && curEval > eval[nx][ny]) {
						eval[nx][ny] = curEval, par[nx][ny] = cur;
					}
				}
			}
		}

		coordS moveTowards(coordS st, coordS dest) {
			while(par[dest.x][dest.y] != st) dest = par[dest.x][dest.y];
			return dest;
		}

		coordS maxArmyPos() {
			ll maxArmy = 0;
			coordS maxCoo = LGgame::genCoo[id];
			for(int i = 1; i <= mapH; ++i)
				for(int j = 1; j <= mapW; ++j) {
					if(gameMap[i][j].player == id) {
						ll weightedArmy = army[i][j];
						if(blockType[i][j] == 0) weightedArmy -= weightedArmy / 4;
						else if(blockType[i][j] == 4) weightedArmy -= weightedArmy / 6;
						if(weightedArmy > maxArmy)
							maxArmy = weightedArmy, maxCoo = { i, j };
					}
				}
			return maxCoo;
		}

	   public:
		Bot() :
		    rnd(rd()) {}

		void init(int botId) {
			id = botId;
			blockTypeValue = { 60, -500, -INF, 0, 50, 25 };
			prevTarget = coordS(-1, -1);
			memset(knownBlockType, 0, sizeof(knownBlockType));
			memset(army, -1, sizeof(army));
			for(auto& gen: seenGeneral) gen = coordS(-1, -1);
		}

		moveS calcNextMove(coordS coo) {
			// Update block types, seen generals & army
			for(int i = 1; i <= mapH; ++i)
				for(int j = 1; j <= mapW; ++j) {
					if(isVisible(i, j, 1 << id)) {
						knownBlockType[i][j] = true, blockType[i][j] = gameMap[i][j].type;
						if(blockType[i][j] == 3)
							seenGeneral[gameMap[i][j].player] = coordS(i, j);
						ll seenArmy = gameMap[i][j].army;
						// Apply exponential smoothing after first seen
						army[i][j] = (army[i][j] < 0 || gameMap[i][j].player == id || gameMap[i][j].player == 0) ? seenArmy : (seenArmy + army[i][j]) / 2;
					} else if(!knownBlockType[i][j]) {
						int type = gameMap[i][j].type;
						if(unpassable(type) || type == 4) blockType[i][j] = 5;
						else if(type == 3) blockType[i][j] = 0;
						else blockType[i][j] = type;
					}
				}

			// Dynamic block values
			if(LGgame::curTurn < 13) return { id, false, coo, coo };
			blockTypeValue[0] = 55 + pow(LGgame::curTurn, 0.2);        // plain
			blockTypeValue[1] = -500 * pow(LGgame::curTurn, -0.1);     // swamp
			blockTypeValue[4] = 28 * pow(LGgame::curTurn - 12, 0.15);  // city
			blockTypeValue[5] = 35 + 15 * pow(LGgame::curTurn, 0.15);  // unknown

			// Focus disabled?
			if(!isValidPosition(coo.x, coo.y) || gameMap[coo.x][coo.y].player != id || gameMap[coo.x][coo.y].army < 2)
				coo = maxArmyPos(), prevTarget = coordS(-1, -1);
			else if(coo == prevTarget || (prevTarget.x != -1 && gameMap[prevTarget.x][prevTarget.y].player == id))
				prevTarget = coordS(-1, -1);

			// Attack if possible
			ll minArmy = INF;
			int targetId = -1;
			for(int i = 1; i <= LGgame::playerCnt; ++i) {
				if(i != id && seenGeneral[i] != coordS(-1, -1) && LGgame::isAlive[i]) {
					ll thatArmy = LGgame::gameStats[i].back().army;
					if(thatArmy < minArmy)
						minArmy = thatArmy, targetId = i;
				}
			}

			// Determine target
			evaluateRouteCosts(coo);
			coordS targetPos(-1, -1);
			static std::uniform_real_distribution<double> dis(0, 1);
			if(targetId != -1) targetPos = seenGeneral[targetId];
			else if(prevTarget.x == -1 || blockType[prevTarget.x][prevTarget.y] != 0 || knownBlockType[prevTarget.x][prevTarget.y]) {
				ll maxBlockValue = -INF;
				if(dis(rnd) < 0.02) {
					// Exploration: find random plain with unknown type (might be a general)
					std::vector<coordS> unknownPlains;
					for(int i = 1; i <= mapH; ++i)
						for(int j = 1; j <= mapW; ++j)
							if(blockType[i][j] == 0 && !knownBlockType[i][j] && dist[i][j] < 500)
								unknownPlains.emplace_back(i, j);
					if(!unknownPlains.empty()) {
						std::uniform_int_distribution<int> randIndex(0, unknownPlains.size() - 1);
						targetPos = unknownPlains[randIndex(rnd)];
						maxBlockValue = 1e9;
					}
				}
				for(int i = 1; i <= mapH; ++i)
					for(int j = 1; j <= mapW; ++j)
						if(gameMap[i][j].player != id && dist[i][j] < 500 && !unpassable(blockType[i][j])) {
							ll blockValue = blockTypeValue[blockType[i][j]] + eval[i][j] / 5 - (dist[i][j] + army[i][j] / 2);
							if(LGset::gameMode == 0)
								blockValue -= approxDist({ i, j }, seenGeneral[id]) * 5ll;
							if(blockValue > maxBlockValue)
								maxBlockValue = blockValue, targetPos = { i, j };
						}
				int x = prevTarget.x, y = prevTarget.y;
				if(x != -1 && gameMap[x][y].player != id && !unpassable(blockType[x][y])) {
					ll prevBlockValue = blockTypeValue[blockType[x][y]] + eval[x][y] / 5 - (dist[x][y] + army[x][y] / 2);
					if(LGset::gameMode == 0)
						prevBlockValue -= approxDist({ x, y }, seenGeneral[id]) * 5ll;
					if(std::abs(prevBlockValue - maxBlockValue) < 25)
						targetPos = prevTarget;
				}
			} else {
				targetPos = prevTarget;
			}
			prevTarget = targetPos;

			if(blockType[coo.x][coo.y] == 1 || dis(rnd) > 0.07 || eval[targetPos.x][targetPos.y] > 150) {
				return { id, true, coo, moveTowards(coo, targetPos) };
			}

			evaluateRouteCosts(targetPos);

			// Find own cell with maximum weight
			ll maxWeight = -INF;
			coordS newFocus(-1, -1);
			for(int i = 1; i <= mapH; ++i)
				for(int j = 1; j <= mapW; ++j)
					if(gameMap[i][j].player == id && army[i][j] > 1) {
						ll weight = eval[i][j] - 30 * dist[i][j];
						if(blockType[i][j] == 3) weight -= weight / 4;
						if(weight > maxWeight)
							maxWeight = weight, newFocus = { i, j };
					}

			if(newFocus == coordS(-1, -1)) {
				return { id, true, coo, coo };
			}

			evaluateRouteCosts(newFocus);
			return { id, true, newFocus, moveTowards(newFocus, targetPos) };
		}
	};

	std::unordered_map<int, Bot> bots;

	inline void initBot(int id) {
		bots[id].init(id);
	}

	inline moveS calcNextMove(int id, coordS coo) {
		return bots[id].calcNextMove(coo);
	}

}  // namespace gcBot

#endif  // __BOT_GC__
