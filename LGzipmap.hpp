/* This is LGzipmap.hpp file of LocalGen.                                */
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

#ifndef __LGZIPMAP_HPP__
#define __LGZIPMAP_HPP__

#include <queue>
#include "LGmaps.hpp"
#include "LGgame.hpp"

const int LEN_ZIP = 100005, CHAR_AD = 48, LEN_MOVE = 30005,replaySorter=2000;
char strdeZip[LEN_ZIP];
char strZip[LEN_ZIP];
char strGameZip[4 * LEN_ZIP];
char strdeGameZip[4 * LEN_ZIP];
Block curMap[505][505];
playerCoord mapCoord[17][30],curCoord[30];
Block mapSet[17][505][505];

long long totTurn,curTurn,totMove;
std::pair<long long,long long> curMoveS;
std::queue<long long> signMap;
std::queue<long long> signCmd;
struct movementS {
	int id, op;
	long long turn;
	void clear() {
		id=turn=op=0;
	}
};
std::queue<movementS> movementPack;
movementS dezipedMovementS[4*LEN_ZIP];
movementS tmp;

inline long long PMod(long long& x) {
	long long res = x & 63;
	x >>= 6;
	return res;
}

void trans(int st,int en) {
	for(int i=st,cur=0; i<en; i++,cur++)
		strdeZip[cur]=strdeGameZip[i];
}

void retrans(int cur) {
	for(int i=1; i<=mapH; i++) {
		for(int j=1; j<=mapW; j++)
			mapSet[cur][i][j]=gameMap[i][j];
	}
}

std::pair<long long,long long> bin_search(long long curTurn) {
	long long l=1,r=totMove;
	std::pair<long long,long long> ans=std::make_pair(0ll,0ll);
	while(l<=r) {
		long long mid=l+r>>1;
		if(dezipedMovementS[mid].turn==curTurn) ans.first=mid;
		if(dezipedMovementS[mid].turn>=curTurn) r=mid-1;
		else l=mid+1;
	}
	l=1,r=totMove;
	while(l<=r) {
		long long mid=l+r>>1;
		if(dezipedMovementS[mid].turn==curTurn+1) ans.second=mid;
		if(dezipedMovementS[mid].turn>=curTurn+1) r=mid-1;
		else l=mid+1;
	}
	return ans;
}

void Zip() {
	register int p = 0, i, j;
	long long k1 = mapH, k2 = mapW;
	strZip[p++] = 44;
	strZip[p++] = PMod(k1) + CHAR_AD;
	strZip[p++] = PMod(k1) + CHAR_AD;
	strZip[p++] = PMod(k2) + CHAR_AD;
	strZip[p++] = PMod(k2) + CHAR_AD;

	for(i = 1; i <= mapH; i++)
		for(j = 1; j <= mapW; j++, p++) {
			strZip[p++] = gameMap[i][j].team + CHAR_AD;
			strZip[p] = (gameMap[i][j].type << 2) + (gameMap[i][j].lit << 1);
			k1 = gameMap[i][j].army;

			if(k1 < 0) {
				k1 = -k1;
				strZip[p++] += CHAR_AD + 1;
			} else
				strZip[p++] += CHAR_AD;

			for(k2 = 1; k2 <= 11; k2++)
				strZip[p++] = PMod(k1) + CHAR_AD;
		}
	strZip[p] = '\0';
}

void deZip() {
	register int i, j, k = 4;
	int f, p = 0;

	for(; strdeZip[p] != '\0'; p++)
		strdeZip[p] -= CHAR_AD;

	mapH = (strdeZip[1] << 6) + strdeZip[0];
	mapW = (strdeZip[3] << 6) + strdeZip[2];

	for(i = 1; i <= mapH; i++)
		for(j = 1; j <= mapW; j++) {
			gameMap[i][j].team = strdeZip[k++];
			bool f = strdeZip[k] & 1;
			strdeZip[k] >>= 1;
			gameMap[i][j].lit = strdeZip[k] & 1;
			strdeZip[k] >>= 1;
			gameMap[i][j].type = strdeZip[k++];
			gameMap[i][j].army = 0;

			for(p = 10; p >= 0; p--)
				gameMap[i][j].army = (gameMap[i][j].army << 6) + strdeZip[k + p];
			k += 11;
			gameMap[i][j].army = f ? (-gameMap[i][j].army) : gameMap[i][j].army;
		}
}

