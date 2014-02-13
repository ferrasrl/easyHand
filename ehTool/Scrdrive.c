//   +-------------------------------------------+
//   | SCRdrive  Driver scroll list (ex SCROLL_E)|
//   |          -Versione Nuova                  |
//   |          -Versione 98                     |
//   |           .WS_LAST                        |
//   |           Velocizzata la ricerca col      |
//   |           filtro, tenendo il limiti       |
//   |           della zona di lavoro            |
//   |                                           |
//   |             by Ferrà Art & Technology 1998 |
//   +-------------------------------------------+

#include "/easyhand/inc/easyhand.h"

typedef struct{

	SINT Hdb; 		  // Numero del ADB
	SINT IndexNum; // Numero del KeyPath
	SINT (*FunzDisp)(SINT cmd,
					 struct WS_DISPEXT *DExt,
					 struct WINSCR buf[],
					 CHAR *PtBuf,
					 LONG SizeRec,
					 SINT Hdb,
					 SINT IndexNum);

	SINT  ExtFilter;
	float FiltUP;
	float FiltDN;
	LONG  RecUP;
	LONG  RecDN;

	SINT  memohdl;
	LONG  RecSize;

	CHAR *PtrMemo;
	CHAR *RecBuffer;
	SINT  Dove;
	SINT  End;
} DBEXT;

void sort(void *base,SINT num,SINT dim);

static void DB_GetLast(struct WS_INFO *ws,DBEXT *ADB);
static SINT DB_seek(LONG info,DBEXT *ADB,struct WINSCR buf[],struct WS_INFO *ws);
static SINT DB_perc(LONG info,DBEXT *ADB,struct WINSCR buf[],struct WS_INFO *ws);
static SINT DB_find(SINT cmd,LONG info,CHAR *str,DBEXT *ADB,struct WINSCR buf[],struct WS_INFO *ws);
static void DB_buf(DBEXT *ADB,struct WINSCR buf[],struct WS_INFO *ws);
static SINT DB_load(DBEXT *ADB,struct WINSCR buf[],struct WS_INFO *ws);
static SINT DB_zone(DBEXT *ADB);


// --------------------------------------------------
//  DB_GetLast                                      |
//  Setta il db nella posizione degli ultimi        |
//  record visualizzabili                           |
//                                                  |
//  Non ritorna nulla                               |
//                                                  |
// --------------------------------------------------

static void DB_GetLast(struct WS_INFO *ws,DBEXT *ADB)
{
	SINT a,test;
	LONG Hrec;

	// -----------------------------------------------
	//  Ricalcolo il puntatore partendo dal Basso    |
	//  e tornando indietro                          |
	// -----------------------------------------------

	if (ADB->RecDN)  {a=adb_get(ADB->Hdb,ADB->RecDN,ADB->IndexNum);
					  if (a) ADB->RecDN=ADBNULL;
					}
	if (!ADB->RecDN) {a=adb_find(ADB->Hdb,ADB->IndexNum,
															 B_GET_LAST,"",B_RECORD);}
	if (a) return;

	for (a=0;a<(ws->numcam-1);)
			{
//			 rifo4:

			 // Controllo di essere arrivato in cima
			 if (ADB->RecUP)
				{
				 adb_position(ADB->Hdb,&Hrec);
				 //sonic(200,1,2,3,4,5);
				 if (Hrec==ADB->RecUP) return;
				}

//			 if (adb_find(ADB->Hdb,ADB->IndexNum,B_GET_PREVIOUS,"",B_RECORD)) break;

			 // ------------------------------------------------------
			 //  Se c'è il filtro controllo la validità del record   |
			 // ------------------------------------------------------
			 if (ADB->ExtFilter)
				 {
					test=(ADB->FunzDisp)(WS_FILTER,NULL,NULL,NULL,ADBNULL,ADB->Hdb,ADB->IndexNum);
					//if (test==OFF) break; // Fine della ricerca
					//if ((test==OFF)||(test==SEMIOFF)) goto rifo4; // Campo non valido ma continua
					if (test==ON) a++;
				 }
					else
				 {a++;test=ON;}

			 // Se il record ultimo non c'è me lo trovo
			 if (!ADB->RecDN&&(test==ON)) {adb_position(ADB->Hdb,&ADB->RecDN);}
			 if (adb_find(ADB->Hdb,ADB->IndexNum,B_GET_PREVIOUS,"",B_RECORD)) break;
			}
}

// --------------------------------------------------
//  DB_SEEK sposta la lettura del db in avanti o    |
//          indietro                                |
//                                                  |
//  info = Quanti record                            |
//                                                  |
//  Ritorna : 0 - Nessun spostamento                |
//            1 - Mandare in stampa                 |
//                                                  |
// --------------------------------------------------
static SINT DB_seek(LONG info,
					DBEXT *ADB,
					struct WINSCR buf[],
					struct WS_INFO *ws)
{
	SINT  a,SReale;
	SINT  FlagCORSA=OFF;
	LONG  PtrFind;
	LONG  b;
	SINT  test;
	LONG  Hrec;
	CHAR *p,*p2;
//	TCHAR serv[80];

	// --------------------------------------
	//       SPOSTAMENTO VERSO IL BASSO     !
	// --------------------------------------
	SReale=0;

				if (info>0) // Scroll verso il basso
				{
				 FlagCORSA=OFF;

				 // Leggo l'ultimo record
				 PtrFind=buf[(SINT) (ws->numcam-1)].record;
				 // Se è a NULL pi— in gi— no si può andare
				 if (PtrFind==ADBNULL) return 0;
				 adb_get(ADB->Hdb,PtrFind,ADB->IndexNum);
				 // ------------------------
				 // Scrollo di info in gi— |
				 // ------------------------
				 for (b=0;b<info;b++)
				 {
					rifai2:
					// Fine corsa misurata in apertura
					if (ADB->RecDN)
						{adb_position(ADB->Hdb,&Hrec);
						 if (Hrec==ADB->RecDN) {FlagCORSA=ON; break;}
						}

					if (adb_find(ADB->Hdb,ADB->IndexNum,B_GET_NEXT,"",B_RECORD))
						 {FlagCORSA=ON;
							break;// Fine corsa
						 }

					// ----------------------------------------------------
					// Se c'è il filtro controllo la validità del record  |
					// ----------------------------------------------------

					if (ADB->ExtFilter)
					 {
					 test=(ADB->FunzDisp)(WS_FILTER,NULL,NULL,NULL,
						                  ADBNULL,ADB->Hdb,ADB->IndexNum);
					 if (test==OFF) break; // Fine della ricerca
					 if (test==2) goto rifai2; // Campo non valido ma continua SEMIOFF
					 }

					SReale++; // Conta quanti ne trovo

					// scroll dei movimenti verso l'alto
					p=ADB->PtrMemo;
					p2=farPtr(p,ADB->RecSize);
					memcpy(p,p2,ADB->RecSize*(ws->numcam-1));
					for (a=0;a<(SINT) ws->numcam;a++)
						{//p2=farPtr(p,ADB->RecSize);
						 //memcpy(p,p2,(SINT) ADB->RecSize);
						 buf[a].record=buf[a+1].record;
						 //p=p2;
						}
					// Aggiorna l'ultimo movimento
					adb_position(ADB->Hdb,&buf[(SINT) (ws->numcam-1)].record);
				 }

				// Aggiorna l'offset
				// Se c'è stato fine corsa ricalcolo l'offset
				// (offset può essere sbagliato per arrotondamento in Perc)
				ws->offset+=SReale;
				//if (FlagCORSA) ws.offset=(ws.maxcam-ws.numcam);

				if ((FlagCORSA)&&(info==1)&&(ws->offset==(ws->maxcam-ws->numcam))) return 0;

				// --------------------------------------------------------
				// Se l'offset> supera l'offset massimo                   |
				// Ricalcola l'offset e ristampa tutto                    |
				// --------------------------------------------------------
				if ((FlagCORSA)||(ws->offset>(ws->maxcam-ws->numcam)))
					{
					//  ws.koffset=ws.offset-(ws.maxcam-ws.numcam)-1;
//				sprintf(serv,"offset %ld  maxnum %ld numcam %ld ",
//								ws->offset,ws->maxcam,ws->numcam);
//				Adispfm(0,0,15,0,SET,"SMALL F",3,serv);
					 ws->offset=(ws->maxcam-ws->numcam);
					 ws->refre=ON;
					 ADB->Dove=0; ADB->End=(SINT) ws->numcam;
					 DB_GetLast(ws,ADB);
					 return -1;
					}
				}
				else

