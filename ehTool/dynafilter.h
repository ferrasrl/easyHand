//   ---------------------------------------------
//   | DYNAFILTER Filtro dinamico             	 
//   | Permette l'inserimento ed il controllo                                          
//   | di un filtro su multicampi e con comparazione                                          
//   | multiple con caratteri jolly                                          
//   |                                           
//   |                                           
//   |             by Ferr… Art & Tecnology 2001
//   ---------------------------------------------

typedef struct {
	CHAR szFieldName[80];// Campo del Dbase da controllare
	SINT Hdl;		 // Hdl della memoria
	SINT iNum;       // Numero di voci
	CHAR **lppArray; // Array delle voci separate
	BOOL fBlobField;
	SINT iOperator; // Operatore 0=AND 1=OR // Operazione (usi futuri per ora 0 = eguaglianza)
	SINT iPriority;  // Più è alta la priorità prima fa il calcolo
} DBDYNA;

typedef struct {
	CHAR *lpFieldName;   // Nome del campo
	BOOL fBlobField;
	CHAR *lpValue;   // Valori da controllare separati dalla virgola
	SINT iOperator; // 0 = Uguale
	SINT iPriority;  // Più è alta la priorità prima fa il calcolo
} DBDYNASET;

BOOL DbDynaFilter(SINT cmd,LONG info,void *ptr);
