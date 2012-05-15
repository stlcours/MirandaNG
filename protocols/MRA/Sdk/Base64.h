/*
 * Copyright (c) 2003 Rozhuk Ivan <rozhuk.im@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */


#if !defined(AFX_BASE64__H__INCLUDED_)
#define AFX_BASE64__H__INCLUDED_



#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000



//typedef unsigned char BYTE;
//
//      BASE64 coding:
//      214             46              138
//      11010100        00101110        10001010
//            !             !             !
//      ---------->>> convert 3 8bit to 4 6bit
//      110101  000010  111010  001010
//      53      2       58      10
//      this numbers is offset in array coding below...
//
						   //01234567890123456789012345
static BYTE *pbtCodingTableBASE64=(BYTE*)"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";                   //52...63
static BYTE btDeCodingTableBASE64[256]={64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,62,64,64,64,63,52,53,54,55,56,57,58,59,60,61,64,64,64,64,64,64,64,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,64,64,64,64,64,64,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64};




__inline void BASE64CopyUnSafe(LPCVOID lpcSource,LPCVOID lpcDestination,SIZE_T dwSize,SIZE_T *pdwCopyedSize)
{// �������� ������ �64 �������
	LPBYTE lpbSource,lpbDestination;

	lpbSource=(LPBYTE)lpcSource;
	lpbDestination=(LPBYTE)lpcDestination;
	while(dwSize--)
	{
		if ((*lpbSource)>32 && (*lpbSource)<128) (*lpbDestination++)=(*lpbSource);
		lpbSource++;
	}
	if (pdwCopyedSize) (*pdwCopyedSize)=((SIZE_T)lpbDestination-(SIZE_T)lpcDestination);
}



