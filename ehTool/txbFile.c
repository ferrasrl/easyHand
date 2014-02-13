//   ---------------------------------------------
//	 | txbFile
//	 | Gestione dei dbase in formato Txb (
//   | (da un'illuminazione Aprile 2003)
//   |
//	 |							by Ferrà 2003
//	 |							by Ferrà srl 2011
//   ---------------------------------------------
#include "/easyhand/inc/easyhand.h"
#include "/easyhand/inc/txbFile.h"

#define TXB_HEADER "<TXB><VER>2.0</VER><ENCODING>%s</ENCODING><COLSEP>%s</COLSEP><ROWSEP>%s</ROWSEP></TXB>" CRLF
//#define TXB_MAXSIZERECORD 0xFF00

//static	BYTE *lpEndLine="\1\r\n";
//static	INT iEL=3;
//static	BYTE *lpEndCol="\2";//{'\r','\n'};
//static	INT iEC=1;

static EH_AR _fillBufferEx(PS_TXB psTxb,EN_TXB_ARRAY enArray,INT iCursor,BOOL fAlloc,INT *piRealCursor,CHAR * lpWho);
static BOOL _TXBHeaderParser(PS_TXB psTxb,CHAR * pszBuffer);

//
// ->lpRealLine (Line reali)
//   -> Sort Line (Line ordinate)
//   -> Filter line (line filtrate)

static CHAR * _tagExtract(CHAR * lpx,CHAR * lpTag)
{
	CHAR szBuf[80];
	static CHAR szServ[2000];
	CHAR * lpStart,* lpEnd;
	INT iSize;

	sprintf(szBuf,"<%s>",lpTag);
	lpStart=strstr(lpx,szBuf); if (!lpStart) return NULL;
	
	sprintf(szBuf,"</%s>",lpTag);
	lpEnd=strstr(lpx,szBuf); if (!lpEnd) return NULL;

	iSize=(INT) lpEnd-(INT) lpStart-(strlen(lpTag)+2);
	if (iSize>sizeof(szServ)) ehExit("_tagExtract OVerload");
	memcpy(szServ,lpStart+strlen(lpTag)+2,iSize);
	szServ[iSize]=0;
	return szServ;
}


//
// _cvsString()
//
static CHAR * _cvsString(CHAR *pStart,CHAR cChar) {
	
	CHAR *p,*pDest=pStart;
	
	for (p=pStart+1;*p;p++) {

		if (*p==cChar)
		{
			p++;
			if (*p!=cChar) break;
		}
		*pDest=*p; pDest++;
	}
	*pDest=0;
	memmove(pDest,p,strlen(p));
	return pDest;
}

//
// CSVOpen()
//
PS_TXB CSVOpen(UTF8 * utfFileName, BOOL bUtf8ToAnsi)
{
	// Leggo il file CSV e lo trasformo un TXB
	CHAR * pFileCsv,*pFile;
	CHAR * p=NULL;

	INT a,b,iFld;
	CHAR *	pFinal;
	CHAR *	lpCol="\1",*lpRow="\2\n";
	EH_AR	arRow,arFld;
	DWORD	dwSize;
	PS_TXB	hRet;

	// Caricol il file in memoria
	pFileCsv=fileStrRead(utfFileName); if (!pFileCsv) return NULL;
	pFile=ehAllocZero(strlen(pFileCsv)*2); strcpy(pFile,pFileCsv); ehFree(pFileCsv);

	// Trovo doppi apici e sostituisco separatori
	for (p=pFile;*p;p++) {

		if (*p=='\"') {
			p=_cvsString(p,'\"');}
		else if (*p=='\'') {
			p=_cvsString(p,'\'');}

		//
		// Marco la colonna
		//
		if (*p==';') {*p=*lpCol;}

		//
		// Marco la riga
		//
		if (*p=='\r'||*p=='\n') {
			if (!memcmp(p,"\r\n",2)) 
			{
				strReplace(p,"\r\n",lpRow); 
			}
			else if (!memcmp(p,"\n\r",2)) 
			{
				strReplace(p,"\n\r",lpRow);
			}
			else if (*p=='\r') {
				strReplace(p,"\r",lpRow);
			}
			else if (*p=='\n')  {
				strReplace(p,"\n",lpRow);
			}
			p++; //p+=2;
		}
	}
		
	// puntatore alla stringa finale
	arRow=ARCreate(pFile,lpRow,NULL); 
	pFinal=ehAllocZero(strlen(pFile)*2);
	ehFree(pFile);
	
	//
	// Intesto il file TXB prendendo i dati dal CSV
	//
	arFld=ARCreate(arRow[0],lpCol,&iFld);
	sprintf(pFinal,TXB_HEADER,"ANSI",lpCol,lpRow);
	for (b=0;b<iFld;b++)
	{
		sprintf(strNext(pFinal),"%s,%s%3d%d",
				strTrim(arFld[b]),
				"A",0,0);
		strcat(pFinal,lpCol);
	}
	ARDestroy(arFld);
	strcat(pFinal,lpRow);

//	if (bEncode) { // se richiesto encoding
	for (a=1;arRow[a];a++)
	{
		arFld=ARCreate(arRow[a],lpCol,&iFld);
			
		for (b=0;b<iFld;b++)
		{
//			if (b) strcat(pFinal,lpCol);
			if (bUtf8ToAnsi)
			{
				CHAR *pAnsi=strEncodeEx(1,strTrim(arFld[b]),2,SD_UTF8,SE_ANSI);
				//sprintf(strNext(pFinal),"%s%s",pAnsi,lpCol);
				strcat(pFinal,pAnsi);
				ehFree(pAnsi);
			}
			else
			{
				//sprintf(strNext(pFinal),"%s%s",strTrim(arFld[b]),lpCol);
				strcat(pFinal,strTrim(arFld[b]));
			}
			strcat(pFinal,lpCol);
		}

		strcat(pFinal,lpRow);
		ARDestroy(arFld);
	}
	ARDestroy(arRow);

	dwSize=strlen(pFinal);

#ifdef _DEBUG
	fileStrWrite("c:\\prova.txb",pFinal);
#endif

#ifdef UNICODE
	{
		CHAR szFileName[200];
		UnicodeToChar(szFileName,fileName(utfFileName));
		hRet=txbOpenMemo(szFileName,pFile,dwSize,TRUE);
	}
#else
	hRet=txbOpenMemo(fileName(utfFileName),pFinal,dwSize,TRUE);
#endif

//	ehFree(pFinal);
	return hRet;
}

//
// txbOpenOnFile()
//
static PS_TXB _TXBHeader(UTF8 * utfFileName) {
	
	PS_TXB psTxb;
	INT hdl,iSize;

	iSize=sizeof(S_TXB)+1;
	hdl=memoAlloc(RAM_AUTO,iSize,utfFileName);if (hdl<0) return NULL;
	psTxb=memoLock(hdl); if (!psTxb) ehError();
	memset(psTxb,0,iSize);
	
	psTxb->hdl=hdl;
	psTxb->iVer=TBXVER;
	strcpy(psTxb->utfNomeFile,utfFileName);

	return psTxb;
}

//
// txbOpen()
//
PS_TXB txbOpen(UTF8 * utfFileName)
{
	CHAR * lpFile;
	lpFile=fileStrRead(utfFileName); 
	if (!lpFile) return txbOpenOnFile(utfFileName);

#ifdef UNICODE
	{
		CHAR szFileName[200];
		UnicodeToChar(szFileName,fileName(utfFileName));
		return txbOpenMemo(szFileName,lpFile,dwSize,TRUE);
	}
#else
	return txbOpenMemo(fileName(utfFileName),lpFile,0,TRUE);
#endif
}

