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

	class Bot {
	   private:
		int id;                  // Current bot's identifier
		coordS seenGeneral[64];  // Stores last seen positions of other generals
		std::array<ll, 6> blockValueWeight;
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

		inline int getType(int x, int y) {
			int type = gameMap[x][y].type;
			if(isVisible(x, y, 1 << id)) return knownBlockType[x][y] = true, type;
			if(unpassable(type) || type == 4) return 5;
			if(type == 3) return 0;
			return type;
		}

		void evaluateRouteCosts(coordS st) {
			static const ll typeValues[] = { -5, -10, -INF, -5, -40, 0, -INF, -INF, -200 };
			auto gv = [&](int x, int y) -> ll {
				if(blockType[x][y] == 1) return -1;
				if(blockType[x][y] == 5) return -20;
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
		    rnd(std::random_device()()) {}

		void init(int botId) {
			id = botId;
			blockValueWeight = { 31 - LGset::plainRate[LGset::gameMode], -2000, -INF, 0, 50, 25 };
			prevTarget = coordS(-1, -1);
			memset(knownBlockType, 0, sizeof(knownBlockType));
			memset(army, -1, sizeof(army));
			for(auto& gen: seenGeneral) gen = coordS(-1, -1);
		}

		moveS calcNextMove(coordS coo) {
			// Update seen generals & army
			for(int i = 1; i <= mapH; ++i)
				for(int j = 1; j <= mapW; ++j)
					if(isVisible(i, j, 1 << id)) {
						if(gameMap[i][j].type == 3)
							seenGeneral[gameMap[i][j].player] = coordS(i, j);
						ll seenArmy = gameMap[i][j].army;
						// Apply exponential smoothing after first seen
						army[i][j] = (army[i][j] == -1 || gameMap[i][j].player == id) ? seenArmy : (seenArmy + army[i][j]) / 2;
					}

			// Update block types
			for(int i = 1; i <= mapH; ++i)
				for(int j = 1; j <= mapW; ++j)
					if(!knownBlockType[i][j])
						blockType[i][j] = getType(i, j);

			// Focus disabled?
			if(gameMap[coo.x][coo.y].player != id || gameMap[coo.x][coo.y].army == 0)
				coo = maxArmyPos();

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
			coordS targetPos = coo;
			if(targetId == -1) {
				ll maxBlockValue = -INF;
				for(int i = 1; i <= mapH; ++i)
					for(int j = 1; j <= mapW; ++j)
						if(gameMap[i][j].player != id && !unpassable(blockType[i][j])) {
							ll blockValue = blockValueWeight[blockType[i][j]] - dist[i][j] - army[i][j] / 11;
							if(blockValue > maxBlockValue)
								maxBlockValue = blockValue, targetPos = { i, j };
						}
				int x = prevTarget.x, y = prevTarget.y;
				if(x != -1 && gameMap[x][y].player != id && !unpassable(blockType[x][y])) {
					ll prevBlockValue = blockValueWeight[blockType[x][y]] - dist[x][y] - army[x][y] / 11;
					if(std::abs(prevBlockValue - maxBlockValue) < 25)
						targetPos = prevTarget;
				}
			} else {
				targetPos = seenGeneral[targetId];
			}
			prevTarget = targetPos;

			static std::uniform_real_distribution<double> dis(0, 1);
			if(blockType[coo.x][coo.y] == 1 || dis(rnd) > 0.07) {
				return { id, true, coo, moveTowards(coo, targetPos) };
			}

			evaluateRouteCosts(targetPos);

			// Find own cell with maximum weight
			ll maxWeight = -INF;
			coordS newFocus(-1, -1);
			for(int i = 1; i <= mapH; ++i)
				for(int j = 1; j <= mapW; ++j)
					if(gameMap[i][j].player == id && army[i][j] > 1) {
						ll weight = eval[i][j] - 50 * dist[i][j];
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
