// ------------------------------------------------
//   mySqlEx
//  
//  
//                               by Ferrà srl 2012
// ------------------------------------------------
/*
#ifndef _MYSQL_MT
	#define _MYSQL_MT
#endif
*/
#include "/easyhand/inc/easyhand.h"
#include "/easyhand/inc/mySqlEx.h"
#include "/easyhand/inc/txbFile.h"

#define TXB_SEP_COL "\4"
#define TXB_SEP_ROW "\5"
#define TXB_NULL "?nil?"

static void _txbRowExport(INT iFields,S_FLD_INFO *arsField,S_TXB *psTxb,SQL_RS rsSet);
static CHAR * _getType(S_FLD_INFO * psFld);

//
// mysGetSchema() - Ritorna NULL se non ho lo schema
//
S_TBL_INFO * mysGetSchema(DYN_SECTION_FUNC CHAR * pszSchema,_DMI * pdmiSchema) {

	DYN_SECTION_GLOBALPTR
	S_TBL_INFO sTable;
	SQL_RS rsTable;

	// Controllo se ho lo schema
	sql_query(DYN_SECTIONC "SHOW DATABASES LIKE '%s'",pszSchema);
	rsTable=sql_stored(DYN_SECTION); 
	sql_fetch(rsTable);
	if (!rsTable) {
//		alert("OK");
		return NULL;
	}
	sql_free(rsTable);

	sql_query(DYN_SECTIONC "SHOW FULL TABLES FROM %s",pszSchema);
	rsTable=sql_stored(DYN_SECTION);
	DMIOpen(pdmiSchema,RAM_AUTO,rsTable?rsTable->iRows:20,sizeof(S_TBL_INFO),"Tables");
	while (sql_fetch(rsTable)) {

		CHAR * pszType=sql_ptr(rsTable,"2");

		//if (*pszTable=='_') continue;
		_(sTable);
		strcpy(sTable.szName,sql_ptr(rsTable,"1"));
		if (!strCaseCmp(pszType,"BASE TABLE")) sTable.enTableType=TBL_BASE;
		else if (!strCaseCmp(pszType,"VIEW")) 
			sTable.enTableType=TBL_VIEW;
		else 
			sTable.enTableType=TBL_UNKNOW;
//		if (!strCaseCmp(pszType,"BASE TABLE")) sTable.enTableType=TBL_BASE;
		DMIAppendDyn(pdmiSchema,&sTable);
	}
	sql_free(rsTable);

	return DMILock(pdmiSchema,NULL);
}

//
// mysGetTableInfo()
//
S_FLD_INFO * mysGetTableInfo(DYN_SECTION_FUNC CHAR * pszTable,_DMI * pdmiField,_DMI * pdmiIndex) {

	DYN_SECTION_GLOBALPTR
	SQL_RS rsSet;
	S_FLD_INFO sField;

	CHAR *		pszFieldName=NULL;
	CHAR *		pszNullMode=NULL;
	CHAR *		pszType,*pszTypeComplex=NULL;
	CHAR *		pszSize=NULL;
	CHAR *		pszCollation;
	CHAR *		pszE="";
	CHAR *		p;
	CHAR		szLastName[80]="";
//	BOOL		bOk;
	
//	printf("%s " ,pszTable);

	//
	// Estraggo informazioni della tabella ----
	//
	
	if (sql_query(DYN_SECTIONC "?SHOW FULL COLUMNS FROM %s",pszTable)) return NULL;

	DMIOpen(pdmiField,RAM_AUTO,50,sizeof(S_FLD_INFO),"Field");
	rsSet=sql_stored(DYN_SECTION);
	if (rsSet) {
		
		int iNumFld=rsSet->iRows;
		
		while (sql_fetch(rsSet)) {

			_(sField);

			//
			// Nome del campo
			//
			pszFieldName=sql_ptr(rsSet,"Field");
			strcpy(sField.szName,pszFieldName);

			pszCollation=sql_ptr(rsSet,"Collation");
			if (pszCollation) {
				if (!strBegin(pszCollation,"latin1_")) 
					sField.enEncoding=ULT_CS_LATIN1;
				if (!strBegin(pszCollation,"utf8_")) 
					sField.enEncoding=ULT_CS_UTF8;
				if (!strBegin(pszCollation,"ucs2_general_ci")) 
					sField.enEncoding=ULT_CS_UNICODE;
			} else sField.enEncoding=ULT_CS_LATIN1;
			
			pszTypeComplex=sql_ptr(rsSet,"Type");
			pszType=strExtract(pszTypeComplex,NULL,"(",0,0);
			if (!pszType) pszType=strExtract(pszTypeComplex,NULL," ",0,0);
			if (!pszType) pszType=strDup(pszTypeComplex);
			if (!pszType) ehError();

			// Dimensioni
			pszSize=strExtract(pszTypeComplex,"(",")",0,0);
			if (pszSize) {
			
				EH_AR ar;
				INT iNum;
				ar=ARCreate(pszSize,",",&iNum);
				sField.iSize=atoi(ar[0]);
				if (iNum==2) sField.iDecimal=atoi(ar[1]);
				ehFree(pszSize);
				ARDestroy(ar);

			}

			// Tipo
			if (!strcmp(pszType,"int")) {
				sField.enFldType=ADB_INT32;
				if (!strCmp(sql_ptr(rsSet,"Extra"),"auto_increment")) sField.enFldType=ADB_AINC;

			}
			else if (!strcmp(pszType,"smallint")) {
				sField.enFldType=ADB_INT;
			}
			else if (!strcmp(pszType,"tinyint")) {
				sField.enFldType=ADB_BOOL;
			}
			else if (!strcmp(pszType,"varchar")||!strcmp(pszType,"char")) {
				sField.enFldType=ADB_ALFA;
				if (!sField.iSize) ehExit("CHAR/VARCHAR con lunghezza errata [%s] [%d]",pszType,sField.iSize);
				if (!strCmp(pszCollation,"utf8_general_ci")) 
					sField.bCaseInsensible=true;
			}
			else if (!strcmp(pszType,"text")) {
				sField.enFldType=ADB_BLOB;
			}
			else if (!strcmp(pszType,"decimal")) {
				sField.enFldType=ADB_COBD;
			}
			else if (!strcmp(pszType,"float")) {
				sField.enFldType=ADB_COBD;
			}
			else if (!strcmp(pszType,"point")) {
				sField.enFldType=ADB_POINT;
			}
			else if (!strcmp(pszType,"geometry")) {
				sField.enFldType=ADB_GEOMETRY;
			}
			else if (!strcmp(pszType,"timestamp")) {
				sField.enFldType=ADB_TIMESTAMP;
			}
			else {

				ehExit("Formato non riconosciuto [%s]",pszType);
			}

			// in caso di campi GIS esporto in formato TEXT
			
			pszNullMode=sql_ptr(rsSet,"Null"); if (!pszNullMode) ehError();
			if (strstr(pszNullMode,"YES")) sField.bIsNullable=1;
			if (strstr(pszNullMode,"NO")) sField.bIsNullable=0;
			ehFreePtrs(1,&pszType);
			
			p=sql_ptr(rsSet,"Default");
			if (!p) strcpy(sField.szDefault,"?nil?"); else strcpy(sField.szDefault,p);


			strcpy(sField.szFieldBefore,szLastName);
			strcpy(szLastName,sField.szName);
			DMIAppendDyn(pdmiField,&sField);

		}
		sql_free(rsSet);
	}


	//
	// Leggo la chiave primaria o unica (??????)
	//
/*
	bOk=false;
	if (!sql_query(DYN_SECTIONC "?SHOW KEYS FROM %s WHERE Key_name = 'PRIMARY'",pszTable)) bOk=1;
	else if (!sql_query(DYN_SECTIONC "SHOW KEYS FROM %s WHERE non_unique=0",pszTable)) bOk=1;
	if (bOk) {
	
		rsSet=sql_stored(DYN_SECTION);
		if (rsSet) {
			// Non capisco ... è incompleto
		}
		sql_free(rsSet);
	
	}
*/

	//
	// Leggo Informazioni indice
	//

	if (pdmiIndex) {
	
		if (!sql_query(DYN_SECTIONC "?SHOW INDEXES FROM %s",pszTable)) {

			DMIOpen(pdmiIndex,RAM_AUTO,50,sizeof(S_IDX_INFO),"Index");
			rsSet=sql_stored(DYN_SECTION);
			while (sql_fetch(rsSet)) {
				S_IDX_INFO sIndex;
				_(sIndex);
				strCpy(sIndex.szName,sql_ptr(rsSet,"Key_name"),sizeof(sIndex.szName));
				sIndex.bUnique=!sql_int(rsSet,"Non_unique");
				sIndex.iSeq=sql_int(rsSet,"Seq_in_index");
				strCpy(sIndex.szField,sql_ptr(rsSet,"Column_name"),sizeof(sIndex.szField));
				strCpy(sIndex.szCollation,sql_ptr(rsSet,"Collation"),sizeof(sIndex.szCollation));
				DMIAppendDyn(pdmiIndex,&sIndex);
			}
			sql_free(rsSet);
		
		}
	
	}




	return DMILock(pdmiField,NULL);

}

