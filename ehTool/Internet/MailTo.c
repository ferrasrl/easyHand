//   ---------------------------------------------
//   ³ MailTo                      
//   ³                                           
//   ³             by Ferr… Art & Tecnology 2000
//   ---------------------------------------------

#include "\ehtool\include\ehsw_idb.h"
#include "MailTo.h"
#include <mapi.h>

static MapiRecipDesc recipient = 
{
  0, // Reserved
  MAPI_TO, // Indica il tipo di recipiente
  "?",// Destinatario
  "SMPT:info@ferra.com", // FAX:Numero /SMPT:login@domain.com
  0,// lpEntryID (Bo ???)
  NULL
};

static MapiMessage message =
{
  0, // reserved
  "subject", // subject
  "Text", // Note Text
  NULL, // Message Type ?
  NULL, // DateReceived
  NULL, // Conversasion ID (?)
  MAPI_RECEIPT_REQUESTED, // Flag del tipo di messaggio (Trasmesso,Letto,
  NULL, // Originator
  1, // Numero di messaggi nel recipiente
  &recipient, // Puntatore al recipiente
  0, // Numero di strutture che definiscono i file attaccati
  NULL // Puntatore alle strutture
};


// Per ora solo su NT
static	LPMAPILOGON    lpfnMAPILogon;
static	LPMAPISENDMAIL lpfnMAPISendMail;
static	LPMAPILOGOFF   lpfnMAPILogoff;


void MailTo(CHAR *lpEmail,CHAR *lpObject,CHAR *lpMess,CHAR *lpNome)
{
  MailServer(WS_OPEN,NULL,NULL,NULL,NULL);
  MailServer(WS_DO,lpEmail,lpObject,lpMess,lpNome);
  MailServer(WS_CLOSE,NULL,NULL,NULL,NULL);
}

void MailServer(SINT cmd,CHAR *email,CHAR *lpObject,CHAR *lpMess,CHAR *lpNome)
{
	static LHANDLE lhSession;
	static HANDLE hMAPILib;
	ULONG ulDato;
	
	switch(cmd)
	{
		// Apro il servizio
		case WS_OPEN:

			hMAPILib=LoadLibrary("msoemapi.DLL");
			if  (hMAPILib==NULL) {printf("Errore DLL"); getch(); exit(1);}
			lpfnMAPILogon    =(LPMAPILOGON)    GetProcAddress(hMAPILib,"MAPILogon");
			lpfnMAPISendMail =(LPMAPISENDMAIL) GetProcAddress(hMAPILib,"MAPISendMail");
			lpfnMAPILogoff   =(LPMAPILOGOFF)   GetProcAddress(hMAPILib,"MAPILogoff");

			if (lpfnMAPILogon==NULL) {PRG_end("MAPILogon ?"); }
			// Apro la sessione con il gestore di posta
			ulDato=	
			(*lpfnMAPILogon)(0,
//					 "Microsoft Outlook", // Profilo
					 "Outlook express", // Profilo
					 NULL, // Password
		             MAPI_NEW_SESSION ,
					 //MAPI_LOGON_UI||//0//
					 //MAPI_ALLOW_OTHERS,
					 //MAPI_PASSWORD_UI,
					 0, // must be 0
					 &lhSession);
	
			if (ulDato!=SUCCESS_SUCCESS) 
			{
				CHAR *pe;
				switch (ulDato)
				{
				 case MAPI_E_AMBIGUOUS_RECIPIENT : pe="MAPI_E_AMBIGUOUS_RECIPIENT"; break;
				 case MAPI_E_ATTACHMENT_NOT_FOUND : pe="MAPI_E_ATTACHMENT_NOT_FOUND "; break;
				 case MAPI_E_ATTACHMENT_OPEN_FAILURE : pe="MAPI_E_ATTACHMENT_OPEN_FAILURE"; break;
				 case MAPI_E_BAD_RECIPTYPE : pe="MAPI_E_BAD_RECIPTYPE "; break;
				 case MAPI_E_FAILURE : pe="MAPI_E_FAILURE "; break;
				 case MAPI_E_INSUFFICIENT_MEMORY : pe="MAPI_E_INSUFFICIENT_MEMORY "; break;
				 case MAPI_E_INVALID_RECIPS : pe="MAPI_E_INVALID_RECIPS "; break;
				 case MAPI_E_LOGIN_FAILURE : pe="MAPI_E_LOGIN_FAILURE "; break;
				 case MAPI_E_TEXT_TOO_LARGE : pe="MAPI_E_TEXT_TOO_LARGE "; break;
				 case MAPI_E_TOO_MANY_FILES : pe="MAPI_E_TOO_MANY_FILES "; break;
				 case MAPI_E_TOO_MANY_RECIPIENTS: pe="MAPI_E_TOO_MANY_RECIPIENTS"; break;
				 case MAPI_E_UNKNOWN_RECIPIENT : pe="MAPI_E_UNKNOWN_RECIPIENT "; break;
				 case MAPI_E_USER_ABORT : pe="MAPI_E_USER_ABORT "; break;
				}
				 PRG_end("Error LogON (%s)\n",pe);
			}
			break;

		case WS_DO:

			recipient.lpszAddress=email;
			recipient.lpszName=lpNome;
			message.lpszSubject=lpObject;
			message.lpszNoteText=lpMess;

			// Trasmissione dell'email
			// Mando l'email
			ulDato=(*lpfnMAPISendMail)(lhSession,
								0,// Handle parent
								&message, // MEssaggio
								0,0);

			if (ulDato!=SUCCESS_SUCCESS) 
			{
				CHAR *pe;
				switch (ulDato)
				{
					case MAPI_E_AMBIGUOUS_RECIPIENT : pe="MAPI_E_AMBIGUOUS_RECIPIENT"; break;
					case MAPI_E_ATTACHMENT_NOT_FOUND : pe="MAPI_E_ATTACHMENT_NOT_FOUND "; break;
					case MAPI_E_ATTACHMENT_OPEN_FAILURE : pe="MAPI_E_ATTACHMENT_OPEN_FAILURE"; break;
					case MAPI_E_BAD_RECIPTYPE : pe="MAPI_E_BAD_RECIPTYPE "; break;
					case MAPI_E_FAILURE : pe="MAPI_E_FAILURE "; break;
					case MAPI_E_INSUFFICIENT_MEMORY : pe="MAPI_E_INSUFFICIENT_MEMORY "; break;
					case MAPI_E_INVALID_RECIPS : pe="MAPI_E_INVALID_RECIPS "; break;
					case MAPI_E_LOGIN_FAILURE : pe="MAPI_E_LOGIN_FAILURE "; break;
					case MAPI_E_TEXT_TOO_LARGE : pe="MAPI_E_TEXT_TOO_LARGE "; break;
					case MAPI_E_TOO_MANY_FILES : pe="MAPI_E_TOO_MANY_FILES "; break;
					case MAPI_E_TOO_MANY_RECIPIENTS: pe="MAPI_E_TOO_MANY_RECIPIENTS"; break;
					case MAPI_E_UNKNOWN_RECIPIENT : pe="MAPI_E_UNKNOWN_RECIPIENT "; break;
					case MAPI_E_USER_ABORT : pe="MAPI_E_USER_ABORT "; break;
				}
				PRG_end("Error Sending (%s)\n",pe);
			}
			break;
	
		case WS_CLOSE:

			// Chiusura della trasmissione
			(*lpfnMAPILogoff)(lhSession,0,0,0);
			FreeLibrary(hMAPILib);
			lpfnMAPILogon    =NULL;
			lpfnMAPISendMail =NULL;
			lpfnMAPILogoff   =NULL;
			break;
	}
}



