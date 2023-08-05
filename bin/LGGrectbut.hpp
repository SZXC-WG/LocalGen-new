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
	PIMAGE buttonImage; // image info
	PIMAGE backgroundImage; // background image
	int backgroundImageWidth, backgroundImageHeight;
	int buttonWidth, buttonHeight; // width & height
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
	explicit rectBUTTON() {
		buttonImage = newimage();
		backgroundImage = nullptr;
		buttonWidth = buttonHeight = 1;
		walign = LEFT_TEXT, halign = TOP_TEXT;
		frameWidth = 1;
		status = 0;
		enableAutoFrameColor = true;
		enableTextShadow = true; textShadowWeight = 1;
		enableShadow = 1; enableButtonShadow = 1;
	}
	~rectBUTTON() {
		delimage(buttonImage);
		delimage(backgroundImage);
	}
	rectBUTTON(int _width, int _height) {
		rectBUTTON();
		buttonHeight = _height, buttonWidth = _width;
	}
	rectBUTTON(rectBUTTON&& but) {
		delimage(buttonImage);
		buttonImage = but.buttonImage;
		buttonHeight = but.buttonHeight, buttonWidth = but.buttonWidth;
		backgroundColor = but.backgroundColor, textColor = but.textColor;
		text = but.text;
	}
	rectBUTTON(const rectBUTTON& but) {
		delimage(buttonImage);
		buttonImage = but.buttonImage;
		buttonHeight = but.buttonHeight, buttonWidth = but.buttonWidth;
		backgroundColor = but.backgroundColor, textColor = but.textColor;
		text = but.text;
	}
	inline rectBUTTON& draw() {
		delimage(buttonImage);
		if(enableShadow && enableButtonShadow) buttonImage = newimage(buttonWidth+3, buttonHeight+3);
		else buttonImage = newimage(buttonWidth, buttonHeight);
		ege_enable_aa(buttonImage);
		setbkcolor(LGGraphics::bgColor, buttonImage);
		setbkcolor_f(LGGraphics::bgColor, buttonImage);
		setbkmode(TRANSPARENT, buttonImage);
		if((status == 1 || status == 2) && enableShadow && enableButtonShadow) {
			setfillcolor(LGGraphics::mainColor, buttonImage);
			bar(3, 3, buttonWidth+3, buttonHeight+3, buttonImage);
		}
		setfillcolor(backgroundColor, buttonImage);
		bar(0, 0, buttonWidth, buttonHeight, buttonImage);
		if(backgroundImage != nullptr) {
			if(getwidth(backgroundImage)!=backgroundImageWidth||getheight(backgroundImage)!=backgroundImageHeight) imageOperation::zoomImage(backgroundImage,backgroundImageWidth,backgroundImageHeight);
			putimage_withalpha(buttonImage,backgroundImage,0,0);
		}
		setfont(fontHeight, fontWidth, fontName.c_str(), buttonImage);
		settextjustify(walign, halign, buttonImage);
		register int ox, oy;
		if(walign == LEFT_TEXT) ox = 0;
		else if(walign == CENTER_TEXT) ox = buttonWidth / 2;
		else ox = buttonWidth - 1;
		if(halign == TOP_TEXT) oy = 0;
		else if(halign == CENTER_TEXT) oy = (buttonHeight - fontHeight * (text.size() - 1)) / 2;
		else oy = buttonHeight - fontHeight * (text.size() - 1) - 1;
		for(auto s:text) {
			if(enableShadow && enableTextShadow) {
				setcolor(LGGraphics::mainColor, buttonImage);
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
				rectangle(1, 1, buttonWidth, buttonHeight, buttonImage);
		} else {
			if(status == 1 || status == 2) {
				setfillcolor(0x80808080, buttonImage);
				ege_fillrect(0, 0, buttonWidth, buttonHeight, buttonImage);
			}
		}
		return *this;
	}
	inline rectBUTTON& display() {
		draw();
		putimage(locationX, locationY, buttonImage);
		return *this;
	}
	inline rectBUTTON& size(int _width, int _height) { buttonHeight = _height; buttonWidth = _width; return *this; }
	inline rectBUTTON& bgcolor(color_t _color) { backgroundColor = _color; return *this; }
	inline rectBUTTON& textcolor(color_t _color) { textColor = _color; return *this; }
	inline rectBUTTON& addtext(wstring _text) { text.push_back(_text); return *this; }
	inline rectBUTTON& poptext() { if(!text.empty()) text.pop_back(); return *this; }
	inline rectBUTTON& cleartext() { text.clear(); return *this; }
	inline rectBUTTON& fontname(string _fontName) { fontName = _fontName; return *this; }
	inline rectBUTTON& fontsize(int _fontHeight, int _fontWidth) { fontHeight = _fontHeight; fontWidth = _fontWidth; return *this; }
	inline rectBUTTON& move(int _X, int _Y) { locationY = _Y, locationX = _X; return *this; }
	inline rectBUTTON& textalign(int _walign = -1, int _halign = -1) {
		if(~_walign) walign = _walign;
		if(~_halign) halign = _halign;
		return *this;
	}
	inline rectBUTTON& event(std::function<void()> event) { clickEvent = event; return *this; }
	inline rectBUTTON& frame(int _width) { frameWidth = _width; return *this; }
	inline rectBUTTON& framecolor(bool _enableAuto = 1, color_t _color = 0xffffffff) { enableAutoFrameColor = _enableAuto, frameColor = _color; return *this; }
	inline rectBUTTON& bgimage(PIMAGE _img) {
		if(backgroundImage == nullptr) backgroundImage = newimage();
		imageOperation::copyImage(backgroundImage,_img);
		backgroundImageWidth = getwidth(backgroundImage);
		backgroundImageHeight = getheight(backgroundImage);
		return *this;
	}
	inline rectBUTTON& bgsize(int w,int h) { backgroundImageWidth = w,backgroundImageHeight = h; return *this; }
	inline rectBUTTON& delbgimage() { backgroundImage = nullptr; return *this; }
	inline rectBUTTON& detect() {
		POINT mousePos;
		GetCursorPos(&mousePos);
		ScreenToClient(getHWnd(), &mousePos);
		if(mousePos.x < locationX || mousePos.x > min(locationX + buttonWidth - 1, getwidth()) || mousePos.y < locationY || mousePos.y > min(locationY + buttonHeight - 1, getheight()))
			return status = 0, * this;
		while(mousemsg()) {
			mouse_msg msg = getmouse();
			if(!(msg.x < locationX || msg.x > min(locationX + buttonWidth - 1, getwidth()) || msg.y < locationY || msg.y > min(locationY + buttonHeight - 1, getheight()))
			   && msg.is_left() && msg.is_down()) return status = 2, * this;
		}
		return status = 1, * this;
	}
};

#endif // __LGGRECTBUT_HPP__
