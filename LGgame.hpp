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

// standard libraries
#include <algorithm>
#include <deque>
#include <string>
#include <chrono>
#include <random>
#include <thread>
using std::string;
// windows libraries
#include <conio.h>
// project headers
#include "LGcons.hpp"
#include "LGmaps.hpp"

int getMove0(int id,playerCoord coos[]) {
	return 0;
}
int getMove1(int id,playerCoord coos[]) {
	return 0;
}

const int dx[5] = {0,-1,0,1,0};
const int dy[5] = {0,0,-1,0,1};

struct gameStatus {
	bool isWeb;
	int cheatCode;
	int playerCnt;
	int isAlive[64];
	int stepDelay; /* ms */
	bool played;
	
	// constructor
	gameStatus() = default;
	gameStatus(bool iW,int chtC,int pC,int sD) {
		isWeb=iW; cheatCode=chtC; playerCnt=pC; stepDelay=sD;
		for(register int i=1; i<=pC; ++i) isAlive[i]=1;
		played=0;
	}
	// destructor
	~gameStatus() = default;
	
	int curTurn;
	void updateMap() {
		++curTurn;
		for(int i=1; i<=mapH; ++i) {
			for(int j=1; j<=mapW; ++j) {
				if(gameMap[i][j].team==0) continue;
				switch(gameMap[i][j].type) {
					case 0: /* plain */ {
						if(curTurn%25==0) ++gameMap[i][j].army;
						break;
					}
					case 1: /* swamp */ {
						if(gameMap[i][j].army>0) if(!(--gameMap[i][j].army)) gameMap[i][j].team=0;
						break;
					}
					case 2: /* mountain */ break; /* ??? */
					case 3: /* general */ {
						++gameMap[i][j].army;
						break;
					}
					case 4: /* city */ {
						++gameMap[i][j].army;
						break;
					}
				}
			}
		}
	}
	
	playerCoord genCoo[64];
	// general init
	void initGenerals(playerCoord coos[]) {
		std::deque<playerCoord> gens;
		for(int i=1; i<=mapH; ++i) for(int j=1; j<=mapW; ++j) if(gameMap[i][j].type==3) gens.push_back(playerCoord{i,j});
		while(gens.size()<playerCnt) {
			std::mt19937 p(std::chrono::system_clock::now().time_since_epoch().count());
			int x,y;
			do x=p()%mapH+1,y=p()%mapW+1;
			while(gameMap[x][y].type!=0);
			gens.push_back(playerCoord{x,y});
		}
		sort(gens.begin(),gens.end(),[](playerCoord a,playerCoord b){return a.x==b.x?a.y<b.y:a.x<b.x;});
		std::shuffle(gens.begin(),gens.end(),std::mt19937(std::chrono::system_clock::now().time_since_epoch().count()));
		for(int i=1; i<=playerCnt; ++i) {
			coos[i]=genCoo[i]=gens[i-1];
			gameMap[genCoo[i].x][genCoo[i].y].team=i;
		}
		for(int i=1; i<=mapH; ++i) for(int j=1; j<=mapW; ++j) 
				if(gameMap[i][j].type==3&&gameMap[i][j].team==0) gameMap[i][j].type=0;
	}
	
	void kill(int p1,int p2) {
		isAlive[p2]=0;
		for(int i=1; i<=mapH; ++i) {
			for(int j=1; j<=mapW; ++j) {
				if(gameMap[i][j].team==p2&&gameMap[i][j].type!=3) {
					gameMap[i][j].team=p1;
					gameMap[i][j].army=(gameMap[i][j].army+1)>>1;
				}
			}
		}
	}
	
	// struct for movement
	struct moveS { int id; playerCoord to; int army; };
	// vector for inline movements
	std::deque<moveS> inlineMove;
	
