#include <bits/stdc++.h>
#include "LGcons.hpp"
#include "LGmaps.hpp"
#include "LGzipmap.hpp"
#include "LGgame.hpp"
#include <graphics.h>
using namespace std;

void exitExe();

namespace imageOperation
{
	void zoomImage(PIMAGE &pimg, int zoomWidth, int zoomHeight)
	{
		if ((pimg == NULL) || (zoomWidth == getwidth(pimg) && zoomHeight == getheight(pimg)))
			return;

		PIMAGE zoomImage = newimage(zoomWidth, zoomHeight);
		putimage(zoomImage, 0, 0, zoomWidth, zoomHeight, pimg, 0, 0, getwidth(pimg), getheight(pimg));
		delimage(pimg);

		pimg = zoomImage;
	}
}

bool isdllOK();
void toAvoidCEBugInGraphicsImportMap(string fileName);
int returnMapNum();
namespace LGGraphics
{
	string fileName;
	int plCnt = 0;

	struct mapData
	{
		int heightPerBlock;
		int widthPerBlock;
		int height, width;
	} mapDataStore;
	void selectOrImportMap();
	void doMapImport();
	void importPlCnt();
	void WelcomePage();
	void doMapSelect();
	void inputMapData(int a, int b, int c, int d)
	{
		mapDataStore.heightPerBlock = a;
		mapDataStore.widthPerBlock = b;
		mapDataStore.height = c;
		mapDataStore.width = d;
		return;
	}

	void WelcomePage()
	{
		initgraph(1200, 1000);
		setbkcolor(WHITE);
		setbkcolor_f(WHITE);
		setfont(500, 0, "Freestyle Script");
		setcolor(BLUE);
		xyprintf(150, 0, "LocalGen");
		setfillcolor(GREEN);
		bar(100, 600, 500, 800);
		setbkmode(TRANSPARENT);
		setcolor(WHITE);
		setfont(100, 0, "Freestyle Script");
		xyprintf(150, 650, "Single Player");
		setfillcolor(RED);
		bar(700, 600, 1100, 800);
		xyprintf(750, 650, "MultiPlayer");
		mouse_msg msg;
		while (1)
		{
			msg = getmouse();
			// xyprintf(750, 650, "Mouse!");
			// cout << msg.x << ' ' << msg.y << endl;
			if (msg.is_left() && msg.is_down() && msg.x >= 100 && msg.x <= 500 && msg.y >= 600 && msg.y <= 800)
				break;
			if (msg.is_left() && msg.is_down() && msg.x >= 700 && msg.x <= 1100 && msg.y >= 600 && msg.y <= 800)
			{
				xyprintf(400, 900, "Sorry! Multiplayer Mode is still developping.");
				Sleep(4000);
				exitExe();
			}
		}
	}

	void selectOrImportMap()
	{
		setfillcolor(BROWN);
		bar(100, 600, 500, 800);
		setbkmode(TRANSPARENT);
		setcolor(WHITE);
		setfont(100, 0, "Freestyle Script");
		xyprintf(150, 650, "Choose a Map");
		setfillcolor(BROWN);
		bar(700, 600, 1100, 800);
		xyprintf(750, 650, "Import Map");
		mouse_msg msg;
		bool select;
		while (1)
		{
			msg = getmouse();
			// xyprintf(750, 650, "Mouse!");
			// cout << msg.x << ' ' << msg.y << endl;
			if (msg.is_left() && msg.is_down() && msg.x >= 100 && msg.x <= 500 && msg.y >= 600 && msg.y <= 800)
			{
				select = true;
				break;
			}
			if (msg.is_left() && msg.is_down() && msg.x >= 700 && msg.x <= 1100 && msg.y >= 600 && msg.y <= 800)
			{
				select = false;
				break;
			}
		}
		cleardevice();
		if (!select)
			doMapImport();
		else
			doMapSelect();
	}

