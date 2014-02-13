//
// adbConst.h
// Costanti e macro usate da adb
//
//

#ifdef __cplusplus
extern "C" {
#endif
	typedef enum {
	// Prova di archiviazione dati
	ADB_ALFA,
	ADB_NUME,
	ADB_DATA,
	ADB_INT,		// Intero a 16bit
	ADB_FLOAT,
	ADB_BOOL,		// Valore vero o falso
	ADB_COBD,		// Cobol Decimal  New 2000
	ADB_COBN,		// Cobol Numeric  New 2000
	ADB_AINC,		// AutoIncrement  New 2000
	ADB_BLOB,		// TEXT Note da 5 a 32K
	ADB_INT32,		// Intero a 32 bit

	ADB_GEOMETRY,	// Geometrico
	ADB_POINT,		// Point
	ADB_BINARY,		// Dati binary
	ADB_TIMESTAMP	// TimeStamp (UTC value)

} EN_FLDTYPE;
	#define HREC LONG

#ifdef __cplusplus
}
#endif
