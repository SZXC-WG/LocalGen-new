/* This is LGmaps.hpp file of LocalGen.                                  */
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

#ifndef __LGMAPS_HPP__
#define __LGMAPS_HPP__

#include "LGdef.hpp"
#include "glib/GLIB_HEAD.hpp"

#define ll long long

bool isVisible(int x, int y, int Code) {
	if(gameMap[x][y].lit)
		return true;
	for(int i = -1; i <= 1; ++i)
		for(int j = -1; j <= 1; ++j)
			if(Code & (1 << gameMap[x + i][y + j].player))
				return true;
	return false;
}
void printBlockNum(bool visible, long long army, int player, int curx, int cury) {
	int luX = LGGraphics::windowData.maplocX + blockWidth * (cury - 1);
	int luY = LGGraphics::windowData.maplocY + blockHeight * (curx - 1);
	string out = to_string(army);
	if(!visible) return;
	if(textwidth(out.c_str()) <= blockWidth - 2)
		outtextxy(luX + blockWidth / 2, luY + blockHeight / 2, out.c_str());
	else {
		while(out.size()>1ull && textwidth((out+".."s).c_str()) > blockWidth - 2) out.pop_back();
		outtextxy(luX + blockWidth / 2, luY + blockHeight / 2, (out+".."s).c_str());
	}
}

