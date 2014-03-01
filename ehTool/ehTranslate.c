// -------------------------------------------------------------
// ehTranslate
// Traduzioni via Web ... e non solo
// 
// Ferrà srl 11/2011
// -------------------------------------------------------------

#include "/easyhand/inc/easyhand.h" 
#include "/easyhand/inc/ehTranslate.h" 
//#include "/easyhand/ehtool/webtool.h"
#include <time.h>


// Interior: 3 CabaÃ±as, 1 baÃ±o, comedor tranf.  \u000d\u000aAccesorios: Toldos, cubierta, tapas de escotilla, Molinete de ancla elÃ©ctrico, kit de anclaje + ANS. incluso, kit de amarre, calefacciÃ³n Webasto (l\/m reacondicionado), batt. + batt. Caldera de agua caliente, N4 2009: herramientas, estaciÃ³n de piloto de viento tridata, raymarine plotter + A6000, imp radar, VHF, radio TV\u000d\u000aVELAS: vela mayor 3 (carbono, kevlar, crucero), genona 2 (kevlar, crucero), engrapar kevlar, OlÃ­mpico, SPI, 2 spi, gennaker, jeack lesy + copriranda anotaciones: conducir bombilla, Polo de spinnaker de carbono\u000d\u000aComodidades: dentro de 50 millas
//
// GoogleTranslate()
// traduce il testo passato nella lingua richiesta grazie a Google
//
#define GOOGLE_MAX_LENGTH 1024
typedef struct {

	S_TTE * psTag;
	EH_AR	arPhrase;
	INT	iPhraseProblems;
	INT	iPhraseTotal;
	CHAR	szCharBreak[20];

} S_PHRASES;

static void _tagBuild(CHAR * pszDest,INT id,INT iType) {

	switch (iType) {

		case 0:
			sprintf(pszDest,"{%d/}",id);
			break;

		case 1:
			sprintf(pszDest,"{%d}",id);
			break;

		case 2:
			sprintf(pszDest,"[%d]",id);
			break;

		default:
			ehError();
	}
}
					//sprintf(szServ,"<%d>",ARLen(psTag->arTag));


// static BOOL L_GoogleMaxSize(EH_AR arPhrase);
static BYTE * _googleCall( WCHAR * pwString,
						   CHAR  * langSource,
						   CHAR  * langDest,
						   CHAR  * pszApiKey,
						   CHAR  * pszUserIp,
						   INT  * piRequestDone,
						   ET_ERROR * petError);
static UTF8 * _bingCall(UTF8 * pszSource,
						CHAR * langSource,
						CHAR * langDest,
						CHAR * pszApiKey,
						INT * piRequestDone,
						ET_ERROR * piError);

static INT _tokControl(CHAR *psz,CHAR *pszChar);
static BOOL _isNaN(BYTE *pString); // new 2009
static void _phraseDestroy(S_PHRASES * psPh);

//
// _googlePhraseExtract()
//
S_PHRASES * _phraseCreate(UTF8 * pszSource,INT iMaxPhrases) { //,EN_TTE enTech) {

	S_PHRASES * psPh=ehAllocZero(sizeof(S_PHRASES));
	INT f,iPart;
	INT iNum;

//	psPh->arPhrase=ARNew();

	//
	// Ricodifico i Tag Sensibili
	//
	psPh->psTag=tagEncode(pszSource);//,enTech);//TTE_GOOGLE);
	*psPh->szCharBreak=0;

	//
	// Spezzo la frase (se è il caso)
	//
	
	if (strlen(psPh->psTag->pszSourceTagged)<GOOGLE_MAX_LENGTH)
	{
		psPh->arPhrase=ARNew();
		ARAdd(&psPh->arPhrase,psPh->psTag->pszSourceTagged);
	}
	else
	{
		// Array con 
		CHAR *arDiv[]={".",CRLF,"\r","\n",";"," - ","-",",","(",")",NULL};
		INT x;
		
		iPart=100000;
		for (x=0;arDiv[x];x++)
		{
			f=_tokControl(psPh->psTag->pszSourceTagged,arDiv[x]);
			if (f) 
			{
				if (f>iMaxPhrases&&iMaxPhrases) continue;
				if (f<iPart) 
				{
//					pCharBreak=arDiv[x];	
					strcpy(psPh->szCharBreak,arDiv[x]);
					iPart=f;
					if (psPh->psTag->bHtml) 
						break;
				}
			}
		}

		//
		// Non riesce a spezzare la stringa > mi fermo
		//
		if (!*psPh->szCharBreak) 
		{
			_phraseDestroy(psPh);
			return NULL; // = GE_STRING_TOO_LONG
		}
		psPh->arPhrase=ARCreate(psPh->psTag->pszSourceTagged,psPh->szCharBreak,&iNum);

	}
/*
#ifdef EH_CONSOLE
	if (iNum>1) printf("(%d parti)",iNum);
#endif
*/
	psPh->iPhraseTotal=ARLen(psPh->arPhrase);
	return psPh;
}


