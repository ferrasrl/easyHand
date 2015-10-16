	// ------------------------------------------------
//  ehExif
//  by	Agostino Dondi 2010
//		Giorgio Tassistro 2011
//									Ferrà srl 2011
// ------------------------------------------------


#include "/easyhand/inc/easyhand.h"
#include "/easyhand/inc/ehExif.h"
#include <math.h>
#include <time.h>

#ifndef _WIN32
    #include <limits.h>
#endif

// S_IMAGE_INFO sImageInfo;

struct {
	unsigned char * DirWithThumbnailPtrs;
	double	FocalplaneXRes;
	double	FocalplaneUnits;
	int		ExifImageWidth;
	void *	OrientationPtr;
	int		OrientationNumFormat; 

} _sExif;
static	int		DumpExifMap  = FALSE;
//static	int		_bShowTags   = TRUE;    

// Prototypes from jpgfile.c

void		_DiscardData(void);
void	DiscardAllButExif(void);
static INT	_ReadJpegSections(FILE * infile, EN_EXIF_READMODE ReadMode,S_EXIF * psExif);
static void _processGpsInfo(S_EXIF *psExif, unsigned char * DirStart, int ByteCountUnused, unsigned char * OffsetBase, unsigned ExifLength);
INT		ReplaceThumbnail(const CHAR * ThumbFileName);
INT		SaveThumbnail(CHAR * ThumbFileName);
//INT		RemoveSectionType(INT SectionType);
INT		RemoveUnknownSections(void);
void	WriteJpegFile(const CHAR * FileName);
S_EXIF_SECTION * FindSection(INT SectionType);
S_EXIF_SECTION * CreateSection(INT SectionType, BYTE * Data, INT size);
void	ResetJpgfile(void);


//
// JPGExifRead()
//
BOOL JPGExifRead(UTF8 * pszFileName,S_EXIF * psExif,BOOL bGetArrayCompleted) {
	
	FILE *pf;
	WCHAR *pwcFile;
	INT iRet;
	BOOL bRet=TRUE;
	pwcFile=utfToWcs(pszFileName);
	pf=_wfopen(pwcFile,L"rb"); if (!pf) return bRet;

	memset(psExif,0,sizeof(S_EXIF));
	if (bGetArrayCompleted) {
		psExif->arTable=ARNew();
		psExif->arTablePrint=ARNew();
	}

    // Scan the JPEG headers.
    iRet = _ReadJpegSections(pf, READ_EXIF,psExif);
	if (iRet) {
		bRet=FALSE;
    } 
	fclose(pf);
	ehFree(pwcFile);
	_DiscardData();

	return bRet;
}

BOOL JPGExitFree(S_EXIF * psExif) {

	if (psExif->arTable) psExif->arTable=ARDestroy(psExif->arTable);
	if (psExif->arTablePrint) psExif->arTablePrint=ARDestroy(psExif->arTablePrint);
	return FALSE;
}

//static unsigned char * DirWithThumbnailPtrs;
//static double	FocalplaneXRes;
//static double	FocalplaneUnits;
//static int		ExifImageWidth;
static int		MotorolaOrder = 0;

// for fixing the rotation.

static int SupressNonFatalErrors = TRUE; 
static const char * CurrentFile;

#define MAX_SECTIONS 100
void ProcessMakerNote(S_EXIF * psExif,unsigned char * ValuePtr, int ByteCount, unsigned char * OffsetBase, unsigned ExifLength);
	   
static S_EXIF_SECTION Sections[MAX_SECTIONS];
static int SectionsRead;
static int HaveAll;


//--------------------------------------------------------------------------
// Table of Jpeg encoding process names
static const S_EXIF_TABLE ProcessTable[] = {
    { M_SOF0,   "Baseline"},
    { M_SOF1,   "Extended sequential"},
    { M_SOF2,   "Progressive"},
    { M_SOF3,   "Lossless"},
    { M_SOF5,   "Differential sequential"},
    { M_SOF6,   "Differential progressive"},
    { M_SOF7,   "Differential lossless"},
    { M_SOF9,   "Extended sequential, arithmetic coding"},
    { M_SOF10,  "Progressive, arithmetic coding"},
    { M_SOF11,  "Lossless, arithmetic coding"},
    { M_SOF13,  "Differential sequential, arithmetic coding"},
    { M_SOF14,  "Differential progressive, arithmetic coding"},
    { M_SOF15,  "Differential lossless, arithmetic coding"},
};

#define PROCESS_TABLE_SIZE  (sizeof(ProcessTable) / sizeof(S_EXIF_TABLE))

// 1 - "The 0th row is at the visual top of the image,    and the 0th column is the visual left-hand side."
// 2 - "The 0th row is at the visual top of the image,    and the 0th column is the visual right-hand side."
// 3 - "The 0th row is at the visual bottom of the image, and the 0th column is the visual right-hand side."
// 4 - "The 0th row is at the visual bottom of the image, and the 0th column is the visual left-hand side."
// 5 - "The 0th row is the visual left-hand side of of the image,  and the 0th column is the visual top."
// 6 - "The 0th row is the visual right-hand side of of the image, and the 0th column is the visual top."
// 7 - "The 0th row is the visual right-hand side of of the image, and the 0th column is the visual bottom."
// 8 - "The 0th row is the visual left-hand side of of the image,  and the 0th column is the visual bottom."

// Note: The descriptions here are the same as the name of the command line
// option to pass to jpegtran to right the image

static const char * arOrientTab[] = {
    "Undefined",
    "Normal",           // 1
    "flip horizontal",  // left right reversed mirror
    "rotate 180",       // 3
    "flip vertical",    // upside down mirror
    "transpose",        // Flipped about top-left <--> bottom-right axis.
    "rotate 90",        // rotate 90 cw to right it.
    "transverse",       // flipped about top-right <--> bottom-left axis
    "rotate 270",       // rotate 270 to right it.
	NULL
};

const int BytesPerFormat[] = {0,1,1,2,4,8,1,1,2,4,8,4,8};

//--------------------------------------------------------------------------
// Describes tag values

#define TAG_MAKE               0x010F
#define TAG_MODEL              0x0110
#define TAG_ORIENTATION        0x0112
#define TAG_DATETIME           0x0132
#define TAG_THUMBNAIL_OFFSET   0x0201
#define TAG_THUMBNAIL_LENGTH   0x0202
#define TAG_EXPOSURETIME       0x829A
#define TAG_FNUMBER            0x829D
#define TAG_EXIF_OFFSET        0x8769
#define TAG_EXPOSURE_PROGRAM   0x8822
#define TAG_GPSINFO            0x8825
#define TAG_ISO_EQUIVALENT     0x8827
#define TAG_DATETIME_ORIGINAL  0x9003
#define TAG_DATETIME_DIGITIZED 0x9004
#define TAG_SHUTTERSPEED       0x9201
#define TAG_APERTURE           0x9202
#define TAG_EXPOSURE_BIAS      0x9204
#define TAG_MAXAPERTURE        0x9205
#define TAG_SUBJECT_DISTANCE   0x9206
#define TAG_METERING_MODE      0x9207
#define TAG_LIGHT_SOURCE       0x9208
#define TAG_FLASH              0x9209
#define TAG_FOCALLENGTH        0x920A
#define TAG_MAKER_NOTE         0x927C
#define TAG_USERCOMMENT        0x9286
#define TAG_EXIF_IMAGEWIDTH    0xa002
#define TAG_EXIF_IMAGELENGTH   0xa003
#define TAG_INTEROP_OFFSET     0xa005
#define TAG_FOCALPLANEXRES     0xa20E
#define TAG_FOCALPLANEUNITS    0xa210
#define TAG_EXPOSURE_INDEX     0xa215
#define TAG_EXPOSURE_MODE      0xa402
#define TAG_WHITEBALANCE       0xa403
#define TAG_DIGITALZOOMRATIO   0xA404
#define TAG_FOCALLENGTH_35MM   0xa405

static const S_EXIF_TABLE arTagTable[] = {
  { 0x001,   "InteropIndex"},
  { 0x002,   "InteropVersion"},
  { 0x100,   "ImageWidth"},
  { 0x101,   "ImageLength"},
  { 0x102,   "BitsPerSample"},
  { 0x103,   "Compression"},
  { 0x106,   "PhotometricInterpretation"},
  { 0x10A,   "FillOrder"},
  { 0x10D,   "DocumentName"},
  { 0x10E,   "ImageDescription"},
  { 0x10F,   "Make"},
  { 0x110,   "Model"},
  { 0x111,   "StripOffsets"},
  { 0x112,   "Orientation"},
  { 0x115,   "SamplesPerPixel"},
  { 0x116,   "RowsPerStrip"},
  { 0x117,   "StripByteCounts"},
  { 0x11A,   "XResolution"},
  { 0x11B,   "YResolution"},
  { 0x11C,   "PlanarConfiguration"},
  { 0x128,   "ResolutionUnit"},
  { 0x12D,   "TransferFunction"},
  { 0x131,   "Software"},
  { 0x132,   "DateTime"},
  { 0x13B,   "Artist"},
  { 0x13E,   "WhitePoint"},
  { 0x13F,   "PrimaryChromaticities"},
  { 0x156,   "TransferRange"},
  { 0x200,   "JPEGProc"},
  { 0x201,   "ThumbnailOffset"},
  { 0x202,   "ThumbnailLength"},
  { 0x211,   "YCbCrCoefficients"},
  { 0x212,   "YCbCrSubSampling"},
  { 0x213,   "YCbCrPositioning"},
  { 0x214,   "ReferenceBlackWhite"},
  { 0x1001,  "RelatedImageWidth"},
  { 0x1002,  "RelatedImageLength"},
  { 0x828D,  "CFARepeatPatternDim"},
  { 0x828E,  "CFAPattern"},
  { 0x828F,  "BatteryLevel"},
  { 0x8298,  "Copyright"},
  { 0x829A,  "ExposureTime"},
  { 0x829D,  "FNumber"},
  { 0x83BB,  "IPTC/NAA"},
  { 0x8769,  "ExifOffset"},
  { 0x8773,  "InterColorProfile"},
  { 0x8822,  "ExposureProgram"},
  { 0x8824,  "SpectralSensitivity"},
  { 0x8825,  "GPS Dir offset"},
  { 0x8827,  "ISOSpeedRatings"},
  { 0x8828,  "OECF"},
  { 0x9000,  "ExifVersion"},
  { 0x9003,  "DateTimeOriginal"},
  { 0x9004,  "DateTimeDigitized"},
  { 0x9101,  "ComponentsConfiguration"},
  { 0x9102,  "CompressedBitsPerPixel"},
  { 0x9201,  "ShutterSpeedValue"},
  { 0x9202,  "ApertureValue"},
  { 0x9203,  "BrightnessValue"},
  { 0x9204,  "ExposureBiasValue"},
  { 0x9205,  "MaxApertureValue"},
  { 0x9206,  "SubjectDistance"},
  { 0x9207,  "MeteringMode"},
  { 0x9208,  "iLightSource"},
  { 0x9209,  "Flash"},
  { 0x920A,  "dFocalLength"},
  { 0x927C,  "MakerNote"},
  { 0x9286,  "UserComment"},
  { 0x9290,  "SubSecTime"},
  { 0x9291,  "SubSecTimeOriginal"},
  { 0x9292,  "SubSecTimeDigitized"},
  { 0xA000,  "FlashPixVersion"},
  { 0xA001,  "ColorSpace"},
  { 0xA002,  "ExifImageWidth"},
  { 0xA003,  "ExifImageLength"},
  { 0xA004,  "RelatedAudioFile"},
  { 0xA005,  "InteroperabilityOffset"},
  { 0xA20B,  "FlashEnergy"},              
  { 0xA20C,  "SpatialFrequencyResponse"}, 
  { 0xA20E,  "FocalPlaneXResolution"},    
  { 0xA20F,  "FocalPlaneYResolution"},    
  { 0xA210,  "FocalPlaneResolutionUnit"}, 
  { 0xA214,  "SubjectLocation"},          
  { 0xA215,  "ExposureIndex"},            
  { 0xA217,  "SensingMethod"},            
  { 0xA300,  "FileSource"},
  { 0xA301,  "SceneType"},
  { 0xA301,  "CFA Pattern"},
  { 0xA401,  "CustomRendered"},
  { 0xA402,  "ExposureMode"},
  { 0xA403,  "WhiteBalance"},
  { 0xA404,  "DigitalZoomRatio"},
  { 0xA405,  "FocalLengthIn35mmFilm"},
  { 0xA406,  "SceneCaptureType"},
  { 0xA407,  "GainControl"},
  { 0xA408,  "Contrast"},
  { 0xA409,  "Saturation"},
  { 0xA40a,  "Sharpness"},
  { 0xA40c,  "SubjectDistanceRange"},
} ;