				// --------------------------------------
				//       SPOSTAMENTO VERSO L'ALTO       !
				// --------------------------------------
				{
				 PtrFind=buf[0].record;
				 if (PtrFind==ADBNULL) return 0;// Nessun Record

				 adb_get(ADB->Hdb,PtrFind,ADB->IndexNum);
				 for (b=0;b<(info*-1);b++)
				 {
					rifai3:
					 // Fine corsa misurata in apertura
					 if (ADB->RecUP)
						{adb_position(ADB->Hdb,&Hrec);
						 if (Hrec==ADB->RecUP) break;
						}
					if (adb_find(ADB->Hdb,ADB->IndexNum,
											 B_GET_PREVIOUS,"",B_RECORD)) break;// Fine corsa

					if (ADB->ExtFilter)
					 {
					 test=(ADB->FunzDisp)(WS_FILTER,NULL,NULL,NULL,ADBNULL,
																ADB->Hdb,ADB->IndexNum);
					 if (test==OFF) break; // Fine della ricerca

					 if (test==2) goto rifai3; // Campo non valido ma continua
					 }

					 SReale++;

					 for (a=(SINT) (ws->numcam-1);a>0;a--)
							{
							 p=farPtr(ADB->PtrMemo,(ADB->RecSize*(a-1)));
							 p2=farPtr(p,ADB->RecSize);
							 memcpy(p2,p,(SINT) ADB->RecSize);
							 buf[a].record=buf[a-1].record;
							 };

					 adb_position(ADB->Hdb,&buf[0].record);
				 }
				ws->offset-=SReale;
				if ((SReale==0)&&(ws->offset>0)) {ws->offset=0;ws->refre=ON;}
				if (ws->offset<0) {ws->koffset=ws->offset*-1;ws->offset=0;}
				}


				// PRE CONTROLLO PRIMA DEL WS_LOAD
				if (SReale==0) return 0; // Nessun spostamento
				//if (SReale>(ws.numcam-1)) SReale=(SINT) (ws.numcam-1);

				if (info>0) ADB->Dove=(SINT) ws->numcam-SReale; else ADB->Dove=0;
				ADB->End=SReale;
				if (ADB->Dove<0) ADB->Dove=0;
				if (ADB->End>ws->numcam) ADB->End=(SINT) ws->numcam;
				adb_get(ADB->Hdb,buf[ADB->Dove].record,ADB->IndexNum);

				return -1;
}

// --------------------------------------------------
//  DB_PERC sposta la lettura del db ad una         |
//          determinata percentuale                 |
//                                                  |
//  Ritorna : 0 - Nessun spostamento                |
//            1 - Mandare in stampa                 |
//                                                  |
// --------------------------------------------------
static SINT DB_perc(LONG info,
										DBEXT *ADB,
										struct WINSCR buf[],
										struct WS_INFO *ws)
{
	SINT a,test;
	float percent,perc2;
	SINT FineCorsa=OFF;
	LONG hrec,OriRec;

	perc2=(float) ws->maxcam-ws->numcam;
	perc2=perc2*100/ws->maxcam;

	if (info==-1) // Forzo la fine
		{percent=100;
		 ws->offset=(LONG) perc2;
		}
		else
		{
		 percent=(float) info*perc2/(ws->maxcam-ws->numcam);
		 ws->offset=info;

	// Controllo limiti con filtro
	if (ADB->ExtFilter)
					{float Range;
					 Range=ADB->FiltDN-ADB->FiltUP;
					 percent=ADB->FiltUP+((Range/100)*percent);
					}
		}

	// 	Il cursore è settato in testa
	if (percent<.1F) {ws->offset=0;goto goperc1;}

	adb_getperc(ADB->Hdb,ADB->IndexNum,percent);

	// -----------------------------------------------------------
	// Se il filtro è attivo controlla la validità del record    |
	// -----------------------------------------------------------
	if (ADB->ExtFilter)
		{do
		 {
		 test=(ADB->FunzDisp)(WS_FILTER,NULL,NULL,NULL,ADBNULL,ADB->Hdb,ADB->IndexNum);
		 if (test==ON) goto goperc1;

		 if ((ADB->RecUP)||(test==OFF))
				{
				 adb_position(ADB->Hdb,&hrec);
				 //sonic(200,1,2,3,4,5);
				 if (hrec==ADB->RecUP) break;
				}

		 // Sei alla fine
		 if (ADB->RecDN)
				{
				 adb_position(ADB->Hdb,&hrec);
				 //sonic(200,1,2,3,4,5);
				 if (hrec==ADB->RecDN)
					 {
						DB_GetLast(ws,ADB);
						return -1;
					 }
				}

		 } while (!adb_find(ADB->Hdb,ADB->IndexNum,B_GET_PREVIOUS,"",B_RECORD));
		 ws->offset=0;
		}

 goperc1:

 ADB->Dove=0;
 ADB->End=(SINT) ws->numcam;
 buf[0].record=ADBNULL;
 ws->refre=ON;

 // -----------------------------------------------------------
 //  Se l'offset è all'inizio stampa e basta                  |
 // -----------------------------------------------------------

 if (ws->offset<=0)
		{ws->offset=0;
		 //(ADB->FunzDisp)(WS_FIRST,NULL,NULL,NULL,NULL,ADB->Hdb,ADB->IndexNum);

		 if (ADB->RecUP) {a=adb_get(ADB->Hdb,ADB->RecUP,ADB->IndexNum);
											if (a) ADB->RecUP=0;
											}
		 return -1;
		}

