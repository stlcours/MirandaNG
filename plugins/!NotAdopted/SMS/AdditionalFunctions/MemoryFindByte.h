#if !defined(AFX_MEMORYFINDBYTE__H__INCLUDED_)
#define AFX_MEMORYFINDBYTE__H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000



__inline LPVOID MemoryFindByte(SIZE_T dwFrom,LPCVOID lpcSource,SIZE_T dwSourceSize,unsigned char chWhatFind)
{
	LPVOID lpRet=NULL;

	if (lpcSource && dwSourceSize)
	{
		if (dwFrom<dwSourceSize)
		{
			lpRet=memchr((LPCVOID)(((SIZE_T)lpcSource)+dwFrom),chWhatFind,(dwSourceSize-dwFrom));
		}
	}
return(lpRet);
}



__inline LPVOID MemoryFindByteReverse(SIZE_T dwFrom,LPCVOID lpcSource,SIZE_T dwSourceSize,unsigned char chWhatFind)
{
	LPVOID lpRet=NULL;

	__asm
	{
		push	ebx				// ��������� �������
		push	edi				// ��������� �������
		push	esi				// ��������� �������

		mov		ecx,dwSourceSize
		test	ecx,ecx			//; �������� �������� ���������, �� !=0
		je		short end_func

		mov		edi,lpcSource	//; di = string
		test	edi,edi			//; �������� �������� ���������, �� !=0
		jz		short end_func

		mov		eax,dwFrom

/////////////////////////////////////////////
		cmp		eax,ecx			//; �������� ecx(=len)=>dwFrom
		jae		short end_func

		std						//; count 'up' on string this time
		sub		ecx,eax			//; ��������� ������ �� dwFrom(��� ������)
		add		edi,ecx			//; �������� ������ �� dwSourceSize(�� �����)
		mov		al,chWhatFind	//; al=search byte
 		repne	scasb			//; find that byte
		inc		edi				//; di points to byte which stopped scan
		cmp		[edi],al		//; see if we have a hit
		jne		short end_func	//; yes, point to byte
		mov		lpRet,edi		//; ax=pointer to byte
	end_func:

		cld
		pop		esi				// ��������������� ���������� ��������
		pop		edi				// ��������������� ���������� ��������
		pop		ebx				// ��������������� ���������� ��������
	}
return(lpRet);
}



#endif // !defined(AFX_MEMORYFINDBYTE__H__INCLUDED_)
