// --------------------------------------
// CGIUTIL   
// by Ferra Art & Technology
// --------------------------------------

// typedef signed char BOOL;
// typedef char  TCHAR;
// #define FALSE 0
// #define TRUE  1

//typedef signed int  SINT;

void  OutputFile(char *Html);
CHAR *Snummask(double Numer,SINT numcif,SINT dec,SINT sep,SINT segno);
char  *HTMLLoadFile(char *FileName);
int   ChangeVar(char *Html,char *Nome,char *String);
void ShowCGI(FORM_ENTRY_PTR parms);
