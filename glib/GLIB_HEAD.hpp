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

#include <algorithm>
#include <deque>
#include <functional>
#include <string>
#include <utility>
#include <vector>
using std::deque;
using std::max;
using std::min;
using std::string;
using std::to_string;
using std::to_wstring;
using std::vector;
using std::wstring;

#include <graphics.h>

#include "../LGdef.hpp"

/* quick function for window visibility */
inline __attribute__((always_inline)) bool windowIsVisible() {
    return (::GetWindowLong(getHWnd(), GWL_STYLE) & WS_VISIBLE) != 0;
}

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
void copy(PIMAGE& dstimg, PIMAGE& srcimg);
/**
 * @brief Zoom a image.
 * @param pimg target image
 * @param zoomWidth target width
 * @param zoomHeight target height
 */
void zoom(PIMAGE& pimg, int zoomWidth, int zoomHeight);

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
namespace text {

/**
 * @category glib.text
 * @brief Easy struct for a piece of text.
 */
struct Single {
    color_t color;
    wstring text, font;
    int fontHeight, fontWidth;
    Single() {};
    Single(color_t _color, wstring _text) : color(_color), text(_text) {};
    Single(color_t _color, wstring _text, wstring _font, int _fH, int _fW = 0)
        : color(_color),
          text(_text),
          font(_font),
          fontHeight(_fH),
          fontWidth(_fW) {};

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
struct VarInt {
    int* var;
    color_t color;
    wstring sbText;
    wstring font;
    int fontHeight, fontWidth;
    VarInt() {};
    VarInt(int* _var) : var(_var) {};
    VarInt(int* _var, color_t _color) : var(_var), color(_color) {};
    VarInt(int* _var, color_t _color, wstring _sbText)
        : var(_var), color(_color), sbText(_sbText) {};
    VarInt(int* _var, color_t _color, wstring _sbText, wstring _font, int _fH,
           int _fW = 0)
        : var(_var),
          color(_color),
          sbText(_sbText),
          font(_font),
          fontHeight(_fH),
          fontWidth(_fW) {};

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
struct Line {
    deque<Single> text;
    Line() {};
    Line(deque<Single> _text) : text(_text) {};

    void push_front(Single _stext) { text.push_front(_stext); }
    void push_front(color_t _color, wstring _stext) {
        text.emplace_front(_color, _stext);
    }
    void push_back(Single _stext) { text.push_back(_stext); }
    void push_back(color_t _color, wstring _stext) {
        text.emplace_back(_color, _stext);
    }
    void pop_front() { text.pop_front(); }
    void pop_back() { text.pop_back(); }

    int width(PIMAGE _pimg = NULL) {
        int ret = 0;
        for (auto& t : text) ret += t.width(_pimg);
        return ret;
    }
    int height(PIMAGE _pimg = NULL) {
        int ret = 0;
        for (auto& t : text) ret = max(ret, t.height(_pimg));
        return ret;
    }

    void print(int _X, int _Y, PIMAGE _pimg = NULL) {
        int x = _X, y = _Y;
        for (auto& t : text) {
            t.print(x, y, _pimg);
            x += t.width();
        }
    }
};

}  // namespace text

namespace button {

class Rect {
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
    int status;  // button status: free(0) / cursor-on(1) / clicked(2)
    std::function<void()> clickEvent;  // event when clicked
    explicit Rect();
    ~Rect();
    Rect(int _width, int _height);
    Rect(Rect&& but);
    Rect(const Rect& but);
    inline Rect& draw();
    inline Rect& display(PIMAGE pimg = NULL);
    inline Rect& size(int _width, int _height);
    inline Rect& bgcolor(color_t _color);
    inline Rect& textcolor(color_t _color);
    inline Rect& addtext(wstring _text);
    inline Rect& poptext();
    inline Rect& cleartext();
    inline Rect& fontname(wstring _fontName);
    inline Rect& fontsize(int _fontHeight, int _fontWidth);
    inline Rect& move(int _X, int _Y);
    inline Rect& textalign(int _walign = -1, int _halign = -1);
    inline Rect& event(std::function<void()> event);
    inline Rect& frame(int _width);
    inline Rect& framecolor(bool _enableAuto = 1, color_t _color = 0xffffffff);
    inline Rect& bgimage(PIMAGE _img);
    inline Rect& bgsize(int _width, int _height);
    inline Rect& delbgimage();
    LG_DEPRECATED inline Rect& detect();
    inline bool detect(mouse_msg _mouse);
};

class Circ {
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
    int status;  // button status: free(0) / cursor-on(1) / clicked(2)
    std::function<void()> clickEvent;  // event when clicked
    explicit Circ();
    ~Circ();
    Circ(int _radius);
    Circ(Circ&& but);
    Circ(const Circ& but);
    inline Circ& draw();
    inline Circ& display(PIMAGE pimg = NULL);
    inline Circ& radius(int _radius);
    inline Circ& bgcolor(color_t _color);
    inline Circ& textcolor(color_t _color);
    inline Circ& addtext(wstring _text);
    inline Circ& poptext();
    inline Circ& cleartext();
    inline Circ& fontname(wstring _fontName);
    inline Circ& fontsize(int _fontHeight, int _fontWidth);
    inline Circ& move(int _X, int _Y);
    inline Circ& textalign(int _walign = -1, int _halign = -1);
    inline Circ& event(std::function<void()> event);
    inline Circ& frame(int _width);
    inline Circ& framecolor(bool _enableAuto = 1, color_t _color = 0xffffffff);
    inline Circ& bgimage(PIMAGE _img);
    inline Circ& bgsize(int _width, int _height);
    inline Circ& delbgimage();
    LG_DEPRECATED inline Circ& detect();
    inline bool detect(mouse_msg _mouse);
};
}  // namespace button

namespace checkbox {

class Rect {
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
    explicit Rect();
    ~Rect();
    inline Rect& draw();
    inline Rect& display(PIMAGE pimg = NULL);
    inline int gwidth();
    inline int gheight();
    inline std::pair<int, int> gsize();
    inline Rect& size(int _width, int _height);
    inline Rect& bgcolor(color_t _color);
    inline Rect& move(int _X, int _Y);
    inline Rect& event(decltype(clickEvent) _event);
    inline Rect& frame(int _width);
    inline Rect& framecolor(color_t _color);
    inline Rect& fillcolor(color_t _color);
    LG_DEPRECATED inline Rect& detect();
    inline bool detect(mouse_msg _mouse);
    inline Rect& variable(bool* _varPtr);
    inline Rect& changeState();
};

struct RectWithText {
    int locX, locY;
    color_t bgColor;
    PIMAGE textImage;
    checkbox::Rect checkBox;
    text::Line boxText;
    int blankWidth;

