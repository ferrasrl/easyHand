//   +-------------------------------------------+
//   | mdb - Interfaccia i files .mdb (access)
//   |             
//   |							Ferrà srl 10/2013
//   +-------------------------------------------+

// Reference:
// http://msdn.microsoft.com/en-us/library/cc811599(v=office.12).aspx
// ADO API Reference: http://msdn.microsoft.com/en-us/library/windows/desktop/ms678086(v=vs.85).aspx
//

#include "/easyhand/inc/easyhand.h"
//#import "msado15.dll" no_namespace rename("EOF", "EndOfFile")

#include <oledb.h>
#include <stdio.h>
#include <conio.h>
#include "icrsint.h"
#import <C:\\Program Files\\Common Files\\System\\ado\\msado15.dll> no_namespace rename( "EOF", "EndOfFile" )

#ifndef EH_MDB
#include "/easyhand/inc/eh_mdb.h"
#endif

static struct {
	BOOL bReady;
	_RecordsetPtr pRst;
} _s={0,NULL};

const char * _DAM = "ADO";

inline void TESTHR(HRESULT x) { if FAILED(x) ehError();};
static void _PrintProviderError(_ConnectionPtr pConnection);
static void _PrintComError(_com_error &e);
static void _test( EH_MDB_SECTION * psSection);
static bool _GetRowsOK(_RecordsetPtr pRstTemp,int intNumber, _variant_t& avarData);

//
// Connessione
//
EH_MDB_SECTION *	mdbConnect(	CHAR *	pszFileMdb,
								CHAR *  pszConnectionString,	
								CHAR *	pszUser,
								CHAR *	pszPassword) {

	EH_MDB_SECTION * psSection=(EH_MDB_SECTION *) ehAllocZero(sizeof(EH_MDB_SECTION));
	CHAR szConnectionString[800];
//	INT hr;


	if (!_s.bReady) {
		
		if ( FAILED(::CoInitialize(NULL)) ) ehError();
		_s.bReady=true;
		// ::CoUninitialize(); alla fine
	}

//	Database * oDb=new Database();

	if (!strEmpty(pszConnectionString))
		strcpy(szConnectionString,pszConnectionString);
	else
		sprintf(szConnectionString,"Provider=Microsoft.ACE.OLEDB.12.0;Data Source=%s;Persist Security Info=False;",pszFileMdb);

	//
	// Provo a connettermi
	//
	_ConnectionPtr pConnection = NULL;
//	_RecordsetPtr pRst = NULL;
	IADORecordBinding *picRs = NULL;   // Interface Pointer declared.
//	CEmployeeRs emprs;   // C++ Class object
//	DBDATE varDate;

	try {
		
		TESTHR(pConnection.CreateInstance(__uuidof(Connection)));
		TESTHR(pConnection->Open(szConnectionString, "", "", adConnectUnspecified));
//		if (FAILED(hr)) ehError(); // Non riesco a connettermi

		psSection->pConn=pConnection;
		psSection->pvConn=_variant_t( (IDispatch *) psSection->pConn,true);

		// Test
// 
//		_test(psSection);


	}
	catch(_com_error &e) {

      // Display errors, if any. Pass a connection pointer accessed from the Connection.
       _PrintProviderError(pConnection);
       _PrintComError(e);
   }

	/*
	ADODB::_ConnectionPtr pConn("ADODB.Connection");
	psSection->oDatabase=(void *) pConn;
	if (!pConn->Open(strEver(pszUser),strEver(pszPassword),szConnectionString, ADODB::adConnectUnspecified))
	{
		//strAssign(&psSection->pszLastError,oDb->GetErrorErrStr());
		psSection->bError=true;
	}
	*/
	return psSection;
}



