//   +-------------------------------------------+
//   | Crypt
//   |								Ferrà srl 2015
//   +-------------------------------------------+

#include <windows.h>
#include <wincrypt.h>

BOOL	FCryptStringService(SINT iMode, BYTE *lpSource, BYTE *lpDestination, DWORD dwSizeDest, BYTE *szPassword);
CHAR *	StrToCryptUrl(CHAR *lpStr,CHAR *Password);
CHAR *	CryptUrlToStr(CHAR *lpStr,CHAR *Password);

CHAR *	HMAC(CHAR * pszStr, CHAR * pszPass, DWORD AlgId,BOOL bBase64Ret); // CALG_SHA1 - CALG_MD5

#ifdef WITH_OPENSSL 
CHAR *	sha1(CHAR * pszStr, BOOL bBase64Ret);
#endif

#ifndef CALG_HMAC
#define CALG_HMAC (ALG_CLASS_HASH | ALG_TYPE_ANY | ALG_SID_HMAC)
#endif

#ifndef CRYPT_IPSEC_HMAC_KEY
#define CRYPT_IPSEC_HMAC_KEY 0x00000100
#endif

#pragma comment(lib, "crypt32.lib")


