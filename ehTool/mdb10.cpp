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
/*
#include <stdio.h>
#include <comdef.h>
#include <conio.h>
*/
#ifndef EH_MDB
#include "/easyhand/inc/eh_mdb10.h"
#endif

static struct {
	BOOL bReady;
} _s={0};


ADODB::_RecordsetPtr rec1=NULL;

_variant_t  vtMissing1(DISP_E_PARAMNOTFOUND, VT_ERROR); 

void ErrorHandler(_com_error &e,EH_LST lst)
{
	lstPushf(lst,"Error:");
	lstPushf(lst,"Code = %08lx",e.Error());
	lstPushf(lst,"Code meaning = %s", (CHAR * ) e.ErrorMessage());
	lstPushf(lst,"Source = %s", (CHAR * ) e.Source());
	lstPushf(lst,"Description = %s",(CHAR * ) e.Description());
	
}

Database::Database() // Costruttore
{
	m_Cnn=NULL;
	lstErr=lstNew();
}
Database::~Database() // Distruttore
{
	lstDestroy(lstErr);
}
CHAR * Database::GetErrorErrStr(void)
{
	return lstToString(lstErr,"\n","","");
}

void Table::GetErrorErrStr(CHAR * pszErrStr)
{
	CHAR * psz=lstToString(lstErr,"\n","","");
	strcpy(pszErrStr,psz);
	ehFree(psz);
}

bool Database::Open(CHAR * UserName, CHAR * Pwd,CHAR * CnnStr)
{
	//cnn->Open(strCnn,"sa","sa",NULL);
	try
	{
		HRESULT hr;
		hr    = m_Cnn.CreateInstance( __uuidof( ADODB::Connection ) );
		m_Cnn->Open(CnnStr, UserName, Pwd, NULL);
	}
	
	CATCHERROR(m_Cnn,0)

	lstPushf(lstErr,"Success");
	return 1;
}

bool Database::OpenTbl(int Mode, CHAR * CmdStr, Table &Tbl)
{
	if(m_Cnn==NULL)
	{
		Tbl.m_Rec=NULL;
		lstPushf(lstErr,"Invalid Connection");
		return 0;
	}
	RecPtr t_Rec=NULL;
	try
	{
		//t_Rec->putref_ActiveConnection(m_Cnn);
		//vtMissing<<-->>_variant_t((IDispatch *) m_Cnn, true)
		t_Rec.CreateInstance( __uuidof( ADODB::Recordset ) );
		t_Rec->Open(CmdStr,_variant_t((IDispatch *) m_Cnn, true),ADODB::adOpenStatic,ADODB::adLockOptimistic,Mode);
	}
	
	CATCHERROR(Tbl.m_Rec,0)

	Tbl.m_Rec=t_Rec;
	lstPushf(lstErr,"Success");
	return 1;
}

bool Database::Execute(CHAR * CmdStr)
{
	try
	{
		m_Cnn->Execute(CmdStr,NULL,1);
	}
	catch(_com_error &e)
	{
		ErrorHandler(e,lstErr);
		return 0;
	}
	lstPushf(lstErr,"Success");
	return 1;
}

bool Database::Execute(CHAR * CmdStr, Table * Tbl)
{
	RecPtr t_Rec=NULL;
	try
	{
		t_Rec=m_Cnn->Execute(CmdStr,NULL,1);
	}

	CATCHERROR(Tbl->m_Rec,0)

	lstPushf(lstErr,"Success");
	Tbl->m_Rec=t_Rec;
	return 1;
}

Table::Table()
{
	m_Rec=NULL;
	lstErr=lstNew();
}

int Table::ISEOF()
{
	int rs;
	if(m_Rec==NULL)
	{
		lstPushf(lstErr,"Invalid Record");
		return -1;
	}
	try{
		rs=m_Rec->EndOfFile;
	}
	
	CATCHERROR(m_Rec,-2)

	lstPushf(lstErr,"Success");
	return rs;
}

bool Table::Get(CHAR * FieldName, CHAR * FieldValue)
{
	try
	{
		_variant_t  vtValue;
		vtValue = m_Rec->Fields->GetItem(FieldName)->GetValue();
		sprintf(FieldValue,"%s",(LPCSTR)((_bstr_t)vtValue.bstrVal));
	}

	CATCHERRGET

	lstPushf(lstErr,"Success");
	return 1;
}

