//
// cryptAccess
//
#include "/easyhand/inc/blowFish.h"

BOOL	caCreate(CHAR * pszLocalName,CHAR * pszRemoteName,CHAR * pszUserName,CHAR * pszPassword);
BOOL	caDestroy(CHAR * pszRemoteName);
BOOL	caAddFile(	CHAR * pszNameUrl,
					CHAR * pszUser,
					CHAR * pszPassword,
					CHAR * pszBlowFish);
BOOL	caRemoveFile(CHAR * pszNameUrl,CHAR * pszBlowFish);

EH_LST caOpenAll(CHAR * pszBlowFish,INT * piError);
BOOL	caCloseAll(EH_LST lstConn);

