//   ---------------------------------------------
//	 | NTSERVICE                                 |
//	 | Gestione Windows Service                  |
//	 |                             by Ferrà 2004 |
//   ---------------------------------------------

#include "/easyhand/inc/easyhand.h"
//#include <tchar.h>
#include <stdio.h>
#include "/easyhand/ehtool/NTService.h"
#pragma comment(lib, "Psapi.lib")

// This function attempts to stop a service. It allows the caller to // specify whether dependent services should also be stopped. It also 
// accepts a timeout value, to prevent a scenario in which a service 
// shutdown hangs, then the application stopping the service hangs.
// 
// Parameters:   
//   hSCM - Handle to the service control manager.
//   hService - Handle to the service to be stopped.
//   fStopDependencies - Indicates whether to stop dependent services.
//   dwTimeout - maximum time (in milliseconds) to wait
// 
// If the operation is successful, returns ERROR_SUCCESS. Otherwise, // returns a system error code.
static DWORD _serviceStopNow( S_NT_SERVICE * psNtService, BOOL fStopDependencies, DWORD dwTimeout );
static DWORD _serviceStart(S_NT_SERVICE *psNtService,BOOL fShow) ;

//
// FNTServiceGet() - Ritorna se esiste un servizio, e carica i dati del servizio in psNtService (se presente)
//
// dwMode= SERVICE_ALL_ACCESS (per esempio)

BOOL FNTServiceGet(CHAR * pszServiceName,DWORD dwMode,S_NT_SERVICE * psNtService) {

   SC_HANDLE hSCM;
   SC_HANDLE hService;
   DWORD     dwError;
   BYTE *	 lpError;
   BOOL		 bExist=FALSE;

   __try 
   {
      // Open the SCM database
      hSCM = OpenSCManager( NULL, NULL, SC_MANAGER_CONNECT );
      if ( !hSCM ) 
      {
			lpError=osErrorStrAlloc(osGetError());
			ehLogWrite(TEXT("OpenSCManager(%d:%s)"),osGetError(),lpError);
			dwError=1;
			ehFree(lpError);
			__leave;
      }

      // Open the specified service
      hService = OpenService( hSCM, pszServiceName, dwMode );
      if ( !hService ) 
      {
		DWORD dwErr=osGetError();
		lpError=osErrorStrAlloc(dwErr);
		ehLogWrite(TEXT("OpenService(%s > %d:%s)"),pszServiceName,dwErr,lpError);
		dwError=dwErr;
		ehFree(lpError);
		__leave;
      } 
	  else 
	  {
		bExist=TRUE;
	  }

   } 
   __finally 
   {
	   if (!psNtService) {
		
			if ( hService ) CloseServiceHandle( hService );
			if ( hSCM ) CloseServiceHandle( hSCM );
	   } 
	   else 
	   {
			memset(psNtService,0,sizeof(S_NT_SERVICE));
			psNtService->hSCM=hSCM;
			psNtService->hService=hService;
	   }
   }

   return bExist;

}

//
// FNTServiceFree()
//
void FNTServiceFree(S_NT_SERVICE * psNtService) {

	if ( psNtService->hService ) CloseServiceHandle( psNtService->hService );
	if ( psNtService->hSCM ) CloseServiceHandle( psNtService->hSCM );
}





//
// FNTServiceStop()
//
// Ritorna 
// 0 = Tutto ok (fermato)
// 1060 = Il servizio non esiste
// 1460 = Errore di TimeOut
// 1 = Il servizio non può essere "fermato"