//
//	mdbQuery
//
int mdbQuery(EH_MDB_SECTION * psSection,CHAR * pszQuery) {

	BOOL bNoErrorStop=false;

	if (!psSection) ehExit("mdbQuery: section null");
	if (strEmpty(pszQuery)) ehError();
	if (*pszQuery=='?') {bNoErrorStop=true; pszQuery++;}
//	_ConnectionPtr pConn = psSection->pConn;

	psSection->dwQueryCounter++;
#ifdef EH_CONSOLE
	if (psSection->fDebug) {printf("%s" CRLF,pszQuery);}
#else
	if (psSection->fDebug) {fprintf(stderr,"%s" CRLF,pszQuery);}
#endif
	strAssign(&psSection->pszLastQuery,pszQuery);

//	_test(psSection);

// = psSection->pRst;
	TESTHR(_s.pRst.CreateInstance(__uuidof(Recordset)));
	psSection->pRst=_s.pRst;
   try  {

		 _s.pRst->Open(pszQuery, 
					_variant_t( (IDispatch *) psSection->pConn,true), 
					adOpenStatic, 
					adLockReadOnly,		// <-- da rendere parametrico
					adCmdText);
//         pRst->MoveFirst();

   }  
   catch(_com_error &e) {
      // Notify the user of errors if any.
      // Pass a connection pointer accessed from the Recordset.
      _variant_t vtConnect = _s.pRst->GetActiveConnection();
	  // _variant_t vtConnect = _variant_t( (IDispatch *) psSection->pConn,true);
      // GetActiveConnection returns connect string if connection
      // is not open, else returns Connection object.
      switch(vtConnect.vt) {
         case VT_BSTR:
            _PrintComError(e);
            break;
         case VT_DISPATCH:
            _PrintProviderError(vtConnect);
            break;
         default:
            printf("Errors occured.");
            break;
      }
	  if (!bNoErrorStop) ehError()
	}
//    psSection->pRst=pRst;
	return 1;
}

//
// mdbStore()
//
EH_MDB_RS	mdbStore(EH_MDB_SECTION * psSection) {

	
	EH_MDB_RS pRes=NULL;

	// Trovo dizionario e creo buffer
	if (!psSection->pRst) {return NULL;} // Precedente errore in query

	pRes=(EH_MDB_RS) ehAllocZero(sizeof(EH_MDB_RESULTSET));// sizeof(EH_odbcQueryResultCreate(psOdbcSection,iLimit); 
	if (pRes) pRes->pszQuery=strDup(psSection->pszLastQuery);
	pRes->iOffset=0;
	pRes->iCurrentRow=-1;
	pRes->pRst=psSection->pRst;

	return pRes;

}

//
// mdbFetch()
//
BOOL		mdbFetch(EH_MDB_RS psRs) {

//	SQLRETURN sqlReturn;
	if (!psRs) return false;
	_RecordsetPtr pRst=(_RecordsetPtr&) psRs->pRst;
	
	if (!psRs->arCells) // Prima volta
	{
		psRs->iOffset=0;
		psRs->iCurrentRow=0;
	}
	else
	{
		INT i;
		psRs->iCurrentRow++; if ((psRs->iCurrentRow-psRs->iOffset)<psRs->iRowsReady) return true;
		psRs->iOffset+=psRs->iRowsReady;
		INT iCells=psRs->iRowsReady*psRs->iFieldNum;
		for (i=0;i<iCells;i++) {ehFreeNN(psRs->arCells[i]);}
		ehFreePtr(&psRs->arCells);

	}

	// Carico il pezzo successivo
	psRs->iRowsReady=0;
	try  {

//		while (true) { 

			 // If GetRowsOK is successful, print the results,
			 // noting if the end of the file was reached.
			long lUbound;
			
			// Memorizza il resuilt in un array variant
			_variant_t avarData = pRst->GetRows(10);
//			_variant_t avarData=(_variant_t&) psRs->avarData;
			
 			HRESULT hr = SafeArrayGetUBound(avarData.parray, 2, &lUbound);   
			if (hr==S_OK) {
				psRs->iRowsReady=lUbound+1;
				psRs->iOffset=0;
				psRs->iCurrentRow=-1;
				// pRst->MoveFirst();
			}

			//
			// Creo array C (non ho trovato altre soluzioni per ora)
			//
			if (psRs->iRowsReady) {
				
				long arlIndeces[2];
				int iRow,iFld,iFields=pRst->GetFields()->Count;
				CHAR * pszDato;
				psRs->iFieldNum=iFields;
				INT iCells=psRs->iRowsReady*psRs->iFieldNum;
				psRs->arCells=(EH_AR) ehAllocZero(sizeof(CHAR *)*iCells);

				for (iRow= 0 ; iRow < lUbound+1 ; iRow++) {
					for (iFld=0;iFld<iFields;iFld++) {

						_variant_t result;
						result.vt = VT_BSTR;
						arlIndeces[0] = iFld; 
						arlIndeces[1] = iRow;
						hr = SafeArrayGetElement(avarData.parray, arlIndeces, &result);
						
						if (hr==S_OK) {
							if (result.vt!=VT_NULL) {
								_bstr_t bStr=(_bstr_t) result;
								pszDato=(CHAR * ) ehAlloc(bStr.length()+1);
								sprintf(pszDato, "%s", (LPCTSTR)(_bstr_t) result);
								psRs->arCells[iRow*iFields+iFld]=pszDato;//strDup((CHAR *) pszDato);
							}
						}
					}
				}
			}
			psRs->iCurrentRow=0;
//			TESTHR(SafeArrayDestroy(avarData));

   }  
   catch(_com_error &e) {
      // Notify the user of errors if any.
      // Pass a connection pointer accessed from the Recordset.
      _variant_t vtConnect = pRst->GetActiveConnection();
	  // _variant_t vtConnect = _variant_t( (IDispatch *) psSection->pConn,true);
      // GetActiveConnection returns connect string if connection
      // is not open, else returns Connection object.
      switch(vtConnect.vt) {
         case VT_BSTR:
            _PrintComError(e);
            break;
         case VT_DISPATCH:
            _PrintProviderError(vtConnect);
            break;
         default:
            printf("Errors occured.");
            break;
      }
   }
	if (!psRs->iRowsReady) return false;
	return true;

/*
	sqlReturn=SQLFetch(pRes->hStmt);
	if (sqlReturn==SQL_NO_DATA_FOUND) return FALSE;
	if (sqlReturn==SQL_SUCCESS||sqlReturn==SQL_SUCCESS_WITH_INFO) return TRUE;
	return TRUE;
*/
}

