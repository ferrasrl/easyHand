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
/*  Code128b("abc 123-45";                                          */
/*                                                                   */
/*  Distributing our source code or fonts outside your               */
/*  organization requires a distribution license.                    */
/*********************************************************************/

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/ehtool/BarCode.h"
#include "/easyhand/ehtool/imgutil.h"

/*
EH_IMG _Barcode(CHAR *pszBarCode, double h, double w, unsigned int len);

EH_IMG EAN13(CHAR *pszBarCode, double h, double w) {
  
	return _Barcode(pszBarCode, h, w, 13);
}

EH_IMG UPC_A(CHAR *pszBarCode, double h, double w)
{
	return _Barcode(pszBarCode, h, w, 12);
}

//
// _GetCheckDigit()
//
CHAR _GetCheckDigit(CHAR *pszBarCode)
{
	//Compute the check digit
	int i, digit, r;
	CHAR cChar;
	int sum = 0;

	for ( i = 1; i <= 11; i += 2)
	{
		digit = pszBarCode[i] - '0';//'0';
		sum += 3 * digit;
	}

	for (i = 0; i <= 10; i += 2)
	{
		digit = pszBarCode[i] - '0';
		sum += digit;
	}

	r = sum % 10;
	if (r > 0)
	{
		r = 10 - r;
	}
	cChar = '0' + r;
	return cChar;
}

//
// TestCheckDigit()
//
BOOL _TestCheckDigit(CHAR *pszBarCode)
{
  //Test validity of check digit
  int i, digit;
  int sum = 0;
  for (i = 1; i <= 11; i += 2)
  {
    digit = pszBarCode[i] - '0';
    sum += 3 * digit;
  }
  for (i = 0; i <= 10; i += 2)
  {
    digit = pszBarCode[i] - '0';
    sum += digit;
  }
  digit = pszBarCode[12] - '0';
  return (sum + digit) % 10 == 0;
}

// Code and parity constants for EAN13 and UPC_A
static BYTE * bc_codes[3][10] = {
  {
    "0001101","0011001","0010011","0111101","0100011",
    "0110001","0101111","0111011","0110111","0001011"
  },
  {
    "0100111","0110011","0011011","0100001","0011101",
    "0111001","0000101","0010001","0001001","0010111"
  },
  {
    "1110010","1100110","1101100","1000010","1011100",
    "1001110","1010000","1000100","1001000","1110100"
  } };


static int bc_parities[10][6] = {
  { 0, 0, 0, 0, 0, 0 },
  { 0, 0, 1, 0, 1, 1 },
  { 0, 0, 1, 1, 0, 1 },
  { 0, 0, 1, 1, 1, 0 },
  { 0, 1, 0, 0, 1, 1 },
  { 0, 1, 1, 0, 0, 1 },
  { 0, 1, 1, 1, 0, 0 },
  { 0, 1, 0, 1, 0, 1 },
  { 0, 1, 0, 1, 1, 0 },
  { 0, 1, 1, 0, 1, 0 } };


//
// _Barcode()
//
EH_IMG _Barcode(CHAR *pszBarCode, double h, double w, unsigned int len)
{
	BYTE szBar[200];
	BYTE szCode[300];
	int i,digit;
	int *piParity;
	//
	// Padding
	//
	int padlen = len - 1 - strlen(pszBarCode);//(int) pszBarCode.Length();

//	BYTE locBarcode = pszBarCode;
	memset(szBar,'0',padlen); szBar[padlen]=0;
//	locBarcode.Pad(padlen, '0', false); //str_pad($pszBarCode,$len-1,'0',STR_PAD_LEFT);
	if (len == 12)
	{
		//locBarcode = "0" + locBarcode;
		strIns(szBar,"0");
	}
	//Add or control the check digit
	if (strlen(szBar) == 12)
	{
//		locBarcode += BYTE(GetCheckDigit(locBarcode));
		strAppend(szBar),"%c",_GetCheckDigit(szBar));
	}
	else if (!_TestCheckDigit(szBar))
	{
		//$this->Error('Incorrect check digit');
		return FALSE;
	}

	//
	// Convert digits to bars
	//
	strcpy(szCode,"101");
	digit = szBar[0]-'0';
	piParity=bc_parities+digit;
	for (i = 1; i <= 6; i++)
	{
		digit = szBar[i] - '0';
		strcat(szCode,bc_codes[p[i-1]][digit]);
//		code += bc_codes[p[i-1]][digit];
	}
	strcat(szCode,"01010");
	for (i = 7; i <= 12; i++)
	{
		digit = locBarcode[i] - '0';
		strcat(szCode,bc_codes[2][digit]);
		//code += bc_codes[2][digit];
	}
	strcat(szCode,"101");

	//
	// Creo il bitmap
	//

	//
	// Draw bars 
	//
	for (i = 0; i < strlen(szCode); i++)
	{
//		if (code[i] == wxT('1'))
//		{
//		  m_document->Rect(x + i * w, y, w, h, wxPDF_STYLE_FILL);
//		}
	}
	//Print text under pszBarCode
//	m_document->SetFont("Arial", "", 12);
//	m_document->Text(x, y + h + 11 / m_document->GetScaleFactor(), locBarcode.Right(len));
	return 0;
}

// Character set constant for Code39
static BYTE * code39_chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ-. $/+%*";

//Conversion tables for Code39
static BYTE * code39_narrowEncoding[] = {
  "101001101101", "110100101011", "101100101011",
  "110110010101", "101001101011", "110100110101",
  "101100110101", "101001011011", "110100101101",
  "101100101101", "110101001011", "101101001011",
  "110110100101", "101011001011", "110101100101",
  "101101100101", "101010011011", "110101001101",
  "101101001101", "101011001101", "110101010011",
  "101101010011", "110110101001", "101011010011",
  "110101101001", "101101101001", "101010110011",
  "110101011001", "101101011001", "101011011001",
  "110010101011", "100110101011", "110011010101",
  "100101101011", "110010110101", "100110110101",
  "100101011011", "110010101101", "100110101101",
  "100100100101", "100100101001", "100101001001",
  "101001001001", "100101101101" };

static BYTE * code39_wideEncoding[] = {
  "101000111011101", "111010001010111", "101110001010111",
  "111011100010101", "101000111010111", "111010001110101",
  "101110001110101", "101000101110111", "111010001011101",
  "101110001011101", "111010100010111", "101110100010111",
  "111011101000101", "101011100010111", "111010111000101",
  "101110111000101", "101010001110111", "111010100011101",
  "101110100011101", "101011100011101", "111010101000111",
  "101110101000111", "111011101010001", "101011101000111",
  "111010111010001", "101110111010001", "101010111000111",
  "111010101110001", "101110101110001", "101011101110001",
  "111000101010111", "100011101010111", "111000111010101",
  "100010111010111", "111000101110101", "100011101110101",
  "100010101110111", "111000101011101", "100011101011101",
  "100010001000101", "100010001010001", "100010100010001",
  "101000100010001", "100010111011101" };

bool
wxPdfBarCodeCreator::Code39(double x, double y, const BYTE& code, bool ext, bool cks, double w, double h, bool wide)
{
  BYTE locCode = code;
  //Display code
  m_document->SetFont("Arial", "", 10);
  m_document->Text(x, y + h + 4, locCode);

  if (ext)
  {
    if (!locCode.IsAscii())
    {
      // code contains invalid character(s)
      return false;
    }
    //Extended encoding
    locCode = EncodeCode39Ext(locCode);
  }
  else
  {
    //Convert to upper case
    locCode.UpperCase();
    //Check validity
    size_t j;
    bool valid = true;
    for (j = 0; valid && j < locCode.Length(); j++)
    {
      valid = valid && locCode[j] != wxT('*') && code39_chars.Find(locCode[j]) >= 0;
    }
    if (!valid)
    {
      //$this->Error('Invalid pszBarCode value: '.$code);
      return false;
    }
  }

  //Compute checksum
  if (cks)
  {
    locCode += ChecksumCode39(locCode);
  }

  //Add start and stop characters
  locCode = "*" + locCode + "*";

  BYTE* encoding = wide ? code39_wideEncoding : code39_narrowEncoding;

  //Inter-character spacing
  BYTE gap = (w > 0.29) ? "00" : "0";

  //Convert to bars
  BYTE encode = "";
  size_t i;
  for (i = 0; i< locCode.Length(); i++)
  {
    int pos = code39_chars.Find(locCode[i]);
    encode += encoding[pos] + gap;
  }

  //Draw bars
  DrawCode39(encode, x, y, w, h);
  return true;
}


wxChar
wxPdfBarCodeCreator::ChecksumCode39(const BYTE& code)
{

  //Compute the modulo 43 checksum

  int sum = 0;
  size_t i;
  for (i = 0; i < code.Length(); i++)
  {
    sum += code39_chars.Find(code[i]);
  }
  int r = sum % 43;
  return code39_chars[r];
}

// Encoding table for Code39 Extended
static BYTE code39_encode[] = {
  "%U", "$A", "$B", "$C",
  "$D", "$E", "$F", "$G",
  "$H", "$I", "$J", "$K",
  "$L", "$M", "$N", "$O",
  "$P", "$Q", "$R", "$S",
  "$T", "$U", "$V", "$W",
  "$X", "$Y", "$Z", "%A",
  "%B", "%C", "%D", "%E",
  " ",  "/A", "/B", "/C",
  "/D", "/E", "/F", "/G",
  "/H", "/I", "/J", "/K",
  "/L", "-",  ".",  "/O",
  "0",  "1",  "2",  "3",
  "4",  "5",  "6",  "7",
  "8",  "9",  "/Z", "%F",
  "%G", "%H", "%I", "%J",
  "%V", "A",  "B",  "C",
  "D",  "E",  "F",  "G",
  "H",  "I",  "J",  "K",
  "L",  "M",  "N",  "O",
  "P",  "Q",  "R",  "S",
  "T",  "U",  "V",  "W",
  "X",  "Y",  "Z",  "%K",
  "%L", "%M", "%N", "%O",
  "%W", "+A", "+B", "+C",
  "+D", "+E", "+F", "+G",
  "+H", "+I", "+J", "+K",
  "+L", "+M", "+N", "+O",
  "+P", "+Q", "+R", "+S",
  "+T", "+U", "+V", "+W",
  "+X", "+Y", "+Z", "%P",
  "%Q", "%R", "%S", "%T" };

BYTE
wxPdfBarCodeCreator::EncodeCode39Ext(const BYTE& code)
{

  //Encode characters in extended mode
  BYTE codeExt = "";
  size_t i;
  for (i = 0 ; i < code.Length(); i++)
  {
    codeExt += code39_encode[code[i]];
  }
  return codeExt;
}

void
wxPdfBarCodeCreator::DrawCode39(const BYTE& code, double x, double y, double w, double h)
{
  //Draw bars
  size_t i;
  for (i = 0; i < code.Length(); i++)
  {
    if (code[i] == wxT('1'))
    {
      m_document->Rect(x + i * w, y, w, h, wxPDF_STYLE_FILL);
    }
  }
}

// Character and pszBarCode constants for I25
static BYTE i25_chars = "0123456789AZ";
static BYTE i25_barChar[] = {
  "nnwwn", "wnnnw", "nwnnw", "wwnnn", "nnwnw",
  "wnwnn", "nwwnn", "nnnww", "wnnwn", "nwnwn",
  "nn", "wn" };

bool
wxPdfBarCodeCreator::I25(double xpos, double ypos, const BYTE& code, double basewidth, double height)
{
  // wide/narrow codes for the digits
  BYTE locCode = code;
  double wide = basewidth;
  double narrow = basewidth / 3 ;
  double lineWidth;

  if ((locCode.Length() > 0 && !wxIsdigit(locCode[0])) || !locCode.IsNumber())
  {
    return false;
  }

  // add leading zero if code-length is odd
  if (locCode.Length() % 2 != 0)
  {
    locCode = "0" + locCode;
  }

  m_document->SetFont("Arial", "", 10);
  m_document->Text(xpos, ypos + height + 4, locCode);
  m_document->SetFillColour(0);

  // add start and stop codes
  locCode = "AA" + locCode + "ZA";

  size_t i;
  for (i = 0; i < locCode.Length(); i += 2)
  {
    // choose next pair of digits
    int digitBar = i25_chars.Find(locCode[i]);
    int digitSpace = i25_chars.Find(locCode[i+1]);

    // create a wide/narrow-sequence (first digit=bars, second digit=spaces)
    BYTE seq = "";
    size_t j;
    for (j = 0; j < i25_barChar[digitBar].Length(); j++)
    {
      seq += BYTE(i25_barChar[digitBar][j]) + BYTE(i25_barChar[digitSpace][j]);
    }
    for (j = 0; j < seq.Length(); j++)
    {
      // set lineWidth depending on value
      lineWidth = (seq[j] == wxT('n')) ? narrow : wide;
      // draw every second value, because the second digit of the pair is represented by the spaces
      if (j % 2 == 0)
      {
        m_document->Rect(xpos, ypos, lineWidth, height, wxPDF_STYLE_FILL);
      }
      xpos += lineWidth;
    }
  }
  return true;
}

  // draws a bar code for the given zip code using pdf lines
  // triggers error if zip code is invalid
  // x,y specifies the lower left corner of the bar code
bool
wxPdfBarCodeCreator::PostNet(double x, double y, const BYTE& zipcode)
{
  // Save nominal bar dimensions in user units
  // Full Bar Nominal Height = 0.125"
  double fullBarHeight = 9 / m_document->GetScaleFactor();
  // Half Bar Nominal Height = 0.050"
  double halfBarHeight = 3.6 / m_document->GetScaleFactor();
  // Full and Half Bar Nominal Width = 0.020"
  double barWidth = 1.44 / m_document->GetScaleFactor();
  // Bar Spacing = 0.050"
  double barSpacing = 3.6 / m_document->GetScaleFactor();

  double fiveBarSpacing = barSpacing * 5;

  // validate the zip code
  if (!ZipCodeValidate(zipcode))
  {
    return false;
  }

  // set the line width
  m_document->SetLineWidth(barWidth);

  // draw start frame bar
  m_document->Line(x, y, x, y - fullBarHeight);
  x += barSpacing;

  // draw digit bars
  size_t i;
  int digit;
  for (i = 0; i < zipcode.Length(); i++)
  {
    if (i != 5)
    {
      digit = zipcode[i] - '0';
      ZipCodeDrawDigitBars(x, y, barSpacing, halfBarHeight, fullBarHeight, digit);
      x += fiveBarSpacing;
    }
  }

  // draw check sum digit
  digit = ZipCodeCheckSumDigit(zipcode);
  ZipCodeDrawDigitBars(x, y, barSpacing, halfBarHeight, fullBarHeight, digit);
  x += fiveBarSpacing;

  // draw end frame bar
  m_document->Line(x, y, x, y - fullBarHeight);
  return true;
}

// valid zip codes are of the form DDDDD or DDDDD-DDDD
// where D is a digit from 0 to 9, returns the validated zip code
bool
wxPdfBarCodeCreator::ZipCodeValidate(const BYTE& zipcode)
{
  bool valid = true;
  if (zipcode.Length() == 5 || zipcode.Length() == 10)
  {
    // check that all characters are numeric
    size_t i;
    for (i = 0; valid && i < zipcode.Length(); i++ )
    {
      if ((i != 5 && !wxIsdigit(zipcode[i])) || (i == 5 && zipcode[5] != wxT('-')))
      {
        valid = false;
      }
    }
  }
  else
  {
    valid = false;
  }
  return valid;
}

// takes a validated zip code and
// calculates the checksum for POSTNET
int
wxPdfBarCodeCreator::ZipCodeCheckSumDigit(const BYTE& zipcode)
{
  // calculate sum of digits
  size_t i;
  int sum = 0;
  for (i = 0; i < zipcode.Length(); i++)
  {
    if (i != 5)
    {
      sum += (zipcode[i] - '0');
    }
  }

  // return checksum digit
  int r = sum % 10;
  if (r > 0)
  {
    r = 10 - r;
  }
  return r;
}

// Bar definitions for Postnet
// 1 represents full-height bars and 0 represents half-height bars
static int postnet_barDefinitions[10][5] = {
  { 1, 1, 0, 0, 0 },
  { 0, 0, 0, 1, 1 },
  { 0, 0, 1, 0, 1 },
  { 0, 0, 1, 1, 0 },
  { 0, 1, 0, 0, 1 },
  { 0, 1, 0, 1, 0 },
  { 0, 1, 1, 0, 0 },
  { 1, 0, 0, 0, 1 },
  { 1, 0, 0, 1, 0 },
  { 1, 0, 1, 0, 0 } };

// Takes a digit and draws the corresponding POSTNET bars.
void
wxPdfBarCodeCreator::ZipCodeDrawDigitBars(double x, double y, double barSpacing,
                                           double halfBarHeight, double fullBarHeight,
                                           int digit)
{
  // check for invalid digit
  if (digit >= 0 && digit <= 9)
  {
    // draw the five bars representing a digit
    int i;
    for (i = 0; i < 5; i++)
    {
      if (postnet_barDefinitions[digit][i] == 1)
      {
        m_document->Line(x, y, x, y - fullBarHeight);
      }
      else
      {
        m_document->Line(x, y, x, y - halfBarHeight);
      }
      x += barSpacing;
    }
  }
}

// --- Code128 implementation ---

// Code128 bar specification
static short code128_bars[108][6] = {
  { 2, 1, 2, 2, 2, 2 },  //  0 : [ ]
  { 2, 2, 2, 1, 2, 2 },  //  1 : [!]
  { 2, 2, 2, 2, 2, 1 },  //  2 : ["]
  { 1, 2, 1, 2, 2, 3 },  //  3 : [#]
  { 1, 2, 1, 3, 2, 2 },  //  4 : [$]
  { 1, 3, 1, 2, 2, 2 },  //  5 : [%]
  { 1, 2, 2, 2, 1, 3 },  //  6 : [&]
  { 1, 2, 2, 3, 1, 2 },  //  7 : [']
  { 1, 3, 2, 2, 1, 2 },  //  8 : [(]
  { 2, 2, 1, 2, 1, 3 },  //  9 : [)]

  { 2, 2, 1, 3, 1, 2 },  // 10 : [*]
  { 2, 3, 1, 2, 1, 2 },  // 11 : [+]
  { 1, 1, 2, 2, 3, 2 },  // 12 : [,]
  { 1, 2, 2, 1, 3, 2 },  // 13 : [-]
  { 1, 2, 2, 2, 3, 1 },  // 14 : [.]
  { 1, 1, 3, 2, 2, 2 },  // 15 : [/]
  { 1, 2, 3, 1, 2, 2 },  // 16 : [0]
  { 1, 2, 3, 2, 2, 1 },  // 17 : [1]
  { 2, 2, 3, 2, 1, 1 },  // 18 : [2]
  { 2, 2, 1, 1, 3, 2 },  // 19 : [3]

  { 2, 2, 1, 2, 3, 1 },  // 20 : [4]
  { 2, 1, 3, 2, 1, 2 },  // 21 : [5]
  { 2, 2, 3, 1, 1, 2 },  // 22 : [6]
  { 3, 1, 2, 1, 3, 1 },  // 23 : [7]
  { 3, 1, 1, 2, 2, 2 },  // 24 : [8]
  { 3, 2, 1, 1, 2, 2 },  // 25 : [9]
  { 3, 2, 1, 2, 2, 1 },  // 26 : [:]
  { 3, 1, 2, 2, 1, 2 },  // 27 : [;]
  { 3, 2, 2, 1, 1, 2 },  // 28 : [<]
  { 3, 2, 2, 2, 1, 1 },  // 29 : [=]

  { 2, 1, 2, 1, 2, 3 },  // 30 : [>]
  { 2, 1, 2, 3, 2, 1 },  // 31 : [?]
  { 2, 3, 2, 1, 2, 1 },  // 32 : [@]
  { 1, 1, 1, 3, 2, 3 },  // 33 : [A]
  { 1, 3, 1, 1, 2, 3 },  // 34 : [B]
  { 1, 3, 1, 3, 2, 1 },  // 35 : [C]
  { 1, 1, 2, 3, 1, 3 },  // 36 : [D]
  { 1, 3, 2, 1, 1, 3 },  // 37 : [E]
  { 1, 3, 2, 3, 1, 1 },  // 38 : [F]
  { 2, 1, 1, 3, 1, 3 },  // 39 : [G]

  { 2, 3, 1, 1, 1, 3 },  // 40 : [H]
  { 2, 3, 1, 3, 1, 1 },  // 41 : [I]
  { 1, 1, 2, 1, 3, 3 },  // 42 : [J]
  { 1, 1, 2, 3, 3, 1 },  // 43 : [K]
  { 1, 3, 2, 1, 3, 1 },  // 44 : [L]
  { 1, 1, 3, 1, 2, 3 },  // 45 : [M]
  { 1, 1, 3, 3, 2, 1 },  // 46 : [N]
  { 1, 3, 3, 1, 2, 1 },  // 47 : [O]
  { 3, 1, 3, 1, 2, 1 },  // 48 : [P]
  { 2, 1, 1, 3, 3, 1 },  // 49 : [Q]

  { 2, 3, 1, 1, 3, 1 },  // 50 : [R]
  { 2, 1, 3, 1, 1, 3 },  // 51 : [S]
  { 2, 1, 3, 3, 1, 1 },  // 52 : [T]
  { 2, 1, 3, 1, 3, 1 },  // 53 : [U]
  { 3, 1, 1, 1, 2, 3 },  // 54 : [V]
  { 3, 1, 1, 3, 2, 1 },  // 55 : [W]
  { 3, 3, 1, 1, 2, 1 },  // 56 : [X]
  { 3, 1, 2, 1, 1, 3 },  // 57 : [Y]
  { 3, 1, 2, 3, 1, 1 },  // 58 : [Z]
  { 3, 3, 2, 1, 1, 1 },  // 59 : [[]

  { 3, 1, 4, 1, 1, 1 },  // 60 : [\]
  { 2, 2, 1, 4, 1, 1 },  // 61 : []]
  { 4, 3, 1, 1, 1, 1 },  // 62 : [^]
  { 1, 1, 1, 2, 2, 4 },  // 63 : [_]
  { 1, 1, 1, 4, 2, 2 },  // 64 : [`]
  { 1, 2, 1, 1, 2, 4 },  // 65 : [a]
  { 1, 2, 1, 4, 2, 1 },  // 66 : [b]
  { 1, 4, 1, 1, 2, 2 },  // 67 : [c]
  { 1, 4, 1, 2, 2, 1 },  // 68 : [d]
  { 1, 1, 2, 2, 1, 4 },  // 69 : [e]

  { 1, 1, 2, 4, 1, 2 },  // 70 : [f]
  { 1, 2, 2, 1, 1, 4 },  // 71 : [g]
  { 1, 2, 2, 4, 1, 1 },  // 72 : [h]
  { 1, 4, 2, 1, 1, 2 },  // 73 : [i]
  { 1, 4, 2, 2, 1, 1 },  // 74 : [j]
  { 2, 4, 1, 2, 1, 1 },  // 75 : [k]
  { 2, 2, 1, 1, 1, 4 },  // 76 : [l]
  { 4, 1, 3, 1, 1, 1 },  // 77 : [m]
  { 2, 4, 1, 1, 1, 2 },  // 78 : [n]
  { 1, 3, 4, 1, 1, 1 },  // 79 : [o]

  { 1, 1, 1, 2, 4, 2 },  // 80 : [p]
  { 1, 2, 1, 1, 4, 2 },  // 81 : [q]
  { 1, 2, 1, 2, 4, 1 },  // 82 : [r]
  { 1, 1, 4, 2, 1, 2 },  // 83 : [s]
  { 1, 2, 4, 1, 1, 2 },  // 84 : [t]
  { 1, 2, 4, 2, 1, 1 },  // 85 : [u]
  { 4, 1, 1, 2, 1, 2 },  // 86 : [v]
  { 4, 2, 1, 1, 1, 2 },  // 87 : [w]
  { 4, 2, 1, 2, 1, 1 },  // 88 : [x]
  { 2, 1, 2, 1, 4, 1 },  // 89 : [y]

  { 2, 1, 4, 1, 2, 1 },  // 90 : [z]
  { 4, 1, 2, 1, 2, 1 },  // 91 : [{]
  { 1, 1, 1, 1, 4, 3 },  // 92 : [|]
  { 1, 1, 1, 3, 4, 1 },  // 93 : [}]
  { 1, 3, 1, 1, 4, 1 },  // 94 : [~]
  { 1, 1, 4, 1, 1, 3 },  // 95 : [DEL]
  { 1, 1, 4, 3, 1, 1 },  // 96 : [FNC3]
  { 4, 1, 1, 1, 1, 3 },  // 97 : [FNC2]
  { 4, 1, 1, 3, 1, 1 },  // 98 : [SHIFT]
  { 1, 1, 3, 1, 4, 1 },  // 99 : [Cswap]

  { 1, 1, 4, 1, 3, 1 },  //100 : [Bswap]
  { 3, 1, 1, 1, 4, 1 },  //101 : [Aswap]
  { 4, 1, 1, 1, 3, 1 },  //102 : [FNC1]
  { 2, 1, 1, 4, 1, 2 },  //103 : [Astart]
  { 2, 1, 1, 2, 1, 4 },  //104 : [Bstart]
  { 2, 1, 1, 2, 3, 2 },  //105 : [Cstart]
  { 2, 3, 3, 1, 1, 1 },  //106 : [STOP]
  { 2, 1, 0, 0, 0, 0 }   //107 : [END BAR]
};

// Code128 special codes
const wxChar CODE128_FNC3_INDEX  =  96;
const wxChar CODE128_FNC2_INDEX  =  97;
const wxChar CODE128_SHIFT       =  98;
const wxChar CODE128_CODE_TO_C   =  99;
const wxChar CODE128_CODE_TO_B   = 100;
const wxChar CODE128_CODE_TO_A   = 101;
const wxChar CODE128_FNC1_INDEX  = 102;
const wxChar CODE128_START_A     = 103;
const wxChar CODE128_START_B     = 104;
const wxChar CODE128_START_C     = 105;
const wxChar CODE128_BARS_STOP   = 106;
const wxChar CODE128_ENDBAR      = 107;

// Code128 internal functions

static bool Code128ValidChar(wxChar ch)
{
  return (ch >= 0 && ch <= 127) || (ch >= CODE128_FNC1 && ch <= CODE128_FNC4);
}

static bool Code128ValidInCodeSetA(wxChar ch)
{
  return (ch >= 0 && ch <= 95) || (ch >= CODE128_FNC1 && ch <= CODE128_FNC4);
}

static bool Code128ValidInCodeSetB(wxChar ch)
{
  return (ch >= 32 && ch <= 127) || (ch >= CODE128_FNC1 && ch <= CODE128_FNC4);
}

static bool Code128ValidInCodeSetC(wxChar ch)
{
  return (ch >= '0' && ch <= '9');
}

static void Code128AddCheck(BYTE& pszBarCode)
{
  size_t k = 1;
  BYTE::const_iterator ch = pszBarCode.begin();
  int chk = *ch;
  for (++ch; ch != pszBarCode.end(); ++ch, ++k)
  {
    chk += (int)(*ch) * k;
  }
  chk = chk % 103;
  pszBarCode += wxChar(chk);
  pszBarCode += CODE128_BARS_STOP;
  pszBarCode += CODE128_ENDBAR;
}

#if 0
static BYTE Code128RemoveFNC1(const BYTE& code)
{
  BYTE buffer = wxEmptyString;
  size_t len = code.length();
  size_t k;
  for (k = 0; k < len; ++k)
  {
    wxChar c = code[k];
    if (c >= 32 && c <= 126)
    {
      buffer += c;
    }
  }
  return buffer;
}
#endif

static bool Code128IsNextDigits(const BYTE& text, size_t textIndex, int numDigits)
{
  size_t len = text.Len();
  while (textIndex < len && numDigits > 0)
  {
    if (text[textIndex] == CODE128_FNC1)
    {
      ++textIndex;
      continue;
    }
    int n = (numDigits > 2) ? 2 : numDigits;
    if (textIndex + n > len)
      return false;
    while (n-- > 0)
    {
      wxChar c = text[textIndex++];
      if (c < '0' || c > wxT('9'))
        return false;
      --numDigits;
    }
  }
  return (numDigits == 0);
}

static BYTE Code128PackDigits(const BYTE& text, size_t& textIndex, int numDigits)
{
  BYTE code = wxEmptyString;
  while (numDigits > 0)
  {
    if (text[textIndex] == CODE128_FNC1)
    {
      code += CODE128_FNC1_INDEX;
      ++textIndex;
      continue;
    }
    numDigits -= 2;
    int c1 = text[textIndex++] - '0';
    int c2 = text[textIndex++] - '0';
    code += wxChar(c1 * 10 + c2);
  }
  return code;
}

static BYTE Code128MakeCode(const BYTE& text, bool ucc)
{
  BYTE out = wxEmptyString;
  size_t tLen = text.Len();

  // if no text is given return a valid raw pszBarCode
  if (tLen == 0)
  {
    out += CODE128_START_B;
    if (ucc)
    {
      out += CODE128_FNC1_INDEX;
    }
    return out;
  }

  // Check whether pszBarCode text is valid
  BYTE::const_iterator ch;
  for (ch = text.begin(); ch != text.end(); ++ch)
  {
    if (*ch > 127 && *ch != CODE128_FNC1)
    {
      wxLogError(BYTE("wxPdfBarCodeCreator::Code128RawText: ") +
                 BYTE::Format(_("There are illegal characters for pszBarCode 128 in '%s'.", text.c_str()));
      return out;
    }
  }

  wxChar c = text[0];
  wxChar currentCode = CODE128_START_B;
  size_t index = 0;
  if (Code128IsNextDigits(text, index, 2))
  {
    currentCode = CODE128_START_C;
    out += currentCode;
    if (ucc)
    {
      out += CODE128_FNC1_INDEX;
    }
    out += Code128PackDigits(text, index, 2);
  }
  else if (c < 32)
  {
    currentCode = CODE128_START_A;
    out += currentCode;
    if (ucc)
      out += CODE128_FNC1_INDEX;
    out += wxChar(c + 64);
    ++index;
  }
  else
  {
    out += currentCode;
    if (ucc)
    {
      out += CODE128_FNC1_INDEX;
    }
    if (c == CODE128_FNC1)
    {
      out += CODE128_FNC1_INDEX;
    }
    else
    {
      out += wxChar(c - 32);
    }
    ++index;
  }
  while (index < tLen)
  {
    switch (currentCode)
    {
      case CODE128_START_A:
        {
          if (Code128IsNextDigits(text, index, 4))
          {
            currentCode = CODE128_START_C;
            out += CODE128_CODE_TO_C;
            out += Code128PackDigits(text, index, 4);
          }
          else
          {
            c = text[index++];
            switch (c)
            {
              case CODE128_FNC1:
                out += CODE128_FNC1_INDEX;
                break;
              case CODE128_FNC2:
                out += CODE128_FNC2_INDEX;
                break;
              case CODE128_FNC3:
                out += CODE128_FNC3_INDEX;
                break;
              case CODE128_FNC4:
                out += CODE128_CODE_TO_A;
                break;
              default:
                if (c >= 96)
                {
                  if (index < tLen && text[index] >= 96)
                  {
                    currentCode = CODE128_START_B;
                    out += CODE128_CODE_TO_B;
                  }
                  else
                  {
                    out += CODE128_SHIFT;
                  }
                  out += wxChar(c - 32);
                }
                else if (c < 32)
                {
                  out += wxChar(c + 64);
                }
                else
                {
                 out += wxChar(c - 32);
                }
                break;
            }
          }
        }
        break;
      case CODE128_START_B:
        {
          if (Code128IsNextDigits(text, index, 4))
          {
            currentCode = CODE128_START_C;
            out += CODE128_CODE_TO_C;
            out += Code128PackDigits(text, index, 4);
          }
          else
          {
            c = text[index++];
            switch (c)
            {
              case CODE128_FNC1:
                out += CODE128_FNC1_INDEX;
                break;
              case CODE128_FNC2:
                out += CODE128_FNC2_INDEX;
                break;
              case CODE128_FNC3:
                out += CODE128_FNC3_INDEX;
                break;
              case CODE128_FNC4:
                out += CODE128_CODE_TO_B;
                break;
              default:
                if (c < 32)
                {
                  if (index < tLen && text[index] < 32)
                  {
                    currentCode = CODE128_START_A;
                    out += CODE128_CODE_TO_A;
                    out += wxChar(c + 64);
                  }
                  else
                  {
                    out += CODE128_SHIFT;
                    out += wxChar(c + 64);
                  }
                }
                else
                {
                  out += wxChar(c - 32);
                }
                break;
            }
          }
        }
        break;
      case CODE128_START_C:
        {
          if (Code128IsNextDigits(text, index, 2))
          {
            out += Code128PackDigits(text, index, 2);
          }
          else
          {
            c = text[index++];
            if (c == CODE128_FNC1)
            {
              out += CODE128_FNC1_INDEX;
            }
            else if (c < 32)
            {
              currentCode = CODE128_START_A;
              out += CODE128_CODE_TO_A;
              out += wxChar(c + 64);
            }
            else
            {
              currentCode = CODE128_START_B;
              out += CODE128_CODE_TO_B;
              out += wxChar(c - 32);
            }
          }
        }
        break;
    }
  }
  return out;
}

static const struct code128_ailist_t
{
  int ai;
  int len;
}
code128_ailist[] = {
  { 0,    20 },
  { 1,    16 },
  { 2,    16 },
  { 10,   -1 },
  { 11,    9 },
  { 12,    8 },
  { 13,    8 },
  { 15,    8 },
  { 17,    8 },
  { 20,    4 },
  { 21,   -1 },
  { 22,   -1 },
  { 23,   -1 },
  { 30,   -1 },
  { 37,   -1 },
  { 240,  -1 },
  { 241,  -1 },
  { 250,  -1 },
  { 251,  -1 },
  { 252,  -1 },
  { 3900, -1 },
  { 3901, -1 },
  { 3902, -1 },
  { 3903, -1 },
  { 3904, -1 },
  { 3905, -1 },
  { 3906, -1 },
  { 3907, -1 },
  { 3908, -1 },
  { 3909, -1 },
  { 3910, -1 },
  { 3911, -1 },
  { 3912, -1 },
  { 3913, -1 },
  { 3914, -1 },
  { 3915, -1 },
  { 3916, -1 },
  { 3917, -1 },
  { 3918, -1 },
  { 3919, -1 },
  { 3920, -1 },
  { 3921, -1 },
  { 3922, -1 },
  { 3923, -1 },
  { 3924, -1 },
  { 3925, -1 },
  { 3926, -1 },
  { 3927, -1 },
  { 3928, -1 },
  { 3929, -1 },
  { 3930, -1 },
  { 3931, -1 },
  { 3932, -1 },
  { 3933, -1 },
  { 3934, -1 },
  { 3935, -1 },
  { 3936, -1 },
  { 3937, -1 },
  { 3938, -1 },
  { 3939, -1 },
  { 400,  -1 },
  { 401,  -1 },
  { 402,  20 },
  { 403,  -1 },
  { 410,  16 },
  { 411,  16 },
  { 412,  16 },
  { 413,  16 },
  { 414,  16 },
  { 415,  16 },
  { 420,  -1 },
  { 421,  -1 },
  { 422,   6 },
  { 423,  -1 },
  { 424,   6 },
  { 425,   6 },
  { 426,   6 },
  { 7001, 17 },
  { 7002, -1 },
  { 7030, -1 },
  { 7031, -1 },
  { 7032, -1 },
  { 7033, -1 },
  { 7034, -1 },
  { 7035, -1 },
  { 7036, -1 },
  { 7037, -1 },
  { 7038, -1 },
  { 7039, -1 },
  { 8001, 18 },
  { 8002, -1 },
  { 8003, -1 },
  { 8004, -1 },
  { 8005, 10 },
  { 8006, 22 },
  { 8007, -1 },
  { 8008, -1 },
  { 8018, 22 },
  { 8020, -1 },
  { 8100, 10 },
  { 8101, 14 },
  { 8102,  6 },
  { 90,   -1 },
  { 91,   -1 },
  { 92,   -1 },
  { 93,   -1 },
  { 94,   -1 },
  { 95,   -1 },
  { 96,   -1 },
  { 97,   -1 },
  { 98,   -1 },
  { 99,   -1 }
  };

static int Code128GetAILength(int ai)
{
  int len = 0;
  if (ai >= 3100 && ai < 3700)
  {
    len = 10;
  }
  else
  {
    size_t aiCount = WXSIZEOF(code128_ailist);
    if (ai >= code128_ailist[0].ai && ai <= code128_ailist[aiCount-1].ai)
    {
      size_t lo = 0;
      size_t hi = aiCount-1;
      size_t n;
      while (lo < hi)
      {
        n = (lo + hi) / 2;
        if (ai < code128_ailist[n].ai)
        {
          hi = n;
        }
        else if (ai > code128_ailist[n].ai)
        {
          lo = n;
        }
        else
        {
          len = code128_ailist[n].len;
          break;
        }
      }
    }
  }
  return len;
}

// Code128 public methods

bool
wxPdfBarCodeCreator::Code128A(double x, double y, CHAR *pszBarCode, double h, double w)
{
  // Check whether pszBarCode text is valid
  BYTE::const_iterator ch;
  for (ch = pszBarCode.begin(); ch != pszBarCode.end(); ++ch)
  {
    if (!Code128ValidInCodeSetA(*ch))
    {
      wxLogError(BYTE("wxPdfBarCodeCreator::Code128A: ") +
                 BYTE::Format(_("There are illegal characters for Code128A in '%s'.", pszBarCode.c_str()));
      return false;
    }
  }
  BYTE bcode = CODE128_START_A;
  for (ch = pszBarCode.begin(); ch != pszBarCode.end(); ++ch)
  {
    switch (wxChar(*ch))
    {
      case CODE128_FNC1:
        bcode += CODE128_FNC1_INDEX;
        break;
      case CODE128_FNC2:
        bcode += CODE128_FNC2_INDEX;
        break;
      case CODE128_FNC3:
        bcode += CODE128_FNC3_INDEX;
        break;
      case CODE128_FNC4:
        bcode += CODE128_CODE_TO_A;
        break;
      default:
        if (*ch < 32)
          bcode += wxChar((int)(*ch) + 64);
        else
          bcode += wxChar((int)(*ch) - 32);
        break;
    }
  }
  Code128AddCheck(bcode);
  Code128Draw(x, y, bcode, h, w);
  return true;
}

bool
wxPdfBarCodeCreator::Code128B(double x, double y, CHAR *pszBarCode, double h, double w)
{
  // Check whether pszBarCode text is valid
  BYTE::const_iterator ch;
  for (ch = pszBarCode.begin(); ch != pszBarCode.end(); ++ch)
  {
    if (!Code128ValidInCodeSetB(*ch))
    {
      wxLogError(BYTE("wxPdfBarCodeCreator::Code128B: ") +
                 BYTE::Format(_("There are illegal characters for Code128B in '%s'.", pszBarCode.c_str()));
      return false;
    }
  }
  BYTE bcode = CODE128_START_B;
  for (ch = pszBarCode.begin(); ch != pszBarCode.end(); ++ch)
  {
    switch (wxChar(*ch))
    {
      case CODE128_FNC1:
        bcode += CODE128_FNC1_INDEX;
        break;
      case CODE128_FNC2:
        bcode += CODE128_FNC2_INDEX;
        break;
      case CODE128_FNC3:
        bcode += CODE128_FNC3_INDEX;
        break;
      case CODE128_FNC4:
        bcode += CODE128_CODE_TO_B;
        break;
      default:
        bcode += wxChar((int)(*ch) - 32);
        break;
    }
  }
  Code128AddCheck(bcode);
  Code128Draw(x, y, bcode, h, w);
  return true;
}

bool
wxPdfBarCodeCreator::Code128C(double x, double y, CHAR *pszBarCode, double h, double w)
{
  // Check whether pszBarCode text is valid
  if (pszBarCode.Len() % 2 != 0)
  {
      wxLogError(BYTE("wxPdfBarCodeCreator::Code128C: ") +
                 BYTE::Format(_("Invalid odd length for Code128C in '%s'.", pszBarCode.c_str()));
      return false;
  }

  BYTE::const_iterator ch;
  for (ch = pszBarCode.begin(); ch != pszBarCode.end(); ++ch)
  {
    if (!Code128ValidInCodeSetC(*ch))
    {
      wxLogError(BYTE("wxPdfBarCodeCreator::Code128C: ") +
                 BYTE::Format(_("There are illegal characters for Code128C in '%s'.", pszBarCode.c_str()));
      return false;
    }
  }
  BYTE bcode = CODE128_START_C;
  size_t index = 0;
  while (index < pszBarCode.Len())
  {
    bcode += Code128PackDigits(pszBarCode, index, 2);
  }
  Code128AddCheck(bcode);
  Code128Draw(x, y, bcode, h, w);
  return true;
}

bool
wxPdfBarCodeCreator::Code128(double x, double y, CHAR *pszBarCode, double h, double w)
{
  // Check whether pszBarCode text is valid
  BYTE::const_iterator ch;
  for (ch = pszBarCode.begin(); ch != pszBarCode.end(); ++ch)
  {
    if (!Code128ValidChar(*ch))
    {
      wxLogError(BYTE("wxPdfBarCodeCreator::Code128: ") +
                 BYTE::Format(_("There are illegal characters for Code128 in '%s'.", pszBarCode.c_str()));
      return false;
    }
  }

  bool ucc = false;
  BYTE bcode = Code128MakeCode(pszBarCode, ucc);
  size_t len = bcode.Len();
  if (len == 0) return false;

  Code128AddCheck(bcode);
  Code128Draw(x, y, bcode, h, w);
  return true;
}

bool
wxPdfBarCodeCreator::EAN128(double x, double y, CHAR *pszBarCode, double h, double w)
{
  BYTE uccCode = wxEmptyString;
  if (pszBarCode[0] == wxT('('))
  {
    size_t idx = 0;
    while (idx != BYTE::npos)
    {
      size_t end = pszBarCode.find(wxT(')'), idx);
      if (end == BYTE::npos)
      {
        wxLogError(BYTE("wxPdfBarCodeCreator::EAN128: ") +
                   BYTE::Format(_("Badly formed UCC/EAN-128 string '%s'.", pszBarCode.c_str()));
        return false;
      }
      BYTE sai = pszBarCode.SubString(idx+1, end-1);
      if (sai.Len() < 2)
      {
        wxLogError(BYTE("wxPdfBarCodeCreator::EAN128: ") +
                   BYTE::Format(_("AI too short (%s).", sai.c_str()));
        return false;
      }
      int len = 0;
      long ai;
      if (sai.ToLong(&ai))
      {
        len = Code128GetAILength((int) ai);
      }
      if (len == 0)
      {
        wxLogError(BYTE("wxPdfBarCodeCreator::EAN128: ") +
                   BYTE::Format(_("AI not found (%s).", sai.c_str()));
        return false;
      }
      sai = BYTE::Format("%ld", ai);
      if (sai.Len() == 1)
      {
        sai.Prepend("0");
      }
      idx = pszBarCode.find(wxT('('), end);
      size_t next = (idx == BYTE::npos) ? pszBarCode.Len() : idx;
      uccCode += sai + pszBarCode.SubString(end+1, next-1);
      if (len < 0)
      {
        if (idx != BYTE::npos)
        {
          uccCode += CODE128_FNC1;
        }
      }
      else if (next - end - 1 + sai.Len() != (size_t) len)
      {
        wxLogError(BYTE("wxPdfBarCodeCreator::EAN128: ") +
                   BYTE::Format(_("Invalid AI length (%s).", sai.c_str()));
        return false;
      }
    }
  }
  else
  {
    uccCode = pszBarCode;
  }

  // Check whether pszBarCode text is valid
  BYTE::const_iterator ch;
  for (ch = uccCode.begin(); ch != uccCode.end(); ++ch)
  {
    if (!Code128ValidChar(*ch))
    {
      wxLogError(BYTE("wxPdfBarCodeCreator::EAN128: ") +
                 BYTE::Format(_("There are illegal characters for EAN128 in '%s'.", pszBarCode.c_str()));
      return false;
    }
  }

  bool ucc = true;
  BYTE bcode = Code128MakeCode(uccCode, ucc);
  size_t len = bcode.Len();
  if (len == 0) return false;

  Code128AddCheck(bcode);
  Code128Draw(x, y, bcode, h, w);
  return true;
}

void
wxPdfBarCodeCreator::Code128Draw(double x, double y, CHAR *pszBarCode, double h, double w)
{
  //Draw bars
  double barWidth;
  double xPos = x;
  short* bars;
  size_t j;
  BYTE::const_iterator ch;
  for (ch = pszBarCode.begin(); ch != pszBarCode.end(); ++ch)
  {
    bars = code128_bars[*ch];
    for (j = 0; j < 6 && bars[j] != 0; j = j+2)
    {
      barWidth = bars[j] * w;
      m_document->Rect(xPos, y, barWidth, h, wxPDF_STYLE_FILL);
                        xPos += (bars[j]+bars[j+1]) * w;
    }
  }
}

*/
BOOL BarCode_Code39 (char *DataToEncode, char *output, long *iSize)
{
	char *DataToPrint;
	char *PrintableString;
	BYTE *p,*lpD;
//	int I;
	//int CurrentChar;
	int BufferCounter = 0;

	//If DataToEncode is empty, return a non-zero value
	if (DataToEncode==NULL) return 1;
	if (!*DataToEncode) return 1;

	lpD=DataToPrint=malloc(512);
	PrintableString=malloc(512);

	for (p=DataToEncode;*p;p++,lpD++)
	{
		if (*p==' ') *lpD='='; else *lpD=*p;
	}
	*lpD=0;
	strupr(DataToPrint);
	sprintf(PrintableString,"*%s*",DataToPrint);
	
	// Allow the string to return to the proper size.
	*iSize = (long) strlen(PrintableString); 
	strcpy(output, PrintableString); 

	free(DataToPrint);
	free(PrintableString);
	return 0;
}

