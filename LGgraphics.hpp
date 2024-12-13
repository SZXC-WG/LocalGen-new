/* This is LGgraphics.hpp file of LocalGen.                              */
/* Copyright (c) 2024 SZXC Work Group; All rights reserved.              */
/* Developers: http://github.com/SZXC-WG                                 */
/* Project: http://github.com/SZXC-WG/LocalGen-new                       */
/*                                                                       */
/* This project is licensed under the MIT license. That means you can    */
/* download, use and share a copy of the product of this project. You    */
/* may modify the source code and make contribution to it too. But, you  */
/* must print the copyright information at the front of your product.    */
/*                                                                       */
/* The full MIT license this project uses can be found here:             */
/* http://github.com/SZXC-WG/LocalGen-new/blob/main/LICENSE.md           */

#ifndef LGGRAPHICS_HPP_
#define LGGRAPHICS_HPP_

#include "LGdef.hpp"
#include "glib/GLIB_HEAD.hpp"

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
		windowData.heightPerBlock = a;
		windowData.widthPerBlock = b;
		windowData.height = c;
		windowData.width = d;
		return;
	}
	void initWindowSize() {
		initgraph(800, 600, INIT_RENDERMANUAL | INIT_UNICODE);
		ege_enable_aa(true, NULL);
		setcaption(L"LocalGen v" VER_STRING " Window Size Selection");
		setbkcolor(bgColor);
		setbkcolor_f(bgColor);
		bool changeMade = true;
		cleardevice();
		setbkmode(TRANSPARENT);
		settextjustify(CENTER_TEXT, CENTER_TEXT);
		setfont(-80, 0, LGset::mainFontName.c_str(), 0, 0, FW_BOLD, 0, 0, 0);
		setcolor(mainColor);
		xyprintf(252, 82, L"Local");
		setcolor(WHITE);
		xyprintf(250, 80, L"Local");
		setcolor(mainColor);
		xyprintf(252, 152, L"generals.io");
		setcolor(WHITE);
		xyprintf(250, 150, L"generals.io");
		putimage_withalpha(NULL, iconImg, 500, 10, 0, 0, getwidth(iconImg), getheight(iconImg));
		delimage(iconImg);
		setfont(20, 0, "Lucida Fax");
		xyprintf(500 + getwidth(iconImg) / 2, 10 + getheight(iconImg) + 20 / 2 + 10,
		         L"version %d.%d.%d (build %d)", VER_MAJOR, VER_MINOR, VER_RELEASE, VER_BUILD);
		setfont(50, 0, LGset::mainFontName.c_str());
		setcolor(mainColor);
		xyprintf(251, 251, L"Please Select Window Size:");
		setcolor(WHITE);
		xyprintf(250, 250, L"Please Select Window Size:");
		settextjustify(LEFT_TEXT, TOP_TEXT);
		rectBUTTON scrsz[10];
		for(int i = 200; i <= 500; i += 100) {
			int p = i / 100 - 2;
			scrsz[p].size(400, 50);
			scrsz[p].bgcolor(bgColor);
			scrsz[p].textcolor(WHITE);
			scrsz[p].fontname(LGset::mainFontName.c_str());
			scrsz[p].fontsize(30, 0);
			scrsz[p].move(50, 180 + i / 4 * 2 + p * 3);
			scrsz[p].addtext(to_wstring(i * 4) + L" × " + to_wstring(i * 9 / 4));
			scrsz[p].clickEvent = [i]() -> void { select = i / 100; };
			scrsz[p].textalign(CENTER_TEXT, CENTER_TEXT);
			scrsz[p].display();
		}
		{
			int i = 600;
			int p = i / 100 - 2;
			scrsz[p].size(400, 50);
			scrsz[p].bgcolor(bgColor);
			scrsz[p].textcolor(WHITE);
			scrsz[p].fontname(LGset::mainFontName.c_str());
			scrsz[p].fontsize(30, 0);
			scrsz[p].move(50, 180 + i / 4 * 2 + p * 3);
			scrsz[p].addtext(L"Full Screen (" + to_wstring(GetSystemMetrics(SM_CXSCREEN)) + L" × " + to_wstring(GetSystemMetrics(SM_CYSCREEN)) + L")");
			scrsz[p].clickEvent = [i]() -> void { select = i / 100; };
			scrsz[p].textalign(CENTER_TEXT, CENTER_TEXT);
			scrsz[p].display();
		}
		for(; is_run(); delay_fps(60)) {
			for(int i = 200; i <= 600; i += 100) {
				int p = i / 100 - 2;
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
			windowData.zoomX = (double)(1.0 * (double)w / 1600.0);
			windowData.zoomY = (double)(1.0 * (double)h / 900.0);
		} else {
			windowData.zoomX = windowData.zoomY = (double)select / 4.0;
			int nScreenWidth, nScreenHeight;
			nScreenWidth = GetSystemMetrics(SM_CXSCREEN);
			nScreenHeight = GetSystemMetrics(SM_CYSCREEN);
			initgraph(1600 * windowData.zoomX, 900 * windowData.zoomY, RENDER_AUTO);
		}
		setcaption(L"LocalGen v" VER_STRING " by SZXC Work Group");
	}

	void initname() {
		cleardevice();
		int scrw = windowData.zoomX * 1600, scrh = windowData.zoomY * 900;
		sys_edit nameBox;
		rectBUTTON submitButton;
		setfillcolor(WHITE);
		bar(scrw / 2 - 120, scrh / 2 - 80, scrw / 2 + 120, scrh / 2 + 80);
		setcolor(BLACK);
		setfont(30, 0, LGset::mainFontName.c_str());
		settextjustify(CENTER_TEXT, CENTER_TEXT);
		xyprintf(scrw / 2, scrh / 2 - 60, "Enter your username:");
		nameBox.create();
		nameBox.size(220, 40);
		nameBox.move(scrw / 2 - 110, scrh / 2 - 40);
		nameBox.setfont(30, 0, LGset::mainFontName.c_str());
		nameBox.setcolor(mainColor);
		nameBox.settext("Anonymous");
		nameBox.visible(true);
		submitButton
		    .size(100, 40)
		    .move(scrw / 2 - 50, scrh / 2 + 30)
		    .fontname(LGset::mainFontName.c_str())
		    .fontsize(zoomY(40), 0)
		    .bgcolor(mainColor)
		    .textcolor(WHITE)
		    .textalign(CENTER_TEXT, CENTER_TEXT)
		    .addtext(L"Submit");
		for(; is_run(); delay_fps(120)) {
			submitButton.detect().display();
			if(submitButton.status == 2) {
				char s[100];
				nameBox.gettext(sizeof(s), s);
				username = s;
				if(checkValidUsername(username)) return;
				else {
					setfillcolor(errorColor);
					bar(scrw / 2 - 110, scrh / 2 + 5, scrw / 2 + 110, scrh / 2 + 25);
					setcolor(BLACK);
					setfont(15, 0, LGset::mainFontName.c_str());
					xyprintf(scrw / 2, scrh / 2 + 15, "Username must be between 3 and 16 letters.");
				}
			}
		}
	}

	void WelcomePage() {
		initWindowSize();
		init();
		// printf("before pages.");
		initPages();
		{
			std::wifstream pnfin("_players.ini");
			if(!pnfin.fail()) {
				for(int i = 1; i <= 16; ++i) {
					wstring name, col;
					getline(pnfin, name);
					if(pnfin.fail()) break;
					name = wcharTransfer(name);
					playerInfo[i].name = name;
					getline(pnfin, col);
					color_t c = std::stoul(col, nullptr, 0);
					playerInfo[i].color = c;
					if(pnfin.fail()) break;
				}
				pnfin.close();
			}
		}
	WelcomePageStartLabel:;
		setbkmode(TRANSPARENT);
		setbkcolor(bgColor);
		setbkcolor_f(bgColor);
		// initname();
		cleardevice();
		PIMAGE zfavi = newimage();
		getimage(zfavi, "PNG", "IMAGE_FAVICON");
		images::zoomImage(zfavi, getwidth(zfavi) * 1.8 * windowData.zoomX, getheight(zfavi) * 1.8 * windowData.zoomY);
		putimage_withalpha(NULL, zfavi, 100 * windowData.zoomX, 50 * windowData.zoomY,
		                   0, 0, getwidth(zfavi), getheight(zfavi));
		delimage(zfavi);
		settextjustify(CENTER_TEXT, TOP_TEXT);
		setfont(150 * windowData.zoomY, 0, LGset::mainFontName.c_str(), 0, 0, FW_BOLD, 0, 0, 0);
		setcolor(WHITE);
		xyprintf(330 * windowData.zoomX, 500 * windowData.zoomY, "Local");
		xyprintf(330 * windowData.zoomX, 600 * windowData.zoomY, "generals.io");
		setfont(zoomY(30), 0, "Lucida Fax");
		xyprintf(330 * windowData.zoomX, 750 * windowData.zoomY, "version %d.%d.%d (build %d)", VER_MAJOR, VER_MINOR, VER_RELEASE, VER_BUILD);
		rectBUTTON local, web, replay, createmap;
		rectBUTTON donate;
		circBUTTON settings, repo;
		local
		    .size(600 * windowData.zoomX, 100 * windowData.zoomY)
		    .move(700 * windowData.zoomX, 100 * windowData.zoomY)
		    .addtext(L"local game")
		    .textalign(CENTER_TEXT, CENTER_TEXT)
		    .bgcolor(WHITE)
		    .textcolor(mainColor)
		    .fontname(LGset::mainFontName.c_str())
		    .fontsize(50 * windowData.zoomY, 0)
		    .framecolor(false, mainColor)
		    .frame(10 * windowData.zoomY)
		    .display();
		web
		    .size(600 * windowData.zoomX, 100 * windowData.zoomY)
		    .move(700 * windowData.zoomX, 225 * windowData.zoomY)
		    .addtext(L"web game (beta)")
		    .textalign(CENTER_TEXT, CENTER_TEXT)
		    .bgcolor(WHITE)
		    .textcolor(mainColor)
		    .fontname(LGset::mainFontName.c_str())
		    .fontsize(50 * windowData.zoomY, 0)
		    .framecolor(false, mainColor)
		    .frame(10 * windowData.zoomY)
		    .display();
		replay
		    .size(600 * windowData.zoomX, 100 * windowData.zoomY)
		    .move(700 * windowData.zoomX, 350 * windowData.zoomY)
		    .addtext(L"load replay (beta)")
		    .textalign(CENTER_TEXT, CENTER_TEXT)
		    .bgcolor(WHITE)
		    .textcolor(mainColor)
		    .fontname(LGset::mainFontName.c_str())
		    .fontsize(50 * windowData.zoomY, 0)
		    .framecolor(false, mainColor)
		    .frame(10 * windowData.zoomY)
		    .display();
		createmap
		    .size(600 * windowData.zoomX, 100 * windowData.zoomY)
		    .move(700 * windowData.zoomX, 475 * windowData.zoomY)
		    .addtext(L"create map (beta)")
		    .textalign(CENTER_TEXT, CENTER_TEXT)
		    .bgcolor(WHITE)
		    .textcolor(mainColor)
		    .fontname(LGset::mainFontName.c_str())
		    .fontsize(50 * windowData.zoomY, 0)
		    .framecolor(false, mainColor)
		    .frame(10 * windowData.zoomY)
		    .display();
		donate
		    .size(400 * windowData.zoomX, 50 * windowData.zoomY)
		    .move(1150 * windowData.zoomX, 750 * windowData.zoomY)
		    .addtext(L"Please donate to support us!")
		    .textalign(CENTER_TEXT, CENTER_TEXT)
		    .bgcolor(WHITE)
		    .textcolor(mainColor)
		    .fontname(LGset::mainFontName.c_str())
		    .fontsize(25 * windowData.zoomY, 0)
		    .framecolor(false, mainColor)
		    .frame(10 * windowData.zoomY)
		    .event([]() {
			    cleardevice();
			    PIMAGE donate_wc = newimage(), donate_ap = newimage();
			    getimage_pngfile(donate_wc, "img/donate_wechat.png");
			    getimage_pngfile(donate_ap, "img/donate_alipay.png");
			    images::zoomImage(donate_wc, 600 * windowData.zoomX, 800 * windowData.zoomY);
			    images::zoomImage(donate_ap, 600 * windowData.zoomX, 800 * windowData.zoomY);
			    putimage(100 * windowData.zoomX, 10 * windowData.zoomY, donate_wc);
			    putimage((100 + 800) * windowData.zoomX, 10 * windowData.zoomY, donate_ap);
			    xyprintf(800 * windowData.zoomX, 830 * windowData.zoomY, L"press any key to close...");
			    delimage(donate_wc);
			    delimage(donate_ap);
			    flushkey();
			    getkey();
		    })
		    .display();
		settings
		    .radius(50 * min(windowData.zoomX, windowData.zoomY))
		    .move(1450 * windowData.zoomX, 150 * windowData.zoomY)
		    .bgimage(pimg[9])  // settings (options.png)
		    .bgsize(75 * windowData.zoomX, 75 * windowData.zoomY)
		    .textalign(CENTER_TEXT, CENTER_TEXT)
		    .bgcolor(WHITE)
		    .framecolor(false, mainColor)
		    .frame(10 * windowData.zoomY)
		    .enableButtonShadow = false;
		settings
		    .display();
		repo
		    .radius(50 * min(windowData.zoomX, windowData.zoomY))
		    .move(1450 * windowData.zoomX, 525 * windowData.zoomY)
		    .bgimage(pimg[10])  // github repo (github.png)
		    .bgsize(75 * windowData.zoomX, 75 * windowData.zoomY)
		    .textalign(CENTER_TEXT, CENTER_TEXT)
		    .bgcolor(WHITE)
		    .framecolor(false, mainColor)
		    .frame(10 * windowData.zoomY)
		    .event([]() -> void { system("start http://github.com/SZXC-WG/LocalGen-new"); })
		    .enableButtonShadow = false;
		repo
		    .display();

		// rectCBOX cbox_test;
		// cbox_test
		// .size(20 * windowData.zoomX, 20 * windowData.zoomY)
		// .move(20 * windowData.zoomX, 20 * windowData.zoomY)
		// .bgcolor(bgColor)
		// .framecolor(0xffffffff)
		// .fillcolor(0xffffffff)
		// .frame(5 * windowData.zoomX);

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
				localOptions();
				break;
			}
			if(web.status == 2) {
				webOptions();
				break;
			}
			if(replay.status == 2) {
				replayPage();
				break;
			}
			if(createmap.status == 2) {
				createMapPage();
				break;
			}
			if(donate.status == 2) {
				donate.clickEvent();
				goto WelcomePageStartLabel;
			}
			if(settings.status == 2) {
				settingsPage();
				goto WelcomePageStartLabel;
			}
			if(repo.status == 2) {
				repo.clickEvent();
				;
			}

			// cbox_test.detect().display();
			// if(cbox_test.status == 2) cbox_test.changeState();
		}
		settextjustify(LEFT_TEXT, TOP_TEXT);
		exit(0);
	}

	void settingsPage() {
		cleardevice();
		flushkey();
		LGset::read();
		for(; is_run(); delay_fps(120)) {
			while(kbmsg()) {
				key_msg msg = getkey();
				if(msg.key == 27) {
					LGset::write();
					return;
				}
			}
			while(mousemsg()) {
				mouse_msg msg = getmouse();
				p_settings.detect(msg);
				// printf("mouse msg:\n");
				// #define TF(v) ((v)?("TRUE"):("FALSE"))
				// printf("is_left(): %s\n", TF(msg.is_left()));
				// printf("is_down(): %s\n", TF(msg.is_down()));
				// fflush(stdout);
			}
			cleardevice();
			p_settings.display(NULL);
			for(auto it: p_settings.gContent()) {
				switch(it.iType) {
					case ITEM_SUBPAGE: {
						it.info.subPage;
					} break;
					// case ITEM_LINETEXT: { it.info.lText; } break;
					// case ITEM_CONDTEXT: { it.info.cdtnText->print(); } break;
					case ITEM_RECTBUTTON: {
						if(it.info.rButton->status == 2) {
							it.info.rButton->clickEvent();
							it.info.rButton->status = 0;
						}
					} break;
					case ITEM_CIRCBUTTON: {
						if(it.info.cButton->status == 2) {
							it.info.cButton->clickEvent();
							it.info.cButton->status = 0;
						}
					} break;
					case ITEM_RECTCHKBOX: {
						if(it.info.rChkBox->status == 2) {
							it.info.rChkBox->changeState();
							it.info.rChkBox->status = 0;
						}
					} break;
					case ITEM_RECTCHKBOX_WITH_TEXT: {
						if(it.info.cBText->checkBox.status == 2) {
							it.info.cBText->checkBox.changeState();
							it.info.cBText->checkBox.status = 0;
						}
					} break;
				}
			}
		}
	}

	void testPage() {
		cleardevice();
		int scrw = windowData.zoomX * 1600, scrh = windowData.zoomY * 900;
		for(int i = 1; i <= 12; ++i) LGgame::team[i].clear();
		int team = 1;
		//receive team id and player names from server
		LGgame::playerNames[1] = username;
		LGgame::team[1].push_back(1);
		LGgame::playerNames[2] = "LocalGen";
		LGgame::playerNames[3] = "123123123123";
		LGgame::playerNames[4] = "234234234234";
		LGgame::playerNames[5] = "345345345345";
		LGgame::playerNames[6] = "456456456456";
		LGgame::playerNames[7] = "567567567567";
		LGgame::playerNames[8] = "678678678678";
		LGgame::playerNames[9] = "789789789789";
		LGgame::playerNames[10] = "890890890890";
		LGgame::playerNames[11] = "901901901901";
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
		rectBUTTON teamButton[13], selectButton, forceStart;
		for(int i = 1; i <= 12; ++i) {
			teamButton[i]
			    .size(50, 30)
			    .move(scrw / 2 + (i - 7) * 50, 70)
			    .addtext(to_wstring(i))
			    .textalign(CENTER_TEXT, CENTER_TEXT)
			    .bgcolor(WHITE)
			    .textcolor(mainColor)
			    .fontname(LGset::mainFontName.c_str())
			    .fontsize(25, 0)
			    .framecolor(false, mainColor);
		}
		selectButton
		    .size(240, 30)
		    .move(scrw / 2 - 120, 30)
		    .addtext(L"Select a custom map")
		    .textalign(CENTER_TEXT, CENTER_TEXT)
		    .bgcolor(WHITE)
		    .textcolor(mainColor)
		    .fontname(LGset::mainFontName.c_str())
		    .fontsize(25, 0)
		    .framecolor(false, mainColor);
		forceStart  //need an on/off button
		    .size(220, 50)
		    .move(scrw / 2 - 110, scrh - 80)
		    .addtext(L"Force Start 0/1")
		    .textalign(CENTER_TEXT, CENTER_TEXT)
		    .bgcolor(WHITE)
		    .textcolor(mainColor)
		    .fontname(LGset::mainFontName.c_str())
		    .fontsize(zoomY(40), 0)
		    .framecolor(false, mainColor);
		for(; is_run(); delay_fps(120)) {
			for(int i = 1; i <= 12; ++i) teamButton[i].detect().display();
			selectButton.detect().display();
			for(int i = 1; i <= 12; ++i) {
				if(teamButton[i].status == 2) {
					//
				}
			}
			if(selectButton.status == 2) {
				//
			}
			int width[13], irow[13], rowWidth[13], rowHeight[13], totRows = 0, begWid[13];
			rowWidth[0] = 3000;
			for(int i = 1; i <= 12; ++i) {
				width[i] = 0;
				if(LGgame::team[i].size() == 0) continue;
				setfont(25, 0, LGset::mainFontName.c_str());
				width[i] = textwidth(("Team " + to_string(i)).c_str());
				setfont(15, 0, LGset::mainFontName.c_str());
				for(auto x: LGgame::team[i]) width[i] = max(width[i], textwidth(LGgame::playerNames[x].c_str()));
				if(rowWidth[totRows] + (width[i] + 35) > scrw - 100) {
					++totRows;
					rowWidth[totRows] = 10;
					rowHeight[totRows] = 0;
				}
				irow[i] = totRows;
				rowWidth[totRows] += width[i] + 35;
				rowHeight[totRows] = max(rowHeight[totRows], int(32 + 15 * LGgame::team[i].size()));
			}
			rowHeight[0] = 10;
			for(int i = 1; i <= totRows; ++i) {
				rowHeight[i] = rowHeight[i - 1] + rowHeight[i] + 10;
				begWid[i] = (scrw - rowWidth[i]) / 2 - 10;
			}
			setfillcolor(0xff333333);
			bar(50, 110, scrw - 50, 110 + rowHeight[totRows]);
			for(int i = 12; i; --i) {
				if(LGgame::team[i].size() == 0) continue;
				setfillcolor(bgColor);
				bar(rowWidth[irow[i]] - width[i] - 25 + begWid[irow[i]], rowHeight[irow[i] - 1] + 110, rowWidth[irow[i]] + begWid[irow[i]], rowHeight[irow[i] - 1] + 15 * LGgame::team[i].size() + 32 + 110);
				setcolor(WHITE);
				setfont(25, 0, LGset::mainFontName.c_str());
				settextjustify(CENTER_TEXT, CENTER_TEXT);
				xyprintf(rowWidth[irow[i]] - (width[i] + 25) / 2 + begWid[irow[i]], rowHeight[irow[i] - 1] + 12 + 110, ("Team " + to_string(i)).c_str());
				setfont(15, 0, LGset::mainFontName.c_str());
				settextjustify(LEFT_TEXT, TOP_TEXT);
				for(int j = 0; j < LGgame::team[i].size(); ++j) {
					setfillcolor(playerInfo[LGgame::team[i][j]].color);
					bar(rowWidth[irow[i]] - width[i] - 25 + 5 + 2 + begWid[irow[i]], rowHeight[irow[i] - 1] + 5 + 20 + j * 15 + 2 + 110,
					    rowWidth[irow[i]] - width[i] - 25 + 5 + 13 + begWid[irow[i]], rowHeight[irow[i] - 1] + 5 + 20 + j * 15 + 13 + 110);
					xyprintf(rowWidth[irow[i]] - width[i] - 25 + 20 + begWid[irow[i]], rowHeight[irow[i] - 1] + 5 + 20 + j * 15 + 110, LGgame::playerNames[LGgame::team[i][j]].c_str());
				}
				rowWidth[irow[i]] -= width[i] + 35;
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
		static const int butH = 150;
		for(int i = 1; i <= mapNum; ++i) {
			mapbut[i]
			    .size(300 * windowData.zoomX - 3, butH * windowData.zoomY - 3)
			    .move(((i - 1) % 5 * 300 + 50) * windowData.zoomX, ((i - 1) / 5 * butH + 100 + shiftval) * windowData.zoomY)
			    .bgcolor(bgColor)
			    .textcolor(WHITE)
			    .textalign(CENTER_TEXT, CENTER_TEXT)
			    .fontname(LGset::mainFontName.c_str())
			    .fontsize(16 * windowData.zoomY, 0)
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
		settextjustify(CENTER_TEXT, TOP_TEXT);
		setcolor(WHITE);
		setfont(100 * windowData.zoomY, 0, LGset::mainFontName.c_str());
		setlinewidth(5 * windowData.zoomY);
		delay_ms(50);
		flushmouse();
		mapSelected = 0;
		mouse_msg msg;
		for(; is_run(); delay_fps(120)) {
			cleardevice();
			while(mousemsg()) {
				int id = int((msg.y / windowData.zoomY - 100 - shiftval) / butH) * 5 + int((msg.x / windowData.zoomX - 50) / 300) + 1;
				mapbut[id].status = 0;
				msg = getmouse();
				shiftval += msg.wheel;
				if(shiftval > 0) shiftval = 0;
				if(shiftval < -((mapNum - 1) / 5 * butH)) shiftval = -((mapNum - 1) / 5 * butH);
				if(msg.x < 50 * windowData.zoomX || msg.x > 1550 * windowData.zoomX || msg.y < 100 * windowData.zoomY || msg.y > 900 * windowData.zoomY) continue;
				id = int((msg.y / windowData.zoomY - 100 - shiftval) / butH) * 5 + int((msg.x / windowData.zoomX - 50) / 300) + 1;
				if(msg.is_left() && msg.is_down()) mapbut[id].status = 2;
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
				    .addtext(mapInfo[i].chiname)
				    .addtext(mapInfo[i].engname)
				    .addtext(L"General Count: " + (~mapInfo[i].generalcnt ? to_wstring(mapInfo[i].generalcnt) : L"2~12"))
				    .addtext(L"Plain Count: " + (~mapInfo[i].plaincnt ? to_wstring(mapInfo[i].plaincnt) : L"undetermined"))
				    .addtext(L"City Count: " + (~mapInfo[i].citycnt ? to_wstring(mapInfo[i].citycnt) : L"undetermined"))
				    .addtext(L"Mountain Count: " + (~mapInfo[i].mountaincnt ? to_wstring(mapInfo[i].mountaincnt) : L"undetermined"))
				    .addtext(L"Swamp Count: " + (~mapInfo[i].swampcnt ? to_wstring(mapInfo[i].swampcnt) : L"undetermined"))
				    .addtext(L"Size: " + (~mapInfo[i].height ? to_wstring(mapInfo[i].height) : L"undetermined") + L" × " + (~mapInfo[i].width ? to_wstring(mapInfo[i].width) : L"undetermined"))
				    .move(((i - 1) % 5 * 300 + 50) * windowData.zoomX, ((i - 1) / 5 * butH + 100 + shiftval) * windowData.zoomY)
				    .display();
				if(mapbut[i].status == 2) mapSelected = i;
			}
			setfillcolor(bgColor);
			bar(0, 0, 1600 * windowData.zoomX, 100 * windowData.zoomY);
			xyprintf(800 * windowData.zoomX, 0, "CHOOSE A MAP FROM BELOW:");
			line(0, 100 * windowData.zoomY, 1600 * windowData.zoomX, 100 * windowData.zoomY);
			if(mapSelected) break;
		}
		flushkey();

		/** game options **/

		doMapSelect();

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
		int shiftval = 0, ret;

		settextjustify(CENTER_TEXT, CENTER_TEXT);
		setfont(50 * windowData.zoomY, 0, LGset::mainFontName.c_str());
		setlinewidth(1);

		serverBox
		    .size(200 * windowData.zoomX, 100 * windowData.zoomY)
		    .move(400 * windowData.zoomX, 350 * windowData.zoomY)
		    .textalign(CENTER_TEXT, CENTER_TEXT)
		    .fontname(LGset::mainFontName.c_str())
		    .fontsize(50 * windowData.zoomY, 0)
		    .bgcolor(WHITE)
		    .textcolor(mainColor)
		    .addtext(L"Server")
		    .status = 0;
		serverBox.enableButtonShadow = true;

		clientBox
		    .size(200 * windowData.zoomX, 100 * windowData.zoomY)
		    .move(800 * windowData.zoomX, 350 * windowData.zoomY)
		    .textalign(CENTER_TEXT, CENTER_TEXT)
		    .fontname(LGset::mainFontName.c_str())
		    .fontsize(50 * windowData.zoomY, 0)
		    .bgcolor(WHITE)
		    .textcolor(mainColor)
		    .addtext(L"Client")
		    .status = 0;
		clientBox.enableButtonShadow = true;

		serverBox.display();
		clientBox.display();
		setcolor(WHITE);

		for(; is_run(); delay_fps(120)) {
			while(mousemsg()) {
				serverBox.status = 0;
				clientBox.status = 0;
				mouse_msg msg = getmouse();

				if(msg.x >= 400 * windowData.zoomX && msg.x <= 600 * windowData.zoomY && msg.y >= 350 * windowData.zoomY && msg.y <= 450 * windowData.zoomY) {
					serverBox.status = 1;

					if(msg.is_left()) serverBox.status = 2;
					continue;
				}

				if(msg.x >= 800 * windowData.zoomX && msg.x <= 1000 * windowData.zoomY && msg.y >= 350 * windowData.zoomY && msg.y <= 450 * windowData.zoomY) {
					clientBox.status = 1;

					if(msg.is_left()) clientBox.status = 2;
					continue;
				}
			}

			serverBox.display();
			clientBox.display();

			if(serverBox.status == 2) goto serverOptions;
			if(clientBox.status == 2) goto clientOptions;
		}

	serverOptions:;

		for(int i = 1; i <= mapNum; ++i) {
			mapbut[i]
			    .size(300 * windowData.zoomX - 3, 200 * windowData.zoomY - 3)
			    .move(((i - 1) % 4 * 300) * windowData.zoomX, ((i - 1) / 4 * 200 + shiftval) * windowData.zoomY)
			    .bgcolor(bgColor)
			    .textcolor(WHITE)
			    .textalign(CENTER_TEXT, CENTER_TEXT)
			    .fontname(LGset::mainFontName.c_str())
			    .fontsize(22 * windowData.zoomY, 0)
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
		impbox.move(1250 * windowData.zoomX, 100 * windowData.zoomY);
		impbox.size(300 * windowData.zoomX, 700 * windowData.zoomY);
		impbox.setbgcolor(WHITE);
		impbox.setcolor(mainColor);
		impbox.setfont(30 * windowData.zoomY, 0, LGset::mainFontName.c_str());
		impbox.visible(true);

		impfin
		    .textalign(CENTER_TEXT, CENTER_TEXT)
		    .move(1250 * windowData.zoomX, 825 * windowData.zoomY)
		    .size(300 * windowData.zoomX, 50 * windowData.zoomY)
		    .bgcolor(WHITE)
		    .textcolor(mainColor)
		    .fontname(LGset::mainFontName.c_str())
		    .fontsize(zoomY(40) * windowData.zoomY, 0)
		    .addtext(L"confirm and submit")
		    .display();
		settextjustify(CENTER_TEXT, CENTER_TEXT);
		setcolor(WHITE);
		setfont(50 * windowData.zoomY, 0, LGset::mainFontName.c_str());
		delay_ms(50);
		xyprintf(1400 * windowData.zoomX, 50 * windowData.zoomY, "or import a map...");
		delay_ms(0);
		delay_ms(50);
		flushmouse();
		mapSelected = 0;
		mouse_msg msg;
		for(; is_run(); delay_fps(120)) {
			while(mousemsg()) {
				int id = int((msg.y - shiftval * windowData.zoomX) / (200 * windowData.zoomY)) * 4 + int(msg.x / (300 * windowData.zoomX)) + 1;
				mapbut[id].status = 0;
				impfin.status = 0;
				msg = getmouse();
				shiftval += msg.wheel;
				if(shiftval > 0) shiftval = 0;
				if(shiftval < -(mapNum - 1) / 4 * 200) shiftval = -(mapNum - 1) / 4 * 200;
				if(msg.x >= 1250 * windowData.zoomX && msg.x <= 1550 * windowData.zoomY && msg.y >= 825 * windowData.zoomY && msg.y <= 875 * windowData.zoomY) {
					impfin.status = 1;
					if(msg.is_left()) impfin.status = 2;
					continue;
				}
				if(msg.x < 0 || msg.x > 1200 * windowData.zoomX || msg.y < 0 || msg.y > 900 * windowData.zoomY) continue;
				id = int((msg.y - shiftval * windowData.zoomX) / (200 * windowData.zoomY)) * 4 + int(msg.x / (300 * windowData.zoomX)) + 1;
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
				    .addtext(mapInfo[i].chiname)
				    .addtext(mapInfo[i].engname)
				    .addtext(L"General Count: " + (~mapInfo[i].generalcnt ? to_wstring(mapInfo[i].generalcnt) : L"2~12"))
				    .addtext(L"Plain Count: " + (~mapInfo[i].plaincnt ? to_wstring(mapInfo[i].plaincnt) : L"undetermined"))
				    .addtext(L"City Count: " + (~mapInfo[i].citycnt ? to_wstring(mapInfo[i].citycnt) : L"undetermined"))
				    .addtext(L"Mountain Count: " + (~mapInfo[i].mountaincnt ? to_wstring(mapInfo[i].mountaincnt) : L"undetermined"))
				    .addtext(L"Swamp Count: " + (~mapInfo[i].swampcnt ? to_wstring(mapInfo[i].swampcnt) : L"undetermined"))
				    .addtext(L"Size: " + (~mapInfo[i].height ? to_wstring(mapInfo[i].height) : L"undetermined") + L" × " + (~mapInfo[i].width ? to_wstring(mapInfo[i].width) : L"undetermined"))
				    .move(((i - 1) % 4 * 300) * windowData.zoomX, ((i - 1) / 4 * 200 + shiftval) * windowData.zoomY)
				    .display();
				if(mapbut[i].status == 2) mapSelected = i;
			}
			impfin.display();
			if(impfin.status == 2) {
				std::array<char, 1000> s;
				impbox.gettext(1000, s.data());
				std::ifstream fin(s.data());
				if(!fin) {
					impfin.poptext().addtext(L"map not found").display();
					delay_ms(100);
					impfin.poptext().addtext(L"confirm and submit").display();
					impfin.status = 1;
					continue;
				} else {
					fin >> strdeZip;
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
		return;

	clientOptions:;
		ret = LGclient::GAME();
		return;
	}

	void replayPage() {
		cleardevice();
		setrendermode(RENDER_MANUAL);
		LGreplay::rreplay.initReplay();
		for(int i = 1; i <= LGgame::playerCnt; ++i) LGgame::isAlive[i] = 1;
		printMap(1048575, { -1, -1 });

		rectBUTTON jumpbut;
		sys_edit jumpbox;
		rectBUTTON jumpsmbut;
		rectBUTTON stepbybut;
		rectBUTTON exitbut;
		rectBUTTON backbut;
		rectBUTTON nextbut;
		rectBUTTON autobut;

		jumpbut.textalign(CENTER_TEXT, CENTER_TEXT)
		    .bgcolor(WHITE)
		    .textcolor(BLACK)
		    .move(0 * windowData.zoomX, 35 * windowData.zoomY)
		    .fontname(LGset::mainFontName.c_str())
		    .fontsize(30 * windowData.zoomY, 0)
		    .size(150 * windowData.zoomX, 30 * windowData.zoomY)
		    .addtext(L"Jump to turn: ")
		    .enableTextShadow = false;
		jumpbox.create(false);
		jumpbox.size(50 * windowData.zoomX, 30 * windowData.zoomY);
		jumpbox.move(150 * windowData.zoomX, 35 * windowData.zoomY);
		jumpbox.setfont(25 * windowData.zoomY, 0, LGset::mainFontName.c_str());
		jumpbox.visible(true);
		jumpsmbut.textalign(CENTER_TEXT, CENTER_TEXT)
		    .bgcolor(WHITE)
		    .textcolor(BLACK)
		    .move(200 * windowData.zoomX, 35 * windowData.zoomY)
		    .fontname(LGset::mainFontName.c_str())
		    .fontsize(30 * windowData.zoomY, 0)
		    .size(50 * windowData.zoomX, 30 * windowData.zoomY)
		    .addtext(L"→")
		    .enableTextShadow = false;

		int smsx = 0, smsy = 0, midact = 0;
		for(; is_run();) {
			while(mousemsg()) {
				mouse_msg msg = getmouse();
				if(msg.is_wheel()) {
					blockWidth += msg.wheel / 120;
					blockHeight += msg.wheel / 120;
					blockWidth = max(blockWidth, 2);
					blockHeight = max(blockHeight, 2);
				}
				if(msg.is_move()) {
					if(midact == 1) {
						LGGraphics::windowData.maplocX += msg.x - smsx;
						LGGraphics::windowData.maplocY += msg.y - smsy;
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
			printMap(1048575, { -1, -1 });
			const static int screenszr = 1600 * LGGraphics::windowData.zoomX;
			static int fpslen;
			static int turnlen;
			setfillcolor(LGGraphics::bgColor);
			bar(screenszr - 10 - fpslen - 10 - turnlen - 10, 0, screenszr, 20 * LGGraphics::windowData.zoomY);
			setfont(20 * LGGraphics::windowData.zoomY, 0, LGset::mainFontName.c_str());
			fpslen = textwidth(("FPS: " + to_string(getfps())).c_str());
			turnlen = textwidth(("Turn " + to_string(LGreplay::rreplay.curTurn) + ".").c_str());
			setfillcolor(RED);
			bar(screenszr - fpslen - 10, 0, screenszr, 20 * LGGraphics::windowData.zoomY);
			rectangle(screenszr - fpslen - 10, 0, screenszr, 20 * LGGraphics::windowData.zoomY);
			setfillcolor(BLUE);
			bar(screenszr - fpslen - 10 - turnlen - 10, 0, screenszr - fpslen - 10, 20 * LGGraphics::windowData.zoomY);
			rectangle(screenszr - fpslen - 10 - turnlen - 10, 0, screenszr - fpslen - 10, 20 * LGGraphics::windowData.zoomY);
			settextjustify(CENTER_TEXT, TOP_TEXT);
			xyprintf(screenszr - fpslen / 2 - 5, 0, "FPS: %f", getfps());
			xyprintf(screenszr - fpslen - 10 - turnlen / 2 - 5, 0, "Turn %d.", LGreplay::rreplay.curTurn);
			LGgame::statistics();
			LGgame::ranklist(true);
			jumpbut.display();
			jumpsmbut.detect().display();
			if(jumpsmbut.status == 2) {
				char s[101];
				jumpbox.gettext(sizeof(s) - 1, s);
				int toTurn = atoi(s);
				if(toTurn != 0) LGreplay::rreplay.gotoTurn(toTurn);
			}
			delay_ms(0);
		}
	}

	void createMapPage() {
		LGgame::inCreate = 1;
		cleardevice();
		setrendermode(RENDER_MANUAL);
		settextjustify(LEFT_TEXT, TOP_TEXT);
		int scrw = windowData.zoomX * 1600, scrh = windowData.zoomY * 900;
		sys_edit citynumBox, plainnumBox, savenameBox;
		rectBUTTON saveButton, cancelButton, loadButton;
		citynumBox.create();
		citynumBox.size(zoomX(80), zoomY(30));
		citynumBox.move(scrw / 2 + zoomX(5), scrh - zoomY(110));
		citynumBox.setfont(zoomY(20), 0, LGset::mainFontName.c_str());
		citynumBox.setcolor(mainColor);
		citynumBox.settext("40");
		plainnumBox.create();
		plainnumBox.size(zoomX(80), zoomY(30));
		plainnumBox.move(scrw / 2 + zoomX(35), scrh - zoomY(110));
		plainnumBox.setfont(zoomY(20), 0, LGset::mainFontName.c_str());
		plainnumBox.setcolor(mainColor);
		plainnumBox.settext("40");
		savenameBox.create();
		savenameBox.size(zoomX(100), zoomY(40));
		savenameBox.move(scrw / 2 + zoomX(20), scrh / 2 - zoomY(20));
		savenameBox.setfont(zoomY(30), 0, LGset::mainFontName.c_str());
		savenameBox.setcolor(mainColor);
		savenameBox.settext("map");
		saveButton
		    .size(zoomX(90), zoomY(40))
		    .move(scrw / 2 - zoomX(100), scrh / 2 + zoomY(30))
		    .fontname(LGset::mainFontName.c_str())
		    .fontsize(zoomY(40), 0)
		    .bgcolor(mainColor)
		    .textcolor(WHITE)
		    .textalign(CENTER_TEXT, CENTER_TEXT)
		    .addtext(L"Save");
		cancelButton
		    .size(zoomX(90), zoomY(40))
		    .move(scrw / 2 + zoomX(10), scrh / 2 + zoomY(30))
		    .fontname(LGset::mainFontName.c_str())
		    .fontsize(zoomY(40), 0)
		    .bgcolor(mainColor)
		    .textcolor(WHITE)
		    .textalign(CENTER_TEXT, CENTER_TEXT)
		    .addtext(L"Cancel");
		loadButton
		    .size(zoomX(90), zoomY(40))
		    .move(scrw / 2 - zoomX(100), scrh / 2 + zoomY(30))
		    .fontname(LGset::mainFontName.c_str())
		    .fontsize(zoomY(40), 0)
		    .bgcolor(mainColor)
		    .textcolor(WHITE)
		    .textalign(CENTER_TEXT, CENTER_TEXT)
		    .addtext(L"Load");
		mapW = mapH = 10;
		for(int i = 1; i <= mapH; ++i)
			for(int j = 1; j <= mapW; ++j) gameMap[i][j] = { 0, 0, 0, 0 };
		printMap(1048575, { -1, -1 });
		createOptions(0, scrh / 2 - zoomY(140));
		setfillcolor(mainColor);
		int smsx = 0, smsy = 0, midact = 0, type = 0, citynum = 40, plainnum = 40;
		bool moved = false, saved = false;
		std::chrono::steady_clock::duration prsttm;
		for(; is_run(); delay_fps(120)) {
			while(mousemsg()) {
				mouse_msg msg = getmouse();
				if(msg.is_wheel()) {
					blockWidth += msg.wheel / 120;
					blockHeight += msg.wheel / 120;
					blockWidth = max(blockWidth, 2);
					blockHeight = max(blockHeight, 2);
				}
				if(msg.is_move()) {
					if(midact == 1) {
						LGGraphics::windowData.maplocX += msg.x - smsx;
						LGGraphics::windowData.maplocY += msg.y - smsy;
						smsx = msg.x, smsy = msg.y;
						moved = true;
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
							if(msg.x < zoomX(40) && msg.y >= scrh / 2 - zoomY(140) && msg.y < scrh / 2 + zoomY(140))
								type = (msg.y - (scrh / 2 - zoomY(140))) / zoomY(40);
							else if(msg.x >= scrw / 2 - zoomX(145) && msg.x < scrw / 2 - zoomX(5) && msg.y >= scrh - zoomY(60) && msg.y < scrh - zoomY(20)) {
								settextjustify(CENTER_TEXT, CENTER_TEXT);
								setfillcolor(WHITE);
								setcolor(BLACK);
								bar(scrw / 2 - zoomX(130), scrh / 2 - zoomY(80), scrw / 2 + zoomX(130), scrh / 2 + zoomY(80));
								setfont(zoomY(40), 0, LGset::mainFontName.c_str());
								xyprintf(scrw / 2, scrh / 2 - zoomY(50), "Save map");
								xyprintf(scrw / 2 - zoomX(55), scrh / 2, "Map name:");
								savenameBox.visible(true);
								setfillcolor(mainColor);
								saveButton.display();
								cancelButton.display();
								delay_ms(0);
								for(; is_run(); delay_fps(120)) {
									saveButton.detect().display();
									cancelButton.detect().display();
									if(saveButton.status == 2) {
										char s[100];
										savenameBox.gettext(sizeof(s), s);
										string ss = s;
										Zip();
										freopen(("maps/" + ss + ".lg").c_str(), "w", stdout);
										printf("%s", strZip);
										fclose(stdout);
										saved = true;
										LGgame::inCreate = 0;
										return;
									}
									if(cancelButton.status == 2) break;
								}
								savenameBox.visible(false);
							} else if(msg.x >= scrw / 2 + zoomX(5) && msg.x < scrw / 2 + zoomX(145) && msg.y >= scrh - zoomY(60) && msg.y < scrh - zoomY(20)) {
								settextjustify(CENTER_TEXT, CENTER_TEXT);
								setfillcolor(WHITE);
								setcolor(BLACK);
								bar(scrw / 2 - zoomX(130), scrh / 2 - zoomY(80), scrw / 2 + zoomX(130), scrh / 2 + zoomY(80));
								setfont(zoomY(40), 0, LGset::mainFontName.c_str());
								xyprintf(scrw / 2, scrh / 2 - zoomY(50), "Load map");
								xyprintf(scrw / 2 - zoomX(55), scrh / 2, "Map name:");
								savenameBox.visible(true);
								setfillcolor(mainColor);
								loadButton.display();
								cancelButton.display();
								delay_ms(0);
								for(; is_run(); delay_fps(120)) {
									loadButton.detect().display();
									cancelButton.detect().display();
									if(loadButton.status == 2) {
										char s[100];
										savenameBox.gettext(sizeof(s), s);
										string ss = s;
										FILE* file;
										file = fopen(("maps/" + ss + ".lg").c_str(), "r");
										if(!file) {
											MessageBoxA(nullptr, "Map not found!", "Local Generals", MB_OK);
											continue;
										}
										fread(strdeZip, 1, LEN_ZIP, file);
										deZip();
										break;
									}
									if(cancelButton.status == 2) break;
								}
								savenameBox.visible(false);
							} else if(msg.x >= scrw / 2 - zoomX(200) && msg.x < scrw / 2 - zoomX(170) && msg.y >= zoomY(10) && msg.y < zoomY(40)) {
								if(mapH > 1) --mapH;
							} else if(msg.x >= scrw / 2 - zoomX(50) && msg.x < scrw / 2 - zoomY(20) && msg.y >= zoomY(10) && msg.y < zoomY(40)) {
								if(mapH < 100) {
									++mapH;
									for(int i = 1; i <= mapW; ++i) gameMap[mapH][i] = { 0, 0, 0, 0 };
								}
							} else if(msg.x >= scrw / 2 + zoomX(20) && msg.x < scrw / 2 + zoomX(50) && msg.y >= zoomY(10) && msg.y < zoomY(40)) {
								if(mapW > 1) --mapW;
							} else if(msg.x >= scrw / 2 + zoomX(170) && msg.x < scrw / 2 + zoomX(200) && msg.y >= zoomY(10) && msg.y < zoomY(40)) {
								if(mapW < 100) {
									++mapW;
									for(int i = 1; i <= mapH; ++i) gameMap[i][mapW] = { 0, 0, 0, 0 };
								}
							} else if(msg.x >= LGGraphics::windowData.maplocX &&
							          msg.y >= LGGraphics::windowData.maplocY &&
							          msg.x <= LGGraphics::windowData.maplocX + blockWidth * mapW &&
							          msg.y <= LGGraphics::windowData.maplocY + blockHeight * mapH) {
								int lin = (msg.y + blockHeight - 1 - LGGraphics::windowData.maplocY) / blockHeight;
								int col = (msg.x + blockWidth - 1 - LGGraphics::windowData.maplocX) / blockWidth;
								switch(type) {
									case 0:
										gameMap[lin][col].player = 0;
										gameMap[lin][col].type = 2;
										gameMap[lin][col].army = 0;
										break;
									case 1:
										gameMap[lin][col].player = 0;
										gameMap[lin][col].type = 1;
										gameMap[lin][col].army = 0;
										break;
									case 2:
										gameMap[lin][col].player = 0;
										gameMap[lin][col].type = 3;
										gameMap[lin][col].army = 0;
										break;
									case 3:
										gameMap[lin][col].player = 0;
										gameMap[lin][col].type = 4;
										gameMap[lin][col].army = citynum;
										break;
									case 4:
										gameMap[lin][col].player = 0;
										gameMap[lin][col].type = 0;
										gameMap[lin][col].army = plainnum;
										break;
									case 5:
										gameMap[lin][col].lit = !gameMap[lin][col].lit;
										break;
									case 6:
										gameMap[lin][col].player = 0;
										gameMap[lin][col].type = 0;
										gameMap[lin][col].army = 0;
										break;
								}
							}
						}
					}
				}
			}
			cleardevice();
			printMap(1048575, { -1, -1 });
			createOptions(type, scrh / 2 - zoomY(140));
			settextjustify(CENTER_TEXT, CENTER_TEXT);
			setcolor(BLACK);
			setfillcolor(WHITE);
			setfont(zoomY(20), 0, LGset::mainFontName.c_str());
			if(type == 0) {
				bar(scrw / 2 - zoomX(105), scrh - zoomY(110), scrw / 2 + zoomX(105), scrh - zoomY(80));
				xyprintf(scrw / 2, scrh - zoomY(95), "Click a tile to place a mountain.");
			}
			if(type == 1) {
				bar(scrw / 2 - zoomX(105), scrh - zoomY(130), scrw / 2 + zoomX(105), scrh - zoomY(80));
				xyprintf(scrw / 2, scrh - zoomY(115), "Click a tile to place a swamp.");
				xyprintf(scrw / 2, scrh - zoomY(95), "Swamps drain 1 army per turn.");
			}
			if(type == 5) {
				bar(scrw / 2 - zoomX(95), scrh - zoomY(110), scrw / 2 + zoomX(95), scrh - zoomY(80));
				xyprintf(scrw / 2, scrh - zoomY(95), "Click a tile to toggle light tile.");
			}
			if(type == 6) {
				bar(scrw / 2 - zoomX(80), scrh - zoomY(110), scrw / 2 + zoomX(80), scrh - zoomY(80));
				xyprintf(scrw / 2, scrh - zoomY(95), "Click a tile to remove it.");
			}
			if(type == 3) {
				citynumBox.visible(true);
				bar(scrw / 2 - zoomX(85), scrh - zoomY(110), scrw / 2 + zoomX(5), scrh - zoomY(80));
				xyprintf(scrw / 2 - zoomX(40), scrh - zoomY(95), "City Strength:");
				char s[10];
				citynumBox.gettext(sizeof(s), s);
				sscanf(s, "%d", &citynum);
			} else citynumBox.visible(false);
			if(type == 4) {
				plainnumBox.visible(true);
				bar(scrw / 2 - zoomX(115), scrh - zoomY(110), scrw / 2 + zoomX(35), scrh - zoomY(80));
				xyprintf(scrw / 2 - zoomX(40), scrh - zoomY(95), "Neutral Army Strength:");
				char s[10];
				plainnumBox.gettext(sizeof(s), s);
				sscanf(s, "%d", &plainnum);
			} else plainnumBox.visible(false);
			setfillcolor(WHITE);
			setcolor(BLACK);
			setfont(zoomY(30), 0, LGset::mainFontName.c_str());
			bar(scrw / 2 - zoomX(200), zoomY(10), scrw / 2 - zoomX(20), zoomY(40));
			xyprintf(scrw / 2 - zoomX(110), zoomY(25), "Height: %d", mapH);
			xyprintf(scrw / 2 - zoomX(185), zoomY(25), "-");
			xyprintf(scrw / 2 - zoomX(35), zoomY(25), "+");
			bar(scrw / 2 + zoomX(20), zoomY(10), scrw / 2 + zoomX(200), zoomY(40));
			xyprintf(scrw / 2 + zoomX(110), zoomY(25), "Width: %d", mapW);
			xyprintf(scrw / 2 + zoomX(35), zoomY(25), "-");
			xyprintf(scrw / 2 + zoomX(185), zoomY(25), "+");
			bar(scrw / 2 - zoomX(155), scrh - zoomY(70), scrw / 2 + zoomX(155), scrh - zoomY(10));
			setfillcolor(mainColor);
			setcolor(WHITE);
			setfont(zoomY(40), 0, LGset::mainFontName.c_str());
			bar(scrw / 2 - zoomX(145), scrh - zoomY(60), scrw / 2 - zoomX(5), scrh - zoomY(20));
			xyprintf(scrw / 2 - zoomX(75), scrh - zoomY(40), "Save map");
			bar(scrw / 2 + zoomX(5), scrh - zoomY(60), scrw / 2 + zoomX(145), scrh - zoomY(20));
			xyprintf(scrw / 2 + zoomX(75), scrh - zoomY(40), "Load map");
			settextjustify(LEFT_TEXT, TOP_TEXT);
		}
	}

	void doMapSelect() {
		cleardevice();
		setrendermode(RENDER_AUTO);
		// setfillcolor(WHITE);
		// bar(5 * windowData.mapSizeX, 5 * windowData.mapSizeY,
		//     505 * windowData.mapSizeX, 305 * windowData.mapSizeY);
		setcolor(WHITE);
		setlinewidth(5 * windowData.zoomY);
		rectangle(5 * windowData.zoomX, 5 * windowData.zoomY,
		          505 * windowData.zoomX, 305 * windowData.zoomY);
		setlinewidth(1);
		setcolor(mainColor);
		setfont(40 * windowData.zoomY, 0, LGset::mainFontName.c_str(), 0, 0, FW_BOLD, false, false, false);
		settextjustify(CENTER_TEXT, TOP_TEXT);
		xyprintf(255 * windowData.zoomX, 5 * windowData.zoomY,
		         L"%ls", mapInfo[mapSelected].chiname.c_str());
		xyprintf(255 * windowData.zoomX, 45 * windowData.zoomY,
		         L"%ls", mapInfo[mapSelected].engname.c_str());
		setcolor(WHITE);
		setfont(30 * windowData.zoomY, 0, LGset::mainFontName.c_str());
		xyprintf(255 * windowData.zoomX, 85 * windowData.zoomY,
		         L"Author: %ls", mapInfo[mapSelected].auth.c_str());
		xyprintf(255 * windowData.zoomX, 115 * windowData.zoomY,
		         L"Size: %d × %d", mapInfo[mapSelected].height, mapInfo[mapSelected].width);
		xyprintf(255 * windowData.zoomX, 145 * windowData.zoomY,
		         L"General Count: %d", mapInfo[mapSelected].generalcnt);
		xyprintf(255 * windowData.zoomX, 175 * windowData.zoomY,
		         L"Plain Count: %d", mapInfo[mapSelected].plaincnt);
		xyprintf(255 * windowData.zoomX, 205 * windowData.zoomY,
		         L"City Count: %d", mapInfo[mapSelected].citycnt);
		xyprintf(255 * windowData.zoomX, 235 * windowData.zoomY,
		         L"Mountain Count: %d", mapInfo[mapSelected].mountaincnt);
		xyprintf(255 * windowData.zoomX, 265 * windowData.zoomY,
		         L"Swamp Count: %d", mapInfo[mapSelected].swampcnt);
		sys_edit heiinput, widinput, amninput, amxinput;
		int height, width, armymin, armymax;
		heiinput.create();
		widinput.create();
		amninput.create();
		amxinput.create();
		if(mapSelected < 9) {
			setcolor(WHITE);
			setfont(40 * windowData.zoomY, 0, LGset::mainFontName.c_str());
			settextjustify(RIGHT_TEXT, TOP_TEXT);
			xyprintf(800 * windowData.zoomX, 5 * windowData.zoomY, L"Input Height (<=100):");
			xyprintf(800 * windowData.zoomX, 45 * windowData.zoomY, L"Input Width (<=100):");
			if(mapSelected == MAP_CITY_ID) {
				xyprintf(800 * windowData.zoomX, 85 * windowData.zoomY, L"Input MINIMUM Army:");
				xyprintf(800 * windowData.zoomX, 125 * windowData.zoomY, L"Input MAXIMUM Army:");
			}
			heiinput.move(810 * windowData.zoomX, 6 * windowData.zoomY);
			heiinput.size(200 * windowData.zoomX, 38 * windowData.zoomY);
			heiinput.setbgcolor(WHITE);
			heiinput.setcolor(mainColor);
			heiinput.setfont(35 * windowData.zoomY, 0, LGset::mainFontName.c_str());
			widinput.move(810 * windowData.zoomX, 46 * windowData.zoomY);
			widinput.size(200 * windowData.zoomX, 38 * windowData.zoomY);
			widinput.setbgcolor(WHITE);
			widinput.setcolor(mainColor);
			widinput.setfont(35 * windowData.zoomY, 0, LGset::mainFontName.c_str());
			if(mapSelected == MAP_CITY_ID) {
				amninput.move(810 * windowData.zoomX, 86 * windowData.zoomY);
				amninput.size(200 * windowData.zoomX, 38 * windowData.zoomY);
				amninput.setbgcolor(WHITE);
				amninput.setcolor(mainColor);
				amninput.setfont(35 * windowData.zoomY, 0, LGset::mainFontName.c_str());
				amxinput.move(810 * windowData.zoomX, 126 * windowData.zoomY);
				amxinput.size(200 * windowData.zoomX, 38 * windowData.zoomY);
				amxinput.setbgcolor(WHITE);
				amxinput.setcolor(mainColor);
				amxinput.setfont(35 * windowData.zoomY, 0, LGset::mainFontName.c_str());
			}
			heiinput.visible(true);
			widinput.visible(true);
			if(mapSelected == MAP_CITY_ID) {
				amninput.visible(true);
				amxinput.visible(true);
			}
			rectBUTTON heib, widb, amnb, amxb, endb;
			heib
			    .move(1020 * windowData.zoomX, 6 * windowData.zoomY)
			    .size(100 * windowData.zoomX, 38 * windowData.zoomY)
			    .bgcolor(WHITE)
			    .textcolor(mainColor)
			    .textalign(CENTER_TEXT, CENTER_TEXT)
			    .fontsize(35 * windowData.zoomY, 0)
			    .fontname(LGset::mainFontName.c_str())
			    .addtext(L"confirm");
			widb
			    .move(1020 * windowData.zoomX, 46 * windowData.zoomY)
			    .size(100 * windowData.zoomX, 38 * windowData.zoomY)
			    .bgcolor(WHITE)
			    .textcolor(mainColor)
			    .textalign(CENTER_TEXT, CENTER_TEXT)
			    .fontsize(35 * windowData.zoomY, 0)
			    .fontname(LGset::mainFontName.c_str())
			    .addtext(L"confirm");
			if(mapSelected == MAP_CITY_ID) {
				amnb
				    .move(1020 * windowData.zoomX, 86 * windowData.zoomY)
				    .size(100 * windowData.zoomX, 38 * windowData.zoomY)
				    .bgcolor(WHITE)
				    .textcolor(mainColor)
				    .textalign(CENTER_TEXT, CENTER_TEXT)
				    .fontsize(35 * windowData.zoomY, 0)
				    .fontname(LGset::mainFontName.c_str())
				    .addtext(L"confirm");
				amxb
				    .move(1020 * windowData.zoomX, 126 * windowData.zoomY)
				    .size(100 * windowData.zoomX, 38 * windowData.zoomY)
				    .bgcolor(WHITE)
				    .textcolor(mainColor)
				    .textalign(CENTER_TEXT, CENTER_TEXT)
				    .fontsize(35 * windowData.zoomY, 0)
				    .fontname(LGset::mainFontName.c_str())
				    .addtext(L"confirm");
			}
			endb
			    .move(810 * windowData.zoomX, 166 * windowData.zoomY)
			    .size(310 * windowData.zoomX, 38 * windowData.zoomY)
			    .bgcolor(WHITE)
			    .textcolor(mainColor)
			    .textalign(CENTER_TEXT, CENTER_TEXT)
			    .fontsize(35 * windowData.zoomY, 0)
			    .fontname(LGset::mainFontName.c_str())
			    .addtext(L"end input");
			heib.display();
			widb.display();
			if(mapSelected == MAP_CITY_ID) {
				amnb.display();
				amxb.display();
			}
			endb.display();
			bool hb, wb, nb, xb;
			hb = wb = nb = xb = false;
			for(; is_run(); delay_fps(60)) {
				heib.detect().display();
				widb.detect().display();
				if(mapSelected == MAP_CITY_ID) {
					amnb.detect().display();
					amxb.detect().display();
				}
				endb.detect().display();
				if(heib.status == 2) {
					char s[55];
					heiinput.gettext(sizeof(s), s);
					int p = sscanf(s, "%d", &height);
					if(p != 1) {
						heib.poptext().addtext(L"invalid").display();
						delay_ms(100);
						heib.poptext().addtext(L"confirm").display();
					} else hb = true;
				}
				if(widb.status == 2) {
					char s[55];
					widinput.gettext(sizeof(s), s);
					int p = sscanf(s, "%d", &width);
					if(p != 1) {
						widb.poptext().addtext(L"invalid").display();
						delay_ms(100);
						widb.poptext().addtext(L"confirm").display();
					} else wb = true;
				}
				if(amnb.status == 2) {
					char s[55];
					amninput.gettext(sizeof(s), s);
					int p = sscanf(s, "%d", &armymin);
					if(p != 1) {
						amnb.poptext().addtext(L"invalid").display();
						delay_ms(100);
						amnb.poptext().addtext(L"confirm").display();
					} else nb = true;
				}
				if(amxb.status == 2) {
					char s[55];
					amxinput.gettext(sizeof(s), s);
					int p = sscanf(s, "%d", &armymax);
					if(p != 1) {
						amxb.poptext().addtext(L"invalid").display();
						delay_ms(100);
						amxb.poptext().addtext(L"confirm").display();
					} else xb = true;
				}
				if(endb.status == 2) {
					if(hb && wb && (mapSelected == MAP_CITY_ID ? (nb && xb) : 1)) {
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
		settextjustify(LEFT_TEXT, TOP_TEXT);
		importGameSettings();
		if(mapSelected < 9) {
			switch(mapSelected) {
				case MAP_RANDOM_ID: createRandomMap(height, width); break;
				case MAP_STANDARD_ID: createStandardMap(height, width); break;
				case MAP_SP_MAZE_ID: createMazeMap(height, width, 0, 0); break;
				case MAP_MR_MAZE_ID: createMazeMap(height, width, 2, 0); break;
				case MAP_SP_SMAZE_ID: createMazeMap(height, width, 0, 1); break;
				case MAP_MR_SMAZE_ID: createMazeMap(height, width, 2, 1); break;
				case MAP_CITY_ID: createFullCityMap(height, width, armymin, armymax, plCnt); break;
				case MAP_PLAIN_ID: createFullPlainMap(height, width, plCnt); break;
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
		rectBUTTON plCntBox[20]; /* 2~16 */
		rectBUTTON checkBox[20]; /* 2~16 */
		rectBUTTON checkOA;
		rectBUTTON gameBox;
		plCnt = LGset::defaultPlayerNum;
		stDel = LGset::defaultSpeed;
		settextjustify(CENTER_TEXT, CENTER_TEXT);
		setfont(50 * windowData.zoomY, 0, LGset::mainFontName.c_str());
		xyprintf(250 * windowData.zoomX, 350 * windowData.zoomY,
		         "Choose Player Count");
		setlinewidth(1);
		for(int i = 2; i <= 16; ++i) {
			int rowNum = (i - 2) / 4;
			int colNum = (i - 2) % 4;
			rectangle(55 * windowData.zoomX + 100 * windowData.zoomX * colNum,
			          400 * windowData.zoomY + 100 * windowData.zoomY * rowNum,
			          155 * windowData.zoomX + 100 * windowData.zoomX * colNum,
			          500 * windowData.zoomY + 100 * windowData.zoomY * rowNum);
			plCntBox[i]
			    .size(100 * windowData.zoomX - 1 * 2, 100 * windowData.zoomY - 1 * 2)
			    .move(55 * windowData.zoomX + 100 * windowData.zoomX * colNum + 1,
			          400 * windowData.zoomY + 100 * windowData.zoomY * rowNum + 1)
			    .textalign(CENTER_TEXT, CENTER_TEXT)
			    .fontname(LGset::mainFontName.c_str())
			    .fontsize(30 * windowData.zoomY, 0)
			    .bgcolor(bgColor)
			    .textcolor(WHITE)
			    .addtext(to_wstring(i));
			plCntBox[i].enableButtonShadow = false;
			plCntBox[i].display();
		}
		xyprintf(250 * windowData.zoomX, 850 * windowData.zoomY,
		         "Current Count: %d", plCnt);
		xyprintf(800 * windowData.zoomX, 350 * windowData.zoomY,
		         "Input Game Speed");
		xyprintf(800 * windowData.zoomX, 400 * windowData.zoomY,
		         "(integer between 1 and 10000)");
		speedBox.create();
		speedBox.move(575 * windowData.zoomX, 450 * windowData.zoomY);
		speedBox.size(300 * windowData.zoomX, 50 * windowData.zoomY);
		speedBox.setfont(50 * windowData.zoomY, 0, LGset::mainFontName.c_str());
		speedBox.setcolor(mainColor);
		speedBox.visible(true);
		speedSubmit
		    .size(150 * windowData.zoomX, 50 * windowData.zoomY)
		    .move(900 * windowData.zoomX, 450 * windowData.zoomY)
		    .fontname(LGset::mainFontName.c_str())
		    .fontsize(30 * windowData.zoomY, 0)
		    .bgcolor(WHITE)
		    .textcolor(mainColor)
		    .textalign(CENTER_TEXT, CENTER_TEXT)
		    .addtext(L"submit");
		speedSubmit.display();
		xyprintf(800 * windowData.zoomX, 550 * windowData.zoomY,
		         "Current Speed: %d", stDel);
		xyprintf(1350 * windowData.zoomX, 350 * windowData.zoomY,
		         "Choose Visible Players:");
		cheatCode = 0b00000000000000010;
		for(int i = 1; i <= 16; ++i) {
			int rowNum = (i - 1) / 4;
			int colNum = (i - 1) % 4;
			checkBox[i]
			    .addtext(playerInfo[i].name)
			    .textalign(CENTER_TEXT, CENTER_TEXT)
			    .fontname(LGset::mainFontName.c_str())
			    .fontsize(26 * windowData.zoomY, 0)
			    .size(100 * windowData.zoomX - 1 * 2, 100 * windowData.zoomY - 1 * 2)
			    .move(1150 * windowData.zoomX + 100 * windowData.zoomX * colNum + 1,
			          400 * windowData.zoomY + 100 * windowData.zoomY * rowNum + 1);
			checkBox[i].enableButtonShadow = false;
			checkBox[i].enableTextShadow = false;
		}
		checkOA
		    .size(400 * windowData.zoomX - 1 * 2, 100 * windowData.zoomY - 1 * 2)
		    .move(1150 * windowData.zoomX + 1, 800 * windowData.zoomY + 1)
		    .addtext(L"Crystal Clear")
		    .textalign(CENTER_TEXT, CENTER_TEXT)
		    .fontname(LGset::mainFontName.c_str())
		    .fontsize(35 * windowData.zoomY, 0);
		checkOA.enableButtonShadow = false;
		checkOA.enableTextShadow = false;
		checkOA.display();
		gameBox
		    .size(200 * windowData.zoomX, 100 * windowData.zoomY)
		    .move(1150 * windowData.zoomX, 100 * windowData.zoomY)
		    .addtext(L"START")
		    .textalign(CENTER_TEXT, CENTER_TEXT)
		    .fontname(LGset::mainFontName.c_str())
		    .fontsize(50 * windowData.zoomY, 0)
		    .bgcolor(WHITE)
		    .textcolor(mainColor);
		delay_ms(0);
		for(; is_run(); delay_fps(120)) {
			for(int i = 2; i <= 16; ++i) {
				plCntBox[i].detect().display();
				if(plCntBox[i].status == 2) plCnt = i;
			}
			plCnt = min(plCnt, mapInfo[mapSelected].generalcnt + mapInfo[mapSelected].plaincnt);
			setfillcolor(bgColor);
			bar(55 * windowData.zoomX, 801 * windowData.zoomY,
			    455 * windowData.zoomX, 900 * windowData.zoomY);
			xyprintf(250 * windowData.zoomX, 850 * windowData.zoomY,
			         "Current Count: %d", plCnt);
			speedSubmit.detect().display();
			if(speedSubmit.status == 2) {
				char s[105];
				speedBox.gettext(sizeof(s), s);
				int t = stDel;
				int f = sscanf(s, "%d", &stDel);
				if(f != 1) stDel = t;
				if(stDel <= 0 || stDel > 10000) stDel = t;
			}
			bar(600 * windowData.zoomX, 501 * windowData.zoomY,
			    1050 * windowData.zoomX, 600 * windowData.zoomY);
			xyprintf(800 * windowData.zoomX, 550 * windowData.zoomY,
			         "Current Speed: %d", stDel);
			setfillcolor(bgColor);
			bar(1150 * windowData.zoomX, 400 * windowData.zoomY,
			    1550 * windowData.zoomX, 800 * windowData.zoomY);
			xyprintf(1350 * windowData.zoomX, 350 * windowData.zoomY,
			         "Choose Visible Players:");
			for(int i = 1; i <= plCnt; ++i) {
				if(cheatCode >> i & 1) {
					checkBox[i]
					    .bgcolor(playerInfo[i].color)
					    .textcolor(WHITE);
					;
				} else {
					checkBox[i]
					    .bgcolor(bgColor)
					    .textcolor(playerInfo[i].color);
					;
				}
				int rowNum = (i - 1) / 4;
				int colNum = (i - 1) % 4;
				rectangle(1150 * windowData.zoomX + 100 * windowData.zoomX * colNum,
				          400 * windowData.zoomY + 100 * windowData.zoomY * rowNum,
				          1250 * windowData.zoomX + 100 * windowData.zoomX * colNum,
				          500 * windowData.zoomY + 100 * windowData.zoomY * rowNum);
				checkBox[i].detect().display();
				if(checkBox[i].status == 2) {
					cheatCode &= (((1 << plCnt) - 1) << 1);
					cheatCode ^= (1 << i);
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
			rectangle(1150 * windowData.zoomX, 800 * windowData.zoomY,
			          1550 * windowData.zoomX, 900 * windowData.zoomY);
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
		blockHeight = 22 * windowData.zoomY;
		blockWidth = 22 * windowData.zoomX;
		blockHeight = blockWidth = min(blockHeight, blockWidth);
		windowData.widthPerBlock = blockWidth;
		windowData.heightPerBlock = blockHeight;
		setbkmode(TRANSPARENT);
		pimg[1] = newimage();
		getimage(pimg[1], "PNG", "IMAGE_CITY");
		// imageOperation::zoomImage(pimg[1], windowData.heightPerBlock, windowData.widthPerBlock);
		pimg[2] = newimage();
		getimage(pimg[2], "PNG", "IMAGE_CROWN");
		// imageOperation::zoomImage(pimg[2], windowData.heightPerBlock, windowData.widthPerBlock);
		pimg[3] = newimage();
		getimage(pimg[3], "PNG", "IMAGE_MOUNTAIN");
		// imageOperation::zoomImage(pimg[3], windowData.heightPerBlock, windowData.widthPerBlock);
		pimg[4] = newimage();
		getimage(pimg[4], "PNG", "IMAGE_SWAMP");
		// imageOperation::zoomImage(pimg[4], windowData.heightPerBlock, windowData.widthPerBlock);
		pimg[5] = newimage();
		getimage(pimg[5], "PNG", "IMAGE_OBSTACLE");
		// imageOperation::zoomImage(pimg[5], windowData.heightPerBlock, windowData.widthPerBlock);
		pimg[6] = newimage();
		getimage(pimg[6], "PNG", "IMAGE_CURRENTON");
		// imageOperation::zoomImage(pimg[6], windowData.heightPerBlock, windowData.widthPerBlock);
		pimg[7] = newimage();
		getimage(pimg[7], "PNG", "IMAGE_ERASE");
		pimg[8] = newimage();
		getimage(pimg[8], "PNG", "IMAGE_LIGHT");
		pimg[9] = newimage();
		getimage(pimg[9], "PNG", "IMAGE_OPTIONS");
		pimg[10] = newimage();
		getimage(pimg[10], "PNG", "IMAGE_GITHUB");
		for(int i = 1; i <= 10; i++) ege_enable_aa(true, pimg[i]);
		ege_enable_aa(true);
		setbkcolor(0xff222222);
		setbkcolor_f(0xff222222);
		cleardevice();
	}

	void initPages() {
		itemS tmp;  // temp variable for items

		// settings page
		p_settings.size(zoomX(1600), zoomY(900)).move(zoomX(0), zoomY(0)).dSize(zoomX(1600), zoomY(900)).setBgColor(LGGraphics::bgColor);
		tmp.iType = ITEM_LINETEXT;
		tmp.info.lText = new lineTextS;
		tmp.locX = zoomX(100);
		tmp.locY = zoomY(20);
		tmp.downLoc();
		tmp.info.lText->push_back(singleTextS(WHITE, L"Settings", LGset::mainFontName, -zoomY(50), 0));
		p_settings.addItem(tmp);
		tmp.iType = ITEM_RECTCHKBOX_WITH_TEXT;
		tmp.info.cBText = new rCBOXtextS;
		tmp.locX = zoomX(100);
		tmp.locY = zoomY(120);
		tmp.downLoc();
		tmp.info.cBText->bgColor = LGGraphics::bgColor;
		tmp.info.cBText->boxText.push_back(singleTextS(WHITE, L"Enable god power (originated from v1.0.0 bug)", LGset::mainFontName, -zoomY(20), 0));
		tmp.info.cBText->checkBox.size(zoomX(20), zoomY(20)).frame(2).variable(&LGset::enableGodPower).bgcolor(bgColor).fillcolor(WHITE).framecolor(WHITE);
		tmp.info.cBText->blankWidth = zoomX(10);
		p_settings.addItem(tmp);
		tmp.iType = ITEM_RECTCHKBOX_WITH_TEXT;
		tmp.info.cBText = new rCBOXtextS;
		tmp.locX = zoomX(100);
		tmp.locY = zoomY(150);
		tmp.downLoc();
		tmp.info.cBText->bgColor = LGGraphics::bgColor;
		tmp.info.cBText->boxText.push_back(singleTextS(WHITE, L"Enable gong sound when starting game", LGset::mainFontName, -zoomY(20), 0));
		tmp.info.cBText->checkBox.size(zoomX(20), zoomY(20)).frame(2).variable(&LGset::enableGongSound).bgcolor(bgColor).fillcolor(WHITE).framecolor(WHITE);
		tmp.info.cBText->blankWidth = zoomX(10);
		p_settings.addItem(tmp);
		tmp.iType = ITEM_RECTCHKBOX_WITH_TEXT;
		tmp.info.cBText = new rCBOXtextS;
		tmp.locX = zoomX(100);
		tmp.locY = zoomY(180);
		tmp.downLoc();
		tmp.info.cBText->bgColor = LGGraphics::bgColor;
		tmp.info.cBText->boxText.push_back(singleTextS(WHITE, L"Enable analysis in game", LGset::mainFontName, -zoomY(20), 0));
		tmp.info.cBText->checkBox.size(zoomX(20), zoomY(20)).frame(2).variable(&LGset::enableAnalysisInGame).bgcolor(bgColor).fillcolor(WHITE).framecolor(WHITE);
		tmp.info.cBText->blankWidth = zoomX(10);
		p_settings.addItem(tmp);
		tmp.iType = ITEM_VARINTTEXT;
		tmp.info.vIText = new varIntTextS(&LGset::game::gameMode, WHITE, L"Game Mode: ", LGset::mainFontName, -zoomY(20), 0);
		tmp.locX = zoomX(100);
		tmp.locY = zoomY(210);
		tmp.downLoc();
		p_settings.addItem(tmp);
		tmp.iType = ITEM_CIRCBUTTON;
		tmp.info.cButton = new circBUTTON;
		tmp.locX = zoomX(260);
		tmp.locY = zoomY(225);  // centre
		tmp.downLoc();
		tmp.info.cButton->radius(zoomY(15));
		tmp.info.cButton->addtext(L"0").bgcolor(LGGraphics::mainColor).textcolor(WHITE).fontsize(zoomY(20), 0).textalign(CENTER_TEXT, CENTER_TEXT).fontname(LGset::mainFontName);
		tmp.info.cButton->enableShadow = true;
		tmp.info.cButton->enableTextShadow = false;
		tmp.info.cButton->enableButtonShadow = false;
		tmp.info.cButton->event([&]() -> void { LGset::game::gameMode = 0; });
		p_settings.addItem(tmp);
		tmp.info.cButton = new circBUTTON;
		tmp.locX = zoomX(300);
		tmp.locY = zoomY(225);  // centre
		tmp.downLoc();
		tmp.info.cButton->radius(zoomY(15));
		tmp.info.cButton->addtext(L"1").bgcolor(LGGraphics::mainColor).textcolor(WHITE).fontsize(zoomY(20), 0).textalign(CENTER_TEXT, CENTER_TEXT).fontname(LGset::mainFontName);
		tmp.info.cButton->enableShadow = true;
		tmp.info.cButton->enableTextShadow = false;
		tmp.info.cButton->enableButtonShadow = false;
		tmp.info.cButton->event([&]() -> void { LGset::game::gameMode = 1; });
		p_settings.addItem(tmp);
		tmp.info.cButton = new circBUTTON;
		tmp.locX = zoomX(340);
		tmp.locY = zoomY(225);  // centre
		tmp.downLoc();
		tmp.info.cButton->radius(zoomY(15));
		tmp.info.cButton->addtext(L"2").bgcolor(LGGraphics::mainColor).textcolor(WHITE).fontsize(zoomY(20), 0).textalign(CENTER_TEXT, CENTER_TEXT).fontname(LGset::mainFontName);
		tmp.info.cButton->enableShadow = true;
		tmp.info.cButton->enableTextShadow = false;
		tmp.info.cButton->enableButtonShadow = false;
		tmp.info.cButton->event([&]() -> void { LGset::game::gameMode = 2; });
		p_settings.addItem(tmp);
		tmp.iType = ITEM_LINETEXT;
		tmp.info.lText = new lineTextS;
		tmp.locX = zoomX(800);
		tmp.locY = zoomY(110);
		tmp.downLoc();
		tmp.info.lText->push_back(singleTextS(WHITE, L"Modifiers", LGset::mainFontName, -zoomY(35), 0));
		p_settings.addItem(tmp);
		tmp.iType = ITEM_RECTCHKBOX_WITH_TEXT;
		tmp.info.cBText = new rCBOXtextS;
		tmp.locX = zoomX(800);
		tmp.locY = zoomY(170);
		tmp.downLoc();
		tmp.info.cBText->bgColor = LGGraphics::bgColor;
		tmp.info.cBText->boxText.push_back(singleTextS(WHITE, L"Leapfrog", LGset::mainFontName, -zoomY(20), 0));
		tmp.info.cBText->checkBox.size(zoomX(20), zoomY(20)).frame(2).variable(&LGset::modifier::Leapfrog).bgcolor(bgColor).fillcolor(WHITE).framecolor(WHITE);
		tmp.info.cBText->blankWidth = zoomX(10);
		p_settings.addItem(tmp);
		tmp.iType = ITEM_RECTCHKBOX_WITH_TEXT;
		tmp.info.cBText = new rCBOXtextS;
		tmp.locX = zoomX(800);
		tmp.locY = zoomY(200);
		tmp.downLoc();
		tmp.info.cBText->bgColor = LGGraphics::bgColor;
		tmp.info.cBText->boxText.push_back(singleTextS(WHITE, L"City-State (unavailable)", LGset::mainFontName, -zoomY(20), 0));
		tmp.info.cBText->checkBox.size(zoomX(20), zoomY(20)).frame(2).variable(&LGset::modifier::CityState).bgcolor(bgColor).fillcolor(WHITE).framecolor(WHITE);
		tmp.info.cBText->blankWidth = zoomX(10);
		p_settings.addItem(tmp);
		tmp.iType = ITEM_RECTCHKBOX_WITH_TEXT;
		tmp.info.cBText = new rCBOXtextS;
		tmp.locX = zoomX(800);
		tmp.locY = zoomY(230);
		tmp.downLoc();
		tmp.info.cBText->bgColor = LGGraphics::bgColor;
		tmp.info.cBText->boxText.push_back(singleTextS(WHITE, L"Misty Veil", LGset::mainFontName, -zoomY(20), 0));
		tmp.info.cBText->checkBox.size(zoomX(20), zoomY(20)).frame(2).variable(&LGset::modifier::MistyVeil).bgcolor(bgColor).fillcolor(WHITE).framecolor(WHITE);
		tmp.info.cBText->blankWidth = zoomX(10);
		p_settings.addItem(tmp);
		tmp.iType = ITEM_RECTCHKBOX_WITH_TEXT;
		tmp.info.cBText = new rCBOXtextS;
		tmp.locX = zoomX(800);
		tmp.locY = zoomY(260);
		tmp.downLoc();
		tmp.info.cBText->bgColor = LGGraphics::bgColor;
		tmp.info.cBText->boxText.push_back(singleTextS(WHITE, L"Silent War", LGset::mainFontName, -zoomY(20), 0));
		tmp.info.cBText->checkBox.size(zoomX(20), zoomY(20)).frame(2).variable(&LGset::modifier::SilentWar).bgcolor(bgColor).fillcolor(WHITE).framecolor(WHITE);
		tmp.info.cBText->blankWidth = zoomX(10);
		p_settings.addItem(tmp);
		tmp.iType = ITEM_RECTCHKBOX_WITH_TEXT;
		tmp.info.cBText = new rCBOXtextS;
		tmp.locX = zoomX(800);
		tmp.locY = zoomY(350);
		tmp.downLoc();
		tmp.info.cBText->bgColor = LGGraphics::bgColor;
		tmp.info.cBText->boxText.push_back(singleTextS(WHITE, L"Neutral Resist", LGset::mainFontName, -zoomY(20), 0));
		tmp.info.cBText->checkBox.size(zoomX(20), zoomY(20)).frame(2).variable(&LGset::modifier::NeutralResist).bgcolor(bgColor).fillcolor(WHITE).framecolor(WHITE);
		tmp.info.cBText->blankWidth = zoomX(10);
		p_settings.addItem(tmp);
	}
}  // namespace LGGraphics

inline int getHeightPerBlock() { return LGGraphics::windowData.heightPerBlock; }

inline int getWidthPerBlock() { return LGGraphics::windowData.widthPerBlock; }

#endif  // LGGRAPHICS_HPP_
