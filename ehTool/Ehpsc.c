//   +------------------------------------------+
//   | EHPSC   Funzioni per il controllo         |
//   |         dell'applicativo EHPowerServer    |
//   |         Client Dos                        |
//   |                                           |
//   |            by Ferrà Art & Technology 1998 |
//   |            Created by G.Tassistro         |
//   +-------------------------------------------+

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/ehtool/ehpsc.h"
#define EHPC_MAX_LINE 1500

static SINT BinScan(CHAR *Memo,CHAR *FindText,SINT LenFind,SINT Start,WORD Len)
{
	SINT a,Stop;

	//l=strlen(FindText);
	Stop=(Len-LenFind+1);
	if (Start>=Stop) return -1;

	Memo+=Start;
	for (a=Start;a<Stop;a++)
	{if (!memcmp(Memo,FindText,LenFind)) return a;
	 Memo++;
	}
 return -1;
}

static void UNImemset(CHAR *p,CHAR Car,WORD MaxLen)
{
 WORD a;
 for (a=0;a<MaxLen;a++,p++) {if (*p) *p=Car;}
}

static void UNImemcpy(CHAR *p,CHAR *str,WORD StrLen,WORD MaxLen)
{
 WORD a;
 if (StrLen<1) return;

 for (a=0;a<MaxLen;a++,p++)
	 {if (*p)
		{*p=*str; str++;
		 StrLen--; if (!StrLen) break;
		}
	 }
}
static void UNIConversion(CHAR *UNIFindText,CHAR *Find)
{
 CHAR *Sp,*Dp;

 Sp=Find;
 Dp=UNIFindText;

 for (;*Sp;Sp++)
 {
	*Dp=*Sp; Dp++;
	*Dp=0; Dp++;
 }
}

static CHAR *UNItraslate(CHAR *p,SINT Num)
{
 static CHAR String[80];
 CHAR *Dp;
 SINT a;
 Dp=String;

 for (a=0;a<Num;p++)
 {
	if (*p) {*Dp=*p; Dp++; a++;}
 }
 *Dp=0;
 return String;
}
#define GOSTEP 20000

SINT BinMultiScan(CHAR *Buffer,
				  CHAR *FindText,
				  SINT Pos,
				  WORD BufferSize)
{
 SINT LenFind;
 SINT TPos;
 CHAR UNIFind[80];
 CHAR *Read;

 LenFind=strlen(FindText);

 // Ricerca standard ASCII 8 bit
 TPos=BinScan(Buffer,FindText,LenFind,Pos,BufferSize);

 // Ricerca standard UNICODE 16 bit
 if (TPos<0)
		{UNIConversion(UNIFind,FindText);
		 TPos=BinScan(Buffer,UNIFind,(LenFind<<1)-1,Pos,BufferSize);
		}

 // Ricerca complessa in tecnologia Mista
 if (TPos<0)
	 {
		// SINT y;
		 SINT LPos;

		 // ---------------------------------------
		 // Ricerco con l'Intestazione a 8 Bit    |
		 // ---------------------------------------
		 LPos=Pos;
		 for (;;)
		 {
			TPos=BinScan(Buffer,"[#",2,LPos,BufferSize);
			if (TPos<0) break;

			if ((BufferSize-TPos)<35) return GOSTEP; // Richiedo lo spostamento avanti

			Read=UNItraslate(Buffer+TPos,LenFind);
			//if (!strcmp(FindText,"[#Qta04*")) break;
			if (!memcmp(Read,FindText,LenFind)) break;
			LPos=TPos+1;
		 }


		 if (TPos>-1) return TPos;

		 // ---------------------------------------
		 // Ricerco con l'Intestazione a 16 Bit   |
		 // ---------------------------------------
		 UNIConversion(UNIFind,"[#");

		 LPos=Pos;
		 for (;;)
		 {
			TPos=BinScan(Buffer,UNIFind,3,LPos,BufferSize);
			if (TPos<0) break;
			if ((BufferSize-TPos)<35) return GOSTEP; // Richiedo lo spostamento avanti

			Read=UNItraslate(Buffer+TPos,LenFind);
			//if (!strcmp(FindText,"[#Qta04*")) break;
			if (!memcmp(Read,FindText,LenFind)) break;
			LPos=TPos+1;
		 }
	 }

 return TPos;
}
/*
static void MemSwap(SINT  HdlFile,
										LONG  *dwSizeFile,
										CHAR *Buffer,
										WORD  BufferSize,
										LONG  Pos,
										WORD  SizeSwap,
										CHAR *Str)
{
 SINT LenScroll;
 LONG Sorg,Dest,LastSorg;
 BOOL Fine=FALSE;
 WORD Size;
 LONG FileSize=*dwSizeFile;

 // 1234567890
 //  ***
 //  Giorgio567890
 //  ------->

 LenScroll=strlen(Str)-SizeSwap;
 printf("Pos=%ld SizeSwap=%d LenStr=%d LenScroll=%d\n",
				 Pos,SizeSwap,strlen(Str),LenScroll);

 printf("Filesize=%ld\n",FileSize);

 if (LenScroll==0) goto FINE;
 if (LenScroll<0)
	{// Scroll a Sinistra DA FARE
		LenScroll=0;
	  goto FINE;
	}

 // Bisogna spostare tutti in avanti

 Sorg=FileSize-BufferSize;
 LastSorg=FileSize;
 for (;;)
 {
	Size=BufferSize;

	if (Sorg<(Pos+SizeSwap))
		{Sorg=Pos+LenScroll;
		 Size=(WORD) (LastSorg-Sorg);
		 Fine=TRUE;
		}

	Dest=Sorg+LenScroll;

	printf("Pos=%ld | Da %ld a %ld (End=%ld) size[%d]\n",
					Pos,Sorg,Dest,Dest+Size-1,Size);
	getch();
	memoRead(HdlFile,Sorg,Buffer,Size);
	memoWrite(HdlFile,Dest,Buffer,Size);

	if (Fine) break;
	LastSorg=Sorg;
	Sorg-=BufferSize;
 }

 FINE:
 memoWrite(HdlFile,Pos,Str,strlen(Str));
 *dwSizeFile=FileSize+LenScroll;
}
	*/
