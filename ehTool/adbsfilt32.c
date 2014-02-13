//   ---------------------------------------------
//   | ADBSFILT32  Special Filter per Dbase      |
//   |             Versione 32bit MThread        |
//   |             by Ferr… Art & Tecnology 1999 |
//   ---------------------------------------------
#include "\ehtool\include\ehsw_idb.h"
#include "\ehtool\mySql.h"

extern struct ADB_INFO *ADB_info;
extern SINT ADB_hdl;
extern SINT ADB_ult; // Numero di MGZ
extern SINT ADB_max;
extern SINT ADB_network;
extern SINT EHPower;

extern SINT ADB_network;   // ON/OFF se si sta usando un NetWork
extern SINT  ADB_HL;   // Se il Link gerarchico Š attivo
extern CHAR HLtabella[];  // Percorso e nome della Tabella Gerarchica
extern LONG adb_HLink;    // Numero di Link Gerarchici
extern SINT  adb_HLinkHdl; // Handle che contiene la tabella

extern SINT  ADB_lock;    // Flag di comportamento in lock

extern struct IPTINFO *IPT_info;
extern SINT IPT_ult; // Numero di input aperti

static BOOL Comparatore(ADBF_ELEM *Element,BOOL ConvNum,CHAR *Valore);

//   +-------------------------------------------+
//   |                                           |
//   |                                           |
//   |             by Ferr… Art & Tecnology 1998 |
//   +-------------------------------------------+

