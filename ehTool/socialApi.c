//   ---------------------------------------------
//	 | socialApi
//   |
//	 |							by Ferrà srl 2015
//   ---------------------------------------------
#include "/easyhand/inc/easyhand.h"
#include "/easyhand/inc/socialApi.h"


// Google Signin
// https://developers.google.com/identity/
// https://developers.google.com/identity/sign-in/web/


// https://dev.twitter.com/oauth/overview/authorizing-requests

//
// twCreate()
//
S_TWITTER * twCreate(CHAR * pszConsumerKey,CHAR * pszConsumerSecret,CHAR * pszAgent) {

	S_TWITTER * psTw=ehAllocZero(sizeof(S_TWITTER));
	psTw->pszConsumerKey=strDup(pszConsumerKey);
	psTw->pszConsumerSecret=strDup(pszConsumerSecret);
	psTw->pszAgent=pszAgent?strDup(pszAgent):NULL;
	psTw->lstResult=coupleCreate();

	return psTw;
}

//
// twDestroy()
//
S_TWITTER *	twDestroy(S_TWITTER * psTw) {

	psTw->jsResult=jsonDestroy(psTw->jsResult);
	coupleDestroy(psTw->lstResult);
	ehFreePtrs(	6,
				&psTw->pszConsumerKey,
				&psTw->pszConsumerSecret,
				&psTw->pszAccessToken,
				&psTw->pszAccessTokenSecret,

				&psTw->pszAgent,

				&psTw->pszLastResult);
	
	ehFree(psTw);
	return NULL;

}
//
// twGetNonce()
//
CHAR * twGetNonce(void) {

	CHAR szServ[200];
	CHAR * psz;
	INT a;
	time_t t;

	*szServ=0;
	srand((unsigned) time(&t));
	for (a=0;;a++) {
		strAppend(szServ,"%u",rand()%50);
		if (strlen(szServ)>32) break;
	}
	
	psz=strEncode(szServ,SE_BASE64,NULL); strcpy(psz,strOmit(psz,"=")); if (strlen(psz)>32) psz[32]=0;
	return psz;

}

