/* This is GLIB_rectcbox.hpp file of SZXC-WG EGE graphics lib.           */
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

#ifndef __LG_GLIB_RECTCBOX_HPP__
#define __LG_GLIB_RECTCBOX_HPP__

#include "GLIB_HEAD.hpp"

_GLIB_NAMESPACE_HEAD

inline namespace checkbox {

	class rectCBOX {
	  private:
		PIMAGE boxImage;
		int width, height;
		int locationX, locationY;
		color_t backgroundColor;
		color_t frameColor;
		int frameWidth;
		color_t fillColor;
		bool pressed;
		int status; /* button status: free(0) / cursor-on(1) / clicked(2) */
		std::function<void()> clickEvent;
	  public:
		explicit rectCBOX() {
			boxImage = newimage();
			width = height = 1;
			frameColor = fillColor = 0xffffffff;
			status = 0; pressed = 0;
		};
		~rectCBOX() { delimage(boxImage); }
		inline rectCBOX& draw() {}
		inline rectCBOX& display() {}
		inline rectCBOX& size(int _width, int _height) {}
		inline rectCBOX& bgcolor(color_t _color) {}
		inline rectCBOX& move(int _X, int _Y) {}
		inline rectCBOX& event(decltype(clickEvent) _event) { clickEvent = _event; }
		inline rectCBOX& frame(int _width) {}
		inline rectCBOX& framecolor(bool _enableAuto = 1, color_t _color = 0xffffffff) {}
		inline rectCBOX& detect() {}
	};

}

_GLIB_NAMESPACE_TAIL

#endif // __LG_GLIB_RECTCBOX_HPP__
