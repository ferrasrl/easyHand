//   ---------------------------------------------
//	 ³ CEDBTool.c
//	 ³                                       
//	 ³                          by Ferrà 2003
//   ---------------------------------------------
#include "\ehtool\include\ehsw_idb.h"

#include <tchar.h>
#ifndef _WIN32_WCE
#include <rapi.h>
#endif
#include "\ehtool\CEDBTool.h"

/*
#define   PROP_SHORT        MAKELONG( CEVT_I2, 0) // column 0
#define   PROP_USHORT       MAKELONG( CEVT_UI2, 1) // column 1
#define   PROP_LONG         MAKELONG( CEVT_I4, 2)// column 2
#define   PROP_ULONG        MAKELONG( CEVT_UI4, 3)// column 3
#define   PROP_FILETIME     MAKELONG( CEVT_FILETIME, 4)// column 4
#define   PROP_LPWSTR       MAKELONG( CEVT_LPWSTR, 5)// column 5
#define   PROP_CEBLOB       MAKELONG( CEVT_BLOB, 6)// column 6
*/

// ------------------------------------------------------------------------
//
// ------------------------------------------------------------------------
BOOL CEDB_MountingVolume(PCEGUID pceguid, WCHAR *lpwDbase,DWORD dwFlag)
{
  BOOLEAN bFinished = FALSE;  // Loop (enumeration) control
  if (!CeMountDBVol(pceguid,         // Pointer to a CEGUID.
                    lpwDbase,         // Database-volume name.
                    dwFlag))    // Create the database volume
                                      // if it does not exist.
  {
    win_infoarg(TEXT("ERROR: CeMountDBVol failed (%ld)"),GetLastError());
	return TRUE;
  }
  return FALSE;
} // End of MountingDBVolume example code


CEOID CEDB_Create(PCEGUID pceVolume,CHAR *lpNome,DWORD dwNewDBType,WORD wNumSortOrder, SORTORDERSPEC * rgSortSpecs)
{
	CEDBASEINFO CEDBInfo;
	CEOID CeOid=0;
	ZeroFill(CEDBInfo);
	CEDBInfo.dwFlags=CEDB_VALIDDBFLAGS;//|CEDB_NOCOMPRESS;
	//CEDBInfo.dwFlags &= ~(CEDB_NOCOMPRESS);
	CharToUnicode(CEDBInfo.szDbaseName,lpNome);
	CEDBInfo.dwDbaseType=dwNewDBType;
	CEDBInfo.wNumSortOrder=wNumSortOrder;
	memcpy(CEDBInfo.rgSortSpecs,rgSortSpecs,wNumSortOrder*sizeof(SORTORDERSPEC));
	//CEDBInfo.rgSortSpecs=rgSortSpecs;
	
	if (!pceVolume)
	{
		CeOid=CeCreateDatabase(CEDBInfo.szDbaseName,CEDBInfo.dwDbaseType,wNumSortOrder,rgSortSpecs);
	//	win_infoarg("Entro qui [%s:%d]",lpNome,CeOid);
	}
	else
	{
		CeOid=CeCreateDatabaseEx(pceVolume, &CEDBInfo);
		//win_infoarg("Creo [%s]",lpNome);
	}

	if (!CeOid)
	{
        long lError;
#if defined(_WIN32_WCE)
        lError = GetLastError();
#else
        lError = CeGetLastError();
#endif
        switch(lError)
        {
         case ERROR_DISK_FULL:
           win_infoarg(TEXT("CEDB_Create(): Spazio insufficiente per creare il dbase [%s].\r\n"),lpNome);
           break;
         case ERROR_DUP_NAME:
           win_infoarg(TEXT("CEDB_Create(): Il db [%s] esistente.\r\n"),lpNome);
           break;
         default:
   			win_infoarg(TEXT("CEDB_Create(): CeCreateDatabase failed with error: %ld\r\n"), lError);
           break;
		}
    }
	return CeOid;
}

