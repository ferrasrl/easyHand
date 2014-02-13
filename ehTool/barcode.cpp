/*********************************************************************/
/*                                                                   */
/*  C++ Functions for Bar Code Fonts 5.09                            */
/*                                                                   */
/*  Copyright, IDAutomation.com, Inc. 2005. All rights reserved.     */
/*                                                                   */
/*  Visit http://www.IDAutomation.com  for more information.         */
/*                                                                   */
/*  You may incorporate our Source Code in your application          */
/*  only if you own a valid Multi-user or Developer                  */
/*  license from IDAutomation.com, Inc. for the associated font and  */
/*  the copyright notices are not removed from the source code.      */
/*                                                                   */
/*  C++ users can add this library to their C++ applications         */
/*  and access the functions directly, for example:                  */
/*  Code128b("abc 123-45");                                          */
/*                                                                   */
/*  Distributing our source code or fonts outside your               */
/*  organization requires a distribution license.                    */
/*********************************************************************/

#include "\ehtool\include\ehsw_idb.h"
#include "\ehtool\barcode.h"

static  int  StrCnt;
static  char StrFunction [64][257];

/* ---------------------- */
/*  Standard Prototypes   */
/*  Private functions	  */
/* ---------------------- */
extern int  asc (char *);
extern char *mid (char *, int, int);
char FindMod10Digit(char *);
char *CalcMSICheckDigit(char *);

/* ------------------- */
/*  User's Prototypes  
	Public Functions for exporting */
/* ------------------- */

/* The function defined above will take three parameters:

    * pcDataToEncode - The name of the domain you are inquiring about.
    * szReturnVal -The variable the encoded data will be returned in.
    * iSize - The length of the encoded data being returned without the padding. */


/* --------------------- */
/*  Run Time Functions   */
/* --------------------- */

extern char *mid (char *S, int start, int length)
{
	if(++StrCnt==64) 
	{
		StrCnt = 0;
	}
	
	if (start > (int) strlen(S))
	{ 
		StrFunction[StrCnt][0]='\0'; 
	}
	else
	{
		strncpy (StrFunction[StrCnt], &S [start-1], length);
	}

	StrFunction[StrCnt][length]='\0';
	return StrFunction[StrCnt];
}

extern int asc(char *z)
{
	static int q;
	q = 0;
	memmove(&q,z,1);
	return q;
}

/* ---------------------- */
/*  User Subs/Functions   */
/* ---------------------- */
long __stdcall IDAutomation_Code128a(char *DataToEncode,  char *output, long *iSize)
{
	if(DataToEncode == NULL)
		return 1;
	if(strlen(DataToEncode) == 0)
		return 1;
	
	char *DataToPrint;
	DataToPrint = new char[512];

	char *PrintableString;
	PrintableString = new char[512];

	int  C128Start;
	int  C128StartA;
	int  C128StartB;
	int  C128StartC;
	int  C128Stop;
	int  weightedTotal;
	int  I;
	int  CurrentChar;
	int  CurrentValue;
	int  CheckDigitValue;
	int  CheckDigit;
	char C128CheckDigit;
	char C128CurrentChar;
	int BufferCounter = 0;

	C128StartA=203;
	C128StartB=204;
	C128StartC=205;
	C128Stop=206;

	/* Here we select character set A */
	C128Start=C128StartA;

	/* <<<< Calculate Modulo 103 Check Digit >>>> */
	/* Set WeightedTotal to the value of the start character */
	weightedTotal=C128Start-100;

	for(I=1;I <= (int)strlen(DataToEncode);I++)
	{
		/* Get the ASCII value of each character */
		CurrentChar=(asc(mid(DataToEncode,I,1)));

		/* Get the Code 128 value of CurrentChar according to chart */
  		if(CurrentChar<135) {CurrentValue=CurrentChar-32;}
  		if(CurrentChar>134) {CurrentValue=CurrentChar-100;}

		/* Multiply by the weighting character */
  		CurrentValue=CurrentValue*I;

		/* Add the values together to get the weighted total*/
  		weightedTotal=weightedTotal+CurrentValue;
	}

	/* divide the WeightedTotal by 103 and get the remainder, this is the CheckDigitValue*/
	CheckDigitValue = (int) (weightedTotal % 103);

	/* Now that we have the CheckDigitValue, find the corresponding ASCII character from the table */
	if(CheckDigitValue < 95 && CheckDigitValue > 0)
	{
		CheckDigit = CheckDigitValue + 32;
	}
	if(CheckDigitValue > 94)
	{ 
		CheckDigit = CheckDigitValue + 100;
	}
	if(CheckDigitValue == 0)
	{
		CheckDigit = 194;
	}
	C128CheckDigit=CheckDigit;

	/* Check for spaces or "00" and print ASCII 194 instead */
	/* place changes in DataToPrint */
	for(I=1;I <= (int)strlen(DataToEncode);I++)
	{
		C128CurrentChar = DataToEncode[I-1];
		if(C128CurrentChar==' ') 
		{
			C128CurrentChar=(char)194;
		}
		BufferCounter += sprintf(DataToPrint + BufferCounter, "%c", DataToEncode[I-1]);
		//strcat(DataToPrint,&C128CurrentChar);
	}

	/* Get PrintableString */
	sprintf(PrintableString,"%c%s%c%c",C128Start,DataToPrint,C128CheckDigit,C128Stop);

	// Allow the string to return to the proper size.
	*iSize = (long)strlen(PrintableString); 
	strncpy(output, PrintableString, strlen(PrintableString)); 
	delete [] DataToPrint;
	delete [] PrintableString;
	return 0;
} //end code128A()

long __stdcall IDAutomation_Code128b(char *DataToEncode, char *output, long *iSize)
{
	if(DataToEncode == NULL)
		return 1;
	if(strlen(DataToEncode) == 0)
		return 1;
	
	char *DataToPrint;
	DataToPrint = new char[512];

	char *PrintableString;
	PrintableString = new char[512];

	int BufferCounter = 0;
	int  C128Start;
	int  C128StartA;
	int  C128StartB;
	int  C128StartC;
	int  C128Stop;
	int  weightedTotal;
	int  I;
	int  CurrentChar;
	int  CurrentValue;
	int  CheckDigitValue;
	int  CheckDigit;
	char C128CheckDigit;
	char C128CurrentChar;

	C128StartA=203;
	C128StartB=204;
	C128StartC=205;
	C128Stop=206;

	/* Here we select character set A */
	C128Start=C128StartB;

	/* <<<< Calculate Modulo 103 Check Digit >>>> */
	/* Set WeightedTotal to the value of the start character */
	weightedTotal=C128Start-100;

	for(I=1;I <= (int)strlen(DataToEncode);I++)
	{
		/* Get the ASCII value of each character */
		CurrentChar=(asc(mid(DataToEncode,I,1)));

		/* Get the Code 128 value of CurrentChar according to chart */
		if(CurrentChar<135) {CurrentValue=CurrentChar-32;}
		if(CurrentChar>134) {CurrentValue=CurrentChar-100;}

		/* Multiply by the weighting character */
		CurrentValue=CurrentValue*I;

		/* Add the values together */
		weightedTotal=weightedTotal+CurrentValue;
	}

	/* divide the WeightedTotal by 103 and get the remainder, this is the CheckDigitValue	*/
	CheckDigitValue = (int) (weightedTotal % 103);

	/* Now that we have the CheckDigitValue, find the corresponding ASCII character
	from the table */
	if(CheckDigitValue<95 && CheckDigitValue>0) {CheckDigit=CheckDigitValue+32;}
	if(CheckDigitValue>94) {CheckDigit=CheckDigitValue+100;}
	if(CheckDigitValue==0) {CheckDigit=194;}
	C128CheckDigit=CheckDigit;

	/* Check for spaces or "00" and print ASCII 194 instead */
	/* place changes in DataToPrint */
	BufferCounter = 0;
	for(I=1;I <= (int)strlen(DataToEncode);I++)
	{
		C128CurrentChar=DataToEncode[I-1];
		if(C128CurrentChar==' ') 
		{
			C128CurrentChar = (char)194;
		}
		BufferCounter += sprintf(DataToPrint + BufferCounter, "%c", C128CurrentChar);
	}

	/* Get PrintableString */
	sprintf(PrintableString,"%c%s%c%c",C128Start, DataToPrint,C128CheckDigit,C128Stop);

	// Allow the string to return to the proper size.
	*iSize = (long)strlen(PrintableString); 
	strncpy(output, PrintableString, strlen(PrintableString)); 
	delete [] DataToPrint;
	delete [] PrintableString;
	return 0;
} //end Code128B()

long __stdcall IDAutomation_Code128c(char *DataToEncode, char *output, long *iSize)
{
	if(DataToEncode == NULL)
		return 1;
	if(strlen(DataToEncode) == 0)
		return 1;
	
	char *DataToPrint;
	DataToPrint = new char[512];

	char *PrintableString;
	PrintableString = new char[512];

	char *OnlyCorrectData;
	OnlyCorrectData = new char[512];

	char *TempOnlyCorrectData;
	TempOnlyCorrectData = new char[512];

	int  C128Start;
	int  C128Stop;
	int  weightedTotal;
	int  WeightValue;
	int  I;
	int  CurrentValue;
	int  CheckDigitValue;
	char C128CheckDigit;
	int PrintBuffer = 0;
	/* Check to make sure data is numeric and remove dashes, etc. */
	for(I=1;I <= (int)strlen(DataToEncode);I++)
	{
		/* Add all numbers to OnlyCorrectData string */
		if(isdigit(asc(mid(DataToEncode,I,1))) != 0) 
		{
			PrintBuffer += sprintf(TempOnlyCorrectData + PrintBuffer, "%s", mid(DataToEncode,I,1));
		}
	}

	//DataToEncode = OnlyCorrectData;

	/* Check for an even number of digits, add 0 if not even */
	int rem = (int) (strlen(TempOnlyCorrectData) % 2);
	if(rem == 1)	
	{
		sprintf(OnlyCorrectData, "0%s", TempOnlyCorrectData);
	}
	else
		sprintf(OnlyCorrectData, "%s", TempOnlyCorrectData);

	/* Assign start & stop codes */
	C128Start=205;
	C128Stop=206;

	/* <<<< Calculate Modulo 103 Check Digit and generate DataToPrint >>>> */
	/* Set WeightedTotal to the Code 128 value of the start character */
	weightedTotal=105;
	WeightValue = 1;
	PrintBuffer = 0;
	for(I=1;I <= (int)strlen(OnlyCorrectData);I = I + 2)
	{
		/* Get the value of each number pair */
		CurrentValue = atoi(mid(OnlyCorrectData, I, 2));

		/* Get the DataToPrint */
		if(CurrentValue < 95 && CurrentValue > 0) 
		{
			PrintBuffer += sprintf(DataToPrint + PrintBuffer,"%c", CurrentValue + 32);
		}
		if(CurrentValue>94) 
		{
			PrintBuffer += sprintf(DataToPrint + PrintBuffer,"%c", CurrentValue + 100);
		}
		if(CurrentValue==0) 
		{
			PrintBuffer += sprintf(DataToPrint + PrintBuffer,"%c", (char)194);
		}

		/* Multiply by the weighting character */
		CurrentValue=CurrentValue * WeightValue;

		/* Add the values together to get the weighted total*/
		weightedTotal = weightedTotal + CurrentValue;
		WeightValue = WeightValue + 1;
	}

	/* Divide the weighted total by 103 and get the remainder, this is the CheckDigitValue
	*/
	CheckDigitValue = (int)(weightedTotal % 103);

	/* Now that we have the CheckDigitValue, find the corresponding ASCII character
	from the table */
	if (CheckDigitValue<95 && CheckDigitValue>0) 
	{
		C128CheckDigit=CheckDigitValue+32;
	}
	if(CheckDigitValue>94)
	{
		C128CheckDigit=CheckDigitValue+100;
	}
	if (CheckDigitValue==0) 
	{
		C128CheckDigit = (char)194;
	}

	/* Get PrintableString */
	sprintf(PrintableString,"%c%s%c%c",C128Start,DataToPrint,C128CheckDigit,C128Stop);

	// Allow the string to return to the proper size.
	*iSize = (long)strlen(PrintableString); 
	strncpy(output, PrintableString, strlen(PrintableString)); 
	delete [] DataToPrint;
	delete [] OnlyCorrectData;
	delete [] PrintableString;
	delete [] TempOnlyCorrectData;
	return 0;
} //end Code128C()

long __stdcall IDAutomation_Codabar(char *DataToEncode, char *output, long *iSize)
{
	if(DataToEncode == NULL)
		return 1;
	if(strlen(DataToEncode) == 0)
		return 1;

	char *OnlyCorrectData;
	OnlyCorrectData = new char[512];

	char *PrintableString;
	PrintableString = new char[512];
	
	int I;
	int PrintBuffer = 0;
	/* Check to make sure data is numeric, $, +, -, /, or :, and remove all others. */
	for(I = 1;I <= (int)strlen(DataToEncode);I++)
	{
		if((int) asc(mid(DataToEncode, I, 1)) > 0 && isdigit(asc(mid(DataToEncode,I,1))))
			PrintBuffer += sprintf(OnlyCorrectData + PrintBuffer, "%s", mid(DataToEncode, I, 1));
		else if(DataToEncode[I - 1] == '$' || DataToEncode[I - 1] == '+' || DataToEncode[I - 1] == '-' || 
			DataToEncode[I - 1] == '/' || DataToEncode[I - 1] == '.' || DataToEncode[I - 1] == ':')
			PrintBuffer += sprintf(OnlyCorrectData + PrintBuffer, "%s", mid(DataToEncode,I,1));
	}

	/* Get Printable String */
	sprintf(PrintableString,"A%sB", OnlyCorrectData);

	// Allow the string to return to the proper size.
	*iSize = (long)strlen(PrintableString); 
	strncpy(output, PrintableString, strlen(PrintableString)); 

	delete [] OnlyCorrectData;
	delete [] PrintableString;
	
	return 0;
} //end UniversalCodabar()

