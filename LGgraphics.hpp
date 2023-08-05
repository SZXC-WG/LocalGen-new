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
			scrsz[p].size(400, 50);
			scrsz[p].bgcolor(bgColor);
			scrsz[p].textcolor(WHITE);
			scrsz[p].fontname("Quicksand");
			scrsz[p].fontsize(40, 0);
			scrsz[p].move(50, 180 + i / 4 * 2 + p * 3);
			scrsz[p].addtext(to_wstring(i * 4) + L" × " + to_wstring(i * 9 / 4));
			scrsz[p].clickEvent = [i]()->void { select = i / 100; };
			scrsz[p].textalign(CENTER_TEXT, CENTER_TEXT);
			scrsz[p].display();
		} {
			int i = 600;
			register int p = i / 100 - 2;
			scrsz[p].size(400, 50);
			scrsz[p].bgcolor(bgColor);
			scrsz[p].textcolor(WHITE);
			scrsz[p].fontname("Quicksand");
			scrsz[p].fontsize(40, 0);
			scrsz[p].move(50, 180 + i / 4 * 2 + p * 3);
			scrsz[p].addtext(L"Full Screen ("+to_wstring(GetSystemMetrics(SM_CXSCREEN))+L" x "+to_wstring(GetSystemMetrics(SM_CYSCREEN))+L")");
			scrsz[p].clickEvent = [i]()->void { select = i / 100; };
			scrsz[p].textalign(CENTER_TEXT, CENTER_TEXT);
			scrsz[p].display();
		}
		for(; is_run(); delay_fps(60)) {
			for(int i = 200; i <= 600; i += 100) {
				register int p = i / 100 - 2;
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

	void initname() {
		cleardevice();
		int scrw=mapDataStore.mapSizeX*1600,scrh=mapDataStore.mapSizeY*900;
		sys_edit nameBox;
		rectBUTTON submitButton;
		setfillcolor(WHITE);
		bar(scrw/2-120,scrh/2-80,scrw/2+120,scrh/2+80);
		setcolor(BLACK);
		setfont(30,0,"Quicksand");
		settextjustify(CENTER_TEXT,CENTER_TEXT);
		xyprintf(scrw/2,scrh/2-60,"Enter your username:");
		nameBox.create();
		nameBox.size(220,40);
		nameBox.move(scrw/2-110,scrh/2-40);
		nameBox.setfont(30,0,"Quicksand");
		nameBox.setcolor(mainColor);
		nameBox.settext("Anonymous");
		nameBox.visible(true);
		submitButton
		.size(100,40)
		.move(scrw/2-50,scrh/2+30)
		.fontname("Quicksand")
		.fontsize(40,0)
		.bgcolor(mainColor)
		.textcolor(WHITE)
		.textalign(CENTER_TEXT,CENTER_TEXT)
		.addtext(L"Submit");
		for(; is_run(); delay_fps(120)) {
			submitButton.detect().display();
			if(submitButton.status==2) {
				char s[100];
				nameBox.gettext(sizeof(s),s);
				username=s;
				if(checkValidUsername(username)) return;
				else {
					setfillcolor(errorColor);
					bar(scrw/2-110,scrh/2+5,scrw/2+110,scrh/2+25);
					setcolor(BLACK);
					setfont(15,0,"Quicksand");
					xyprintf(scrw/2,scrh/2+15,"Username must be between 3 and 16 letters.");
				}
			}
		}
	}

	void WelcomePage() {
		initWindowSize();
		init();
	WelcomePageStartLabel:;
		setbkmode(TRANSPARENT);
		setbkcolor(bgColor);
		setbkcolor_f(bgColor);
		// initname();
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
		circBUTTON settings,repo;
		local
		.size(600 * mapDataStore.mapSizeX, 100 * mapDataStore.mapSizeY)
		.move(700 * mapDataStore.mapSizeX, 100 * mapDataStore.mapSizeY)
		.addtext(L"local game")
		.textalign(CENTER_TEXT, CENTER_TEXT)
		.bgcolor(WHITE)
		.textcolor(mainColor)
		.fontname("Quicksand")
		.fontsize(75 * mapDataStore.mapSizeY, 0)
		.framecolor(false, mainColor)
		.frame(10 * mapDataStore.mapSizeY)
		.display();
		web
		.size(600 * mapDataStore.mapSizeX, 100 * mapDataStore.mapSizeY)
		.move(700 * mapDataStore.mapSizeX, 225 * mapDataStore.mapSizeY)
		.addtext(L"web game (beta)")
		.textalign(CENTER_TEXT, CENTER_TEXT)
		.bgcolor(WHITE)
		.textcolor(mainColor)
		.fontname("Quicksand")
		.fontsize(75 * mapDataStore.mapSizeY, 0)
		.framecolor(false, mainColor)
		.frame(10 * mapDataStore.mapSizeY)
		.display();
		replay
		.size(600 * mapDataStore.mapSizeX, 100 * mapDataStore.mapSizeY)
		.move(700 * mapDataStore.mapSizeX, 350 * mapDataStore.mapSizeY)
		.addtext(L"load replay (beta)")
		.textalign(CENTER_TEXT, CENTER_TEXT)
		.bgcolor(WHITE)
		.textcolor(mainColor)
		.fontname("Quicksand")
		.fontsize(75 * mapDataStore.mapSizeY, 0)
		.framecolor(false, mainColor)
		.frame(10 * mapDataStore.mapSizeY)
		.display();
		createmap
		.size(600 * mapDataStore.mapSizeX, 100 * mapDataStore.mapSizeY)
		.move(700 * mapDataStore.mapSizeX, 475 * mapDataStore.mapSizeY)
		.addtext(L"create map (beta)")
		.textalign(CENTER_TEXT, CENTER_TEXT)
		.bgcolor(WHITE)
		.textcolor(mainColor)
		.fontname("Quicksand")
		.fontsize(75 * mapDataStore.mapSizeY, 0)
		.framecolor(false, mainColor)
		.frame(10 * mapDataStore.mapSizeY)
		.display();
		donate
		.size(400 * mapDataStore.mapSizeX, 50 * mapDataStore.mapSizeY)
		.move(1150 * mapDataStore.mapSizeX, 750 * mapDataStore.mapSizeY)
		.addtext(L"Please donate to support us!")
		.textalign(CENTER_TEXT, CENTER_TEXT)
		.bgcolor(WHITE)
		.textcolor(mainColor)
		.fontname("Quicksand")
		.fontsize(40 * mapDataStore.mapSizeY, 0)
		.framecolor(false, mainColor)
		.frame(10 * mapDataStore.mapSizeY)
		.event([]() {
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
		settings
		.radius(50 * min(mapDataStore.mapSizeX,mapDataStore.mapSizeY))
		.move(1450 * mapDataStore.mapSizeX, 150 * mapDataStore.mapSizeY)
		.bgimage(pimg[9]) // settings (options.png)
		.bgsize(75 * mapDataStore.mapSizeX, 75 * mapDataStore.mapSizeY)
		.textalign(CENTER_TEXT, CENTER_TEXT)
		.bgcolor(WHITE)
		.framecolor(false, mainColor)
		.frame(10 * mapDataStore.mapSizeY)
		.enableButtonShadow = false; settings
		.display();
		repo
		.radius(50 * min(mapDataStore.mapSizeX,mapDataStore.mapSizeY))
		.move(1450 * mapDataStore.mapSizeX, 525 * mapDataStore.mapSizeY)
		.bgimage(pimg[10]) // github repo (github.png)
		.bgsize(75 * mapDataStore.mapSizeX, 75 * mapDataStore.mapSizeY)
		.textalign(CENTER_TEXT, CENTER_TEXT)
		.bgcolor(WHITE)
		.framecolor(false, mainColor)
		.frame(10 * mapDataStore.mapSizeY)
		.event([]()->void{system("start http://github.com/LocalGen-dev/LocalGen-new");})
		.enableButtonShadow = false; settings
		.display();
		delay_ms(0);
		for(; is_run(); delay_fps(120)) {
			local.detect().display();
			web.detect().display();
			replay.detect().display();
			createmap.detect().display();
			donate.detect().display();
			settings.detect().display();
			repo.detect().display();
			if(local.status == 2) {
				localOptions(); break;
			}
			if(web.status == 2) {
				webOptions(); break;
			}
			if(replay.status == 2) {
				replayPage(); break;
			}
			if(createmap.status == 2) {
				createMapPage(); break;
			}
			if(donate.status == 2) {
				donate.clickEvent(); goto WelcomePageStartLabel;
			}
			if(repo.status == 2)
				repo.clickEvent();
		}
		settextjustify(LEFT_TEXT, TOP_TEXT);
		exit(0);
	}

	void testPage() {
		cleardevice();
		int scrw=mapDataStore.mapSizeX*1600,scrh=mapDataStore.mapSizeY*900;
		for(int i=1; i<=12; ++i) LGgame::team[i].clear();
		int team=1;
		//receive team id and player names from server
		LGgame::playerNames[1]=username;
		LGgame::team[1].push_back(1);
		LGgame::playerNames[2]="LocalGen";
		LGgame::playerNames[3]="123123123123";
		LGgame::playerNames[4]="234234234234";
		LGgame::playerNames[5]="345345345345";
		LGgame::playerNames[6]="456456456456";
		LGgame::playerNames[7]="567567567567";
		LGgame::playerNames[8]="678678678678";
		LGgame::playerNames[9]="789789789789";
		LGgame::playerNames[10]="890890890890";
		LGgame::playerNames[11]="901901901901";
		LGgame::team[2].push_back(2);
		LGgame::team[3].push_back(3);
		LGgame::team[4].push_back(4);
		LGgame::team[5].push_back(5);
		LGgame::team[6].push_back(6);
		LGgame::team[7].push_back(7);
		LGgame::team[8].push_back(8);
		LGgame::team[9].push_back(9);
		LGgame::team[9].push_back(10);
		LGgame::team[9].push_back(11);
		rectBUTTON teamButton[13],selectButton,forceStart;
		for(int i=1; i<=12; ++i) {
			teamButton[i]
			.size(50,30)
			.move(scrw/2+(i-7)*50,70)
			.addtext(to_wstring(i))
			.textalign(CENTER_TEXT, CENTER_TEXT)
			.bgcolor(WHITE)
			.textcolor(mainColor)
			.fontname("Quicksand")
			.fontsize(25,0)
			.framecolor(false, mainColor);
		}
		selectButton
		.size(240,30)
		.move(scrw/2-120,30)
		.addtext(L"Select a custom map")
		.textalign(CENTER_TEXT, CENTER_TEXT)
		.bgcolor(WHITE)
		.textcolor(mainColor)
		.fontname("Quicksand")
		.fontsize(25,0)
		.framecolor(false, mainColor);
		forceStart//need an on/off button
		.size(220,50)
		.move(scrw/2-110,scrh-80)
		.addtext(L"Force Start 0/1")
		.textalign(CENTER_TEXT, CENTER_TEXT)
		.bgcolor(WHITE)
		.textcolor(mainColor)
		.fontname("Quicksand")
		.fontsize(40,0)
		.framecolor(false, mainColor);
		for(; is_run(); delay_fps(120)) {
			for(int i=1; i<=12; ++i) teamButton[i].detect().display();
			selectButton.detect().display();
			for(int i=1; i<=12; ++i) {
				if(teamButton[i].status==2) {
					//
				}
			}
			if(selectButton.status==2) {
				//
			}
			int width[13],irow[13],rowWidth[13],rowHeight[13],totRows=0,begWid[13];
			rowWidth[0]=3000;
			for(int i=1; i<=12; ++i) {
				width[i]=0;
				if(LGgame::team[i].size()==0) continue;
				setfont(25,0,"Quicksand");
				width[i]=textwidth(("Team "+to_string(i)).c_str());
				setfont(15,0,"Quicksand");
				for(auto x:LGgame::team[i]) width[i]=max(width[i],textwidth(LGgame::playerNames[x].c_str()));
				if(rowWidth[totRows]+(width[i]+35)>scrw-100) {
					++totRows;
					rowWidth[totRows]=10;
					rowHeight[totRows]=0;
				}
				irow[i]=totRows;
				rowWidth[totRows]+=width[i]+35;
				rowHeight[totRows]=max(rowHeight[totRows],int(32+15*LGgame::team[i].size()));
			}
			rowHeight[0]=10;
			for(int i=1; i<=totRows; ++i) {
				rowHeight[i]=rowHeight[i-1]+rowHeight[i]+10;
				begWid[i]=(scrw-rowWidth[i])/2-10;
			}
			setfillcolor(0xff333333);
			bar(50,110,scrw-50,110+rowHeight[totRows]);
			for(int i=12; i; --i) {
				if(LGgame::team[i].size()==0) continue;
				setfillcolor(bgColor);
				bar(rowWidth[irow[i]]-width[i]-25+begWid[irow[i]],rowHeight[irow[i]-1]+110,rowWidth[irow[i]]+begWid[irow[i]],rowHeight[irow[i]-1]+15*LGgame::team[i].size()+32+110);
				setcolor(WHITE);
				setfont(25,0,"Quicksand");
				settextjustify(CENTER_TEXT,CENTER_TEXT);
				xyprintf(rowWidth[irow[i]]-(width[i]+25)/2+begWid[irow[i]],rowHeight[irow[i]-1]+12+110,("Team "+to_string(i)).c_str());
				setfont(15,0,"Quicksand");
				settextjustify(LEFT_TEXT,TOP_TEXT);
				for(int j=0; j<LGgame::team[i].size(); ++j) {
					setfillcolor(playerInfo[LGgame::team[i][j]].color);
					bar(rowWidth[irow[i]]-width[i]-25+5+2+begWid[irow[i]],rowHeight[irow[i]-1]+5+20+j*15+2+110,
					    rowWidth[irow[i]]-width[i]-25+5+13+begWid[irow[i]],rowHeight[irow[i]-1]+5+20+j*15+13+110);
					xyprintf(rowWidth[irow[i]]-width[i]-25+20+begWid[irow[i]],rowHeight[irow[i]-1]+5+20+j*15+110,LGgame::playerNames[LGgame::team[i][j]].c_str());
				}
				rowWidth[irow[i]]-=width[i]+35;
			}
		}
	}

	void localOptions() {
		// testPage();
		cleardevice();
		setbkmode(TRANSPARENT);
		setbkcolor(bgColor);
		setbkcolor_f(bgColor);

		/** select/import map **/

		rectBUTTON mapbut[505];
		int shiftval = 0;
		for(int i = 1; i <= mapNum; ++i) {
			mapbut[i]
			.size(300 * mapDataStore.mapSizeX - 3, 200 * mapDataStore.mapSizeY - 3)
			.move(((i - 1) % 4 * 300) * mapDataStore.mapSizeX, ((i - 1) / 4 * 200 + shiftval) * mapDataStore.mapSizeY)
			.bgcolor(bgColor)
			.textcolor(WHITE)
			.textalign(CENTER_TEXT,CENTER_TEXT)
			.fontname("Quicksand")
			.fontsize(22 * mapDataStore.mapSizeY, 0)
			// .addtext(maps[i].chiname)
			// .addtext(maps[i].engname)
			// .addtext("General Count: " + to_string(maps[i].generalcnt))
			// .addtext("Plain Count: " + to_string(maps[i].plaincnt))
			// .addtext("City Count: " + to_string(maps[i].citycnt))
			// .addtext("Mountain Count: " + to_string(maps[i].mountaincnt))
			// .addtext("Swamp Count: " + to_string(maps[i].swampcnt))
			// .addtext("Size: " + to_string(maps[i].hei) + " × " + to_string(maps[i].wid))
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
		.textalign(CENTER_TEXT,CENTER_TEXT)
		.move(1250 * mapDataStore.mapSizeX, 825 * mapDataStore.mapSizeY)
		.size(300 * mapDataStore.mapSizeX, 50 * mapDataStore.mapSizeY)
		.bgcolor(WHITE)
		.textcolor(mainColor)
		.fontname("Quicksand")
		.fontsize(40 * mapDataStore.mapSizeY, 0)
		.addtext(L"confirm and submit")
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
				.addtext(L"General Count: " + (~maps[i].generalcnt?to_wstring(maps[i].generalcnt):L"2~12"))
				.addtext(L"Plain Count: " + (~maps[i].plaincnt?to_wstring(maps[i].plaincnt):L"undetermined"))
				.addtext(L"City Count: " + (~maps[i].citycnt?to_wstring(maps[i].citycnt):L"undetermined"))
				.addtext(L"Mountain Count: " + (~maps[i].mountaincnt?to_wstring(maps[i].mountaincnt):L"undetermined"))
				.addtext(L"Swamp Count: " + (~maps[i].swampcnt?to_wstring(maps[i].swampcnt):L"undetermined"))
				.addtext(L"Size: " + (~maps[i].hei?to_wstring(maps[i].hei):L"undetermined") + L" × " + (~maps[i].wid?to_wstring(maps[i].wid):L"undetermined"))
				.move(((i - 1) % 4 * 300) * mapDataStore.mapSizeX, ((i - 1) / 4 * 200 + shiftval) * mapDataStore.mapSizeY)
				.display();
				if(mapbut[i].status == 2) mapSelected = i;
			}
			impfin.display();
			if(impfin.status == 2) {
				std::array<char,1000> s;
				impbox.gettext(1000, s.data());
				std::ifstream fin(s.data());
				if(!fin) {
					impfin.poptext().addtext(L"map not found").display();
					delay_ms(100);
					impfin.poptext().addtext(L"confirm and submit").display();
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

		if(mapSelected) doMapSelect();
		else importGameSettings();

		LGgame::init(cheatCode, plCnt, stDel);
		int ret = LGlocal::GAME();
	}

	void webOptions() {
		cleardevice();
		setbkmode(TRANSPARENT);
		setbkcolor(bgColor);
		setbkcolor_f(bgColor);

		rectBUTTON serverBox;
		rectBUTTON clientBox;
		rectBUTTON mapbut[505];
		rectBUTTON impfin;
		sys_edit impbox;
		int shiftval = 0,ret;

		settextjustify(CENTER_TEXT, CENTER_TEXT);
		setfont(50 * mapDataStore.mapSizeY, 0, "Quicksand");
		setlinewidth(1);

		serverBox
		.size(200 * mapDataStore.mapSizeX, 100 * mapDataStore.mapSizeY)
		.move(400 * mapDataStore.mapSizeX,350 * mapDataStore.mapSizeY)
		.textalign(CENTER_TEXT, CENTER_TEXT)
		.fontname("Quicksand")
		.fontsize(50 * mapDataStore.mapSizeY, 0)
		.bgcolor(WHITE)
		.textcolor(mainColor)
		.addtext(L"Server")
		.status=0;
		serverBox.enableButtonShadow = true;

		clientBox
		.size(200 * mapDataStore.mapSizeX, 100 * mapDataStore.mapSizeY)
		.move(800 * mapDataStore.mapSizeX,350 * mapDataStore.mapSizeY)
		.textalign(CENTER_TEXT, CENTER_TEXT)
		.fontname("Quicksand")
		.fontsize(50 * mapDataStore.mapSizeY, 0)
		.bgcolor(WHITE)
		.textcolor(mainColor)
		.addtext(L"Client")
		.status=0;
		clientBox.enableButtonShadow = true;

		serverBox.display();
		clientBox.display();
		setcolor(WHITE);

		for(; is_run(); delay_fps(120)) {
			while(mousemsg()) {
				serverBox.status=0;
				clientBox.status=0;
				mouse_msg msg = getmouse();

				if(msg.x >= 400 * mapDataStore.mapSizeX && msg.x <= 600 * mapDataStore.mapSizeY
				   && msg.y >= 350 * mapDataStore.mapSizeY && msg.y <= 450 * mapDataStore.mapSizeY) {
					serverBox.status = 1;

					if(msg.is_left()) serverBox.status = 2;
					continue;
				}

				if(msg.x >= 800 * mapDataStore.mapSizeX && msg.x <= 1000 * mapDataStore.mapSizeY
				   && msg.y >= 350 * mapDataStore.mapSizeY && msg.y <= 450 * mapDataStore.mapSizeY) {
					clientBox.status = 1;

					if(msg.is_left()) clientBox.status = 2;
					continue;
				}
			}

			serverBox.display();
			clientBox.display();

			if(serverBox.status==2) goto serverOptions;
			if(clientBox.status==2) goto clientOptions;
		}

	serverOptions:;

		for(int i = 1; i <= mapNum; ++i) {
			mapbut[i]
			.size(300 * mapDataStore.mapSizeX - 3, 200 * mapDataStore.mapSizeY - 3)
			.move(((i - 1) % 4 * 300) * mapDataStore.mapSizeX, ((i - 1) / 4 * 200 + shiftval) * mapDataStore.mapSizeY)
			.bgcolor(bgColor)
			.textcolor(WHITE)
			.textalign(CENTER_TEXT,CENTER_TEXT)
			.fontname("Quicksand")
			.fontsize(22 * mapDataStore.mapSizeY, 0)
			// .addtext(maps[i].chiname)
			// .addtext(maps[i].engname)
			// .addtext("General Count: " + to_string(maps[i].generalcnt))
			// .addtext("Plain Count: " + to_string(maps[i].plaincnt))
			// .addtext("City Count: " + to_string(maps[i].citycnt))
			// .addtext("Mountain Count: " + to_string(maps[i].mountaincnt))
			// .addtext("Swamp Count: " + to_string(maps[i].swampcnt))
			// .addtext("Size: " + to_string(maps[i].hei) + " × " + to_string(maps[i].wid))
			.display();
		}

		impbox.create(true);
		impbox.move(1250 * mapDataStore.mapSizeX, 100 * mapDataStore.mapSizeY);
		impbox.size(300 * mapDataStore.mapSizeX, 700 * mapDataStore.mapSizeY);
		impbox.setbgcolor(WHITE);
		impbox.setcolor(mainColor);
		impbox.setfont(30 * mapDataStore.mapSizeY, 0, "Quicksand");
		impbox.visible(true);

		impfin
		.textalign(CENTER_TEXT,CENTER_TEXT)
		.move(1250 * mapDataStore.mapSizeX, 825 * mapDataStore.mapSizeY)
		.size(300 * mapDataStore.mapSizeX, 50 * mapDataStore.mapSizeY)
		.bgcolor(WHITE)
		.textcolor(mainColor)
		.fontname("Quicksand")
		.fontsize(40 * mapDataStore.mapSizeY, 0)
		.addtext(L"confirm and submit")
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
				.addtext(L"General Count: " + (~maps[i].generalcnt?to_wstring(maps[i].generalcnt):L"2~12"))
				.addtext(L"Plain Count: " + (~maps[i].plaincnt?to_wstring(maps[i].plaincnt):L"undetermined"))
				.addtext(L"City Count: " + (~maps[i].citycnt?to_wstring(maps[i].citycnt):L"undetermined"))
				.addtext(L"Mountain Count: " + (~maps[i].mountaincnt?to_wstring(maps[i].mountaincnt):L"undetermined"))
				.addtext(L"Swamp Count: " + (~maps[i].swampcnt?to_wstring(maps[i].swampcnt):L"undetermined"))
				.addtext(L"Size: " + (~maps[i].hei?to_wstring(maps[i].hei):L"undetermined") + L" × " + (~maps[i].wid?to_wstring(maps[i].wid):L"undetermined"))
				.move(((i - 1) % 4 * 300) * mapDataStore.mapSizeX, ((i - 1) / 4 * 200 + shiftval) * mapDataStore.mapSizeY)
				.display();
				if(mapbut[i].status == 2) mapSelected = i;
			}
			impfin.display();
			if(impfin.status == 2) {
				std::array<char,1000> s;
				impbox.gettext(1000, s.data());
				std::ifstream fin(s.data());
				if(!fin) {
					impfin.poptext().addtext(L"map not found").display();
					delay_ms(100);
					impfin.poptext().addtext(L"confirm and submit").display();
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

		if(mapSelected) doMapSelect();
		else importGameSettings();

		LGgame::init(cheatCode, plCnt, stDel);
		ret = LGserver::GAME();
		return ;

	clientOptions:;
		ret=LGclient::GAME();
		return ;
	}

	void replayPage() {
		cleardevice();
		setrendermode(RENDER_MANUAL);
		LGreplay::rreplay.initReplay();
		for(int i = 1; i <= LGgame::playerCnt; ++i) LGgame::isAlive[i] = 1;
		printMap(1048575, {-1,-1});

		rectBUTTON jumpbut;
		sys_edit jumpbox;
		rectBUTTON jumpsmbut;
		rectBUTTON stepbybut;
		rectBUTTON exitbut;
		rectBUTTON backbut;
		rectBUTTON nextbut;
		rectBUTTON autobut;

		jumpbut.textalign(CENTER_TEXT,CENTER_TEXT)
		.bgcolor(WHITE).textcolor(BLACK)
		.move(0 * mapDataStore.mapSizeX, 35 * mapDataStore.mapSizeY)
		.fontname("Quicksand")
		.fontsize(30 * mapDataStore.mapSizeY, 0)
		.size(150 * mapDataStore.mapSizeX, 30 * mapDataStore.mapSizeY)
		.addtext(L"Jump to turn: ").enableTextShadow = false;
		jumpbox.create(false);
		jumpbox.size(50 * mapDataStore.mapSizeX, 30 * mapDataStore.mapSizeY);
		jumpbox.move(150 * mapDataStore.mapSizeX, 35 * mapDataStore.mapSizeY);
		jumpbox.setfont(25 * mapDataStore.mapSizeY, 0, "Quicksand");
		jumpbox.visible(true);
		jumpsmbut.textalign(CENTER_TEXT,CENTER_TEXT)
		.bgcolor(WHITE).textcolor(BLACK)
		.move(200 * mapDataStore.mapSizeX, 35 * mapDataStore.mapSizeY)
		.fontname("Quicksand")
		.fontsize(30 * mapDataStore.mapSizeY, 0)
		.size(50 * mapDataStore.mapSizeX, 30 * mapDataStore.mapSizeY)
		.addtext(L"→").enableTextShadow = false;

		int smsx=0,smsy=0,midact=0;
		for(; is_run();) {
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
					case key_esc: { /*[ESC]*/
						closegraph();
						return;
					}
					case key_left: LGreplay::rreplay.preTurn(); break;
					case key_right: LGreplay::rreplay.nextTurn(); break;
				}
			}
			cleardevice();
			printMap(1048575, {-1,-1});
			const static int screenszr = 1600 * LGGraphics::mapDataStore.mapSizeX;
			static int fpslen;
			static int turnlen;
			setfillcolor(LGGraphics::bgColor);
			bar(screenszr - 10 - fpslen - 10 - turnlen - 10, 0, screenszr, 20 * LGGraphics::mapDataStore.mapSizeY);
			setfont(20 * LGGraphics::mapDataStore.mapSizeY, 0, "Quicksand");
			fpslen = textwidth(("FPS: " + to_string(getfps())).c_str());
			turnlen = textwidth(("Turn " + to_string(LGreplay::rreplay.curTurn) + ".").c_str());
			setfillcolor(RED);
			bar(screenszr - fpslen - 10, 0, screenszr, 20 * LGGraphics::mapDataStore.mapSizeY);
			rectangle(screenszr - fpslen - 10, 0, screenszr, 20 * LGGraphics::mapDataStore.mapSizeY);
			setfillcolor(BLUE);
			bar(screenszr - fpslen - 10 - turnlen - 10, 0, screenszr - fpslen - 10, 20 * LGGraphics::mapDataStore.mapSizeY);
			rectangle(screenszr - fpslen - 10 - turnlen - 10, 0, screenszr - fpslen - 10, 20 * LGGraphics::mapDataStore.mapSizeY);
			settextjustify(CENTER_TEXT, TOP_TEXT);
			xyprintf(screenszr - fpslen / 2 - 5, 0, "FPS: %f", getfps());
			xyprintf(screenszr - fpslen - 10 - turnlen / 2 - 5, 0, "Turn %d.", LGreplay::rreplay.curTurn);
			LGgame::ranklist();
			jumpbut.display();
			jumpsmbut.detect().display();
			if(jumpsmbut.status==2) {
				char s[101];
				jumpbox.gettext(sizeof(s)-1,s);
				int toTurn = atoi(s);
				if(toTurn!=0) LGreplay::rreplay.gotoTurn(toTurn);
			}
			delay_ms(0);
		}
	}

	void createMapPage() {
		LGgame::inCreate=1;
		cleardevice();
		setrendermode(RENDER_MANUAL);
		settextjustify(LEFT_TEXT, TOP_TEXT);
		int scrw=mapDataStore.mapSizeX*1600,scrh=mapDataStore.mapSizeY*900;
		sys_edit citynumBox,plainnumBox,savenameBox;
		rectBUTTON saveButton,cancelButton,loadButton;
		citynumBox.create();
		citynumBox.size(80,30);
		citynumBox.move(scrw/2+5,scrh-110);
		citynumBox.setfont(20,0,"Quicksand");
		citynumBox.setcolor(mainColor);
		citynumBox.settext("40");
		plainnumBox.create();
		plainnumBox.size(80,30);
		plainnumBox.move(scrw/2+35,scrh-110);
		plainnumBox.setfont(20,0,"Quicksand");
		plainnumBox.setcolor(mainColor);
		plainnumBox.settext("40");
		savenameBox.create();
		savenameBox.size(100,40);
		savenameBox.move(scrw/2+20,scrh/2-20);
		savenameBox.setfont(30,0,"Quicksand");
		savenameBox.setcolor(mainColor);
		savenameBox.settext("map");
		saveButton
		.size(90,40)
		.move(scrw/2-100,scrh/2+30)
		.fontname("Quicksand")
		.fontsize(40,0)
		.bgcolor(mainColor)
		.textcolor(WHITE)
		.textalign(CENTER_TEXT,CENTER_TEXT)
		.addtext(L"Save");
		cancelButton
		.size(90,40)
		.move(scrw/2+10,scrh/2+30)
		.fontname("Quicksand")
		.fontsize(40,0)
		.bgcolor(mainColor)
		.textcolor(WHITE)
		.textalign(CENTER_TEXT,CENTER_TEXT)
		.addtext(L"Cancel");
		loadButton
		.size(90,40)
		.move(scrw/2-100,scrh/2+30)
		.fontname("Quicksand")
		.fontsize(40,0)
		.bgcolor(mainColor)
		.textcolor(WHITE)
		.textalign(CENTER_TEXT,CENTER_TEXT)
		.addtext(L"Load");
		mapW=mapH=10;
		for(int i=1; i<=mapH; ++i)
			for(int j=1; j<=mapW; ++j) gameMap[i][j] = {0,0,0,0};
		printMap(1048575, {-1,-1});
		createOptions(0,scrh/2-140);
		setfillcolor(mainColor);
		int smsx=0,smsy=0,midact=0,type=0,citynum=40,plainnum=40;
		bool moved=false,saved=false;
		std::chrono::steady_clock::duration prsttm;
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
							if(msg.x<40&&msg.y>=scrh/2-140&&msg.y<scrh/2+140)
								type=(msg.y-(scrh/2-140))/40;
							else if(msg.x>=scrw/2-145&&msg.x<scrw/2-5&&msg.y>=scrh-60&&msg.y<scrh-20) {
								settextjustify(CENTER_TEXT, CENTER_TEXT);
								setfillcolor(WHITE);
								setcolor(BLACK);
								bar(scrw/2-130,scrh/2-80,scrw/2+130,scrh/2+80);
								setfont(40,0,"Quicksand");
								xyprintf(scrw/2,scrh/2-50,"Save map");
								xyprintf(scrw/2-55,scrh/2,"Map name:");
								savenameBox.visible(true);
								setfillcolor(mainColor);
								saveButton.display();
								cancelButton.display();
								delay_ms(0);
								for(; is_run(); delay_fps(120)) {
									saveButton.detect().display();
									cancelButton.detect().display();
									if(saveButton.status==2) {
										char s[100];
										savenameBox.gettext(sizeof(s),s);
										string ss=s;
										Zip();
										freopen(("maps/"+ss+".lg").c_str(),"w",stdout);
										printf("%s",strZip);
										fclose(stdout);
										saved=true;
										LGgame::inCreate=0;
										return;
									}
									if(cancelButton.status==2) break;
								}
								savenameBox.visible(false);
							} else if(msg.x>=scrw/2+5&&msg.x<scrw/2+145&&msg.y>=scrh-60&&msg.y<scrh-20) {
								settextjustify(CENTER_TEXT, CENTER_TEXT);
								setfillcolor(WHITE);
								setcolor(BLACK);
								bar(scrw/2-130,scrh/2-80,scrw/2+130,scrh/2+80);
								setfont(40,0,"Quicksand");
								xyprintf(scrw/2,scrh/2-50,"Load map");
								xyprintf(scrw/2-55,scrh/2,"Map name:");
								savenameBox.visible(true);
								setfillcolor(mainColor);
								loadButton.display();
								cancelButton.display();
								delay_ms(0);
								for(; is_run(); delay_fps(120)) {
									loadButton.detect().display();
									cancelButton.detect().display();
									if(loadButton.status==2) {
										char s[100];
										savenameBox.gettext(sizeof(s),s);
										string ss=s;
										FILE* file;
										file=fopen(("maps/"+ss+".lg").c_str(),"r");
										if(!file) {
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
							} else if(msg.x>=scrw/2-200&&msg.x<scrw/2-170&&msg.y>=10&&msg.y<40) {
								if(mapH>1) --mapH;
							} else if(msg.x>=scrw/2-50&&msg.x<scrw/2-20&&msg.y>=10&&msg.y<40) {
								if(mapH<100) {
									++mapH;
									for(int i=1; i<=mapW; ++i) gameMap[mapH][i]= {0,0,0,0};
								}
							} else if(msg.x>=scrw/2+20&&msg.x<scrw/2+50&&msg.y>=10&&msg.y<40) {
								if(mapW>1) --mapW;
							} else if(msg.x>=scrw/2+170&&msg.x<scrw/2+200&&msg.y>=10&&msg.y<40) {
								if(mapW<100) {
									++mapW;
									for(int i=1; i<=mapH; ++i) gameMap[i][mapW]= {0,0,0,0};
								}
							} else if(msg.x >= LGGraphics::mapDataStore.maplocX &&
							          msg.y >= LGGraphics::mapDataStore.maplocY &&
							          msg.x <= LGGraphics::mapDataStore.maplocX + widthPerBlock * mapW &&
							          msg.y <= LGGraphics::mapDataStore.maplocY + heightPerBlock * mapH) {
								int lin = (msg.y + heightPerBlock - 1 - LGGraphics::mapDataStore.maplocY) / heightPerBlock;
								int col = (msg.x + widthPerBlock - 1 - LGGraphics::mapDataStore.maplocX) / widthPerBlock;
								switch(type) {
									case 0:
										gameMap[lin][col].player=0;
										gameMap[lin][col].type=2;
										gameMap[lin][col].army=0;
										break;
									case 1:
										gameMap[lin][col].player=0;
										gameMap[lin][col].type=1;
										gameMap[lin][col].army=0;
										break;
									case 2:
										gameMap[lin][col].player=0;
										gameMap[lin][col].type=3;
										gameMap[lin][col].army=0;
										break;
									case 3:
										gameMap[lin][col].player=0;
										gameMap[lin][col].type=4;
										gameMap[lin][col].army=citynum;
										break;
									case 4:
										gameMap[lin][col].player=0;
										gameMap[lin][col].type=0;
										gameMap[lin][col].army=plainnum;
										break;
									case 5:
										gameMap[lin][col].lit=!gameMap[lin][col].lit;
										break;
									case 6:
										gameMap[lin][col].player=0;
										gameMap[lin][col].type=0;
										gameMap[lin][col].army=0;
										break;
								}
							}
						}
					}
				}
			}
			cleardevice();
			printMap(1048575, {-1,-1});
			createOptions(type,scrh/2-140);
			settextjustify(CENTER_TEXT, CENTER_TEXT);
			setcolor(BLACK);
			setfillcolor(WHITE);
			setfont(20,0,"Quicksand");
			if(type==0) {
				bar(scrw/2-105,scrh-110,scrw/2+105,scrh-80);
				xyprintf(scrw/2,scrh-95,"Click a tile to place a mountain.");
			}
			if(type==1) {
				bar(scrw/2-105,scrh-130,scrw/2+105,scrh-80);
				xyprintf(scrw/2,scrh-115,"Click a tile to place a swamp.");
				xyprintf(scrw/2,scrh-95,"Swamps drain 1 army per turn.");
			}
			if(type==5) {
				bar(scrw/2-95,scrh-110,scrw/2+95,scrh-80);
				xyprintf(scrw/2,scrh-95,"Click a tile to toggle light tile.");
			}
			if(type==6) {
				bar(scrw/2-80,scrh-110,scrw/2+80,scrh-80);
				xyprintf(scrw/2,scrh-95,"Click a tile to remove it.");
			}
			if(type==3) {
				citynumBox.visible(true);
				bar(scrw/2-85,scrh-110,scrw/2+5,scrh-80);
				xyprintf(scrw/2-40,scrh-95,"City Strength:");
				char s[10];
				citynumBox.gettext(sizeof(s),s);
				sscanf(s,"%d",&citynum);
			} else citynumBox.visible(false);
			if(type==4) {
				plainnumBox.visible(true);
				bar(scrw/2-115,scrh-110,scrw/2+35,scrh-80);
				xyprintf(scrw/2-40,scrh-95,"Neutral Army Strength:");
				char s[10];
				plainnumBox.gettext(sizeof(s),s);
				sscanf(s,"%d",&plainnum);
			} else plainnumBox.visible(false);
			setfillcolor(WHITE);
			setcolor(BLACK);
			setfont(30,0,"Quicksand");
			bar(scrw/2-200,10,scrw/2-20,40);
			xyprintf(scrw/2-110,25,"Height: %d",mapH);
			xyprintf(scrw/2-185,25,"-");
			xyprintf(scrw/2-35,25,"+");
			bar(scrw/2+20,10,scrw/2+200,40);
			xyprintf(scrw/2+110,25,"Width: %d",mapW);
			xyprintf(scrw/2+35,25,"-");
			xyprintf(scrw/2+185,25,"+");
			bar(scrw/2-155,scrh-70,scrw/2+155,scrh-10);
			setfillcolor(mainColor);
			setcolor(WHITE);
			setfont(40,0,"Quicksand");
			bar(scrw/2-145,scrh-60,scrw/2-5,scrh-20);
			xyprintf(scrw/2-75,scrh-40,"Save map");
			bar(scrw/2+5,scrh-60,scrw/2+145,scrh-20);
			xyprintf(scrw/2+75,scrh-40,"Load map");
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
		         "%ls", maps[mapSelected].chiname.c_str());
		xyprintf(255 * mapDataStore.mapSizeX, 45 * mapDataStore.mapSizeY,
		         "%ls", maps[mapSelected].engname.c_str());
		setcolor(WHITE);
		setfont(30 * mapDataStore.mapSizeY, 0, "Quicksand");
		xyprintf(255 * mapDataStore.mapSizeX, 85 * mapDataStore.mapSizeY,
		         L"Author: %ls", maps[mapSelected].auth.c_str());
		xyprintf(255 * mapDataStore.mapSizeX, 115 * mapDataStore.mapSizeY,
		         L"Size: %d × %d", maps[mapSelected].hei, maps[mapSelected].wid);
		xyprintf(255 * mapDataStore.mapSizeX, 145 * mapDataStore.mapSizeY,
		         L"General Count: %d", maps[mapSelected].generalcnt);
		xyprintf(255 * mapDataStore.mapSizeX, 175 * mapDataStore.mapSizeY,
		         L"Plain Count: %d", maps[mapSelected].plaincnt);
		xyprintf(255 * mapDataStore.mapSizeX, 205 * mapDataStore.mapSizeY,
		         L"City Count: %d", maps[mapSelected].citycnt);
		xyprintf(255 * mapDataStore.mapSizeX, 235 * mapDataStore.mapSizeY,
		         L"Mountain Count: %d", maps[mapSelected].mountaincnt);
		xyprintf(255 * mapDataStore.mapSizeX, 265 * mapDataStore.mapSizeY,
		         L"Swamp Count: %d", maps[mapSelected].swampcnt);
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
			xyprintf(800 * mapDataStore.mapSizeX, 5 * mapDataStore.mapSizeY, L"Input Height (<=100):");
			xyprintf(800 * mapDataStore.mapSizeX, 45 * mapDataStore.mapSizeY, L"Input Width (<=100):");
			if(mapSelected == 3) {
				xyprintf(800 * mapDataStore.mapSizeX, 85 * mapDataStore.mapSizeY, L"Input MINIMUM Army:");
				xyprintf(800 * mapDataStore.mapSizeX, 125 * mapDataStore.mapSizeY, L"Input MAXIMUM Army:");
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
			.move(1020 * mapDataStore.mapSizeX, 6 * mapDataStore.mapSizeY)
			.size(100 * mapDataStore.mapSizeX, 38 * mapDataStore.mapSizeY)
			.bgcolor(WHITE)
			.textcolor(mainColor)
			.textalign(CENTER_TEXT,CENTER_TEXT)
			.fontsize(35 * mapDataStore.mapSizeY, 0)
			.fontname("Quicksand")
			.addtext(L"confirm");
			widb
			.move(1020 * mapDataStore.mapSizeX, 46 * mapDataStore.mapSizeY)
			.size(100 * mapDataStore.mapSizeX, 38 * mapDataStore.mapSizeY)
			.bgcolor(WHITE)
			.textcolor(mainColor)
			.textalign(CENTER_TEXT,CENTER_TEXT)
			.fontsize(35 * mapDataStore.mapSizeY, 0)
			.fontname("Quicksand")
			.addtext(L"confirm");
			if(mapSelected == 3) {
				amnb
				.move(1020 * mapDataStore.mapSizeX, 86 * mapDataStore.mapSizeY)
				.size(100 * mapDataStore.mapSizeX, 38 * mapDataStore.mapSizeY)
				.bgcolor(WHITE)
				.textcolor(mainColor)
				.textalign(CENTER_TEXT,CENTER_TEXT)
				.fontsize(35 * mapDataStore.mapSizeY, 0)
				.fontname("Quicksand")
				.addtext(L"confirm");
				amxb
				.move(1020 * mapDataStore.mapSizeX, 126 * mapDataStore.mapSizeY)
				.size(100 * mapDataStore.mapSizeX, 38 * mapDataStore.mapSizeY)
				.bgcolor(WHITE)
				.textcolor(mainColor)
				.textalign(CENTER_TEXT,CENTER_TEXT)
				.fontsize(35 * mapDataStore.mapSizeY, 0)
				.fontname("Quicksand")
				.addtext(L"confirm");
			}
			endb
			.move(810 * mapDataStore.mapSizeX, 166 * mapDataStore.mapSizeY)
			.size(310 * mapDataStore.mapSizeX, 38 * mapDataStore.mapSizeY)
			.bgcolor(WHITE)
			.textcolor(mainColor)
			.textalign(CENTER_TEXT,CENTER_TEXT)
			.fontsize(35 * mapDataStore.mapSizeY, 0)
			.fontname("Quicksand")
			.addtext(L"end input");
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
						heib.poptext().addtext(L"invalid").display();
						delay_ms(100);
						heib.poptext().addtext(L"confirm").display();
					} else hb=true;
				}
				if(widb.status == 2) {
					char s[55];
					widinput.gettext(sizeof(s), s);
					int p = sscanf(s, "%d", &width);
					if(p!=1) {
						widb.poptext().addtext(L"invalid").display();
						delay_ms(100);
						widb.poptext().addtext(L"confirm").display();
					} else wb=true;
				}
				if(amnb.status == 2) {
					char s[55];
					amninput.gettext(sizeof(s), s);
					int p = sscanf(s, "%d", &armymin);
					if(p!=1) {
						amnb.poptext().addtext(L"invalid").display();
						delay_ms(100);
						amnb.poptext().addtext(L"confirm").display();
					} else nb=true;
				}
				if(amxb.status == 2) {
					char s[55];
					amxinput.gettext(sizeof(s), s);
					int p = sscanf(s, "%d", &armymax);
					if(p!=1) {
						amxb.poptext().addtext(L"invalid").display();
						delay_ms(100);
						amxb.poptext().addtext(L"confirm").display();
					} else xb=true;
				}
				if(endb.status == 2) {
					if(hb&&wb&&(mapSelected==3?(nb&&xb):1)) {
						endb.status = 0;
						endb.display();
						break;
					} else {
						endb.poptext().addtext(L"not finished").display();
						delay_ms(100);
						endb.poptext().addtext(L"end input").display();
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
			.size(100 * mapDataStore.mapSizeX - 1*2, 100 * mapDataStore.mapSizeY - 1*2)
			.move(55 * mapDataStore.mapSizeX + 100 * mapDataStore.mapSizeX * colNum + 1,
			      400 * mapDataStore.mapSizeY + 100 * mapDataStore.mapSizeY * rowNum + 1)
			.textalign(CENTER_TEXT, CENTER_TEXT)
			.fontname("Quicksand")
			.fontsize(50 * mapDataStore.mapSizeY, 0)
			.bgcolor(bgColor)
			.textcolor(WHITE)
			.addtext(to_wstring(i));
			plCntBox[i].enableButtonShadow = false;
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
		.size(150 * mapDataStore.mapSizeX, 50 * mapDataStore.mapSizeY)
		.move(900 * mapDataStore.mapSizeX, 450 * mapDataStore.mapSizeY)
		.fontname("Quicksand")
		.fontsize(50 * mapDataStore.mapSizeY, 0)
		.bgcolor(WHITE)
		.textcolor(mainColor)
		.textalign(CENTER_TEXT,CENTER_TEXT)
		.addtext(L"submit");
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
			.textalign(CENTER_TEXT, CENTER_TEXT)
			.fontname("Quicksand")
			.fontsize(40 * mapDataStore.mapSizeY, 0)
			.size(100 * mapDataStore.mapSizeX - 1*2, 100 * mapDataStore.mapSizeY - 1*2)
			.move(1150 * mapDataStore.mapSizeX + 100 * mapDataStore.mapSizeX * colNum + 1,
			      400 * mapDataStore.mapSizeY + 100 * mapDataStore.mapSizeY * rowNum + 1);
			checkBox[i].enableButtonShadow = false;
			checkBox[i].enableTextShadow = false;
		}
		checkOA
		.size(400 * mapDataStore.mapSizeX - 1*2, 100 * mapDataStore.mapSizeY - 1*2)
		.move(1150 * mapDataStore.mapSizeX + 1, 700 * mapDataStore.mapSizeY + 1)
		.addtext(L"Overall Select")
		.textalign(CENTER_TEXT, CENTER_TEXT)
		.fontname("Quicksand")
		.fontsize(50 * mapDataStore.mapSizeY, 0);
		checkOA.enableButtonShadow = false;
		checkOA.enableTextShadow = false;
		checkOA.display();
		gameBox
		.size(200 * mapDataStore.mapSizeX, 100 * mapDataStore.mapSizeY)
		.move(1150 * mapDataStore.mapSizeX, 100 * mapDataStore.mapSizeY)
		.addtext(L"START")
		.textalign(CENTER_TEXT, CENTER_TEXT)
		.fontname("Quicksand")
		.fontsize(50 * mapDataStore.mapSizeY, 0)
		.bgcolor(WHITE)
		.textcolor(mainColor);
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
					.bgcolor(playerInfo[i].color)
					.textcolor(WHITE);;
				} else {
					checkBox[i]
					.bgcolor(bgColor)
					.textcolor(playerInfo[i].color);;
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
				.bgcolor(mainColor)
				.textcolor(WHITE);
			} else {
				checkOA
				.bgcolor(bgColor)
				.textcolor(mainColor);
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
		pimg[7] = newimage();
		getimage(pimg[7], "img/erase.png");
		pimg[8] = newimage();
		getimage(pimg[8], "img/light.png");
		pimg[9] = newimage();
		getimage(pimg[9], "img/options.png");
		pimg[10] = newimage();
		getimage(pimg[10], "img/github.png");
		for(int i = 1; i <= 10; i++) ege_enable_aa(true, pimg[i]);
		ege_enable_aa(true);
		setbkcolor(0xff222222);
		setbkcolor_f(0xff222222);
		cleardevice();
	}
}

inline int getHeightPerBlock() { return LGGraphics::mapDataStore.heightPerBlock; }

inline int getWidthPerBlock() { return LGGraphics::mapDataStore.widthPerBlock; }

#endif // __LGGRAPHICS_HPP__