void zipGame(long long totTurn) {
	int p = 0, curTurn = 0;

	strGameZip[p++] = 45;
	strGameZip[p++] = PMod(totTurn) + CHAR_AD;
	strGameZip[p++] = PMod(totTurn) + CHAR_AD;

	while(!movementPack.empty()) {
		if(movementPack.front().turn != curTurn) {
			for(int i = curTurn + 1; i <= movementPack.front().turn; i++)
				strGameZip[p++] = 46;
			curTurn = movementPack.front().turn;
		}
		if(movementPack.front().op==-1) goto here;
		strGameZip[p++] = movementPack.front().id + CHAR_AD;
		strGameZip[p++] = movementPack.front().op + CHAR_AD;
	here:;
		movementPack.pop();
	}

	strGameZip[p++] = 47;
	strGameZip[p] = '\0';
}

void mapReset(int c) {
	for(int i=1; i<=mapH; i++) {
		for(int j=1; j<=mapW; j++) curMap[i][j]=mapSet[c][i][j];
	}
}

void deZipGame() {
	long long cur=0,p=0;
	long long beg,fin;

	while(strdeGameZip[p]!='\0') {
		if(strdeGameZip[p]==44||strdeGameZip[p]==45)
			signMap.push(p);
		p++;
	}

	beg=signMap.front();
	signMap.pop();
	fin=signMap.front();
	signMap.pop();

	while(1) {
		trans(beg,fin);
		deZip();
		retrans(++cur);
		if(signMap.empty()) break;
		beg=fin;
		fin=signMap.front();
		signMap.pop();
	}

	mapReset(1);

	p=0;
	while(strdeGameZip[p]!=45) p++;

	totTurn=(strdeGameZip[p+2]<<6)+strdeGameZip[p+1];
	p=p+3;

	while(strdeGameZip[p]!='\0') {
		if(strdeGameZip[p]==46||strdeGameZip[p]==47)
			signCmd.push(p);
		p++;
	}

	beg=signCmd.front();
	signCmd.pop();
	fin=signCmd.front();
	signCmd.pop();

	while(1) {
		if(fin-beg!=1) {
			++curTurn;
			for(int i=beg,j=1; i<fin; i++,j++) {
				if(j&1)
					tmp.id=strdeGameZip[i]-CHAR_AD;
				else {
					tmp.op=strdeGameZip[i]-CHAR_AD;
					tmp.turn=curTurn;
					dezipedMovementS[++totMove]=tmp;
					tmp.clear();
				}
			}
		}
		if(signCmd.empty()) break;
		beg=fin;
		fin=signCmd.front();
		signCmd.pop();
	}
}

void initializeReplay(gameStatus curStatus) {
	for(int i=1; i<=mapH; i++) {
		for(int j=1; j<=mapW; j++) {
			if(curMap[i][j].type==3) {
				mapCoord[1][curMap[i][j].team].x=curCoord[curMap[i][j].team].x=i;
				mapCoord[1][curMap[i][j].team].y=curCoord[curMap[i][j].team].y=j;
			}
		}
	}
	for(int i=1; i<=totMove; i++) {
		curStatus.analyzeMove(dezipedMovementS[i].id,dezipedMovementS[i].op,curCoord[dezipedMovementS[i].id]);
		if(dezipedMovementS[i].turn%replaySorter==0) {
			mapCoord[dezipedMovementS[i].turn/replaySorter+1][dezipedMovementS[i].id].x=curCoord[dezipedMovementS[i].id].x;
			mapCoord[dezipedMovementS[i].turn/replaySorter+1][dezipedMovementS[i].id].y=curCoord[dezipedMovementS[i].id].y;
		}
	}
}

void Replay(int dir,long long curTurn,gameStatus curStatus) {
	if(dir) {
		curMoveS=bin_search(curTurn+1);
		for(int i=curMoveS.first; i<curMoveS.second; i++)
			curStatus.analyzeMove(dezipedMovementS[i].id,dezipedMovementS[i].op,curCoord[dezipedMovementS[i].id]);
	} else {
		for(int i=1; i<=20; i++)
			curCoord[i]=mapCoord[curTurn/replaySorter+1][i];
		curMoveS=bin_search(curTurn/replaySorter*replaySorter);
		while(dezipedMovementS[curMoveS.first].turn<curTurn) {
			curStatus.analyzeMove(dezipedMovementS[curMoveS.first].id,dezipedMovementS[curMoveS.first].op,curCoord[dezipedMovementS[curMoveS.first].id]);
			curMoveS.first++;
		}
	}
}

#endif
