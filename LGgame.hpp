/* This is LGgame.hpp file of LocalGen.                                  */
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

#ifndef __LGGAME_HPP__
#define __LGGAME_HPP__

#include "LGdef.hpp"
#include "LGbot.hpp"
#include "LGreplay.hpp"

void LGgame::printGameMessage(gameMessageStore now) {
	++LGgame::gameMesC;
	setcolor(WHITE);
	// setfont(40 * LGGraphics::mapDataStore.mapSizeY, 0, "Lucida Fax");
	// xyprintf(960 * LGGraphics::mapDataStore.mapSizeX, 330 * LGGraphics::mapDataStore.mapSizeY, "GameMessage");
	setfont(20 * LGGraphics::mapDataStore.mapSizeY, 0, "Courier New");
	settextjustify(RIGHT_TEXT, TOP_TEXT);
	int tmp = 0;
	if(now.playerB == -1) {
		setcolor(playerInfo[now.playerA].color);
		xyprintf(1600 * LGGraphics::mapDataStore.mapSizeX - 1 - textwidth((" won the game at Turn #" + to_string(now.turnNumber)).c_str()), 20 * (LGgame::playerCnt + 2 + LGgame::gameMesC) * LGGraphics::mapDataStore.mapSizeY, "%7s", playerInfo[now.playerA].name.c_str());
		setcolor(RED);
		xyprintf(1600 * LGGraphics::mapDataStore.mapSizeX - 1, 20 * (LGgame::playerCnt + 2 + LGgame::gameMesC) * LGGraphics::mapDataStore.mapSizeY, " won the game at Turn #%d", now.turnNumber);
		setcolor(WHITE);
	} else if(1 == now.playerB && now.playerA == 1)
		xyprintf(1600 * LGGraphics::mapDataStore.mapSizeX - 1, 20 * (LGgame::playerCnt + 2 + LGgame::gameMesC) * LGGraphics::mapDataStore.mapSizeY, "You surrendered at Turn #%d", now.turnNumber);
	else {
		setcolor(playerInfo[now.playerA].color);
		xyprintf(1600 * LGGraphics::mapDataStore.mapSizeX - 1 - textwidth((" at Turn #" + to_string(now.turnNumber)).c_str()) - textwidth((" " + playerInfo[now.playerB].name).c_str()) - textwidth(" killed "), 20 * (LGgame::playerCnt + 2 + LGgame::gameMesC) * LGGraphics::mapDataStore.mapSizeY, "%7s", playerInfo[now.playerA].name.c_str());
		setcolor(WHITE);
		xyprintf(1600 * LGGraphics::mapDataStore.mapSizeX - 1 - textwidth((" at Turn #" + to_string(now.turnNumber)).c_str()) - textwidth((" " + playerInfo[now.playerB].name).c_str()), 20 * (LGgame::playerCnt + 2 + LGgame::gameMesC) * LGGraphics::mapDataStore.mapSizeY, " killed ", now.turnNumber);
		setcolor(playerInfo[now.playerB].color);
		xyprintf(1600 * LGGraphics::mapDataStore.mapSizeX - 1 - textwidth((" at Turn #" + to_string(now.turnNumber)).c_str()), 20 * (LGgame::playerCnt + 2 + LGgame::gameMesC) * LGGraphics::mapDataStore.mapSizeY, "%s", playerInfo[now.playerB].name.c_str());
		setcolor(WHITE);
		xyprintf(1600 * LGGraphics::mapDataStore.mapSizeX - 1, 20 * (LGgame::playerCnt + 2 + LGgame::gameMesC) * LGGraphics::mapDataStore.mapSizeY, " at Turn #%d", now.turnNumber);
	}
	settextjustify(LEFT_TEXT, TOP_TEXT);
}

