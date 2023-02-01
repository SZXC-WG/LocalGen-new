#include <bits/stdc++.h>
#include "LGcons.hpp"
#include "LGmaps.hpp"
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
int GAME(bool isWeb, int cheatCode, int plCnt, int stDel);
namespace LGGraphics
{
	string fileName;
	int stDel = 1;
	int plCnt = 0;
	int mapSelected = 0;
	int cheatCode = 0;

	struct mapData
	{
		int heightPerBlock;
		int widthPerBlock;
		int height, width;
	} mapDataStore;
	void selectOrImportMap();
	void doMapImport();
	void WelcomePage();
	void doMapSelect();
	void importGameSettings();
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
		importGameSettings();
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
			setfont(20, 0, "Segoe UI");
			rectprintf(left, up + 1, 150, 40, "id:%02d %s", maps[i].id, maps[i].chiname.c_str());
			setfont(18, 0, "Segoe UI");
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
				int x = (msg.y + 199) / 200, y = (msg.x + 199) / 200;
				mapSelected = (x - 1) * 6 + y;
				break;
			}
		}
		cleardevice();
		setcolor(GREEN);
		setfont(40, 0, "Segoe UI");
		xyprintf(10, 10, "id: %02d", maps[mapSelected].id);
		xyprintf(10, 40, "%s", maps[mapSelected].chiname.c_str());
		setfont(30, 0, "Segoe UI");
		xyprintf(300, 40, "%s", maps[mapSelected].engname.c_str());
		xyprintf(10, 70, "Author of the Map: %s", maps[mapSelected].auth.c_str());
		xyprintf(10, 100, "Size of the Map: %d * %d", maps[mapSelected].hei, maps[mapSelected].wid);
		xyprintf(10, 130, "GeneralCount : %d          PlainCount: %d", maps[mapSelected].generalcnt, maps[mapSelected].plaincnt);
		xyprintf(10, 160, "MountainCount: %d          CityCount : %d", maps[mapSelected].mountaincnt, maps[mapSelected].citycnt);
		if (mapSelected < 6)
		{
			setfont(30, 0, "Segoe UI");
			setcolor(MAGENTA);
			int height = 0;
			key_msg msg;
			xyprintf(10, 200, "Please Input the Height of the Map (<=500)");
			while (1)
			{
				msg = getkey();
				if (msg.msg == key_msg_char)
				{
					if (msg.key == 13 && height <= 500 && height >= 1)
						break;
					if (msg.key == 8)
						height /= 10;
					else if (msg.key >= '0' && msg.key <= '9')
					{
						height = height * 10 + msg.key - '0';
					}
					cleardevice();
					setcolor(GREEN);
					setfont(40, 0, "Segoe UI");
					xyprintf(10, 10, "id: %02d", maps[mapSelected].id);
					xyprintf(10, 40, "%s", maps[mapSelected].chiname.c_str());
					setfont(30, 0, "Segoe UI");
					xyprintf(300, 40, "%s", maps[mapSelected].engname.c_str());
					xyprintf(10, 70, "Author of the Map: %s", maps[mapSelected].auth.c_str());
					xyprintf(10, 100, "Size of the Map: %d * %d", maps[mapSelected].hei, maps[mapSelected].wid);
					xyprintf(10, 130, "GeneralCount : %d          PlainCount: %d", maps[mapSelected].generalcnt, maps[mapSelected].plaincnt);
					xyprintf(10, 160, "MountainCount: %d          CityCount : %d", maps[mapSelected].mountaincnt, maps[mapSelected].citycnt);
					setcolor(MAGENTA);
					xyprintf(10, 200, "Please Input the Height of the Map (<=500)");
					xyprintf(10, 230, "%d", height);
				}
			}
			cleardevice();
			setcolor(GREEN);
			setfont(40, 0, "Segoe UI");
			xyprintf(10, 10, "id: %02d", maps[mapSelected].id);
			xyprintf(10, 40, "%s", maps[mapSelected].chiname.c_str());
			setfont(30, 0, "Segoe UI");
			xyprintf(300, 40, "%s", maps[mapSelected].engname.c_str());
			xyprintf(10, 70, "Author of the Map: %s", maps[mapSelected].auth.c_str());
			xyprintf(10, 100, "Size of the Map: %d * %d", maps[mapSelected].hei, maps[mapSelected].wid);
			xyprintf(10, 130, "GeneralCount : %d          PlainCount: %d", maps[mapSelected].generalcnt, maps[mapSelected].plaincnt);
			xyprintf(10, 160, "MountainCount: %d          CityCount : %d", maps[mapSelected].mountaincnt, maps[mapSelected].citycnt);
			setcolor(MAGENTA);
			xyprintf(10, 200, "Please Input the Height of the Map (<=500)");
			xyprintf(10, 230, "%d", height);
			int width = 0;
			xyprintf(600, 200, "Please Input the Width of the Map (<=500)");
			while (1)
			{
				msg = getkey();
				if (msg.msg == key_msg_char)
				{
					if (msg.key == 13 && width <= 500 && width >= 1)
						break;
					if (msg.key == 8)
						width /= 10;
					else if (msg.key >= '0' && msg.key <= '9')
					{
						width = width * 10 + msg.key - '0';
					}
					cleardevice();
					setcolor(GREEN);
					setfont(40, 0, "Segoe UI");
					xyprintf(10, 10, "id: %02d", maps[mapSelected].id);
					xyprintf(10, 40, "%s", maps[mapSelected].chiname.c_str());
					setfont(30, 0, "Segoe UI");
					xyprintf(300, 40, "%s", maps[mapSelected].engname.c_str());
					xyprintf(10, 70, "Author of the Map: %s", maps[mapSelected].auth.c_str());
					xyprintf(10, 100, "Size of the Map: %d * %d", maps[mapSelected].hei, maps[mapSelected].wid);
					xyprintf(10, 130, "GeneralCount : %d          PlainCount: %d", maps[mapSelected].generalcnt, maps[mapSelected].plaincnt);
					xyprintf(10, 160, "MountainCount: %d          CityCount : %d", maps[mapSelected].mountaincnt, maps[mapSelected].citycnt);
					setcolor(MAGENTA);
					xyprintf(10, 200, "Please Input the Height of the Map (<=500)");
					xyprintf(10, 230, "%d", height);
					xyprintf(600, 200, "Please Input the Width of the Map (<=500)");
					xyprintf(600, 230, "%d", width);
				}
			}
			long long armyMin = 0, armyMax = 0;
			if (mapSelected == 3)
			{
				setfont(30, 0, "Segoe UI");
				setcolor(MAGENTA);
				key_msg msg;
				xyprintf(10, 300, "Please Input the Minimum Number of Army on each Block");
				bool isPositive = true;
				while (1)
				{
					msg = getkey();
					if (msg.msg == key_msg_char)
					{
						if (msg.key == 13)
							break;
						if (msg.key == 8)
							armyMin /= 10;
						else if (msg.key >= '0' && msg.key <= '9')
						{
							armyMin = armyMin * 10 + msg.key - '0';
						}
						else if (msg.key == '-')
							isPositive = !isPositive;
						cleardevice();
						setcolor(GREEN);
						setfont(40, 0, "Segoe UI");
						xyprintf(10, 10, "id: %02d", maps[mapSelected].id);
						xyprintf(10, 40, "%s", maps[mapSelected].chiname.c_str());
						setfont(30, 0, "Segoe UI");
						xyprintf(300, 40, "%s", maps[mapSelected].engname.c_str());
						xyprintf(10, 70, "Author of the Map: %s", maps[mapSelected].auth.c_str());
						xyprintf(10, 100, "Size of the Map: %d * %d", maps[mapSelected].hei, maps[mapSelected].wid);
						xyprintf(10, 130, "GeneralCount : %d          PlainCount: %d", maps[mapSelected].generalcnt, maps[mapSelected].plaincnt);
						xyprintf(10, 160, "MountainCount: %d          CityCount : %d", maps[mapSelected].mountaincnt, maps[mapSelected].citycnt);
						setcolor(MAGENTA);
						xyprintf(10, 200, "Please Input the Height of the Map (<=500)");
						xyprintf(10, 230, "%d", height);
						xyprintf(600, 200, "Please Input the Width of the Map (<=500)");
						xyprintf(600, 230, "%d", width);
						xyprintf(10, 300, "Please Input the Minimum Number of Army on each Block");
						long long realarmy = armyMin;
						if (!isPositive)
							realarmy = -realarmy;
						xyprintf(10, 330, "%lld", realarmy);
					}
				}
				if (!isPositive)
					armyMin = -armyMin;
				cleardevice();
				setcolor(GREEN);
				setfont(40, 0, "Segoe UI");
				xyprintf(10, 10, "id: %02d", maps[mapSelected].id);
				xyprintf(10, 40, "%s", maps[mapSelected].chiname.c_str());
				setfont(30, 0, "Segoe UI");
				xyprintf(300, 40, "%s", maps[mapSelected].engname.c_str());
				xyprintf(10, 70, "Author of the Map: %s", maps[mapSelected].auth.c_str());
				xyprintf(10, 100, "Size of the Map: %d * %d", maps[mapSelected].hei, maps[mapSelected].wid);
				xyprintf(10, 130, "GeneralCount : %d          PlainCount: %d", maps[mapSelected].generalcnt, maps[mapSelected].plaincnt);
				xyprintf(10, 160, "MountainCount: %d          CityCount : %d", maps[mapSelected].mountaincnt, maps[mapSelected].citycnt);
				setcolor(MAGENTA);
				xyprintf(10, 200, "Please Input the Height of the Map (<=500)");
				xyprintf(10, 230, "%d", height);
				xyprintf(600, 200, "Please Input the Width of the Map (<=500)");
				xyprintf(600, 230, "%d", width);
				xyprintf(10, 300, "Please Input the Minimum Number of Army on each Block");
				xyprintf(10, 330, "%lld", armyMin);
				xyprintf(600, 300, "Please Input the Maximum Number of Army on each Block");
				isPositive = true;
				while (1)
				{
					msg = getkey();
					if (msg.msg == key_msg_char)
					{
						if (msg.key == 13 && armyMax * (isPositive ? 1 : (-1)) >= armyMin)
							break;
						if (msg.key == 8)
							armyMax /= 10;
						else if (msg.key >= '0' && msg.key <= '9')
						{
							armyMax = armyMax * 10 + msg.key - '0';
						}
						else if (msg.key == '-')
							isPositive = !isPositive;
						cleardevice();
						setcolor(GREEN);
						setfont(40, 0, "Segoe UI");
						xyprintf(10, 10, "id: %02d", maps[mapSelected].id);
						xyprintf(10, 40, "%s", maps[mapSelected].chiname.c_str());
						setfont(30, 0, "Segoe UI");
						xyprintf(300, 40, "%s", maps[mapSelected].engname.c_str());
						xyprintf(10, 70, "Author of the Map: %s", maps[mapSelected].auth.c_str());
						xyprintf(10, 100, "Size of the Map: %d * %d", maps[mapSelected].hei, maps[mapSelected].wid);
						xyprintf(10, 130, "GeneralCount : %d          PlainCount: %d", maps[mapSelected].generalcnt, maps[mapSelected].plaincnt);
						xyprintf(10, 160, "MountainCount: %d          CityCount : %d", maps[mapSelected].mountaincnt, maps[mapSelected].citycnt);
						setcolor(MAGENTA);
						xyprintf(10, 200, "Please Input the Height of the Map (<=500)");
						xyprintf(10, 230, "%d", height);
						xyprintf(600, 200, "Please Input the Width of the Map (<=500)");
						xyprintf(600, 230, "%d", width);
						xyprintf(10, 300, "Please Input the Minimum Number of Army on each Block");
						xyprintf(10, 330, "%lld", armyMin);
						xyprintf(600, 300, "Please Input the Maximum Number of Army on each Block");
						long long realarmy = armyMax;
						if (!isPositive)
							realarmy = -realarmy;
						xyprintf(600, 330, "%lld", realarmy);
					}
				}
				if (!isPositive)
					armyMax = -armyMax;
				cleardevice();
				setcolor(GREEN);
				setfont(40, 0, "Segoe UI");
				xyprintf(10, 10, "id: %02d", maps[mapSelected].id);
				xyprintf(10, 40, "%s", maps[mapSelected].chiname.c_str());
				setfont(30, 0, "Segoe UI");
				xyprintf(300, 40, "%s", maps[mapSelected].engname.c_str());
				xyprintf(10, 70, "Author of the Map: %s", maps[mapSelected].auth.c_str());
				xyprintf(10, 100, "Size of the Map: %d * %d", maps[mapSelected].hei, maps[mapSelected].wid);
				xyprintf(10, 130, "GeneralCount : %d          PlainCount: %d", maps[mapSelected].generalcnt, maps[mapSelected].plaincnt);
				xyprintf(10, 160, "MountainCount: %d          CityCount : %d", maps[mapSelected].mountaincnt, maps[mapSelected].citycnt);
				setcolor(MAGENTA);
				xyprintf(10, 200, "Please Input the Height of the Map (<=500)");
				xyprintf(10, 230, "%d", height);
				xyprintf(600, 200, "Please Input the Width of the Map (<=500)");
				xyprintf(600, 230, "%d", width);
				xyprintf(10, 300, "Please Input the Minimum Number of Army on each Block");
				xyprintf(10, 330, "%lld", armyMin);
				xyprintf(600, 300, "Please Input the Maximum Number of Army on each Block");
				xyprintf(600, 330, "%lld", armyMax);
			}
			importGameSettings();
			switch (mapSelected)
			{
			case 1:
				createRandomMap(height, width);
				break;
			case 2:
				createStandardMap(height, width);
				break;
			case 3:
				createFullCityMap(height, width, armyMin, armyMax, plCnt);
				break;
			case 4:
				createFullSwampMap(height, width, plCnt);
				break;
			case 5:
				createFullPlainMap(height, width, plCnt);
				break;
			}
		}
		else
		{
			cleardevice();
			setcolor(GREEN);
			setfont(40, 0, "Segoe UI");
			xyprintf(10, 10, "id: %02d", maps[mapSelected].id);
			xyprintf(10, 40, "%s", maps[mapSelected].chiname.c_str());
			setfont(30, 0, "Segoe UI");
			xyprintf(300, 40, "%s", maps[mapSelected].engname.c_str());
			xyprintf(10, 70, "Author of the Map: %s", maps[mapSelected].auth.c_str());
			xyprintf(10, 100, "Size of the Map: %d * %d", maps[mapSelected].hei, maps[mapSelected].wid);
			xyprintf(10, 130, "GeneralCount : %d          PlainCount: %d", maps[mapSelected].generalcnt, maps[mapSelected].plaincnt);
			xyprintf(10, 160, "MountainCount: %d          CityCount : %d", maps[mapSelected].mountaincnt, maps[mapSelected].citycnt);
			importGameSettings();
			copyMap(mapSelected);
		}
		GAME(0, cheatCode, plCnt, stDel);
	}

	void doMapImport()
	{
		cleardevice();
		setcolor(BLUE);
		setfont(30, 0, "Segoe UI");
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
				setfont(30, 0, "Segoe UI");
				xyprintf(10, 10, "Please Input Filename with suffix (end with enter)");
				xyprintf(10, 110, "%s", fileName.c_str());
			}
		}
		toAvoidCEBugInGraphicsImportMap(fileName);
	}

	bool cheatCodeSelected[13];

	void importGameSettings()
	{
		int circlePos = 550;
		PIMAGE refreshCopy = newimage();
		getimage(refreshCopy, 0, 0, 1200, 400);
		setfont(20, 0, "Segue UI");
		setcolor(BLUE);
		bool changeMade = true;
		int mouseDownCount = 0;
		bool endConfig = false;
		cheatCode = 2;
		cheatCodeSelected[1] = true;
		for (; is_run(); delay_fps(120))
		{
			if (changeMade)
			{
				cleardevice();
				putimage(0, 0, refreshCopy);
				setcolor(BLUE);
				rectprintf(10, 450, 380, 100, "Please Select The Number of Players");
				for (int i = 1; i <= 12; i++)
				{
					int lineNumber = (i + 2) / 3;
					int columnNumber = i - (lineNumber - 1) * 3;
					if (i == plCnt)
					{
						setcolor(RED);
						circle(100 * columnNumber, 600 + 100 * (lineNumber - 1), 50);
						setcolor(BLUE);
					}
					xyprintf(100 * columnNumber, 600 + 100 * (lineNumber - 1), "%d", i);
					rectangle((columnNumber - 1) * 100 + 50, (lineNumber - 1) * 100 + 550, columnNumber * 100 + 50, lineNumber * 100 + 550);
				}
				rectprintf(410, 450, 380, 100, "Drag to Select the Speed of the Game");
				setfillcolor(LIGHTGRAY);
				bar(575, 550, 625, 950);
				setfillcolor(BLUE);
				fillellipsef(600, circlePos, 30, 30);
				rectprintf(810, 450, 380, 100, "Select the Players you want to watch Directly");
				for (int i = 1; i <= 12; i++)
				{
					int lineNumber = (i + 2) / 3;
					int columnNumber = i - (lineNumber - 1) * 3;
					setcolor(BLUE);
					rectangle((columnNumber - 1) * 100 + 850, (lineNumber - 1) * 100 + 550, columnNumber * 100 + 850, lineNumber * 100 + 550);
				}
				for (int i = 1; i <= plCnt; i++)
				{
					int lineNumber = (i + 2) / 3;
					int columnNumber = i - (lineNumber - 1) * 3;
					setcolor(defTeams[i].color);
					rectprintf((columnNumber - 1) * 100 + 850, (lineNumber - 1) * 100 + 550, 100, 100, "%s", defTeams[i].name.c_str());
					if (cheatCodeSelected[i])
					{
						setcolor(RED);
						circle(800 + 100 * columnNumber, 600 + 100 * (lineNumber - 1), 50);
						setcolor(BLUE);
					}
				}
				rectangle(900, 30, 1100, 105);
				setfont(70, 0, "Freestyle Script");
				rectprintf(900, 30, 200, 75, "Start Game!");
				setfont(20, 0, "Segue UI");
				if (stDel != 0)
					xyprintf(405, 975, "Speed Now: %d", stDel);
				else
					xyprintf(405, 975, "Speed Now: MAXSPEED");
				changeMade = false;
			}
			while (mousemsg())
			{
				mouse_msg msg = getmouse();
				// cout << msg.x << ' ' << msg.y << ' ' << msg.is_left() << ' ' << msg.is_down() << ' ' << msg.is_up() << ' ' << msg.is_move() << ' ' << mouseDownCount << ' ' << circlePos << endl;
				if (50 <= msg.x && msg.x <= 350 && 550 <= msg.y && 950 >= msg.y)
				{
					if (msg.is_left() && msg.is_down())
					{
						int col = (msg.x + 49) / 100;
						int lin = (msg.y - 550 + 99) / 100;
						plCnt = (lin - 1) * 3 + col;
						changeMade = true;
					}
				}
				if (msg.is_left() && msg.is_down())
					mouseDownCount++;
				if (msg.is_left() && msg.is_up() && mouseDownCount > 0)
					mouseDownCount--;
				if (msg.x >= 570 && msg.x <= 630 && msg.y >= 550 && msg.y <= 950)
				{
					if (mouseDownCount > 0)
					{
						circlePos = msg.y;
						if (circlePos >= 940)
							stDel = 0;
						else
							stDel = ((circlePos - 550) / 20) + 1;
						changeMade = true;
					}
				}
				if (850 <= msg.x && 1150 >= msg.x && 550 <= msg.y && msg.y <= 950)
				{
					if (msg.is_left() && msg.is_down())
					{
						int col = (msg.x - 850 + 99) / 100;
						int lin = (msg.y - 550 + 99) / 100;
						int num = (lin - 1) * 3 + col;
						if (num <= plCnt && num > 0)
						{
							cheatCode ^= (1 << (num));
							cheatCodeSelected[num] ^= 1;
							changeMade = true;
						}
					}
				}
				if (msg.x >= 900 && msg.x <= 1100 && msg.y >= 30 && msg.y <= 105 && msg.is_down() && msg.is_left())
				{
					endConfig = true;
					cleardevice();
					if (stDel == 0)
						stDel = 60;
					return;
				}
			}
		}
	}

	void init()
	{
		heightPerBlock = 900 / mapH;
		widthPerBlock = 900 / mapW;
		heightPerBlock = widthPerBlock = min(heightPerBlock, widthPerBlock);
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
		initgraph(1700, 1000);
		setbkcolor(WHITE);
		setbkcolor_f(WHITE);
		cleardevice();
	}
}

int getHeightPerBlock()
{
	return LGGraphics::mapDataStore.heightPerBlock;
}

int getWidthPerBlock()
{
	return LGGraphics::mapDataStore.widthPerBlock;
}
