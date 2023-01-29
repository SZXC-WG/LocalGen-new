/* This is LGpages.hpp file of LocalGen.                                  */
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

#ifndef __LGPAGES_HPP__
#define __LGPAGES_HPP__

#include <random>
#include <chrono>
#include "LGcons.hpp"
#include "LGmaps.hpp"
#include "LGzipmap.hpp"
#include "LGgame.hpp"

const int frameH = 7, frameW = 49;

inline void CB(int k)
{
	setbcolor(defTeams[k].color);
	fputs("  ", stdout);
	resetattr();
}

inline void chsFrame(int x, int y, bool f)
{
	register int i;
	gotoxy(x, y);
	putchar(f ? '+' : ' ');

	for (i = 1; i <= frameW; i++)
		putchar(f ? '-' : ' ');

	putchar(f ? '+' : ' ');
	gotoxy(x + frameH + 1, y);
	putchar(f ? '+' : ' ');

	for (i = 1; i <= frameW; i++)
		putchar(f ? '-' : ' ');
	putchar(f ? '+' : ' ');

	for (i = 1; i <= frameH; i++)
	{
		gotoxy(x + i, y);
		putchar(f ? '|' : ' ');
		gotoxy(x + i, y + frameW + 1);
		putchar(f ? '|' : ' ');
	}
}

