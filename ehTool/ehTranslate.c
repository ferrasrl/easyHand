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


static struct {

	BOOL	bReady;
	EH_LST	lstAzureToken;

} _s={false};

static void _transEndPoint(BOOL x) {
	if (!_s.bReady) return;
	if (_s.lstAzureToken) _s.lstAzureToken=lstDestroy(_s.lstAzureToken);
}

static CHAR *  _azureTokenRequest(ET_PARAM	enParam,CHAR * pszJsonString) {

	EH_WEB *psWeb;
//	CHAR * pszTranslateParams=pszJsonString;
	if (!_s.lstAzureToken) _s.lstAzureToken=lstNew(); // Creo la prima volta la lista

	//
	// Richiedo 'azureToken' a 'mamma' Microsoft
	//
	if (strstr(pszJsonString,"arAzureClient")) {

		CHAR * pszAppTranslatorUrl="https://api.datamarket.azure.com/Bing/MicrosoftTranslator/v1/Translate";
		CHAR * pszScope="http://api.microsofttranslator.com";
		CHAR * pszGrantType="client_credentials";
		CHAR * pszAuthUrl="https://datamarket.accesscontrol.windows.net/v2/OAuth2-13";

		//
		// https://datamarket.azure.com/developer/applications/
		//
		EH_LST lst;
		CHAR * pszUrl;
		S_SEN * psSen;
		INT a,iLen;
		EH_JSON * psJson=jsonCreate(pszJsonString);
//		EH_LST lstToken=lstNew();
		
		lstClean(_s.lstAzureToken);

		if (psJson->enType==JSON_ERROR) ehError();
		iLen=atoi(jsonGet(psJson,"arAzureClient.length"));
		for (a=0;a<iLen;a++) {
	
			CHAR * psz=jsonGetf(psJson,"arAzureClient[%d]",a);
			EH_AR ar;

			if (strEmpty(psz)) continue;
			ar=strSplit(psz,"|");

			psSen=senCreate();
			lst=lstNew();
			lstPushf(lst,"%s?",pszAuthUrl);
			lstPushf(lst,"grant_type=%s",senEncode(psSen,SE_URL,pszGrantType));
			lstPushf(lst,"&scope=%s",pszScope);//senEncode(psSen,SE_URL,pszScope));
			lstPushf(lst,"&client_id=%s",senEncode(psSen,SE_URL,ar[0]));
			lstPushf(lst,"&client_secret=%s",senEncode(psSen,SE_URL,ar[1]));
			senDestroy(psSen);
			pszUrl=lstToString(lst,"","","");
			psWeb=webHttpReq(pszUrl,"POST",NULL,FALSE,60);
			if (enParam&ET_SERVICE_CALL) printf(">azureToken:");
			if (!psWeb->sError.enCode) { // Leggi pagina caricata 
			
				EH_JSON * psJson;
				psJson=jsonCreate(psWeb->pData);
				psz=jsonGet(psJson,"access_token");
				lstPush(_s.lstAzureToken,strEver(psz));
				jsonDestroy(psJson);
				if (enParam&ET_SERVICE_CALL) printf("ok." CRLF);

			} else {
				lstPush(_s.lstAzureToken,"");
				printf("\7Azure Error: Account %s accesso error" CRLF,ar[0]);
				if (enParam&ET_SERVICE_CALL) printf("ko!" CRLF);

			}
			ehFree(ar);
			webHttpReqFree(psWeb);
			ehFree(pszUrl);
			lstDestroy(lst);
		}

		//
		// Aggiungo Token di accesso Trovati
		//
		/*
		if (lstToken->iLength) {
			CHAR * psz=lstToString(lstToken,","," ,arAzureToken:[","]"); xx
			pszTranslateParams[strlen(pszTranslateParams)-1]=0;
			strCat(&pszTranslateParams,psz);
			strCat(&pszTranslateParams,"}");
			ehFree(psz);
		}
		*/

		jsonDestroy(psJson);
//		lstDestroy(lstToken);
	}

//	ehFree(pszJsonString);
	return pszJsonString;
}

