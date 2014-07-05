//   ---------------------------------------------
//   | fxdServer Ferrà Hierarchic Dbase 1.1      | ex FLM_FHD.C
//   |                                           |
//   |             by Ferrà Art & Tecnology 2000 |
//   |             by Ferrà srl 2007
//   ---------------------------------------------
//    NOTA
//    Modificato sotto effetto di ACTIGRIP+influenza Pacifica 12/2007
//

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/ehtool/fxdServer.h"
#include "/easyhand/inc/xmlParser.h"

static BOOL fTheFile=FALSE;

#if (defined(_ADB32)||defined(_ADB32P))
static BOOL LFHDAdbImport(DRVMEMOINFO *dmiFxd,SINT idParent,CHAR *lpFileName);
#endif

FHD_PROPERTY sFhdProperty;

#define FHDMAXRECORD 10000
CHAR * arFhdTypeList[]={"ALFA","NUME","DATA","INT16","FLOAT","FLAG","COB-D","COB-N","AUTOINC","VARCHAR","UINT32","GEOMETRY","POINT","BINARY","TIMESTAMP",NULL}; // arFhdTypeList
CHAR * FhdTypeListXml[]={"VARCHAR","T_NUMBER","T_DATE","INT16","FLOAT","BOOL","COB-D","COB-N","AUTOINC","BLOB","UINT32","GEOMETRY","POINT","BINARY","TIMESTAMP",NULL};
static CHAR *FHDEncoding(BYTE *lp);
static CHAR *FHDDecoding(BYTE *lp);

DWORD FNTServiceStart(CHAR *lpService, BOOL fStopDependencies, DWORD dwTimeout ) ;
DWORD FNTServiceStop(CHAR *lpService, BOOL fStopDependencies, DWORD dwTimeout ) ;

static BOOL FHD_LoadFile(BYTE *pFileName,_DMI *pdmi);
static BOOL FHD_SaveFile(BYTE *pFileName,_DMI *pdmi);

static BOOL _FxdLoadFile(BYTE *pFileName,_DMI *pdmi);
static SINT _FxdSaveFile(BYTE *pFileName,_DMI *pdmi);
static void StringCmp(CHAR *lpDest,CHAR *lpSorg,SINT Len,BOOL fSpaceConvert);

/*
static FXD_TAB_INFO *NewTableInfo(void);
static FXD_FLD_INFO *NewFieldInfo(void);
static FXD_IDX_INFO *NewIndexInfo(void);
*/

//
// Funzioni di decodifica da CODE > a struttura
//
static void FHD_TableDecode(CHAR *lpCode,FXD_TAB_INFO *lps);//CHAR **lpLocalFile,SINT *iFql,CHAR **lpSQLTableName);
static void FHD_TableEncode(CHAR *lpCode,FXD_TAB_INFO *lps);// CHAR *lpLocalFile,SINT iFql,CHAR *lpSQLTableName);

static void FHD_FieldDecode(CHAR *lpCode,FXD_FLD_INFO *lps);
static void FHD_FieldEncode(CHAR *lpCode,FXD_FLD_INFO *lps);

static void FHD_IndexDecode(CHAR *lpCode,FXD_IDX_INFO *lps);
static void FHD_IndexEncode(CHAR *lpCode,FXD_IDX_INFO *lps);

static void FhdRemove(_DMI *pdmi,SINT info);

static SINT iAutoCode=0;
static SINT FhdNextNumber() {

	iAutoCode++; return iAutoCode;

}


/*
CHAR * _arsDbPlatform[]={	
					"Pervasive (btrieve)",
					"IBM DB2",
					"Oracle",
					"Microsoft SQL Server",
					"MySQL",
					"SQLite",
					NULL};
					*/


SINT fxdServer(EN_MESSAGE cmd,LONG info,void *str)
{
	static _DMI dmiFxd={-1,-1,-1,-1};
	SINT a;
	SINT iCount;
	FXD_ELEMENT sFxd;
	FXD_ELEMENT *lpFhd;
	FHDPROCESS *lpFhdProc;
	SINT iReturn=0;

	switch (cmd)
	{
		// ---------------------------------
		// Apro una gestione di dbase FHD
		// --------------------------------
		case WS_OPEN: // Carico le Classi/tabelle di un dbase
			
			// Carico il file FHD in memoria
			dmiFxd.Num=0;
			DMIOpen(&dmiFxd,RAM_AUTO,FHDMAXRECORD,sizeof(FXD_ELEMENT),"FhdTable");
			ZeroFill(sFhdProperty);
			iAutoCode=0;

			switch (info)
			{
				case FHD_OPEN_NEW: // Nuovo dbase
					strcpy(sFhdProperty.szFHDName,str);
					break;
				
				case FHD_OPEN_RESET:
					//*sFhdProperty.szUser=0; *sFhdProperty.szFileSource=0;
					ZeroFill(sFhdProperty);
					break;
			}
			break;

		// ---------------------------------
		// Leggo un File
		// --------------------------------
		case WS_LOAD: 
			switch (info)
			{
				//
				// Formato FXD
				//
 			  case FHD_MODE_FXD:
					if (_FxdLoadFile(str,&dmiFxd)) break;
					break;

				//
				// Formato FHD
				//

 			  case FHD_MODE_FHD:
					if (str==NULL) break;
					if (FHD_LoadFile(str,&dmiFxd)) break;
					break;
/*
			  case FHD_MODE_ADB: 
				  LFHDAdbImport(&dmiFxd,str); 
				  break;
				  */
			}
			break;

#if (defined(_ADB32)||defined(_ADB32P))
		case WS_DO: 
			LFHDAdbImport(&dmiFxd,info,str); 
			break;
#endif

		case WS_CLOSE: 

			if (str!=NULL) // Scrivo il dbase
			{
				//win_infoarg("%d",info);
				switch (info&0xFF)
				{
					//
					// Formato FXD
					//
 					  case FHD_MODE_FXD:
						iReturn=_FxdSaveFile(str,&dmiFxd);
						break;
					//
					// Formato FXD
					//
 					  case FHD_MODE_FHD:
						FHD_SaveFile(str,&dmiFxd);
						break;

					  default:
						  ehExit("Error in FhdServer:WS_CLOSE %d",info);
				}
			}

			if (!(info&FHD_NO_FREE))
			{
				for (a=0;a<dmiFxd.Num;a++)
				{
					DMIRead(&dmiFxd,a,&sFxd);
					FhdElementFree(&sFxd);
					DMIWrite(&dmiFxd,a,&sFxd);
				}

				DMIClose(&dmiFxd,"FhdTable");
			}
			break;

		// Restituisce il numero di tabelle presenti
		case WS_COUNT: return dmiFxd.Num;

		case WS_INF: return (SINT) &dmiFxd;
			//break;

		case WS_REALGET: // Legge i dati di una tabella
			if (info==FHD_GET_NAME) return (LONG) sFhdProperty.szFHDName; 
			if (info==FHD_GET_FILE) return (LONG) sFhdProperty.szFileSource; 
			if (info==FHD_GET_LAST) info=dmiFxd.Num-1;
			if (info==FHD_GET_DSN)  return (LONG) sFhdProperty.szDSN; 
			if (info==FHD_GET_USER) return (LONG) sFhdProperty.szUser; 
			if (info==FHD_GET_PASS) return (LONG) sFhdProperty.szPass; 
			if (info==FHD_GET_LIB) return (LONG) sFhdProperty.szLibrary; 
			if (info==FHD_GET_PLATFORM) return sFhdProperty.iDefaultPlatform; 
			if (info==FHD_GET_VERSION) return (LONG) sFhdProperty.szVersion; 
			if (info==FHD_GET_BTRIEVE) return (LONG) sFhdProperty.bBtrieveMigration;
			if (info==FHD_GET_SERVER)  return (LONG) sFhdProperty.szServer; 
			if (info>dmiFxd.Num) return -1;
			DMIRead(&dmiFxd,info,str);
			break;

		case WS_REALSET: // Legge i dati di una tabella
			if (info==FHD_SET_NAME) strcpy(sFhdProperty.szFHDName,str); 
			if (info==FHD_SET_FILE) strcpy(sFhdProperty.szFileSource,str); 
			if (info==FHD_SET_DSN) strcpy(sFhdProperty.szDSN,str); 
			if (info==FHD_SET_USER) strcpy(sFhdProperty.szUser,str); 
			if (info==FHD_SET_PASS) strcpy(sFhdProperty.szPass,str); 
			if (info==FHD_SET_LIB) strcpy(sFhdProperty.szLibrary,str); 
			if (info==FHD_SET_PLATFORM) sFhdProperty.iDefaultPlatform=(SINT) str; 
			if (info==FHD_SET_VERSION) strcpy(sFhdProperty.szVersion,str); 
			if (info==FHD_SET_BTRIEVE) sFhdProperty.bBtrieveMigration=(SINT) str;
			if (info==FHD_SET_SERVER) strcpy(sFhdProperty.szServer,str); 
			
			if (info<0) break;
			DMIWrite(&dmiFxd,info,str);
			break;
		
		case WS_DEL:

			//
			// Cancellazione ricorsiva
			//
			FhdRemove(&dmiFxd,info);

			/*
			if (str)
			{
				if (!strcmp(str,"TABLE")) // Cancella la tabella ed i suoi dati 
				{
				  //info++;
				  DMIRead(&dmiFxd,info,&sFxd); FhdElementFree(&sFxd);
				  DMIDelete(&dmiFxd,info,&sFxd);
				  while (TRUE)
				  {
					if (info>=dmiFxd.Num) break;
					DMIRead(&dmiFxd,info,&sFxd);	  
					if ((sFxd.iType==FHD_FIELD)||(sFxd.iType==FHD_INDEX))
					{
						FhdElementFree(&sFxd);
						DMIDelete(&dmiFxd,info,&sFxd);		  
					} 
					else 
					break;
				  }
				  break;
				}

				if (!strcmp(str,"CLASS")) // Cancella la classe ed i suoi dati
				{
				  DMIRead(&dmiFxd,info,&sFxd); FhdElementFree(&sFxd);
				  DMIDelete(&dmiFxd,info,&sFxd);
				  while (TRUE)
				  {
					if (info>=dmiFxd.Num) break;
					DMIRead(&dmiFxd,info,&sFxd);	FhdElementFree(&sFxd);  
					if (sFxd.iType==FHD_FOLDER) break;
					DMIDelete(&dmiFxd,info,&sFxd);		  
				  }
				  break;
				}
				
				win_infoarg("FHDWSDEL:? [%s]",str);
			}
			DMIDelete(&dmiFxd,info,&sFxd);
			*/
			break;

		case WS_INSERT:
			
			if (info<dmiFxd.Num) 
			{DMIInsert(&dmiFxd,info,&sFxd);
			 lpFhd=str;
			 lpFhd->idAutocode=FhdNextNumber();
			 DMIWrite(&dmiFxd,info,lpFhd);
			}
			else
			{
			 lpFhd=str;
			 lpFhd->idAutocode=FhdNextNumber();
			 DMIAppend(&dmiFxd,str);
			}
			break;
		
		case WS_ADD:
			lpFhd=str;
			lpFhd->idAutocode=FhdNextNumber();//iEnumCounter++;
			DMIAppend(&dmiFxd,str);
			break;
		
		case WS_FIND: // Ricerca
			if (info==FHD_FIND_FIELD)
			{
				for (a=0;a<dmiFxd.Num;a++)
				{
					DMIRead(&dmiFxd,a,&sFxd);
					if (!strcmp(sFxd.szName,str)) return a;
				}
				return -1;
			}
			else if (info==FHD_FIND_TABLE_FIELD)
			{
				EH_AR ar;
				SINT iNum,idxTable=-1,idxField=-1;
				ar=ARCreate(str,".",&iNum); 
				if (iNum!=2) 
					ehError();

				//
				// Ricerco l'indice della tabella
				//
				for (a=0;a<dmiFxd.Num;a++) {
					DMIRead(&dmiFxd,a,&sFxd); if (sFxd.iType!=FHD_TABLE) continue;
					if (!strCaseCmp(sFxd.szName,ar[0])) {idxTable=a; break;}
				}
				if (idxTable<0) return -1;
				for (a=idxTable+1;a<dmiFxd.Num;a++) {
					DMIRead(&dmiFxd,a,&sFxd); if (sFxd.iType!=FHD_FIELD) break;
					if (!strCaseCmp(sFxd.szName,ar[1])) {idxField=a; break;}
				}
				ARDestroy(ar);
				return idxField;

			}
			else if (info==FHD_FIND_TABLE) {
				for (a=0;a<dmiFxd.Num;a++) {
					DMIRead(&dmiFxd,a,&sFxd); if (sFxd.iType!=FHD_TABLE) continue;
					if (!strCaseCmp(sFxd.szName,str)) {return a;}
				}
				return -1;
			
			}

			for (a=0;a<dmiFxd.Num;a++)
			{
			  DMIRead(&dmiFxd,a,&sFxd);
			  if (sFxd.idAutocode==info) return a;
			}
			return -1;
		// -------------------------------------
		// Processa una funzione di comando
		// -------------------------------------
		case WS_PROCESS:

			lpFhdProc=(FHDPROCESS *) str;			
			
			switch (lpFhdProc->iType) {

				//
				// Muove un oggetto da una parte ad un altra
				//
				case FHD_PROC_MOVE:
				
					//
					// Determino le dimensioni dell'oggetto
					//
					fxdServer(WS_REALGET,lpFhdProc->iFrom,&sFxd); 
					switch (sFxd.iType) {

						case FHD_TABLE:
							iCount=1;
							for (a=lpFhdProc->iFrom+1;a<dmiFxd.Num;a++) {

								fxdServer(WS_REALGET,a,&sFxd);
								if ((sFxd.iType==FHD_TABLE)||
									(sFxd.iType==FHD_FOLDER)) break;
								iCount++;
							}
							break;

						case FHD_VIEW:
							iCount=1;
							break;

						default:
							ehError(); // Tipo di spostamento non gestito > 
							break;
					}
					
					lpFhdProc->iTo=DMIMove(&dmiFxd,lpFhdProc->iFrom,lpFhdProc->iTo,iCount);
					fxdServer(WS_REALGET,lpFhdProc->iTo,&sFxd); 
					printf("qui");
					break;

			}
			break;
	}
  return 0;
}

					//	{
							/*
							BYTE *lpMemo;
							BYTE *lpTable;
							BYTE *lpFrom;
							BYTE *lpTo;
							SINT iCoda;
							*/
