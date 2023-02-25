#include <stdio.h>
#include <conio.h>
#include <mutex>
#include <queue>
#include <thread>
#include <winsock2.h>
#include "LGcons.hpp"
const int SSN=205,SSL=100005;
const int SKPORT=8888;

std::mutex mLock;
int failSock,ky,opt;
char sendBuf[SSL],recvBuf[SSL];
SOCKET clientSocket;

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

void sockConnect(){
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

void sockMessage(){
	std::lock_guard<std::mutex> mGuard(mLock);
	send(clientSocket,sendBuf,sizeof(sendBuf),0);
	memset(sendBuf,0,sizeof(sendBuf));
}

void sockCollect(){
	std::lock_guard<std::mutex> mGuard(mLock);
	int res=recv(clientSocket,recvBuf,sizeof(recvBuf),0);
	
	if(res>0){
		gotoxy(opt,0);
		printf("%s",recvBuf);opt++;
	}memset(recvBuf,0,sizeof(recvBuf));
}

void serverGame(){
	
}

signed main(){
	initattr();
	char ch,scin[SSL];
	int scnt=0;
	printf(":             ");
	opt=2;
	sockConnect();
	
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
					sockMessage();
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
	closesocket(clientSocket);
	return 0;
}
