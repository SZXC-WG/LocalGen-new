/* This is LGgraphics.hpp file of LocalGen.                                */
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

#ifndef __LGGRAPHICS_HPP__
#define __LGGRAPHICS_HPP__

#include "LGdef.hpp"
#include "LGreplay.hpp"

namespace imageOperation {
	void copyImage(PIMAGE& dstimg, PIMAGE& srcimg) {
		if(dstimg == NULL || srcimg == NULL) return;
		getimage(dstimg,srcimg,0,0,getwidth(srcimg),getheight(srcimg));
	}
	void zoomImage(PIMAGE& pimg, int zoomWidth, int zoomHeight) {
		if((pimg == NULL) || (zoomWidth == getwidth(pimg) && zoomHeight == getheight(pimg)))
			return;

		PIMAGE zoomImage = newimage(zoomWidth, zoomHeight);
		putimage(zoomImage, 0, 0, zoomWidth, zoomHeight, pimg, 0, 0, getwidth(pimg), getheight(pimg));
		delimage(pimg);

		pimg = zoomImage;
	}
	void setWindowTransparent(bool enable, int alpha) {
		HWND egeHwnd = getHWnd();
		LONG nRet = ::GetWindowLong(egeHwnd, GWL_EXSTYLE);
		nRet |= WS_EX_LAYERED;
		::SetWindowLong(egeHwnd, GWL_EXSTYLE, nRet);
		if(!enable)
			alpha = 0xFF;
		SetLayeredWindowAttributes(egeHwnd, 0, alpha, LWA_ALPHA);
	}
}

bool FullScreen(HWND hwnd, int fullscreenWidth, int fullscreenHeight, int colourBits, int refreshRate) {
	DEVMODE fullscreenSettings;
	bool isChangeSuccessful;
	RECT windowBoundary;

	EnumDisplaySettings(NULL, 0, &fullscreenSettings);
	fullscreenSettings.dmPelsWidth = fullscreenWidth;
	fullscreenSettings.dmPelsHeight = fullscreenHeight;
	fullscreenSettings.dmBitsPerPel = colourBits;
	fullscreenSettings.dmDisplayFrequency = refreshRate;
	fullscreenSettings.dmFields = DM_PELSWIDTH |
	                              DM_PELSHEIGHT |
	                              DM_BITSPERPEL |
	                              DM_DISPLAYFREQUENCY;

	SetWindowLongPtr(hwnd, GWL_EXSTYLE, WS_EX_APPWINDOW);
	SetWindowLongPtr(hwnd, GWL_STYLE, GetWindowLongPtr(hwnd, GWL_STYLE) ^ WS_CAPTION);
	SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, fullscreenWidth, fullscreenHeight, SWP_SHOWWINDOW);
	isChangeSuccessful = ChangeDisplaySettings(&fullscreenSettings, CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL;

	return isChangeSuccessful;
}

inline void ege_circle(int x, int y, int r) { ege_ellipse(x - r, y - r, r << 1, r << 1); }
inline void ege_fillcircle(int x, int y, int r) { ege_fillellipse(x - r, y - r, r << 1, r << 1); }

namespace LGGraphics {
	void inputMapData(int a, int b, int c, int d) {
		mapDataStore.heightPerBlock = a;
		mapDataStore.widthPerBlock = b;
		mapDataStore.height = c;
		mapDataStore.width = d;
		return;
	}
	void initWindowSize() {
		initgraph(800, 600);
		ege_enable_aa(true, NULL);
		setcaption("LocalGen v" VER_STRING " Window Size Selection");
		setbkcolor(bgColor);
		setbkcolor_f(bgColor);
		bool changeMade = true;
		cleardevice();
		setbkmode(TRANSPARENT);
		settextjustify(CENTER_TEXT, CENTER_TEXT);
		setfont(100, 0, "QuickSand", 0, 0, FW_BOLD, 0, 0, 0);
		setcolor(mainColor);
		xyprintf(252, 82, "Local");
		setcolor(WHITE);
		xyprintf(250, 80, "Local");
		setcolor(mainColor);
		xyprintf(252, 152, "generals.io");
		setcolor(WHITE);
		xyprintf(250, 150, "generals.io");
		putimage_withalpha(NULL, favi, 500, 10, 0, 0, getwidth(favi), getheight(favi));
		delimage(favi);
		setfont(20, 0, "Lucida Fax");
		xyprintf(500 + getwidth(favi) / 2, 10 + getheight(favi) + 20 / 2 + 10,
		         "version %d.%d.%d (build %d)", VER_MAJOR, VER_MINOR, VER_RELEASE, VER_BUILD);
		setfont(50, 0, "Quicksand");
		setcolor(mainColor);
		xyprintf(251, 251, "Please Select Window Size:");
		setcolor(WHITE);
		xyprintf(250, 250, "Please Select Window Size:");
		settextjustify(LEFT_TEXT, TOP_TEXT);
		rectBUTTON scrsz[10];
		for(int i = 200; i <= 500; i += 100) {
			register int p = i / 100 - 2;
			scrsz[p].setsize(400, 50);
			scrsz[p].setbgcol(bgColor);
			scrsz[p].settxtcol(WHITE);
			scrsz[p].setfontname("Quicksand");
			scrsz[p].setfontsz(40, 0);
			scrsz[p].setlocation(50, 180 + i / 4 * 2 + p * 3);
			scrsz[p].addtext(to_string(i * 4) + " ¡Á " + to_string(i * 9 / 4));
			scrsz[p].clickEvent = [i]()->void { select = i / 100; };
			scrsz[p].setalign(CENTER_TEXT, CENTER_TEXT);
			scrsz[p].display();
		} {
			int i = 600;
			register int p = i / 100 - 2;
			scrsz[p].setsize(400, 50);
			scrsz[p].setbgcol(bgColor);
			scrsz[p].settxtcol(WHITE);
			scrsz[p].setfontname("Quicksand");
			scrsz[p].setfontsz(40, 0);
			scrsz[p].setlocation(50, 180 + i / 4 * 2 + p * 3);
			scrsz[p].addtext("Full Screen ("+to_string(GetSystemMetrics(SM_CXSCREEN))+" ¡Á "+to_string(GetSystemMetrics(SM_CYSCREEN))+")");
			scrsz[p].clickEvent = [i]()->void { select = i / 100; };
			scrsz[p].setalign(CENTER_TEXT, CENTER_TEXT);
			scrsz[p].display();
		}
		for(; is_run(); delay_fps(60)) {
			for(int i = 200; i <= 600; i += 100) {
				register int p = i / 100 - 2;
				register int k = scrsz[p].status;
				scrsz[p].detect();
				scrsz[p].display();
				if(scrsz[p].status == 2) scrsz[p].clickEvent(), changeMade = false;
			}
			if(!changeMade) break;
		}
	finishSelect:
		if(select == 6) {
			movewindow(0, 0, false);
			resizewindow(-1, -1);
			FullScreen(getHWnd());
			int w = getmaxx(), h = getmaxy();
			mapDataStore.mapSizeX = (double)(1.0 * (double)w / 1600.0);
			mapDataStore.mapSizeY = (double)(1.0 * (double)h / 900.0);
		} else {
			mapDataStore.mapSizeX = mapDataStore.mapSizeY = (double)select / 4.0;
			int nScreenWidth, nScreenHeight;
			nScreenWidth = GetSystemMetrics(SM_CXSCREEN);
			nScreenHeight = GetSystemMetrics(SM_CYSCREEN);
			initgraph(1600 * mapDataStore.mapSizeX, 900 * mapDataStore.mapSizeY, RENDER_AUTO);
		}
		setcaption("LocalGen v" VER_STRING " developed by LocalGen-dev");
	}