static void _phraseDestroy(S_PHRASES * psPh) {

	CHAR * pTradotto;
	if (psPh->arPhrase) ARDestroy(psPh->arPhrase);
	pTradotto=tagDecode(psPh->psTag,NULL,FALSE,TRUE); 
	ehFreePtr(&pTradotto);
	ehFree(psPh);

}

/*
CHAR * GoogleTranslateUtf8(CHAR *	pSourceUtf8,
						   CHAR *	langSource,
						   CHAR *	langDest,
						   BOOL		bShowError,		// T/F se devo mostrare gli errori
						   BYTE *	pszApiKey,		// ApiKey (non è strettamente necessaria)
						   BYTE *	pszUserIp,		// Ip da comunicare a Google end-user (non è strettamente necessario)
						   INT *	piRequestDone, // Numero di richieste fatte
						   BOOL		bDetectLangSource,
						   EN_GOOGLE_ERROR	*	piError,
						   void * (* extCacheTrans)(EN_MESSAGE enMess,LONG lParam,void *pVoid))
						   */

UTF8 * ehTranslate(	ET_PARAM	enParam,
					UTF8 *		pszSource,
					CHAR *		pszLangSource,
					CHAR *		pszLangDest,
					CHAR *		pszJsonParams,	// {googlekey:"asda", bingkey:"asd"}
					ET_REPORT * psReport,
					void *		(* extAssist)(EN_MESSAGE enMess,LONG lParam,void *pVoid)
					)

