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

#ifndef __LG_GLIB_PAGE_HPP__
#define __LG_GLIB_PAGE_HPP__

#include "GLIB_HEAD.hpp"

_GLIB_NAMESPACE_HEAD

inline namespace page {

	inline void scrBarS::draw() {} // todo))
	inline void scrBarS::display() {} // todo))

	inline pageS& pageS::size(int _sizeX, int _sizeY) {
		sizeX = _sizeX; sizeY = _sizeY; return *this;
	}
	inline pageS& pageS::width(int _sizeX) {
		sizeX = _sizeX; return *this;
	}
	inline pageS& pageS::height(int _sizeY) {
		sizeY = _sizeY; return *this;
	}
	inline pageS& pageS::move(int _locX, int _locY) {
		locX = _locX; locY = _locY; return *this;
	}
	inline pageS& pageS::setBgColor(color_t _color) {
		bgColor = _color; return *this;
	}
	inline pageS& pageS::dSize(int _sizeX, int _sizeY) {
		dispSize = {_sizeX,_sizeY}; return *this;
	}
	inline pageS& pageS::dWidth(int _sizeX) {
		dispSize.lX = _sizeX; return *this;
	}
	inline pageS& pageS::dHeight(int _sizeY) {
		dispSize.lY = _sizeY; return *this;
	}

	inline pageS& pageS::detect() {
		for(auto& it:content) {
			switch(it.iType) {
				case ITEM_SUBPAGE: { it.info.subPage->detect(); } break;
				case ITEM_RECTBUTTON: { it.info.rButton->detect(); } break;
				case ITEM_CIRCBUTTON: { it.info.cButton->detect(); } break;
				case ITEM_RECTCHKBOX: { it.info.rChkBox->detect(); } break;
				case ITEM_RECTCHKBOX_WITH_TEXT: { it.info.cBText->detect(); } break; // to do
				default: break; // nothing
			}
		}
	}
	inline pageS& pageS::draw() {
		printf("IN DRAW FUNCTION.\n");
		delimage(pageImage);
		pageImage = newimage(sizeX,sizeY);
		cleardevice(pageImage);
		setbkcolor(bgColor,pageImage);
		setbkcolor_f(bgColor,pageImage);
		for(auto& it:content) {
			printf("FETCHING AN ITEM...\n");
			switch(it.iType) {
				case ITEM_SUBPAGE: { it.info.subPage->display(pageImage); } printf("ITEM DRAWED: SUBPAGE\n"); break;
				case ITEM_LINETEXT: { it.info.lText->print(it.locX,it.locY,pageImage); } printf("ITEM DRAWED: LINETEXT (0) %ls\n",it.info.lText->text[0].text.c_str()); break;
				// case ITEM_CONDTEXT: { it.info.cdtnText->print(); } break;
				case ITEM_RECTBUTTON: { it.info.rButton->display(pageImage); } printf("ITEM DRAWED: RECTBUTTON\n"); break;
				case ITEM_CIRCBUTTON: { it.info.cButton->display(pageImage); } printf("ITEM DRAWED: CIRCBUTTON\n"); break;
				case ITEM_RECTCHKBOX: { it.info.rChkBox->display(pageImage); } printf("ITEM DRAWED: RECTCHKBOX\n"); break;
				case ITEM_RECTCHKBOX_WITH_TEXT: { it.info.cBText->display(it.locX,it.locY,pageImage); } printf("ITEM DRAWED: RCBOXTEXT\n"); break;
			}
		}
	}
	inline pageS& pageS::display(PIMAGE pimg) {
		printf("DISPLAYING... PAGE DRAW STARTS.\n");
		draw();
		printf("PAGE DRAW ENDS.\n");
		putimage(pimg, locX, locY, dispSize.lX, dispSize.lY,
		         pageImage, 0, 0, sizeX, sizeY);
		printf("PAGE DISPLAYED.\n");
	}

	inline pageS& pageS::addItem(const pageS::item_t& _item) { content.push_back(_item); return *this; }
	inline pageS& pageS::popItem() { content.pop_back(); return *this; }
	inline pageS& pageS::delItem(int _pos) { content.erase(content.begin()+_pos); return *this; }
	inline pageS::item_t& pageS::frontItem() { return content.front(); }
	inline pageS::item_t& pageS::backItem() { return content.back(); }
	inline pageS::item_t& pageS::getItem(int _pos) { return content.at(_pos); }
	inline pageS::ctn_t::iterator pageS::getItemIt(int _pos) { return content.begin()+_pos; }

	inline __attribute__((always_inline)) void pageS::run() {
		for(; is_run();) {
			if(!windowIsVisible()) continue; // maybe this is not true
			detect();
			// must be something here.
			if(!parentPage) display(parentPage->pageImage);
			else display(NULL);
		}
	}
}

_GLIB_NAMESPACE_TAIL

#endif // __LG_GLIB_PAGE_HPP__
