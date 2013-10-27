; ���������������� ������ ISWin7 v0.4.2
;
; ��������� Windows Vista ���� ���������� ���������� ���� ���� �������
;
; ��� �������� ������ ��������� ������� ����������� ���� ��� �� ���� �������.
; �������� � Windows 7, Windows 8
;
; � Windows XP � Windows Vista �� ���������� �������������
; ��� �� ������ ����������������� ��������� � ���� ������������ ��������
;
; ���������: VoLT
;
; � ���������� �������� ��������� ������ ��������� ���� ��������� � �������� Windows 7
;
; ������� ������������� �� ����� ������� s00p (������ � nnm-club.ru)
; ���� ��� �� ������� ������� ������ R.G. ReCoding (������ rustorka.com) �� ���� � ������� ��������

[Setup]
AppName=My Program
AppVerName=My Program version 1.5
DefaultDirName={pf}\My Program
DefaultGroupName=My Program
UninstallDisplayIcon={app}\MyProg.exe
Compression=lzma
SolidCompression=yes
OutputDir=.

[Files]
Source: ISWin7.dll; DestDir: {tmp}; Flags: dontcopy

         // Handle ����� ������ ���� WizardForm.Handle.
         // ��������� Left, Top, Right ��� Bottom ������ ����� ������,
         // ���� ���� �� ���� ���������� ����� -1 �� ������ �������� �� �� ����
[Code]   
function win7_init(Handle:HWND; Left, Top, Right, Bottom : Integer): Boolean;
external 'win7_init@files:ISWin7.dll stdcall';

procedure win7_free;
external 'win7_free@files:ISWin7.dll stdcall';

procedure InitializeWizard();
begin
  // ��� ����� ��������� ����������� ��������� ������ �������
  WizardForm.Bevel.Height := 1;
  // �������������� ����������
  if win7_init(WizardForm.Handle, 0, 0, 0, 47) then
  begin
    WizardForm.Caption := '��������';
  end
  else
  begin
    WizardForm.Caption := '���������';
  end;
end;

procedure DeinitializeSetup();
begin
  // ��������� ����������
  win7_free;
end;

procedure CurPageChanged(CurPageID: Integer);
begin
  WizardForm.ReadyPage.Hide;
end;




