{
	WCHAR *pw;
	BYTE *pTradotto=NULL;
	S_PHRASES * psPh=NULL;
	INT f;
	BOOL bError=FALSE;
	INT iLen;
	INT iCharsBad;
	BYTE *pUtf;
	CHAR szLangSource[20];
	double dRap;
	INT iWords;
	ET_REPORT sReportTemp;
	EH_JSON * psJson;
	CHAR * pszUserIp=NULL;
	INT		iMaxPhrases=0;

	if (!psReport) psReport=&sReportTemp;
	memset(psReport,0,sizeof(ET_REPORT));
	if (!(enParam&ET_TRY_LOCAL)) extAssist=NULL;

	strcpy(psReport->szLangSource,pszLangSource);
	strcpy(psReport->szLangDest,pszLangDest);

	//
	// Assistente: ricerco prima in cache la frase commpleta
	//
	strTrim(pszSource);

	if (enParam&ET_STAT) {
		psReport->dwChars=strlen(pszSource);
		psReport->dwWords=strWordCount(pszSource);
	}
	if (!(enParam&ET_TRANS)) return NULL;

	if (pszJsonParams) {
		CHAR * psz;
		psJson=jsonCreate(pszJsonParams);
		psz=jsonGet(psJson,"maxPhrases");
		if (psz) iMaxPhrases=atoi(psz);
	}

	//
	// PREPARAZIONE FRASI DA RICHIEDERE
	// A seconda del servizio esterno di traduzione creo le frasi richieste
	//
	psPh=_phraseCreate(pszSource,iMaxPhrases);//,TTE_BING); 
	/*
	if (enParam&ETT_BING) {
		psPh=_phraseCreate(pszSource,TTE_BING); 
		psReport->enTech=ETT_BING;
	}
	else if (enParam&ETT_GOOGLE) {
		psPh=_phraseCreate(pszSource,TTE_GOOGLE);
		psReport->enTech=ETT_GOOGLE;
	}
	else if (enParam&ETT_LOCAL) {
		psPh=_phraseCreate(pszSource,TTE_GOOGLE);
		psReport->enTech=ETT_LOCAL;
//		_googleBingExtract();
	}
	else ehError();
*/
	if (!psPh) {
		psReport->etError=GE_STRING_TOO_LONG;
		return NULL;
	}

	//
	// Richiesta traduzioni frasi
	//
	if (enParam&ET_OUTPUTDETT) {
		if (psPh->iPhraseTotal>1) 
			printf("%s|%d}",pszLangDest,psPh->iPhraseTotal);
			else
			printf("%s}",pszLangDest);
	}

	if (iMaxPhrases&&psPh->iPhraseTotal>iMaxPhrases) {
	
		psReport->etError=GE_TOO_PHRASES;
		_phraseDestroy(psPh);
		jsonDestroy(psJson);
		return NULL;

	}
	if (enParam&ET_OUTPUTDETT) printf("|Phrase: total %d, problems %d, html:%d|" CRLF,psPh->iPhraseTotal,psPh->iPhraseProblems,psPh->psTag->bHtml);

	for (f=0;psPh->arPhrase[f];f++)
	{
		CHAR *	pszPhrase;
		CHAR *	pszPrefix=NULL;

		strTrim(psPh->arPhrase[f]); 
		pszPhrase=psPh->arPhrase[f];
		if (strEmpty(pszPhrase)) continue;
		iLen=strlen(pszPhrase); if (iLen<2) continue; // Non traduco frasi con meno di 2 caratteri

		if (psPh->psTag->bHtml) {
/*
			if (*pszPhrase=='{') pszPrefix=strExtract(pszPhrase,NULL,"}",FALSE,TRUE);

			if (pszPrefix) {
				CHAR szServ[200];
				strCpy(szServ,pszPrefix,sizeof(szServ)); szServ[strlen(szServ)-1]=0;
				if (!_isNaN(szServ)) {
					pszPhrase+=strlen(pszPrefix);
				}
			}			
			*/
		}

		//
		// a. Controllo se è un numero
		//
		if (!_isNaN(pszPhrase)) {ehFreePtr(&pszPrefix); continue;}

		//
		// b. Conto le parole
		//
		iWords=0; iCharsBad=0; dRap=0;
		if (enParam&ET_LANGDETECT) { //bDetectLangSource)

			iWords=strWordCount(pszPhrase);

			// Precontrollo stringa
			// Proporzione: iCharsBad:iLen=x:100
			iCharsBad=strCount(pszPhrase,".()[],-*_");
			dRap=iCharsBad*100/iLen;
			if (dRap>50) // Frase composta da oltre il 50% di simboli e non di lettere
			{
				psReport->etError=GE_STRING_SUSPECT; // L'indicazione della lingua sorgente è errata
				psPh->iPhraseProblems++;
				if (enParam&ET_OUTPUTDETT) printf("?");
				ehFreePtr(&pszPrefix); 
				continue;
			}
		}

		//
		// Local request ###########################################################################################
		//
		pUtf=NULL;

		if (extAssist) {

			S_TRANSLATION sTrans;

			sTrans.pszLangSource=pszLangSource;
			sTrans.pszLangDest=pszLangDest;
			sTrans.utfSource=pszPhrase;
			sTrans.utfDest=NULL;
			if (extAssist(WS_FIND,0,&sTrans)) {
				psReport->iLocalResponse++;
				if (enParam&ET_STAT) {
					psReport->dwLocalChars+=strlen(pszPhrase);
					psReport->dwLocalWords+=strWordCount(pszPhrase);
				}
				pUtf=sTrans.utfDest;
				if (pUtf) {
					psReport->enTech=ET_LOCAL;
					if (enParam&ET_OUTPUTDETT) printf("l");
				}
			}
		}
		
		
		//
		// BING request ###########################################################################################
		//
		if (!pUtf&&
			psJson&&
			enParam&ET_TRY_BING) {
			
			CHAR * pszBingApp;
			INT iRequestDone=0;
			
			pszBingApp=jsonGet(psJson,"bingAppId");
			if (enParam&ET_OUTPUTDETT) printf("|pszBingApp:%s|" CRLF,pszBingApp);
			if (pszBingApp) {

				if (enParam&ET_STAT) {

					psReport->dwBingChars+=strlen(pszPhrase);
					psReport->dwBingWords+=strWordCount(pszPhrase);

				}

				pUtf=_bingCall(		pszPhrase,
									pszLangSource,
									pszLangDest,
									pszBingApp,
									&iRequestDone,
 									&psReport->etError); 
				if (enParam&ET_OUTPUTDETT) {
					printf("(%s) > (%s) #%d",pszPhrase,pUtf,psReport->etError);
				}
				psReport->iBingRequest+=iRequestDone;
				if (!strEmpty(pUtf)) 
					{	psReport->enTech=ET_BING;
						psReport->iBingResponse++; 
						if (enParam&ET_OUTPUTDETT) printf("B");
					}
					else
					{psReport->iBingErrors++; if (enParam&ET_OUTPUTDETT) printf("Bx");}
			}
		}

		//
		// GOOGLE request ###########################################################################################
		//
		if (!pUtf&&
			psJson&&
			enParam&ET_TRY_GOOGLE) {
		
				CHAR * pszGooleKey;
				INT iRequestDone=0;
				pszGooleKey=jsonGet(psJson,"googlekey");
				if (pszGooleKey) {
	
					if (extAssist) pszUserIp=extAssist(WS_DO,0,""); // Richiedo IP nuovo ad ogni frase

					//
					// Compongo indirizzo URL con i dati
					//
					pw=strDecode(pszPhrase,SE_UTF8,NULL);  if (!pw) ehError();

					// NOTA: da utf8>unicode e poi in minuscolo, ho visto che traduce Google meglio
					_wcslwr(pw); 

					// Se devo capire la lingua, azzero la lingua sorgente
					if (enParam&ET_LANGDETECT)
					{
						*szLangSource=0;

					} else strcpy(szLangSource,pszLangSource);

					if (enParam&ET_STAT) {
						psReport->dwGoogleChars+=strlen(pszPhrase);
						psReport->dwGoogleWords+=strWordCount(pszPhrase);
					}

					pUtf=_googleCall(	pw,
										szLangSource,
										pszLangDest,
										pszGooleKey,
										pszUserIp,
										&iRequestDone,
										&psReport->etError); 
					psReport->iGoogleRequest+=iRequestDone;
					if (pUtf) 
						{	psReport->enTech=ET_GOOGLE;
							psReport->iGoogleResponse++; 
							if (enParam&ET_OUTPUTDETT) printf("G");
						}
						else
						{psReport->iGoogleErrors++; if (enParam&ET_OUTPUTDETT) printf("Gx");}
					ehFree(pw);
				}
		}

		//
		// Controlli Post Traduzione -----------------------------------------------------------------------------|
		//
		if (pUtf) {

			//
			// Controllo della lingua in uscita
			//
			if (enParam&ET_LANGDETECT)
			{
				if (psReport->etError==GE_LANG_DIFFERENT)
				{
					psPh->iPhraseProblems++;
					ehFree(pUtf);
					continue;
				}
				//
				// Se la lingua è diversa da quella richiesta 
				// E la lingua è |fr/en/de|
				//
				if (strstr("|fr|en|de|",szLangSource)&&
					dRap<30&&
					strcmp(szLangSource,pszLangSource))
				{
					if (iWords>2) {
						psReport->etError=GE_LANG_DIFFERENT;
					}
					else {
						if (!psReport->etError) psReport->etError=GE_STRING_SUSPECT; // L'indicazione della lingua sorgente è errata
					}
					psPh->iPhraseProblems++;
				}
			}


			//
			// Memorizzo la traduzione andata a buon fine - Update dell Assistant
			//
			if (extAssist&&
				psReport->enTech!=ET_LOCAL) {
			
				S_TRANSLATION sTrans;

				sTrans.enTech=psReport->enTech;
				sTrans.pszLangSource=pszLangSource;
				sTrans.pszLangDest=pszLangDest;
				sTrans.utfSource=pszPhrase;
				sTrans.utfDest=pUtf;
				extAssist(WS_UPDATE,0,&sTrans);

			}

			if (pszPrefix) {
			
				CHAR *psz=ehAlloc(strlen(pszPrefix)+strlen(pUtf)+100);
				strcpy(psz,pszPrefix); strcat(psz,pUtf);
				strAssign(&psPh->arPhrase[f],psz);
				ehFreePtr(&pszPrefix); 
				ehFree(psz);

			} else {

				strAssign(&psPh->arPhrase[f],pUtf);
			
			}
			
			
//			printf("*");
		}
		
		if (!pUtf&&!psReport->etError) {
			psPh->iPhraseProblems++;
			psReport->etError=GE_UNAVAILABLE;
			bError=TRUE;
		}
		ehFreePtr(&pUtf);

		if (psReport->etError==GE_ABUSE) {bError=TRUE; break;}

	} // <--------- Loop sulle frasi  -------------------------------


	//
	// Fine della traduzione richiesta all'esterno -------------------------------------|
	//

	//
	// Posto controllo 1
	// Sotto il 10% di problemi sull'intera stringa resetto l'errore
	//
	if (psPh->iPhraseProblems&&!bError)
	{
		dRap=psPh->iPhraseProblems*100/psPh->iPhraseTotal;
		
		if (dRap<10) {
			psReport->etError=GE_NONE; // L'indicazione della lingua sorgente è errata
		}
	}

	//
	// Non ho errori: Riassemblo le frasi in una stringa
	//
	if (!bError) 
	{
		if (psPh->szCharBreak) 
			pTradotto=ARToString(psPh->arPhrase,psPh->szCharBreak,"",""); 
			else
			pTradotto=strDup(psPh->arPhrase[0]);

		//
		// "Ritaggo" la frase originale
		//
		pTradotto=tagDecode(psPh->psTag,pTradotto,TRUE,FALSE); 
	}
	//
	// ERRORE in traduzione
	//
	else 
	{
		pTradotto=tagDecode(psPh->psTag,NULL,FALSE,FALSE); 
		ehFreePtr(&pTradotto);
		if (enParam&ET_OUTPUTDETT) printf("!");
	}

	//
	// Libero le ricorse
	//
	//ARDestroy(arPhrase);
	_phraseDestroy(psPh);
	jsonDestroy(psJson);
	return pTradotto;
}


