//   ---------------------------------------------
//	 | cryptAccess
//   |
//	 |							by Ferrà srl 2014
//   ---------------------------------------------
#include "/easyhand/inc/easyhand.h"
#include "/easyhand/inc/cryptAccess.h"
#pragma comment(lib,"mpr.lib")

typedef struct {

	CHAR 	szRemote[500];
	BOOL	bClose;

} S_CONN;

static struct {
	
	EH_LST lstConn;

} _s={NULL};
 

//
// caAdd() - > Aggiunge una connessione al file
// Ritorna FALSE se tutto ok
//
BOOL caAddFile(	CHAR * pszNameUrl,
				CHAR * pszUser,
				CHAR * pszPassword,
				CHAR * pszBlowFish) 

{
 
	CHAR * psz;
	CHAR szServ[1024];
	CHAR szCode[2048];
	CHAR szFileConnection[512];
	EH_AR ar;

	if (!strEmpty(pszNameUrl)&&
		!strEmpty(pszUser)&&
		!strEmpty(pszPassword)&&
		!strEmpty(pszBlowFish)) {

			sprintf(szServ,"%s|%s|%s",pszNameUrl,pszUser,pszPassword);
			psz=bfHexEncrypt(szServ,pszBlowFish,szCode,sizeof(szCode));
			
			ehAppPath("connections.txt",szFileConnection,sizeof(szFileConnection));
			ar=ARFromTxt(szFileConnection,false); if (!ar) ar=ARNew();
			if (!ARIsIn(ar,psz,false)) ARAdd(&ar,psz);
		
			psz=ARToString(ar,"\n","","");
			fileStrWrite(szFileConnection,psz);
			ehFree(psz);
			ARDestroy(ar);
			return false;
	}

	return true;
}

//
// caRemoveFile() - > Aggiunge una connessione al file
// Ritorna FALSE se tutto ok
//
BOOL caRemoveFile(	CHAR * pszNameUrl,
					CHAR * pszBlowFish) 

{

	CHAR szFileConnection[512];
	EH_AR ar;
	INT a;
	BOOL bRet=true;
	CHAR * psz;

	ehAppPath("connections.txt",szFileConnection,sizeof(szFileConnection));

	ar=ARFromTxt(szFileConnection,false); 
	if (ar) { 
		for (a=0;ar[a];a++) {
		
			CHAR szCode[2048];
			EH_AR ars;
			psz=bfHexDecrypt(ar[a],pszBlowFish,szCode,sizeof(szCode));
			ars=strSplit(psz,"|");
			if (!strCmp(ars[0],pszNameUrl)) 
			{
				ARDel(&ar,a); ehFree(ars); 
				bRet=false;
				break;
			}
			ehFree(ars); 
		}

		// Update connections
		psz=ARToString(ar,"\n","","");
		fileStrWrite(szFileConnection,psz);
		ehFree(psz);
		ARDestroy(ar);
		
	}

	return bRet;
}




//
// caOpenAll()
// Apre connessione e ritorna lista 
//
static void _caExit(BOOL x);

EH_LST caOpenAll(CHAR * pszBlowFish,INT * piError) {

//	NETRESOURCE	nr;
//	DWORD		dwRes;
	CHAR		szCode[2048];
	CHAR		szFileConnection[512];
	EH_AR		ar;
	INT			a;
	CHAR *		psz;
	INT			iError=0;
	S_CONN		sConn;

	if (_s.lstConn) ehError();
	_s.lstConn=lstCreate(sizeof(S_CONN));
	ehAppPath("connections.txt",szFileConnection,sizeof(szFileConnection));
	ar=ARFromTxt(szFileConnection,true); 
	if (ar) { 
		
		for (a=0;ar[a];a++) {

			EH_AR ars;
			psz=bfHexDecrypt(ar[a],pszBlowFish,szCode,sizeof(szCode));
			if (!psz) ehError();
			ars=strSplit(psz,"|"); if (ARLen(ars)<3) ehExit("[%s]",psz);
			
			_(sConn);
			strcpy(sConn.szRemote,ars[0]);
			sConn.bClose=false;
			if (!caCreate(NULL,ars[0],ars[1],ars[2])) {

				lstPush(_s.lstConn,&sConn);
			} else iError++;
			ehFree(ars);
		}
		ARDestroy(ar);
	}
	if (!_s.lstConn->iLength) _s.lstConn=lstDestroy(_s.lstConn);
	ehAddExit(_caExit);
	if (piError) *piError=iError;
	return _s.lstConn;
}

