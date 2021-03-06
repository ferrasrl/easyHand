// -------------------------------------------------------------
// GoogleTrans.c
// Traduzioni via Google Translator
// 
// Ferrà srl 02/2010
// -------------------------------------------------------------

#include "/easyhand/inc/easyhand.h" 
#include "/easyhand/inc/GoogleTrans.h" 
#include "/easyhand/ehtool/webtool.h"
#include <time.h>
//
// GoogleTranslate()
// traduce il testo passato nella lingua richiesta grazie a Google
//
#define GOOGLE_MAX_LENGTH 1024
static BOOL L_GoogleMaxSize(EH_AR arPhrase);
static BYTE * _googleCall(WCHAR *pwString,BYTE *langSource,BYTE *langDest,BYTE *pszApiKey,BYTE *pszUserIp,SINT *piRequestDone,SINT *piErrorReturn);

WCHAR * GoogleTranslate(WCHAR *pwSource,CHAR *pszLangSource,CHAR *pszLangDest,BOOL bShowError,BYTE *pszApiKey,BYTE *pszUserIp,SINT *piRequestDone,BOOL bDetect,EN_GOOGLE_ERROR *piError,
						void * (* extCacheTrans)(EN_MESSAGE enMess,LONG lParam,void *pVoid))
{
	CHAR *pUtf,*pRet;
	WCHAR *pwRet;
	pUtf=strEncodeW(pwSource,SE_UTF8,NULL);
	pRet=GoogleTranslateUtf8(pUtf,pszLangSource,pszLangDest,bShowError,pszApiKey,pszUserIp,piRequestDone,bDetect,piError,extCacheTrans); ehFree(pUtf);
	pwRet=strDecode(pRet,SE_UTF8,NULL); ehFree(pRet);
	return pwRet;
}

