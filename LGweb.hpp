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

#include "LGdef.hpp"

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
	long long k1=LGgame::curTurn,k2;
	sendBuf[p++]=44;
	sendBuf[p++]=LGgame::playerCnt+CHAR_AD;
	sendBuf[p++]=LGgame::gameSpeed+CHAR_AD;
	sendBuf[p++]=PMod(k1)+CHAR_AD;
	sendBuf[p++]=PMod(k1)+CHAR_AD;
	sendBuf[p++]=PMod(k1)+CHAR_AD;
	sendBuf[p++]=PMod(k1)+CHAR_AD;

	for(i=1; i<=LGgame::playerCnt; i++) {
		sendBuf[p++]=LGgame::isAlive[i]+CHAR_AD;
		sendBuf[p++]=LGgame::playerCoo[i].x+CHAR_AD;
		sendBuf[p++]=LGgame::playerCoo[i].y+CHAR_AD;
	}

	k1=mapH,k2=mapW;
	sendBuf[p++]=PMod(k1)+CHAR_AD;
	sendBuf[p++]=PMod(k1)+CHAR_AD;
	sendBuf[p++]=PMod(k2)+CHAR_AD;
	sendBuf[p++]=PMod(k2)+CHAR_AD;

	for(i=1; i<=mapH; i++)
		for(j=1; j<=mapW; j++) {
			sendBuf[p++]=gameMap[i][j].player+CHAR_AD;
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
		if(recvBuf[1]==CHAR_AD) {
			sockCon[sockID]=false;
			LGgame::isAlive[sockID]=false;
		} else {
			if(lisEnd) sendBuf[1]=CHAR_AD;
			else sendBuf[1]=sockID+CHAR_AD;

			sendBuf[0]=43;
			sendBuf[2]='\0';
			send(serverSocket[sockID],sendBuf,sizeof(sendBuf),0);
			LGgame::isAlive[sockID]=true;
		}
	} else {
		if(recvBuf[1]!=CHAR_AD)
			LGgame::analyzeMove(sockID,recvBuf[1]-CHAR_AD,LGgame::playerCoo[sockID]);
		else LGgame::playerCoo[sockID]= {recvBuf[2]-CHAR_AD,recvBuf[3]-CHAR_AD};
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

int LGserver::GAME() {
	cleardevice();
	setrendermode(RENDER_MANUAL);
	LGGraphics::inputMapData(std::min(900 / mapH, 900 / mapW), std::min(900 / mapH, 900 / mapW), mapH, mapW);
	LGGraphics::init();

	lisEnd=false;
	std::mt19937 mtrd(std::chrono::system_clock::now().time_since_epoch().count());
	std::thread th(sockListen);
	th.detach();
	int plCnt=0,rbCnt=LGgame::playerCnt;
	rectBUTTON startBox;
	lisCon=true;

	LGgame::initGenerals(LGgame::playerCoo);
	settextjustify(CENTER_TEXT, CENTER_TEXT);
	setfont(50 * LGGraphics::mapDataStore.mapSizeY, 0, "Quicksand");

	startBox
	.size(200 * LGGraphics::mapDataStore.mapSizeX, 100 * LGGraphics::mapDataStore.mapSizeY)
	.move(400 * LGGraphics::mapDataStore.mapSizeX,350 * LGGraphics::mapDataStore.mapSizeY)
	.textalign(CENTER_TEXT, CENTER_TEXT)
	.fontname("Quicksand")
	.fontsize(50 * LGGraphics::mapDataStore.mapSizeY, 0)
	.bgcolor(WHITE)
	.textcolor(LGGraphics::mainColor)
	.addtext(L"Start game")
	.status=0;
	startBox.display();

	for(; is_run()&&lisCon; delay_fps(60)) {
		{
			std::lock_guard<std::mutex> mGuard(mLock);
			plCnt=totSock-1;
		}

		rbCnt=LGgame::playerCnt-plCnt;
		cleardevice();
		startBox.display();
		xyprintf(800 * LGGraphics::mapDataStore.mapSizeX, 400 * LGGraphics::mapDataStore.mapSizeY, "Player Number : %d",plCnt);
		sockCollect();

		while(mousemsg()) {
			startBox.status=0;
			mouse_msg msg = getmouse();

			if(msg.x >= 400 * LGGraphics::mapDataStore.mapSizeX && msg.x <= 600 * LGGraphics::mapDataStore.mapSizeY
			   && msg.y >= 350 * LGGraphics::mapDataStore.mapSizeY && msg.y <= 450 * LGGraphics::mapDataStore.mapSizeY) {
				startBox.status = 1;

				if(msg.is_left()) {
					lisCon=false;
					break;
				}
			}
		}
	}

	{
		std::lock_guard<std::mutex> mGuard(mLock);
		lisEnd=true;
	}

	LGgame::cheatCode=1048575;
	LGgame::gameMesC = 0;

	int robotId[64];
	std::deque<int> movement;

	for(int i = plCnt+1; i <= plCnt+rbCnt; ++i)
		robotId[i] = mtrd() % 300 + 1;

	zipSendBuf();
	sockBroadcast();
	sendBuf[0]=42;
	sendBuf[1]='\0';
	sockBroadcast();
	LGgame::updateMap();
	printMap(LGgame::cheatCode, LGgame::playerCoo[1]);
	LGgame::curTurn = 0;
	bool gameEnd = 0;
	rectBUTTON fpsbut;
	fpsbut.move(0, 1400 * LGGraphics::mapDataStore.mapSizeX);
	fpsbut.size(20 * LGGraphics::mapDataStore.mapSizeY, 200 * LGGraphics::mapDataStore.mapSizeX);
	fpsbut.textalign(CENTER_TEXT, CENTER_TEXT);
	fpsbut.fontname("Courier New");
	fpsbut.fontsize(20 * LGGraphics::mapDataStore.mapSizeY, 0);
	fpsbut.bgcolor(RED);
	fpsbut.textcolor(WHITE);
	rectBUTTON turnbut;
	turnbut
	.move(0, 1250 * LGGraphics::mapDataStore.mapSizeX)
	.size(20 * LGGraphics::mapDataStore.mapSizeY, 150 * LGGraphics::mapDataStore.mapSizeX)
	.textalign(CENTER_TEXT, CENTER_TEXT)
	.fontname("Courier New")
	.fontsize(20 * LGGraphics::mapDataStore.mapSizeY, 0)
	.bgcolor(BLUE)
	.textcolor(WHITE);

	flushkey();
	flushmouse();
	LGgame::beginTime = std::chrono::steady_clock::now().time_since_epoch();
	LGgame::inServer=true;

	for(; is_run(); delay_fps(LGgame::gameSpeed)) {
		while(mousemsg()) {
			mouse_msg msg = getmouse();

			if(msg.is_wheel()) {
				widthPerBlock += msg.wheel / 120;
				heightPerBlock += msg.wheel / 120;
				widthPerBlock = max(widthPerBlock, 2);
				heightPerBlock = max(heightPerBlock, 2);
			}
			/* Server map doesn't need dragging? */
		}

		while(kbmsg()) {
			key_msg ch = getkey();

			if(ch.msg == key_msg_up) continue;
			if(ch.key==27) {
				MessageBoxW(getHWnd(), wstring(L"YOU QUIT THE GAME.").c_str(), L"EXIT", MB_OK | MB_SYSTEMMODAL);
				closegraph();
				return 0;
			}
		}

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

		/* Server doesn't need a game end tip. */
		// if(!gameEnd) {
		// 	int ed = 0;
		// 	for(int i = 1; i <= LGgame::playerCnt; ++i)
		// 		ed |= (LGgame::isAlive[i] << i);
		// 	if(__builtin_popcount(ed) == 1) {
		// 		MessageBoxW(nullptr,
		// 		            ("PLAYER " + playerInfo[std::__lg(ed)].name + " WON!" + "\n" +
		// 		             "THE game WILL CONTINUE." + "\n" +
		// 		             "YOU CAN PRESS [ESC] TO EXIT.")
		// 		            .c_str(),
		// 		            "game END", MB_OK | MB_SYSTEMMODAL);
		// 		gameEnd = 1;
		// 		LGgame::cheatCode = 1048575;
		// 		zipSendBuf();
		// 		sockBroadcast();
		// 	}
		// }

		{
			std::chrono::nanoseconds timePassed = std::chrono::steady_clock::now().time_since_epoch() - LGgame::beginTime;
			int needFlushToTurn = ceil(timePassed.count() / 1000000000.0L * LGgame::gameSpeed);
			int lackTurn = LGgame::curTurn - needFlushToTurn;
			if(lackTurn < 0);
			else {
				while(lackTurn > 0) {
					timePassed = std::chrono::steady_clock::now().time_since_epoch() - LGgame::beginTime;
					needFlushToTurn = ceil(timePassed.count() / 1000000000.0L * LGgame::gameSpeed);
					lackTurn = LGgame::curTurn - needFlushToTurn;
				}
				cleardevice();
				printMap(LGgame::cheatCode, LGgame::playerCoo[0]);
				LGgame::ranklist();
				int screenszr = 1600 * LGGraphics::mapDataStore.mapSizeX;
				static int fpslen;
				static int turnlen;
				static int rspeedlen;
				setfillcolor(LGGraphics::bgColor);
				bar(screenszr - rspeedlen - 10 - fpslen - 10 - turnlen - 10, 0, screenszr, 20 * LGGraphics::mapDataStore.mapSizeY);
				setfont(20 * LGGraphics::mapDataStore.mapSizeY, 0, "Quicksand");
				timePassed = std::chrono::steady_clock::now().time_since_epoch() - LGgame::beginTime;
				fpslen = textwidth(("FPS: " + to_string(getfps())).c_str());
				turnlen = textwidth(("Turn " + to_string(LGgame::curTurn) + ".").c_str());
				rspeedlen = textwidth(("Real Speed: " + to_string(LGgame::curTurn * 1.0L / (timePassed.count() / 1000000000.0L))).c_str()); setfillcolor(RED);
				setfillcolor(GREEN);
				bar(screenszr - rspeedlen - 10, 0, screenszr, 20 * LGGraphics::mapDataStore.mapSizeY);
				rectangle(screenszr - rspeedlen - 10, 0, screenszr, 20 * LGGraphics::mapDataStore.mapSizeY);
				setfillcolor(RED);
				bar(screenszr - rspeedlen - 10 - fpslen - 10, 0, screenszr - rspeedlen - 10, 20 * LGGraphics::mapDataStore.mapSizeY);
				rectangle(screenszr - rspeedlen - 10 - fpslen - 10, 0, screenszr - rspeedlen - 10, 20 * LGGraphics::mapDataStore.mapSizeY);
				setfillcolor(BLUE);
				bar(screenszr - rspeedlen - 10 - fpslen - 10 - turnlen - 10, 0, screenszr - rspeedlen - 10 - fpslen - 10, 20 * LGGraphics::mapDataStore.mapSizeY);
				rectangle(screenszr - rspeedlen - 10 - fpslen - 10 - turnlen - 10, 0, screenszr - rspeedlen - 10 - fpslen - 10, 20 * LGGraphics::mapDataStore.mapSizeY);
				settextjustify(CENTER_TEXT, TOP_TEXT);
				xyprintf(screenszr - rspeedlen / 2 - 5, 0, "Real Speed: %Lf", LGgame::curTurn * 1.0L / (timePassed.count() / 1000000000.0L));
				xyprintf(screenszr - rspeedlen - 10 - fpslen / 2 - 5, 0, "FPS: %f", getfps());
				xyprintf(screenszr - rspeedlen - 10 - fpslen - 10 - turnlen / 2 - 5, 0, "Turn %d.", LGgame::curTurn);
			}
		}
	}

	sendBuf[0]=43;
	sendBuf[1]=CHAR_AD;
	sendBuf[2]='\0';
	sockBroadcast();
	std::lock_guard<std::mutex> mGuard(mLock);

	for(int i=1; i<totSock; i++)
		closesocket(serverSocket[i]);

	return 0;
}

void LGclient::dezipRecvBuf() {
	register int p=1,i,j,k;
	LGgame::playerCnt=recvBuf[p++]-CHAR_AD;
	LGgame::gameSpeed=recvBuf[p++]-CHAR_AD;
	LGgame::curTurn=recvBuf[p++]-CHAR_AD;
	LGgame::curTurn+=((recvBuf[p++]-CHAR_AD)<<6);
	LGgame::curTurn+=((recvBuf[p++]-CHAR_AD)<<12);
	LGgame::curTurn+=((recvBuf[p++]-CHAR_AD)<<18);

	for(i=1; i<=LGgame::playerCnt; i++) {
		LGgame::isAlive[i]=recvBuf[p++]-CHAR_AD;
		LGgame::playerCoo[i].x=recvBuf[p++]-CHAR_AD;
		LGgame::playerCoo[i].y=recvBuf[p++]-CHAR_AD;
	}

	mapH=recvBuf[p++]-CHAR_AD;
	mapH+=((recvBuf[p++]-CHAR_AD)<<6);
	mapW=recvBuf[p++]-CHAR_AD;
	mapW+=((recvBuf[p++]-CHAR_AD)<<6);

	for(i=1; i<=mapH; i++)
		for(j=1; j<=mapW; j++) {
			gameMap[i][j].player=recvBuf[p++]-CHAR_AD;
			recvBuf[p]-=CHAR_AD;
			bool f=recvBuf[p]&1; recvBuf[p]>>=1;
			gameMap[i][j].lit=recvBuf[p]&1; recvBuf[p]>>=1;
			gameMap[i][j].type=recvBuf[p++];
			gameMap[i][j].army=0;

			for(k=7; k>=0; k--)
				gameMap[i][j].army=(gameMap[i][j].army<<6)+recvBuf[p+k]-CHAR_AD;

			gameMap[i][j].army=f?(-gameMap[i][j].army):gameMap[i][j].army;
			p+=8;
		}
}

void LGclient::sockConnect() {
	if(initSock())
		return ;

	clientSocket=socket(AF_INET,SOCK_STREAM,0);
	SOCKADDR_IN connectAddr;
	connectAddr.sin_family=AF_INET;
	connectAddr.sin_addr.S_un.S_addr=inet_addr(IP);
	connectAddr.sin_port=htons(SKPORT);
	int res=connect(clientSocket,(LPSOCKADDR)&connectAddr,sizeof(connectAddr));
	u_long iMode=1;
	ioctlsocket(clientSocket,FIONBIO,&iMode);

	if(res==SOCKET_ERROR) {
		failSock|=4;
		WSACleanup();
		return ;
	} return ;
}

void LGclient::sockMessage() {
	std::lock_guard<std::mutex> mGuard(mLock);
	send(clientSocket,sendBuf,sizeof(sendBuf),0);
	memset(sendBuf,0,sizeof(sendBuf));
}

void LGclient::procMessage() {
	if(recvBuf[0]==42) lisCon=false;
	else if(recvBuf[0]==43) {
		if(recvBuf[1]==CHAR_AD) {
			failSock=5;
			exitExe();
		} else playerNumber=recvBuf[1]-CHAR_AD;
	} else dezipRecvBuf();
}

bool LGclient::sockCollect() {
	std::lock_guard<std::mutex> mGuard(mLock);
	int res=recv(clientSocket,recvBuf,sizeof(recvBuf),0);

	if(res>0) {
		procMessage();
		return true;
	}

	memset(recvBuf,0,sizeof(recvBuf));
	return false;
}

void LGclient::quitGame() {
	sendBuf[0]=43;
	sendBuf[1]=CHAR_AD;
	sendBuf[2]='\0';
	sockMessage();
}

int LGclient::GAME() {
	cleardevice();
	setrendermode(RENDER_MANUAL);
	LGGraphics::inputMapData(std::min(900 / mapH, 900 / mapW), std::min(900 / mapH, 900 / mapW), mapH, mapW);
	LGGraphics::init();
	LGgame::inClient = true;
	std::mt19937 mtrd(std::chrono::system_clock::now().time_since_epoch().count());
	rectBUTTON quitBox,IPfin;
	sys_edit IPbox;
	int plCnt;
	lisCon=true;

	settextjustify(CENTER_TEXT, CENTER_TEXT);
	setfont(50 * LGGraphics::mapDataStore.mapSizeY, 0, "Quicksand");

	quitBox
	.size(200 * LGGraphics::mapDataStore.mapSizeX, 100 * LGGraphics::mapDataStore.mapSizeY)
	.move(400 * LGGraphics::mapDataStore.mapSizeX,350 * LGGraphics::mapDataStore.mapSizeY)
	.textalign(CENTER_TEXT, CENTER_TEXT)
	.fontname("Quicksand")
	.fontsize(50 * LGGraphics::mapDataStore.mapSizeY, 0)
	.bgcolor(WHITE)
	.textcolor(LGGraphics::mainColor)
	.addtext(L"Quit game")
	.status=0;
	quitBox.display();

	IPfin
	.size(200 * LGGraphics::mapDataStore.mapSizeX, 100 * LGGraphics::mapDataStore.mapSizeY)
	.move(100 * LGGraphics::mapDataStore.mapSizeX,350 * LGGraphics::mapDataStore.mapSizeY)
	.textalign(CENTER_TEXT, CENTER_TEXT)
	.fontname("Quicksand")
	.fontsize(50 * LGGraphics::mapDataStore.mapSizeY, 0)
	.bgcolor(WHITE)
	.textcolor(LGGraphics::mainColor)
	.addtext(L"Connect")
	.status=0;
	IPfin.display();

	IPbox.create(true);
	IPbox.move(800 * LGGraphics::mapDataStore.mapSizeX, 350 * LGGraphics::mapDataStore.mapSizeY);
	IPbox.size(400 * LGGraphics::mapDataStore.mapSizeX, 100 * LGGraphics::mapDataStore.mapSizeY);
	IPbox.setbgcolor(WHITE);
	IPbox.setcolor(LGGraphics::mainColor);
	IPbox.setfont(50 * LGGraphics::mapDataStore.mapSizeY, 0, "Quicksand");
	IPbox.visible(true);

	for(; is_run(); delay_fps(60)) {
		quitBox.display();
		IPfin.display();

		while(mousemsg()) {
			quitBox.status=0;
			IPfin.status=0;
			mouse_msg msg = getmouse();

			if(msg.x >= 400 * LGGraphics::mapDataStore.mapSizeX && msg.x <= 600 * LGGraphics::mapDataStore.mapSizeY
			   && msg.y >= 350 * LGGraphics::mapDataStore.mapSizeY && msg.y <= 450 * LGGraphics::mapDataStore.mapSizeY) {
				quitBox.status = 1;

				if(msg.is_left()) {
					closegraph();
					return 0;
				} continue;
			}

			if(msg.x >= 100 * LGGraphics::mapDataStore.mapSizeX && msg.x <= 300 * LGGraphics::mapDataStore.mapSizeY
			   && msg.y >= 350 * LGGraphics::mapDataStore.mapSizeY && msg.y <= 450 * LGGraphics::mapDataStore.mapSizeY) {
				IPfin.status = 1;

				if(msg.is_left()) {
					IPfin.status=2;
					break;
				} continue;
			}
		}

		if(IPfin.status==2) {
			IPbox.gettext(25,IP);
			break;
		}
	}

	sockConnect();
	IPbox.visible(false);
	cleardevice();
	sendBuf[0]=43;
	sendBuf[1]='\0';
	sockMessage();

	for(; is_run()&&lisCon; delay_fps(60)) {
		cleardevice();
		quitBox.display();
		xyprintf(1000 * LGGraphics::mapDataStore.mapSizeX, 400 * LGGraphics::mapDataStore.mapSizeY, "Player Number : %d",playerNumber);
		sockCollect();

		while(mousemsg()) {
			quitBox.status=0;
			mouse_msg msg = getmouse();

			if(msg.x >= 400 * LGGraphics::mapDataStore.mapSizeX && msg.x <= 600 * LGGraphics::mapDataStore.mapSizeY
			   && msg.y >= 350 * LGGraphics::mapDataStore.mapSizeY && msg.y <= 450 * LGGraphics::mapDataStore.mapSizeY) {
				quitBox.status = 1;

				if(msg.is_left()) {
					quitGame();
					closegraph();
					return 0;
				}
			}
		}
	}

	LGgame::cheatCode=(1<<playerNumber);
	std::deque<int> movement;
	printMap(LGgame::cheatCode, LGgame::playerCoo[playerNumber]);
	bool gameEnd = 0;
	int movLin,movCol;
	rectBUTTON fpsbut;
	fpsbut.move(0, 1400 * LGGraphics::mapDataStore.mapSizeX);
	fpsbut.size(20 * LGGraphics::mapDataStore.mapSizeY, 200 * LGGraphics::mapDataStore.mapSizeX);
	fpsbut.textalign(CENTER_TEXT, CENTER_TEXT);
	fpsbut.fontname("Courier New");
	fpsbut.fontsize(20 * LGGraphics::mapDataStore.mapSizeY, 0);
	fpsbut.bgcolor(RED);
	fpsbut.textcolor(WHITE);
	rectBUTTON turnbut;
	turnbut
	.move(0, 1250 * LGGraphics::mapDataStore.mapSizeX)
	.size(20 * LGGraphics::mapDataStore.mapSizeY, 150 * LGGraphics::mapDataStore.mapSizeX)
	.textalign(CENTER_TEXT, CENTER_TEXT)
	.fontname("Courier New")
	.fontsize(20 * LGGraphics::mapDataStore.mapSizeY, 0)
	.bgcolor(BLUE)
	.textcolor(WHITE);

	flushkey();
	flushmouse();
	LGgame::beginTime = std::chrono::steady_clock::now().time_since_epoch();

	int midact = 0;
	int smsx = 0, smsy = 0; bool moved = false;
	std::chrono::steady_clock::duration prsttm;
	for(; is_run(); delay_fps(std::min(LGgame::gameSpeed + 0.5, 120.5))) {
		movLin=LGgame::playerCoo[playerNumber].x;
		movCol=LGgame::playerCoo[playerNumber].y;

		while(mousemsg()) {
			mouse_msg msg = getmouse();

			if(msg.is_wheel()) {
				widthPerBlock += msg.wheel / 120;
				heightPerBlock += msg.wheel / 120;
				widthPerBlock = max(widthPerBlock, 2);
				heightPerBlock = max(heightPerBlock, 2);
			}
			if(msg.is_move()) {
				if(midact == 1) {
					LGGraphics::mapDataStore.maplocX += msg.x - smsx;
					LGGraphics::mapDataStore.maplocY += msg.y - smsy;
					smsx = msg.x, smsy = msg.y; moved = true;
				}
			} else if(msg.is_left()) {
				if(msg.is_down()) {
					prsttm = std::chrono::steady_clock::now().time_since_epoch();
					midact = 1;
					smsx = msg.x, smsy = msg.y;
					moved = false;
				} else {
					midact = 0;
					std::chrono::steady_clock::duration now = std::chrono::steady_clock::now().time_since_epoch();
					if(!moved && now - prsttm < 300ms) {
						if(msg.x >= LGGraphics::mapDataStore.maplocX &&
						   msg.y >= LGGraphics::mapDataStore.maplocY &&
						   msg.x <= LGGraphics::mapDataStore.maplocX + widthPerBlock * mapW &&
						   msg.y <= LGGraphics::mapDataStore.maplocY + heightPerBlock * mapH) {
							movLin = (msg.y - LGGraphics::mapDataStore.maplocY + heightPerBlock - 1) / heightPerBlock;
							movCol = (msg.x - LGGraphics::mapDataStore.maplocX + widthPerBlock - 1) / widthPerBlock;
							movement.clear();
							movement.emplace_back(0);
						}
					}
				}
			}
		}

		while(kbmsg()) {
			key_msg ch = getkey();
			if(ch.msg == key_msg_up)
				continue;
			switch(ch.key) {
				case int('w'): movement.emplace_back(1); break;
				case int('a'): movement.emplace_back(2); break;
				case int('s'): movement.emplace_back(3); break;
				case int('d'): movement.emplace_back(4); break;

				case key_up: /*[UP]*/ movement.emplace_back(5); break;
				case key_left: /*[LEFT]*/ movement.emplace_back(6); break;
				case key_down: /*[DOWN]*/ movement.emplace_back(7); break;
				case key_right: /*[RIGHT]*/ movement.emplace_back(8); break;

				case int('g'): movement.emplace_back(0); break;
				case int('e'):
					if(!movement.empty())
						movement.pop_back();
					break;
				case int('q'): movement.clear(); break;
				case 27: {
					MessageBoxW(getHWnd(), wstring(L"YOU QUIT THE GAME.").c_str(), L"EXIT", MB_OK | MB_SYSTEMMODAL);
					quitGame();
					closegraph();
					return 0;
				}
				case int('\b'): {
					if(!LGgame::isAlive[playerNumber])
						break;
					int confirmSur = MessageBoxW(getHWnd(), wstring(L"ARE YOU SURE TO SURRENDER?").c_str(), L"CONFIRM SURRENDER", MB_YESNO | MB_SYSTEMMODAL);
					if(confirmSur == 7)
						break;
					LGgame::isAlive[playerNumber] = 0;
					LGgame::printGameMessage({playerNumber, playerNumber, LGgame::curTurn});
					quitGame();
					closegraph();
					return 0;
					break;
				}
			}
		}

		if(!movement.empty()) {
			sendBuf[0]=44;
			sendBuf[1]=movement.front()+CHAR_AD;
			sendBuf[2]=movLin+CHAR_AD;
			sendBuf[3]=movCol+CHAR_AD;
			sendBuf[4]='\0';
			sockMessage();
			movement.pop_front();
		} while(!movement.empty())
			movement.pop_front();

		while(sockCollect());

		if(!gameEnd) {
			int ed = 0;
			for(int i = 1; i <= LGgame::playerCnt; ++i)
				ed |= (LGgame::isAlive[i] << i);
			if(__builtin_popcount(ed) == 1) {
				MessageBoxW(nullptr,
				            (L"PLAYER " + playerInfo[std::__lg(ed)].name + L" WON!" + L"\n" +
				             L"THE GAME WILL CONTINUE." + L"\n" +
				             L"YOU CAN PRESS [ESC] TO EXIT.")
				            .c_str(),
				            L"GAME END", MB_OK | MB_SYSTEMMODAL);
				gameEnd = 1;
				register int winnerNum = std::__lg(ed);
				LGgame::cheatCode = 1048575;
				LGgame::printGameMessage({winnerNum, -1, LGgame::curTurn});
			}
		} {
			std::chrono::nanoseconds timePassed = std::chrono::steady_clock::now().time_since_epoch() - LGgame::beginTime;
			int needFlushToTurn = ceil(timePassed.count() / 1000000000.0L * LGgame::gameSpeed);
			int lackTurn = LGgame::curTurn - needFlushToTurn;
			if(lackTurn < 0);
			else {
				while(lackTurn > 0) {
					timePassed = std::chrono::steady_clock::now().time_since_epoch() - LGgame::beginTime;
					needFlushToTurn = ceil(timePassed.count() / 1000000000.0L * LGgame::gameSpeed);
					lackTurn = LGgame::curTurn - needFlushToTurn;
				}
				cleardevice();
				printMap(LGgame::cheatCode, LGgame::playerCoo[0]);
				LGgame::ranklist();
				int screenszr = 1600 * LGGraphics::mapDataStore.mapSizeX;
				static int fpslen;
				static int turnlen;
				static int rspeedlen;
				setfillcolor(LGGraphics::bgColor);
				bar(screenszr - rspeedlen - 10 - fpslen - 10 - turnlen - 10, 0, screenszr, 20 * LGGraphics::mapDataStore.mapSizeY);
				setfont(20 * LGGraphics::mapDataStore.mapSizeY, 0, "Quicksand");
				timePassed = std::chrono::steady_clock::now().time_since_epoch() - LGgame::beginTime;
				fpslen = textwidth((L"FPS: " + to_wstring(getfps())).c_str());
				turnlen = textwidth((L"Turn " + to_wstring(LGgame::curTurn) + L".").c_str());
				rspeedlen = textwidth((L"Real Speed: " + to_wstring(LGgame::curTurn * 1.0L / (timePassed.count() / 1000000000.0L))).c_str()); setfillcolor(RED);
				setfillcolor(GREEN);
				bar(screenszr - rspeedlen - 10, 0, screenszr, 20 * LGGraphics::mapDataStore.mapSizeY);
				rectangle(screenszr - rspeedlen - 10, 0, screenszr, 20 * LGGraphics::mapDataStore.mapSizeY);
				setfillcolor(RED);
				bar(screenszr - rspeedlen - 10 - fpslen - 10, 0, screenszr - rspeedlen - 10, 20 * LGGraphics::mapDataStore.mapSizeY);
				rectangle(screenszr - rspeedlen - 10 - fpslen - 10, 0, screenszr - rspeedlen - 10, 20 * LGGraphics::mapDataStore.mapSizeY);
				setfillcolor(BLUE);
				bar(screenszr - rspeedlen - 10 - fpslen - 10 - turnlen - 10, 0, screenszr - rspeedlen - 10 - fpslen - 10, 20 * LGGraphics::mapDataStore.mapSizeY);
				rectangle(screenszr - rspeedlen - 10 - fpslen - 10 - turnlen - 10, 0, screenszr - rspeedlen - 10 - fpslen - 10, 20 * LGGraphics::mapDataStore.mapSizeY);
				settextjustify(CENTER_TEXT, TOP_TEXT);
				xyprintf(screenszr - rspeedlen / 2 - 5, 0, L"Real Speed: %Lf", LGgame::curTurn * 1.0L / (timePassed.count() / 1000000000.0L));
				xyprintf(screenszr - rspeedlen - 10 - fpslen / 2 - 5, 0, L"FPS: %f", getfps());
				xyprintf(screenszr - rspeedlen - 10 - fpslen - 10 - turnlen / 2 - 5, 0, L"Turn %d.", LGgame::curTurn);
			}
		}
	}
	return 0;
}

#endif
