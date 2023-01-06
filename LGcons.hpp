/* This is LGcons.hpp file of LocalGen.                                  */
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

#ifndef __LGCONS_HPP__
#define __LGCONS_HPP__ 

#include <cstdio>
#include <windows.h>
#include <conio.h>

// clear the full window: too slow, don't use!
inline void clearance() { fputs("\033[2J",stdout); }
// clear the line (only the chars after the cursor)
inline void clearline() { fputs("\033[K",stdout); }

// make the cursor go to a specticular place in the screen
inline void gotoxy(int x,int y) { printf("\033[%d:%dH",y,x); }
// make the cursor move n lines up
inline void curup(int c=1) { printf("\033[%dA",c); }
// make the cursor move n lines down
inline void curdown(int c=1) { printf("\033[%dB",c); }
// make the cursor move n chars right
inline void curright(int c=1) { printf("\033[%dC",c); }
// make the cursor move n chars left
inline void curleft(int c=1) { printf("\033[%dD",c); }

// hide cursor
inline void hideCursor() { fputs("\033[?25l",stdout); }
// show cursor
inline void showCursor() { fputs("\033[?25h",stdout); }

// attr init: without this will lead to errors
inline int initattr() {
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return GetLastError();
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) return GetLastError();
    dwMode |= 0x0004;
    if (!SetConsoleMode(hOut, dwMode)) return GetLastError();
    return 0;
}

// foreground color
inline void setfcolor(int RGB) { printf("\033[38;2;%d;%d;%dm",RGB/65536,RGB/256%256,RGB%256); }
inline void setfcolor(int R,int G,int B) { printf("\033[38;2;%d;%d;%dm",R,G,B); }
// background color
inline void setbcolor(int RGB) { printf("\033[48;2;%d;%d;%dm",RGB/65536,RGB/256%256,RGB%256); }
inline void setbcolor(int R,int G,int B) { printf("\033[48;2;%d;%d;%dm",R,G,B); }

// print chars with underline
inline void underline() { fputs("\033[4m",stdout); }

// reset text attributes
inline void resetattr() { fputs("\033[0m",stdout); }

#endif // __LGCONS_HPP__

