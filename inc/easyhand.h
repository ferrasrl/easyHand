//   +-------------------------------------------+
//    Easyhand.h
//    Header di Easyhand
//
//                by Ferrà Art & Technology 1999
//                by Ferrà srl 2008
//   +-------------------------------------------+
#ifndef __easyhand__

	#define __easyhand__
	#include "/easyhand/inc/eh_core.h"

	#ifdef EH_OBSOLETE
		#include "/easyhand/inc/eh_obsolete.h"
	#endif
	//
	// EH_INTERNET
	//
	#if (defined(EH_MAIL)||defined(EH_INTERNET))
		#include "/easyhand/inc/eh_internet.h"
	#endif

	//
	// EH_MAIL
	//
	#if (defined(EH_MAIL))
		#include "/easyhand/inc/ehMail.h"
	#endif

	//
	// EH_FASTCGI
	//
	#ifdef EH_FASTCGI

		#include "/easyhand/ehtoolx/fastCGI/include/fcgi_config.h"
		#define NO_FCGI_DEFINES
		#include "/easyhand/ehtoolx/fastCGI/include/fcgi_stdio.h"
		#undef  printf
		#define	printf   FCGI_printf

	#endif

	//
	// EH_ZLIB
	//
	#ifdef EH_ZLIB
		#include "/easyhand/inc/eh_zip.h"
		
	#endif


	//
	// BTRIEVE / ADB
	//
	#if (defined(_ADB32)||defined(_ADB32P))
		#include "/easyhand/inc/eh_adb.h"
	#else
		#include "/easyhand/inc/adbConst.h" // Solo le costanti e macro
	#endif

	//
	// <== SQL SECTION (START) =======================================================
	//

void *	 sqlScroll(struct OBJ * objCalled,EN_MESSAGE cmd,LONG info,CHAR *str);
#define _sqlScrollAdaptor_ EH_DISPEXT * psExt=(EH_DISPEXT *) pbReturn; SQL_RS rsSet=(SQL_RS ) lParam; EN_MESSAGE enMess=message;

	#ifdef EH_SQL_ODBC

		#define EH_ODBC
		#define SQL_RS EH_ODBC_RS

//		#ifndef EH_ODBC_MT // Single-Thread

			#define sql_count odbc_count
			#define sql_query odbc_queryarg
			#define sql_querybig odbc_queryargBig
			#define sql_row odbc_queryrow
			#define sql_store() odbc_store_result(200)
			#define sql_fetch odbc_fetch_row
			#define sql_hookget odbc_hookget
			#define sql_hookupdate odbc_hookupdate
/*
//		#else
			#define sql_count(msg,...) odbc_count(arsOdbcSection,msg,__VA_ARGS__)
			#define sql_query(msg,...) odbc_queryarg(arsOdbcSection,msg,__VA_ARGS__)
			#define sql_querybig(size,msg,...) odbc_queryargBig(arsOdbcSection,size,msg,__VA_ARGS__)
			#define sql_row(msg,...) odbc_queryrow(arsOdbcSection,msg,__VA_ARGS__)
			#define sql_store() odbc_store_result(arsOdbcSection,200)
			#define sql_fetch odbc_fetch_row
			#define sql_hookget(hook,mess,...) odbc_hookget(arsOdbcSection,hook,mess,__VA_ARGS__)
			#define sql_hookupdate(hook,query,...) odbc_hookupdate(arsOdbcSection,hook,query,__VA_ARGS__)
//		#endif
*/

		#define sql_free(a) {odbc_free_result(a); a=NULL;}
		#define sql_find(a,b) odbc_fldfind(a,b,false)
		#define sql_int(a,b) odbc_fldint(a,b)
		#define sql_ptr(a,b) odbc_fldptr(a,b)
		#define sql_num(a,b) odbc_fldnum(a,b)

//		#ifndef EH_CONSOLE
//			#define EH_SQLPARAMS EH_OBJPARAMS,SQL_RS rsSet
//		#endif

	#endif

	#ifdef EH_ODBC
		#include "/easyhand/inc/eh_odbc.h"
	#endif


	#ifdef EH_SQL_MYSQL

	#if (!defined(_MYSQL)&&!defined(_MYSQL_MT))
		#define _MYSQL
	#endif
		#include "/easyhand/inc/mySql.h"
		#define SQL_RS EH_MYSQL_RS
		#define sql_count mys_count
		#define sql_query mys_queryarg
		#define sql_querybig mys_queryargBig
		#define sql_row mys_queryrow
		#define sql_lastid mys_lastid
#ifdef _MYSQL_MT
		#define sql_store(a) mys_store_result(a)
		#define sql_stored(a) mys_store_result(a)
#else
		#define sql_store() mys_store_result()
		#define sql_stored(a) mys_store_result()
