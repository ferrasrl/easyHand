//   ---------------------------------------------
//   | ADBSFILT  Special Filter per Dbase     	 
//   |                                           
//   |             by Ferrà Art & Technology 1998 
//   |             by Ferrà srl 2007
//   ---------------------------------------------
#include "/easyhand/inc/easyhand.h"

 // extern struct ADB_INFO *ADB_info;

 extern SINT ADB_hdl;
 extern SINT ADB_ult; // Numero di MGZ
 extern SINT ADB_max;
 extern SINT ADB_network;
 
 extern SINT ADB_network;   // ON/OFF se si sta usando un NetWork
 extern SINT  ADB_HL;   // Se il Link gerarchico è attivo
 extern CHAR HLtabella[];  // Percorso e nome della Tabella Gerarchica
 extern LONG adb_HLink;    // Numero di Link Gerarchici
 extern SINT  adb_HLinkHdl; // Handle che contiene la tabella

 extern SINT  ADB_lock;    // Flag di comportamento in lock

 extern struct IPTINFO *IPT_info;
 extern SINT IPT_ult; // Numero di input aperti

static BOOL Comparatore(HDB hdb,ADBF_ELEM *Element,BOOL ConvNum,CHAR *Valore);

//   
//  adb_Filter()
//  Filtro di ricerca agganciato ad adb
//                                     
//                                     
//                                     
//                by Ferrà Art & Technology 1998 |
//
//				  Reso compatibile MT 11/2007
//	 							Ferrà srl 2007
//   +-------------------------------------------+