HANDLE CEDB_Open(PCEGUID pceVolume,
				 CHAR *lpNome,
				 DWORD dwFlag,
				 CEPROPID CePropID, // Selezione dell'indice di ordinamento (Identificatore)
				 CEOID *lpCeOid)
{
	WCHAR szBuffer[255];
	HANDLE hNewDb;
	CharToUnicode(szBuffer,lpNome);

	if (!pceVolume)
	{
	hNewDb=CeOpenDatabase(lpCeOid,
						  szBuffer,
						  CePropID,
						  dwFlag,//CEDB_AUTOINCREMENT,
						  NULL);
	}
	else
	{
	hNewDb=CeOpenDatabaseEx(pceVolume,
							lpCeOid,
							szBuffer,
							CePropID,
							dwFlag,//CEDB_AUTOINCREMENT,
							NULL);
	}
	if (hNewDb==INVALID_HANDLE_VALUE)
	{
        long lError;

#if defined(_WIN32_WCE)
        lError = GetLastError();
#else
        lError = CeGetLastError();
#endif
        switch(lError)
        {
         case ERROR_INVALID_PARAMETER :
           win_infoarg(TEXT("CEDB_Open(): Parametro non valido [%s].\n\r"),lpNome);
           break;
         case ERROR_FILE_NOT_FOUND:
           win_infoarg(TEXT("CEDB_Open(): Il db [%s] è inesistente.\n\r"),lpNome);
           break;
		 case ERROR_NOT_ENOUGH_MEMORY:
           win_infoarg(TEXT("CEDB_Open(): Memoria insufficiente per aprire [%s].\n\r"),lpNome);
           break;
         default:
   			win_infoarg(TEXT("CEDB_Open(): ERROR: CeCreateDatabase failed with error: %ld\n\r"), lError);
           break;
        }
	}
	return hNewDb;
}


SINT CEDB_recnum(PCEGUID pceguid,CEOID CeOID)
{
	CEOIDINFO  CeObject;
	if ( !CeOidGetInfoEx(pceguid, CeOID, &CeObject))
	{
		win_infoarg(TEXT("CEDB_recnum(); CeOidGetInfo failed with error (%ld)\r\n"), GetLastError());
	}
	else
	{
     if (CeObject.wObjType!=OBJTYPE_DATABASE) return 0;
	}
	return CeObject.infDatabase.wNumRecords;
}



void CEDB_OidInfo(PCEGUID pceguid,CEOID CeOID)
{
	CEOIDINFO  CeObject;

//	if ( !CeOidGetInfo(CeOID, &CeObject) )
	if ( !CeOidGetInfoEx(pceguid, CeOID, &CeObject))
	{
			win_infoarg(TEXT("ERROR: CeOidGetInfo failed with error (%ld)\r\n"), GetLastError());
	}
	else
	{
     switch(CeObject.wObjType)
     {
      case OBJTYPE_INVALID:
		win_infoarg(TEXT("The object store contains no valid object that has this object identifier. \n"));
		break;
      
	  case OBJTYPE_FILE:
		win_infoarg(TEXT("The object is a file. \n"));
		break;
      
	  case OBJTYPE_DIRECTORY:
		win_infoarg(TEXT("The object is a directory. \n"));
		break;
      
	  case OBJTYPE_DATABASE:
		win_infoarg(TEXT("The object is a database. \n"));
		break;
      
	  case OBJTYPE_RECORD:
		win_infoarg(TEXT("The object is a record inside a database. \n"));
		break;
      
	  default:
		win_infoarg(TEXT("Unrecognized wObjType\n"));
     }
	
	 win_infoarg(TEXT("DB Flags: %#lx\r\nDB Name: %ls\r\nDB Type: %ld\r\n#Records: %d\r\n#Sort Orders: %d\r\nSorted On:"),
		CeObject.infDatabase.dwFlags,
		CeObject.infDatabase.szDbaseName,
		CeObject.infDatabase.dwDbaseType,
		CeObject.infDatabase.wNumRecords,
		CeObject.infDatabase.wNumSortOrder);
/*
			for ( i = 0 ; i < CeObject.infDatabase.wNumSortOrder ; i++)
			{
				wsprintf(szSort, TEXT("\r\nRow: %ld\r\n"), CeObject.infDatabase.rgSortSpecs[i].propid);
						
				if ( CeObject.infDatabase.rgSortSpecs[i].dwFlags & CEDB_SORT_DESCENDING )
					_tcscat(szSort, TEXT("  DESCENDING \r\n"));

				if ( CeObject.infDatabase.rgSortSpecs[i].dwFlags & CEDB_SORT_CASEINSENSITIVE )
					_tcscat(szSort, TEXT("  CASEINSENSITIVE \r\n"));

				if ( CeObject.infDatabase.rgSortSpecs[i].dwFlags & CEDB_SORT_UNKNOWNFIRST )
					_tcscat(szSort, TEXT("  UNKNOWNFIRST \r\n"));

				if ( CeObject.infDatabase.rgSortSpecs[i].dwFlags & CEDB_SORT_GENERICORDER )
					_tcscat(szSort, TEXT("  GENERICORDER \r\n"));
				
				_tcscat(szBuf, szSort);

			}
			SendMessage(hwndEdit, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)szBuf);
*/
	}
}