//
// _caExit()
//
static void _caExit(BOOL x) {
	if (_s.lstConn) {
		caCloseAll(_s.lstConn);
		_s.lstConn=NULL;
	}
}

//
// caCloseAll()
//
BOOL caCloseAll(EH_LST lst) {
	
	S_CONN * psCon;
	if (!lst) return true;

	for (lstLoop(lst,psCon)) {
		if (psCon->bClose) 	caDestroy(psCon->szRemote);
	}

	lstDestroy(lst);
	return false;
}

//
// caEnum() - Provvisioria da finire, in pratica non fa nulla
//
BOOL caEnum(void) {

	DWORD dwResult, dwResultEnum;
	HANDLE hEnum;
	DWORD cbBuffer = 16384;     // 16K is a good size
	DWORD cEntries = -1;        // enumerate all possible entries
	LPNETRESOURCE lpnrLocal;    // pointer to enumerated structures
	DWORD i;
	LPNETRESOURCE lpnr = NULL;

    //
    // Call the WNetOpenEnum function to begin the enumeration.
    //
    dwResult = WNetOpenEnum(RESOURCE_GLOBALNET, // all network resources
                            RESOURCETYPE_ANY,   // all resources
                            0,  // enumerate all resources
                            lpnr,       // NULL first time the function is called
                            &hEnum);    // handle to the resource

    if (dwResult != NO_ERROR) {
        printf("WnetOpenEnum failed with error %d\n", dwResult);
        return FALSE;
    }
    //
    // Call the GlobalAlloc function to allocate resources.
    //
    lpnrLocal = (LPNETRESOURCE) GlobalAlloc(GPTR, cbBuffer);
    if (lpnrLocal == NULL) {
        printf("WnetOpenEnum failed with error %d\n", dwResult);
//      NetErrorHandler(hwnd, dwResult, (LPSTR)"WNetOpenEnum");
        return FALSE;
    }

    do {
        //
        // Initialize the buffer.
        //
        ZeroMemory(lpnrLocal, cbBuffer);
        //
        // Call the WNetEnumResource function to continue
        //  the enumeration.
        //
        dwResultEnum = WNetEnumResource(hEnum,  // resource handle
                                        &cEntries,      // defined locally as -1
                                        lpnrLocal,      // LPNETRESOURCE
                                        &cbBuffer);     // buffer size
        //
        // If the call succeeds, loop through the structures.
        //
        if (dwResultEnum == NO_ERROR) {

            for (i = 0; i < cEntries; i++) {

                // Call an application-defined function to
                //  display the contents of the NETRESOURCE structures.
                //
                //DisplayStruct(i, &lpnrLocal[i]);
				printf("%d:%s" CRLF,i,lpnrLocal[i].lpRemoteName);

                // If the NETRESOURCE structure represents a container resource, 
                //  call the EnumerateFunc function recursively.
/*
                if (RESOURCEUSAGE_CONTAINER == (lpnrLocal[i].dwUsage
                                                & RESOURCEUSAGE_CONTAINER))
//          if(!EnumerateFunc(hwnd, hdc, &lpnrLocal[i]))
                    if (!EnumerateFunc(&lpnrLocal[i]))
                        printf("EnumerateFunc returned FALSE\n");
//            TextOut(hdc, 10, 10, "EnumerateFunc returned FALSE.", 29);
*/
            }
        }
        // Process errors.
        //
        else if (dwResultEnum != ERROR_NO_MORE_ITEMS) {
            printf("WNetEnumResource failed with error %d\n", dwResultEnum);

//      NetErrorHandler(hwnd, dwResultEnum, (LPSTR)"WNetEnumResource");
            break;
        }
    }
    //
    // End do.
    //
    while (dwResultEnum != ERROR_NO_MORE_ITEMS);
    //
    // Call the GlobalFree function to free the memory.
    //
    GlobalFree((HGLOBAL) lpnrLocal);
    //
    // Call WNetCloseEnum to end the enumeration.
    //
    dwResult = WNetCloseEnum(hEnum);

    if (dwResult != NO_ERROR) {
        //
        // Process errors.
        //
//        printf("WNetCloseEnum failed with error %d\n", dwResult);
//    NetErrorHandler(hwnd, dwResult, (LPSTR)"WNetCloseEnum");
        return false;
    }

    return true;
}