//
// BarCode_EAN8()
// 
BOOL BarCode_EAN8(CHAR *lpDataToEncode, CHAR *lpOutput, LONG *lpiSize)
{
	char *DataToEncode2;
	char *OnlyCorrectData;
	int  i;
	int  iFactor;
	int  weightedTotal;
	int  CheckDigit;
	int PrintBuffer = 0;
	BYTE *p,*lpD;

	if (!lpDataToEncode) return 1;
	if (!*lpDataToEncode) return 1;
	
	DataToEncode2 = ehAlloc(512);
	OnlyCorrectData = ehAlloc(512);

	strcpy(DataToEncode2,lpDataToEncode);

	lpD=OnlyCorrectData;
	for (p=DataToEncode2;*p;p++)
	{
		if (isdigit(*p)) {*lpD=*p; lpD++;}
	}
	*lpD=0;

	if(strlen(OnlyCorrectData) >= 7) 
		OnlyCorrectData[7]=0;
		//sprintf(DataToEncode2,"%s",mid(OnlyCorrectData, 1, 7));
		else
		strcpy(OnlyCorrectData,"0005000");
	strcpy(DataToEncode2,OnlyCorrectData);
	
	iFactor=3;
	weightedTotal=0;
	for(i=strlen(DataToEncode2)-1;i>=0; i--)
	{
		weightedTotal=weightedTotal+DataToEncode2[i]*iFactor;
		iFactor=4-iFactor;
	}

	i = (weightedTotal % 10);
	if (i) CheckDigit=(10-i); else CheckDigit=0;
	DataToEncode2[7]=CheckDigit+48;
	DataToEncode2[8]=0;

	lpD=lpOutput;
	for (p=DataToEncode2,i=0;*p;p++,i++)
	{
		if (!i) {*lpD++=(*p-48+96);}
		if (i>0&&i<4) *lpD++=*p;
		if (i>=4) *lpD++=*p+16;
	}
	*lpD++='(';  
	*lpD++=0;
//	win_infoarg("%s",lpOutput);

	// Allow the string to return to the proper size.
	*lpiSize = (long) strlen(lpOutput); 
	ehFree(DataToEncode2);
	ehFree(OnlyCorrectData);
	
	return 0;
}//end ean8()


