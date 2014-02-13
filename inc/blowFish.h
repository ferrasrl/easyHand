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

CHAR * bfHexDecrypt(CHAR * pszCryptHex,CHAR * pszKey,CHAR * pszDest,INT iSizeDest);
CHAR * bfHexEncrypt(CHAR * pszString,CHAR * pszKey,CHAR * pszHexDest,INT iHexDest);
CHAR * bfHexEncryptAlloc(CHAR * pszCryptHex,CHAR * pszKey);
CHAR * bfHexDecryptAlloc(CHAR * pszCryptHex,CHAR * pszKey);

