/* This is GLIB_HEAD.hpp file of SZXC EGE graphics lib.                  */
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

#ifndef LG_GLIB_HEAD_HPP_
#define LG_GLIB_HEAD_HPP_

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
using std::min;
using std::max;
using std::vector;
using std::deque;

#include <graphics.h>

#include "../LGdef.hpp"

/* quick function for window visibility */
inline __attribute__((always_inline)) bool windowIsVisible() { return (::GetWindowLong(getHWnd(), GWL_STYLE) & WS_VISIBLE) != 0; }

_GLIB_NAMESPACE_HEAD

/**
 * @brief Namespace for operations on images.
 */
namespace images {

	/**
	 * @brief Copy one image to another.
	 * @param dstimg destination image
	 * @param srcimg source image
	 */
	void copyImage(PIMAGE& dstimg, PIMAGE& srcimg);
	/**
	 * @brief Zoom a image.
	 * @param pimg target image
	 * @param zoomWidth target width
	 * @param zoomHeight target height
	 */
	void zoomImage(PIMAGE& pimg, int zoomWidth, int zoomHeight);
	/**
	 * @brief Set the Window Transparent object
	 * @param enable whether enabled
	 * @param alpha value
	 */
	void setWindowTransparent(bool enable, int alpha = 0xFF);

}  // namespace images

/**
 * @brief Inline namespace for texts.
 */
inline namespace text {

	/**
	 * @category glib.text
	 * @brief Easy struct for a piece of text.
	 */
	struct singleTextS {
		color_t color;
		wstring text, font;
		int fontHeight, fontWidth;
		singleTextS() {};
		singleTextS(color_t _color, wstring _text) :
		    color(_color), text(_text) {};
		singleTextS(color_t _color, wstring _text, wstring _font, int _fH, int _fW = 0) :
		    color(_color), text(_text), font(_font), fontHeight(_fH), fontWidth(_fW) {};

		int width(PIMAGE _pimg = NULL) {
			setfont(fontHeight, fontWidth, font.c_str(), _pimg);
			return textwidth(text.c_str(), _pimg);
		}
		int height(PIMAGE _pimg = NULL) {
			setfont(fontHeight, fontWidth, font.c_str(), _pimg);
			return textheight(text.c_str(), _pimg);
		}

		void print(int _X, int _Y, PIMAGE _pimg = NULL) {
			setcolor(color, _pimg);
			setfont(fontHeight, fontWidth, font.c_str(), _pimg);
			outtextxy(_X, _Y, text.c_str(), _pimg);
		}
	};

	/**
	 * @category glib.text
	 * @brief Easy struct for a text attached to a variable integer.
	 */
	struct varIntTextS {
		int* var;
		color_t color;
		wstring sbText;
		wstring font;
		int fontHeight, fontWidth;
		varIntTextS() {};
		varIntTextS(int* _var) :
		    var(_var) {};
		varIntTextS(int* _var, color_t _color) :
		    var(_var), color(_color) {};
		varIntTextS(int* _var, color_t _color, wstring _sbText) :
		    var(_var), color(_color), sbText(_sbText) {};
		varIntTextS(int* _var, color_t _color, wstring _sbText, wstring _font, int _fH, int _fW = 0) :
		    var(_var), color(_color), sbText(_sbText), font(_font), fontHeight(_fH), fontWidth(_fW) {};

		int width(PIMAGE _pimg = NULL) {
			setfont(fontHeight, fontWidth, font.c_str(), _pimg);
			return textwidth(to_wstring(*var).c_str(), _pimg);
		}
		int height(PIMAGE _pimg = NULL) {
			setfont(fontHeight, fontWidth, font.c_str(), _pimg);
			return textheight(to_wstring(*var).c_str(), _pimg);
		}

		void print(int _X, int _Y, PIMAGE _pimg = NULL) {
			setcolor(color, _pimg);
			setfont(fontHeight, fontWidth, font.c_str(), _pimg);
			outtextxy(_X, _Y, (sbText + to_wstring(*var)).c_str(), _pimg);
		}
	};

	/**
	 * @category glib.text
	 * @brief Struct for texts on a single line.
	 */
	struct lineTextS {
		deque<singleTextS> text;
		lineTextS() {};
		lineTextS(deque<singleTextS> _text) :
		    text(_text) {};

		void push_front(singleTextS _stext) { text.push_front(_stext); }
		void push_front(color_t _color, wstring _stext) { text.emplace_front(_color, _stext); }
		void push_back(singleTextS _stext) { text.push_back(_stext); }
		void push_back(color_t _color, wstring _stext) { text.emplace_back(_color, _stext); }
		void pop_front() { text.pop_front(); }
		void pop_back() { text.pop_back(); }

		int width(PIMAGE _pimg = NULL) {
			int ret = 0;
			for(auto& t: text) ret += t.width(_pimg);
			return ret;
		}
		int height(PIMAGE _pimg = NULL) {
			int ret = 0;
			for(auto& t: text) ret = max(ret, t.height(_pimg));
			return ret;
		}

		void print(int _X, int _Y, PIMAGE _pimg = NULL) {
			int x = _X, y = _Y;
			for(auto& t: text) {
				t.print(x, y, _pimg);
				x += t.width();
			}
		}
	};

}  // namespace text