	void doMapSelect()
	{
		int mapNum = returnMapNum();
		cleardevice();
		if (!isdllOK())
		{
			xyprintf(10, 10, "Oops, it seems that your 'defMap.dll' is missing!\n You can download it on github. ");
			Sleep(4000);
			exitExe();
		}
		int left, right, up, down;
		int x, y;
		for (int i = 1; i <= mapNum; i++)
		{
			x = (i + 5) / 6;
			y = ((i % 6 == 0) ? 6 : i % 6);
			left = (y - 1) * 200;
			right = y * 200;
			up = (x - 1) * 200;
			down = x * 200;
			// imageOperation::zoomImage(pimg[5], 200, 200);
			// putimage_transparent(NULL, pimg[5], left, up, getpixel(0, 0, pimg[1]));
			setcolor(BLUE);
			setfont(20, 0, "Consolas");
			rectprintf(left, up + 1, 150, 40, "id:%02d %s", maps[i].id, maps[i].chiname.c_str());
			setfont(18, 0, "Consolas");
			rectprintf(left, up + 40, 180, 20, "%s", maps[i].engname.c_str());
			rectprintf(left, up + 60, 180, 20, "GeneralCount: %d", maps[i].generalcnt);
			rectprintf(left, up + 80, 180, 20, "SwampCount: %d", maps[i].swampcnt);
			rectprintf(left, up + 100, 180, 20, "MountainCount: %d", maps[i].mountaincnt);
			rectprintf(left, up + 120, 180, 20, "CityCount: %d", maps[i].citycnt);
			rectprintf(left, up + 150, 180, 20, "Size: %d * %d", maps[i].hei, maps[i].wid);
		}
		mouse_msg msg;
		while (1)
		{
			msg = getmouse();
			if (msg.is_left() && msg.is_down())
			{
			}
		}
	}

	void doMapImport()
	{
		cleardevice();
		setcolor(BLUE);
		setfont(30, 0, "Consolas");
		xyprintf(10, 10, "Please Input Filename with suffix (end with enter)");
		key_msg msg;
		while (1)
		{
			msg = getkey();
			if (msg.msg == key_msg_char)
			{
				if (msg.key == 13)
					break;
				if (msg.key == 8)
					fileName.pop_back();
				else
					fileName += (char)(msg.key);
				cleardevice();
				setcolor(BLUE);
				setfont(30, 0, "Consolas");
				xyprintf(10, 10, "Please Input Filename with suffix (end with enter)");
				xyprintf(10, 110, "%s", fileName.c_str());
			}
		}
		toAvoidCEBugInGraphicsImportMap(fileName);
	}

	void importPlCnt()
	{
		cleardevice();
		setcolor(BLUE);
		setfont(30, 0, "Consolas");
		xyprintf(10, 10, "Please enter the number of players in the game (<=12) ");
		key_msg msg;
		plCnt = 0;
		while (1)
		{
			msg = getkey();
			if (msg.msg == key_msg_char)
			{
				if (msg.key == 13 && plCnt <= 12 && plCnt >= 1)
					break;
				if (msg.key == 8)
					plCnt /= 10;
				else if (msg.key >= '0' && msg.key <= '9')
				{
					plCnt = plCnt * 10 + msg.key - '0';
				}
				cleardevice();
				setcolor(BLUE);
				setfont(30, 0, "Consolas");
				xyprintf(10, 10, "Please enter the number of players in the game (<=12) ");
				xyprintf(10, 110, "%d", plCnt);
			}
		}
	}

	void init()
	{
		setbkmode(TRANSPARENT);
		pimg[1] = newimage();
		getimage(pimg[1], "img/city.png");
		imageOperation::zoomImage(pimg[1], mapDataStore.heightPerBlock, mapDataStore.widthPerBlock);
		pimg[2] = newimage();
		getimage(pimg[2], "img/crown.png");
		imageOperation::zoomImage(pimg[2], mapDataStore.heightPerBlock, mapDataStore.widthPerBlock);
		pimg[3] = newimage();
		getimage(pimg[3], "img/mountain.png");
		imageOperation::zoomImage(pimg[3], mapDataStore.heightPerBlock, mapDataStore.widthPerBlock);
		pimg[4] = newimage();
		getimage(pimg[4], "img/swamp.png");
		imageOperation::zoomImage(pimg[4], mapDataStore.heightPerBlock, mapDataStore.widthPerBlock);
		pimg[5] = newimage();
		getimage(pimg[5], "img/currentOn.png");
		imageOperation::zoomImage(pimg[5], mapDataStore.widthPerBlock, mapDataStore.heightPerBlock);
		initgraph(mapDataStore.width * mapDataStore.widthPerBlock, mapDataStore.height * mapDataStore.heightPerBlock);
		setbkcolor(LIGHTGRAY);
		setbkcolor_f(LIGHTGRAY);
		cleardevice();
	}
}