//
// ehTranslateStart()
//
	/*

CHAR * ehTranslateStart(ET_PARAMenParam,CHAR * pszJsonString) {

	CHAR * pszTranslateParams;//=strDup(pszJsonString);
	if (!_s.bReady) {
		
		_(_s.bReady);
		_s.lstAzureToken=lstNew();

	}
	pszTranslateParams=_azureTokenRequest(enParam,pszJsonString);
	//
	// Richiedo 'azureToken' a 'mamma' Microsoft
	//
	if (strstr(pszTranslateParams,"arAzureClient")&&
		!strstr(pszTranslateParams,"arAzureToken")) {

//		CHAR * pszClient_id="in-cloud";
//		CHAR * pszClientSecret="Nm99KFev3kzFmzTIhahv90hIXFDHy8prlui1JOk1ev8=";

		CHAR * pszAppTranslatorUrl="https://api.datamarket.azure.com/Bing/MicrosoftTranslator/v1/Translate";
		CHAR * pszScope="http://api.microsofttranslator.com";
		CHAR * pszGrantType="client_credentials";
		CHAR * pszAuthUrl="https://datamarket.accesscontrol.windows.net/v2/OAuth2-13";

		//
		// https://datamarket.azure.com/developer/applications/
		//
		EH_LST lst;
		CHAR * pszUrl;
		S_SEN * psSen;
		INT a,iLen;
		EH_JSON * psJson=jsonCreate(pszTranslateParams);
		EH_LST lstToken=lstNew();
		
		if (psJson->enType==JSON_ERROR) ehError();
		iLen=atoi(jsonGet(psJson,"arAzureClient.length"));
		for (a=0;a<iLen;a++) {
	
			CHAR * psz=jsonGetf(psJson,"arAzureClient[%d]",a);
			EH_AR ar;

			if (strEmpty(psz)) continue;
			ar=strSplit(psz,"|");

			psSen=senCreate();
			lst=lstNew();
			lstPushf(lst,"%s?",pszAuthUrl);
			lstPushf(lst,"grant_type=%s",senEncode(psSen,SE_URL,pszGrantType));
			lstPushf(lst,"&scope=%s",pszScope);//senEncode(psSen,SE_URL,pszScope));
			lstPushf(lst,"&client_id=%s",senEncode(psSen,SE_URL,ar[0]));
			lstPushf(lst,"&client_secret=%s",senEncode(psSen,SE_URL,ar[1]));
			senDestroy(psSen);
			pszUrl=lstToString(lst,"","","");


			psWeb=webHttpReq(pszUrl,"POST",NULL,FALSE,60);
			if (!psWeb->sError.enCode) { // Leggi pagina caricata 
			
				EH_JSON * psJson;
				psJson=jsonCreate(psWeb->pData);
				psz=jsonGet(psJson,"access_token");
				if (!strEmpty(psz)) lstPushf(lstToken,"'%s'",psz);
				jsonDestroy(psJson);

			} else {
			
				printf("\7Azure Error: Account %s accesso error" CRLF,ar[0]);

			}
			ehFree(ar);
			webHttpReqFree(psWeb);
			ehFree(pszUrl);
			lstDestroy(lst);
		}

		//
		// Aggiungo Token di accesso Trovati
		//
		if (lstToken->iLength) {
			CHAR * psz=lstToString(lstToken,","," ,arAzureToken:[","]");
			pszTranslateParams[strlen(pszTranslateParams)-1]=0;
			strCat(&pszTranslateParams,psz);
			strCat(&pszTranslateParams,"}");
			ehFree(psz);
		
		}

		jsonDestroy(psJson);
		lstDestroy(lstToken);
	}

	ehFree(pszJsonString);
	return pszTranslateParams;

}
	*/


// Interior: 3 CabaÃ±as, 1 baÃ±o, comedor tranf.  \u000d\u000aAccesorios: Toldos, cubierta, tapas de escotilla, Molinete de ancla elÃ©ctrico, kit de anclaje + ANS. incluso, kit de amarre, calefacciÃ³n Webasto (l\/m reacondicionado), batt. + batt. Caldera de agua caliente, N4 2009: herramientas, estaciÃ³n de piloto de viento tridata, raymarine plotter + A6000, imp radar, VHF, radio TV\u000d\u000aVELAS: vela mayor 3 (carbono, kevlar, crucero), genona 2 (kevlar, crucero), engrapar kevlar, OlÃ­mpico, SPI, 2 spi, gennaker, jeack lesy + copriranda anotaciones: conducir bombilla, Polo de spinnaker de carbono\u000d\u000aComodidades: dentro de 50 millas
//
// GoogleTranslate()
// traduce il testo passato nella lingua richiesta grazie a Google
//
#define GOOGLE_MAX_LENGTH 1024
typedef struct {
	
	BOOL	bTranslate;
	UTF8 *	pszStr;
	CHAR	szBreak[20];
	CHAR *	pszBefore;
	CHAR *	pszAfter;

} S_PHSTR;

typedef struct {

	S_TTE * psTag;
	//EH_AR	arPhrase;
	EH_LST	lstPhrase;
	INT		iPhraseProblems;
	INT		iPhraseToTrans;
//	CHAR	szCharBreak[20];
	INT		iMaxPhrases;
	INT		iSeveralError; // Frasti troppo lunghe

} S_PHRASES;


#define TAG_START "\5"
#define TAG_END "\6"

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

		case 3:
			sprintf(pszDest,TAG_START "%d" TAG_END,id);
			break;

		default:
			ehError();
	}
}
					//sprintf(szServ,"<%d>",ARLen(psTag->arTag));


// static BOOL L_GoogleMaxSize(EH_AR arPhrase);
static BYTE * _googleCall( ET_PARAM enParam, 
						   WCHAR * pwString,
						   CHAR  * langSource,
						   CHAR  * langDest,
						   CHAR  * pszApiKey,
						   CHAR  * pszUserIp,
						   INT  * piRequestDone,
						   ET_ERROR * petError);
static UTF8 * _bingCall(ET_PARAM enParam,
						UTF8 * pszSource,
						CHAR * langSource,
						CHAR * langDest,
						CHAR * pszApiKey,
						INT * piRequestDone,
						ET_ERROR * piError);

static UTF8 * _azureCall(ET_PARAM enParam,
						UTF8 * pszSource,
						CHAR * langSource,
						CHAR * langDest,
						CHAR * pszToken,
						INT * piRequestDone,
						ET_ERROR * piError);

static INT _tokControl(CHAR *psz,CHAR *pszChar);
static BOOL _isNaN(BYTE *pString); // new 2009
static S_PHRASES * _phraseDestroy(S_PHRASES * psPh);