//							SINT iSize=iCount*sizeof(FXD_ELEMENT);

							//
							// B) Determino le dimensioni della tabella (conto FIELD e INDEX)
							//
				//		}

/*


							lpMemo=ehAlloc(iSize);
							lpTable=memoLock(dmiFxd.Hdl);
							lpFrom=lpTable+(lpFhdProc->iFrom*sizeof(FXD_ELEMENT));

							//
							// Copio la tabella nella memoria di backup
							//
							memcpy(lpMemo,lpFrom,iSize);
							
							//
							// Cancello le righe occupate dall'oggetto
							//
							iCoda =(dmiFxd.Num-(lpFhdProc->iFrom+iCount))*sizeof(FXD_ELEMENT);
							memmove(lpFrom,
									lpFrom+iSize,
									iCoda);
							dmiFxd.Num-=iCount;

							//
							// Se copio dopo torno indietro
							//
							if (lpFhdProc->iTo>lpFhdProc->iFrom) lpFhdProc->iTo-=iCount;

							//
							// N.B. +1 per spostarlo dopo la classe
							//
							lpTo  = lpTable+(lpFhdProc->iTo*sizeof(FXD_ELEMENT));
							iCoda = (dmiFxd.Num-lpFhdProc->iTo)*sizeof(FXD_ELEMENT);
							memmove(lpTo+iSize,lpTo,iCoda);
				  
							//
							// Copio nella nuova posizione la tabella
							//
							memcpy(lpTo,lpMemo,iSize);
							dmiFxd.Num+=iCount;
							ehFree(lpMemo);
							memoUnlock(dmiFxd.Hdl);

			}
			break;
							*/

#if (defined(_ADB32)||defined(_ADB32P))
static BOOL LFHDAdbImport(DRVMEMOINFO *dmiFxd,SINT idFolderParent,CHAR *lpFileName)
{
	SINT rt;
	FILE *pf1;
	SINT Hadbcod=-1;//,dbhdl;
	CHAR serv[500];
	struct ADB_HEAD AdbHead;
	FXD_ELEMENT sFxd;
	SINT dbhdl;
	struct ADB_REC *lpRecord;
	struct ADB_INDEX *lpIndex;
	struct ADB_INDEX_SET *lpIndexSet;
	CHAR *lpMemo;
	SINT iSizeInfo;
	SINT a,b,Count;
	SINT idTableParent;

	pf1=fopen(lpFileName,"rb"); if (!pf1) return -1;
	fread(&AdbHead,sizeof(AdbHead),1,pf1);
    if (strcmp(AdbHead.id,"Adb1.2 Eh3")) return -1;


	//fxdServer(WS_REALGET,idxParent,&FhdFolder);

	// -----------------------------------------
	// Inserisco il nome della tabella
	// -----------------------------------------
	
	ZeroFill(sFxd);
	sFxd.iType=FHD_TABLE;
	OemToChar(AdbHead.DescFile,AdbHead.DescFile);
	fxdPropertyBuilder(&sFxd,FHD_TABLE,FALSE);
	sFxd.idParent=idFolderParent;
	strAssign(&sFxd.psTableInfo->pszLocalFile,lpFileName);
	strcpy(sFxd.szName,AdbHead.DescFile);
	sFxd.szName[0]=(CHAR) toupper(sFxd.szName[0]);
	fxdServer(WS_ADD,0,&sFxd);
	idTableParent=sFxd.idAutocode;
	rt=OFF;

	// -----------------------------------------
	// Inserisco i campi degli input
	// -----------------------------------------

	iSizeInfo=sizeof(struct ADB_HEAD)+
			  (AdbHead.Field*sizeof(struct ADB_REC))+
			  (AdbHead.Index*sizeof(struct ADB_INDEX)+
			  (AdbHead.IndexDesc*sizeof(struct ADB_INDEX_SET)));

	dbhdl=memoAlloc(M_HEAP,iSizeInfo,"FLDIMP");
	if (dbhdl<0)
	{sprintf(serv,"FldImp():Memoria insufficiente per aprire :\n%s",lpFileName);
	 win_errgrave(serv);
	}
	
	lpMemo=memoPtr(dbhdl,NULL);
	fseek(pf1,0,SEEK_SET);
	if (!fread(lpMemo,iSizeInfo,1,pf1)) {rt=ON; goto fine;}

	lpRecord=(struct ADB_REC *) lpMemo+sizeof(struct ADB_HEAD);//farPtr(lpMemo,sizeof(struct ADB_HEAD));
	lpIndex=(struct ADB_INDEX *) ((BYTE *) lpRecord)+(AdbHead.Field*sizeof(struct ADB_REC));//farPtr(lpRecord,(AdbHead.Field*sizeof(struct ADB_REC)));
	lpIndexSet=(struct ADB_INDEX_SET *) ((BYTE *) lpIndex)+(AdbHead.Index*sizeof(struct ADB_INDEX));//farPtr(lpIndex,(AdbHead.Index*sizeof(struct ADB_INDEX)));

	// --------------------------------------------
	// Carico i Campi del Record
	// --------------------------------------------
	for (a=0;a<AdbHead.Field;a++)
	{
		ZeroFill(sFxd);
		sFxd.iType=FHD_FIELD;
		strcpy(sFxd.szName,lpRecord[a].desc);
		fxdPropertyBuilder(&sFxd,FHD_FIELD,FALSE);
		sFxd.psFieldInfo->iType=lpRecord[a].tipo;
		sFxd.psFieldInfo->iSize=lpRecord[a].size;
		sFxd.psFieldInfo->iDecimal=lpRecord[a].tipo2;
		sFxd.idParent=idTableParent;

		//sFFI.lpNote="";
		//sFFI.lpCharSet="";
		//FHD_FieldEncode(sFxd.szOldCode,&sFFI);
		//				 lpRecord[a].tipo,
		//				 lpRecord[a].size,
		//				 lpRecord[a].tipo2,
		//				 ""); // Nessuna nota

		fxdServer(WS_ADD,0,&sFxd);
	}

	// --------------------------------------------
	// Carico i Campi degli Indici
	// --------------------------------------------
	for (a=0;a<AdbHead.Index;a++)
	{
		BOOL fDup;
		ZeroFill(sFxd);
		sFxd.iType=FHD_INDEX;
		strcpy(sFxd.szName,lpIndex[a].IndexName);
	    OemToChar(lpIndex[a].IndexDesc,lpIndex[a].IndexDesc);
        //sprintf(serv,"%s|",lpIndex[a].IndexDesc);
		*serv=0;
		Count=0;
		for (b=0;b<AdbHead.IndexDesc;b++)
		{
			if (!strcmp(lpIndex[a].IndexName,lpIndexSet[b].IndexName))
			{
				Count++; if (Count==1) continue;
				if (Count==2) 
						{strcat(serv,lpIndexSet[b].FieldName);
						 fDup=lpIndexSet[b].dup;
						}else
							{strcat(serv,"+");
							 strcat(serv,lpIndexSet[b].FieldName);
							}
			}
		}

		fxdPropertyBuilder(&sFxd,FHD_INDEX,FALSE);
//		ZeroFill(sFII);
		sFxd.psIndexInfo->lpDesc=strDup(lpIndex[a].IndexDesc);
		sFxd.psIndexInfo->fDup=fDup;
		sFxd.psIndexInfo->lpCompos=strDup(serv);
		sFxd.idParent=idTableParent;

		// Creo il formato standard IndexCode 
//		FHD_IndexEncode(sFxd.szOldCode,&sFII);
		//	             lpIndex[a].IndexDesc, // Descrizione
		//				 fDup,// Duplicabile
		//				 FALSE, // Case sensitive
		//				 FALSE,
		//				 serv // Composizione
						 //);
		fxdServer(WS_ADD,0,&sFxd);
	}
	
	fine:
	memoFree(dbhdl,"");
	fclose(pf1);
	return rt;
}

// --------------------------------------------------------
// Costruisce i DDF
//
static SINT LocalRecnum(BYTE *PosBlock)
{
	BTI_WORD size;
	LONG NRecord;

#pragma pack(1)
	typedef struct {
	 INT16 LenRec;
	 INT16 PageSize;
	 INT16 IndexNum;
	 LONG RecNum;
	 INT16 fileflag;
	 INT16 reserved;
	 INT16 PageNoUsed;
	} B_INFOREC ;
#pragma pack()

	B_INFOREC *databuf;
	BTI_SINT status;

	size=128+(1+30)*16+265; // Ci sto' largo
	databuf=(B_INFOREC *) ehAlloc(size);
	status = BTRV( B_STAT,
				    PosBlock,
				    databuf,// Puntatore ai dati
				    &size, // Grandezza dei dati
				    databuf,   // ???> in output
				    0); // ??? in input
	NRecord=databuf->RecNum;
	ehFree(databuf);
	if (!status) return NRecord;
	return status;
}

// Trovo offset di posizione e dimensioni
static BOOL LocalGetFldPos(CHAR *lpFileAdb,CHAR *lpField,INT16 *lpiOffset,INT16 *lpiSize)
{
	SINT a,rt=OFF;
	FILE *pf1;
	SINT Hadbcod=-1,dbhdl;
	CHAR serv[255];
	struct ADB_HEAD AdbHead;
	struct ADB_REC *lpRecord;
	struct ADB_INDEX *lpIndex;
	struct ADB_INDEX_SET *lpIndexSet;
	CHAR *lpMemo;
	SINT iSizeInfo;

	pf1=fopen(lpFileAdb,"rb"); if (!pf1) {rt=ON; goto fine;}
    fread(&AdbHead,sizeof(AdbHead),1,pf1);
    if (strcmp(AdbHead.id,"Adb1.2 Eh3")) {rt=ON; goto fine;}

	// ------------------------------------
	//	    Struttura della memoria ADB    !
	//	                                   !
	//	255 Nome del file                  !
	//	128 PosBlock                       !
	//	<-> Record Buffer                  !
	//	<-> KeyMoreGreat buffer            !
	//	<-> Attributi del file             !
	//	    ADB_HEAD                       !
	//	    ADB_REC * i campi              !
	//	    ADB_INDEX * gli indici         !
	//	    ADB_INDEX_SET sorgente definiz !
	//	                                   !
	// ------------------------------------

	rt=OFF;
	iSizeInfo=sizeof(struct ADB_HEAD)+
			  (AdbHead.Field*sizeof(struct ADB_REC))+
			  (AdbHead.Index*sizeof(struct ADB_INDEX)+
			  (AdbHead.IndexDesc*sizeof(struct ADB_INDEX_SET)));

	dbhdl=memoAlloc(M_HEAP,iSizeInfo,"FLDIMP");
	if (dbhdl<0)
	{sprintf(serv,"FldImp():Memoria insufficiente per aprire :\n%s",lpFileAdb);
	 win_errgrave(serv);
	}
	
	lpMemo=memoPtr(dbhdl,NULL);
	fseek(pf1,0,SEEK_SET);
	if (!fread(lpMemo,iSizeInfo,1,pf1)) {rt=ON; goto fine;}

	lpRecord=(struct ADB_REC *) lpMemo+sizeof(struct ADB_HEAD);//farPtr(lpMemo,sizeof(struct ADB_HEAD));
	lpIndex=(struct ADB_INDEX *) ((BYTE *) lpRecord)+(AdbHead.Field*sizeof(struct ADB_REC));//farPtr(lpRecord,(AdbHead.Field*sizeof(struct ADB_REC)));
	lpIndexSet=(struct ADB_INDEX_SET *) ((BYTE *) lpIndex)+(AdbHead.Index*sizeof(struct ADB_INDEX));//farPtr(lpIndex,(AdbHead.Index*sizeof(struct ADB_INDEX)));

	rt=2;
	for (a=0;a<AdbHead.Field;a++)
	{
		if (!strcmp(lpRecord[a].desc,lpField))
		{
			*lpiOffset=(INT16) lpRecord[a].pos;
			*lpiSize=(INT16) lpRecord[a].RealSize;
			rt=0;
			break;
		}
	}
/*
	// --------------------------------------------
	// Carico i Campi del Record
	// --------------------------------------------
	ListView_DeleteAllItems(hwndLVFields);
	for (a=0;a<AdbHead.Field;a++)
	{
		sprintf(serv,"%d",a+1); LVItemSet(hwndLVFields,a,0,serv);
		// Descrizione
		LVItemSet(hwndLVFields,a,1,lpRecord[a].desc);
		// Tipo
		LVItemSet(hwndLVFields,a,2,arFhdTypeList[lpRecord[a].tipo]);
		// Lunghezza
		sprintf(serv,"%d",lpRecord[a].size);
		LVItemSet(hwndLVFields,a,3,serv);

		switch (lpRecord[a].tipo)
		{
			case ADB_ALFA:
				if (lpRecord[a].tipo2) LVItemSet(hwndLVFields,a,5,"Si"); 
									   else
									   LVItemSet(hwndLVFields,a,5,"No");
				break;
			
			case ADB_NUME:
				sprintf(serv,"%d",lpRecord[a].tipo2);
				LVItemSet(hwndLVFields,a,4,serv);
				break;

		}
	}

	// --------------------------------------------
	// Carico i Campi degli Indici
	// --------------------------------------------
	ListView_DeleteAllItems(hwndLVIndex);
//	win_infoarg("%d",AdbHead.Index);
	for (a=0;a<AdbHead.Index;a++)
	{
		SINT Count=0;
		sprintf(serv,"%d",a+1); LVItemSet(hwndLVIndex,a,0,serv);
		// Descrizione
		LVItemSet(hwndLVIndex,a,1,lpIndex[a].IndexName);
		LVItemSet(hwndLVIndex,a,2,lpIndex[a].IndexDesc);
		*serv=0;

		for (b=0;b<AdbHead.IndexDesc;b++)
		{
			
			if (!strcmp(lpIndex[a].IndexName,lpIndexSet[b].IndexName))
			{
				Count++; if (Count==1) continue;
				if (!*serv) strcpy(serv,lpIndexSet[b].FieldName);
							else
							{strcat(serv,"+");
							 strcat(serv,lpIndexSet[b].FieldName);
							}
			}
		}
		LVItemSet(hwndLVIndex,a,3,serv);
	}

	strcpy(lpDesc,AdbHead.DescFile);
*/

	fine:
	memoFree(dbhdl,"");
	fclose(pf1);
	if (rt) win_infoarg("Nel file %s %d) non è stato trovato il campo [%s]",lpFileAdb,rt,lpField); 
	return rt;
}

