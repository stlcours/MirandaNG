typedef struct
{
	wchar_t Alignment;	// ������������. L - � ����� �������, R - � ������.
	WORD Interval;		// ����������, �� ������� ������� ������ ������� �� ������ ���� ������.
	wchar_t *String;		// ���������� ������.
} RowItemInfo;

/* ������� ��������� ������ � ���������� ������ ����� � ��������������� �� �����.
���������:
InputString - ������ ��� �������;
RowItemsList - ������ ��������� ���������.
������������ �������� - ���������� ��������� � �������. */
WORD GetRowItems(wchar_t *InputString, RowItemInfo **RowItemsList);

/* ������� ���������� ���������� ���� � ��������� ������ ���������� ����. */
BYTE DaysInMonth(BYTE Month, WORD Year);

// ������� ���������� ���� ������ �� ����
// 7 - ��, 1 - �� � �. �.
BYTE DayOfWeek(BYTE Day, BYTE Month, WORD Year);

/* ���������:
	Value - ���������� ����;
	Unit - ������� ��������� (0 - �����, 1 - ���������, 2 - ���������, 3 - �������������);
	Buffer - ����� ������ ��� ������ ����������;
	Size - ������ ������.
������������ ��������: ��������� ������ ������. */
size_t GetFormattedTraffic(DWORD Value, BYTE Unit, wchar_t *Buffer, size_t Size);

size_t GetDurationFormatM(DWORD Duration, wchar_t *Format, wchar_t *Buffer, size_t Size);

signed short int TimeCompare(SYSTEMTIME st1, SYSTEMTIME st2);