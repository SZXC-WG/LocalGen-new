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

#ifndef __LGPRINT_HPP__
#define __LGPRINT_HPP__

struct Picxel
{
	int colorFront, colorBack;
	char Chr;

	bool operator==(Picxel x)
	{
		return colorFront == x.colorFront && colorBack == x.colorBack && Chr == x.Chr;
	}
};

char numS[15];
Picxel nptMap[505][505], ptMap[505][505];
const char NUM_s[15] = {0, 'H', 'K', 'W', 'L', 'M', 'Q', 'Y', 'B', 'G', 'T'};
int mapH, mapW, ptH, ptW;
const int CHEAT_CODE = 1048575;

void initMap()
{
	ptH = mapH;
	ptW = mapW * 5;

	for (int i = 1; i <= mapH; i++)
		for (int j = 1; j <= mapW; j++)
			gameMap[i][j].type = 0, gameMap[i][j].army = 0, gameMap[i][j].team = 0;
}

void printFrame()
{
	gotoxy(0, 1);
	for (int i = 1; i <= ptW; i++)
		printf("-");
	gotoxy(ptH + 1, 1);
	for (int i = 1; i <= ptW; i++)
		printf("-");
	for (int i = 1; i <= ptH; i++)
	{
		gotoxy(i, 0);
		printf("|");
		gotoxy(i, ptW + 1);
		printf("|");
	}

	gotoxy(0, ptW + 1);
	printf("+");
	gotoxy(0, 0);
	printf("+");
	gotoxy(ptH + 1, 0);
	printf("+");
	gotoxy(ptH + 1, ptW + 1);
	printf("+");
}

inline void fillCf(int x1, int y1, int x2, int y2, int C)
{
	register int i, j;
	for (i = x1; i <= x2; i++)
		for (j = y1; j <= y2; j++)
			nptMap[i][j].colorFront = C;
}
inline void fillCb(int x1, int y1, int x2, int y2, int C)
{
	register int i, j;
	for (i = x1; i <= x2; i++)
		for (j = y1; j <= y2; j++)
			nptMap[i][j].colorBack = C;
}
inline void fillCh(int x1, int y1, int x2, int y2, char c)
{
	register int i, j;
	for (i = x1; i <= x2; i++)
		for (j = y1; j <= y2; j++)
			nptMap[i][j].Chr = c;
}
inline void fillNs(int x, int y1, int y2)
{
	for (register int i = y1; i <= y2; i++)
		nptMap[x][i].Chr = numS[i - y1];
}

inline void turnStr(long long x)
{
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

inline bool printCheck(int x, int y, int printCode)
{
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

void writeMap()
{
	register int i, j;
	for (i = 1; i <= mapH; i++)
		for (j = 1; j <= mapW; j++)
		{
			if (printCheck(i, j, printCode))
			{
				switch (gameMap[i][j].type)
				{
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
				}
				if (gameMap[i][j].army != 0 && gameMap[i][j].type != 2)
				{
					turnStr(gameMap[i][j].army);
					fillNs(i, j * 5 - 3, j * 5 - 1);
				}
				else
					gameMap[i][j].army = 0;
				fillCf(i, j * 5 - 4, i, j * 5, playerInfo[gameMap[i][j].team].color);
			}
			else
			{
				switch (gameMap[i][j].type)
				{
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
}
void printMap(int printCode)
{
	writeMap(printCode);
	setfcolor(7);
	setbcolor(0);
	register int i, j, ncolorFront = 7, ncolorBack = 0;

	for (i = 1; i <= ptH; i++)
		for (j = 1; j <= ptW; j++)
			if (!(nptMap[i][j] == ptMap[i][j]))
			{
				if (nptMap[i][j].colorFront != ncolorFront || nptMap[i][j].colorBack != ncolorBack)
				{
					setfcolor(playerInfo[ncolorFront = nptMap[i][j].colorFront].color);
					setbcolor(playerInfo[ncolorBack = nptMap[i][j].colorBack].color);
				}
				if (ptMap[i][j].Chr != nptMap[i][j].Chr)
				{
					gotoxy(i, j);
					putchar(nptMap[i][j].Chr);
				}
				ptMap[i][j] = nptMap[i][j];
			}
}

#endif
