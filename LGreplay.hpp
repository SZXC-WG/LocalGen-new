#ifndef __LGREPLAY_HPP__
#define __LGREPLAY_HPP__

#include "LGdef.hpp"

namespace LGreplay {
	char ntoc(int x) {
		switch(x) {
			case 0 ... 25:return x+65;
			case 26 ... 51:return x+71;
			case 52 ... 61:return x-4;
			case 62:return '+';
			case 63:return '/';
			default:return '_';
		}
	}
	int cton(char x) {
		switch(x) {
			case 65 ... 90:return x-65;
			case 97 ... 122:return x-71;
			case 48 ... 57:return x+4;
			case '+':return 62;
			case '/':return 63;
			default:return 0;
		}
	}
	string ntos(int x,int len) {
		string res="";
		while(x) {res+=ntoc(x&63); x>>=6;}
		if(len!=-1) while(res.size()<len) res+='A';
		len=res.size();
		for(int i=0; i<len/2; ++i) std::swap(res[i],res[len-i-1]);
		return res;
	}
	int ston(char* s,int len) {
		if(len==-1) len=strlen(s);
		int res=0;
		for(int i=0; i<len; ++i) res=res<<6|cton(s[i]);
		return res;
	}
	string zipBlock(Block B) {
		string res="";
		res+=ntoc(B.lit<<3|B.type);
		res+=ntoc(B.player<<1|(B.army<0));
		res+=ntos(std::abs(B.army),4);
		return res;
	}

	Movement::Movement() {}
	Movement::Movement(int tm,int d,playerCoord c) {player=tm; dir=d; coord=c;}
	string Movement::zip() {
		if(dir<1||dir>4) return "";
		string res="";
		res+=ntoc(player);
		res+=ntos(coord.x,2);
		res+=ntos(coord.y,2);
		res+=ntoc(dir);
		return res;
	}
	Movement readMove(char* buf) {
		Movement mov;
		mov.player=cton(buf[0]);
		mov.coord.x=ston(buf+1,2);
		mov.coord.y=ston(buf+3,2);
		mov.dir=cton(buf[5]);
		return mov;
	}

	WReplay::WReplay(string Fname) {Filename=Fname;}
	void WReplay::initReplay(string Fname) {
		Filename=Fname;
		file=fopen(Fname.c_str(),"w");
		string info="";
		info+=ntoc(LGgame::playerCnt);
		info+=ntos(mapH,2);
		info+=ntos(mapW,2);
		fprintf(file,info.c_str());
		for(int i=1; i<=mapH; ++i)
			for(int j=1; j<=mapW; ++j) fprintf(file,zipBlock(gameMap[i][j]).c_str());
	}
	void WReplay::newTurn() {fprintf(file,"_");}
	void WReplay::newMove(Movement mov) {fprintf(file,mov.zip().c_str());}

