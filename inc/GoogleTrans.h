//   +-------------------------------------------+
//    GoogleTrans.h
//								by Ferrà srl 2010
//   +-------------------------------------------+

typedef enum {
	GTM_DEFAULT,
	GTM_GOOGLE
} EN_TAG_MODE;

typedef struct {
	EH_AR arTag;
	CHAR *pszSource;
	CHAR *pszSourceTagged;
	DWORD dwSizeTag;
} S_GTAGENC;

typedef enum {
	GE_NONE,
	GE_LANG_SOURCE,		// La lingua indicata come sorgente non è esatta
	GE_STRING_SUSPECT,	// La stringa da analizzare è da controllare
	GE_SERVER_ERROR,	// Dopo 5 tentativi non ho la traduzione
	GE_STRING_TOO_LONG,	// Stringa troppo lunga e non divisibile
	GE_ABUSE			// Dopo 5 tentativi non ho la traduzione
} EN_GOOGLE_ERROR;

typedef struct {
	CHAR *	pszLangSource;
	CHAR *	pszLangDest;
	UTF8 *  utfSource;
	UTF8 *  utfDest;
} S_TRANSLATION;


S_GTAGENC *GTagEncode(CHAR *pSource,SINT iMode); 
void GTagFree(S_GTAGENC *psTagEnc); 
CHAR *	GTagDecode(S_GTAGENC *psTagEnc,CHAR *pSource,BOOL bFreeSource,BOOL bFreeTag); 
WCHAR * GoogleTranslate(WCHAR *pwSource,
						CHAR *langSource,
						CHAR *langDest,
						BOOL bBreakOnErr,
						BYTE *pszApiKey,
						BYTE *pUserIp,
						SINT *piRequestDone,
						BOOL bDetect,
						EN_GOOGLE_ERROR *piError,
						void * (* extCacheTrans)(EN_MESSAGE enMess,LONG lParam,void *pVoid)
						);
CHAR *	GoogleTranslateUtf8(CHAR *pSourceUtf8,
							CHAR *langSource,
							CHAR *langDest,
							BOOL bBreakOnErr,
							BYTE *pszApiKey,
							BYTE *pUserIp,
							SINT *piRequestDone,
							BOOL bDetect,
							EN_GOOGLE_ERROR *piError,
							void * (* extCacheTrans)(EN_MESSAGE enMess,LONG lParam,void *pVoid)
							);