void MainPage()
{
	clearance();
	hideCursor();
	register int i, j, x, y;
	std::mt19937 mtrd(std::chrono::system_clock::now().time_since_epoch().count());
	LGGraphics::WelcomePage();
	LGGraphics::selectOrImportMap();
	setfcolor(defTeams[0].color);
	char chCmd = 0, fileName[105];
	int chs = 0, plCnt, stDel, cht, cheatCode;
	FILE *fileP;
	if (!chs)
	{
		if (!dllExit)
		{
			clearance();
			gotoxy(1, 1);
			printf("Oops, it seems like you don't have 'defMap.dll'!");
			Sleep(2000);
			exitExe();
		}

		i = 1, x = 3, y = 6;
		clearance();
		hideCursor();

		for (; i <= mapNum; i++)
		{
			gotoxy(x, y);
			printf("id:%02d", maps[i].id);
			gotoxy(x, y + 7);
			printf("%s", maps[i].chiname.c_str());
			gotoxy(x + 1, y);
			printf("%s", maps[i].engname.c_str());
			gotoxy(x + 2, y);
			printf("Auth: %s", maps[i].auth.c_str());
			gotoxy(x + 3, y);
			printf("Hei: %d", maps[i].hei);
			gotoxy(x + 3, y + 20);
			printf("Wid: %d", maps[i].wid);
			gotoxy(x + 4, y);
			printf("GeneralCnt: %d", maps[i].generalcnt);
			gotoxy(x + 4, y + 20);
			printf("PlainCnt: %d", maps[i].plaincnt);
			gotoxy(x + 5, y);
			printf("SwampCnt: %d", maps[i].swampcnt);
			gotoxy(x + 5, y + 20);
			printf("MountainCnt: %d", maps[i].mountaincnt);
			gotoxy(x + 6, y);
			printf("CityCnt: %d", maps[i].citycnt);

			if (y > frameW * 4)
				y = 6, x += frameH + 2;
			else
				y += frameW + 2;
		}

		x = 2, y = 4, chs = 1;
		chsFrame(x, y, 1);

		chCmd = 0;
		while (chCmd != 13)
		{
			chCmd = getch();
			chsFrame(x, y, 0);

			switch (tolower(chCmd))
			{
			case 'w':
				if (chs - 5 > 0)
					chs -= 5, x = x - frameH - 2;
				break;
			case 's':
				if (chs + 5 <= mapNum)
					chs += 5, x = x + frameH + 2;
				break;
			case 'a':
				if (chs - 1 > 0)
					chs--, y = y - frameW - 2;
				break;
			case 'd':
				if (chs + 1 <= mapNum)
					chs++, y = y + frameW + 2;
				break;
			}

			if (y > frameW * 5)
				y = 4, x += frameH + 2;
			if (y < 4)
				y = 12 + frameW * 4, x = x - frameH - 2;
			chsFrame(x, y, 1);
			gotoxy(100, 1);
			printf("%d %d %d", chs, x, y);
		}

		clearance();
		hideCursor();
		x = 3, y = 6;
		gotoxy(x, y);
		printf("id:%02d", maps[chs].id);
		gotoxy(x, y + 7);
		printf("%s", maps[chs].chiname.c_str());
		gotoxy(x + 1, y);
		printf("%s", maps[chs].engname.c_str());
		gotoxy(x + 2, y);
		printf("Auth: %s", maps[chs].auth.c_str());
		gotoxy(x + 3, y);
		printf("Hei: %d", maps[chs].hei);
		gotoxy(x + 3, y + 20);
		printf("Wid: %d", maps[chs].wid);
		gotoxy(x + 4, y);
		printf("GeneralCnt: %d", maps[chs].generalcnt);
		gotoxy(x + 4, y + 20);
		printf("PlainCnt: %d", maps[chs].plaincnt);
		gotoxy(x + 5, y);
		printf("SwampCnt: %d", maps[chs].swampcnt);
		gotoxy(x + 5, y + 20);
		printf("MountainCnt: %d", maps[chs].mountaincnt);
		gotoxy(x + 6, y);
		printf("CityCnt: %d", maps[chs].citycnt);
		x = 2, y = 4;
		chsFrame(x, y, 1);

	__defmap_plcnt:
		gotoxy(2, 11 + frameW);
		printf("Player Count (<=12): ");
		clearline();
		scanf("%d", &plCnt);
		if (plCnt < 2 || plCnt > 12)
			goto __defmap_plcnt;
	__spemap_maph:
		gotoxy(3, 11 + frameW);
		if (chs < 6)
		{
			int H, W;
			long long amn, amx;
			printf("Map Height (<=500): ");
			clearline();
			scanf("%d", &H);
			if (H < 5 || H > 500)
				goto __spemap_maph;
		__spemap_mapw:
			gotoxy(4, 11 + frameW);
			printf("Map Width (<=500): ");
			clearline();
			scanf("%d", &W);
			if (W < 5 || W > 500)
				goto __spemap_mapw;
			gotoxy(5, 11 + frameW);
			if (chs == 3)
			{
				printf("MINIMUM Army: ");
				clearline();
				scanf("%lld", &amn);
				gotoxy(6, 11 + frameW);
				printf("MAXIMUM Army: ");
				clearline();
				scanf("%lld", &amx);
				gotoxy(7, 11 + frameW);
			}
			switch (chs)
			{
			case 1:
				createRandomMap(H, W);
				break;
			case 2:
				createStandardMap(H, W);
				break;
			case 3:
				createFullCityMap(H, W, amn, amx, plCnt);
				break;
			case 4:
				createFullSwampMap(H, W, plCnt);
				break;
			case 5:
				createFullPlainMap(H, W, plCnt);
				break;
			}
		}
		else
			copyMap(chs);
	}
	else
	{
		gotoxy(18, 37 + 17);
		showCursor();
		scanf("%s", fileName);
		fileP = fopen(fileName, "r");
		fscanf(fileP, "%s", strdeZip);
		fclose(fileP);
		deZip();
		hideCursor();
	__import_map_plcnt:
		gotoxy(20, 37);
		showCursor();
		printf("Player Count (<=12): ");
		clearline();
		scanf("%d", &plCnt);
		if (plCnt < 2 || plCnt > 12)
			goto __import_map_plcnt;
		hideCursor();
	}

	//	if(chs) {
	//		gotoxy(18,54);
	//		scanf("%s",fileName);
	//		fileP=fopen(fileName,"r");
	//		fscanf(fileP,"%s",strdeZip);
	//		fclose(fileP);
	//		deZip();
	//	} else {
	//		char ch;
	//		int mapid=0;
	//		gotoxy(17,37); printf("Map Lists:"); clearline();
	//		gotoxy(18,37); printf("   %2s | %-20s | %-42s |","ID","CHN name","ENG name");
	//		for(int i=1; i<=mapNum; ++i) gotoxy(18+i,37),printf("   %2d | %-20s | %-42s |",i,maps[i].chiname.c_str(),maps[i].engname.c_str());
	//		gotoxy(19,37); printf(">>");
	//		int chsd=1;
	//		do {
	//			ch=getch();
	//			hideCursor();
	//			gotoxy(18+chsd,37); printf("  ");
	//			switch(tolower(ch)){
	//				case 'w': if(chsd>1) --chsd; break;
	//				case 's': if(chsd<mapNum) ++chsd; break;
	//			}
	//			gotoxy(18+chsd,37); printf(">>");
	//		} while(ch!=13);
	//		if(chsd>=5) {
	//			copyMap(chsd);
	//			int g=maps[chsd].generalcnt;
	//			int p=maps[chsd].plaincnt;
	//			int m=maps[chsd].mountaincnt;
	//			int s=maps[chsd].swampcnt;
	//			int c=maps[chsd].citycnt;
	//			gotoxy(19,113); printf("Map Information",p); clearline();
	//			gotoxy(20,113); printf("%2s | %4s | %4s | %4s | %4s |","G","P","C","M","S");
	//			gotoxy(21,113); printf("%2d | %4d | %4d | %4d | %4d |",g,p,c,m,s);
	//			__CHSDPLAYER: gotoxy(22,113); printf("Input Player Count(<=%d): ",std::min(g+p,16)); scanf("%d",&plCnt); if(plCnt<0||plCnt>std::min(p,16)) goto __CHSDPLAYER;
	//		}
	//		else {
	//			int H,W,amn,amx;
	//			gotoxy(19,113); printf("Map Height: "); scanf("%d",&H);
	//			gotoxy(20,113); printf("Map Width: "); scanf("%d",&W);
	//			__CHSDSPEPLAYER: gotoxy(21,113); printf("Player Count(<=%d):",std::min(H*W,16)); scanf("%d",&plCnt); if(plCnt<0||plCnt>std::min(H*W,16)) goto __CHSDSPEPLAYER;
	//			switch(chsd) {
	//				case 1: createRandomMap(H,W); break;
	//				case 2: {
	//					gotoxy(22,113); printf("MINIMUM Army: "); scanf("%d",&amn);
	//					gotoxy(23,113); printf("MAXIMUM Army: "); scanf("%d",&amx);
	//					createFullCityMap(H,W,amn,amx,plCnt); break;
	//				}
	//				case 3: createFullSwampMap(H,W,plCnt); break;
	//				case 4: createFullPlainMap(H,W,plCnt); break;
	//			}
	//		}
	//		for(int i=-1; i<=mapNum; ++i) gotoxy(18+i,37),clearline();
	//	}

inputstDel:;
	gotoxy(2, 41 + frameW);
	printf("Choose Game Speed:");
	clearline();
	for (int i = 1; i <= 20; ++i)
	{
		gotoxy(2 + i, 41 + frameW + 3);
		printf("%dx", i);
	}
	gotoxy(2 + 21, 41 + frameW + 3);
	printf("FAST!");
	gotoxy(2 + 1, 41 + frameW);
	printf(">> ");
	chs = 1;
	chCmd = 0;
	while (chCmd != 13)
	{
		chCmd = getch();
		hideCursor();
		gotoxy(2 + chs, 41 + frameW);
		printf("  ");
		switch (tolower(chCmd))
		{
		case 'w':
			if (chs > 1)
				--chs;
			break;
		case 's':
			if (chs < 21)
				++chs;
			break;
		}
		gotoxy(2 + chs, 41 + frameW);
		printf(">>");
	}
	if (chs == 21)
		stDel = 0;
	else
		stDel = 1000 / chs;

	gotoxy(2, 63 + frameW);
	printf("Cheat Code:");
	gotoxy(3, 63 + frameW);
	printf("Select the players you want to watch directly.");
	cheatCode = 2;
	chs = 1;
inputCheat:;
	hideCursor();
	for (int i = 1; i <= plCnt; ++i)
	{
		gotoxy(3 + i, 63 + frameW + 3);
		setfcolor(defTeams[i].color);
		if (cheatCode & (1 << i))
			underline();
		printf("%s", defTeams[i].name.c_str());
		resetattr();
		setfcolor(0);
		setbcolor(0);
		printf("|");
		resetattr();
	}
	gotoxy(3 + plCnt + 1, 63 + frameW + 3);
	printf("Overall");
	if (cheatCode == 1048575)
	{
		gotoxy(3 + plCnt + 1, 63 + frameW + 3 + 7 + 1);
		underline();
		printf("Selected");
		resetattr();
		setfcolor(0);
		setbcolor(0);
		printf("|");
		resetattr();
	}
	else
		printf("         ");
	gotoxy(3 + plCnt + 2, 63 + frameW + 3);
	printf("COMPLETE SELECTION");
	gotoxy(3 + chs, 63 + frameW);
	printf(">>");
	chCmd = 0;
	while (chCmd != 13)
	{
		chCmd = getch();
		gotoxy(3 + chs, 63 + frameW);
		printf("  ");
		switch (tolower(chCmd))
		{
		case 'w':
			if (chs > 1)
				--chs;
			break;
		case 's':
			if (chs < plCnt + 2)
				++chs;
			break;
		}
		gotoxy(3 + chs, 63 + frameW);
		printf(">>");
	}
	if (chs != plCnt + 2)
	{
		if (chs == plCnt + 1)
			cheatCode = ((cheatCode & 1) ? 2 : 1048575);
		else
			cheatCode ^= (1 << chs);
		goto inputCheat;
	}

	clearance();
	GAME(0, cheatCode, plCnt, stDel);
	return;
}

#endif
