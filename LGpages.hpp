/* This is LGpages.hpp file of LocalGen.                                  */
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

#ifndef __LGPAGES_HPP__
#define __LGPAGES_HPP__

#include <random>
#include <chrono>
#include "LGcons.hpp"
#include "LGmaps.hpp"
#include "LGzipmap.hpp"
#include "LGgame.hpp"

void MainPage()
{
	std::mt19937 mtrd(std::chrono::system_clock::now().time_since_epoch().count());
	LGGraphics::WelcomePage();
	LGGraphics::selectOrImportMap();
	return;
}

#endif