 // -----------------------------------------------------------
 //  Controlla quanti ce ne sono                              |
 // -----------------------------------------------------------
 adb_position(ADB->Hdb,&OriRec);
 for (a=1;a<(SINT) ws->numcam;a++)
 {
	erifa:

	// Sei alla fine
	if (ADB->RecDN)
			{adb_position(ADB->Hdb,&hrec);
			 //sonic(200,1,2,3,4,5);
			 if (hrec==ADB->RecDN)
					 {FineCorsa=ON;
						break;
					 }
				}

	if (adb_find(ADB->Hdb,ADB->IndexNum,B_GET_NEXT,"",B_RECORD))
		 {FineCorsa=ON;break;}

	if (ADB->ExtFilter)
		{test=(ADB->FunzDisp)(WS_FILTER,NULL,NULL,NULL,ADBNULL,ADB->Hdb,ADB->IndexNum);
		 if (test!=ON) goto erifa;
		}
 }

 if (FineCorsa)
		 {if (!ADB->RecDN) {adb_position(ADB->Hdb,&ADB->RecDN);}
			ws->offset=ws->maxcam-ws->numcam;DB_GetLast(ws,ADB);
		 }
		 else
		 {adb_get(ADB->Hdb,OriRec,ADB->IndexNum);}

 return -1;
}

// --------------------------------------------------
//  DB_FIND Ricerca un determinato record           |
//                                                  |
//                                                  |
//  Ritorna : 0 - Nessun spostamento                |
//            1 - Mandare in stampa                 |
//                                                  |
// --------------------------------------------------
static SINT DB_find(SINT cmd,
									 LONG info,
									 CHAR *str,
									 DBEXT *ADB,
									 struct WINSCR buf[],
									 struct WS_INFO *ws)
{
	float percent,perc2;
	SINT test;
	// Primo controllo
	if (((*str==0)||(ws->maxcam==0))&&(cmd==WS_FIND)) return 0;

	// Ricerca quello che deve cercare
	test=(ADB->FunzDisp)(cmd,NULL,NULL,str,info,ADB->Hdb,ADB->IndexNum);

	// Controlla che sia buono
	if (test) test=(ADB->FunzDisp)(WS_FILTER,NULL,NULL,NULL,ADBNULL,
																 ADB->Hdb,ADB->IndexNum);

	//printf("test %d",test);
	if (test!=ON) // Se non lo è setta la lista dall'inizio
					 {return 0;}
						else
						adb_position(ADB->Hdb,&ws->selez);

	adb_findperc(ADB->Hdb,ADB->IndexNum,&percent);// Legge la posizione in percentuale
	perc2=percent*(ws->maxcam-1)/100;
	ws->offset=(LONG) perc2;
	ws->refre=ON;
	buf[0].record=ADBNULL;
	ws->dbult=-1; // <-- aggiunta
	ws->refre=ON;

	if (ws->offset>(ws->maxcam-1)) ws->offset=ws->maxcam-1;
	if (ws->offset<0) ws->offset=0;

	//	scr_adbExt1(WS_BUF,0,"");
	ADB->Dove=0; ADB->End=(SINT) ws->numcam;
	DB_buf(ADB,buf,ws);

	if (ws->maxcam<=ws->numcam)
			 {//scr_adbExt1(WS_DBSEEK,ws->numcam*-1,"");}
				if (DB_seek(ws->numcam*-1,ADB,buf,ws)) DB_buf(ADB,buf,ws);
			 }
			 else
			 {SINT a,righe;

				//scr_adbExt1(WS_DBSEEK,(ws->numcam/2)*-1,"");
				if (DB_seek((ws->numcam/2)*-1,ADB,buf,ws)) DB_buf(ADB,buf,ws);
				// Conto quante righe vuote ci sono
				righe=0;
				for (a=0;a<(SINT) ws->numcam;a++) {if (buf[a].record==ADBNULL) righe++;}
				// Ricalibra la fine dbase
				if (righe>0)
					{
					 //scr_adbExt1(WS_DBSEEK,-righe,"");
					 if (DB_seek(-righe,ADB,buf,ws)) DB_buf(ADB,buf,ws);
					 ws->offset=(ws->maxcam-ws->numcam);
					}
			 }

 return -1;
}

// --------------------------------------------------
//  DB_BUF  Carica il buffer con i record letti     |
//                                                  |
//  non ritorna nulla                               |
//                                                  |
// --------------------------------------------------
static void DB_buf(DBEXT *ADB,
				   struct WINSCR buf[],
				   struct WS_INFO *ws)
{
	SINT a,b,test;
	LONG Hrec;

	if (ws->dbult==ADBNULL) // prima volta
		 {if (ws->maxcam==0) {a=0;goto canc;}
			//(ADB->FunzDisp)(WS_FIRST,NULL,NULL,NULL,NULL,ADB->Hdb,ADB->IndexNum);
			if (!ADB->RecUP) DB_zone(ADB);
			ws->offset=0;
			if (!ADB->RecUP) return; // Errore
			// Leggo il primo record
			adb_get(ADB->Hdb,ADB->RecUP,ADB->IndexNum);
			}
			else
			if (ws->dbult==buf[0].record) return; // il buffer è ok

			// -------------------------------------
			//  Carica i nuovi valori nella cache  !
			// -------------------------------------
			for (a=0;a<ADB->End;a++)
			{
			 if (buf[ADB->Dove].keypt) memcpy(buf[ADB->Dove].keypt,ADB->RecBuffer,(SINT) ADB->RecSize);
			 adb_position(ADB->Hdb,&buf[ADB->Dove].record);

			 ADB->Dove++;

			 rifai:
			 // Fine corsa misurata in apertura
			 if (ADB->RecDN)
					{adb_position(ADB->Hdb,&Hrec);
					 if (Hrec==ADB->RecDN) break;
					}
			 if (adb_find(ADB->Hdb,ADB->IndexNum,B_GET_NEXT,"",B_RECORD)) break;
			 if (ADB->ExtFilter)
				{
				 test=(ADB->FunzDisp)(WS_FILTER,NULL,NULL,NULL,ADBNULL,ADB->Hdb,ADB->IndexNum);
				 if (test==OFF) break; // Fine della ricerca

				 if (test==SEMIOFF) goto rifai; // Campo non valido ma continua
				}
			}

			// Devo trovarmi l'ultima posizione valida
			if (!ADB->RecDN)
			 {//win_info("Controlla");
				adb_findperc(ADB->Hdb,ADB->IndexNum,&ADB->FiltDN);
				adb_position(ADB->Hdb,&ADB->RecDN);
			 }


			// Cancella il seguito
			canc:
			for (b=a;b<ADB->End;b++)// era (b=a+1
			 {if (ADB->Dove>=ADB->End) break;
			  buf[ADB->Dove].record=ADBNULL;
			  if (buf[ADB->Dove].keypt!=NULL) strcpy(buf[ADB->Dove].keypt,"");
			  ADB->Dove++;
			 }

