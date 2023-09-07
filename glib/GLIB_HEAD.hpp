/* This is GLIB_HEAD.hpp file of SZXC EGE graphics lib.                  */
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

#ifndef __LG_GLIB_HEAD_HPP__
#define __LG_GLIB_HEAD_HPP__

#define _GLIB_NAMESPACE_HEAD inline namespace _glib {
#define _GLIB_NAMESPACE_TAIL }

#include <functional>
#include <utility>
#include <string>
#include <algorithm>
#include <vector>
#include <deque>
using std::string;
using std::wstring;
using std::to_string;
using std::to_wstring;
using std::min; using std::max;
using std::vector; using std::deque;

#include <graphics.h>

#include "../LGdef.hpp"

_GLIB_NAMESPACE_HEAD

/**
 * @brief Namespace for operations on images.
 */
namespace images {

	/**
	 * @brief Copy one image to another.
	 *
	 * @param dstimg destination image
	 * @param srcimg source image
	 */
	void copyImage(PIMAGE& dstimg, PIMAGE& srcimg);
	void zoomImage(PIMAGE& pimg, int zoomWidth, int zoomHeight);
	void setWindowTransparent(bool enable, int alpha = 0xFF);

} // namespace images

/**
 * @brief Inline namespace for storing texts.
 */
inline namespace text {

	struct singleTextS {
		color_t color;
		wstring text;
		singleTextS() {};
		singleTextS(color_t _color, wstring _text) : color(_color), text(_text) {};

		int width(PIMAGE _pimg = NULL) { return textwidth(text.c_str(), _pimg); }
		int height(PIMAGE _pimg = NULL) { return textheight(text.c_str(), _pimg); }

		void print(int _X, int _Y, PIMAGE _pimg = NULL) {
			setcolor(color);
			outtextxy(_X, _Y, text.c_str(), _pimg);
		}
	};

	struct lineTextS {
		deque<singleTextS> text;
		lineTextS() {};
		lineTextS(deque<singleTextS> _text) : text(_text) {};

		void push_front(singleTextS _stext) { text.push_front(_stext); }
		void push_front(color_t _color, wstring _stext) { text.emplace_front(_color,_stext); }
		void push_back(singleTextS _stext) { text.push_back(_stext); }
		void push_back(color_t _color, wstring _stext) { text.emplace_back(_color,_stext); }
		void pop_front() { text.pop_front(); }
		void pop_back() { text.pop_back(); }

		int width(PIMAGE _pimg = NULL) {
			int ret=0;
			for(auto& t:text) ret += t.width(_pimg);
			return ret;
		}
		int height(PIMAGE _pimg = NULL) {
			int ret=0;
			for(auto& t:text) ret = max(ret, t.height(_pimg));
			return ret;
		}

		void print(int _X, int _Y, PIMAGE _pimg = NULL) {
			int x=_X, y=_Y;
			for(auto& t:text) {
				t.print(x, y, _pimg);
				x += t.width();
			}
		}
	};

} // inline namespace text

inline namespace button {

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
		explicit rectBUTTON();
		~rectBUTTON();
		rectBUTTON(int _width, int _height);
		rectBUTTON(rectBUTTON&& but);
		rectBUTTON(const rectBUTTON& but);
		inline rectBUTTON& draw();
		inline rectBUTTON& display();
		inline rectBUTTON& size(int _width, int _height);
		inline rectBUTTON& bgcolor(color_t _color);
		inline rectBUTTON& textcolor(color_t _color);
		inline rectBUTTON& addtext(wstring _text);
		inline rectBUTTON& poptext();
		inline rectBUTTON& cleartext();
		inline rectBUTTON& fontname(string _fontName);
		inline rectBUTTON& fontsize(int _fontHeight, int _fontWidth);
		inline rectBUTTON& move(int _X, int _Y);
		inline rectBUTTON& textalign(int _walign = -1, int _halign = -1);
		inline rectBUTTON& event(std::function<void()> event);
		inline rectBUTTON& frame(int _width);
		inline rectBUTTON& framecolor(bool _enableAuto = 1, color_t _color = 0xffffffff);
		inline rectBUTTON& bgimage(PIMAGE _img);
		inline rectBUTTON& bgsize(int _width,int _height);
		inline rectBUTTON& delbgimage();
		inline rectBUTTON& detect();
	};

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
		explicit circBUTTON();
		~circBUTTON();
		circBUTTON(int _radius);
		circBUTTON(circBUTTON&& but);
		circBUTTON(const circBUTTON& but);
		inline circBUTTON& draw();
		inline circBUTTON& display();
		inline circBUTTON& radius(int _radius);
		inline circBUTTON& bgcolor(color_t _color);
		inline circBUTTON& textcolor(color_t _color);
		inline circBUTTON& addtext(wstring _text);
		inline circBUTTON& poptext();
		inline circBUTTON& cleartext();
		inline circBUTTON& fontname(string _fontName);
		inline circBUTTON& fontsize(int _fontHeight, int _fontWidth);
		inline circBUTTON& move(int _X, int _Y);
		inline circBUTTON& textalign(int _walign = -1, int _halign = -1);
		inline circBUTTON& event(std::function<void()> event);
		inline circBUTTON& frame(int _width);
		inline circBUTTON& framecolor(bool _enableAuto = 1, color_t _color = 0xffffffff);
		inline circBUTTON& bgimage(PIMAGE _img);
		inline circBUTTON& bgsize(int _width,int _height);
		inline circBUTTON& delbgimage();
		inline circBUTTON& detect();
	};

} // inline namespace button

inline namespace checkbox {

	class rectCBOX {
	  private:
		PIMAGE boxImage;
		int boxWidth, boxHeight;
		int locationX, locationY;
		color_t backgroundColor;
		color_t frameColor;
		int frameWidth;
		color_t fillColor;
	  public:
		bool pressed;
		int status; /* button status: free(0) / cursor-on(1) / clicked(2) */
		bool* varPtr;
		std::function<void()> clickEvent;
		explicit rectCBOX();
		~rectCBOX();
		inline rectCBOX& draw();
		inline rectCBOX& display();
		inline rectCBOX& size(int _width, int _height);
		inline rectCBOX& bgcolor(color_t _color);
		inline rectCBOX& move(int _X, int _Y);
		inline rectCBOX& event(decltype(clickEvent) _event);
		inline rectCBOX& frame(int _width);
		inline rectCBOX& framecolor(color_t _color);
		inline rectCBOX& fillcolor(color_t _color);
		inline rectCBOX& detect();
		inline rectCBOX& variable(bool* _varPtr);
		inline rectCBOX& changeState();
	};

} // inline namespace checkbox

_GLIB_NAMESPACE_TAIL // inline namespace glib

#endif // __LG_GLIB_HEAD_HPP__