//
// adb_TypeToDataType()
//
#if (!defined(_ADB32)&&!defined(_ADB32P))
EH_DATATYPE adb_TypeToDataType(EN_FLDTYPE enType) {
	
	EH_DATATYPE enRet=_UNKNOW;

	switch (enType) {

		case ADB_ALFA:	enRet=_ALFA; break;
		case ADB_NUME:	enRet=_NUMBER; break;
		case ADB_DATA:	enRet=_DATE; break;
		case ADB_INT:	enRet=_INTEGER; break;
		case ADB_FLOAT: enRet=_NUMBER; break;
		case ADB_BOOL:	enRet=_BOOL; break;
		case ADB_COBD:	enRet=_NUMBER; break;
		case ADB_COBN:	enRet=_NUMBER; break;
		case ADB_AINC:	enRet=_ID; break;
		case ADB_BLOB:	enRet=_TEXT; break;
		case ADB_INT32: enRet=_INTEGER; break;
		case ADB_BINARY: enRet=_BINARY; break;
		case ADB_POINT: enRet=_POINT; break;
		case ADB_GEOMETRY: enRet=_GEOMETRY; break;
		case ADB_TIMESTAMP: enRet=_TIME; break;
			
	
		default:
			ehError();
			break;
	}

	return enRet;

}
#endif

//
// DataTypeToadb_Type()
//
EN_FLDTYPE DataTypeToadb_Type(EH_DATATYPE enType) {
	
	EN_FLDTYPE enRet=-1;

	switch (enType) {

		case _ALFA:		enRet=ADB_ALFA; break;
		case _NUMBER:	enRet=ADB_NUME; break;
		case _DATE:		enRet=ADB_DATA; break;
		case _INTEGER:	enRet=ADB_INT;	break;
		case _BOOL:		enRet=ADB_BOOL; break;
		case _ID:		enRet=ADB_AINC; break;
		case _TEXT:		enRet=ADB_BLOB; break;


		case _BINARY:	enRet=ADB_BINARY; break;
		case _POINT:	enRet=ADB_POINT; break;
		case _GEOMETRY:	enRet=ADB_GEOMETRY; break;
		case _TIME:		enRet=ADB_TIMESTAMP; break;

		default: 
			ehExit("corrispondenza non trovata in DataTypeToadb_Type");
			break;
	}

	return enRet;

}


//
// mysTableExport()
// Ritorna True se non riesce
//
BOOL mysTableExport(DYN_SECTION_FUNC CHAR * pszTable,UTF8 * utfFileDest,CHAR * pszWhere,BOOL bShowProgress, DWORD * pdwRecords) {

	DYN_SECTION_GLOBALPTR
	_DMI dmiField=DMIRESET;
	S_FLD_INFO * arsFld;
	TXBFLD *	arsTxbFld;
	S_TXB *		psTxb;
	INT a;
	INT		iCount,iOffset,iMaxRecords,iBlock;
	CHAR *	pszQueryFields=NULL;
	SQL_RS rsSet;
	DWORD iExport=0;

	if (pdwRecords) * pdwRecords=0;
	if (!mysGetTableInfo(DYN_SECTIONC pszTable,&dmiField,NULL)) { return TRUE;}	
	pszQueryFields=ehAlloc(dmiField.Num*50); *pszQueryFields=0;
	arsFld=DMILock(&dmiField,NULL);
	for (a=0;a<dmiField.Num;a++)
	{
		strAdd(pszQueryFields,arsFld[a].szName,",",false);
	}

	//
	// Creo il TXB
	// Traduco le informazioni dei campi in tabella nell'array
	//
	arsTxbFld=ehAllocZero(sizeof(TXBFLD)*(dmiField.Num+1));
	for (a=0;a<dmiField.Num;a++)
	{
		arsTxbFld[a].lpName=arsFld[a].szName;
		arsTxbFld[a].iLen=arsFld[a].iSize;
		arsTxbFld[a].enType=adb_TypeToDataType(arsFld[a].enFldType);
		if (arsTxbFld[a].enType==_UNKNOW) ehExit("Field Type sconosciuto: %s",arsFld[a].szName);

		switch (arsTxbFld[a].enType)
		{
			case _NUMBER : 
			 arsTxbFld[a].iInfo=arsFld[a].iDecimal;
			 break;
		}
	}

	psTxb=txbCreate(utfFileDest,
					TXB_SEP_COL, // stringa separatore colonne (Uso questo perchè \1,\2 e \3 potrei trovarli
					TXB_SEP_ROW CRLF, // stringa separatore righe
					"UTF-8", // NULL= default (ASCII)
					arsTxbFld);// Descrizione dei campi
	if (!psTxb) osError(true,0,"Non riesco a creare %s",utfFileDest);
	ehFree(arsTxbFld);

	//
	// Esporto il file
	//

	if (!strEmpty(pszWhere)) {
		iMaxRecords=sql_count(DYN_SECTIONC "%s WHERE %s",pszTable,pszWhere); 
	
	} else {
		iMaxRecords=sql_count(DYN_SECTIONC pszTable); 
	}
	if (iMaxRecords<0) 
		ehError();
	iBlock=32000;

	for (iOffset=0;iOffset<iMaxRecords;iOffset+=iBlock) {
		iCount=iOffset;

		// Prendo il blocco
		//sql_query("SELECT * FROM %s LIMIT %d,%d",pszTable,iOffset,iBlock);
		if (!strEmpty(pszWhere)) {
			CHAR * pszOrder;
			if (!strstr(pszWhere,"ORDER BY")) pszOrder="ORDER BY 1 ASC"; else pszOrder=""; 
			sql_query(DYN_SECTIONC "SELECT %s FROM %s WHERE %s %s LIMIT %d,%d",pszQueryFields,pszTable,pszWhere,pszOrder,iOffset,iBlock);
		}
		else {
			sql_query(DYN_SECTIONC "SELECT %s FROM %s ORDER BY 1 ASC LIMIT %d,%d",pszQueryFields,pszTable,iOffset,iBlock);
		}
		printf("\rrichiedo a mySql da %d (%d records) [%d%%] ...     ",iOffset,iBlock,iOffset*100/iMaxRecords);
		rsSet=sql_stored(DYN_SECTION);
		while (sql_fetch(rsSet)) {
			if (bShowProgress) {
				iCount++;
				if ((!(iCount%512)&&iMaxRecords>0)||iMaxRecords<1000) {  
					printf("\r%s > %d (%d%%) ...        ",pszTable,iCount,iCount*100/iMaxRecords);
				}
			}
			_txbRowExport(dmiField.Num,arsFld,psTxb,rsSet);
			iExport++;
		}
		fflush(psTxb->ch);
		sql_free(rsSet);
	}
	if (bShowProgress) {
		printf("\r%s > %d (completed)." CRLF,pszTable,iMaxRecords);
	}
	//
	// Chiudo
	//
	txbClose(psTxb);
	DMIClose(&dmiField,"Field");
	ehFree(pszQueryFields);
	if (pdwRecords) * pdwRecords=iExport;
	return FALSE;	
}

//
// _getPrimaryKey()
//
static CHAR * _getPrimaryKey(_DMI * pdmiField,_DMI * pdmiIndex,SQL_RS rsSet,EH_LST lstField) {

	INT a,iNum;
	S_IDX_INFO sIndex;
	S_FLD_INFO sField;
	CHAR szName[80]="";
	CHAR * pszKey=NULL;
	EH_LST lst=lstNew();
	CHAR * pszValue,* pszString;

	for (a=0;a<pdmiIndex->iLength;a++) {
		
		DMIRead(pdmiIndex,a,&sIndex);
		if (!strcmp(sIndex.szName,"PRIMARY")||sIndex.bUnique) {
			strcpy(szName,sIndex.szName);
		}

		if (!*szName) continue;
		if (strcmp(sIndex.szName,szName)) break;

		if (lstField) {
			lstPush(lstField,sIndex.szField);
		}

		if (rsSet) {

			iNum=mysFieldSearch(pdmiField,sIndex.szField); if (iNum<0) ehError();
			DMIRead(pdmiField,iNum,&sField);
			pszValue=sql_ptr(rsSet,sIndex.szField);
			switch (sField.enFldType)
			{
				case ADB_NUME : 
				case ADB_COBD : 
				case ADB_COBN : 
				case ADB_FLOAT:
					
					if (!pszValue) pszValue="NULL";
					pszString=strDup(pszValue);
					break;

				case ADB_BOOL :
				case ADB_INT  : 
				case ADB_INT32: 
				case ADB_AINC:
					if (!pszValue) pszValue="NULL";
					pszString=strDup(pszValue);
					break;

				case ADB_ALFA:
				case ADB_DATA:
				case ADB_BLOB:

					pszString=strEncode(pszValue,SE_SQLSTR,NULL);
					break;

				case ADB_BINARY:
				case ADB_POINT:
				case ADB_GEOMETRY:
					break;

				default:
					ehError();
					break;
			}

			lstPushf(lst,"%s=%s",sIndex.szField,pszString);
			ehFree(pszString);

		}		
	}
	if (rsSet) {
		pszKey=lstToString(lst," AND ","","");
	}
	lstDestroy(lst);
	return pszKey;

}



