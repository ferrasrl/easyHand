//   ---------------------------------------------
//   | DATAUTIL Funzioni di servizio per il      |
//   |          trattamento delle date           |
//   |                                           |
//   |            by Ferr‡ Art & Technology 2004
//   ---------------------------------------------
#include "/easyhand/inc/easyhand.h"
#include "/easyhand/ehtool/main/datautil.h"
#include <time.h>
#include <math.h>

// +-----------------------------------------+
// | ora_make Costruisce lora in formato     |
// |          00:00                          |
// |         Ferr‡ Art & Technology (c) 1996 |
// +-----------------------------------------+

CHAR *ora_make(SINT hh,SINT mm)
{
	static CHAR Ora[12];
	sprintf(Ora,"%02d:%02d",hh,mm);
	return Ora;
}

// +-----------------------------------------+
// | NGget Ritorna l'NG utile...qualche volta|
// |                                         |
// | NG = 0 Lun                              |
// | NG = 1 Mar                              |
// | NG = 2 Mer                              |
// | NG = 3 Gio                              |
// | NG = 4 Ven                              |
// | NG = 5 Sab                              |
// | NG = 6 Dom                              |
// |                                         |
// |                                         |
// |         Ferr‡ Art & Technology (c) 1996 |
// +-----------------------------------------+
SINT NGgetS(CHAR *DataDritta)
{
	CHAR serv[12];
	strcpy(serv,dateFor(DataDritta,"-"));
	serv[2]=0; serv[5]=0;
	return NGget(atoi(serv),atoi(serv+3),atoi(serv+6));
}

SINT NGget(SINT day,SINT month,SINT year)
{

// month : 1..12
// date  : 1..31
// year  : 1582..4902
//
// returns : 0 (sunday) ... 6 (saturday)
    SINT yearnum,cent,dayw; 

	month = month-2;
     if(month<1) {
        month+=12;
        year--;
     }

     //if ((year>=2000)&&(month>=2)) day++;
	 yearnum=year%100;  cent=year/100;
	 
     dayw = (SINT) (floor(2.6*month-0.2)+day+yearnum+yearnum/4+cent/4-2*cent);
	 if (dayw<0) dayw=7+(dayw%7);
	 dayw%=7;

	 if (!dayw) dayw=6; else dayw--;
     return dayw;
}

CHAR *DammiFineMese(CHAR *DataDritta)
{
	static CHAR Dat1[9];
	SINT m,a,mg;

	memcpy(Dat1,DataDritta+2,2);
	Dat1[2]=0;
	m=atoi(Dat1);
	a=atoi(DataDritta+4);

	mg=sys.arDayPerMonth[m-1];
	if (((a%4)==0)&&(m==2)) mg++; // Bisestile

	sprintf(Dat1,"%02d%02d%04d",mg,m,a);
	return Dat1;
}
// La funzione sottrae Data A dalla Data B
// se Flag=FALSE le date sono in formato GG/MM/AAAA
// se Flag=TRUE  le date sono in formato GGMMAAAA

LONG DateSub(BOOL Flag,CHAR *DataDrittaA,CHAR *DataDrittaB)
{
	LONG Segno=0;
	LONG Giorni=0;
	CHAR DtA[12];
	CHAR DtB[12];
	CHAR *p1,*p2;
	EH_TIME timeEleA;
	EH_TIME timeEleB;

	if (!Flag)
	{
	strcpy(DtA,DataDrittaA+6);
	memcpy(DtA+4,DataDrittaA+3,2);
	memcpy(DtA+6,DataDrittaA,2);
	DtA[8]=0;

	strcpy(DtB,DataDrittaB+6);
	memcpy(DtB+4,DataDrittaB+3,2);
	memcpy(DtB+6,DataDrittaB,2);
	DtB[8]=0;
	}
	else
	{
	strcpy(DtA,DataDrittaA+4);
	memcpy(DtA+4,DataDrittaA+2,2);
	memcpy(DtA+6,DataDrittaA,2);
	DtA[8]=0;

	strcpy(DtB,DataDrittaB+4);
	memcpy(DtB+4,DataDrittaB+2,2);
	memcpy(DtB+6,DataDrittaB,2);
	DtB[8]=0;
	}

	if (strcmp(DtB,DtA)>0)
	 {
		p1=DataDrittaB;
		p2=DataDrittaA;
		Segno=-1;
	 }
	 else
	 {
		p1=DataDrittaA;
		p2=DataDrittaB;
		Segno=1;
	 }

	timeSet(&timeEleA,p1,NULL);//Flag);
	timeSet(&timeEleB,p2,NULL);//Flag);

 // Calcolo sfasamento

	Giorni=-timeEleB.wDayYear;//timeDayOfYear(&timeEleB);

	//
	// Loop sugli anni
	//
	for (;timeEleB.wYear<timeEleA.wYear;timeEleB.wYear++)
	{
		 Giorni+=365; if (!(timeEleB.wYear%4)) Giorni++;
	}

	Giorni+=timeEleA.wDayYear;//timeDayOfYear(&timeEleA);

	return (Giorni*Segno);
}
//
// La funzione calcola i minuti che intercorrono da
// DATA_A/ORA_A --- > DATA_B/ORA_B ; logicamente esegue DATA_B-DATA_A
// se Flag=FALSE le date sono in formato AAAAMMMGG (db)
// se Flag=TRUE  le date sono in formato GGMMAAAA
// L'ora deve essere nel formato "hh:mm"
//
LONG TimeDiff(BOOL fDat, // FALSE= data AAAAMMGG TRUE=GGMMAAAA
			  CHAR *lpData_A,CHAR *lpOra_A,
			  CHAR *lpData_B,CHAR *lpOra_B)
{
	CHAR szDateA[20];
	CHAR szDateB[20];
	CHAR szServ[80];
	SINT iDay;
	SINT iMin_A,iMin_B;

	// Calcolo la distanza dalla Data/OraEnd della tratta precedente e il Data/OraStart di quella nuova
	if (fDat) strcpy(szDateA,lpData_A); else strcpy(szDateA,dateYtoD(lpData_A));
	if (fDat) strcpy(szDateB,lpData_B); else strcpy(szDateB,dateYtoD(lpData_B));
	iDay=DateSub(TRUE,szDateB,szDateA);
	// Calcolo distanza in ore
	// Ora start-ora end (vecchia) + iDay*60*24 
	// Ora start
	strcpy(szServ,lpOra_A); szServ[2]=0; iMin_A=atoi(szServ)*60+atoi(szServ+3);
	strcpy(szServ,lpOra_B); szServ[2]=0; iMin_B=atoi(szServ)*60+atoi(szServ+3);
	return (iMin_B-iMin_A+(iDay*60*24));
}

