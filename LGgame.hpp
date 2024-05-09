/* This is LGgame.hpp file of LocalGen.                                  */
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

#ifndef __LGGAME_HPP__
#define __LGGAME_HPP__

#include "LGdef.hpp"
#include "LGbot.hpp"
#include "LGreplay.hpp"

void LGgame::capture(int p1, int p2) {
	if(p2 == 1) {
		std::chrono::nanoseconds bg = std::chrono::steady_clock::now().time_since_epoch();
		if((!inReplay)&&(!inServer)) MessageBoxW(nullptr, wstring(L"YOU ARE KILLED BY PLAYER " + playerInfo[p1].name + L" AT TURN " + to_wstring(LGgame::curTurn) + L".").c_str(), L"", MB_OK | MB_SYSTEMMODAL);
		std::chrono::nanoseconds ed = std::chrono::steady_clock::now().time_since_epoch();
		LGgame::beginTime += ed - bg;
	}
	LGgame::isAlive[p2] = 0;
	for(int i = 1; i <= mapH; ++i) {
		for(int j = 1; j <= mapW; ++j) {
			if(gameMap[i][j].player == p2 && gameMap[i][j].type != 3) {
				gameMap[i][j].player = p1;
				gameMap[i][j].army = (gameMap[i][j].army + 1) >> 1;
			}
		}
	}
	gmt(genCoo[p2]) = 4;
	if(LGset::modifier::Leapfrog) {
		// modifier Leap Frog
		gmt(genCoo[p1]) = 4;
		gmt(genCoo[p2]) = 3;
		genCoo[p1] = genCoo[p2];
	}
	lastTurn[p2] = coordS{-1, -1};
}

