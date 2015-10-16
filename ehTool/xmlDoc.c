//   ---------------------------------------------
//   | xmlDoc - Gestore di file XML 
//   |         
//   |							Ferrà 2005-2014
//   ---------------------------------------------
//
#include "/easyhand/inc/easyhand.h"
#include "/easyhand/inc/xmlDoc.h"

static void		_xmlScan(XMLD * psDoc,CHAR **lpSource,XMLPTR psParent);
static CHAR *	_getEntity(XMLD *px,CHAR **lppSource,INT *lpiSize,EN_XMLTAG_IS *lpiType,BYTE **lpSourcePoint);
static INT		_lineNumberSearch(BYTE *pSource,CHAR *pLimit);
static void		_lineCountUpdate(XMLD * psDoc,BYTE *pStart,BYTE *pEnd);
static EH_LST	_xmlAttribParser(CHAR * pszAtt);
static void		_nameSet(XMLD * psDoc,XMLPTR px,CHAR * pszName);
//static INT iCurrentLine=0;


//
// xmlOpen()
//
XMLD *	xmlOpen(CHAR * pszData,INT iMode,BOOL bUseNameSpace) {

	CHAR * pb;
	XMLD * psDoc=ehAllocZero(sizeof(XMLD));

	switch (iMode)
	{
		case XMLOPEN_FILE: // ptr è un nome di un file
		// Alloco in memoria il file
			strcpy(psDoc->szFile,pszData);
			psDoc->pszFile=fileMemoRead(pszData,NULL);
			if (!psDoc->pszFile) return NULL; // Il file non esiste o non è accessibile
			break;

		case XMLOPEN_PTR: // é il file XML
			psDoc->pszFile=strDup(pszData);
			break;

		default:ehError();
	}

	psDoc->iCurrentLine=1;
	psDoc->bUseNamespace=bUseNameSpace;
	pb=strstr(psDoc->pszFile,"<"); if (!pb) ehError(); // Controllino
	_xmlScan(psDoc,&pb,NULL);
	return psDoc;

}

