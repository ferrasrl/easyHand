//-----------------------------------------------------------------------
//	FormulaProcess  Trasforma la formula in un numero                   |
//											Creato da G.Tassistro       |
//											Ferrà Art & Tecnology 1999  |
//  Ritorna TRUE = Formula errata                                       |
//          FALSE= Formula OK Valore messo in "Valore"                  |
//-----------------------------------------------------------------------
#define FORMULA_FUNC_EXT BOOL (*funcExt)(EN_MESSAGE,SINT,CHAR*,void*)
BOOL	FormulaProcess(CHAR * lpFormula,double * pdValore,BOOL bViewError,FORMULA_FUNC_EXT);
void	FormulaSetNotify(void (*NewFunction)(SINT iMode,CHAR *pMess,...));
void	FormulaDebug(BOOL);
EH_AR	getFuncArg(BYTE *pString,BYTE *pCharStart,BYTE *pCharStop,SINT *piError,SINT *piArgNum); // new 2009