			ws->dbult=buf[0].record;
}
// --------------------------------------------------
//  DB_ZONE Stabilisce il primo ed ultimo record    |
//          del dbase                               |
//                                                  |
//                                                  |
//  ritorno NOACTIVE zona globale senza filtro      |
//          OFF      Nessun record                  |
//          ON       Limiti caricati                |
//                                                  |
// --------------------------------------------------
static SINT DB_zone(DBEXT *ADB)
{
	SINT a;
	// -----------------------------------
	// A) Richiesta del Primo Record     |
	// -----------------------------------

	ADB->RecUP=ADBNULL; ADB->RecDN=ADBNULL;
	ADB->FiltUP=0; ADB->FiltDN=0;

	a=(ADB->FunzDisp)(WS_FIRST,NULL,NULL,NULL,ADBNULL,ADB->Hdb,ADB->IndexNum);
	// ----------------------------------------------
	// B) Nessun filtro è attivo                    |
	//    Carico la struttura nel modo ottimale     |
	// ----------------------------------------------

	if (a==NOACTIVE)
		 {ADB->ExtFilter=OFF;
//			ws->maxcam=adb_recnum(ADB->Hdb);
			// Posizioni delle percentuali
			ADB->FiltUP=0; ADB->FiltDN=100;
			adb_position(ADB->Hdb,&ADB->RecUP);
			adb_find(ADB->Hdb,ADB->IndexNum,B_GET_LAST,"",B_RECORD);
			adb_position(ADB->Hdb,&ADB->RecDN);
			return NOACTIVE;
		 }

	// ----------------------------------------------
	// C) Ritorno ad OFF = 0record                  |
	// ----------------------------------------------

	if (a!=ON) return OFF;

	// -------------------------------------------------------------
	// D) A filtro Attivo controllo e registro la Prima Posizione  |
	// -------------------------------------------------------------

	ADB->ExtFilter=ON;

	do {  // Loopizzo
	 a=(ADB->FunzDisp)(WS_FILTER,NULL,NULL,NULL,ADBNULL,ADB->Hdb,ADB->IndexNum);
	 if (a==OFF) return OFF; // Non ci sono record validi
	 if (a==ON) break;     // Trovato
	} while	(!adb_find(ADB->Hdb,ADB->IndexNum,B_GET_NEXT,"",B_RECORD));

	// Registro la prima posizione
	adb_findperc(ADB->Hdb,ADB->IndexNum,&ADB->FiltUP);
	adb_position(ADB->Hdb,&ADB->RecUP);

	// ------------------------------------------------
	// E) Ricerco l'ultima posizione                  |
	// ------------------------------------------------
	a=(ADB->FunzDisp)(WS_LAST,NULL,NULL,NULL,ADBNULL,ADB->Hdb,ADB->IndexNum);

	// Se risponde
	if (a)
		 {
			do {  // Loopizzo
			 a=(ADB->FunzDisp)(WS_FILTER,NULL,NULL,NULL,ADBNULL,ADB->Hdb,ADB->IndexNum);
			 // CONDIZIONE DA CONTROLLARE
			 if (a==OFF) return OFF; // Non ci sono record validi
			 if (a==ON) break;     // Trovato
			 } while (!adb_find(ADB->Hdb,ADB->IndexNum,B_GET_PREVIOUS,"",B_RECORD));

			// Registro l'ultima posizione
			adb_findperc(ADB->Hdb,ADB->IndexNum,&ADB->FiltDN);
			adb_position(ADB->Hdb,&ADB->RecDN);
		 }

 return ON;
}

// --------------------------------------------------
//  DB_WS_LOAD Pre-Caricamento iniziale                |
//                                                  |
//  Ritorna  0 = Tutto OK                           |
//          -1 = Non Extern                         |
//                                                  |
//                                                  |
//                                                  |
// --------------------------------------------------
//	PRG_END("Scr_adbExt1:No disp function");

static SINT DB_load(DBEXT *ADB,
					struct WINSCR buf[],
					struct WS_INFO *ws)
{
	SINT swclex;
	SINT a,test;
	LONG size,Hrec;
	CHAR *p;


	if (ADB->memohdl!=-1) memoFree(ADB->memohdl,"scr9");
	ADB->memohdl=-1;
	ADB->PtrMemo=NULL;
	ADB->RecBuffer=NULL;

	if (ADB->FunzDisp==NULL) return -1;
	ws->maxcam=adb_recnum(ADB->Hdb);
	ws->offset=ADBNULL;
	ws->selez=ADBNULL; // Nessun selezionato
	ws->koffset=-1;
	ws->kselez=-1;
	ws->dispext=ON;
	ws->dbult=ADBNULL;
	ws->refre=ON;

	ADB->RecSize=adb_recsize(ADB->Hdb);
	ADB->RecBuffer=adb_DynRecBuf(ADB->Hdb);
	ws->sizecam=ADB->RecSize; // ???
	size=ADB->RecSize*ws->numcam; // Calcola Grandezza Buffer

	ADB->memohdl=memoAlloc(M_HEAP,size,"DB_load");
	if (ADB->memohdl<0) ehExit("Non memory in DB_load");
	ADB->PtrMemo=memo_heap(ADB->memohdl);

	// Presetta keypt
	p=ADB->PtrMemo;
	for (a=0;a<ws->numcam;a++)
			{buf[a].record=ADBNULL;
	         buf[a].keypt=p; 
			 p=farPtr(p,ADB->RecSize);
			}

	// -------------------------------------------------------------
	// Trova i limiti della zona e conta i record se c'è il filtro |
	// -------------------------------------------------------------
	mouse_graph(0,0,"CLEX");

	ws->maxcam=0;

	a=DB_zone(ADB);

	// Copertura Globale
	if (a==NOACTIVE)
		 {ADB->ExtFilter=OFF; ws->maxcam=adb_recnum(ADB->Hdb);
			goto NU;
		 }

	// Nessun record nei limiti
	if (a==OFF) goto NU;

	// -------------------------------------------------------------
	// Se tutti e due i limiti sono stabiliti chiedo se devo fare  |
	// il calcolo proporzionale                                    |
	// -------------------------------------------------------------

	if (ADB->RecUP&&ADB->RecDN)
		{float perc1,perc2;
		 LONG Rec;

		 a=(ADB->FunzDisp)(WS_DO,NULL,NULL,NULL,ADBNULL,ADB->Hdb,ADB->IndexNum);
		 if (a)
			{
			 adb_get(ADB->Hdb,ADB->RecUP,ADB->IndexNum);
			 adb_findperc(ADB->Hdb,ADB->IndexNum,&perc1);
			 adb_get(ADB->Hdb,ADB->RecDN,ADB->IndexNum);
			 adb_findperc(ADB->Hdb,ADB->IndexNum,&perc2);

			 // Proporzione:  (perc2-perc1):100=x:maxrec
			 Rec=(LONG) ((perc2-perc1)*(float) adb_recnum(ADB->Hdb)/100);
			 ws->maxcam=Rec;
			 goto NU;
			}
		}

	// ---------------------------------
	// Conto i Record                  |
	// ---------------------------------

