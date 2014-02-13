SINT FGetProfileChar(CHAR *TheFile,CHAR *Var,BYTE *Valore);
SINT FGetProfileCharEx(CHAR *TheFile,CHAR *Var,BYTE *Valore,SINT iSizeValore);
void FSetProfileChar(CHAR *file,CHAR *Var,CHAR *Valore);
SINT FGetProfileInt(CHAR *file,CHAR *Var,SINT Dammi);
void FSetProfileInt(CHAR *file,CHAR *Var,SINT Valore);

#ifdef _WIN32
BOOL FSetProfileBin(CHAR *file,CHAR *Var,void *Valore,SINT Len);
BOOL FGetProfileBin(CHAR *file,CHAR *Var,void *Valore,SINT Len);
void FGetProfileRect(CHAR *file,CHAR *Var,RECT *Rect);
void FSetProfileRect(CHAR *file,CHAR *Var,RECT *Rect);
#endif