void CEDB_ShowPropDesc(WORD cPropID, HANDLE hDbase, LONG dwCurrentRecord)
{
//	WCHAR		szBuf[200];
	//WORD		cPropID = 1;
	LPBYTE 		bData = NULL;
	DWORD		cbData = 0;
//	SYSTEMTIME	SystemTime;
	CEOID      CeOID = 0;
	DWORD		dwIndex;
	CEPROPID	CeProId=CEVT_I2;

	cPropID=0;
	if ( hDbase == INVALID_HANDLE_VALUE)
	{
		//wsprintf(szBuf, TEXT("ERROR: CeOpenDatabase failed to open database.(Error: %ld)\r\n"), GetLastError());
		//OutputDebugString(szBuf);
		//MessageBox(NULL, szBuf, TEXT("ERROR"), MB_OKCANCEL );
		win_infoarg(TEXT("Errore Handle dbase non valido"));
	}
	else
	{
		if ( dwCurrentRecord == -1 )
		{
			if ( !CeSeekDatabase(hDbase, CEDB_SEEK_CURRENT, 0, &dwIndex) )
			{
				dwIndex = 0;
				win_infoarg(TEXT("ERROR: CeSeekDatabase failed to get record index.(Error: %ld)\r\n"), GetLastError());
				//OutputDebugString(szBuf);
				//MessageBox(NULL, szBuf, TEXT("ERROR"), MB_OKCANCEL );
				//SendMessage(hwndEdit,WM_SETTEXT, 0, (LPARAM) (LPTSTR) TEXT(""));
				return;
			}
		}
		else
			dwIndex = dwCurrentRecord;

		// check for eof by reading after a seek failure
		if( !CeReadRecordProps(hDbase, CEDB_ALLOWREALLOC, &cPropID,&CeProId,&bData,&cbData) )
		{
			DWORD dwError;

			if ( (dwError=GetLastError()) != ERROR_NO_MORE_ITEMS )
			{
				win_infoarg(TEXT("ERROR: CeReadRecordProps failed to read database.(Error: %ld)\r\n"), dwError);
			}
		}
		
	}

	if( bData == NULL )
	{
		win_infoarg(TEXT("ERROR: CeReadRecordProps failed to read database.(Error: %ld)\r\n"), GetLastError());
		return;
	}

	win_infoarg(TEXT("Ritorna [%x : %d]"),bData,cbData);

	switch(TypeFromPropID(CeProId))
	{
		case CEVT_I2:
			win_infoarg(TEXT("Record: %ld\r\nProperty: Short\r\nDATA:\r\n%d"), 0, bData?((PCEPROPVAL) bData)->val.iVal: 0) ;
			break;
		case CEVT_UI2:
			win_infoarg(TEXT("Record: %ld\r\nProperty: Unsigned Short\r\nDATA:\r\n%d"), 0, bData?((PCEPROPVAL) bData)->val.uiVal: 0) ;
			break;
		case CEVT_I4:
			win_infoarg(TEXT("Record: %ld\r\nProperty: Long\r\nDATA:\r\n%d"), dwIndex, bData?((PCEPROPVAL) bData)->val.lVal: 0) ;
			break;
		case CEVT_UI4:
			win_infoarg(TEXT("Record: %ld\r\nProperty: Unsigned Long\r\nDATA:\r\n%d"), dwIndex, bData?((PCEPROPVAL) bData)->val.ulVal: 0) ;
			break;
		case CEVT_FILETIME:
			win_infoarg(TEXT("FILETIME"));
			/*
			if ( bData != NULL )
			{
				FileTimeToSystemTime(&(((PCEPROPVAL) bData)->val.filetime), &SystemTime);

				wsprintf(szBuf, TEXT("Record: %ld\r\nProperty: File Time\r\nDATA:\r\nYear:%d\r\nMonth:%d\r\n"
					TEXT("Day:%d\r\nDay of Week:%d\r\nHour:%d\r\nMinute:%d\r\nSecond:%d\r\nMilliseconds:%d")),
					dwIndex, SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay, SystemTime.wDayOfWeek,
					SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond, SystemTime.wMilliseconds) ;
			}
			else
				wsprintf(szBuf,TEXT("Record: %ld\r\nProperty: File Time"), dwIndex);
*/
			break;
		case CEVT_LPWSTR:
			/*
			wsprintf(szBuf, TEXT("Record: %ld\r\nProperty: String\r\nDATA:\r\n%-70s"), dwIndex,
				bData?((PCEPROPVAL) bData)->val.lpwstr: NULL) ;

			if ( bData && lstrlen(((PCEPROPVAL) bData)->val.lpwstr) > 70 )
				lstrcat(szBuf, TEXT("..."));
*/
			win_infoarg(TEXT("LPWSTR"));
			break;
		case CEVT_BLOB:
			win_infoarg(TEXT("BLOB"));
			/*
		{
			DWORD dwMaxShow = 15;
			TCHAR szTempBuf[6];
			DWORD i;
			wsprintf(szBuf,TEXT("Record: %ld\r\nProperty: BLOB\r\nDATA:\r\n"), dwIndex);
			if( bData )
			{
				dwMaxShow = min(dwMaxShow, ((PCEPROPVAL) bData)->val.blob.dwCount);
				for (i = 0; i < dwMaxShow; i++)
				{
				  wsprintf (szTempBuf, TEXT("%#2x,"), (BYTE)((PCEPROPVAL) bData)->val.blob.lpb[i]);
				  lstrcat (szBuf, szTempBuf);
				}
				if (dwMaxShow < ((PCEPROPVAL) bData)->val.blob.dwCount )
					lstrcat (szBuf, TEXT("..."));
			}
			*/
			break;

		default:
			win_infoarg(TEXT("Record: %ld\r\nUnknown record property [%d]"),0,TypeFromPropID(CeProId));
			break;
	}
	if ( bData ) LocalFree(bData);
//	SendMessage(hwndEdit,WM_SETTEXT, 0, (LPARAM)(LPCTSTR)szBuf);
}