//
// twRequest
// Ritorna False se tutto OK
// Valorizza
//
BOOL	twRequest(	S_TWITTER * psTw,
					CHAR * pszVersion,
					CHAR * pszMethod,
					CHAR * pszApi,
					CHAR * pszFormat,
					...) 
{
	EH_WEB		sWeb;
	EH_WEB *	psWeb;
	CHAR *		pszNonce;
	DWORD		dwTimeStamp=0;
	EH_LST		lstAll,lstHeader,lstBody,lstParams;
	CHAR		szUrl[2058];
	EH_JSON	*	psJs=NULL,*psJson;
	S_SEN *		psSen;	
	time_t		now;
	CHAR *		pszParamStr;
	EH_LST		lst;
	CHAR *		pszString;
	CHAR *		pszSigning;
	CHAR *		pszSignature;
	CHAR *		psz;
	BOOL		bError=true;
	
	CHAR * pszParams=NULL;
	if (pszFormat) 
	{	
		strFromArgs(pszFormat,pszParams); // Cerco i parametri
		psJs=jsonCreate(pszParams); //if (!psJs) ehError();
		ehFree(pszParams);
	}

	psSen=senCreate();
//	sprintf(szUrl,"https://api.twitter.com/1.1/%s",pszApi);
	if (!strEmpty(pszVersion)) 
		sprintf(szUrl,"https://api.twitter.com/%s/%s",pszVersion,pszApi);
		else
		sprintf(szUrl,"https://api.twitter.com/%s",pszApi);
	
	lstAll=lstNew();
	lstHeader=lstNew();
	lstParams=lstNew();
	lstBody=lstNew();

	//
	// Leggo i parametri header
	//
	if (psJs) {
		psJson=jsonGetPtr(psJs,"header");
		if (psJson) {
			psJson=psJson->psChild;
			do {

				if (psJson->enType!=JSON_ARRAY) {
					lstPushf(lstAll,"%s=%s",psJson->pszName,senEncode(psSen,SE_URL,psJson->utfValue));
					lstPushf(lstHeader,"%s=\"%s\"",psJson->pszName,senEncode(psSen,SE_URL,psJson->utfValue));
				} 

			} while ((psJson=psJson->psNext));
		}
	}

	//
	// Leggo i parametri body
	//
	if (psJs) {
		psJson=jsonGetPtr(psJs,"body"); 
		if (psJson) {
			psJson=psJson->psChild;
			do {

				if (psJson->enType!=JSON_ARRAY) {
					lstPushf(lstAll,"%s=%s",psJson->pszName,senEncode(psSen,SE_URL,psJson->utfValue));
					lstPushf(lstBody,"%s=%s",psJson->pszName,senEncode(psSen,SE_URL,psJson->utfValue));
				} 

			} while ((psJson=psJson->psNext));
		}
	}

	//
	// Leggo i parametri params
	//
	if (psJs) {
		psJson=jsonGetPtr(psJs,"params");
		if (psJson) {
			psJson=psJson->psChild;
			do {

				if (psJson->enType!=JSON_ARRAY) {
					lstPushf(lstAll,"%s=%s",psJson->pszName,senEncode(psSen,SE_URL,psJson->utfValue));
					lstPushf(lstParams,"%s=%s",psJson->pszName,senEncode(psSen,SE_URL,psJson->utfValue));
				} 

			} while ((psJson=psJson->psNext));
		}
	}

	//
	// Aggiungo i parametri obbligatori all'header
	//
	lstPushf(lstAll,"oauth_consumer_key=%s",senEncode(psSen,SE_URL,psTw->pszConsumerKey));
	lstPushf(lstHeader,"oauth_consumer_key=\"%s\"",senEncode(psSen,SE_URL,psTw->pszConsumerKey));

	pszNonce=twGetNonce();	
	lstPushf(lstAll,"oauth_nonce=%s",senEncode(psSen,SE_URL,pszNonce));
	lstPushf(lstHeader,"oauth_nonce=\"%s\"",senEncode(psSen,SE_URL,pszNonce));

	lstPushf(lstAll,"oauth_signature_method=HMAC-SHA1");
	lstPushf(lstHeader,"oauth_signature_method=\"HMAC-SHA1\"");

	time(&now);	
	lstPushf(lstAll,"oauth_timestamp=%I64u",now);
	lstPushf(lstHeader,"oauth_timestamp=\"%I64u\"",now);
	if (psTw->pszAccessToken) 
	{
		lstPushf(lstAll,"oauth_token=%s",psTw->pszAccessToken);
		lstPushf(lstHeader,"oauth_token=\"%s\"",psTw->pszAccessToken);
	}
	lstPushf(lstAll,"oauth_version=1.0");
	lstPushf(lstHeader,"oauth_version=\"1.0\"");

	//
	//  Compongo lstALl (header + params + body) (DA FARE)
	//
	lstSortText(lstHeader,false);
	lstSortText(lstBody,false);
	lstSortText(lstParams,false);
	lstSortText(lstAll,false);

	//
	// Compongo la "Parameters String" 
	//
	pszParamStr=lstToString(lstAll,"&","","");
	lstDestroy(lstAll);

	//
	// Compongo la Signature Base String
	//
	lst=lstNew();
	lstPush(lst,pszMethod); // <-- Deve essere maiuscolo Es .. GET,POST ecc
	lstPush(lst,senEncode(psSen,SE_URL,szUrl));	// Url di richiesta
	lstPush(lst,senEncode(psSen,SE_URL,pszParamStr)); // Parametri

	pszString=lstToString(lst,"&","",""); 
	lstDestroy(lst);
	ehFree(pszParamStr);

	//
	// Compongo la Signing key
	//
	lst=lstNew();

	_(sWeb);
	strcpy(sWeb.szReq,pszMethod);
	lstPush(lst,senEncode(psSen,SE_URL,psTw->pszConsumerSecret));	// Consumer Secrect (Applicazione)
	lstPush(lst,senEncode(psSen,SE_URL,strEver(psTw->pszAccessTokenSecret))); // Access Token Secret (Diverso per utente)
	
	pszSigning=lstToString(lst,"&","",""); 
	lstDestroy(lst);

	//
	// Signing
	//
//#ifdef _DEBUG
//	pszSignature=HMAC("POST&https%3A%2F%2Fapi.twitter.com%2F1.1%2Foauth%2Frequest_token&oauth_consumer_key%3D8zRThD53OQURH8grmAzy7abkQ%26oauth_nonce%3D34a596344f4ef85b55bac9ee056e0251%26oauth_signature_method%3DHMAC-SHA1%26oauth_timestamp%3D1437992818%26oauth_version%3D1.0",pszSigning,CALG_SHA1,true); 
//#endif

	pszSignature=HMAC(pszString,pszSigning,CALG_SHA1,true); //strUpr(pszSignature);
	lstPushf(lstHeader,"oauth_signature=\"%s\"",senEncode(psSen,SE_URL,pszSignature));
	ehFree(pszString);
	ehFree(pszSigning);
	ehFree(pszSignature);

	/*
			    String authorization_header_string = "OAuth oauth_consumer_key=\"" + oauth_consumer_key + "\",oauth_signature_method=\"HMAC-SHA1\",oauth_timestamp=\"" + 
			    		oauth_timestamp + "\",oauth_nonce=\"" + oauth_nonce + "\",oauth_version=\"1.0\",oauth_signature=\"" + URLEncoder.encode(oauth_signature, "UTF-8") + "\"";
*/

	sWeb.lpUri=szUrl;

	if (lstParams->iLength) {
		psz=lstToString(lstParams,"&","","");
		strAppend(szUrl,"?%s",psz);
		ehFree(psz);
	}
	lstDestroy(lstParams);

	sWeb.lpUserAgent=psTw->pszAgent;
	sWeb.iTimeoutSec=30;
	sWeb.pszAccept="*/*";
	if (lstBody->iLength) {
		sWeb.lpPostArgs=lstToString(lstBody,"&","","");
	}

	sWeb.lstHeader=lstNew();
	psz=lstToString(lstHeader,", ","","");
	lstPushf(sWeb.lstHeader,"Authorization: OAuth %s",psz);
	ehFree(psz);
	lstDestroy(lstHeader);
	lstDestroy(lstBody);
	ehFree(pszNonce);


//	lstPushf(sWeb.lstHeader,"Authorization: OAuth oauth_callback=\"%s\",",senEncode(psSen,SE_URL,"http://app.yacht4web.lan/twitter.jsp"));
	sWeb.bNotRetryWithError=true;
	sWeb.iKeepAlive=true;
	psWeb=webHttpReqEx(&sWeb,false);

	// Nonce (codice unico della richiesta) 
	// Signature (firma della richiesta da vedere)
	// Signature method (metodo di codifica della richiesta ( HMAC-SHA1 )
	// Timestamp (Unix epoca numero di secondi dal 1970)
	// Token
	// Version
	if (psWeb->sError.enCode==WTE_OK) {
		
		strAssign(&psTw->pszLastResult,psWeb->pData);
		psTw->lstResult=coupleDestroy(psTw->lstResult);
		psTw->jsResult=jsonDestroy(psTw->jsResult);
		if (*psTw->pszLastResult!='{') psTw->lstResult=coupleFromUrl(psTw->pszLastResult); else psTw->jsResult=jsonCreate(psTw->pszLastResult);
		bError=false;

	} else {

		CHAR szServ[1024];
		sprintf(szServ,"%s:%s:%-80.80s",psWeb->sError.szHttpErrorCode,psWeb->sError.szHttpErrorDesc,strEver(psWeb->pData));
		strTrim(szServ);
		strAssign(&psTw->pszLastResult,szServ);
		coupleClean(psTw->lstResult);
		psTw->jsResult=jsonDestroy(psTw->jsResult);
	
	}
	webHttpReqFree(psWeb);
	senDestroy(psSen);
	if (psJs) jsonDestroy(psJs);
	return bError;
}