//
// mysTableUtf8Repair()
// Ritorna True se non riesce
//
BOOL mysTableUtf8Repair(DYN_SECTION_FUNC CHAR * pszTable,BOOL bShowProgress,DWORD * pdwUpdate) {

	DYN_SECTION_GLOBALPTR
	_DMI dmiField=DMIRESET;
	_DMI dmiIndex=DMIRESET;
	S_FLD_INFO * arsFld;
	INT		a;
	INT		iCount,iOffset,iMaxRecords,iBlock;
//	CHAR *	pszQueryFields=NULL;
	CHAR *	pszQueryCols=NULL;
	SQL_RS rsSet;
	DWORD iExport=0;
	CHAR * pszFld;
	EH_LST lstCols,lstField,lstUtf;
	INT iUpdate=0;
	INT iBack;

//	if (pdwRecords) * pdwRecords=0;
	if (pdwUpdate) *pdwUpdate=0;
	if (!mysGetTableInfo(DYN_SECTIONC pszTable,&dmiField,&dmiIndex)) { return true;}	
	arsFld=DMILock(&dmiField,NULL);
//	pszQueryFields=ehAlloc(dmiField.Num*50); *pszQueryFields=0;

	lstCols=lstNew();
	lstUtf=lstNew();
	//
	// Inserisco campi primari
	//
	_getPrimaryKey(&dmiField,&dmiIndex,NULL,lstCols);

	//
	// Cerco campi con UTF8
	//
	iBack=lstCols->iLength;
	for (a=0;a<dmiField.Num;a++)
	{
		if (arsFld[a].enEncoding==ULT_CS_UTF8) {lstPush(lstCols,arsFld[a].szName); lstPush(lstUtf,arsFld[a].szName);}
	}

	if (lstCols->iLength==iBack) {
	
		lstDestroy(lstCols);
		DMIClose(&dmiField,"Field");
		DMIClose(&dmiIndex,"Field");
		return false; // Non contiene record da modificare
	}
	pszQueryCols=lstToString(lstCols,",","","");
	lstDestroy(lstCols);


	//
	// Updating 
	//
	iMaxRecords=sql_count(DYN_SECTIONC pszTable); 
	lstField=lstNew();
	iBlock=32000;
	for (iOffset=0;iOffset<iMaxRecords;iOffset+=iBlock) {
		iCount=iOffset;
		sql_query(DYN_SECTIONC "SELECT %s FROM %s ORDER BY 1 ASC LIMIT %d,%d",pszQueryCols,pszTable,iOffset,iBlock);
		printf("\rrichiedo a mySql da %d (%d records) [%d%%] ...     ",iOffset,iBlock,iOffset*100/iMaxRecords);
		rsSet=sql_stored(DYN_SECTION);
		while (sql_fetch(rsSet)) {
			BOOL bNotUpdate;
			if (bShowProgress) {
				iCount++;
				if ((!(iCount%512)&&iMaxRecords>0)||iMaxRecords<1000) {  
					printf("\r%s > %d (%d%%) ...        ",pszTable,iCount,iCount*100/iMaxRecords);
				}
			}
			bNotUpdate=false;
			lstClean(lstField);
			for (lstLoop(lstUtf,pszFld)) {
			
				CHAR *	pszBug;
				CHAR *	pszGood=NULL;
				CHAR *	pUtfSql=NULL;
				BOOL	bStrError=false;
				pszBug=sql_ptr(rsSet,pszFld);
				if (strEmpty(pszBug)) continue;

				for (pszBug=sql_ptr(rsSet,pszFld);;pszBug=pUtfSql) {
					CHAR * pszBefore=strDup(pszBug);
					ehFreeNN(pUtfSql);
					pUtfSql=strEncodeEx(1,pszBefore,2,SD_UTF8,SE_WTC);
					ehFree(pszBefore);
					if (isUtf8(pUtfSql,false)) 
					{
						ehFreeNN(pszGood);
						pszGood=strEncodeEx(1,pUtfSql,2,SD_UTF8,SE_UTF8);
					} else break;
					if (!strcmp(pszBug,pUtfSql)) break; // Sono identiche
					bStrError=true;
				}
				if (!pszGood) {bStrError=false; bNotUpdate=true; pszGood=strDup("?");}
				ehFree(pUtfSql);
				if (bStrError) {
					pUtfSql=strEncode(pszGood,SE_SQL,NULL); 
					lstPushf(lstField,"%s%s='%s'",lstField->iLength?",":"",pszFld,pUtfSql);
					ehFree(pUtfSql);				
				}
				ehFree(pszGood);
				
			}

			//
			// Ci sono campi da upgradare
			//
			if (lstField->iLength) {
			
				CHAR * pszQuery;
				CHAR * pszPrimaryKey;
				lstInsf(lstField,0,"UPDATE %s SET ",pszTable);
				pszPrimaryKey=_getPrimaryKey(&dmiField,&dmiIndex,rsSet,NULL);
				lstPushf(lstField," WHERE %s",pszPrimaryKey);
				pszQuery=lstToString(lstField,"","","");
				ehFree(pszPrimaryKey);
 				sql_query(DYN_SECTIONC "%s",pszQuery);
				ehFree(pszQuery);
				iUpdate++;

			}
			
		}
		sql_free(rsSet);
	}
	if (bShowProgress) {
		printf("\r%s > %d (completed)." CRLF,pszTable,iMaxRecords);
	}
	if (pdwUpdate) *pdwUpdate=iUpdate;
	DMIClose(&dmiField,"Field");
	DMIClose(&dmiIndex,"Field");
	ehFree(pszQueryCols);
	lstDestroy(lstField);
	lstDestroy(lstUtf);
 	return FALSE;	
}


//
// _txbRowExport()
//

static void _txbRowExport(INT iFields,S_FLD_INFO *arsFld,S_TXB *psTxb,SQL_RS rsSet)
{

	INT a;
	BYTE *	pString;
	BYTE *	pszValue;
	double	dValue;
	CHAR	szFormat[100];
	SIZE_T	sizLen;
	CHAR *	psz64;
	BOOL	bError=FALSE;

	for (a=0;a<iFields;a++)
	{
		pszValue=sql_ptr(rsSet,arsFld[a].szName);
		if (pszValue) dValue=atof(pszValue); else dValue=0;

		switch (arsFld[a].enFldType)
		 {
			case ADB_NUME : 
			case ADB_COBD : 
			case ADB_COBN : 
			case ADB_FLOAT:
				sprintf(szFormat,"%%.%df",arsFld[a].iDecimal); 
				fprintf(psTxb->ch,szFormat,dValue); 
				break;

			case ADB_BOOL :
			case ADB_INT  : 
			case ADB_INT32: 
			case ADB_AINC:
				fprintf(psTxb->ch,"%d",(INT) dValue);//atoi(arsFld[a].pBindBuffer));
				break;
		 
			case ADB_ALFA:
			case ADB_DATA:
			case ADB_BLOB:
			case ADB_TIMESTAMP:

				if (!pszValue) {
					fprintf(psTxb->ch,TXB_NULL);
					break;
				}

				//
				// Tolgo i due separatori dalla stringa a livello precauzionale
				//
				if (strstr(pszValue,TXB_SEP_COL)||strstr(pszValue,TXB_SEP_COL)) 
				{
					bError=TRUE;
					while (strReplace(pszValue,TXB_SEP_COL,""));
					while (strReplace(pszValue,TXB_SEP_ROW,""));
				}

				switch (arsFld[a].enEncoding)
				{
					default:
					case ULT_CS_LATIN1:
						pString=strEncode(pszValue,SE_UTF8,NULL);
						fprintf(psTxb->ch,"%s",pString);
						ehFree(pString);
						break;

					case ULT_CS_UTF8:
						fprintf(psTxb->ch,"%s",pszValue);
						break;
				}
				break;

			case ADB_BINARY:
			case ADB_GEOMETRY:
			case ADB_POINT:
				sizLen=sql_len(rsSet,arsFld[a].szName);
				if (!pszValue)
					fprintf(psTxb->ch,TXB_NULL);
					else {
					
						psz64=base64Encode(0,pszValue,sizLen);
						fprintf(psTxb->ch,"?B64?%s",psz64);
						ehFree(psz64);
					}
//				printf("qui");
				break;
			default:
				ehError();

		 }
		fprintf(psTxb->ch,psTxb->szColSep);
	}
	fprintf(psTxb->ch,"%s",psTxb->szRowSep);			
}


