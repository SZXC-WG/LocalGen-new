#include <iostream>
using std::cout;

// clear the full window: too slow, don't use!
inline void clearance() { cout<<"\033[2J"; }

inline void gotoxy(int x,int y) {cout<<("\033["s+to_string(y)+":"s+to_string(x)+"H"s);}
