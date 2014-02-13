//
// DateUtil.h
//

//
// Funzioni in disuso
//

//CHAR *	data_sep(CHAR *dat,CHAR *sep);
//CHAR *	data_sepEx(CHAR *dat,CHAR *sep,CHAR *lpBuffer);
//CHAR *	ora_sep(CHAR *ora,CHAR sep);
//CHAR *	ora_oggi(CHAR *tipo);

CHAR *	ora_make(SINT hh,SINT mm);
SINT	NGgetS(CHAR *DataDritta);
SINT	NGget(SINT GG,SINT MM,SINT AA);
//CHAR *	DataReverse(CHAR *ptd);
//CHAR *	daAnno(CHAR *dat);
//CHAR *	daGiorno(CHAR *dat);
CHAR *	DammiFineMese(CHAR *DataDritta);
CHAR *	MeseNome(SINT Mese,SINT Modo);


//LONG  DateSub(CHAR *DataDrittaA,CHAR *DataDrittaB);
LONG	DateSub(BOOL Flag,CHAR *DataDrittaA,CHAR *DataDrittaB);
//CHAR *	data_calcola(CHAR *data,SINT valore);
//#define DateAdd data_calcola
//#define DateFormat data_sep
//#define DateToday data_oggi
//CHAR *DateTodayRev(void);
//CHAR *DateTodayRfc(void); // new 2009

// New 99
CHAR *	DateChoose(CHAR *Date);

//CHAR *	TimeMalloc(CHAR *tipo,SINT iSize);
//DWORD	TimeMillisec(void); // new 2008
// New 2002
LONG	TimeDiff(BOOL fDat, // FALSE= data AAAAMMGG TRUE=GGMMAAAA
			  CHAR *lpData_A,CHAR *lpOra_A,
			  CHAR *lpData_B,CHAR *lpOra_B);
//CHAR *	ClockToTime(time_t tempo,SINT iMode); // New 2006
//CHAR *	TimeFormat(CHAR *lpHMS,SINT iMode);

//#ifdef _WIN32
 // New 2007
// void UnixTimeToFileTime(time_t t, LPFILETIME pft);
// int gettimeofday(struct timeval *tv, struct timezone *tz); // NEW 2010
//#endif

