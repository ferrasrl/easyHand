// -----------------------------------------------------
// eh_zip.h
//
// -----------------------------------------------------

#include "/easyhand/ehtoolx/Zlib/zlib.h"

BYTE * ehGunzip(BYTE * pbSource,SIZE_T iSourceSize,SIZE_T * psizDest); // G..Unzippa
BYTE * ehGzip(BYTE * pbSource,SIZE_T iSourceSize,SIZE_T * psizDest); // G..zippa