DWORD FNTServiceStop(CHAR * pszServiceName, BOOL fStopDependencies, DWORD dwTimeout ,BOOL fShowWindow) 
{
//	SERVICE_STATUS	sServiceStatus;
	DWORD			dwError=0;
	CHAR			szServ[200];
	S_NT_SERVICE	sNtService;
	printf("stopping %s ...",pszServiceName);
	sprintf(szServ,"stopping service %s ...",pszServiceName);
	
	if (!FNTServiceGet(pszServiceName,SERVICE_STOP | SERVICE_QUERY_STATUS | SERVICE_ENUMERATE_DEPENDENTS, &sNtService)) ehExit(__FUNCTION__":[%s] non esiste",pszServiceName);

	__try 
	{

		QueryServiceStatus(sNtService.hService,&sNtService.sServiceStatus);
	  /* 
	  if (sServiceStatus.dwCurrentState==SERVICE_STOPPED||
		  sServiceStatus.dwCurrentState==SERVICE_STOP_PENDING)
		  */
		if (!(sNtService.sServiceStatus.dwControlsAccepted&SERVICE_ACCEPT_STOP))
			{
				dwError=1;
				__leave;
			}	

      // Try to stop the service, specifying a 30 second timeout
#ifdef _CONSOLE
      if (fShowWindow) printf("%s\n\r",szServ);
#else
      if (fShowWindow) win_time2(szServ,0);
#endif
	  
	  dwError = _serviceStopNow( &sNtService, fStopDependencies, dwTimeout ) ;
#ifndef _CONSOLE
	  if (fShowWindow) win_close();
#endif
	  /*
      if ( dwError == ERROR_SUCCESS )
         win_infoarg(TEXT("Service stopped.\n") );
      else
         win_infoarg(TEXT("StopService(%d)"), dwError );
		 */

   } 
   __finally 
   {
	   FNTServiceFree(&sNtService);
   }
   if (!dwError) printf("service stopped." CRLF); else printf("stop error: %d" CRLF,dwError);
   return dwError;
}

//
// _serviceStopNow()
//
static DWORD _serviceStopNow(S_NT_SERVICE *psNtService, BOOL fStopDependencies, DWORD dwTimeout ) 
{
//   SERVICE_STATUS ss;

	DWORD dwStartTime,dwSec;

	printf("---" CRLF);
	if (!psNtService->sServiceStatus.dwWaitHint) psNtService->sServiceStatus.dwWaitHint=200;
	printf("sServiceStatus.dwWaitHint: %d" CRLF,psNtService->sServiceStatus.dwWaitHint);
	printf("timeout: %d" CRLF,dwTimeout);
	

	// Make sure the service is not already stopped
	// if ( !QueryServiceStatus( psNtService->hService, &ss ) ) return osGetError();
	if ( psNtService->sServiceStatus.dwCurrentState == SERVICE_STOPPED )  return ERROR_SUCCESS;

	// If a stop is pending, just wait for it
	dwStartTime = GetTickCount();
	while ( psNtService->sServiceStatus.dwCurrentState == SERVICE_STOP_PENDING ) 
	{
		Sleep( psNtService->sServiceStatus.dwWaitHint );
		if ( !QueryServiceStatus( psNtService->hService, &psNtService->sServiceStatus ) ) return osGetError();
		if ( psNtService->sServiceStatus.dwCurrentState == SERVICE_STOPPED ) return ERROR_SUCCESS;
		dwSec=((GetTickCount() - dwStartTime)/1000);
		if (  dwSec> dwTimeout ) {
			  printf("Timeout! %d > %d",dwSec,dwTimeout);
			  //ERROR_TIMEOUT
			  break;
		  }
	}

   // If the service is running, dependencies must be stopped first
   if ( fStopDependencies ) 
   {
      DWORD i;
      DWORD dwBytesNeeded;
      DWORD dwCount;

      LPENUM_SERVICE_STATUS   lpDependencies = NULL;
      ENUM_SERVICE_STATUS     ess;
      SC_HANDLE               hDepService;

      // Pass a zero-length buffer to get the required buffer size
      if ( EnumDependentServices( psNtService->hService, SERVICE_ACTIVE,  lpDependencies, 0, &dwBytesNeeded, &dwCount ) ) 
      {
         // If the Enum call succeeds, then there are no dependent
         // services so do nothing
		  printf("no dipendencies." CRLF);
      } 
      else 
      {
         if ( osGetError() != ERROR_MORE_DATA ) return osGetError(); // Unexpected error

         // Allocate a buffer for the dependencies
         lpDependencies = (LPENUM_SERVICE_STATUS) HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwBytesNeeded );
         if ( !lpDependencies ) return osGetError();

         __try {
            // Enumerate the dependencies
            if ( !EnumDependentServices( psNtService->hService, SERVICE_ACTIVE, 
                  lpDependencies, dwBytesNeeded, &dwBytesNeeded,
                  &dwCount ) ) return osGetError();

            for ( i = 0; i < dwCount; i++ ) 
            {
               ess = *(lpDependencies + i);

               // Open the service
               hDepService = OpenService( psNtService->hSCM, ess.lpServiceName, SERVICE_STOP | SERVICE_QUERY_STATUS );
               if ( !hDepService ) return osGetError();

               __try {
                   // Send a stop code
                  if ( !ControlService( hDepService, SERVICE_CONTROL_STOP,&psNtService->sServiceStatus ) )return osGetError();

                  // Wait for the service to stop
                  while ( psNtService->sServiceStatus.dwCurrentState != SERVICE_STOPPED ) 
                  {
                     Sleep( psNtService->sServiceStatus.dwWaitHint );
                     if ( !QueryServiceStatus( hDepService, &psNtService->sServiceStatus ) ) return osGetError();
                     if ( psNtService->sServiceStatus.dwCurrentState == SERVICE_STOPPED ) break;
                     if ( ((GetTickCount() - dwStartTime)/1000) > dwTimeout ) return ERROR_TIMEOUT;
                  }

               } 
               __finally 
               {
                  // Always release the service handle
                  CloseServiceHandle( hDepService );
               }
            }
         } 
         __finally 
         {
            // Always free the enumeration buffer
            HeapFree( GetProcessHeap(), 0, lpDependencies );
         }
      } 
   }

   //
   // Send a stop code to the main service
   //
   printf("-> Send stop to service ..." CRLF);

   if (!ControlService( psNtService->hService, 
						SERVICE_CONTROL_STOP, 
						&psNtService->sServiceStatus ) ) 
   {
	   SINT iError=osGetError();
	   BYTE * psz=osErrorStrAlloc(iError);
	   printf("Stop error: %d - %s" CRLF,iError,psz);
	   ehFree(psz);
	   return iError;
   }

   dwStartTime = GetTickCount();

   printf("-> waiting service stop ..." CRLF);

   // Wait for the service to stop
   while ( psNtService->sServiceStatus.dwCurrentState != SERVICE_STOPPED ) 
   {
		Sleep( 300 ); //mouse_inp();
		if ( !QueryServiceStatus( psNtService->hService, &psNtService->sServiceStatus ) ) 
		{
		   SINT iError=osGetError();
		   BYTE * psz=osErrorStrAlloc(iError);
		   printf("Stop error: %d - %s" CRLF,iError,psz);
		   ehFree(psz);
		   return iError;
		}
		if ( psNtService->sServiceStatus.dwCurrentState == SERVICE_STOPPED )  break;

		dwSec=((GetTickCount() - dwStartTime)/1000);
		if ( dwSec> dwTimeout ) {
				printf("Timeout! %d > %d" CRLF,dwSec,dwTimeout);
				return ERROR_TIMEOUT;
		  }
   }

   // Return success
   return ERROR_SUCCESS;
}