long __stdcall IDAutomation_Interleaved2of5(char *DataToEncode, char *output, long *iSize)
{
	if(DataToEncode == NULL)
		return 1;
	if(strlen(DataToEncode) == 0)
		return 1;
	
	char *PrintableString;
	PrintableString = new char[512];

	char *DataToPrint;
	DataToPrint = new char[512];

	char *OnlyCorrectData;
	OnlyCorrectData = new char[512];

	char *TempString;
	TempString = new char[512];

	int I;
	int CurrentNumberPair = 0;
	char StartCode;
	char StopCode;
	int BufferCounter = 0;
	
	/* Check to make sure data is numeric and remove dashes, etc. */
	for(I=1;I <= (int)strlen(DataToEncode);I++)
	{
		/* Add all numbers to OnlyCorrectData string */
		if(isdigit(asc(mid(DataToEncode,I,1))))
		{
			BufferCounter += sprintf(OnlyCorrectData + BufferCounter,"%c", asc(mid(DataToEncode,I,1)));
		}
	}	

	BufferCounter = 0;
	
	/* Check for an even number of digits, add 0 if not even */
	if((int)(strlen(OnlyCorrectData) % 2) == 1)
	{
		sprintf(TempString,"%d%s",0, OnlyCorrectData);
	}
	else
		sprintf(TempString,"%s",OnlyCorrectData);

	/* Assign start and stop codes */
	StartCode = (char)203;
	StopCode = (char)204;
	for(I=1;I <= (int)strlen(TempString);I = I + 2)
	{
		/* Get the value of each number pair */
		CurrentNumberPair = atoi(mid(TempString, I, 2));
		/* == Get the ASCII value of CurrentChar according to chart by adding to the value == */
		if(CurrentNumberPair < 94) 
		{
			BufferCounter += sprintf(DataToPrint + BufferCounter,"%c", (char)(CurrentNumberPair + 33));
		}
		if(CurrentNumberPair > 93)
		{
			BufferCounter += sprintf(DataToPrint + BufferCounter,"%c", (char)(CurrentNumberPair + 103));
		}
	}

	/* Get Printable String */
	sprintf(PrintableString,"%c%s%c", StartCode, DataToPrint, StopCode);

	// Allow the string to return to the proper size.
	*iSize = (long)strlen(PrintableString); 
	strncpy(output, PrintableString, strlen(PrintableString)); 
	delete [] TempString;
	delete [] OnlyCorrectData;
	delete [] PrintableString;
	delete [] DataToPrint;
	
	return 0;
} //end Interleaved2of5()

long __stdcall IDAutomation_Interleaved2of5Mod10 (char *DataToEncode, char *output, long *iSize)
{
	if(DataToEncode == NULL)
		return 1;
	if(strlen(DataToEncode) == 0)
		return 1;
	
	char *DataToEncode2;
	DataToEncode2 = new char[512];
	
	char *PrintableString;
	PrintableString = new char[512];

	char *OnlyCorrectData;
	OnlyCorrectData = new char[512];

	int BufferCounter = 0;
	char StartCode;
	char StopCode;
	int I = 0;
	int CurrentNumberPair = 0;
	
	/* Check to make sure data is numeric and remove dashes, etc. */
	for(I=1;I <= (int)strlen(DataToEncode);I++)
	{
		/* Add all numbers to OnlyCorrectData string */
		if(isdigit(asc(mid(DataToEncode,I,1))))
		{
			BufferCounter += sprintf(OnlyCorrectData + BufferCounter, "%s", mid(DataToEncode, I, 1));
		}
	}

	//now that we are done sorting out the data, get the check character
	BufferCounter += sprintf(OnlyCorrectData + BufferCounter, "%c", FindMod10Digit(OnlyCorrectData));

	/* Check for an even number of digits, add 0 if not even */
	if((int)(strlen(OnlyCorrectData) % 2) == 1)
		sprintf(DataToEncode2, "0%s", OnlyCorrectData);
	else	
		sprintf(DataToEncode2, "%s", OnlyCorrectData);
 

	/* Assign start and stop codes */
	StartCode = (char)203;
	StopCode = (char)204;
	BufferCounter = 0;
	BufferCounter += sprintf(PrintableString + BufferCounter, "%c", StartCode);
	for(I = 1;I <= (int)strlen(DataToEncode2);I = I + 2)
	{
		CurrentNumberPair = atoi(mid(DataToEncode2, I, 2));
		if(CurrentNumberPair < 94)
			BufferCounter += sprintf(PrintableString + BufferCounter, "%c", (char)(CurrentNumberPair + 33));
		else
			BufferCounter += sprintf(PrintableString + BufferCounter, "%c", (char)(CurrentNumberPair + 103));
	}

	BufferCounter += sprintf(PrintableString + BufferCounter, "%c", StopCode);

	// Allow the string to return to the proper size.
	*iSize = (long)strlen(PrintableString); 
	strncpy(output, PrintableString, strlen(PrintableString)); 
	delete [] DataToEncode2;
	delete [] OnlyCorrectData;
	delete [] PrintableString;
	
	return 0;
} //end Interleaved2of5Mod10()


long __stdcall IDAutomation_Code39 (char *DataToEncode, char *output, long *iSize)
{
	//If DataToEncode is empty, return a non-zero value
	if(DataToEncode == NULL)
		return 1;
	if(strlen(DataToEncode) == 0)
		return 1;

	char *DataToPrint;
	DataToPrint = new char[512];
	
	char *PrintableString;
	PrintableString = new char[512];

	int I;
	int CurrentChar;
	int BufferCounter = 0;
	/* Check for spaces in code */
	for(I=1;I <= (int)strlen(DataToEncode);I++)
	{
		/* Get each character one at a time */
		CurrentChar = asc((mid(DataToEncode,I,1)));
		/* To print the barcode symbol representing a space you will */
		/* to type or print "=" (the equal character) instead of a space character.*/
		if(CurrentChar == ' ')
		{
			CurrentChar = '=';
		}
		BufferCounter += sprintf(DataToPrint + BufferCounter,"%c", CurrentChar);
	}
	
	/* Get Printable String */
	sprintf(PrintableString,"*%s*",strupr(DataToPrint));
	
	// Allow the string to return to the proper size.
	*iSize = (long) strlen(PrintableString); 
	strcpy(output, PrintableString); 

	delete [] DataToPrint;
	delete [] PrintableString;
	
	return 0;
}
long __stdcall IDAutomation_Code39Mod43 (char *DataToEncode, char *output, long *iSize)
{
	//If DataToEncode is empty, return a non-zero value
	if(DataToEncode == NULL)
		return 1;
	if(strlen(DataToEncode) == 0)
		return 1;
	
	char *DataToEncode2;
	DataToEncode2 = new char[512];

	char *DataToPrint;
	DataToPrint = new char[512];

	char *PrintableString;
	PrintableString = new char[512];

	int I;
	int weightedTotal;
	int	CurrentChar;			//ascii value of current character
	char CurrentValue;
	char CheckDigit;
	char CheckDigitValue;
	int bufferCounter = 0;

	/* Get data from user, this is the DataToEncode */
	sprintf(DataToEncode2,"%s",strupr(DataToEncode));
	weightedTotal = 0;
	/* only pass correct data */
	for(I = 1;I <= (int)strlen(DataToEncode2);I++)
	{
		/* Get each character one at a time */
		CurrentChar = asc((mid(DataToEncode2,I,1)));
		/* Get the value of CurrentChar according to MOD43 */
		/* 0-9 */
		if(CurrentChar < 58 && CurrentChar > 47) //1-9 
		{
			bufferCounter += sprintf(DataToPrint + bufferCounter, "%s", mid(DataToEncode2,I,1));
			CurrentValue = CurrentChar - 48;
		}
		else if(CurrentChar < 91 && CurrentChar > 64)   //A-Z
		{
			bufferCounter += sprintf(DataToPrint + bufferCounter, "%s", mid(DataToEncode2,I,1));
			CurrentValue = CurrentChar - 55;
		}
		else if(CurrentChar == 32) 		/* Space */
		{
			bufferCounter += sprintf(DataToPrint + bufferCounter, "%s", "=");
			CurrentValue = 38;
		}
		else if(CurrentChar == 45) /* - */
		{
			bufferCounter += sprintf(DataToPrint + bufferCounter, "%s", mid(DataToEncode2,I,1));
			CurrentValue = 36;
		}
		else if(CurrentChar == 46) /* . */
		{
			bufferCounter += sprintf(DataToPrint + bufferCounter, "%s", mid(DataToEncode2,I,1));
			CurrentValue = 37;
		}
		else if(CurrentChar == 36) /* $ */
		{
			bufferCounter += sprintf(DataToPrint + bufferCounter, "%s", mid(DataToEncode2,I,1));
			CurrentValue = 39;
		}
		else if(CurrentChar == 47) /* / */
		{
			bufferCounter += sprintf(DataToPrint + bufferCounter, "%s", mid(DataToEncode2,I,1));
			CurrentValue = 40;
		}
		else if(CurrentChar == 43) /* + */
		{
			bufferCounter += sprintf(DataToPrint + bufferCounter, "%s", mid(DataToEncode2,I,1));
			CurrentValue = 41;
		}
		else if(CurrentChar == 37) /* % */
		{
			bufferCounter += sprintf(DataToPrint + bufferCounter, "%s", mid(DataToEncode2,I,1));
			CurrentValue = 42;
		}
		else
		{
			//Invalid Character for Code 39 
			CurrentValue = 0;
		}
		/* add the values together */
		weightedTotal = weightedTotal + CurrentValue;
	}//end for loop
	
	/* divide the WeightedTotal by 43 and get the remainder, this is the CheckDigit	*/
	CheckDigitValue = (int)((weightedTotal % 43));

	/* Assign values to characters */
	if(CheckDigitValue < 10) /* 0-9 */
	{
		CheckDigit = CheckDigitValue + 48;
	}	
	else if((CheckDigitValue < 36) && (CheckDigitValue > 9)) /* A-Z */
	{
		CheckDigit = CheckDigitValue + 55;
	}
	else if(CheckDigitValue == 38) /* Space */
	{
		CheckDigit = 61;
	}	
	else if(CheckDigitValue == 36) /* - */
	{
		CheckDigit = 45;
	}		
	else if(CheckDigitValue == 37)    /* . */
	{
		CheckDigit = 46;
	}	
	else if(CheckDigitValue == 39)	/* $ */
	{
		CheckDigit = 36;
	}
	else if(CheckDigitValue == 40)	/* / */
	{
		CheckDigit = 47;
	}	
	else if(CheckDigitValue == 41)	/* + */
	{
		CheckDigit = 43;
	}	
	else if(CheckDigitValue == 42)	/* % */
	{
		CheckDigit = 37;
	}	
	
	/* Get Printable String */
	sprintf(PrintableString,"*%s%c*", DataToPrint, CheckDigit);
	
	// Allow the string to return to the proper size.
	*iSize = (long)strlen(PrintableString); 
	strncpy(output, PrintableString, strlen(PrintableString)); 
	delete [] DataToEncode2;
	delete [] DataToPrint;
	delete [] PrintableString;
	
	return 0;
}
long __stdcall IDAutomation_Postnet (char *DataToEncode, char *output, long *iSize)
{
	/* Enter all the numbers without dashes */
	if(DataToEncode == NULL)
		return 1;
	if(strlen(DataToEncode) == 0)
		return 1;
	
	char *OnlyCorrectData;
	OnlyCorrectData = new char[512];

	char *PrintableString;
	PrintableString = new char[512];
	
	int BufferCounter = 0;
	int  I;
	int  weightedTotal;
	int  CheckDigit;

	/* Check to make sure data is numeric and remove dashes, etc. */
	for(I=1;I <= (int)strlen(DataToEncode);I++)
	{
		/* Add all numbers to OnlyCorrectData string */
		if(isdigit(asc(mid(DataToEncode,I,1))))
			BufferCounter += sprintf(OnlyCorrectData + BufferCounter,"%s", mid(DataToEncode,I,1));
	}

	/* <<<< Calculate Check Digit >>>>.  which is just the sum of the numbers */
	weightedTotal=0;

	for(I = 1;I <= (int)strlen(OnlyCorrectData);I++)
	{		
		weightedTotal += atoi(mid(OnlyCorrectData, I, 1));
	}

	/* Find the CheckDigit by finding the number + weightedTotal that = a multiple of 10 */
	/* divide by 10, get the remainder and subtract from 10 */
	I = (int)((weightedTotal % 10));
	if(	I!= 0)
		CheckDigit=(10-I);
	else
		CheckDigit=0;
	
	BufferCounter = 0;
	BufferCounter += sprintf(PrintableString + BufferCounter,"(%s%i)", OnlyCorrectData, CheckDigit);
	
	// Allow the string to return to the proper size.
	*iSize = (long) strlen(PrintableString); 
	strcpy(output, PrintableString);
	
	delete [] PrintableString;
	delete [] OnlyCorrectData;
	return 0;
} //end Postnet()