// odbc_f MySqlField
// Operazione su i campi di una query
// odbc_ffind() ricerca

INT		mdbFldFind(EH_MDB_RS psRs,CHAR * pszName,BOOL bNoError) 
{
	_RecordsetPtr pRst=(_RecordsetPtr&) psRs->pRst;

	long x,iFields= pRst->GetFields()->Count;
	for (x = 0 ; x < iFields; x++ ) {
		if (!strcmp((char*) pRst->GetFields()->Item[x]->Name,pszName)) 
		{
			return x;
		}
	}
	x=atoi(pszName); if (x<iFields) return x; // Numero di colonna
	if (!bNoError) ehExit("campo inesistente [%s]",pszName); 
	return -1;
}


//
// mdbFldPtr()
//
CHAR *		mdbFldPtr(EH_MDB_RS psRs,CHAR * pszName) {

	CHAR * pszRet;
	_RecordsetPtr pRst=(_RecordsetPtr&) psRs->pRst;

	if (!psRs) ehError();
	INT iFld=mdbFldFind(psRs,pszName,FALSE);

	pszRet=psRs->arCells[psRs->iCurrentRow*psRs->iFieldNum+iFld];
	return pszRet;

}

//
// mdbFldInt()
//
INT			mdbFldInt(EH_MDB_RS psRs,CHAR * pszName) {

	return 0;
}

//
// mdbFldNum()
//
double		mdbFldNum(EH_MDB_RS psRs,CHAR * pszName) {

	return 0;
}

//
// mdbFree()
//
BOOL		mdbFree(EH_MDB_RS psRs) {

	return 0;
}



static void _PrintProviderError(_ConnectionPtr pConnection) {
   // Print Provider Errors from Connection object.
   // pErr is a record object in the Connection's Error collection.
   ErrorPtr pErr = NULL;

   if ( (pConnection->Errors->Count) > 0) {
      long nCount = pConnection->Errors->Count;
      // Collection ranges from 0 to nCount -1.
      for ( long i = 0 ; i < nCount ; i++ ) {
         pErr = pConnection->Errors->GetItem(i);
         printf("\t Error number: %x\t%s", pErr->Number, pErr->Description);
      }
   }
}

static void _PrintComError(_com_error &e) {
   _bstr_t bstrSource(e.Source());
   _bstr_t bstrDescription(e.Description());

   // Print COM errors. 
   printf("Error\n");
   printf("\tCode = %08lx\n", e.Error());
   printf("\tCode meaning = %s\n", e.ErrorMessage());
   printf("\tSource = %s\n", (LPCSTR) bstrSource);
   printf("\tDescription = %s\n", (LPCSTR) bstrDescription);
}

