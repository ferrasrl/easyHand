//-------------------------------------------
// eh_obsolete.h
// Easyhand Obsolete
// Include di retrocompatibilità
// Ferrà srl 2011
//

#define PRG_start ehStart
#define PRG_end ehExit
#define PRG_endPoint ehError

#define memo_usata memoUsed
#define memo_chiedi memoAlloc
#define memo_libera memoFree
#define memo_scrivivar memoWrite
#define memo_leggivar memoRead
#define memo_copyall memoCopyAll
#define memoGetType memoGetType
#define memo_name memoGetName
#define memo_info memoGetInfo
#define Wmemo_lock memoLock
#define Wmemo_lockEx memoLockEx
#define Wmemo_unlock memoUnlock
#define Wmemo_unlockEx memoUnlockEx
#define memo_clone memoClone

#define EhAlloc ehAlloc
#define EhAllocZero ehAllocZero
#define EhReAlloc ehRealloc
#define EhFree ehFree
#define EhFreeNN ehFreeNN
#define EhFreePtr ehFreePtr
#define EhFreePtrs ehFreePtrs
#define Ehmemcpy ehMemcpy

#define EhLogOpen ehLogOpen
#define EhLogWrite ehLogWrite
#define EhLogWriteEx ehLogWriteEx

#define EhConWrite ehPrint
#define ehdSet timeSet
#define ehdCalc timeCalc
#define ehdGetDate timeGetDate
#define ehdFormat timeFormat
#define DateToday dateToday
#define EH_DATE EH_TIME
#define dtNow dtNow
#define DtGetDate dtGetDate
#define DtGetTime dtGetTime
#define DtCompare dtCompare

#define DateTodayRev dateTodayRev
#define data_oggi dateToday
#define daAnno dateYtoD
#define daGiorno dateDtoY
#define data_calcola dateCalc
//#define timeT64ToEhd timeT64ToEht