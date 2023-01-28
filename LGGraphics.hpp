#include<bits/stdc++.h>
#include<graphics.h>
using namespace std;

namespace imageOperation {
	void zoomImage(PIMAGE& pimg, int zoomWidth, int zoomHeight) {
		//pimg为空，或目标图像大小和原图像一样，则不用缩放，直接返回
		if ((pimg == NULL) || (zoomWidth == getwidth(pimg) && zoomHeight == getheight(pimg)))
			return;

		PIMAGE zoomImage = newimage(zoomWidth, zoomHeight);
		putimage(zoomImage, 0, 0, zoomWidth, zoomHeight, pimg, 0, 0, getwidth(pimg), getheight(pimg));
		delimage(pimg);

		pimg = zoomImage;
	}
}

namespace LGGraphics {
	PIMAGE pimg[5];
	
	struct mapData
	{
		int heightPerBlock;
		int widthPerBlock;
		int height,width;
	}mapDataStore;
	
	void inputMapData(int a,int b,int c,int d)
	{
		mapDataStore.heightPerBlock=a;
		mapDataStore.widthPerBlock=b;
		mapDataStore.height=c;
		mapDataStore.width=d;
		return ;
	}
	
	void init() {
		setfont(-2, 0, "宋体");
		pimg[1] = newimage();
		getimage(pimg[1], "img/city.png");
		imageOperation::zoomImage(pimg[1], mapDataStore.heightPerBlock, mapDataStore.widthPerBlock);
		pimg[2] = newimage();
		getimage(pimg[2], "img/crown.png");
		imageOperation::zoomImage(pimg[2], mapDataStore.heightPerBlock, mapDataStore.widthPerBlock);
		pimg[3] = newimage();
		getimage(pimg[3], "img/mountain.png");
		imageOperation::zoomImage(pimg[3], mapDataStore.heightPerBlock, mapDataStore.widthPerBlock);
		pimg[4] = newimage();
		getimage(pimg[4], "img/swamp.png");
		imageOperation::zoomImage(pimg[4], mapDataStore.heightPerBlock, mapDataStore.widthPerBlock);
		initgraph(mapDataStore.height*mapDataStore.heightPerBlock,mapDataStore.width*mapDataStore.widthPerBlock);
		setbkcolor(WHITE);
		setbkcolor_f(WHITE);
		cleardevice();
	}
}
