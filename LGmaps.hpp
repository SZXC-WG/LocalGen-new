/* This is LGmaps.hpp file of LocalGen.                                  */
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

#ifndef __LGMAPS_HPP__
#define __LGMAPS_HPP__

#include <string>
#include <random>
#include <chrono>
#include <unistd.h>
#include<graphics.h>
using std::string;
using std::to_string;
#include "LGcons.hpp"

#define ll long long

struct Block {
	int team; /* the team who holds this block */
	int type; /* the block's type: 0->plain, 1->swamp, 2->mountain, 3->general, 4->city */
	ll army;  /* count of army on this block */
	bool lit; /* whether the block is lighted(lit) */
};

int mapH, mapW;
Block gameMap[505][505]; /* maximum 500*500 */

struct teamS {
	string name; /* team name */
	int color;	 /* team color */
};
teamS defTeams[64] = {
	{"White", 0xffffff},
	{"Red", 0xff0000},
	{"Green", 0x00ff00},
	{"Blue", 0x0000ff},
	{"Yellow", 0xffff00},
	{"Cyan", 0x00ffff},
	{"Rose", 0xff00ff},
	{"Orange", 0xff8000},
	{"Lime", 0x80ff00},
	{"Brown", 0x804000},
	{"Grey", 0x808080},
	{"Teal", 0x008080},
	{"Purple", 0xc000c0},
	{"Silver", 0xc0c0c0},
	{"Maroon", 0xc00000},
	{"Emerald", 0x00ce80},
	{"Olive", 0x808000},
};

struct playerCoord {
	int x, y;
};

const char NUM_s[20] = {0, 'H', 'K', 'W', 'L', 'M', 'Q', 'I', 'G', 'B', 'N', 'T'};
bool isVisible(int x, int y, int printCode) {
	if(gameMap[x][y].lit)
		return true;
	for(int i = -1; i <= 1; ++i)
		for(int j = -1; j <= 1; ++j)
			if(printCode & (1 << gameMap[x + i][y + j].team))
				return true;
	return false;
}
void printNum(bool visible, long long army, int team, char lchar, char rchar, char mchar=' ', char repchar=' ') {
	if(visible) {
		setfcolor(defTeams[team].color);
		putchar(lchar);
		if(army < 0) {
			register long long absd = -army;
			if(absd < 10) {
				putchar(mchar);
				printf("%2lld", army);
			} else if(absd < 100) {
				printf("%3lld", army);
			} else if(absd < (ll)(1e13)) {
				string p = to_string(army);
				printf("%s%c", p.substr(0, 2).c_str(), NUM_s[p.size() - 3]);
			} else fputs("-MX",stdout);
		} else if(army == 0) {
			putchar(mchar);
			putchar(mchar);
			if(lchar=='['||lchar=='$') putchar('0');
			else putchar(mchar);
		} else if(army < 10) {
			putchar(mchar);
			putchar(mchar);
			printf("%1lld", army);
		} else if(army < 100) {
			putchar(mchar);
			printf("%2lld", army);
		} else if(army < 1000) {
			printf("%3lld", army);
		} else if(army < (ll)(1e14)) {
			string p = to_string(army);
			printf("%s%c", p.substr(0, 2).c_str(), NUM_s[p.size() - 3]);
		} else fputs("MAX",stdout);
		putchar(rchar);
	} else {
		putchar(repchar);
		putchar(repchar);
		putchar(repchar);
		putchar(repchar);
		putchar(repchar);
	}
	resetattr();
	setbcolor(0x000000);
}
void printMap(int printCode, playerCoord coo) {
//	puts(string(mapW * 5 + 2, '_').c_str());
//	for(int i = 1; i <= mapH; ++i) {
//		putchar('|');
//		for(int j = 1; j <= mapW; ++j) {
//			setbcolor(0x000000);
//			setfcolor(0xffffff);
//			if(coo.x==i && coo.y==j) setbcolor(0x000080);
//			switch(gameMap[i][j].type) {
//				case 0: { /* plain */
//					printNum(isVisible(i,j,printCode),gameMap[i][j].army,gameMap[i][j].team,' ',' ');
//					break;
//				}
//				case 1: { /* swamp */
//					printNum(isVisible(i,j,printCode),gameMap[i][j].army,gameMap[i][j].team,'=','=','=','=');
//					break;
//				}
//				case 2: { /* mountain */
//					fputs("#####", stdout);
//					break;
//				}
//				case 3: { /* general */
//					if(!gameMap[i][j].team) {
//						if(isVisible(i, j, printCode)) fputs("$GEN$", stdout);
//						else fputs("     ", stdout);
//					} else printNum(isVisible(i,j,printCode),gameMap[i][j].army,gameMap[i][j].team,'$','$');
//					break;
//				}
//				case 4: { /* city */
//					printNum(isVisible(i,j,printCode),gameMap[i][j].army,gameMap[i][j].team,'[',']',' ','#');
//					break;
//				}
//			}
//		}
//		clearline();
//		setfcolor(0xffffff);
//		putchar('|');
//		putchar('\n');
//	}
//	setfcolor(0xffffff);
//	for(register int i = 1; i <= mapW * 5 + 1; i += 2)
//		fputs("£þ", stdout);
//	putchar('\n');
//	fflush(stdout);
	int widthPerBlock=1920/mapH,heightPerBlock=1080/mapW;
	for(int curx=1;curx<=mapH;curx++)
	{
		for(int cury=1;cury<=mapW;cury++)
		{
			setfillcolor(defTeams[gameMap[curx][cury].team].color);
			bar(widthPerBlock*(curx-1),heightPerBlock*(cury-1),widthPerBlock*curx,heightPerBlock*cury);
		}
	}
}

