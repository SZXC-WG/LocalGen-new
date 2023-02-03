#include "LGweb.hpp"
#include "LGcons.hpp"
#include "LGgame.hpp"
#include "LGzipmap.hpp"

std::mutex mLock;
int ky,totSock,opt;
char sendBuf[SSL],recvBuf[SSL];
SOCKET serverSocket[SSN];
bool sockCon[SSN],lisEnd;
int stDel,plCnt,rbCnt;
std::deque<int> movement;

void sockListen(){
	if(initSock())
	return ;
	
	SOCKET listenSocket=socket(AF_INET,SOCK_STREAM,0);
	SOCKADDR_IN listenAddr;
	listenAddr.sin_family=AF_INET;
	listenAddr.sin_addr.S_un.S_addr=INADDR_ANY;
	listenAddr.sin_port=htons(SKPORT);
	int res=bind(listenSocket,(LPSOCKADDR)&listenAddr,sizeof(listenAddr));
	
	if(res==SOCKET_ERROR){
		failSock|=4;
		WSACleanup();
		return ;
	}
	
	u_long iMode=1;
	int lis=listen(listenSocket,20);
	
	while(1){
		SOCKET *clientSocket;
		
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

void procMessage(){
	if()
}

void sockBroadcast(){
	std::lock_guard<std::mutex> mGuard(mLock);
	
	for(int i=1;i<totSock;i++)
	send(serverSocket[i],sendBuf,sizeof(sendBuf),0);
	
	memset(sendBuf,0,sizeof(sendBuf));
}

void sockCollect(){
	std::lock_guard<std::mutex> mGuard(mLock);
	
	for(int i=1;i<totSock;i++){
		int res=recv(serverSocket[i],recvBuf,sizeof(recvBuf),0);
		if(res>0) procMessage();
		memset(recvBuf,0,sizeof(recvBuf));
	}
}

int serverGame(){
	gameStatus GAME(1,1048575,plCnt+rbCnt,stDel);
	
	if (GAME.played)
		return -1;
		
	GAME.played = 1;
	GAME.gameMesC = 0;
	
	if (!GAME.isWeb)
	{
		int robotId[64];
		playerCoord coordinate[64];
		std::mt19937 mtrd(std::chrono::system_clock::now().time_since_epoch().count());
		
		for (int i = plCnt+1; i <= plCnt+rbCnt; ++i)
			robotId[i] = mtrd() % 300 + 1;
			
		GAME.initGenerals(coordinate);
		GAME.updateMap();
		GAME.curTurn = 0;
		bool gameEnd = 0;
		
		for (; is_run(); delay_fps(GAME.stepDelay))
		{
			GAME.updateMap();
			sockCollect();
			
			while (!movement.empty() && GAME.analyzeMove(1, movement.front(), coordinate[1]))
				movement.pop_front();
			if (!movement.empty())
				movement.pop_front();
			
			for (int i = plCnt+1; i <= plCnt+rbCnt; ++i)
			{
				if (!GAME.isAlive[i])
					continue;
				switch (robotId[i])
				{
				case 1 ... 100:
					GAME.analyzeMove(i, smartRandomBot::smartRandomBot(i, coordinate[i]), coordinate[i]);
					break;
				case 101 ... 200:
					GAME.analyzeMove(i, xrzBot::xrzBot(i, coordinate[i]), coordinate[i]);
					break;
				case 201 ... 300:
					GAME.analyzeMove(i, xiaruizeBot::xiaruizeBot(i, coordinate[i]), coordinate[i]);
					break;
				default:
					GAME.analyzeMove(i, 0, coordinate[i]);
				}
			}
			
			GAME.flushMove();
			zipStatus();
			int k=0;
			
			for(k=0;strZipStatus[k]!='\0';k++)
			sendBuf[k]=strZipStatus[k];
			
			sendBuf[k]='\0';
			sockBroadcast();
			
			if (GAME.cheatCode != 1048575)
			{
				int alldead = 0;
				for (int i = 1; i <= GAME.playerCnt && !alldead; ++i)
				{
					if (GAME.cheatCode & (1 << i))
						if (GAME.isAlive[i])
							alldead = 1;
				}
				if (!alldead)
				{
					GAME.cheatCode = 1048575;
					MessageBox(nullptr, "ALL THE PLAYERS YOU SELECTED TO BE SEEN IS DEAD.\nTHE OVERALL CHEAT MODE WILL BE SWITCHED ON.", "TIP", MB_OK | MB_SYSTEMMODAL);
				}
			}
			if (!gameEnd)
			{
				int ed = 0;
				for (int i = 1; i <= GAME.playerCnt; ++i)
					ed |= (GAME.isAlive[i] << i);
				if (__builtin_popcount(ed) == 1)
				{
					MessageBox(nullptr,
							   ("PLAYER " + defTeams[std::__lg(ed)].name + " WON!" + "\n" +
								"THE GAME WILL CONTINUE." + "\n" +
								"YOU CAN PRESS [ESC] TO EXIT.")
								   .c_str(),
							   "GAME END", MB_OK | MB_SYSTEMMODAL);
					gameEnd = 1;
					GAME.winnerNum = std::__lg(ed);
					GAME.cheatCode = 1048575;
				}
			}
		}
	}
}

signed main(){
	initattr();
	puts("This is the server of LocalGenerals.");
	puts("Press any buttom to start the server.");
	char ch;
	
	while(1)
	if(kbhit()){
		char ch=getch();
		break;
	}
	
	clearance();
	std::thread th(sockListen);
	th.detach();
	printf("Listening......");
	
	while(!lisEnd){
		sockCollect();
		gotoxy(1,1);
	}
	
	{
		std::lock_guard<std::mutex> mGuard(mLock);
		plCnt=totSock-1;
	}
	
	serverGame();
	
//	char ch,scin[SSL];
//	int scnt=0;
//	printf(":             ");
//	opt=2;
//	std::thread th(sockListen);
//	th.detach();
//	
//	while(1){
//		sockCollect();
//		
//		if(_kbhit()){
//			ch=_getch();
//			
//			switch(ch){
//				case 48:goto END;
//				case 13:
//					for(int i=0;i<scnt;i++)
//					sendBuf[i]=scin[i];
//					sendBuf[scnt]=scin[scnt]='\0';
//					sockBroadcast();
//					gotoxy(opt,1);
//					printf("%s",scin);opt++;
//					gotoxy(1,2);
//					printf("                                                ");
//					scnt=0;
//					break;
//				case 8:
//					if(scnt>0){
//						gotoxy(1,scnt+2);
//						printf("\b \b");
//						scnt--;
//					}break;
//				default:
//					gotoxy(1,scnt+2);
//					printf("%c",ch);
//					scin[scnt++]=ch;
//					break;
//			}
//		}
//	}
//	
//	END:;
	WSACleanup();
	return 0;
}