long __stdcall IDAutomation_EAN8 (char *DataToEncode, char *output, long *iSize)
{
	/* The purpose of this code is to calculate the EAN-8 barcode */
	/* Enter all the numbers without dashes */
	if(DataToEncode == NULL)
		return 1;
	if(strlen(DataToEncode) == 0)
		return 1;
	
	char *DataToEncode2;
	DataToEncode2 = new char[512];

	char *DataToPrint;
	DataToPrint = new char[512];

	char *OnlyCorrectData;
	OnlyCorrectData = new char[512];

	char *PrintableString;
	PrintableString = new char[512];

	int  I;
	int  Factor;
	int  weightedTotal;
	char CurrentChar;
	int  CheckDigit;
	int PrintBuffer = 0;
	sprintf(DataToEncode2,"%s",DataToEncode);

	/* Check to make sure data is numeric and remove dashes, etc. */
	for(I=1;I <= (int)strlen(DataToEncode2);I++)
	{
		/* Add all numbers to OnlyCorrectData string */
		if(isdigit(asc(mid(DataToEncode2,I,1))))
		{
			PrintBuffer += sprintf(OnlyCorrectData + PrintBuffer,"%c", asc(mid(DataToEncode2,I,1)));
		}
	}

	if(strlen(OnlyCorrectData) >= 7)
		sprintf(DataToEncode2,"%s",mid(OnlyCorrectData, 1, 7));
	else
		sprintf(DataToEncode2,"%s","0005000");
	
	/* <<<< Calculate Check Digit >>>> */
	Factor=3;
	weightedTotal=0;
	for(I = (int)strlen(DataToEncode2);I >= 1;I = I +- 1)
	{
		/* Get the value of each number starting at the end */
		CurrentChar=asc(mid(DataToEncode2,I,1));
		/* multiply by the weighting factor which is 3,1,3,1... */
		/* and add the sum together */
		weightedTotal=weightedTotal+CurrentChar*Factor;
		/* change factor for next calculation */
		Factor=4-Factor;
	}

	/* Find the CheckDigit by finding the number + weightedTotal that = a multiple of 10 */
	/* divide by 10, get the remainder and subtract from 10 */
	I = (int)(weightedTotal % 10);
	if(I!=0)
		CheckDigit=(10-I);
	else
		CheckDigit=0;

	sprintf(DataToEncode2,"%s%i", DataToEncode2, CheckDigit);

	/* Now that have the total number including the check digit, determine character to print */
	/* for proper barcoding */
	PrintBuffer = 0;
	for(I = 1;I <= (int)strlen(DataToEncode2);I++)
	{

		/* Get the ASCII value of each number */
		CurrentChar = asc(mid(DataToEncode2,I,1));

		/* Print different barcodes according to the location of the CurrentChar and CurrentEncoding */
		if(I==1)
		{
			/* For the first character print the normal guard pattern */
			/* and then the barcode without the human readable character */
			PrintBuffer += sprintf(DataToPrint + PrintBuffer,"(%c",CurrentChar);
		}
		if(I==2)
		{
			PrintBuffer += sprintf(DataToPrint + PrintBuffer,"%c", CurrentChar);			 
		}
		if(I==3)
		{
			PrintBuffer += sprintf(DataToPrint + PrintBuffer,"%c", CurrentChar);			 
		}
		if(I==4)
		{
			/* Print the center guard pattern after the 6th character */
			//DataToPrint=DataToPrint&Chr(CurrentChar)&"*";
			PrintBuffer += sprintf(DataToPrint + PrintBuffer,"%c*", CurrentChar);			 
		}
		if(I==5)
		{
			PrintBuffer += sprintf(DataToPrint + PrintBuffer,"%c", CurrentChar+27);			 
		}
		if(I==6)
		{
			PrintBuffer += sprintf(DataToPrint + PrintBuffer,"%c", CurrentChar+27);			 
		}
		if(I==7)
		{
			PrintBuffer += sprintf(DataToPrint + PrintBuffer,"%c", CurrentChar+27);			 
		}
		if(I==8)
		{
			/* Print the check digit as 8th character + normal guard pattern */
			PrintBuffer += sprintf(DataToPrint + PrintBuffer,"%c(", CurrentChar+27);
		}		 
	}

	/* Get Printable String */
	sprintf(PrintableString,"%s",DataToPrint);
	
	// Allow the string to return to the proper size.
	*iSize = (long)strlen(PrintableString); 
	strncpy(output, PrintableString, strlen(PrintableString)); 
	delete [] DataToEncode2;
	delete [] DataToPrint;
	delete [] OnlyCorrectData;
	delete [] PrintableString;
	
	return 0;
}//end ean8()


long __stdcall IDAutomation_EAN13 (char *DataToEncode, char *output, long *iSize)
{
	if(DataToEncode == NULL)
		return 1;
	if(strlen(DataToEncode) == 0)
		return 1;
	
	int StringLength = 0;
	int I = 0;					//for loop counter
	int Factor = 3;				//weighting factor
	int weightedTotal = 0;		//The weighted Totals
	int CurrentChar = 0;		//The ascii value of the current character
	int DataToPrintBuffer = 0;	//Buffer for using sprintf
	int CheckDigit = 0;			//the value of the check digit
	int LeadingDigit = 0;		//the value of the first character in the ean data
	int CurrentEncoding = 0;	//the ASCII value of the current character in the encoding parity string 

	char* LPrintableString;		//our output string..DataToPrint + EANAddOnToPrint
	LPrintableString = new char[512];

	char* ActualDataToEncode;	//after all conversions and substitutions to OnlyCorrectData.  This is UPCE compressed data
	ActualDataToEncode = new char[20];

	char* DataToPrint;			//encoded data before adding start/stop/supplements
	DataToPrint = new char[20];
	
	char* EAN2AddOn;			//2 digit EAN add on
	EAN2AddOn = new char[3];
	
	char* OnlyCorrectData;		//after filtering out any non numerics from DataTOEncode
	OnlyCorrectData = new char[20];

    char* EAN5AddOn;			//5 digit EAN add on
	EAN5AddOn = new char[6];

    char* EANAddOnToPrint;		//After the EAN value is encoded
	EANAddOnToPrint = new char[15];

	char* Encoding;				//The encoding parity 
	Encoding = new char[20];

	char* Temp;					//used as placeholder for encoding
	Temp = new char[512];
	//Add all numbers to OnlyCorrectData string
	StringLength = (int)strlen(DataToEncode);
	for(I = 0;I < StringLength;I++)
	{		
		if(isdigit((int)DataToEncode[I]))
		{
			DataToPrintBuffer += sprintf(OnlyCorrectData + DataToPrintBuffer, "%c", DataToEncode[I]);
		}
	}

	DataToPrintBuffer = 0;
	if(strlen(OnlyCorrectData) < 12 || strlen(OnlyCorrectData) == 16 || strlen(OnlyCorrectData) > 18)
	{
		sprintf(OnlyCorrectData, "%s", "0000000000000");
	}
	
	//The 13th character will be the check digit
	if(strlen(OnlyCorrectData) == 12 || strlen(OnlyCorrectData) == 13)
	{
		sprintf(LPrintableString, "%s", mid(OnlyCorrectData, 1, 12));
	}
	else if(strlen(OnlyCorrectData) == 14)
	{
		sprintf(LPrintableString,"%s", mid(OnlyCorrectData, 1, 12));
		sprintf(EAN2AddOn,"%s", mid(OnlyCorrectData, 13, 2));
	}
	else if(strlen(OnlyCorrectData) == 15)
	{
		sprintf(LPrintableString,"%s", mid(OnlyCorrectData, 1, 12));
		sprintf(EAN2AddOn,"%s", mid(OnlyCorrectData, 14, 2));
	}
	else if(strlen(OnlyCorrectData) == 17)
	{
		sprintf(LPrintableString,"%s", mid(OnlyCorrectData, 1, 12));
		sprintf(EAN5AddOn,"%s", mid(OnlyCorrectData, 13, 5));
	}
	else if(strlen(OnlyCorrectData) == 18)
	{
		sprintf(LPrintableString,"%s", mid(OnlyCorrectData, 1, 12));
		sprintf(EAN5AddOn,"%s", mid(OnlyCorrectData, 14, 5));
	}
	
	
	//Get the check digit for our 12 characters and determine what encoding we need to use
	/* <<<< Calculate Check Digit >>>> */
	for(I = (int)strlen(LPrintableString);I >= 1;I--)
	{
		/* Get the value of each number starting at the end */
		CurrentChar = asc(mid(LPrintableString,I,1)) - 48;
		/* multiply by the weighting factor which is 3,1,3,1... */
		/* and add the sum together */
		weightedTotal = weightedTotal + (CurrentChar * Factor);
		/* change factor for next calculation */
		Factor = 4 - Factor;
	}

	/* Find the CheckDigitValue by finding the number + weightedTotal that = a multiple of 10 */
	/* divide by 10, get the remainder and subtract from 10 */
	I = (int)(weightedTotal % 10);
	if(I != 0)
		CheckDigit=(10 - I);
	else
		CheckDigit = 0;

	/* Now we must encode the leading digit into the left half of the EAN-13 symbol */
	/* by using variable parity between character sets A and B */
	LeadingDigit = (((asc(mid(LPrintableString, 1, 1))))-48);
	
	if(LeadingDigit == 0)	
		sprintf(Encoding, "AAAAAACCCCCC");
	else if(LeadingDigit == 1)	
		sprintf(Encoding, "AABABBCCCCCC");
	else if(LeadingDigit == 2)	
		sprintf(Encoding, "AABBABCCCCCC");
	else if(LeadingDigit == 3)
		sprintf(Encoding, "AABBBACCCCCC");
	else if(LeadingDigit == 4)
		sprintf(Encoding, "ABAABBCCCCCC");
	else if(LeadingDigit == 5)
		sprintf(Encoding, "ABBAABCCCCCC");
	else if(LeadingDigit == 6)
		sprintf(Encoding, "ABBBAACCCCCC");
	else if(LeadingDigit == 7)
		sprintf(Encoding, "ABABABCCCCCC");
	else if(LeadingDigit == 8)
		sprintf(Encoding, "ABABBACCCCCC");
	else if(LeadingDigit == 9)
		sprintf(Encoding, "ABBABACCCCCC");

	/* add the check digit to the end of the barcode & remove the leading digit */
	sprintf(ActualDataToEncode, "%s%i", mid(LPrintableString, 2, 11), CheckDigit);
	
	/* Now that we have the total number including the check digit, determine character to print */
	/* for proper barcoding: */
	for(I = 1;I <= (int)strlen(ActualDataToEncode);I++)
	{
		/* Get the ASCII value of each number excluding the first number because */
		/* it is encoded with variable parity */
		CurrentChar = asc(mid(ActualDataToEncode, I, 1));
		CurrentEncoding = asc(mid(Encoding, I, 1));

		/* Print different barcodes according to the location of the CurrentChar and CurrentEncoding */
		if(CurrentEncoding == 'A')
			DataToPrintBuffer += sprintf(DataToPrint + DataToPrintBuffer, "%c", CurrentChar);
		else if(CurrentEncoding == 'B')
			DataToPrintBuffer += sprintf(DataToPrint + DataToPrintBuffer, "%c", CurrentChar + 17);
		else if(CurrentEncoding=='C')
			DataToPrintBuffer += sprintf(DataToPrint + DataToPrintBuffer, "%c", CurrentChar + 27);

		/* add in the 1st character along with guard patterns */
		if(I == 1)
		{
			DataToPrintBuffer = 0;
			/* For the LeadingDigit print the human readable character, */
			/* the normal guard pattern and then the rest of the barcode */
			sprintf(Temp,"%i",LeadingDigit);
			if(LeadingDigit > 4)
			{
				sprintf(Temp,"%c(%s",asc(Temp) + 64, DataToPrint);
				DataToPrintBuffer += sprintf(DataToPrint + DataToPrintBuffer, "%s", Temp);
			}
			else if(LeadingDigit < 5)
			{
				sprintf(Temp,"%c(%s",asc(Temp) + 37, DataToPrint);
				DataToPrintBuffer += sprintf(DataToPrint + DataToPrintBuffer,"%s",Temp);
			}
		}
		else if(I==6)
		{
			/* Print the center guard pattern after the 6th character */
			DataToPrintBuffer += sprintf(DataToPrint + DataToPrintBuffer, "*");
		}
		else if(I==12)
		{
			/* For the last character (12) print the the normal guard pattern */
			/* after the barcode */
			DataToPrintBuffer += sprintf(DataToPrint + DataToPrintBuffer, "(");
		}
	}//end for loop thru characters

	/* Process 5 digit add on if it exists */
	if(strlen(EAN5AddOn) == 5)
	{
		/* Get check digit for add on */
		Factor = 3;
		weightedTotal=0;
		for(I = (int)strlen(EAN5AddOn);I >= 1;I--)
		{
			/* Get the value of each number starting at the end */
			CurrentChar = asc(mid(EAN5AddOn, I, 1)) - 48;
			/* multiply by the weighting factor which is 3,9,3,9. */
			/* and add the sum together */
			if(Factor == 3)
				weightedTotal = weightedTotal + (CurrentChar * 3);
			else if(Factor == 1)
				weightedTotal = weightedTotal + CurrentChar * 9;

			/* change factor for next calculation */
			Factor = 4 - Factor;
		}
		/* Find the CheckDigit by extracting the right-most number from weightedTotal */
		sprintf(Temp,"%i",weightedTotal);
		CheckDigit = asc(mid(Temp,(int)strlen(Temp), 1)) - 48;

		/* Now we must encode the add-on CheckDigit into the number sets */
		/* by using variable parity between character sets A and B */		
		if(CheckDigit == 0)			
			sprintf(Encoding,"BBAAA");
		else if(CheckDigit == 1)			
			sprintf(Encoding,"BABAA");
		else if(CheckDigit == 2)			
			sprintf(Encoding,"BAABA");
		else if(CheckDigit == 3)			
			sprintf(Encoding,"BAAAB");
		else if(CheckDigit == 4)
			sprintf(Encoding,"ABBAA");
		else if(CheckDigit == 5)			
			sprintf(Encoding,"AABBA");
		else if(CheckDigit == 6)			
			sprintf(Encoding,"AAABB");
		else if(CheckDigit == 7)			
			sprintf(Encoding,"ABABA");
		else if(CheckDigit == 8)			
			sprintf(Encoding,"ABAAB");
		else if(CheckDigit == 9)			
			sprintf(Encoding,"AABAB");

		/* Now that we have the total number including the check digit, determine character to print */
		/* for proper barcoding: */
		DataToPrintBuffer = 0;
		for(I = 1;I <= (int)strlen(EAN5AddOn);I++)
		{
			/* Get the value of each number.  It is encoded with variable parity */
			CurrentChar = asc(mid(EAN5AddOn,I,1)) - 48;
			CurrentEncoding = asc(mid(Encoding, I, 1));
			if(I == 1)
				DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c%c", 32, 43);

			/* Print different barcodes according to the location of the CurrentChar and CurrentEncoding */
			if(CurrentEncoding == 'A')
			{
				if(CurrentChar == 0)
					DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 34);
				else if(CurrentChar == 1)
					DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 35);
				else if(CurrentChar == 2)
					DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 36);
				else if(CurrentChar == 3)
					DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 37);
				else if(CurrentChar == 4)				
					DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 38);
				else if(CurrentChar == 5)				
					DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 44);
				else if(CurrentChar == 6)				
					DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 46);
				else if(CurrentChar == 7)				
					DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 47);
				else if(CurrentChar == 8)				
					DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 58);
				else if(CurrentChar == 9)				
					DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 59);
			} //end if encoding = A

			if(CurrentEncoding == 'B')
			{
				if(CurrentChar == 0)
					DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 122);
				else if(CurrentChar == 1)
					DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 61);
				else if(CurrentChar == 2)
					DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 63);
				else if(CurrentChar == 3)
					DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 64);
				else if(CurrentChar == 4)				
					DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 91);
				else if(CurrentChar == 5)				
					DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 92);
				else if(CurrentChar == 6)				
					DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 93);
				else if(CurrentChar == 7)				
					DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 95);
				else if(CurrentChar == 8)				
					DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 123);
				else if(CurrentChar == 9)				
					DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 125);
			}//end if encoding = B

			/* add in the space & add-on guard pattern */
			if(I==1)
			{
				/* Now print add-on delineators between each add-on character */
				DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 33);
				//sprintf(Temp,"%c%c%s%c",32,43,EANAddOnToPrint,33);
				//sprintf(EANAddOnToPrint,"%s",Temp);				
			}
			else if(I == 2)
				DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 33);
				//sprintf(EANAddOnToPrint,"%s%c",EANAddOnToPrint,33);
			else if(I == 3)
				DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 33);
			//sprintf(EANAddOnToPrint,"%s%c",EANAddOnToPrint,33);
			else if(I == 4)
				DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 33);
				//sprintf(EANAddOnToPrint,"%s%c",EANAddOnToPrint,33);
			else if(I == 5)
			{
			}
		} //end for loop thru addon
	} //end if sup add on = 5

	/* Process 2 digit add on if it exists */
	if(strlen(EAN2AddOn)==2)
	{
		int Tempi = atoi(mid(EAN2AddOn, 1, 2));
		
		/* Get encoding for add on */
		for(I = 0;I <= 99;I = I + 4)
		{
			if(Tempi == I)
				sprintf(Encoding, "AA");
			if(Tempi == I + 1)
				sprintf(Encoding, "AB");
			if(Tempi == I + 2)
				sprintf(Encoding, "BA");
			if(Tempi == I + 3)
				sprintf(Encoding, "BB");
		}
		/* Now that we have the total number including the encoding */
		/* determine what to print */
		DataToPrintBuffer = 0;
		for(I = 1;I <= (int)strlen(EAN2AddOn);I++)
		{
			/* Get the value of each number. It is encoded with variable parity */
			if(I == 1)
				DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, " %c", 43);

			CurrentChar = asc(mid(EAN2AddOn, I, 1)) - 48;
			CurrentEncoding = asc(mid(Encoding, I, 1));
			/* Print different barcodes according to the location of the CurrentChar and CurrentEncoding */
			if(CurrentEncoding == 'A')
			{
				if(CurrentChar == 0)
					DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 34);
				else if(CurrentChar == 1)
					DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 35);
				else if(CurrentChar == 2)
					DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 36);
				else if(CurrentChar == 3)
					DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 37);
				else if(CurrentChar == 4)				
					DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 38);
				else if(CurrentChar == 5)				
					DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 44);
				else if(CurrentChar == 6)				
					DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 46);
				else if(CurrentChar == 7)				
					DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 47);
				else if(CurrentChar == 8)				
					DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 58);		
				else if(CurrentChar == 9)				
					DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 59);
			} //end if encoding = A
			if(CurrentEncoding == 'B')
			{
				if(CurrentChar == 0)
					DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 122);
				else if(CurrentChar == 1)
					DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 61);
				else if(CurrentChar == 2)
					DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 63);
				else if(CurrentChar == 3)
					DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 64);
				else if(CurrentChar == 4)				
					DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 91);	
				else if(CurrentChar == 5)				
					DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 92);
				else if(CurrentChar == 6)				
					DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 93);
				else if(CurrentChar == 7)				
					DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 95);
				else if(CurrentChar == 8)				
					DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 123);
				else if(CurrentChar == 9)				
					DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 125);
			}//end if encoding = B
			
            /* add in the space & add-on guard pattern */
			if(I==1)
			{
				DataToPrintBuffer += sprintf(EANAddOnToPrint + DataToPrintBuffer, "%c", 33);
			}
		}//end for loop thru 2 digit supp
	} //end if ean 2 add on
	
	//Now we have everything together.  free LPrintableString and re-allocate memory
	delete[] LPrintableString;
	LPrintableString = new char[512];
	if(strlen(EAN2AddOn) == 2 || strlen(EAN5AddOn) == 5)
		sprintf(LPrintableString, "%s%s", DataToPrint, EANAddOnToPrint);
	else
		sprintf(LPrintableString, "%s", DataToPrint);

	// Allow the string to return to the proper size.
	*iSize = (long)strlen(LPrintableString); 
	strncpy(output, LPrintableString, strlen(LPrintableString)); 
	
	delete[] LPrintableString;
	delete[] ActualDataToEncode;
	delete[] OnlyCorrectData;
	delete[] DataToPrint;
	delete[] EAN2AddOn;
	delete[] EAN5AddOn;
	delete[] EANAddOnToPrint;
	delete[] Encoding;
	delete[] Temp;
	return 0;
}

