//   +-------------------------------------------+
//    Euritmo
//
//                by Ferrà srl 2013
//   +-------------------------------------------+

#ifndef _EURITMO
#define _EURITMO

typedef struct {
	
	CHAR *			pszName;
	EH_DATATYPE		enType;
	INT				iLen;
	CHAR *			pszFormat;
	BOOL			bObb;	// Obbligatorio
	CHAR *			pszDefault;

	// Interno
	CHAR *			pszValue;	// Settaggio fatto da record

} EUR_FIELD;
/*
typedef struct {

	CHAR * pszFieldName;
	CHAR * pszValue;

} EUR_SET;
*/
typedef struct {

	CHAR *		pszFileName;
	EH_LST		lstFile;
	//EH_FILE *	psFile;

} EUR_FILE;

typedef struct {
	
	EUR_FILE *	psEurFile;	// File di riferimento
	EUR_FIELD * arsFormat;	// Formato del record
//	EH_LST lstSet;			// Lista dei settaggi

} EUR_RECORD;


typedef struct {
	CHAR * pszName;
	EUR_FIELD * ars;
} S_EUR_ITEM;


EUR_FILE * euOpen(UTF8 * pszFileName);
CHAR * euClose(EUR_FILE * psEurFile,BOOL bGetRemove);
EUR_RECORD * euCreate(EUR_FILE * psEurFile,CHAR * pszTipoRecord);
EUR_RECORD * euDestroy(EUR_RECORD * );

EUR_FIELD * eurSearch(EUR_FIELD * arsField,CHAR * pszFieldName);
CHAR * euRecInfo(EUR_RECORD * psRec);
BOOL euSet(EUR_RECORD * psRec, CHAR * pszFieldName,CHAR * pszValue);
BOOL euSetf(EUR_RECORD * psRec, CHAR * pszFieldName,CHAR * pszFormat, ...);
BOOL euWrite(EUR_RECORD * psRec,BOOL bRecFree);


#endif