BOOL BarCode_EAN13(CHAR *lpDataToEncode, CHAR * lpOutput, LONG *lpiSize)
{
	char *DataToEncode2;
	char *OnlyCorrectData;
	int  i;
	int  iFactor;
	int  weightedTotal;
	int  CheckDigit;
	int PrintBuffer = 0;
	BYTE *p,*lpD;
	BYTE first;
	BOOL tableA;

	*lpOutput=0;
	if (!lpDataToEncode) return 1;
	if (!*lpDataToEncode) return 1;
	
	DataToEncode2 = ehAlloc(512);
	OnlyCorrectData = ehAlloc(512);

	strcpy(DataToEncode2,lpDataToEncode);

	lpD=OnlyCorrectData;
	for (p=DataToEncode2;*p;p++)
	{
		if (isdigit(*p))  {*lpD=*p; lpD++;}
	}
	*lpD=0;

	//win_infoarg("[%s] %d",OnlyCorrectData,strlen(OnlyCorrectData));
	if(strlen(OnlyCorrectData)>= 12) 
		OnlyCorrectData[12]=0;
		//sprintf(DataToEncode2,"%s",mid(OnlyCorrectData, 1, 7));
		else
		strcpy(OnlyCorrectData,"000000000000");
	strcpy(DataToEncode2,OnlyCorrectData);
	
	
	// Calcola ed inserisce il CheckDigit
	iFactor=3;
	weightedTotal=0;
	for(i=strlen(DataToEncode2)-1;i>=0; i--)
	{
		weightedTotal=weightedTotal+(DataToEncode2[i]-48)*iFactor;
		iFactor=4-iFactor;
	}

	i = (weightedTotal % 10);
	if (i) CheckDigit=(10-i); else CheckDigit=0;
	DataToEncode2[12]=CheckDigit+48;
	DataToEncode2[13]=0;

	lpD=lpOutput;
	p=DataToEncode2;

	// Flags - primo e secondo carattere
	*lpD++=*p++;
	*lpD++=(*p-48+65); p++;

	first=DataToEncode2[0]-48;
	for (i=2; i<=6; i++)
	{
		tableA=FALSE;
		switch (i)
		{
			case 2:
				if (first >= 0 && first <= 3) tableA = TRUE;
				break;
			case 3:
				if (first==0 || first==4 || first==7 || first==8) tableA = TRUE; 
				break;
			case 4:
				if (first==0 || first==1 || first==4 || first==5 || first==9) tableA = TRUE; 
				break;
			case 5:
				if (first==0 || first==2 || first==5 || first==6 || first==7) tableA = TRUE; 
				break;
			case 6:
				if (first==0 || first==3 || first==6 || first==8 || first==9) tableA = TRUE; 
				break;
		}

		if (tableA)
					*lpD++= (BYTE) (DataToEncode2[i]-48+65);
					else
					*lpD++= (BYTE) (DataToEncode2[i]-48+75);
	}
	*lpD++='*'; //Ajout séparateur central / Add middle separator

	for (i=7; i<=12; i++)
	{
		//CodeBarre += (char)(97 + Convert.ToInt32(chaine.Substring(i, 1))); 
		*lpD++= (BYTE) (DataToEncode2[i]-48+97);
	}
	*lpD++='+'; //Ajout de la marque de fin / Add end mark
	*lpD++=0;
//	win_infoarg("%s",lpOutput);

	// Allow the string to return to the proper size.
	*lpiSize = (long) strlen(lpOutput); 
	ehFree(DataToEncode2);
	ehFree(OnlyCorrectData);
	
	return 0;
}