CHAR * _strExtract(CHAR * pszSource,CHAR * pszTagStart,CHAR * pszTagEnds,BOOL fCaseInsensitive,BOOL bWithTagStart,BOOL bWithTagEnd)
{
	INT iLen,iLenTag=0;
	CHAR * pszStart,*pszEnd;
	CHAR * pRet, * p;
	CHAR * pszLimit;
	
	//EH_AR arStart;
	EH_AR arEnd;

#ifdef __windows__
	CHAR * (*funcStr)(CHAR *,CHAR *);
#else
	CHAR * (*funcStr)(__const CHAR *,__const CHAR *);
#endif
	if (fCaseInsensitive)
        funcStr=strCaseStr;
        else
        funcStr=strstr;

	if (strEmpty(pszTagStart)) {pszStart=pszSource; iLenTag=0;}
		else
		{
			pszStart=funcStr(pszSource,pszTagStart); iLenTag=strLen(pszTagStart);
			if (!pszStart) return NULL;
		}


	arEnd=strSplit(pszTagEnds,"\1");
	if (ARLen(arEnd)) {

		int a,iLen;
		pszEnd=NULL;
		pszLimit=pszSource+strlen(pszSource);
		for (p=pszStart+strlen(pszTagStart);*p;p++) {

			for (a=0;arEnd[a];a++) {

				CHAR * pszTagx=arEnd[a];
				
				iLen=strlen(pszTagx);
				if (!iLen) continue;
				if (p+iLen>pszLimit) continue; // Troppo lunga
				if (!memcmp(p,pszTagx,iLen)) {pszEnd=p; break;}

			}
			if (pszEnd) {
				if (bWithTagEnd) pszEnd+=iLen;
				break;
			}
		}
	

//			pEnd=funcStr(pStart+iLenTag,pTagEnd);
		


	} else {
	
		pszEnd=pszStart+strLen(pszStart);
	}

	if (!pszEnd) return NULL;
	if (pszTagStart&&!bWithTagStart) pszStart+=strLen(pszTagStart);

	iLen=(SIZE_T) pszEnd-(SIZE_T) pszStart;

	pRet=ehAlloc((iLen<4)?10:iLen+1);
	memcpy(pRet,pszStart,iLen);
	pRet[iLen]=0;
	ehFree(arEnd);
	return pRet;
}

					