void createRandomMap(int crtH = -1, int crtW = -1) {
	std::mt19937 mtrd(std::chrono::system_clock::now().time_since_epoch().count());
	register int i, j;

	if(crtH < 0)
		mapH = mtrd() % 50 + 1;
	else
		mapH = crtH;
	if(crtW < 0)
		mapW = mtrd() % 50 + 1;
	else
		mapW = crtW;

	for(i = 1; i <= mapH; i++)
		for(j = 1; j <= mapW; j++) {
			int x = 0, f;
			if(i - 2 > 0 && gameMap[i - 2][j].type == 2)
				x = 1;
			if(j - 2 > 0 && gameMap[i][j - 2].type == 2)
				x = 1;
			if(i - 2 > 0 && j + 2 <= mapW && gameMap[i - 2][j + 2].type == 2)
				x = 1;
			if(i - 2 > 0 && j - 2 > 0 && gameMap[i - 2][j - 2].type == 2)
				x = 1;
			gameMap[i][j].lit=(mtrd()%114514==1);
			if(x) {
				f = mtrd() % 4;
				if(f < 2) {
					gameMap[i][j].type = f;
					gameMap[i][j].army = mtrd() % 2;

					if(gameMap[i][j].army)
						gameMap[i][j].army = mtrd() % 100 + 1;
				} else {
					gameMap[i][j].type = f + 1;
					gameMap[i][j].army = mtrd() % 2;

					if(gameMap[i][j].army)
						gameMap[i][j].army = mtrd() % 100 + 1;
				}
			} else {
				gameMap[i][j].type = mtrd() % 5;
				gameMap[i][j].army = mtrd() % 2;

				if(gameMap[i][j].army)
					gameMap[i][j].army = mtrd() % 100 + 1;
			}
		}
}
void createStandardMap(int crtH = -1, int crtW = -1) {
	//7/8 mountain 1/2 swamp 9 plain 1 city 1 general
	//1/2 swamp 16/17 plain 1 city 1 general
	std::mt19937 mtrd(std::chrono::system_clock::now().time_since_epoch().count());
	register int i, j;

	if(crtH < 0)
		mapH = mtrd() % 50 + 1;
	else
		mapH = crtH;
	if(crtW < 0)
		mapW = mtrd() % 50 + 1;
	else
		mapW = crtW;

	for(i = 1; i <= mapH; i++)
		for(j = 1; j <= mapW; j++) {
			int x1 = 0, x2=0, f;
			if(i - 2 > 0 && gameMap[i - 2][j].type == 2)
				x1 = 1;
			if(j - 2 > 0 && gameMap[i][j - 2].type == 2)
				x1 = 1;
			if(i - 2 > 0 && j + 2 <= mapW && gameMap[i - 2][j + 2].type == 2)
				x1 = 1;
			if(i - 2 > 0 && j - 2 > 0 && gameMap[i - 2][j - 2].type == 2)
				x1 = 1;
			if(i-1>0&&gameMap[i-1][j].type==1) x2=1;
			if(j-1>0&&gameMap[i][j-1].type==1) x2=1;
			
			gameMap[i][j].army=0;
			gameMap[i][j].lit=0;
			f=mtrd()%20;
			
			if(!x1){
				if(f<8-x2) gameMap[i][j].type=2;
				else if(f<9) gameMap[i][j].type=1;
				else if(f<18) gameMap[i][j].type=0;
				else if(f<19) gameMap[i][j].type=3;
				else gameMap[i][j].type=4,gameMap[i][j].army=40;
			}else{
				if(f<1+x2) gameMap[i][j].type=1;
				else if(f<18) gameMap[i][j].type=0;
				else if(f<19) gameMap[i][j].type=3;
				else gameMap[i][j].type=4,gameMap[i][j].army=40;
			}
		}
}
void createFullCityMap(int crtH, int crtW, long long armyMN, long long armyMX, int plCnt) {
	std::mt19937 mtrd(std::chrono::system_clock::now().time_since_epoch().count());
	std::uniform_int_distribution<long long> rd(armyMN, armyMX);
	mapH = crtH, mapW = crtW;
	for(int i = 1; i <= mapH; ++i) {
		for(int j = 1; j <= mapW; ++j) {
			gameMap[i][j].type = 4;
			gameMap[i][j].army = rd(mtrd);
			gameMap[i][j].team = 0;
			gameMap[i][j].lit = 0;
		}
	}
	for(int i = 1; i <= plCnt; ++i) {
		int x, y;
		do
			x = mtrd() % mapH + 1, y = mtrd() % mapW + 1;
		while(gameMap[x][y].type != 4);
		gameMap[x][y].type = 3;
		gameMap[x][y].army = 0;
	}
}
void createFullSwampMap(int crtH, int crtW, int plCnt) {
	std::mt19937 mtrd(std::chrono::system_clock::now().time_since_epoch().count());
	mapH = crtH, mapW = crtW;
	for(int i = 1; i <= mapH; ++i) {
		for(int j = 1; j <= mapW; ++j) {
			gameMap[i][j].type = 1;
			gameMap[i][j].team = 0;
			gameMap[i][j].army = 0;
			gameMap[i][j].lit = 0;
		}
	}
	for(int i = 1; i <= plCnt; ++i) {
		int x, y;
		do
			x = mtrd() % mapH + 1, y = mtrd() % mapW + 1;
		while(gameMap[x][y].type != 1);
		gameMap[x][y].type = 3;
		gameMap[x][y].army = 0;
	}
}
void createFullPlainMap(int crtH, int crtW, int plCnt) {
	mapH = crtH, mapW = crtW;
	for(int i = 1; i <= mapH; ++i) {
		for(int j = 1; j <= mapW; ++j) {
			gameMap[i][j].type = 0;
			gameMap[i][j].team = 0;
			gameMap[i][j].army = 0;
			gameMap[i][j].lit = 0;
		}
	}
}

