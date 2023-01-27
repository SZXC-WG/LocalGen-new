#include<conio.h>

int getch_cons()
{
	return _getch();
}

bool kbhit_cons()
{
	return _kbhit();
}
