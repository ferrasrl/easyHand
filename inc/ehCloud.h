//
// ehCloud.h <--------------
//

#include "/easyhand/ehtoolx/Zlib/zlib.h"

typedef struct {

	INT		iType;	// 0=Ferra
	
	CHAR	szCloud[280];
	CHAR	szUser[280];
	CHAR	szPassword[280];
	CHAR	szCodex[50];

	BOOL	bLogin;
	CHAR *	pszKey;	// Chiave di accesso
	CHAR *	pszKeyId_flat;	// Chiave di accesso
	CHAR *	pszKeyId_encoded;	// Chiave di accesso
	UINT	idUser;

	CHAR	szUrl[280];
	CHAR	szSection[200];
	CHAR	szLocalFolder[500];
	void  * (*funcNotify)(EH_SRVPARAMS);

} EH_CLOUD;

typedef struct {

	INT idCode;	
	CHAR szNome[80];
	CHAR szCognome[80];
	CHAR szPassword[80];
	CHAR szCodex[80];
	BOOL bLock;

	CHAR szFolder[500];

} S_CLOUD_USER;

INT		ehcFileUpdate(	UTF8 * pszCloudServer, // NULL = Ferra
						UTF8 * pszDirLocal,
						UTF8 * pszDirRemote,
						UTF8 * pszFileNames,
						BOOL   bShowActivity);

BOOL	ehcOpen(EH_CLOUD * psCloud,CHAR * pszCloud,CHAR * pszUser,CHAR * pszPassword);
void	ehcNotify(EH_CLOUD * psCloud,void * (funcNotify)(EH_SRVPARAMS));
BOOL	ehcClose(EH_CLOUD * psCloud);
BOOL	ehcFileSync(EH_CLOUD * psCloud,CHAR * pszLocalFolderCloud,CHAR * pszRemoteFolder,BOOL bSubDirRecorsive,EH_LST lstError);
BOOL	ehcFileDownload(EH_CLOUD * psCloud,CHAR * pszFileName,EH_LST lstError);
BOOL	ehcFileUpload(EH_CLOUD * psCloud,CHAR * pszFileName,EH_LST lstError);


#ifdef EH_INTERNET
BOOL	ehcWebZip(CHAR * pszWhat,BYTE * pbDataSource,ULONG ulLength);
BYTE *	ehcWebUnzip(EH_WEB * psWeb,ULONG * pulSize);
#endif