HINSTANCE defMap;
typedef int (*func1)();
typedef string(*func2)(int, int);
typedef int (*func3)(int, int);
func1 statusCheck;
func2 getMapInfoStr;
func3 getMapInfoNum;

struct MapInfoS {
	int id;
	string chiname;
	string engname;
	string auth;
	int hei;
	int wid;
	int generalcnt;
	int swampcnt;
	int citycnt;
	int mountaincnt;
	int plaincnt;
	MapInfoS() = default;
	~MapInfoS() = default;
};
MapInfoS maps[205];

struct MapGeoS {
	int id;
	string geo;
	int aBits;
	string army;
	string light;
};
MapGeoS mapG[205];

bool dllExit;

int mapNum;

void initDefMap() {
	if(access("defMap.dll", F_OK) == -1) {
		dllExit = 0;
		return;
	} else {
		dllExit = 1;
		defMap = LoadLibrary("defMap.dll");
		statusCheck = (func1)GetProcAddress(defMap, "statusCheck");
		getMapInfoStr = (func2)GetProcAddress(defMap, "getMapInfoStr");
		getMapInfoNum = (func3)GetProcAddress(defMap, "getMapInfoNum");
	}
	mapNum = statusCheck();

	for(int i = 0; i <= mapNum; i++) {
		maps[i].id = mapG[i].id = i;
		maps[i].chiname = getMapInfoStr(i, 1);
		maps[i].engname = getMapInfoStr(i, 2);
		maps[i].auth = getMapInfoStr(i, 3);
		mapG[i].geo = getMapInfoStr(i, 4);
		mapG[i].army = getMapInfoStr(i, 5);
		mapG[i].light = getMapInfoStr(i, 6);
		maps[i].hei = getMapInfoNum(i, 1);
		maps[i].wid = getMapInfoNum(i, 2);
		maps[i].plaincnt = getMapInfoNum(i, 3);
		maps[i].swampcnt = getMapInfoNum(i, 4);
		maps[i].mountaincnt = getMapInfoNum(i, 5);
		maps[i].generalcnt = getMapInfoNum(i, 6);
		maps[i].citycnt = getMapInfoNum(i, 7);
		mapG[i].aBits = getMapInfoNum(i, 8);
	}
}

