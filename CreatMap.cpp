#include"cons.h"
const int T_SEC=1000;

struct Node{
	int B,K;
	long long D; 
};

struct Picxel{
	int C1,C2;
	char c;

	bool operator==(Picxel x){
		return C1==x.C1&&C2==x.C2&&c==x.c;
	}
};

int sL,sW,rL,rW,Px,Py;
Node Map[80][80];
Picxel mp[505][505],fmp[505][505];
char s[15],nm[505],NUM_s[15]={0,'H','K','W','L','M','Q','Y','B','G','T'};
char P_s[200105];
FILE *pk;

void FrameOutput(){Cls();
	SetPos(0,1);for(int i=1;i<=rW;i++) printf("-");
	SetPos(rL+1,1);for(int i=1;i<=rW;i++) printf("-");
	for(int i=1;i<=rL;i++){
		SetPos(i,0);printf("|");
		SetPos(i,rW+1);printf("|");
	}

	SetPos(0,rW+1);printf("+");
	SetPos(0,0);printf("+");
	SetPos(rL+1,0);printf("+");
	SetPos(rL+1,rW+1);printf("+");

	SetPos(1,rW+3);printf("CreateMap.");
	SetPos(2,rW+3);printf("Use 'W', 'A', 'S', 'D' to move.");
	SetPos(3,rW+3);printf("Setting 'P'->Plain, 'M'->Mud, 'R'->Rock, 'C'->City.");
	SetPos(4,rW+3);printf("Use 'Q' to stop and enter the numbers.");
	SetPos(5,rW+3);printf("Use 'X' to Loadout the Map.");
	SetPos(6,rW+3);printf("Use 'E' to exit.");
	SetPos(7,rW+3);printf("Input:");
}

inline void FillC1(int x1,int y1,int x2,int y2,int C){
	register int i,j;
	for(i=x1;i<=x2;i++)
	for(j=y1;j<=y2;j++)
	mp[i][j].C1=C;
}
inline void FillC2(int x1,int y1,int x2,int y2,int C){
	register int i,j;
	for(i=x1;i<=x2;i++)
	for(j=y1;j<=y2;j++)
	mp[i][j].C2=C;
}
inline void Fillc(int x1,int y1,int x2,int y2,char c){
	register int i,j;
	for(i=x1;i<=x2;i++)
	for(j=y1;j<=y2;j++)
	mp[i][j].c=c;
}
inline void Fills(int x,int y1,int y2){
	for(register int i=y1;i<=y2;i++)
	mp[x][i].c=s[i-y1];
}

inline void Move(int x,int y){
	FillC2(Px,Py*5-4,Px,Py*5,0);
	Px=(Px+x<1||Px+x>sL)?Px:(Px+x);
	Py=(Py+y<1||Py+y>sW)?Py:(Py+y);
	FillC2(Px,Py*5-4,Px,Py*5,240);
}

inline void SetK(int x,int y,int k){Map[x][y].K=k;}
inline void SetD(int x,int y,long long d){Map[x][y].D=d;}
inline void SetB(int x,int y,int b){Map[x][y].B=b;}
inline void SetW(int x,int y,int b,long long d,int k){
	Map[x][y].B=b;
	Map[x][y].D=d;
	Map[x][y].K=k;
}

inline void TurnStr(long long x){
	for(int i=0;i<10;i++) s[i]=' ';
	int f=0,tw,gw;
	
	if(x<0){
		x=-x;
		s[0]='-';
		
		while(x/100!=0){
			x/=10;
			f++;
		}
		
		s[0]='-';
		tw=x/10;
		gw=x%10;
		
		if(tw==0) s[1]=gw+48;
		else{s[1]=tw+48;
		if(f==0) s[2]=gw+48;
		else if(f<10) s[2]=NUM_s[f];
		else s[2]='T';}
	}else{
		while(x/1000!=0){
			x/=10;
			f++;
		}
		
		gw=x%10;x/=10;
		tw=x%10;x/=10;
		
		if(x!=0) s[0]=x+48;
		if(tw!=0||x!=0) s[1]=tw+48;
		
		if(f==0) s[2]=gw+48;
		else if(f<10) s[2]=NUM_s[f];
		else s[2]='T';
	}
}

