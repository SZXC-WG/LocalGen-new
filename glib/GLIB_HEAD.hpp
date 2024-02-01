/* This is GLIB_HEAD.hpp file of SZXC EGE graphics lib.                  */
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
	/**
	 * @brief Zoom a image.
	 * 
	 * @param pimg target image
	 * @param zoomWidth target width
	 * @param zoomHeight target height
	 */
	void zoomImage(PIMAGE& pimg, int zoomWidth, int zoomHeight);
	/**
	 * @brief Set the Window Transparent object
	 * 
	 * @param enable whether enabled
	 * @param alpha value
	 */
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
		wstring fontName; // font face name
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
		inline rectBUTTON& display(PIMAGE pimg = NULL);
		inline rectBUTTON& size(int _width, int _height);
		inline rectBUTTON& bgcolor(color_t _color);
		inline rectBUTTON& textcolor(color_t _color);
		inline rectBUTTON& addtext(wstring _text);
		inline rectBUTTON& poptext();
		inline rectBUTTON& cleartext();
		inline rectBUTTON& fontname(wstring _fontName);
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
		inline circBUTTON& display(PIMAGE pimg = NULL);
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
		inline rectCBOX& display(PIMAGE pimg = NULL);
		inline int gwidth();
		inline int gheight();
		inline std::pair<int,int> gsize();
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

	struct rCBOXtextS {
		PIMAGE textImage;
		rectCBOX checkBox;
		lineTextS boxText;
		int blankWidth;

		explicit rCBOXtextS();
		~rCBOXtextS();

		inline rCBOXtextS& detect();
		inline rCBOXtextS& draw();
		inline rCBOXtextS& display(int _X, int _Y, PIMAGE pimg = NULL);
	};

} // inline namespace checkbox

inline namespace page {

	class pageS;
	class scrBarS;

	struct itemS {
		unsigned iType;
		int locX, locY;
		union {
			/* 0 */ pageS* subPage;
			/* 1 */ lineTextS* lText;
			// /* 2 */ conditionText* cdtnText; // to do
			/* 3 */ rectBUTTON* rButton;
			/* 4 */ circBUTTON* cButton;
			/* 5 */ rectCBOX* rChkBox;
			/* 6 */ rCBOXtextS* cBText;
		} info;
		inline operator decltype(info)& () { return info; }
		inline itemS() = default;
		inline itemS(decltype(iType) _iType) : iType(_iType) {};

		inline void move(int _X, int _Y) { locX = _X, locY = _Y; }

		template <typename _Ftn_t>
		inline void work(_Ftn_t _ftn) {
			switch(iType) {
				case 0: { _ftn(*info.subPage); } break;
				case 1: { _ftn(*info.lText); } break;
				// case 2: { _ftn(*info.cdtnText); } break;
				case 3: { _ftn(*info.rButton); } break;
				case 4: { _ftn(*info.cButton); } break;
				case 5: { _ftn(*info.rChkBox); } break;
				case 6: { _ftn(*info.cBText); } break;
			}
		}
		template <typename _Ftn_t,
		          typename _Rtn_t>
		inline _Rtn_t workRet(_Ftn_t _ftn) {
			register _Rtn_t ret;
			switch(iType) {
				case 0: { ret = _ftn(*info.subPage); } break;
				case 1: { ret = _ftn(*info.lText); } break;
				// case 2: { ret = _ftn(*info.cdtnText); } break;
				case 3: { ret = _ftn(*info.rButton); } break;
				case 4: { ret = _ftn(*info.cButton); } break;
				case 5: { ret = _ftn(*info.rChkBox); } break;
				case 6: { ret = _ftn(*info.cBText); } break;
			}
			return ret;
		}
		inline ~itemS() { work([](auto _itm)->void{delete(&_itm);}); }
	};

	struct scrBarS {
		double fromRatio;
		double toRatio;
		int barWidth;
		int barLength;
		// To do: drag function.
		inline void draw();
		inline void display();
	};

	class pageS {
	  public:
		typedef itemS item_t;
		typedef vector<item_t> ctn_t;
		typedef scrBarS scroll_t;
	  private:
		PIMAGE pageImage;
		int sizeX, sizeY;
		int locX, locY;
		ctn_t content;
		struct { int lX,lY; } dispSize;
		scroll_t scrBarV, scrBarH;
		bool enableVBar, enableHBar;
	  public:
		pageS() : pageImage(newimage()) {};
		pageS(int _sizeX,
		      int _sizeY,
		      int _locX,
		      int _locY)
			: sizeX(_sizeX),
			  sizeY(_sizeY),
			  locX(_locX),
			  locY(_locY) {
			pageImage = newimage(_sizeX, _sizeY);
		};
		~pageS() { delimage(pageImage); }

		inline /* void */ pageS& detect(); // overall detect (for interactive items, e.g.buttons)
		inline /* void */ pageS& draw(); // draw page backstage
		inline /* void */ pageS& display(PIMAGE pimg = NULL); // draw page frontstage (including backstage)

		inline /* void */ pageS& addItem(itemS _item);
		inline /* void */ pageS& popItem();
		inline /* void */ pageS& delItem(int _pos);
		inline item_t& getItem(int _pos);
		inline ctn_t::iterator getItemIt(int _pos);
	};

} // inline namespace page

_GLIB_NAMESPACE_TAIL // inline namespace glib

#endif // __LG_GLIB_HEAD_HPP__
