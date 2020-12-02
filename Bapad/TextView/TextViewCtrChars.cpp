#include "pch.h"
#include "TextView.h"

static const size_t CTRLCHARSNUM = 32;

static const wchar_t* CtrlStr(DWORD ch)
{
	static const wchar_t* reps[CTRLCHARSNUM] =
	{
		L"NUL", L"SOH", L"STX", L"ETX", L"EOT", L"ENQ", L"ACK", L"BEL",
		L"BS",  L"HT",  L"LF",  L"VT",  L"FF",  L"CR",  L"SO",  L"SI",
		L"DLE", L"DC1", L"DC2", L"DC3", L"DC4", L"NAK", L"SYN", L"ETB",
		L"CAN", L"EM",  L"SUB", L"ESC", L"FS",  L"GS",  L"RS",  L"US"
	};

	return ch < ' ' ? reps[ch] : L"???";
}

void PaintRect(HDC hdc, RECT* rect, COLORREF fill)
{
	fill = SetBkColor(hdc, fill);

	ExtTextOutW(hdc, 0, 0, ETO_OPAQUE, rect, 0, 0, 0);

	SetBkColor(hdc, fill);
}



//
//	Return width of specified control-character
//
int TextView::GetCtrlCharWidth(HDC hdc, ULONG64 chValue, FONT* font)
{
	SIZE sz;
	const wchar_t* str = CtrlStr(chValue % 32);
	GetTextExtentPoint32W(hdc, str, wcslen(str), &sz);
	return sz.cx + 4;
}

//
//	Display an ASCII control character in inverted colours
//  to what is currently set in the DC
//
int TextView::PaintCtrlChar(HDC hdc, int xpos, int ypos, ULONG64 chValue, FONT* font)
{
	SIZE  sz;
	RECT  rect;
	const wchar_t* str = CtrlStr(chValue % CTRLCHARSNUM);

	int yoff = maxAscent + heightAbove - font->tm.tmAscent;

	COLORREF fg = GetTextColor(hdc);
	COLORREF bg = GetBkColor(hdc);

	// find out how big the text will be
	GetTextExtentPoint32W(hdc, str, wcslen(str), &sz);
	SetRect(&rect, xpos, ypos, xpos + sz.cx + 4, ypos + lineHeight);

	// paint the background white
	PaintRect(hdc, &rect, bg);

	// adjust rectangle for first black block
	rect.right -= 1;
	rect.top += font->nInternalLeading + yoff;
	rect.bottom = rect.top + font->tm.tmHeight - font->nDescent - font->nInternalLeading;

	// paint the first black block
	PaintRect(hdc, &rect, fg);

	// prepare device context
	fg = SetTextColor(hdc, bg);
	bg = SetBkColor(hdc, fg);

	// paint the text and the second "black" block at the same time
	InflateRect(&rect, -1, 1);
	ExtTextOutW(hdc, xpos + 1, ypos + yoff, ETO_OPAQUE | ETO_CLIPPED, &rect, str, wcslen(str), 0);

	// restore device context
	SetTextColor(hdc, fg);
	SetBkColor(hdc, bg);

	return sz.cx + 4;
}

//
//	Manually calculate the internal-leading and descent
//  values for a font by parsing a small bitmap of a single letter "E"
//	and locating the top and bottom of this letter.
//
void TextView::InitCtrlCharFontAttr(HDC hdc, FONT* font)
{
	// create a temporary off-screen bitmap
	HDC		hdcTemp = CreateCompatibleDC(hdc);
	HBITMAP hbmTemp = CreateBitmap(font->tm.tmAveCharWidth, font->tm.tmHeight, 1, 1, 0);
	HANDLE  hbmOld = SelectObject(hdcTemp, hbmTemp);
	HANDLE  hfnOld = SelectObject(hdcTemp, font->hFont);

	// black-on-white text
	SetTextColor(hdcTemp, RGB(0, 0, 0));
	SetBkColor(hdcTemp, RGB(255, 255, 255));
	SetBkMode(hdcTemp, OPAQUE);

	TextOutW(hdcTemp, 0, 0, L"E", 1);

	// give default values just in case the scan fails
	font->nInternalLeading = font->tm.tmInternalLeading;
	font->nDescent = font->tm.tmDescent;

	// scan downwards looking for the top of the letter 'E'
	for (int y = 0; y < font->tm.tmHeight; y++)
	{
		COLORREF col;

		if ((col = GetPixel(hdcTemp, font->tm.tmAveCharWidth / 2, y)) == RGB(0, 0, 0))
		{
			font->nInternalLeading = y;
			break;
		}
	}

	// scan upwards looking for the bottom of the letter 'E'
	for (LONG y = font->tm.tmHeight - 1; y >= 0; y--)
	{
		COLORREF col;

		if ((col = GetPixel(hdcTemp, font->tm.tmAveCharWidth / 2, y)) == RGB(0, 0, 0))
		{
			font->nDescent = font->tm.tmHeight - y - 1;
			break;
		}
	}

	// give larger fonts a thicker border
	if (font->nInternalLeading > 1 && font->nDescent > 1 && font->tm.tmHeight > 18)
	{
		font->nInternalLeading--;
		font->nDescent--;
	}

	// cleanup
	SelectObject(hdcTemp, hbmOld);
	SelectObject(hdcTemp, hfnOld);
	DeleteDC(hdcTemp);
	DeleteObject(hbmTemp);
}