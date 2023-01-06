/* This is LGmaps.hpp file of LocalGen.                                  */
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

#ifndef __LGMAPS_HPP__
#define __LGMAPS_HPP__

#define ll long long

struct Block {
	int team; /* the team who holds this block */
	int type; /* the block's type: 0->general, 1->plain, 2->mountain, 3->city, 4->swamp */
	ll army; /* count of army on this block */
};

Block gameMap[1005][1005]; /* maximum 1000*1000 */

struct teamS {
	string name;
	int color;
};
teamS defTeams[64] = {
	{"Black"s,0xffffff},
	{"Red"s,0xff0000},
	{"Green"s,0x00ff00},
	{"Blue"s,0x0000ff},
	{"Yellow"s,0xffff00},
	{"Cyan"s,0x00ffff},
	{"Rose"s,0xff00ff},
	{"Orange"s,0xff8000},
	{"Lime"s,0x80ff00},
	{"Brown"s,0x804000},
	{"Grey"s,0x808080},
	{"Teal"s,0x008080},
	{"Purple"s,0xc000c0},
	{"Silver"s,0xc0c0c0},
	{"Maroon"s,0xc00000},
	{"Emerald"s,0x00ce80},
	{"Olive"s,0x808000},
};

void printMap(bool cht) {
}

#undef ll // long long

#endif // __LGMAPS_HPP

