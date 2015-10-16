//   ---------------------------------------------
//   | ZCMP_ProgressBar
//   | ZoneComponent ProgressBar
//   |                                              
//   | Gestisce una ProgressBar
//   | in una oggetto ZONAP                        
//   |                                              
//   |         by Ferrà Art & Technology 1993-2002
//   ---------------------------------------------

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/ehtool/ZCMP_ProgressBar.h"

/*
			*(lpi)=0; // Contatore
			*psBar->iMax=100; // Massimo
			psBar->iPerc=0; // Percentuale
			*(lpi+3)=sys.Color3DHighLight; // Percentuale*/
typedef struct {
	SINT iCount;
	SINT iMax;
	SINT iPerc;
	EH_COLOR cBar;
} EH_PROGRESS_BAR;

// -----------------------------------------------
// ProgressBar
// -----------------------------------------------
void * ehzProgressBar(struct OBJ *objCalled,EN_MESSAGE cmd,LONG info,void *ptr)
{
	 EH_DISPEXT *DExt=ptr;
//	 SINT *lpi;
	 SINT iLx,iLy;
	 SINT iRiemp;
	 SINT iPos;
	 SINT iBar;
	 SINT iDiv;
	 SINT a;
	 SINT ThePoint;
	 SINT iMyShadow;
	 SINT iEnd;
	 EH_PROGRESS_BAR * psBar;

	 //lpi=(SINT *) objCalled->text;
	 psBar=(EH_PROGRESS_BAR *) objCalled->pOther;

	 switch(cmd) {
		case WS_INF: return NULL;

		case WS_CREATE:
			psBar=objCalled->pOther=ehAllocZero(sizeof(EH_PROGRESS_BAR));
			psBar->iMax=100;
			psBar->cBar=sys.Color3DHighLight;
			if (!strcmp(objCalled->text,"WIN")) {
				objCalled->hWnd=CreateWindowEx(0,
								   PROGRESS_CLASS, 
								   NULL,
								   WS_CHILD|WS_VISIBLE,
								   0,0,
								   10,10,//-(MENU_HEIGHT),
								   WindowNow(),//sGMSetup.hwAnagTB,//sGMSetup.hwAnag,
								   (HMENU) NULL,
								   sys.EhWinInstance,
								   NULL);
				SendMessage(objCalled->hWnd, PBM_SETRANGE,0,MAKELPARAM(0,100));
				SendMessage(objCalled->hWnd, PBM_SETSTEP, MAKEWPARAM(1, 0), 0);
			} else objCalled->hWnd=NULL;

			break;

		case WS_DESTROY:
			ehFreePtr(&objCalled->pOther);
			if (objCalled->hWnd) DestroyWindow(objCalled->hWnd);
			break;

		case WS_DO: // Spostamento / Ridimensionamento
			MoveWindow(objCalled->hWnd,DExt->px,DExt->py,DExt->lx,DExt->ly,TRUE);
			break;

		case WS_SETFLAG:

			if (!strcmp(ptr,"MAX")) // Setta lo stile della finestra
			{
				if (info<1) info=1;
				//*psBar->iMax=info;
				psBar->iMax=info;
			}
			
			if (!strcmp(ptr,"COL")) // Setta lo stile della finestra
			{
//				*(lpi+3)=info;
				psBar->cBar=info;
				obj_vedisolo(objCalled->nome);
			}
			
			if (!strcmp(ptr,"CNT")) // Setta lo stile della finestra
			{
				//*lpi=info;
				psBar->iCount=info;

				// Calcolo percentuale
				iRiemp=psBar->iCount; if (iRiemp>(psBar->iMax)) iRiemp=psBar->iMax; 
				iPos=0;
				if (psBar->iMax) iPos=100*iRiemp/psBar->iMax;// calcolo percntuale di riempimento
			
				// Se la percentuale è cambiata ristampo il tutto
				if (psBar->iPerc!=iPos) {
					psBar->iPerc=iPos; 
					if (objCalled->hWnd)
						SendMessage(objCalled->hWnd, PBM_SETPOS, psBar->iPerc, 0); 
						else
						obj_vedisolo(objCalled->nome);
				}

				//if (*lpi!=info) {*lpi=info; obj_vedisolo(objCalled->nome);}
			}

			break;


		case WS_DISPLAY: 
			if (objCalled->hWnd) {InvalidateRect(objCalled->hWnd,NULL,TRUE); break;}

			iLx=DExt->px+DExt->lx-1; iLy=DExt->py+DExt->ly-1;
			iRiemp=psBar->iCount;  if (iRiemp>(psBar->iMax)) iRiemp=psBar->iMax;
			iPos=100*iRiemp/psBar->iMax;// calcolo percentuale di riempimento
			iBar=(DExt->lx-3)*iRiemp/psBar->iMax;// Calcolo dimensione barra pieno
			iDiv=(DExt->px+iBar);

 			//Tbox(DExt->px,DExt->py,iLx,iLy,0,SET);
			box3d(DExt->px,DExt->py,iLx,iLy,1);
//			dispf(iText,DExt->py,15,-1,ON,"SMALL F",3,szServ);
			ThePoint=(DExt->ly/3);
			iMyShadow=ColorLum(sys.Color3DShadow,-30);
			if (iBar>0) 
			{
				 Tline(DExt->px+1,DExt->py+1,iDiv,DExt->py+1,iMyShadow,SET);
				 for (a=2;a<DExt->ly-2;a++) {

					 LONG Col;
					 if (a<ThePoint) 
						Col=ColorFusion(sys.Color3DShadow,psBar->cBar,(100/ThePoint*a));
						else
						Col=ColorFusion(sys.Color3DShadow,psBar->cBar,(100/(DExt->ly-ThePoint)*(DExt->ly-a)));
//					 iEnd=iDiv+a-2; if (iEnd>iLx-1) iEnd=iLx-1;
					 iEnd=iDiv-2; if (iEnd>iLx-1) iEnd=iLx-1;
					 Tline(DExt->px+2,DExt->py+a,iEnd,DExt->py+a,Col,SET);
				 }
				 Tline(DExt->px+1,iLy-1,iEnd,iLy-1,ColorLum(sys.Color3DShadow,-40),SET);
			}

			ThePoint=DExt->ly-ThePoint;
			if (iBar<(DExt->lx-2)) 
			{//Tboxp(iDiv+1,DExt->py+2,iDiv+1,iLy-2,0,SET);
			 //Tboxp(iDiv+2,DExt->py+1,iLx-1,iLy-1,ColorLum(sys.Color3DShadow,-10),SET);
			 for (a=1;a<DExt->ly-1;a++)
			 {
			  LONG Col;
			  if (a<ThePoint) Col=ColorFusion(iMyShadow,sys.Color3DLight,(100/ThePoint*a));
							  else
							  Col=ColorFusion(iMyShadow,sys.Color3DLight,(100/(DExt->ly-ThePoint)*(DExt->ly-a)));
//				if (iBar>0) {iEnd=iDiv+a; if (iEnd>iLx-1) iEnd=iLx-1;} else iEnd=iDiv+1;
				if (iBar>0) {iEnd=iDiv-1; if (iEnd>iLx-1) iEnd=iLx-1;} else iEnd=iDiv+1;
				Tline(iEnd,DExt->py+a,iLx-1,DExt->py+a,Col,SET);
			 }
			}

			//sys.fFontBold=FALSE;
			break;
	}
	return NULL;
}
