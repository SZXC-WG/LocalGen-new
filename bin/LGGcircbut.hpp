/* This is LGGcircbut.hpp file of LocalGen graphics lib. (based on EGE)  */
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

#ifndef __LGGCIRCBUT_HPP__
#define __LGGCIRCBUT_HPP__

#include <graphics.h> // EGE
#include <cmath>
#include <algorithm>
#include <vector>
#include <string>
#include <functional>
using std::hypot;
using std::string;
using std::to_string;
using std::vector;
using std::min; using std::max;

// class for circ buttons
class circBUTTON {
  private:
	PIMAGE button; // image info
	PIMAGE bgImage;
	int bgimgwid, bgimghei;
	int radius; // radius
	color_t bgcol; // background color
	color_t txtcol; // text color
	vector<string> text; // text
	string font; // font face name
	int fonthei, fontwid; // font height & width
	int lnwid;
	bool autortcol; color_t rtcol;
	int walign, halign; // align method
	int wloc, hloc; // location on screen

  public:
	bool txtshadow; int txtshadowwei;
	bool floating;
	bool floatshadow;
	int status; // button status: free(0) / cursor-on(1) / clicked(2)
	std::function<void()> clickEvent; // event when clicked
	explicit circBUTTON() {
		button = newimage();
		bgImage = nullptr;
		radius = 1;
		walign = LEFT_TEXT, halign = TOP_TEXT;
		lnwid = 1;
		status = 0;
		autortcol = true;
		txtshadow = true; txtshadowwei = 1;
		floating = 1; floatshadow = 1;
	}
	~circBUTTON() {
		delimage(button);
		delimage(bgImage);
	}
	circBUTTON(int rad) {
		circBUTTON();
		radius = rad;
	}
	circBUTTON(circBUTTON&& but) {
		delimage(button);
		button = but.button;
		radius = but.radius;
		bgcol = but.bgcol, txtcol = but.txtcol;
		text = but.text;
	}
	circBUTTON(const circBUTTON& but) {
		delimage(button);
		button = but.button;
		radius = but.radius;
		bgcol = but.bgcol, txtcol = but.txtcol;
		text = but.text;
	}
	inline circBUTTON& draw() {
		delimage(button);
		if(floating && floatshadow) button = newimage(radius * 2 + 3, radius * 2 + 3);
		else button = newimage(radius * 2, radius * 2);
		ege_enable_aa(button);
		setbkcolor(LGGraphics::bgColor, button);
		setbkcolor_f(LGGraphics::bgColor, button);
		setbkmode(TRANSPARENT, button);
		if((status == 1 || status == 2) && floating && floatshadow) {
			setfillcolor(LGGraphics::mainColor, button);
			fillellipse(radius + 3, radius + 3, radius, radius, button);
		}
		setfillcolor(bgcol, button);
		fillellipse(radius, radius, radius, radius, button);
		if(bgImage != nullptr) {
			if(getwidth(bgImage)!=bgimgwid||getheight(bgImage)!=bgimghei) imageOperation::zoomImage(bgImage,bgimgwid,bgimghei);
			putimage_withalpha(button,bgImage,radius-bgimgwid/2,radius-bgimghei/2);
		}
		setfont(fonthei, fontwid, font.c_str(), button);
		settextjustify(walign, halign, button);
		register int ox, oy;
		if(walign == LEFT_TEXT) ox = 0;
		else if(walign == CENTER_TEXT) ox = radius;
		else ox = radius * 2 - 1;
		if(halign == TOP_TEXT) oy = 0;
		else if(halign == CENTER_TEXT) oy = (radius * 2 - fonthei * (text.size() - 1)) / 2;
		else oy = radius * 2 - fonthei * (text.size() - 1) - 1;
		for(auto s:text) {
			if(txtshadow) {
				setcolor(0xff008080, button);
				outtextxy(ox+txtshadowwei, oy+txtshadowwei, s.c_str(), button);
			}
			setcolor(txtcol, button);
			outtextxy(ox, oy, s.c_str(), button);
			oy += fonthei;
		}
		if(!floating) {
			if(autortcol) setcolor(0xff000000 | ~bgcol, button);
			else setcolor(rtcol, button);
			setlinewidth(lnwid, button);
			if(status == 1 || status == 2)
				ellipse(radius, radius, 0, 360, radius, radius, button);
		} else {
			if(status == 1 || status == 2) {
				setfillcolor(0x80808080, button);
				ege_fillellipse(0, 0, radius * 2, radius * 2, button);
			}
		}
		return *this;
	}
	inline circBUTTON& display() {
		draw();
		putimage(wloc - radius, hloc - radius, button);
		return *this;
	}
	inline circBUTTON& setrad(int rad) { radius = rad; return *this; }
	inline circBUTTON& setbgcol(color_t col) { bgcol = col; return *this; }
	inline circBUTTON& settxtcol(color_t col) { txtcol = col; return *this; }
	inline circBUTTON& addtext(string txt) { text.push_back(txt); return *this; }
	inline circBUTTON& poptext() { if(!text.empty()) text.pop_back(); return *this; }
	inline circBUTTON& cleartext() { text.clear(); return *this; }
	inline circBUTTON& setfontname(string ft) { font = ft; return *this; }
	inline circBUTTON& setfontsz(int fh, int fw) { fonthei = fh; fontwid = fw; return *this; }
	inline circBUTTON& setlocation(int w, int h) { hloc = h, wloc = w; return *this; }
	inline circBUTTON& setalign(int wa = -1, int ha = -1) {
		if(~wa) walign = wa;
		if(~ha) halign = ha;
		return *this;
	}
	inline circBUTTON& setevent(std::function<void()> event) { clickEvent = event; return *this; }
	inline circBUTTON& setlnwid(int w) { lnwid = w; return *this; }
	inline circBUTTON& setrtcol(bool automated = 1, color_t col = 0xffffffff) { autortcol = automated, rtcol = col; return *this; }
	inline circBUTTON& setbgimg(PIMAGE img) {
		if(bgImage == nullptr) bgImage = newimage();
		imageOperation::copyImage(bgImage,img);
		bgimgwid = getwidth(bgImage);
		bgimghei = getheight(bgImage);
		return *this;
	}
	inline circBUTTON& setbgsize(int w,int h) { bgimgwid = w,bgimghei = h; return *this; }
	inline circBUTTON& delbgimg() { bgImage = nullptr; return *this; }
	inline circBUTTON& detect() {
		POINT mousePos;
		GetCursorPos(&mousePos);
		ScreenToClient(getHWnd(), &mousePos);
		double dist = hypot(mousePos.x - wloc, mousePos.y - hloc);
		if(dist > radius) return status = 0, * this;
		while(mousemsg()) {
			mouse_msg msg = getmouse();
			if(hypot(msg.x - wloc, msg.y - hloc) <= radius
			   && msg.is_left() && msg.is_down()) return status = 2, *this;
		}
		return status = 1, *this;
	}
};

#endif // __LGGCIRCBUT_HPP__