//
// TagEncode()
// Riconosco ed estraggo i tag "sensibili" e gli inserisco in un array, 
// sostitituendoli con {1}
//
static void _tagGrouping(CHAR * pUtf,S_TTE * psTag);
S_TTE * tagEncode(CHAR *pSourceUtf8)//,INT iMode) 
{
	S_TTE * psTag;
	BYTE *pUtf,*p;
	//BOOL bHtml=false;
	CHAR szServ[80];
	psTag=ehAllocZero(sizeof(S_TTE));
	psTag->arTag=ARNew();
	psTag->pszSourceTagged=strDup(pSourceUtf8);
	pUtf=ehAlloc(strlen(pSourceUtf8)*10); strcpy(pUtf,pSourceUtf8);

	while (true) {

		// Witango
		p=_strExtract(pUtf,"@@"," \1<",false,true,false); //if (p) {p[strlen(p)-1]=0;}
		//p=strExtract(pUtf,"@@"," ",FALSE,TRUE);	
		if (!p) p=strExtract(pUtf,"@@",NULL,TRUE,TRUE); 
		if (!p) p=strExtract(pUtf,"<@",">",TRUE,TRUE);

		// Tag C
		if (!p) {p=strstr(pUtf,"%d"); if (p) p=strDup("%d");}
		if (!p) {p=strstr(pUtf,"%s"); if (p) p=strDup("%s");}
		if (!p) {
			p=strExtract(pUtf,"<",">",TRUE,TRUE);
			if (p) psTag->bHtml=true;
		}// Tag Html
		
		if (p) 
		{
			_tagBuild(szServ,ARLen(psTag->arTag),2); // era 1
			ARAddarg(&psTag->arTag,"%s\1%s",szServ,p);

//			while (strReplace(szServ,"<","[["));
//			while (strReplace(szServ,">","]]"));
			psTag->dwSizeTag+=strlen(p);
			while (strReplace(pUtf,p,szServ)) { psTag->dwSizeTag+=strlen(p);}
			//psTag->dwSizeTag+=strlen(p);
			ehFree(p);
		} else break;	
	}
	// replace del mio tag
//	while (strReplace(pUtf,"[[","<"));
//	while (strReplace(pUtf,"]]",">"));


	//
	// La frase contiene Html
	//
	if (psTag->bHtml) {

		//CHAR * pStart=NULL;
		CHAR *psz;
//		BOOL bTag=FALSE;

		//
		// Sostituisco &nbsp;
		//
		psz="&nbsp;";
		if (strstr(pUtf,psz)) {
			//sprintf(szServ,"{%d}",ARLen(psTag->arTag));
			_tagBuild(szServ,ARLen(psTag->arTag),2);
			ARAddarg(&psTag->arTag,"%s\1%s",szServ,psz);
			while (strReplace(pUtf,psz,szServ));
		}

		/*
		//
		// Sostituisco eventuale encoding ISO con UTF8
		//
		if (strstr(pUtf,"&")) {
			
			CHAR * pUtf8;
			pUtf8=strEncodeEx(1,pUtf,2,SD_HTML,SE_UTF8);
			ehFree(pUtf);
			pUtf=pUtf8;
		
		}
		*/
		
		
		//
		// Aggrego i gruppi di Tag con nulla tra di loro
		//
		_tagGrouping(pUtf,psTag);

	}


	strAssign(&psTag->pszSourceTagged,pUtf);
	ehFree(pUtf);
	return psTag;
}

