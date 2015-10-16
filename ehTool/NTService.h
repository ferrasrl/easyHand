//   ---------------------------------------------
//	 | NTSERVICE                                 |
//	 | Gestione Windows Service                  |
//	 |                             by Ferrà 2004 |
//   ---------------------------------------------
#include <tlhelp32.h>
#include <psapi.h>

typedef struct {
   
	SC_HANDLE hSCM;
	SC_HANDLE hService;
	SERVICE_STATUS sServiceStatus;

} S_NT_SERVICE;

BOOL	FNTServiceGet(CHAR * pszServiceName,DWORD dwMode,S_NT_SERVICE * psNtService);
void	FNTServiceFree(S_NT_SERVICE * psNtService);

DWORD	FNTServiceStop(CHAR *lpService, BOOL fStopDependencies, DWORD dwTimeout ,BOOL fShowWindow);
DWORD	FNTServiceStart(CHAR *lpService, DWORD dwTimeout ,BOOL fShowWindow);
DWORD	FNTServiceRestart(CHAR * pszServiceName, DWORD dwTimeout ,BOOL fShowWindow);

//BOOL FNTGetProcessList(void); // Lista i processi in memoria
EH_AR	FNTGetProcessList(void);
BOOL	FNTKillProcess(CHAR *lpExeName); // Chiude "VIULENTEMENTE" un processo
EH_AR	FNTGetProcessListEx(CHAR *pProgramName); // new 2009
BOOL	FNTServiceCheck(CHAR * pszServiceName); // new 2011