long __stdcall IDAutomation_UPCa (char *DataToEncode, char *output, long *iSize)
{
	if(DataToEncode == NULL)
		return 1;
	if(strlen(DataToEncode) == 0)
		return 1;
	
	char DataToPrint[512];
	char DataToEncode2[512];
	char OnlyCorrectData[512];
	char EAN2AddOn[512];
	char EAN5AddOn[512];
	char EANAddOnToPrint[512];
	char Encoding[512];
	char Temp[512];
	static char PrintableString[512];
	char CurrentChar;
	char CurrentEncoding;
	int  Factor;
	int  weightedTotal;
	int  CheckDigit;
	int  Tempi;
	int  I;

	//reset array(s)
	DataToPrint[0]=(char)0;
	DataToEncode2[0]=(char)0;
	OnlyCorrectData[0]=(char)0;
	EAN2AddOn[0]=(char)0;
	EAN5AddOn[0]=(char)0;
	EANAddOnToPrint[0]=(char)0;
	Encoding[0]=(char)0;
	Temp[0]=(char)0;
	PrintableString[0]=(char)0;

	sprintf(DataToEncode2,"%s",DataToEncode);
	/* Check to make sure data is numeric and remove dashes, etc. */
	for(I=1;I <= (int)strlen(DataToEncode2);I++)
	{
		/* Add all numbers to OnlyCorrectData string */
		if(isdigit(asc(mid(DataToEncode2,I,1))))
		{
			sprintf(OnlyCorrectData,"%s%s",&OnlyCorrectData,mid(DataToEncode2,I,1));
		}
	}

	/* Remove check digits if they added one */
	if(strlen(OnlyCorrectData)==12)
	{
		sprintf(OnlyCorrectData,"%s",mid(OnlyCorrectData,1,11));
	}
	if(strlen(OnlyCorrectData)==14)
	{
		sprintf(OnlyCorrectData,"%s%s",mid(OnlyCorrectData,1,11),mid(OnlyCorrectData,13,2));
	}
	if(strlen(OnlyCorrectData)==17)
	{
		sprintf(OnlyCorrectData,"%s%s",mid(OnlyCorrectData,1,11),mid(OnlyCorrectData,13,5));
	}
	if(strlen(OnlyCorrectData)==16)
	{
		sprintf(EAN5AddOn,"%s",mid(OnlyCorrectData,12,5));
	}
	if(strlen(OnlyCorrectData)==13)
	{
		sprintf(EAN2AddOn,"%s",mid(OnlyCorrectData,12,2));
	}

	if(strlen(OnlyCorrectData) != 11 && strlen(OnlyCorrectData) != 12 && strlen(OnlyCorrectData) != 14 && 
		strlen(OnlyCorrectData) != 17 && strlen(OnlyCorrectData) != 16 && strlen(OnlyCorrectData) != 13)
	{
		sprintf(OnlyCorrectData,"%s","00000000000");
	}

	/* split 12 digit number from add-on */
	sprintf(DataToEncode2,"%s",mid(OnlyCorrectData,1,11));

	/* <<<< Calculate Check Digit >>>> */
	Factor=3;
	weightedTotal=0;
	for(I = (int)strlen(DataToEncode2);I >= 1;I = I + -1)
	{
		/* Get the value of each number starting at the end */
		CurrentChar=(asc(mid(DataToEncode2,I,1)))-48;

		/* multiply by the weighting factor which is 3,1,3,1... */
		/* and add the sum together */
		weightedTotal=weightedTotal+CurrentChar*Factor;

		/* change factor for next calculation */
		Factor=4-Factor;
	}

	/* Find the CheckDigit by finding the number + weightedTotal that = a multiple of 10 */
	/* divide by 10, get the remainder and subtract from 10 */
	I = (int)(weightedTotal % 10);
	if(I!=0)
	{
		CheckDigit=(10-I);
	}
	else
	{
		CheckDigit=0;
	}

	sprintf(DataToEncode2,"%s%i",&DataToEncode2,CheckDigit);
	/* Now that have the total number including the check digit, determine character to print */
	/* for proper barcoding */
	for(I=1;I <= (int)strlen(DataToEncode2);I++)
	{
		/* Get the ASCII value of each number */
		CurrentChar=asc(mid(DataToEncode2,I,1));
		/* Print different barcodes according to the location of the CurrentChar */
		for(;;)
		{
			if(I==1)
			{
				/* For the first character print the human readable character, the normal */
				/* guard pattern and then the barcode without the human readable character */
				if((CurrentChar-48)>4)
				{
					sprintf(DataToPrint,"%c(%c",CurrentChar+64,CurrentChar+49);
				}
				if((CurrentChar-48)<5)
				{
					sprintf(DataToPrint,"%c(%c",CurrentChar+37,CurrentChar+49);
				}
				break;
			}
			if(I==2)
			{
				sprintf(DataToPrint,"%s%c",&DataToPrint,CurrentChar);
				break;
			}
			if(I==3)
			{
				sprintf(DataToPrint,"%s%c",&DataToPrint,CurrentChar);
				break;
			}
			if(I==4)
			{
				sprintf(DataToPrint,"%s%c",&DataToPrint,CurrentChar);
				break;
			}
			if(I==5)
			{
				sprintf(DataToPrint,"%s%c",&DataToPrint,CurrentChar);
				break;
			}
			if(I==6)
			{
				/* Print the center guard pattern after the 6th character */
				sprintf(DataToPrint,"%s%c*",&DataToPrint,CurrentChar);
				break;
			}
			if(I==7)
			{
				/* Add 27 to the ASII value of characters 6-12 to print from character set+ C */
				/* this is required when printing to the right of the center guard pattern */
				sprintf(DataToPrint,"%s%c",&DataToPrint,CurrentChar+27);
				break;
			}
			if(I==8)
			{
				sprintf(DataToPrint,"%s%c",&DataToPrint,CurrentChar+27);
				break;
			}
			if(I==9)
			{
				sprintf(DataToPrint,"%s%c",&DataToPrint,CurrentChar+27);
				break;
			}
			if(I==10)
			{
				sprintf(DataToPrint,"%s%c",&DataToPrint,CurrentChar+27);
				break;
			}
			if(I==11)
			{
				sprintf(DataToPrint,"%s%c",&DataToPrint,CurrentChar+27);
				break;
			}
			if(I==12)
			{
				/* For the last character print the barcode without the human readable character, */
				/* the normal guard pattern and then the human readable character. */
				if((CurrentChar-48)>4)
				{
					sprintf(DataToPrint,"%s%c(%c",&DataToPrint,CurrentChar+59,CurrentChar+64);
				}
				if((CurrentChar-48)<5)
				{
					sprintf(DataToPrint,"%s%c(%c",&DataToPrint,CurrentChar+59,CurrentChar+37);
				}
			}
			break;
		} //end for
	}//end for thru characters

	/* Process 5 digit add on if it exists */
	if(strlen(EAN5AddOn)==5)
	{
		/* Get check digit for add on */
		Factor=3;
		weightedTotal=0;
		for(I = (int)strlen(EAN5AddOn);I >= 1;I--)
		{
			/* Get the value of each number starting at the end */
			CurrentChar=asc(mid(EAN5AddOn,I,1))-48;
			/* multiply by the weighting factor which is 3,9,3,9. */
			/* and add the sum together */
			if(Factor==3)
			{
				weightedTotal=weightedTotal+(CurrentChar*3);
			}
			if(Factor==1)
			{
				weightedTotal=weightedTotal+CurrentChar*9;
			}
			/* change factor for next calculation */
			Factor=4-Factor;
		}

		/* Find the CheckDigit by extracting the right-most number from weightedTotal */
		sprintf(Temp,"%i",weightedTotal);
		CheckDigit = asc(mid(Temp, (int)strlen(Temp), 1)) - 48;

		/* Now we must encode the add-on CheckDigit into the number sets */
		/* by using variable parity between character sets A and B */
		for(;;)
		{
			if(CheckDigit==0)
			{
				sprintf(Encoding,"BBAAA");
				break;
			}
			if(CheckDigit==1)
			{
				sprintf(Encoding,"BABAA");
				break;
			}
			if(CheckDigit==2)
			{
				sprintf(Encoding,"BAABA");
				break;
			}
			if(CheckDigit==3)
			{
				sprintf(Encoding,"BAAAB");
				break;
			}
			if(CheckDigit==4)
			{
				sprintf(Encoding,"ABBAA");
				break;
			}
			if(CheckDigit==5)
			{
				sprintf(Encoding,"AABBA");
				break;
			}
			if(CheckDigit==6)
			{
				sprintf(Encoding,"AAABB");
				break;
			}
			if(CheckDigit==7)
			{
				sprintf(Encoding,"ABABA");
				break;
			}
			if(CheckDigit==8)
			{
				sprintf(Encoding,"ABAAB");
				break;
			}
			if(CheckDigit==9)
			{
				sprintf(Encoding,"AABAB");
			}
			break;
		}


		/* Now that we have the total number including the check digit, determine character to print */
		/* for proper barcoding: */
		for(I=1;I <= (int)strlen(EAN5AddOn);I++)
		{
			/* Get the value of each number */
			/* it is encoded with variable parity */
			CurrentChar=asc(mid(EAN5AddOn,I,1))-48;
			CurrentEncoding=asc(mid(Encoding,I,1));

			/* Print different barcodes according to the location of the CurrentChar and CurrentEncoding */
			for(;;)
			{
				if(CurrentEncoding=='A')
				{
					if(CurrentChar==0)
					{
						sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,34);
					}
					if(CurrentChar==1)
					{
						sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,35);
					}
					if(CurrentChar==2)
					{
						sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,36);
					}
					if(CurrentChar==3)
					{
						sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,37);
					}
					if(CurrentChar==4)
					{
						sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,38);
					}
					if(CurrentChar==5)
					{
						sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,44);
					}
					if(CurrentChar==6)
					{
						sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,46);
					}
					if(CurrentChar==7)
					{
						sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,47);
					}
					if(CurrentChar==8)
					{
						sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,58);
					}
					if(CurrentChar==9)
					{
						sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,59);
					}
					break;
				}
				if(CurrentEncoding=='B')
				{
					if(CurrentChar==0)
					{
						sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,122);
					}
					if(CurrentChar==1)
					{
						sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,61);
					}
					if(CurrentChar==2)
					{
						sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,63);
					}
					if(CurrentChar==3)
					{
						sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,64);
					}
					if(CurrentChar==4)
					{
						sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,91);
					}
					if(CurrentChar==5)
					{
						sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,92);
					}
					if(CurrentChar==6)
					{
						sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,93);
					}
					if(CurrentChar==7)
					{
						sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,95);
					}
					if(CurrentChar==8)
					{
						sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,123);
					}
					if(CurrentChar==9)
					{
						sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,125);
					}
				}
				
				break;
			}

			/* add in the space & add-on guard pattern */
			for(;;)
			{
				if(I==1)
				{
					sprintf(Temp,"%c%s%c",43,&EANAddOnToPrint,33);
					sprintf(EANAddOnToPrint,"%s",&Temp);
					/* Now print add-on delineators between each add-on character */
					break;
				}
				if(I==2)
				{
					sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,33);
					break;
				}
				if(I==3)
				{
					sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,33);
					break;
				}
				if(I==4)
				{
					sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,33);
					break;
				}
				if(I==5)
				{
				}
				break;
			}
		}
	} //end if ean 5 add-on

	/* Process 2 digit add on if it exists */
	if(strlen(EAN2AddOn)==2)
	{

		Tempi=(((asc(mid(EAN2AddOn,1,1))))-48)*10;
		Tempi=(Tempi + (asc(mid(EAN2AddOn,2,1))))-48;

		/* Get encoding for add on */
		for(I=0;I<=99;I=I+4)
		{
			if(Tempi==I)
			{
				sprintf(Encoding,"AA");
			}
			if(Tempi==I+1)
			{
				sprintf(Encoding,"AB");
			}
			if(Tempi==I+2)
			{
				sprintf(Encoding,"BA");
			}
			if(Tempi==I+3)
			{
				sprintf(Encoding,"BB");
			}
		}

		/* Now that we have the total number including the encoding */
		/* determine what to print */
		for(I=1;I <= (int)strlen(EAN2AddOn);I++)
		{
			/* Get the value of each number */
			/* it is encoded with variable parity */
			CurrentChar=asc(mid(EAN2AddOn,I,1))-48;
			CurrentEncoding=asc(mid(Encoding,I,1));

			/* Print different barcodes according to the location of the CurrentChar and CurrentEncoding */
			for(;;)
			{
				if(CurrentEncoding=='A')
				{
					if(CurrentChar==0)
					{
						sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,34);
					}
					if(CurrentChar==1)
					{
						sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,35);
					}
					if(CurrentChar==2)
					{
						sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,36);
					}
					if(CurrentChar==3)
					{
						sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,37);
					}
					if(CurrentChar==4)
					{
						sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,38);
					}
					if(CurrentChar==5)
					{
						sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,44);
					}
					if(CurrentChar==6)
					{
						sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,46);
					}
					if(CurrentChar==7)
					{
						sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,47);
					}
					if(CurrentChar==8)
					{
						sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,58);
					}
					if(CurrentChar==9)
					{
						sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,59);
					}
					break;
				}
				if(CurrentEncoding=='B')
				{
					if(CurrentChar==0)
					{
						sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,122);
					}
					if(CurrentChar==1)
					{
						sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,61);
					}
					if(CurrentChar==2)
					{
						sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,63);
					}
					if(CurrentChar==3)
					{
						sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,64);
					}
					if(CurrentChar==4)
					{
						sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,91);
					}
					if(CurrentChar==5)
					{
						sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,92);
					}
					if(CurrentChar==6)
					{
						sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,93);
					}
					if(CurrentChar==7)
					{
						sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,95);
					}
					if(CurrentChar==8)
					{
						sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,123);
					}
					if(CurrentChar==9)
					{
						sprintf(EANAddOnToPrint,"%s%c",&EANAddOnToPrint,125);
					}
				}
				break;
			}

			/* add in the space & add-on guard pattern */
			for(;;)
			{
				if(I==1)
				{
					sprintf(Temp,"%c%s%c",43,&EANAddOnToPrint,33);
					sprintf(EANAddOnToPrint,"%s",&Temp);
					/* Now print add-on delineators between each add-on character */
					break;
				}
				if(I==2)
				{

				}
				break;
			}
		}//end for
	}//end if addon = 2


	/* Get Printable String */
	sprintf(PrintableString,"%s%s", &DataToPrint, &EANAddOnToPrint);
	
	// Allow the string to return to the proper size.
	*iSize = (long)strlen(PrintableString); 
	strncpy(output, PrintableString, strlen(PrintableString)); 
	return 0;
} //end UPCa()