//
// FNTServiceCheck()
//
BOOL FNTServiceCheck(CHAR * pszServiceName) {
   return FNTServiceGet(pszServiceName,SERVICE_ALL_ACCESS, NULL);
}

//
// FNTServiceRestart()
//
DWORD FNTServiceRestart(CHAR * pszServiceName, DWORD dwTimeout ,BOOL fShowWindow) 
{
	DWORD dwError;
	FNTServiceStop(pszServiceName,TRUE,dwTimeout,fShowWindow);
	dwError=FNTServiceStart(pszServiceName, dwTimeout ,fShowWindow);
	return dwError;
}


//
// FNTServiceStart()
//
DWORD FNTServiceStart(CHAR * pszServiceName, DWORD dwTimeout ,BOOL fShowWindow) 
{
	DWORD			dwError;
	CHAR			szServ[200];
//	BYTE *			lpError;
	S_NT_SERVICE	sNtService;
	sprintf(szServ,"starting %s ...",pszServiceName);

	if (!FNTServiceGet(pszServiceName,SERVICE_ALL_ACCESS, &sNtService)) ehExit("[%s] non esiste",pszServiceName);

   __try 
   {

//      // Open the specified service
  //    hService = OpenService( hSCM, lpService, SERVICE_ALL_ACCESS );
	   /*
      if ( !hService ) 
      {
		  DWORD dwErr=osGetError();
	      lpError=osErrorStrAlloc(osGetError());
		  ehLogWrite(TEXT("OpenService(%s > %d:%s)"),lpService,dwErr,lpError);
		  if (fShowWindow) 
		  {
		   switch (dwErr)
		   {
			case ERROR_ACCESS_DENIED: win_infoarg(TEXT("OpenService(ACCESS_DENIED)")); break;
			case ERROR_INVALID_HANDLE: win_infoarg(TEXT("OpenService(INVALID_HANDLE)")); break;
			case ERROR_INVALID_NAME: win_infoarg(TEXT("OpenService(INVALID_NAME)")); break;
			case ERROR_SERVICE_DOES_NOT_EXIST: win_infoarg(TEXT("OpenService(SERVICE_DOES_NOT_EXIST)")); break;
			default: if (fShowWindow) win_infoarg(TEXT("OpenService(%d)"), osGetError() ); 
					 break;
		   }
		  }
		  dwError=dwErr;
		  ehFree(lpError);
         __leave;
      }
*/
#ifdef EH_CONSOLE
      if (fShowWindow) printf("%s" ,szServ);
#else
      if (fShowWindow) win_time2(szServ,0);
#endif

	  dwError = _serviceStart( &sNtService ,fShowWindow) ;

#ifndef _CONSOLE
	  if (fShowWindow) win_close();
#endif
	  //if ( dwError == ERROR_SUCCESS ) win_infoarg("StartService SUCCESS.\n"); 
   } 
   __finally 
   {
	   FNTServiceFree(&sNtService);
   }
   printf("%d" CRLF,dwError);
   return dwError;
}