	adb_get(ADB->Hdb,ADB->RecUP,ADB->IndexNum);
	swclex=0;
	do
	 {
		test=(ADB->FunzDisp)(WS_FILTER,NULL,NULL,NULL,ADBNULL,ADB->Hdb,ADB->IndexNum);

		// Fine della ricerca per fine Dbase
		if (test==OFF) break;

		// Registro la numerazione dei record Buoni
		if (test==ON) ws->maxcam++;

		// Se ho un record di limite lo controllo
		if (ADB->RecDN)
			{adb_position(ADB->Hdb,&Hrec);
			 if (Hrec==ADB->RecDN) break;
			}

		// GRAFICA: Mano che si muove
		if (!(ws->maxcam%30))
			 {swclex^=1;
				if (swclex) mouse_graph(0,0,"CLEX");
										else
										mouse_graph(0,0,"CLEX2");
			 }

	 } while (!adb_find(ADB->Hdb,ADB->IndexNum,B_GET_NEXT,"",B_RECORD));

	 // --------------------------------------------
	 // Se non avevo il record limite lo registro  |
	 // --------------------------------------------

	 if (!ADB->RecDN)
			{adb_position(ADB->Hdb,&ADB->RecDN);
			 adb_findperc(ADB->Hdb,ADB->IndexNum,&ADB->FiltDN);
			}

	 NU:
//	 printf("Trovati %ld record",ws->maxcam);
//	 getch();
	 mouse_graph(0,0,"MS01");
	 return 0;
}

//  +-----------------------------------------+
//	| scr_adbExt1 Funzione driver per adb     |
//	|             con stampa esterna          |
//	|                                         |
//	|                                         |
//	|          by Ferrà Art & Technology 1996 |
//	+-----------------------------------------+
void *scr_adbExt1(SINT cmd,LONG info,CHAR *str)
{
	#define	 MAX_Y 30

	static struct WINSCR buf[MAX_Y],rit;
	static struct WS_INFO ws;
	static DBEXT  ADB;
	static BOOL   Pass=ON;
	SINT    b;

	// CREA un reset della struttura ADB la prima volta
	if (Pass) {ADB.memohdl=-1;
						 ADB.FunzDisp=NULL;
						 ADB.RecBuffer=NULL;
						 ADB.PtrMemo=NULL;
						 Pass=OFF;
						 }

	if (cmd==WS_INF) return &ws;
	switch (cmd) {

	case WS_BUF : //			  			Richiesta buffer

				ADB.Dove=0; ADB.End=(SINT) ws.numcam;
				DB_buf(&ADB,buf,&ws);
				break;

	case WS_DBSEEK : //			  		 Spostamento dell'offset relativo
				if (DB_seek(info,&ADB,buf,&ws)) DB_buf(&ADB,buf,&ws);
				break;

	case WS_DBPERC : //			  				 Ricerca di un offset assoluto
				if (DB_perc(info,&ADB,buf,&ws)) DB_buf(&ADB,buf,&ws);
				break;

	case WS_KEYPRESS :
				if (key_press2(KEY_F9)) {strcpy(str,"ESC:F9"); break;}
				// Transfer del comando a sub-driver
				(ADB.FunzDisp)(cmd,NULL,NULL,str,info,ADB.Hdb,ADB.IndexNum);
				break;

	case WS_FINDKEY: strupr(str);
	case WS_REALSET : //			  						Ricerca la Chiave selezionata
	case WS_FIND : //			  						Ricerca la Chiave selezionata
			 DB_find(cmd,info,str,&ADB,buf,&ws);
			 break;

	case WS_SEL : //			  			Settaggio selez
			 ws.selez=info;
			 if (ADB.FunzDisp==NULL) ehExit("scr_adbExt1:No ext/disp function");
			 (ADB.FunzDisp)(cmd,NULL,NULL,str,info,ADB.Hdb,ADB.IndexNum);
			 break;

	case WS_OFF : //			  			Settaggio offset

			 ehExit("scr_adbExt1:Usato da oggetto O_SCROLL");
			 break;

	case WS_PTREC : //			  			Restituisce pt alla chiave selezionata
			 rit.record=ws.selez;
			 rit.keypt=NULL;
			 for (b=0;b<ws.numcam;b++)// era (b=a+1
			 {if (buf[b].record==ws.selez) {rit.keypt=buf[b].keypt;break;}
			 }
			 return &rit;

	case WS_REALGET:
			 return (&ws.selez);

	case WS_REFON : //			  			       Richiesta di refresh schermo
			 ws.refre=ON;
			 break;

	case WS_REFOFF : //			  											 Schermo rifreshato
			 ws.refre=OFF;
			 break;

	case WS_DISPLAY : //			  			Richiesta buffer
			 if (ADB.FunzDisp==NULL) ehExit("scr_adbExt1:No ext/disp function");
			 (ADB.FunzDisp)(0,
											 (struct WS_DISPEXT *) str,
											 buf,
											 ADB.PtrMemo,
											 ADB.RecSize,
											 ADB.Hdb,
											 ADB.IndexNum);

			 break;

	case WS_SETFILTER : //			  			Richiesta buffer
	case WS_SETFLAG : //			  			Richiesta buffer
			 if (ADB.FunzDisp==NULL) ehExit("scr_adbExt1:No ext/disp function");
			 (ADB.FunzDisp)(cmd,(struct WS_DISPEXT *) &ADB,NULL,str,info,ADB.Hdb,ADB.IndexNum);
			 break;


	case WS_OPEN : //														  PREPARA I DATI
			 if ((info<3)||(info>MAX_Y)) {ehExit("Errore campi scr_adbExt1");}
			 ws.numcam=info;// Assegna il numero di campi da visualizzare

	case WS_LOAD :
			 if (DB_load(&ADB,buf,&ws)) ehExit("Scr_adbExt1:No disp function");
			 break;

	case WS_CLOSE : //														  LIBERA LA MEMORIA
			 //printf("SADB1:Chiusura [%d]",ADB.memohdl); getch();
			 if (ADB.memohdl==-1) break; // Già chiuso
			 memoFree(ADB.memohdl,"scr10");
			 ADB.memohdl=-1;
			 ADB.RecBuffer=NULL;
			 ADB.PtrMemo=NULL;
			 break;

	case WS_EXTFUNZ: //										FUNZIONE DISPLAY DA USARE
			 ADB.FunzDisp=(SINT (*)(SINT cmd,
						  struct WS_DISPEXT *DExt,
						  struct WINSCR buf[],
						  CHAR *PtBuf,
						  LONG SizeRec,
						  SINT Hdb,
						  SINT IndexNum)) str;
			 break;

	case WS_HDB : //														  DBASE DA USARE
			 ADB.Hdb=(SINT) info;
			 // Succede se WS_HDB è prima della dichiarazione della funzione
			 if (ADB.FunzDisp==NULL)
					ehExit("No Funzdisp");
					else
					(ADB.FunzDisp)(WS_HDB,NULL,NULL,str,ADBNULL,ADB.Hdb,ADB.IndexNum);
			 break;

	case WS_IDX : //														  DBASE DA USARE
			 ADB.IndexNum=(SINT) info;
			 // Succede se ADBINX è prima della dichiarazione della funzione
			 if (ADB.FunzDisp==NULL)
					ehExit("No Funzdisp");
					else
					(ADB.FunzDisp)(WS_IDX,(struct WS_DISPEXT *) &ADB,NULL,str,ADBNULL,ADB.Hdb,ADB.IndexNum);

			 break;


//	default : efx1();efx1();
	 }
	return &buf;
#undef MAX_Y

}