	// movement analyzer
	int analyzeMove(int id,int mv,playerCoord& coo) {
		switch(mv) {
			case 0: coo=genCoo[id]; break;
			case 1 ... 4: {
				playerCoord newCoo{coo.x+dx[mv],coo.y+dy[mv]};
				if(newCoo.x<1||newCoo.x>mapH||newCoo.y<1||newCoo.y>mapW||gameMap[newCoo.x][newCoo.y].type==2) return 1;
				moveS insMv{id,newCoo,0};
				if(gameMap[coo.x][coo.y].team==id)
					insMv.army=gameMap[coo.x][coo.y].army-1,gameMap[coo.x][coo.y].army=1;
				inlineMove.push_back(insMv);
				coo=newCoo;
				break;
			}
			case 5 ... 8: {
				playerCoord newCoo{coo.x+dx[mv-4],coo.y+dy[mv-4]};
				if(newCoo.x<1||newCoo.x>mapH||newCoo.y<1||newCoo.y>mapW) return 1;
				coo=newCoo;
				break;
			}
			default: return -1;
		}
		return 0;
	}
	// flush existing movements
	void flushMove() {
		while(!inlineMove.empty()) {
			moveS cur=inlineMove.front();
			inlineMove.pop_front();
			if(gameMap[cur.to.x][cur.to.y].team==cur.id) gameMap[cur.to.x][cur.to.y].army+=cur.army;
			else {
				gameMap[cur.to.x][cur.to.y].army-=cur.army;
				if(gameMap[cur.to.x][cur.to.y].army<0) {
					gameMap[cur.to.x][cur.to.y].army=-gameMap[cur.to.x][cur.to.y].army;
					int p=gameMap[cur.to.x][cur.to.y].team;
					gameMap[cur.to.x][cur.to.y].team=cur.id;
					if(gameMap[cur.to.x][cur.to.y].type==3) /* general */ {
						kill(cur.id,p);
						gameMap[cur.to.x][cur.to.y].type=4;
					}
				}
			}
		}
	}
	// main
	int operator()() {
		if(played) return -1;
		played=1;
		if(!isWeb) {
			int robotId[64];
			playerCoord coordinate[64];
			for(int i=2; i<=playerCnt; ++i) robotId[i] = std::mt19937(std::chrono::system_clock::now().time_since_epoch().count())()&1;
			initGenerals(coordinate);
			updateMap();
			printMap(cheatCode,coordinate[1]);
			std::deque<int> movement;
			curTurn=0;
			std::chrono::nanoseconds lPT = std::chrono::steady_clock::now().time_since_epoch();
			while(1) {
				if(_kbhit()) {
					int ch=_getch();
					switch(ch=tolower(ch)) {
						case int('w'): movement.emplace_back(1); break;
						case int('a'): movement.emplace_back(2); break;
						case int('s'): movement.emplace_back(3); break;
						case int('d'): movement.emplace_back(4); break;
						case 224: /**/ {
							ch=_getch();
							switch(ch) {
								case 72: /*[UP]*/    movement.emplace_back(5); break;
								case 75: /*[LEFT]*/  movement.emplace_back(6); break;
								case 80: /*[RIGHT]*/ movement.emplace_back(7); break;
								case 77: /*[DOWN]*/  movement.emplace_back(8); break;
							}
							break;
						}
						case int('g'): movement.emplace_back(0); break;
						case int('e'): if(!movement.empty()) movement.pop_back(); break;
						case int('q'): movement.clear(); break;
						case 27: MessageBox(nullptr,string("YOU QUIT THE GAME.").c_str(),"",MB_OK); return 0;
						case '\b': {
							MessageBox(nullptr,string("YOU SURRENDERED.").c_str(),"",MB_OK);
							isAlive[1]=0;
							for(int i=1; i<=mapH; ++i) {
								for(int j=1; j<=mapW; ++j) {
									if(gameMap[i][j].team==1) {
										gameMap[i][j].team=0;
										if(gameMap[i][j].type==3) gameMap[i][j].type=4;
									}
								}
							}
							cheatCode=1048575;
							break;
						}
					}
				}
				if(std::chrono::steady_clock::now().time_since_epoch()-lPT < std::chrono::milliseconds(stepDelay)) continue;
				updateMap();
				while(!movement.empty() && analyzeMove(1,movement.front(),coordinate[1])) movement.pop_front();
				if(!movement.empty()) movement.pop_front();
				for(int i=2; i<=playerCnt; ++i) {
					if(robotId[i]==0) analyzeMove(i,getMove0(i,coordinate),coordinate[i]);
					if(robotId[i]==1) analyzeMove(i,getMove1(i,coordinate),coordinate[i]);
				}
				flushMove();
				int ed=0;
				for(int i=1; i<=playerCnt; ++i) ed|=(isAlive[i]<<i-1);
				if(__builtin_popcount(ed)==1) MessageBox(nullptr,("PLAYER "+defTeams[std::__lg(ed)+1].name+" WON!"+"\n"+"THE GAME WILL CONTINUE."+"\n"+"YOU CAN PRESS [ESC] TO EXIT.").c_str(),"",MB_OK);
				gotoxy(1,1);
				printMap(cheatCode,coordinate[1]);
				lPT=std::chrono::steady_clock::now().time_since_epoch();
			}
		}
		return 0;
	}
};

int GAME(bool isWeb,int cheatCode,int plCnt,int stDel) {
	hideCursor();
	gameStatus newGame = gameStatus(isWeb,cheatCode,plCnt,stDel);
	return newGame();
}

#endif // __LGGAME_HPP__

