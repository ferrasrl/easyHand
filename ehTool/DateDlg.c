//   ---------------------------------------------
//   | DATEDLG  Date Dialog Box                  |
//   |          Finestra per la scelta di una    |
//   |          data                             |
//   |             by Ferrà Art & Technology 1999 |
//   ---------------------------------------------
#include "/easyhand/inc/easyhand.h"
#include "/easyhand/ehtool/main/datautil.h"
#include "/easyhand/ehtool/main/armaker.h"

#include <time.h>

static CHAR *MeseLungo[]={"Gennaio","Febbraio","Marzo",
						   "Aprile","Maggio","Giugno",
						   "Luglio","Agosto","Settembre",
						   "Ottobre","Novembre","Dicembre",NULL};

// Da testare !!?!
static DateCtrl(CHAR * pszNowData)
{
	CHAR Dat1[12];
	CHAR Dat2[12];
	EH_TIME sTime;

	timeSet(&sTime,pszNowData,NULL); timeSetEOM(&sTime);
    strcpy(Dat1,dateDtoY(pszNowData));
	timeGetDate(&sTime,Dat2,TRUE);
//	strcpy(Dat2,dateDtoY(DammiFineMese(NowData)));
	if (strcmp(Dat1,Dat2)>0) timeGetDate(&sTime,pszNowData,FALSE);//strcpy(NowData,DammiFineMese(NowData));
}

static const SINT Dx=14;
static const SINT Dy=55;
static const SINT yOfs=25;
static const SINT xLen=22;
static const SINT yLen=18;
static TCHAR bDay_Old[50];

static void ViewDay(CHAR *NowData)
{
  CHAR Serv[80];
  TCHAR bDay[50];
  SINT Ng,a,Tg;
  LONG Col;
//  boxp(11,53,186,232,15,SET);
  const CHAR NDay[]="LMMGVSD";
  
  if (NowData==NULL) 
  {
   RECT Zona;
   memset(bDay_Old,255,sizeof(bDay_Old)); 
   //sys.fFontBold=TRUE;
   Zona.left=Dx; Zona.top=Dy;
   Zona.bottom=Dy+6*yLen+yOfs-1; Zona.right=(Dx+7*xLen)-1;
   for (a=0;a<7;a++)
   {
	SINT px=Dx+(a%7)*xLen;
	if (a%2) Col=RGB(255,255,255); else Col=RGB(240,240,240);
	Tboxp(px,Dy,px+xLen-1,Zona.bottom,Col,SET);
	Tboxp(px,Dy,px+xLen-1,Dy+yOfs-5,ColorLum(Col,-10),SET);
    Serv[0]=NDay[a]; Serv[1]=0;
	Col=0; if (a==6) Col=11;
	dispfm(px+((xLen-font_lenf(Serv,"SMALL F",3,STYLE_BOLD))>>1),Dy+3,Col,-1,STYLE_BOLD,"SMALL F",3,Serv);
   }
   
   box3d(Zona.left-1,Zona.top-1,Zona.right+1,Zona.bottom+1,4);
   box3d(Zona.left-2,Zona.top-2,Zona.right+2,Zona.bottom+2,1);
 //  sys.fFontBold=FALSE;
   return;
  }
  strcpy(Serv,DammiFineMese(NowData)); // Numero di giorni del mese
  //win_infoarg("%s",Serv);
  Serv[2]=0; Tg=atoi(Serv);

  sprintf(Serv,"01%s",NowData+2); // Trovo il primo giorno
  memset(bDay,0,sizeof(bDay));
  Ng=NGgetS(Serv);
  for (a=0;a<Tg;a++)
  {
    bDay[Ng+a]=a+1;
	if (!strcmp(Serv,NowData)) bDay[Ng+a]|=64;
	strcpy(Serv,dateCalc(Serv,1));
  }
  
  for (a=0;a<(7*6);a++)
  {
    SINT px=Dx+(a%7)*xLen+2;
	SINT py=Dy+(a/7)*yLen+yOfs;
	
    if (bDay[a]==bDay_Old[a]) continue;
	if (a%7%2) Col=RGB(255,255,255); else Col=RGB(240,240,240);
	if (bDay[a]) 
	{   SINT cBg,cFg;
		if (bDay[a]&64) 
			{cBg=sys.arsColor[3]; if ((a%7)==6) cBg=sys.arsColor[11];
		     cFg=15;
			} else {cBg=Col; cFg=0;}
		Tboxp(px,py,px+18,py+15,cBg,SET);
		//if (bDay[a]&64) sys.fFontBold=TRUE;
		dispnum(px+17,py+1,cFg,-1,(bDay[a]&64)?STYLE_BOLD:SET,"SMALL F",3,2,0,0,(LONG) bDay[a]&31);
		//if (bDay[a]&64) sys.fFontBold=FALSE;
	}
	else Tboxp(px,py,px+18,py+15,Col,SET);
  }
  memcpy(bDay_Old,bDay,sizeof(bDay));
}