/*
static void FCeMakeSample2(HANDLE hNewDb,SINT iTable)
{
	FILETIME        FileTime;
	SYSTEMTIME      SystemTime;
	WCHAR			*lpStr;
	WCHAR           *lpBlob;
//	HWND            hwndTV;
//	HWND            hParent;

	CEPROPVAL  NewRecProps[7];

	NewRecProps[0].propid = PROP_SHORT;
	NewRecProps[0].wLenData = 0;
	NewRecProps[0].wFlags = 0;
	NewRecProps[0].val.iVal = 10;

	NewRecProps[1].propid = PROP_SHORT;
	NewRecProps[1].wLenData = 0;
	NewRecProps[1].wFlags = 0;
	NewRecProps[1].val.iVal = 9;

	NewRecProps[2].propid = PROP_SHORT;
	NewRecProps[2].wLenData = 0;
	NewRecProps[2].wFlags = 0;
	NewRecProps[2].val.iVal = 8;

	NewRecProps[3].propid = PROP_SHORT;
	NewRecProps[3].wLenData = 0;
	NewRecProps[3].wFlags = 0;
	NewRecProps[3].val.iVal = 7;

	NewRecProps[4].propid = PROP_SHORT;
	NewRecProps[4].wLenData = 0;
	NewRecProps[4].wFlags = 0;
	NewRecProps[4].val.iVal = 6;

	NewRecProps[5].propid = PROP_SHORT;
	NewRecProps[5].wLenData = 0;
	NewRecProps[5].wFlags = 0;
	NewRecProps[5].val.iVal = 5;

	NewRecProps[6].propid = PROP_SHORT;
	NewRecProps[6].wLenData = 0;
	NewRecProps[6].wFlags = 0;
	NewRecProps[6].val.iVal = 4;

	NewRecProps[1].propid = PROP_USHORT;
	NewRecProps[1].wLenData = 0;
	NewRecProps[1].wFlags = 0;
	NewRecProps[1].val.uiVal = 1;

	NewRecProps[2].propid = PROP_SHORT;
	NewRecProps[2].wLenData = 0;
	NewRecProps[2].wFlags = 0;
	NewRecProps[2].val.iVal = 8;

	NewRecProps[2].propid = PROP_LONG;
	NewRecProps[2].wLenData = 0;
	NewRecProps[2].wFlags = 0;
	NewRecProps[2].val.lVal = 0xffffffff;
	NewRecProps[3].propid = PROP_ULONG;
	NewRecProps[3].wLenData = 0;
	NewRecProps[3].wFlags = 0;
	NewRecProps[3].val.ulVal = 0x7FFFFFFF;

	SystemTime.wYear = 1999;
	SystemTime.wMonth = 12;
	SystemTime.wDayOfWeek = 6;
	SystemTime.wDay = 25;
	SystemTime.wHour = 12;
	SystemTime.wMinute = 12;
	SystemTime.wSecond = 12;
	SystemTime.wMilliseconds = 12;

	SystemTimeToFileTime(&SystemTime,&FileTime);
	NewRecProps[4].propid = PROP_FILETIME;
	NewRecProps[4].wLenData = 0;
	NewRecProps[4].wFlags = 0;
	NewRecProps[4].val.filetime = FileTime;
								
	lpStr = (WCHAR *) LocalAlloc(LMEM_FIXED, 50 * sizeof(WCHAR));
	if ( lpStr ) wcscpy(lpStr, L"This is a string property");

	NewRecProps[5].propid = PROP_LPWSTR;
	NewRecProps[5].wLenData = 0;
	NewRecProps[5].wFlags = 0;
	NewRecProps[5].val.lpwstr = lpStr;

	lpBlob = (WCHAR *)LocalAlloc(LMEM_FIXED, 50  * sizeof(WCHAR));
	if ( lpBlob ) wcscpy(lpBlob, L"This is a blob property");

	NewRecProps[6].propid = PROP_CEBLOB;
	NewRecProps[6].wLenData = 0;
	NewRecProps[6].wFlags = 0;
	NewRecProps[6].val.blob.dwCount = 50  * sizeof(WCHAR);
	NewRecProps[6].val.blob.lpb  = (LPBYTE) lpBlob;

    //write record out with 7 property fields...
    if( !CeWriteRecordProps(hNewDb, 0, 7, NewRecProps) )
	{
									
									TCHAR szError[50];
									wsprintf(szError, TEXT("ERROR: CeWriteRecordProps failed (%ld)"), GetLastError());
									OutputDebugString(szError);
									MessageBox(hwndTV, szError, TEXT("Error"), MB_OKCANCEL );
									
									win_infoarg("Errore in scrittura record");
	}
	if( lpBlob ) LocalFree(lpBlob);
	if( lpStr ) LocalFree(lpStr);
}

*/