LONG adb_Filter(HDB hdb,SINT iCommand,CHAR *pCampo,CHAR *pAzione,void *pValore)
{
	ADB_FILTER *Filter;
//	ADBF_ELEM *Element;
	ADBF_ELEM sElement,*psElement,*psElement2;
	CHAR szServ[ADBF_ELEMMAX];
	struct ADB_REC *Fld;
	SINT a,b,x;//,Cmp;
	BOOL bIndexRead,bGood;
	SINT IndexUse;
	CHAR *lpName;
	SINT Test;
	SINT LastPt;
	SINT SoloElement;
	BOOL Solo;
	SINT LCount;
	BOOL LStatus;
	CHAR szName[80];
	INT16 *lpsi;
	CHAR *lpFld;
	HDB hdbDest;

//	if (((hdb<0)||(hdb>=ADB_max))||(ADB_info[hdb].HdlRam==-1)) ehExit("Flt01 Hdb=%d",hdb);
	if ((hdb<0)||(hdb>=ADB_max)) ehExit("Flt01 hdb=%d",hdb);
	Filter=ADB_info[hdb].AdbFilter;

	//win_infoarg("%d",hdb);
	switch (iCommand)
	 {
		// -------------------------------------------------------------
		// Informazioni
		// -------------------------------------------------------------
		case WS_INF:

			if (!strcmp(pAzione,"?INDEX")||
				!strcmp(pAzione,"?ELEMENT"))
			{
				// --------------------------------------
				// Ricerco quale indice usare           |
				// --------------------------------------
				IndexUse=0;
				bIndexRead=FALSE;
				Filter->fBreakExtern=FALSE;
				
				if (Filter->FirstIndex!=-1) IndexUse=Filter->Element[Filter->FirstIndex].iFldType;
				for (a=(Filter->FirstIndex<0)?0:Filter->FirstIndex;a<Filter->NumFilter;a++)
				{
					psElement=Filter->Element+a; 
					if (!psElement->iType) continue;

					lpName=psElement->lpFldName; 
                    strcpy(szName,psElement->lpFldName);

					// Ricerco se ho una selezione per quel campo
					for (b=0;b<Filter->NumFilter;b++) 
					{
						
						psElement2=Filter->Element+b; if (psElement2->iType) continue;
							
						if (!strcmp(psElement2->lpFldName,szName))
						{
							//IndexUse=Filter->Element[a].iFldType;
							IndexUse=psElement->iFldType;
							bIndexRead=TRUE;
							break;
						}
					}
					if (bIndexRead) break;
				}
				
				if (!strcmp(pAzione,"?INDEX"))  * (SINT *) pValore=IndexUse;
				if (!strcmp(pAzione,"?ELEMENT")) memcpy(pValore,psElement,sizeof(ADBF_ELEM));
				return bIndexRead;
			}
			break;

		case WS_SETFLAG:
			
			if (!strcmp(pAzione,"FIXINDEX"))
			{
				Filter->idxInUse=* (SINT *) pValore;
				Filter->idxFloat=FALSE;
			}

			if (!strcmp(pAzione,"MT"))
			{
				Filter->iMode=1;
				Filter->hwndProgress=pValore;
			}
			break;

		// -------------------------------------------------------------
		// Apertura del gestore del filtro                             |
		// -------------------------------------------------------------
		case WS_OPEN:
				//Ex:	adb_Filter(hdbEvent,WS_OPEN,"10","",&Filter);
//				if (pValore!=NULL) win_infoarg("Obsoleto:\nTogliere il passaggio del adb_filter ad adb_filter(WS_OPEN...\ne togliere adb_filter(WS_CLOSE...");
				if (ADB_info[hdb].AdbFilter)  
					{
					 adb_Filter(hdb,WS_CLOSE,NULL,NULL,NULL);
					}
				
				Filter=ADB_info[hdb].AdbFilter=ehAlloc(sizeof(ADB_FILTER));

				memset(Filter,0,sizeof(ADB_FILTER));
				ADB_info[hdb].AdbFilter=Filter;
				if (pCampo) Filter->MaxFilter=atoi(pCampo);
				if (Filter->MaxFilter<1) Filter->MaxFilter=10;
				sprintf(szServ,"*hdbFilter:%02d",hdb);
				Filter->hdlFilter=memoAlloc(RAM_AUTO,Filter->MaxFilter*sizeof(ADBF_ELEM),szServ);
				Filter->Element=memoLock(Filter->hdlFilter);
				Filter->FirstIndex=-1;

				// Area di allocazione dei records
				Filter->hdlRecords=-1;
				Filter->arRecord=NULL;
				Filter->idxInUse=-1;
				Filter->idxInUse=TRUE;
				break;

		// -------------------------------------------------------------
		// Chiusura del Gestore del Filtro                             |
		// -------------------------------------------------------------
		case WS_CLOSE:
				if (Filter==NULL) break;

				// Controllo se esiste una funzione esterna 
				if (Filter->ExtProcess) {(Filter->ExtProcess)(WS_CLOSE,hdb,Filter);}

				// Loop sugli elementi per liberare eventuali risorse impegnate
				 for (a=0;a<Filter->NumFilter;a++)
				 {
					psElement=Filter->Element+a;
					if (psElement->iType==12)
					{
						ehFreeNN(psElement->pBuffer);
						if (psElement->arFields) {ARDestroy(psElement->arFields); psElement->arFields=NULL;}
						if (psElement->arStringSearch) {ARDestroy(psElement->arStringSearch); psElement->arStringSearch=NULL;}
					}
				 }

				//if (Filter->HdlFilter==-2) break;
				adb_Filter(hdb,WS_DEL,NULL,NULL,NULL);	
				sprintf(szServ,"*hdbFilter:%02d",hdb); memoFree(Filter->hdlFilter,szServ);
	
				if (ADB_info[hdb].AdbFilter!=NULL)  
					{ehFree(ADB_info[hdb].AdbFilter); 
				     ADB_info[hdb].AdbFilter=NULL;
					}
				//ADB_info[hdb].AdbFilter=NULL;
				break;

		// -------------------------------------------------------------
		// Aggiungo un indicazione di filtro                           |
		// -------------------------------------------------------------
		case WS_ADD:
				 if (Filter==NULL) ehExit("ADBSF:02");
				 if (Filter->NumFilter>=Filter->MaxFilter) ehExit("ADBSF:03");
				 if (Filter->ExtProcess) ehExit("ADBSF:03B");

				 // -------------------------------------------
				 // Processo esterno New 2002
				 // -------------------------------------------
				 if (*pAzione=='#')
				 {
					Filter->ExtProcess=pValore;
					// Controllo se esiste una funzione esterna 
					(Filter->ExtProcess)(WS_OPEN,hdb,Filter);
					break;
				 }
				 
				 psElement=Filter->Element+Filter->NumFilter;
				 memset(psElement,0,sizeof(ADBF_ELEM));

				 // -------------------------------------------
				 // Aggiungo un elemento di comparazione      |
				 // -------------------------------------------
				 if (*pAzione!='*')
				 {
					// Logica di funzionamento
					// Bit 1 =      Bit 2 <   Bit 3 >
					if (!strcmp(pAzione,"=")||!strcmp(pAzione,"==")) psElement->iCompType=1;
					if (!strcmp(pAzione,">"))  psElement->iCompType=2;
					if (!strcmp(pAzione,">=")) psElement->iCompType=3;
					if (!strcmp(pAzione,"<"))  psElement->iCompType=4;
					if (!strcmp(pAzione,"<=")) psElement->iCompType=5;
					if (!strcmp(pAzione,"!=")) psElement->iCompType=6;
					
					// New 99 Ricerca con search
					if (!strcmp(pAzione,">>")) psElement->iCompType=7;

					// New 2001 Ricerca Begin with /End with
					if (!strcmp(pAzione,"->")) psElement->iCompType=8;
					if (!strcmp(pAzione,"<-")) psElement->iCompType=9;
					
					// New 2003 Ricerca con search
					if (!strcmp(pAzione,">+")) psElement->iCompType=10;
					
					// New 2006 Ricerca con strstr Case insensible
					if (!strcmp(pAzione,">#")) psElement->iCompType=11;

					// New 2009 Ricerca con and
					if (!strcmp(pAzione,"&")) psElement->iCompType=13;

					//
					// New 2007 
					// Ricerca tipo Google con Multi campo di ricerca
					//
					if (!strcmp(pAzione,">$")) 
					{
						BYTE *pData=strDup(pValore);
						psElement->iCompType=12;					
						
						// Cerco Stringhe di ricerca
						_strlwr(pData);
						while (strReplace(pData,"  "," "));
						psElement->arStringSearch=ARCreate(pData," ",NULL);
						ehFree(pData);


						pData=strDup(pCampo);
						psElement->arFields=ARCreate(pData,"+",NULL);
						for (x=0;psElement->arFields[x];x++)
						{
							lpFld=adb_FldInfo(hdb,psElement->arFields[x],&Fld);
							if (lpFld==NULL) ehExit("ADBSF: [%s] > [%s] ?",pCampo,psElement->arFields[x]);
						}
						ehFree(pData);

						psElement->iFldType=1000; // Multi campo
						psElement->pBuffer=ehAlloc(1024); // <- Dovrebbe essere parametrico

					}
					else // Campo normale
					{
						lpFld=adb_FldInfo(hdb,pCampo,&Fld); if (lpFld==NULL) ehExit("ADBSF:[%s]?",pCampo);
 						psElement->lpFldName=Fld->desc;
						psElement->iFldType=Fld->tipo;
						if (strstr(pValore,"|")) psElement->bMultiComp=TRUE;
					}

					if (!psElement->iCompType) ehExit("ADBSF:04 [%s][%s]",pAzione,pValore);
					psElement->iGrp=0; // Uso futuro
					psElement->bMultiComp=FALSE;
					if (strlen(pValore)>(ADBF_ELEMMAX-1)) ehExit("ADBSF:05");
					strcpy(psElement->szValore,pValore);

					if (!psElement->bMultiComp)
					{
						switch(psElement->iFldType)
						 {
							 case ADB_ALFA:
							 case ADB_BLOB:
										//if (strlen(pValore)>(ADBF_ELEMMAX-1)) ehExit("ADBSF:05");
										//strcpy(psElement->Valore,Valore);
										break;

							 case ADB_DATA: // Fornire la data ggmmaaaa
										if (strlen(pValore)!=8) ehExit("ADBSF:05");
										strcpy(psElement->szValore,dateDtoY(pValore));
										break;

							 case ADB_BOOL:
							 case ADB_INT:
										psElement->fValore=atoi(pValore);
										memset(psElement->szValore,0,sizeof(psElement->szValore));
										lpsi=(INT16 *) psElement->szValore; *lpsi=atoi(pValore);
										break;

							 case ADB_INT32:
							 case ADB_AINC:
										psElement->fValore=atoi(pValore);
										//memset(psElement->Valore,0,sizeof(psElement->Valore));
										//lpi=(INT *) psElement->Valore; *lpi=atoi(pValore);
										break;

							 case ADB_NUME:
							 case ADB_COBD:
							 case ADB_COBN:
										psElement->fValore=atof(pValore);
										break;
						 }
					}
//				  }
					Filter->NumComp++;
				 }
				 else
				 // -------------------------------------------
				 // Aggiungo una priorità di indice           |
				 // -------------------------------------------
					{
					 lpFld=adb_FldInfo(hdb,pCampo,&Fld);
				     // Lo Faccio puntare nel dbase
					 psElement->lpFldName=Fld->desc;
					 if (Filter->FirstIndex==-1) Filter->FirstIndex=Filter->NumFilter;
					 psElement->iType=1;
					 psElement->iFldType=adb_indexnum(hdb,pValore);
					 strcpy(psElement->szValore,pValore);
					}

				 Filter->NumFilter++;
				 break;

		case WS_COUNT: return Filter->NumFilter;

		// -------------------------------------------------------------
		// Controlla il filtro                                         |
		// -------------------------------------------------------------
		case WS_LINK:
			 for (a=0;a<Filter->NumFilter;a++)
			 {
				psElement=Filter->Element+a;
				//if (psElement->iType) continue;
				win_infoarg("%d) [%s][%d][%d]  [%.2f][%s]\n",
								psElement->iType,
								psElement->lpFldName,
								psElement->iFldType,
								psElement->iCompType,
								psElement->fValore,
								psElement->szValore);
			 }
			 //getch();
			 break;

		// -------------------------------------------------------------
		// Controlla se il record ‚ buono                              |
		// Campo=NULL  Controllo di tutti i campi di comparazione      |
		// Campo="num" num=Numero campo da controllare                 |
		// -------------------------------------------------------------
		case WS_PROCESS:
				 if (Filter==NULL) ehExit("ADBSF:12");
				 bGood=TRUE;

				 // Richiesta di controllo singolo elemento
				 if (pCampo)
					 {Solo=TRUE; SoloElement=atoi(pCampo);
					 }
					 else
					 {Solo=FALSE; SoloElement=0;
					 }

				 for (a=SoloElement;a<Filter->NumFilter;a++)
				 {
					psElement=Filter->Element+a;
					if (psElement->iType) continue;

					// ---------------------------------
					// Comparazione semplice           |
					// ---------------------------------
					if (!psElement->bMultiComp)
					{
					 bGood=Comparatore(hdb,psElement,FALSE,psElement->szValore);
					}
					else
					// ---------------------------------
					// Comparazione Multipla           |
					// ---------------------------------
					 {
						CHAR *p;
						CHAR *p2;
						for (p=psElement->szValore;*p;)
						{
						 p2=strstr(p,"|"); if (p2) *p2=0;
						 //printf("<%s>",p);
						 bGood=Comparatore(hdb,psElement,TRUE,p);
						 if (p2) *p2='|';
						 if (bGood) break;
						 if (p2) p=p2+1; else break;
						}
					 }


					if (!bGood) break;
					if (Solo) break;
				 }
				 return bGood;


		// -------------------------------------------------------------
		// Prepara la lista di puntatori                               |
		//                                                             |
		// -------------------------------------------------------------

		case WS_DOLIST:

				lpName=NULL;
				IndexUse=0;		 
				bIndexRead=adb_Filter(hdb,WS_INF,NULL,"?ELEMENT",&sElement);
				*szName=0;
				if (bIndexRead)
				{
					IndexUse=sElement.iFldType;
					lpName=sElement.lpFldName;
				}

				//
				// CERCO IL PRIMO RECORD
				//
				// Controllo se posso usare l'indice mobile     
				//
				if (Filter->idxFloat)
					{
						Filter->idxInUse=IndexUse;
					}
					else
					{
					 // Se il filtro voluto ‚ uguale a quello consigliato
					 // Vado avanto come normalmente
					 // altrimenti segnalo con non ho trovato un indice da usare
					 // adatto ed uso Filter->IndexInUse
						if (Filter->idxInUse==IndexUse) bIndexRead=TRUE; else bIndexRead=FALSE;
					}

				 // -----------------------------------------------------
				 // Se esiste un indice con il primo campo incrimato    
				 // cioè la possibilità di avvicinarsi al primo record  
				 // valido                                              
				 // controllo se c'è una selezione =,>,>= |             
				 // se SI effettuo in find B_GET_GE                     
				 // -----------------------------------------------------
					LastPt=-1;
					if (!lpName) bIndexRead=FALSE;
					if (bIndexRead)
					{
						bIndexRead=FALSE;
						// ------------------------------------------------
						// Ricerco se ho una selezione per quel campo     |
						// ------------------------------------------------
						for (b=0;b<Filter->NumFilter;b++)
						{
						 psElement=Filter->Element+b;
						 if (psElement->iType) continue;
						 if (!strcmp(psElement->lpFldName,lpName))
							{
							 // Controllo sull'uguaglianza
							 if (psElement->iCompType<4)
							 {
//										 printf("Index [%d]>= di %s\n",
//														 IndexUse,
//														 psElement->Valore);
								switch  (psElement->iFldType)
								{
								 case ADB_ALFA:
								 case ADB_DATA:
									 Test=adb_find(	hdb,
													Filter->idxInUse,
													B_GET_GE,
													psElement->szValore,
													B_RECORD);
									  break;

								 case ADB_BOOL:
								 case ADB_INT:
								 case ADB_INT32:
								 case ADB_AINC:
								 case ADB_NUME:
								 case ADB_COBD:
								 case ADB_COBN:
									  Test=adb_Sfind(hdb,
													 Filter->idxInUse,
													 B_GET_GE,1,
													 psElement->fValore);
									  //win_infoarg("Test = %d [%.0f]",Test,psElement->fValore);
									  break;
								}
								// Non trovo niente
								if (Test) return -1;
								if (psElement->iCompType==1) LastPt=b;
								bGood=TRUE;
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
							psElement=Filter->Element+b; if (psElement->iType) continue;
							if (!strcmp(psElement->lpFldName,lpName))
							 {
								// Controllo sull'uguaglianza
								if ((psElement->iCompType==4)||
									 (psElement->iCompType==5))
									{
									 /*
									 printf("Index [%d]<= di %s\n",
													 Filter->IndexInUse,
													 psElement->Valore);
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
					 if (!bIndexRead) Test=adb_find(hdb,Filter->idxInUse,B_GET_FIRST,NULL,B_RECORD);

					 //
					 // Pulisco
					 // 
					 adb_Filter(hdb,WS_DEL,NULL,NULL,NULL); // Cancello l'eventuale lista precedentemente memorizzata

					 //
					 // Alloco ricorse
					 // 
					 Filter->iRecordsMax=adb_recnum(hdb);
					 Filter->iRecordsReady=0;
					 sprintf(szServ,"*hdbFltRecs");
					 if (!Filter->iRecordsMax) break;

					 Filter->hdlRecords=memoAlloc(RAM_AUTO,
													Filter->iRecordsMax*sizeof(LONG),
													szServ);
					 Filter->arRecord=memoLock(Filter->hdlRecords);

					 // --------------------------------------------------------
					 // Loop/Controllo/Download puntatori                      |
					 // --------------------------------------------------------
					 LCount=0;
					 LStatus=0;
					 do {
							 bGood=(BOOL) adb_Filter(hdb,WS_PROCESS,0,0,0);
							 if (Filter->fBreakExtern) break;
							 LCount++;
							 //
							 // Ogni cento records, aggiorno la barra di progressione
							 //
							 if (!(LCount%100)&&Filter->iMode==1)
								{
								 if (Filter->hwndProgress&&Filter->iRecordsMax) 
								 {
									 // x:100=LCount:MaxRec;
									 SendMessage(Filter->hwndProgress, PBM_SETPOS, (WPARAM) (LCount*100/Filter->iRecordsMax), 0);
								 }
								}

							 if (bGood)
							 {
								adb_position(hdb,Filter->arRecord+Filter->iRecordsReady);
								Filter->iRecordsReady++;
							 }

							 // ---------------------------------------------------------
							 // Se ho un campo master controllo di non essere arrivato  |
							 // alla fine della ricerca                                 |
							 // ---------------------------------------------------------
							 if (LastPt!=-1)
								 {
									 //printf("<- Fine (?) %d\n",LastPt);
									 sprintf(szServ,"%d",LastPt);
									 if (!adb_Filter(hdb,WS_PROCESS,szServ,0,0)) break;
								 }
								 //else printf("");

					} while (!adb_find(hdb,Filter->idxInUse,B_GET_NEXT,"",B_RECORD));
					 break;

		// -------------------------------------------------------------
		// Chiede una clonazione di filtro                             |
		// Hdb filtro sorgente                                         |
		// Campo="num" num=Numero campo da controllare                 |
		// P.S. Server a adb_OpenClone()                               |
		// -------------------------------------------------------------
		case WS_CLONE:
		
			hdbDest= * (HDB *) pValore;
			if (Filter==NULL) break;

			//adb_Filter(hdbDest,WS_OPEN,Filter->MaxFilter,0,0);
			
			// Creo il nuovo filtro
			sprintf(szName,"%d",Filter->MaxFilter);
			adb_Filter(hdbDest,WS_OPEN,szName,NULL,NULL);
			
			// Copio gli elementi nella nuova destinazione clone
			memcpy(ADB_info[hdbDest].AdbFilter->Element,
			       Filter->Element,
				   Filter->MaxFilter*sizeof(ADBF_ELEM));

			ADB_info[hdbDest].AdbFilter->NumComp=Filter->NumComp;
			ADB_info[hdbDest].AdbFilter->NumFilter=Filter->NumFilter;
			ADB_info[hdbDest].AdbFilter->FirstIndex=Filter->FirstIndex;
			ADB_info[hdbDest].AdbFilter->idxInUse=Filter->idxInUse;
			ADB_info[hdbDest].AdbFilter->idxFloat=Filter->idxFloat;
			break;

		// -------------------------------------------------------------
		// Cancella la lista di puntatori                              |
		// (SE c'è)                                                    |
		// -------------------------------------------------------------

		case WS_DEL:
			if (!Filter) break;
			sprintf(szServ,"*hdbFltRecs");
			if (Filter->hdlRecords!=-1) memoFree(Filter->hdlRecords,szServ);
			Filter->arRecord=NULL;
			Filter->hdlRecords=-1;
			Filter->iRecordsMax=0;
			Filter->iRecordsReady=0;
			break;

		default:
#ifndef EH_CONSOLE
			dispx(__FILE__ ":" __FUNCTION__ ": comando sconosciuto %d",iCommand);
#endif
			ehLogWrite(__FILE__ ":" __FUNCTION__ ": comando sconosciuto %d",iCommand);
			Sleep(5000);
			break;
	 }
 return 0;
}

static BOOL Comparatore(HDB hdb,ADBF_ELEM *psElement,BOOL ConvNum,CHAR *pValore)
{
 SINT Cmp;
 INT16 *lpi,Val16;
 double fValore;
 double dValore;
 BOOL Check;
 CHAR *lpBlob;
 CHAR *lpFld;

 if (ConvNum) fValore=atof(pValore); else fValore=psElement->fValore;

 if (psElement->lpFldName) lpFld=adbFieldPtr(hdb,psElement->lpFldName);
 // ------------------------------
 // Search New 99
 // ------------------------------
 if (psElement->iCompType==7) 
 {
	CHAR *p;
	switch (psElement->iFldType)
	{
	 case ADB_ALFA:
	 case ADB_DATA: // Fornire la data ggmmaaaa
			 p=strstr(lpFld,pValore);
			 if (p) Check=TRUE; else Check=FALSE;
			 break;

	 case ADB_BLOB:
			 lpBlob=adb_BlobGetAlloc(hdb,psElement->lpFldName);
			 p=strstr(lpBlob,pValore);
			 adb_BlobFree(lpBlob);
			 if (p) Check=TRUE; else Check=FALSE;
			 break;

	 default:
			 ehExit("ADBSF:Errore in campo search");
			 break;
	}
	return Check; 
 }

 // ------------------------------
 // Begin with
 // ------------------------------
 if (psElement->iCompType==8) 
 {
	switch (psElement->iFldType)
	{
	 case ADB_ALFA:
			 if (strlen(lpFld)>=strlen(pValore))
				{Check=!memcmp(lpFld,pValore,strlen(pValore));} else Check=FALSE;
			 break;

	 case ADB_BLOB:
			 lpBlob=adb_BlobGetAlloc(hdb,psElement->lpFldName);
			 if (strlen(lpBlob)>=strlen(pValore))
				{Check=!memcmp(lpBlob,pValore,strlen(pValore));} else Check=FALSE;
			 adb_BlobFree(lpBlob);
			 break;

	 default:
			 ehExit("ADBSF:Errore in campo begin with");
			 break;
	}
	return Check; 
 }

 // ------------------------------
 // End With
 // ------------------------------
 if (psElement->iCompType==9) 
 {
	switch (psElement->iFldType)
	{
	 case ADB_ALFA:
			 if (strlen(lpFld)>=strlen(pValore))
				{Check=!memcmp(lpFld,pValore,strlen(pValore));} else Check=FALSE;
			 break;

	 case ADB_BLOB:
			 lpBlob=adb_BlobGetAlloc(hdb,psElement->lpFldName);
			 if (strlen(lpBlob)>=strlen(pValore))
				{Check=!memcmp(lpBlob+(strlen(lpBlob)-strlen(pValore)),pValore,strlen(pValore));} else Check=FALSE;
			 adb_BlobFree(lpBlob);
			 break;
	 default:
			 ehExit("ADBSF:Errore in campo end with");
			 break;
	}
	return Check; 
 }
 
 // ------------------------------
 // Search 2003
 // ------------------------------
 if (psElement->iCompType==10) 
 {
	CHAR *p,*lp;
	SINT iSize=strlen(pValore);

	switch (psElement->iFldType)
	{
	 case ADB_ALFA:
	 case ADB_DATA: // Fornire la data ggmmaaaa
	 case ADB_BLOB:
			if (psElement->iFldType==ADB_BLOB)	
				lpBlob=lp=adb_BlobGetAlloc(hdb,psElement->lpFldName); 
				else 
				lp=lpFld; 
			
			Check=FALSE; 
			while (TRUE)
			{
				p=strstr(lp,pValore); if (!p) {break;}
				if (p[iSize]==0||p[iSize]==',') {Check=TRUE; break;}
				lp=p+1;
			}

			if (psElement->iFldType==ADB_BLOB)	adb_BlobFree(lpBlob);
			break;

	 default:
			 ehExit("ADBSF:Errore in campo search");
			 break;
	}
	return Check; 
 }

 
 // ------------------------------
 // Search 2006
 // ------------------------------
 if (psElement->iCompType==11) 
 {
	CHAR *lp;
	SINT iSize=strlen(pValore);

	switch (psElement->iFldType)
	{
	 case ADB_ALFA:
	 case ADB_DATA: // Fornire la data ggmmaaaa
	 case ADB_BLOB:
		 {
			CHAR *lpValore=ehAlloc(iSize+1);
			strcpy(lpValore,pValore); _strlwr(lpValore);
			if (psElement->iFldType==ADB_BLOB)	
				lpBlob=lp=adb_BlobGetAlloc(hdb,psElement->lpFldName); 
				else 
				lp=lpFld; 
			
			Check=FALSE; 
			_strlwr(lp);
			if (strstr(lp,lpValore)) Check=TRUE;
			ehFree(lpValore);
			if (psElement->iFldType==ADB_BLOB)	adb_BlobFree(lpBlob);
		 }
		 break;

	 default:
		 ehExit("ADBSF:Errore in campo search");
		 break;
	}
	return Check; 
 }

 // ------------------------------
 // Search 2007 (Tipo Google o motore di ricerca Internet)
 // ------------------------------
 if (psElement->iCompType==12) 
 {
	CHAR *lp;
	struct ADB_REC *fldInfo;
	SINT x,iSize=strlen(pValore);

	//
	// A) Preparo stringa di comparazione
	//
	*psElement->pBuffer=0;
	for (x=0;psElement->arFields[x];x++)
	{
		lpFld=adb_FldInfo(hdb,psElement->arFields[x],&fldInfo); 

		switch (fldInfo->tipo)
		{
		 case ADB_ALFA:
		 case ADB_DATA: // Fornire la data ggmmaaaa
			 strcat(psElement->pBuffer,lpFld);
			 break;

		 case ADB_BLOB:
			 lp=adb_BlobGetAlloc(hdb,psElement->arFields[x]);
			 strcat(psElement->pBuffer,lp);
			 ehFree(lp);
			 break;

		 case ADB_INT:
		 case ADB_BOOL:
		 case ADB_AINC:
		 case ADB_INT32:
			 sprintf(psElement->pBuffer+strlen(psElement->pBuffer),"%d",adb_FldInt(hdb,psElement->arFields[x]));
			 break;

		 case ADB_NUME:
		 case ADB_FLOAT:
		 case ADB_COBD:
		 case ADB_COBN:
			 sprintf(psElement->pBuffer+strlen(psElement->pBuffer),"%.2f",adb_FldNume(hdb,psElement->arFields[x]));
			 break;

		 default: break;
		}
		strcat(psElement->pBuffer," ");

	}

	Check=FALSE; _strlwr(psElement->pBuffer);
	for (x=0;psElement->arStringSearch[x];x++)
	{
		if (strstr(psElement->pBuffer,psElement->arStringSearch[x])) Check=TRUE; else {Check=FALSE; break;}
	}
	return Check; 
 }

 // ------------------------------
 // AND logico New 2009
 // ------------------------------
 if (psElement->iCompType==13) 
 {
	 switch(psElement->iFldType)
		{
		 case ADB_BOOL:
		 case ADB_INT:
		 case ADB_NUME:
		 case ADB_COBD:
		 case ADB_COBN:
		 case ADB_AINC:
		 case ADB_INT32:
			 dValore=adb_FldInt(hdb,psElement->lpFldName);
			 if ((SINT) dValore & (SINT) fValore) Check=TRUE; else Check=FALSE;
			 break;
		}
	 return Check;
 }

 switch(psElement->iFldType)
	{
	 case ADB_ALFA:
	 case ADB_DATA: // Fornire la data ggmmaaaa
			 Cmp=strcmp(lpFld,pValore);
			 break;
	 
	 case ADB_BLOB:
			 lpBlob=adb_BlobGetAlloc(hdb,psElement->lpFldName);
			 Cmp=strcmp(lpBlob,pValore);
			 adb_BlobFree(lpBlob);
			 break;

	 case ADB_BOOL:
	 case ADB_INT:
			 Cmp=0;
			 lpi=(INT16 *) lpFld;
			 Val16=* (INT16 *) pValore;
			 if (*lpi>Val16) Cmp=1;
			 if (*lpi<Val16) Cmp=-1;
			 break;

	 case ADB_NUME:
			 Cmp=0;
			 if (atof(lpFld)>fValore) Cmp=1;
			 if (atof(lpFld)<fValore) Cmp=-1;
			 break;

	 case ADB_COBD:
	 case ADB_COBN:
			 Cmp=0;
			 dValore=adb_FldNume(hdb,psElement->lpFldName);
			 if (dValore>fValore) Cmp=1;
			 if (dValore<fValore) Cmp=-1;
			 //_d_("%s|%.2f,%.2f           ",psElement->lpFldName,dValore,fValore); ehSleep(100);
			 break;

	 case ADB_AINC:
	 case ADB_INT32:
			 Cmp=0;
			 dValore=adb_FldInt(hdb,psElement->lpFldName);
			 if (dValore>fValore) Cmp=1;
			 if (dValore<fValore) Cmp=-1;
			 //_d_("%s|%.2f,%.2f           ",psElement->lpFldName,dValore,fValore); ehSleep(100);
			 break;
	}
 // Controllo cosa mi interessa
	Check=FALSE;
	
	switch (psElement->iCompType)
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



/*
DOLIST
// Controllo se esiste una funzione esterna 
					 if (Filter->ExtProcess)
					 {
						 (Filter->ExtProcess)(WS_DOLIST,hdb,Filter);
						 break;
					 }

					 // --------------------------------------
					 // Ricerco quale indice usare           |
					 // --------------------------------------
					 IndexUse=0;
					 Check=FALSE;
					 lpName="";
					 if (Filter->FirstIndex!=-1) 
					 {IndexUse=Filter->Element[Filter->FirstIndex].iFldType;
					  for (a=Filter->FirstIndex;a<Filter->NumFilter;a++)
					  {
						Element=Filter->Element+a; if (!psElement->iType) continue;
						lpName=psElement->lpFldName;
                        strcpy(szName,psElement->lpFldName);
                        
						// Ricerco se ho una selezione per quel campo
						for (b=0;b<Filter->NumFilter;b++)
						 {
							Element=Filter->Element+b; if (psElement->iType) continue;
							if (!strcmp(psElement->lpFldName,szName))
								{
								 IndexUse=Filter->Element[a].iFldType;
								 Check=TRUE;
								 break;
								}
						 }
						if (Check) break;
					  }
					 }
*/