//
// twGetProfile()
//
EH_JSON * twGetCredential(S_TWITTER * psTw) {

	EH_JSON * psJs;

	if (!twRequest(	psTw,
					"1.1",
					"GET",
					"account/verify_credentials.json",
					"{"
					//	"header: {oauth_token:'%s'},"
						"params: {include_entities:'true', include_email:'true'}"
					"}"
					//psTw->pszAccessToken
					)) {
	
		psJs=psTw->jsResult;
	
	}
	return psJs;

}

//
// twSetTokenArgs()
//
BOOL twSetTokenArgs(S_TWITTER * psTw,CHAR * pszStr) {

	EH_LST		lstArg;
	EH_JSON *	psJs=NULL;

	lstArg=coupleFromUrl(pszStr);
	strAssign(&psTw->pszAccessToken,coupleGet(lstArg,"oauth_token"));
	strAssign(&psTw->pszAccessTokenSecret,coupleGet(lstArg,"oauth_token_secret"));
	lstArg=coupleDestroy(lstArg);
	return false;

}

//
// twCreate()
//
S_GOOGLE_PLUS * gpCreate(CHAR * pszApiKey,CHAR * pszClientId,CHAR * pszClientSecret,CHAR * pszAgent) {

	S_GOOGLE_PLUS * psGp=ehAllocZero(sizeof(S_GOOGLE_PLUS));
	psGp->pszApiKey=strDup(pszApiKey);
	psGp->pszClientId=strDup(pszClientId);
	psGp->pszClientSecret=strDup(pszClientSecret);
	psGp->pszAgent=pszAgent?strDup(pszAgent):NULL;
	psGp->lstResult=coupleCreate();

	return psGp;
}