void LGgame::kill(int p1, int p2) {
	if(p2 == 1) {
		std::chrono::nanoseconds bg = std::chrono::steady_clock::now().time_since_epoch();
		if(!inReplay) MessageBoxA(nullptr, string("YOU ARE KILLED BY PLAYER " + playerInfo[p1].name + " AT TURN " + to_string(LGgame::curTurn) + ".").c_str(), "", MB_OK | MB_SYSTEMMODAL);
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
	printGameMessage({p1, p2, LGgame::curTurn});
	lastTurn[p2] = playerCoord{-1, -1};
}

// movement analyzer
int LGgame::analyzeMove(int id, int mv, playerCoord& coo) {
	switch(mv) {
		case -1:
			break;
		case 0:
			coo = LGgame::genCoo[id];
			lastTurn[id] = coo;
			break;
		case 1 ... 4: {
			playerCoord newCoo{coo.x + dx[mv], coo.y + dy[mv]};
			if(newCoo.x < 1 || newCoo.x > mapH || newCoo.y < 1 || newCoo.y > mapW || gameMap[newCoo.x][newCoo.y].type == 2)
				return 1;
			moveS insMv{
				id,
				coo,
				newCoo,
			};
			LGgame::inlineMove.push_back(insMv);
			coo = newCoo;
			lastTurn[id] = coo;
			break;
		}
		case 5 ... 8: {
			playerCoord newCoo{coo.x + dx[mv - 4], coo.y + dy[mv - 4]};
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
// flush existing movements
void LGgame::flushMove() {
	while(!LGgame::inlineMove.empty()) {
		moveS cur = LGgame::inlineMove.front();
		LGgame::inlineMove.pop_front();
		if(!LGgame::isAlive[cur.id])
			continue;
		if(gameMap[cur.from.x][cur.from.y].player != cur.id)
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
					kill(cur.id, p);
					gameMap[cur.to.x][cur.to.y].type = 4;
					for(auto& mv : LGgame::inlineMove)
						if(mv.id == p)
							mv.id = cur.id;
				}
			}
		}
	}
}
// general init
void LGgame::initGenerals(playerCoord coos[]) {
	std::deque<playerCoord> gens;
	for(int i = 1; i <= mapH; ++i)
		for(int j = 1; j <= mapW; ++j)
			if(gameMap[i][j].type == 3)
				gens.push_back(playerCoord{i, j});
	while(gens.size() < LGgame::playerCnt) {
		std::mt19937 p(std::chrono::system_clock::now().time_since_epoch().count());
		int x, y;
		do x = p() % mapH + 1, y = p() % mapW + 1;
		while(gameMap[x][y].type != 0);
		gens.push_back(playerCoord{x, y});
		gameMap[x][y].type = 3;
		gameMap[x][y].army = 0;
	}
	sort(gens.begin(), gens.end(), [](playerCoord a, playerCoord b) { return a.x == b.x ? a.y < b.y : a.x < b.x; });
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
}
void LGgame::updateMap() {
	++LGgame::curTurn;
	for(int i = 1; i <= mapH; ++i) {
		for(int j = 1; j <= mapW; ++j) {
			if(gameMap[i][j].player == 0) continue;
			switch(gameMap[i][j].type) {
				case 0: {
					/* plain */
					if(LGgame::curTurn % 25 == 0) ++gameMap[i][j].army;
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
		}
	}
}

// ranklist printings
void LGgame::ranklist() {
	setfont(20 * LGGraphics::mapDataStore.mapSizeY, 0, "Quicksand");
	static int nlen, alen, plen, clen, tlen, aihlen, botlen;
	setfillcolor(LGGraphics::bgColor);
	// bar(1600 * LGGraphics::mapDataStore.mapSizeX - nlen - alen - plen - clen - tlen - aihlen - botlen - 35, 20 * LGGraphics::mapDataStore.mapSizeY,
	//     1600 * LGGraphics::mapDataStore.mapSizeX, (LGgame::playerCnt + 2) * 20 * LGGraphics::mapDataStore.mapSizeY + 5 + 5);
	struct node {
		int id;
		long long army;
		int plain, city, tot;
		long long armyInHand;
	} rklst[64];
	for(int i = 1; i <= LGgame::playerCnt; ++i) {
		rklst[i].id = i;
		rklst[i].army = rklst[i].armyInHand = 0;
		rklst[i].plain = rklst[i].city = rklst[i].tot = 0;
	}
	for(int i = 1; i <= mapH; ++i) {
		for(int j = 1; j <= mapW; ++j) {
			if(gameMap[i][j].player == 0)
				continue;
			if(gameMap[i][j].type == 2)
				continue;
			++rklst[gameMap[i][j].player].tot;
			if(gameMap[i][j].type == 0)
				++rklst[gameMap[i][j].player].plain;
			else if(gameMap[i][j].type == 4)
				++rklst[gameMap[i][j].player].city;
			else if(gameMap[i][j].type == 3)
				++rklst[gameMap[i][j].player].city;
			rklst[gameMap[i][j].player].army += gameMap[i][j].army;
		}
	}
	for(int i = 1; i <= LGgame::playerCnt; ++i) {
		if(gameMap[LGgame::playerCoo[i].x][LGgame::playerCoo[i].y].player != i)
			continue;
		rklst[i].armyInHand = gameMap[LGgame::playerCoo[i].x][LGgame::playerCoo[i].y].army;
	}
	std::sort(rklst + 1, rklst + LGgame::playerCnt + 1, [](node a, node b) { return a.army > b.army; });
	nlen = textwidth("PLAYER");
	alen = textwidth("ARMY");
	plen = textwidth("PLAIN");
	clen = textwidth("CITY");
	tlen = textwidth("TOT");
	aihlen = textwidth("ARMY IN HAND");
	botlen = textwidth("BOT NAME");
	for(int i = 1; i <= LGgame::playerCnt; ++i) {
		nlen = max(nlen, textwidth(playerInfo[rklst[i].id].name.c_str()));
		alen = max(alen, textwidth(to_string(rklst[i].army).c_str()));
		plen = max(plen, textwidth(to_string(rklst[i].plain).c_str()));
		clen = max(clen, textwidth(to_string(rklst[i].city).c_str()));
		tlen = max(tlen, textwidth(to_string(rklst[i].tot).c_str()));
		aihlen = max(aihlen, textwidth(to_string(rklst[i].armyInHand).c_str()));
		botlen = max(botlen, textwidth(botName[LGgame::robotId[rklst[i].id]/100+1].c_str()));
	}
	int ed = 1600 * LGGraphics::mapDataStore.mapSizeX;
	int s7 = ed - botlen - 5;
	int s6 = s7 - aihlen - 5;
	int s5 = s6 - tlen - 5;
	int s4 = s5 - clen - 5;
	int s3 = s4 - plen - 5;
	int s2 = s3 - alen - 5;
	int s1 = s2 - nlen - 5;
	int prhei = 20 * LGGraphics::mapDataStore.mapSizeY;
	setcolor(WHITE);
	settextjustify(CENTER_TEXT, TOP_TEXT);
	rectangle(s1, prhei, s2, prhei + prhei);
	rectangle(s2, prhei, s3, prhei + prhei);
	rectangle(s3, prhei, s4, prhei + prhei);
	rectangle(s4, prhei, s5, prhei + prhei);
	rectangle(s5, prhei, s6, prhei + prhei);
	rectangle(s6, prhei, s7, prhei + prhei);
	rectangle(s7, prhei, ed, prhei + prhei);
	xyprintf((s1+s2)/2, prhei, "PLAYER");
	xyprintf((s2+s3)/2, prhei, "ARMY");
	xyprintf((s3+s4)/2, prhei, "PLAIN");
	xyprintf((s4+s5)/2, prhei, "CITY");
	xyprintf((s5+s6)/2, prhei, "TOT");
	xyprintf((s6+s7)/2, prhei, "ARMY IN HAND");
	xyprintf((s7+ed)/2, prhei, "BOT NAME");
	for(int i = 1; i <= LGgame::playerCnt; i++) {
		setfillcolor(playerInfo[rklst[i].id].color);
		bar(s1, prhei * (i + 1), ed, prhei * (i + 2));
		rectangle(s1, prhei * (i + 1), s2, prhei * (i + 2));
		rectangle(s2, prhei * (i + 1), s3, prhei * (i + 2));
		rectangle(s3, prhei * (i + 1), s4, prhei * (i + 2));
		rectangle(s4, prhei * (i + 1), s5, prhei * (i + 2));
		rectangle(s5, prhei * (i + 1), s6, prhei * (i + 2));
		rectangle(s6, prhei * (i + 1), s7, prhei * (i + 2));
		rectangle(s7, prhei * (i + 1), ed, prhei * (i + 2));
		xyprintf((s1+s2)/2, prhei * (i + 1), playerInfo[rklst[i].id].name.c_str());
		xyprintf((s2+s3)/2, prhei * (i + 1), to_string(rklst[i].army).c_str());
		xyprintf((s3+s4)/2, prhei * (i + 1), to_string(rklst[i].plain).c_str());
		xyprintf((s4+s5)/2, prhei * (i + 1), to_string(rklst[i].city).c_str());
		xyprintf((s5+s6)/2, prhei * (i + 1), to_string(rklst[i].tot).c_str());
		xyprintf((s6+s7)/2, prhei * (i + 1), to_string(rklst[i].armyInHand).c_str());
		xyprintf((s7+ed)/2, prhei * (i + 1), botName[LGgame::robotId[rklst[i].id]/100+1].c_str());
	}
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
		cleardevice();
		setrendermode(RENDER_MANUAL);
		LGGraphics::inputMapData(std::min(900 / mapH, 900 / mapW), std::min(900 / mapH, 900 / mapW), mapH, mapW);
		LGGraphics::init();
		std::mt19937 mtrd(std::chrono::system_clock::now().time_since_epoch().count());
		LGgame::robotId[1] = -100;
		for(int i = 2; i <= LGgame::playerCnt; ++i)
			LGgame::robotId[i] = mtrd() % 300;
		LGgame::initGenerals(LGgame::playerCoo);
		for(int i = 1; i <= LGgame::playerCnt; ++i) LGgame::playerCoo[i] = LGgame::genCoo[i];
		std::deque<int> movement;
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
		LGGraphics::mapDataStore.maplocX = - (LGgame::genCoo[1].y) * widthPerBlock + 800 * LGGraphics::mapDataStore.mapSizeX;
		LGGraphics::mapDataStore.maplocY = - (LGgame::genCoo[1].x) * heightPerBlock + 450 * LGGraphics::mapDataStore.mapSizeY;
		int smsx = 0, smsy = 0; bool moved = false;
		std::chrono::steady_clock::duration prsttm;
		for(; is_run();) {
			while(mousemsg()) {
				mouse_msg msg = getmouse();
				if(msg.is_wheel()) {
					widthPerBlock += msg.wheel / 120;
					heightPerBlock += msg.wheel / 120;
					widthPerBlock = max(widthPerBlock, 2);
					heightPerBlock = max(heightPerBlock, 2);
				}
				if(msg.is_move()) {
					if(midact == 1) {
						LGGraphics::mapDataStore.maplocX += msg.x - smsx;
						LGGraphics::mapDataStore.maplocY += msg.y - smsy;
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
							if(msg.x >= LGGraphics::mapDataStore.maplocX &&
							   msg.y >= LGGraphics::mapDataStore.maplocY &&
							   msg.x <= LGGraphics::mapDataStore.maplocX + widthPerBlock * mapW &&
							   msg.y <= LGGraphics::mapDataStore.maplocY + heightPerBlock * mapH) {
								int lin = (msg.y + heightPerBlock - 1 - LGGraphics::mapDataStore.maplocY) / heightPerBlock;
								int col = (msg.x + widthPerBlock - 1 - LGGraphics::mapDataStore.maplocX) / widthPerBlock;
								LGgame::playerCoo[1] = {lin, col};
								movement.clear();
							}
						}
					}
				}
			}
			while(kbmsg()) {
				key_msg ch = getkey();
				if(ch.key == key_space) {
					std::chrono::nanoseconds bg = std::chrono::steady_clock::now().time_since_epoch();
					while((!kbmsg()) || (getkey().key != key_space));
					std::chrono::nanoseconds ed = std::chrono::steady_clock::now().time_since_epoch();
					LGgame::beginTime += ed - bg;
				}
				if(ch.msg == key_msg_up)
					continue;
				switch(ch.key) {
					case int('w'): movement.emplace_back(1); break;
					case int('a'): movement.emplace_back(2); break;
					case int('s'): movement.emplace_back(3); break;
					case int('d'): movement.emplace_back(4); break;

					case key_up: /*[UP]*/ movement.emplace_back(5); break;
					case key_left: /*[LEFT]*/ movement.emplace_back(6); break;
					case key_down: /*[DOWN]*/ movement.emplace_back(7); break;
					case key_right: /*[RIGHT]*/ movement.emplace_back(8); break;

					case int('g'): movement.emplace_back(0); break;
					case int('e'):
						if(!movement.empty())
							movement.pop_back();
						break;
					case int('q'): movement.clear(); break;
					case 27: {
						MessageBoxA(getHWnd(), string("YOU QUIT THE GAME.").c_str(), "EXIT", MB_OK | MB_SYSTEMMODAL);
						closegraph();
						return 0;
					}
					case int('\b'): {
						if(!LGgame::isAlive[1])
							break;
						std::chrono::nanoseconds bg = std::chrono::steady_clock::now().time_since_epoch();
						int confirmSur = MessageBoxA(getHWnd(), string("ARE YOU SURE TO SURRENDER?").c_str(), "CONFIRM SURRENDER", MB_YESNO | MB_SYSTEMMODAL);
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
						LGgame::printGameMessage({1, 1, LGgame::curTurn});
						lastTurn[1] = playerCoord{-1, -1};
						break;
					}
				}
			}
			LGgame::updateMap();
			LGreplay::wreplay.newTurn();
			playerCoord tmpcoo=LGgame::playerCoo[1];
			while(!movement.empty() && LGgame::analyzeMove(1, movement.front(), LGgame::playerCoo[1]))
				movement.pop_front(),tmpcoo=LGgame::playerCoo[1];
			int mv;
			if(!movement.empty()) {
				mv=movement.front();
				if(mv>=1&&mv<=4) {
					LGreplay::Movement mov(1,mv,tmpcoo);
					LGreplay::wreplay.newMove(mov);
				}
				movement.pop_front();
			}
			// MessageBoxA(getHWnd(), string("TESTING").c_str(), "TEST MESSAGE", MB_OK);
			for(int i = 2; i <= LGgame::playerCnt; ++i) {
				if(!LGgame::isAlive[i])
					continue;
				tmpcoo=LGgame::playerCoo[i];
				switch(LGgame::robotId[i]) {
					case 0 ... 99:
						mv=smartRandomBot::smartRandomBot(i, LGgame::playerCoo[i]);
						if(!LGgame::analyzeMove(i, mv, LGgame::playerCoo[i])&&mv>=1&&mv<=4) {
							LGreplay::Movement mov(i,mv,tmpcoo);
							LGreplay::wreplay.newMove(mov);
						}
						break;
					case 100 ... 199:
						mv=xrzBot::xrzBot(i, LGgame::playerCoo[i]);
						if(!LGgame::analyzeMove(i, mv, LGgame::playerCoo[i])&&mv>=1&&mv<=4) {
							LGreplay::Movement mov(i,mv,tmpcoo);
							LGreplay::wreplay.newMove(mov);
						}
						break;
					case 200 ... 299:
						mv=xiaruizeBot::xiaruizeBot(i, LGgame::playerCoo[i]);
						if(!LGgame::analyzeMove(i, mv, LGgame::playerCoo[i])&&mv>=1&&mv<=4) {
							LGreplay::Movement mov(i,mv,tmpcoo);
							LGreplay::wreplay.newMove(mov);
						}
						break;
					default:
						LGgame::analyzeMove(i, 0, LGgame::playerCoo[i]);
				}
			}
			LGgame::flushMove();
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
					MessageBoxA(nullptr, "ALL THE PLAYERS YOU SELECTED TO BE SEEN IS DEAD.\nTHE OVERALL CHEAT MODE WILL BE SWITCHED ON.", "TIP", MB_OK | MB_SYSTEMMODAL);
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
					MessageBoxA(nullptr,
					            ("PLAYER " + playerInfo[std::__lg(ed)].name + " WON!" + "\n" +
					             "THE GAME WILL CONTINUE." + "\n" +
					             "YOU CAN PRESS [ESC] TO EXIT.")
					            .c_str(),
					            "GAME END", MB_OK | MB_SYSTEMMODAL);
					std::chrono::nanoseconds eed = std::chrono::steady_clock::now().time_since_epoch();
					LGgame::beginTime += eed - bg;
					gameEnd = 1;
					register int winnerNum = std::__lg(ed);
					LGgame::cheatCode = 1048575;
					LGgame::printGameMessage({winnerNum, -1, LGgame::curTurn});
				}
			}
			if(1) {
				std::chrono::nanoseconds timePassed = std::chrono::steady_clock::now().time_since_epoch() - LGgame::beginTime;
				int needFlushToTurn = ceil(timePassed.count() / 1'000'000'000.0L * LGgame::gameSpeed);
				int lackTurn = LGgame::curTurn - needFlushToTurn;
				if(lackTurn < 0);
				else {
					while(lackTurn > 0) {
						timePassed = std::chrono::steady_clock::now().time_since_epoch() - LGgame::beginTime;
						needFlushToTurn = ceil(timePassed.count() / 1'000'000'000.0L * LGgame::gameSpeed);
						lackTurn = LGgame::curTurn - needFlushToTurn;
					}
					cleardevice();
					printMap(LGgame::cheatCode, LGgame::playerCoo[1]);
					LGgame::ranklist();
					int screenszr = 1600 * LGGraphics::mapDataStore.mapSizeX;
					static int fpslen;
					static int turnlen;
					static int rspeedlen;
					setfillcolor(LGGraphics::bgColor);
					bar(screenszr - rspeedlen - 10 - fpslen - 10 - turnlen - 10, 0, screenszr, 20 * LGGraphics::mapDataStore.mapSizeY);
					setfont(20 * LGGraphics::mapDataStore.mapSizeY, 0, "Quicksand");
					timePassed = std::chrono::steady_clock::now().time_since_epoch() - LGgame::beginTime;
					fpslen = textwidth(("FPS: " + to_string(getfps())).c_str());
					turnlen = textwidth(("Turn " + to_string(LGgame::curTurn) + ".").c_str());
					rspeedlen = textwidth(("Real Speed: " + to_string(LGgame::curTurn * 1.0L / (timePassed.count() / 1'000'000'000.0L))).c_str());				setfillcolor(RED);
					setfillcolor(GREEN);
					bar(screenszr - rspeedlen - 10, 0, screenszr, 20 * LGGraphics::mapDataStore.mapSizeY);
					rectangle(screenszr - rspeedlen - 10, 0, screenszr, 20 * LGGraphics::mapDataStore.mapSizeY);
					setfillcolor(RED);
					bar(screenszr - rspeedlen - 10 - fpslen - 10, 0, screenszr - rspeedlen - 10, 20 * LGGraphics::mapDataStore.mapSizeY);
					rectangle(screenszr - rspeedlen - 10 - fpslen - 10, 0, screenszr - rspeedlen - 10, 20 * LGGraphics::mapDataStore.mapSizeY);
					setfillcolor(BLUE);
					bar(screenszr - rspeedlen - 10 - fpslen - 10 - turnlen - 10, 0, screenszr - rspeedlen - 10 - fpslen - 10, 20 * LGGraphics::mapDataStore.mapSizeY);
					rectangle(screenszr - rspeedlen - 10 - fpslen - 10 - turnlen - 10, 0, screenszr - rspeedlen - 10 - fpslen - 10, 20 * LGGraphics::mapDataStore.mapSizeY);
					settextjustify(CENTER_TEXT, TOP_TEXT);
					xyprintf(screenszr - rspeedlen / 2 - 5, 0, "Real Speed: %Lf", LGgame::curTurn * 1.0L / (timePassed.count() / 1'000'000'000.0L));
					xyprintf(screenszr - rspeedlen - 10 - fpslen / 2 - 5, 0, "FPS: %f", getfps());
					xyprintf(screenszr - rspeedlen - 10 - fpslen - 10 - turnlen / 2 - 5, 0, "Turn %d.", LGgame::curTurn);
				}
			}
		}
		return 0;
	}
}

#endif // __LGGAME_HPP__