#define TAG_TABLE_SIZE  (sizeof(arTagTable) / sizeof(S_EXIF_TABLE))

/*
INT		Get16u(void * Short);
UINT	_Get32u(void * Long);
INT		_Get32s(void * Long);
void	_Put32u(void * Value, unsigned PutValue);
*/

//--------------------------------------------------------------------------
// Convert a 16 bit unsigned value to file's native byte order
//--------------------------------------------------------------------------
static void _Put16u(void * Short, unsigned short PutValue)
{
    if (MotorolaOrder){
        ((BYTE *) Short)[0] = (BYTE) (PutValue>>8);
        ((BYTE *) Short)[1] = (BYTE) PutValue;
    }else{
        ((BYTE *) Short)[0] = (BYTE) PutValue;
        ((BYTE *) Short)[1] = (BYTE) (PutValue>>8);
    }
}

//--------------------------------------------------------------------------
// Convert a 16 bit unsigned value from file's native byte order
//--------------------------------------------------------------------------
static INT _Get16u(void * Short)
{
    if (MotorolaOrder){
        return (((BYTE *)Short)[0] << 8) | ((BYTE *)Short)[1];
    }else{
        return (((BYTE *)Short)[1] << 8) | ((BYTE *)Short)[0];
    }
}

//--------------------------------------------------------------------------
// Convert a 32 bit signed value from file's native byte order
//--------------------------------------------------------------------------
static int _Get32s(void * Long)
{
    if (MotorolaOrder){
        return  ((( char *)Long)[0] << 24) | (((BYTE *)Long)[1] << 16)
              | (((BYTE *)Long)[2] << 8 ) | (((BYTE *)Long)[3] << 0 );
    }else{
        return  ((( char *)Long)[3] << 24) | (((BYTE *)Long)[2] << 16)
              | (((BYTE *)Long)[1] << 8 ) | (((BYTE *)Long)[0] << 0 );
    }
}

//--------------------------------------------------------------------------
// Convert a 32 bit unsigned value to file's native byte order
//--------------------------------------------------------------------------
static void _Put32u(void * Value, unsigned PutValue)
{
    if (MotorolaOrder){
        ((BYTE *)Value)[0] = (BYTE)(PutValue>>24);
        ((BYTE *)Value)[1] = (BYTE)(PutValue>>16);
        ((BYTE *)Value)[2] = (BYTE)(PutValue>>8);
        ((BYTE *)Value)[3] = (BYTE)PutValue;
    }else{
        ((BYTE *)Value)[0] = (BYTE)PutValue;
        ((BYTE *)Value)[1] = (BYTE)(PutValue>>8);
        ((BYTE *)Value)[2] = (BYTE)(PutValue>>16);
        ((BYTE *)Value)[3] = (BYTE)(PutValue>>24);
    }
}

//--------------------------------------------------------------------------
// Convert a 32 bit unsigned value from file's native byte order
//--------------------------------------------------------------------------
static unsigned _Get32u(void * Long)
{
    return (unsigned)_Get32s(Long) & 0xffffffff;
}

//--------------------------------------------------------------------------
// Display a number as one of its many formats
//--------------------------------------------------------------------------
static void _getFormatNumber(CHAR * pszDest,void * ValuePtr, int Format, int ByteCount)
{
    int s,n;
	*pszDest=0;
    for(n=0;n<16;n++){

        switch(Format){

            case FMT_SBYTE:
            case FMT_BYTE:      strAppend(pszDest,"%02x",*(BYTE *)ValuePtr); s=1;  break;
            case FMT_USHORT:    strAppend(pszDest,"%d",_Get16u(ValuePtr)); s=2; break;
            case FMT_ULONG:     
            case FMT_SLONG:     strAppend(pszDest,"%d",_Get32s(ValuePtr)); s=4; break;
            case FMT_SSHORT:    strAppend(pszDest,"%hd",(signed short)_Get16u(ValuePtr)); s=2; break;
            case FMT_URATIONAL:
            case FMT_SRATIONAL: 
               strAppend(pszDest,"%d/%d",_Get32s(ValuePtr), _Get32s(4+(char *)ValuePtr)); 
               s = 8;
               break;

            case FMT_SINGLE:    strAppend(pszDest,"%f",(double)*(float *)ValuePtr); s=8; break;
            case FMT_DOUBLE:    strAppend(pszDest,"%f",*(double *)ValuePtr); s=8; break;
            default: 
                strAppend(pszDest,"Unknown format %d:", Format);
                return;
        }
        ByteCount -= s;
        if (ByteCount <= 0) break;
        strcat(pszDest,", ");
        ValuePtr = (void *)((char *)ValuePtr + s);

    }
    if (n >= 16) strcat(pszDest,"...");
}


//--------------------------------------------------------------------------
// Evaluate number, be it int, rational, or float from directory.
//--------------------------------------------------------------------------
double _ConvertAnyFormat(void * ValuePtr, int Format)
{
    double Value;
    Value = 0;

    switch(Format){

        case FMT_SBYTE:     Value = *(signed char *)ValuePtr;  break;
        case FMT_BYTE:      Value = *(BYTE *)ValuePtr;        break;

        case FMT_USHORT:    Value = _Get16u(ValuePtr);          break;
        case FMT_ULONG:     Value = _Get32u(ValuePtr);          break;

        case FMT_URATIONAL:
        case FMT_SRATIONAL: 
            {
                int Num,Den;
                Num = _Get32s(ValuePtr);
                Den = _Get32s(4+(char *)ValuePtr);
                if (Den == 0){
                    Value = 0;
                }else{
                    Value = (double)Num/Den;
                }
                break;
            }

        case FMT_SSHORT:    Value = (signed short)_Get16u(ValuePtr);  break;
        case FMT_SLONG:     Value = _Get32s(ValuePtr);                break;

        // Not sure if this is correct (never seen float used in Exif format)
        case FMT_SINGLE:    Value = (double)*(float *)ValuePtr;      break;
        case FMT_DOUBLE:    Value = *(double *)ValuePtr;             break;

        default:
            ErrNonfatal("Illegal format code %d",Format,0);
    }
    return Value;
}

