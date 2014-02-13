//   +-------------------------------------------+
//   | ARMaker  ArrayMaker                       |
//   |          Costruttore di Array             |
//   |                                           |
//   |          cmd         ptr                  |
//   |          WS_OPEN     "len record"         |
//   |          WS_ADD      ->Record             |
//   |                                           |
//   |          WS_CLOSE    "Nome lista memo"    |
//   |          ritorna Hdl della lista          |
//   |                                           |
//   |             4 Maggio                      |
//   |             by Ferr… Art & Tecnology 1999 |
//   +-------------------------------------------+

#define LNSIZE sizeof(LONG)

typedef struct {
	BOOL bOpen;
	BOOL bReady;
	LONG lRecordSize;
	UINT uiListRec;
	UINT uiListPtr;
	UINT uiListCount;
	LONG *lpOffsets;
	BYTE *lpRecords;
	void **array;
} S_ARMAKER;

#ifndef EH_MEMO_DEBUG
SINT ARMaker(SINT cmd,void *ptr);
SINT ARMakerEx(SINT cmd,void *ptr,S_ARMAKER *lpsArMaker);
#else
SINT _ARMaker(SINT cmd,void *ptr,CHAR *pFile,SINT iLine);
#define ARMaker(a,b) _ARMaker(a,b,__FILE__,__LINE__)
SINT _ARMakerEx(SINT cmd,void *ptr,S_ARMAKER *lpsArMaker,BYTE *pProg,SINT iLine);
#define ARMakerEx(a,b,c) _ARMakerEx(a,b,c,__FILE__,__LINE__)
#endif

EH_ARF ARFHdlToPtr(SINT hdl);
// new 2007
void *	ARPtrMakerEx(SINT cmd,void *ptr,S_ARMAKER *lpsArMaker);
void *	ARPtrMaker(SINT cmd,void *ptr);