inline namespace button {

	class rectBUTTON {
	   private:
		PIMAGE buttonImage;      // image info
		PIMAGE backgroundImage;  // background image
		int backgroundImageWidth, backgroundImageHeight;
		int buttonWidth, buttonHeight;  // width & height
		color_t backgroundColor;        // background color
		color_t textColor;              // text color
		vector<wstring> text;           // text
		wstring fontName;               // font face name
		int fontHeight, fontWidth;      // font height & width
		int frameWidth;
		bool enableAutoFrameColor;
		color_t frameColor;
		int walign, halign;        // align method
		int locationX, locationY;  // location on screen
	   public:
		bool enableTextShadow;
		int textShadowWeight;
		bool enableShadow;
		bool enableButtonShadow;
		int status;                        // button status: free(0) / cursor-on(1) / clicked(2)
		std::function<void()> clickEvent;  // event when clicked
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
		inline rectBUTTON& bgsize(int _width, int _height);
		inline rectBUTTON& delbgimage();
		LG_DEPRECATED inline rectBUTTON& detect();
		inline bool detect(mouse_msg _mouse);
	};

	class circBUTTON {
	   private:
		PIMAGE buttonImage;  // image info
		PIMAGE backgroundImage;
		int backgroundImageWidth, backgroundImageHeight;
		int buttonRadius;           // radius
		color_t backgroundColor;    // background color
		color_t textColor;          // text color
		vector<wstring> text;       // text
		wstring fontName;           // font face name
		int fontHeight, fontWidth;  // font height & width
		int frameWidth;
		bool enableAutoFrameColor;
		color_t frameColor;
		int walign, halign;        // align method
		int locationX, locationY;  // location on screen
	   public:
		bool enableTextShadow;
		int textShadowWeight;
		bool enableShadow;
		bool enableButtonShadow;
		int status;                        // button status: free(0) / cursor-on(1) / clicked(2)
		std::function<void()> clickEvent;  // event when clicked
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
		inline circBUTTON& fontname(wstring _fontName);
		inline circBUTTON& fontsize(int _fontHeight, int _fontWidth);
		inline circBUTTON& move(int _X, int _Y);
		inline circBUTTON& textalign(int _walign = -1, int _halign = -1);
		inline circBUTTON& event(std::function<void()> event);
		inline circBUTTON& frame(int _width);
		inline circBUTTON& framecolor(bool _enableAuto = 1, color_t _color = 0xffffffff);
		inline circBUTTON& bgimage(PIMAGE _img);
		inline circBUTTON& bgsize(int _width, int _height);
		inline circBUTTON& delbgimage();
		LG_DEPRECATED inline circBUTTON& detect();
		inline bool detect(mouse_msg _mouse);
	};
}  // namespace button

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
		inline std::pair<int, int> gsize();
		inline rectCBOX& size(int _width, int _height);
		inline rectCBOX& bgcolor(color_t _color);
		inline rectCBOX& move(int _X, int _Y);
		inline rectCBOX& event(decltype(clickEvent) _event);
		inline rectCBOX& frame(int _width);
		inline rectCBOX& framecolor(color_t _color);
		inline rectCBOX& fillcolor(color_t _color);
		LG_DEPRECATED inline rectCBOX& detect();
		inline bool detect(mouse_msg _mouse);
		inline rectCBOX& variable(bool* _varPtr);
		inline rectCBOX& changeState();
	};

	struct rCBOXtextS {
		int locX, locY;
		color_t bgColor;
		PIMAGE textImage;
		rectCBOX checkBox;
		lineTextS boxText;
		int blankWidth;

		explicit rCBOXtextS();
		~rCBOXtextS();

		inline rCBOXtextS& move(int _X, int _Y);

		LG_DEPRECATED inline rCBOXtextS& detect();
		inline bool detect(mouse_msg _mouse);
		inline rCBOXtextS& draw();
		inline rCBOXtextS& display(int _X, int _Y, PIMAGE pimg = NULL);
	};

}  // namespace checkbox

