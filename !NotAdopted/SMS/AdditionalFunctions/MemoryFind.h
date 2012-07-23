#if !defined(AFX_MEMORYFIND__H__INCLUDED_)
#define AFX_MEMORYFIND__H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

__inline LPVOID MemoryFind(SIZE_T dwFrom,LPCVOID lpcSource,SIZE_T dwSourceSize,LPCVOID lpcWhatFind,SIZE_T dwWhatFindSize)
{
	LPVOID lpRet=NULL;

	__asm
	{
		push	ebx					// ��������� �������
		push	edi					// ��������� �������
		push	esi					// ��������� �������

		mov			ecx,dwSourceSize	//; ecx = Source string Size
		test		ecx,ecx				// is size unknown?
		jz			short end_func

		mov			edx,dwWhatFindSize	//; edx = WhatFind string Size
		test		edx,edx				// is size unknown?
		jz			short end_func

		mov			ebx,dwFrom			// ebx - start pos in Source string
		mov			edi,lpcSource		//; edi = Source string
		mov			esi,lpcWhatFind		//; esi = WhatFind string

		cmp			ebx,ecx				// �������� ecx(=len)=>ulFrom
		jae			short end_func

		add			edi,ebx				// �������� ������ �� ulFrom(��� ������)
		sub			ecx,ebx				// ��������� ������ SourceSize �� ulFrom(��� ������)

		cmp			ecx,edx				// �������� NEWSourceSize ??? ulWhatFindSize
		je			short begin_memorycompare	// NEWulSourceSize==ulWhatFindSize, Source ??? WhatFind
		jl			short end_func		// NEWulSourceSize<ulWhatFindSize, => Source!=WhatFind

		sub			ecx,edx				// ��������� ������ SourceSize �� ulWhatFindSize
		inc			ecx

        mov			al,[esi]			//; al=search byte
		dec			edi
		cld								//; �������� � ������ �����������

	find_loop:
		test		ecx,ecx
		jz			short end_func
		inc			edi
		repne		scasb				//; find that byte
        dec			edi					//; di points to byte which stopped scan

        cmp			[edi],al			//; see if we have a hit
        jne			short end_func		//; yes, point to byte

	begin_memorycompare:
		push		esi
		push		edi
		push		ecx
		mov			ecx,edx				//;	ulWhatFindSize ������ (CX ������������ � REPE),
		repe		cmpsb				//;	���������� ��.
		pop			ecx
		pop			edi
		pop			esi
		jne			short find_loop		//; ������� ZF = 0, ���� ������������
										//; ������ �� ��������� (mismatch) match:
										//; ���� �� ������ ����, ������, ���
										//; ��������� (match)
		mov			lpRet,edi			//; ax=pointer to byte
	end_func:

		pop		esi					// ��������������� ���������� ��������
		pop		edi					// ��������������� ���������� ��������
		pop		ebx					// ��������������� ���������� ��������
	}
return(lpRet);
}


#endif // !defined(AFX_MEMORYFIND__H__INCLUDED_)
