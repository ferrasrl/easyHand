//   
//   OdbcScroll
//   
void * OdbcScroll(EH_OBJPARAMS);
// #define _adaptor(obj,mess,info,psExt,pRes) obj, (BOOL *) psExt,EXT_CALL,NULL,mess,info,(LONG) pRes 
#define _odbcScrollAdaptor_ EH_DISPEXT * psExt=(EH_DISPEXT *) pbReturn; SQL_RS rsSet=(SQL_RS ) lParam; EN_MESSAGE enMess=message;
