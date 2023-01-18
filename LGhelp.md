# LocalGen Code Help

## Before all...

All contributors who contributed to this project should write the functions, variables and anythings else they wrote here.

## List

> `LGcons.hpp`  
> > Files included:
> >
> > ```cpp
> > #include <cstdio>
> > #include <windows.h>
> > #include <conio.h>
> > ```
> >
> > Functions:
> >
> > ```cpp
> > // clear the full window: too slow, don't use!
> > inline void clearance();
> > // clear the line (only the chars after the cursor)
> > inline void clearline();
> >
> > // make the cursor go to a specticular place in the screen
> > inline void gotoxy(int x,int y);
> > // make the cursor move n lines up
> > inline void curup(int c=1);
> > // make the cursor move n lines down
> > inline void curdown(int c=1);
> > // make the cursor move n chars right
> > inline void curright(int c=1);
> > // make the cursor move n chars left
> > inline void curleft(int c=1);
> > 
> > // hide cursor
> > inline void hideCursor();
> > // show cursor
> > inline void showCursor();
> > 
> > // attr init: without this will lead to errors
> > inline int initattr();
> > 
> > // foreground color
> > inline void setfcolor(int RGB);
> > inline void setfcolor(int R,int G,int B);
> > // background color
> > inline void setbcolor(int RGB);
> > inline void setbcolor(int R,int G,int B);
> > 
> > // print chars with underline
> > inline void underline();
> > 
> > // reset text attributes
> > inline void resetattr();
> > ```
> >
> `LGmaps.hpp`
> > Files included:
> >
> > ```cpp
> > #include <string>
> > using std::string;
> > ```
> >
> > Structures:
> >
> > ```cpp
> > struct Block {
> >     int team; /* the team who holds this block */
> >     int type; /* the block's type: 0->plain, 1->swamp, 2->mountain, 3->general, 4->city */
> >     long long army; /* count of army on this block */
> > };
> > struct teamS {
> >     string name; /* team name */
> >     int color; /* team color */
> > };
> > ```
> >
> > Variables:
> > 

## ** Bot返回值 **
```
-1~8
-1：不动
0：回家
1-4：带兵上左下右
5-8：不带兵上左下右（可穿墙）
```