//
// txbOpenWeb()
//
#ifdef EH_INTERNET
PS_TXB txbOpenWeb(CHAR * pszUrl,CHAR * pszMethod,S_WEB_ERR * psWebError) {

	EH_WEB * psWeb;
	PS_TXB txb=NULL;

	if (strEmpty(pszUrl)) return NULL;
	psWeb=webHttpReq(pszUrl,pszMethod,NULL,false,30);
	if (psWebError) memcpy(psWebError,&psWeb->sError,sizeof(S_WEB_ERR));
	if (psWeb->sError.enCode) {
		// Errore di ricezione
	}
	else if (strEmpty(psWeb->pData)) {
	}
	else if (!strcmp(psWeb->pData,"<EMPTY>")) {
	}
	else {
		txb=txbOpenMemo("txbFile",psWeb->pData,0,FALSE);
	}
	webHttpReqFree(psWeb);
	return txb;
}
#endif
//
// txbClose()
//
BOOL txbClose(PS_TXB htx) {

	PS_TXB psTxb;
	INT a,hdl;

	psTxb=htx; if (psTxb==NULL) ehExit("txbClose(): htx e' nullo");

	for (a=0;a<psTxb->iFieldCount;a++) {
		ehFree(psTxb->lpFieldInfo[a].lpName);
	}

	ehFreeNN(psTxb->arRealLine);
	ehFreeNN(psTxb->lpFieldPtr);
	ehFreeNN(psTxb->lpBuffer);
	ehFreeNN(psTxb->arSortLine); 
	ehFreeNN(psTxb->arFilterLine);
	ehFreeNN(psTxb->lpFieldInfo);
	ehFreeNN(psTxb->pszSource);
	ehFreeNN(psTxb->ariFis);
	if (psTxb->ch) fclose(psTxb->ch);
#ifdef __windows__	
	if (psTxb->hFile) CloseHandle(psTxb->hFile);
#endif

	hdl=psTxb->hdl;
	memset(psTxb,0,sizeof(S_TXB));
	memoFree(hdl,"HTX");
	return FALSE;
}


//
// txbOpenMemo()
//
PS_TXB txbOpenMemo(CHAR *lpTxbName,
				   CHAR *pszSource,
				   DWORD dwSize,
				   BOOL bUseSource)  // TRUE = psSource non viene "duplicato" ma viene usata la memoria pszSource (fa in modo di non allocare due volte) 
{
	PS_TXB psTxb;
	CHAR * lp, * p;
	INT iLenRecord,iMaxRecord;

	//
	// A) Creo intestazione
	//
	psTxb=_TXBHeader(lpTxbName);
	if (!dwSize) dwSize=strlen(pszSource);//+1;
	psTxb->uiFileSize=dwSize;

	if (bUseSource) 
			psTxb->pszSource=pszSource;
			else
			psTxb->pszSource=strDup(pszSource);
	if (!psTxb->pszSource) ehError();
	psTxb->pszSource[dwSize]=0;
	psTxb->pvPointMax=psTxb->pszSource+dwSize; // -1; // Calcolo la posizione massima del puntatore
	if (_TXBHeaderParser(psTxb,psTxb->pszSource)) {txbClose(psTxb); return NULL;}

	// 
	// B) Conto le linee per generare l'indice
	//
	lp=psTxb->pszDataStart;
	psTxb->iRealLines=0;
	while (TRUE)
	{
		lp=strstr(lp,psTxb->szRowSep); if (!lp) break;
		psTxb->iRealLines++;
		lp+=psTxb->iRowSep;
	}
	if (psTxb->iRealLines) {
		psTxb->arRealLine=ehAlloc(psTxb->iRealLines*sizeof(INT *));

		//
		// Popolo l'indice
		//
		lp=psTxb->pszDataStart; iMaxRecord=0;
		psTxb->iRealLines=0; 
		while (TRUE)
		{
			p=strstr(lp,psTxb->szRowSep); if (!p) break;
			psTxb->arRealLine[psTxb->iRealLines]=lp; *p=0; // Metto la fine ad ogni linea in memoria
			iLenRecord=strlen(lp);if (iLenRecord>iMaxRecord) iMaxRecord=iLenRecord;
			psTxb->iRealLines++;
			lp=p+psTxb->iRowSep;
		}

		psTxb->iMaxBuffer=iMaxRecord+1;//TXB_MAXSIZERECORD;
		psTxb->lpBuffer=ehAllocZero(psTxb->iMaxBuffer); if (!psTxb->lpBuffer) ehError();
	}
	return psTxb;
}


#ifdef __windows__
//
// txbOpenOnFile()
//
PS_TXB txbOpenOnFile(UTF8 * utfFileName) {

	PS_TXB	psTxb;
	BYTE *	pszBuffer;
	DWORD	dwSizeBuffer=0x2000000,dwRead;
	BYTE *	p,*lp;
	UINT64	uiOffset,uiPos,uiLastPos;
	LARGE_INTEGER li;
	INT	iLastPoint;
	INT	iLenRecord;

	if (!fileCheck(utfFileName)) return NULL;
	psTxb=_TXBHeader(fileName(utfFileName)); if (!psTxb) return psTxb;
	psTxb->bFileSource=TRUE;
	psTxb->uiFileSize=fileSize(utfFileName);
	
	//
	// Leggo intestazione file
	//
	pszBuffer=ehAlloc(dwSizeBuffer+1);
	psTxb->hFile = CreateFile((LPCTSTR) utfFileName,
				   GENERIC_READ,
				   FILE_SHARE_READ,
				   NULL,
				   OPEN_EXISTING,
				   0,
				  (HANDLE)NULL);
	ReadFile(psTxb->hFile,pszBuffer,(dwSizeBuffer>1024)?1024:dwSizeBuffer,&dwRead,NULL);
	pszBuffer[dwRead]=0;

	if (_TXBHeaderParser(psTxb,pszBuffer)) {txbClose(psTxb); psTxb=NULL; return psTxb;}

	//
	// Loop sul file per contare le righe
	//
#if (!defined(EH_CONSOLE))
	MouseCursorWait();
#endif
	uiOffset=psTxb->uiDataOffset;
	
	psTxb->dwFisMax=250000;
	psTxb->ariFis=ehAlloc(sizeof(UINT64)*psTxb->dwFisMax);
	while (TRUE) {
	
		li.QuadPart=uiOffset;
		li.LowPart=SetFilePointer(psTxb->hFile,li.LowPart,&li.HighPart,FILE_BEGIN);

		ReadFile(psTxb->hFile,pszBuffer,dwSizeBuffer,&dwRead,NULL); if (!dwRead) break;
		pszBuffer[dwRead]=0;
	
		lp=pszBuffer;
		uiLastPos=uiOffset;
		while (TRUE)
		{
			// <--- lp è l'inizio della rigfa
			p=strstr(lp,psTxb->szRowSep); if (!p) break;
			psTxb->iRealLines++;
			uiPos=uiOffset+((INT) lp-(INT) pszBuffer);
			
//			if (psTxb->iRealLines==1) lpField=lp;
			if (psTxb->iRealLines>1)  {
				DWORD idx=psTxb->iRealLines-2;
				if (idx>=psTxb->dwFisMax) 
				{
					SIZE_T tSizeOld=sizeof(UINT64)*psTxb->dwFisMax;
					psTxb->dwFisMax+=250000;
					psTxb->ariFis=ehRealloc(psTxb->ariFis,tSizeOld,sizeof(UINT64)*psTxb->dwFisMax);
				}
				psTxb->ariFis[idx]=uiPos;

				iLenRecord=(DWORD) (uiPos-uiLastPos); uiLastPos=uiPos;
				if (iLenRecord>psTxb->iMaxBuffer) psTxb->iMaxBuffer=iLenRecord;


			}
			lp=p+psTxb->iRowSep;
		}

		iLastPoint=(INT) lp-(INT) pszBuffer; 
		if (!iLastPoint) ehError(); // Buffer troppo piccolo
		uiOffset+=iLastPoint;//+psTxb->iRowSep;
	}

#if (!defined(EH_CONSOLE))
	MouseCursorDefault();
#endif

	psTxb->iMaxBuffer+=1;psTxb->lpBuffer=ehAllocZero(psTxb->iMaxBuffer); 
	psTxb->iRealLines--;
	if (!psTxb->lpBuffer) ehError();
	psTxb->pszDataStart=NULL;

	return psTxb;
}
#else

PS_TXB txbOpenOnFile(UTF8 * utfFileName) {
	ehExit("txbOpenOnFile() - non implementato in questa piattaforma");
	return NULL;
}

#endif

