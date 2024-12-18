/* This is LGdef.hpp file of LocalGen.                                   */
/* Copyright (c) 2023 SZXC Work Group; All rights reserved.              */
/* Developers: http://github.com/SZXC-WG                                 */
/* Project: http://github.com/SZXC-WG/LocalGen-new                       */
/*                                                                       */
/* This project is licensed under the MIT license. That means you can    */
/* download, use and share a copy of the product of this project. You    */
/* may modify the source code and make contribution to it too. But, you  */
/* must print the copyright information at the front of your product.    */
/*                                                                       */
/* The full MIT license this project uses can be found here:             */
/* http://github.com/SZXC-WG/LocalGen-new/blob/main/LICENSE.md           */

#ifndef __LGDEF_HPP__
#define __LGDEF_HPP__

/**** OVERALL HEADERS ****/

/** C/C++ file operations **/
#include <cstdio>
#include <fstream>
/** C/C++ string operations **/
#include <cstring>
#include <string>
/** C++ random library **/
#include <random>
/** C++ time library **/
#include <chrono>
/** C++ STL **/
#include <algorithm>
#include <functional>
/** used containers (other than string) **/
#include <vector>
#include <deque>
#include <array>
#include <queue>
#include <map>
#include <unordered_map>
/** thread operations **/
#include <mutex>
#include <thread>
/** web operations (of Windows) **/
#include <winsock2.h>
/** Win32 library **/
#include <windows.h>
#include <io.h>
/** EGE library **/
#include <graphics.h> // main file of EGE
#include <ege/sys_edit.h> // text boxes of EGE
/** project header **/
#include "LocalGen_private.h" // project information

/** using **/
using std::vector;
using std::deque;
using std::string;
using std::wstring;
using std::to_string;
using std::to_wstring;
using std::min; using std::max;
using namespace std::literals;

/**** constant variables ****/

constexpr int dx[5] = {0, -1, 0, 1, 0};
constexpr int dy[5] = {0, 0, -1, 0, 1};
constexpr char NUM_s[20] = {0, 'H', 'K', 'W', 'L', 'M', 'Q', 'I', 'G', 'B', 'N', 'T'}; // number suffixes; 10^2<->10^12
constexpr int LEN_ZIP = 100005, CHAR_AD = 48, LEN_MOVE = 30005, replaySorter = 2000;
constexpr int SSN=205,SSL=100005;
constexpr int SKPORT=14514;

const int CTHour = atoi(__TIME__);
const int CTMin = atoi(&__TIME__[3]);
const int CTSec = atoi(&__TIME__[6]);
std::unordered_map<string,int> __MTON({
	{"Jan"s,  1},
	{"Feb"s,  2},
	{"Mar"s,  3},
	{"Apr"s,  4},
	{"May"s,  5},
	{"Jun"s,  6},
	{"Jul"s,  7},
	{"Aug"s,  8},
	{"Sep"s,  9},
	{"Oct"s, 10},
	{"Nov"s, 11},
	{"Dec"s, 12},
});
const int CTMonth = __MTON[string(__DATE__).substr(0,3)];
const int CTDay = atoi(&__DATE__[4]);
const int CTYear = atoi(&__DATE__[7]);

/**** structures ****/

/**
 * @brief Struct storing the general information of a map.
 */
struct MapInfoS {
	int id; // map id in storage
	wstring chiname; // chinese(or other languages) name of map
	wstring engname; // english name of map (generic)
	wstring auth; // author of the map
	int height; // map height
	int width; // map width
	int generalcnt; // count of generals of map
	int swampcnt; // count of swamps of map
	int citycnt; // count of cities of map
	int mountaincnt; // count of mountains of map
	int plaincnt; // count of plains of map
	string mapFile; // map file name (stored in 'maps' folder)
	MapInfoS() = default;
	~MapInfoS() = default;
};

