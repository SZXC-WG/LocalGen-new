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

const int advertisementCnt=2,advertisementChg=1000;
char advertisementStr[2][205]={\
"Movements: You can use 'W''A''S''D' to move, 'G' for home, 'Q' for surrander,and ESC to quit!                  ",\
"Thanks to ZLY, CHR, XRZ and KTQ for devoting their hair to this game!                                          "};

void gameAdvertisement(){
	static int chs=-1;
	static std::chrono::nanoseconds lPT=std::chrono::steady_clock::now().time_since_epoch();
	
	if(chs!=-1&&std::chrono::steady_clock::now().time_since_epoch()-lPT<std::chrono::milliseconds(advertisementChg))
	printf("%s",advertisementStr[chs]);
	else{
		std::mt19937 p(std::chrono::system_clock::now().time_since_epoch().count());
		chs=p()%advertisementCnt;
		printf("%s",advertisementStr[chs]);
		lPT=std::chrono::steady_clock::now().time_since_epoch();
	}return ;
}

#endif
