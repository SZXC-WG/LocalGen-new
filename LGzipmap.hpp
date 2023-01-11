/* This is LGzipmap.hpp file of LocalGen.                                */
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

#ifndef __LGZIPMAP_HPP__
#define __LGZIPMAP_HPP__

#include "LGmaps.hpp"

const int LEN_ZIP=100005,CHAR_AD=48;
char strdeZip[LEN_ZIP];
char strZip[LEN_ZIP];

inline long long PMod(long long &x){
	long long res=x&63;
	x>>=6;return res;
}

void Zip(){
	register int p=0,i,j;
	long long k1=mapH,k2=mapW;
	strZip[p++]=PMod(k1)+CHAR_AD;strZip[p++]=PMod(k1)+CHAR_AD;
	strZip[p++]=PMod(k2)+CHAR_AD;strZip[p++]=PMod(k2)+CHAR_AD;
	
	for(i=1;i<=mapH;i++) for(j=1;j<=mapW;j++,p++){
		strZip[p++]=((gameMap[i][j].team<<3)+gameMap[i][j].team)+CHAR_AD;
		k1=gameMap[i][j].army;
		
		if(k1<0){
			k1=-k1;
			strZip[p+11]=CHAR_AD+1;
		}else strZip[p+11]=CHAR_AD;
		
		for(k2=1;k2<=11;k2++)
		strZip[p++]=PMod(k1)+CHAR_AD;
	}strZip[p]='\0';
}

void deZip(){
	register int i,j,k=4;
	int f,p=0;
	
	for(;strdeZip[p]!='\0';p++) strdeZip[p]-=CHAR_AD;
	
	mapH=(strdeZip[1]<<6)+strdeZip[0];
	mapW=(strdeZip[3]<<6)+strdeZip[2];
	
	for(i=1;i<=mapH;i++) for(j=1;j<=mapW;j++){
		gameMap[i][j].type=strdeZip[k]&7;strdeZip[k]>>=3;
		gameMap[i][j].team=strdeZip[k++];
		gameMap[i][j].army=0;
		
		for(p=10;p>=0;p--) gameMap[i][j].army=(gameMap[i][j].army<<6)+strdeZip[k+p];
		k+=11;
		gameMap[i][j].army=strdeZip[k++]?(-gameMap[i][j].army):gameMap[i][j].army;
	}
}

#endif
