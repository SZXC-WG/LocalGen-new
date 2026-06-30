
#include "LGdef.hpp"

/**
 * OOP draft for games.
 * 
 * Every game is a class that inherits from gameBaseS.
 * Every player is either a human or a robot.
 * All robots should inherit from botBaseS.
 */

class gameBaseS {
   protected:
	int mapH, mapW;               // map height and width
	Block inGameMap[105][105];    // game map in game
	std::deque<moveS> moveQueue;  // moves in queue
	int playerCnt;
	coordS generalCoord[64];  // general's coordinate

	LG_DEPRECATED_S("Only for compatibility. Do not use.")
	/**
	 * @brief Deprecated function only for compatibility with old (int)LGgame::analyzeMove(int,int,coordS).
	 * 
	 * @param id player id.
	 * @param mv move id.
	 * @param coo focus coordinate.
	 * 
	 * @return
	 * 0/-1. 0 for success, -1 for invalid %mv.
	 */
	virtual inline int analyzeMove(int id, int mv, coordS& coo) {
		switch(mv) {
			case -1:
				break;
			case 0:
				coo = generalCoord[id];
				lastTurn[id] = coo;
				break;
			case 1 ... 4: {
				coordS newCoo{ coo.x + dx[mv], coo.y + dy[mv] };
				if(newCoo.x < 1 || newCoo.x > mapH || newCoo.y < 1 || newCoo.y > mapW || gameMap[newCoo.x][newCoo.y].type == 2)
					return 1;
				moveS insMv{
					id,
					true,
					coo,
					newCoo,
				};
				moveQueue.push_back(insMv);
				coo = newCoo;
				lastTurn[id] = coo;
				break;
			}
			case 5 ... 8: {
				coordS newCoo{ coo.x + dx[mv - 4], coo.y + dy[mv - 4] };
				if(newCoo.x < 1 || newCoo.x > mapH || newCoo.y < 1 || newCoo.y > mapW)
					return 1;
				coo = newCoo;
				lastTurn[id] = coo;
				break;
			}
			default:
				return -1;
		}
		return 0;
	}

	/**
	 * @brief Check whether a move is valid.
	 * 
	 * @param mv waiting-to-be-checked move.
	 * 
	 * @return 
	 * Integer specifying the result of the check.
	 * Here, TA stands for 'taking army'.
	 * 0: passed;
	 * 1: coordinate out of bound;
	 * 2: (TA) moving through mountains (i.e. map[%mv.from].type or map[%mv.to].type equals BLOCK_MOUNTAIN);
	 * 3: (TA) moving too far (i.e. %mv.from & %mv.to is not 4-adjacent);
	 * 4: moving to the same place (i.e. %mv.from == %mv.to);
	 * 5: invalid player;
	 */
	virtual inline int checkMove(moveS mv) {
		if(mv.id < 1 || mv.id > playerCnt) return 5;
		if(mv.from == mv.to) return 4;
		if(mv.from.x < 1 || mv.from.x > mapH || mv.from.y < 1 || mv.from.y > mapW) return 1;
		if(mv.to.x < 1 || mv.to.x > mapH || mv.to.y < 1 || mv.to.y > mapW) return 1;
		if(this->inGameMap[mv.from.x][mv.from.y].type == BLOCK_MOUNTAIN && mv.takeArmy) return 2;
		if(this->inGameMap[mv.to.x][mv.to.y].type == BLOCK_MOUNTAIN && mv.takeArmy) return 2;
		if(abs(mv.to.x - mv.from.x) + abs(mv.to.y - mv.from.y) > 1 && mv.takeArmy) return 3;  // focus change
		return 0;
	}

	virtual inline void flushMove() = 0;

   public:
	virtual inline void start() = 0;

	virtual inline int game() = 0;

	virtual inline void end() = 0;
};
