/* This is GLIB_rectbut.hpp file of SZXC-WG EGE graphics lib.            */
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

#ifndef LG_GLIB_RECTBUT_HPP_
#define LG_GLIB_RECTBUT_HPP_

#include "GLIB_HEAD.hpp"

_GLIB_NAMESPACE_HEAD

namespace button {

Rect::Rect() {
    buttonImage = ege::newimage();
    backgroundImage = nullptr;
    buttonWidth = buttonHeight = 1;
    walign = ege::LEFT_TEXT, halign = ege::TOP_TEXT;
    frameWidth = 1;
    status = 0;
    enableAutoFrameColor = true;
    enableTextShadow = true;
    textShadowWeight = 1;
    enableShadow = 1;
    enableButtonShadow = 1;
}
Rect::~Rect() {
    delimage(buttonImage);
    delimage(backgroundImage);
}
Rect::Rect(int _width, int _height) {
    Rect();
    buttonHeight = _height, buttonWidth = _width;
}
Rect::Rect(Rect&& but) {
    delimage(buttonImage);
    buttonImage = but.buttonImage;
    buttonHeight = but.buttonHeight, buttonWidth = but.buttonWidth;
    backgroundColor = but.backgroundColor, textColor = but.textColor;
    text = but.text;
}
Rect::Rect(const Rect& but) {
    delimage(buttonImage);
    buttonImage = but.buttonImage;
    buttonHeight = but.buttonHeight, buttonWidth = but.buttonWidth;
    backgroundColor = but.backgroundColor, textColor = but.textColor;
    text = but.text;
}
inline Rect& Rect::draw() {
    delimage(buttonImage);
    if (enableShadow && enableButtonShadow)
        buttonImage = ege::newimage(buttonWidth + 3, buttonHeight + 3);
    else
        buttonImage = ege::newimage(buttonWidth, buttonHeight);
    ege::ege_enable_aa(buttonImage);
    ege::setbkcolor(0xff222222, buttonImage);
    ege::setbkcolor_f(0xff222222, buttonImage);
    ege::setbkmode(TRANSPARENT, buttonImage);
    if ((status == 1 || status == 2) && enableShadow && enableButtonShadow) {
        ege::setfillcolor(0xff008080, buttonImage);
        bar(3, 3, buttonWidth + 3, buttonHeight + 3, buttonImage);
    }
    ege::setfillcolor(backgroundColor, buttonImage);
    bar(0, 0, buttonWidth, buttonHeight, buttonImage);
    if (backgroundImage != nullptr) {
        if (ege::getwidth(backgroundImage) != backgroundImageWidth ||
            ege::getheight(backgroundImage) != backgroundImageHeight)
            images::zoom(backgroundImage, backgroundImageWidth,
                         backgroundImageHeight);
        putimage_withalpha(buttonImage, backgroundImage, 0, 0);
    }
    ege::setfont(-fontHeight, fontWidth, fontName.c_str(), buttonImage);
    ege::settextjustify(walign, halign, buttonImage);
    int ox, oy;
    if (walign == ege::LEFT_TEXT)
        ox = 0;
    else if (walign == ege::CENTER_TEXT)
        ox = buttonWidth / 2;
    else
        ox = buttonWidth - 1;
    if (halign == ege::TOP_TEXT)
        oy = 0;
    else if (halign == ege::CENTER_TEXT)
        oy = (buttonHeight - fontHeight * (text.size() - 1)) / 2;
    else
        oy = buttonHeight - fontHeight * (text.size() - 1) - 1;
    for (auto s : text) {
        if (enableShadow && enableTextShadow) {
            ege::setcolor(0xff008080, buttonImage);
            ege::outtextxy(ox + textShadowWeight, oy + textShadowWeight,
                           s.c_str(), buttonImage);
        }
        ege::setcolor(textColor, buttonImage);
        ege::outtextxy(ox, oy, s.c_str(), buttonImage);
        oy += fontHeight;
    }
    if (!enableShadow) {
        if (enableAutoFrameColor)
            ege::setcolor(0xff000000 | ~backgroundColor, buttonImage);
        else
            ege::setcolor(frameColor, buttonImage);
        setlinewidth(frameWidth, buttonImage);
        if (status == 1 || status == 2)
            ege::rectangle(1, 1, buttonWidth, buttonHeight, buttonImage);
    } else {
        if (status == 1 || status == 2) {
            ege::setfillcolor(0x80808080, buttonImage);
            ege::ege_fillrect(0, 0, buttonWidth, buttonHeight, buttonImage);
        }
    }
    return *this;
}
inline Rect& Rect::display(ege::PIMAGE pimg) {
    draw();
    ege::putimage(pimg, locationX, locationY, buttonImage);
    return *this;
}
inline Rect& Rect::size(int _width, int _height) {
    buttonHeight = _height;
    buttonWidth = _width;
    return *this;
}
inline Rect& Rect::bgcolor(ege::color_t _color) {
    backgroundColor = _color;
    return *this;
}
inline Rect& Rect::textcolor(ege::color_t _color) {
    textColor = _color;
    return *this;
}
inline Rect& Rect::addtext(std::wstring _text) {
    text.push_back(_text);
    return *this;
}
inline Rect& Rect::poptext() {
    if (!text.empty()) text.pop_back();
    return *this;
}
inline Rect& Rect::cleartext() {
    text.clear();
    return *this;
}
inline Rect& Rect::fontname(std::wstring _fontName) {
    fontName = _fontName;
    return *this;
}
inline Rect& Rect::fontsize(int _fontHeight, int _fontWidth) {
    fontHeight = _fontHeight;
    fontWidth = _fontWidth;
    return *this;
}
inline Rect& Rect::move(int _X, int _Y) {
    locationY = _Y, locationX = _X;
    return *this;
}
inline Rect& Rect::textalign(int _walign, int _halign) {
    if (~_walign) walign = _walign;
    if (~_halign) halign = _halign;
    return *this;
}
inline Rect& Rect::event(std::function<void()> event) {
    clickEvent = event;
    return *this;
}
inline Rect& Rect::frame(int _width) {
    frameWidth = _width;
    return *this;
}
inline Rect& Rect::framecolor(bool _enableAuto, ege::color_t _color) {
    enableAutoFrameColor = _enableAuto, frameColor = _color;
    return *this;
}
inline Rect& Rect::bgimage(ege::PIMAGE _img) {
    if (backgroundImage == nullptr) backgroundImage = ege::newimage();
    images::copy(backgroundImage, _img);
    backgroundImageWidth = ege::getwidth(backgroundImage);
    backgroundImageHeight = ege::getheight(backgroundImage);
    return *this;
}
inline Rect& Rect::bgsize(int _width, int _height) {
    backgroundImageWidth = _width, backgroundImageHeight = _height;
    return *this;
}
inline Rect& Rect::delbgimage() {
    backgroundImage = nullptr;
    return *this;
}
inline Rect& Rect::detect() {
    POINT mousePos;
    GetCursorPos(&mousePos);
    ScreenToClient(ege::getHWnd(), &mousePos);
    if (mousePos.x < locationX ||
        mousePos.x > std::min(locationX + buttonWidth - 1, ege::getwidth()) ||
        mousePos.y < locationY ||
        mousePos.y > std::min(locationY + buttonHeight - 1, ege::getheight()))
        return status = 0, *this;
    while (ege::mousemsg()) {
        ege::mouse_msg msg = ege::getmouse();
        if (!(msg.x < locationX ||
              msg.x > std::min(locationX + buttonWidth - 1, ege::getwidth()) ||
              msg.y < locationY ||
              msg.y >
                  std::min(locationY + buttonHeight - 1, ege::getheight())) &&
            msg.is_left() && msg.is_down())
            return status = 2, *this;
    }
    return status = 1, *this;
}
inline bool Rect::detect(ege::mouse_msg _mouse) {
    _mouse.x -= locationX;
    _mouse.y -= locationY;
    if (_mouse.x < 0 || _mouse.x > buttonWidth - 1 || _mouse.y < 0 ||
        _mouse.y > buttonHeight - 1)
        return status = 0, false;
    if (_mouse.is_left() && _mouse.is_down())
        status = 2;
    else
        status = 1;
    return true;
}

}  // namespace button

_GLIB_NAMESPACE_TAIL

#endif  // LG_GLIB_RECTBUT_HPP_