	void WelcomePage() {
		initWindowSize();
	WelcomePageStartLabel:;
		setbkmode(TRANSPARENT);
		setbkcolor(bgColor);
		setbkcolor_f(bgColor);
		cleardevice();
		PIMAGE zfavi = newimage();
		getimage_pngfile(zfavi, "img/favicon.png");
		imageOperation::zoomImage(zfavi, getwidth(zfavi) * 1.8 * mapDataStore.mapSizeX, getheight(zfavi) * 1.8 * mapDataStore.mapSizeY);
		putimage_withalpha(NULL, zfavi, 100 * mapDataStore.mapSizeX, 50 * mapDataStore.mapSizeY,
		                   0, 0, getwidth(zfavi), getheight(zfavi));
		settextjustify(CENTER_TEXT, TOP_TEXT);
		setfont(150 * mapDataStore.mapSizeY, 0, "Quicksand", 0, 0, FW_BOLD, 0, 0, 0);
		setcolor(WHITE);
		xyprintf(330 * mapDataStore.mapSizeX, 500 * mapDataStore.mapSizeY, "Local");
		xyprintf(330 * mapDataStore.mapSizeX, 600 * mapDataStore.mapSizeY, "generals.io");
		setfont(30, 0, "Lucida Fax");
		xyprintf(330 * mapDataStore.mapSizeX, 750 * mapDataStore.mapSizeY, "version %d.%d.%d (build %d)", VER_MAJOR, VER_MINOR, VER_RELEASE, VER_BUILD);
		rectBUTTON local, web, replay, createmap;
		rectBUTTON donate;
		local
		.setsize(375 * mapDataStore.mapSizeX, 175 * mapDataStore.mapSizeY)
		.setlocation(700 * mapDataStore.mapSizeX, 100 * mapDataStore.mapSizeY)
		.addtext("local game")
		.setalign(CENTER_TEXT, CENTER_TEXT)
		.setbgcol(WHITE)
		.settxtcol(mainColor)
		.setfontname("Quicksand")
		.setfontsz(75 * mapDataStore.mapSizeY, 0)
		.setrtcol(false, mainColor)
		.setlnwid(10 * mapDataStore.mapSizeY)
		.display();
		web
		.setsize(375 * mapDataStore.mapSizeX, 175 * mapDataStore.mapSizeY)
		.setlocation(1125 * mapDataStore.mapSizeX, 100 * mapDataStore.mapSizeY)
		.addtext("web game")
		.setalign(CENTER_TEXT, CENTER_TEXT)
		.setbgcol(WHITE)
		.settxtcol(mainColor)
		.setfontname("Quicksand")
		.setfontsz(75 * mapDataStore.mapSizeY, 0)
		.setrtcol(false, mainColor)
		.setlnwid(10 * mapDataStore.mapSizeY)
		.display();
		replay
		.setsize(375 * mapDataStore.mapSizeX, 175 * mapDataStore.mapSizeY)
		.setlocation(700 * mapDataStore.mapSizeX, 300 * mapDataStore.mapSizeY)
		.addtext("load replay")
		.setalign(CENTER_TEXT, CENTER_TEXT)
		.setbgcol(WHITE)
		.settxtcol(mainColor)
		.setfontname("Quicksand")
		.setfontsz(75 * mapDataStore.mapSizeY, 0)
		.setrtcol(false, mainColor)
		.setlnwid(10 * mapDataStore.mapSizeY)
		.display();
		createmap
		.setsize(375 * mapDataStore.mapSizeX, 175 * mapDataStore.mapSizeY)
		.setlocation(1125 * mapDataStore.mapSizeX, 300 * mapDataStore.mapSizeY)
		.addtext("create map")
		.setalign(CENTER_TEXT, CENTER_TEXT)
		.setbgcol(WHITE)
		.settxtcol(mainColor)
		.setfontname("Quicksand")
		.setfontsz(75 * mapDataStore.mapSizeY, 0)
		.setrtcol(false, mainColor)
		.setlnwid(10 * mapDataStore.mapSizeY)
		.display();
		donate
		.setsize(400 * mapDataStore.mapSizeX, 50 * mapDataStore.mapSizeY)
		.setlocation(1150 * mapDataStore.mapSizeX, 800 * mapDataStore.mapSizeY)
		.addtext("Please donate to support us!")
		.setalign(CENTER_TEXT, CENTER_TEXT)
		.setbgcol(WHITE)
		.settxtcol(mainColor)
		.setfontname("Quicksand")
		.setfontsz(40 * mapDataStore.mapSizeY, 0)
		.setrtcol(false, mainColor)
		.setlnwid(10 * mapDataStore.mapSizeY)
		.setevent([]() {
			cleardevice();
			PIMAGE donate_wc = newimage(), donate_ap = newimage();
			getimage_pngfile(donate_wc, "img/donate_wechat.png");
			getimage_pngfile(donate_ap, "img/donate_alipay.png");
			imageOperation::zoomImage(donate_wc, 600 * mapDataStore.mapSizeX, 800 * mapDataStore.mapSizeY);
			imageOperation::zoomImage(donate_ap, 600 * mapDataStore.mapSizeX, 800 * mapDataStore.mapSizeY);
			putimage(100 * mapDataStore.mapSizeX, 10 * mapDataStore.mapSizeY, donate_wc);
			putimage((100 + 800) * mapDataStore.mapSizeX, 10 * mapDataStore.mapSizeY, donate_ap);
			xyprintf(800 * mapDataStore.mapSizeX, 830 * mapDataStore.mapSizeY, "press any key to close...");
			delimage(donate_wc); delimage(donate_ap);
			flushkey();
			getkey();
		}).display();
		delay_ms(0);
		for(; is_run(); delay_fps(120)) {
			local.detect().display();
			web.detect().display();
			replay.detect().display();
			createmap.detect().display();
			donate.detect().display();
			if(local.status == 2) {
				localOptions(); break;
			}
			// if(web.status == 2) {
			// 	webOptions(); break;
			// }
			if(replay.status == 2) {
				replayPage(); exit(0); break;
			}
			// if(createmap.status == 2) {
			// 	createMapPage(); break;
			// }
			if(donate.status == 2) {
				donate.clickEvent(); goto WelcomePageStartLabel;
			}
		}
		settextjustify(LEFT_TEXT, TOP_TEXT);
	}

