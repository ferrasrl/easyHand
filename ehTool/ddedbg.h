//   -----------------------------------------------
//   | DdeDbg
//   | Utility per il debug attraverso server DDebug
//   | 
//   |          by Ferrà Art & Technology 1993-2002
//   -----------------------------------------------

#include <dde.h>

#define DDE_TIMEOUT 3000
void DdeDispx(CHAR *Mess,...);
#define DDEDBG_APPNAME "DdeDbg"
#define DDEDBG_TOPIC "Viever"
