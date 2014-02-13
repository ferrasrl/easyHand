//   +-------------------------------------------+
//   | CLIPBRD   Programma di accesso al         |
//   |           ClipBoard                       |
//   |           HEADER                          |
//   |                                           |
//   |             by Ferr… Art & Tecnology 1997 |
//   +-------------------------------------------+

#define CB_TEXT         1
#define CB_BITMAP       2
#define CB_METAFILEPICT 3
#define CB_SYLK         4
#define CB_DIF          5
#define CB_TIFF         6
#define CB_OEMTEXT      7
#define CB_DIB          8
#define CB_PALETTE      9

#ifdef _WIN32
SINT InWindows(void);
SINT CBopen(void);
SINT CBclose(void);
SINT CBclear(void);
SINT CBset(CHAR *Byte,WORD len,SINT tipo);
SINT CBget(CHAR *Byte,SINT tipo);
LONG CBsize(SINT tipo);
#else
SINT InWindows(void);
SINT CBopen(void);
SINT CBclose(void);
SINT CBclear(void);
SINT CBset(CHAR *Byte,WORD len,SINT tipo);
SINT CBget(CHAR *Byte,SINT tipo);
LONG CBsize(SINT tipo);
#endif
