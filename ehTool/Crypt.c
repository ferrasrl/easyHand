//   +-------------------------------------------+
//   | Crypt
//   | Gestione sistemi di cryptazione
//   |             
//   |							   Ferrà srl 2007
//   +-------------------------------------------+
/*
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
*/

#include "/easyhand/inc/easyhand.h"
#include <wincrypt.h>
#include "/easyhand/ehtool/crypt.h"

// http://msdn2.microsoft.com/en-us/library/aa382358.aspx

#ifndef ENCRYPT_ALGORITHM 
 #define ENCRYPT_ALGORITHM CALG_RC4
 #define ENCRYPT_BLOCK_SIZE 1024
 #define KEYLENGTH  0x00800000
// #define KEYLENGTH  0x0010
#endif
//
// iMode > 0=Crypt / 1=Decrypt
//
BOOL FCryptStringService(SINT iMode, BYTE *lpSource, BYTE *lpDestination, DWORD dwSizeDest, BYTE *szPassword)
{
	HCRYPTPROV hCryptProv=0; 
	HCRYPTHASH hHash; 
	HCRYPTKEY hKey; 
	DWORD dwCount;
	// BYTE *lpBuffer;

	//-------------------------------------------------------------------
	// Richiedo l'handle del default provider (Crypt Service Provider) 
	//-------------------------------------------------------------------
	if (!CryptAcquireContext(	&hCryptProv, 
								NULL, 
								MS_ENHANCED_PROV, 
								PROV_RSA_FULL, 
								CRYPT_VERIFYCONTEXT)) ehExit("error CryptAcquireContext! (%d)",GetLastError()); 

	//-------------------------------------------------------------------
	// Creo un hash object derivato dalla password
	//-------------------------------------------------------------------
	if (!CryptCreateHash(	hCryptProv, 
						   CALG_MD5, 
						   0, 0, &hHash)) ehExit("Error during CryptCreateHash!\n");

	//-------------------------------------------------------------------
	// Hash the password. 
	//
	if (!CryptHashData(hHash, (BYTE *)szPassword, strlen(szPassword), 0)) ehExit("Error during CryptHashData. \n"); 

	//-------------------------------------------------------------------
	// Derive a session key from the hash object. 
	//
	if (!CryptDeriveKey(hCryptProv, ENCRYPT_ALGORITHM, hHash, KEYLENGTH, &hKey)) // <-------------------------
		ehExit("Error during CryptDeriveKey!\n"); 

	//-------------------------------------------------------------------
	// Destroy hash object. 
	if (hHash) 
	{
		if (!(CryptDestroyHash(hHash))) ehExit("Error during CryptDestroyHash"); 
		hHash = 0;
	}

	//-------------------------------------------------------------------
	// Cripto
	// 
	//memset(lpDestination,0,dwSizeDest);
	strcpy(lpDestination,lpSource);	

	switch (iMode)
	{
		case 0: dwCount=strlen(lpDestination);
				if (!CryptEncrypt(hKey, 
								  0, 
								  TRUE, // Ultima sezione di decodifica
								  0, 
								  lpDestination, 
								  &dwCount, 
								  dwSizeDest))
								{					
									ehExit("Error during CryptEncrypt. %d \n",GetLastError()); 
								} 
				 break;

		case 1: dwCount=strlen(lpDestination);
				if (!CryptDecrypt(hKey, 
								  0, 
								  TRUE, // Ultima sezione di decodifica
								  0, 
								  lpDestination, 
								  &dwCount))
								{					
									ehExit("Error during CryptDecrypt. %d \n",GetLastError()); 
								} 
				 break;

	}


	//-------------------------------------------------------------------
	// Rilascio il provider
	//-------------------------------------------------------------------
	if (hCryptProv)
		{
				if(!(CryptReleaseContext(hCryptProv, 0))) ehExit("Error during CryptReleaseContext");
		}

	return (TRUE);
}



// Non provato !!!!

BOOL FEncryptFile(
        PCHAR szSource, 
        PCHAR szDestination, 
        PCHAR szPassword)