bool Table::Get(CHAR * FieldName,int& FieldValue)
{
	try
	{
		_variant_t  vtValue;
		vtValue = m_Rec->Fields->GetItem(FieldName)->GetValue();
		FieldValue=vtValue.intVal;
	}

	CATCHERRGET

	lstPushf(lstErr,"Success");
	return 1;
}

bool Table::Get(CHAR * FieldName,float& FieldValue)
{
	try
	{
		_variant_t  vtValue;
		vtValue = m_Rec->Fields->GetItem(FieldName)->GetValue();
		FieldValue=vtValue.fltVal;
	}

	CATCHERRGET

	lstPushf(lstErr,"Success");
	return 1;
}

bool Table::Get(CHAR * FieldName,double& FieldValue)
{
	try
	{
		_variant_t  vtValue;
		vtValue = m_Rec->Fields->GetItem(FieldName)->GetValue();
		FieldValue=vtValue.dblVal;
		//GetDec(vtValue,FieldValue,3);
	}

	CATCHERRGET

	lstPushf(lstErr,"Success");
	return 1;
}

HRESULT Table::MoveNext()
{
	HRESULT hr;
	try
	{
		hr=m_Rec->MoveNext();
	}
	catch(_com_error &e)
	{
		ErrorHandler(e,lstErr);
		//m_Rec=NULL;
		return -2;
	}
	lstPushf(lstErr,"Success");
	return hr;
}

HRESULT Table::MovePrevious()
{
	HRESULT hr;
	try
	{
		hr=m_Rec->MovePrevious();
	}
	catch(_com_error &e)
	{
		ErrorHandler(e,lstErr);
		//m_Rec=NULL;
		return -2;
	}
	lstPushf(lstErr,"Success");
	return hr;
}

HRESULT Table::MoveFirst()
{
	HRESULT hr;
	try
	{
		hr=m_Rec->MoveFirst();
	}
	catch(_com_error &e)
	{
		ErrorHandler(e,lstErr);
		//m_Rec=NULL;
		return -2;
	}
	lstPushf(lstErr,"Success");
	return hr;
}

HRESULT Table::MoveLast()
{
	HRESULT hr;
	try
	{
		hr=m_Rec->MoveLast();
	}
	catch(_com_error &e)
	{
		ErrorHandler(e,lstErr);
		//m_Rec=NULL;
		return -2;
	}
	lstPushf(lstErr,"Success");
	return hr;
}

/*
CHAR  szConnectionString[800];
"Provider=SQLOLEDB.1;Persist Security Info=False;"\
				 "User ID=lab;Initial Catalog=pubs;Data "\
				 "Source=PRASUN@CSE" ;
char ErrStr[200];
*/


//
// Connessione
//
EH_MDB_SECTION *	mdbConnect(	CHAR *	pszFileMdb,
								CHAR *  pszConnectionString,	
								CHAR *	pszUser,
								CHAR *	pszPassword) {

	EH_MDB_SECTION * psSection=(EH_MDB_SECTION *) ehAllocZero(sizeof(EH_MDB_SECTION));
	
	CHAR szConnectionString[800];

	if (!_s.bReady) {::CoInitialize(NULL); _s.bReady=true;}
	
	Database * oDb=new Database();

	if (!strEmpty(pszConnectionString))
		strcpy(szConnectionString,pszConnectionString);
	else
		sprintf(szConnectionString,"Provider=Microsoft.ACE.OLEDB.12.0;Data Source=%s;Persist Security Info=False;",pszFileMdb);
	psSection->oDatabase=(void *) oDb;

	//
	// Provo a connettermi
	//
	if(!oDb->Open(pszUser,pszPassword,szConnectionString))
	{
		strAssign(&psSection->pszLastError,oDb->GetErrorErrStr());
		psSection->bError=true;
	}

	return psSection;
}

//
//	mdbQuery
//
int mdbQuery(EH_MDB_SECTION * psSection,CHAR * pszQuery) {

	// Table * tbl=new Table();
	if (!psSection) ehExit("mdbQuery: section null");
	BOOL bNoErrorStop=false;
	Database * oDb=(Database *) psSection->oDatabase;

	psSection->dwQueryCounter++;
	
	if (strEmpty(pszQuery)) ehError();
	if (*pszQuery=='?') {bNoErrorStop=true; pszQuery++;}

#ifdef EH_CONSOLE
	if (psSection->fDebug) {printf("%s" CRLF,pszQuery);}
#else
	if (psSection->fDebug) {fprintf(stderr,"%s" CRLF,pszQuery);}
#endif

	strAssign(&psSection->pszLastQuery,pszQuery);

	RecPtr t_Rec=NULL;
	psSection->oRec=NULL;
	try
	{
		t_Rec=oDb->m_Cnn->Execute(pszQuery,NULL,1);
	}
	catch(_com_error &e)
	{
		ErrorHandler(e,oDb->lstErr);
		return 0;
	}
	
	psSection->oRec=t_Rec;
	return 1;

/*
	if (!oDb->Execute(pszQuery,tbl))
	{
		CHAR * psz=oDb->GetErrorErrStr();
		strAssign(&psSection->pszLastError,psz);
		ehFree(psz);
		delete tbl;
		return 1;
	}
	*/
	//
	
}