//
// twDestroy()
//
S_GOOGLE_PLUS *	gpDestroy(S_GOOGLE_PLUS * psGp) {

	ehFreePtrs(	4,
				&psGp->pszClientId,
				&psGp->pszClientSecret,
				&psGp->pszAgent,
				&psGp->pszLastResult);
	coupleDestroy(psGp->lstResult);
	ehFree(psGp);
	return NULL;

}

//
// gpRequest() - REST API -  
// Ritorna False se tutto OK
// Valorizza
//
BOOL	gpRequest(	S_GOOGLE_PLUS * psGp,
					CHAR * pszMethod,
					CHAR * pszApi,
					CHAR * pszJson,
					...) 
{
	EH_WEB		sWeb;
	EH_WEB *	psWeb;
	DWORD		dwTimeStamp=0;
	EH_LST		lstAll,lstHeader,lstBody,lstParams;
	CHAR		szUrl[2058];
	EH_JSON	*	psJs,*psJson;
	S_SEN *		psSen;	
	CHAR *		psz;
	BOOL		bError=true;
	
	CHAR * pszParams=NULL;
	strFromArgs(pszJson,pszParams); // Cerco i parametri
	psJs=jsonCreate(pszParams); //if (!psJs) ehError();
	ehFree(pszParams);

	psSen=senCreate();
	sprintf(szUrl,"https://www.googleapis.com/plus/v1/%s",pszApi);
	
	lstAll=lstNew();
	lstHeader=lstNew();
	lstParams=lstNew();
	lstBody=lstNew();

	//
	// Leggo i parametri header
	//
	if (psJs) {
		psJson=jsonGetPtr(psJs,"header");
		if (psJson) {
			psJson=psJson->psChild;
			do {

				if (psJson->enType!=JSON_ARRAY) {
					lstPushf(lstAll,"%s=%s",psJson->pszName,senEncode(psSen,SE_URL,psJson->utfValue));
					lstPushf(lstHeader,"%s=\"%s\"",psJson->pszName,senEncode(psSen,SE_URL,psJson->utfValue));
				} 

			} while ((psJson=psJson->psNext));
		}
	}

	//
	// Leggo i parametri body
	//
	if (psJs) {
		psJson=jsonGetPtr(psJs,"body");
		if (psJson) {
			psJson=psJson->psChild;
			do {

				if (psJson->enType!=JSON_ARRAY) {
					lstPushf(lstAll,"%s=%s",psJson->pszName,senEncode(psSen,SE_URL,psJson->utfValue));
					lstPushf(lstBody,"%s=%s",psJson->pszName,senEncode(psSen,SE_URL,psJson->utfValue));
				} 

			} while ((psJson=psJson->psNext));
		}
	}

	//
	// Leggo i parametri params
	//
	if (psJs) {
		psJson=jsonGetPtr(psJs,"params");
		if (psJson) {
			psJson=psJson->psChild;
			do {

				if (psJson->enType!=JSON_ARRAY) {
					lstPushf(lstAll,"%s=%s",psJson->pszName,senEncode(psSen,SE_URL,psJson->utfValue));
					lstPushf(lstParams,"%s=%s",psJson->pszName,senEncode(psSen,SE_URL,psJson->utfValue));
				} 

			} while ((psJson=psJson->psNext));
		}
	}
/*
	//
	// Aggiungo i parametri obbligatori all'header
	//
	lstPushf(lstAll,"oauth_consumer_key=%s",senEncode(psSen,SE_URL,psTw->pszConsumerKey));
	lstPushf(lstHeader,"oauth_consumer_key=\"%s\"",senEncode(psSen,SE_URL,psTw->pszConsumerKey));

	pszNonce=twGetNonce();	
	lstPushf(lstAll,"oauth_nonce=%s",senEncode(psSen,SE_URL,pszNonce));
	lstPushf(lstHeader,"oauth_nonce=\"%s\"",senEncode(psSen,SE_URL,pszNonce));

	lstPushf(lstAll,"oauth_signature_method=HMAC-SHA1");
	lstPushf(lstHeader,"oauth_signature_method=\"HMAC-SHA1\"");

	time(&now);	
	lstPushf(lstAll,"oauth_timestamp=%I64u",now);
	lstPushf(lstHeader,"oauth_timestamp=\"%I64u\"",now);
	if (psTw->pszToken) 
	{
		lstPushf(lstAll,"oauth_token=%s",psTw->pszToken);
		lstPushf(lstHeader,"oauth_token=\"%s\"",psTw->pszToken);
	}
	lstPushf(lstAll,"oauth_version=1.0");
	lstPushf(lstHeader,"oauth_version=\"1.0\"");

	//
	//  Compongo lstALl (header + params + body) (DA FARE)
	//
	lstSortText(lstAll,false);
	
	//
	// Compongo la "Parameters String" 
	//
	pszParamStr=lstToString(lstAll,"&","","");
	lstDestroy(lstAll);

	//
	// Compongo la Signature Base String
	//
	lst=lstNew();
	lstPush(lst,pszMethod); // <-- Deve essere maiuscolo Es .. GET,POST ecc
	lstPush(lst,senEncode(psSen,SE_URL,szUrl));	// Url di richiesta
	lstPush(lst,senEncode(psSen,SE_URL,pszParamStr)); // Parametri

	pszString=lstToString(lst,"&","",""); 
	lstDestroy(lst);
	ehFree(pszParamStr);

	//
	// Compongo la Signing key
	//
	lst=lstNew();

	_(sWeb);
	strcpy(sWeb.szReq,pszMethod);
	lstPush(lst,senEncode(psSen,SE_URL,psTw->pszConsumerSecret));	// Url di richiesta
	lstPush(lst,senEncode(psSen,SE_URL,strEver(psTw->pszTokenSecret))); // Parametri
	
	pszSigning=lstToString(lst,"&","",""); 
	lstDestroy(lst);

	//
	// Signing
	//
//#ifdef _DEBUG
//	pszSignature=HMAC("POST&https%3A%2F%2Fapi.twitter.com%2F1.1%2Foauth%2Frequest_token&oauth_consumer_key%3D8zRThD53OQURH8grmAzy7abkQ%26oauth_nonce%3D34a596344f4ef85b55bac9ee056e0251%26oauth_signature_method%3DHMAC-SHA1%26oauth_timestamp%3D1437992818%26oauth_version%3D1.0",pszSigning,CALG_SHA1,true); 
//#endif

	pszSignature=HMAC(pszString,pszSigning,CALG_SHA1,true); //strUpr(pszSignature);
	lstPushf(lstHeader,"oauth_signature=\"%s\"",senEncode(psSen,SE_URL,pszSignature));
	ehFree(pszString);
	ehFree(pszSigning);
	ehFree(pszSignature);
	*/
	sWeb.lpUri=szUrl;

	if (lstParams->iLength) {
		psz=lstToString(lstParams,"&","","");
		strAppend(szUrl,"?%s",psz);
		ehFree(psz);
	}
	lstDestroy(lstParams);

	sWeb.lpUserAgent=psGp->pszAgent;
	sWeb.iTimeoutSec=30;
	sWeb.pszAccept="*/*";
	if (lstBody->iLength) {
		sWeb.lpPostArgs=lstToString(lstBody,"&","","");
	}

	sWeb.lstHeader=lstNew();
	psz=lstToString(lstHeader,", ","","");
	lstPushf(sWeb.lstHeader,"Authorization: OAuth %s",psz);
	ehFree(psz);
	lstDestroy(lstHeader);
	lstDestroy(lstBody);
//	ehFree(pszNonce);

	psWeb=webHttpReqEx(&sWeb,false);

	// Nonce (codice unico della richiesta) 
	// Signature (firma della richiesta da vedere)
	// Signature method (metodo di codifica della richiesta ( HMAC-SHA1 )
	// Timestamp (Unix epoca numero di secondi dal 1970)
	// Token
	// Version
	if (psWeb->sError.enCode==WTE_OK) {
		
		strAssign(&psGp->pszLastResult,psWeb->pData);
		coupleDestroy(psGp->lstResult);
		psGp->lstResult=coupleFromUrl(psGp->pszLastResult);
		bError=false;

	} else {

		strAssign(&psGp->pszLastResult,psWeb->pData);
		coupleClean(psGp->lstResult);
	
	}
	webHttpReqFree(psWeb);
	senDestroy(psSen);
	if (psJs) jsonDestroy(psJs);
	return bError;
}