void WriteMap(){
	register int i,j;
	for(i=1;i<=sL;i++)
	for(j=1;j<=sW;j++){
		switch(Map[i][j].K){
			case 0:Fillc(i,j*5-4,i,j*5,' ');break;
			case 1:Fillc(i,j*5-4,i,j*5,'=');break;
			case 2:Fillc(i,j*5-4,i,j*5,'#');break;
			case 3:
				mp[i][j*5-4].c='[';
				mp[i][j*5].c=']';
				Fillc(i,j*5-3,i,j*5-1,' ');
				break;
			default:break;
		}if(Map[i][j].D!=0&&Map[i][j].K!=2){
			TurnStr(Map[i][j].D);
			Fills(i,j*5-3,j*5-1);
		}else Map[i][j].D=0;
		FillC1(i,j*5-4,i,j*5,Map[i][j].B);
	}
}
void PrintMap(){
	Color(7);
	register int i,j,nC1=7,nC2=0;

	for(i=1;i<=rL;i++)
	for(j=1;j<=rW;j++)
	if(!(mp[i][j]==fmp[i][j])){
		if(mp[i][j].C1!=nC1||mp[i][j].C2!=nC2)
		Color(mp[i][j].C1+mp[i][j].C2),nC1=mp[i][j].C1,nC2=mp[i][j].C2;

		SetPos(i,j);
		putchar(mp[i][j].c);
		fmp[i][j]=mp[i][j];
	}
}
void MakeMp(){
	for(int i=1;i<=rL;i++) for(int j=1;j<=rW;j++)
	Map[i][j].K=0,Map[i][j].D=0,Map[i][j].B=7;
}

inline long long PMod(long long &x){
	long long res=x&63;
	x>>=6;return res;
}

void Pack(){
	register int p=0,i,j;
	long long k1=sL,k2=sW;
	P_s[p++]=PMod(k1)+1;P_s[p++]=PMod(k1)+1;
	P_s[p++]=PMod(k2)+1;P_s[p++]=PMod(k2)+1;
	/*
	for(i=1;i<=sL;i++)
	for(j=1;j<=sW;j++){
		P_s[p++]=(Map[i][j].B<<3+Map[i][j].K<<1)+1;
		k1=Map[i][j].D;
		
		if(k1<0){
			k1=-k1;
			P_s[p-1]++;
		}
		
		for(k2=1;k2<=11;k2++)
		P_s[p++]=PMod(k1)+1;
	}*/P_s[p]='\0';
}

signed main(){
	puts("请调整好窗口大小！");
	puts("CreateMap(Size L/W Tip:at most 100):");
	scanf("%d%d",&sL,&sW);
	rL=sL;
	rW=sW*5;
	Px=1;
	Py=1;
	char c;
	long long d;

	HideCursor();
	MakeMp();
	FrameOutput();
	Move(0,0);
	PrintMap();

	while(1){
		fflush(stdin);
		WriteMap();
		PrintMap();
		
		if(_kbhit()){
			c=_getch();
			d=0;
			
			switch(c){
				case 'w':
				case 'W':Move(-1,0);break;
				case 's':
				case 'S':Move(1,0);break;
				case 'a':
				case 'A':Move(0,-1);break;
				case 'd':
				case 'D':Move(0,1);break;
				case 'q':
				case 'Q':
					SetPos(7,rW+9);
					scanf("%lld",&d);
					SetD(Px,Py,d);
					SetPos(7,rW+9);
					printf("                                                           ");
					break;
				case 'e':
				case 'E':Cls();return 0;break;
				case 'p':
				case 'P':SetK(Px,Py,0);break;
				case 'm':
				case 'M':SetK(Px,Py,1);break;
				case 'r':
				case 'R':SetK(Px,Py,2);break;
				case 'c':
				case 'C':SetK(Px,Py,3);break;
				case 'x':
				case 'X':
					SetPos(7,rW+9);
					scanf("%s",nm);
					pk=fopen(nm,"w");
					Pack();
					fprintf(pk,"%s",P_s);
					SetPos(7,rW+9);
					printf("                                                           ");
					break;
				default:break;
			}
		}Sleep(10);
	}return 0;
}
