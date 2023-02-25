/* This is the LGlib.hpp file of LocalGen.                               */
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

#ifndef __LGLIB_HPP__
#define __LGLIB_HPP__

#include <stdio.h>
#include <random>
#include <time.h>
#include <chrono>

#include "LGcons.hpp"

const int advertisementCnt=7,advertisementChg=4000;
char advertisementStr[advertisementCnt+5][205] = {
	"Press 'E' can cancel your last un-processed move.",
	"Press 'Q' can erase all your un-processed moves.",
	"Press WASD to move your aim!",
	"Press [ESC] can make you exit the game.",
	"Press [BACKSPACE] to surrender.",
	"Thanks to ZLY, CHR, XRZ and JYL for devoting their hair to this game!",
	"This game is going to have web function, but CHR is a bit confused...",
};

void gameAdvertisement(){
	static int chs=-1;
	static std::chrono::nanoseconds lPT=std::chrono::steady_clock::now().time_since_epoch();
	static std::mt19937 p(std::chrono::system_clock::now().time_since_epoch().count());
	
	if(chs!=-1&&std::chrono::steady_clock::now().time_since_epoch()-lPT<std::chrono::milliseconds(advertisementChg)) 
		printf("%s",advertisementStr[chs]),clearline();
	else {
		chs=p()%advertisementCnt;
		printf("%s",advertisementStr[chs]),clearline();
		lPT=std::chrono::steady_clock::now().time_since_epoch();
	} 
	return ;
}

/*
44 :SIG1(MAP)
45 :SIG2(START)
46 :SIG3(CMD)
47 :SIG4(END)
48 :CHAR_AD
36 $ :start and end of the text

.lgmp :map file
.lgrep :replay file
*/

#endif