//
// ########## FACEBOOK ####################################
//


//
// fbCreate()
// https://developers.facebook.com/docs/facebook-login/manually-build-a-login-flow/v2.4
//
S_FACEBOOK * fbCreate(	BOOL	bTokenCheckReq,	// COntrolla ed eventualmente richiedere l'access Token
						CHAR *	pszAppId,
						CHAR *	pszAppSecret,
						CHAR *	pszFacebookUserId,
						CHAR *	pszAccessToken) 
{

	// https://developers.facebook.com/docs/facebook-login/manually-build-a-login-flow/v2.4#token
	S_FACEBOOK * psFb=ehAllocZero(sizeof(S_FACEBOOK));
	CHAR szUrl[2048];
	EH_WEB *	psWeb;

	psFb->pszAppId=strDup(pszAppId);
	psFb->pszAppSecret=strDup(pszAppSecret);
	psFb->pszUserId=strDup(pszFacebookUserId);
	psFb->pszAccessToken=strDup(pszAccessToken);
	psFb->pszVer=strDup("v2.4");

	/*
GET /oauth/access_token?
     client_id={app-id}
    &client_secret={app-secret}
    &grant_type=client_credentials
	*/

	//
	// App Access Token
	//
	sprintf(szUrl,"https://graph.facebook.com/%s/oauth/access_token?client_id=%s&client_secret=%s&grant_type=client_credentials",
			psFb->pszVer,
			psFb->pszAppId,psFb->pszAppSecret
			);
	psWeb=webHttpReq(szUrl,"GET",NULL,false,30);
	if (psWeb->sError.enCode==WTE_OK) {
		EH_JSON * js=jsonCreate(psWeb->pData);
		if (js) {
			strAssign(&psFb->pszAppToken,jsonGet(js,"access_token"));
		}
		jsonDestroy(js);
		
	}
	webHttpReqFree(psWeb);

	//
	// https://developers.facebook.com/docs/roadmap/completed-changes/offline-access-removal
	//
	if (bTokenCheckReq&&!strEmpty(pszAccessToken)) {
	
		// Controllo se è valido (da fare)
		
		// Richiedo il token
		/*
		var uri=_y$.base+"/account/webManage.jsp?a=social";
		var szLoc="https://www.facebook.com/dialog/oauth?client_id="+_ywSocial.fbAppId+"&scope="+szScope+"&redirect_uri="+escape(uri)+"&response_type=token";
		*/
		EH_WEB *	psWeb;
		S_SEN *		psSen;
		CHAR		szUrl[1024];

		psSen=senCreate();
		sprintf(szUrl,"https://graph.facebook.com/oauth/access_token?"            
				"client_id=%s"
				"&client_secret=%s"
				"&grant_type=fb_exchange_token"
				"&fb_exchange_token=%s",
				senEncode(psSen,SE_URL,psFb->pszAppId),
				senEncode(psSen,SE_URL,psFb->pszAppSecret),
				senEncode(psSen,SE_URL,pszAccessToken));

		psWeb=webHttpReq(szUrl,"GET",NULL,false,30);

	//	printf("%s",psWeb->pData);
		webHttpReqFree(psWeb);
		senDestroy(psSen);

		/*
		CHAR *	pszScope="public_profile,email,user_friends,publish_actions,publish_pages";
		CHAR *	pszRedirect="http://it.yacht4web.com";
		CHAR	szUrl[1024];
		EH_WEB * psWeb;
		S_SEN * psSen;

		psSen=senCreate();
		sprintf(szUrl,"https://www.facebook.com/dialog/oauth?client_id=%s&scope=%s&redirect=&%s&response_type=token",
				senEncode(psSen,SE_URL,psFb->pszAppId),
				senEncode(psSen,SE_URL,pszScope),
				senEncode(psSen,SE_URL,pszRedirect));
		psWeb=webHttpReq(szUrl,"GET",NULL,false,30);


		printf("qui");
		webHttpReqFree(psWeb);
		senDestroy(psSen);
*/

	}

	if (!psFb->pszAppToken&&!psFb->pszAccessToken) {
	
		psFb=fbDestroy(psFb);
	}

	/*
	// Non so poi vedi
	if (strEmpty(psFb->pszAccessToken)&&!bTokenCheckReq) {
		strAssign(&psFb->pszAccessToken,psFb->pszAppToken);
	}
	*/

	return psFb;

}

