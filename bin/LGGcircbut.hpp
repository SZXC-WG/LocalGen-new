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
	PIMAGE buttonImage; // image info
	PIMAGE backgroundImage;
	int backgroundImageWidth, backgroundImageHeight;
	int buttonRadius; // radius
	color_t backgroundColor; // background color
	color_t textColor; // text color
	vector<wstring> text; // text
	string fontName; // font face name
	int fontHeight, fontWidth; // font height & width
	int frameWidth;
	bool enableAutoFrameColor; color_t frameColor;
	int walign, halign; // align method
	int locationX, locationY; // location on screen

  public:
	bool enableTextShadow; int textShadowWeight;
	bool enableShadow;
	bool enableButtonShadow;
	int status; // button status: free(0) / cursor-on(1) / clicked(2)
	std::function<void()> clickEvent; // event when clicked
	explicit circBUTTON() {
		buttonImage = newimage();
		backgroundImage = nullptr;
		buttonRadius = 1;
		walign = LEFT_TEXT, halign = TOP_TEXT;
		frameWidth = 1;
		status = 0;
		enableAutoFrameColor = true;
		enableTextShadow = true; textShadowWeight = 1;
		enableShadow = 1; enableButtonShadow = 1;
	}
	~circBUTTON() {
		delimage(buttonImage);
		delimage(backgroundImage);
	}
	circBUTTON(int _radius) {
		circBUTTON();
		buttonRadius = _radius;
	}
	circBUTTON(circBUTTON&& but) {
		delimage(buttonImage);
		buttonImage = but.buttonImage;
		buttonRadius = but.buttonRadius;
		backgroundColor = but.backgroundColor, textColor = but.textColor;
		text = but.text;
	}
	circBUTTON(const circBUTTON& but) {
		delimage(buttonImage);
		buttonImage = but.buttonImage;
		buttonRadius = but.buttonRadius;
		backgroundColor = but.backgroundColor, textColor = but.textColor;
		text = but.text;
	}
	inline circBUTTON& draw() {
		delimage(buttonImage);
		if(enableShadow && enableButtonShadow) buttonImage = newimage(buttonRadius * 2 + 3, buttonRadius * 2 + 3);
		else buttonImage = newimage(buttonRadius * 2, buttonRadius * 2);
		ege_enable_aa(buttonImage);
		setbkcolor(LGGraphics::bgColor, buttonImage);
		setbkcolor_f(LGGraphics::bgColor, buttonImage);
		setbkmode(TRANSPARENT, buttonImage);
		if((status == 1 || status == 2) && enableShadow && enableButtonShadow) {
			setfillcolor(LGGraphics::mainColor, buttonImage);
			fillellipse(buttonRadius + 3, buttonRadius + 3, buttonRadius, buttonRadius, buttonImage);
		}
		setfillcolor(backgroundColor, buttonImage);
		fillellipse(buttonRadius, buttonRadius, buttonRadius, buttonRadius, buttonImage);
		if(backgroundImage != nullptr) {
			if(getwidth(backgroundImage)!=backgroundImageWidth||getheight(backgroundImage)!=backgroundImageHeight) imageOperation::zoomImage(backgroundImage,backgroundImageWidth,backgroundImageHeight);
			putimage_withalpha(buttonImage,backgroundImage,buttonRadius-backgroundImageWidth/2,buttonRadius-backgroundImageHeight/2);
		}
		setfont(fontHeight, fontWidth, fontName.c_str(), buttonImage);
		settextjustify(walign, halign, buttonImage);
		register int ox, oy;
		if(walign == LEFT_TEXT) ox = 0;
		else if(walign == CENTER_TEXT) ox = buttonRadius;
		else ox = buttonRadius * 2 - 1;
		if(halign == TOP_TEXT) oy = 0;
		else if(halign == CENTER_TEXT) oy = (buttonRadius * 2 - fontHeight * (text.size() - 1)) / 2;
		else oy = buttonRadius * 2 - fontHeight * (text.size() - 1) - 1;
		for(auto s:text) {
			if(enableShadow && enableTextShadow) {
				setcolor(0xff008080, buttonImage);
				outtextxy(ox+textShadowWeight, oy+textShadowWeight, s.c_str(), buttonImage);
			}
			setcolor(textColor, buttonImage);
			outtextxy(ox, oy, s.c_str(), buttonImage);
			oy += fontHeight;
		}
		if(!enableShadow) {
			if(enableAutoFrameColor) setcolor(0xff000000 | ~backgroundColor, buttonImage);
			else setcolor(frameColor, buttonImage);
			setlinewidth(frameWidth, buttonImage);
			if(status == 1 || status == 2)
				ellipse(buttonRadius, buttonRadius, 0, 360, buttonRadius, buttonRadius, buttonImage);
		} else {
			if(status == 1 || status == 2) {
				setfillcolor(0x80808080, buttonImage);
				ege_fillellipse(0, 0, buttonRadius * 2, buttonRadius * 2, buttonImage);
			}
		}
		return *this;
	}
	inline circBUTTON& display() {
		draw();
		putimage(locationX - buttonRadius, locationY - buttonRadius, buttonImage);
		return *this;
	}
	inline circBUTTON& radius(int _radius) { buttonRadius = _radius; return *this; }
	inline circBUTTON& bgcolor(color_t _color) { backgroundColor = _color; return *this; }
	inline circBUTTON& textcolor(color_t _color) { textColor = _color; return *this; }
	inline circBUTTON& addtext(wstring _text) { text.push_back(_text); return *this; }
	inline circBUTTON& poptext() { if(!text.empty()) text.pop_back(); return *this; }
	inline circBUTTON& cleartext() { text.clear(); return *this; }
	inline circBUTTON& fontname(string _fontName) { fontName = _fontName; return *this; }
	inline circBUTTON& fontsize(int _fontHeight, int _fontWidth) { fontHeight = _fontHeight; fontWidth = _fontWidth; return *this; }
	inline circBUTTON& move(int _X, int _Y) { locationY = _Y, locationX = _X; return *this; }
	inline circBUTTON& textalign(int _walign = -1, int _halign = -1) {
		if(~_walign) walign = _walign;
		if(~_halign) halign = _halign;
		return *this;
	}
	inline circBUTTON& event(std::function<void()> event) { clickEvent = event; return *this; }
	inline circBUTTON& frame(int _width) { frameWidth = _width; return *this; }
	inline circBUTTON& framecolor(bool _enableAuto = 1, color_t _color = 0xffffffff) { enableAutoFrameColor = _enableAuto, frameColor = _color; return *this; }
	inline circBUTTON& bgimage(PIMAGE _img) {
		if(backgroundImage == nullptr) backgroundImage = newimage();
		imageOperation::copyImage(backgroundImage,_img);
		backgroundImageWidth = getwidth(backgroundImage);
		backgroundImageHeight = getheight(backgroundImage);
		return *this;
	}
	inline circBUTTON& bgsize(int _width,int _height) { backgroundImageWidth = _width,backgroundImageHeight = _height; return *this; }
	inline circBUTTON& delbgimage() { backgroundImage = nullptr; return *this; }
	inline circBUTTON& detect() {
		POINT mousePos;
		GetCursorPos(&mousePos);
		ScreenToClient(getHWnd(), &mousePos);
		double dist = hypot(mousePos.x - locationX, mousePos.y - locationY);
		if(dist > buttonRadius) return status = 0, * this;
		while(mousemsg()) {
			mouse_msg msg = getmouse();
			if(hypot(msg.x - locationX, msg.y - locationY) <= buttonRadius
			   && msg.is_left() && msg.is_down()) return status = 2, * this;
		}
		return status = 1, * this;
	}
};

#endif // __LGGCIRCBUT_HPP__
