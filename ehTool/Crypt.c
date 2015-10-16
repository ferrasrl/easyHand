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
// http://www.rohitab.com/discuss/topic/39777-hmac-md5sha1/

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





 
typedef struct _my_blob{
    BLOBHEADER header;
    DWORD len;
    BYTE key[0];
} my_blob;

/*
int main(int argc, _TCHAR* argv[])
{

    char * hash_sha1 = HMAC("ROSDEVIL", "password", CALG_SHA1);
    char * hash_md5 = HMAC("ROSDEVIL", "password", CALG_MD5);

    cout<<"Hash HMAC-SHA1: "<<hash_sha1<<" ( "<<strlen(hash_sha1)<<" )"<<endl;
    cout<<"Hash HMAC-MD5: "<<hash_md5<<" ( "<<strlen(hash_md5)<<" )"<<endl;
 
    delete [] hash_sha1;
    delete [] hash_md5;

    cin.get();
    return 0;
}
*/

//
// HMAC()
//
CHAR * HMAC(CHAR * pszStr, CHAR * pszPassword, DWORD AlgId,BOOL bBase64Ret) 
{

	HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    HCRYPTKEY hKey = 0;
    HCRYPTHASH hHmacHash = 0;
    BYTE * pbHash = 0;
    DWORD dwDataLen = 0;
    HMAC_INFO HmacInfo;
//	char * res;
    int err = 0;
	DWORD kbSize; 
    //char * temp;
	CHAR szServ[20];
	unsigned int m;
	my_blob * kb = NULL;
	CHAR * pszRet=NULL;

    _(HmacInfo);

    if (AlgId == CALG_MD5){
        HmacInfo.HashAlgid = CALG_MD5;
        pbHash = ehAllocZero(16);
        dwDataLen = 16;
    }else if(AlgId == CALG_SHA1){
        HmacInfo.HashAlgid = CALG_SHA1;
        pbHash = ehAllocZero(20);
        dwDataLen = 20;
    }else{
        return NULL;
    }

    ZeroMemory(pbHash, sizeof(dwDataLen));

//    
    kbSize = sizeof(my_blob) + strlen(pszPassword);

    kb = (my_blob*) ehAlloc(kbSize);
    kb->header.bType = PLAINTEXTKEYBLOB;
    kb->header.bVersion = CUR_BLOB_VERSION;
    kb->header.reserved = 0;
    kb->header.aiKeyAlg = CALG_RC2;
    memcpy(&kb->key, pszPassword, strlen(pszPassword));
    kb->len = strlen(pszPassword);


    if (!CryptAcquireContext(&hProv, NULL, MS_ENHANCED_PROV, PROV_RSA_FULL,CRYPT_VERIFYCONTEXT | CRYPT_NEWKEYSET)){
        err = 1;
        goto Exit;
    }


    if (!CryptImportKey(hProv, (BYTE*)kb, kbSize, 0, CRYPT_IPSEC_HMAC_KEY, &hKey)){
        err = 1;
        goto Exit;
    }

    if (!CryptCreateHash(hProv, CALG_HMAC, hKey, 0, &hHmacHash)){
        err = 1;
        goto Exit;
    }


    if (!CryptSetHashParam(hHmacHash, HP_HMAC_INFO, (BYTE*)&HmacInfo, 0)){
        err = 1;
        goto Exit;
    }

    if (!CryptHashData(hHmacHash, (BYTE*) pszStr, strlen(pszStr), 0)){
        err = 1;
        goto Exit;
    }

    if (!CryptGetHashParam(hHmacHash, HP_HASHVAL, pbHash, &dwDataLen, 0)){
        err = 1;
        goto Exit;
    }

    
//    ZeroMemory(res, dwDataLen * 2);
	if (!bBase64Ret) {
		pszRet=ehAllocZero((dwDataLen * 2)+1);
	//    temp = ehAllocZero(10);
		for (m = 0; m < dwDataLen; m++){
			sprintf(szServ, "%2X", pbHash[m]);
			if (szServ[1]==' ') szServ[1]='0'; // note these two: they are two CORRECTIONS to the conversion in HEX, sometimes the Zeros are
			if (szServ[0]==' ') szServ[0]='0'; // printed with a space, so we replace spaces with zeros; (this error occurs mainly in HMAC-SHA1)
			strcat(pszRet,szServ);
//			strAppend(pszRet,"%s", szServ);
		}
	} else {
	
		pszRet=base64Encode(0,pbHash,dwDataLen);
	
	}
  //  ehFree(temp);

Exit:
    ehFree(kb);
    if(hHmacHash)
        CryptDestroyHash(hHmacHash);
    if(hKey)
        CryptDestroyKey(hKey);
    if(hHash)
        CryptDestroyHash(hHash);
    if(hProv)
        CryptReleaseContext(hProv, 0);

	ehFree(pbHash);

    if (err == 1){
        ehFreePtr(&pszRet);
    }

    return pszRet;
}
//Note: using HMAC-MD5 you could perform the famous CRAM-MD5 used to authenticate
//smtp servers.
#ifdef WITH_OPENSSL 