void printMap(int Code, coordS coo) {
	static const color_t cscol = 0xff808080,
	                     plcol = 0xffdcdcdc,
	                     mtcol = 0xffbbbbbb,
	                     unseen = 0xff3c3c3c;
	setcolor(WHITE);
	int blockFontSize = std::min(std::max(int(0.375 * blockHeight /* from the site */), (int)(LGset::blockMinFontSize)), (int)(LGset::blockMaxFontSize));
	// setfont(20, 0, LGset::mainFontName.c_str());
	// xyprintf(5 * LGGraphics::windowData.zoomX, 800 * LGGraphics::windowData.zoomY,
	//          "block font size: %d px\n", blockFontSize);
	setfont(-blockFontSize, 0, LGset::mainFontName.c_str());
	settextjustify(CENTER_TEXT, CENTER_TEXT);
	PIMAGE npimg[9];
	for(int i=1; i<=6; ++i) {
		npimg[i] = newimage();
		images::copyImage(npimg[i],pimg[i]);
		images::zoomImage(npimg[i],blockWidth,blockHeight);
	}
	npimg[7]=newimage();
	images::copyImage(npimg[7],pimg[8]);
	images::zoomImage(npimg[7],blockWidth/3,blockHeight/3);
	npimg[8]=newimage();
	images::copyImage(npimg[8],pimg[8]);
	images::zoomImage(npimg[8],blockWidth,blockHeight);
	for(int curx = 1; curx <= mapH; curx++) {
		for(int cury = 1; cury <= mapW; cury++) {
			if(isVisible(curx, cury, Code)) {
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
			bar(LGGraphics::windowData.maplocX + blockWidth * (cury - 1),
			    LGGraphics::windowData.maplocY + blockHeight * (curx - 1),
			    LGGraphics::windowData.maplocX + blockWidth * cury,
			    LGGraphics::windowData.maplocY + blockHeight * curx);
			// ege_fillrect(widthPerBlock * (cury - 1), heightPerBlock * (curx - 1), widthPerBlock, heightPerBlock);
			switch(gameMap[curx][cury].type) {
				case 0: {
					/* plain */
					if(gameMap[curx][cury].army != 0)
						printBlockNum(isVisible(curx, cury, Code), gameMap[curx][cury].army, gameMap[curx][cury].player, curx, cury);
					break;
				}
				case 1: {
					/* swamp */
					putimage_withalpha(NULL, npimg[4],
					                   LGGraphics::windowData.maplocX + blockWidth * (cury - 1),
					                   LGGraphics::windowData.maplocY + blockHeight * (curx - 1));
					if(gameMap[curx][cury].army != 0)
						printBlockNum(isVisible(curx, cury, Code), gameMap[curx][cury].army, gameMap[curx][cury].player, curx, cury);
					break;
				}
				case 2: {
					/* mountain */
					if(isVisible(curx, cury, Code))
						putimage_withalpha(NULL, npimg[3],
						                   LGGraphics::windowData.maplocX + blockWidth * (cury - 1),
						                   LGGraphics::windowData.maplocY + blockHeight * (curx - 1));
					else
						putimage_withalpha(NULL, npimg[5],
						                   LGGraphics::windowData.maplocX + blockWidth * (cury - 1),
						                   LGGraphics::windowData.maplocY + blockHeight * (curx - 1));
					break;
				}
				case 3: {
					/* general */
					if(isVisible(curx, cury, Code))
						putimage_withalpha(NULL, npimg[2],
						                   LGGraphics::windowData.maplocX + blockWidth * (cury - 1),
						                   LGGraphics::windowData.maplocY + blockHeight * (curx - 1));
					printBlockNum(isVisible(curx, cury, Code), gameMap[curx][cury].army, gameMap[curx][cury].player, curx, cury);
					break;
				}
				case 4: {
					/* city */
					if(isVisible(curx, cury, Code))
						putimage_withalpha(NULL, npimg[1],
						                   LGGraphics::windowData.maplocX + blockWidth * (cury - 1),
						                   LGGraphics::windowData.maplocY + blockHeight * (curx - 1));
					else
						putimage_withalpha(NULL, npimg[5],
						                   LGGraphics::windowData.maplocX + blockWidth * (cury - 1),
						                   LGGraphics::windowData.maplocY + blockHeight * (curx - 1));
					printBlockNum(isVisible(curx, cury, Code), gameMap[curx][cury].army, gameMap[curx][cury].player, curx, cury);
					break;
				}
			}
			if(LGgame::inCreate&&gameMap[curx][cury].lit) {
				if(gameMap[curx][cury].type==0&&gameMap[curx][cury].army==0) {
					putimage_withalpha(NULL, npimg[8],
					                   LGGraphics::windowData.maplocX + blockWidth * (cury - 1),
					                   LGGraphics::windowData.maplocY + blockHeight * (curx - 1));
				} else {
					putimage_withalpha(NULL, npimg[7],
					                   LGGraphics::windowData.maplocX + blockWidth * (cury - 1),
					                   LGGraphics::windowData.maplocY + blockHeight * (curx - 1));
				}
			}
		}
	}
	if((~coo.x)||(~coo.y))
		putimage_withalpha(NULL, npimg[6],
		                   LGGraphics::windowData.maplocX + blockWidth * (coo.y - 1),
		                   LGGraphics::windowData.maplocY + blockHeight * (coo.x - 1));
	for(int i=1; i<=8; ++i) delimage(npimg[i]);
	settextjustify(LEFT_TEXT, TOP_TEXT);
}

void createOptions(int type, int h) {
	static const color_t col = 0xffdcdcdc,
	                     plcol = 0xff3c3c3c,
	                     selcol = 0xff008080;
	PIMAGE npimg[9];
	for(int i=1; i<=8; ++i) {
		npimg[i] = newimage();
		images::copyImage(npimg[i],pimg[i]);
		images::zoomImage(npimg[i],40,40);
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
void createLabyrinthMap(int crtH, int crtW, int TYPE) {
	mapH = crtH, mapW = crtW;
	for(int i=1; i<=crtH; ++i) for(int j=1; j<=crtW; ++j) gameMap[i][j].army = 0;
	for(int i=1; i<=crtH; ++i) for(int j=1; j<=crtW; ++j) gameMap[i][j].type = 2;
	for(int i=1; i<=crtH; ++i) for(int j=1; j<=crtW; ++j) gameMap[i][j].lit = false;
	for(int i=1; i<=crtH; ++i) for(int j=1; j<=crtW; ++j) gameMap[i][j].player = 0;
	std::mt19937 mtrd(std::chrono::system_clock::now().time_since_epoch().count());
	static coordS p[10005];
	static std::function<bool(int,int)> adjacent = [](int i,int j)->bool{ return (abs(p[i].x-p[j].x)+abs(p[i].y-p[j].y)==2); };
	static int c,m,f[10005];
	static std::function<int(int)> getf = [](int u)->int{ return f[u]=(f[u]==u?u:getf(f[u])); };
	static std::function<void(int,int)> unite = [](int u,int v)->void{ int x=getf(u),y=getf(v); f[x]=(x==y?f[x]:y); };
	static struct edge { int u,v; } e[10005];
	static int ec[10005],mec[10005];
	static bool vs[10005][10005];
	c=0;
	for(int i=1; i<=crtH; i+=2)
		for(int j=1; j<=crtW; j+=2) {
			p[++c]=coordS{i,j};
			mec[c]=4;
			if(i-2<0) --mec[c];
			if(i+2>crtH) --mec[c];
			if(j-2<0) --mec[c];
			if(j+2>crtW) --mec[c];
			ec[c]=0;
			gameMap[i][j].type=0;
		}
	for(int i=1; i<=c; ++i) for(int j=1; j<=c; ++j) vs[i][j]=false;
	m=c-1;
	if(TYPE==1) ++m;
	if(TYPE==2) m+=crtH+crtW;
	std::iota(f+1,f+c+1,1);
	for(int i=1; i<c; ++i) {
		int u,v;
		while(1) {
			u=mtrd()%c+1,v=mtrd()%c+1;
			if(adjacent(u,v)&&getf(u)!=getf(v)) break;
		}
		unite(u,v);
		e[i]=edge{u,v};
		++ec[u],++ec[v];
		vs[u][v]=vs[v][u]=true;
	}
	for(int i=c; i<=m; ++i) {
		int u,v;
		while(1) {
			u=mtrd()%c+1,v=mtrd()%c+1;
			if(adjacent(u,v)&&!vs[u][v]) break;
		}
		e[i]=edge{u,v};
		++ec[u],++ec[v];
	}
	for(int i=1; i<=m; ++i) {
		coordS a=p[e[i].u],b=p[e[i].v];
		coordS coo = coordS{(a.x+b.x)>>1,(a.y+b.y)>>1};
		gameMap[coo.x][coo.y].type = 4;
		gameMap[coo.x][coo.y].army = 40;
	}
	for(int i=1; i<=c; ++i) {
		gameMap[p[i].x][p[i].y].type = 0;
		if(ec[i]==1) gameMap[p[i].x][p[i].y].type = 3;
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

void getAllFiles(string path, std::vector<string>& files, string fileExt)  {
	long hFile = 0;
	struct _finddata_t fileinfo;
	string p;
	if((hFile = _findfirst(p.assign(path).append("\\*" + fileExt).c_str(), &fileinfo)) != -1) {
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
	mapNum = 8;
	mapInfo[1] = MapInfoS {1, L"随机", L"Random", L"LocalGen", 50, 50, 2500, 2500, 2500, 2500, 2500, string()};
	mapInfo[2] = MapInfoS {2, L"标准", L"Standard", L"LocalGen", 50, 50, 2500, 2500, 2500, 2500, 2500, string()};
	mapInfo[3] = MapInfoS {3, L"单路迷宫（无环迷宫）", L"Single-Path Labyrinth", L"LocalGen", 50, 50, 2500, 2500, 2500, 2500, 2500, string()};
	mapInfo[4] = MapInfoS {4, L"基环迷宫", L"Single-Ring Labyrinth", L"LocalGen", 50, 50, 2500, 2500, 2500, 2500, 2500, string()};
	mapInfo[5] = MapInfoS {5, L"多环迷宫", L"Multi-Ring Labyrinth", L"LocalGen", 50, 50, 2500, 2500, 2500, 2500, 2500, string()};
	mapInfo[6] = MapInfoS {6, L"全塔", L"Full Tower/City", L"LocalGen", 50, 50, 2500, 0, 2500, 0, 0, string()};
	mapInfo[7] = MapInfoS {7, L"大平原", L"Great Plains", L"LocalGen", 50, 50, 2500, 0, 0, 0, 2500, string()};
	mapInfo[8] = MapInfoS {8, L"大沼泽", L"Everglades", L"LocalGen", 50, 50, 2500, 2500, 0, 0, 0, string()};
	std::vector<string> files;
	getAllFiles("maps", files, ".ini");
	for(auto iniFile : files) {
		string s = iniFile.substr(0, iniFile.size() - 4);
		wstring chin;
		wstring engn, auth;
		std::wifstream iniFS(iniFile);
		std::getline(iniFS, chin);
		chin = wcharTransfer(chin);
		std::getline(iniFS, engn);
		std::getline(iniFS, auth);
		iniFS.close();
		++mapNum;
		mapInfo[mapNum].id = mapNum;
		mapInfo[mapNum].chiname = chin;
		mapInfo[mapNum].engname = engn;
		mapInfo[mapNum].mapFile = s + ".lg";
		mapInfo[mapNum].auth = auth;
		std::ifstream lgFS(mapInfo[mapNum].mapFile);
		lgFS.getline(strdeZip, sizeof strdeZip);
		lgFS.close();
		deZip();
		mapInfo[mapNum].height = mapH, mapInfo[mapNum].width = mapW;
		for(int i=1; i<=mapH; ++i) {
			for(int j=1; j<=mapW; ++j) {
				switch(gameMap[i][j].type) {
					case 0: ++mapInfo[mapNum].plaincnt; break;
					case 1: ++mapInfo[mapNum].swampcnt; break;
					case 2: ++mapInfo[mapNum].mountaincnt; break;
					case 3: ++mapInfo[mapNum].generalcnt; break;
					case 4: ++mapInfo[mapNum].citycnt; break;
				}
			}
		}
	}
}
void readMap(int mid) {
	FILE* lgFP = fopen(mapInfo[mid].mapFile.c_str(), "r");
	fscanf(lgFP, "%s", strdeZip);
	deZip();
}

#undef ll // long long

#endif // __LGMAPS_HPP
