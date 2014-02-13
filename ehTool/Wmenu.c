//   ---------------------------------------------
//   | WMENU  Utilità di richiesta tipo W95      |
//   |        proprietà oggetto                  |
//   |                                           |
//   |                                           |
//   |             17 Aprile 2.0                 |
//   |             by Ferrà Art & Technology 1998 |
//   ---------------------------------------------

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/ehtool/wmenu.h"

#define WSETBG 5
// ------------------------------------------------
//  Proprietà dell'oggetto                        !
// ------------------------------------------------
static void Lbox3d(SINT x1,SINT y1,SINT x2,SINT y2);
static SINT Ldisp3Dfm_h(SINT px,SINT py,
							 SINT hdl,SINT nfi,CHAR *str);


// Versione Window
#ifdef _WIN32

SINT WMenu(CHAR * pszTitle,CHAR * pszSubTitle,struct WMENU Menu[])
{
	CHAR MFont[]="SMALL F";
	SINT MFhdl;
	SINT MNfi=3,MAlt;
	SINT NLinee=0;
	SINT OPx,OPy;
	SINT Px,Py;
	SINT Px2,Py2;
	SINT Dx=0,Dy=0;
	SINT SpesX=8;
	SINT SpesY=4;
	SINT a,rt;
	SINT MenSel=-1,MenOld=-1;
	SINT Zy=5;
	SINT rx,ry;
	SINT Ultx,Ulty;
	SINT win;
	BOOL TLock;
	CHAR serv[40];
	EH_EVENT sEvent;
	POINT Point;
	RECT rcBox;

	struct OBJ obj[]={
	// TipObj    Nome    Sts Lck   x   y CL1 CL2 TESTO
	{O_ZONA,"--" ,OFF,ON , -1,-1,-1,-1,"",""},
	{STOP}
	};

	// struct VGAMEMO vga;

	rx=0;ry=0;
	// if (ft) {rx=relwx;ry=relwy;}

	// Conta le linee
	for (NLinee=0;Menu[NLinee].Flag!=STOP;NLinee++)
		{a=font_lenf(Menu[NLinee].Desc,MFont,MNfi,0);
		 if (a>Dx) Dx=a;
		}

	a=font_lenf(pszTitle,"#Arial",18,STYLE_BOLD)+font_lenf(pszSubTitle,"SMALL F",3,STYLE_NORMAL)+2;
	if (Dx<a) Dx=a;

	Dx+=30;
	MFhdl=fontFind(MFont,MNfi,0);//&MFmax,&MFhdl);
	MAlt=font_alt(MFhdl)+2;

	Dy=(NLinee+1)*MAlt;
	Dx+=SpesX*2+20; Dy+=SpesY*2;
	Dy+=Zy;

	GetCursorPos(&Point); // Leggo la posizione del cursore
	Px=Point.x-5; Py=Point.y-5;
	
	Dx+=4; Dy+=4;
	if (Px+Dx>(sys.sizeDesktop.cx-1)) Px=sys.sizeDesktop.cx-1;
	if (Py+Dy>(sys.sizeDesktop.cy-1)) Py=sys.sizeDesktop.cy-Dy-1;

	OPx=Px; OPy=Py;
	TLock=sys.fTitleLock; sys.fTitleLock=TRUE;
	win=win_openEx(OPx,OPy,"WMENU",
					Dx,Dy,
					-1,// Colore di sfondo standard
					OFF,
					WS_EX_WINDOWEDGE,
					WS_POPUP| // Fa' in modo che le child windows non vengano sovrascritte
					//WS_BORDER|
					//WS_THICKFRAME|
					WS_VISIBLE,
					TRUE,
					NULL); // Non fare l'emulazione Dos

	sys.fTitleLock=TLock;
	xy_rel(0,0);
	box3d(0,0,Dx-1,Dy-1,5);
	box3d(1,1,Dx-2,Dy-2,0);
	if (sys.fSoundEfx) PlayMySound("Efx1");
 
/*
#ifdef _WIN32
 efx3();
 for (a=0;a<Dy;a+=2+(a/10))
 {
	//MoveWindow(WIN_info[0].hWnd,0,0,sys.video_x,a,TRUE);
	MoveWindow(WIN_info[win].hWnd,OPx,OPy,Dx,a,TRUE);
	UpdateWindow(WIN_info[0].hWnd);
	//PauseActive(1);
	ehSleep(10);
 }
 MoveWindow(WIN_info[win].hWnd,OPx,OPy,Dx,Dy,TRUE);
#endif
 */

	//SetCapture(WIN_info[win].hWnd);
	ehSetCapture(WindowNow());
	Px=2; Py=2;	Px2=Px+Dx-3; Py2=Py+Dy-3;

//	Tboxp(Px+1,Py+1,Px2-3,Py+1+MAlt+2,GetSysColor(COLOR_ACTIVECAPTION),SET);
	rectFill(&rcBox,Px+1,Py+1,Px2-3,Py+1+MAlt+2);
	BoxGradient(&rcBox,NULL,
				AlphaColor(255,sys.ColorSelectBack),
				AlphaColor(255,ColorLum(sys.ColorSelectBack,-40)));


	a=Adispfm(Px+6,Py+1,15,-1,STYLE_BOLD,"#Arial",18,pszTitle);

	if (!strEmpty(pszSubTitle)) {sprintf(serv," %s",pszSubTitle); Adispfm(Px+10+a,Py+4,14,-1,STYLE_NORMAL,"SMALL F",3,serv);}
	Px+=SpesX; Py+=SpesY+MAlt+Zy;

	rt=-1;
	obj_open(obj);
// 	Amgz_on(Px-SpesX,Py-(SpesY+MAlt+Zy),Px2,Py2,12,4,-1,"MS03");
	mouse_graph(0,0,"MS03");
	Ultx=-10; Ulty=-10;	MenSel=0; MenOld=-400;
	while(TRUE)
	{
		SINT x,y;
		BOOL bExit=FALSE;
		eventGet(&sEvent);
		switch (sEvent.iEvent)
		{
			case EE_LBUTTONUP:
				if (MenSel==-1) {bExit=TRUE; break;}
				if (!Menu[MenSel].Flag) {efx2(); break;}
				rt=(SINT) Menu[MenSel].cmd[0];
				if (rt==0) rt=-1; // Linea divisore
				bExit=TRUE;
				break;

			case EE_CHAR:
				if (sEvent.iTasto==27) bExit=TRUE;
				break;

			case EE_RBUTTONDOWN:
				bExit=TRUE;
				break;
		}
		if (bExit) break;

		// Controllo se il mouse si è spostato
			x=sys.ms_x-rx; y=sys.ms_y-ry;
			if ((Ultx!=x)||(Ulty!=y))
			 {
				if (Ultx!=-10)
				{
				 if ((x>Px)&&(x<Px2)&&(y>Py)&&(y<Py2))
				 {MenSel=((y+3)-Py)/MAlt;} else MenSel=-1;
				}
				Ultx=x;Ulty=y;
			 }

		if (key_press2(_FDN))
			{MenSel++;
			 if (Menu[MenSel].Desc[0]=='-') MenSel++;
			 if (MenSel>(NLinee-1)) MenSel=0;}

		if (key_press2(_FUP))
			{MenSel--;
			 if (MenSel<0) MenSel=NLinee-1;
			 if (Menu[MenSel].Desc[0]=='-') MenSel--;
			 }

		if (MenSel!=MenOld)
		{ 
			//SINT ColBg,ColCar;
			EH_COLOR cText,cBackground;
			for (a=0;a<NLinee;a++)
			{
				//if (a==MenSel) {ColBg=WSETBG; ColCar=3;} else {ColBg=2; ColCar=0;}
				if (a==MenSel)
				{
					cText=sys.ColorSelectText;//sys.ColorButtonText;
					cBackground=sys.ColorSelectBack;
				}
				else
				{
					cText=sys.ColorButtonText;
					cBackground=sys.ColorBackGround;
				}

				//if (!Menu[a].Flag&&(ColBg==3)) ColBg=2;

				//
				// Separatore
				//
				if (Menu[a].Desc[0]=='-')
				{
					SINT y;
					y=Py+(a*MAlt)+(MAlt/2)-2;
					Tline(Px,y,Px2-SpesX,y,sys.Color3DShadow,SET);
					Tline(Px,y+1,Px2-SpesX,y+1,sys.Color3DHighLight,SET);
				}
				else
				{
					SINT z;
					sprintf(serv," %s",Menu[a].Desc);
					if (Menu[a].Flag) 
					{
		                //LONG col1,col2;
						SINT Modo;
						//col1=RealColor(ColCar);
						//if (ColBg==2) col2=sys.ColorBackGround; else col2=RealColor(ColBg);
                        Modo=ModeColor(TRUE);
						z=Adispfm_h(Px,Py+a*MAlt,cText,cBackground,MFhdl,serv);
						Aboxp(Px+z,Py+a*MAlt,Px2-SpesX,Py+(a+1)*MAlt-3,cBackground,SET);
						ModeColor(Modo);
						Tbox(Px-1,Py+a*MAlt-1,Px2-SpesX+1,Py+(a+1)*MAlt-2,cBackground,SET);
					}
				    else
					{
						// DISABILITATO
						 //z=disp3Dfm_h(Px,Py+a*MAlt,MFhdl,MNfi,serv)+3;
						 z=Ldisp3Dfm_h(Px,Py+a*MAlt,MFhdl,MNfi,serv)+3;
						 Tboxp(Px+z,Py+a*MAlt,Px2-SpesX,Py+(a+1)*MAlt-3,sys.ColorBackGround,SET);
						 //Tbox(Px-1,Py+a*MAlt-1,Px2-SpesX+1,Py+(a+1)*MAlt-2,cBackground,SET);
					}
				}
			}
			MenOld=MenSel;
		}
	}

	ehReleaseCapture();
	win_close();
	UpdateWindow(WIN_info[sys.WinWriteFocus].hWnd);
	mgz_ask();
	MouseCursorDefault();
	
	return rt;
}