//--------------------------------------------------------------------------
// iProcess one of the nested EXIF directories.
//--------------------------------------------------------------------------
static void _ProcessExifDir(	S_EXIF * psExif,
								unsigned char * DirStart, 
								unsigned char * OffsetBase, 
								unsigned ExifLength, 
								int NestingLevel)
{
    int de;
    int a;
    int NumDirEntries;
    unsigned ThumbnailOffset = 0;
    unsigned ThumbnailSize = 0;
    char IndentString[25];

    if (NestingLevel > 4){
        ErrNonfatal("Maximum directory nesting exceeded (corrupt exif header)", 0,0);
        return;
    }

    memset(IndentString, ' ', 25);
    IndentString[NestingLevel * 4] = '\0';
	psExif->bExif=TRUE;

    NumDirEntries = _Get16u(DirStart);
    #define DIR_ENTRY_ADDR(Start, Entry) (Start+2+12*(Entry))

    {
        unsigned char * DirEnd;
        DirEnd = DIR_ENTRY_ADDR(DirStart, NumDirEntries);
        if (DirEnd+4 > (OffsetBase+ExifLength)){
            if (DirEnd+2 == OffsetBase+ExifLength || DirEnd == OffsetBase+ExifLength){
                // Version 1.3 of jhead would truncate a bit too much.
                // This also caught later on as well.
            }else{
                ErrNonfatal("Illegally sized directory",0,0);
                return;
            }
        }
        if (DumpExifMap){
            printf("Map: %05d-%05d: Directory\n",DirStart-OffsetBase, DirEnd+4-OffsetBase);
        }


    }

	if (psExif->arTablePrint){
        ARAddarg(&psExif->arTablePrint,"%s> Dir has %d entries)",IndentString,NumDirEntries); // strcpy(szName,IndentString);
    }

    for (de=0;de<NumDirEntries;de++){
        int Tag, Format, Components;
        unsigned char * ValuePtr;
        int ByteCount;
        unsigned char * DirEntry;
        DirEntry = DIR_ENTRY_ADDR(DirStart, de);

        Tag = _Get16u(DirEntry);
        Format = _Get16u(DirEntry+2);
        Components = _Get32u(DirEntry+4);

        if ((Format-1) >= NUM_FORMATS) {
            // (-1) catches illegal zero case as unsigned underflows to positive large.
            ErrNonfatal("Illegal number format %d for tag %04x", Format, Tag);
            continue;
        }

        if ((unsigned)Components > 0x10000){
            ErrNonfatal("Illegal number of components %d for tag %04x", Components, Tag);
            continue;
        }

        ByteCount = Components * BytesPerFormat[Format];

        if (ByteCount > 4){
            unsigned OffsetVal;
            OffsetVal = _Get32u(DirEntry+8);
            // If its bigger than 4 bytes, the dir entry contains an offset.
            if (OffsetVal+ByteCount > ExifLength){
                // Bogus pointer offset and / or bytecount value
                ErrNonfatal("Illegal value pointer for tag %04x", Tag,0);
                continue;
            }
            ValuePtr = OffsetBase+OffsetVal;

            if (OffsetVal > psExif->uiLargestExifOffset){
                psExif->uiLargestExifOffset = OffsetVal;
            }

            if (DumpExifMap){
                printf("Map: %05d-%05d:   Data for tag %04x\n",OffsetVal, OffsetVal+ByteCount, Tag);
            }
        }else{
            // 4 bytes or less and value is in the dir entry itself
            ValuePtr = DirEntry+8;
        }

        if (Tag == TAG_MAKER_NOTE){
            if (psExif->arTablePrint){
               ARAddarg(&psExif->arTablePrint,"%s    Maker note: ",IndentString);
            }
            ProcessMakerNote(psExif,ValuePtr, ByteCount, OffsetBase, ExifLength);
            continue;
        }

        if (psExif->arTablePrint){
			
			CHAR szName[1024];
			CHAR szCode[1024];
			CHAR szValue[1024];

			strcpy(szName,IndentString);
            // Show tag name
            for (a=0;;a++){

                if (a >= TAG_TABLE_SIZE){
                    strAppend(szName,"    Unknown Tag %04x ", Tag);
					sprintf(szCode,"EXIF|%04X|?",Tag);
                    break;
                }
                if (arTagTable[a].Tag == Tag){
                    strAppend(szName,"    %s",arTagTable[a].Desc);
					sprintf(szCode,"EXIF|%04X|%s",Tag,arTagTable[a].Desc);
                    break;
                }
            }
			
			*szValue=0;
            // Show tag value.
            switch(Format){

                case FMT_BYTE:
                    if(ByteCount>1){
                        sprintf(szValue,"%.*ls\n", ByteCount/2, (wchar_t *)ValuePtr);
                    }else{
                        _getFormatNumber(szValue,ValuePtr, Format, ByteCount);
//                        printf("\n");
                    }

                    break;

                case FMT_UNDEFINED:
					*szValue=0;
					break;
                    // Undefined is typically an ascii string.

                case FMT_STRING:
					
					a=ByteCount; if (a>(sizeof(szValue)-1)) a=sizeof(szValue);
					memcpy(szValue,ValuePtr,a); szValue[a]=0;
/*
                    // String arrays printed without function call (different from int arrays)
                    {
                        int NoPrint = 0;
                        printf("\"");
                        for (a=0;a<ByteCount;a++){
                            if (ValuePtr[a] >= 32){
                                putchar(ValuePtr[a]);
                                NoPrint = 0;
                            }else{
                                // Avoiding indicating too many unprintable characters of proprietary
                                // bits of binary information this program may not know how to parse.
                                if (!NoPrint && a != ByteCount-1){
                                    putchar('?');
                                    NoPrint = 1;
                                }
                            }
                        }
                        printf("\"\n");
                    }
					*/
                    break;

                default:
                    // Handle arrays of numbers later (will there ever be?)
                    _getFormatNumber(szValue,ValuePtr, Format, ByteCount);
//                    printf("\n");
            }
		 ARAddarg(&psExif->arTablePrint,"%s = \"%s\"",szName,szValue);	
		 strTrim(szCode);
		 ARAddarg(&psExif->arTable,"%s|%s",szCode,szValue);	
        }

		//
        // Estraggo i componenti per il tag statici
		//
        switch(Tag){

            case TAG_MAKE:
                strCpy(psExif->szCameraMake, (char *)ValuePtr, sizeof(psExif->szCameraMake));//ByteCount < 31 ? ByteCount : 31);
                break;

            case TAG_MODEL:
                strCpy(psExif->szCameraModel, (char *)ValuePtr, sizeof(psExif->szCameraModel));//ByteCount < 39 ? ByteCount : 39);
                break;

            case TAG_DATETIME_ORIGINAL:
                // If we get a DATETIME_ORIGINAL, we use that one.
                strCpy(psExif->szDateTime, (char *)ValuePtr, sizeof(psExif->szDateTime));
                // Fallthru...

            case TAG_DATETIME_DIGITIZED:
            case TAG_DATETIME:
                if (!isdigit(psExif->szDateTime[0])){
                    // If we don't already have a DATETIME_ORIGINAL, use whatever
                    // time fields we may have.
                    strCpy(psExif->szDateTime, (char *)ValuePtr, sizeof(psExif->szDateTime));
                }

                if (psExif->iNumDateTimeTags >= MAX_DATE_COPIES){
                    ErrNonfatal("More than %d date fields!  This is nuts", MAX_DATE_COPIES, 0);
                    break;
                }
                psExif->iDateTimeOffsets[psExif->iNumDateTimeTags++] = 
                    (char *)ValuePtr - (char *)OffsetBase;
                break;


            case TAG_USERCOMMENT:
                // Olympus has this padded with trailing spaces.  Remove these first.
                for (a=ByteCount;;){
                    a--;
                    if ((ValuePtr)[a] == ' '){
                        (ValuePtr)[a] = '\0';
                    }else{
                        break;
                    }
                    if (a == 0) break;
                }

                // Copy the comment
                if (memcmp(ValuePtr, "ASCII",5) == 0){
                    for (a=5;a<10;a++){
                        int c;
                        c = (ValuePtr)[a];
                        if (c != '\0' && c != ' '){
                            strCpy(psExif->szComments, (char *)ValuePtr+a, sizeof(psExif->szComments));
                            break;
                        }
                    }
                    
                }else{
                    strCpy(psExif->szComments, (char *)ValuePtr, sizeof(psExif->szComments));
                }
                break;

            case TAG_FNUMBER:
                // Simplest way of expressing aperture, so I trust it the most.
                // (overwrite previously computd value if there is one)
                psExif->dApertureFNumber = (float) _ConvertAnyFormat(ValuePtr, Format);
                break;

            case TAG_APERTURE:
            case TAG_MAXAPERTURE:
                // More relevant info always comes earlier, so only use this field if we don't 
                // have appropriate aperture information yet.
                if (psExif->dApertureFNumber == 0){
                    psExif->dApertureFNumber 
                        = (float) exp(_ConvertAnyFormat(ValuePtr, Format)*log(2)*0.5);
                }
                break;

            case TAG_FOCALLENGTH:
                // Nice digital cameras actually save the focal length as a function
                // of how farthey are zoomed in.
                psExif->dFocalLength = (float)_ConvertAnyFormat(ValuePtr, Format);
                break;

            case TAG_SUBJECT_DISTANCE:
                // Inidcates the distacne the autofocus camera is focused to.
                // Tends to be less accurate as distance increases.
                psExif->dDistance = (float)_ConvertAnyFormat(ValuePtr, Format);
                break;

            case TAG_EXPOSURETIME:
                // Simplest way of expressing exposure time, so I trust it most.
                // (overwrite previously computd value if there is one)
                psExif->dExposureTime = (float)_ConvertAnyFormat(ValuePtr, Format);
                break;

            case TAG_SHUTTERSPEED:
                // More complicated way of expressing exposure time, so only use
                // this value if we don't already have it from somewhere else.
                if (psExif->dExposureTime == 0){
                    psExif->dExposureTime = (float)(1/exp(_ConvertAnyFormat(ValuePtr, Format)*log(2)));
                }
                break;


            case TAG_FLASH:
                psExif->iFlashUsed=(int)_ConvertAnyFormat(ValuePtr, Format);
                break;

            case TAG_ORIENTATION:

				if (_sExif.OrientationPtr){
                    // Already have orientation.  The second orientation is likely
                    // to be that of the thumbnail image.
                    break;
                }

				psExif->iOrientation = (int) _ConvertAnyFormat(ValuePtr, Format);
                _sExif.OrientationPtr = ValuePtr;
                _sExif.OrientationNumFormat = Format;
                if (psExif->iOrientation < 0 || psExif->iOrientation > 8){
                    ErrNonfatal("Undefined rotation value %d", psExif->iOrientation, 0);
                    psExif->iOrientation = 0;
                }
				psExif->pszOrientationText=(CHAR *) arOrientTab[psExif->iOrientation];
                break;

            case TAG_EXIF_IMAGELENGTH:
            case TAG_EXIF_IMAGEWIDTH:
                // Use largest of height and width to deal with images that have been
                // rotated to portrait format.
                a = (int) _ConvertAnyFormat(ValuePtr, Format);
                if (_sExif.ExifImageWidth < a) _sExif.ExifImageWidth = a;
                break;

            case TAG_FOCALPLANEXRES:
                _sExif.FocalplaneXRes = _ConvertAnyFormat(ValuePtr, Format);
                break;

            case TAG_FOCALPLANEUNITS:
                switch((int)_ConvertAnyFormat(ValuePtr, Format)){
                    case 1: _sExif.FocalplaneUnits = 25.4; break; // inch
                    case 2: 
                        // According to the information I was using, 2 means meters.
                        // But looking at the Cannon powershot's files, inches is the only
                        // sensible value.
                        _sExif.FocalplaneUnits = 25.4;
                        break;

                    case 3: _sExif.FocalplaneUnits = 10;   break;  // centimeter
                    case 4: _sExif.FocalplaneUnits = 1;    break;  // millimeter
                    case 5: _sExif.FocalplaneUnits = .001; break;  // micrometer
                }
                break;

            case TAG_EXPOSURE_BIAS:
                psExif->dExposureBias = (float)_ConvertAnyFormat(ValuePtr, Format);
                break;

            case TAG_WHITEBALANCE:
                psExif->iWhitebalance = (int)_ConvertAnyFormat(ValuePtr, Format);
                break;

            case TAG_LIGHT_SOURCE:
                psExif->iLightSource = (int)_ConvertAnyFormat(ValuePtr, Format);
                break;

            case TAG_METERING_MODE:
                psExif->iMeteringMode = (int)_ConvertAnyFormat(ValuePtr, Format);
                break;

            case TAG_EXPOSURE_PROGRAM:
                psExif->iExposureProgram = (int)_ConvertAnyFormat(ValuePtr, Format);
                break;

            case TAG_EXPOSURE_INDEX:
                if (psExif->iISOequivalent == 0){
                    // Exposure index and ISO equivalent are often used interchangeably,
                    // so we will do the same in jhead.
                    // http://photography.about.com/library/glossary/bldef_ei.htm
                    psExif->iISOequivalent = (int)_ConvertAnyFormat(ValuePtr, Format);
                }
                break;

            case TAG_EXPOSURE_MODE:
                psExif->iExposureMode = (int)_ConvertAnyFormat(ValuePtr, Format);
                break;

            case TAG_ISO_EQUIVALENT:
                psExif->iISOequivalent = (int)_ConvertAnyFormat(ValuePtr, Format);
                if ( psExif->iISOequivalent < 50 ){
                    // Fixes strange encoding on some older digicams.
                    psExif->iISOequivalent *= 200;
                }
                break;

            case TAG_DIGITALZOOMRATIO:
                psExif->dDigitalZoomRatio = (float)_ConvertAnyFormat(ValuePtr, Format);
                break;

            case TAG_THUMBNAIL_OFFSET:
                ThumbnailOffset = (unsigned)_ConvertAnyFormat(ValuePtr, Format);
                _sExif.DirWithThumbnailPtrs = DirStart;
                break;

            case TAG_THUMBNAIL_LENGTH:
                ThumbnailSize = (unsigned)_ConvertAnyFormat(ValuePtr, Format);
                psExif->iThumbnailSizeOffset = ValuePtr-OffsetBase;
                break;

            case TAG_EXIF_OFFSET:
				if (psExif->arTablePrint) ARAddarg(&psExif->arTablePrint,"%s    Exif Dir:",IndentString);

            case TAG_INTEROP_OFFSET:
                if (Tag == TAG_INTEROP_OFFSET && psExif->arTablePrint) ARAddarg(&psExif->arTablePrint,"%s    Interop Dir:",IndentString);
                {
                    unsigned char * SubdirStart;
                    SubdirStart = OffsetBase + _Get32u(ValuePtr);
                    if (SubdirStart < OffsetBase || SubdirStart > OffsetBase+ExifLength){
                        ErrNonfatal("Illegal exif or interop ofset directory link",0,0);
                    }else{
                        _ProcessExifDir(psExif,SubdirStart, OffsetBase, ExifLength, NestingLevel+1);
                    }
                    continue;
                }
                break;

            case TAG_GPSINFO:
                if (psExif->arTablePrint) ARAddarg(&psExif->arTablePrint,"%s    GPS info dir:",IndentString);
                {
                    unsigned char * SubdirStart;
                    SubdirStart = OffsetBase + _Get32u(ValuePtr);
                    if (SubdirStart < OffsetBase || SubdirStart > OffsetBase+ExifLength){
                        ErrNonfatal("Illegal GPS directory link",0,0);
                    }else{
                        _processGpsInfo(psExif,SubdirStart, ByteCount, OffsetBase, ExifLength);
                    }
                    continue;
                }
                break;

            case TAG_FOCALLENGTH_35MM:
                // The focal length equivalent 35 mm is a 2.2 tag (defined as of April 2002)
                // if its present, use it to compute equivalent focal length instead of 
                // computing it from sensor geometry and actual focal length.
                psExif->uiFocalLength35mmEquiv = (unsigned)_ConvertAnyFormat(ValuePtr, Format);
                break;
        }
    }


    {
        // In addition to linking to subdirectories via exif tags, 
        // there's also a potential link to another directory at the end of each
        // directory.  this has got to be the result of a comitee!
        unsigned char * SubdirStart;
        unsigned Offset;

        if (DIR_ENTRY_ADDR(DirStart, NumDirEntries) + 4 <= OffsetBase+ExifLength){
            Offset = _Get32u(DirStart+2+12*NumDirEntries);
            if (Offset){
                SubdirStart = OffsetBase + Offset;
                if (SubdirStart > OffsetBase+ExifLength || SubdirStart < OffsetBase){
                    if (SubdirStart > OffsetBase && SubdirStart < OffsetBase+ExifLength+20){
                        // Jhead 1.3 or earlier would crop the whole directory!
                        // As Jhead produces this form of format incorrectness, 
                        // I'll just let it pass silently
                        if (psExif->arTablePrint) ARAddarg(&psExif->arTablePrint,"Thumbnail removed with Jhead 1.3 or earlier");
                    }else{
                        ErrNonfatal("Illegal subdirectory link",0,0);
                    }
                }else{
                    if (SubdirStart <= OffsetBase+ExifLength){
                        if (psExif->arTablePrint) ARAddarg(&psExif->arTablePrint,"%s    Continued directory ",IndentString);
                        _ProcessExifDir(psExif,SubdirStart, OffsetBase, ExifLength, NestingLevel+1);
                    }
                }
                if (Offset > psExif->uiLargestExifOffset) {
                    psExif->uiLargestExifOffset = Offset;
                }
            }
        }else{
            // The exif header ends before the last next directory pointer.
        }
    }

    if (ThumbnailOffset){
        psExif->cThumbnailAtEnd = FALSE;

        if (DumpExifMap){
            printf("Map: %05d-%05d: Thumbnail\n",ThumbnailOffset, ThumbnailOffset+ThumbnailSize);
        }

        if (ThumbnailOffset <= ExifLength){
            if (ThumbnailSize > ExifLength-ThumbnailOffset){
                // If thumbnail extends past exif header, only save the part that
                // actually exists.  Canon's EOS viewer utility will do this - the
                // thumbnail extracts ok with this hack.
                ThumbnailSize = ExifLength-ThumbnailOffset;
                if (psExif->arTablePrint) ARAddarg(&psExif->arTablePrint,"Thumbnail incorrectly placed in header");

            }
            // The thumbnail pointer appears to be valid.  Store it.
            psExif->uiThumbnailOffset = ThumbnailOffset;
            psExif->uiThumbnailSize = ThumbnailSize;

            if (psExif->arTablePrint){
                ARAddarg(&psExif->arTablePrint,"Thumbnail size: %d bytes",ThumbnailSize);
            }
        }
    }
}

