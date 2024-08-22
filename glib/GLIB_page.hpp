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

inline namespace page {

	inline void itemS::move(int _X, int _Y) { locX = _X, locY = _Y; }
	inline void itemS::downLoc() {
		switch(iType) {
			case ITEM_SUBPAGE: {
				info.subPage->move(locX, locY);
			} break;
			// case ITEM_CONDTEXT: { info.cdtnText->move(locX,locY); } break;
			case ITEM_RECTBUTTON: {
				info.rButton->move(locX, locY);
			} break;
			case ITEM_CIRCBUTTON: {
				info.cButton->move(locX, locY);
			} break;
			case ITEM_RECTCHKBOX: {
				info.rChkBox->move(locX, locY);
			} break;
			case ITEM_RECTCHKBOX_WITH_TEXT: {
				info.cBText->move(locX, locY);
			} break;
		}
	}
	template <typename _Ftn_t>
	inline void itemS::work(_Ftn_t _ftn) {
		switch(iType) {
			case ITEM_SUBPAGE: {
				_ftn(*info.subPage);
			} break;
			case ITEM_VARINTTEXT: {
				_ftn(*info.vIText);
			} break;
			case ITEM_LINETEXT: {
				_ftn(*info.lText);
			} break;
			// case ITEM_CONDTEXT: { _ftn(*info.cdtnText); } break;
			case ITEM_RECTBUTTON: {
				_ftn(*info.rButton);
			} break;
			case ITEM_CIRCBUTTON: {
				_ftn(*info.cButton);
			} break;
			case ITEM_RECTCHKBOX: {
				_ftn(*info.rChkBox);
			} break;
			case ITEM_RECTCHKBOX_WITH_TEXT: {
				_ftn(*info.cBText);
			} break;
		}
	}
	template <typename _Ftn_t,
	          typename _Rtn_t>
	inline _Rtn_t itemS::workRet(_Ftn_t _ftn) {
		_Rtn_t ret;
		switch(iType) {
			case ITEM_SUBPAGE: {
				ret = _ftn(*info.subPage);
			} break;
			case ITEM_VARINTTEXT: {
				ret = _ftn(*info.vIText);
			} break;
			case ITEM_LINETEXT: {
				ret = _ftn(*info.lText);
			} break;
			// case ITEM_CONDTEXT: { ret = _ftn(*info.cdtnText); } break;
			case ITEM_RECTBUTTON: {
				ret = _ftn(*info.rButton);
			} break;
			case ITEM_CIRCBUTTON: {
				ret = _ftn(*info.cButton);
			} break;
			case ITEM_RECTCHKBOX: {
				ret = _ftn(*info.rChkBox);
			} break;
			case ITEM_RECTCHKBOX_WITH_TEXT: {
				ret = _ftn(*info.cBText);
			} break;
		}
		return ret;
	}

	inline void scrBarS::draw() {}     // todo))
	inline void scrBarS::display() {}  // todo))

	inline pageS& pageS::size(int _sizeX, int _sizeY) {
		sizeX = _sizeX;
		sizeY = _sizeY;
		return *this;
	}
	inline pageS& pageS::width(int _sizeX) {
		sizeX = _sizeX;
		return *this;
	}
	inline pageS& pageS::height(int _sizeY) {
		sizeY = _sizeY;
		return *this;
	}
	inline pageS& pageS::move(int _locX, int _locY) {
		locX = _locX;
		locY = _locY;
		return *this;
	}
	inline pageS& pageS::setBgColor(color_t _color) {
		bgColor = _color;
		return *this;
	}
	inline pageS& pageS::dSize(int _sizeX, int _sizeY) {
		dispSize = { _sizeX, _sizeY };
		return *this;
	}
	inline pageS& pageS::dWidth(int _sizeX) {
		dispSize.lX = _sizeX;
		return *this;
	}
	inline pageS& pageS::dHeight(int _sizeY) {
		dispSize.lY = _sizeY;
		return *this;
	}

	inline pageS& pageS::detect() {
		for(auto& it: content) {
			switch(it.iType) {
				case ITEM_SUBPAGE: {
					it.info.subPage->detect();
				} break;
				case ITEM_RECTBUTTON: {
					it.info.rButton->detect();
				} break;
				case ITEM_CIRCBUTTON: {
					it.info.cButton->detect();
				} break;
				case ITEM_RECTCHKBOX: {
					it.info.rChkBox->detect();
				} break;
				case ITEM_RECTCHKBOX_WITH_TEXT: {
					it.info.cBText->detect();
				} break;         // to do
				default: break;  // nothing
			}
		}
		return *this;
	}
	inline bool pageS::detect(mouse_msg _mouse) {
		_mouse.x -= locX;
		_mouse.y -= locY;
		_mouse.x = _mouse.x * sizeX / dispSize.lX;
		_mouse.y = _mouse.y * sizeY / dispSize.lY;
		if(_mouse.x < 0 || _mouse.y < 0 || _mouse.x > sizeX || _mouse.y > sizeY) return false;
		for(auto& it: content) {
			bool success = false;
			switch(it.iType) {
				case ITEM_SUBPAGE: {
					success = it.info.subPage->detect(_mouse);
				} break;
				case ITEM_RECTBUTTON: {
					success = it.info.rButton->detect(_mouse);
				} break;
				case ITEM_CIRCBUTTON: {
					success = it.info.cButton->detect(_mouse);
				} break;
				case ITEM_RECTCHKBOX: {
					success = it.info.rChkBox->detect(_mouse);
				} break;
				case ITEM_RECTCHKBOX_WITH_TEXT: {
					success = it.info.cBText->detect(_mouse);
				} break;         // to do
				default: break;  // nothing
			}
			if(success) return success;  // only one.
		}
		return false;  // nothing detected.
	}
	inline pageS& pageS::draw() {
		// printf("IN DRAW FUNCTION.\n");
		delimage(pageImage);
		pageImage = newimage(sizeX, sizeY);
		cleardevice(pageImage);
		setbkcolor(bgColor, pageImage);
		setbkcolor_f(bgColor, pageImage);
		for(auto& it: content) {
			switch(it.iType) {
				case ITEM_SUBPAGE: {
					it.info.subPage->display(pageImage);
				} break;
				case ITEM_VARINTTEXT: {
					it.info.vIText->print(it.locX, it.locY, pageImage);
				} break;
				case ITEM_LINETEXT: {
					it.info.lText->print(it.locX, it.locY, pageImage);
				} break;
				// case ITEM_CONDTEXT: { it.info.cdtnText->print(); } break;
				case ITEM_RECTBUTTON: {
					it.info.rButton->display(pageImage);
				} break;
				case ITEM_CIRCBUTTON: {
					it.info.cButton->display(pageImage);
				} break;
				case ITEM_RECTCHKBOX: {
					it.info.rChkBox->display(pageImage);
				} break;
				case ITEM_RECTCHKBOX_WITH_TEXT: {
					it.info.cBText->display(it.locX, it.locY, pageImage);
				} break;
			}
		}
		return *this;
	}
	inline pageS& pageS::display(PIMAGE pimg) {
		draw();
		putimage(pimg, locX, locY, dispSize.lX, dispSize.lY,
		         pageImage, 0, 0, sizeX, sizeY);
		return *this;
	}

	inline pageS::ctn_t& pageS::gContent() { return content; }
	inline pageS::ctn_t::size_type pageS::cSize() const { return content.size(); }
	inline pageS& pageS::addItem(const pageS::item_t& _item) {
		content.push_back(_item);
		return *this;
	}
	inline pageS& pageS::popItem() {
		content.pop_back();
		return *this;
	}
	inline pageS& pageS::delItem(int _pos) {
		content.erase(content.begin() + _pos);
		return *this;
	}
	inline pageS::item_t& pageS::frontItem() { return content.front(); }
	inline pageS::item_t& pageS::backItem() { return content.back(); }
	inline pageS::item_t& pageS::getItem(int _pos) { return content.at(_pos); }
	inline pageS::ctn_t::iterator pageS::getItemIt(int _pos) { return content.begin() + _pos; }

	inline __attribute__((always_inline)) void pageS::run() {
		for(; is_run();) {
			if(!windowIsVisible()) continue;  // maybe this is not true
			detect();
			// must be something here.
			if(!parentPage) display(parentPage->pageImage);
			else display(NULL);
		}
	}
}  // namespace page

_GLIB_NAMESPACE_TAIL

#endif  // LG_GLIB_PAGE_HPP_