//
// _phraseBreak() > RItorna True se non riesce a spezzare la stringa
//
BOOL _phraseBreak(S_PHRASES * psPh,CHAR * pszPhrase) {

	S_PHSTR sPhs;
	INT iPart;
	INT a,f;
	EH_AR ar;
	INT iLen;
	CHAR * p;
	CHAR szCharBreak[80]="";
	BOOL bTrans=false;

	if (strEmpty(pszPhrase)) return false;

	for (p=pszPhrase;*p;p++) {
		if (*p>' ') {bTrans=true; break;}
	}

	// Solo caratteri di controllo
	if (!bTrans) {
		_(sPhs);
		sPhs.bTranslate=false; 
		sPhs.pszStr=strDup(pszPhrase);
		lstPush(psPh->lstPhrase,&sPhs);
		return false;
	
	}

	if (strlen(pszPhrase)<GOOGLE_MAX_LENGTH)
	{
		_(sPhs);
		sPhs.bTranslate=true; 
		sPhs.pszStr=strDup(pszPhrase);
		lstPush(psPh->lstPhrase,&sPhs);
		psPh->iPhraseToTrans++;
	}
	else
	{
		// Array con 
		CHAR *arDiv[]={".",CRLF,"\r","\n",";"," - ","-",",",NULL};//"(",")",NULL};
		INT x;
		
		iPart=100000;
		for (x=0;arDiv[x];x++)
		{
			f=_tokControl(psPh->psTag->pszSourceTagged,arDiv[x]);
			if (f) 
			{
				if (f>psPh->iMaxPhrases&&psPh->iMaxPhrases) 
					continue;
				if (f<iPart) 
				{
//					pCharBreak=arDiv[x];	
					strcpy(szCharBreak,arDiv[x]);
					iPart=f;
					if (psPh->psTag->bHtml) 
						break;
				}
			}
		}

		//
		// Non riesce a spezzare la stringa > mi fermo
		//
 		if (!*szCharBreak) 
		{
			//_phraseDestroy(psPh);
			psPh->iSeveralError++;
			return true; // = GE_STRING_TOO_LONG
		}
		ar=strSplit(psPh->psTag->pszSourceTagged,szCharBreak);
		iLen=ARLen(ar);
		for (a=0;ar[a];a++) {
		
			_(sPhs);
			sPhs.bTranslate=true; 
			if (iLen<a-1) strcpy(sPhs.szBreak,szCharBreak);
			sPhs.pszStr=strDup(ar[a]);
			lstPush(psPh->lstPhrase,&sPhs);
			psPh->iPhraseToTrans++;
		}
		ehFree(ar);

	}
	return false;
}

//
// _phraseCreate > Crea la lista delle frasi da tradurre
//

S_PHRASES * _phraseCreate(UTF8 * pszSource,INT iMaxPhrases) { //,EN_TTE enTech) {

	S_PHRASES * psPh=ehAllocZero(sizeof(S_PHRASES));
	CHAR * p;
	BOOL bFirst=true;

	//
	// Ricodifico i Tag Sensibili
	//
	psPh->psTag=tagEncode(pszSource);
	psPh->iMaxPhrases=iMaxPhrases;

	//
	// Estraggo Tag (da non tradurre) e Frasi (spezzate se necessario)
	//
	psPh->lstPhrase=lstCreate(sizeof(S_PHSTR));//ARNew();
	for (p=psPh->psTag->pszSourceTagged;*p;p++) {
	
		CHAR * pszOpen=strstr(p,TAG_START);
		S_PHSTR sPh;

		if (pszOpen) {
			CHAR * pszClose=strstr(pszOpen,TAG_END);

			//
			// Se c'è prende il primo pezzo tra l'inizio e il primo TAG di seperazione
			//
			if (bFirst&&(pszOpen-1)>psPh->psTag->pszSourceTagged) {
			
				CHAR * psz;
				psz=strTake(psPh->psTag->pszSourceTagged,pszOpen-1);
				if (!strEmpty(psz)) 
					_phraseBreak(psPh,psz);
				ehFree(psz); 
			}
			bFirst=false;

			if (pszClose) {

				//
				// Alloca il tag di separazione
				//
				_(sPh);
				sPh.bTranslate=false;
				sPh.pszStr=strTake(pszOpen,pszClose);
				lstPush(psPh->lstPhrase,&sPh); 

				//
				// Cerco il pezzo dopo (spezzo la stringa)
				//
				pszOpen=strstr(pszClose+1,TAG_START);
				if (pszOpen) {
				
					CHAR * psz;
					psz=strTake(pszClose+1,pszOpen-1);
					if (!strEmpty(psz)) 
						_phraseBreak(psPh,psz);
					p=pszClose+strlen(psz);
					ehFree(psz);

				} else { // Ultima stringa

					_phraseBreak(psPh,pszClose+1);
					p=pszClose+strlen(pszClose+1);
				}
				
			}
		} else {
			
			_phraseBreak(psPh,p);
			break;		
		}

	}

//
//	psPh->iPhraseTotal=ARLen(psPh->arPhrase);
	if (psPh->iSeveralError) {
		psPh=_phraseDestroy(psPh);
	} else {
	
		S_PHSTR * psPhs;
	//
	// Post controlli, creo testa e coda
	//	
		for (lstLoop(psPh->lstPhrase,psPhs)) {
	
			CHAR * pszStart=NULL;
			CHAR * pszEnd=NULL;
			BOOL bReplace=false;
			#define JUMP_CHARS ",.-()*#/\\~$;:[]{}@=|" // Salto questi carattari prima e dopo il testo per cui è richiesta la traduzione

			if (!psPhs->bTranslate) continue;
			if (strEmpty(psPhs->pszStr)) {psPhs->bTranslate=false; continue;}
			if (strlen(psPhs->pszStr)<2) {psPhs->bTranslate=false; continue;}

			//
			// Estraggo testa della frase
			//
			pszStart=psPhs->pszStr;
			for (p=psPhs->pszStr;*p;p++) {
				if (*p>' '&&!strchr(JUMP_CHARS,*p)) break;
			}
			if (p>psPhs->pszStr) {bReplace=true; psPhs->pszBefore=strTake(psPhs->pszStr,p-1); pszStart=p;}

			//
			// Estraggo coda della frase
			//
			pszEnd=pszStart+strlen(pszStart)-1;
			for (p=pszEnd;p>=pszStart;p--) {
				if (*p>' '&&!strchr(JUMP_CHARS,*p)) break;
			}
			if (p!=pszStart) 
			{
				bReplace=true; 
				psPhs->pszAfter=strTake(p+1,pszEnd); 
				pszEnd=p;
			}
			if (bReplace) {
				p=strTake(pszStart,pszEnd);
				strAssign(&psPhs->pszStr,p);
				ehFreeNN(p);
			
			}
		}

	
	}
	return psPh;
}


