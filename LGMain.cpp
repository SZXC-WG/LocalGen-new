/* This is the MAIN program of LocalGen.                                 */
/* Copyright (c) 2023 LocalGen-dev; All rights reserved.                 */
/* Developers: http://github.com/LocalGen-dev                            */
/* Project: http://github.com/LocalGen-dev/LocalGen-new                  */
/*                                                                       */
/* This project is licensed under the MIT license. That means you can    */
/* download, use and share a copy of the product of this project. You    */
/* may modify the source code and make contribution to it too. But, you  */
/* must print the copyright information at the front of your product.    */
/*                                                                       */
/* The full MIT license this project uses can be found here:             */
/* http://github.com/LocalGen-dev/LocalGen-new/blob/main/LICENSE.md      */

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
#include "LocalGen-new_private.h"
/* pages */
#include "LGpages.hpp" // __LGPAGES_HPP__

/*************** the main function ***************/
HWND hwnd = GetConsoleWindow();
signed main(signed argc, char **argv)
{
	ShowWindow(hwnd, SW_MAXIMIZE);
	SetConsoleTitle("Localized Generals.io v" FILE_VERSION " (Set your console window font size! Recommended:5-8)");
	initMaps();
	MainPage(); // start main page
	exitExe();	// exit program
}

#endif // __LGMAIN_CPP__
