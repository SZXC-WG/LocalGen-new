int serverGame() {
	game.isWeb=1;
	game.cheatCode=1048575;
	game.playerCnt=plCnt+rbCnt;
	game.stepDelay=stDel;

	if(game.played)
		return -1;

	game.played = 1;
	game.gameMesC = 0;

	if(!game.isWeb) {
		int robotId[64];
		std::mt19937 mtrd(std::chrono::system_clock::now().time_since_epoch().count());

		for(int i = plCnt+1; i <= plCnt+rbCnt; ++i)
			robotId[i] = mtrd() % 300 + 1;

		game.initGenerals(coordinate);
		game.updateMap();
		game.curTurn = 0;
		bool gameEnd = 0;

		for(; is_run(); delay_fps(game.stepDelay)) {
			game.updateMap();
			sockCollect();

			while(!movement.empty() && game.analyzeMove(1, movement.front(), coordinate[1]))
				movement.pop_front();
			if(!movement.empty())
				movement.pop_front();

			for(int i = plCnt+1; i <= plCnt+rbCnt; ++i) {
				if(!game.isAlive[i])
					continue;
				switch(robotId[i]) {
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

			if(game.cheatCode != 1048575) {
				int alldead = 0;
				for(int i = 1; i <= game.playerCnt && !alldead; ++i) {
					if(game.cheatCode & (1 << i))
						if(game.isAlive[i])
							alldead = 1;
				}
				if(!alldead) {
					game.cheatCode = 1048575;
					MessageBox(nullptr, "ALL THE PLAYERS YOU SELECTED TO BE SEEN IS DEAD.\nTHE OVERALL CHEAT MODE WILL BE SWITCHED ON.", "TIP", MB_OK | MB_SYSTEMMODAL);
				}
			}
			if(!gameEnd) {
				int ed = 0;
				for(int i = 1; i <= game.playerCnt; ++i)
					ed |= (game.isAlive[i] << i);
				if(__builtin_popcount(ed) == 1) {
					MessageBox(nullptr,
					           ("PLAYER " + playerInfo[std::__lg(ed)].name + " WON!" + "\n" +
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
signed main() {
//	initattr();
	puts("This is the server of LocalGenerals.");
	puts("Press any buttom to start the server.");
	char ch;

	while(1)
	if(kbhit()) {
		char ch=getch();
		break;
	}

	clearance();
	std::thread th(sockListen);
	th.detach();
	printf("Listening......");

	while(!lisEnd) {
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