//
// fbDestroy()
//
S_FACEBOOK * fbDestroy(S_FACEBOOK * psFb) {
	
	if (!psFb) return NULL;
	ehFreePtrs(6,&psFb->pszAppId,&psFb->pszAppSecret,&psFb->pszUserId,&psFb->pszAppToken,&psFb->pszAccessToken,&psFb->pszVer);
	
	jsonDestroy(psFb->jsResult);
	ehFreePtrs(1,&psFb->pszLastResult);
	ehFree(psFb);
	return NULL;
}

//
// fbExchangeToken()
//

EH_JSON * fbExchangeToken(S_FACEBOOK * psFb,CHAR * pszAccessToken,BOOL bGetInfo) {

	EH_WEB *	psWeb;
	S_SEN *		psSen;
	CHAR		szUrl[1024];
	EH_JSON *	js=NULL;
	EH_LST		lst;
	CHAR * psz;

	psSen=senCreate();
	sprintf(szUrl,"https://graph.facebook.com/oauth/access_token?"            
			"client_id=%s"
			"&client_secret=%s"
			"&grant_type=fb_exchange_token"
			"&fb_exchange_token=%s",
			senEncode(psSen,SE_URL,psFb->pszAppId),
			senEncode(psSen,SE_URL,psFb->pszAppSecret),
			senEncode(psSen,SE_URL,pszAccessToken));

	psWeb=webHttpReq(szUrl,"GET",NULL,false,30);
	lst=coupleFromUrl(psWeb->pData);
	if (lst) {
		psz=coupleGet(lst,"access_token");
		if (psz) {
			CHAR szServ[2048];
			strAssign(&psFb->pszAccessTokenEx,psz);
			if (bGetInfo) {
				sprintf(szServ,"{'access_token':'%s','expires':%d}",psz,atoi(strEver(coupleGet(lst,"expires"))));
				js=jsonCreate(szServ);
			}
		} else {
			if (bGetInfo) {
				js=jsonCreate(psWeb->pData);
			}
		}
		coupleDestroy(lst);
	}
	webHttpReqFree(psWeb);
	senDestroy(psSen);
	return js;
}

