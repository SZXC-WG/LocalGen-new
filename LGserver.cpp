#include "LGweb.hpp"
#include "LGcons.hpp"
#include "LGgame.hpp"
#include "LGzipmap.hpp"

//Value
int opt;
int stDel,plCnt,rbCnt;
std::deque<int> movement;
playerCoord coordinate[64];
gameStatus game(0,0,0,0);

//Socket
std::mutex mLock;
int totSock;
bool sockCon[SSN],lisEnd;
SOCKET serverSocket[SSN];
char sendBuf[SSL],recvBuf[SSL];

//Functions
void zipSendBuf(){
	register int p=0,i,j;
	long long k1=mapH,k2=mapW;
	sendBuf[p++]=44;
	sendBuf[p++]=game.playerCnt;
	
	for(i=1;i<=game.playerCnt;i++){
		sendBuf[p++]=coordinate[i].x;
		sendBuf[p++]=coordinate[i].y;
	}
	
	sendBuf[p++]=PMod(k1)+CHAR_AD;
	sendBuf[p++]=PMod(k1)+CHAR_AD;
	sendBuf[p++]=PMod(k2)+CHAR_AD;
	sendBuf[p++]=PMod(k2)+CHAR_AD;
	
	for(i=1;i<=mapH;i++)
	for(j=1;j<=mapW;j++){
		sendBuf[p++]=gameMap[i][j].team+CHAR_AD;
		sendBuf[p]=(gameMap[i][j].type<<2)+(gameMap[i][j].lit<<1);
		k1=gameMap[i][j].army;
		
		if(k1<0){
			k1=-k1;
			sendBuf[p++]+=CHAR_AD+1;
		}else sendBuf[p++]+=CHAR_AD;
		
		for(k2=1;k2<=8;k2++)
		sendBuf[p++]=PMod(k1)+CHAR_AD;
	}sendBuf[p]='\0';
}

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

void procMessage(int sockID){
	if(recvBuf[0]==43){
		if(recvBuf[1]==CHAR_AD)
		sockCon[sockID]=false;
		else{
			if(lisEnd) sendBuf[1]=CHAR_AD;
			else sendBuf[1]=sockID+CHAR_AD;
			
			sendBuf[0]=43;
			sendBuf[2]='\0';
			send(serverSocket[sockID],sendBuf,sizeof(sendBuf),0);
		}
	}else{
		
	}
}

void sockBroadcast(){
	std::lock_guard<std::mutex> mGuard(mLock);
	
	for(int i=1;i<totSock;i++) if(sockCon[i])
	send(serverSocket[i],sendBuf,sizeof(sendBuf),0);
	
	memset(sendBuf,0,sizeof(sendBuf));
}

void sockCollect(){
	std::lock_guard<std::mutex> mGuard(mLock);
	
	for(int i=1;i<totSock;i++) if(sockCon[i]){
		int res=recv(serverSocket[i],recvBuf,sizeof(recvBuf),0);
		if(res>0) procMessage(i);
		memset(recvBuf,0,sizeof(recvBuf));
	}
}

int serverGame(){
	game.isWeb=1;
	game.cheatCode=1048575;
	game.playerCnt=plCnt+rbCnt;
	game.stepDelay=stDel;
	
	if (game.played)
		return -1;
		
	game.played = 1;
	game.gameMesC = 0;
	
	if (!game.isWeb)
	{
		int robotId[64];
		std::mt19937 mtrd(std::chrono::system_clock::now().time_since_epoch().count());
		
		for (int i = plCnt+1; i <= plCnt+rbCnt; ++i)
			robotId[i] = mtrd() % 300 + 1;
			
		game.initGenerals(coordinate);
		game.updateMap();
		game.curTurn = 0;
		bool gameEnd = 0;
		
		for (; is_run(); delay_fps(game.stepDelay))
		{
			game.updateMap();
			sockCollect();
			
			while (!movement.empty() && game.analyzeMove(1, movement.front(), coordinate[1]))
				movement.pop_front();
			if (!movement.empty())
				movement.pop_front();
			
			for (int i = plCnt+1; i <= plCnt+rbCnt; ++i)
			{
				if (!game.isAlive[i])
					continue;
				switch (robotId[i])
				{
				case 1 ... 100:
					game.analyzeMove(i, smartRandomBot::smartRandomBot(i, coordinate[i]), coordinate[i]);
					break;
				case 101 ... 200:
					game.analyzeMove(i, xrzBot::xrzBot(i, coordinate[i]), coordinate[i]);
					break;
				case 201 ... 300:
					game.analyzeMove(i, xiaruizeBot::xiaruizeBot(i, coordinate[i]), coordinate[i]);
					break;
				default:
					game.analyzeMove(i, 0, coordinate[i]);
				}
			}
			
			game.flushMove();
			zipSendBuf();
			sockBroadcast();
			
			if (game.cheatCode != 1048575)
			{
				int alldead = 0;
				for (int i = 1; i <= game.playerCnt && !alldead; ++i)
				{
					if (game.cheatCode & (1 << i))
						if (game.isAlive[i])
							alldead = 1;
				}
				if (!alldead)
				{
					game.cheatCode = 1048575;
					MessageBox(nullptr, "ALL THE PLAYERS YOU SELECTED TO BE SEEN IS DEAD.\nTHE OVERALL CHEAT MODE WILL BE SWITCHED ON.", "TIP", MB_OK | MB_SYSTEMMODAL);
				}
			}
			if (!gameEnd)
			{
				int ed = 0;
				for (int i = 1; i <= game.playerCnt; ++i)
					ed |= (game.isAlive[i] << i);
				if (__builtin_popcount(ed) == 1)
				{
					MessageBox(nullptr,
							   ("PLAYER " + defTeams[std::__lg(ed)].name + " WON!" + "\n" +
								"THE game WILL CONTINUE." + "\n" +
								"YOU CAN PRESS [ESC] TO EXIT.")
								   .c_str(),
							   "game END", MB_OK | MB_SYSTEMMODAL);
					gameEnd = 1;
					game.winnerNum = std::__lg(ed);
					game.cheatCode = 1048575;
				}
			}
		}
	}
}

//Main
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