static EN_CHARTYPE _fldCollection(INT iFields,S_FLD_INFO * arsFld,CHAR * pszField) {
	
	INT a;
	for (a=0;a<iFields;a++) {
		if (!strcmp(arsFld[a].szName,pszField)) return arsFld[a].enEncoding;
	}
	return ULT_CS_UNKNOW;
}

static BOOL _fldNull(INT iFields,S_FLD_INFO * arsFld,CHAR * pszField) {
	
	INT a;
	for (a=0;a<iFields;a++) {
		if (!strcmp(arsFld[a].szName,pszField)) return arsFld[a].bIsNullable;
	}
	ehError();
	return false;
	
}

static S_FLD_INFO * _fldInfo(INT iFields,S_FLD_INFO * arsFld,CHAR * pszField) {
	
	INT a;
	for (a=0;a<iFields;a++) {
		if (!strcmp(arsFld[a].szName,pszField)) return &arsFld[a];
	}
	return NULL;
	
}


static void _addTxbFields(	BOOL bInsert,
							EH_LST lstQuery,
							// Dati destinazione
							INT iFields,		
							S_FLD_INFO * arsFld,
							// Dati srogente
							S_TXB * psTxb)
{
	INT k;
	S_FLD_INFO * psFieldDest=NULL;
	CHAR *	pE="";
//	CHAR	szServ[800];
	CHAR	szFormat[200];
	BOOL	bFree=false;

	for (k=0;k<psTxb->iFieldCount;k++)
	{
		CHAR * pTxb=NULL;
		CHAR * pString=NULL;
		EN_CHARTYPE enFldEncoding;
		EN_FLDTYPE enFldType=DataTypeToadb_Type(psTxb->lpFieldInfo[k].enType);

		psFieldDest=_fldInfo(iFields,arsFld,psTxb->lpFieldInfo[k].lpName);
		if (!psFieldDest) {
			continue;
		}

		if (!strEmpty(pE)) lstPush(lstQuery,pE);
		if (!bInsert) 
			lstPushf(lstQuery,"%s=",psFieldDest->szName);//arsFld[k].szName);

		switch (enFldType)
		{
			case ADB_NUME : 
			case ADB_COBD : 
			case ADB_COBN : 
			case ADB_FLOAT:
				sprintf(szFormat,"%%.%df",psTxb->lpFieldInfo[k].iInfo);
				lstPushf(lstQuery,szFormat,txbFldNume(psTxb,psTxb->lpFieldInfo[k].lpName));
				break;

			case ADB_BOOL :
			case ADB_INT  : 
			case ADB_INT32: 
			case ADB_AINC:
				lstPushf(lstQuery,"%d",txbFldInt(psTxb,psTxb->lpFieldInfo[k].lpName));
				break;

			case ADB_ALFA:
			case ADB_DATA:
			case ADB_BLOB:
			case ADB_TIMESTAMP:

				pTxb=txbFldPtr(psTxb,psTxb->lpFieldInfo[k].lpName);
				if (!strCmp(pTxb,TXB_NULL)) 
					pTxb=NULL;

				//
				// Se non è nullable
				//
				if (!_fldNull(iFields,arsFld,psTxb->lpFieldInfo[k].lpName)) pTxb=strEver(pTxb);

				// Controllo se il campo ha un parametro di CharSet UTF8 
				enFldEncoding=_fldCollection(iFields,arsFld,psTxb->lpFieldInfo[k].lpName);
				bFree=FALSE;

				if (!pTxb) pString=strDup("NULL");
				else {
					
					switch (enFldEncoding)
					{
						default:
						case ULT_CS_LATIN1:
							pString=strEncodeEx(1,pTxb,2,SE_UTF8,SE_SQLSTR);
							break;

						case ULT_CS_UTF8:
							pString=strEncode(pTxb,SE_SQLSTR,NULL);
							break;
					}
				}
				lstPush(lstQuery,pString);
				ehFree(pString);
				break;

		case ADB_BINARY:
		case ADB_POINT:
		case ADB_GEOMETRY:

			pTxb=txbFldPtr(psTxb,psTxb->lpFieldInfo[k].lpName);

			// Campo NULL
			if (!strCmp(pTxb,TXB_NULL)) {


				if (!_fldNull(iFields,arsFld,psTxb->lpFieldInfo[k].lpName)) {

					switch (enFldType)
					{
						case ADB_POINT:
						case ADB_GEOMETRY:
							lstPushf(lstQuery,"GeomFromText('Point(0 0)')");
							break;

						case ADB_BINARY:
							lstPushf(lstQuery,"X'0'");
							break;
					}

				} 
				else {

					lstPushf(lstQuery,"NULL");
				
				}
				
				// Campo binario con encodin B64
			} else if (!strBegin(pTxb,"?B64?")) {

				SIZE_T sizRet;
				BYTE * pb;
				CHAR * pszHex;

				pb=base64Decode(pTxb+5,&sizRet);
				pszHex=hexEncode(pb,sizRet);

				lstPush(lstQuery,"X'");
				lstPush(lstQuery,pszHex);
				lstPush(lstQuery,"'");

				ehFreePtrs(2,&pb,&pszHex);
			}
			break;
		default:
			ehError();
			break;
		}

		pE=",";
	}
}

//
// mysTableImport()
//
BOOL mysTableImport(DYN_SECTION_FUNC 
					UTF8 *	pszFileSource,
					CHAR *	pszTableDest,
					BOOL	bNotUpdate,
					BOOL	bShowProgress,
					EH_LST	lstErrors) {

	DYN_SECTION_GLOBALPTR
	S_TXB *		psTxb;
	INT	i,k;
	CHAR *	pszFields=NULL;
	CHAR * pszQuery;
//	DWORD	dwMaxSize=1000000;
	//CHAR	* pszBuffer=ehAllocZero(dwMaxSize+1);
	EH_LST	lstQuery=lstNew();
	_DMI	dmiFieldDest=DMIRESET;
	INT iFields; 
	S_FLD_INFO * arsFld;
	S_FLD_INFO * psFieldDest;

	if (!mysGetTableInfo(DYN_SECTIONC pszTableDest,&dmiFieldDest,NULL)) 
	{
		lstDestroy(lstQuery); 
		return true;
	}	
	arsFld=DMILock(&dmiFieldDest,NULL);
	iFields=dmiFieldDest.Num;

	//
	// Apro il txb
	//
	printf("\ropen %s .. ",fileName(pszFileSource));
	psTxb=txbOpen(pszFileSource); 
	if (!psTxb) 
	{	
		lstDestroy(lstQuery); 
		printf("Errore in apertura file %s",pszFileSource); 
		return true;
	}

	psTxb->iCharDecoding=ULT_CS_UTF8; // Richiedo conversione in UTF8

	//
	// costruisco parte per nome campi della query - INSERT INTO (campi)
	//
	pszFields=NULL;
	for (k=0;k<psTxb->iFieldCount;k++)
	{
		psFieldDest=_fldInfo(iFields,arsFld,psTxb->lpFieldInfo[k].lpName);
		if (!psFieldDest) {
			if (bShowProgress) printf("Campo %s inesistente nella tabella di destinazione." CRLF,psTxb->lpFieldInfo[k].lpName);
			continue;
		}
		if (!pszFields) strAssign(&pszFields,psTxb->lpFieldInfo[k].lpName);
		else {
			strCat(&pszFields,",");			
			strCat(&pszFields,psTxb->lpFieldInfo[k].lpName);
		}
	}

	//
	// valori per colonne
	//
	for (i=0;i<psTxb->iRealLines;i++)
	{
		CHAR *	pszTmp=NULL;
		
		lstClean(lstQuery);
		if (!bNotUpdate) 
			lstPush(lstQuery,"?INSERT INTO ");
			else
			lstPush(lstQuery,"?INSERT IGNORE INTO ");

		lstPush(lstQuery,pszTableDest);
		lstPush(lstQuery," (");
		lstPush(lstQuery,pszFields);
		lstPush(lstQuery,") VALUES (");

		if (bShowProgress) {
			if ((!(i%512)&&psTxb->iRealLines>0)||psTxb->iRealLines<1000) {  
				printf("\r%s < %d (%d%%) ...        ",pszTableDest,i,i*100/psTxb->iRealLines);
			}
		}

		txbRealGet(psTxb,i);
		
		_addTxbFields(	true,
						lstQuery,
							// Dati destinazione
						iFields,		
						arsFld,
							// Dati srogente
						psTxb);

		lstPushf(lstQuery,")");

		if (!bNotUpdate) {
		
			lstPush(lstQuery," ON DUPLICATE KEY UPDATE ");
			_addTxbFields(	false,
							lstQuery,
								// Dati destinazione
							iFields,		
							arsFld,
								// Dati srogente
							psTxb);
		
		}
		pszQuery=lstToString(lstQuery,"","",""); 
		if (mys_query(DYN_SECTIONC pszQuery)) {
			CHAR * psz=mys_QueryLastError(DYN_SECTIONC pszQuery);
			if (lstErrors) {
				lstPush(lstErrors,psz);
			}
			else ehExit("mysTableImport(): %s",psz);
//			ehFree(psz);
			//lstErrors
		}
		ehFree(pszQuery);

	}
//	ehFree(pszBuffer);
	ehFree(pszFields);
	lstDestroy(lstQuery);
	printf("\r%s completed (100%%).",pszTableDest);
	txbClose(psTxb);
	DMIClose(&dmiFieldDest,"field");

	return false;
}