//  +-----------------------------------------+
//	| scr_adbExt2 Funzione driver per adb     |
//	|             con stampa esterna          |
//	|                                         |
//	|                                         |
//	|          by Ferrà Art & Technology 1996 |
//	+-----------------------------------------+
void *scr_adbExt2(SINT cmd,LONG info,CHAR *str)
{
	#define	MAX_Y 30

	static struct WINSCR buf[MAX_Y],rit;
	static struct WS_INFO ws;
	static DBEXT ADB;
	static BOOL   Pass=ON;
	SINT    b;
	// CREA un reset della struttura ADB la prima volta
	// CREA un reset della struttura ADB la prima volta
	if (Pass) {ADB.memohdl=-1;
						 ADB.FunzDisp=NULL;
						 ADB.RecBuffer=NULL;
						 ADB.PtrMemo=NULL;
						 Pass=OFF;}

	if (cmd==WS_INF) return &ws;
	switch (cmd) {

	case WS_BUF : //			  			Richiesta buffer

				ADB.Dove=0; ADB.End=(SINT) ws.numcam;
				DB_buf(&ADB,buf,&ws);
				break;

	case WS_DBSEEK : //			  		 Spostamento dell'offset relativo
				if (DB_seek(info,&ADB,buf,&ws)) DB_buf(&ADB,buf,&ws);
				break;

	case WS_DBPERC : //			  				 Ricerca di un offset assoluto
				if (DB_perc(info,&ADB,buf,&ws)) DB_buf(&ADB,buf,&ws);
				break;

	case WS_KEYPRESS :
				if (key_press2(KEY_F9)) {strcpy(str,"ESC:F9"); break;}
				// Transfer del comando a sub-driver
				(ADB.FunzDisp)(cmd,NULL,NULL,str,info,ADB.Hdb,ADB.IndexNum);
				break;

	case WS_FINDKEY: strupr(str);
	case WS_REALSET : //			  						Ricerca la Chiave selezionata
	case WS_FIND : //			  						Ricerca la Chiave selezionata
			 DB_find(cmd,info,str,&ADB,buf,&ws);
			 break;

	case WS_SEL : //			  			Settaggio selez
			 ws.selez=info;
			 if (ADB.FunzDisp==NULL) ehExit("scr_adbExt2:No ext/disp function");
			 (ADB.FunzDisp)(cmd,NULL,NULL,str,info,ADB.Hdb,ADB.IndexNum);
			 break;

	case WS_OFF : //			  			Settaggio offset

			 ehExit("scr_adbExt2:Usato da oggetto O_SCROLL");
			 break;

	case WS_PTREC : //			  			Restituisce pt alla chiave selezionata
			 rit.record=ws.selez;
			 rit.keypt=NULL;
			 for (b=0;b<ws.numcam;b++)// era (b=a+1
			 {if (buf[b].record==ws.selez) {rit.keypt=buf[b].keypt;break;}
			 }
			 return &rit;

	case WS_REALGET:
			 return (&ws.selez);

	case WS_REFON : //			  			       Richiesta di refresh schermo
			 ws.refre=ON;
			 break;

	case WS_REFOFF : //			  											 Schermo rifreshato
			 ws.refre=OFF;
			 break;

	case WS_DISPLAY : //			  			Richiesta buffer
			 if (ADB.FunzDisp==NULL) ehExit("scr_adbExt2:No ext/disp function");
			 (ADB.FunzDisp)(0,
											 (struct WS_DISPEXT *) str,
											 buf,
											 ADB.PtrMemo,
											 ADB.RecSize,
											 ADB.Hdb,
											 ADB.IndexNum);

			 break;

	case WS_SETFILTER : //			  			Richiesta buffer
	case WS_SETFLAG : //			  			Richiesta buffer
			 if (ADB.FunzDisp==NULL) ehExit("scr_adbExt2:No ext/disp function");
			 (ADB.FunzDisp)(cmd,NULL,NULL,str,info,ADB.Hdb,ADB.IndexNum);
			 break;


	case WS_OPEN : //														  PREPARA I DATI
			 ZeroFill(buf);
			 if ((info<3)||(info>MAX_Y)) {ehExit("Errore campi scr_adbExt2");}
			 ws.numcam=info;// Assegna il numero di campi da visualizzare

	case WS_LOAD :
			 if (DB_load(&ADB,buf,&ws)) ehExit("scr_adbExt2:No disp function");
			 break;

	case WS_CLOSE : //														  LIBERA LA MEMORIA
			 if (ADB.memohdl==-1) break; // Già chiuso
			 memoFree(ADB.memohdl,"scr10");
			 ADB.memohdl=-1;
			 ADB.RecBuffer=NULL;
			 ADB.PtrMemo=NULL;
			 break;

	case WS_EXTFUNZ: //										FUNZIONE DISPLAY DA USARE
			 ADB.FunzDisp=(SINT (*)(SINT cmd,
														 struct WS_DISPEXT *DExt,
														 struct WINSCR buf[],
														 CHAR *PtBuf,
														 LONG SizeRec,
														 SINT Hdb,
														 SINT IndexNum)) str;
			 break;

	case WS_HDB : //														  DBASE DA USARE
			 ADB.Hdb=(SINT) info;
			 // Succede se WS_HDB è prima della dichiarazione della funzione
			 if (ADB.FunzDisp==NULL)
					ehExit("No Funzdisp");
					else
					(ADB.FunzDisp)(WS_HDB,NULL,NULL,str,ADBNULL,ADB.Hdb,ADB.IndexNum);
			 break;

	case WS_IDX : //														  DBASE DA USARE
			 ADB.IndexNum=(SINT) info;
			 // Succede se ADBINX è prima della dichiarazione della funzione
			 if (ADB.FunzDisp==NULL)
					ehExit("No Funzdisp");
					else
					(ADB.FunzDisp)(WS_IDX,NULL,NULL,str,ADBNULL,ADB.Hdb,ADB.IndexNum);
			 break;


//	default : efx1();efx1();
	 }
	return &buf;
#undef MAX_Y
}