	void localOptions() {
		cleardevice();
		setbkmode(TRANSPARENT);
		setbkcolor(bgColor);
		setbkcolor_f(bgColor);

		/** select/import map **/

		rectBUTTON mapbut[505];
		int shiftval = 0;
		for(int i = 1; i <= mapNum; ++i) {
			mapbut[i]
			.setsize(300 * mapDataStore.mapSizeX - 3, 200 * mapDataStore.mapSizeY - 3)
			.setlocation(((i - 1) % 4 * 300) * mapDataStore.mapSizeX, ((i - 1) / 4 * 200 + shiftval) * mapDataStore.mapSizeY)
			.setbgcol(bgColor)
			.settxtcol(WHITE)
			.setalign(CENTER_TEXT,CENTER_TEXT)
			.setfontname("Quicksand")
			.setfontsz(22 * mapDataStore.mapSizeY, 0)
			// .addtext(maps[i].chiname)
			// .addtext(maps[i].engname)
			// .addtext("General Count: " + to_string(maps[i].generalcnt))
			// .addtext("Plain Count: " + to_string(maps[i].plaincnt))
			// .addtext("City Count: " + to_string(maps[i].citycnt))
			// .addtext("Mountain Count: " + to_string(maps[i].mountaincnt))
			// .addtext("Swamp Count: " + to_string(maps[i].swampcnt))
			// .addtext("Size: " + to_string(maps[i].hei) + " ¡Á " + to_string(maps[i].wid))
			.display();
		}
		sys_edit impbox;
		impbox.create(true);
		impbox.move(1250 * mapDataStore.mapSizeX, 100 * mapDataStore.mapSizeY);
		impbox.size(300 * mapDataStore.mapSizeX, 700 * mapDataStore.mapSizeY);
		impbox.setbgcolor(WHITE);
		impbox.setcolor(mainColor);
		impbox.setfont(30 * mapDataStore.mapSizeY, 0, "Quicksand");
		impbox.visible(true);
		rectBUTTON impfin;
		impfin
		.setalign(CENTER_TEXT,CENTER_TEXT)
		.setlocation(1250 * mapDataStore.mapSizeX, 825 * mapDataStore.mapSizeY)
		.setsize(300 * mapDataStore.mapSizeX, 50 * mapDataStore.mapSizeY)
		.setbgcol(WHITE)
		.settxtcol(mainColor)
		.setfontname("Quicksand")
		.setfontsz(40 * mapDataStore.mapSizeY, 0)
		.addtext("confirm and submit")
		.display();
		settextjustify(CENTER_TEXT, CENTER_TEXT);
		setcolor(WHITE);
		setfont(50 * mapDataStore.mapSizeY, 0, "Quicksand");
		delay_ms(50);
		xyprintf(1400 * mapDataStore.mapSizeX, 50 * mapDataStore.mapSizeY, "or import a map...");
		delay_ms(0);
		delay_ms(50);
		flushmouse();
		mapSelected = 0;
		mouse_msg msg;
		for(; is_run(); delay_fps(120)) {
			while(mousemsg()) {
				int id = int((msg.y - shiftval * mapDataStore.mapSizeX) / (200 * mapDataStore.mapSizeY)) * 4 + int(msg.x / (300 * mapDataStore.mapSizeX)) + 1;
				mapbut[id].status = 0;
				impfin.status = 0;
				msg = getmouse();
				shiftval += msg.wheel;
				if(shiftval > 0) shiftval = 0;
				if(shiftval < -(mapNum - 1) / 4 * 200) shiftval = -(mapNum - 1) / 4 * 200;
				if(msg.x >= 1250 * mapDataStore.mapSizeX && msg.x <= 1550 * mapDataStore.mapSizeY
				   && msg.y >= 825 * mapDataStore.mapSizeY && msg.y <= 875 * mapDataStore.mapSizeY) {
					impfin.status = 1;
					if(msg.is_left()) impfin.status = 2;
					continue;
				}
				if(msg.x < 0 || msg.x > 1200 * mapDataStore.mapSizeX || msg.y < 0 || msg.y > 900 * mapDataStore.mapSizeY) continue;
				id = int((msg.y - shiftval * mapDataStore.mapSizeX) / (200 * mapDataStore.mapSizeY)) * 4 + int(msg.x / (300 * mapDataStore.mapSizeX)) + 1;
				if(msg.is_left()) mapbut[id].status = 2;
				else mapbut[id].status = 1;
			}
			for(int i = 1; i <= mapNum; ++i) {
				int presta = mapbut[i].status;
				mapbut[i].status = 0;
				mapbut[i].cleartext().display();
				mapbut[i].status = presta;
			}
			for(int i = 1; i <= mapNum; ++i) {
				mapbut[i]
				.addtext(maps[i].chiname)
				.addtext(maps[i].engname)
				.addtext("General Count: " + (~maps[i].generalcnt?to_string(maps[i].generalcnt):"2~12"))
				.addtext("Plain Count: " + (~maps[i].plaincnt?to_string(maps[i].plaincnt):"undetermined"))
				.addtext("City Count: " + (~maps[i].citycnt?to_string(maps[i].citycnt):"undetermined"))
				.addtext("Mountain Count: " + (~maps[i].mountaincnt?to_string(maps[i].mountaincnt):"undetermined"))
				.addtext("Swamp Count: " + (~maps[i].swampcnt?to_string(maps[i].swampcnt):"undetermined"))
				.addtext("Size: " + (~maps[i].hei?to_string(maps[i].hei):"undetermined") + " ¡Á " + (~maps[i].wid?to_string(maps[i].wid):"undetermined"))
				.setlocation(((i - 1) % 4 * 300) * mapDataStore.mapSizeX, ((i - 1) / 4 * 200 + shiftval) * mapDataStore.mapSizeY)
				.display();
				if(mapbut[i].status == 2) mapSelected = i;
			}
			impfin.display();
			if(impfin.status == 2) {
				std::array<char,1000> s;
				impbox.gettext(1000, s.data());
				std::ifstream fin(s.data());
				if(!fin) {
					impfin.poptext().addtext("map not found").display();
					delay_ms(100);
					impfin.poptext().addtext("confirm and submit").display();
					impfin.status = 1;
					continue;
				} else {
					fin>>strdeZip;
					deZip();
					impbox.visible(false);
					mapSelected = 0;
					break;
				}
			}
			if(mapSelected) {
				impbox.visible(false);
				break;
			}
		}
		flushkey();

		/** game options **/

		if(mapSelected) doMapSelect(); // temporary
		else {
			importGameSettings();
			localGame(cheatCode, plCnt, stDel);
			exit(0);
			// temporary
		}
	}