//
// _TXBHeaderParser()
//
static BOOL _TXBHeaderParser(PS_TXB psTxb,CHAR * pszData) {

	CHAR *lpx,*p,*pszOffset,*lp;

	// Analizzo intestazione del TXB
	if (memcmp(pszData,"<TXB>",5)) 
	{
		//ehFreePtr(&psTxb->pszSource);
		//memoFree(psTxb->hdl,"Memo"); 
		win_infoarg("%s non e' un file TXB",psTxb->utfNomeFile); 
		return TRUE;
	}
	lpx=pszData;
	p=strstr(pszData,"</TXB>" CRLF); if (p) {*p=0; pszOffset=p+8;}

	psTxb->iVer=(INT) (atof(_tagExtract(lpx,"VER"))*100);
	strcpy(psTxb->szColSep,_tagExtract(lpx,"COLSEP")); psTxb->iColSep=strlen(psTxb->szColSep);
	strcpy(psTxb->szRowSep,_tagExtract(lpx,"ROWSEP")); psTxb->iRowSep=strlen(psTxb->szRowSep);

	psTxb->iCharType=0;
	p=_tagExtract(lpx,"ENCODING");
	if (p) 
		{
			if (!strcmp(p,"ASCII")) psTxb->iCharType=ULT_CS_ASCII;
			if (!strcmp(p,"ANSI")) psTxb->iCharType=ULT_CS_ANSI; 
			if (!strcmp(p,"ISO")) psTxb->iCharType=ULT_CS_LATIN1; // Es. &euro;
			if (!strcmp(p,"UTF-8")) psTxb->iCharType=ULT_CS_UTF8;
		}
	psTxb->iCharDecoding=psTxb->iCharType;

	// Conto le linee
	psTxb->iRealLines=0;
	psTxb->uiDataOffset=(INT) pszOffset- (INT) pszData;
//	psTxb->lpData=pszOffset;
	
	// 
	// Estraggo i campi (conteggio e allocazione)
	//
	psTxb->iFieldCount=0;
	lp=pszOffset;
	while (TRUE)
	{
		p=strstr(lp,psTxb->szColSep); if (!p) break;
		if (!memcmp(lp,psTxb->szRowSep,psTxb->iRowSep)) break; // Fine
		psTxb->iFieldCount++;
		lp=p+psTxb->iColSep;
	}

	if (!psTxb->iFieldCount) 
	{
		ehAlert("!psTxb->iFieldCount");
		// Chiudo tutte le memoria impegnate e ritorno NULL (DA FARE)
//		txbClose((HANDLE) psTxb);
		return TRUE;
	}

	//win_infoarg("Field contati [%d]\n\r",psTxb->iFieldCount);

	psTxb->pszDataStart=lp+psTxb->iRowSep;
	psTxb->lpFieldInfo=ehAlloc(psTxb->iFieldCount*sizeof(TXBFLD)); if (!psTxb->lpFieldInfo) ehError();
	psTxb->lpFieldPtr=ehAlloc(psTxb->iFieldCount*sizeof(BYTE *)); if (!psTxb->lpFieldPtr) ehError();

	//
	// Elenco campi
	//
	psTxb->iFieldCount=0; 
	lp=pszOffset;
	while (TRUE)
	{
		EH_AR ar;
		INT iFld;
		CHAR *lp2;

		p=strstr(lp,psTxb->szColSep); if (!p) break;
		if (!memcmp(lp,psTxb->szRowSep,psTxb->iRowSep)) break; // Fine
		*p=0;
		
		//win_infoarg("Nome[%s] %d",lp,psTxb->iFieldCount);
		ar=ARCreate(lp,",",&iFld); if (iFld!=2) ehError();
		psTxb->lpFieldInfo[psTxb->iFieldCount].lpName=strDup(ar[0]);
		psTxb->lpFieldPtr[psTxb->iFieldCount]=NULL;

		lp2=ar[1];
		psTxb->lpFieldInfo[psTxb->iFieldCount].iInfo=*(lp2+4)-'0'; // Info
		*(lp2+4)=0;
		psTxb->lpFieldInfo[psTxb->iFieldCount].iLen=atoi(lp2+1);
		psTxb->lpFieldInfo[psTxb->iFieldCount].enType=_UNKNOW;
		switch (*lp2)
		{
			case 'A': psTxb->lpFieldInfo[psTxb->iFieldCount].enType=_ALFA; break;
			case 'N': psTxb->lpFieldInfo[psTxb->iFieldCount].enType=_NUMBER; break;
			case 'D': psTxb->lpFieldInfo[psTxb->iFieldCount].enType=_DATE; break;
			case 'I': psTxb->lpFieldInfo[psTxb->iFieldCount].enType=_INTEGER; break;
			case 'B': psTxb->lpFieldInfo[psTxb->iFieldCount].enType=_BOOL; break;
			case 'L': psTxb->lpFieldInfo[psTxb->iFieldCount].enType=_INTEGER; break;
			case '+': psTxb->lpFieldInfo[psTxb->iFieldCount].enType=_ID; break;

			// Dati binary (base 64)
			case 'b': psTxb->lpFieldInfo[psTxb->iFieldCount].enType=_BINARY; break;
			case 'p': psTxb->lpFieldInfo[psTxb->iFieldCount].enType=_POINT; break;
			case 'g': psTxb->lpFieldInfo[psTxb->iFieldCount].enType=_GEOMETRY; break;
			case 't': psTxb->lpFieldInfo[psTxb->iFieldCount].enType=_TIME; break;

			default:
				ehError();
		}
//		}
		psTxb->iFieldCount++;
		lp=p+1;
		ARDestroy(ar);

	}


	return FALSE;
}

//
// txbFind()
//
BOOL txbFind(PS_TXB htx,EN_TXB_ARRAY enArray,INT iMode,void *lpb)
{
	PS_TXB psTxb;
	INT iLines;
	psTxb=htx; if (psTxb==NULL) ehExit("txbClose(): htx e' nullo");

	switch (enArray)
	{
		case TXA_REAL: iLines=psTxb->iRealLines; break;
		case TXA_FILTER: iLines=psTxb->iFilterLines; break;
		case TXA_SORT: iLines=psTxb->iSortLines; break;
	}

	switch (iMode)
	{
		case TXB_FIND_FIRST:
			if (iLines<1) return TRUE;
			psTxb->iVirtualCursor=0;
			break;
	
		case TXB_FIND_NEXT:
			if (psTxb->iVirtualCursor>=(iLines-1)) return TRUE;
			psTxb->iVirtualCursor++;
			break;
	}

	// Riempo i campi
	if (_fillBufferEx(psTxb,enArray,psTxb->iVirtualCursor,FALSE,NULL,"find")) return FALSE; else return TRUE;
	//return FALSE;
}

static DWORD bzlen(CHAR *lp)
{
	DWORD dwl;
	for (dwl=0;*lp;lp++,dwl++);
	return dwl;
}

//
// Riempe il buffer
//

INT _wcsCount(WCHAR *pSource,WCHAR pDato)
{
	INT iCount=0;
	for (;*pSource;pSource++)
	{
		//if (strchr(pChar,*pSource)) iCount++;
		if (*pSource==pDato) iCount++;
	}
	return iCount;
}

