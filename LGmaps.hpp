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
			if(printCode & (1 << gameMap[x + i][y + j].player))
				return true;
	return false;
}
void printNum(bool visible, long long army, int player, int curx, int cury) {
	if(!visible)
		return;
	if(visible) {
		if(army < 0) {
			register long long absd = -army;
			if(absd < 100)
				xyprintf(LGGraphics::mapDataStore.maplocX + widthPerBlock * (cury - 1) + widthPerBlock / 2,
				         LGGraphics::mapDataStore.maplocY + heightPerBlock * (curx - 1) + heightPerBlock / 2,
				         "%lld", army);
			else if(absd < (ll)(1e13)) {
				string p = to_string(army);
				xyprintf(LGGraphics::mapDataStore.maplocX + widthPerBlock * (cury - 1) + widthPerBlock / 2,
				         LGGraphics::mapDataStore.maplocY + heightPerBlock * (curx - 1) + heightPerBlock / 2,
				         "%s%c", p.substr(0, 2).c_str(), NUM_s[p.size() - 3]);
			} else
				xyprintf(LGGraphics::mapDataStore.maplocX + widthPerBlock * (cury - 1) + widthPerBlock / 2,
				         LGGraphics::mapDataStore.maplocY + heightPerBlock * (curx - 1) + heightPerBlock / 2,
				         "-MX");
		} else if(army == 0) {
			if(gameMap[curx][cury].type != 0 && gameMap[curx][cury].type != 1)
				xyprintf(LGGraphics::mapDataStore.maplocX + widthPerBlock * (cury - 1) + widthPerBlock / 2,
				         LGGraphics::mapDataStore.maplocY + heightPerBlock * (curx - 1) + heightPerBlock / 2,
				         "0");
		} else if(army < 1000)
			xyprintf(LGGraphics::mapDataStore.maplocX + widthPerBlock * (cury - 1) + widthPerBlock / 2,
			         LGGraphics::mapDataStore.maplocY + heightPerBlock * (curx - 1) + heightPerBlock / 2,
			         "%lld", army);
		else if(army < (ll)(1e14)) {
			string p = to_string(army);
			xyprintf(LGGraphics::mapDataStore.maplocX + widthPerBlock * (cury - 1) + widthPerBlock / 2,
			         LGGraphics::mapDataStore.maplocY + heightPerBlock * (curx - 1) + heightPerBlock / 2,
			         "%s%c", p.substr(0, 2).c_str(), NUM_s[p.size() - 3]);
		} else
			xyprintf(LGGraphics::mapDataStore.maplocX + widthPerBlock * (cury - 1) + widthPerBlock / 2,
			         LGGraphics::mapDataStore.maplocY + heightPerBlock * (curx - 1) + heightPerBlock / 2, "MAX");
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
	PIMAGE npimg[9];
	for(int i=1; i<=6; ++i) {
		npimg[i] = newimage();
		imageOperation::copyImage(npimg[i],pimg[i]);
		imageOperation::zoomImage(npimg[i],widthPerBlock,heightPerBlock);
	}
	npimg[7]=newimage();
	imageOperation::copyImage(npimg[7],pimg[8]);
	imageOperation::zoomImage(npimg[7],widthPerBlock/3,heightPerBlock/3);
	npimg[8]=newimage();
	imageOperation::copyImage(npimg[8],pimg[8]);
	imageOperation::zoomImage(npimg[8],widthPerBlock,heightPerBlock);
	for(int curx = 1; curx <= mapH; curx++) {
		for(int cury = 1; cury <= mapW; cury++) {
			if(isVisible(curx, cury, printCode)) {
				if(gameMap[curx][cury].player == 0) {
					if(gameMap[curx][cury].type == 0 && gameMap[curx][cury].army == 0)
						setfillcolor(plcol);
					else if(gameMap[curx][cury].type == 0)
						setfillcolor(cscol);
					else if(gameMap[curx][cury].type == 1)
						setfillcolor(cscol);
					else if(gameMap[curx][cury].type == 2)
						setfillcolor(mtcol);
					else if(gameMap[curx][cury].type == 3)
						setfillcolor(LGGraphics::mainColor);
					else if(gameMap[curx][cury].type == 4)
						setfillcolor(cscol);
				} else
					setfillcolor(playerInfo[gameMap[curx][cury].player].color);
			} else
				setfillcolor(unseen);
			bar(LGGraphics::mapDataStore.maplocX + widthPerBlock * (cury - 1),
			    LGGraphics::mapDataStore.maplocY + heightPerBlock * (curx - 1),
			    LGGraphics::mapDataStore.maplocX + widthPerBlock * cury,
			    LGGraphics::mapDataStore.maplocY + heightPerBlock * curx);
			// ege_fillrect(widthPerBlock * (cury - 1), heightPerBlock * (curx - 1), widthPerBlock, heightPerBlock);
			switch(gameMap[curx][cury].type) {
				case 0: {
					/* plain */
					printNum(isVisible(curx, cury, printCode), gameMap[curx][cury].army, gameMap[curx][cury].player, curx, cury);
					break;
				}
				case 1: {
					/* swamp */
					putimage_withalpha(NULL, npimg[4],
					                   LGGraphics::mapDataStore.maplocX + widthPerBlock * (cury - 1),
					                   LGGraphics::mapDataStore.maplocY + heightPerBlock * (curx - 1));
					printNum(isVisible(curx, cury, printCode), gameMap[curx][cury].army, gameMap[curx][cury].player, curx, cury);
					break;
				}
				case 2: {
					/* mountain */
					if(isVisible(curx, cury, printCode))
						putimage_withalpha(NULL, npimg[3],
						                   LGGraphics::mapDataStore.maplocX + widthPerBlock * (cury - 1),
						                   LGGraphics::mapDataStore.maplocY + heightPerBlock * (curx - 1));
					else
						putimage_withalpha(NULL, npimg[5],
						                   LGGraphics::mapDataStore.maplocX + widthPerBlock * (cury - 1),
						                   LGGraphics::mapDataStore.maplocY + heightPerBlock * (curx - 1));
					break;
				}
				case 3: {
					/* general */
					if(isVisible(curx, cury, printCode))
						putimage_withalpha(NULL, npimg[2],
						                   LGGraphics::mapDataStore.maplocX + widthPerBlock * (cury - 1),
						                   LGGraphics::mapDataStore.maplocY + heightPerBlock * (curx - 1));
					printNum(isVisible(curx, cury, printCode), gameMap[curx][cury].army, gameMap[curx][cury].player, curx, cury);
					break;
				}
				case 4: {
					/* city */
					if(isVisible(curx, cury, printCode))
						putimage_withalpha(NULL, npimg[1],
						                   LGGraphics::mapDataStore.maplocX + widthPerBlock * (cury - 1),
						                   LGGraphics::mapDataStore.maplocY + heightPerBlock * (curx - 1));
					else
						putimage_withalpha(NULL, npimg[5],
						                   LGGraphics::mapDataStore.maplocX + widthPerBlock * (cury - 1),
						                   LGGraphics::mapDataStore.maplocY + heightPerBlock * (curx - 1));
					printNum(isVisible(curx, cury, printCode), gameMap[curx][cury].army, gameMap[curx][cury].player, curx, cury);
					break;
				}
			}
			if(LGgame::inCreate&&gameMap[curx][cury].lit) {
				if(gameMap[curx][cury].type==0&&gameMap[curx][cury].army==0) {
					putimage_withalpha(NULL, npimg[8],
					                   LGGraphics::mapDataStore.maplocX + widthPerBlock * (cury - 1),
					                   LGGraphics::mapDataStore.maplocY + heightPerBlock * (curx - 1));
				} else {
					putimage_withalpha(NULL, npimg[7],
					                   LGGraphics::mapDataStore.maplocX + widthPerBlock * (cury - 1),
					                   LGGraphics::mapDataStore.maplocY + heightPerBlock * (curx - 1));
				}
			}
		}
	}
	if(~coo.x||~coo.y) putimage_withalpha(NULL, npimg[6],
		                                      LGGraphics::mapDataStore.maplocX + widthPerBlock * (coo.y - 1),
		                                      LGGraphics::mapDataStore.maplocY + heightPerBlock * (coo.x - 1));
	for(int i=1; i<=8; ++i) delimage(npimg[i]);
	settextjustify(LEFT_TEXT, TOP_TEXT);
}

void createOptions(int type,int h) {
	static const color_t col=0xffdcdcdc,
	                     plcol=0xff3c3c3c,
	                     selcol=0xff008080;
	PIMAGE npimg[9];
	for(int i=1; i<=8; ++i) {
		npimg[i] = newimage();
		imageOperation::copyImage(npimg[i],pimg[i]);
		imageOperation::zoomImage(npimg[i],40,40);
	}
	setcolor(WHITE);
	setfont(14, 0, "Segoe UI");
	settextjustify(CENTER_TEXT, CENTER_TEXT);
	setfillcolor(col);
	bar(0,h,40,h+280);
	setfillcolor(selcol);
	bar(0,h+type*40,40,h+40+type*40);
	setfillcolor(plcol);
	bar(5,h+165,35,h+195);
	putimage_withalpha(NULL,npimg[3],0,h);
	putimage_withalpha(NULL,npimg[4],0,h+40);
	putimage_withalpha(NULL,npimg[2],0,h+80);
	putimage_withalpha(NULL,npimg[1],0,h+120);
	putimage_withalpha(NULL,npimg[8],0,h+200);
	putimage_withalpha(NULL,npimg[7],0,h+240);
	xyprintf(20,h+180,"40");
	for(int i=1; i<=8; ++i) delimage(npimg[i]);
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
			gameMap[i][j].player = 0;
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
			gameMap[i][j].player = 0;
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
			gameMap[i][j].player = 0;
			gameMap[i][j].army = 0;
			gameMap[i][j].lit = 0;
		}
	}
}