	void webOptions() {
	}

	void replayPage() {
		cleardevice();
		setrendermode(RENDER_MANUAL);
		LGreplay::rreplay.initReplay();
		LGGraphics::init();
		for(int i = 1; i <= LGgame::playerCnt; ++i) LGgame::isAlive[i] = 1;
		printMap(1048575, {-1,-1});
		int smsx=0,smsy=0,midact=0;
		for(; is_run(); delay_fps(120)) {
			while(mousemsg()) {
				mouse_msg msg = getmouse();
				if(msg.is_wheel()) {
					widthPerBlock += msg.wheel / 120;
					heightPerBlock += msg.wheel / 120;
					widthPerBlock = max(widthPerBlock, 2);
					heightPerBlock = max(heightPerBlock, 2);
				}
				if(msg.is_move()) {
					if(midact == 1) {
						LGGraphics::mapDataStore.maplocX += msg.x - smsx;
						LGGraphics::mapDataStore.maplocY += msg.y - smsy;
						smsx = msg.x, smsy = msg.y;
					}
				} else if(msg.is_left()) {
					if(msg.is_down()) {
						midact = 1;
						smsx = msg.x, smsy = msg.y;
					} else midact = 0;
				}
			}
			while(kbmsg()) {
				key_msg ch = getkey();
				if(ch.msg == key_msg_up) continue;
				switch(ch.key) {
					case 27: { /*[ESC]*/
						closegraph();
						return;
					}
					case key_left: LGreplay::rreplay.preTurn(); break;
					case key_right: LGreplay::rreplay.nextTurn(); break;
				}
			}
			cleardevice();
			printMap(1048575, {-1,-1});
			LGgame::ranklist();
		}
	}