void FHD_DDFMaker(void)
{
	BTI_WORD dataLen;
	BTI_SINT status;
	CHAR pbFile[129]; // PosBlock File
	CHAR pbField[129]; // PosBlock File
	CHAR pbIndex[129]; // PosBlock File
	CHAR szFileTemp[MAXPATH];
	CHAR szDDFFile[MAXPATH];
	CHAR szDDFField[MAXPATH];
	CHAR szDDFIndex[MAXPATH];
	CHAR szDbPath[MAXPATH];
	CHAR szLastAdbFile[MAXPATH];
	CHAR Serv[200];
	CHAR KeyBuffer[255];

	CHAR *lpSource="/easyhand/Projects/dbSample/ddfempty/";
	CHAR szLastTable[32];
	CHAR *lpc,szFName[32];
	FXD_ELEMENT sFxd;
	SINT iCount;
	SINT a;
	SINT b;

	BOOL fStop=FALSE;
	BOOL fError=FALSE;
	SINT iField=0;
    SINT iIndex=0;
	BOOL fTable=FALSE;
	BOOL fIndex=FALSE;
	SINT iFileCount;
	SINT iFieldCount;
	
	DDF_FILE  ddfFile;
	DDF_FIELD ddfField;
	DDF_INDEX ddfIndex;
	BYTE *pField,*pComposiz;
	
	DRVMEMOINFO dmiList=DMIRESET;
	typedef struct {
		SINT iId;
		CHAR szName[60];
	} DDFLIST;
	
	DDFLIST DDFList;

//	CHAR *lpTableLocalFile;
//	SINT iTableFql;
//	CHAR *lpTableSQLTableName;

	//FNTServiceStop(TEXT("Tango_2000_Server"),FALSE,30000);
	//FNTServiceStart(TEXT("Tango_2000_Server"),FALSE,30000);
	//return;

	// ---------------------------------------------------
	// Trovo la locazione del DDF
	//
	if (!*sFhdProperty.szFileSource) {win_infoarg("FHD senza percorso"); return;}
	strcpy(szDbPath,filePath(sFhdProperty.szFileSource));

	mouse_graph(0,0,"CLEX");

	// ---------------------------------------------------
	// Copio i file vuoti nella nuova destinazione
	//
	status=FALSE;

	sprintf(szDDFFile,"%sfile.ddf",szDbPath);
	sprintf(szFileTemp,"%sfile.ddf",lpSource);
	if (!CopyFile(szFileTemp,szDDFFile,FALSE)) status=TRUE;
	
	if (!status)
	{
		sprintf(szDDFField,"%sfield.ddf",szDbPath);
		sprintf(szFileTemp,"%sfield.ddf",lpSource);
		if (!CopyFile(szFileTemp,szDDFField,FALSE)) status=TRUE;
	}

	if (!status)
	{
		sprintf(szDDFIndex,"%sindex.ddf",szDbPath);
		sprintf(szFileTemp,"%sindex.ddf",lpSource);
		if (!CopyFile(szFileTemp,szDDFIndex,FALSE)) status=TRUE;
	}

	if (status) 
		{
		 BYTE *pError=osErrorStrAlloc(0);
		 win_infoarg(	"ATTENZIONE:\n"
						"Non e' stato possibile creare i ddf vuoti, necessari al processo.\n"
						"Chiudere gli applicativi che possono avere in uso i ddf e riprovare\n%s > %s\n%s",
						szFileTemp,szDDFFile,
						pError);
		 ehFree(pError);
		 return;
		}

	// ---------------------------------------------------
	// Apro il file
	//
	dataLen=0;
	status=BTRV(B_OPEN,pbFile, NULL,&dataLen,szDDFFile,EXCLUSIVE);
	iFileCount=LocalRecnum(pbFile);
	status|=BTRV(B_OPEN,pbField,NULL,&dataLen,szDDFField,0);
	iFieldCount=LocalRecnum(pbField);
	status|=BTRV(B_OPEN,pbIndex,NULL,&dataLen,szDDFIndex,0);
	
	if (status)// ehExit("Errore in open DDF");
		{
		 win_infoarg("ATTENZIONE:\nNon è stato possibile aprire in modo esclusivo i file DDF necessari al processo.\nChiudere gli applicativi che possono avere in uso i ddf e riprovare.");
		 return;
		}

	iCount=fxdServer(WS_COUNT,0,NULL);
	fStop=FALSE;
	
	for (a=0;a<iCount;a++)
	{
		fxdServer(WS_REALGET,a,&sFxd);
		
		switch (sFxd.iType)
		{
			case FHD_FOLDER: // La classe la salto
					break;
			
			case FHD_TABLE: // Nuova definizione della Tabella			
					// sFxd.szCode = File
					// sFxd.szName = Nome della tabella
					//win_infoarg("Tabella %s %s",sFxd.szCode,sFxd.szName);
					//FHD_TableDecode(sFxd.szOldCode,&lpTableLocalFile,&iTableFql,&lpTableSQLTableName);
					if (!*sFxd.psTableInfo->pszLocalFile) 
					{
						win_infoarg("ERRORE:\n"
									"Alla tabella %s manca il file assegnato\n"
									"DDF generati incompleti !",
									sFxd.szName);
						fStop=TRUE;
						fError=TRUE;
						break;
					}

					if (strstr(sFxd.psTableInfo->pszLocalFile,"?")) 
					{
						win_infoarg("ERRORE:\nAlla tabella %s manca il file assegnato\nDDF generati incompleti !",
									sFxd.szName);
						fStop=TRUE;
						fError=TRUE;
						break;
					}

					ZeroFill(ddfFile);
					iFileCount++;
					
					ddfFile.siFileId=iFileCount;
					StringCmp(ddfFile.szTableName,sFxd.szName,sizeof(ddfFile.szTableName),TRUE);
					strcpy(szLastTable,sFxd.szName);
					strcpy(szFName,fileName(sFxd.psTableInfo->pszLocalFile));
					lpc=strReverseStr(szFName,"."); if (lpc) *lpc=0;
					sprintf(Serv,"%s.bti",szFName);
					StringCmp(ddfFile.szTableLocation,Serv,sizeof(ddfFile.szTableLocation),FALSE);
					sprintf(szLastAdbFile,"%s%s.adb",szDbPath,szFName);

					if (!fileCheck(szLastAdbFile)) 
					{
						win_infoarg("ERRORE:\nIl file assegnato alla tabella %s, non esiste\nDDF generati incompleti !",
									sFxd.szName);
						fStop=TRUE;
						fError=TRUE;
						break;
					}

					ddfFile.bFileFlags=64;

					dataLen=sizeof(ddfFile);
					status = BTRV( B_INSERT,
								   pbFile,
								   &ddfFile,// Puntatore ai dati
								   &dataLen, // Grandezza dei dati
								   KeyBuffer,   // ???> in output
								   0); // ??? in input
					if (status)
					{
						win_infoarg("ERRORE %d in scrittura file.ddf:\nDDF generati incompleti !\n[%s]",
									status,Serv);
						fStop=TRUE;
						fError=TRUE;
						break;
					}
					iIndex=0;

					if (dmiList.Hdl>-1) DMIClose(&dmiList,"DDFLIST2");
					DMIOpen(&dmiList,RAM_AUTO,100000,sizeof(DDFLIST),"DDFLIST");
					break;

			case FHD_FIELD: // Nuova definizione del campo

					// Descrizione
					//FHD_FieldDecode(sFxd.szOldCode,&sFFI);
					ZeroFill(ddfField);
					
					//win_infoarg("[%s]->[%s]",szLastAdbFile,sFxd.szName);

					// Trovo offset di posizione e dimensioni
					if (LocalGetFldPos(szLastAdbFile,sFxd.szName,&ddfField.siFieldOffset,&ddfField.siFieldSize))
					{
						fStop=TRUE;
						fError=TRUE;
						//ehFree(sFFI.ar);
						break;
					}
					
					// Elimino i campi superiori a 254
					//if (ddfField.siFieldSize>254) continue;

					iFieldCount++;
					ddfField.siFieldId=iFieldCount;
					ddfField.siFileId=iFileCount;
					StringCmp(ddfField.szFieldName,sFxd.szName,sizeof(ddfField.szFieldName),TRUE);

					switch (sFxd.psFieldInfo->iType)
					{
						case ADB_ALFA:
								ddfField.bDataTypeCode=ZSTRING_TYPE; //  11 C ZString
								//if (ddfField.siFieldSize>254) ddfField.bDataTypeCode=0; // String
								//if (!sFxd.psFieldInfo->iType2) ddfField.siCaseFlags=TRUE; // Case insesible
								if (sFxd.psFieldInfo->bCaseInsensible) ddfField.siCaseFlags=TRUE; // Case insesible
								break;

						case ADB_BLOB:
								ddfField.bDataTypeCode=21;//B_BLOB;// 21 C ZString
								//win_infoarg("%d",ddfField.siFieldSize);
								//if (!sFxd.psFieldInfo->iType2) ddfField.siCaseFlags=TRUE; // Case insesible
								if (sFxd.psFieldInfo->bCaseInsensible) ddfField.siCaseFlags=TRUE; // Case insesible
								//ddfField.siCaseFlags|=4; // Abilito l'inserimento a NULL
								break;
						
						case ADB_NUME: // Numerico Ferrà = C ZString
								ddfField.bDataTypeCode=ZSTRING_TYPE; // 11 C ZString
								break;

						case ADB_COBD:
								ddfField.bDataTypeCode=DECIMAL_TYPE; // Cobol decimal
								ddfField.bDecimalDelimiter=sFxd.psFieldInfo->iDecimal;
								ddfField.siCaseFlags=1; //4
								break;
						
						case ADB_COBN:
								ddfField.bDataTypeCode=NUMERIC_TYPE; // Cobol Numeric
								ddfField.bDecimalDelimiter=sFxd.psFieldInfo->iDecimal;
								ddfField.siCaseFlags=1; //4
								break;

						case ADB_DATA:
								ddfField.bDataTypeCode=ZSTRING_TYPE; // 11 C ZString
								break;

						case ADB_INT:
						case ADB_BOOL:
						case ADB_INT32:
								ddfField.bDataTypeCode=UNSIGNED_BINARY_TYPE; // 14 Intero a 16bit
								break;

						case ADB_AINC:
								ddfField.bDataTypeCode=AUTOINCREMENT_TYPE; // 14 Intero a 16bit
								break;
								
						case ADB_FLOAT:
								ddfField.bDataTypeCode=IEEE_TYPE; // 2 Float a 4 byte
						break;
					}
					

					// Aggiungo il campo alla lista
					ZeroFill(DDFList);
					DDFList.iId=iFieldCount;
					strcpy(DDFList.szName,sFxd.szName);
					DMIAppend(&dmiList,&DDFList);

					dataLen=sizeof(ddfField);
					//if (iType!=ADB_AINC) ddfField.siCaseFlags|=4; // Abilito l'inserimento a NULL
					status = BTRV( B_INSERT,
								   pbField,
								   &ddfField,// Puntatore ai dati
								   &dataLen, // Grandezza dei dati
								   KeyBuffer,   // ???> in output
								   0); // ??? in input
					if (status)
					{
						win_infoarg("FIELD:\nERRORE %d in scrittura field.ddf:\nDDF generati incompleti !\n[%s]",
									status,sFxd.szName);
						fStop=TRUE;
						fError=TRUE;
						//ehFree(sFFI.ar);
						break;
					}
					//ehFree(sFFI.ar);
					break;

			case FHD_INDEX: // Creazione degli indici

					//FHD_IndexDecode(sFxd.szOldCode,&sFII);
									 //&lpDesc,
									 //&fDup,
									 //&fCaseIns,
									 //&fSort,
									 //&lpComp);

					// ---------------------------------------------------
					// Creazione del nome dell'indice field.ddf
					//
					ZeroFill(ddfField);
					iFieldCount++;
					
					ddfField.siFieldId=iFieldCount;
					ddfField.siFileId=iFileCount;
					StringCmp(ddfField.szFieldName,sFxd.szName,sizeof(ddfField.szFieldName),TRUE);
					ddfField.bDataTypeCode=255;
					ddfField.siFieldOffset=iIndex; 

					dataLen=sizeof(ddfField);
					status = BTRV( B_INSERT,
								   pbField,
								   &ddfField,// Puntatore ai dati
								   &dataLen, // Grandezza dei dati
								   KeyBuffer,   // ???> in output
								   0); // ??? in input
					if (status)
					{
						win_infoarg("INDEX [%s]:\nERRORE %d in scrittura field.ddf:\nDDF generati incompleti !\n[%s]",
									szLastTable,status,sFxd.szName);
						fStop=TRUE;
						fError=TRUE;
						break;
					}
					
					// ---------------------------------------------------
					// Creazione dei record index.ddf
					//
					ZeroFill(ddfIndex);
					ddfIndex.siFileId=iFileCount; // Identificativo del file
					ddfIndex.siIndexNumber=iIndex; // Identificativo del file
					pField=pComposiz=strDup(sFxd.psIndexInfo->lpCompos);
					while (TRUE)
					{
					  CHAR *lpNextField;
					  SINT iId;
					  lpNextField=strstr(pField,"+"); if (lpNextField) *lpNextField=0;
					  // Ricerco nella lista il campo
					  iId=-1;
					  for (b=0;b<dmiList.Num;b++)
					  {
						DMIRead(&dmiList,b,&DDFList);
						if (!strcmp(DDFList.szName,pField)) {iId=DDFList.iId; break;}
					  }

					  if (iId==-1) 
					  {
						ehExit("iId=-1 su dbase %s indice %s campo %s",szLastAdbFile,sFxd.szName,pField);

					  }
					   ddfIndex.siFieldId=(INT16) iId;
					   
					   // Definizione dei flags btrieve (sintesi dell'algoritmo di adb_crea())
					   ddfIndex.siBtrieveFlags=EXTTYPE_KEY|MOD;

					   if (sFxd.psIndexInfo->fDup) ddfIndex.siBtrieveFlags|=DUP; // Duplicabile
					   if (sFxd.psIndexInfo->fCase) ddfIndex.siBtrieveFlags|=NOCASE_KEY; // case insensitive
					   if (lpNextField) ddfIndex.siBtrieveFlags|=SEG; // Segue
					   //if (sFxd.psIndexInfo->fSort) ddfIndex.siBtrieveFlags|=DESC_KEY;
					   if (!strcmp(sFxd.psIndexInfo->pType,"DESC")) ddfIndex.siBtrieveFlags|=DESC_KEY;
					   
					   dataLen=sizeof(ddfIndex);
					   status = BTRV( B_INSERT,
								   pbIndex,
								   &ddfIndex,// Puntatore ai dati
								   &dataLen, // Grandezza dei dati
								   KeyBuffer,   // ???> in output
								   0); // ??? in input
					   if (status)
						{
							win_infoarg("ERRORE %d in scrittura index.ddf:\nDDF generati incompleti !",status);
							fStop=TRUE;
							fError=TRUE;
							break;
						}
					   
					   ddfIndex.siSegmentPart++;
					   if (lpNextField) {*lpNextField='+'; pField=lpNextField+1;} else break;
					}
					ehFree(pComposiz);
					iIndex++;
					break;

		}
		if (fStop) break;
	}

	/*
	// --------------------
	// Definizione chiavi !
	// --------------------
	KS=0;
	KeyMoreGreat=-1;
	KeyDup=0;

	for (a=0;a<Index;a++)
	{
	 flag=0;
		AdbIndex[a].KeySize=0;
	 for (b=0;b<IndexDesc;b++)
	 {
		if (strcmp(AdbIndex[a].IndexName,idx[b].IndexName)) continue;
		flag++;
		if (flag==1) continue;// Il primo Š la descrizione
		FieldLoc=adb_Floc(idx[b].FieldName,RecInfo);
		if (FieldLoc<0)
			{sprintf(serv,"ADB_CREA:Campo in index non trovato\n[%s] ?",idx[b].FieldName);
			 win_errgrave(serv);
			 }
		keySpecs[KS].position = RecInfo[FieldLoc].pos+1; // Posizione
		keySpecs[KS].length = RecInfo[FieldLoc].RealSize; // Larghezza

		if (idx[b].flag==OFF) // Settaggio standard
				{keySpecs[KS].flags = EXTTYPE_KEY|MOD;}
				else
				{keySpecs[KS].flags = idx[b].bti_flag;
				}

		// Se il flag duplicabile Š a ON
		if (idx[b].dup) keySpecs[KS].flags|=DUP;

		if (idx[b].bti_flag&DUP) KeyDup++; // Chiave Duplicabile

		// Prosegue ?
		if ((flag-1)<AdbIndex[a].KeySeg) keySpecs[KS].flags |= 0x10;

		switch (RecInfo[FieldLoc].tipo)
		{
		 case ADB_ALFA :
		 case ADB_NUME :
		 case ADB_DATA :
										keySpecs[KS].type = ZSTRING_TYPE; break;
		 case ADB_BOOL :
		 case ADB_INT  :keySpecs[KS].type = INTEGER_TYPE; break;

		 case ADB_FLOAT:keySpecs[KS].type = IEEE_TYPE; break;
		}
		keySpecs[KS].null = 0;
		AdbIndex[a].KeySize+=RecInfo[FieldLoc].RealSize;//Dimensioni chiave
		if (AdbIndex[a].KeySize>KeyMoreGreat) KeyMoreGreat=AdbIndex[a].KeySize;

		KS++;
	 }
	}

	if (KS!=KeySpec) win_errgrave("ADB_CREA:Errore in definizione indici");

*/

	DMIClose(&dmiList,"DDFLIST");
	// ---------------------------------------------
	// Chiudo i files
	BTRV(B_CLOSE,pbFile,0,0,0,0);
	BTRV(B_CLOSE,pbField,0,0,0,0);
	BTRV(B_CLOSE,pbIndex,0,0,0,0);

//	FHD_DDFReport(szDbPath);
	if (!fError) win_time("DDF creato regolarmente",1);
	MouseCursorDefault();
}

