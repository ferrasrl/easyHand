//   ---------------------------------------------
//   | SQLDriver32
//   ---------------------------------------------

void SQLDriverPreset(struct OBJ *poj, // Struttura Oggetti
					 CHAR *ObjName, // Nome oggetto
					 SINT (*FunzDisp)(SINT cmd,CHAR *PtDati,LONG  dato,void  *str,SINT Hdb,SINT IndexNum), // Funzione responsabile della stampa
					 HDB Hdb,
					 SINT iIndex,
					 CHAR *lpWhereIniziale
					 ); // dbase su cui effettuare le ricerche

void *SQLDriver32(struct OBJ *objCalled,SINT cmd,LONG info,CHAR *str);
// Cambio di filtro
//void SQLWhereChange(CHAR *lpObjName,CHAR *Mess,...);
void SQLWhereChange(CHAR *lpObjName,BOOL fFindLast,CHAR *Mess,...);