	void createMapPage() {
		cleardevice();
		setrendermode(RENDER_MANUAL);
		settextjustify(LEFT_TEXT, TOP_TEXT);
		sys_edit citynumBox,plainnumBox,savenameBox;
		rectBUTTON saveButton,cancelButton,loadButton;
		citynumBox.create();
		citynumBox.move(140,400);
		citynumBox.size(80,30);
		citynumBox.setfont(20,0,"Quicksand");
		citynumBox.setcolor(mainColor);
		citynumBox.settext("40");
		plainnumBox.create();
		plainnumBox.move(140,400);
		plainnumBox.size(80,30);
		plainnumBox.setfont(20,0,"Quicksand");
		plainnumBox.setcolor(mainColor);
		plainnumBox.settext("40");
		savenameBox.create();
		savenameBox.move(310,240);
		savenameBox.size(100,40);
		savenameBox.setfont(30,0,"Quicksand");
		savenameBox.setcolor(mainColor);
		savenameBox.settext("map");
		saveButton
		.setsize(80,40)
		.setlocation(200,290)
		.setfontname("Quicksand")
		.setfontsz(40,0)
		.setbgcol(mainColor)
		.settxtcol(WHITE)
		.setalign(CENTER_TEXT,CENTER_TEXT)
		.addtext("Save");
		cancelButton
		.setsize(80,40)
		.setlocation(300,290)
		.setfontname("Quicksand")
		.setfontsz(40,0)
		.setbgcol(mainColor)
		.settxtcol(WHITE)
		.setalign(CENTER_TEXT,CENTER_TEXT)
		.addtext("Cancel");
		loadButton
		.setsize(80,40)
		.setlocation(200,290)
		.setfontname("Quicksand")
		.setfontsz(40,0)
		.setbgcol(mainColor)
		.settxtcol(WHITE)
		.setalign(CENTER_TEXT,CENTER_TEXT)
		.addtext("Load");
		LGGraphics::init();
		mapW=mapH=10;
		for(int i=1;i<=mapH;++i)
			for(int j=1;j<=mapW;++j) gameMap[i][j]={0,0,0,0};
		printMap(1048575,{-1,-1});
		createOptions(0);
		setfillcolor(mainColor);
		int smsx=0,smsy=0,midact=0,type=2,citynum=40,plainnum=40;
		bool moved=false,saved=false;
		std::chrono::steady_clock::duration prsttm;
		for(;is_run();delay_fps(120)){
			while(mousemsg()) {
				mouse_msg msg = getmouse();
				if(msg.is_wheel()) {
					widthPerBlock += msg.wheel / 120;
					heightPerBlock += msg.wheel / 120;
					widthPerBlock = max(widthPerBlock, 2);
					heightPerBlock = max(heightPerBlock, 2);
				}
				if(msg.is_move()) {
					if(midact == 1) {
						LGGraphics::mapDataStore.maplocX += msg.x - smsx;
						LGGraphics::mapDataStore.maplocY += msg.y - smsy;
						smsx = msg.x, smsy = msg.y; moved = true;
					}
				} else if(msg.is_left()) {
					if(msg.is_down()) {
						prsttm = std::chrono::steady_clock::now().time_since_epoch();
						midact = 1;
						smsx = msg.x, smsy = msg.y;
						moved = false;
					} else {
						midact = 0;
						std::chrono::steady_clock::duration now = std::chrono::steady_clock::now().time_since_epoch();
						if(!moved && now - prsttm < 200ms) {
							if(msg.x<40&&msg.y>=100&&msg.y<340){
								type=(msg.y-100)/40;
							}else if(msg.x>=160&&msg.x<300&&msg.y>=400&&msg.y<440){
								settextjustify(CENTER_TEXT, CENTER_TEXT);
								setfillcolor(WHITE);
								setcolor(mainColor);
								bar(160,180,420,340);
								setfont(40,0,"Quicksand");
								xyprintf(290,210,"Save map");
								xyprintf(235,260,"Map name:");
								savenameBox.visible(true);
								setfillcolor(mainColor);
								saveButton.display();
								cancelButton.display();
								delay_ms(0);
								for(;is_run();delay_fps(120)){
									saveButton.detect().display();
									cancelButton.detect().display();
									if(saveButton.status==2){
										char s[100];
										savenameBox.gettext(sizeof(s),s);
										string ss=s;
										Zip();
										freopen(("maps/"+ss+".lg").c_str(),"w",stdout);
										printf("%s",strZip);
										fclose(stdout);
										saved=true;
										return;
									}
									if(cancelButton.status==2) break;
								}
								savenameBox.visible(false);
							}else if(msg.x>=320&&msg.x<460&&msg.y>=400&&msg.y<440){
								settextjustify(CENTER_TEXT, CENTER_TEXT);
								setfillcolor(WHITE);
								setcolor(mainColor);
								bar(160,180,420,340);
								setfont(40,0,"Quicksand");
								xyprintf(290,210,"Load map");
								xyprintf(235,260,"Map name:");
								savenameBox.visible(true);
								setfillcolor(mainColor);
								loadButton.display();
								cancelButton.display();
								delay_ms(0);
								for(;is_run();delay_fps(120)){
									loadButton.detect().display();
									cancelButton.detect().display();
									if(loadButton.status==2){
										char s[100];
										savenameBox.gettext(sizeof(s),s);
										string ss=s;
										FILE* file;
										file=fopen(("maps/"+ss+".lg").c_str(),"r");
										if(!file){
											MessageBoxA(nullptr,"Map not found!","Local Generals",MB_OK);
											continue;
										}
										fread(strdeZip,1,LEN_ZIP,file);
										deZip();
										break;
									}
									if(cancelButton.status==2) break;
								}
								savenameBox.visible(false);
							}else if(msg.x>=55&&msg.x<95&&msg.y<40){
								if(mapH>1) --mapH;
							}else if(msg.x>=245&&msg.x<285&&msg.y<40){
								if(mapH<100){
									++mapH;
									for(int i=1;i<=mapW;++i) gameMap[mapH][i]={0,0,0,0};
								}
							}else if(msg.x>=315&&msg.x<355&&msg.y<40){
								if(mapW>1) --mapW;
							}else if(msg.x>=505&&msg.x<545&&msg.y<40){
								if(mapW<100){
									++mapW;
									for(int i=1;i<=mapH;++i) gameMap[i][mapW]={0,0,0,0};
								}
							}else if(msg.x >= LGGraphics::mapDataStore.maplocX &&
							   msg.y >= LGGraphics::mapDataStore.maplocY &&
							   msg.x <= LGGraphics::mapDataStore.maplocX + widthPerBlock * mapW &&
							   msg.y <= LGGraphics::mapDataStore.maplocY + heightPerBlock * mapH) {
								int lin = (msg.y + heightPerBlock - 1 - LGGraphics::mapDataStore.maplocY) / heightPerBlock;
								int col = (msg.x + widthPerBlock - 1 - LGGraphics::mapDataStore.maplocX) / widthPerBlock;
								switch(type){
									case 0:
										gameMap[lin][col].team=0;
										gameMap[lin][col].type=0;
										gameMap[lin][col].army=0;
										break;
									case 1:
										gameMap[lin][col].team=0;
										gameMap[lin][col].type=1;
										gameMap[lin][col].army=0;
										break;
									case 2:
										gameMap[lin][col].team=0;
										gameMap[lin][col].type=2;
										gameMap[lin][col].army=0;
										break;
									case 3:
										gameMap[lin][col].team=0;
										gameMap[lin][col].type=3;
										gameMap[lin][col].army=0;
										break;
									case 4:
										gameMap[lin][col].team=0;
										gameMap[lin][col].type=4;
										gameMap[lin][col].army=citynum;
										break;
									case 5:
										gameMap[lin][col].team=0;
										gameMap[lin][col].type=0;
										gameMap[lin][col].army=plainnum;
										break;
								}
							}
						}
					}
				}
			}
			cleardevice();
			printMap(1048575, {-1,-1});
			createOptions(type);
			if(type==4){
				citynumBox.visible(true);
				setfillcolor(WHITE);
				bar(60,400,140,430);
				setcolor(mainColor);
				setfont(20,0,"Quicksand");
				xyprintf(60,400,"City count:");
				char s[10];
				citynumBox.gettext(sizeof(s),s);
				sscanf(s,"%d",&citynum);
			}else citynumBox.visible(false);
			if(type==5){
				plainnumBox.visible(true);
				setfillcolor(WHITE);
				bar(60,400,140,430);
				setcolor(mainColor);
				setfont(20,0,"Quicksand");
				xyprintf(60,400,"Army count:");
				char s[10];
				plainnumBox.gettext(sizeof(s),s);
				sscanf(s,"%d",&plainnum);
			}else plainnumBox.visible(false);
			settextjustify(CENTER_TEXT, CENTER_TEXT);
			setfillcolor(WHITE);
			setcolor(mainColor);
			setfont(40,0,"Quicksand");
			bar(100,0,240,40);
			bar(55,0,95,40);
			bar(245,0,285,40);
			xyprintf(170,20,"Height: %d",mapH);
			xyprintf(75,20,"-");
			xyprintf(265,20,"+");
			bar(360,0,500,40);
			bar(315,0,355,40);
			bar(505,0,545,40);
			xyprintf(430,20,"Width: %d",mapW);
			xyprintf(335,20,"-");
			xyprintf(525,20,"+");
			bar(160,400,300,440);
			xyprintf(230,420,"Save map");
			bar(320,400,460,440);
			xyprintf(390,420,"Load map");
			settextjustify(LEFT_TEXT, TOP_TEXT);
		}
	}

