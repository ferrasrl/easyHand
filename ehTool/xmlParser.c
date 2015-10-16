//   ---------------------------------------------
//   | xmlParser  Utilità per il controllo
//   |          di documenti XML
//   |
//   |             by Ferrà Art & Technology 2001
//   |             Giugno 2001
//   |
//   |
//   | DOC = Documento
//   |  Composto da Element (Elementi)
//   |  Ogni elemento ha degli attributi dinamici
//   |  Ogni elemento puà avere un valore dinamico
//   |  Ogni elemento può avere figli
//   |  Versione 3.0	Migliorato parser degli attributi (capisci a me...)
//   ---------------------------------------------
//
#include "/easyhand/inc/easyhand.h"
#include "/easyhand/inc/xmlParser.h"

// #include "/easyhand/ehtool/main/armaker.h"

static void _xmlScan(XMLDOC *psDoc,CHAR **lpSource,INT iParent);
static CHAR * _getEntity(CHAR **lppSource,INT *lpiSize,EN_XMLTAG_IS *lpiType,BYTE **lpSourcePoint);
static INT _lineNumberSearch(BYTE *pSource,CHAR *pLimit);
static void _lineCountUpdate(BYTE *pStart,BYTE *pEnd);

static INT iCurrentLine=0;

//
// xmlParser()
//
void * xmlParser(XMLDOC * psDoc,INT cmd,INT info,void *ptr)
{
	CHAR *lp;
	BYTE *pb;
	CHAR *lpMemo,*lpElem;
	INT a;
	XMLELEMENT xmlElement,*pXml;
	XMLARRAY arsXml;

	INT iLevel;
	INT		iStep;
	INT iLevelStart;
	INT idx,iIndex;
	BYTE *lpSearch;
//	S_ARMAKER sArMaker;
	BOOL bAllChildren;
	INT iElement,idxStart;
	CHAR * psz;

	switch (cmd)
	{
		case WS_OPEN: // Apro un documento XML

			if (psDoc->hdlDOM) xmlParser(psDoc,WS_CLOSE,0,NULL);
			switch (info&1)
			{
				case XMLOPEN_FILE: // ptr è un nome di un file
					// Alloco in memoria il file
					psDoc->hdlDOM=fileLoad(ptr,RAM_AUTO);
					if (psDoc->hdlDOM<0) ehExit("Errore in lettura %s = %d",ptr,psDoc->hdlDOM);
					strcpy(psDoc->szFile,ptr);
					psDoc->lpDOM=memoLock(psDoc->hdlDOM);
					break;

				case XMLOPEN_PTR: // é il file XML
					psDoc->hdlDOM=memoAlloc(RAM_AUTO,strlen(ptr)+1,"XML");
					strcpy(psDoc->szFile,"");
					psDoc->lpDOM=memoLock(psDoc->hdlDOM);
					strcpy(psDoc->lpDOM,ptr);
					break;
			}

			// Parser del XML per creare l'array degli Elementi
			DMIReset(&psDoc->dmiElement);
			DMIOpen(&psDoc->dmiElement,RAM_AUTO,100+info,sizeof(XMLELEMENT),"xmlElement");

			pb=psDoc->lpDOM; iCurrentLine=1;
			pb=strstr(pb,"<"); if (!pb) ehError();
			_xmlScan(psDoc,&pb,-1);
			psDoc->arElement=DMILock(&psDoc->dmiElement,NULL);//memoLock(psDoc->dmiElement.Hdl); // Chiudo dopo la scansione
			break;

		case WS_CLOSE: // Chiudo il gestore dei documenti XML
			for (a=0;a<psDoc->dmiElement.Num;a++)
			{
				xmlElementFree(&psDoc->arElement[a]);
			}

			if (psDoc->arId) ehFree(psDoc->arId);
			ehFreePtrs(3,&psDoc->pszXml,&psDoc->pszVersion,&psDoc->pszEncoding);
			DMIClose(&psDoc->dmiElement,"xmlElement"); psDoc->arElement=NULL;
			memoFree(psDoc->hdlDOM,"xmlParser"); psDoc->hdlDOM=0;

			break;

		case WS_ADD:
			break;

		case WS_FIND:
			idx=(INT) xmlParser(psDoc,WS_FINDKEY,info,ptr); if (idx<0) return NULL;
			return &psDoc->arElement[idx];

		case WS_FINDKEY: // Ritorna l'indice dell'elemento nell'array in memoria

			// Ricerca degli elementi
			lpMemo=lpElem=strDup(ptr);
			if (!lpMemo) 
				ehError();
			if (info>0)
			{
				DMIRead(&psDoc->dmiElement,info,&xmlElement);
				a=info; iLevel=xmlElement.iLevel;
			}
			else
			{
				a=0;
				iLevel=0;
			}

			//
			// Controllo se "Figlio di"
			//
			if (*lpElem=='.')
			{
				a++;
				iLevel++;
				lpElem++;
			}

			//
			// Cerco il padre dell'elemento
			//
			if (*lpElem=='^')
			{
				iLevel--;
				iIndex=-1;
				a--;
				for (;a>-1;a--)
				{
					if (psDoc->arElement[a].iLevel==iLevel) {iIndex=a; break;}
					if (psDoc->arElement[a].iLevel<iLevel) break;
				}
				ehFree(lpMemo);
				return (INT *) iIndex;
			}


			iLevelStart=iLevel;

			//
			// Ricerca del primo figlio "."
			//
			if (!*lpElem) // Voglio il primo figlio
			{
				iIndex=-1;
				for (;a<psDoc->dmiElement.Num;a++)
				{
					if (psDoc->arElement[a].iLevel==iLevel) {iIndex=a; break;}
					if (psDoc->arElement[a].iLevel<iLevelStart) break;
				}
				ehFree(lpMemo);
				return (INT *) iIndex;
			}

			//
			// Ricerca del prossimo (figlio stesso livello)
			//
			if (*lpElem=='>')
			{
				iIndex=-1; a++;
				for (;a<psDoc->dmiElement.Num;a++)
				{
					if (psDoc->arElement[a].iLevel==iLevel) {iIndex=a; break;}
					if (psDoc->arElement[a].iLevel<iLevelStart) break;
				}
				ehFree(lpMemo);
				return (INT *) iIndex;
			}

			while (TRUE)
			{
				lp=strstr(lpElem,"."); if (lp) *lp=0;
				iIndex=-1;
				//
				// Ricerca usando il namespace (new 2008)
				//
				if (psDoc->bUseNamespace)
				{
					for (;a<psDoc->dmiElement.Num;a++)
					{
						if ((!strcmp(psDoc->arElement[a].pName,lpElem)||!strcmp(lpElem,"*"))&&psDoc->arElement[a].iLevel==iLevel)
						{
							iIndex=a;
							break;
						}
						if (psDoc->arElement[a].iLevel<iLevelStart) {if (lp) *lp='.'; ehFree(lpMemo); return (INT *) iIndex;} // Fine del giro
					}
				}
				//
				// Ricerca senza namespace
				//
				else
				{
					INT iLevelEnter=psDoc->arElement[a].iLevel;
					for (;a<psDoc->dmiElement.Num;a++)
					{
						//DMIRead(&psDoc->dmiElement,a,&xmlElement); if (!strcmp(xmlElement.szName,lpElem)&&xmlElement.iLevel==iLevel)
						if ((!strcmp(psDoc->arElement[a].pName,lpElem)||!strcmp(lpElem,"*"))&&psDoc->arElement[a].iLevel==iLevel)
						{
							iIndex=a; break;
						}
						if (psDoc->arElement[a].iLevel<iLevelStart||
							psDoc->arElement[a].iLevel<iLevel) {
							if (lp) *lp='.'; 
							ehFree(lpMemo); 
							return (INT *) iIndex;
						} // Fine del giro
					}
				}

				if (iIndex==-1)
				{
					if (lp) *lp='.';
					ehFree(lpMemo);
					return (INT *) iIndex;
				}

				if (!lp) break; // Finito
				*lp='.';
				lpElem=lp+1;
				iLevel++; a++;
			}
			ehFree(lpMemo);
			return (INT *) iIndex;

		case WS_PROCESS: // Richiede un array di struttura

			//win_infoarg("xmlParser: Richiedo array per %s" CRLF,ptr); getch();
			psz=ptr;
			idxStart=(INT) xmlParser(psDoc,WS_FINDKEY,info,psz);
			if (idxStart<0) 
				return NULL;

			pXml=psDoc->arElement+idxStart; 
			//printf("Trovato %d %s" CRLF,idx,ptr); getch();
			iElement=0; 
			for (iStep=0;iStep<2;iStep++) {
				
				idx=idxStart;
				if (iStep==1) arsXml=ehAllocZero((iElement+1)*sizeof(XMLELEMENT *));
				iElement=0;
				pXml=psDoc->arElement+idx; 
				if (iStep==1) {arsXml[iElement]=pXml;}
				iElement++;
				// ARPtrMakerEx(WS_ADD,pXml,&sArMaker);

				lpSearch=pXml->pName; iLevelStart=pXml->iLevel;
				lp=ptr; bAllChildren=false; if (lp[strlen(lp)-1]=='*') bAllChildren=true;
				for (idx++;idx<psDoc->dmiElement.Num;idx++)
				{
					pXml=psDoc->arElement+idx;
					if (pXml->iLevel>iLevelStart) continue;
					if (pXml->iLevel<iLevelStart) break;
					if (bAllChildren||
						!strcmp(pXml->pName,lpSearch))
					{
						pXml=psDoc->arElement+idx;
						//ARPtrMakerEx(WS_ADD,pXml,&sArMaker);
						if (iStep==1) {arsXml[iElement]=pXml;}
						iElement++;
					}
				}
				if (iStep==1) {arsXml[iElement]=NULL; iElement++;}
				//arXml=ARPtrMakerEx(WS_CLOSE,NULL,&sArMaker);
			}
#ifdef EH_MEMO_DEBUG
			if (ptr) memDebugUpdate(arsXml,ptr,__LINE__);
#endif
			return arsXml;

	}
	return NULL;
}