//Creates a formatted text for MSI Plessy Barcode
long __stdcall IDAutomation_MSI (char *DataToEncode, char *output, long *iSize)
{
	if(DataToEncode == NULL)
		return 1;
	if(strlen(DataToEncode) == 0)
		return 1;
	
	char* DataToPrint;	//the data that we will return
	DataToPrint = new char[512];

	char* OnlyCorrectData;	//DataToEncode after it is filtered
	OnlyCorrectData = new char[512];

	int sprintBuffer = 0;
	int  I;

	// Check to make sure data is numeric and remove dashes, etc. 
	for(I = 1;I <= (int)strlen(DataToEncode);I++)
	{
		//Add all numbers to OnlyCorrectData string 
		if(isdigit(asc(mid(DataToEncode, I, 1))))
		{
			sprintBuffer += sprintf(OnlyCorrectData + sprintBuffer, "%s", mid(DataToEncode, I, 1));
		}
	}

	// Get the string w/ start, stop, check characters
	sprintf(DataToPrint,"(%s%s)", OnlyCorrectData, CalcMSICheckDigit(OnlyCorrectData));

	// Allow the string to return to the proper size.
	*iSize = (long)strlen(DataToPrint); 
	strncpy(output, DataToPrint, strlen(DataToPrint)); 
	
	delete [] DataToPrint;
	delete [] OnlyCorrectData;

	return 0;
} //end MSI()

char *CalcMSICheckDigit(char DataToEncode[])
{
	//Step 1 -- Create a new number of the odd position digits starting from the right and going left, but store the
    //digits from left to right. 
    //We will create the odd position number & prepare for Step 4 by getting the sum of all even position charactesr
    int StringLength = (int)strlen(DataToEncode);
    char OddNumbers[30];
	char sOddNumberProduct[8];
    int OddDigit = TRUE;
    int EvenNumberSum = 0;
	int BufferCounterOdd = 0;
	int BufferCounterEven = 0;
	int OddNumberProduct = 0;
	int OddNumberSum = 0;
	int CD = 0;
	int Idx = 0;
	static char sCD[1];

	for(Idx = StringLength;Idx >= 0;Idx--)
	{
		if(OddDigit == TRUE)
		{
			BufferCounterOdd += sprintf(OddNumbers + BufferCounterOdd, "%s", mid(DataToEncode, Idx, 1));
			OddDigit = FALSE;
		}
		else
		{
			EvenNumberSum += atoi(mid(DataToEncode, Idx, 1));
			OddDigit = TRUE;
		}
	}
        
    //Step 2 -- Multiply this new number by 2.
    OddNumberProduct = atoi(OddNumbers) * 2;

    //Step 3 -- Add all of the digits of the product from step two.
	sprintf(sOddNumberProduct, "%d", OddNumberProduct);
	StringLength = (int)strlen(sOddNumberProduct);
    OddNumberSum = 0;

	for(Idx = 1;Idx <= StringLength;Idx++)
	{
		OddNumberSum += atoi(mid(sOddNumberProduct, Idx, 1));
	}
   
    //Step 4 -- Add all of the digits not used in step one to the result in step three.
    //We will store the result in OddNumberSum just so we don't have to create another variable
    OddNumberSum = OddNumberSum + EvenNumberSum;
    
    //Step 5 -- Determine the smallest number which when added to the result in step four
    //will result in a multiple of 10. This is the check character.
    OddNumberSum = OddNumberSum % 10;
	if(OddNumberSum != 0)
		CD = 10 - OddNumberSum;
	else
		CD = 0;

	sprintf(sCD, "%i", CD);

	return sCD;

}

