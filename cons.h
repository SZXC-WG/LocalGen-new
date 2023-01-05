//Cls: clean screen
//SetPos: Pos->x row y col
//Color: Color->Pixel
//HideCursor: HideConsoleCursor

#include<windows.h>
#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<conio.h>

const int T=984;
const int S=1000000007;

HANDLE hOut=GetStdHandle(STD_OUTPUT_HANDLE);

inline void Cls(){
	system("cls");
}

inline void SetPos(int x,int y){
	COORD pos;pos.X=y,pos.Y=x;
	SetConsoleCursorPosition(hOut,pos);
}

inline void Color(int x){
	SetConsoleTextAttribute(hOut,x);
}

void HideCursor(){
	CONSOLE_CURSOR_INFO cursor_info={1,0};
	SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE),&cursor_info);
}