#endif
		#define sql_fetch mys_fetch_row
		#define sql_free(a) {mys_free_result(a); a=NULL;}
		#define sql_int(a,b) mys_fldint(a,b)
		#define sql_ptr(a,b) mys_fldptr(a,b)
		#define sql_num(a,b) mys_fldnum(a,b)
		#define sql_len(a,b) mys_fldlen(a,b)

	#endif

	#ifdef EH_SQL_SQLITE

		#define EH_SQLITE
		#include "/easyhand/ehtool/SQLite.h"

		#define SQL_RS EH_SQLITE_RS
		#define sql_count sqlite_count
		#define sql_query sqlite_queryarg
		#define sql_querybig sqlite_queryargBig
		#define sql_row sqlite_queryrow
		#define sql_max sqlite_max
		
		#define sql_store() sqlite_store()
		#define sql_fetch sqlite_fetch
		#define sql_free(a) {sqlite_free(a); a=NULL;}

		#define sql_find(a,b) sqlite_fldfind(a,b)
		#define sql_int(a,b) sqlite_int(a,b)
		#define sql_ptr(a,b) sqlite_ptr(a,b)
		#define sql_num(a,b) sqlite_num(a,b)
	#endif

	#if (!defined(EH_SQL_SQLITE)&&defined(EH_SQLITE))
		#include "/easyhand/ehtool/SQLite.h"
	#endif

	#ifdef EH_MDB
		#include "/easyhand/inc/eh_mdb.h"
	#endif

    //
    // <== SQL SECTION (END) =======================================================
    //
#endif

#ifdef __OBJC__
 #import "/easyhand/inc/ehxKernel.h"
#endif

//
// <== EH_MAIN Only =======================================================
//

#ifdef EH_MAIN

	#if (defined(EH_MEMO_DEBUG))
		#pragma message("\7 Attenzione: Easyhand Memory Debug > ATTIVO!")
    #endif

	#ifdef __windows__
		#pragma comment(lib, "Psapi.lib")
	#endif

    //
    // EH_ZLIB
    //
	#if (defined(EH_ZLIB))
		#pragma message("--> Includo zddl.lib <-------- per ZLIB -----------")
		#pragma comment(lib, "/easyhand/ehtoolx/zlib/lib/zdll.lib")
	#endif

    //
    // EH_CONSOLE
    //
	#if (defined(EH_FASTCGI))

		#pragma message("--> Includo libfcgi.lib <-------- per fastCGI -----------")
		#pragma comment(lib, "/Easyhand/ehtoolx/fastCGI/lib/win32/libfcgi.lib")	

	#endif


    //
    // EH_CONSOLE
    //
    #if (defined(EH_CONSOLE))

		#if (!defined(WIN64)) 

			#if (defined(EH_MEMO_DEBUG))
				#pragma message("--> Includo /easyhand/lib/win32/ehLib9cs_dm.lib <-------- eh:Console (debug memory) -----------")
				#pragma comment(lib, "/easyhand/lib/win32/ehLib9cs_dm.lib")
			#elif (defined(_DEBUG))
				#pragma message("--> Includo /easyhand/lib/win32/ehLib9cs_d.lib <-------- eh:Console (debug) -----------")
				#pragma comment(lib, "/easyhand/lib/win32/ehLib9cs_d.lib")
			#else
				#pragma message("--> Includo /easyhand/lib/win32/ehLib9cs.lib <-------- eh:Console (debug) -----------")
				#pragma comment(lib, "/easyhand/lib/win32/ehLib9cs.lib")
			#endif

		#endif

    //
    // EH_PRINT
    //
	#elif (defined(EH_PRINT))

		#if (defined(EH_MEMO_DEBUG))
			#pragma message("--> Includo /easyhand/lib/win32/ehLib9p_dm.lib <-------- eh:Gui Print (debug memory) -----------")
			#pragma comment(lib, "/easyhand/lib/win32/ehLib9p_dm.lib")
		#elif (defined(_DEBUG))
			#pragma message("--> Includo /easyhand/lib/win32/ehLib9p_d.lib <-------- eh:Gui Print (debug) -----------")
			#pragma comment(lib, "/easyhand/lib/win32/ehLib9p_d.lib")
		#else
			#pragma message("--> Includo /easyhand/lib/win32/ehLib9p.lib <-------- eh:Gui Print (debug) -----------")
			#pragma comment(lib, "/easyhand/lib/win32/ehLib9p.lib")
		#endif

    //
    // EH_COMPLETED
    //
	#elif (defined(EH_COMPLETED))

		#if (defined(EH_MEMO_DEBUG))
			#pragma message("--> Includo /easyhand/lib/win32/ehLib9_dm.lib <-------- eh:Gui Completed (debug memory) -----------")
			#pragma comment(lib, "/easyhand/lib/win32/ehLib9_dm.lib")
		#elif (defined(_DEBUG))
			#pragma message("--> Includo /easyhand/lib/win32/ehLib9_d.lib <-------- eh:Gui Completed (debug) -----------")
			#pragma comment(lib, "/easyhand/lib/win32/ehLib9_d.lib")
		#else
			#pragma message("--> Includo /easyhand/lib/win32/ehLib9.lib <-------- eh:Gui Completed (debug) -----------")
			#pragma comment(lib, "/easyhand/lib/win32/ehLib9.lib")
		#endif

	#endif

    //
    // Mail
    //
    #if (defined(EH_MAIL))

		#ifdef EH_MEMO_DEBUG
			#pragma message("--> Includo /easyhand/lib/win32/ehMail9cs_dm.lib <-------- eMail support (debug) -----------")
			#pragma comment(lib, "/easyhand/lib/win32/ehMail9cs_dm.lib")
		#else
			#pragma message("--> Includo /easyhand/lib/win32/ehMail9cs.lib <-------- eMail support -----------")
			#pragma comment(lib, "/easyhand/lib/win32/ehMail9cs.lib")
		#endif

		#ifndef EH_INTERNET
			#define EH_INTERNET
		#endif

    #endif

	//
	// EH_INTERNET (Librerie necessarie)
	//
    #ifdef EH_INTERNET

		#ifdef __windows__

			#ifdef EH_MEMO_DEBUG