// movement analyzer
int LGgame::analyzeMove(int id, int mv, coordS& coo) {
	switch(mv) {
		case -1:
			break;
		case 0:
			coo = LGgame::genCoo[id];
			lastTurn[id] = coo;
			break;
		case 1 ... 4: {
			coordS newCoo{coo.x + dx[mv], coo.y + dy[mv]};
			if(newCoo.x < 1 || newCoo.x > mapH || newCoo.y < 1 || newCoo.y > mapW || gameMap[newCoo.x][newCoo.y].type == 2)
				return 1;
			moveS insMv {
				id,
				true,
				coo,
				newCoo,
			};
			LGgame::inlineMove.push_back(insMv);
			coo = newCoo;
			lastTurn[id] = coo;
			break;
		}
		case 5 ... 8: {
			coordS newCoo{coo.x + dx[mv - 4], coo.y + dy[mv - 4]};
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
int LGgame::checkMove(moveS mv) {
	if(mv.id < 1 || mv.id > LGgame::playerCnt) return 5;
	if(mv.from == mv.to) return 4;
	if(mv.from.x < 1 || mv.from.x > mapH || mv.from.y < 1 || mv.from.y > mapW) return 2;
	if(mv.to.x < 1 || mv.to.x > mapH || mv.to.y < 1 || mv.to.y > mapW) return 2;
	if(gameMap[mv.from.x][mv.from.y].type == 2 && mv.takeArmy) return 2;
	if(gameMap[mv.to.x][mv.to.y].type == 2 && mv.takeArmy) return 2;
	if(abs(mv.to.x - mv.from.x) + abs(mv.to.y - mv.from.y) > 1 && mv.takeArmy) return 3; // focus change
	return 0;
}
// flush existing movements
void LGgame::flushMove() {
	while(!LGgame::inlineMove.empty()) {
		moveS cur = LGgame::inlineMove.front();
		LGgame::inlineMove.pop_front();
		if(!cur.takeArmy) {
			LGgame::playerCoo[cur.id] = cur.to;
			continue;
		}
		if(!LGgame::isAlive[cur.id])
			continue;
		if(gameMap[cur.from.x][cur.from.y].player != cur.id && !LGset::enableGodPower)
			continue;
		if(gameMap[cur.to.x][cur.to.y].player == cur.id) {
			gameMap[cur.to.x][cur.to.y].army += gameMap[cur.from.x][cur.from.y].army - 1;
			gameMap[cur.from.x][cur.from.y].army = 1;
		} else {
			gameMap[cur.to.x][cur.to.y].army -= gameMap[cur.from.x][cur.from.y].army - 1;
			gameMap[cur.from.x][cur.from.y].army = 1;
			if(gameMap[cur.to.x][cur.to.y].army < 0) {
				gameMap[cur.to.x][cur.to.y].army = -gameMap[cur.to.x][cur.to.y].army;
				int p = gameMap[cur.to.x][cur.to.y].player;
				gameMap[cur.to.x][cur.to.y].player = cur.id;
				if(gameMap[cur.to.x][cur.to.y].type == 3) {
					/* general */
					capture(cur.id, p);
					// gameMap[cur.to.x][cur.to.y].type = 4; // move to function capture(int,int)
					/* for(auto& mv : LGgame::inlineMove)
						if(mv.id == p)
							mv.id = cur.id; */ // useless and cause bugs
				}
			}
		}
		LGgame::playerCoo[cur.id] = cur.to;
	}
}
// general init
void LGgame::initGenerals(coordS coos[]) {
	std::deque<coordS> gens;
	for(int i = 1; i <= mapH; ++i)
		for(int j = 1; j <= mapW; ++j)
			if(gameMap[i][j].type == 3)
				gens.push_back(coordS{i, j});
	while(gens.size() < LGgame::playerCnt) {
		std::mt19937 p(std::chrono::system_clock::now().time_since_epoch().count());
		int x, y;
		do x = p() % mapH + 1, y = p() % mapW + 1;
		while(gameMap[x][y].type != 0);
		gens.push_back(coordS{x, y});
		gameMap[x][y].type = 3;
		gameMap[x][y].army = 0;
	}
	sort(gens.begin(), gens.end(), [](coordS a, coordS b) { return a.x == b.x ? a.y < b.y : a.x < b.x; });
	std::shuffle(gens.begin(), gens.end(), std::mt19937(std::chrono::system_clock::now().time_since_epoch().count()));
	for(int i = 1; i <= LGgame::playerCnt; ++i) {
		coos[i] = lastTurn[i] = LGgame::genCoo[i] = gens[i - 1];
		gameMap[coos[i].x][coos[i].y].player = i;
		gameMap[coos[i].x][coos[i].y].army = 0;
	}
	for(int i = 1; i <= mapH; ++i)
		for(int j = 1; j <= mapW; ++j)
			if(gameMap[i][j].type == 3 && gameMap[i][j].player == 0)
				gameMap[i][j].type = 0;
	if(LGset::gameMode == 1) {
		for(int i = 1; i <= LGgame::playerCnt; ++i) gmt(LGgame::genCoo[i]) = 4, gma(LGgame::genCoo[i]) = 1;
	} else if(LGset::gameMode == 2) {
		for(int i = 1; i <= LGgame::playerCnt; ++i) gmt(LGgame::genCoo[i]) = 0, gma(LGgame::genCoo[i]) = 1;
	}
}
void LGgame::updateMap() {
	++LGgame::curTurn;
	for(int i = 1; i <= mapH; ++i) {
		for(int j = 1; j <= mapW; ++j) {
			if(gameMap[i][j].player != 0) {
				switch(gameMap[i][j].type) {
					case 0: {
						/* plain */
						if(LGgame::curTurn % LGset::plainRate[LGset::gameMode] == 0) ++gameMap[i][j].army;
						break;
					}
					case 1: {
						/* swamp */
						if(gameMap[i][j].army > 0) if(!(--gameMap[i][j].army)) gameMap[i][j].player = 0;
						break;
					}
					case 2:	   /* mountain */
						break; /* ??? */
					case 3: {
						/* general */
						++gameMap[i][j].army;
						break;
					}
					case 4: {
						/* city */
						++gameMap[i][j].army;
						break;
					}
				}
			} else if(LGset::game::modifier::NeutralResist) {
				switch(gameMap[i][j].type) {
					case 0: {
						/* plain */
						if(LGgame::curTurn % (2*LGset::plainRate[LGset::gameMode]) == 0) ++gameMap[i][j].army;
						break;
					}
					case 1: {
						/* swamp */
						if(gameMap[i][j].army > 0) if(!(--gameMap[i][j].army)) gameMap[i][j].player = 0;
						break;
					}
					case 2:	   /* mountain */
						break; /* ??? */
					case 3: {
						/* general */
						++gameMap[i][j].army;
						break;
					}
					case 4: {
						/* city */
						if(LGgame::curTurn % 3 == 0) ++gameMap[i][j].army;
						break;
					}
				}
			}
		}
	}
}

void LGgame::statistics() {
	for(int i=1; i<=LGgame::playerCnt; ++i) gameStats[i].push_back(turnStatS());
	for(int i=1; i<=LGgame::playerCnt; ++i) gameStats[i].back().id = i;
	for(int i=1; i<=LGgame::playerCnt; ++i) gameStats[i].back().focus = LGgame::playerFocus[i];
	for(int i=1; i<=LGgame::playerCnt; ++i) gameStats[i].back().coo = LGgame::playerCoo[i];
	for(int i=1; i<=mapH; ++i) {
		for(int j=1; j<=mapW; ++j) {
			if(gmp(i,j) == 0) continue;
			gameStats[gmp(i,j)].back().army += gma(i,j);
			if(gmt(i,j) == 0) gameStats[gmp(i,j)].back().plain += 1;
			else if(gmt(i,j) == 1) gameStats[gmp(i,j)].back().swamp += 1;
			else if(gmt(i,j) == 2) gameStats[gmp(i,j)].back().mount += 1;
			else if(gmt(i,j) == 3) gameStats[gmp(i,j)].back().city += 1;
			else if(gmt(i,j) == 4) gameStats[gmp(i,j)].back().city += 1;
		}
	}
}

// ranklist printings
void LGgame::ranklist(bool print) {
	bool printBot = !(LGgame::inReplay|LGgame::inServer|LGgame::inClient);
	bool printAIH = !(LGgame::inReplay);
	setfont(20 * LGGraphics::windowData.zoomY, 0, LGset::mainFontName.c_str());
	static int nlen = -10, alen = -10, plen = -10, clen = -10, tlen = -10, inclen = -10, aihlen = -10, botlen = -10;
	setfillcolor(LGGraphics::bgColor);
	// bar(1600 * LGGraphics::windowData.mapSizeX - nlen - alen - plen - clen - tlen - aihlen - botlen - 35, 20 * LGGraphics::windowData.mapSizeY,
	//     1600 * LGGraphics::windowData.mapSizeX, (LGgame::playerCnt + 2) * 20 * LGGraphics::windowData.mapSizeY + 5 + 5);
	struct node {
		int id;
		long long army;
		int plain, city, tot, inc;
		long long armyInHand;
	} rklst[64];
	for(int i = 1; i <= LGgame::playerCnt; ++i) {
		rklst[i].id = i;
		rklst[i].army = LGgame::gameStats[i].back().army;
		rklst[i].armyInHand = LGgame::gameStats[i].back().gaih();
		rklst[i].plain = LGgame::gameStats[i].back().plain;
		rklst[i].city = LGgame::gameStats[i].back().city;
		rklst[i].inc = LGgame::gameStats[i].back().plain / LGset::plainRate[LGset::gameMode] + LGgame::gameStats[i].back().city - LGgame::gameStats[i].back().swamp;
		rklst[i].tot = LGgame::gameStats[i].back().gtot();
	}
	if(!print) return;
	std::sort(rklst + 1, rklst + LGgame::playerCnt + 1, [](node a, node b) { return a.army > b.army; });
	nlen = textwidth(L"PLAYER");
	if(!LGset::modifier::SilentWar) alen = textwidth(L"ARMY");
	if(!LGset::modifier::SilentWar) plen = textwidth(L"PLAIN");
	if(!LGset::modifier::SilentWar) clen = textwidth(L"CITY");
	if(!LGset::modifier::SilentWar) tlen = textwidth(L"TOT");
	if(!LGset::modifier::SilentWar) tlen = textwidth(L"INC");
	if(!LGset::modifier::SilentWar && printAIH) aihlen = textwidth(L"AIH");
	if(printBot) botlen = textwidth(L"BOT NAME");
	for(int i = 1; i <= LGgame::playerCnt; ++i) {
		nlen = max(nlen, textwidth(playerInfo[rklst[i].id].name.c_str()));
		if(!LGset::modifier::SilentWar) alen = max(alen, textwidth(to_string(rklst[i].army).c_str()));
		if(!LGset::modifier::SilentWar) plen = max(plen, textwidth(to_string(rklst[i].plain).c_str()));
		if(!LGset::modifier::SilentWar) clen = max(clen, textwidth(to_string(rklst[i].city).c_str()));
		if(!LGset::modifier::SilentWar) tlen = max(tlen, textwidth(to_string(rklst[i].tot).c_str()));
		if(!LGset::modifier::SilentWar) inclen = max(inclen, textwidth(to_string(rklst[i].inc).c_str()));
		if(!LGset::modifier::SilentWar && printAIH) aihlen = max(aihlen, textwidth(to_string(rklst[i].armyInHand).c_str()));
		if(printBot) botlen = max(botlen, textwidth(botName[LGgame::robotId[rklst[i].id]/100+1].c_str()));
	}
	int ed = 1600 * LGGraphics::windowData.zoomX;
	int s8 = ed - botlen - 10;
	int s7 = s8 - aihlen - 10;
	int s6 = s7 - inclen - 10;
	int s5 = s6 - tlen - 10;
	int s4 = s5 - clen - 10;
	int s3 = s4 - plen - 10;
	int s2 = s3 - alen - 10;
	int s1 = s2 - nlen - 10;
	int prhei = 20 * LGGraphics::windowData.zoomY;
	setcolor(WHITE);
	settextjustify(CENTER_TEXT, TOP_TEXT);
	setfillcolor(LGGraphics::bgColor);
	bar(s1, prhei * 1, ed, prhei * 2);
	rectangle(s1, prhei, s2, prhei + prhei);
	rectangle(s2, prhei, s3, prhei + prhei);
	rectangle(s3, prhei, s4, prhei + prhei);
	rectangle(s4, prhei, s5, prhei + prhei);
	rectangle(s5, prhei, s6, prhei + prhei);
	rectangle(s6, prhei, s7, prhei + prhei);
	rectangle(s7, prhei, s8, prhei + prhei);
	rectangle(s8, prhei, ed, prhei + prhei);
	xyprintf((s1+s2)/2, prhei, L"PLAYER");
	if(!LGset::modifier::SilentWar) xyprintf((s2+s3)/2, prhei, L"ARMY");
	if(!LGset::modifier::SilentWar) xyprintf((s3+s4)/2, prhei, L"PLAIN");
	if(!LGset::modifier::SilentWar) xyprintf((s4+s5)/2, prhei, L"CITY");
	if(!LGset::modifier::SilentWar) xyprintf((s5+s6)/2, prhei, L"TOT");
	if(!LGset::modifier::SilentWar) xyprintf((s6+s7)/2, prhei, L"INC");
	if(!LGset::modifier::SilentWar && printAIH) xyprintf((s7+s8)/2, prhei, L"AIH");
	if(printBot) xyprintf((s8+ed)/2, prhei, L"BOT NAME");
	for(int i = 1; i <= LGgame::playerCnt; i++) {
		setfillcolor(playerInfo[rklst[i].id].color);
		if(!LGgame::isAlive[rklst[i].id]) setfillcolor(0xff808080);
		bar(s1, prhei * (i + 1), ed, prhei * (i + 2));
		rectangle(s1, prhei * (i + 1), s2, prhei * (i + 2));
		if(!LGset::modifier::SilentWar) rectangle(s2, prhei * (i + 1), s3, prhei * (i + 2));
		if(!LGset::modifier::SilentWar) rectangle(s3, prhei * (i + 1), s4, prhei * (i + 2));
		if(!LGset::modifier::SilentWar) rectangle(s4, prhei * (i + 1), s5, prhei * (i + 2));
		if(!LGset::modifier::SilentWar) rectangle(s5, prhei * (i + 1), s6, prhei * (i + 2));
		if(!LGset::modifier::SilentWar) rectangle(s6, prhei * (i + 1), s7, prhei * (i + 2));
		if(!LGset::modifier::SilentWar && printAIH) rectangle(s7, prhei * (i + 1), s8, prhei * (i + 2));
		if(printBot) rectangle(s8, prhei * (i + 1), ed, prhei * (i + 2));
		xyprintf((s1+s2)/2, prhei * (i + 1), playerInfo[rklst[i].id].name.c_str());
		if(!LGset::modifier::SilentWar) xyprintf((s2+s3)/2, prhei * (i + 1), to_string(rklst[i].army).c_str());
		if(!LGset::modifier::SilentWar) xyprintf((s3+s4)/2, prhei * (i + 1), to_string(rklst[i].plain).c_str());
		if(!LGset::modifier::SilentWar) xyprintf((s4+s5)/2, prhei * (i + 1), to_string(rklst[i].city).c_str());
		if(!LGset::modifier::SilentWar) xyprintf((s5+s6)/2, prhei * (i + 1), to_string(rklst[i].tot).c_str());
		if(!LGset::modifier::SilentWar) xyprintf((s6+s7)/2, prhei * (i + 1), to_string(rklst[i].inc).c_str());
		if(!LGset::modifier::SilentWar && printAIH) xyprintf((s7+s8)/2, prhei * (i + 1), to_string(rklst[i].armyInHand).c_str());
		if(printBot) xyprintf((s8+ed)/2, prhei * (i + 1), botName[LGgame::robotId[rklst[i].id]/100+1].c_str());
	}
}
void LGgame::printAnalysis() {
#define _log(b,x) (((x)!=-1)?(((x)==0)?(0):(log(x)/log(b)+1)):-1)
	static constexpr double lB = 1.00001;
	static int XTurn = 1, XTurnINC = 1;
	static int YMaxLand = 50;
	static int YMaxArmy = 1;
	static int YMaxCity = 1;
	static double landX[64], landY[64],
	       armyX[64], armyY[64],
	       cityX[64], cityY[64];
	static PIMAGE landI = newimage(600 * LGGraphics::windowData.zoomX, 200 * LGGraphics::windowData.zoomY),
	       armyI = newimage(600 * LGGraphics::windowData.zoomX, 200 * LGGraphics::windowData.zoomY),
	       cityI = newimage(600 * LGGraphics::windowData.zoomX, 200 * LGGraphics::windowData.zoomY);
	setbkcolor(LGGraphics::bgColor, landI);
	setbkcolor(LGGraphics::bgColor, armyI);
	setbkcolor(LGGraphics::bgColor, cityI);
	setbkcolor_f(LGGraphics::bgColor, landI);
	setbkcolor_f(LGGraphics::bgColor, armyI);
	setbkcolor_f(LGGraphics::bgColor, cityI);
	const int graphRDX = 600 * LGGraphics::windowData.zoomX, graphRDY = 200 * LGGraphics::windowData.zoomY;
	static const auto ulLand = [&]()->long long {
		long long upperLimit = 0;
		for(int i=LGgame::playerCnt; i>=1; --i)
			for(auto x : gameStats[i])
				upperLimit = max(upperLimit, (long long)x.gtot());
		return upperLimit;
	};
	static const auto ulArmy = [&]()->long long {
		long long upperLimit = 0;
		for(int i=LGgame::playerCnt; i>=1; --i)
			for(auto x : gameStats[i])
				upperLimit = max(upperLimit, x.army);
		return upperLimit;
	};
	static const auto ulCity = [&]()->long long {
		long long upperLimit = 0;
		for(int i=LGgame::playerCnt; i>=1; --i)
			for(auto x : gameStats[i])
				upperLimit = max(upperLimit, (long long)(x.city+x.plain/LGset::plainRate[LGset::gameMode]));
		return upperLimit;
	};
	static const auto redrawLand = [&]()->void {
		cleardevice(landI);
		// settextjustify(LEFT_TEXT,CENTER_TEXT);
		// xyprintf(0,800 * LGGraphics::windowData.zoomY,"upperLimit: %lld",upperLimit);
		setlinewidth(2,landI);
		setcolor(0xffffffff,landI);
		rectangle(0,0,graphRDX,graphRDY,landI);
		setlinewidth(1,landI);
		for(int i=LGgame::playerCnt; i>=1; --i) {
			setcolor(playerInfo[i].color,landI);
			double cx=0,cy=0;
			long long ca=0;
			for(int j=0; j<=curTurn; ++j) {
				int ct = j;
				long long a;
				a=gameStats[i][j].gtot();
				double nx=_log(lB,ct)*(graphRDX-0)*1.0/_log(lB,XTurn);
				double ny=_log(lB,a)*(graphRDY-0)*1.0/_log(lB,YMaxLand);
				if(nx-cx<=1&&!(ny==0&&cy!=0)) continue;
				line(0+cx,graphRDY-cy,0+nx,graphRDY-ny,landI);
				cx=nx,cy=ny,ca=a;
			}
			landX[i]=cx,landY[i]=cy;
			// xyprintf(0,(800-15*(LGgame::playerCnt-i+1)) * LGGraphics::windowData.zoomY,"%ls - army:%lld cx:%.1f cy:%.1f ca:%.1f",playerInfo[i].name.c_str(),historyArmy[i].back().second,cx,cy,ca);
		}
	};
	static const auto redrawArmy = [&]()->void {
		cleardevice(armyI);
		// settextjustify(LEFT_TEXT,CENTER_TEXT);
		// xyprintf(0,800 * LGGraphics::windowData.zoomY,"upperLimit: %lld",upperLimit);
		setlinewidth(2,armyI);
		setcolor(0xffffffff,armyI);
		rectangle(0,0,graphRDX,graphRDY,armyI);
		setlinewidth(1,armyI);
		for(int i=LGgame::playerCnt; i>=1; --i) {
			setcolor(playerInfo[i].color,armyI);
			double cx=0,cy=0;
			long long ca=0;
			for(int j=0; j<=curTurn; ++j) {
				int ct = j;
				long long a;
				a=gameStats[i][j].army;
				double nx=_log(lB,ct)*(graphRDX-0)*1.0/_log(lB,XTurn);
				double ny=_log(lB,a)*(graphRDY-0)*1.0/_log(lB,YMaxArmy);
				if(nx-cx<=1&&!(ny==0&&cy!=0)) continue;
				line(0+cx,graphRDY-cy,0+nx,graphRDY-ny,armyI);
				cx=nx,cy=ny,ca=a;
			}
			armyX[i]=cx,armyY[i]=cy;
			// xyprintf(0,(800-15*(LGgame::playerCnt-i+1)) * LGGraphics::windowData.zoomY,"%ls - army:%lld cx:%.1f cy:%.1f ca:%.1f",playerInfo[i].name.c_str(),historyArmy[i].back().second,cx,cy,ca);
		}
	};
	static auto redrawCity = [&]()->void {
		cleardevice(cityI);
		// settextjustify(LEFT_TEXT,CENTER_TEXT);
		// xyprintf(0,800 * LGGraphics::windowData.zoomY,"upperLimit: %lld",upperLimit);
		setlinewidth(2,cityI);
		setcolor(0xffffffff,cityI);
		rectangle(0,0,graphRDX,graphRDY,cityI);
		setlinewidth(1,cityI);
		for(int i=LGgame::playerCnt; i>=1; --i) {
			setcolor(playerInfo[i].color,cityI);
			double cx=0,cy=0;
			long long ca=0;
			for(int j=0; j<=curTurn; ++j) {
				int ct = j;
				long long a;
				a=gameStats[i][j].city+gameStats[i][j].plain/LGset::plainRate[LGset::gameMode];
				double nx=_log(lB,ct)*(graphRDX-0)*1.0/_log(lB,XTurn);
				double ny=_log(lB,a)*(graphRDY-0)*1.0/_log(lB,YMaxCity);
				if(nx-cx<=1&&!(ny==0&&cy!=0)) continue;
				line(0+cx,graphRDY-cy,0+nx,graphRDY-ny,cityI);
				cx=nx,cy=ny,ca=a;
			}
			cityX[i]=cx,cityY[i]=cy;
			// xyprintf(0,(800-15*(LGgame::playerCnt-i+1)) * LGGraphics::windowData.zoomY,"%ls - army:%lld cx:%.1f cy:%.1f ca:%.1f",playerInfo[i].name.c_str(),historyArmy[i].back().second,cx,cy,ca);
		}
	};
	bool flagLand, flagArmy, flagcity;
	flagLand = flagArmy = flagcity = false;
	while(XTurn < LGgame::curTurn) {
		XTurn += XTurnINC;
		if(XTurn >= XTurnINC * 10) XTurnINC *= 10;
		flagLand = flagArmy = flagcity = true;
	}
	while(YMaxLand < ulLand()) {
		YMaxLand += 50;
		flagLand = true;
	}
	while(YMaxArmy < ulArmy()) {
		YMaxArmy *= 2;
		flagArmy = true;
	}
	while(YMaxCity < ulCity()) {
		YMaxCity += 50;
		flagcity = true;
	}
	if(flagLand) redrawLand();
	else {
		for(int i=LGgame::playerCnt; i>=1; --i) {
			setcolor(playerInfo[i].color,landI);
			double cx=0,cy=0;
			int ct = curTurn; long long a;
			a = gameStats[i].back().gtot();
			cx=_log(lB,ct)*(graphRDX-0)*1.0/_log(lB,XTurn);
			cy=_log(lB,a)*(graphRDY-0)*1.0/_log(lB,YMaxLand);
			if(cx-landX[i]<=1&&!(cy==0&&landY[i]!=0)) continue;
			line(0+landX[i],graphRDY-landY[i],0+cx,graphRDY-cy,landI);
			landX[i]=cx,landY[i]=cy;
		}
	}
	if(flagArmy) redrawArmy();
	else {
		for(int i=LGgame::playerCnt; i>=1; --i) {
			setcolor(playerInfo[i].color,armyI);
			double cx=0,cy=0;
			int ct = curTurn; long long a;
			a = gameStats[i].back().army;
			cx=_log(lB,ct)*(graphRDX-0)*1.0/_log(lB,XTurn);
			cy=_log(lB,a)*(graphRDY-0)*1.0/_log(lB,YMaxArmy);
			if(cx-armyX[i]<=1&&!(cy==0&&armyY[i]!=0)) continue;
			line(0+armyX[i],graphRDY-armyY[i],0+cx,graphRDY-cy,armyI);
			armyX[i]=cx,armyY[i]=cy;
		}
	}
	if(flagcity) redrawCity();
	else {
		for(int i=LGgame::playerCnt; i>=1; --i) {
			setcolor(playerInfo[i].color,cityI);
			double cx=0,cy=0;
			int ct = curTurn; long long a;
			a = gameStats[i].back().city+gameStats[i].back().plain/LGset::plainRate[LGset::gameMode];
			cx=_log(lB,ct)*(graphRDX-0)*1.0/_log(lB,XTurn);
			cy=_log(lB,a)*(graphRDY-0)*1.0/_log(lB,YMaxCity);
			if(cx-cityX[i]<=1&&!(cy==0&&cityY[i]!=0)) continue;
			line(0+cityX[i],graphRDY-cityY[i],0+cx,graphRDY-cy,cityI);
			cityX[i]=cx,cityY[i]=cy;
		}
	}
	putimage(1000 * LGGraphics::windowData.zoomX, 300 * LGGraphics::windowData.zoomY, landI);
	putimage(1000 * LGGraphics::windowData.zoomX, 500 * LGGraphics::windowData.zoomY, armyI);
	putimage(1000 * LGGraphics::windowData.zoomX, 700 * LGGraphics::windowData.zoomY, cityI);
#undef _log
}

namespace LGgame {
	void init(int chtC, int pC, int gS) {
		cheatCode = chtC;
		playerCnt = pC;
		gameSpeed = gS;
		inReplay = false;
		inCreate = false;
		for(register int i = 1; i <= pC; ++i) isAlive[i] = 1;
	}
};

namespace LGlocal {
	int GAME() {
		// gong sound
		if(LGset::enableGongSound) {
			MUSIC gongsound;
			gongsound.OpenFile("sound/gong.mp3");
			if(gongsound.IsOpen()) {
				gongsound.Play();
				while(gongsound.GetPlayStatus()==MUSIC_MODE_PLAY);
				gongsound.Close();
			}
		}

		cleardevice();
		setrendermode(RENDER_MANUAL);
		LGGraphics::inputMapData(std::min(900 / mapH, 900 / mapW), std::min(900 / mapH, 900 / mapW), mapH, mapW);
		LGGraphics::init();

		// init robots
		std::mt19937 mtrd(std::chrono::system_clock::now().time_since_epoch().count());
		LGgame::robotId[1] = -100;
		for(int i = 2; i <= LGgame::playerCnt; ++i) {
			// LGgame::robotId[i] = mtrd() % 500;
			// LGgame::robotId[i] = mtrd() % 200 + 300;
			// LGgame::robotId[i] = mtrd() % 100 + 400;
			LGgame::robotId[i] = mtrd() % 100 + 300;
			while(((LGset::gameMode == 1 || LGset::gameMode == 2)
			       && (200 <= LGgame::robotId[i] && LGgame::robotId[i] < 300)))
				// LGgame::robotId[i] = mtrd() % 400;
				// LGgame::robotId[i] = mtrd() % 200 + 300;
				LGgame::robotId[i] = mtrd() % 100 + 300;
			if(300 <= LGgame::robotId[i] && LGgame::robotId[i] < 400) zlyBot::initBot(i);
			if(400 <= LGgame::robotId[i] && LGgame::robotId[i] < 500) zlyBot_v2::initBot(i);
		}

		// init generals
		LGgame::initGenerals(LGgame::playerCoo);

		for(int i = 1; i <= LGgame::playerCnt; ++i) LGgame::playerFocus[i] = LGgame::playerCoo[i] = LGgame::genCoo[i];
		std::deque<moveS> movement;
		LGgame::updateMap();
		Zip();
		LGreplay::wreplay.initReplay();
		printMap(LGgame::cheatCode, LGgame::playerCoo[1]);
		LGgame::curTurn = 0;
		bool gameEnd = 0;
		LGgame::beginTime = std::chrono::steady_clock::now().time_since_epoch();
		flushkey();
		flushmouse();
		int midact = 0;
		LGGraphics::windowData.maplocX = - (LGgame::genCoo[1].y) * blockWidth + 800 * LGGraphics::windowData.zoomX;
		LGGraphics::windowData.maplocY = - (LGgame::genCoo[1].x) * blockHeight + 450 * LGGraphics::windowData.zoomY;
		int smsx = 0, smsy = 0; bool moved = false;
		std::chrono::steady_clock::duration prsttm;
		bool toNextTurn = true, gamePaused = false;
		std::chrono::nanoseconds pauseBeginTime, pauseEndTime;
		bool shiftPressed = false;
		LGgame::statistics();
		delay_ms(0);
		for(; is_run();) {
			while(mousemsg()) {
				mouse_msg msg = getmouse();
				if(msg.is_wheel()) {
					int msx = msg.x, msy = msg.y;
					int blx = (msg.y - LGGraphics::windowData.maplocY) / blockHeight;
					int bly = (msg.x - LGGraphics::windowData.maplocX) / blockWidth;
					blockWidth += msg.wheel / 120 * 2;
					blockHeight += msg.wheel / 120 * 2;
					blockWidth = max(blockWidth, 10);
					blockHeight = max(blockHeight, 10);
					LGGraphics::windowData.maplocX = msx - bly * blockWidth;
					LGGraphics::windowData.maplocY = msy - blx * blockHeight;
				}
				if(msg.is_move()) {
					if(midact == 1) {
						LGGraphics::windowData.maplocX += msg.x - smsx;
						LGGraphics::windowData.maplocY += msg.y - smsy;
						smsx = msg.x, smsy = msg.y; moved = true;
					}
				} else if(msg.is_left()) {
					if(msg.is_down()) {
						prsttm = std::chrono::steady_clock::now().time_since_epoch();
						midact = 1;
						smsx = msg.x, smsy = msg.y;
						moved = false;
					} else {
						midact = 0;
						std::chrono::steady_clock::duration now = std::chrono::steady_clock::now().time_since_epoch();
						if(!moved && now - prsttm < 200ms) {
							if(msg.x >= LGGraphics::windowData.maplocX &&
							   msg.y >= LGGraphics::windowData.maplocY &&
							   msg.x <= LGGraphics::windowData.maplocX + blockWidth * mapW &&
							   msg.y <= LGGraphics::windowData.maplocY + blockHeight * mapH) {
								int lin = (msg.y + blockHeight - 1 - LGGraphics::windowData.maplocY) / blockHeight;
								int col = (msg.x + blockWidth - 1 - LGGraphics::windowData.maplocX) / blockWidth;
								moveS mv { 1, false, LGgame::playerFocus[1], coordS{lin,col} };
								if(!LGgame::checkMove(mv)) {
									LGgame::inlineMove.push_back(mv);
									LGgame::playerFocus[1] = mv.to;
								}
							}
						}
					}
				}
			}
			while(kbmsg()) {
				key_msg ch = getkey();
				if(ch.key == key_space) {
					if(!gamePaused) {
						pauseBeginTime = std::chrono::steady_clock::now().time_since_epoch();
						gamePaused = true;
					} else {
						gamePaused = false;
						pauseEndTime = std::chrono::steady_clock::now().time_since_epoch();
						LGgame::beginTime += pauseEndTime - pauseBeginTime;
					}
				}
				if(ch.key == key_shift) {
					if(ch.msg == key_msg_up) shiftPressed = false;
					else if(ch.msg == key_msg_down) shiftPressed = true;
				}
				if(ch.msg == key_msg_up)
					continue;
				switch(ch.key) {
					case int('w'): case key_up: { /*[UP]*/
						coordS to = coordS{LGgame::playerFocus[1].x+dx[1],LGgame::playerFocus[1].y+dy[1]};
						moveS mv = moveS{1, !shiftPressed, LGgame::playerFocus[1], to};
						if(!LGgame::checkMove(mv)) {
							movement.push_back(mv);
							LGgame::playerFocus[1] = to;
						}
					} break;
					case int('a'): case key_left: { /*[LEFT]*/
						coordS to = coordS{LGgame::playerFocus[1].x+dx[2],LGgame::playerFocus[1].y+dy[2]};
						moveS mv = moveS{1, !shiftPressed, LGgame::playerFocus[1], to};
						if(!LGgame::checkMove(mv)) {
							movement.push_back(mv);
							LGgame::playerFocus[1] = to;
						}
					} break;
					case int('s'): case key_down: { /*[DOWN]*/
						coordS to = coordS{LGgame::playerFocus[1].x+dx[3],LGgame::playerFocus[1].y+dy[3]};
						moveS mv = moveS{1, !shiftPressed, LGgame::playerFocus[1], to};
						if(!LGgame::checkMove(mv)) {
							movement.push_back(mv);
							LGgame::playerFocus[1] = to;
						}
					} break;
					case int('d'): case key_right: { /*[RIGHT]*/
						coordS to = coordS{LGgame::playerFocus[1].x+dx[4],LGgame::playerFocus[1].y+dy[4]};
						moveS mv = moveS{1, !shiftPressed, LGgame::playerFocus[1], to};
						if(!LGgame::checkMove(mv)) {
							movement.push_back(mv);
							LGgame::playerFocus[1] = to;
						}
					} break;

					case int('g'): {
						movement.push_back({1, false, LGgame::playerFocus[1], LGgame::genCoo[1]});
						LGgame::playerFocus[1] = LGgame::playerCoo[1] = LGgame::genCoo[1];
						movement.clear();
					} break;
					case int('e'): {
						if(!movement.empty()) {
							moveS mv = movement.back();
							movement.pop_back();
							LGgame::playerFocus[1] = mv.from;
						}
					} break;
					case int('q'): {
						movement.clear();
						LGgame::playerFocus[1] = LGgame::playerCoo[1];
					} break;
					case 27: { /*[ESC]*/
						MessageBoxW(getHWnd(), wstring(L"YOU QUIT THE GAME.").c_str(), L"EXIT", MB_OK | MB_SYSTEMMODAL);
						closegraph();
						return 0;
					}
					case int('\b'): {
						if(!LGgame::isAlive[1])
							break;
						std::chrono::nanoseconds bg = std::chrono::steady_clock::now().time_since_epoch();
						int confirmSur = MessageBoxW(getHWnd(), wstring(L"ARE YOU SURE TO SURRENDER?").c_str(), L"CONFIRM SURRENDER", MB_YESNO | MB_SYSTEMMODAL);
						std::chrono::nanoseconds ed = std::chrono::steady_clock::now().time_since_epoch();
						LGgame::beginTime += ed - bg;
						if(confirmSur == 7)
							break;
						LGgame::isAlive[1] = 0;
						for(int i = 1; i <= mapH; ++i) {
							for(int j = 1; j <= mapW; ++j) {
								if(gameMap[i][j].player == 1) {
									gameMap[i][j].player = 0;
									if(gameMap[i][j].type == 3)
										gameMap[i][j].type = 4;
								}
							}
						}
						lastTurn[1] = coordS{-1, -1};
						break;
					}
				}
			}
			if(gamePaused) toNextTurn = false;
			if(toNextTurn) {
				LGgame::updateMap();
				LGreplay::wreplay.newTurn();
				coordS tmpcoo=LGgame::playerFocus[1];
				moveS mv;
				while(!movement.empty() && LGgame::checkMove(movement.front()))
					movement.pop_front(),tmpcoo=LGgame::playerFocus[1];
				if(!movement.empty()) {
					mv=movement.front();
					LGreplay::Movement mov(mv);
					LGreplay::wreplay.newMove(mov);
					LGgame::inlineMove.push_back(mv);
					movement.pop_front();
				}
				// MessageBoxA(getHWnd(), string("TESTING").c_str(), "TEST MESSAGE", MB_OK);
				for(int i = 2; i <= LGgame::playerCnt; ++i) {
					if(!LGgame::isAlive[i])
						continue;
					tmpcoo=LGgame::playerFocus[i];
					switch(LGgame::robotId[i]) {
						case 0 ... 99:
							mv=smartRandomBot::calcNextMove(i, LGgame::playerFocus[i]);
							if(!LGgame::checkMove(mv)) {
								LGreplay::Movement mov(mv);
								LGreplay::wreplay.newMove(mov);
								LGgame::inlineMove.push_back(mv);
								LGgame::playerFocus[i] = mv.to;
							}
							break;
						case 100 ... 199:
							mv=xrzBot::calcNextMove(i, LGgame::playerFocus[i]);
							if(!LGgame::checkMove(mv)) {
								LGreplay::Movement mov(mv);
								LGreplay::wreplay.newMove(mov);
								LGgame::inlineMove.push_back(mv);
								LGgame::playerFocus[i] = mv.to;
							}
							break;
						case 200 ... 299:
							mv=xiaruizeBot::calcNextMove(i, LGgame::playerFocus[i]);
							if(!LGgame::checkMove(mv)) {
								LGreplay::Movement mov(mv);
								LGreplay::wreplay.newMove(mov);
								LGgame::inlineMove.push_back(mv);
								LGgame::playerFocus[i] = mv.to;
							}
							break;
						case 300 ... 399:
							mv=zlyBot::calcNextMove(i, LGgame::playerFocus[i]);
							if(!LGgame::checkMove(mv)) {
								LGreplay::Movement mov(mv);
								LGreplay::wreplay.newMove(mov);
								LGgame::inlineMove.push_back(mv);
								LGgame::playerFocus[i] = mv.to;
							}
							break;
						case 400 ... 499:
							mv=zlyBot_v2::calcNextMove(i, LGgame::playerFocus[i]);
							if(!LGgame::checkMove(mv)) {
								LGreplay::Movement mov(mv);
								LGreplay::wreplay.newMove(mov);
								LGgame::inlineMove.push_back(mv);
								LGgame::playerFocus[i] = mv.to;
							}
							break;
					}
				}
				LGgame::flushMove();
				LGgame::statistics();
				if(LGset::gameMode != 0) {
					if(LGset::gameMode == 1) {
						for(int id=1; id<=LGgame::playerCnt; ++id) {
							if(LGgame::gameStats[id].back().city<=0) {
								LGgame::isAlive[id] = 0;
								for(int i=1; i<=mapH; ++i) for(int j=1; j<=mapW; ++j) if(gmp(i,j)==id) gmp(i,j)=0;
							}
						}
					} else if(LGset::gameMode == 2) {
						for(int id=1; id<=LGgame::playerCnt; ++id) {
							if(LGgame::gameStats[id].back().gtot()<=0) LGgame::isAlive[id] = 0;
						}
					}
				}
			}
			if(LGgame::cheatCode != 1048575) {
				int alldead = 0;
				for(int i = 1; i <= LGgame::playerCnt && !alldead; ++i) {
					if(LGgame::cheatCode & (1 << i))
						if(LGgame::isAlive[i])
							alldead = 1;
				}
				if(!alldead) {
					LGgame::cheatCode = 1048575;
					std::chrono::nanoseconds bg = std::chrono::steady_clock::now().time_since_epoch();
					MessageBoxW(nullptr, L"ALL THE PLAYERS YOU SELECTED TO BE SEEN IS DEAD.\nTHE OVERALL CHEAT MODE WILL BE SWITCHED ON.", L"TIP", MB_OK | MB_SYSTEMMODAL);
					std::chrono::nanoseconds ed = std::chrono::steady_clock::now().time_since_epoch();
					LGgame::beginTime += ed - bg;
				}
			}
			if(!gameEnd) {
				int ed = 0;
				for(int i = 1; i <= LGgame::playerCnt; ++i)
					ed |= (LGgame::isAlive[i] << i);
				if(__builtin_popcount(ed) == 1) {
					std::chrono::nanoseconds bg = std::chrono::steady_clock::now().time_since_epoch();
					MessageBoxW(nullptr,
					            (L"PLAYER " + playerInfo[std::__lg(ed)].name + L" WON!" + L"\n" +
					             L"THE GAME WILL CONTINUE." + L"\n" +
					             L"YOU CAN PRESS [ESC] TO EXIT.")
					            .c_str(),
					            L"GAME END", MB_OK | MB_SYSTEMMODAL);
					std::chrono::nanoseconds eed = std::chrono::steady_clock::now().time_since_epoch();
					LGgame::beginTime += eed - bg;
					gameEnd = 1;
					register int winnerNum = std::__lg(ed);
					LGgame::cheatCode = 1048575;
				}
			}
			if(1) {
				toNextTurn = true;
				std::chrono::nanoseconds timePassed = std::chrono::steady_clock::now().time_since_epoch() - LGgame::beginTime;
				int needFlushToTurn = ceil(timePassed.count() / 1'000'000'000.0L * LGgame::gameSpeed);
				int lackTurn = needFlushToTurn - LGgame::curTurn;
				if(lackTurn<=0||gamePaused) {
					cleardevice();
					printMap(LGgame::cheatCode, LGgame::playerFocus[1]);
				}
				LGgame::ranklist(lackTurn<=0||gamePaused);
				if(lackTurn<=0||gamePaused) {
					if(LGset::enableAnalysisInGame) LGgame::printAnalysis();
					int screenszr = 1600 * LGGraphics::windowData.zoomX;
					static int fpslen;
					static int turnlen;
					static int rspeedlen;
					setfillcolor(LGGraphics::bgColor);
					bar(screenszr - rspeedlen - 10 - fpslen - 10 - turnlen - 10, 0, screenszr, 20 * LGGraphics::windowData.zoomY);
					setfont(20 * LGGraphics::windowData.zoomY, 0, LGset::mainFontName.c_str());
					timePassed = std::chrono::steady_clock::now().time_since_epoch() - LGgame::beginTime;
					fpslen = textwidth((L"FPS: " + to_wstring(getfps())).c_str());
					turnlen = textwidth((L"Turn " + to_wstring(LGgame::curTurn) + L".").c_str());
					rspeedlen = textwidth((L"Real Speed: " + to_wstring(LGgame::curTurn * 1.0L / (timePassed.count() / 1'000'000'000.0L))).c_str());				setfillcolor(RED);
					setfillcolor(GREEN);
					bar(screenszr - rspeedlen - 10, 0, screenszr, 20 * LGGraphics::windowData.zoomY);
					rectangle(screenszr - rspeedlen - 10, 0, screenszr, 20 * LGGraphics::windowData.zoomY);
					setfillcolor(RED);
					bar(screenszr - rspeedlen - 10 - fpslen - 10, 0, screenszr - rspeedlen - 10, 20 * LGGraphics::windowData.zoomY);
					rectangle(screenszr - rspeedlen - 10 - fpslen - 10, 0, screenszr - rspeedlen - 10, 20 * LGGraphics::windowData.zoomY);
					setfillcolor(BLUE);
					bar(screenszr - rspeedlen - 10 - fpslen - 10 - turnlen - 10, 0, screenszr - rspeedlen - 10 - fpslen - 10, 20 * LGGraphics::windowData.zoomY);
					rectangle(screenszr - rspeedlen - 10 - fpslen - 10 - turnlen - 10, 0, screenszr - rspeedlen - 10 - fpslen - 10, 20 * LGGraphics::windowData.zoomY);
					settextjustify(CENTER_TEXT, TOP_TEXT);
					xyprintf(screenszr - rspeedlen / 2 - 5, 0, L"Real Speed: %Lf", LGgame::curTurn * 1.0L / (timePassed.count() / 1'000'000'000.0L));
					xyprintf(screenszr - rspeedlen - 10 - fpslen / 2 - 5, 0, L"FPS: %f", getfps());
					xyprintf(screenszr - rspeedlen - 10 - fpslen - 10 - turnlen / 2 - 5, 0, L"Turn %d.", LGgame::curTurn);
					timePassed = std::chrono::steady_clock::now().time_since_epoch() - LGgame::beginTime;
					needFlushToTurn = ceil(timePassed.count() / 1'000'000'000.0L * LGgame::gameSpeed);
					lackTurn = LGgame::curTurn - needFlushToTurn;
					if(lackTurn <= 0 || gamePaused) toNextTurn = false;
				}
			}
		}
		return 0;
	}
}

#endif // __LGGAME_HPP__
