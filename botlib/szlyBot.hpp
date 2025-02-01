#ifndef __BOT_SZLY__
#define __BOT_SZLY__

#include "../LGdef.hpp"
#include <unordered_map>
#include <array>
#include <queue>
#include <random>

namespace szlyBot {

	using ll = long long;
	const ll INF = 10'000'000'000'000'000LL;

	enum botModeE {
		BOT_MODE_ATTACK,
		BOT_MODE_EXPLORE,
		BOT_MODE_DEFEND,
	};

	constexpr int dx[] = { -1, 0, 1, 0 };
	constexpr int dy[] = { 0, -1, 0, 1 };

	class Bot {
	   private:
		int id;  // Current bot's identifier
		botModeE mode;
		coordS seenGeneral[64];  // Stores last seen positions of other generals
		std::array<ll, 6> blockValueWeight;
		ll blockValue[256][256];
		ll eval[256][256];
		coordS par[256][256];
		int dist[256][256];
		// 0->plain, 1->swamp, 2->mountain, 3->general, 4->city, 5->unknown
		int blockType[256][256];
		bool knownBlockType[256][256];
		std::mt19937 rnd;

		static inline bool isValidPosition(int x, int y) {
			return x >= 1 && x <= mapH && y >= 1 && y <= mapW && gameMap[x][y].type != 2;
		}

		static inline int approxDist(coordS st, coordS dest) {
			return std::abs(st.x - dest.x) + std::abs(st.y - dest.y);
		}

		inline int getType(int x, int y) {
			int type = gameMap[x][y].type;
			if(isVisible(x, y, 1 << id)) return knownBlockType[x][y] = true, type;
			if(type == 2 || type == 4) return 5;
			if(type == 3) return 0;
			return type;
		}

		void computeRoutes(coordS st) {
			static const ll typeValues[] = { -5, -10, -INF, -5, -40, -200 };
			auto gv = [&](int x, int y) -> ll {
				if(!isValidPosition(x, y)) return -INF;
				if(gameMap[x][y].player == id) return gameMap[x][y].army;
				if(isVisible(x, y, 1 << id)) return -gameMap[x][y].army;
				return typeValues[blockType[x][y]];
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

			for(int i = 1; i <= mapH; ++i) {
				for(int j = 1; j <= mapW; ++j) {
					blockValue[i][j] = (gameMap[i][j].player == id)
					                       ? -INF
					                       : blockValueWeight[gameMap[i][j].type] - dist[i][j];
				}
			}
		}

		coordS moveTowards(coordS st, coordS dest) {
			while(par[dest.x][dest.y] != st) dest = par[dest.x][dest.y];
			return dest;
		}

		moveS smartMove(coordS st, coordS dest, float P = 0.7) {
			coordS fastestMove = moveTowards(st, dest);
			coordS gatherPos = coordS(-1, -1);
			double maxWeight = -1e18;
			for(int i = 0; i < 4; ++i) {
				int nx = st.x + dx[i], ny = st.y + dy[i];
				if(!isValidPosition(nx, ny)) continue;
				coordS nc(nx, ny);
				if(nc != fastestMove && gameMap[nx][ny].player == id && gameMap[nx][ny].army > 2) {
					double weight = gameMap[nx][ny].army - 250. / approxDist(nc, seenGeneral[id]);
					if(blockType[nx][ny] == 1) weight++;
					else if(blockType[nx][ny] == 3) weight *= 0.5;
					else if(blockType[nx][ny] == 4) weight *= 0.75;
					if(weight > maxWeight) maxWeight = weight, gatherPos = nc;
				}
			}
			static std::uniform_real_distribution<float> dis(-1, 1);
			if(gatherPos == coordS(-1, -1) || dis(rnd) > P * std::tanh(maxWeight / 75.)) {
				return { id, true, st, fastestMove };
			}
			return { id, true, gatherPos, st };
		}

	   public:
		Bot() :
		    rnd(std::random_device()()) {}

		void init(int botId) {
			id = botId;
			blockValueWeight = { 30 - LGset::plainRate[LGset::gameMode] + 1, -1500, -INF, 5, 30, 30 };
			memset(knownBlockType, 0, sizeof(knownBlockType));
			for(auto& gen: seenGeneral) gen = coordS(-1, -1);
		}

		moveS calcNextMove(coordS coo) {
			// Update seen generals
			for(int i = 1; i <= mapH; ++i)
				for(int j = 1; j <= mapW; ++j)
					if(isVisible(i, j, 1 << id) && gameMap[i][j].type == 3)
						seenGeneral[gameMap[i][j].player] = coordS(i, j);

			// Update block types
			for(int i = 1; i <= mapH; ++i)
				for(int j = 1; j <= mapW; ++j)
					if(!knownBlockType[i][j])
						blockType[i][j] = getType(i, j);

			// Find strongest own cell
			if(gameMap[coo.x][coo.y].player != id || gameMap[coo.x][coo.y].army == 0) {
				ll maxArmy = 0;
				coordS maxCoo = LGgame::genCoo[id];
				for(int i = 1; i <= mapH; ++i)
					for(int j = 1; j <= mapW; ++j) {
						if(gameMap[i][j].player == id) {
							ll weightedArmy = gameMap[i][j].army;
							if(blockType[i][j] == 0) weightedArmy -= weightedArmy / 4;
							else if(blockType[i][j] == 4) weightedArmy -= weightedArmy / 5;
							if(weightedArmy > maxArmy)
								maxArmy = weightedArmy, maxCoo = { i, j };
						}
					}

				coo = maxCoo;
			}

			// Determine bot mode
			mode = BOT_MODE_EXPLORE;
			ll minArmy = INF;
			int targetId = -1;
			for(int i = 1; i <= LGgame::playerCnt; ++i) {
				if(i != id && seenGeneral[i] != coordS(-1, -1) && LGgame::isAlive[i]) {
					mode = BOT_MODE_ATTACK;
					if(LGgame::gameStats[i].back().army < minArmy) {
						minArmy = LGgame::gameStats[i].back().army;
						targetId = i;
					}
				}
			}

			computeRoutes(coo);
			if(mode == BOT_MODE_ATTACK) {
				return smartMove(coo, seenGeneral[targetId], 0.85);
			}

			coordS targetPos;
			ll maxBlockValue = -INF;
			for(int i = 1; i <= mapH; ++i)
				for(int j = 1; j <= mapW; ++j)
					if(blockValue[i][j] > maxBlockValue)
						maxBlockValue = blockValue[i][j], targetPos = { i, j };

			return smartMove(coo, targetPos, 0.7);
		}
	};

	std::unordered_map<int, Bot> bots;

	inline void initBot(int id) {
		bots[id].init(id);
	}

	inline moveS calcNextMove(int id, coordS coo) {
		return bots[id].calcNextMove(coo);
	}

}  // namespace szlyBot

#endif  // __BOT_SZLY__
