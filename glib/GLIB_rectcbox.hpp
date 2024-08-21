/* This is GLIB_rectcbox.hpp file of SZXC-WG EGE graphics lib.           */
/* Thanks to EGE: http://xege.org/                                       */
/* Copyright (c) 2023 SZXC Work Group; All rights reserved.              */
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

#ifndef __LG_GLIB_RECTCBOX_HPP__
#define __LG_GLIB_RECTCBOX_HPP__

#include "GLIB_HEAD.hpp"

_GLIB_NAMESPACE_HEAD

inline namespace checkbox {

	rectCBOX::rectCBOX() {
		boxImage = newimage();
		boxWidth = boxHeight = 1;
		frameColor = fillColor = 0xffffffff;
		status = 0;
		pressed = 0;
		varPtr = nullptr;
	};
	rectCBOX::~rectCBOX() { delimage(boxImage); }
	inline rectCBOX& rectCBOX::draw() {
		delimage(boxImage);
		boxImage = newimage(boxWidth, boxHeight);
		setfillcolor(backgroundColor, boxImage);
		bar(0, 0, boxWidth, boxHeight, boxImage);
		setcolor(frameColor, boxImage);
		setlinewidth(frameWidth, boxImage);
		rectangle(1, 1, boxWidth, boxHeight, boxImage);
		if(pressed) {
			setfillcolor(fillColor, boxImage);
			bar(frameWidth - 1, frameWidth - 1, boxWidth - frameWidth + 1, boxHeight - frameWidth + 1, boxImage);
		}
		if(status == 1 || status == 2) {
			setfillcolor(0x80808080, boxImage);
			// ege_fillrect(frameWidth - 1, frameWidth - 1,
			//              boxWidth - 2 * frameWidth + 1, boxHeight - 2 * frameWidth + 1,
			//              boxImage);
			ege_fillrect(0, 0, boxWidth, boxHeight, boxImage);
		}
		return *this;
	}
	inline rectCBOX& rectCBOX::display(PIMAGE pimg) {
		draw();
		putimage(pimg, locationX, locationY, boxImage);
		return *this;
	}
	inline int rectCBOX::gwidth() { return boxWidth; }
	inline int rectCBOX::gheight() { return boxHeight; }
	inline std::pair<int, int> rectCBOX::gsize() { return std::make_pair(boxWidth, boxHeight); }
	inline rectCBOX& rectCBOX::size(int _width, int _height) {
		boxWidth = _width, boxHeight = _height;
		return *this;
	}
	inline rectCBOX& rectCBOX::bgcolor(color_t _color) {
		backgroundColor = _color;
		return *this;
	}
	inline rectCBOX& rectCBOX::move(int _X, int _Y) {
		locationX = _X, locationY = _Y;
		return *this;
	}
	inline rectCBOX& rectCBOX::event(decltype(clickEvent) _event) {
		clickEvent = _event;
		return *this;
	}
	inline rectCBOX& rectCBOX::frame(int _width) {
		frameWidth = _width;
		return *this;
	}
	inline rectCBOX& rectCBOX::framecolor(color_t _color) {
		frameColor = _color;
		return *this;
	}
	inline rectCBOX& rectCBOX::fillcolor(color_t _color) {
		fillColor = _color;
		return *this;
	}
	inline rectCBOX& rectCBOX::detect() {
		POINT mousePos;
		GetCursorPos(&mousePos);
		ScreenToClient(getHWnd(), &mousePos);
		if(mousePos.x < locationX || mousePos.x > min(locationX + boxWidth - 1, getwidth()) || mousePos.y < locationY || mousePos.y > min(locationY + boxHeight - 1, getheight()))
			return status = 0, *this;
		while(mousemsg()) {
			mouse_msg msg = getmouse();
			if(!(msg.x < locationX || msg.x > min(locationX + boxWidth - 1, getwidth()) || msg.y < locationY || msg.y > min(locationY + boxHeight - 1, getheight())) && msg.is_left() && msg.is_down()) return status = 2, *this;
		}
		return status = 1, *this;
	}
	inline bool rectCBOX::detect(mouse_msg _mouse) {
		_mouse.x -= locationX;
		_mouse.y -= locationY;
		if(_mouse.x < 0 || _mouse.x > boxWidth - 1 || _mouse.y < 0 || _mouse.y > boxHeight - 1) return status = 0, false;
		if(_mouse.is_left() && _mouse.is_down()) status = 2;
		else status = 1;
		return true;
	}
	inline rectCBOX& rectCBOX::variable(bool* _varPtr) {
		varPtr = _varPtr;
		pressed = *varPtr;
		return *this;
	}
	inline rectCBOX& rectCBOX::changeState() {
		pressed = !pressed;
		if(varPtr != nullptr) *varPtr = pressed;
		return *this;
	}

	inline rCBOXtextS::rCBOXtextS() {
		textImage = newimage();
		blankWidth = 0;
	}
	inline rCBOXtextS::~rCBOXtextS() { delimage(textImage); }
	inline rCBOXtextS& rCBOXtextS::move(int _X, int _Y) {
		locX = _X;
		locY = _Y;
		return *this;
	}
	inline rCBOXtextS& rCBOXtextS::detect() {
		checkBox.detect();
		return *this;
	}
	inline bool rCBOXtextS::detect(mouse_msg _mouse) {
		_mouse.x -= locX;
		_mouse.y -= locY;
		return checkBox.detect(_mouse);
	}
	inline rCBOXtextS& rCBOXtextS::draw() {
		int totW = boxText.width(textImage) + blankWidth + checkBox.gwidth();
		int totH = max(boxText.height(textImage), checkBox.gheight());
		delimage(textImage);
		textImage = newimage(totW, totH);
		setbkcolor(bgColor, textImage);
		setbkcolor_f(bgColor, textImage);
		settextjustify(LEFT_TEXT, TOP_TEXT);
		checkBox.move(0, totH / 2 - checkBox.gheight() / 2);
		checkBox.display(textImage);
		boxText.print(checkBox.gwidth() + blankWidth, totH / 2 - boxText.height(textImage) / 2, textImage);
		return *this;
	}
	inline rCBOXtextS& rCBOXtextS::display(int _X, int _Y, PIMAGE pimg) {
		draw();
		putimage(pimg, _X, _Y, textImage);
		return *this;
	}

}  // namespace checkbox

_GLIB_NAMESPACE_TAIL

#endif  // __LG_GLIB_RECTCBOX_HPP__