void * lstAnni(struct OBJ *objCalled,EN_MESSAGE cmd,LONG info,CHAR *str)
{
 SINT a;
 CHAR szServ[80];
 SINT  num,iAnno;

	// ------------------------------------------------------
	//           WS_OPEN : COSTRUISCE PUNTATORI E LISTA  			!
	// ------------------------------------------------------

	switch (cmd)
	{
		case WS_OPEN:

			strcpy(szServ,dateToday()); iAnno=atoi(szServ+4)-5; 
			ARMaker(WS_OPEN,NULL);
			for (a=0;a<10;a++)
			{
			sprintf(szServ,"%4d",iAnno); iAnno++;
			ARMaker(WS_ADD,szServ);
			}
			objCalled->ptr=ARFHdlToPtr(ARMaker(WS_CLOSE,"Anni"));
			objCalled->pOther=objCalled->ptr;
			objCalled->col2=10;
			break;

	// ------------------------------------------------------
	//   CHIEDE IL CODICE RELATIVO ALLA SELEZIONE IN LISTA  !
	// ------------------------------------------------------

 case WS_PTREC :
		return *(objCalled->ptr+objCalled->col1);

	// ---------------------------------------------------------
	//   RICERCA UN CODICE RELATIVO PER LA SELEZIONE IN LISTA  !
	// ---------------------------------------------------------

 case WS_SEL :
			
		num=-1;

		for (a=0;a<atoi(objCalled->text+4);a++)
		{
			if (!strcmp(*(objCalled->ptr+a),str)) {num=a;break;}
		}
		if (num==-1) return NULL;
		objCalled->col1=num;
		return *(objCalled->ptr+a);

	// ------------------------------------------------------
	//             WS_CLOSE : LIBERA MEMORIA USATA       			!
	// ------------------------------------------------------

 case WS_CLOSE: // Close
		ehFreeNN(objCalled->pOther);
		objCalled->pOther=NULL;
		break;

 }
return 0;
}

static void SetLocalCombo(BYTE *pDate)
{
	CHAR szServ[20];
	// ipt_writevedi(0,"",atoi(pDate+4));
	memcpy(szServ,pDate+2,2); szServ[2]=0;
	obj_listset("MESE",atoi(szServ)-1);
	obj_listcodset("LSTANNI",(pDate+4));
}