//
// _serviceStart()
//
static DWORD _serviceStart(S_NT_SERVICE *psNtService,BOOL fShow) 
{ 
    SERVICE_STATUS ssStatus; 
    DWORD dwOldCheckPoint; 
    DWORD dwStartTickCount;
    DWORD dwWaitTime;
    DWORD dwStatus;
	BYTE *lpError;
 
    if (!StartService(
			psNtService->hService,  // handle to service 
            0,           // number of arguments 
            NULL) )      // no arguments 
    {
		lpError=osErrorStrAlloc(osGetError());
		ehLogWrite("StartService(): %d:%s",osGetError(),lpError);
		ehFree(lpError);

        if (fShow) win_infoarg("ERROR:StartService"); 
		return -2;
    }
    else 
    {
        //win_infoarg("ERROR:Service start pending"); return -3;
    }
 
    // Check the status until the service is no longer start pending. 
 
    if (!QueryServiceStatus( 
			psNtService->hService,   // handle to service 
            &ssStatus) )  // address of status information structure
    {
		lpError=osErrorStrAlloc(osGetError());
		ehLogWrite("QueryServiceStatus(): %d:%s",osGetError(),lpError);
		ehFree(lpError);

        if (fShow) win_infoarg("ERROR:QueryServiceStatus"); 
		return -3;
    }
 
    // Save the tick count and initial checkpoint.

    dwStartTickCount = GetTickCount();
    dwOldCheckPoint = ssStatus.dwCheckPoint;

    while (ssStatus.dwCurrentState == SERVICE_START_PENDING) 
    { 
        // Do not wait longer than the wait hint. A good interval is 
        // one tenth the wait hint, but no less than 1 second and no 
        // more than 10 seconds. 
 
        dwWaitTime = ssStatus.dwWaitHint / 10;

        if( dwWaitTime < 1000 ) dwWaitTime = 1000;
			else if ( dwWaitTime > 10000 ) dwWaitTime = 10000;

        Sleep( dwWaitTime );

        // Check the status again. 
 
        if (!QueryServiceStatus( 
                psNtService->hService,   // handle to service 
                &ssStatus) )  // address of structure
            break; 
 
        if ( ssStatus.dwCheckPoint > dwOldCheckPoint )
        {
            // The service is making progress.

            dwStartTickCount = GetTickCount();
            dwOldCheckPoint = ssStatus.dwCheckPoint;
        }
        else
        {
            if(GetTickCount()-dwStartTickCount > ssStatus.dwWaitHint)
            {
                // No progress made within the wait hint
                break;
            }
        }
    } 

    if (ssStatus.dwCurrentState == SERVICE_RUNNING) 
    {
        dwStatus = NO_ERROR;
    }
    else 
    { 
		if (fShow) 
		{
         win_infoarg("\nService not started. \n");
         win_infoarg("  Current State: %d\n", ssStatus.dwCurrentState); 
         win_infoarg("  Exit Code: %d\n", ssStatus.dwWin32ExitCode); 
         win_infoarg("  Service Specific Exit Code: %d\n", ssStatus.dwServiceSpecificExitCode); 
         win_infoarg("  Check Point: %d\n", ssStatus.dwCheckPoint); 
         win_infoarg("  Wait Hint: %d\n", ssStatus.dwWaitHint); 
		}
        dwStatus = osGetError();
    } 
 
    return dwStatus;
}