enum special_map_id {
	MAP_RANDOM_ID = 1,
	MAP_STANDARD_ID = 2,
	MAP_SP_LABYRINTH_ID = 3,
	MAP_SR_LABYRINTH_ID = 4,
	MAP_MR_LABYRINTH_ID = 5,
	MAP_CITY_ID = 6,
	MAP_PLAIN_ID = 7,
	MAP_SWAMP_ID = 8,
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
struct coordS {
	int x, y;
	coordS() = default;
	coordS(int x, int y) : x(x), y(y) {};
};
bool operator==(coordS a, coordS b) { return a.x == b.x && a.y == b.y; }
bool operator!=(coordS a, coordS b) { return a.x != b.x || a.y != b.y; }
coordS operator+(coordS a, coordS b) { return coordS(a.x + b.x, a.y + b.y); }
coordS operator-(coordS a, coordS b) { return coordS(a.x - b.x, a.y - b.y); }

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
	color_t color; /* player color */
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
 * @brief Struct saving a player move (in game).
 */
struct moveS {
	int id; // player id of move
	bool takeArmy; // whether the move takes army
	coordS from; // coordinate from
	coordS to; // coordinate to
};

//====value====//

string username; // game user's name
PIMAGE pimg[55]; // software used images
MapInfoS mapInfo[5005]; // storing all imported maps
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
int blockWidth, blockHeight; // block height and width when printing
int mapNum; // count of imported maps

char strZipStatus[LEN_ZIP];
char strZip[LEN_ZIP]; // storing the zipped map
char strdeZip[LEN_ZIP]; // storing the to-be-dezipped zipped map
char strGameZip[LEN_ZIP<<2]; // storing the zipped game (deprecated)
char strdeGameZip[LEN_ZIP<<2]; // storing the to-be-dezipped zipped game (deprecated)
char strStatusZip[LEN_ZIP<<2]; // storing the zipped game status
char strdeStatusZip[LEN_ZIP<<2]; // storing the to-be-dezipped game status

std::vector<passS> passId[505][505]; // every block's passing info
coordS lastTurn[20]; // every player's last turn coordinate (in game)

int failSock;

/**** ALL functions ****/

// quick game map functions
inline int& gmp(int x,int y) { return gameMap[x][y].player; }
inline int& gmt(int x,int y) { return gameMap[x][y].type; }
inline long long& gma(int x,int y) { return gameMap[x][y].army; }
inline bool& gml(int x,int y) { return gameMap[x][y].lit; }
inline int& gmp(coordS c) { return gameMap[c.x][c.y].player; }
inline int& gmt(coordS c) { return gameMap[c.x][c.y].type; }
inline long long& gma(coordS c) { return gameMap[c.x][c.y].army; }
inline bool& gml(coordS c) { return gameMap[c.x][c.y].lit; }

std::wstring wcharTransfer(const wstring& ws);

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
 * @brief Check whether the block is visible according to the Code.
 *
 * @return true
 * @return false
 */
bool isVisible(int x, int y, int Code);
/**
 * @brief Print a block's army number.
 *
 * @param visible whether the block is visible
 * @param army army number
 * @param player player occupying the block
 * @param curx block coordinate on x-axis
 * @param cury block coordinate on y-axis
 */
void printBlockNum(bool visible, long long army, int player, int curx, int cury);

void createRandomMap(int crtH, int crtW); // generate random map (special) with H and W
void createStandardMap(int crtH, int crtW); // generate standard map (special) with H and W
void createLabyrinthMap(int crtH, int crtW, int TYPE); // generate labyrinth map (special) with H and W
void createFullCityMap(int crtH, int crtW, long long armyMN, long long armyMX, int plCnt);
void createFullSwampMap(int crtH, int crtW, int plCnt);
void createFullPlainMap(int crtH, int crtW, int plCnt);

void getAllFiles(string path, std::vector<string>& files, string fileExt);
void initMaps();
void readMap(int mid); // read map and copy to gameMap
void printMap(int Code, coordS coo); // print the current gameMap
void createOptions(int type,int h);

inline long long PMod(long long& x);
std::pair<long long, long long> bin_search(long long curTurn);
void Zip(); // zip map
void deZip(); // dezip map

bool initSock(); // initialize socket web

/*** settings ***/

// Namespace for LocalGen settings.
namespace LGset {
	const string settingFileExt = ".lgsts";
	const string settingFile = "_settings" + settingFileExt;

	static size_t settingLength = 0;

	wstring userName((L"Anomynous"s + wstring(16,0)).c_str(), 16);
	bool enableGodPower = false; // god power - originated from v1.0.0 bug
	unsigned short defaultPlayerNum = 2;
	unsigned short defaultSpeed = 1;
	unsigned short defaultUserId = 1; // for player color
	bool enableGongSound = true;
	wstring replayFileName((L"replay.lgr"s + wstring(50,0)).c_str(),50);
	bool enableBetaTag = true; // currently no change allowed
	unsigned short socketPort = 14514; // no change allowed
	wstring mainFontName((L"Quicksand"s + wstring(30,0)).c_str(),30);
	unsigned short blockMinFontSize = 8;
	unsigned short blockMaxFontSize = 18;

