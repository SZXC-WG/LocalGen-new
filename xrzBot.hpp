#include "LGmaps.hpp"
#include <algorithm>
using namespace std;

const int attackModeNum = 1;
const int defenceModeNum = 2;
static int turnNumber = 0;
static int defenceModeStart;
static int operationMode = 1;
const int changeOnX[4] = {-1, 0, 1, 0};
const int changeOnY[4] = {0, -1, 0, 1};
static playerCoord previousPos;

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
    previousPos = coord;
    return (*operat.begin()).direction;
}

int attackMode(int id, playerCoord coord)
{
    struct node
    {
        int x = 0, y = 0;
        int enermyCount = 0;
        bool isMountain = 0, enermyAround = 0;
        int MountainNum = 0;
        bool isSwamp = 0;
        int visited = 1e9;
        int direction = 0;
        bool operator<(node b)
        {
            if (x == previousPos.x && y == previousPos.y)
                return false;
            if (b.x == previousPos.x && b.y == previousPos.y)
                return false;
            if (visited != b.visited)
                return visited > b.visited;
            if (enermyAround != b.enermyAround)
                return enermyAround < b.enermyAround;
            if (isMountain && b.isMountain)
                return MountainNum < b.MountainNum;
            if (visited > 1000)
                return true;
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
        if (gameMap[destinationX][destinationY].type == 4 && gameMap[destinationX][destinationY].army <= gameMap[coord.x][coord.y].army && gameMap[coord.x][coord.y].team != id)
            return i + 1;
        if (gameMap[destinationX][destinationY].team == id)
            tmp.visited = gameMap[destinationX][destinationY].army;
        else if (gameMap[destinationX][destinationY].team == 0 && gameMap[destinationX][destinationY].army == 0)
            tmp.visited = 1e9;
        else if (gameMap[destinationX][destinationY].army != 0)
            tmp.visited = -gameMap[destinationX][destinationY].army;
        if (gameMap[destinationX][destinationY].type == 1)
            tmp.isSwamp = true;
        if (gameMap[destinationX][destinationY].type == 4)
        {
            tmp.isMountain = true;
            tmp.MountainNum = gameMap[destinationX][destinationY].army;
        }
        else
            tmp.enermyCount = gameMap[destinationX][destinationY].army;
        for (int j = 0; j < 4; j++)
        {
            int secdesX = destinationX + changeOnX[j], secdesY = destinationY + changeOnY[j];
            if (gameMap[secdesX][secdesY].team != id && gameMap[secdesX][secdesY].team != 0)
            {
                tmp.enermyAround = true;
            }
        }
        operat.push_back(tmp);
    }
    sort(operat.begin(), operat.end());
    previousPos = coord;
    return (*operat.begin()).direction;
}

int xrzBot(int id, playerCoord coord)
{
    srand(time(0));
    turnNumber++;
    //    if (operationMode == 114514)
    //        operationMode = attackModeNum;
    if (gameMap[coord.x][coord.y].team != id || gameMap[coord.x][coord.y].army <= 0)
        return 0;
    if (rand() % 10 == 0)
    {
        operationMode = defenceModeNum;
        if (defenceModeStart == 0)
            defenceModeStart = turnNumber;
    }
    if (turnNumber - defenceModeStart > 4)
    {
        operationMode = attackModeNum;
        defenceModeStart = 0;
    }
    if (operationMode == 1)
        return attackMode(id, coord);
    else
        return defenceMode(id, coord);
}
