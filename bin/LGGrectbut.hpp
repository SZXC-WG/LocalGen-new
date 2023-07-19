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

// class for rect buttons
class rectBUTTON {
  private:
	PIMAGE button; // image info
	PIMAGE bgImage; // background image
	int bgimgwid, bgimghei;
	int wid, hei; // width & height
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
	explicit rectBUTTON() {
		button = newimage();
		bgImage = nullptr;
		wid = hei = 1;
		walign = LEFT_TEXT, halign = TOP_TEXT;
		lnwid = 1;
		status = 0;
		autortcol = true;
		txtshadow = true; txtshadowwei = 1;
		floating = 1; floatshadow = 1;
	}
	~rectBUTTON() {
		delimage(button);
		delimage(bgImage);
	}
	rectBUTTON(int w, int h) {
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
		if(floating && floatshadow) button = newimage(wid+3, hei+3);
		else button = newimage(wid, hei);
		ege_enable_aa(button);
		setbkcolor(0xff222222, button);
		setbkcolor_f(0xff222222, button);
		setbkmode(TRANSPARENT, button);
		if((status == 1 || status == 2) && floating && floatshadow) {
			setfillcolor(0xff008080, button);
			bar(3, 3, wid+3, hei+3, button);
		}
		setfillcolor(bgcol, button);
		bar(0, 0, wid, hei, button);
		if(bgImage != nullptr) {
			if(getwidth(bgImage)!=bgimgwid||getheight(bgImage)!=bgimghei) imageOperation::zoomImage(bgImage,bgimgwid,bgimghei);
			putimage_withalpha(button,bgImage,0,0);
		}
		setfont(fonthei, fontwid, font.c_str(), button);
		settextjustify(walign, halign, button);
		register int ox, oy;
		if(walign == LEFT_TEXT) ox = 0;
		else if(walign == CENTER_TEXT) ox = wid / 2;
		else ox = wid - 1;
		if(halign == TOP_TEXT) oy = 0;
		else if(halign == CENTER_TEXT) oy = (hei - fonthei * (text.size() - 1)) / 2;
		else oy = hei - fonthei * (text.size() - 1) - 1;
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
				rectangle(1, 1, wid, hei, button);
		} else {
			if(status == 1 || status == 2) {
				setfillcolor(0x80808080, button);
				ege_fillrect(0, 0, wid, hei, button);
			}
		}
		return *this;
	}
	inline rectBUTTON& display() {
		draw();
		putimage(wloc, hloc, button);
		return *this;
	}
	inline rectBUTTON& setsize(int w, int h) { hei = h; wid = w; return *this; }
	inline rectBUTTON& setbgcol(color_t col) { bgcol = col; return *this; }
	inline rectBUTTON& settxtcol(color_t col) { txtcol = col; return *this; }
	inline rectBUTTON& addtext(string txt) { text.push_back(txt); return *this; }
	inline rectBUTTON& poptext() { if(!text.empty()) text.pop_back(); return *this; }
	inline rectBUTTON& cleartext() { text.clear(); return *this; }
	inline rectBUTTON& setfontname(string ft) { font = ft; return *this; }
	inline rectBUTTON& setfontsz(int fh, int fw) { fonthei = fh; fontwid = fw; return *this; }
	inline rectBUTTON& setlocation(int w, int h) { hloc = h, wloc = w; return *this; }
	inline rectBUTTON& setalign(int wa = -1, int ha = -1) {
		if(~wa) walign = wa;
		if(~ha) halign = ha;
		return *this;
	}
	inline rectBUTTON& setevent(std::function<void()> event) { clickEvent = event; return *this; }
	inline rectBUTTON& setlnwid(int w) { lnwid = w; return *this; }
	inline rectBUTTON& setrtcol(bool automated = 1, color_t col = 0xffffffff) { autortcol = automated, rtcol = col; return *this; }
	inline rectBUTTON& setbgimg(PIMAGE img) {
		if(bgImage == nullptr) bgImage = newimage();
		imageOperation::copyImage(bgImage,img);
		bgimgwid = getwidth(bgImage);
		bgimghei = getheight(bgImage);
		return *this;
	}
	inline rectBUTTON& delbgimg() { bgImage = nullptr; return *this; }
	inline rectBUTTON& detect() {
		POINT mousePos;
		GetCursorPos(&mousePos);
		ScreenToClient(getHWnd(), &mousePos);
		if(mousePos.x < wloc || mousePos.x > min(wloc + wid - 1, getwidth()) || mousePos.y < hloc || mousePos.y > min(hloc + hei - 1, getheight()))
			return status = 0, * this;
		while(mousemsg()) {
			mouse_msg msg = getmouse();
			if(!(msg.x < wloc || msg.x > min(wloc + wid - 1, getwidth()) || msg.y < hloc || msg.y > min(hloc + hei - 1, getheight()))
			   && msg.is_left() && msg.is_down()) return status = 2, * this;
		}
		return status = 1, * this;
	}
};

#endif // __LGGRECTBUT_HPP__
