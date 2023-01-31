#include <stdio.h>
#include <conio.h>
#include "LGweb.hpp"
#include "LGcons.hpp"

std::mutex mLock;
int ky,totSock,opt;
char sendBuf[SSL],recvBuf[SSL];
SOCKET serverSocket[SSN];
bool sockCon[SSN],lisEnd;

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
			sockCon[totSock]=true;
		}
	}
	
	closesocket(listenSocket);
	return ;
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
		if(res>0){
			gotoxy(opt,0);
			printf("%s\n",recvBuf);opt++;
		}memset(recvBuf,0,sizeof(recvBuf));
	}
}

void serverGame(){
	
}

signed main(){
//	puts("This is the server of LocalGenerals.");
//	puts("Press any buttom to start the server.");
//	char ch;
//	
//	while(1)
//	if(_kbhit()){
//		char ch=getch();
//		break;
//	}
//	
//	std::thread th(sockListen);
//	th.detach();
//	clearance();
//	printf("Listening......");
//	serverGame();
	initattr();
	char ch,scin[SSL];
	int scnt=0;
	printf(":             ");
	opt=2;
	std::thread th(sockListen);
	th.detach();
	
	while(1){
		sockCollect();
		
		if(_kbhit()){
			ch=_getch();
			
			switch(ch){
				case 48:goto END;
				case 13:
					for(int i=0;i<scnt;i++)
					sendBuf[i]=scin[i];
					sendBuf[scnt]=scin[scnt]='\0';
					sockBroadcast();
					gotoxy(opt,1);
					printf("%s",scin);opt++;
					gotoxy(1,2);
					printf("                                                ");
					scnt=0;
					break;
				case 8:
					if(scnt>0){
						gotoxy(1,scnt+2);
						printf("\b \b");
						scnt--;
					}break;
				default:
					gotoxy(1,scnt+2);
					printf("%c",ch);
					scin[scnt++]=ch;
					break;
			}
		}
	}
	
	END:;
	WSACleanup();
	return 0;
}
