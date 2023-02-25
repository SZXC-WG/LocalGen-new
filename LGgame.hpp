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

void printGameMessage(gameMessageStore now) {
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

void kill(int p1, int p2) {
	if(p2 == 1) MessageBoxA(nullptr, string("YOU ARE KILLED BY PLAYER " + playerInfo[p1].name + " AT TURN " + to_string(LGgame::curTurn) + ".").c_str(), "", MB_OK | MB_SYSTEMMODAL);
	LGgame::isAlive[p2] = 0;
	for(int i = 1; i <= mapH; ++i) {
		for(int j = 1; j <= mapW; ++j) {
			if(gameMap[i][j].team == p2 && gameMap[i][j].type != 3) {
				gameMap[i][j].team = p1;
				gameMap[i][j].army = (gameMap[i][j].army + 1) >> 1;
			}
		}
	}
	printGameMessage({p1, p2, LGgame::curTurn});
	lastTurn[p2] = playerCoord{-1, -1};
}

// movement analyzer
int analyzeMove(int id, int mv, playerCoord& coo) {
	movementPack.push(movementS{id, mv, curTurn});
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
void flushMove() {
	while(!LGgame::inlineMove.empty()) {
		moveS cur = LGgame::inlineMove.front();
		LGgame::inlineMove.pop_front();
		if(!LGgame::isAlive[cur.id])
			continue;
		if(gameMap[cur.from.x][cur.from.y].team != cur.id)
			continue;
		if(gameMap[cur.to.x][cur.to.y].team == cur.id) {
			gameMap[cur.to.x][cur.to.y].army += gameMap[cur.from.x][cur.from.y].army - 1;
			gameMap[cur.from.x][cur.from.y].army = 1;
		} else {
			gameMap[cur.to.x][cur.to.y].army -= gameMap[cur.from.x][cur.from.y].army - 1;
			gameMap[cur.from.x][cur.from.y].army = 1;
			if(gameMap[cur.to.x][cur.to.y].army < 0) {
				gameMap[cur.to.x][cur.to.y].army = -gameMap[cur.to.x][cur.to.y].army;
				int p = gameMap[cur.to.x][cur.to.y].team;
				gameMap[cur.to.x][cur.to.y].team = cur.id;
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
void initGenerals(playerCoord coos[]) {
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
		gameMap[coos[i].x][coos[i].y].team = i;
		gameMap[coos[i].x][coos[i].y].army = 0;
	}
	for(int i = 1; i <= mapH; ++i)
		for(int j = 1; j <= mapW; ++j)
			if(gameMap[i][j].type == 3 && gameMap[i][j].team == 0)
				gameMap[i][j].type = 0;
}
void updateMap() {
	++LGgame::curTurn;
	for(int i = 1; i <= mapH; ++i) {
		for(int j = 1; j <= mapW; ++j) {
			if(gameMap[i][j].team == 0) continue;
			switch(gameMap[i][j].type) {
				case 0: {
					/* plain */
					if(LGgame::curTurn % 25 == 0) ++gameMap[i][j].army;
					break;
				}
				case 1: {
					/* swamp */
					if(gameMap[i][j].army > 0) if(!(--gameMap[i][j].army)) gameMap[i][j].team = 0;
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
void ranklist() {
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
			if(gameMap[i][j].team == 0)
				continue;
			if(gameMap[i][j].type == 2)
				continue;
			++rklst[gameMap[i][j].team].tot;
			if(gameMap[i][j].type == 0)
				++rklst[gameMap[i][j].team].plain;
			else if(gameMap[i][j].type == 4)
				++rklst[gameMap[i][j].team].city;
			else if(gameMap[i][j].type == 3)
				++rklst[gameMap[i][j].team].city;
			rklst[gameMap[i][j].team].army += gameMap[i][j].army;
		}
	}
	for(int i = 1; i <= LGgame::playerCnt; ++i) {
		if(gameMap[LGgame::playerCoo[i].x][LGgame::playerCoo[i].y].team != i)
			continue;
		rklst[i].armyInHand = gameMap[LGgame::playerCoo[i].x][LGgame::playerCoo[i].y].army;
	}
	std::sort(rklst + 1, rklst + LGgame::playerCnt + 1, [](node a, node b) { return a.army > b.army; });
	// setfillcolor(WHITE);
	// ege_fillrect(widthPerBlock * mapW, 0, 1600 * LGGraphics::mapDataStore.mapSizeX - widthPerBlock * mapW, 900 * LGGraphics::mapDataStore.mapSizeY);
	// ege_fillrect(0, heightPerBlock * mapH, 1600 * LGGraphics::mapDataStore.mapSizeX, 900 * LGGraphics::mapDataStore.mapSizeY - heightPerBlock * mapH);
	rectBUTTON rkbut;
	rkbut
	.settxtcol(WHITE)
	.setfontname("Courier New")
	.setfonthw(20 * LGGraphics::mapDataStore.mapSizeY, 0)
	.setalign(CENTER_TEXT, CENTER_TEXT)
	.setrtcol(false, WHITE)
	.status = 1;
	rkbut.setbgcol(BLACK);
	rkbut
	.setlocation(20 * LGGraphics::mapDataStore.mapSizeY, 975 * LGGraphics::mapDataStore.mapSizeX)
	.sethw(20 * LGGraphics::mapDataStore.mapSizeY, 75 * LGGraphics::mapDataStore.mapSizeX + 1)
	.poptext().addtext("PLAYER")
	.display();
	rkbut
	.setlocation(20 * LGGraphics::mapDataStore.mapSizeY, 1050 * LGGraphics::mapDataStore.mapSizeX)
	.sethw(20 * LGGraphics::mapDataStore.mapSizeY, 125 * LGGraphics::mapDataStore.mapSizeX + 1)
	.poptext().addtext("ARMY")
	.display();
	rkbut
	.setlocation(20 * LGGraphics::mapDataStore.mapSizeY, 1175 * LGGraphics::mapDataStore.mapSizeX)
	.sethw(20 * LGGraphics::mapDataStore.mapSizeY, 50 * LGGraphics::mapDataStore.mapSizeX + 1)
	.poptext().addtext("PLAIN")
	.display();
	rkbut
	.setlocation(20 * LGGraphics::mapDataStore.mapSizeY, 1225 * LGGraphics::mapDataStore.mapSizeX)
	.sethw(20 * LGGraphics::mapDataStore.mapSizeY, 50 * LGGraphics::mapDataStore.mapSizeX + 1)
	.poptext().addtext("CITY")
	.display();
	rkbut
	.setlocation(20 * LGGraphics::mapDataStore.mapSizeY, 1275 * LGGraphics::mapDataStore.mapSizeX)
	.sethw(20 * LGGraphics::mapDataStore.mapSizeY, 50 * LGGraphics::mapDataStore.mapSizeX + 1)
	.poptext().addtext("TOT")
	.display();
	rkbut
	.setlocation(20 * LGGraphics::mapDataStore.mapSizeY, 1325 * LGGraphics::mapDataStore.mapSizeX)
	.sethw(20 * LGGraphics::mapDataStore.mapSizeY, 125 * LGGraphics::mapDataStore.mapSizeX + 1)
	.poptext().addtext("ARMY IN HAND")
	.display();
	rkbut
	.setlocation(20 * LGGraphics::mapDataStore.mapSizeY, 1450 * LGGraphics::mapDataStore.mapSizeX)
	.sethw(20 * LGGraphics::mapDataStore.mapSizeY, 150 * LGGraphics::mapDataStore.mapSizeX + 1)
	.poptext().addtext("WHICH BOT?")
	.display();
	for(int i = 1; i <= LGgame::playerCnt; i++) {
		rkbut.setbgcol(playerInfo[rklst[i].id].color);
		rkbut
		.setlocation(20 * (i + 1) * LGGraphics::mapDataStore.mapSizeY, 975 * LGGraphics::mapDataStore.mapSizeX)
		.sethw(20 * LGGraphics::mapDataStore.mapSizeY, 75 * LGGraphics::mapDataStore.mapSizeX + 1)
		.poptext().addtext(playerInfo[rklst[i].id].name)
		.display();
		char s[1005];
		if(rklst[i].army < 1000000000) sprintf(s, "%lld", rklst[i].army);
		else {
			register int p = std::to_string(rklst[i].army * 1.0L / 1e9L).find('.');
			sprintf(s, "%.*.*LfG", 7, 7 - 1 - p, rklst[i].army * 1.0L / 1e9L);
		}
		rkbut
		.setlocation(20 * (i + 1) * LGGraphics::mapDataStore.mapSizeY, 1050 * LGGraphics::mapDataStore.mapSizeX)
		.sethw(20 * LGGraphics::mapDataStore.mapSizeY, 125 * LGGraphics::mapDataStore.mapSizeX + 1)
		.poptext().addtext(s)
		.display();
		rkbut
		.setlocation(20 * (i + 1) * LGGraphics::mapDataStore.mapSizeY, 1175 * LGGraphics::mapDataStore.mapSizeX)
		.sethw(20 * LGGraphics::mapDataStore.mapSizeY, 50 * LGGraphics::mapDataStore.mapSizeX + 1)
		.poptext().addtext(to_string(rklst[i].plain))
		.display();
		rkbut
		.setlocation(20 * (i + 1) * LGGraphics::mapDataStore.mapSizeY, 1225 * LGGraphics::mapDataStore.mapSizeX)
		.sethw(20 * LGGraphics::mapDataStore.mapSizeY, 50 * LGGraphics::mapDataStore.mapSizeX + 1)
		.poptext().addtext(to_string(rklst[i].city))
		.display();
		rkbut
		.setlocation(20 * (i + 1) * LGGraphics::mapDataStore.mapSizeY, 1275 * LGGraphics::mapDataStore.mapSizeX)
		.sethw(20 * LGGraphics::mapDataStore.mapSizeY, 50 * LGGraphics::mapDataStore.mapSizeX + 1)
		.poptext().addtext(to_string(rklst[i].tot))
		.display();
		rkbut
		.setlocation(20 * (i + 1) * LGGraphics::mapDataStore.mapSizeY, 1325 * LGGraphics::mapDataStore.mapSizeX)
		.sethw(20 * LGGraphics::mapDataStore.mapSizeY, 125 * LGGraphics::mapDataStore.mapSizeX + 1)
		.poptext().addtext(to_string(rklst[i].armyInHand))
		.display();
		rkbut
		.setlocation(20 * (i + 1) * LGGraphics::mapDataStore.mapSizeY, 1450 * LGGraphics::mapDataStore.mapSizeX)
		.sethw(20 * LGGraphics::mapDataStore.mapSizeY, 150 * LGGraphics::mapDataStore.mapSizeX + 1)
		.poptext().addtext(botName[LGgame::robotId[rklst[i].id]/100+1])
		.display();
	}
}

namespace LGgame {

	void init(int chtC, int pC, int sD) {
		cheatCode = chtC;
		playerCnt = pC;
		stepDelay = sD;
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
		initGenerals(LGgame::playerCoo);
		for(int i = 1; i <= LGgame::playerCnt; ++i) LGgame::playerCoo[i] = LGgame::genCoo[i];
		std::deque<int> movement;
		updateMap();
		Zip();
		zipStatus(LGgame::playerCnt);
		printMap(LGgame::cheatCode, LGgame::playerCoo[1]);
		curTurn = 0;
		bool gameEnd = 0;
		rectBUTTON fpsbut;
		fpsbut.setlocation(0, 1400 * LGGraphics::mapDataStore.mapSizeX);
		fpsbut.sethw(20 * LGGraphics::mapDataStore.mapSizeY, 200 * LGGraphics::mapDataStore.mapSizeX);
		fpsbut.setalign(CENTER_TEXT, CENTER_TEXT);
		fpsbut.setfontname("Courier New");
		fpsbut.setfonthw(20 * LGGraphics::mapDataStore.mapSizeY, 0);
		fpsbut.setbgcol(RED);
		fpsbut.settxtcol(WHITE);
		rectBUTTON turnbut;
		turnbut
		.setlocation(0, 1250 * LGGraphics::mapDataStore.mapSizeX)
		.sethw(20 * LGGraphics::mapDataStore.mapSizeY, 150 * LGGraphics::mapDataStore.mapSizeX)
		.setalign(CENTER_TEXT, CENTER_TEXT)
		.setfontname("Courier New")
		.setfonthw(20 * LGGraphics::mapDataStore.mapSizeY, 0)
		.setbgcol(BLUE)
		.settxtcol(WHITE);
		for(; is_run(); delay_fps(std::min(LGgame::stepDelay + 0.5, 120.5))) {
			while(mousemsg()) {
				mouse_msg msg = getmouse();
				if(msg.is_down() && msg.is_left() && msg.x <= widthPerBlock * mapW && msg.y <= heightPerBlock * mapH) {
					int lin = (msg.y + heightPerBlock - 1) / heightPerBlock;
					int col = (msg.x + widthPerBlock - 1) / widthPerBlock;
					LGgame::playerCoo[1] = {lin, col};
					movement.clear();
				}
			}
			while(kbmsg()) {
				key_msg ch = getkey();
				if(ch.key == key_space) {
					while((!kbmsg()) || (getkey().key != key_space)) ;
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
						int confirmSur = MessageBoxA(getHWnd(), string("ARE YOU SURE TO SURRENDER?").c_str(), "CONFIRM SURRENDER", MB_YESNO | MB_SYSTEMMODAL);
						if(confirmSur == 7)
							break;
						LGgame::isAlive[1] = 0;
						for(int i = 1; i <= mapH; ++i) {
							for(int j = 1; j <= mapW; ++j) {
								if(gameMap[i][j].team == 1) {
									gameMap[i][j].team = 0;
									if(gameMap[i][j].type == 3)
										gameMap[i][j].type = 4;
								}
							}
						}
						printGameMessage({1, 1, LGgame::curTurn});
						lastTurn[1] = playerCoord{-1, -1};
						break;
					}
				}
			}
			updateMap();
			while(!movement.empty() && analyzeMove(1, movement.front(), LGgame::playerCoo[1]))
				movement.pop_front();
			if(!movement.empty())
				movement.pop_front();
			for(int i = 2; i <= LGgame::playerCnt; ++i) {
				if(!LGgame::isAlive[i])
					continue;
				switch(LGgame::robotId[i]) {
					case 0 ... 99:
						analyzeMove(i, smartRandomBot::smartRandomBot(i, LGgame::playerCoo[i]), LGgame::playerCoo[i]);
						break;
					case 100 ... 199:
						analyzeMove(i, xrzBot::xrzBot(i, LGgame::playerCoo[i]), LGgame::playerCoo[i]);
						break;
					case 200 ... 299:
						analyzeMove(i, xiaruizeBot::xiaruizeBot(i, LGgame::playerCoo[i]), LGgame::playerCoo[i]);
						break;
					default:
						analyzeMove(i, 0, LGgame::playerCoo[i]);
				}
			}
			if(LGgame::curTurn % 2000 == 0)
				Zip(), zipStatus(LGgame::playerCnt);
			flushMove();
			if(LGgame::cheatCode != 1048575) {
				int alldead = 0;
				for(int i = 1; i <= LGgame::playerCnt && !alldead; ++i) {
					if(LGgame::cheatCode & (1 << i))
						if(LGgame::isAlive[i])
							alldead = 1;
				}
				if(!alldead) {
					LGgame::cheatCode = 1048575;
					MessageBoxA(nullptr, "ALL THE PLAYERS YOU SELECTED TO BE SEEN IS DEAD.\nTHE OVERALL CHEAT MODE WILL BE SWITCHED ON.", "TIP", MB_OK | MB_SYSTEMMODAL);
				}
			}
			if(!gameEnd) {
				int ed = 0;
				for(int i = 1; i <= LGgame::playerCnt; ++i)
					ed |= (LGgame::isAlive[i] << i);
				if(__builtin_popcount(ed) == 1) {
					MessageBoxA(nullptr,
					            ("PLAYER " + playerInfo[std::__lg(ed)].name + " WON!" + "\n" +
					             "THE GAME WILL CONTINUE." + "\n" +
					             "YOU CAN PRESS [ESC] TO EXIT.")
					            .c_str(),
					            "GAME END", MB_OK | MB_SYSTEMMODAL);
					zipGame(curTurn);
					gameEnd = 1;
					register int winnerNum = std::__lg(ed);
					LGgame::cheatCode = 1048575;
					printGameMessage({winnerNum, -1, LGgame::curTurn});
				}
			}
			printMap(LGgame::cheatCode, LGgame::playerCoo[1]);
			if(LGgame::curTurn % std::max(LGgame::stepDelay / 10, 1) == 0)
				ranklist();
			fpsbut.poptext();
			fpsbut.addtext("FPS: " + to_string(getfps()));
			fpsbut.display();
			turnbut.poptext();
			turnbut.addtext("Turn " + to_string(LGgame::curTurn) + ".");
			turnbut.display();
		}
		return 0;
	}
}

int localGame(bool isWeb, int cheatCode, int plCnt, int stDel) {
	LGgame::init(cheatCode, plCnt, stDel);
	int ret = LGlocal::GAME();
	return ret;
}

#endif // __LGGAME_HPP__