static BOOL LWin=FALSE;
static void LocalVediIcone(void)
{
 static SINT Cont=0;
 CHAR Serv[10];

 if (!LWin) return;
 sprintf(Serv,"ILPT%d",Cont);
 ico_disp(12,13,Serv);
 Cont++;
 if (Cont>5) Cont=0;
}
/*
#ifdef _WIN32
void EHPSCheck(void)
{
//  FindWindow("#32770 (Dialog)",
 HWND hWnd;
 hWnd=FindWindow("#32770","EHServer 1.0");
 if (hWnd==NULL) ShellExecute(NULL,"open",ehPath("EHPS.exe"),"","",SW_NORMAL);
}
#endif
*/
SINT EHPSClient(SINT cmd,CHAR *Find,CHAR *str)
{
	SINT Hdl=-1;

	static SINT HdlBuffer=-1;
	static CHAR *Buffer=NULL;
	const  WORD BufferSize=2048;

	static SINT HdlFile=-1;
	static LONG dwSizeFile=0;
	SINT Check;
	CHAR  FindText[30];
	CHAR  UNIFindText[60];
	FILE  *pf;
	CHAR *p2;

	LONG  Count;
	LONG  Cop;
	SINT  Pos;
	// SINT  Test;
	BOOL  FlagAll;
	CHAR UCEndFind[]={'#',0,']'};

	SINT  PosF;
	SINT  PosF_UNI;
	WORD  MaxLen;

// printf("cmd=%d\n",cmd); getch();
	switch (cmd)
	{
		case WS_DISPLAY:
#ifdef WIN32
			if (!strcmp(Find,"HTML"))
			{
			 //if (f_open(ehPath("EHPS.DAT"),"a",&pf)) ehExit("CAZ");
			 //fprintf(pf,"#HTMLVIEW:%s\r\n",str);
			 //f_close(pf);
				ShellExecute(WindowNow(),"open",str,NULL,NULL,SW_MAXIMIZE);
			}


			if (!strcmp(Find,"EXEC"))
			{
			 //efx3();
//						 if (f_open(ehPath("EHPS.DAT"),"a",&pf)) ehExit("CAZ");
//						 fprintf(pf,"#EXECMD:%s\r\n",str);
//						 f_close(pf);
			 win_infoarg("[%s]",str);
			}
#else

			if (!strcmp(Find,"HTML"))
			{
				 if (f_open(ehPath("EHPS.DAT"),"a",&pf)) ehExit("CAZ");
				 fprintf(pf,"#HTMLVIEW:%s\r\n",str);
				 f_close(pf);
			}


			if (!strcmp(Find,"EXEC"))
			{
				 //efx3();
				 if (f_open(ehPath("EHPS.DAT"),"a",&pf)) ehExit("CAZ");
				 fprintf(pf,"#EXECMD:%s\r\n",str);
				 f_close(pf);
			}
#endif
			break;

	case WS_OPEN:
		/*
#ifdef _WIN32
					    EHPSCheck();
#endif
		*/
					//printf("WS_OPEN [%s]\n",str); getch();
			if (HdlFile>0) memoFree(HdlFile,"EH4");
			if (HdlBuffer>0) memoFree(HdlBuffer,"EH5");
			HdlFile=-1; HdlBuffer=-1; Buffer=NULL;

			if (!fileCheck(str)) ehExit("%s non esiste",str);
//					os_errset(ONVEDI);
//				pf=fopen(str,"rb");
//					os_errset(POP);
//				if (!pf) return -1;
//				fclose(pf);

			dwSizeFile=fileSize(str); if (dwSizeFile<0) return -1;

			Hdl=fileLoad(str,RAM_AUTO); if (Hdl<0) return -2;
			HdlBuffer=memoAlloc(M_HEAP,BufferSize,"EHPS");
			Buffer=memoPtr(HdlBuffer,NULL);

			HdlFile=memoAlloc(RAM_AUTO,dwSizeFile*2,"EHPSL");
			if (HdlFile<0) {memoFree(Hdl,"EH"); return HdlFile*100;}

			if (!LWin)
			{
				 #ifdef _WIN32
				 win_openEx(EHWP_MOUSECENTER,0,"",363,51,sys.ColorBackGround,0,0,WS_POPUP|WS_BORDER|WS_VISIBLE,FALSE,NULL);
				 #else
				 win_open(-2,65,363,51,2,3,OFF,"");
				 #endif

				 LWin=TRUE;
				 LocalVediIcone();
				 dispf(48,12,0,-1,0,"SMALL F",3,"Compilazione del documento Word Microsoft");
				 dispf(48,30,0,-1,0,"SMALL F",3,"Attendere prego ...");
			}

			// Copio il file nella memoria larga
			Count=0;
			for (;;)
			{
				 Cop=dwSizeFile-Count; if (Cop>BufferSize) Cop=BufferSize;
				 //printf("dwSizeFile %ld Count= %ld Cop=%ld\n",dwSizeFile,Count,Cop);
				 if (Cop<=0) break;
				 memoRead(Hdl,Count,Buffer,(WORD) Cop);
				 memoWrite(HdlFile,Count,Buffer,(WORD) Cop);
				 Count+=Cop;
			}
			memoFree(Hdl,"EH2");
			break;

	// Pulisce tutti campi non utilizzati
	case WS_DEL:

			LocalVediIcone();
			eventGet(NULL);
			if (HdlFile==-1) ehExit("EHP:1");

			Count=0;
			strcpy(FindText,"[#");
			//strcpy(UNIFindText,FindText);
			for (;;)
			{

				 Cop=dwSizeFile-Count; if (Cop>BufferSize) Cop=BufferSize;
				 //printf("dwSizeFile %ld Count= %ld Cop=%ld\n",dwSizeFile,Count,Cop);
				 if (Cop<=0) break;

				 Ricarica2:
				 if (Count<0) ehExit("EHPS:C2"); // Proviamo
				 memoRead(HdlFile,Count,Buffer,(WORD) Cop);

				 Pos=0;
				 for (;;)
				 {
					CHAR *p;

					Pos=BinMultiScan(Buffer,FindText,Pos,BufferSize);
					// ------------------------------------
					// Non ho trovato il Valore           |
					// ------------------------------------
					if (Pos<0) break;
					//----------------------------------------------------------
					// SPECIAL:Richiesta del multiscan di leggere pi— avanti   |
					//----------------------------------------------------------
					if (Pos==GOSTEP) {Count+=BufferSize-40; goto Ricarica2;}
					//----------------------------------------------------------
					// TROVATO: Ma il puntatore ritornato è troppo in fondo    |
					// Ricarico la sezione spostandomi in avanti e rieseguo    |
					//----------------------------------------------------------
					if ((BufferSize-Pos)<35) {Count+=Pos-5; goto Ricarica2;}


					p=Buffer+Pos;

					//----------------------------------------------
					// Verifico la validità del Mark               |
					//----------------------------------------------

					p2=UNItraslate(p,30);
					//printf("DEL [%s] [%d][%d]",p2,Pos,BufferSize); getch();
					p2+=2;
					if ((*p2<'A')||(*p2>'Z')) {Pos++; continue;}
					if (!strstr(p2,"*")) {Pos++; continue;}
					//printf("OK\n");


					//----------------------------------------------
					// Ricerco la Coda                             |
					//----------------------------------------------
					PosF=BinScan(Buffer,"*#]",3,Pos,BufferSize);
					PosF_UNI=BinScan(Buffer,UCEndFind,sizeof(UCEndFind),Pos,BufferSize);

					// -----------------------------------------------------
					// Non ho trovato la CODA mi sposto in avanti di 30    |
					// -----------------------------------------------------
					if ((PosF==-1)&&(PosF_UNI==-1))
						{
						 Count+=30; goto Ricarica2;
						}

					// -------------------------------
					// Possibilità di unicode        |
					// -------------------------------
					if (((PosF_UNI<PosF)&&(PosF_UNI!=-1))||
							((PosF_UNI!=-1)&&(PosF<0)))
							 MaxLen=PosF_UNI-Pos+sizeof(UCEndFind);
							 else
							 MaxLen=PosF-Pos+3;

						if (MaxLen>EHPC_MAX_LINE)
						{CHAR Serv[200];
						 //SINT y;
						 sprintf(Serv,"DEL:Control [%-15.15s..] Len[%d] PosF=%d UPF=%d",
										 UNItraslate(p,15),MaxLen,PosF,PosF_UNI);
						 WIN_fz=OFF;
						 win_info(Serv);
						 WIN_fz=ON;
						 //sprintf(Serv,"%-190.190s",p);
						 //win_info(Serv);
						 /*
						 for (y=0;y<400;y++)
						 {
							printf("y=%d) %c [%d]\n",y,*(p+y),(SINT) *(p+y));
							if (getch()==ESC) break;
						 }
						 */
						 //memset(p,' ',2);
						 UNImemset(p,' ',2);
						 //goto Ricarica2;
						} else UNImemset(p,' ',MaxLen);

					memoWrite(HdlFile,Count,Buffer,(WORD) Cop);
					goto Ricarica2;
				 }

				 Count+=(BufferSize-(strlen(FindText)+2));
				}
				break;

	// Cambio di stringa
	case WS_PROCESS:

			LocalVediIcone();
			//eventGet(NULL);
			if (HdlFile==-1) ehExit("EHP:2");
			//printf("PROCESS [%s][%s]\n",Find,str); getch();
			Count=0;
			Check=TRUE;
			if (*Find=='*') {Find++; FlagAll=TRUE;} else FlagAll=FALSE;

			sprintf(FindText,"[#%s*",Find);
			UNIConversion(UNIFindText,FindText);

			//printf("Cerco [%s]-[%s]\n",FindText,UNIFindText);
			//if (getch()==ESC) ehExit("");
			for (;;)
			{
				 Cop=dwSizeFile-Count; if (Cop>BufferSize) Cop=BufferSize;
				 if (Cop<=0) break;

				 Ricarica:
				 if (Count<0) ehExit("EHPS:C1[%s]",FindText); // Proviamo
				 memoRead(HdlFile,Count,Buffer,(WORD) Cop);

				 Pos=0;
				 for (;;)
				 {
					CHAR *p;
					SINT LenStr;

					Pos=BinMultiScan(Buffer,FindText,Pos,BufferSize);
					// ------------------------------------
					// Non ho trovato il Valore           |
					// ------------------------------------
					if (Pos<0) break;
					//----------------------------------------------------------
					// SPECIAL:Richiesta del multiscan di leggere pi— avanti   |
					//----------------------------------------------------------
					if (Pos==GOSTEP) {Count+=BufferSize-40; goto Ricarica;}

					//----------------------------------------------------------
					// TROVATO: Ma il puntatore ritornato è troppo in fondo    |
					// Ricarico la sezione spostandomi in avanti e rieseguo    |
					//----------------------------------------------------------
					if ((BufferSize-Pos)<35) {Count+=Pos-5; goto Ricarica;}

					p=Buffer+Pos;

					/*
					printf("A) %s | Count=%ld Pos=[%d] <%c%c%c%c%c>\n",
									FindText,Count,Pos,
									*p,*(p+1),*(p+2),*(p+3),*(p+4),*(p+5));
					mouse_input();
					if (key_press(ESC)) break;
					efx2();
						*/

					//----------------------------------------------
					// Ricerco la Coda                             |
					//----------------------------------------------
					PosF=BinScan(Buffer,"*#]",3,Pos,BufferSize);
					PosF_UNI=BinScan(Buffer,UCEndFind,sizeof(UCEndFind),Pos,BufferSize);

					// -----------------------------------------------------
					// Non ho trovato la CODA mi sposto in avanti di 30    |
					// -----------------------------------------------------
					if ((PosF==-1)&&(PosF_UNI==-1))
						 {
							 Count+=30;
							 goto Ricarica;
							}

					// -------------------------------
					// Possibilità di unicode        |
					// -------------------------------
					if (((PosF_UNI<PosF)&&(PosF_UNI!=-1))||
							((PosF_UNI!=-1)&&(PosF<0)))
							 MaxLen=PosF_UNI-Pos+sizeof(UCEndFind);
							 else
							 MaxLen=PosF-Pos+3;

				 /*
					printf("B) PosF=%d PosFUNI[%d] MaxLen=%d\n",
									PosF,PosF_UNI,MaxLen);
					mouse_input(); if (key_press(ESC)) break;
					efx2();
					 */

					//MaxLen=PosF-Pos+3;
					//printf("FindText [%s] MaxLen [%d]\n",FindText,MaxLen); getch();

					if (MaxLen>EHPC_MAX_LINE)
						{CHAR Serv[200];
						 sprintf(Serv,"Controllare campo [%s] Len[%d]",FindText,MaxLen);
						 win_info(Serv);
						 UNImemset(p,' ',2);
						}
						else
						{
						 UNImemset(p,' ',MaxLen);
						 LenStr=strlen(str); if (LenStr>MaxLen) LenStr=MaxLen;
						 //printf("[%s][%s]\n",FindText,str); getch();
						 UNImemcpy(p,str,(WORD) LenStr,MaxLen);
						}

				Check=FALSE;
				memoWrite(HdlFile,Count,Buffer,(WORD) Cop);

				if (!FlagAll) {Count=dwSizeFile;break;}
				goto Ricarica;
			 }
			 Count+=(BufferSize-(strlen(UNIFindText)+2));
			}
			/*
			if (Check) {printf("             NO ? %s\n",Find); getch();}
									else
								 {printf("SI ! %s\n",Find); getch();}
				*/
			return Check;

	// Avvio il server di stampa
	case WS_DO:
					//printf("WS_DO [%s]",Find); getch();
#ifdef WIN32
#else
				if (!strcmp(Find,"GO"))
				 {
					if (f_open(ehPath("EHPS.GO"),"wb+",&pf)) ehExit("CAZ");
					fprintf(pf,"GO!");
					f_close(pf);
				 }

				if (!strcmp(Find,"DEL"))
				 {
					remove(ehPath("EHPS.GO"));
					remove(ehPath("EHPS.DAT"));
				 }
#endif
				break;

	case WS_CLOSE:

			if (HdlFile==-1) ehExit("EHP:3");
			//printf("WS_CLOSE [%s]\n",str); getch();

			if (*str)
			 {

				Count=0;
				rifo:

				pf=fopen(str,"wb");
				if (!pf)
					{if (win_ask("SCRITTURA BLOCCATA\nIl file è in uso da un'altro applicativo\nRiprovo ?")==ESC) return -1;
					 goto rifo;
					// if (Test) return -1;
					}


				for (;;)
				{
				 LocalVediIcone();
				 Cop=dwSizeFile-Count; if (Cop>BufferSize) Cop=BufferSize;
				 //printf("dwSizeFile %ld Count= %ld Cop=%ld\n",dwSizeFile,Count,Cop);
				 if (Cop<=0) break;
				 memoRead(HdlFile,Count,Buffer,(WORD) Cop);
				 fwrite(Buffer,(WORD) Cop,1,pf);
				 Count+=Cop;
				}
				fclose(pf);

			}
			if (LWin) {win_close(); LWin=FALSE;}

#ifdef WIN32
			//win_infoarg("[%s]",str);
//						ShellExecute(WindowNow(),"print",str,NULL,NULL,SW_SHOWMINNOACTIVE);
#ifdef _DEBUG
			ShellExecute(WindowNow(),"open",str,NULL,NULL,SW_SHOWMINNOACTIVE);
#else
			//win_infoarg("[%s]",str);
			if (!*Find)
				ShellExecute(WindowNow(),"open",str,NULL,NULL,SW_SHOWMINNOACTIVE);
			else
				ShellExecute(WindowNow(),"print",str,NULL,NULL,SW_SHOWMINNOACTIVE);
#endif
						
#else
			// Scrittura dell'EHPS
			if (!strcmp(Find,"EHPS"))
			 {
				if (f_open(ehPath("EHPS.DAT"),"a",&pf)) ehExit("CAZ");
				fprintf(pf,"#PRINTDEL:%s\r\n",str);
				f_close(pf);
			 }
#endif
			if (HdlFile>0) memoFree(HdlFile,"EH4");
			if (HdlBuffer>0) memoFree(HdlBuffer,"EH5");
			HdlFile=-1; HdlBuffer=-1; Buffer=NULL;
			break;
 }
 return 0;
}