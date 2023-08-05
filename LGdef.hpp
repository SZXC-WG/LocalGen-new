/* This is LGdef.hpp file of LocalGen.                                   */
/* Copyright (c) 2023 LocalGen-dev; All rights reserved.                 */
/* Developers: http://github.com/LocalGen-dev                            */
/* Project: http://github.com/LocalGen-dev/LocalGen-new                  */
/*                                                                       */
/* This project is licensed under the MIT license. That means you can    */
/* download, use and share a copy of the product of this project. You    */
/* may modify the source code and make contribution to it too. But, you  */
/* must print the copyright information at the front of your product.    */
/*                                                                       */
/* The full MIT license this project uses can be found here:             */
/* http://github.com/LocalGen-dev/LocalGen-new/blob/main/LICENSE.md      */

#ifndef __LGDEF_HPP__
#define __LGDEF_HPP__

//====def====//

#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
#include <random>
#include <chrono>
#include <algorithm>
#include <vector>
#include <deque>
#include <array>
#include <queue>
#include <mutex>
#include <thread>
#include <winsock2.h>
#include <windows.h>
#include <io.h>
#include <graphics.h>
#include <ege/sys_edit.h>
#include "LocalGen_private.h"

using std::vector;
using std::deque;
using std::string;
using std::wstring;
using std::to_string;
using std::to_wstring;
using namespace std::literals;

//====const====//

const int dx[5] = {0, -1, 0, 1, 0};
const int dy[5] = {0, 0, -1, 0, 1};
const char NUM_s[20] = {0, 'H', 'K', 'W', 'L', 'M', 'Q', 'I', 'G', 'B', 'N', 'T'}; // number suffixes; 10^2<->10^12
const int LEN_ZIP = 100005, CHAR_AD = 48, LEN_MOVE = 30005, replaySorter = 2000;
const int SSN=205,SSL=100005;
const int SKPORT=14514;

//====struct====//

/**
 * @brief Struct storing the general information of a map.
 */
struct MapInfoS {
	int id; // map id in storage
	wstring chiname; // chinese(or other languages) name of map
	wstring engname; // english name of map (generic)
	wstring auth; // author of the map
	int hei; // map height
	int wid; // map width
	int generalcnt; // count of generals of map
	int swampcnt; // count of swamps of map
	int citycnt; // count of cities of map
	int mountaincnt; // count of mountains of map
	int plaincnt; // count of plains of map
	string filename; // map file name (stored in 'maps' folder)
	MapInfoS() = default;
	~MapInfoS() = default;
};

/**
 * @brief Struct saving a player move (in replay).
 */
struct movementS {
	int id, op; // id: player ID; op: player move type.
	long long turn; // turn number when the move occurs
	void clear() { // move clearance; construct a void move.
		id = turn = op = 0;
	}
};

/**
 * @brief Struct saving the coordinate of a player's focused block.
 */
struct playerCoord {
	int x, y;
};

/**
 * @brief Struct saving player's passing info.
 */
struct passS {
	int id, turn; // id: player; turn: passing turn.
};

/**
 * @brief Struct saving the basic information of a player.
 */
struct playerS {
	wstring name;		/* player name */
	unsigned int color; /* player color */
};

/**
 * @brief Struct saving the basic information of a block.
 */
struct Block {
	int player; /* the player who holds this block */
	int type; /* the block's type: 0->plain, 1->swamp, 2->mountain, 3->general, 4->city */
	long long army;  /* count of army on this block */
	bool lit; /* whether the block is lighted(lit) */
};

/**
 * @brief Struct saving a game message.
 */
struct gameMessageStore {
	int playerA, playerB;
	int turnNumber;
};

/**
 * @brief Struct saving a player move (in game).
 */
struct moveS {
	int id; // player id of move
	playerCoord from; // coordinate from
	playerCoord to; // coordinate to
};

//====value====//

string username; // game user's name
PIMAGE pimg[55]; // software used images
MapInfoS maps[5005]; // storing all imported maps
Block gameMap[505][505]; /* current game map; maximum 500*500 */
playerS playerInfo[64] = { // player information (default written)
	{L"White", 0xffffffff},
	{L"Red", 0xffff0000},
	{L"Aqua", 0xff4363d8},
	{L"Green", 0xff008000},
	{L"Teal", 0xff008080},
	{L"Orange", 0xfff58231},
	{L"Pink", 0xfff032e6},
	{L"Purple", 0xff800080},
	{L"Maroon", 0xff800000},
	{L"Yellow", 0xffb09f30},
	{L"Brown", 0xff9a6324},
	{L"Blue", 0xff0000ff},
	{L"Indigo", 0xff483d8b},
};

