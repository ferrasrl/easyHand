#ifndef __SKEYLINK_H
#define __SKEYLINK_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __SKEYDRV_H
#define __SKEYDRV_H

/*
	Smartkey commands
*/
#define MAKE_KEY_CODE(low,high) (((unsigned short)(low)) | (((unsigned short)(high)) << 8))

#define SCRAMBLING_MODE		's'
#define READING_MODE		'r'
#define WRITING_MODE		'w'
#define FIXING_MODE		'f'
#define LOCATING_MODE		'l'
#define COMPARING_MODE		'c'
#define PROGRAMMING_MODE	'p'
#define MODEL_READING_MODE	'm'
#define ENCRYPTING_MODE		'e'
#define BLOCK_READING_MODE	MAKE_KEY_CODE('b','r')
#define BLOCK_WRITING_MODE	MAKE_KEY_CODE('b','w')

/*
	Smartkey NEW commands
*/
#define SERIAL_NUMBER_READING_MODE 'n'
#define FIX_READING_MODE 'x'
#define EXT_MODEL_READING_MODE 'h'
#define FAIL_COUNTER_READING_MODE 'a'

/*
	Legacy commands
*/
#define DEACTIVATE_ACCESS_MODE	MAKE_KEY_CODE('d','d')
#define ACTIVATE_ACCESS_MODE	MAKE_KEY_CODE('d','a')
#define ANTIHACKER_MODE		't'

/*
	Smartkey models
*/
#define SKEY_NONE	'0'		/* No Smartkey */
#define SKEY_FX		'1'		/* Smartkey mod. FX */
#define SKEY_PR		'2'		/* Smartkey mod. PR */
#define SKEY_EP		'3'		/* Smartkey mod. EP */
#define SKEY_NET_5	'4'		/* Smartkey mod. NET 5 users */
#define SKEY_NET_10	'5'		/* Smartkey mod. NET 10 users */
#define SKEY_NET_25	'6'		/* Smartkey mod. NET 25 users */
#define SKEY_NET_50	'7'		/* Smartkey mod. NET 50 users */
#define SKEY_NET_NL	'8'		/* Smartkey mod. NET no limit */
#define SKEY_SP		'9'		/* Smartkey mod. SP */
#define SKEY_NET	'A'		/* Smartkey mod. NET */
#define SKEY_SB		'B'		/* Smartkey mod. SB */
#define SKEY_WI		'C'		/* Smartkey mod. WI */

/*
	Return codes
*/
#define ST_OK			0	/* No errors */
#define ST_NONE_KEY		-1	/* No Smartkey present */
#define ST_SYNT_ERR		-2	/* Syntax error	*/
#define ST_LABEL_FAILED 	-3	/* Uncorrect label */
#define ST_PW_DATA_FAILED	-4	/* Uncorrect password or data */
#define ST_EXEC_ERROR		-16	/* Max exec reached */
#define ST_HW_FAILURE		-20	/* Smartkey damaged */

/*
	Field length
*/
#define LABEL_LENGTH		16
#define PASSWORD_LENGTH		16
#define DATA_LENGTH		64
#define EXTENDED_DATA_LENGTH	352
#define SCRAMBLE_LENGTH		8

/*
	Communication structure definition
*/
#if defined(WIN32)
#pragma pack(1)
#endif

typedef struct __SKEY_DATA {
	short lpt;
	short command;
	unsigned char label[LABEL_LENGTH];
	unsigned char password[PASSWORD_LENGTH];
	unsigned char data[DATA_LENGTH];
	short fail_counter;
	short status;
	unsigned char ext_data[EXTENDED_DATA_LENGTH];
} SKEY_DATA;

#if defined(WIN32)
#pragma pack()
#endif

#endif /* __SKEYDRV_H */

/*
	Smartkey net command codes
*/
#define NET_KEY_OPEN		'O'
#define NET_KEY_ACCESS		'A'
#define NET_KEY_CLOSE		'C'

/*
	Smartkey command codes
*/
#define USER_NUMBER_MODE	'U'

/*
	Net return codes
*/
#define ST_NET_ERROR		-5  /* Lan error */
#define ST_CLOSE_ERROR		-6  /* Attempting to CLOSE  without OPENing */
#define ST_ACCESS_ERROR		-7  /* Attempting to ACCESS without OPENing */
#define ST_USER_ERROR		-8  /* Max user reached */
#define ST_NET_PWD_ERR		-9  /* Net password wrong */
#define ST_TSR_NOT_INST		-10 /* SKEYSRV not found */
#define ST_INIT_ERROR		-11 /* Insufficient PC memory */
#define ST_PATH_ERR		-12 /* Path error of file not found */
#define ST_DRIVER_NOT_INST	-13 /* SKEYTSR not installed */
#define ST_TOO_MANY_OPEN_KEY	-14 /* Too many open SmartKey */
#define ST_NET_PASS_INVALID	-15 /* Invalid net password */

#define ST_NET_CONF_ERROR -21 /* Configuration error in INI/Registry/Environment */
#define ST_NET_ANP_INIT_ERROR -22 /* Error inizializing the ANP protocol */
#define ST_NET_TCPIP_INIT_ERROR -23 /* Error inizializing the TCPIP protocol */
#define ST_NET_NOVELL_INIT_ERROR -24 /* Error inizializing the Novell protocol */
#define ST_NET_LOCAL_INIT_ERROR -25 /* Error inizializing the Local protocol */
#define ST_NET_KEY_NOT_MAP -26 /* Not MAP programmed key found when MAP is requested */

/* 
	Code returned in data[0] after NET_OPEN
*/
#define NET_TYPE_LOCAL 0
#define NET_TYPE_IPX 1
#define NET_TYPE_ANP 2
#define NET_TYPE_TCPIP 3

/*
	Structure definition
*/
#if defined(WIN32)
#pragma pack(1)
#endif

typedef struct __KEY_NET {
	short		net_command;
	unsigned long	net_password;
	short		lpt;
	short		command;
	unsigned char	label[LABEL_LENGTH];
	unsigned char	password[PASSWORD_LENGTH];
	unsigned char	data[DATA_LENGTH];
	short		fail_counter;
	short		status;
	unsigned char	ext_data[EXTENDED_DATA_LENGTH];
} KEY_NET;

#if defined(WIN32)
#pragma pack()
#endif

/*
	int smartlink_inizialize(void)
		Inizialize the smartlink driver
		Return 0 if successfull

	void smartlink_deinizialize(void)
		Deinizialize the smartlink driver
		Return 0 if successfull

	short smartlink(KEY_NET* key)
		Call the driver.
		Return key->k.status code
*/
__declspec(dllimport) short __stdcall smartlink(KEY_NET*);

#ifdef __cplusplus
}
#endif

#endif