CHAR * sha1(CHAR * pszStr, BOOL bBase64Ret) 
{
	CHAR * pszRet;
	DWORD m;
	CHAR szServ[20];
	DWORD dwDataLen=SHA_DIGEST_LENGTH;
	unsigned char hash[SHA_DIGEST_LENGTH]; // == 20

	SHA1(pszStr, strlen(pszStr), hash);

	// do some stuff with the hash
	if (!bBase64Ret) {
		pszRet=ehAllocZero((dwDataLen * 2)+1);
	//    temp = ehAllocZero(10);
		for (m = 0; m < dwDataLen; m++){
			sprintf(szServ, "%2.2x", hash[m]);
//			if (szServ[1]==' ') szServ[1]='0'; // note these two: they are two CORRECTIONS to the conversion in HEX, sometimes the Zeros are
//			if (szServ[0]==' ') szServ[0]='0'; // printed with a space, so we replace spaces with zeros; (this error occurs mainly in HMAC-SHA1)
			strcat(pszRet,szServ);
//			strAppend(pszRet,"%s", szServ);
		}
	} else {
	
		pszRet=base64Encode(0,hash,dwDataLen);
	
	}
	return pszRet;

}


#endif

//
// sha1()
//
/*
CHAR * sha1(CHAR * pszStr, BOOL bBase64Ret) 
{

	//--------------------------------------------------------------------
	// Declare variables.
	//
	// hProv:           Handle to a cryptographic service provider (CSP). 
	//                  This example retrieves the default provider for  
	//                  the PROV_RSA_FULL provider type.  
	// hHash:           Handle to the hash object needed to create a hash.
	// hKey:            Handle to a symmetric key. This example creates a 
	//                  key for the RC4 algorithm.
	// hHmacHash:       Handle to an HMAC hash.
	// pbHash:          Pointer to the hash.
	// dwDataLen:       Length, in bytes, of the hash.
	// Data1:           Password string used to create a symmetric key.
	// Data2:           Message string to be hashed.
	// HmacInfo:        Instance of an HMAC_INFO structure that contains 
	//                  information about the HMAC hash.
	// 
	HCRYPTPROV  hProv;
	HCRYPTHASH  hHash;
	HCRYPTKEY   hKey;
	HCRYPTHASH  hHmacHash;
	PBYTE       pbHash;
	DWORD       dwDataLen   = 0;
//	BYTE        Data1[]     = {0x70,0x61,0x73,0x73,0x77,0x6F,0x72,0x64};
//	BYTE        Data2[]     = {0x6D,0x65,0x73,0x73,0x61,0x67,0x65};
	HMAC_INFO   HmacInfo;
	DWORD i;

	//--------------------------------------------------------------------
	// Zero the HMAC_INFO structure and use the SHA1 algorithm for
	// hashing.

	ZeroMemory(&HmacInfo, sizeof(HmacInfo));
	HmacInfo.HashAlgid = CALG_SHA1;

	//--------------------------------------------------------------------
	// Acquire a handle to the default RSA cryptographic service provider.

	if (!CryptAcquireContext(
		&hProv,                   // handle of the CSP
		NULL,                     // key container name
		NULL,                     // CSP name
		PROV_RSA_FULL,            // provider type
		CRYPT_VERIFYCONTEXT))     // no key access is requested
	{
	   printf(" Error in AcquireContext 0x%08x \n",
			  GetLastError());
	   goto ErrorExit;
	}

	//--------------------------------------------------------------------
	// Derive a symmetric key from a hash object by performing the
	// following steps:
	//    1. Call CryptCreateHash to retrieve a handle to a hash object.
	//    2. Call CryptHashData to add a text string (password) to the 
	//       hash object.
	//    3. Call CryptDeriveKey to create the symmetric key from the
	//       hashed password derived in step 2.
	// You will use the key later to create an HMAC hash object. 

	if (!CryptCreateHash(
		hProv,                    // handle of the CSP
		CALG_SHA1,                // hash algorithm to use
		0,                        // hash key
		0,                        // reserved
		&hHash))                  // address of hash object handle
	{
	   printf("Error in CryptCreateHash 0x%08x \n",
			  GetLastError());
	   goto ErrorExit;
	}

	if (!CryptHashData(
		hHash,                    // handle of the hash object
		pszStr,                    // password to hash
		strlen(pszStr),            // number of bytes of data to add
		0))                       // flags
	{
	   printf("Error in CryptHashData 0x%08x \n", 
			  GetLastError());
	   goto ErrorExit;
	}

	if (!CryptDeriveKey(
		hProv,                    // handle of the CSP
		CALG_RC4,                 // algorithm ID
		hHash,                    // handle to the hash object
		0,                        // flags
		&hKey))                   // address of the key handle
	{
	   printf("Error in CryptDeriveKey 0x%08x \n", 
			  GetLastError());
	   goto ErrorExit;
	}

	//--------------------------------------------------------------------
	// Create an HMAC by performing the following steps:
	//    1. Call CryptCreateHash to create a hash object and retrieve 
	//       a handle to it.
	//    2. Call CryptSetHashParam to set the instance of the HMAC_INFO 
	//       structure into the hash object.
	//    3. Call CryptHashData to compute a hash of the message.
	//    4. Call CryptGetHashParam to retrieve the size, in bytes, of
	//       the hash.
	//    5. Call malloc to allocate memory for the hash.
	//    6. Call CryptGetHashParam again to retrieve the HMAC hash.

	if (!CryptCreateHash(
		hProv,                    // handle of the CSP.
		CALG_HMAC,                // HMAC hash algorithm ID
		hKey,                     // key for the hash (see above)
		0,                        // reserved
		&hHmacHash))              // address of the hash handle
	{
	   printf("Error in CryptCreateHash 0x%08x \n", 
			  GetLastError());
	   goto ErrorExit;
	}

	if (!CryptSetHashParam(
		hHmacHash,                // handle of the HMAC hash object
		HP_HMAC_INFO,             // setting an HMAC_INFO object
		(BYTE*)&HmacInfo,         // the HMAC_INFO object
		0))                       // reserved
	{
	   printf("Error in CryptSetHashParam 0x%08x \n", 
			  GetLastError());
	   goto ErrorExit;
	}
	if (!CryptHashData(
		hHmacHash,                // handle of the HMAC hash object
		Data2,                    // message to hash
		sizeof(Data2),            // number of bytes of data to add
		0))                       // flags
	{
	   printf("Error in CryptHashData 0x%08x \n", 
			  GetLastError());
	   goto ErrorExit;
	}
	//--------------------------------------------------------------------
	// Call CryptGetHashParam twice. Call it the first time to retrieve
	// the size, in bytes, of the hash. Allocate memory. Then call 
	// CryptGetHashParam again to retrieve the hash value.

	if (!CryptGetHashParam(
		hHmacHash,                // handle of the HMAC hash object
		HP_HASHVAL,               // query on the hash value
		NULL,                     // filled on second call
		&dwDataLen,               // length, in bytes, of the hash
		0))
	{
	   printf("Error in CryptGetHashParam 0x%08x \n", 
			  GetLastError());
	   goto ErrorExit;
	}

	pbHash = (BYTE*)malloc(dwDataLen);
	if(NULL == pbHash) 
	{
	   printf("unable to allocate memory\n");
	   goto ErrorExit;
	}
	    
	if (!CryptGetHashParam(
		hHmacHash,                 // handle of the HMAC hash object
		HP_HASHVAL,                // query on the hash value
		pbHash,                    // pointer to the HMAC hash value
		&dwDataLen,                // length, in bytes, of the hash
		0))
	{
	   printf("Error in CryptGetHashParam 0x%08x \n", GetLastError());
	   goto ErrorExit;
	}

	// Print the hash to the console.

	printf("The hash is:  ");
	for(i = 0 ; i < dwDataLen ; i++) 
	{
	   printf("%2.2x ",pbHash[i]);
	}
	printf("\n");

	// Free resources.
	ErrorExit:
		if(hHmacHash)
			CryptDestroyHash(hHmacHash);
		if(hKey)
			CryptDestroyKey(hKey);
		if(hHash)
			CryptDestroyHash(hHash);    
		if(hProv)
			CryptReleaseContext(hProv, 0);
		if(pbHash)
			free(pbHash);
		return 0;
}

*/