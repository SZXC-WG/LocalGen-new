#include <cstdio>
#include <windows.h>
#include <conio.h>

// clear the full window: too slow, don't use!
inline void clearance() { fputs("\033[2J",stdout); }
// clear the line (only the chars after the cursor)
inline void clearline() { fputs("\033[K",stdout); }

// make the cursor go to a specticular place in the screen
inline void gotoxy(int x,int y) { printf("\033[%d:%dH",y,x); }
// make the cursor move n lines up
inline void curup(int c) { printf("\033[%dA",c); }
// make the cursor move n lines down
inline void curdown(int c) { printf("\033[%dB",c); }
// make the cursor move n chars left
inline void curleft(int c) { printf("\033[%dC",c); }
// make the cursor move n chars right
inline void curright(int c) { printf("\033[%dD",c); }

// hide cursor
inline void hideCursor() { fputs("\033[?25l",stdout); }
// show cursor
inline void showCursor() { fputs("\033[?25h",stdout); }

// color init: without this will lead to errors
inline int initcolor() {
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return GetLastError();
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) return GetLastError();
    dwMode |= 0x0004;
    if (!SetConsoleMode(hOut, dwMode)) return GetLastError();
    return 0;
}
// foreground color
inline void setfcolor(int R,int G,int B) { printf("\033[38;2;%d;%d;%dm",R,G,B); }
// background color
inline void setbcolor(int R,int G,int B) { printf("\033[48;2;%d;%d;%dm",R,G,B); }

// print chars with underline
inline void underline() { fputs("\033[4m"); }

// reset text attributes
inline void resetattr() { fputs("\033[0m",stdout); }