//
// _tagGrouping() > Aggrega i pezzi di soli Tags
// 
static void _tagGrouping(CHAR * pUtf,S_TTE * psTag) {

	CHAR * p;
	CHAR cTagStart='[';
	//CHAR * pszTagStart="[";
	CHAR cTagEnd=']';
	CHAR * pTagStart=NULL;
	INT iMode=0;
	CHAR * pszTake;
	CHAR szServ[80];

	for (p=pUtf;*p;p++) {

		switch (iMode) {

			case 0: // Non armato
				if (*p==cTagStart) {pTagStart=p; iMode=1;}
				break;

			case 1: // Armo la fine
				
				if (*p==cTagEnd) {iMode=2; break;}
				if (strchr("0123456789",*p)) break;
				pTagStart=NULL; iMode=0; // Falso allarme 
				break;

			case 2: 
				if (*p==cTagStart) {iMode=1; break;} // Inizia un altro tag
				
				pszTake=strTake(pTagStart,p-1); 
				_tagBuild(szServ,ARLen(psTag->arTag),2); // era 1
				ARAddarg(&psTag->arTag,"%s\1%s",szServ,pszTake);
				strReplace(pTagStart,pszTake,szServ);
				p=pTagStart+strlen(szServ)-1; // -1 perchè incremento nel loop
				ehFree(pszTake);
				iMode=0; // Riarmo ricerca
				break;

		
		}
	}
}

	/*
	CHAR *psz,* pStart=NULL;
	BOOL bTag=false;
	CHAR szServ[80];
	CHAR cTagStart='[';
	CHAR * pszTagStart="[";
	CHAR cTagEnd=']';

	for (p=pUtf;*p;p++) {

		if (!pStart) pStart=p;
		if (*p==cTagStart) bTag=true;
		else if (*p==cTagEnd) {bTag=false; continue;}
		if (!bTag) continue;
		if (*p<'!') continue;

		if (p==pStart) continue;
		psz=strTake(pStart,p-1);
		if (psz) {
			// E' solo un Tag
			if (*psz==cTagStart&&
				psz[strlen(psz)-1]==cTagEnd&&
				strCount(psz,pszTagStart)==1) {
			}
			else {

				_tagBuild(szServ,ARLen(psTag->arTag),1);
				ARAddarg(&psTag->arTag,"%s\1%s",szServ,psz);
				strReplace(pUtf,psz,szServ);
				p=strstr(pUtf,szServ); 
				p+=strlen(szServ);

			}
			ehFree(psz); 
		}			
		for (;*p!='{'&&*p;p++); p--;
		pStart=NULL;
	
	}

	if (pStart) {

		psz=strTake(pStart,pStart+strlen(pStart)-1);
		_tagBuild(szServ,ARLen(psTag->arTag),0);
		//sprintf(szServ,"<%d>",ARLen(psTag->arTag));
		ARAddarg(&psTag->arTag,"%s\1%s",szServ,psz);
		strReplace(pUtf,psz,szServ);

	}
	*/


//
// TagDecode
//
CHAR * tagDecode(S_TTE *psTagEnc,CHAR *pszSourceUtf8,BOOL bFreeSource,BOOL bFreeTag)
{
	INT a,b;
	BYTE *pUtf=NULL;
	CHAR *pszRet=NULL;
	EH_AR ar;
	INT iTags=ARLen(psTagEnc->arTag);
	if (iTags)
	{
		if (pszSourceUtf8)
		{
			pUtf=ehAlloc(strlen(pszSourceUtf8)+psTagEnc->dwSizeTag+50); 
			strcpy(pUtf,pszSourceUtf8);

			for (b=0;b<5;b++) {
				for (a=0;psTagEnc->arTag[a];a++)
				{
					ar=ARCreate(psTagEnc->arTag[a],"\1",NULL);
					while (strReplace(pUtf,ar[0],ar[1]));
					ARDestroy(ar);
				}
			}
			pszRet=pUtf;
		}
	} 
	else 
	{
		if (pszSourceUtf8) pszRet=strDup(pszSourceUtf8);
	}

	if (bFreeTag)
	{
		tagFree(psTagEnc);
	}

	if (bFreeSource) ehFree(pszSourceUtf8);
	return pszRet;
}

void tagFree(S_TTE *psTagEnc)
{
	psTagEnc->arTag=ARDestroy(psTagEnc->arTag);
	ehFreePtr(&psTagEnc->pszSource);
	ehFreePtr(&psTagEnc->pszSourceTagged);
	ehFree(psTagEnc);
}

/*
static BOOL L_GoogleMaxSize(EH_AR arPhrase)
{
	INT f;
	INT iLen;

	// Precontrollo 1024
	for (f=0;arPhrase[f];f++)
	{
		strTrim(arPhrase[f]); if (strEmpty(arPhrase[f])) continue;
		iLen=strlen(arPhrase[f]); 
		if (iLen>GOOGLE_MAX_LENGTH)  return TRUE;
	}
	return FALSE;
}
*/

