#ifndef _M_CHANGEKEYBOARDLAYOUT_H
#define _M_CHANGEKEYBOARDLAYOUT_H

// ������ ��������� ������ ��� ���� � ��������� �������
// wParam - HWND ����, ��� NULL ��� ���� � ������
// lParam ������ ���� 0
// ���������� 0 � ������ ������ � ��������� �������� (1) ��� ������.
// ����������: ����� "�������� �����" ������������ �� ��������������� ����� ��� �������� ������� �������.
#define MS_CKL_CHANGELAYOUT "ChangeKeyboardLayout/ChangeLayout"

//wParam ������ ���� ����.
//lParam - LPCTSTR ������, ��������� �������� ��������� ����������,
//���������� HKL ��������� ������, ��� NULL � ������ ������.
//����������: ��� ����������� ��������� ����������� ����� "��������� ������ - ������� ���������"
#define MS_CKL_GETLAYOUTOFTEXT "ChangeKeyboardLayout/GetLayoutOfText"

typedef struct  
{
	HKL hklFrom;				//layout of the current text
	HKL hklTo;				    //layout of the result text
	BOOL bTwoWay;
}CKLLayouts;

//wParam - LPCTSTR ��������� ������
//lParam - ��������� �� ��������� CKLLayouts, ���������� ��������� ��� 
//��������� ������ � ����� "���������������� ��������������"
//���������� LPTSTR �� �������������� ������
#define MS_CKL_CHANGETEXTLAYOUT "ChangeKeyboardLayout/ChangeTextLayout"

#endif