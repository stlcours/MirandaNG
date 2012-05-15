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


#if !defined(AFX_BUFFTOLOWERCASE__H__INCLUDED_)
#define AFX_BUFFTOLOWERCASE__H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000



__inline BOOL BuffToLowerCase(LPCVOID lpcOutBuff,LPCVOID lpcBuff,SIZE_T dwLen)
{
	BOOL bRet=TRUE;

#if defined(_WIN64) || !defined(_WIN32)
	if (lpcOutBuff && lpcBuff && dwLen)
	{
		BYTE bt;
		LPBYTE lpbtIn=(LPBYTE)lpcBuff,lpbtOut=(LPBYTE)lpcOutBuff;

		for(SIZE_T i=dwLen;i;i--)
		{
			bt=(*(lpbtIn++));
			if (bt>='A' && bt<='Z') bt|=32;
			(*(lpbtOut++))=bt;
		}
	}
#else
	__asm
	{
		mov		ecx,dwLen
		test	ecx,ecx
		jz		short end_func

		push	ebx					// ��������� �������
		push	edi					// ��������� �������
		push	esi					// ��������� �������
		mov		esi,lpcBuff
		mov		edi,lpcOutBuff
		mov		bl,'A'
		mov		bh,'Z'
		mov		ah,32
		cld

	lowcaseloop:
		lodsb
		cmp		al,bl
		jl		short savebyte
		cmp		al,bh
		jg		short savebyte
		or		al,ah

	savebyte:
		stosb

		dec		ecx
		jnz		short lowcaseloop

		pop		esi					// ��������������� ���������� ��������
		pop		edi					// ��������������� ���������� ��������
		pop		ebx					// ��������������� ���������� ��������
	end_func:
	}
#endif
return(bRet);
}




#endif // !defined(AFX_BUFFTOLOWERCASE__H__INCLUDED_)