LONG adb_Filter(SINT Hdb,SINT cmd,CHAR *Campo,CHAR *Azione,void *Valore)
{
	ADB_FILTER *Filter;
	ADBF_ELEM sElement;
	ADBF_ELEM *lpElement,*lpElement2;
	CHAR Serv[ADBF_ELEMMAX];
	struct ADB_REC *Fld;
	SINT a,b;//,Cmp;
	BOOL Check;
//	SINT *lpi;
	SINT IndexUse;
	CHAR *lpName;
	SINT Test;
	SINT LastPt;
	SINT SoloElement;
	BOOL Solo;
	LONG Hrec;
	SINT LCount;
	BOOL LStatus;
	CHAR szName[80];
	INT16 *lpsi;
	HWND hProgress;
	LONG lMaxRec;
	HDB  HdbDest;
	CHAR *lpFld;

//	if (((Hdb<0)||(Hdb>=ADB_max))||(ADB_info[Hdb].HdlRam==-1)) ehExit("Flt01 Hdb=%d",Hdb);
	Filter=ADB_info[Hdb].AdbFilter;

#ifdef _ADBEMU
	if (ADB_info[Hdb].iEmuStartPoint) ehExit("Invocato FIlter su HDB in emulazione %d",Hdb);
#endif
	switch (cmd)
	 {
		// -------------------------------------------------------------
		// Informazioni
		// -------------------------------------------------------------
		case WS_INF:
			if (!strcmp(Azione,"?INDEX")||!strcmp(Azione,"?ELEMENT"))
			{
				// --------------------------------------
				// Ricerco quale indice usare           |
				// --------------------------------------
				IndexUse=0;
				Check=FALSE;
				Filter->fBreakExtern=FALSE;
				
				if (Filter->FirstIndex!=-1) IndexUse=Filter->Element[Filter->FirstIndex].iFldType;
				for (a=(Filter->FirstIndex<0)?0:Filter->FirstIndex;a<Filter->NumFilter;a++)
				{
					lpElement=Filter->Element+a; if (!lpElement->iType) continue;

					lpName=lpElement->lpFldName; 
                    strcpy(szName,lpElement->lpFldName);
                        
					// Ricerco se ho una selezione per quel campo
					for (b=0;b<Filter->NumFilter;b++)
					{
						
						lpElement2=Filter->Element+b; if (lpElement2->iType) continue;
							
						if (!strcmp(lpElement2->lpFldName,szName))
						{
							//IndexUse=Filter->Element[a].FldType;
							IndexUse=lpElement->iFldType;
							Check=TRUE;
							break;
						}
					}
					if (Check) break;
				}
				
				if (!strcmp(Azione,"?INDEX"))  * (SINT *) Valore=IndexUse;
				if (!strcmp(Azione,"?ELEMENT")) memcpy(Valore,lpElement,sizeof(sElement));
				return Check;
			}
			
			break;

		case WS_SETFLAG:
			
			if (!strcmp(Azione,"FIXINDEX"))
			{
				Filter->idxInUse=* (SINT *) Valore;
				Filter->idxFloat=FALSE;
			}
			break;

		// -------------------------------------------------------------
		// Apertura del gestore del filtro                             |
		// -------------------------------------------------------------
		case WS_OPEN:
				//Ex:	adb_Filter(HdbEvent,WS_OPEN,"10","",&Filter);

				if (ADB_info[Hdb].AdbFilter!=NULL)  
					{
						//
						// Modalità MultiThread
						// Potrei avere un processo che sta usando l'attuale filtro
						// devo informare che siamo in chiusura ed aspettare 
						//
						//ADB_info[Hdb].AdbFilter->fBreakExtern=TRUE;

						adb_Filter(Hdb,WS_CLOSE,NULL,NULL,NULL);
					}
				Filter=ADB_info[Hdb].AdbFilter=malloc(sizeof(ADB_FILTER));
				//Filter=Valore;
				//if (Filter->hdlFilter==-1) Filter->hdlFilter=-2;
				//if (Filter->hdlFilter!=-2) ehExit("Init Filter!");
				memset(Filter,0,sizeof(ADB_FILTER));
				ADB_info[Hdb].AdbFilter=Filter;
				if (Campo) Filter->MaxFilter=atoi(Campo);
				if (Filter->MaxFilter<1) Filter->MaxFilter=10;
				sprintf(Serv,"*HDBFlt:%02d",Hdb);
				Filter->hdlFilter=memoAlloc(RAM_HEAP,Filter->MaxFilter*sizeof(ADBF_ELEM),Serv);
				Filter->Element=memo_heap(Filter->hdlFilter);
				Filter->FirstIndex=-1;

				Filter->HdlRecord=-1;
				Filter->lpList=NULL;
				Filter->NumRecord=0; 
				Filter->MaxRecord=0;
				Filter->idxInUse=-1;
				Filter->idxFloat=TRUE;
				break;

		// -------------------------------------------------------------
		// Chiusura del Gestore del Filtro                             |
		// -------------------------------------------------------------
		case WS_CLOSE:

				if (Filter==NULL) break;
				//win_time("Chiusura del filtro",1);
				//if (Filter->hdlFilter==-2) break;
				sprintf(Serv,"*HDBFlt:%02d",Hdb);
				memoFree(Filter->hdlFilter,Serv);

				if (Filter->HdlRecord!=-1)
						{
						 memoFree(Filter->HdlRecord,"*HDBRec32");
						}


				//Filter->hdlFilter=-2;
				if (ADB_info[Hdb].AdbFilter!=NULL)  
					{free(ADB_info[Hdb].AdbFilter); 
				     ADB_info[Hdb].AdbFilter=NULL;
					}

				//ADB_info[Hdb].AdbFilter=NULL;
				break;

		// -------------------------------------------------------------
		// Aggiungo un indicazione di filtro                           |
		// -------------------------------------------------------------
		case WS_ADD:
						 if (Filter==NULL) ehExit("ADBSF:02");
						 if (Filter->NumFilter>=Filter->MaxFilter) ehExit("ADBSF:03");
						 lpElement=Filter->Element+Filter->NumFilter;
						 memset(lpElement,0,sizeof(ADBF_ELEM));

						 lpElement->lpFldName=Campo;

						 // -------------------------------------------
						 // Aggiungo un elemento di comparazione      |
						 // -------------------------------------------
						 if (*Azione!='*')
						 {
							lpFld=adb_FldInfo(Hdb,Campo,&Fld);;
							if (lpFld==NULL) ehExit("ADBSF:[%s]?",Campo);
							lpElement->CompType=0;

							// Logica di funzionamento
							// Bit 1 =      Bit 2 <   Bit 3 >
							if (!strcmp(Azione,"=")||!strcmp(Azione,"==")) lpElement->CompType=1;
							if (!strcmp(Azione,">"))  lpElement->CompType=2;
							if (!strcmp(Azione,">=")) lpElement->CompType=3;
							if (!strcmp(Azione,"<"))  lpElement->CompType=4;
							if (!strcmp(Azione,"<=")) lpElement->CompType=5;
							if (!strcmp(Azione,"!=")) lpElement->CompType=6;
							if (!lpElement->CompType) ehExit("ADBSF:04 [%s][%s]",Azione,Valore);
							lpElement->Grp=0; // Uso futuro
							lpElement->FldType=Fld->tipo;
							lpElement->MultiComp=FALSE;
							if (strlen(Valore)>(ADBF_ELEMMAX-1)) ehExit("ADBSF:05");
							strcpy(lpElement->Valore,Valore);

							// Multi comparazione
							if (strstr(Valore,"|"))
								{lpElement->MultiComp=TRUE;}
								else
							// Comparazione Normale
							{
							switch  (lpElement->FldType)
							 {
								 case ADB_ALFA:
											//if (strlen(Valore)>(ADBF_ELEMMAX-1)) ehExit("ADBSF:05");
											//strcpy(lpElement->Valore,Valore);
											break;

								 case ADB_DATA: // Fornire la data ggmmaaaa
											if (strlen(Valore)!=8) ehExit("ADBSF:05");
											strcpy(lpElement->Valore,dateDtoY(Valore));
											break;

								 case ADB_BOOL:
								 case ADB_INT:
											lpElement->fValore=atoi(Valore);
											memset(lpElement->Valore,0,sizeof(lpElement->Valore));
											lpsi=(INT16 *) lpElement->Valore; *lpsi=atoi(Valore);
											break;

								 case ADB_NUME:
											lpElement->fValore=atof(Valore);
											break;
							 }
							}
							Filter->NumComp++;
							}
						 else
						 // -------------------------------------------
						 // Aggiungo una priorit… di indice           |
						 // -------------------------------------------
							{
							 if (Filter->FirstIndex==-1) Filter->FirstIndex=Filter->NumFilter;
							 lpElement->Type=1;
							 lpElement->FldType=adb_indexnum(Hdb,Valore);
							 strcpy(lpElement->Valore,Valore);
							}

						 Filter->NumFilter++;
						 break;

		case WS_COUNT: return Filter->NumFilter;

		// -------------------------------------------------------------
		// Controlla il filtro                                         |
		// -------------------------------------------------------------
		case WS_LINK:
#ifndef _WIN32
						 for (a=0;a<Filter->NumFilter;a++)
						 {
							Element=Filter->Element+a;
							//if (lpElement->Type) continue;
							printf("%d) [%s][%d][%d]  [%.2f][%s]\n",
											lpElement->Type,
											lpElement->lpFldName,
											lpElement->FldType,
											lpElement->CompType,
											lpElement->fValore,
											lpElement->Valore);
						 }
						 getch();
#endif
						 break;

		// -------------------------------------------------------------
		// Controlla se il record ‚ buono                              |
		// Campo=NULL  Controllo di tutti i campi di comparazione      |
		// Campo="num" num=Numero campo da controllare                 |
		// -------------------------------------------------------------
		case WS_PROCESS:
						 if (Filter==NULL) ehExit("ADBSF:12");
						 Check=TRUE;

						 // Richiesta di controllo singolo elemento
						 if (Campo)
							 {Solo=TRUE;
								SoloElement=atoi(Campo);
							 }
							 else
							 {Solo=FALSE;
								SoloElement=0;
							 }

						 for (a=SoloElement;a<Filter->NumFilter;a++)
						 {
							lpElement=Filter->Element+a;
							if (lpElement->Type) continue;

							// ---------------------------------
							// Comparazione semplice           |
							// ---------------------------------
							if (!lpElement->MultiComp)
							{
							 Check=Comparatore(lpElement,FALSE,lpElement->Valore);
							}
							else
							// ---------------------------------
							// Comparazione Multipla           |
							// ---------------------------------
							 {
								CHAR *p;
								CHAR *p2;
								for (p=lpElement->Valore;*p;)
								{
								 p2=strstr(p,"|"); if (p2) *p2=0;
								 //printf("<%s>",p);
								 Check=Comparatore(lpElement,TRUE,p);
								 if (p2) *p2='|';
								 if (Check) break;
								 if (p2) p=p2+1; else break;
								}
							 }


							if (!Check) break;
							if (Solo) break;
						 }


						 return Check;
		// -------------------------------------------------------------
		// Cancella la lista di puntatori                              |
		// (SE c'è)                                                    |
		// -------------------------------------------------------------

		case WS_DEL:
			if (Filter)
			{
			 if (Filter->HdlRecord!=-1) memoFree(Filter->HdlRecord,"*HDBRec32");
			 Filter->lpList=NULL;
			 Filter->HdlRecord=-1;
			 Filter->MaxRecord=0;
			 Filter->NumRecord=0;
			}
			break;

		// -------------------------------------------------------------
		// Prepara la lista di puntatori                               |
		//                                                             |
		// -------------------------------------------------------------

		case WS_DOLIST:

				 IndexUse=0;		 
				 if (Valore) hProgress=(HWND) Valore; else hProgress=NULL;
				 Check=adb_Filter(Hdb,WS_INF,NULL,"?ELEMENT",&sElement);
				 *szName=0;
				 if (Check)
				 {
				   IndexUse=sElement.FldType;
				   //strcpy(szName,sElement.lpFldName);
				   lpName=sElement.lpFldName;
				 }

				 // ----------------------------------------------
				 // Controllo se posso usare l'indice mobile     |
				 // ----------------------------------------------
				 if (Filter->idxFloat)
					{
					 Filter->idxInUse=IndexUse;
					}
					else
					{
					 // Se il filtro voluto è uguale a quello consigliato
					 // Vado avanto come normalmente
					 // altrimenti segnalo con non ho trovato un indice da usare
					 // adatto ed uso Filter->idxInUse
					 if (Filter->idxInUse==IndexUse) Check=TRUE; else Check=FALSE;
					}

					 // -----------------------------------------------------
					 // Se esiste un indice con il primo campo incrimato    |
					 // cioè la possibilità di avvicinarsi al primo record  |
					 // valido                                              |
					 // controllo se c'è una selezione =,>,>= |             |
					 // se SI effettuo in find B_GET_GE                     |
					 // -----------------------------------------------------

						LastPt=-1;
						if (Check)
						{
							Check=FALSE;
							// ------------------------------------------------
							// Ricerco se ho una selezione per quel campo     |
							// ------------------------------------------------
							for (b=0;b<Filter->NumFilter;b++)
							{
							 lpElement=Filter->Element+b;
							 if (lpElement->Type) continue;
							 if (!strcmp(lpElement->lpFldName,lpName))
								{
								 // Controllo sull'uguaglianza
								 if (lpElement->CompType<4)
								 {
//										 printf("Index [%d]>= di %s\n",
//														 IndexUse,
//														 lpElement->Valore);
									switch  (lpElement->FldType)
									{
									 case ADB_ALFA:
									 case ADB_DATA:
										 Test=adb_find(Hdb,
													   Filter->idxInUse,
													   B_GET_GE,
													   lpElement->Valore,
													   B_RECORD);
										  break;

									 case ADB_BOOL:
									 case ADB_INT:
									 case ADB_NUME:
										 Test=adb_Sfind(Hdb,
													   Filter->idxInUse,
													   B_GET_GE,1,
													   lpElement->fValore);
										  break;
									}
									
									// Non trovo niente
									if (Test) return -1;
									if (lpElement->CompType==1) LastPt=b;
									Check=TRUE;
									break;
								 }
								}
							}

							// ---------------------------------------------
							// Ricerco se esiste un limite finale          |
							// ---------------------------------------------

							if (LastPt==-1)
							{
							 for (b=0;b<Filter->NumFilter;b++)
							 {
								lpElement=Filter->Element+b; if (lpElement->Type) continue;
								if (!strcmp(lpElement->lpFldName,lpName))
								 {
									// Controllo sull'uguaglianza
									if ((lpElement->CompType==4)||
										 (lpElement->CompType==5))
										{
										 /*
										 printf("Index [%d]<= di %s\n",
														 Filter->idxInUse,
														 lpElement->Valore);
														 */
										 LastPt=b; break;
										}
								}
							 }
							}// If

						 }

					 // ------------------------------------------------------
					 // Non ho trovato niente effettuo un bel find first     |
					 // ------------------------------------------------------
					 if (!Check) Test=adb_find(Hdb,Filter->idxInUse,B_GET_FIRST,"",B_RECORD);

					 // --------------------------------------------------------
					 // Libero / Richiedo la memoria per la tabella puntatori  |
					 // --------------------------------------------------------
/*
					 if (Filter->HdlRecord!=-1) memoFree(Filter->HdlRecord,Serv);
					 Filter->HdlRecord=-1;
					 if (Test) return -1;
*/
					 sprintf(Serv,"*HDBRec:%02d",Hdb);
					 adb_Filter(Hdb,WS_DEL,NULL,NULL,NULL);

					 Filter->MaxRecord=adb_recnum(Hdb);
					 Filter->HdlRecord=memoAlloc(RAM_AUTO,
												   Filter->MaxRecord*sizeof(HREC),
												   Serv);
					 Filter->NumRecord=0;

					 // Mi tiro fuori il puntatore diretto

					 Filter->lpList=memoLock(Filter->HdlRecord);
					
					 // --------------------------------------------------------
					 // Loop/Controllo/Download puntatori                      |
					 // --------------------------------------------------------
					 LCount=0;
					 LStatus=0;
					 lMaxRec=adb_recnum(Hdb);
					 do {
							 Check=(BOOL) adb_Filter(Hdb,WS_PROCESS,0,0,0);
							 /*
							 printf("*(%2d)[%s][%s] ",
											 Check,
											 adb_FldPtr(Hdb,"DITTA"),
											 adb_FldPtr(Hdb,"IDNET"));
							 if (getch()==ESC) ehExit("CAz");
								 */
							 if (Filter->fBreakExtern) break;
							 
							 LCount++;
							 if (!(LCount%100))
								{
								 if (Valore) 
								 {
									 // x:100=LCount:MaxRec;
									 SendMessage(hProgress, PBM_SETPOS, (WPARAM) (LCount*100/lMaxRec), 0);
								 }
								}


							 if (Check)
								 {
									adb_position(Hdb,&Hrec);
									//memoWrite(Filter->HdlRecord,
									//			   Filter->NumRecord*sizeof(LONG),
									//			   &Hrec,sizeof(LONG));
									Filter->lpList[Filter->NumRecord]=Hrec;
									Filter->NumRecord++;
									//Filter->lpList[Filter->NumRecord]=Filter->NumRecord;
									
									//pausa(500);
									//dispx("%d",Filter->NumRecord);
								 }

							 // ---------------------------------------------------------
							 // Se ho un campo master controllo di non essere arrivato  |
							 // alla fine della ricerca                                 |
							 // ---------------------------------------------------------
							 if (LastPt!=-1)
								 {
									 //printf("<- Fine (?) %d\n",LastPt);
									 sprintf(Serv,"%d",LastPt);
									 Check=adb_Filter(Hdb,WS_PROCESS,Serv,0,0);
									 if (!Check) break;
								 }
								 //else printf("");

							} while (!adb_find(Hdb,Filter->idxInUse,B_GET_NEXT,"",B_RECORD));
					 //printf("Totali: [%ld]\n",Filter->NumRecord);
					 memoUnlock(Filter->HdlRecord);
				
					 //mouse_graph(0,0,"MS01");
					 break;
		
		// -------------------------------------------------------------
		// Chiede una clonazione di filtro                             |
		// Hdb filtro sorgente                                         |
		// Campo="num" num=Numero campo da controllare                 |
		// P.S. Server a adb_OpenClone()                               |
		// -------------------------------------------------------------
		case WS_CLONE:
		
			HdbDest= * (HDB *) Valore;
			if (Filter==NULL) break;

			//adb_Filter(HdbDest,WS_OPEN,Filter->MaxFilter,0,0);
			
			// Creo il nuovo filtro
			sprintf(szName,"%d",Filter->MaxFilter);
			adb_Filter(HdbDest,WS_OPEN,szName,NULL,NULL);
			
			// Copio gli elementi nella nuova destinazione clone
			memcpy(ADB_info[HdbDest].AdbFilter->Element,
			       Filter->Element,
				   Filter->MaxFilter*sizeof(ADBF_ELEM));

			ADB_info[HdbDest].AdbFilter->NumComp=Filter->NumComp;
			ADB_info[HdbDest].AdbFilter->NumFilter=Filter->NumFilter;
			ADB_info[HdbDest].AdbFilter->FirstIndex=Filter->FirstIndex;
			ADB_info[HdbDest].AdbFilter->idxInUse=Filter->idxInUse;
			ADB_info[HdbDest].AdbFilter->idxFloat=Filter->idxFloat;
			
			//dispx("CLONE %d) %d/%d",HdbDest,ADB_info[HdbDest].AdbFilter->idxFloat,ADB_info[HdbDest].AdbFilter->idxInUse); 
			//pausa(4000);

			// Rialloco i puntatori ai campi nelle nuove posizioni del clone
			/*
			for (a=0;a<Filter->NumComp;a++)
			{
				lpElement=Filter->Element+a;
				ADB_info[HdbDest].AdbFilter->Element[a].lpFld=adb_FldInfo(HdbDest,lpElement->lpFldName,&Fld);
				//win_infoarg("%d) [%s]",a,lpElement->lpFldName);
			}
			*/
			break;

	 }
 return 0;
}
/*
static BOOL Comparatore(ADBF_ELEM *lpElement,BOOL ConvNum,CHAR *Valore)
{
 SINT Cmp;
 INT16 *lpi;
 INT16 Val16;
 double fValore;
 BOOL Check;
 CHAR *lpFld;

 if (ConvNum) fValore=atof(Valore); else fValore=lpElement->fValore;

 lpFld=adb_FldPtr(
 switch(lpElement->FldType)
	{
	 case ADB_ALFA:
	 case ADB_DATA: // Fornire la data ggmmaaaa
			 Cmp=strcmp(lpElement->lpFld,Valore);
			 break;

	 case ADB_FLAG:
	 case ADB_INT:
			 Cmp=0;
			 lpi=(INT16 *) lpElement->lpFld;
			 Val16=* (INT16 *) Valore;
			 //if (*lpi>(SINT) fValore) Cmp=1;
			 //if (*lpi<(SINT) fValore) Cmp=-1;
			 if (*lpi>Val16) Cmp=1;
			 if (*lpi<Val16) Cmp=-1;
			 break;

	 case ADB_NUME:
			 Cmp=0;
			 if (atof(lpElement->lpFld)>fValore) Cmp=1;
			 if (atof(lpElement->lpFld)<fValore) Cmp=-1;
			 break;
	}
 // Controllo cosa mi interessa
	Check=FALSE;
	switch (lpElement->CompType)
	 {
		 case 1: // ==
			 if (!Cmp) Check=TRUE;
			 break;
		 case 2: // >
			 if (Cmp>0) Check=TRUE;
			 break;
		 case 3: // >=
			 if (Cmp>=0) Check=TRUE;
			 break;
		 case 4: // <
			 if (Cmp<0) Check=TRUE;
			 break;
		 case 5: // <=
			 if (Cmp<=0) Check=TRUE;
			 break;
		 case 6: // <=
			 if (Cmp) Check=TRUE;
			 break;
	 }
 return Check;
}
*/
static BOOL Comparatore(ADBF_ELEM *lpElement,BOOL ConvNum,CHAR *Valore)
{
 SINT Cmp;
 INT16 *lpi;
 INT16 Val16;
 double fValore;
 BOOL Check;
 if (ConvNum) fValore=atof(Valore); else fValore=lpElement->fValore;

 switch(lpElement->FldType)
	{
	 case ADB_ALFA:
	 case ADB_DATA: // Fornire la data ggmmaaaa
			 Cmp=strcmp(lpElement->lpFldName,Valore);
			 break;

	 case ADB_BOOL:
	 case ADB_INT:
			 Cmp=0;
			 lpi=(INT16 *) lpElement->lpFldName;
			 Val16=* (INT16 *) Valore;
			 //if (*lpi>(SINT) fValore) Cmp=1;
			 //if (*lpi<(SINT) fValore) Cmp=-1;
			 if (*lpi>Val16) Cmp=1;
			 if (*lpi<Val16) Cmp=-1;
			 break;

	 case ADB_NUME:
			 Cmp=0;
			 if (atof(lpElement->lpFldName)>fValore) Cmp=1;
			 if (atof(lpElement->lpFldName)<fValore) Cmp=-1;
			 break;
	}
 // Controllo cosa mi interessa
	Check=FALSE;
	switch (lpElement->CompType)
	 {
		 case 1: // ==
			 if (!Cmp) Check=TRUE;
			 break;
		 case 2: // >
			 if (Cmp>0) Check=TRUE;
			 break;
		 case 3: // >=
			 if (Cmp>=0) Check=TRUE;
			 break;
		 case 4: // <
			 if (Cmp<0) Check=TRUE;
			 break;
		 case 5: // <=
			 if (Cmp<=0) Check=TRUE;
			 break;
		 case 6: // <=
			 if (Cmp) Check=TRUE;
			 break;
	 }
 return Check;
}
