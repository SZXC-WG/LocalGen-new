/* This is the MAIN program of LocalGen.                                 */
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

#ifndef __LGMAIN_CPP__
#define __LGMAIN_CPP__

#if __cplusplus < 201300L
	#error This program should be compiled under the C++14\
	standard. If you use the C++ standard under it,\
	please use a standard greater. In G++, you can\
	compile with option -std=c++14 or -std=gnu++14 to\
	use the C++14 standard.
#endif // < C++14

/*************** header files ***************/

/* dev project headers */
#include "LocalGen_private.h"
#include "LGdef.hpp"
#include "glib/GLIB_HEAD.hpp"
#include "glib/GLIB_image.hpp"
#include "glib/GLIB_rectbut.hpp"
#include "glib/GLIB_circbut.hpp"
#include "glib/GLIB_rectcbox.hpp"
#include "LGencoding.hpp"
#include "LGset.hpp"
#include "LGmaps.hpp"
#include "LGgraphics.hpp"
#include "LGbot.hpp"
#include "LGzipmap.hpp"
#include "LGgame.hpp"
#include "LGweb.hpp"
#include "LGreplay.hpp"

/*************** the main function ***************/
signed main(signed argc, char** argv) {
	FreeConsole();
	initMaps();
	LGset::read();
	LGset::write();
	MainPage(); // start main page
	exitExe();	// exit program
}

#endif // __LGMAIN_CPP__
