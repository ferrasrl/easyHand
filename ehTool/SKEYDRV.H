/* --------------------------------------------------------------------	*/
/*				    EUTRON spa 			 					*/
/* --------------------------------------------------------------------	*/
/*	Language    : C											*/
/*	Application : Smartkey Driver for MS-DOS, WINDOWS and OS/2			*/
/*	Module      : skeydrv.h										*/
/*	Version     : 5.0											*/
/*	Date	    : 2.9.1993											*/
/*	Note	    :												*/
/*															*/
/* --------------------------------------------------------------------	*/

/* --------------------------------------------------------------------	*/
/* Smartkey mode codes											*/
/* --------------------------------------------------------------------	*/

#define	SCRAMBLING_MODE		's'
#define	READING_MODE		'r'
#define	WRITING_MODE		'w'
#define	FIXING_MODE		'f'
#define	LOCATING_MODE		'l'
#define	COMPARING_MODE		'c'
#define	PROGRAMMING_MODE	'p'
#define	MODEL_READING_MODE	'm'
#define	ENCRYPTING_MODE		'e'
#define	BLOCK_READING_MODE	"br"
#define	BLOCK_WRITING_MODE	"bw"

/* --------------------------------------------------------------------	*/
/* Smartkey models							*/
/* --------------------------------------------------------------------	*/

#define	SKEY_NONE	'0'		/* No Smartkey			*/
#define SKEY_FX		'1'		/* Smartkey mod. FX		*/
#define SKEY_PR		'2'		/* Smartkey mod. PR		*/
#define SKEY_EP		'3'		/* Smartkey mod. EP		*/
#define	SKEY_NET_5	'4'		/* Smartkey mod. NET 5 users	*/
#define	SKEY_NET_10	'5'		/* Smartkey mod. NET 10 users	*/
#define	SKEY_NET_25	'6'		/* Smartkey mod. NET 25 users	*/
#define	SKEY_NET_50	'7'		/* Smartkey mod. NET 50 users	*/
#define	SKEY_NET_NL	'8'		/* Smartkey mod. NET no limit	*/
#define	SKEY_SP		'9'		/* Smartkey mod. SP		*/
#define	SKEY_NET	'A'		/* Smartkey mod. NET		*/

/* --------------------------------------------------------------------	*/
/* Return codes								*/
/* --------------------------------------------------------------------	*/

#define ST_OK			0	/* No errors			*/
#define ST_NONE_KEY		-1	/* No Smartkey present		*/
#define ST_SYNT_ERR		-2	/* Syntax error			*/
#define ST_LABEL_FAILED 	-3	/* Uncorrect label		*/
#define ST_PW_DATA_FAILED	-4	/* Uncorrect password or data	*/
#define	ST_HW_FAILURE		-20	/* Smartkey damaged		*/

/* --------------------------------------------------------------------	*/
/* Field length								*/
/* --------------------------------------------------------------------	*/

#define LABEL_LENGTH		16
#define PASSWORD_LENGTH		16
#define DATA_LENGTH		64
#define EXTENDED_DATA_LENGTH	352
#define	SCRAMBLE_LENGTH		8

/* --------------------------------------------------------------------	*/
/* Communication structure definition					*/
/* --------------------------------------------------------------------	*/

typedef struct smartkey {
	short		lpt;
	short		command;
	CHAR	label[LABEL_LENGTH];
	CHAR	password[PASSWORD_LENGTH];
	CHAR	data[DATA_LENGTH];
	short		fail_counter;
	short		status;
	CHAR	ext_data[EXTENDED_DATA_LENGTH];
} SKEY_DATA;

/* --------------------------------------------------------------------	*/
/* Interface function prototype						*/
/* --------------------------------------------------------------------	*/

short	__far __cdecl msclink(SKEY_DATA __far *);