// -----------------------------------------------------
// CEDB_FieldAlloc
// Alloca (ed azzera) spazio per registrare un record
//
// Ritorna il puntatore alla struttura di definizione delle proprietà
#if !defined(_WIN32_WCE)
CEPROPVAL *CEDB_FldAlloc(WORD wNum,SINT *lpHdl,CHAR *lpWho)
{
	SINT hdl;
	BYTE *lpByte;
	DWORD dwSize=sizeof(CEPROPVAL)*wNum;
	hdl=memo_chiedi(RAM_AUTO,dwSize,lpWho);
	*lpHdl=hdl;
	lpByte=Wmemo_lock(hdl);
	memset(lpByte,0,dwSize);
	return (CEPROPVAL *) lpByte;
}

// -----------------------------------------------------
// CEDB_FieldFree
// Libera lo spazio per registrare un record
//
BOOL CEDB_FldFree(SINT hdlField)
{
	memo_libera(hdlField,"CEDBFree");
	return FALSE;
}
#endif

// -----------------------------------------------------
// ATTENZIONE: In caso di stringhe/blob alloca memoria
// -----------------------------------------------------

void CEDB_FldWriteDirect(CEPROPVAL *lpCpl,
						 WORD wNum, // Enumerazione del campo
						 WORD wType, // Tipo del campo
						 ULONG ulNume, // Valore numerico o lunghezza in caso di Blob
						 void *str, // Valore del campo per string/time / e blob
						 WORD wFlags) // Flags
{
	CEPROPVAL *lpCplDest=lpCpl+wNum;

	lpCplDest->propid=MAKELONG(wType, wNum);
    lpCplDest->wLenData=0;
    lpCplDest->wFlags=wFlags;
	ZeroFill(lpCplDest->val);

	switch(wType)
	{
		case CEVT_I2:	lpCplDest->val.iVal=(INT) ulNume; break;
		case CEVT_UI2:	lpCplDest->val.uiVal=(UINT) ulNume; break;
		case CEVT_I4:	lpCplDest->val.lVal=(LONG) ulNume; break;
		case CEVT_UI4:	lpCplDest->val.ulVal=(ULONG) ulNume; break;
		case CEVT_FILETIME:
			win_infoarg(TEXT("FILETIME DA FARE"));
			/*
			if ( bData != NULL )
			{
				FileTimeToSystemTime(&(((PCEPROPVAL) bData)->val.filetime), &SystemTime);

				wsprintf(szBuf, TEXT("Record: %ld\r\nProperty: File Time\r\nDATA:\r\nYear:%d\r\nMonth:%d\r\n"
					TEXT("Day:%d\r\nDay of Week:%d\r\nHour:%d\r\nMinute:%d\r\nSecond:%d\r\nMilliseconds:%d")),
					dwIndex, SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay, SystemTime.wDayOfWeek,
					SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond, SystemTime.wMilliseconds) ;
			}
			else
				wsprintf(szBuf,TEXT("Record: %ld\r\nProperty: File Time"), dwIndex);
*/
			break;
		case CEVT_LPWSTR:
			//win_infoarg("LPWSTR");
			lpCplDest->val.lpwstr = (WCHAR *) LocalAlloc(LMEM_FIXED, (strlen(str)+1) * sizeof(WCHAR));
			CharToUnicode(lpCplDest->val.lpwstr,str);
			break;
		case CEVT_BLOB:
			//win_infoarg("BLOB DA FARE");
			lpCplDest->val.blob.dwCount=(DWORD) ulNume;
			lpCplDest->val.blob.lpb=LocalAlloc(LMEM_FIXED, lpCplDest->val.blob.dwCount);
			memcpy(lpCplDest->val.blob.lpb,str,lpCplDest->val.blob.dwCount);
			/*
		{
			DWORD dwMaxShow = 15;
			TCHAR szTempBuf[6];
			DWORD i;
			wsprintf(szBuf,TEXT("Record: %ld\r\nProperty: BLOB\r\nDATA:\r\n"), dwIndex);
			if( bData )
			{
				dwMaxShow = min(dwMaxShow, ((PCEPROPVAL) bData)->val.blob.dwCount);
				for (i = 0; i < dwMaxShow; i++)
				{
				  wsprintf (szTempBuf, TEXT("%#2x,"), (BYTE)((PCEPROPVAL) bData)->val.blob.lpb[i]);
				  lstrcat (szBuf, szTempBuf);
				}
				if (dwMaxShow < ((PCEPROPVAL) bData)->val.blob.dwCount )
					lstrcat (szBuf, TEXT("..."));
			}
			*/
			break;

		default:
			//win_infoarg("Record: %ld\r\nUnknown record property [%d]",0,TypeFromPropID(CeProId));
			break;
	}
//	if ( bData ) LocalFree(bData);
}