static CHAR * _charEncoding(EN_CHARTYPE enEncoding) {

	switch (enEncoding) {
		default:
			ehError();
		case ULT_CS_UTF8: return "utf8";
		case ULT_CS_UNICODE: return "ucs2";
	}
	return "?";

}


//
// _getType()
//
CHAR * _getType(S_FLD_INFO * psFld)
{
	static CHAR szServ[800];
	CHAR *	pszBinary;

	strcpy(szServ,"?");
	switch (psFld->enFldType)
	{
		case ADB_ALFA:  
			pszBinary=""; if (!psFld->bCaseInsensible) pszBinary=" BINARY";
			if (psFld->enEncoding<1)
				sprintf(szServ,"VARCHAR(%d)%s",psFld->iSize,pszBinary); 
				else
				sprintf(szServ,"VARCHAR(%d)%s CHARACTER SET %s",psFld->iSize,pszBinary,_charEncoding(psFld->enEncoding)); 
			break;
					
		case ADB_BLOB:	
			pszBinary=""; //if (psFld->iType2) lpBinary=" BINARY";
			if (psFld->enEncoding<1)
				sprintf(szServ,"TEXT%s",pszBinary); 
				else
				{
					sprintf(szServ,"TEXT%s CHARACTER SET %s",pszBinary,_charEncoding(psFld->enEncoding));
				}
			break;

		case ADB_NUME: 
		case ADB_COBD:
		case ADB_COBN:  
			sprintf(szServ,"DECIMAL(%d,%d)",psFld->iSize,psFld->iDecimal);
			break;

		case ADB_FLOAT: 
			strcpy(szServ,"FLOAT");
			break;

		case ADB_AINC:	
			strcpy(szServ,"INT UNSIGNED AUTO_INCREMENT");// GENERATED ALWAYS AS IDENTITY (START WITH 1, INCREMENT BY 1)");
			break;


		case ADB_INT32: 
			strcpy(szServ,"INT"); 
			break;

		case ADB_BOOL:  
			strcpy(szServ,"BOOL"); 
			break;

		case ADB_INT: 
			strcpy(szServ,"SMALLINT"); 
			break;

		case ADB_DATA:  
			strcpy(szServ,"CHAR(8)"); 
			break;

		case ADB_GEOMETRY:
			strcpy(szServ,"GEOMETRY"); 
			break;

		case ADB_POINT:
			strcpy(szServ,"POINT"); 
			break;

		default:
			ehAlert("Tipo di campo %d, non gestito",psFld->enFldType);
			break;

	}


	if (!psFld->bIsNullable) strcat(szServ," NOT NULL");

	if (strCmp(psFld->szDefault,"?nil?")) {
		if (!*psFld->szDefault)
			strcat(szServ," DEFAULT \'\'");
			else
			strAppend(szServ," DEFAULT %s",psFld->szDefault);
	}

	if (strEmpty(psFld->szFieldBefore))
			strcat(szServ," FIRST");
			else
			{
				//DMIRead(&dmiClipFields,sClip.iItem-1,&s);
				strAppend(szServ," AFTER %s",psFld->szFieldBefore);
			}

	return szServ;
}

//
//
//
CHAR * mysFieldModify(EN_MESSAGE enMess,CHAR * pszTable,S_FLD_INFO * psFld) {

	CHAR * pszBuffer=ehAlloc(1024);
	CHAR * pszAction;
	switch (enMess) {

		case WS_UPDATE: pszAction="MODIFY"; break;
		case WS_INSERT: pszAction="ADD"; break;
		case WS_DEL:
			sprintf(pszBuffer,"ALTER TABLE %s DROP %s RESTRICT",pszTable,psFld->szName);
			return pszBuffer;

		default: break;

	}
	
	sprintf(pszBuffer,"ALTER TABLE %s %s %s %s",pszTable,pszAction,psFld->szName,_getType(psFld));
 	return pszBuffer;

}

// ALTER TABLE Asl MODIFY NOME VARCHAR(101) CHARACTER SET utf8 NOT NULL AFTER IDCODE


//
// _fldSearch()
//
INT mysFieldSearch(_DMI * pdmiFld,CHAR * pszName) {

	INT a;
	S_FLD_INFO * arsFld=DMILock(pdmiFld,NULL);
	for (a=0;a<pdmiFld->iLength;a++) {
		
		if (!strCaseCmp(arsFld[a].szName,pszName))  return a;

	}
	return -1;

}

INT mysTableSearch(_DMI * pdmiTbl,CHAR * pszName) {

	INT a;
	S_TBL_INFO * arsTbl=DMILock(pdmiTbl,NULL);
	for (a=0;a<pdmiTbl->iLength;a++) {
		
		if (!strCaseCmp(arsTbl[a].szName,pszName))  return a;

	}
	return -1;

}

//
//
//
static void _addFields(BOOL bInsert,
					   CHAR * pszBuffer,
					   INT iFields,
					   S_FLD_INFO * arsField,
					   SQL_RS rsSet) {
	
	INT k;
	CHAR * pE;
	CHAR * pszValue;

	pE="";
	for (k=0;k<iFields;k++)
	{
		CHAR * pTxb=NULL;
		CHAR * pString=NULL;
//		EN_CHARTYPE enFldEncoding;
		pszValue=sql_ptr(rsSet,arsField[k].szName); 

		strcat(pszBuffer,pE);
		if (!bInsert) strAppend(pszBuffer,"%s=",arsField[k].szName);
		switch (arsField[k].enFldType)
		{
			case ADB_NUME : 
			case ADB_COBD : 
			case ADB_COBN : 
			case ADB_FLOAT:
				
				if (!pszValue) pszValue="NULL";
				strcat(pszBuffer,pszValue);
				break;

			case ADB_BOOL :
			case ADB_INT  : 
			case ADB_INT32: 
			case ADB_AINC:
				if (!pszValue) pszValue="NULL";
				strcat(pszBuffer,pszValue);
				break;

			case ADB_ALFA:
			case ADB_DATA:
			case ADB_BLOB:

				pString=strEncode(pszValue,SE_SQLSTR,NULL);
				strcat(pszBuffer,pString);
				ehFree(pString);
				break;

			case ADB_BINARY:
			case ADB_POINT:
			case ADB_GEOMETRY:

				/*
				pTxb=txbFldPtr(psTxb,psTxb->lpFieldInfo[k].lpName);

				// Campo NULL
				if (!strCmp(pTxb,TXB_NULL)) {


					if (!_fldNull(iFields,arsFld,psTxb->lpFieldInfo[k].lpName)) {

						switch (enFldType)
						{
							case ADB_POINT:
							case ADB_GEOMETRY:
								strcat(pszBuffer,"GeomFromText('Point(0 0)')");
								break;

							case ADB_BINARY:
								strcat(pszBuffer,"X'0'");
								break;
						}

					} 
					else {

						strcat(pszBuffer,"NULL");
					
					}

					
					
					// Campo binario con encodin B64
				} else if (!strBegin(pTxb,"?B64?")) {

					SIZE_T sizRet;
					BYTE * pb;
					CHAR * pszHex;

					pb=base64Decode(pTxb+5,&sizRet);
					pszHex=hexEncode(pb,sizRet);

					strcat(pszBuffer,"X'");
					strcat(pszBuffer,pszHex);
					strcat(pszBuffer,"'");

					ehFreePtrs(2,&pb,&pszHex);
				}
				*/
				break;

			default:
				ehError();
				break;
			}
			pE=",";
		}
}

