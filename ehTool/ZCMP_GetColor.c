//   ---------------------------------------------
//   | ZCMP_GetColor
//   | ZoneComponent GetColor
//   |                                              
//   | Gestisce la finestra di windows per la selezione di un colore
//   | in una oggetto ZONAP                        
//   |                                              
//   |  ATTENZIONE:                               
//   |                                              
//   |                                              
//   |         by Ferrà Art & Tecnology 1993-2000
//   ---------------------------------------------

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/ehtool/fbfile.h"
#include "/easyhand/ehtool/ZCMP_GetColor.h"

#define CLMAX 16
static EH_COLORLIST CLList[CLMAX];
static BOOL fReset=TRUE;
#define CL_FINDOBJ  0

static SINT CLFind(SINT iCosa,void *ptr)
{
	SINT a;
	for (a=0;a<CLMAX;a++)
	{
		switch (iCosa)
		{
			case CL_FINDOBJ:  if (CLList[a].lpObj==(struct OBJ *) ptr) return a;
							  break;
		}
	}
	return -1;
}
static SINT CLAlloc(struct OBJ *obj)
{
	SINT a;
	for (a=0;a<CLMAX;a++)
	{
	  if (CLList[a].lpObj==NULL)
	  {
		CLList[a].lpObj=obj;
		return a;
	  }
	}
	ehExit("CL: overload");
	return 0;
}

void * EhGetColor(struct OBJ *pojCalled,SINT cmd,LONG info,void *ptr)
{
	struct WS_DISPEXT *DExt=ptr;
	SINT iCLIndex=-1;
	CHOOSECOLOR cc;
	static COLORREF CurstColors[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
//	COLORREF crColor;

	if (fReset)
	{
	 //if (cmd!=WS_START) win_infoarg("Inizializzare EhListView()");
	 memset(&CLList,0,sizeof(EH_COLORLIST)*CLMAX);
	 //EhTrackInizialize();
	 fReset=FALSE;
	 //return 0;
	}

	iCLIndex=CLFind(CL_FINDOBJ,pojCalled);
	if (iCLIndex<0) iCLIndex=CLAlloc(pojCalled);
	switch(cmd)
	{
		case WS_INF: break;

		case WS_SEL:
		
		ZeroFill(cc);
		cc.lStructSize=sizeof(CHOOSECOLOR);
		cc.hwndOwner  = WindowNow();
		cc.hInstance  = NULL;//sys.EhWinInstance;
		cc.rgbResult  = CLList[iCLIndex].lColor;//RGB(0x80,0x80,0x80);
		cc.lpCustColors=CurstColors; // Custom color
		cc.Flags      = CC_RGBINIT|CC_FULLOPEN;
		cc.lCustData  = 0L;
		cc.lpfnHook   = NULL;
		cc.lpTemplateName = NULL;
		if (ChooseColor(&cc))
		{
			// Il nuovo colore selezionato
			CLList[iCLIndex].lColor=cc.rgbResult;
			obj_vedisolo(pojCalled->nome);
		}
		break;

		case WS_CLOSE: // Distruzione
			CLList[iCLIndex].lpObj=NULL;
			break;

		case WS_REALSET:
			CLList[iCLIndex].lColor=info;
			break;

		case WS_REALGET:
			if (info) 
			{
				return (CHAR *) ((GetRValue(CLList[iCLIndex].lColor)<<16)|(GetGValue(CLList[iCLIndex].lColor)<<8)|GetBValue(CLList[iCLIndex].lColor));
			}
			return (CHAR *) CLList[iCLIndex].lColor;

		case WS_DISPLAY:
		case WS_DO: // Spostamento / Ridimensionamento
			//MoveWindow(OBJ_CallSub->hWnd,DExt->px,DExt->py,DExt->lx,DExt->ly,TRUE);
			box(DExt->px,DExt->py,DExt->px+DExt->lx-1,DExt->py+DExt->ly-1,0,SET);
			Tbox(DExt->px+1,DExt->py+1,DExt->px+DExt->lx-2,DExt->py+DExt->ly-2,sys.ColorBackGround,SET);
			Tboxp(DExt->px+2,DExt->py+2,DExt->px+DExt->lx-3,DExt->py+DExt->ly-3,CLList[iCLIndex].lColor,SET);
			break;
	}

	return NULL;
}