//Function to return human readable text
long __stdcall IDAutomation_Code128HumanReadable (char *DataToEncode, long &ApplyTilde, char *output, long *iSize)
{
	if(DataToEncode == NULL)
		return 1;
	if(strlen(DataToEncode) == 0)
		return 1;

	char* ReadableText;		//The human readable text
	ReadableText = new char[512];

	char* StringToCheckFiltered;	//used to find mod 10 check digit
	StringToCheckFiltered = new char[512];

	char *RevFilteredString;	//reversed string for mod 10 calc
	int BufferCounter = 0;		//sprintf buffer
	int BufferCounter2 = 0;		//sprintf buffer
	int i = 0;					//for loop counter
	int q = 0;					//for loop counter
	int GoodData = 0;			//switch for determining if data is valid
	int FNCDone;
	
	for(i = 0;i < (int)strlen(DataToEncode);i++)
	{
		FNCDone = FALSE;
		if((int)asc(mid(DataToEncode, i + 1, 1)) != 126 || ApplyTilde == FALSE || strlen(DataToEncode) < 4)
		{
			//need to make sure there are enough characters left in DataToEncode and add AI characters
			if(asc(mid(DataToEncode, i + 1, 1)) == 212 && i <= (int)strlen(DataToEncode) - 3)			{
				
				BufferCounter += sprintf(ReadableText + BufferCounter, "%s%s%s"," (", mid(DataToEncode, i + 2, 2), ") ");
				i += 2;
			}
			else if(asc(mid(DataToEncode, i + 1, 1)) == 213 && i <= (int)strlen(DataToEncode) - 4)
			{
				BufferCounter += sprintf(ReadableText + BufferCounter, "%s%s%s"," (", mid(DataToEncode, i + 2, 3), ") ");
				i += 3;
			}
			else if(asc(mid(DataToEncode, i + 1, 1)) == 214 && i <= (int)strlen(DataToEncode) - 5)
			{
				BufferCounter += sprintf(ReadableText + BufferCounter, "%s%s%s"," (", mid(DataToEncode, i + 2, 4), ") ");
				i += 4;
			}
			else if(asc(mid(DataToEncode, i + 1, 1)) == 215 && i <= (int)strlen(DataToEncode) - 6)
			{
				BufferCounter += sprintf(ReadableText + BufferCounter, "%s%s%s"," (", mid(DataToEncode, i + 2, 5), ") ");
				i += 5;
			}
			else if(asc(mid(DataToEncode, i + 1, 1)) == 216 && i <= (int)strlen(DataToEncode) - 7)
			{
				BufferCounter += sprintf(ReadableText + BufferCounter, "%s%s%s"," (", mid(DataToEncode, i + 2, 6), ") ");
				i += 6;
			}
			else if(asc(mid(DataToEncode, i + 1, 1)) == 217 && i <= (int)strlen(DataToEncode) - 8)
			{
				BufferCounter += sprintf(ReadableText + BufferCounter, "%s%s%s"," (", mid(DataToEncode, i + 2, 7), ") ");
				i += 7;
			}
			else if(asc(mid(DataToEncode, i + 1, 1)) == 202 && i <= (int)strlen(DataToEncode) - 4 && 
				isdigit((int) DataToEncode[i + 1]) && isdigit((int) DataToEncode[i + 2])) //check for FNC1
			{
				//we can auto-detect some AIs if there is an FNC1 in the data to encode
				//Is 4 digit AI by detection?
				if(atoi(mid(DataToEncode, i + 2, 2)) == 80 || atoi(mid(DataToEncode, i + 2, 2)) == 81 ||
					(atoi(mid(DataToEncode, i + 2, 2)) <= 34 && atoi(mid(DataToEncode, i + 2, 2)) >= 31))
				{
					BufferCounter += sprintf(ReadableText + BufferCounter, "%s%s%s"," (", mid(DataToEncode, i + 2, 4), ") ");
					i += 4;
					FNCDone = TRUE;
				}

				//Is 3 digit AI by detection?
				if(FNCDone == FALSE)
				{					
					if((atoi(mid(DataToEncode, i + 2, 2)) <= 49 && atoi(mid(DataToEncode, i + 2, 2)) >= 40) ||
						(atoi(mid(DataToEncode, i + 2, 2)) <= 25 && atoi(mid(DataToEncode, i + 2, 2)) >= 23))
					{
						BufferCounter += sprintf(ReadableText + BufferCounter, "%s%s%s"," (", mid(DataToEncode, i + 2, 3), ") ");
						i += 3;
						FNCDone = TRUE;
					}
				}

				//Is 2 digit AI by detection?
				if(FNCDone == FALSE)
				{					
					if((atoi(mid(DataToEncode, i + 2, 2)) <= 30 && atoi(mid(DataToEncode, i + 2, 2)) >= 0) ||
						(atoi(mid(DataToEncode, i + 2, 2)) <= 99 && atoi(mid(DataToEncode, i + 2, 2)) >= 90))
					{
						BufferCounter += sprintf(ReadableText + BufferCounter, "%s%s%s"," (", mid(DataToEncode, i + 2, 2), ") ");
						i += 2;
						FNCDone = TRUE;
					}

				}
				//If no AI was detected, set default to 4 digit AI:
				if(FNCDone == FALSE)
				{
					BufferCounter += sprintf(ReadableText + BufferCounter, "%s%s%s"," (", mid(DataToEncode, i + 4, 4), ") ");
					i += 4;
					FNCDone = TRUE;
				}
			}
			else if(asc(mid(DataToEncode, i + 1, 1)) < 32)
				BufferCounter += sprintf(ReadableText + BufferCounter, "%s", " ");
			else if(asc(mid(DataToEncode, i + 1, 1)) >= 32 && asc(mid(DataToEncode, i + 1, 1)) <= 126)
				BufferCounter += sprintf(ReadableText + BufferCounter, "%s", mid(DataToEncode, i + 1, 1));
		}
		else
		{
			//We are here because we found a tilde and ApplyTilde == true
			//If the current character is the tilde and applyTilde is set to true, we 
			//need to look at the next 3 characters to determine the ASCII character that
			//the user is attempting to encode.  We also verify that following the ~ are 
			//3 numbers between 1 && 256.
			int char0 = (int) asc(mid(DataToEncode, i + 2, 1));
			int char1 = (int) asc(mid(DataToEncode, i + 3, 1));
			int char2 = (int) asc(mid(DataToEncode, i + 4, 1));
			if (i <= (int)strlen(DataToEncode) - 3 && isdigit(char0) && isdigit(char1)
				&& isdigit(char2) )
			{
				//Verify that the number is between 1 & 32
				
				char *tempNum = mid(DataToEncode,i + 2, 3);
				int convertedValue = atoi(mid(DataToEncode, i + 2, 3));
				if((convertedValue > 0 && convertedValue < 32))
				{
					//We've verified that this is correct tilde processing.  Increase the loop counter
					//and skip these characters, but add a space in their place
					BufferCounter += sprintf(ReadableText + BufferCounter, "%s", " ");
					i = i + 3;
				}
				else if(convertedValue == 197)
					i = i + 3;
				else if(convertedValue != 197) //Otherwise just add the current character						
					BufferCounter += sprintf(ReadableText + BufferCounter, "%c", DataToEncode[i]);
			} 			
			//Here we are looking to create a mod 10 check digit.  If we see ~m10, we take the 10 characters before
			//~m and calculate a mod 10 check digit.  The data to encode string has to be more than 5 characters and
			//the ~ can not be the first character (since we're pulling data from before the ~).  109 is the ASCII
			//value of little m
			else if(((int)strlen(DataToEncode) > 5 && i > 1 && i <= (int)strlen(DataToEncode) - 3) && (int) DataToEncode[i + 1] == 109)
			{
				//an example would be 987654321~m09.  We would take the 9 characters before ~, calculate a mod 10 check
				//digit (ensuring all values were numbers first), and replace ~m09 w/ the newly calculated check digit
				
				//our counter is currently on the ~.  Grab the 2 characters after the m after the ~.  If they do not convert 
				//to a numeric, we hit the catch and move on.
				if(isdigit(DataToEncode[i + 2]) && isdigit(DataToEncode[i + 3]))
				{
					int NumCharsToConvert = atoi(mid(DataToEncode, i + 3, 2));
					//Now that know we have a number, we have to make sure there are enough characters preceding the tilde
					//to do the processing.  If there are not, we encode the ~ and move on.  i is at the ~
					//and is 0-based.  So accounting for the that we have to take 1 from the number of characters to convert
					if(i - (NumCharsToConvert - 1) >= 0)
					{
						//total number of characters before tilde.  We want to skip any non-numeric characters 
						//when taking into account the number of characters to go back.  Meaning if they try to
						//encode 1234A567~m06, we would calculate the mod 10 digit by using 234567.
						//int NumPrecedingCharacters = i - 1;
						char *StringToCheck = mid(DataToEncode, i - NumCharsToConvert + 1, NumCharsToConvert);
						
						int NumCharsFiltered = 0;
						//for this, StringToCheck must be all numerics.  Make sure
						GoodData = 0;
						BufferCounter2 = 0;
						//We have to loop backwards
						for(q = (int)strlen(StringToCheck);q > 0 ;q--)
						{						
							GoodData = FALSE;
							if((int)StringToCheck[q - 1] > 0 && (int)StringToCheck[q - 1] <= 126  && isdigit((int)StringToCheck[q - 1]))								
								GoodData = TRUE;;
							
							if(GoodData)
							{
								if(NumCharsFiltered < NumCharsToConvert )
								{
									BufferCounter2 += sprintf(StringToCheckFiltered + BufferCounter2, "%s", mid(StringToCheck, q, 1));
									NumCharsFiltered++;
								}
								else //we're done.  We've accounted for all characters
									break;
							}	
						}
						
						//If we've looped through all of the characters and don't have enough to meet the
						//number defined by the user, don't do the encoding
						if(StringToCheckFiltered == "")
							GoodData = FALSE;
						else
						{
							GoodData = TRUE;
							RevFilteredString = strrev(StringToCheckFiltered);
						}

						if(GoodData != FALSE)
						{  //now we are ready to calculate the check digit
							BufferCounter += sprintf(ReadableText + BufferCounter, "%c", FindMod10Digit(RevFilteredString));
							i += 3;  //move the character pointer past ~mnn
						}
						else //to many letters between numbers
							BufferCounter += sprintf(ReadableText + BufferCounter, "%c", DataToEncode[i]);
					} 
					else //not enough characters preceding the tilde
						BufferCounter += sprintf(ReadableText + BufferCounter, "%c", DataToEncode[i]);
				} 
				else //the 2 characters after ~m are not digits
					BufferCounter += sprintf(ReadableText + BufferCounter, "%c", DataToEncode[i]);
			} //end ~ processing
		} //end else apply tilde == true
	} //end for loop
	
	// Allow the string to return to the proper size.
	*iSize = (long)strlen(ReadableText); 
	strncpy(output, ReadableText, strlen(ReadableText)); 
	
	delete [] ReadableText;
	delete [] StringToCheckFiltered;

	return 0;
}//Code128HumanReadable()

//UCC 128.  Checks for an FNC1 character at the begining.  If it is not there, one is added and then 128 Auto is called.
long __stdcall IDAutomation_UCC128 (char *DataToEncode, char *output, long *iSize)
{
	char* StringToPass;		//The total string w/ start, stop, check, & encoding
	StringToPass = new char[512];

	int char0 = (int) asc(mid(DataToEncode, 1, 1));
	if(char0 != 202)
		sprintf(StringToPass, "%c%s", (char)202, DataToEncode);
	else
		sprintf(StringToPass, "%s", DataToEncode);

	long iTilde = 1;
	IDAutomation_Code128(StringToPass, &iTilde, output, iSize);
	
	delete [] StringToPass;
	return 0;
} //UCC 128 functionality.  

//Code 128 auto function.  determines best possible encoding scenario
long __stdcall IDAutomation_Code128 (char *DataToEncode, long &ApplyTilde, char *output, long *iSize)
{
	if(DataToEncode == NULL)
		return 1;
	if(strlen(DataToEncode) == 0)
		return 1;

	char* localPrintableString;		//The total string w/ start, stop, check, & encoding
	localPrintableString = new char[512];

	char* OnlyCorrectData;			//data to encode after tilde processing is applied
	OnlyCorrectData = new char[512];
	
	char* StringToCheckFiltered;	//used in tilde processing
	StringToCheckFiltered = new char[512];
	
	char *tildeStringToCheck;
	tildeStringToCheck = new char[512];

	char CurrentEncoding;			//the current encoding structure
	char StartChar;
	char StopChar;
	int DataToEncodeLength = 0;		//length of data to encode
	int CurrentValue = 0;			//ascii value of current character
	int i = 0;						//for loop counter
	int GoodData = 0;				//determines if we have valid data
	int q = 0;						// for loop counter

	int BufferCounter = 0;
	int L_BufferCounter = 0;
	int WeightedTotal = 0;
	int StringLength = 0;
	int CurrentCharNumber = 0;
	int CheckDigitValue = 0;
	int WeightValue = 0;
	DataToEncodeLength = (int)strlen(DataToEncode);
	StopChar = (char)206;

	//is this character set A?
	if(((int)asc(mid(DataToEncode, 1, 1))) < 32)
	{
		StartChar = (char) 203;  //char set A start character
		CurrentEncoding = 'A';
	}
	//is this character set B?
	if(((int) asc(mid(DataToEncode, 1, 1)) > 31 && (int) asc(mid(DataToEncode, 1, 1)) < 127) || ((int) asc(mid(DataToEncode, 1, 1)) == 197))
	{
		StartChar = (char) 204;  //char set B start character
		CurrentEncoding = 'B';
	}
	//should we make it set C?
	int char0 = (int)asc(mid(DataToEncode, 1, 1));
	int char1 = (int)asc(mid(DataToEncode, 2, 1));
	int char2 = (int)asc(mid(DataToEncode, 3, 1));
	int char3 = (int)asc(mid(DataToEncode, 4, 1));

	if(DataToEncodeLength > 3 && (char0 > 0 && char1 > 0 && char2 > 0 && char3 > 0) && isdigit(char0) && isdigit(char1) &&
		isdigit(char2) && isdigit(char3))
	{
		StartChar = (char) 205;  //char set C start character
		CurrentEncoding = 'C';
	}

	if ((int) asc(mid(DataToEncode, 1, 1)) == 202 || ((int) asc(mid(DataToEncode, 1, 1)) >= 212 && (int) asc(mid(DataToEncode, 1, 1)) <= 217)) // 202 is the FNC1
	{
		StartChar = (char) 205;  //char set C start character
		CurrentEncoding = 'C';
	}

	//Before we start looping through characters, we are going to find any tildes and replace the values 
	for(i = 0; i <= DataToEncodeLength - 1; i++)
	{
		if((DataToEncodeLength > 5 && i > 1 && i <= DataToEncodeLength - 3) && (int) DataToEncode[i] == 126 
			&& (int) DataToEncode[i + 1] == 109 && ApplyTilde)
		{
			//our counter is currently on the ~.  Grab the 2 characters after the m after the ~.  If they do not convert 
			//to a numeric, we hit the catch and move on.
			if(isdigit(DataToEncode[i + 2]) && isdigit(DataToEncode[i + 3]))
			{			
				//char *text = mid(DataToEncode,i + 2, 2);
				WeightValue = atoi(mid(DataToEncode, i + 3, 2));
				if(i - WeightValue < 1)
					WeightValue = i - 1;
				
				int pb2 = 0; //sprintf buffer
				int numCharsTaken = 0;
				for(int k = i - 1;k >= 0;k--) //currently i is on the ~, k - 1 gives the array value of ~, k - 2 is where we need to start
				{
					if((int)DataToEncode[k] >= 48 && (int)DataToEncode[k] <= 57)
					{
						pb2 += sprintf(tildeStringToCheck + pb2, "%c", DataToEncode[k]);
						numCharsTaken++;
						if(numCharsTaken == WeightValue)
							break;
					}
				}
				if(numCharsTaken > 0)
				{
					//tildeStringToCheck is backwards.  need to flip it before adding it to the datatoencode
					L_BufferCounter += sprintf(OnlyCorrectData + L_BufferCounter, "%c", FindMod10Digit(strrev(tildeStringToCheck)));
					i += 3;
				}
				else
					L_BufferCounter += sprintf(OnlyCorrectData + L_BufferCounter, "%s", mid(DataToEncode, i, 1));			
			} 
			else //the 2 characters after ~m are not digits
				L_BufferCounter += sprintf(OnlyCorrectData + L_BufferCounter,"%c", DataToEncode[i]);
		} //end check ~mxx
			//If the current character is the tilde and applyTilde is set to true, we 
			//need to look at the next 3 characters to determine the ASCII character that
			//the user is attempting to encode.  We also verify that following the ~ are 
			//3 numbers between 1 && 256.
		else if ((DataToEncodeLength >= 4 && i <= DataToEncodeLength - 3)  && (int) DataToEncode[i] == 126 
			&& isdigit((int) DataToEncode[i + 1]) && isdigit((int) DataToEncode[i + 2])
			&& isdigit((int) DataToEncode[i + 3]) && ApplyTilde)
		{
			//Verify that the number is between 1 & 32 or the FNC2 or FNC1 characters
			int convertedValue = atoi(mid(DataToEncode, i + 2, 3));
			if((convertedValue > 0 && convertedValue < 32) || convertedValue == 197 || convertedValue == 202 || convertedValue == 126 ||
				(convertedValue >= 212 && convertedValue <= 217) )
			{
				L_BufferCounter += sprintf(OnlyCorrectData + L_BufferCounter,"%c", (char) (convertedValue));
				i = i + 3;
			}
			else
				L_BufferCounter += sprintf(OnlyCorrectData + L_BufferCounter,"%c", DataToEncode[i]);
		} 	//end check for ~xxx	
			//if we have back to back tildes, only encode one
		else if((DataToEncodeLength >= 2 && i <= DataToEncodeLength - 2) && (int) DataToEncode[i] == 126 && 
			(int) DataToEncode[i + 1] == 126 && ApplyTilde)
		{
			L_BufferCounter += sprintf(OnlyCorrectData + L_BufferCounter,"%c", DataToEncode[i]);
			i++;
		}
		else if(((int) asc(mid(DataToEncode, i + 1, 1)) <= 126 && (int) asc(mid(DataToEncode, i + 1, 1)) >= 0) || (int) asc(mid(DataToEncode, i + 1, 1)) == 202)
			L_BufferCounter += sprintf(OnlyCorrectData + L_BufferCounter,"%c", DataToEncode[i]);
	}//end loop for apply tilde encoding

	L_BufferCounter = 0;
	//Add the start character to our printable string
	L_BufferCounter += sprintf(localPrintableString + L_BufferCounter,"%c", StartChar);

	DataToEncodeLength = (int)strlen(OnlyCorrectData);
	//Loop thru the characters and create our printable string
	for(i = 0; i <= DataToEncodeLength - 1; i++)
	{
		//First check for FNC1
		if( i < DataToEncodeLength - 1 && ((int) asc(mid(OnlyCorrectData, i + 1, 1)) == 202 || (int) asc(mid(OnlyCorrectData, i + 1, 1)) == 212 || (int) asc(mid(OnlyCorrectData, i + 1, 1)) == 213 
			|| (int) asc(mid(OnlyCorrectData, i + 1, 1)) == 214 || (int) asc(mid(OnlyCorrectData, i + 1, 1)) == 215
			|| (int) asc(mid(OnlyCorrectData, i + 1, 1)) == 216 || (int) asc(mid(OnlyCorrectData, i + 1, 1)) == 217 || (int) asc(mid(OnlyCorrectData, i + 1, 1)) == 218))
		{
			L_BufferCounter += sprintf(localPrintableString + L_BufferCounter,"%c", (char)202);
		}
		//next we look to see if we have a string of consecutive numbers.  First check is to make sure we have enough characters.
		//second check determines if we have numbers.  third check is if we are already in set C.
		else if( (DataToEncodeLength > 3 && i <= (DataToEncodeLength - 2)) &&
			( (isdigit((int) asc(mid(OnlyCorrectData, i + 1, 1))) && isdigit((int) asc(mid(OnlyCorrectData, i + 2, 1))) && 
			isdigit((int) asc(mid(OnlyCorrectData, i + 3, 1))) && isdigit((int) asc(mid(OnlyCorrectData, i + 4, 1))) ) || 
			(i <= DataToEncodeLength - 1 && isdigit((int) asc(mid(OnlyCorrectData, i + 1, 1))) && 
			isdigit((int) asc(mid(OnlyCorrectData, i + 2, 1))) && CurrentEncoding == 'C' )))

		{
			if(CurrentEncoding != 'C')
			{
				L_BufferCounter += sprintf(localPrintableString + L_BufferCounter,"%c", (char)199);
				CurrentEncoding = 'C';
			}

			/* Get the value of each number pair */
			CurrentValue = atoi(mid(OnlyCorrectData, i + 1, 2));

			/* Get the DataToPrint */
			if(CurrentValue < 95 && CurrentValue > 0) 
			{
				L_BufferCounter += sprintf(localPrintableString + L_BufferCounter,"%c", (char)(CurrentValue + 32));
			}
			if(CurrentValue > 94)
			{
				L_BufferCounter += sprintf(localPrintableString + L_BufferCounter,"%c", (char)(CurrentValue + 100));
			}
			if(CurrentValue == 0)
			{
				L_BufferCounter += sprintf(localPrintableString + L_BufferCounter,"%c", (char)194);
			}
			
			//increase the counter by one see we took two characters
			i++;
		}//end else for set C
		//Now check for set A.  if the ascii value is under 32 or we are already in set A and the ascii value is under 96, go in.
		else if ((i <= DataToEncodeLength - 1) && (((int) asc(mid(OnlyCorrectData, i + 1, 1)) < 32) || ((CurrentEncoding == 'A') && ((int) asc(mid(OnlyCorrectData, i + 1, 1)) < 96)))) 
		{	//now we are looking to switch to set A
			if (CurrentEncoding != 'A') 
			{
				L_BufferCounter += sprintf(localPrintableString + L_BufferCounter,"%c", (char)201);
				CurrentEncoding = 'A';
			}
			if ((int) OnlyCorrectData[i] < 32) 
			{
				L_BufferCounter += sprintf(localPrintableString + L_BufferCounter,"%c", (char)((int) OnlyCorrectData[i] + 96));
			}
			else if ((int) OnlyCorrectData[i] > 31) 
			{
				L_BufferCounter += sprintf(localPrintableString + L_BufferCounter,"%c", (char)OnlyCorrectData[i]);
			}
			else if ((int) OnlyCorrectData[i] == 32)
				L_BufferCounter += sprintf(localPrintableString + L_BufferCounter,"%c", (char)194);
		}//end else for set A
		//Account for the FNC2 character
		else if((int) asc(mid(OnlyCorrectData, i + 1, 1)) == 197)
		{
			if(CurrentEncoding == 'C')
			{
				L_BufferCounter += sprintf(localPrintableString + L_BufferCounter,"%c", (char)200);
				CurrentEncoding = 'B';
			}
			L_BufferCounter += sprintf(localPrintableString + L_BufferCounter,"%c", OnlyCorrectData[i]);
		}
		//Everything else should be set B
		else if ((i <= DataToEncodeLength - 1) && ((int) asc(mid(OnlyCorrectData, i + 1, 1)) > 31 && (int) asc(mid(OnlyCorrectData, i + 1, 1)) < 127)) 
		{
			// Switch to set B if not already in it
			if (CurrentEncoding != 'B') 
			{
				L_BufferCounter += sprintf(localPrintableString + L_BufferCounter,"%c", (char)200);
				CurrentEncoding = 'B';
			}
			
			//just a regular character.  Add it to the string.  Handle spaces separately
			if ((int) OnlyCorrectData[i] == 32)
				L_BufferCounter += sprintf(localPrintableString + L_BufferCounter,"%c", (char)194);
			else
				L_BufferCounter += sprintf(localPrintableString + L_BufferCounter,"%c", OnlyCorrectData[i]);
		}//end else we're doing set B
	}//end loop thru characters

	//<<<< Calculate Modulo 103 Check Digit >>>>
    WeightedTotal = (asc(&StartChar) - 100);
	StringLength = (int)strlen(localPrintableString);
	CurrentCharNumber = 0;
	CurrentValue = 0;
	CheckDigitValue = 0;
	for(q = 2;q <= StringLength;q++)  //start at 2 because of the start character already on localPrintableString
	{
		CurrentCharNumber = asc(mid(localPrintableString, q, 1));
		if(CurrentCharNumber < 135)
			CurrentValue = (CurrentCharNumber - 32);
		else if(CurrentCharNumber > 134  && CurrentCharNumber != 194)
			CurrentValue = (CurrentCharNumber - 100);
		else if(CurrentCharNumber == 194)
			CurrentValue = 0;

		CurrentValue = CurrentValue * (q - 1);
		WeightedTotal = WeightedTotal + CurrentValue;
	}
    
    CheckDigitValue = (WeightedTotal % 103);

    if(CheckDigitValue < 95 && CheckDigitValue > 0)
		L_BufferCounter += sprintf(localPrintableString + L_BufferCounter,"%c", (char)(CheckDigitValue + 32));
	else if(CheckDigitValue > 94)
		L_BufferCounter += sprintf(localPrintableString + L_BufferCounter,"%c", (char)(CheckDigitValue + 100));
	//tdw for bug 788
	//01Sep05
	//else if(CheckDigitValue = 0)
	else if(CheckDigitValue ==0)
		L_BufferCounter += sprintf(localPrintableString + L_BufferCounter,"%c", (char)(194));	

	//add the stop character
	L_BufferCounter += sprintf(localPrintableString + L_BufferCounter,"%c", StopChar);
	
	// Allow the string to return to the proper size.
	*iSize = (long)strlen(localPrintableString); 
	strncpy(output, localPrintableString, strlen(localPrintableString)); 

	delete [] localPrintableString;		
	delete [] OnlyCorrectData;
	delete [] StringToCheckFiltered;
	delete [] tildeStringToCheck;
	return 0;
} //end Code128()
	