//
// GoogleCall (Richiesta a Google della traduzione di una frase)
//
static BYTE * _googleCall( WCHAR * pwString,
						   CHAR  * langSource,
						   CHAR  * langDest,
						   CHAR  * pszApiKey,
						   CHAR  * pszUserIp,
						   INT  * piRequestDone,
						   ET_ERROR * petError)
{
	BYTE *pszLastError;
	CHAR szServ[200];
	BYTE *psz;
	BYTE *pPage=ehAlloc(64000);
	INT t;
	BOOL bSeveralError=FALSE;
	BYTE *putfRet=NULL;
	S_SEN *psSen;
	BOOL bExit=FALSE;
	EH_WEB *psWeb;

	INT iRequestDone=0;
	if (!piRequestDone) piRequestDone=&iRequestDone; else iRequestDone=*piRequestDone;

	strcpy(pPage,"http://ajax.googleapis.com/ajax/services/language/translate?v=1.0&q="); 
	psz=strEncodeEx(2,pwString,2,SE_UTF8,SE_URL);
	strcat(pPage,psz);
	ehFree(psz);

	psSen=senCreate();
	sprintf(szServ,"%s|%s",langSource,langDest); 
	sprintf(strNext(pPage),"&langpair=%s",senEncode(psSen,SE_URL,szServ));
	if (!strEmpty(pszApiKey)) sprintf(strNext(pPage),"&key=%s",senEncode(psSen,SE_URL,pszApiKey));
	if (!strEmpty(pszUserIp)) sprintf(strNext(pPage),"&userip=%s",senEncode(psSen,SE_URL,pszUserIp));
	senDestroy(psSen);

	//
	// Chiamata a Google (Faccio 5 tentativi)
	//
	*petError=GE_NONE;
	pszLastError=strDup("");
	bSeveralError=FALSE;
	for (t=0;t<5;t++)
	{
		psWeb=webHttpReq(pPage,"GET",NULL,FALSE,0); iRequestDone++;
		if (psWeb->sError.enCode)
		{
#ifdef EH_CONSOLE
			printf("(!%d)?",t+1); 
#endif	
			//FWebGetFree(&sWS);
			webHttpReqFree(psWeb);
			bSeveralError=TRUE; 
			strAssign(&pszLastError,psWeb->sError.szDesc);
			break;
		}
		else
		{
			EH_JSON * psJson;
			BYTE *pResponse;
			BYTE *pDetected;
			BYTE *pFraseTradotta;
			INT iResponse;

			psJson=jsonCreate(psWeb->pData);
//			jsonPrint(psJson,0);
 
//			while (TRUE)  
		
			strAssign(&pszLastError,psWeb->pData);

			// Cerco la lingua di ritorno
			//pDetected=strExtract(sWS.pData,"detectedSourceLanguage\":","}",FALSE,FALSE);
			pDetected=jsonGet(psJson,"responseData.detectedSourceLanguage");
			if (pDetected) {
//					strcpy(pDetected,strOmit(pDetected,"\"\'"));
				if (!*langSource) strcpy(langSource,pDetected);
//					ehFree(pDetected);
			}

			//pResponse=strExtract(sWS.pData,"responseStatus\":","}",FALSE,FALSE);
			pResponse=jsonGet(psJson,"responseStatus");
			if (!pResponse) {bSeveralError=TRUE; break;}
//				strTrim(pResponse);
			iResponse=atoi(pResponse);
//				ehFree(pResponse); 

//				printf("#TEST: b" CRLF);
			if (iResponse!=200) {

				//printf("-- %s" CRLF,jsonGet(psJson,"responsDetails"));
				strAssign(&pszLastError,jsonGet(psJson,"responseDetails"));
//				_getch();

			}
			switch (iResponse)
			{
				case 400:
					bSeveralError=TRUE;
					bExit=TRUE;
					*petError=GE_LANG_DIFFERENT;
//						strAssign(&pszLastError,"[Lingua sconosciuta:400]");
					break;
				
				case 403: // Abuso
					/*
#ifdef EH_CONSOLE
						printf("(Abuse!%d)?",t+1); 
#endif	
						*/
					*petError=GE_ABUSE;
					bSeveralError=TRUE; 
//						strAssign(&pszLastError,"[Abuse:403]");
					break;

				case 404: // Invalid result data
					printf("[404]");
					bSeveralError=TRUE; 
					*petError=GE_SERVER_ERROR;
//						strAssign(&pszLastError,"[Abuse:404]");
					break;

				case 503: // Non lo so
					printf("[503]");
					bSeveralError=TRUE; 
					*petError=GE_SERVER_ERROR;
//						strAssign(&pszLastError,"[503]");
					break;

				case 200: 
					bSeveralError=FALSE;
					*petError=GE_NONE;
					break;

				default:
#ifdef EH_CONSOLE
					printf("_googleCall(): %d:[%s]",iResponse,psWeb->pData);
					_getch();
#endif
					bSeveralError=TRUE; 
					break;

			}
			
			if (bSeveralError) break;

			pFraseTradotta=jsonGet(psJson,"responseData.translatedText");
			if (!pFraseTradotta) {
				webHttpReqFree(psWeb);
				bSeveralError=TRUE; 
				break;
			}
			
			putfRet=strDup(pFraseTradotta);
			psJson=jsonDestroy(psJson);
//			FWebGetFree(&sWS);
			webHttpReqFree(psWeb);

			// Attendo
			if (!bSeveralError||bExit) break; // Tradotta
			ehSleep(1000); 

		} 

	} // Tentativi (t loop)

	//
	// Il server non risponde (o non risponde in tempo utile) > mi fermo (non analizzo le altri frasi
	//
	if (bSeveralError) {
		printf("GoogleCall: %s" CRLF,pszLastError); 
	}
	ehFreePtr(&pszLastError);
	*piRequestDone=iRequestDone;
	ehFree(pPage);
	if (putfRet) ehSleep(2000); 
	return putfRet;
}



