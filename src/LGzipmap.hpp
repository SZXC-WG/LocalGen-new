/* This is LGzipmap.hpp file of LocalGen.                                */
/* Copyright (c) 2024 SZXC Work Group; All rights reserved.              */
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

#ifndef LGZIPMAP_HPP_
#define LGZIPMAP_HPP_

#include "LGdef.hpp"

inline long long PMod(long long& x) {
    long long res = x & 63;
    x >>= 6;
    return res;
}

void Zip() {
    int p = 0, i, j;
    long long k1 = mapH, k2 = mapW;
    strZip[p++] = PMod(k1) + CHAR_AD;
    strZip[p++] = PMod(k1) + CHAR_AD;
    strZip[p++] = PMod(k2) + CHAR_AD;
    strZip[p++] = PMod(k2) + CHAR_AD;

    for (i = 1; i <= mapH; i++)
        for (j = 1; j <= mapW; j++) {
            strZip[p++] = gameMap[i][j].player + CHAR_AD;
            strZip[p] = (gameMap[i][j].type << 2) + (gameMap[i][j].lit << 1);
            k1 = gameMap[i][j].army;

            if (k1 < 0) {
                k1 = -k1;
                strZip[p++] += CHAR_AD + 1;
            } else
                strZip[p++] += CHAR_AD;

            for (k2 = 1; k2 <= 8; k2++) strZip[p++] = PMod(k1) + CHAR_AD;
        }
    strZip[p] = '\0';
}

void deZip() {
    int i, j, k = 4;
    int f, p = 0;

    for (; strdeZip[p] != '\0'; p++) strdeZip[p] -= CHAR_AD;

    mapH = (strdeZip[1] << 6) + strdeZip[0];
    mapW = (strdeZip[3] << 6) + strdeZip[2];

    for (i = 1; i <= mapH; i++)
        for (j = 1; j <= mapW; j++) {
            gameMap[i][j].player = strdeZip[k++];
            bool f = strdeZip[k] & 1;
            strdeZip[k] >>= 1;
            gameMap[i][j].lit = strdeZip[k] & 1;
            strdeZip[k] >>= 1;
            gameMap[i][j].type = strdeZip[k++];
            gameMap[i][j].army = 0;

            for (p = 7; p >= 0; p--)
                gameMap[i][j].army =
                    (gameMap[i][j].army << 6) + strdeZip[k + p];
            k += 8;
            gameMap[i][j].army = f ? (-gameMap[i][j].army) : gameMap[i][j].army;
        }
}

#endif  // LGZIPMAP_HPP_