void FHD_DDFViewer(void)
{
	CHAR szDbPath[MAXPATH];
	strcpy(szDbPath,filePath(sFhdProperty.szFileSource));
	FHD_DDFReport(szDbPath);

}

void FHD_DDFReport(CHAR *lpDbPath)
{
	BTI_WORD dataLen;
	BTI_SINT status;
	CHAR pbFile[129]; // PosBlock File
	CHAR pbField[129]; // PosBlock File
	CHAR pbIndex[129]; // PosBlock File
	DDF_FILE  ddfFile;
	DDF_FIELD ddfField;
	DDF_INDEX ddfIndex;
	
	CHAR KeyBuffer[255];
	CHAR szTempFile[MAXPATH];
	CHAR szDDFFile[MAXPATH];
	CHAR szDDFField[MAXPATH];
	CHAR szDDFIndex[MAXPATH];
	SINT a,b;
	FILE *ch;
	CHAR *lpType;
	CHAR Serv[2000];
	SINT iLast;

	typedef struct {
		SINT iNum;
		CHAR *lpName;
	} DATATYPE;


	DATATYPE DType[]={
		{  0,"String"},
		{  1,"Integer"},
		{  2,"IEEE Float"},
		{  3,"Btrieve Date"},
		{  4,"Btrieve Time"},
		{  5,"Cobol Decimal"},
		{  6,"Cobol Money"},
		{  7,"Logical"},
		{  8,"Cobol Numeric"},
		{  9,"Basic BFloat"},
		{ 10,"Pascal LString"},
		{ 11,"C Zstring"},
		{ 12,"Variable Len Note"},
		{ 13,"L Var"},
		{ 14,"Unsigned Binary"}, 
		{ 15,"Autoincrement"},
		{ 16,"Bit"},
		{ 17,"Cobol Numeric STS"},
		{ 21,"Blob"}, // New 2000
		{238,"BCD10"},
		{239,"MagicPC Numeric"},
		{240,"MagicPC Time"},
		{241,"MagicPC Date"},
		{242,"YMD 3byte"},
		{243,"YYMMDD 6byte"},
		{244,"YYYYMMDD 8byte"},
		{245,"TAS Pro3 date"},
		{246,"TAS Pro3 Numeric"},
		{247,"Cobol decimal comp"},

		{253,"VB Date-Time"},
		{254,"VB Currency"},
		{255,"Index"},
		{0,NULL},
	};

	DATATYPE DIndex[]={
		{0x0001,"Duplicates"},
		{0x0002,"Modifiable"},
		{0x0004,"Binary"},
		{0x0008,"Null"},
		{0x0010,"Segmented"},
		{0x0020,"ACS"},
		{0x0040,"descending"},
		{0x0080,"Dupes handled w/ unique suffix"},
		{0x0100,"Extended"},
		{0x0200,"optionally null"},
		{0x0400,"Case insensitive"},
		{0x4000,"key only type"},
		{0x8000,"drop index"},
		{0,NULL},
	};
	
	//sprintf(szTempFile,"%s\\Table.wri",sSetup.pszCreaFolder);
	fileTempName(NULL,"crea",szTempFile,TRUE);
	strcat(szTempFile,".wri");
	ch=fopen(szTempFile,"wb"); if (!ch) return;
	
	// Intestazione RTF
	fprintf(ch,"{\\rtf1\\ansi\\ansicpg1252\\deff0\\deflang1040{\\fonttbl {\\f0\\fnil\\fcharset0 Courier New;}}\n");
	fprintf(ch,"\\uc1\\pard\\ulnone\\f0 ");
	fprintf(ch,"Analisi DDF:%s\\par\n",fileName(lpDbPath));
	fprintf(ch,"\\pard\\ri-1636 \\b \\b0\\par\n","?");
	fprintf(ch,"\\pard\\fs16\\par\n");

	// ---------------------------------------------------
	// Apro il file
	//
	dataLen=0;
	sprintf(szDDFFile,"%sFile.ddf",lpDbPath);
	sprintf(szDDFField,"%sField.ddf",lpDbPath);
	sprintf(szDDFIndex,"%sIndex.ddf",lpDbPath);
	status=BTRV(B_OPEN,pbFile,NULL,&dataLen,szDDFFile,(WORD) NORMAL|MEFS);
	status=BTRV(B_OPEN,pbField,NULL,&dataLen,szDDFField,(WORD) NORMAL|MEFS);
	status=BTRV(B_OPEN,pbIndex,NULL,&dataLen,szDDFIndex,(WORD) NORMAL|MEFS);

	// -------------------------------------------
	// Scansione del FILE.DDF
	//
	dataLen=sizeof(DDF_FILE);
	status=BTRV ((WORD) B_GET_FIRST,
				 pbFile,
				 &ddfFile,    // Puntatore ai dati
				 &dataLen, 										// Grandezza dei dati
				 KeyBuffer,    // Ricerca e riceve chiave
				 (WORD) 0); 									// Indice da usare
	
	fprintf(ch,"FILE.DDF:\\par\n");
//	fprintf(ch,"------------------------------------------------------------------------------------------------------------ \\par\n");
	fprintf(ch,"ID   :TableName           :TableLocation                                                   :bFlag:Reserved\\par\n");
	fprintf(ch,"------------------------------------------------------------------------------------------------------------ \\par\n");
	for (a=0;;a++)
	{
	 ddfFile.szTableName[19]=0;
	 ddfFile.szTableLocation[63]=0;
	 fprintf(ch,"%5d:%-20.20s:%-64.64s:%4d :%d\\par\n",
				ddfFile.siFileId,
				ddfFile.szTableName,
				ddfFile.szTableLocation,
				(SINT) ddfFile.bFileFlags,
				(SINT) ddfFile.Reserved[0]);
	 status=BTRV ((WORD) B_GET_NEXT,
				  pbFile,
				  &ddfFile,    // Puntatore ai dati
				  &dataLen, 										// Grandezza dei dati
				  KeyBuffer,    // Ricerca e riceve chiave
				  (WORD) 0); // Indice da usare
	 if (status) break;
	}

	// -------------------------------------------
	// Scansione del FIELD.DDF
	//
	fprintf(ch,"\\par\n");
	fprintf(ch,"FIELD.DDF:\\par\n");
//	fprintf(ch,"---------------------------------------------------------------------------------------------------- \\par\n");
	fprintf(ch,"ID   :IdFile:FieldName           :Type                              :Offset:Lenght:DecDel: Case\\par\n");
	fprintf(ch,"---------------------------------------------------------------------------------------------------- \\par\n");
	dataLen=sizeof(DDF_FIELD);
	status=BTRV ((WORD) B_GET_FIRST,
				 pbField,
				 &ddfField,    // Puntatore ai dati
				 &dataLen, 										// Grandezza dei dati
				 KeyBuffer,    // Ricerca e riceve chiave
				 (WORD) 0); 									// Indice da usare

	iLast=1;
	for (a=0;;a++)
	{
	 ddfField.szFieldName[19]=0;

	 lpType="?";
	 for (b=0;;b++)
	 {
		 if (DType[b].lpName==NULL) break;
		 if (DType[b].iNum==ddfField.bDataTypeCode) {lpType=DType[b].lpName; break;}
	 }

	 if (iLast!=ddfField.siFileId)
	 {
	    fprintf(ch,"---------------------------------------------------------------------------------------------------- \\par\n");
		iLast=ddfField.siFileId;
	 }
	 fprintf(ch,"%5d:%5d :%-20.20s:%3d %-30.30s: %5d: %5d: %5d:%5d\\par\n",
		     ddfField.siFieldId,
			 ddfField.siFileId,
			 ddfField.szFieldName,

			 (SINT) ddfField.bDataTypeCode,lpType,
			 (SINT) ddfField.siFieldOffset,
			 (SINT) ddfField.siFieldSize,
			 (SINT) ddfField.bDecimalDelimiter,
			 (SINT) ddfField.siCaseFlags);

	 status=BTRV ((WORD) B_GET_NEXT,
				  pbField,
				  &ddfField,    // Puntatore ai dati
				  &dataLen, 										// Grandezza dei dati
				  KeyBuffer,    // Ricerca e riceve chiave
				  (WORD) 0); // Indice da usare
	 if (status) break;
	}

	// -------------------------------------------
	// Scansione del INDEX.DDF
	//
	fprintf(ch,"\\par\n");
	fprintf(ch,"INDEX.DDF:\\par\n");
	//fprintf(ch,"-----------------------------------------------\\par\n");
	fprintf(ch,"FileID :FieldID :Index :Segment : Btrieve Flags \\par\n");
	fprintf(ch,"-----------------------------------------------\\par\n");
	dataLen=sizeof(DDF_INDEX);
	
	status=BTRV ((WORD) B_GET_FIRST,
				 pbIndex,
				 &ddfIndex,    // Puntatore ai dati
				 &dataLen, 										// Grandezza dei dati
				 KeyBuffer,    // Ricerca e riceve chiave
				 (WORD) 0); 									// Indice da usare

	iLast=1;
	for (a=0;;a++)
	{
	  *Serv=0;
	  for (b=0;;b++)
	  {
		 if (DIndex[b].lpName==NULL) break;
		 if (DIndex[b].iNum&ddfIndex.siBtrieveFlags) 
		 {if (*Serv) strcat(Serv,"+");
		  strcat(Serv,DIndex[b].lpName);
		 }
	  }
	
	  if (iLast!=ddfIndex.siFileId)
	  {
		fprintf(ch,"-----------------------------------------------\\par\n");
		iLast=ddfIndex.siFileId;
	  }
	  fprintf(ch," %5d :  %5d :%5d :  %5d : %d %s\\par\n",
		     (SINT) ddfIndex.siFileId,
			 (SINT) ddfIndex.siFieldId,
			 (SINT) ddfIndex.siIndexNumber,
			 (SINT) ddfIndex.siSegmentPart,
			 (SINT) ddfIndex.siBtrieveFlags,
			 Serv);

	 status=BTRV ((WORD) B_GET_NEXT,
				  pbIndex,
				  &ddfIndex,    // Puntatore ai dati
				  &dataLen, 										// Grandezza dei dati
				  KeyBuffer,    // Ricerca e riceve chiave
				  (WORD) 0); // Indice da usare
	 if (status) break;
	}

	BTRV(B_CLOSE,pbFile,0,0,0,0);
	BTRV(B_CLOSE,pbField,0,0,0,0);
	BTRV(B_CLOSE,pbIndex,0,0,0,0);
	
	fprintf(ch,"\\fs20\\par\n");
	fprintf(ch,"}\n");
	fclose(ch);
	
	ShellExecute(WindowNow(),"open",szTempFile,NULL,NULL,SW_MAXIMIZE);
}