//
// mdbStore()
//
EH_MDB_RS	mdbStore(EH_MDB_SECTION * psSection) {

	
	EH_MDB_RS pRes;

	// Trovo dizionario e creo buffer
	if (!psSection->oRec) {return NULL;} // Precedente errore in query

	pRes=(EH_MDB_RS) ehAllocZero(sizeof(EH_MDB_RESULTSET));// sizeof(EH_odbcQueryResultCreate(psOdbcSection,iLimit); 
	pRes->oRec=psSection->oRec; 
	pRes->psSection=psSection;
	/*
#ifdef EH_MEMO_DEBUG 
		memDebugUpdate(pRes,pProg,iLine);
#endif
		*/
	if (pRes) pRes->pszQuery=strDup(psSection->pszLastQuery);
	return pRes;

}

//
// mdbFetch()
//
BOOL		mdbFetch(EH_MDB_RS psRs) {

	printf("qui");

	return 0;
}

//
// mdbFldPtr()
//
CHAR *		mdbFldPtr(EH_MDB_RS psRs,CHAR * pszName) {

	CHAR * pszRet=NULL;
	Database * oDb=(Database *) psRs->psSection->oDatabase;
	/*
	RecPtr oRec=NULL;
	oRec=(RecPtr)  psRs->psSection->oRec;

	try
	{
	//	_variant_t  vtValue;
	//	vtValue =	oRec->Fields->GetItem(pszName)->GetValue();
//		pszRet=		(CHAR * )((_bstr_t)vtValue.bstrVal);
		// sprintf(FieldValue,"%s",(LPCSTR)((_bstr_t)vtValue.bstrVal));

	}
	catch(_com_error &e)
	{
		ErrorHandler(e,oDb->lstErr);
		lstPushf(oDb->lstErr,"**For Field Name:%s",pszName);
		return 0;
	}
*/
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

//	mdbStore
//	mdbFetch
//	mdbFldPtr
//	mdbFree
//

int mdbCheck(CHAR * pszFileName)
{

	::CoInitialize(NULL);
	Database db;
	Table tbl;
	CHAR szConnectionString[800];
	sprintf(szConnectionString,"Provider=Microsoft.ACE.OLEDB.12.0;Data Source=%s;Persist Security Info=False;",pszFileName);

	//
	// Apro DB
	//
	if(!db.Open(NULL,NULL,szConnectionString))
	{
		CHAR * psz=db.GetErrorErrStr();
		printf("%s" CRLF,psz);
		ehFree(psz);
		return 1;
//		cout<<ErrStr<<"\n";
	}
/*
	//
	// Eseguo la Query
	//
	if(!db.Execute("select * from LavoroProctor",tbl))
	{
		CHAR * psz=db.GetErrorErrStr();
		printf("%s" CRLF,psz);
		ehFree(psz);
		return 1;
	}
*/
	printf("qui");
	/*if(!db.OpenTbl(ADODB::adCmdText,"select * from authors order by au_fname,au_id",tbl))
	{
		db.GetErrorErrStr(ErrStr);
		cout<<ErrStr<<"\n";
	}

	char id[100];
	if(!tbl.ISEOF())
		tbl.MoveFirst();
	
	while(!tbl.ISEOF())
	{
		if(tbl.Get("au_id",id))
			cout<<"\nid:"<<id;
		else
		{
			tbl.GetErrorErrStr(ErrStr);
			cout<<"\n"<<ErrStr<<"\n";
			break;
		}
		if(tbl.Get("au_fname",id))
			cout<<" fname:"<<id;
		else
		{
			tbl.GetErrorErrStr(ErrStr);
			cout<<"\n"<<ErrStr<<"\n";
			break;
		}

		tbl.MoveNext();
	}
	*/
 	::CoUninitialize();
	return 0;
}