//--------------------------------------------------------------------------
// _processExif();
// iProcess a EXIF marker
// Describes all the drivel that most digital cameras include...
//--------------------------------------------------------------------------
static void _processExif(S_EXIF * psExif, unsigned char * ExifSection, unsigned int length)
{
    int FirstOffset;

    _sExif.FocalplaneXRes = 0;
    _sExif.FocalplaneUnits = 0;
    _sExif.ExifImageWidth = 0;
    _sExif.OrientationPtr = NULL;


    if (psExif->arTablePrint){
        ARAddarg(&psExif->arTablePrint,"Exif header %d bytes long",length);
    }

    {   // Check the EXIF header component
        static BYTE ExifHeader[] = "Exif\0\0";
        if (memcmp(ExifSection+2, ExifHeader,6)){
            ErrNonfatal("Incorrect Exif header",0,0);
            return;
        }
    }

    if (memcmp(ExifSection+8,"II",2) == 0){
        if (psExif->arTablePrint) ARAddarg(&psExif->arTablePrint,"Exif section in Intel order");
        MotorolaOrder = 0;
    }else{
        if (memcmp(ExifSection+8,"MM",2) == 0){
            if (psExif->arTablePrint) ARAddarg(&psExif->arTablePrint,"Exif section in Motorola order");
            MotorolaOrder = 1;
        }else{
            ErrNonfatal("Invalid Exif alignment marker.",0,0);
            return;
        }
    }

    // Check the next value for correctness.
    if (_Get16u(ExifSection+10) != 0x2a){
        ErrNonfatal("Invalid Exif start (1)",0,0);
        return;
    }

    FirstOffset = _Get32u(ExifSection+12);
    if (FirstOffset < 8 || FirstOffset > 16){
        // I used to ensure this was set to 8 (website I used indicated its 8)
        // but PENTAX Optio 230 has it set differently, and uses it as offset. (Sept 11 2002)
        ErrNonfatal("Suspicious offset of first IFD value",0,0);
    }

    _sExif.DirWithThumbnailPtrs = NULL;

    // First directory starts 16 bytes in.  All offset are relative to 8 bytes in.
    _ProcessExifDir(psExif,ExifSection+8+FirstOffset, ExifSection+8, length-8, 0);

    psExif->cThumbnailAtEnd = psExif->uiThumbnailOffset >= psExif->uiLargestExifOffset ? TRUE : FALSE;

    if (DumpExifMap){
        unsigned a,b;
        printf("Map: %05d- End of exif\n",length-8);
        for (a=0;a<length-8;a+= 10){
            printf("Map: %05d ",a);
            for (b=0;b<10;b++) printf(" %02x",*(ExifSection+8+a+b));
            printf("\n");
        }
    }


    // Compute the CCD width, in millimeters.
    if (_sExif.FocalplaneXRes != 0){
        // Note: With some cameras, its not possible to compute this correctly because
        // they don't adjust the indicated focal plane resolution units when using less
        // than maximum resolution, so the dCCDWidth value comes out too small.  Nothing
        // that Jhad can do about it - its a camera problem.
        psExif->dCCDWidth = (float)(_sExif.ExifImageWidth * _sExif.FocalplaneUnits / _sExif.FocalplaneXRes);

        if (psExif->dFocalLength && psExif->uiFocalLength35mmEquiv == 0){
            // Compute 35 mm equivalent focal length based on sensor geometry if we haven't
            // already got it explicitly from a tag.
            psExif->uiFocalLength35mmEquiv = (int)(psExif->dFocalLength/psExif->dCCDWidth*36 + 0.5);
        }
    }
}