#endif

SINT FHD_EnumType(CHAR *lpType)
{
	SINT a;
	for (a=0;;a++)
	{
		if (arFhdTypeList[a]==NULL) return -1;
		if (!strcmp(lpType,arFhdTypeList[a])) return a;
	}
	//win_infoarg("iEnumType ? : [%s]",lpType);
	//return -1;
}

static void FHD_FieldEncode(CHAR *lpCode,FXD_FLD_INFO *lps)
{
    CHAR *p;
	for (p=lps->lpNote;*p;p++) {if (*p=='|') *p='\1';}
	sprintf(lpCode,"%s|%d|%d|%s|%s",arFhdTypeList[lps->iType],lps->iSize,lps->iDecimal,lps->lpNote,lps->lpCharSet);
	//win_infoarg("[%s]",lpCode);
}

//void FHD_GetFieldCode(CHAR *lpCode,SINT *iType,SINT *iSize,SINT *iStyle,CHAR **lpNote)
static void FHD_FieldDecode(CHAR *lpCode,FXD_FLD_INFO *lps)
{
	CHAR *p;
	SINT a,iLen;
	EH_AR arDati;
	memset(lps,0,sizeof(FXD_FLD_INFO));
	strAssign(&lps->lpNote,""); // lps->lpNote=""; 
	strAssign(&lps->lpCharSet,""); // lps->lpCharSet="";

	// momentaneo (da , a |)
	if (sFhdProperty.iVersion<200)
	{
		for (p=lpCode;*p;p++) if (*p==',') *p='|';
	}

	if (sFhdProperty.iVersion<210)
	{
		a=0;
		for (p=lpCode;*p;p++) 
		{
			if (*p=='|') a++;
			if (a>3&&*p=='|') *p='-';
		}
	}

	arDati=ARCreate(lpCode,"|",&iLen); //iLen=ARLen(lps->ar);
	lps->iType=FHD_EnumType(arDati[0]); 
	lps->iSize=atoi(arDati[1]);
	lps->iDecimal=atoi(arDati[2]);
	if (lps->iType==ALFA&&lps->iDecimal) {lps->iDecimal=0; lps->bCaseInsensible=TRUE;}
	if (iLen>3) 
	{
		BYTE *p;
		lps->lpNote=strDup(arDati[3]); 

		//
		// Pulisco dai "meno in coda" inseriti a casa di un bug presente in un breve periodo
		// può essere tolto + avanti
		//
		for (p=lps->lpNote+strlen(lps->lpNote)-1;p>=lps->lpNote;p--) {if (*p=='-') *p=0; else break;}

	}

	if (iLen>4) lps->lpCharSet=strDup(arDati[4]); 
	ARDestroy(arDati);

	/*

	
	p=strstr(lpCode,"|"); if (p==NULL) ehExit("pu"); else *p=0; // Tipo
	lps->iType=FHD_EnumType(lpCode); 
	p2=p+1; p=strstr(p2,"|");  if (p==NULL) return; else *p=0; // Size
	lps->iSize=atoi(p2);
	p2=p+1; p=strstr(p2,"|");  if (p==NULL) return; else *p=0; // Style
	lps->iType2=atoi(p2);
	lps->lpNote=p+1; // Note
	if (!strcmp(lps->lpNote,"NULL")) lps->lpNote="";
	*/
}

static void FHD_TableDecode(CHAR *lpCode,FXD_TAB_INFO *psTable)//CHAR **lpLocalFile,SINT *iFql,CHAR **lpSQLTableName)
{
	CHAR *p,*p2;
	memset(psTable,0,sizeof(FXD_TAB_INFO));
	psTable->pszSqlAlternateName=strDup("");

	p=strstr(lpCode,"|"); if (p) *p=0;
	psTable->pszLocalFile=strDup(lpCode); if (!p) return; 
	p2=p+1; p=strstr(p2,"|");  if (p==NULL) return; else *p=0; // FQL Tipo
	psTable->enSqlPlatform=atoi(p2);
	strAssign(&psTable->pszSqlAlternateName,p+1);
	psTable->pszTableComments=strDup("");
	psTable->pszPresetAutoinc=strDup("");
}

static void FHD_TableEncode(CHAR *lpCode,FXD_TAB_INFO *psTable)//CHAR *lpLocalFile,SINT iFql,CHAR *lpSQLTableName)
{
	sprintf(lpCode,"%s|%d|%s",psTable->pszLocalFile,psTable->enSqlPlatform,strupr(psTable->pszSqlAlternateName));
}


/*
CHAR *FHD_FieldCodeToADB(CHAR *szCode,struct ADB_REC *AdbRec)
{
  CHAR *p,*p2;
  memset(AdbRec,0,sizeof(struct ADB_REC));
  p=szCode;
  // Tipo
  p2=strstr(p,","); if (p2) *p2=0; else ehExit("CADB:1");
  AdbRec->tipo=FHD_EnumType(p); p=p2+1;
  
  // Lunghezza
  p2=strstr(p,","); if (p2) *p2=0; else ehExit("CADB:2");
  AdbRec->size=atoi(p); p=p2+1;

  // Tipo2
  p2=strstr(p,","); if (p2) *p2=0; else ehExit("CADB:3");
  AdbRec->tipo2=atoi(p); p=p2+1;
  return p;  
}

void FHD_ADBToFieldCode(CHAR *szCode,struct ADB_REC *AdbRec)
{
  sprintf(szCode,"%s,%d,%d,NULL",arFhdTypeList[AdbRec->tipo],AdbRec->size,AdbRec->tipo2);
}
*/


//void FHD_IndexEncode(CHAR *lpCode,CHAR *lpDesc,BOOL fDup,BOOL fCase,BOOL fSort,CHAR *lpCompos)
static void FHD_IndexEncode(CHAR *lpCode,FXD_IDX_INFO *lps)
{
//	sprintf(lpCode,"%s|%s|%s|%s|%s",lps->lpDesc,lps->fDup?"1":"0",lps->fCase?"1":"0",lps->fSort?"1":"0",lps->lpCompos);
	sprintf(lpCode,"%s|%s|%s|%s|%s",lps->lpDesc,lps->fDup?"1":"0",lps->fCase?"1":"0",0,lps->lpCompos);
}

//void FHD_IndexDecode(CHAR *lpCode,CHAR **lpDesc,BOOL *fDup,BOOL *fCase,BOOL *fSort,CHAR **lpCompos)
static void FHD_IndexDecode(CHAR *lpCode,FXD_IDX_INFO *lps)
{
	CHAR *p,*p2;
	/*
	*fDup=FALSE;
	*fCase=FALSE;
	*fSort=FALSE; // Ascending (default)
	*lpCompos="?";
	*/
	memset(lps,0,sizeof(FXD_IDX_INFO));
	strAssign(&lps->lpCompos,"?");
	
	p=strstr(lpCode,"|"); if (p==NULL) ehExit("pu"); else *p=0; // Nome
	strAssign(&lps->lpDesc,lpCode);
	p2=p+1; p=strstr(p2,"|");  if (p==NULL) return; else *p=0; // Duplicabile
	lps->fDup=atoi(p2);
	p2=p+1; p=strstr(p2,"|");  if (p==NULL) return; else *p=0; // Case insensitive
	lps->fCase=atoi(p2);
	p2=p+1; p=strstr(p2,"|");  
	if (p!=NULL) {*p=0; 
	//lps->fSort=atoi(p2); 
	if (atoi(p2)) strAssign(&lps->pType,"DESC");
	p2=p+1;} // Direzione (new 2006)
	strAssign(&lps->lpCompos,p2);
}


static void StringCmp(CHAR *lpDest,CHAR *lpSorg,SINT Len,BOOL fSpaceConvert)
{
	CHAR *p;
	CHAR *memo;
	SINT iLenSorg;
	memset(lpDest,32,Len);
	memo=ehAlloc(strlen(lpSorg)+1); strcpy(memo,lpSorg);
	if (fSpaceConvert) 
	{
	 for (p=memo;*p;p++) {if (*p==' ') *p='_';}
	 for (p=memo;*p;p++) {if (*p=='.') *p='_';}
	}
	iLenSorg=strlen(memo); if (iLenSorg>Len) iLenSorg=Len;
	memcpy(lpDest,memo,iLenSorg);
	ehFree(memo);
}


// Codifico le note in modo da poter registrare tutti i codici ASCII e il CR/LF
static CHAR *FHDEncoding(BYTE *lp)
{
	BYTE *lpn=ehAlloc(5000);
	CHAR szServ[20];
	*lpn=0;
	
	for (;*lp;lp++)
	{
		if (*lp<32||*lp>122&&*lp!='|') sprintf(szServ,"{%d}",(SINT) *lp); else sprintf(szServ,"%c",*lp);
		//if (*lp=='|') *lp='\1';
		strcat(lpn,szServ);
	}
	return lpn;
}
static CHAR *FHDDecoding(BYTE *lp)
{
	BYTE *lpn=ehAlloc(5000);
	CHAR szServ[20];
	*lpn=0;
	for (;*lp;lp++)
	{
		if (*lp=='{')
		{
			CHAR *lp2;
			lp2=strstr(lp,"}"); if (!lp2) break; // File corrotto
			*lp2=0;
			sprintf(szServ,"%c",(BYTE) atoi(lp+1));
			lp=lp2;
		}
		else 
		{
			//if (*lp=='\1') *lp='|';
			sprintf(szServ,"%c",*lp);
		}
		strcat(lpn,szServ);
	}
	return lpn;
}

void fxdPropertyBuilder(FXD_ELEMENT *pElement,SINT iType,BOOL bAllocInfo)
{
	pElement->psFolderInfo=NULL;
	pElement->psTableInfo=NULL;
	pElement->psFieldInfo=NULL;
	pElement->psIndexInfo=NULL;

	switch (iType)
	{
		case FHD_FOLDER: 
			pElement->psFolderInfo=ehAllocZero(sizeof(FXD_FOL_INFO)); 
			break;

		case FHD_TABLE: 
			pElement->psTableInfo=ehAllocZero(sizeof(FXD_TAB_INFO)); 
			if (bAllocInfo)
			{
				pElement->psTableInfo->pszLocalFile=strDup("");
				pElement->psTableInfo->pszSqlAlternateName=strDup("");
				pElement->psTableInfo->pszTableComments=strDup("");
				pElement->psTableInfo->pszPresetAutoinc=strDup("");
			}
			break;

		case FHD_FIELD: 
			pElement->psFieldInfo=ehAllocZero(sizeof(FXD_FLD_INFO)); 
			if (bAllocInfo)
			{
				pElement->psFieldInfo->lpCharSet=strDup("");
				pElement->psFieldInfo->lpNote=strDup("");
			}
			break;

		case FHD_INDEX: 
			pElement->psIndexInfo=ehAllocZero(sizeof(FXD_IDX_INFO)); 
			if (bAllocInfo) {
				pElement->psIndexInfo->lpCompos=strDup("");
				pElement->psIndexInfo->lpDesc=strDup("");
			}
			break;

		case FHD_VIEW: 
		case FHD_QUERY: 
			pElement->psQueryInfo=ehAllocZero(sizeof(FXD_QRY_INFO)); 
			if (bAllocInfo) {
				pElement->psQueryInfo->pszQuery=strDup("");
				pElement->psQueryInfo->pszComments=strDup("");
			}
			break;


		default: break;
	}
}

//
// FhdElementClone()
//
FXD_ELEMENT *FhdElementClone(FXD_ELEMENT *pSource)
{
	FXD_ELEMENT *pElement;
	pElement=ehAllocZero(sizeof(FXD_ELEMENT)); 
	memcpy(pElement,pSource,sizeof(FXD_ELEMENT));

	// Clono sotto struttura Tabella
	if (pSource->psFolderInfo)
	{
		fxdPropertyBuilder(pElement,FHD_FOLDER,FALSE);
		memcpy(pElement->psFolderInfo,pSource->psFolderInfo,sizeof(FXD_FOL_INFO));
	}

	// Clono sotto struttura Tabella
	if (pSource->psTableInfo)
	{
		fxdPropertyBuilder(pElement,FHD_TABLE,FALSE);
		memcpy(pElement->psTableInfo,pSource->psTableInfo,sizeof(FXD_TAB_INFO));
		pElement->psTableInfo->pszLocalFile=strDup(pSource->psTableInfo->pszLocalFile);
		pElement->psTableInfo->pszSqlAlternateName=strDup(pSource->psTableInfo->pszSqlAlternateName);
		pElement->psTableInfo->pszTableComments=strDup(pSource->psTableInfo->pszTableComments);
		pElement->psTableInfo->pszPresetAutoinc=strDup(pSource->psTableInfo->pszPresetAutoinc);
//		strAssign(&pElement->psTableInfo->pTableComments,pSource->psTableInfo->pTableComments);
	}

	// Clono sotto struttura Campo
	if (pSource->psFieldInfo)
	{
		fxdPropertyBuilder(pElement,FHD_FIELD,FALSE);
		memcpy(pElement->psFieldInfo,pSource->psFieldInfo,sizeof(FXD_FLD_INFO));
		pElement->psFieldInfo->lpCharSet=strDup(pSource->psFieldInfo->lpCharSet);
		pElement->psFieldInfo->iNullMode=pSource->psFieldInfo->iNullMode;
		//strAssign(&pElement->psFieldInfo->lpCharSet,pSource->psFieldInfo->lpCharSet);
		pElement->psFieldInfo->lpNote=strDup(pSource->psFieldInfo->lpNote);
	}

	// Clono sotto struttura Indice
	if (pSource->psIndexInfo)
	{
		fxdPropertyBuilder(pElement,FHD_INDEX,FALSE);
		memcpy(pElement->psIndexInfo,pSource->psIndexInfo,sizeof(FXD_IDX_INFO));
		pElement->psIndexInfo->lpCompos=strDup(pSource->psIndexInfo->lpCompos);
		pElement->psIndexInfo->lpDesc=strDup(pSource->psIndexInfo->lpDesc);
		pElement->psIndexInfo->pType=strDup(pSource->psIndexInfo->pType);
	}

	// Clono sotto struttura Query
	if (pSource->psQueryInfo)
	{
		fxdPropertyBuilder(pElement,FHD_QUERY,FALSE);
		memcpy(pElement->psQueryInfo,pSource->psQueryInfo,sizeof(FXD_QRY_INFO));
		pElement->psQueryInfo->pszQuery=strDup(pSource->psQueryInfo->pszQuery);
		pElement->psQueryInfo->pszComments=strDup(pSource->psQueryInfo->pszComments);
	}

	return pElement;
}