static S_PHRASES * _phraseDestroy(S_PHRASES * psPh) {

	CHAR * pTradotto;
	S_PHSTR * psPhs;

	pTradotto=tagDecode(psPh->psTag,NULL,FALSE,TRUE); 
	ehFreePtr(&pTradotto);
	for (lstLoop(psPh->lstPhrase,psPhs)) {
		ehFreeNN(psPhs->pszStr);
		ehFreeNN(psPhs->pszAfter);
		ehFreeNN(psPhs->pszBefore);
	}
	psPh->lstPhrase=lstDestroy(psPh->lstPhrase);

	ehFree(psPh);

	return NULL;

}

static BOOL _isBlackList(EH_LST lstBlackKey,ET_TECH enTech,CHAR * pszAppId) {

	ET_BLACK_ITEM * psItem;
	
	if (!lstBlackKey) return false;

	for (lstLoop(lstBlackKey,psItem)) {
		if (psItem->enTech==enTech&&!strcmp(psItem->pszAppId,pszAppId)) return true;
	}
	return false;

}

//
// ehTranslate()
//
UTF8 * ehTranslate(	ET_PARAM	enParam,
					UTF8 *		pszSource,
					CHAR *		pszLangSource,
					CHAR *		pszLangDest,
					CHAR *		pszJsonParams,	// {googlekey:"asda", bingkey:"asd"}
					ET_REPORT * psReport,
					void *		(* extAssist)(EN_MESSAGE enMess,LONG lParam,void *pVoid),
					EH_LST		lstBlackKey
					)

