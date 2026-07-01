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

#ifndef LG_GLIB_PAGE_HPP_
#define LG_GLIB_PAGE_HPP_

#include "GLIB_HEAD.hpp"

_GLIB_NAMESPACE_HEAD

namespace page {

inline void Item::move(int _X, int _Y) { locX = _X, locY = _Y; }
inline void Item::downLoc() {
    switch (iType) {
        case ItemType::SUBPAGE: {
            info.subPage->move(locX, locY);
        } break;
        // case ItemType::TEXT_COND: { info.cdtnText->move(locX,locY); } break;
        case ItemType::BUTTON_RECT: {
            info.rButton->move(locX, locY);
        } break;
        case ItemType::BUTTON_CIRC: {
            info.cButton->move(locX, locY);
        } break;
        case ItemType::CHKBOX_RECT: {
            info.rChkBox->move(locX, locY);
        } break;
        case ItemType::CHKBOX_RECT_WITH_TEXT: {
            info.cBText->move(locX, locY);
        } break;
    }
}
template <typename _Ftn_t>
inline void Item::work(_Ftn_t _ftn) {
    switch (iType) {
        case ItemType::SUBPAGE: {
            _ftn(*info.subPage);
        } break;
        case ItemType::TEXT_VARINT: {
            _ftn(*info.vIText);
        } break;
        case ItemType::TEXT_LINE: {
            _ftn(*info.lText);
        } break;
        // case ItemType::TEXT_COND: { _ftn(*info.cdtnText); } break;
        case ItemType::BUTTON_RECT: {
            _ftn(*info.rButton);
        } break;
        case ItemType::BUTTON_CIRC: {
            _ftn(*info.cButton);
        } break;
        case ItemType::CHKBOX_RECT: {
            _ftn(*info.rChkBox);
        } break;
        case ItemType::CHKBOX_RECT_WITH_TEXT: {
            _ftn(*info.cBText);
        } break;
    }
}
template <typename _Ftn_t, typename _Rtn_t>
inline _Rtn_t Item::workRet(_Ftn_t _ftn) {
    _Rtn_t ret;
    switch (iType) {
        case ItemType::SUBPAGE: {
            ret = _ftn(*info.subPage);
        } break;
        case ItemType::TEXT_VARINT: {
            ret = _ftn(*info.vIText);
        } break;
        case ItemType::TEXT_LINE: {
            ret = _ftn(*info.lText);
        } break;
        // case ItemType::TEXT_COND: { ret = _ftn(*info.cdtnText); } break;
        case ItemType::BUTTON_RECT: {
            ret = _ftn(*info.rButton);
        } break;
        case ItemType::BUTTON_CIRC: {
            ret = _ftn(*info.cButton);
        } break;
        case ItemType::CHKBOX_RECT: {
            ret = _ftn(*info.rChkBox);
        } break;
        case ItemType::CHKBOX_RECT_WITH_TEXT: {
            ret = _ftn(*info.cBText);
        } break;
    }
    return ret;
}

inline void ScrBar::draw() {}     // todo))
inline void ScrBar::display() {}  // todo))

inline Page& Page::size(int _sizeX, int _sizeY) {
    sizeX = _sizeX;
    sizeY = _sizeY;
    return *this;
}
inline Page& Page::width(int _sizeX) {
    sizeX = _sizeX;
    return *this;
}
inline Page& Page::height(int _sizeY) {
    sizeY = _sizeY;
    return *this;
}
inline Page& Page::move(int _locX, int _locY) {
    locX = _locX;
    locY = _locY;
    return *this;
}
inline Page& Page::setBgColor(color_t _color) {
    bgColor = _color;
    return *this;
}
inline Page& Page::dSize(int _sizeX, int _sizeY) {
    dispSize = {_sizeX, _sizeY};
    return *this;
}
inline Page& Page::dWidth(int _sizeX) {
    dispSize.lX = _sizeX;
    return *this;
}
inline Page& Page::dHeight(int _sizeY) {
    dispSize.lY = _sizeY;
    return *this;
}

inline Page& Page::detect() {
    for (auto& it : content) {
        switch (it.iType) {
            case ItemType::SUBPAGE: {
                it.info.subPage->detect();
            } break;
            case ItemType::BUTTON_RECT: {
                it.info.rButton->detect();
            } break;
            case ItemType::BUTTON_CIRC: {
                it.info.cButton->detect();
            } break;
            case ItemType::CHKBOX_RECT: {
                it.info.rChkBox->detect();
            } break;
            case ItemType::CHKBOX_RECT_WITH_TEXT: {
                it.info.cBText->detect();
            } break;         // to do
            default: break;  // nothing
        }
    }
    return *this;
}
inline bool Page::detect(mouse_msg _mouse) {
    _mouse.x -= locX;
    _mouse.y -= locY;
    _mouse.x = _mouse.x * sizeX / dispSize.lX;
    _mouse.y = _mouse.y * sizeY / dispSize.lY;
    if (_mouse.x < 0 || _mouse.y < 0 || _mouse.x > sizeX || _mouse.y > sizeY)
        return false;
    for (auto& it : content) {
        bool success = false;
        switch (it.iType) {
            case ItemType::SUBPAGE: {
                success = it.info.subPage->detect(_mouse);
            } break;
            case ItemType::BUTTON_RECT: {
                success = it.info.rButton->detect(_mouse);
            } break;
            case ItemType::BUTTON_CIRC: {
                success = it.info.cButton->detect(_mouse);
            } break;
            case ItemType::CHKBOX_RECT: {
                success = it.info.rChkBox->detect(_mouse);
            } break;
            case ItemType::CHKBOX_RECT_WITH_TEXT: {
                success = it.info.cBText->detect(_mouse);
            } break;         // to do
            default: break;  // nothing
        }
        if (success) return success;  // only one.
    }
    return false;  // nothing detected.
}
inline Page& Page::draw() {
    // printf("IN DRAW FUNCTION.\n");
    delimage(pageImage);
    pageImage = newimage(sizeX, sizeY);
    cleardevice(pageImage);
    setbkcolor(bgColor, pageImage);
    setbkcolor_f(bgColor, pageImage);
    for (auto& it : content) {
        switch (it.iType) {
            case ItemType::SUBIMAGE: {
                putimage(pageImage, it.locX, it.locY, it.info.subImage);
            } break;
            case ItemType::SUBPAGE: {
                it.info.subPage->display(pageImage);
            } break;
            case ItemType::TEXT_VARINT: {
                it.info.vIText->print(it.locX, it.locY, pageImage);
            } break;
            case ItemType::TEXT_LINE: {
                it.info.lText->print(it.locX, it.locY, pageImage);
            } break;
            // case ItemType::TEXT_COND: { it.info.cdtnText->print(); } break;
            case ItemType::BUTTON_RECT: {
                it.info.rButton->display(pageImage);
            } break;
            case ItemType::BUTTON_CIRC: {
                it.info.cButton->display(pageImage);
            } break;
            case ItemType::CHKBOX_RECT: {
                it.info.rChkBox->display(pageImage);
            } break;
            case ItemType::CHKBOX_RECT_WITH_TEXT: {
                it.info.cBText->display(it.locX, it.locY, pageImage);
            } break;
        }
    }
    return *this;
}
inline Page& Page::display(PIMAGE pimg) {
    draw();
    putimage(pimg, locX, locY, dispSize.lX, dispSize.lY, pageImage, 0, 0, sizeX,
             sizeY);
    return *this;
}

inline Page::ctn_t& Page::gContent() { return content; }
inline Page::ctn_t::size_type Page::cSize() const { return content.size(); }
inline Page& Page::addItem(const Page::item_t& _item) {
    content.push_back(_item);
    return *this;
}
inline Page& Page::popItem() {
    content.pop_back();
    return *this;
}
inline Page& Page::delItem(int _pos) {
    content.erase(content.begin() + _pos);
    return *this;
}
inline Page::item_t& Page::frontItem() { return content.front(); }
inline Page::item_t& Page::backItem() { return content.back(); }
inline Page::item_t& Page::getItem(int _pos) { return content.at(_pos); }
inline Page::ctn_t::iterator Page::getItemIt(int _pos) {
    return content.begin() + _pos;
}

inline __attribute__((always_inline)) void Page::run() {
    for (; is_run();) {
        if (!windowIsVisible()) continue;  // maybe this is not true
        detect();
        // must be something here.
        if (!parentPage)
            display(parentPage->pageImage);
        else
            display(NULL);
    }
}
}  // namespace page

_GLIB_NAMESPACE_TAIL

#endif  // LG_GLIB_PAGE_HPP_