#define STR_CDATA_START "<![CDATA["
#define STR_CDATA_END "]]>"

CHAR *TagCloseSearch(BYTE *lpStart)
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

static CHAR * _getEntity(CHAR **lppSource,INT *lpiSize,EN_XMLTAG_IS *lpiType,BYTE **lpSourcePoint)
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
		if (*lps==10) iCurrentLine++;
	}
	for (;*lps;lps++)
	{
		if (*lps==10) iCurrentLine++;
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
						_lineCountUpdate(lpEntityStart,lpEntityEnd);
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
					lpEntityStart=lps; lpEntityEnd=TagCloseSearch(lpEntityStart+1);
					if (lpEntityEnd)
					{
						_lineCountUpdate(lpEntityStart,lpEntityEnd);
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

static CHAR *LEndTag(CHAR *lp)
{
	for (;*lp;lp++)
	{
		if (*lp=='>') return (lp);
	}
	return lp-1;
}
static EH_AR XmlAttribParser(BYTE *pszAtt)
{
	EH_AR ar=ARNew();
	INT a,iMode=0;
	BYTE *pStart=pszAtt;
	BYTE *ps=NULL;
	BYTE *pszAttrib=NULL,*pszValue=NULL,*pszValueAdd=NULL;
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
				pszValueAdd=ehAlloc(strlen(pszAttrib)+strlen(pszValue)+10);
				sprintf(pszValueAdd,"%s\1%s",pszAttrib,pszValue?pszValue:"");
				ARAdd(&ar,pszValueAdd);
				ehFreePtr(&pszValueAdd);
				ehFreePtr(&pszAttrib);
				ehFreePtr(&pszValue);
				ps=NULL;
				cComma=0;
				iMode=0; bFull=TRUE;
				break;
		}
	}
	if (pszAttrib||pszValue) ehError();
	if (!bFull)
	{
		ar=ARDestroy(ar);
	}
	else
	{
		// Spesso tutte le stringhe per fare la ricerca più veloce
		for (a=0;ar[a];a++) {ps=strstr(ar[a],"\1"); *ps=0;}
	}

	return ar;
}