	inline namespace game {
		/**
		 * game mode:
		 * 0: normal 普通
		 * 1: city state 城邦
		 * 即：只要 CITY > 0 即可为国
		 * 2: land state 社稷江山！
		 * 即：只要 TOT > 0 即可为国
		 */
		unsigned short gameMode = 0;
		short plainRate[3] = {25, 25, 2};
	}

	inline namespace file {
		inline vector<wchar_t> getBuf();
		inline bool check();
		inline void read();
		inline void write();
	} // inline namespace file
} // namespace LGset

/*** graphics ***/

bool FullScreen(HWND hwnd, int fullscreenWidth = GetSystemMetrics(SM_CXSCREEN), int fullscreenHeight = GetSystemMetrics(SM_CYSCREEN), int colourBits = 32, int refreshRate = 60);

/**
 * @brief Namespace for EGE graphics (except image operations)
 */
namespace LGGraphics {
	constexpr color_t bgColor = 0xff222222; // background color
	constexpr color_t mainColor = 0xff008080; // main color
	constexpr color_t errorColor = 0xfffbbbbb; // error color
	PIMAGE iconImg; // favicon image
	string fileName; // ???
	[[deprecated]] int stDel = 1; // temporary variable for speed (deprecated)
	int plCnt = 0; // temporary variable for count of players
	int mapSelected = 0; // ID of the map selected
	int cheatCode = 0; // binary code of visibility in game
	/**
	 * @brief Struct for storing printing info of map.
	 */
	struct WindowDataS {
		int heightPerBlock; // block height in printing
		int widthPerBlock; // block width in printing
		int height, width; // ???
		double zoomX, zoomY; // window size zoom (relatively to 1920x1080)
		int maplocX, maplocY; // location of map in printing
	} windowData; // storing variable
	void WelcomePage();
	void settingsPage();
	void localOptions();
	void webOptions();
	void createMapPage();
	void replayPage();
	void doMapSelect();
	void importGameSettings();
	void inputMapData(int a, int b, int c, int d);
	int select = 0;
	void initWindowSize();
	bool cheatCodeSelected[13];
	void init();
}

inline int getHeightPerBlock(); // get LGGraphics::windowData.heightPerBlock; deprecated
inline int getWidthPerBlock(); // get LGGraphics::windowData.widthPerBlock; deprecated

/**** others ****/

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
	int gameSpeed;
	int gameMesC;
	string playerNames[64];
	std::vector<int> team[64];
	coordS genCoo[64];
	std::deque<moveS> inlineMove;
	coordS playerCoo[64];
	coordS playerFocus[64];
	std::chrono::nanoseconds beginTime;
	struct turnStatS {
		int id;
		int plain, city, mount, swamp;
		long long army;
		coordS focus, coo;
		inline int gtot() { return plain + city + mount + swamp; }
		inline long long gaih() {
			if(gmp(coo)!=id) return 0;
			return gma(coo);
		}
	};
	vector<turnStatS> gameStats[64];

	void init(int chtC, int pC, int gS);
	void capture(int p1, int p2);
	[[deprecated("This function will be deleted since v5.0.")]]
	int analyzeMove(int id, int mv, coordS& coo);
	int checkMove(moveS coo);
	void flushMove();
	void initGenerals(coordS coos[]);
	void updateMap();
	void statistics();
	void ranklist(bool print);
	void printAnalysis();
}

namespace LGreplay {
	const string replayFileName="replay.lgr";
	char ntoc(int x);
	int cton(char x);
	string ntos(int x,int len=-1);
	int ston(char* s,int len=-1);
	string zipBlock(Block B);
	struct Movement {
		moveS move;
		Movement();
		Movement(moveS c);
		string zip();
	};
	Movement readMove(char* buf);
	struct WReplay {
		string Filename;
		FILE* file;
		WReplay(string Fname=replayFileName);
		void initReplay(string Fname=replayFileName);
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
		RReplay(string Fname=replayFileName);
		void resetGame();
		bool _nextTurn();
		int nextTurn();
		void gotoTurn(int turnid);
		int preTurn();
		void initReplay(string Fname=replayFileName);
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
 * @brief Main Page Function. This page occurs when entering the program.
 */
void MainPage() {
	std::mt19937 mtrd(std::chrono::system_clock::now().time_since_epoch().count());
	LGGraphics::iconImg = newimage();
	getimage_pngfile(LGGraphics::iconImg, "img/favicon.png");
	LGGraphics::WelcomePage();
//	LGGraphics::localOptions();
	return;
}

#endif // __LGDEF_HPP__
