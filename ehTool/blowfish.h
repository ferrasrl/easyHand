//
// blowFish
//
  
typedef struct {
  DWORD P[16 + 2];
  DWORD S[4][256];
} S_BLOWFISH_CTX;

void bfInit(S_BLOWFISH_CTX * psCtx, BYTE * key, INT iKeyLem);
void bfEncrypt(S_BLOWFISH_CTX * psCtx, DWORD * pXl, DWORD * pXr);
void bfDecrypt(S_BLOWFISH_CTX * psCtx, DWORD * pXl, DWORD * pXr);
void bfStrDecrypt(CHAR * pszCryptStr,CHAR * pszKey,CHAR * pszDest,INT iSizeDest);
