//   +-------------------------------------------+
//   | DMIutil  DRVMEMOINFO Open                 |
//   |             by Ferr… Art & Tecnology 1998 |
//   +-------------------------------------------+
// Struttura di utilit… varia per driver

#ifndef NUME
#define ALFA 0
#define NUME 1
#define DATA 2
#endif

typedef struct {
	SINT idb;
	CHAR szName[40];
	SINT iType; // ALFA,NUME
	CHAR *lpValore;
	double dValore;
} ARFIELD;


void ar_close(SINT idb);
void ar_recreset(SINT idb);
SINT ar_FldFind(SINT idb,CHAR *lpFieldName,ARFIELD *lpField);

SINT ar_FldInt(SINT idb,CHAR *lpFieldName);
TCHAR *ar_FldPtr(SINT idb,CHAR *lpFieldName);

void ar_FldWrite(SINT idb,CHAR *lpFieldName,TCHAR *lpValue,double dVal);
TCHAR *ar_FldToQuery(SINT idb);
void ar_FldToAdb(SINT idb,BOOL fAfterFree);