static void Lbox3d(SINT x1,SINT y1,SINT x2,SINT y2)
{
	Tline(x1,y1,x2,y1,sys.Color3DShadow,SET);
	Tline(x2,y1,x2,y2,sys.Color3DShadow,SET);
	Tline(x1,y1+1,x1,y2,sys.Color3DHighLight,SET);
	Tline(x1,y2,x2-1,y2,sys.Color3DHighLight,SET);
}

static SINT Ldisp3Dfm_h(SINT px,SINT py,SINT hdl,SINT nfi,CHAR *str)
{
    SINT a,Col;
	Col=ModeColor(TRUE);
	Adispfm_h(px,py,sys.Color3DHighLight,-1,hdl,str);
	a=Adispfm_h(px+1,py-1,sys.Color3DShadow,-1,hdl,str);
	ModeColor(Col);
	return a;

}


#else  // DOS VERSIONE
SINT WMenu(CHAR *Titolo,CHAR *SubTit,struct WMENU Menu[])
{
 CHAR MFont[]="SMALL F";
 SINT MFhdl,MFmax;
 SINT MNfi=3,MAlt;
 SINT NLinee=0;
 SINT Px,Py;
 SINT Px2,Py2;
 SINT Dx=0,Dy=0;
 SINT SpesX=8;
 SINT SpesY=4;
 SINT ms,a,rt;
 SINT MenSel=-1,MenOld=-1;
 SINT Zy=5;
 SINT rx,ry;
 SINT Ultx,Ulty;

 CHAR serv[40];
 struct OBJ obj[]={
// TipObj    Nome    Sts Lck   x   y CL1 CL2 TESTO
	{O_ZONA,"--" ,OFF,ON , -1,-1,-1,-1,"",""},
	{STOP}
	};

 struct VGAMEMO vga;

 rx=0;ry=0;
// if (ft) {rx=relwx;ry=relwy;}

 // Conta le linee
 for (NLinee=0;Menu[NLinee].Flag!=STOP;NLinee++)
		{a=font_lenf(Menu[NLinee].Desc,MFont,MNfi);
		 if (a>Dx) Dx=a;
		}

 a=font_lenf(Titolo,"SANS SERIF F",1)+font_lenf(SubTit,"SMALL F",2)+2;
 if (Dx<a) Dx=a;

 Dx+=30;
 fontFind(MFont,&MFmax,&MFhdl);
 MAlt=font_alt(MFhdl,MNfi)+2;

 Dy=(NLinee+1)*MAlt;
 Dx+=SpesX*2+20; Dy+=SpesY*2;
 Dy+=Zy;

 // Calcola le dimensioni della finestra
 //Px=OGT_buf1->px1+5;Py=OGT_buf1->py1+5;
 Px=sys.ms_x-rx;Py=sys.ms_y-ry;
 if (Px+Dx>(sys.video_x-1)) Px=sys.video_x-Dx-1;
 if (Py+Dy>(sys.video_y-1)) Py=sys.video_y-Dy-1;
 Px2=Px+Dx-1; Py2=Py+Dy-1;

 // Salva lo schermo e scrive la finestra
 Avideo_bck(Px,Py,Px2,Py2,RAM_AUTO,&vga);
 Aboxp(Px,Py,Px2,Py2,2,SET);   
 Aline(Px,Py+1,Px,Py2,0,SET);  
 Aline(Px+1,Py2,Px2-1,Py2,0,SET);
 Aline(Px+1,Py+1,Px2-1,Py+1,15,SET); Aline(Px2-1,Py+1,Px2-1,Py2-1,15,SET);
 Aline(Px+1,Py+2,Px+1,Py2-1,1,SET);  Aline(Px+1,Py2-1,Px2-2,Py2-1,1,SET);

 Aboxp(Px+3,Py+3,Px2-3,Py+3+MAlt,5,SET);
 a=Adispfm(Px+6,Py+1,14,-1,SET,"SANS SERIF F",1,Titolo);
 if (*SubTit)
	{
	 sprintf(serv,"(%s)",SubTit);
	 Adispfm(Px+10+a,Py+6,14,-1,SET,"SMALL F",2,serv);
	}
 Px+=SpesX; Py+=SpesY+MAlt+Zy;

 rt=-1;
//	int  Amgz_on(int x,int y,int x2,int y2,int grp,int sx,int sy,char *ico);
//	int  mgz_off(int x,int y,int x2,int y2);

 obj_open(obj);
 WIN_ult++; // tapullo per i controlli sulle MGZ e HMZ
 //strcpy(WIN_info[WIN_ult].titolo,"PMENU"); // Copia il titolo
 WIN_info[WIN_ult].titolo=""; // Copia il titolo
 Amgz_on(Px-SpesX,Py-(SpesY+MAlt+Zy),Px2,Py2,12,4,-1,"MS03");

 Ultx=-10; Ulty=-10;
 MenSel=0;  MenOld=-400;
 while (TRUE)
 {SINT x,y;

	ms=eventGet(NULL);

	// Controllo se il mouse si è spostato
	x=sys.ms_x-rx; y=sys.ms_y-ry;
	if ((Ultx!=x)||(Ulty!=y))
	 {
		if (Ultx!=-10)
		{
		 if ((x>Px)&&(x<Px2)&&(y>Py)&&(y<Py2))
		 {MenSel=((y+3)-Py)/MAlt;} else MenSel=-1;
		}
		Ultx=x;Ulty=y;
	 }

	if (key_press2(_FDN))
		{MenSel++;
		 if (Menu[MenSel].Desc[0]=='-') MenSel++;
		 if (MenSel>(NLinee-1)) MenSel=0;
		}

	if (key_press2(_FUP))
		{MenSel--;
		 if (MenSel<0) MenSel=NLinee-1;
		 if (Menu[MenSel].Desc[0]=='-') MenSel--;
		}

	if (MenSel!=MenOld)
	{ SINT ColBg,ColCar;
	// Stampa il menu
		for (a=0;a<NLinee;a++)
		{if (a==MenSel) {ColBg=WSETBG;ColCar=3;} else {ColBg=2; ColCar=0;}
		 if (!Menu[a].Flag&&(ColBg==3)) ColBg=2;
		 if (Menu[a].Desc[0]=='-')
		 {SINT y;
			y=Py+(a*MAlt)+(MAlt/2)-2;
			Aline(Px,y,Px2-SpesX,y,1,SET);
			Aline(Px,y+1,Px2-SpesX,y+1,15,SET);
		 }
		 else
			{SINT z;
			 sprintf(serv," %s",Menu[a].Desc);
//			 if(Menu[a].Flag) z=dispfm_h(Px,Py+a*MAlt,ColCar,ColBg,SET,MFhdl,MNfi,serv);
			 if(Menu[a].Flag) {z=Adispfm_h(Px,Py+a*MAlt,ColCar,ColBg,SET,MFhdl,MNfi,serv);
							   //if (ColBg==WSETBG) z=dispfm_h(Px+1,Py+a*MAlt,ColCar,-1,SET,MFhdl,MNfi,serv);
							   Aboxp(Px+z,Py+a*MAlt,Px2-SpesX,Py+(a+1)*MAlt-3,ColBg,SET);
								}
								else
												{
												 //z=disp3Dfm_h(Px,Py+a*MAlt,MFhdl,MNfi,serv)+3;
												 z=Ldisp3Dfm_h(Px,Py+a*MAlt,MFhdl,MNfi,serv)+3;
												 Aboxp(Px+z,Py+a*MAlt,Px2-SpesX,Py+(a+1)*MAlt-3,2,SET);
												}
			 /*
			 boxp(Px+z,Py+a*MAlt,Px2-SpesX,Py+(a+1)*MAlt-3,ColBg,SET);
			 if (ColBg==3) box(Px-1,Py+a*MAlt-1,
										 Px2-SpesX+1,Py+(a+1)*MAlt-2,5,SET);
										 else
										 box(Px-1,Py+a*MAlt-1,
										 Px2-SpesX+1,Py+(a+1)*MAlt-2,ColBg,SET);
				 */
			 if (ColBg==WSETBG) Lbox3d(Px-1,Py+a*MAlt-1,
													Px2-SpesX+1,Py+(a+1)*MAlt-2);
													else
													Abox(Px-1,Py+a*MAlt-1,
													Px2-SpesX+1,Py+(a+1)*MAlt-2,ColBg,SET);


			}
		}
		MenOld=MenSel;
	}

	// Esce senza selezionare
	if ((sys.sLastEvent.iEvent==EE_LBUTTONDOWN)||key_press(CR))
	 {
		/*
		if ((sys.ms_x<Px)||
				(sys.ms_x>Px2)||
				(sys.ms_y<Py)||
				(sys.ms_y<Py2)) break;
	 */
//	 key_clean();;
	 if (MenSel==-1) break;

	 if ((sys.sLastEvent.iEvent==EE_LBUTTONDOWN))
	 {
	 while(TRUE)
		 {ms=eventGet(NULL);
			//if (key_press(ESC)) break;
			if (sys.sLastEvent.iEvent==EE_RBUTTONDOWN) break;
			}
	 }

	 if (Menu[MenSel].Flag==OFF) {efx2(); continue;}
	 rt=(SINT) Menu[MenSel].cmd[0];
	 if (rt==0) rt=-1; // Linea divisore
	 break;
	 }

	if (key_press(ESC)) break;
	if (sys.sLastEvent.iEvent==EE_RBUTTONDOWN) {rt=-2;break;}
}

 video_rst(&vga);
 //Amgz_off(Px,Py,Px2,Py2);
 Amgz_off(Px-SpesX,Py-(SpesY+MAlt+Zy),Px2,Py2);

 mgz_ask();
 obj_close();
 WIN_ult--; // tapullo per i controlli sulle MGZ e HMZ

 return rt;
}


static void Lbox3d(SINT x1,SINT y1,SINT x2,SINT y2)
{
	Aline(x1,y1,x2,y1,1,SET);
	Aline(x2,y1,x2,y2,1,SET);
	Aline(x1,y1+1,x1,y2,15,SET);
	Aline(x1,y2,x2-1,y2,15,SET);
}

static SINT Ldisp3Dfm_h(SINT px,SINT py,
							 SINT hdl,SINT nfi,CHAR *str)
{
	Adispfm_h(px,py,15,-1,SET,hdl,nfi,str);
	return Adispfm_h(px+1,py-1,1,-1,SET,hdl,nfi,str);
}


#endif
