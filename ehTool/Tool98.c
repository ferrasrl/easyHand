//   +-------------------------------------------+
//   | TOOL98                                 	 |
//   |                                           |
//   |             by Ferrà Art & Technology 1998 |
//   +-------------------------------------------+

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/ehtool/tool98.h"


void LocalMessage(CHAR *Mess)
{
//	SINT a;
	SINT xWidth,yHeight;
	SINT msx,msy,msb;
//	struct VGAMEMO hlphdl;
	SINT spx=16,spy=12;
	RECT Rect;
	EH_EVENT sEvent;
	RECT rcBox,rcGradient;
	CHAR *pFont="#Arial";
	SINT iAlt=16;

	LONG Style;
	Style=GetWindowLong(WindowNow(),GWL_STYLE);
	SetWindowLong(WindowNow(),GWL_STYLE,	           
				   WS_SYSMENU|
				   WS_MINIMIZEBOX|
				   WS_MAXIMIZE|WS_MAXIMIZEBOX|
				   WS_OVERLAPPEDWINDOW|
				   WS_VISIBLE|
				   WS_SIZEBOX);


	// Guardo se sono sulla parte desiderata
	msx=sys.ms_x,msy=sys.ms_y,msb=sys.ms_b;

	xWidth=font_lenf(Mess,pFont,iAlt,0);
	yHeight=font_altf(pFont,iAlt,0);

	if ((msy+spy+12)>(sys.video_y-1)) spy*=-1;
	if ((msx+spx+xWidth+2)>(sys.video_x-1)) {spx-=(xWidth+18);}

	efx3();
/*
	for (a=0;a<xWidth;a+=1+a)
	{
	 Abox(msx+spx,msy+spy+1,msx+spx+2+a,msy+spy+yHeight+3,1,SET);
	 Aboxp(msx+spx+2,msy+spy+1,msx+spx+4+a,msy+spy+12,12,SET);
	 Abox(msx+spx+1,msy+spy,msx+spx+5+a,msy+spy+yHeight+2,0,SET);
	 ehSleep(10);
	}
*/
	rectFill(&Rect,msx+spx,msy+spy,msx+spx+6+xWidth,msy+spy+yHeight+4);

	//Abox(msx+spx,msy+spy+1,msx+spx+2+xWidth,msy+spy+yHeight+3,1,SET); // ombra
	rectFill(&rcBox,msx+spx,msy+spy,msx+spx+4+xWidth,msy+spy+yHeight+2);
	memcpy(&rcGradient,&rcBox,sizeof(RECT));
	rcGradient.right=rcGradient.left; rcGradient.top--; rcGradient.bottom++;

	BoxGradient(&rcBox,&rcGradient,
				AlphaColor(70,sys.arsColor[12]),
				AlphaColor(196,sys.arsColor[12]));

//	Aboxp(msx+spx,msy+spy,msx+spx+4+xWidth,msy+spy+yHeight+2,12,SET);
	Abox(msx+spx,msy+spy,msx+spx+4+xWidth,msy+spy+yHeight+2,0,SET);
	Adispfm(msx+spx+3,msy+spy+1,0,-1,STYLE_NORMAL,pFont,iAlt,Mess);

	ehSleep(500);
	for (;(msx==sys.ms_x)&&(msy==sys.ms_y);)
	{
		ehWaitEvent(WIN_info[sys.WinInputFocus].hWnd,FALSE,TRUE,&sEvent,TRUE);
	}
	InvalidateRect(WindowNow(),&Rect,TRUE);
	SetWindowLong(WindowNow(),GWL_STYLE,Style); OsEventLoop(5);
	InvalidateRect(WindowNow(),&Rect,TRUE);

}


/*
CHAR *String(SINT Memo)
{
 static SINT Hdl=-1;
 if (Memo==POP) {memoFree(Hdl,"String"); Hdl=-1; return NULL;}
 if (Memo==ALFA) return memoPtr(Hdl);
 if (Hdl!=-1) ehExit("String(): doppia invocazione");
 Hdl=memoAlloc(M_HEAP,Memo,"String");
 return memoPtr(Hdl);
}
*/



