
BOOL BarCode_EAN8 (CHAR *lpDataToEncode, CHAR *lpOutput, LONG *lpiSize);
BOOL BarCode_EAN13 (CHAR *lpDataToEncode, CHAR *lpOutput, LONG *lpiSize);
BOOL BarCode_Code39 (char *DataToEncode, char *output, long *iSize);
/*

#ifdef __cplusplus
extern "C" {
#endif

long __stdcall IDAutomation_UPCe (char *pcDataToEncode, char *szReturnVal, long *iSize);
long __stdcall IDAutomation_Codabar (char *pcDataToEncode, char *szReturnVal, long *iSize);
long __stdcall IDAutomation_Code128a (char *pcDataToEncode, char *szReturnVal, long *iSize);
long __stdcall IDAutomation_Code128b (char *pcDataToEncode, char *szReturnVal, long *iSize);
long __stdcall IDAutomation_Code128c (char *pcDataToEncode, char *szReturnVal, long *iSize);
long __stdcall IDAutomation_Interleaved2of5 (char *pcDataToEncode, char *szReturnVal, long *iSize);
long __stdcall IDAutomation_Interleaved2of5Mod10 (char *pcDataToEncode, char *szReturnVal, long *iSize);
long __stdcall IDAutomation_Code39 (char *pcDataToEncode, char *szReturnVal, long *iSize);
long __stdcall IDAutomation_Code39Mod43 (char *pcDataToEncode, char *szReturnVal, long *iSize);
long __stdcall IDAutomation_EAN8 (char *pcDataToEncode, char *szReturnVal, long *iSize);
long __stdcall IDAutomation_EAN13 (char *pcDataToEncode, char *szReturnVal, long *iSize);
long __stdcall IDAutomation_UPCa (char *pcDataToEncode, char *szReturnVal, long *iSize);
long __stdcall IDAutomation_MSI (char *pcDataToEncode, char *szReturnVal, long *iSize);
long __stdcall IDAutomation_Postnet (char *pcDataToEncode, char *szReturnVal, long *iSize);
long __stdcall IDAutomation_Code128 (char *DataToEncode, long *ApplyTilde, char *szReturnVal, long *iSize);
long __stdcall IDAutomation_Code128HumanReadable (char *DataToEncode, long *ApplyTilde, char *szReturnVal, long *iSize);
long __stdcall IDAutomation_UCC128 (char *DataToEncode, char *szReturnVal, long *iSize);

#ifdef __cplusplus
}
#endif
*/