void getAllFiles(string path, std::vector<string>& files, string fileType)  {
	long hFile = 0;
	struct _finddata_t fileinfo;
	string p;
	if((hFile = _findfirst(p.assign(path).append("\\*" + fileType).c_str(), &fileinfo)) != -1) {
		do files.push_back(p.assign(path).append("\\").append(fileinfo.name));
		while(_findnext(hFile, &fileinfo) == 0);
		_findclose(hFile);
	}
}

extern int mapNum;
/*
struct MapInfoS { int id; string chiname; string engname; string auth; int hei; int wid; int generalcnt; int swampcnt; int citycnt; int mountaincnt; int plaincnt; MapInfoS() = default; ~MapInfoS() = default; };
*/
void initMaps() {
	mapNum = 5;
	maps[1] = MapInfoS {1, L"随机地图", L"Random", L"LocalGen", 50, 50, 2500, 2500, 2500, 2500, 2500, string()};
	maps[2] = MapInfoS {2, L"标准地图", L"Standard", L"LocalGen", 50, 50, 2500, 2500, 2500, 2500, 2500, string()};
	maps[3] = MapInfoS {3, L"完全塔", L"Full Tower/City", L"LocalGen", 50, 50, 2500, 0, 2500, 0, 0, string()};
	maps[4] = MapInfoS {4, L"大沼泽", L"Great Swamp", L"LocalGen", 50, 50, 2500, 2500, 0, 0, 0, string()};
	maps[5] = MapInfoS {5, L"大平原", L"Great Plain", L"LocalGen", 50, 50, 2500, 0, 0, 0, 2500, string()};
	std::vector<string> files;
	getAllFiles("maps", files, ".ini");
	for(auto x : files) {
		string s = x.substr(0, x.size() - 4);
		wstring chin;
		wstring engn, auth;
		std::wifstream inif(x);
		std::getline(inif, chin);
		std::getline(inif, engn);
		std::getline(inif, auth);
		inif.close();
		++mapNum;
		maps[mapNum].id = mapNum;
		maps[mapNum].chiname = chin;
		maps[mapNum].engname = engn;
		maps[mapNum].filename = s + ".lg";
		maps[mapNum].auth = auth;
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