void FhdElementFree(FXD_ELEMENT *pFhdElement)
{
	if (pFhdElement->psFolderInfo)
	{
		ehFree(pFhdElement->psFolderInfo);
		pFhdElement->psFolderInfo=NULL;
	}

	if (pFhdElement->psTableInfo)
	{
		strAssign(&pFhdElement->psTableInfo->pszLocalFile,NULL);
		strAssign(&pFhdElement->psTableInfo->pszSqlAlternateName,NULL);
		strAssign(&pFhdElement->psTableInfo->pszTableComments,NULL);
		strAssign(&pFhdElement->psTableInfo->pszPresetAutoinc,NULL);
		ehFreePtr(&pFhdElement->psTableInfo); //pFhdElement->psTableInfo=NULL;
	}

	if (pFhdElement->psFieldInfo)
	{
		strAssign(&pFhdElement->psFieldInfo->lpCharSet,NULL);
		strAssign(&pFhdElement->psFieldInfo->lpNote,NULL);
		ehFreePtr(&pFhdElement->psFieldInfo); //pFhdElement->psFieldInfo=NULL;
	}

	if (pFhdElement->psIndexInfo)
	{
		strAssign(&pFhdElement->psIndexInfo->lpCompos,NULL);
		strAssign(&pFhdElement->psIndexInfo->lpDesc,NULL);
		strAssign(&pFhdElement->psIndexInfo->pType,NULL);
		ehFreePtr(&pFhdElement->psIndexInfo); //pFhdElement->psIndexInfo=NULL;
	}

	if (pFhdElement->psQueryInfo)
	{
		ehFreePtr(&pFhdElement->psQueryInfo->pszQuery);
		ehFreePtr(&pFhdElement->psQueryInfo->pszComments);
		ehFreePtr(&pFhdElement->psQueryInfo);
	}

}

//
// FhdSaveFile()
//
BOOL FHD_SaveFile(BYTE *pFileName,_DMI *pdmi)
{
	FXD_ELEMENT sFxd;
	CHAR *lp;
	SINT a;
	FILE *pf1;
	CHAR szOldCode[500];

	if (!fileCheck(pFileName)) return TRUE;
	pf1=fopen(pFileName,"wb"); if (!pf1) return TRUE;

	strcpy(sFhdProperty.szFileSource,(CHAR *) pFileName);
	fprintf(pf1,"#Ver 2.1" CRLF);
	fprintf(pf1,"#Name %s" CRLF,sFhdProperty.szFHDName);
	fprintf(pf1,"#DSN %s" CRLF,sFhdProperty.szDSN);
	fprintf(pf1,"#User %s" CRLF,sFhdProperty.szUser);
	fprintf(pf1,"#Pass %s" CRLF,sFhdProperty.szPass);
	fprintf(pf1,"#Library %s" CRLF,sFhdProperty.szLibrary);
	fprintf(pf1,"#Platform %d" CRLF,sFhdProperty.iDefaultPlatform);
	fprintf(pf1,"#Version %s" CRLF,sFhdProperty.szVersion);
	fprintf(pf1,CRLF);
	fprintf(pf1,"#TabStart" CRLF);

	for (a=0;a<pdmi->Num;a++)
	{
		DMIRead(pdmi,a,&sFxd);
		switch (sFxd.iType)
		{
			case FHD_FOLDER:
				fprintf(pf1,">C,%s" CRLF,sFxd.szName);
				break;
			
			case FHD_TABLE:
				FHD_TableEncode(szOldCode,sFxd.psTableInfo);
				fprintf(pf1,">T,%s,%s" CRLF,szOldCode,sFxd.szName);
				break;

			case FHD_FIELD:
				//FHD_FieldDecode(szOldCode,&sFFI); // Leggo
				FHD_FieldEncode(szOldCode,sFxd.psFieldInfo); // Rifaccio
				lp=FHDEncoding(szOldCode);
				fprintf(pf1,">F,%s,%s" CRLF,sFxd.szName,lp);
				//ehFree(sFFI.ar);
				ehFree(lp);
				break;

			case FHD_INDEX:
				FHD_IndexEncode(szOldCode,sFxd.psIndexInfo); // Rifaccio
				lp=FHDEncoding(szOldCode);
				fprintf(pf1,">I,%s,%s" CRLF,sFxd.szName,szOldCode);
				ehFree(lp);
				break;
		}
	}
	fprintf(pf1,"#TabEnd" CRLF);
	fclose(pf1);
	return FALSE;
}

//
// _FxdSaveFile()
//
static void FXD_SaveFolder(FILE *pf1,_DMI *pdmi,SINT idxFolder);

SINT _FxdSaveFile(BYTE *pFileName,_DMI *pdmi)
{
	BYTE *lp,*lpFileName;
	FILE *pf1;
	SINT a;
	BOOL bFolder;
	FXD_ELEMENT sFxd;

	//
	// Scrivo il formato (FXD) Ferrà eXtended dBase XML 
	//
	lpFileName=strDup((CHAR *) pFileName);
	lp=strReverseStr(lpFileName,"."); 
	if (!lp) strcat(lpFileName,".fxd"); else strcpy(lp,".fxd");

	pf1=fopen(lpFileName,"wb");	
	if (!pf1) {

		SINT iError=GetLastError();
		osError(FALSE,iError,"Errore in scrittura di %s.",lpFileName);
		return iError;
	}

	fprintf(pf1,
		"<?xml version=\"1.0\" encoding=\"utf-8\" ?>" CRLF
		"<fxd version=\"3.0\">" CRLF);

	fprintf(pf1,"<name>%s</name>" CRLF,sFhdProperty.szFHDName);
	xmlTagFileWrite(pf1,"version",sFhdProperty.szVersion,NULL,CRLF);
	fprintf(pf1,"<platform>%d</platform>" CRLF,sFhdProperty.iDefaultPlatform);
	if (sFhdProperty.bBtrieveMigration) fprintf(pf1,"<btrieve>1</btrieve>" CRLF);
	xmlTagFileWrite(pf1,"dsn",sFhdProperty.szDSN,NULL,CRLF);
	xmlTagFileWrite(pf1,"server",sFhdProperty.szServer,NULL,CRLF);
	xmlTagFileWrite(pf1,"library",sFhdProperty.szLibrary,NULL,CRLF);
	xmlTagFileWrite(pf1,"user",sFhdProperty.szUser,NULL,CRLF);
	xmlTagFileWrite(pf1,"pass",sFhdProperty.szPass,NULL,CRLF);
	xmlTagFileWrite(pf1,"comments","",NULL,CRLF);


	//
	// Salvataggio ricorsivo
	// 

	bFolder=FALSE;
	for (a=0;a<pdmi->Num;a++)
	{
		DMIRead(pdmi,a,&sFxd);
		if (sFxd.iType==FHD_FOLDER&&!sFxd.idParent) FXD_SaveFolder(pf1,pdmi,a);
	}
	fprintf(pf1,"</fxd>");
	fclose(pf1);

//	ShellExecute(WindowNow(),"open","notepad++",lpFileName,"",SW_NORMAL);
	ehFree(lpFileName);
	return FALSE;
}

static void FXD_SaveFolder(FILE *pf1,_DMI *pdmi,SINT idxFolder)
{
	FXD_ELEMENT sFxd;
	SINT i,iParent;
	SINT a,b;

	static SINT iLevel=0;
	CHAR szTabs[100],szTabs2[100];

	DMIRead(pdmi,idxFolder,&sFxd); 
	iParent=sFxd.idAutocode;
	iLevel++;

	memset(szTabs,'\t',iLevel); szTabs[iLevel-1]=0;
	memset(szTabs2,'\t',iLevel); szTabs2[iLevel]=0;
	fprintf(pf1,"%s<folder name=\"%s\" expanded=\"%d\">" CRLF,szTabs,sFxd.szName,sFxd.psFolderInfo->bOpen); 
	for (i=idxFolder+1;i<pdmi->Num;i++)
	{
		DMIRead(pdmi,i,&sFxd); if (sFxd.idParent!=iParent) continue;
		//if (sFxd.iType==FHD_FOLDER&&sFxd.idParent!=iParent) break; // Folder non mio
		switch (sFxd.iType)
		{
			case FHD_FOLDER:
				FXD_SaveFolder(pf1,pdmi,i);
				break;
			
			case FHD_VIEW:
			case FHD_QUERY:
				fprintf(pf1,"%s<query name=\"%s\">" CRLF,szTabs2,sFxd.szName);
				fprintf(pf1,"%s<type>%d</type>" CRLF,szTabs2,sFxd.iType);
				xmlTagFileWrite(pf1,"command",sFxd.psQueryInfo->pszQuery,szTabs2,CRLF);
				xmlTagFileWrite(pf1,"comments",sFxd.psQueryInfo->pszComments,szTabs2,CRLF);
				fprintf(pf1,"%s</query>" CRLF,szTabs2);
				a=b-1;
				break;

			case FHD_TABLE:

				fprintf(pf1,"%s<table name=\"%s\">" CRLF,szTabs2,sFxd.szName);
				xmlTagFileWrite(pf1,"sqlname",sFxd.psTableInfo->pszSqlAlternateName,szTabs2,CRLF);
				xmlTagFileWrite(pf1,"filename",sFxd.psTableInfo->pszLocalFile,szTabs2,CRLF);
				xmlTagFileWrite(pf1,"comments",sFxd.psTableInfo->pszTableComments,szTabs2,CRLF);
				xmlTagFileWrite(pf1,"autoinc",sFxd.psTableInfo->pszPresetAutoinc,szTabs2,CRLF);
				fprintf(pf1,"%s<platform>%d</platform>" CRLF,szTabs2,sFxd.psTableInfo->enSqlPlatform);
				//fprintf(pf1,"\t<comments></comments>" CRLF);
//				xmlTagFileWrite(pf1,"comments","",szTabs2,CRLF);

				//
				// Elenco dei campi
				//

				fprintf(pf1,"%s<fields>" CRLF,szTabs2);
				for (b=i+1;b<pdmi->Num;b++)
				{
					DMIRead(pdmi,b,&sFxd); if (sFxd.iType!=FHD_FIELD) break;
					//FHD_FieldDecode(sFxd.szOldCode,&sFFI); // Leggo
					//FHD_FieldEncode(sFxd.szOldCode,&sFFI); // Rifaccio
					fprintf(pf1,"%s\t<field name=\"%s\">" CRLF,szTabs2,sFxd.szName);
					fprintf(pf1,"%s\t\t<type text=\"%s\">%d</type>" CRLF,szTabs2,FhdTypeListXml[sFxd.psFieldInfo->iType],sFxd.psFieldInfo->iType);
					if (sFxd.psFieldInfo->iType!=9) fprintf(pf1,"%s\t\t<size>%d</size>" CRLF,szTabs2,sFxd.psFieldInfo->iSize);
					if (sFxd.psFieldInfo->iDecimal>0) fprintf(pf1,"%s\t\t<decimal>%d</decimal>" CRLF,szTabs2,sFxd.psFieldInfo->iDecimal);
					if (sFxd.psFieldInfo->bCaseInsensible) fprintf(pf1,"%s\t\t<case>%d</case>" CRLF,szTabs2,sFxd.psFieldInfo->bCaseInsensible);
					if (!strEmpty(sFxd.psFieldInfo->lpCharSet)) fprintf(pf1,"%s\t\t<charset>%s</charset>" CRLF,szTabs2,sFxd.psFieldInfo->lpCharSet);
					fprintf(pf1,"%s\t\t<nullMode>%d</nullMode>" CRLF,szTabs2,sFxd.psFieldInfo->iNullMode);
					if (!strEmpty(sFxd.psFieldInfo->lpNote)) 
					{
						BYTE *lpNote=strEncode(sFxd.psFieldInfo->lpNote,SE_UTF8,NULL);
						fprintf(pf1,"%s\t\t<comments>" CDATA_START "%s" CDATA_STOP "</comments>" CRLF,szTabs2,lpNote);
						ehFree(lpNote);
					}
					//ehFree(sFFI.ar);
					fprintf(pf1,"%s\t</field>" CRLF,szTabs2);
				}
				fprintf(pf1,"%s</fields>" CRLF,szTabs2);

				//
				// Elenco degli indici
				//
				fprintf(pf1,"%s<indexes>" CRLF,szTabs2);
				for (;b<pdmi->Num;b++)
				{
					DMIRead(pdmi,b,&sFxd); if (sFxd.iType!=FHD_INDEX) break;
					fprintf(pf1,"%s\t<index name=\"%s\">" CRLF,szTabs2,sFxd.szName);

					//FHD_IndexDecode(sFxd.szOldCode,&sFII);
					if (sFxd.psIndexInfo->lpDesc)
					{
						BYTE *pBuf=strEncode(sFxd.psIndexInfo->lpDesc,SE_UTF8,NULL);
						fprintf(pf1,"%s\t\t<comments>" CDATA_START "%s" CDATA_STOP "</comments>" CRLF,szTabs2,pBuf);
						ehFree(pBuf);
					}
					fprintf(pf1,"%s\t\t<case-ins>%d</case-ins>" CRLF,szTabs2,sFxd.psIndexInfo->fCase);
					fprintf(pf1,"%s\t\t<duplicable>%d</duplicable>" CRLF,szTabs2,sFxd.psIndexInfo->fDup);
//					fprintf(pf1,"%s\t\t<sort>%s</sort>" CRLF,szTabs2,sFxd.psIndexInfo->fSort?"DESC":"ASC");
					fprintf(pf1,"%s\t\t<sort>%s</sort>" CRLF,szTabs2,sFxd.psIndexInfo->pType);
					fprintf(pf1,"%s\t\t<fields>%s</fields>" CRLF,szTabs2,sFxd.psIndexInfo->lpCompos);

					//ehFree(sFII.ar);
					fprintf(pf1,"%s\t</index>" CRLF,szTabs2);
				}							
				fprintf(pf1,"%s</indexes>" CRLF,szTabs2);
				fprintf(pf1,"%s</table>" CRLF,szTabs2);
				a=b-1;
				break;
		}
	}
	fprintf(pf1,"%s</folder>" CRLF,szTabs);
	iLevel--;
}
	