//
// fbRequest() - REST API -  
// Ritorna False se tutto OK
// Valorizza
//
BOOL	fbRequest(	S_FACEBOOK * psFb,
					CHAR * pszMethod,
					CHAR * pszApi,
					CHAR * pszJson,
					...) 
{
	EH_WEB		sWeb;
	EH_WEB *	psWeb;
	DWORD		dwTimeStamp=0;
	EH_LST		lstBody,lstParams;
	CHAR		szUrl[2058];
	EH_JSON	*	psJs=NULL,*psJson;
	S_SEN *		psSen;	
	CHAR *		psz;
	BOOL		bError=true;
	CHAR *		pszToken;
	
	CHAR * pszParams=NULL;
	CHAR * pszVer=NULL;
	if (pszJson) {
		strFromArgs(pszJson,pszParams); // Cerco i parametri
		psJs=jsonCreate(pszParams); //if (!psJs) ehError();
		ehFree(pszParams);
		if (psJs) pszVer=jsonGet(psJs,"ver");
	}
	

	psSen=senCreate();
	if (!pszVer) pszVer=psFb->pszVer;
	sprintf(szUrl,"https://graph.facebook.com/%s/%s",pszVer,pszApi);
	strReplace(szUrl,"[id]",psFb->pszUserId);
	strReplace(szUrl,"[user-id]",psFb->pszUserId);
	strReplace(szUrl,"[appid]",psFb->pszAppId);
	strReplace(szUrl,"[app-id]",psFb->pszAppId);
	
//	lstHeader=lstNew();
	lstParams=lstNew();
	lstBody=lstNew();
/*

	//
	// Leggo i parametri header
	//
	if (psJs) {
		psJson=jsonGetPtr(psJs,"header");
		if (psJson) {
			psJson=psJson->psChild;
			do {

				if (psJson->enType!=JSON_ARRAY) {
					lstPushf(lstAll,"%s=%s",psJson->pszName,senEncode(psSen,SE_URL,psJson->utfValue));
					lstPushf(lstHeader,"%s=\"%s\"",psJson->pszName,senEncode(psSen,SE_URL,psJson->utfValue));
				} 

			} while ((psJson=psJson->psNext));
		}
	}
*/
	//
	// Leggo i parametri body
	//
	if (psJs) {
		psJson=jsonGetPtr(psJs,"body");
		if (psJson) {
			psJson=psJson->psChild;
			do {

				if (psJson->enType!=JSON_ARRAY) {
					//lstPushf(lstAll,"%s=%s",psJson->pszName,senEncode(psSen,SE_URL,psJson->utfValue));
					lstPushf(lstBody,"%s=%s",psJson->pszName,senEncode(psSen,SE_URL,psJson->utfValue));
				} 

			} while ((psJson=psJson->psNext));
		}
	}

	//
	// Leggo i parametri params
	//
	if (psJs) {
		psJson=jsonGetPtr(psJs,"params");
		if (psJson) {
			psJson=psJson->psChild;
			do {

				if (psJson->enType!=JSON_ARRAY) {
//					lstPushf(lstAll,"%s=%s",psJson->pszName,senEncode(psSen,SE_URL,psJson->utfValue));
					lstPushf(lstParams,"%s=%s",psJson->pszName,senEncode(psSen,SE_URL,psJson->utfValue));
				} 

			} while ((psJson=psJson->psNext));
		}
	}
	
	//
	// Token
	//
	pszToken=NULL; 
	if (psJs) {psz=jsonGet(psJs,"token"); if (psz) pszToken=psz;}
	if (!pszToken)	pszToken=psFb->pszAccessToken;
	if (!pszToken)	pszToken=psFb->pszAppToken;
	if (pszToken)	lstPushf(lstParams,"access_token=%s",senEncode(psSen,SE_URL,pszToken));

	//  "https://graph.facebook.com/v2.4/me?fields=id%2Cname%2Caddress%2Cfamily%7Bage_range%7D%2Cwebsite&access_token=CAACEdEose0cBANUv8c3AECEu8d8NPY4qrqcMOPOZADtqFte8jJcQYOAT1xIV9Sxwq2qrpfyKqUztge0ZCoy1LApASOBgvjoS82FyzNA31D4i9CMdlJBielIaNcoM8WyseJQli8FcviZAPXkZAbGijz2jGeuXqfd9mukWaeQBXtgcWpe9gBCqzjweyZBrZBaFIeDFgNzd4z7gZDZD"

	_(sWeb);
	strcpy(sWeb.szReq,pszMethod);
	sWeb.lpUri=szUrl;
	if (lstParams->iLength) {
		psz=lstToString(lstParams,"&","","");
		strAppend(szUrl,"?%s",psz);
		ehFree(psz);
	}
	lstDestroy(lstParams);
//	sWeb.lpUserAgent=psGp->pszAgent;
	sWeb.iTimeoutSec=30;
	sWeb.pszAccept="*/*";
	if (lstBody->iLength) {
		sWeb.lpPostArgs=lstToString(lstBody,"&","",CRLF CRLF);
	}

//  

	if (!strEmpty(sWeb.lpPostArgs)) sWeb.bPostDirect=true;
 	psWeb=webHttpReqEx(&sWeb,false);

	// Nonce (codice unico della richiesta) 
	// Signature (firma della richiesta da vedere)
	// Signature method (metodo di codifica della richiesta ( HMAC-SHA1 )
	// Timestamp (Unix epoca numero di secondi dal 1970)
	// Token
	// Version
	if (psFb->jsResult) jsonDestroy(psFb->jsResult);
	if (psWeb->sError.enCode==WTE_OK) {
		
		strAssign(&psFb->pszLastResult,psWeb->pData);
		psFb->jsResult=jsonCreate(psWeb->pData);
		bError=false;

	} else {

		strAssign(&psFb->pszLastResult,psWeb->pData);
		psFb->jsResult=jsonCreate(psWeb->pData);
//		coupleClean(psFb->lstResult);
	
	}
	if (sWeb.lpPostArgs) ehFree(sWeb.lpPostArgs);
	webHttpReqFree(psWeb);

	senDestroy(psSen);
	lstDestroy(lstBody);
	if (psJs) jsonDestroy(psJs);
	if (!psFb->jsResult) psFb->jsResult=jsonCreate("{error:{code:'10001', desc:'connection error'}}");
	return bError;
}