	void doMapSelect() {
		cleardevice();
		setrendermode(RENDER_AUTO);
		// setfillcolor(WHITE);
		// bar(5 * mapDataStore.mapSizeX, 5 * mapDataStore.mapSizeY,
		//     505 * mapDataStore.mapSizeX, 305 * mapDataStore.mapSizeY);
		setcolor(WHITE);
		setlinewidth(5 * mapDataStore.mapSizeY);
		rectangle(5 * mapDataStore.mapSizeX, 5 * mapDataStore.mapSizeY,
		          505 * mapDataStore.mapSizeX, 305 * mapDataStore.mapSizeY);
		setlinewidth(1);
		setcolor(mainColor);
		setfont(40 * mapDataStore.mapSizeY, 0, "Quicksand", 0, 0, FW_BOLD, false, false, false);
		settextjustify(CENTER_TEXT,TOP_TEXT);
		xyprintf(255 * mapDataStore.mapSizeX, 5 * mapDataStore.mapSizeY,
		         "%s", maps[mapSelected].chiname.c_str());
		xyprintf(255 * mapDataStore.mapSizeX, 45 * mapDataStore.mapSizeY,
		         "%s", maps[mapSelected].engname.c_str());
		setcolor(WHITE);
		setfont(30 * mapDataStore.mapSizeY, 0, "Quicksand");
		xyprintf(255 * mapDataStore.mapSizeX, 85 * mapDataStore.mapSizeY,
		         "Author: %s", maps[mapSelected].auth.c_str());
		xyprintf(255 * mapDataStore.mapSizeX, 115 * mapDataStore.mapSizeY,
		         "Size: %d ¡Á %d", maps[mapSelected].hei, maps[mapSelected].wid);
		xyprintf(255 * mapDataStore.mapSizeX, 145 * mapDataStore.mapSizeY,
		         "General Count: %d", maps[mapSelected].generalcnt);
		xyprintf(255 * mapDataStore.mapSizeX, 175 * mapDataStore.mapSizeY,
		         "Plain Count: %d", maps[mapSelected].plaincnt);
		xyprintf(255 * mapDataStore.mapSizeX, 205 * mapDataStore.mapSizeY,
		         "City Count: %d", maps[mapSelected].citycnt);
		xyprintf(255 * mapDataStore.mapSizeX, 235 * mapDataStore.mapSizeY,
		         "Mountain Count: %d", maps[mapSelected].mountaincnt);
		xyprintf(255 * mapDataStore.mapSizeX, 265 * mapDataStore.mapSizeY,
		         "Swamp Count: %d", maps[mapSelected].swampcnt);
		sys_edit heiinput,widinput,amninput,amxinput;
		int height,width,armymin,armymax;
		heiinput.create();
		widinput.create();
		amninput.create();
		amxinput.create();
		if(mapSelected < 6) {
			setcolor(WHITE);
			setfont(40 * mapDataStore.mapSizeY, 0, "Quicksand");
			settextjustify(RIGHT_TEXT,TOP_TEXT);
			xyprintf(800 * mapDataStore.mapSizeX, 5 * mapDataStore.mapSizeY, "Input Height (<=100):");
			xyprintf(800 * mapDataStore.mapSizeX, 45 * mapDataStore.mapSizeY, "Input Width (<=100):");
			if(mapSelected == 3) {
				xyprintf(800 * mapDataStore.mapSizeX, 85 * mapDataStore.mapSizeY, "Input MINIMUM Army:");
				xyprintf(800 * mapDataStore.mapSizeX, 125 * mapDataStore.mapSizeY, "Input MAXIMUM Army:");
			}
			heiinput.move(810 * mapDataStore.mapSizeX, 6 * mapDataStore.mapSizeY);
			heiinput.size(200 * mapDataStore.mapSizeX, 38 * mapDataStore.mapSizeY);
			heiinput.setbgcolor(WHITE);
			heiinput.setcolor(mainColor);
			heiinput.setfont(35 * mapDataStore.mapSizeY, 0, "Quicksand");
			widinput.move(810 * mapDataStore.mapSizeX, 46 * mapDataStore.mapSizeY);
			widinput.size(200 * mapDataStore.mapSizeX, 38 * mapDataStore.mapSizeY);
			widinput.setbgcolor(WHITE);
			widinput.setcolor(mainColor);
			widinput.setfont(35 * mapDataStore.mapSizeY, 0, "Quicksand");
			if(mapSelected == 3) {
				amninput.move(810 * mapDataStore.mapSizeX, 86 * mapDataStore.mapSizeY);
				amninput.size(200 * mapDataStore.mapSizeX, 38 * mapDataStore.mapSizeY);
				amninput.setbgcolor(WHITE);
				amninput.setcolor(mainColor);
				amninput.setfont(35 * mapDataStore.mapSizeY, 0, "Quicksand");
				amxinput.move(810 * mapDataStore.mapSizeX, 126 * mapDataStore.mapSizeY);
				amxinput.size(200 * mapDataStore.mapSizeX, 38 * mapDataStore.mapSizeY);
				amxinput.setbgcolor(WHITE);
				amxinput.setcolor(mainColor);
				amxinput.setfont(35 * mapDataStore.mapSizeY, 0, "Quicksand");
			}
			heiinput.visible(true);
			widinput.visible(true);
			if(mapSelected == 3) {
				amninput.visible(true);
				amxinput.visible(true);
			}
			rectBUTTON heib,widb,amnb,amxb,endb;
			heib
			.setlocation(1020 * mapDataStore.mapSizeX, 6 * mapDataStore.mapSizeY)
			.setsize(100 * mapDataStore.mapSizeX, 38 * mapDataStore.mapSizeY)
			.setbgcol(WHITE)
			.settxtcol(mainColor)
			.setalign(CENTER_TEXT,CENTER_TEXT)
			.setfontsz(35 * mapDataStore.mapSizeY, 0)
			.setfontname("Quicksand")
			.addtext("confirm");
			widb
			.setlocation(1020 * mapDataStore.mapSizeX, 46 * mapDataStore.mapSizeY)
			.setsize(100 * mapDataStore.mapSizeX, 38 * mapDataStore.mapSizeY)
			.setbgcol(WHITE)
			.settxtcol(mainColor)
			.setalign(CENTER_TEXT,CENTER_TEXT)
			.setfontsz(35 * mapDataStore.mapSizeY, 0)
			.setfontname("Quicksand")
			.addtext("confirm");
			if(mapSelected == 3) {
				amnb
				.setlocation(1020 * mapDataStore.mapSizeX, 86 * mapDataStore.mapSizeY)
				.setsize(100 * mapDataStore.mapSizeX, 38 * mapDataStore.mapSizeY)
				.setbgcol(WHITE)
				.settxtcol(mainColor)
				.setalign(CENTER_TEXT,CENTER_TEXT)
				.setfontsz(35 * mapDataStore.mapSizeY, 0)
				.setfontname("Quicksand")
				.addtext("confirm");
				amxb
				.setlocation(1020 * mapDataStore.mapSizeX, 126 * mapDataStore.mapSizeY)
				.setsize(100 * mapDataStore.mapSizeX, 38 * mapDataStore.mapSizeY)
				.setbgcol(WHITE)
				.settxtcol(mainColor)
				.setalign(CENTER_TEXT,CENTER_TEXT)
				.setfontsz(35 * mapDataStore.mapSizeY, 0)
				.setfontname("Quicksand")
				.addtext("confirm");
			}
			endb
			.setlocation(810 * mapDataStore.mapSizeX, 166 * mapDataStore.mapSizeY)
			.setsize(310 * mapDataStore.mapSizeX, 38 * mapDataStore.mapSizeY)
			.setbgcol(WHITE)
			.settxtcol(mainColor)
			.setalign(CENTER_TEXT,CENTER_TEXT)
			.setfontsz(35 * mapDataStore.mapSizeY, 0)
			.setfontname("Quicksand")
			.addtext("end input");
			heib.display();
			widb.display();
			if(mapSelected == 3) {
				amnb.display();
				amxb.display();
			}
			endb.display();
			bool hb,wb,nb,xb;
			hb=wb=nb=xb=false;
			for(; is_run(); delay_fps(60)) {
				heib.detect().display();
				widb.detect().display();
				if(mapSelected == 3) {
					amnb.detect().display();
					amxb.detect().display();
				}
				endb.detect().display();
				if(heib.status == 2) {
					char s[55];
					heiinput.gettext(sizeof(s), s);
					int p = sscanf(s, "%d", &height);
					if(p!=1) {
						heib.poptext().addtext("invalid").display();
						delay_ms(100);
						heib.poptext().addtext("confirm").display();
					} else hb=true;
				}
				if(widb.status == 2) {
					char s[55];
					widinput.gettext(sizeof(s), s);
					int p = sscanf(s, "%d", &width);
					if(p!=1) {
						widb.poptext().addtext("invalid").display();
						delay_ms(100);
						widb.poptext().addtext("confirm").display();
					} else wb=true;
				}
				if(amnb.status == 2) {
					char s[55];
					amninput.gettext(sizeof(s), s);
					int p = sscanf(s, "%d", &armymin);
					if(p!=1) {
						amnb.poptext().addtext("invalid").display();
						delay_ms(100);
						amnb.poptext().addtext("confirm").display();
					} else nb=true;
				}
				if(amxb.status == 2) {
					char s[55];
					amxinput.gettext(sizeof(s), s);
					int p = sscanf(s, "%d", &armymax);
					if(p!=1) {
						amxb.poptext().addtext("invalid").display();
						delay_ms(100);
						amxb.poptext().addtext("confirm").display();
					} else xb=true;
				}
				if(endb.status == 2) {
					if(hb&&wb&&(mapSelected==3?(nb&&xb):1)) {
						endb.status = 0;
						endb.display();
						break;
					} else {
						endb.poptext().addtext("not finished").display();
						delay_ms(100);
						endb.poptext().addtext("end input").display();
					}
				}
			}
		}
		heiinput.setreadonly(true);
		widinput.setreadonly(true);
		amninput.setreadonly(true);
		amxinput.setreadonly(true);
		settextjustify(LEFT_TEXT,TOP_TEXT);
		importGameSettings();
		if(mapSelected < 6) {
			switch(mapSelected) {
				case 1: createRandomMap(height,width); break;
				case 2: createStandardMap(height,width); break;
				case 3: createFullCityMap(height,width,armymin,armymax,plCnt); break;
				case 4: createFullSwampMap(height,width,plCnt); break;
				case 5: createFullPlainMap(height,width,plCnt); break;
			}
		} else readMap(mapSelected);
		heiinput.visible(false);
		widinput.visible(false);
		amninput.visible(false);
		amxinput.visible(false);
		localGame(cheatCode, plCnt, stDel);
		exit(0);
	}

