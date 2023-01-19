#include "LGmaps.hpp"
#include <algorithm>
using namespace std;

namespace xrzBotNamespace { 
	
	const int attackModeNum = 1;
	const int defenceModeNum = 2;
	static int turnNumber = 0;
	static int defenceModeStart;
	static int operationMode = 114514;
	
	const int changeOnX[4] = {-1, 0, 1, 0};
	const int changeOnY[4] = {0, -1, 0, 1};
	
	int defenceMode(int id, playerCoord coord)
	{
	    struct node
	    {
	        int x, y;
	        int visited;
	        int direction;
	        bool operator<(node b)
	        {
	            return visited > b.visited;
	        }
	    };
	    vector<node> operat;
	    for (int i = 0; i < 4; i++)
	    {
	        int destinationX = coord.x + changeOnX[i];
	        int destinationY = coord.y + changeOnY[i];
	        node tmp;
	        tmp.direction = i + 1;
	        tmp.x = destinationX;
	        tmp.y = destinationY;
	        if (gameMap[destinationX][destinationY].type == 2)
	            continue;
	        if (destinationX < 1 || destinationX > mapH || destinationY < 1 || destinationY > mapW)
	            continue;
	        if (gameMap[destinationX][destinationY].team == id)
	            tmp.visited = gameMap[destinationX][destinationY].army;
	        else
	            continue;
	        operat.push_back(tmp);
	    }
	    node tmp;
	    tmp.direction = rand() % 5;
	    operat.push_back(tmp);
	    sort(operat.begin(), operat.end());
	    return operat[0].direction;
	}
	
	int attackMode(int id, playerCoord coord)
	{
	    struct node
	    {
	        int x, y;
	        int enermyCount;
	        bool isMountain, enermyAround;
	        int MountainNum;
	        bool isSwamp;
	        int visited;
	        int direction;
	        bool operator<(node b)
	        {
	            if (enermyAround != b.enermyAround)
	                return enermyAround < b.enermyAround;
	            if (isMountain && b.isMountain)
	                return MountainNum < b.MountainNum;
	            if (isSwamp != b.isSwamp)
	                return isSwamp < b.isSwamp;
	            if (visited > rand() % 1000 + 10)
	                return true;
	            return rand() & 1;
	        }
	    };
	    vector<node> operat;
	    for (int i = 0; i < 4; i++)
	    {
	        int destinationX = coord.x + changeOnX[i];
	        int destinationY = coord.y + changeOnY[i];
	        node tmp;
	        tmp.direction = i + 1;
	        tmp.x = destinationX;
	        tmp.y = destinationY;
	        if (gameMap[destinationX][destinationY].type == 2)
	            continue;
	        if (destinationX < 1 || destinationX > mapH || destinationY < 1 || destinationY > mapW)
	            continue;
	        if (gameMap[destinationX][destinationY].type == 3 && gameMap[destinationX][destinationY].army * 2 <= gameMap[coord.x][coord.y].army)
	            return i + 1;
	        if (gameMap[destinationX][destinationY].team == id)
	            tmp.visited = gameMap[destinationX][destinationY].army;
	        if (gameMap[destinationX][destinationY].type == 1)
	            tmp.isSwamp = true;
	        if (gameMap[destinationX][destinationY].type == 3)
	        {
	            tmp.isMountain = true;
	            tmp.MountainNum = gameMap[destinationX][destinationY].army;
	        }
	        else
	            tmp.enermyCount = gameMap[destinationX][destinationY].army;
	        for (int j = 0; j < 4; j++)
	        {
	            int secdesX = destinationX + changeOnX[j], secdesY = destinationY + changeOnY[j];
	            if (gameMap[secdesX][secdesY].team != id)
	            {
	                tmp.enermyAround = true;
	                break;
	            }
	        }
	        operat.push_back(tmp);
	    }
	    node tmp;
	    tmp.direction = rand() % 5;
	    operat.push_back(tmp);
	    sort(operat.begin(), operat.end());
	    return operat[0].direction;
	}
	
	int xrzBot(int id, playerCoord coord)
	{
	    turnNumber++;
	    if (operationMode == 114514)
	        operationMode = attackModeNum;
	    if (gameMap[coord.x][coord.y].team != id || gameMap[coord.x][coord.y].army == 0)
	        return 0;
	    if (gameMap[coord.x][coord.y].army < rand() % min(max(1, turnNumber - 100), 10000) && turnNumber >= 100)
	    {
	        operationMode = defenceModeNum;
	        if (defenceModeStart == 0)
	            defenceModeStart = turnNumber;
	    }
	    if (gameMap[coord.x][coord.y].army >= 20 && turnNumber - defenceModeStart > rand() % min(max(20, turnNumber - 100), 10000))
	        operationMode = attackModeNum;
	    if (operationMode == 1)
	        return attackMode(id, coord);
	    else
	        return defenceMode(id, coord);
	}

}