    explicit RectWithText();
    ~RectWithText();

    inline RectWithText& move(int _X, int _Y);

    LG_DEPRECATED inline RectWithText& detect();
    inline bool detect(mouse_msg _mouse);
    inline RectWithText& draw();
    inline RectWithText& display(int _X, int _Y, PIMAGE pimg = NULL);
};

}  // namespace checkbox

namespace page {

class Page;
class ScrBar;

// Enum to specify type of a page item.
enum class ItemType {
    SUBIMAGE,
    SUBPAGE,
    TEXT_VARINT,
    TEXT_LINE,
    TEXT_COND,
    BUTTON_RECT,
    BUTTON_CIRC,
    CHKBOX_RECT,
    CHKBOX_RECT_WITH_TEXT,
};

struct Item {
    ItemType iType;
    int locX, locY;
    union {
        PIMAGE subImage;
        Page* subPage;
        text::VarInt* vIText;
        text::Line* lText;
        // text::conditionText* cdtnText; // todo))
        button::Rect* rButton;
        button::Circ* cButton;
        checkbox::Rect* rChkBox;
        checkbox::RectWithText* cBText;
    } info;

    inline Item() = default;
    inline Item(decltype(iType) _iType) : iType(_iType) {};
    inline Item(decltype(iType) _iType, int _locX, int _locY)
        : iType(_iType), locX(_locX), locY(_locY) {};

    inline void move(int _X, int _Y);
    inline void downLoc();

    template <typename _Ftn_t>
    inline void work(_Ftn_t _ftn);
    template <typename _Ftn_t, typename _Rtn_t>
    inline _Rtn_t workRet(_Ftn_t _ftn);
    // inline ~Item() { work([](auto _itm)->void{delete(&_itm);}); }
};

// todo))
struct ScrBar {
    double fromRatio;
    double toRatio;
    int barWidth;
    int barLength;
    // Todo)): drag function.
    inline void draw();
    inline void display();
};

class Page {
   public:
    // specify class types
    typedef Item item_t;
    typedef vector<item_t> ctn_t;
    typedef ScrBar scroll_t;

   private:
    PIMAGE pageImage;
    Page* parentPage;
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
    Page() : pageImage(newimage()), parentPage(NULL) {};
    Page(Page* _parent) : pageImage(newimage()), parentPage(_parent) {};
    Page(Page* _parent, int _sizeX, int _sizeY, int _locX, int _locY)
        : sizeX(_sizeX),
          sizeY(_sizeY),
          locX(_locX),
          locY(_locY),
          dispSize(decltype(dispSize){_sizeX, _sizeY}) {
        pageImage = newimage(_sizeX, _sizeY);
        parentPage = _parent;
    };
    ~Page() { delimage(pageImage); }

    inline Page& size(int _sizeX, int _sizeY);
    inline Page& width(int _sizeX);
    inline Page& height(int _sizeY);
    inline Page& move(int _locX, int _locY);
    inline Page& setBgColor(color_t _color);
    inline Page& dSize(int _sizeX, int _sizeY);
    inline Page& dWidth(int _sizeX);
    inline Page& dHeight(int _sizeY);

    // overall mouse & keyboard detect (for interactive items, e.g.buttons)
    // this mode of message collecting is deprecated, and will be removed.
    LG_DEPRECATED inline Page& detect();
    // overall mouse & keyboard detect (for interactive items, e.g.buttons)
    // right implementation.
    inline bool detect(mouse_msg _mouse);
    // draw page backstage
    inline Page& draw();
    // draw page frontstage (including backstage)
    inline Page& display(PIMAGE pimg);

    // get the content itself
    inline ctn_t& gContent();
    // get the content items count
    inline ctn_t::size_type cSize() const;
    // add a new page item
    inline Page& addItem(const Page::item_t& _item);
    // pop the last page item
    inline Page& popItem();
    // delete a specific page item
    inline Page& delItem(int _pos);
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
