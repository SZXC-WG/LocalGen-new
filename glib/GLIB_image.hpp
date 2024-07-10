/* This is GLIB_image.hpp file of SZXC-WG graphics lib.                  */
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

#ifndef __LG_GLIB_IMAGE_HPP__
#define __LG_GLIB_IMAGE_HPP__

#include "GLIB_HEAD.hpp"

_GLIB_NAMESPACE_HEAD

namespace images {

	void copyImage(PIMAGE& dstimg, PIMAGE& srcimg) {
		if(dstimg == NULL || srcimg == NULL) return;
		getimage(dstimg, srcimg, 0, 0, getwidth(srcimg), getheight(srcimg));
	}
	void zoomImage(PIMAGE& pimg, int zoomWidth, int zoomHeight) {
		if((pimg == NULL) || (zoomWidth == getwidth(pimg) && zoomHeight == getheight(pimg)))
			return;

		PIMAGE zoomImage = newimage(zoomWidth, zoomHeight);
		putimage(zoomImage, 0, 0, zoomWidth, zoomHeight, pimg, 0, 0, getwidth(pimg), getheight(pimg));
		delimage(pimg);

		pimg = zoomImage;
	}
	void setWindowTransparent(bool enable, int alpha) {
		HWND egeHwnd = getHWnd();
		LONG nRet = ::GetWindowLong(egeHwnd, GWL_EXSTYLE);
		nRet |= WS_EX_LAYERED;
		::SetWindowLong(egeHwnd, GWL_EXSTYLE, nRet);
		if(!enable)
			alpha = 0xFF;
		SetLayeredWindowAttributes(egeHwnd, 0, alpha, LWA_ALPHA);
	}

}  // namespace images

_GLIB_NAMESPACE_TAIL

#endif  // __LG_GLIB_IMAGE_HPP__