//--------------------------------------------------------------------------
// Cler the rotation tag in the exif header to 1.
// Note: The thumbnail is NOT rotated.  That would be really hard, especially
// stuffing it back into the exif header, because its size might change.
//--------------------------------------------------------------------------
/*
const char * ClearOrientation(void)
{
    if (OrientationPtr == NULL) return NULL;
    
    switch(OrientationNumFormat){
        case FMT_SBYTE:
        case FMT_BYTE:      
            *(BYTE *)OrientationPtr = 1;
            break;

        case FMT_USHORT:    
            _Put16u(OrientationPtr, 1);                
            break;

        case FMT_ULONG:     
        case FMT_SLONG:     
            memset(OrientationPtr, 0, 4);
            // Can't be bothered to write  generic Put32 if I only use it once.
            if (MotorolaOrder){
                ((BYTE *)OrientationPtr)[3] = 1;
            }else{
                ((BYTE *)OrientationPtr)[0] = 1;
            }
            break;

        default:
            return NULL;
    }

    return arOrientTab[psExif->Orientation];
}


//--------------------------------------------------------------------------
// Remove thumbnail out of the exif image.
//--------------------------------------------------------------------------
int RemoveThumbnail(unsigned char * ExifSection)
{
    if (!_sExif.DirWithThumbnailPtrs || 
        psExif->ThumbnailOffset == 0 || 
        psExif->ThumbnailSize == 0){
        // No thumbnail, or already deleted it.
        return 0;
    }
    if (psExif->ThumbnailAtEnd == FALSE){
        ErrNonfatal("Thumbnail is not at end of header, can't chop it off", 0, 0);
        return 0;
    }

    {
        int de;
        int NumDirEntries;
        NumDirEntries = _Get16u(_sExif.DirWithThumbnailPtrs);

        for (de=0;de<NumDirEntries;de++){
            int Tag;
            unsigned char * DirEntry;
            DirEntry = DIR_ENTRY_ADDR(_sExif.DirWithThumbnailPtrs, de);
            Tag = _Get16u(DirEntry);
            if (Tag == TAG_THUMBNAIL_LENGTH){
                // Set length to zero.
                if (_Get16u(DirEntry+2) != FMT_ULONG){
                    // non standard format encoding.  Can't do it.
                    ErrNonfatal("Can't remove thumbnail", 0, 0);
                    return 0;
                }
                _Put32u(DirEntry+8, 0);
            }                    
        }
    }

    // This is how far the non thumbnail data went.
    return psExif->ThumbnailOffset+8;

}

*/
//--------------------------------------------------------------------------
// Convert exif time to Unix time structure
//--------------------------------------------------------------------------
int Exif2tm(struct tm * timeptr, char * ExifTime)
{
    int a;

    timeptr->tm_wday = -1;

    // Check for format: YYYY:MM:DD HH:MM:SS format.
    // Date and time normally separated by a space, but also seen a ':' there, so
    // skip the middle space with '%*c' so it can be any character.
    a = sscanf(ExifTime, "%d%*c%d%*c%d%*c%d:%d:%d",
            &timeptr->tm_year, &timeptr->tm_mon, &timeptr->tm_mday,
            &timeptr->tm_hour, &timeptr->tm_min, &timeptr->tm_sec);


    if (a == 6){
        timeptr->tm_isdst = -1;  
        timeptr->tm_mon -= 1;      // Adjust for unix zero-based months 
        timeptr->tm_year -= 1900;  // Adjust for year starting at 1900 
        return TRUE; // worked. 
    }

    return FALSE; // Wasn't in Exif date format.
}
/*
//--------------------------------------------------------------------------
// ShowImageInfo()
//
// Show the collected image info, displaying camera F-stop and shutter speed
// in a consistent and legible fashion.
//--------------------------------------------------------------------------
void ShowImageInfo(int ShowFileInfo)
{
    if (ShowFileInfo){
        printf("File name    : %s\n",psExif->FileName);
        printf("File size    : %d bytes\n",psExif->FileSize);

        {
            char Temp[20];
            struct tm ts;
            ts = *localtime(&psExif->FileDateTime);
            strftime(Temp, 20, "%Y:%m:%d %H:%M:%S", &ts);
            printf("File date    : %s\n",Temp);
        }
    }

    if (psExif->szCameraMake[0]){
        printf("Camera make  : %s\n",psExif->szCameraMake);
        printf("Camera model : %s\n",psExif->CameraModel);
    }
    if (psExif->DateTime[0]){
        printf("Date/Time    : %s\n",psExif->DateTime);
    }
    printf("Resolution   : %d x %d\n",psExif->iWidth, psExif->iHeight);

    if (psExif->Orientation > 1){
        // Only print orientation if one was supplied, and if its not 1 (normal orientation)
        printf("Orientation  : %s\n", arOrientTab[psExif->Orientation]);
    }

    if (psExif->iIsColor == 0){
        printf("Color/bw     : Black and white\n");
    }

    if (psExif->FlashUsed >= 0){
        if (psExif->FlashUsed & 1){    
            printf("Flash used   : Yes");
            switch (psExif->FlashUsed){
	            case 0x5: printf(" (Strobe light not detected)"); break;
	            case 0x7: printf(" (Strobe light detected) "); break;
	            case 0x9: printf(" (manual)"); break;
	            case 0xd: printf(" (manual, return light not detected)"); break;
	            case 0xf: printf(" (manual, return light  detected)"); break;
	            case 0x19:printf(" (auto)"); break;
	            case 0x1d:printf(" (auto, return light not detected)"); break;
	            case 0x1f:printf(" (auto, return light detected)"); break;
	            case 0x41:printf(" (red eye reduction mode)"); break;
	            case 0x45:printf(" (red eye reduction mode return light not detected)"); break;
	            case 0x47:printf(" (red eye reduction mode return light  detected)"); break;
	            case 0x49:printf(" (manual, red eye reduction mode)"); break;
	            case 0x4d:printf(" (manual, red eye reduction mode, return light not detected)"); break;
	            case 0x4f:printf(" (red eye reduction mode, return light detected)"); break;
	            case 0x59:printf(" (auto, red eye reduction mode)"); break;
	            case 0x5d:printf(" (auto, red eye reduction mode, return light not detected)"); break;
	            case 0x5f:printf(" (auto, red eye reduction mode, return light detected)"); break;
            }
        }else{
            printf("Flash used   : No");
            switch (psExif->FlashUsed){
	            case 0x18:printf(" (auto)"); break;
            }
        }
        printf("\n");
    }


    if (psExif->dFocalLength){
        printf("Focal length : %4.1fmm",(double)psExif->dFocalLength);
        if (psExif->uiFocalLength35mmEquiv){
            printf("  (35mm equivalent: %dmm)", psExif->uiFocalLength35mmEquiv);
        }
        printf("\n");
    }

    if (psExif->DigitalZoomRatio > 1){
        // Digital zoom used.  Shame on you!
        printf("Digital Zoom : %1.3fx\n", (double)psExif->DigitalZoomRatio);
    }

    if (psExif->dCCDWidth){
        printf("CCD width    : %4.2fmm\n",(double)psExif->dCCDWidth);
    }

    if (psExif->ExposureTime){
        if (psExif->ExposureTime < 0.010){
            printf("Exposure time: %6.4f s ",(double)psExif->ExposureTime);
        }else{
            printf("Exposure time: %5.3f s ",(double)psExif->ExposureTime);
        }
        if (psExif->ExposureTime <= 0.5){
            printf(" (1/%d)",(int)(0.5 + 1/psExif->ExposureTime));
        }
        printf("\n");
    }
    if (psExif->ApertureFNumber){
        printf("Aperture     : f/%3.1f\n",(double)psExif->ApertureFNumber);
    }
    if (psExif->Distance){
        if (psExif->Distance < 0){
            printf("Focus dist.  : Infinite\n");
        }else{
            printf("Focus dist.  : %4.2fm\n",(double)psExif->Distance);
        }
    }

    if (psExif->iISOequivalent){
        printf("ISO equiv.   : %2d\n",(int)psExif->iISOequivalent);
    }

    if (psExif->ExposureBias){
        // If exposure bias was specified, but set to zero, presumably its no bias at all,
        // so only show it if its nonzero.
        printf("Exposure bias: %4.2f\n",(double)psExif->ExposureBias);
    }
        
    switch(psExif->Whitebalance) {
        case 1:
            printf("Whitebalance : Manual\n");
            break;
        case 0:
            printf("Whitebalance : Auto\n");
            break;
    }

    //Quercus: 17-1-2004 Added iLightSource, some cams return this, whitebalance or both
    switch(psExif->iLightSource) {
        case 1:
            printf("Light Source : Daylight\n");
            break;
        case 2:
            printf("Light Source : Fluorescent\n");
            break;
        case 3:
            printf("Light Source : Incandescent\n");
            break;
        case 4:
            printf("Light Source : Flash\n");
            break;
        case 9:
            printf("Light Source : Fine weather\n");
            break;
        case 11:
            printf("Light Source : Shade\n");
            break;
        default:; //Quercus: 17-1-2004 There are many more modes for this, check Exif2.2 specs
            // If it just says 'unknown' or we don't know it, then
            // don't bother showing it - it doesn't add any useful information.
    }

    if (psExif->MeteringMode){ // 05-jan-2001 vcs
        switch(psExif->MeteringMode) {
        case 2:
            printf("Metering Mode: center weight\n");
            break;
        case 3:
            printf("Metering Mode: spot\n");
            break;
        case 5:
            printf("Metering Mode: matrix\n");
            break;
        }
    }

    if (psExif->ExposureProgram){ // 05-jan-2001 vcs
        switch(psExif->ExposureProgram) {
        case 1:
            printf("Exposure     : Manual\n");
            break;
        case 2:
            printf("Exposure     : program (auto)\n");
            break;
        case 3:
            printf("Exposure     : aperture priority (semi-auto)\n");
            break;
        case 4:
            printf("Exposure     : shutter priority (semi-auto)\n");
            break;
        case 5:
            printf("Exposure     : Creative Program (based towards depth of field)\n"); 
            break;
        case 6:
            printf("Exposure     : Action program (based towards fast shutter speed)\n");
            break;
        case 7:
            printf("Exposure     : Portrait Mode\n");
            break;
        case 8:
            printf("Exposure     : LandscapeMode \n");
            break;
        default:
            break;
        }
    }
    switch(psExif->ExposureMode){
        case 0: // Automatic (not worth cluttering up output for)
            break;
        case 1: printf("Exposure Mode: Manual\n");
        case 2: printf("Exposure Mode: Auto bracketing\n");
    }


    if (psExif->iProcess != M_SOF0){
        // don't show it if its the plain old boring 'baseline' process, but do
        // show it if its something else, like 'progressive' (used on web sometimes)
        int a;
        for (a=0;;a++){
            if (a >= PROCESS_TABLE_SIZE){
                // ran off the end of the table.
                printf("Jpeg process : Unknown\n");
                break;
            }
            if (ProcessTable[a].Tag == psExif->iProcess){
                printf("Jpeg process : %s\n",ProcessTable[a].Desc);
                break;
            }
        }
    }

    if (psExif->iGpsInfoPresent){
        printf("GPS Latitude : %s\n",psExif->szGpsLat);
        printf("GPS Longitude: %s\n",psExif->szGpsLong);
        if (psExif->szGpsAlt[0]) printf("GPS Altitude : %s\n",psExif->szGpsAlt);
    }

    // Print the comment. Print 'Comment:' for each new line of comment.
    if (psExif->szComments[0]){
        int a,c;
        printf("Comment      : ");
        for (a=0;a<MAX_COMMENT;a++){
            c = psExif->szComments[a];
            if (c == '\0') break;
            if (c == '\n'){
                // Do not start a new line if the string ends with a carriage return.
                if (psExif->szComments[a+1] != '\0'){
                    printf("\nComment      : ");
                }else{
                    printf("\n");
                }
            }else{
                putchar(c);
            }
        }
        printf("\n");
    }
    printf("\n");
}


//--------------------------------------------------------------------------
// Summarize highlights of image info on one line (suitable for grep-ing)
//--------------------------------------------------------------------------
void ShowConciseImageInfo(void)
{
    printf("\"%s\"",psExif->FileName);

    printf(" %dx%d",psExif->iWidth, psExif->iHeight);

    if (psExif->ExposureTime){
        if (psExif->ExposureTime <= 0.5){
            printf(" (1/%d)",(int)(0.5 + 1/psExif->ExposureTime));
        }else{
            printf(" (%1.1f)",psExif->ExposureTime);
        }
    }

    if (psExif->ApertureFNumber){
        printf(" f/%3.1f",(double)psExif->ApertureFNumber);
    }

    if (psExif->uiFocalLength35mmEquiv){
        printf(" f(35)=%dmm",psExif->uiFocalLength35mmEquiv);
    }

    if (psExif->FlashUsed >= 0 && psExif->FlashUsed & 1){
        printf(" (flash)");
    }

    if (psExif->iIsColor == 0){
        printf(" (bw)");
    }

    printf("\n");
}


*/