//
// L_AddFolder()
//

BOOL L_AddFolder(XMLDOC sXml,_DMI *pdmi,XMLELEMENT *pFolder,SINT iMainParent)
{
	SINT iFolderCode;
	FXD_ELEMENT sFxd;
	XMLELEMENT **arSubFolder;
	XMLELEMENT **arTable;
	XMLELEMENT **arField;
	XMLELEMENT **arIndex;
	XMLELEMENT *lpXml;
	XMLELEMENT **arQuery;
	BYTE *p;
	SINT iTableParent;
	SINT f,t;

	p=xmlGetAttribAlloc(pFolder,"name",NULL); if (!p) {win_infoarg("File corrotto"); return TRUE;}
	ZeroFill(sFxd);
	sFxd.iType=FHD_FOLDER;
	sFxd.idParent=iMainParent;
	iFolderCode=sFxd.idAutocode=FhdNextNumber();
	strcpy(sFxd.szName,p); ehFree(p);
	fxdPropertyBuilder(&sFxd,FHD_FOLDER,FALSE);
	p=xmlGetAttribAlloc(pFolder,"expanded",NULL); if (p) {sFxd.psFolderInfo->bOpen=atoi(p);}
	ehFree(p);
	DMIAppend(pdmi,&sFxd);
	
	//
	// Loop sulle cartelle (figlie)
	//
	arSubFolder=xmlParser(&sXml,WS_PROCESS,pFolder->idx,".folder"); 
	if (arSubFolder) 
	{	
		for (t=0;arSubFolder[t];t++)
		{
			if (L_AddFolder(sXml,pdmi,arSubFolder[t],iFolderCode)) {ehFree(arSubFolder); return TRUE;}
		}	
		xmlArrayFree(arSubFolder);
	}

	//
	// Loop sulle tabelle (figlie)
	//
	arTable=xmlParser(&sXml,WS_PROCESS,pFolder->idx,".table"); 
	if (arTable) 
	{	
		for (t=0;arTable[t];t++)
		{
			p=xmlGetAttribAlloc(arTable[t],"name",NULL); if (!p) {win_infoarg("File corrotto"); return TRUE;}
			ZeroFill(sFxd);
			iTableParent=sFxd.idAutocode=FhdNextNumber();
			sFxd.idParent=iFolderCode;
			sFxd.iType=FHD_TABLE;
			strcpy(sFxd.szName,p); ehFree(p);
			fxdPropertyBuilder(&sFxd,FHD_TABLE,FALSE);
			
			lpXml=xmlParser(&sXml,WS_FIND,arTable[t]->idx,"table.platform"); 
			if (lpXml) sFxd.psTableInfo->enSqlPlatform=atoi(lpXml->lpValue?lpXml->lpValue:"");

			lpXml=xmlParser(&sXml,WS_FIND,arTable[t]->idx,"table.sqlname"); 
			if (lpXml) strAssign(&sFxd.psTableInfo->pszSqlAlternateName,lpXml->lpValue?lpXml->lpValue:"");
			if (!strCaseCmp(sFxd.psTableInfo->pszSqlAlternateName,sFxd.szName)) 
				strAssign(&sFxd.psTableInfo->pszSqlAlternateName,"");

			lpXml=xmlParser(&sXml,WS_FIND,arTable[t]->idx,"table.filename"); 
			if (lpXml) strAssign(&sFxd.psTableInfo->pszLocalFile,lpXml->lpValue?lpXml->lpValue:"");
			// Azzero il filename "locale" se la piattaforma non lo supporta
			if (sFxd.psTableInfo->enSqlPlatform!=FHD_BTRIEVE&&!sFhdProperty.bBtrieveMigration) 
					strAssign(&sFxd.psTableInfo->pszLocalFile,"");


			lpXml=xmlParser(&sXml,WS_FIND,arTable[t]->idx,"table.comments"); 
			if (lpXml) strAssign(&sFxd.psTableInfo->pszTableComments,lpXml->lpValue?lpXml->lpValue:"");

			lpXml=xmlParser(&sXml,WS_FIND,arTable[t]->idx,"table.autoinc"); 
			if (lpXml) strAssign(&sFxd.psTableInfo->pszPresetAutoinc,lpXml->lpValue?lpXml->lpValue:"");

			//				win_infoarg("%s %d",sFxd.szName,sFxd.psTableInfo->enSqlPlatform);
			DMIAppend(pdmi,&sFxd);

			//
			// Loop sui campi
			//
			arField=xmlParser(&sXml,WS_PROCESS,arTable[t]->idx,".fields.field");
			if (arField)
			{
				for (f=0;arField[f];f++)
				{	
					p=xmlGetAttribAlloc(arField[f],"name",NULL); if (!p) {win_infoarg("File corrotto"); return TRUE;}
					
					ZeroFill(sFxd);
					sFxd.idAutocode=FhdNextNumber();
					sFxd.idParent=iTableParent;
					sFxd.iType=FHD_FIELD;
					strcpy(sFxd.szName,p);
					fxdPropertyBuilder(&sFxd,FHD_FIELD,FALSE);
					ehFree(p);

					lpXml=xmlParser(&sXml,WS_FIND,arField[f]->idx,".type"); 
					if (lpXml) sFxd.psFieldInfo->iType=atoi(lpXml->lpValue?lpXml->lpValue:"");

					lpXml=xmlParser(&sXml,WS_FIND,arField[f]->idx,".size"); 
					if (lpXml) sFxd.psFieldInfo->iSize=atoi(lpXml->lpValue?lpXml->lpValue:"");

					lpXml=xmlParser(&sXml,WS_FIND,arField[f]->idx,".decimal"); 
					if (lpXml) sFxd.psFieldInfo->iDecimal=atoi(lpXml->lpValue?lpXml->lpValue:"");

					lpXml=xmlParser(&sXml,WS_FIND,arField[f]->idx,".case"); 
					if (lpXml) sFxd.psFieldInfo->bCaseInsensible=atoi(lpXml->lpValue?lpXml->lpValue:"");

					lpXml=xmlParser(&sXml,WS_FIND,arField[f]->idx,".charset"); 
					if (lpXml) strAssign(&sFxd.psFieldInfo->lpCharSet,lpXml->lpValue?lpXml->lpValue:"");

					lpXml=xmlParser(&sXml,WS_FIND,arField[f]->idx,".nullMode"); 
					if (lpXml) sFxd.psFieldInfo->iNullMode=atoi(lpXml->lpValue?lpXml->lpValue:"0");

					lpXml=xmlParser(&sXml,WS_FIND,arField[f]->idx,".comments"); 
					if (lpXml) 
					{
						BYTE *p=lpXml->lpValue?lpXml->lpValue:"";
						if (*p)
						{
							WCHAR *pw;
							BYTE *pok;
							strReplace(p,CDATA_START,"");
							strReplace(p,CDATA_STOP,"");
							pw=strDecode(p,SE_UTF8,NULL);
							pok=wcsToStr(pw); ehFree(pw);
							strAssign(&sFxd.psFieldInfo->lpNote,pok);
							ehFree(pok);
						}
					}
					DMIAppend(pdmi,&sFxd);

				}
				xmlArrayFree(arField);
			}

			//
			// Loop sugli indici
			//
			arIndex=xmlParser(&sXml,WS_PROCESS,arTable[t]->idx,".indexes.index");
			if (arIndex)
			{
				for (f=0;arIndex[f];f++)
				{	
					p=xmlGetAttribAlloc(arIndex[f],"name",NULL); if (!p) {win_infoarg("File corrotto"); return TRUE;}

					ZeroFill(sFxd);
					sFxd.idAutocode=FhdNextNumber();
					sFxd.idParent=iTableParent;
					sFxd.iType=FHD_INDEX;
					strcpy(sFxd.szName,p);
					fxdPropertyBuilder(&sFxd,FHD_INDEX,FALSE);
					ehFree(p);

					lpXml=xmlParser(&sXml,WS_FIND,arIndex[f]->idx,".case-ins"); 
					if (lpXml) sFxd.psIndexInfo->fCase=atoi(lpXml->lpValue?lpXml->lpValue:"");

					lpXml=xmlParser(&sXml,WS_FIND,arIndex[f]->idx,".duplicable"); 
					if (lpXml) sFxd.psIndexInfo->fDup=atoi(lpXml->lpValue?lpXml->lpValue:"");

					lpXml=xmlParser(&sXml,WS_FIND,arIndex[f]->idx,".sort"); 
//					if (lpXml) {if (lpXml->lpValue) {if (!strcmp(lpXml->lpValue,"DESC")) sFxd.psIndexInfo->fSort=1;}}
					strAssign(&sFxd.psIndexInfo->pType,"");
					if (lpXml) {if (lpXml->lpValue) {strAssign(&sFxd.psIndexInfo->pType,lpXml->lpValue);}}

					lpXml=xmlParser(&sXml,WS_FIND,arIndex[f]->idx,".fields"); 
					if (lpXml) strAssign(&sFxd.psIndexInfo->lpCompos,lpXml->lpValue?lpXml->lpValue:"");

					lpXml=xmlParser(&sXml,WS_FIND,arIndex[f]->idx,".comments"); 
					if (lpXml) 
					{
						BYTE *p=lpXml->lpValue?lpXml->lpValue:"";
						if (*p)
						{
							WCHAR *pw;
							BYTE *pok;
							strReplace(p,CDATA_START,"");
							strReplace(p,CDATA_STOP,"");
							pw=strDecode(p,SE_UTF8,NULL);
							pok=wcsToStr(pw); ehFree(pw);
							strAssign(&sFxd.psIndexInfo->lpDesc,pok);
							ehFree(pok);
						}
					}

					DMIAppend(pdmi,&sFxd);
				}
				xmlArrayFree(arIndex);
			}
		}
		xmlArrayFree(arTable);
	}

	//
	// Loop sulle eventuali query
	//
	arQuery=xmlParser(&sXml,WS_PROCESS,pFolder->idx,".query"); 
	if (arQuery) 
	{	
		for (t=0;arQuery[t];t++)
		{
			p=xmlGetAttribAlloc(arQuery[t],"name",NULL); if (!p) {win_infoarg("File corrotto"); return TRUE;}
			ZeroFill(sFxd);
			iTableParent=sFxd.idAutocode=FhdNextNumber();
			sFxd.idParent=iFolderCode;
			lpXml=xmlParser(&sXml,WS_FIND,arQuery[t]->idx,"query.type"); 
			if (lpXml) sFxd.iType=atoi(lpXml->lpValue?lpXml->lpValue:"");
			if (!sFxd.iType) ehError();
			strcpy(sFxd.szName,p); ehFree(p);
			fxdPropertyBuilder(&sFxd,FHD_QUERY,FALSE);

			lpXml=xmlParser(&sXml,WS_FIND,arQuery[t]->idx,"query.command"); 
			if (lpXml) strAssign(&sFxd.psQueryInfo->pszQuery,lpXml->lpValue?lpXml->lpValue:"");

			lpXml=xmlParser(&sXml,WS_FIND,arQuery[t]->idx,"query.comments"); 
			if (lpXml) strAssign(&sFxd.psQueryInfo->pszComments,lpXml->lpValue?lpXml->lpValue:"");


			DMIAppend(pdmi,&sFxd);
		}
		xmlArrayFree(arQuery);
	}
	return FALSE;
}