SINT CEDB_InsertDirectFree(HANDLE hNewDb, CEPROPVAL *lpCpl, WORD wProp)
{
	WORD i;
	LONG lError=0;
	if (!CeWriteRecordProps(hNewDb, 0, wProp, lpCpl))
	{
		lError=GetLastError();
	}
	for (i=0;i<wProp;i++)
	{
		switch (TypeFromPropID(lpCpl[i].propid))
		{
		case CEVT_LPWSTR: 
			if (lpCpl[i].val.lpwstr)
			{
				LocalFree(lpCpl[i].val.lpwstr); 
				lpCpl[i].val.lpwstr=NULL;
			}
			break;

		case CEVT_BLOB:
			if (lpCpl[i].val.blob.lpb)
			{
				LocalFree(lpCpl[i].val.blob.lpb); 
				lpCpl[i].val.blob.lpb=NULL;
			}
			break;
		}
	}
	return lError;
}

WORD CEDB_ADBTypeConvert(SINT iTipo)
{
	switch (iTipo)
	{
			case ADB_ALFA: // ALFA
			case ADB_NUME: // NUME
			case ADB_DATA: // DATA
			case ADB_FLOAT: // FLOAT
			case ADB_COBD: // COB-D
			case ADB_COBN: // COB-N
			case ADB_BLOB: // BLOB
			default:
				return CEVT_LPWSTR;
								
			case ADB_INT: // INT16
			case ADB_BOOL: // FLAG
				return CEVT_I2;

			case ADB_AINC:  // AUTOINC
			case ADB_INT32: // UINT32
				return CEVT_UI4;
	}
	return 0;
}

