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

namespace button {

Circ::Circ() {
    buttonImage = ege::newimage();
    backgroundImage = nullptr;
    buttonRadius = 1;
    walign = ege::LEFT_TEXT, halign = ege::TOP_TEXT;
    frameWidth = 1;
    status = 0;
    enableAutoFrameColor = true;
    enableTextShadow = true;
    textShadowWeight = 1;
    enableShadow = true;
    enableButtonShadow = true;
}
Circ::~Circ() {
    ege::delimage(buttonImage);
    ege::delimage(backgroundImage);
}
Circ::Circ(int _radius) {
    Circ();
    buttonRadius = _radius;
}
Circ::Circ(Circ&& but) {
    ege::delimage(buttonImage);
    buttonImage = but.buttonImage;
    buttonRadius = but.buttonRadius;
    backgroundColor = but.backgroundColor, textColor = but.textColor;
    text = but.text;
}
Circ::Circ(const Circ& but) {
    ege::delimage(buttonImage);
    buttonImage = but.buttonImage;
    buttonRadius = but.buttonRadius;
    backgroundColor = but.backgroundColor, textColor = but.textColor;
    text = but.text;
}
inline Circ& Circ::draw() {
    ege::delimage(buttonImage);
    if (enableShadow && enableButtonShadow)
        buttonImage = ege::newimage(buttonRadius * 2 + 3, buttonRadius * 2 + 3);
    else
        buttonImage = ege::newimage(buttonRadius * 2, buttonRadius * 2);
    ege::ege_enable_aa(buttonImage);
    ege::setbkcolor(0xff222222, buttonImage);
    ege::setbkcolor_f(0xff222222, buttonImage);
    ege::setbkmode(TRANSPARENT, buttonImage);
    if ((status == 1 || status == 2) && enableShadow && enableButtonShadow) {
        ege::setfillcolor(0xff008080, buttonImage);
        ege::fillellipse(buttonRadius + 3, buttonRadius + 3, buttonRadius,
                         buttonRadius, buttonImage);
    }
    ege::setfillcolor(backgroundColor, buttonImage);
    ege::ege_fillellipse(0, 0, 2 * buttonRadius, 2 * buttonRadius, buttonImage);
    if (backgroundImage != nullptr) {
        if (ege::getwidth(backgroundImage) != backgroundImageWidth ||
            ege::getheight(backgroundImage) != backgroundImageHeight)
            images::zoom(backgroundImage, backgroundImageWidth,
                         backgroundImageHeight);
        ege::putimage_withalpha(buttonImage, backgroundImage,
                                buttonRadius - backgroundImageWidth / 2,
                                buttonRadius - backgroundImageHeight / 2);
    }
    ege::setfont(-fontHeight, fontWidth, fontName.c_str(), buttonImage);
    ege::settextjustify(walign, halign, buttonImage);
    int ox, oy;
    if (walign == ege::LEFT_TEXT)
        ox = 0;
    else if (walign == ege::CENTER_TEXT)
        ox = buttonRadius;
    else
        ox = buttonRadius * 2 - 1;
    if (halign == ege::TOP_TEXT)
        oy = 0;
    else if (halign == ege::CENTER_TEXT)
        oy = (buttonRadius * 2 - fontHeight * (text.size() - 1)) / 2;
    else
        oy = buttonRadius * 2 - fontHeight * (text.size() - 1) - 1;
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
        ege::setlinewidth(frameWidth, buttonImage);
        if (status == 1 || status == 2)
            ege::ellipse(buttonRadius, buttonRadius, 0, 360, buttonRadius,
                         buttonRadius, buttonImage);
    } else {
        if (status == 1 || status == 2) {
            ege::setfillcolor(0x80808080, buttonImage);
            ege::ege_fillellipse(0, 0, buttonRadius * 2, buttonRadius * 2,
                                 buttonImage);
        }
    }
    return *this;
}
inline Circ& Circ::display(ege::PIMAGE pimg) {
    draw();
    ege::putimage_withalpha(pimg, buttonImage, locationX - buttonRadius,
                            locationY - buttonRadius);
    return *this;
}
inline Circ& Circ::radius(int _radius) {
    buttonRadius = _radius;
    return *this;
}
inline Circ& Circ::bgcolor(ege::color_t _color) {
    backgroundColor = _color;
    return *this;
}
inline Circ& Circ::textcolor(ege::color_t _color) {
    textColor = _color;
    return *this;
}
inline Circ& Circ::addtext(std::wstring _text) {
    text.push_back(_text);
    return *this;
}
inline Circ& Circ::poptext() {
    if (!text.empty()) text.pop_back();
    return *this;
}
inline Circ& Circ::cleartext() {
    text.clear();
    return *this;
}
inline Circ& Circ::fontname(std::wstring _fontName) {
    fontName = _fontName;
    return *this;
}
inline Circ& Circ::fontsize(int _fontHeight, int _fontWidth) {
    fontHeight = _fontHeight;
    fontWidth = _fontWidth;
    return *this;
}
inline Circ& Circ::move(int _X, int _Y) {
    locationY = _Y, locationX = _X;
    return *this;
}
inline Circ& Circ::textalign(int _walign, int _halign) {
    if (~_walign) walign = _walign;
    if (~_halign) halign = _halign;
    return *this;
}
inline Circ& Circ::event(std::function<void()> event) {
    clickEvent = event;
    return *this;
}
inline Circ& Circ::frame(int _width) {
    frameWidth = _width;
    return *this;
}
inline Circ& Circ::framecolor(bool _enableAuto, ege::color_t _color) {
    enableAutoFrameColor = _enableAuto, frameColor = _color;
    return *this;
}
inline Circ& Circ::bgimage(ege::PIMAGE _img) {
    if (backgroundImage == nullptr) backgroundImage = ege::newimage();
    images::copy(backgroundImage, _img);
    backgroundImageWidth = ege::getwidth(backgroundImage);
    backgroundImageHeight = ege::getheight(backgroundImage);
    return *this;
}
inline Circ& Circ::bgsize(int _width, int _height) {
    backgroundImageWidth = _width, backgroundImageHeight = _height;
    return *this;
}
inline Circ& Circ::delbgimage() {
    backgroundImage = nullptr;
    return *this;
}
inline Circ& Circ::detect() {
    POINT mousePos;
    GetCursorPos(&mousePos);
    ScreenToClient(ege::getHWnd(), &mousePos);
    double dist = std::hypot(mousePos.x - locationX, mousePos.y - locationY);
    if (dist > buttonRadius) return status = 0, *this;
    while (ege::mousemsg()) {
        ege::mouse_msg msg = ege::getmouse();
        if (std::hypot(msg.x - locationX, msg.y - locationY) <= buttonRadius &&
            msg.is_left() && msg.is_down())
            return status = 2, *this;
    }
    return status = 1, *this;
}
inline bool Circ::detect(ege::mouse_msg _mouse) {
    _mouse.x -= locationX;
    _mouse.y -= locationY;
    if (std::hypot(_mouse.x, _mouse.y) > buttonRadius) return status = 0, false;
    if (_mouse.is_left() && _mouse.is_down())
        status = 2;
    else
        status = 1;
    return true;
}

}  // namespace button

_GLIB_NAMESPACE_TAIL

#endif  // LG_GLIB_CIRCBUT_HPP_