//calculates a mod 10 check digit from the input string
char FindMod10Digit(char *input)
{
	int remainder;
	int M10Factor = 0;
	int M10WeightedTotal = 0;
    int M10StringLength = 0;
	int M10I = 0;

	M10Factor = 3;
	M10WeightedTotal = 0;
    M10StringLength = (int)strlen(input);
	for(M10I = M10StringLength;M10I > 0;M10I--)
	{
		//Get the value of each number starting at the end
		//multiply by the weighting factor which is 3,1,3,1...
		//and add the sum together
         M10WeightedTotal = M10WeightedTotal +  (atoi(mid(input, M10I, 1)) * M10Factor);
		//change factor for next calculation
         M10Factor = 4 - M10Factor;
	}
	remainder = M10WeightedTotal % 10;
	if (remainder == 0) // Zero is the smallest remainder of 10
		return (char)48;//character 48 is the number 0
	else //the ASCII value follows sequentially after 48
		return (char) ((10 - remainder) + 48);  
} //end FindMod10Digit()

/*	Upce function.  Takes in Data To encode, a variable to be populated w/ the return value, and a variable that will be populated
	w/ the size of the return value	*/
long __stdcall IDAutomation_UPCe (char *DataToEncode, char *output, long *iSize)
{
	if(DataToEncode == NULL)
		return 1;
	if(strlen(DataToEncode) == 0)
		return 1;
	
	int StringLength = 0;
	int I = 0;	//for loop counter

	char* LPrintableString;		//our output string..DataToPrint + EANAddOnToPrint
	LPrintableString = new char[512];

	char* ActualDataToEncode;	//after all conversions and substitutions to OnlyCorrectData.  This is UPCE compressed data
	ActualDataToEncode = new char[20];

	char* DataToPrint;			//encoded data before adding start/stop/supplements
	DataToPrint = new char[512];
	
	char* EAN2AddOn;			//2 digit EAN add on
	EAN2AddOn = new char[3];
	
	char* OnlyCorrectData;		//after filtering out any non numerics from DataTOEncode
	OnlyCorrectData = new char[20];

    char* EAN5AddOn;			//5 digit EAN add on
	EAN5AddOn = new char[6];

    char* EANAddOnToPrint;		//After the EAN value is encoded
	EANAddOnToPrint = new char[11];

	int Factor = 3;		//Check digit weighting factor
    int WeightedTotal = 0;		//check digit weighting total
	int CheckDigit = 0;	//check digit value 
	char *Encoding;	//pattern for UPCe encoding
	int CurrentNumber;  //used to obtain numeric values from strings
	char midEncoding;	//char representation of single character for switch statements
	char *sCheck = 0;	//check digit character
	int bufferCounter = 0;	//position to place characters in string using sprintf
	
	//Values used to compress from 12 digits to 8 for UPCe
	char *D1;
	char *D2;
	char *D3;
	char *D4;
	char *D5;
	char *D6;
	char *D7;
	char *D8;
	char *D9;
	char *D10;
	char *D11;
	char *D12;
	
	StringLength = (int)strlen(DataToEncode);
	LPrintableString[0]=(char)0;
	//We are capping this at 20 character because UPCe, at most, only allows 17 numerics
	if(StringLength > 20)
		StringLength = 20;

    //Add all numbers to OnlyCorrectData string
	for(I = 0;I < StringLength;I++)
	{		
		if(isdigit((int)DataToEncode[I]))
		{
			bufferCounter += sprintf(OnlyCorrectData + bufferCounter, "%c", DataToEncode[I]);
		}
	}

	//Remove check digits if they added one
	if(strlen(OnlyCorrectData) < 11)
		sprintf(OnlyCorrectData,"%s","00000000000");
	else if(strlen(OnlyCorrectData) == 15)
		sprintf(OnlyCorrectData, "%s", "00000000000");
	else if(strlen(OnlyCorrectData) > 18)
		sprintf(LPrintableString,"%s","00000000000");

	if(strlen(OnlyCorrectData) == 12)
		sprintf(LPrintableString,"%s", mid(OnlyCorrectData, 1, 11));
	if(strlen(OnlyCorrectData) == 14)
		sprintf(LPrintableString,"%s%s", mid(OnlyCorrectData, 1, 11), mid(OnlyCorrectData, 13, 2));
	if(strlen(OnlyCorrectData) == 17)
		sprintf(LPrintableString,"%s%s", mid(OnlyCorrectData, 1, 11), mid(OnlyCorrectData, 13, 5));

     //Get Add Ons
	if(strlen(OnlyCorrectData) == 16)
		sprintf(EAN5AddOn,"%s", mid(OnlyCorrectData, 12, 5));
    else if(strlen(OnlyCorrectData) == 13)
	{
		sprintf(EAN2AddOn,"%s", mid(OnlyCorrectData, 12, 2));
	}
	else
	{
		sprintf(EAN2AddOn,"%s", "");
		sprintf(EAN5AddOn,"%s", "");
		sprintf(EANAddOnToPrint,"%s", "");
	}

     //OnlyCorrectData needs to be 11 characters.
	if(strlen(OnlyCorrectData) > 11)
		sprintf(OnlyCorrectData,"%s", mid(OnlyCorrectData, 1, 11));
     
	//<<<< Calculate Check Digit >>>>
     Factor = 3;
     WeightedTotal = 0;
	 
	 //we need to go backwards thru OnlyCorrectData for the check digit
	 for(I = (int)strlen(OnlyCorrectData);I >= 0;I--)
	 {     
		//Get the value of each number starting at the end
		CurrentNumber = atoi(mid(OnlyCorrectData, I, 1));

	    //multiply by the weighting factor which is 3,1,3,1... and add the sum together
        WeightedTotal = WeightedTotal + (CurrentNumber * Factor);
		
		//change factor for next calculation
        Factor = 4 - Factor;
     }

	//Find the CheckDigit by finding the number + WeightedTotal that = a multiple of 10
	//divide by 10, get the remainder and subtract from 10
	I = (WeightedTotal % 10);
	if(I != 0)
		CheckDigit = (10 - I);
	else
		CheckDigit = 0;

	sprintf(OnlyCorrectData,"%s%i", OnlyCorrectData, CheckDigit);

	//Compress UPC-A to UPC-E if possible

	D1 = mid(OnlyCorrectData, 1, 1);
	D2 = mid(OnlyCorrectData, 2, 1);
	D3 = mid(OnlyCorrectData, 3, 1);
	D4 = mid(OnlyCorrectData, 4, 1);
	D5 = mid(OnlyCorrectData, 5, 1);
	D6 = mid(OnlyCorrectData, 6, 1);
	D7 = mid(OnlyCorrectData, 7, 1);
	D8 = mid(OnlyCorrectData, 8, 1);
	D9 = mid(OnlyCorrectData, 9, 1);
	D10 = mid(OnlyCorrectData, 10, 1);
	D11 = mid(OnlyCorrectData, 11, 1);
	D12 = mid(OnlyCorrectData, 12, 1);

	//Condition A
	if((atoi(D11) == 5 || atoi(D11) == 6 || atoi(D11) == 7 || atoi(D11) == 8 || atoi(D11) == 9) &&
		(atoi(D6) != 0 && atoi(D7) == 0 && atoi(D8) == 0 && atoi(D9) == 0 && atoi(D10) == 0))
	{
		sprintf(ActualDataToEncode, "%s%s%s%s%s%s", D2, D3, D4, D5, D6, D11);
	}

	//Condition B
	else if((atoi(D6) == 0 && atoi(D7) == 0 && atoi(D8) == 0 && atoi(D9) == 0 && atoi(D10) == 0) && atoi(D5) != 0)
	{
		sprintf(ActualDataToEncode, "%s%s%s%s%s%s", D2, D3, D4, D5, D11, "4");
	}
	
	//Condition C
	else if((atoi(D5) == 0 && atoi(D6) == 0 && atoi(D7) == 0 && atoi(D8) == 0)
		&& (atoi(D4) == 1 || atoi(D4) == 2 || atoi(D4) == 0))
	{
		sprintf(ActualDataToEncode, "%s%s%s%s%s%s", D2, D3, D9, D10, D11, D4);
	}

	//Condition D
	else if((atoi(D5) == 0 && atoi(D6) == 0 && atoi(D7) == 0 && atoi(D8) == 0 && atoi(D9) == 0) &&
		(atoi(D4) == 3 || atoi(D4) == 4 || atoi(D4) == 5 || atoi(D4) == 6 || atoi(D4) == 7 || atoi(D4) == 8 || atoi(D4) == 9) )
	{
		sprintf(ActualDataToEncode, "%s%s%s%s%s%s", D2, D3, D4, D10, D11, "3");
	}
	else

	{
		
		sprintf(ActualDataToEncode, "%s", OnlyCorrectData);
	}
     
	//Run UPC-E compression only if DataToEncode = 6
	if(strlen(ActualDataToEncode) == 6)
	{
		//Now we must encode the check character into the symbol
		//by using variable parity between character sets A and B
		switch(atoi(D12))
		{
			case 0:
				Encoding = "BBBAAA";
				break;
			case 1:
				Encoding = "BBABAA";
				break;
			case 2:
				Encoding = "BBAABA";
				break;
			case 3:
				Encoding = "BBAAAB";
				break;
			case 4:
				Encoding = "BABBAA";
				break;
			case 5:
				Encoding = "BAABBA";
				break;
			case 6:
				Encoding = "BAAABB";
				break;
			case 7:
				Encoding = "BABABA";
				break;
			case 8:
				Encoding = "BABAAB";
				break;
			case 9:
				Encoding = "BAABAB";
				break;

		}
		bufferCounter = 0;
		for(I = 1;I <= (int)strlen(ActualDataToEncode);I++)
		{

			if(I == 1)
			{
				//For the LeadingDigit print the human readable character, the normal guard pattern and then the rest of the barcode
				//This is the first character so we restart the buffer from the begining
				bufferCounter += sprintf(DataToPrint, "%c%s", (char)85, "(");
			}
			//Get the ASCII value of each number
			CurrentNumber = asc(mid(ActualDataToEncode, I, 1));

			//Print different barcodes according to the location of the CurrentChar and CurrentEncoding
			midEncoding = Encoding[I - 1];
			switch(midEncoding)
			{
				case 'A':
					bufferCounter += sprintf(DataToPrint + bufferCounter, "%c", (char)CurrentNumber);
					break;
				case 'B':
					bufferCounter += sprintf(DataToPrint + bufferCounter, "%c", (char)(CurrentNumber + 17));
					break;
			}

            //add in the 1st character along with guard patterns
			switch(I)
			{
				
				case 1:
					
					break;
				case 6:
					//Print the SPECIAL guard pattern and check character
					if(atoi(D12) > 4)
						bufferCounter += sprintf(DataToPrint + bufferCounter, "%s%c", ")", (char)(asc(D12) + 64));
					if(atoi(D12) < 5)
						bufferCounter += sprintf(DataToPrint + bufferCounter, "%s%c", ")", (char)(asc(D12) + 37));
					break;
			}//end switch
		}//end for loop 
	} // end if
     
	//if we weren't able to compress DataToEncode into 6 characters....
	//determine character to print for proper upc-a barcoding
	bufferCounter = 0;
	if(strlen(ActualDataToEncode) != 6)
	{
		for(I = 1;I <= (int)strlen(ActualDataToEncode);I++)
		{
			//Get the ASCII value of each number
			CurrentNumber = asc(mid(ActualDataToEncode, I, 1));

			//Print different barcodes according to the location of the CurrentChar
			switch(I)
			{
				case 1:
					//For the first character print the human readable character, the normal
					//guard pattern and then the barcode without the human readable character
                    if(atoi(mid(ActualDataToEncode, I, 1)) > 4)
						bufferCounter += sprintf(DataToPrint + bufferCounter, "%c%s%c", (char)(CurrentNumber + 64), "(", (char)(CurrentNumber + 49));
					if(atoi(mid(ActualDataToEncode, I, 1)) < 5)
						bufferCounter += sprintf(DataToPrint + bufferCounter, "%c%s%c", (char)(CurrentNumber + 37), "(", (char)(CurrentNumber + 49));
					break;
				case 2:
					bufferCounter += sprintf(DataToPrint + bufferCounter, "%c", mid(ActualDataToEncode, I, 1));
					break;
				case 3:
					bufferCounter += sprintf(DataToPrint + bufferCounter, "%c", mid(ActualDataToEncode, I, 1));
					break;
				case 4:
					bufferCounter += sprintf(DataToPrint + bufferCounter, "%c", mid(ActualDataToEncode, I, 1));
					break;
				case 5:
					bufferCounter += sprintf(DataToPrint + bufferCounter, "%c", mid(ActualDataToEncode, I, 1));
					break;
				case 6:
					//Print the center guard pattern after the 6th character
					bufferCounter += sprintf(DataToPrint + bufferCounter, "%c%s", mid(ActualDataToEncode, I, 1), "*");
					break;
				case 7:
					//Add 27 to the ASII value of characters 6-12 to print from character set+ C
					//this is required when printing to the right of the center guard pattern
					bufferCounter += sprintf(DataToPrint + bufferCounter, "%c", (char)(CurrentNumber + 27));
					break;
				case 8:
					bufferCounter += sprintf(DataToPrint + bufferCounter, "%c", (char)(CurrentNumber + 27));
					break;
				case 9:
					bufferCounter += sprintf(DataToPrint + bufferCounter, "%c", (char)(CurrentNumber + 27));
					break;
				case 10:
					bufferCounter += sprintf(DataToPrint + bufferCounter, "%c", (char)(CurrentNumber + 27));
					break;
				case 11:
					bufferCounter += sprintf(DataToPrint + bufferCounter, "%c", (char)(CurrentNumber + 27));
					break;
				case 12:
					//For the last character print the barcode without the human readable character,
					//the normal guard pattern and then the human readable character.
					if(atoi(mid(ActualDataToEncode, I, 1)) > 4)
						bufferCounter += sprintf(DataToPrint + bufferCounter, "%c%s%c", (char)(CurrentNumber + 59), ")", (char)(CurrentNumber + 64));
					if(atoi(mid(ActualDataToEncode, I, 1)) < 5)
						bufferCounter += sprintf(DataToPrint + bufferCounter, "%c%s%c", (char)(CurrentNumber + 59), ")", (char)(CurrentNumber + 37));
					break;
			}//end switch
		}//end for loop thru characters
	} //end if not 6 characters
     
	//Process 5 digit add on if it exists
	if(strlen(EAN5AddOn) == 5)
	{
		//Get check digit for add on
		Factor = 3;
		WeightedTotal = 0;
		for(I = (int)strlen(EAN5AddOn);I >= 0;I--)
		{
			//Get the value of each number starting at the end
            CurrentNumber = atoi(mid(EAN5AddOn, I, 1));
			//multiply by the weighting factor which is 3,9,3,9...and add the sum together
			if(Factor = 3)
				WeightedTotal = WeightedTotal + (CurrentNumber * 3);
			if(Factor = 1 )
				WeightedTotal = WeightedTotal + (CurrentNumber * 9);
        
			//change factor for next calculation
            Factor = 4 - Factor;
		} //end loop thru characters
		//Find the CheckDigit by extracting the right-most number from WeightedTotal
		
		sprintf(sCheck,"%i", WeightedTotal);

		CheckDigit = atoi(mid(sCheck, (int)strlen(sCheck), 1));
		
		//Now we must encode the add-on CheckDigit into the number sets
		//by using variable parity between character sets A and B
		switch(CheckDigit)
		{
			case 0:
               Encoding = "BBAAA";
			   break;
			case 1:
               Encoding = "BABAA";
			   break;
			case 2:
               Encoding = "BAABA";
			   break;
			case 3:
               Encoding = "BAAAB";
			   break;
			case 4:
               Encoding = "ABBAA";
			   break;
			case 5:
               Encoding = "AABBA";
			   break;
			case 6:
               Encoding = "AAABB";
			   break;
			case 7:
               Encoding = "ABABA";
			   break;
			case 8:
               Encoding = "ABAAB";
			   break;
			case 9:
               Encoding = "AABAB";
			   break;
		}
		//Now that we have the total number including the check digit, determine character to print for proper barcoding:
		bufferCounter = 0;
		for(I = 1;I <= (int)strlen(EAN5AddOn);I++)
		{
			//Get the value of each number.  It is encoded with variable parity
            CurrentNumber = atoi(mid(EAN5AddOn, I, 1));
			//Print different barcodes according to the location of the CurrentChar and CurrentEncoding
			midEncoding = Encoding[I - 1];
			switch(midEncoding)
			{
				case 'A':
					if(CurrentNumber == 0) 
						bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)34);
                    if(CurrentNumber == 1) 
						bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)35);
                    if(CurrentNumber == 2) 
						bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)36);
                    if(CurrentNumber == 3) 
						bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)37);
                    if(CurrentNumber == 4) 
						bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)38);
                    if(CurrentNumber == 5) 
						bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)44);
                    if(CurrentNumber == 6) 
						bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)46);
                    if(CurrentNumber == 7) 
						bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)47);
                    if(CurrentNumber == 8) 
						bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)58);
                    if(CurrentNumber == 9) 
						bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)59);
					break;

				case 'B':
					if(CurrentNumber == 0) 
						bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)122);
                    if(CurrentNumber == 1) 
						bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)61);
                    if(CurrentNumber == 2) 
						bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)63);
                    if(CurrentNumber == 3) 
						bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)64);
                    if(CurrentNumber == 4) 
						bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)91);
                    if(CurrentNumber == 5) 
						bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)92);
                    if(CurrentNumber == 6) 
						bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)93);
                    if(CurrentNumber == 7) 
						bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)95);
                    if(CurrentNumber == 8) 
						bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)123);
                    if(CurrentNumber == 9) 
						bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)125);
					break;
			}
			
			//add in the space & add-on guard pattern
			switch(I)
			{
				case 1:
					bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c%s%c", (char)43, EANAddOnToPrint, (char)33);
					break;
					//Now print add-on delineators between each add-on character
				case 2:
					bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)33);
					break;
				case 3:
                    bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)33);
					break;
				case 4:
                    bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)33);
					break;
				case 5:
                    //EANAddOnToPrint = EANAddOnToPrint
					break;
			}
		} //end for loop
	}	//end 5 digit supplement work
	
	//Now for the 2 digit add on
	if(strlen(EAN2AddOn) == 2)
	{
		//Get encoding for add on
		for(I = 0;I <= 99;I++)
		{
			if(atoi(EAN2AddOn) == I)
				Encoding = "AA";
            if(atoi(EAN2AddOn) == I + 1)
				Encoding = "AB";
            if(atoi(EAN2AddOn) == I + 2)
				Encoding = "BA";
            if(atoi(EAN2AddOn) == I + 3)
				Encoding = "BB";
		}//end for loop
          
		bufferCounter = 0;
		//Now that we have the total number including the encoding determine what to print
		for(I = 1;I <= (int)strlen(EAN2AddOn);I++)
		{
			CurrentNumber = atoi(mid(EAN2AddOn, I, 1));
			midEncoding = Encoding[I - 1];
			//Print different barcodes according to the location of the CurrentChar and CurrentEncoding
			switch(midEncoding)
			{
				case 'A':
					if(CurrentNumber == 0) 
						bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)34);
					if(CurrentNumber == 1) 
						bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)35);
					if(CurrentNumber == 2) 
						bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)36);
					if(CurrentNumber == 3) 
						bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)37);
					if(CurrentNumber == 4) 
						bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)38);
					if(CurrentNumber == 5) 
						bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)44);
					if(CurrentNumber == 6) 
						bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)46);
					if(CurrentNumber == 7) 
						bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)47);
					if(CurrentNumber == 8) 
						bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)58);
					if(CurrentNumber == 9) 
						bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)59);
					break;

				case 'B':
					if(CurrentNumber == 0) 
						bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)122);
					if(CurrentNumber == 1) 
						bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)61);
					if(CurrentNumber == 2) 
						bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)63);
					if(CurrentNumber == 3) 
						bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)64);
					if(CurrentNumber == 4) 
						bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)91);
					if(CurrentNumber == 5) 
						bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)92);
					if(CurrentNumber == 6) 
						bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)93);
					if(CurrentNumber == 7) 
						bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)95);
					if(CurrentNumber == 8) 
						bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)123);
					if(CurrentNumber == 9) 
						bufferCounter += sprintf(EANAddOnToPrint + bufferCounter, "%c", (char)125);
					break;
			} //end switch
			//add in the space & add-on guard pattern
			switch(I)
			{
				case 1:
					bufferCounter += sprintf(EANAddOnToPrint, "%c%s%c", (char)43, EANAddOnToPrint, (char)33);
					break;
				case 2:
					//EANAddOnToPrint = EANAddOnToPrint
					break;
			}
		}//end loop thru characers
	} //end ean 2 digit supp processing
     
	//Now we have everything together
	sprintf(LPrintableString, "%s%s", DataToPrint, EANAddOnToPrint);
	
	// Allow the string to return to the proper size.
	*iSize = (long)strlen(LPrintableString); 
	strncpy(output, LPrintableString, strlen(LPrintableString)); 

	delete[] LPrintableString;
	delete[] ActualDataToEncode;
	delete[] OnlyCorrectData;
	delete[] DataToPrint;
	delete[] EAN2AddOn;
	delete[] EAN5AddOn;
	delete[] EANAddOnToPrint;    
	return 0;
} //end upce