//
// _rowImport()
//
static BOOL _rowImport(S_MYSQL_SECTION * psecDest,
					   CHAR * pszTableDest,

					   INT iFields,
					   S_FLD_INFO * arsField,
					   CHAR * pszFields,	// Elenco dei campi per l'insert

					   CHAR * pszBuffer,	// Buffer da usare
					   SQL_RS rsSet,
					   BOOL bUpdate) {


	//
	// Preparo l'insert
	//
	if (!bUpdate) {

		// Insert / ignore
		strcpy(pszBuffer,"INSERT IGNORE INTO ");
		strcat(pszBuffer,pszTableDest);
		strcat(pszBuffer," (");
		strcat(pszBuffer,pszFields);
		strcat(pszBuffer,") VALUES (");
	}
	else {
	
		// Insert 
		strcpy(pszBuffer,"INSERT INTO ");
		strcat(pszBuffer,pszTableDest);
		strcat(pszBuffer," (");
		strcat(pszBuffer,pszFields);
		strcat(pszBuffer,") VALUES (");
	
	}
	_addFields(	true,pszBuffer,iFields,arsField,rsSet);
	strcat(pszBuffer,")");

	if (bUpdate) {
		strcat(pszBuffer," ON DUPLICATE KEY UPDATE ");
		_addFields(	false,pszBuffer,iFields,arsField,rsSet);
	
	
	}

#ifdef _MYSQL_MT
	mys_query(psecDest,pszBuffer);
#else
	mys_query(pszBuffer);
#endif
	return false;
}


//
// mysTableSync()
// Ritorna FALSE se tutto ok
//
BOOL mysTableSync(S_MYSQL_SECTION * psecSource,CHAR * pszTableSource,
				  S_MYSQL_SECTION * psecDest,CHAR * pszTableDest,
				  CHAR *	pszCode,
				  CHAR *	pszWhere,
				  BOOL		bOnlyIncrement,
				  BOOL		bAutoRemove,
				  INT		iBlockRecords,
				  BOOL		bShowProgress,
				  BOOL		bOperatorAsk) {


	_DMI dmiField=DMIRESET;
	_DMI dmiIndex=DMIRESET;
	S_FLD_INFO * arsFld;
	INT a;
	INT		iCount,iOffset,iMaxRecords,iBlock;
	CHAR *	pszQueryFields=NULL;
	SQL_RS rsSet;
	DWORD	dwMaxSize=300000;
	CHAR*	pszBuffer=ehAllocZero(dwMaxSize+1);
	EH_LST	lstCode;

#ifdef _MYSQL_MT 
#define _SectionSource psecSource,
#define _SectionDest psecDest,
#else
#define _SectionSource
#define _SectionDest
#endif

	//
	// Leggo informazioni tabella sorgente 
	//
	if (!mysGetTableInfo(_SectionSource pszTableSource,&dmiField,&dmiIndex)) return true;

	pszQueryFields=ehAlloc(dmiField.Num*50); *pszQueryFields=0;
	arsFld=DMILock(&dmiField,NULL);
	for (a=0;a<dmiField.Num;a++) {
		strAdd(pszQueryFields,arsFld[a].szName,",",false);
	}

	//
	// Determino codice da usare per i confronti
	//
	if (strEmpty(pszCode)) {
		if (mysFieldSearch(&dmiField,"IDCODE")>-1) pszCode="IDCODE";
	}
	if (strEmpty(pszCode)) {
		if (mysFieldSearch(&dmiField,"CODICE")>-1) pszCode="CODICE";
	}
	if (strEmpty(pszCode)&&(bOnlyIncrement||bAutoRemove)) ehError();


	//
	// Update/Insert del file (import totale)
	//
	lstCode=lstNew();
	iBlock=iBlockRecords;
	if (!bOnlyIncrement) {

		if (!strEmpty(pszWhere)) 
			iMaxRecords=sql_count(_SectionSource "%s WHERE %s",pszTableSource,pszWhere); 
			else	
			iMaxRecords=sql_count(_SectionSource pszTableSource);

		if (bShowProgress) printf("[%d] update ..." CRLF,iMaxRecords);
		for (iOffset=0;iOffset<iMaxRecords;iOffset+=iBlock) {
			iCount=iOffset;

			//
			// Prendo il blocco di dati dal dbase sorgente
			//
			sql_query(_SectionSource "SELECT %s FROM %s ORDER BY 1 ASC LIMIT %d,%d",pszQueryFields,pszTableSource,iOffset,iBlock);
			if (bShowProgress) printf("\rrichiedo a mySql da %d (%d records) [%d%%] ...     ",iOffset,iBlock,iOffset*100/iMaxRecords);
			rsSet=sql_stored(psecSource); 
			while (sql_fetch(rsSet)) {

				if (bShowProgress) {
					iCount++;
					if ((!(iCount%512)&&iMaxRecords>0)||iMaxRecords<1000) {  
						printf("\r%s > %d (%d%%) ...        ",pszTableSource,iCount,iCount*100/iMaxRecords);
					}
				}
				
				//
				// Insert o update della linea nella destinazione
				// 
				_rowImport(	psecDest,pszTableDest,

							dmiField.Num,arsFld,pszQueryFields,
	
							pszBuffer,	// Buffer da usare
							rsSet,
							true);
				if (bAutoRemove) 
				{
					EH_AR ar=ARFSplit(pszCode,"+");
					CHAR szServ[1024]="";
					for (a=0;ar[a];a++) {
						strAdd(szServ,sql_ptr(rsSet,ar[a]),"\1",true);
					}
					ehFree(ar);
					lstPush(lstCode,szServ);
				}

			}
			sql_free(rsSet);
		}
	}
	//
	// Solo incremento mancanti
	//
	else {

		CHAR szLastCode[30]="0";
		bAutoRemove=false; // non si può fare
		//
		// A) determino ultimo codice sulla destinazione
		//
		rsSet=sql_row(_SectionDest "SELECT %s FROM %s ORDER BY 1 DESC",pszCode,pszTableDest);
		if (rsSet) {
			strcpy(szLastCode,sql_ptr(rsSet,pszCode));
			sql_free(rsSet);
		}

		if (!strEmpty(pszWhere)) 
			iMaxRecords=sql_count(_SectionSource "%s WHERE (%s>%s AND (%s))",pszTableSource,pszCode,szLastCode,pszWhere); 
			else	
			iMaxRecords=sql_count(_SectionSource "%s WHERE (%s>%s)",pszTableSource,pszCode,szLastCode); 
		if (bShowProgress) printf("[%d] incremental ..." CRLF,iMaxRecords);

		//
		// B) Chiedo dal codice in avanti
		//
		for (iOffset=0;iOffset<iMaxRecords;iOffset+=iBlock) {
			iCount=iOffset;

			//
			// Prendo il blocco di dati dal dbase sorgente
			//
			sql_query(_SectionSource  "SELECT %s FROM %s WHERE %s>%s ORDER BY 1 ASC LIMIT %d,%d",
						pszQueryFields,
						pszTableSource,
						pszCode,szLastCode,
						iOffset,iBlock);

			printf("\rrichiedo a mySql da %d (%d records) [%d%%] ...     ",iOffset,iBlock,iOffset*100/iMaxRecords);
			rsSet=sql_stored(psecSource); 
			while (sql_fetch(rsSet)) {

				if (bShowProgress) {
					iCount++;
					if ((!(iCount%512)&&iMaxRecords>0)||iMaxRecords<1000) {  
						printf("\r%s > %d (%d%%) ...        ",pszTableSource,iCount,iCount*100/iMaxRecords);
					}
				}
				
				//
				// Insert o update della linea nella destinazione
				// 
				_rowImport(	psecDest,pszTableDest,

							dmiField.Num,arsFld,pszQueryFields,
	
							pszBuffer,	// Buffer da usare
							rsSet,
							true);
			}
			sql_free(rsSet);
		}

		// C) Eseguo importazione della query
	
	}

	if (bShowProgress) {
		printf("\r%s > %d (completed)." CRLF,pszTableDest,iMaxRecords);
	}

	//
	// in lstCode ho gli ID importatim quelli in più sono di troppo
	// Esegui la query sulla destinazione del solo codice e tolgo quelli non presenti nella lista in memoria
	//
	if (bAutoRemove) {
		
		INT iFields;
		EH_AR arFld=ARCreate(pszCode,"+",&iFields);
		CHAR szServ[1024], * pszValue;
		CHAR * pszCodes;
		EH_LST lstRemove=lstNew();

		if (bShowProgress) printf("Controllo records ..." CRLF);

		//
		// Creo una stringa con gli ID attuali
		//
		pszCodes=lstToString(lstCode,"\2","\2","\2");

		//
		// Carico tutti i codici locali per vedere se esitono degli ID in più
		//
		strcpy(szServ,pszCode); while (strReplace(szServ,"+",","));
		sql_query(_SectionDest "SELECT %s FROM %s",szServ,pszTableDest);
		rsSet=sql_stored(psecDest);
		while (sql_fetch(rsSet)) {

			if (!(rsSet->iCurrent%512)) printf("\rRimuovo records : controllo (%d/%d) ...",rsSet->iCurrent,rsSet->iRows);

			// MonoCampo
			if (iFields==1) {
				
				strcpy(szServ,"\2"); strcat(szServ,sql_ptr(rsSet,pszCode)); strcat(szServ,"\2");
//				if (!strstr(pszCodes,szServ)) lstPush(lstRemove,pszValue);
//				if (!lstSearch(lstCode,pszValue)) 
					
				
			// MultiCampo
			} else {
			
				//strcpy(szServ,"\2");
				*szServ=0;
				for (a=0;arFld[a];a++) {
					strAdd(szServ,sql_ptr(rsSet,arFld[a]),"\1",true);
				}
				strcat(szServ,"\2");
				strIns(szServ,"\2");
//				if (!lstSearch(lstCode,szServ)) 
//					lstPush(lstRemove,szServ);
			}
			if (!strstr(pszCodes,szServ)) 
				lstPush(lstRemove,strOmit(szServ,"\2"));

		}
		ehFree(pszCodes);
		
		//
		// Cancellazione ID
		//
		if (lstRemove->iLength) {
			if (bShowProgress) printf("Rimuovo %d records" CRLF,lstRemove->iLength);
			for (lstLoop(lstRemove,pszValue)) {
				printf("remove %s ..." CRLF,pszValue);
				if (iFields==1) {
					sql_query(_SectionDest "DELETE FROM %s WHERE %s=%s",pszTableDest,pszCode,pszValue);
				} else {
					EH_AR arValue=ARFSplit(pszValue,"\1");
					*szServ=0;
					for (a=0;arFld[a];a++) {
						if (*szServ) strcat(szServ," AND ");
						strAppend(szServ,"%s='%s'",arFld[a],arValue[a]);
					}	
					//printf("%s",szServ); _getch();
//					printf("DELETE FROM %s WHERE (%s)",pszTableDest,szServ);
					sql_query(_SectionDest "DELETE FROM %s WHERE (%s)",pszTableDest,szServ);
					ehFree(arValue);

				}

			}
		} else {
			printf("Nessun record da rimuovere." CRLF);
		}
		ARDestroy(arFld);
		lstDestroy(lstRemove);

	}

	//
	// Chiudo
	//

	DMIClose(&dmiField,"Field");
	ehFree(pszQueryFields);
	return FALSE;	

}