	void importGameSettings() {
		sys_edit speedBox;
		rectBUTTON speedSubmit;
		rectBUTTON plCntBox[15]; /* 2~12 */
		rectBUTTON checkBox[15]; /* 2~12 */
		rectBUTTON checkOA;
		rectBUTTON gameBox;
		plCnt = 2; stDel = 1;
		settextjustify(CENTER_TEXT, CENTER_TEXT);
		setfont(50 * mapDataStore.mapSizeY, 0, "Quicksand");
		xyprintf(250 * mapDataStore.mapSizeX, 350 * mapDataStore.mapSizeY,
		         "Choose Player Count");
		setlinewidth(1);
		for(int i=2; i<=12; ++i) {
			int rowNum = (i - 2) / 4;
			int colNum = (i - 2) % 4;
			rectangle(55 * mapDataStore.mapSizeX + 100 * mapDataStore.mapSizeX * colNum,
			          400 * mapDataStore.mapSizeY + 100 * mapDataStore.mapSizeY * rowNum,
			          155 * mapDataStore.mapSizeX + 100 * mapDataStore.mapSizeX * colNum,
			          500 * mapDataStore.mapSizeY + 100 * mapDataStore.mapSizeY * rowNum);
			plCntBox[i]
			.setsize(100 * mapDataStore.mapSizeX - 1*2, 100 * mapDataStore.mapSizeY - 1*2)
			.setlocation(55 * mapDataStore.mapSizeX + 100 * mapDataStore.mapSizeX * colNum + 1,
			             400 * mapDataStore.mapSizeY + 100 * mapDataStore.mapSizeY * rowNum + 1)
			.setalign(CENTER_TEXT, CENTER_TEXT)
			.setfontname("Quicksand")
			.setfontsz(50 * mapDataStore.mapSizeY, 0)
			.setbgcol(bgColor)
			.settxtcol(WHITE)
			.addtext(to_string(i));
			plCntBox[i].floatshadow = false;
			plCntBox[i].display();
		}
		xyprintf(250 * mapDataStore.mapSizeX, 750 * mapDataStore.mapSizeY,
		         "Current Count: %d", plCnt);
		xyprintf(800 * mapDataStore.mapSizeX, 350 * mapDataStore.mapSizeY,
		         "Input Game Speed");
		xyprintf(800 * mapDataStore.mapSizeX, 400 * mapDataStore.mapSizeY,
		         "(integer between 1 and 10000)");
		speedBox.create();
		speedBox.move(575 * mapDataStore.mapSizeX, 450 * mapDataStore.mapSizeY);
		speedBox.size(300 * mapDataStore.mapSizeX, 50 * mapDataStore.mapSizeY);
		speedBox.setfont(50 * mapDataStore.mapSizeY, 0, "Quicksand");
		speedBox.setcolor(mainColor);
		speedBox.visible(true);
		speedSubmit
		.setsize(150 * mapDataStore.mapSizeX, 50 * mapDataStore.mapSizeY)
		.setlocation(900 * mapDataStore.mapSizeX, 450 * mapDataStore.mapSizeY)
		.setfontname("Quicksand")
		.setfontsz(50 * mapDataStore.mapSizeY, 0)
		.setbgcol(WHITE)
		.settxtcol(mainColor)
		.setalign(CENTER_TEXT,CENTER_TEXT)
		.addtext("submit");
		speedSubmit.display();
		xyprintf(800 * mapDataStore.mapSizeX, 550 * mapDataStore.mapSizeY,
		         "Current Speed: %d", stDel);
		xyprintf(1350 * mapDataStore.mapSizeX, 350 * mapDataStore.mapSizeY,
		         "Choose Visible Players:");
		cheatCode = 0b0000000000010;
		for(int i=1; i<=12; ++i) {
			int rowNum = (i - 1) / 4;
			int colNum = (i - 1) % 4;
			checkBox[i]
			.addtext(playerInfo[i].name)
			.setalign(CENTER_TEXT, CENTER_TEXT)
			.setfontname("Quicksand")
			.setfontsz(40 * mapDataStore.mapSizeY, 0)
			.setsize(100 * mapDataStore.mapSizeX - 1*2, 100 * mapDataStore.mapSizeY - 1*2)
			.setlocation(1150 * mapDataStore.mapSizeX + 100 * mapDataStore.mapSizeX * colNum + 1,
			             400 * mapDataStore.mapSizeY + 100 * mapDataStore.mapSizeY * rowNum + 1);
			checkBox[i].floatshadow = false;
			checkBox[i].txtshadow = false;
		}
		checkOA
		.setsize(400 * mapDataStore.mapSizeX - 1*2, 100 * mapDataStore.mapSizeY - 1*2)
		.setlocation(1150 * mapDataStore.mapSizeX + 1, 700 * mapDataStore.mapSizeY + 1)
		.addtext("Overall Select")
		.setalign(CENTER_TEXT, CENTER_TEXT)
		.setfontname("Quicksand")
		.setfontsz(50 * mapDataStore.mapSizeY, 0);
		checkOA.floatshadow = false;
		checkOA.txtshadow = false;
		checkOA.display();
		gameBox
		.setsize(200 * mapDataStore.mapSizeX, 100 * mapDataStore.mapSizeY)
		.setlocation(1150 * mapDataStore.mapSizeX, 100 * mapDataStore.mapSizeY)
		.addtext("START")
		.setalign(CENTER_TEXT, CENTER_TEXT)
		.setfontname("Quicksand")
		.setfontsz(50 * mapDataStore.mapSizeY, 0)
		.setbgcol(WHITE)
		.settxtcol(mainColor);
		delay_ms(0);
		for(; is_run(); delay_fps(120)) {
			for(int i=2; i<=12; ++i) {
				plCntBox[i].detect().display();
				if(plCntBox[i].status == 2) plCnt = i;
			}
			plCnt = min(plCnt, maps[mapSelected].generalcnt + maps[mapSelected].plaincnt);
			setfillcolor(bgColor);
			bar(55 * mapDataStore.mapSizeX, 701 * mapDataStore.mapSizeY,
			    455 * mapDataStore.mapSizeX, 800 * mapDataStore.mapSizeY);
			xyprintf(250 * mapDataStore.mapSizeX, 750 * mapDataStore.mapSizeY,
			         "Current Count: %d", plCnt);
			speedSubmit.detect().display();
			if(speedSubmit.status == 2) {
				char s[105];
				speedBox.gettext(sizeof(s),s);
				int t=stDel;
				int f=sscanf(s,"%d",&stDel);
				if(f!=1) stDel=t;
				if(stDel<=0||stDel>10000) stDel=t;
			}
			bar(600 * mapDataStore.mapSizeX, 501 * mapDataStore.mapSizeY,
			    1050 * mapDataStore.mapSizeX, 600 * mapDataStore.mapSizeY);
			xyprintf(800 * mapDataStore.mapSizeX, 550 * mapDataStore.mapSizeY,
			         "Current Speed: %d", stDel);
			setfillcolor(bgColor);
			bar(1150 * mapDataStore.mapSizeX, 400 * mapDataStore.mapSizeY,
			    1550 * mapDataStore.mapSizeX, 800 * mapDataStore.mapSizeY);
			xyprintf(1350 * mapDataStore.mapSizeX, 350 * mapDataStore.mapSizeY,
			         "Choose Visible Players:");
			for(int i=1; i<=plCnt; ++i) {
				if(cheatCode>>i&1) {
					checkBox[i]
					.setbgcol(playerInfo[i].color)
					.settxtcol(WHITE);;
				} else {
					checkBox[i]
					.setbgcol(bgColor)
					.settxtcol(playerInfo[i].color);;
				}
				int rowNum = (i - 1) / 4;
				int colNum = (i - 1) % 4;
				rectangle(1150 * mapDataStore.mapSizeX + 100 * mapDataStore.mapSizeX * colNum,
				          400 * mapDataStore.mapSizeY + 100 * mapDataStore.mapSizeY * rowNum,
				          1250 * mapDataStore.mapSizeX + 100 * mapDataStore.mapSizeX * colNum,
				          500 * mapDataStore.mapSizeY + 100 * mapDataStore.mapSizeY * rowNum);
				checkBox[i].detect().display();
				if(checkBox[i].status == 2) {
					cheatCode &= (((1<<plCnt)-1)<<1);
					cheatCode ^= (1<<i);
				}
			}
			if(cheatCode == 1048575) {
				checkOA
				.setbgcol(mainColor)
				.settxtcol(WHITE);
			} else {
				checkOA
				.setbgcol(bgColor)
				.settxtcol(mainColor);
			}
			rectangle(1150 * mapDataStore.mapSizeX, 700 * mapDataStore.mapSizeY,
			          1550 * mapDataStore.mapSizeX, 800 * mapDataStore.mapSizeY);
			checkOA.detect().display();
			if(checkOA.status == 2) {
				if(cheatCode != 1048575) cheatCode = 1048575;
				else cheatCode = 0b0000000000010;
			}
			gameBox.detect().display();
			if(gameBox.status == 2) return;
		}
	}