CHAR *DateChoose(CHAR *Date)
{
// Creato dal progetto : data.vpg 
//	SINT a;
	static CHAR szNowData[20];
//	CHAR Serv[20];
	//CHAR *ListAnni[]={"9991","9992","9993","9994","9995","9996","9997","9998","9999",NULL};
	//CHAR **arAnni;

	struct OBJ obj[]={
	// TipObj    Nome    Sts Lck    x   y CL1 CL2 TESTO
	{OW_LIST ,"MESE"   ,OFF,ON , 12, 29,  0, 12,"70","",0,MeseLungo},
	{O_ICONEA,"ESC"    ,OFF,ON ,130,198, 40, 26,"ESCI"},
	{O_KEYDIM,"CR"     ,OFF,ON , 11,198, 83, 26,"#Applica"},
	{OW_LIST ,"LSTANNI",OFF,ON ,110, 29,  0,  5,"38","",lstAnni},
	{STOP}
	}; 

  if (strlen(Date)==8) strcpy(szNowData,Date); else strcpy(szNowData,dateToday());

/*
  arAnni=ARSplitC("9991|9992|9993|9994|9995|9996|9997|9998|9999","|");
  strcpy(arAnni[4],NowData+4);
  for (a=1;a<=4;a++)
  {
	  sprintf(Serv,"%d",(atoi(arAnni[4])-a));
	  strcpy(arAnni[(4-a)],Serv);
	  sprintf(Serv,"%d",(atoi(arAnni[4])+a));
	  strcpy(arAnni[(4+a)],Serv);
  }
  */

//for (a=0;arAnni[a];a++) win_infoarg("%s",arAnni[a]);
  //obj[3].ptr=arAnni;

  win_open(EHWP_FOCUSCENTER,0,181,236,-1,3,ON,"Selezione di data");
  
//  boxp(11,53,186,232,15,SET);
  ViewDay(NULL);

  obj_open(obj);
  obj_vedi();

// Loop di controllo EH

//	obj_listset("MESE",atoi(Serv)-1);
//	obj_listcodset("LSTANNI",(NowData+4));

  SetLocalCombo(szNowData);

	while (TRUE) 
	{
		SINT ms;
		ViewDay(szNowData);
		//	ipt_writevedi(0,"",atoi(NowData+4));
		//	memcpy(Serv,NowData+2,2); Serv[2]=0;
		//	obj_listset("MESE",atoi(Serv)-1);
		//	obj_listcodset("LSTANNI",(NowData+4));

		ms=eventGetWait(NULL);

		if (ms==IN_SX)
		{
		//	  SINT x=(sys.ms_x+relwx-Dx)/xLen;
		//	  SINT y=(sys.ms_y+relwy-Dy)/yLen;
		  SINT x=(sys.ms_x-relwx-Dx);
		  SINT y=(sys.ms_y-relwy-Dy-yOfs);
		  if ((y>0)&&(x>0)) 
		  { SINT ng;
			y/=yLen; x/=xLen;
			if ((x<7)&&(y<6))
			{
			 ng=x+y*7;
			 //_d_("%d,%d [%d]     ",x,y,ng);
			 if (bDay_Old[ng]) 
			 {
			  sprintf(szNowData,"%02d%s",bDay_Old[ng]&31,szNowData+2);
			 }
			}
		  }
	}

	if (key_press(CR)||obj_press("CROFF")) break;
	if (key_press(ESC)||obj_press("ESCOFF")) {strcpy(szNowData,Date); break;}
	if (key_press2(_FDX)) strcpy(szNowData,dateCalc(szNowData,1));
	if (key_press2(_FSX)) strcpy(szNowData,dateCalc(szNowData,-1));
	if (key_press2(_FUP)) strcpy(szNowData,dateCalc(szNowData,-7));
	if (key_press2(_FDN)) strcpy(szNowData,dateCalc(szNowData,7));
	if (key_press2(_HOME)) strcpy(szNowData,dateToday());
/*
	if (obj_press("MESE"))
	{
		sprintf(Serv,"%c%c%02d%s",NowData[0],NowData[1],OBJ_key+1,obj_listcodget("LSTANNI"));
		strcpy(NowData,Serv);
		DateCtrl(NowData);
	}
*/
	if (obj_press("MESE")||obj_press("LSTANNI"))
	{
		sprintf(szNowData,"%c%c%02d%s",szNowData[0],szNowData[1],(obj_listget("MESE")+1),obj_listcodget("LSTANNI"));
		DateCtrl(szNowData);
		SetLocalCombo(szNowData);
	}
	
	if (key_press2(_PGDN)) 
	{
        SINT Mese=(szNowData[2]-'0')*10+(szNowData[3]-'0');
		SINT Anno=atoi(szNowData+4);
		Mese++; if (Mese>12) {Mese=1; Anno++;}
		if (Anno>2036) Anno=2036;
		sprintf(szNowData,"%c%c%02d%04d",szNowData[0],szNowData[1],Mese,Anno);
		DateCtrl(szNowData);
	}

	if (key_press2(_PGUP)) 
	{
        SINT Mese=(szNowData[2]-'0')*10+(szNowData[3]-'0');
		SINT Anno=atoi(szNowData+4);
		//win_infoarg("%d %d [%s]",Mese,Anno,NowData);
		Mese--; if (Mese<1) {Mese=12; Anno--;}
		if (Anno<1971) Anno=1970;
		sprintf(szNowData,"%c%c%02d%04d",szNowData[0],szNowData[1],Mese,Anno);
		DateCtrl(szNowData);
	}
	
  };
  win_close();
//  ehFree(arAnni);
  return szNowData;
}