static void getDebugPriv( void )
{
	HANDLE hToken;
	LUID sedebugnameValue;
	TOKEN_PRIVILEGES tkp;

	if ( ! OpenProcessToken( GetCurrentProcess(),
		TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken ) )
		return;

	if ( !LookupPrivilegeValue( NULL, SE_DEBUG_NAME, &sedebugnameValue ) )
	{
		CloseHandle( hToken );
		return;
	}

	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Luid = sedebugnameValue;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	AdjustTokenPrivileges( hToken, FALSE, &tkp, sizeof tkp, NULL, NULL );

	CloseHandle( hToken );
}

// Ritorna FALSE se tutto OK
//		   TRUE processo non trovato

BOOL FNTKillProcess(CHAR *lpExeName) 
{ 
    HANDLE         hProcessSnap = NULL; 
    BOOL           bRet      = FALSE; 
    PROCESSENTRY32 pe32      = {0}; 
 
	getDebugPriv(); // Setto i privilegi del process in modo da poter terminare qualunque processo

    //  Take a snapshot of all processes in the system. 

    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); if (hProcessSnap == (HANDLE)-1)  return (TRUE); 
 
    //  Fill in the size of the structure before using it. 

    pe32.dwSize = sizeof(PROCESSENTRY32); 
 
    //  Walk the snapshot of the processes, and for each process, 
    //  display information. 
    bRet = TRUE; 

    if (Process32First(hProcessSnap, &pe32)) 
    { 
        //DWORD         dwPriorityClass; 
        BOOL          bGotModule = FALSE; 
        MODULEENTRY32 me32       = {0}; 
 
        do 
        { 
			
			//win_infoarg("[%s]",pe32.szExeFile);
			if (!strcmp(pe32.szExeFile,lpExeName))
			{
				HANDLE hProcess;
				BOOL bResult;

				hProcess=OpenProcess(SYNCHRONIZE|PROCESS_TERMINATE, TRUE, pe32.th32ProcessID);
				if (hProcess)
				{
					/*
					win_infoarg("[%s] Thread=%d (Handle=%d)",
								pe32.szExeFile,
								pe32.th32ProcessID,
								hProcess);
								*/
					bResult = TerminateProcess(hProcess,0);
					CloseHandle(hProcess);
					bRet = FALSE;    // could not walk the list of processes 
				}
					/*
				{
				 SINT iErr=TerminateProcess(hProcess,0);
				 if(WaitForSingleObject(hProcess, 5000)!=WAIT_OBJECT_0) 
				 	bResult = TerminateProcess(hProcess,0);
					else
					bResult = TRUE; 
				 CloseHandle(hProcess);
				}
				*/
				break;
			}
        } 
        while (Process32Next(hProcessSnap, &pe32)); 
    } 
 
    // Do not forget to clean up the snapshot object. 

    CloseHandle (hProcessSnap); 
    return (bRet); 
}