// Apro ed alloco in memoria FDB
typedef struct {
	BYTE iTipo;
	CHAR szNome[33];
	ULONG uldato;
} S_FDB;
static CEGUID ceFDBVolume;
DRVMEMOINFO dmiFDB=DMIRESET;
BOOL CEDB_FDBOpen(CHAR *lpDbase)
{
	HANDLE hDict;
	CEOID CeOid=0;
	BOOL fError=FALSE;
	WCHAR szwNewDBName[MAX_PATH];
	
	CharToUnicode(szwNewDBName,lpDbase);
	if (CEDB_MountingVolume(&ceFDBVolume,szwNewDBName,OPEN_EXISTING)) return TRUE;

	//win_infoarg("Esiste [%s]",lpDbase);
	/*
	hDict=CEDB_Open(pceVolume,
					FDBDICTIONARY,
					0,
					MAKELONG(CEVT_UI2,0),
					&CeOid);
	// Apro la tabella del dizionario

	hDict=CeFindFirstDatabaseEx(&ceFDBVolume,FDB_BASE);
	if (hDict)
	{
		win_infoarg("OK");

		CeOid=CeFindNextDatabaseEx(hDict,&ceFDBVolume);
		if (CeOid)
		{
			CEDB_OidInfo(&ceFDBVolume,CeOid);
		} else win_infoarg("Errore");
		CeCloseHandle(hDict);
	}

					*/
	hDict=CEDB_Open(&ceFDBVolume,FDBDICTIONARY,CEDB_AUTOINCREMENT,0,&CeOid);

	if (hDict!=INVALID_HANDLE_VALUE)
	{
		SINT iRec;
//		WORD wPropID;
//		DWORD dwDataBuffer;
//		PCEPROPVAL lpData;
		iRec=CEDB_recnum(&ceFDBVolume,CeOid);
		// DMIOpen(&dmiFDB,RAM_AUTO,iRec,sizeof(S_FDB),"FDB");

		// Alloco il dizionario
		//CEDB_OidInfo(&ceFDBVolume,CeOid);
		//printf("\n\r");

//		win_infoarg(TEXT("OK1"));
/*
		while (TRUE)
		{
			lpData=NULL;
			wPropID=0;
			dwDataBuffer=0;
			CeOid=CeReadRecordProps(hDict, 
									CEDB_ALLOWREALLOC, 
									&wPropID, // Numero di proprietà lette
									NULL,
									(LPBYTE *) &lpData,
									&dwDataBuffer);
			if (!CeOid) break;
			//printf("[%d] %d - %d\n\r",CeOid, lpData[0].val.uiVal,lpData[1].val.uiVal);
			//win_infoarg(TEXT("%d - %d\n\r"),lpData[0].val.uiVal, lpData[1].val.uiVal);
				
			if (lpData) LocalFree(lpData);
		}
		*/
//		win_infoarg(TEXT("OK2"));


#if defined(_WIN32_WCE)
		CloseHandle(hDict); // Chiudo il dizionario	
#else
		CeCloseHandle(hDict); // Chiudo il dizionario	
#endif
	}
	else fError=TRUE;

	if (fError) 
	{
		CeUnmountDBVol(&ceFDBVolume);
		return TRUE;
	}

	return FALSE;
}

BOOL CEDB_FDBClose(void)
{
	// DMIClose(&dmiFDB,"FDB");
	CeFlushDBVol(&ceFDBVolume);
	CeUnmountDBVol(&ceFDBVolume);
	return FALSE;
}

