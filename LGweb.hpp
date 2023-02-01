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

<<<<<<< Updated upstream
int failSock;
=======
int failSock,ky;
std::mutex mLock;
bool stopSock,usedStr[SSN],lisEnd;
char *sockStr[SSN][SSL];
std::queue<int> strSend,strRecv;
>>>>>>> Stashed changes

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

<<<<<<< Updated upstream
=======
void pushSendPack(){
	std::lock_guard<std::mutex> mGuard(mLock);
	strSend.push(ky);
	usedStr[ky]=true;
}

void pushRecvPack(){
	std::lock_guard<std::mutex> mGuard(mLock);
	strRecv.push(ky);
	usedStr[ky]=true;
}

int pullSendPack(){
	std::lock_guard<std::mutex> mGuard(mLock);
	int res=strSend.front();
	strSend.pop();
	usedStr[res]=false;
	ky=ky>res?res:ky;
	return res;
}

int pullRecvPack(){
	std::lock_guard<std::mutex> mGuard(mLock);
	int res=strRecv.front();
	strRecv.pop();
	usedStr[res]=false;
	ky=ky>res?res:ky;
	return res;
}

void serverThread(SOCKET* socketServer){
	
}

void clientThread(SOCKET* socketClient){
	
}

bool sockListen(){
	if(initSock())
	return true;
	
	SOCKET listenSocket=socket(AF_INET,SOCK_STREAM,0);
	SOCKADDR_IN listenAddr;
	listenAddr.sin_family=AF_INET;
	listenAddr.sin_addr.S_un.S_addr=INADDR_ANY;
	listenAddr.sin_port=htons(SKPORT);
	int res=bind(listenSocket,(LPSOCKADDR)&listenAddr,sizeof(listenAddr));
	
	if(res==SOCKET_ERROR){
		WSACleanup();
		return true;
	}
	
	int lis=listen(listenSocket,20);
	
	while(1){
		{
			std::lock_guard<std::mutex> mGuard(mLock);
			
			if(lisEnd)
			break;
		}
		
		SOCKET *clientSocket=new SOCKET;
		int sockAddrLen=sizeof(sockaddr);
		*clientSocket=accept(listenSocket,0,0);
	//	thread th();
		
	}
	
	while(1){
		Sleep(500);
		std::lock_guard<std::mutex> mGuard(mLock);
		
		if(stopSock)
		break;
	}
	
	closesocket(listenSocket);
	WSACleanup();
	return false;
}

>>>>>>> Stashed changes
#endif