	void init() {
		heightPerBlock = 22 * mapDataStore.mapSizeY;
		widthPerBlock = 22 * mapDataStore.mapSizeX;
		heightPerBlock = widthPerBlock = min(heightPerBlock, widthPerBlock);
		mapDataStore.widthPerBlock = widthPerBlock;
		mapDataStore.heightPerBlock = heightPerBlock;
		setbkmode(TRANSPARENT);
		pimg[1] = newimage();
		getimage(pimg[1], "img/city.png");
		// imageOperation::zoomImage(pimg[1], mapDataStore.heightPerBlock, mapDataStore.widthPerBlock);
		pimg[2] = newimage();
		getimage(pimg[2], "img/crown.png");
		// imageOperation::zoomImage(pimg[2], mapDataStore.heightPerBlock, mapDataStore.widthPerBlock);
		pimg[3] = newimage();
		getimage(pimg[3], "img/mountain.png");
		// imageOperation::zoomImage(pimg[3], mapDataStore.heightPerBlock, mapDataStore.widthPerBlock);
		pimg[4] = newimage();
		getimage(pimg[4], "img/swamp.png");
		// imageOperation::zoomImage(pimg[4], mapDataStore.heightPerBlock, mapDataStore.widthPerBlock);
		pimg[5] = newimage();
		getimage(pimg[5], "img/obstacle.png");
		// imageOperation::zoomImage(pimg[5], mapDataStore.heightPerBlock, mapDataStore.widthPerBlock);
		pimg[6] = newimage();
		getimage(pimg[6], "img/currentOn.png");
		// imageOperation::zoomImage(pimg[6], mapDataStore.heightPerBlock, mapDataStore.widthPerBlock);
		// for(int i = 1; i <= 6; i++) ege_enable_aa(true, pimg[i]);
		// ege_enable_aa(true);
		setbkcolor(0xff222222);
		setbkcolor_f(0xff222222);
		cleardevice();
	}
}

inline int getHeightPerBlock() { return LGGraphics::mapDataStore.heightPerBlock; }

inline int getWidthPerBlock() { return LGGraphics::mapDataStore.widthPerBlock; }

#endif // __LGGRAPHICS_HPP__