//
// caCheck() > Beta non finito di testare
//
BOOL caCheck( CHAR * pszServer )
{  
    DWORD dwBufferSize = sizeof(NETRESOURCE);
    LPBYTE lpBuffer;                  // buffer
    NETRESOURCE nr;
    LPTSTR pszSystem = NULL;          // variable-length strings
	NETRESOURCE * psNr;

    //
    // Set the block of memory to zero; then initialize
    // the NETRESOURCE structure. 
    //
    _(nr);

    nr.dwScope       = RESOURCE_GLOBALNET;
    nr.dwType        = RESOURCETYPE_ANY;
    nr.lpRemoteName  = pszServer;

    //
    // First call the WNetGetResourceInformation function with 
    // memory allocated to hold only a NETRESOURCE structure. This 
    // method can succeed if all the NETRESOURCE pointers are NULL.
    // If the call fails because the buffer is too small, allocate
    // a larger buffer.
    //
    lpBuffer = malloc( dwBufferSize );
    if (lpBuffer == NULL) 
        return FALSE;

    while ( WNetGetResourceInformation(&nr, lpBuffer, &dwBufferSize, 
                &pszSystem) == ERROR_MORE_DATA)
    {
        lpBuffer = (LPBYTE) realloc(lpBuffer, dwBufferSize);
    }

    // Process the contents of the NETRESOURCE structure and the
    // variable-length strings in lpBuffer and set dwValue. When
    // finished, free the memory.
	psNr=(NETRESOURCE * )lpBuffer;
    free(lpBuffer);

    return true;
}

//
// caCreate() 
// Ritorna FALSE se tutto ok
// 
BOOL caCreate(CHAR *	pszLocalName,
			  CHAR *	pszRemoteName,
			  CHAR *	pszUserName,
			  CHAR *	pszPassword) 
{
	NETRESOURCE	nr;
	DWORD		dwRes;
	BOOL		bRet=true;

//	caEnum();
//	caCheck(pszRemoteName);

	_(nr);
	nr.dwType           =   RESOURCETYPE_DISK;
	nr.lpLocalName      =   pszLocalName;
	nr.lpRemoteName     =   pszRemoteName;

	dwRes=WNetAddConnection2  (		&nr,
									pszPassword,
									pszUserName,
									0
								);
	
	if (dwRes==NO_ERROR) {

		bRet=false;

	} else {

		CHAR * psz=osErrorStrAlloc(dwRes);
		// ERROR_NO_SUCH_LOGON_SESSION
		if (psz) {
			CHAR * pszMess=strOmit(psz,"\r\n");
			ehLogWrite("caCreate(): %d:%s - %s",dwRes,pszRemoteName,pszMess);
			ehFreeNN(psz);
		}

	}
	return bRet;

}

//
// caDestroy()
//
BOOL caDestroy(CHAR * pszRemoteName) {

	WNetCancelConnection2(pszRemoteName, 0, FALSE);
	return false;
}



/*
static void _connectionTest(void) {

    dwRes   =   WNetAddConnection2  (   &nr,
                                        "veronika98",
                                        "marco",
                                        0
                                    );

//	You'll need to link with 'mpr.lib'

	
	// system("net use \\192.168.99.80 /user:marco veronika98");
	{
		CHAR * pszFile="\\\\192.168.99.80\\wwwroot\\yacht4web\\status.php";
		CHAR * psz=fileStrRead(pszFile);
		printf("%s=[%s]",pszFile,psz);
		ehFreeNN(psz);
	}

	WNetCancelConnection2("\\\\192.168.99.80\\wwwroot", 0, FALSE);
//	if (osUserLogon("YWMASTER\\marco","veronika98")) ehExit("Logon error");

}
*/