static bool _GetRowsOK(_RecordsetPtr pRstTemp,int intNumber, _variant_t& avarData) {

	// Store results of GetRows method in array.
   avarData = pRstTemp->GetRows(intNumber);

   // Return False only if fewer than the desired number of rows were returned, 
   // but not because the end of the Recordset was reached.
   long lUbound;
   HRESULT hr = SafeArrayGetUBound(avarData.parray, 2, &lUbound);   
   if (hr == 0) {
      if ((intNumber > lUbound + 1) && (!(pRstTemp->EndOfFile)))
         return false;
      else
         return true;   
   }
   else  {
      printf ("\nUnable to Get the Array's Upper Bound\n");
      return false;
   }
}

static void _test( EH_MDB_SECTION * psSection) {

	_RecordsetPtr pRst;// = psSection->pRst;
	TESTHR(pRst.CreateInstance(__uuidof(Recordset)));

   try  {

		pRst->Open("SELECT * FROM LavoroProctor", _variant_t( (IDispatch *) psSection->pConn,true), adOpenStatic, adLockReadOnly, adCmdText);
		while (true) { 

         int intLines = 0;

         int intRows=10;

         // If GetRowsOK is successful, print the results,
         // noting if the end of the file was reached.
         _variant_t avarRecords;

         if (_GetRowsOK(pRst, intRows, avarRecords)) {
            long lUbound;
            HRESULT hr = SafeArrayGetUBound(avarRecords.parray, 2, &lUbound);

            if (hr == 0) {
               if (intRows > lUbound + 1) {
                  printf("\n(Not enough records in Recordset to retrieve %d rows)\n", intRows);
               }
            }
            printf("%d record(s) found.\n", lUbound + 1);

            // Print the retrieved data.
            for (int intRecords = 0 ; intRecords < lUbound+1 ; intRecords++) {
               long rgIndices[2];
               rgIndices[0] = 0; 
               rgIndices[1] = intRecords;
               _variant_t result;
               result.vt = VT_BSTR;

               hr = SafeArrayGetElement(avarRecords.parray, rgIndices, &result);

               if (hr == 0)
                  printf("%s ",(LPCSTR)(_bstr_t)result);
               
               rgIndices[0] = 1;

               hr = SafeArrayGetElement(avarRecords.parray, rgIndices, &result);

               if (hr == 0)
                  printf("%s, ",(LPCSTR)(_bstr_t)result);

               rgIndices[0] = 2;

               hr = SafeArrayGetElement(avarRecords.parray, rgIndices, &result);

               if (hr == 0)
                  printf("%s\n", (LPCSTR)(_bstr_t)result);

               intLines ++;

               if (intLines % 10 == 0) {
                  printf("\nPress any key to continue...");
                  _getch();
                  intLines = 0;
                  system("cls");
               }
            }
            printf("\n");
         }
         else {
            // Assume GetRows error caused by another user's data changes, 
            // use Requery to refresh the Recordset and start over.
            printf("GetRows failed--retry?\n");
            char chKey;
            do {
               chKey = _getch();
            } while (toupper(chKey) != 'Y'  && toupper(chKey) != 'N');

            if (toupper(chKey) == 'Y')
               pRst->Requery(adOptionUnspecified);
            else {
               printf("GetRows failed!\n");
               break;
            }
         }

         // Because using GetRows leaves the current record pointer at the last record 
         // accessed, move the pointer back to the beginning of the Recordset before 
         // looping back for another search.
         pRst->MoveFirst();
      }
   }  
   catch(_com_error &e) {
      // Notify the user of errors if any.
      // Pass a connection pointer accessed from the Recordset.
      _variant_t vtConnect = pRst->GetActiveConnection();
	  // _variant_t vtConnect = _variant_t( (IDispatch *) psSection->pConn,true);
      // GetActiveConnection returns connect string if connection
      // is not open, else returns Connection object.
      switch(vtConnect.vt) {
         case VT_BSTR:
            _PrintComError(e);
            break;
         case VT_DISPATCH:
            _PrintProviderError(vtConnect);
            break;
         default:
            printf("Errors occured.");
            break;
      }
   }
}