//-------------------------------------------------------------------
// Parameters passed are:
//  szSource, the name of the input, a plaintext file.
//  szDestination, the name of the output, an encrypted file to be 
//   created.
//  szPassword, either NULL if a password is not to be used or the 
//   string that is the password.
{ 
	//-------------------------------------------------------------------
	// Declare and initialize local variables.

	FILE *hSource; 
	FILE *hDestination; 

	HCRYPTPROV hCryptProv; 
	HCRYPTKEY hKey; 
	//HCRYPTKEY hXchgKey; 
	HCRYPTHASH hHash; 

	//PBYTE pbKeyBlob; 
	//DWORD dwKeyBlobLen; 

	PBYTE pbBuffer; 
	DWORD dwBlockLen; 
	DWORD dwBufferLen; 
	DWORD dwCount; 

	//-------------------------------------------------------------------
	// Open source file. 

	if(hSource = fopen(szSource,"rb"))
	{
		win_infoarg("The source plaintext file, %s, is open. \n", szSource);
	}
	else
	{ 
		ehExit("Error opening source plaintext file!");
	} 
	//-------------------------------------------------------------------
	// Open destination file. 

	if(hDestination = fopen(szDestination,"wb"))
	{
		win_infoarg("Destination file %s is open. \n", szDestination);
	}
	else
	{
		ehExit("Error opening destination ciphertext file!"); 
	}

	//-------------------------------------------------------------------
	// Richiedo l'handle del default provider (Crypt Service Provider) 
	//-------------------------------------------------------------------
	if(CryptAcquireContext(
		&hCryptProv, 
		NULL, 
		MS_ENHANCED_PROV, 
		PROV_RSA_FULL, 
		0))
	{
		win_infoarg("A cryptographic provider has been acquired. \n");
	}
	else
	{
		ehExit("Error during CryptAcquireContext!"); 
	}

	//-------------------------------------------------------------------
	// Create the session key.
	//-------------------------------------------------------------------
	/*
	if(!szPassword) 
	{ 
	//--------------------------------------------------------------
	// No password was passed.
	// Encrypt the file with a random session key, and write the key
	// to a file. 

	//--------------------------------------------------------------
	// Create a random session key. 

	if(CryptGenKey(
	hCryptProv, 
	ENCRYPT_ALGORITHM, 
	KEYLENGTH | CRYPT_EXPORTABLE, 
	&hKey))
	{
	win_infoarg("A session key has been created. \n");
	} 
	else
	{
	ehExit("Error during CryptGenKey. \n"); 
	}
	//--------------------------------------------------------------
	// Get the handle to the encrypter's exchange public key. 

	if(CryptGetUserKey(
	hCryptProv, 
	AT_KEYEXCHANGE, 
	&hXchgKey))
	{
	win_infoarg("The user public key has been retrieved. \n");
	}
	else
	{ 
	ehExit("User public key is not available \
	and may not exist."); 
	}
	//--------------------------------------------------------------
	// Determine size of the key BLOB, and allocate memory. 

	if(CryptExportKey(
	hKey, 
	hXchgKey, 
	SIMPLEBLOB, 
	0, 
	NULL, 
	&dwKeyBlobLen))
	{
	win_infoarg("The key BLOB is %d bytes long. \n",dwKeyBlobLen);
	}
	else
	{  
	ehExit("Error computing BLOB length! \n");
	}
	if(pbKeyBlob =(BYTE *)ehAlloc(dwKeyBlobLen))
	{ 
	win_infoarg("Memory is allocated for the key BLOB. \n");
	}
	else
	{ 
	ehExit("Out of memory. \n"); 
	}
	//--------------------------------------------------------------
	// Encrypt and export the session key into a simple key BLOB. 

	if(CryptExportKey(
	hKey, 
	hXchgKey, 
	SIMPLEBLOB, 
	0, 
	pbKeyBlob, 
	&dwKeyBlobLen))
	{
	win_infoarg("The key has been exported. \n");
	} 
	else
	{
	ehExit("Error during CryptExportKey!\n");
	} 
	//--------------------------------------------------------------
	// Release the key exchange key handle. 

	if(hXchgKey)
	{
	if(!(CryptDestroyKey(hXchgKey)))
	ehExit("Error during CryptDestroyKey"); 

	hXchgKey = 0;
	}

	//--------------------------------------------------------------
	// Write the size of the key BLOB to a destination file. 

	fwrite(&dwKeyBlobLen, sizeof(DWORD), 1, hDestination); 
	if(ferror(hDestination))
	{ 
	ehExit("Error writing header.");
	}
	else
	{
	win_infoarg("A file header has been written. \n");
	}
	//--------------------------------------------------------------
	// Write the key BLOB to a destination file. 

	fwrite(pbKeyBlob, 1, dwKeyBlobLen, hDestination); 
	if(ferror(hDestination))
	{ 
	ehExit("Error writing header");
	}
	else
	{
	win_infoarg("The key BLOB has been written to the file. \n");
	}
	// Free memory.
	free(pbKeyBlob);
	} 
	else 
	*/
	{ 
		//-------------------------------------------------------------------
		// Il file sarà criptato con una session key derivata dalal password
		// 
		// The session key will be recreated when the file is decrypted
		// only if the password used to create the key is available. 
		//

		//-------------------------------------------------------------------
		// Creo un hash object
		//-------------------------------------------------------------------

		if(CryptCreateHash(
			hCryptProv, 
			CALG_MD5, 
			0, 
			0, 
			&hHash))
		{
			win_infoarg("A hash object has been created. \n");
		}
		else
		{ 
			ehExit("Error during CryptCreateHash!\n");
		}  
		//-------------------------------------------------------------------
		// Hash the password. 
		//
		if(CryptHashData(
			hHash, 
			(BYTE *)szPassword, 
			strlen(szPassword), 
			0))
		{
			win_infoarg("The password has been added to the hash. \n");
		}
		else
		{
			ehExit("Error during CryptHashData. \n"); 
		}
		//-------------------------------------------------------------------
		// Derive a session key from the hash object. 
		//
		if(CryptDeriveKey(
			hCryptProv, 
			ENCRYPT_ALGORITHM, 
			hHash, 
			KEYLENGTH, 
			&hKey)) // <-------------------------
		{
			win_infoarg("An encryption key is derived from the password hash. \n"); 
		}
		else
		{
			ehExit("Error during CryptDeriveKey!\n"); 
		}
		//-------------------------------------------------------------------
		// Destroy hash object. 

		if(hHash) 
		{
			if(!(CryptDestroyHash(hHash)))
				ehExit("Error during CryptDestroyHash"); 
			hHash = 0;
		}
	}

	//-------------------------------------------------------------------
	// The session key is now ready. If it is not a key derived from a 
	// password, the session key encrypted with the encrypter's private 
	// key has been written to the destination file.

	//-------------------------------------------------------------------
	// Determine the number of bytes to encrypt at a time. 
	// This must be a multiple of ENCRYPT_BLOCK_SIZE.
	// ENCRYPT_BLOCK_SIZE is set by a <mark type=keyword>#define</mark> statement.

	dwBlockLen = 1000 - 1000 % ENCRYPT_BLOCK_SIZE; 

	//-------------------------------------------------------------------
	// Determine the block size. If a block cipher is used, 
	// it must have room for an extra block. 

	if(ENCRYPT_BLOCK_SIZE > 1) 
		dwBufferLen = dwBlockLen + ENCRYPT_BLOCK_SIZE; 
	else 
		dwBufferLen = dwBlockLen; 

	//-------------------------------------------------------------------
	// Allocate memory. 
	if(pbBuffer = (BYTE *)ehAlloc(dwBufferLen))
	{
		win_infoarg("Memory has been allocated for the buffer. \n");
	}
	else
	{ 
		ehExit("Out of memory. \n"); 
	}

	//-------------------------------------------------------------------
	// In a do loop, encrypt the source file, 
	// and write to the source file. 

	do 
	{ 

		//-------------------------------------------------------------------
		// Read up to dwBlockLen bytes from the source file. 
		dwCount = fread(pbBuffer, 1, dwBlockLen, hSource); 
		if (ferror(hSource))  ehExit("Error reading plaintext!\n");

		//-------------------------------------------------------------------
		// Encrypt data. 
		if(!CryptEncrypt(hKey, 
			0, 
			feof(hSource), 
			0, 
			pbBuffer, &dwCount, 
			dwBufferLen))
		{ 
			ehExit("Error during CryptEncrypt. \n"); 
		} 

		//-------------------------------------------------------------------
		// Write data to the destination file. 

		fwrite(pbBuffer, 1, dwCount, hDestination); 
		if(ferror(hDestination))
		{ 
			ehExit("Error writing ciphertext.");
		}
	} 
	while(!feof(hSource)); 
	//-------------------------------------------------------------------
	// End the do loop when the last block of the source file has been
	// read, encrypted, and written to the destination file.

	//-------------------------------------------------------------------
	// Close files.

	if (hSource)      {if (fclose(hSource)) ehExit("Error closing source file");}
	if (hDestination) {if(fclose(hDestination)) ehExit("Error closing destination file");}

	//-------------------------------------------------------------------
	// Free memory. 

	if(pbBuffer) ehFree(pbBuffer); 

	//-------------------------------------------------------------------
	// Destroy the session key. 

	if(hKey)
	{
		if(!(CryptDestroyKey(hKey)))
			ehExit("Error during CryptDestroyKey");
	}

	//-------------------------------------------------------------------
	// Release the provider handle. 

	if(hCryptProv)
	{
		if(!(CryptReleaseContext(hCryptProv, 0)))
			ehExit("Error during CryptReleaseContext");
	}
	return(TRUE); 
} // end Encryptfile

CHAR * StrToCryptUrl(CHAR *lpStr,CHAR *Password)
{
	CHAR *lpBufA,*lpBufB;

	lpBufA=strDup(lpStr);
	FCryptStringService(0,lpStr, lpBufA, strlen(lpBufA)+1, Password);
	lpBufB=strEncode(lpBufA,SE_URL,NULL);
	ehFree(lpBufA);
	return lpBufB;
}

CHAR * CryptUrlToStr(CHAR *lpStr,CHAR *Password)
{
	CHAR *lpBufA,*lpBufB;

	lpBufA=strDecode(lpStr,SE_URL,NULL);
	lpBufB=strDup(lpBufA);
	FCryptStringService(1,lpBufA, lpBufB, strlen(lpBufB)+1, Password);
	ehFree(lpBufA);
	return lpBufB;
}