static void _decode(XMLDOC * psDoc, XMLELEMENT * psXml) {

	switch (psDoc->enCharSource) {
			
			case ULT_CS_LATIN1:
				psXml->pwValue=strEncodeEx(1,psXml->lpValue,1,SD_HTML);
				break;
	
			case ULT_CS_UTF8:
				psXml->pwValue=strEncodeEx(1,psXml->lpValue,2,SD_HTML,SD_UTF8);
				break;

			default:
				break;
	}
}

//
//
//
static void _xmlScan(XMLDOC * psDoc,CHAR **lpSource,INT iParent)
{
	BYTE *lpEntity,*pb;
	CHAR *lpName;
	CHAR *lpAtt;
//	CHAR *lp;

	INT iCount=0;
	INT iSize;
	BOOL fOpen,fClose;
	XMLELEMENT xmlElement;
	EN_XMLTAG_IS enType;
	INT idxLastElement;
	static INT iLevel=0;
	BYTE *lpSourcePoint;

	DMILock(&psDoc->dmiElement,NULL);
	while (TRUE)
	{
		if (!**lpSource) break;
		lpEntity=_getEntity(lpSource,&iSize,&enType,&lpSourcePoint);
		if (enType==IS_A_ERROR) {win_infoarg("Errore in XML"); ehFree(lpEntity); break;}
		if (enType==IS_A_END) {ehFreeNN(lpEntity); break;}

		switch (enType)
		{
			case IS_A_TAG: // E' un TAG (Elemento)

				//
				// Intestazione del XML
				//
				if (!_memicmp(lpEntity,"<?xml",5)) {

					psDoc->pszXml=strDup(lpEntity);
					psDoc->pszVersion=strExtract(lpEntity,"version=\"","\"",true,false);
					if (!psDoc->pszVersion) psDoc->pszVersion=strExtract(lpEntity,"version=\'","\'",true,false);
					psDoc->pszEncoding=strExtract(lpEntity,"encoding=\"","\"",true,false);
					if (!psDoc->pszEncoding) psDoc->pszVersion=strExtract(lpEntity,"encoding=\'","\'",true,false);

					if (psDoc->pszEncoding) {
						if (!strCaseCmp(psDoc->pszEncoding,"Windows-1252")) psDoc->enCharSource=ULT_CS_LATIN1;
						else if (!strCaseCmp(psDoc->pszEncoding,"ISO-8859-1")) psDoc->enCharSource=ULT_CS_LATIN1;
						else if (!strCaseCmp(psDoc->pszEncoding,"utf8")||!strCaseCmp(psDoc->pszEncoding,"utf-8")) psDoc->enCharSource=ULT_CS_UTF8;
						else printf("\7Encoding XML [%s] non gestito",psDoc->pszEncoding);
					}
					psDoc->enCharGet=psDoc->enCharSource;
					ehFree(lpEntity); 
					continue;
				}		// Versione

				// Estraggo Nome elemento
				if (lpEntity[iSize-2]=='/') {fOpen=FALSE; lpEntity[iSize-2]=0;} else fOpen=TRUE;
				lpName=lpEntity+1; lpEntity[iSize-1]=0;
				if (*lpName=='/') {fClose=TRUE; fOpen=FALSE; lpName++;} else fClose=FALSE;
				for (pb=lpName;*pb;pb++) {if (*pb<'!') break;}
				//lp=strstr(lpName," ");
				if (pb) {*pb=0; lpAtt=pb+1;} else lpAtt=NULL;
				//win_infoarg("iParent=%d Tag[%s] Att=[%s] fOpen=%d fClose=%d",iParent,lpName,lpAtt,fOpen,fClose);

				if (fClose)
				{
					if (iParent>-1)
					{
						DMIRead(&psDoc->dmiElement,iParent,&xmlElement);
						xmlElement.lpSourceEnd=LEndTag(lpSourcePoint);

						xmlElement.pszValueStart=LEndTag(xmlElement.lpSourceStart)+1;
						xmlElement.pszValueEnd=lpSourcePoint-1;

						DMIWrite(&psDoc->dmiElement,iParent,&xmlElement);
					}
					// if (psXmlElementParent) psXmlElementParent->lpSourceEnd=LEndTag(lpSourcePoint);
					ehFree(lpEntity); return;
				}

				//
				// Inserisco il Nuovo elemento
				//
				_(xmlElement);
				xmlElement.psDoc=psDoc;
				xmlElement.pNameOriginal=strDup(lpName); // <---

				//
				// Estraggo NameSpace
				//
				if (psDoc->bUseNamespace)
				{
					CHAR *p;
					p=strstr(xmlElement.pNameOriginal,":");
					if (p)
					{	*p=0;
						xmlElement.pNamespace=strDup(xmlElement.pNameOriginal); // <--
						xmlElement.pName=strDup(p+1); // <--
						*p=':';
					}
					else
					{
						xmlElement.pName=xmlElement.pNameOriginal;
					}
				}
				else
				{
					xmlElement.pName=xmlElement.pNameOriginal;
				}

				xmlElement.iLevel=iLevel;
				xmlElement.iParent=iParent;
				xmlElement.idx=psDoc->dmiElement.Num;
				xmlElement.lpSourceStart=lpSourcePoint;
				xmlElement.iSourceLine=iCurrentLine;
				if (lpAtt) xmlElement.arAttrib=XmlAttribParser(lpAtt);
				DMIUnlock(&psDoc->dmiElement);
				DMIAppendDyn(&psDoc->dmiElement,&xmlElement);
				DMILock(&psDoc->dmiElement,NULL);

				if (fOpen)
				{
					iLevel++;
					idxLastElement=psDoc->dmiElement.Num-1;
					_xmlScan(psDoc,lpSource,idxLastElement);
					iLevel--;
				}
				ehFree(lpEntity);
 				break;

			case IS_A_VALUE:
				if (!psDoc->dmiElement.Num) ehExit("xmlError:[%s]",lpEntity);
				DMIRead(&psDoc->dmiElement,psDoc->dmiElement.Num-1,&xmlElement);
				xmlElement.lpValue=strDup(lpEntity);
				_decode(psDoc,&xmlElement);
				DMIWrite(&psDoc->dmiElement,psDoc->dmiElement.Num-1,&xmlElement);
				ehFree(lpEntity);
				break;

			case IS_A_CDATA:

				DMIRead(&psDoc->dmiElement,psDoc->dmiElement.Num-1,&xmlElement);
				xmlElement.lpValue=strDup(lpEntity);
				strReplace(xmlElement.lpValue,STR_CDATA_START,"");
				strReplace(xmlElement.lpValue,STR_CDATA_END,"");
				_decode(psDoc,&xmlElement);
				DMIWrite(&psDoc->dmiElement,psDoc->dmiElement.Num-1,&xmlElement);
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
CHAR * _xmlGetAttribAllocEx(XMLELEMENT *psDoc,CHAR *lpName,CHAR *pDefault,CHAR *pProgram,INT iLine)
{
	CHAR *lp,*lpBuf=NULL;
	lp=xmlGetAttrib(psDoc,lpName);
	if (lp) lpBuf=_strDupEx(lp,pProgram,iLine);
	if (!lpBuf&&pDefault) lpBuf=_strDupEx(pDefault,pProgram,iLine);
	return lpBuf;
}

#else
CHAR * xmlGetAttribAlloc(XMLELEMENT *psDoc,CHAR *lpName,CHAR *pDefault)
{
	CHAR *lp,*lpBuf=NULL;
	lp=xmlGetAttrib(psDoc,lpName);
	if (lp) lpBuf=strDup(lp);
	if (!lpBuf&&pDefault) lpBuf=strDup(pDefault);
	return lpBuf;
}
#endif

/*
BYTE *XMLGetAttribSQ(XMLELEMENT *psDoc,CHAR *lpName)
{
	CHAR *lps;
	CHAR *lpe;
	CHAR szServ[80];
//	static BYTE szAttrib[600];

	if (!psDoc->lpAttrib) return NULL;
	lp
	sprintf(szServ," %s=\'",lpName);
	lps=strstr(psDoc->lpAttrib,szServ); if (!lps) return NULL;
	lps+=(strlen(szServ));
	lpe=strstr(lps,"\'"); if (lpe) *lpe=0; else return NULL;
	if (strlen(lps)>(sizeof(szAttrib)-1)) ehExit("XMLGetAttribSQ(): Overload Xml Attribute memory");
	strcpy(szAttrib,lps);
	*lpe='\'';
	return szAttrib;
}
	*/

CHAR * xmlGetAttrib(XMLELEMENT *psDoc,CHAR *lpName)
{
	CHAR *lps;
	if (!psDoc->arAttrib) return NULL;
//	lps=ARSearch(psDoc->arAttrib,lpName,FALSE);
	lps=ARIsIn(psDoc->arAttrib,lpName,FALSE);
	if (!lps)
	{
		CHAR szServ[200];
		sprintf(szServ,":%s",lpName);
		lps=ARSearch(psDoc->arAttrib,szServ,FALSE);
	}
	if (lps) lps+=strlen(lps)+1;
	return lps;
}

//
// xmlGetAttribStr() legge una stringa con tutti gli attributi (da liberare con ehFree();
//
CHAR *	xmlGetAttribStr(XMLELEMENT *psDoc)
{
	CHAR *lps=NULL,*p;
	EH_AR arNew=ARNew();
	INT a;
	if (!psDoc->arAttrib) return NULL;
	for (a=0;psDoc->arAttrib[a];a++) {
		p=psDoc->arAttrib[a]+strlen(psDoc->arAttrib[a])+1;
		ARAddarg(&arNew,"%s=\"%s\"",psDoc->arAttrib[a],p);
		//dwSize=strlen(psDoc->arAttrib[a])+strlen(p)+4;
	}
	lps=ARToString(arNew," ","","");
	return lps;
}


/*
	sprintf(szServ," %s=\"",lpName);
	lps=strstr(psDoc->lpAttrib,szServ);
	if (!lps)
	{
		sprintf(szServ,":%s=\"",lpName);
		lps=strstr(psDoc->lpAttrib,szServ);
	}
	if (!lps) return XMLGetAttribSQ(psDoc,lpName);
	lps+=(strlen(szServ));
	lpe=strstr(lps,"\""); if (lpe) *lpe=0; else return NULL;
	if (strlen(lps)>(sizeof(szAttrib)-1)) ehExit("xmlGetAttrib(): Overload Xml Attribute memory");
	strcpy(szAttrib,lps);
	*lpe='\"';
	return szAttrib;
}
	*/

CHAR * xmlGetValue(XMLDOC * psDoc,INT idx,CHAR *pNodeElement)
{
	XMLELEMENT *pXml;
	pXml=xmlParser(psDoc, WS_FIND, idx, pNodeElement);
	if (!pXml) return NULL;
	return pXml->lpValue;
}

CHAR * xmlGet(XMLELEMENT * pxml,CHAR *pszNodeElement)
{
	return xmlGetValue(pxml->psDoc,pxml->idx,pszNodeElement);

}
INT		xmlGetIdx(XMLELEMENT * pxml) {
	if (!pxml) return 0; else return pxml->idx;
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

static void _lineCountUpdate(BYTE *pStart,BYTE *pEnd)
{
	BYTE *p;
	for (p=pStart;p<pEnd;p++) {if (*p==10) iCurrentLine++;}
}

void * xmlArrayFree(XMLARRAY ar)
{
	if (ar) ehFree(ar);
	return NULL;
}

XMLDOC * xmlDoc(XMLDOC *psDoc, XMLELEMENT *psEle) {
	if (psDoc) return psDoc;
	return psEle->psDoc;
}

XMLELEMENT * xmlElementClone(XMLELEMENT * psXmlOriginal)
{
	XMLELEMENT * psXmlReturn=NULL;
	psXmlReturn=ehAlloc(sizeof(XMLELEMENT));
	memcpy(psXmlReturn,psXmlOriginal,sizeof(XMLELEMENT));
	psXmlReturn->bAlloc=TRUE;
	if (psXmlReturn->pNamespace) psXmlReturn->pNamespace=strDup(psXmlOriginal->pNamespace);
	if (psXmlReturn->pName) psXmlReturn->pName=strDup(psXmlOriginal->pName);
	if (psXmlReturn->pNameOriginal) psXmlReturn->pNameOriginal=strDup(psXmlOriginal->pNameOriginal);
//	if (psXmlReturn->lpAttrib) psXmlReturn->lpAttrib=strDup(psXmlOriginal->lpAttrib);
	if (psXmlReturn->arAttrib) psXmlReturn->arAttrib=ARDup(psXmlOriginal->arAttrib);
	if (psXmlReturn->lpValue) psXmlReturn->lpValue=strDup(psXmlOriginal->lpValue);
	if (psXmlReturn->pwValue) psXmlReturn->pwValue=wcsDup(psXmlOriginal->pwValue);
	return psXmlReturn;
}

void * xmlElementFree(XMLELEMENT * psXmlOriginal)
{
	BOOL bAlloc;
	if (!psXmlOriginal) return NULL;
	bAlloc=psXmlOriginal->bAlloc;
	if (psXmlOriginal->pName&&
		psXmlOriginal->pNameOriginal!=psXmlOriginal->pName) {
  			ehFree(psXmlOriginal->pName);
	}
	ehFreeNN(psXmlOriginal->pNameOriginal);
	ehFreeNN(psXmlOriginal->pNamespace);
	if (psXmlOriginal->arAttrib) ARDestroy(psXmlOriginal->arAttrib);
	ehFreeNN(psXmlOriginal->lpValue);
	ehFreeNN(psXmlOriginal->pwValue);
	if (bAlloc) ehFree(psXmlOriginal); else memset(psXmlOriginal,0,sizeof(XMLELEMENT));
	return NULL;
}

XMLELEMENT * xmlElementWithAttribute(XMLARRAY arXml,CHAR *pAttributeName,CHAR *pAttributeValue,BOOL bElementAlloc)
{
	INT a;
	XMLELEMENT * psXmlReturn=NULL;
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
INT xmlIdBuilder(XMLDOC *psDoc)
{
	INT a,iCnt=0;
	CHAR *psz;
	if (psDoc->arId) ehFree(psDoc->arId);
	psDoc->arId=ehAllocZero(psDoc->dmiElement.Num*sizeof(INT));
	for (a=0;a<psDoc->dmiElement.Num;a++)
	{
		if (!psDoc->arElement[a].arAttrib) continue;
		psz=xmlGetAttrib(&psDoc->arElement[a],"id");
		if (psz)
		{
			psDoc->arId[iCnt]=a; iCnt++;
		}
	}
	psDoc->arId[iCnt]=-1;
	return iCnt;
}

//
// XMLIdSearch() - 2010
//
XMLELEMENT * xmlIdSearch(XMLDOC *psDoc,CHAR *psIdValue)
{
	INT a,iCnt=0;
	XMLELEMENT *xmlRet;
	if (!psDoc->arId) ehError();
	for (a=0;psDoc->arId[a]>-1;a++)	{
		xmlRet=&psDoc->arElement[psDoc->arId[a]];
		if (!strcmp(xmlGetAttrib(xmlRet,"id"),psIdValue)) return xmlRet;
	}
	return NULL;
}

//
// Ritorna l'indirizzo URI del name space indicato
//
	/*
CHAR * XMLNameSpaceLocation(XMLDOC *psDoc,CHAR *pNamespace,INT idx)
{
	XMLELEMENT *pElement;
	BYTE *pBuf,*pNS,*pStart,*pEnd;
	pElement=&psDoc->arElement[idx];
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

CHAR * xmlElementToString(XMLDOC * xmlDoc,XMLELEMENT *psXmlElement,BOOL bOnlyValue) {

	CHAR *pString=NULL;
	if (!bOnlyValue)
		pString=strTake(psXmlElement->lpSourceStart,psXmlElement->lpSourceEnd);
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
BYTE * xmlQGet(CHAR *pszXml,CHAR *pszFormat,...) {

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