	string ts(int x) {
		string s=" ";
		while(x) {
			s=(char)(x%10+48)+s;
			x/=10;
		}
		return s;
	}
	Block readBlock(FILE* fp) {
		char* readBuf=new char[6];
		fread(readBuf,1,6,fp);
		Block B;
		B.lit=cton(readBuf[0])>>3;
		B.type=cton(readBuf[0])&7;
		int t=cton(readBuf[1]);
		B.player=t>>1;
		int nega=1; if(t&1) nega=-1;
		B.army=ston(readBuf+2,4)*nega;
		return B;
	}
	int QwQ(Movement mov) {
		playerCoord coo=mov.coord;
		int id=mov.player,mv=mov.dir;
		switch(mv) {
			case 1 ... 4: {
				playerCoord newCoo{coo.x + dx[mv], coo.y + dy[mv]};
				if(newCoo.x < 1 || newCoo.x > mapH || newCoo.y < 1 || newCoo.y > mapW || gameMap[newCoo.x][newCoo.y].type == 2)
					return 1;
				moveS insMv{id,coo,newCoo,};
				LGgame::inlineMove.push_back(insMv);
				break;
			}
			default:return -1;
		}
		return 0;
	}
	void updMap(int turn) {
		for(int i = 1; i <= mapH; ++i) {
			for(int j = 1; j <= mapW; ++j) {
				if(gameMap[i][j].player == 0) continue;
				switch(gameMap[i][j].type) {
					case 0: {
						if(turn % 25 == 0) ++gameMap[i][j].army;
						break;
					}
					case 1: {
						if(gameMap[i][j].army > 0) if(!(--gameMap[i][j].army)) gameMap[i][j].player = 0;
						break;
					}
					case 2:break;
					case 3: {
						++gameMap[i][j].army;
						break;
					}
					case 4: {
						++gameMap[i][j].army;
						break;
					}
				}
			}
		}
	}
	void replayMap::download() {
		for(int i=1; i<=mapH; ++i)
			for(int j=1; j<=mapW; ++j) rMap[i][j]=gameMap[i][j];
		for(int i=1; i<=LGgame::playerCnt; ++i) alive[i]=LGgame::isAlive[i];
	}
	void replayMap::upload() {
		for(int i=1; i<=mapH; ++i)
			for(int j=1; j<=mapW; ++j) gameMap[i][j]=rMap[i][j];
		for(int i=1; i<=LGgame::playerCnt; ++i) LGgame::isAlive[i]=alive[i];
	}
	RReplay::RReplay(string Fname) {Filename=Fname;}
	void RReplay::resetGame() {
		for(int i=1; i<=mapH; ++i)
			for(int j=1; j<=mapW; ++j) gameMap[i][j]=startMap[i][j];
		for(int i=1; i<=LGgame::playerCnt; ++i) LGgame::isAlive[i]=1;
	}
	bool RReplay::_nextTurn() {
		updMap(curTurn);
		++curTurn;
		while(1) {
			if(fread(readBuf,1,1,file)!=1) return 1;
			++seekPos;
			if(readBuf[0]=='_') break;
			fread(readBuf+1,1,5,file);
			seekPos+=5;
			QwQ(readMove(readBuf));
		}
		LGgame::flushMove();
		return 0;
	}
	int RReplay::nextTurn() {
		if(curTurn==totTurn) return 1;
		updMap(curTurn);
		++curTurn;
		fseek(file,1,SEEK_CUR);
		++seekPos;
		while(seekPos<turnPos[curTurn]) {
			fread(readBuf,1,6,file);
			seekPos+=6;
			QwQ(readMove(readBuf));
		}
		LGgame::flushMove();
		return 0;
	}
	void RReplay::gotoTurn(int turnid) {
		if(turnid<0||turnid>totTurn) return;
		// if(turnid==0) {
		// 	resetGame();
		// 	return;
		// }
		curTurn=0;
		seekPos=turnPos[curTurn];
		fseek(file,seekPos,SEEK_SET);
		resetGame();
		while(curTurn<turnid) nextTurn();
	}
	int RReplay::preTurn() {
		if(curTurn==0) return 1;
		gotoTurn(curTurn-1);
		return 0;
	}
	void RReplay::initReplay(string Fname) {
		LGgame::inReplay = true;
		Filename=Fname;
		file=fopen(Fname.c_str(),"r");
		if(!file) {
			MessageBoxA(nullptr,"No such replay file!","Local Generals",MB_OK);
			closegraph();
			exit(0);
		}
		seekPos=0;
		fseek(file,0,SEEK_SET);
		fread(readBuf,1,5,file);
		seekPos+=5;
		LGgame::playerCnt=ston(readBuf,1);
		mapH=ston(readBuf+1,2);
		mapW=ston(readBuf+3,2);
		for(int i=1; i<=mapH; ++i)
			for(int j=1; j<=mapW; ++j) gameMap[i][j]=readBlock(file),seekPos+=6;
		for(int i=1; i<=mapH; ++i)
			for(int j=1; j<=mapW; ++j) startMap[i][j]=gameMap[i][j];
		fseek(file,1,SEEK_CUR);
		++seekPos;
		totTurn=0;
		curTurn=0;
		midStates.clear();
		replayMap rmap;
		rmap.download();
		midStates.push_back(rmap);
		while(1) {
			turnPos[totTurn++]=seekPos-1;
			if(_nextTurn()) break;
			if(totTurn%100==0) {
				replayMap nrmap;
				nrmap.download();
				midStates.push_back(nrmap);
			}
		}
		replaySize=seekPos;
		seekPos=turnPos[0];
		fseek(file,turnPos[0],SEEK_SET);
		curTurn=0;
		resetGame();
	}
}

#endif
