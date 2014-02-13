#ifdef __cplusplus
extern "C" {
#endif

SINT FGetProfileChar(TCHAR *TheFile,CHAR *Var,BYTE *Valore);
SINT FGetProfileCharEx(TCHAR *TheFile,CHAR *Var,BYTE *Valore,SINT *piSizeValore);
BYTE *FGetProfileAlloc(TCHAR *TheFile,CHAR *pVarName,BYTE *pDefault); // new 2007

void FSetProfileChar(TCHAR *file,CHAR *Var,CHAR *Valore);
SINT FGetProfileInt(TCHAR *file,CHAR *Var,SINT Dammi);
void FSetProfileInt(TCHAR *file,CHAR *Var,SINT Valore);

#ifdef _WIN32
BOOL FSetProfileBin(TCHAR *file,CHAR *Var,void *Valore,SINT Len);
BOOL FGetProfileBin(TCHAR *file,CHAR *Var,void *Valore,SINT Len);
void FGetProfileRect(TCHAR *file,CHAR *Var,RECT *Rect);
void FSetProfileRect(TCHAR *file,CHAR *Var,RECT *Rect);
void FSetProfileMode(SINT iMode);
BOOL FGetProfileWP(TCHAR *file,CHAR *Var,BOOL fShow);// New 2005
void FSetProfileWP(TCHAR *file,CHAR *Var); // New 2005
#endif

#ifdef __cplusplus
}
#endif
