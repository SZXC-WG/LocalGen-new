/* This is LGGrectbut.hpp file of LocalGen graphics lib. (based on EGE)  */
/* Thanks to EGE: http://xege.org/                                       */
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

#ifndef __LGGRECTBUT_HPP__
#define __LGGRECTBUT_HPP__

#include <graphics.h> // EGE
#include <algorithm>
#include <vector>
#include <string>
#include <functional>
using std::string;
using std::to_string;
using std::vector;
using std::min; using std::max;

// class for buttons
class rectBUTTON {
  private:
	PIMAGE button; // image info
	int hei, wid; // height & width
	color_t bgcol; // background color
	color_t txtcol; // text color
	vector<string> text; // text
	string font; // font face name
	int fonthei, fontwid; // font height & width
	int halign, walign; // align method
	int hloc, wloc; // location on screen
	int status; // button status: free(0) / cursor-on(1) / clicked(2)

  public:
	std::function<void()> clickEvent; // event when clicked
	explicit rectBUTTON() {
		button = newimage();
		hei = wid = 1;
		halign = TOP_TEXT, walign = LEFT_TEXT;
	}
	~rectBUTTON() {
		delimage(button);
	}
	rectBUTTON(int h, int w) {
		rectBUTTON();
		hei = h, wid = w;
	}
	rectBUTTON(rectBUTTON&& but) {
		delimage(button);
		button = but.button;
		hei = but.hei, wid = but.wid;
		bgcol = but.bgcol, txtcol = but.txtcol;
		text = but.text;
	}
	rectBUTTON(const rectBUTTON& but) {
		delimage(button);
		button = but.button;
		hei = but.hei, wid = but.wid;
		bgcol = but.bgcol, txtcol = but.txtcol;
		text = but.text;
	}
	inline rectBUTTON& draw() {
		delimage(button);
		button = newimage(wid, hei);
		setbkmode(TRANSPARENT, button);
		setfillcolor(bgcol, button);
		ege_fillrect(0, 0, wid, hei, button);
		setcolor(txtcol, button);
		setfont(fonthei, fontwid, font.c_str(), button);
		settextjustify(walign, halign, button);
		register int ox, oy;
		if(walign == LEFT_TEXT) ox = 0;
		else if(walign == CENTER_TEXT) ox = wid / 2;
		else ox = wid;
		if(halign == TOP_TEXT) oy = 0;
		else if(halign == CENTER_TEXT) oy = (hei - fonthei * (text.size() - 1)) / 2;
		else oy = hei - fonthei * (text.size() - 1);
		for(auto s:text) {
			outtextxy(ox, oy, s.c_str(), button);
			oy += fonthei;
		}
		setcolor(0xff000000 | ~bgcol, button);
		if(status == 1)
			ege_rectangle(1, 1, wid - 1, hei - 1, button);
		else if(status == 2)
			ege_rectangle(1, 1, wid - 1, hei - 1, button);
	}
	inline rectBUTTON& display() {
		draw();
		putimage(NULL, wloc, hloc, wid, hei, button, 0, 0);
	}
	inline rectBUTTON& seth(int h) { hei = h; }
	inline rectBUTTON& setw(int w) { wid = w; }
	inline rectBUTTON& sethw(int h, int w) { hei = h; wid = w; }
	inline rectBUTTON& setbgcol(color_t col) { bgcol = col; }
	inline rectBUTTON& settxtcol(color_t col) { txtcol = col; }
	inline rectBUTTON& addtext(string txt) { text.push_back(txt); }
	inline rectBUTTON& poptext() { if(!text.empty()) text.pop_back(); }
	inline rectBUTTON& cleartext() { text.clear(); }
	inline rectBUTTON& setfontname(string ft) { font = ft; }
	inline rectBUTTON& setfonth(int fh) { fonthei = fh; }
	inline rectBUTTON& setfontw(int fw) { fontwid = fw; }
	inline rectBUTTON& setfonthw(int fh, int fw) { fonthei = fh; fontwid = fw; }
	inline rectBUTTON& setlocation(int h, int w) { hloc = h, wloc = w; }
	inline rectBUTTON& setalign(int ha = -1, int wa = -1) {
		if(~ha) halign = ha;
		if(~wa) walign = wa;
	}
	inline rectBUTTON& setevent(std::function<void()> event) { clickEvent = event; }
	inline rectBUTTON& detect() {
		/* todo */
		POINT mousePos;
		GetCursorPos(&mousePos);
		ScreenToClient(getHWnd(), &mousePos);
		if(mousePos.x < wloc || mousePos.x > min(wloc + wid - 1, getwidth()) || mousePos.y < hloc || mousePos.y > min(hloc + hei - 1, getheight()))
			return status = 0, *this;
		while(mousemsg()) {
			mouse_msg msg = getmouse();
			if(!(msg.x < wloc || msg.x > min(wloc + wid - 1, getwidth()) || msg.y < hloc || msg.y > min(hloc + hei - 1, getheight()))
			   && msg.is_left() && msg.is_down()) return status = 2, *this;
		}
		return status = 1, *this;
	}
};

#endif // __LGGRECTBUT_HPP__