//
// xmlClose() - Salvo il file (se richiesto) e libero le risorse impegnate
//
BOOL	xmlClose(XMLD * pxDoc,BOOL bSave) {

	if (bSave&&!strEmpty(pxDoc->szFile)) {
	
		CHAR * psz;
		EH_LST lst;
		EH_FILE * psFile;

		lst=xmlToLst(pxDoc->pxRoot,NULL);
		psFile=fileOpen(pxDoc->szFile,FO_WRITE|FO_CREATE_ALWAYS); 
		if (!psFile) ehExit("file open error: %s",pxDoc->szFile);
		if (!pxDoc->bNotHeaderXml) filePrint(psFile,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>" CRLF); // Header > da vedere
		for (lstLoop(lst,psz)) {
			filePrint(psFile,"%s" CRLF,psz);
		}
		fileClose(psFile);
		lstDestroy(lst);

	}

	if (pxDoc->pxRoot) xmlRemove(pxDoc,pxDoc->pxRoot,false);
	ehFreePtr(&pxDoc->pszFile);
	ehFree(pxDoc);
	return false;

}

//
// xmlGet()
//
DWORD xmlLength(XMLD * pxDoc, CHAR * pszFormat) {

	XMLPTR pxml,px;
	DWORD dwCont=0;
	pxml=xmlGet(pxDoc,"%s",pszFormat);
	if (pxml) {
	
		for (px=pxml;px;px=px->pxNext) {
			
			if (!strCmp(px->pName,pxml->pName)) dwCont++; else break;

		}
	}
	return dwCont;
}

static EH_AR _strParser(CHAR * psz) {

	CHAR * pszSep=".";	
	if (strstr(psz,">")) pszSep=">"; 
	return ARCreate(psz,pszSep,NULL);

}




//
// xmlGet()
//
XMLPTR xmlGet(XMLD * pxDoc, CHAR * pszFormat,...) {

	EH_AR	ar;
	CHAR *	pszStart=NULL,*pszEnd=NULL,*pszTag,*p;
	INT		a;
	CHAR *	pRet=NULL;
	INT		idx;
	BOOL	bTake;
//	CHAR *	pszSep;
	XMLPTR	px,pxFocus=NULL,pxStart=pxDoc->pxRoot;
	CHAR *	pszName;
	CHAR *	pszElement;
	INT		iCnt;

	//
	// Creo stringa
	//
	strFromArgs(pszFormat,pszElement);

	//
	// Estraggo items
	//
	ar=_strParser(pszElement);
	for (a=0;ar[a];a++) {
		bTake=FALSE;
		pszTag=ar[a];
		idx=0;

		p=strstr(pszTag,"[");
		if (p) {
			p=strExtract(pszTag,"[","]",FALSE,FALSE);
			idx=atoi(p);
			ehFree(p);
			p=strstr(pszTag,"["); *p=0;
			if (idx<0) break;
		}

		pszName=ar[a];
		pxFocus=NULL;
		for (px=pxStart,iCnt=0;px;px=px->pxNext) {
		
			if (!strCmp(px->pName,pszName)) 
			{
				if (iCnt==idx)  {pxFocus=px; break;}
				iCnt++;
			}

		}
		if (!pxFocus) {pRet=NULL; break;} else pxStart=pxFocus->pxFirstChild;
	}
	ARDestroy(ar);
	ehFree(pszElement);
	return pxFocus;

}

XMLPTR xmlLast(XMLPTR px) {

	XMLPTR pxLast; 
	for (;px;px=px->pxNext) {
		pxLast=px;
	}
	return pxLast;
}

//
// xmlAppendChild()
//
XMLPTR xmlAppendChild(XMLD * pxDoc, XMLPTR pxParent, CHAR * pszNew,...) {

	CHAR * pszElement;
	XMLPTR pxRet=NULL;
	strFromArgs(pszNew,pszElement);
	pxRet=xmlAdd(pxDoc, true, pxParent, ">%s",pszElement);

	ehFree(pszElement);
	return pxRet;

}

//
// xmlAdd() - Aggiunge un nodo
// Ritorna il puntatore al nodo aggiunto
//
XMLPTR xmlAdd(XMLD * pxDoc, BOOL bAddEver, XMLPTR pxParent, CHAR * pszNew,...) {

	XMLPTR pxb,px,pxStart=NULL,pxPoint,pxLast=NULL;
	XMLPTR pxRet=NULL;
	INT a,iLevel=0;
	CHAR * pszElement;
	EH_AR ar;

	strFromArgs(pszNew,pszElement);

	if (!pxParent) {
		pxStart=pxDoc->pxRoot; 
		pxPoint=pxStart;
	}
	else {
		
		pxStart=pxParent;
		pxPoint=pxStart;
	}

	if (*pszElement=='>') {
		
		pxStart=pxStart->pxFirstChild;
		pxPoint=pxStart;
		ar=_strParser(pszElement+1);
	}
	else {
		ar=_strParser(pszElement);
	}

	//
	// Array dei nodi da aggiungere
	//
	
	for (a=0;ar[a];a++) {

		px=ehAllocZero(sizeof(XMLE));
		px->psDoc=pxDoc;
		_nameSet(pxDoc,px,ar[a]);
		if (pxPoint) 
		{
			iLevel=pxPoint->iLevel;
			px->pxParent=pxPoint->pxParent;
		}
		else {
			px->iLevel=iLevel;
			px->pxParent=pxParent;
		}
		

		//
		// Controllo se item c'è già
		//
		pxLast=NULL; pxb=NULL;
		if (!bAddEver) {

			for (pxb=pxPoint;pxb;pxb=pxb->pxNext) {

				pxLast=pxb;
				if (!strcmp(pxb->pName,px->pName)) {
				
						pxParent=pxb;
						pxPoint=pxb->pxFirstChild;
						xmlElementFree(px);
						pxRet=pxParent;
						break;
				}
			}
		}

/*
		pxLast=NULL;
		for (pxb=pxPoint;pxb;pxb=pxb->pxNext) {

			pxLast=pxb;
			if (!strcmp(pxb->pName,px->pName)) {
			
				if (!bAddEver) {
					pxParent=pxb;
					pxPoint=pxb->pxFirstChild;
					xmlElementFree(px);
					pxRet=pxParent;
					break;
				}
				else {
				
					pxPoint=xmlLast(pxb);
					pxPoint->pxNext=px;
					pxPoint=px;
					pxRet=px;
					break;

				}
			}
		}
*/
		//
		// AppendChild() -> Aggiungo un figlio
		//
		if (!pxb) {

			if (px->pxParent) 
			{
				px->iLevel=px->pxParent->iLevel+1;
				if (px->pxParent->pxFirstChild&&!pxLast) 
					pxLast=xmlLast(px->pxParent->pxFirstChild);
			}


			// Ho altri figli = Accodo
			if (pxLast) {
				
				pxParent=pxLast->pxNext=px;
			}
			// Non ho altri figli = Inserisco il primo
			else {
				pxParent->pxFirstChild=px;
			}
			pxParent=px;
			pxPoint=NULL;
			pxRet=pxParent;
		}
	}
	ARDestroy(ar);
	ehFree(pszElement);
	return pxRet;
}

//
// xmlSet()
// Ritorna il puntatore al nodo aggiunto
//
XMLPTR xmlSet(XMLD * pxDoc, BOOL bAddEver, XMLPTR pxParent, CHAR * pszNew,...) {

	CHAR * pszElement;
	CHAR * psz;
	XMLPTR pxmlEle;
	CHAR * pszAttrib=NULL;
	strFromArgs(pszNew,pszElement);

	if (!pxParent) pxParent=pxDoc->pxRoot;

	psz=strstr(pszElement,"="); 
	if (psz) 
	{
		*psz=0;
		pszAttrib=strstr(psz+1,"\1"); if (pszAttrib) *pszAttrib=0;
	}
	pxmlEle=xmlAdd(pxDoc,bAddEver,pxParent,pszElement);
	if (pxmlEle&&psz) strAssign(&pxmlEle->pszValue,psz+1);

	if (pszAttrib) {

		EH_JSON * js;
		EH_JSON * psJson;
		INT a=0;

		pszAttrib++;
		psJson=js=jsonCreate(pszAttrib);
		
		do {

			if (psJson->enType!=JSON_ARRAY) {
				xmlSetAttrib(pxmlEle,psJson->pszName,"%s",psJson->utfValue);
			} 
			// Ho array e no child !!

		} while ((psJson=psJson->psNext));
		jsonDestroy(js);
		
	}
	ehFree(pszElement);
	return pxmlEle;

}


//
// xmlRemove()
// Ritorna TRUE se ci sono problemi
//
BOOL xmlRemove(XMLD * pxDoc, XMLPTR pxElement,BOOL bOnlyChildren) {

	XMLPTR pxb,pxRemove,pxLast;

	if (!pxElement) ehError();

	//
	// Sistemo i puntatori,  prossimo e firstChild
	//
	if (pxElement->pxParent&&!bOnlyChildren) {

		pxLast=NULL;
		for (pxb=pxElement->pxParent->pxFirstChild;pxb;pxb->pxNext) {
			if (pxb==pxElement) {
				if (!pxLast) 
					pxElement->pxParent->pxFirstChild=pxb->pxNext; //  Sono il primo figlio > faccio puntare come primo il al prossimo
					else
					pxLast->pxNext=pxb->pxNext; //  Il figlio precedent, punta al prossimo
				break;

			}
			if (pxLast==pxb) 
				break;
			pxLast=pxb;
		}
	}

	//
	// Ho figli > Loop ricorsivo e pulisco le corrispondenze
	//
	if (pxElement->pxFirstChild) {
		for (pxb=pxElement->pxFirstChild;pxb;) {
			pxRemove=pxb->pxNext; 
			xmlRemove(pxDoc,pxb,false);
			pxb=pxRemove;
		}
	}
		
	if (!bOnlyChildren) xmlElementFree(pxElement);
	return false;
}

//
// xmlToLst()
//
EH_LST xmlToLst(XMLPTR pxStart,EH_LST lst) {

	XMLPTR px;
	CHAR * pszIndent;
	static INT iLevel=0;
	CHAR * pszAtt;

	if (!lst) {lst=lstNew(); iLevel=-1;}
	iLevel++;
	pszIndent=strDup(strPad('\t',iLevel));

	for (px=pxStart;px;px=px->pxNext) {
	
		pszAtt=xmlGetAttribStr(px);
		if (px->pxFirstChild) {

			if (!strEmpty(pszAtt)) 
				lstPushf(lst,"%s<%s %s>",pszIndent,px->pName,pszAtt);
				else
				lstPushf(lst,"%s<%s>",pszIndent,px->pName);

			xmlToLst(px->pxFirstChild,lst);
			lstPushf(lst,"%s</%s>",pszIndent,px->pName);
		} 
		else {

			// Senza valore
			if (strEmpty(px->pszValue)) {

				if (!strEmpty(pszAtt)) 
					lstPushf(lst,"%s<%s %s />",pszIndent,px->pName,pszAtt);
					else
					lstPushf(lst,"%s<%s />",pszIndent,px->pName);
			}

			// Con Valore
			else {
		
				if (!strEmpty(pszAtt)) 
					lstPushf(lst,"%s<%s %s>%s</%s>",pszIndent,px->pName,pszAtt,px->pszValue,px->pName);
					else
					lstPushf(lst,"%s<%s>%s</%s>",pszIndent,px->pName,px->pszValue,px->pName);
			
			}

		}
		ehFreeNN(pszAtt);
	}
	ehFree(pszIndent);
	iLevel--;
	return lst;

}

//
// xmlPrint() - Visualizza un xml
//
void xmlPrint(XMLD * pxDoc,XMLPTR pxElement) {
	
	XMLPTR pxStart=pxDoc->pxRoot;
	CHAR * psz;
	EH_LST lst;

	if (pxElement) pxStart=pxElement;
	lst=xmlToLst(pxStart,NULL);
	for (lstLoop(lst,psz)) {
		
		printf("%s" CRLF,psz);
	}
	lstDestroy(lst);
}


CHAR * _tagCloseSearch(BYTE *lpStart)
{
	INT iCount=1;
	for (;*lpStart;lpStart++)
	{
		if (*lpStart=='>') iCount--;
		if (*lpStart=='<') iCount++;
		if (!iCount) return lpStart;
	}
	return NULL;
}

static CHAR * _endTag(CHAR *lp)
{
	for (;*lp;lp++)
	{
		if (*lp=='>') return (lp);
	}
	return lp-1;
}

static CHAR * _getEntity(XMLD * psDoc,CHAR **lppSource,INT *lpiSize,EN_XMLTAG_IS *lpiType,BYTE **lpSourcePoint)
{
	BYTE *lps=*lppSource;
	CHAR *lpEntityStart=NULL;
	CHAR *lpEntityEnd=NULL;
	CHAR *lpValueStart=NULL;
	CHAR *lpValueEnd=NULL;
	BOOL fCdata=FALSE;
	BOOL bString=FALSE;
	CHAR cOpen;
	INT iType=IS_A_ERROR;

	*lpiType=iType;
	*lpSourcePoint=0;

	// Tolgo le cose che non sono caratteri validi davanti
	for (;*lps;lps++)
	{
		if (*lps>32) break;
		if (*lps==10) psDoc->iCurrentLine++;
	}
	for (;*lps;lps++)
	{
		if (*lps==10) psDoc->iCurrentLine++;
		if (*lps<32) continue;

	//	printf("[%c:%d]",*lps,*lps);

		// Inizio Tag ?
		if (*lps=='<'&&!fCdata)
		{
			// Fine di un valore
			if (lpValueStart) {lps--; lpValueEnd=lps; break;}

			if (*(lps+1)=='!')
			{
				// E' un CDATA
				if (!memcmp(lps,STR_CDATA_START,8)&&!fCdata) {fCdata=TRUE; iType=IS_A_CDATA; lpValueStart=lps; continue;}

				//
				// E' un commento ?
				//
				if (!strBegin(lps,"<!--"))
				{
					lpEntityStart=lps;
					// Cerco la fine
					lpEntityEnd=strstr(lps,"-->");
					if (lpEntityEnd)
					{
						lpEntityEnd+=2;
						_lineCountUpdate(psDoc,lpEntityStart,lpEntityEnd);
						lps=lpEntityEnd; iType=IS_A_COMMENT;
					}
					break;
				}

				//
				// E' un DOCTYPE
				//
				if (!strBegin(lps,"<!DOCTYPE "))
				{
					// Cerco la fine
					lpEntityStart=lps; lpEntityEnd=_tagCloseSearch(lpEntityStart+1);
					if (lpEntityEnd)
					{
						_lineCountUpdate(psDoc,lpEntityStart,lpEntityEnd);
						lps=lpEntityEnd;
						iType=IS_A_DOCTYPE;
					}
					break;
				}

/*
			//
			// E' un ENTITY ?
			//
			if (!strBegin(lps,"<!ENTITY "))
			{
				lpEntityStart=lps;
				// Cerco la fine
				lpEntityEnd=strstr(lps,">"); if (lpEntityEnd) {lps=lpEntityEnd; iType=IS_A_ENTITY;}
				break;
			}
*/

			}

			// E' un' inizio di TAG
			lpEntityStart=lps; iType=IS_A_TAG;
			//iType=IS_A_VALUE;
		}

		if (fCdata)
		{
			if (!memcmp(lps,"]]>",3))
			{
				lps+=2; lpValueEnd=lps; break;
			}
		}

		// Controllo stringhe (new 2009)
		if (iType==IS_A_TAG)
		{
			if (*lps=='\"'||*lps=='\'')
			{
				// La stringa  è aperta e devo chiuderla
				if (!bString)
					{bString=TRUE; cOpen=*lps;}
					else if (*lps==cOpen) {bString=FALSE; cOpen=0;}
			}
		}


		// Fine del TAG
		if (!bString&&*lps=='>'&&!fCdata)
		{
			if (!lpEntityStart&&!lpValueStart) break;
			if (lpEntityStart) {lpEntityEnd=lps; break;}
		}

		// Inizio di un valore
		if (!lpValueStart&&!lpEntityStart) {lpValueStart=lps; iType=IS_A_VALUE;}
	}

	//printf("Esco"); getch();
	if (lpEntityStart&&lpEntityEnd)
	{
		INT iSize=((INT) lpEntityEnd-(INT) (lpEntityStart))+1;
		CHAR *lpEntity=ehAlloc(iSize+2);
		memcpy(lpEntity,lpEntityStart,iSize); lpEntity[iSize]=0;
		*lppSource=lps+1;
		*lpiSize=iSize;
		*lpiType=iType;//IS_A_TAG;
		*lpSourcePoint=lpEntityStart;
		return lpEntity;
	}

	if (lpValueStart&&lpValueEnd)
	{
		INT iSize=((INT) lpValueEnd-(INT) (lpValueStart))+1;
		CHAR *lpEntity=ehAlloc(iSize+2);
		memcpy(lpEntity,lpValueStart,iSize); lpEntity[iSize]=0;
		*lppSource=lps+1;
		*lpiSize=iSize;
		*lpSourcePoint=lpValueStart;
		//if (fCdata) *lpiType=IS_A_CDATA; else *lpiType=IS_A_VALUE;
		*lpiType=iType;
		/*
		if (iSize<255)  win_infoarg("CDATA[%d][%s]",iSize,lps+1);
						else
						{lpEntity[20]=0;
						 win_infoarg("CDATA[%d][%s...%s]",iSize,lpEntity,lpEntity+iSize-20);
						}
		*/
		return lpEntity;
	}

	if (!lpValueStart&&
		!lpValueEnd&&
		!lpEntityStart&&
		!lpEntityEnd) *lpiType=IS_A_END;
	return NULL;
}


static EH_LST _xmlAttribParser(CHAR * pszAtt) {

	EH_LST lst=lstCreate(sizeof(XML_ATTRIB));
	XML_ATTRIB sAtt;
	INT		iMode=0;
	BYTE *pStart=pszAtt;
	BYTE *ps=NULL;
	BYTE *pszAttrib=NULL,*pszValue=NULL;//,*pszValueAdd=NULL;
	BOOL bFull=FALSE;
	BYTE cComma;
	for (;*pszAtt;pszAtt++)
	{
		switch (iMode)
		{
			case 0: // Ricerca nome
				if (*pszAtt<' ') continue;

				if (strchr("\'\"",*pszAtt))
					ehError(); // Caratteri non validi Errore nel xml
				if (!ps) ps=pszAtt;
				if (*pszAtt=='=')
				{
					pszAttrib=strTake(ps,pszAtt-1); strTrim(pszAttrib); // Trovato attriobuto
					iMode=1; ps=NULL;
					continue;
				}
//				else
//					if (*pszAtt==' ')
//						ehError(); // attributo senza uguale es <tag nowrap> non valido in XML

				break;

			case 1: // Ricerca inizio valore
				if (*pszAtt<' ') continue;

				if (!strchr("\'\"",*pszAtt)) ehError(); // Caratteri non validi Errore nel xml
				cComma=*pszAtt;
				ps=pszAtt+1;
				iMode=2;
				break;

			case 2: // Ricerca fine valore
				if (*pszAtt!=cComma) continue;

				pszValue=strTake(ps,pszAtt-1); if (!pszValue) pszValue=strDup("");
//				pszValueAdd=ehAlloc(strlen(pszAttrib)+strlen(pszValue)+10);
				_(sAtt);
				sAtt.pszName=pszAttrib;
				sAtt.pszValue=pszValue;
				lstPush(lst,&sAtt);
				ps=NULL;
				cComma=0;
				iMode=0; bFull=TRUE;
				break;
		}
	}
	//if (pszAttrib||pszValue) 
	//		ehError();
	if (!bFull)
	{
		lst=lstDestroy(lst);
	}

	return lst;
}

//
// _nameSet()
//
static void _nameSet(XMLD * psDoc,XMLPTR px,CHAR * pszName) {

	px->pNameOriginal=strDup(pszName); 

	//
	// Estraggo NameSpace
	//
	if (psDoc->bUseNamespace)
	{
		CHAR *p;
		p=strstr(px->pNameOriginal,":");
		if (p){
			*p=0;
			px->pNamespace=strDup(px->pNameOriginal); // <--
			px->pName=strDup(p+1); // <--
			*p=':';
		}
		else { 
			px->pName=px->pNameOriginal;
		}
	}
	else
	{
		px->pName=px->pNameOriginal;
	}

}

//
// _xmlScan() Funzione ricorsiva
//
static void _xmlScan(XMLD * psDoc,CHAR **lpSource,XMLPTR pxParent)
{
	BYTE *	lpEntity,*pb;
	CHAR *	lpName;
	CHAR *	pszAtt=NULL;

	INT		iCount=0;
	INT		iSize;
	BOOL	fOpen,fClose;
	XMLPTR	px=NULL;
	XMLPTR	pxLast=NULL;
	EN_XMLTAG_IS enType;
	static	INT iLevel=0;
	BYTE *	lpSourcePoint;

	while (true)
	{
		if (!**lpSource) break;
		lpEntity=_getEntity(psDoc,lpSource,&iSize,&enType,&lpSourcePoint);
		if (enType==IS_A_ERROR) {win_infoarg("Errore in XML"); ehFree(lpEntity); break;}
		if (enType==IS_A_END) {ehFreeNN(lpEntity); break;}

		switch (enType)
		{
			case IS_A_TAG: // E' un TAG (Elemento)

				// Nome dell'elemento
				if (!_memicmp(lpEntity,"<?xml",5)) {ehFree(lpEntity); continue;}		// Versione

				// Estraggo Nome elemento
				if (lpEntity[iSize-2]=='/') {fOpen=FALSE; lpEntity[iSize-2]=0;} else fOpen=TRUE;
				lpName=lpEntity+1; lpEntity[iSize-1]=0;
				if (*lpName=='/') {fClose=TRUE; fOpen=FALSE; lpName++;} else fClose=FALSE;
				for (pb=lpName;*pb;pb++) {if (*pb<'!') break;}
				
				if (pb) {*pb=0; pszAtt=pb+1;} else pszAtt=NULL;
				

				if (fClose)
				{
					if (pxParent)
					{
						//DMIRead(&psDoc->dmiElement,iParent,&xmlElement);
						pxParent->pszSourceEnd=_endTag(lpSourcePoint);
						//px->lpSourceEnd=_endTag(lpSourcePoint);
						if (px) pxParent->pszValueStart=_endTag(px->pszSourceStart)+1;
						pxParent->pszValueEnd=lpSourcePoint-1;
						//DMIWrite(&psDoc->dmiElement,iParent,&xmlElement);
					}
					// if (psXmlElementParent) psXmlElementParent->lpSourceEnd=_endTag(lpSourcePoint);
					ehFree(lpEntity); 
					return;
				}

				//
				// Inserisco il Nuovo elemento
				//
				px=ehAllocZero(sizeof(XMLE));
				px->psDoc=psDoc;
				_nameSet(psDoc,px,lpName);
				px->iLevel=iLevel;
				px->pxParent=pxParent;
				px->pszSourceStart=lpSourcePoint;
				px->iSourceLine=psDoc->iCurrentLine;
				if (!strEmpty(pszAtt)) px->lstAttrib=_xmlAttribParser(pszAtt);

				if (!pxParent) psDoc->pxRoot=px; else {if (!pxParent->pxFirstChild) pxParent->pxFirstChild=px;}
				if (pxLast) pxLast->pxNext=px;
				if (fOpen)
				{
					iLevel++;
					
					_xmlScan(psDoc,lpSource,px);
					iLevel--;
				}
				ehFree(lpEntity);
				pxLast=px;
 				break;

			case IS_A_VALUE:
				if (!psDoc->pxRoot) ehExit("xmlError:[%s]",lpEntity);
				//DMIRead(&psDoc->dmiElement,psDoc->dmiElement.Num-1,&xmlElement);
				pxParent->bCdata=false;
				pxParent->pszValue=strDup(lpEntity);
				//DMIWrite(&psDoc->dmiElement,psDoc->dmiElement.Num-1,&xmlElement);
				ehFree(lpEntity);
				break;

			case IS_A_CDATA:

//				DMIRead(&psDoc->dmiElement,psDoc->dmiElement.Num-1,&xmlElement);
				pxParent->pszValue=strDup(lpEntity);
				pxParent->bCdata=true;
				strReplace(pxParent->pszValue,STR_CDATA_START,"");
				strReplace(pxParent->pszValue,STR_CDATA_END,"");
//				DMIWrite(&psDoc->dmiElement,psDoc->dmiElement.Num-1,&xmlElement);
				ehFree(lpEntity);
				break;

			case IS_A_COMMENT: // <!-- -->
			//	printf("Commento: %s",lpEntity);
				ehFree(lpEntity);
				break;

			case IS_A_ENTITY: // <!ENTITY >
			//	printf("Commento: %s",lpEntity);
				ehFree(lpEntity);
				break;

			default:
				ehFree(lpEntity);
				break;

		}
	}
	return;
}

#ifdef EH_MEMO_DEBUG
CHAR * _xmlGetAttribAllocEx(XMLPTR px,CHAR *lpName,CHAR *pDefault,CHAR *pProgram,INT iLine)
{
	CHAR *lp,*lpBuf=NULL;
	lp=xmlGetAttrib(px,lpName);
	if (lp) lpBuf=_strDupEx(lp,pProgram,iLine);
	if (!lpBuf&&pDefault) lpBuf=_strDupEx(pDefault,pProgram,iLine);
	return lpBuf;
}

#else
CHAR * xmlGetAttribAlloc(XMLPTR px,CHAR *lpName,CHAR *pDefault)
{
	CHAR *lp,*lpBuf=NULL;
	lp=xmlGetAttrib(px,lpName);
	if (lp) lpBuf=strDup(lp);
	if (!lpBuf&&pDefault) lpBuf=strDup(pDefault);
	return lpBuf;
}
#endif



//
// xmlGetAttrib()
//
CHAR * xmlGetAttrib(XMLPTR px,CHAR * pszName)
{
	XML_ATTRIB * psAtt;
	if (!px->lstAttrib) return NULL;

	for (lstLoop(px->lstAttrib,psAtt)) {
		if (!strcmp(psAtt->pszName,pszName)) return psAtt->pszValue;
	}
	return NULL;
}


//
//  xmlSetAttrib()
//
BOOL	xmlSetAttrib(XMLPTR px,CHAR * pszAttrib,CHAR *pszFormat,...) {

	XML_ATTRIB * psAtt=NULL;
	CHAR * pszValue;

	if (!px) return true;
	if (!px->lstAttrib) px->lstAttrib=lstCreate(sizeof(XML_ATTRIB));

	strFromArgs(pszFormat,pszValue);


	for (lstLoop(px->lstAttrib,psAtt)) {
		if (!strcmp(psAtt->pszName,pszAttrib)) {
		
			if (!pszValue) {
				lstRemoveI(px->lstAttrib,px->lstAttrib->psCurrent);
				return false;
			}
			else {
				strAssign(&psAtt->pszValue,pszValue);
				return false;
			}
		}
	}
	if (!psAtt) {
		XML_ATTRIB sAtt;
		_(sAtt);
		sAtt.pszName=strDup(pszAttrib);
		sAtt.pszValue=strDup(pszValue);
		lstPush(px->lstAttrib,&sAtt);
	}
	ehFree(pszValue);
	return false;
}


//
// xmlGetAttribStr() legge una stringa con tutti gli attributi (da liberare con ehFree();
//
CHAR *	xmlGetAttribStr(XMLPTR px)
{
	XML_ATTRIB * psAtt;
	EH_LST lst=NULL;
	CHAR * psz;

	if (!px->lstAttrib) return NULL;
	lst=lstNew();
	for (lstLoop(px->lstAttrib,psAtt)) {
		lstPushf(lst,"%s=\"%s\"",psAtt->pszName,psAtt->pszValue);
	}
	psz=lstToString(lst," ","",""); lstDestroy(lst);
	return psz;
}



//
// xmlTagFileWrite)
//
void xmlTagFileWrite(FILE *pf1,BYTE *pTag,BYTE *pValue,BYTE *pLineStart,BYTE *pLineEnd)
{
	if (pLineStart) fwrite(pLineStart,strlen(pLineStart),1,pf1);
	if (!pValue) pValue="";
	if (*pValue)
	{
		fprintf(pf1,"<%s>%s</%s>",pTag,pValue,pTag);
	} else fprintf(pf1,"<%s/>",pTag);
	if (pLineEnd) fwrite(pLineEnd,strlen(pLineEnd),1,pf1);
}
//
// xmlArrayLen()
//
INT xmlArrayLen(XMLARRAY ar)
{
	INT a;
	for (a=0;ar[a];a++)	{}	return a;
}

static void _lineCountUpdate(XMLD * psDoc,BYTE *pStart,BYTE *pEnd)
{
	BYTE *p;
	for (p=pStart;p<pEnd;p++) {if (*p==10) psDoc->iCurrentLine++;}
}



XMLD * xmlDoc(XMLD *psDoc, XMLE *psEle) {
	if (psDoc) return psDoc;
	return psEle->psDoc;
}

XMLPTR xmlElementClone(XMLPTR psXmlOriginal)
{
	XMLE * psXmlReturn=NULL;
	psXmlReturn=ehAlloc(sizeof(XMLE));
	memcpy(psXmlReturn,psXmlOriginal,sizeof(XMLE));
	if (psXmlReturn->pNamespace) psXmlReturn->pNamespace=strDup(psXmlOriginal->pNamespace);
	if (psXmlReturn->pName) psXmlReturn->pName=strDup(psXmlOriginal->pName);
	if (psXmlReturn->pNameOriginal) psXmlReturn->pNameOriginal=strDup(psXmlOriginal->pNameOriginal);
//	if (psXmlReturn->lpAttrib) psXmlReturn->lpAttrib=strDup(psXmlOriginal->lpAttrib);
	ehError(); // Da fare la duplicazione della lista degli attibuti
//	if (psXmlReturn->lszAttrib) psXmlReturn->arAttrib=lstDup(psXmlOriginal->lstAttrib);
	if (psXmlReturn->pszValue) psXmlReturn->pszValue=strDup(psXmlOriginal->pszValue);
	return psXmlReturn;
}

static void _attDestroy(EH_LST lstAttrib) {

	XML_ATTRIB * psAtt;
	for (lstLoop(lstAttrib,psAtt)) {
		ehFreePtrs(2,&psAtt->pszName,&psAtt->pszValue);
	}
	lstDestroy(lstAttrib);

}

void * xmlElementFree(XMLE * psXmlOriginal)
{
	if (!psXmlOriginal) return NULL;
	if (psXmlOriginal->pName&&psXmlOriginal->pNameOriginal!=psXmlOriginal->pName)
  		ehFree(psXmlOriginal->pName);
	ehFreeNN(psXmlOriginal->pNameOriginal);
	ehFreeNN(psXmlOriginal->pNamespace);
	if (psXmlOriginal->lstAttrib) _attDestroy(psXmlOriginal->lstAttrib);
	ehFreeNN(psXmlOriginal->pszValue);
	ehFree(psXmlOriginal); //else memset(psXmlOriginal,0,sizeof(XMLE));
	return NULL;
}

XMLE * xmlElementWithAttribute(XMLARRAY arXml,CHAR *pAttributeName,CHAR *pAttributeValue,BOOL bElementAlloc)
{
	INT a;
	XMLE * psXmlReturn=NULL;
	BOOL bWithNS,bFound;
	CHAR szAttribNS[300];

	if (!arXml) return NULL;
	if (strstr(pAttributeName,":")) bWithNS=TRUE; else bWithNS=FALSE;
	if (!bWithNS) sprintf(szAttribNS,":%s",pAttributeValue);

	for (a=0;arXml[a];a++)
	{
		BYTE *pNameAttrib=xmlGetAttribAlloc(arXml[a],pAttributeName,NULL);
		bFound=FALSE;
		if (!pNameAttrib) continue;

		if (!strcmp(pNameAttrib,pAttributeValue)) bFound=TRUE;
		if (!bFound&&!bWithNS)
		{
			if (strstr(pNameAttrib,szAttribNS)) bFound=TRUE;
		}
		if (bFound)
		{
			if (bElementAlloc) psXmlReturn=xmlElementClone(arXml[a]); else psXmlReturn=arXml[a];
		}

		ehFree(pNameAttrib);
		if (psXmlReturn) break;
	}
	return psXmlReturn;
}


//
// XMLIdBuilder() - 2010
//
/*
INT xmlIdBuilder(XMLD *px)
{
	INT a,iCnt=0;
	CHAR *psz;
	if (px->arId) ehFree(px->arId);
	px->arId=ehAllocZero(px->dmiElement.Num*sizeof(INT));
	for (a=0;a<px->dmiElement.Num;a++)
	{
		if (!px->arElement[a].arAttrib) continue;
		psz=xmlGetAttrib(&px->arElement[a],"id");
		if (psz)
		{
			px->arId[iCnt]=a; iCnt++;
		}
	}
	px->arId[iCnt]=-1;
	return iCnt;
}
//
// XMLIdSearch() - 2010
//
XMLE * xmlIdSearch(XMLD *px,CHAR *psIdValue)
{
	INT a,iCnt=0;
	XMLE *xmlRet;
	if (!px->arId) ehError();
	for (a=0;px->arId[a]>-1;a++)	{
		xmlRet=&px->arElement[px->arId[a]];
		if (!strcmp(xmlGetAttrib(xmlRet,"id"),psIdValue)) return xmlRet;
	}
	return NULL;
}
*/

//
// Ritorna l'indirizzo URI del name space indicato
//
	/*
CHAR * XMLNameSpaceLocation(XMLD *px,CHAR *pNamespace,INT idx)
{
	XMLE *pElement;
	BYTE *pBuf,*pNS,*pStart,*pEnd;
	pElement=&px->arElement[idx];
	for (pStart=pElement->lpAttrib;;)
	{
		pStart=strstr(pStart," xmlns:"); if (!pStart) break;
		pStart+=7; pEnd=strstr(pStart,"=");
		pBuf=strTake(pStart,pEnd-1); pNS=strTrim(pBuf); ehFree(pBuf);
		if (!strcmp(pNS,pNamespace)) ehExit("");
	}
	ehExit("da vedere");
	return NULL;
}
	*/

CHAR * xmlElementToString(XMLD * xmlDoc,XMLE *psXmlElement,BOOL bOnlyValue) {

	CHAR *pString=NULL;
	if (!bOnlyValue)
		pString=strTake(psXmlElement->pszSourceStart,psXmlElement->pszSourceEnd);
		else
		pString=strTake(psXmlElement->pszValueStart,psXmlElement->pszValueEnd);

	return pString;
}

XMLNAMESPACE * xmlNSExtract(CHAR *pszInputName,XMLNAMESPACE *psNamespace)
{
	S_XNS *psNS;
	memset(psNamespace,0,sizeof(XMLNAMESPACE));
	psNS=xnsCreate(pszInputName);
	if (psNS->pszSpace) strcpy(psNamespace->szNameSpace,psNS->pszSpace);
	if (psNS->pszElement) strcpy(psNamespace->szName,psNS->pszElement);
	xnsDestroy(psNS);
	return psNamespace;
}

#ifdef EH_MEMO_DEBUG
XNS_PTR _xnsCreate(CHAR *pszInputName,CHAR *pFile,INT iLine)
#else
XNS_PTR xnsCreate(CHAR *pszInputName)
#endif
{
	BYTE *p;
	XNS_PTR psNS;
	if (!pszInputName) return NULL;

	psNS=ehAllocZero(sizeof(S_XNS));
	p=strstr(pszInputName,":");
	if (p)
	{
		INT l=(p-pszInputName);
		psNS->pszSpace=strTake(pszInputName,p-1);
		psNS->pszElement=strDup(p+1);
	}
	else
	{
		psNS->pszElement=strDup(pszInputName);
	}

#ifdef EH_MEMO_DEBUG
	memDebugUpdate(psNS,pFile,iLine);
#endif
	return psNS;
}

#ifdef EH_MEMO_DEBUG
XNS_PTR _xnsDup(XNS_PTR xns,CHAR *pFile,INT iLine)
{
	XNS_PTR xnsNew;
	if (!xns) return xns;
	xnsNew=_ehAlloc(sizeof(S_XNS),TRUE,pFile,iLine);//ehAllocZero(sizeof(S_XNS));
	xnsNew->pszElement=_strDupEx(xns->pszElement,pFile,iLine);
	xnsNew->pszSpace=_strDupEx(xns->pszSpace,pFile,iLine);
	return xnsNew;
}

#else
XNS_PTR xnsDup(XNS_PTR xns)
{
	XNS_PTR xnsNew;
	if (!xns) return xns;
	xnsNew=ehAllocZero(sizeof(S_XNS));
	xnsNew->pszElement=strDup(xns->pszElement);
	xnsNew->pszSpace=strDup(xns->pszSpace);
	return xnsNew;
}
#endif

XNS_PTR xnsDestroy(XNS_PTR psNST)
{
	if (!psNST) return NULL;
	ehFreeNN(psNST->pszSpace);
	ehFreeNN(psNST->pszElement);
	ehFree(psNST);
	return NULL;
}

CHAR * xnsString(XNS_PTR psNST)
{
	DWORD dwSize;
	BYTE *pszString;
	if (!psNST) ehError();
	if (strEmpty(psNST->pszElement)) ehError();
	if (!strEmpty(psNST->pszSpace))
	{
		dwSize=strlen(psNST->pszSpace)+strlen(psNST->pszElement)+2;
		pszString=ehAlloc(dwSize);
		sprintf(pszString,"%s:%s",psNST->pszSpace,psNST->pszElement);
	}
	else
		pszString=strDup(psNST->pszElement);
	return pszString;
}





//
// xmlQGet()
//
CHAR * xmlQGet(CHAR * pszXml,CHAR * pszFormat,...) {

	CHAR szTagOpen[100];
	CHAR szTagOpenParam[100];
	CHAR szTagClose[100];
	EH_AR ar;
	CHAR *pszStart=NULL,*pszEnd=NULL,*pszTag,*p;
	CHAR *pszSectionStart,*pszSectionEnd;
	INT a,b;
	CHAR *pRet=NULL;
	INT idx;
	BOOL bTake;
	CHAR * pszSep;

	//
	// Estraggo Item
	//

	va_list Ah;
	CHAR * pszElement=NULL;
	DWORD dwSize=2048;
	INT iRet;

	va_start(Ah,pszFormat);
	while (TRUE)
	{
		pszElement=ehAlloc(dwSize);
		iRet=vsnprintf(pszElement,dwSize-1,pszFormat,Ah); // Messaggio finale
		if (iRet>-1) break;
		dwSize+=2048;
		if (dwSize>1000000) ehError();
		ehFree(pszElement);
	}



	pszStart=pszXml;
	pszEnd=pszXml+strlen(pszXml);
	pszSep=".";
	if (strstr(pszElement,">")) pszSep=">"; 
	ar=ARCreate(pszElement,pszSep,NULL);
	a=ARLen(ar);

	//
	// Richiesta di conteggio degli elementi
	//
	if (a>1) {
		if (!strcmp(ar[a-1],"length")) {
			CHAR szServ[300],*pszLoop;
			strCpy(szServ,pszElement,sizeof(szServ)-1);
			pszLoop=strstr(szServ,".length");
			for (a=0;;a++) {
				sprintf(pszLoop,"[%d]",a);
				pRet=xmlQGet(pszXml,szServ);
//				if (pRet) {BYTE * psz=strOmitAlloc(pRet,"\r\n"); strAssign(&pRet,psz); ehFree(psz);}
				if (!pRet) break; else ehFree(pRet);
			}
			if (pRet) ehFree(pRet);
			pRet=ehAlloc(100);
			sprintf(pRet,"%d",a);
			goto FINE;
			//ARDestroy(ar);
			//return pRet;
		}
	}

	for (a=0;ar[a];a++) {
		bTake=FALSE;
		pszTag=ar[a];
		idx=0;

		p=strstr(pszTag,"[");
		if (p) {
			p=strExtract(pszTag,"[","]",FALSE,FALSE);
			idx=atoi(p);
			ehFree(p);
			p=strstr(pszTag,"["); *p=0;
		}
		if (idx<0) break;
		sprintf(szTagOpen,"<%s>",ar[a]);
		sprintf(szTagOpenParam,"<%s ",ar[a]);
		sprintf(szTagClose,"</%s>",ar[a]);
		for (b=0;b<=idx;b++) {

			// Trovo l'inizio
			pszSectionStart=strstr(pszStart,szTagOpen);
			if (!pszSectionStart) pszSectionStart=strstr(pszStart,szTagOpenParam);
			if (!pszSectionStart) break;

			pszSectionEnd=strstr(pszStart,szTagClose); if (!pszSectionEnd) break;
			if (pszSectionStart>=pszEnd||pszSectionEnd>=pszEnd) break;
			pszStart=pszSectionEnd+strlen(szTagClose);
		}

		if (!pszSectionStart||!pszSectionEnd) break;
		if (pszSectionStart>=pszEnd||pszSectionEnd>=pszEnd) break;
		pszStart=pszSectionStart+strlen(szTagOpen);
		pszEnd=pszSectionEnd-1;
		bTake=TRUE;
	}

	if (bTake) pRet=strTake(pszStart,pszEnd); else ehFreePtr(&pRet);

FINE:
	ARDestroy(ar);
	ehFree(pszElement);
	va_end(Ah);

	return pRet;

}

//
// xmlArray()
//
XMLARRAY xmlArrayAlloc(XMLD * pxDoc,PXML pxEle,BOOL bChildren) {

	PXML px;
	XMLARRAY arx;
	INT iCnt;
	if (bChildren&&pxEle) pxEle=pxEle->pxFirstChild;

	for (iCnt=0,px=pxEle;px;px=px->pxNext,iCnt++); 
	iCnt++;
	arx=ehAllocZero(sizeof(PXML)*iCnt);
	for (iCnt=0,px=pxEle;px;px=px->pxNext,iCnt++) {
		arx[iCnt]=px;
	}
	return arx;

}
//
// xmlArrayFree()
//
XMLARRAY xmlArrayFree(XMLARRAY ar)
{
	if (ar) ehFree(ar);
	return NULL;
}