{
	WCHAR *pw;
	BYTE *pTradotto=NULL;
	S_PHRASES * psPh=NULL;
	S_PHSTR * psPhs;
	//INT f;
	BOOL bError=FALSE;
	INT iLen;
	INT iCharsBad;
	BYTE *pUtf;
	CHAR szLangDetect[20]="";
	double dRap;
	INT iWords;
	ET_REPORT sReportTemp;
	EH_JSON * psJson=NULL;
	CHAR * pszUserIp=NULL;
	INT		iMaxPhrases=0;
	ET_BLACK_ITEM sBlack;
	
	if (lstBlackKey) {
	
		if (lstBlackKey->iSize!=sizeof(ET_BLACK_ITEM)) ehExit("Usare transBlackListCreate();");
	}

	if (!_s.bReady) {_(_s); _s.bReady=true; ehAddExit(_transEndPoint);} // <-- Resetto la struttura Privata

	if (!psReport) psReport=&sReportTemp;
	memset(psReport,0,sizeof(ET_REPORT));

	if (!(enParam&ET_TRY_LOCAL)) extAssist=NULL;

	if (!pszLangSource) ehError();
	strcpy(psReport->szLangSource,pszLangSource);
	if (!pszLangDest) ehError();
	strcpy(psReport->szLangDest,pszLangDest);

	//
	// Assistente: ricerco prima in cache la frase completa
	//
	strTrim(pszSource);
	if (!*pszSource) {
		return strDup("");
	}

	if (enParam&ET_STAT) {
		psReport->dwChars=strlen(pszSource);
		psReport->dwWords=strWordCount(pszSource);
	}
	if (!(enParam&ET_TRANS)&&!(enParam&ET_LANGDETECT)) return NULL; // Nessun servizio richiesto

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
	
	if (!psPh) {
		psReport->etError=GE_STRING_TOO_LONG;
		return NULL;
	}

	//
	// Richiesta traduzioni frasi
	//
	if (enParam&ET_OUTPUTDETT) {
		if (psPh->iPhraseToTrans>1) 
			printf("%s|%d}",pszLangDest,psPh->iPhraseToTrans);
			else
			printf("%s}",pszLangDest);
	}

	if (iMaxPhrases&&psPh->iPhraseToTrans>iMaxPhrases) {
	
		psReport->etError=GE_TOO_PHRASES;
		_phraseDestroy(psPh);
		jsonDestroy(psJson);
		return NULL;

	}
	if (enParam&ET_OUTPUTDETT) printf("|Phrase: total %d, problems %d, html:%d|" CRLF,psPh->iPhraseToTrans,psPh->iPhraseProblems,psPh->psTag->bHtml);
	if (enParam&ET_DOT_PROGRESS) printf("%d",psPh->iPhraseToTrans);

	//for (f=0;psPh->arPhrase[f];f++)
	for (lstLoop(psPh->lstPhrase,psPhs))
	{
		CHAR *	pszPhrase;
		CHAR *	pszPrefix=NULL;

		if (!psPhs->bTranslate) continue;

		// strTrim(psPhs->pszStr); 
		pszPhrase=psPhs->pszStr;
		if (strEmpty(pszPhrase)) continue;
		iLen=strlen(pszPhrase); if (iLen<2) continue; // Non traduco frasi con meno di 2 caratteri

		//
		// a. Controllo se è un numero
		//
 		if (!_isNaN(pszPhrase)) {ehFreePtr(&pszPrefix); continue;}

		//
		// b. Conto le parole
		//
		iWords=0; iCharsBad=0; dRap=0;
		if (enParam&ET_LANGDETECT) {

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
					if (enParam&ET_SERVICE_CALL) printf("l$");
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
			EH_LST lstKey;
			
			// Cerco Appi o multi Appid
			lstKey=lstNew();
			pszBingApp=jsonGet(psJson,"bingAppId");
			if (pszBingApp) lstPush(lstKey,pszBingApp);
			else {
			
				pszBingApp=jsonGet(psJson,"arBingAppId.length");
				if (pszBingApp) {
					INT a,iLen=atoi(pszBingApp);
					for (a=0;a<iLen;a++) {

						lstPush(lstKey,jsonGetf(psJson,"arBingAppId[%d]",a));

					}
				
				}
			}

			if (enParam&ET_OUTPUTDETT) printf("|pszBingApp:%s|" CRLF,pszBingApp);
			if (lstKey->iLength) {

				CHAR * pszAppId;
				if (enParam&ET_STAT) {

					psReport->dwBingChars+=strlen(pszPhrase);
					psReport->dwBingWords+=strWordCount(pszPhrase);

				}
				for (lstLoop(lstKey,pszAppId)) {

					if (_isBlackList(lstBlackKey,ET_BING,pszAppId)) continue;

					pUtf=_bingCall(		enParam,
										pszPhrase,
										pszLangSource,
										pszLangDest,
										pszAppId,
										&iRequestDone,
 										&psReport->etError); 

					if (enParam&ET_OUTPUTDETT) {
						printf("(%s) > (%s) #%d",pszPhrase,pUtf,psReport->etError);
					}
					psReport->iBingRequest+=iRequestDone;
					if (!strEmpty(pUtf)) 
						{	
							psReport->enTech=ET_BING;
							psReport->iBingResponse++; 
							if (enParam&ET_OUTPUTDETT) printf("B");
							break;
						}
						else
						{
							psReport->iBingErrors++; if (enParam&ET_OUTPUTDETT) printf("Bx");
							ehSleep(1000);
						}

					//
					// Abuso + BlackList
					//
					if (psReport->etError==GE_ABUSE&&
						lstBlackKey) {
					
							_(sBlack);
							sBlack.enTech=ET_BING;
							sBlack.pszAppId=strDup(pszAppId);
							lstPush(lstBlackKey,&sBlack);
					}
				}
			}
			lstDestroy(lstKey);
		}

		//
		// AZURE (Market Place) request ###########################################################################################
		//
		if (!pUtf&&
			psJson&&
			enParam&ET_TRY_AZURE) {

			CHAR * pszToken;
			INT iRequestDone=0;
			
			if (!_s.lstAzureToken) _azureTokenRequest(enParam,pszJsonParams);

//			printf("Azure token: %d",_s.lstAzureToken->iLength
			if (_s.lstAzureToken->iLength) {

				if (enParam&ET_STAT) {

					psReport->dwAzureChars+=strlen(pszPhrase);
					psReport->dwAzureWords+=strWordCount(pszPhrase);

				}

				for (lstLoop(_s.lstAzureToken,pszToken)) {

RESTART:			if (strEmpty(pszToken)) continue;
					if (_isBlackList(lstBlackKey,ET_AZURE,pszToken)) continue;

					pUtf=_azureCall(	enParam,
										pszPhrase,
										pszLangSource,
										pszLangDest,
										pszToken,
										&iRequestDone,
 										&psReport->etError); 

					if (enParam&ET_OUTPUTDETT) {
						printf("(%s) > (%s) #%d",pszPhrase,pUtf,psReport->etError);
					}
					psReport->iAzureRequest+=iRequestDone;
					if (!strEmpty(pUtf)) 
						{	
							psReport->enTech=ET_AZURE;
							psReport->iAzureResponse++; 
							if (enParam&ET_OUTPUTDETT) printf("B");
							break;
						}
						else
						{
							psReport->iAzureErrors++; if (enParam&ET_OUTPUTDETT) printf("Bx");
							ehSleep(1000);
						}

					// Token scaduto, cazzo ...
					if (psReport->etError==GE_TOKEN_EXPIRED) {
					
						// a. Richiedere nuovi token
						printf(CRLF," ------ " CRLF);
						_azureTokenRequest(enParam,pszJsonParams);
						pszToken=lstFirst(_s.lstAzureToken);
						printf("Reload ..." CRLF);
						goto RESTART;
						// c. Resettare la lista per riprovare con il nuovo token
						

					} 


					//
					// Abuso + BlackList
					//
					if (lstBlackKey&&
						(psReport->etError==GE_ABUSE||psReport->etError==GE_NOT_CREDIT)
						)
					{
					
							_(sBlack);
							sBlack.enTech=ET_AZURE;
							sBlack.pszAppId=strDup(pszToken);
							lstPush(lstBlackKey,&sBlack);
					}

				}
			}
//			lstDestroy(lstToken);
				
		
			
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
						*szLangDetect=0;

					} else strcpy(szLangDetect,pszLangSource);

					if (enParam&ET_STAT) {
						psReport->dwGoogleChars+=strlen(pszPhrase);
						psReport->dwGoogleWords+=strWordCount(pszPhrase);
					}

					pUtf=_googleCall(	enParam,
										pw,
										szLangDetect,
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
				strCpy(szLangDetect,pUtf,sizeof(szLangDetect));
				//
				// Se la lingua è diversa da quella richiesta 
				// E la lingua è |fr/en/de|
				//
/*
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
				*/
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
				strAssign(&psPhs->pszStr,psz);
				ehFreePtr(&pszPrefix); 
				ehFree(psz);

			} else {

				strAssign(&psPhs->pszStr,pUtf);
			
			}
			
		}
		
		if (!pUtf&&!psReport->etError) {
			psPh->iPhraseProblems++;
			psReport->etError=GE_UNAVAILABLE;
			bError=TRUE;
		}
		ehFreePtr(&pUtf);
		if (enParam&ET_LANGDETECT&&*szLangDetect) break; // Basta la prima frase
		if (psReport->etError==GE_ABUSE) {bError=TRUE; }
		if (bError) break;

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
		dRap=psPh->iPhraseProblems*100/psPh->iPhraseToTrans;
		
		if (dRap<10) {
			psReport->etError=GE_NONE; // L'indicazione della lingua sorgente è errata
		}
	}

	//
	// Non ho errori: Riassemblo le frasi in una stringa
	//
	if (!bError) 
	{
		for (lstLoop(psPh->lstPhrase,psPhs)) {
			if (psPhs->pszBefore) strCat(&pTradotto,psPhs->pszBefore);
			strCat(&pTradotto,psPhs->pszStr);
			if (psPhs->pszAfter) strCat(&pTradotto,psPhs->pszAfter);
			if (*psPhs->szBreak) strCat(&pTradotto,psPhs->szBreak);
		}
		/*
		if (psPh->szCharBreak) 
			pTradotto=ARToString(psPh->arPhrase,psPh->szCharBreak,"",""); 
			else
			pTradotto=strDup(psPh->arPhrase[0]);
			*/


		//
		// "Ritaggo" la frase originale
		//
		pTradotto=tagDecode(psPh->psTag,pTradotto,TRUE,FALSE); 

		//
		// Detect
		//
		if (enParam&ET_LANGDETECT) {
			ehFreePtr(&pTradotto);
			if (!strEmpty(szLangDetect)) pTradotto=strDup(szLangDetect);
		
		} 

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
	S_TDP sTdp;
	BYTE *pUtf,*p;
	CHAR szServ[80];
	psTag=ehAllocZero(sizeof(S_TTE));
	psTag->lstToken=lstCreate(sizeof(S_TDP));
	psTag->pszSourceTagged=strDup(pSourceUtf8);
	pUtf=ehAlloc(strlen(pSourceUtf8)*10); strcpy(pUtf,pSourceUtf8);

	while (true) {

		// Witango
		p=_strExtract(pUtf,"@@"," \1<",false,true,false); //if (p) {p[strlen(p)-1]=0;}

		//p=strExtract(pUtf,"@@"," ",FALSE,TRUE);	
		if (!p) p=strExtract(pUtf,"@@",NULL,TRUE,TRUE); 
		if (!p) p=strExtract(pUtf,"<@",">",TRUE,TRUE);
		if (!p) // Html
		{
			p=strExtract(pUtf,"&#",";",TRUE,TRUE); // Mha
			if (!p) p=strExtract(pUtf,"&lt",";",TRUE,TRUE); // Mha
			if (!p) p=strExtract(pUtf,"&gt",";",TRUE,TRUE); // Mha
			// if (!p) p=strExtract(pUtf,"<code>","</code>",TRUE,TRUE); // Cosa sta tra code/code non viene tradotto
			if (!p) p=strExtract(pUtf,"<",">",TRUE,TRUE);
			if (p&&!psTag->bHtml) psTag->bHtml=true;
		}

		// Tag C
		if (!p) {p=strstr(pUtf,"%d"); if (p) p=strDup("%d");}
		if (!p) {p=strstr(pUtf,"%s"); if (p) p=strDup("%s");}

		// Tag interni
		if (!p) p=strExtract(pUtf,"@#","#@",true,true);
		
		if (p) 
		{
//			_tagBuild(szServ,ARLen(psTag->arTag),2); // era 1
//			ARAddarg(&psTag->arTag,"%s\1%s",szServ,p);
			_tagBuild(szServ,psTag->lstToken->iLength,3); // era 1
			_(sTdp);
			sTdp.pszIs=strDup(szServ);
			sTdp.pszWas=strDup(p);
			lstPush(psTag->lstToken,&sTdp);

			psTag->dwSizeTag+=strlen(p);
			while (strReplace(pUtf,p,szServ)) {psTag->dwSizeTag+=strlen(p);}
			ehFree(p);
		} else break;	
	}

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
//			_tagBuild(szServ,ARLen(psTag->arTag),2);
			_tagBuild(szServ,psTag->lstToken->iLength,3);
			sTdp.pszIs=strDup(szServ);
			sTdp.pszWas=strDup(p);
			lstPush(psTag->lstToken,&sTdp);

//			ARAddarg(&psTag->arTag,"%s\1%s",szServ,psz);
			while (strReplace(pUtf,psz,szServ));
		}
		
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

	S_TDP sTdp;
	CHAR * p;
	CHAR cTagStart=*TAG_START;
	CHAR cTagEnd=*TAG_END;
	CHAR * pTagStart=NULL;
	INT iMode=0;
	CHAR * pszTake,c;
	CHAR szServ[80];
	CHAR * pszStart=pUtf;

	for (p=pszStart;*p;p++) {
		c=*p;
		switch (iMode) {

			case 0: // Non armato
				if (c==cTagStart) {pTagStart=p; iMode=1;}
				break;

			case 1: // Armo la fine
				
				if (c==cTagEnd) {
					iMode=2; break;
				}
				if (strchr("0123456789",c)) 
					break;
				pTagStart=NULL; iMode=0; // Falso allarme 
				break;

			case 2: 
				if (c==cTagStart) {iMode=1; break;} // Inizia un altro tag
				if (strchr(" .\t\r\n",c)) 
						break;
				
				pszStart=pTagStart;
				pszTake=strTake(pTagStart,p-1); 
				if (strCount(pszTake,TAG_START)==1&&!strCount(pszTake,"\t\r\n")) { // Un tag solo (senza altro nel mezzo)= non raggruppo ... passo oltre
					p=pTagStart+strlen(pszTake)-1;
					ehFree(pszTake);
					iMode=0;
					break;
				}
//				_tagBuild(szServ,ARLen(psTag->arTag),2); // era 1
				//ARAddarg(&psTag->arTag,"%s\1%s",szServ,pszTake);
				_tagBuild(szServ,psTag->lstToken->iLength,3); // era 1
				_(sTdp);
				sTdp.pszIs=strDup(szServ);
				sTdp.pszWas=strDup(pszTake);
				lstPush(psTag->lstToken,&sTdp);

				while (strReplace(pUtf,pszTake,szServ));
				p=pszStart-1;
				ehFree(pszTake);
				iMode=0; // Riarmo ricerca
				break;

		
		}
	}

	if (iMode==2) {
	
		pszTake=strTake(pTagStart,p-1); 
		if (strCount(pszTake,TAG_START)>1) { // Un tag solo = non raggruppo ... passo oltre
			_tagBuild(szServ,psTag->lstToken->iLength,3); // era 1
			_(sTdp);
			sTdp.pszIs=strDup(szServ);
			sTdp.pszWas=strDup(pszTake);
			lstPush(psTag->lstToken,&sTdp);
			while (strReplace(pUtf,pszTake,szServ));
			ehFree(pszTake);
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
CHAR * tagDecode(S_TTE * psTagEnc,CHAR *pszSourceUtf8,BOOL bFreeSource,BOOL bFreeTag)
{
//	INT b;
	BYTE *pUtf=NULL;
	CHAR *pszRet=NULL;
//	EH_AR ar;
	S_TDP * psTdp;
//	INT iTags=ARLen(psTagEnc->arTag);
	if (psTagEnc->lstToken->iLength)
	{
		if (pszSourceUtf8)
		{
			BOOL bFound=true;
			pUtf=ehAlloc(strlen(pszSourceUtf8)+psTagEnc->dwSizeTag+50); 
			strcpy(pUtf,pszSourceUtf8);

			//for (b=0;b<5;b++) {
			while (bFound) {
				
				bFound=false;
				for (lstLoop(psTagEnc->lstToken,psTdp)) {
					if (!strstr(pUtf,psTdp->pszIs)) continue;
					bFound=true;
					while (strReplace(pUtf,psTdp->pszIs,psTdp->pszWas));
				}

				/*
				for (a=0;psTagEnc->arTag[a];a++)
				{
					ar=ARCreate(psTagEnc->arTag[a],"\1",NULL);
					while (strReplace(pUtf,ar[0],ar[1]));
					ARDestroy(ar);
				}
				*/
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

void tagFree(S_TTE * psTagEnc)
{
	//psTagEnc->arTag=ARDestroy(psTagEnc->arTag);
	S_TDP * psTdp;
	for (lstLoop(psTagEnc->lstToken,psTdp)) {
		ehFree(psTdp->pszIs);
		ehFree(psTdp->pszWas);
	}
	psTagEnc->lstToken=lstDestroy(psTagEnc->lstToken);
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
// https://cloud.google.com/translate/v2/pricing (il servizio è a pagamento)
// 
static BYTE * _googleCall( ET_PARAM enParam, 
						   WCHAR * pwString,
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


	// https://cloud.google.com/translate/v2/getting_started#background-operations
	if (enParam&ET_LANGDETECT) {

		strcpy(pPage,"https://www.googleapis.com/language/translate/v2/detect?q="); 
		//strcpy(pPage,"http://ajax.googleapis.com/ajax/services/language/detect?v=1.0&q="); 
		psz=strEncodeEx(2,pwString,2,SE_UTF8,SE_URL);
		strcat(pPage,psz);
		ehFree(psz);

		psSen=senCreate();
		if (!strEmpty(pszApiKey)) sprintf(strNext(pPage),"&key=%s",senEncode(psSen,SE_URL,pszApiKey));
		senDestroy(psSen);
	}
	else if (enParam&ET_TRANS) {

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
	}
	//
	// Chiamata a Google (Faccio 5 tentativi)
	//
	*petError=GE_NONE;
	pszLastError=strDup("");
	bSeveralError=FALSE;
	for (t=0;t<5;t++)
	{
		psWeb=webHttpReq(pPage,"GET",NULL,FALSE,0); iRequestDone++;
		if (enParam&ET_SERVICE_CALL) printf("G$");
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
// _bingCall (Richiesta a Bin della traduzione di una frase) > in disuso
//
static UTF8 * _bingCall(ET_PARAM	enParam,
						UTF8 *		pszSource,
						CHAR *		langSource,
						CHAR *		langDest,
						CHAR *		pszApiKey,
						INT *		piRequestDone,
						ET_ERROR *	petError)
{
	BYTE * pszLastError;
	BYTE * psz;
	BYTE * pPage=ehAlloc(64000);
	CHAR * pszSrc;
	CHAR * putfRet=NULL;
	EH_WEB *psWeb;
	EH_AR	arTrans=NULL;

	INT iRequestDone=0;
	if (!piRequestDone) piRequestDone=&iRequestDone; else iRequestDone=*piRequestDone;
	if (strEmpty(pszApiKey)) ehExit("Non e' indicata la bingAppId");

	if (enParam&ET_LANGDETECT) 
		sprintf(pPage,"http://api.microsofttranslator.com/V2/Ajax.svc/Detect?appId=%s&text=",pszApiKey);
	else if (enParam&ET_TRANS)
		sprintf(pPage,"http://api.microsofttranslator.com/V2/Ajax.svc/Translate?appId=%s&from=%s&to=%s&text=",pszApiKey,langSource,langDest);
	

	// Effetto un pre-encoding di < e >
	pszSrc=ehAlloc(64000);
	strcpy(pszSrc,pszSource);

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
	if (enParam&ET_SERVICE_CALL) printf("B$");
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

				if (enParam&ET_DOT_PROGRESS) printf("."); 

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
					printf("- (bing) %s [%s]" CRLF,putfRet,pszApiKey);
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
	if (putfRet&&*petError==GE_NONE) ehSleep(300); 
	ehFreePtr(&pszLastError);
	ehFree(pszSrc);

	if (arTrans) ARDestroy(arTrans);
	return putfRet;
}



//
// _azureCall (Richiesta a Microsoft Azure della traduzione di una frase) > in disuso
//
static UTF8 * _azureCall(ET_PARAM	enParam,
						UTF8 *		pszSource,
						CHAR *		langSource,
						CHAR *		langDest,
						CHAR *		pszToken,
						INT *		piRequestDone,
						ET_ERROR *	petError)
{
	BYTE * pszLastError;
	BYTE * psz;
	BYTE * pPage=ehAlloc(64000);
	CHAR * pszSrc;
	CHAR * putfRet=NULL;
	EH_WEB *psWeb;
	EH_AR	arTrans=NULL;

	INT iRequestDone=0;
	if (!piRequestDone) piRequestDone=&iRequestDone; else iRequestDone=*piRequestDone;
	if (strEmpty(pszToken)) ehExit("Token non indicato _azureCall");
	pszSrc=ehAlloc(64000);

	//
	// Chiamata a Azure 
	//

	// 
	// https://datamarket.azure.com/developer/applications
	// https://api.datamarket.azure.com/Bing/MicrosoftTranslator/v1/Translate
	// 
	psz=strEncode(pszToken,SE_URL,NULL);
	if (enParam&ET_LANGDETECT) {
		sprintf(pPage,"http://api.microsofttranslator.com/V2/Ajax.svc/Detect?appId=Bearer+%s&text=",psz);
	} else {
		sprintf(pPage,"http://api.microsofttranslator.com/V2/Ajax.svc/Translate?appId=Bearer+%s&from=%s&to=%s&text=",psz,langSource,langDest);
	}
	ehFree(psz);

	// Effetto un pre-encoding di < e >
	strcpy(pszSrc,pszSource);
	while (strReplace(pszSrc,"<","&lt;"));
	while (strReplace(pszSrc,">","&gt;"));
	psz=strEncode(pszSrc,SE_URL,NULL);
	strcat(pPage,psz);
	ehFree(psz);

	*petError=GE_NONE;
	pszLastError=strDup("");
	psWeb=webHttpReq(pPage,"GET",NULL,FALSE,0); // <--------------------------------
	iRequestDone++; 
	
	if (enParam&ET_SERVICE_CALL) printf("A$");
	if (psWeb->sError.enCode)
	{
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

				if (enParam&ET_DOT_PROGRESS) printf("."); 

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

				 
				if (strstr(putfRet,"TranslateApiException")) {

					if (strstr(putfRet," has zero balance")) {
				
						*petError=GE_NOT_CREDIT;
						printf("Credito esaurito." CRLF);
						ehFreePtr(&putfRet);
					}
					else {

						*petError=GE_ABUSE;
						printf("- (1) %s [%s]" CRLF,putfRet,pszToken);
						ehFreePtr(&putfRet);
					}

				}

				else if (!strBegin(putfRet,"ArgumentException: ")) {
					
					*petError=GE_SERVER_ERROR;
					if (strstr(putfRet,"The incoming token has expired")) {
						*petError=GE_TOKEN_EXPIRED;
						printf("Expired!" );
					} 
					else  printf("- (2) %s" CRLF,putfRet);
					
					ehFreePtr(&putfRet);

				}/*
					else {

						*petError=GE_ABUSE;
						printf("- (3) %s [%s]" CRLF,putfRet,pszToken);
						ehFreePtr(&putfRet);
					}*/

			}

		
		} else {
			
			*petError=GE_SERVER_ERROR;
		}

	} 
	webHttpReqFree(psWeb);	

	*piRequestDone=iRequestDone;
	ehFree(pPage);
	if (putfRet&&*petError==GE_NONE) ehSleep(300); 
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

//
// transReportFree()
//
EH_LST transBlackListCreate(void) {

	return lstCreate(sizeof(ET_BLACK_ITEM));
}

EH_LST transBlackListDestroy(EH_LST lst) {

	ET_BLACK_ITEM * psItem;
	for (lstLoop(lst,psItem)) {
		ehFree(psItem->pszAppId);	
	}
 	return lstDestroy(lst);;
}


//
// ehTagReplace() - Sostituisce delle coppie con un tag virtuale per evitare la traduzione
//
EH_LST ehTagReplace(CHAR * pszSource, INT iNums, ...) {

	va_list vl;
	CHAR * psz;
	INT i;
	CHAR szServ[200];
	EH_LST lst;
	EH_AR ar;
	CHAR * pszTag;

	va_start(vl,iNums);	
	lst=lstNew();

	for (i=0;i<iNums;i++)
	{
		psz=va_arg(vl,CHAR *);
		ar=strSplit(psz,"\1");
		while (true) {
			pszTag=strExtract(pszSource,ar[0],ar[1],false,true);
			if (pszTag) {
				sprintf(szServ,"<tag id='%d'/>",lst->iLength);
				strReplace(pszSource,pszTag,szServ);
				lstPushf(lst,"%s\1%s",szServ,pszTag);
				ehFree(pszTag);
			} else break;
		}
		ehFree(ar);
	}
		
	va_end(vl);
	return lst;
}

//
// ehTagResume()
//
CHAR * ehTagResume(CHAR * pszSource,EH_LST lst) {
	
	CHAR * pszNew=NULL;
	CHAR * psz;
	
	if (pszSource) {
		if (lst->iLength) {
			DWORD dwMemo=strlen(pszSource);
			for (lstLoop(lst,psz)) {
				dwMemo+=strlen(psz);
			}
			pszNew=ehAlloc(dwMemo);
			strcpy(pszNew,pszSource);

			for (lstLoop(lst,psz)) {
				EH_AR ar=strSplit(psz,"\1");
				strReplace(pszNew,ar[0],ar[1]);
				ehFree(ar);			
			}
		} else pszNew=strDup(pszSource);
	
	}
	lstDestroy(lst);
	ehFree(pszSource);
	return pszNew;

}
