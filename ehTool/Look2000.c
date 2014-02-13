//   ---------------------------------------------
//   | Look2000                                  |
//   | Gestisce il Look 98                       |
//   |                                           |
//   |             Created by Tassistro 13/1/98  |
//   |             Aggiunto ICONEB      23/5/98  |
//   |             by Ferr… Art & Tecnology 1998 |
//   ---------------------------------------------
#include "\ehtool\include\ehsw_i.h"

#ifdef _WIN32
#define L98H  sys.Color3DHighLight
#define L98L  sys.Color3DShadow
#define L98B  sys.ColorBackGround
#define L98V  RGB(0,255,192)
#define L98N  RGB(0,0,0)
#define LBOX  Tbox
#define LLINE Tline
#else
#define L98H 15
#define L98N 0
#define L98L 1
#define L98B 2
#define L98V 12
#define LBOX  box
#define LLINE line
#endif


static SINT Aperto=OFF;

void Look98FT(SINT cmd,void *str);
void Look98(void)
{
 if (!Aperto) FTIME_on(Look98FT,0);
 Aperto=ON;
 OBJ_FlatLook=TRUE;
}

void Look98Stop(void)
{
 if (Aperto) FTIME_off(Look98FT);
 Aperto=OFF;
 OBJ_FlatLook=FALSE;
}




