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

	inline void scrBarS::draw() {} // to do
	inline void scrBarS::display() {} // to do

	inline pageS& pageS::detect() {
		for(auto& it:content) {
			switch(it.iType) {
				case 0: { it.info.subPage->detect(); } break;
				case 3: { it.info.rButton->detect(); } break;
				case 4: { it.info.cButton->detect(); } break;
				case 5: { it.info.rChkBox->detect(); } break;
			}
		}
	}
	inline pageS& pageS::draw() {
		for(auto& it:content) {
			switch(it.iType) {
				case 0: { it.info.subPage->display(pageImage); } break;
				case 1: { it.info.lText->print(it.locX,it.locY,pageImage); } break;
				// case 2: { it.info.cdtnText->print(); } break;
				case 3: { it.info.rButton->display(pageImage); } break;
				case 4: { it.info.cButton->display(pageImage); } break;
				case 5: { it.info.rChkBox->display(pageImage); } break;
				case 6: { it.info.cBText; } break; // to do
			}
		}
	}
	inline pageS& pageS::display(PIMAGE pimg) {
		draw();
		putimage(pimg, locX, locY, dispSize.lX, dispSize.lY,
		         pageImage, 0, 0, sizeX, sizeY);
	}

}

_GLIB_NAMESPACE_TAIL

#endif // __LG_GLIB_PAGE_HPP__
