//   +-------------------------------------------+
//   | SmartKeyCheck                             |
//   | Test della smartkey                       |
//   |                                           |
//   |                                           |
//   |             by Ferr… Art & Tecnology 1998 |
//   +-------------------------------------------+
/*
#include <dos.h>
#include <bios.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mem.h>
#include <dir.h>
#include <alloc.h>

#include <flm_main.h>
#include <flm_adb.h>
#include <flm_vid.h>
  */
#include "\ehtool\include\ehsw_i.h"

#include "c:\ehtool\skeydrv.h"
#include "c:\ehtool\skutil.h"

/*-------------------------------------------------------------------------*/
/*			SMARTKEY initialization structure		   */
/*-------------------------------------------------------------------------*/

SKEY_DATA user_key;
SKEY_DATA far *ptr_key = &user_key;		/* WARNING ! Must be FAR   */

/*-------------------------------------------------------------------------*/
/*				      main				   */
/*-------------------------------------------------------------------------*/


SINT SKLocate(CHAR *Label,SINT *lpt)
{

user_key.command = (SINT) 'L';
memset (user_key.label,'\0', LABEL_LENGTH);
strcpy (user_key.label, Label);

msclink (ptr_key); /* Driver call pointing       */

*lpt = user_key.lpt;
return user_key.status;
}

CHAR *SKModel(CHAR *Label)
{
 user_key.command = (SINT) 'M';
 memset (user_key.label,'\0', LABEL_LENGTH);
 strcpy (user_key.label, Label);

 msclink (ptr_key); /* Driver call pointing       */

 if (user_key.status) return NULL;


 switch (*user_key.data)
	{
	 case '0': return "NOKEY";
	 case '1': return "FX";
	 case '2': return "PR";
	 case '3': return "EP";
	 case '9': return "SP";
	 case 'A': return "NET";
	}

 return "?";
}

//                                                         |
// Fare prima un locate                                    |
//                                                         |
// Ritorna                                                 |
// 0= Ok                                                   |
// -1 = Non Š presente la Chiave/Label                     |
// -2 = Sintax/Chiave FX/Chiave gi… fissata ecc...         |
// -20= Operazione di scrittura non effettuata             |

CHAR *SKBlockRead(CHAR *Label,CHAR *Password,SINT Start,SINT Size)
{
 SINT *lwPt;
 user_key.command = (SINT) ('B'<<8)+'R';
 memset (user_key.label,'\0', LABEL_LENGTH);
 strcpy (user_key.label, Label);

 memset (user_key.password,'\0', PASSWORD_LENGTH);
 strcpy (user_key.password, Password);

 memset (user_key.data,'\0',DATA_LENGTH);
 lwPt=(SINT *) &user_key.data;

 lwPt[0]=Start;  // da 0
 lwPt[1]=Size; // 15 Word

 msclink (ptr_key); /* Driver call pointing       */

 if (user_key.status) return NULL;
 return user_key.data;
}

// Ritorna
// 0= Ok
// -1 = Non Š presente la Chiave/Label
// -2 = Sintax/Chiave FX/Chiave gi… fissata ec...
// -4 = Label OK Password errata
// -20= Operazione di scrittura non effettuata

SINT SKProgram(CHAR *OldLabel,CHAR *NewLabel,CHAR *NewPassword)
{
 user_key.command = (SINT) 'P';

 // Current label
 memset (user_key.data,'\0',DATA_LENGTH);
 strcpy (user_key.data,OldLabel);

 memset (user_key.label,'\0', LABEL_LENGTH);
 strcpy (user_key.label, NewLabel);

 memset (user_key.password,'\0', PASSWORD_LENGTH);
 strcpy (user_key.password, NewPassword);

 msclink (ptr_key); /* Driver call pointing       */

 return user_key.status;
}

// Ritorna
// 0= Ok
// -1 = Non Š presente la Chiave/Label
// -2 = Sintax/Chiave FX/Chiave gi… fissata ec...
// -4 = Label OK Password errata
// -20= Operazione di scrittura non effettuata


SINT SKWrite(CHAR *Label,CHAR *Password,CHAR *Data)
{
 user_key.command = (SINT) 'W';
 memset (user_key.label,'\0',LABEL_LENGTH);
 strcpy (user_key.label, Label);

 memset (user_key.password,'\0',PASSWORD_LENGTH);
 strcpy (user_key.password, Password);

 memset (user_key.data,'\0',DATA_LENGTH);
 strcpy (user_key.data,Data);

 msclink (ptr_key); /* Driver call pointing       */

 return user_key.status;
}


SINT SKCompare(CHAR *Label,CHAR *Password,CHAR *Data)
{
 user_key.command = (SINT) 'C';
 memset (user_key.label,'\0',LABEL_LENGTH);
 strcpy (user_key.label, Label);

 memset (user_key.password,'\0',PASSWORD_LENGTH);
 strcpy (user_key.password, Password);

 memset (user_key.data,'\0',DATA_LENGTH);
 strcpy (user_key.data,Data);

 msclink (ptr_key); /* Driver call pointing       */

 if (user_key.status>0) return 0;
 return user_key.status;
}