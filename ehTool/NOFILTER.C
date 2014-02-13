//   ---------------------------------------------
//   | ADBSFILT  Special Filter per Dbase     	 |
//   |                                           |
//   |             by Ferr… Art & Tecnology 1998 |
//   ---------------------------------------------
#include "\ehtool\include\ehsw_idb.h"
#include "\ehtool\datautil.h"

//   +-------------------------------------------+
//   | ADB_ASFIND Cerca una chiave composta   	 |
//   |            in un indice stabilito         |
//   |                                           |
//   |                                           |
//   |             by Ferr… Art & Tecnology 1998 |
//   +-------------------------------------------+

LONG adb_Filter(SINT Hdb,SINT cmd,CHAR *Campo,CHAR *Azione,void *Valore)
{
 Hdb++; cmd++;
 *Campo=*Campo;
 Azione=Valore;
 Valore=Azione;
 return 0;
}