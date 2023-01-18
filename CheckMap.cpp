#include "cons.h"
const int T_SEC = 1000, CHAR_AD = 48;

struct Node
{
	int B, K;
	long long D;
	bool L;
};

struct Picxel
{
	int C1, C2;
	char c;

	bool operator==(Picxel x)
	{
		return C1 == x.C1 && C2 == x.C2 && c == x.c;
	}
};

int sL, sW, rL, rW, Col[15] = {7};
Node Map[80][80];
Picxel mp[505][505], fmp[505][505];
char s[15], nm[505], NUM_s[15] = {0, 'H', 'K', 'W', 'L', 'M', 'Q', 'Y', 'B', 'G', 'T'};
char P_s[200105];
FILE *pk;

inline void FillC1(int x1, int y1, int x2, int y2, int C)
{
	register int i, j;
	for (i = x1; i <= x2; i++)
		for (j = y1; j <= y2; j++)
			mp[i][j].C1 = C;
}
inline void FillC2(int x1, int y1, int x2, int y2, int C)
{
	register int i, j;
	for (i = x1; i <= x2; i++)
		for (j = y1; j <= y2; j++)
			mp[i][j].C2 = C;
}
inline void Fillc(int x1, int y1, int x2, int y2, char c)
{
	register int i, j;
	for (i = x1; i <= x2; i++)
		for (j = y1; j <= y2; j++)
			mp[i][j].c = c;
}
inline void Fills(int x, int y1, int y2)
{
	for (register int i = y1; i <= y2; i++)
		mp[x][i].c = s[i - y1];
}

inline void SetK(int x, int y, int k) { Map[x][y].K = k; }
inline void SetD(int x, int y, long long d) { Map[x][y].D = d; }
inline void SetB(int x, int y, int b) { Map[x][y].B = b; }
inline void SetW(int x, int y, int b, long long d, int k)
{
	Map[x][y].B = b;
	Map[x][y].D = d;
	Map[x][y].K = k;
}

inline void TurnStr(long long x)
{
	for (int i = 0; i < 10; i++)
		s[i] = ' ';
	int f = 0, tw, gw;

	if (x < 0)
	{
		x = -x;
		s[0] = '-';

		while (x / 100 != 0)
		{
			x /= 10;
			f++;
		}

		s[0] = '-';
		tw = x / 10;
		gw = x % 10;

		if (tw == 0)
			s[1] = gw + 48;
		else
		{
			s[1] = tw + 48;
			if (f == 0)
				s[2] = gw + 48;
			else if (f < 10)
				s[2] = NUM_s[f];
			else
				s[2] = 'T';
		}
	}
	else
	{
		while (x / 1000 != 0)
		{
			x /= 10;
			f++;
		}

		gw = x % 10;
		x /= 10;
		tw = x % 10;
		x /= 10;

		if (x != 0)
			s[0] = x + 48;
		if (tw != 0 || x != 0)
			s[1] = tw + 48;

		if (f == 0)
			s[2] = gw + 48;
		else if (f < 10)
			s[2] = NUM_s[f];
		else
			s[2] = 'T';
	}
}

void WriteMap()
{
	register int i, j;
	for (i = 1; i <= sL; i++)
		for (j = 1; j <= sW; j++)
		{
			switch (Map[i][j].K)
			{
			case 0:
				Fillc(i, j * 5 - 4, i, j * 5, ' ');
				break;
			case 1:
				Fillc(i, j * 5 - 4, i, j * 5, '=');
				break;
			case 2:
				Fillc(i, j * 5 - 4, i, j * 5, '#');
				break;
			case 3:
				mp[i][j * 5 - 4].c = '$';
				mp[i][j * 5].c = '$';
				Fillc(i, j * 5 - 3, i, j * 5 - 1, ' ');
				break;
			case 4:
				mp[i][j * 5 - 4].c = '[';
				mp[i][j * 5].c = ']';
				Fillc(i, j * 5 - 3, i, j * 5 - 1, ' ');
				break;
			default:
				break;
			}
			if (Map[i][j].D != 0 && Map[i][j].K != 2)
			{
				TurnStr(Map[i][j].D);
				Fills(i, j * 5 - 3, j * 5 - 1);
			}
			else
				Map[i][j].D = 0;
			FillC1(i, j * 5 - 4, i, j * 5, Col[Map[i][j].B]);
			FillC2(i, j * 5 - 4, i, j * 5, Map[i][j].L * 16);
		}
}
void PrintMap()
{
	Color(7);
	register int i, j, nC1 = 7, nC2 = 0;

	for (i = 1; i <= rL; i++)
		for (j = 1; j <= rW; j++)
			if (!(mp[i][j] == fmp[i][j]))
			{
				if (mp[i][j].C1 != nC1 || mp[i][j].C2 != nC2)
					Color(mp[i][j].C1 + mp[i][j].C2), nC1 = mp[i][j].C1, nC2 = mp[i][j].C2;

				SetPos(i, j);
				putchar(mp[i][j].c);
				fmp[i][j] = mp[i][j];
			}
}

void DePack()
{
	register int i, j, k = 4;
	int f, p = 0;

	for (; P_s[p] != '\0'; p++)
		P_s[p] -= CHAR_AD;

	sL = (P_s[1] << 6) + P_s[0];
	sW = (P_s[3] << 6) + P_s[2];
	rL = sL;
	rW = sW * 5;
	for (i = 1; i <= sL; i++)
		for (j = 1; j <= sW; j++)
		{
			Map[i][j].B = P_s[k++];
			bool f = P_s[k] & 1;
			P_s[k] >>= 1;
			Map[i][j].L = P_s[k] & 1;
			P_s[k] >>= 1;
			Map[i][j].K = P_s[k++];
			Map[i][j].D = 0;

			for (p = 10; p >= 0; p--)
				Map[i][j].D = (Map[i][j].D << 6) + P_s[k + p];
			k += 11;
			Map[i][j].D = f ? (-Map[i][j].D) : Map[i][j].D;
		}
}

signed main()
{
	puts("������ô��ڴ�С��");
	printf("CheckMap(File name):");
	scanf("%s", nm);
	pk = fopen(nm, "r");
	fflush(pk);
	fscanf(pk, "%s", P_s);
	fclose(pk);
	Cls();
	DePack();
	HideCursor();
	WriteMap();
	PrintMap();
	system("pause");
	return 0;
}