//				#pragma message("--> Includo /easyhand/lib/win32/ehSocket9cs_dm.lib <-------- Socket support (debug) -----------")
//				#pragma comment(lib, "/easyhand/lib/win32/ehSocket9cs_dm.lib")
				#pragma message("--> Includo /easyhand/lib/win32/ehWeb9cs_dm.lib   <-------- Web support (debug) -----------")
				#pragma comment(lib, "/easyhand/lib/win32/ehWeb9cs_dm.lib")
				#pragma comment(lib, "Ws2_32.lib")
				#pragma comment(lib, "wininet.lib")
				#pragma message("--> Includo wsock32.lib <-------------------")
				#pragma comment(lib, "wsock32.lib")

			#else

//				#pragma message("--> Includo /easyhand/lib/win32/ehSocket9.lib <-------- Socket support -----------")
//				#pragma comment(lib, "/easyhand/lib/win32/ehSocket9.lib")
#ifdef EH_CONSOLE
				#pragma message("--> Includo /easyhand/lib/win32/ehWeb9cs.lib    <-------- Web support -----------")
				#pragma comment(lib, "/easyhand/lib/win32/ehWeb9cs.lib")
#else
				#pragma message("--> Includo /easyhand/lib/win32/ehWeb9.lib    <-------- Web support -----------")
				#pragma comment(lib, "/easyhand/lib/win32/ehWeb9.lib")
#endif
				#pragma comment(lib, "Ws2_32.lib")
				#pragma comment(lib, "wininet.lib")
				#pragma message("--> Includo wsock32.lib <-------------------")
				#pragma comment(lib, "wsock32.lib")

			#endif

			// Richiesto SSL x Https
			#ifdef WITH_OPENSSL
				#pragma message("--> + /easyhand/ehtoolx/OpenSSL - GnuWin32/lib/libcrypto.lib <--------------------")
				#pragma comment(lib, "/easyhand/ehtoolx/OpenSSL - GnuWin32/lib/libcrypto.lib")
				#pragma message("--> + /easyhand/ehtoolx/OpenSSL - GnuWin32/lib/libssl.lib <--------------------")
				#pragma comment(lib, "/easyhand/ehtoolx/OpenSSL - GnuWin32/lib/libssl.lib")
			#endif

		#endif
    #endif

    #if (defined(EH_SQLITE))
        #pragma message("--> Includo /easyhand/lib/win32/ehSql9.lib <-------- SQLite 3 support -----------")
        #pragma comment(lib, "/easyhand/lib/win32/ehSql9.lib")
    #endif

    #if (!defined(EH_CONSOLE))
        #pragma message("--> Includo version.lib <-------- Version support -----------")
        #pragma comment(lib, "version.lib")
    #endif

	#ifdef EH_WIN_GDI
        #pragma message("--> Includo Msimg32.lib <-------- per AlphaBlend -----------")
		#pragma comment(lib, "Msimg32.lib")	// The GDI+ binary
	#endif


//
// Variabili
//
    #include "/easyhand/inc/eh_var.h"

#else
    #include "/easyhand/inc/eh_vari.h"
#endif

//#if (defined(_ADB32)||defined(_ADB32P))
//	#include "/easyhand/inc/eh_adbvar.h"
//#endif