inline namespace page {

	class pageS;
	class scrBarS;

	// Enum to specify type of a page item.
	enum item_type {
		ITEM_SUBPAGE = 0,
		ITEM_VARINTTEXT,
		ITEM_LINETEXT,
		ITEM_CONDTEXT,
		ITEM_RECTBUTTON,
		ITEM_CIRCBUTTON,
		ITEM_RECTCHKBOX,
		ITEM_RECTCHKBOX_WITH_TEXT,
	};

	struct itemS {
		item_type iType;
		int locX, locY;
		union {
			pageS* subPage;
			varIntTextS* vIText;
			lineTextS* lText;
			// conditionText* cdtnText; // todo))
			rectBUTTON* rButton;
			circBUTTON* cButton;
			rectCBOX* rChkBox;
			rCBOXtextS* cBText;
		} info;

		inline itemS() = default;
		inline itemS(decltype(iType) _iType) :
		    iType(_iType) {};
		inline itemS(decltype(iType) _iType, int _locX, int _locY) :
		    iType(_iType), locX(_locX), locY(_locY) {};

		inline void move(int _X, int _Y);
		inline void downLoc();

		template <typename _Ftn_t>
		inline void work(_Ftn_t _ftn);
		template <typename _Ftn_t,
		          typename _Rtn_t>
		inline _Rtn_t workRet(_Ftn_t _ftn);
		// inline ~itemS() { work([](auto _itm)->void{delete(&_itm);}); }
	};

	// todo))
	struct scrBarS {
		double fromRatio;
		double toRatio;
		int barWidth;
		int barLength;
		// Todo)): drag function.
		inline void draw();
		inline void display();
	};

	class pageS {
	   public:
		// specify class types
		typedef itemS item_t;
		typedef vector<item_t> ctn_t;
		typedef scrBarS scroll_t;
	   private:
		PIMAGE pageImage;
		pageS* parentPage;
		int sizeX, sizeY;
		color_t bgColor;
		int locX, locY;
		struct {
			int lX, lY;
		} dispSize;
		ctn_t content;
		scroll_t scrBarV, scrBarH;
		bool enableVBar, enableHBar;
	   public:
		pageS() :
		    pageImage(newimage()), parentPage(NULL) {};
		pageS(pageS* _parent) :
		    pageImage(newimage()), parentPage(_parent) {};
		pageS(pageS* _parent,
		      int _sizeX,
		      int _sizeY,
		      int _locX,
		      int _locY) :
		    sizeX(_sizeX),
		    sizeY(_sizeY),
		    locX(_locX),
		    locY(_locY),
		    dispSize(decltype(dispSize){ _sizeX, _sizeY }) {
			pageImage = newimage(_sizeX, _sizeY);
			parentPage = _parent;
		};
		~pageS() { delimage(pageImage); }

		inline pageS& size(int _sizeX, int _sizeY);
		inline pageS& width(int _sizeX);
		inline pageS& height(int _sizeY);
		inline pageS& move(int _locX, int _locY);
		inline pageS& setBgColor(color_t _color);
		inline pageS& dSize(int _sizeX, int _sizeY);
		inline pageS& dWidth(int _sizeX);
		inline pageS& dHeight(int _sizeY);

		// overall mouse & keyboard detect (for interactive items, e.g.buttons)
		// this mode of message collecting is deprecated, and will be removed.
		LG_DEPRECATED inline pageS& detect();
		// overall mouse & keyboard detect (for interactive items, e.g.buttons)
		// right implementation.
		inline bool detect(mouse_msg _mouse);
		// draw page backstage
		inline pageS& draw();
		// draw page frontstage (including backstage)
		inline pageS& display(PIMAGE pimg);

		// get the content itself
		inline ctn_t& gContent();
		// get the content items count
		inline ctn_t::size_type cSize() const;
		// add a new page item
		inline pageS& addItem(const pageS::item_t& _item);
		// pop the last page item
		inline pageS& popItem();
		// delete a specific page item
		inline pageS& delItem(int _pos);
		// get the page item (itself) at the front
		inline item_t& frontItem();
		// get the page item (itself) at the back
		inline item_t& backItem();
		// get a specific page item (itself)
		inline item_t& getItem(int _pos);
		// get a specific page item (its iterator)
		inline ctn_t::iterator getItemIt(int _pos);

		inline __attribute__((always_inline)) void run();  // todo)) how to run?
	};

}  // namespace page

_GLIB_NAMESPACE_TAIL  // inline namespace glib

#endif  // LG_GLIB_HEAD_HPP_