//
// mysSchemaSync()
//
// myMedixal 
// è il dbase di base Template che viene usato 
//
// - per permettere di programmare gli applicativi
// - Come base di confronto per sincronizzare il dbase in uso
//

#ifndef _MYSQL_MT 
static void _dbTableStructSync(CHAR * pszTableRef,CHAR * pszTableTarget,BOOL bOperatorAsk);

void mysSchemaSync(CHAR *	pszSchemaSource,
				   CHAR *	pszSchemaTarget,
				   CHAR *	pszSchemaImport,
				   BOOL		bOperatorAsk) // T/F se l'operatore deve dare l'ok (getch)
{

	CHAR	szTableSource[500];
	CHAR	szTableDest[500];
	BOOL	bError;
	CHAR *	putfFileTemp="c:\\comFerra\\fileTempDb.txb";
	S_TBL_INFO * arsRef,* arsTarget;
	
	_DMI	dmiRef=DMIRESET;
	_DMI	dmiTarget=DMIRESET;
	INT		a,idx=0;

	if (!strCaseCmp(pszSchemaSource,pszSchemaTarget)) ehError(); // Se sono uguale errore !!
	arsRef=mysGetSchema(pszSchemaSource,&dmiRef); // Schema di riferimento

	if (!arsRef) ehError();
	arsTarget=mysGetSchema(pszSchemaTarget,&dmiTarget);

	//
	// Creo il dbase
	//
	if (!arsTarget) {
		
		sql_query("CREATE DATABASE %s",pszSchemaTarget);
		arsTarget=mysGetSchema(pszSchemaTarget,&dmiTarget);
		if (!arsTarget) ehError();
	
	}
  	
	//
	// Creo/Sincronizzo gli schemi
	//

	for (a=0;a<dmiRef.iLength;a++) {

		CHAR * pszTable=arsRef[a].szName;
		if (*pszTable=='_') continue;

		printf("%d:%s" CRLF,a,pszTable);
		if (arsRef[a].enTableType==TBL_BASE)
		{
			idx=mysTableSearch(&dmiTarget,pszTable);

			//
			//	Verifico se esiste
			//  NO = Creo ed importo
			//
			if (idx<0) {

				bError=sql_query(	"CREATE TABLE %s.%s LIKE %s.%s",
									pszSchemaTarget,pszTable,
									pszSchemaSource,pszTable);		

				// Importo i dati
				if (!strEmpty(pszSchemaImport)) {

					sprintf(szTableSource,"%s.%s",pszSchemaImport,pszTable);
					sprintf(szTableDest,"%s.%s",pszSchemaTarget,pszTable);

					// sprintf(szTableSource,"%s.%s",pszSchemaImport,pszTable);
					if (strCaseCmp(szTableSource,szTableDest)) {
						if (!mysTableExport(szTableSource,putfFileTemp,NULL,true,NULL)) {
							mysTableImport(putfFileTemp,szTableDest,false,true,NULL);
						} else {
							ehError();
						}
					}			

				}

			} else {

			//
			//  SI = Sincronizzo la tabella
			//

				sprintf(szTableSource,"%s.%s",pszSchemaSource,pszTable);
				sprintf(szTableDest,"%s.%s",pszSchemaTarget,pszTable);
				_dbTableStructSync(szTableSource,szTableDest,bOperatorAsk);
			
			}

		}
		

		//
		// Vista logica
		//
		if (arsRef[a].enTableType==TBL_VIEW)
		{
			SQL_RS rsSet;
			CHAR * pszBuffer;
			CHAR szSearch[200],szReplace[200];

		// 	printf("qui: SHOW CREATE VIEW %s.%s" CRLF,pszSchemaSource,pszTable);

			sql_query("SHOW CREATE VIEW %s.%s",pszSchemaSource,pszTable);
			rsSet=sql_store(); sql_fetch(rsSet);
			if (!rsSet) ehError();

			pszBuffer=ehAlloc(120000);
			strCpy(pszBuffer,sql_ptr(rsSet,"2"),120000);
			sprintf(szSearch,"`%s`",pszSchemaSource);
			sprintf(szReplace,"`%s`",pszSchemaTarget);
			while (strCaseReplace(pszBuffer,szSearch,szReplace));
			strCaseReplace(pszBuffer,"CREATE ","CREATE OR REPLACE ");
			sql_query("%s",pszBuffer);
			ehFree(pszBuffer);
			sql_free(rsSet);
		}

	}
	fileRemove(putfFileTemp);

	//
	// Controllo tabella cambiate
	//
	for (a=0;a<dmiTarget.iLength;a++) {

		CHAR * pszTable=arsTarget[a].szName;
		if (arsTarget[a].enTableType!=TBL_BASE) continue;

		if (*pszTable=='_') continue;
		idx=mysTableSearch(&dmiRef,pszTable);

		//
		// Cancello (rinomino) la tabella che non esiste più
		//
		if (idx<0) {

			sprintf(szTableSource,"%s.%s",pszSchemaTarget,pszTable);
			sprintf(szTableDest,"%s._%s_%s",pszSchemaTarget,dtNow(),pszTable);
			printf("rename %s > %s" CRLF,szTableSource,szTableDest);
			_getch();
			sql_query("ALTER TABLE %s RENAME TO %s",szTableSource,szTableDest);

		}
	
	}


}

