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

#include "LGdef.hpp"

#define ll long long

bool isVisible(int x, int y, int printCode) {
	if(gameMap[x][y].lit)
		return true;
	for(int i = -1; i <= 1; ++i)
		for(int j = -1; j <= 1; ++j)
			if(printCode & (1 << gameMap[x + i][y + j].team))
				return true;
	return false;
}
void printNum(bool visible, long long army, int team, int curx, int cury) {
	if(!visible)
		return;
	if(visible) {
		if(army < 0) {
			register long long absd = -army;
			if(absd < 100)
				xyprintf(widthPerBlock * (cury - 1) + widthPerBlock / 2, heightPerBlock * (curx - 1) + heightPerBlock / 2, "%lld", army);
			else if(absd < (ll)(1e13)) {
				string p = to_string(army);
				xyprintf(widthPerBlock * (cury - 1) + widthPerBlock / 2, heightPerBlock * (curx - 1) + heightPerBlock / 2, "%s%c", p.substr(0, 2).c_str(), NUM_s[p.size() - 3]);
			} else
				xyprintf(widthPerBlock * (cury - 1) + widthPerBlock / 2, heightPerBlock * (curx - 1) + heightPerBlock / 2, "-MX");
		} else if(army == 0) {
			if(gameMap[curx][cury].type != 0 && gameMap[curx][cury].type != 1)
				xyprintf(widthPerBlock * (cury - 1) + widthPerBlock / 2, heightPerBlock * (curx - 1) + heightPerBlock / 2, "0");
		} else if(army < 1000)
			xyprintf(widthPerBlock * (cury - 1) + widthPerBlock / 2, heightPerBlock * (curx - 1) + heightPerBlock / 2, "%lld", army);
		else if(army < (ll)(1e14)) {
			string p = to_string(army);
			xyprintf(widthPerBlock * (cury - 1) + widthPerBlock / 2, heightPerBlock * (curx - 1) + heightPerBlock / 2, "%s%c", p.substr(0, 2).c_str(), NUM_s[p.size() - 3]);
		} else
			xyprintf(widthPerBlock * (cury - 1) + widthPerBlock / 2, heightPerBlock * (curx - 1) + heightPerBlock / 2, "MAX");
	}
}

