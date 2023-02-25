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

bool initSock() {
	WORD w_req=MAKEWORD(2,2);
	WSADATA wsadata;
	int err=WSAStartup(w_req,&wsadata);

	if(err!=0) failSock|=1;
	if(LOBYTE(wsadata.wVersion)!=2||HIBYTE(wsadata.wHighVersion)!=2) {
		failSock|=2;
		WSACleanup();
		return true;
	} return false;
}

void LGserver::zipSendBuf() {
	register int p=0,i,j;
	long long k1=mapH,k2=mapW;
	sendBuf[p++]=44;
	sendBuf[p++]=LGgame::playerCnt;

	for(i=1; i<=LGgame::playerCnt; i++) {
		sendBuf[p++]=LGgame::playerCoo[i].x;
		sendBuf[p++]=LGgame::playerCoo[i].y;
	}

	sendBuf[p++]=PMod(k1)+CHAR_AD;
	sendBuf[p++]=PMod(k1)+CHAR_AD;
	sendBuf[p++]=PMod(k2)+CHAR_AD;
	sendBuf[p++]=PMod(k2)+CHAR_AD;

	for(i=1; i<=mapH; i++)
	for(j=1; j<=mapW; j++) {
		sendBuf[p++]=gameMap[i][j].team+CHAR_AD;
		sendBuf[p]=(gameMap[i][j].type<<2)+(gameMap[i][j].lit<<1);
		k1=gameMap[i][j].army;

		if(k1<0) {
			k1=-k1;
			sendBuf[p++]+=CHAR_AD+1;
		} else sendBuf[p++]+=CHAR_AD;

		for(k2=1; k2<=8; k2++)
			sendBuf[p++]=PMod(k1)+CHAR_AD;
	} sendBuf[p]='\0';
}

void LGserver::sockListen() {
	if(initSock())
		return ;

	SOCKET listenSocket=socket(AF_INET,SOCK_STREAM,0);
	SOCKADDR_IN listenAddr;
	listenAddr.sin_family=AF_INET;
	listenAddr.sin_addr.S_un.S_addr=INADDR_ANY;
	listenAddr.sin_port=htons(SKPORT);
	int res=bind(listenSocket,(LPSOCKADDR)&listenAddr,sizeof(listenAddr));

	if(res==SOCKET_ERROR) {
		failSock|=4;
		WSACleanup();
		return ;
	}

	u_long iMode=1;
	int lis=listen(listenSocket,20);

	while(1) {
		SOCKET* clientSocket;

		{
			std::lock_guard<std::mutex> mGuard(mLock);
			totSock++;

			if(lisEnd)
				break;

			clientSocket=&serverSocket[totSock];
		}

		*clientSocket=accept(listenSocket,0,0);
		ioctlsocket(*clientSocket,FIONBIO,&iMode);

		{
			std::lock_guard<std::mutex> mGuard(mLock);

			if(lisEnd)
				break;

			sockCon[totSock]=true;
		}
	}

	closesocket(listenSocket);
	return ;
}

void LGserver::procMessage(int sockID) {
	if(recvBuf[0]==43) {
		if(recvBuf[1]==CHAR_AD)
		sockCon[sockID]=false;
		else {
			if(lisEnd) sendBuf[1]=CHAR_AD;
			else sendBuf[1]=sockID+CHAR_AD;

			sendBuf[0]=43;
			sendBuf[2]='\0';
			send(serverSocket[sockID],sendBuf,sizeof(sendBuf),0);
		}
	} else {

	}
}

void LGserver::sockBroadcast() {
	std::lock_guard<std::mutex> mGuard(mLock);

	for(int i=1; i<totSock; i++) if(sockCon[i])
	send(serverSocket[i],sendBuf,sizeof(sendBuf),0);

	memset(sendBuf,0,sizeof(sendBuf));
}

void LGserver::sockCollect() {
	std::lock_guard<std::mutex> mGuard(mLock);

	for(int i=1; i<totSock; i++) if(sockCon[i]) {
		int res=recv(serverSocket[i],recvBuf,sizeof(recvBuf),0);
		if(res>0) procMessage(i);
		memset(recvBuf,0,sizeof(recvBuf));
	}
}



#endif
