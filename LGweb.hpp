/* This is LGweb.hpp file of LocalGen.                                  */
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

#ifndef __LGWEB_HPP__
#define __LGWEB_HPP__

#include <stdio.h>
#include <mutex>
#include <queue>
#include <thread>
#include <winsock2.h>
const int SSN=205,SSL=100005;
const int SKPORT=14514;

int failSock;

bool initSock() {
	WORD w_req=MAKEWORD(2,2);
	WSADATA wsadata;
	int err=WSAStartup(w_req,&wsadata);

	if(err!=0) failSock|=1;
	if(LOBYTE(wsadata.wVersion)!=2||HIBYTE(wsadata.wHighVersion)!=2) {
		failSock|=2;
		WSACleanup();
		return true;
	}return false;
}

#endif