int mapH, mapW; // height and width of map
int widthPerBlock, heightPerBlock; // block height and width when printing
int mapNum; // count of imported maps

char strZipStatus[LEN_ZIP];
char strZip[LEN_ZIP]; // storing the zipped map
char strdeZip[LEN_ZIP]; // storing the to-be-dezipped zipped map
char strGameZip[LEN_ZIP<<2]; // storing the zipped game (deprecated)
char strdeGameZip[LEN_ZIP<<2]; // storing the to-be-dezipped zipped game (deprecated)
char strStatusZip[LEN_ZIP<<2]; // storing the zipped game status
char strdeStatusZip[LEN_ZIP<<2]; // storing the to-be-dezipped game status

std::vector<passS> passId[505][505]; // every block's passing info
playerCoord lastTurn[20]; // every player's last turn coordinate (in game)

int failSock;

//====function====//

/**
 * @brief Function for comparing two coordinates equal.
 *
 * @param a playerCoord
 * @param b playerCoord
 * @return true
 * @return false
 */
bool operator== (playerCoord a,playerCoord b) {
	return a.x==b.x&&a.y==b.y;
}

/**
 * @brief Function for checking whether the username is valid.
 *
 * @param username string
 * @return true
 * @return false
 */
bool checkValidUsername(string username) {
	if(username.size()<3||username.size()>16) return 0;
	for(int i=0; i<username.size(); ++i) {
		int x=username[i];
		if(x<32||x>126) return 0;
	}
	return 1;
}

/**
 * @brief Function for safely exiting software.
 */
inline void exitExe() { WSACleanup(); exit(0); }

/**
 * @brief Check whether the block is visible according to the printCode.
 *
 * @return true
 * @return false
 */
bool isVisible(int x, int y, int printCode);
/**
 * @brief Print a block's army number.
 *
 * @param visible whether the block is visible
 * @param army army number
 * @param player player occupying the block
 * @param curx block coordinate on x-axis
 * @param cury block coordinate on y-axis
 */
void printNum(bool visible, long long army, int player, int curx, int cury);

void createRandomMap(int crtH, int crtW); // generate random map (special) with H and W
void createStandardMap(int crtH, int crtW); // generate standard map (special) with H and W
void createFullCityMap(int crtH, int crtW, long long armyMN, long long armyMX, int plCnt);
void createFullSwampMap(int crtH, int crtW, int plCnt);
void createFullPlainMap(int crtH, int crtW, int plCnt);

void getAllFiles(string path, std::vector<string>& files, string fileType);
void initMaps();
void readMap(int mid); // read map and copy to gameMap
void printMap(int printCode, playerCoord coo); // print the current gameMap
void createOptions(int type,int h);

inline long long PMod(long long& x);
std::pair<long long, long long> bin_search(long long curTurn);
void Zip(); // zip map
void deZip(); // dezip map

bool initSock(); // initialize socket web
void toAvoidCEBugInGraphicsImportMap(string fileName); // ???

/***** graphics *****/

bool FullScreen(HWND hwnd, int fullscreenWidth = GetSystemMetrics(SM_CXSCREEN), int fullscreenHeight = GetSystemMetrics(SM_CYSCREEN), int colourBits = 32, int refreshRate = 60);

/**
 * @brief Namespace for operations on images. (based on EGE)
 */
namespace imageOperation {
	/**
	 * @brief Copy one image to another.
	 *
	 * @param dstimg destination image
	 * @param srcimg source image
	 */
	void copyImage(PIMAGE& dstimg, PIMAGE& srcimg);
	void zoomImage(PIMAGE& pimg, int zoomWidth, int zoomHeight);
	void setWindowTransparent(bool enable, int alpha = 0xFF);
}

/**
 * @brief Namespace for EGE graphics (except image operations)
 */
