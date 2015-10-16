
#include "/easyhand/ehtool/crypt.h"

//
// Twitter.h
//
typedef struct {

	CHAR *		pszConsumerKey;
	CHAR *		pszConsumerSecret;

	CHAR *		pszAccessToken;
	CHAR *		pszAccessTokenSecret;

	CHAR *		pszAgent;

	CHAR *		pszLastResult;
	EH_LST		lstResult;
	EH_JSON *	jsResult;


} S_TWITTER;


S_TWITTER * twCreate(CHAR * pszConsumerKey,CHAR * pszConsumerSecret,CHAR * pszAgent);
S_TWITTER *	twDestroy(S_TWITTER * psTw);
BOOL		twRequest(	S_TWITTER * psTw,
						CHAR * pszVersion,
						CHAR * pszMethod,
						CHAR * pszApi,
						CHAR * pszFormat,
						...);
BOOL		twSetTokenArgs(S_TWITTER * psTw,CHAR * pszStr);
EH_JSON *	twGetCredential(S_TWITTER * psTw);

//
// GooglePlus (non completato)
//
typedef struct {

	CHAR *	pszApiKey;
	CHAR *	pszClientId;
	CHAR *	pszClientSecret;
	CHAR *	pszAgent;

	CHAR *	pszLastResult;
	EH_LST	lstResult;

} S_GOOGLE_PLUS;


S_GOOGLE_PLUS * gpCreate(CHAR * pszAppApiKey,CHAR * pszAppClientId,CHAR * pszAppClientSecret,CHAR * pszAgent);
S_GOOGLE_PLUS *	gpDestroy(S_GOOGLE_PLUS * psGp);
BOOL	gpRequest(	S_GOOGLE_PLUS * psGp,
					CHAR * pszMethod,
					CHAR * pszApi,
					CHAR * pszFormat,
					...);


//
// Facebook 
//
typedef struct {

	CHAR * pszVer;
	CHAR * pszAppId;
	CHAR * pszAppSecret;
	CHAR * pszUserId;	// Di facebook

	CHAR * pszAppToken;			// Token a livello applicazione
	CHAR * pszAccessToken;		// Token a Accesso (utente limitato nel tempo)
	CHAR * pszAccessTokenEx;	// Token a Accesso Long Time

	CHAR *		pszLastResult;
	EH_JSON *	jsResult;

} S_FACEBOOK;


S_FACEBOOK *	fbCreate(BOOL	bTokenCheckReq,CHAR * pszAppApiKey,CHAR * pszAppClientId,CHAR * pszAppClientSecret,CHAR * pszAgent);
S_FACEBOOK *	fbDestroy(S_FACEBOOK * psFb);
BOOL			fbRequest(	S_FACEBOOK * psFb,
							CHAR * pszMethod,
							CHAR * pszApi,
							CHAR * pszJson,
							...);
EH_JSON *		fbExchangeToken(S_FACEBOOK * psFb,CHAR * pszAccessToken,BOOL bGetInfo);
void *			fbGroups(S_FACEBOOK * psFb,BOOL bRetJson);
