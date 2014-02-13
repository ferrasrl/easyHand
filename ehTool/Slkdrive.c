//   +-------------------------------------------+
//   | SLKdrive Esporta in formato SLK           |
//   | Versione 1.0                              |
//   |                                           |
//   |            by Ferrà Art & Technology 1997 |
//   +-------------------------------------------+

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/ehtool/slkdrive.h"

//long  SLKdrive(int cmd,char *Location,int tipo,void *ptr);

//   +-------------------------------------------+
//   | Driver di scrittura formato SYLK          |
//   |                                           |
//   |                                           |
//   |           by Ferrà Art & Technology 1997 |
//   +-------------------------------------------+
void SLKtoXY(CHAR *Location,SINT *px,SINT *py)
{
 //static CHAR serv[20];
 CHAR *p;
 SINT x,y;
// static int LastY=-1;
// static int LastX=-1;
 strupr(Location);
 p=Location;
 // Molto semplice: da cambiare
 x=*p-'A'+1; p++;
 if ((*p+1)>='A') {x*=26;x+=(*p-'A'+1); p++;}
 y=atoi(p);
 *px=x;
 *py=y;
// return serv;
}
//C;K"NAiNAeNBeNAoNAaNAu"
void Bpush(CHAR *p,SINT dim)
{
	SINT l;
	l=strlen(p);
	p+=l;
	for (;l>0;l--,p--) *(p+dim)=*p;
}

void SLKSpecialChar(CHAR *buf,SINT size)
{
 CHAR *p;
 CHAR by;

 for (p=buf;*p;p++)
 {
	by=*p;
	if (by=='') {Bpush(p,3); *p=27; *(p+1)='N'; *(p+2)='A'; *(p+3)='i';}
	if (by=='è') {Bpush(p,3); *p=27; *(p+1)='N'; *(p+2)='A'; *(p+3)='e';}
	if (by=='•') {Bpush(p,3); *p=27; *(p+1)='N'; *(p+2)='A'; *(p+3)='o';}
	if (by=='à') {Bpush(p,3); *p=27; *(p+1)='N'; *(p+2)='A'; *(p+3)='a';}
	if (by=='—') {Bpush(p,3); *p=27; *(p+1)='N'; *(p+2)='A'; *(p+3)='u';}
 }

 if (strlen(buf)>(UINT) size) ehExit("Internal Error");
}