/*
	
//
// gettimeofday()
//
#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif

int gettimeofday(struct timeval *tv, struct timezone * tz)
{
  FILETIME ft;
  time_t tmpres = 0; // unsigned __int64
  static int tzflag=0;
 
  if (NULL != tv)
  {
    GetSystemTimeAsFileTime(&ft);
 
    tmpres |= ft.dwHighDateTime;
    tmpres <<= 32;
    tmpres |= ft.dwLowDateTime;
 
    // converting file time to unix epoch
    tmpres -= DELTA_EPOCH_IN_MICROSECS; 
    tmpres /= 10;  // convert into microseconds
    tv->tv_sec = (long)(tmpres / 1000000UL);
    tv->tv_usec = (long)(tmpres % 1000000UL);
  }
 
  if (NULL != tz)
  {
    tz->tz_minuteswest = _timezone / 60;
    tz->tz_dsttime = _daylight;
  }
 
  return 0;
} 


*/

/*
static CHAR *GiornoLungoIT[]={"Lunedi","Martedi",
							 "Mercoledi","Giovedi",
							 "Venerdi","Sabato",
							 "Domenica"};

static CHAR *GiornoCortoIT[]={"Lun","Mar","Mer","Gio","Ven","Sab","Dom"};

static CHAR *MeseLungoIT[]={"Gennaio","Febbraio","Marzo",
						   "Aprile","Maggio","Giugno",
						   "Luglio","Agosto","Settembre",
						   "Ottobre","Novembre","Dicembre"};

static CHAR *MeseCortoIT[]={"Gen","Feb","Mar",
						   "Apr","Mag","Giu",
						   "Lug","Ago","Set",
						   "Ott","Nov","Dic"};

#ifdef _WIN32

static CHAR *GiornoLungoUK[]={"Monday",
							  "Tuesday",
							  "Wednesday",
							  "Thursday",
							  "Friday",
							  "Saturday",
							  "Sunday"};

static CHAR *GiornoCortoUK[]={"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};

static CHAR *MeseLungoUK[]={"January","February","March","April",
							"May","June","July","August","September",
							"October","November","December"};

static CHAR *MeseCortoUK[]={"Jan","Feb","Mar",
						   "Apr","May","Jun",
						   "Jul","Aug","Sep",
						   "Oct","Nov","Dec"};
#endif

static CHAR **GiornoLungo=GiornoLungoIT;
static CHAR **GiornoCorto=GiornoCortoIT;
static CHAR **MeseLungo=MeseLungoIT;
static CHAR **MeseCorto=MeseCortoIT;
#ifdef _WIN32

static void SwitchLang()
{
	
	switch (sys.iLanguage)
	{
		case EH_LANG_ITALIAN:
			GiornoLungo=GiornoLungoIT;
			GiornoCorto=GiornoCortoIT;
			MeseLungo=MeseLungoIT;
			MeseCorto=MeseCortoIT;
			break;

		case EH_LANG_ENGLISH:
			GiornoLungo=GiornoLungoUK;
			GiornoCorto=GiornoCortoUK;
			MeseLungo=MeseLungoUK;
			MeseCorto=MeseCortoUK;
			break;
	}

}
	#endif

CHAR *MeseNome(SINT Mese,SINT Modo)
{
#ifdef _WIN32
	SwitchLang();
#endif

switch (Modo)
 {
	 case 0: return MeseLungo[Mese-1];
	 case 1: return MeseCorto[Mese-1];
 }
 return "?";
}

*/

/*
---------------------------------------------------------------------------
By: Heiki Kask
Re: Suveaeg
---------------------------------------------------------------------------
 > 1. Millal minnakse Åle suveajale ja jÑlle tagasi ?

Siis, kui ollakse tund aega hiljaks jÑÑdud vìi varem tuldud ;)

 > 2. Oli miski kaval valem, mis lubas aasta, kuu ja
 > pÑeva jÑrgi nÑdalapÑeva leida. Kas keegi teab kuidas
 > see tÑpselt kÑis ?

Valem oli olemas, vist zelleri nimeline. TÑpset valemit ei mÑleta, aga selline
jupp peaks asja Ñra tegema kÅll (sisusse ei ole sÅvenenud):
*/


/*
DWORD TimeMillisec(void)
{
	SYSTEMTIME sysTimeStruct;
	GetLocalTime( &sysTimeStruct ); 
	return sysTimeStruct.wMilliseconds;
}
*/


/*
CHAR * ora_sep(CHAR *ora,CHAR sep)
{
	static CHAR RetOra[10];
	sprintf(RetOra,"%c%c%c%c%c%c%c%c",*ora,*(ora+1),sep,*(ora+2),*(ora+3),sep,*(ora+4),*(ora+5));
	return RetOra;
}
*/
