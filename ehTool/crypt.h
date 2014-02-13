//   +-------------------------------------------+
//   | Crypt
//   |									Ferrà srl
//   +-------------------------------------------+

BOOL FCryptStringService(SINT iMode, BYTE *lpSource, BYTE *lpDestination, DWORD dwSizeDest, BYTE *szPassword);
CHAR *StrToCryptUrl(CHAR *lpStr,CHAR *Password);
CHAR *CryptUrlToStr(CHAR *lpStr,CHAR *Password);