void copyMap(int mapid) {
	int h = maps[mapid].hei, w = maps[mapid].wid;
	mapH = h, mapW = w;
	for(int i = 1; i <= h; ++i)
		for(int j = 1; j <= w; ++j)
			gameMap[i][j].team = 0;
	for(int i = 1; i <= h; ++i) {
		for(int j = 1; j <= w; ++j) {
			switch(mapG[mapid].geo[(i - 1) * w + j - 1]) {
				case 'S':
					gameMap[i][j].type = 1;
					break;
				case 'G':
					gameMap[i][j].type = 3;
					break;
				case 'P':
					gameMap[i][j].type = 0;
					break;
				case 'M':
					gameMap[i][j].type = 2;
					break;
				case 'C':
					gameMap[i][j].type = 4;
					break;
			}
		}
	}
	for(int i = 1; i <= h; ++i) {
		for(int j = 1; j <= w; ++j) {
			int p = 0, q = ((i - 1) * w + j - 1) * mapG[mapid].aBits;
			for(int k = q; k < q + mapG[mapid].aBits; ++k)
				p = p * 26 + tolower(mapG[mapid].army[k]) - 'a';
			if(isupper(mapG[mapid].army[q]))
				p = -p;
			gameMap[i][j].army = p;
		}
	}
	for(int i = 1; i <= h; ++i)
		for(int j = 1; j <= w; ++j)
			gameMap[i][j].lit = mapG[mapid].light[(i - 1) * w + j - 1] - '0';
}

void exitExe() {
	FreeLibrary(defMap);
	exit(0);
}

#undef ll // long long

#endif // __LGMAPS_HPP
