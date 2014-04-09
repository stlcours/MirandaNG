#include "common.h"

WORD CSteamProto::SteamToMirandaStatus(int state)
{
	switch (state)
	{
	case 0: //Offline
		return ID_STATUS_OFFLINE;
	case 2: //Busy
		return ID_STATUS_DND;
	case 3: //Away
		return ID_STATUS_AWAY;
		/*case 4: //Snoozing
		prim = PURPLE_STATUS_EXTENDED_AWAY;
		break;
		case 5: //Looking to trade
		return "trade";
		case 6: //Looking to play
		return "play";*/
		//case 1: //Online
	default:
		return ID_STATUS_ONLINE;
	}
}

int CSteamProto::MirandaToSteamState(int status)
{
	switch (status)
	{
	case ID_STATUS_OFFLINE:
		return 0; //Offline
	case ID_STATUS_DND:
		return 2; //Busy
	case ID_STATUS_AWAY:
		return 3; //Away
		/*case 4: //Snoozing
		prim = PURPLE_STATUS_EXTENDED_AWAY;
		break;
		case 5: //Looking to trade
		return "trade";
		case 6: //Looking to play
		return "play";*/
		//case 1: //Online
	default:
		return ID_STATUS_ONLINE;
	}
}

int CSteamProto::RsaEncrypt(const SteamWebApi::RsaKeyApi::RsaKey &rsaKey, const char *data, DWORD dataSize, BYTE *encryptedData, DWORD &encryptedSize)
{
	const char *pszModulus = rsaKey.GetModulus();
	DWORD cchModulus = (DWORD)strlen(pszModulus);

	// convert hex string to byte array
	DWORD cbLen = 0, dwSkip = 0, dwFlags = 0;
	if (!CryptStringToBinaryA(pszModulus, cchModulus, CRYPT_STRING_HEX, NULL, &cbLen, &dwSkip, &dwFlags))
		return GetLastError();

	// allocate a new buffer.
	BYTE *pbBuffer = (BYTE*)malloc(cbLen);
	if (!CryptStringToBinaryA(pszModulus, cchModulus, CRYPT_STRING_HEX, pbBuffer, &cbLen, &dwSkip, &dwFlags))
		return GetLastError();

	// reverse byte array, because of microsoft
	for (int i = 0; i < (int)(cbLen / 2); ++i)
	{
		BYTE temp = pbBuffer[cbLen - i - 1];
		pbBuffer[cbLen - i - 1] = pbBuffer[i];
		pbBuffer[i] = temp;
	}

	HCRYPTPROV hCSP = 0;
	if (!CryptAcquireContext(&hCSP, NULL, NULL, PROV_RSA_AES, CRYPT_SILENT) &&
		!CryptAcquireContext(&hCSP, NULL, NULL, PROV_RSA_AES, CRYPT_SILENT | CRYPT_NEWKEYSET))
		return GetLastError();

	// Move the key into the key container.
	DWORD cbKeyBlob = sizeof(PUBLICKEYSTRUC) + sizeof(RSAPUBKEY) + cbLen;
	BYTE *pKeyBlob = (BYTE*)malloc(cbKeyBlob);

	// Fill in the data.
	PUBLICKEYSTRUC *pPublicKey = (PUBLICKEYSTRUC*)pKeyBlob;
	pPublicKey->bType = PUBLICKEYBLOB; 
	pPublicKey->bVersion = CUR_BLOB_VERSION;  // Always use this value.
	pPublicKey->reserved = 0;                 // Must be zero.
	pPublicKey->aiKeyAlg = CALG_RSA_KEYX;     // RSA public-key key exchange. 

	// The next block of data is the RSAPUBKEY structure.
	RSAPUBKEY *pRsaPubKey = (RSAPUBKEY*)(pKeyBlob + sizeof(PUBLICKEYSTRUC));
	pRsaPubKey->magic = 0x31415352; // RSA1 // Use public key
	pRsaPubKey->bitlen = cbLen * 8;  // Number of bits in the modulus.
	pRsaPubKey->pubexp = 0x10001; // "010001" // Exponent.

	// Copy the modulus into the blob. Put the modulus directly after the
	// RSAPUBKEY structure in the blob.
	BYTE *pKey = (BYTE*)(((BYTE *)pRsaPubKey) + sizeof(RSAPUBKEY));
	//pKeyBlob + sizeof(BLOBHEADER)+ sizeof(RSAPUBKEY); 
	memcpy(pKey, pbBuffer, cbLen);

	// Now import public key
	HCRYPTKEY phKey = 0;
	if (!CryptImportKey(hCSP, pKeyBlob, cbKeyBlob, 0, 0, &phKey))
		return GetLastError();

	// if data is not allocated just renurn size
	if (encryptedData == NULL)
	{
		// get length of encrypted data
		if (!CryptEncrypt(phKey, 0, TRUE, 0, NULL, &encryptedSize, dataSize))
			return GetLastError();
		return 0;
	}

	// encrypt password
	memcpy(encryptedData, data, dataSize);
	if (!CryptEncrypt(phKey, 0, TRUE, 0, encryptedData, &dataSize, encryptedSize))
		return GetLastError();

	// reverse byte array again
	for (int i = 0; i < (int)(encryptedSize / 2); ++i)
	{
		BYTE temp = encryptedData[encryptedSize - i - 1];
		encryptedData[encryptedSize - i - 1] = encryptedData[i];
		encryptedData[i] = temp;
	}

	free(pKeyBlob);
	CryptDestroyKey(phKey);
	
	free(pbBuffer);
	CryptReleaseContext(hCSP, 0);

	return 0;
}