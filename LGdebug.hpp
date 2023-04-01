/* This is LGprint.hpp file of LocalGen.                                  */
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

// clear the full window: too slow, don't use!

#include<conio.h>

namespace LGdebug{
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	
	inline void clearance() {
		HANDLE hdout = GetStdHandle(STD_OUTPUT_HANDLE);
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		GetConsoleScreenBufferInfo(hdout, &csbi);
		DWORD size = csbi.dwSize.X * csbi.dwSize.Y, num = 0;
		COORD pos = {0, 0};
		FillConsoleOutputCharacter(hdout, ' ', size, pos, &num);
		FillConsoleOutputAttribute(hdout, csbi.wAttributes, size, pos, &num);
		SetConsoleCursorPosition(hdout, pos);
	}
	// clear the line (only the chars after the cursor)
	inline void clearline() { fputs("\033[K", stdout); }
	
	// get the cursor's position
	void getxy(int& x, int& y) {
		CONSOLE_SCREEN_BUFFER_INFO bInfo;
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &bInfo);
		y = bInfo.dwCursorPosition.X;
		x = bInfo.dwCursorPosition.Y;
	}
	// make the cursor go to a specticular place in the screen
	inline void gotoxy(int x, int y) { printf("\033[%d;%dH", x, y); }
	// make the cursor move n lines up
	inline void curup(int c = 1) { printf("\033[%dA", c); }
	// make the cursor move n lines down
	inline void curdown(int c = 1) { printf("\033[%dB", c); }
	// make the cursor move n chars right
	inline void curright(int c = 1) { printf("\033[%dC", c); }
	// make the cursor move n chars left
	inline void curleft(int c = 1) { printf("\033[%dD", c); }
	
	// hide cursor
	inline void hideCursor() { fputs("\033[?25l", stdout); }
	// show cursor
	inline void showCursor() { fputs("\033[?25h", stdout); }
	
	// attr init: without this will lead to errors
	inline int initattr() {
		HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
		if(hOut == INVALID_HANDLE_VALUE)
			return GetLastError();
		DWORD dwMode = 0;
		if(!GetConsoleMode(hOut, &dwMode))
			return GetLastError();
		dwMode |= 0x0004;
		if(!SetConsoleMode(hOut, dwMode))
			return GetLastError();
		return 0;
	}
	
	inline void Color(int x){
		SetConsoleTextAttribute(hOut, x);
	}
	
	// foreground color
	inline void setfcolor(int RGB) { printf("\033[38;2;%d;%d;%dm", RGB / 65536, RGB / 256 % 256, RGB % 256); }
	inline void setfcolor(int R, int G, int B) { printf("\033[38;2;%d;%d;%dm", R, G, B); }
	// background color
	inline void setbcolor(int RGB) { printf("\033[48;2;%d;%d;%dm", RGB / 65536, RGB / 256 % 256, RGB % 256); }
	inline void setbcolor(int R, int G, int B) { printf("\033[48;2;%d;%d;%dm", R, G, B); }
	
	// print chars with underline
	inline void underline() { fputs("\033[4m", stdout); }
	
	// reset text attributes
	inline void resetattr() { fputs("\033[0m\033[38;2;255;255;255m", stdout); }
	
	struct Picxel{
		int colorFront, colorBack;
		char Chr;
	
		bool operator==(Picxel x)
		{
			return colorFront == x.colorFront && colorBack == x.colorBack && Chr == x.Chr;
		}
	};
	
	char numS[15];
	Picxel nptMap[505][505], ptMap[505][505];
	int ptH, ptW;
	int COLOR[20]={7,1,2,3,4,5,6,8,9,10,11,12,13,14,15};
	
	void initMap(){
		ptH = mapH;
		ptW = mapW * 5;
	}
	
	void printFrame(){
		gotoxy(1, 2);
		for (int i = 1; i <= ptW; i++) printf("-");
		gotoxy(ptH + 2, 2);
		for (int i = 1; i <= ptW; i++) printf("-");
		for (int i = 1; i <= ptH; i++){
			gotoxy(i+1, 1);
			printf("|");
			gotoxy(i+1, ptW + 2);
			printf("|");
		}
	
		gotoxy(1, ptW + 2);
		printf("+");
		gotoxy(1, 1);
		printf("+");
		gotoxy(ptH + 2, 1);
		printf("+");
		gotoxy(ptH + 2, ptW + 2);
		printf("+");
	}
	
	inline void fillCf(int x1, int y1, int x2, int y2, int C){
		register int i, j;
		for (i = x1; i <= x2; i++)
			for (j = y1; j <= y2; j++)
				nptMap[i][j].colorFront = C;
	}
	inline void fillCb(int x1, int y1, int x2, int y2, int C){
		register int i, j;
		for (i = x1; i <= x2; i++)
			for (j = y1; j <= y2; j++)
				nptMap[i][j].colorBack = C;
	}
	inline void fillCh(int x1, int y1, int x2, int y2, char c){
		register int i, j;
		for (i = x1; i <= x2; i++)
			for (j = y1; j <= y2; j++)
				nptMap[i][j].Chr = c;
	}
	inline void fillNs(int x, int y1, int y2){
		for (register int i = y1; i <= y2; i++)
			nptMap[x][i].Chr = numS[i - y1];
	}
	
	inline void turnStr(long long x){
		for (int i = 0; i < 10; i++)
			numS[i] = ' ';
		int f = 0, tw, gw;
	
		if (x < 0)
		{
			x = -x;
			numS[0] = '-';
	
			while (x / 100 != 0)
			{
				x /= 10;
				f++;
			}
	
			numS[0] = '-';
			tw = x / 10;
			gw = x % 10;
	
			if (tw == 0)
				numS[1] = gw + 48;
			else
			{
				numS[1] = tw + 48;
				if (f == 0)
					numS[2] = gw + 48;
				else if (f < 10)
					numS[2] = NUM_s[f];
				else
					numS[2] = 'T';
			}
		}
		else
		{
			while (x / 1000 != 0)
			{
				x /= 10;
				f++;
			}
	
			gw = x % 10;
			x /= 10;
			tw = x % 10;
			x /= 10;
	
			if (x != 0)
				numS[0] = x + 48;
			if (tw != 0 || x != 0)
				numS[1] = tw + 48;
	
			if (f == 0)
				numS[2] = gw + 48;
			else if (f < 10)
				numS[2] = NUM_s[f];
			else
				numS[2] = 'T';
		}
	}
	
	inline bool printCheck(int x, int y, int printCode){
		if (printCode & (1 << gameMap[x][y].team))
			return true;
		if (x - 1 > 0 && (printCode & (1 << gameMap[x - 1][y].team)))
			return true;
		if (x + 1 <= mapH && (printCode & (1 << gameMap[x + 1][y].team)))
			return true;
		if (y - 1 > 0 && (printCode & (1 << gameMap[x][y - 1].team)))
			return true;
		if (y + 1 <= mapW && (printCode & (1 << gameMap[x][y + 1].team)))
			return true;
		return false;
	}
	
	void writeMap(int PTC){
		register int i, j;
		for (i = 1; i <= mapH; i++)
		for (j = 1; j <= mapW; j++)
		if (printCheck(i, j, PTC)||gameMap[i][j].lit){
			switch (gameMap[i][j].type){
				case 0:
					fillCh(i, j * 5 - 4, i, j * 5, ' ');
					break;
				case 1:
					fillCh(i, j * 5 - 4, i, j * 5, '=');
					break;
				case 2:
					fillCh(i, j * 5 - 4, i, j * 5, '#');
					break;
				case 3:
					nptMap[i][j * 5 - 4].Chr = '$';
					nptMap[i][j * 5].Chr = '$';
					fillCh(i, j * 5 - 3, i, j * 5 - 1, ' ');
					break;
				case 4:
					nptMap[i][j * 5 - 4].Chr = '[';
					nptMap[i][j * 5].Chr = ']';
					fillCh(i, j * 5 - 3, i, j * 5 - 1, ' ');
					break;
				default:
					break;
			}if (gameMap[i][j].army != 0 && gameMap[i][j].type != 2){
				turnStr(gameMap[i][j].army);
				fillNs(i, j * 5 - 3, j * 5 - 1);
			}else gameMap[i][j].army = 0;
			
			fillCf(i, j * 5 - 4, i, j * 5, COLOR[gameMap[i][j].team]);
		}else{
			switch (gameMap[i][j].type){
				case 0:
				case 3:
					fillCh(i, j * 5 - 4, i, j * 5, ' ');
					break;
				case 1:
					fillCh(i, j * 5 - 4, i, j * 5, '=');
					break;
				case 2:
				case 4:
					fillCh(i, j * 5 - 4, i, j * 5, '#');
					break;
				default:
					break;
			}
		}
	}
	
	void printMap(int PTC){
		writeMap(PTC);
		Color(7);
		register int i, j, ncolorFront = 7, ncolorBack = 0;
		
		for (i = 1; i <= ptH; i++)
		for (j = 1; j <= ptW; j++)
		if (!(nptMap[i][j] == ptMap[i][j])){
			if (nptMap[i][j].colorFront != ncolorFront || nptMap[i][j].colorBack != ncolorBack)
			Color((ncolorFront = nptMap[i][j].colorFront)+(ncolorBack = nptMap[i][j].colorBack));
			
			gotoxy(i+1, j+1);
			putchar(nptMap[i][j].Chr);
			ptMap[i][j] = nptMap[i][j];
		}
	}
	
	int debugBegin(){
		initattr();
		int mapNum,plCnt,stDel,PTC;
		
		puts("DEBUG! DEBUG! DEBUG!");
		scanf("%d%d%d%d",&mapNum,&plCnt,&stDel,&PTC);
		
		if(mapNum < 6) {
			int ht,wt;
			scanf("%d%d",&ht,&wt);
			
			switch(mapNum) {
				case 1: createRandomMap(ht,wt); break;
				case 2: createStandardMap(ht,wt); break;
				case 3: puts("BUG!");return 1;
				case 4: createFullSwampMap(ht,wt,plCnt); break;
				case 5: createFullPlainMap(ht,wt,plCnt); break;
			}
		} else readMap(mapNum);
		
		initMap();
		clearance();
		int robotId[64];
		playerCoord coordinate[64];
		std::mt19937 mtrd(std::chrono::system_clock::now().time_since_epoch().count());
		for(int i = 2; i <= plCnt; ++i)
			robotId[i] = mtrd() % 300 + 1;
//			for(int i=2; i<=playerCnt/2+1; ++i) robotId[i] = 1;
//			for(int i=playerCnt/2+2; i<=playerCnt; ++i) robotId[i] = 51; // for robot debug
		LGgame::initGenerals(coordinate);
		LGgame::updateMap();
		printMap(PTC);
		std::deque<int> movement;
		LGgame::curTurn = 0;
		bool gameEnd = 0;
		std::chrono::nanoseconds lPT = std::chrono::steady_clock::now().time_since_epoch();
		
		while(1) {gotoxy(20,1);puts("fuck?");
			if(kbhit()) {
//				int ch = getch();
//				switch(ch = tolower(ch)) {
//					case int(' '):
//						while(getch() != ' ')
//							;
//						break;
//					case int('c'):
//						clearance();
//						break;
//					case int('w'):
//						movement.emplace_back(1);
//						break;
//					case int('a'):
//						movement.emplace_back(2);
//						break;
//					case int('s'):
//						movement.emplace_back(3);
//						break;
//					case int('d'):
//						movement.emplace_back(4);
//						break;
//					case 224: { /**/
//						ch = getch();
//						switch(ch) {
//							case 72: /*[UP]*/
//								movement.emplace_back(5);
//								break;
//							case 75: /*[LEFT]*/
//								movement.emplace_back(6);
//								break;
//							case 80: /*[RIGHT]*/
//								movement.emplace_back(7);
//								break;
//							case 77: /*[DOWN]*/
//								movement.emplace_back(8);
//								break;
//						}
//						break;
//					}
//					case int('g'):
//						movement.emplace_back(0);
//						break;
//					case int('e'):
//						if(!movement.empty())
//							movement.pop_back();
//						break;
//					case int('q'):
//						movement.clear();
//						break;
//					case 27:
//						MessageBox(nullptr, string("YOU QUIT THE GAME.").c_str(), "EXIT", MB_OK);
//						return 0;
//					case int('\b'): {
//						if(!LGgame::isAlive[1])
//							break;
//						int confirmSur = MessageBox(nullptr, string("ARE YOU SURE TO SURRENDER?").c_str(), "CONFIRM SURRENDER", MB_YESNO);
//						if(confirmSur == 7)
//							break;
//						LGgame::isAlive[1] = 0;
//						for(int i = 1; i <= mapH; ++i) {
//							for(int j = 1; j <= mapW; ++j) {
//								if(gameMap[i][j].team == 1) {
//									gameMap[i][j].team = 0;
//									if(gameMap[i][j].type == 3)
//										gameMap[i][j].type = 4;
//								}
//							}
//						}
//						break;
//					}
//				}
			}
			gotoxy(21,1);puts("fuck!");
			if(std::chrono::steady_clock::now().time_since_epoch() - lPT < std::chrono::milliseconds(stDel))
				continue;
			LGgame::updateMap();
			while(!movement.empty() && LGgame::analyzeMove(1, movement.front(), coordinate[1]))
				movement.pop_front();
			if(!movement.empty())
				movement.pop_front();
			for(int i = 2; i <= LGgame::playerCnt; ++i) {
				if(!LGgame::isAlive[i])
					continue;
				switch(robotId[i]) {
					case 1 ... 100:
						LGgame::analyzeMove(i, smartRandomBot::smartRandomBot(i, coordinate[i]), coordinate[i]);
						break;
					case 101 ... 200:
						LGgame::analyzeMove(i, xrzBot::xrzBot(i, coordinate[i]), coordinate[i]);
						break;
					case 201 ... 300:
						LGgame::analyzeMove(i, xiaruizeBot::xiaruizeBot(i, coordinate[i]), coordinate[i]);
						break;
					default:
						LGgame::analyzeMove(i, 0, coordinate[i]);
				}
			}
			LGgame::flushMove();
			if(PTC!=1048575) {
				int alldead = 0;
				for(int i=1; i<=plCnt&&!alldead; ++i) {
					if(PTC&(1<<i)) if(LGgame::isAlive[i]) alldead=1;
				}
				if(!alldead) {
					PTC=1048575;
					MessageBox(nullptr,"ALL THE PLAYERS YOU SELECTED TO BE SEEN IS DEAD.\nTHE OVERALL CHEAT MODE WILL BE SWITCHED ON.","TIP",MB_OK);
				}
			}
			if(!gameEnd) {
				int ed = 0;
				for(int i = 1; i <= plCnt; ++i)
					ed |= (LGgame::isAlive[i] << i);
				if(__builtin_popcount(ed) == 1) {
					MessageBox(nullptr,
					           ("PLAYER " + playerInfo[std::__lg(ed)].name + " WON!" + "\n" +
					            "THE GAME WILL CONTINUE." + "\n" +
					            "YOU CAN PRESS [ESC] TO EXIT.")
					           .c_str(),
					           "GAME END", MB_OK);
					gameEnd = 1;
					PTC = 1048575;
				}
			}
			gotoxy(1, 1);
			printMap(PTC, coordinate[1]);
			fflush(stdout);
			lPT = std::chrono::steady_clock::now().time_since_epoch();
		}
		return 0;
	}
}