//  +-----------------------------------------+
//	| scr_adbExt3 Funzione driver per adb     |
//	|             con stampa esterna          |
//	|                                         |
//	|                                         |
//	|          by Ferrà Art & Technology 1996 |
//	+-----------------------------------------+
void *scr_adbExt3(SINT cmd,LONG info,CHAR *str)
{
	#define	 MAX_Y 30

	static struct WINSCR buf[MAX_Y],rit;
	static struct WS_INFO ws;
	static DBEXT ADB;
	static BOOL   Pass=ON;
	SINT    b;
	// CREA un reset della struttura ADB la prima volta
	// CREA un reset della struttura ADB la prima volta
	if (Pass) {ADB.memohdl=-1;
						 ADB.FunzDisp=NULL;
						 ADB.RecBuffer=NULL;
						 ADB.PtrMemo=NULL;
						 Pass=OFF;}

	if (cmd==WS_INF) return &ws;
	switch (cmd) {

	case WS_BUF : //			  			Richiesta buffer

				ADB.Dove=0; ADB.End=(SINT) ws.numcam;
				DB_buf(&ADB,buf,&ws);
				break;

	case WS_DBSEEK : //			  		 Spostamento dell'offset relativo
				if (DB_seek(info,&ADB,buf,&ws)) DB_buf(&ADB,buf,&ws);
				break;

	case WS_DBPERC : //			  				 Ricerca di un offset assoluto
				if (DB_perc(info,&ADB,buf,&ws)) DB_buf(&ADB,buf,&ws);
				break;

	case WS_KEYPRESS :
				if (key_press2(KEY_F9)) {strcpy(str,"ESC:F9"); break;}
				// Transfer del comando a sub-driver
				(ADB.FunzDisp)(cmd,NULL,NULL,str,info,ADB.Hdb,ADB.IndexNum);
				break;

	case WS_FINDKEY: strupr(str);
	case WS_REALSET : //			  						Ricerca la Chiave selezionata
	case WS_FIND : //			  						Ricerca la Chiave selezionata
			 DB_find(cmd,info,str,&ADB,buf,&ws);
			 break;

	case WS_SEL : //			  			Settaggio selez
			 ws.selez=info;
			 if (ADB.FunzDisp==NULL) ehExit("scr_adbExt3:No ext/disp function");
			 (ADB.FunzDisp)(cmd,NULL,NULL,str,info,ADB.Hdb,ADB.IndexNum);
			 break;

	case WS_OFF : //			  			Settaggio offset

			 ehExit("scr_adbExt3:Usato da oggetto O_SCROLL");
			 break;

	case WS_PTREC : //			  			Restituisce pt alla chiave selezionata
			 rit.record=ws.selez;
			 rit.keypt=NULL;
			 for (b=0;b<ws.numcam;b++)// era (b=a+1
			 {if (buf[b].record==ws.selez) {rit.keypt=buf[b].keypt;break;}
			 }
			 return &rit;

	case WS_REALGET:
			 return (&ws.selez);

	case WS_REFON : //			  			       Richiesta di refresh schermo
			 ws.refre=ON;
			 break;

	case WS_REFOFF : //			  											 Schermo rifreshato
			 ws.refre=OFF;
			 break;

	case WS_DISPLAY : //			  			Richiesta buffer
			 if (ADB.FunzDisp==NULL) ehExit("scr_adbExt3:No ext/disp function");
			 (ADB.FunzDisp)( 0,
							 (struct WS_DISPEXT *) str,
							 buf,
							 ADB.PtrMemo,
							 ADB.RecSize,
							 ADB.Hdb,
							 ADB.IndexNum);
			 break;

	case WS_SETFILTER : //			  			Richiesta buffer
	case WS_SETFLAG : //			  			Richiesta buffer
			 if (ADB.FunzDisp==NULL) ehExit("scr_adbExt3:No ext/disp function");
			 (ADB.FunzDisp)(cmd,NULL,NULL,str,info,ADB.Hdb,ADB.IndexNum);
			 break;


	case WS_OPEN : //														  PREPARA I DATI
			 ZeroFill(buf);
			 if ((info<3)||(info>MAX_Y)) {ehExit("Errore campi scr_adbExt3");}
			 ws.numcam=info;// Assegna il numero di campi da visualizzare

	case WS_LOAD :
			 if (DB_load(&ADB,buf,&ws)) ehExit("scr_adbExt3:No disp function");
			 break;

	case WS_CLOSE : //														  LIBERA LA MEMORIA
			 if (ADB.memohdl==-1) break; // Già chiuso
			 memoFree(ADB.memohdl,"scr10");
			 ADB.memohdl=-1;
			 ADB.RecBuffer=NULL;
			 ADB.PtrMemo=NULL;
			 break;

	case WS_EXTFUNZ: //										FUNZIONE DISPLAY DA USARE
			 ADB.FunzDisp=(SINT (*)(SINT cmd,
														 struct WS_DISPEXT *DExt,
														 struct WINSCR buf[],
														 CHAR *PtBuf,
														 LONG SizeRec,
														 SINT Hdb,
														 SINT IndexNum)) str;
			 break;

	case WS_HDB : //														  DBASE DA USARE
			 ADB.Hdb=(SINT) info;
			 // Succede se WS_HDB è prima della dichiarazione della funzione
			 if (ADB.FunzDisp==NULL)
					ehExit("No Funzdisp");
					else
					(ADB.FunzDisp)(WS_HDB,NULL,NULL,str,ADBNULL,ADB.Hdb,ADB.IndexNum);
			 break;

	case WS_IDX : //														  DBASE DA USARE
			 ADB.IndexNum=(SINT) info;
			 // Succede se ADBINX è prima della dichiarazione della funzione
			 if (ADB.FunzDisp==NULL)
					ehExit("No Funzdisp");
					else
					(ADB.FunzDisp)(WS_IDX,NULL,NULL,str,ADBNULL,ADB.Hdb,ADB.IndexNum);
			 break;


//	default : efx1();efx1();
	 }
	return &buf;
#undef MAX_Y
}
// -------------------------------------------------
//  ScrollDeco   Descrizione-Codice                |
//               Funzione di appoggio scr_adbExt?  |
//               Visualizza descrizione e codice   |
//                                                 |
// -------------------------------------------------


SINT ScrollDeco(SINT cmd,struct WS_DISPEXT *DExt,struct WINSCR buf[],
							 CHAR *PtrMemo,LONG RecSize,SINT Hdb,SINT IndexNum)
{
	CHAR *p;
	static SINT ofsdesc=-1;
	static SINT ofscod=-1;
	static SINT dila=32;
	SINT a;
	LONG Hrec;

	switch (cmd)
	{
	case 0:// chiamata dal driver
		if (ofsdesc==-1) ehExit("ScrollCaus");
		if (buf[DExt->ncam].record==ADBNULL) return OFF; // ??
		p=farPtr(PtrMemo,DExt->ncam*RecSize);

		dispfm_h(DExt->px,DExt->py,DExt->col1,DExt->col2,DExt->hdl,p+ofsdesc);

		//a=font_len(p+ofscod,DExt->hdl);
		a=DExt->px+DExt->lx-dila;
		line(a-3,DExt->py,a-3,DExt->py+DExt->ly-1,2,SET);

		dispfm_h(a,
						 DExt->py,
						 DExt->col1,
						 DExt->col2,
						 DExt->hdl,p+ofscod);

		break;

 case WS_HDB: // Cambio di indice

		ofscod =adb_FldOffset(Hdb,PtrMemo);
		dila   =adb_FldSize(Hdb,PtrMemo)*8;
		ofsdesc=adb_FldOffset(Hdb,"DESCRIZIONE");
		break;

 case WS_FINDKEY: // Ricerca del codice
	 RecSize=1;
	 strupr(PtrMemo);

 case WS_FIND: // Ricerca del codice

		if (adb_find(Hdb,(SINT) RecSize,B_GET_GE,PtrMemo,B_RECORD)) {break;}
		adb_position(Hdb,&Hrec);
		adb_get(Hdb,Hrec,IndexNum);
		return ON;

 case WS_IDX: // Cambio di indice
		break;

 case WS_FIRST: // Setta il dbase al primo record

		 if (adb_find(Hdb,IndexNum,B_GET_FIRST,"",B_RECORD)) break;
		 return NOACTIVE;

 case WS_FILTER: // Controllo sul record se è accettabile
		 return ON;

 case WS_SETFILTER: // Setta il dbase al primo record
		 break;

 case WS_KEYPRESS :
		 if (key_press(ESC)) {strcpy(PtrMemo,"ESC:ESC"); break;}
		 break;

 }
 return OFF;
}

