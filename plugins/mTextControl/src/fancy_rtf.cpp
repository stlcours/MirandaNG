#include "stdafx.h"

struct BBCodeInfo
{
	wchar_t *start;
	wchar_t *end;
	bool(*func)(IFormattedTextDraw *ftd, CHARRANGE range, wchar_t *txt, DWORD cookie);
	DWORD cookie;
};

enum {
	BBS_BOLD_S, BBS_BOLD_E, BBS_ITALIC_S, BBS_ITALIC_E, BBS_UNDERLINE_S, BBS_UNDERLINE_E,
	BBS_STRIKEOUT_S, BBS_STRIKEOUT_E, BBS_COLOR_S, BBS_COLOR_E, BBS_URL1, BBS_URL2,
	BBS_IMG1, BBS_IMG2
};

static bool bbCodeSimpleFunc(IFormattedTextDraw *ftd, CHARRANGE range, wchar_t *, DWORD cookie)
{
	CHARFORMAT cf = { 0 };
	cf.cbSize = sizeof(cf);
	switch (cookie) {
	case BBS_BOLD_S:
		cf.dwMask = CFM_BOLD;
		cf.dwEffects = CFE_BOLD;
		break;
	case BBS_BOLD_E:
		cf.dwMask = CFM_BOLD;
		break;
	case BBS_ITALIC_S:
		cf.dwMask = CFM_ITALIC;
		cf.dwEffects = CFE_ITALIC;
		break;
	case BBS_ITALIC_E:
		cf.dwMask = CFM_ITALIC;
		break;
	case BBS_UNDERLINE_S:
		cf.dwMask = CFM_UNDERLINE;
		cf.dwEffects = CFE_UNDERLINE;
		break;
	case BBS_UNDERLINE_E:
		cf.dwMask = CFM_UNDERLINE;
		break;
	case BBS_STRIKEOUT_S:
		cf.dwMask = CFM_STRIKEOUT;
		cf.dwEffects = CFE_STRIKEOUT;
		break;
	case BBS_STRIKEOUT_E:
		cf.dwMask = CFM_STRIKEOUT;
		break;
	}

	ITextServices *ts = ftd->getTextService();

	LRESULT lResult;
	ts->TxSendMessage(EM_SETSEL, range.cpMin, -1, &lResult);
	ts->TxSendMessage(EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf, &lResult);
	ts->TxSendMessage(EM_SETSEL, range.cpMin, range.cpMax, &lResult);
	ts->TxSendMessage(EM_REPLACESEL, FALSE, (LPARAM)L"", &lResult);

	return true;
}

static bool bbCodeImageFunc(IFormattedTextDraw *ftd, CHARRANGE range, wchar_t *txt, DWORD)
{
	ITextServices *ts = ftd->getTextService();
	ITextDocument *td = ftd->getTextDocument();

	long cnt;
	LRESULT lResult;
	ts->TxSendMessage(EM_SETSEL, range.cpMin, range.cpMax, &lResult);
	IRichEditOle* RichEditOle;
	ts->TxSendMessage(EM_GETOLEINTERFACE, 0, (LPARAM)&RichEditOle, &lResult);
	td->Freeze(&cnt);

#ifdef _WIN64
	bool res = InsertBitmap(RichEditOle, CacheIconToEmf((HICON)_wtoi64(txt)));
#else
	bool res = InsertBitmap(RichEditOle, CacheIconToEmf((HICON)_wtoi(txt)));
#endif

	td->Unfreeze(&cnt);
	RichEditOle->Release();
	return res;
}

static BBCodeInfo bbCodes[] =
{
	{ L"[b]", 0, bbCodeSimpleFunc, BBS_BOLD_S },
	{ L"[/b]", 0, bbCodeSimpleFunc, BBS_BOLD_E },
	{ L"[i]", 0, bbCodeSimpleFunc, BBS_ITALIC_S },
	{ L"[/i]", 0, bbCodeSimpleFunc, BBS_ITALIC_E },
	{ L"[u]", 0, bbCodeSimpleFunc, BBS_UNDERLINE_S },
	{ L"[/u]", 0, bbCodeSimpleFunc, BBS_UNDERLINE_E },
	{ L"[s]", 0, bbCodeSimpleFunc, BBS_STRIKEOUT_S },
	{ L"[/s]", 0, bbCodeSimpleFunc, BBS_STRIKEOUT_E },

	//	{ L"[color=",  L"]",     bbCodeSimpleFunc, BBS_COLOR_S },
	//	{ L"[/color]", 0,           bbCodeSimpleFunc, BBS_COLOR_E }

	{ L"[$hicon=", L"$]", bbCodeImageFunc, 0 }

	//	{ L"[url]",   L"[/url]", bbCodeSimpleFunc, BBS_URL1 },
	//	{ L"[url=",    L"]",     bbCodeSimpleFunc, BBS_URL2 },
	//	{ L"[url]",   L"[/url]", bbCodeSimpleFunc, BBS_IMG1 },
	//	{ L"[url=",    L"]",     bbCodeSimpleFunc, BBS_IMG2 },
};
static int bbCodeCount = sizeof(bbCodes) / sizeof(*bbCodes);

void bbCodeParse(IFormattedTextDraw *ftd)
{
	ITextServices *ts = ftd->getTextService();
	LRESULT lResult;

	int pos = 0;
	for (bool found = true; found;) {
		found = false;
		CHARRANGE fRange; fRange.cpMin = -1;
		wchar_t *fText = 0;
		BBCodeInfo *fBBCode = NULL;

		for (int i = 0; i < bbCodeCount; i++) {
			CHARRANGE range;

			FINDTEXTEX fte;
			fte.chrg.cpMin = pos;
			fte.chrg.cpMax = -1;

			fte.lpstrText = bbCodes[i].start;
			ts->TxSendMessage(EM_FINDTEXTEX, (WPARAM)FR_DOWN, (LPARAM)&fte, &lResult);
			if (lResult == -1)
				continue;
			range = fte.chrgText;

			if (bbCodes[i].end) {
				fte.chrg.cpMin = fte.chrgText.cpMax;
				fte.lpstrText = bbCodes[i].end;
				ts->TxSendMessage(EM_FINDTEXTEX, (WPARAM)FR_DOWN, (LPARAM)&fte, &lResult);
				if (lResult == -1)
					continue;
				range.cpMax = fte.chrgText.cpMax;
			}

			if ((fRange.cpMin == -1) || (fRange.cpMin > range.cpMin)) {
				fRange = range;
				fBBCode = bbCodes + i;
				found = true;

				if (fText)
				{
					delete[] fText;
					fText = nullptr;
				}
				if (bbCodes[i].end) {
					TEXTRANGE trg;
					trg.chrg.cpMin = fte.chrg.cpMin;
					trg.chrg.cpMax = fte.chrgText.cpMin;
					trg.lpstrText = new wchar_t[trg.chrg.cpMax - trg.chrg.cpMin + 1];
					ts->TxSendMessage(EM_GETTEXTRANGE, 0, (LPARAM)&trg, &lResult);
					fText = trg.lpstrText;
				}
			}
		}

		if (found) {
			found = fBBCode->func(ftd, fRange, fText, fBBCode->cookie);
			if (fText) delete[] fText;
		}
	}
}