namespace LGGraphics {
	const color_t bgColor = 0xff222222; // background color
	const color_t mainColor = 0xff008080; // main color
	const color_t errorColor = 0xfffbbbbb; // error color
	PIMAGE favi; // favicon image
	string fileName; // ???
	int stDel = 1; // temporary variable for speed (deprecated)
	int plCnt = 0; // temporary variable for count of players
	int mapSelected = 0; // ID of the map selected
	int cheatCode = 0; // binary code of visibility in game
	/**
	 * @brief Struct for storing printing info of map.
	 */
	struct mapData {
		int heightPerBlock; // block height in printing
		int widthPerBlock; // block width in printing
		int height, width; // ???
		double mapSizeX, mapSizeY; // ???
		int maplocX, maplocY; // location of map in printing
	} mapDataStore; // storing variable
	void WelcomePage();
	void localOptions();
	void webOptions();
	void createMapPage();
	void replayPage();
	[[deprecated]] void doMapImport();
	void doMapSelect();
	// void doRepImport();
	void importGameSettings();
	void inputMapData(int a, int b, int c, int d);
	int select = 0;
	void initWindowSize();
	bool cheatCodeSelected[13];
	void init();
}

inline int getHeightPerBlock(); // get LGGraphics::mapDataStore.heightPerBlock; deprecated
inline int getWidthPerBlock(); // get LGGraphics::mapDataStore.widthPerBlock; deprecated

/***** others *****/

namespace LGgame {
	bool inReplay;
	bool inCreate;
	bool inServer;
	bool inClient;
	int curTurn;
	int cheatCode;
	int playerCnt;
	int isAlive[64];
	int robotId[64];
	int gameSpeed; /* fps */
	int gameMesC;
	string playerNames[64];
	std::vector<int> team[64];
	playerCoord genCoo[64];
	std::deque<moveS> inlineMove;
	playerCoord playerCoo[64];
	std::chrono::nanoseconds beginTime;

	void init(int chtC, int pC, int gS);
	void printGameMessage(gameMessageStore now);
	void kill(int p1, int p2);
	int analyzeMove(int id, int mv, playerCoord& coo);
	void flushMove();
	void initGenerals(playerCoord coos[]);
	void updateMap();
	void ranklist();
}

namespace LGreplay {
	const string defaultReplayFilename="replay.lgr";
	char ntoc(int x);
	int cton(char x);
	string ntos(int x,int len=-1);
	int ston(char* s,int len=-1);
	string zipBlock(Block B);
	struct Movement {
		int player,dir;
		playerCoord coord;
		Movement();
		Movement(int tm,int d,playerCoord c);
		string zip();
	};
	Movement readMove(char* buf);
	struct WReplay {
		string Filename;
		FILE* file;
		WReplay(string Fname=defaultReplayFilename);
		void initReplay(string Fname=defaultReplayFilename);
		void newTurn();
		void newMove(Movement mov);
	} wreplay;
	string ts(int x);
	Block readBlock(FILE* fp);
	int QwQ(Movement mov);
	void updMap(int turn);
	struct replayMap {
		Block rMap[105][105];
		bool alive[21];
		void download();
		void upload();
	};
	struct RReplay {
		string Filename;
		FILE* file;
		int totTurn,turnPos[10005],seekPos,replaySize,curTurn;
		Block startMap[505][505];
		vector<replayMap> midStates;
		char* readBuf=new char[256];
		RReplay(string Fname=defaultReplayFilename);
		void resetGame();
		bool _nextTurn();
		int nextTurn();
		void gotoTurn(int turnid);
		int preTurn();
		void initReplay(string Fname=defaultReplayFilename);
	} rreplay;
}

namespace LGlocal {
	int GAME();
};

namespace LGserver {
	std::mutex mLock;
	int totSock;
	bool sockCon[SSN],lisEnd,lisCon;
	SOCKET serverSocket[SSN];
	char sendBuf[SSL],recvBuf[SSL];

	void zipSendBuf();
	void sockListen();
	void procMessage(int sockID);
	void sockBroadcast();
	void sockCollect();
	int GAME();
};

namespace LGclient {
	std::mutex mLock;
	char sendBuf[SSL],recvBuf[SSL],IP[25];
	int playerNumber;
	SOCKET clientSocket;
	bool lisCon;

	void dezipRecvBuf();
	void sockConnect();
	void procMessage();
	void sockMessage();
	bool sockCollect();
	void quitGame();
	int GAME();
};

/**
 * @brief Main Page Function. This page occurs when entering the software.
 */
void MainPage() {
	std::mt19937 mtrd(std::chrono::system_clock::now().time_since_epoch().count());
	LGGraphics::favi = newimage();
	getimage_pngfile(LGGraphics::favi, "img/favicon.png");
	LGGraphics::WelcomePage();
//	LGGraphics::localOptions();
	return;
}

#include "bin/LGGrectbut.hpp"
#include "bin/LGGcircbut.hpp"

#endif // __LGDEF_HPP__
