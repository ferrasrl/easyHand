//   +-------------------------------------------+
//   | DMIutil  DRVMEMOINFO Open                 |
//   |             by Ferr… Art & Tecnology 1998 |
//   +-------------------------------------------+

typedef struct  {
	 BOOL  Reset;
	 SINT  Hdl;
	 WORD  Size;
	 LONG  Max;
	 LONG  Num;
 } _DMI;

#define DRVMEMOINFO _DMI
#define DMIRESET {-1,-1,-1,-1}

void DMIOpen(_DMI *DRVinfo,SINT MemoType,LONG Max,SINT Size,CHAR *Nome);
//void DMIAppend(DRVMEMOINFO *DRVinfo,void *Dato);
void DMIAppendDyn(_DMI *DRVinfo,void *Dato); // Only 32bit
void DMIInsertDyn(_DMI *DRVinfo,LONG Pos,void *Dato);
//void DMIWrite(DRVMEMOINFO *DRVinfo,LONG Pos,void *Dato);
void DMIDelete(_DMI *DRVinfo,LONG Pos,void *Dato);
void DMIInsert(_DMI *DRVinfo,LONG Pos,void *Dato);

void DMIClose(_DMI *DRVinfo,CHAR *Nome);
// New 2006
CHAR *DMIDebug(CHAR *,LONG);

// New 2003
void DMIReadEx(_DMI *DRVinfo,LONG Pos,void *Dato,CHAR *lpWho);
void DMIWriteEx(_DMI *DRVinfo,LONG Pos,void *Dato,CHAR *lpWho);

#ifdef _DEBUG
#define DMIAppend(a,b) DMIAppendEx(a,b,DMIDebug(__FILE__,__LINE__))
#define DMIRead(a,b,c) DMIReadEx(a,b,c,DMIDebug(__FILE__,__LINE__))
#define DMIWrite(a,b,c) DMIWriteEx(a,b,c,DMIDebug(__FILE__,__LINE__))
#else
#define DMIAppend(a,b) DMIAppendEx(a,b,DMIDebug(__FILE__,__LINE__))
#define DMIRead(a,b,c) DMIReadEx(a,b,c,DMIDebug(__FILE__,__LINE__))
#define DMIWrite(a,b,c) DMIWriteEx(a,b,c,DMIDebug(__FILE__,__LINE__))
#endif

void DMIAppendEx(_DMI *DRVinfo,void *Dato,CHAR *lpWho);
void DMISort(_DMI *DRVinfo, int (__cdecl *compare )(const void *elem1, const void *elem2 ));

// New 9/2007
BOOL DMIBinaryFind(_DMI *pDmi,void *pElement,SINT iSize,SINT *pIndex);
