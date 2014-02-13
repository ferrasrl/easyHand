//   +-------------------------------------------+
//    ehExif
//							by Ferrà srl 2011
//   +-------------------------------------------+


typedef BYTE BYTE;
/*
#ifndef TRUE
    #define TRUE 1
    #define FALSE 0
#endif
*/
#define MAX_COMMENT 2000

#ifdef _WIN32
    #define PATH_MAX _MAX_PATH
#endif

//--------------------------------------------------------------------------
// This structure is used to store jpeg file sections in memory.
//
typedef struct {

    BYTE *		pbData;
    INT			Type;
    unsigned	Size;

} S_EXIF_SECTION;
/*
extern INT ExifSectionIndex;
extern INT DumpExifMap;
*/
#define MAX_DATE_COPIES 10

//--------------------------------------------------------------------------
// This structure stores Exif header image elements in a simple manner
// Used to store camera data as extracted from the various ways that it can be
// stored in an exif header
typedef struct {

	BOOL	bExif;				// TRUE/FALSE se presente
    CHAR	szCameraMake[32];
    CHAR	szCameraModel[40];
    CHAR	szDateTime[20];
    INT		iHeight, iWidth;
    INT		iOrientation;
	CHAR  * pszOrientationText;
    INT		iIsColor;
    INT		iProcess;
    INT		iFlashUsed;
    float	dFocalLength;
    float	dExposureTime;
    float	dApertureFNumber;
    float	dDistance;
    float	dCCDWidth;
    float	dExposureBias;
    float	dDigitalZoomRatio;
    UINT	uiFocalLength35mmEquiv; // Exif 2.2 tag - usually not present.
    INT		iWhitebalance;
    INT		iMeteringMode;
    INT		iExposureProgram;
    INT		iExposureMode;
    INT		iISOequivalent;
    INT		iLightSource;
    CHAR	szComments[MAX_COMMENT];

    UINT	uiThumbnailOffset;          // Exif offset to thumbnail
    UINT	uiThumbnailSize;            // Size of thumbnail.
    UINT	uiLargestExifOffset;        // Last exif data referenced (to check if thumbnail is at end)
    CHAR	cThumbnailAtEnd;              // Exif header ends with the thumbnail
                                       // (we can only modify the thumbnail if its at the end)
    INT		iThumbnailSizeOffset;

    INT		iDateTimeOffsets[MAX_DATE_COPIES];
    INT		iNumDateTimeTags;

    INT		iGpsInfoPresent;
    CHAR	szGpsLat[31];
    CHAR	szGpsLong[31];
    CHAR	szGpsAlt[20];

	EH_AR	arTable;		// Tabella con tutte le informazioni raccolte
	EH_AR	arTablePrint;	// Tabella ideale per la stampa

} S_EXIF; // ImageInfo_t


#define EXIT_FAILURE  1
#define EXIT_SUCCESS  0

// jpgfile.c functions
typedef enum {
    READ_EXIF = 1,
    READ_IMAGE = 2,
    READ_ALL = 3
} EN_EXIF_READMODE; // ReadMode_t


// prototypes for jhead.c functions
void	ErrFatal(CHAR * msg);
void	ErrNonfatal(CHAR * msg, INT a1, INT a2);

// Prototypes for exif.c functions.
INT		Exif2tm(struct tm * timeptr, CHAR * ExifTime);
void	process_EXIF (BYTE * CharBuf, UINT length);
INT		RemoveThumbnail(BYTE * ExifSection);
void	ShowImageInfo(INT ShowFileInfo);
void	ShowConciseImageInfo(void);
const	CHAR * ClearOrientation(void);
void	PrintFormatNumber(void * ValuePtr, INT Format, INT ByteCount);
double	ConvertAnyFormat(void * ValuePtr, INT Format);

//--------------------------------------------------------------------------
// Exif format descriptor stuff
//extern const INT BytesPerFormat[];

#define NUM_FORMATS 12

#define FMT_BYTE       1 
#define FMT_STRING     2
#define FMT_USHORT     3
#define FMT_ULONG      4
#define FMT_URATIONAL  5
#define FMT_SBYTE      6
#define FMT_UNDEFINED  7
#define FMT_SSHORT     8
#define FMT_SLONG      9
#define FMT_SRATIONAL 10
#define FMT_SINGLE    11
#define FMT_DOUBLE    12





// Variables from jhead.c used by exif.c
//extern ImageInfo_t ImageInfo;
//extern INT ShowTags;

//--------------------------------------------------------------------------
// JPEG markers consist of one or more 0xFF bytes, followed by a marker
// code byte (which is not an FF).  Here are the marker codes of interest
// in this program.  (See jdmarker.c for a more complete list.)
//--------------------------------------------------------------------------

#define M_SOF0  0xC0            // Start Of Frame N
#define M_SOF1  0xC1            // N indicates which compression process
#define M_SOF2  0xC2            // Only SOF0-SOF2 are now in common use
#define M_SOF3  0xC3
#define M_SOF5  0xC5            // NB: codes C4 and CC are NOT SOF markers
#define M_SOF6  0xC6
#define M_SOF7  0xC7
#define M_SOF9  0xC9
#define M_SOF10 0xCA
#define M_SOF11 0xCB
#define M_SOF13 0xCD
#define M_SOF14 0xCE
#define M_SOF15 0xCF
#define M_SOI   0xD8            // Start Of Image (beginning of datastream)
#define M_EOI   0xD9            // End Of Image (end of datastream)
#define M_SOS   0xDA            // Start Of Scan (begins compressed data)
#define M_JFIF  0xE0            // Jfif marker
#define M_EXIF  0xE1            // Exif marker
#define M_COM   0xFE            // COMment 
#define M_DQT   0xDB
#define M_DHT   0xC4
#define M_DRI   0xDD



#define MAX_GPS_TAG 0x1e
#define TAG_GPS_LAT_REF    1
#define TAG_GPS_LAT        2
#define TAG_GPS_LONG_REF   3
#define TAG_GPS_LONG       4
#define TAG_GPS_ALT_REF    5
#define TAG_GPS_ALT        6

typedef struct {
    unsigned short Tag;
    char * Desc;
} S_EXIF_TABLE;


BOOL JPGExifRead(UTF8 * pszFileName,S_EXIF * psExif,BOOL bGetArrayCompleted);
BOOL JPGExitFree(S_EXIF * psExif);

// makernote.c prototypes
//extern void ProcessMakerNote(BYTE * DirStart, INT ByteCount, BYTE * OffsetBase, unsigned ExifLength);

// gpsinfo.c prototypes
void ProcessGpsInfo(BYTE * ValuePtr, INT ByteCount, BYTE * OffsetBase, unsigned ExifLength);

// Prototypes for myglob.c module
//extern void MyGlob(const CHAR * Pattern , void (*FileFuncParm)(const CHAR * FileName));