// ------------------------------------------
// | CREA6 Builder  by Ferrà A&T 22/10/1999 |
// ------------------------------------------¸
static void FromCode(CHAR *lpDa,CHAR *lpA)
{
// Creato dal progetto : Nuovo.vpg 

  struct OBJ obj[]={
  // ObjType   Name    Sts Enab  x   y Cl1 Cl2 Text
	{O_KEYDIM,"CR"    ,OFF, ON,127, 85, 81, 25,"#Ok"},
	{STOP}
	};

  struct IPT ipt[]={
	{ 1, 1,ALFA,RIGA, 94, 35,112, 20,  0, 15,  1,  1},
	{ 1, 1,ALFA,RIGA, 94, 61,112, 20,  0, 15,  1,  1},
	{ 0, 0,STOP}
	};

//  Header di attivazione
	JOBBER; // Riconoscimento programma
	win_open(EHWP_MOUSECENTER,69,221,123,-1,3,ON,"Range di spedizione");
	dispf(13,32,0,-1,ON,"SMALL F",3,"Dal codice");
	dispf(13,58,0,-1,ON,"SMALL F",3,"Al codice");

//  Carico OBJ & variazioni sui Font
	obj_open(obj);
	obj_vedi();
//  Carico IPT & font
	ipt_font("VGASYS",0);
	ipt_open(ipt);
	ipt_reset(); ipt_vedi();

// Loop di controllo EH
	for (;;)
	{
	  ipt_ask();
	  if (key_press(ESC)||obj_press("ESCOFF")) break;
	  if (key_press(CR)||obj_press("CROFF"))
	  {
		if (!*ipt_read(1)) ipt_writevedi(1,"zzzzzzzzzzzzzz",0);
		strcpy(lpDa,ipt_read(0));
		strcpy(lpA,ipt_read(1));
		break;
	  }
	};
	win_close();
}