LONG SLKdrive(SINT cmd,CHAR *Location,SINT tipo,void *ptr)
{
 CHAR buf[210];
 CHAR buf2[210];
// FILE *pf;
 CHAR *p,*p2;
 LONG pt;
 SINT Ret=0;

 static struct INFOSLK SLK;

// static int LastX[PORTMAX]=-1;
// static int LastY[PORTMAX]=-1;
// static char Stile[30];

 switch (cmd)
 {
	// ---------------------------------------------
	// Apre il drive con il modello indicato       |
	// ---------------------------------------------
	case WS_OPEN:

		SLK.pf=fopen(ptr,"r");
		if (!SLK.pf) {Ret=-1;break;}

	// Alloca la memoria desiderata
	//printf("Leggo [%s]",ptr); getch();
	SLK.SLKinfo.Max=10000;
	SLK.SLKinfo.Num=0;
	SLK.SLKinfo.Size=200;
	SLK.SLKinfo.Hdl=memoAlloc(RAM_AUTO,SLK.SLKinfo.Size*SLK.SLKinfo.Max,"SLK");
	if (SLK.SLKinfo.Hdl<0) ehExit("SLK");

//	fseek(pf,0,SEEK_SET);
	// Conta le linee
	while(TRUE)
	{
		if (!fgets(buf,sizeof(buf),SLK.pf)) break;
		if (*buf=='E') break;
		SLKdrive(WS_ADD,"",NOTE,buf);
	}

	fclose(SLK.pf);
	strcpy(SLK.Stile,"M0");
	break;

	// ---------------------------------------------
	// Chiudi il drive                             |
	//                                             |
	// a) Salva nel file indicato                  |
	//                                             |
	//                                             |
	// ---------------------------------------------
	case WS_CLOSE:

			if (tipo==ON)
				{
					SLK.pf=fopen(ptr,"w");
				 
					if (!SLK.pf) {Ret=-1;goto VANTA;}
					SLKdrive(WS_ADD,"",NOTE,"E\n");
					for (pt=0;pt<SLK.SLKinfo.Num;pt++)
					{
						memoRead(SLK.SLKinfo.Hdl,
										pt*SLK.SLKinfo.Size,buf,
										SLK.SLKinfo.Size);
						fwrite(buf,strlen(buf),1,SLK.pf);
					}

					fclose(SLK.pf);
				}

			VANTA:
			if (SLK.SLKinfo.Hdl>-1) memoFree(SLK.SLKinfo.Hdl,"SLK");
			SLK.SLKinfo.Hdl=-1;
			break;

	// ---------------------------------------------
	// WS_FIND                                     |
	// Ricerca un stringa nella lista              |
	//                                             |
	// ---------------------------------------------
	case WS_FIND:

			 //printf("Devo Ricercare [%s]",ptr); getch();
			 for (pt=0;pt<SLK.SLKinfo.Num;pt++)
				{
				 memoRead(SLK.SLKinfo.Hdl,
											 pt*SLK.SLKinfo.Size,
											 buf,
											 SLK.SLKinfo.Size);
				 if (strstr(buf,(CHAR *) ptr)) return pt;
				}
			 return -1;
			 //break;

	// ---------------------------------------------
	// WS_REALSET                                  |
	// Salva nel file indicato il contenuto in     |
	// memoria                                     |
	//                                             |
	// ---------------------------------------------
	case WS_REALSET: break;

	// ---------------------------------------------
	// WS_UPDATE                                   |
	//                                             |
	// Sostituisci la macro con il nome indicato   |
	// ---------------------------------------------
	case WS_UPDATE:
		//{
			//BOOL fTest=FALSE;
			 strcpy(buf,ptr);
			 p=strstr(buf,"|"); if (!p) return -1; // Non c'è la macro
			 *p=0; p2=p+1;
			 pt=SLKdrive(WS_FIND,"",0,buf);
			 if (pt==-1) return -2; // Non trovata la corrispodenza
			 memoRead(SLK.SLKinfo.Hdl,
						   pt*SLK.SLKinfo.Size,buf2,
						   SLK.SLKinfo.Size);
			 //if (strstr(buf,"DESCDIP")) fTest=TRUE;
			 //if (fTest) win_infoarg("Prima [%s]",buf2);
			 p=strstr(buf2,buf);
			 //strcpy(p+strlen(p2),p+strlen(buf));
			 memmove(p+strlen(p2),p+strlen(buf),strlen(p+strlen(buf))+1);
			 memmove(p,p2,strlen(p2));
			 //printf("Dopo [%s]\n",buf2); getch();
			 // Decodifica lettere accentate
			 //if (fTest) win_infoarg("Dopo [%s][%s]",buf2,p2);
			 SLKSpecialChar(buf2,sizeof(buf2));
			 //if (fTest) win_infoarg("DopoSpecial [%s]",buf2);
			 memoWrite(SLK.SLKinfo.Hdl,
							pt*SLK.SLKinfo.Size,buf2,strlen(buf2)+1);
		//}
			 break;

	// ---------------------------------------------
	// WS_ADD                                      |
	//                                             |
	// Aggiunge in coda al file SLK i nuovi dati   |
	// ---------------------------------------------
	case WS_ADD:

/*F;STBM0;Y5;X1
C;K"12-12-1997"
F;STBM0;X2
C;K0.430902777777778
F;STBM0;X3
C;K"In produzione"
F;STBM0;X4
C;K10
F;STBM0;X5
C;K10
F;STBM0;X6
C;K10
F;STBM0;X7
C;K10
F;STBM0;X8
C;K10
F;P36;FF1G;STBM0;X9
C;K10
F;P3;FI0G;STBM0;X10
C;K10
F;STBM0;X11
C;K0.1
*/
			 if (SLK.SLKinfo.Hdl==-1) ehExit("SLK:ADD no open");

			 // Calcola le cordinate di scrittura
			 if (*Location)
			 {SINT x,y;
				switch (*Location)
				 {
					case '>': // Avanti di 1 a destra
										x=SLK.LastX+1; y=SLK.LastY;
										break;
					case '.': // Carrage Return
										x=1; y=SLK.LastY+1;
										break;

					default : SLKtoXY(Location,&x,&y);
										break;
				 }
/*
				if (*Location=='>') {x=SLK.LastX+1; y=SLK.LastY;}
														 else SLKtoXY(Location,&x,&y);
	*/
//			 printf("Locazioni [%s] x=%d,y=%d",Location,x,y); getch();

//									; B=Bottom
//									; T=Top
//									; L=Left
//									; R=Right
//									; M0=Normale
//									; M1=Grassetto
//									; M2=Italico
//									; M3=Grassetto-Italico
//									; M4=?????????????????
//									; S = ??????
//			 strcpy(buf,"F;STM0");
			 strcpy(buf,"F;");
			 if (*SLK.Stile) {strcat(buf,"S"); strcat(buf,SLK.Stile);}

			 if (y!=SLK.LastY) {sprintf(buf2,";Y%d",y); strcat(buf,buf2);}
			 if (x!=SLK.LastX) {sprintf(buf2,";X%d",x); strcat(buf,buf2);}
			 strcat(buf,"\n");
			 SLK.LastX=x; SLK.LastY=y;
			 //if (ptr)
			 SLKdrive(WS_ADD,"",NOTE,buf);
			 }

			 switch (tipo)
			 {
				// Scrittura diretta
				case NOTE: //if (OK) {printf("[%s]",ptr); getch();}
									 memoWrite(SLK.SLKinfo.Hdl,
															SLK.SLKinfo.Num*SLK.SLKinfo.Size,
															ptr,SLK.SLKinfo.Size);
									 SLK.SLKinfo.Num++;
									 break;

				case ALFA: if (!ptr) break;
									 sprintf(buf,"C;K\"%s\"\n",(CHAR *) ptr);
									 // Trasforma le accentate in Windows UNICODE
									 SLKSpecialChar(buf,sizeof(buf));
									 SLKdrive(WS_ADD,"",NOTE,buf);
									 break;

				case NUME: if (!ptr) break;
									 sprintf(buf,"C;K%s\n",(CHAR *) ptr);
									 SLKdrive(WS_ADD,"",NOTE,buf);
									 break;
			 }
#ifdef _WIN32
			 // Devo ridimensionare la memoria
			 if (SLK.SLKinfo.Num>(SLK.SLKinfo.Max-10))
			 {
				SINT NewHdl;
				SINT iSize;

				iSize=SLK.SLKinfo.Size*SLK.SLKinfo.Max;
				SLK.SLKinfo.Max+=1000;
				NewHdl=memoAlloc(RAM_AUTO,SLK.SLKinfo.Size*SLK.SLKinfo.Max,"SLK");
				memcpy(memoLock(NewHdl),memoLock(SLK.SLKinfo.Hdl),iSize);
				memoUnlock(NewHdl);
				memoUnlock(SLK.SLKinfo.Hdl);
				memoFree(SLK.SLKinfo.Hdl,"SLK");		
				SLK.SLKinfo.Hdl=NewHdl;
			 }
			 break;
#endif

	// ---------------------------------------------
	// WS_FILTER                                   |
	// ---------------------------------------------
	case WS_FILTER:

			 strcpy(SLK.Stile,(CHAR *) ptr);
			 break;

 }
return Ret;
}