__inline DWORD BASE64EncodeUnSafe(LPCVOID lpcSource,SIZE_T dwSourceSize,LPCVOID lpcDestination,SIZE_T dwDestinationSize,SIZE_T *pdwEncodedSize)
{// BASE64 �����������
	DWORD dwRetErrorCode;
	SIZE_T dwEncodedSize=((dwSourceSize*4+11)/12*4+1);//(((dwSourceSize+2)/3)*4);
		
	if ((dwDestinationSize<dwEncodedSize))
	{// �������� ������ ������� ���
		dwRetErrorCode=ERROR_INSUFFICIENT_BUFFER;
	}else{// ������ ��������� ������� ����������
		dwEncodedSize=0;
		if(lpcSource && lpcDestination && dwSourceSize)
		{
#ifdef  _WIN64
			LPBYTE lpbtSource=(LPBYTE)lpcSource,lpbtDestination=(LPBYTE)lpcDestination;
			SIZE_T i;

			for (i=0;i<dwSourceSize;i+=3)
			{
				*(lpbtDestination++)=pbtCodingTableBASE64[(*lpbtSource)>>2];									// c1
				*(lpbtDestination++)=pbtCodingTableBASE64[(((*lpbtSource)<<4)&060) | ((lpbtSource[1]>>4)&017)];	// c2
				*(lpbtDestination++)=pbtCodingTableBASE64[((lpbtSource[1]<<2)&074) | ((lpbtSource[2]>>6)&03)];	// c3
				*(lpbtDestination++)=pbtCodingTableBASE64[lpbtSource[2] & 077];									// c4
				lpbtSource+=3;
			}

			// If dwSourceSize was not a multiple of 3, then we have encoded too many characters.  Adjust appropriately.
			if(i==(dwSourceSize+1))
			{// There were only 2 bytes in that last group
				lpbtDestination[-1]='=';
			}else
			if(i==(dwSourceSize+2))
			{// There was only 1 byte in that last group
				lpbtDestination[-1]='=';
				lpbtDestination[-2]='=';
			}

			(*lpbtDestination)=0;
			dwEncodedSize=(lpbtDestination-((LPBYTE)lpcDestination));

#else
		__asm{
			push	ebx					// ��������� �������
			push	edi					// ��������� �������
			push	esi					// ��������� �������
		
			mov		ebx,pbtCodingTableBASE64// ebx = ����� ������� �������������
			mov		ecx,dwSourceSize	// ecx = ������ �������� �������
			mov     edi,lpcDestination	// edi = ����� ��������� �������
			mov     esi,lpcSource		// esi = ��������� �� ������� ������
			cld
			jmp		short read_loop_cmp

		//////////Code function///////////////////////////////////////////
		// ������� ���������
		// eax - �������� ������, ������������ ������ 3 ����� //in buff, 3 byte used
		// eax - ��������� ������, ������������ 4 ����� //out buff, 4 byte used
		code_func:
			// ������ ������ ������� ����������,
			// ������ ������� ��� ������� �����
			bswap   eax
			rol     eax,6
			shl     al,2
			ror     eax,10
			shr     ax,2
			shr     al,2
			xlat
			rol     eax,8
			xlat
			rol     eax,8
			xlat
			rol     eax,8
			xlat
			rol     eax,8
			bswap   eax// 188-235					
			ret
		//////////////////////////////////////////////////////////////////

		/////////////Read & converting cycle//////////////////////////////
		read_loop:
			lodsd						// ������ 4 �����
			dec     esi					// ���������� ������ 3
			and		eax,0x00FFFFFF
		//====================================================
			// ������ ������ ������� ����������,
			// ������ ������� ��� ������� �����
			bswap   eax
			rol     eax,6
			shl     al,2
			ror     eax,10
			shr     ax,2
			shr     al,2
			xlat
			rol     eax,8
			xlat
			rol     eax,8
			xlat
			rol     eax,8
			xlat
			rol     eax,8
			bswap   eax
		//====================================================
			stosd
			sub     ecx,3
		
		read_loop_cmp:
			cmp		ecx,3				// ���������, ����� ������ ���� ��� ������� 4 �����
			jg		short read_loop		// ���� ������ 3 � ����� ����, �� ���������� ������
			
		/////////////////////////////////////////////////////////////////
			xor		eax,eax				// �������� �������
			cmp		ecx,3				// ���������� ������ � 3
			je		short tree_byte_tail// ���� ������ 3 �����, �� ��������� � ������� �������������� ������� ����� ������
			cmp		ecx,2				// ���������� ������ � 2
			je		short two_byte_tail	// ���� ������ 2 �����, �� ��������� � ������� �������������� ������� ����� ������
											// �����, ������ ������� ����� 1
		//////////tail 1 byte////////////////////////////////////////////
			mov		al,byte ptr [esi]	// ������ 1 ����
			call	code_func			// �����������
			and		eax,0x0000FFFF		// �������� ��������� ��� �����
			or		eax,0x3D3D0000		// ���������� � ��������� ��� ����� 61("=")
			jmp		short end_tail_handler	//

		//////////tail 2 byte////////////////////////////////////////////
		two_byte_tail:
			mov		ax,word ptr [esi]	// ������ 2 �����
			call	code_func			// �����������
			and		eax,0x00FFFFFF		// �������� ��������� ����
			or		eax,0x3D000000		// ���������� � ��������� ���� 61("=")
			jmp		short end_tail_handler	//
			
		//////////tail 3 byte////////////////////////////////////////////
		tree_byte_tail:
			lodsw
			ror		eax,16
			mov		al,byte ptr [esi]	// ������ 1 ����
			rol		eax,16
			call	code_func			// �����������

		end_tail_handler:
			stosd						// ���������� 4 �����, ��� ��������������

			sub		edi,lpcDestination	// ��������� ����������� ����, ���������� � �������� ������
			mov		dwEncodedSize,edi	// ���������� ����������� ���� � ������������ ����������
			pop		esi					// ��������������� ���������� ��������
			pop		edi					// ��������������� ���������� ��������
			pop		ebx					// ��������������� ���������� ��������
			}
#endif
			dwRetErrorCode=NO_ERROR;
		}else{
			dwRetErrorCode=ERROR_INVALID_HANDLE;
		}
	}
	if (pdwEncodedSize) (*pdwEncodedSize)=dwEncodedSize;

return(dwRetErrorCode);
}


__inline DWORD BASE64Encode(LPCVOID lpcSource,SIZE_T dwSourceSize,LPCVOID lpcDestination,SIZE_T dwDestinationSize,SIZE_T *pdwEncodedSize)
{// BASE64 �����������
	DWORD dwRetErrorCode;

	__try
	{
		dwRetErrorCode=BASE64EncodeUnSafe(lpcSource,dwSourceSize,lpcDestination,dwDestinationSize,pdwEncodedSize);
	}__except(EXCEPTION_EXECUTE_HANDLER){
		dwRetErrorCode=ERROR_INVALID_ADDRESS;
	}
return(dwRetErrorCode);
}