//
// _bingCall (Richiesta a Google della traduzione di una frase)
//
static UTF8 * _bingCall(UTF8 * pszSource,
						CHAR * langSource,
						CHAR * langDest,
						CHAR * pszApiKey,
						INT * piRequestDone,
						ET_ERROR * petError)
{
	BYTE * pszLastError;
	BYTE * psz;
	BYTE * pPage=ehAlloc(64000);
	CHAR * pszSrc;
	CHAR * putfRet=NULL;
//	CHAR szServ[200];
	EH_WEB *psWeb;
	EH_AR	arTrans=NULL;

	INT iRequestDone=0;
	if (!piRequestDone) piRequestDone=&iRequestDone; else iRequestDone=*piRequestDone;
	if (strEmpty(pszApiKey)) ehExit("Non e' indicata la bingAppId");

	sprintf(pPage,"http://api.microsofttranslator.com/V2/Ajax.svc/Translate?appId=%s&from=%s&to=%s&text=",pszApiKey,langSource,langDest);

	// Effetto un pre-encoding di < e >
	pszSrc=ehAlloc(64000);
	strcpy(pszSrc,pszSource);

	// Sostituisco tag {} con 
	/*
	arTrans=ARNew();
	while (true) {
		CHAR * psz,* pszVal;
		CHAR szServ[]="<%/>";	
		psz=strExtract(pszSrc,"{","}",false,true); if (!psz) break;
		pszVal=strExtract(psz,"{","}",false,false);
		ARAddarg(&arTrans,"%s=%s",psz,szServ);
		strReplace(pszSrc,psz,szServ);
		ehFreePtrs(2,&pszVal,&psz);

	}

*/
	while (strReplace(pszSrc,"<","&lt;"));
	while (strReplace(pszSrc,">","&gt;"));

//	while (strReplace(pszSrc,"}","\1"));
	psz=strEncode(pszSrc,SE_URL,NULL);
	strcat(pPage,psz);
	ehFree(psz);

	//
	// Chiamata a Bing 
	//
	*petError=GE_NONE;
	pszLastError=strDup("");
	psWeb=webHttpReq(pPage,"GET",NULL,FALSE,0); iRequestDone++;

	if (psWeb->sError.enCode)
	{
//		bSeveralError=TRUE; 
		strAssign(&pszLastError,psWeb->sError.szDesc);
	}
	else
	{
		CHAR  *psz;
		INT a;
		psz=strstr(psWeb->pData,"\"");
		if (psz) {

			psz++; psz[strlen(psz)-1]=0;
			//
			// (Errore) ï»¿"ArgumentException: Invalid appId\u000d\u000aParameter name: appId : ID=3835.V2_Json.Translate.2556659A
			//			ï»¿"Ã‰lectronique: VHF stÃ©rÃ©o ECHO-CD-LOG graphique traceur TV SOCKET
			//
#ifdef _DEBUG
//			printf("> %s" CRLF,psz);
#endif
			putfRet=strEncodeEx(1,psz,2,SD_JSON,SE_UTF8);
			if (putfRet) {

				// Reswapping ...
				strcpy(pszSrc,putfRet); ehFree(putfRet); 
				if (arTrans) {
					for (a=0;arTrans[a];a++) {
						EH_AR ar=ARFSplit(arTrans[a],"=");
						if (strstr(pszSrc,ar[1]))
							strReplace(pszSrc,ar[1],ar[0]);
							else
							strcat(pszSrc,ar[0]);
						ehFree(ar);
					}
				}
				putfRet=strDup(pszSrc);

				if (strstr(putfRet,"TranslateApiException")||
					strstr(putfRet,".V2_Json.")) {
					*petError=GE_ABUSE;
					printf("- %s" CRLF,putfRet);
					ehFreePtr(&putfRet);
				
				}

				else if (!strBegin(putfRet,"ArgumentException: ")) {
					*petError=GE_SERVER_ERROR;
					printf("- %s" CRLF,putfRet);
					ehFreePtr(&putfRet);
//					_getch();
//						break;
				}
			}

		
		} else {
			
			*petError=GE_SERVER_ERROR;
		}

	} // else
	webHttpReqFree(psWeb);	

	*piRequestDone=iRequestDone;
	ehFree(pPage);
	if (putfRet) ehSleep(300); 
	ehFreePtr(&pszLastError);
	ehFree(pszSrc);

	if (arTrans) ARDestroy(arTrans);
	return putfRet;
}

static INT _tokControl(CHAR *psz,CHAR *pszChar)
{
	INT a,iLen;
	
	EH_AR ar=ARCreate(psz,pszChar,&iLen);
	for (a=0;ar[a];a++)
	{
		strTrim(ar[a]);
		if (strlen(ar[a])>GOOGLE_MAX_LENGTH) 
			{iLen=0; break;}
	}
	ARDestroy(ar);
	return iLen;
}

static BOOL _isNaN(BYTE *pString) // new 2009
{
 BYTE *p;
 for (p=pString;*p;p++) 
 {
	 if (*p==' '||*p=='\t'||*p=='\n'||*p=='\r') continue;
//	 if ((*p=='.')&&!fPoint) {fPoint=TRUE; continue;}
	 if ((*p<'0')||(*p>'9')) return TRUE;
 }
 return FALSE;
}
