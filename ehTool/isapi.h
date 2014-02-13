#include "httpext.h"
CHAR *Isapi_VarAlloc(EXTENSION_CONTROL_BLOCK *pECB,CHAR *lpVarName);
CHAR *Isapi_ArgAlloc(EXTENSION_CONTROL_BLOCK *pECB,CHAR *lpVarName);
void Isapi_printf(EXTENSION_CONTROL_BLOCK *pECB, char *pszFormat, ...);
DWORD Isapi_SendHeaderToClient(EXTENSION_CONTROL_BLOCK *pECB,SINT iStatus,char *lpStatus,char *pszFormat, ...);
BOOL Isapi_FilePutOut(EXTENSION_CONTROL_BLOCK *pECB,SINT iStatus,CHAR *lpStatus,CHAR *lpFile,SINT iMode);