void ErrFatal(char * msg)
{
    win_infoarg("ErrFatal : %s\n", msg);
    if (CurrentFile) win_infoarg("in file '%s'\n",CurrentFile);
    exit(EXIT_FAILURE);
} 



void ErrNonfatal(char * msg, int a1, int a2)
{
    if (SupressNonFatalErrors) return;

    win_infoarg("Nonfatal Error : ");
    if (CurrentFile) win_infoarg("'%s' ",CurrentFile);
    win_infoarg( msg, a1, a2);
    win_infoarg("\n");
} 



//--------------------------------------------------------------------------
// iProcess exif format directory, as used by Cannon maker note
//--------------------------------------------------------------------------
void ProcessCanonMakerNoteDir(S_EXIF * psExif,unsigned char * DirStart, unsigned char * OffsetBase, unsigned ExifLength)
{
    int de;
    int a;
    int NumDirEntries;
	CHAR szValue[1024];

    NumDirEntries = _Get16u(DirStart);
    #define DIR_ENTRY_ADDR(Start, Entry) (Start+2+12*(Entry))

    {
        unsigned char * DirEnd;
        DirEnd = DIR_ENTRY_ADDR(DirStart, NumDirEntries);
        if (DirEnd > (OffsetBase+ExifLength)){
            ErrNonfatal("Illegally sized directory",0,0);
            return;
        }

        if (DumpExifMap){
            printf("Map: %05d-%05d: Directory (makernote)\n",DirStart-OffsetBase, DirEnd-OffsetBase);
        }
    }

    if (psExif->arTablePrint){
        ARAddarg(&psExif->arTablePrint,"(dir has %d entries)",NumDirEntries);
    }

    for (de=0;de<NumDirEntries;de++){
        int Tag, Format, Components;
        unsigned char * ValuePtr;
        int ByteCount;
        unsigned char * DirEntry;
        DirEntry = DIR_ENTRY_ADDR(DirStart, de);

        Tag = _Get16u(DirEntry);
        Format = _Get16u(DirEntry+2);
        Components = _Get32u(DirEntry+4);

        if ((Format-1) >= NUM_FORMATS) {
            // (-1) catches illegal zero case as unsigned underflows to positive large.
            ErrNonfatal("Illegal number format %d for tag %04x", Format, Tag);
            continue;
        }

        if ((unsigned)Components > 0x10000){
            ErrNonfatal("Illegal number of components %d for tag %04x", Components, Tag);
            continue;
        }

        ByteCount = Components * BytesPerFormat[Format];

        if (ByteCount > 4){
            unsigned OffsetVal;
            OffsetVal = _Get32u(DirEntry+8);
            // If its bigger than 4 bytes, the dir entry contains an offset.
            if (OffsetVal+ByteCount > ExifLength){
                // Bogus pointer offset and / or bytecount value
                ErrNonfatal("Illegal value pointer for tag %04x", Tag,0);
                continue;
            }
            ValuePtr = OffsetBase+OffsetVal;

            if (DumpExifMap){
                printf("Map: %05d-%05d:   Data for makernote tag %04x\n",OffsetVal, OffsetVal+ByteCount, Tag);
            }
        }else{
            // 4 bytes or less and value is in the dir entry itself
            ValuePtr = DirEntry+8;
        }

		*szValue=0;

        // Show tag value.
        switch(Format){

            case FMT_UNDEFINED:
                // Undefined is typically an ascii string.

            case FMT_STRING:

                // String arrays printed without function call (different from int arrays)
                if (psExif->arTablePrint){

					a=ByteCount; if (a>(sizeof(szValue)-1)) a=sizeof(szValue);
					memcpy(szValue,ValuePtr,a); szValue[a]=0;
/*
                    printf("\"");
                    for (a=0;a<ByteCount;a++){
                        int ZeroSkipped = 0;
                        if (ValuePtr[a] >= 32){
                            if (ZeroSkipped){
                                printf("?");
                                ZeroSkipped = 0;
                            }
                            putchar(ValuePtr[a]);
                        }else{
                            if (ValuePtr[a] == 0){
                                ZeroSkipped = 1;
                            }
                        }
                    }
                    printf("\"\n");
					*/
                }
                break;

            default:
                if (psExif->arTablePrint){
                    _getFormatNumber(szValue,ValuePtr, Format, ByteCount);
//                    printf("\n");
                }
        }

        if (psExif->arTablePrint){
            // Show tag name
            ARAddarg(&psExif->arTablePrint,"            Canon maker tag %04x Value = %s", Tag,szValue);
        }

        if (Tag == 1 && ByteCount >= 17*sizeof(unsigned short)){
            int IsoCode = _Get16u(ValuePtr + 16*sizeof(unsigned short));
            if (IsoCode >= 16 && IsoCode <= 24){
                psExif->iISOequivalent = 50 << (IsoCode-16);
            } 
        }

        if (Tag == 4 && ByteCount >= 8*sizeof(unsigned short)){
            int WhiteBalance = _Get16u(ValuePtr + 7*sizeof(unsigned short));
            switch(WhiteBalance){
                // 0=Auto, 6=Custom
                case 1: psExif->iLightSource = 1; break; // Sunny
                case 2: psExif->iLightSource = 1; break; // Cloudy
                case 3: psExif->iLightSource = 3; break; // Thungsten
                case 4: psExif->iLightSource = 2; break; // Fourescent
                case 5: psExif->iLightSource = 4; break; // Flash
            }
        }
    }
}

//--------------------------------------------------------------------------
// Show generic maker note - just hex bytes.
//--------------------------------------------------------------------------
void _ShowMakerNoteGeneric(S_EXIF *psExif,unsigned char * ValuePtr, int ByteCount)
{
    int a;
    for (a=0;a<ByteCount;a++){
        if (a > 10){
            printf("...");
            break;
        }
        printf(" %02x",ValuePtr[a]);
    }
    printf(" (%d bytes)", ByteCount);
    printf("\n");

}

//--------------------------------------------------------------------------
// iProcess maker note - to the limited extent that its supported.
//--------------------------------------------------------------------------
void ProcessMakerNote(S_EXIF * psExif,unsigned char * ValuePtr, int ByteCount, unsigned char * OffsetBase, unsigned ExifLength)
{
    if (strstr(psExif->szCameraMake, "Canon")){
        ProcessCanonMakerNoteDir(psExif,ValuePtr, OffsetBase, ExifLength);
    }else{
        if (psExif->arTablePrint){
            _ShowMakerNoteGeneric(psExif,ValuePtr, ByteCount);
        }
    }
}
/*
//--------------------------------------------------------------------------
// Parsing of GPS info from exif header.
//
// Matthias Wandel,  Dec 1999 - Dec 2002 
//--------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>

#ifndef _WIN32
    #include <limits.h>
#endif
#include "jhead.h"
*/



static const char * GpsTags[MAX_GPS_TAG+1]= {
    "VersionID       ",//0x00  
    "LatitudeRef     ",//0x01  
    "Latitude        ",//0x02  
    "LongitudeRef    ",//0x03  
    "Longitude       ",//0x04  
    "AltitudeRef     ",//0x05  
    "Altitude        ",//0x06  
    "TimeStamp       ",//0x07  
    "Satellites      ",//0x08  
    "Status          ",//0x09  
    "MeasureMode     ",//0x0A  
    "DOP             ",//0x0B  
    "SpeedRef        ",//0x0C  
    "Speed           ",//0x0D  
    "TrackRef        ",//0x0E  
    "Track           ",//0x0F  
    "ImgDirectionRef ",//0x10  
    "ImgDirection    ",//0x11  
    "MapDatum        ",//0x12  
    "DestLatitudeRef ",//0x13  
    "DestLatitude    ",//0x14  
    "DestLongitudeRef",//0x15  
    "DestLongitude   ",//0x16  
    "DestBearingRef  ",//0x17  
    "DestBearing     ",//0x18  
    "DestDistanceRef ",//0x19  
    "DestDistance    ",//0x1A  
    "ProcessingMethod",//0x1B  
    "AreaInformation ",//0x1C  
    "DateStamp       ",//0x1D  
    "Differential    ",//0x1E
};

