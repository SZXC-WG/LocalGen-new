/* This is the MAIN program of LocalGen.                                 */
/* Copyright (c) 2023 LocalGen-dev; All rights reserved.                 */
/* Developers: http://github.com/LocalGen-dev                            */
/* Project: http://github.com/LocalGen-dev/Local-Generals.io             */
/*                                                                       */
/* This project is licensed under the MIT license. That means you can    */
/* download, use and share a copy of the product of this project. You    */
/* may modify the source code and make contribution to it too. But, you  */
/* must print the copyright information at the front of your product.    */
/*                                                                       */
/* The full MIT license this project uses can be found here:             */
/* http://github.com/LocalGen-dev/Local-Generals.io/blob/main/LICENSE.md */

#ifndef __LGMAIN_CPP__
#define __LGMAIN_CPP__

#if __cplusplus < 201300L
#error\
 This program should be compiled under the C++14\
 standard. If you uses the C++ standard under it,\
 please uses a standard greater. In G++, you can\
 compile with option -std=c++14 or -std=gnu++14 to\
 use the C++14 standard.
#endif // < C++14

/*************** header files ***************/

/* map structs */
#include "LGmaps.hpp" // __LGMAPS_HPP__
/* map zipping */
#include "LGzipmap.hpp" // __LGZIPMAP_HPP__
/* default maps */
#include "LGdefmap.hpp" // __LGDEFMAP_HPP__
/* printings */
#include "LGprint.hpp" // __LGPRINT_HPP__
/* Internet base */
#include "LGweb.hpp" // __LGWEB_HPP__
/* game options */
#include "LGgame.hpp" // __LGGAME_HPP__
/* pages */
#include "LGpages.hpp" // __LGPAGES_HPP__

/*************** the main function ***************/

signed main(signed argc, char** argv) {
	MainPage(); // start main page 
	exit(0);    // exit program
}

#endif // __LGMAIN_CPP__ 
