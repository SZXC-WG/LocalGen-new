/* This is GLIB_rectcbox.hpp file of SZXC-WG EGE graphics lib.           */
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

#ifndef LG_GLIB_RECTCBOX_HPP_
#define LG_GLIB_RECTCBOX_HPP_

#include "GLIB_HEAD.hpp"

_GLIB_NAMESPACE_HEAD

namespace checkbox {

Rect::Rect() {
    boxImage = ege::newimage();
    boxWidth = boxHeight = 1;
    frameColor = fillColor = 0xffffffff;
    status = 0;
    pressed = 0;
    varPtr = nullptr;
};
Rect::~Rect() { ege::delimage(boxImage); }
inline Rect& Rect::draw() {
    ege::delimage(boxImage);
    boxImage = ege::newimage(boxWidth, boxHeight);
    setfillcolor(backgroundColor, boxImage);
    bar(0, 0, boxWidth, boxHeight, boxImage);
    setcolor(frameColor, boxImage);
    setlinewidth(frameWidth, boxImage);
    rectangle(1, 1, boxWidth, boxHeight, boxImage);
    if (pressed) {
        setfillcolor(fillColor, boxImage);
        bar(frameWidth - 1, frameWidth - 1, boxWidth - frameWidth + 1,
            boxHeight - frameWidth + 1, boxImage);
    }
    if (status == 1 || status == 2) {
        setfillcolor(0x80808080, boxImage);
        // ege_fillrect(frameWidth - 1, frameWidth - 1,
        //              boxWidth - 2 * frameWidth + 1, boxHeight - 2 *
        //              frameWidth + 1, boxImage);
        ege_fillrect(0, 0, boxWidth, boxHeight, boxImage);
    }
    return *this;
}
inline Rect& Rect::display(ege::PIMAGE pimg) {
    draw();
    ege::putimage(pimg, locationX, locationY, boxImage);
    return *this;
}
inline int Rect::gwidth() { return boxWidth; }
inline int Rect::gheight() { return boxHeight; }
inline std::pair<int, int> Rect::gsize() {
    return std::make_pair(boxWidth, boxHeight);
}
inline Rect& Rect::size(int _width, int _height) {
    boxWidth = _width, boxHeight = _height;
    return *this;
}
inline Rect& Rect::bgcolor(ege::color_t _color) {
    backgroundColor = _color;
    return *this;
}
inline Rect& Rect::move(int _X, int _Y) {
    locationX = _X, locationY = _Y;
    return *this;
}
inline Rect& Rect::event(decltype(clickEvent) _event) {
    clickEvent = _event;
    return *this;
}
inline Rect& Rect::frame(int _width) {
    frameWidth = _width;
    return *this;
}
inline Rect& Rect::framecolor(ege::color_t _color) {
    frameColor = _color;
    return *this;
}
inline Rect& Rect::fillcolor(ege::color_t _color) {
    fillColor = _color;
    return *this;
}
inline Rect& Rect::detect() {
    POINT mousePos;
    GetCursorPos(&mousePos);
    ScreenToClient(ege::getHWnd(), &mousePos);
    if (mousePos.x < locationX ||
        mousePos.x > std::min(locationX + boxWidth - 1, ege::getwidth()) ||
        mousePos.y < locationY ||
        mousePos.y > std::min(locationY + boxHeight - 1, ege::getheight()))
        return status = 0, *this;
    while (ege::mousemsg()) {
        ege::mouse_msg msg = ege::getmouse();
        if (!(msg.x < locationX ||
              msg.x > std::min(locationX + boxWidth - 1, ege::getwidth()) ||
              msg.y < locationY ||
              msg.y > std::min(locationY + boxHeight - 1, ege::getheight())) &&
            msg.is_left() && msg.is_down())
            return status = 2, *this;
    }
    return status = 1, *this;
}
inline bool Rect::detect(ege::mouse_msg _mouse) {
    _mouse.x -= locationX;
    _mouse.y -= locationY;
    if (_mouse.x < 0 || _mouse.x > boxWidth - 1 || _mouse.y < 0 ||
        _mouse.y > boxHeight - 1)
        return status = 0, false;
    if (_mouse.is_left() && _mouse.is_down())
        status = 2;
    else
        status = 1;
    return true;
}
inline Rect& Rect::variable(bool* _varPtr) {
    varPtr = _varPtr;
    pressed = *varPtr;
    return *this;
}
inline Rect& Rect::changeState() {
    pressed = !pressed;
    if (varPtr != nullptr) *varPtr = pressed;
    return *this;
}

inline RectWithText::RectWithText() {
    textImage = ege::newimage();
    blankWidth = 0;
}
inline RectWithText::~RectWithText() { ege::delimage(textImage); }
inline RectWithText& RectWithText::move(int _X, int _Y) {
    locX = _X;
    locY = _Y;
    return *this;
}
inline RectWithText& RectWithText::detect() {
    checkBox.detect();
    return *this;
}
inline bool RectWithText::detect(ege::mouse_msg _mouse) {
    _mouse.x -= locX;
    _mouse.y -= locY;
    return checkBox.detect(_mouse);
}
inline RectWithText& RectWithText::draw() {
    int totW = boxText.width(textImage) + blankWidth + checkBox.gwidth();
    int totH = std::max(boxText.height(textImage), checkBox.gheight());
    ege::delimage(textImage);
    textImage = ege::newimage(totW, totH);
    ege::setbkcolor(bgColor, textImage);
    ege::setbkcolor_f(bgColor, textImage);
    ege::settextjustify(ege::LEFT_TEXT, ege::TOP_TEXT);
    checkBox.move(0, totH / 2 - checkBox.gheight() / 2);
    checkBox.display(textImage);
    boxText.print(checkBox.gwidth() + blankWidth,
                  totH / 2 - boxText.height(textImage) / 2, textImage);
    return *this;
}
inline RectWithText& RectWithText::display(int _X, int _Y, ege::PIMAGE pimg) {
    draw();
    ege::putimage(pimg, _X, _Y, textImage);
    return *this;
}

}  // namespace checkbox

_GLIB_NAMESPACE_TAIL

#endif  // LG_GLIB_RECTCBOX_HPP_