EH_AR FNTGetProcessList(void) 
{ 
    HANDLE         hProcessSnap = NULL; 
    BOOL           bRet      = FALSE; 
    PROCESSENTRY32 pe32      = {0}; 
	EH_AR ar=NULL;
 
    //  Take a snapshot of all processes in the system. 

    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); 
    if (hProcessSnap == (HANDLE)-1)  return NULL; 
 
    //  Fill in the size of the structure before using it. 

    pe32.dwSize = sizeof(PROCESSENTRY32); 
 
    //  Walk the snapshot of the processes, and for each process, 
    //  display information. 

    if (Process32First(hProcessSnap, &pe32)) 
    { 
//        DWORD         dwPriorityClass; 
        BOOL          bGotModule = FALSE; 
        MODULEENTRY32 me32       = {0}; 
 
		ar=ARNew();
        do 
        { 
			/*
            bGotModule = GetProcessModule(pe32.th32ProcessID, 
                pe32.th32ModuleID, &me32, sizeof(MODULEENTRY32)); 

            if (bGotModule) 
            { 
                HANDLE hProcess; 
 
                // Get the actual priority class. 
                hProcess = OpenProcess (PROCESS_ALL_ACCESS, 
                    FALSE, pe32.th32ProcessID); 
                dwPriorityClass = GetPriorityClass (hProcess); 
                CloseHandle (hProcess); 

                // Print the process's information. 
                printf( "\nPriority Class Base\t%d\n", 
                    pe32.pcPriClassBase); 
                printf( "PID\t\t\t%d\n", pe32.th32ProcessID);
                printf( "Thread Count\t\t%d\n", pe32.cntThreads);
                printf( "Module Name\t\t%s\n", me32.szModule);
                printf( "Full Path\t\t%s\n\n", me32.szExePath);
            } 
			*/
//			win_infoarg("[%s]",pe32.szExeFile);
			ARAdd(&ar,pe32.szExeFile);
        } 
        while (Process32Next(hProcessSnap, &pe32)); 
        bRet = TRUE; 
    } 
    else 
        bRet = FALSE;    // could not walk the list of processes 
 
    // Do not forget to clean up the snapshot object. 

    CloseHandle (hProcessSnap); 
    return ar; 
} 


EH_AR FNTGetProcessListEx(CHAR *pProgramName) 
{ 
	HANDLE         hProcessSnap = NULL; 
    BOOL           bRet      = FALSE; 
    PROCESSENTRY32 pe32      = {0}; 
	EH_AR ar=NULL;
 
    //  Take a snapshot of all processes in the system. 

    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); 
    if (hProcessSnap == (HANDLE)-1)  return NULL; 
 
    //  Fill in the size of the structure before using it. 

    pe32.dwSize = sizeof(PROCESSENTRY32); 
 
    //  Walk the snapshot of the processes, and for each process, 
    //  display information. 

    if (Process32First(hProcessSnap, &pe32)) 
    { 
        BOOL          bGotModule = FALSE; 
        MODULEENTRY32 me32       = {0}; 
		PROCESS_MEMORY_COUNTERS sPmc;
		DWORD dwPriorityClass;
		HANDLE hProcess;
		BOOL bCheck;
        do 
        { 
			BOOL bTake=true;
			if (!strEmpty(pProgramName))
			{
				bTake=false;
				if (!strCaseCmp(pe32.szExeFile,pProgramName)) bTake=TRUE;
			}

			if (bTake)
			{
				if (!ar) ar=ARNew();

				hProcess = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
				if (hProcess) {

					dwPriorityClass = GetPriorityClass (hProcess);
					_(sPmc);
					bCheck=GetProcessMemoryInfo(hProcess,&sPmc,sizeof(sPmc));
					if (bCheck) {
/*
				printf("%s " CRLF,pe32.szExeFile);
				printf( "\tPageFaultCount: %d\n", sPmc.PageFaultCount );
				printf( "\tPeakWorkingSetSize: %d\n", sPmc.PeakWorkingSetSize );
				printf( "\tWorkingSetSize: %d\n", sPmc.WorkingSetSize );
				printf( "\tQuotaPeakPagedPoolUsage: %d\n", 
						  sPmc.QuotaPeakPagedPoolUsage );
				printf( "\tQuotaPagedPoolUsage: %d\n", 
						  sPmc.QuotaPagedPoolUsage );
				printf( "\tQuotaPeakNonPagedPoolUsage: %d\n", 
						  sPmc.QuotaPeakNonPagedPoolUsage );
				printf( "\tQuotaNonPagedPoolUsage: %d\n", 
						  sPmc.QuotaNonPagedPoolUsage );
				printf( "\tPagefileUsage: %d\n", sPmc.PagefileUsage ); 
				printf( "\tPeakPagefileUsage: %d\n", 
						  sPmc.PeakPagefileUsage );
*/
						ARAddarg(&ar,"%s|#%d|%u|%u|%d",
											pe32.szExeFile,
											pe32.th32ProcessID,
											sPmc.WorkingSetSize,
											sPmc.PeakPagefileUsage,
											dwPriorityClass);
					}
					CloseHandle(hProcess); 
				}
			}

			// 
        } 
        while (Process32Next(hProcessSnap, &pe32)); 
    } 
 
    // Do not forget to clean up the snapshot object. 

    CloseHandle (hProcessSnap); 
	if (!ARLen(ar)) ar=ARDestroy(ar);
    return ar; 
}
