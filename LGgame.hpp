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
#include <functional>
#include <algorithm>
#include <deque>
#include <vector>
#include <string>
#include <chrono>
#include <random>
#include <thread>
using std::string;
using namespace std::literals;
// windows libraries
#include <windows.h>
#include <conio.h>
// project headers
#include "LGcons.hpp"
#include "LGmaps.hpp"
// Robot
#include "xrzBot.hpp"
#include "xiaruizeBot.hpp"

const int dx[5] = {0,-1,0,1,0};
const int dy[5] = {0,0,-1,0,1};

struct passS { int id,turn; };
std::vector<passS> passId[505][505];
playerCoord lastTurn[20];

int smartRandomBot(int id,playerCoord coo) {
	static std::mt19937 mtrd(std::chrono::system_clock::now().time_since_epoch().count());
	if(gameMap[coo.x][coo.y].team!=id||gameMap[coo.x][coo.y].army==0) return 0;
	struct node { int type,team; long long army; int dir; };
	node p[5]; int pl=0;
	for(int i=1; i<=4; ++i) {
		if(coo.x+dx[i]<1||coo.x+dx[i]>mapH||coo.y+dy[i]<1||coo.y+dy[i]>mapW||gameMap[coo.x+dx[i]][coo.y+dy[i]].type==2) continue;
		p[++pl]={gameMap[coo.x+dx[i]][coo.y+dy[i]].type,gameMap[coo.x+dx[i]][coo.y+dy[i]].team,gameMap[coo.x+dx[i]][coo.y+dy[i]].army,i};
	}
	bool rdret=mtrd()%2;
	auto cmp = [&](node a,node b)->bool {
		if(a.type==3&&a.team!=id) return true;
		if(b.type==3&&b.team!=id) return false;
		if(a.team==0) return rdret;
		if(b.team==0) return !rdret;
		if(a.team==id&&b.team!=id) return false;
		if(a.team!=id&&b.team==id) return true;
		if(a.team==id&&b.team==id) return a.army>b.army;
		return a.army<b.army;
	};
	std::sort(p+1,p+pl+1,cmp);
	return p[1].dir;
}
int ktqBot(int id,playerCoord coo){
	static std::mt19937 mtrd(std::chrono::system_clock::now().time_since_epoch().count());
	using ll = long long;
	static int swampDir[20]={3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3};
	if(gameMap[coo.x][coo.y].team!=id||gameMap[coo.x][coo.y].army==0) return 0;
	struct node{
		int to,team;
		ll army,del;
		int type;
		bool operator<(node b){
			return army<b.army||(army==b.army&&del<b.del);
		}
	};
	node p[10];
	int cnt=0;
	for(int i=1;i<=4;i++){
		int tx=coo.x+dx[i],ty=coo.y+dy[i];
		if(gameMap[tx][ty].type==2||tx<1||tx>mapH||ty<1||ty>mapW)continue;
		p[++cnt]={i,gameMap[tx][ty].team,gameMap[tx][ty].army,gameMap[tx][ty].army,gameMap[tx][ty].type};
		if(p[cnt].type!=1&&p[cnt].team==id) p[cnt].army=-p[cnt].army,p[cnt].del=-p[cnt].del;
		if(p[cnt].type==4&&p[cnt].team!=id) p[cnt].army=2*p[cnt].army-ll(1e15);
		else if(p[cnt].type==0&&p[cnt].team!=id) p[cnt].army=p[cnt].army-ll(1e15);
		else if(p[cnt].type==1) { p[cnt].del=200; p[cnt].army=-1e16; }
		else if(p[cnt].type==3&&p[cnt].team!=id) p[cnt].army=-ll(1e18);
	}
	std::sort(p+1,p+cnt+1);
//	gotoxy(mapH+2+16+1+id,1); clearline();
//	fputs(defTeams[id].name.c_str(),stdout);
//	printf(": ");
//	for(int i=1; i<=cnt; ++i) printf("{%d %d %lld %lld %d} ",p[i].to,p[i].team,p[i].army,p[i].del,p[i].type);
//	fflush(stdout); _getch();
	for(int i=1;i<=cnt;i++) {
		if(p[i].del<gameMap[coo.x][coo.y].army) return p[i].to;
	}
	return -1;
}

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
			gameMap[x][y].type=3;
			gameMap[x][y].army=0;
		}
		sort(gens.begin(),gens.end(),[](playerCoord a,playerCoord b){return a.x==b.x?a.y<b.y:a.x<b.x;});
		std::shuffle(gens.begin(),gens.end(),std::mt19937(std::chrono::system_clock::now().time_since_epoch().count()));
		for(int i=1; i<=playerCnt; ++i) {
			coos[i]=genCoo[i]=gens[i-1];
			gameMap[genCoo[i].x][genCoo[i].y].team=i;
			gameMap[genCoo[i].x][genCoo[i].y].army=0;
		}
		for(int i=1; i<=mapH; ++i) for(int j=1; j<=mapW; ++j) 
				if(gameMap[i][j].type==3&&gameMap[i][j].team==0) gameMap[i][j].type=0;
	}
	
	int gameMesC=0;
	void kill(int p1,int p2) {
		if(p2==1) {
			cheatCode=1048575;
			MessageBox(nullptr,string("YOU ARE KILLED BY PLAYER "+defTeams[p1].name+" AT TURN "+to_string(curTurn)+".").c_str(),"",MB_OK);
		}
		isAlive[p2]=0;
		for(int i=1; i<=mapH; ++i) {
			for(int j=1; j<=mapW; ++j) {
				if(gameMap[i][j].team==p2&&gameMap[i][j].type!=3) {
					gameMap[i][j].team=p1;
					gameMap[i][j].army=(gameMap[i][j].army+1)>>1;
				}
			}
		}
		++gameMesC;
		gotoxy(mapH+2+gameMesC,65);
		setfcolor(0xffffff);
		fputs("PLAYER ",stdout);
		setfcolor(defTeams[p1].color);
		printf("%-7s",defTeams[p1].name.c_str());
		setfcolor(0xffffff);
		fputs(" KILLED PLAYER ",stdout);
		setfcolor(defTeams[p2].color);
		printf("%-7s",defTeams[p2].name.c_str());
		setfcolor(0xffffff);
		printf(" AT TURN %d.",curTurn);
		fflush(stdout);
	}
	
	// struct for movement
	struct moveS { int id; playerCoord to; long long army; };
	// vector for inline movements
	std::deque<moveS> inlineMove;
	
	// movement analyzer
	int analyzeMove(int id,int mv,playerCoord& coo) {
		switch(mv) {
			case -1: break;
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
			if(!isAlive[cur.id]) continue;
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
						for(auto& mv:inlineMove) if(mv.id==p) mv.id=cur.id,mv.army=(mv.army+1)>>1;
					}
				}
			}
		}
	}
	
	// ranklist printings
	void ranklist(playerCoord coos[]) {
		struct node {
			int id;
			long long army;
			int plain,city,tot;
			long long armyInHand; 
		} rklst[64];
		for(int i=1; i<=playerCnt; ++i) {
			rklst[i].id=i;
			rklst[i].army=rklst[i].armyInHand=0;
			rklst[i].plain=rklst[i].city=rklst[i].tot=0;
		}
		for(int i=1; i<=mapH; ++i) {
			for(int j=1; j<=mapW; ++j) {
				if(gameMap[i][j].team==0) continue; 
				if(gameMap[i][j].type==2) continue;
				++rklst[gameMap[i][j].team].tot;
				if(gameMap[i][j].type==0) ++rklst[gameMap[i][j].team].plain;
				else if(gameMap[i][j].type==4) ++rklst[gameMap[i][j].team].city;
				else if(gameMap[i][j].type==3) ++rklst[gameMap[i][j].team].city;
				rklst[gameMap[i][j].team].army+=gameMap[i][j].army;
			}
		}
		for(int i=1; i<=playerCnt; ++i) {
			if(gameMap[coos[i].x][coos[i].y].team!=i) continue;
			rklst[i].armyInHand=gameMap[coos[i].x][coos[i].y].army;
		}
		std::sort(rklst+1,rklst+playerCnt+1,[](node a,node b){return a.army>b.army;});
		setfcolor(0xffffff); underline();
		printf("| %7s | %8s | %5s | %5s | %5s | %13s |","PLAYER","ARMY","PLAIN","CITY","TOT","ARMY IN HAND");
		resetattr(); setfcolor(0x000000); putchar('|'); putchar('\n');
		for(int i=1; i<=playerCnt; ++i) {
			if(isAlive[rklst[i].id]) setfcolor(defTeams[rklst[i].id].color);
			else setfcolor(defTeams[10].color);
			underline();
			printf("| %7s | ",defTeams[rklst[i].id].name.c_str());
			if(rklst[i].army<100000000) printf("%8lld | ",rklst[i].army);
			else {
				register int p=std::to_string(rklst[i].army*1.0L/1e9L).find('.');
				printf("%*.*LfG | ",7,7-1-p,rklst[i].army*1.0L/1e9L);
			}
			printf("%5d | %5d | %5d | %13lld |",rklst[i].plain,rklst[i].city,rklst[i].tot,rklst[i].armyInHand);
			resetattr(); /*printf("%d",rklst[i].id);*/setfcolor(0x000000); putchar('|'); putchar('\n');
		}
		resetattr();
		setfcolor(0xffffff); 
		fflush(stdout);
	}
	
	// main
	int operator()() {
		if(played) return -1;
		played=1;
		if(!isWeb) {
			int robotId[64];
			playerCoord coordinate[64];
			std::mt19937 mtrd(std::chrono::system_clock::now().time_since_epoch().count());
			for(int i=2; i<=playerCnt/2+1; ++i) robotId[i] = 1;
			for(int i=playerCnt/2+2; i<=playerCnt; ++i) robotId[i] = 51;// for robot debug
			initGenerals(coordinate);
			updateMap();
			printMap(cheatCode,coordinate[1]);
			std::deque<int> movement;
			curTurn=0;
			bool gameEnd=0;
			std::chrono::nanoseconds lPT = std::chrono::steady_clock::now().time_since_epoch();
			while(1) {
				if(_kbhit()) {
					int ch=_getch();
					switch(ch=tolower(ch)) {
						case int(' '): while(_getch()!=' '); break;
						case int('c'): clearance(); break;
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
						case int('\b'): {
							if(!isAlive[1]) break;
							int confirmSur=MessageBox(nullptr,string("ARE YOU SURE TO SURRENDER?").c_str(),"SURRENDER",MB_YESNO);
							if(confirmSur==7) break;
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
							++gameMesC;
							gotoxy(mapH+2+gameMesC,65);
							setfcolor(0xffffff);
							fputs("PLAYER ",stdout);
							setfcolor(defTeams[1].color);
							printf("%-7s",defTeams[1].name.c_str());
							setfcolor(0xffffff);
							printf(" SURRENDERED AT TURN %d.",curTurn);
							fflush(stdout);
							break;
						}
					}
				}
				if(std::chrono::steady_clock::now().time_since_epoch()-lPT < std::chrono::milliseconds(stepDelay)) continue;
				updateMap();
				while(!movement.empty() && analyzeMove(1,movement.front(),coordinate[1])) movement.pop_front();
				if(!movement.empty()) movement.pop_front();
				for(int i=2; i<=playerCnt; ++i) {
					if(!isAlive[i]) continue;
					switch(robotId[i]) {
						case 1: analyzeMove(i,xiaruizeBot::xiaruizeBot(i,coordinate[i]),coordinate[i]); break;
						case 51: analyzeMove(i,xrzBot::xrzBot(i,coordinate[i]),coordinate[i]); break;
						default: analyzeMove(i,0,coordinate[i]);
					}
				}
				flushMove();
				if(!gameEnd) {
					int ed=0;
					for(int i=1; i<=playerCnt; ++i) ed|=(isAlive[i]<<i);
					if(__builtin_popcount(ed)==1) {
						MessageBox(nullptr,
						           ("PLAYER "+defTeams[std::__lg(ed)].name+" WON!"+"\n"+
						            "THE GAME WILL CONTINUE."+"\n"+
									"YOU CAN PRESS [ESC] TO EXIT.").c_str(),
								   "",MB_OK);
						gameEnd=1;
						cheatCode=1048575;
						++gameMesC;
						gotoxy(mapH+2+gameMesC,65);
						setfcolor(0xffffff);
						fputs("PLAYER ",stdout);
						setfcolor(defTeams[std::__lg(ed)].color);
						printf("%-7s",defTeams[std::__lg(ed)].name.c_str());
						setfcolor(0xffffff);
						printf(" WON AT TURN %d!!!",curTurn);
						fflush(stdout);
					}
				}
				gotoxy(1,1);
				printMap(cheatCode,coordinate[1]);
				ranklist(coordinate);
				lPT=std::chrono::steady_clock::now().time_since_epoch();
			}
		}
		return 0;
	}
};

int GAME(bool isWeb,int cheatCode,int plCnt,int stDel) {
	setvbuf(stdout,nullptr,_IOFBF,5000000);
	hideCursor();
	clearance();
	gotoxy(1,1);
	int ret = gameStatus(isWeb,cheatCode,plCnt,stDel)();
	setvbuf(stdout,nullptr,_IONBF,0);
	return ret;
}

#endif // __LGGAME_HPP__

