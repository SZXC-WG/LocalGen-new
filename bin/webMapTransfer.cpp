#include <bits/stdc++.h>
using namespace std;

using uint = unsigned;
using ll = long long;
using ull = unsigned long long;
using pii = pair<int,int>;
using pll = pair<ll,int>;

struct Block {
	int player; /* the team who holds this block */
	int type; /* the block's type: 0->plain, 1->swamp, 2->mountain, 3->general, 4->city */
	long long army;  /* count of army on this block */
	bool lit; /* whether the block is lighted(lit) */
};

const int LEN_ZIP = 100005, CHAR_AD = 48, LEN_MOVE = 30005, replaySorter = 2000;
char strZip[LEN_ZIP];

int mapH, mapW;
Block gameMap[505][505];

inline long long PMod(long long& x) {
	long long res = x & 63;
	x >>= 6;
	return res;
}

void Zip() {
	register int p = 0, i, j;
	long long k1 = mapH, k2 = mapW;
	strZip[p++] = PMod(k1) + CHAR_AD;
	strZip[p++] = PMod(k1) + CHAR_AD;
	strZip[p++] = PMod(k2) + CHAR_AD;
	strZip[p++] = PMod(k2) + CHAR_AD;

	for(i = 1; i <= mapH; i++)
		for(j = 1; j <= mapW; j++) {
			strZip[p++] = gameMap[i][j].player + CHAR_AD;
			strZip[p] = (gameMap[i][j].type << 2) + (gameMap[i][j].lit << 1);
			k1 = gameMap[i][j].army;

			if(k1 < 0) {
				k1 = -k1;
				strZip[p++] += CHAR_AD + 1;
			} else
				strZip[p++] += CHAR_AD;

			for(k2 = 1; k2 <= 8; k2++)
				strZip[p++] = PMod(k1) + CHAR_AD;
		}
	strZip[p] = '\0';
}

signed main() {
	string filename;
	cout<<"input file name: "; cin>>filename;
	ifstream ifs(filename);
	ifs>>mapH>>mapW;
	for(int i=1; i<=mapH; ++i) for(int j=1; j<=mapW; ++j) ifs>>gameMap[i][j].type;
	for(int i=1; i<=mapH; ++i) for(int j=1; j<=mapW; ++j) ifs>>gameMap[i][j].army;
	for(int i=1; i<=mapH; ++i) for(int j=1; j<=mapW; ++j) {
		int t; ifs>>t;
		gameMap[i][j].lit = t;
	}
	ifs.close();
	cout<<mapH<<" "<<mapW<<endl;
	Zip();
	ofstream ofs("output.lg");
	ofs<<strZip<<endl;
	ofs.close();
	return 0;
}