void printMap(int printCode, playerCoord coo) {
	static const color_t cscol = 0xff808080,
	                     plcol = 0xffdcdcdc,
	                     mtcol = 0xffbbbbbb,
	                     unseen = 0xff3c3c3c;
	setcolor(WHITE);
	setfont(std::max((heightPerBlock + 2) / 3 * 2 - 2, 3), 0, "Segoe UI");
	settextjustify(CENTER_TEXT, CENTER_TEXT);
	for(int curx = 1; curx <= mapH; curx++) {
		for(int cury = 1; cury <= mapW; cury++) {
			if(isVisible(curx, cury, printCode)) {
				if(gameMap[curx][cury].team == 0) {
					if(gameMap[curx][cury].type == 0 && gameMap[curx][cury].army == 0)
						setfillcolor(plcol);
					else if(gameMap[curx][cury].type == 0)
						setfillcolor(cscol);
					else if(gameMap[curx][cury].type == 1)
						setfillcolor(cscol);
					else if(gameMap[curx][cury].type == 2)
						setfillcolor(mtcol);
					else if(gameMap[curx][cury].type == 4)
						setfillcolor(cscol);
				} else
					setfillcolor(playerInfo[gameMap[curx][cury].team].color);
			} else
				setfillcolor(unseen);
			ege_fillrect(widthPerBlock * (cury - 1), heightPerBlock * (curx - 1), widthPerBlock, heightPerBlock);
			switch(gameMap[curx][cury].type) {
				case 0: {
					/* plain */
					printNum(isVisible(curx, cury, printCode), gameMap[curx][cury].army, gameMap[curx][cury].team, curx, cury);
					break;
				}
				case 1: {
					/* swamp */
					putimage_withalpha(NULL, pimg[4], widthPerBlock * (cury - 1), heightPerBlock * (curx - 1));
					printNum(isVisible(curx, cury, printCode), gameMap[curx][cury].army, gameMap[curx][cury].team, curx, cury);
					break;
				}
				case 2: {
					/* mountain */
					if(isVisible(curx, cury, printCode))
						putimage_withalpha(NULL, pimg[3], widthPerBlock * (cury - 1), heightPerBlock * (curx - 1));
					else
						putimage_withalpha(NULL, pimg[5], widthPerBlock * (cury - 1), heightPerBlock * (curx - 1));
					break;
				}
				case 3: {
					/* general */
					if(isVisible(curx, cury, printCode))
						putimage_withalpha(NULL, pimg[2], widthPerBlock * (cury - 1), heightPerBlock * (curx - 1));
					printNum(isVisible(curx, cury, printCode), gameMap[curx][cury].army, gameMap[curx][cury].team, curx, cury);
					break;
				}
				case 4: {
					/* city */
					if(isVisible(curx, cury, printCode))
						putimage_withalpha(NULL, pimg[1], widthPerBlock * (cury - 1), heightPerBlock * (curx - 1));
					else
						putimage_withalpha(NULL, pimg[5], widthPerBlock * (cury - 1), heightPerBlock * (curx - 1));
					printNum(isVisible(curx, cury, printCode), gameMap[curx][cury].army, gameMap[curx][cury].team, curx, cury);
					break;
				}
			}
		}
	}
	putimage_withalpha(NULL, pimg[6], widthPerBlock * (coo.y - 1), heightPerBlock * (coo.x - 1));
	settextjustify(LEFT_TEXT, TOP_TEXT);
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
			gameMap[i][j].lit = (mtrd() % 114514 == 1);
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
	// 7/8 mountain 1/2 swamp 9 plain 1 city 1 general
	// 1/2 swamp 16/17 plain 1 city 1 general
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
			int x1 = 0, x2 = 0, f;
			if(i - 2 > 0 && gameMap[i - 2][j].type == 2)
				x1 = 1;
			if(j - 2 > 0 && gameMap[i][j - 2].type == 2)
				x1 = 1;
			if(i - 2 > 0 && j + 2 <= mapW && gameMap[i - 2][j + 2].type == 2)
				x1 = 1;
			if(i - 2 > 0 && j - 2 > 0 && gameMap[i - 2][j - 2].type == 2)
				x1 = 1;
			if(i - 1 > 0 && gameMap[i - 1][j].type == 1)
				x2 = 1;
			if(j - 1 > 0 && gameMap[i][j - 1].type == 1)
				x2 = 1;

			gameMap[i][j].army = 0;
			gameMap[i][j].lit = 0;
			f = mtrd() % 20;

			if(!x1) {
				if(f < 8 - x2)
					gameMap[i][j].type = 2;
				else if(f < 9)
					gameMap[i][j].type = 1;
				else if(f < 18)
					gameMap[i][j].type = 0;
				else if(f < 19)
					gameMap[i][j].type = 3;
				else
					gameMap[i][j].type = 4, gameMap[i][j].army = 40;
			} else {
				if(f < 1 + x2)
					gameMap[i][j].type = 1;
				else if(f < 18)
					gameMap[i][j].type = 0;
				else if(f < 19)
					gameMap[i][j].type = 3;
				else
					gameMap[i][j].type = 4, gameMap[i][j].army = 40;
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

void getAllFiles(string path, std::vector<string>& files, string fileType)  {
	long hFile = 0; // 文件句柄
	struct _finddata_t fileinfo; // 文件信息
	string p;
	if((hFile = _findfirst(p.assign(path).append("\\*" + fileType).c_str(), &fileinfo)) != -1) {
		do files.push_back(p.assign(path).append("\\").append(fileinfo.name)); // 保存文件的全路径
		while(_findnext(hFile, &fileinfo) == 0);   // 寻找下一个，成功返回0，否则-1
		_findclose(hFile);
	}
}

extern int mapNum;
/*
struct MapInfoS { int id; string chiname; string engname; string auth; int hei; int wid; int generalcnt; int swampcnt; int citycnt; int mountaincnt; int plaincnt; MapInfoS() = default; ~MapInfoS() = default; };
*/
void initMaps() {
	mapNum = 5;
	maps[1] = MapInfoS {1, "随机地图", "Random", "LocalGen", -1, -1, -1, -1, -1, -1, -1, string()};
	maps[2] = MapInfoS {2, "标准地图", "Standard", "LocalGen", -1, -1, -1, -1, -1, -1, -1, string()};
	maps[3] = MapInfoS {3, "完全塔", "Full Tower/City", "LocalGen", -1, -1, -1, 0, -1, 0, 0, string()};
	maps[4] = MapInfoS {4, "完全沼泽", "Full Swamp", "LocalGen", -1, -1, -1, -1, 0, 0, 0, string()};
	maps[5] = MapInfoS {5, "大平原", "Plain", "LocalGen", -1, -1, -1, 0, 0, 0, -1, string()};
	std::vector<string> files;
	getAllFiles("maps", files, ".ini");
	for(auto x : files) {
		string s = x.substr(0, x.size() - 4);
		string chin, engn;
		std::ifstream inif(x);
		std::getline(inif, chin);
		std::getline(inif, engn);
		inif.close();
		++mapNum;
		maps[mapNum].id = mapNum;
		maps[mapNum].chiname = chin;
		maps[mapNum].engname = engn;
		maps[mapNum].filename = s + ".lg";
		std::ifstream lgf(maps[mapNum].filename);
		lgf.getline(strdeZip, sizeof strdeZip);
		lgf.close();
		deZip();
		maps[mapNum].hei = mapH, maps[mapNum].wid = mapW;
		for(int i=1; i<=mapH; ++i) {
			for(int j=1; j<=mapW; ++j) {
				switch(gameMap[i][j].type) {
					case 0: ++maps[mapNum].plaincnt; break;
					case 1: ++maps[mapNum].swampcnt; break;
					case 2: ++maps[mapNum].mountaincnt; break;
					case 3: ++maps[mapNum].generalcnt; break;
					case 4: ++maps[mapNum].citycnt; break;
				}
			}
		}
	}
}
void readMap(int mid) {
	FILE* file = fopen(maps[mid].filename.c_str(), "r");
	fscanf(file, "%s", strdeZip);
	deZip();
}

#undef ll // long long

#endif // __LGMAPS_HPP
