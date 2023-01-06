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
#include <string>
#include <chrono>
#include <random>
using std::string;
using std::mt19937;
// windows libraries
#include <conio.h>
// project headers
#include "LGmaps.hpp"

struct playerCoord { int x,y; };

struct gameStatus {
	bool isWeb;
	bool cheat;
	int playerCnt;
	int isAlive[64];
	int stepDelay;
	bool played;
	// constructor
	gameStatus() = default;
	gameStatus(bool iW,bool cht,int pC,int sD) {
		isWeb=iW; cheat=cht; playerCnt=pC; stepDelay=sD;
		for(register int i=1; i<=pC; ++i) isAlive[i]=1;
		played=0;
	}
	// destructor
	~gameStatus() = default;
	// move analyzer
	int analyzeMove(int mv,playerCoord& coo) {
		return 0;
	}
	// main
	int operator() () {
		if(played) return -1;
		played=1;
		if(!isWeb) {
			printMap(cheat);
			int robotId[64];
			playerCoord coordinate[64];
			for(int i=2; i<=playerCnt; ++i) robotId = mt19937(std::chrono::system_clock::now()::time_since_epoch().count())()&1;
			initGenerals(coordinate);
			deque<int> movement;
			while(1) {
				int ch=_getch();
				switch(ch=tolower(ch)) {
					case int('w'): movement.emplace_back(1); break;
					case int('a'): movement.emplace_back(2); break;
					case int('s'): movement.emplace_back(3); break;
					case int('d'): movement.emplace_back(4); break;
					case 224: /**/ {
						ch=_getch();
						switch(ch) {
							case 72: /*[UP]*/    movement.emplace_back(1); break;
							case 75: /*[LEFT]*/  movement.emplace_back(2); break;
							case 77: /*[RIGHT]*/ movement.emplace_back(3); break;
							case 80: /*[DOWN]*/  movement.emplace_back(4); break;
						}
						break;
					}
					case int('h'): movement.emplace_back(0); break;
					case int('e'): if(!movement.empty()) movement.pop_back(); break;
					case int('q'): movement.clear(); break;
					case 27: 
				}
				while(analyzeMove(movement.front(),coordinate[1])) movement.pop_front();
				movement.pop_front();
				for(int i=2; i<=playerCnt; ++i) {
					if(robotId[i]==0) analyzeMove(getMove0(i,coordinate[i]),coordinate[i]);
					if(robotId[i]==1) analyzeMove(getMove1(i,coordinate[i]),coordinate[i]);
				}
				printMap(cheat);
			}
		}
	}
};

void GAME(bool isWeb,bool cheat,int plCnt,int stDel) {
	gameStatus newGame = gameStatus(isWeb,cheat,plCnt,stDel);
	newGame();
}

#endif // __LGGAME_HPP__