//--------------------------------------------------------------------------
// iProcess GPS info directory
//--------------------------------------------------------------------------
static void _processGpsInfo(S_EXIF *psExif, unsigned char * DirStart, int ByteCountUnused, unsigned char * OffsetBase, unsigned ExifLength)
{
    int de;
    unsigned a;
    int NumDirEntries;

    NumDirEntries = _Get16u(DirStart);
    #define DIR_ENTRY_ADDR(Start, Entry) (Start+2+12*(Entry))

    if (psExif->arTablePrint){
        ARAddarg(&psExif->arTablePrint,"(dir has %d entries)",NumDirEntries);
    }

    psExif->iGpsInfoPresent = TRUE;
    strcpy(psExif->szGpsLat, "? ?");
    strcpy(psExif->szGpsLong, "? ?");
    psExif->szGpsAlt[0] = 0; 

    for (de=0;de<NumDirEntries;de++){
        unsigned Tag, Format, Components;
        unsigned char * ValuePtr;
        int ComponentSize;
        unsigned ByteCount;
        unsigned char * DirEntry;
        DirEntry = DIR_ENTRY_ADDR(DirStart, de);

        Tag = _Get16u(DirEntry);
        Format = _Get16u(DirEntry+2);
        Components = _Get32u(DirEntry+4);

        if ((Format-1) >= NUM_FORMATS) {
            // (-1) catches illegal zero case as unsigned underflows to positive large.
            ErrNonfatal("Illegal number format %d for tag %04x", Format, Tag);
            continue;
        }

        ComponentSize = BytesPerFormat[Format];
        ByteCount = Components * ComponentSize;

        if (ByteCount > 4){
            unsigned OffsetVal;
            OffsetVal = _Get32u(DirEntry+8);
            // If its bigger than 4 bytes, the dir entry contains an offset.
            if (OffsetVal+ByteCount > ExifLength){
                // Bogus pointer offset and / or bytecount value
                ErrNonfatal("Illegal value pointer for tag %04x", Tag,0);
                continue;
            }
            ValuePtr = OffsetBase+OffsetVal;
        }else{
            // 4 bytes or less and value is in the dir entry itself
            ValuePtr = DirEntry+8;
        }

        switch(Tag){
            char FmtString[21];
            char TempString[50];
            double Values[3];

            case TAG_GPS_LAT_REF:
                psExif->szGpsLat[0] = ValuePtr[0];
                break;

            case TAG_GPS_LONG_REF:
                psExif->szGpsLong[0] = ValuePtr[0];
                break;

            case TAG_GPS_LAT:
            case TAG_GPS_LONG:
                if (Format != FMT_URATIONAL){
                    ErrNonfatal("Inappropriate format (%d) for GPS coordinates!", Format, 0);
                }
                strcpy(FmtString, "%0.0fd %0.0fm %0.0fs");

                for (a=0;a<3;a++){
                    int den, digits;

                    den = _Get32s(ValuePtr+4+a*ComponentSize);
                    digits = 0;
                    while (den > 1){
                        den = den / 10;
                        digits += 1;
                    }
                    FmtString[1+a*7] = (char)('2'+digits+(digits ? 1 : 0));
                    FmtString[3+a*7] = (char)('0'+digits);

                    Values[a] = _ConvertAnyFormat(ValuePtr+a*ComponentSize, Format);
                }
                sprintf(TempString, FmtString, Values[0], Values[1], Values[2]);

                if (Tag == TAG_GPS_LAT){
                    strncpy(psExif->szGpsLat+2, TempString, 29);
                }else{
                    strncpy(psExif->szGpsLong+2, TempString, 29);
                }
                break;

            case TAG_GPS_ALT_REF:
                psExif->szGpsAlt[0] = (char)(ValuePtr[0] ? '-' : ' ');
                break;

            case TAG_GPS_ALT:
                sprintf(psExif->szGpsAlt + 1, "%dm", _Get32s(ValuePtr));
                break;
        }

        if (psExif->arTablePrint){
			CHAR szCode[1024];
			CHAR szName[1024];
			CHAR szValue[1024];

            // Show tag value.
            if (Tag < MAX_GPS_TAG){
                sprintf(szName,"        GPS%s", GpsTags[Tag]);
				sprintf(szCode,"GPS|%04x|%s",Tag,GpsTags[Tag]);
            }else{
                // Show unknown tag
                sprintf(szName,"        Illegal GPS tag %04x", Tag);
				sprintf(szCode,"GPS|%04x|?",Tag,GpsTags[Tag]);
            }
			*szValue=0;
            switch(Format){
                case FMT_UNDEFINED:
                    // Undefined is typically an ascii string.

                case FMT_STRING:

					a=ByteCount; if (a>(sizeof(szValue)-1)) a=sizeof(szValue);
					memcpy(szValue,ValuePtr,a); szValue[a]=0;
/*
                    // String arrays printed without function call (different from int arrays)
                    {
                        printf("\"");
                        for (a=0;a<ByteCount;a++){
                            int ZeroSkipped = 0;
                            if (ValuePtr[a] >= 32){
                                if (ZeroSkipped){
                                    printf("?");
                                    ZeroSkipped = 0;
                                }
                                putchar(ValuePtr[a]);
                            }else{
                                if (ValuePtr[a] == 0){
                                    ZeroSkipped = 1;
                                }
                            }
                        }
                        printf("\"\n");
                    }
					*/
                    break;

                default:
                    // Handle arrays of numbers later (will there ever be?)
                    for (a=0;;){
                        _getFormatNumber(szValue+strlen(szValue),ValuePtr+a*ComponentSize, Format, ByteCount);
                        if (++a >= Components) break;
                        strcat(szValue,", ");
                    }
                    //printf("\n");
            }
			ARAddarg(&psExif->arTablePrint,"%s = %s",szName,szValue);
			strTrim(szCode);
			ARAddarg(&psExif->arTable,"%s|%s",szCode,szValue);

        }
    }
}

   

/*
//--------------------------------------------------------------------------
// Program to pull the information out of various types of EXIF digital 
// camera files and show it in a reasonably consistent way
//
// This module handles basic Jpeg file handling
//
// Matthias Wandel,  Dec 1999 - Dec 2002 
//--------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>

#ifdef _WIN32
    #include <process.h>
    #include <io.h>
    #include <sys/utime.h>
#else
    #include <utime.h>
    #include <sys/types.h>
    #include <unistd.h>
    #include <errno.h>
    #include <limits.h>
#endif

#include "jhead.h"

// Storage for simplified info extracted from file.
ImageInfo_t ImageInfo;



*/

#define PSEUDO_IMAGE_MARKER 0x123; // Extra value.
//--------------------------------------------------------------------------
// Get 16 bits motorola order (always) for jpeg header stuff.
//--------------------------------------------------------------------------
static int Get16m(const void * Short)
{
    return (((BYTE *)Short)[0] << 8) | ((BYTE *)Short)[1];
}


//--------------------------------------------------------------------------
// iProcess a COM marker.
// We want to print out the marker contents as legible text;
// we must guard against random junk and varying newline representations.
//--------------------------------------------------------------------------
static void _processCom(S_EXIF *psExif,const BYTE * Data, int length)
{
    int ch;
    char Comment[MAX_COMMENT+1];
    int nch;
    int a;

    nch = 0;

    if (length > MAX_COMMENT) length = MAX_COMMENT; // Truncate if it won't fit in our structure.

    for (a=2;a<length;a++){
        ch = Data[a];

        if (ch == '\r' && Data[a+1] == '\n') continue; // Remove cr followed by lf.

        if (ch >= 32 || ch == '\n' || ch == '\t'){
            Comment[nch++] = (char)ch;
        }else{
            Comment[nch++] = '?';
        }
    }

    Comment[nch] = '\0'; // Null terminate

    if (psExif->arTablePrint){
        ARAddarg(&psExif->arTablePrint,"COM marker comment: %s",Comment);
    }

    strcpy(psExif->szComments,Comment);
}

 
//--------------------------------------------------------------------------
// iProcess a SOFn marker.  This is useful for the image dimensions
//--------------------------------------------------------------------------
static void _process_SOFn(S_EXIF *psExif,const BYTE * Data, int marker)
{
    int data_precision, num_components;

    data_precision = Data[2];
    psExif->iHeight = Get16m(Data+3);
    psExif->iWidth = Get16m(Data+5);
    num_components = Data[7];

    if (num_components == 3){
        psExif->iIsColor = 1;
    }else{
        psExif->iIsColor = 0;
    }

    psExif->iProcess = marker;

    if (psExif->arTablePrint){
        ARAddarg(&psExif->arTablePrint,"JPEG image is %uw * %uh, %d color components, %d bits per sample",psExif->iWidth, psExif->iHeight, num_components, data_precision);
    }
}




//--------------------------------------------------------------------------
// Parse the marker stream until SOS or EOI is seen;
//--------------------------------------------------------------------------
int _ReadJpegSections(FILE * infile, EN_EXIF_READMODE ReadMode,S_EXIF * psExif)
{
    int a;
    int HaveCom = FALSE;

//	memset(psExif,0,sizeof(S_EXIF));
    a = fgetc(infile);

    if (a != 0xff || fgetc(infile) != M_SOI){
        return FALSE;
    }
    for(;;){

        int itemlen;
        int marker = 0;
        int ll,lh, got;
        BYTE * Data;

        if (SectionsRead >= MAX_SECTIONS){
            ErrFatal("Too many sections in jpg file");
        }

        for (a=0;a<7;a++){
            marker = fgetc(infile);
            if (marker != 0xff) break;

            if (a >= 6){
                printf("too many padding bytes\n");
                return FALSE;
            }
        }

        if (marker == 0xff){
            // 0xff is legal padding, but if we get that many, something's wrong.
            ErrFatal("too many padding bytes!");
        }

        Sections[SectionsRead].Type = marker;
  
        // Read the length of the section.
        lh = fgetc(infile);
        ll = fgetc(infile);

        itemlen = (lh << 8) | ll;

        if (itemlen < 2){
            ErrFatal("invalid marker");
        }

        Sections[SectionsRead].Size = itemlen;

        Data = (BYTE *) ehAlloc(itemlen);
        if (Data == NULL){
            ErrFatal("Could not allocate memory");
        }
        Sections[SectionsRead].pbData = Data;

        // Store first two pre-read bytes.
        Data[0] = (BYTE)lh;
        Data[1] = (BYTE)ll;

        got = fread(Data+2, 1, itemlen-2, infile); // Read the whole section.
        if (got != itemlen-2){
            ErrFatal("Premature end of file?");
        }
        SectionsRead += 1;

        switch(marker){

            case M_SOS:   // stop before hitting compressed data 
                // If reading entire image is requested, read the rest of the data.
                if (ReadMode & READ_IMAGE){
                    int cp, ep, size;
                    // Determine how much file is left.
                    cp = ftell(infile);
                    fseek(infile, 0, SEEK_END);
                    ep = ftell(infile);
                    fseek(infile, cp, SEEK_SET);

                    size = ep-cp;
                    Data = (BYTE *) ehAlloc(size);
                    if (Data == NULL){
                        ErrFatal("could not allocate data for entire image");
                    }

                    got = fread(Data, 1, size, infile);
                    if (got != size){
                        ErrFatal("could not read the rest of the image");
                    }

                    Sections[SectionsRead].pbData = Data;
                    Sections[SectionsRead].Size = size;
                    Sections[SectionsRead].Type = PSEUDO_IMAGE_MARKER;
                    SectionsRead ++;
                    HaveAll = 1;
                }
                return TRUE;

            case M_EOI:   // in case it's a tables-only JPEG stream
                printf("No image in jpeg!\n");
                return FALSE;

            case M_COM: // Comment section
                if (HaveCom || ((ReadMode & READ_EXIF) == 0)){
                    // Discard this section.
                    ehFree(Sections[--SectionsRead].pbData);
                }else{
                    _processCom(psExif,Data, itemlen);
                    HaveCom = TRUE;
                }
                break;

            case M_JFIF:
                // Regular jpegs always have this tag, exif images have the exif
                // marker instead, althogh ACDsee will write images with both markers.
                // this program will re-create this marker on absence of exif marker.
                // hence no need to keep the copy from the file.
                ehFree(Sections[--SectionsRead].pbData);
                break;

            case M_EXIF:
                // Seen files from some 'U-lead' software with Vivitar scanner
                // that uses marker 31 for non exif stuff.  Thus make sure 
                // it says 'Exif' in the section before treating it as exif.
                if ((ReadMode & READ_EXIF) && memcmp(Data+2, "Exif", 4) == 0){
                    _processExif(psExif,Data, itemlen);
                }else{
                    // Discard this section.
                    ehFree(Sections[--SectionsRead].pbData);
                }
                break;

            case M_SOF0: 
            case M_SOF1: 
            case M_SOF2: 
            case M_SOF3: 
            case M_SOF5: 
            case M_SOF6: 
            case M_SOF7: 
            case M_SOF9: 
            case M_SOF10:
            case M_SOF11:
            case M_SOF13:
            case M_SOF14:
            case M_SOF15:
                _process_SOFn(psExif,Data, marker);
                break;
            default:
                // Skip any other sections.
                if (psExif->arTablePrint){
                    ARAddarg(&psExif->arTablePrint,"Jpeg section marker 0x%02x size %d",marker, itemlen);
                }
                break;
        }
    }
    return TRUE;
}

