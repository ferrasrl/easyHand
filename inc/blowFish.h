//
// blowFish
//
  
typedef struct {
  DWORD P[16 + 2];
  DWORD S[4][256];
} S_BLOWFISH;

void bfInit(S_BLOWFISH * psCtx, BYTE * key, INT iKeyLem);
void bfEncrypt(S_BLOWFISH * psCtx, DWORD * pXl, DWORD * pXr);
void bfDecrypt(S_BLOWFISH * psCtx, DWORD * pXl, DWORD * pXr);

CHAR * bfHexEncrypt(CHAR * pszString,CHAR * pszKey,CHAR * pszHexDest,INT iHexDest);
CHAR * bfHexDecrypt(CHAR * pszCryptHex,CHAR * pszKey,CHAR * pszDest,INT iSizeDest);
CHAR * bfHexEncryptAlloc(CHAR * pszCryptHex,CHAR * pszKey);
CHAR * bfHexDecryptAlloc(CHAR * pszCryptHex,CHAR * pszKey);

CHAR * bfBaseEncrypt(	CHAR *	pszSource,
						CHAR *	pszKey,
						CHAR *	pszBaseDest,
						INT		iBaseDestSize);
CHAR * bfBaseDecrypt(CHAR * pszCryptBase,CHAR * pszKey,CHAR * pszDest,INT iSizeDest);