SINT L_TokControl(CHAR *psz,CHAR *pszChar)
{
	SINT a,iLen;
	
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

BOOL _isNaN(BYTE *pString) // new 2009
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


CHAR * GoogleTranslateUtf8(CHAR *	pSourceUtf8,
						   CHAR *	langSource,
						   CHAR *	langDest,
						   BOOL		bShowError,		// T/F se devo mostrare gli errori
						   BYTE *	pszApiKey,		// ApiKey (non è strettamente necessaria)
						   BYTE *	pszUserIp,		// Ip da comunicare a Google end-user (non è strettamente necessario)
						   SINT *	piRequestDone, // Numero di richieste fatte
						   BOOL		bDetectLangSource,
						   EN_GOOGLE_ERROR	*	piError,
						   void * (* extCacheTrans)(EN_MESSAGE enMess,LONG lParam,void *pVoid))
{
	WCHAR *pw;
	BYTE *pTradotto=NULL;
	EH_AR arPhrase;
	SINT f,iNum;
	BOOL bError=FALSE;
	S_GTAGENC *psTag;
	SINT iLen,iPart;
	SINT iCharsBad;
	BYTE *pCharBreak=NULL;
	BYTE *pUtf;
	CHAR szLangSource[20];
	double dRap;
	SINT iPhraseProblems;
	SINT iPhraseTotal;
	SINT iWords;
	SINT iErrorReturn;

	if (piRequestDone) *piRequestDone=0;
	if (piError) *piError=GE_NONE; // L'indicazione della lingua sorgente è errata

	//
	// Se ho cache esterna, ricerco prima in cache
	//
	strTrim(pSourceUtf8);
	if (extCacheTrans) {

		S_TRANSLATION sTrans;

		sTrans.pszLangSource=langSource;
		sTrans.pszLangDest=langDest;
		sTrans.utfSource=pSourceUtf8;
		sTrans.utfDest=NULL;
		if (extCacheTrans(WS_FIND,0,&sTrans)) {
			return sTrans.utfDest;
		}
	
	}


	//
	// Ricodifico i Tag Sensibili
	//
	psTag=GTagEncode(pSourceUtf8,GTM_GOOGLE);

	//
	// Spezzo la frase (se è il caso)
	//
	
	if (strlen(psTag->pszSourceTagged)<GOOGLE_MAX_LENGTH)
	{
		arPhrase=ARNew();
		ARAdd(&arPhrase,psTag->pszSourceTagged);
		iNum=1;
	}
	else
	{
		// Array con 
		CHAR *arDiv[]={".",CRLF,"\r","\n",";"," - ","-",",",NULL};
		SINT x;
		pCharBreak=NULL;
		iPart=100000;
		for (x=0;arDiv[x];x++)
		{
			f=L_TokControl(psTag->pszSourceTagged,arDiv[x]);
			if (f) 
			{
				if (f<iPart) 
				{
					pCharBreak=arDiv[x];	
					iPart=f;
				}
			}
		}

		//
		// Non riesce a spezzare la stringa > mi fermo
		//
		if (!pCharBreak) 
		{
//			printf("[%s]",psTag->pszSourceTagged);
//			_getch();
			pTradotto=GTagDecode(psTag,NULL,FALSE,TRUE); 
			ehFreePtr(&pTradotto);
			*piError=GE_STRING_TOO_LONG;
			printf(">");
			return pTradotto;
		}
		arPhrase=ARCreate(psTag->pszSourceTagged,pCharBreak,&iNum);
	}

#ifdef EH_CONSOLE
	if (iNum>1) printf("(%d parti)",iNum);
#endif

	iPhraseTotal=ARLen(arPhrase);
	iPhraseProblems=0; 
	for (f=0;arPhrase[f];f++)
	{
		strTrim(arPhrase[f]); 
		if (strEmpty(arPhrase[f])) continue;
		iLen=strlen(arPhrase[f]);  if (iLen<2) continue;

#ifdef _CONSOLE
		if (iLen>GOOGLE_MAX_LENGTH) 
		{
			printf("[%d][%s]",iLen,arPhrase[f]);
			_getch();
		}
#endif
		
		//
		// Controllo se è un numero
		//
		if (!_isNaN(arPhrase[f])) continue;

		//
		// Conto le parole
		//
		iWords=0; iCharsBad=0; dRap=0;
		if (bDetectLangSource)
		{
			iWords=strWordCount(arPhrase[f]);

			// Precontrollo stringa
			// Proporzione: iCharsBad:iLen=x:100
			iCharsBad=strCount(arPhrase[f],".()[],-*");
			dRap=iCharsBad*100/iLen;
			if (dRap>50) // Frase composta da oltre il 50% di simboli e non di lettere
			{
				*piError=GE_STRING_SUSPECT; // L'indicazione della lingua sorgente è errata
				continue;
			}
		}

		//
		// Se ho cache esterna, ricerco prima in cache
		//
		pUtf=NULL;
		if (extCacheTrans&&strcmp(arPhrase[f],pSourceUtf8)) {

			S_TRANSLATION sTrans;

			sTrans.pszLangSource=langSource;
			sTrans.pszLangDest=langDest;
			sTrans.utfSource=arPhrase[f];
			sTrans.utfDest=NULL;
			if (extCacheTrans(WS_FIND,0,&sTrans)) {
				pUtf=sTrans.utfDest;
			}
		
		}
		
		if (extCacheTrans) {
		
			pszUserIp=extCacheTrans(WS_REALGET,0,NULL);
		}

		//
		// Chiedo al web <--------------
		//

		if (!pUtf) {

			//
			// Compongo indirizzo URL con i dati
			//
			pw=strDecode(arPhrase[f],SE_UTF8,NULL); 
			if (!pw) ehError();

			// da utf8> unicode e porto in minuscolo, ho visto che traduce meglio
			_wcslwr(pw); 
			// Controllo le prime 3 frasi
			if (bDetectLangSource)
			{
				*szLangSource=0;

			} else strcpy(szLangSource,langSource);

			//
			// Se non ho risultato ricerco con Google
			//
			pUtf=_googleCall(	pw,
								szLangSource,
								langDest,
								pszApiKey,
								pszUserIp,
								piRequestDone,
								&iErrorReturn); 
			ehFree(pw);
		}

		if (bDetectLangSource)
		{

			if (iErrorReturn==400)
			{
				*piError=GE_LANG_SOURCE;
				iPhraseProblems++;
				ehFree(pUtf);
				continue;
			}

			if (strstr("|fr|en|de|",szLangSource)&&
				dRap<30&&
				strcmp(szLangSource,langSource))
			{
				if (iWords>2)
				{
					*piError=GE_LANG_SOURCE;
				}
				else
				{
					if (!*piError) *piError=GE_STRING_SUSPECT; // L'indicazione della lingua sorgente è errata
				}
				iPhraseProblems++;
			}
		}

		if (iErrorReturn!=200) 
		{
			bError=TRUE; 
			if (piError) 
			{
				*piError=GE_SERVER_ERROR; // L'indicazione della lingua sorgente è errata
				if (iErrorReturn==403) *piError=GE_ABUSE;
			}
			break;
		}
	
		//
		// Traduzione andata a buon fine - Update della cache personale
		//
		if (extCacheTrans) {
		
			S_TRANSLATION sTrans;

			sTrans.pszLangSource=langSource;
			sTrans.pszLangDest=langDest;
			sTrans.utfSource=arPhrase[f];
			sTrans.utfDest=pUtf;
			extCacheTrans(WS_UPDATE,0,&sTrans);

		}



		strAssign(&arPhrase[f],pUtf);
		ehFree(pUtf);

		printf(".");
		ehSleep(2000);
	} // Loop sulle frasi 

	if (iPhraseProblems&&!bError)
	{
		dRap=iPhraseProblems*100/iPhraseTotal;
		// Sotto il 10% di problemi sull'intera frase resetto l'errore
		if (dRap<10)
		{
			if (piError) *piError=GE_NONE; // L'indicazione della lingua sorgente è errata
		}
	}

	if (!bError) 
	{
		if (pCharBreak) 
			pTradotto=ARToString(arPhrase,pCharBreak,"",""); 
			else
			pTradotto=strDup(arPhrase[0]);

		pTradotto=GTagDecode(psTag,pTradotto,TRUE,TRUE); 
	}
	else 
	{
		pTradotto=GTagDecode(psTag,NULL,FALSE,TRUE); 
		ehFreePtr(&pTradotto);
		printf("!");
	}
	ARDestroy(arPhrase);
	
	return pTradotto;
}

//
// TagEncode()
// Riconosco ed estraggo i tag "sensibili" e gli inserisco in un array, 
// sostitituendoli con {1}
//
S_GTAGENC * GTagEncode(CHAR *pSourceUtf8,SINT iMode) 
{
	S_GTAGENC *psTag;
	BYTE *pUtf,*p;
	CHAR szServ[80];
	psTag=ehAllocZero(sizeof(S_GTAGENC));
	psTag->arTag=ARNew();
	psTag->pszSourceTagged=strDup(pSourceUtf8);
	pUtf=ehAlloc(strlen(pSourceUtf8)*10); strcpy(pUtf,pSourceUtf8);

	while (TRUE) {

		// Witango
		p=strExtract(pUtf,"@@"," ",FALSE,TRUE);	if (p) {p[strlen(p)-1]=0;}
		if (!p) p=strExtract(pUtf,"@@",NULL,TRUE,TRUE); 
		if (!p) p=strExtract(pUtf,"<@",">",TRUE,TRUE);
		// Tag C
		if (!p) {p=strstr(pUtf,"%d"); if (p) p=strDup("%d");}
		if (!p) {p=strstr(pUtf,"%s"); if (p) p=strDup("%s");}
		if (!p) p=strExtract(pUtf,"<",">",TRUE,TRUE); // Tag Html
		
		if (p) 
		{
			switch (iMode)
			{
				case GTM_GOOGLE:
					sprintf(szServ,"<tag value='%d' />",ARLen(psTag->arTag));
					break;

				default:
					sprintf(szServ,"{%d}",ARLen(psTag->arTag));
					break;
			}
			ARAddarg(&psTag->arTag,"%s\1%s",szServ,p);

			while (strReplace(szServ,"<","[["));
			while (strReplace(szServ,">","]]"));
			while (strReplace(pUtf,p,szServ));
			psTag->dwSizeTag+=strlen(p);
			ehFree(p);
		} else break;	
	}
	// replace del mio tag
	while (strReplace(pUtf,"[[","<"));
	while (strReplace(pUtf,"]]",">"));

	strAssign(&psTag->pszSourceTagged,pUtf);
	ehFree(pUtf);
	return psTag;
}

//
// TagDecode
//
CHAR *GTagDecode(S_GTAGENC *psTagEnc,CHAR *pszSourceUtf8,BOOL bFreeSource,BOOL bFreeTag)
{
	SINT a;
	BYTE *pUtf=NULL;
	CHAR *pszRet=NULL;
	EH_AR ar;
	SINT iTags=ARLen(psTagEnc->arTag);
	if (iTags)
	{
		if (pszSourceUtf8)
		{
			pUtf=ehAlloc(strlen(pszSourceUtf8)+psTagEnc->dwSizeTag+50); 
			strcpy(pUtf,pszSourceUtf8);

			for (a=0;psTagEnc->arTag[a];a++)
			{
				ar=ARCreate(psTagEnc->arTag[a],"\1",NULL);
				while (strReplace(pUtf,ar[0],ar[1]));
				ARDestroy(ar);
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
		GTagFree(psTagEnc);
	}

	if (bFreeSource) ehFree(pszSourceUtf8);
	return pszRet;
}

void GTagFree(S_GTAGENC *psTagEnc)
{
	psTagEnc->arTag=ARDestroy(psTagEnc->arTag);
	ehFreePtr(&psTagEnc->pszSource);
	ehFreePtr(&psTagEnc->pszSourceTagged);
	ehFree(psTagEnc);
}


static BOOL L_GoogleMaxSize(EH_AR arPhrase)
{
	SINT f;
	SINT iLen;

	// Precontrollo 1024
	for (f=0;arPhrase[f];f++)
	{
		strTrim(arPhrase[f]); if (strEmpty(arPhrase[f])) continue;
		iLen=strlen(arPhrase[f]); 
		if (iLen>GOOGLE_MAX_LENGTH)  return TRUE;
	}
	return FALSE;
}

//
// GoogleCall (Richiesta a Google della traduzione di una frase)
//
static BYTE * _googleCall(WCHAR *pwString,
						   BYTE *langSource,
						   BYTE *langDest,
						   BYTE *pszApiKey,
						   BYTE *pszUserIp,
						   SINT *piRequestDone,
						   SINT *piErrorReturn)
{
	BYTE *pszLastError;
	CHAR szServ[200];
	FWEBSERVER sWS;
	BYTE *psz;
	SINT iWebError;
	BYTE *pPage=ehAlloc(64000);
	SINT t;
	BOOL bSeveralError=FALSE;
	BYTE *putfRet=NULL;
	S_SEN *psSen;
	BOOL bExit=FALSE;
	

	SINT iRequestDone=0;
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
	*piErrorReturn=0;
	pszLastError=strDup("");
	bSeveralError=FALSE;
	for (t=0;t<5;t++)
	{
		ZeroFill(sWS);
		sWS.lpUri=pPage;
		sWS.keep_alive=TRUE;

		printf("> REQ: [%s]" CRLF,pPage);
		iWebError=FWebGetDirect(&sWS,"GET",NULL); iRequestDone++;
//		printf(CRLF "< RES: %d:%d:[%s]" CRLF,sWS.iWebGetError,iWebError,sWS.pData);

		if (iWebError)
		{
#ifdef EH_CONSOLE
			printf("(!%d)?",t+1); 
#endif	
			FWebGetFree(&sWS);
			bSeveralError=TRUE; 
			strAssign(&pszLastError,ehWebError(sWS.iWebGetError));
			break;
		}
		else
		{
			EH_JSON * psJson;
			BYTE *pResponse;
			BYTE *pDetected;
			BYTE *pFraseTradotta;
			SINT iResponse;

			psJson=jsonCreate(sWS.pData);
//			jsonPrint(psJson,0);
 
//			while (TRUE)  
		
			strAssign(&pszLastError,sWS.pData);

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
				strAssign(&pszLastError,jsonGet(psJson,"responsDetails"));
//				_getch();

			}
			switch (iResponse)
			{
				case 400:
					bSeveralError=TRUE;
					bExit=TRUE;
//						strAssign(&pszLastError,"[Lingua sconosciuta:400]");
					break;
				
				case 403: // Abuso
#ifdef EH_CONSOLE
						printf("(Abuse!%d)?",t+1); 
#endif	
					bSeveralError=TRUE; 
//						strAssign(&pszLastError,"[Abuse:403]");
					break;

				case 404: // Invalid result data
					printf("[404]");
					bSeveralError=TRUE; 
//						strAssign(&pszLastError,"[Abuse:404]");
					break;

				case 503: // Non lo so
					printf("[503]");
					bSeveralError=TRUE; 
//						strAssign(&pszLastError,"[503]");
					break;

				case 200: 
					bSeveralError=FALSE;
					break;

				default:
#ifdef EH_CONSOLE
					printf("_googleCall(): %d:[%s]",iResponse,sWS.pData);
					_getch();
#endif
					bSeveralError=TRUE; 
					break;

			}
			*piErrorReturn=iResponse;
			if (bSeveralError) break;

			pFraseTradotta=jsonGet(psJson,"responseData.translatedText");
//				pFraseTradotta=strExtract(sWS.pData,"translatedText\":\"","\",",FALSE,FALSE);
// 				if (!pFraseTradotta) pFraseTradotta=strExtract(sWS.pData,"translatedText\":\"","\"}",FALSE,FALSE);
			if (!pFraseTradotta) 
			{
				bSeveralError=TRUE; break;
			}
			
			putfRet=strDup(pFraseTradotta);//strEncodeEx(1,pFraseTradotta,2,SD_JSON,SE_UTF8); 
//			ehFree(pFraseTradotta);
			psJson=jsonDestroy(psJson);
			FWebGetFree(&sWS);
			if (!bSeveralError||bExit) break; // Tradotta

			// Attendo un secondo e ci riprovo
			Sleep(1000); 
			
		} // else

	} // Tentativi (t loop)

	//
	// Il server non risponde (o non risponde in tempo utile) > mi fermo (non analizzo le altri frasi
	//
	if (bSeveralError)
	{
		printf("GoogleCall: %s",pszLastError); 

	}
	ehFreePtr(&pszLastError);
	*piRequestDone=iRequestDone;
	ehFree(pPage);

	if (putfRet) ehSleep(1000);
	return putfRet;
}