__inline DWORD BASE64DecodeUnSafe(LPCVOID lpcSource,SIZE_T dwSourceSize,LPCVOID lpcDestination,SIZE_T dwDestinationSize,SIZE_T *pdwDecodedSize)
{// BASE64 �������������
	DWORD dwRetErrorCode;
	SIZE_T dwDecodedSize=((dwSourceSize>>2)*3);// ((dwSourceSize/4)*3);

	if ((dwDestinationSize<dwDecodedSize))
	{// �������� ������ ������� ���
		dwRetErrorCode=ERROR_INSUFFICIENT_BUFFER;
	}else{// ������ ��������� ������� ����������
		dwDecodedSize=0;
		if(lpcSource && lpcDestination)
		{// ������� � �������
			if (dwSourceSize>3)
			{
#ifdef  _WIN64
				LPBYTE lpbtSource=(LPBYTE)lpcSource,lpbtDestination=(LPBYTE)lpcDestination;

				for(SIZE_T i=0;i<dwSourceSize;i+=4)
				{
					*(lpbtDestination++)=(unsigned char) (btDeCodingTableBASE64[(*lpbtSource)] << 2 | btDeCodingTableBASE64[lpbtSource[1]] >> 4);
					*(lpbtDestination++)=(unsigned char) (btDeCodingTableBASE64[lpbtSource[1]] << 4 | btDeCodingTableBASE64[lpbtSource[2]] >> 2);
					*(lpbtDestination++)=(unsigned char) (btDeCodingTableBASE64[lpbtSource[2]] << 6 | btDeCodingTableBASE64[lpbtSource[3]]);
					lpbtSource+=4;
				}

				dwDecodedSize=(lpbtDestination-((LPBYTE)lpcDestination));
				if((*((BYTE*)lpcSource+(dwSourceSize-1)))=='=') dwDecodedSize--;
				if((*((BYTE*)lpcSource+(dwSourceSize-2)))=='=') dwDecodedSize--;


#else
			__asm{
				push	ebx					// ��������� �������
				push	edi					// ��������� �������
				push	esi					// ��������� �������
				
				mov		ebx,offset btDeCodingTableBASE64// ebx = ����� ������� �������������
				mov		ecx,dwSourceSize	// ecx = ������ �������� �������
				mov     edi,lpcDestination	// edi = ����� ��������� �������
				mov     esi,lpcSource		// esi = ��������� �� ������� ������
				cld

			read_loop:
				lodsd						// ������ 4 �����
			//===============bit conversion====================================
			// eax - �������� ������, ������������ ������ 4 ����� //in buff, 4 byte used
			// eax - ��������� ������, ������������ ������ 3 ����� //out buff, 3 byte used
				// ������ ������ ������� �������� ���
				bswap   eax

				ror     eax,8
				xlat

				ror     eax,8
				xlat

				ror     eax,8
				xlat

				ror     eax,8
				xlat

				shl     al,2
				shl     ax,2
				rol     eax,10
				shr     al,2
				ror     eax,6
				bswap   eax
				mov		edx,eax
												//234-250
			//=============================================================== 
				stosd
				dec		edi
				sub     ecx,4
				cmp		ecx,3
				jg		short read_loop

				sub		edi,lpcDestination	// ��������� ����������� ����, ���������� � �������� ������
				mov		dwDecodedSize,edi	// ���������� ����������� ���� � ������������ ����������
				pop		esi					// ��������������� ���������� ��������
				pop		edi					// ��������������� ���������� ��������
				pop		ebx					// ��������������� ���������� ��������
				}

				if((*((BYTE*)lpcSource+(dwSourceSize-1)))=='=') dwDecodedSize--;
				if((*((BYTE*)lpcSource+(dwSourceSize-2)))=='=') dwDecodedSize--;
#endif
				dwRetErrorCode=NO_ERROR;
			}else{// �� ������� ������� ������� ���� ������
				dwRetErrorCode=ERROR_INSUFFICIENT_BUFFER;
			}
		}else{
			dwRetErrorCode=ERROR_INVALID_HANDLE;
		}
	}

	if (pdwDecodedSize) (*pdwDecodedSize)=dwDecodedSize;
return(dwRetErrorCode);
}


__inline DWORD BASE64Decode(LPCVOID lpcSource,SIZE_T dwSourceSize,LPCVOID lpcDestination,SIZE_T dwDestinationSize,SIZE_T *pdwDecodedSize)
{// BASE64 �������������
	DWORD dwRetErrorCode;

	__try
	{
		dwRetErrorCode=BASE64DecodeUnSafe(lpcSource,dwSourceSize,lpcDestination,dwDestinationSize,pdwDecodedSize);
	}__except(EXCEPTION_EXECUTE_HANDLER){
		dwRetErrorCode=ERROR_INVALID_ADDRESS;
	}
return(dwRetErrorCode);
}


__inline DWORD BASE64DecodeFormated(LPCVOID lpcSource,SIZE_T dwSourceSize,LPCVOID lpcDestination,SIZE_T dwDestinationSize,SIZE_T *pdwDecodedSize)
{// BASE64 ������������� � ������������� ��������������
	DWORD dwRetErrorCode;

	if (dwSourceSize<=dwDestinationSize)
	{
		BASE64CopyUnSafe(lpcSource,lpcDestination,dwSourceSize,&dwSourceSize);
		dwRetErrorCode=BASE64DecodeUnSafe(lpcDestination,dwSourceSize,lpcDestination,dwDestinationSize,pdwDecodedSize);
	}else{// �� ������� ������� ������� ���� ������
		dwRetErrorCode=ERROR_INSUFFICIENT_BUFFER;
	}

return(dwRetErrorCode);
}



#endif // !defined(AFX_BASE64__H__INCLUDED_)
