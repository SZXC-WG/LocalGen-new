/* This is GLIB_circbut.hpp file of SZXC-WG graphics lib.                */
/* Thanks to EGE: http://xege.org/                                       */
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

#ifndef LG_GLIB_CIRCBUT_HPP_
#define LG_GLIB_CIRCBUT_HPP_

#include "GLIB_HEAD.hpp"

_GLIB_NAMESPACE_HEAD

inline namespace button {

	circBUTTON::circBUTTON() {
		buttonImage = newimage();
		backgroundImage = nullptr;
		buttonRadius = 1;
		walign = LEFT_TEXT, halign = TOP_TEXT;
		frameWidth = 1;
		status = 0;
		enableAutoFrameColor = true;
		enableTextShadow = true;
		textShadowWeight = 1;
		enableShadow = true;
		enableButtonShadow = true;
	}
	circBUTTON::~circBUTTON() {
		delimage(buttonImage);
		delimage(backgroundImage);
	}
	circBUTTON::circBUTTON(int _radius) {
		circBUTTON();
		buttonRadius = _radius;
	}
	circBUTTON::circBUTTON(circBUTTON&& but) {
		delimage(buttonImage);
		buttonImage = but.buttonImage;
		buttonRadius = but.buttonRadius;
		backgroundColor = but.backgroundColor, textColor = but.textColor;
		text = but.text;
	}
	circBUTTON::circBUTTON(const circBUTTON& but) {
		delimage(buttonImage);
		buttonImage = but.buttonImage;
		buttonRadius = but.buttonRadius;
		backgroundColor = but.backgroundColor, textColor = but.textColor;
		text = but.text;
	}
	inline circBUTTON& circBUTTON::draw() {
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
			if(getwidth(backgroundImage) != backgroundImageWidth || getheight(backgroundImage) != backgroundImageHeight) images::zoomImage(backgroundImage, backgroundImageWidth, backgroundImageHeight);
			putimage_withalpha(buttonImage, backgroundImage, buttonRadius - backgroundImageWidth / 2, buttonRadius - backgroundImageHeight / 2);
		}
		setfont(-fontHeight, fontWidth, fontName.c_str(), buttonImage);
		settextjustify(walign, halign, buttonImage);
		int ox, oy;
		if(walign == LEFT_TEXT) ox = 0;
		else if(walign == CENTER_TEXT) ox = buttonRadius;
		else ox = buttonRadius * 2 - 1;
		if(halign == TOP_TEXT) oy = 0;
		else if(halign == CENTER_TEXT) oy = (buttonRadius * 2 - fontHeight * (text.size() - 1)) / 2;
		else oy = buttonRadius * 2 - fontHeight * (text.size() - 1) - 1;
		for(auto s: text) {
			if(enableShadow && enableTextShadow) {
				setcolor(0xff008080, buttonImage);
				outtextxy(ox + textShadowWeight, oy + textShadowWeight, s.c_str(), buttonImage);
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
	inline circBUTTON& circBUTTON::display(PIMAGE pimg) {
		draw();
		putimage(pimg, locationX - buttonRadius, locationY - buttonRadius, buttonImage);
		return *this;
	}
	inline circBUTTON& circBUTTON::radius(int _radius) {
		buttonRadius = _radius;
		return *this;
	}
	inline circBUTTON& circBUTTON::bgcolor(color_t _color) {
		backgroundColor = _color;
		return *this;
	}
	inline circBUTTON& circBUTTON::textcolor(color_t _color) {
		textColor = _color;
		return *this;
	}
	inline circBUTTON& circBUTTON::addtext(wstring _text) {
		text.push_back(_text);
		return *this;
	}
	inline circBUTTON& circBUTTON::poptext() {
		if(!text.empty()) text.pop_back();
		return *this;
	}
	inline circBUTTON& circBUTTON::cleartext() {
		text.clear();
		return *this;
	}
	inline circBUTTON& circBUTTON::fontname(wstring _fontName) {
		fontName = _fontName;
		return *this;
	}
	inline circBUTTON& circBUTTON::fontsize(int _fontHeight, int _fontWidth) {
		fontHeight = _fontHeight;
		fontWidth = _fontWidth;
		return *this;
	}
	inline circBUTTON& circBUTTON::move(int _X, int _Y) {
		locationY = _Y, locationX = _X;
		return *this;
	}
	inline circBUTTON& circBUTTON::textalign(int _walign, int _halign) {
		if(~_walign) walign = _walign;
		if(~_halign) halign = _halign;
		return *this;
	}
	inline circBUTTON& circBUTTON::event(std::function<void()> event) {
		clickEvent = event;
		return *this;
	}
	inline circBUTTON& circBUTTON::frame(int _width) {
		frameWidth = _width;
		return *this;
	}
	inline circBUTTON& circBUTTON::framecolor(bool _enableAuto, color_t _color) {
		enableAutoFrameColor = _enableAuto, frameColor = _color;
		return *this;
	}
	inline circBUTTON& circBUTTON::bgimage(PIMAGE _img) {
		if(backgroundImage == nullptr) backgroundImage = newimage();
		images::copyImage(backgroundImage, _img);
		backgroundImageWidth = getwidth(backgroundImage);
		backgroundImageHeight = getheight(backgroundImage);
		return *this;
	}
	inline circBUTTON& circBUTTON::bgsize(int _width, int _height) {
		backgroundImageWidth = _width, backgroundImageHeight = _height;
		return *this;
	}
	inline circBUTTON& circBUTTON::delbgimage() {
		backgroundImage = nullptr;
		return *this;
	}
	inline circBUTTON& circBUTTON::detect() {
		POINT mousePos;
		GetCursorPos(&mousePos);
		ScreenToClient(getHWnd(), &mousePos);
		double dist = hypot(mousePos.x - locationX, mousePos.y - locationY);
		if(dist > buttonRadius) return status = 0, *this;
		while(mousemsg()) {
			mouse_msg msg = getmouse();
			if(hypot(msg.x - locationX, msg.y - locationY) <= buttonRadius && msg.is_left() && msg.is_down()) return status = 2, *this;
		}
		return status = 1, *this;
	}
	inline bool circBUTTON::detect(mouse_msg _mouse) {
		_mouse.x -= locationX;
		_mouse.y -= locationY;
		if(hypot(_mouse.x, _mouse.y) > buttonRadius) return status = 0, false;
		if(_mouse.is_left() && _mouse.is_down()) status = 2;
		else status = 1;
		return true;
	}

}  // namespace button

_GLIB_NAMESPACE_TAIL

#endif  // LG_GLIB_CIRCBUT_HPP_