//
// _FxdLoadFile()
//
BOOL _FxdLoadFile(BYTE *pFilename,_DMI *pdmi)
{
	XMLDOC sXml;
	XMLELEMENT *lpXml;
	XMLELEMENT **arFolder;
	SINT a;
	BYTE *p;
	
	// Alloco memoria
	DMIOpen(pdmi,RAM_AUTO,FHDMAXRECORD,sizeof(FXD_ELEMENT),"FhdTable");

	if (!pFilename) return TRUE;

	ZeroFill(sFhdProperty);
	strcpy(sFhdProperty.szFileSource,(CHAR *) pFilename);

	ZeroFill(sXml);
	xmlParser(&sXml,WS_OPEN,0,pFilename); // Apro il file

	//
	// Parametri generali della struttura
	//

	lpXml=xmlParser(&sXml,WS_FIND,0,"fxd"); if (!lpXml) {win_infoarg("File corrotto"); goto ERRORE_XML;}
	p=xmlGetAttribAlloc(lpXml,"version",NULL); if (!p) {win_infoarg("File corrotto"); goto ERRORE_XML;}
	sFhdProperty.iVersion=(SINT) (atof(p)*100);
	ehFree(p);

	lpXml=xmlParser(&sXml,WS_FIND,0,"fxd.name"); if (!lpXml) {win_infoarg("File corrotto"); goto ERRORE_XML;}
	strcpy(sFhdProperty.szFHDName,lpXml->lpValue);
	lpXml=xmlParser(&sXml,WS_FIND,0,"fxd.dsn"); if (lpXml) strcpy(sFhdProperty.szDSN,lpXml->lpValue?lpXml->lpValue:"");
	lpXml=xmlParser(&sXml,WS_FIND,0,"fxd.server"); if (lpXml) strcpy(sFhdProperty.szServer,strEver(lpXml->lpValue));
	lpXml=xmlParser(&sXml,WS_FIND,0,"fxd.user"); if (lpXml)	strcpy(sFhdProperty.szUser,lpXml->lpValue?lpXml->lpValue:"");
	lpXml=xmlParser(&sXml,WS_FIND,0,"fxd.pass"); if (lpXml)	strcpy(sFhdProperty.szPass,lpXml->lpValue?lpXml->lpValue:"");
	lpXml=xmlParser(&sXml,WS_FIND,0,"fxd.version"); if (lpXml) strcpy(sFhdProperty.szVersion,lpXml->lpValue?lpXml->lpValue:"");
	lpXml=xmlParser(&sXml,WS_FIND,0,"fxd.library"); if (lpXml) strcpy(sFhdProperty.szLibrary,lpXml->lpValue?lpXml->lpValue:"");
	lpXml=xmlParser(&sXml,WS_FIND,0,"fxd.platform"); 
		if (lpXml) sFhdProperty.iDefaultPlatform=atoi(lpXml->lpValue?lpXml->lpValue:"");
	lpXml=xmlParser(&sXml,WS_FIND,0,"fxd.btrieve"); if (lpXml) sFhdProperty.bBtrieveMigration=atoi(lpXml->lpValue?lpXml->lpValue:"");
	lpXml=xmlParser(&sXml,WS_FIND,0,"fxd.comments"); // Da gestire <-------------------

	//
	// Richiedo array <folder> (principale)
	//
	arFolder=xmlParser(&sXml,WS_PROCESS,0,"fxd.folder"); if (!arFolder) {return FALSE;}//win_infoarg("File corrotto folder"); goto ERRORE_XML;}

	//
	// Loop sui folders principali
	//
	for (a=0;arFolder[a];a++)
	{
	//	p=xmlGetAttribAlloc(arFolder[a],"name"); if (!p) {win_infoarg("File corrotto"); goto ERRORE_XML;}
	//	ehFree(p);
		if (L_AddFolder(sXml,pdmi,arFolder[a],0)) {ehFree(arFolder); goto ERRORE_XML;}
	}
	ehFree(arFolder);
	xmlParser(&sXml,WS_CLOSE,0,NULL); // Chiudo il file xml
	return FALSE;

ERRORE_XML:
	xmlParser(&sXml,WS_CLOSE,0,NULL); // Chiudo il file xml
	return TRUE;
}

//
// FHD_LoadFile()
//

BOOL FHD_LoadFile(BYTE *pFileName,_DMI *pdmi)
{
	BOOL fTab=FALSE;
	CHAR szOldCode[500];
	CHAR szBuf[1500];
	FILE *pf1;
	CHAR *p;
	BYTE *lpBuffer;
	FXD_ELEMENT sFxd;
	SINT iParent,iTableParent;

	pf1=fopen(pFileName,"r"); if (!pf1) return TRUE;
	ZeroFill(sFhdProperty);
	strcpy(sFhdProperty.szFileSource,(CHAR *) pFileName);

	// Alloco memoria
	DMIOpen(pdmi,RAM_AUTO,FHDMAXRECORD,sizeof(FXD_ELEMENT),"FhdTable");

	while (TRUE)//;for (;;)
	{
		if (!fgets(szBuf,sizeof(szBuf),pf1)) break;
		p=strstr(szBuf,"\n"); if (p) *p=0;
		p=strstr(szBuf,"\r"); if (p) *p=0;
		if (!strcmp(szBuf,"#TabEnd")) fTab=FALSE;

		if (!_memicmp(szBuf,"#Ver ",5)) 
		{
			//win_infoarg("[%s]",szBuf+5);
			sFhdProperty.iVersion=(SINT) (atof(szBuf+5)*100);
		}

		if (!memcmp(szBuf,"#Name",5)) strcpy(sFhdProperty.szFHDName,szBuf+6);
		if (!memcmp(szBuf,"#DSN",4)) strcpy(sFhdProperty.szDSN,szBuf+5);
		if (!memcmp(szBuf,"#User",5)) strcpy(sFhdProperty.szUser,szBuf+6);
		if (!memcmp(szBuf,"#Pass",5)) strcpy(sFhdProperty.szPass,szBuf+6);
		if (!memcmp(szBuf,"#Library",8)) strcpy(sFhdProperty.szLibrary,szBuf+9);
		if (!memcmp(szBuf,"#Platform",9)) sFhdProperty.iDefaultPlatform=atoi(szBuf+10);
		if (!memcmp(szBuf,"#Version",8)) strcpy(sFhdProperty.szVersion,szBuf+9);
		
		// Alloco tabella e classi
		if (fTab)
		{
	//		win_infoarg("[%s]",szBuf);

			lpBuffer=FHDDecoding(szBuf);

			if (lpBuffer[1]=='C') // Classe (Folder)
			{
				ZeroFill(sFxd);
				sFxd.iType=FHD_FOLDER;
				fxdPropertyBuilder(&sFxd,FHD_FOLDER,FALSE);
				iParent=sFxd.idAutocode=FhdNextNumber();
				strcpy(sFxd.szName,lpBuffer+3);
				DMIAppend(pdmi,&sFxd);
			}

			if (lpBuffer[1]=='T') // Table
			{
			 ZeroFill(sFxd);
			 sFxd.iType=FHD_TABLE;
			 p=strstr(lpBuffer+3,","); *p=0;
			 strcpy(szOldCode,lpBuffer+3);
			 
			 fxdPropertyBuilder(&sFxd,FHD_TABLE,FALSE);
			 FHD_TableDecode(szOldCode,sFxd.psTableInfo);//&lpTableLocalFile,&iTableFql,&lpTableSQLTableName);								
			 if (!strCaseCmp(sFxd.psTableInfo->pszSqlAlternateName,sFxd.szName)) strAssign(&sFxd.psTableInfo->pszSqlAlternateName,"");

	//							 if (fTheFile)	sprintf(sFxd.szName,"%s - %s",p+1,lpBuffer+3);
	//											else
			 strcpy(sFxd.szName,p+1); 
			 sFxd.szName[0]=(CHAR) toupper(sFxd.szName[0]);
			 iTableParent=sFxd.idAutocode=FhdNextNumber();
			 sFxd.idParent=iParent;
			 DMIAppend(pdmi,&sFxd);
			}

			if (lpBuffer[1]=='F') // Field
			{
			 ZeroFill(sFxd);
			 sFxd.iType=FHD_FIELD;
			 p=strstr(lpBuffer+3,","); *p=0;
			 strcpy(sFxd.szName,lpBuffer+3); // Nome del Campo
			 sFxd.szName[0]=(CHAR) toupper(sFxd.szName[0]);
			 strcpy(szOldCode,p+1);   // Descrizione del Campo
			 fxdPropertyBuilder(&sFxd,FHD_FIELD,FALSE);
			 FHD_FieldDecode(szOldCode,sFxd.psFieldInfo); // Leggo
			 sFxd.idAutocode=FhdNextNumber();
			 sFxd.idParent=iTableParent;
			 DMIAppend(pdmi,&sFxd);
			}

			if (lpBuffer[1]=='I') // Indici
			{
				 ZeroFill(sFxd);
				 //>F,Codice,ALFA,30,0,No,NULL
				 //>F,Descrizione,ALFA,50,0,No,NULL
				 
				 sFxd.iType=FHD_INDEX;
				 p=strstr(lpBuffer+3,","); *p=0;
				 strcpy(sFxd.szName,lpBuffer+3); // Nome del Campo
				 sFxd.szName[0]=(CHAR) toupper(sFxd.szName[0]);
				 strcpy(szOldCode,p+1);   // Descrizione del Campo
				 fxdPropertyBuilder(&sFxd,FHD_INDEX,FALSE);
				 FHD_IndexDecode(szOldCode,sFxd.psIndexInfo); // Leggo

				 sFxd.idAutocode=FhdNextNumber();
				 sFxd.idParent=iTableParent;
				 DMIAppend(pdmi,&sFxd);
			}
			ehFree(lpBuffer);
		}
		if (!strcmp(szBuf,"#TabStart")) fTab=TRUE;
	}
	fclose(pf1);
	return FALSE;
}

//
// Cancellazione ricorsiva
//

static void FhdRemoveTree(_DMI *pdmi,SINT info)
{
	SINT a,idParent;
	FXD_ELEMENT sFhdElement;

	// Contrassegno da rimuovo 
	DMIRead(pdmi,info,&sFhdElement);
	idParent=sFhdElement.idAutocode;
	sFhdElement.bRemove=TRUE;
//	win_infoarg("%d %s",info,sFhdElement.szName);
	DMIWrite(pdmi,info,&sFhdElement);

	//
	// Controllo se ha figli
	//
	for (a=info+1;a<pdmi->Num;a++)
	{
		DMIRead(pdmi,a,&sFhdElement); if (sFhdElement.idParent!=idParent) continue;
		FhdRemoveTree(pdmi,a); // Rimuovo il figlio
	}
}
void FhdRemove(_DMI *pdmi,SINT info)
{
	SINT a;
	FXD_ELEMENT sFhdElement;
	FhdRemoveTree(pdmi,info);

	for (a=pdmi->Num-1;a>=0;a--)
	{
		DMIRead(pdmi,a,&sFhdElement); 
		if (sFhdElement.bRemove) 
		{
			DMIDelete(pdmi,a,NULL); 
			//win_infoarg("%d [%s]",a,sFhdElement.szName);
		}
	}

}

//
// fxdGetTableName()
//
CHAR * fxdGetTableName(SINT idxTable,BOOL bNotLibrary,CHAR *pszName) //,SINT enSqlPlatform,CHAR *lpLibrary,CHAR *lpTable)
{
	static CHAR szTable[200]="";
	FXD_ELEMENT sFxd;
	CHAR * pszTable;
//	BYTE * pszLibrary=(CHAR *) fxdServer(WS_REALGET,FHD_GET_LIB,NULL);

	fxdServer(WS_REALGET,idxTable,&sFxd);
	if (sFxd.iType!=FHD_TABLE) 
		ehExit("L'indice %d non è una tabella",idxTable);
	if (!pszName) 
		pszTable=(!strEmpty(sFxd.psTableInfo->pszSqlAlternateName))?sFxd.psTableInfo->pszSqlAlternateName:sFxd.szName;
		else
		pszTable=pszName;
	if (bNotLibrary) {
		strcpy(szTable,pszTable);
	}
	else {
		switch (sFxd.psTableInfo->enSqlPlatform)
		{
			case FHD_SQL_ORACLE:
				if (*sFhdProperty.szLibrary)	
					sprintf(szTable,"\"%s\".\"%s\"",sFhdProperty.szLibrary,pszTable);
					else
					sprintf(szTable,"\"%s\"",pszTable);
				break;

			default:
				sprintf(szTable,"%s%s%s",sFhdProperty.szLibrary,*sFhdProperty.szLibrary?".":"",pszTable);
				break;
		}
	}
	return szTable;
}

#ifdef EH_GDI_API 
//
// lstFxdPlatform()
//
void * lstFxdMainPlatform(struct OBJ *psObj,EN_MESSAGE enMess,LONG lParam,void * pVoid)
{
	EH_AR ar;
	INT a;

	switch (enMess) {

		 case WS_OPEN:
			ar=ARNew();
			for (a=1;arsFxdPlatform[a].pszName;a++) {
				ARAddarg(&ar,"%s\t%d",arsFxdPlatform[a].pszName,arsFxdPlatform[a].enCode);
			}
			psObj->ptr=ar;
			if (ARLen(ar)<10) psObj->col2=ARLen(ar); else psObj->col2=10;
			break;

		case WS_CLOSE:
			ARDestroy(psObj->ptr);
			break;

	}

	return NULL;
}


//
// lstFxdPlatform()
//
void * lstFxdPlatform(struct OBJ *psObj,EN_MESSAGE enMess,LONG lParam,void * pVoid)
{
	EH_AR ar;
	INT a;

	switch (enMess) {

		 case WS_OPEN:
			ar=ARNew();
			for (a=0;arsFxdPlatform[a].pszName;a++) {

				if (a)
					ARAddarg(&ar,"%s\t%d",arsFxdPlatform[a].pszName,arsFxdPlatform[a].enCode);
					else {
				
						ARAddarg(	&ar,"%s (%s)\t%d",
									arsFxdPlatform[a].pszName,
									fxdPlatformDesc(sFhdProperty.iDefaultPlatform),
									arsFxdPlatform[a].enCode);
						
					}

			}
			psObj->ptr=ar;
			if (ARLen(ar)<10) psObj->col2=ARLen(ar); else psObj->col2=10;
			break;

		case WS_CLOSE:
			ARDestroy(psObj->ptr);
			break;

	}

	return NULL;
}
#endif

//
// fxdPlatformDesc()
//
CHAR * fxdPlatformDesc(INT iPlatform) {
	
	EN_SQL_PLATFORM_INFO * psPi=fxdPlatformInfo(iPlatform);
	if (psPi) return psPi->pszName;
	return "?";

}
EN_SQL_PLATFORM fxdPlatformReal(EN_SQL_PLATFORM enPlatform) {
	
	EN_SQL_PLATFORM_INFO * psPi=fxdPlatformInfo(enPlatform);
	if (psPi->enCode==FHD_DEFAULT) ehError();
	return psPi->enCode;
}

//
// fxdPlatformDesc()
//
EN_SQL_PLATFORM_INFO * fxdPlatformInfo(INT iPlatform) {
	
	INT a;

	for (a=0;arsFxdPlatform[a].pszName;a++) {
		if (arsFxdPlatform[a].enCode==iPlatform) {
			
			if (iPlatform!=FHD_DEFAULT) 
				return arsFxdPlatform+a;
				else
				{
					if (sFhdProperty.iDefaultPlatform!=FHD_DEFAULT) return fxdPlatformInfo(sFhdProperty.iDefaultPlatform);
			
			}

		};
	}
	return NULL;

}