void Look98FT(SINT cmd,void *str)
{
 static SINT UltObj=-1;
 static SINT MCap=0;
 static SINT cnt=0;
 static SINT UltimaWin=-2;
// static SINT UltimaObj=-2;
 static SINT FlagPress=OFF;
 struct OBJ *poj;
 struct OBJ_ALO *alo=OBJ_info[OBJ_ult].alo;
// static SINT NOBJ=OFF;
 SINT x1,x2,y1,y2;

#ifdef _WIN32
 static POINT LastWinDim;
#endif

 if (cmd==255) {strcpy(str,"Look98"); return;}
 if (OBJ_ult<0) return;

 // Cambio di finestra
 if (WIN_ult!=UltimaWin)
		{
		 //efx2();
		 UltimaWin=WIN_ult;
		 MCap=0; UltObj=-1;

#ifdef _WIN32
		 LastWinDim.x=WIN_info[WIN_ult].CLx;
		 LastWinDim.y=WIN_info[WIN_ult].CLy;
#endif
		}
#ifdef _WIN32
        else
		{
		 if ((WIN_info[WIN_ult].CLx!=LastWinDim.x)||
			 (WIN_info[WIN_ult].CLy!=LastWinDim.y)) 
		 {
		  UltimaWin=WIN_ult;
		  MCap=0; UltObj=-1;
		  LastWinDim.x=WIN_info[WIN_ult].CLx;
		  LastWinDim.y=WIN_info[WIN_ult].CLy;
		 }
		}

#endif 

// printf("%d %d %d\n",WIN_ult,WIN_info[WIN_ult].obj_num,OBJ_ult);
 // La nuova finestra non ha oggetti
#ifdef _WIN32
 if ((UltimaWin>-1)&&(WIN_info[WIN_ult].obj_num>=OBJ_ult)) return;
#else
 if ((UltimaWin>-1)&&(WIN_info[WIN_ult].obj_num>=OBJ_ult)) return;
#endif

 /*
 if (WIN_ult!=UltimaWin)
		{
		 //efx2(); printf("Cambio WIN");
		 //efx2();
		 //printf("%d<%d",WIN_ult,UltimaWin);
		 if (WIN_ult!=UltimaWin)
			 {MCap=0; UltimaObj=-2; UltObj=-1; NOBJ=OFF;}
			 else
			 {NOBJ=ON;}
			UltimaWin=WIN_ult;
		 }
//WIN_info[WIN_ult].obj_num
 // Cambio di oggetti
 if (OBJ_ult!=UltimaObj)
		{
		 //efx2(); printf("Cambio Obj");
		 UltimaObj=OBJ_ult; MCap=0; UltObj=-1;
		 NOBJ=OFF;
		 }
	 */

 // Non ci sono oggetti
 //if (NOBJ) return;
 //sonic(1000,1,2,3,4,5);
 if (cnt>=OBJ_info[OBJ_ult].numobj) {cnt=0; MCap++; return;}
 //win_infoarg("%d %d %d",cnt,OBJ_ult,OBJ_info[OBJ_ult].numobj);

 // ---------------------------------------------
 // Fuori dall'oggetto                          |
 // ---------------------------------------------

 if ((UltObj!=cnt)&&(MCap<3))
		 {poj=alo[cnt].poj;
			x1=alo[cnt].x1;
			y1=alo[cnt].y1;
			x2=alo[cnt].x2;
			y2=alo[cnt].y2;

			//if (UltObj!=-1)
			//{
			 //sonic(1200,1,2,3,4,5);

			 switch (poj->tipo)
			 {
				case O_PMENU:
				 LBOX(x1+2,y1+1,x2,y2-1,L98B,SET);
				 break;

				case O_SCROLL:
				case O_SCRDB:
				 LBOX(x1,y1,x2,y2,L98L,SET);  //5
				 break;
/*
#ifdef _WIN32
				case OW_SCR:
				case OW_SCRDB:
				 LBOX(poj->px,poj->py,
					 poj->px+poj->col1-1,
					 poj->py+poj->col2-1,L98L,SET);
				     break;
#endif
					 */

#ifdef _WIN32
				case OW_LIST:
				case OW_LISTP:
					  if (poj->status==OFF)
					  {
//				       LBOX(x1,y1,x2,y2,L98H,SET); LBOX(x1+1,y1+1,x2-1,y2-1,L98L,SET); // +
//				       LBOX(x1,y1,x2,y2,L98L,SET); LBOX(x1+1,y1+1,x2-1,y2-1,L98B,SET); LBOX(x1+1,y1+1,x2-1,y1+1,L98H,SET); LBOX(x1,y2-1,x2,y2-1,L98L,SET); 
//				       LBOX(x1,y1,x2,y2,L98L,SET); box3d(x1+1,y1+1,x2-1,y2-1,0); // ++
					   
					   LBOX(x1,y1,x2,y1,L98B,SET); LBOX(x1,y2,x2,y2,L98B,SET);  // Linea sopra sotto
					   LBOX(x1,y1+1,x2,y2-1,L98L,SET); // Quadrato L
					   if (poj->lock) LBOX(x1+1,y1+2,x2-1,y2-2,L98H,SET); // Quadrato H
									  else
									  LBOX(x1+1,y1+2,x2-1,y2-2,L98B,SET); // Quadrato H
					   LBOX(x2-GetSystemMetrics(SM_CXHSCROLL)+2,y1+3,x2-2,y2-3,L98B,SET);
					  }
				      break;
#endif

				case O_LIST:
				 if (poj->lock)
				 {
					LBOX(x1,y1,x2-1,y2-1,L98L,SET);  //5
				 }
				 break;

				case O_ICONEA:
				case O_IMARKA:
				case O_IRADIA:
					 if (!poj->status)
					 {
						LBOX(x1+2,y1+2,x2,y2-1,L98B,SET);
						LBOX(x1+1,y1+2,x2-1,y2,L98B,SET);
						LBOX(x1+1,y1+2,x2,y2,L98B,SET);
						LLINE(x1+1,y1+1,x2,y1+1,L98H,SET);
						//LBOX(x1,y1,x2+1,y2+1,L98L,SET);
						LBOX(x1,y1,x2,y2,L98L,SET);
					 }
					 break;

				case O_ICONEB:
				case O_IMARKB:
				case O_IRADIB:
					 if (!poj->status)
					 {
						LBOX(x1,y1,x2+1,y2+1,L98B,SET);
						LBOX(x1+1,y1+1,x2,y2,L98B,SET);
						Tboxp(x1+2,y1+1,x2-2,y1+3,L98B,SET);
						Tboxp(x1+2,y2-3,x2-2,y2-1,L98B,SET);
					 }
					 break;

				case O_KEYDIM:
					 //LBOX(x1+2,y1+1,x2-1,y2-1,2,SET);
					 //LBOX(x1+3,y1+2,x2-2,y2-2,2,SET);
					 if (!poj->status)
					 {
					 LBOX(x1+1,y1+2,x2-1,y2-1,L98B,SET);
					 LBOX(x1+2,y1+2,x2-2,y2-2,L98B,SET);
					 LLINE(x1+1,y1+1,x2-1,y1+1,L98H,SET);
					 LBOX(x1,y1,x2,y2,L98L,SET);
					 }
					 break;
			 }
		 }

#ifdef _WIN32
 /*
 if ((sys.ms_x>WIN_info[sys.WinInputFocus].clp_x2)||
		 (sys.ms_x<WIN_info[sys.WinInputFocus].clp_x1)||
		 (sys.ms_y>WIN_info[sys.WinInputFocus].clp_y2)||
		 (sys.ms_y<WIN_info[sys.WinInputFocus].clp_y1))
		 {if (UltObj!=-1) goto reset; goto avanti;}
*/
    if (WIN_info[sys.WinInputFocus].ClipNum) 
	{
	 struct CLIP Clip;
     WinGetClip(&Clip,sys.WinInputFocus,TRUE);
     if ((sys.ms_x>Clip.x2)||
		 (sys.ms_x<Clip.x1)||
		 (sys.ms_y>Clip.y2)||
		 (sys.ms_y<Clip.y1))
	 {
		  if (UltObj!=-1) goto reset; 
	      goto avanti;
	 }
	
	}
#else
 if ((sys.ms_x>sys.clp_x2)||
		 (sys.ms_x<sys.clp_x1)||
		 (sys.ms_y>sys.clp_y2)||
		 (sys.ms_y<sys.clp_y1))
		 {if (UltObj!=-1) goto reset; goto avanti;}
#endif
 // ---------------------------
 // Sopra l'oggetto           |
 // ---------------------------

 if ((sys.ms_x>=(alo[cnt].x1+relwx))&&
		 (sys.ms_y>=(alo[cnt].y1+relwy))&&
		 (sys.ms_x<=(alo[cnt].x2+relwx))&&
		 (sys.ms_y<=(alo[cnt].y2+relwy)))
				{poj=alo[cnt].poj;
				 x1=alo[cnt].x1;
				 y1=alo[cnt].y1;
				 x2=alo[cnt].x2;
				 y2=alo[cnt].y2;

				 //				Oggetto disattivato
				 if (poj->status==ON) {FlagPress=ON; return;}
				 if (poj->lock==OFF) goto reset;
				 if (FlagPress) {MCap=0; UltObj=-1; FlagPress=OFF;}
				 if (UltObj==cnt) return;

				 // Sono su un oggetto
				 //sonic(200,1,2,3,4,5);
					switch (poj->tipo)
					{

					 case O_SCROLL:
					 case O_SCRDB:
								LBOX(x1,y1,x2,y2,L98V,SET); //0
								break;
/*
#ifdef _WIN32
					case OW_SCR:
					case OW_SCRDB:
								LBOX(poj->px,poj->py,
									poj->px+poj->col1-1,
									poj->py+poj->col2-1,L98V,SET);
									break;
#endif
									*/
#ifdef _WIN32
				case OW_LIST:
				case OW_LISTP:
					  if (poj->status==OFF)
					  {
				       box3d(x1,y1,x2,y2,1);
					   box3d(x1+1,y1+1,x2-1,y2-1,4);
                       box3d(x2-GetSystemMetrics(SM_CXHSCROLL)+2,y1+2,x2-2,y2-3,0);
					   line(x2-1,y1+1,x2-1,y2-2,0,SET);
					   line(x2-GetSystemMetrics(SM_CXHSCROLL)+2,y2-2,x2-1,y2-2,0,SET);
					   Tline(x2-GetSystemMetrics(SM_CXHSCROLL)+1,y1+2,
						     x2-GetSystemMetrics(SM_CXHSCROLL)+1,y2-2,L98B,SET);

					  }
				      break;
#endif
					 case O_LIST:
								//LBOX(x1,y1,x2-1,y2-1,0,SET);
								//box3d(x1,y1,x2-1,y2-1,0);  //5
								LBOX(x1,y1,x2-1,y2-1,L98V,SET);  //5

								break;

					 case O_PMENU:
								box3d(x1+2,y1+1,x2,y2-1,0);
								break;

					 case O_ICONEA:
					 case O_IMARKA:
					 case O_IRADIA:
//								box3d(x1+1,y1+1,x2,y2,0);
//								LBOX(x1+1,y1+1,x2,y1+1,2,SET);
								box3d(x1+1,y1+1,x2-1,y2-1,0);
								//box3d(x1+2,y1+2,x2-1,y2-1,0);
								//LBOX(x1,y1,x2+1,y2+1,L98N,SET);
								LBOX(x1,y1,x2,y2,L98N,SET);
								break;

					 case O_ICONEB:
					 case O_IMARKB:
					 case O_IRADIB:
								box3d(x1+1,y1+1,x2,y2,0);
								LBOX(x1,y1,x2+1,y2+1,L98N,SET);
								Tline(x1+2,y1+1,x2-2,y1+1,ColorLum(sys.ColorBackGround,10),SET);
								Tline(x1+2,y1+2,x2-2,y1+2,ColorLum(sys.ColorBackGround,30),SET);
								//Tline(x1+2,y1+3,x2-2,y1+3,ColorLum(sys.ColorBackGround,10),SET);

								//Tline(x1+2,y2-1,x2-2,y2-1,ColorLum(sys.ColorBackGround,-30),SET);
								Tline(x1+2,y2-1,x2-2,y2-1,ColorLum(sys.ColorBackGround,-20),SET);
								Tline(x1+2,y2-2,x2-2,y2-2,ColorLum(sys.ColorBackGround,-5),SET);
								break;

					 case O_KEYDIM:
//								box3d(x1+1,y1+1,x2-1,y2-1,0);
								LBOX(x1+1,y1+1,x2-1,y1+1,L98B,SET); // new
								box3d(x1+1,y1+2,x2-1,y2-1,0);
								//box3d(x1+2,y1+2,x2-2,y2-2,0);
								LBOX(x1,y1,x2,y2,L98N,SET);
								break;
					}
				 MCap=0;
				 UltObj=cnt;
				 }
				 else
				 {
					if (MCap>=2) goto reset;
				 }
 avanti:

	// Cambio di oggetti
 if (FlagPress) {MCap=0; UltObj=-1; FlagPress=OFF;}

 if (MCap>10) {MCap=10;}
 cnt++;
 return;

 reset:
 if (UltObj!=-1) {UltObj=-1; MCap=0;}
 goto avanti;
}