//--------------------------------------------------------------------------
// Discard read data.
//--------------------------------------------------------------------------
void _DiscardData(void)
{
    int a;
    for (a=0;a<SectionsRead;a++){
        ehFree(Sections[a].pbData);
    }
    // memset(&sImageInfo, 0, sizeof(sImageInfo));
    SectionsRead = 0;
    HaveAll = 0;
}
/*
//--------------------------------------------------------------------------
// Read image data.
//--------------------------------------------------------------------------
int ReadJpegFile(const char * FileName, EN_EXIF_READMODE ReadMode)
{
    FILE * infile;
    int ret;

    infile = fopen(FileName, "rb"); // Unix ignores 'b', windows needs it.

    if (infile == NULL) {
        fprintf(stderr, "can't open '%s'\n", FileName);
        return FALSE;
    }

    // Scan the JPEG headers.
    ret = _ReadJpegSections(infile, ReadMode);
    if (!ret){
        printf("Not JPEG: %s\n",FileName);
    }

    fclose(infile);

    if (ret == FALSE){
        _DiscardData();
    }
    return ret;
}
*/


//--------------------------------------------------------------------------
// Replace or remove exif thumbnail
//--------------------------------------------------------------------------
/*
int SaveThumbnail(char * ThumbFileName)
{
    FILE * ThumbnailFile;

    if (psExif->ThumbnailOffset == 0 || psExif->ThumbnailSize == 0){
        printf("Image contains no thumbnail\n");
        return FALSE;
    }

    if (strcmp(ThumbFileName, "-") == 0){
        // A filename of '-' indicates thumbnail goes to stdout.
        // This doesn't make much sense under Windows, so this feature is unix only.
        ThumbnailFile = stdout;
    }else{
        ThumbnailFile = fopen(ThumbFileName,"wb");
    }

    if (ThumbnailFile){
        BYTE * ThumbnailPointer;
        S_EXIF_SECTION * ExifSection;
        ExifSection = FindSection(M_EXIF);
        ThumbnailPointer = ExifSection->Data+psExif->ThumbnailOffset+8;

        fwrite(ThumbnailPointer, psExif->ThumbnailSize ,1, ThumbnailFile);
        fclose(ThumbnailFile);
        return TRUE;
    }else{
        ErrFatal("Could not write thumbnail file");
        return FALSE;
    }
}


//--------------------------------------------------------------------------
// Replace or remove exif thumbnail
//--------------------------------------------------------------------------
int ReplaceThumbnail(const char * ThumbFileName)
{
    FILE * ThumbnailFile;
    int ThumbLen, NewExifSize;
    S_EXIF_SECTION * ExifSection;
    BYTE * ThumbnailPointer;

    if (psExif->ThumbnailOffset == 0 || psExif->ThumbnailAtEnd == FALSE){
        // Adding or removing of thumbnail is not possible - that would require rearranging
        // of the exif header, which is risky, and jhad doesn't know how to do.

        printf("Image contains no thumbnail to replace - add is not possible\n");
        return FALSE;
    }

    if (ThumbFileName){
        ThumbnailFile = fopen(ThumbFileName,"rb");

        if (ThumbnailFile == NULL){
            ErrFatal("Could not read thumbnail file");
            return FALSE;
        }

        // get length
        fseek(ThumbnailFile, 0, SEEK_END);

        ThumbLen = ftell(ThumbnailFile);
        fseek(ThumbnailFile, 0, SEEK_SET);

        if (ThumbLen + psExif->ThumbnailOffset > 0x10000-20){
            ErrFatal("Thumbnail is too large to insert into exif header");
        }
    }else{
        ThumbLen = 0;
        ThumbnailFile = NULL;
    }

    ExifSection = FindSection(M_EXIF);

    NewExifSize = psExif->ThumbnailOffset+8+ThumbLen;
    ExifSection->Data = (BYTE *)realloc(ExifSection->Data, NewExifSize);

    ThumbnailPointer = ExifSection->Data+psExif->ThumbnailOffset+8;

    if (ThumbnailFile){
        fread(ThumbnailPointer, ThumbLen, 1, ThumbnailFile);
        fclose(ThumbnailFile);
    }

    psExif->ThumbnailSize = ThumbLen;

    _Put32u(ExifSection->Data+psExif->ThumbnailSizeOffset+8, ThumbLen);

    ExifSection->Data[0] = (BYTE)(NewExifSize >> 8);
    ExifSection->Data[1] = (BYTE)NewExifSize;
    ExifSection->Size = NewExifSize;

    return TRUE;
}

*/

//--------------------------------------------------------------------------
// Discard everything but the exif and comment sections.
//--------------------------------------------------------------------------
void DiscardAllButExif(void)
{
    S_EXIF_SECTION ExifKeeper;
    S_EXIF_SECTION CommentKeeper;
    int a;

    memset(&ExifKeeper, 0, sizeof(ExifKeeper));
    memset(&CommentKeeper, 0, sizeof(CommentKeeper));

    for (a=0;a<SectionsRead;a++){
        if (Sections[a].Type == M_EXIF && ExifKeeper.Type == 0){
            ExifKeeper = Sections[a];
        }else if (Sections[a].Type == M_COM && CommentKeeper.Type == 0){
            CommentKeeper = Sections[a];
        }else{
            ehFree(Sections[a].pbData);
        }
    }
    SectionsRead = 0;
    if (ExifKeeper.Type){
        Sections[SectionsRead++] = ExifKeeper;
    }
    if (CommentKeeper.Type){
        Sections[SectionsRead++] = CommentKeeper;
    }
}    
/*
//--------------------------------------------------------------------------
// Write image data back to disk.
//--------------------------------------------------------------------------
void WriteJpegFile(const char * FileName)
{
    FILE * outfile;
    int a;

    if (!HaveAll){
        ErrFatal("Can't write back - didn't read all");
    }

    outfile = fopen(FileName,"wb");
    if (outfile == NULL){
        ErrFatal("Could not open file for write");
    }

    // Initial static jpeg marker.
    fputc(0xff,outfile);
    fputc(0xd8,outfile);
    
    if (Sections[0].Type != M_EXIF && Sections[0].Type != M_JFIF){
        // The image must start with an exif or jfif marker.  If we threw those away, create one.
        static BYTE JfifHead[18] = {
            0xff, M_JFIF,
            0x00, 0x10, 'J' , 'F' , 'I' , 'F' , 0x00, 0x01, 
            0x01, 0x01, 0x01, 0x2C, 0x01, 0x2C, 0x00, 0x00 
        };
        fwrite(JfifHead, 18, 1, outfile);
    }

    // Write all the misc sections
    for (a=0;a<SectionsRead-1;a++){
        fputc(0xff,outfile);
        fputc(Sections[a].Type, outfile);
        fwrite(Sections[a].pbData, Sections[a].Size, 1, outfile);
    }

    // Write the remaining image data.
    fwrite(Sections[a].Data, Sections[a].Size, 1, outfile);
       
    fclose(outfile);
}
*/

//--------------------------------------------------------------------------
// Check if image has exif header.
//--------------------------------------------------------------------------
S_EXIF_SECTION * FindSection(int SectionType)
{
    int a;

    for (a=0;a<SectionsRead;a++){
        if (Sections[a].Type == SectionType){
            return &Sections[a];
        }
    }
    // Could not be found.
    return NULL;
}

//--------------------------------------------------------------------------
// Remove a certain type of section.
//--------------------------------------------------------------------------
/*
int RemoveSectionType(int SectionType)
{
    int a;
    for (a=0;a<SectionsRead-1;a++){
        if (Sections[a].Type == SectionType){
            // Free up this section
            ehFree(Sections[a].pbData);
            // Move succeding sections back by one to close space in array.
            memmove(Sections+a, Sections+a+1, sizeof(S_EXIF_SECTION) * (SectionsRead-a));
            SectionsRead -= 1;
            return TRUE;
        }
    }
    return FALSE;
}
*/

//--------------------------------------------------------------------------
// Remove sectons not part of image and not exif or comment sections.
//--------------------------------------------------------------------------
int RemoveUnknownSections(void)
{
    int a;
    int Modified = FALSE;
    for (a=0;a<SectionsRead-1;){
        switch(Sections[a].Type){
            case  M_SOF0:
            case  M_SOF1:
            case  M_SOF2:
            case  M_SOF3:
            case  M_SOF5:
            case  M_SOF6:
            case  M_SOF7:
            case  M_SOF9:
            case  M_SOF10:
            case  M_SOF11:
            case  M_SOF13:
            case  M_SOF14:
            case  M_SOF15:
            case  M_SOI:
            case  M_EOI:
            case  M_SOS:
            case  M_JFIF:
            case  M_EXIF:
            case  M_COM:
            case  M_DQT:
            case  M_DHT:
            case  M_DRI:
                // keep.
                a++;
                break;
            default:
                // Unknown.  Delete.
                ehFree(Sections[a].pbData);
                // Move succeding sections back by one to close space in array.
                memmove(Sections+a, Sections+a+1, sizeof(S_EXIF_SECTION) * (SectionsRead-a));
                SectionsRead -= 1;
                Modified = TRUE;
        }
    }
    return Modified;
}

//--------------------------------------------------------------------------
// Add a section (assume it doesn't already exist) - used for 
// adding comment sections.
//--------------------------------------------------------------------------
S_EXIF_SECTION * CreateSection(int SectionType, unsigned char * pbData, int Size)
{
    S_EXIF_SECTION * NewSection;
    int a;

    // Insert it in third position - seems like a safe place to put 
    // things like comments.

    if (SectionsRead < 2){
        ErrFatal("Too few sections!");
    }
    if (SectionsRead >= MAX_SECTIONS){
        ErrFatal("Too many sections!");
    }

    for (a=SectionsRead;a>2;a--){
        Sections[a] = Sections[a-1];          
    }
    SectionsRead += 1;

    NewSection = Sections+2;

    NewSection->Type = SectionType;
    NewSection->Size = Size;
    NewSection->pbData = pbData;

    return NewSection;
}


//--------------------------------------------------------------------------
// Initialisation.
//--------------------------------------------------------------------------
void ResetJpgfile(void)
{
    memset(&Sections, 0, sizeof(Sections));
    SectionsRead = 0;
    HaveAll = 0;
}

