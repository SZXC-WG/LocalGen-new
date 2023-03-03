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

int LGserver::GAME(){
	std::thread th(sockListen);
	th.detach();
	int plCnt,rbCnt,stDel;
	
	{
		std::lock_guard<std::mutex> mGuard(mLock);
		plCnt=totSock-1;
	}
	
	LGgame::cheatCode=1048575;
	LGgame::playerCnt=plCnt+rbCnt;
	LGgame::stepDelay=stDel;
	LGgame::gameMesC = 0;
	
	int robotId[64];
	std::deque<int> movement;
	std::mt19937 mtrd(std::chrono::system_clock::now().time_since_epoch().count());

	for(int i = plCnt+1; i <= plCnt+rbCnt; ++i)
		robotId[i] = mtrd() % 300 + 1;

	LGgame::initGenerals(LGgame::playerCoo);
	LGgame::updateMap();
	LGgame::curTurn = 0;
	bool gameEnd = 0;

	for(; is_run(); delay_fps(LGgame::stepDelay)) {
		LGgame::updateMap();
		sockCollect();
		
		if(!movement.empty())
		movement.pop_front();

		for(int i = plCnt+1; i <= plCnt+rbCnt; ++i) {
			if(!LGgame::isAlive[i])
				continue;
			switch(robotId[i]) {
				case 1 ... 100:
					LGgame::analyzeMove(i, smartRandomBot::smartRandomBot(i, LGgame::playerCoo[i]), LGgame::playerCoo[i]);
					break;
				case 101 ... 200:
					LGgame::analyzeMove(i, xrzBot::xrzBot(i, LGgame::playerCoo[i]), LGgame::playerCoo[i]);
					break;
				case 201 ... 300:
					LGgame::analyzeMove(i, xiaruizeBot::xiaruizeBot(i, LGgame::playerCoo[i]), LGgame::playerCoo[i]);
					break;
				default:
					LGgame::analyzeMove(i, 0, LGgame::playerCoo[i]);
			}
		}

		LGgame::flushMove();
		zipSendBuf();
		sockBroadcast();

		if(LGgame::cheatCode != 1048575) {
			int alldead = 0;
			for(int i = 1; i <= LGgame::playerCnt && !alldead; ++i) {
				if(LGgame::cheatCode & (1 << i))
					if(LGgame::isAlive[i])
						alldead = 1;
			}
			if(!alldead) {
				LGgame::cheatCode = 1048575;
				MessageBox(nullptr, "ALL THE PLAYERS YOU SELECTED TO BE SEEN IS DEAD.\nTHE OVERALL CHEAT MODE WILL BE SWITCHED ON.", "TIP", MB_OK | MB_SYSTEMMODAL);
			}
		}
		if(!gameEnd) {
			int ed = 0;
			for(int i = 1; i <= LGgame::playerCnt; ++i)
				ed |= (LGgame::isAlive[i] << i);
			if(__builtin_popcount(ed) == 1) {
				MessageBox(nullptr,
				           ("PLAYER " + playerInfo[std::__lg(ed)].name + " WON!" + "\n" +
				            "THE game WILL CONTINUE." + "\n" +
				            "YOU CAN PRESS [ESC] TO EXIT.")
				           .c_str(),
				           "game END", MB_OK | MB_SYSTEMMODAL);
				gameEnd = 1;
				LGgame::cheatCode = 1048575;
				zipSendBuf();
				sockBroadcast();
			}
		}
	}
	
	sendBuf[0]=43;
	sendBuf[1]=48;
	sendBuf[2]='\0';
	sockBroadcast();
	std::lock_guard<std::mutex> mGuard(mLock);
	
	for(int i=1;i<totSock;i++)
	closesocket(serverSocket[i]);
	
	return 0;
}

void LGclient::sockConnect(){
	if(initSock())
	return ;
	
	clientSocket=socket(AF_INET,SOCK_STREAM,0);
	SOCKADDR_IN connectAddr;
	connectAddr.sin_family=AF_INET;
	connectAddr.sin_addr.S_un.S_addr=inet_addr("192.168.32.35");
	connectAddr.sin_port=htons(SKPORT);
	int res=connect(clientSocket,(LPSOCKADDR)&connectAddr,sizeof(connectAddr));
	u_long iMode=1;
	ioctlsocket(clientSocket,FIONBIO,&iMode);
	
	if(res==SOCKET_ERROR){
		failSock|=4;
		WSACleanup();
		return ;
	}return ;
}

void LGclient::sockMessage(){
	std::lock_guard<std::mutex> mGuard(mLock);
	send(clientSocket,sendBuf,sizeof(sendBuf),0);
	memset(sendBuf,0,sizeof(sendBuf));
}

void LGclient::procMessage() {
	if(recvBuf[0]==43) {
		if(recvBuf[1]==CHAR_AD){
			failSock=5;
			exitExe();
		}else playerNumber=recvBuf[1]-CHAR_AD;
	} else {

	}
}

void LGclient::sockCollect(){
	std::lock_guard<std::mutex> mGuard(mLock);
	int res=recv(clientSocket,recvBuf,sizeof(recvBuf),0);
	
	if(res>0) procMessage();
		
	memset(recvBuf,0,sizeof(recvBuf));
}

int LGclient::GAME(){
}

#endif
