//   ---------------------------------------------
//   | ControlloCFPI()
//   |          
//   |          
//   |             by Ferrà Art & Technology 2004
//   ---------------------------------------------

//
//	Controllo Codice Fiscale e Partita IVA
//  --------------------------------------
//	passare :	iTipoCodice (1=Partita Iva,2=Codice Fiscale)
//				lpCodice (codice da controllare)
//	ritorna :	0=Codice giusto,1=Lunghezza irregolare,2=Codice sbagliato			
//	

#include "\ehtool\include\ehsw_i.h"
#include <time.h>
#include <math.h>

SINT ControlloCFPI(SINT iTipoCodice,CHAR *lpCodice)
{	
	SINT	iRetErr=0; 	
	SINT	iTotCaratteri=0;
	SINT	a;
	SINT	iControllo1,iControllo2;
	SINT	iCod[]={1,0,5,7,9,13,15,17,19,21,2,4,18,20,11,3,6,8,12,14,16,10,22,25,24,23};
	SINT	iCarattere;
	CHAR	*lp;
	CHAR	szServ[20];
	BOOL	fErrore=FALSE;
	
	/*	
	iCod[0]=1; iCod[1]=0; iCod[2]=5; 
	iCod[3]=7; iCod[4]=9; iCod[5]=13;
	iCod[6]=15; iCod[7]=17; iCod[8]=19;
	iCod[9]=21;	iCod[10]=2; iCod[11]=4; 
	iCod[12]=18;iCod[13]=20; iCod[14]=11; 
	iCod[15]=3;iCod[16]=6; iCod[17]=8; 
	iCod[18]=12; iCod[19]=14; iCod[20]=16; 
	iCod[21]=10; iCod[22]=22;iCod[23]=25; 
	iCod[24]=24; iCod[25]=23;
	*/

	if (!lpCodice) {iRetErr=1; return iRetErr;}
	if (!*lpCodice) {iRetErr=1; return iRetErr;}

	//lp=strchr(lpCodice,0);
	//iTotCaratteri=lp-lpCodice;
	iTotCaratteri=strlen(lpCodice);
	if ((iTipoCodice==1&&iTotCaratteri!=11)||
		(iTipoCodice==2&&iTotCaratteri!=16)) {iRetErr=1; return iRetErr;}

	switch (iTipoCodice)
	{
		case 1:	//	Controllo Partita IVA 		
			for (lp=lpCodice;*lp;lp++) {if (*lp<48||*lp>57) {fErrore=TRUE; break;}}		
			if (fErrore) {iRetErr=2; break;}
			if (!memcmp(lpCodice,"0000000",7)) {iRetErr=2; break;}		
			if (!memcmp(lpCodice+7,"000",3)) {iRetErr=2; break;}

			iTotCaratteri=0;
			for (a=0;a<9;a+=2)
			{
				lp=lpCodice+a;
				iTotCaratteri+=*lp-48;
			}

			for (a=1;a<10;a+=2)
			{
				lp=lpCodice+a;
				itoa(100+((*lp-48)*2),szServ,10);
				lp=szServ+1; iTotCaratteri+=*lp-48;
				lp=szServ+2; iTotCaratteri+=*lp-48;
			}
			
			itoa(100+iTotCaratteri,szServ,10);
			lp=szServ+2; iControllo1=*lp-48;
			lp=lpCodice+10; iControllo2=*lp-48;

			if (iControllo1==0 && iControllo2!=0) {iRetErr=2; break;}
			if ((iControllo1+iControllo2)!=10 && iControllo1!=0) {iRetErr=2; break;}
			break;


		case 2:	//	Controllo Codice Fiscale
			iControllo1=0; iControllo2=0;

			for (a=0;a<15;a++)
			{
				lp=lpCodice+a; 							
				if (a>5 && a!=8 && a!=11) 
					{if (*lp<48 || *lp>57) {fErrore=TRUE; break;}}
					else 
					{if (*lp<65 || *lp>90) {fErrore=TRUE; break;}}

				iCarattere=*lp;
				if (a<6 || a==8 || a==11) 
					iCarattere-=65;
					else 
					iCarattere-=48;
				
				if (iControllo1) 
					{iControllo2+=iCarattere; iControllo1=0;}
					else 
					{iControllo2+=iCod[iCarattere]; iControllo1=1;}			
			}

			if (fErrore) {iRetErr=2; break;}

			iControllo1=(iControllo2/26);
			iControllo1=iControllo2-(iControllo1*26);
			lp=lpCodice+15; 			
			if (*lp!=(65+iControllo1)) iRetErr=2;
			break;
	}
	
	return iRetErr;
}
