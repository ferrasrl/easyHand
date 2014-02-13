//   +-------------------------------------------+
//   | SUPERFIND                                 |
//   | Funzione di Ricerca in campo ADB          |
//   |                                           |
//   | Indicare i campi separati da              |
//   | & per AND                                 |
//   | | per OR                                  |
//   |                                           |
//   |            Creato da G.Tassistro          |
//   |            Ferrà Art & Technology 1996     |
//   +-------------------------------------------+
#include "/easyhand/inc/easyhand.h"

//   +-------------------------------------------+
//   | Hdb 			- Handle Dbase                   |
//   | str 			- Argomento da cercare           |
//   |                                           |
//   |            & Separatore AND di ricerca    |
//   |            | Separatore OR di ricerca     |
//   |                                           |
//   | Hrec			- Ultimo record trovato          |
//   | Indice   - Numero dell'indice di ricerca  |
//   | Field    - Campo in cui cercare           |
//   |                                           |
//   +-------------------------------------------+

struct SUPERFIND {
	CHAR *pt;
	SINT Azione;
};


SINT SuperFind(SINT Hdb,CHAR *str,LONG *Hrec,SINT Indice,CHAR *Field)
{
 CHAR BMS_ico[NOMEICONE_SIZE+1];  //icone corrente del mouse
 SINT BMS_ax,BMS_ay;
 static SINT memoria=0;
 static CHAR UltCerco[50]={0};
 static SINT Modo=OFF;

 CHAR *PtConf;
 // struct VGAMEMO vga;
 CHAR BFS[50];
 SINT x,y,x2,y2,dx,dy;
 SINT FlagOut=0;
 SINT cnt=0;
 SINT iCnt2=0;
 SINT VeroCnt=0;
 SINT FlagBck=OFF;
 #define MAXSP 10
 struct SUPERFIND SP[MAXSP];
 SINT a,NumSP;
 CHAR *pE,*pS;
 SINT FlagMEMO;
 SINT xMs,yMs;
 static CHAR Titolo[]="SUPERFIND";

 PtConf=adb_FldPtr(Hdb, Field);

 // ----------------------------
 // Prepara campi di ricerca   !
 // ----------------------------

 strcpy(BFS,str); // Copia la ricerca
 NumSP=0;
 pS=BFS;

 while (TRUE)
 {// Controllo AND
	pE=strstr(pS,"&");
	if (pE==NULL) goto ContOR;

	SP[NumSP].pt=pS;
	SP[NumSP].Azione=AND;
	*pE=0; pS=++pE;
	NumSP++; continue;


	// Controllo OR
	ContOR:
	pE=strstr(BFS,"|");
	if (pE==NULL) break; //     Fine Loop

	SP[NumSP].pt=pS;
	SP[NumSP].Azione=OR;
	*pE=0; pS=++pE;
	NumSP++; continue;
 }

 SP[NumSP].pt=pS;
 SP[NumSP].Azione=OR;
 if (pE!=NULL) *pE=0;
 NumSP++;
/*
 printf("%s\n",str);
 for (a=0;a<NumSP;a++)
 {
	printf("%2d:%2d [%s]\n",a,SP[a].Azione,SP[a].pt);
 }
 getch();
 return;
	*/

 if (strlen(str)>sizeof(UltCerco)) ehExit("Superfind");
 if (strcmp(str,UltCerco)) {*Hrec=0; Modo=OFF; strcpy(UltCerco,str);}

	//Cambio il mouse
	strcpy(BMS_ico,sys.szMouseCursorName);
	BMS_ax=sys.sMouseCursorHotPoint.x; 
	BMS_ay=sys.sMouseCursorHotPoint.y;
	mouse_graph(0, 0,"MS05");
	x=sys.ms_x; y=sys.ms_y;

#ifdef _WIN32
	{
	 POINT Point;
     GetCursorPos(&Point);
     xMs=Point.x;
	 yMs=Point.y;
	}
#else
	xMs=sys.ms_x; yMs=sys.ms_y;
#endif
	ico_info(&dx,&dy,"PAPER");
	x2=x+dx; y2=y+dy;

	 WIN_ult++; // tapullo per i controlli sulle MGZ e HMZ
	 // cazzo che palo 2/settembre 97
	 //strcpy(WIN_info[WIN_ult].titolo,Titolo); // Copia il titolo
	 WIN_info[WIN_ult].titolo=Titolo; // Copia il titolo

	RIFO:
	if(*Hrec==0)
	 {
		// ---------------------------------------
		// Prima volta cioe' inizio la ricerca   !
		// ---------------------------------------
		memoria=0;

		if (Modo==OFF) // Prima ricerca
		 {SINT a;
			a=adb_find(Hdb, Indice, B_GET_GE, SP[0].pt, B_RECORD);
			if ( (a) || memcmp(SP[0].pt,PtConf,strlen(SP[0].pt)) )
									{Modo=ON; goto RIFO;}
		 }
		 else // Seconda ricerca
		 {
		 if (adb_find(Hdb, Indice, B_GET_FIRST, "", B_RECORD))
			 {FlagOut=-2;goto FINE;}
		 }
	 }
	else
	 {
		// Si riposiziona
		adb_get(Hdb,*Hrec,Indice);

		if(adb_find(Hdb, Indice, B_GET_NEXT, "", B_RECORD))
		 {
			Modo=ON;
			*Hrec=0;
			memoria=0;
			if(adb_find(Hdb, Indice, B_GET_FIRST, "", B_RECORD))
				{FlagOut=-1;goto FINE;}
		 }
	 }

	for(;;)
	{
	// eventGet(NULL); if (key_press(ESC)) {FlagOut=-1; goto FINE;}
	 cnt++;
	 if (cnt>32)
			{cnt=0;
			 iCnt2++;
			 if (iCnt2>10)
			 {
				//eventGet(NULL); 
				eventGet(NULL);
				if (key_press(ESC)) {FlagOut=-1; goto FINE;}
				iCnt2=0;
			 }
			 
#ifdef _WIN32
			 SetCursorPos(xMs+4,yMs-2+VeroCnt++); //_d_("%d  ",y-2+VeroCnt++);
#else
			 mouse_set(xMs+4,yMs-2+VeroCnt++); //_d_("%d  ",y-2+VeroCnt++);
#endif
			 if (!FlagBck)
				{
//				 Avideo_bck(x,y,x2,y2,RAM_AUTO,&vga);
//				 Aico_disp(x,y,"PAPER");
				 FlagBck=ON;
				}
			 }

	 if (VeroCnt>(dy-10)) VeroCnt=0;

// -----------------------------------------
//				   CONTRONTA VALIDITA'           !
// -----------------------------------------

	 FlagMEMO=0;
	 for (a=0;a<NumSP;a++)
	 {
		// Trovato CAMPO
		if (strstr(PtConf, SP[a].pt) != 0 )
		{if (a==0) {FlagMEMO=1; continue;} // è IL PRIMO
		 switch (SP[a-1].Azione)
		 {
			case OR  : FlagMEMO|=1; break;
			case AND : FlagMEMO&=1; break;
		 }
		}
		else
		{
		 if (a==0) continue;
		 if (SP[a-1].Azione==AND) {FlagMEMO=0; break;}
		}
	 }
	 if (FlagMEMO) {memoria=1; break;}

	 /*
	 if(strstr(PtConf, str) != 0 ) // PtConf
		{
		 memoria=1; break;}
		 */
// -----------------------------------------
//				CERCA IL PROSSIMO                !
// -----------------------------------------

	 if(adb_find(Hdb, Indice, B_GET_NEXT, "", B_RECORD))
		{
		 if(memoria != 1)
			{
			 FlagOut=-2; break;}

		 *Hrec=0;
		 memoria=0;
		 adb_find(Hdb, Indice, B_GET_FIRST, "", B_RECORD);
		}
	}

 // Fine
 FINE:

	 WIN_ult--;

// if (FlagBck) video_rst(&vga);
 mouse_graph(BMS_ax, BMS_ay, BMS_ico);
#ifdef _WIN32
	SetCursorPos(xMs,yMs); //_d_("%d  ",y-2+VeroCnt++);
#else
	mouse_set(xMs,yMs); //_d_("%d  ",y-2+VeroCnt++);
#endif
 if (FlagOut==-2) win_time("Parola inesistente",2);
				  else
				  adb_position(Hdb, Hrec);
 return FlagOut;

}