//
// _fillBufferEx() 
// Ritorna NULL se non riesce a leggere
//
static EH_AR _fillBufferEx(PS_TXB psTxb,EN_TXB_ARRAY enArray,INT iCursor,BOOL fAlloc,INT *piRealCursor,CHAR * pWho)
{
	CHAR * lpLast,*lp;
	INT iLen,iFieldCount;
	INT iRealCursor;
	CHAR * lpBuffer,* pNewBuffer=NULL;
	WCHAR *pw;
	EH_AR arFields=NULL;
	INT iCount;

	//
	// Trovo il cursore reale
	//
	if (iCursor<0) ehExit("_fillBufferEx: Cursor<0");
	iRealCursor=txbGetCursor(psTxb,enArray,iCursor); if (iRealCursor<0) return NULL;

	if (!fAlloc) lpBuffer=psTxb->lpBuffer; else lpBuffer=ehAlloc(psTxb->iMaxBuffer);
	
	// 
	if (!psTxb->bFileSource) {
		iLen=strlen(psTxb->arRealLine[iRealCursor]); 
		if (iLen>=psTxb->iMaxBuffer) ehExit("FillBuffer(%s): out of memory [%s:%d]",pWho,psTxb->utfNomeFile,psTxb->iRealLines);
		strcpy(lpBuffer,psTxb->arRealLine[iRealCursor]);
	}
	else
	{
#ifdef __windows__
		DWORD dwRead;
		LARGE_INTEGER li;
		li.QuadPart=psTxb->ariFis[iRealCursor];
		li.LowPart=SetFilePointer(psTxb->hFile,li.LowPart,&li.HighPart,FILE_BEGIN);
		ReadFile(psTxb->hFile,lpBuffer,psTxb->iMaxBuffer-1,&dwRead,NULL); 
		lpBuffer[dwRead]=0;
		lp=strstr(lpBuffer,psTxb->szRowSep); if (lp) *lp=0;
#else
		ehError();
#endif
	}
	if (piRealCursor) *piRealCursor=iRealCursor;

	// Copio nel buffer di appoggio la linea
	iCount=strCount(lpBuffer,psTxb->szColSep);

	//
	// Eseguo il decoding se necessario
	//
	if (psTxb->iCharDecoding!=psTxb->iCharType)
	{
		// Creo array con le stringhe separate
		EH_AR arFld=ARCreate(lpBuffer,psTxb->szColSep,NULL);
		INT x;
		*lpBuffer=0;

		// Decodifico parola per parola ( la stringa unica era pericoloa)
		for (x=0;arFld[x];x++)
		{
			if (x) strcat(lpBuffer,psTxb->szColSep);
			if (*arFld[x])
			{
				CHAR *pSource=arFld[x];

				// Decodifico
				switch (psTxb->iCharType)
				{
					case ULT_CS_LATIN1:	pw=strDecode(pSource,SE_ISO_LATIN1,NULL); break;
					case ULT_CS_UTF8:	pw=strDecode(pSource,SE_UTF8,NULL); break;
					case ULT_CS_ASCII: pw=strToWcs(pSource); break;
					case ULT_CS_ANSI: pw=strToWcs(pSource); break;
					default: ehError(); pw=NULL;
				}

				// Ri-Codifico
				// if (fAlloc) strAssign(&lpBuffer,pNewBuffer); else {lpBuffer=pNewBuffer; fAlloc=TRUE;}
				switch (psTxb->iCharDecoding)
				{
					case ULT_CS_LATIN1:	pNewBuffer=wcsToStr(pw); break;
					case ULT_CS_UTF8:	pNewBuffer=strEncodeW(pw,SE_UTF8,NULL); break;
					case ULT_CS_ASCII:  pNewBuffer=wcsToStr(pw); break;
					case ULT_CS_ANSI:	pNewBuffer=wcsToStr(pw); break;
					default: ehError();
				}
				ehFreeNN(pw);
			} else pNewBuffer=strDup("");
			strcat(lpBuffer,pNewBuffer); 
			ehFree(pNewBuffer);
		}
		ARDestroy(arFld);
		if (strlen(lpBuffer)>(UINT) psTxb->iMaxBuffer) ehError();
//		strcpy(lpBuffer,pNewBuffer); // 
		
	}

	// win_infoarg("BZ [%s]\n\r",psTxb->lpBuffer);

	//
	// Trovo i puntatori ai campi
	//
	if (fAlloc)
	{
		arFields=ARCreate(lpBuffer,psTxb->szColSep,&iFieldCount);
		iFieldCount--;
	}
	else
	{
//		INT iCount;
		iFieldCount=0; lpLast=lpBuffer; 
		//iCount=ChrCount(lpBuffer,psTxb->szColSep);
		while (TRUE)
		{
			lp=strstr(lpLast,psTxb->szColSep); if (lp) *lp=0;
			if (iFieldCount<psTxb->iFieldCount)
			{
				psTxb->lpFieldPtr[iFieldCount]=lpLast;
			}
			iFieldCount++;
			if (lp) lpLast=lp+strlen(psTxb->szColSep); else break;
		}
		iFieldCount--;
	}

	//
	// Post Controllo Coerenza
	//
	if (iFieldCount!=psTxb->iFieldCount) 
	{
#ifdef _DEBUG
		ehPrintd("[%s][%s]",psTxb->lpFieldPtr[0],psTxb->lpFieldPtr[1]);
#endif

		strcpy(lpBuffer,psTxb->arRealLine[iRealCursor]);
		if (fAlloc) {ARDestroy(arFields); ehFree(lpBuffer);}
		if (psTxb->bSilentError) return NULL;

#ifdef _UNICODE
		ehExit("FillBuffer(%s):\nN.Campi ?\n%S\nLinea %d|%d\nAlloc:%d\n(CONTATI %d != GIUSTI %d)",
#else
		ehExit("FillBuffer(%s):\n"
				"n. campi errato ?\n"
				"TXB: %s, COLSEP: %d\n"
				"Linea %d, max %d\n"
				"Alloc:%d\n"
				"(CONTATI %d != GIUSTI %d)",
#endif
				pWho?pWho:"?",
				psTxb->utfNomeFile,*psTxb->szColSep,
				iRealCursor,psTxb->iRealLines,
				fAlloc,
				iFieldCount,psTxb->iFieldCount);
		//exit(0);
		//win_infoarg("[%s]" CRLF,lpBuffer);
		//ehExit("FillBuffer(): N Field error");
	}

	if (fAlloc) ehFree(lpBuffer); else arFields=psTxb->lpFieldPtr; // Devo ritornare --- >
	return arFields;
}

//
// txbFldPtr()
//
CHAR * txbFldPtr(PS_TXB htx,CHAR * lpCampo)
{
	PS_TXB psTxb; 
	INT a;
	psTxb=htx; if (psTxb==NULL) ehExit("txbFieldPtr(): htx e' nullo");

	// Cerco il campo
	for (a=0;a<psTxb->iFieldCount;a++)
	{
		if (!strcmp(psTxb->lpFieldInfo[a].lpName,lpCampo))
		{
#ifdef TXB_DEBUG
			if (!psTxb->lpFieldPtr[a])
			{
				ehLogWrite("Il campo %s e' NULL",lpCampo);
				win_infoarg("Il campo %s e' NULL",lpCampo);
			}
#endif

			return psTxb->lpFieldPtr[a];
		}
	}

#ifdef TXB_DEBUG
	ehLogWrite("Non trovato il campo: %s",lpCampo);
	win_infoarg("Non trovato il campo: %s",lpCampo);
#endif

	return NULL;
}

//
// txbFldIdx()
//
INT txbFldIdx(PS_TXB htx,CHAR * lpCampo)
{
	PS_TXB psTxb;
	INT a;
	psTxb=htx; if (psTxb==NULL) ehExit("txbFieldPtr(): htx e' nullo");

	// Cerco il campo
	for (a=0;a<psTxb->iFieldCount;a++)
	{
		if (!strcmp(psTxb->lpFieldInfo[a].lpName,lpCampo))
		{
			return a;
		}
	}
	return -1;
}

//
// txbFldInfo()
//
TXBFLD * txbFldInfo(PS_TXB htx,CHAR * lpCampo)
{
	PS_TXB psTxb;
	INT a;
	psTxb=htx; if (psTxb==NULL) ehExit("txbFieldPtr(): htx e' nullo");

	// Cerco il campo
	for (a=0;a<psTxb->iFieldCount;a++)
	{
		if (!strcmp(psTxb->lpFieldInfo[a].lpName,lpCampo))
		{
			return psTxb->lpFieldInfo+a;
		}
	}
	return NULL;
}

//
// txbFldInt()
//
INT txbFldInt(PS_TXB htx,CHAR * lpCampo)
{
	TXBFLD *TXBFLD;
	if (!(TXBFLD=txbFldInfo(htx,lpCampo))) return -123;
	if (TXBFLD->enType!=_INTEGER&&
		TXBFLD->enType!=_BOOL&&
		TXBFLD->enType!=_ID) return -124;

	return atoi(txbFldPtr(htx,lpCampo));
}

//
// txbFldNume()
//
double txbFldNume(PS_TXB htx,CHAR * lpCampo)
{
	TXBFLD *TXBFLD;
	if (!(TXBFLD=txbFldInfo(htx,lpCampo))) return -123;

	if (TXBFLD->enType!=_INTEGER&&
		TXBFLD->enType!=_ID&&
		TXBFLD->enType!=_BOOL&&
		TXBFLD->enType!=_NUMBER) return -124;
	return atof(txbFldPtr(htx,lpCampo));
}
//
// txbVirtualGet()
// Rtirona TRUE se c'è un errore.
//
BOOL txbVirtualGet(PS_TXB htx,EN_TXB_ARRAY enArray,INT iLine)
{
	PS_TXB psTxb;
	psTxb=htx; if (psTxb==NULL) ehExit("TXBGet(): htx e' nullo");

	if (iLine>(psTxb->iRealLines-1)) return TRUE;
	psTxb->iVirtualCursor=iLine;
	if (_fillBufferEx(psTxb,enArray,iLine,FALSE,NULL,"VirtualGet")) return FALSE; 
	return TRUE; // Errore
}

//
//
//
EH_AR txbVirtualGetAlloc(PS_TXB htx,EN_TXB_ARRAY enArray,INT iLine,INT *piRealCursor)
{
	PS_TXB psTxb;
	psTxb=htx; if (psTxb==NULL) ehExit("txbVirtualGetAlloc(): htx e' nullo");
	return _fillBufferEx(psTxb,enArray,iLine,TRUE,piRealCursor,"VirtualGetAlloc");
}

//
// txbGetCursor()
//
INT txbGetCursor(PS_TXB htx,EN_TXB_ARRAY enArray,INT iRow)
{
	INT iRealCursor=0;
	PS_TXB psTxb=htx;
	switch (enArray)
	{
		case TXA_SORT: 
			if (iRow>=psTxb->iSortLines) return -1;
			iRealCursor=psTxb->arSortLine[iRow]; 
			break;

		case TXA_FILTER: 
			if (iRow>=psTxb->iFilterLines) return -1;
			iRealCursor=psTxb->arFilterLine[iRow]; break;

		default: // Reale
			if (iRow>=psTxb->iRealLines) return -1;
			iRealCursor=iRow; 
			break;
	}
	return iRealCursor;
}

//
// txbRealGet()
//
BOOL txbRealGet(PS_TXB htx,INT iLine)
{
	PS_TXB psTxb;
	psTxb=htx; if (psTxb==NULL) ehExit("TXBGet(): htx e' nullo");

	if (iLine>(psTxb->iRealLines-1)) return TRUE;
	psTxb->iRealCursor=iLine;
	if (_fillBufferEx(psTxb,TXA_REAL,iLine,FALSE,NULL,"RealGet")) return FALSE; else return TRUE;

}

// --------------------------------------------------------------------
// Gestione del Sort
// --------------------------------------------------------------------
static CHAR * lpBufferStringA=NULL; // Buffer A utilizzato in fase di sort
static CHAR * lpBufferStringB=NULL; // Buffer B utilizzato in fase di sort
static CHAR ** lpSortFields;
static INT iSortFields;
static PS_TXB lphtxSort;
static BOOL fSortDescendent;
static BOOL fSortCaseNotSensible;

//
//
//
static int _Compare(const void *Dato1, const void *Dato2)
{
	const INT *lpiA,*lpiB;
	INT a;

	lpiA=Dato1; lpiB=Dato2;

	// Leggo primo Record da confrontare

	// Confronto a campo singolo
	if (iSortFields==1)
	{
		if (txbRealGet(lphtxSort,*lpiA)) ehExit("Errore in _Compare A %d (max %d)",*lpiA,lphtxSort->iRealLines);
		// Compongo stringa A
		strcpy(lpBufferStringA,txbFieldPtr((PS_TXB) lphtxSort,lpSortFields[0]));
		// Leggo il secondo elemento
		if (txbRealGet(lphtxSort,*lpiB)) ehExit("Errore in _Compare 2");
		strcpy(lpBufferStringB,txbFieldPtr((PS_TXB) lphtxSort,lpSortFields[0]));
	}
	// Confronto a campo multiplo
	else
	{
		if (txbRealGet(lphtxSort,*lpiA)) ehExit("Errore in _Compare A %d (max %d)",*lpiA,lphtxSort->iRealLines);
		*lpBufferStringA=0;
		for (a=0;a<iSortFields;a++)
		{
			// Compongo stringa A
			strcat(lpBufferStringA,txbFieldPtr((PS_TXB) lphtxSort,lpSortFields[a]));
		}

		// Leggo il secondo elemento
		if (txbRealGet(lphtxSort,*lpiB)) ehExit("Errore in _Compare 2");
		*lpBufferStringB=0;
		for (a=0;a<iSortFields;a++)
		{
			// Compongo stringa A
			strcat(lpBufferStringB,txbFieldPtr((PS_TXB) lphtxSort,lpSortFields[a]));
		}
		//printf("{%s}{%s}\n\r",lpBufferStringA,lpBufferStringB);
	}

	//printf("[%s][%s]\n\r",lpBufferStringA,lpBufferStringB);
	// Algoritmo di comparazione
	if (fSortCaseNotSensible)
	{
#if defined(_WIN32_WCE)
		_strupr(lpBufferStringA); 
		_strupr(lpBufferStringB);
#else
		strUpr(lpBufferStringA); 
		strUpr(lpBufferStringB);
#endif
	}
	
	if (fSortDescendent) return strcmp(lpBufferStringA,lpBufferStringB)*-1;
	return strcmp(lpBufferStringA,lpBufferStringB);
}

//
// txbSort()
//
BOOL txbSort(PS_TXB htx,
			 EN_TXB_ARRAY enArraySource,
			 CHAR * lpFields,
			 BOOL fDesc,
			 BOOL fCaseNotSensible) // TXBS chi devo ordinare (Tutti tranne SORT)
{
	PS_TXB psTxb;
	INT a,iTotalLines;
	CHAR * lp,*lpLast;
	CHAR * lpLocalBuffer;
	INT *arSort;

	psTxb=htx; if (psTxb==NULL) ehExit("txbSort(): htx e' nullo");
	//if (!psTxb->arSortLine) ehExit("TXB:SortLine non allocato");
	if (!psTxb->arSortLine) txbArrayAlloc(htx,TXA_SORT,FALSE);

	if (!lpFields) return TRUE; // Nessun sort richiesto

	lphtxSort=htx;
	fSortDescendent=fDesc;
	fSortCaseNotSensible=fCaseNotSensible;

	lpLocalBuffer=ehAlloc(bzlen(lpFields)+1);
	strcpy(lpLocalBuffer,lpFields);
	lpSortFields=ehAlloc(50*sizeof(BYTE *)); // Massimo 50 campi accodati

	// Estraggo i Campi
	lpLast=lpLocalBuffer;
	iSortFields=0;
	for (lp=lpLocalBuffer;*lp;lp++)
	{
		// Trovata la fine di un campo
		if (*lp=='+')
		{
			*lp=0;
			lpSortFields[iSortFields]=lpLast;
			iSortFields++; lpLast=lp+1;
		}
	}

	lpSortFields[iSortFields]=lpLast; iSortFields++;

	// Pulisco l'array dell'ordinamento
//	psTxb->arSortLine=GlobalAlloc(GMEM_FIXED,psTxb->iRealLines*sizeof(INT));
//	if (!psTxb->lpiSortLine) ehExit("lpiSortLine(): No memory");
	switch (enArraySource)
	{
			case TXA_SORT: 
				ehExit("txbSort(): invocato con TXA_SORT"); break;

			case TXA_REAL: 		
				arSort=psTxb->arSortLine;
				for (a=0;a<psTxb->iRealLines;a++) psTxb->arSortLine[a]=a;
				iTotalLines=psTxb->iRealLines;
				psTxb->iSortLines=iTotalLines;
				break;

			case TXA_FILTER:
				arSort=psTxb->arFilterLine;
				iTotalLines=psTxb->iFilterLines;
				break;
	}

	lpBufferStringA=ehAlloc(psTxb->iMaxBuffer); if (!lpBufferStringA) ehExit("TBXSort(): out of memory A");
	lpBufferStringB=ehAlloc(psTxb->iMaxBuffer); if (!lpBufferStringB) ehExit("TBXSort(): out of memory B");

	//win_infoarg(L"Entrai %d [%d]",iSortFields,psTxb->iRealLines);

	// win_infoarg("Sort %d > %d",iWhereList,iTotalLines);
	// Ordino
	//quicksort
	qsort(arSort,//psTxb->arSortLine, // Puntatore all' array degli indici di linea
		  iTotalLines,  // Numero linee
		  sizeof(INT),		   // Dimensione del campo array
		  _Compare);		   // Funzione di comparazione

	//if (iWhereList==TXBS_FILTER) txbListCopy(htx,TXBS_SORT,TXBS_FILTER);

	//win_infoarg(L"Entrai dopo %d",iSortFields);

	// Libero le risorse impegnate
	ehFree(lpBufferStringA); lpBufferStringA=NULL;
	ehFree(lpBufferStringB); lpBufferStringB=NULL;
	ehFree(lpLocalBuffer);
	ehFree(lpSortFields);
	return FALSE;
}

//
// txbArrayCopy() - Copia una array list in un altro
//
void txbArrayCopy(PS_TXB htx,EN_TXB_ARRAY enSource,EN_TXB_ARRAY enDest)
{
	PS_TXB psTxb=htx;
	switch (enSource)
	{
		case TXA_SORT:
			
			switch (enDest)
			{
				case TXA_REAL: ehExit("txbListCopy(): Permission error"); break;
				case TXA_SORT: break;
				case TXA_FILTER: 
					memcpy(psTxb->arFilterLine,psTxb->arSortLine,sizeof(INT)*psTxb->iSortLines);
					psTxb->iFilterLines=psTxb->iSortLines;
					break;
			}
			break;
	
		case TXA_FILTER:
			
			switch (enDest)
			{
				case TXA_REAL: ehExit("txbListCopy(): Permission error"); break;
				case TXA_FILTER: break;
				case TXA_SORT: 
					memcpy(psTxb->arSortLine,psTxb->arFilterLine,sizeof(INT)*psTxb->iFilterLines);
					psTxb->iSortLines=psTxb->iFilterLines;
					break;
			}
			break;
	}
}

//
// txbArrayAlloc()
//
BOOL txbArrayAlloc(PS_TXB htx,EN_TXB_ARRAY enSourceMode,BOOL fCloneReal)
{
	PS_TXB psTxb=htx;
	INT a;
	switch (enSourceMode)
	{
		case TXA_SORT:
			ehFreePtr(&psTxb->arSortLine);
			psTxb->arSortLine=ehAlloc(psTxb->iRealLines*sizeof(INT));
			if (fCloneReal)
			{
				for (a=0;a<psTxb->iRealLines;a++) psTxb->arSortLine[a]=a;
				psTxb->iSortLines=psTxb->iRealLines;
			}
			break;

		case TXA_FILTER:
			ehFreePtr(&psTxb->arFilterLine);
			psTxb->arFilterLine=ehAlloc(psTxb->iRealLines*sizeof(INT));
			if (fCloneReal)
			{
				for (a=0;a<psTxb->iRealLines;a++) psTxb->arFilterLine[a]=a;
				psTxb->iFilterLines=psTxb->iRealLines;
			}
			break;

		default:
			ehError();
	}
	return FALSE;
}
//
// txbFilter()
// Preparazione array di filtro
//
BOOL txbFilter(PS_TXB htx,EN_TXB_ARRAY enArraySource,BOOL (*proc)(PS_TXB htx,BOOL *)) // 0=Uso originale, 1=Uso Elenco Sortato
{
	PS_TXB psTxb;
	BOOL bBreak;
	INT a;//,iCursor;

	psTxb=htx; if (psTxb==NULL) ehExit("txbFilter(): htx e' nullo");
//	if (!psTxb->arFilterLine) ehExit("arFilterLine non allocato");
	if (!psTxb->arFilterLine) txbArrayAlloc(htx,TXA_FILTER,FALSE);

	psTxb->iFilterLines=0; 
	bBreak=FALSE;
	switch(enArraySource)
	{
		case TXA_REAL:  // 
						// Non ho precedura collegata -> Clono Real
						//
			if (!proc)
			{
				for (a=0;a<psTxb->iRealLines;a++) psTxb->arFilterLine[a]=a;
				psTxb->iFilterLines=psTxb->iRealLines;
				break;
			}

			psTxb->iFilterMax=psTxb->iRealLines;
			for (psTxb->iFilterPos=0;psTxb->iFilterPos<psTxb->iRealLines;psTxb->iFilterPos++)
			{
				//psTxb->iRealCursor=a;
				_fillBufferEx(psTxb,TXA_REAL,psTxb->iFilterPos,FALSE,NULL,"FilterReal");
				if ((*proc)(htx,&bBreak)) {psTxb->arFilterLine[psTxb->iFilterLines]=psTxb->iFilterPos; psTxb->iFilterLines++;}
				if (bBreak) break;
			}
			break;

		case TXA_SORT:  // 
						// Non ho precedura collegata -> Clono sort
						//
			if (!proc)
			{
				//win_infoarg("qui; %d",psTxb->iSortLines);
				memcpy(psTxb->arFilterLine,psTxb->arSortLine,psTxb->iSortLines*sizeof(INT));
				psTxb->iFilterLines=psTxb->iSortLines;
				break;
			}

			psTxb->iFilterMax=psTxb->iSortLines;
			for (psTxb->iFilterPos=0;psTxb->iFilterPos<psTxb->iSortLines;psTxb->iFilterPos++)
			{
				//psTxb->iRealCursor=a;
				_fillBufferEx(psTxb,enArraySource,psTxb->iFilterPos,FALSE,NULL,"FilterReal");
				if ((*proc)(htx,&bBreak)) {psTxb->arFilterLine[psTxb->iFilterLines]=psTxb->iFilterPos; psTxb->iFilterLines++;}
				if (bBreak) break;
			}
			break;
		
		default:
			ehError();
			break;
	}

	return FALSE;
}


//
// txbFindRecord()
//
BOOL txbFindRecord(PS_TXB htx,EN_TXB_ARRAY enArray,BOOL (*proc)(PS_TXB ,void *),void *lpdata)
{
	PS_TXB psTxb;
	INT a;

	psTxb=htx; if (psTxb==NULL) ehExit("txbFindRecord(): htx e' nullo");

	for (a=0;a<psTxb->iRealLines;a++)
	{
		psTxb->iRealCursor=a;
		_fillBufferEx(psTxb,enArray,a,FALSE,NULL,"FindRecord");
		if ((*proc)(htx,lpdata)) return a;
	}
	return -1;
}


#if (defined(_ADB32)||defined(_ADB32P))
extern struct ADB_INFO *ADB_info;

//
// Cerco un un campo
//

static BOOL LFieldFound(CHAR *lpField,CHAR *lpList)
{
	CHAR szNome[80];
	if (!lpList) return FALSE;
	sprintf(szNome,"%s|",lpField);
	if (strstr(lpList,szNome)) return TRUE; else return FALSE;
}

#if (defined(_ADB32)||defined(_ADB32P))

	// 
	// txbAdbExport()
	// Trasforma un ADB un file di testo TXB
	// 
	void txbAdbExport(	HDB hdb,
						INT iIndex,
						UTF8 * utfFileExport,
						CHAR *lpCol,
						CHAR *lpRow,
						CHAR *lpFieldsInclude, // Includere questi campi separati da pipe = NULL TUTTI
						CHAR *lpFieldsExclude, // Non includere questi campi separati da pipe = NULL non controllo
						CHAR *lpAddFields,
						BOOL (*ExternControl)(INT,LONG,void *))
	{
		INT iCount,iMax;
		FILE *chOut;
		INT a;

		struct ADB_HEAD *HeadDest;
		struct ADB_REC *RecordDest;
		struct ADB_REC *FldSource;
		CHAR *lpBlob;

		HeadDest=ADB_info[hdb].AdbHead;
		RecordDest=(struct ADB_REC *) (HeadDest+1);

		// ---------------------------------------------------------
		// Loop sul dizionario campi dell'Hdb di destinazione
		//
		
		chOut=fopen(utfFileExport,"wb");
		if (chOut==NULL) ehExit("Errore in apertura %s",utfFileExport);
		iCount=0; iMax=adb_recnum(hdb);

	//	fprintf(chOut,"TXB1.0%sASCII%s%s",lpCol,lpCol,lpRow);
		fprintf(chOut,"<TXB><VER>2.0</VER><ENCODING>%s</ENCODING><COLSEP>%s</COLSEP><ROWSEP>%s</ROWSEP></TXB>" CRLF,"ASCII",lpCol,lpRow);

		// Scrivo l'intestazione con i campi
		//printf("[%d]\n",HeadDest->Field); getch();
		for (a=0;a<HeadDest->Field;a++) 
		{
			CHAR *lpTipo;
			INT iLen;
			INT iInfo;
			
			if (lpFieldsInclude&&!LFieldFound(RecordDest[a].desc,lpFieldsInclude)) continue;
			if (lpFieldsExclude&&LFieldFound(RecordDest[a].desc,lpFieldsExclude)) continue;

			lpTipo=" ";
			iLen=0; iInfo=0;
			switch (RecordDest[a].tipo)
			{
				case ADB_ALFA: lpTipo="A"; iLen=RecordDest[a].size; iInfo=(RecordDest[a].tipo2)?1:0; break;
				case ADB_NUME: lpTipo="N"; iLen=RecordDest[a].size; iInfo=RecordDest[a].tipo2; break;
				case ADB_DATA: lpTipo="D"; iLen=8; break;
				case ADB_INT: lpTipo="I"; break;
				case ADB_FLOAT: lpTipo="N"; iLen=5; iInfo=2; break;
				case ADB_BOOL: lpTipo="B"; break;
				case ADB_COBD: lpTipo="N"; iLen=RecordDest[a].size; iInfo=RecordDest[a].tipo2; break;
				case ADB_COBN: lpTipo="N"; iLen=RecordDest[a].size; iInfo=RecordDest[a].tipo2; break;
				case ADB_AINC: lpTipo="+"; break;
				case ADB_BLOB: lpTipo="A"; iLen=0; break;
				case ADB_INT32: lpTipo="L"; break;
				default: ehError();
			}
			fprintf(chOut,"%s,%s%3d%d%s",
					RecordDest[a].desc,
					lpTipo,iLen,iInfo,
					lpCol);
		}

		// Se ci sono campi aggiuntivi gli aggiungo in testa
		if (lpAddFields && ExternControl)
		{
			CHAR *lp,*lp2;
			CHAR *lpCopy=strDup(lpAddFields);
			lp=lpCopy;
			while (TRUE)
			{
				lp2=strstr(lp,"|"); if (lp2) *lp2=0;
				fprintf(chOut,"%s,%s%3d%d%s",lp,"A",0,0,lpCol);
				//printf("%s,%s%3d%d%s",lp,"A",0,0,lpCol); 
				if (lp2) {*lp2='|'; lp=lp2+1;} else break;
			}
			ehFree(lpCopy);
		}

		//printf("OK 2\n"); getch();
		fprintf(chOut,lpRow);

		if (!adb_find(hdb,iIndex,B_GET_FIRST,NULL,B_RECORD))
		{
			do
			{
				if (ExternControl) {(*ExternControl)(WS_COUNT,iCount,NULL);}
				if (ExternControl) {if ((*ExternControl)(WS_DO,hdb,NULL)) continue;}

				for (a=0;a<HeadDest->Field;a++) 
				{
	//				if (LFieldJump(RecordDest[a].desc,lpFieldsJump)) continue;
					if (lpFieldsInclude&&!LFieldFound(RecordDest[a].desc,lpFieldsInclude)) continue;
					if (lpFieldsExclude&&LFieldFound(RecordDest[a].desc,lpFieldsExclude)) continue;
					adb_FldInfo(hdb,RecordDest[a].desc,&FldSource);

					//if (iCount>1221) {win_infoarg("%d %s [%d]",a,RecordDest[a].desc,FldSource->tipo); getch(); }

					//fprintf(chOut,"%s%s",RecordDest[a].desc,lpCol);
					switch (FldSource->tipo)
					{
 						case ADB_NUME : 
						case ADB_COBD : 
						case ADB_COBN : 
						case ADB_FLOAT:
							fprintf(chOut,"%6.3f",adb_FldNume(hdb,RecordDest[a].desc));
							break;

						case ADB_BOOL :
						case ADB_INT  : 
						case ADB_INT32: 
						case ADB_AINC:
							fprintf(chOut,"%d",adb_FldInt(hdb,RecordDest[a].desc));
							break;
			 
						case ADB_ALFA :
						case ADB_DATA :
							//win_infoarg("[%s]",adb_FldPtr(hdb,RecordDest[a].desc));
							fprintf(chOut,"%s",adb_FldPtr(hdb,RecordDest[a].desc));
							break;
						
						case ADB_BLOB:
							lpBlob=adb_BlobGetAlloc(hdb,RecordDest[a].desc);
							//if (iCount>1221) {win_infoarg(lpBlob);}

							fprintf(chOut,"%s",lpBlob);
							adb_BlobFree(lpBlob);
							break;
					}
					fprintf(chOut,lpCol);
				}

				// Se ci sono campi aggiuntivi li chiedo al processo esterno
				if (lpAddFields&&ExternControl)
				{
					CHAR *lp,*lp2,*lp3;
					CHAR *lpCopy=strDup(lpAddFields);
					lp=lpCopy;
					while (TRUE)
					{
						lp2=strstr(lp,"|"); if (lp2) *lp2=0;
						lp3=lp;

						(*ExternControl)(WS_REALGET,hdb,&lp3);
						fprintf(chOut,"%s",lp3);
						ehFree(lp3);
						fprintf(chOut,lpCol);
						if (lp2) {*lp2='|'; lp=lp2+1;} else break;
					}
					ehFree(lpCopy);
				}


				fprintf(chOut,lpRow);
				iCount++;
				//if (!(iCount%30)) 
	//			printf("Export [%s]: Record %ld (%3d%%)\r",lpFileExport,iCount,iCount*100/iMax);
			} while (!adb_find(hdb,iIndex,B_GET_NEXT,NULL,B_RECORD));
		}
	//	if (iMax) printf("Export [%s]: Record %ld (%3d%%)\r",lpFileExport,iCount,iCount*100/iMax);
		fclose(chOut);
	//	printf("\r\n");
	}
#endif

//
//
//
BOOL txbFldImport(HDB hdbDest,PS_TXB txbSource,BOOL fCutSpace)
{
	INT a;
	CHAR *pt;
	struct ADB_HEAD *HeadDest;
	struct ADB_REC *RecordDest;
	CHAR *pdata;
	INT iError;

	HeadDest=ADB_info[hdbDest].AdbHead;
	RecordDest=(struct ADB_REC *) (HeadDest+1);
//		win_infoarg("[%s]=[%s]",RecordDest[a].desc,pt);

	// ---------------------------------------------------------
	// Loop sul dizionario campi dell'Hdb di destinazione
	//
	
	for (a=0;a<HeadDest->Field;a++) 
	{
		pt=txbFldPtr(txbSource,RecordDest[a].desc);
		adb_FldReset(hdbDest,RecordDest[a].desc);
		// ------------------------------------------------------
		// Nuovo Nome campo inesistente nel file sorgente
		if (pt) 
		{
			//adb_FldReset(hdbDest,RecordDest[a].desc);
//		}
//		else
//		{
		 switch (RecordDest[a].tipo)
		 {
			// da ALFA --> ALFA
			// da BLOB --> ALFA
			case ADB_ALFA: // <--- Destinazione
			
				 if (fCutSpace) strTrimRight(pt);
				 if ((INT) strlen(pt)>RecordDest[a].size) pt[RecordDest[a].size]=0; // new 2005
				 adb_FldWrite(hdbDest,RecordDest[a].desc,pt,0);
				break;
			
			// da BLOB --> BLOB
			// da ALFA --> BLOB
			case ADB_BLOB: // Tipo destinazione
				
				  if (fCutSpace) strTrimRight(pt);
#ifndef EH_CONSOLE
				  if (strlen(pt)>1024) win_infoarg("txbFldImport: %s > 1024 ( %d )",RecordDest[a].desc,strlen(pt));
#endif
				  adb_FldWrite(hdbDest,RecordDest[a].desc,pt,0);
				  break;

			case ADB_DATA:
				pdata=adb_FldPtr(hdbDest,RecordDest[a].desc);
				memcpy(pdata,pt,8); *(pdata+8)=0; // 0 Finale
				break;
				
			case ADB_INT:
			case ADB_INT32:
			case ADB_BOOL:
			case ADB_NUME:
			case ADB_COBN:
			case ADB_COBD:
			case ADB_AINC:

				if (adb_FldWriteCheck(hdbDest,RecordDest[a].desc,NULL,atof(pt),TRUE,&iError))
				{
#ifndef EH_CONSOLE
					if (win_ask("Proseguo con l'importazione ?")==ESC) return TRUE;
#endif
				}
				else
				{
				  adb_FldWrite(hdbDest,RecordDest[a].desc,NULL,atof(pt));
				}
				break;
				
		 } // Switch
		} // Else
	}// For
 return FALSE;
}

#endif

// -----------------------------------------------------------
// txbCreate()
// Crea un file TXB
// -----------------------------------------------------------
#ifndef _UNICODE
PS_TXB txbCreate(UTF8 * utfFileName,
			     CHAR *lpCol, // Carattere separatore colonne
			     CHAR *lpRow, // Carattere separatore righe
				 CHAR *lpEnconding, // NULL= default (ASCII)
			     TXBFLD *arFld)// Descrizione dei campi
{
	INT a;
	FILE *ch;
	CHAR *lpTipo;
	INT iLen,iInfo;
	DWORD dwSize;
	INT hdl,iRealSize;
	PS_TXB psTxb;
	
	//
	// Non testato con nomi strani in windows
	//
	ch=fopen(utfFileName,"wb"); if (ch==NULL) ehExit("Errore in apertura %s",utfFileName);

	// Creo zona di memoria con la descrizione
	dwSize=0;//fileSize(utfFileName);
	iRealSize=dwSize+sizeof(S_TXB)+1;
	hdl=memoAlloc(RAM_AUTO,iRealSize,fileName(utfFileName));
	if (hdl<0) {fclose(ch); return NULL;}

	psTxb=memoLock(hdl); 
	memset(psTxb,0,iRealSize);
	psTxb->hdl=hdl;
	psTxb->iVer=TBXVER;
	psTxb->pszDataStart=(CHAR *) psTxb; 
	psTxb->pszDataStart+=sizeof(S_TXB);
	psTxb->pszDataStart[dwSize]=0; // ho dei dubbi...

	if (strEmpty(lpEnconding)) lpEnconding="ASCII";
	fprintf(ch,TXB_HEADER ,lpEnconding,lpCol,lpRow);
	strcpy(psTxb->szColSep,lpCol);
	strcpy(psTxb->szRowSep,lpRow);
	psTxb->iFieldCount=0;
	for (a=0;;a++)
	{
		if (!arFld[a].lpName) break;
		psTxb->iFieldCount++;
	}

	//psTxb->lpFieldName=ehAlloc(psTxb->iFieldCount*sizeof(BYTE *));
	psTxb->lpFieldInfo=ehAlloc(psTxb->iFieldCount*sizeof(TXBFLD));
	psTxb->lpFieldPtr=ehAlloc(psTxb->iFieldCount*sizeof(BYTE *));

	for (a=0;;a++)
	{
		if (!arFld[a].lpName) break;

		lpTipo=" ";
		iLen=0; iInfo=0;
		switch (arFld[a].enType)
		{
			case _ALFA:  lpTipo="A"; iLen=arFld[a].iLen; iInfo=arFld[a].iInfo; break;
			case _NUMBER:  lpTipo="N"; iLen=arFld[a].iLen; iInfo=arFld[a].iInfo; break;
			case _DATE:  lpTipo="D"; iLen=8; break;
			case _INTEGER:   lpTipo="I"; break;
//			case _NUMBER: lpTipo="N"; iLen=5; iInfo=2; break;
			case _BOOL:  lpTipo="B"; break;
//			case ADB_COBD:  lpTipo="N"; iLen=arFld[a].iLen; iInfo=arFld[a].iInfo; break;
//			case ADB_COBN:  lpTipo="N"; iLen=arFld[a].iLen; iInfo=arFld[a].iInfo; break;
			case _ID:  lpTipo="+"; break;
			case _TEXT:  lpTipo="A"; iLen=0; break;
			// Campi binari
			case _BINARY:  lpTipo="b"; iLen=0; break;
			case _POINT:  lpTipo="p"; iLen=0; break;
			case _GEOMETRY:  lpTipo="g"; iLen=0; break;
			case _TIME:  lpTipo="t"; iLen=0; break;
			default: 
				ehExit("Tipo campo non gestito: %d",arFld[a].enType);
				break;
//			case ADB_INT32: lpTipo="L"; break;
		}
//		psTxb->iFieldCount++;
		fprintf(ch,"%s,%s%3d%d%s",
				arFld[a].lpName,
				lpTipo,
				iLen,
				iInfo,
				lpCol);

		//psTxb->lpFieldName[a]=lpFld[a].lpNome;
		psTxb->lpFieldPtr[a]=NULL;
		memcpy(psTxb->lpFieldInfo+a,arFld+a,sizeof(TXBFLD));
		psTxb->lpFieldInfo[a].lpName=strDup(arFld[a].lpName);
	}
	fprintf(ch,"%s",lpRow);
	psTxb->ch=ch;
	return psTxb;	
}
#endif


//
// txbArgWrite()
//
void txbArgWrite(PS_TXB lph, // Handle Txb
				 CHAR * pszSep, // Carattere di separazione campi
				 CHAR *	pszFormat,
				 ...)
{
	va_list Ah;
	INT a,idx;
	PS_TXB lpTxb=(PS_TXB ) lph;	
	CHAR *lp;
	CHAR *lpBufProc=ehAlloc(64000); 
//	CHAR *lpBufWrite=ehAlloc(64000);

	if (pszFormat) 
	{
	 va_start(Ah,pszFormat);
	 vsprintf(lpBufProc,pszFormat,Ah); // Elaboro stringa

	// win_infoarg("[%s]",lpBufProc);
	 lp=strtok(lpBufProc,pszSep);
	 idx=0;
	 while (lp)
	 {
		if (idx>=lpTxb->iFieldCount) ehExit("txbArgWrite: overload %d>=%d",idx,lpTxb->iFieldCount);
		
	//	win_infoarg("%d = %s",idx,lp);
		switch (lpTxb->lpFieldInfo[idx].enType)
		 {
// 			case ADB_NUME : 
//			case ADB_COBD : 
//			case ADB_COBN : 
//			case ADB_FLOAT:
			case _NUMBER:

				fprintf(lpTxb->ch,"%6.3f",atof(lp));
				break;

//			case ADB_BOOL :
//			case ADB_INT  : 
//			case ADB_INT32: 
//			case ADB_AINC:
			case _ID:
			case _INTEGER:
			case _BOOL:

				fprintf(lpTxb->ch,"%d",atoi(lp));
				break;
		 
//			case ADB_ALFA :
//			case ADB_DATA :
//			case ADB_BLOB:
			case _ALFA:
			case _TEXT:
				fprintf(lpTxb->ch,"%s",lp);
				break;
		 }
		 
		fprintf(lpTxb->ch,"%s",lpTxb->szColSep);
		idx++; lp=strtok(NULL,pszSep);
	 }

	 // Riempo i campi vuoti
	 for (a=idx;a<lpTxb->iFieldCount;a++)
	 {
		switch (lpTxb->lpFieldInfo[a].enType)
		 {
// 			case ADB_NUME : 
//			case ADB_COBD : 
//			case ADB_COBN : 
//			case ADB_FLOAT:
			case _NUMBER:
				fprintf(lpTxb->ch,"%6.3f",.0f);
				break;

//			case ADB_BOOL :
//			case ADB_INT  : 
//			case ADB_INT32: 
//			case ADB_AINC:
			case _ID:
			case _INTEGER:
			case _BOOL:
				fprintf(lpTxb->ch,"%d",0);
				break;
		 
//			case ADB_ALFA :
//			case ADB_DATA :
//			case ADB_BLOB:
			case _ALFA:
			case _TEXT:
				fprintf(lpTxb->ch,"");
				break;
		 }
	 }

	 fprintf(lpTxb->ch,"%s",lpTxb->szRowSep);
	 va_end(Ah);
	}

	ehFree(lpBufProc);
//	ehFree(lpBufWrite);
}



/*
// http://www.w3.org/TR/REC-html40/charset.html#h-5.1
// ISOEncoding
// Traduce i &lt; o i &#x; etc , in ASCII
void ISOtoAscii(CHAR *lpBuffer)
{
	CHAR *pS,*pD;
	pS=pD=lpBuffer;
	for (pS=lpBuffer;*pS;pS++)
	{
		if (*pS=='&')
		{
			CHAR *pE;
			pE=strstr(pS,";"); if (!pE) continue; else *pE=0;
			if (strlen(pS)<3) {pS=pE;  continue;}
			_strlwr(pS);
			if (!strcmp(pS,"&lt")) {*pD='<'; pD++;}
			if (!strcmp(pS,"&gt")) {*pD='>'; pD++;}
			if (!strcmp(pS,"&amp")) {*pD='&'; pD++;}
			if (!strcmp(pS,"&quot")) {*pD='\"'; pD++;}
			if (!memcmp(pS,"&#x",3)) {*pD=xtoi(pS+3); pD++;}
			if (!memcmp(pS,"&#",2)) {*pD=atoi(pS+2); pD++;}
			pS=pE;
			continue;
		}
		else
		{
			*pD=*pS; pD++;
		}
	}
	*pD=0;
}
*/