//
// fbGroups()
//
void * fbGroups(S_FACEBOOK * psFb,BOOL bRetJson) {

	EH_LST lst=lstNew();
	BOOL bRes;
	CHAR * pszNext=NULL;
	INT iLimit=25;
	INT a,iLen,iCount=0;
	BOOL bError=false;
	CHAR * psz;
	void * pRet=NULL;
	lstPush(lst,"[");
	do {
	
		if (!pszNext) {
			bRes=fbRequest(	psFb,
						"GET",
						"[user-id]/groups",
						"{ver:'v2.3', params: {limit:%d}}",iLimit);
			pszNext=NULL;
		}
		else
		bRes=fbRequest(	psFb,
						"GET",
						"[user-id]/groups",
						"{ver:'v2.3', params: {after:'%s', limit:%d}}",pszNext,iLimit); 
		if (bRes) {
			bError=true; 
			break;
		} else {

			iLen=atoi(jsonGet(psFb->jsResult,"data.length"));
			if (!iLen) break;
			for (a=0;a<iLen;a++) {
				if (iCount) lstPush(lst,",");
			//	jsonPrint(psFb->jsResult,2);
				psz=jsonToString(psFb->jsResult,false,"data[%d]",a);
				lstPush(lst,psz);
				ehFree(psz); iCount++;
			}
			
		}

//		jsonPrint(psFb->jsResult,3);
		strAssign(&pszNext,jsonGet(psFb->jsResult,"paging.cursors.after"));
		//psz=jsonGet(psFb->jsResult,"paging.cursors.after");
	
	} while (pszNext);
	lstPush(lst,"]");
	ehFreePtr(&pszNext);

	if (!bError) {
		psz=lstToString(lst,"","","");
		if (bRetJson) {pRet=jsonCreate(psz); ehFree(psz);} else pRet=psz;
	}
	lstDestroy(lst);
	
	return pRet;
}
