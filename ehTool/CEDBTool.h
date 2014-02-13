//   ---------------------------------------------
//	 | CEDBTool.h
//	 |                                       
//	 |								by Ferrà 2003
//   ---------------------------------------------

#define FDBDICTIONARY "_Dictionary"
// dwFlag  - (CREATE_ALWAYS,OPEN_EXISTING,OPEN_ALWAYS)
BOOL		CEDB_MountingVolume(PCEGUID,WCHAR *lpwDbase,DWORD dwFlag);
CEOID		CEDB_Create(PCEGUID pceVolume,CHAR *lpNome,DWORD dwNewDBType,WORD wNumSortOrder, SORTORDERSPEC * rgSortSpecs);

HANDLE CEDB_Open(PCEGUID pceVolume,
				 CHAR *lpNome,
				 DWORD dwFlag,
				 CEPROPID CePropID, // Selezione dell'indice di ordinamento (Identificatore)
				 CEOID *lpCeOid);

void		CEDB_OidInfo(PCEGUID pceguid,CEOID CeOID);
void		CEDB_ShowPropDesc(WORD cPropID, HANDLE hDbase, LONG dwCurrentRecord);
CEPROPVAL *	CEDB_FldAlloc(WORD wNum,SINT *lpHdl,CHAR *lpWho);
BOOL		CEDB_FldFree(SINT hdlField);
//void		CEDB_FldWrite(CEPROPID propid,WORD wFlags,double dNume,TCHAR *str);
void CEDB_FldWriteDirect(CEPROPVAL *lpCpl,
						 WORD nNum,
						 WORD wType,
						 ULONG lNume,
						 void *str,
						 WORD wFlags
						 );
SINT CEDB_InsertDirectFree(HANDLE hNewDb, CEPROPVAL *lpCpl, WORD wProp);
WORD CEDB_ADBTypeConvert(SINT iTipo);
/*
CEPROPID propid;
WORD wLenData;
WORD wFlags;
CEVALUNION val;
*/
#define FDB_BASE 100000

BOOL CEDB_FDBOpen(CHAR *lpDbase);
BOOL CEDB_FDBClose(void);
