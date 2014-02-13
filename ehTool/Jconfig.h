/* jconfig.bcc --- jconfig.h for Borland C (Turbo C) on MS-DOS. */
/* see jconfig.doc for explanations */

#define HAVE_PROTOTYPES
#define HAVE_UNSIGNED_CHAR
#define HAVE_UNSIGNED_SHORT
/* #define void CHAR */
/* #define const */
#undef CHAR_IS_UNSIGNED
#define HAVE_STDDEF_H
#define HAVE_STDLIB_H
#undef NEED_BSD_STRINGS
#undef NEED_SYS_TYPES_H
/*#define NEED_FAR_POINTERS       /* for small or medium memory model */
#undef NEED_SHORT_EXTERNAL_NAMES
#define INCOMPLETE_TYPES_BROKEN	/* suppress undefined-structure warnings */

#ifdef JPEG_INTERNALS

#undef RIGHT_SHIFT_IS_UNSIGNED

/*#define USE_MSDOS_MEMMGR        /* Define this if you use jmemdos.c */

/*#define MAX_ALLOC_CHUNK 65520L  /* Maximum request to malloc() */

/*#define USE_FMEM                /* Borland has _fmemcpy() and _fmemset() */

#endif /* JPEG_INTERNALS */

#ifdef JPEG_CJPEG_DJPEG

#define BMP_SUPPORTED		/* BMP image file format */
#define GIF_SUPPORTED		/* GIF image file format */
#define PPM_SUPPORTED		/* PBMPLUS PPM/PGM image file format */
#undef RLE_SUPPORTED		/* Utah RLE image file format */
#define TARGA_SUPPORTED		/* Targa image file format */

/*#define TWO_FILE_COMMANDLINE*/
#define USE_SETMODE		/* Borland has setmode() */
#define NEED_SIGNAL_CATCHER	/* Define this if you use jmemdos.c */
#undef DONT_USE_B_MODE
#undef PROGRESS_REPORT		/* optional */

#endif /* JPEG_CJPEG_DJPEG */