//
// _idxToStr() > Trasforma in una stringa la definizione dell'indice
//
static CHAR * _idxToStr(_DMI * pdmiIdxTarget,CHAR * pszName,CHAR * pszTable) {

	EH_LST lst=lstNew();
	S_IDX_INFO sIndex;
	INT a;
	CHAR * psz;

	for (a=0;a<pdmiIdxTarget->iLength;a++) {
		
		DMIRead(pdmiIdxTarget,a,&sIndex);
		if (strcmp(sIndex.szName,pszName)) continue;
		if (!lst->iLength) {
		
			lstPush(lst,"CREATE ");
			if (sIndex.bUnique) lstPush(lst,"UNIQUE ");
			lstPushf(lst,"INDEX %s ON %s (",pszName,pszTable);

		} else 
			lstPush(lst,",");
		
		lstPush(lst,sIndex.szField);
		if (*sIndex.szCollation=='A') lstPush(lst," ASC"); else lstPush(lst," DESC");
	}
	if (lst->iLength) lstPush(lst,")");
	psz=lstToString(lst,"","","");
	if (strEmpty(psz)) ehFreePtr(&psz);
	lstDestroy(lst);
	return psz;

}
/*
		 strcpy(szServ2,FhdElement.szName); strupr(szServ2);
		 while (strReplace(szServ2," ","_"));

		 strcpy(szServ,"CREATE");
		 if (!strcmp(FhdElement.psIndexInfo->pType,"FULLTEXT")||
			 !strcmp(FhdElement.psIndexInfo->pType,"SPATIAL"))
		 {
			sprintf(strNext(szServ)," %s INDEX %s ON ",FhdElement.psIndexInfo->pType,szServ2);
		 }
		 else
		 {
			sprintf(strNext(szServ),"%sINDEX %s ON ",lpUnique,szServ2);
		 }
						//SQLTableName(enPlatform,lpLibrary,szName));
		 strcat(szServ,szTable);//SQLTableName(enPlatform,lpLibrary,szTable));
		 strcat(szServ," ("); 
		 strcat(lpBuffer,szServ); 

		 //
		 // Elenco i campi dell'indice
		 //
		 arField=ARCreate(FhdElement.psIndexInfo->lpCompos,"+",NULL);
		 for (f=0;arField[f];f++)
		 {
			EH_AR arName=ARCreate(arField[f]," ",NULL);
			if (f) strcat(lpBuffer,",");
			if (LocalFldFind(idxTable,arName[0],&FhdOtherElement)) 
			{
				win_infoarg("Creazione Indice %s:\n"
							"il campo %s non esiste",
							szServ2,arName[0]);
				ARDestroy(arName);
				break;
			}
			strcat(lpBuffer,arField[f]);

			if (FhdOtherElement.psFieldInfo->iType==ADB_BLOB // Se non siamo in Fulltext
				&&strcmp(FhdElement.psIndexInfo->pType,"FULLTEXT")) strcat(lpBuffer,"(20)"); // Aggiungo la dimensione fissa a quelli a lunghezza dinamica
			// if (FhdElement.psIndexInfo->fSort) strcat(lpBuffer," DESC"); else strcat(lpBuffer," ASC");
			if (!strcmp(FhdElement.psIndexInfo->pType,"DESC")) strcat(lpBuffer," DESC");
			if (!strcmp(FhdElement.psIndexInfo->pType,"ASC")) strcat(lpBuffer," ASC");
			ARDestroy(arName);

		 }
		 strcat(lpBuffer,")"); 
		 if (iLanguageMode==1) strcat(lpBuffer,"\\");
		 if (iLanguageMode==2) strcat(lpBuffer,";");
		 ARDestroy(arField);


		 ARAdd(&arQuery,lpBuffer);
*/

//
// _dbTableStructSync()
//
static void _dbTableStructSync(CHAR * pszTableRef,CHAR * pszTableTarget,BOOL bOperatorAsk) {

	_DMI dmiRef=DMIRESET;
	_DMI dmiTarget=DMIRESET;
	_DMI dmiIdxRef=DMIRESET;
	_DMI dmiIdxTarget=DMIRESET;
	S_FLD_INFO * arsRef,* arsTarget, * psFld, * psFldRef;
	S_FLD_INFO sFldRef,sFld;
	INT a,idx;
	S_IDX_INFO sIndex;
	CHAR * pszCreateTarget;
	CHAR * pszCreateRef;

	arsRef=mysGetTableInfo(pszTableRef,&dmiRef,&dmiIdxRef);
	arsTarget=mysGetTableInfo(pszTableTarget,&dmiTarget,&dmiIdxTarget);
	
	//
	// COLONNE: Sincronia sulle colonne per verificare coerenza dei campi
	//
	for (a=0;a<dmiTarget.iLength;a++) {

		psFld=arsTarget+a;
		memcpy(&sFld,psFld,sizeof(S_FLD_INFO)); _(sFld.szFieldBefore);
		idx=mysFieldSearch(&dmiRef,psFld->szName);

		//
		// Il campo non esiste più, è da cancellare
		//
		if (idx<0) {


			CHAR * pszQuery=mysFieldModify(WS_DEL,pszTableTarget,psFld);
			printf("Rimuovo %s.%d " CRLF " -> %s" CRLF,pszTableTarget,psFld->szName,pszQuery); 
			if (bOperatorAsk) _getch();
			sql_query("%s",pszQuery);
			ehFree(pszQuery);
		
		} else {

		//
		// Il campo esiste ma è differente
		//

			psFldRef=arsRef+idx;
			memcpy(&sFldRef,psFldRef,sizeof(S_FLD_INFO));  _(sFldRef.szFieldBefore);
			
			if (memcmp(&sFldRef,&sFld,sizeof(S_FLD_INFO))) {
			
				CHAR * pszQuery=mysFieldModify(WS_UPDATE,pszTableTarget,psFldRef);
				printf("-> %s" CRLF,pszQuery); 
				if (bOperatorAsk) _getch();
				sql_query("%s",pszQuery);
				ehFree(pszQuery);

			}
		
		}
	}

	//
	// Loop sul reference per verificare se ci sono campi nuovi
	//
	for (a=0;a<dmiRef.iLength;a++) {

		psFldRef=arsRef+a;
		memcpy(&sFldRef,psFldRef,sizeof(S_FLD_INFO)); _(sFldRef.szFieldBefore);
		idx=mysFieldSearch(&dmiTarget,psFldRef->szName);
		
		// Campo nuovo
		if (idx<0) {

			CHAR * pszQuery=mysFieldModify(WS_INSERT,pszTableTarget,psFldRef);
			printf("-> %s" CRLF,pszQuery); 
			if (bOperatorAsk) _getch();
			sql_query("%s",pszQuery);
			ehFree(pszQuery);		
		}
	}


	//
	// INDICI: Sincronia Indici
	//

	for (a=0;a<dmiIdxTarget.iLength;a++) {
		
		DMIRead(&dmiIdxTarget,a,&sIndex); 
		if (sIndex.iSeq!=1) continue;
		pszCreateTarget=_idxToStr(&dmiIdxTarget,sIndex.szName,"test");
		pszCreateRef=_idxToStr(&dmiIdxRef,sIndex.szName,"test");

		//
		// L'indice è cambiato
		//
		if (strCmp(pszCreateTarget,pszCreateRef)) {
		
			//
			// Cancello il vecchio o il non usato
			//
			sql_query("ALTER TABLE %s DROP INDEX %s",pszTableTarget,sIndex.szName); // Cancello il vecchio

			//
			// Se presente Creo il nuovo 
			//
			if (pszCreateRef) {
				ehFree(pszCreateRef);
				pszCreateRef=_idxToStr(&dmiIdxRef,sIndex.szName,pszTableTarget);
				printf("-> ReplaceIndex: %s" CRLF,pszCreateRef); 
				if (bOperatorAsk) _getch();
				sql_query("%s",pszCreateRef);
			}
		}
		ehFreePtrs(2,&pszCreateTarget,&pszCreateRef);
	}

	//
	// Cerco quelli nuovi
	//

	for (a=0;a<dmiIdxRef.iLength;a++) {
		
		DMIRead(&dmiIdxRef,a,&sIndex); 
		if (sIndex.iSeq!=1) continue;
		pszCreateRef=_idxToStr(&dmiIdxRef,sIndex.szName,"test");
		pszCreateTarget=_idxToStr(&dmiIdxTarget,sIndex.szName,"test");

		//
		// Creo Nuovo indice non presente nel targhet
		//
		if (!pszCreateTarget) {
		
			ehFree(pszCreateRef);
			pszCreateRef=_idxToStr(&dmiIdxRef,sIndex.szName,pszTableTarget);
			printf("-> New Index: %s" CRLF,pszCreateRef); 
			if (bOperatorAsk) _getch();
			sql_query("%s",pszCreateRef);
		}
		ehFreePtrs(2,&pszCreateTarget,&pszCreateRef);
	}



	DMIClose(&dmiRef,"ref");
	DMIClose(&dmiTarget,"ref");
	DMIClose(&dmiIdxRef,"ref");
	DMIClose(&dmiIdxTarget,"ref");



}
#endif 