/*
//  +-----------------------------------------+
//	| scroll_ex- Esempio di scroll            |
//	|            carica i file della directory|
//	|                                         |
//	|                        by Ferrà 1995/96 |
//	+-----------------------------------------+
void * scroll_ex(SINT cmd,LONG info,CHAR *str)
{
	#define	 MAX_X 20
	#define	 MAX_Y 30

	static struct WINSCR buf[MAX_Y];
	static struct WS_INFO ws;

	LONG a;
	LONG pt;

	static SINT IptMirror;
	static SINT scrhdl=-1;
	struct ffblk file;
	SINT fine;
	static CHAR *p,*pmem=NULL;
	//WORD sgm;
	CHAR serv[MAXDIR];

	CHAR BMS_ico[NOMEICONE+1]; // Icone corrente del mouse
	SINT BMS_ax,BMS_ay;

	//-------------------------------------------------

	if (cmd==WS_INF) return &ws;
	switch (cmd) {

	case WS_BUF : //			  			Richiesta buffer

				if (scrhdl==-1) break;
				for (a=0;a<ws.numcam;a++) {
				 pt=a+ws.offset; if (pt>=ws.maxcam) break;
				 buf[(SINT) a].keypt=pmem+((SINT) pt*MAX_X);
				}
				break;

	case WS_OFF : //			  												Settaggio offset

				ws.offset=info;
				break;

	case WS_FINDKEY:
				strupr(str);

	case WS_FIND : //			  						Ricerca la Chiave selezionata

				if (scrhdl==-1) break;
				a=ws.selez+1;

				if (memcmp(str,pmem+((SINT) a*MAX_X),strlen(str))==0)
						{scroll_ex(WS_OFF,a,"");
						 if (ws.offset>(ws.maxcam-ws.numcam))
								 ws.offset=(ws.maxcam-ws.numcam);
						 scroll_ex(WS_SEL,a,"");
						 break;}
				{

				for(a=0;a<ws.maxcam;a++)
				{
				 if (memcmp(str,pmem+((SINT) a*MAX_X),strlen(str))<=0)
						{scroll_ex(WS_OFF,a,"");
						 if (ws.offset>(ws.maxcam-ws.numcam))
								{ws.offset=(ws.maxcam-ws.numcam);}
						 scroll_ex(WS_SEL,a,"");
						 break;}
				 }
				}
				break;

	case WS_SEL : //			  			Settaggio selez

				ws.selez=info;

				if ((info>-1)&&IptMirror)
					{ipt_write(1,pmem+((SINT) info*MAX_X),0);
					 ipt_vedisolo(1);
					 }

				//sonic(2000,1,1,1,1,6); //pausa(30);
				break;

	case WS_PTREC : //			  			Restituisce pt alla chiave selezionata

				buf[0].record=ws.selez;
				buf[0].keypt=pmem+((SINT) ws.selez*MAX_X);
				break;


	case WS_REFON : //			  			       Richiesta di refresh schermo

				ws.refre=ON;
				break;

	case WS_REFOFF : //			  											 Schermo rifreshato

				ws.refre=OFF;
				break;

	case WS_OPEN : //														  PREPARA I DATI

				if ((info<3)||(info>MAX_Y))
						{
						ehExit("Errore campi scroll_ex");
						}

				ws.sizecam=MAX_X;
				ws.numcam=info;// Assegna il numero di campi da visualizzare

	case WS_LOAD :

				if (scrhdl>-1) memoFree(scrhdl,"scr1");// Libera la memoria

				scrhdl=-1;
				ws.maxcam=0;
				ws.offset=0;
				ws.selez=-1;
				ws.koffset=-1;
				ws.kselez=-1;
				ws.dispext=OFF;
				ws.refre=ON;

				//	Conta i file
				*serv=0;
				//if (*str) strcpy(serv,str);
				//strcpy(serv,pathcur);
				strcpy(serv,"*.*");
				//strcat(serv,extcur); //strcat(serv,extcur);

				// Cambia il mouse
				strcpy(BMS_ico,MS_ico);
				BMS_ax=MS_ax; BMS_ay=MS_ay;
				mouse_graph(0,0,"CLEX");

				dos_errset(OFF);

				fine=f_findfirst(serv,&file,0);
				while (!fine) {ws.maxcam++; fine=f_findnext(&file);}

				// Non ci sono pi— files
				if ((fine)&&(DE_coden==0x12)) {fine=0;}

				dos_errset(POP);

				if (fine) dos_errvedi("listfile()\n");

				if (ws.maxcam==0) goto FINEC;//	No file

				scrhdl=memoAlloc(M_HEAP,
													 (LONG) ws.maxcam*MAX_X,
													 "listfile()");

				if (scrhdl<0) ehExit("Memoria insufficiente in line __LINE__");
				pmem=memo_heap(scrhdl);

				//	Copia i nomi dei file in memoria

				dos_errset(OFF);

				fine=f_findfirst(serv,&file,0);
				if (fine) goto FINEC;
				p=pmem;
				while (!fine) {
					 strcpy(p,file.ff_name);
					 p+=MAX_X;
					 fine=f_findnext(&file);
				}

				dos_errset(POP);

				if ((fine)&&(DE_coden==0x12)) {fine=0;}
				if (fine) dos_errvedi("");

			//	ORDINA I FILE IN MODO ALFABETICO
				sort(pmem,(SINT) ws.maxcam,MAX_X);

				FINEC:
				mouse_graph(BMS_ax,BMS_ay,BMS_ico);

				return (SINT *) fine;
				//break;

	case WS_CLOSE : //														  LIBERA LA MEMORIA

				if (scrhdl>-1) memoFree(scrhdl,"scr2");// Libera la memoria
				scrhdl=-1;
				break;

//	default : efx1();efx1();
	 }
	return &buf;
#undef MAX_X
#undef MAX_Y

}
*/
void ScrVirtualClick(struct OBJ *poj,struct WS_INFO *ws)
{
	SINT a,lx;
	SINT x1,y1,x2;
	SINT fontalt;
	LONG selez;
	SINT cam_y;
	LONG sl2;

	lx=atoi(poj->text);
	fontalt=font_alt(poj->idxFont);

	x1=poj->px;  y1=poj->py;
	x2=x1+lx+8;//+22;

	selez=ws->selez;
	//cam_x=x1+4;
	cam_y=y1+5;

	if ((sys.ms_x<(x1+relwx))||(sys.ms_x>(x2+relwx))) return;

	a=(SINT) sys.ms_y-(SINT) cam_y-(SINT) relwy+1;
	if (a<0) sl2=-1; else {a/=(SINT) fontalt; sl2=ws->offset+a;}
	if ((sl2>-1)&&(sl2<ws->maxcam)) selez=sl2; else selez=-1;
	(poj->funcExtern)(poj,WS_SEL,selez,"");
}